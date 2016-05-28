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
#include "Region.h"
#include "ScriptEngine.h"

#include <list>

namespace GemRB {

class Sprite2D;
class ViewScriptingRef;

class GEM_EXPORT View {
private:
	Sprite2D* background;
	Sprite2D* cursor;
	ViewScriptingRef* scriptingRef;

	mutable bool dirty;

	// TODO: we could/should generalize this
	// MarkDirty could take a region, and more complicated views could potentially
	// save a lot of drawing time by only drawing their dirty portions (GameControl?)
	Regions dirtyBGRects;

protected:
	View* superView;
	Region frame;
	std::list<View*> subViews;
	String tooltip;

	// Flags: top byte is reserved for View flags, subclasses may use the remaining bits however they want
	unsigned int flags;

private:
	void DirtyBGRect(const Region&);
	void DrawBackground(const Region*) const;
	void DrawSubviews() const;

	virtual void DrawSelf(Region /*drawFrame*/, const Region& /*clip*/) {};

	virtual void AddedToView(View*) {}
	virtual void RemovedFromView(View*) {}
	virtual void SubviewAdded(View* view, View* parent);
	virtual void SubviewRemoved(View* view, View* parent);

	// notifications
	// TODO: RESIZE_WIDTH, RESIZE_HEIGHT, and RESIZE_SUBVIEWS should all do something on change
	virtual void FlagsChanged(unsigned int /*oldflags*/) {}
	virtual void SizeChanged(const Size&) {}
	virtual void WillDraw() {}

public:
	// using Held so we can have polymorphic drag operations
	struct DragOp : public Held<DragOp> {
		View* dragView;

		DragOp(View* v);
		virtual ~DragOp();
	};

	enum AutoresizeFlags {
		RESIZE_WIDTH = 1 << 29,		// resize the view horizontally if horizontal content exceeds width
		RESIZE_HEIGHT = 1 << 30,	// resize the view vertically if vertical content exceeds width

		RESIZE_SUBVIEWS = 1 << 31	// resize immidiate subviews by the same ammount as this views frame change
	};

	enum ViewFlags {
		Invisible = 1 << 28,
		Disabled = 1 << 27
	};

	View(const Region& frame);
	virtual ~View();

	void Draw();

	void MarkDirty();
	bool NeedsDraw() const;

	virtual bool IsAnimated() const { return false; }
	virtual bool IsOpaque() const { return background != NULL; }
	virtual bool EventHit(const Point& p) const;

	bool SetFlags(unsigned int arg_flags, int opcode);
	inline unsigned int Flags() { return flags; }

	void SetVisible(bool vis) { SetFlags(Invisible, (vis) ? OP_NAND : OP_OR ); }
	bool IsVisible() const;
	void SetDisabled(bool disable) { SetFlags(Disabled, (disable) ? OP_OR : OP_NAND); }
	bool IsDisabled() const { return flags&Disabled; }
	virtual bool IsDisabledCursor() const { return IsDisabled(); }

	Region Frame() const { return frame; }
	Point Origin() const { return frame.Origin(); }
	Size Dimensions() const { return frame.Dimensions(); }
	void SetFrame(const Region& r);
	void SetFrameOrigin(const Point&);
	void SetFrameSize(const Size&);
	void SetBackground(Sprite2D*);

	void AddSubviewInFrontOfView(View*, const View* = NULL);
	View* RemoveSubview(const View*);
	View* SubviewAt(const Point&, bool ignoreTransparency = false, bool recursive = false);

	Point ConvertPointToSuper(const Point&) const;
	Point ConvertPointFromSuper(const Point&) const;
	Point ConvertPointToWindow(const Point&) const;
	Point ConvertPointFromWindow(const Point&) const;
	Point ConvertPointToScreen(const Point&) const;
	Point ConvertPointFromScreen(const Point&) const;

	virtual bool CanLockFocus() const { return true; };
	virtual bool CanUnlockFocus() const { return true; };
	virtual bool TracksMouseDown() const { return false; }

	virtual Holder<DragOp> DragOperation() { return Holder<DragOp>(NULL); }
	virtual bool AcceptsDragOperation(const DragOp&) const { return false; }
	virtual void CompleteDragOperation(const DragOp&) {}

	virtual bool OnKeyPress(const KeyboardEvent& /*Key*/, unsigned short /*Mod*/) { return false; };
	virtual bool OnKeyRelease(const KeyboardEvent& /*Key*/, unsigned short /*Mod*/) { return false; };
	virtual void OnMouseEnter(const MouseEvent& /*me*/, const DragOp*) {};
	virtual void OnMouseLeave(const MouseEvent& /*me*/, const DragOp*) {};
	virtual void OnMouseOver(const MouseEvent& /*me*/);
	virtual void OnMouseDown(const Point&, unsigned short /*Button*/, unsigned short /*Mod*/);
	virtual void OnMouseUp(const Point&, unsigned short /*Button*/, unsigned short /*Mod*/);
	virtual void OnMouseWheelScroll(const Point&);

	void SetTooltip(const String& string);
	virtual String TooltipText() const { return tooltip; }
	/* override the standard cursors. default does not override (returns NULL). */
	virtual Sprite2D* Cursor() const { return cursor; }
	void SetCursor(Sprite2D* c);

	// GUIScripting
	void AssignScriptingRef(ViewScriptingRef* ref);
	ViewScriptingRef* GetScriptingRef() { return scriptingRef; }
	void DeleteScriptingRef();
};

}

#endif /* defined(__GemRB__View__) */
