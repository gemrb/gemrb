#ifndef IDSIMPCD_H
#define IDSIMPCD_H

#include "../Core/ClassDesc.h"

class IDSImpCD : public ClassDesc
{
public:
	IDSImpCD(void);
	~IDSImpCD(void);
	void * Create(void);
	const char * ClassName(void);
	SClass_ID SuperClassID(void);
	Class_ID ClassID(void);
};
static IDSImpCD IDSImpCD;

#endif
