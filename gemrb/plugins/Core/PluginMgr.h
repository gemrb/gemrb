#ifndef PLUGINMGR_H
#define PLUGINMGR_H

#include "../../includes/win32def.h"
#include "../../includes/globals.h"
#include <vector>

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT PluginMgr
{
public:
	PluginMgr(char * pluginpath);
	~PluginMgr(void);
private:
	std::vector<ClassDesc*> plugins;
public:
	bool IsAvailable(SClass_ID plugintype);
	void * GetPlugin(SClass_ID plugintype);
	void FreePlugin(void * ptr);
};

#endif
