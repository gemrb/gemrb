#include "BG2HCAnimCD.h"
#include "BG2HCAnim.h"

BG2HCAnimCD::BG2HCAnimCD(void)
{
}

BG2HCAnimCD::~BG2HCAnimCD(void)
{
}

void * BG2HCAnimCD::Create(void)
{
	return new BG2HCAnim();
}

const char* BG2HCAnimCD::ClassName(void)
{
	return "BG2HCAnimSeq";
}

SClass_ID BG2HCAnimCD::SuperClassID(void)
{
	return IE_HCANIMS_CLASS_ID;
}

Class_ID BG2HCAnimCD::ClassID(void)
{
	return Class_ID(0x00000000, 0x715a0356);
}

const char* BG2HCAnimCD::InternalName(void)
{
	return "BG2HCAnimSeq";
}
