#include "MVEPlay.h"
#include "MVEPlayerDesc.h"

MVEPlayerDesc::MVEPlayerDesc(void)
{
}

MVEPlayerDesc::~MVEPlayerDesc(void)
{
}

void * MVEPlayerDesc::Create(void)
{
	return new MVEPlay();
}

const char* MVEPlayerDesc::ClassName(void)
{
	return "MVEPlayer";
}

SClass_ID MVEPlayerDesc::SuperClassID(void)
{
	return IE_MVE_CLASS_ID;
}


Class_ID MVEPlayerDesc::ClassID(void)
{
	return Class_ID(0x5a271846, 0x982648af);
}

const char* MVEPlayerDesc::InternalName(void)
{
	return "MVEPlayer";
}
