#include "MUSImpCD.h"
#include "MUSImp.h"

MUSImpCD::MUSImpCD(void)
{
}

MUSImpCD::~MUSImpCD(void)
{
}

void * MUSImpCD::Create(void)
{
	return new MUSImp();
}

const char* MUSImpCD::ClassName(void)
{
	return "MUSImporter";
}

SClass_ID MUSImpCD::SuperClassID(void)
{
	return IE_MUS_CLASS_ID;
}


Class_ID MUSImpCD::ClassID(void)
{
	return Class_ID(0x6adc1420, 0xCC241263);
}

const char* MUSImpCD::InternalName(void)
{
	return "MUSImp";
}
