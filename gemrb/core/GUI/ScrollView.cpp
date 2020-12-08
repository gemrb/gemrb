/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2016 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "ScrollView.h"

#include "GUI/ScrollBar.h"

#include "GUIScriptInterface.h"
#include "Interface.h"

namespace GemRB {

void ScrollView::ContentView::SizeChanged(const Size& oldsize)
{
	// considering it an error for a ContentView to exist outside of a ScrollView
	assert(superView);
	ScrollView* sv = static_cast<ScrollView*>(superView);

	// keep the same position
	int dx = frame.w - oldsize.w;
	int dy = frame.h - oldsize.h;

	ResizeToSubviews();
	sv->ScrollDelta(Point(dx, dy));
}
	
void ScrollView::ContentView::SubviewAdded(View* /*view*/, View* /*parent*/)
{
	ResizeToSubviews();
}
	
void ScrollView::ContentView::SubviewRemoved(View* /*view*/, View* /*parent*/)
{
	ResizeToSubviews();
}
	
void ScrollView::ContentView::ResizeToSubviews()
{
	assert(superView);
	// content view behaves as if RESIZE_WIDTH and RESIZE_HEIGHT are set always
	Size newSize = superView->Dimensions();

	if (!subViews.empty()) {
		std::list<View*>::iterator it = subViews.begin();
		Region bounds = (*it)->Frame();

		for (++it; it != subViews.end(); ++it) {
			Region r = (*it)->Frame();
			bounds.ExpandToRegion(r);
		}

		newSize.w = std::max(newSize.w, bounds.w);
		newSize.h = std::max(newSize.h, bounds.h);
	}
	assert(newSize.w >= superView->Frame().w && newSize.h >= superView->Frame().h);

	// set frame size directly to avoid infinite recursion
	// subviews were already resized, so no need to run the autoresize
	frame.w = newSize.w;
	frame.h = newSize.h;

	ScrollView* sv = static_cast<ScrollView*>(superView);
	sv->UpdateScrollbars();
}
	
void ScrollView::ContentView::WillDraw(const Region& /*drawFrame*/, const Region& clip)
{
	ScrollView* parent = static_cast<ScrollView*>(superView);
	
	Region clipArea = parent->ContentRegion();
	Point origin = parent->ConvertPointToWindow(clipArea.Origin());
	clipArea.x = origin.x;
	clipArea.y = origin.y;
	
	const Region intersect = clip.Intersect(clipArea);
	if (intersect.Dimensions().IsEmpty()) return; // outside the window/screen
	
	// clip drawing to the ContentRegion, then restore after drawing
	Video* video = core->GetVideoDriver();
	video->SetScreenClip(&intersect);
}

void ScrollView::ContentView::DidDraw(const Region& /*drawFrame*/, const Region& clip)
{
	core->GetVideoDriver()->SetScreenClip(&clip);
}

ScrollView::ScrollView(const Region& frame)
: View(frame), contentView(Region())
{
	View::AddSubviewInFrontOfView(&contentView);

	ScrollBar* sbar = GetControl<ScrollBar>("SBGLOB", 0);
	if (sbar == NULL) {
		// FIXME: this happens with the console window (non-issue, but causing noise)
		Log(ERROR, "ScrollView", "Unable to add scrollbars: missing default scrollbar template.");
		hscroll = NULL;
		vscroll = NULL;
	} else {
		// TODO: add horizontal scrollbars
		// this is a limitation in the Scrollbar class
		hscroll = NULL;
		
		vscroll = new ScrollBar(*sbar);
		
		Region sbFrame = vscroll->Frame();
		sbFrame.x = frame.w - sbFrame.w;
		sbFrame.y = 0;
		sbFrame.h = frame.h;
		
		vscroll->SetFrame(sbFrame);
		
		// ensure scrollbars are on top
		View::AddSubviewInFrontOfView(vscroll, &contentView);

		ControlEventHandler handler = [this](Control* sb) {
			ScrollbarValueChange(static_cast<ScrollBar*>(sb));
		};
		
		vscroll->SetAction(handler, Control::ValueChange);
		vscroll->SetAutoResizeFlags(ResizeRight|ResizeTop|ResizeBottom, OP_SET);
	}

	contentView.SetFrame(Region(Point(), frame.Dimensions()));
	contentView.SetFlags(RESIZE_WIDTH|RESIZE_HEIGHT, OP_OR);
	contentView.SetAutoResizeFlags(ResizeAll, OP_SET);
}

ScrollView::~ScrollView()
{
	View::RemoveSubview(&contentView); // no delete
	
	delete hscroll;
	delete vscroll;
}

void ScrollView::UpdateScrollbars()
{
	// FIXME: this is assuming an origin of 0,0
	const Size& mySize = ContentRegion().Dimensions();
	const Region& contentFrame = contentView.Frame();

	if (hscroll && contentFrame.w > mySize.w) {
		// assert(hscroll);
		// TODO: add horizontal scrollbars
		// this is a limitation in the Scrollbar class
		hscroll->SetValue(-contentFrame.Origin().x);
	}
	if (vscroll) {
		if (contentFrame.h > mySize.h) {
			vscroll->SetVisible((Flags()&View::IgnoreEvents) ? false : true);

			int maxVal = contentFrame.h - mySize.h;
			Control::ValueRange range(0, maxVal);
			vscroll->SetValueRange(range);
		} else {
			vscroll->SetVisible(false);
		}
		vscroll->SetValue(-contentFrame.Origin().y);
	}
}
	
void ScrollView::ScrollbarValueChange(ScrollBar* sb)
{
	const Point& origin = contentView.Origin();
	
	if (sb == hscroll) {
		Point p(-short(sb->GetValue()), origin.y);
		ScrollTo(p);
	} else if (sb == vscroll) {
		Point p(origin.x, -short(sb->GetValue()));
		ScrollTo(p);
	} else {
		Log(ERROR, "ScrollView", "ScrollbarValueChange for unknown scrollbar");
	}
}
	
void ScrollView::WillDraw(const Region& /*drawFrame*/, const Region& /*clip*/)
{
	if (animation) {
		// temporarily change the origin for drawing purposes
		contentView.SetFrameOrigin(animation.Next(GetTickCount()));
	}
}
	
void ScrollView::DidDraw(const Region& /*drawFrame*/, const Region& /*clip*/)
{
	if (animation) {
		// restore the origin to the true location passed to ScrollTo
		contentView.SetFrameOrigin(animation.end);
	}
}
	
Region ScrollView::ContentRegion() const
{
	Region cr = Region(Point(0,0), Dimensions());
	if (hscroll && hscroll->IsVisible()) {
		cr.h -= hscroll->Frame().h;
	}
	if (vscroll && vscroll->IsVisible()) {
		cr.w -= vscroll->Frame().w;
	}
	return cr;
}
	
void ScrollView::FlagsChanged(unsigned int /*oldflags*/)
{
	if (Flags()&IgnoreEvents) {
		if (hscroll) {
			hscroll->SetVisible(false);
		}
		
		if (vscroll) {
			vscroll->SetVisible(false);
		}
	}
}

void ScrollView::AddSubviewInFrontOfView(View* front, const View* back)
{
	contentView.AddSubviewInFrontOfView(front, back);
}

View* ScrollView::RemoveSubview(const View* view)
{
	return contentView.RemoveSubview(view);
}

View* ScrollView::SubviewAt(const Point& p, bool ignoreTransparency, bool recursive)
{
	View* v = View::SubviewAt(p, ignoreTransparency, recursive);
	return (v == &contentView) ? NULL : v;
}

void ScrollView::Update()
{
	contentView.ResizeToSubviews();
	UpdateScrollbars();
}

bool ScrollView::CanScroll(const Point& p) const
{
	const Size& mySize = ContentRegion().Dimensions();
	const Size& contentSize = contentView.Dimensions();
	return contentSize.h > mySize.h + p.y && contentSize.w > mySize.w + p.x;
}

Point ScrollView::ScrollOffset() const
{
	return contentView.Origin();
}

void ScrollView::SetScrollIncrement(int inc)
{
	if (hscroll) {
		hscroll->StepIncrement = inc;
	}
	if (vscroll) {
		vscroll->StepIncrement = inc;
	}
}

void ScrollView::ScrollDelta(const Point& p)
{
	ScrollDelta(p, 0);
}

void ScrollView::ScrollDelta(const Point& p, ieDword duration)
{
	ScrollTo(p + contentView.Origin(), duration);
}

void ScrollView::ScrollTo(const Point& p)
{
	ScrollTo(p, 0);
}

void ScrollView::ScrollTo(Point newP, ieDword duration)
{
	short maxx = frame.w - contentView.Dimensions().w;
	short maxy = frame.h - contentView.Dimensions().h;
	assert(maxx <= 0 && maxy <= 0);

	// clamp values so we dont scroll beyond the content
	newP.x = Clamp<short>(newP.x, maxx, 0);
	newP.y = Clamp<short>(newP.y, maxy, 0);

	Point current = (animation) ? animation.Current() : contentView.Origin();
	contentView.SetFrameOrigin(newP);
	UpdateScrollbars();

	// set up animation if required
	if  (duration) {
		animation = PointAnimation(current, newP, duration);
	} else {
		// cancel the existing animation (if any)
		animation = PointAnimation();
	}
}

bool ScrollView::OnKeyPress(const KeyboardEvent& key, unsigned short mod)
{
	// TODO: get scroll amount from settings
	int amount = 10;
	Point scroll;
	switch (key.keycode) {
		case GEM_UP:
			scroll.y = amount;
			break;
		case GEM_DOWN:
			scroll.y = -amount;
			break;
		case GEM_LEFT:
			scroll.x = amount;
			break;
		case GEM_RIGHT:
			scroll.x = -amount;
			break;
	}
	if (!scroll.isnull() && CanScroll(scroll)) {
		ScrollDelta(scroll);
		return true;
	}

	return this->View::OnKeyPress(key, mod);
}

bool ScrollView::OnMouseWheelScroll(const Point& delta)
{
	ScrollDelta(delta);
	return true;
}

bool ScrollView::OnMouseDrag(const MouseEvent& me)
{
	if (EventMgr::MouseButtonState(GEM_MB_ACTION)) {
		ScrollDelta(me.Delta());
		return true;
	}
	
	return false;
}

}
