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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Button.cpp,v 1.71 2004/08/29 21:22:59 edheldil Exp $
 *
 */

#include "../../includes/win32def.h"
#include "Button.h"
#include "Interface.h"

#define SND_BUTTON_PRESSED 0
#define SND_BUTTON_RELEASE0 1
#define SND_BUTTON_RELEASE1 2

#define DEF_BUTTON1 4

static char ButtonSounds[3][9] = {
	"GAM_09\0\0", "GAM_03\0\0", "GAM_04\0\0"
};
static bool ButtonInited = false;

Button::Button(bool Clear)
{
	if (!ButtonInited) {
		ButtonInited = true;
		int soundtable = core->LoadTable( "defsound" );
		if (soundtable >= 0) {
			TableMgr* tm = core->GetTable( soundtable );
			if (tm) {
				for (int i = 0; i < 3; i++) {
					strncpy( ButtonSounds[i],
						tm->QueryField( i + DEF_BUTTON1, 0 ), 8 );
				}
				core->DelTable( soundtable );
			}
		}
	}
	Unpressed = Pressed = Selected = Disabled = NULL;
	this->Clear = Clear;
	State = IE_GUI_BUTTON_UNPRESSED;
	ButtonOnPress[0] = 0;
	ButtonOnShiftPress[0] = 0;
	ButtonOnRightPress[0] = 0;
	MouseEnterButton[0] = 0;
	MouseLeaveButton[0] = 0;
	MouseOverButton[0] = 0;
	Text = ( char * ) calloc( 64, sizeof(char) );
	hasText = false;
	font = core->GetButtonFont();
	palette = ( Color * ) malloc( 256 * sizeof( Color ) );
	memcpy( palette, font->GetPalette(), 256 * sizeof( Color ) );
	for (int i = 0; i < 256; i++) {
		palette[i].r = ( palette[i].r * 2 ) / 3;
		palette[i].g = ( palette[i].g * 2 ) / 3;
		palette[i].b = ( palette[i].b * 2 ) / 3;
	}
	Flags = IE_GUI_BUTTON_NORMAL;
	ToggleState = false;
	Picture = NULL;
	//MOS Draggable Stuff
	Dragging = false;
	ScrollX = 0;
	ScrollY = 0;
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
	if (Text) {
		free( Text );
	}
	video->FreePalette( palette );
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
	if (!Changed && !((Window*)Owner)->Floating) {
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

	// Button label
	if (hasText && ! ( Flags & IE_GUI_BUTTON_NO_TEXT )) {
		Color* ppoi = NULL;
		int align = 0;

		if (State == IE_GUI_BUTTON_DISABLED)
			ppoi = palette;
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
		else
			align |= IE_FONT_ALIGN_MIDDLE;

		font->Print( Region( x + XPos, y + YPos, Width, Height ),
			     ( unsigned char * ) Text, ppoi,
			     align | IE_FONT_SINGLE_LINE, true );
	}

	// Button picture
	if (Picture && ( Flags & IE_GUI_BUTTON_PICTURE )) {
		if (Flags & IE_GUI_BUTTON_DRAGGABLE) {
			Region r( XPos, YPos, Width, Height );
			core->GetVideoDriver()->BlitSprite( Picture, XPos + ScrollX,
										YPos + ScrollY, true, &r );
		} else {
			short xOffs = ( Width / 2 ) - ( Picture->Width / 2 );
			short yOffs = ( Height / 2 ) - ( Picture->Height / 2 );
			core->GetVideoDriver()->BlitSprite( Picture, x + XPos + xOffs,
										y + YPos + yOffs, true );
		}
	}

	for (int i = 0; i < MAX_NUM_BORDERS; i++) {
		ButtonBorder *fr = &borders[i];
		if (! fr->enabled) continue;

		Region r = Region( x + XPos + fr->dx1, y + YPos + fr->dy1, Width - (fr->dx1 + fr->dx2 + 1), Height - (fr->dy1 + fr->dy2 + 1) );
		core->GetVideoDriver()->DrawRect( r, fr->color, false );
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
void Button::SetBorder(int index, int dx1, int dy1, int dx2, int dy2, Color* color, bool enabled)
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

	//Button == 1 means Left Mouse Button
	if (Button == 1) {
		State = IE_GUI_BUTTON_PRESSED;
		Changed = true;
		if (Flags & IE_GUI_BUTTON_SOUND) {
			core->GetSoundMgr()->Play( ButtonSounds[SND_BUTTON_PRESSED] );
		}
		if (Flags & IE_GUI_BUTTON_DRAGGABLE) {
			Dragging = true;
			DragX = x;
			DragY = y;
		}
	}
}
/** Mouse Button Up */
void Button::OnMouseUp(unsigned short x, unsigned short y,
	unsigned char Button, unsigned short Mod)
{
	if (State == IE_GUI_BUTTON_DISABLED || State == IE_GUI_BUTTON_LOCKED) {
		return;
	}
	if (Flags & IE_GUI_BUTTON_DRAGGABLE) {
		Dragging = false;
	}
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
				core->GetDictionary()->SetAt( VarName, tmp ^ Value );
			}
		} else if (Flags & IE_GUI_BUTTON_RADIOBUTTON) {
			//radio button
			ToggleState = true;
			SetState( IE_GUI_BUTTON_SELECTED );
			if (VarName[0] != 0) {
				core->GetDictionary()->SetAt( VarName, Value );
				( ( Window * ) Owner )->RedrawControls( VarName, Value );
			}
		} else {
			if (VarName[0] != 0)
				core->GetDictionary()->SetAt( VarName, Value );
		}
/* divide says these sounds are tied to the window, not the released button
		if (Flags & IE_GUI_BUTTON_SOUND) {
			if (Flags & IE_GUI_BUTTON_ALT_SOUND)
				core->GetSoundMgr()->Play( ButtonSounds[SND_BUTTON_RELEASE1] );
			else
				core->GetSoundMgr()->Play( ButtonSounds[SND_BUTTON_RELEASE0] );
		}
*/
		if (Button == GEM_MB_ACTION) {
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

	if (Dragging) {
		ScrollX += ( x - DragX );
		ScrollY += ( y - DragY );
		if (ScrollX < ( -Picture->Width ))
			ScrollX = -Picture->Width;
		if (ScrollY < ( -Picture->Height ))
			ScrollY = -Picture->Height;
		if (ScrollX > 0)
			ScrollX = 0;
		if (ScrollY > 0)
			ScrollY = 0;
		Changed = true;
	}
	if (Flags & IE_GUI_BUTTON_DRAGGABLE) {
		( ( Window * ) Owner )->Cursor = 44;
	} else {
		( ( Window * ) Owner )->Cursor = 0;
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
		strncpy( Text, string, 63 );
		hasText = true;
		if (core->HasFeature( GF_UPPER_BUTTON_TEXT ))
			Text = strupr( Text );
	}
	Changed = true;
	return 0;
}

/** Set Event */
void Button::SetEvent(char* funcName, int eventType)
{
	switch (eventType) {
		case IE_GUI_BUTTON_ON_PRESS:
			strcpy( ButtonOnPress, funcName );
			break;
		case IE_GUI_MOUSE_OVER_BUTTON:
			strcpy( MouseOverButton, funcName );
			break;
		case IE_GUI_MOUSE_ENTER_BUTTON:
			strcpy( MouseEnterButton, funcName );
			break;
		case IE_GUI_MOUSE_LEAVE_BUTTON:
			strcpy( MouseLeaveButton, funcName );
			break;
		case IE_GUI_BUTTON_ON_SHIFT_PRESS:
			strcpy( ButtonOnShiftPress, funcName );
			break;
		case IE_GUI_BUTTON_ON_RIGHT_PRESS:
			strcpy( ButtonOnRightPress, funcName );
			break;
	}

	Changed = true;
}

/** Sets the Display Flags */
int Button::SetFlags(int arg_flags, int opcode)
{
	switch (opcode) {
		case OP_SET:
			Flags = arg_flags;  //set
			break;
		case OP_OR:
			Flags |= arg_flags; //turn on
			break;
		case OP_NAND:
			Flags &= ~arg_flags;//turn off
			break;
		default:
			return -1;
	}
	Changed = true;
	( ( Window * ) Owner )->Invalidate();
	return 0;
}

/** Redraws a button from a given radio button group */
void Button::RedrawButton(char* VariableName, unsigned int Sum)
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
	}						//other buttons, no value
	if (ToggleState) {
		State = IE_GUI_BUTTON_SELECTED;
	} else {
		State = IE_GUI_BUTTON_UNPRESSED;
	}
	Changed = true;
}
/** Sets the Picture */
void Button::SetPicture(Sprite2D* Picture)
{
	if (this->Picture) {
		core->GetVideoDriver()->FreeSprite( this->Picture );
	}
	this->Picture = Picture;
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
