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
#include "Interface.h"
#include "Map.h"
#include "RNG.h"

#include "Logging/Logging.h"
#include "Scriptable/Actor.h"

#include <array>
#include <limits>
#include <set>

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

	LogDebugPathfinder("FindPath", "caller = {}, source = {}, destination = {}, dist = {}, actorCircleSize = {}",
			   actorContext.scriptName, source, destination,
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
		LogDebugPathfinder("FindPath", "{} can't fit in destination", actorContext.scriptName);
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
				Log(DEBUG, "FindPath", "Abandoning path of {}, it was executing for {}ms which exceeds the threshold.",
				    actorContext.scriptName, timeFromStartMs);
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
	}

	LogDebugPathfinder("FindPath", "Pathing failed for {}", actorContext.scriptName);

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

void PathFinder::BlockSearchMapFor(const Movable* actor, TileProps& tileProps)
{
	auto flag = actor->IsPC() ? PathMapFlags::PC : PathMapFlags::NPC;
	tileProps.PaintSearchMap(actor->SMPos, actor->circleSize, flag);
}


void PathFinder::ClearSearchMapFor(const std::vector<Actor*>& actors, const Movable* actor, TileProps& tileProps)
{
	ClearSearchMapFor(actors, actor, actor->Pos, actor->SMPos, actor->circleSize, tileProps);
}

void PathFinder::ClearSearchMapFor(const std::vector<Actor*>& actors, const Movable* instigatorIdentity, const Point& actorPos, const SearchmapPoint& actorSMPos, int actorCircleSize, TileProps& tileProps)
{
	// Find nearby actors that need their searchmap restored
	std::vector<Actor*> nearActors;
	constexpr unsigned int radiusFeet = MAX_CIRCLE_SIZE * 3; // WithinRange takes feet
	constexpr int flags = GA_NO_DEAD | GA_NO_LOS | GA_NO_UNSCHEDULED;

	for (auto actor : actors) {
		if (!WithinRange(actor, actorPos, radiusFeet)) {
			continue;
		}
		if (!actor->ValidTarget(flags)) {
			continue;
		}
		nearActors.emplace_back(actor);
	}

	tileProps.PaintSearchMap(actorSMPos, actorCircleSize, PathMapFlags::UNMARKED);

	// Restore the searchmap areas of any nearby actors that could
	// have been cleared by this PaintSearchMap(..., PathMapFlags::UNMARKED).
	// Skip the instigator itself — its footprint was just cleared intentionally.
	for (const Actor* neighbour : nearActors) {
		if (neighbour == instigatorIdentity) continue;
		if (neighbour->BlocksSearchMap()) {
			BlockSearchMapFor(neighbour, tileProps);
		}
	}
}

void PathFinder::ClearSearchMapFor(const std::vector<ActorSearchMapData>& actorsData, const Movable* instigatorIdentity, const Point& actorPos, const SearchmapPoint& actorSMPos, int actorCircleSize, TileProps& tileProps)
{
	tileProps.PaintSearchMap(actorSMPos, actorCircleSize, PathMapFlags::UNMARKED);

	// Restore the searchmap areas of any nearby actors that could
	// have been cleared by this PaintSearchMap(..., PathMapFlags::UNMARKED).
	// Skip the instigator itself — its footprint was just cleared intentionally.
	// Uses snapshotted actor data — safe for worker threads.
	constexpr unsigned int radiusPixels = MAX_CIRCLE_SIZE * 3 * 16;
	constexpr unsigned int radiusPixelsSquared = radiusPixels * radiusPixels;
	for (const auto& data : actorsData) {
		if (!data.blocksSearchMap) continue;
		if (data.identity == instigatorIdentity) continue;
		if (SquaredDistance(data.pos, actorPos) > radiusPixelsSquared) continue;

		const auto flag = data.isPC ? PathMapFlags::PC : PathMapFlags::NPC;
		tileProps.PaintSearchMap(data.smPos, data.circleSize, flag);
	}
}

// p is in tile coords
PathMapFlags PathFinder::GetBlockedTile(const TileProps& tileProps, const SearchmapPoint& p, int size)
{
	if (size == -1) {
		return GetBlockedTile(tileProps, p);
	} else {
		return GetBlockedInRadiusTile(tileProps, p, size);
	}
}

PathMapFlags PathFinder::GetBlockedTile(const TileProps& tileProps, const SearchmapPoint& p)
{
	PathMapFlags ret = tileProps.QuerySearchMap(p);
	if (bool(ret & PathMapFlags::TRAVEL)) {
		ret |= PathMapFlags::PASSABLE;
	}
	if (bool(ret & (PathMapFlags::DOOR_IMPASSABLE | PathMapFlags::ACTOR))) {
		ret &= ~PathMapFlags::PASSABLE;
	}
	if (bool(ret & PathMapFlags::DOOR_OPAQUE)) {
		ret = PathMapFlags::SIDEWALL;
	}
	return ret;
}


PathMapFlags PathFinder::GetBlockedInRadiusTile(const TileProps& tileProps, const SearchmapPoint& tp, uint16_t size, const bool stopOnImpassable)
{
	// We check a circle of radius size-2 around (px,py)
	// TODO: recheck that this matches originals
	// these circles are perhaps slightly different for sizes 7 and up.

	PathMapFlags ret = PathMapFlags::IMPASSABLE;
	size = Clamp<uint16_t>(size, 2, MAX_CIRCLESIZE);
	uint16_t r = size - 2;

	std::vector<BasePoint> points;
	if (r == 0) { // avoid generating 16 identical points
		points.push_back(tp);
		points.push_back(tp);
	} else {
		points = PlotCircle(tp, r);
	}
	for (size_t i = 0; i < points.size(); i += 2) {
		const BasePoint& p1 = points[i];
		const BasePoint& p2 = points[i + 1];
		assert(p1.y == p2.y);
		assert(p2.x <= p1.x);

		for (int x = p2.x; x <= p1.x; ++x) {
			PathMapFlags flags = GetBlockedTile(tileProps, SearchmapPoint(x, p1.y));
			if (stopOnImpassable && flags == PathMapFlags::IMPASSABLE) {
				return PathMapFlags::IMPASSABLE;
			}
			ret |= flags;
		}
	}

	if (bool(ret & (PathMapFlags::DOOR_IMPASSABLE | PathMapFlags::ACTOR | PathMapFlags::SIDEWALL))) {
		ret &= ~PathMapFlags::PASSABLE;
	}
	if (bool(ret & PathMapFlags::DOOR_OPAQUE)) {
		ret = PathMapFlags::SIDEWALL;
	}

	return ret;
}

PathMapFlags PathFinder::GetBlockedInLine(const TileProps& tileProps, const NavmapPoint& s, const NavmapPoint& d, bool stopOnImpassable, const Actor* caller)
{
	int speed = caller ? caller->GetSpeed() : 0;
	int cSize = caller ? caller->circleSize : 0;
	return GetBlockedInLine(tileProps, s, d, stopOnImpassable, speed, cSize);
}

PathMapFlags PathFinder::GetBlockedInLine(const TileProps& tileProps, const NavmapPoint& s, const NavmapPoint& d, bool stopOnImpassable, int actorSpeed, int actorCircleSize)
{
	PathMapFlags ret = PathMapFlags::IMPASSABLE;
	SearchmapPoint sms { s };
	float_t factor = actorSpeed ? float_t(gamedata->GetStepTime()) / float_t(actorSpeed) : 1;

	const auto getBlockedStatusFn = (stopOnImpassable && actorCircleSize) ? &PathFinder::GetChildBlockedStatusForBigSize : &PathFinder::GetChildBlockedStatusForSmallSize;
	LineStepper<NavmapPoint> walk { s, d, factor };
	while (walk.Step()) {
		SearchmapPoint smp { walk.Current() };
		if (sms == smp) continue;

		// see note in GetBlockedInLineTile
		const PathMapFlags blockStatus = (getBlockedStatusFn) (tileProps, smp, actorCircleSize);
		if (stopOnImpassable && blockStatus == PathMapFlags::IMPASSABLE) {
			return PathMapFlags::IMPASSABLE;
		}
		ret |= blockStatus;
	}
	if (bool(ret & (PathMapFlags::DOOR_IMPASSABLE | PathMapFlags::ACTOR | PathMapFlags::SIDEWALL))) {
		ret &= ~PathMapFlags::PASSABLE;
	}
	if (bool(ret & PathMapFlags::DOOR_OPAQUE)) {
		ret = PathMapFlags::SIDEWALL;
	}

	return ret;
}

PathMapFlags PathFinder::GetBlockedInLineTile(const TileProps& tileProps, const SearchmapPoint& s, const SearchmapPoint& d, bool stopOnImpassable, const Actor* caller)
{
	int speed = caller ? caller->GetSpeed() : 0;
	int cSize = caller ? caller->circleSize : 0;
	return GetBlockedInLineTile(tileProps, s, d, stopOnImpassable, speed, cSize);
}

PathMapFlags PathFinder::GetBlockedInLineTile(const TileProps& tileProps, const SearchmapPoint& s, const SearchmapPoint& d, bool stopOnImpassable, int actorSpeed, int actorCircleSize)
{
	PathMapFlags ret = PathMapFlags::IMPASSABLE;
	float_t factor = actorSpeed ? float_t(gamedata->GetStepTime()) / float_t(actorSpeed) / 16 : 1;

	const auto getBlockedStatusFn = (stopOnImpassable && actorCircleSize) ? &PathFinder::GetChildBlockedStatusForBigSize : &PathFinder::GetChildBlockedStatusForSmallSize;
	LineStepper<SearchmapPoint> walk { s, d, factor };
	while (walk.Step()) {
		const SearchmapPoint& p = walk.Current();
		if (s == p) continue;

		// do a wider check for bigger actors (for the common case it's the same)
		// should not be used for IsVisibleLOS
		const PathMapFlags blockStatus = (getBlockedStatusFn) (tileProps, p, actorCircleSize);
		if (stopOnImpassable && blockStatus == PathMapFlags::IMPASSABLE) {
			return PathMapFlags::IMPASSABLE;
		}
		ret |= blockStatus;
	}
	if (bool(ret & (PathMapFlags::DOOR_IMPASSABLE | PathMapFlags::ACTOR | PathMapFlags::SIDEWALL))) {
		ret &= ~PathMapFlags::PASSABLE;
	}
	if (bool(ret & PathMapFlags::DOOR_OPAQUE)) {
		ret = PathMapFlags::SIDEWALL;
	}

	return ret;
}

bool PathFinder::IsVisibleLOS(const TileProps& tileProps, const Point& s, const Point& d, const Actor* caller)
{
	PathMapFlags ret = GetBlockedInLine(tileProps, s, d, false, caller);
	return !bool(ret & PathMapFlags::SIDEWALL);
}

bool PathFinder::IsVisibleLOS(const TileProps& tileProps, const SearchmapPoint& s, const SearchmapPoint& d, const Actor* caller)
{
	PathMapFlags ret = GetBlockedInLineTile(tileProps, s, d, false, caller);
	return !bool(ret & PathMapFlags::SIDEWALL);
}

bool PathFinder::IsVisibleLOS(const TileProps& tileProps, const SearchmapPoint& s, const SearchmapPoint& d, int actorSpeed, int actorCircleSize)
{
	PathMapFlags ret = GetBlockedInLineTile(tileProps, s, d, false, actorSpeed, actorCircleSize);
	return !bool(ret & PathMapFlags::SIDEWALL);
}


bool PathFinder::IsLineWalkable(const PathMapFlags accumulatedFlags, const bool areActorsBlocking)
{
	// Check the geometry first:
	// `accumulatedFlags` is OR-accumulated over the whole line, so a single tile with ACTOR flag, sets ACTOR for
	// all of it. Ignoring actors would cause ignore any wall on the line, so pathfinder could route a path
	// going straight through a wall - and that's a pretty bad pathfinder's job if you ask me. Unless it's a pathfinder
	// for ghosts.
	if (static_cast<bool>(accumulatedFlags & (PathMapFlags::SIDEWALL | PathMapFlags::DOOR_IMPASSABLE))) {
		return false;
	}
	const PathMapFlags mask = PathMapFlags::PASSABLE | (areActorsBlocking ? PathMapFlags::UNMARKED : PathMapFlags::ACTOR);
	return static_cast<bool>(accumulatedFlags & mask);
}

bool PathFinder::IsWalkableTo(const TileProps& tileProps, const Point& s, const Point& d, bool actorsAreBlocking, const Actor* caller)
{
	PathMapFlags ret = GetBlockedInLine(tileProps, s, d, true, caller);
	return IsLineWalkable(ret, actorsAreBlocking);
}

bool PathFinder::IsWalkableTo(const TileProps& tileProps, const Point& s, const Point& d, bool actorsAreBlocking, int actorSpeed, int actorCircleSize)
{
	PathMapFlags ret = GetBlockedInLine(tileProps, s, d, true, actorSpeed, actorCircleSize);
	return IsLineWalkable(ret, actorsAreBlocking);
}

bool PathFinder::AdjustPositionX(const TileProps& tileProps, SearchmapPoint& goal, const Size& radius, int size)
{
	int minx = 0;
	if (goal.x > radius.w) {
		minx = goal.x - radius.w;
	}
	int maxx = goal.x + radius.w + 1;

	const Size& mapSize = tileProps.GetSize();

	if (maxx > mapSize.w)
		maxx = mapSize.w;

	for (int scanx = minx; scanx < maxx; scanx++) {
		if (goal.y >= radius.h) {
			const SearchmapPoint p(scanx, goal.y - radius.h);
			if (bool(GetBlockedTile(tileProps, p, size) & PathMapFlags::PASSABLE)) {
				goal.x = scanx;
				goal.y = goal.y - radius.h;
				return true;
			}
		}
		if (goal.y + radius.h < mapSize.h) {
			const SearchmapPoint p(scanx, goal.y + radius.h);
			if (bool(GetBlockedTile(tileProps, p, size) & PathMapFlags::PASSABLE)) {
				goal.x = scanx;
				goal.y = goal.y + radius.h;
				return true;
			}
		}
	}
	return false;
}

bool PathFinder::AdjustPositionY(const TileProps& tileProps, SearchmapPoint& goal, const Size& radius, int size)
{
	int miny = 0;
	if (goal.y > radius.h) {
		miny = goal.y - radius.h;
	}
	int maxy = goal.y + radius.h + 1;

	const Size& mapSize = tileProps.GetSize();
	if (maxy > mapSize.h)
		maxy = mapSize.h;
	for (int scany = miny; scany < maxy; scany++) {
		if (goal.x >= radius.w) {
			const SearchmapPoint p(goal.x - radius.w, scany);
			if (bool(GetBlockedTile(tileProps, p, size) & PathMapFlags::PASSABLE)) {
				goal.x = goal.x - radius.w;
				goal.y = scany;
				return true;
			}
		}
		if (goal.x + radius.w < mapSize.w) {
			const SearchmapPoint p(goal.x + radius.w, scany);
			if (bool(GetBlockedTile(tileProps, p, size) & PathMapFlags::PASSABLE)) {
				goal.x = goal.x + radius.w;
				goal.y = scany;
				return true;
			}
		}
	}
	return false;
}

// best adjustment attempt given an initial direction to look around
// at the same time we don't want to look too far in the same direction, since getting close
// to the target is more important
void PathFinder::AdjustPositionDirected(const TileProps& tileProps, NavmapPoint& goal, orient_t direction, int startingRadius, unsigned int minDistance)
{
	const Size& mapSize = tileProps.GetSize();
	SearchmapPoint smptGoal { goal };
	if (smptGoal.x > mapSize.w) {
		smptGoal.x = mapSize.w;
	}
	if (smptGoal.y > mapSize.h) {
		smptGoal.y = mapSize.h;
	}

	// search at starting orientation first, then left and right of it, then repeat with higher radius
	// a bit like a sparse cone projectile
	// NOTE: OrientedOffset is wrong for radius > 1, ignoring the other 8 possible orientations
	//       it's not really important, since we're usually checking circle sizes larger than 1
	//       and there'd be overlaps for longer than it takes to find a good position
	const std::array<orient_t, 5> orients { direction, NextOrientation(direction, 2), PrevOrientation(direction, 2), NextOrientation(direction), PrevOrientation(direction) };
	std::set<SearchmapPoint, SearchmapPoint::Cmp> baseOffsets; // OrientedOffset only offsets in 8 directions, so there will be duplicates
	for (size_t idx = 0; idx < orients.size(); idx++) {
		Point p = OrientedOffset(orients[idx], 1);
		baseOffsets.emplace(p.x, p.y);
	}

	std::map<unsigned int, SearchmapPoint, std::greater<>> candidates;
	NavmapPoint adjGoal = goal - NavmapPoint(8, 6);
	int radius = startingRadius - 1;
	while (radius < 2 * startingRadius) { // reduce this search radius if needed
		for (auto& offset : baseOffsets) {
			SearchmapPoint candidate = smptGoal + offset * radius;
			if (bool(GetBlockedTile(tileProps, candidate, startingRadius) & PathMapFlags::PASSABLE)) {
				unsigned int range = SquaredDistance(candidate.ToNavmapPoint(), adjGoal);
				candidates[range] = candidate;
			}
		}
		radius++;
	}

	if (candidates.empty()) {
		// fall back to regular search
		AdjustPosition(tileProps, smptGoal);
	} else {
		// pick the closest candidate, taking the needed range into account
		// the map is reverse-sorted already
		bool found = false;
		unsigned int minDist2 = minDistance * minDistance;
		for (const auto& candidate : candidates) {
			if (candidate.first > minDist2) continue;
			smptGoal = candidate.second;
			found = true;
			break;
		}
		if (!found) {
			// if all are further than what we want, go as close as possible
			smptGoal = candidates.crbegin()->second;
		}
	}

	goal.x = smptGoal.x * 16 + 8;
	goal.y = smptGoal.y * 12 + 6;
}

void PathFinder::AdjustPosition(const TileProps& tileProps, SearchmapPoint& goal, const Size& startingRadius, int size)
{
	const Size& mapSize = tileProps.GetSize();
	Size radius = startingRadius;

	if (goal.x > mapSize.w) {
		goal.x = mapSize.w;
	}
	if (goal.y > mapSize.h) {
		goal.y = mapSize.h;
	}

	while (radius.w < mapSize.w || radius.h < mapSize.h) {
		//lets make it slightly random where the actor will appear
		if (RandomFlip()) {
			if (AdjustPositionX(tileProps, goal, radius, size)) {
				return;
			}
			if (AdjustPositionY(tileProps, goal, radius, size)) {
				return;
			}
		} else {
			if (AdjustPositionY(tileProps, goal, radius, size)) {
				return;
			}
			if (AdjustPositionX(tileProps, goal, radius, size)) {
				return;
			}
		}
		if (radius.w < mapSize.w) {
			radius.w++;
		}
		if (radius.h < mapSize.h) {
			radius.h++;
		}
	}
}
}
