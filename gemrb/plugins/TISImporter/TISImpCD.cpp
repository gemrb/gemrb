#include "TISImpCD.h"
#include "TISImp.h"

TISImpCD::TISImpCD(void)
{
}

TISImpCD::~TISImpCD(void)
{
}

void * TISImpCD::Create(void)
{
	return new TISImp();
}

const char * TISImpCD::ClassName(void)
{
	return "TISImporter";
}

SClass_ID TISImpCD::SuperClassID(void)
{
	return IE_TIS_CLASS_ID;
}

Class_ID TISImpCD::ClassID(void)
{
	return Class_ID(0x3651adc9, 0x47adce38);
}
