#include "../../includes/win32def.h"
#include "AREImp.h"
#include "../Core/TileMapMgr.h"
#include "../Core/AnimationMgr.h"
#include "../Core/Interface.h"
#include "../Core/ActorMgr.h"
#include "../Core/FileStream.h"

AREImp::AREImp(void)
{
	autoFree = false;
	str = NULL;
}

AREImp::~AREImp(void)
{
	if(autoFree && str)
		delete(str);
}

bool AREImp::Open(DataStream * stream, bool autoFree)
{
	if(stream == NULL)
		return false;
	if(this->autoFree && str)
		delete(str);
	str = stream;
	this->autoFree = autoFree;
	char Signature[8];
	str->Read(Signature, 8);
	if(strncmp(Signature, "AREAV1.0", 8) != 0)
		return false;
	//TEST VERSION: SKIPPING VALUES
	str->Read(WEDResRef, 8);
	str->Seek(0x54, GEM_STREAM_START);
	str->Read(&ActorOffset, 4);
	str->Read(&ActorCount, 2);
	str->Seek(0xac, GEM_STREAM_START);
	str->Read(&AnimCount, 4);
	str->Read(&AnimOffset, 4);
	return true;
}

Map * AREImp::GetMap()
{
	Map * map = new Map();

	if(!core->IsAvailable(IE_WED_CLASS_ID)) {
		printf("[AREImporter]: No Tile Map Manager Available.\n");
		return false;
	}
	TileMapMgr * tmm = (TileMapMgr*)core->GetInterface(IE_WED_CLASS_ID);
	DataStream * wedfile = core->GetResourceMgr()->GetResource(WEDResRef, IE_WED_CLASS_ID);
	tmm->Open(wedfile);
	map->AddTileMap(tmm->GetTileMap());
	core->FreeInterface(tmm);
	str->Seek(ActorOffset, GEM_STREAM_START);
	if(!core->IsAvailable(IE_CRE_CLASS_ID)) {
		printf("[AREImporter]: No Actor Manager Available, skipping actors\n");
		return map;
	}
	ActorMgr * actmgr = (ActorMgr*)core->GetInterface(IE_CRE_CLASS_ID);
	for(int i = 0; i < ActorCount; i++) {
		char		   CreResRef[8];
		unsigned long  Orientation;
		unsigned short XPos, YPos, XDes, YDes;
		str->Seek(32, GEM_CURRENT_POS);
		str->Read(&XPos, 2);
		str->Read(&YPos, 2);
		str->Read(&XDes, 2);
		str->Read(&YDes, 2);
		str->Seek(12, GEM_CURRENT_POS);
		str->Read(&Orientation, 4);
		str->Seek(72, GEM_CURRENT_POS);
		str->Read(CreResRef, 8);
		DataStream * crefile;
		unsigned long CreOffset, CreSize;
		str->Read(&CreOffset, 4);
		str->Read(&CreSize, 4);
		str->Seek(128, GEM_CURRENT_POS);
		if(CreOffset != 0) {
			char cpath[_MAX_PATH];
			strcpy(cpath, core->GamePath);
			strcat(cpath, str->filename);
			FILE * str = fopen(cpath, "rb");
			FileStream * fs = new FileStream();
			fs->Open(str, CreOffset, CreSize, true);
			crefile = fs;
		}
		else {
			crefile = core->GetResourceMgr()->GetResource(CreResRef, IE_CRE_CLASS_ID);
		}
		actmgr->Open(crefile, true);
		Actor * actor = actmgr->GetActor();
		actor->XPos = XPos;
		actor->XDes = XDes;
		actor->YPos = YPos;
		actor->YDes = YDes;
		actor->Orientation = Orientation;
		map->AddActor(actor);
	}
	str->Seek(AnimOffset, GEM_STREAM_START);
	core->FreeInterface(actmgr);
	if(!core->IsAvailable(IE_BAM_CLASS_ID)) {
		printf("[AREImporter]: No Animation Manager Available, skipping animations\n");
		return map;
	}
	//AnimationMgr * am = (AnimationMgr*)core->GetInterface(IE_BAM_CLASS_ID);
	for(unsigned int i = 0; i < AnimCount; i++) {
		Animation * anim;
		str->Seek(32, GEM_CURRENT_POS);
		unsigned short animX, animY;
		str->Read(&animX, 2);
		str->Read(&animY, 2);
		str->Seek(4, GEM_CURRENT_POS);
		char animBam[9];
		str->Read(animBam, 8);
		animBam[8] = 0;
		unsigned short animCycle, animFrame;
		str->Read(&animCycle, 2);
		str->Read(&animFrame, 2);
		unsigned long animFlags;
		str->Read(&animFlags, 4);
		str->Seek(20, GEM_CURRENT_POS);
		unsigned char mode = ((animFlags & 2) != 0) ? IE_SHADED : IE_NORMAL;
		//am->Open(core->GetResourceMgr()->GetResource(animBam, IE_BAM_CLASS_ID), true);
		//anim = am->GetAnimation(animCycle, animX, animY);
		AnimationFactory * af =  (AnimationFactory*)core->GetResourceMgr()->GetFactoryResource(animBam, IE_BAM_CLASS_ID, mode);
		anim = af->GetCycle(animCycle);
		anim->x = animX;
		anim->y = animY;
		map->AddAnimation(anim);		
	}
	//core->FreeInterface(am);
	return map;
}
