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

namespace GemRB {
	
typedef void (ScrollView::*ScrollCB)(Control*);
	
void ScrollView::ContentView::SizeChanged(const Size& /*oldsize*/)
{
	// considering it an error for a ContentView to exist outside of a ScrollView
	assert(superView);
	ScrollView* sv = static_cast<ScrollView*>(superView);
	sv->ContentFrameChanged();
}
	
void ScrollView::ContentView::SubviewAdded(View* /*view*/, View* parent)
{
	if (parent == this) {
		ResizeToSubviews();
	}
}
	
void ScrollView::ContentView::SubviewRemoved(View* /*view*/, View* parent)
{
	if (parent == this) {
		ResizeToSubviews();
	}
}
	
void ScrollView::ContentView::ResizeToSubviews()
{
	Size newSize = superView->Dimensions();
	
	if (!subViews.empty()) {
		std::list<View*>::iterator it = subViews.begin();
		Region bounds = (*it)->Frame();
		
		for (++it; it != subViews.end(); ++it) {
			Region r = (*it)->Frame();
			bounds = Region::RegionEnclosingRegions(bounds, r);
		}
		
		//Point origin = Origin() + bounds.Origin();
		//SetFrameOrigin(origin);
		
		newSize.w = std::max(newSize.w, bounds.w);
		newSize.h = std::max(newSize.h, bounds.h);
	}
	SetFrameSize(newSize);
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

		ControlEventHandler handler = NULL;
		ScrollCB cb = reinterpret_cast<ScrollCB>(&ScrollView::ScrollbarValueChange);
		handler = new MethodCallback<ScrollView, Control*>(this, cb);
		vscroll->SetAction(handler, Control::ValueChange);
	}
	
	contentView.SetFrame(Region(Point(), frame.Dimensions()));
	contentView.SetFlags(RESIZE_WIDTH|RESIZE_HEIGHT, OP_OR);
	SetFlags(RESIZE_SUBVIEWS, OP_OR);
}

ScrollView::~ScrollView()
{
	View::RemoveSubview(&contentView); // no delete
	
	delete hscroll;
	delete vscroll;
}

void ScrollView::ContentFrameChanged()
{
	const Size& mySize = Dimensions();
	const Size& contentSize = contentView.Dimensions();

	if (hscroll && contentSize.w > mySize.w) {
		// assert(hscroll);
		// TODO: add horizontal scrollbars
		// this is a limitation in the Scrollbar class
	}
	if (vscroll) {
		if (contentSize.h > mySize.h) {
			vscroll->SetVisible(true);
			
			int maxVal = contentSize.h - mySize.h;
			Control::ValueRange range(0, maxVal);
			vscroll->SetValueRange(range);
		} else {
			vscroll->SetVisible(false);
		}
	}
}
	
void ScrollView::ScrollbarValueChange(ScrollBar* sb)
{
	const Point& origin = contentView.Origin();
	
	if (sb == hscroll) {
		Point p(sb->GetValue(), origin.y);
		ScrollTo(p);
	} else if (sb == vscroll) {
		Point p(origin.x, sb->GetValue());
		ScrollTo(p);
	} else {
		Log(ERROR, "ScrollView", "ScrollbarValueChange for unknown scrollbar");
	}
}
	
void ScrollView::WillDraw()
{
	if (animation) {
		// temporarily change the origin for drawing purposes
		contentView.SetFrameOrigin(animation.NextPoint());
	}
}
	
void ScrollView::DidDraw()
{
	if (animation) {
		// restore the origin to the true location passed to ScrollTo
		contentView.SetFrameOrigin(animation.end);
	}
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

void ScrollView::ScrollDelta(const Point& p, ieDword duration)
{
	ScrollTo(p + contentView.Origin(), duration);
}

void ScrollView::ScrollTo(Point newP, ieDword duration)
{
	short maxx = frame.w - contentView.Dimensions().w;
	short maxy = frame.h - contentView.Dimensions().h;
	assert(maxx <= 0 && maxy <= 0);

	// clamp values so we dont scroll beyond the content
	newP.x = Clamp<short>(newP.x, 0, maxx);
	newP.y = Clamp<short>(newP.y, 0, maxy);

	// set up animation if required
	if  (duration) {
		animation = PointAnimation(contentView.Origin(), newP, duration);
	} else {
		// cancel the existing animation (if any)
		animation = PointAnimation();
	}

	contentView.SetFrameOrigin(newP);
}

bool ScrollView::OnKeyPress(const KeyboardEvent& key, unsigned short /*mod*/)
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
	if (!scroll.isnull()) {
		ScrollDelta(scroll);
		return true;
	}

	return false;
}

void ScrollView::OnMouseWheelScroll(const Point& delta)
{
	ScrollDelta(delta);
}

void ScrollView::OnMouseDrag(const MouseEvent& me)
{
	if (EventMgr::ButtonState(GEM_MB_ACTION)) {
		ScrollDelta(me.Delta());
	}
}

}
