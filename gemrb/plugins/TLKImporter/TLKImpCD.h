#ifndef TLKIMPCD_H
#define TLKIMPCD_H

#include "../Core/ClassDesc.h"

class TLKImpCD : public ClassDesc
{
public:
	TLKImpCD(void);
	~TLKImpCD(void);
	void * Create(void);
	const char * ClassName(void);
	SClass_ID SuperClassID(void);
	Class_ID ClassID(void);
};
static TLKImpCD TlkImpCD;

#endif
