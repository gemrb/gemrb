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

#include "GUIScriptInterface.h"

#include "Resource.h"

namespace GemRB {

static inline ScriptingId ModifiedCtrlIdForWin(ScriptingId id, const WindowScriptingRef* winref)
{
	if (winref) {
		id &= 0x00000000ffffffff; // control id is lower 32bits
		id |= (winref->Id << 32); // window id will be stored in upper 32 bits, but only uses 16
		id |= 0x8000000000000000; // MSB signifies this is a control id (so that win id 0 and ctrl 0 on that win have distinct ids)
	}
	return id;
}

const ControlScriptingRef* GetControlRef(ScriptingId id, const Window* win)
{
	ScriptingGroup_t group = "Control";
	if (win) {
		const WindowScriptingRef* winref = static_cast<const WindowScriptingRef*>(win->GetScriptingRef());
		if (winref) {
			group = winref->ScriptingGroup();
			id = ModifiedCtrlIdForWin(id, winref);
		}
	}
	const ScriptingRefBase* base = ScriptEngine::GetScriptingRef(group, id);
	return static_cast<const ControlScriptingRef*>(base);
}

Control* GetControl(ScriptingId id, const Window* win)
{
	const ControlScriptingRef* ref = GetControlRef(id, win);
	return ScriptingRefCast<Control>(ref);
}

Window* GetWindow(ScriptingId id, const ScriptingGroup_t& pack)
{
	const WindowScriptingRef* ref = dynamic_cast<const WindowScriptingRef*>(ScriptEngine::GetScriptingRef(pack, id));
	return ScriptingRefCast<Window>(ref);
}

std::vector<View*> GetViews(const ScriptingGroup_t& group)
{
	ScriptEngine::ScriptingDefinitions defs = ScriptEngine::GetScriptingGroup(group);
	std::vector<View*> views;

	for (const auto& def : defs) {
		const ViewScriptingRef* ref = static_cast<const ViewScriptingRef*>(def.second);
		views.push_back(ref->GetObject());
	}

	return views;
}

const ControlScriptingRef* RegisterScriptableControl(Control* ctrl, ScriptingId id, const ControlScriptingRef* existing)
{
	if (!ctrl) return NULL;

	ScriptingGroup_t group = "Control";
	const Window* win = ctrl->GetWindow();
	if (win) {
		const WindowScriptingRef* winref = static_cast<const WindowScriptingRef*>(win->GetScriptingRef());
		if (winref) {
			id = ModifiedCtrlIdForWin(id, winref);
			group = winref->ScriptingGroup();
		}
	}

	ctrl->ControlID = (ieDword)id;

	if (existing) {
		return static_cast<const ControlScriptingRef*>(ctrl->ReplaceScriptingRef(existing, id, group));
	} else {
		return static_cast<const ControlScriptingRef*>(ctrl->AssignScriptingRef(id, group));
	}
}

const WindowScriptingRef* RegisterScriptableWindow(Window* win, const ScriptingGroup_t& pack, ScriptingId id)
{
	if (!win) return NULL;
	assert(win->GetScriptingRef() == NULL);

	return static_cast<const WindowScriptingRef*>(win->AssignScriptingRef(id, pack));
}

}
