#ifndef CREIMPCD_H
#define CREIMPCD_H

#include "../Core/ClassDesc.h"

class CREImpCD : public ClassDesc
{
public:
	CREImpCD(void);
	~CREImpCD(void);
	void * Create(void);
	const char* ClassName(void);
	SClass_ID SuperClassID(void);
	Class_ID ClassID(void);
	const char* InternalName(void);
};
static CREImpCD CreImpCD;

#endif
