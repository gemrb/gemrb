#ifndef SCRIPTENGINE_H
#define SCRIPTENGINE_H

#include "Plugin.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT ScriptEngine : public Plugin
{
public:
	ScriptEngine(void);
	~ScriptEngine(void);
	/** Initialization Routine */
	virtual bool Init(void) = 0;
	/** Load Script */
	virtual bool LoadScript(const char * filename) = 0;
	/** Run Function */
	virtual bool RunFunction(const char * fname) = 0;
};

#endif
