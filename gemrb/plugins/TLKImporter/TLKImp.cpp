#include "TLKImp.h"

TLKImp::TLKImp(void)
{
	str = NULL;
	autoFree = false;
}

TLKImp::~TLKImp(void)
{
	if(str && autoFree)
		delete(str);
}

bool TLKImp::Open(DataStream * stream, bool autoFree)
{
	if(stream == NULL)
		return false;
	if(str && this->autoFree)
		delete(str);
	str = stream;
	this->autoFree = autoFree;
	char Signature[8];
	str->Read(Signature, 8);
	if(strncmp(Signature, "TLK V1  ", 8) != 0) {
		printf("[TLKImporter]: Not a valid TLK File.");
		return false;
	}
	str->Seek(2, GEM_CURRENT_POS);
	str->Read(&StrRefCount, 4);
	str->Read(&Offset, 4);
	return true;
}

char * TLKImp::GetString(unsigned long strref)
{
	if(strref >= StrRefCount)
		return NULL;
	unsigned long Volume, Pitch, StrOffset, Length;
	unsigned short type;
	char SoundResRef[8];
	str->Seek(18+(strref*0x1A), GEM_STREAM_START);
	str->Read(&type, 2);
	if(type == 2)
		return NULL;
	str->Read(SoundResRef, 8);
	str->Read(&Volume, 4);
	str->Read(&Pitch, 4);
	str->Read(&StrOffset, 4);
	str->Read(&Length, 4);
	str->Seek(StrOffset+Offset, GEM_STREAM_START);
	char * string = (char*)malloc(Length+1);
	str->Read(string, Length);
	string[Length] = 0;
	return string;
}
