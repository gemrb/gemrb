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

#include "Debug.h"
#include "GUI/GUIScriptInterface.h"
#include "GUI/ScrollBar.h"
#include "GUI/TextSystem/Font.h"
#include "Interface.h"
#include "Sprite2D.h"
#include "Video/Video.h"

#include <typeinfo>
#include <utility>


namespace GemRB {

View::DragOp::DragOp(View* v, Holder<Sprite2D> cursor)
: dragView(v), cursor(std::move(cursor))
{}

View::DragOp::~DragOp() {
	dragView->CompleteDragOperation(*this);
}

View::View(const Region& frame)
	: frame(frame)
{}

View::~View()
{
	ClearScriptingRefs();
	
	if (superView) {
		superView->RemoveSubview(this);
	}

	for (const auto& subView : subViews) {
		subView->superView = nullptr;
		delete subView;
	}
}

void View::SetBackground(Holder<Sprite2D> bg, const Color* c)
{
	background = std::move(bg);
	if (c) backgroundColor = *c;

	MarkDirty();
}

void View::SetCursor(Holder<Sprite2D> c)
{
	cursor = std::move(c);
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
void View::MarkDirty(const Region* /*rgn*/)
{
	// TODO: we could implement partial redraws by storing the dirty region
	// not much to gain at the moment, however

	dirty = true;
}

void View::MarkDirty()
{
	return MarkDirty(nullptr);
}

bool View::NeedsDraw() const
{
	// cull anything that can't be seen
	if (frame.size.IsInvalid() || (flags&Invisible)) return false;

	// check ourselves
	if (dirty || IsAnimated()) {
		return true;
	}

	// else we don't need an update
	return false;
}

void View::InvalidateSubviews() const
{
	for (View* subview : subViews) {
		Region r = ConvertRegionFromSuper(frame);
		r = r.Intersect(subview->frame);
		r.origin = subview->ConvertPointFromSuper(r.origin);
		subview->MarkDirty(&r);
	}
}

bool View::IsVisible() const
{
	bool isVisible = !(flags&Invisible);
	if (superView && isVisible) {
		return superView->IsVisible();
	}
	return isVisible;
}

bool View::IsDisabledCursor() const
{
	if (InDebugMode(DebugMode::VIEWS)) {
		return IsDisabled();
	}
	return false;
}

bool View::IsReceivingEvents() const
{
	bool getEvents = !(flags&(IgnoreEvents|Invisible|Disabled));
	if (superView && getEvents) {
		return superView->IsReceivingEvents();
	}
	return getEvents;
}

Regions View::DirtySuperViewRegions() const
{
	// since we dont support overlapping views...
	// if we are opaque we cover everything and dont care about the superview
	// if we arent but we need to redraw then we simply report our entire area

	if (IsOpaque()) {
		return {};
	}

	if (NeedsDraw()) {
		return { frame };
	}
	
	Regions dirtyAreas;
	for (const View* subview : subViews) {
		Regions r = subview->DirtySuperViewRegions();
		dirtyAreas.reserve(dirtyAreas.size() + r.size());
		std::transform(r.begin(), r.end(), std::back_inserter(dirtyAreas), [this](const Region& rgn) {
			return ConvertRegionToSuper(rgn).Intersect(frame);
		});
	}
	return dirtyAreas;
}

void View::DrawSubviews(bool drawBG) const
{
	drawBG = drawBG && HasBackground();
	for (View* subview : subViews) {
		if (drawBG && !subview->IsOpaque()) {
			for (const Region& r : subview->DirtySuperViewRegions()) {
				DrawBackground(&r);
			}
		}
		subview->Draw();
	}
}

Region View::DrawingFrame() const
{
	return Region(ConvertPointToWindow(Point(0,0)), Dimensions());
}

bool View::HasBackground() const
{
	return backgroundColor.a > 0 || background;
}

void View::DrawBackground(const Region* rgn) const
{
	if (backgroundColor.a > 0) {
		if (rgn) {
			Region r = ConvertRegionToWindow(*rgn);
			VideoDriver->DrawRect(r, backgroundColor, true);
		} else if (window) {
			assert(superView);
			Region r = superView->ConvertRegionToWindow(frame);
			VideoDriver->DrawRect(r, backgroundColor, true);
		} else {
			// FIXME: this is a Window and we need this hack becasue Window::WillDraw() changed the coordinate system
			VideoDriver->DrawRect(Region(Point(), Dimensions()), backgroundColor, true);
		}
	}
		
	// NOTE: technically it's possible for the BG to be smaller than the view
	// if this were to happen then the areas outside the BG wouldnt get redrawn
	// you should make sure the backgound color is set to something suitable in those cases

	if (background) {
		if (rgn) {
			Region intersect = rgn->Intersect(background->Frame);
			Point screenPt = ConvertPointToWindow(intersect.origin);
			Region toClip(screenPt, intersect.size);
			VideoDriver->BlitSprite(background, intersect, toClip, BlitFlags::BLENDED);
		} else {
			Point dp = ConvertPointToWindow(Point(background->Frame.x, background->Frame.y));
			VideoDriver->BlitSprite(background, dp);
		}
	}
}

void View::Draw()
{
	if (flags&Invisible) return;

	const Region clip = VideoDriver->GetScreenClip();
	const Region& drawFrame = DrawingFrame();
	const Region& intersect = clip.Intersect(drawFrame);
	if (intersect.size.IsInvalid()) return; // outside the window/screen

	// clip drawing to the view bounds, then restore after drawing
	VideoDriver->SetScreenClip(&intersect);

	bool needsDraw = NeedsDraw(); // check this before WillDraw else an animation update might get missed
	// notify subclasses that drawing is about to happen. could pass the rects too, but no need ATM.
	WillDraw(drawFrame, intersect);

	if (needsDraw) {
		InvalidateSubviews();
		DrawBackground(nullptr);
		DrawSelf(drawFrame, intersect);
	}

	// always call draw on subviews because they can be dirty without us
	DrawSubviews(!needsDraw);
	DidDraw(drawFrame, intersect); // notify subclasses that drawing finished
	dirty = false;

	if (InDebugMode(DebugMode::VIEWS)) {
		const Window* win = GetWindow();
		if (win == nullptr) {
			VideoDriver->DrawRect(drawFrame, ColorBlue, false);
			debuginfo = EventMgr::ModState(GEM_MOD_SHIFT);
		} else if (NeedsDraw()) {
			VideoDriver->DrawRect(drawFrame, ColorRed, false);
		} else {
			VideoDriver->DrawRect(drawFrame, ColorGreen, false);
		}
		debuginfo = debuginfo || EventMgr::ModState(GEM_MOD_CTRL);

		if (debuginfo) {
			const ViewScriptingRef* ref = GetScriptingRef();
			if (ref) {
				const Font* fnt = core->GetTextFont();
				ScriptingId id = ref->Id;
				id &= 0x00000000ffffffff; // control id is lower 32bits

				std::string formatted = fmt::format("id: {}  grp: {}  \nflgs: {}\ntype:{}", id, ref->ScriptingGroup(), flags, typeid(*this).name());
				const String* string = StringFromCString(formatted.c_str());
				assert(string);
				
				Region r = drawFrame;
				r.w = win ? win->Frame().w - r.x : Frame().w - r.x;
				Font::StringSizeMetrics metrics = {r.size, 0, 0, true};
				fnt->StringSize(*string, &metrics);
				r.h = metrics.size.h;
				r.w = metrics.size.w;
				VideoDriver->SetScreenClip(nullptr);
				VideoDriver->DrawRect(r, ColorBlack, true);
				fnt->Print(r, *string, IE_FONT_ALIGN_TOP|IE_FONT_ALIGN_LEFT, {ColorWhite, ColorBlack});
				delete string;
			}
		}
	}

	// restore the screen clip
	VideoDriver->SetScreenClip(&clip);
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
	r.origin = ConvertPointToSuper(r.origin);
	return r;
}

Region View::ConvertRegionFromSuper(Region r) const
{
	r.origin = ConvertPointFromSuper(r.origin);
	return r;
}

Region View::ConvertRegionToWindow(Region r) const
{
	r.origin = ConvertPointToWindow(r.origin);
	return r;
}

Region View::ConvertRegionFromWindow(Region r) const
{
	r.origin = ConvertPointFromWindow(r.origin);
	return r;
}

Region View::ConvertRegionToScreen(Region r) const
{
	r.origin = ConvertPointToScreen(r.origin);
	return r;
}

Region View::ConvertRegionFromScreen(Region r) const
{
	r.origin = ConvertPointFromScreen(r.origin);
	return r;
}

void View::AddSubviewInFrontOfView(View* front, const View* back)
{
	if (front == nullptr) return;
	
	// remember, being "in front" means coming after in the list
	auto it = subViews.begin();
	if (back != nullptr) {
		it = std::find(subViews.begin(), subViews.end(), back);
		assert(it != subViews.end());
		++it;
	}

	const View* super = front->superView;
	if (super == this) {
		// already here, but may need to move the view
		auto cur = std::find(subViews.begin(), subViews.end(), front);
		subViews.splice(it, subViews, cur);
	} else {
		if (super != nullptr) {
			front->superView->RemoveSubview(front);
		}
		
		subViews.insert(it, front);
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

View* View::RemoveSubview(const View* view) noexcept
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
	MarkDirty();

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
	for (const auto& subView : subViews) {
		subView->AddedToWindow(newwin);
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

void View::RemovedFromView(const View*)
{
	window = nullptr;
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
	if (flags & (IgnoreEvents | Invisible)) {
		return false;
	}

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
	for (auto it = subViews.rbegin(); it != subViews.rend(); ++it) {
		View* v = *it;
		Point subP = v->ConvertPointFromSuper(p);
		if ((ignoreTransparency && v->frame.PointInside(p)) || v->HitTest(subP)) {
			if (recursive) {
				View* subV = v->SubviewAt(subP, ignoreTransparency, recursive);
				v = subV ? subV : v;
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

	for (const auto& subView : subViews) {
		if (subView == view) {
			return true;
		}
		if (subView->ContainsView(view)) {
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
		return win ? win : superView->GetWindow();
	}
	return NULL;
}

void View::ResizeSubviews(const Size& oldSize)
{
	for (const auto& subView : subViews) {
		unsigned short arFlags = subView->AutoResizeFlags();

		if (arFlags == ResizeNone)
			continue;

		Region newSubFrame = subView->Frame();
		int delta = frame.w - oldSize.w;

		if (arFlags & ResizeRight) {
			if (arFlags & ResizeLeft) {
				newSubFrame.w += delta;
			} else {
				newSubFrame.x += delta;
			}
		} else if (arFlags & ResizeLeft) {
			newSubFrame.x += delta;
		}

		delta = frame.h - oldSize.h;
		if (arFlags & ResizeBottom) {
			if (arFlags & ResizeTop) {
				newSubFrame.h += delta;
			} else {
				newSubFrame.y += delta;
			}
		} else if (arFlags & ResizeTop) {
			newSubFrame.y += delta;
		}

		subView->SetFrame(newSubFrame);
	}
	MarkDirty();
}

void View::SetFrame(const Region& r)
{
	SetFrameOrigin(r.origin);
	SetFrameSize(r.size);
}

void View::SetFrameOrigin(const Point& p)
{
	Point oldP = frame.origin;
	if (oldP == p) return;
	
	frame.origin = p;
	
	MarkDirty();
	OriginChanged(oldP);
}

void View::SetFrameSize(const Size& s)
{
	const Size oldSize = frame.size;
	if (oldSize == s) return;

	frame.w = std::max(0, s.w);
	frame.h = std::max(0, s.h);

	ResizeSubviews(oldSize);

	SizeChanged(oldSize);
}

bool View::SetFlags(unsigned int arg_flags, BitOp opcode)
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
	
bool View::SetAutoResizeFlags(unsigned short arg_flags, BitOp opcode)
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
	if (InDebugMode(DebugMode::VIEWS)) {
		debuginfo = true;
	}

	OnMouseEnter(me, op);
}

void View::MouseLeave(const MouseEvent& me, const DragOp* op)
{
	if (InDebugMode(DebugMode::VIEWS)) {
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
		VideoDriver->StartTextInput();
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

const ViewScriptingRef* View::ReplaceScriptingRef(const ViewScriptingRef* old, ScriptingId id, const ScriptingGroup_t& group)
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
		} else {
			delete newref;
		}
	}
	return nullptr;
}

const ViewScriptingRef* View::RemoveScriptingRef(const ViewScriptingRef* ref)
{
	static ScriptingId id = 0;
	return ReplaceScriptingRef(ref, id++, "__DEL__");
}
	
void View::ClearScriptingRefs() noexcept
{
	for (auto rit = scriptingRefs.begin(); rit != scriptingRefs.end();) {
		ViewScriptingRef* ref = *rit;
		assert(ref->GetObject() == this);
		bool unregistered = ScriptEngine::UnregisterScriptingRef(ref);
		assert(unregistered);
		delete ref;
		rit = scriptingRefs.erase(rit);
	}
}
	
ViewScriptingRef* View::CreateScriptingRef(ScriptingId id, ScriptingGroup_t group)
{
	return new ViewScriptingRef(this, id, group);
}
	
const ViewScriptingRef* View::AssignScriptingRef(ScriptingId id, const ScriptingGroup_t& group)
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

const ViewScriptingRef* View::GetScriptingRef(ScriptingId id, ScriptingGroup_t group) const
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
