#include "ZLibMgrDesc.h"
#include "ZLibManager.h"

ZLibMgrDesc::ZLibMgrDesc(void)
{
}

ZLibMgrDesc::~ZLibMgrDesc(void)
{
}

void * ZLibMgrDesc::Create(void)
{
	return new ZLibManager();
}

const char* ZLibMgrDesc::ClassName(void)
{
	return "ZLibCM";
}

SClass_ID ZLibMgrDesc::SuperClassID(void)
{
	return IE_COMPRESSION_CLASS_ID;
}

Class_ID ZLibMgrDesc::ClassID(void)
{
	return Class_ID(0xA1C8E1D1, 0xE7AA2BE5);
}
