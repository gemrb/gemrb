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

#include "EnumFlags.h"

#include "Orientation.h"
#include "Region.h"
#include "Resource.h"

#include <cstdint>
#include <vector>

namespace GemRB {

//searchmap conversion bits

enum class PathMapFlags : uint8_t {
	UNMARKED = 0,
	IMPASSABLE = 0,
	PASSABLE = 1,
	TRAVEL = 2,
	NO_SEE = 4,
	SIDEWALL = 8,
	AREAMASK = (IMPASSABLE | PASSABLE | TRAVEL | NO_SEE | SIDEWALL),
	DOOR_OPAQUE = 16,
	DOOR_IMPASSABLE = 32,
	PC = 64,
	NPC = 128,
	ACTOR = (PC | NPC),
	DOOR = (DOOR_OPAQUE | DOOR_IMPASSABLE),
	NOTAREA = (ACTOR | DOOR),
	NOTDOOR = (ACTOR | AREAMASK),
	NOTACTOR = (DOOR | AREAMASK)
};

using NavmapPoint = Point;
using SearchmapPoint = Point;

struct PathNode {
	Point point;
	orient_t orient;
};
using Path = std::vector<PathNode>;

enum {
	PF_SIGHT = 1,
	PF_BACKAWAY = 2,
	PF_ACTORS_ARE_BLOCKING = 4
};


// Point-distance pair, used by the pathfinder's priority queue
// to sort nodes by their (heuristic) distance from the destination
class PQNode {
public:
	PQNode(Point p, float_t l) : point(p), dist(l) {};
	PQNode() : point(Point(0, 0)), dist(0) {};

	Point point;
	float_t dist;

	friend bool operator < (const PQNode &lhs, const PQNode &rhs) { return lhs.dist < rhs.dist;}
	friend bool operator > (const PQNode &lhs, const PQNode &rhs){ return rhs < lhs; }
	friend bool operator <= (const PQNode &lhs, const PQNode &rhs){ return !(lhs > rhs); }
	friend bool operator >= (const PQNode &lhs, const PQNode &rhs){ return !(lhs < rhs); }
	friend bool operator == (const PQNode &lhs, const PQNode &rhs) { return lhs.point == rhs.point; }
	friend bool operator != (const PQNode &lhs, const PQNode &rhs) { return !(lhs == rhs); }

};

}

#endif
