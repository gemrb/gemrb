#ifndef P2DAIMPCD_H
#define P2DAIMPCD_H

#include "../Core/ClassDesc.h"

class p2DAImpCD : public ClassDesc
{
public:
	p2DAImpCD(void);
	~p2DAImpCD(void);
	void * Create(void);
	const char* ClassName(void);
	SClass_ID SuperClassID(void);
	Class_ID ClassID(void);
	const char* InternalName(void);
};
static p2DAImpCD p2daImpCD;

#endif
