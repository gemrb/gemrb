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

	const ResRef& ScriptingGroup() const override {
		return group;
	}

	// class to instantiate on the script side (Python)
	const ScriptingClassId ScriptingClass() const override {
		return {"View"};
	};

	virtual ViewScriptingRef* Clone(ScriptingId id, ResRef group) const {
		return new ViewScriptingRef(this->GetObject(), id, group);
	}

	// TODO: perhapps in the future the GUI script implementation for view methods should be moved here
};

class WindowScriptingRef : public ViewScriptingRef {
public:
	typedef Window* RefType;
	
	WindowScriptingRef(Window* win, ScriptingId id, ResRef winpack)
	: ViewScriptingRef(win, id, winpack) {}

	// class to instantiate on the script side (Python)
	const ScriptingClassId ScriptingClass() const override {
		static ScriptingClassId cls("Window");
		return cls;
	};

	ViewScriptingRef* Clone(ScriptingId id, ResRef group) const override {
		return new WindowScriptingRef(static_cast<Window*>(GetObject()), id, group);
	}
};

class ControlScriptingRef : public ViewScriptingRef {
public:
	typedef Control* RefType;
	
	ControlScriptingRef(Control* ctrl, ScriptingId id, ResRef group)
	: ViewScriptingRef(ctrl, id, group) {}

	// class to instantiate on the script side (Python)
	const ScriptingClassId ScriptingClass() const override {
		Control* ctrl = static_cast<Control*>(GetObject());

		// would just use type_info here, but its implementation specific...
		switch (ctrl->ControlType) {
			case IE_GUI_BUTTON:
				return "Button";
			case IE_GUI_LABEL:
				return "Label";
			case IE_GUI_EDIT:
				return "TextEdit";
			case IE_GUI_TEXTAREA:
				return "TextArea";
			case IE_GUI_SCROLLBAR:
				return "ScrollBar";
			case IE_GUI_WORLDMAP:
				return "WorldMap";
			default:
				return "Control";
		}
	};

	ViewScriptingRef* Clone(ScriptingId id, ResRef group) const override {
		return new ControlScriptingRef(static_cast<Control*>(GetObject()), id, group);
	}
};


Window* GetWindow(ScriptingId id, ResRef pack);
const WindowScriptingRef* RegisterScriptableWindow(Window*, ResRef pack, ScriptingId id);

GEM_EXPORT View* GetView(const ScriptingRefBase* base);
GEM_EXPORT std::vector<View*> GetViews(ResRef pack);
GEM_EXPORT Control* GetControl(ScriptingId id, Window* win);
GEM_EXPORT const ControlScriptingRef* GetControlRef(ScriptingId id, Window* win);
GEM_EXPORT const ControlScriptingRef* RegisterScriptableControl(Control* ctrl, ScriptingId id, const ControlScriptingRef* existing = nullptr);

template <class T>
T* GetControl(ScriptingId id, Window* win) {
	return dynamic_cast<T*>(GetControl(id, win));
}

template <class T>
T* GetControl(ResRef group, ScriptingId id) {
	const ControlScriptingRef* ref = static_cast<const ControlScriptingRef*>(ScriptEngine::GetScripingRef(group, id));
	if (ref) {
		return dynamic_cast<T*>(ref->GetObject());
	}
	return NULL;
}

}

#endif
