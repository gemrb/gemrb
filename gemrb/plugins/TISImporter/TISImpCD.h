#ifndef TISIMPCD_H
#define TISIMPCD_G

#include "../Core/ClassDesc.h"

class TISImpCD : public ClassDesc
{
public:
	TISImpCD(void);
	~TISImpCD(void);
	void * Create(void);
	const char * ClassName(void);
	SClass_ID SuperClassID(void);
	Class_ID ClassID(void);
};
static TISImpCD TisImpCD;

#endif
