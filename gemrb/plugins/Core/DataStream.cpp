#include "../../includes/win32def.h"
#include "DataStream.h"

DataStream::DataStream(void)
{
	Pos = 0;
	Encrypted = false;
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
	if(two == 0xFFFF) {
		Pos=0;
		Encrypted = true;
		return true;
	}
	Seek(0, GEM_STREAM_START);
	Encrypted = false;
	return false;
}
/** No descriptions */
void DataStream::ReadDecrypted(void * buf, int size)
{
	for(int i=0;i<size;i++)
		((unsigned char *) buf)[i]^=GEM_ENCRYPTION_KEY[(Pos+i)&63];
}
