#include "../../includes/win32def.h"
#include "MemoryStream.h"

MemoryStream::MemoryStream(void * buffer, int length)
{
	ptr = buffer;
	this->length = length;
	pos = 0;
	strcpy(filename, "");
}

MemoryStream::~MemoryStream(void)
{
}

int MemoryStream::Read(void * dest, int length)
{
	if(length+pos > this->length)
		return GEM_ERROR;
	Byte * p = (Byte*)ptr + pos;
	memcpy(dest, p, length);
	pos+=length;
	return GEM_OK;
}

int MemoryStream::Seek(int pos, int startpos)
{
	switch(startpos) {
		case GEM_CURRENT_POS:
			{
			if((this->pos + pos) < 0)
				return GEM_ERROR;
			if((this->pos + pos) >= length)
				return GEM_ERROR;
			this->pos+=pos;
			}
		break;

		case GEM_STREAM_START:
			{
			if(pos >= length)
				return GEM_ERROR;
			this->pos = length;
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
