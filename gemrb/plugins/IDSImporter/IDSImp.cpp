#include "../../includes/win32def.h"
#include "../../includes/globals.h"
#include "IDSImp.h"
#include "IDSImpDefs.h"
#include <ctype.h>

IDSImp::IDSImp(void)
{
	str = NULL;
	autoFree = false;
}

IDSImp::~IDSImp(void)
{

	if(str && autoFree)
		delete(str);

	for(int i = 0; i < ptrs.size(); i++) {
		free(ptrs[i]);
	}
}

bool IDSImp::Open(DataStream * stream, bool autoFree)
{
	if(stream == NULL)
		return false;
	if(str)
		return false;
	str = stream;
	this->autoFree = autoFree;

	bool Encrypted=str->CheckEncrypted();
	char tmp[11];
	str->ReadLine(tmp, 10);
	tmp[10]=0;
	if(tmp[0]!='I') {
		str->Seek(Encrypted?2:0,GEM_STREAM_START);
		str->Pos=0;
	}
	while(true) {
		char * line = (char*)malloc(256);
		int len = str->ReadLine(line, 256);
		if(len == -1) {
			free(line);
			break;
		}
		if(len == 0) {
			free(line);
			continue;
		}
		if(len < 256)
			line = (char*)realloc(line, len+1);
		char * str = strtok(line, " ");
		Pair p;
		p.val=strtoul(str,NULL,0);
		str = strtok(NULL, " ");
		p.str = str;
		if(str!=NULL) {
			ptrs.push_back(line);
			pairs.push_back(p);
		}
		else {
			free(line);
		}
	}

	return true;

}

long IDSImp::GetValue(const char * txt)
{
	for(unsigned int i = 0; i < pairs.size(); i++) {
		if(stricmp(pairs[i].str, txt) == 0)
			return pairs[i].val;
	}
	return -1;
}

const char * IDSImp::GetValue(int val)
{
	for(unsigned int i = 0; i < pairs.size(); i++) {
		if(pairs[i].val == val)
			return pairs[i].str;
	}
	return "";
}
