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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Game.h,v 1.2 2003/12/09 20:54:48 balrog994 Exp $
 *
 */

#ifndef GAME_H
#define GAME_H

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

#include <vector>
#include "Map.h"

class GEM_EXPORT Game
{
public:
	Game(void);
	~Game(void);
private:
	std::vector<ActorBlock*> PCs;
	std::vector<Map*> Maps;
public:
	ActorBlock* GetPC(int slot);
	int SetPC(ActorBlock *pc);
	int DelPC(int slot, bool autoFree = false);
	Map * GetMap(int index);
	int AddMap(Map* map);
	int DelMap(int index, bool autoFree = false);
};

#endif
