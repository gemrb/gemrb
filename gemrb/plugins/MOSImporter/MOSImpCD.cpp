#include "MOSImpCD.h"
#include "MOSImp.h"

MOSImpCD::MOSImpCD(void)
{
}

MOSImpCD::~MOSImpCD(void)
{
}

void * MOSImpCD::Create(void)
{
	return new MOSImp();
}

const char* MOSImpCD::ClassName(void)
{
	return "MOSImporter";
}

SClass_ID MOSImpCD::SuperClassID(void)
{
	return IE_MOS_CLASS_ID;
}


Class_ID MOSImpCD::ClassID(void)
{
	return Class_ID(0x763abc16, 0x19a65410);
}

const char* MOSImpCD::InternalName(void)
{
	return "MOSImp";
}
