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
/** No descriptions */
int MemoryStream::ReadLine(void * buf, int maxlen)
{
	unsigned char *p = (unsigned char*)buf;
	int i = 0;
	while(i < (maxlen-1)) {
		Byte ch = *((Byte*)ptr + Pos);
		if(Encrypted)
			p[i]^=GEM_ENCRYPTION_KEY[Pos&63];
		Pos++;
		if(ch == '\n')
			break;
		if(ch == '\t')
			ch = ' ';
    if(ch != '\r')
			p[i++] = ch;
	}
	p[i] = 0;
	return i-1;
}
