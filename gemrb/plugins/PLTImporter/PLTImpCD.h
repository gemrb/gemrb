#ifndef PLTIMPCD_H
#define PLTIMPCD_H

#include "../Core/ClassDesc.h"

class PLTImpCD :
	public ClassDesc
{
public:
	PLTImpCD(void);
	~PLTImpCD(void);
	void * Create(void);
	const char* ClassName(void);
	SClass_ID SuperClassID(void);
	Class_ID ClassID(void);
	const char* InternalName(void);
};
static PLTImpCD PltImpCD;

#endif
