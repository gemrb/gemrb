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
	ScriptingGroup_t group;
public:
	ViewScriptingRef(View* view, ScriptingId id, ScriptingGroup_t group)
	: ScriptingRef(view, id), group(group) {}

	const ScriptingGroup_t& ScriptingGroup() const override {
		return group;
	}

	// class to instantiate on the script side (Python)
	ScriptingClassId ScriptingClass() const override {
		return {"View"};
	};

	virtual ViewScriptingRef* Clone(ScriptingId id, ScriptingGroup_t scriptingGroup) const {
		return new ViewScriptingRef(this->GetObject(), id, scriptingGroup);
	}

	// TODO: perhapps in the future the GUI script implementation for view methods should be moved here
};

class WindowScriptingRef : public ViewScriptingRef {
public:
	using RefType = Window*;
	
	WindowScriptingRef(Window* win, ScriptingId id, ScriptingGroup_t winpack)
	: ViewScriptingRef(win, id, winpack) {}

	// class to instantiate on the script side (Python)
	ScriptingClassId ScriptingClass() const override {
		static ScriptingClassId cls("Window");
		return cls;
	};

	ViewScriptingRef* Clone(ScriptingId id, ScriptingGroup_t group) const override {
		return new WindowScriptingRef(static_cast<Window*>(GetObject()), id, group);
	}
	
	Window* GetWindow() const noexcept {
		return static_cast<Window*>(GetObject());
	}
};

class ControlScriptingRef : public ViewScriptingRef {
public:
	using RefType = Control*;
	
	ControlScriptingRef(Control* ctrl, ScriptingId id, ScriptingGroup_t group)
	: ViewScriptingRef(ctrl, id, group) {}

	// class to instantiate on the script side (Python)
	ScriptingClassId ScriptingClass() const override {
		const Control* ctrl = static_cast<const Control*>(GetObject());

		// would just use type_info here, but its implementation specific...
		switch (ctrl->ControlType) {
			case IE_GUI_BUTTON:
				return "Button";
			case IE_GUI_PROGRESSBAR:
				return "ProgressBar";
			case IE_GUI_SLIDER:
				return "Slider";
			case IE_GUI_EDIT:
				return "TextEdit";
			// 4 - unused
			case IE_GUI_TEXTAREA:
				return "TextArea";
			case IE_GUI_LABEL:
				return "Label";
			case IE_GUI_SCROLLBAR:
				return "ScrollBar";
			case IE_GUI_WORLDMAP:
				return "WorldMap";
			case IE_GUI_MAP:
				return "Map";
			default:
				return "Control";
		}
	};

	ViewScriptingRef* Clone(ScriptingId id, ScriptingGroup_t group) const override {
		return new ControlScriptingRef(static_cast<Control*>(GetObject()), id, group);
	}
	
	Control* GetControl() const noexcept {
		return static_cast<Control*>(GetObject());
	}
};


Window* GetWindow(ScriptingId id, const ScriptingGroup_t& pack);
const WindowScriptingRef* RegisterScriptableWindow(Window*, const ScriptingGroup_t& pack, ScriptingId id);

GEM_EXPORT std::vector<View*> GetViews(const ScriptingGroup_t& pack);
GEM_EXPORT Control* GetControl(ScriptingId id, const Window* win);
GEM_EXPORT const ControlScriptingRef* GetControlRef(ScriptingId id, const Window* win);
GEM_EXPORT const ControlScriptingRef* RegisterScriptableControl(Control* ctrl, ScriptingId id, const ControlScriptingRef* existing = nullptr);

template <class T>
T* GetView(const ViewScriptingRef* ref) noexcept {
	if (ref) {
		return dynamic_cast<T*>(ref->GetObject());
	}
	return nullptr;
}

template <class T>
T* GetControl(ScriptingId id, const Window* win) {
	return dynamic_cast<T*>(GetControl(id, win));
}

template <class T>
T* GetControl(ScriptingGroup_t group, ScriptingId id) {
	auto ref = static_cast<const ControlScriptingRef*>(ScriptEngine::GetScriptingRef(group, id));
	return GetView<T>(ref);
}

}

#endif
