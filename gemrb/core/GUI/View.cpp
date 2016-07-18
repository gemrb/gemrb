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

#include "GUI/GUIScriptInterface.h"
#include "GUI/ScrollBar.h"
#include "Interface.h"
#include "Sprite2D.h"
#include "Video.h"

namespace GemRB {

View::DragOp::DragOp(View* v) : dragView(v)
{
}

View::DragOp::~DragOp() {
	dragView->CompleteDragOperation(*this);
}

View::View(const Region& frame, View* superview)
	: frame(frame)
{
	scriptingRef = NULL;
	superView = NULL;
	window = NULL;

	dirty = true;
	flags = 0;

	if (superview)
		superview->AddSubviewInFrontOfView(this);
}

View::~View()
{
	DeleteScriptingRef();
	if (superView) {
		superView->RemoveSubview(this);
	}
	std::list<View*>::iterator it;
	for (it = subViews.begin(); it != subViews.end(); ++it) {
		View* view = *it;
		view->superView = NULL;
		delete view;
	}
}

void View::SetBackground(Sprite2D* bg)
{
	background = bg;

	MarkDirty();
}

void View::SetCursor(Sprite2D* c)
{
	cursor = c;
}

void View::MarkDirty()
{
	if (dirty == false) {
		dirty = true;

		if (superView && !IsOpaque()) {
			superView->DirtyBGRect(frame);
		}

		std::list<View*>::iterator it;
		for (it = subViews.begin(); it != subViews.end(); ++it) {
			(*it)->MarkDirty();
		}
	}
}

bool View::NeedsDraw() const
{
	if (frame.Dimensions().IsEmpty() || (flags&Invisible)) return false;

	// subviews are drawn over when the superview is drawn, so always redraw when super needs it.
	if (superView && superView->IsAnimated()) return true;

	return (dirty || IsAnimated());
}

bool View::IsVisible() const
{
	if (superView && !(flags&Invisible)) {
		return superView->IsVisible();
	}
	return !(flags&Invisible);
}

void View::DirtyBGRect(const Region& r)
{
	// if we are going to draw the entire BG, no need to compute and store this
	if (NeedsDraw())
		return;

	// do we want to intersect this too?
	//Region bgRgn = Region(background->XPos, background->YPos, background->Width, background->Height);
	Region clip(Point(), Dimensions());
	dirtyBGRects.push_back( r.Intersect(clip) );
}

void View::DrawSubviews() const
{
	std::list<View*>::const_iterator it;
	for (it = subViews.begin(); it != subViews.end(); ++it) {
		(*it)->Draw();
	}
}

void View::DrawBackground(const Region* rgn) const
{
	if (superView && !IsOpaque()) {
		if (rgn) {
			Region r = frame.Intersect(Region(ConvertPointToSuper(rgn->Origin()), rgn->Dimensions()));
			superView->DrawBackground(&r);
		} else {
			// already in super coordinates
			superView->DrawBackground(&frame);
		}
	}

	// FIXME: technically its possible for the BG to be smaller than the view
	// if this were to happen then the areas outside the BG wouldnt get redrawn
	// we should probably draw black in those areas...
	if (background) {
		Video* video = core->GetVideoDriver();
		if (rgn) {
			Region bgRgn = Region(background->XPos, background->YPos, background->Width, background->Height);
			Region intersect = rgn->Intersect(bgRgn);
			Point screenPt = ConvertPointToWindow(intersect.Origin());
			Region toClip(screenPt, intersect.Dimensions());
			video->BlitSprite( background.get(), intersect, toClip);
		} else {
			Point dp = ConvertPointToWindow(Point(background->XPos, background->YPos));
			video->BlitSprite( background.get(), dp.x, dp.y );
		}
	}
}

void View::Draw()
{
	if (flags&Invisible) return;

	Video* video = core->GetVideoDriver();
	const Region& clip = video->GetScreenClip();
	const Region& drawFrame = Region(ConvertPointToWindow(Point(0,0)), Dimensions());
	const Region& intersect = clip.Intersect(drawFrame);
	if (intersect.Dimensions().IsEmpty()) return; // outside the window/screen

	// clip drawing to the view bounds, then restore after drawing
	video->SetScreenClip(&intersect);
	// notify subclasses that drawing is about to happen. could pass the rects too, but no need ATM.
	WillDraw();

	if (NeedsDraw()) {
		if (background) {
			DrawBackground(NULL);
		}
		DrawSelf(drawFrame, intersect);
		dirty = false;
	} else {
		Regions::iterator it = dirtyBGRects.begin();
		while (it != dirtyBGRects.end()) {
			DrawBackground(&(*it++));
		}
	}

	dirtyBGRects.clear();
	// always call draw on subviews because they can be dirty without us
	DrawSubviews();
	// restore the screen clip
	video->SetScreenClip(&clip);
}

Point View::ConvertPointToSuper(const Point& p) const
{
	return p + Origin();
}

Point View::ConvertPointFromSuper(const Point& p) const
{
	return p - Origin();
}

Point View::ConvertPointToWindow(const Point& p) const
{
	// NULL superview is screen
	if (superView) {
		return superView->ConvertPointToWindow(ConvertPointToSuper(p));
	}
	return p;
}

Point View::ConvertPointFromWindow(const Point& p) const
{
	// NULL superview is screen
	if (superView) {
		return superView->ConvertPointFromWindow(ConvertPointFromSuper(p));
	}
	return p;
}

Point View::ConvertPointToScreen(const Point& p) const
{
	// NULL superview is screen
	Point newP = ConvertPointToSuper(p);
	if (superView) {
		newP = superView->ConvertPointToScreen(newP);
	}
	return newP;
}

Point View::ConvertPointFromScreen(const Point& p) const
{
	// NULL superview is screen
	Point newP = ConvertPointFromSuper(p);
	if (superView) {
		newP = superView->ConvertPointFromScreen(newP);
	}
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
	front->MarkDirty(); // must redraw the control now
	SubviewAdded(front, this);
	front->AddedToView(this);
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
	DirtyBGRect(subView->Frame());

	subView->superView = NULL;
	SubviewRemoved(subView, this);
	subView->RemovedFromView(this);
	return subView;
}

void View::SubviewAdded(View* view, View* parent)
{
	if (superView) {
		superView->SubviewAdded(view, parent);
	}
}

void View::SubviewRemoved(View* view, View* parent)
{
	if (superView) {
		superView->SubviewRemoved(view, parent);
	}
}

void View::AddedToView(View* view)
{
	Window* win = view->GetWindow();
	if (win == NULL)
		win = dynamic_cast<Window*>(view);
	window = win;
}

void View::RemovedFromView(View*)
{
	window = NULL;
}

bool View::HitTest(const Point& p) const
{
	if (!IsOpaque() && background) {
		return !background->IsPixelTransparent(p.x, p.y);
	}
	return true;
}

View* View::SubviewAt(const Point& p, bool ignoreTransparency, bool recursive)
{
	// iterate backwards because the backmost control would be drawn on top
	std::list<View*>::reverse_iterator it;
	for (it = subViews.rbegin(); it != subViews.rend(); ++it) {
		View* v = *it;
		const Region& viewFrame = v->Frame();
		Point subP = v->ConvertPointFromSuper(p);
		if (viewFrame.PointInside(p) && (ignoreTransparency || v->HitTest(subP))) {
			if (recursive) {
				View* subV = v->SubviewAt(subP, ignoreTransparency, recursive);
				v = (subV) ? subV : v;
			}
			return v;
		}
	}
	return NULL;
}

Window* View::GetWindow() const
{
	if (window)
		return window;

	if (superView) {
		Window* win = dynamic_cast<Window*>(superView);
		return (win) ? win : superView->GetWindow();
	}
	return NULL;
}

void View::SetFrame(const Region& r)
{
	SetFrameOrigin(r.Origin());
	SetFrameSize(r.Dimensions());
}

void View::SetFrameOrigin(const Point& p)
{
	frame.x = p.x;
	frame.y = p.y;
	MarkDirty();
}

void View::SetFrameSize(const Size& s)
{
	Size oldSize = frame.Dimensions();
	if (oldSize == s) return;

	frame.w = s.w;
	frame.h = s.h;

	SizeChanged(oldSize);

	if (Flags()&RESIZE_SUBVIEWS) {
		std::list<View*>::iterator it;
		for (it = subViews.begin(); it != subViews.end(); ++it) {
			View* subview = *it;
			Region newSubFrame = subview->Frame();
			newSubFrame.w += s.w - oldSize.w;
			newSubFrame.h += s.h - oldSize.h;
			subview->SetFrame(newSubFrame);
		}
	}
	MarkDirty();
}

bool View::SetFlags(unsigned int arg_flags, int opcode)
{
	unsigned int oldflags = flags;
	bool ret = SetBits(flags, arg_flags, opcode);

	if (flags != oldflags) {
		FlagsChanged(oldflags);
		MarkDirty();
	}

	return ret;
}

void View::SetTooltip(const String& string)
{
	tooltip = string;
	TrimString(tooltip); // for proper vertical alaignment
}

// View simpler either forwards events to concrete subclasses, or to its attached scrollbar
void View::OnMouseOver(const MouseEvent& me)
{
	if (superView) {
		superView->OnMouseOver(me);
	}
}

void View::OnMouseDrag(const MouseEvent& me)
{
	if (superView) {
		superView->OnMouseDrag(me);
	}
}

void View::OnMouseDown(const MouseEvent& me, unsigned short mod)
{
	if (superView) {
		superView->OnMouseDown(me, mod);
	}
}

void View::OnMouseUp(const MouseEvent& me, unsigned short mod)
{
	if (superView) {
		superView->OnMouseUp(me, mod);
	}
}

void View::OnMouseWheelScroll(const Point& delta)
{
	if (superView) {
		superView->OnMouseWheelScroll(delta);
	}
}

void View::AssignScriptingRef(ViewScriptingRef* ref)
{
	delete scriptingRef;
	scriptingRef = ref;
}

void View::DeleteScriptingRef()
{
	ScriptEngine::UnregisterScriptingRef(scriptingRef);
	AssignScriptingRef(NULL);
}

}
