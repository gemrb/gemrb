#include "PLTImpCD.h"
#include "PLTImp.h"

PLTImpCD::PLTImpCD(void)
{
}

PLTImpCD::~PLTImpCD(void)
{
}

void * PLTImpCD::Create(void)
{
	return new PLTImp();
}

const char* PLTImpCD::ClassName(void)
{
	return "PLTImporter";
}

SClass_ID PLTImpCD::SuperClassID(void)
{
	return IE_PLT_CLASS_ID;
}


Class_ID PLTImpCD::ClassID(void)
{
	return Class_ID(0x61725ade, 0x33ac4005);
}

const char* PLTImpCD::InternalName(void)
{
	return "PLTImp";
}
