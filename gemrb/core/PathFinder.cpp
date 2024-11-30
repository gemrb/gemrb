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

#include "PathFinder.h"

#include "Debug.h"
#include "FibonacciHeap.h"
#include "GameData.h"
#include "Map.h"
#include "RNG.h"

#include "Scriptable/Actor.h"

#include <array>
#include <limits>

namespace GemRB {

constexpr size_t DEGREES_OF_FREEDOM = 4;
constexpr size_t RAND_DEGREES_OF_FREEDOM = 16;
constexpr unsigned int SEARCHMAP_SQUARE_DIAGONAL = 20; // sqrt(16 * 16 + 12 * 12)
constexpr std::array<char, DEGREES_OF_FREEDOM> dxAdjacent { { 1, 0, -1, 0 } };
constexpr std::array<char, DEGREES_OF_FREEDOM> dyAdjacent { { 0, 1, 0, -1 } };

// Cosines
constexpr std::array<float_t, RAND_DEGREES_OF_FREEDOM> dxRand { { 0.000, -0.383, -0.707, -0.924, -1.000, -0.924, -0.707, -0.383, 0.000, 0.383, 0.707, 0.924, 1.000, 0.924, 0.707, 0.383 } };
// Sines
constexpr std::array<float_t, RAND_DEGREES_OF_FREEDOM> dyRand { { 1.000, 0.924, 0.707, 0.383, 0.000, -0.383, -0.707, -0.924, -1.000, -0.924, -0.707, -0.383, 0.000, 0.383, 0.707, 0.924 } };

// Find the best path of limited length that brings us the farthest from d
Path Map::RunAway(const Point& s, const Point& d, int maxPathLength, bool backAway, const Actor* caller) const
{
	if (!caller || !caller->GetSpeed()) return {};
	Point p = s;
	float_t dx = s.x - d.x;
	float_t dy = s.y - d.y;
	char xSign = 1, ySign = 1;
	size_t tries = 0;
	NormalizeDeltas(dx, dy, float_t(gamedata->GetStepTime()) / caller->GetSpeed());
	if (std::abs(dx) <= 0.333 && std::abs(dy) <= 0.333) return {};
	while (SquaredDistance(p, s) < unsigned(maxPathLength * maxPathLength * SEARCHMAP_SQUARE_DIAGONAL * SEARCHMAP_SQUARE_DIAGONAL)) {
		Point rad(std::lround(p.x + 3 * xSign * dx), std::lround(p.y + 3 * ySign * dy));
		if (!(GetBlockedInRadius(rad, caller->circleSize) & PathMapFlags::PASSABLE)) {
			tries++;
			// Give up and call the pathfinder if backed into a corner
			// should we return nullptr instead, so we don't accidentally get closer to d?
			// it matches more closely the iwd beetles in ar1015, but is too restrictive — then they can't move at all
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
	return FindPath(s, p, caller->circleSize, caller->circleSize, flags, caller);
}

PathNode Map::RandomWalk(const Point& s, int size, int radius, const Actor* caller) const
{
	if (!caller || !caller->GetSpeed()) return {};
	NavmapPoint p = s;
	size_t i = RAND<size_t>(0, RAND_DEGREES_OF_FREEDOM - 1);
	float_t dx = 3 * dxRand[i];
	float_t dy = 3 * dyRand[i];

	NormalizeDeltas(dx, dy, float_t(gamedata->GetStepTime()) / caller->GetSpeed());
	size_t tries = 0;
	while (SquaredDistance(p, s) < unsigned(radius * radius * SEARCHMAP_SQUARE_DIAGONAL * SEARCHMAP_SQUARE_DIAGONAL)) {
		if (!(GetBlockedInRadius(p + Point(dx, dy), size) & PathMapFlags::PASSABLE)) {
			tries++;
			// Give up if backed into a corner
			if (tries > RAND_DEGREES_OF_FREEDOM) {
				return {};
			}
			// Random rotation
			i = RAND<size_t>(0, RAND_DEGREES_OF_FREEDOM - 1);
			dx = 3 * dxRand[i];
			dy = 3 * dyRand[i];
			NormalizeDeltas(dx, dy, float_t(gamedata->GetStepTime()) / caller->GetSpeed());
			p = s;
		} else {
			p.x += dx;
			p.y += dy;
		}
	}
	while (!(GetBlockedInRadius(p + Point(dx, dy), size) & (PathMapFlags::PASSABLE | PathMapFlags::ACTOR))) {
		p.x -= dx;
		p.y -= dy;
	}
	PathNode randomStep;
	const Size& mapSize = PropsSize();
	randomStep.point = Clamp(p, Point(1, 1), Point((mapSize.w - 1) * 16, (mapSize.h - 1) * 12));
	randomStep.orient = GetOrient(s, p);
	return randomStep;
}

Path Map::GetLinePath(const Point& start, int Steps, orient_t Orientation, int flags) const
{
	Point dest = start;

	float_t xoff, yoff, mult;
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

	return GetLinePath(start, dest, 2, Orientation, flags);
}

Path Map::GetLinePath(const Point& start, const Point& dest, int Speed, orient_t Orientation, int flags) const
{
	int Count = 0;
	int Max = Distance(start, dest);
	Point diff = dest - start;
	Path path;
	path.nodes.reserve(Max);
	path.AppendStep(PathNode { start, Orientation });
	auto StartNode = path.begin();
	for (int Steps = 0; Steps < Max; Steps++) {
		Point p;
		p.x = start.x + (diff.x * Steps / Max);
		p.y = start.y + (diff.y * Steps / Max);

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
			StartNode = path.AppendStep({ p, Orientation });
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

PathNode Map::GetLineEnd(const Point& p, int steps, orient_t orient) const
{
	PathNode lineEnd;
	lineEnd.point.x = p.x + steps * SEARCHMAP_SQUARE_DIAGONAL * dxRand[orient];
	lineEnd.point.y = p.y + steps * SEARCHMAP_SQUARE_DIAGONAL * dyRand[orient];
	const Size& mapSize = PropsSize();
	lineEnd.point = Clamp(lineEnd.point, Point(1, 1), Point((mapSize.w - 1) * 16, (mapSize.h - 1) * 12));
	lineEnd.orient = GetOrient(p, lineEnd.point);
	return lineEnd;
}

// Find a path from start to goal, ending at the specified distance from the
// target (the goal must be in sight of the end, if PF_SIGHT is specified)
Path Map::FindPath(const Point& s, const Point& d, unsigned int size, unsigned int minDistance, int flags, const Actor* caller) const
{
	TRACY(ZoneScoped);
	if (InDebugMode(DebugMode::PATHFINDER))
		Log(DEBUG, "FindPath", "s = {}, d = {}, caller = {}, dist = {}, size = {}",
		    s, d,
		    fmt::WideToChar { caller ? caller->GetShortName() : u"nullptr" },
		    minDistance, size);
	bool actorsAreBlocking = flags & PF_ACTORS_ARE_BLOCKING;

	// TODO: we could optimize this function further by doing everything in SearchmapPoint and converting at the end
	SearchmapPoint smptDest0 { d };
	NavmapPoint nmptDest = d;
	NavmapPoint nmptSource = s;
	if (!(GetBlockedInRadiusTile(smptDest0, size) & PathMapFlags::PASSABLE)) {
		// If the desired target is blocked, find the path
		// to the nearest reachable point.
		// Also avoid bumping a still actor out of its position,
		// but stop just before it
		orient_t direction = GetOrient(nmptDest, nmptSource);
		AdjustPositionDirected(nmptDest, direction, size);
	}

	if (nmptDest == nmptSource) return {};

	SearchmapPoint smptSource { nmptSource };
	SearchmapPoint smptDest { nmptDest };

	if (minDistance < size && !(GetBlockedInRadiusTile(smptDest, size) & (PathMapFlags::PASSABLE | PathMapFlags::ACTOR))) {
		Log(DEBUG, "FindPath", "{} can't fit in destination", fmt::WideToChar { caller ? caller->GetShortName() : u"nullptr" });
		return {};
	}

	const Size& mapSize = PropsSize();
	if (!mapSize.PointInside(smptSource)) return {};

	// Initialize data structures
	FibonacciHeap<PQNode> open;
	std::vector<bool> isClosed(mapSize.Area(), false);
	std::vector<NavmapPoint> parents(mapSize.Area(), Point(0, 0));
	std::vector<unsigned short> distFromStart(mapSize.Area(), std::numeric_limits<unsigned short>::max());
	distFromStart[smptSource.y * mapSize.w + smptSource.x] = 0;
	parents[smptSource.y * mapSize.w + smptSource.x] = nmptSource;
	open.emplace(PQNode(nmptSource, 0));
	bool foundPath = false;
	static bool usePlainThetaStar = gamedata->GetMiscRule("LAZY_THETA_STAR") == 0;
	unsigned int squaredMinDist = minDistance * minDistance;

	// Weighted heuristic. Finds sub-optimal paths but should be quite a bit faster
	constexpr float_t HEURISTIC_WEIGHT = 1.5;
	auto getHeuristic = [&](const SearchmapPoint& smptChild, const int& smptChildIdx) {
		// Calculate heuristic
		int xDist = smptChild.x - smptDest.x;
		int yDist = smptChild.y - smptDest.y;
		// Tie-breaking used to smooth out the path
		int dxCross = smptDest.x - smptSource.x;
		int dyCross = smptDest.y - smptSource.y;
		int crossProduct = std::abs(xDist * dyCross - yDist * dxCross) >> 3;
		double distance = std::hypot(xDist, yDist);
		double heuristic = HEURISTIC_WEIGHT * (distance + crossProduct);
		double estDist = distFromStart[smptChildIdx] + heuristic;
		return estDist;
	};

	while (!open.empty()) {
		NavmapPoint nmptCurrent = open.top().point;
		open.pop();
		SearchmapPoint smptCurrent { nmptCurrent };
		int smptCurrentIdx = smptCurrent.y * mapSize.w + smptCurrent.x;
		if (parents[smptCurrentIdx].IsZero()) {
			continue;
		}

		if (smptCurrent == smptDest) {
			nmptDest = nmptCurrent;
			foundPath = true;
			break;
		} else if (minDistance &&
			   parents[smptCurrentIdx] != nmptCurrent &&
			   SquaredDistance(nmptCurrent, nmptDest) < squaredMinDist &&
			   (!(flags & PF_SIGHT) || IsVisibleLOS(smptCurrent, smptDest0))) { // FIXME: should probably be smptDest
			smptDest = smptCurrent;
			nmptDest = nmptCurrent;
			foundPath = true;
			break;
		}
		isClosed[smptCurrentIdx] = true;

		for (size_t i = 0; i < DEGREES_OF_FREEDOM; i++) {
			NavmapPoint nmptChild(nmptCurrent.x + 16 * dxAdjacent[i], nmptCurrent.y + 12 * dyAdjacent[i]);
			SearchmapPoint smptChild { nmptChild };
			// Outside map
			if (smptChild.x < 0 || smptChild.y < 0 || smptChild.x >= mapSize.w || smptChild.y >= mapSize.h) continue;
			// Already visited
			int smptChildIdx = smptChild.y * mapSize.w + smptChild.x;
			if (isClosed[smptChildIdx]) continue;

			PathMapFlags childBlockStatus;
			if (size > 2) {
				childBlockStatus = GetBlockedInRadiusTile(smptChild, size);
			} else {
				childBlockStatus = GetBlockedTile(smptChild);
			}
			bool childBlocked = !(childBlockStatus & (PathMapFlags::PASSABLE | PathMapFlags::ACTOR));
			if (childBlocked) continue;

			// If there's an actor, check it can be bumped away
			const Actor* childActor = GetActor(nmptChild, GA_NO_DEAD | GA_NO_UNSCHEDULED);
			bool childIsUnbumpable = childActor && childActor != caller && (actorsAreBlocking || !childActor->ValidTarget(GA_ONLY_BUMPABLE));
			if (childIsUnbumpable) continue;

			SearchmapPoint smptCurrent2 { nmptCurrent };
			NavmapPoint nmptParent = parents[smptCurrent2.y * mapSize.w + smptCurrent2.x];
			SearchmapPoint smptParent { nmptParent };
			unsigned short oldDist = distFromStart[smptChildIdx];

			if (usePlainThetaStar) {
				// Theta-star path if there is LOS
				if (IsWalkableTo(smptParent, smptChild, actorsAreBlocking, caller)) {
					unsigned short newDist = distFromStart[smptParent.y * mapSize.w + smptParent.x] + Distance(smptParent, smptChild);
					if (newDist < oldDist) {
						parents[smptChildIdx] = nmptParent;
						distFromStart[smptChildIdx] = newDist;
					}
					// Fall back to A-star path
				} else {
					unsigned short newDist = distFromStart[smptCurrent2.y * mapSize.w + smptCurrent2.x] + Distance(smptCurrent2, smptChild);
					if (newDist < oldDist) {
						parents[smptChildIdx] = nmptCurrent;
						distFromStart[smptChildIdx] = newDist;
					}
				}

				if (distFromStart[smptChildIdx] < oldDist) {
					PQNode newNode(nmptChild, getHeuristic(smptChild, smptChildIdx));
					open.emplace(newNode);
				}
			} else {
				// Lazy Theta star*
				unsigned short newDist = distFromStart[smptParent.y * mapSize.w + smptParent.x] + Distance(smptParent, smptChild);
				if (newDist < oldDist) {
					parents[smptChildIdx] = nmptParent;
					distFromStart[smptChildIdx] = newDist;
				}

				if (distFromStart[smptChildIdx] < oldDist) {
					// Theta-star path if there is LOS
					if (!IsWalkableTo(smptParent, smptChild, actorsAreBlocking, caller)) {
						// Fall back to A-star path
						distFromStart[smptChildIdx] = std::numeric_limits<unsigned short>::max();
						// Find already visited neighbour with shortest: path from start + path to child
						for (size_t j = 0; j < DEGREES_OF_FREEDOM; j++) {
							NavmapPoint nmptVis(nmptChild.x + 16 * dxAdjacent[j], nmptChild.y + 12 * dyAdjacent[j]);
							SearchmapPoint smptVis { nmptVis };
							// Outside map
							if (smptVis.x < 0 || smptVis.y < 0 || smptVis.x >= mapSize.w || smptVis.y >= mapSize.h) continue;
							// Only consider already visited
							if (!isClosed[smptVis.y * mapSize.w + smptVis.x]) continue;

							unsigned short oldVisDist = distFromStart[smptChildIdx];
							newDist = distFromStart[smptVis.y * mapSize.w + smptVis.x] + Distance(smptVis, smptChild);
							if (newDist < oldVisDist) {
								parents[smptChildIdx] = nmptVis;
								distFromStart[smptChildIdx] = newDist;
							}
						}
						if (distFromStart[smptChildIdx] >= oldDist) continue;
					}

					PQNode newNode(nmptChild, getHeuristic(smptChild, smptChildIdx));
					open.emplace(newNode);
				}
			}
		}
	}

	if (foundPath) {
		Path resultPath;
		NavmapPoint nmptCurrent = nmptDest;
		NavmapPoint nmptParent;
		SearchmapPoint smptCurrent { nmptCurrent };
		while (!resultPath || nmptCurrent != parents[smptCurrent.y * mapSize.w + smptCurrent.x]) {
			nmptParent = parents[smptCurrent.y * mapSize.w + smptCurrent.x];
			PathNode newStep { nmptCurrent, S };
			// movement in general allows characters to walk backwards given that
			// the destination is behind the character (within a threshold), and
			// that the distance isn't too far away
			// we approximate that with a relaxed collinearity check and intentionally
			// skip the first step, otherwise it doesn't help with iwd beetles in ar1015
			if (flags & PF_BACKAWAY && resultPath && std::abs(area2(nmptCurrent, resultPath.GetStep(0).point, nmptParent)) < 300) {
				newStep.orient = GetOrient(nmptCurrent, nmptParent);
			} else {
				newStep.orient = GetOrient(nmptParent, nmptCurrent);
			}

			resultPath.PrependStep(newStep);
			nmptCurrent = nmptParent;

			smptCurrent = SearchmapPoint(nmptCurrent);
		}
		return resultPath;
	} else if (InDebugMode(DebugMode::PATHFINDER)) {
		if (caller) {
			Log(DEBUG, "FindPath", "Pathing failed for {}", fmt::WideToChar { caller->GetShortName() });
		} else {
			Log(DEBUG, "FindPath", "Pathing failed");
		}
	}

	return {};
}

void Map::NormalizeDeltas(float_t& dx, float_t& dy, float_t factor)
{
	constexpr float_t STEP_RADIUS = 2.0;

	float_t ySign = std::copysign(1.0, dy);
	float_t xSign = std::copysign(1.0, dx);
	dx = std::fabs(dx);
	dy = std::fabs(dy);
	float_t dxOrig = dx;
	float_t dyOrig = dy;
	if (dx == 0.0) {
		dy = STEP_RADIUS * 0.75f;
	} else if (dy == 0.0) {
		dx = STEP_RADIUS;
	} else {
		float_t q = STEP_RADIUS / std::hypot(dx, dy);
		dx = dx * q;
		dy = dy * q * 0.75f;
	}
	dx = std::min(dx * factor, dxOrig);
	dy = std::min(dy * factor, dyOrig);
	dx = std::ceil(dx) * xSign;
	dy = std::ceil(dy) * ySign;
}

}
