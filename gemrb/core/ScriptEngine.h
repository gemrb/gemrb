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

#ifndef SCRIPTENGINE_H
#define SCRIPTENGINE_H

#include "Plugin.h"
#include "System/Logging.h"

#include <map>
#include <string>

namespace GemRB {

class Point;

typedef unsigned long ScriptingId;
typedef std::string ScriptingClassId;

class ScriptingRefBase {
public:
	const ScriptingId Id; // unique id for each object in a ScriptingGroup

	ScriptingRefBase(ScriptingId id)
	: Id(id) {}

	virtual ~ScriptingRefBase() {}

	// key to separate groups of objects for faster searching and id collision prevention
	virtual const std::string& ScriptingGroup()=0;
	// class to instantiate on the script side (Python)
	virtual const ScriptingClassId ScriptingClass()=0;
};

template <class T>
class ScriptingRef : public ScriptingRefBase {
private:
	T* ref;
public:
	ScriptingRef(T* ref, ScriptingId id)
	: ScriptingRefBase(id), ref(ref) {}

	T* GetObject() { return ref; }
};


class GEM_EXPORT ScriptEngine : public Plugin {
private:
	typedef std::map<ScriptingId, ScriptingRefBase*> ScriptingDefinitions;
	typedef std::map<std::string, ScriptingDefinitions> ScriptingDict;
	static ScriptingDict GUIDict;

public:
	static bool RegisterScriptingRef(ScriptingRefBase* ref);
	static bool UnregisterScriptingRef(ScriptingRefBase* ref);

	static ScriptingRefBase* GetScripingRef(ScriptingClassId classId, ScriptingId id)
	{
		ScriptingRefBase* ref = NULL;
		ScriptingDefinitions::iterator it = GUIDict[classId].find(id);
		if (it != GUIDict[classId].end()) {
			ref = (*it).second;
		}
		return ref;
	}

	static const ScriptingId InvalidId = static_cast<ScriptingId>(-1);

public:
	ScriptEngine(void) {};
	virtual ~ScriptEngine(void) {};
	/** Initialization Routine */
	virtual bool Init(void) = 0;
	/** Load Script */
	virtual bool LoadScript(const char* filename) = 0;
	/** Run Function */
	virtual bool RunFunction(const char *ModuleName, const char* FunctionName, bool report_error=true, int intparam=-1) = 0;
	virtual bool RunFunction(const char* Modulename, const char* FunctionName, bool report_error, Point) = 0;
	/** Exec a single String */
	virtual void ExecString(const char* string, bool feedback) = 0;
};

}

#endif
