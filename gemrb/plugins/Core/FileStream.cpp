#include "../../includes/win32def.h"
#include "FileStream.h"

FileStream::FileStream(void)
{
	opened = false;
	str = NULL;
	autoFree = false;
}

FileStream::~FileStream(void)
{
	if(autoFree && str)
		fclose(str);
}

bool FileStream::Open(const char * filename, bool autoFree)
{
	if(str && this->autoFree) {
		fclose(str);
	}
	this->autoFree = autoFree;
	str = fopen(filename, "rb");
	if(str == NULL) {
		return false;
	}
	startpos = 0;
	opened = true;
	fseek(str, 0, SEEK_END);
	size = ftell(str)+1;
	fseek(str, 0, SEEK_SET);
	ExtractFileFromPath(this->filename, filename);
	Pos = 0;
	return true;
}

bool FileStream::Open(FILE * stream, int startpos, int size, bool autoFree)
{
	if(str && this->autoFree) {
		fclose(str);
	}
	this->autoFree = autoFree;
	str = stream;
	if(str == NULL)
		return false;
	this->startpos = startpos;
	opened = true;
	this->size = size;
	strcpy(filename, "");
	fseek(str, startpos, SEEK_SET);
	Pos = 0;
	return true;
}

int FileStream::Read(void * dest, int length)
{
	if(!opened)
		return GEM_ERROR;
	size_t c = fread(dest, 1, length, str);
	//if(feof(str)) { /* slightly modified by brian  oct 11 2003*/
	//	return GEM_EOF;
	//}
	if(c < 0) {
		return GEM_ERROR;
	}
	if(Encrypted)
		ReadDecrypted(dest, c);
	Pos+=c;
	return (int)c;
}

int FileStream::Seek(int pos, int startpos)
{
	if(!opened)
		return GEM_ERROR;
	switch(startpos) 
		{
		case GEM_CURRENT_POS:
			fseek(str, pos, SEEK_CUR);
			Pos+=pos;
		break;

		case GEM_STREAM_START:
			fseek(str, this->startpos + pos, SEEK_SET);
			Pos = pos;
		break;

		default:
			return GEM_ERROR;
		}
	return GEM_OK;
}

unsigned long FileStream::Size()
{
	return size;	
}
/** No descriptions */
int FileStream::ReadLine(void * buf, int maxlen)
{
	if(feof(str))
		return -1;
	unsigned char *p = (unsigned char*)buf;
	int i = 0;
	while(i < (maxlen-1)) {
		int ch = fgetc(str);
		if(feof(str))
			 break;
		if(Pos==size)
			break;
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
	return i;
}
