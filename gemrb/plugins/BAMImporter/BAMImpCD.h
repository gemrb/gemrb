#ifndef BAMIMPCD_H
#define BAMIMPCD_H

#include "../Core/ClassDesc.h"

class BAMImpCD : public ClassDesc
{
public:
	BAMImpCD(void);
	~BAMImpCD(void);
	void * Create(void);
	const char* ClassName(void);
	SClass_ID SuperClassID(void);
	Class_ID ClassID(void);
	const char* InternalName(void);
};
static BAMImpCD BamImpCD;

#endif
