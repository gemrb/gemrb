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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/GAMImporter/GAMImp.cpp,v 1.50 2005/04/29 21:53:01 avenger_teambg Exp $
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
		if (stricmp( core->GameType, "pst" ) == 0) {
			PCSize = 0x168;
			version = 12;
		}
		else if ( (stricmp( core->GameType, "iwd" ) == 0) 
			|| (stricmp( core->GameType, "how" ) == 0) ) {
			PCSize = 0x180;
			version = 11;
		}
		else {
			PCSize = 0x160;
			version=10;
		}
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
	unsigned int i;
	Game* newGame = new Game();

	str->ReadDword( &newGame->GameTime );
	str->ReadWord( &newGame->WhichFormation );
	for (i = 0; i < 5; i++) {
		str->ReadWord( &newGame->Formations[i] );
	}
	str->ReadDword( &newGame->PartyGold );
	str->ReadDword( &newGame->WeatherBits ); 
	ieDword PCOffset, PCCount;
	ieDword UnknownOffset, UnknownCount;
	ieDword NPCOffset, NPCCount;
	ieDword GlobalOffset, GlobalCount;
	ieDword JournalOffset, JournalCount;
	ieDword KillVarsOffset, KillVarsCount;
	ieDword FamiliarsOffset;

	str->ReadDword( &PCOffset );
	str->ReadDword( &PCCount );
	str->ReadDword( &UnknownOffset );
	str->ReadDword( &UnknownCount );
	str->ReadDword( &NPCOffset );
	str->ReadDword( &NPCCount );
	str->ReadDword( &GlobalOffset );
	str->ReadDword( &GlobalCount );
	str->ReadResRef( newGame->CurrentArea );
	str->ReadDword( &newGame->Unknown48 );
	str->ReadDword( &JournalCount );
	str->ReadDword( &JournalOffset );
	switch (version) {
		default:
			str->ReadDword( &newGame->Reputation );
			str->ReadResRef( newGame->CurrentArea ); // FIXME: see above
			str->ReadDword( &newGame->ControlStatus );
			str->ReadDword( &KillVarsCount ); //this is still unknown
			str->ReadDword( &FamiliarsOffset );
			str->Seek( 72, GEM_CURRENT_POS);
			break;

		case 12:
			//this is another unknown offset/count, but who cares
			str->ReadDword( &UnknownOffset );
			str->ReadDword( &UnknownCount );
			str->ReadResRef( newGame->AnotherArea );
			str->ReadDword( &KillVarsOffset );
			str->ReadDword( &KillVarsCount );
			str->ReadDword( &FamiliarsOffset );
			str->Seek( 72, GEM_CURRENT_POS);
			break;
	}

	newGame->globals = new Variables();
	newGame->globals->SetType( GEM_VARIABLES_INT );

	if (!newGame->CurrentArea[0]) {
		// 0 - single player, 1 - tutorial, 2 - multiplayer
		ieDword playmode = 0;
		core->GetDictionary()->Lookup( "PlayMode", playmode );
		playmode *= 3;
		int i = core->LoadTable( "STARTARE" );
		TableMgr* tm = core->GetTable( i );
		char* resref = tm->QueryField( playmode );
		strnuprcpy( newGame->CurrentArea, resref, 8 );
	}

	//Loading PCs
	ActorMgr* aM = ( ActorMgr* ) core->GetInterface( IE_CRE_CLASS_ID );
	for (i = 0; i < PCCount; i++) {
		str->Seek( PCOffset + ( i * PCSize ), GEM_STREAM_START );
		Actor *actor = GetActor( aM, true );
		newGame->JoinParty( actor, false );
	}

	//Loading NPCs
	for (i = 0; i < NPCCount; i++) {
		str->Seek( NPCOffset + ( i * PCSize ), GEM_STREAM_START );
		Actor *actor = GetActor( aM, false );
		newGame->AddNPC( actor );
	}
	core->FreeInterface( aM );

	//Loading Global Variables
	char Name[33];
	Name[32] = 0;
	str->Seek( GlobalOffset, GEM_STREAM_START );
	for (i = 0; i < GlobalCount; i++) {
		ieDword Value;
		str->Read( Name, 32 );
		str->Seek( 8, GEM_CURRENT_POS );
		str->ReadDword( &Value );
		str->Seek( 40, GEM_CURRENT_POS );
		newGame->globals->SetAt( Name, Value );
	}
	if(core->HasFeature(GF_HAS_KAPUTZ) ) {
		newGame->kaputz = new Variables();
		newGame->kaputz->SetType( GEM_VARIABLES_INT );
		str->Seek( KillVarsOffset, GEM_STREAM_START );
		for (i = 0; i < KillVarsCount; i++) {
			ieDword Value;
			str->Read( Name, 32 );
			str->Seek( 8, GEM_CURRENT_POS );
			str->ReadDword( &Value );
			str->Seek( 40, GEM_CURRENT_POS );
			newGame->kaputz->SetAt( Name, Value );
		}
	}

	//Loading Journal entries
	str->Seek( JournalOffset, GEM_STREAM_START );
	for (i = 0; i < JournalCount; i++) {
		GAMJournalEntry* je = GetJournalEntry();
		newGame->AddJournalEntry( je );
	}

	// Loading known creatures array (beasts)
	if (version == 12 && core->GetBeastsINI() != NULL) {
		int beasts_count = core->GetBeastsINI()->GetTagsCount();
		newGame->beasts = (ieByte*)malloc(beasts_count);
		str->Seek( FamiliarsOffset, GEM_STREAM_START );
		str->Read( newGame->beasts, beasts_count );
	}

	return newGame;
}

Actor* GAMImp::GetActor( ActorMgr* aM, bool is_in_party )
{
	unsigned int i;
	PCStruct pcInfo;
	Actor* actor;

	str->ReadWord( &pcInfo.Selected );
	str->ReadWord( &pcInfo.PartyOrder );
	str->ReadDword( &pcInfo.OffsetToCRE );
	str->ReadDword( &pcInfo.CRESize );
	str->ReadResRef( pcInfo.CREResRef );
	str->ReadDword( &pcInfo.Orientation );
	str->ReadResRef( pcInfo.Area );
	str->ReadWord( &pcInfo.XPos );
	str->ReadWord( &pcInfo.YPos );
	str->ReadWord( &pcInfo.ViewXPos );
	str->ReadWord( &pcInfo.ViewYPos );
	str->Read( &pcInfo.Unknown28, 100 );
	for (i = 0; i < 4; i++) {
		str->ReadWord( &pcInfo.QuickWeaponSlot[i] );
	}
	str->Read( &pcInfo.Unknown94, 8 );
	for (i = 0; i < 3; i++) {
		str->Read( &pcInfo.QuickSpellResRef[i], 8 );
	}
	for (i = 0; i < 3; i++) {
		str->ReadWord( &pcInfo.QuickItemSlot[i] );
	}
	str->Read( &pcInfo.UnknownBA, 6 );
	if (version==22) { //skipping some bytes for iwd2
		str->Seek( 254, GEM_CURRENT_POS);
	}
	str->Read( &pcInfo.Name, 32 );
	if (version==12) { //Torment
		str->Seek( 8, GEM_CURRENT_POS);
	}
	str->ReadDword( &pcInfo.TalkCount );

	PCStatsStruct* ps = GetPCStats();

	if (pcInfo.OffsetToCRE) {
		str->Seek( pcInfo.OffsetToCRE, GEM_STREAM_START );
		void* Buffer = malloc( pcInfo.CRESize );
		str->Read( Buffer, pcInfo.CRESize );
		MemoryStream* ms = new MemoryStream( Buffer, pcInfo.CRESize );
		aM->Open( ms );
		actor = aM->GetActor();
		if(pcInfo.Name[0]!=0 && pcInfo.Name[0]!=-1) { //torment has them as 0 or -1
			actor->SetText(pcInfo.Name,0); //setting both names
		}
		actor->TalkCount = pcInfo.TalkCount;
	} else {
		DataStream* ds = core->GetResourceMgr()->GetResource(
				pcInfo.CREResRef, IE_CRE_CLASS_ID );
		aM->Open( ds );
		actor = aM->GetActor();
	}

	actor->Destination.x = actor->Pos.x = pcInfo.XPos;
	actor->Destination.y = actor->Pos.y = pcInfo.YPos;
	strcpy( actor->Area, pcInfo.Area );
	actor->SetOrientation( pcInfo.Orientation,0 );
	actor->TalkCount = pcInfo.TalkCount;

	actor->InParty = is_in_party ? (pcInfo.PartyOrder + 1) : 0;
	actor->InternalFlags |= IF_FROMGAME;

	actor->PCStats = ps;

	return actor;
}

PCStatsStruct* GAMImp::GetPCStats ()
{
	PCStatsStruct* ps = new PCStatsStruct();
	int i;

	str->ReadDword( &ps->BestKilledName );
	str->ReadDword( &ps->BestKilledXP );
	str->ReadDword( &ps->AwayTime );
	str->ReadDword( &ps->JoinDate );
	str->ReadDword( &ps->unknown10 );
	str->ReadDword( &ps->KillsChapterXP );
	str->ReadDword( &ps->KillsChapterCount );
	str->ReadDword( &ps->KillsTotalXP );
	str->ReadDword( &ps->KillsTotalCount );
	for (i = 0; i <= 3; i++) {
		str->ReadResRef( ps->FavouriteSpells[i] );
	}
	for (i = 0; i <= 3; i++)
		str->ReadWord( &ps->FavouriteSpellsCount[i] );

	for (i = 0; i <= 3; i++) {
		str->ReadResRef( ps->FavouriteWeapons[i] );
	}
	for (i = 0; i <= 3; i++)
		str->ReadWord( &ps->FavouriteWeaponsCount[i] );

	return ps;
}

GAMJournalEntry* GAMImp::GetJournalEntry()
{
	GAMJournalEntry* j = new GAMJournalEntry();

	str->ReadDword( &j->Text );
	str->ReadDword( &j->GameTime ); 
	//this could be wrong, most likely these are 2 words, or a dword
	str->Read( &j->Chapter, 1 );
	str->Read( &j->unknown09, 1 );
	str->Read( &j->Section, 1 );
	str->Read( &j->Group, 1 ); // this is a GemRB extension

	return j;
}
