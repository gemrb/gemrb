#ifndef TLKIMP_H
#define TLKIMP_H

#include "../Core/StringMgr.h"

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
	char * GetString(unsigned long strref);
};

#endif
