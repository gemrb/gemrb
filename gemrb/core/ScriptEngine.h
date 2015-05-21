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

#include <map>
#include <string>

namespace GemRB {

class Point;
class ScriptingRef;

typedef unsigned long ScriptingId;
typedef const std::string& ScriptingClassId;

class GEM_EXPORT ScriptEngine : public Plugin {
private:
	typedef std::map<ScriptingId, ScriptingRef*> ScriptingDefinitions;
	typedef std::map<std::string, ScriptingDefinitions> ScriptingDict;
	static ScriptingDict GUIDict;

public:
	static bool RegisterScriptable(ScriptingRef* ref);
	static bool UnRegisterScriptable(ScriptingRef* ref);

public:
	ScriptEngine(void);
	virtual ~ScriptEngine(void);
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

class ScriptingRef {
public:
	const ScriptingId id;

	ScriptingRef(ScriptingId id)
	: id(id)
	{
		ScriptEngine::RegisterScriptable(this);
	}
	virtual ~ScriptingRef()
	{
		ScriptEngine::UnRegisterScriptable(this);
	}

	virtual ScriptingClassId ClassId()=0;
};

template <class T>
class ScriptingObject : public ScriptingRef {
	ScriptingClassId classId;
	T& obj;
public:
	ScriptingObject(T& obj, ScriptingClassId classId, ScriptingId id)
	: ScriptingRef(id), classId(classId), obj(obj) {}
	T& Object() { return obj; }

	ScriptingClassId ClassId() { return classId; }
};

}

#endif
