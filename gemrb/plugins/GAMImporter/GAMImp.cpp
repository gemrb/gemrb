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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $Id$
 *
 */

#include "../../includes/win32def.h"
#include "../../includes/globals.h"
#include "GAMImp.h"
#include "../Core/Interface.h"
#include "../Core/MapMgr.h"
#include "../Core/ResourceMgr.h"
#include "../Core/MemoryStream.h"
#include "../Core/DataFileMgr.h"


#define MAZE_DATA_SIZE 1720
#define FAMILIAR_FILL_SIZE 324
// if your compiler chokes on this, use -1 or 0xff whichever works for you
#define UNINITIALIZED_CHAR '\xff'

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
		//soa (soa part of tob)
		version = GAM_VER_BG2;
		PCSize = 0x160;
	} else if (strncmp( Signature, "GAMEV2.1", 8 ) == 0) {
		//tob
		version = GAM_VER_TOB;
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
			//sound folder name takes up this space,
			//so it is handy to make this check
		} else if ( core->HasFeature(GF_SOUNDFOLDERS) ) {
			PCSize = 0x180;
			version = GAM_VER_IWD;
		} else {
			PCSize = 0x160;
			version=GAM_VER_BG;
		}
	} else {
		printMessage( "GAMImporter","This file is not a valid GAM File\n", LIGHT_RED );
		return false;
	}

	return true;
}

Game* GAMImp::LoadGame(Game *newGame)
{
	unsigned int i;
	//Game* newGame = new Game();

	// saving in original version requires the original version
	// otherwise it is set to 0 at construction time
	if (core->SaveAsOriginal) {
		newGame->version=version;
	}

	ieDword GameTime;
	str->ReadDword( &GameTime );
	newGame->GameTime = GameTime*AI_UPDATE_TIME;

	str->ReadWord( &newGame->WhichFormation );
	for (i = 0; i < 5; i++) {
		str->ReadWord( &newGame->Formations[i] );
	}
	//hack for PST
	if (version==GAM_VER_PST) {
		newGame->Formations[0] = newGame->WhichFormation;
		newGame->WhichFormation = 0;
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
			memcpy(newGame->AnotherArea, newGame->CurrentArea, sizeof(ieResRef) );
			str->ReadDword( &newGame->ControlStatus );
			str->ReadDword( &KillVarsCount ); //this is still unknown
			str->ReadDword( &FamiliarsOffset );
			str->ReadDword( &SavedLocOffset );
			str->ReadDword( &SavedLocCount );
			str->ReadDword( &newGame->RealTime);
			str->Seek( 60, GEM_CURRENT_POS);
			break;

		case GAM_VER_PST:
			str->ReadDword( &MazeOffset );
			str->ReadDword( &newGame->Reputation );
			str->ReadResRef( newGame->AnotherArea );
			str->ReadDword( &KillVarsOffset );
			str->ReadDword( &KillVarsCount );
			str->ReadDword( &FamiliarsOffset ); //bestiary
			str->ReadResRef( newGame->AnotherArea ); //yet another area
			SavedLocOffset = 0;
			SavedLocCount = 0;
			str->Seek( 64, GEM_CURRENT_POS);
			break;
	}

	if (!newGame->CurrentArea[0]) {
		// 0 - normal, 1 - tutorial, 2 - extension
		AutoTable tm("STARTARE");
		ieDword playmode = 0;
		//only bg2 has 9 rows (iwd's have 6 rows - normal+extension)
		if (tm && tm->GetRowCount()==9) {
			core->GetDictionary()->Lookup( "PlayMode", playmode );
			playmode *= 3;
		}

		const char* resref = tm->QueryField( playmode );
		strnlwrcpy( newGame->CurrentArea, resref, 8 );
	}

	//Loading PCs
	ActorMgr* aM = ( ActorMgr* ) core->GetInterface( IE_CRE_CLASS_ID );
	for (i = 0; i < PCCount; i++) {
		str->Seek( PCOffset + ( i * PCSize ), GEM_STREAM_START );
		Actor *actor = GetActor( aM, true );
		newGame->JoinParty( actor, 0 );
		if (actor->Selected) {
			newGame->SelectActor(actor, true, SELECT_QUIET);
		}
	}

	//Loading NPCs
	for (i = 0; i < NPCCount; i++) {
		str->Seek( NPCOffset + ( i * PCSize ), GEM_STREAM_START );
		Actor *actor = GetActor( aM, false );
		newGame->AddNPC( actor );
	}
	core->FreeInterface( aM );

	//apparently BG1/IWD2 relies on this, if chapter is unset, it is
	//set to -1, hopefully it won't break anything
	newGame->locals->SetAt("CHAPTER", (ieDword) -1);

	//Loading Global Variables
	ieVariable Name;
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
		str->Seek( FamiliarsOffset, GEM_STREAM_START );
	} else {
		if (FamiliarsOffset) {
			str->Seek( FamiliarsOffset, GEM_STREAM_START );
			for (i=0; i<9;i++) {
				str->ReadResRef( newGame->GetFamiliar(i) );
			}
		}
	}
	// Loading known creatures array (beasts)
	if(core->GetBeastsINI() != NULL) {
		int beasts_count = BESTIARY_SIZE;
		newGame->beasts = (ieByte*)calloc(sizeof(ieByte),beasts_count);
		if(FamiliarsOffset) {
			str->Read( newGame->beasts, beasts_count );
		}
	}

	if (SavedLocCount && SavedLocOffset) {
		str->Seek( SavedLocOffset, GEM_STREAM_START );
		//reading saved locations
	}
	return newGame;
}

void SanityCheck(ieWord a,ieWord &b,const char *message)
{
	if (a==0xffff) {
		b=0xffff;
		return;
	}
	if (b==0xffff) {
		printMessage("GAMImp"," ",LIGHT_RED);
		printf("Invalid Slot Enabler caught: %s!\n", message);
		b=0;
	}
}

Actor* GAMImp::GetActor( ActorMgr* aM, bool is_in_party )
{
	unsigned int i;
	PCStruct pcInfo;
	ieDword tmpDword;
	ieWord tmpWord;

	memset( &pcInfo,0,sizeof(pcInfo) );
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
	str->ReadWord( &pcInfo.ModalState ); //see Modal.ids
	str->ReadWord( &pcInfo.Happiness );
	str->Read( &pcInfo.Unknown2c, 96 );

	if (version==GAM_VER_GEMRB || version==GAM_VER_IWD2) {
		ieResRef tmp;

		for (i = 0; i < 4; i++) {
			str->ReadWord( &pcInfo.QuickWeaponSlot[i] );
			str->ReadWord( &pcInfo.QuickWeaponSlot[i+4] );
		}
		for (i = 0; i < 4; i++) {
			str->ReadWord( &tmpWord );
			SanityCheck( pcInfo.QuickWeaponSlot[i], tmpWord, "weapon");
			pcInfo.QuickWeaponHeader[i]=tmpWord;
			str->ReadWord( &tmpWord );
			SanityCheck( pcInfo.QuickWeaponSlot[i+4], tmpWord, "weapon");
			pcInfo.QuickWeaponHeader[i+4]=tmpWord;
		}
		for (i = 0; i < MAX_QSLOTS; i++) {
			str->Read( &pcInfo.QuickSpellResRef[i], 8 );
		}
		str->Read( &pcInfo.QuickSpellClass, MAX_QSLOTS ); //9 bytes

		str->Seek( 1, GEM_CURRENT_POS); //skipping a padding byte
		for (i = 0; i < 3; i++) {
			str->ReadWord( &pcInfo.QuickItemSlot[i] );
		}
		for (i = 0; i < 3; i++) {
			str->ReadWord( &tmpWord );
			SanityCheck( pcInfo.QuickItemSlot[i], tmpWord, "item");
			pcInfo.QuickItemHeader[i]=tmpWord;
		}
		pcInfo.QuickItemHeader[3]=0xffff;
		pcInfo.QuickItemHeader[4]=0xffff;
		if (version == GAM_VER_IWD2) {
			//quick innates
			//we spare some memory and time by storing them in the same place
			//this may be slightly buggy because IWD2 doesn't clear the
			//fields, but QuickSpellClass is set correctly, problem is
			//that GemRB doesn't clear QuickSpellClass
			for (i = 0; i < MAX_QSLOTS; i++) {
				str->Read( &tmp[i], 8 );
				if ((tmp[0]!=0) && (pcInfo.QuickSpellResRef[0]==0)) {
					memcpy( pcInfo.QuickSpellResRef[i], tmp, 8);
					//innates
					pcInfo.QuickSpellClass[i]=0xff;
				}
			}
			//recently discovered fields (bard songs)
			//str->Seek( 72, GEM_CURRENT_POS);
			for(i = 0; i<MAX_QSLOTS;i++) {
				str->Read( &tmp[i], 8 );
				if ((tmp[0]!=0) && (pcInfo.QuickSpellResRef[0]==0)) {
					memcpy( pcInfo.QuickSpellResRef[i], tmp, 8);
					//bardsongs
					pcInfo.QuickSpellClass[i]=0xfe;
				}
			}
		}
		//QuickSlots are customisable in iwd2 and GemRB
		//thus we adopt the iwd2 style actor info
		for (i=0;i<MAX_QSLOTS;i++) {
			str->ReadDword( &tmpDword );
			pcInfo.QSlots[i] = (ieByte) tmpDword;
		}
	} else {
		for (i = 0; i < 4; i++) {
			str->ReadWord( &pcInfo.QuickWeaponSlot[i] );
		}
		for (i = 0; i < 4; i++) {
			str->ReadWord( &tmpWord );
			SanityCheck( pcInfo.QuickWeaponSlot[i], tmpWord, "weapon");
			pcInfo.QuickWeaponHeader[i]=tmpWord;
		}
		for (i = 0; i < 3; i++) {
			str->Read( &pcInfo.QuickSpellResRef[i], 8 );
		}
		if (version==GAM_VER_PST) { //Torment
			for (i = 0; i < 5; i++) {
				str->ReadWord( &pcInfo.QuickItemSlot[i] );
			}
			for (i = 0; i < 5; i++) {
				str->ReadWord( &tmpWord );
				SanityCheck( pcInfo.QuickItemSlot[i], tmpWord, "item");
				pcInfo.QuickItemHeader[i]=tmpWord;
			}
			//str->Seek( 10, GEM_CURRENT_POS ); //enabler fields
		} else {
			for (i = 0; i < 3; i++) {
				str->ReadWord( &pcInfo.QuickItemSlot[i] );
			}
			for (i = 0; i < 3; i++) {
				str->ReadWord( &tmpWord );
				SanityCheck( pcInfo.QuickItemSlot[i], tmpWord, "item");
				pcInfo.QuickItemHeader[i]=tmpWord;
			}
			//str->Seek( 6, GEM_CURRENT_POS ); //enabler fields
		}
		pcInfo.QSlots[0] = 0xff; //(invalid, will be regenerated)
	}
	str->Read( &pcInfo.Name, 32 );
	str->ReadDword( &pcInfo.TalkCount );

	ieDword pos = str->GetPos();

	Actor* actor = NULL;
	tmpWord = is_in_party ? (pcInfo.PartyOrder + 1) : 0;

	if (pcInfo.OffsetToCRE) {
		str->Seek( pcInfo.OffsetToCRE, GEM_STREAM_START );
		void* Buffer = malloc( pcInfo.CRESize );
		str->Read( Buffer, pcInfo.CRESize );
		//somehow autofree MemoryStream doesn't work on msvc 7.0
		//separate heap for dll's?
		MemoryStream* ms = new MemoryStream( Buffer, pcInfo.CRESize, false );
		if (ms) {
			aM->Open( ms, true );
			actor = aM->GetActor(tmpWord);
		}
		free (Buffer);

 		//torment has them as 0 or -1
		if (pcInfo.Name[0]!=0 && pcInfo.Name[0]!=UNINITIALIZED_CHAR) {
			actor->SetText(pcInfo.Name,0); //setting both names
		}
		actor->TalkCount = pcInfo.TalkCount;
	} else {
		DataStream* ds = core->GetResourceMgr()->GetResource(
				pcInfo.CREResRef, IE_CRE_CLASS_ID );
		//another plugin cannot free memory stream from this plugin
		//so auto free is a no-no
		if (ds) {
			aM->Open( ds, true );
			actor = aM->GetActor(pcInfo.PartyOrder);
		}
	}
	if (!actor) {
		return actor;
	}

	//
	str->Seek(pos, GEM_STREAM_START);
	//
	actor->CreateStats();
	PCStatsStruct *ps = actor->PCStats;
	GetPCStats(ps);
	memcpy(ps->QSlots, pcInfo.QSlots, sizeof(pcInfo.QSlots) );
	memcpy(ps->QuickSpells, pcInfo.QuickSpellResRef, MAX_QSLOTS*sizeof(ieResRef) );
	memcpy(ps->QuickSpellClass, pcInfo.QuickSpellClass, MAX_QSLOTS );
	memcpy(ps->QuickWeaponSlots, pcInfo.QuickWeaponSlot, MAX_QUICKWEAPONSLOT*sizeof(ieWord) );
	memcpy(ps->QuickWeaponHeaders, pcInfo.QuickWeaponHeader, MAX_QUICKWEAPONSLOT*sizeof(ieWord) );
	memcpy(ps->QuickItemSlots, pcInfo.QuickItemSlot, MAX_QUICKITEMSLOT*sizeof(ieWord) );
	memcpy(ps->QuickItemHeaders, pcInfo.QuickItemHeader, MAX_QUICKITEMSLOT*sizeof(ieWord) );
	actor->Destination.x = actor->Pos.x = pcInfo.XPos;
	actor->Destination.y = actor->Pos.y = pcInfo.YPos;
	strcpy( actor->Area, pcInfo.Area );
	actor->SetOrientation( pcInfo.Orientation,0 );
	actor->TalkCount = pcInfo.TalkCount;
	actor->ModalState = pcInfo.ModalState;

	actor->SetPersistent( tmpWord );

	actor->Selected = pcInfo.Selected;
	return actor;
}

void GAMImp::GetPCStats (PCStatsStruct *ps)
{
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

	str->ReadResRef( ps->SoundSet );

	if (core->HasFeature(GF_SOUNDFOLDERS) ) {
		str->Read( ps->SoundFolder, 32);
	}
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

	//moved this here, so one can disable killvars in a pst style game
	//or enable them in gemrb
	if(core->HasFeature(GF_HAS_KAPUTZ) ) {
		KillVarsCount = game->kaputz->GetCount();
	} else {
		KillVarsCount = 0;
	}
	switch(game->version)
	{
	case GAM_VER_GEMRB:
		headersize = 0xb4;
		PCSize = 0x160;
		break;
	case GAM_VER_IWD:
		headersize = 0xb4;
		PCSize = 0x180;
		break;
	case GAM_VER_BG:
	case GAM_VER_BG2:
	case GAM_VER_TOB:
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
		break;
	default:
		return -1;
	}
	PCOffset = headersize;

	ActorMgr *am = (ActorMgr *) core->GetInterface( IE_CRE_CLASS_ID );
	PCCount = game->GetPartySize(false);
	headersize += PCCount * PCSize;
	for (i = 0;i<PCCount; i++) {
		Actor *ac=game->GetPC(i, false);
		headersize +=am->GetStoredFileSize(ac);
	}
	NPCOffset = headersize;

	NPCCount = game->GetNPCCount();
	headersize += NPCCount * PCSize;
	for (i = 0;i<NPCCount; i++) {
		Actor *ac=game->GetNPC(i);
		headersize +=am->GetStoredFileSize(ac);
	}
	core->FreeInterface( am );

	if (game->mazedata) {
		MazeOffset = headersize;
		headersize += MAZE_DATA_SIZE;
	} else {
		MazeOffset = 0;
	}

	GlobalOffset = headersize;

	GlobalCount = game->locals->GetCount();
	headersize += GlobalCount * 84;
	JournalOffset = headersize;

	JournalCount = game->GetJournalCount();
	headersize += JournalCount * 12;

	KillVarsOffset = headersize;
	if (KillVarsCount) {
		headersize += KillVarsCount * 84;
	}

	if (game->version==GAM_VER_BG) {
		FamiliarsOffset = 0;
	} else {
		FamiliarsOffset = headersize;
		if (core->GetBeastsINI()) {
			headersize +=BESTIARY_SIZE;
		}
		if (game->version!=GAM_VER_PST) {
			headersize += 9 * 8 + 82 * 4;
		}
	}
	SavedLocOffset = headersize;
	//i don't use this yet, it should be used only for bg2
	SavedLocCount = 0;
	return headersize + SavedLocCount * 20;
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
	ieVariable tmpname;
	POSITION pos=NULL;
	const char *name;
	ieDword value;

	memset(filling,0,sizeof(filling) );
	for (unsigned int i=0;i<KillVarsCount;i++) {
		//global variables are locals for game, that's why the local/global confusion
		pos=game->kaputz->GetNextAssoc( pos, name, value);
		strnspccpy(tmpname,name,32);
		stream->Write( tmpname, 32);
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
	ieVariable tmpname;
	POSITION pos=NULL;
	const char *name;
	ieDword value;

	memset(filling,0,sizeof(filling) );
	for (unsigned int i=0;i<GlobalCount;i++) {
		//global variables are locals for game, that's why the local/global confusion
		pos=game->locals->GetNextAssoc( pos, name, value);
		strnspccpy(tmpname,name,32);
		stream->Write( tmpname, 32);
		stream->Write( filling, 8);
		stream->WriteDword( &value);
		//40 bytes of empty crap
		stream->Write( filling, 40);
	}
	return 0;
}

int GAMImp::PutHeader(DataStream *stream, Game *game)
{
	int i;
	char Signature[10];
	ieDword tmpDword;

	memcpy( Signature, "GAMEV0.0", 8);
	Signature[5]+=game->version/10;
	if (game->version==GAM_VER_PST) { //pst version
		Signature[7]+=1;
	}
	else {
		Signature[7]+=game->version%10;
	}
	stream->Write( Signature, 8);
	//using Signature for padding
	memset(Signature, 0, sizeof(Signature));
	tmpDword = game->GameTime/AI_UPDATE_TIME;
	stream->WriteDword( &tmpDword );
	//pst has a single preset of formations
	if (game->version==GAM_VER_PST) {
		stream->WriteWord( &game->Formations[0]);
		stream->Write( Signature, 10);
	} else {
		stream->WriteWord( &game->WhichFormation );
		for(i=0;i<5;i++) {
			stream->WriteWord( &game->Formations[i]);
		}
	}
	stream->WriteDword( &game->PartyGold );
	stream->WriteDword( &game->WeatherBits );
	stream->WriteDword( &PCOffset );
	stream->WriteDword( &PCCount );
	//these fields are zeroed in any original savegame
	tmpDword = 0;
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
	case GAM_VER_TOB:
	case GAM_VER_IWD2:
		stream->WriteDword( &game->Reputation );
		stream->WriteResRef( game->CurrentArea );
		stream->WriteDword( &game->ControlStatus );
		stream->WriteDword( &tmpDword);
		stream->WriteDword( &FamiliarsOffset);
		stream->WriteDword( &SavedLocOffset);
		stream->WriteDword( &SavedLocCount);
		break;
	case GAM_VER_PST:
		stream->WriteDword( &MazeOffset );
		stream->WriteDword( &game->Reputation );
		stream->WriteResRef( game->CurrentArea );
		stream->WriteDword( &KillVarsOffset );
		stream->WriteDword( &KillVarsCount );
		stream->WriteDword( &FamiliarsOffset );
		stream->WriteResRef( game->CurrentArea ); //again
		break;
	}
	stream->WriteDword( &game->RealTime ); //this isn't correct, this field is the realtime
	stream->WriteDword( &SavedLocOffset);
	for (i=0;i<7;i++) {
		stream->Write( Signature, 8);
	}
	return 0;
}

int GAMImp::PutActor(DataStream *stream, Actor *ac, ieDword CRESize, ieDword CREOffset, ieDword version)
{
	int i;
	ieDword tmpDword;
	ieWord tmpWord;
	char filling[194];

	memset(filling,0,sizeof(filling) );
	if (ac->Selected) {
		tmpWord=1;
	} else {
		tmpWord=0;
	}

	stream->WriteWord( &tmpWord);
	tmpWord = ac->InParty-1;
	stream->WriteWord( &tmpWord);

	stream->WriteDword( &CREOffset);
	stream->WriteDword( &CRESize);
	//creature resref is always unused in saved games
	stream->Write( filling, 8);
	tmpDword = ac->GetOrientation();
	stream->WriteDword( &tmpDword);
	stream->WriteResRef(ac->Area);
	tmpWord = ac->Pos.x;
	stream->WriteWord( &tmpWord);
	tmpWord = ac->Pos.y;
	stream->WriteWord( &tmpWord);
	//no viewport, we cheat
	tmpWord = ac->Pos.x;
	stream->WriteWord( &tmpWord);
	tmpWord = ac->Pos.y;
	stream->WriteWord( &tmpWord);
	tmpWord = (ieWord) ac->ModalState;
	stream->WriteWord( &tmpWord);
	tmpWord = 0; //happiness
	stream->WriteWord( &tmpWord);
	//a field of zeroes
	stream->Write(filling, 96);

	//quickweapons
	if (version==GAM_VER_IWD2 || version==GAM_VER_GEMRB) {
		for (i=0;i<4;i++) {
			stream->WriteWord( ac->PCStats->QuickWeaponSlots+i);
			stream->WriteWord( ac->PCStats->QuickWeaponSlots+4+i);
		}
		for (i=0;i<4;i++) {
			stream->WriteWord( ac->PCStats->QuickWeaponHeaders+i);
			stream->WriteWord( ac->PCStats->QuickWeaponHeaders+4+i);
		}
	} else {
		for (i=0;i<4;i++) {
			stream->WriteWord( ac->PCStats->QuickWeaponSlots+i);
		}
		for (i=0;i<4;i++) {
			stream->WriteWord( ac->PCStats->QuickWeaponHeaders+i);
		}
	}

	//quickspells
	if (version==GAM_VER_IWD2 || version==GAM_VER_GEMRB) {
		for (i=0;i<MAX_QSLOTS;i++) {
			if ( (ieByte) ac->PCStats->QuickSpellClass[i]>=0xfe) {
				stream->Write(filling,8);
			} else {
				stream->Write(ac->PCStats->QuickSpells[i],8);
			}
		}
		//quick spell classes, clear the field for iwd2 if it is
		//a bard song/innate slot (0xfe or 0xff)
		memcpy(filling, ac->PCStats->QuickSpellClass, MAX_QSLOTS);
		if (version==GAM_VER_IWD2) {
			for(i=0;i<MAX_QSLOTS;i++) {
				if((ieByte) filling[i]>=0xfe) {
					filling[i]=0;
				}
			}
		}
		stream->Write(filling,10);
	} else {
		for (i=0;i<3;i++) {
			stream->Write(ac->PCStats->QuickSpells[i],8);
		}
	}

	//quick items
	switch (version) {
	case GAM_VER_PST: case GAM_VER_GEMRB:
		for (i=0;i<MAX_QUICKITEMSLOT;i++) {
			stream->WriteWord(ac->PCStats->QuickItemSlots+i);
		}
		for (i=0;i<MAX_QUICKITEMSLOT;i++) {
			stream->WriteWord(ac->PCStats->QuickItemHeaders+i);
		}
		break;
	default:
		for (i=0;i<3;i++) {
			stream->WriteWord(ac->PCStats->QuickItemSlots+i);
		}
		for (i=0;i<3;i++) {
			stream->WriteWord(ac->PCStats->QuickItemHeaders+i);
		}
		break;
	}

	//innates, bard songs and quick slots are saved only in iwd2
	if (version==GAM_VER_IWD2 || version==GAM_VER_GEMRB) {
		for (i=0;i<MAX_QSLOTS;i++) {
			if ( (ieByte) ac->PCStats->QuickSpellClass[i]==0xff) {
				stream->Write(ac->PCStats->QuickSpells[i],8);
			} else {
				stream->Write(filling,8);
			}
		}
		for (i=0;i<MAX_QSLOTS;i++) {
			if ((ieByte) ac->PCStats->QuickSpellClass[i]==0xfe) {
				stream->Write(ac->PCStats->QuickSpells[i],8);
			} else {
				stream->Write(filling,8);
			}
		}
		for (i=0;i<MAX_QSLOTS;i++) {
			tmpDword = ac->PCStats->QSlots[i];
			stream->WriteDword( &tmpDword);
		}
	}

	if (ac->LongStrRef==0xffffffff) {
		strncpy(filling, ac->LongName, 32);
	} else {
		char *tmpstr = core->GetString(ac->LongStrRef, IE_STR_STRREFOFF);
		strncpy(filling, tmpstr, 32);
		core->FreeString( tmpstr );
	}
	stream->Write( filling, 32);
	memset(filling,0,32);
	stream->WriteDword( &ac->TalkCount);
	stream->WriteDword( &ac->PCStats->BestKilledName);
	stream->WriteDword( &ac->PCStats->BestKilledXP);
	stream->WriteDword( &ac->PCStats->AwayTime);
	stream->WriteDword( &ac->PCStats->JoinDate);
	stream->WriteDword( &ac->PCStats->unknown10);
	stream->WriteDword( &ac->PCStats->KillsChapterXP);
	stream->WriteDword( &ac->PCStats->KillsChapterCount);
	stream->WriteDword( &ac->PCStats->KillsTotalXP);
	stream->WriteDword( &ac->PCStats->KillsTotalCount);
	for (i=0;i<4;i++) {
		stream->WriteResRef( ac->PCStats->FavouriteSpells[i]);
	}
	for (i=0;i<4;i++) {
		stream->WriteWord( &ac->PCStats->FavouriteSpellsCount[i]);
	}
	for (i=0;i<4;i++) {
		stream->WriteResRef( ac->PCStats->FavouriteWeapons[i]);
	}
	for (i=0;i<4;i++) {
		stream->WriteWord( &ac->PCStats->FavouriteWeaponsCount[i]);
	}
	stream->Write( ac->PCStats->SoundSet, 8); //soundset
	if (core->HasFeature(GF_SOUNDFOLDERS) ) {
		stream->Write(ac->PCStats->SoundFolder, 32);
	}
	if (version==GAM_VER_IWD2 || version==GAM_VER_GEMRB) {
		stream->Write(filling, 194);
	}
	return 0;
}

int GAMImp::PutPCs(DataStream *stream, Game *game)
{
	unsigned int i;
	ActorMgr *am = (ActorMgr *) core->GetInterface( IE_CRE_CLASS_ID );
	ieDword CREOffset = PCOffset + PCCount * PCSize;

	for(i=0;i<PCCount;i++) {
		Actor *ac = game->GetPC(i, false);
		ieDword CRESize = am->GetStoredFileSize(ac);
		PutActor(stream, ac, CRESize, CREOffset, game->version);
		CREOffset += CRESize;
	}

	for(i=0;i<PCCount;i++) {
		Actor *ac = game->GetPC(i, false);
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
		PutActor(stream, ac, CRESize, CREOffset, game->version);
		CREOffset += CRESize;
	}
	for(i=0;i<NPCCount;i++) {
		Actor *ac = game->GetNPC(i);
		//reconstructing offsets again
		am->GetStoredFileSize(ac);
		am->PutActor( stream, ac);
	}
	core->FreeInterface( am );
	return 0;
}

int GAMImp::PutMaze(DataStream *stream, Game *game)
{
	stream->Write( game->mazedata, MAZE_DATA_SIZE);
	return 0;
}

int GAMImp::PutFamiliars(DataStream *stream, Game *game)
{
	int len = 0;
	if (core->GetBeastsINI()) {
		len = BESTIARY_SIZE;
		if (game->version==GAM_VER_PST) {
			//only GemRB version can have all features, return when it is PST
			//gemrb version will have the beasts after the familiars
			stream->Write( game->beasts, len );
			return 0;
		}
	}

	char filling[FAMILIAR_FILL_SIZE];

	memset( filling,0,sizeof(filling) );
	for (unsigned int i=0;i<9;i++) {
		stream->WriteResRef( game->GetFamiliar(i) );
	}
	stream->WriteDword( &SavedLocOffset);
	if (len) {
		stream->Write( game->beasts, len );
	}
	stream->Write( filling, FAMILIAR_FILL_SIZE - len);
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

	if (game->mazedata) {
		ret = PutMaze( stream, game);
		if (ret) {
			return ret;
		}
	}

	ret = PutVariables( stream, game);
	if (ret) {
		return ret;
	}

	ret = PutJournals( stream, game);
	if (ret) {
		return ret;
	}

	if (core->HasFeature(GF_HAS_KAPUTZ) ) {
		ret = PutKillVars( stream, game);
		if (ret) {
			return ret;
		}

	}
	if (FamiliarsOffset) {
		ret = PutFamiliars( stream, game);
		if (ret) {
			return ret;
		}
	}

	return 0;
}
