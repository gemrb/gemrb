#include "WEDImpCD.h"
#include "WEDImp.h"

WEDImpCD::WEDImpCD(void)
{
}

WEDImpCD::~WEDImpCD(void)
{
}

void * WEDImpCD::Create(void)
{
	return new WEDImp();
}

const char * WEDImpCD::ClassName(void)
{
	return "WEDImp";
}

SClass_ID WEDImpCD::SuperClassID(void)
{
	return IE_WED_CLASS_ID;
}

Class_ID WEDImpCD::ClassID(void)
{
	return Class_ID(0x078a4123, 0xafc417de);
}
