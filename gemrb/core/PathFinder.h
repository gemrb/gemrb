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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */
#ifndef PATHFINDER_H
#define PATHFINDER_H

namespace GemRB {

//searchmap conversion bits

enum {
	PATH_MAP_IMPASSABLE = 0,
	PATH_MAP_PASSABLE = 1,
	PATH_MAP_TRAVEL = 2,
	PATH_MAP_NO_SEE = 4,
	PATH_MAP_SIDEWALL = 8,
	PATH_MAP_AREAMASK = 15,
	PATH_MAP_FREE = 0,
	PATH_MAP_DOOR_OPAQUE = 16,
	PATH_MAP_DOOR_IMPASSABLE = 32,
	PATH_MAP_PC = 64,
	PATH_MAP_NPC = 128,
	PATH_MAP_ACTOR = (PATH_MAP_PC|PATH_MAP_NPC),
	PATH_MAP_DOOR = (PATH_MAP_DOOR_OPAQUE|PATH_MAP_DOOR_IMPASSABLE),
	PATH_MAP_NOTAREA = (PATH_MAP_ACTOR|PATH_MAP_DOOR),
	PATH_MAP_NOTDOOR = (PATH_MAP_ACTOR|PATH_MAP_AREAMASK),
	PATH_MAP_NOTACTOR = (PATH_MAP_DOOR|PATH_MAP_AREAMASK)
};

struct PathNode {
	PathNode* Parent;
	PathNode* Next;
	unsigned short x;
	unsigned short y;
	unsigned int orient;
};

}

#endif
