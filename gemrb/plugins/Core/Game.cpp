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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Game.cpp,v 1.35 2004/04/15 16:20:15 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "Game.h"
#include "MapMgr.h"
#include "DataStream.h"
#include "Interface.h"

extern Interface* core;

Game::Game(void)
	: Scriptable( ST_GLOBAL )
{
	PartyGold = 0;
	SetScript( core->GlobalScript, 0 );
	MapIndex = -1;
	SelectedSingle = 0;
	globals = NULL;
	familiars = NULL;
}

Game::~Game(void)
{
	for (size_t i = 0; i < Maps.size(); i++) {
		delete( Maps[i] );
	}
	for (size_t i = 0; i < PCs.size(); i++) {
		delete ( PCs[i] );
	}
	for (size_t i = 0; i < NPCs.size(); i++) {
		delete ( NPCs[i] );
	}
	if (globals) {
		delete globals;
	}
	if (familiars) {
		free (familiars);
	}
	size_t i=Journals.size();
	while(i--) {
		delete Journals[i];
	}
}

int Game::FindPlayer(unsigned int partyID)
{
	for(unsigned int slot=0; slot<PCs.size(); slot++) {
		if(PCs[slot]->InParty==partyID) return slot;
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

Map* Game::GetMap(unsigned int index)
{
	if (index >= Maps.size()) {
		return NULL;
	}
	return Maps[index];
}

Map *Game::GetCurrentMap()
{
	return GetMap(MapIndex);
}

int Game::AddMap(Map* map)
{
	for (size_t i = 0; i < Maps.size(); i++) {
		if (!Maps[i]) {
			Maps[i] = map;
			return ( int ) i;
		}
	}
	Maps.push_back( map );
	return ( int ) Maps.size() - 1;
}

int Game::DelMap(unsigned int index, bool autoFree)
{
	if (index >= Maps.size()) {
		return -1;
	}
	if (!Maps[index]) {
		return -1;
	}
	if (autoFree) {
		delete( Maps[index] );
	}
	Maps[index] = NULL;
	return 0;
}

int Game::LoadMap(char* ResRef)
{
	MapMgr* mM = ( MapMgr* ) core->GetInterface( IE_ARE_CLASS_ID );
	DataStream* ds = core->GetResourceMgr()->GetResource( ResRef,
												IE_ARE_CLASS_ID );
	mM->Open( ds, true );
	Map* newMap = mM->GetMap();
	core->FreeInterface( mM );
	if (!newMap) {
		return -1;
	}
	for (unsigned int i = 0; i < PCs.size(); i++) {
		if (stricmp( PCs[i]->Area, ResRef ) == 0) {
			newMap->AddActor( PCs[i] );
		}
	}
	for (unsigned int i = 0; i < NPCs.size(); i++) {
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
	for(int i=0; i<PartySize; i++) {
		if (PCs[i]->GetStat(IE_STATE_ID)&STATE_DEAD) {
			continue;
		}
		PCs[i]->NewStat(IE_XP,xp,0);
	}
}
