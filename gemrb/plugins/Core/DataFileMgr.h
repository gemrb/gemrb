#ifndef DATAFILEMGR_H
#define DATAFILEMGR_H

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

class GEM_EXPORT DataFileMgr : public Plugin
{
public:
	DataFileMgr(void);
	virtual ~DataFileMgr(void);
	virtual bool Open(DataStream * stream, bool autoFree = false) = 0;
	virtual int GetTagsCount() = 0;
	virtual const char * GetKeyAsString(const char * Tag, const char * Key, const char * Default) = 0;
	virtual const int GetKetAsInt(const char * Tag, const char * Key, const int Default) = 0;
};

#endif
