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
#include "Palette.h"

namespace GemRB {

Button::Button(Region& frame)
	: Control(frame),
	buttonImages()
{
	ControlType = IE_GUI_BUTTON;
	State = IE_GUI_BUTTON_UNPRESSED;

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
	memset(eventHandlers, 0, sizeof(eventHandlers));
	memset(&SourceRGB,0,sizeof(SourceRGB));
	memset(&DestRGB,0,sizeof(DestRGB));
	memset( borders, 0, sizeof( borders ));
	starttime = 0;
	Anchor.null();
	PushOffset = Point(2, 2);
}
Button::~Button()
{
	SetImage(BUTTON_IMAGE_NONE, NULL);
	Sprite2D::FreeSprite( Picture );
	ClearPictureList();

	gamedata->FreePalette( normal_palette);
	gamedata->FreePalette( disabled_palette);
}

/** Sets the 'type' Image of the Button to 'img'.
 see 'BUTTON_IMAGE_TYPE' */
void Button::SetImage(BUTTON_IMAGE_TYPE type, Sprite2D* img)
{
	if (type >= BUTTON_IMAGE_TYPE_COUNT) {
		Log(ERROR, "Button", "Trying to set a button image index out of range: %d", type);
		return;
	}

	if (type <= BUTTON_IMAGE_NONE) {
		for (int i=0; i < BUTTON_IMAGE_TYPE_COUNT; i++) {
			Sprite2D::FreeSprite(buttonImages[i]);
		}
		Flags &= IE_GUI_BUTTON_NO_IMAGE;
	} else {
		Sprite2D::FreeSprite(buttonImages[type]);
		buttonImages[type] = img;
		// FIXME: why do we not acquire the image here?!
		/*
		 currently IE_GUI_BUTTON_NO_IMAGE cannot be infered from the presence or lack of images
		 leaving this here commented out in case it may be useful in the future.

		if (img) {
			Flags &= ~IE_GUI_BUTTON_NO_IMAGE;
		} else {
			// check if we should set IE_GUI_BUTTON_NO_IMAGE

			int i=0;
			for (; i < BUTTON_IMAGE_TYPE_COUNT; i++) {
				if (buttonImages[i] != NULL) {
					break;
				}
			}
			if (i == BUTTON_IMAGE_TYPE_COUNT) {
				// we made it to the end of the list without breaking so we have no images
				Flags &= IE_GUI_BUTTON_NO_IMAGE;
			}
		}*/
	}
	MarkDirty();
}

/** make SourceRGB go closer to DestRGB */
void Button::CloseUpColor()
{
	if (!starttime) return;
	//using the realtime timer, because i don't want to
	//handle Game at this point
	unsigned long newtime;

	newtime = GetTickCount();
	if (newtime<starttime) {
		return;
	}
	MarkDirty();
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
void Button::DrawSelf(Region rgn, const Region& /*clip*/)
{
	Video * video = core->GetVideoDriver();

	// Button image
	if (!( Flags & IE_GUI_BUTTON_NO_IMAGE )) {
		Sprite2D* Image = NULL;

		switch (State) {
			case IE_GUI_BUTTON_UNPRESSED:
			case IE_GUI_BUTTON_LOCKED:
			case IE_GUI_BUTTON_LOCKED_PRESSED:
				Image = buttonImages[BUTTON_IMAGE_UNPRESSED];
				break;
			case IE_GUI_BUTTON_FAKEPRESSED:
			case IE_GUI_BUTTON_PRESSED:
				Image = buttonImages[BUTTON_IMAGE_PRESSED];
				break;
			case IE_GUI_BUTTON_SELECTED:
				Image = buttonImages[BUTTON_IMAGE_SELECTED];
				break;
			case IE_GUI_BUTTON_DISABLED:
			case IE_GUI_BUTTON_FAKEDISABLED:
				Image = buttonImages[BUTTON_IMAGE_DISABLED];
				break;
		}
		if (!Image) {
			Image = buttonImages[BUTTON_IMAGE_UNPRESSED];
		}
		if (Image) {
			// FIXME: maybe it's useless...
			int xOffs = ( frame.w / 2 ) - ( Image->Width / 2 );
			int yOffs = ( frame.h / 2 ) - ( Image->Height / 2 );

			video->BlitSprite( Image, rgn.x + xOffs, rgn.y + yOffs, true );
		}
	}

	if (State == IE_GUI_BUTTON_PRESSED) {
		//shift the writing/border a bit
		rgn.x += PushOffset.x;
		rgn.y += PushOffset.y;
	}

	// Button picture
	if (AnimPicture) {
		int xOffs = ( frame.w / 2 ) - ( AnimPicture->Width / 2 );
		int yOffs = ( frame.h / 2 ) - ( AnimPicture->Height / 2 );
		Region r( rgn.x + xOffs, rgn.y + yOffs, (int)(AnimPicture->Width * Clipping), AnimPicture->Height );

		if (Flags & IE_GUI_BUTTON_CENTER_PICTURES) {
			video->BlitSprite( AnimPicture, rgn.x + xOffs + AnimPicture->XPos, rgn.y + yOffs + AnimPicture->YPos, true, &r );
		} else {
			video->BlitSprite( AnimPicture, rgn.x + xOffs, rgn.y + yOffs, true, &r );
		}
	}

	// Button picture
	int picXPos = 0, picYPos = 0;
	if (Picture && (Flags & IE_GUI_BUTTON_PICTURE) ) {
		// Picture is drawn centered
		picXPos = ( rgn.w / 2 ) - ( Picture->Width / 2 ) + rgn.x;
		picYPos = ( rgn.h / 2 ) - ( Picture->Height / 2 ) + rgn.y;
		if (Flags & IE_GUI_BUTTON_HORIZONTAL) {
			picXPos += Picture->XPos;
			picYPos += Picture->YPos;

			// Clipping: 0 = overlay over full button, 1 = no overlay
			int overlayHeight = Picture->Height * (1.0 - Clipping);
			if (overlayHeight < 0)
				overlayHeight = 0;
			if (overlayHeight >= Picture->Height)
				overlayHeight = Picture->Height;
			int buttonHeight = Picture->Height - overlayHeight;

			Region rb = Region(picXPos, picYPos, Picture->Width, buttonHeight);
			Region ro = Region(picXPos, picYPos + buttonHeight, Picture->Width, overlayHeight);

			video->BlitSprite( Picture, picXPos, picYPos, true, &rb );

			// TODO: Add an option to add BLIT_GREY to the flags
			video->BlitGameSprite( Picture, picXPos, picYPos, BLIT_TINTED, SourceRGB, 0, 0, &ro, true);

			// do NOT uncomment this, you can't change Changed or invalidate things from
			// the middle of Window::DrawWindow() -- it needs moving to somewhere else
			// ^ We can now... should this be uncommented then?
			//CloseUpColor();
		}
		else {
			Region r( picXPos, picYPos, (int)(Picture->Width * Clipping), Picture->Height );
			video->BlitSprite( Picture, picXPos + Picture->XPos, picYPos + Picture->YPos, true, &r );
		}
	}

	// Composite pictures (paperdolls/description icons)
	if (!PictureList.empty() && (Flags & IE_GUI_BUTTON_PICTURE) ) {
		std::list<Sprite2D*>::iterator iter = PictureList.begin();
		int xOffs = 0, yOffs = 0;
		if (Flags & IE_GUI_BUTTON_CENTER_PICTURES) {
			// Center the hotspots of all pictures
			xOffs = frame.w / 2;
			yOffs = frame.h / 2;
		} else if (Flags & IE_GUI_BUTTON_BG1_PAPERDOLL) {
			// Display as-is
			xOffs = 0;
			yOffs = 0;
		} else {
			// Center the first picture, and align the rest to that
			xOffs = frame.w / 2 - (*iter)->Width/2 + (*iter)->XPos;
			yOffs = frame.h / 2 - (*iter)->Height/2 + (*iter)->YPos;
		}

		for (; iter != PictureList.end(); ++iter) {
			video->BlitSprite( *iter, rgn.x + xOffs, rgn.y + yOffs, true );
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

		Region r = rgn;
		if (Picture && (Flags & IE_GUI_BUTTON_PORTRAIT) == IE_GUI_BUTTON_PORTRAIT) {
			// constrain the label (status icons) to the picture bounds
			// FIXME: we have to do +1 because the images are 1 px too small to fit 3 icons...
			r = Region(picXPos, picYPos, Picture->Width + 1, Picture->Height);
		} else if ((IE_GUI_BUTTON_ALIGN_LEFT | IE_GUI_BUTTON_ALIGN_RIGHT |
				   IE_GUI_BUTTON_ALIGN_TOP   | IE_GUI_BUTTON_ALIGN_BOTTOM |
					IE_GUI_BUTTON_MULTILINE) & Flags) {
			// FIXME: I'm unsure when exactly this adjustment applies...
			r = Region( rgn.x + 5, rgn.y + 5, rgn.w - 10, rgn.h - 10);
		}

		font->Print( r, Text, ppoi, (ieByte) align );
	}

	if (! (Flags&IE_GUI_BUTTON_NO_IMAGE)) {
		for (int i = 0; i < MAX_NUM_BORDERS; i++) {
			ButtonBorder *fr = &borders[i];
			if (! fr->enabled) continue;

			Region r = Region( rgn.x + fr->dx1, rgn.y + fr->dy1, rgn.w - (fr->dx1 + fr->dx2 + 1), rgn.h - (fr->dy1 + fr->dy2 + 1) );
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
		MarkDirty();
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
	MarkDirty();
}

void Button::EnableBorder(int index, bool enabled)
{
	if (index >= MAX_NUM_BORDERS)
		return;

	if (borders[index].enabled != enabled) {
		borders[index].enabled = enabled;
		MarkDirty();
	}
}

void Button::SetFont(Font* newfont)
{
	font = newfont;
}

bool Button::WantsDragOperation()
{
	return (State != IE_GUI_BUTTON_DISABLED && eventHandlers[IE_GUI_MOUSE_LEAVE_BUTTON] !=0 && VarName[0] != 0);
}

/** Handling The default button (enter) */
bool Button::OnSpecialKeyPress(unsigned char Key)
{
	if (State != IE_GUI_BUTTON_DISABLED && State != IE_GUI_BUTTON_LOCKED) {
		if (Key == GEM_RETURN) {
			if (Flags & IE_GUI_BUTTON_DEFAULT ) {
				RunEventHandler( eventHandlers[IE_GUI_BUTTON_ON_PRESS] );
				return true;
			}
		}
		else if (Key == GEM_ESCAPE) {
			if (Flags & IE_GUI_BUTTON_CANCEL ) {
				RunEventHandler( eventHandlers[IE_GUI_BUTTON_ON_PRESS] );
				return true;
			}
		}
	}
	return Control::OnSpecialKeyPress(Key);
}

/** Mouse Button Down */
void Button::OnMouseDown(const Point& p, unsigned short Button, unsigned short Mod)
{
	if (State == IE_GUI_BUTTON_DISABLED) {
		return;
	}

	if (core->GetDraggedItem () && !eventHandlers[IE_GUI_BUTTON_ON_DRAG_DROP]) {
		View::OnMouseDown(p, Button, Mod);
		return;
	}

	//Button == 1 means Left Mouse Button
	switch(Button&GEM_MB_NORMAL) {
	case GEM_MB_ACTION:
		// We use absolute screen position here, so drag_start
		//   remains valid even after window/control is moved
		drag_start = ConvertPointToScreen(p);

		if (State == IE_GUI_BUTTON_LOCKED) {
			SetState( IE_GUI_BUTTON_LOCKED_PRESSED );
			return;
		}
		SetState( IE_GUI_BUTTON_PRESSED );
		if (Flags & IE_GUI_BUTTON_SOUND) {
			core->PlaySound( DS_BUTTON_PRESSED );
		}
		if ((Button & GEM_MB_DOUBLECLICK)) {
			RunEventHandler( eventHandlers[IE_GUI_BUTTON_ON_DOUBLE_PRESS] );
		}
		break;
	}
}

/** Mouse Button Up */
void Button::OnMouseUp(const Point& p, unsigned short Button, unsigned short Mod)
{
	if (State == IE_GUI_BUTTON_DISABLED) {
		return;
	}

	//what was just dropped?
	int dragtype = 0;
	if (core->GetDraggedItem ()) dragtype=1;
	if (core->GetDraggedPortrait ()) dragtype=2;

	//if something was dropped, but it isn't handled here: it didn't happen
	if (dragtype && !eventHandlers[IE_GUI_BUTTON_ON_DRAG_DROP])
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
		if (( p.x >= frame.w ) || ( p.y >= frame.h )) {
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
			RunEventHandler( eventHandlers[IE_GUI_BUTTON_ON_DRAG_DROP] );
			return;
		case 2:
			RunEventHandler( eventHandlers[IE_GUI_BUTTON_ON_DRAG_DROP_PORTRAIT] );
			return;
	}

	if ((Button&GEM_MB_NORMAL) == GEM_MB_ACTION) {
		if ((Mod & GEM_MOD_SHIFT) && eventHandlers[IE_GUI_BUTTON_ON_SHIFT_PRESS])
			RunEventHandler( eventHandlers[IE_GUI_BUTTON_ON_SHIFT_PRESS] );
		else
			RunEventHandler( eventHandlers[IE_GUI_BUTTON_ON_PRESS] );
	} else if (Button == GEM_MB_MENU) {
		RunEventHandler( eventHandlers[IE_GUI_BUTTON_ON_RIGHT_PRESS] );
	}
}

void Button::OnMouseOver(const Point& p)
{
	Owner->Cursor = IE_CURSOR_NORMAL;
	if (State == IE_GUI_BUTTON_DISABLED) {
		return;
	}

	if ( RunEventHandler( eventHandlers[IE_GUI_MOUSE_OVER_BUTTON] )<0) {
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
		Point sp = ConvertPointToScreen(p);
		core->GetDictionary()->SetAt( "DragX", sp.x );
		core->GetDictionary()->SetAt( "DragY", sp.y );
		drag_start = drag_start + sp;

		RunEventHandler( eventHandlers[IE_GUI_BUTTON_ON_DRAG] );
	}
}

void Button::OnMouseEnter(const Point&)
{
	if (State == IE_GUI_BUTTON_DISABLED) {
		return;
	}

	if (eventHandlers[IE_GUI_MOUSE_ENTER_BUTTON] !=0 && VarName[0] != 0) {
		core->GetDictionary()->SetAt( VarName, Value );
	}

	RunEventHandler( eventHandlers[IE_GUI_MOUSE_ENTER_BUTTON] );
}

void Button::OnMouseLeave(const Point&)
{
	if (State == IE_GUI_BUTTON_DISABLED) {
		return;
	}
	if (WantsDragOperation()) {
		core->GetDictionary()->SetAt( VarName, Value );
	}
	RunEventHandler( eventHandlers[IE_GUI_MOUSE_LEAVE_BUTTON] );
}

/** Sets the Text of the current control */
void Button::SetText(const String& string)
{
	Text = string;
	if (string.length()) {
		if (Flags&IE_GUI_BUTTON_LOWERCASE)
			StringToLower( Text );
		else if (Flags&IE_GUI_BUTTON_CAPS)
			StringToUpper( Text );
		hasText = true;
	} else {
		hasText = false;
	}
	MarkDirty();
}

/** Set Event Handler */
bool Button::SetEvent(int eventType, ControlEventHandler handler)
{
	if ( eventType >= 0 && eventType < IE_GUI_BUTTON_EVENTS_COUNT ) {
		eventHandlers[eventType] = handler;
		return true;
	}
	return false;
}

/** Refresh a button from a given radio button group */
void Button::UpdateState(const char* VariableName, unsigned int Sum)
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
	Sprite2D::FreeSprite( Picture );
	ClearPictureList();
	Picture = newpic;
	MarkDirty();
	Flags |= IE_GUI_BUTTON_PICTURE;
}

/** Clears the list of Pictures */
void Button::ClearPictureList()
{
	for (std::list<Sprite2D*>::iterator iter = PictureList.begin();
		 iter != PictureList.end(); ++iter)
		Sprite2D::FreeSprite( *iter );
	PictureList.clear();
	MarkDirty();
}

/** Add picture to the end of the list of Pictures */
void Button::StackPicture(Sprite2D* Picture)
{
	PictureList.push_back(Picture);
	MarkDirty();
	Flags |= IE_GUI_BUTTON_PICTURE;
}

bool Button::EventHit(const Point& p) const
{
	// some buttons have hollow Image frame filled w/ Picture
	// some buttons in BG2 are text only (if BAM == 'GUICTRL')
	Sprite2D* Unpressed = buttonImages[BUTTON_IMAGE_UNPRESSED];
	if (Picture || PictureList.size() || !Unpressed) return true;

	int xOffs = ( frame.w / 2 ) - ( Unpressed->Width / 2 );
	int yOffs = ( frame.h / 2 ) - ( Unpressed->Height / 2 );
	return !Unpressed->IsPixelTransparent(p.x - xOffs, p.y - yOffs);
}

// Set palette used for drawing button label in normal state
void Button::SetTextColor(const Color &fore, const Color &back)
{
	gamedata->FreePalette( normal_palette );
	normal_palette = new Palette( fore, back );
	MarkDirty();
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
	MarkDirty();
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
