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

#include "GUI/EventMgr.h"
#include "Video/Video.h"

namespace GemRB {

ScrollBar::ScrollBar(const Region& frame, const Holder<Sprite2D> images[IMAGE_COUNT])
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
	return frame.h - GetFrameHeight(IMAGE_SLIDER) - GetFrameHeight(IMAGE_DOWN_UNPRESSED) - GetFrameHeight(IMAGE_UP_UNPRESSED);
}

int ScrollBar::GetFrameHeight(int frame) const
{
	return Frames[frame]->Frame.h;
}

void ScrollBar::ScrollDelta(const Point& delta)
{
	Point p = Point(-delta.x, -delta.y);
	int xy = *((State & SLIDER_HORIZONTAL) ? &p.x : &p.y);
	if (xy == 0) return;

	if (p.y > 0) {
		// flip the reference point to the bottom of the slider
		p.y += GetFrameHeight(IMAGE_SLIDER);
	}
	ScrollTo(p + AxisPosFromValue());
}

// FIXME: horizontal broken
void ScrollBar::ScrollTo(const Point& p)
{
	int pxRange = SliderPxRange();
	double percent = Clamp<double>(p.y, 0, pxRange) / pxRange;
	const ValueRange& range = GetValueRange();

	value_t newPos = round(double((percent * (range.second - range.first)) + range.first));
	SetValue(newPos);
}

Point ScrollBar::AxisPosFromValue() const
{
	const ValueRange& range = GetValueRange();
	if (range.second <= range.first) return Point();

	Point p;
	int xy = round((SliderPxRange() / double(range.second - range.first)) * GetValue());
	if (State & SLIDER_HORIZONTAL) {
		p.x = xy;
	} else {
		p.y = xy;
	}
	return p;
}

void ScrollBar::ScrollBySteps(int steps)
{
	int val = GetValue() + (steps * StepIncrement);
	const ValueRange& range = GetValueRange();
	value_t clamped = Clamp<int>(val, range.first, range.second);
	SetValue(clamped);
}

void ScrollBar::ScrollUp()
{
	ScrollBySteps(-1);
}

void ScrollBar::ScrollDown()
{
	ScrollBySteps(1);
}

bool ScrollBar::IsOpaque() const
{
	bool opaque = Control::IsOpaque();
	if (!opaque) {
		opaque = Frames[IMAGE_TROUGH]->HasTransparency() == false;
	}
	return opaque;
}

/** Draws the ScrollBar control */
void ScrollBar::DrawSelf(const Region& drawFrame, const Region& /*clip*/)
{
	int upMy = GetFrameHeight(IMAGE_UP_UNPRESSED);
	int doMy = GetFrameHeight(IMAGE_DOWN_UNPRESSED);
	unsigned int domy = (frame.h - doMy);

	//draw the up button
	if ((State & UP_PRESS) != 0) {
		VideoDriver->BlitSprite(Frames[IMAGE_UP_PRESSED], drawFrame.origin);
	} else {
		VideoDriver->BlitSprite(Frames[IMAGE_UP_UNPRESSED], drawFrame.origin);
	}
	int maxy = drawFrame.y + drawFrame.h - GetFrameHeight(IMAGE_DOWN_UNPRESSED);
	int stepy = GetFrameHeight(IMAGE_TROUGH);
	// some "scrollbars" are sized to just show the up and down buttons
	// we must skip the trough (and slider) in those cases
	if (maxy > upMy + doMy) {
		// draw the trough
		if (stepy) {
			Region rgn(drawFrame.x, drawFrame.y + upMy, drawFrame.w, domy - upMy);
			for (int dy = drawFrame.y + upMy; dy < maxy; dy += stepy) {
				//TROUGH surely exists if it has a nonzero height
				Point p = Frames[IMAGE_TROUGH]->Frame.origin;
				p.x += ((frame.w - Frames[IMAGE_TROUGH]->Frame.w - 1) / 2) + drawFrame.x;
				p.y += dy;
				VideoDriver->BlitSprite(Frames[IMAGE_TROUGH], p, &rgn);
			}
		}
		// draw the slider
		int slx = ((frame.w - Frames[IMAGE_SLIDER]->Frame.w - 1) / 2);
		// FIXME: doesn't respect SLIDER_HORIZONTAL
		int sly = AxisPosFromValue().y;
		Point p = drawFrame.origin + Frames[IMAGE_SLIDER]->Frame.origin;
		p.x += slx;
		p.y += upMy + sly;
		VideoDriver->BlitSprite(Frames[IMAGE_SLIDER], p);
	}
	//draw the down button
	if ((State & DOWN_PRESS) != 0) {
		VideoDriver->BlitSprite(Frames[IMAGE_DOWN_PRESSED], Point(drawFrame.x, maxy));
	} else {
		VideoDriver->BlitSprite(Frames[IMAGE_DOWN_UNPRESSED], Point(drawFrame.x, maxy));
	}
}

/** Mouse Button Down */
bool ScrollBar::OnMouseDown(const MouseEvent& me, unsigned short /*Mod*/)
{
	// FIXME: this doesn't respect SLIDER_HORIZONTAL
	Point p = ConvertPointFromScreen(me.Pos());
	if (p.x < 0 || p.x > frame.w) {
		// don't allow the scrollbar to engage when the mouse is outside
		// this happens when a scrollbar is used as an event proxy for a windows
		return false;
	}
	if (p.y <= GetFrameHeight(IMAGE_UP_UNPRESSED)) {
		State |= UP_PRESS;
		ScrollUp();
		return true;
	}
	if (p.y >= frame.h - GetFrameHeight(IMAGE_DOWN_UNPRESSED)) {
		State |= DOWN_PRESS;
		ScrollDown();
		return true;
	}
	// if we made it this far we will jump the nib to y and "grab" it
	// this way we only need to click once to jump+scroll
	State |= SLIDER_GRAB;
	ieWord sliderPos = AxisPosFromValue().y + GetFrameHeight(IMAGE_UP_UNPRESSED);
	if (p.y >= sliderPos && p.y <= sliderPos + GetFrameHeight(IMAGE_SLIDER)) {
		// FIXME: hack. we shouldnt mess with the sprite position should we?
		// scrollbars may share images, so no, we shouldn't do this. need to fix or odd behavior will occur when 2 scrollbars are visible.
		Frames[IMAGE_SLIDER]->Frame.y = p.y - sliderPos - GetFrameHeight(IMAGE_SLIDER) / 2;
		return true;
	}
	// FIXME: assumes IMAGE_UP_UNPRESSED.h == IMAGE_DOWN_UNPRESSED.h
	int offset = GetFrameHeight(IMAGE_UP_UNPRESSED) + GetFrameHeight(IMAGE_SLIDER) / 2;
	if (State & SLIDER_HORIZONTAL) {
		p.x -= offset;
	} else {
		p.y -= offset;
	}

	ScrollTo(p);
	return true;
}

/** Mouse Button Up */
bool ScrollBar::OnMouseUp(const MouseEvent& /*me*/, unsigned short /*Mod*/)
{
	MarkDirty();
	State = 0;
	Frames[IMAGE_SLIDER]->Frame.y = 0; //this is to clear any offset incurred by grabbing the slider
	return true;
}

/** Mousewheel scroll */
bool ScrollBar::OnMouseWheelScroll(const Point& delta)
{
	const ValueRange& range = GetValueRange();
	if (State == 0 && range.second > 0) { // don't allow mousewheel to do anything if the slider is being interacted with already.
		// FIXME: implement horizontal check
		// IsPerPixelScrollable() is false for ScrollBar so `delta` is in steps
		ScrollBySteps(-delta.y);
		return true;
	}
	return Control::OnMouseWheelScroll(delta);
}

/** Mouse Drag Event */
bool ScrollBar::OnMouseDrag(const MouseEvent& me)
{
	if (State & SLIDER_GRAB) {
		Point p = ConvertPointFromScreen(me.Pos());
		Point slideroffset(Frames[IMAGE_SLIDER]->Frame.x, Frames[IMAGE_SLIDER]->Frame.y);
		// FIXME: assumes IMAGE_UP_UNPRESSED.h == IMAGE_DOWN_UNPRESSED.h
		int offset = GetFrameHeight(IMAGE_UP_UNPRESSED) + GetFrameHeight(IMAGE_SLIDER) / 2;
		if (State & SLIDER_HORIZONTAL) {
			p.x -= offset;
		} else {
			p.y -= offset;
		}
		ScrollTo(p - slideroffset);
	}
	return true;
}

bool ScrollBar::OnKeyPress(const KeyboardEvent& key, unsigned short mod)
{
	if (State == 0) {
		switch (key.keycode) {
			// TODO: should probably only handle keys corresponding to scroll direction
			case GEM_UP:
				ScrollUp();
				return true;
			case GEM_DOWN:
				ScrollDown();
				return true;
			case GEM_LEFT:
				// TODO: implement horizontal scrollbars
				return true;
			case GEM_RIGHT:
				// TODO: implement horizontal scrollbars
				return true;
		}
	}

	return Control::OnKeyPress(key, mod);
}

}
