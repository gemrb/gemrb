#ifndef MEMORYSTREAM_H
#define MEMORYSTREAM_H

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

class GEM_EXPORT MemoryStream : public DataStream
{
private:
	void * ptr;
	int length;
public:
	MemoryStream(void * buffer, int length);
	~MemoryStream(void);
	int Read(void * dest, int length);
	int Seek(int pos, int startpos);
	unsigned long Size();
};

#endif
