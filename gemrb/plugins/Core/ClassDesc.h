#ifndef CLASSDESC_H
#define CLASSDESC_H

#include "../Core/Class_ID.h"
#include "../../includes/SClassID.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT ClassDesc
{
public:
	ClassDesc(void);
	virtual ~ClassDesc(void);
	virtual void * Create() = 0;
	virtual int BeginCreate();
	virtual int EndCreate();
	virtual const char* ClassName(void) = 0;
	virtual SClass_ID SuperClassID(void) = 0;
	virtual Class_ID ClassID(void) = 0;
	virtual Class_ID SubClassID(void);
	virtual const char* InternalName(void);
	//TODO: Add Functions for Function Parameter Handling (GemScript)
};

#endif
