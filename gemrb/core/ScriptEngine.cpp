// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
