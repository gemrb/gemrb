#include "BMPImpCD.h"
#include "BMPImp.h"

BMPImpCD::BMPImpCD(void)
{
}

BMPImpCD::~BMPImpCD(void)
{
}

void * BMPImpCD::Create(void)
{
	return new BMPImp();
}

const char* BMPImpCD::ClassName(void)
{
	return "BMPImporter";
}

SClass_ID BMPImpCD::SuperClassID(void)
{
	return IE_BMP_CLASS_ID;
}


Class_ID BMPImpCD::ClassID(void)
{
	return Class_ID(0x5183026a, 0xf09871a3);
}

const char* BMPImpCD::InternalName(void)
{
	return "BMPImp";
}
