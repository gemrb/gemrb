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

#include "DataFileMgr.h"
#include "GameData.h"
#include "Interface.h"
#include "MapMgr.h"
#include "PluginMgr.h"
#include "TableMgr.h"
#include "Scriptable/Actor.h"
#include "Streams/SlicedStream.h"

#include <cassert>

namespace GemRB {

#define FAMILIAR_FILL_SIZE 324
// if your compiler chokes on this, use -1 or 0xff whichever works for you
#define UNINITIALIZED_CHAR '\xff'

bool GAMImporter::Import(DataStream* str)
{
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
		if (core->HasFeature(GFFlags::HAS_KAPUTZ) ) { //pst
			PCSize = 0x168;
			version = GAM_VER_PST;
			//sound folder name takes up this space,
			//so it is handy to make this check
		} else if ( core->HasFeature(GFFlags::SOUNDFOLDERS) ) {
			PCSize = 0x180;
			version = GAM_VER_IWD;
		} else {
			PCSize = 0x160;
			version=GAM_VER_BG;
		}
	} else {
		Log(ERROR, "GAMImporter", "This file is not a valid GAM File! Actual signature: {}", Signature);
		return false;
	}

	return true;
}

Game* GAMImporter::LoadGame(Game *newGame, int ver_override)
{
	// saving in original version requires the original version
	// otherwise it is set to 0 at construction time
	if (core->config.SaveAsOriginal) {
		// HACK: default icewind2.gam is 2.0! handled by script
		if(ver_override) {
			newGame->version = ver_override;
		}
		else {
			newGame->version = version;
		}
	}

	ieDword GameTime;
	str->ReadDword(GameTime);
	newGame->GameTime = GameTime * core->Time.defaultTicksPerSec;

	str->ReadWord(newGame->WhichFormation);
	for (unsigned short& formation : newGame->Formations) {
		str->ReadWord(formation);
	}
	//hack for PST
	if (version==GAM_VER_PST) {
		newGame->Formations[0] = newGame->WhichFormation;
		newGame->WhichFormation = 0;
	}
	str->ReadDword(newGame->PartyGold);
	str->ReadWord(newGame->NPCAreaViewed); // area of selected PC (overrides stored CurrentArea); in ToB this is named 'nPCAreaViewed'
	str->ReadWord(newGame->WeatherBits);
	str->ReadDword(PCOffset);
	str->ReadDword(PCCount);
	//these fields are not really used by any engine, and never saved
	//str->ReadDword(UnknownOffset);
	//str->ReadDword(UnknownCount);
	str->Seek( 8, GEM_CURRENT_POS);
	str->ReadDword(NPCOffset);
	str->ReadDword(NPCCount);
	str->ReadDword(GlobalOffset);
	str->ReadDword(GlobalCount);
	str->ReadResRef(newGame->LastMasterArea); // this is the 'master area', different for subareas
	str->ReadDword(newGame->CurrentLink);//in ToB this is named 'currentLink'
	str->ReadDword(JournalCount);
	str->ReadDword(JournalOffset);
	switch (version) {
		default:
			MazeOffset = 0;
			str->ReadDword(newGame->Reputation);
			str->ReadResRef(newGame->CurrentArea);
			newGame->AnotherArea = newGame->CurrentArea;
			str->ReadDword(newGame->ControlStatus);
			str->ReadDword(newGame->Expansion);
			str->ReadDword(FamiliarsOffset);
			str->ReadDword(SavedLocOffset);
			str->ReadDword(SavedLocCount);

			// iwd2 HoF mode was stored at the bg2 location of SavedLocOffset
			if (version == GAM_VER_IWD2) {
				newGame->HOFMode = SavedLocOffset == 1;
			}

			str->ReadDword(newGame->RealTime);
			str->ReadDword(PPLocOffset);
			str->ReadDword(PPLocCount);
			str->ReadDword(newGame->zoomLevel);
			str->Seek(48, GEM_CURRENT_POS);
			// TODO: EEs used up these bits, see https://gibberlings3.github.io/iesdp/file_formats/ie_formats/gam_v2.0.htm#GAMEV2_0_Header
			break;

		case GAM_VER_PST:
			str->ReadDword(MazeOffset);
			str->ReadDword(newGame->Reputation);
			str->ReadResRef( newGame->AnotherArea );
			str->ReadDword(KillVarsOffset);
			str->ReadDword(KillVarsCount);
			str->ReadDword(FamiliarsOffset); //bestiary
			str->ReadResRef( newGame->AnotherArea ); //yet another area
			SavedLocOffset = 0;
			SavedLocCount = 0;
			PPLocOffset = 0;
			PPLocCount = 0;
			str->Seek( 64, GEM_CURRENT_POS);
			break;
	}

	if (newGame->CurrentArea.IsEmpty()) {
		// 0 - normal, 1 - tutorial, 2 - extension
		AutoTable tm = gamedata->LoadTable("STARTARE");
		assert(tm);
		ieDword playmode = 0;
		//only bg2 has 9 rows (iwd's have 6 rows - normal+extension)
		if (tm->GetRowCount() == 9) {
			playmode = core->GetVariable("PlayMode", 0);
			playmode *= 3;
		}

		newGame->CurrentArea = tm->QueryField(playmode, 0);
	}

	//Loading PCs
	auto aM = GetImporter<ActorMgr>(IE_CRE_CLASS_ID);
	for (unsigned int i = 0; i < PCCount; i++) {
		str->Seek( PCOffset + ( i * PCSize ), GEM_STREAM_START );
		Actor *actor = GetActor( aM, true );
		newGame->JoinParty( actor, actor->Selected?JP_SELECT:0 );
		// potentially override the current area, now that the pcs are loaded
		if (newGame->version != GAM_VER_PST && static_cast<ieWord>(actor->InParty - 1) == newGame->NPCAreaViewed) {
			newGame->CurrentArea = actor->Area;
			newGame->AnotherArea = newGame->CurrentArea;
		}
	}

	//Loading NPCs
	for (unsigned int i = 0; i < NPCCount; i++) {
		str->Seek( NPCOffset + ( i * PCSize ), GEM_STREAM_START );
		Actor *actor = GetActor( aM, false );
		newGame->AddNPC( actor );
	}

	//apparently BG1/IWD2 relies on this, if chapter is unset, it is
	//set to -1, hopefully it won't break anything
	//PST has no chapter variable by default, and would crash on one
	if (!core->HasFeature(GFFlags::NO_NEW_VARIABLES)) {
		newGame->locals["CHAPTER"] = -1;
	}

	// load initial values from var.var
	core->LoadInitialValues("GLOBAL", newGame->locals);

	//Loading Global Variables
	ieVariable Name;
	str->Seek( GlobalOffset, GEM_STREAM_START );
	for (unsigned int i = 0; i < GlobalCount; i++) {
		ieDword Value;
		str->ReadVariable(Name);
		str->Seek( 8, GEM_CURRENT_POS );
		str->ReadDword(Value);
		str->Seek( 40, GEM_CURRENT_POS );
		newGame->locals[Name] = Value;
	}
	if(core->HasFeature(GFFlags::HAS_KAPUTZ) ) {
		// load initial values from var.var
		core->LoadInitialValues("KAPUTZ", newGame->kaputz);
		str->Seek( KillVarsOffset, GEM_STREAM_START );
		for (unsigned int i = 0; i < KillVarsCount; i++) {
			ieDword Value;
			str->ReadVariable(Name);
			str->Seek( 8, GEM_CURRENT_POS );
			str->ReadDword(Value);
			str->Seek( 40, GEM_CURRENT_POS );
			newGame->kaputz[Name] = Value;
		}
	}

	//Loading Journal entries
	str->Seek( JournalOffset, GEM_STREAM_START );
	for (unsigned int i = 0; i < JournalCount; i++) {
		GAMJournalEntry* je = GetJournalEntry();
		newGame->AddJournalEntry( je );
	}

	if (version == GAM_VER_PST) {
		//loading maze
		if (MazeOffset) {
			//Don't allocate memory in plugins (MSVC chokes on this)
			newGame->AllocateMazeData();
			str->Seek(MazeOffset, GEM_STREAM_START );
			for (unsigned int i = 0; i < MAZE_ENTRY_COUNT; i++) {
				GetMazeEntry(newGame->mazedata+i*MAZE_ENTRY_SIZE);
			}
			GetMazeHeader(newGame->mazedata+MAZE_ENTRY_COUNT*MAZE_ENTRY_SIZE);
		}
		str->Seek( FamiliarsOffset, GEM_STREAM_START );
	} else {
		if (FamiliarsOffset) {
			str->Seek( FamiliarsOffset, GEM_STREAM_START );
			for (unsigned int i = 0; i < 9; i++) {
				ResRef tmp;
				str->ReadResRef(tmp);
				newGame->SetFamiliar(tmp, i);
			}
		}
	}
	// Loading known creatures array (beasts)
	if(core->GetBeastsINI() != NULL) {
		int beasts_count = BESTIARY_SIZE;
		if(FamiliarsOffset) {
			str->Read(newGame->beasts.data(), beasts_count);
		}
	}

	if (SavedLocCount && SavedLocOffset) {
		str->Seek( SavedLocOffset, GEM_STREAM_START );
		for (unsigned int i = 0; i < SavedLocCount; i++) {
			GAMLocationEntry *gle = newGame->GetSavedLocationEntry(i);
			str->ReadResRef( gle->AreaResRef );
			str->ReadPoint(gle->Pos);
		}
	}

	if (PPLocCount && PPLocOffset) {
		str->Seek( PPLocOffset, GEM_STREAM_START );
		for (unsigned int i = 0; i < PPLocCount; i++) {
			GAMLocationEntry *gle = newGame->GetPlaneLocationEntry(i);
			str->ReadResRef( gle->AreaResRef );
			str->ReadPoint(gle->Pos);
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
		Log(ERROR, "GAMImporter", "Invalid Slot Enabler caught: {}!", message);
		b=0;
	}
}

struct PCStruct {
	ieWord   Selected;
	ieWord   PartyOrder;
	ieDword  OffsetToCRE;
	ieDword  CRESize;
	ResRef CREResRef;
	ieDword  Orientation;
	ResRef Area;
	Point Pos;
	Point   ViewPos;
	Modal   ModalState;
	ieWordSigned   Happiness;
	ieDword  Interact[MAX_INTERACT];
	ieWord   QuickWeaponSlot[MAX_QUICKWEAPONSLOT];
	ieWord   QuickWeaponHeader[MAX_QUICKWEAPONSLOT];
	ResRef QuickSpellResRef[MAX_QSLOTS];
	ieWord   QuickItemSlot[MAX_QUICKITEMSLOT];
	ieWord   QuickItemHeader[MAX_QUICKITEMSLOT];
	ieVariable Name;
	ieDword  TalkCount;
	ieByte QSlots[GUIBT_COUNT];
	ieByte QuickSpellClass[MAX_QSLOTS];
};

Actor* GAMImporter::GetActor(const std::shared_ptr<ActorMgr>& aM, bool is_in_party)
{
	PCStruct pcInfo{};
	ieDword tmpDword;
	ieWord tmpWord;

	str->ReadWord(pcInfo.Selected);
	str->ReadWord(pcInfo.PartyOrder);
	str->ReadDword(pcInfo.OffsetToCRE);
	str->ReadDword(pcInfo.CRESize);
	str->ReadResRef( pcInfo.CREResRef );
	str->ReadDword(pcInfo.Orientation);
	str->ReadResRef( pcInfo.Area );
	str->ReadPoint(pcInfo.Pos);
	str->ReadPoint(pcInfo.ViewPos);
	str->ReadEnum<Modal>(pcInfo.ModalState); //see Modal.ids
	str->ReadScalar<ieWordSigned>(pcInfo.Happiness);
	for (unsigned int& interact : pcInfo.Interact) {
		str->ReadDword(interact); //interact counters
	}

	bool extended = version==GAM_VER_GEMRB || version==GAM_VER_IWD2;
	if (extended) {
		ResRef tmp;

		for (unsigned int i = 0; i < 4; i++) {
			str->ReadWord(pcInfo.QuickWeaponSlot[i]);
			str->ReadWord(pcInfo.QuickWeaponSlot[i+4]);
		}
		for (unsigned int i = 0; i < 4; i++) {
			str->ReadWord(tmpWord);
			SanityCheck( pcInfo.QuickWeaponSlot[i], tmpWord, "weapon");
			pcInfo.QuickWeaponHeader[i]=tmpWord;
			str->ReadWord(tmpWord);
			SanityCheck( pcInfo.QuickWeaponSlot[i+4], tmpWord, "weapon");
			pcInfo.QuickWeaponHeader[i+4]=tmpWord;
		}
		for (auto& spell : pcInfo.QuickSpellResRef) {
			str->ReadResRef(spell);
		}
		str->Read( &pcInfo.QuickSpellClass, MAX_QSLOTS ); //9 bytes

		str->Seek( 1, GEM_CURRENT_POS); //skipping a padding byte
		for (unsigned int i = 0; i < 3; i++) {
			str->ReadWord(pcInfo.QuickItemSlot[i]);
		}
		for (unsigned int i = 0; i < 3; i++) {
			str->ReadWord(tmpWord);
			SanityCheck( pcInfo.QuickItemSlot[i], tmpWord, "item");
			pcInfo.QuickItemHeader[i]=tmpWord;
		}
		pcInfo.QuickItemHeader[3]=0xffff;
		pcInfo.QuickItemHeader[4]=0xffff;
		if (version == GAM_VER_IWD2) {
			//quick innates
			//we spare some memory and time by storing them in the same place
			//this may be slightly buggy because IWD2 doesn't clear the
			//fields, but QuickSpellClass is set correctly
			for (unsigned int i = 0; i < MAX_QSLOTS; i++) {
				str->ReadResRef(tmp);
				if (!tmp.IsEmpty() && pcInfo.QuickSpellResRef[i].IsEmpty()) {
					pcInfo.QuickSpellResRef[i] = tmp;
					//innates
					pcInfo.QuickSpellClass[i]=0xff;
				}
			}
			//recently discovered fields (bard songs)
			for (unsigned int i = 0; i < MAX_QSLOTS; i++) {
				str->ReadResRef(tmp);
				if (!tmp.IsEmpty() && pcInfo.QuickSpellResRef[i].IsEmpty()) {
					pcInfo.QuickSpellResRef[i] = tmp;
					//bardsongs
					pcInfo.QuickSpellClass[i]=0xfe;
				}
			}
		}
		//QuickSlots are customisable in iwd2 and GemRB
		//thus we adopt the iwd2 style actor info
		//the first 3 slots are hardcoded anyway
		// in iwd2 0 is guard, bg2 talk
		// in iwd1 and bg1 0 is guard, 1 talk, everything shifted one further
		pcInfo.QSlots[0] = ACT_DEFEND;
		pcInfo.QSlots[1] = ACT_WEAPON1;
		pcInfo.QSlots[2] = ACT_WEAPON2;
		for (unsigned int i = 0; i < MAX_QSLOTS; i++) {
			str->ReadDword(tmpDword);
			pcInfo.QSlots[i+3] = (ieByte) tmpDword;
		}
	} else {
		for (unsigned int i = 0; i < 4; i++) {
			str->ReadWord(pcInfo.QuickWeaponSlot[i]);
		}
		for (unsigned int i = 0; i < 4; i++) {
			str->ReadWord(tmpWord);
			SanityCheck( pcInfo.QuickWeaponSlot[i], tmpWord, "weapon");
			pcInfo.QuickWeaponHeader[i]=tmpWord;
		}
		for (unsigned int i = 0; i < 3; i++) {
			str->ReadResRef(pcInfo.QuickSpellResRef[i]);
		}
		if (version==GAM_VER_PST) { //Torment
			for (unsigned short& slot : pcInfo.QuickItemSlot) {
				str->ReadWord(slot);
			}
			for (unsigned int i = 0; i < 5; i++) {
				str->ReadWord(tmpWord);
				SanityCheck( pcInfo.QuickItemSlot[i], tmpWord, "item");
				pcInfo.QuickItemHeader[i]=tmpWord;
			}
			//str->Seek( 10, GEM_CURRENT_POS ); //enabler fields
		} else {
			for (unsigned int i = 0; i < 3; i++) {
				str->ReadWord(pcInfo.QuickItemSlot[i]);
			}
			for (unsigned int i = 0; i < 3; i++) {
				str->ReadWord(tmpWord);
				SanityCheck( pcInfo.QuickItemSlot[i], tmpWord, "item");
				pcInfo.QuickItemHeader[i]=tmpWord;
			}
			//str->Seek( 6, GEM_CURRENT_POS ); //enabler fields
		}
		pcInfo.QSlots[0] = 0xff; //(invalid, will be regenerated)
	}
	str->ReadVariable(pcInfo.Name);
	str->ReadDword(pcInfo.TalkCount);

	size_t pos = str->GetPos();

	Actor* actor = NULL;
	tmpWord = is_in_party ? (pcInfo.PartyOrder + 1) : 0;

	if (pcInfo.OffsetToCRE) {
		DataStream* ms = SliceStream( str, pcInfo.OffsetToCRE, pcInfo.CRESize );
		if (ms) {
			aM->Open(ms);
			actor = aM->GetActor(tmpWord);
		}

		//torment has them as 0 or -1
		if (pcInfo.Name[0] != 0) {
			actor->SetName(StringFromCString(pcInfo.Name.c_str()), 0);
		}
		actor->TalkCount = pcInfo.TalkCount;
	} else {
		DataStream* ds = gamedata->GetResourceStream(pcInfo.CREResRef, IE_CRE_CLASS_ID);
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
	for (int i = 0; i < MAX_QSLOTS; i++) {
		ps->QuickSpells[i] = pcInfo.QuickSpellResRef[i];
	}
	memcpy(ps->QuickSpellBookType, pcInfo.QuickSpellClass, MAX_QSLOTS);
	memcpy(ps->QuickWeaponSlots, pcInfo.QuickWeaponSlot, MAX_QUICKWEAPONSLOT*sizeof(ieWord) );
	memcpy(ps->QuickWeaponHeaders, pcInfo.QuickWeaponHeader, MAX_QUICKWEAPONSLOT*sizeof(ieWord) );
	memcpy(ps->QuickItemSlots, pcInfo.QuickItemSlot, MAX_QUICKITEMSLOT*sizeof(ieWord) );
	memcpy(ps->QuickItemHeaders, pcInfo.QuickItemHeader, MAX_QUICKITEMSLOT*sizeof(ieWord) );
	actor->ReinitQuickSlots();
	actor->Destination = actor->Pos = pcInfo.Pos;
	actor->Area = pcInfo.Area;
	actor->SetOrientation(ClampToOrientation(pcInfo.Orientation), false);
	actor->TalkCount = pcInfo.TalkCount;
	actor->Modal.State = pcInfo.ModalState;
	actor->SetModalSpell(actor->Modal.State, {});
	ps->Happiness = pcInfo.Happiness;
	memcpy(ps->Interact, pcInfo.Interact, MAX_INTERACT *sizeof(ieDword) );

	actor->SetPersistent( tmpWord );

	actor->Selected = pcInfo.Selected;
	return actor;
}

void GAMImporter::GetPCStats (PCStatsStruct *ps, bool extended)
{
	str->ReadStrRef(ps->BestKilledName);
	str->ReadDword(ps->BestKilledXP);
	str->ReadDword(ps->AwayTime);
	str->ReadDword(ps->JoinDate);
	str->ReadDword(ps->unknown10);
	str->ReadDword(ps->KillsChapterXP);
	str->ReadDword(ps->KillsChapterCount);
	str->ReadDword(ps->KillsTotalXP);
	str->ReadDword(ps->KillsTotalCount);
	for (int i = 0; i <= 3; i++) {
		str->ReadResRef( ps->FavouriteSpells[i] );
	}
	for (int i = 0; i <= 3; i++)
		str->ReadWord(ps->FavouriteSpellsCount[i]);

	for (int i = 0; i <= 3; i++) {
		str->ReadResRef( ps->FavouriteWeapons[i] );
	}
	for (int i = 0; i <= 3; i++)
		str->ReadWord(ps->FavouriteWeaponsCount[i]);

	str->ReadResRef( ps->SoundSet );

	if (core->HasFeature(GFFlags::SOUNDFOLDERS) ) {
		ieVariable soundFolder;
		str->ReadVariable(soundFolder);
		ps->SoundFolder = StringFromCString(soundFolder.c_str());
	}
	
	//iwd2 has some PC only stats that the player can set (this can be done via a guiscript interface)
	if (extended) {
		//3 - expertise
		//4 - power attack
		//5 - arterial strike
		//6 - hamstring
		//7 - rapid shot
		for (unsigned int& extraSetting : ps->ExtraSettings) {
			str->ReadDword(extraSetting);
		}
	}
}

GAMJournalEntry* GAMImporter::GetJournalEntry()
{
	GAMJournalEntry* j = new GAMJournalEntry();

	str->ReadStrRef(j->Text);
	str->ReadDword(j->GameTime);
	//this could be wrong, most likely these are 2 words, or a dword
	str->Read( &j->Chapter, 1 );
	str->Read( &j->unknown09, 1 );
	str->Read( &j->Section, 1 );
	str->Read( &j->Group, 1 ); // this is a GemRB extension

	return j;
}

int GAMImporter::GetStoredFileSize(const Game *game)
{
	int headersize;

	//moved this here, so one can disable killvars in a pst style game
	//or enable them in gemrb
	if(core->HasFeature(GFFlags::HAS_KAPUTZ) ) {
		KillVarsCount = static_cast<ieDword>(game->kaputz.size());
	} else {
		KillVarsCount = 0;
	}
	switch(game->version)
	{
	case GAM_VER_IWD:
		headersize = 0xb4;
		PCSize = 0x180;
		break;
	case GAM_VER_BG:
	case GAM_VER_BG2:
	case GAM_VER_TOB:
	case GAM_VER_GEMRB:
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

	auto am = GetImporter<ActorMgr>(IE_CRE_CLASS_ID);
	PCCount = game->GetPartySize(false);
	headersize += PCCount * PCSize;
	for (unsigned int i = 0; i < PCCount; i++) {
		const Actor *ac = game->GetPC(i, false);
		headersize += am->GetStoredFileSize(ac);
	}
	NPCOffset = headersize;

	NPCCount = game->GetNPCCount();
	headersize += NPCCount * PCSize;
	for (unsigned int i = 0; i < NPCCount; i++) {
		const Actor *ac = game->GetNPC(i);
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

	GlobalCount = static_cast<ieDword>(game->locals.size());
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

	if (game->version == GAM_VER_IWD2) {
		SavedLocOffset = game->HOFMode;
		SavedLocCount = 0;
		// there is an unknown dword at the end of iwd2 savegames (see PutSavedLocations)
		headersize += 4;
	} else {
		SavedLocOffset = headersize;
		SavedLocCount = game->GetSavedLocationCount();
	}
	headersize += SavedLocCount*12;

	PPLocOffset = headersize;
	PPLocCount = game->GetPlaneLocationCount();

	return headersize + PPLocCount * 12;
}

int GAMImporter::PutJournals(DataStream *stream, const Game *game) const
{
	for (unsigned int i=0;i<JournalCount;i++) {
		const GAMJournalEntry *j = game->GetJournalEntry(i);

		stream->WriteStrRef(j->Text);
		stream->WriteDword(j->GameTime);
		//this could be wrong, most likely these are 2 words, or a dword
		stream->Write( &j->Chapter, 1 );
		stream->Write( &j->unknown09, 1 );
		stream->Write( &j->Section, 1 );
		stream->Write( &j->Group, 1 ); // this is a GemRB extension
	}

	return 0;
}

//only in ToB (and iwd2)
int GAMImporter::PutSavedLocations(DataStream *stream, Game *game) const
{
	//iwd2 has a single 0 dword here (at the end of the file)
	//it could be a hacked out saved location list (inherited from SoA)
	//if the field is missing, original engine cannot load this saved game
	if (game->version==GAM_VER_IWD2) {
		stream->WriteDword(0);
		return 0;
	}

	for (unsigned int i=0;i<SavedLocCount;i++) {
			const GAMLocationEntry *j = game->GetSavedLocationEntry(i);

			stream->WriteResRef(j->AreaResRef);
			stream->WritePoint(j->Pos);
	}
	return 0;
}

int GAMImporter::PutPlaneLocations(DataStream *stream, Game *game) const
{
	for (unsigned int i=0;i<PPLocCount;i++) {
			const GAMLocationEntry *j = game->GetPlaneLocationEntry(i);

			stream->WriteResRef(j->AreaResRef);
			stream->WritePoint(j->Pos);
	}
	return 0;
}

//only in PST
int GAMImporter::PutKillVars(DataStream *stream, const Game *game) const
{
	for (const auto& entry : game->kaputz) {
		//global variables are locals for game, that's why the local/global confusion
		stream->WriteVariableUC(ieVariable(entry.first));
		stream->WriteFilling(8);
		stream->WriteDword(entry.second);
		//40 bytes of empty crap
		stream->WriteFilling(40);
	}
	return 0;
}

int GAMImporter::PutVariables(DataStream *stream, const Game *game) const
{
	ieVariable tmpname;

	for (const auto& entry : game->locals) {
		//global variables are locals for game, that's why the local/global confusion

		/* PST hates to have some variables lowercased. */
		if (core->HasFeature(GFFlags::NO_NEW_VARIABLES)) {
			/* This is one anomaly that must have a space injected (PST crashes otherwise). */
			if (entry.first == "dictionary_githzerai_hjacknir") {
				tmpname = "DICTIONARY_GITHZERAI_ HJACKNIR";
			} else {
				tmpname = MakeVariable(entry.first);
			}
		} else {
			tmpname = MakeVariable(entry.first);
		}

		stream->WriteVariableUC(tmpname);
		stream->WriteFilling(8);
		stream->WriteDword(entry.second);
		//40 bytes of empty crap
		stream->WriteFilling(40);
	}
	return 0;
}

int GAMImporter::PutHeader(DataStream *stream, const Game *game) const
{
	ResRef signature = "GAMEV0.0";
	ieDword tmpDword;

	signature[5] += game->version / 10;
	if (game->version==GAM_VER_PST || game->version==GAM_VER_BG) { //pst/bg1 saved version
		signature[7] += 1;
	}
	else {
		signature[7] += game->version % 10;
	}
	stream->WriteResRef(signature);

	tmpDword = game->GameTime / core->Time.defaultTicksPerSec;
	stream->WriteDword(tmpDword);
	//pst has a single preset of formations
	if (game->version==GAM_VER_PST) {
		stream->WriteWord(game->Formations[0]);
		stream->WriteFilling(10);
	} else {
		stream->WriteWord(game->WhichFormation);
		for (const unsigned short& formation : game->Formations) {
			stream->WriteWord(formation);
		}
	}
	stream->WriteDword(game->PartyGold);
	ieWord NPCAreaViewed = -1;
	for (const auto& actor : game->selected) {
		if (actor->InParty) {
			NPCAreaViewed = actor->InParty - 1;
			break;
		}
	}
	stream->WriteWord(NPCAreaViewed);
	stream->WriteWord(game->WeatherBits);
	stream->WriteDword(PCOffset);
	stream->WriteDword(PCCount);
	//these fields are zeroed in any original savegame
	tmpDword = 0;
	stream->WriteDword(tmpDword);
	stream->WriteDword(tmpDword);
	stream->WriteDword(NPCOffset);
	stream->WriteDword(NPCCount);
	stream->WriteDword(GlobalOffset);
	stream->WriteDword(GlobalCount);
	ResRef masterArea = game->CurrentArea;
	if (!game->MasterArea(game->CurrentArea)) {
		masterArea = game->LastMasterArea; // not necessarily correct
		if (masterArea.IsEmpty() || !game->MasterArea(masterArea)) {
			masterArea = game->CurrentArea;
		}
	}
	stream->WriteResRefUC(masterArea);
	stream->WriteDword(game->CurrentLink);
	stream->WriteDword(JournalCount);
	stream->WriteDword(JournalOffset);

	switch(game->version) {
	case GAM_VER_GEMRB:
	case GAM_VER_BG:
	case GAM_VER_IWD:
	case GAM_VER_BG2:
	case GAM_VER_TOB:
	case GAM_VER_IWD2:
		stream->WriteDword(game->Reputation);
		stream->WriteResRefUC(masterArea); // current area, but usually overriden via NPCAreaViewed
		stream->WriteDword(game->ControlStatus);
		stream->WriteDword(game->Expansion);
		stream->WriteDword(FamiliarsOffset);
		stream->WriteDword(SavedLocOffset);
		stream->WriteDword(SavedLocCount);
		break;
	case GAM_VER_PST:
		stream->WriteDword(MazeOffset);
		stream->WriteDword(game->Reputation);
		stream->WriteResRefLC(game->CurrentArea);
		stream->WriteDword(KillVarsOffset);
		stream->WriteDword(KillVarsCount);
		stream->WriteDword(FamiliarsOffset);
		stream->WriteResRefLC(game->CurrentArea); //again
		break;
	}
	stream->WriteDword(game->RealTime); //this isn't correct, this field is the realtime
	stream->WriteDword(PPLocOffset);
	stream->WriteDword(PPLocCount);
	stream->WriteDword(game->zoomLevel);
	stream->WriteFilling(48); //unknown

	//save failed, but it is not our fault, returning now before the asserts kill us
	if (stream->GetPos()==0) {
		return -1;
	}
	return 0;
}

int GAMImporter::PutActor(DataStream* stream, const Actor* ac, ieDword CRESize, ieDword CREOffset, ieDword GAMVersion) const
{
	if (ac->Selected) {
		stream->WriteWord(1);
	} else {
		stream->WriteWord(0);
	}

	stream->WriteWord(ac->InParty - 1);

	stream->WriteDword(CREOffset);
	stream->WriteDword(CRESize);
	//creature resref is always unused in saved games
	//BG1 doesn't even like the * in there, zero fill
	//seems to be accepted by all
	stream->WriteFilling(8);
	stream->WriteDword(ac->GetOrientation());
	stream->WriteResRefUC(ac->Area);
	stream->WritePoint(ac->Pos);
	//no viewport, we cheat
	stream->WriteWord(ac->Pos.x - core->config.Width / 2);
	stream->WriteWord(ac->Pos.y - core->config.Height / 2);
	stream->WriteEnum(ac->Modal.State);
	stream->WriteWord(ac->PCStats->Happiness);
	//interact counters
	for (unsigned int& interact : ac->PCStats->Interact) {
		stream->WriteDword(interact);
	}

	//quickweapons
	if (GAMVersion == GAM_VER_IWD2 || GAMVersion == GAM_VER_GEMRB) {
		for (int i = 0; i < 4; i++) {
			stream->WriteWord(ac->PCStats->QuickWeaponSlots[i]);
			stream->WriteWord(ac->PCStats->QuickWeaponSlots[4 + i]);
		}
		for (int i = 0; i < 4; i++) {
			stream->WriteWord(ac->PCStats->QuickWeaponHeaders[i]);
			stream->WriteWord(ac->PCStats->QuickWeaponHeaders[4 + i]);
		}
	} else {
		for (int i = 0; i < 4; i++) {
			stream->WriteWord(ac->PCStats->QuickWeaponSlots[i]);
		}
		for (int i = 0; i < 4; i++) {
			stream->WriteWord(ac->PCStats->QuickWeaponHeaders[i]);
		}
	}

	//quickspells
	if (GAMVersion == GAM_VER_IWD2 || GAMVersion == GAM_VER_GEMRB) {
		for (int i = 0; i < MAX_QSLOTS; i++) {
			if (ac->PCStats->QuickSpellBookType[i] >= 0xfe) {
				stream->WriteFilling(8);
			} else {
				stream->WriteResRef(ac->PCStats->QuickSpells[i]);
			}
		}
		//quick spell classes, clear the field for iwd2 if it is
		//a bard song/innate slot (0xfe or 0xff)
		char filling[10] = {};
		memcpy(filling, ac->PCStats->QuickSpellBookType, MAX_QSLOTS);
		if (GAMVersion == GAM_VER_IWD2) {
			for (int i = 0; i < MAX_QSLOTS; i++) {
				if((ieByte) filling[i]>=0xfe) {
					filling[i]=0;
				}
			}
		}
		stream->Write(filling,10);
	} else {
		for (int i = 0; i < 3; i++) {
			stream->WriteResRef(ac->PCStats->QuickSpells[i]);
		}
	}

	//quick items
	switch (GAMVersion) {
	case GAM_VER_PST: case GAM_VER_GEMRB:
		for (unsigned short& quickItemSlot : ac->PCStats->QuickItemSlots) {
			stream->WriteWord(quickItemSlot);
		}
		for (unsigned short& quickItemHeader : ac->PCStats->QuickItemHeaders) {
			stream->WriteWord(quickItemHeader);
		}
		break;
	default:
		for (int i = 0; i < 3; i++) {
			stream->WriteWord(ac->PCStats->QuickItemSlots[i]);
		}
		for (int i = 0; i < 3; i++) {
			stream->WriteWord(ac->PCStats->QuickItemHeaders[i]);
		}
		break;
	}

	//innates, bard songs and quick slots are saved only in iwd2
	if (GAMVersion == GAM_VER_IWD2 || GAMVersion == GAM_VER_GEMRB) {
		for (int i = 0; i < MAX_QSLOTS; i++) {
			if (ac->PCStats->QuickSpellBookType[i] == 0xff) {
				stream->WriteResRef(ac->PCStats->QuickSpells[i]);
			} else {
				stream->WriteFilling(8);
			}
		}
		for (int i = 0; i < MAX_QSLOTS; i++) {
			if (ac->PCStats->QuickSpellBookType[i] == 0xfe) {
				stream->WriteResRef(ac->PCStats->QuickSpells[i]);
			} else {
				stream->WriteFilling(8);
			}
		}
		for (int i = 0; i < MAX_QSLOTS; i++) {
			stream->WriteDword(ac->PCStats->QSlots[i+3]);
		}
	}

	if (ac->LongStrRef == ieStrRef::INVALID) {
		std::string tmpstr = TLKStringFromString(ac->GetLongName());
		stream->WriteVariable(ieVariable(tmpstr));
	} else {
		std::string tmpstr = core->GetMBString(ac->LongStrRef, STRING_FLAGS::STRREFOFF);
		stream->WriteVariable(ieVariable(tmpstr));
	}
	stream->WriteDword(ac->TalkCount);
	stream->WriteStrRef(ac->PCStats->BestKilledName);
	stream->WriteDword(ac->PCStats->BestKilledXP);
	stream->WriteDword(ac->PCStats->AwayTime);
	stream->WriteDword(ac->PCStats->JoinDate);
	stream->WriteDword(ac->PCStats->unknown10);
	stream->WriteDword(ac->PCStats->KillsChapterXP);
	stream->WriteDword(ac->PCStats->KillsChapterCount);
	stream->WriteDword(ac->PCStats->KillsTotalXP);
	stream->WriteDword(ac->PCStats->KillsTotalCount);
	for (const auto& favouriteSpell : ac->PCStats->FavouriteSpells) {
		stream->WriteResRefUC(favouriteSpell);
	}
	for (unsigned short& favCount : ac->PCStats->FavouriteSpellsCount) {
		stream->WriteWord(favCount);
	}
	for (const auto& favouriteWeapon : ac->PCStats->FavouriteWeapons) {
		stream->WriteResRefUC(favouriteWeapon);
	}
	for (unsigned short& favCount : ac->PCStats->FavouriteWeaponsCount) {
		stream->WriteWord(favCount);
	}
	stream->WriteResRefUC(ac->PCStats->SoundSet);
	if (core->HasFeature(GFFlags::SOUNDFOLDERS) ) {
		auto soundFolder = TLKStringFromString(ac->PCStats->SoundFolder);
		stream->WriteStringLC(soundFolder, ieVariable::Size);
	}
	if (GAMVersion == GAM_VER_IWD2 || GAMVersion == GAM_VER_GEMRB) {
		//I don't know how many fields are actually used in IWD2 saved game
		//but we got at least 8 (and only 5 of those are actually used)
		for (unsigned int& extraSetting : ac->PCStats->ExtraSettings) {
			stream->WriteDword(extraSetting);
		}
		stream->WriteFilling(130);
	}

	return 0;
}

int GAMImporter::PutPCs(DataStream *stream, const Game *game) const
{
	auto am = GetImporter<ActorMgr>(IE_CRE_CLASS_ID);
	ieDword CREOffset = PCOffset + PCCount * PCSize;

	for (unsigned int i = 0; i < PCCount; i++) {
		assert(stream->GetPos() == PCOffset + i * PCSize);
		const Actor *ac = game->GetPC(i, false);
		ieDword CRESize = am->GetStoredFileSize(ac);
		PutActor(stream, ac, CRESize, CREOffset, game->version);
		CREOffset += CRESize;
	}

	CREOffset = PCOffset + PCCount * PCSize; // just for the asserts..
	assert(stream->GetPos() == CREOffset);

	for (unsigned int i = 0; i < PCCount; i++) {
		assert(stream->GetPos() == CREOffset);
		const Actor *ac = game->GetPC(i, false);
		//reconstructing offsets again
		CREOffset += am->GetStoredFileSize(ac);
		am->PutActor( stream, ac);
	}
	assert(stream->GetPos() == CREOffset);
	return 0;
}

int GAMImporter::PutNPCs(DataStream *stream, const Game *game) const
{
	auto am = GetImporter<ActorMgr>(IE_CRE_CLASS_ID);
	ieDword CREOffset = NPCOffset + NPCCount * PCSize;

	for (unsigned int i = 0; i < NPCCount; i++) {
		assert(stream->GetPos() == NPCOffset + i * PCSize);
		const Actor *ac = game->GetNPC(i);
		ieDword CRESize = am->GetStoredFileSize(ac);
		PutActor(stream, ac, CRESize, CREOffset, game->version);
		CREOffset += CRESize;
	}
	CREOffset = NPCOffset + NPCCount * PCSize; // just for the asserts..
	assert(stream->GetPos() == CREOffset);

	for (unsigned int  i = 0; i < NPCCount; i++) {
		assert(stream->GetPos() == CREOffset);
		const Actor *ac = game->GetNPC(i);
		//reconstructing offsets again
		CREOffset += am->GetStoredFileSize(ac);
		am->PutActor( stream, ac);
	}
	assert(stream->GetPos() == CREOffset);
	return 0;
}

void GAMImporter::GetMazeHeader(void *memory) const
{
	maze_header *m = (maze_header *) memory;
	str->ReadDword(m->maze_sizex);
	str->ReadDword(m->maze_sizey);
	str->ReadDword(m->pos1x);
	str->ReadDword(m->pos1y);
	str->ReadDword(m->pos2x);
	str->ReadDword(m->pos2y);
	str->ReadDword(m->pos3x);
	str->ReadDword(m->pos3y);
	str->ReadDword(m->pos4x);
	str->ReadDword(m->pos4y);
	str->ReadDword(m->trapcount);
	str->ReadDword(m->initialized);
	str->ReadDword(m->unknown2c);
	str->ReadDword(m->unknown30);
}

void GAMImporter::GetMazeEntry(void *memory) const
{
	maze_entry *h = (maze_entry *) memory;

	str->ReadDword(h->me_override);
	str->ReadDword(h->valid);
	str->ReadDword(h->accessible);
	str->ReadDword(h->trapped);
	str->ReadDword(h->traptype);
	str->ReadWord(h->walls);
	str->ReadDword(h->visited);
}

void GAMImporter::PutMazeHeader(DataStream *stream, void *memory) const
{
	const maze_header *m = (maze_header *) memory;
	stream->WriteDword(m->maze_sizex);
	stream->WriteDword(m->maze_sizey);
	stream->WriteDword(m->pos1x);
	stream->WriteDword(m->pos1y);
	stream->WriteDword(m->pos2x);
	stream->WriteDword(m->pos2y);
	stream->WriteDword(m->pos3x);
	stream->WriteDword(m->pos3y);
	stream->WriteDword(m->pos4x);
	stream->WriteDword(m->pos4y);
	stream->WriteDword(m->trapcount);
	stream->WriteDword(m->initialized);
	stream->WriteDword(m->unknown2c);
	stream->WriteDword(m->unknown30);
}

void GAMImporter::PutMazeEntry(DataStream *stream, void *memory) const
{
	const maze_entry *h = (maze_entry *) memory;
	stream->WriteDword(h->me_override);
	stream->WriteDword(h->valid);
	stream->WriteDword(h->accessible);
	stream->WriteDword(h->trapped);
	stream->WriteDword(h->traptype);
	stream->WriteWord(h->walls);
	stream->WriteDword(h->visited);
}

int GAMImporter::PutMaze(DataStream *stream, const Game *game) const
{
	for(int i=0;i<MAZE_ENTRY_COUNT;i++) {
		PutMazeEntry(stream, game->mazedata+i*MAZE_ENTRY_SIZE);
	}
	PutMazeHeader(stream, game->mazedata+MAZE_ENTRY_COUNT*MAZE_ENTRY_SIZE);
	return 0;
}

int GAMImporter::PutFamiliars(DataStream *stream, const Game *game) const
{
	int len = 0;
	if (core->GetBeastsINI()) {
		len = BESTIARY_SIZE;
		if (game->version==GAM_VER_PST) {
			//only GemRB version can have all features, return when it is PST
			//gemrb version will have the beasts after the familiars
			stream->Write(game->beasts.data(), len);
			return 0;
		}
	}

	for (unsigned int i=0;i<9;i++) {
		stream->WriteResRef( game->GetFamiliar(i) );
	}
	stream->WriteDword(SavedLocOffset);
	if (len) {
		stream->Write(game->beasts.data(), len);
	}
	stream->WriteFilling(FAMILIAR_FILL_SIZE - len);
	return 0;
}

int GAMImporter::PutGame(DataStream *stream, Game *game) const
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

	if (core->HasFeature(GFFlags::HAS_KAPUTZ) ) {
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
	if (SavedLocOffset || game->version == GAM_VER_IWD2) {
		ret = PutSavedLocations(stream, game);
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

} // namespace GemRB

#include "plugindef.h"

GEMRB_PLUGIN(0xD7F7040, "GAM File Importer")
PLUGIN_CLASS(IE_GAM_CLASS_ID, ImporterPlugin<GAMImporter>)
END_PLUGIN()
