#ifndef RESOURCEMGR_H
#define RESOURCEMGR_H

#include "Plugin.h"
#include "../../includes/SClassID.h"
#include "DataStream.h"
#include "../../includes/globals.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT ResourceMgr : public Plugin
{
public:
	ResourceMgr(void);
	virtual ~ResourceMgr(void);
	virtual bool LoadResFile(const char * resfile) = 0;
	virtual DataStream * GetResource(const char * resname, SClass_ID type) = 0;
	virtual void * GetFactoryResource(const char * resname, SClass_ID type, unsigned char mode = IE_NORMAL) = 0;
};

#endif
