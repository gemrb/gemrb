#include "CREImp.h"
#include "../Core/Interface.h"
#include "../Core/HCAnimationSeq.h"

CREImp::CREImp(void)
{
	str = NULL;
	autoFree = false;
}

CREImp::~CREImp(void)
{
	if(str && autoFree)
		delete(str);
}

bool CREImp::Open(DataStream * stream, bool autoFree)
{
	if(stream == NULL)
		return false;
	if(str && this->autoFree)
		delete(str);
	str = stream;
	this->autoFree = autoFree;
	char Signature[8];
	str->Read(Signature, 8);
	if(strncmp(Signature, "CRE V1.0", 8) != 0) {
		printf("[CREImporter]: Not a CRE File or File Version not supported: %8s\n", Signature);
		return false;
	}
	return true;
}

Actor * CREImp::GetActor()
{
	Actor * act = new Actor();
	unsigned long strref;
	str->Read(&strref, 4);
	act->LongName = core->GetString(strref);
	str->Read(&strref, 4);
	act->ShortName = core->GetString(strref);
	str->Seek(0x28, GEM_STREAM_START);
	str->Read(&act->Animation, 2);
	str->Seek(0x2c, GEM_STREAM_START);
	str->Read(&act->Metal, 1);
	str->Read(&act->Minor, 1);
	str->Read(&act->Major, 1);
	str->Read(&act->Skin, 1);
	str->Read(&act->Leather, 1);
	str->Read(&act->Armor, 1);
	str->Read(&act->Hair, 1);
	str->Seek(0x272, GEM_STREAM_START);
	str->Read(&act->Race, 1);
	str->Read(&act->Class, 1);
	str->Seek(1, GEM_CURRENT_POS);
	str->Read(&act->Gender, 1);
	core->GetHCAnim(act);
	return act;
}
