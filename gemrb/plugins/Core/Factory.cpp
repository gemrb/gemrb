#include "win32def.h"
#include "Factory.h"

Factory::Factory(void)
{
}

Factory::~Factory(void)
{
	for(unsigned int i = 0; i < fobjects.size(); i++) {
		delete(fobjects[i]);
	}
}

void Factory::AddFactoryObject(FactoryObject * fobject)
{
	fobjects.push_back(fobject);
}

int Factory::IsLoaded(const char * ResRef, SClass_ID type)
{
	for(unsigned int i = 0; i < fobjects.size(); i++) {
		if(fobjects[i]->SuperClassID == type) {
			if(strncmp(fobjects[i]->ResRef, ResRef, 8) == 0) {
				return i;
			}
		}
	}
	return -1;
}

FactoryObject * Factory::GetFactoryObject(int pos)
{
	return fobjects[pos];
}

void Factory::FreeObjects(void)
{
	for(unsigned int i = 0; i < fobjects.size(); i++) {
		delete(fobjects[i]);
	}
}
