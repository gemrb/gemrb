#ifndef COMPRESSOR_H
#define COMPRESSOR_H

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

class GEM_EXPORT Compressor : public Plugin
{
public:
	Compressor(void);
	virtual ~Compressor(void);
	/** Initialization Function. Returns FALSE if there was an error during initialization, else returns TRUE. */
	virtual int Init(void);
	/** Decompress Function. 'Byte * dest' is the preallocated destination buffer, 'ulong* dlen' is a pointer to the destination buffer size, 'Byte * src' is the source buffer, 'ulong slen' is the source buffer size */
	virtual int Decompress(void * dest, unsigned long* dlen, void * src, unsigned long slen) = 0;
};

#endif
