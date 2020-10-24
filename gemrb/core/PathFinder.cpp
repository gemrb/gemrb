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

#include "win32def.h"

#include <array>
#include <cmath>
#include <limits>

namespace GemRB {

constexpr size_t DEGREES_OF_FREEDOM = 4;
constexpr size_t RAND_DEGREES_OF_FREEDOM = 16;
constexpr unsigned int SEARCHMAP_SQUARE_DIAGONAL = 20; // sqrt(16 * 16 + 12 * 12)
constexpr std::array<char, DEGREES_OF_FREEDOM> dxAdjacent{{1, 0, -1, 0}};
constexpr std::array<char, DEGREES_OF_FREEDOM> dyAdjacent{{0, 1, 0, -1}};

// Cosines
constexpr std::array<double, RAND_DEGREES_OF_FREEDOM> dxRand{{0.000, -0.383, -0.707, -0.924, -1.000, -0.924, -0.707, -0.383, 0.000, 0.383, 0.707, 0.924, 1.000, 0.924, 0.707, 0.383}};
// Sines
constexpr std::array<double, RAND_DEGREES_OF_FREEDOM> dyRand{{1.000, 0.924, 0.707, 0.383, 0.000, -0.383, -0.707, -0.924, -1.000, -0.924, -0.707, -0.383, 0.000, 0.383, 0.707, 0.924}};

// Find the best path of limited length that brings us the farthest from d
PathNode *Map::RunAway(const Point &s, const Point &d, unsigned int size, int maxPathLength, bool backAway, const Actor *caller) const
{
	if (!caller || !caller->GetSpeed()) return nullptr;
	Point p = s;
	double dx = s.x - d.x;
	double dy = s.y - d.y;
	char xSign = 1, ySign = 1;
	size_t tries = 0;
	NormalizeDeltas(dx, dy, double(gamedata->GetStepTime()) / caller->GetSpeed());
	while (SquaredDistance(p, s) < unsigned(maxPathLength * maxPathLength * SEARCHMAP_SQUARE_DIAGONAL * SEARCHMAP_SQUARE_DIAGONAL)) {
		if (!(GetBlockedInRadius(std::lround(p.x + 3 * xSign * dx), std::lround(p.y + 3 * ySign * dy), size) & PATH_MAP_PASSABLE)) {
			tries++;
			// Give up and call the pathfinder if backed into a corner
			if (tries > RAND_DEGREES_OF_FREEDOM) break;
			// Random rotation
			xSign = RAND(0, 1) ? -1 : 1;
			ySign = RAND(0, 1) ? -1 : 1;

		}
		p.x += dx;
		p.y += dy;
	}
	int flags = PF_SIGHT;
	if (backAway) flags |= PF_BACKAWAY;
	return FindPath(s, p, size, size, flags, caller);
}

PathNode *Map::RandomWalk(const Point &s, int size, int radius, const Actor *caller) const
{
	if (!caller || !caller->GetSpeed()) return nullptr;
	NavmapPoint p = s;
	size_t i = RAND(0, RAND_DEGREES_OF_FREEDOM - 1);
	double dx = 3 * dxRand[i];
	double dy = 3 * dyRand[i];

	NormalizeDeltas(dx, dy, double(gamedata->GetStepTime()) / caller->GetSpeed());
	size_t tries = 0;
	while (SquaredDistance(p, s) < unsigned(radius * radius * SEARCHMAP_SQUARE_DIAGONAL * SEARCHMAP_SQUARE_DIAGONAL)) {
		if (!(GetBlockedInRadius(p.x + dx, p.y + dx, size) & PATH_MAP_PASSABLE)) {
			tries++;
			// Give up if backed into a corner
			if (tries > RAND_DEGREES_OF_FREEDOM) {
				return nullptr;
			}
			// Random rotation
			i = RAND(0, RAND_DEGREES_OF_FREEDOM - 1);
			dx = 3 * dxRand[i];
			dy = 3 * dyRand[i];
			NormalizeDeltas(dx, dy, double(gamedata->GetStepTime()) / caller->GetSpeed());
			p = s;
		} else {
			p.x += dx;
			p.y += dy;
		}
	}
	while (!(GetBlockedInRadius(p.x + dx, p.y + dx, size) & (PATH_MAP_PASSABLE|PATH_MAP_ACTOR))) {

		p.x -= dx;
		p.y -= dy;
	}
	PathNode *step = new PathNode;
	step->x = p.x;
	step->y = p.y;
	step->x = Clamp(step->x, 1u, (Width - 1) * 16);
	step->y = Clamp(step->y, 1u, (Height - 1) * 12);
	step->Parent = nullptr;
	step->Next = nullptr;
	step->orient = GetOrient(p, s);
	return step;
}

bool Map::TargetUnreachable(const Point &s, const Point &d, unsigned int size, bool actorsAreBlocking) const
{
	int flags = PF_SIGHT;
	if (actorsAreBlocking) flags |= PF_ACTORS_ARE_BLOCKING;
	PathNode *path = FindPath(s, d, size, 0, flags);
	bool targetUnreachable = path == nullptr;
	if (!targetUnreachable) {
		PathNode *thisNode = path;
		while (thisNode) {
			PathNode *nextNode = thisNode->Next;
			delete(thisNode);
			thisNode = nextNode;
		}
	}
	return targetUnreachable;
}

// Use this function when you target something by a straight line projectile (like a lightning bolt, arrow, etc)
PathNode *Map::GetLine(const Point &start, const Point &dest, int flags) const
{
	int Orientation = GetOrient(start, dest);
	return GetLine(start, dest, 1, Orientation, flags);
}

PathNode *Map::GetLine(const Point &start, int Steps, int Orientation, int flags) const
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

PathNode *Map::GetLine(const Point &start, const Point &dest, int Speed, int Orientation, int flags) const
{
	PathNode *StartNode = new PathNode;
	PathNode *Return = StartNode;
	StartNode->Next = nullptr;
	StartNode->Parent = nullptr;
	StartNode->x = start.x;
	StartNode->y = start.y;
	StartNode->orient = Orientation;

	int Count = 0;
	int Max = Distance(start, dest);
	for (int Steps = 0; Steps < Max; Steps++) {
		Point p;
		p.x = (ieWord) start.x + ((dest.x - start.x) * Steps / Max);
		p.y = (ieWord) start.y + ((dest.y - start.y) * Steps / Max);

		//the path ends here as it would go off the screen, causing problems
		//maybe there is a better way, but i needed a quick hack to fix
		//the crash in projectiles
		if ((signed) p.x < 0 || (signed) p.y < 0) {
			return Return;
		}
		if ((ieWord) p.x > Width * 16 || (ieWord) p.y > Height * 12) {
			return Return;
		}

		if (!Count) {
			StartNode->Next = new PathNode;
			StartNode->Next->Parent = StartNode;
			StartNode = StartNode->Next;
			StartNode->Next = nullptr;
			Count = Speed;
		} else {
			Count--;
		}

		StartNode->x = p.x;
		StartNode->y = p.y;
		StartNode->orient = Orientation;
		bool wall = GetBlocked(p.x / 16, p.y / 12) & (PATH_MAP_DOOR_IMPASSABLE | PATH_MAP_SIDEWALL);
		if (wall) switch (flags) {
			case GL_REBOUND:
				Orientation = (Orientation + 8) & 15;
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

PathNode *Map::GetLine(const Point &p, int steps, unsigned int orient) const
{
	PathNode *step = new PathNode;
	step->x = p.x + steps * SEARCHMAP_SQUARE_DIAGONAL * dxRand[orient];
	step->y = p.y + steps * SEARCHMAP_SQUARE_DIAGONAL * dyRand[orient];
	step->x = Clamp(step->x, 1u, (Width - 1) * 16);
	step->y = Clamp(step->y, 1u, (Height - 1) * 12);
	step->orient = GetOrient(Point(step->x, step->y), p);
	step->Next = nullptr;
	step->Parent = nullptr;
	return step;
}

// Find a path from start to goal, ending at the specified distance from the
// target (the goal must be in sight of the end, if PF_SIGHT is specified)
PathNode *Map::FindPath(const Point &s, const Point &d, unsigned int size, unsigned int minDistance, int flags, const Actor *caller) const
{
	Log(DEBUG, "FindPath", "s = (%d, %d), d = (%d, %d), caller = %s, dist = %d, size = %d", s.x, s.y, d.x, d.y, caller ? caller->GetName(0) : "nullptr", minDistance, size);
	NavmapPoint nmptDest = d;
	NavmapPoint nmptSource = s;
	if (!(GetBlockedInRadius(d.x, d.y, size) & PATH_MAP_PASSABLE)) {
		// If the desired target is blocked, find the path
		// to the nearest reachable point.
		// Also avoid bumping a still actor out of its position,
		// but stop just before it
		AdjustPositionNavmap(nmptDest);
	}
	if (minDistance < size && !(GetBlockedInRadius(nmptDest.x, nmptDest.y, size) & (PATH_MAP_PASSABLE | PATH_MAP_ACTOR))) {
		Log(DEBUG, "FindPath", "%s can't fit in destination", caller ? caller->GetName(0) : "nullptr");
		return nullptr;
	}
	SearchmapPoint smptSource(nmptSource.x / 16, nmptSource.y / 12);
	SearchmapPoint smptDest(nmptDest.x / 16, nmptDest.y / 12);
	if (smptDest == smptSource) return nullptr;

	// Initialize data structures
	FibonacciHeap<PQNode> open;
	std::vector<bool> isClosed(Width * Height, false);
	std::vector<NavmapPoint> parents(Width * Height, Point(0, 0));
	std::vector<unsigned short> distFromStart(Width * Height, std::numeric_limits<unsigned short>::max());
	distFromStart[smptSource.y * Width + smptSource.x] = 0;
	parents[smptSource.y * Width + smptSource.x] = nmptSource;
	open.emplace(PQNode(nmptSource, 0));
	bool foundPath = false;
	unsigned int squaredMinDist = minDistance * minDistance;

	while (!open.empty()) {
		NavmapPoint nmptCurrent = open.top().point;
		open.pop();
		SearchmapPoint smptCurrent(nmptCurrent.x / 16, nmptCurrent.y / 12);
		if (parents[smptCurrent.y * Width + smptCurrent.x] == Point(0, 0)) {
			continue;
		}

		if (smptCurrent == smptDest) {
			nmptDest = nmptCurrent;
			foundPath = true;
			break;
		} else if (minDistance) {
			if (parents[smptCurrent.y * Width + smptCurrent.x] != nmptCurrent &&
					SquaredDistance(nmptCurrent, nmptDest) < squaredMinDist) {
				if (!(flags & PF_SIGHT) || IsVisibleLOS(nmptCurrent, d)) {
					smptDest = smptCurrent;
					nmptDest = nmptCurrent;
					foundPath = true;
					break;
				}
			}
		}
		isClosed[smptCurrent.y * Width + smptCurrent.x] = true;

		for (size_t i = 0; i < DEGREES_OF_FREEDOM; i++) {
			NavmapPoint nmptChild(nmptCurrent.x + 16 * dxAdjacent[i], nmptCurrent.y + 12 * dyAdjacent[i]);
			SearchmapPoint smptChild(nmptChild.x / 16, nmptChild.y / 12);
			// Outside map
			if (smptChild.x < 0 ||	smptChild.y < 0 || (unsigned) smptChild.x >= Width || (unsigned) smptChild.y >= Height) continue;
			// Already visited
			if (isClosed[smptChild.y * Width + smptChild.x]) continue;
			// If there's an actor, check it can be bumped away
			Actor* childActor = GetActor(nmptChild, GA_NO_DEAD|GA_NO_UNSCHEDULED);
			bool childIsUnbumpable = childActor && childActor != caller && (flags & PF_ACTORS_ARE_BLOCKING || !childActor->ValidTarget(GA_ONLY_BUMPABLE));
			if (childIsUnbumpable) continue;

			unsigned childBlockStatus = GetBlockedInRadius(nmptChild.x, nmptChild.y, size);
			bool childBlocked = !(childBlockStatus & (PATH_MAP_PASSABLE | PATH_MAP_ACTOR | PATH_MAP_TRAVEL));
			if (childBlocked) continue;

			// Weighted heuristic. Finds sub-optimal paths but should be quite a bit faster
			const float HEURISTIC_WEIGHT = 1.5;
			SearchmapPoint smptCurrent(nmptCurrent.x / 16, nmptCurrent.y / 12);
			NavmapPoint nmptParent = parents[smptCurrent.y * Width + smptCurrent.x];
			unsigned short oldDist = distFromStart[smptChild.y * Width + smptChild.x];
			// Theta-star path if there is LOS
			if (IsWalkableTo(nmptParent, nmptChild, flags & PF_ACTORS_ARE_BLOCKING, caller)) {
				SearchmapPoint smptParent(nmptParent.x / 16, nmptParent.y / 12);
				unsigned short newDist = distFromStart[smptParent.y * Width + smptParent.x] + Distance(smptParent, smptChild);
				if (newDist < oldDist) {
					parents[smptChild.y * Width + smptChild.x] = nmptParent;
					distFromStart[smptChild.y * Width + smptChild.x] = newDist;
				}
			// Fall back to A-star path
			} else if (IsWalkableTo(nmptCurrent, nmptChild, flags & PF_ACTORS_ARE_BLOCKING, caller)) {
				unsigned short newDist = distFromStart[smptCurrent.y * Width + smptCurrent.x] + Distance(smptCurrent, smptChild);
				if (newDist < oldDist) {
					parents[smptChild.y * Width + smptChild.x] = nmptCurrent;
					distFromStart[smptChild.y * Width + smptChild.x] = newDist;
				}
			}

			if (distFromStart[smptChild.y * Width + smptChild.x] < oldDist) {
				// Calculate heuristic
				int xDist = smptChild.x - smptDest.x;
				int yDist = smptChild.y - smptDest.y;
				// Tie-breaking used to smooth out the path
				int dxCross = smptDest.x - smptSource.x;
				int dyCross = smptDest.y - smptSource.y;
				int crossProduct = std::abs(xDist * dyCross - yDist * dxCross) >> 3;
				double distance = std::sqrt(xDist * xDist + yDist * yDist);
				double heuristic = HEURISTIC_WEIGHT * (distance + crossProduct);
				double estDist = distFromStart[smptChild.y * Width + smptChild.x] + heuristic;
				PQNode newNode(nmptChild, estDist);
				open.emplace(newNode);
			}
		}
	}

	if (foundPath) {
		PathNode *resultPath = nullptr;
		NavmapPoint nmptCurrent = nmptDest;
		NavmapPoint nmptParent;
		SearchmapPoint smptCurrent(nmptCurrent.x / 16, nmptCurrent.y / 12);
		while (!resultPath || nmptCurrent != parents[smptCurrent.y * Width + smptCurrent.x]) {
			nmptParent = parents[smptCurrent.y * Width + smptCurrent.x];
			PathNode *newStep = new PathNode;
			newStep->x = nmptCurrent.x;
			newStep->y = nmptCurrent.y;
			newStep->Next = resultPath;
			newStep->Parent = nullptr;
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

			smptCurrent.x = nmptCurrent.x / 16;
			smptCurrent.y = nmptCurrent.y / 12;
		}
		return resultPath;
	} else if (caller) {
		Log(DEBUG, "FindPath", "Pathing failed for %s", caller->GetName(0));
	} else {
		Log(DEBUG, "FindPath", "Pathing failed");
	}

	return nullptr;
}

void Map::NormalizeDeltas(double &dx, double &dy, const double &factor)
{
	const double STEP_RADIUS = 2.0;
	double dxOrig;
	double dyOrig;
	char ySign;
	char xSign;
	ySign = dy > 0 ? 1 : (dy < 0 ? -1 : 0);
	xSign = dx > 0 ? 1 : (dx < 0 ? -1 : 0);
	dx = std::abs(dx);
	dy = std::abs(dy);
	dxOrig = dx;
	dyOrig = dy;
	if (dx == 0) {
		dy = STEP_RADIUS;
	} else if (dy == 0) {
		dx = STEP_RADIUS;
	} else {
		double squaredRadius = dx * dx + dy * dy;
		double ratio = (STEP_RADIUS * STEP_RADIUS) / squaredRadius;
		dx = sqrt(dx * dx * ratio);
		// Speed on the y axis is downscaled (12 / 16) in order to correct searchmap scaling and avoid curved paths
		dy = sqrt(dy * dy * ratio) * 0.75;
	}
	dx = std::min(dx * factor, dxOrig);
	dy = std::min(dy * factor, dyOrig);
	dx = std::ceil(dx) * xSign;
	dy = std::ceil(dy) * ySign;
}
}
