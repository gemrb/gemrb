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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Game.cpp,v 1.5 2003/12/15 09:20:24 balrog994 Exp $
 *
 */

#include "../../includes/win32def.h"
#include "Game.h"

Game::Game(void)
{
}

Game::~Game(void)
{
	/*for(size_t i = 0; i < PCs.size(); i++) {
		delete(PCs[i].actor);
	}*/
	for(size_t i = 0; i < Maps.size(); i++) {
		delete(Maps[i]);
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
