#ifndef ACMIMPORTERCD_H
#define ACMIMPORTERCD_H

#include "../Core/ClassDesc.h"

class ACMImporterCD :
	public ClassDesc
{
public:
	ACMImporterCD(void);
	~ACMImporterCD(void);
	void * Create(void);
	const char* ClassName(void);
	SClass_ID SuperClassID(void);
	Class_ID ClassID(void);
	const char* InternalName(void);
};

static ACMImporterCD AcmImporterCD;

#endif
