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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/GAMImporter/GAMImp.cpp,v 1.18 2004/03/17 01:07:26 edheldil Exp $
 *
 */

#include "../../includes/win32def.h"
#include "../../includes/globals.h"
#include "GAMImp.h"
#include "../Core/Interface.h"
#include "../Core/MapMgr.h"
#include "../Core/MemoryStream.h"


GAMImp::GAMImp(void)
{
	str = NULL;
	autoFree = false;
}

GAMImp::~GAMImp(void)
{
	if (str && autoFree) {
		delete( str );
	}
}

bool GAMImp::Open(DataStream* stream, bool autoFree)
{
	if (stream == NULL) {
		return false;
	}
	if (str) {
		return false;
	}
	str = stream;
	this->autoFree = autoFree;
	char Signature[8];
	str->Read( Signature, 8 );
	if (strncmp( Signature, "GAMEV2.0", 8 ) == 0) {
		//soa, tob
		version = 20;
		PCSize = 0x160;
	} else if (strncmp( Signature, "GAMEV1.0", 8 ) == 0) {
		//bg1?
		version = 10;
		PCSize = 0x160;
	} else if (strncmp( Signature, "GAMEV1.1", 8 ) == 0) {
		//iwd, torment, totsc
		version = 11;
		if (stricmp( core->GameType, "pst" ) == 0)
			PCSize = 0x168;
		else if (stricmp( core->GameType, "iwd" ) == 0)
			PCSize = 0x180;
		else
			PCSize = 0x160;
	} else if (strncmp( Signature, "GAMEV2.2", 8 ) == 0) {
		//iwd2
		version = 22;
		PCSize = 0x340;
	} else {
		printf( "[GAMImporter]: This file is not a valid GAM File\n" );
		return false;
	}

	return true;
}

Game* GAMImp::GetGame()
{
	Game* newGame = new Game();


	str->Read( &newGame->GameTime, 4 );
	str->Read( &newGame->WhichFormation, 2 );
	for (int i = 0; i < 5; i++) {
		str->Read( &newGame->Formations[i], 2 );
	}
	str->Read( &newGame->PartyGold, 4 );
	str->Read( &newGame->Unknown1c, 4 ); //this is unknown yet
	str->Read( &newGame->PCOffset, 4 );
	str->Read( &newGame->PCCount, 4 );
	str->Read( &newGame->UnknownOffset, 4 );
	str->Read( &newGame->UnknownCount, 4 );
	str->Read( &newGame->NPCOffset, 4 );
	str->Read( &newGame->NPCCount, 4 );
	str->Read( &newGame->GLOBALOffset, 4 );
	str->Read( &newGame->GLOBALCount, 4 );
	str->Read( &newGame->CurrentArea, 8 );
	newGame->CurrentArea[8] = 0;
	str->Read( &newGame->Unknown48, 4 );
	str->Read( &newGame->JournalCount, 4 );
	str->Read( &newGame->JournalOffset, 4 );
	switch (version) {
		default:
			 {
				str->Read( &newGame->Reputation, 4 );
				str->Read( &newGame->CurrentArea, 8 ); // FIXME: see above
				newGame->CurrentArea[8] = 0;
				str->Read( &newGame->KillVarsOffset, 4 );
				str->Read( &newGame->KillVarsCount, 4 );
				str->Read( &newGame->FamiliarsOffset, 4 );

				str->Read( &newGame->Unknowns, 72 );
				// should read 
			}
			break;

		case 11:
			//iwd, torment, totsc
			 {
				str->Read( &newGame->UnknownOffset54, 4 );
				str->Read( &newGame->UnknownCount58, 4 );
				str->Read( &newGame->AnotherArea, 8 );
				newGame->AnotherArea[8] = 0;
				str->Read( &newGame->KillVarsOffset, 4 );
				str->Read( &newGame->KillVarsCount, 4 );
				str->Read( &newGame->FamiliarsOffset, 4 );
				str->Read( &newGame->AnotherArea, 8 );
				newGame->AnotherArea[8] = 0;  // FIXME: see above
				str->Read( &newGame->Unknowns, 64 );
			}	
			break;
	}

	newGame->globals = new Variables();
	newGame->globals->SetType( GEM_VARIABLES_INT );

	//Game* newGame = new Game();
	if (!newGame->CurrentArea[0]) {
		// 0 - single player, 1 - tutorial, 2 - multiplayer
		unsigned long playmode = 0;
		core->GetDictionary()->Lookup( "PlayMode", playmode );
		playmode *= 3;
		int i = core->LoadTable( "STARTARE" );
		TableMgr* tm = core->GetTable( i );
		char* resref = tm->QueryField( playmode );
		strncpy( newGame->CurrentArea, resref, 8 );
		newGame->CurrentArea[8] = 0;
	}

	int mi = newGame->LoadMap( newGame->CurrentArea );
	Map* newMap = newGame->GetMap( mi );

	//Loading PCs
	ActorMgr* aM = ( ActorMgr* ) core->GetInterface( IE_CRE_CLASS_ID );
	for (unsigned int i = 0; i < newGame->PCCount; i++) {
		str->Seek( newGame->PCOffset + ( i * PCSize ), GEM_STREAM_START );
		Actor *actor = GetActor( aM, true );

		if (stricmp( actor->Area, newGame->CurrentArea ) == 0)
			newMap->AddActor( actor );
		newGame->SetPC( actor );
	}

	//Loading NPCs
	for (unsigned int i = 0; i < newGame->NPCCount; i++) {
		str->Seek( newGame->NPCOffset + ( i * PCSize ), GEM_STREAM_START );
		Actor *actor = GetActor( aM, false );

		if (stricmp( actor->Area, newGame->CurrentArea ) == 0)
			newMap->AddActor( actor );
		newGame->AddNPC( actor );
	}
	core->FreeInterface( aM );

	//Loading GLOBALS
	str->Seek( newGame->GLOBALOffset, GEM_STREAM_START );
	for (unsigned int i = 0; i < newGame->GLOBALCount; i++) {
		char Name[33];
		unsigned long Value;
		str->Read( Name, 32 );
		Name[32] = 0;
		str->Seek( 8, GEM_CURRENT_POS );
		str->Read( &Value, 4 );
		str->Seek( 40, GEM_CURRENT_POS );
		newGame->globals->SetAt( Name, Value );
	}

	//Loading Journal entries
	str->Seek( newGame->JournalOffset, GEM_STREAM_START );
	for (unsigned int i = 0; i < newGame->JournalCount; i++) {
		GAMJournalEntry* je = GetJournalEntry();
		newGame->AddJournalEntry( je );
	}

	// Loading known creatures array (familiars)
	if (version == 11 && core->GetBeastsINI() != NULL) {
		int beasts_count = core->GetBeastsINI()->GetTagsCount();
		newGame->familiars = (ieByte*)malloc(beasts_count);
		str->Seek( newGame->FamiliarsOffset, GEM_STREAM_START );
		str->Read( newGame->familiars, beasts_count );
	}
	//newGame->AddMap(newMap);
	return newGame;
}

Actor* GAMImp::GetActor( ActorMgr* aM, bool is_in_party )
{
	PCStruct pcInfo;
	Actor* actor;


	str->Read( &pcInfo.Unknown0, 2 );
	str->Read( &pcInfo.PartyOrder, 2 );
	str->Read( &pcInfo.OffsetToCRE, 4 );
	str->Read( &pcInfo.CRESize, 4 );
	str->Read( &pcInfo.CREResRef, 8 );
	str->Read( &pcInfo.Orientation, 4 );
	str->Read( &pcInfo.Area, 8 );
	str->Read( &pcInfo.XPos, 2 );
	str->Read( &pcInfo.YPos, 2 );
	str->Read( &pcInfo.ViewXPos, 2 );
	str->Read( &pcInfo.ViewYPos, 2 );
	str->Read( &pcInfo.Unknown28, 100 );
	for (int i = 0; i < 4; i++) {
		str->Read( &pcInfo.QuickWeaponSlot[i], 2 );
	}
	str->Read( &pcInfo.Unknown94, 8 );
	for (int i = 0; i < 3; i++) {
		str->Read( &pcInfo.QuickSpellResRef[i], 8 );
	}
	for (int i = 0; i < 3; i++) {
		str->Read( &pcInfo.QuickItemSlot[i], 2 );
	}
	str->Read( &pcInfo.UnknownBA, 6 );

	if (pcInfo.OffsetToCRE) {
		str->Seek( pcInfo.OffsetToCRE, GEM_STREAM_START );
		void* Buffer = malloc( pcInfo.CRESize );
		str->Read( Buffer, pcInfo.CRESize );
		MemoryStream* ms = new MemoryStream( Buffer, pcInfo.CRESize );
		aM->Open( ms );
		actor = aM->GetActor();
	} else {
		DataStream* ds = core->GetResourceMgr()->GetResource( pcInfo.CREResRef,
								      IE_CRE_CLASS_ID );
		aM->Open( ds );
		actor = aM->GetActor();
	}

	actor->XPos = pcInfo.XPos;
	actor->YPos = pcInfo.YPos;
	actor->Orientation = pcInfo.Orientation;
	actor->InParty = is_in_party;
	actor->FromGame = true;
	actor->AnimID = IE_ANI_AWAKE;
	strcpy( actor->Area, pcInfo.Area );

	return actor;
}

GAMJournalEntry* GAMImp::GetJournalEntry()
{
	GAMJournalEntry* j = new GAMJournalEntry();

	str->Read( &j->Text, 4 );
	str->Read( &j->Time, 4 );
	str->Read( &j->Chapter, 1 );
	str->Read( &j->unknown09, 1 );
	str->Read( &j->Section, 1 );
	str->Read( &j->unknown0B, 1 );

	return j;
}
