#ifndef AREIMPCD_H
#define AREIMPCD_H

#include "../Core/ClassDesc.h"

class AREImpCD : public ClassDesc
{
public:
	AREImpCD(void);
	~AREImpCD(void);
	void * Create(void);
	const char* ClassName(void);
	SClass_ID SuperClassID(void);
	Class_ID ClassID(void);
	const char* InternalName(void);
};
static AREImpCD AreImpCD;

#endif
