#ifndef FACTORY_H
#define FACTORY_H

#include "../../includes/globals.h"
#include "FactoryObject.h"
#include "AnimationFactory.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT Factory
{
private:
	std::vector<FactoryObject*> fobjects;
public:
	Factory(void);
	~Factory(void);
	void AddFactoryObject(FactoryObject * fobject);
	int IsLoaded(const char * ResRef, SClass_ID type);
	FactoryObject * GetFactoryObject(int pos);
	void FreeObjects(void);
};

#endif
