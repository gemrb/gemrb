#include "ACMImporterCD.h"
#include "ACMImp.h"

ACMImporterCD::ACMImporterCD(void)
{
}

ACMImporterCD::~ACMImporterCD(void)
{
}

void * ACMImporterCD::Create(void)
{
	return new ACMImp();
}

const char* ACMImporterCD::ClassName(void)
{
	return "ACMImporter";
}

SClass_ID ACMImporterCD::SuperClassID(void)
{
	return IE_WAV_CLASS_ID;
}

Class_ID ACMImporterCD::ClassID(void)
{
	return Class_ID(0x905a0ac1, 0xf817af6c);
}

const char* ACMImporterCD::InternalName(void)
{
	return "ACMImp";
}
