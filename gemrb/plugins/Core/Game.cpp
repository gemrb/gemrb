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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Game.cpp,v 1.55 2004/08/18 21:55:19 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "Game.h"
#include "MapMgr.h"
#include "DataStream.h"
#include "Interface.h"

extern Interface* core;

Game::Game(void) : Scriptable( ST_GLOBAL )
{
	PartyGold = 0;
	SetScript( core->GlobalScript, 0 );
	MapIndex = -1;
	SelectedSingle = 0;
	Reputation = 0;
	CombatCounter = 0; //stored here until we know better
	globals = NULL;
	kaputz = NULL;
	familiars = NULL;
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
	for(unsigned int slot=0; slot<PCs.size(); slot++) {
		if(PCs[slot]->InParty==partyID) {
			return slot;
		}
	}
	return -1;
}

Actor* Game::FindPC(unsigned int partyID)
{
	for(unsigned int slot=0; slot<PCs.size(); slot++) {
		if(PCs[slot]->InParty==partyID) return PCs[slot];
	}
	return NULL;
}

Actor* Game::FindPC(const char *scriptingname)
{
	for(unsigned int slot=0; slot<PCs.size(); slot++) {
		if(strnicmp(PCs[slot]->scriptName,scriptingname,32)==0 )
		{
			return PCs[slot];
		}
	}
	return NULL;
}

Actor* Game::FindNPC(unsigned int partyID)
{
	for(unsigned int slot=0; slot<NPCs.size(); slot++) {
		if(NPCs[slot]->InParty==partyID) return NPCs[slot];
	}
	return NULL;
}

Actor* Game::FindNPC(const char *scriptingname)
{
	for(unsigned int slot=0; slot<NPCs.size(); slot++) {
		if(strnicmp(NPCs[slot]->scriptName,scriptingname,32)==0 )
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

int Game::SetPC(Actor* pc)
{
	int slot = InParty( pc );
	if (slot != -1) {
		return slot;
	}
	slot = InStore( pc );
	if (slot != -1)	   //it is an NPC, we remove it from the NPC vector
	{
		DelNPC(slot, false);
	}
	if(!PCs.size() ) {
		Reputation = pc->GetStat(IE_REPUTATION);
	}
	PCs.push_back( pc );
	return ( int ) PCs.size() - 1;
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
	int slot = InParty( actor );
	if (slot < 0) {
		return slot;
	}
	std::vector< Actor*>::iterator m = PCs.begin() + slot;
	PCs.erase( m );
	NPCs.push_back( actor );
	return ( int ) NPCs.size() - 1;
}

int Game::JoinParty(Actor* actor)
{
	int slot = InStore( actor );
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
	if (index < 0 || index >= GetPartySize (false))
		return false;

	SelectedSingle = index;
	return true;
}

int Game::GetSelectedPCSingle()
{
	return SelectedSingle;
}

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

int Game::FindMap(const char *ResRef)
{
	int index = Maps.size();
	while (index--) {
		Map *map=Maps[index];
		if (strnicmp(ResRef, map->scriptName, 8) == 0) {
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

Map* Game::GetMap(unsigned int index)
{
	if (index >= Maps.size()) {
		return NULL;
	}
	return Maps[index];
}

Map *Game::GetMap(const char *areaname)
{
	int index = LoadMap(areaname);
	if(index>=0) {
		return GetMap(index);
	}
	return NULL;
}

Map *Game::GetCurrentMap()
{
	return GetMap(MapIndex);
}

//TODO: master area determination
bool Game::MasterArea(const char *area)
{
	return 0;
}

int Game::AddMap(Map* map)
{
/* we don't allow holes in this vector
	unsigned int i=Maps.size();
	while(i--) {
		if (!Maps[i]) {
			Maps[i] = map;
			return ( int ) i;
		}
	}
*/
	unsigned int i = Maps.size();
	if(MasterArea(map->scriptName) ) {
		//no push_front, we do this ugly hack
		Maps.push_back(NULL);
		for(;i;i--) {
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
	Map *map = Maps[index];

	if (!map) {
		return -1;
	}
	if(forced || map->CanFree() )
	{
		delete( Maps[index] );
		Maps.erase( Maps.begin()+index);
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
	if(index>=0) {
		return index;
	}
	//check if the current area could be removed, 
	//don't remove it if the pathfinder is still connected to it 
	DelMap( MapIndex, false );

	MapMgr* mM = ( MapMgr* ) core->GetInterface( IE_ARE_CLASS_ID );
	DataStream* ds = core->GetResourceMgr()->GetResource( ResRef, IE_ARE_CLASS_ID );
	if(!ds) {
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
	unsigned long chapter = 0;
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

void Game::ShareXP(int xp)
{
	int PartySize = GetPartySize(true); //party size, only alive
	if(PartySize<1) {
		return;
	}
	xp /= PartySize;
	if(!xp) {
		return;
	}
	
	for(unsigned int i=0; i<PCs.size(); i++) {
		if (PCs[i]->GetStat(IE_STATE_ID)&STATE_DEAD) {
			continue;
		}
		PCs[i]->NewStat(IE_XP,xp,MOD_ADDITIVE);
	}
	//core->DisplayConstantString(); //you have gained ... xp
}

bool Game::EveryoneStopped()
{
	for(unsigned int i=0; i<PCs.size(); i++) {
		if(PCs[i]->path ) return false;
	}
	return true;
}

//canmove=true: if some PC can't move (or hostile), then this returns false 
bool Game::EveryoneNearPoint(const char *area, int x, int y, bool canmove)
{
	for(unsigned int i=0; i<PCs.size(); i++) {
		if (PCs[i]->GetStat(IE_STATE_ID)&STATE_DEAD) {
			continue;
		}
		//someone is uncontrollable, can't move
		if(PCs[i]->GetStat(IE_EA)>GOODCUTOFF) {
			return false;
		}
		if(PCs[i]->GetStat(IE_STATE_ID)&STATE_CANTMOVE) {
			return false;
		}
		if(stricmp(PCs[i]->Area,area) ) {
			return false;
		}
		if(Distance(x,y,PCs[i])>MAX_OPERATING_DISTANCE) {
			return false;
		}
	}
	return true;
}

bool Game::PartyMemberDied()
{
	for(unsigned int i=0; i<PCs.size(); i++) {
		if(PCs[i]->InternalFlags&IF_JUSTDIED)
			return true;
	}
	return false;
}

void Game::IncrementChapter()
{
	//clear statistics
	unsigned long chapter = 0;
	globals->Lookup("CHAPTER",chapter);
	globals->SetAt("CHAPTER",chapter+1);
}

void Game::ReputationAltered()
{
	for(unsigned int i=0; i<PCs.size(); i++) {
		PCs[i]->NewStat(IE_REPUTATION, Reputation, MOD_ABSOLUTE);
	}
}
