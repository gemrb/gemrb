/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2004 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

// This class represents the .gam (savegame) file in the engine

#include "Game.h"

#include "defsounds.h"
#include "strrefs.h"
#include "win32def.h"

#include "DisplayMessage.h"
#include "GameData.h"
#include "Interface.h"
#include "IniSpawn.h"
#include "MapMgr.h"
#include "MusicMgr.h"
#include "Particles.h"
#include "PluginMgr.h"
#include "ScriptEngine.h"
#include "TableMgr.h"
#include "GameScript/GameScript.h"
#include "GUI/GameControl.h"
#include "System/DataStream.h"
#include "System/StringBuffer.h"
#include "Video.h"
#include "MapReverb.h"

#include <algorithm>
#include <iterator>
#include <vector>

namespace GemRB {

struct HealingResource {
	ieResRef resref;
	Actor *caster;
	ieWord amounthealed;
	ieWord amount;
	HealingResource(ieResRef ref, Actor *cha, ieWord ah, ieWord a)
		: caster(cha), amounthealed(ah), amount(a) {
		CopyResRef(resref, ref);
	}
	HealingResource() {
		CopyResRef(resref, "");
		amount = 0;
		amounthealed = 0;
		caster = NULL;
	}
	bool operator < (const HealingResource &str) const {
		return (amounthealed < str.amounthealed);
	}
};

struct Injured {
	int hpneeded;
	Actor *character;
	Injured(int hps, Actor *cha)
		: hpneeded(hps), character(cha) {
		// already done
	}
	bool operator < (const Injured &str) const {
		return (hpneeded < str.hpneeded);
	}
};

#define MAX_MAPS_LOADED 1

Game::Game(void) : Scriptable( ST_GLOBAL )
{
	protagonist = PM_YES; //set it to 2 for iwd/iwd2 and 0 for pst
	partysize = 6;
	Ticks = 0;
	GameTime = RealTime = 0;
	version = 0;
	Expansion = 0;
	LoadMos[0] = 0;
	TextScreen[0] = 0;
	SelectedSingle = 1; //the PC we are looking at (inventory, shop)
	PartyGold = 0;
	SetScript( core->GlobalScript, 0 );
	MapIndex = -1;
	Reputation = 0;
	ControlStatus = 0;
	CombatCounter = 0; //stored here until we know better
	StateOverrideTime = 0;
	StateOverrideFlag = 0;
	BanterBlockTime = 0;
	BanterBlockFlag = 0;
	WeatherBits = 0;
	crtable = NULL;
	kaputz = NULL;
	beasts = NULL;
	mazedata = NULL;
	timestop_owner = NULL;
	timestop_end = 0;
	event_timer = 0;
	event_handler = NULL;
	weather = new Particles(200);
	weather->SetRegion(0, 0, core->Width, core->Height);
	LastScriptUpdate = 0;
	WhichFormation = 0;
	NpcInParty = 0;
	CurrentLink = 0;
	PartyAttack = false;

	//loading master areas
	AutoTable table;
	if (table.load("mastarea")) {
		int i = table->GetRowCount();
		mastarea.reserve(i);
		while(i--) {
			char *tmp = (char *) malloc(9);
			strnuprcpy(tmp, table->GetRowName(i), 8);
			mastarea.push_back( tmp );
		}
	}

	//loading rest/daylight switching movies (only bg2 has them)
	memset(restmovies,'*',sizeof(restmovies));
	memset(daymovies,'*',sizeof(daymovies));
	memset(nightmovies,'*',sizeof(nightmovies));
	if (table.load("restmov")) {
		for(int i=0;i<8;i++) {
			strnuprcpy(restmovies[i],table->QueryField(i,0),8);
			strnuprcpy(daymovies[i],table->QueryField(i,1),8);
			strnuprcpy(nightmovies[i],table->QueryField(i,2),8);
		}
	}

	//loading npc starting levels
	ieResRef tn;
	if (Expansion == 5) { // tob is special
		CopyResRef(tn, "npclvl25");
	} else {
		CopyResRef(tn, "npclevel");
	}
	if (table.load(tn)) {
		int cols = table->GetColumnCount();
		int rows = table->GetRowCount();
		int i, j;
		npclevels.reserve(rows);
		for (i = 0; i < rows; i++) {
			npclevels.push_back (std::vector<char *>(cols+1));
			for(j = -1; j < cols; j++) {
				char *ref = new char[9];
				if (j == -1) {
					CopyResRef(ref, table->GetRowName(i));
					npclevels[i][j+1] = ref;
				} else {
					CopyResRef(ref, table->QueryField(i, j));
					npclevels[i][j+1] = ref;
				}
			}
		}
	}

	interval = 1000/AI_UPDATE_TIME;
	hasInfra = false;
	familiarBlock = false;
	//FIXME:i'm not sure in this...
	NoInterrupt();
	bntchnc = NULL;
	bntrows = -1;
}

Game::~Game(void)
{
	delete weather;
	for (auto map : Maps) {
		delete map;
	}
	for (auto pc : PCs) {
		delete pc;
	}
	for (auto npc : NPCs) {
		delete npc;
	}
	for (auto ma : mastarea) {
		free(ma);
	}

	if (crtable) {
		delete[] crtable;
	}

	if (mazedata) {
		free (mazedata);
	}
	if (kaputz) {
		delete kaputz;
	}
	if (beasts) {
		free (beasts);
	}
	for (auto journal : Journals) {
		delete journal;
	}

	for (auto sp : savedpositions) {
		free(sp);
	}

	for (auto pp : planepositions) {
		free(pp);
	}

	for (auto nl : npclevels) {
		for (auto nll : nl) {
			delete [] nll;
		}
	}
}

static bool IsAlive(Actor *pc)
{
	if (pc->GetStat(IE_STATE_ID)&STATE_DEAD) {
		return false;
	}
	return true;
}

void Game::ReversePCs()
{
	for (auto pc : PCs) {
		pc->InParty = PCs.size() - pc->InParty + 1;
	}
	core->SetEventFlag(EF_PORTRAIT|EF_SELECTION);
}

int Game::FindPlayer(unsigned int partyID)
{
	for (unsigned int slot=0; slot<PCs.size(); slot++) {
		if (PCs[slot]->InParty==partyID) {
			return slot;
		}
	}
	return -1;
}

Actor* Game::FindPC(unsigned int partyID)
{
	for (auto pc : PCs) {
		if (pc->InParty == partyID) return pc;
	}
	return NULL;
}

Actor* Game::FindPC(const char *scriptingname)
{
	for (auto pc : PCs) {
		if (strnicmp(pc->GetScriptName(), scriptingname, 32) == 0) {
			return pc;
		}
	}
	return NULL;
}

Actor* Game::FindNPC(unsigned int partyID)
{
	for (auto npc : NPCs) {
		if (npc->InParty == partyID) return npc;
	}
	return NULL;
}

Actor* Game::FindNPC(const char *scriptingname)
{
	for (auto npc : NPCs) {
		if (strnicmp(npc->GetScriptName(), scriptingname, 32) == 0) {
			return npc;
		}
	}
	return NULL;
}

Actor *Game::GetGlobalActorByGlobalID(ieDword globalID)
{
	for (auto pc : PCs) {
		if (pc->GetGlobalID() == globalID) {
			return pc;
		}
	}
	for (auto npc : NPCs) {
		if (npc->GetGlobalID() == globalID) {
			return npc;
		}
	}
	return NULL;
}

Actor* Game::GetPC(unsigned int slot, bool onlyalive) const
{
	if (slot >= PCs.size()) {
		return NULL;
	}
	if (onlyalive) {
		for (auto pc : PCs) {
			if (IsAlive(pc) && !slot--) {
				return pc;
			}
		}
		return NULL;
	}
	return PCs[slot];
}

int Game::InStore(Actor* pc) const
{
	for (unsigned int i = 0; i < NPCs.size(); i++) {
		if (NPCs[i] == pc) {
			return i;
		}
	}
	return -1;
}

int Game::InParty(Actor* pc) const
{
	for (unsigned int i = 0; i < PCs.size(); i++) {
		if (PCs[i] == pc) {
			return i;
		}
	}
	return -1;
}

int Game::DelPC(unsigned int slot, bool autoFree)
{
	if (slot >= PCs.size()) {
		return -1;
	}
	if (!PCs[slot]) {
		return -1;
	}
	SelectActor(PCs[slot], false, SELECT_NORMAL);
	if (autoFree) {
		delete( PCs[slot] );
	}
	std::vector< Actor*>::iterator m = PCs.begin() + slot;
	PCs.erase( m );
	return 0;
}

int Game::DelNPC(unsigned int slot, bool autoFree)
{
	if (slot >= NPCs.size()) {
		return -1;
	}
	if (!NPCs[slot]) {
		return -1;
	}
	if (autoFree) {
		delete( NPCs[slot] );
	}
	std::vector< Actor*>::iterator m = NPCs.begin() + slot;
	NPCs.erase( m );
	return 0;
}

//i'm sure this could be faster
void Game::ConsolidateParty()
{
	int max = (int) PCs.size();
	for (int i=1;i<=max;) {
		if (FindPlayer(i)==-1) {
			for (auto pc : PCs) {
				if (pc->InParty > i) {
					pc->InParty--;
				}
			}
		} else i++;
	}
	for (auto m = PCs.begin(); m != PCs.end(); ++m) {
		(*m)->RefreshEffects(NULL);
		//TODO: how to set up bardsongs
		(*m)->SetModalSpell((*m)->Modal.State, 0);
	}
}

int Game::LeaveParty (Actor* actor)
{
	core->SetEventFlag(EF_PORTRAIT);
	actor->CreateStats(); //create or update stats for leaving
	actor->SetBase(IE_EXPLORE, 0);

	SelectActor(actor, false, SELECT_NORMAL);
	int slot = InParty( actor );
	if (slot < 0) {
		return slot;
	}
	std::vector< Actor*>::iterator m = PCs.begin() + slot;
	PCs.erase( m );

	ieDword id = actor->GetGlobalID();
	for (auto pc : PCs) {
		pc->PCStats->LastLeft = id;
		if (pc->InParty>actor->InParty) {
			pc->InParty--;
		}
	}
	//removing from party, but actor remains in 'game'
	actor->SetPersistent(0);
	NPCs.push_back( actor );

	if (core->HasFeature( GF_HAS_DPLAYER )) {
		// we must reset various existing scripts
		actor->SetScript("", SCR_DEFAULT );
		actor->SetScript("", SCR_CLASS, false);
		actor->SetScript("", SCR_RACE, false);
		actor->SetScript("WTASIGHT", SCR_GENERAL, false);
		if (actor->GetBase(IE_MC_FLAGS) & MC_EXPORTABLE) {
			actor->SetDialog("MULTIJ");
		}
	}
	actor->SetBase( IE_EA, EA_NEUTRAL );
	return ( int ) NPCs.size() - 1;
}

//determines if startpos.2da has rotation rows (it cannot have tutorial line)
bool Game::DetermineStartPosType(const TableMgr *strta)
{
	if ((strta->GetRowCount()>=6) && !stricmp(strta->GetRowName(4),"START_ROT" ) )
	{
		return true;
	}
	return false;
}

#define PMODE_COUNT 3

void Game::InitActorPos(Actor *actor)
{
	//start.2da row labels
	const char *mode[PMODE_COUNT] = { "NORMAL", "TUTORIAL", "EXPANSION" };

	unsigned int ip = (unsigned int) (actor->InParty-1);
	AutoTable start("start");
	AutoTable strta("startpos");

	if (!start || !strta) {
		error("Game", "Game is missing character start data.\n");
	}
	// 0 - single player, 1 - tutorial, 2 - expansion
	ieDword playmode = 0;
	core->GetDictionary()->Lookup( "PlayMode", playmode );

	//Sometimes playmode is set to -1 (in pregenerate)
	//normally execution shouldn't ever come here, but it actually does
	//preventing problems by defaulting to the regular entry points
	if (playmode>PMODE_COUNT) {
		playmode = 0;
	}
	const char *xpos = start->QueryField(mode[playmode],"XPOS");
	const char *ypos = start->QueryField(mode[playmode],"YPOS");
	const char *area = start->QueryField(mode[playmode],"AREA");
	const char *rot = start->QueryField(mode[playmode],"ROT");

	actor->Pos.x = actor->Destination.x = (short) atoi( strta->QueryField( strta->GetRowIndex(xpos), ip ) );
	actor->Pos.y = actor->Destination.y = (short) atoi( strta->QueryField( strta->GetRowIndex(ypos), ip ) );
	actor->HomeLocation.x = actor->Pos.x;
	actor->HomeLocation.y = actor->Pos.y;
	actor->SetOrientation( atoi( strta->QueryField( strta->GetRowIndex(rot), ip) ), false );

	if (strta.load("startare")) {
		strnlwrcpy(actor->Area, strta->QueryField( strta->GetRowIndex(area), 0 ), 8 );
	} else {
		strnlwrcpy(actor->Area, CurrentArea, 8 );
	}
}

int Game::JoinParty(Actor* actor, int join)
{
	core->SetEventFlag(EF_PORTRAIT);
	actor->CreateStats(); //create stats if they didn't exist yet
	actor->InitButtons(actor->GetActiveClass(), false); // init actor's action bar
	actor->SetBase(IE_EXPLORE, 1);
	if (join&JP_INITPOS) {
		InitActorPos(actor);
	}
	int slot = InParty( actor );
	if (slot != -1) {
		return slot;
	}
	size_t size = PCs.size();

	if (join&JP_JOIN) {
		//update kit abilities of actor
		ieDword baseclass = 0;
		if (core->HasFeature(GF_LEVELSLOT_PER_CLASS)) {
			// get the class for iwd2; luckily there are no NPCs, everyone joins at level 1, so multi-kit annoyances can be ignored
			baseclass = actor->GetBase(IE_CLASS);
		}
		actor->ApplyKit(false, baseclass);
		//update the quickslots
		actor->ReinitQuickSlots();
		//set the joining date
		actor->PCStats->JoinDate = GameTime;
		//if the protagonist has the same portrait replace it
		Actor *prot = GetPC(0, false);
		if (prot && (!strcmp(actor->SmallPortrait, prot->SmallPortrait) || !strcmp(actor->LargePortrait, prot->LargePortrait))) {
			AutoTable ptab("portrait");
			if (ptab) {
				CopyResRef(actor->SmallPortrait, ptab->QueryField(actor->SmallPortrait, "REPLACEMENT"));
				CopyResRef(actor->LargePortrait, ptab->QueryField(actor->LargePortrait, "REPLACEMENT"));
			}
		}

		//set the lastjoined trigger
		if (size) {
			ieDword id = actor->GetGlobalID();
			for (size_t i=0;i<size; i++) {
				Actor *a = GetPC(i, false);
				a->PCStats->LastJoined = id;
			}
		} else {
			Reputation = actor->GetStat(IE_REPUTATION);
		}
	}
	slot = InStore( actor );
	if (slot >= 0) {
		std::vector< Actor*>::iterator m = NPCs.begin() + slot;
		NPCs.erase( m );
	}

	PCs.push_back( actor );
	if (!actor->InParty) {
		actor->InParty = (ieByte) (size+1);
	}

	if (join&(JP_INITPOS|JP_SELECT)) {
		actor->Selected = 0; // don't confuse SelectActor!
		SelectActor(actor,true, SELECT_NORMAL);
	}

	return ( int ) size;
}

int Game::GetPartySize(bool onlyalive) const
{
	if (onlyalive) {
		int count = 0;
		for (auto pc : PCs) {
			if (!IsAlive(pc)) {
				continue;
			}
			count++;
		}
		return count;
	}
	return (int) PCs.size();
}

/* sends the hotkey trigger to all selected actors */
void Game::SetHotKey(unsigned long Key)
{
	for (auto actor : selected) {
		if (actor->IsSelected()) {
			actor->AddTrigger(TriggerEntry(trigger_hotkey, (ieDword) Key));
		}
	}
}

bool Game::SelectPCSingle(int index)
{
	Actor* actor = FindPC( index );
	if (!actor)
		return false;

	SelectedSingle = index;
	core->SetEventFlag(EF_SELECTION);

	return true;
}

int Game::GetSelectedPCSingle() const
{
	return SelectedSingle;
}

Actor* Game::GetSelectedPCSingle(bool onlyalive)
{
	Actor *pc = FindPC(SelectedSingle);
	if (!pc) return NULL;

	if (onlyalive && !IsAlive(pc)) {
		return NULL;
	}
	return pc;
}

/*
 * SelectActor() - handle (de)selecting actors.
 * If selection was changed, runs "SelectionChanged" handler
 *
 * actor - either specific actor, or NULL for all
 * select - whether actor(s) should be selected or deselected
 * flags:
 * SELECT_REPLACE - if true, deselect all other actors when selecting one
 * SELECT_QUIET   - do not run handler if selection was changed. Used for
 * nested calls to SelectActor()
 */

bool Game::SelectActor(Actor* actor, bool select, unsigned flags)
{
	std::vector< Actor*>::iterator m;

	// actor was not specified, which means all selectables should be (de)selected
	if (! actor) {
		for (auto selectee : selected) {
			selectee->Select(false);
			selectee->SetOver(false);
		}
		selected.clear();

		if (select) {
			area->SelectActors();
/*
			for ( m = PCs.begin(); m != PCs.end(); ++m) {
				if (! *m) {
					continue;
				}
				SelectActor( *m, true, SELECT_QUIET );
			}
*/
		}

		if (! (flags & SELECT_QUIET)) {
			core->SetEventFlag(EF_SELECTION);
		}
		Infravision();
		return true;
	}

	// actor was specified, so we will work with him
	if (select) {
		if (! actor->ValidTarget( GA_SELECT | GA_NO_DEAD ))
			return false;

		// deselect all actors first when exclusive
		if (flags & SELECT_REPLACE) {
			if (selected.size() == 1 && actor->IsSelected()) {
				assert(selected[0] == actor);
				// already the only selected actor
				return true;
			}
			SelectActor( NULL, false, SELECT_QUIET );
		} else if (actor->IsSelected()) {
			// already selected
			return true;
		}

		actor->Select( true );
		assert(actor->IsSelected());
		selected.push_back( actor );
	} else {
		if (!actor->IsSelected()) {
			// already not selected
			return true;

			/*for ( m = selected.begin(); m != selected.end(); ++m) {
				assert((*m) != actor);
			}
			return true;*/
		}
		for ( m = selected.begin(); m != selected.end(); ++m) {
			if ((*m) == actor) {
				selected.erase( m );
				break;
			}
		}
		actor->Select( false );
		assert(!actor->IsSelected());
	}

	if (! (flags & SELECT_QUIET)) {
		core->SetEventFlag(EF_SELECTION);
	}
	Infravision();
	return true;
}

// Gets sum of party level, if onlyalive is true, then counts only living PCs
// If you need average party level, divide this with GetPartySize
int Game::GetTotalPartyLevel(bool onlyalive) const
{
	int amount = 0;

	for (auto pc : PCs) {
			if (onlyalive) {
				if (pc->GetStat(IE_STATE_ID) & STATE_DEAD) {
					continue;
				}
			}
			amount += pc->GetXPLevel(0);
	}

	return amount;
}

// Returns map structure (ARE) if it is already loaded in memory
int Game::FindMap(const char *ResRef)
{
	int index = (int) Maps.size();
	while (index--) {
		Map *map=Maps[index];
		if (strnicmp(ResRef, map->GetScriptName(), 8) == 0) {
			return index;
		}
	}
	return -1;
}

Map* Game::GetMap(unsigned int index) const
{
	if (index >= Maps.size()) {
		return NULL;
	}
	return Maps[index];
}

Map *Game::GetMap(const char *areaname, bool change)
{
	int index = LoadMap(areaname, change);
	if (index >= 0) {
		if (change) {
			MapIndex = index;
			area = GetMap(index);
			memcpy (CurrentArea, areaname, 8);
			//change the tileset if needed
			area->ChangeMap(IsDay());
			area->SetupAmbients();
			ChangeSong(false, true);
			Infravision();

			//call area customization script for PST
			//moved here because the current area is set here
			ScriptEngine *sE = core->GetGUIScriptEngine();
			if (core->HasFeature(GF_AREA_OVERRIDE) && sE) {
				//area ResRef is accessible by GemRB.GetGameString (STR_AREANAME)
				sE->RunFunction("Maze", "CustomizeArea");
			}

			return area;
		}
		return GetMap(index);
	}
	return NULL;
}

bool Game::MasterArea(const char *area)
{
	for (auto ma : mastarea) {
		if (!strnicmp(ma, area, 8)) {
			return true;
		}
	}
	return false;
}

// guess the master area by comparing the area name to entries in mastarea.2da
// returns the area numerically closest to the passed one
// pst has also areas named arNNNNc, bgt araNNNN (which we mishandle)
// TODO: consider caching with an internal field or even a separate table to map the relation
/*
Map* Game::GetMasterArea(const char *area)
{
	unsigned int areaNum;
	unsigned int masterNum;
	unsigned int prevDiff = 0;
	ieResRef prevArea;
	sscanf(area, "%*c%*c%u%*c", &areaNum);

	// mastarea.2da is not sorted, so make sure to check all the rows/areas
	unsigned int i=(int) mastarea.size();
	while(i--) {
		sscanf(mastarea[i], "%*c%*c%u%*c", &masterNum);
		if (areaNum > masterNum) {
			continue;
		} else if (areaNum == masterNum) {
			return NULL; // optimisation, should never be called with a masterarea already
		}
		if (prevDiff == 0 || (prevDiff > masterNum - areaNum && masterNum < areaNum)) {
			// first master bigger than us or
			// this area is numerically closer than the last choice, but still smaller
			CopyResRef(prevArea, mastarea[i+1]);
			prevDiff = masterNum - areaNum;
		}
	}
	// this could be slow, loading a full map!
	// luckily when queried from subareas, it should already be cached and fast
	return GetMap(prevArea, false);
}*/

void Game::SetMasterArea(const char *area)
{
	if (MasterArea(area) ) return;
	char *tmp = (char *) malloc(9);
	strnlwrcpy (tmp,area,8);
	mastarea.push_back(tmp);
}

int Game::AddMap(Map* map)
{
	if (MasterArea(map->GetScriptName()) ) {
		Maps.insert(Maps.begin(), 1, map);
		MapIndex++;
		return 0;
	}
	unsigned int i = (unsigned int) Maps.size();
	Maps.push_back( map );
	return i;
}

// this function should archive the area, and remove it only if the area
// contains no active actors (combat, partymembers, etc)
int Game::DelMap(unsigned int index, int forced)
{
	if (index >= Maps.size()) {
		return -1;
	}
	Map *map = Maps[index];

	if (MapIndex == (int) index) { //can't remove current map in any case
		const char *name = map->GetScriptName();
		memcpy(AnotherArea, name, sizeof(AnotherArea));
		return -1;
	}

	if (!map) { //this shouldn't happen, i guess
		Log(WARNING, "Game", "Erased NULL Map");
		Maps.erase(Maps.begin() + index);
		if (MapIndex > (int) index) {
			MapIndex--;
		}
		return 1;
	}

	if (forced || Maps.size() > MAX_MAPS_LOADED) {
		//keep at least one master
		const char *name = map->GetScriptName();
		if (MasterArea(name) && !AnotherArea[0]) {
			memcpy(AnotherArea, name, sizeof(AnotherArea));
			if (!forced) {
				return -1;
			}
		}
		//this check must be the last, because
		//after PurgeActors you cannot keep the
		//area in memory
		//Or the queues should be regenerated!
		if (!map->CanFree()) {
			return 1;
		}
		//if there are still selected actors on the map (e.g. summons)
		//unselect them now before they get axed
		std::vector< Actor*>::iterator m;
		for (m = selected.begin(); m != selected.end();) {
			if (!(*m)->InParty && !stricmp(Maps[index]->GetScriptName(), (*m)->Area)) {
				m = selected.erase(m);
			} else {
				++m;
			}
		}

		//remove map from memory
		core->SwapoutArea(Maps[index]);
		delete(Maps[index]);
		Maps.erase(Maps.begin() + index);
		//current map will be decreased
		if (MapIndex > (int) index) {
			MapIndex--;
		}
		return 1;
	}
	//didn't remove the map
	return 0;
}

void Game::PlacePersistents(Map *newMap, const char *ResRef)
{
	// count the number of replaced actors, so we don't need to recheck them
	// if their max level is still lower than ours, each check would also result in a substitution
	unsigned int last = NPCs.size() - 1;
	for (unsigned int i = 0; i < NPCs.size(); i++) {
		if (stricmp( NPCs[i]->Area, ResRef ) == 0) {
			if (i <= last && CheckForReplacementActor(i)) {
				i--;
				last--;
				continue;
			}
			newMap->AddActor( NPCs[i], false );
			NPCs[i]->SetMap(newMap);
		}
	}
}

/* Loads an area */
int Game::LoadMap(const char* ResRef, bool loadscreen)
{
	unsigned int ret;
	Map *newMap;
	PluginHolder<MapMgr> mM(IE_ARE_CLASS_ID);
	ScriptEngine *sE = core->GetGUIScriptEngine();

	//this shouldn't happen
	if (!mM) {
		return -1;
	}

	int index = FindMap(ResRef);
	if (index>=0) {
		return index;
	}

	bool hide = false;
	if (loadscreen && sE) {
		hide = core->HideGCWindow();
		sE->RunFunction("LoadScreen", "StartLoadScreen");
		sE->RunFunction("LoadScreen", "SetLoadScreen");
	}
	DataStream* ds = gamedata->GetResource( ResRef, IE_ARE_CLASS_ID );
	if (!ds) {
		goto failedload;
	}
	if(!mM->Open(ds)) {
		goto failedload;
	}
	newMap = mM->GetMap(ResRef, IsDay());
	if (!newMap) {
		goto failedload;
	}

	core->LoadProgress(100);

	ret = AddMap( newMap );

	//spawn creatures on a map already in the game
	//this feature exists in all blackisle games but not in bioware games
	if (core->HasFeature(GF_SPAWN_INI)) {
		newMap->LoadIniSpawn();
	}

	for (size_t i = 0; i < PCs.size(); i++) {
		Actor *pc = PCs[i];
		if (stricmp(pc->Area, ResRef) == 0) {
			newMap->AddActor(pc, false);
		}
	}

	PlacePersistents(newMap, ResRef);

	if (hide) {
		core->UnhideGCWindow();
	}
	newMap->InitActors();

	if (newMap->reverb) {
		core->GetAudioDrv()->UpdateMapAmbient(*newMap->reverb);
	}

	return ret;
failedload:
	if (hide) {
		core->UnhideGCWindow();
	}
	core->LoadProgress(100);
	return -1;
}

// check if the actor is in npclevel.2da and replace accordingly
bool Game::CheckForReplacementActor(int i)
{
	if (core->InCutSceneMode() || npclevels.empty()) {
		return false;
	}

	Actor* act = NPCs[i];
	ieDword level = GetTotalPartyLevel(false) / GetPartySize(false);
	if (!(act->Modified[IE_MC_FLAGS]&MC_BEENINPARTY) && !(act->Modified[IE_STATE_ID]&STATE_DEAD) && act->GetXPLevel(false) < level) {
		ieResRef newcre = "****"; // default table value
		for (auto nl : npclevels) {
			if (!stricmp(nl[0], act->GetScriptName()) && (level > 2)) {
				// the tables have entries only up to level 24
				ieDword safeLevel = npclevels[0].size();
				if (level < safeLevel) {
					safeLevel = level;
				}
				CopyResRef(newcre, nl[safeLevel-2]);
				break;
			}
		}

		if (stricmp(newcre, "****")) {
			int pos = gamedata->LoadCreature(newcre, 0, false, act->version);
			if (pos < 0) {
				error("Game::CheckForReplacementActor", "LoadCreature failed: pos is negative!\n");
			} else {
				Actor *newact = GetNPC(pos);
				if (!newact) {
					error("Game::CheckForReplacementActor", "GetNPC failed: cannot find act!\n");
				} else {
					newact->Pos = act->Pos; // the map is not loaded yet, so no SetPosition
					newact->TalkCount = act->TalkCount;
					newact->InteractCount = act->InteractCount;
					CopyResRef(newact->Area, act->Area);
					DelNPC(InStore(act), true);
					return true;
				}
			}
		}
	}
	return false;
}

int Game::AddNPC(Actor* npc)
{
	int slot = InStore( npc ); //already an npc
	if (slot != -1) {
		return slot;
	}
	slot = InParty( npc );
	if (slot != -1) {
		return -1;
	} //can't add as npc already in party
	npc->SetPersistent(0);
	NPCs.push_back( npc );

	if (npc->Selected) {
		npc->Selected = 0; // don't confuse SelectActor!
		SelectActor(npc, true, SELECT_NORMAL);
	}

	return (int) NPCs.size() - 1;
}

Actor* Game::GetNPC(unsigned int Index)
{
	if (Index >= NPCs.size()) {
		return NULL;
	}
	return NPCs[Index];
}

void Game::SwapPCs(unsigned int Index1, unsigned int Index2)
{
	if (Index1 >= PCs.size()) {
		return;
	}

	if (Index2 >= PCs.size()) {
		return;
	}
	int tmp = PCs[Index1]->InParty;
	PCs[Index1]->InParty = PCs[Index2]->InParty;
	PCs[Index2]->InParty = tmp;
	//signal a change of the portrait window
	core->SetEventFlag(EF_PORTRAIT | EF_SELECTION);
}

void Game::DeleteJournalEntry(ieStrRef strref)
{
	size_t i=Journals.size();
	while(i--) {
		if ((Journals[i]->Text==strref) || (strref==(ieStrRef) -1) ) {
			delete Journals[i];
			Journals.erase(Journals.begin()+i);
		}
	}
}

void Game::DeleteJournalGroup(int Group)
{
	size_t i=Journals.size();
	while(i--) {
		if (Journals[i]->Group==(ieByte) Group) {
			delete Journals[i];
			Journals.erase(Journals.begin()+i);
		}
	}
}
/* returns true if it modified or added a journal entry */
bool Game::AddJournalEntry(ieStrRef strref, int Section, int Group)
{
	GAMJournalEntry *je = FindJournalEntry(strref);
	if (je) {
		//don't set this entry again in the same section
		if (je->Section==Section) {
			return false;
		}
		if ((Section == IE_GAM_QUEST_DONE) && Group) {
			//removing all of this group and adding a new entry
			DeleteJournalGroup(Group);
		} else {
			//modifying existing entry
			je->Section = (ieByte) Section;
			je->Group = (ieByte) Group;
			ieDword chapter = 0;
			if (!core->HasFeature(GF_NO_NEW_VARIABLES)) {
				locals->Lookup("CHAPTER", chapter);
			}
			je->Chapter = (ieByte) chapter;
			je->GameTime = GameTime;
			return true;
		}
	}
	je = new GAMJournalEntry;
	je->GameTime = GameTime;
	ieDword chapter = 0;
	if (!core->HasFeature(GF_NO_NEW_VARIABLES)) {
		locals->Lookup("CHAPTER", chapter);
	}
	je->Chapter = (ieByte) chapter;
	je->unknown09 = 0;
	je->Section = (ieByte) Section;
	je->Group = (ieByte) Group;
	je->Text = strref;

	Journals.push_back( je );
	return true;
}

void Game::AddJournalEntry(GAMJournalEntry* entry)
{
	Journals.push_back( entry );
}

unsigned int Game::GetJournalCount() const
{
	return (unsigned int) Journals.size();
}

GAMJournalEntry* Game::FindJournalEntry(ieStrRef strref)
{
	for (auto entry : Journals) {
		if (entry->Text == strref) {
			return entry;
		}
	}

	return NULL;
}

GAMJournalEntry* Game::GetJournalEntry(unsigned int Index)
{
	if (Index >= Journals.size()) {
		return NULL;
	}
	return Journals[Index];
}

unsigned int Game::GetSavedLocationCount() const
{
	return (unsigned int) savedpositions.size();
}

void Game::ClearSavedLocations()
{
	for (auto sp : savedpositions) {
		delete sp;
	}
	savedpositions.clear();
}

GAMLocationEntry* Game::GetSavedLocationEntry(unsigned int i)
{
	size_t current = savedpositions.size();
	if (i>=current) {
		if (i>PCs.size()) {
			return NULL;
		}
		savedpositions.resize(i+1);
		while(current<=i) {
			savedpositions[current++]=(GAMLocationEntry *) calloc(1, sizeof(GAMLocationEntry) );
		}
	}
	return savedpositions[i];
}

unsigned int Game::GetPlaneLocationCount() const
{
	return (unsigned int) planepositions.size();
}

void Game::ClearPlaneLocations()
{
	for (auto pp : planepositions) {
		delete pp;
	}
	planepositions.clear();
}

GAMLocationEntry* Game::GetPlaneLocationEntry(unsigned int i)
{
	size_t current = planepositions.size();
	if (i>=current) {
		if (i>PCs.size()) {
			return NULL;
		}
		planepositions.resize(i+1);
		while(current<=i) {
			planepositions[current++]=(GAMLocationEntry *) calloc(1, sizeof(GAMLocationEntry) );
		}
	}
	return planepositions[i];
}

char *Game::GetFamiliar(unsigned int Index)
{
	return Familiars[Index];
}

//reading the challenge rating table for iwd2 (only when needed)
void Game::LoadCRTable()
{
	AutoTable table("moncrate");
	if (table.ok()) {
		int maxrow = table->GetRowCount()-1;
		crtable = new CRRow[MAX_LEVEL];
		for(int i=0;i<MAX_LEVEL;i++) {
			//row shouldn't be larger than maxrow
			int row = i<maxrow?i:maxrow;
			int maxcol = table->GetColumnCount(row)-1;
			for(int j=0;j<MAX_CRLEVEL;j++) {
				//col shouldn't be larger than maxcol
				int col = j<maxcol?j:maxcol;
				crtable[i][j]=atoi(table->QueryField(row,col) );
			}
		}
	}
}

// FIXME: figure out the real mechanism
int Game::GetXPFromCR(int cr)
{
	if (!crtable) LoadCRTable();
	if (!crtable) {
		Log(ERROR, "Game", "Cannot find moncrate.2da!");
		return 0;
	}

	int size = GetPartySize(true);
	if (!size) return 0; // everyone just died anyway
	// NOTE: this is an average of averages; if it turns out to be wrong,
	// compute the party average directly
	int level = GetTotalPartyLevel(true) / size;
	if (cr >= MAX_CRLEVEL) {
		cr = MAX_CRLEVEL;
	} else if (cr-1 < 0) {
		cr = 1;
	}
	Log(MESSAGE, "Game", "Challenge Rating: %d, party level: %d", cr, level);
	// it also has a column for cr 0.25 and 0.5, so let's treat cr as a 1-based index
	// but testing shows something else affects it further, so we divide by 2 to match
	// the net is full of claims of halved values, so perhaps just a quick final rebalancing tweak
	return crtable[level-1][cr-1]/2;
}

void Game::ShareXP(int xp, int flags)
{
	int individual;

	if (flags&SX_CR) {
		xp = GetXPFromCR(xp);
	}

	if (flags&SX_DIVIDE) {
		int PartySize = GetPartySize(true); //party size, only alive
		if (PartySize<1) {
			return;
		}
		individual = xp / PartySize;
	} else {
		individual = xp;
	}

	if (!individual) {
		return;
	}

	//you have gained/lost ... xp
	if (core->HasFeedback(FT_MISC)) {
		if (xp > 0) {
			displaymsg->DisplayConstantStringValue(STR_GOTXP, DMC_BG2XPGREEN, (ieDword) xp);
		} else {
			displaymsg->DisplayConstantStringValue(STR_LOSTXP, DMC_BG2XPGREEN, (ieDword) -xp);
		}
	}
	for (auto pc : PCs) {
		if (pc->GetStat(IE_STATE_ID) & STATE_DEAD) {
			continue;
		}
		pc->AddExperience(individual, flags & SX_COMBAT);
	}
}

bool Game::EveryoneStopped() const
{
	for (auto pc : PCs) {
		if (pc->InMove()) return false;
	}
	return true;
}

//canmove=true: if some PC can't move (or hostile), then this returns false
bool Game::EveryoneNearPoint(Map *area, const Point &p, int flags) const
{
	for (auto pc : PCs) {
		if (flags&ENP_ONLYSELECT) {
			if(!pc->Selected) {
				continue;
			}
		}
		if (pc->GetStat(IE_STATE_ID) & STATE_DEAD) {
			continue;
		}
		if (flags&ENP_CANMOVE) {
			//someone is uncontrollable, can't move
			if (pc->GetStat(IE_EA) > EA_GOODCUTOFF) {
				return false;
			}

			if (pc->GetStat(IE_STATE_ID) & STATE_CANTMOVE) {
				return false;
			}
		}
		if (pc->GetCurrentArea() != area) {
			return false;
		}
		if (Distance(p, pc) > MAX_TRAVELING_DISTANCE) {
			Log(MESSAGE, "Game", "Actor %s is not near!", pc->LongName);
			return false;
		}
	}
	return true;
}

//called when someone died
void Game::PartyMemberDied(Actor *actor)
{
	//this could be null, in some extreme cases...
	Map *area = actor->GetCurrentArea();

	unsigned int size = PCs.size();
	Actor *react = NULL;
	for (unsigned int i = core->Roll(1, size, 0), n = 0; n < size; i++, n++) {
		Actor *pc = PCs[i%size];
		if (pc == actor) {
			continue;
		}
		if (pc->GetStat(IE_STATE_ID)&STATE_DEAD) {
			continue;
		}
		if (pc->GetStat(IE_MC_FLAGS) & MC_EXPORTABLE) {
			continue;
		}
		if (pc->GetCurrentArea()!=area) {
			continue;
		}
		if (pc->HasSpecialDeathReaction(actor->GetScriptName())) {
			react = pc;
			break;
		} else if (react == NULL) {
			react = pc;
		}
	}
	if (react != NULL) {
		react->ReactToDeath(actor->GetScriptName());
	}
}

void Game::IncrementChapter()
{
	//chapter first set to 0 (prologue)
	ieDword chapter = (ieDword) -1;
	locals->Lookup("CHAPTER",chapter);
	//increment chapter only if it exists
	locals->SetAt("CHAPTER", chapter+1, core->HasFeature(GF_NO_NEW_VARIABLES) );
	//clear statistics
	for (auto pc : PCs) {
		//all PCs must have this!
		pc->PCStats->IncrementChapter();
	}
}

void Game::SetReputation(ieDword r)
{
	if (r<10) r=10;
	else if (r>200) r=200;
	if (Reputation>r) {
		if (core->HasFeedback(FT_MISC)) displaymsg->DisplayConstantStringValue(STR_LOSTREP, DMC_GOLD, (Reputation-r)/10);
	} else if (Reputation<r) {
		if (core->HasFeedback(FT_MISC)) displaymsg->DisplayConstantStringValue(STR_GOTREP, DMC_GOLD, (r-Reputation)/10);
	}
	Reputation = r;
	for (auto pc : PCs) {
		pc->SetBase(IE_REPUTATION, Reputation);
	}
}

void Game::SetControlStatus(unsigned int value, int mode)
{
	core->SetBits(ControlStatus, value, mode);
	core->SetEventFlag(EF_CONTROL);
}

void Game::AddGold(ieDword add)
{
	if (!add) {
		return;
	}
	ieDword old = PartyGold;
	if (signed(PartyGold + add) < 0) {
		PartyGold = 0;
	} else {
		PartyGold += add;
	}
	if (old<PartyGold) {
		displaymsg->DisplayConstantStringValue( STR_GOTGOLD, DMC_GOLD, PartyGold-old);
	} else {
		displaymsg->DisplayConstantStringValue( STR_LOSTGOLD, DMC_GOLD, old-PartyGold);
	}
}

EffectRef fx_set_regenerating_state_ref = { "State:Regenerating", -1 };

//later this could be more complicated
void Game::AdvanceTime(ieDword add, bool fatigue)
{
	ieDword h = GameTime/core->Time.hour_size;
	GameTime+=add;
	if (h!=GameTime/core->Time.hour_size) {
		//asking for a new weather when the hour changes
		WeatherBits&=~WB_HASWEATHER;
		//update clock display
		core->GetGUIScriptEngine()->RunFunction("GUICommonWindows", "UpdateClock");
	}

	// emulate speeding through effects than need more than just an expiry check (eg. regeneration)
	// and delay most idle actions
	// but only if we skip for at least an hour
	if (add >= core->Time.hour_size) {
		for (auto pc : PCs) {
			pc->ResetCommentTime();
			int conHealRate = pc->GetConHealAmount();;
			// 1. regeneration as an effect
			// No matter the mode, if it is persistent, the actor will get fully healed in an hour.
			// However the effect does its own timekeeping, so we can't easily check the duration,
			// so we treat all regeneration as permanent - the most common kind (eg. from rings)
			if (pc->fxqueue.HasEffect(fx_set_regenerating_state_ref)) {
				pc->Heal(0);
			} else if (conHealRate) {
				// 2. regeneration from high constitution / TNO
				// some of the speeds are very slow, so calculate the accurate amount
				pc->Heal(add / conHealRate);
			}
		}
	}

	Ticks+=add*interval;
	if (!fatigue) {
		// update everyone in party, so they think no time has passed
		// nobody else, including familiars, gets luck penalties from fatigue
		for (auto pc : PCs) {
			pc->IncreaseLastRested(add);
		}
	}

	//change the tileset if needed
	Map *map = GetCurrentArea();
	if (map && map->ChangeMap(IsDay())) {
		//play the daylight transition movie appropriate for the area
		//it is needed to play only when the area truly changed its tileset
		//this is signalled by ChangeMap
		// ... but don't do it for a scripted DayNight change
		if (!fatigue) return;
		int areatype = (area->AreaType&(AT_FOREST|AT_CITY|AT_DUNGEON))>>3;
		ieResRef *res;

		if (IsDay()) {
			res=&nightmovies[areatype];
		} else {
			res=&daymovies[areatype];
		}
		if (*res[0]!='*') {
			core->PlayMovie(*res);
		}
	}
}

//returns true if there are excess players in the team
bool Game::PartyOverflow() const
{
	GameControl *gc = core->GetGameControl();
	if (!gc) {
		return false;
	}
	//don't start this screen when the gui is busy
	if (gc->GetDialogueFlags() & (DF_IN_DIALOG|DF_IN_CONTAINER|DF_FREEZE_SCRIPTS) ) {
		return false;
	}
	if (!partysize) {
		return false;
	}
	return (PCs.size()>partysize);
}

bool Game::AnyPCInCombat() const
{
	if (!CombatCounter) {
		return false;
	}

	return true;
}

//returns true if the protagonist (or the whole party died)
bool Game::EveryoneDead() const
{
	//if there are no PCs, then we assume everyone dead
	if (!PCs.size() ) {
		return true;
	}
	if (protagonist==PM_NO) {
		Actor *nameless = PCs[0];
		// don't trigger this outside pst, our game loop depends on it
		if (nameless->GetStat(IE_STATE_ID)&STATE_NOSAVE && core->HasFeature(GF_PST_STATE_FLAGS)) {
			if (area->INISpawn) {
				area->INISpawn->RespawnNameless();
			}
		}
		return false;
	}
	// if protagonist died
	if (protagonist==PM_YES) {
		if (PCs[0]->GetStat(IE_STATE_ID)&STATE_NOSAVE) {
			return true;
		}
		return false;
	}
	//protagonist == 2
	for (auto pc : PCs) {
		if (!(pc->GetStat(IE_STATE_ID)&STATE_NOSAVE)) {
			return false;
		}
	}
	return true;
}

//runs all area scripts

void Game::UpdateScripts()
{
	Update();

	PartyAttack = false;

	for (size_t idx = 0; idx < Maps.size(); idx++) {
		Maps[idx]->UpdateScripts();
	}

	if (PartyAttack) {
		//ChangeSong will set the battlesong only if CombatCounter is nonzero
		CombatCounter=150;
		ChangeSong(false, true);
	} else {
		if (CombatCounter) {
			CombatCounter--;
			//Change song if combatcounter went down to 0
			if (!CombatCounter) {
				ChangeSong(false, false);
			}
		}
	}

	if (StateOverrideTime)
		StateOverrideTime--;
	if (BanterBlockTime)
		BanterBlockTime--;

	if (Maps.size()>MAX_MAPS_LOADED) {
		size_t idx = Maps.size();

		//starting from 0, so we see the most recent master area first
		for(unsigned int i=0;i<idx;i++) {
			DelMap( (unsigned int) i, false );
		}
	}

	// perhaps a StartMusic action stopped the area music?
	// (we should probably find a less silly way to handle this,
	// because nothing can ever stop area music now..)
	if (!core->GetMusicMgr()->IsPlaying()) {
		ChangeSong(false,false);
	}

	//this is used only for the death delay so far
	if (event_handler) {
		if (!event_timer) {
			event_handler();
			event_handler = NULL;
		}
		event_timer--;
	}

	if (EveryoneDead()) {
		//don't check it any more
		protagonist = PM_NO;
		core->GetGUIScriptEngine()->RunFunction("GUIWORLD", "DeathWindow");
		return;
	}

	if (PartyOverflow()) {
		partysize = 0;
		core->GetGUIScriptEngine()->RunFunction("GUIWORLD", "OpenReformPartyWindow");
		return;
	}
}

void Game::SetTimedEvent(EventHandler func, int count)
{
	event_timer = count;
	event_handler = func;
}

void Game::SetProtagonistMode(int mode)
{
	protagonist = mode;
}

void Game::SetPartySize(int size)
{
	// 0 size means no party size control
	if (size<0) {
		return;
	}
	partysize = (size_t) size;
}

//Get the area dependent rest movie
ieResRef *Game::GetDream(Map *area)
{
	//select dream based on area
	int daynight = IsDay();
	if (area->Dream[daynight][0]) {
		return area->Dream+daynight;
	}
	int dream = (area->AreaType&(AT_FOREST|AT_CITY|AT_DUNGEON))>>3;
	return restmovies+dream;
}

//Start dream cutscenes for player1
void Game::PlayerDream()
{
	Scriptable *Sender = GetPC(0,true);
	if (!Sender) return;

	GameScript* gs = new GameScript( "player1d", Sender,0,0 );
	gs->Update();
	delete( gs );
}

//Start a TextScreen dream for the protagonist
void Game::TextDream()
{
	ieDword dream, chapter;
	locals->Lookup("CHAPTER", chapter);
	if (!locals->Lookup("DREAM", dream)) {
		dream = 1;
	}
	snprintf(TextScreen, sizeof(ieResRef)-1, "drmtxt%d", dream+1);
	if ((chapter > dream) && (core->Roll(1, 100, 0) <= 33)
		&& gamedata->Exists(TextScreen, IE_2DA_CLASS_ID)) {

		// give innate spell to protagonist
		AutoTable drm(TextScreen);
		if (drm) {
			const char *repLabel;
			if (Reputation >= 100)
				repLabel = "GOOD_POWER";
			else
				repLabel = "BAD_POWER";
			int row = drm->GetRowIndex(repLabel);
			if (row != -1) {
				Actor *actor = GetPC(0, false);
				actor->LearnSpell(drm->QueryField(row, 0), LS_MEMO|LS_LEARN);
			}
		}

		locals->SetAt("DREAM", dream+1);
		core->SetEventFlag(EF_TEXTSCREEN);
	}
}

// returns 0 if it can
// returns strref or -1 if it can't
int Game::CanPartyRest(int checks) const
{
	if (checks == REST_NOCHECKS) return 0;

	if (checks & REST_CONTROL) {
		for (auto pc : PCs) {
			if (pc->GetStat(IE_STATE_ID) & STATE_MINDLESS) {
				// You cannot rest at this time because you do not have control of all your party members
				return displaymsg->GetStringReference(STR_CANTTRESTNOCONTROL);
			}
		}
	}

	Actor *leader = GetPC(0, true);
	assert(leader);
	Map *area = leader->GetCurrentArea();
	//we let them rest if someone is paralyzed, but the others gather around
	if (checks & REST_SCATTER) {
		if (!EveryoneNearPoint(area, leader->Pos, 0)) {
			//party too scattered
			return displaymsg->GetStringReference(STR_SCATTERED);
		}
	}

	if (checks & REST_CRITTER) {
		//don't allow resting while in combat
		if (AnyPCInCombat()) {
			return displaymsg->GetStringReference(STR_CANTRESTMONS);
		}
		//don't allow resting if hostiles are nearby
		if (area->AnyEnemyNearPoint(leader->Pos)) {
			return displaymsg->GetStringReference(STR_CANTRESTMONS);
		}
	}

	//rest check, if PartyRested should be set, area should return true
	if (checks & REST_AREA) {
		//you cannot rest here
		if (area->AreaFlags & AF_NOSAVE) {
			return displaymsg->GetStringReference(STR_MAYNOTREST);
		}

		if (core->HasFeature(GF_AREA_OVERRIDE)) {
			// pst doesn't care about area types (see comments near AF_NOSAVE definition)
			// and repurposes these area flags!
			if ((area->AreaFlags & (AF_TUTORIAL|AF_DEADMAGIC)) == (AF_TUTORIAL|AF_DEADMAGIC)) {
				// you must obtain permission
				return 38587;
			} else if (area->AreaFlags&AF_TUTORIAL) {
				// you cannot rest in this area
				return 34601;
			} else if (area->AreaFlags&AF_DEADMAGIC) {
				// you cannot rest right now
				return displaymsg->GetStringReference(STR_MAYNOTREST);
			}
		} else {
			// you may not rest here, find an inn
			if (!(area->AreaType & (AT_OUTDOOR|AT_FOREST|AT_DUNGEON|AT_CAN_REST_INDOORS))) {
				return displaymsg->GetStringReference(STR_MAYNOTREST);
			}
		}
	}

	return 0;
}

// checks: can anything prevent us from resting?
// dream:
//   -1: no dream
//    0, 8+: dream based on area
//    1-7: dream selected from a fixed list
// hp: how much hp the rest will heal
// returns true if a cutscene dream is about to be played
bool Game::RestParty(int checks, int dream, int hp)
{
	if (CanPartyRest(checks)) {
		return false;
	}

	Actor *leader = GetPC(0, true);
	assert(leader);
	// TODO: implement "rest until healed", it's an option in some games
	int hours = 8;
	int hoursLeft = 0;
	if (checks & REST_AREA) {
		//area encounters
		// also advances gametime (so partial rest is possible)
		Trigger* parameters = new Trigger;
		parameters->int0Parameter = 0; // TIMEOFDAY_DAY, with a slight preference for daytime interrupts
		hoursLeft = area->CheckRestInterruptsAndPassTime(leader->Pos, hours, GameScript::TimeOfDay(nullptr, parameters));
		delete parameters;
		if (hoursLeft) {
			// partial rest only, so adjust the parameters for the loop below
			if (hp) {
				hp = hp * (hours - hoursLeft) / hours;
				// 0 means full heal, so we need to cancel it if we rounded to 0
				if (!hp) {
					hp = 1;
				}
			}
			hours -= hoursLeft;
			// the interruption occured before any resting could be done, so just bail out
			if (!hours) {
				return false;
			}
		}
	} else {
		AdvanceTime(hours * core->Time.hour_size);
	}

	int i = GetPartySize(true); // party size, only alive

	while (i--) {
		Actor *tar = GetPC(i, true);
		tar->ClearPath();
		tar->SetModal(MS_NONE, 0);
		//if hp = 0, then healing will be complete
		tar->Heal(hp);
		// auto-cast memorized healing spells if requested and available
		// run it only once, since it loops itself to save time
		if (i+1 == GetPartySize(true)) {
			CastOnRest();
		}
		//removes fatigue, recharges spells
		tar->Rest(hours);
		if (!hoursLeft)
			tar->PartyRested();
	}

	// also let familiars rest
	for (auto tar : NPCs) {
		if (tar->GetBase(IE_EA) == EA_FAMILIAR) {
			tar->ClearPath();
			tar->SetModal(MS_NONE, 0);
			tar->Heal(hp);
			tar->Rest(hours);
			if (!hoursLeft) tar->PartyRested();
		}
	}

	// abort the partial rest; we got what we wanted
	if (hoursLeft) {
		return false;
	}

	//movie, cutscene, and still frame dreams
	bool cutscene = false;
	if (dream>=0) {
		//cutscene dreams
		if (gamedata->Exists("player1d",IE_BCS_CLASS_ID, true)) {
			cutscene = true;
			PlayerDream();
		// all games have these bg1 leftovers, but only bg2 replaced the content
		} else if (gamedata->GetResource("drmtxt2", IE_2DA_CLASS_ID, true)->Size() > 0) {
			cutscene = true;
			TextDream();
		}

		//select dream based on area
		ieResRef *movie;
		if (dream==0 || dream>7) {
			movie = GetDream(area);
		} else {
			movie = restmovies+dream;
		}
		if (*movie[0]!='*') {
			core->PlayMovie(*movie);
		}
	}

	//set partyrested flags
	PartyRested();
	area->PartyRested();
	core->SetEventFlag(EF_ACTION);

	//bg1 has "You have rested for <DURATION>" while pst has "You have
	//rested for <HOUR> <DURATION>" and then bg1 has "<HOUR> hours" while
	//pst just has "Hours", so this works for both
	int restindex = displaymsg->GetStringReference(STR_REST);
	int hrsindex = displaymsg->GetStringReference(STR_HOURS);
	char* tmpstr = NULL;

	core->GetTokenDictionary()->SetAtCopy("HOUR", hours);

	//this would be bad
	if (hrsindex == -1 || restindex == -1) return cutscene;
	tmpstr = core->GetCString(hrsindex, 0);
	//as would this
	if (!tmpstr) return cutscene;

	core->GetTokenDictionary()->SetAtCopy("DURATION", tmpstr);
	core->FreeString(tmpstr);
	displaymsg->DisplayString(restindex, DMC_WHITE, 0);
	return cutscene;
}

// calculate an estimate of spell's healing power
inline static int CastOnRestHealingAmount(Actor *caster, SpecialSpellType &specialSpell)
{
	int healing = specialSpell.amount;
	if (specialSpell.bonus_limit > 0) {
		// cheating a bit, but the whole function is a heuristic anyway
		int bonusLevel = caster->GetAnyActiveCasterLevel();
		if (bonusLevel > specialSpell.bonus_limit) bonusLevel = specialSpell.bonus_limit;
		healing += bonusLevel; // 1 HP per level, usually corresponding to the die bonus
	}
	return healing;
}

// heal on rest and similar
void Game::CastOnRest()
{
	typedef std::vector<HealingResource> RestSpells;
	typedef std::vector<Injured> RestTargets;

	ieDword tmp = 0;
	core->GetDictionary()->Lookup("Heal Party on Rest", tmp);
	int specialCount = core->GetSpecialSpellsCount();
	if (!tmp || specialCount == -1) {
		return;
	}

	RestTargets wholeparty;
	int ps = GetPartySize(true);
	int ps2 = ps;
	for (int idx = 1; idx <= ps; idx++) {
		Actor *tar = FindPC(idx);
		if (tar) {
			ieWord hpneeded = tar->GetStat(IE_MAXHITPOINTS) - tar->GetStat(IE_HITPOINTS);
			wholeparty.push_back(Injured(hpneeded, tar));
		}
	}
	// Following algorithm works thus:
	// - If at any point there are no more injured party members, stop
	// (amount of healing done is an estimation)
	// - cast party members' all heal-all spells
	// - repeat:
	//       cast the most potent healing spell on the most injured member
	SpecialSpellType *special_spells = core->GetSpecialSpells();
	std::sort(wholeparty.begin(), wholeparty.end());
	RestSpells healingspells;
	RestSpells nonhealingspells;
	while (specialCount--) {
		SpecialSpellType &specialSpell = special_spells[specialCount];
		// Cast multi-target healing spells
		if ((specialSpell.flags & (SP_REST|SP_HEAL_ALL)) == (SP_REST|SP_HEAL_ALL)) {
			while (ps-- && wholeparty.back().hpneeded > 0) {
				Actor *tar = GetPC(ps, true);
				while (tar && tar->spellbook.HaveSpell(specialSpell.resref, 0) && wholeparty.back().hpneeded > 0) {
					tar->DirectlyCastSpell(tar, specialSpell.resref, 0, 1, true);
					for (auto injuree : wholeparty) {
						injuree.hpneeded -= CastOnRestHealingAmount(tar, specialSpell);
					}
				}
				std::sort(wholeparty.begin(), wholeparty.end());
			}
			ps = ps2;
		// Gather rest of the spells
		} else if (specialSpell.flags & SP_REST) {
			while (ps--) {
				Actor *tar = GetPC(ps, true);
				if (tar && tar->spellbook.HaveSpell(specialSpell.resref, 0)) {
					HealingResource resource;
					resource.caster = tar;
					CopyResRef(resource.resref, specialSpell.resref);
					resource.amount = 0;
					resource.amounthealed = CastOnRestHealingAmount(tar, specialSpell);
					// guess the booktype; one will definitely match due to HaveSpell above
					int booktype = 0;
					while (resource.amount == 0 && booktype < tar->spellbook.GetTypes()) {
						resource.amount = tar->spellbook.CountSpells(specialSpell.resref, booktype, 0);
						booktype++;
					}
					if (resource.amount == 0) continue;
					if (resource.amounthealed > 0 ) {
						healingspells.push_back(resource);
					} else {
						nonhealingspells.push_back(resource);
					}
				}
			}
			ps = ps2;
		}
	}
	std::sort(wholeparty.begin(), wholeparty.end());
	std::sort(healingspells.begin(), healingspells.end());
	// Heal who's still injured
	while (!healingspells.empty() && wholeparty.back().hpneeded > 0) {
		Injured &mostInjured = wholeparty.back();
		HealingResource &mostHealing = healingspells.back();
		mostHealing.caster->DirectlyCastSpell(mostInjured.character, mostHealing.resref, 0, 1, true);
		mostHealing.amount--;
		mostInjured.hpneeded -= mostHealing.amounthealed;
		std::sort(wholeparty.begin(), wholeparty.end());
		if (mostHealing.amount == 0) {
			healingspells.pop_back();
		}
	}
	// Other rest-time spells
	// Everybody gets something while stocks last!
	// In other words a better priorization of targets is needed
	ieWord spelltarget = 0;
	while (!nonhealingspells.empty()) {
		HealingResource &restingSpell = nonhealingspells.back();
		restingSpell.caster->DirectlyCastSpell(wholeparty.at(spelltarget).character, restingSpell.resref, 0, 1, true);
		restingSpell.amount--;
		if (restingSpell.amount == 0) {
			nonhealingspells.pop_back();
		}
		spelltarget++;
		if (spelltarget == wholeparty.size()) {
			spelltarget = 0;
		}
	}
}

//timestop effect
void Game::TimeStop(Actor* owner, ieDword end)
{
	timestop_owner=owner;
	timestop_end=end;
}

// check if the passed actor is a victim of timestop
bool Game::TimeStoppedFor(const Actor* target)
{
	if (!timestop_owner) {
		return false;
	}
	if (target == timestop_owner || target->GetStat(IE_DISABLETIMESTOP)) {
		return false;
	}
	return true;
}

//recalculate the party's infravision state
void Game::Infravision()
{
	hasInfra = false;
	Map *map = GetCurrentArea();
	if (!map) return;

	ieDword tmp = 0;
	core->GetDictionary()->Lookup("infravision", tmp);

	bool someoneWithInfravision = false;
	bool allSelectedWithInfravision = true;
	bool someoneSelected = false;

	for (auto actor : PCs) {
		if (!IsAlive(actor)) continue;
		if (actor->GetCurrentArea()!=map) continue;

		bool hasInfravision = actor->GetStat(IE_STATE_ID) & STATE_INFRA;
		someoneWithInfravision |= hasInfravision;

		someoneSelected |= actor->Selected;
		if (actor->Selected) {
			allSelectedWithInfravision &= hasInfravision;
		}

		if ((someoneWithInfravision && tmp) || (!tmp && !allSelectedWithInfravision)) {
			break;
		}
	}

	hasInfra = (tmp && someoneWithInfravision) || (allSelectedWithInfravision && someoneSelected);
}

//returns the colour which should be applied onto the whole game area viewport
//this is based on timestop, dream area, weather, daytime

static const Color DreamTint={0xf0,0xe0,0xd0,0x10};    //light brown scale
static const Color NightTint={0x80,0x80,0xe0,0x40};    //dark, bluish
static const Color DuskTint={0xe0,0x80,0x80,0x40};     //dark, reddish
static const Color DarkTint={0x80,0x80,0xe0,0x10};     //slightly dark bluish

const Color *Game::GetGlobalTint() const
{
	Map *map = GetCurrentArea();
	if (!map) return NULL;
	if (map->AreaFlags&AF_DREAM) {
		return &DreamTint;
	}
	if ((map->AreaType&(AT_OUTDOOR|AT_DAYNIGHT|AT_EXTENDED_NIGHT)) == (AT_OUTDOOR|AT_DAYNIGHT) ) {
		//get daytime colour
		ieDword daynight = core->Time.GetHour(GameTime);
		if (daynight<2 || daynight>22) {
			return &NightTint;
		}
		if (daynight>20 || daynight<4) {
			return &DuskTint;
		}
	}
	if ((map->AreaType&(AT_OUTDOOR|AT_WEATHER)) == (AT_OUTDOOR|AT_WEATHER)) {
		//get weather tint
		if (WeatherBits&WB_RAIN) {
			return &DarkTint;
		}
	}

	return NULL;
}

// applies the global tint, if any
void Game::ApplyGlobalTint(Color &tint, ieDword &flags) const
{
	const Color *globalTint = GetGlobalTint();
	if (globalTint) {
		if (flags & BLIT_TINTED) {
			Color::MultiplyTint(tint, globalTint);
		} else {
			flags |= BLIT_TINTED;
			tint = *globalTint;
			tint.a = 255;
		}
	}
}

bool Game::IsDay()
{
	ieDword daynight = core->Time.GetHour(GameTime);
	if(daynight<4 || daynight>20) {
		return false;
	}
	return true;
}

void Game::ChangeSong(bool always, bool force)
{
	int Song;
	static int BattleSong = 0;

	if (!area) return;

	if (CombatCounter) {
		//battlesong
		Song = SONG_BATTLE;
		BattleSong++;
	} else {
		//will select SONG_DAY or SONG_NIGHT
		// TODO: should this take time of day into account?
		Song = core->Time.GetHour(GameTime)/12;
		BattleSong = 0;
	}
	//area may override the song played (stick in battlemusic)
	//always transition gracefully with ChangeSong
	//force just means, we schedule the song for later, if currently
	//is playing
	// make sure we only start one battle song at a time, since we're called once per party member
	if (BattleSong < 2) {
		area->PlayAreaSong( Song, always, force );
	}
}

/* this method redraws weather. If update is false,
// then the weather particles won't change (game paused)
*/
void Game::DrawWeather(const Region &screen, bool update)
{
	if (!weather) {
		return;
	}
	if (!area->HasWeather()) {
		return;
	}

	weather->Draw( screen );
	if (!update) {
		return;
	}

	if (!(WeatherBits & (WB_RAIN|WB_SNOW)) ) {
		if (weather->GetPhase() == P_GROW) {
			weather->SetPhase(P_FADE);
		}
	}
	int drawn = weather->Update();
	if (drawn) {
		WeatherBits &= ~WB_INCREASESTORM;
	}

	if (WeatherBits&WB_HASWEATHER) {
		return;
	}
	StartRainOrSnow(true, area->GetWeather());
}

/* sets the weather type */
void Game::StartRainOrSnow(bool conditional, int w)
{
	if (conditional && (w & (WB_RAIN|WB_SNOW)) ) {
		if (WeatherBits & (WB_RAIN | WB_SNOW) )
			return;
	}
	// whatever was responsible for calling this, we now have some set weather
	WeatherBits = w | WB_HASWEATHER;
	if (w & WB_LIGHTNINGMASK) {
		if (WeatherBits&WB_INCREASESTORM) {
			//already raining
			if (GameTime&1) {
				core->PlaySound(DS_LIGHTNING1, SFX_CHAN_AREA_AMB);
			} else {
				core->PlaySound(DS_LIGHTNING2, SFX_CHAN_AREA_AMB);
			}
		} else {
			//start raining (far)
			core->PlaySound(DS_LIGHTNING3, SFX_CHAN_AREA_AMB);
		}
	}
	if (w&WB_SNOW) {
		core->PlaySound(DS_SNOW, SFX_CHAN_AREA_AMB);
		weather->SetType(SP_TYPE_POINT, SP_PATH_FLIT, SP_SPAWN_SOME);
		weather->SetPhase(P_GROW);
		weather->SetColor(SPARK_COLOR_WHITE);
		return;
	}
	if (w&WB_RAIN) {
		core->PlaySound(DS_RAIN, SFX_CHAN_AREA_AMB);
		weather->SetType(SP_TYPE_LINE, SP_PATH_RAIN, SP_SPAWN_SOME);
		weather->SetPhase(P_GROW);
		weather->SetColor(SPARK_COLOR_STONE);
		return;
	}
	weather->SetPhase(P_FADE);
}

void Game::SetExpansion(ieDword value)
{
	if (value) {
		if (Expansion>=value) {
			return;
		}
		Expansion = value;
	}

	core->SetEventFlag(EF_EXPANSION);
	switch(value) {
	default:
		break;
	//TODO: move this hardcoded hack to the scripts
	case 0:
		core->GetDictionary()->SetAt( "PlayMode", 2 );

		int i = GetPartySize(false);
		while(i--) {
			Actor *actor = GetPC(i, false);
			InitActorPos(actor);
		}
	}
}

void Game::dump() const
{
	StringBuffer buffer;

	buffer.append("Currently loaded areas:\n");
	for (auto map : Maps) {
		print("%s", map->GetScriptName());
	}
	buffer.appendFormatted("Current area: %s   Previous area: %s\n", CurrentArea, PreviousArea);
	if (Scripts[0]) {
		buffer.appendFormatted("Global script: %s\n", Scripts[0]->GetName());
	}
	int hours = GameTime/core->Time.hour_size;
	buffer.appendFormatted("Game time: %d (%d days, %d hours)\n", GameTime, hours/24, hours%24);
	buffer.appendFormatted("CombatCounter: %d\n", (int) CombatCounter);

	buffer.appendFormatted("Party size: %d\n", (int) PCs.size());
	for (auto actor : PCs) {
		buffer.appendFormatted("Name: %s Order %d %s\n",actor->ShortName, actor->InParty, actor->Selected?"x":"-");
	}

	buffer.appendFormatted("\nNPC count: %d\n", (int) NPCs.size());
	for (auto actor : NPCs) {
		buffer.appendFormatted("Name: %s\tSelected: %s\n", actor->ShortName, actor->Selected ? "x ": "-");
	}
	Log(DEBUG, "Game", buffer);
}

Actor *Game::GetActorByGlobalID(ieDword globalID)
{
	for (auto map : Maps) {
		Actor *actor = map->GetActorByGlobalID(globalID);
		if (actor) return actor;
	}
	return GetGlobalActorByGlobalID(globalID);
}

ieByte *Game::AllocateMazeData()
{
	if (mazedata) {
		free(mazedata);
	}
	mazedata = (ieByte*)malloc(MAZE_DATA_SIZE);
	return mazedata;
}

int Game::RemainingTimestop() const
{
	int remaining = timestop_end - GameTime;
	return remaining > 0 ? remaining : 0;
}

bool Game::IsTimestopActive() const
{
	return timestop_end > GameTime;
}

bool Game::RandomEncounter(ieResRef &BaseArea)
{
	if (bntrows<0) {
		AutoTable table;

		if (table.load("bntychnc")) {
			bntrows = table->GetRowCount();
			bntchnc = (int *) calloc(sizeof(int),bntrows);
			for(int i = 0; i<bntrows; i++) {
				bntchnc[i] = atoi(table->QueryField(i, 0));
			}
		} else {
			bntrows = 0;
		}
	}

	int rep = Reputation/10;
	if (rep>=bntrows) return false;
	if (core->Roll(1, 100, 0)>bntchnc[rep]) return false;
	//TODO: unhardcode this
	memcpy(BaseArea+4,"10",3);
	return gamedata->Exists(BaseArea, IE_ARE_CLASS_ID);
}

void Game::ResetPartyCommentTimes()
{
	for (auto pc : PCs) {
		pc->ResetCommentTime();
	}
}

bool Game::OnlyNPCsSelected() const
{
	bool hasPC = false;
	for (const Actor *selectee : selected) {
		if (selectee->GetStat(IE_SEX) < SEX_BOTH) {
			hasPC = true;
			break;
		}
	}
	return !hasPC;
}


}
