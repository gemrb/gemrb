/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/AREImporter/AREImp.cpp,v 1.12 2003/11/28 09:37:40 balrog994 Exp $
 *
 */

#include "../../includes/win32def.h"
#include "AREImp.h"
#include "../Core/TileMapMgr.h"
#include "../Core/AnimationMgr.h"
#include "../Core/Interface.h"
#include "../Core/ActorMgr.h"
#include "../Core/FileStream.h"
#include "../Core/ImageMgr.h"

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
	int bigheader;
	if(strncmp(Signature, "AREAV1.0", 8) != 0) {
		if(strncmp(Signature, "AREAV9.1", 8) != 0)
			return false;
		else
			bigheader=16;
	}
	else
		bigheader=0;
	//TEST VERSION: SKIPPING VALUES
	str->Read(WEDResRef, 8);
	str->Seek(0x54+bigheader, GEM_STREAM_START);
	str->Read(&ActorOffset, 4);
	str->Read(&ActorCount, 2);
	str->Seek(0x7c+bigheader, GEM_STREAM_START);
	str->Read(&VerticesOffset, 4);
	str->Read(&VerticesCount, 2);
	str->Seek(0xa4+bigheader, GEM_STREAM_START);
	str->Read(&DoorsCount, 4);
	str->Read(&DoorsOffset, 4);
	//str->Seek(0xac, GEM_STREAM_START);
	//str->Seek(0xac+bigheader, GEM_STREAM_START);
	str->Read(&AnimCount, 4);
	str->Read(&AnimOffset, 4);
	str->Seek(8,GEM_CURRENT_POS); //skipping some
	str->Read(&SongHeader,4);
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

	ImageMgr * lm = (ImageMgr*)core->GetInterface(IE_BMP_CLASS_ID);
	char ResRef[9];
	strcpy(ResRef, WEDResRef);
	strcat(ResRef, "LM");
	DataStream * lmstr = core->GetResourceMgr()->GetResource(ResRef, IE_BMP_CLASS_ID);
	lm->Open(lmstr, true);
	TileMap * tm = tmm->GetTileMap();


	str->Seek(SongHeader, GEM_STREAM_START);
	//5 is the number of song indices
	for(int i=0;i<5;i++) {
	  str->Read(map->SongHeader.SongList+i, 4 );
	}

	//Loading Doors
	for(int i = 0; i < DoorsCount; i++) {
		str->Seek(DoorsOffset + (i*0xc8), GEM_STREAM_START);
		int count;
		unsigned long Flags, OpenFirstVertex, ClosedFirstVertex;
		unsigned short OpenVerticesCount, ClosedVerticesCount;
		char LongName[33], ShortName[9];
		short minX, maxX, minY, maxY;
		unsigned long cursor;
		Region BBClosed, BBOpen;
		str->Read(LongName, 32);
		LongName[32] = 0;
		str->Read(ShortName, 8);
		ShortName[8] = 0;
		str->Read(&Flags, 4);
		str->Read(&OpenFirstVertex, 4);
		str->Read(&OpenVerticesCount, 2);
		str->Read(&ClosedVerticesCount, 2);
		str->Read(&ClosedFirstVertex, 4);
		str->Read(&minX, 2);
		str->Read(&minY, 2);
		str->Read(&maxX, 2);
		str->Read(&maxY, 2);
		BBOpen.x = minX;
		BBOpen.y = minY;
		BBOpen.w = maxX-minX;
		BBOpen.h = maxY-minY;
		str->Read(&minX, 2);
		str->Read(&minY, 2);
		str->Read(&maxX, 2);
		str->Read(&maxY, 2);
		BBClosed.x = minX;
		BBClosed.y = minY;
		BBClosed.w = maxX-minX;
		BBClosed.h = maxY-minY;
		str->Seek(0x20, GEM_CURRENT_POS);
		str->Read(&cursor, 4);
		//Reading Open Polygon
		str->Seek(VerticesOffset + (OpenFirstVertex*4), GEM_STREAM_START);
		Point * points = (Point*)malloc(OpenVerticesCount*sizeof(Point));
		for(int x = 0; x < OpenVerticesCount; x++) {
			str->Read(&points[x].x, 2);
			str->Read(&points[x].y, 2);
		}
		Gem_Polygon * open = new Gem_Polygon(points, OpenVerticesCount);
		open->BBox = BBOpen;
		free(points);
		//Reading Closed Polygon
		str->Seek(VerticesOffset + (ClosedFirstVertex*4), GEM_STREAM_START);
		points = (Point*)malloc(ClosedVerticesCount*sizeof(Point));
		for(int x = 0; x < ClosedVerticesCount; x++) {
			str->Read(&points[x].x, 2);
			str->Read(&points[x].y, 2);
		}
		Gem_Polygon * closed = new Gem_Polygon(points, ClosedVerticesCount);
		closed->BBox = BBClosed;
		free(points);
		//Getting Door Information fro the WED File
		unsigned short * indices = tmm->GetDoorIndices(ShortName, &count);
		Door * door = tm->AddDoor(ShortName, (Flags&1 ? 0 : 1), indices, count, open, closed);
		door->Cursor = cursor;
	}
	//Loading Actors
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
		ActorBlock ab;
		ab.XPos = XPos;
		ab.YPos = YPos;
		ab.XDes = XDes;
		ab.YDes = YDes;
		ab.actor = actmgr->GetActor();
		ab.AnimID = IE_ANI_AWAKE;
		if(ab.actor->BaseStats[IE_STATE_ID] & STATE_DEAD)
			ab.AnimID = IE_ANI_SLEEP;
		ab.Orientation = (unsigned char)Orientation;
		map->AddActor(ab);
	}
	core->FreeInterface(actmgr);
	str->Seek(AnimOffset, GEM_STREAM_START);
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
	map->AddTileMap(tm, lm);
	core->FreeInterface(tmm);
	//core->FreeInterface(am);
	return map;
}
