/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/GAMImporter/GAMImp.cpp,v 1.5 2003/12/19 23:05:54 balrog994 Exp $
 *
 */

#include "../../includes/win32def.h"
#include "../../includes/globals.h"
#include "GAMImp.h"
#include "../Core/Interface.h"
#include "../Core/MapMgr.h"
#include "../Core/MemoryStream.h"

static Variables * globals;

GAMImp::GAMImp(void)
{
	str = NULL;
	autoFree = false;
}

GAMImp::~GAMImp(void)
{

	if(str && autoFree)
		delete(str);
}

bool GAMImp::Open(DataStream * stream, bool autoFree)
{
	if(stream == NULL)
		return false;
	if(str)
		return false;
	str = stream;
	this->autoFree = autoFree;
	char Signature[8];
	str->Read(Signature, 8);
	if(strncmp(Signature, "GAMEV2.0", 8) == 0) { //soa, tob
		version=20;
		PCSize = 0x160;
	}
	else if(strncmp(Signature, "GAMEV1.0", 8) == 0) { //bg1?
		version=10;
		PCSize = 0x160;
	}
	else if(strncmp(Signature, "GAMEV1.1", 8) == 0) {//iwd, torment, totsc
		version=11;
		if(stricmp(core->GameType, "pst") == 0)
			PCSize = 0x168;
		else if(stricmp(core->GameType, "iwd") == 0)
			PCSize = 0x180;
		else
			PCSize = 0x160;
	}
	else if(strncmp(Signature, "GAMEV2.2", 8) == 0) {//iwd2
		version=22;
		PCSize = 0x160;
	}
	else {
                printf("[GAMImporter]: This file is not a valid GAM File\n");
                return false;
        }

	str->Read(&GameTime,4);
	str->Read(&WhichFormation,2);
	for(int i=0;i<5;i++) {
		str->Read(Formations+i,2);
	}
	str->Read(&PartyGold,4);
	str->Read(&Unknown1c,4); //this is unknown yet
	str->Read(&PCOffset,4);
	str->Read(&PCCount,4);
	str->Read(&UnknownOffset,4);
	str->Read(&UnknownCount,4);
	str->Read(&NPCOffset,4);
	str->Read(&NPCCount,4);
	str->Read(&GLOBALOffset, 4);
	str->Read(&GLOBALCount, 4);
	str->Read(&CurrentArea, 8);
	CurrentArea[8] = 0;
	str->Read(&Unknown48, 4);
	str->Read(&JournalCount, 4);
	str->Read(&JournalOffset, 4);
	switch(version) {
		default: 
			{
				str->Read(&Unknown54, 4);
				str->Read(&CurrentArea, 8);
				CurrentArea[8] = 0;
				str->Read(&Unknowns, 84);
			}
		break;

		case 11: //iwd, torment, totsc
			{
				str->Read(&UnknownOffset54, 4);
				str->Read(&UnknownCount58, 4);
				str->Read(&CurrentArea, 8);
				CurrentArea[8] = 0;
				str->Read(&KillVarsOffset, 4);
				str->Read(&KillVarsCount, 4);
				str->Read(&SomeBytesArrayOffset, 4);
				str->Read(&AnotherArea, 8);
				AnotherArea[8] = 0;
				str->Read(&Unknowns, 64);
			}	
		break;
	}
	return true;
}

Game * GAMImp::GetGame()
{
	if(globals)
		globals->RemoveAll();
	Game * newGame = new Game();
	MapMgr * mM = (MapMgr*)core->GetInterface(IE_ARE_CLASS_ID);
	if(!CurrentArea[0]) {
		int i = core->LoadTable("STARTARE");
		TableMgr * tm = core->GetTable(i);
		char * resref = tm->QueryField();
		strncpy(CurrentArea, resref, 8);
		CurrentArea[8] = 0;
	}
	DataStream * ds = core->GetResourceMgr()->GetResource(CurrentArea, IE_ARE_CLASS_ID);
	mM->Open(ds, true);
	Map * newMap = mM->GetMap();
	core->FreeInterface(mM);
	//Loading PCs
	ActorMgr * aM = (ActorMgr*)core->GetInterface(IE_CRE_CLASS_ID);
	for(unsigned int i = 0; i < PCCount; i++) {
		str->Seek(PCOffset+(i*PCSize), GEM_STREAM_START);
		PCStruct pcInfo;
		str->Read(&pcInfo, 0xC0);
		if(stricmp(pcInfo.Area, CurrentArea) != 0)
			continue;
		str->Seek(pcInfo.OffsetToCRE, GEM_STREAM_START);
		void *Buffer = malloc(pcInfo.CRESize);
		str->Read(Buffer, pcInfo.CRESize);
		MemoryStream * ms = new MemoryStream(Buffer, pcInfo.CRESize);
		aM->Open(ms);
		Actor * actor = aM->GetActor();
		actor->XPos = pcInfo.XPos;
		actor->YPos = pcInfo.YPos;
		actor->InParty = true;
		newMap->AddActor(actor);
		newGame->SetPC(actor);
		delete(ms);
	}
	//Loading NPCs
	for(unsigned int i = 0; i < NPCCount; i++) {
		str->Seek(NPCOffset+(i*PCSize), GEM_STREAM_START);
		PCStruct pcInfo;
		str->Read(&pcInfo, 0xC0);
		if(stricmp(pcInfo.Area, CurrentArea) != 0)
			continue;
		Actor * actor;
		if(pcInfo.OffsetToCRE) {
			str->Seek(pcInfo.OffsetToCRE, GEM_STREAM_START);
			void *Buffer = malloc(pcInfo.CRESize);
			str->Read(Buffer, pcInfo.CRESize);
			MemoryStream * ms = new MemoryStream(Buffer, pcInfo.CRESize);
			aM->Open(ms);
			actor = aM->GetActor();
		} else {
			DataStream * ds = core->GetResourceMgr()->GetResource(pcInfo.CREResRef, IE_CRE_CLASS_ID);
			aM->Open(ds);
			actor = aM->GetActor();
		}
		actor->XPos = pcInfo.XPos;
		actor->YPos = pcInfo.YPos;
		actor->InParty = false;
		actor->AnimID = IE_ANI_AWAKE;
		newMap->AddActor(actor);
	}
	core->FreeInterface(aM);
	//Loading GLOBALS
	str->Seek(GLOBALOffset, GEM_STREAM_START);
	for(unsigned int i = 0; i < GLOBALCount; i++) {
		char Name[33];
		unsigned long Value;
		str->Read(Name, 32);
		Name[32] = 0;
		str->Seek(8, GEM_CURRENT_POS);
		str->Read(&Value, 4);
		str->Seek(40, GEM_CURRENT_POS);
		globals->SetAt(Name, Value);
	}
	newGame->AddMap(newMap);
	return newGame;
}
