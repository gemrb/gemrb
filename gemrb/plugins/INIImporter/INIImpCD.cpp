#include "INIImpCD.h"
#include "INIImp.h"

INIImpCD::INIImpCD(void)
{
}

INIImpCD::~INIImpCD(void)
{
}

void * INIImpCD::Create(void)
{
	return new INIImp();
}

const char* INIImpCD::ClassName(void)
{
	return "INIImporter";
}

SClass_ID INIImpCD::SuperClassID(void)
{
	return IE_INI_CLASS_ID;
}

Class_ID INIImpCD::ClassID(void)
{
	return Class_ID(0x581ad362, 0x770acd3e);
}

const char* INIImpCD::InternalName(void)
{
	return "INIImp";
}
