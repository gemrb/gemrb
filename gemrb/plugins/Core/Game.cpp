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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Game.cpp,v 1.70 2005/04/03 21:00:03 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "Game.h"
#include "MapMgr.h"
#include "DataStream.h"
#include "Interface.h"
#include "../../includes/strrefs.h"

#define MAX_MAPS_LOADED  5

Game::Game(void) : Scriptable( ST_GLOBAL )
{
	LoadMos[0] = 0;
	SelectedSingle = 1; //the PC we are looking at (inventory, shop)
	PartyGold = 0;
	SetScript( core->GlobalScript, 0 );
	MapIndex = -1;
	Reputation = 0;
	CombatCounter = 0; //stored here until we know better
	globals = NULL;
	kaputz = NULL;
	familiars = NULL;
	int mtab = core->LoadTable("mastarea");
	if (mtab) {
		TableMgr *table = core->GetTable(mtab);
		int i = table->GetRowCount();
		while(i--) {
			char *tmp = (char *) malloc(9);
			strncpy (tmp,table->QueryField(i,0),8);
			tmp[8]=0;
			mastarea.push_back( tmp );
		}
	}
	core->DelTable(mtab);
}

Game::~Game(void)
{
	size_t i;

	for (i = 0; i < Maps.size(); i++) {
		delete( Maps[i] );
	}
	for (i = 0; i < PCs.size(); i++) {
		delete ( PCs[i] );
	}
	for (i = 0; i < NPCs.size(); i++) {
		delete ( NPCs[i] );
	}
	for (i = 0; i < mastarea.size(); i++) {
		free ( mastarea[i] );
	}
	if (globals) {
		delete globals;
	}
	if (kaputz) {
		delete kaputz;
	}
	if (familiars) {
		free (familiars);
	}
	i=Journals.size();
	while(i--) {
		delete Journals[i];
	}
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
	for (unsigned int slot=0; slot<PCs.size(); slot++) {
		if (PCs[slot]->InParty==partyID) return PCs[slot];
	}
	return NULL;
}

Actor* Game::FindPC(const char *scriptingname)
{
	for (unsigned int slot=0; slot<PCs.size(); slot++) {
		if (strnicmp(PCs[slot]->GetScriptName(),scriptingname,32)==0 )
		{
			return PCs[slot];
		}
	}
	return NULL;
}

Actor* Game::FindNPC(unsigned int partyID)
{
	for (unsigned int slot=0; slot<NPCs.size(); slot++) {
		if (NPCs[slot]->InParty==partyID) return NPCs[slot];
	}
	return NULL;
}

Actor* Game::FindNPC(const char *scriptingname)
{
	for (unsigned int slot=0; slot<NPCs.size(); slot++) {
		if (strnicmp(NPCs[slot]->GetScriptName(),scriptingname,32)==0 )
		{
			return NPCs[slot];
		}
	}
	return NULL;
}

Actor* Game::GetPC(unsigned int slot)
{
	if (slot >= PCs.size()) {
		return NULL;
	}
	return PCs[slot];
}

int Game::InStore(Actor* pc)
{
	for (unsigned int i = 0; i < NPCs.size(); i++) {
		if (NPCs[i] == pc) {
			return i;
		}
	}
	return -1;
}

int Game::InParty(Actor* pc)
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

int Game::LeaveParty(Actor* actor)
{
	actor->CreateStats(); //create or update stats for leaving
	actor->SetBase(IE_EXPLORE, 0);
	int slot = InParty( actor );
	if (slot < 0) {
		return slot;
	}
	std::vector< Actor*>::iterator m = PCs.begin() + slot;
	PCs.erase( m );
	NPCs.push_back( actor );
	return ( int ) NPCs.size() - 1;
}

int Game::JoinParty(Actor* actor, bool join)
{
	actor->CreateStats(); //create stats if they didn't exist yet
	actor->SetBase(IE_EXPLORE, 1);
	int slot = InParty( actor );
	if (slot != -1) {
		return slot;
	}
	if (join) {
		actor->PCStats->JoinDate = GameTime;
		if (!PCs.size() ) {
			Reputation = actor->GetStat(IE_REPUTATION);
		}
	}
	slot = InStore( actor );
	if (slot >= 0) {
		std::vector< Actor*>::iterator m = NPCs.begin() + slot;
		NPCs.erase( m );
	}
	PCs.push_back( actor );
	actor->InParty = PCs.size();
	return ( int ) PCs.size() - 1;
}

int Game::GetPartySize(bool onlyalive)
{
	if (onlyalive) {
		int count = 0;
		for (unsigned int i = 0; i < PCs.size(); i++) {
			if (PCs[i]->GetStat(IE_STATE_ID)&STATE_DEAD) {
				continue;
			}
			count++;
		}
		return count;
	}
	return PCs.size();
}

bool Game::SelectPCSingle(int index)
{
	Actor* actor = FindPC( index );
	if (!actor || ! actor->ValidTarget( GA_SELECT | GA_NO_DEAD ))
		return false;

	SelectedSingle = index;
	return true;
}

int Game::GetSelectedPCSingle()
{
	return SelectedSingle;
}

/*
 * SelectActor() - handle (de)selecting actors.
 *     If selection was changed, runs "SelectionChanged" handler
 *
 * actor - either specific actor, or NULL for all
 * select - whether actor(s) should be selected or deselected
 * flags:
 * SELECT_ONE   - if true, deselect all other actors when selecting one
 * SELECT_QUIET - do not run handler if selection was changed. Used for
 * nested calls to SelectActor()
 */

bool Game::SelectActor(Actor* actor, bool select, unsigned flags)
{
	std::vector< Actor*>::iterator m;

	// actor was not specified, which means all PCs should be (de)selected
	if (! actor) {
		if (select) {
			SelectActor( NULL, false, SELECT_QUIET );
			for ( m = PCs.begin(); m != PCs.end(); ++m) {
				if (! *m) {
					continue;
				}
				SelectActor( *m, true, SELECT_QUIET );
			}
		}
		else {
			for ( m = selected.begin(); m != selected.end(); ++m) {
				(*m)->Select( false );
				(*m)->SetOver( false );
			}

			selected.clear();
		}

		if (! (flags & SELECT_QUIET)) { 
			core->GetGUIScriptEngine()->RunFunction( "SelectionChanged" );
		}
		return true;
	}

	// actor was specified, so we will work with him

	// If actor is already (de)selected, report success, but do nothing
	//if (actor->IsSelected() == select)
	//	return true;

	
	if (select) {
		if (! actor->ValidTarget( GA_SELECT | GA_NO_DEAD ))
			return false;

		// deselect all actors first when exclusive
		if (flags & SELECT_REPLACE) {
			SelectActor( NULL, false, SELECT_QUIET );
		}

		actor->Select( true );
		selected.push_back( actor );
	}
	else {
		for ( m = selected.begin(); m != selected.end(); ++m) {
			if ((*m) == actor) {
				selected.erase( m );
				break;
			}
		}
		actor->Select( false );
	}

	if (! (flags & SELECT_QUIET)) { 
		core->GetGUIScriptEngine()->RunFunction( "SelectionChanged" );
	}
	return true;
}

// Gets average party level, of onlyalive is true, then counts only living PCs
int Game::GetPartyLevel(bool onlyalive)
{
	int count = 0;
	for (unsigned int i = 0; i<PCs.size(); i++) {
			if (onlyalive) {
				if (PCs[i]->GetStat(IE_STATE_ID)&STATE_DEAD) {
					continue;
				}
			}
			count += PCs[i]->GetXPLevel(0);
	}
	return count;
}

// Returns map structure (ARE) if it is already loaded in memory
int Game::FindMap(const char *ResRef)
{
	int index = Maps.size();
	while (index--) {
		Map *map=Maps[index];
		if (strnicmp(ResRef, map->GetScriptName(), 8) == 0) {
			return index;
		}
	}
	return -1;
}

/*  TODO, create save game
int Game::WriteGame()
{
//write .gam structure
	int index=Maps.size();
	while (index--) {
		Map *map = Maps[index];
	//write map
	}
}
*/

Map* Game::GetMap(unsigned int index) const
{
	if (index >= Maps.size()) {
		return NULL;
	}
	return Maps[index];
}

Map *Game::GetMap(const char *areaname, bool change)
{
	int index = LoadMap(areaname);
	if (index >= 0) {
		if (change) {
			MapIndex = index;
			area = GetMap(index);
			memcpy (CurrentArea, areaname, 8);
		}
		return GetMap(index);
	}
	return NULL;
}

bool Game::MasterArea(const char *area)
{
	int i=mastarea.size();
	while(i--) {
		if (strnicmp(mastarea[i], area, 8) ) {
			return true;
		}
	}
	return false;
}

void Game::SetMasterArea(const char *area)
{
	if (MasterArea(area) ) return;
	char *tmp = (char *) malloc(9);
	strnuprcpy (tmp,area,8);
	mastarea.push_back(tmp);
}

int Game::AddMap(Map* map)
{
	unsigned int i = Maps.size();
	if (MasterArea(map->GetScriptName()) ) {
		//no push_front, we do this ugly hack
		Maps.push_back(NULL);
		for (;i;i--) {
			Maps[i]=Maps[i-1];
		}
		Maps[0] = map;
		MapIndex++;
		return 0;
	}
	Maps.push_back( map );
	return i;
}

int Game::DelMap(unsigned int index, bool forced)
{
//this function should archive the area, and remove it only if the area
//contains no active actors (combat, partymembers, etc)
	if (index >= Maps.size()) {
		return -1;
	}

	if (MapIndex==(int) index) { //can't remove current map in any case
		return -1;
	}

	Map *map = Maps[index];

	if (!map) { //this shouldn't happen, i guess
		printMessage("Game","Erased NULL Map\n",YELLOW);
		Maps.erase( Maps.begin()+index);
		if (MapIndex>(int) index) {
			MapIndex--;
		}
		return 1;
	}
	if (forced || ((Maps.size()>MAX_MAPS_LOADED) && map->CanFree() ) )
	{
		delete( Maps[index] );
		Maps.erase( Maps.begin()+index);
		//current map will be decreased
		if (MapIndex>(int) index) {
			MapIndex--;
		}
		return 1;
	}
	//didn't remove the map
	return 0;
}

/* Loads an area, changepf == true if you want to setup the pathfinder too */
//FIXME: changepf is removed now
int Game::LoadMap(const char* ResRef)
{
	unsigned int i;
	int index = FindMap(ResRef);
	if (index>=0) {
		return index;
	}
	//check if any other areas could be removed (cached)
	//areas cannot be removed with PCs
	//master areas cannot be removed with active actors (pathlength!=0 or combat)
	index = Maps.size();
	while (index--) {
		DelMap( index, false );
	}

	MapMgr* mM = ( MapMgr* ) core->GetInterface( IE_ARE_CLASS_ID );
	DataStream* ds = core->GetResourceMgr()->GetResource( ResRef, IE_ARE_CLASS_ID );
	if (!ds) {
		core->FreeInterface( mM );
		return -1;
	}
	mM->Open( ds, true );
	Map* newMap = mM->GetMap(ResRef);
	core->FreeInterface( mM );
	if (!newMap) {
		return -1;
	}

	for (i = 0; i < PCs.size(); i++) {
		if (stricmp( PCs[i]->Area, ResRef ) == 0) {
			newMap->AddActor( PCs[i] );
		}
	}
	for (i = 0; i < NPCs.size(); i++) {
		if (stricmp( NPCs[i]->Area, ResRef ) == 0) {
			newMap->AddActor( NPCs[i] );
		}
	}
	return AddMap( newMap );
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
	}     //can't add as npc already in party
	npc->InternalFlags|=IF_FROMGAME;
	NPCs.push_back( npc );
	return NPCs.size() - 1;
}

Actor* Game::GetNPC(unsigned int Index)
{
	if (Index >= NPCs.size()) {
		return NULL;
	}
	return NPCs[Index];
}

void Game::DeleteJournalEntry(ieStrRef strref)
{
	size_t i=Journals.size();
	while(i--) {
		if (Journals[i]->Text==strref) {
			delete Journals[i];
			Journals.erase(Journals.begin()+i);
		}
	}
}

void Game::DeleteJournalGroup(ieByte Group)
{
	size_t i=Journals.size();
	while(i--) {
		if (Journals[i]->Group==Group) {
			delete Journals[i];
			Journals.erase(Journals.begin()+i);
		}
	}
}

void Game::AddJournalEntry(ieStrRef strref, int Section, int Group)
{
	GAMJournalEntry *je = new GAMJournalEntry;
	je->GameTime = GameTime;
	ieDword chapter = 0;
	globals->Lookup("CHAPTER", chapter);
	je->Chapter = (ieByte) chapter;
	je->Section = Section;
	je->Group = Group;
	je->Text = strref;
	Journals.push_back( je );
}

void Game::AddJournalEntry(GAMJournalEntry* entry)
{
	Journals.push_back( entry );
}

int Game::GetJournalCount()
{
	return Journals.size();
}

GAMJournalEntry* Game::GetJournalEntry(unsigned int Index)
{
	if (Index >= Journals.size()) {
		return NULL;
	}
	return Journals[Index];
}

void Game::ShareXP(int xp, bool divide)
{
	if (divide) {
		int PartySize = GetPartySize(true); //party size, only alive
		if (PartySize<1) {
			return;
		}
		xp /= PartySize;
	}

	if (!xp) {
		return;
	}
	
	for (unsigned int i=0; i<PCs.size(); i++) {
		if (PCs[i]->GetStat(IE_STATE_ID)&STATE_DEAD) {
			continue;
		}
		PCs[i]->NewStat(IE_XP,xp,MOD_ADDITIVE);
	}
	char value[10];

	sprintf( value, "%d", xp );
	core->GetTokenDictionary()->SetAtCopy( "XP", value );
	core->DisplayConstantString( STR_GOTXP, 0xc0c000); //you have gained ... xp
}

bool Game::EveryoneStopped()
{
	for (unsigned int i=0; i<PCs.size(); i++) {
		if (PCs[i]->path ) return false;
	}
	return true;
}

//canmove=true: if some PC can't move (or hostile), then this returns false 
bool Game::EveryoneNearPoint(const char *area, Point &p, bool canmove)
{
	for (unsigned int i=0; i<PCs.size(); i++) {
		if (PCs[i]->GetStat(IE_STATE_ID)&STATE_DEAD) {
			continue;
		}
		if (canmove) {
			//someone is uncontrollable, can't move
			if (PCs[i]->GetStat(IE_EA)>GOODCUTOFF) {
				return false;
			}

			if (PCs[i]->GetStat(IE_STATE_ID)&STATE_CANTMOVE) {
				return false;
			}
		}
		if (stricmp(PCs[i]->Area,area) ) {
			return false;
		}
		if (Distance(p,PCs[i])>MAX_TRAVELING_DISTANCE) {
			return false;
		}
	}
	return true;
}

bool Game::PartyMemberDied()
{
	for (unsigned int i=0; i<PCs.size(); i++) {
		if (PCs[i]->InternalFlags&IF_JUSTDIED)
			return true;
	}
	return false;
}

void Game::IncrementChapter()
{
	//clear statistics
	
	for (unsigned int i=0; i<PCs.size(); i++) {
		//all PCs must have this!
		PCs[i]->PCStats->IncrementChapter();
	}
	ieDword chapter = 0;
	globals->Lookup("CHAPTER",chapter);
	globals->SetAt("CHAPTER",chapter+1);
}

void Game::SetReputation(int r)
{
	if (r<10) r=10;
	else if (r>200) r=200;
	Reputation = (ieDword) r;
	for (unsigned int i=0; i<PCs.size(); i++) {
		PCs[i]->SetStat(IE_REPUTATION, Reputation);
	}
}
