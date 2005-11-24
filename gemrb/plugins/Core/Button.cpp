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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Button.cpp,v 1.91 2005/11/24 17:44:08 wjpalenstijn Exp $
 *
 */

#include "../../includes/win32def.h"
#include "Button.h"
#include "Interface.h"
#include "Video.h"
#include "Variables.h"
#include "../../includes/defsounds.h"

Button::Button(bool Clear)
{
	Unpressed = Pressed = Selected = Disabled = NULL;
	this->Clear = Clear;
	State = IE_GUI_BUTTON_UNPRESSED;
	ResetEventHandler( ButtonOnPress );
	ResetEventHandler( ButtonOnShiftPress );
	ResetEventHandler( ButtonOnRightPress );
	ResetEventHandler( ButtonOnDragDrop );
	ResetEventHandler( MouseEnterButton );
	ResetEventHandler( MouseLeaveButton );
	ResetEventHandler( MouseOverButton );
	Text = ( char * ) calloc( 64, sizeof(char) );
	hasText = false;
	font = core->GetButtonFont();
	normal_palette = NULL;
	disabled_palette = ( Color * ) malloc( 256 * sizeof( Color ) );
	memcpy( disabled_palette, font->GetPalette(), 256 * sizeof( Color ) );
	for (int i = 0; i < 256; i++) {
		disabled_palette[i].r = ( disabled_palette[i].r * 2 ) / 3;
		disabled_palette[i].g = ( disabled_palette[i].g * 2 ) / 3;
		disabled_palette[i].b = ( disabled_palette[i].b * 2 ) / 3;
	}
	Flags = IE_GUI_BUTTON_NORMAL;
	ToggleState = false;
	Picture = NULL;
	Picture2 = NULL;
	Clipping = 1.0;
	memset( borders, 0, sizeof( borders ));
}
Button::~Button()
{
	Video* video = core->GetVideoDriver();
	if (Clear) {
		if (Unpressed)
			video->FreeSprite( Unpressed );
		if (Pressed && ( Pressed != Unpressed ))
			video->FreeSprite( Pressed );
		if (Selected &&
			( ( Selected != Pressed ) && ( Selected != Unpressed ) ))
			video->FreeSprite( Selected );
		if (Disabled &&
			( ( Disabled != Pressed ) &&
			( Disabled != Unpressed ) &&
			( Disabled != Selected ) ))
			video->FreeSprite( Disabled );
	}
	if (Picture) {
		video->FreeSprite( Picture );
	}
	if (Picture2) {
		video->FreeSprite( Picture2 );
	}
	if (Text) {
		free( Text );
	}
	if (normal_palette)
		video->FreePalette( normal_palette );
	video->FreePalette( disabled_palette );
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
			 {
				if (Unpressed && Clear)
					core->GetVideoDriver()->FreeSprite( Unpressed );
				Unpressed = img;
			}
			break;

		case IE_GUI_BUTTON_PRESSED:
			 {
				if (Pressed && Clear)
					core->GetVideoDriver()->FreeSprite( Pressed );
				Pressed = img;
			}
			break;

		case IE_GUI_BUTTON_SELECTED:
			 {
				if (Selected && Clear)
					core->GetVideoDriver()->FreeSprite( Selected );
				Selected = img;
			}
			break;

		case IE_GUI_BUTTON_DISABLED:
			 {
				if (Disabled && Clear)
					core->GetVideoDriver()->FreeSprite( Disabled );
				Disabled = img;
			}
			break;
	}
	Changed = true;
}
/** Draws the Control on the Output Display */
void Button::Draw(unsigned short x, unsigned short y)
{
	if (!Changed && !(((Window*)Owner)->Flags&WF_FLOAT) ) {
		return;
	}
	Changed = false;
	if (XPos == 65535) {
		return;
	}

	// Button image
	if (!( Flags & IE_GUI_BUTTON_NO_IMAGE )) {
		Sprite2D* Image = NULL;

		switch (State) {
			case IE_GUI_BUTTON_UNPRESSED:
			case IE_GUI_BUTTON_LOCKED:
				Image = Unpressed;
				break;

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
				Image = Disabled;
				if (! Image)
					Image = Unpressed;
				break;
		}
		if (Image) {
			// FIXME: maybe it's useless...
			short xOffs = ( Width / 2 ) - ( Image->Width / 2 );
			short yOffs = ( Height / 2 ) - ( Image->Height / 2 );

			core->GetVideoDriver()->BlitSprite( Image, x + XPos + xOffs, y + YPos + yOffs, true );
		}
	}

	if (State == IE_GUI_BUTTON_PRESSED) {
		//shift the writing/border a bit
		x+= 2;
		y+= 2;
	}

	// Button picture
	if (Picture && ( Flags & IE_GUI_BUTTON_PICTURE )) {
		int h = Picture->Height;
		if (Picture2) {
			h += Picture2->Height;
		}
		short xOffs = ( Width / 2 ) - ( Picture->Width / 2 );
		short yOffs = ( Height / 2 ) - ( h / 2 );
		Region r( x + XPos + xOffs, y + YPos + yOffs, (int)(Picture->Width * Clipping), h );
		core->GetVideoDriver()->BlitSprite( Picture, x + XPos + xOffs, y + YPos + yOffs, true, &r );
		if (Picture2) {
			core->GetVideoDriver()->BlitSprite( Picture2, x + XPos + xOffs, y + YPos + yOffs + Picture->Height, true, &r );
		}
	}

	// Button picture
	if (AnimPicture) {
		short xOffs = ( Width / 2 ) - ( AnimPicture->Width / 2 );
		short yOffs = ( Height / 2 ) - ( AnimPicture->Height / 2 );
		Region r( x + XPos + xOffs, y + YPos + yOffs, (int)(AnimPicture->Width * Clipping), AnimPicture->Height );
		core->GetVideoDriver()->BlitSprite( AnimPicture, x + XPos + xOffs, y + YPos + yOffs, true, &r );
	}



	// Button label
	if (hasText && ! ( Flags & IE_GUI_BUTTON_NO_TEXT )) {
		Color* ppoi = normal_palette;
		int align = 0;

		if (State == IE_GUI_BUTTON_DISABLED)
			ppoi = disabled_palette;
		// FIXME: hopefully there's no button which sunks when selected
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

		font->Print( Region( x + XPos, y + YPos, Width - 2, Height - 2 ),
			     ( unsigned char * ) Text, ppoi,
			     align | IE_FONT_SINGLE_LINE, true );
	}

	for (int i = 0; i < MAX_NUM_BORDERS; i++) {
		ButtonBorder *fr = &borders[i];
		if (! fr->enabled) continue;

		Region r = Region( x + XPos + fr->dx1, y + YPos + fr->dy1, Width - (fr->dx1 + fr->dx2 + 1), Height - (fr->dy1 + fr->dy2 + 1) );
		core->GetVideoDriver()->DrawRect( r, fr->color, fr->filled );
	}
}
/** Sets the Button State */
void Button::SetState(unsigned char state)
{
	if (state > IE_GUI_BUTTON_LOCKED) // If wrong value inserted
	{
		return;
	}
	if (State != state) {
		Changed = true;
	}
	State = state;
}
void Button::SetBorder(int index, int dx1, int dy1, int dx2, int dy2, Color* color, bool enabled, bool filled)
{
	if (index >= MAX_NUM_BORDERS)
		return;

	ButtonBorder *fr = &borders[index];
	fr->dx1 = dx1;
	fr->dy1 = dy1;
	fr->dx2 = dx2;
	fr->dy2 = dy2;
	memcpy( &(fr->color), color, sizeof( Color ));
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
	if (State == IE_GUI_BUTTON_DISABLED || State == IE_GUI_BUTTON_LOCKED) {
		return;
	}
	if (Flags & IE_GUI_BUTTON_DEFAULT) {
		if (Key == GEM_RETURN) {
			RunEventHandler( ButtonOnPress );
		}
	}
}

/** Mouse Button Down */
void Button::OnMouseDown(unsigned short x, unsigned short y,
	unsigned char Button, unsigned short /*Mod*/)
{
	if (State == IE_GUI_BUTTON_DISABLED || State == IE_GUI_BUTTON_LOCKED) {
		return;
	}

	if (core->GetDraggedItem () && !ButtonOnDragDrop[0])
		return;

	//Button == 1 means Left Mouse Button
	if (Button == 1) {
		State = IE_GUI_BUTTON_PRESSED;
		Changed = true;
		if (Flags & IE_GUI_BUTTON_SOUND) {
			core->PlaySound( DS_BUTTON_PRESSED );
		}
		// We use absolute screen position here, so drag_start
		//   remains valid even after window/control is moved
		drag_start.x = ((Window*)Owner)->XPos + XPos + x;
		drag_start.y = ((Window*)Owner)->YPos + YPos + y;
	}
}
/** Mouse Button Up */
void Button::OnMouseUp(unsigned short x, unsigned short y,
	unsigned char Button, unsigned short Mod)
{
	if (State == IE_GUI_BUTTON_DISABLED || State == IE_GUI_BUTTON_LOCKED) {
		return;
	}
	if (core->GetDraggedItem () && !ButtonOnDragDrop[0])
		return;

	if (State == IE_GUI_BUTTON_PRESSED) {
		if (ToggleState) {
			SetState( IE_GUI_BUTTON_SELECTED );
		} else {
			SetState( IE_GUI_BUTTON_UNPRESSED );
		}
	}
	if (( x <= Width ) && ( y <= Height )) {
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
				( ( Window * ) Owner )->RedrawControls( VarName, tmp );
			}
		} else {
			if (Flags & IE_GUI_BUTTON_RADIOBUTTON) {
				//radio button
				ToggleState = true;
				SetState( IE_GUI_BUTTON_SELECTED );
			}
			if (VarName[0] != 0) {
				core->GetDictionary()->SetAt( VarName, Value );
				( ( Window * ) Owner )->RedrawControls( VarName, Value );
			}
		}

		if (core->GetDraggedItem()) {
			RunEventHandler( ButtonOnDragDrop );
		} else if (Button == GEM_MB_ACTION) {
			if ((Mod == 1) && ButtonOnShiftPress[0])
				RunEventHandler( ButtonOnShiftPress );
			else 
				RunEventHandler( ButtonOnPress );
		} else if (Button == GEM_MB_MENU && ButtonOnRightPress[0]) 
			RunEventHandler( ButtonOnRightPress );
		
	}
}

void Button::OnMouseOver(unsigned short x, unsigned short y)
{
	if (State == IE_GUI_BUTTON_DISABLED) {
		return;
	}

	RunEventHandler( MouseOverButton );

	if (State == IE_GUI_BUTTON_LOCKED) {
		return;
	}

	( ( Window * ) Owner )->Cursor = IE_CURSOR_NORMAL;

	if ((Flags & IE_GUI_BUTTON_DRAGGABLE) && (State == IE_GUI_BUTTON_PRESSED)) {
		// We use absolute screen position here, so drag_start
		//   remains valid even after window/control is moved
		int dx = ((Window*)Owner)->XPos + XPos + x - drag_start.x;
		int dy = ((Window*)Owner)->YPos + YPos + y - drag_start.y;
		core->GetDictionary()->SetAt( "DragX", dx );
		core->GetDictionary()->SetAt( "DragY", dy );
		drag_start.x += dx;
		drag_start.y += dy;
		RunEventHandler( ButtonOnDrag );
	}
}

void Button::OnMouseEnter(unsigned short /*x*/, unsigned short /*y*/)
{
	if (State == IE_GUI_BUTTON_DISABLED) {
		return;
	}

	RunEventHandler( MouseEnterButton );
}

void Button::OnMouseLeave(unsigned short /*x*/, unsigned short /*y*/)
{
	if (State == IE_GUI_BUTTON_DISABLED) {
		return;
	}

	RunEventHandler( MouseLeaveButton );
}


/** Sets the Text of the current control */
int Button::SetText(const char* string, int /*pos*/)
{
	if (string == NULL) {
		hasText = false;
	} else if (string[0] == 0) {
		hasText = false;
	} else {
		if (core->HasFeature( GF_UPPER_BUTTON_TEXT ))
			strnuprcpy( Text, string, 63 );
		else
			strncpy( Text, string, 63 );
		hasText = true;
	}
	Changed = true;
	return 0;
}

/** Set Event Handler */
bool Button::SetEvent(int eventType, EventHandler handler)
{
	Changed = true;

	switch (eventType) {
		case IE_GUI_BUTTON_ON_PRESS:
			SetEventHandler( ButtonOnPress, handler );
			break;
		case IE_GUI_MOUSE_OVER_BUTTON:
			SetEventHandler( MouseOverButton, handler );
			break;
		case IE_GUI_MOUSE_ENTER_BUTTON:
			SetEventHandler( MouseEnterButton, handler );
			break;
		case IE_GUI_MOUSE_LEAVE_BUTTON:
			SetEventHandler( MouseLeaveButton, handler );
			break;
		case IE_GUI_BUTTON_ON_SHIFT_PRESS:
			SetEventHandler( ButtonOnShiftPress, handler );
			break;
		case IE_GUI_BUTTON_ON_RIGHT_PRESS:
			SetEventHandler( ButtonOnRightPress, handler );
			break;
		case IE_GUI_BUTTON_ON_DRAG_DROP:
			SetEventHandler( ButtonOnDragDrop, handler );
			break;
		case IE_GUI_BUTTON_ON_DRAG:
			SetEventHandler( ButtonOnDrag, handler );
			break;
	default:
		return Control::SetEvent( eventType, handler );
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
		ToggleState = false;
	}		//other buttons, no value
	if (ToggleState) {
		State = IE_GUI_BUTTON_SELECTED;
	} else {
		State = IE_GUI_BUTTON_UNPRESSED;
	}
	Changed = true;
}
/** Sets the Picture */
void Button::SetPicture(Sprite2D* newpic)
{
	if (Picture) {
		core->GetVideoDriver()->FreeSprite( Picture );
	}
	if (newpic) {
		newpic->XPos = 0;
		newpic->YPos = 0;
	}
	Picture = newpic;
	Changed = true;
	Flags |= IE_GUI_BUTTON_PICTURE;
	( ( Window * ) Owner )->Invalidate();
}

/** Sets the secondary Picture (low half of paperdolls) */
void Button::SetPicture2(Sprite2D* newpic)
{
	if (Picture2) {
		core->GetVideoDriver()->FreeSprite( Picture2 );
	}
	if (newpic) {
	// XPos contains a special adjustment, don't erase it!
	// Yeah, i know this is the ugliest hack (hacking another hack)
	//	newpic->XPos = 0;
		newpic->YPos = 0;
	}
	Picture2 = newpic;
	Changed = true;
	( ( Window * ) Owner )->Invalidate();
}

bool Button::IsPixelTransparent(unsigned short x, unsigned short y)
{
	// some buttons have hollow Image frame filled w/ Picture
	// some buttons in BG2 are text only (if BAM == 'GUICTRL')
	if (Picture || ! Unpressed) return false;
	return core->GetVideoDriver()->IsSpritePixelTransparent(Unpressed, x, y);
}

// Set palette used for drawing button label in normal state
void Button::SetTextColor(Color fore, Color back)
{

	if (normal_palette)
		core->GetVideoDriver()->FreePalette( normal_palette );

	normal_palette = core->GetVideoDriver()->CreatePalette( fore, back );

	Changed = true;
}

