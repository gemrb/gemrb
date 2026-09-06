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

#include <array>
#include <cstdint>
#include <cstdlib>
#include <utility>
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
	PathNode GetStep(const size_t idx) const
	{
		return nodes[idx];
	}
	PathNode GetLastStep() const
	{
		if (nodes.empty()) {
			return {};
		}
		return nodes[nodes.size() - 1];
	}
	PathNode GetCurrentStep() const
	{
		return nodes[currentStep];
	}
	PathNode GetNextStep(const size_t x) const
	{
		const size_t next = currentStep + x;
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

constexpr unsigned int SEARCHMAP_SQUARE_WIDTH = 16;
constexpr unsigned int SEARCHMAP_SQUARE_HEIGHT = 12;
constexpr unsigned int SEARCHMAP_SQUARE_DIAGONAL = 20; // sqrt(16 * 16 + 12 * 12)

namespace {

	// GetBlockedTile's fixup table, built at compile time - see the comment there.
	// Written as a free function plus an index_sequence expansion.
	// When moving to C++17, we could just use a loop writing into a std::array
	// instead, without this 3-steps template parameters expansion.
	constexpr PathMapFlags SearchMapFixupEntry(unsigned i) noexcept
	{
		PathMapFlags f = static_cast<PathMapFlags>(i);
		if (bool(f & PathMapFlags::TRAVEL)) {
			f |= PathMapFlags::PASSABLE;
		}
		if (bool(f & (PathMapFlags::DOOR_IMPASSABLE | PathMapFlags::ACTOR))) {
			f &= ~PathMapFlags::PASSABLE;
		}
		if (bool(f & PathMapFlags::DOOR_OPAQUE)) {
			f = PathMapFlags::SIDEWALL;
		}
		return f;
	}

	template<std::size_t... IDXS>
	constexpr std::array<PathMapFlags, 256> MakeSearchMapFixupTable(std::index_sequence<IDXS...>) noexcept
	{
		return { { SearchMapFixupEntry(IDXS)... } };
	}

	// Anonymous namespace, not a function-local static inside GetBlockedTile: a function-local
	// static of an inline function is emitted as an STB_GNU_UNIQUE ELF symbol (so that every
	// translation unit in the process shares one instance), and under -fPIC that forces every
	// access through the GOT - a memory indirection on EVERY tile fetch.
	// An anonymous-namespace object has internal linkage: every translation unit that includes this header
	// gets its own private copy, addressed directly with a rip-relative lea, no GOT, no indirection.
	// The cost is a few hundred bytes of duplicated .rodata per translation unit.
	constexpr std::array<PathMapFlags, 256> SearchMapFixupTable = MakeSearchMapFixupTable(std::make_index_sequence<256> {});
} // namespace

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

	static bool IsVisibleLOS(const TileProps& tileProps, const Point& s, const Point& d);
	static bool IsVisibleLOS(const TileProps& tileProps, const SearchmapPoint& s, const SearchmapPoint& d);

	static bool IsWalkableTo(const TileProps& tileProps, const Point& s, const Point& d, bool actorsAreBlocking, const Actor* caller);
	static bool IsWalkableTo(const TileProps& tileProps, const Point& s, const Point& d, bool actorsAreBlocking, int actorCircleSize);
	static bool IsLineWalkable(PathMapFlags accumulatedFlags, bool areActorsBlocking);

	static bool AdjustPositionX(const TileProps& tileProps, SearchmapPoint& goal, const Size& radius, int size = -1);
	static bool AdjustPositionY(const TileProps& tileProps, SearchmapPoint& goal, const Size& radius, int size = -1);

	// The line these walk is the straight segment between the endpoints, at the resolution of the
	// tile grid. actorCircleSize only widens the inspection around each tile of that segment
	static PathMapFlags GetBlockedInLine(const TileProps& tileProps, const NavmapPoint& s, const NavmapPoint& d, bool stopOnImpassable, const Actor* caller = nullptr);
	static PathMapFlags GetBlockedInLine(const TileProps& tileProps, const NavmapPoint& s, const NavmapPoint& d, bool stopOnImpassable, int actorCircleSize);

	static PathMapFlags GetBlockedInLineTile(const TileProps& tileProps, const SearchmapPoint& s, const SearchmapPoint& d, bool stopOnImpassable, const Actor* caller = nullptr);
	static PathMapFlags GetBlockedInLineTile(const TileProps& tileProps, const SearchmapPoint& s, const SearchmapPoint& d, bool stopOnImpassable, int actorCircleSize);

	// same as GetBlocked, but in TileCoords
	static PathMapFlags GetBlockedTile(const TileProps& tileProps, const SearchmapPoint&, int size);

	// PathFinder is GEM_EXPORT (public API). Under GCC's default -fPIC handling, an out-of-line
	// definition of this function is subject to ELF symbol interposition and can never be inlined into
	// any caller - not even one in PathFinder.cpp itself.
	// This is the tile-fetch hot path for every walkability and line-of-sight walk, keep it inlined in a header.
	static PathMapFlags GetBlockedTile(const TileProps& tileProps, const SearchmapPoint& p)
	{
		// PathMapFlags is a uint8_t enum, so the fixup (TRAVEL implies PASSABLE, DOOR_IMPASSABLE/
		// ACTOR clear it, DOOR_OPAQUE forces SIDEWALL) has only 256 possible inputs and collapses
		// to SearchMapFixupTable above, turning three dependent branches per tile fetch into one
		// load of compile-time computed data.
		return SearchMapFixupTable[static_cast<uint8_t>(tileProps.QuerySearchMap(p))];
	}

	static PathMapFlags GetBlockedInRadiusTile(const TileProps& tileProps, const SearchmapPoint&, uint16_t size, bool stopOnImpassable = true);

	/**
	 * The exact per-frame movement delta: the direction from the actor to its next waypoint,
	 * renormalized to one engine step and scaled by the actor's move rate, with the y axis
	 * downscaled by 12/16 so that a step covers the same ground in either direction on the
	 * anisotropic navmap. Clamped so a step never overshoots the waypoint.
	 */
	static void ScaleDeltas(float_t& dx, float_t& dy, float_t factor = 1);

	/**
	 * ScaleDeltas() rounded away from zero on each axis independently.
	 *
	 * The rounding guarantees at least one pixel of progress per axis per call, which is what a
	 * caller that has nowhere to keep a fraction needs in order to converge at all. It costs the
	 * direction: a step of (1.96, 0.31) becomes (2, 1), so shallow lines come out at 45 degrees.
	 * Only use it where the walk is a probe that has to terminate, never to sample geometry and
	 * never to move an actor along a validated path.
	 */
	static void NormalizeDeltas(float_t& dx, float_t& dy, float_t factor = 1);

	/**
	 * Probe a walk a point towards a target one engine step at a time.
	 * The delta is renormalized on every step.
	 * Only use it where the walk is a probe that has to terminate, never to sample geometry and
	 * never to move an actor along a validated path.
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

	/**
	 * Enumerates the searchmap tiles a straight ray crosses, in order, skipping the tile it
	 * starts on.
	 *
	 * The sampling resolution is the tile grid itself, so every tile the segment passes through
	 * is handed out exactly once and none is stepped over, whatever the length or slope of the
	 * line.
	 *
	 * The algorithm:
	 * This is the grid traversal of J. Amanatides and A. Woo, "A Fast Voxel Traversal Algorithm
	 * for Ray Tracing" - the standard way to enumerate the cells a ray meets, and the one raycasters
	 * and voxel engines use. Paper: http://www.cse.yorku.ca/~amana/research/grid.pdf
	 *
	 * Write the segment as `start + t * delta`, t running 0 to 1. Leaving a tile means crossing
	 * one of its two grid lines, so the walk only has to know, at each point, which of the two
	 * comes first:
	 *
	 * - `tMaxX` is the t at which the next vertical grid line is crossed, `tMaxY` the t of the
	 *   next horizontal one,
	 * - the smaller one wins: step that axis by one tile and push its `tMax` on by `tDelta`,
	 *   the t it takes to cross one whole tile on that axis,
	 * - repeat. The tiles come out in the order the segment meets them, and the cost is one
	 *   comparison and one addition per tile rather than anything per pixel.
	 *
	 * Two deviations from the paper, both tailored for our use:
	 *
	 * - it is kept in integers. `tMaxX` is `errX / ax`, where `errX` is the distance still to go
	 *   to the next vertical grid line and `ax` is `abs(delta.x)`; comparing it with `tMaxY` is
	 *   then `errX * ay` against `errY * ax`, so the whole walk needs no division, square root
	 *   or float. That comparison term, `err = errX * ay - errY * ax`, is carried forward rather
	 *   than rebuilt each step: errX only ever grows by cellW and errY by cellH, so err only
	 *   ever grows by the loop invariants `errStepX = cellW * ay` and `errStepY = cellH * ax`,
	 *   both computed once in the constructor. A step is then a masked add of one of those two
	 *   precomputed values,
	 * - the paper's ray is unbounded and stops on a hit; this is a segment, so an axis that has
	 *   reached the target tile is pinned. That both terminates the walk exactly on the far end
	 *   and keeps rounding from carrying it one tile past.
	 *
	 * A tie, `tMaxX == tMaxY`, is the segment going exactly through a tile corner, where four
	 * tiles meet: the walk moves diagonally and is never inside either of the two tiles beside
	 * the diagonal. Whether those matter depends on what is travelling the line, so they are not
	 * reported as tiles on it - CutACorner() flags the step and CornerBesideX()/CornerBesideY()
	 * name them, and the caller decides:
	 *
	 * - sight ignores them. A ray has no width and does thread the joint of two diagonal walls.
	 * - walkability blocks only when *both* are blocked. An actor can round a single convex
	 *   corner, which is what a diagonal step past one wall tile is, but it cannot thread the
	 *   joint between two: its body is on integer pixels and has nowhere to be.
	 *
     * The constructor picks the space: NavmapPoint endpoints are navmap pixels, SearchmapPoint
	 * ones are tile indices and are taken as tile centres.
	 */
	class GridRayCast {
	public:
		GridRayCast(const NavmapPoint& from, const NavmapPoint& to) noexcept
			: GridRayCast(from.x, from.y, to.x, to.y, SEARCHMAP_SQUARE_WIDTH, SEARCHMAP_SQUARE_HEIGHT) {}

		// tile indices in half tiles, so that a tile centre is still an exact integer
		GridRayCast(const SearchmapPoint& from, const SearchmapPoint& to) noexcept
			: GridRayCast(2 * from.x + 1, 2 * from.y + 1, 2 * to.x + 1, 2 * to.y + 1, 2, 2) {}

		/** Moves onto the next tile on the segment; false once the last one has been handed out. */
		bool Step() noexcept
		{
			// the following branch fires exactly once per walk (the final step) and is perfectly
			// predictable after the first few iterations of a long line, so there is little
			// to nothing to gain by trying to remove/reduce it
			if (current.x == target.x && current.y == target.y) return false;

			// branchless Amanatides & Woo step:
			// Pending axes are 0/1 masks; the comparison produces a 0/1 mask without a branch.
			// A tie (err == 0) takes both axes.
			//
			// `err` is the paper's `tMaxX - tMaxY` scaled to integers: errX * ay - errY * ax.
			// It is carried rather than rebuilt each iteration, because errX only ever grows by
			// cellW and errY by cellH, so err only ever grows by the loop invariants cellW * ay / cellH * ax.
			// That takes the two 64-bit multiplies off the loop-carried dependency chain.
			const int32_t pendingX = current.x != target.x;
			const int32_t pendingY = current.y != target.y;
			const int32_t takeX = pendingX & ((1 - pendingY) | static_cast<int32_t>(err <= 0));
			const int32_t takeY = pendingY & ((1 - pendingX) | static_cast<int32_t>(err >= 0));

			cutACorner = takeX && takeY;

			current.x += stepX * takeX;
			current.y += stepY * takeY;
			err += (errStepX & -static_cast<int64_t>(takeX)) - (errStepY & -static_cast<int64_t>(takeY));
			return true;
		}

		const SearchmapPoint& Current() const noexcept { return current; }

		/** Whether the step just taken went diagonally through the point four tiles share. */
		bool CutACorner() const noexcept { return cutACorner; }

		/** The two tiles the segment passed between; only meaningful after CutACorner(). */
		SearchmapPoint CornerBesideX() const noexcept { return SearchmapPoint(current.x, current.y - stepY); }
		SearchmapPoint CornerBesideY() const noexcept { return SearchmapPoint(current.x - stepX, current.y); }

	private:
		GridRayCast(const int sx, const int sy, const int dx, const int dy, const int w, const int h) noexcept
			: current(sx / w, sy / h),
			  target(dx / w, dy / h),
			  stepX(dx >= sx ? 1 : -1),
			  stepY(dy >= sy ? 1 : -1)
		{
			const int64_t ax = std::abs(dx - sx);
			const int64_t ay = std::abs(dy - sy);
			// the paper's initial tMax, as a distance rather than a t: how far the start
			// lies from the first grid line it will cross on each axis
			const int64_t errX = stepX > 0 ? static_cast<int64_t>(current.x + 1) * w - sx : sx - static_cast<int64_t>(current.x) * w;
			const int64_t errY = stepY > 0 ? static_cast<int64_t>(current.y + 1) * h - sy : sy - static_cast<int64_t>(current.y) * h;
			err = errX * ay - errY * ax;
			errStepX = w * ay;
			errStepY = h * ax;
		}

		SearchmapPoint current;
		SearchmapPoint target;
		// errX * ay - errY * ax, carried; and what a step on each axis adds to it
		int64_t err = 0;
		int64_t errStepX = 0;
		int64_t errStepY = 0;
		int stepX;
		int stepY;
		bool cutACorner = false;
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
