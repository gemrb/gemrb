/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "ScrollBar.h"

#include "win32def.h"

#include "Interface.h"
#include "Variables.h"
#include "Sprite2D.h"
#include "GUI/EventMgr.h"
#include "GUI/TextArea.h"
#include "GUI/Window.h"

namespace GemRB {

ScrollBar::ScrollBar(const Region& frame, Sprite2D* images[IMAGE_COUNT])
: Control(frame)
{
	Init(images);
}

ScrollBar::ScrollBar(const ScrollBar& sb)
: Control(sb.Frame())
{
	Init(sb.Frames);

	StepIncrement = sb.StepIncrement;
	SetValueRange(sb.GetValueRange());
}

ScrollBar& ScrollBar::operator=(const ScrollBar& sb)
{
	Init(sb.Frames);

	StepIncrement = sb.StepIncrement;
	SetValueRange(sb.GetValueRange());

	return *this;
}

int ScrollBar::SliderPxRange() const
{
	return frame.h
			- GetFrameHeight(IMAGE_SLIDER)
			- GetFrameHeight(IMAGE_DOWN_UNPRESSED)
			- GetFrameHeight(IMAGE_UP_UNPRESSED);
}

int ScrollBar::GetFrameHeight(int frame) const
{
	return Frames[frame]->Height;
}

void ScrollBar::SetPosForY(int y)
{
	y -= GetFrameHeight(IMAGE_UP_UNPRESSED) + GetFrameHeight(IMAGE_SLIDER) / 2;

	int pxRange = SliderPxRange();
	float percent = Clamp<float>(y, 0, pxRange) / pxRange;
	const ValueRange& range = GetValueRange();

	ieDword newPos = ((percent * (range.second - range.first)) + range.first);
	SetValue(newPos);
}

int ScrollBar::YPosFromValue() const
{
	const ValueRange& range = GetValueRange();
	if (range.second == range.first) return 0;
	return (SliderPxRange() / (range.second - range.first)) * GetValue();
}

/** Refreshes the ScrollBar according to a guiscript variable */
void ScrollBar::UpdateState(unsigned int Sum)
{
	SetValue( Sum );
}

void ScrollBar::ScrollBySteps(int steps)
{
	ieDword val = GetValue() + (steps * StepIncrement);
	SetValue(val); 
}

/** SDL < 1.3 Mousewheel support */
void ScrollBar::ScrollUp()
{
	ScrollBySteps(-1);
}

/** SDL < 1.3 Mousewheel support */
void ScrollBar::ScrollDown()
{
	ScrollBySteps(1);
}

bool ScrollBar::IsOpaque() const
{
	/*
	 IWD2 scrollbars have a transparent trough and we dont have a good way to know about such things.
	 returning false for now.
	return (Frames[IE_GUI_SCROLLBAR_TROUGH]->Width >= Frames[IE_GUI_SCROLLBAR_SLIDER]->Width);
	 */
	return false;
}

/** Draws the ScrollBar control */
void ScrollBar::DrawSelf(Region drawFrame, const Region& /*clip*/)
{
	Video *video=core->GetVideoDriver();
	int upMy = GetFrameHeight(IMAGE_UP_UNPRESSED);
	int doMy = GetFrameHeight(IMAGE_DOWN_UNPRESSED);
	unsigned int domy = (frame.h - doMy);

	//draw the up button
	if (( State & UP_PRESS ) != 0) {
		video->BlitSprite( Frames[IMAGE_UP_PRESSED].get(), drawFrame.x, drawFrame.y, &drawFrame );
	} else {
		video->BlitSprite( Frames[IMAGE_UP_UNPRESSED].get(), drawFrame.x, drawFrame.y, &drawFrame );
	}
	int maxy = drawFrame.y + drawFrame.h - GetFrameHeight(IMAGE_DOWN_UNPRESSED);
	int stepy = GetFrameHeight(IMAGE_TROUGH);
	// some "scrollbars" are sized to just show the up and down buttons
	// we must skip the trough (and slider) in those cases
	if (maxy >= stepy) {
		// draw the trough
		if (stepy) {
			Region rgn( drawFrame.x, drawFrame.y + upMy, drawFrame.w, domy - upMy);
			for (int dy = drawFrame.y + upMy; dy < maxy; dy += stepy) {
				//TROUGH surely exists if it has a nonzero height
				video->BlitSprite( Frames[IMAGE_TROUGH].get(),
					drawFrame.x + Frames[IMAGE_TROUGH]->XPos + ( ( frame.w - Frames[IMAGE_TROUGH]->Width - 1 ) / 2 ),
					dy + Frames[IMAGE_TROUGH]->YPos, &rgn );
			}
		}
		// draw the slider
		short slx = ((frame.w - Frames[IMAGE_SLIDER]->Width - 1) / 2 );
		int sly = YPosFromValue();
		video->BlitSprite( Frames[IMAGE_SLIDER].get(),
						  drawFrame.x + slx + Frames[IMAGE_SLIDER]->XPos,
						  drawFrame.y + Frames[IMAGE_SLIDER]->YPos + upMy + sly, &drawFrame );
	}
	//draw the down button
	if (( State & DOWN_PRESS ) != 0) {
		video->BlitSprite( Frames[IMAGE_DOWN_PRESSED].get(), drawFrame.x, maxy, &drawFrame );
	} else {
		video->BlitSprite( Frames[IMAGE_DOWN_UNPRESSED].get(), drawFrame.x, maxy, &drawFrame );
	}
}

/** Mouse Button Down */
void ScrollBar::OnMouseDown(const MouseEvent& me, unsigned short /*Mod*/)
{
	Point p = ConvertPointFromScreen(me.Pos());
	if (p.y <= GetFrameHeight(IMAGE_UP_UNPRESSED) ) {
		State |= UP_PRESS;
		ScrollUp();
		return;
	}
	if (p.y >= frame.h - GetFrameHeight(IMAGE_DOWN_UNPRESSED)) {
		State |= DOWN_PRESS;
		ScrollDown();
		return;
	}
	// if we made it this far we will jump the nib to y and "grab" it
	// this way we only need to click once to jump+scroll
	State |= SLIDER_GRAB;
	ieWord sliderPos = YPosFromValue() + GetFrameHeight(IMAGE_UP_UNPRESSED);
	if (p.y >= sliderPos && p.y <= sliderPos + GetFrameHeight(IMAGE_SLIDER)) {
		// FIXME: hack. we shouldnt mess with the sprite position should we?
		// scrollbars may share images, so no, we shouldn't do this. need to fix or odd behavior will occur when 2 scrollbars are visible.
		Frames[IMAGE_SLIDER]->YPos = p.y - sliderPos - GetFrameHeight(IMAGE_SLIDER)/2;
		return;
	}
	SetPosForY(p.y);
}

/** Mouse Button Up */
void ScrollBar::OnMouseUp(const MouseEvent& /*me*/, unsigned short /*Mod*/)
{
	MarkDirty();
	State = 0;
	Frames[IMAGE_SLIDER]->YPos = 0; //this is to clear any offset incurred by grabbing the slider
}

/** Mousewheel scroll */
void ScrollBar::OnMouseWheelScroll(const Point& delta)
{
	if ( State == 0 ){ // don't allow mousewheel to do anything if the slider is being interacted with already.
		short fauxY = YPosFromValue();
		if (fauxY + delta.y <= 0) fauxY = 0;
		else fauxY += delta.y;
		SetPosForY(fauxY);
	}
}

/** Mouse Drag Event */
void ScrollBar::OnMouseDrag(const MouseEvent& me)
{
	Point p = ConvertPointFromScreen(me.Pos());
	SetPosForY(p.y - Frames[IMAGE_SLIDER]->YPos);
}

}
