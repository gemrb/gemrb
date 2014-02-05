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

namespace GemRB {

TextArea::TextArea(const Region& frame, Font* text, Font* caps,
				   Color hitextcolor, Color /*initcolor*/, Color lowtextcolor)
	: Control(frame), ftext(text)
{
	ControlType = IE_GUI_TEXTAREA;
	rows = 0;
	TextYPos = 0;
	ticks = starttime = 0;
	Cursor = NULL;
	CurPos = 0;
	Value = 0xffffffff;
	ResetEventHandler( TextAreaOnChange );
	PortraitResRef[0]=0;
	// quick font optimization (prevents creating unnecessary spans)
	// FIXME: the color/palette for the initials font is unused? why?
	if (caps == ftext) {
		finit = NULL;
		//initpalette = NULL;
	} else {
		finit = caps;
		//initpalette = core->CreatePalette( initcolor, lowtextcolor );
	}
	palette = core->CreatePalette( hitextcolor, lowtextcolor );
	dialogPal = NULL;
	Color tmp = {
		hitextcolor.b, hitextcolor.g, hitextcolor.r, 0
	};
	selected = core->CreatePalette( tmp, lowtextcolor );
	tmp.r = 255;
	tmp.g = 152;
	tmp.b = 102;
	lineselpal = core->CreatePalette( tmp, lowtextcolor );
	dialogOptions = NULL;
	selectedOption = NULL;
	textContainer = new TextContainer(frame.Dimensions(), ftext, palette);
}

TextArea::~TextArea(void)
{
	ClearDialogOptions();

	gamedata->FreePalette( palette );
	//gamedata->FreePalette( initpalette );
	gamedata->FreePalette( dialogPal );
	gamedata->FreePalette( selected );
	gamedata->FreePalette( lineselpal );
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
		// FIXME: actually check the TextYPos compared to the TA height.
		if (true) { // the text is offscreen
			return false;
		}
		MarkDirty();
		return true;
	}
	return Control::NeedsDraw();
}

void TextArea::DrawInternal(Region& clip)
{
	if (!textContainer) {
		return;
	}

	Video *video = core->GetVideoDriver();
	if (Flags&IE_GUI_TEXTAREA_SPEAKER) {
		if (AnimPicture) {
			video->BlitSprite(AnimPicture, clip.x, clip.y, true, &clip);
			clip.x+=AnimPicture->Width;
			clip.w-=AnimPicture->Width;
		}
	}

	if (Flags&IE_GUI_TEXTAREA_SMOOTHSCROLL)
	{
		unsigned long thisTime = GetTickCount();
		if (thisTime>starttime) {
			starttime = thisTime+ticks;
			TextYPos++;// can't use ScrollToY
			// FIXME: completely broken
			//if (TextYPos % ftext->maxHeight == 0) SetRow(startrow + 1);
		}
	}

	/* lets fake scrolling the text by simply offsetting the textClip by between 0 and maxHeight pixels.
	 don't forget to increase the clipping height by the same amount */
	short LineOffset = (short)(TextYPos % ftext->maxHeight);
	int x = clip.x, y = clip.y - LineOffset;
	Region textClip(x, y, clip.w, clip.h + LineOffset);
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
	int ret = Control::SetScrollBar(ptr);
	CalcRowCount();
	return ret;
}

/** Sets the Actual Text */
void TextArea::SetText(const char* text)
{
	Clear();
	AppendText(text);
	UpdateControls();
}

/** Appends a String to the current Text */
void TextArea::AppendText(const char* text)
{
	if (text) {
		String* string = StringFromCString(text);
		// TODO: parse the string tags ([color],[p],etc) into spans
		//TextSpan* span = new TextSpan(string, ftext, palette, const Size& frame, 0);
		if (finit) {
			// FIXME: this assumes that finit is always for drop caps (not just a diffrent looking font)
			// append drop cap spans
			TextSpan* dc = new TextSpan(string->substr(0, 1), finit, NULL);
			textContainer->AppendSpan(dc);
			string->erase(0, 1);
			Size s = Size(Width - 5 - dc->SpanFrame().w, GetRowHeight());
			TextSpan* span = new TextSpan(*string, ftext, palette, s, IE_FONT_ALIGN_LEFT);
			textContainer->AppendSpan(span);
			s.w -= 5; // FIXME: this is arbitrary
			s.h = dc->SpanFrame().h - GetRowHeight();
			size_t textLen = span->RenderedString().length() - 1;
			span = new TextSpan(string->substr(textLen), ftext, palette, s, IE_FONT_ALIGN_RIGHT);
			textContainer->AppendSpan(span);
			textLen += span->RenderedString().length() - 1;
			// append the remainder
			textContainer->AppendText(string->substr(textLen));
		} else {
			textContainer->AppendText(*string);
		}
		delete string;
		UpdateControls();
	}
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

	GameControl* gc = core->GetGameControl();
	if (gc && gc->GetDialogueFlags()&DF_IN_DIALOG) {
		// This hack is to refresh the mouse cursor so that reply below cursor gets
		// highlighted during a dialog
		// FIXME: we check DF_IN_DIALOG here to avoid recurssion in the MessageWindowLogger, but what happens when an error happens during dialog?
		// I'm not super sure about how to avoid that. for now the logger will not log anything in dialog mode.
		int x,y;
		core->GetVideoDriver()->GetMousePos(x,y);
		core->GetEventMgr()->MouseMove(x,y);
	}

	core->RedrawAll();
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
	}else if(sb){
		// our scrollbar has set position for us
		TextYPos = y;
	}else{
		// no scrollbar. need to call SetRow myself.
		// SetRow will set TextYPos.
		SetRow( y / ftext->maxHeight );
	}
}

/** Set Starting Row */
void TextArea::SetRow(int row)
{
	if (row <= rows) {
		// FIXME: this is super wrong now
		TextYPos = row * ftext->maxHeight;
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
	bar->SetMax(rows);
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
	if (!dialogOptions) return;

	Point p = Point(x, y - textContainer->ContainerFrame().h);
	TextSpan* hoverSpan = dialogOptions->SpanAtPoint(p);

	if (selectedOption && selectedOption != hoverSpan) {
		MarkDirty();
		selectedOption->SetPalette(dialogPal);
		selectedOption = NULL;
	} else if (hoverSpan) {
		MarkDirty();
		selectedOption = hoverSpan;
		selectedOption->SetPalette(selected);
	}
}

/** Mouse Button Up */
void TextArea::OnMouseUp(unsigned short /*x*/, unsigned short /*y*/,
						 unsigned short Button, unsigned short /*Mod*/)
{
	if (!(Button & (GEM_MB_ACTION|GEM_MB_MENU)))
		return;

	if (selectedOption) {
		MarkDirty();

		GameControl* gc = core->GetGameControl();
		if (gc && (gc->GetDialogueFlags()&DF_IN_DIALOG) ) {
			int dlgIdx = -1;
			std::vector<DialogOptionSpan>::const_iterator it;
			for (it = dialogOptSpans.begin(); it != dialogOptSpans.end(); ++it) {
				if( (*it).second == selectedOption ) {
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
			return;
		}
	}

	if (VarName[0] != 0) {
		core->GetDictionary()->SetAt( VarName, Value );
	}
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
	// FIXME: implement this properly
	return Control::QueryText();
	/*
	if ( Value<lines.size() ) {
		return ( const char * ) lines[Value];
	}
	return "";
	 */
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
	selectedOption = NULL;
}

void TextArea::SetDialogOptions(const std::vector<DialogOption>& opts,
								const Color* color, const Color* hiColor)
{
	dialogPal->release();
	if (color)
		dialogPal = core->CreatePalette(*color, ColorBlack);
	else
		dialogPal = ftext->GetPalette();

	if (hiColor) {
		selected->release();
		selected = core->CreatePalette(*hiColor, ColorBlack);
	}

	ClearDialogOptions(); // deletes previous options
	// FIXME: calculate the real frame (padding)
	dialogOptions = new TextContainer(Size(Width, Height), ftext, dialogPal);
	wchar_t optNum[6];
	for (size_t i = 0; i < opts.size(); i++) {
		swprintf(optNum, sizeof(optNum), L"%d. - ", i+1);
		TextSpan* span = new TextSpan(optNum + opts[i].second, ftext, dialogPal, Size(Width, 0), IE_FONT_ALIGN_LEFT);
		dialogOptSpans.push_back(std::make_pair(opts[i].first, span));
		dialogOptions->AppendSpan(span); // container owns the span
	}
}

void TextArea::Clear()
{
	delete textContainer;
	textContainer = new TextContainer(ControlFrame().Dimensions(), ftext, palette);;
}

//setting up the textarea for smooth scrolling, the first
//TEXTAREA_OUTOFTEXT callback is called automatically
void TextArea::SetupScroll()
{
	// ticks is the number of ticks it takes to scroll this font 1 px
	ticks = 2400 / ftext->maxHeight;
	//clearing the textarea
	Clear();
	//unsigned int i = (unsigned int) (1 + ((Height - 1) / ftext->maxHeight)); // ceiling
	// FIXME: set the TextYPos out of bounds below the TA
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
			core->RedrawAll();
			break;
		case GEM_MB_SCRLDOWN:
			scrlbr->ScrollDown();
			core->RedrawAll();
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
