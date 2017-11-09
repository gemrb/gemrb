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
	// content view behaves as if RESIZE_WIDTH and RESIZE_HEIGHT are set always
	Size newSize = superView->Dimensions();
	
	if (!subViews.empty()) {
		std::list<View*>::iterator it = subViews.begin();
		View* view = *it;
		Region bounds = view->Frame();
		
		for (++it; it != subViews.end(); ++it) {
			Region r = view->Frame();
			bounds = Region::RegionEnclosingRegions(bounds, r);
		}
		
		//Point origin = Origin() + bounds.Origin();
		//SetFrameOrigin(origin);
		
		newSize.w = std::max(newSize.w, bounds.w + bounds.x);
		newSize.h = std::max(newSize.h, bounds.h + bounds.y);
	}
	SetFrameSize(newSize);
}
	
void ScrollView::ContentView::WillDraw()
{
	Video* video = core->GetVideoDriver();
	screenClip = video->GetScreenClip();
	
	ScrollView* parent = static_cast<ScrollView*>(superView);
	
	Region clipArea = parent->ContentRegion();
	Point origin = parent->ConvertPointToWindow(clipArea.Origin());
	clipArea.x = origin.x;
	clipArea.y = origin.y;
	
	const Region intersect = screenClip.Intersect(clipArea);
	if (intersect.Dimensions().IsEmpty()) return; // outside the window/screen
	
	// clip drawing to the ContentRegion, then restore after drawing
	video->SetScreenClip(&intersect);
}

void ScrollView::ContentView::DidDraw()
{
	core->GetVideoDriver()->SetScreenClip(&screenClip);
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
		vscroll->SetAutoResizeFlags(ResizeRight|ResizeTop|ResizeBottom, OP_SET);
	}
	
	contentView.SetFrame(Region(Point(), frame.Dimensions()));
	contentView.SetFlags(RESIZE_WIDTH|RESIZE_HEIGHT, OP_OR);
}

ScrollView::~ScrollView()
{
	View::RemoveSubview(&contentView); // no delete
	
	delete hscroll;
	delete vscroll;
}
	
void ScrollView::SizeChanged(const Size& /* oldsize */)
{
	Update();
}

void ScrollView::ContentFrameChanged()
{
	// FIXME: this is assuming an origin of 0,0
	const Size& mySize = ContentRegion().Dimensions();
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
		Point p(-sb->GetValue(), origin.y);
		ScrollTo(p);
	} else if (sb == vscroll) {
		Point p(origin.x, -sb->GetValue());
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

	Point origin = contentView.Origin(); // how much we have scrolled in -px

	if (vscroll) {
		vscroll->SetValue(-origin.y);
	}
	if (hscroll) {
		hscroll->SetValue(-origin.x);
	}
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
	newP.x = Clamp<short>(newP.x, maxx, 0);
	newP.y = Clamp<short>(newP.y, maxy, 0);

	// set up animation if required
	if  (duration) {
		animation = PointAnimation(contentView.Origin(), newP, duration);
	} else {
		// cancel the existing animation (if any)
		animation = PointAnimation();
	}

	contentView.SetFrameOrigin(newP);
	Update();
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
	if (EventMgr::ButtonState(GEM_MB_ACTION)) {
		ScrollDelta(me.Delta());
		return true;
	}
	
	return false;
}

}
