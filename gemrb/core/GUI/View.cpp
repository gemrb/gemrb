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

#if DEBUG_VIEWS
#include "GUI/TextSystem/Font.h"

#include <typeinfo>
#endif

namespace GemRB {

View::DragOp::DragOp(View* v) : dragView(v)
{}

View::DragOp::~DragOp() {
	dragView->CompleteDragOperation(*this);
}

View::View(const Region& frame)
	: frame(frame)
{
	eventProxy = NULL;
	superView = NULL;
	window = NULL;

	dirty = true;
	flags = 0;
	autoresizeFlags = ResizeNone;
	
#if DEBUG_VIEWS
	debuginfo = false;
#endif
}

View::~View()
{
	ClearScriptingRefs();
	
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

void View::SetBackground(Sprite2D* bg, const Color* c)
{
	background = bg;
	if (c) backgroundColor = *c;

	MarkDirty();
}

void View::SetCursor(Sprite2D* c)
{
	cursor = c;
}

void View::SetEventProxy(View* proxy)
{
	while (proxy && proxy->eventProxy) {
		proxy = proxy->eventProxy;
	}
	
	eventProxy = proxy;
}

// TODO: while GemRB does support nested subviews, it does not (fully) support overlapping subviews (same superview, intersecting frame)
// expect weird things to happen with them
// this method takes the dirty region so the framework exists, but currently this just invalidates any intersecting subviews
void View::MarkDirty(const Region* rgn)
{
	if (dirty == false) {
		dirty = true;

		if (superView && !IsOpaque()) {
			superView->DirtyBGRect(frame);
		}

		std::list<View*>::iterator it;
		for (it = subViews.begin(); it != subViews.end(); ++it) {
			View* view = *it;
			if (rgn == NULL || view->frame.IntersectsRegion(*rgn)) {
				// TODO: implement partial redraws and pass the intersection rather than invalidating the entire view
				view->MarkDirty();
			}
		}
	}
}

void View::MarkDirty()
{
	return MarkDirty(NULL);
}

bool View::NeedsDraw() const
{
	// cull anything that can't be seen
	if (frame.Dimensions().IsEmpty() || (flags&Invisible)) return false;

	// check ourselves
	if (dirty || IsAnimated()) {
		return true;
	}

	// check our ancestors
	if (superView) {
		return superView->NeedsDraw();
	}

	// else we don't need an update
	return false;
}

bool View::IsVisible() const
{
	bool isVisible = !(flags&Invisible);
	if (superView && isVisible) {
		return superView->IsVisible();
	}
	return isVisible;
}

bool View::IsReceivingEvents() const
{
	bool getEvents = !(flags&(IgnoreEvents|Invisible|Disabled));
	if (superView && getEvents) {
		return superView->IsReceivingEvents();
	}
	return getEvents;
}

void View::DirtyBGRect(const Region& r)
{
	// no need to draw the parent BG for opaque views
	if (superView && !IsOpaque()) {
		Region rgn = frame.Intersect(Region(ConvertPointToSuper(r.Origin()), r.Dimensions()));
		superView->DirtyBGRect(rgn);
	}

	// if we are going to draw the entire BG, no need to compute and store this
	if (NeedsDraw())
		return;

	// do we want to intersect this too?
	//Region bgRgn = Region(background->Frame.x, background->Frame.y, background->Frame.w, background->Height);
	Region clip(Point(), Dimensions());
	Region dirty = r.Intersect(clip);
	dirtyBGRects.push_back(dirty);
	
	MarkDirty(&dirty);
}

void View::DrawSubviews() const
{
	std::list<View*>::const_iterator it;
	for (it = subViews.begin(); it != subViews.end(); ++it) {
		(*it)->Draw();
	}
}

Region View::DrawingFrame() const
{
	return Region(ConvertPointToWindow(Point(0,0)), Dimensions());
}

void View::DrawBackground(const Region* rgn) const
{
	Video* video = core->GetVideoDriver();
	if (backgroundColor.a > 0) {
		if (rgn) {
			Point p = ConvertPointToWindow(rgn->Origin());
			video->DrawRect(Region(p, rgn->Dimensions()), backgroundColor, true);
		} else if (window) {
			Point p = ConvertPointToWindow(frame.Origin());
			video->DrawRect(Region(p, Dimensions()), backgroundColor, true);
		} else {
			// FIXME: this is a Window and we need this hack becasue Window::WillDraw() changed the coordinate system
			video->DrawRect(Region(Point(), Dimensions()), backgroundColor, true);
		}
	}
		
	// NOTE: technically it's possible for the BG to be smaller than the view
	// if this were to happen then the areas outside the BG wouldnt get redrawn
	// you should make sure the backgound color is set to something suitable in those cases

	if (background) {
		if (rgn) {
			Region intersect = rgn->Intersect(background->Frame);
			Point screenPt = ConvertPointToWindow(intersect.Origin());
			Region toClip(screenPt, intersect.Dimensions());
			video->BlitSprite( background.get(), intersect, toClip);
		} else {
			Point dp = ConvertPointToWindow(Point(background->Frame.x, background->Frame.y));
			video->BlitSprite( background.get(), dp.x, dp.y );
		}
	}
}

void View::Draw()
{
	if (flags&Invisible) return;

	Video* video = core->GetVideoDriver();
	const Region clip = video->GetScreenClip();
	const Region& drawFrame = DrawingFrame();
	const Region& intersect = clip.Intersect(drawFrame);
	if (intersect.Dimensions().IsEmpty()) return; // outside the window/screen

	// clip drawing to the view bounds, then restore after drawing
	video->SetScreenClip(&intersect);

	bool needsDraw = NeedsDraw(); // check this before WillDraw else an animation update might get missed
	// notify subclasses that drawing is about to happen. could pass the rects too, but no need ATM.
	WillDraw();

	if (needsDraw) {
		DrawBackground(NULL);
		DrawSelf(drawFrame, intersect);
	} else {
		Regions::iterator it = dirtyBGRects.begin();
		while (it != dirtyBGRects.end()) {
			DrawBackground(&(*it++));
		}
	}

	dirtyBGRects.clear();

	// always call draw on subviews because they can be dirty without us
	DrawSubviews();
	DidDraw(); // notify subclasses that drawing finished
	dirty = false;
	
#if DEBUG_VIEWS
	Window* win = GetWindow();
	if (win == NULL ) {
		video->DrawRect(drawFrame, ColorBlue, false);
		debuginfo = EventMgr::ModState(GEM_MOD_SHIFT);
	} else if (NeedsDraw()) {
		video->DrawRect(drawFrame, ColorRed, false);
	} else {
		video->DrawRect(drawFrame, ColorGreen, false);
	}
	debuginfo = debuginfo || EventMgr::ModState(GEM_MOD_CTRL);
	
	if (debuginfo) {
		const ViewScriptingRef* ref = GetScriptingRef();
		if (ref) {
			Font* fnt = core->GetTextFont();
			ScriptingId id = ref->Id;
			id &= 0x00000000ffffffff; // control id is lower 32bits
			
			wchar_t string[256];
			swprintf(string, sizeof(string), L"id: %lu  grp: %s  \nflgs: %lu\ntype:%s",
					 id, ref->ScriptingGroup().CString(), flags, typeid(*this).name());
			Region r = drawFrame;
			r.w = (win) ? win->Frame().w - r.x : Frame().w - r.x;
			Font::StringSizeMetrics metrics = {r.Dimensions(), 0, true};
			fnt->StringSize(string, &metrics);
			r.h = metrics.size.h;
			r.w = metrics.size.w;
			video->SetScreenClip(NULL);
			video->DrawRect(r, ColorBlack, true);
			fnt->Print(r, string, NULL, IE_FONT_ALIGN_TOP|IE_FONT_ALIGN_LEFT);
		}
	}
#endif
	
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
		// remember, being "in front" means coming after in the list
		subViews.insert(++it, front);
	}

	front->superView = this;
	front->MarkDirty(); // must redraw the control now
	
	View* ancestor = this;
	do {
		ancestor->SubviewAdded(front, this);
		ancestor = ancestor->superView;
	} while (ancestor);
	
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
	assert(subView == view);
	subViews.erase(it);
	DirtyBGRect(subView->Frame());

	subView->superView = NULL;
	subView->RemovedFromView(this);
	
	View* ancestor = this;
	do {
		ancestor->SubviewRemoved(subView, this);
		ancestor = ancestor->superView;
	} while (ancestor);
	
	return subView;
}

View* View::RemoveFromSuperview()
{
	View* super = superView;
	if (super) {
		super->RemoveSubview(this);
	}
	return super;
}
	
void View::AddedToWindow(Window* newwin)
{
	window = newwin;
	std::list<View*>::iterator it;
	for (it = subViews.begin(); it != subViews.end(); ++it) {
		View* subview = *it;
		subview->AddedToWindow(newwin);
	}
}

void View::AddedToView(View* view)
{
	Window* newwin = view->GetWindow();
	if (newwin == NULL)
		newwin = dynamic_cast<Window*>(view);
	
	if (newwin != window) {
		AddedToWindow(newwin);
	}
}

void View::RemovedFromView(View*)
{
	window = NULL;
}

bool View::IsOpaque() const
{
	if (backgroundColor.a == 0xff) {
		return true;
	}
	
	return background && background->HasTransparency() == false;
}

bool View::HitTest(const Point& p) const
{
	Region r(Point(), Dimensions());
	if (!r.PointInside(p)) {
		return false;
	}

	if (!IsOpaque() && background) {
		return !background->IsPixelTransparent(p);
	}
	return true;
}

View* View::SubviewAt(const Point& p, bool ignoreTransparency, bool recursive)
{
	// iterate backwards because the backmost control would be drawn on top
	std::list<View*>::reverse_iterator it;
	for (it = subViews.rbegin(); it != subViews.rend(); ++it) {
		View* v = *it;
		Point subP = v->ConvertPointFromSuper(p);
		if ((ignoreTransparency && v->frame.PointInside(p)) || v->HitTest(subP)) {
			if (recursive) {
				View* subV = v->SubviewAt(subP, ignoreTransparency, recursive);
				v = (subV) ? subV : v;
			}
			return v;
		}
	}
	return NULL;
}

bool View::ContainsView(const View* view) const
{
	if (view == NULL) {
		return false;
	}

	if (this == view) {
		return true;
	}

	std::list<View*>::const_iterator it;
	for (it = subViews.begin(); it != subViews.end(); ++it) {
		const View* subview = *it;
		if (subview == view) {
			return true;
		}
		if (subview->ContainsView(view)) {
			return true;
		}
	}
	return false;
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

void View::ResizeSubviews(const Size& oldSize)
{
	std::list<View*>::iterator it;
	for (it = subViews.begin(); it != subViews.end(); ++it) {
		View* subview = *it;
		unsigned short flags = subview->AutoResizeFlags();

		if (flags == ResizeNone)
			continue;

		Region newSubFrame = subview->Frame();
		int delta = frame.w - oldSize.w;

		if (flags&ResizeRight) {
			if (flags&ResizeLeft) {
				newSubFrame.w += delta;
			} else {
				newSubFrame.x += delta;
			}
		} else if (flags&ResizeLeft) {
			newSubFrame.x += delta;
		}

		delta = frame.h - oldSize.h;
		if (flags&ResizeBottom) {
			if (flags&ResizeTop) {
				newSubFrame.h += delta;
			} else {
				newSubFrame.y += delta;
			}
		} else if (flags&ResizeTop) {
			newSubFrame.y += delta;
		}

		subview->SetFrame(newSubFrame);
	}
	MarkDirty();
}

void View::SetFrame(const Region& r)
{
	SetFrameOrigin(r.Origin());
	SetFrameSize(r.Dimensions());
}

void View::SetFrameOrigin(const Point& p)
{
	Point oldP = frame.Origin();
	if (oldP == p) return;
	
	MarkDirty(); // refresh the old position in the superview
	frame.x = p.x;
	frame.y = p.y;
	
	OriginChanged(oldP);
}

void View::SetFrameSize(const Size& s)
{
	const Size oldSize = frame.Dimensions();
	if (oldSize == s) return;

	MarkDirty(); // refresh the old position in the superview
	frame.w = std::max(0, s.w);
	frame.h = std::max(0, s.h);

	ResizeSubviews(oldSize);

	SizeChanged(oldSize);
}

bool View::SetFlags(unsigned int arg_flags, int opcode)
{
	unsigned int oldflags = flags;
	bool ret = SetBits(flags, arg_flags, opcode);

	if (flags != oldflags) {
		FlagsChanged(oldflags);
		MarkDirty();

		if (window && window->FocusedView() == this) {
			if (CanLockFocus() == false) {
				window->SetFocused(NULL);
			}
		}
	}

	return ret;
}
	
bool View::SetAutoResizeFlags(unsigned short arg_flags, int opcode)
{
	return SetBits(autoresizeFlags, arg_flags, opcode);
}

void View::SetTooltip(const String& string)
{
	tooltip = string;
	TrimString(tooltip); // for proper vertical alaignment
}

// View simply forwards to its eventProxy, if one exists,
// otherwise attempts to handle the event by calling "OnEvent"
// if the event is unhandled, bubble it up to the superview
#define HandleEvent1(meth, p1) \
HandleEvent(meth(p1))

#define HandleEvent2(meth, p1, p2) \
HandleEvent(meth(p1, p2))
	
#define HandleEvent(meth) \
if (eventProxy) { \
	eventProxy->On ## meth; \
	return; \
} \
\
if ((flags&(Disabled|IgnoreEvents)) == 0) { \
	bool handled = On ## meth; \
	if (handled == false && superView) { \
		superView->meth; \
	} \
}

bool View::KeyPress(const KeyboardEvent& key, unsigned short mod)
{
	if (eventProxy) {
		return eventProxy->OnKeyPress(key, mod);
	}
	return OnKeyPress(key, mod);
}

bool View::KeyRelease(const KeyboardEvent& key, unsigned short mod)
{
	if (eventProxy) {
		return eventProxy->OnKeyRelease(key, mod);
	}
	return OnKeyRelease(key, mod);
}

void View::TextInput(const TextEvent& te)
{
	if (eventProxy) {
		return eventProxy->OnTextInput(te);
	}
	OnTextInput(te);
}

void View::MouseEnter(const MouseEvent& me, const DragOp* op)
{
#if DEBUG_VIEWS
	debuginfo = true;
#endif
	
	OnMouseEnter(me, op);
}

void View::MouseLeave(const MouseEvent& me, const DragOp* op)
{
#if DEBUG_VIEWS
	debuginfo = false;
	MarkDirty();
#endif
	
	OnMouseLeave(me, op);
}

void View::MouseOver(const MouseEvent& me)
{
	HandleEvent1(MouseOver, me);
}

void View::MouseDrag(const MouseEvent& me)
{
	HandleEvent1(MouseDrag, me);
}

void View::MouseDown(const MouseEvent& me, unsigned short mod)
{
	HandleEvent2(MouseDown, me, mod);
}

void View::MouseUp(const MouseEvent& me, unsigned short mod)
{
	HandleEvent2(MouseUp, me, mod);
}

void View::MouseWheelScroll(const Point& delta)
{
	HandleEvent1(MouseWheelScroll, delta);
}

void View::TouchDown(const TouchEvent& te, unsigned short mod)
{
	HandleEvent2(TouchDown, te, mod);
}

void View::TouchUp(const TouchEvent& te, unsigned short mod)
{
	HandleEvent2(TouchUp, te, mod);
}

void View::TouchGesture(const GestureEvent& gesture)
{
	HandleEvent1(TouchGesture, gesture);
}

bool View::OnTouchDown(const TouchEvent& te, unsigned short mod)
{
	// default acts as left mouse down
	if (te.numFingers == 1) {
		// TODO: use touch pressure to toggle tooltips
		MouseEvent me = MouseEventFromTouch(te, true);
		return OnMouseDown(me, mod);
	}
	return false;
}

bool View::OnTouchUp(const TouchEvent& te, unsigned short mod)
{
	// default acts as left mouse up
	if (te.numFingers == 1) {
		MouseEvent me = MouseEventFromTouch(te, false);
		return OnMouseUp(me, mod);
	}
	return false;
}

bool View::OnTouchGesture(const GestureEvent& gesture)
{
	// default acts as mouse drag for 1 finger
	// or a mousewheel event for 2
	if (gesture.numFingers == 1) {
		MouseEvent me = MouseEventFromTouch(gesture, true);
		return OnMouseDrag(me);
	} else if (gesture.numFingers == 2) {
		return OnMouseWheelScroll(gesture.Delta());
	}
	return false;
}

const ViewScriptingRef* View::ReplaceScriptingRef(const ViewScriptingRef* old, ScriptingId id, ResRef group)
{
	std::vector<ViewScriptingRef*>::iterator it = std::find(scriptingRefs.begin(), scriptingRefs.end(), old);
	if (it != scriptingRefs.end()) {
		bool unregistered = ScriptEngine::UnregisterScriptingRef(old);
		assert(unregistered);
		delete old;

		ViewScriptingRef* newref = CreateScriptingRef(id, group);

		if (ScriptEngine::RegisterScriptingRef(newref)) {
			*it = newref;
			return newref;
		}
	}
	return nullptr;
}

const ViewScriptingRef* View::RemoveScriptingRef(const ViewScriptingRef* ref)
{
	static ScriptingId id = 0;
	return ReplaceScriptingRef(ref, id++, "__DEL__");
}
	
void View::ClearScriptingRefs()
{
	std::vector<ViewScriptingRef*>::iterator rit;
	for (rit = scriptingRefs.begin(); rit != scriptingRefs.end();) {
		ViewScriptingRef* ref = *rit;
		assert(GetView(ref) == this);
		bool unregistered = ScriptEngine::UnregisterScriptingRef(ref);
		assert(unregistered);
		delete ref;
		rit = scriptingRefs.erase(rit);
	}
}
	
ViewScriptingRef* View::CreateScriptingRef(ScriptingId id, ResRef group)
{
	return new ViewScriptingRef(this, id, group);
}
	
const ViewScriptingRef* View::AssignScriptingRef(ScriptingId id, ResRef group)
{
	ViewScriptingRef* ref = CreateScriptingRef(id, group);
	if (ScriptEngine::RegisterScriptingRef(ref)) {
		scriptingRefs.push_back(ref);
		return ref;
	} else {
		delete ref;
		return NULL;
	}
}

const ViewScriptingRef* View::GetScriptingRef(ScriptingId id, ResRef group) const
{
	auto it = std::find_if(scriptingRefs.begin(), scriptingRefs.end(), [&](const ViewScriptingRef* ref) {
		return ref->Id == id && ref->ScriptingGroup() == group;
	});
	
	return (it != scriptingRefs.end()) ? *it : nullptr;
}
	
const ViewScriptingRef* View::GetScriptingRef() const
{
	if (scriptingRefs.empty()) {
		return NULL;
	}
	return scriptingRefs.front();
}

}
