#include "CHUImp.h"
#include "CHUImpCD.h"

CHUImpCD::CHUImpCD(void)
{
}

CHUImpCD::~CHUImpCD(void)
{
}

void * CHUImpCD::Create(void)
{
	return new CHUImp();
}

const char* CHUImpCD::ClassName(void)
{
	return "CHUImporter";
}

SClass_ID CHUImpCD::SuperClassID(void)
{
	return IE_CHU_CLASS_ID;
}

Class_ID CHUImpCD::ClassID(void)
{
	return Class_ID(0xc018af34, 0x10a63725);
}

const char* CHUImpCD::InternalName(void)
{
	return "CHUImp";
}
