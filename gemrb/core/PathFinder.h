// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef PATHFINDER_H
#define PATHFINDER_H

#include "exports.h"

#include "Debug.h"
#include "Orientation.h"
#include "PathFinderRequest.h"
#include "Region.h"
#include "TileProps.h"
#include "TraversabilityCache.h"

#include "Logging/Logging.h"
#include "Scriptable/Scriptable.h"

#include <vector>


namespace GemRB {
class Actor;
class Movable;

// Log() at DEBUG, gated on the pathfinder debug flag. Shared by the pathfinder and its scheduler,
// so neither spams a plain debug build with per-request output.
template<typename... ARGS>
void LogDebugPathfinder(const char* owner, const char* message, ARGS&&... args)
{
	if (InDebugMode(DebugMode::PATHFINDER)) {
		Log(DEBUG, owner, message, std::forward<ARGS>(args)...);
	}
}

/**
 * Lightweight snapshot of actor data needed for searchmap operations on worker threads.
 * Avoids dereferencing live Actor pointers from pathfinder threads.
 */
struct ActorSearchMapData {
	const Actor* identity = nullptr; // pointer identity only, never dereferenced on worker threads
	Point pos;
	SearchmapPoint smPos;
	int circleSize = 0;
	bool isPC = false;
	bool blocksSearchMap = false;
};

struct PathNode {
	Point point;
	orient_t orient;
	bool waypoint = false;
	bool operator==(const PathNode& other) const noexcept
	{
		return point == other.point && orient == other.orient;
	}
};

struct Path {
	std::vector<PathNode> nodes;
	size_t currentStep = 0;
	using iterator = std::vector<PathNode>::iterator;
	using const_iterator = std::vector<PathNode>::const_iterator;

	bool operator==(const Path& other) const noexcept
	{
		return nodes == other.nodes && currentStep == other.currentStep;
	}
	bool operator!=(const Path& other) const noexcept
	{
		return !operator==(other);
	}
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
	void PrependStep(PathNode&& step)
	{
		nodes.insert(nodes.begin(), step);
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

/**
 * PathFinder - stateless class implementing pathfinding-related algorithms.
 *
 * Responsibilities:
 * - provides static pathfinding algorithms (FindPath, terrain queries, etc.),
 * - takes terrain data (TileProps) and actor data as parameters,
 * - no Map dependency - all functions are pure static methods,
 * - no internal state, thread-safe when given proper inputs.
 */
class GEM_EXPORT PathFinder {
public:
	static PathMapFlags GetBlockedTile(const TileProps& tileProps, const SearchmapPoint&);

	// helper function used when the size > 2
	static PathMapFlags GetChildBlockedStatusForBigSize(const TileProps& tileProps, const SearchmapPoint& smptChild, const unsigned int size)
	{
		return GetBlockedInRadiusTile(tileProps, smptChild, size);
	}
	// helper function used when the size <= 2
	static PathMapFlags GetChildBlockedStatusForSmallSize(const TileProps& tileProps, const SearchmapPoint& smptChild, const unsigned int /* size */)
	{
		return GetBlockedTile(tileProps, smptChild);
	}

	static void BlockSearchMapFor(const Movable* actor, TileProps& tileProps);

	static void ClearSearchMapFor(const std::vector<Actor*>& actors, const Movable* actor, TileProps& tileProps);
	static void ClearSearchMapFor(const std::vector<Actor*>& actors, const Movable* instigatorIdentity, const Point& actorPos, const SearchmapPoint& actorSMPos, int actorCircleSize, TileProps& tileProps);
	static void ClearSearchMapFor(const std::vector<ActorSearchMapData>& actorsData, const Movable* instigatorIdentity, const Point& actorPos, const SearchmapPoint& actorSMPos, int actorCircleSize, TileProps& tileProps);

	static void AdjustPosition(const TileProps& tileProps, SearchmapPoint& goal, const Size& startingRadius = Size(), int size = -1);
	static void AdjustPositionDirected(const TileProps& tileProps, NavmapPoint& goal, orient_t direction, int startingRadius, unsigned int minDistance);

	/* Finds the path which leads to near d */
	static Path FindPath(const TraversabilityCache::Data_t& traversabilityCacheSnapshot, const TileProps& tileProps, const Point& source, const Point& destination, const ActorPathContext& actorContext, unsigned int minDistance = 0, int pathfindingFlags = PF_SIGHT);

	static bool IsVisibleLOS(const TileProps& tileProps, const Point& s, const Point& d, const Actor* caller);
	static bool IsVisibleLOS(const TileProps& tileProps, const SearchmapPoint& s, const SearchmapPoint& d, const Actor* caller);
	static bool IsVisibleLOS(const TileProps& tileProps, const SearchmapPoint& s, const SearchmapPoint& d, int actorSpeed, int actorCircleSize);

	static bool IsWalkableTo(const TileProps& tileProps, const Point& s, const Point& d, bool actorsAreBlocking, const Actor* caller);
	static bool IsWalkableTo(const TileProps& tileProps, const Point& s, const Point& d, bool actorsAreBlocking, int actorSpeed, int actorCircleSize);
	static bool IsLineWalkable(PathMapFlags accumulatedFlags, bool areActorsBlocking);

	static bool AdjustPositionX(const TileProps& tileProps, SearchmapPoint& goal, const Size& radius, int size = -1);
	static bool AdjustPositionY(const TileProps& tileProps, SearchmapPoint& goal, const Size& radius, int size = -1);

	static PathMapFlags GetBlockedInLine(const TileProps& tileProps, const NavmapPoint& s, const NavmapPoint& d, bool stopOnImpassable, const Actor* caller = nullptr);
	static PathMapFlags GetBlockedInLine(const TileProps& tileProps, const NavmapPoint& s, const NavmapPoint& d, bool stopOnImpassable, int actorSpeed, int actorCircleSize);

	static PathMapFlags GetBlockedInLineTile(const TileProps& tileProps, const SearchmapPoint& s, const SearchmapPoint& d, bool stopOnImpassable, const Actor* caller = nullptr);
	static PathMapFlags GetBlockedInLineTile(const TileProps& tileProps, const SearchmapPoint& s, const SearchmapPoint& d, bool stopOnImpassable, int actorSpeed, int actorCircleSize);

	// same as GetBlocked, but in TileCoords
	static PathMapFlags GetBlockedTile(const TileProps& tileProps, const SearchmapPoint&, int size);
	static PathMapFlags GetBlockedInRadiusTile(const TileProps& tileProps, const SearchmapPoint&, uint16_t size, bool stopOnImpassable = true);

	static void NormalizeDeltas(float_t& dx, float_t& dy, float_t factor = 1);

	/**
	 * Walks a point towards a target one engine step at a time.
	 *
	 * The delta is renormalized on every step.
	 * Movable::DoStep() runs one such step per frame.
	 *
	 * The point type picks the space: NavmapPoint steps in pixels, SearchmapPoint in tiles.
	 */
	template<typename PointType>
	class LineStepper {
	public:
		LineStepper(const PointType& from, const PointType& to, float_t stepFactor = 1)
			: p(from), d(to), factor(stepFactor) {}

		/** Takes one step; false once the target is reached, leaving Current() on it. */
		bool Step()
		{
			if (p == d) return false;

			float_t dx = d.x - p.x;
			float_t dy = d.y - p.y;
			NormalizeDeltas(dx, dy, factor);
			p.x += dx;
			p.y += dy;
			return true;
		}

		const PointType& Current() const { return p; }

	private:
		PointType p;
		PointType d;
		float_t factor;
	};

	/** Calculate a destination point for running away from a threat.
	 * Returns false and leaves outPoint untouched if the actor is too slow or the deltas are
	 * too small; on success outPoint holds the destination. */
	static bool CalculateRunAwayPoint(const TileProps& tileProps, const Point& source, const Point& threat, int maxPathLength, int actorSpeed, int actorCircleSize, Point& outPoint);

	/** Calculate a random walk destination within the specified radius.
	 * Returns false and leaves outStep untouched if the actor is too slow or gets stuck; on
	 * success outStep holds the destination and the orientation to face while walking there. */
	static bool CalculateRandomWalkPoint(const TileProps& tileProps, const Point& source, int actorCircleSize, int radius, int actorSpeed, PathNode& outStep);

	/** Calculate a straight line path from start to dest.
	 * Returns a path with nodes at specified speed intervals. */
	static Path CalculateLinePath(const TileProps& tileProps, const Point& start, const Point& dest, int speed, orient_t orientation, int flags);

	/** Calculate the end point of a line starting at p with given steps and orientation. */
	static PathNode CalculateLineEnd(const TileProps& tileProps, const Point& p, int steps, orient_t orient);
};
}

#endif
