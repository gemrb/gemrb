#include "../../includes/win32def.h"
#include "TLKImp.h"
#include "../Core/Interface.h"

TLKImp::TLKImp(void)
{
	str = NULL;
	autoFree = false;
	if(stricmp(core->GameType, "bg1") == 0)
		isBG1 = true;
	else
		isBG1 = false;
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
  while(*source && (*source!=delim) && maxlength--) {
    *dest++=*source++;
  }
  *dest=0;
  return dest;
}

//if this function returns -1 then it is not a built in token, dest may be NULL
int TLKImp::BuiltinToken(char *Token, char *dest)
{
	char *Decoded=NULL;
	int TokenLength;   //decoded token length

//this may be hardcoded, all engines are the same or don't use it
	if(!strcmp(Token,"FIGHTERTYPE") ) {
		Decoded=GetString(10086,0);
		goto exit_function;
	}
	if(!strcmp(Token,"WEAPONNAME") ) {
//this should be character dependent, we don't have a character sheet yet
	}

	if(!strcmp(Token,"MAGESCHOOL") ) {
//this should be character dependent, we don't have a character sheet yet
		unsigned long row=0; //default value is 0 (generalist)
 //this is subject to change, the row number in magesch.2da
		core->GetDictionary()->Lookup("MAGESCHOOL",row); 
		int ind = core->LoadTable("magesch");
		TableMgr *tm=core->GetTable(ind);
		if(tm)
		{
			char *value=tm->QueryField(row,0);
			Decoded=GetString(atoi(value),0);
			goto exit_function;
		}
	}

	return -1;  //not decided

exit_function:
	if(Decoded)
	{
		TokenLength=strlen(Decoded);
		if(dest) memcpy(dest,Decoded,TokenLength);
		free(Decoded);
	}
	return TokenLength;
}

bool TLKImp::ResolveTags(char *dest, char *source, int Length)
{
  int NewLength;
  char Token[MAX_VARIABLE_LENGTH+1];

  NewLength=0;
  for(int i=0; source[i];i++) {
	if(source[i]=='<') {
		i+=mystrncpy(Token, source+i+1, MAX_VARIABLE_LENGTH, '>' )-Token+1;
		int TokenLength=BuiltinToken(Token, dest+NewLength);
		if(TokenLength==-1) {
			TokenLength=core->GetTokenDictionary()->GetValueLength(Token);
			if(TokenLength) {
				if(TokenLength+NewLength>Length)
					return false;
				core->GetTokenDictionary()->Lookup(Token, dest+NewLength, TokenLength);
			}
		}
		NewLength+=TokenLength;
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

bool TLKImp::GetNewStringLength(char *string, unsigned long &Length)
{
  int NewLength;
  bool lChange;
  char Token[MAX_VARIABLE_LENGTH+1];

  lChange=false;
  NewLength=0;
  for(int i=0;i<Length;i++) {
	if(string[i]=='<') {   // token
		lChange=true;
		i+=mystrncpy(Token, string+i+1, MAX_VARIABLE_LENGTH, '>' )-Token+1;
		int TokenLength=BuiltinToken(Token, NULL);
		if(TokenLength==-1) {
			NewLength+=core->GetTokenDictionary()->GetValueLength(Token);
		}
		else {
			NewLength+=TokenLength;
		}
	}
	else {
		if(string[i]=='[') { //voice actor directives
			lChange=true;
			char *tmppoi=strchr(string+i+1,']');
			if(tmppoi)
				NewLength+=tmppoi-string-i-1;
			else
				break;
		}
		else NewLength++;
	}
  }
  Length=NewLength;
  return lChange;
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
		if(isBG1) {
			if(string[0] == '[') {
				for(int i = 1; i < Length; i++) {
					if(string[i] == ']') {
						i++;
						char * s = (char*)malloc(Length-i+1);
						strcpy(s, &string[i]);
						free(string);
						string = s;
						Length = strlen(string);
						break;
					}
				}
			}
		}
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
