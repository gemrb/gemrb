#ifndef WEDIMPCD_H
#define WEDIMPCD_H

#include "../Core/ClassDesc.h"

class WEDImpCD : public ClassDesc
{
public:
	WEDImpCD(void);
	~WEDImpCD(void);
	void * Create(void);
	const char * ClassName(void);
	SClass_ID SuperClassID(void);
	Class_ID ClassID(void);
};
static WEDImpCD WedImpCD;

#endif
