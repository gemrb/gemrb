#ifndef INIIMP_H
#define INIIMP_H

#include "../Core/DataFileMgr.h"
#include "../../includes/globals.h"

typedef struct INIPair {
	char * Name, * Value;
} INIPair;

class INITag {
private:
	std::vector<INIPair> pairs;
	char * TagName;
public:
	INITag(const char * Name)
	{
		int len = strlen(Name)+1;
		TagName = (char*)malloc(len);
		memcpy(TagName, Name, len);
	}

	~INITag()
	{
		free(TagName);
		for(int i = 0; i < pairs.size(); i++) {
			free(pairs[i].Name);
			free(pairs[i].Value);
		}
	}

	const char * GetTagName()
	{
		return TagName;
	}

	void AddLine(char * Line)
	{
		INIPair p;
		char * equal = strchr(Line, '=');
		*equal = 0;
		char * NameKey = Line;
		char * ValueKey = equal+1;
		//Left Trimming
		while(*NameKey != '\0') {
			if(*NameKey != ' ')
				break;
			NameKey++;
		}
		while(*ValueKey != '\0') {
			if(*ValueKey != ' ')
				break;
			ValueKey++;
		}
		//Right Trimming
		int NameKeyLen = strlen(NameKey);
		int ValueKeyLen = strlen(ValueKey);
		char * endNameKey = NameKey+NameKeyLen-1;
		char * endValueKey = ValueKey+ValueKeyLen-1;
		while(endNameKey != NameKey) {
			if(*endNameKey != ' ')
				break;
			*endNameKey-- = 0;
			NameKeyLen--;
		}
		while(endValueKey != ValueKey) {
			if(*endValueKey != ' ')
				break;
			*endValueKey-- = 0;
			ValueKeyLen--;
		}
		/*
		//Re-NewLine the string
		char * VKptr = ValueKey;
		while(*VKptr != '\0') {
			if((*VKptr == ' ') && (*(VKptr+1) == ' ')) {
				*VKptr++ = '\n';
				*VKptr = '\n';
			}
			VKptr++;
		}*/
		//Allocating Buffers
		p.Name = (char*)malloc(NameKeyLen+1);
		p.Value = (char*)malloc(ValueKeyLen+1);
		//Copying buffers
		memcpy(p.Name, NameKey, NameKeyLen+1);
		memcpy(p.Value, ValueKey, ValueKeyLen+1);
		//Adding to Tag Pairs
		pairs.push_back(p);
	}

	const char * GetKeyAsString(const char * Key, const char * Default)
	{
		for(int i = 0; i < pairs.size(); i++) {
			if(stricmp(Key, pairs[i].Name) == 0)
				return pairs[i].Value;
		}
		return Default;
	}

	const int GetKeyAsInt(const char * Key, const int Default)
	{
		const char * ret = NULL;
		for(int i = 0; i < pairs.size(); i++) {
			if(stricmp(Key, pairs[i].Name) == 0) {
				ret = pairs[i].Value;
				break;
			}
		}
		if(!ret)
			return Default;
		return atoi(ret);
	}
};

class INIImp : public DataFileMgr
{
private:
	DataStream * str;
	bool autoFree;
	std::vector<INITag*> tags;
	
public:
	INIImp(void);
	~INIImp(void);
	bool Open(DataStream * stream, bool autoFree = false);
	int GetTagsCount()
	{
		return tags.size();
	}
	const char * GetKeyAsString(const char * Tag, const char * Key, const char * Default);
	const int GetKetAsInt(const char * Tag, const char * Key, const int Default);
};

#endif
