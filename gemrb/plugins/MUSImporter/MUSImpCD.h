#ifndef MOSIMPCD_H
#define MOSIMPCD_H

#include "../Core/ClassDesc.h"

class MUSImpCD :
	public ClassDesc
{
public:
	MUSImpCD(void);
	~MUSImpCD(void);
	void * Create(void);
	const char* ClassName(void);
	SClass_ID SuperClassID(void);
	Class_ID ClassID(void);
	const char* InternalName(void);
};
static MUSImpCD MusImpCD;

#endif
