#ifndef BIFIMPCD_H
#define BIFIMPCD_H

#include "../Core/ClassDesc.h"

class BIFImpCD : public ClassDesc
{
public:
	BIFImpCD(void);
	~BIFImpCD(void);
	void * Create(void);
	const char* ClassName(void);
	SClass_ID SuperClassID(void);
	Class_ID ClassID(void);
	const char* InternalName(void);
};
static BIFImpCD BifImpCD;

#endif
