// SPDX-FileCopyrightText: 2026 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef TESTS_SEARCHMAPBUILDER_H
#define TESTS_SEARCHMAPBUILDER_H

#include "../../core/PathFinder.h"
#include "../../core/Region.h"
#include "../../core/Scriptable/Selectable.h"
#include "../../core/TileProps.h"
#include "../../core/TraversabilityCache.h"

#include <gtest/gtest.h>
#include <initializer_list>
#include <string>
#include <vector>

namespace GemRB {
namespace test {

	/**
	 * Define ASCII glyphs for terrain and actors, so that pathfinding tests
	 * won't need real data, and instead define their cases in-place.
	 */
	namespace Glyph {
		// Terrain, input and rendered output
		constexpr char Floor = '.'; // passable floor
		constexpr char Wall = '#'; // impassable and blocks line of sight (SIDEWALL)
		constexpr char Blocked = 'X'; // impassable, but not a sight blocker (plain IMPASSABLE)
		constexpr char Travel = 'T'; // passable travel trigger
		constexpr char Door = 'D'; // passable floor with a closed door on it
		constexpr char OpaqueDoor = 'O'; // as Door, but the door is opaque too

		// Actors, input only. The glyph says how big the actor is, because the footprint
		// PaintSearchMap() stamps is a circle of radius circleSize-1 around the glyph.
		// The tile under an actor is passable floor. Footprints are painted in reading order, so a
		// later actor overwrites the marks of an earlier one where they overlap, same as in game.
		// The same glyphs also populate TestTraversability.
		constexpr char FirstPC = '1'; // '1'..'8', a party member of that circleSize
		constexpr char LastPC = '8';
		constexpr char FirstNPC = 'a'; // 'a'..'h', an NPC of circleSize 1..8
		constexpr char LastNPC = 'h';

		// Special waypoint, which name a spot so a test need not spell out raw coordinates.
		// Both sit on passable floor and carry no searchmap state of their own;
		// at most one of each per map.
		// Render() draws them back, but only over plain floor: over anything else the
		// terrain wins, so a real state is never hidden behind a marker.
		constexpr char Start = 'S'; // where a walk starts, read back with Start()
		constexpr char End = 'E'; // where it should end, read back with End()

		// Output only, how an actor footprint renders back
		constexpr char PCMark = 'P'; // a PC mark on floor
		constexpr char NPCMark = 'N'; // an NPC mark on floor
		constexpr char PCOnTravel = 'p'; // a PC mark on a travel tile
		constexpr char NPCOnTravel = 'n'; // an NPC mark on a travel tile
		constexpr char Bug = '!'; // an actor mark on terrain that cannot be walked on
		constexpr char Unknown = '?'; // a flag combination with no glyph, so an unexpected one

		// Output only, drawn by MatchesWithPath() over everything else, waypoints included.
		// The two are split so a drawing shows what the pathfinder actually decided: it returns
		// only the corners, and everything between them is the line walk filling in.
		constexpr char PathWaypoint = '@'; // a waypoint the pathfinder returned
		constexpr char PathStep = '*'; // a tile walked between two waypoints
	}

	/**
	 * A map drawing, one string per row.
	 */
	using MapRows = std::initializer_list<std::string>;

	inline bool IsActorGlyph(const char c)
	{
		return (c >= Glyph::FirstPC && c <= Glyph::LastPC) || (c >= Glyph::FirstNPC && c <= Glyph::LastNPC);
	}

	/** Waypoints mark spots for the test to read back; they are not terrain of their own. */
	inline bool IsWaypointGlyph(const char c)
	{
		return c == Glyph::Start || c == Glyph::End;
	}

	inline PathMapFlags ActorGlyphFlag(const char c)
	{
		return (c >= Glyph::FirstPC && c <= Glyph::LastPC) ? PathMapFlags::PC : PathMapFlags::NPC;
	}

	inline uint16_t ActorGlyphCircleSize(const char c)
	{
		return (c >= Glyph::FirstPC && c <= Glyph::LastPC) ? uint16_t(c - Glyph::FirstPC + 1) : uint16_t(c - Glyph::FirstNPC + 1);
	}

	inline PathMapFlags ParseSearchMapChar(const char c)
	{
		switch (c) {
			case Glyph::Floor:
				return PathMapFlags::PASSABLE;
			case Glyph::Wall:
				return PathMapFlags::SIDEWALL;
			case Glyph::Blocked:
				return PathMapFlags::IMPASSABLE;
			case Glyph::Travel:
				return PathMapFlags::PASSABLE | PathMapFlags::TRAVEL;
			case Glyph::Door:
				return PathMapFlags::PASSABLE | PathMapFlags::DOOR_IMPASSABLE;
			case Glyph::OpaqueDoor:
				return PathMapFlags::PASSABLE | PathMapFlags::DOOR_IMPASSABLE | PathMapFlags::DOOR_OPAQUE;
			default:
				// actors stand on plain floor; their footprint is painted afterwards
				if (IsActorGlyph(c)) return PathMapFlags::PASSABLE;
				// so do waypoints, which are only markers for the test to read back
				if (IsWaypointGlyph(c)) return PathMapFlags::PASSABLE;

				// anything else is a typo in the literal, not terrain - report test failure
				ADD_FAILURE() << "unknown searchmap glyph '" << c << "'";
				return PathMapFlags::IMPASSABLE;
		}
	}

	/** Inverse of ParseSearchMapChar(), so a whole map can be conveniently diffed as text. */
	inline char RenderSearchMapChar(const PathMapFlags flags)
	{
		const PathMapFlags terrain = flags & PathMapFlags::AREAMASK;
		const PathMapFlags actor = flags & PathMapFlags::ACTOR;
		const PathMapFlags door = flags & PathMapFlags::DOOR;

		if (actor != PathMapFlags::UNMARKED) {
			// an actor mark only belongs on walkable terrain; anywhere else it should be a bug
			if (door != PathMapFlags::UNMARKED || !bool(terrain & PathMapFlags::PASSABLE)) {
				return Glyph::Bug;
			}
			const bool travel = bool(terrain & PathMapFlags::TRAVEL);
			if (actor == PathMapFlags::PC) return travel ? Glyph::PCOnTravel : Glyph::PCMark;
			if (actor == PathMapFlags::NPC) return travel ? Glyph::NPCOnTravel : Glyph::NPCMark;
			return Glyph::Bug; // both actor bits at once, which nothing ever paints
		}

		if (bool(door & PathMapFlags::DOOR_IMPASSABLE)) {
			return bool(door & PathMapFlags::DOOR_OPAQUE) ? Glyph::OpaqueDoor : Glyph::Door;
		}

		if (terrain == PathMapFlags::IMPASSABLE) return Glyph::Blocked;
		if (terrain == PathMapFlags::PASSABLE) return Glyph::Floor;
		if (terrain == PathMapFlags::SIDEWALL) return Glyph::Wall;
		if (terrain == (PathMapFlags::PASSABLE | PathMapFlags::TRAVEL)) return Glyph::Travel;
		return Glyph::Unknown; // any unhandled above combination is treated as unknown and should be investigated
	}

	/**
	 * A stand-in for an actor, for the cache's identity slot and ActorPathContext. FindPath()
	 * only ever compares these pointers - never dereferences them - so any distinct address
	 * will do. Use MakeActorIdentity() to get one.
	 */
	using ActorIdentity = const Movable*;

	/** A distinct, never-dereferenced actor identity. Pass the same tag to keep the same one. */
	inline ActorIdentity MakeActorIdentity(const char& tag)
	{
		// deliberate unsafe cast - in tests we don't use real actor instances,
		// we just need a number for the sake of identity comparison
		return reinterpret_cast<ActorIdentity>(&tag); // NOSONAR
	}

	/**
	 * Whether a drawing paints its actor glyphs into the searchmap.
	 *
	 * Paint is designed for a terrain-only test, where nothing else will ever mark those tiles.
	 * Skip is intended for a live Map, where we expect the state to be driven by the actors in a game loop.
	 */
	enum class ActorPainting {
		Paint,
		Skip
	};

	/**
	 * Builds TileProps out of an ASCII drawing, so pathfinder tests can state their terrain
	 * inline instead of shipping a searchmap BMP. Only the searchmap byte is filled in; the
	 * pathfinder never reads the material, height or light channels.
	 *
	 * One character is one searchmap tile, which is 16x12 navmap pixels.
	 */
	class TestSearchMap {
	public:
		struct DrawnActor {
			SearchmapPoint tile;
			uint16_t circleSize;
			PathMapFlags flag; // PC or NPC
		};

		TestSearchMap(const MapRows inMapRows, const ActorPainting painting = ActorPainting::Paint)
		{
			const int rowCount = static_cast<int>(inMapRows.size());
			const int rowWidth = rowCount ? static_cast<int>(inMapRows.begin()->size()) : 0;

			// verify we have all lines the same length
			for (const std::string& row : inMapRows) {
				if (static_cast<int>(row.size()) != rowWidth) {
					ADD_FAILURE() << "every map row must be " << rowWidth
						      << " characters wide, got " << row;
					return;
				}
			}

			height = rowCount;
			width = rowWidth;

			props = OwningTileProps::MakeEmpty(Size(width, height));

			int y = 0;
			// parse chars one by one and write it as flags
			// It builds the terrain first, actors are not marked here yet
			for (const std::string& row : inMapRows) {
				for (int x = 0; x < width; ++x) {
					const SearchmapPoint tile(x, y);
					props.SetTileProp(tile, TileProps::Property::SEARCH_MAP,
							  uint8_t(ParseSearchMapChar(row[x])));
					if (IsActorGlyph(row[x])) {
						drawnActors.push_back({ tile, ActorGlyphCircleSize(row[x]),
									ActorGlyphFlag(row[x]) });
					} else if (row[x] == Glyph::Start) {
						if (hasStart) ADD_FAILURE() << "a map can only carry one start point";
						start = tile;
						hasStart = true;
					} else if (row[x] == Glyph::End) {
						if (hasEnd) ADD_FAILURE() << "a map can only carry one end point";
						end = tile;
						hasEnd = true;
					}
				}
				++y;
			}

			// only after the terrain is ready, mark the actors
			if (painting == ActorPainting::Paint) {
				for (const DrawnActor& actor : drawnActors) {
					props.PaintSearchMap(actor.tile, actor.circleSize, actor.flag);
				}
			}
		}

		TileProps& Props() noexcept { return props; }
		const TileProps& Props() const noexcept { return props; }

		int Width() const noexcept { return width; }
		int Height() const noexcept { return height; }

		PathMapFlags At(int x, int y) const noexcept
		{
			return props.QuerySearchMap(SearchmapPoint(x, y));
		}

		/** Every actor glyph the drawing carried, in reading order. */
		const std::vector<DrawnActor>& Actors() const noexcept { return drawnActors; }

		/** Centre of the nth drawn actor's tile, in navmap coordinates. */
		Point ActorPosOf(const size_t index) const
		{
			if (index >= drawnActors.size()) {
				ADD_FAILURE() << "this map has no actor number " << index;
				return {};
			}
			const SearchmapPoint& tile = drawnActors[index].tile;
			return Nav(tile.x, tile.y);
		}

		/** Identity of the nth drawn actor */
		ActorIdentity ActorIdentityOf(const size_t index) const
		{
			if (index >= drawnActors.size()) {
				ADD_FAILURE() << "this map has no actor number " << index;
				return nullptr;
			}
			// deliberate unsafe cast - in tests we don't use real actor instances,
			// we just need a number for the sake of identity comparison
			return reinterpret_cast<ActorIdentity>(&drawnActors[index]); // NOSONAR
		}

		/** Circle size of the nth drawn actor */
		uint16_t ActorCircleSizeOf(const size_t index) const
		{
			if (index >= drawnActors.size()) {
				ADD_FAILURE() << "this map has no actor number " << index;
				return 1;
			}
			return drawnActors[index].circleSize;
		}

		/** Centre of the start point tile, in navmap coordinates. */
		Point Start() const
		{
			if (!hasStart) ADD_FAILURE() << "this map has no start point defined";
			return Nav(start.x, start.y);
		}

		/** Centre of the end point tile, in navmap coordinates. */
		Point End() const
		{
			if (!hasEnd) ADD_FAILURE() << "this map has no end point defined";
			return Nav(end.x, end.y);
		}

		/** The map as it stands now, one string per row, in the glyphs listed above. */
		std::vector<std::string> Render() const
		{
			std::vector<std::string> out;
			out.reserve(height);
			for (int y = 0; y < height; ++y) {
				std::string row;
				row.reserve(width);
				for (int x = 0; x < width; ++x) {
					char glyph = RenderSearchMapChar(At(x, y));
					// waypoints are drawn back in, but only over plain floor: real
					// searchmap state must never hide behind a marker
					if (glyph == Glyph::Floor) {
						if (hasStart && start == SearchmapPoint(x, y)) glyph = Glyph::Start;
						if (hasEnd && end == SearchmapPoint(x, y)) glyph = Glyph::End;
					}
					row.push_back(glyph);
				}
				out.push_back(std::move(row));
			}
			return out;
		}

		/**
		 * Compares the current state against a drawing of the expected one. Meant for
		 * EXPECT_TRUE(map.Matches({...})) - this allows to:
		 * a) assert on expected ASCII drawing of the map, instead of series of asserts on coords,
		 * b) print nice message on failure.
		 */
		testing::AssertionResult Matches(MapRows expected) const;

		/**
		 * As Matches(), but with the walked route drawn over the terrain: Glyph::PathWaypoint on
		 * waypoints pathfinder returned, Glyph::PathStep on the tiles the line walk crosses
		 * to get between them.
		 * This allows to state the route test expects as a picture instead of a
		 * list of coordinates, and shows at a glance how much of it the pathfinder chose.
		 */
		testing::AssertionResult MatchesWithPath(const Point& from, const Path& path, MapRows expected) const;

		// centre of a tile in navmap coordinates, which is what the Point-taking overloads want
		static Point Nav(int x, int y) noexcept
		{
			return Point(x * 16 + 8, y * 12 + 6);
		}

	private:
		testing::AssertionResult CompareRows(const std::vector<std::string>& actual, MapRows expected) const;

		OwningTileProps props;
		std::vector<DrawnActor> drawnActors;
		int width = 0;
		int height = 0;
		SearchmapPoint start;
		SearchmapPoint end;
		bool hasStart = false;
		bool hasEnd = false;
	};

	/**
	 * The traversability cache mockup for test, constructed from a drawn map.
	 * Needed for FindPath().
	 *
	 * Mimics token count mechanic for cells from real TraversabilityCache.
	 * For simplicity owns the pool its pages come from, so it can be neither copied nor moved.
	 */
	class TestTraversability {
	public:
		/**
		 * Populated from the map's own actor glyphs, so a drawing states the actors once and
		 * both layers agree on them. Bumpability is not something a glyph carries, so it is
		 * given here for all of them at once; AddActor() still covers anything finer.
		 */
		explicit TestTraversability(const TestSearchMap& map, bool actorsAreBumpable = true)
			: navWidth(map.Width() * 16),
			  navHeight(map.Height() * 12),
			  // the trailing spare cell matches TraversabilityCache::ValidateTraversabilityCacheSize(),
			  // which keeps one as a dumpster for out-of-range writes
			  data(pool, size_t(navWidth) * navHeight + 1)
		{
			for (size_t i = 0; i < map.Actors().size(); ++i) {
				const auto& drawn = map.Actors()[i];
				AddActor(map.ActorPosOf(i), drawn.circleSize, actorsAreBumpable, map.ActorIdentityOf(i));
			}
		}

		TestTraversability(const TestTraversability&) = delete;
		TestTraversability& operator=(const TestTraversability&) = delete;

		/**
		 * Stamps an actor's ground circle into the cache, the same footprint and token value
		 * TraversabilityCache::Update() would give it.
		 */
		void AddActor(const Point& pos, int circleSize, bool bumpable, ActorIdentity who = nullptr)
		{
			const int baseSize = Selectable::CircleSize2Radius(circleSize);
			const Size shape(baseSize * 8, baseSize * 6);
			const Point origin = pos - shape.Center();
			const auto token = bumpable ? TraversabilityCache::TraversabilityCellValueActor : TraversabilityCache::TraversabilityCellValueActorNonTraversable;

			for (int y = 0; y < shape.h; ++y) {
				for (int x = 0; x < shape.w; ++x) {
					const Point cell(origin.x + x, origin.y + y);
					if (cell.x < 0 || cell.y < 0 || cell.x >= navWidth || cell.y >= navHeight) continue;
					if (!Selectable::IsOverCircle(cell, pos, circleSize)) continue;

					const size_t idx = size_t(cell.y) * navWidth + cell.x;
					TraversabilityCache::TraversabilityCellData cellData = data[idx];
					cellData.state += token;
					// deliberate unsafe cast - in tests we don't use real actor instances,
					// we just need a number for the sake of identity comparison
					cellData.occupyingActor = reinterpret_cast<Actor*>(const_cast<Movable*>(who)); // NOSONAR
					data[idx] = cellData;
				}
			}
		}

		TraversabilityCache::TraversabilityCellState StateAt(const Point& navPoint) const
		{
			return data[size_t(navPoint.y) * navWidth + navPoint.x].state;
		}

		/** Who the cache has standing on that navmap pixel, which is what FindPath() compares. */
		ActorIdentity ActorAt(const Point& navPoint) const
		{
			const auto actorPtr = data[size_t(navPoint.y) * navWidth + navPoint.x].occupyingActor;
			// deliberate unsafe cast - in tests we don't use real actor instances,
			// we just need a number for the sake of identity comparison
			return reinterpret_cast<ActorIdentity>(actorPtr); // NOSONAR
		}

		const TraversabilityCache::Data_t& Data() const noexcept { return data; }

	private:
		int navWidth = 0;
		int navHeight = 0;
		FixedSizePool<TraversabilityCache::Data_t::TPage_t> pool;
		TraversabilityCache::Data_t data;
	};

	/**
	 * Runs the real FindPath() over a drawn map.
	 *
	 * Speed 0 keeps it clear of gamedata->GetStepTime(), so no Interface is needed.
	 */
	inline Path CallFindPath(const TestSearchMap& map, const TestTraversability& traversability,
				 const Point& from, const Point& to, ActorIdentity self = nullptr,
				 unsigned int circleSize = 1, int flags = PF_SIGHT)
	{
		ActorPathContext actor;
		actor.circleSize = circleSize;
		actor.identity = self;
		return PathFinder::FindPath(traversability.Data(), map.Props(), from, to, actor, 0, flags);
	}

	/**
	 * Finds a path from the nth drawn actor to `to`.
	 */
	inline Path PathDrawnActor(const TestSearchMap& map, const TestTraversability& traversability,
				   size_t index, const Point& to, int flags = PF_SIGHT)
	{
		return CallFindPath(map, traversability, map.ActorPosOf(index), to,
				    map.ActorIdentityOf(index), map.ActorCircleSizeOf(index), flags);
	}

	/**
	 * Every tile a path passes through, source included, in walking order.
	 */
	inline std::vector<SearchmapPoint> PathTiles(const Point& from, const Path& path)
	{
		std::vector<SearchmapPoint> tiles;
		auto appendIfNotPresent = [&tiles](const SearchmapPoint& tile) {
			if (tiles.empty() || !(tiles.back() == tile)) tiles.push_back(tile);
		};

		Point p = from;
		appendIfNotPresent(SearchmapPoint { p });
		for (size_t i = 0; i < path.Size(); ++i) {
			PathFinder::LineStepper<NavmapPoint> walk { p, path.GetStep(i).point };
			while (walk.Step()) {
				if (walk.Current() == p) {
					// a step should always move, bail out if that ever stops holding
					ADD_FAILURE() << "LineStepper makes no progress for point " << i << "  at ("
						      << p.x << ',' << p.y << ')';
					return tiles;
				}
				p = walk.Current();
				appendIfNotPresent(SearchmapPoint { p });
			}
		}
		return tiles;
	}

	/**
	 * Checks a path can actually be walked: no leg crosses a wall or a shut door.
	 */
	inline testing::AssertionResult PathAvoidsWalls(const TestSearchMap& map, const Point& from, const Path& path)
	{
		if (path.Empty()) {
			return testing::AssertionFailure() << "path is empty";
		}

		Point previous = from;
		for (size_t i = 0; i < path.Size(); ++i) {
			const Point step = path.GetStep(i).point;
			const PathMapFlags leg = PathFinder::GetBlockedInLine(map.Props(), previous, step, false, 0, 0);
			if (bool(leg & (PathMapFlags::SIDEWALL | PathMapFlags::DOOR_IMPASSABLE))) {
				return testing::AssertionFailure()
					<< "leg " << i << ", (" << previous.x << ',' << previous.y << ") -> ("
					<< step.x << ',' << step.y << "), crosses a wall";
			}
			previous = step;
		}
		return testing::AssertionSuccess();
	}

	inline testing::AssertionResult TestSearchMap::MatchesWithPath(const Point& from, const Path& path,
								       const MapRows expected) const
	{
		std::vector<std::string> drawn = Render();
		auto plot = [&drawn, this](const SearchmapPoint& tile, char glyph) {
			if (tile.x >= 0 && tile.x < width && tile.y >= 0 && tile.y < height) {
				drawn[tile.y][tile.x] = glyph;
			}
		};

		// the walked tiles first, so that a waypoint is never buried under one of them
		for (const SearchmapPoint& tile : PathTiles(from, path)) {
			plot(tile, Glyph::PathStep);
		}
		for (size_t i = 0; i < path.Size(); ++i) {
			plot(SearchmapPoint { path.GetStep(i).point }, Glyph::PathWaypoint);
		}
		return CompareRows(drawn, expected);
	}

	inline testing::AssertionResult TestSearchMap::Matches(const MapRows expected) const
	{
		return CompareRows(Render(), expected);
	}

	inline testing::AssertionResult TestSearchMap::CompareRows(const std::vector<std::string>& actual,
								   const MapRows expected) const
	{
		const std::vector<std::string> want(expected.begin(), expected.end());

		if (want.size() != actual.size()) {
			return testing::AssertionFailure()
				<< "expected " << want.size() << " rows, the map has " << actual.size();
		}

		std::vector<int> badRows;
		for (size_t y = 0; y < want.size(); ++y) {
			if (want[y] != actual[y]) badRows.push_back(static_cast<int>(y));
		}
		if (badRows.empty()) return testing::AssertionSuccess();

		const size_t column = std::max<size_t>(size_t(width), 8) + 6;
		std::string msg = "\nsearchmap mismatch on " + std::to_string(badRows.size()) + " row(s)\n";
		msg += "  expected" + std::string(column - 8, ' ') + "actual\n";
		for (size_t y = 0; y < want.size(); ++y) {
			std::string line = "  " + want[y];
			line.resize(column + 2, ' ');
			line += actual[y];
			if (want[y] != actual[y]) line += "   <-- differs";
			msg += line + "\n";
		}

		bool sawBug = false;
		bool sawUnknown = false;
		for (const std::string& row : actual) {
			sawBug = sawBug || row.find(Glyph::Bug) != std::string::npos;
			sawUnknown = sawUnknown || row.find(Glyph::Unknown) != std::string::npos;
		}
		if (sawBug) msg += "  '!' = actor mark on terrain that cannot be walked on\n";
		if (sawUnknown) msg += "  '?' = unexpected flag combination, needs investigation\n";

		return testing::AssertionFailure() << msg;
	}
}
}

#endif // TESTS_SEARCHMAPBUILDER_H
