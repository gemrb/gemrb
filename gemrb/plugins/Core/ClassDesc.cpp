#include "win32def.h"
#include "ClassDesc.h"

ClassDesc::ClassDesc(void)
{
}

ClassDesc::~ClassDesc(void)
{
}

int ClassDesc::BeginCreate()
{
	return 0;
}

int ClassDesc::EndCreate()
{
	return 0;
}

Class_ID ClassDesc::SubClassID(void)
{
	return Class_ID();
}

const char* ClassDesc::InternalName(void)
{
	return 0;
}
