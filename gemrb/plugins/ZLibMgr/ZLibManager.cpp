#include "../../includes/win32def.h"
#include "../../includes/globals.h"
#include "ZLibManager.h"
#include "../../zlib/zlib.h"

ZLibManager::ZLibManager(void)
{
}

ZLibManager::~ZLibManager(void)
{
}

// ZLib Decompression Routine
int ZLibManager::Decompress(void * dest, unsigned long* dlen, void * src, unsigned long slen)
{
	int res = uncompress((Bytef*)dest, dlen, (Bytef*)src, slen);
	switch(res) {
		case Z_MEM_ERROR:
		case Z_BUF_ERROR:
		case Z_DATA_ERROR:
			return GEM_ERROR;
		default:
			return GEM_OK;
	}
}
