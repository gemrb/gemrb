#ifndef DATASTREAM_H
#define DATASTREAM_H

#include "../../includes/globals.h"

#define GEM_CURRENT_POS 0
#define GEM_STREAM_START 1

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT DataStream
{
public:
	int Pos;
	bool Encrypted;
	DataStream(void);
	virtual ~DataStream(void);
	virtual int Read(void * dest, int len) = 0;
	virtual int Seek(int pos, int startpos) = 0;
	virtual unsigned long Size() = 0;
  /** Returns true if the stream is encrypted */
  bool CheckEncrypted();
  /** No descriptions */
  void ReadDecrypted(void * buf, int size);
	char filename[_MAX_PATH];
};

#endif
