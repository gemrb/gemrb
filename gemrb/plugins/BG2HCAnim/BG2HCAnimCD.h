#ifndef BG2HCANIMCD_H
#define BG2HCANIMCD_H

#include "../Core/ClassDesc.h"

class BG2HCAnimCD : public ClassDesc
{
public:
	BG2HCAnimCD(void);
	~BG2HCAnimCD(void);
	void * Create(void);
	const char* ClassName(void);
	SClass_ID SuperClassID(void);
	Class_ID ClassID(void);
	const char* InternalName(void);
};
static BG2HCAnimCD Bg2HCAnimCD;

#endif
