/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "GUI/Button.h"

#include "GUI/GameControl.h"
#include "GUI/EventMgr.h"
#include "GUI/ScrollBar.h"
#include "GUI/Window.h"

#include "win32def.h"
#include "defsounds.h"
#include "ie_cursors.h"

#include "GameData.h"
#include "Interface.h"
#include "Palette.h"
#include "Variables.h"
#include "Video.h"

namespace GemRB {

Button::Button()
{
	Unpressed = Pressed = Selected = Disabled = NULL;
	State = IE_GUI_BUTTON_UNPRESSED;
	ResetEventHandler( ButtonOnPress );
	ResetEventHandler( ButtonOnDoublePress );
	ResetEventHandler( ButtonOnShiftPress );
	ResetEventHandler( ButtonOnRightPress );
	ResetEventHandler( ButtonOnDragDrop );
	ResetEventHandler( ButtonOnDrag );
	ResetEventHandler( MouseEnterButton );
	ResetEventHandler( MouseLeaveButton );
	ResetEventHandler( MouseOverButton );
	//Text = ( char * ) calloc( 64, sizeof(char) );
	Text = NULL;
	hasText = false;
	font = core->GetButtonFont();
	normal_palette = NULL;
	disabled_palette = font->GetPalette()->Copy();
	for (int i = 0; i < 256; i++) {
		disabled_palette->col[i].r = ( disabled_palette->col[i].r * 2 ) / 3;
		disabled_palette->col[i].g = ( disabled_palette->col[i].g * 2 ) / 3;
		disabled_palette->col[i].b = ( disabled_palette->col[i].b * 2 ) / 3;
	}
	Flags = IE_GUI_BUTTON_NORMAL;
	ToggleState = false;
	Picture = NULL;
	Clipping = 1.0;
	memset(&SourceRGB,0,sizeof(SourceRGB));
	memset(&DestRGB,0,sizeof(DestRGB));
	memset( borders, 0, sizeof( borders ));
	starttime = 0;
	Anchor.null();
	PushOffset = Point(2, 2);
}
Button::~Button()
{
	Video* video = core->GetVideoDriver();
	video->FreeSprite( Disabled );
	video->FreeSprite( Selected );
	video->FreeSprite( Pressed );
	video->FreeSprite( Unpressed );
	video->FreeSprite( Picture );
	ClearPictureList();
	if (Text) {
		free( Text );
	}
	gamedata->FreePalette( normal_palette);
	gamedata->FreePalette( disabled_palette);
}
/** Sets the 'type' Image of the Button to 'img'.
'type' may assume the following values:
- IE_GUI_BUTTON_UNPRESSED
- IE_GUI_BUTTON_PRESSED
- IE_GUI_BUTTON_SELECTED
- IE_GUI_BUTTON_DISABLED */
void Button::SetImage(unsigned char type, Sprite2D* img)
{
	switch (type) {
		case IE_GUI_BUTTON_UNPRESSED:
		case IE_GUI_BUTTON_LOCKED:
		case IE_GUI_BUTTON_LOCKED_PRESSED:
			core->GetVideoDriver()->FreeSprite( Unpressed );
			Unpressed = img;
			break;

		case IE_GUI_BUTTON_SECOND:
		case IE_GUI_BUTTON_PRESSED:
			core->GetVideoDriver()->FreeSprite( Pressed );
			Pressed = img;
			break;

		case IE_GUI_BUTTON_SELECTED:
			core->GetVideoDriver()->FreeSprite( Selected );
			Selected = img;
			break;

		case IE_GUI_BUTTON_DISABLED:
		case IE_GUI_BUTTON_THIRD:
			core->GetVideoDriver()->FreeSprite( Disabled );
			Disabled = img;
			break;
	}
	Changed = true;
}

/** make SourceRGB go closer to DestRGB */
void Button::CloseUpColor()
{
	if (!starttime) return;
	//using the realtime timer, because i don't want to
	//handle Game at this point
	unsigned long newtime;

	Changed = true;
	newtime = GetTickCount();
	if (newtime<starttime) {
		return;
	}

	Color nc;

	nc.r = (SourceRGB.r + DestRGB.r) / 2;
	nc.g = (SourceRGB.g + DestRGB.g) / 2;
	nc.b = (SourceRGB.b + DestRGB.b) / 2;
	nc.a = (SourceRGB.a + DestRGB.a) / 2;
	if (SourceRGB.r == nc.r &&
		SourceRGB.g == nc.g &&
		SourceRGB.b == nc.b &&
		SourceRGB.a == nc.a) {
		SourceRGB = DestRGB;
		starttime = 0;
		return;
	}

	SourceRGB = nc;
	starttime = newtime + 40;
}

/** Draws the Control on the Output Display */
void Button::Draw(unsigned short x, unsigned short y)
{
	if (!Changed && !(Owner->Flags&WF_FLOAT) ) {
		return;
	}
	Changed = false;
	if (XPos == 65535 || Width == 0) {
		return;
	}

	Video * video = core->GetVideoDriver();

	// Button image
	if (!( Flags & IE_GUI_BUTTON_NO_IMAGE )) {
		Sprite2D* Image = NULL;

		switch (State) {
			case IE_GUI_BUTTON_UNPRESSED:
			case IE_GUI_BUTTON_LOCKED:
			case IE_GUI_BUTTON_LOCKED_PRESSED:
				Image = Unpressed;
				break;

			case IE_GUI_BUTTON_SECOND:
			case IE_GUI_BUTTON_PRESSED:
				Image = Pressed;
				if (! Image)
					Image = Unpressed;
				break;

			case IE_GUI_BUTTON_SELECTED:
				Image = Selected;
				if (! Image)
					Image = Unpressed;
				break;

			case IE_GUI_BUTTON_DISABLED:
			case IE_GUI_BUTTON_THIRD:
				Image = Disabled;
				if (! Image)
					Image = Unpressed;
				break;
		}
		if (Image) {
			// FIXME: maybe it's useless...
			int xOffs = ( Width / 2 ) - ( Image->Width / 2 );
			int yOffs = ( Height / 2 ) - ( Image->Height / 2 );

			video->BlitSprite( Image, x + XPos + xOffs, y + YPos + yOffs, true );
		}
	}

	if (State == IE_GUI_BUTTON_PRESSED) {
		//shift the writing/border a bit
		x+= PushOffset.x;
		y+= PushOffset.y;
	}

	// Button picture
	if (Picture  && (Flags & IE_GUI_BUTTON_PICTURE) ) {
		// Picture is drawn centered
		int xOffs = ( Width / 2 ) - ( Picture->Width / 2 );
		int yOffs = ( Height / 2 ) - ( Picture->Height / 2 );
		if (Flags & IE_GUI_BUTTON_HORIZONTAL) {
			xOffs += x + XPos + Picture->XPos;
			yOffs += y + YPos + Picture->YPos;

			// Clipping: 0 = overlay over full button, 1 = no overlay
			int overlayHeight = Picture->Height * (1.0 - Clipping);
			if (overlayHeight < 0)
				overlayHeight = 0;
			if (overlayHeight >= Picture->Height)
				overlayHeight = Picture->Height;
			int buttonHeight = Picture->Height - overlayHeight;

			Region rb = Region(xOffs, yOffs, Picture->Width, buttonHeight);
			Region ro = Region(xOffs, yOffs + buttonHeight, Picture->Width, overlayHeight);

			video->BlitSprite( Picture, xOffs, yOffs, true, &rb );

			// TODO: Add an option to add BLIT_GREY to the flags
			video->BlitGameSprite( Picture, xOffs, yOffs, BLIT_TINTED, SourceRGB, 0, 0, &ro, true);

			// do NOT uncomment this, you can't change Changed or invalidate things from
			// the middle of Window::DrawWindow() -- it needs moving to somewhere else
			//CloseUpColor();
		}
		else {
			Region r( x + XPos + xOffs, y + YPos + yOffs, (int)(Picture->Width * Clipping), Picture->Height );
			video->BlitSprite( Picture, x + XPos + xOffs + Picture->XPos, y + YPos + yOffs + Picture->YPos, true, &r );
		}
	}

	// Composite pictures (paperdolls/description icons)
	if (!PictureList.empty() && (Flags & IE_GUI_BUTTON_PICTURE) ) {
		std::list<Sprite2D*>::iterator iter = PictureList.begin();
		int xOffs = 0, yOffs = 0;
		if (Flags & IE_GUI_BUTTON_CENTER_PICTURES) {
			// Center the hotspots of all pictures
			xOffs = Width/2;
			yOffs = Height/2;
		} else if (Flags & IE_GUI_BUTTON_BG1_PAPERDOLL) {
			// Display as-is
			xOffs = 0;
			yOffs = 0;
		} else {
			// Center the first picture, and align the rest to that
			xOffs = Width/2 - (*iter)->Width/2 + (*iter)->XPos;
			yOffs = Height/2 - (*iter)->Height/2 + (*iter)->YPos;
		}

		for (; iter != PictureList.end(); ++iter) {
			video->BlitSprite( *iter, x + XPos + xOffs, y + YPos + yOffs, true );
		}
	}

	// Button picture
	if (AnimPicture) {
		int xOffs = ( Width / 2 ) - ( AnimPicture->Width / 2 );
		int yOffs = ( Height / 2 ) - ( AnimPicture->Height / 2 );
		Region r( x + XPos + xOffs, y + YPos + yOffs, (int)(AnimPicture->Width * Clipping), AnimPicture->Height );

		if (Flags & IE_GUI_BUTTON_CENTER_PICTURES) {
			video->BlitSprite( AnimPicture, x + XPos + xOffs + AnimPicture->XPos, y + YPos + yOffs + AnimPicture->YPos, true, &r );
		} else {
			video->BlitSprite( AnimPicture, x + XPos + xOffs, y + YPos + yOffs, true, &r );
		}
	}

	// Button label
	if (hasText && ! ( Flags & IE_GUI_BUTTON_NO_TEXT )) {
		Palette* ppoi = normal_palette;
		int align = 0;

		if (State == IE_GUI_BUTTON_DISABLED)
			ppoi = disabled_palette;
		// FIXME: hopefully there's no button which sinks when selected
		//   AND has text label
		//else if (State == IE_GUI_BUTTON_PRESSED || State == IE_GUI_BUTTON_SELECTED) {

		if (Flags & IE_GUI_BUTTON_ALIGN_LEFT)
			align |= IE_FONT_ALIGN_LEFT;
		else if (Flags & IE_GUI_BUTTON_ALIGN_RIGHT)
			align |= IE_FONT_ALIGN_RIGHT;
		else
			align |= IE_FONT_ALIGN_CENTER;

		if (Flags & IE_GUI_BUTTON_ALIGN_TOP)
			align |= IE_FONT_ALIGN_TOP;
		else if (Flags & IE_GUI_BUTTON_ALIGN_BOTTOM)
			align |= IE_FONT_ALIGN_BOTTOM;
		else
			align |= IE_FONT_ALIGN_MIDDLE;

		if (! (Flags & IE_GUI_BUTTON_MULTILINE)) {
			align |= IE_FONT_SINGLE_LINE;
		}
		font->Print( Region( x + XPos, y + YPos, Width - 2, Height - 2),
			( unsigned char * ) Text, ppoi,
			(ieByte) align, true );
	}

	if (! (Flags&IE_GUI_BUTTON_NO_IMAGE)) {
		for (int i = 0; i < MAX_NUM_BORDERS; i++) {
			ButtonBorder *fr = &borders[i];
			if (! fr->enabled) continue;

			Region r = Region( x + XPos + fr->dx1, y + YPos + fr->dy1, Width - (fr->dx1 + fr->dx2 + 1), Height - (fr->dy1 + fr->dy2 + 1) );
			video->DrawRect( r, fr->color, fr->filled );
		}
	}
}
/** Sets the Button State */
void Button::SetState(unsigned char state)
{
	if (state > IE_GUI_BUTTON_LOCKED_PRESSED) {// If wrong value inserted
		return;
	}
	if (State != state) {
		Changed = true;
		State = state;
	}
}
void Button::SetBorder(int index, int dx1, int dy1, int dx2, int dy2, const Color &color, bool enabled, bool filled)
{
	if (index >= MAX_NUM_BORDERS)
		return;

	ButtonBorder *fr = &borders[index];
	fr->dx1 = dx1;
	fr->dy1 = dy1;
	fr->dx2 = dx2;
	fr->dy2 = dy2;
	fr->color = color;
	fr->enabled = enabled;
	fr->filled = filled;
	Changed = true;
}

void Button::EnableBorder(int index, bool enabled)
{
	if (index >= MAX_NUM_BORDERS)
		return;

	if (borders[index].enabled != enabled) {
		borders[index].enabled = enabled;
		Changed = true;
	}
}

void Button::SetFont(Font* newfont)
{
	font = newfont;
}
/** Handling The default button (enter) */
void Button::OnSpecialKeyPress(unsigned char Key)
{
	if (State != IE_GUI_BUTTON_DISABLED && State != IE_GUI_BUTTON_LOCKED) {
		if (Key == GEM_RETURN) {
			if (Flags & IE_GUI_BUTTON_DEFAULT ) {
				RunEventHandler( ButtonOnPress );
				return;
			}
		}
		else if (Key == GEM_ESCAPE) {
			if (Flags & IE_GUI_BUTTON_CANCEL ) {
				RunEventHandler( ButtonOnPress );
				return;
			}
		}
	}
	Control::OnSpecialKeyPress(Key);
}

/** Mouse Button Down */
void Button::OnMouseDown(unsigned short x, unsigned short y,
	unsigned short Button, unsigned short Mod)
{
	if (State == IE_GUI_BUTTON_DISABLED) {
		Control::OnMouseDown(x,y,Button,Mod);
		return;
	}

	if (core->GetDraggedItem () && !ButtonOnDragDrop) {
		Control::OnMouseDown(x,y,Button,Mod);
		return;
	}

	ScrollBar* scrlbr = (ScrollBar*) sb;
	if (!scrlbr) {
		Control *ctrl = Owner->GetScrollControl();
		if (ctrl && (ctrl->ControlType == IE_GUI_SCROLLBAR)) {
			scrlbr = (ScrollBar *) ctrl;
		}
	}

	//Button == 1 means Left Mouse Button
	switch(Button&GEM_MB_NORMAL) {
	case GEM_MB_ACTION:
		// We use absolute screen position here, so drag_start
		//   remains valid even after window/control is moved
		drag_start.x = Owner->XPos + XPos + x;
		drag_start.y = Owner->YPos + YPos + y;

		if (State == IE_GUI_BUTTON_LOCKED) {
			SetState( IE_GUI_BUTTON_LOCKED_PRESSED );
			return;
		}
		SetState( IE_GUI_BUTTON_PRESSED );
		if (Flags & IE_GUI_BUTTON_SOUND) {
			core->PlaySound( DS_BUTTON_PRESSED );
		}
		if ((Button & GEM_MB_DOUBLECLICK) && ButtonOnDoublePress) {
			RunEventHandler( ButtonOnDoublePress );
		}
		break;
	case GEM_MB_SCRLUP:
		if (scrlbr) {
			scrlbr->ScrollUp();
		}
		break; 
	case GEM_MB_SCRLDOWN:
		if (scrlbr) {
			scrlbr->ScrollDown();
		}
		break;
	}
}
/** Mouse Button Up */
void Button::OnMouseUp(unsigned short x, unsigned short y,
	unsigned short Button, unsigned short Mod)
{
	if (State == IE_GUI_BUTTON_DISABLED) {
		return;
	}

	//what was just dropped?
	int dragtype = 0;
	if (core->GetDraggedItem ()) dragtype=1;
	if (core->GetDraggedPortrait ()) dragtype=2;

	//if something was dropped, but it isn't handled here: it didn't happen
	if (dragtype && !ButtonOnDragDrop)
		return;

	switch (State) {
	case IE_GUI_BUTTON_PRESSED:
		if (ToggleState) {
			SetState( IE_GUI_BUTTON_SELECTED );
		} else {
			SetState( IE_GUI_BUTTON_UNPRESSED );
		}
		break;
	case IE_GUI_BUTTON_LOCKED_PRESSED:
		SetState( IE_GUI_BUTTON_LOCKED );
		break;
	}

	//in case of dragged/dropped portraits, allow the event to happen even
	//when we are out of bound
	if (dragtype!=2) {
		if (( x >= Width ) || ( y >= Height )) {
			return;
		}
	}
	if (Flags & IE_GUI_BUTTON_CHECKBOX) {
		//checkbox
		ToggleState = !ToggleState;
		if (ToggleState)
			SetState( IE_GUI_BUTTON_SELECTED );
		else
			SetState( IE_GUI_BUTTON_UNPRESSED );
		if (VarName[0] != 0) {
			ieDword tmp = 0;
			core->GetDictionary()->Lookup( VarName, tmp );
			tmp ^= Value;
			core->GetDictionary()->SetAt( VarName, tmp );
			Owner->RedrawControls( VarName, tmp );
		}
	} else {
		if (Flags & IE_GUI_BUTTON_RADIOBUTTON) {
			//radio button
			ToggleState = true;
			SetState( IE_GUI_BUTTON_SELECTED );
		}
		if (VarName[0] != 0) {
			core->GetDictionary()->SetAt( VarName, Value );
			Owner->RedrawControls( VarName, Value );
		}
	}

	switch (dragtype) {
		case 1:
			RunEventHandler( ButtonOnDragDrop );
			return;
		case 2:
			RunEventHandler( ButtonOnDragDropPortrait );
			return;
	}

	if ((Button&GEM_MB_NORMAL) == GEM_MB_ACTION) {
		if ((Mod & GEM_MOD_SHIFT) && ButtonOnShiftPress)
			RunEventHandler( ButtonOnShiftPress );
		else
			RunEventHandler( ButtonOnPress );
	} else {
		if (Button == GEM_MB_MENU && ButtonOnRightPress)
			RunEventHandler( ButtonOnRightPress );
	}
}

void Button::OnMouseWheelScroll(short x, short y)
{
	ScrollBar* scrlbr = (ScrollBar*) sb;
	if (!scrlbr) {
		Control *ctrl = Owner->GetScrollControl();
		if (ctrl && (ctrl->ControlType == IE_GUI_SCROLLBAR)) {
			scrlbr = (ScrollBar *) ctrl;
		}
	}
	
	if (scrlbr) {
		scrlbr->OnMouseWheelScroll(x, y);
	}
}

void Button::OnMouseOver(unsigned short x, unsigned short y)
{
	Owner->Cursor = IE_CURSOR_NORMAL;
	if (State == IE_GUI_BUTTON_DISABLED) {
		return;
	}

	if ( RunEventHandler( MouseOverButton )<0) {
		//event handler destructed this object
		return;
	}

	//well, no more flags for buttons, and the portraits we can perform action on
	//are in fact 'draggable multiline pictures' (with image)
	if ((Flags & IE_GUI_BUTTON_DISABLED_P) == IE_GUI_BUTTON_PORTRAIT) {
		GameControl *gc = core->GetGameControl();
		if (gc) {
			Owner->Cursor = gc->GetDefaultCursor();
		}
	}

	if (State == IE_GUI_BUTTON_LOCKED) {
		return;
	}

	//portrait buttons are draggable and locked
	if ((Flags & IE_GUI_BUTTON_DRAGGABLE) && 
			      (State == IE_GUI_BUTTON_PRESSED || State ==IE_GUI_BUTTON_LOCKED_PRESSED)) {
		// We use absolute screen position here, so drag_start
		//   remains valid even after window/control is moved
		int dx = Owner->XPos + XPos + x - drag_start.x;
		int dy = Owner->YPos + YPos + y - drag_start.y;
		core->GetDictionary()->SetAt( "DragX", dx );
		core->GetDictionary()->SetAt( "DragY", dy );
		drag_start.x = (ieWord) (drag_start.x + dx);
		drag_start.y = (ieWord) (drag_start.y + dy);
		RunEventHandler( ButtonOnDrag );
	}
}

void Button::OnMouseEnter(unsigned short /*x*/, unsigned short /*y*/)
{
	if (State == IE_GUI_BUTTON_DISABLED) {
		return;
	}

	if (MouseEnterButton !=0 && VarName[0] != 0) {
		core->GetDictionary()->SetAt( VarName, Value );
	}

	RunEventHandler( MouseEnterButton );
}

void Button::OnMouseLeave(unsigned short /*x*/, unsigned short /*y*/)
{
	if (State == IE_GUI_BUTTON_DISABLED) {
		return;
	}

	if (MouseLeaveButton !=0 && VarName[0] != 0) {
		core->GetDictionary()->SetAt( VarName, Value );
	}

	RunEventHandler( MouseLeaveButton );
}


/** Sets the Text of the current control */
void Button::SetText(const char* string)
{
	free(Text);
	Text = NULL;
	if (string == NULL) {
		hasText = false;
	} else if (string[0] == 0) {
		hasText = false;
	} else {
		Text = strndup( string, 255 );
		if (Flags&IE_GUI_BUTTON_LOWERCASE)
			strlwr( Text );
		else if (Flags&IE_GUI_BUTTON_CAPS)
			strupr( Text );
		hasText = true;
	}
	Changed = true;
}

/** Set Event Handler */
bool Button::SetEvent(int eventType, EventHandler handler)
{
	Changed = true;

	switch (eventType) {
		case IE_GUI_BUTTON_ON_PRESS:
			ButtonOnPress = handler;
			break;
		case IE_GUI_MOUSE_OVER_BUTTON:
			MouseOverButton = handler;
			break;
		case IE_GUI_MOUSE_ENTER_BUTTON:
			MouseEnterButton = handler;
			break;
		case IE_GUI_MOUSE_LEAVE_BUTTON:
			MouseLeaveButton = handler;
			break;
		case IE_GUI_BUTTON_ON_SHIFT_PRESS:
			ButtonOnShiftPress = handler;
			break;
		case IE_GUI_BUTTON_ON_RIGHT_PRESS:
			ButtonOnRightPress = handler;
			break;
		case IE_GUI_BUTTON_ON_DRAG_DROP:
			ButtonOnDragDrop = handler;
			break;
		case IE_GUI_BUTTON_ON_DRAG_DROP_PORTRAIT:
			ButtonOnDragDropPortrait = handler;
			break;
		case IE_GUI_BUTTON_ON_DRAG:
			ButtonOnDrag = handler;
			break;
		case IE_GUI_BUTTON_ON_DOUBLE_PRESS:
			ButtonOnDoublePress = handler;
			break;
	default:
		return false;
	}

	return true;
}

/** Redraws a button from a given radio button group */
void Button::RedrawButton(const char* VariableName, unsigned int Sum)
{
	if (strnicmp( VarName, VariableName, MAX_VARIABLE_LENGTH )) {
		return;
	}
	if (State == IE_GUI_BUTTON_DISABLED) {
		return;
	}
	if (Flags & IE_GUI_BUTTON_RADIOBUTTON) {
		ToggleState = ( Sum == Value );
	}   	//radio button, exact value
	else if (Flags & IE_GUI_BUTTON_CHECKBOX) {
		ToggleState = !!( Sum & Value );
	} //checkbox, bitvalue
	else {
		return;
	} //other buttons, nothing to redraw
	if (ToggleState) {
		SetState(IE_GUI_BUTTON_SELECTED);
	} else {
		SetState(IE_GUI_BUTTON_UNPRESSED);
	}
}
/** Sets the Picture */
void Button::SetPicture(Sprite2D* newpic)
{
	core->GetVideoDriver()->FreeSprite( Picture );
	ClearPictureList();
	Picture = newpic;
	Changed = true;
	Flags |= IE_GUI_BUTTON_PICTURE;
	Owner->Invalidate();
}

/** Clears the list of Pictures */
void Button::ClearPictureList()
{
	Video* video = core->GetVideoDriver();
	for (std::list<Sprite2D*>::iterator iter = PictureList.begin();
		 iter != PictureList.end(); ++iter)
		video->FreeSprite( *iter );
	PictureList.clear();
	Changed = true;
	Owner->Invalidate();
}

/** Add picture to the end of the list of Pictures */
void Button::StackPicture(Sprite2D* Picture)
{
	PictureList.push_back(Picture);
	Changed = true;
	Flags |= IE_GUI_BUTTON_PICTURE;
	Owner->Invalidate();
}

bool Button::IsPixelTransparent(unsigned short x, unsigned short y)
{
	// some buttons have hollow Image frame filled w/ Picture
	// some buttons in BG2 are text only (if BAM == 'GUICTRL')
	if (Picture || PictureList.size() || ! Unpressed) return false;
	return Unpressed->IsPixelTransparent(x, y);
}

// Set palette used for drawing button label in normal state
void Button::SetTextColor(const Color &fore, const Color &back)
{
	gamedata->FreePalette( normal_palette );
	normal_palette = core->CreatePalette( fore, back );
	Changed = true;
}

void Button::SetHorizontalOverlay(double clip, const Color &/*src*/, const Color &dest)
{
	if ((Clipping>clip) || !(Flags&IE_GUI_BUTTON_HORIZONTAL) ) {
		Flags |= IE_GUI_BUTTON_HORIZONTAL;
#if 0
		// FIXME: This doesn't work while CloseUpColor isn't being called
		// (see Draw)
		SourceRGB=src;
		DestRGB=dest;
		starttime = GetTickCount();
		starttime += 40;
#else
		SourceRGB = DestRGB = dest;
		starttime = 0;
#endif
	}
	Clipping = clip;
	Changed = true;
}

void Button::SetAnchor(ieWord x, ieWord y)
{
	Anchor = Point(x,y);
}

void Button::SetPushOffset(ieWord x, ieWord y)
{
	PushOffset = Point(x,y);
}

}
