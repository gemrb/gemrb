#ifndef GUISCRIPT_H
#define GUISCRIPT_H

#include "../Core/ScriptEngine.h"

#ifdef _DEBUG
#undef _DEBUG
#include "Python.h"
#define _DEBUG
#else
#include "Python.h"
#endif

class GUIScript : public ScriptEngine
{
private:
	PyObject *pName, *pModule, *pDict;
	PyObject *pGemRB, *pGemRBDict;
public:
	GUIScript(void);
	~GUIScript(void);
	/** Initialization Routine */
	bool Init(void);
	/** Load Script */
	bool LoadScript(const char * filename);
	/** Run Function */
	bool RunFunction(const char * fname);
};

#endif
