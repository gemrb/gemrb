#include "DirectXVideoDC.h"
#include "DirectXVideoDriver.h"

DirectXVideoDC::DirectXVideoDC(void)
{
}

DirectXVideoDC::~DirectXVideoDC(void)
{
}

void * DirectXVideoDC::Create(void)
{
	return new DirectXVideoDriver();
}

const char * DirectXVideoDC::ClassName(void)
{
	return "DirectXVideo";
}

SClass_ID DirectXVideoDC::SuperClassID(void)
{
	return IE_VIDEO_CLASS_ID;
}

Class_ID DirectXVideoDC::ClassID(void)
{
	return Class_ID(0x9916afcd, 0x87adce12);
}
