// TlkOverride.h: interface for the CTlkOverride class.
//
//////////////////////////////////////////////////////////////////////

#ifndef TLKOVERRIDE_H
#define TLKOVERRIDE_H

#include "../../includes/globals.h"
#include "../Core/Interface.h"
#include "../Core/FileStream.h"
#ifdef CACHE_TLK_OVERRIDE
#include <map>

typedef std::map<ieStrRef, char *> StringMapType;
#endif

class CTlkOverride  
{
private:
#ifdef CACHE_TLK_OVERRIDE
	StringMapType stringmap;
#endif
	DataStream *tot_str;
	DataStream *toh_str;
	ieDword AuxCount;

	void CloseResources();
	DataStream *GetAuxHdr();
	DataStream *GetAuxTlk();
	char* LocateString2(ieDword offset);
	char* LocateString(ieStrRef strref);
	ieDword GetLength();
public:
	CTlkOverride();
	virtual ~CTlkOverride();

	char *CS(const char *src);
	bool Init();
	char *ResolveAuxString(ieStrRef strref, int &Length);
};

#endif //TLKOVERRIDE_H

