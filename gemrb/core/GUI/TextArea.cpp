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
#include "ImageMgr.h"
#include "Video.h"
#include "GUI/EventMgr.h"
#include "GUI/GameControl.h"
#include "GUI/Window.h"
#include "Scriptable/Actor.h"

#define EDGE_PADDING 3

namespace GemRB {

TextArea::TextArea(const Region& frame, Font* text, Font* caps,
				   Color hitextcolor, Color initcolor, Color lowtextcolor)
	: Control(frame), Text(), ftext(text)
{
	ControlType = IE_GUI_TEXTAREA;
	rows = 0;
	TextYPos = 0;
	ticks = starttime = 0;
	Cursor = NULL;
	CurPos = 0;
	Value = -1;
	ResetEventHandler( TextAreaOnChange );
	PortraitResRef[0]=0;
	// quick font optimization (prevents creating unnecessary spans)
	// FIXME: the color/palette for the initials font is unused? why?
	if (caps == ftext) {
		finit = NULL;
		initpalette = NULL;
	} else {
		finit = caps;
		initpalette = core->CreatePalette( initcolor, lowtextcolor );
	}
	palette = core->CreatePalette( hitextcolor, lowtextcolor );
	Color tmp = {
		255, 180, 0, 0
	};
	hoverPal = core->CreatePalette( tmp, lowtextcolor );
	tmp.r = 255;
	tmp.g = 100;
	tmp.b = 0;
	selectedPal = core->CreatePalette( tmp, lowtextcolor );
	dialogOptions = NULL;
	textContainer = NULL;
	Clear();
}

TextArea::~TextArea(void)
{
	ClearDialogOptions();

	gamedata->FreePalette( palette );
	gamedata->FreePalette( initpalette );
	gamedata->FreePalette( hoverPal );
	gamedata->FreePalette( selectedPal );
	core->GetVideoDriver()->FreeSprite( Cursor );
	delete textContainer;
}

void TextArea::RefreshSprite(const char *portrait)
{
	if (AnimPicture) {
		if (!strnicmp(PortraitResRef, portrait, 8) ) {
			return;
		}
		SetAnimPicture(NULL);
	}
	strnlwrcpy(PortraitResRef, portrait, 8);
	ResourceHolder<ImageMgr> im(PortraitResRef, true);
	if (im == NULL) {
		return;
	}

	SetAnimPicture ( im->GetSprite2D() );
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

	Video *video = core->GetVideoDriver();

	if (Flags&IE_GUI_TEXTAREA_SPEAKER) {
		if (AnimPicture) {
			video->BlitSprite(AnimPicture, clip.x, clip.y, true, &clip);
			clip.x+=AnimPicture->Width;
			clip.w-=AnimPicture->Width;
		}
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

	if (dialogOptions) {
		y += textContainer->ContainerFrame().h;
		dialogOptions->DrawContents(x, y);
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
				TextSpan* dc = new TextSpan(text.substr(textpos, 1), finit, initpalette);
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
			//print("pos: %d After: %s", CurPos, lines[CurLine]);
			CalcRowCount();
			RunEventHandler( TextAreaOnChange );
		}
		return true;
	}

	//Selectable=false for dialogs, rather unintuitive, but fact
	if ((Flags & IE_GUI_TEXTAREA_SELECTABLE) || ( Key < '1' ) || ( Key > '9' ))
		return false;
	GameControl *gc = core->GetGameControl();
	if (gc && (gc->GetDialogueFlags()&DF_IN_DIALOG) ) {
		MarkDirty();

		size_t lookupIdx = Key - '1';
		int dlgIdx = -1;
		if (lookupIdx < dialogOptSpans.size()) {
			dlgIdx = dialogOptSpans[lookupIdx].first;
			assert(dlgIdx >= 0);
			gc->dialoghandler->DialogChoose( dlgIdx );
			ClearDialogOptions();
		}
		return true;
	}
	return false;
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
		if (Flags&IE_GUI_TEXTAREA_SPEAKER) {
			const char *portrait = NULL;
			Actor *actor = NULL;
			GameControl *gc = core->GetGameControl();
			if (gc) {
				Scriptable *target = gc->dialoghandler->GetTarget();
				if (target && target->Type == ST_ACTOR) {
					actor = (Actor *)target;
				}
			}
			if (actor) {
				portrait = actor->GetPortrait(1);
			}
			if (portrait) {
				RefreshSprite(portrait);
			}
			if (AnimPicture) {
				// TODO: resize TextContiner to account for AnimPicture
			}
		}
		size_t textHeight = textContainer->ContainerFrame().h;
		if (dialogOptions) {
			textHeight += dialogOptions->ContainerFrame().h;
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
	if (!dialogOptions && !(Flags&IE_GUI_TEXTAREA_SELECTABLE))
		return;

	TextSpan* span = NULL;
	Point p = Point(x, y);
	if (dialogOptions) {
		p.y -= textContainer->ContainerFrame().h;
		span = dialogOptions->SpanAtPoint(p);
	} else {
		span = textContainer->SpanAtPoint(p);
	}

	if (hoverSpan && hoverSpan != span) {
		if (hoverSpan == selectedSpan) {
			hoverSpan->SetPalette(selectedPal);
		} else {
			// reset the old hover span
			// for dialog options we use the "selected" palette for their default look
			hoverSpan->SetPalette((dialogOptions) ? selectedPal : palette);
		}
		hoverSpan = NULL;
	}
	if (span) {
		hoverSpan = span;
		hoverSpan->SetPalette(hoverPal);
	}
	MarkDirty();
}

/** Mouse Button Up */
void TextArea::OnMouseUp(unsigned short /*x*/, unsigned short y,
						 unsigned short Button, unsigned short /*Mod*/)
{
	if (!(Button & (GEM_MB_ACTION|GEM_MB_MENU)) || !hoverSpan)
		return;

	if (selectedSpan) {
		// reset the previous selection
		selectedSpan->SetPalette((dialogOptions) ? selectedPal : palette);
		Value = -1;
	}
	selectedSpan = hoverSpan; // select the item under the mouse
	if (selectedSpan) {
		selectedSpan->SetPalette(selectedPal);

		if (dialogOptions) {
			// FIXME: can't this be done by setting Value and doing a callback?
			// TextArea ideally shouldnt know anything about this
			GameControl* gc = core->GetGameControl();
			if (gc && (gc->GetDialogueFlags()&DF_IN_DIALOG) ) {
				int dlgIdx = -1;
				std::vector<DialogOptionSpan>::const_iterator it;
				for (it = dialogOptSpans.begin(); it != dialogOptSpans.end(); ++it) {
					if( (*it).second == selectedSpan ) {
						dlgIdx = (*it).first;
						break;
					}
				}
				if (dlgIdx == -1) {
					//this kills this object, don't use any more data!
					gc->dialoghandler->EndDialog();
					return;
				}
				gc->dialoghandler->DialogChoose( dlgIdx );
				ClearDialogOptions();
				//return;
				Value = dlgIdx;
			}
		} else {
			Value = (TextYPos + y) / GetRowHeight();
		}

		if (VarName[0] != 0) {
			core->GetDictionary()->SetAt( VarName, Value );
		}
	}
	MarkDirty();

	RunEventHandler( TextAreaOnChange );
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

bool TextArea::SetEvent(int eventType, EventHandler handler)
{
	switch (eventType) {
	case IE_GUI_TEXTAREA_ON_CHANGE:
		TextAreaOnChange = handler;
		break;
	default:
		return false;
	}

	return true;
}

void TextArea::ClearDialogOptions()
{
	dialogOptSpans.clear();
	delete dialogOptions; // deletes the old spans too
	dialogOptions = NULL;
	selectedSpan = NULL;
	hoverSpan = NULL;
}

void TextArea::SetDialogOptions(const std::vector<DialogOption>& opts,
								const Color* color, const Color* hiColor)
{
	// selectedPal is the normal palette for dialog options
	if (selectedPal)
		selectedPal->release();
	if (color)
		selectedPal = core->CreatePalette(*color, ColorBlack);
	else
		selectedPal = ftext->GetPalette();

	if (hiColor) {
		hoverPal->release();
		hoverPal = core->CreatePalette(*hiColor, ColorBlack);
	}

	ClearDialogOptions(); // deletes previous options
	// FIXME: calculate the real frame (padding)
	dialogOptions = new TextContainer(Size(Width - EDGE_PADDING, -1), ftext, selectedPal);
	wchar_t optNum[6];
	for (size_t i = 0; i < opts.size(); i++) {
		swprintf(optNum, sizeof(optNum), L"%d. - ", i+1);
		TextSpan* span = new TextSpan(optNum + opts[i].second, ftext, selectedPal, Size(Width - EDGE_PADDING, 0), IE_FONT_ALIGN_LEFT);
		dialogOptSpans.push_back(std::make_pair(opts[i].first, span));
		dialogOptions->AppendSpan(span); // container owns the span
	}
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

	if (sb) {
		// if we have a scrollbar we should grow as much as needed vertically
		// pad only on left edge
		textContainer = new TextContainer(Size(Width - EDGE_PADDING, -1), ftext, palette);
	} else {
		// otherwise limit the text to our frame
		// pad on both edges
		textContainer = new TextContainer(Size(Width - (EDGE_PADDING * 2), Height), ftext, palette);
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
