#ifndef BMPIMPCD_H
#define BMPIMPCD_H

#include "../Core/ClassDesc.h"

class BMPImpCD :
	public ClassDesc
{
public:
	BMPImpCD(void);
	~BMPImpCD(void);
	void * Create(void);
	const char* ClassName(void);
	SClass_ID SuperClassID(void);
	Class_ID ClassID(void);
	const char* InternalName(void);
};
static BMPImpCD BmpImpCD;

#endif
