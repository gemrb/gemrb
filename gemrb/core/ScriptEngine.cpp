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

bool ScriptEngine::RegisterScriptingRef(const ScriptingRefBase* ref)
{
	if (ref == NULL) return false;

	ScriptEngine::ScriptingDict::iterator it = GUIDict.find(ref->ScriptingGroup());
	if (it != GUIDict.end()) {
		if (it->second.count(ref->Id)) {
			return false;
		}
	}

	GUIDict[ref->ScriptingGroup()][ref->Id] = ref;
	return true;
}

bool ScriptEngine::UnregisterScriptingRef(const ScriptingRefBase* ref)
{
	if (ref == NULL) return false;

	ScriptEngine::ScriptingDict::iterator it = GUIDict.find(ref->ScriptingGroup());
	if (it == GUIDict.end()) {
		return false;
	}
	return (it->second.erase(ref->Id) > 0) ? true : false;
}

ScriptEngine::Parameter ScriptEngine::RunFunction(const char* Modulename, const char* FunctionName, bool report_error)
{
	return RunFunction(Modulename, FunctionName, FunctionParameters {}, report_error);
}

}
