#ifndef KEYIMPCD_H
#define KEYIMPCD_H

#include "../../includes/globals.h"

class KEYImpCD :
	public ClassDesc
{
public:
	KEYImpCD(void);
	~KEYImpCD(void);
	void * Create();
	const char* ClassName(void);
	SClass_ID SuperClassID(void);
	Class_ID ClassID(void);
	const char* InternalName(void);
};
static KEYImpCD KEYImporterCD;

#endif
