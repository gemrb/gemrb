// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef GUISCRIPT_H
#define GUISCRIPT_H

// NOTE: Python.h has to be included first.
#include "ScriptEngine.h"

#include <Python.h>

namespace GemRB {

class View;

enum {
	SV_BPP,
	SV_WIDTH,
	SV_HEIGHT,
	SV_GAMEPATH,
	SV_TOUCH,
	SV_SAVEPATH,
	SV_INICONF,
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

	PyObject* RunPyFunction(const char* moduleName, const char* fname, const FunctionParameters& params, bool report_error = true);
	PyObject* RunPyFunction(const char* moduleName, const char* fname, PyObject* pArgs, bool report_error = true);
	/** Exec a single File */
	bool ExecFile(const char* file);
	/** Exec a single String */
	bool ExecString(const std::string& string, bool feedback = false) override;

	PyObject* ConstructObjectForScriptable(const ScriptingRefBase*) const;
	PyObject* ConstructObject(const std::string& pyclassname, ScriptingId id) const;
	PyObject* ConstructObject(const std::string& pyclassname, PyObject* pArgs, PyObject* kwArgs = nullptr) const;

	const ScriptingRefBase* GetScriptingRef(PyObject* obj) const;
	void AssignViewAttributes(PyObject* obj, View* view) const;
};

extern GUIScript* gs;

}

#endif
