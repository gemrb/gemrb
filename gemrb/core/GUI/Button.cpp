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

#include "defsounds.h"
#include "ie_cursors.h"

#include "Game.h"
#include "GameData.h"
#include "Logging/Logging.h"
#include "Palette.h"

#include <utility>

#define IS_PORTRAIT (Picture && ((flags&IE_GUI_BUTTON_PORTRAIT) == IE_GUI_BUTTON_PORTRAIT))

namespace GemRB {

Button::Button(const Region& frame)
	: Control(frame)
{
	ControlType = IE_GUI_BUTTON;
	HotKeyCallback = METHOD_CALLBACK(&Button::HandleHotKey, this);

	SetFont(core->GetButtonFont());
	SetFlags(IE_GUI_BUTTON_NORMAL, BitOp::OR);
}

Button::~Button()
{
	delete animation;
	SetImage(BUTTON_IMAGE_NONE, NULL);
	ClearPictureList();

	if (hotKey.global) {
		UnregisterHotKey();
	}
}

void Button::UnregisterHotKey()
{
	if (hotKey) {
		if (hotKey.global) {
			EventMgr::UnRegisterHotKeyCallback(HotKeyCallback, hotKey.key, hotKey.mod);
		} else {
			window->UnRegisterHotKeyCallback(HotKeyCallback, hotKey.key);
		}
	}
}

/** Sets the 'type' Image of the Button to 'img'.
 see 'BUTTON_IMAGE_TYPE' */
void Button::SetImage(BUTTON_IMAGE_TYPE type, Holder<Sprite2D> img)
{
	if (type >= BUTTON_IMAGE_TYPE_COUNT) {
		Log(ERROR, "Button", "Trying to set a button image index out of range: {}", type);
		return;
	}

	if (type <= BUTTON_IMAGE_NONE) {
		for (auto& buttonImage : buttonImages) {
			buttonImage = nullptr;
		}
		flags &= IE_GUI_BUTTON_NO_IMAGE;
	} else {
		buttonImages[type] = std::move(img);
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

void Button::WillDraw(const Region& /*drawFrame*/, const Region& /*clip*/)
{
	if (animation && animation->HasEnded()) {
		SetAnimation(nullptr);
	}
}

void Button::DidDraw(const Region& /*drawFrame*/, const Region& /*clip*/)
{
	tick_t time = GetMilliseconds();
	if (overlayAnim) {
		overlayAnim.Next(time);
	}
	if (animation) {
		animation->Next(time);
	}
}

/** Draws the Control on the Output Display */
void Button::DrawSelf(const Region& rgn, const Region& /*clip*/)
{
	Video * video = core->GetVideoDriver();

	// Button image
	if (!( flags & IE_GUI_BUTTON_NO_IMAGE )) {
		Holder<Sprite2D> Image;

		switch (ButtonState) {
			case FAKEPRESSED:
			case PRESSED:
				Image = buttonImages[BUTTON_IMAGE_PRESSED];
				break;
			case SELECTED:
				Image = buttonImages[BUTTON_IMAGE_SELECTED];
				break;
			case DISABLED:
			case FAKEDISABLED:
				Image = buttonImages[BUTTON_IMAGE_DISABLED];
				break;
			default:
				Image = buttonImages[BUTTON_IMAGE_UNPRESSED];
				break;
		}
		if (Image) {
			// FIXME: maybe it's useless...
			Point offset((frame.w / 2) - (Image->Frame.w / 2), (frame.h / 2) - (Image->Frame.h / 2));
			video->BlitSprite(Image, rgn.origin + offset);
		}
	}

	if (ButtonState == PRESSED) {
		//shift the writing/border a bit
		rgn.x += PushOffset.x;
		rgn.y += PushOffset.y;
	}

	// Button picture
	Point picPos;
	if (Picture && (flags & IE_GUI_BUTTON_PICTURE) ) {
		// Picture is drawn centered
		picPos.x = (rgn.w / 2) - (Picture->Frame.w / 2) + rgn.x;
		picPos.y = (rgn.h / 2) - (Picture->Frame.h / 2) + rgn.y;
		if (flags & IE_GUI_BUTTON_HORIZONTAL) {
			picPos += Picture->Frame.origin;

			// Clipping: 0 = overlay over full button, 1 = no overlay
			int overlayHeight = Picture->Frame.h * (1.0 - Clipping);
			if (overlayHeight < 0)
				overlayHeight = 0;
			if (overlayHeight >= Picture->Frame.h)
				overlayHeight = Picture->Frame.h;
			int buttonHeight = Picture->Frame.h - overlayHeight;

			if (overlayHeight) {
				// TODO: Add an option to add BlitFlags::GREY to the flags
				const Color& col = overlayAnim.Current();
				video->BlitGameSprite(Picture, picPos, BlitFlags::COLOR_MOD, col);
			}

			Region rb = Region(picPos.x, picPos.y, Picture->Frame.w, buttonHeight);
			video->BlitSprite( Picture, rb.origin, &rb );
		} else {
			Region r(picPos.x, picPos.y, (Picture->Frame.w * Clipping), Picture->Frame.h);
			video->BlitSprite(Picture, Picture->Frame.origin + r.origin, &r);
		}
	}

	// Button animation
	if (animation && animation->Current()) {
		auto AnimPicture = animation->Current();
		int xOffs = ( frame.w / 2 ) - ( AnimPicture->Frame.w / 2 );
		int yOffs = ( frame.h / 2 ) - ( AnimPicture->Frame.h / 2 );
		Region r( rgn.x + xOffs, rgn.y + yOffs, int(AnimPicture->Frame.w * Clipping), AnimPicture->Frame.h );

		if (flags & IE_GUI_BUTTON_CENTER_PICTURES) {
			video->BlitSprite( AnimPicture, r.origin + AnimPicture->Frame.origin, &r );
		} else {
			video->BlitSprite( AnimPicture, r.origin, &r );
		}
	}

	// Composite pictures (paperdolls/description icons)
	if (!PictureList.empty() && (flags & IE_GUI_BUTTON_PICTURE) ) {
		auto iter = PictureList.begin();
		Point offset;
		if (flags & IE_GUI_BUTTON_CENTER_PICTURES) {
			// Center the hotspots of all pictures
			offset.x = frame.w / 2;
			offset.y = frame.h / 2;
		} else if (flags & IE_GUI_BUTTON_BG1_PAPERDOLL) {
			// Display as-is
		} else {
			// Center the first picture, and align the rest to that
			offset.x = frame.w / 2 - (*iter)->Frame.w/2 + (*iter)->Frame.x;
			offset.y = frame.h / 2 - (*iter)->Frame.h/2 + (*iter)->Frame.y;
		}

		BlitFlags blitFlags = BlitFlags::NONE;
		if (flags & IE_GUI_BUTTON_HORIZONTAL) {
			blitFlags = BlitFlags::HALFTRANS;
		}
		for (; iter != PictureList.end(); ++iter) {
			video->BlitSprite(*iter, rgn.origin + offset, nullptr, blitFlags);
		}
	}

	// Button label
	if (hasText && ! ( flags & IE_GUI_BUTTON_NO_TEXT )) {
		// FIXME: hopefully there's no button which sinks when selected
		//   AND has text label
		//else if (State == IE_GUI_BUTTON_PRESSED || State == IE_GUI_BUTTON_SELECTED) {

		ieByte align = 0;
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

		Region r = rgn;
		if (IS_PORTRAIT) {
			// constrain the label (status icons) to the picture bounds
			// FIXME: we have to do +1 because the images are 1 px too small to fit 3 icons...
			r = Region(picPos.x, picPos.y, Picture->Frame.w + 1, Picture->Frame.h);
		} else if (flags&IE_GUI_BUTTON_ANCHOR) {
			r.x += Anchor.x;
			r.y += Anchor.y;
			r.w -= Anchor.x;
			r.h -= Anchor.y;
		} else {
			Font::StringSizeMetrics metrics {r.size, 0, 0, false};
			font->StringSize(Text, &metrics);

			if (metrics.numLines == 1 && (IE_GUI_BUTTON_ALIGNMENT_FLAGS & flags)) {
				// FIXME: I'm unsure when exactly this adjustment applies...
				// we do know that if a button is multiline it should not have margins
				// I'm actually wondering if we need this at all anymore
				// I suspect its origins predate the fixing of font baseline alignment
				r.ExpandAllSides(-5);
			}
		}
		
		Color c = textColor;
		if (ButtonState == DISABLED || IsDisabled()) {
			c.r *= 0.66;
			c.g *= 0.66;
			c.b *= 0.66;
		}

		Font::PrintColors colors {c, ColorBlack};
		font->Print(r, Text, align, colors);
	}

	if (! (flags&IE_GUI_BUTTON_NO_IMAGE)) {
		for (const auto& border : borders) {
			const ButtonBorder *fr = &border;
			if (! fr->enabled) continue;

			const Region& frRect = fr->rect;
			Region r = Region( rgn.origin + frRect.origin, frRect.size );
			if (pulseBorder && !fr->filled) {
				Color mix = GlobalColorCycle.Blend(ColorWhite, fr->color);
				video->DrawRect( r, mix, fr->filled, BlitFlags::BLENDED );
			} else {
				video->DrawRect( r, fr->color, fr->filled, BlitFlags::BLENDED );
			}
		}
	}
}

void Button::FlagsChanged(unsigned int /*oldflags*/)
{
	if (animation) {
		animation->flags = SpriteAnimation::PLAY_NORMAL;
		if (flags & IE_GUI_BUTTON_PLAYRANDOM) {
			animation->flags |= SpriteAnimation::PLAY_RANDOM;
		}
		if (flags & IE_GUI_BUTTON_PLAYONCE) {
			animation->flags |= SpriteAnimation::PLAY_ONCE;
		}
		if (flags & IE_GUI_BUTTON_PLAYALWAYS) {
			animation->flags |= SpriteAnimation::PLAY_ALWAYS;
		}
	}
}

/** Sets the Button State */
void Button::SetState(State state)
{
	if (state > LOCKED_PRESSED) {// If wrong value inserted
		return;
	}

	// FIXME: we should properly consolidate IE_GUI_BUTTON_DISABLED with the view Disabled flag
	SetDisabled(state == DISABLED);

	if (ButtonState != state) {
		MarkDirty();
		ButtonState = state;
	}
}

bool Button::IsAnimated() const
{
	if (animation) {
		return true;
	}

	if (overlayAnim) {
		return true;
	}

	if (pulseBorder) {
		return true;
	}

	return false;
}

bool Button::IsOpaque() const
{
	if (animation) {
		// we are dirty every frame anyway
		// so no need to get fancy
		// just redraw everything
		return false;
	}

	bool opaque = View::IsOpaque();
	if (!opaque && Picture) {
		opaque = !(flags&IE_GUI_BUTTON_NO_IMAGE) && !Picture->HasTransparency();
	}
	
	return opaque;
}

void Button::SetBorder(int index, const Region& rgn, const Color &color, bool enabled, bool filled)
{
	if (index >= MAX_NUM_BORDERS)
		return;

	ButtonBorder fr = { rgn, color, filled, enabled };
	borders[index] = std::move(fr);

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
	if (IsDisabled() || flags & IE_GUI_BUTTON_NO_TOOLTIP) {
		return L"";
	}

	ieDword showHotkeys = core->GetVariable("Hotkeys On Tooltips", 0);
	if (showHotkeys && hotKey) {
		String s;
		switch (hotKey.key) {
			// FIXME: these arent localized...
			case GEM_ESCAPE:
				s += L"Esc";
				break;
			case GEM_RETURN:
				s += L"Enter";
				break;
			case GEM_PGUP:
				s += L"PgUp";
				break;
			case GEM_PGDOWN:
				s += L"PgDn";
				break;
			default:
				if (hotKey.key >= GEM_FUNCTIONX(1) && hotKey.key <= GEM_FUNCTIONX(16)) {
					s.push_back(L'F');
					int offset = hotKey.key - GEM_FUNCTIONX(0);
					if (hotKey.key > GEM_FUNCTIONX(9)) {
						s.push_back(L'1');
						offset -= 10;
					}
					s.push_back(L'0' + offset);
				} else {
					s.push_back(towupper(hotKey.key));
				}
				break;
		}
		String tt = ((tooltip.length()) ? tooltip : QueryText());
		tt = (tt.length()) ? s + L": " + tt : s;
		return tt;
	}
	return Control::TooltipText();
}

Holder<Sprite2D> Button::Cursor() const
{
	if (IS_PORTRAIT) {
		const GameControl* gc = core->GetGameControl();
		if (gc) {
			Holder<Sprite2D> cur = gc->GetTargetActionCursor();
			if (cur) return cur;
		}
	}
	return Control::Cursor();
}

bool Button::AcceptsDragOperation(const DragOp& dop) const
{
	// FIXME: this implementation is obviously not future proof
	// portrait buttons accept other portraits and dropped items
	if (IS_PORTRAIT) {
		return true;
	}
	
	return Control::AcceptsDragOperation(dop);
}

void Button::CompleteDragOperation(const DragOp& dop)
{
	if (dop.dragView == this) {
		// this was the dragged view
		EnableBorder(1, false);
	}
	
	Control::CompleteDragOperation(dop);
}

Holder<Sprite2D> Button::DragCursor() const
{
	if (IS_PORTRAIT) {
		// TODO: would it be an enhancement to actually use the portrait for the drag icon?
		return core->Cursors[IE_CURSOR_SWAP];
	} else if (Picture) {
		return Picture;
	}
	
	return Control::DragCursor();
}

/** Mouse Button Down */
bool Button::OnMouseDown(const MouseEvent& me, unsigned short mod)
{
	ActionKey key(Action::DragDropDest);
	if (core->GetDraggedItem() && !SupportsAction(key)) {
		return true;
	}
    
	if (me.button == GEM_MB_ACTION) {
		if (ButtonState == LOCKED) {
			SetState(LOCKED_PRESSED);
			return true;
		}
		SetState(PRESSED);
		if (flags & IE_GUI_BUTTON_SOUND) {
			core->PlaySound(DS_BUTTON_PRESSED, SFX_CHAN_GUI);
		}
	}
	return Control::OnMouseDown(me, mod);
}

/** Mouse Button Up */
bool Button::OnMouseUp(const MouseEvent& me, unsigned short mod)
{
	bool drag = core->GetDraggedItem () != NULL;

	if (drag && me.repeats == 1) {
		ActionKey key(Action::DragDropDest);
		if (SupportsAction(key)) {
			return PerformAction(key);
		} else {
			//if something was dropped, but it isn't handled here: it didn't happen
			return false;
		}
	}

	if (ButtonState == LOCKED_PRESSED) {
		SetState(LOCKED);
	} else {
		SetState(UNPRESSED);
	}

	DoToggle();
	return Control::OnMouseUp(me, mod);
}

bool Button::OnMouseOver(const MouseEvent& me)
{
	if (ButtonState == LOCKED) {
		return true;
	}

	return Control::OnMouseOver(me);
}

void Button::OnMouseEnter(const MouseEvent& me, const DragOp* dop)
{
	Control::OnMouseEnter(me, dop);

	if (IsFocused() && me.ButtonState(GEM_MB_ACTION)) {
		SetState(PRESSED);
	}

	for (const auto& border : borders) {
		const ButtonBorder *fr = &border;
		if (fr->enabled) {
			pulseBorder = !fr->filled;
			MarkDirty();
			break;
		}
	}
}

void Button::OnMouseLeave(const MouseEvent& me, const DragOp* dop)
{
	Control::OnMouseLeave(me, dop);

	if (ButtonState == PRESSED && (dop == nullptr || dop->dragView == this)) {
		SetState(UNPRESSED);
	}

	if (pulseBorder) {
		pulseBorder = false;
		MarkDirty();
	}
}

/** Sets the Text of the current control */
void Button::SetText(String string)
{
	Text = std::move(string);
	if (Text.length()) {
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

BitOp Button::GetDictOp() const noexcept
{
	if (flags & IE_GUI_BUTTON_CHECKBOX) {
		return BitOp::XOR;
	}
	
	return BitOp::SET;
}

/** Refresh a button from a given radio button group */
void Button::UpdateState(value_t Sum)
{
	if (IsDisabled()) {
		// FIXME: buttons should be able to both be disabled and reflect their state
		return;
	}
	
	State state = UNPRESSED;
	if (flags & IE_GUI_BUTTON_RADIOBUTTON) {
		//radio button, exact value
		state = Sum == GetValue() ? SELECTED : UNPRESSED;
	} else if (flags & IE_GUI_BUTTON_CHECKBOX) {
		//checkbox, bitvalue
		state = bool(Sum & GetValue()) ? SELECTED : UNPRESSED;
	} else {
		//other buttons, nothing to redraw
		return;
	}

	SetState(state);
}

void Button::DoToggle()
{
	UpdateDictValue();
}

/** Sets the Picture */
void Button::SetPicture(Holder<Sprite2D> newpic)
{
	ClearPictureList();
	Picture = std::move(newpic);
	if (Picture) {
		// try fitting to width if rescaling is possible, otherwise we automatically crop
		unsigned int ratio = CeilDiv(Picture->Frame.w, frame.w);
		if (ratio > 1) {
			Holder<Sprite2D> img = core->GetVideoDriver()->SpriteScaleDown(Picture, ratio);
			Picture = std::move(img);
		}
		flags |= IE_GUI_BUTTON_PICTURE;
	} else {
		flags &= ~IE_GUI_BUTTON_PICTURE;
	}
	MarkDirty();
}

void Button::SetAnimation(SpriteAnimation* anim)
{
	delete animation;
	animation = anim;
	FlagsChanged(flags); // sync animation flags
	
	MarkDirty();
}

/** Clears the list of Pictures */
void Button::ClearPictureList()
{
	PictureList.clear();
	MarkDirty();
}

/** Add picture to the end of the list of Pictures */
void Button::StackPicture(const Holder<Sprite2D>& newPicture)
{
	PictureList.push_back(newPicture);
	MarkDirty();
	flags |= IE_GUI_BUTTON_PICTURE;
}

bool Button::HitTest(const Point& p) const
{
	bool hit = Control::HitTest(p);
	if (hit) {
		// some buttons have hollow Image frame filled w/ Picture
		// some buttons in BG2 are text only (if BAM == 'GUICTRL')
		Holder<Sprite2D> Unpressed = buttonImages[BUTTON_IMAGE_UNPRESSED];
		if (Picture || !PictureList.empty() || !Unpressed) return true;

		Point off;
		off.x = (frame.w / 2) - (Unpressed->Frame.w / 2) + Unpressed->Frame.x;
		off.y = (frame.h / 2) - (Unpressed->Frame.h / 2) + Unpressed->Frame.y;
		hit = !Unpressed->IsPixelTransparent(p - off);
	}
	return hit;
}

// Set palette used for drawing button label in normal state
void Button::SetTextColor(const Color &color)
{
	textColor = color;
	MarkDirty();
}

void Button::SetHorizontalOverlay(double clip, const Color& src, const Color &dest)
{
	if ((Clipping>clip) || !(flags&IE_GUI_BUTTON_HORIZONTAL) ) {
		flags |= IE_GUI_BUTTON_HORIZONTAL;

		overlayAnim = ColorAnimation(src, dest, false);
	}
	Clipping = clip;
	MarkDirty();
}

void Button::SetAnchor(int x, int y)
{
	Anchor = Point(x,y);
}

void Button::SetPushOffset(int x, int y)
{
	PushOffset = Point(x,y);
}

bool Button::SetHotKey(KeyboardKey key, short mod, bool global)
{
	UnregisterHotKey();
	
	if (key == 0) {
		return true;
	}

	if (global) {
		if (EventMgr::RegisterHotKeyCallback(HotKeyCallback, key, mod)) {
			hotKey.key = key;
			hotKey.mod = mod;
			hotKey.global = true;
			return true;
		}
	} else if (window->RegisterHotKeyCallback(HotKeyCallback, key)) { // FIXME: this doesn't respect mod param
		hotKey.key = key;
		hotKey.mod = mod;
		return true;
	}
	return false;
}

bool Button::HandleHotKey(const Event& e)
{
	if (IsReceivingEvents() && e.type == Event::KeyDown) {
		// only run once on keypress (or should it be KeyRelease?)
		// we could support both; key down = left mouse down, key up = left mouse up
		DoToggle();
		return PerformAction();
	}
	return false;
}

}
