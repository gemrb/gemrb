// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * @file Window.h
 * Declares Window, class serving as a container for Control/widget objects
 * and displaying windows in GUI
 * @author The GemRB Project
 */

#ifndef WINDOW_H
#define WINDOW_H

#include "Control.h"
#include "ScrollView.h"

#include "Video/Video.h"

#include <set>

namespace GemRB {

class WindowManager;

/**
 * @class Window
 * Class serving as a container for Control/widget objects
 * and displaying windows in GUI.
 */

using WindowActionResponder = View::ActionResponder<Window*>;

class GEM_EXPORT Window : public ScrollView,
			  public WindowActionResponder {
public:
	// !!! Keep these synchronized with GUIDefines.py !!!
	enum WindowPosition {
		PosTop = 1, // top
		PosBottom = 2, // bottom
		PosVmid = 3, // top + bottom = vmid
		PosLeft = 4, // left
		PosRight = 8, // right
		PosHmid = 12, // left + right = hmid
		PosCentered = 15 // top + bottom + left + right = center
	};

	// Colors of modal window shadow
	// !!! Keep these synchronized with GUIDefines.py !!!
	enum class ModalShadow {
		None = 0,
		Gray,
		Black
	};
	ModalShadow modalShadow = ModalShadow::None;

	enum WindowFlags {
		Draggable = 1, // window can be dragged (by clicking anywhere in its frame) a la PST radial menu
		Borderless = 2, // window will not draw the ornate borders (see WindowManager::DrawWindows)
		DestroyOnClose = 4, // window will be deleted on close rather then hidden and sent to the back
		AlphaChannel = 8, // Create window with RGBA buffer suitable for creating non rectangular windows
		Modal = 16,
		NoSounds = 32 // doesn't play the open/close sounds
	};

	enum Action : WindowActionResponder::Action {
		// !!! Keep these synchronized with GUIDefines.py !!!
		Closed = 0,
		GainedFocus = 1,
		LostFocus = 2
	};

	using WindowEventHandler = WindowActionResponder::Responder;

private:
	void RecreateBuffer();

	void SubviewAdded(View* view, View* parent) override;
	void SubviewRemoved(View* view, View* parent) override;

	void FlagsChanged(unsigned int /*oldflags*/) override;
	void SizeChanged(const Size&) override;

	void WillDraw(const Region& /*drawFrame*/, const Region& /*clip*/) override;
	void DidDraw(const Region& /*drawFrame*/, const Region& /*clip*/) override;
	void DrawAfterSubviews(const Region& /*drawFrame*/, const Region& /*clip*/) override;

	// attempt to set focus to view. return the focused view which is view if success or the currently focused view (if any) on failure
	View* TrySetFocus(View* view);
	bool IsDraggable() const;

	inline void DispatchMouseMotion(View*, const MouseEvent&);
	inline void DispatchMouseDown(View*, const MouseEvent& me, unsigned short /*Mod*/);
	inline void DispatchMouseUp(View*, const MouseEvent& me, unsigned short /*Mod*/);

	inline void DispatchTouchDown(View*, const TouchEvent& te, unsigned short /*Mod*/);
	inline void DispatchTouchUp(View*, const TouchEvent& te, unsigned short /*Mod*/);
	inline void DispatchTouchGesture(View*, const GestureEvent& gesture);

	inline bool DispatchKey(View*, const Event&);

	bool OnKeyPress(const KeyboardEvent& /*Key*/, unsigned short /*Mod*/) override;

	bool OnMouseDrag(const MouseEvent&) override;
	void OnMouseLeave(const MouseEvent& /*me*/, const DragOp*) override;

	bool OnControllerButtonDown(const ControllerEvent& ce) override;

	ViewScriptingRef* CreateScriptingRef(ScriptingId id, ScriptingGroup_t group) override;

public:
	Window(const Region& frame, WindowManager& mgr);

	void Close();
	void Focus();
	bool DisplayModal(ModalShadow = ModalShadow::None);

	void FocusLost();
	void FocusGained();
	bool HasFocus() const;

	/** Sets 'view' as Focused */
	void SetFocused(View* view);
	View* FocusedView() const { return focusView; }

	void SetPosition(WindowPosition);
	String TooltipText() const override;
	Holder<Sprite2D> Cursor() const override;
	bool IsDisabledCursor() const override;
	bool IsReceivingEvents() const override;

	const VideoBufferPtr& DrawWithoutComposition();
	void RedrawControls(const Control::varname_t& VarName) const;

	bool DispatchEvent(const Event&);
	bool RegisterHotKeyCallback(EventMgr::EventCallback, KeyboardKey key);
	bool UnRegisterHotKeyCallback(const EventMgr::EventCallback&, KeyboardKey key);

	bool IsOpaque() const override { return (Flags() & AlphaChannel) == 0; }
	bool HitTest(const Point& p) const override;

	bool InActionHandler() const;
	void SetAction(Responder handler, const ActionKey& key) override;
	bool PerformAction(const ActionKey& action) override;
	bool SupportsAction(const ActionKey& action) override;

	void DropScaleBuffer();
	void UseScaleBuffer(unsigned int percent, bool force = false);

private: // Private attributes
	using KeyMap = std::map<KeyboardKey, EventMgr::EventCallback>;

	std::set<Control*> Controls;
	KeyMap HotKeys;

	View* focusView = nullptr; // keyboard focus
	View* trackingView = nullptr; // out of bounds mouse tracking
	View* hoverView = nullptr; // view the mouse was last over

	Point dragOrigin;
	UniqueDragOp drag;
	tick_t lastMouseMoveTime;

	VideoBufferPtr backBuffer = nullptr;
	VideoBufferPtr scaleBuffer = nullptr;
	unsigned int scale = 100;
	WindowManager& manager;

	WindowEventHandler eventHandlers[3];
};

}

#endif
