#ifndef TLKIMP_H
#define TLKIMP_H

#include "../Core/StringMgr.h"

#define IE_STR_STRREFON	1
#define IE_STR_SOUND	2

class TLKImp : public StringMgr
{
private:
	DataStream * str;
	bool autoFree;

	//Data
	unsigned long StrRefCount, Offset;
public:
	TLKImp(void);
	~TLKImp(void);
	bool Open(DataStream * stream, bool autoFree = true);
	char * GetString(unsigned long strref, int flags=0);
private:
	/** replaces tags in dest, don't exceed Length */
	bool ResolveTags(char *dest, char *source, int Length);
	/** returns the needed length in Length, 
	    if there was no token, returns false */
	bool GetNewStringLength(char *string, unsigned long &Length);
	/**returns the decoded length of the built-in token
	   if dest is not NULL it also returns the decoded value
	   */
	int BuiltinToken(char *Token, char *dest);
};

#endif
