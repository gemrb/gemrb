#include "AREImpCD.h"
#include "AREImp.h"

AREImpCD::AREImpCD(void)
{
}

AREImpCD::~AREImpCD(void)
{
}

void * AREImpCD::Create(void)
{
	return new AREImp();
}

const char* AREImpCD::ClassName(void)
{
	return "AREImporter";
}

SClass_ID AREImpCD::SuperClassID(void)
{
	return IE_ARE_CLASS_ID;
}

Class_ID AREImpCD::ClassID(void)
{
	return Class_ID(0x7a520471, 0xacb2417c);
}

const char* AREImpCD::InternalName(void)
{
	return "AREImp";
}
