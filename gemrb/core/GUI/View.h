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

#ifndef __GemRB__View__
#define __GemRB__View__

#include "globals.h"
#include "GUI/EventMgr.h"
#include "Holder.h"
#include "Region.h"
#include "ScriptEngine.h"

#include <list>
#include <memory>
#include <vector>

namespace GemRB {

class Sprite2D;
class ViewScriptingRef;
class Window;

class GEM_EXPORT View {
public:
	struct GEM_EXPORT DragOp {
		View* dragView = nullptr;
		View* dropView = nullptr;
		
		Holder<Sprite2D> cursor;
		
		DragOp(View* v, Holder<Sprite2D> cursor);
		virtual ~DragOp();
	};
	
	using UniqueDragOp = std::unique_ptr<DragOp>;
	
	enum AutoresizeFlags {
		// when a superview frame changes...
		ResizeNone = 0,
		ResizeTop = 1 << 0, // keep my top relative to my super
		ResizeBottom = 1 << 1, // keep my bottom relative to my super
		ResizeVertical = ResizeTop|ResizeBottom, // top+bottom effectively resizes me vertically
		ResizeLeft = 1 << 3, // keep my left relative to my super
		ResizeRight = 1 << 4, // keep my right relative to my super
		ResizeHorizontal = ResizeLeft|ResizeRight, // top+bottom effectively resizes me horizontaly
		ResizeAll = ResizeVertical|ResizeHorizontal, // resize me relative to my super
		
		// TODO: move these to TextContainer
		RESIZE_WIDTH = 1 << 27,		// resize the view horizontally if horizontal content exceeds width
		RESIZE_HEIGHT = 1 << 26	// resize the view vertically if vertical content exceeds width
	};
	
	enum ViewFlags {
		Invisible = 1 << 30,
		Disabled = 1 << 29,
		IgnoreEvents = 1 << 28
	};
private:
	Color backgroundColor;
	Holder<Sprite2D> background;
	Holder<Sprite2D> cursor = nullptr;
	std::vector<ViewScriptingRef*> scriptingRefs;

	mutable bool dirty = true;
	
	View* eventProxy = nullptr;

protected:
	View* superView = nullptr;
	// for convenience because we need to get this so much
	// all it is is a saved pointer returned from View::GetWindow and is updated in AddedToView
	Window* window = nullptr;

	Region frame;
	std::list<View*> subViews;
	String tooltip;

	// Flags: top byte is reserved for View flags, subclasses may use the remaining bits however they want
	unsigned int flags = 0;
	unsigned short autoresizeFlags = ResizeNone; // these flags don't produce notifications

private:
	Regions DirtySuperViewRegions() const;
	void DrawBackground(const Region*) const;
	bool HasBackground() const;
	void DrawSubviews() const;
	void MarkDirty(const Region*);
	void InvalidateSubviews(const Region& rgn) const;
	void InvalidateDirtySubviewRegions();

	// TODO: to support partial redraws, we should change the clip parameter to a list of dirty rects
	// that have all been clipped to the video ScreenClip
	// subclasses can then use the list to efficiently redraw only those sections that are dirty
	virtual void DrawSelf(const Region& /*drawFrame*/, const Region& /*clip*/) {};
	Region DrawingFrame() const;

	void AddedToWindow(Window*);
	void AddedToView(View*);
	void RemovedFromView(const View*);
	virtual void SubviewAdded(View* /*view*/, View* /*parent*/) {};
	virtual void SubviewRemoved(View* /*view*/, View* /*parent*/) {};

	// notifications
	virtual void FlagsChanged(unsigned int /*oldflags*/) {}
	virtual void SizeChanged(const Size&) {}
	virtual void OriginChanged(const Point&) {}
	
	// See Draw() for restrictions
	virtual void WillDraw(const Region& /*drawFrame*/, const Region& /*clip*/) {}
	// See Draw() for restrictions
	virtual void DidDraw(const Region& /*drawFrame*/, const Region& /*clip*/) {}
	
	virtual bool IsPerPixelScrollable() const { return true; }
	
	virtual ViewScriptingRef* CreateScriptingRef(ScriptingId id, ScriptingGroup_t group);

protected:
	void ClearScriptingRefs() noexcept;

	void ResizeSubviews(const Size& oldsize);
	
	// these events make no sense to forward
	virtual void OnMouseEnter(const MouseEvent& /*me*/, const DragOp*) {}
	virtual void OnMouseLeave(const MouseEvent& /*me*/, const DragOp*) {}
	virtual void OnTextInput(const TextEvent& /*te*/) {}
	
	// default view implementation does nothing but ignore the event
	virtual bool OnKeyPress(const KeyboardEvent& /*Key*/, unsigned short /*Mod*/) { return false; }
	virtual bool OnKeyRelease(const KeyboardEvent& /*Key*/, unsigned short /*Mod*/) { return false; }
	virtual bool OnMouseOver(const MouseEvent& /*me*/) { return false; }
	virtual bool OnMouseDrag(const MouseEvent& /*me*/) { return TracksMouseDown(); }
	virtual bool OnMouseDown(const MouseEvent& /*me*/, unsigned short /*Mod*/) { return false; }
	virtual bool OnMouseUp(const MouseEvent& /*me*/, unsigned short /*Mod*/) { return false; }
	virtual bool OnMouseWheelScroll(const Point&) { return false; }

	virtual bool OnTouchDown(const TouchEvent& /*te*/, unsigned short /*Mod*/);
	virtual bool OnTouchUp(const TouchEvent& /*te*/, unsigned short /*Mod*/);
	virtual bool OnTouchGesture(const GestureEvent& gesture);
	
	virtual bool OnControllerAxis(const ControllerEvent&);
	virtual bool OnControllerButtonDown(const ControllerEvent&);
	virtual bool OnControllerButtonUp(const ControllerEvent&);
	
	virtual bool IsAnimated() const { return false; }
	virtual bool IsOpaque() const;

public:
	#include "ViewInterfaces.h"

	bool debuginfo = false;

	explicit View(const Region& frame);
	virtual ~View();

	// IMPORTANT: when Draw() is being executed MarkDirty() is a no-op.
	// this includes methods invoked prior to and following the actual drawing such as WillDraw() and DidDraw()
	// continually drawing Views should use an IsAnimated() override
	void Draw();

	void MarkDirty();
	bool NeedsDraw() const;

	virtual bool HitTest(const Point& p) const;

	bool SetFlags(unsigned int arg_flags, BitOp opcode);
	inline unsigned int Flags() const { return flags; }
	bool SetAutoResizeFlags(unsigned short arg_flags, BitOp opcode);
	unsigned short AutoResizeFlags() const { return autoresizeFlags; }

	void SetVisible(bool vis) { SetFlags(Invisible, vis ? BitOp::NAND : BitOp::OR); }
	bool IsVisible() const;
	void SetDisabled(bool disable) { SetFlags(Disabled, disable ? BitOp::OR : BitOp::NAND); }
	bool IsDisabled() const { return flags&Disabled; }
	virtual bool IsDisabledCursor() const;
	virtual bool IsReceivingEvents() const;

	Region Frame() const { return frame; }
	Point Origin() const { return frame.origin; }
	Size Dimensions() const { return frame.size; }
	void SetFrame(const Region& r);
	void SetFrameOrigin(const Point&);
	void SetFrameSize(const Size&);
	void SetBackground(Holder<Sprite2D>, const Color* = nullptr);

	// FIXME: I don't think I like this being virtual. Currently required because ScrollView is "overriding" this
	// we perhapps should instead have ScrollView implement SubviewAdded and move the view to its contentView there
	virtual void AddSubviewInFrontOfView(View*, const View* = NULL);
	View* RemoveSubview(const View*) noexcept;
	View* RemoveFromSuperview();
	View* SubviewAt(const Point&, bool ignoreTransparency = false, bool recursive = false);
	Window* GetWindow() const;
	bool ContainsView(const View* view) const;

	Point ConvertPointToSuper(const Point&) const;
	Point ConvertPointFromSuper(const Point&) const;
	Point ConvertPointToWindow(const Point&) const;
	Point ConvertPointFromWindow(const Point&) const;
	Point ConvertPointToScreen(const Point&) const;
	Point ConvertPointFromScreen(const Point&) const;
	
	Region ConvertRegionToSuper(Region) const;
	Region ConvertRegionFromSuper(Region) const;
	Region ConvertRegionToWindow(Region) const;
	Region ConvertRegionFromWindow(Region) const;
	Region ConvertRegionToScreen(Region) const;
	Region ConvertRegionFromScreen(Region) const;

	virtual bool CanLockFocus() const { return (flags&(IgnoreEvents|Disabled)) == 0; };
	virtual bool CanUnlockFocus() const { return true; };
	virtual void DidFocus() {}
	virtual void DidUnFocus() {}

	virtual bool TracksMouseDown() const { return false; }

	virtual UniqueDragOp DragOperation() { return nullptr; }
	virtual bool AcceptsDragOperation(const DragOp&) const { return false; }
	virtual void CompleteDragOperation(const DragOp&) {}

	bool KeyPress(const KeyboardEvent& /*Key*/, unsigned short /*Mod*/);
	bool KeyRelease(const KeyboardEvent& /*Key*/, unsigned short /*Mod*/);

	void TextInput(const TextEvent& /*te*/);

	void MouseEnter(const MouseEvent& /*me*/, const DragOp*);
	void MouseLeave(const MouseEvent& /*me*/, const DragOp*);
	void MouseOver(const MouseEvent& /*me*/);
	void MouseDrag(const MouseEvent& /*me*/);
	void MouseDown(const MouseEvent& /*me*/, unsigned short /*Mod*/);
	void MouseUp(const MouseEvent& /*me*/, unsigned short /*Mod*/);
	void MouseWheelScroll(const Point&);

	void TouchDown(const TouchEvent& /*te*/, unsigned short /*Mod*/);
	void TouchUp(const TouchEvent& /*te*/, unsigned short /*Mod*/);
	void TouchGesture(const GestureEvent& gesture);
	
	void ControllerAxis(const ControllerEvent&);
	void ControllerButtonDown(const ControllerEvent&);
	void ControllerButtonUp(const ControllerEvent&);

	void SetTooltip(const String& string);
	virtual String TooltipText() const { return tooltip; }
	/* override the standard cursors. default does not override (returns NULL). */
	virtual Holder<Sprite2D> Cursor() const { return cursor; }
	void SetCursor(Holder<Sprite2D> c);
	void SetEventProxy(View* proxy);

	// GUIScripting
	const ViewScriptingRef* AssignScriptingRef(ScriptingId id, const ScriptingGroup_t& group);
	const ViewScriptingRef* ReplaceScriptingRef(const ViewScriptingRef* old, ScriptingId id, const ScriptingGroup_t& group);
	const ViewScriptingRef* RemoveScriptingRef(const ViewScriptingRef*);
	const ViewScriptingRef* GetScriptingRef() const;
	const ViewScriptingRef* GetScriptingRef(ScriptingId id, ScriptingGroup_t group) const;
};

}

#endif /* defined(__GemRB__View__) */
