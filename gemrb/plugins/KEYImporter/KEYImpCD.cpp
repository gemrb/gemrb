#include "KEYImpCD.h"
#include "KeyImp.h"

KEYImpCD::KEYImpCD(void)
{
}

KEYImpCD::~KEYImpCD(void)
{
}

void * KEYImpCD::Create()
{
	return new KeyImp();
}

const char* KEYImpCD::ClassName(void)
{
	return "KEYImporter";
}

SClass_ID KEYImpCD::SuperClassID(void)
{
	return IE_KEY_CLASS_ID;
}

Class_ID KEYImpCD::ClassID(void)
{
	return Class_ID(0x982da6f8, 0x47a4128f);
}

const char* KEYImpCD::InternalName(void)
{
	return "KEYImp";
}
