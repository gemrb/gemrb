#ifndef ARCHIVEIMPORTER_H
#define ARCHIVEIMPORTER_H

#include "../../includes/globals.h"
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

class GEM_EXPORT ArchiveImporter : public Plugin
{
public:
	ArchiveImporter(void);
	virtual ~ArchiveImporter(void);
	virtual int OpenArchive(char* filename, bool cacheCheck = true) = 0;
	virtual DataStream* GetStream(ulong Resource, ulong Type) = 0;
};

#endif
