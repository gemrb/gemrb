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
#include "Map.h"
#include "PathFinder.h"
#include "RNG.h"
#include "Scriptable/Actor.h"

#include "win32def.h"

#include <array>
#include <cmath>
#include <limits>

namespace GemRB {

constexpr std::array<char, Map::DEGREES_OF_FREEDOM> Map::dx{{1, 0, -1, 0}};
constexpr std::array<char, Map::DEGREES_OF_FREEDOM> Map::dy{{0, 1, 0, -1}};


// Find the best path of limited length that brings us the farthest from dX, dY
// The 5th parameter is controlling the orientation of the actor
// 0 - back away, 1 - face direction
PathNode* Map::RunAway(const Point &s, const Point &d, unsigned int size, unsigned int PathLen, int noBackAway)
{
	SearchmapPoint smptFarthest = FindFarthest(d, size, PathLen);
	NavmapPoint nmptFarthest = NavmapPoint(smptFarthest.x * 16, smptFarthest.y * 12);
	int flags = PF_SIGHT;
	if (!noBackAway) flags |= PF_BACKAWAY;
	return FindPath(s, nmptFarthest, size, 0, flags);
}

SearchmapPoint Map::FindFarthest(const NavmapPoint &d, unsigned int size, unsigned int pathLength, int validFlags) const
{
	std::vector<float> dist(Width * Height);
	SearchmapPoint smptFleeFrom = SearchmapPoint(d.x / 16, d.y / 12);
	std::priority_queue<PQNode> open;
	static const float diagWeight = sqrt(2);
	dist[smptFleeFrom.y * Width + smptFleeFrom.x] = 0;
	open.push(PQNode(smptFleeFrom, 0));
	SearchmapPoint smptFarthest;
	SearchmapPoint smptCurrent;
	SearchmapPoint smptChild = smptFleeFrom;

	float farthestDist = 0;
	PQNode newNode = PQNode();
	while (!open.empty()) {
		smptCurrent = open.top().point;
		open.pop();
		// Randomly search clockwise or counterclockwise
		// To prevent always fleeing in the same direction
		int sgn = (RAND(0, 1)) ? -1 : 1;
		for (size_t i = 0; i < DEGREES_OF_FREEDOM; i++) {
			smptChild.x = smptCurrent.x + sgn * dx[i];
			smptChild.y = smptCurrent.y + sgn * dy[i];
			bool childOutsideMap =
				smptChild.x <= 0 ||
				smptChild.y <= 0 ||
				(unsigned) smptChild.x >= Width ||
				(unsigned) smptChild.y >= Height;

			bool childBlocked = GetBlockedInRadius(smptChild.x * 16 + 8, smptChild.y * 12 + 8, false, size) & validFlags;
			if (!childBlocked && !childOutsideMap) {
				float curDist = dist[smptCurrent.y * Width + smptCurrent.x];
				float oldDist = dist[smptChild.y * Width + smptChild.x];
				float newDist = curDist + (dx[i] && dy[i] ? diagWeight : 1);
				if (newDist <= pathLength && newDist > oldDist) {
					if (newDist > farthestDist) {
						farthestDist = newDist;
						smptFarthest = smptChild;
					}
					dist[smptChild.y * Width + smptChild.x] = newDist;
					newNode.point.x = smptChild.x;
					newNode.point.y = smptChild.y;
					newNode.dist = newDist;
					open.push(newNode);
				}
			}
		}
	}
	return smptFarthest;
}

bool Map::TargetUnreachable(const Point &s, const Point &d, unsigned int size, bool actorsAreBlocking)
{
	int flags = PF_SIGHT;
	if (actorsAreBlocking) flags |= PF_ACTORS_ARE_BLOCKING;
	return FindPath(s, d, size, 0, flags) == NULL;
}

// Use this function when you target something by a straight line projectile (like a lightning bolt, arrow, etc)
PathNode* Map::GetLine(const Point &start, const Point &dest, int flags)
{
	int Orientation = GetOrient(start, dest);
	return GetLine(start, dest, 1, Orientation, flags);
}

PathNode* Map::GetLine(const Point &start, int Steps, int Orientation, int flags)
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

PathNode* Map::GetLine(const Point &start, const Point &dest, int Speed, int Orientation, int flags)
{
	PathNode *StartNode = new PathNode;
	PathNode *Return = StartNode;
	StartNode->Next = NULL;
	StartNode->Parent = NULL;
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
			StartNode->Next = NULL;
			Count = Speed;
		} else {
			Count--;
		}

		StartNode->x = p.x;
		StartNode->y = p.y;
		StartNode->orient = Orientation;
		bool wall = GetBlocked(p.x / 16, p.y / 12, true) & (PATH_MAP_DOOR_IMPASSABLE | PATH_MAP_SIDEWALL);
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

void Map::UpdateVertex(const NavmapPoint &s, const NavmapPoint &d, const NavmapPoint &nmptCurrent, const NavmapPoint &nmptChild,
		std::vector<unsigned short> &distFromStart, std::vector<NavmapPoint> &parents, FibonacciHeap<PQNode> &open, const Actor *caller) const
{
	// Weighted heuristic. Finds sub-optimal paths but should be quite a bit faster
	const float HEURISTIC_WEIGHT = 1.5;
	SearchmapPoint smptChild(nmptChild.x / 16, nmptChild.y / 12);
	SearchmapPoint smptCurrent(nmptCurrent.x / 16, nmptCurrent.y / 12);
	NavmapPoint nmptParent = parents[smptCurrent.y * Width + smptCurrent.x];
	unsigned short oldDist = distFromStart[smptChild.y * Width + smptChild.x];
	if (IsWalkableTo(nmptParent, nmptChild, false, caller)) {
		SearchmapPoint smptParent(nmptParent.x / 16, nmptParent.y / 12);
		unsigned short newDist = distFromStart[smptParent.y * Width + smptParent.x] + Distance(smptParent, smptChild);
		if (newDist < oldDist) {
			parents[smptChild.y * Width + smptChild.x] = nmptParent;
			distFromStart[smptChild.y * Width + smptChild.x] = newDist;
		}
	} else if (IsWalkableTo(nmptCurrent, nmptChild, false, caller)) {
		unsigned short newDist = distFromStart[smptCurrent.y * Width + smptCurrent.x] + Distance(smptCurrent, smptChild);
		if (newDist < oldDist) {
			parents[smptChild.y * Width + smptChild.x] = nmptCurrent;
			distFromStart[smptChild.y * Width + smptChild.x] = newDist;
		}
	}

	if (distFromStart[smptChild.y * Width + smptChild.x] < oldDist) {
		SearchmapPoint smptDest(d.x  / 16, d.y / 12);
		// Calculate heuristic
		int xDist = smptChild.x - smptDest.x;
		int yDist = smptChild.y - smptDest.y;
		// Tie-breaking used to smooth out the path
		SearchmapPoint smptSource(s.x  / 16, s.y / 12);
		int dxCross = smptDest.x - smptSource.x;
		int dyCross = smptDest.y - smptSource.y;
		int crossProduct = std::abs(xDist * dyCross - yDist * dxCross);
		unsigned int heuristic = HEURISTIC_WEIGHT * (Distance(smptChild, smptDest) + (crossProduct >> 9));
		unsigned int estDist = distFromStart[smptChild.y * Width + smptChild.x] + heuristic;
		PQNode newNode(nmptChild, estDist);
		open.emplace(newNode);
	}
}

// Find a path from start to goal, ending at the specified distance from the
// target (the goal must be in sight of the end, if PF_SIGHT is specified)
PathNode *Map::FindPath(const Point &s, const Point &d, unsigned int size, unsigned int minDistance, int flags, const Actor *caller)
{
	NavmapPoint nmptDest = d;
	NavmapPoint nmptSource = s;
	if (!(GetBlockedInRadius(d.x, d.y, true, size) & PATH_MAP_PASSABLE)) {
		// If the desired target is blocked, find the path
		// to the nearest reachable point.
		// Also avoid bumping a still actor out of its position,
		// but stop just before it
		AdjustPositionNavmap(nmptDest);
	}
	SearchmapPoint smptSource(nmptSource.x / 16, nmptSource.y / 12);
	SearchmapPoint smptDest(nmptDest.x / 16, nmptDest.y / 12);

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
			int xDist = nmptCurrent.x - d.x;
			int yDist = nmptCurrent.y - d.y;
			unsigned int squaredDist = xDist * xDist + yDist * yDist;
			if (parents[smptCurrent.y * Width + smptCurrent.x] != nmptCurrent &&
					squaredDist < squaredMinDist) {
				if (!(flags & PF_SIGHT) || IsWalkableTo(nmptCurrent, smptDest, flags & PF_ACTORS_ARE_BLOCKING, caller)) {
					smptDest = smptCurrent;
					foundPath = true;
					break;
				}
			}
		}
		isClosed[smptCurrent.y * Width + smptCurrent.x] = true;

		for (size_t i = 0; i < DEGREES_OF_FREEDOM; i++) {
			NavmapPoint nmptChild(nmptCurrent.x + 16 * dx[i], nmptCurrent.y + 12 * dy[i]);
			SearchmapPoint smptChild(nmptChild.x / 16, nmptChild.y / 12);
			// Outside map
			if (smptChild.x <= 0 ||	smptChild.y <= 0 || (unsigned) smptChild.x >= Width || (unsigned) smptChild.y >= Height) continue;
			// Already visited
			if (isClosed[smptChild.y * Width + smptChild.x]) continue;
			// If there's an actor, check it can be bumped away
			if (flags & PF_ACTORS_ARE_BLOCKING) {
				Actor* childActor = GetActor(nmptChild, GA_NO_DEAD|GA_NO_UNSCHEDULED);
				bool childIsUnbumpable = childActor && childActor != caller && !childActor->ValidTarget(GA_ONLY_BUMPABLE);
				if (childIsUnbumpable) continue;
			}

			unsigned childBlockStatus = GetBlockedInRadius(nmptChild.x, nmptChild.y, false, size);
			bool childBlocked = !(childBlockStatus & (PATH_MAP_PASSABLE|PATH_MAP_ACTOR));
			if (!childBlocked) UpdateVertex(nmptSource, nmptDest, nmptCurrent, nmptChild, distFromStart, parents, open, caller);
		}
	}

	if (foundPath) {
		return BuildActorPath(nmptDest, parents, flags & PF_BACKAWAY);
	}

	return NULL;
}

PathNode *Map::BuildActorPath(const NavmapPoint &nmptDest, const std::vector<NavmapPoint> &parents, bool backAway) const {
	PathNode *resultPath = NULL;
	NavmapPoint nmptCurrent = nmptDest;
	NavmapPoint nmptParent;
	SearchmapPoint smptCurrent(nmptCurrent.x / 16, nmptCurrent.y / 12);
	while (!resultPath || nmptCurrent != parents[smptCurrent.y * Width + smptCurrent.x]) {
		nmptParent = parents[smptCurrent.y * Width + smptCurrent.x];
		PathNode *newStep = new PathNode;
		newStep->x = nmptCurrent.x;
		newStep->y = nmptCurrent.y;
		newStep->Next = resultPath;
		newStep->Parent = NULL;
		if (backAway) {
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
