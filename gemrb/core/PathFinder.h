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
	PATH_MAP_UNMARKED = 0,
	PATH_MAP_IMPASSABLE = 0,
	PATH_MAP_PASSABLE = 1,
	PATH_MAP_TRAVEL = 2,
	PATH_MAP_NO_SEE = 4,
	PATH_MAP_SIDEWALL = 8,
	PATH_MAP_AREAMASK = 15,
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
	unsigned int x;
	unsigned int y;
	unsigned int orient;
};

typedef Point NavmapPoint;
typedef Point SearchmapPoint;

enum {
	PF_SIGHT = 1,
	PF_BACKAWAY = 2,
	PF_ACTORS_ARE_BLOCKING = 4
};


// Point-distance pair, used by the pathfinder's priority queue
// to sort nodes by their (heuristic) distance from the destination
class PQNode {
public:
	PQNode(Point p, double l) : point(p), dist(l) {};
	PQNode() : point(Point(0, 0)), dist(0) {};

	Point point;
	double dist;

	friend bool operator < (const PQNode &lhs, const PQNode &rhs) { return lhs.dist < rhs.dist;}
	friend bool operator > (const PQNode &lhs, const PQNode &rhs){ return rhs < lhs; }
	friend bool operator <= (const PQNode &lhs, const PQNode &rhs){ return !(lhs > rhs); }
	friend bool operator >= (const PQNode &lhs, const PQNode &rhs){ return !(lhs < rhs); }
	friend bool operator == (const PQNode &lhs, const PQNode &rhs) { return lhs.point == rhs.point; }
	friend bool operator != (const PQNode &lhs, const PQNode &rhs) { return !(lhs == rhs); }

};

}

#endif
