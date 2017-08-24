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
	
void ScrollView::ContentView::SizeChanged(const Size& /* oldsize */)
{
	// considering it an error for a ContentView to exist outside of a ScrollView
	assert(superView);
	ScrollView* sv = static_cast<ScrollView*>(superView);
	sv->ContentSizeChanged(Frame().Dimensions());
}

ScrollView::ScrollView(const Region& frame)
: View(frame), contentView(Region())
{
	ScrollBar* sbar = GetControl<ScrollBar>("SBGLOB", 0);
	if (sbar == NULL) {
		// FIXME: this happens with the console window (non-issue, but causing noise)
		Log(ERROR, "ScrollView", "Unable to add scrollbars: missing default scrollbar template.");
		hscroll = NULL;
		vscroll = NULL;
	} else {
		// TODO: add horizontal scrollbars
		// this is a limitation in the Scrollbar class
		// don't bother setting or changing the frame, other methods deal with that.
		hscroll = NULL;
		
		vscroll = new ScrollBar(*sbar);
		vscroll->SetVisible(false);
		
		View::AddSubviewInFrontOfView(vscroll);

		ControlEventHandler handler = NULL;
		ScrollCB cb = reinterpret_cast<ScrollCB>(&ScrollView::ScrollbarValueChange);
		handler = new MethodCallback<ScrollView, Control*>(this, cb);
		vscroll->SetAction(handler, Control::ValueChange);
	}

	View::AddSubviewInFrontOfView(&contentView);
	contentView.SetFrame(Region(Point(), frame.Dimensions()));
	contentView.SetFlags(RESIZE_WIDTH|RESIZE_HEIGHT, OP_OR);
}

ScrollView::~ScrollView()
{
	View::RemoveSubview(&contentView); // no delete
	
	delete hscroll;
	delete vscroll;
}

void ScrollView::ContentSizeChanged(const Size& contentSize)
{
	const Size& mySize = Dimensions();

	if (hscroll && contentSize.w > mySize.w) {
		// assert(hscroll);
		// TODO: add horizontal scrollbars
		// this is a limitation in the Scrollbar class
	}
	if (vscroll && contentSize.h > mySize.h) {
		assert(vscroll);
		
		Region sbFrame = vscroll->Frame();
		sbFrame.x = frame.w - sbFrame.w;
		sbFrame.y = 0;
		sbFrame.h = frame.h;
		
		vscroll->SetFrame(sbFrame);
		vscroll->SetVisible(true);

		Control::ValueRange range(0, contentSize.h - mySize.h);
		vscroll->SetValueRange(range);
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

void ScrollView::ScrollDelta(const Point& p)
{
	ScrollTo(p + contentView.Origin());
}

void ScrollView::ScrollTo(Point newP)
{
	short maxx = frame.w - contentView.Dimensions().w;
	short maxy = frame.h - contentView.Dimensions().h;
	assert(maxx <= 0 && maxy <= 0);

	// clamp values so we dont scroll beyond the content
	newP.x = Clamp<short>(newP.x, 0, maxx);
	newP.y = Clamp<short>(newP.y, 0, maxy);

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
