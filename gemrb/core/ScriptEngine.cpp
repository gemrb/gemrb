/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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
 *
 *
 */

#include "ScriptEngine.h"

namespace GemRB {

ScriptEngine::ScriptingDict ScriptEngine::GUIDict;

bool ScriptEngine::RegisterScriptable(ScriptingRef* ref)
{
	ScriptingId id = ref->id;
	ScriptingClassId classId = ref->ClassId();

	// TODO: disallow overwriting existing entries
	GUIDict[classId][id] = ref;
	return true;
}

bool ScriptEngine::UnRegisterScriptable(ScriptingRef* ref)
{
	ScriptingId id = ref->id;
	ScriptingClassId classId = ref->ClassId();

	// TODO: error if not found
	GUIDict[classId].erase(id);
	return true;
}

ScriptEngine::ScriptEngine(void)
{
}

ScriptEngine::~ScriptEngine(void)
{
}

}
