#include "../../includes/win32def.h"
#include "MemoryStream.h"

MemoryStream::MemoryStream(void * buffer, int length)
{
	ptr = buffer;
	this->length = length;
	Pos = 0;
	strcpy(filename, "");
}

MemoryStream::~MemoryStream(void)
{
}

int MemoryStream::Read(void * dest, int length)
{
	if(length+Pos > this->length)
		return GEM_ERROR;
	Byte * p = (Byte*)ptr + Pos;
	memcpy(dest, p, length);
	if(Encrypted)
		ReadDecrypted(dest,length);
	Pos+=length;
	return GEM_OK;
}

int MemoryStream::Seek(int arg_pos, int startpos)
{
	switch(startpos) {
		case GEM_CURRENT_POS:
			{
			if((Pos + arg_pos) < 0)
				return GEM_ERROR;
			if((Pos + arg_pos) >= length)
				return GEM_ERROR;
			Pos+=arg_pos;
			}
		break;

		case GEM_STREAM_START:
			{
			if(arg_pos >= length)
				return GEM_ERROR;
			Pos = length;
			}
		break;

		default:
			return GEM_ERROR;
	}
	return GEM_OK;
}

unsigned long MemoryStream::Size()
{
	return length;
}
