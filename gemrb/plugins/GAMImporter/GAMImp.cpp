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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/GAMImporter/GAMImp.cpp,v 1.54 2005/06/17 19:33:06 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "../../includes/globals.h"
#include "GAMImp.h"
#include "../Core/Interface.h"
#include "../Core/MapMgr.h"
#include "../Core/MemoryStream.h"


#define MAZE_DATA_SIZE    1720

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
	if (strncmp( Signature, "GAMEV0.0", 8 ) == 0) {
		version = GAM_VER_GEMRB;
		PCSize = 0x160;
	} else if (strncmp( Signature, "GAMEV2.0", 8 ) == 0) {
		//soa, tob
		version = GAM_VER_BG2;
		PCSize = 0x160;
	} else if (strncmp( Signature, "GAMEV1.0", 8 ) == 0) {
		//bg1?
		version = GAM_VER_BG;
		PCSize = 0x160;
	} else if (strncmp( Signature, "GAMEV2.2", 8 ) == 0) {
		//iwd2
		version = GAM_VER_IWD2;
		PCSize = 0x340;
	} else if (strncmp( Signature, "GAMEV1.1", 8 ) == 0) {
		//iwd, torment, totsc
		if (core->HasFeature(GF_HAS_KAPUTZ) ) { //pst
			PCSize = 0x168;
			version = GAM_VER_PST;
		} else if ( (stricmp( core->GameType, "iwd" ) == 0) 
			|| (stricmp( core->GameType, "how" ) == 0) ) {
			PCSize = 0x180;
			version = GAM_VER_IWD;
		} else {
			PCSize = 0x160;
			version=GAM_VER_BG;
		}
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

	// saving in original version requires the original version
	// otherwise it is set to 0 at construction time
	if (core->SaveAsOriginal) {
		newGame->version=version;
	}
	str->ReadDword( &newGame->GameTime );
	str->ReadWord( &newGame->WhichFormation );
	for (i = 0; i < 5; i++) {
		str->ReadWord( &newGame->Formations[i] );
	}
	str->ReadDword( &newGame->PartyGold );
	str->ReadDword( &newGame->WeatherBits ); 
	str->ReadDword( &PCOffset );
	str->ReadDword( &PCCount );
	//these fields are not really used by any engine, and never saved
	//str->ReadDword( &UnknownOffset );
	//str->ReadDword( &UnknownCount );
	str->Seek( 8, GEM_CURRENT_POS);
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
			MazeOffset = 0;
			str->ReadDword( &newGame->Reputation );
			str->ReadResRef( newGame->CurrentArea ); // FIXME: see above
			str->ReadDword( &newGame->ControlStatus );
			str->ReadDword( &KillVarsCount ); //this is still unknown
			str->ReadDword( &FamiliarsOffset );
			str->ReadDword( &SavedLocOffset );
			str->ReadDword( &SavedLocCount );
			str->Seek( 64, GEM_CURRENT_POS);
			break;

		case GAM_VER_PST:
			str->ReadDword( &MazeOffset );
			str->ReadDword( &newGame->Reputation );
			str->ReadResRef( newGame->AnotherArea );
			str->ReadDword( &KillVarsOffset );
			str->ReadDword( &KillVarsCount );
			str->ReadDword( &FamiliarsOffset );
			str->ReadDword( &SavedLocOffset );
			str->ReadDword( &SavedLocCount );
			str->Seek( 64, GEM_CURRENT_POS);
			break;
	}

	//newGame->globals = new Variables();
	//newGame->globals->SetType( GEM_VARIABLES_INT );

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
		if (actor->Selected) {
			newGame->SelectActor(actor, true, 0);
		}
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
		newGame->locals->SetAt( Name, Value );
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

	if (version == GAM_VER_PST) {
		//loading maze
		if (MazeOffset) {
			newGame->mazedata = (ieByte*)malloc(MAZE_DATA_SIZE);
			str->Seek(MazeOffset, GEM_STREAM_START );
			str->Read(newGame->mazedata, MAZE_DATA_SIZE); 
		}
		// Loading known creatures array (beasts)
		if(core->GetBeastsINI() != NULL) {
			int beasts_count = core->GetBeastsINI()->GetTagsCount();
			newGame->beasts = (ieByte*)malloc(beasts_count);
			str->Seek( FamiliarsOffset, GEM_STREAM_START );
			str->Read( newGame->beasts, beasts_count );
		}
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
	str->ReadWord( &pcInfo.ModalState );  //see Modal.ids
	str->ReadWord( &pcInfo.Happiness );   //
	str->Read( &pcInfo.Unknown2c, 96 );
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
	if (version==GAM_VER_IWD2) { //skipping some bytes for iwd2
		str->Seek( 254, GEM_CURRENT_POS);
	}
	str->Read( &pcInfo.Name, 32 );
	if (version==GAM_VER_PST) { //Torment
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
	actor->Selected = pcInfo.Selected;

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

int GAMImp::GetStoredFileSize(Game *game)
{
	int headersize;
	unsigned int i;

	ActorMgr *am = (ActorMgr *) core->GetInterface( IE_CRE_CLASS_ID );
	switch(game->version)
	{
	case GAM_VER_GEMRB:
		if(core->HasFeature(GF_HAS_KAPUTZ) ) {
			KillVarsCount = game->kaputz->GetCount();
		}
		headersize = 0xb4;
		PCSize = 0x160;
		break;
	case GAM_VER_IWD:
		headersize = 0xb4;
		PCSize = 0x180;
		break;
	case GAM_VER_BG:
	case GAM_VER_BG2:
		headersize = 0xb4;
		PCSize = 0x160;
		break;
	case GAM_VER_IWD2:
		headersize = 0xb4;
		PCSize = 0x340;
		break;
	case GAM_VER_PST:
		headersize = 0xb8;
		PCSize = 0x168;
		KillVarsCount = game->kaputz->GetCount();
		break;
	default:
		return -1;
	}
	PCOffset = headersize;

	PCCount = game->GetPartySize(false);
	headersize += PCCount * PCSize;
	for (i = 0;i<PCCount; i++) {
		Actor *ac=game->GetPC(i);
		headersize +=am->GetStoredFileSize(ac);
	}
	NPCOffset = headersize;


	NPCCount = game->GetNPCCount();
	headersize += NPCCount * PCSize;
	for (i = 0;i<NPCCount; i++) {
		Actor *ac=game->GetNPC(i);
		headersize +=am->GetStoredFileSize(ac);
	}
	JournalOffset = headersize;

	JournalCount = game->GetJournalCount();
	headersize += JournalCount * 12;
	GlobalOffset = headersize;

	GlobalCount = game->locals->GetCount();
	headersize += GlobalCount * 0x54;

	if (core->HasFeature(GF_HAS_KAPUTZ) ) {
		KillVarsOffset = headersize;
		KillVarsCount = game->locals->GetCount();
		headersize += KillVarsCount * 0x54;
	}

	core->FreeInterface( am );
	return headersize;
}

int GAMImp::PutJournals(DataStream *stream, Game *game)
{
	for (unsigned int i=0;i<JournalCount;i++) {
		GAMJournalEntry *j = game->GetJournalEntry(i);

		stream->WriteDword( &j->Text );
		stream->WriteDword( &j->GameTime ); 
		//this could be wrong, most likely these are 2 words, or a dword
		stream->Write( &j->Chapter, 1 );
		stream->Write( &j->unknown09, 1 );
		stream->Write( &j->Section, 1 );
		stream->Write( &j->Group, 1 ); // this is a GemRB extension
	}

	return 0;
}

//only in PST
int GAMImp::PutKillVars(DataStream *stream, Game *game)
{
	char filling[40];
	POSITION pos=NULL;
	const char *name;
	ieDword value;

	memset(filling,0,sizeof(filling) );
	for (unsigned int i=0;i<KillVarsCount;i++) {
		//global variables are locals for game, that's why the local/global confusion
		pos=game->kaputz->GetNextAssoc( pos, name, value);
		stream->Write( name, 32);
		stream->Write( filling, 8);
		stream->WriteDword( &value);
		//40 bytes of empty crap
		stream->Write( filling, 40);
	}
	return 0;
}

int GAMImp::PutVariables(DataStream *stream, Game *game)
{
	char filling[40];
	POSITION pos=NULL;
	const char *name;
	ieDword value;

	memset(filling,0,sizeof(filling) );
	for (unsigned int i=0;i<GlobalCount;i++) {
		//global variables are locals for game, that's why the local/global confusion
		pos=game->locals->GetNextAssoc( pos, name, value);
		stream->Write( name, 32);
		stream->Write( filling, 8);
		stream->WriteDword( &value);
		//40 bytes of empty crap
		stream->Write( filling, 40);
	}
	return 0;
}

int GAMImp::PutHeader(DataStream *stream, Game *game)
{
	char Signature[8];
	ieDword tmpDword = 0;

	memcpy( Signature, "GAMEV0.0", 8);
	Signature[5]+=game->version/10;
	if (game->version==12) { //pst version
		Signature[7]+=1;
	}
	else {
		Signature[7]+=game->version%10;
	}
	stream->Write( Signature, 8);
	stream->WriteDword( &game->GameTime );
	stream->WriteWord( &game->WhichFormation );
	for(int i=0;i<5;i++)
		stream->WriteWord( &game->Formations[i]);
	stream->WriteDword( &game->PartyGold );
	stream->WriteDword( &game->WeatherBits );
	stream->WriteDword( &PCOffset );
	stream->WriteDword( &PCCount );
	//these fields are zeroed in any original savegame
	stream->WriteDword( &tmpDword );
	stream->WriteDword( &tmpDword );
	stream->WriteDword( &NPCOffset );
	stream->WriteDword( &NPCCount );
	stream->WriteDword( &GlobalOffset );
	stream->WriteDword( &GlobalCount );
	stream->WriteResRef( game->CurrentArea );
	stream->WriteDword( &game->Unknown48 );
	stream->WriteDword( &JournalCount );
	stream->WriteDword( &JournalOffset );

	switch(game->version) {
	case GAM_VER_GEMRB:
	case GAM_VER_BG:
	case GAM_VER_IWD:
	case GAM_VER_BG2:
	case GAM_VER_IWD2:
		stream->WriteDword( &game->Reputation );
		stream->WriteResRef( game->CurrentArea );
		stream->WriteDword( &game->ControlStatus );
		break;
	case GAM_VER_PST:
		stream->WriteDword( &MazeOffset );
		stream->WriteDword( &game->Reputation );
		stream->WriteResRef( game->CurrentArea );
		stream->WriteDword( &KillVarsOffset );
		stream->WriteDword( &KillVarsCount );
		stream->WriteDword( &BestiaryOffset );
		break;
	}

	return 0;
}

int GAMImp::PutActor(DataStream *stream, Actor *ac, ieDword CRESize, ieDword CREOffset)
{
	ieDword tmpDword;
	ieWord tmpWord;

	char filling[8];

	memset(filling,0,sizeof(filling) );
	if (ac->Selected) {
		tmpWord=1;
	} else {
		tmpWord=0;
	}
	stream->WriteWord( &tmpWord);
	tmpWord = ac->InParty;
	stream->WriteWord( &tmpWord);

	stream->WriteDword( &CREOffset);
	stream->WriteDword( &CRESize);
	//creature resref is always unused in saved games
	stream->Write( filling, 8);
	tmpDword = ac->GetOrientation();
	stream->WriteDword (&tmpDword);
	return 0;
}

int GAMImp::PutPCs(DataStream *stream, Game *game)
{
	unsigned int i;
	ActorMgr *am = (ActorMgr *) core->GetInterface( IE_CRE_CLASS_ID );
	ieDword CREOffset = PCOffset + PCCount * PCSize;

	for(i=0;i<PCCount;i++) {
		Actor *ac = game->GetPC(i);
                ieDword CRESize = am->GetStoredFileSize(ac);
		PutActor(stream, ac, CRESize, CREOffset);
		CREOffset += CRESize;
	}

	for(i=0;i<PCCount;i++) {
		Actor *ac = game->GetPC(i);
                //reconstructing offsets again
                am->GetStoredFileSize(ac);
                am->PutActor( stream, ac);
	}
        core->FreeInterface( am );
	return 0;
}

int GAMImp::PutNPCs(DataStream *stream, Game *game)
{
	unsigned int i;
	ActorMgr *am = (ActorMgr *) core->GetInterface( IE_CRE_CLASS_ID );
	ieDword CREOffset = NPCOffset + NPCCount * PCSize;

	for(i=0;i<NPCCount;i++) {
		Actor *ac = game->GetNPC(i);
                ieDword CRESize = am->GetStoredFileSize(ac);
		PutActor(stream, ac, CRESize, CREOffset);
		CREOffset += CRESize;
	}
	for(i=0;i<PCCount;i++) {
		Actor *ac = game->GetPC(i);
                //reconstructing offsets again
                am->GetStoredFileSize(ac);
                am->PutActor( stream, ac);
	}
        core->FreeInterface( am );
	return 0;
}

int GAMImp::PutMaze(DataStream *stream, Game *game)
{
	stream->Write (game->mazedata, MAZE_DATA_SIZE);
	return 0;
}

int GAMImp::PutGame(DataStream *stream, Game *game)
{
	int ret;

	if (!stream || !game) {
		return -1;
	}

	ret = PutHeader( stream, game);
	if (ret) {
		return ret;
	}

	ret = PutPCs( stream, game);
	if (ret) {
		return ret;
	}

	ret = PutNPCs( stream, game);
	if (ret) {
		return ret;
	}

	ret = PutJournals( stream, game);
	if (ret) {
		return ret;
	}

	ret = PutVariables( stream, game);
	if (ret) {
		return ret;
	}

	if (core->HasFeature(GF_HAS_KAPUTZ) ) {
		ret = PutKillVars( stream, game);
		if (ret) {
			return ret;
		}

		ret = PutMaze( stream, game);
		if (ret) {
			return ret;
		}
	}

	return 0;
}
