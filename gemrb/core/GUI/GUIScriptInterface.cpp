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

namespace GemRB {

View* GetView(ScriptingRefBase* base)
{
	ViewScriptingRef* ref = dynamic_cast<ViewScriptingRef*>(base);
	if (ref) {
		return ref->GetObject();
	}
	return NULL;
}

ControlScriptingRef* GetControlRef(ScriptingId id, Window* win)
{
	char group[8] = "Control";
	if (win) {
		snprintf(group, sizeof(group), "Win%02d", win->WindowID);
	}
	ScriptingRefBase* base = ScriptEngine::GetScripingRef(group, id);
	return dynamic_cast<ControlScriptingRef*>(base);
}

Control* GetControl(ScriptingId id, Window* win)
{
	View* view = GetView( GetControlRef(id, win) );
	return static_cast<Control*>(view);
}

Window* GetWindow(ScriptingId id)
{
	View* view = GetView( ScriptEngine::GetScripingRef("Window", id) );
	return static_cast<Window*>(view);
}

}
