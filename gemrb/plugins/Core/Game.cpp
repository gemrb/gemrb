/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Game.cpp,v 1.113 2006/04/19 20:09:32 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "Game.h"
#include "MapMgr.h"
#include "DataStream.h"
#include "Interface.h"
#include "ResourceMgr.h"
#include "ScriptEngine.h"
#include "GameControl.h"
#include "../../includes/strrefs.h"

#define MAX_MAPS_LOADED 1

Game::Game(void) : Scriptable( ST_GLOBAL )
{
	protagonist = PM_YES; //set it to 2 for iwd/iwd2 and 0 for pst
	partysize = 6;
	Ticks = 0;
	version = 0;
	LoadMos[0] = 0;
	SelectedSingle = 1; //the PC we are looking at (inventory, shop)
	PartyGold = 0;
	SetScript( core->GlobalScript, 0 );
	MapIndex = -1;
	Reputation = 0;
	ControlStatus = 0;
	CombatCounter = 0; //stored here until we know better
	WeatherBits = 0;
	kaputz = NULL;
	beasts = NULL;
	mazedata = NULL;
	timestop_owner = NULL;
	timestop_end = 0;
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
	interval = 1000/AI_UPDATE_TIME;
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

	if (mazedata) {
		free (mazedata);
	}
	if (kaputz) {
		delete kaputz;
	}
	if (beasts) {
		free (beasts);
	}
	i=Journals.size();
	while(i--) {
		delete Journals[i];
	}
}

bool IsAlive(Actor *pc)
{
	if (pc->GetStat(IE_STATE_ID)&STATE_DEAD) {
		return false;
	}
	return true;
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

Actor* Game::GetPC(unsigned int slot, bool onlyalive)
{
	if (slot >= PCs.size()) {
		return NULL;
	}
	if (onlyalive) {
		unsigned int i=0;
		while(i<PCs.size() ) {
			Actor *ac = PCs[i++];
			
			if (IsAlive(ac) ) {
				if (!slot--) {
					return ac;
				}
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
	int max = PCs.size();
	std::vector< Actor*>::const_iterator m;
	for (int i=1;i<=max;) {
		if (FindPlayer(i)==-1) {
			
			for ( m = PCs.begin(); m != PCs.end(); ++m) {
				if ( (*m)->InParty>i) {
					(*m)->InParty--;
				}
			}
		} else i++;
	}
	for ( m = PCs.begin(); m != PCs.end(); ++m) {
		(*m)->Init();
	}
}

int Game::LeaveParty (Actor* actor)
{
	actor->CreateStats(); //create or update stats for leaving
	actor->SetBase(IE_EXPLORE, 0);
	SelectActor(actor, false, SELECT_NORMAL);
	int slot = InParty( actor );
	if (slot < 0) {
		return slot;
	}
	std::vector< Actor*>::iterator m = PCs.begin() + slot;
	PCs.erase( m );
	
	for ( m = PCs.begin(); m != PCs.end(); ++m) {
		if ( (*m)->InParty>actor->InParty) {
			(*m)->InParty--;
		}
	}
	//removing from party, but actor remains in 'game'
	actor->SetPersistent(0);
	NPCs.push_back( actor );

	if (core->HasFeature( GF_HAS_DPLAYER )) {
		actor->SetScript( "", SCR_DEFAULT );
	}
	actor->SetBase( IE_EA, EA_NEUTRAL );
	return ( int ) NPCs.size() - 1;
}

int Game::JoinParty(Actor* actor, int join)
{
	actor->CreateStats(); //create stats if they didn't exist yet
	actor->SetBase(IE_EXPLORE, 1);
	int slot = InParty( actor );
	if (slot != -1) {
		return slot;
	}
	if (join&JP_INITPOS) {
		// 0 - single player, 1 - tutorial, 2 - multiplayer
		int saindex = core->LoadTable( "startpos" );
		TableMgr* strta = core->GetTable( saindex );
		ieDword playmode = 0;
		if (strta->GetRowCount()==6) {
			core->GetDictionary()->Lookup( "PlayMode", playmode );
			playmode *= 2;
		}
		actor->Pos.x = actor->Destination.x = atoi( strta->QueryField( playmode, actor->InParty-1 ) );
		actor->Pos.y = actor->Destination.y = atoi( strta->QueryField( playmode + 1, actor->InParty-1 ) );
		core->DelTable( saindex );
	}
	if (join&JP_JOIN) {
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
	size_t size = PCs.size();

	PCs.push_back( actor );
	if (!actor->InParty) {
		actor->InParty = (ieByte) (size+1);
	}

	return ( int ) size;
}

int Game::GetPartySize(bool onlyalive) const
{
	if (onlyalive) {
		int count = 0;
		for (unsigned int i = 0; i < PCs.size(); i++) {
			if (!IsAlive(PCs[i])) {			
				continue;
			}
			count++;
		}
		return count;
	}
	return PCs.size();
}

/* sends the hotkey trigger to all selected actors */
void Game::SetHotKey(unsigned long Key)
{
	for(unsigned int i = 0; i < PCs.size(); i++) {
		Actor *actor = PCs[i];

		if (actor->IsSelected()) {
			actor->HotKey = (ieDword) Key;
		}
	}
}

bool Game::SelectPCSingle(int index)
{
	Actor* actor = FindPC( index );
	if (!actor || ! actor->ValidTarget( GA_SELECT | GA_NO_DEAD ))
		return false;

	SelectedSingle = index;
	return true;
}

int Game::GetSelectedPCSingle() const
{
	return SelectedSingle;
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

	// actor was not specified, which means all PCs should be (de)selected
	if (! actor) {
		for ( m = selected.begin(); m != selected.end(); ++m) {
			(*m)->Select( false );
			(*m)->SetOver( false );
		}

		selected.clear();
		if (select) {
			for ( m = PCs.begin(); m != PCs.end(); ++m) {
				if (! *m) {
					continue;
				}
				SelectActor( *m, true, SELECT_QUIET );
			}
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
	} else {
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
int Game::GetPartyLevel(bool onlyalive) const
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
	strnlwrcpy (tmp,area,8);
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

int Game::DelMap(unsigned int index, int forced)
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
		//keep at least one master
		const char *name = map->GetScriptName();
		if (MasterArea(name)) {
			if(!AnotherArea[0]) {
				memcpy(AnotherArea, name, sizeof(AnotherArea));
				if (!forced) {
					return -1;
				}
			}
		}
		//remove map from memory
		core->SwapoutArea(Maps[index]);
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

	MapMgr* mM = ( MapMgr* ) core->GetInterface( IE_ARE_CLASS_ID );
	DataStream* ds = core->GetResourceMgr()->GetResource( ResRef, IE_ARE_CLASS_ID );
	if (!ds) {
		core->FreeInterface( mM );
		return -1;
	}
	if(!mM->Open( ds, true )) {
		core->FreeInterface( mM );
		return -1;
	}
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
	} //can't add as npc already in party
	npc->SetPersistent(0);
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
/* returns true if it modified or added a journal entry */
bool Game::AddJournalEntry(ieStrRef strref, int Section, int Group)
{
	GAMJournalEntry *je = FindJournalEntry(strref);
	if (je) {
		//don't set this entry again in the same section
		if (je->Section==Section) {
			return false;
		}
		if (Section == IE_GAM_QUEST_DONE && Group) {
			//removing all of this group and adding a new entry
			DeleteJournalGroup(Group);
		} else {
			//modifying existing entry
			je->Section = Section;
			je->Group = Group;
			ieDword chapter = 0;
			locals->Lookup("CHAPTER", chapter);
			je->Chapter = (ieByte) chapter;
			je->GameTime = GameTime;
			return true;
		}
	}
	je = new GAMJournalEntry;
	je->GameTime = GameTime;
	ieDword chapter = 0;
	locals->Lookup("CHAPTER", chapter);
	je->Chapter = (ieByte) chapter;
	je->Section = Section;
	je->Group = Group;
	je->Text = strref;

	Journals.push_back( je );
	return true;
}

void Game::AddJournalEntry(GAMJournalEntry* entry)
{
	Journals.push_back( entry );
}

int Game::GetJournalCount() const
{
	return Journals.size();
}

GAMJournalEntry* Game::FindJournalEntry(ieStrRef strref)
{
	unsigned int Index = Journals.size();
	while(Index--) {
		GAMJournalEntry *ret = Journals[Index];

		if (ret->Text==strref) {
			return ret;
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

char *Game::GetFamiliar(unsigned int Index)
{
	return Familiars[Index];
}

void Game::ShareXP(int xp, bool divide)
{
	int individual;

	if (divide) {
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
	
	for (unsigned int i=0; i<PCs.size(); i++) {
		if (PCs[i]->GetStat(IE_STATE_ID)&STATE_DEAD) {
			continue;
		}
		PCs[i]->AddExperience(individual);
	}
	if (xp>0) {
		core->DisplayConstantStringValue( STR_GOTXP, 0xc0c000, (ieDword) xp); //you have gained ... xp
	} else {
		core->DisplayConstantStringValue( STR_LOSTXP, 0xc0c000, (ieDword) -xp); //you have gained ... xp
	}
}

bool Game::EveryoneStopped() const
{
	for (unsigned int i=0; i<PCs.size(); i++) {
		if (PCs[i]->path ) return false;
	}
	return true;
}

//canmove=true: if some PC can't move (or hostile), then this returns false 
bool Game::EveryoneNearPoint(Map *area, Point &p, int flags) const
{
	for (unsigned int i=0; i<PCs.size(); i++) {
		if (flags&ENP_ONLYSELECT) {
			if(!PCs[i]->Selected) {
				continue;
			}
		}
		if (PCs[i]->GetStat(IE_STATE_ID)&STATE_DEAD) {
			continue;
		}
		if (flags&ENP_CANMOVE) {
			//someone is uncontrollable, can't move
			if (PCs[i]->GetStat(IE_EA)>EA_GOODCUTOFF) {
				return false;
			}

			if (PCs[i]->GetStat(IE_STATE_ID)&STATE_CANTMOVE) {
				return false;
			}
		}
		if (PCs[i]->GetCurrentArea()!=area) {
			return false;
		}
		if (Distance(p,PCs[i])>MAX_TRAVELING_DISTANCE) {
			return false;
		}
	}
	return true;
}

//called when someone died
void Game::PartyMemberDied(Actor *actor)
{
	Map *area = actor->GetCurrentArea();

	for (unsigned int i=0; i<PCs.size(); i++) {
		if (PCs[i]==actor) {
			continue;
		}
		if (PCs[i]->GetStat(IE_STATE_ID)&STATE_DEAD) {
			continue;
		}
		if (PCs[i]->GetCurrentArea()!=area) {
			continue;
		}
		PCs[i]->ReactToDeath(actor->GetScriptName());
	}
}

//reports if someone died
int Game::PartyMemberDied() const
{
	for (unsigned int i=0; i<PCs.size(); i++) {
		if (PCs[i]->GetInternalFlag()&IF_JUSTDIED) {
			return i;
		}
	}
	return -1;
}

void Game::IncrementChapter()
{
	//chapter first set to 0 (prologue)
	ieDword chapter = (ieDword) -1;
	locals->Lookup("CHAPTER",chapter);
	locals->SetAt("CHAPTER",chapter+1);
	//clear statistics	
	for (unsigned int i=0; i<PCs.size(); i++) {
		//all PCs must have this!
		PCs[i]->PCStats->IncrementChapter();
	}
}

void Game::SetReputation(ieDword r)
{
	if (r<10) r=10;
	else if (r>200) r=200;
	if (Reputation>r) {
		core->DisplayConstantStringValue(STR_LOSTREP,0xc0c000,(Reputation-r)/10);
	} else if (Reputation<r) {
		core->DisplayConstantStringValue(STR_GOTREP,0xc0c000,(r-Reputation)/10);
	}
	Reputation = r;
	for (unsigned int i=0; i<PCs.size(); i++) {
		PCs[i]->SetBase(IE_REPUTATION, Reputation);
	}
}

void Game::SetControlStatus(int value, int mode)
{
	switch(mode) {
		case BM_OR: ControlStatus|=value; break;
		case BM_NAND: ControlStatus&=~value; break;
		case BM_SET: ControlStatus=value; break;
		case BM_AND: ControlStatus&=value; break;
		case BM_XOR: ControlStatus^=value; break;
	}
}

/* sets the weather type */
void Game::StartRainOrSnow(bool conditional, int weather)
{
	if (conditional && weather) {
		if (WeatherBits & (WB_RAIN| WB_SNOW) )
			return;
	}
	WeatherBits = weather;
}

void Game::AddGold(ieDword add)
{
	ieDword old;

	if (!add) {
		return;
	}
	old = PartyGold;
	PartyGold += add;
	if (old<PartyGold) {
		core->DisplayConstantStringValue( STR_GOTGOLD, 0xc0c000, PartyGold-old); 
	} else {
		core->DisplayConstantStringValue( STR_LOSTGOLD, 0xc0c000, old-PartyGold);
	}
}

//later this could be more complicated
void Game::AdvanceTime(ieDword add)
{
	GameTime+=add;
	Ticks+=add*interval;
}

//returns true if there are excess players in the team
bool Game::PartyOverflow()
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
//returns true if the protagonist (or the whole party died)
bool Game::EveryoneDead() const
{
	if (protagonist==PM_NO) {
		return false;
	}
	//if there are no PCs, then we assume everyone dead
	if (!PCs.size() ) {
		return true;
	}
	// if protagonist died
	if (protagonist==PM_YES) {
		if (PCs[0]->GetStat(IE_STATE_ID)&STATE_NOSAVE) {
			return true;
		}
		return false;
	}
	//protagonist == 2
	for (unsigned int i=0; i<PCs.size(); i++) {
		if (!(PCs[i]->GetStat(IE_STATE_ID)&STATE_NOSAVE) ) {
			return false;
		}
	}
	return true;
}

//runs all area scripts
void Game::UpdateScripts()
{
	ExecuteScript( Scripts[0] );
	ProcessActions(false);
	size_t idx;

	for (idx=0;idx<Maps.size();idx++) {
		Maps[idx]->UpdateScripts();
	}
	if (Maps.size()>MAX_MAPS_LOADED) {
		idx = Maps.size();
		AnotherArea[0]=0;

		while (idx--) {
			DelMap( idx, false );
		}
	}

	if (GameTime & 0x10) {
		if (EveryoneDead()) {
			//don't check it any more
			protagonist = PM_NO;
			core->GetGUIScriptEngine()->RunFunction("DeathWindow");
			return;
		}
		if (PartyOverflow()) {
			partysize = 0;
			core->GetGUIScriptEngine()->RunFunction("OpenReformPartyWindow");
			return;
		}
	}
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

//noareacheck = no random encounters
void Game::RestParty(bool noareacheck)
{
	Point p = GetPC(0,false)->Pos;
	Map *area = GetCurrentArea();
	//we let them rest if someone is paralyzed, but the others gather around
	if (!EveryoneNearPoint( area, p, 0 ) ) {
		//party too scattered
		core->DisplayConstantString( STR_SCATTERED, 0xff0000 );
		return;
	}
	//rest check, if PartyRested should be set, area should return true
	//area should advance gametime too (so partial rest is possible)
	if (noareacheck || area->Rest( p, 8, GameTime%7200/3600) ) {
		InternalFlags |= IF_PARTYRESTED;
	}
}

//timestop effect
void Game::TimeStop(Actor* owner, ieDword end)
{
	timestop_owner=owner;
	timestop_end=end;
}

//returns the colour which should be applied onto the whole game area viewport
//this is based on timestop, dream area, weather, daytime

static Color TimeStopTint={0xe0,0xe0,0xe0,0x20}; //greyscale
static Color DreamTint={0xf0,0xe0,0xd0,0x10};    //light brown scale
static Color NightTint={0x80,0x80,0xe0,0x40};    //dark, bluish
static Color DuskTint={0xe0,0x80,0x80,0x40};     //dark, reddish
static Color FogTint={0xff,0xff,0xff,0x40};      //whitish
static Color DarkTint={0x80,0x80,0xe0,0x10};     //slightly dark bluish

Color *Game::GetGlobalTint()
{
	if (timestop_end>GameTime) {
		return &TimeStopTint;
	}
	Map *map = GetCurrentArea();
	if (map->AreaFlags&AF_DREAM) {
		return &DreamTint;
	}
	if ((map->AreaType&(AT_OUTDOOR|AT_WEATHER)) == (AT_OUTDOOR|AT_WEATHER)) {
		//get weather tint
		if (WeatherBits&WB_RAIN) {
			return &DarkTint;
		}
		if (WeatherBits&WB_FOG) {
			return &FogTint;
		}
	}
	if ((map->AreaType&(AT_OUTDOOR|AT_DAYNIGHT|AT_EXTENDED_NIGHT)) == (AT_OUTDOOR|AT_DAYNIGHT) ) {
		//get daytime colour
		ieDword daynight = (GameTime%7200/300);
		if (daynight<2 || daynight>11) {
			return &NightTint;
		}
		if (daynight>9 || daynight<4) {
			return &DuskTint;
		}
	}
	return NULL;
}

void Game::DebugDump()
{
	printf("Currently loaded areas:\n");
	for(size_t idx=0;idx<Maps.size();idx++) {
		Map *map = Maps[idx];

		printf("%s\n",map->GetScriptName());
	}
}
