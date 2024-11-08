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

struct PathNode {
	Point point;
	orient_t orient;
	bool waypoint = false;
};

struct Path {
	std::vector<PathNode> nodes;
	size_t currentStep = 0;
	using iterator = std::vector<PathNode>::iterator;
	using const_iterator = std::vector<PathNode>::const_iterator;

	explicit operator bool() const noexcept
	{
		return !nodes.empty();
	}
	bool Empty() const
	{
		return nodes.empty();
	}
	size_t Size() const
	{
		return nodes.size();
	}
	void Clear()
	{
		nodes.clear();
		currentStep = 0;
	}
	PathNode GetStep(size_t idx) const
	{
		return nodes[idx];
	}
	PathNode GetCurrentStep() const
	{
		return nodes[currentStep];
	}
	PathNode GetNextStep(size_t x) const
	{
		size_t next = currentStep + x;
		if (next < nodes.size()) {
			return nodes[next];
		} else {
			return {};
		}
	}
	iterator AppendStep(PathNode&& step)
	{
		nodes.push_back(std::move(step));
		return nodes.end() - 1;
	}
	void PrependStep(PathNode& step)
	{
		nodes.insert(nodes.begin(), std::move(step));
	}
	void AppendPath(const Path& path2)
	{
		nodes.insert(nodes.end(), path2.cbegin(), path2.cend());
	}
	iterator begin() noexcept
	{
		return nodes.begin();
	}
	iterator end() noexcept
	{
		return nodes.end();
	}
	const_iterator cbegin() const noexcept
	{
		return nodes.cbegin();
	}
	const_iterator cend() const noexcept
	{
		return nodes.cend();
	}
};
static_assert(std::is_nothrow_move_constructible<Path>::value, "Path should be noexcept MoveConstructible");

enum {
	PF_SIGHT = 1,
	PF_BACKAWAY = 2,
	PF_ACTORS_ARE_BLOCKING = 4
};


// Point-distance pair, used by the pathfinder's priority queue
// to sort nodes by their (heuristic) distance from the destination
class PQNode {
public:
	PQNode(Point p, float_t l)
		: point(p), dist(l) {};
	PQNode()
		: point(Point(0, 0)), dist(0) {};

	Point point;
	float_t dist;

	friend bool operator<(const PQNode& lhs, const PQNode& rhs) { return lhs.dist < rhs.dist; }
	friend bool operator>(const PQNode& lhs, const PQNode& rhs) { return rhs < lhs; }
	friend bool operator<=(const PQNode& lhs, const PQNode& rhs) { return !(lhs > rhs); }
	friend bool operator>=(const PQNode& lhs, const PQNode& rhs) { return !(lhs < rhs); }
	friend bool operator==(const PQNode& lhs, const PQNode& rhs) { return lhs.point == rhs.point; }
	friend bool operator!=(const PQNode& lhs, const PQNode& rhs) { return !(lhs == rhs); }
};

}

#endif
