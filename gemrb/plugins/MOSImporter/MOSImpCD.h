#ifndef MOSIMPCD_H
#define MOSIMPCD_H

#include "../Core/ClassDesc.h"

class MOSImpCD :
	public ClassDesc
{
public:
	MOSImpCD(void);
	~MOSImpCD(void);
	void * Create(void);
	const char* ClassName(void);
	SClass_ID SuperClassID(void);
	Class_ID ClassID(void);
	const char* InternalName(void);
};
static MOSImpCD MosImpCD;

#endif
