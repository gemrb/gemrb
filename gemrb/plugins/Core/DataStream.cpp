#include "../../includes/win32def.h"
#include "DataStream.h"

DataStream::DataStream(void)
{
}

DataStream::~DataStream(void)
{
}
/** Returns true if the stream is encrypted */
bool DataStream::CheckEncrypted()
{
	unsigned short two;
	Seek(0, GEM_STREAM_START);
	Read(&two, 2);
	Seek(0, GEM_STREAM_START);
	if(two == 0xFFFF) {
		return true;
	}
	return false;
}
