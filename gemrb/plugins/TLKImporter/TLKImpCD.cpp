#include "TLKImpCD.h"
#include "TLKImp.h"

TLKImpCD::TLKImpCD(void)
{
}

TLKImpCD::~TLKImpCD(void)
{
}

void * TLKImpCD::Create(void)
{
	return new TLKImp();
}

const char * TLKImpCD::ClassName(void)
{
	return "TLKImporter";
}

SClass_ID TLKImpCD::SuperClassID(void)
{
	return IE_TLK_CLASS_ID;
}

Class_ID TLKImpCD::ClassID(void)
{
	return Class_ID(0x1a736290, 0xcc174a47);
}
