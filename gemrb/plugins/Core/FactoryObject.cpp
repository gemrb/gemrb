#include "win32def.h"
#include "FactoryObject.h"

FactoryObject::FactoryObject(const char * ResRef, SClass_ID SuperClassID)
{
	strncpy(this->ResRef, ResRef, 8);
	this->SuperClassID = SuperClassID;
}

FactoryObject::~FactoryObject(void)
{
}
