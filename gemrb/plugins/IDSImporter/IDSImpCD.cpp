#include "IDSImpCD.h"
#include "IDSImp.h"

IDSImpCD::IDSImpCD(void)
{
}

IDSImpCD::~IDSImpCD(void)
{
}

void * IDSImpCD::Create(void)
{
	return new IDSImp();
}

const char * IDSImpCD::ClassName(void)
{
	return "IDSImporter";
}

SClass_ID IDSImpCD::SuperClassID(void)
{
	return IE_IDS_CLASS_ID;
}

Class_ID IDSImpCD::ClassID(void)
{
	return Class_ID(0x2a537210, 0xca573a46);
}
