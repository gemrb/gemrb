#include "SDLVideoDC.h"
#include "SDLVideoDriver.h"

SDLVideoDC::SDLVideoDC(void)
{
}

SDLVideoDC::~SDLVideoDC(void)
{
}

void * SDLVideoDC::Create(void)
{
	return new SDLVideoDriver();
}

const char * SDLVideoDC::ClassName(void)
{
	return "SDLVideo";
}

SClass_ID SDLVideoDC::SuperClassID(void)
{
	return IE_VIDEO_CLASS_ID;
}

Class_ID SDLVideoDC::ClassID(void)
{
	return Class_ID(0x67402f81, 0x4b9217d6);
}
