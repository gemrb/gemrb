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

	str->CheckEncrypted();
	char tmp[10];
	str->ReadLine(tmp, 10);
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
		ptrs.push_back(line);
		char * str = strtok(line, " ");
		Pair p;
		if((str[0] == '0') && ((str[1] == 'x') || (str[1] == 'X'))) {
			sscanf(str, "0x%x", p.val);
		}
		else
			p.val = atoi(str);
		str = strtok(NULL, " ");
		p.str = str;
		pairs.push_back(p);
	}

	return true;

}

long IDSImp::GetValue(const char * txt)
{
	for(unsigned int i = 0; i < pairs.size(); i++) {
		if(stricmp(pairs[i].str, txt) == 0)
			return pairs[i].val;
	}
	return 0L;
}

const char * IDSImp::GetValue(int val)
{
	for(unsigned int i = 0; i < pairs.size(); i++) {
		if(pairs[i].val == val)
			return pairs[i].str;
	}
	return NULL;
}