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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Game.cpp,v 1.8 2004/01/05 23:52:11 balrog994 Exp $
 *
 */

#include "../../includes/win32def.h"
#include "Game.h"
#include "MapMgr.h"
#include "DataStream.h"
#include "Interface.h"

extern Interface * core;

Game::Game(void)
{
}

Game::~Game(void)
{
	for(size_t i = 0; i < Maps.size(); i++) {
		delete(Maps[i]);
	}
	for(size_t i = 0; i < PCs.size(); i++) {
		delete(PCs[i]);
	}
	for(size_t i = 0; i < NPCs.size(); i++) {
		delete(NPCs[i]);
	}
}

Actor* Game::GetPC(unsigned int slot)
{
	if(slot >= PCs.size())
		return NULL;
	return PCs[slot];
}
int Game::SetPC(Actor *pc)
{
	PCs.push_back(pc);
	return (int)PCs.size()-1;
}
int Game::DelPC(unsigned int slot, bool autoFree)
{
	if(slot >= PCs.size())
		return -1;
	if(!PCs[slot])
		return -1;
	if(autoFree)
		delete(PCs[slot]);
	PCs[slot] = NULL;
	return 0;
}
Map * Game::GetMap(unsigned int index)
{
	if(index >= Maps.size())
		return NULL;
	return Maps[index];
}
int Game::AddMap(Map* map)
{
	for(size_t i = 0; i < Maps.size(); i++) {
		if(!Maps[i]) {
			Maps[i] = map;
			return (int)i;
		}
	}
	Maps.push_back(map);
	return (int)Maps.size()-1;
}
int Game::DelMap(unsigned int index, bool autoFree)
{
	if(index >= Maps.size())
		return -1;
	if(!Maps[index])
		return -1;
	if(autoFree)
		delete(Maps[index]);
	Maps[index] = NULL;
	return 0;
}

int Game::LoadMap(char *ResRef)
{
	MapMgr * mM = (MapMgr*)core->GetInterface(IE_ARE_CLASS_ID);
	DataStream * ds = core->GetResourceMgr()->GetResource(ResRef, IE_ARE_CLASS_ID);
	mM->Open(ds, true);
	Map * newMap = mM->GetMap();
	core->FreeInterface(mM);
	if(!newMap)
		return -1;
	for(int i = 0; i < NPCs.size(); i++) {
		if(stricmp(NPCs[i]->Area, ResRef) == 0)
			newMap->AddActor(NPCs[i]);
	}
	return AddMap(newMap);
}

int Game::AddNPC(Actor *npc)
{
	NPCs.push_back(npc);
	return NPCs.size()-1;
}

Actor* Game::GetNPC(unsigned int Index)
{
	if(Index >= NPCs.size())
		return NULL;
	return NPCs[Index];
}
