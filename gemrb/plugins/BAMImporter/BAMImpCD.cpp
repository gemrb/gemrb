#include "BAMImpCD.h"
#include "BAMImp.h"

BAMImpCD::BAMImpCD(void)
{
}

BAMImpCD::~BAMImpCD(void)
{
}

void * BAMImpCD::Create(void)
{
	return new BAMImp();
}

const char* BAMImpCD::ClassName(void)
{
	return "BAMImporter";
}

SClass_ID BAMImpCD::SuperClassID(void)
{
	return IE_BAM_CLASS_ID;
}

Class_ID BAMImpCD::ClassID(void)
{
	return Class_ID(0xa2b510c6, 0x037a5db3);
}

const char* BAMImpCD::InternalName(void)
{
	return "BAMImp";
}
