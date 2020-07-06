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
// implementation of the lazy Theta* algorithm, see Daniel et al., 2010
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

#include <cmath>
#include <limits>

namespace GemRB {

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
	static const size_t DEGREES_OF_FREEDOM = 8;
	static const char dx[DEGREES_OF_FREEDOM] = {1, 0, -1, 0, 1, 1, -1, -1};
	static const char dy[DEGREES_OF_FREEDOM] = {0, 1, 0, -1, 1, -1, 1, -1};
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
			bool childBlocked = CheckNavmapPointFlags(smptChild.x * 16 + 8, smptChild.y * 12 + 6, size,
					validFlags, false);
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

// Find a path from start to goal, ending at the specified distance from the
// target (the goal must be in sight of the end, if PF_SIGHT is specified)
PathNode *Map::FindPath(const Point &s, const Point &d, unsigned int size, unsigned int minDistance, int flags)
{
	static const unsigned short MAX_PATH_COST = std::numeric_limits<unsigned short>::max();
	static const size_t DEGREES_OF_FREEDOM = 8;
	static const char dx[DEGREES_OF_FREEDOM] = {1, 0, -1, 0, 1, 1, -1, -1};
	static const char dy[DEGREES_OF_FREEDOM] = {0, 1, 0, -1, 1, -1, 1, -1};
	// Weighted heuristic. Finds sub-optimal paths but should be quite a bit faster
	static const float weight = 1.5;
	SearchmapPoint smptSource;
	SearchmapPoint smptDest;
	smptSource.x = s.x / 16;
	smptSource.y = s.y / 12;
	smptDest.x = d.x / 16;
	smptDest.y = d.y / 12;

	Actor *caller = GetActor(s, 0);

	if (size && CheckNavmapPointFlags(d.x, d.y, size, PATH_MAP_PASSABLE, true)) {
		// We don't want to bump a still npc on our target position, we want to stop before
		AdjustPosition(smptDest);
	}
	// Initialize data structures
	FibonacciHeap<PQNode> open;
	std::vector<bool> isClosed(Width * Height);
	std::vector<SearchmapPoint> parents(Width * Height);
	std::vector<unsigned short> distFromStart(Width * Height);

	int dxCross = smptDest.x - smptSource.x;
	int dyCross = smptDest.y - smptSource.y;

	bool foundPath = false;
	parents[smptSource.y * Width + smptSource.x] = smptSource;
	open.emplace(PQNode(smptSource, 0));
	unsigned int squaredMinDist = minDistance * minDistance;

	SearchmapPoint smptCurrent;
	SearchmapPoint smptChild;
	SearchmapPoint smptParent;
	SearchmapPoint smptBestParent;
	SearchmapPoint smptCurNeighbor;
	NavmapPoint nmptCurrent;
	NavmapPoint nmptCurrentAdjusted;
	NavmapPoint nmptParent;
	PQNode newNode;

	while (!open.empty()) {
		smptCurrent = open.top().point;
		open.pop();
		nmptCurrent.x = smptCurrent.x * 16;
		nmptCurrent.y = smptCurrent.y * 12;
		// This adjustment is needed to prevent quantum tunneling through walls
		// Because the check is not made in the same place the character is rendered at
		// It could look like actors can walk through impassable blocks (e.g. ar2600 [1483.379] in BG1)
		// But the behavior is the same in the original game: actors are actually on the border
		nmptCurrentAdjusted.x = nmptCurrent.x + 8;
		nmptCurrentAdjusted.y = nmptCurrent.y + 6;

		if (smptCurrent == smptDest) {
			foundPath = true;
			break;
		} else if (minDistance) {
			int xDist = nmptCurrentAdjusted.x - d.x;
			int yDist = nmptCurrentAdjusted.y - d.y;
			unsigned int squaredDist = xDist * xDist + yDist * yDist;
			if (parents[smptCurrent.y * Width + smptCurrent.x] != smptCurrent &&
					squaredDist < squaredMinDist) {
				if (!(flags & PF_SIGHT) || IsVisibleLOS(nmptCurrent, d)) {
					smptDest = smptCurrent;
					foundPath = true;
					break;
				}
			}
		}

		// Lazy Theta-star optimization: does the LOS check after inserting a node
		smptParent = parents[smptCurrent.y * Width + smptCurrent.x];
		nmptParent.x = smptParent.x * 16;
		nmptParent.y = smptParent.y * 12;
		bool reachable = IsWalkableTo(nmptCurrent, nmptParent, flags & PF_ACTORS_ARE_BLOCKING);
		if (!reachable && smptCurrent != smptSource) {
			smptBestParent.x = 0;
			smptBestParent.y = 0;
			distFromStart[smptCurrent.y * Width + smptCurrent.x] = MAX_PATH_COST;
			for (size_t i = 0; i < DEGREES_OF_FREEDOM; i++) {
				smptCurNeighbor.x = smptCurrent.x + dx[i];
				smptCurNeighbor.y = smptCurrent.y + dy[i];
				if (isClosed[smptCurNeighbor.y * Width + smptCurNeighbor.x]) {
					unsigned short curParentDist = distFromStart[smptCurNeighbor.y * Width + smptCurNeighbor.x] +
						Distance(smptCurrent, smptCurNeighbor);
					if (curParentDist < distFromStart[smptCurrent.y * Width + smptCurrent.x]) {
						smptBestParent = smptCurNeighbor;
						distFromStart[smptCurrent.y * Width + smptCurrent.x] = curParentDist;
					}
				}
				parents[smptCurrent.y * Width + smptCurrent.x] = smptBestParent;
			}
		}
		isClosed[smptCurrent.y * Width + smptCurrent.x] = true;
		for (size_t i = 0; i < DEGREES_OF_FREEDOM; i++) {
			smptChild.x = smptCurrent.x + dx[i];
			smptChild.y = smptCurrent.y + dy[i];
			bool childOutsideMap =	smptChild.x <= 0 ||
				smptChild.y <= 0 ||
				(unsigned) smptChild.x >= Width ||
				(unsigned) smptChild.y >= Height;
			bool childBlocked = CheckNavmapPointFlags(smptChild.x * 16 + 8, smptChild.y * 12 + 6, size,
					PATH_MAP_PASSABLE, flags & PF_ACTORS_ARE_BLOCKING);
			Actor* childActor = GetActor(NavmapPoint(smptChild.x * 16 + 8, smptChild.y * 12 + 6), GA_NO_DEAD|GA_NO_UNSCHEDULED);
			bool childIsUnbumpable = childActor && childActor != caller && !childActor->ValidTarget(GA_ONLY_BUMPABLE);
			if (!childOutsideMap && !childBlocked && !isClosed[smptChild.y * Width + smptChild.x] && !childIsUnbumpable) {
				if (!distFromStart[smptChild.y * Width + smptChild.x] && smptChild != smptSource) {
					distFromStart[smptChild.y * Width + smptChild.x] = MAX_PATH_COST;
				}
				// Theta-star search
				smptParent = parents[smptCurrent.y * Width + smptCurrent.x];
				unsigned short adjParentDist = Distance(smptParent, smptChild);
				unsigned short oldDist = distFromStart[smptChild.y * Width + smptChild.x];
				unsigned short newDist = distFromStart[smptParent.y * Width + smptParent.x] + adjParentDist;
				if (newDist < oldDist && newDist < MAX_PATH_COST) {
					parents[smptChild.y * Width + smptChild.x] = smptParent;
					distFromStart[smptChild.y * Width + smptChild.x] = newDist;
					// Calculate heuristic
					int xDist = smptChild.x - smptDest.x;
					int yDist = smptChild.y - smptDest.y;
					// Tie-breaking used to smooth out the path
					int crossProduct = std::abs(xDist * dyCross - yDist * dxCross);
					unsigned int heuristic = weight * (Distance(smptChild, smptDest) + (crossProduct >> 9));
					unsigned int estDist = newDist + heuristic;
					newNode.point = smptChild;
					newNode.dist = estDist;
					open.emplace(newNode);
				}
			}
		}
	}

	if (foundPath) {
		return BuildActorPath(smptCurrent, smptDest, parents, flags & PF_BACKAWAY);
	}

	return NULL;
}

PathNode *Map::BuildActorPath(SearchmapPoint &smptCurrent, const SearchmapPoint &smptDest, 
		const std::vector<SearchmapPoint> &parents, bool backAway) const {
	PathNode *resultPath = NULL;
	smptCurrent = smptDest;
	while (!resultPath || smptCurrent != parents[smptCurrent.y * Width + smptCurrent.x]) {
		PathNode *newStep = new PathNode;
		newStep->x = smptCurrent.x;
		newStep->y = smptCurrent.y;
		newStep->Next = resultPath;
		newStep->Parent = NULL;
		if (backAway) {
			newStep->orient = GetOrient(parents[smptCurrent.y * Width + smptCurrent.x], smptCurrent);
		} else {
			newStep->orient = GetOrient(smptCurrent, parents[smptCurrent.y * Width + smptCurrent.x]);
		}
		if (resultPath) {
			resultPath->Parent = newStep;
		}
		resultPath = newStep;
		smptCurrent = parents[smptCurrent.y * Width + smptCurrent.x];
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
