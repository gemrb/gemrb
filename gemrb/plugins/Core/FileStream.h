#ifndef FILESTREAM_H
#define FILESTREAM_H

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

class GEM_EXPORT FileStream : public DataStream
{
private:
	FILE * str;
	ulong startpos;
	bool autoFree;
	bool opened;
	unsigned long size;
public:
	FileStream(void);
	~FileStream(void);

	bool Open(const char * filename, bool autoFree = true);
	bool Open(FILE * stream, int startpos, int size, bool autoFree = false);
	int Read(void * dest, int length);
	int Seek(int pos, int startpos);
	unsigned long Size();
};

#endif
