#ifndef FACTORYOBJECT_H
#define FACTORYOBJECT_H

#include "../../includes/globals.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT FactoryObject
{
public:
	SClass_ID SuperClassID;
	char ResRef[9];
	FactoryObject(const char * ResRef, SClass_ID SuperClassID);
	~FactoryObject(void);
};

#endif
