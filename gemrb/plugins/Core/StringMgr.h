#ifndef STRINGMGR_H
#define STRINGMGR_H

#include "Plugin.h"
#include "DataStream.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT StringMgr : public Plugin
{
public:
	StringMgr(void);
	virtual ~StringMgr(void);
	virtual bool Open(DataStream * stream, bool autoFree = true) = 0;
	virtual char * GetString(unsigned long strref, int flag=0) = 0;
};

#endif
