#ifndef CACHEDFILESTREAM_H
#define CACHEDFILESTREAM_H

#include "FileStream.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT CachedFileStream : public DataStream// : public FileStream
{
private:
	bool autoFree;
	int startpos;
	int size;
	FILE * str;
public:
	CachedFileStream(char * stream, bool autoFree = true);
	CachedFileStream(CachedFileStream * cfs, int startpos, int size, bool autoFree = false);
	~CachedFileStream(void);
	int Read(void * dest, int length);
	int Seek(int pos, int startpos);
	unsigned long Size();
	/** No descriptions */
	int ReadLine(void * buf, int maxlen);
  };
#endif
