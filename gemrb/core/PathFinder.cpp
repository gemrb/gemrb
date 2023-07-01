/* GemRB - Engine Made with preRendered Background
 * Copyright (C) 2020 The GemRB Project
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

// This file implements the pathfinding logic for actors
// The main logic is in Map::FindPath, which is an
// implementation of the Theta* algorithm, see Daniel et al., 2010
// GemRB uses two overlaid representation of the world: the searchmap and the navmap.
// Pathfinding is done on the searchmap and movement is done on the navmap.
// The navmap is bigger than the searchmap by a factor of (16, 12) on the (x, y) axes.
// Traditional, A* based pathfinding done on the searchmap would constrain movement
// to 45-degree angles and not take advantage of the navmap's higher resolution.
// Compared to A*, Theta* relaxes the constraint that two subsequent nodes in a
// path should be adjacent, only requiring them to be visible and for a straight-line
// path to exist. This allows for actors to move at any angle instead of being constrained
// by the searchmap grid. This also means that some paths are shorter than those found
// by A*.
// Moving to each node in the path thus becomes an automatic regulation problem
// which is solved with a P regulator, see Scriptable.cpp

#include "FibonacciHeap.h"
#include "GameData.h"
#include "Map.h"
#include "PathFinder.h"
#include "RNG.h"
#include "Scriptable/Actor.h"

#include <array>
#include <limits>

namespace GemRB {

constexpr size_t DEGREES_OF_FREEDOM = 8;
constexpr size_t RAND_DEGREES_OF_FREEDOM = 16;
constexpr unsigned int SEARCHMAP_SQUARE_DIAGONAL = 20; // sqrt(16 * 16 + 12 * 12)
constexpr std::array<char, DEGREES_OF_FREEDOM> dxAdjacent{{1, 0, -1, 0}};
constexpr std::array<char, DEGREES_OF_FREEDOM> dyAdjacent{{0, 1, 0, -1}};
//constexpr std::array<char, DEGREES_OF_FREEDOM> dxAdjacent{{1, 0, -1, 0, 1, 1, -1,	 -1}};
//constexpr std::array<char, DEGREES_OF_FREEDOM> dyAdjacent{{0, 1, 0, -1, 1, -1, -1, 1}};

// Cosines
constexpr std::array<double, RAND_DEGREES_OF_FREEDOM> dxRand{{0.000, -0.383, -0.707, -0.924, -1.000, -0.924, -0.707, -0.383, 0.000, 0.383, 0.707, 0.924, 1.000, 0.924, 0.707, 0.383}};
// Sines
constexpr std::array<double, RAND_DEGREES_OF_FREEDOM> dyRand{{1.000, 0.924, 0.707, 0.383, 0.000, -0.383, -0.707, -0.924, -1.000, -0.924, -0.707, -0.383, 0.000, 0.383, 0.707, 0.924}};

// Find the best path of limited length that brings us the farthest from d
PathListNode *Map::RunAway(const Point &s, const Point &d, unsigned int size, int maxPathLength, bool backAway, const Actor *caller) const
{
	if (!caller || !caller->GetSpeed()) return nullptr;
	Point p = s;
	double dx = s.x - d.x;
	double dy = s.y - d.y;
	char xSign = 1, ySign = 1;
	size_t tries = 0;
	NormalizeDeltas(dx, dy, double(gamedata->GetStepTime()) / caller->GetSpeed());
	while (SquaredDistance(p, s) < unsigned(maxPathLength * maxPathLength * SEARCHMAP_SQUARE_DIAGONAL * SEARCHMAP_SQUARE_DIAGONAL)) {
		Point rad(std::lround(p.x + 3 * xSign * dx), std::lround(p.y + 3 * ySign * dy));
		if (!(GetBlockedInRadius(rad, size) & PathMapFlags::PASSABLE)) {
			tries++;
			// Give up and call the pathfinder if backed into a corner
			if (tries > RAND_DEGREES_OF_FREEDOM) break;
			// Random rotation
			xSign = RandomFlip() ? -1 : 1;
			ySign = RandomFlip() ? -1 : 1;
			continue;
		}
		p = rad;
	}
	int flags = PF_SIGHT;
	if (backAway) flags |= PF_BACKAWAY;
	return FindPath(s, p, size, size, flags, caller);
}

PathListNode *Map::RandomWalk(const Point &s, int size, int radius, const Actor *caller) const
{
	if (!caller || !caller->GetSpeed()) return nullptr;
	NavmapPoint p = s;
	size_t i = RAND<size_t>(0, RAND_DEGREES_OF_FREEDOM - 1);
	double dx = 3 * dxRand[i];
	double dy = 3 * dyRand[i];

	NormalizeDeltas(dx, dy, double(gamedata->GetStepTime()) / caller->GetSpeed());
	size_t tries = 0;
	while (SquaredDistance(p, s) < unsigned(radius * radius * SEARCHMAP_SQUARE_DIAGONAL * SEARCHMAP_SQUARE_DIAGONAL)) {
		if (!(GetBlockedInRadius(p + Point(dx, dy), size) & PathMapFlags::PASSABLE)) {
			tries++;
			// Give up if backed into a corner
			if (tries > RAND_DEGREES_OF_FREEDOM) {
				return nullptr;
			}
			// Random rotation
			i = RAND<size_t>(0, RAND_DEGREES_OF_FREEDOM - 1);
			dx = 3 * dxRand[i];
			dy = 3 * dyRand[i];
			NormalizeDeltas(dx, dy, double(gamedata->GetStepTime()) / caller->GetSpeed());
			p = s;
		} else {
			p.x += dx;
			p.y += dy;
		}
	}
	while (!(GetBlockedInRadius(p + Point(dx, dy), size) & (PathMapFlags::PASSABLE|PathMapFlags::ACTOR))) {
		p.x -= dx;
		p.y -= dy;
	}
	PathListNode *step = new PathListNode;
	const Size& mapSize = PropsSize();
	step->point = Clamp(p, Point(1, 1), Point((mapSize.w - 1) * 16, (mapSize.h - 1) * 12));
	step->orient = GetOrient(p, s);
	return step;
}

bool Map::TargetUnreachable(const Point &s, const Point &d, unsigned int size, bool actorsAreBlocking) const
{
	int flags = PF_SIGHT;
	if (actorsAreBlocking) flags |= PF_ACTORS_ARE_BLOCKING;
	PathListNode *path = FindPath(s, d, size, 0, flags);
	bool targetUnreachable = path == nullptr;
	if (!targetUnreachable) {
		PathListNode *thisNode = path;
		while (thisNode) {
			PathListNode *nextNode = thisNode->Next;
			delete thisNode;
			thisNode = nextNode;
		}
	}
	return targetUnreachable;
}

// Use this function when you target something by a straight line projectile (like a lightning bolt, arrow, etc)
PathListNode *Map::GetLine(const Point &start, const Point &dest, int flags) const
{
	orient_t Orientation = GetOrient(start, dest);
	return GetLine(start, dest, 1, Orientation, flags);
}

PathListNode *Map::GetLine(const Point &start, int Steps, orient_t Orientation, int flags) const
{
	Point dest = start;

	double xoff, yoff, mult;
	if (Orientation <= 4) {
		xoff = -Orientation / 4.0;
	} else if (Orientation <= 12) {
		xoff = -1.0 + (Orientation - 4) / 4.0;
	} else {
		xoff = 1.0 - (Orientation - 12) / 4.0;
	}

	if (Orientation <= 8) {
		yoff = 1.0 - Orientation / 4.0;
	} else {
		yoff = -1.0 + (Orientation - 8) / 4.0;
	}

	mult = 1.0 / std::max(std::fabs(xoff), std::fabs(yoff));

	dest.x += Steps * mult * xoff + 0.5;
	dest.y += Steps * mult * yoff + 0.5;

	return GetLine(start, dest, 2, Orientation, flags);
}

PathListNode *Map::GetLine(const Point &start, const Point &dest, int Speed, orient_t Orientation, int flags) const
{
	PathListNode *StartNode = new PathListNode;
	PathListNode *Return = StartNode;
	StartNode->point = start;
	StartNode->orient = Orientation;

	int Count = 0;
	int Max = Distance(start, dest);
	for (int Steps = 0; Steps < Max; Steps++) {
		Point p;
		p.x = start.x + ((dest.x - start.x) * Steps / Max);
		p.y = start.y + ((dest.y - start.y) * Steps / Max);

		//the path ends here as it would go off the screen, causing problems
		//maybe there is a better way, but i needed a quick hack to fix
		//the crash in projectiles
		if (p.x < 0 || p.y < 0) {
			return Return;
		}
		
		const Size& mapSize = PropsSize();
		if (p.x > mapSize.w * 16 || p.y > mapSize.h * 12) {
			return Return;
		}

		if (!Count) {
			StartNode->Next = new PathListNode;
			StartNode->Next->Parent = StartNode;
			StartNode = StartNode->Next;
			Count = Speed;
		} else {
			Count--;
		}

		StartNode->point = p;
		StartNode->orient = Orientation;
		bool wall = bool(GetBlocked(p) & (PathMapFlags::DOOR_IMPASSABLE | PathMapFlags::SIDEWALL));
		if (wall) switch (flags) {
			case GL_REBOUND:
				Orientation = ReflectOrientation(Orientation);
				// TODO: recalculate dest (mirror it)
				break;
			case GL_PASS:
				break;
			default: //premature end
				return Return;
		}
	}

	return Return;
}

Path Map::GetLinePath(const Point &start, const Point &dest, int Speed, orient_t Orientation, int flags) const
{
	int Count = 0;
	int Max = Distance(start, dest);
	Path path;
	path.reserve(Max);
	path.push_back(PathNode {start, Orientation});
	auto StartNode = path.begin();
	for (int Steps = 0; Steps < Max; Steps++) {
		Point p;
		p.x = start.x + ((dest.x - start.x) * Steps / Max);
		p.y = start.y + ((dest.y - start.y) * Steps / Max);

		//the path ends here as it would go off the screen, causing problems
		//maybe there is a better way, but i needed a quick hack to fix
		//the crash in projectiles
		if (p.x < 0 || p.y < 0) {
			return path;
		}
		
		const Size& mapSize = PropsSize();
		if (p.x > mapSize.w * 16 || p.y > mapSize.h * 12) {
			return path;
		}

		if (!Count) {
			StartNode = path.insert(path.end(), {p, Orientation});
			Count = Speed;
		} else {
			Count--;
			StartNode->point = p;
			StartNode->orient = Orientation;
		}

		bool wall = bool(GetBlocked(p) & (PathMapFlags::DOOR_IMPASSABLE | PathMapFlags::SIDEWALL));
		if (wall) switch (flags) {
			case GL_REBOUND:
				Orientation = ReflectOrientation(Orientation);
				// TODO: recalculate dest (mirror it)
				break;
			case GL_PASS:
				break;
			default: //premature end
				return path;
		}
	}

	return path;
}

PathListNode *Map::GetLine(const Point &p, int steps, orient_t orient) const
{
	PathListNode *step = new PathListNode;
	step->point.x = p.x + steps * SEARCHMAP_SQUARE_DIAGONAL * dxRand[orient];
	step->point.y = p.y + steps * SEARCHMAP_SQUARE_DIAGONAL * dyRand[orient];
	const Size& mapSize = PropsSize();
	step->point = Clamp(step->point, Point(1, 1), Point((mapSize.w - 1) * 16, (mapSize.h - 1) * 12));
	step->orient = GetOrient(step->point, p);
	return step;
}

// Find a path from start to goal, ending at the specified distance from the
// target (the goal must be in sight of the end, if PF_SIGHT is specified)
PathListNode *Map::FindPath(const Point &s, const Point &d, unsigned int size, unsigned int minDistance, int flags, const Actor *caller) const
{
	if (core->InDebugMode(ID_PATHFINDER)) Log(DEBUG, "FindPath", "s = {}, d = {}, caller = {}, dist = {}, size = {}", s, d, caller ? MBStringFromString(caller->GetShortName()) : "nullptr", minDistance, size);
	
	// TODO: we could optimize this function further by doing everything in SearchmapPoint and converting at the end
	NavmapPoint nmptDest = d;
	NavmapPoint nmptSource = s;
	if (!(GetBlockedInRadius(d, size) & PathMapFlags::PASSABLE)) {
		// If the desired target is blocked, find the path
		// to the nearest reachable point.
		// Also avoid bumping a still actor out of its position,
		// but stop just before it
		AdjustPositionNavmap(nmptDest);
	}
	
	if (nmptDest == nmptSource) return nullptr;
	
	SearchmapPoint smptSource = Map::ConvertCoordToTile(nmptSource);
	SearchmapPoint smptDest = Map::ConvertCoordToTile(nmptDest);
	
	if (minDistance < size && !(GetBlockedInRadiusTile(smptDest, size) & (PathMapFlags::PASSABLE | PathMapFlags::ACTOR))) {
		Log(DEBUG, "FindPath", "{} can't fit in destination", caller ? MBStringFromString(caller->GetShortName()) : "nullptr");
		return nullptr;
	}

	const Size& mapSize = PropsSize();
	if (!mapSize.PointInside(smptSource)) return nullptr;

	// Initialize data structures
	FibonacciHeap<PQNode> open;
	std::vector<bool> isClosed(mapSize.Area(), false);
	std::vector<NavmapPoint> parents(mapSize.Area(), Point(0, 0));
	std::vector<unsigned short> distFromStart(mapSize.Area(), std::numeric_limits<unsigned short>::max());
	distFromStart[smptSource.y * mapSize.w + smptSource.x] = 0;
	parents[smptSource.y * mapSize.w + smptSource.x] = nmptSource;
	open.emplace(PQNode(nmptSource, 0));
	bool foundPath = false;
	unsigned int squaredMinDist = minDistance * minDistance;

	while (!open.empty()) {
		NavmapPoint nmptCurrent = open.top().point;
		open.pop();
		SearchmapPoint smptCurrent = Map::ConvertCoordToTile(nmptCurrent);
		if (parents[smptCurrent.y * mapSize.w + smptCurrent.x].IsZero()) {
			continue;
		}

		if (smptCurrent == smptDest) {
			nmptDest = nmptCurrent;
			foundPath = true;
			break;
		} else if (minDistance) {
			if (parents[smptCurrent.y * mapSize.w + smptCurrent.x] != nmptCurrent &&
					SquaredDistance(nmptCurrent, nmptDest) < squaredMinDist) {
				if (!(flags & PF_SIGHT) || IsVisibleLOS(nmptCurrent, d)) {
					smptDest = smptCurrent;
					nmptDest = nmptCurrent;
					foundPath = true;
					break;
				}
			}
		}
		isClosed[smptCurrent.y * mapSize.w + smptCurrent.x] = true;

		for (size_t i = 0; i < DEGREES_OF_FREEDOM; i++) {
			NavmapPoint nmptChild(nmptCurrent.x + 16 * dxAdjacent[i], nmptCurrent.y + 12 * dyAdjacent[i]);
			SearchmapPoint smptChild = Map::ConvertCoordToTile(nmptChild);
			// Outside map
			if (smptChild.x < 0 ||	smptChild.y < 0 || smptChild.x >= mapSize.w || smptChild.y >= mapSize.h) continue;
			// Already visited
			if (isClosed[smptChild.y * mapSize.w + smptChild.x]) continue;
			// If there's an actor, check it can be bumped away
			const Actor* childActor = GetActor(nmptChild, GA_NO_DEAD | GA_NO_UNSCHEDULED);
			bool childIsUnbumpable = childActor && childActor != caller && (flags & PF_ACTORS_ARE_BLOCKING || !childActor->ValidTarget(GA_ONLY_BUMPABLE));
			if (childIsUnbumpable) continue;

			PathMapFlags childBlockStatus = GetBlockedInRadius(nmptChild, size);
			bool childBlocked = !(childBlockStatus & (PathMapFlags::PASSABLE | PathMapFlags::ACTOR | PathMapFlags::TRAVEL));
			if (childBlocked) continue;

			// Weighted heuristic. Finds sub-optimal paths but should be quite a bit faster
			const float HEURISTIC_WEIGHT = 1.5;
			SearchmapPoint smptCurrent2 = Map::ConvertCoordToTile(nmptCurrent);
			NavmapPoint nmptParent = parents[smptCurrent2.y * mapSize.w + smptCurrent2.x];
			unsigned short oldDist = distFromStart[smptChild.y * mapSize.w + smptChild.x];
			// Theta-star path if there is LOS
			if (IsWalkableTo(nmptParent, nmptChild, flags & PF_ACTORS_ARE_BLOCKING, caller)) {
				SearchmapPoint smptParent = Map::ConvertCoordToTile(nmptParent);
				unsigned short newDist = distFromStart[smptParent.y * mapSize.w + smptParent.x] + Distance(smptParent, smptChild);
				if (newDist < oldDist) {
					parents[smptChild.y * mapSize.w + smptChild.x] = nmptParent;
					distFromStart[smptChild.y * mapSize.w + smptChild.x] = newDist;
				}
			// Fall back to A-star path
			} else if (IsWalkableTo(nmptCurrent, nmptChild, flags & PF_ACTORS_ARE_BLOCKING, caller)) {
				unsigned short newDist = distFromStart[smptCurrent2.y * mapSize.w + smptCurrent2.x] + Distance(smptCurrent2, smptChild);
				if (newDist < oldDist) {
					parents[smptChild.y * mapSize.w + smptChild.x] = nmptCurrent;
					distFromStart[smptChild.y * mapSize.w + smptChild.x] = newDist;
				}
			}

			if (distFromStart[smptChild.y * mapSize.w + smptChild.x] < oldDist) {
				// Calculate heuristic
				int xDist = smptChild.x - smptDest.x;
				int yDist = smptChild.y - smptDest.y;
				// Tie-breaking used to smooth out the path
				int dxCross = smptDest.x - smptSource.x;
				int dyCross = smptDest.y - smptSource.y;
				int crossProduct = std::abs(xDist * dyCross - yDist * dxCross) >> 3;
				double distance = std::hypot(xDist, yDist);
				double heuristic = HEURISTIC_WEIGHT * (distance + crossProduct);
				double estDist = distFromStart[smptChild.y * mapSize.w + smptChild.x] + heuristic;
				PQNode newNode(nmptChild, estDist);
				open.emplace(newNode);
			}
		}
	}

	if (foundPath) {
		PathListNode *resultPath = nullptr;
		NavmapPoint nmptCurrent = nmptDest;
		NavmapPoint nmptParent;
		SearchmapPoint smptCurrent = Map::ConvertCoordToTile(nmptCurrent);
		while (!resultPath || nmptCurrent != parents[smptCurrent.y * mapSize.w + smptCurrent.x]) {
			nmptParent = parents[smptCurrent.y * mapSize.w + smptCurrent.x];
			PathListNode *newStep = new PathListNode;
			newStep->point = nmptCurrent;
			newStep->Next = resultPath;
			if (flags & PF_BACKAWAY) {
				newStep->orient = GetOrient(nmptParent, nmptCurrent);
			} else {
				newStep->orient = GetOrient(nmptCurrent, nmptParent);
			}
			if (resultPath) {
				resultPath->Parent = newStep;
			}
			resultPath = newStep;
			nmptCurrent = nmptParent;

			smptCurrent = Map::ConvertCoordToTile(nmptCurrent);
		}
		return resultPath;
	} else if (core->InDebugMode(ID_PATHFINDER)) {
		if (caller) {
			Log(DEBUG, "FindPath", "Pathing failed for {}", fmt::WideToChar{caller->GetShortName()});
		} else {
			Log(DEBUG, "FindPath", "Pathing failed");
		}
	}

	return nullptr;
}

void Map::NormalizeDeltas(double &dx, double &dy, double factor)
{
	constexpr double STEP_RADIUS = 2.0;

	double ySign = std::copysign(1.0, dy);
	double xSign = std::copysign(1.0, dx);
	dx = std::fabs(dx);
	dy = std::fabs(dy);
	double dxOrig = dx;
	double dyOrig = dy;
	if (dx == 0.0) {
		dy = STEP_RADIUS;
	} else if (dy == 0.0) {
		dx = STEP_RADIUS;
	} else {
		double q = STEP_RADIUS / std::hypot(dx, dy);
		dx = dx * q;
		dy = dy * q * 0.75f;
	}
	dx = std::min(dx * factor, dxOrig);
	dy = std::min(dy * factor, dyOrig);
	dx = std::ceil(dx) * xSign;
	dy = std::ceil(dy) * ySign;
}

}
