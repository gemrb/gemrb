#include "CREImpCD.h"
#include "CREImp.h"

CREImpCD::CREImpCD(void)
{
}

CREImpCD::~CREImpCD(void)
{
}

void * CREImpCD::Create(void)
{
	return new CREImp();
}

const char* CREImpCD::ClassName(void)
{
	return "CREImporter";
}

SClass_ID CREImpCD::SuperClassID(void)
{
	return IE_CRE_CLASS_ID;
}


Class_ID CREImpCD::ClassID(void)
{
	return Class_ID(0xd006381a, 0x192affd2);
}

const char* CREImpCD::InternalName(void)
{
	return "CREImp";
}
