#include "../../includes/win32def.h"
#include "CachedFileStream.h"
#include "Interface.h"

extern Interface * core;

CachedFileStream::CachedFileStream(char * stream, bool autoFree)
{
	char fname[_MAX_PATH];
	ExtractFileFromPath(fname, stream);

	char path[_MAX_PATH];
	strcpy(path, core->CachePath);
	strcat(path, fname);
	str = fopen(path, "rb");
	if(str == NULL) {
		FILE * src = fopen(stream, "rb");
		FILE * dest = fopen(path, "wb");
		void * buff = malloc(1024*1000);
		do {
			size_t len = fread(buff, 1, 1024*1000, src);
			fwrite(buff, 1, len, dest);
		} while(!feof(src));
		free(buff);
		fclose(src);
		fclose(dest);
		str = fopen(path, "rb");
	}
	startpos = 0;
	fseek(str, 0, SEEK_END);
	size = ftell(str);
	fseek(str, 0, SEEK_SET);
	strcpy(filename, fname);
	Pos = 0;
	this->autoFree = autoFree;
}

CachedFileStream::CachedFileStream(CachedFileStream * cfs, int startpos, int size, bool autoFree)
{
	this->size = size;
	this->startpos = startpos;
	this->autoFree = autoFree;
	char cpath[_MAX_PATH];
	strcpy(cpath, core->CachePath);
	strcat(cpath, cfs->filename);
	str = fopen(cpath, "rb");
	fseek(str, startpos, SEEK_SET);
	Pos = 0;
}

CachedFileStream::~CachedFileStream(void)
{
	/*if(autoFree && str) {
		fclose(str);
	}
	autoFree = false; //File stream destructor hack*/
}

int CachedFileStream::Read(void * dest, int length)
{
	size_t c = fread(dest, 1, length, str);
	if(c < 0) {
		if(feof(str)) {
			return GEM_EOF;
		}
		return GEM_ERROR;
	}
	if(Encrypted)
		ReadDecrypted(dest, c);
	Pos+=c;
	return (int)c;
}

int CachedFileStream::Seek(int pos, int startpos)
{
	switch(startpos) 
		{
		case GEM_CURRENT_POS:
			fseek(str, pos, SEEK_CUR);
			Pos+=pos;
		break;

		case GEM_STREAM_START:
			fseek(str, this->startpos + pos, SEEK_SET);
			Pos=pos;
		break;

		default:
			return GEM_ERROR;
		}
	return GEM_OK;
}

unsigned long CachedFileStream::Size()
{
	return size;
}/** No descriptions */
int CachedFileStream::ReadLine(void * buf, int maxlen)
{
	unsigned char *p = (unsigned char*)buf;
	int i = 0;
	while(i < (maxlen-1)) {
		int ch = fgetc(str);
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
}
