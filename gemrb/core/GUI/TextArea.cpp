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
#include "Video.h"
#include "GUI/EventMgr.h"
#include "GUI/Window.h"

#define EDGE_PADDING 3

namespace GemRB {

TextArea::TextArea(const Region& frame, Font* text, Font* caps,
				   Color textcolor, Color initcolor, Color lowtextcolor)
	: Control(frame), Text(), ftext(text), palettes()
{
	ControlType = IE_GUI_TEXTAREA;
	rows = 0;
	TextYPos = 0;
	ticks = starttime = 0;
	Cursor = NULL;
	CurPos = 0;
	Value = -1;
	ResetEventHandler( TextAreaOnChange );
	ResetEventHandler( TextAreaOnSelect );

	if (caps != ftext) {
		// quick font optimization (prevents creating unnecessary spans)
		finit = caps;
		palettes[PALETTE_INITIALS] = core->CreatePalette( initcolor, lowtextcolor );
	} else {
		finit = NULL;
	}

	// init palettes
	SetPalette(&textcolor, PALETTE_NORMAL);
	SetPalette(&initcolor, PALETTE_INITIALS);
	SetPalette(&lowtextcolor, PALETTE_OPTIONS);
	palette = palettes[PALETTE_NORMAL];

	selectOptions = NULL;
	textContainer = NULL;
	Clear();
}

TextArea::~TextArea(void)
{
	for (int i=0; i < PALETTE_TYPE_COUNT; i++) {
		gamedata->FreePalette( palettes[i] );
	}

	ClearSelectOptions();
	core->GetVideoDriver()->FreeSprite( Cursor );
	delete textContainer;
}

bool TextArea::NeedsDraw()
{
	if (Flags&IE_GUI_TEXTAREA_SMOOTHSCROLL) {
		if (TextYPos > textContainer->ContainerFrame().h) {
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
	}

	if (Flags&IE_GUI_TEXTAREA_SMOOTHSCROLL) {
		unsigned long thisTime = GetTickCount();
		if (thisTime>starttime) {
			starttime = thisTime+ticks;
			TextYPos++;// can't use ScrollToY
		}
	}

	int x = clip.x, y = clip.y - TextYPos;
	Region textClip(x, y, clip.w, clip.h);
	textContainer->DrawContents(x, y);

	if (selectOptions) {
		y += textContainer->ContainerFrame().h;
		selectOptions->DrawContents(x, y);
	}
}
/** Sets the Scroll Bar Pointer. If 'ptr' is NULL no Scroll Bar will be linked
	to this Text Area Control. */
int TextArea::SetScrollBar(Control* ptr)
{
	if (!sb && ptr) {
		// only pad left edge
		textContainer->SetMaxFrame(Size(Width - EDGE_PADDING, -1));
	} else if (sb && !ptr) {
		// pad both edges
		textContainer->SetMaxFrame(Size(Width - (EDGE_PADDING * 2), Height));
	}
	int ret = Control::SetScrollBar(ptr);
	CalcRowCount();
	return ret;
}

/** Sets the Actual Text */
void TextArea::SetText(const char* text)
{
	Clear();
	AppendText(text);
}

void TextArea::SetPalette(const Color* color, PALETTE_TYPE idx)
{
	assert(idx < PALETTE_TYPE_COUNT);
	if (color) {
		gamedata->FreePalette(palettes[idx]);
		palettes[idx] = core->CreatePalette( *color, ColorBlack );
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
									w -= lastSpan->SpanFrame().w;
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
								TextSpan* span = new TextSpan(token, fnt, p, frame, align);
								textContainer->AppendSpan(span);
								lastSpan = span;
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
							pal = core->CreatePalette(palCol, palette->back);
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
				TextSpan* dc = new TextSpan(text.substr(textpos, 1), finit, palettes[PALETTE_INITIALS]);
				textContainer->AppendSpan(dc);
				textpos++;
				// FIXME: assuming we have more text!
				// FIXME: the instances of the hard coded numbers are arbitrary padding values
				Size s = Size(Width - EDGE_PADDING - dc->SpanFrame().w, GetRowHeight() + ftext->descent);
				TextSpan* span = new TextSpan(text.substr(textpos), ftext, palette, s, IE_FONT_ALIGN_LEFT);
				textContainer->AppendSpan(span);
				s.w -= 8; // FIXME: arbitrary padding
				// drop cap height + a line descent minus the first line size
				s.h = dc->SpanFrame().h + ftext->descent - GetRowHeight() + 1;// + span->SpanDescent();
				textpos += span->RenderedString().length();
				if (s.h >= ftext->maxHeight) {
					// this is sort of a hack for BAM fonts.
					// drop caps dont exclude their entire frame because of their descent so we do it manually
					textContainer->AddExclusionRect(Region(textContainer->PointForSpan(dc), dc->SpanFrame()));

					span = new TextSpan(text.substr(textpos), ftext, palette, s, IE_FONT_ALIGN_LEFT);
					textContainer->AppendSpan(span);
					textpos += span->RenderedString().length();
				}
			} else {
				textpos = 0;
			}
			textContainer->AppendText(text.substr(textpos));
		} else {
			textContainer->AppendText(text);
		}
		textContainer->ClearSpans();
	}

	Text.append(text);
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

	CalcRowCount();
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

			CurPos++;
			CalcRowCount();
			RunEventHandler( TextAreaOnChange );
		}
		return true;
	}

	if (( Key < '1' ) || ( Key > '9' ))
		return false;

	MarkDirty();

	size_t lookupIdx = Key - '1';
	int dlgIdx = -1;
	if (lookupIdx < OptSpans.size()) {
		dlgIdx = OptSpans[lookupIdx].first;
		assert(dlgIdx >= 0);
		//gc->dialoghandler->DialogChoose( dlgIdx );
		Value = dlgIdx;
		RunEventHandler(TextAreaOnSelect);
	}
	return true;
}

/** Special Key Press */
bool TextArea::OnSpecialKeyPress(unsigned char Key)
{
	size_t len = 0;

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
	CalcRowCount();
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
}

void TextArea::CalcRowCount()
{
	if (textContainer) {
		size_t textHeight = textContainer->ContainerFrame().h;
		if (selectOptions) {
			textHeight += selectOptions->ContainerFrame().h;
		}
		rows = textHeight / GetRowHeight();
	} else {
		rows = 0;
	}
	if (!sb)
		return;
	ScrollBar* bar = ( ScrollBar* ) sb;
	bar->SetMax(rows - (Height / GetRowHeight()));
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
		p.y -= textContainer->ContainerFrame().h;
		p.y += TextYPos;
		// container only has text, so...
		span = dynamic_cast<TextSpan*>(selectOptions->SpanAtPoint(p));
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
			int dlgIdx = -1;
			std::vector<OptionSpan>::const_iterator it;
			for (it = OptSpans.begin(); it != OptSpans.end(); ++it) {
				if( (*it).second == selectedSpan ) {
					dlgIdx = (*it).first;
					break;
				}
			}

			Value = dlgIdx;
			RunEventHandler(TextAreaOnSelect);

			if (VarName[0] != 0) {
				// FIXME: stupid hack. use callbacks instead
				core->GetDictionary()->SetAt( VarName, Value );
			}
		}
	}
	MarkDirty();
}

void TextArea::UpdateState(const char* VariableName, unsigned int Sum)
{
	if (strnicmp( VarName, VariableName, MAX_VARIABLE_LENGTH )) {
		return;
	}
	Value = Sum;
	MarkDirty();
}

void TextArea::SelectText(const char* /*select*/)
{
	// TODO: implement this
}

const String& TextArea::QueryText() const
{
	return Text;
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
	delete selectOptions;
	selectOptions = NULL;
	selectedSpan = NULL;
	hoverSpan = NULL;
}

void TextArea::SetSelectOptions(const std::vector<SelectOption>& opts, bool numbered,
								const Color* color, const Color* hiColor, const Color* selColor)
{
	SetPalette(color, PALETTE_OPTIONS);
	SetPalette(hiColor, PALETTE_HOVER);
	SetPalette(selColor, PALETTE_SELECTED);

	ClearSelectOptions(); // deletes previous options
	// FIXME: calculate the real frame (padding)
	selectOptions = new ContentContainer(Size(Width - EDGE_PADDING, -1), ftext, palettes[PALETTE_SELECTED]);
	wchar_t optNum[6];
	for (size_t i = 0; i < opts.size(); i++) {
		if (numbered) {
			swprintf(optNum, sizeof(optNum), L"%d. - ", i+1);
		}
		TextSpan* span = new TextSpan((numbered) ? optNum + opts[i].second : opts[i].second,
									  ftext, palettes[PALETTE_OPTIONS], Size(Width - EDGE_PADDING, 0), IE_FONT_ALIGN_LEFT);
		OptSpans.push_back(std::make_pair(opts[i].first, span));
		selectOptions->AppendSpan(span); // container owns the span
	}
	UpdateControls();
	// This hack is to refresh the mouse cursor so that reply below cursor gets
	// highlighted during a dialog
	int x,y;
	core->GetVideoDriver()->GetMousePos(x,y);
	core->GetEventMgr()->MouseMove(x,y);
}

void TextArea::Clear()
{
	Text.clear();
	selectedSpan = NULL;
	hoverSpan = NULL;
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
		textContainer = new RestrainedContentContainer(frame, ftext, palette, 50);
	} else {
		textContainer = new ContentContainer(frame, ftext, palette);
	}
}

void TextArea::FlagsChanging(ieDword newFlags)
{
	if ((newFlags^Flags)&IE_GUI_TEXTAREA_HISTORY) {
		// FIXME: not well implemented.
		// any text is lost when changing this flag.
		Clear();
	}
}

//setting up the textarea for smooth scrolling, the first
//TEXTAREA_OUTOFTEXT callback is called automatically
void TextArea::SetupScroll()
{
	// ticks is the number of ticks it takes to scroll this font 1 px
	ticks = 2400 / ftext->maxHeight;
	Clear();
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
