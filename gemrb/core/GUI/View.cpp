/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2015 The GemRB Project
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

#include "View.h"

#include "GUI/ScrollBar.h"
#include "Interface.h"
#include "Video.h"

namespace GemRB {

View::View(const Region& frame)
	: frame(frame)
{
	superView = NULL;

	dirty = true;
}

View::~View()
{
	if (superView) {
		superView->RemoveSubview(this);
	}
	std::list<View*>::iterator it;
	for (it = subViews.begin(); it != subViews.end(); ++it) {
		delete *it;
	}
}

void View::MarkDirty()
{
	dirty = true;
}

bool View::NeedsDraw() const
{
	return (!frame.Dimensions().IsEmpty() && (dirty || IsAnimated()));
}

void View::DrawSubviews()
{
	std::list<View*>::iterator it;
	for (it = subViews.begin(); it != subViews.end(); ++it) {
		View* v = *it;
		v->Draw();
	}
}

void View::Draw()
{
	Video* video = core->GetVideoDriver();

	const Region& clip = video->GetScreenClip();
	const Region& drawFrame = Region(ConvertPointToSuper(Point(0,0)), Dimensions());
	const Region& intersect = clip.Intersect(drawFrame);
	if (intersect.Dimensions().IsEmpty()) return; // outside the window/screen

	if (NeedsDraw()) {
		// clip drawing to the control bounds, then restore after drawing
		video->SetScreenClip(&drawFrame);
		DrawSelf(drawFrame, intersect);
		video->SetScreenClip(&clip);
		dirty = false;
	}

	// always call draw on subviews because they can be dirty without us
	DrawSubviews();
}

Point View::ConvertPointToSuper(const Point& p) const
{
	Point newP = p + Origin();
	return newP;
}

Point View::ConvertPointFromSuper(const Point& p) const
{
	Point newP = p - Origin();
	return newP;
}

void View::AddSubviewInFrontOfView(View* front, const View* back)
{
	if (front == NULL) return;

	std::list<View*>::iterator it;
	it = std::find(subViews.begin(), subViews.end(), back);

	View* super = front->superView;
	if (super == this) {
		// already here, but may need to move the view
		std::list<View*>::iterator cur;
		cur = std::find(subViews.begin(), subViews.end(), front);
		subViews.splice(it, subViews, cur);
	} else {
		if (super != NULL) {
			front->superView->RemoveSubview(front);
		}
		subViews.insert(it, front);
	}

	front->superView = this;
	SubviewAdded(front);
}

View* View::RemoveSubview(const View* view)
{
	if (!view || view->superView != this) {
		return NULL;
	}

	std::list<View*>::iterator it;
	it = std::find(subViews.begin(), subViews.end(), view);
	assert(it != subViews.end());

	View* subView = *it;
	subViews.erase(it);
	const Region& viewFrame = subView->Frame();
	SubviewRemoved(subView);
	subView->superView = NULL;
	return subView;
}

void View::SetFrame(const Region& r)
{
	frame = r;
	MarkDirty();
}

void View::SetFrameOrigin(const Point& p)
{
	frame.x = p.x;
	frame.y = p.y;
	MarkDirty();
}

void View::SetFrameSize(const Size& s)
{
	frame.w = s.w;
	frame.h = s.h;
	MarkDirty();
}

}
