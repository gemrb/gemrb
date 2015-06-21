/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2015 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef GemRB_GUIScriptInterface_h
#define GemRB_GUIScriptInterface_h

#include "Control.h"
#include "ScriptEngine.h"
#include "Window.h"

namespace GemRB {

class ViewScriptingRef : public ScriptingRef<View> {
private:
	ResRef group;
public:
	ViewScriptingRef(View* view, ScriptingId id, ResRef group)
	: ScriptingRef(view, id), group(group) {}

	const ResRef& ScriptingGroup() {
		return group;
	}

	// class to instantiate on the script side (Python)
	virtual const ScriptingClassId ScriptingClass() {
		return ScriptingGroup().CString();
	};

	// TODO: perhapps in the future the GUI script implementation for view methods should be moved here
};

class WindowScriptingRef : public ViewScriptingRef {
public:
	WindowScriptingRef(Window* win, ScriptingId id, ResRef winpack)
	: ViewScriptingRef(win, id, winpack) {}

	// class to instantiate on the script side (Python)
	virtual const ScriptingClassId ScriptingClass() {
		static ScriptingClassId cls("Window");
		return cls;
	};

	// TODO: perhapps in the future the GUI script implementation for window methods should be moved here
};

class ControlScriptingRef : public ViewScriptingRef {
public:
	ControlScriptingRef(Control* ctrl, ScriptingId id, ResRef group)
	: ViewScriptingRef(ctrl, id, group) {}

	// class to instantiate on the script side (Python)
	const ScriptingClassId ScriptingClass() {
		Control* ctrl = static_cast<Control*>(GetObject());

		// would just use type_info here, but its implementation specific...
		switch (ctrl->ControlType) {
			case IE_GUI_BUTTON:
				return "Button";
			case IE_GUI_LABEL:
				return "Label";
			case IE_GUI_TEXTAREA:
				return "TextArea";
			case IE_GUI_SCROLLBAR:
				return "ScrollBar";
			default:
				return "Control";
		}
	};

	// TODO: perhapps in the future the GUI script implementation for window methods should be moved here
};


View* GetView(ScriptingRefBase* base);
Window* GetWindow(ScriptingId id, ResRef pack);
Control* GetControl(ScriptingId id, Window* win);

template <class T>
T* GetControl(ScriptingId id, Window* win) {
	return dynamic_cast<T*>(GetControl(id, win));
}

ControlScriptingRef* GetControlRef(ScriptingId id, Window* win);
ControlScriptingRef* RegisterScriptableControl(Control* ctrl, ScriptingId id);
WindowScriptingRef* RegisterScriptableWindow(Window*, ResRef pack, ScriptingId id);

}

#endif
