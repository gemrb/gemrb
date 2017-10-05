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

#include "Game.h"
#include "GameData.h"
#include "Palette.h"

namespace GemRB {

Button::Button(Region& frame)
	: Control(frame),
	buttonImages()
{
	ControlType = IE_GUI_BUTTON;
	State = IE_GUI_BUTTON_UNPRESSED;
	hotKey = 0;

	hasText = false;
	font = core->GetButtonFont();
	normal_palette = NULL;
	disabled_palette = font->GetPalette()->Copy();
	for (int i = 0; i < 256; i++) {
		disabled_palette->col[i].r = ( disabled_palette->col[i].r * 2 ) / 3;
		disabled_palette->col[i].g = ( disabled_palette->col[i].g * 2 ) / 3;
		disabled_palette->col[i].b = ( disabled_palette->col[i].b * 2 ) / 3;
	}
	SetFlags(IE_GUI_BUTTON_NORMAL, OP_OR);
	ToggleState = false;
	pulseBorder = false;
	Picture = NULL;
	Clipping = 1.0;

	memset( borders, 0, sizeof( borders ));
	starttime = 0;
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
		flags &= IE_GUI_BUTTON_NO_IMAGE;
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
	if (!( flags & IE_GUI_BUTTON_NO_IMAGE )) {
		Sprite2D* Image = NULL;

		switch (State) {
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
			default:
				Image = buttonImages[BUTTON_IMAGE_UNPRESSED];
				break;
		}
		if (Image) {
			// FIXME: maybe it's useless...
			int xOffs = ( frame.w / 2 ) - ( Image->Width / 2 );
			int yOffs = ( frame.h / 2 ) - ( Image->Height / 2 );

			video->BlitSprite( Image, rgn.x + xOffs, rgn.y + yOffs );
		}
	}

	if (State == IE_GUI_BUTTON_PRESSED) {
		//shift the writing/border a bit
		rgn.x += PushOffset.x;
		rgn.y += PushOffset.y;
	}

	// Button picture
	int picXPos = 0, picYPos = 0;
	if (Picture && (flags & IE_GUI_BUTTON_PICTURE) ) {
		// Picture is drawn centered
		picXPos = ( rgn.w / 2 ) - ( Picture->Width / 2 ) + rgn.x;
		picYPos = ( rgn.h / 2 ) - ( Picture->Height / 2 ) + rgn.y;
		if (flags & IE_GUI_BUTTON_HORIZONTAL) {
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

			video->BlitSprite( Picture, picXPos, picYPos, &rb );

			// TODO: Add an option to add BLIT_GREY to the flags
			video->BlitGameSprite( Picture, picXPos, picYPos, BLIT_TINTED, SourceRGB, 0, 0, &ro);

			// do NOT uncomment this, you can't change Changed or invalidate things from
			// the middle of Window::DrawWindow() -- it needs moving to somewhere else
			// ^ We can now... should this be uncommented then?
			//CloseUpColor();
		}
		else {
			Region r( picXPos, picYPos, (int)(Picture->Width * Clipping), Picture->Height );
			video->BlitSprite( Picture, picXPos + Picture->XPos, picYPos + Picture->YPos, &r );
		}
	}

	// Button animation
	if (AnimPicture) {
		int xOffs = ( frame.w / 2 ) - ( AnimPicture->Width / 2 );
		int yOffs = ( frame.h / 2 ) - ( AnimPicture->Height / 2 );
		Region r( rgn.x + xOffs, rgn.y + yOffs, int(AnimPicture->Width * Clipping), AnimPicture->Height );

		if (flags & IE_GUI_BUTTON_CENTER_PICTURES) {
			video->BlitSprite( AnimPicture, rgn.x + xOffs + AnimPicture->XPos, rgn.y + yOffs + AnimPicture->YPos, &r );
		} else {
			video->BlitSprite( AnimPicture, rgn.x + xOffs, rgn.y + yOffs, &r );
		}
	}

	// Composite pictures (paperdolls/description icons)
	if (!PictureList.empty() && (flags & IE_GUI_BUTTON_PICTURE) ) {
		std::list<Sprite2D*>::iterator iter = PictureList.begin();
		int xOffs = 0, yOffs = 0;
		if (flags & IE_GUI_BUTTON_CENTER_PICTURES) {
			// Center the hotspots of all pictures
			xOffs = frame.w / 2;
			yOffs = frame.h / 2;
		} else if (flags & IE_GUI_BUTTON_BG1_PAPERDOLL) {
			// Display as-is
			xOffs = 0;
			yOffs = 0;
		} else {
			// Center the first picture, and align the rest to that
			xOffs = frame.w / 2 - (*iter)->Width/2 + (*iter)->XPos;
			yOffs = frame.h / 2 - (*iter)->Height/2 + (*iter)->YPos;
		}

		for (; iter != PictureList.end(); ++iter) {
			video->BlitSprite( *iter, rgn.x + xOffs, rgn.y + yOffs );
		}
	}

	// Button label
	if (hasText && ! ( flags & IE_GUI_BUTTON_NO_TEXT )) {
		Palette* ppoi = normal_palette;
		int align = 0;

		if (State == IE_GUI_BUTTON_DISABLED)
			ppoi = disabled_palette;
		// FIXME: hopefully there's no button which sinks when selected
		//   AND has text label
		//else if (State == IE_GUI_BUTTON_PRESSED || State == IE_GUI_BUTTON_SELECTED) {

		if (flags & IE_GUI_BUTTON_ALIGN_LEFT)
			align |= IE_FONT_ALIGN_LEFT;
		else if (flags & IE_GUI_BUTTON_ALIGN_RIGHT)
			align |= IE_FONT_ALIGN_RIGHT;
		else
			align |= IE_FONT_ALIGN_CENTER;

		if (flags & IE_GUI_BUTTON_ALIGN_TOP)
			align |= IE_FONT_ALIGN_TOP;
		else if (flags & IE_GUI_BUTTON_ALIGN_BOTTOM)
			align |= IE_FONT_ALIGN_BOTTOM;
		else
			align |= IE_FONT_ALIGN_MIDDLE;

		if (! (flags & IE_GUI_BUTTON_MULTILINE)) {
			align |= IE_FONT_SINGLE_LINE;
		}

		Region r = rgn;
		if (Picture && (flags & IE_GUI_BUTTON_PORTRAIT) == IE_GUI_BUTTON_PORTRAIT) {
			// constrain the label (status icons) to the picture bounds
			// FIXME: we have to do +1 because the images are 1 px too small to fit 3 icons...
			r = Region(picXPos, picYPos, Picture->Width + 1, Picture->Height);
		} else if ((IE_GUI_BUTTON_ALIGN_LEFT | IE_GUI_BUTTON_ALIGN_RIGHT |
				   IE_GUI_BUTTON_ALIGN_TOP   | IE_GUI_BUTTON_ALIGN_BOTTOM |
					IE_GUI_BUTTON_MULTILINE) & flags) {
			// FIXME: I'm unsure when exactly this adjustment applies...
			r = Region( rgn.x + 5, rgn.y + 5, rgn.w - 10, rgn.h - 10);
		}

		font->Print( r, Text, ppoi, (ieByte) align );
	}

	if (! (flags&IE_GUI_BUTTON_NO_IMAGE)) {
		for (int i = 0; i < MAX_NUM_BORDERS; i++) {
			ButtonBorder *fr = &borders[i];
			if (! fr->enabled) continue;

			const Region& frRect = fr->rect;
			Region r = Region( rgn.Origin() + frRect.Origin(), frRect.Dimensions() );
			if (pulseBorder) {
				Color mix;
				unsigned long step = GetTickCount();
				step = tp_steps[(step >> 7) & 7] * 2;

				mix.a = ColorWhite.a;
				mix.r = (ColorWhite.r * step + fr->color.r * (8-step))/8;
				mix.g = (ColorWhite.g * step + fr->color.g * (8-step))/8;
				mix.b = (ColorWhite.b * step + fr->color.b * (8-step))/8;

				video->DrawRect( r, mix, fr->filled );
			} else {
				video->DrawRect( r, fr->color, fr->filled );
			}
		}
	}
	
	if (IsDisabled()) {
		static const Color c(50, 30, 10, 120);
		video->DrawRect(rgn, c, true);
	}
}
/** Sets the Button State */
void Button::SetState(unsigned char state)
{
	if (state > IE_GUI_BUTTON_LOCKED_PRESSED) {// If wrong value inserted
		return;
	}

	if (State == state) {
		return; // avoid redraw
	}

	// FIXME: we should properly consolidate IE_GUI_BUTTON_DISABLED with the view Disabled flag
	SetDisabled(state == IE_GUI_BUTTON_DISABLED);

	if (State != state) {
		MarkDirty();
		State = state;
	}
}

bool Button::IsAnimated() const
{
	return (pulseBorder) ? true : Control::IsAnimated();
}

void Button::SetBorder(int index, const Region& rgn, const Color &color, bool enabled, bool filled)
{
	if (index >= MAX_NUM_BORDERS)
		return;

	ButtonBorder fr = { rgn, color, filled, enabled };
	borders[index] = fr;

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

String Button::TooltipText() const
{
	ieDword showHotkeys = 0;
	core->GetDictionary()->Lookup("Hotkeys On Tooltips", showHotkeys);

	if (showHotkeys && hotKey) {
		String s;
		switch (hotKey) {
			// FIXME: these arent localized...
			case GEM_ESCAPE:
				s += L"Esc";
				break;
			case GEM_RETURN:
				s += L"Ret";
				break;
			default:
				// TODO: check if there are more possible keys
				if (hotKey >= GEM_FUNCTIONX(1) && hotKey <= GEM_FUNCTIONX(16)) {
					s.push_back(L'F');
					int offset = hotKey - GEM_FUNCTIONX(0);
					if (hotKey > GEM_FUNCTIONX(9)) {
						s.push_back(L'1');
						offset -= 10;
					}
					s.push_back(L'0' + offset);
				} else {
					s.push_back(hotKey);
				}
				break;
		}
		s += L": ";
		return s += ((tooltip.length()) ? tooltip : QueryText());
	}
	return Control::TooltipText();
}

Sprite2D* Button::Cursor() const
{
	if ((flags & IE_GUI_BUTTON_PORTRAIT) == IE_GUI_BUTTON_PORTRAIT) {
		GameControl* gc = core->GetGameControl();
		if (gc) {
			return gc->GetTargetActionCursor();
		}
	}
	return Control::Cursor();
}

Holder<Button::DragOp> Button::DragOperation()
{
	if (Picture && (flags & IE_GUI_BUTTON_PORTRAIT) == IE_GUI_BUTTON_PORTRAIT) {
		EnableBorder(1, true);
		return Holder<Button::DragOp>(new PortraitDragOp(this));
	}
	return View::DragOperation();
}

bool Button::AcceptsDragOperation(const DragOp& dop) const
{
	if (dop.dragView != this && dynamic_cast<const PortraitDragOp*>(&dop)) {
		return (Picture && (flags & IE_GUI_BUTTON_PORTRAIT) == IE_GUI_BUTTON_PORTRAIT);
	}
	return View::AcceptsDragOperation(dop);
}

void Button::CompleteDragOperation(const DragOp& dop)
{
	if (dop.dragView == this) {
		// this was the dragged view
		EnableBorder(1, false);
	} else {
		// this was the receiver
		const PortraitDragOp* pdop = dynamic_cast<const PortraitDragOp*>(&dop);
		assert(pdop); // CompleteDragOperation shouldnt be called if we dont accept...
		core->GetGame()->SwapPCs(pdop->PC, ControlID + 1);
	}
}

/** Mouse Button Down */
void Button::OnMouseDown(const MouseEvent& me, unsigned short mod)
{
	ActionKey key(Action::DragDrop);
    if (core->GetDraggedItem() && !SupportsAction(key)) {
        return;
    }
    
	if (me.button == GEM_MB_ACTION) {
		// We use absolute screen position here, so drag_start
		//   remains valid even after window/control is moved
		drag_start = me.Pos();

		if (State == IE_GUI_BUTTON_LOCKED) {
			SetState( IE_GUI_BUTTON_LOCKED_PRESSED );
			return;
		}
		SetState( IE_GUI_BUTTON_PRESSED );
		if (flags & IE_GUI_BUTTON_SOUND) {
			core->PlaySound( DS_BUTTON_PRESSED );
		}
	}
	Control::OnMouseDown(me, mod);
}

/** Mouse Button Up */
void Button::OnMouseUp(const MouseEvent& me, unsigned short mod)
{
	bool drag = core->GetDraggedItem () != NULL;

    if (drag) {
        ActionKey key(Action::DragDrop);
        if (SupportsAction(key)) {
            PerformAction(key);
        } else {
            //if something was dropped, but it isn't handled here: it didn't happen
            return;
        }
    }

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

	if (flags & IE_GUI_BUTTON_CHECKBOX) {
		//checkbox
		ToggleState = !ToggleState;
		if (ToggleState)
			SetState( IE_GUI_BUTTON_SELECTED );
		else
			SetState( IE_GUI_BUTTON_UNPRESSED );
		if (VarName[0] != 0) {
			ieDword tmp = 0;
			core->GetDictionary()->Lookup( VarName, tmp );
			tmp ^= GetValue();
			core->GetDictionary()->SetAt( VarName, tmp );
			window->RedrawControls( VarName, tmp );
		}
	} else {
		if (flags & IE_GUI_BUTTON_RADIOBUTTON) {
			//radio button
			ToggleState = true;
			SetState( IE_GUI_BUTTON_SELECTED );
		}
		if (VarName[0] != 0) {
			ieDword val = GetValue();
			core->GetDictionary()->SetAt( VarName, val );
			window->RedrawControls( VarName, val );
		}
	}
    Control::OnMouseUp(me, mod);
}

void Button::OnMouseOver(const MouseEvent& me)
{
	if (State == IE_GUI_BUTTON_LOCKED) {
		return;
	}

	if ((flags & IE_GUI_BUTTON_DRAGGABLE) &&
			      (State == IE_GUI_BUTTON_PRESSED || State ==IE_GUI_BUTTON_LOCKED_PRESSED)) {

		Point sp = me.Pos() - drag_start;
		core->GetDictionary()->SetAt( "DragX", sp.x );
		core->GetDictionary()->SetAt( "DragY", sp.y );
		// We use absolute screen position here, so drag_start
		//   remains valid even after window/control is moved
		drag_start = drag_start + sp;
    } else {
        Control::OnMouseOver(me);
    }
}

void Button::OnMouseEnter(const MouseEvent& me, const DragOp* dop)
{
	if (IsFocused() && me.ButtonState(GEM_MB_ACTION)) {
		SetState( IE_GUI_BUTTON_PRESSED );
	}

	for (int i = 0; i < MAX_NUM_BORDERS; i++) {
		ButtonBorder *fr = &borders[i];
		if (fr->enabled) {
			pulseBorder = true;
			MarkDirty();
			break;
		}
	}

    Control::OnMouseEnter(me, dop);
}

void Button::OnMouseLeave(const MouseEvent& me, const DragOp* dop)
{
	if (State == IE_GUI_BUTTON_PRESSED && !(flags & IE_GUI_BUTTON_RADIOBUTTON) && !TracksMouseDown()) {
		SetState( IE_GUI_BUTTON_UNPRESSED );
	}
	if (pulseBorder) {
		pulseBorder = false;
		MarkDirty();
	}

    Control::OnMouseLeave(me, dop);
}

/** Sets the Text of the current control */
void Button::SetText(const String& string)
{
	Text = string;
	if (string.length()) {
		if (flags&IE_GUI_BUTTON_LOWERCASE)
			StringToLower( Text );
		else if (flags&IE_GUI_BUTTON_CAPS)
			StringToUpper( Text );
		hasText = true;
	} else {
		hasText = false;
	}
	MarkDirty();
}

/** Refresh a button from a given radio button group */
void Button::UpdateState(unsigned int Sum)
{
	if (flags & IE_GUI_BUTTON_RADIOBUTTON) {
		ToggleState = ( Sum == GetValue() );
	}   	//radio button, exact value
	else if (flags & IE_GUI_BUTTON_CHECKBOX) {
		ToggleState = !!( Sum & GetValue() );
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
	if (Picture) {
		// try fitting to width if rescaling is possible, otherwise we automatically crop
		unsigned int ratio = Picture->Width / frame.w;
		if (ratio > 1) {
			Sprite2D *img = core->GetVideoDriver()->SpriteScaleDown(Picture, ratio);
			Sprite2D::FreeSprite(Picture);
			Picture = img;
		}
		Picture->acquire();
		flags |= IE_GUI_BUTTON_PICTURE;
	} else {
		flags &= ~IE_GUI_BUTTON_PICTURE;
	}
	MarkDirty();
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
	flags |= IE_GUI_BUTTON_PICTURE;
}

bool Button::HitTest(const Point& p) const
{
	bool hit = Control::HitTest(p);
	if (hit) {
		// some buttons have hollow Image frame filled w/ Picture
		// some buttons in BG2 are text only (if BAM == 'GUICTRL')
		Sprite2D* Unpressed = buttonImages[BUTTON_IMAGE_UNPRESSED];
		if (Picture || PictureList.size() || !Unpressed) return true;
		
		int xOffs = ( frame.w / 2 ) - ( Unpressed->Width / 2 );
		int yOffs = ( frame.h / 2 ) - ( Unpressed->Height / 2 );
		hit = !Unpressed->IsPixelTransparent(p.x - xOffs, p.y - yOffs);
	}
	return hit;
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
	if ((Clipping>clip) || !(flags&IE_GUI_BUTTON_HORIZONTAL) ) {
		flags |= IE_GUI_BUTTON_HORIZONTAL;
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

bool Button::SetHotKey(KeyboardKey key)
{
	EventMgr::EventCallback* cb = new MethodCallback<Button, const Event&, bool>(this, &Button::HandleHotKey);
	if (window->RegisterHotKeyCallback(cb, key)) {
		hotKey = key;
		return true;
	}
	delete cb;
	return false;
}

bool Button::HandleHotKey(const Event& e)
{
	if (e.type == Event::KeyDown) {
		// only run once on keypress (or should it be KeyRelease?)
		// we could support both; key down = left mouse down, key up = left mouse up
		return PerformAction();
	}
	return false;
}

}
