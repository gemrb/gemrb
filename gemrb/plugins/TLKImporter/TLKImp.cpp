#include "../../includes/win32def.h"
#include "TLKImp.h"
#include "../Core/Interface.h"

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

inline char *mystrncpy(char *dest, const char *source, int maxlength, char delim)
{
  while(*source && (*source!=delim) && maxlength--)
  {
    *dest++=*source++;
  }
  *dest=0;
  return dest;
}

static bool ResolveTags(char *dest, char *source, int Length)
{
  int NewLength;
  char Token[MAX_VARIABLE_LENGTH+1];

  NewLength=0;
  for(int i=0; source[i];i++) {
	if(source[i]=='<') {
		i+=mystrncpy(Token, source+i+1, MAX_VARIABLE_LENGTH, '>' )-Token+1;
		int TokenLength=core->GetTokenDictionary()->GetValueLength(Token);
		if(TokenLength) {
			if(TokenLength+NewLength>Length)
				return false;
			core->GetTokenDictionary()->Lookup(Token, dest+NewLength, TokenLength);
			NewLength+=TokenLength;
		}
	}
	else {
		if(source[i]=='[') {
			char *tmppoi=strchr(source+i+1, ']');
			if(tmppoi)
				i=tmppoi-source+1;
			else
				break;
		}
		else
			dest[NewLength++]=source[i];
		if(NewLength>Length)
			return false;
	}
  }
  dest[NewLength]=0;
  return true;
}

static bool GetNewStringLength(char *string, unsigned long &Length)
{
  int NewLength;
  char Token[MAX_VARIABLE_LENGTH+1];

  NewLength=0;
  for(int i=0;i<Length;i++) {
	if(string[i]=='<') {
		i+=mystrncpy(Token, string+i+1, MAX_VARIABLE_LENGTH, '>' )-Token+1;
		NewLength+=core->GetTokenDictionary()->GetValueLength(Token);
	}
	else {
		if(string[i]=='[') {
			char *tmppoi=strchr(string+i+1,']');
			if(tmppoi)
				NewLength+=tmppoi-string-i-1;
			else
				break;
		}
		else NewLength++;
	}
  }
  if(NewLength!=Length)
  {
	Length=NewLength;
	return true;
  }
  return false;
}

char * TLKImp::GetString(unsigned long strref, int flags)
{
	if(strref >= StrRefCount)
		return NULL;
	unsigned long Volume, Pitch, StrOffset, Length;
	unsigned short type;
	char SoundResRef[8];
	str->Seek(18+(strref*0x1A), GEM_STREAM_START);
	str->Read(&type, 2);
	str->Read(SoundResRef, 8);
	str->Read(&Volume, 4);
	str->Read(&Pitch, 4);
	str->Read(&StrOffset, 4);
	str->Read(&Length, 4);
	if(Length>65535) Length=65535; 
	char *string;

	if(type & 1) {
		str->Seek(StrOffset+Offset, GEM_STREAM_START);
		string = (char*)malloc(Length+1);
		str->Read(string, Length);
	}
	else {
		Length = 0;
		string = (char*) malloc(1);
	}
	string[Length] = 0;
//tagged text
	if(type&4) {
//GetNewStringLength will look in string and return true
//if the new Length will change due to tokens
//if there is no new length, we are done
		while(GetNewStringLength(string, Length) ) {
			char *string2 = (char *) malloc(Length+1);
//ResolveTags will copy string to string2
			ResolveTags(string2, string, Length);
			free(string);
			string=string2;
		}
	}
	if(flags&IE_STR_STRREFON) {
		char *string2 = (char *) malloc(Length+11);
		sprintf(string2,"%d: %s", strref, string);
		free(string);
		return string2;
	}
	if((type&2) && (flags&IE_STR_SOUND) ) {
//if flags&IE_STR_SOUND play soundresref
	}
	return string;
}
