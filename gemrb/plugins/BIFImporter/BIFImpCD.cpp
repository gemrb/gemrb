#include "BIFImp.h"
#include "BIFImpCD.h"

BIFImpCD::BIFImpCD(void)
{
}

BIFImpCD::~BIFImpCD(void)
{
}

void * BIFImpCD::Create(void)
{
	return new BIFImp();
}

const char* BIFImpCD::ClassName(void)
{
	return "BIFImporter";
}

SClass_ID BIFImpCD::SuperClassID(void)
{
	return IE_BIF_CLASS_ID;
}


Class_ID BIFImpCD::ClassID(void)
{
	return Class_ID(0x578a3f51, 0xd863ed17);
}

const char* BIFImpCD::InternalName(void)
{
	return "BIFImp";
}
