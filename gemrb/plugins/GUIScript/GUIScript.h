/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef GUISCRIPT_H
#define GUISCRIPT_H

// NOTE: Python.h has to be included first.
#include <Python.h>
#include "ScriptEngine.h"

namespace GemRB {

class View;

enum {
   SV_BPP,
   SV_WIDTH,
   SV_HEIGHT,
   SV_GAMEPATH,
   SV_TOUCH,
   SV_SAVEPATH
};

class GUIScript : public ScriptEngine {
private:
	PyObject* pModule = nullptr; // should decref it
	PyObject* pDict = nullptr; // borrowed, but used outside a function
	PyObject* pMainDic = nullptr; // borrowed, but used outside a function
	PyObject* pGUIClasses = nullptr;

public:
	GUIScript(void);
	GUIScript(const GUIScript&) = delete;
	~GUIScript() override;
	GUIScript& operator=(const GUIScript&) = delete;
	/** Initialization Routine */
	bool Init(void) override;
	/** Autodetect GameType */
	bool Autodetect(void);
	/** Load Script */
	bool LoadScript(const std::string& filename) override;
	/** Run Function */
	Parameter RunFunction(const char* Modulename, const char* FunctionName, const FunctionParameters& params, bool report_error = true) override;

	PyObject *RunPyFunction(const char* moduleName, const char* fname, const FunctionParameters& params, bool report_error = true);
	PyObject *RunPyFunction(const char* moduleName, const char* fname, PyObject* pArgs, bool report_error = true);
	/** Exec a single File */
	bool ExecFile(const char* file);
	/** Exec a single String */
	bool ExecString(const std::string &string, bool feedback=false) override;

	PyObject* ConstructObjectForScriptable(const ScriptingRefBase*) const;
	PyObject* ConstructObject(const char* pyclassname, ScriptingId id) const;
	PyObject* ConstructObject(const char* pyclassname, PyObject* pArgs, PyObject* kwArgs = nullptr) const;
	
	const ScriptingRefBase* GetScriptingRef(PyObject* obj) const;
	void AssignViewAttributes(PyObject* obj, View* view) const;
};

extern GUIScript *gs;

}

#endif
