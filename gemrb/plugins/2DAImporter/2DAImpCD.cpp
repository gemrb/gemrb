#include "2DAImpCD.h"
#include "2DAImp.h"

p2DAImpCD::p2DAImpCD(void)
{
}

p2DAImpCD::~p2DAImpCD(void)
{
}

void * p2DAImpCD::Create(void)
{
	return new p2DAImp();
}

const char* p2DAImpCD::ClassName(void)
{
	return "2DAImporter";
}

SClass_ID p2DAImpCD::SuperClassID(void)
{
	return IE_2DA_CLASS_ID;
}

Class_ID p2DAImpCD::ClassID(void)
{
	return Class_ID(0xa36513c6, 0x23755d73);
}

const char* p2DAImpCD::InternalName(void)
{
	return "2DAImp";
}
