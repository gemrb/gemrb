/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "TextArea.h"

#include "win32def.h"

#include "GameData.h"
#include "Interface.h"
#include "Variables.h"
#include "GUI/EventMgr.h"
#include "GUI/Window.h"

#define EDGE_PADDING 3

namespace GemRB {

TextArea::TextArea(const Region& frame, Font* text)
	: Control(frame), contentWrapper(frame.Dimensions()), ftext(text), palettes()
{
	Init();
}

TextArea::TextArea(const Region& frame, Font* text, Font* caps,
				   Color textcolor, Color initcolor, Color lowtextcolor)
	: Control(frame), contentWrapper(frame.Dimensions()), ftext(text), palettes()
{
	// init palettes
	SetPalette(&textcolor, PALETTE_NORMAL);
	SetPalette(&lowtextcolor, PALETTE_OPTIONS);
	palette = palettes[PALETTE_NORMAL];

	if (caps != ftext) {
		// quick font optimization (prevents creating unnecessary spans)
		finit = caps;
	} else {
		finit = NULL;
		SetPalette(&initcolor, PALETTE_INITIALS);
	}

	Init();
}

void TextArea::Init()
{
	ControlType = IE_GUI_TEXTAREA;
	rows = 0;
	TextYPos = 0;
	ticks = starttime = 0;
	strncpy(VarName, "Selected", sizeof(VarName));

	ResetEventHandler( TextAreaOnChange );
	ResetEventHandler( TextAreaOnSelect );

	selectOptions = NULL;
	textContainer = NULL;

	// initialize the Text containers
	ClearText();
	ClearSelectOptions();
	SetScrollBar(NULL);
}

TextArea::~TextArea(void)
{
	for (int i=0; i < PALETTE_TYPE_COUNT; i++) {
		gamedata->FreePalette( palettes[i] );
	}
}

bool TextArea::NeedsDraw()
{
	if (Flags&IE_GUI_TEXTAREA_SMOOTHSCROLL) {
		if (TextYPos > textContainer->ContentFrame().h) {
			 // the text is offscreen
			return false;
		}
		MarkDirty();
		return true;
	}
	return Control::NeedsDraw();
}

void TextArea::DrawInternal(Region& clip)
{
	// apply padding to the clip
	if (sb) {
		clip.w -= EDGE_PADDING;
	} else {
		clip.w -= EDGE_PADDING * 2;
	}
	clip.x += EDGE_PADDING;

	if (AnimPicture) {
		core->GetVideoDriver()->BlitSprite(AnimPicture, clip.x, clip.y, true, &clip);
		clip.x += AnimPicture->Width;
		clip.w -= AnimPicture->Width;
	}
	contentWrapper.SetFrame(Region(Point(), Size(clip.w, 0)));

	if (Flags&IE_GUI_TEXTAREA_SMOOTHSCROLL) {
		unsigned long thisTime = GetTickCount();
		if (thisTime>starttime) {
			starttime = thisTime+ticks;
			TextYPos++;// can't use ScrollToY
		}
	}

	clip.y -= TextYPos;
	contentWrapper.Draw(clip.Origin());

	// now the text is layed out in the container so we can calculate the rows
	int textHeight = contentWrapper.ContentFrame().h;
	int newRows = 0;
	if (textHeight > 0) {
		newRows = textHeight / GetRowHeight();
	}
	if (newRows == rows) {
		// not only do we not need to do anything,
		// but calling SetMax with the same value causes a jumping stutter
		return;
	}
	rows = newRows;
	if (!sb)
		return;
	ScrollBar* bar = ( ScrollBar* ) sb;
	ieWord visibleRows = (Height / GetRowHeight());
	bar->SetMax((rows > visibleRows) ? (rows - visibleRows) : 0);
}
/** Sets the Scroll Bar Pointer. If 'ptr' is NULL no Scroll Bar will be linked
	to this Text Area Control. */
int TextArea::SetScrollBar(Control* ptr)
{
	if (!sb && ptr) {
		// only pad left edge
		contentWrapper.SetFrame(Region(Point(), Size(Width - EDGE_PADDING, -1)));
	} else if (sb && !ptr) {
		// pad both edges
		contentWrapper.SetFrame(Region(Point(), Size(Width - (EDGE_PADDING * 2), Height)));
	}
	return Control::SetScrollBar(ptr);
}

/** Sets the Actual Text */
void TextArea::SetText(const char* text)
{
	ClearText();
	AppendText(text);
}

void TextArea::SetPalette(const Color* color, PALETTE_TYPE idx)
{
	assert(idx < PALETTE_TYPE_COUNT);
	if (color) {
		gamedata->FreePalette(palettes[idx]);
		palettes[idx] = new Palette( *color, ColorBlack );
	} else if (idx > PALETTE_NORMAL) {
		// default to normal
		gamedata->FreePalette(palettes[idx]);
		palettes[idx] = palettes[PALETTE_NORMAL];
		palettes[idx]->acquire();
	}
}

/** Appends a String to the current Text */
void TextArea::AppendText(const char* text)
{
	if (text) {
		String* string = StringFromCString(text);
		AppendText(*string);
		delete string;
	}
}

void TextArea::AppendText(const String& text)
{
	size_t tagPos = text.find_first_of('[');
	if (tagPos != String::npos) {
		if (tagPos != 0) {
			// handle any text before the markup
			textContainer->AppendText(text.substr(0, tagPos));
		}
		// parse the text looking for accepted tags ([cap], [color], [p])
		// [cap] encloses a span of text to be rendered with the finit font
		// [color=%02X%02X%02X] encloses a span of text to be rendered with the given RGB values
		// [p] encloses a span of text to be rendered as an inline block:
		//     it will grow vertically as needed, but be confined to the remaining width of the line

		// span properties
		Color palCol;
		Palette* pal = NULL;
		Font* fnt = ftext;
		Size frame;
		ieByte align = 0;

		enum ParseState {
			TEXT = 0,
			OPEN_TAG,
			CLOSE_TAG,
			COLOR
		};

		TextSpan* lastSpan = NULL;
		String token;
		ParseState state = TEXT;
		String::const_iterator it = text.begin() + tagPos;
		for (; it != text.end(); it++) {
			switch (state) {
				case OPEN_TAG:
					switch (*it) {
						case '=':
							if (token == L"color") {
								state = COLOR;
								token.clear();
							}
							// else is a parse error...
							continue;
						case ']':
							if (token == L"cap") {
								fnt = finit;
								align = IE_FONT_SINGLE_LINE;
							} else if (token == L"p") {
								int w = Width - EDGE_PADDING;
								if (lastSpan) {
									w -= lastSpan->ContentFrame().w;
								}
								if (AnimPicture) {
									w -= AnimPicture->Width;
								}
								frame.w = w;
							}
							state = TEXT;
							token.clear();
							continue;
					}
					break;
				case CLOSE_TAG:
					switch (*it) {
						case ']':
							if (token == L"color") {
								gamedata->FreePalette(pal);
							} else if (token == L"cap") {
								fnt = ftext;
								align = 0;
							} else if (token == L"p") {
								frame.w = 0;
							}
							state = TEXT;
							token.clear();
							continue;
					}
					break;
				case TEXT:
					switch (*it) {
						case '[':
							if (token.length() && token != L"\n") {
								// FIXME: lazy hack.
								// we ought to ignore all white space between markup unless it contains other text
								Palette* p = pal;
								if (fnt == ftext && p == NULL) {
									p = palette;
								}
								lastSpan = new TextSpan(token, fnt, p, &frame);
								textContainer->AppendContent(lastSpan);
							}
							token.clear();
							if (*++it == '/')
								state = CLOSE_TAG;
							else {
								it--;
								state = OPEN_TAG;
							}
							continue;
					}
					break;
				case COLOR:
					switch (*it) {
						case L']':
							swscanf(token.c_str(), L"%02X%02X%02X", &palCol.r, &palCol.g, &palCol.b);
							pal = new Palette(palCol, palette->back);
							state = TEXT;
							token.clear();
							continue;
					}
					break;
				default: // parse error, not clearing token
					state = TEXT;
					break;
			}
			token += *it;
		}
		assert(pal == NULL && state == TEXT);
		if (token.length()) {
			// there was some text at the end without markup
			textContainer->AppendText(token);
		}
	} else if (text.length()) {
		if (finit) {
			// append cap spans
			size_t textpos = text.find_first_not_of(L"\n\t\r ");
			// FIXME: ? maybe we actually want the newlines etc?
			// I think maybe if we clean up the GUIScripts this isn't needed.
			if (textpos != String::npos) {
				// FIXME: initpalette should *not* be used for drop cap font or state fonts!
				// need to figure out how to handle this because it breaks drop caps

				// we must create and append this span here (instead of using AppendText),
				// because the original data files for the DC font specifies a line height of 13
				// that would cause overlap when the lines wrap beneath the DC if we didnt specify the correct size
				Size s = finit->GetGlyph(text[textpos]).dimensions;
				s.h += finit->descent;
				TextSpan* dc = new TextSpan(text.substr(textpos, 1), finit, palettes[PALETTE_INITIALS], &s);
				textContainer->AppendContent(dc);
				textpos++;
				// FIXME: assuming we have more text!
			} else {
				textpos = 0;
			}
			textContainer->AppendText(text.substr(textpos), ftext, NULL);
		} else {
			textContainer->AppendText(text);
		}
	}
	UpdateControls();
}

int TextArea::InsertText(const char* text, int pos)
{
	// TODO: actually implement this
	AppendText(text);
	return pos;
}

void TextArea::UpdateControls()
{
	int pos;

	if (sb) {
		ScrollBar* bar = ( ScrollBar* ) sb;
		if (Flags & IE_GUI_TEXTAREA_AUTOSCROLL)
			pos = rows - ( Height / ftext->maxHeight );
		else
			pos = 0;
		if (pos < 0)
			pos = 0;
		bar->SetPos( pos );
	} else {
		if (Flags & IE_GUI_TEXTAREA_AUTOSCROLL) {
			pos = rows - ( Height / ftext->maxHeight );
			SetRow(pos);
		}
	}
}

/** Key Press Event */
bool TextArea::OnKeyPress(unsigned char Key, unsigned short /*Mod*/)
{
	if (Flags & IE_GUI_TEXTAREA_EDITABLE) {
		if (Key >= 0x20) {
			MarkDirty();

			// TODO: implement this! currently does nothing

			RunEventHandler( TextAreaOnChange );
		}
		return true;
	}

	if (( Key < '1' ) || ( Key > '9' ))
		return false;

	MarkDirty();

	size_t lookupIdx = Key - '1';
	if (lookupIdx < OptSpans.size()) {
		UpdateState(VarName, lookupIdx);
	}
	return true;
}

/** Special Key Press */
bool TextArea::OnSpecialKeyPress(unsigned char Key)
{
	size_t len = 0;
	size_t CurPos = 0;

	if (!(Flags&IE_GUI_TEXTAREA_EDITABLE)) {
		return false;
	}
	MarkDirty();
	// TODO: implement text editing. (going to be tricky...)
	switch (Key) {
		case GEM_HOME:
			CurPos = 0;
			break;
		case GEM_UP:
			break;
		case GEM_DOWN:
			break;
		case GEM_END:
			break;
		case GEM_LEFT:
			if (CurPos > 0) {
				CurPos--;
			} else {

			}
			break;
		case GEM_RIGHT:
			if (CurPos < len) {
				CurPos++;
			} else {

			}
			break;
		case GEM_DELETE:
			if (CurPos>=len) {
				break;
			}
			break;
		case GEM_BACKSP:
			if (CurPos != 0) {
				if (len<1) {
					break;
				}
				CurPos--;
			} else {

			}
			break;
		 case GEM_RETURN:
			//add an empty line after CurLine
			// TODO: implement this
			//copy the text after the cursor into the new line

			//truncate the current line

			//move cursor to next line beginning
			CurPos=0;
			break;
	}
	RunEventHandler( TextAreaOnChange );
	return true;
}

int TextArea::GetRowHeight() const
{
	return ftext->maxHeight;
}

/** Will scroll y pixels. sender is the control requesting the scroll (ie the scrollbar) */
void TextArea::ScrollToY(unsigned long y, Control* sender)
{
	if (sb && sender != sb) {
		// we must "scale" the pixels
		((ScrollBar*)sb)->SetPosForY(y * (((ScrollBar*)sb)->GetStep() / (double)ftext->maxHeight));
		// sb->SetPosForY will recall this method so we dont need to do more... yet.
	} else if (sb) {
		// our scrollbar has set position for us
		TextYPos = y;
		MarkDirty();
		// refresh the cursor/hover selection
		core->GetEventMgr()->FakeMouseMove();
	} else {
		// no scrollbar. need to call SetRow myself.
		// SetRow will set TextYPos.
		SetRow( y / ftext->maxHeight );
	}
}

/** Set Starting Row */
void TextArea::SetRow(int row)
{
	if (row <= rows) {
		TextYPos = row * GetRowHeight();
	}
	MarkDirty();
	// refresh the cursor/hover selection
	core->GetEventMgr()->FakeMouseMove();
}

/** Mousewheel scroll */
/** This method is key to touchscreen scrolling */
void TextArea::OnMouseWheelScroll(short /*x*/, short y)
{
	if (!(IE_GUI_TEXTAREA_SMOOTHSCROLL & Flags)){
		unsigned long fauxY = TextYPos;
		if ((long)fauxY + y <= 0) fauxY = 0;
		else fauxY += y;
		ScrollToY(fauxY, this);
	}
}

/** Mouse Over Event */
void TextArea::OnMouseOver(unsigned short x, unsigned short y)
{
	if (!selectOptions)
		return;

	TextSpan* span = NULL;
	Point p = Point(x, y);
	if (selectOptions) {
		p.y += TextYPos;
		// container only has text, so...
		span = dynamic_cast<TextSpan*>(selectOptions->ContentAtPoint(p));
	}

	if (hoverSpan && hoverSpan != span) {
		if (hoverSpan == selectedSpan) {
			hoverSpan->SetPalette(palettes[PALETTE_SELECTED]);
		} else {
			// reset the old hover span
			hoverSpan->SetPalette(palettes[PALETTE_OPTIONS]);
		}
		hoverSpan = NULL;
	}
	if (span) {
		hoverSpan = span;
		hoverSpan->SetPalette(palettes[PALETTE_HOVER]);
	}
	MarkDirty();
}

/** Mouse Button Up */
void TextArea::OnMouseUp(unsigned short /*x*/, unsigned short /*y*/,
						 unsigned short Button, unsigned short /*Mod*/)
{
	if (!(Button & (GEM_MB_ACTION|GEM_MB_MENU)) || !hoverSpan)
		return;

	if (selectedSpan) {
		// reset the previous selection
		selectedSpan->SetPalette(palettes[PALETTE_OPTIONS]);
		Value = -1;
	}
	selectedSpan = hoverSpan; // select the item under the mouse
	if (selectedSpan) {
		selectedSpan->SetPalette(palettes[PALETTE_SELECTED]);

		if (selectOptions) {
			int optIdx = 0;
			std::vector<OptionSpan>::const_iterator it;
			for (it = OptSpans.begin(); it != OptSpans.end(); ++it) {
				if( (*it).second == selectedSpan ) {
					break;
				}
				optIdx++;
			}
			UpdateState(VarName, optIdx);
		}
	}
}

void TextArea::UpdateState(const char* VariableName, unsigned int optIdx)
{
	if (!VariableName[0]) {
		return;
	}
	if (!selectOptions) {
		// no selectable options present
		// set state to safe and return
		ClearSelectOptions();
		return;
	}

	// always run the TextAreaOnSelect handler even if the value hasnt changed
	// the *context* of the value can change (dialog) and the handler will want to know 
	Value = OptSpans[optIdx].first;

	// this can be called from elsewhere (GUIScript), so we need to make sure we update the selected span
	selectedSpan = OptSpans[optIdx].second;

	core->GetDictionary()->SetAt( VarName, Value );
	RunEventHandler(TextAreaOnSelect);
}

const String& TextArea::QueryText() const
{
	return textContainer->Text();
}

bool TextArea::SetEvent(int eventType, ControlEventHandler handler)
{
	switch (eventType) {
	case IE_GUI_TEXTAREA_ON_CHANGE:
		TextAreaOnChange = handler;
		break;
	case IE_GUI_TEXTAREA_ON_SELECT:
		TextAreaOnSelect = handler;
		break;
	default:
		return false;
	}

	return true;
}

void TextArea::ClearSelectOptions()
{
	OptSpans.clear();
	contentWrapper.RemoveContent(selectOptions);
	delete selectOptions;
	selectOptions = NULL;
	selectedSpan = NULL;
	hoverSpan = NULL;
	// also set the value to "none"
	Value = -1;
}

void TextArea::SetSelectOptions(const std::vector<SelectOption>& opts, bool numbered,
								const Color* color, const Color* hiColor, const Color* selColor)
{
	SetPalette(color, PALETTE_OPTIONS);
	SetPalette(hiColor, PALETTE_HOVER);
	SetPalette(selColor, PALETTE_SELECTED);

	ClearSelectOptions(); // deletes previous options
	selectOptions = new TextContainer(Size(0, 0), ftext, palettes[PALETTE_SELECTED]);
	wchar_t optNum[6];
	for (size_t i = 0; i < opts.size(); i++) {
		if (numbered) {
			swprintf(optNum, sizeof(optNum), L"%d. - ", i+1);
		}
		TextSpan* span = new TextSpan((numbered) ? optNum + opts[i].second : opts[i].second,
									  ftext, palettes[PALETTE_OPTIONS]);
		OptSpans.push_back(std::make_pair(opts[i].first, span));
		selectOptions->AppendContent(span); // container owns the span
	}
	assert(textContainer);
	contentWrapper.InsertContentAfter(selectOptions, textContainer);
	UpdateControls();
	// This hack is to refresh the mouse cursor so that reply below cursor gets
	// highlighted during a dialog
	core->GetEventMgr()->FakeMouseMove();
}

void TextArea::ClearText()
{
	selectedSpan = NULL;
	hoverSpan = NULL;
	contentWrapper.RemoveContent(textContainer);
	delete textContainer;

	Size frame;
	if (sb) {
		// if we have a scrollbar we should grow as much as needed vertically
		// pad only on left edge
		frame.w = Width - EDGE_PADDING;
		frame.h = -1;
	} else {
		// otherwise limit the text to our frame
		// pad on both edges
		frame.w = Width - (EDGE_PADDING * 2);
		frame.h = Height;
	}
	if (Flags&IE_GUI_TEXTAREA_HISTORY) {
		// limit of 50 spans is roughly 25 messages (1 span for actor, 1 for message)
		//textContainer = new RestrainedContentContainer(frame, ftext, palette, 50);
		//textContainer = new TextContainer(frame, ftext, palette);
	} else {
		//textContainer = new TextContainer(frame, ftext, palette);
	}
	textContainer = new TextContainer(frame, ftext, palette);
	contentWrapper.InsertContentAfter(textContainer, NULL); // make sure its at the top
}

void TextArea::FlagsChanging(ieDword newFlags)
{
	if ((newFlags^Flags)&IE_GUI_TEXTAREA_HISTORY) {
		// FIXME: not well implemented.
		// any text is lost when changing this flag.
		ClearText();
	}
}

//setting up the textarea for smooth scrolling, the first
//TEXTAREA_OUTOFTEXT callback is called automatically
void TextArea::SetupScroll()
{
	// ticks is the number of ticks it takes to scroll this font 1 px
	ticks = 2400 / ftext->maxHeight;
	ClearText();
	TextYPos = -Height; // FIXME: this is somewhat fragile (it is reset by SetRow etc)
	Flags |= IE_GUI_TEXTAREA_SMOOTHSCROLL;
	starttime = GetTickCount();
}

void TextArea::OnMouseDown(unsigned short /*x*/, unsigned short /*y*/, unsigned short Button,
	unsigned short /*Mod*/)
{

	ScrollBar* scrlbr = (ScrollBar*) sb;
	
	if (!scrlbr) {
		Control *ctrl = Owner->GetScrollControl();
		if (ctrl && (ctrl->ControlType == IE_GUI_SCROLLBAR)) {
			scrlbr = (ScrollBar *) ctrl;
		}
	}
	if (scrlbr) {
		switch(Button) {
		case GEM_MB_SCRLUP:
			scrlbr->ScrollUp();
			break;
		case GEM_MB_SCRLDOWN:
			scrlbr->ScrollDown();
			break;
		}
	}
}

void TextArea::SetFocus(bool focus)
{
	Control::SetFocus(focus);
	if (hasFocus && Flags & IE_GUI_TEXTAREA_EDITABLE) {
		core->GetVideoDriver()->ShowSoftKeyboard();
	}
}

}
