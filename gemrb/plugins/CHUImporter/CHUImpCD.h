#ifndef CHUIMPCD_H
#define CHUIMPCD_H

#include "../Core/ClassDesc.h"

class CHUImpCD : public ClassDesc
{
public:
	CHUImpCD(void);
	~CHUImpCD(void);
	void * Create(void);
	const char* ClassName(void);
	SClass_ID SuperClassID(void);
	Class_ID ClassID(void);
	const char* InternalName(void);
};
static CHUImpCD ChuImpCD;

#endif
