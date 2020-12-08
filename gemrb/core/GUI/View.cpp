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
#include "GUI/TextSystem/Font.h"
#include "Interface.h"
#include "Sprite2D.h"
#include "Video.h"

#include <typeinfo>

namespace GemRB {

View::DragOp::DragOp(View* v, Holder<Sprite2D> cursor)
: dragView(v), cursor(cursor)
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

void View::SetBackground(Holder<Sprite2D> bg, const Color* c)
{
	background = bg;
	if (c) backgroundColor = *c;

	MarkDirty();
}

void View::SetCursor(Holder<Sprite2D> c)
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
	// TODO: we could implement partial redraws by storing the dirty region
	// not much to gain at the moment, however

	if (dirty == false) {
		dirty = true;

		if (superView && !IsOpaque()) {
			superView->DirtyBGRect(frame);
		}

		std::list<View*>::iterator it;
		for (it = subViews.begin(); it != subViews.end(); ++it) {
			View* view = *it;
			if (rgn) {
				Region intersect = view->frame.Intersect(*rgn);
				const Size& idims = intersect.Dimensions();
				if (!idims.IsEmpty()) {
					Point p = view->ConvertPointFromSuper(intersect.Origin());
					Region r = Region(p, idims);
					view->MarkDirty(&r);
				}
			} else {
				Point p = view->ConvertPointFromSuper(Point());
				Region r = Region(p, Dimensions());
				view->MarkDirty(&r);
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

	// else we don't need an update
	return false;
}

bool View::NeedsDrawRecursive() const
{
	if (NeedsDraw()) {
		return true;
	}
	
	if (superView) {
		return superView->NeedsDrawRecursive();
	}
	
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
	if (NeedsDrawRecursive())
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

	bool needsDraw = NeedsDrawRecursive(); // check this before WillDraw else an animation update might get missed
	// notify subclasses that drawing is about to happen. could pass the rects too, but no need ATM.
	WillDraw(drawFrame, intersect);

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
	DidDraw(drawFrame, intersect); // notify subclasses that drawing finished
	dirty = false;

	if (core->InDebugMode(ID_VIEWS)) {
		Window* win = GetWindow();
		if (win == nullptr) {
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
				Font::StringSizeMetrics metrics = {r.Dimensions(), 0, 0, true};
				fnt->StringSize(string, &metrics);
				r.h = metrics.size.h;
				r.w = metrics.size.w;
				video->SetScreenClip(nullptr);
				video->DrawRect(r, ColorBlack, true);
				fnt->Print(r, string, nullptr, IE_FONT_ALIGN_TOP|IE_FONT_ALIGN_LEFT);
			}
		}
	}

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

Region View::ConvertRegionToSuper(Region r) const
{
	r.SetOrigin(ConvertPointToSuper(r.Origin()));
	return r;
}

Region View::ConvertRegionFromSuper(Region r) const
{
	r.SetOrigin(ConvertPointFromSuper(r.Origin()));
	return r;
}

Region View::ConvertRegionToWindow(Region r) const
{
	r.SetOrigin(ConvertPointToWindow(r.Origin()));
	return r;
}

Region View::ConvertRegionFromWindow(Region r) const
{
	r.SetOrigin(ConvertPointFromWindow(r.Origin()));
	return r;
}

Region View::ConvertRegionToScreen(Region r) const
{
	r.SetOrigin(ConvertPointToScreen(r.Origin()));
	return r;
}

Region View::ConvertRegionFromScreen(Region r) const
{
	r.SetOrigin(ConvertPointFromScreen(r.Origin()));
	return r;
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
	if (core->InDebugMode(ID_VIEWS)) {
		debuginfo = true;
	}

	OnMouseEnter(me, op);
}

void View::MouseLeave(const MouseEvent& me, const DragOp* op)
{
	if (core->InDebugMode(ID_VIEWS)) {
		debuginfo = false;
		MarkDirty();
	}

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
	if ((eventProxy && !eventProxy->IsPerPixelScrollable())
		|| (eventProxy == nullptr && !IsPerPixelScrollable())) {
		Point scaledDelta;
		int speed = core->GetMouseScrollSpeed();

		if (delta.x < 0) {
			scaledDelta.x = std::min<int>(delta.x / speed, -1);
		} else if (delta.x > 0) {
			scaledDelta.x = std::max<int>(delta.x / speed, 1);
		}
		
		if (delta.y < 0) {
			scaledDelta.y = std::min<int>(delta.y / speed, -1);
		} else if (delta.y > 0) {
			scaledDelta.y = std::max<int>(delta.y / speed, 1);
		}
		
		HandleEvent1(MouseWheelScroll, scaledDelta);
	} else {
		HandleEvent1(MouseWheelScroll, delta);
	}
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

void View::ControllerAxis(const ControllerEvent& ce)
{
	HandleEvent1(ControllerAxis, ce);
}

void View::ControllerButtonDown(const ControllerEvent& ce)
{
	HandleEvent1(ControllerButtonDown, ce);
}

void View::ControllerButtonUp(const ControllerEvent& ce)
{
	HandleEvent1(ControllerButtonUp, ce);
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

bool View::OnControllerAxis(const ControllerEvent& ce)
{
	MouseEvent me = MouseEventFromController(ce, true);
	if (me.buttonStates) {
		return OnMouseDrag(me);
	}
	return OnMouseOver(me);
}

bool View::OnControllerButtonDown(const ControllerEvent& ce)
{
	if (ce.button == CONTROLLER_BUTTON_A
		|| ce.button == CONTROLLER_BUTTON_B
		|| ce.button == CONTROLLER_BUTTON_LEFTSTICK)
	{
		MouseEvent me = MouseEventFromController(ce, true);
		// TODO: we might want to add modifiers for "trigger" buttons
		return OnMouseDown(me, 0);
	}
	
	if (ce.button == CONTROLLER_BUTTON_START)
	{
		core->TogglePause();
		return true;
	}
	
	if (ce.button == CONTROLLER_BUTTON_GUIDE)
	{
		core->GetVideoDriver()->StartTextInput();
		return true;
	}
	
	// TODO: we might want to add modifiers for "trigger" buttons
	KeyboardEvent ke = KeyEventFromController(ce);
	return OnKeyPress(ke, 0);
}

bool View::OnControllerButtonUp(const ControllerEvent& ce)
{
	if (ce.button == CONTROLLER_BUTTON_A
		|| ce.button == CONTROLLER_BUTTON_B
		|| ce.button == CONTROLLER_BUTTON_LEFTSTICK)
	{
		MouseEvent me = MouseEventFromController(ce, false);
		// TODO: we might want to add modifiers for "trigger" buttons
		return OnMouseUp(me, 0);
	}
	
	// TODO: we might want to add modifiers for "trigger" buttons
	KeyboardEvent ke = KeyEventFromController(ce);
	return OnKeyRelease(ke, 0);
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
