#include "../../includes/win32def.h"
#include "INIImp.h"
#include "../Core/FileStream.h"

INIImp::INIImp(void)
{
	str = NULL;
	autoFree = false;
}

INIImp::~INIImp(void)
{
	if(str && autoFree)
		delete(str);
	for(int i = 0; i < tags.size(); i++)
		delete(tags[i]);
}

bool INIImp::Open(DataStream * stream, bool autoFree)
{
	if(stream == NULL)
		return false;
	if(str && this->autoFree)
		delete(str);
	str = stream;
	this->autoFree = autoFree;
	int cnt = 0;
	char * strbuf = (char*)malloc(4097);
	INITag * lastTag = NULL;
	do {
		cnt = str->ReadLine(strbuf, 4096);
		if(cnt == -1)
			break;
		if(cnt == 0)
			continue;
		if(strbuf[0] == ';') //Rem
			continue;
		if(strbuf[0] == '[') { // this is a tag
			char * sbptr = strbuf+1;
			char * TagName = sbptr;
			while(*sbptr != '\0') {
				if(*sbptr == ']') {
					*sbptr = 0;
					break;
				}
				sbptr++;
			}
			INITag * it = new INITag(TagName);
			tags.push_back(it);
			lastTag = it;
			continue;
		}
		if(lastTag == NULL)
			continue;
		lastTag->AddLine(strbuf);
	} while(true);
	free(strbuf);
}

const char * INIImp::GetKeyAsString(const char * Tag, const char * Key, const char * Default)
{
	for(int i = 0; i < tags.size(); i++) {
		const char * TagName = tags[i]->GetTagName();
		if(stricmp(TagName, Tag) == 0) {
			return tags[i]->GetKeyAsString(Key, Default);
		}
	}
	return Default;
}

const int INIImp::GetKetAsInt(const char * Tag, const char * Key, const int Default)
{
	for(int i = 0; i < tags.size(); i++) {
		const char * TagName = tags[i]->GetTagName();
		if(stricmp(TagName, Tag) == 0) {
			return tags[i]->GetKeyAsInt(Key, Default);
		}
	}
	return Default;
}
