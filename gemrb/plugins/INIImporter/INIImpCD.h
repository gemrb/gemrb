#ifndef INIIMPCD_H
#define INIIMPCD_H

#include "../Core/ClassDesc.h"

class INIImpCD : public ClassDesc
{
public:
	INIImpCD(void);
	~INIImpCD(void);
	void * Create(void);
	const char* ClassName(void);
	SClass_ID SuperClassID(void);
	Class_ID ClassID(void);
	const char* InternalName(void);
};
static INIImpCD IniImpCD;

#endif
