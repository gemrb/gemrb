#ifndef ZLIBMANAGER_H
#define ZLIBMANAGER_H

#include "../Core/Compressor.h"

class ZLibManager :
	public Compressor
{
public:
	ZLibManager(void);
	~ZLibManager(void);
	// ZLib Decompression Routine
	int Decompress(void * dest, unsigned long* dlen, void * src, unsigned long slen);
public:
	void release(void)
	{
		delete this;
	}
};

#endif
