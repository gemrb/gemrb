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
 *
 */

#include "GAMImporter.h"

#include "globals.h"
#include "win32def.h"

#include "DataFileMgr.h"
#include "GameData.h"
#include "Interface.h"
#include "MapMgr.h"
#include "PluginMgr.h"
#include "TableMgr.h"
#include "Scriptable/Actor.h"
#include "System/SlicedStream.h"

#include <cassert>

using namespace GemRB;

#define FAMILIAR_FILL_SIZE 324
// if your compiler chokes on this, use -1 or 0xff whichever works for you
#define UNINITIALIZED_CHAR '\xff'

GAMImporter::GAMImporter(void)
{
	str = NULL;
}

GAMImporter::~GAMImporter(void)
{
	delete str;
}

bool GAMImporter::Open(DataStream* stream)
{
	if (stream == NULL) {
		return false;
	}
	if (str) {
		return false;
	}
	str = stream;
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
		Log(ERROR, "GAMImporter", "This file is not a valid GAM File");
		return false;
	}

	return true;
}

Game* GAMImporter::LoadGame(Game *newGame, int ver_override)
{
	unsigned int i;

	// saving in original version requires the original version
	// otherwise it is set to 0 at construction time
	if (core->SaveAsOriginal) {
		// HACK: default icewind2.gam is 2.0! handled by script
		if(ver_override) {
			newGame->version = ver_override;
		}
		else {
			newGame->version = version;
		}
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
	//npc count in party???
	str->ReadWord( &newGame->NpcInParty );
	str->ReadWord( &newGame->WeatherBits );
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
	str->ReadDword( &newGame->Unknown48 );//this is still unknown
	str->ReadDword( &JournalCount );
	str->ReadDword( &JournalOffset );
	switch (version) {
		default:
			MazeOffset = 0;
			str->ReadDword( &newGame->Reputation );
			str->ReadResRef( newGame->CurrentArea ); // FIXME: see above
			memcpy(newGame->AnotherArea, newGame->CurrentArea, sizeof(ieResRef) );
			str->ReadDword( &newGame->ControlStatus );
			str->ReadDword( &newGame->Expansion );
			str->ReadDword( &FamiliarsOffset );
			str->ReadDword( &SavedLocOffset );
			str->ReadDword( &SavedLocCount );
			str->ReadDword( &newGame->RealTime);
			str->ReadDword( &PPLocOffset );
			str->ReadDword( &PPLocCount );
			str->Seek( 52, GEM_CURRENT_POS);
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
			PPLocOffset = 0;
			PPLocCount = 0;
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
	PluginHolder<ActorMgr> aM(IE_CRE_CLASS_ID);
	for (i = 0; i < PCCount; i++) {
		str->Seek( PCOffset + ( i * PCSize ), GEM_STREAM_START );
		Actor *actor = GetActor( aM, true );
		newGame->JoinParty( actor, actor->Selected?JP_SELECT:0 );
	}

	//Loading NPCs
	for (i = 0; i < NPCCount; i++) {
		str->Seek( NPCOffset + ( i * PCSize ), GEM_STREAM_START );
		Actor *actor = GetActor( aM, false );
		newGame->AddNPC( actor );
	}

	//apparently BG1/IWD2 relies on this, if chapter is unset, it is
	//set to -1, hopefully it won't break anything
	//PST has no chapter variable by default, and would crash on one
	newGame->locals->SetAt("CHAPTER", (ieDword) -1, core->HasFeature(GF_NO_NEW_VARIABLES));

	// load initial values from var.var
	newGame->locals->LoadInitialValues("GLOBAL");

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
		newGame->kaputz->ParseKey( 1 );
		// load initial values from var.var
		newGame->kaputz->LoadInitialValues("KAPUTZ");
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
			//Don't allocate memory in plugins (MSVC chokes on this)
			newGame->AllocateMazeData();
			str->Seek(MazeOffset, GEM_STREAM_START );
			for (i = 0; i<MAZE_ENTRY_COUNT;i++) {
				GetMazeEntry(newGame->mazedata+i*MAZE_ENTRY_SIZE);
			}
			GetMazeHeader(newGame->mazedata+MAZE_ENTRY_COUNT*MAZE_ENTRY_SIZE);
		}
		str->Seek( FamiliarsOffset, GEM_STREAM_START );
	} else {
		if (FamiliarsOffset) {
			str->Seek( FamiliarsOffset, GEM_STREAM_START );
			for (i=0; i<9;i++) {
				str->ReadResRef( newGame->GetFamiliar(i) );
			}
		} else {
			//clear these fields up
			for (i=0;i<9;i++) {
				memset(newGame->GetFamiliar(i), 0, sizeof(ieResRef));
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

	//TODO: these need to be corrected!
	if (SavedLocCount && SavedLocOffset) {
		ieWord PosX, PosY;

		str->Seek( SavedLocOffset, GEM_STREAM_START );
		for (i=0; i<SavedLocCount; i++) {
			GAMLocationEntry *gle = newGame->GetSavedLocationEntry(i);
			str->ReadResRef( gle->AreaResRef );
			str->ReadWord( &PosX );
			str->ReadWord( &PosY );
			gle->Pos.x=PosX;
			gle->Pos.y=PosY;
		}
	}

	if (PPLocCount && PPLocOffset) {
		ieWord PosX, PosY;

		str->Seek( PPLocOffset, GEM_STREAM_START );
		for (i=0; i<PPLocCount; i++) {
			GAMLocationEntry *gle = newGame->GetPlaneLocationEntry(i);
			str->ReadResRef( gle->AreaResRef );
			str->ReadWord( &PosX );
			str->ReadWord( &PosY );
			gle->Pos.x=PosX;
			gle->Pos.y=PosY;
		}
	}
	return newGame;
}

static void SanityCheck(ieWord a,ieWord &b,const char *message)
{
	if (a==0xffff) {
		b=0xffff;
		return;
	}
	if (b==0xffff) {
		Log(ERROR, "GAMImporter", "Invalid Slot Enabler caught: %s!", message);
		b=0;
	}
}

Actor* GAMImporter::GetActor(Holder<ActorMgr> aM, bool is_in_party )
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
	for (i=0;i<24;i++) {
		str->ReadDword( &pcInfo.Interact[i] ); //interact counters
	}

	bool extended = version==GAM_VER_GEMRB || version==GAM_VER_IWD2;
	if (extended) {
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
				str->Read( tmp, 8 );
				if ((tmp[0]!=0) && (pcInfo.QuickSpellResRef[0]==0)) {
					memcpy( pcInfo.QuickSpellResRef[i], tmp, 8);
					//innates
					pcInfo.QuickSpellClass[i]=0xff;
				}
			}
			//recently discovered fields (bard songs)
			//str->Seek( 72, GEM_CURRENT_POS);
			for(i = 0; i<MAX_QSLOTS;i++) {
				str->Read( tmp, 8 );
				if ((tmp[0]!=0) && (pcInfo.QuickSpellResRef[0]==0)) {
					memcpy( pcInfo.QuickSpellResRef[i], tmp, 8);
					//bardsongs
					pcInfo.QuickSpellClass[i]=0xfe;
				}
			}
		}
		//QuickSlots are customisable in iwd2 and GemRB
		//thus we adopt the iwd2 style actor info
		//the first 3 slots are hardcoded anyway
		pcInfo.QSlots[0] = ACT_TALK;
		pcInfo.QSlots[1] = ACT_WEAPON1;
		pcInfo.QSlots[2] = ACT_WEAPON2;
		for (i=0;i<MAX_QSLOTS;i++) {
			str->ReadDword( &tmpDword );
			pcInfo.QSlots[i+3] = (ieByte) tmpDword;
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
		DataStream* ms = SliceStream( str, pcInfo.OffsetToCRE, pcInfo.CRESize );
		if (ms) {
			aM->Open(ms);
			actor = aM->GetActor(tmpWord);
		}

		//torment has them as 0 or -1
		if (pcInfo.Name[0]!=0 && pcInfo.Name[0]!=UNINITIALIZED_CHAR) {
			actor->SetName(pcInfo.Name,0); //setting both names
		}
		actor->TalkCount = pcInfo.TalkCount;
	} else {
		DataStream* ds = gamedata->GetResource(
				pcInfo.CREResRef, IE_CRE_CLASS_ID );
		//another plugin cannot free memory stream from this plugin
		//so auto free is a no-no
		if (ds) {
			aM->Open(ds);
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
	GetPCStats(ps, extended);
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
	actor->SetModalSpell(pcInfo.ModalState, 0);
	ps->Happiness = pcInfo.Happiness;
	memcpy(ps->Interact, pcInfo.Interact, MAX_INTERACT *sizeof(ieDword) );

	actor->SetPersistent( tmpWord );

	actor->Selected = pcInfo.Selected;
	return actor;
}

void GAMImporter::GetPCStats (PCStatsStruct *ps, bool extended)
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
	
	//iwd2 has some PC only stats that the player can set (this can be done via a guiscript interface)
	if (extended) {
		//3 - expertise
		//4 - power attack
		//5 - arterial strike
		//6 - hamstring
		//7 - rapid shot
		for (i=0;i<16;i++) {
			str->ReadDword( &ps->ExtraSettings[i] );
		}
	}
}

GAMJournalEntry* GAMImporter::GetJournalEntry()
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

int GAMImporter::GetStoredFileSize(Game *game)
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

	PluginHolder<ActorMgr> am(IE_CRE_CLASS_ID);
	PCCount = game->GetPartySize(false);
	headersize += PCCount * PCSize;
	for (i = 0;i<PCCount; i++) {
		Actor *ac = game->GetPC(i, false);
		headersize += am->GetStoredFileSize(ac);
	}
	NPCOffset = headersize;

	NPCCount = game->GetNPCCount();
	headersize += NPCCount * PCSize;
	for (i = 0;i<NPCCount; i++) {
		Actor *ac = game->GetNPC(i);
		headersize += am->GetStoredFileSize(ac);
	}

	if (game->mazedata) {
		MazeOffset = headersize;
		//due to alignment the internal size is not the same as the external size
		headersize += MAZE_DATA_SIZE_HARDCODED;
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
			headersize += BESTIARY_SIZE;
		}
		if (game->version!=GAM_VER_PST) {
			headersize += 9 * 8 + 82 * 4;
		}
	}

	SavedLocOffset = headersize;
	SavedLocCount = game->GetSavedLocationCount();
	//there is an unknown dword at the end of iwd2 savegames
	if (game->version==GAM_VER_IWD2) {
		headersize += 4;
	}
	headersize += SavedLocCount*12;

	PPLocOffset = headersize;
	PPLocCount = game->GetPlaneLocationCount();

	return headersize + PPLocCount * 12;
}

int GAMImporter::PutJournals(DataStream *stream, Game *game)
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

//only in ToB (and iwd2)
int GAMImporter::PutSavedLocations(DataStream *stream, Game *game)
{
	ieWord tmpWord;
	ieDword filling = 0;

	//iwd2 has a single 0 dword here (at the end of the file)
	//it could be a hacked out saved location list (inherited from SoA)
	//if the field is missing, original engine cannot load this saved game
	if (game->version==GAM_VER_IWD2) {
		stream->WriteDword(&filling);
		return 0;
	}

	for (unsigned int i=0;i<SavedLocCount;i++) {
			GAMLocationEntry *j = game->GetSavedLocationEntry(i);

			stream->WriteResRef(j->AreaResRef);
			tmpWord = j->Pos.x;
			stream->WriteWord(&tmpWord);
			tmpWord = j->Pos.y;
			stream->WriteWord(&tmpWord);
	}
	return 0;
}

int GAMImporter::PutPlaneLocations(DataStream *stream, Game *game)
{
	ieWord tmpWord;

	for (unsigned int i=0;i<PPLocCount;i++) {
			GAMLocationEntry *j = game->GetPlaneLocationEntry(i);

			stream->WriteResRef(j->AreaResRef);
			tmpWord = j->Pos.x;
			stream->WriteWord(&tmpWord);
			tmpWord = j->Pos.y;
			stream->WriteWord(&tmpWord);
	}
	return 0;
}

//only in PST
int GAMImporter::PutKillVars(DataStream *stream, Game *game)
{
	char filling[40];
	ieVariable tmpname;
	Variables::iterator pos=NULL;
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

int GAMImporter::PutVariables(DataStream *stream, Game *game)
{
	char filling[40];
	ieVariable tmpname;
	Variables::iterator pos=NULL;
	const char *name;
	ieDword value;

	memset(filling,0,sizeof(filling) );
	for (unsigned int i=0;i<GlobalCount;i++) {
		//global variables are locals for game, that's why the local/global confusion
		pos=game->locals->GetNextAssoc( pos, name, value);
		strnspccpy(tmpname, name, 32);
		stream->Write( tmpname, 32);
		stream->Write( filling, 8);
		stream->WriteDword( &value);
		//40 bytes of empty crap
		stream->Write( filling, 40);
	}
	return 0;
}

int GAMImporter::PutHeader(DataStream *stream, Game *game)
{
	int i;
	char Signature[10];
	ieDword tmpDword;

	memcpy( Signature, "GAMEV0.0", 8);
	Signature[5]+=game->version/10;
	if (game->version==GAM_VER_PST || game->version==GAM_VER_BG) { //pst/bg1 saved version
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
	//hack because we don't need this
	game->NpcInParty=PCCount-1;
	stream->WriteWord( &game->NpcInParty );
	stream->WriteWord( &game->WeatherBits );
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
		stream->WriteDword( &game->Expansion);
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
	stream->WriteDword( &PPLocOffset);
	stream->WriteDword( &PPLocCount);
	char filling[52];
	memset( filling, 0, sizeof(filling) );
	stream->Write( &filling, 52); //unknown

	//save failed, but it is not our fault, returning now before the asserts kill us
	if (stream->GetPos()==0) {
		return -1;
	}
	return 0;
}

int GAMImporter::PutActor(DataStream *stream, Actor *ac, ieDword CRESize, ieDword CREOffset, ieDword version)
{
	int i;
	ieDword tmpDword;
	ieWord tmpWord;
	char filling[130];

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
	//BG1 doesn't even like the * in there, zero fill
	//seems to be accepted by all
	stream->Write( filling, 8);
	tmpDword = ac->GetOrientation();
	stream->WriteDword( &tmpDword);
	stream->WriteResRef(ac->Area);
	tmpWord = ac->Pos.x;
	stream->WriteWord( &tmpWord);
	tmpWord = ac->Pos.y;
	stream->WriteWord( &tmpWord);
	//no viewport, we cheat
	tmpWord = ac->Pos.x-core->Width/2;
	stream->WriteWord( &tmpWord);
	tmpWord = ac->Pos.y-core->Height/2;
	stream->WriteWord( &tmpWord);
	tmpWord = (ieWord) ac->ModalState;
	stream->WriteWord( &tmpWord);
	tmpWord = ac->PCStats->Happiness;
	stream->WriteWord( &tmpWord);
	//interact counters
	for (i=0;i<24;i++) {
		stream->WriteDword( ac->PCStats->Interact+i);
	}

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
		memset(filling,0,sizeof(filling) );
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
			tmpDword = ac->PCStats->QSlots[i+3];
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
		//I don't know how many fields are actually used in IWD2 saved game
		//but we got at least 8 (and only 5 of those are actually used)
		for(i=0;i<16;i++) {
			stream->WriteDword( &ac->PCStats->ExtraSettings[i]);
		}
		stream->Write(filling, 130);
	}

	return 0;
}

int GAMImporter::PutPCs(DataStream *stream, Game *game)
{
	unsigned int i;
	PluginHolder<ActorMgr> am(IE_CRE_CLASS_ID);
	ieDword CREOffset = PCOffset + PCCount * PCSize;

	for(i=0;i<PCCount;i++) {
		assert(stream->GetPos() == PCOffset + i * PCSize);
		Actor *ac = game->GetPC(i, false);
		ieDword CRESize = am->GetStoredFileSize(ac);
		PutActor(stream, ac, CRESize, CREOffset, game->version);
		CREOffset += CRESize;
	}

	CREOffset = PCOffset + PCCount * PCSize; // just for the asserts..
	assert(stream->GetPos() == CREOffset);

	for(i=0;i<PCCount;i++) {
		assert(stream->GetPos() == CREOffset);
		Actor *ac = game->GetPC(i, false);
		//reconstructing offsets again
		CREOffset += am->GetStoredFileSize(ac);
		am->PutActor( stream, ac);
	}
	assert(stream->GetPos() == CREOffset);
	return 0;
}

int GAMImporter::PutNPCs(DataStream *stream, Game *game)
{
	unsigned int i;
	PluginHolder<ActorMgr> am(IE_CRE_CLASS_ID);
	ieDword CREOffset = NPCOffset + NPCCount * PCSize;

	for(i=0;i<NPCCount;i++) {
		assert(stream->GetPos() == NPCOffset + i * PCSize);
		Actor *ac = game->GetNPC(i);
		ieDword CRESize = am->GetStoredFileSize(ac);
		PutActor(stream, ac, CRESize, CREOffset, game->version);
		CREOffset += CRESize;
	}
	CREOffset = NPCOffset + NPCCount * PCSize; // just for the asserts..
	assert(stream->GetPos() == CREOffset);

	for(i=0;i<NPCCount;i++) {
		assert(stream->GetPos() == CREOffset);
		Actor *ac = game->GetNPC(i);
		//reconstructing offsets again
		CREOffset += am->GetStoredFileSize(ac);
		am->PutActor( stream, ac);
	}
	assert(stream->GetPos() == CREOffset);
	return 0;
}

void GAMImporter::GetMazeHeader(void *memory)
{
	maze_header *m = (maze_header *) memory;
	str->ReadDword( &m->maze_sizex );
	str->ReadDword( &m->maze_sizey );
	str->ReadDword( &m->pos1x );
	str->ReadDword( &m->pos1y );
	str->ReadDword( &m->pos2x );
	str->ReadDword( &m->pos2y );
	str->ReadDword( &m->pos3x );
	str->ReadDword( &m->pos3y );
	str->ReadDword( &m->pos4x );
	str->ReadDword( &m->pos4y );
	str->ReadDword( &m->trapcount );
	str->ReadDword( &m->initialized );
	str->ReadDword( &m->unknown2c );
	str->ReadDword( &m->unknown30 );
}

void GAMImporter::GetMazeEntry(void *memory)
{
	maze_entry *h = (maze_entry *) memory;

	str->ReadDword( &h->override );
	str->ReadDword( &h->valid );
	str->ReadDword( &h->accessible );
	str->ReadDword( &h->trapped );
	str->ReadDword( &h->traptype );
	str->ReadWord( &h->walls );
	str->ReadDword( &h->visited );
}

void GAMImporter::PutMazeHeader(DataStream *stream, void *memory)
{
	maze_header *m = (maze_header *) memory;
	stream->WriteDword( &m->maze_sizex );
	stream->WriteDword( &m->maze_sizey );
	stream->WriteDword( &m->pos1x );
	stream->WriteDword( &m->pos1y );
	stream->WriteDword( &m->pos2x );
	stream->WriteDword( &m->pos2y );
	stream->WriteDword( &m->pos3x );
	stream->WriteDword( &m->pos3y );
	stream->WriteDword( &m->pos4x );
	stream->WriteDword( &m->pos4y );
	stream->WriteDword( &m->trapcount );
	stream->WriteDword( &m->initialized );
	stream->WriteDword( &m->unknown2c );
	stream->WriteDword( &m->unknown30 );
}

void GAMImporter::PutMazeEntry(DataStream *stream, void *memory)
{
	maze_entry *h = (maze_entry *) memory;
	stream->WriteDword( &h->override );
	stream->WriteDword( &h->valid );
	stream->WriteDword( &h->accessible );
	stream->WriteDword( &h->trapped );
	stream->WriteDword( &h->traptype );
	stream->WriteWord( &h->walls );
	stream->WriteDword( &h->visited );
}

int GAMImporter::PutMaze(DataStream *stream, Game *game)
{
	for(int i=0;i<MAZE_ENTRY_COUNT;i++) {
		PutMazeEntry(stream, game->mazedata+i*MAZE_ENTRY_SIZE);
	}
	PutMazeHeader(stream, game->mazedata+MAZE_ENTRY_COUNT*MAZE_ENTRY_SIZE);
	return 0;
}

int GAMImporter::PutFamiliars(DataStream *stream, Game *game)
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

int GAMImporter::PutGame(DataStream *stream, Game *game)
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
	if (SavedLocOffset) {
		ret = PutSavedLocations( stream, game);
		if (ret) {
			return ret;
		}
	}
	if (PPLocOffset) {
		ret = PutPlaneLocations( stream, game);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

#include "plugindef.h"

GEMRB_PLUGIN(0xD7F7040, "GAM File Importer")
PLUGIN_CLASS(IE_GAM_CLASS_ID, GAMImporter)
END_PLUGIN()
