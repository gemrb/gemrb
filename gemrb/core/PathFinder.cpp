// SPDX-FileCopyrightText: 2020 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

// This file implements the pathfinding logic for actors
// The main logic is in FindPath method, which is an
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

#include "BucketPriorityQueue.h"
#include "Debug.h"
#include "GameData.h"
#include "Map.h"
#include "RNG.h"

#include "Logging/Logging.h"
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

// Calculate a destination point for running away from d, starting at s.
// Returns false and leaves outPoint untouched if the actor is too slow or the deltas are too
// small; on success outPoint holds the destination. (0, 0) is a legal map coordinate, so
// success/failure cannot be encoded in the point itself.
bool PathFinder::CalculateRunAwayPoint(const TileProps& tileProps, const Point& s, const Point& d, int maxPathLength, int actorSpeed, int actorCircleSize, Point& outPoint)
{
	if (!actorSpeed) return false;
	Point p = s;
	float_t dx = s.x - d.x;
	float_t dy = s.y - d.y;
	char xSign = 1, ySign = 1;
	size_t tries = 0;
	NormalizeDeltas(dx, dy, float_t(gamedata->GetStepTime()) / actorSpeed);
	if (std::abs(dx) <= 0.333 && std::abs(dy) <= 0.333) return false;
	while (SquaredDistance(p, s) < unsigned(maxPathLength * maxPathLength * SEARCHMAP_SQUARE_DIAGONAL * SEARCHMAP_SQUARE_DIAGONAL)) {
		Point rad(std::lround(p.x + 3 * xSign * dx), std::lround(p.y + 3 * ySign * dy));
		if (!(GetBlockedInRadiusTile(tileProps, SearchmapPoint(rad), actorCircleSize) & PathMapFlags::PASSABLE)) {
			tries++;
			// Give up if backed into a corner
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
	outPoint = p;
	return true;
}

// Calculate a random walk destination within the specified radius.
// Returns false and leaves outStep untouched if the actor is too slow or gets stuck; on success
// outStep holds the destination and the orientation to face while walking there.
bool PathFinder::CalculateRandomWalkPoint(const TileProps& tileProps, const Point& s, int actorCircleSize, int radius, int actorSpeed, PathNode& outStep)
{
	if (!actorSpeed) return false;
	NavmapPoint p = s;
	size_t i = RAND<size_t>(0, RAND_DEGREES_OF_FREEDOM - 1);
	float_t dx = 3 * dxRand[i];
	float_t dy = 3 * dyRand[i];

	NormalizeDeltas(dx, dy, float_t(gamedata->GetStepTime()) / actorSpeed);
	size_t tries = 0;
	while (SquaredDistance(p, s) < unsigned(radius * radius * SEARCHMAP_SQUARE_DIAGONAL * SEARCHMAP_SQUARE_DIAGONAL)) {
		if (!(GetBlockedInRadiusTile(tileProps, SearchmapPoint(p + Point(dx, dy)), actorCircleSize) & PathMapFlags::PASSABLE)) {
			tries++;
			// Give up if backed into a corner
			if (tries > RAND_DEGREES_OF_FREEDOM) {
				return false;
			}
			// Random rotation
			i = RAND<size_t>(0, RAND_DEGREES_OF_FREEDOM - 1);
			dx = 3 * dxRand[i];
			dy = 3 * dyRand[i];
			NormalizeDeltas(dx, dy, float_t(gamedata->GetStepTime()) / actorSpeed);
			p = s;
		} else {
			p.x += dx;
			p.y += dy;
		}
	}
	while (!(GetBlockedInRadiusTile(tileProps, SearchmapPoint(p + Point(dx, dy)), actorCircleSize) & (PathMapFlags::PASSABLE | PathMapFlags::ACTOR))) {
		p.x -= dx;
		p.y -= dy;
	}
	const Size& mapSize = tileProps.GetSize();
	outStep.point = Clamp(p, Point(1, 1), Point((mapSize.w - 1) * 16, (mapSize.h - 1) * 12));
	outStep.orient = GetOrient(s, p);
	return true;
}

// Calculate a straight line path from start to dest
// Returns a path with nodes at specified speed intervals
Path PathFinder::CalculateLinePath(const TileProps& tileProps, const Point& start, const Point& dest, int Speed, orient_t Orientation, int flags)
{
	int Count = 0;
	int max = Distance(start, dest);
	Point diff = dest - start;
	Path path;
	path.nodes.reserve(max);
	path.AppendStep(PathNode { start, Orientation });
	auto startNode = path.begin();
	const Size& mapSize = tileProps.GetSize();

	for (int steps = 0; steps < max; steps++) {
		Point p;
		p.x = start.x + (diff.x * steps / max);
		p.y = start.y + (diff.y * steps / max);

		//the path ends here as it would go off the screen, causing problems
		//maybe there is a better way, but i needed a quick hack to fix
		//the crash in projectiles
		if (p.x < 0 || p.y < 0) {
			return path;
		}

		if (p.x > mapSize.w * 16 || p.y > mapSize.h * 12) {
			return path;
		}

		if (!Count) {
			startNode = path.AppendStep({ p, Orientation });
			Count = Speed;
		} else {
			Count--;
			startNode->point = p;
			startNode->orient = Orientation;
		}

		bool wall = bool(GetBlockedTile(tileProps, SearchmapPoint(p)) & (PathMapFlags::DOOR_IMPASSABLE | PathMapFlags::SIDEWALL));
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

// Calculate the end point of a line starting at p with given steps and orientation
PathNode PathFinder::CalculateLineEnd(const TileProps& tileProps, const Point& p, int steps, orient_t orient)
{
	PathNode lineEnd;
	lineEnd.point.x = p.x + steps * SEARCHMAP_SQUARE_DIAGONAL * dxRand[orient];
	lineEnd.point.y = p.y + steps * SEARCHMAP_SQUARE_DIAGONAL * dyRand[orient];
	const Size& mapSize = tileProps.GetSize();
	lineEnd.point = Clamp(lineEnd.point, Point(1, 1), Point((mapSize.w - 1) * 16, (mapSize.h - 1) * 12));
	lineEnd.orient = GetOrient(p, lineEnd.point);
	return lineEnd;
}

// Find a path from start to goal, ending at the specified distance from the
// target (the goal must be in sight of the end, if PF_SIGHT is specified)
Path PathFinder::FindPath(const TraversabilityCache::Data_t& traversabilityCacheSnapshot, const TileProps& tileProps, const Point& source, const Point& destination, const ActorPathContext& actorContext, unsigned int minDistance, int pathfindingFlags)
{
	TRACY(ZoneScoped);

	const unsigned int actorCircleSize = actorContext.circleSize;
	const Movable* const actorIdentity = actorContext.identity;
	const int actorSpeed = actorContext.speed;

	if (InDebugMode(DebugMode::PATHFINDER))
		Log(DEBUG, "FindPath", "source = {}, destination = {}, dist = {}, actorCircleSize = {}",
		    source, destination,
		    minDistance, actorCircleSize);
	const bool actorsAreBlocking = pathfindingFlags & PF_ACTORS_ARE_BLOCKING;
	const auto blockingTraversabilityValue = actorsAreBlocking ? TraversabilityCache::TraversabilityCellValueActor : TraversabilityCache::TraversabilityCellValueActorNonTraversable;

	// TODO: we could optimize this function further by doing everything in SearchmapPoint and converting at the end
	SearchmapPoint smptDest0 { destination };
	NavmapPoint nmptDest = destination;
	NavmapPoint nmptSource = source;
	if (!(GetBlockedInRadiusTile(tileProps, smptDest0, actorCircleSize) & PathMapFlags::PASSABLE)) {
		// If the desired target is blocked, find the path
		// to the nearest reachable point.
		// Also avoid bumping a still actor out of its position,
		// but stop just before it
		orient_t direction = GetOrient(nmptDest, nmptSource);
		AdjustPositionDirected(tileProps, nmptDest, direction, actorCircleSize, minDistance);
	}

	if (nmptDest == nmptSource) return {};

	SearchmapPoint smptSource { nmptSource };
	SearchmapPoint smptDest { nmptDest };

	if (minDistance < actorCircleSize && !(GetBlockedInRadiusTile(tileProps, smptDest, actorCircleSize) & (PathMapFlags::PASSABLE | PathMapFlags::ACTOR))) {
		Log(DEBUG, "FindPath", "can't fit in destination");
		return {};
	}

	const Size& mapSize = tileProps.GetSize();
	if (!mapSize.PointInside(smptSource)) return {};

	const auto getChildBlockedStatusFn = actorCircleSize > 2 ? &PathFinder::GetChildBlockedStatusForBigSize : &PathFinder::GetChildBlockedStatusForSmallSize;

	// Initialize data structures
	const size_t mapCellsCount = mapSize.Area();

	const auto timeOfStartMs = GetMilliseconds();

	// keep most data storage for this algorithm thread_local, to avoid memory allocations;
	// each run we just clear the storage, which is keeping the underlying allocated memory at hand.
	// thread_local rather than static: worker threads run FindPath concurrently
	thread_local BucketPriorityQueue open;
	thread_local std::vector<bool> isClosed;
	thread_local std::vector<NavmapPoint> parents;
	thread_local std::vector<unsigned short> distFromStart;

	// these two, and isClosed further down, are indexed by the same cell index and kept at the
	// same size; resize is a no-op when the size already matches, so this only costs anything on
	// a map change
	parents.resize(mapCellsCount);
	distFromStart.resize(mapCellsCount);

	// cleanup
	open.Clear();
	isClosed.clear();
	isClosed.resize(mapCellsCount, false);
	// `.clear() + .resize()` is generally more performant than `memset` in cases where we have relatively small
	// number of elements, while memset performs better for large vectors
	memset(static_cast<void*>(parents.data()), 0, sizeof(decltype(parents)::value_type) * mapCellsCount);
	memset(static_cast<void*>(distFromStart.data()), 255, sizeof(decltype(distFromStart)::value_type) * mapCellsCount);

	// begin algo init
	distFromStart[smptSource.y * mapSize.w + smptSource.x] = 0;
	parents[smptSource.y * mapSize.w + smptSource.x] = nmptSource;

	open.Push(nmptSource, 0);

	bool foundPath = false;
	unsigned int squaredMinDist = minDistance * minDistance;

	// Weighted heuristic. Finds sub-optimal paths but should be quite a bit faster
	constexpr float_t HEURISTIC_WEIGHT = 1.5;
	const auto getHeuristic = [&](const SearchmapPoint& smptChild, const int& smptChildIdx) {
		// Calculate heuristic
		const int xDist = smptChild.x - smptDest.x;
		const int yDist = smptChild.y - smptDest.y;
		// Tie-breaking used to smooth out the path
		const int dxCross = smptDest.x - smptSource.x;
		const int dyCross = smptDest.y - smptSource.y;
		const int crossProduct = std::abs(xDist * dyCross - yDist * dxCross) >> 3;
		const float distance = std::hypotf(xDist, yDist);
		const float heuristic = HEURISTIC_WEIGHT * (distance + crossProduct);
		const float estDist = distFromStart[smptChildIdx] + heuristic;
		return estDist;
	};

	constexpr uint8_t ITERATION_FREQUENCY_OF_CHECKING_TIMEOUT = 25;
	uint8_t iterationCounterForCheckingTimeout = 0;
	while (!open.IsEmpty()) {
		// guard against stuck paths
		++iterationCounterForCheckingTimeout;
		if (iterationCounterForCheckingTimeout >= ITERATION_FREQUENCY_OF_CHECKING_TIMEOUT) {
			iterationCounterForCheckingTimeout = 0;
			constexpr tick_t FindPathTimeThresholdMs = 15 * 1000;
			const auto timeFromStartMs = GetMilliseconds() - timeOfStartMs;
			if (timeFromStartMs > FindPathTimeThresholdMs) {
				Log(DEBUG, "FindPath", "Abandoning path, it was executing for {}ms which exceeds the threshold.",
				    timeFromStartMs);
				return {};
			}
		}

		const NavmapPoint nmptCurrent = open.Pop();

		const SearchmapPoint smptCurrent { nmptCurrent };
		const int smptCurrentIdx = smptCurrent.y * mapSize.w + smptCurrent.x;
		if (parents[smptCurrentIdx].IsZero()) {
			continue;
		}

		if (smptCurrent == smptDest) {
			nmptDest = nmptCurrent;
			foundPath = true;
			break;
		}

		if (minDistance &&
		    parents[smptCurrentIdx] != nmptCurrent &&
		    SquaredDistance(nmptCurrent, nmptDest) < squaredMinDist &&
		    (!(pathfindingFlags & PF_SIGHT) || IsVisibleLOS(tileProps, smptCurrent, smptDest0, actorSpeed, actorCircleSize))) { // FIXME: should probably be smptDest
			smptDest = smptCurrent;
			nmptDest = nmptCurrent;
			foundPath = true;
			break;
		}

		isClosed[smptCurrentIdx] = true;

		for (size_t i = 0; i < DEGREES_OF_FREEDOM; i++) {
			const NavmapPoint nmptChild(nmptCurrent.x + 16 * dxAdjacent[i], nmptCurrent.y + 12 * dyAdjacent[i]);
			const SearchmapPoint smptChild { nmptChild };
			// Outside map
			if (smptChild.x < 0 || smptChild.y < 0 || smptChild.x >= mapSize.w || smptChild.y >= mapSize.h) continue;
			// Already visited
			int smptChildIdx = smptChild.y * mapSize.w + smptChild.x;
			if (isClosed[smptChildIdx]) continue;

			const PathMapFlags childBlockStatus = (getChildBlockedStatusFn) (tileProps, smptChild, actorCircleSize);
			bool childBlocked = !(childBlockStatus & (PathMapFlags::PASSABLE | PathMapFlags::ACTOR));
			if (childBlocked) continue;

			// If there's an actor, check it can be bumped away
			const TraversabilityCache::TraversabilityCellData navmapCellTraversability = traversabilityCacheSnapshot[nmptChild.y * mapSize.w * 16 + nmptChild.x];
			const bool childIsUnbumpable = navmapCellTraversability.occupyingActor != actorIdentity && navmapCellTraversability.state >= blockingTraversabilityValue;
			if (childIsUnbumpable) continue;

			SearchmapPoint smptCurrent2 { nmptCurrent };
			NavmapPoint nmptParent = parents[smptCurrent2.y * mapSize.w + smptCurrent2.x];
			SearchmapPoint smptParent { nmptParent };
			unsigned short oldDist = distFromStart[smptChildIdx];

			// Lazy Theta star*
			unsigned short newDist = distFromStart[smptParent.y * mapSize.w + smptParent.x] + Distance(smptParent, smptChild);
			if (newDist < oldDist) {
				parents[smptChildIdx] = nmptParent;
				distFromStart[smptChildIdx] = newDist;
			}

			if (distFromStart[smptChildIdx] < oldDist) {
				// Theta-star path if there is LOS
				// so far the searchmap grid appears too coarse to play on, see #2261
				//if (!IsWalkableTo(smptParent, smptChild, actorsAreBlocking, caller)) {
				if (!IsWalkableTo(tileProps, nmptParent, nmptChild, actorsAreBlocking, actorSpeed, actorCircleSize)) {
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

				const float newCost = getHeuristic(smptChild, smptChildIdx);
				open.Push(nmptChild, newCost);
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
			if (pathfindingFlags & PF_BACKAWAY && resultPath && std::abs(area2(nmptCurrent, resultPath.GetStep(0).point, nmptParent)) < 300) {
				newStep.orient = GetOrient(nmptCurrent, nmptParent);
			} else {
				newStep.orient = GetOrient(nmptParent, nmptCurrent);
			}

			resultPath.PrependStep(std::move(newStep));
			nmptCurrent = nmptParent;

			smptCurrent = SearchmapPoint(nmptCurrent);
		}
		return resultPath;
	} else if (InDebugMode(DebugMode::PATHFINDER)) {
		Log(DEBUG, "FindPath", "Pathing failed");
	}

	return {};
}

void PathFinder::NormalizeDeltas(float_t& dx, float_t& dy, const float_t factor)
{
	constexpr float_t STEP_RADIUS = 2.0;

	const float_t ySign = std::copysign(1.0f, dy);
	const float_t xSign = std::copysign(1.0f, dx);
	dx = std::fabs(dx);
	dy = std::fabs(dy);
	const float_t dxOrig = dx;
	const float_t dyOrig = dy;
	if (dx == 0.0) {
		dy = STEP_RADIUS * 0.75f;
	} else if (dy == 0.0) {
		dx = STEP_RADIUS;
	} else {
		const float_t q = STEP_RADIUS / std::hypotf(dx, dy);
		dx = dx * q;
		dy = dy * q * 0.75f;
	}
	dx = std::min(dx * factor, dxOrig);
	dy = std::min(dy * factor, dyOrig);
	dx = std::ceil(dx) * xSign;
	dy = std::ceil(dy) * ySign;
}
}
