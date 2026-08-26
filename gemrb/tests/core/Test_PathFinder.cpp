// SPDX-FileCopyrightText: 2026 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

// Terrain-level pathfinder tests. Everything here runs on a TileProps built from an ASCII
// literal, so no game data, no Interface and no video driver are involved.
// See SearchMapBuilder.h for the glyphs.

#include "SearchMapBuilder.h"

// only for the GL_ flags CalculateLinePath() takes; nothing here builds a Map
#include "../../core/Map.h"
#include "../../core/PathFinder.h"

#include <algorithm>
#include <gtest/gtest-spi.h>
#include <gtest/gtest.h>
#include <string>

namespace GemRB {

using test::TestSearchMap;
namespace Glyph = test::Glyph;

// Actor speed 0 keeps GetBlockedInLine() away from gamedata->GetStepTime(), which needs a
// live Interface. It only sets the step length of the line walk, so a value of 0 (one navmap
// pixel per step).
constexpr int noSpeed = 0;
constexpr int noCircle = 0;

// === TestSearchMap infrastructure tests ===
// Tests below check the test infrastructure itself - defined ASCII layer against raw flags, so that everything
// after them can be written and read as maps.
TEST(SearchMapBuilderTest, ParsesTerrainCharacters)
{
	const TestSearchMap map {
		"#.XT",
		"#.DO"
	};

	EXPECT_EQ(map.At(0, 0), PathMapFlags::SIDEWALL);
	EXPECT_EQ(map.At(1, 0), PathMapFlags::PASSABLE);
	EXPECT_EQ(map.At(2, 0), PathMapFlags::IMPASSABLE);
	EXPECT_EQ(map.At(3, 0), PathMapFlags::PASSABLE | PathMapFlags::TRAVEL);


	EXPECT_EQ(map.At(0, 1), PathMapFlags::SIDEWALL);
	EXPECT_EQ(map.At(1, 1), PathMapFlags::PASSABLE);
	EXPECT_EQ(map.At(2, 1), PathMapFlags::PASSABLE | PathMapFlags::DOOR_IMPASSABLE);
	EXPECT_EQ(map.At(3, 1), PathMapFlags::PASSABLE | PathMapFlags::DOOR_IMPASSABLE | PathMapFlags::DOOR_OPAQUE);

	// out of bounds should be impassable, not a crash
	EXPECT_EQ(map.At(99, 99), PathMapFlags::IMPASSABLE);
}

// A malformed literal is a mistake in the test, check if it correctly reports failure
TEST(SearchMapBuilderTest, RejectsMalformedMaps)
{
	EXPECT_NONFATAL_FAILURE(test::ParseSearchMapChar('{'), "unknown searchmap glyph '{'");

	const test::MapRows raggedRows {
		"###",
		"#.#",
		"##"
	};
	EXPECT_NONFATAL_FAILURE(TestSearchMap { raggedRows },
				"every map row must be 3 characters wide");
}

// An actor glyph carries two things: which side it belongs to and how big it is.
TEST(SearchMapBuilderTest, ParsesActorCharacters)
{
	for (char g = Glyph::FirstPC; g <= Glyph::LastPC; ++g) {
		EXPECT_TRUE(test::IsActorGlyph(g));
	}
	for (char g = Glyph::FirstNPC; g <= Glyph::LastNPC; ++g) {
		EXPECT_TRUE(test::IsActorGlyph(g));
	}

	// terrain never counts as an actor
	EXPECT_FALSE(test::IsActorGlyph(Glyph::Floor));
	EXPECT_FALSE(test::IsActorGlyph(Glyph::Wall));
	EXPECT_FALSE(test::IsActorGlyph(Glyph::Door));

	// sanity: just outside either range
	EXPECT_FALSE(test::IsActorGlyph(char(Glyph::FirstPC - 1)));
	EXPECT_FALSE(test::IsActorGlyph(char(Glyph::LastPC + 1)));
	EXPECT_FALSE(test::IsActorGlyph(char(Glyph::FirstNPC - 1)));
	EXPECT_FALSE(test::IsActorGlyph(char(Glyph::LastNPC + 1)));

	// digits are party members, letters are NPCs
	for (char g = Glyph::FirstPC; g <= Glyph::LastPC; ++g) {
		EXPECT_EQ(test::ActorGlyphFlag(g), PathMapFlags::PC);
	}
	for (char g = Glyph::FirstNPC; g <= Glyph::LastNPC; ++g) {
		EXPECT_EQ(test::ActorGlyphFlag(g), PathMapFlags::NPC);
	}
	// both alphabets run over the same circleSize range of 1 to 8
	uint16_t circleSize;
	char glyph;
	for (circleSize = 1, glyph = Glyph::FirstPC; glyph <= Glyph::LastPC; ++glyph, ++circleSize) {
		EXPECT_EQ(test::ActorGlyphCircleSize(glyph), circleSize);
	}
	for (circleSize = 1, glyph = Glyph::FirstNPC; glyph <= Glyph::LastNPC; ++glyph, ++circleSize) {
		EXPECT_EQ(test::ActorGlyphCircleSize(glyph), circleSize);
	}

	// the terrain an actor stands on is plain floor; the footprint is painted afterwards
	for (char g = Glyph::FirstPC; g <= Glyph::LastPC; ++g) {
		EXPECT_EQ(test::ParseSearchMapChar(g), PathMapFlags::PASSABLE);
	}
	for (char g = Glyph::FirstNPC; g <= Glyph::LastNPC; ++g) {
		EXPECT_EQ(test::ParseSearchMapChar(g), PathMapFlags::PASSABLE);
	}
}

// test named start and end waypoint glyphs
TEST(SearchMapBuilderTest, ParsesWaypointGlyphs)
{
	const TestSearchMap map {
		"#####",
		"#S.E#",
		"#####"
	};

	EXPECT_EQ(map.Start(), TestSearchMap::Nav(1, 1));
	EXPECT_EQ(map.End(), TestSearchMap::Nav(3, 1));

	// they are markers, not terrain: the tiles under them are plain floor
	EXPECT_EQ(map.At(1, 1), PathMapFlags::PASSABLE);
	EXPECT_EQ(map.At(3, 1), PathMapFlags::PASSABLE);
	EXPECT_EQ(test::ParseSearchMapChar(Glyph::Start), PathMapFlags::PASSABLE);
	EXPECT_EQ(test::ParseSearchMapChar(Glyph::End), PathMapFlags::PASSABLE);

	// and they are drawn back in render
	const test::MapRows expected {
		"#####",
		"#S.E#",
		"#####"
	};
	EXPECT_TRUE(map.Matches(expected));
}

// A waypoint is an annotation, so it must never hide what the searchmap actually holds
TEST(SearchMapBuilderTest, RealStateWinsOverWaypoints)
{
	// circleSize 2 footprint is a 3x3 block, so it should spill onto both waypoint tiles
	const TestSearchMap map {
		"#####",
		"#S2E#",
		"#####"
	};
	const test::MapRows expected {
		"#####",
		"#PPP#",
		"#####"
	};
	EXPECT_TRUE(map.Matches(expected));

	// the coordinates still read back, the drawing just does not show them
	EXPECT_EQ(map.Start(), TestSearchMap::Nav(1, 1));
	EXPECT_EQ(map.End(), TestSearchMap::Nav(3, 1));
}

TEST(SearchMapBuilderTest, RejectsDuplicateOrMissingWaypoints)
{
	const test::MapRows twoStarts {
		"#####",
		"#S.S#",
		"#####"
	};
	EXPECT_NONFATAL_FAILURE(TestSearchMap { twoStarts }, "only carry one start point");

	const test::MapRows twoEnds {
		"#####",
		"#E.E#",
		"#####"
	};
	EXPECT_NONFATAL_FAILURE(TestSearchMap { twoEnds }, "only carry one end point");

	const TestSearchMap noWaypoints {
		"###",
		"#.#",
		"###"
	};
	EXPECT_NONFATAL_FAILURE(noWaypoints.Start(), "no start point defined");
	EXPECT_NONFATAL_FAILURE(noWaypoints.End(), "no end point defined");
}

// test map.Actor* accessors
TEST(SearchMapBuilderTest, ReadsBackTheDrawnActors)
{
	const TestSearchMap map {
		".................",
		"..1......c.......",
		".................",
		".................",
		".......4.........",
		".................",
		"................."
	};

	// reading order is row by row, left to right, whatever the glyph or its size
	ASSERT_EQ(map.Actors().size(), 3);
	EXPECT_EQ(map.Actors()[0].tile, SearchmapPoint(2, 1));
	EXPECT_EQ(map.Actors()[1].tile, SearchmapPoint(9, 1));
	EXPECT_EQ(map.Actors()[2].tile, SearchmapPoint(7, 4));

	// ActorPosOf() is that tile's centre in navmap coordinates
	EXPECT_EQ(map.ActorPosOf(0), TestSearchMap::Nav(2, 1));
	EXPECT_EQ(map.ActorPosOf(1), TestSearchMap::Nav(9, 1));
	EXPECT_EQ(map.ActorPosOf(2), TestSearchMap::Nav(7, 4));

	// ActorCircleSizeOf() is what the glyph spelled, across both alphabets
	EXPECT_EQ(map.ActorCircleSizeOf(0), 1);
	EXPECT_EQ(map.ActorCircleSizeOf(1), 3);
	EXPECT_EQ(map.ActorCircleSizeOf(2), 4);

	// digits are party members, letters are NPCs
	EXPECT_EQ(map.Actors()[0].flag, PathMapFlags::PC);
	EXPECT_EQ(map.Actors()[1].flag, PathMapFlags::NPC);
	EXPECT_EQ(map.Actors()[2].flag, PathMapFlags::PC);

	// ActorIdentityOf() gives each actor one of its own, and the same one every time
	EXPECT_NE(map.ActorIdentityOf(0), nullptr);
	EXPECT_NE(map.ActorIdentityOf(1), nullptr);
	EXPECT_NE(map.ActorIdentityOf(2), nullptr);
	EXPECT_NE(map.ActorIdentityOf(0), map.ActorIdentityOf(1));
	EXPECT_NE(map.ActorIdentityOf(1), map.ActorIdentityOf(2));
	EXPECT_NE(map.ActorIdentityOf(0), map.ActorIdentityOf(2));
	EXPECT_EQ(map.ActorIdentityOf(0), map.ActorIdentityOf(0)) << "an identity has to be stable";
}

// Asking for an actor a drawing does not have is a mistake in the test
TEST(SearchMapBuilderTest, RejectsOutOfRangeActorIndex)
{
	const TestSearchMap map {
		"...",
		".1.",
		"..."
	};
	ASSERT_EQ(map.Actors().size(), size_t(1));

	EXPECT_NONFATAL_FAILURE(map.ActorPosOf(1), "no actor number 1");
	EXPECT_NONFATAL_FAILURE(map.ActorCircleSizeOf(1), "no actor number 1");
	EXPECT_NONFATAL_FAILURE(map.ActorIdentityOf(1), "no actor number 1");

	const TestSearchMap noActors {
		"...",
		"...",
		"..."
	};
	ASSERT_TRUE(noActors.Actors().empty());
	EXPECT_NONFATAL_FAILURE(noActors.ActorPosOf(0), "no actor number 0");
}

// The glyphs have to reach the searchmap as the right actor bits over the right area.
TEST(SearchMapBuilderTest, PaintsActorFootprintsFromGlyphs)
{
	const TestSearchMap map {
		"........",
		"..1..a..",
		"........"
	};

	// circleSize 1 is a radius 0 circle, so each actor marks its own tile and nothing else
	EXPECT_EQ(map.At(2, 1), PathMapFlags::PASSABLE | PathMapFlags::PC);
	EXPECT_EQ(map.At(5, 1), PathMapFlags::PASSABLE | PathMapFlags::NPC);

	EXPECT_EQ(map.At(1, 1), PathMapFlags::PASSABLE);
	EXPECT_EQ(map.At(3, 1), PathMapFlags::PASSABLE);
	EXPECT_EQ(map.At(2, 0), PathMapFlags::PASSABLE);
	EXPECT_EQ(map.At(2, 2), PathMapFlags::PASSABLE);
	EXPECT_EQ(map.At(4, 1), PathMapFlags::PASSABLE);
	EXPECT_EQ(map.At(6, 1), PathMapFlags::PASSABLE);

	// a letter one step up the alphabet is one circleSize bigger, same as the digits
	const TestSearchMap bigger {
		".......",
		".......",
		"...b...",
		".......",
		"......."
	};

	// circleSize 2 fills the 3x3 block around the glyph
	for (int y = 1; y <= 3; ++y) {
		for (int x = 2; x <= 4; ++x) {
			EXPECT_EQ(bigger.At(x, y), PathMapFlags::PASSABLE | PathMapFlags::NPC)
				<< "at (" << x << "," << y << ')';
		}
	}
	EXPECT_EQ(bigger.At(1, 2), PathMapFlags::PASSABLE);
	EXPECT_EQ(bigger.At(5, 2), PathMapFlags::PASSABLE);
	EXPECT_EQ(bigger.At(3, 0), PathMapFlags::PASSABLE);
	EXPECT_EQ(bigger.At(3, 4), PathMapFlags::PASSABLE);
}

// Check if we correctly render the path between waypoints
TEST(SearchMapBuilderTest, DrawsWaypointsApartFromTheStepsBetweenThem)
{
	const TestSearchMap map {
		".......",
		".......",
		".......",
		".......",
		"......."
	};

	Path path;
	path.AppendStep(PathNode { TestSearchMap::Nav(5, 1), orient_t::E });
	path.AppendStep(PathNode { TestSearchMap::Nav(5, 3), orient_t::S });

	// two corners as PathWaypoint, everything the walk crosses to reach them as PathStep;
	// the tile it starts on is not a waypoint, so it draws as a step too
	const test::MapRows expected {
		".......",
		".****@.",
		".....*.",
		".....@.",
		"......."
	};
	EXPECT_TRUE(map.MatchesWithPath(TestSearchMap::Nav(1, 1), path, expected));
}

TEST(SearchMapBuilderTest, RendersFlagsBackToGlyphs)
{
	EXPECT_EQ(test::RenderSearchMapChar(PathMapFlags::PASSABLE), Glyph::Floor);
	EXPECT_EQ(test::RenderSearchMapChar(PathMapFlags::SIDEWALL), Glyph::Wall);
	EXPECT_EQ(test::RenderSearchMapChar(PathMapFlags::IMPASSABLE), Glyph::Blocked);
	EXPECT_EQ(test::RenderSearchMapChar(PathMapFlags::PASSABLE | PathMapFlags::TRAVEL), Glyph::Travel);
	EXPECT_EQ(test::RenderSearchMapChar(PathMapFlags::PASSABLE | PathMapFlags::PC), Glyph::PCMark);
	EXPECT_EQ(test::RenderSearchMapChar(PathMapFlags::PASSABLE | PathMapFlags::NPC), Glyph::NPCMark);
	EXPECT_EQ(test::RenderSearchMapChar(PathMapFlags::PASSABLE | PathMapFlags::TRAVEL | PathMapFlags::PC), Glyph::PCOnTravel);

	// an actor mark somewhere it can never legally be
	EXPECT_EQ(test::RenderSearchMapChar(PathMapFlags::SIDEWALL | PathMapFlags::PC), Glyph::Bug);
	EXPECT_EQ(test::RenderSearchMapChar(PathMapFlags::PC), Glyph::Bug);
	EXPECT_EQ(test::RenderSearchMapChar(PathMapFlags::PASSABLE | PathMapFlags::DOOR_IMPASSABLE | PathMapFlags::NPC), Glyph::Bug);
}

TEST(SearchMapBuilderTest, TerrainRoundTrips)
{
	const TestSearchMap map {
		"#######",
		"#..X..#",
		"#.TTT.#",
		"#..D..#",
		"#######"
	};
	const test::MapRows expected {
		"#######",
		"#..X..#",
		"#.TTT.#",
		"#..D..#",
		"#######"
	};
	EXPECT_TRUE(map.Matches(expected));
}

// === the traversability cache ===
// The drawn actors have to reach the cache, not just the searchmap.
TEST(TraversabilityTest, DrawnActorsPopulateTheCache)
{
	const TestSearchMap map {
		"#######",
		"#.1.a.#",
		"#######"
	};

	ASSERT_EQ(map.Actors().size(), size_t(2));
	EXPECT_EQ(map.Actors()[0].flag, PathMapFlags::PC);
	EXPECT_EQ(map.Actors()[1].flag, PathMapFlags::NPC);

	const test::TestTraversability bumpable { map, true };
	EXPECT_EQ(bumpable.StateAt(map.ActorPosOf(0)), TraversabilityCache::TraversabilityCellValueActor);
	EXPECT_EQ(bumpable.StateAt(map.ActorPosOf(1)), TraversabilityCache::TraversabilityCellValueActor);
	EXPECT_EQ(bumpable.StateAt(TestSearchMap::Nav(3, 1)), TraversabilityCache::TraversabilityCellValueEmpty) << "the gap between them is clear";

	// the same drawing, with nothing that can be shoved aside
	const test::TestTraversability solid { map, false };
	EXPECT_EQ(solid.StateAt(map.ActorPosOf(0)), TraversabilityCache::TraversabilityCellValueActorNonTraversable);
	EXPECT_EQ(solid.StateAt(map.ActorPosOf(1)), TraversabilityCache::TraversabilityCellValueActorNonTraversable);

	// each drawn actor gets its own identity, so they can tell each other apart
	EXPECT_NE(map.ActorIdentityOf(0), map.ActorIdentityOf(1));
	EXPECT_NE(map.ActorIdentityOf(0), nullptr);

	// the identity belongs to the drawing rather than to a cache, so it is the one that lands
	// in the cells and every cache built over this map names the same actor
	EXPECT_EQ(bumpable.ActorAt(map.ActorPosOf(0)), map.ActorIdentityOf(0));
	EXPECT_EQ(solid.ActorAt(map.ActorPosOf(0)), map.ActorIdentityOf(0));
	EXPECT_EQ(bumpable.ActorAt(map.ActorPosOf(1)), map.ActorIdentityOf(1));
}

// The cache, not the searchmap, is what makes an actor stop a route. A bumpable one only
// blocks while actors are blocking; a non-bumpable one always does.
TEST(TraversabilityTest, ActorTokensLandOnTheActorsCircle)
{
	const TestSearchMap map {
		"#####",
		"#...#",
		"#.1.#",
		"#####"
	};

	// the drawn '1' populates the cache too, so it is stated once
	test::TestTraversability traversability { map };
	const Point where = map.ActorPosOf(0);

	// the centre carries a bumpable actor's single token ...
	EXPECT_EQ(traversability.StateAt(where), TraversabilityCache::TraversabilityCellValueActor);
	// ... and somewhere well outside the circle carries none
	EXPECT_EQ(traversability.StateAt(TestSearchMap::Nav(1, 1)), TraversabilityCache::TraversabilityCellValueEmpty);

	// a second, non-bumpable actor on the same spot pushes it over the always-blocking mark
	traversability.AddActor(where, 1, false);
	EXPECT_GE(traversability.StateAt(where), TraversabilityCache::TraversabilityCellValueActorNonTraversable);
}

// === Actual useful pathfinding-related tests ===

// IsLineWalkable() decides on flags OR-accumulated over a whole line, so a single ACTOR tile
// sets the bit for the entire line. Ignoring actors must not therefore ignore the walls.
TEST(PathFinderTest, IsLineWalkableRespectsGeometry)
{
	constexpr bool ActorsAreBlocking = true;
	constexpr bool ActorsAreNOTBlocking = false;
	// plain floor
	EXPECT_TRUE(PathFinder::IsLineWalkable(PathMapFlags::PASSABLE, ActorsAreBlocking));
	EXPECT_TRUE(PathFinder::IsLineWalkable(PathMapFlags::PASSABLE, ActorsAreNOTBlocking));

	// nothing walkable on the line at all
	EXPECT_FALSE(PathFinder::IsLineWalkable(PathMapFlags::IMPASSABLE, ActorsAreBlocking));
	EXPECT_FALSE(PathFinder::IsLineWalkable(PathMapFlags::IMPASSABLE, ActorsAreNOTBlocking));

	// an actor blocks only while actors are blocking
	EXPECT_FALSE(PathFinder::IsLineWalkable(PathMapFlags::ACTOR, ActorsAreBlocking));
	EXPECT_TRUE(PathFinder::IsLineWalkable(PathMapFlags::ACTOR, ActorsAreNOTBlocking));
	EXPECT_TRUE(PathFinder::IsLineWalkable(PathMapFlags::PC, ActorsAreNOTBlocking));
	EXPECT_TRUE(PathFinder::IsLineWalkable(PathMapFlags::NPC, ActorsAreNOTBlocking));

	// walls block regardless
	EXPECT_FALSE(PathFinder::IsLineWalkable(PathMapFlags::SIDEWALL, ActorsAreBlocking));
	EXPECT_FALSE(PathFinder::IsLineWalkable(PathMapFlags::SIDEWALL, ActorsAreNOTBlocking));
	EXPECT_FALSE(PathFinder::IsLineWalkable(PathMapFlags::DOOR_IMPASSABLE, ActorsAreBlocking));
	EXPECT_FALSE(PathFinder::IsLineWalkable(PathMapFlags::DOOR_IMPASSABLE, ActorsAreNOTBlocking));

	// an actor standing next to a wall must not open a hole in it
	EXPECT_FALSE(PathFinder::IsLineWalkable(PathMapFlags::SIDEWALL | PathMapFlags::ACTOR, ActorsAreNOTBlocking));
	EXPECT_FALSE(PathFinder::IsLineWalkable(PathMapFlags::DOOR_IMPASSABLE | PathMapFlags::ACTOR, ActorsAreNOTBlocking));
	EXPECT_FALSE(PathFinder::IsLineWalkable(PathMapFlags::SIDEWALL | PathMapFlags::PASSABLE, ActorsAreNOTBlocking));
}

// PaintSearchMap() paints a circle of radius circleSize-1.
// Only walkable terrain may take the mark.
TEST(TilePropsTest, PaintSearchMapKeepsActorsOffWalls)
{
	const TestSearchMap map {
		"#######",
		"#..X..#",
		"#..3..#",
		"#..T..#",
		"#######"
	};

	// the wall above, the obstacle at (3,1) and the walls below are all inside the radius 2
	// footprint and all have to come out of it unchanged; the travel tile takes the mark
	const test::MapRows expected {
		"#######",
		"#PPXPP#",
		"#PPPPP#",
		"#PPpPP#",
		"#######"
	};
	EXPECT_TRUE(map.Matches(expected));
}

TEST(TilePropsTest, PaintSearchMapClearsPreviousActorMark)
{
	TestSearchMap map {
		"#####",
		"#...#",
		"#...#",
		"#####"
	};

	const test::MapRows expectedNpc {
		"#####",
		"#NNN#",
		"#NNN#",
		"#####"
	};
	map.Props().PaintSearchMap(SearchmapPoint(2, 1), 2, PathMapFlags::NPC);
	EXPECT_TRUE(map.Matches(expectedNpc));

	// repainting replaces the actor bits rather than accumulating them
	map.Props().PaintSearchMap(SearchmapPoint(2, 1), 2, PathMapFlags::PC);
	const test::MapRows expectedPc {
		"#####",
		"#PPP#",
		"#PPP#",
		"#####"
	};
	EXPECT_TRUE(map.Matches(expectedPc));

	// test clearing also work as expected
	map.Props().PaintSearchMap(SearchmapPoint(2, 1), 2, PathMapFlags::UNMARKED);
	const test::MapRows expectedCleared {
		"#####",
		"#...#",
		"#...#",
		"#####"
	};
	EXPECT_TRUE(map.Matches(expectedCleared));
}

// Test for specific regression: actors hugging a wall used to make the wall
// disappear for any query that ignored actors, which is how a pathfinder routed actors
// straight through it.
TEST(PathFinderTest, WallStaysSolidWithActorsAlongIt)
{
	const TestSearchMap map {
		"##########",
		"#S..##...#",
		"#..2##2..#",
		"#E..##...#",
		"##########"
	};

	// both actors press right up against the middle wall, and it stays a wall
	const test::MapRows expected {
		"##########",
		"#SPP##PP.#",
		"#.PP##PP.#",
		"#EPP##PP.#",
		"##########"
	};
	ASSERT_TRUE(map.Matches(expected));

	// straight from one actor to the other, which is right across the wall
	const Point west = map.ActorPosOf(0);
	const Point east = map.ActorPosOf(1);

	EXPECT_FALSE(PathFinder::IsWalkableTo(map.Props(), west, east, true, noSpeed, noCircle));
	EXPECT_FALSE(PathFinder::IsWalkableTo(map.Props(), west, east, false, noSpeed, noCircle))
		<< "ignoring actors must not ignore the wall between them";

	// the wall also still blocks sight
	EXPECT_FALSE(PathFinder::IsVisibleLOS(map.Props(), SearchmapPoint(west), SearchmapPoint(east), noSpeed, noCircle));

	// ... while the untouched column on the far side of the room, S to E, is walkable
	EXPECT_TRUE(PathFinder::IsWalkableTo(map.Props(), map.Start(), map.End(), true, noSpeed, noCircle));
}

// LineStepper tests
TEST(PathFinderTest, LineStepperArrivesExactlyAndAlwaysMoves)
{
	const NavmapPoint from(56, 42);
	const NavmapPoint to(248, 66);

	PathFinder::LineStepper<NavmapPoint> walk { from, to };
	Point previous = from;
	size_t steps = 0;
	while (walk.Step()) {
		EXPECT_NE(walk.Current(), previous) << "a step must always move";
		previous = walk.Current();
		ASSERT_LT(++steps, 1000u) << "the walk has to terminate";
	}
	EXPECT_EQ(walk.Current(), to) << "it has to land exactly on the target";
	EXPECT_FALSE(walk.Step()) << "and stay there once it has";

	// the same in tile space, which is what GetBlockedInLineTile() walks
	PathFinder::LineStepper<SearchmapPoint> tiles { SearchmapPoint(0, 0), SearchmapPoint(6, 0) };
	while (tiles.Step()) {}
	EXPECT_EQ(tiles.Current(), SearchmapPoint(6, 0));
}

TEST(PathFinderTest, LineStepperFactorScalesTheStep)
{
	auto stepsWith = [](float_t factor) {
		PathFinder::LineStepper<NavmapPoint> walk { NavmapPoint(0, 0), NavmapPoint(100, 0), factor };
		size_t n = 0;
		while (walk.Step()) ++n;
		return n;
	};

	// factor 1 is the shortest step: 2 pixels along the dominant axis, so 50 of them
	EXPECT_EQ(stepsWith(1), 50u);
	// a faster actor covers the same line in fewer, longer steps
	EXPECT_EQ(stepsWith(4), 13u);
	EXPECT_LT(stepsWith(8), stepsWith(4));
}

// An actor in the way is only an obstacle while actors are blocking; open floor beneath it
// must stay walkable for the ignore-actors queries the pathfinder makes.
TEST(PathFinderTest, ActorBlocksOnlyWhenActorsAreBlocking)
{
	const TestSearchMap map {
		"########",
		"#S.a..E#",
		"########"
	};

	const test::MapRows expected {
		"########",
		"#S.N..E#",
		"########"
	};
	ASSERT_TRUE(map.Matches(expected));

	const Point from = map.Start();
	const Point to = map.End();

	EXPECT_FALSE(PathFinder::IsWalkableTo(map.Props(), from, to, true, noSpeed, noCircle));
	EXPECT_TRUE(PathFinder::IsWalkableTo(map.Props(), from, to, false, noSpeed, noCircle));
}

// A bumpable actor is transparent to a route that means to shove it aside, and solid once the
// caller asks for actors to be treated as blocking. One blocker or several makes no difference:
// each is a separate mark in the cache, and the route either ignores all of them or none.
TEST(TraversabilityTest, BumpableActorsBlockOnlyWhenActorsAreBlocking)
{
	// one tile corridors, so a blocker cannot be walked around
	const std::array<const TestSearchMap, 2> corridors { {
		{ "#########",
		  "#S..a..E#",
		  "#########" },
		{ "#########",
		  "#S.a1a.E#",
		  "#########" },
	} };

	constexpr char selfTag = 0;
	const test::ActorIdentity self = test::MakeActorIdentity(selfTag);

	for (const TestSearchMap& map : corridors) {
		const test::TestTraversability traversability { map, true };
		ASSERT_EQ(traversability.StateAt(map.ActorPosOf(0)), TraversabilityCache::TraversabilityCellValueActor)
			<< "the glyph has to reach the cache FindPathOn() will build";

		const Point from = map.Start();
		const Point to = map.End();

		// intending to bump: the blockers are transparent, so the route runs straight through
		EXPECT_FALSE(test::CallFindPath(map, traversability, from, to, self, 1, PF_SIGHT).Empty())
			<< "a bumpable actor must not stop a route that would bump it";

		// treating actors as solid: there is no way past in a one tile corridor
		EXPECT_TRUE(test::CallFindPath(map, traversability, from, to, self, 1,
					       PF_SIGHT | PF_ACTORS_ARE_BLOCKING)
				    .Empty())
			<< "a blocking actor plugs the only corridor, so there is no route";
	}
}

TEST(TraversabilityTest, UnbumpableActorAlwaysBlocks)
{
	const TestSearchMap map {
		"#########",
		"#S..a..E#",
		"#########"
	};

	const char selfTag = 0;
	const test::ActorIdentity self = test::MakeActorIdentity(selfTag);

	// same drawing, but nothing here can be shoved aside
	const test::TestTraversability traversability { map, false };

	const Point from = map.Start();
	const Point to = map.End();

	EXPECT_TRUE(test::CallFindPath(map, traversability, from, to, self, 1, PF_SIGHT).Empty());
	EXPECT_TRUE(test::CallFindPath(map, traversability, from, to, self, 1,
				       PF_SIGHT | PF_ACTORS_ARE_BLOCKING)
			    .Empty());
}

// An actor must not be stopped by the record of itself standing where it starts.
TEST(TraversabilityTest, ActorIgnoresItsOwnCell)
{
	const TestSearchMap map {
		"#########",
		"#S..a..E#",
		"#########"
	};

	// the drawn actor is the one doing the walking, so it starts inside its own footprint
	const test::TestTraversability traversability { map, false };
	const Point from = map.ActorPosOf(0);
	const Point to = map.End();
	const Path path = test::PathDrawnActor(map, traversability, 0, to,
					       PF_SIGHT | PF_ACTORS_ARE_BLOCKING);
	EXPECT_FALSE(path.Empty()) << "an actor must be able to walk out of its own footprint";
	EXPECT_TRUE(test::PathAvoidsWalls(map, from, path));
	EXPECT_TRUE(test::PathIsSane(map, from, path));

	// the counterpart, which is what proves the cell really is blocking and that the identity
	// is what let the first route through: anyone else is stopped by the very same footprint
	const char strangerTag = 0;
	const Path stranger = test::CallFindPath(map, traversability, map.Start(), to,
						 test::MakeActorIdentity(strangerTag), 1,
						 PF_SIGHT | PF_ACTORS_ARE_BLOCKING);
	EXPECT_TRUE(stranger.Empty()) << "another actor must still be blocked by that footprint";
}


// FindPath tests
TEST(FindPathTest, WalksAnOpenCorridor)
{
	const TestSearchMap map {
		"##########",
		"#S......E#",
		"##########"
	};

	const test::TestTraversability traversability { map };
	const Path path = test::CallFindPath(map, traversability, map.Start(), map.End());

	ASSERT_FALSE(path.Empty());
	EXPECT_TRUE(test::PathAvoidsWalls(map, map.Start(), path));
	EXPECT_TRUE(test::PathIsSane(map, map.Start(), path));

	// nothing to route around, so the pathfinder returns a single waypoint on E and the whole
	// corridor between is the line walk getting there
	const test::MapRows expected {
		"##########",
		"#*******@#",
		"##########"
	};
	EXPECT_TRUE(map.MatchesWithPath(map.Start(), path, expected));
}

// a barrier between source and destination has to be walked around, never through
TEST(FindPathTest, RoutesAroundABarrier)
{
	const TestSearchMap map {
		"###########",
		"#...2#...2#",
		"#....#....#",
		"#....#....#",
		"#.........#",
		"###########"
	};

	const Point start = map.ActorPosOf(0);
	const Point dest = map.ActorPosOf(1);
	// the walker is the drawn actor, so it is routed at the size its glyph gave it
	const test::TestTraversability traversability { map };
	const Path path = test::CallFindPath(map, traversability, start, dest, nullptr, map.ActorCircleSizeOf(0));

	ASSERT_FALSE(path.Empty());
	EXPECT_TRUE(test::PathAvoidsWalls(map, start, path));
	EXPECT_TRUE(test::PathIsSane(map, start, path));

	// it has to come round the open bottom row, so it has to get more >1 waypoint
	EXPECT_GT(path.Size(), 1u);
}

// A destination that cannot be reached at all must not produce a route into it.
TEST(FindPathTest, WillNotEnterASealedRoom)
{
	const TestSearchMap map {
		"###########",
		"#....#....#",
		"#.S..#..E.#",
		"#....#....#",
		"#....#....#",
		"###########"
	};

	const Point from = map.Start();
	const test::TestTraversability traversability { map };
	const Path path = test::CallFindPath(map, traversability, from, map.End());

	ASSERT_TRUE(path.Empty());
}

// A bigger actor cannot use a gap a small one fits through. Both walkers are drawn, so their
// sizes are read off the map rather than passed in beside it.
TEST(FindPathTest, RespectsActorSize)
{
	// two halls joined by a single one tile slit at (10,5), which is the only way across
	const TestSearchMap map {
		"#####################",
		"#.........#.........#",
		"#.........#.........#",
		"#.........#.........#",
		"#.........#.........#",
		"#1...3.........E....#",
		"#.........#.........#",
		"#.........#.........#",
		"#.........#.........#",
		"#.........#.........#",
		"#####################"
	};

	constexpr size_t smallActor = 0;
	constexpr size_t bigActor = 1;

	// bumpable, so neither walker is stopped by the other's mark; only the terrain is in play
	const test::TestTraversability traversability { map, true };
	const Point to = map.End();

	const Path small = test::PathDrawnActor(map, traversability, smallActor, to);
	ASSERT_FALSE(small.Empty()) << "a one tile wide actor fits through a one tile wide slit";
	EXPECT_TRUE(test::PathAvoidsWalls(map, map.ActorPosOf(smallActor), small));
	EXPECT_TRUE(test::PathIsSane(map, map.ActorPosOf(smallActor), small, map.ActorCircleSizeOf(smallActor)));

	// and it really did go through the slit, since there is nowhere else to cross
	const std::vector<SearchmapPoint> tiles = test::PathTiles(map.ActorPosOf(smallActor), small);
	EXPECT_NE(std::find(tiles.begin(), tiles.end(), SearchmapPoint(10, 5)), tiles.end())
		<< "the route must pass through the slit";

	// the big one needs two clear tiles to either side of itself, which the slit cannot give
	const Path large = test::PathDrawnActor(map, traversability, bigActor, to);
	EXPECT_TRUE(large.Empty()) << "a wide actor must not squeeze through a one tile slit";

	// ... and it is the slit that stopped it, not its own size: the same actor walks fine as
	// long as it stays in the open hall it started in
	const Path inHall = test::PathDrawnActor(map, traversability, bigActor, map.ActorPosOf(smallActor));
	EXPECT_FALSE(inHall.Empty()) << "the wide actor must still be able to move within its hall";
}

// Test diagonal line, expect only one waypoint
TEST(FindPathTest, PathLongDiagonal)
{
	// S to E is a very shallow diagonal: 12 tiles across for 2 tiles down, so the straight
	// line between them clips the divider well above the slit
	const TestSearchMap map {
		"#####################",
		"#.........#.........#",
		"#.........#.........#",
		"#..S......#.........#",
		"#.........#.........#",
		"#..............E....#",
		"#.........#.........#",
		"#.........#.........#",
		"#.........#.........#",
		"#.........#.........#",
		"#####################"
	};
	const test::MapRows expected {
		"#####################",
		"#.........#.........#",
		"#.........#.........#",
		"#..**.....#.........#",
		"#...**....#.........#",
		"#....**********@....#",
		"#.........#.........#",
		"#.........#.........#",
		"#.........#.........#",
		"#.........#.........#",
		"#####################"
	};

	const Point from = map.Start();
	const test::TestTraversability traversability { map };
	const Path path = test::CallFindPath(map, traversability, from, map.End());
	EXPECT_EQ(path.Size(), 1) << "the leg has to stay long enough for the rounding to show";
	EXPECT_TRUE(test::PathAvoidsWalls(map, from, path));
	EXPECT_TRUE(test::PathIsSane(map, from, path));
	EXPECT_TRUE(map.MatchesWithPath(map.Start(), path, expected));
}


TEST(FindPathTest, PathUTurn)
{
	const TestSearchMap map {
		"#####################",
		"#.........#....E....#",
		"#.........#.........#",
		"#..S......#.........#",
		"#.........#.........#",
		"#...................#",
		"#.........#.........#",
		"#.........#.........#",
		"#.........#.........#",
		"#.........#.........#",
		"#####################"
	};
	const test::MapRows expected {
		"#####################",
		"#.........#....@....#",
		"#.........#...**....#",
		"#..**.....#..**.....#",
		"#...**....#.**......#",
		"#....******@*.......#",
		"#.........#.........#",
		"#.........#.........#",
		"#.........#.........#",
		"#.........#.........#",
		"#####################"
	};

	const Point from = map.Start();
	const test::TestTraversability traversability { map };
	const Path path = test::CallFindPath(map, traversability, from, map.End());
	EXPECT_EQ(path.Size(), 2) << "expected 2 waypoints";
	EXPECT_TRUE(test::PathAvoidsWalls(map, from, path));
	EXPECT_TRUE(test::PathIsSane(map, from, path));
	EXPECT_TRUE(map.MatchesWithPath(map.Start(), path, expected));
}

// === minDistance ===
// the walker is meant to close in only until it is near enough, not to step onto the target. That is the branch
// in FindPath() which ends the search early, and it is the only one which consults PF_SIGHT.
// minDistance is in navmap pixels, and the engine's own callers count a tile as 20 of them
// (see MAX_OPERATING_DISTANCE)

// The plain case: given a distance to keep, the route ends short of the goal rather than on it.
TEST(FindPathTest, MinDistanceEndsTheRouteShortOfTheGoal)
{
	const TestSearchMap map {
		"#####################",
		"#S.................E#",
		"#####################"
	};

	const Point from = map.Start();
	const Point to = map.End();
	const test::TestTraversability traversability { map };

	// with no distance to keep, the walk ends on the goal itself
	const Path onto = test::CallFindPath(map, traversability, from, to);
	ASSERT_FALSE(onto.Empty());
	EXPECT_EQ(SearchmapPoint { onto.GetLastStep().point }, SearchmapPoint { to });

	// asked to stop five tiles out, it ends within that of the goal but not on it
	constexpr unsigned int keepAway = test::Tiles(5);
	const Path shortOf = test::CallFindPath(map, traversability, from, to, nullptr, 1, PF_SIGHT, keepAway);
	ASSERT_FALSE(shortOf.Empty());
	EXPECT_TRUE(test::PathIsSane(map, from, shortOf));

	const Point last = shortOf.GetLastStep().point;
	EXPECT_NE(SearchmapPoint(last), SearchmapPoint { to }) << "it was told to keep its distance";
	EXPECT_LT(Distance(last, to), keepAway) << "it must stop closer than ordered `minDistance` (" << keepAway << " tiles)";

	// and it didn't go to the target and back up, but rather stop at the nearest occasion
	EXPECT_LT(test::PathLength(from, shortOf), test::PathLength(from, onto));
}

// PF_SIGHT is what makes the early stop conditional on seeing the goal, so a walker with a wall
// between it and the target has to keep going until it has both range and line of sight.
TEST(FindPathTest, MinDistanceWaitsForSightOfTheGoalWithPFSight)
{
	// The divider is a sight blocker, open along the top and bottom row. One tile is enough at
	// this geometry
	const TestSearchMap map {
		"###########",
		"#.........#",
		"#....#....#",
		"#.S..#..E.#",
		"#....#....#",
		"#.........#",
		"###########"
	};

	const Point from = map.Start();
	const Point to = map.End();
	const test::TestTraversability traversability { map };

	// five tiles of range: the tiles just short of the divider are inside it, but blind
	constexpr unsigned int keepAway = test::Tiles(5);

	const Path blind = test::CallFindPath(map, traversability, from, to, nullptr, 1, 0, keepAway);
	ASSERT_FALSE(blind.Empty());
	EXPECT_TRUE(test::PathIsSane(map, from, blind));
	const Point blindEnd = blind.GetLastStep().point;
	EXPECT_FALSE(PathFinder::IsVisibleLOS(map.Props(), SearchmapPoint(blindEnd), SearchmapPoint { to },
					      noSpeed, noCircle))
		<< "without PF_SIGHT range alone is enough, so it stops on the near side of the wall";

	const Path seeing = test::CallFindPath(map, traversability, from, to, nullptr, 1, PF_SIGHT, keepAway);
	ASSERT_FALSE(seeing.Empty());
	EXPECT_TRUE(test::PathIsSane(map, from, seeing));
	EXPECT_TRUE(test::PathAvoidsWalls(map, from, seeing));
	const Point seeingEnd = seeing.GetLastStep().point;
	EXPECT_TRUE(PathFinder::IsVisibleLOS(map.Props(), SearchmapPoint(seeingEnd), SearchmapPoint { to },
					     noSpeed, noCircle))
		<< "with PF_SIGHT it may only stop where it can see the goal";

	// which costs it the walk around the divider
	EXPECT_GT(test::PathLength(from, seeing), test::PathLength(from, blind));
}

// Telling a walker to keep its distance has to shorten the walk, wherever the click landed.
//
// DISABLED: the early stop checks its line of sight against the click point rather than against
// the goal the click was adjusted to, so a click into rock kills the early stop outright and the
// walker closes all the way in.
TEST(FindPathTest, DISABLED_MinDistanceStillStopsShortWhenTheClickWasIntoRock)
{
	const TestSearchMap map {
		"############",
		"#..........#",
		"#...####...#",
		"#S..####...#",
		"#...####...#",
		"#..........#",
		"############"
	};

	const Point from = map.Start();
	const Point intoTheRock = TestSearchMap::Nav(5, 3);
	const test::TestTraversability traversability { map };
	constexpr unsigned int keepAway = test::Tiles(5);

	const Path closingIn = test::CallFindPath(map, traversability, from, intoTheRock, nullptr, 1, PF_SIGHT);
	const Path keepingBack = test::CallFindPath(map, traversability, from, intoTheRock, nullptr, 1,
						    PF_SIGHT, keepAway);

	ASSERT_FALSE(closingIn.Empty());
	ASSERT_FALSE(keepingBack.Empty());
	EXPECT_LT(test::PathLength(from, keepingBack), test::PathLength(from, closingIn))
		<< "being told to keep a distance has to shorten the walk, wherever the click landed";
}

// === the line walk ===

namespace {

	// A corridor from S to E with one wall run across it, in every shape a line can meet: one to three
	// tiles thick, at every position that leaves both ends clear.
	struct WalledCorridor {
		TestSearchMap map;
		int thickness;
		int firstWallTile;

		std::string Describe() const
		{
			return "a wall " + std::to_string(thickness) + " tile(s) thick starting on tile " +
				std::to_string(firstWallTile);
		}
	};

	std::vector<WalledCorridor> WalledCorridors()
	{
		constexpr int MaxThickness = 3;
		const TestSearchMap clear {
			"###########",
			"#S.......E#",
			"###########"
		};
		const SearchmapPoint eye { clear.Start() };
		const SearchmapPoint target { clear.End() };

		std::vector<WalledCorridor> corridors;
		for (int thickness = 1; thickness <= MaxThickness; ++thickness) {
			// the walk skips the tile it starts on and the wall may not bury the target, so the run
			// has to fit strictly between the two
			for (int first = eye.x + 1; first + thickness - 1 < target.x; ++first) {
				TestSearchMap walled { clear };
				for (int x = first; x < first + thickness; ++x) {
					walled.SetTile(x, eye.y, Glyph::Wall);
				}
				corridors.push_back({ walled, thickness, first });
			}
		}
		return corridors;
	}

	// actorSpeed is Actor::walkScale, bigger number is a slower creature. Haste halves the walkScale,
	// encumbrance at half speed doubles it, so the usable range across the supported games is roughly
	// 0-1500. The sweeps below run past the games' shipped values.
	constexpr int slowestSpeed = 2000;

}

// How fast the observer walks has nothing to do with whether a wall is in the way, so every wall
// on the line has to block sight at every speed, wherever it stands and however thick it is.
//
// DISABLED: The tile space line walk borrows NormalizeDeltas(), a navmap function whose
// STEP_RADIUS of 2 means two navmap pixels - an eighth of a tile. GetBlockedInLineTile() converts
// that to tile units by dividing its factor by 16, but only on the branch where there is an actor
// speed to divide by; with no speed the factor is a bare 1 and the stride becomes two whole tiles.
// The conversion is a rounding effect rather than an exact scaling, so it also stops working once
// the factor climbs back over half a tile, which it does for fast actors.
TEST(PathFinderTest, DISABLED_AWallBlocksSightAtEverySpeed)
{
	// the line walk asks gamedata for the step time as soon as the speed is non-zero
	const test::ScopedStepTime stepTime;

	for (const WalledCorridor& corridor : WalledCorridors()) {
		const SearchmapPoint eye { corridor.map.Start() };
		const SearchmapPoint target { corridor.map.End() };

		std::vector<int> sawThroughTheWall;
		for (int speed = 0; speed <= slowestSpeed; ++speed) {
			if (PathFinder::IsVisibleLOS(corridor.map.Props(), eye, target, speed, noCircle)) {
				sawThroughTheWall.push_back(speed);
			}
		}

		if (!sawThroughTheWall.empty()) {
			ADD_FAILURE() << "a wall is a wall whatever the observer's walk speed, but sight passed "
				      << "through " << corridor.Describe() << " at the following actor's speeds: "
				      << AsRanges(sawThroughTheWall);
		}
	}
}

// The same of the navmap sibling, which backs IsWalkableTo() and so Theta*'s line check.
//
// DISABLED: GetBlockedInLine() samples the line with the movement stride NormalizeDeltas() hands
// Movable::DoStep() - two navmap pixels scaled by stepTime / walkScale. Scaling by speed is right
// for moving an actor and wrong for sampling geometry, where the resolution has to come from the
// tile being looked for. Fast actors stride clean over the wall; only speed 0, whose factor is a
// bare 1 and so one navmap pixel per step, samples finely enough.
TEST(PathFinderTest, DISABLED_AWallIsSeenOnTheNavmapLineAtEverySpeed)
{
	// the line walk asks gamedata for the step time as soon as the speed is non-zero
	const test::ScopedStepTime stepTime;

	// both consumers of GetBlockedInLine(): IsVisibleLOS() asks without stopping on impassable,
	// IsWalkableTo() asks with. The flag also picks which blocked status function runs, so a wall
	// has to be reported on either branch.
	for (const bool stopOnImpassable : { false, true }) {
		for (const WalledCorridor& corridor : WalledCorridors()) {
			std::vector<int> missedTheWall;
			for (int speed = 0; speed <= slowestSpeed; ++speed) {
				const PathMapFlags blocked = PathFinder::GetBlockedInLine(
					corridor.map.Props(), corridor.map.Start(), corridor.map.End(),
					stopOnImpassable, speed, noCircle);

				if (!bool(blocked & PathMapFlags::SIDEWALL)) {
					missedTheWall.push_back(speed);
				}
			}

			if (!missedTheWall.empty()) {
				ADD_FAILURE() << "a wall is a wall whatever the observer's walk speed, but the navmap "
					      << "line walk (stopOnImpassable " << std::boolalpha << stopOnImpassable
					      << ") stepped over " << corridor.Describe()
					      << " at the following actor's speeds: " << AsRanges(missedTheWall);
			}
		}
	}
}

// === a destination the actor cannot stand on ===

// Every click on a wall or on another actor arrives here: FindPath() runs the goal through
// AdjustPositionDirected() and builds the route to whatever that returns. The direction it
// searches in points from the goal back at the caller, so the relocated goal should end up on the
// caller's side of whatever is in the way.

// The thin case, which works: the cone finds ground on the near side whichever side that is.
TEST(FindPathTest, ClickIntoAThinWallLandsOnTheCallersSide)
{
	const TestSearchMap map {
		"###########",
		"#....#....#",
		"#S...#...E#",
		"#....#....#",
		"###########"
	};

	const Point rock = TestSearchMap::Nav(5, 2);
	const Point west = map.Start();
	const Point east = map.End();
	const test::TestTraversability traversability { map };

	const Path fromWest = test::CallFindPath(map, traversability, west, rock);
	ASSERT_FALSE(fromWest.Empty());
	EXPECT_TRUE(test::PathIsSane(map, west, fromWest));
	EXPECT_TRUE(test::PathAvoidsWalls(map, west, fromWest));
	EXPECT_EQ(SearchmapPoint(fromWest.GetLastStep().point), SearchmapPoint(4, 2))
		<< "the west caller should stop against the west face";

	const Path fromEast = test::CallFindPath(map, traversability, east, rock);
	ASSERT_FALSE(fromEast.Empty());
	EXPECT_TRUE(test::PathIsSane(map, east, fromEast));
	EXPECT_TRUE(test::PathAvoidsWalls(map, east, fromEast));
	EXPECT_EQ(SearchmapPoint(fromEast.GetLastStep().point), SearchmapPoint(6, 2))
		<< "and the east caller against the east face";
}

// A click into rock walks up to the face of the wall the caller is standing on.
//
// DISABLED: the directed adjustment only reaches one tile out, so against a wall two tiles thick
// it finds nothing and hands over to the undirected scan, which can settle on the far side. The
// east caller is then given a goal it cannot reach, and gets no path at all.
// The lookback distance could always be increased, but it's also true that in bg1 one can get arbitrarily big parties,
// so formation rotation can result in goals very deep in walls.
// Doing our best (like the test expects) even for inaccessible areas seems like the most obvious thing to do.
TEST(FindPathTest, DISABLED_ClickIntoAThickWallStopsAgainstTheNearFace)
{
	const TestSearchMap map {
		"###########",
		"#....##...#",
		"#S...##..E#",
		"#....##...#",
		"###########"
	};

	const Point rock = TestSearchMap::Nav(5, 2);
	const test::TestTraversability traversability { map };

	const Path fromWest = test::CallFindPath(map, traversability, map.Start(), rock);
	ASSERT_FALSE(fromWest.Empty());
	EXPECT_EQ(SearchmapPoint(fromWest.GetLastStep().point), SearchmapPoint(4, 2));

	const Path fromEast = test::CallFindPath(map, traversability, map.End(), rock);
	ASSERT_FALSE(fromEast.Empty()) << "the east caller has a face of its own to walk up to";
	EXPECT_EQ(SearchmapPoint(fromEast.GetLastStep().point), SearchmapPoint(6, 2))
		<< "and it is the east one, not the west";
}

// When the relocated goal is the tile, the caller is already standing on, there is nothing to walk.
TEST(FindPathTest, ClickWhoseAdjustmentLandsOnTheCallerGivesNoPath)
{
	// a 2 tiles closet, so the only passable ground near the clicked wall is the caller itself
	const TestSearchMap map {
		"######",
		"###S.#",
		"######"
	};

	const Point inTheCloset = map.Start();
	const test::TestTraversability traversability { map };

	const Path path = test::CallFindPath(map, traversability, inTheCloset, TestSearchMap::Nav(2, 1));
	EXPECT_TRUE(path.Empty());
}

// Clicking on somebody ends the route beside them, never on them. What stops it is the actor's
// searchmap mark, which clears PASSABLE for the tile
TEST(FindPathTest, ClickOnAnActorEndsBesideIt)
{
	const TestSearchMap map {
		"#########",
		"#.......#",
		"#.S...a.#",
		"#.......#",
		"#########"
	};

	const Point from = map.Start();
	const Point onto = map.ActorPosOf(0);
	const SearchmapPoint targetTile { onto };

	for (const bool bumpable : { true, false }) {
		const test::TestTraversability traversability { map, bumpable };
		const Path path = test::CallFindPath(map, traversability, from, onto, nullptr, 1,
						     PF_SIGHT | PF_ACTORS_ARE_BLOCKING);
		ASSERT_FALSE(path.Empty()) << "bumpable = " << bumpable;
		EXPECT_TRUE(test::PathIsSane(map, from, path)) << "bumpable = " << bumpable;
		const SearchmapPoint last { path.GetLastStep().point };
		EXPECT_NE(last, targetTile) << "bumpable = " << bumpable << ": the route ended on the target";
		EXPECT_LE(std::abs(last.x - targetTile.x), 1) << "bumpable = " << bumpable;
	}
}

// A big actor sent somewhere only a small one fits gets the nearest spot it does fit, in the open,
// rather than nothing at all - the directed adjustment relocates the goal before the search starts.
// That is also why the `minDistance < actorCircleSize` guard further down FindPath() almost never
// fires: by the time it is reached the goal has usually been moved somewhere legal. Worth noting
// that the guard compares a navmap pixel distance against a circle size, which are not the same
// unit, so any minDistance of 8 or more skips it regardless of the actor.
TEST(FindPathTest, BigActorClickingIntoANookStopsInTheOpen)
{
	// the corridor along row 7, right of x == 14, is one tile tall
	const TestSearchMap map {
		"#####################",
		"#..............######",
		"#..............######",
		"#..............######",
		"#..............######",
		"#..............######",
		"#..............######",
		"#....S.............E#",
		"#..............######",
		"#..............######",
		"#..............######",
		"#####################"
	};

	const Point from = map.Start();
	const Point to = map.End();
	const test::TestTraversability traversability { map };
	constexpr int hallEdge = 14;

	// the small one walks in and stops on the goal
	const Path small = test::CallFindPath(map, traversability, from, to, nullptr, 1);
	ASSERT_FALSE(small.Empty());
	EXPECT_TRUE(test::PathIsSane(map, from, small));
	EXPECT_EQ(SearchmapPoint(small.GetLastStep().point), SearchmapPoint { to });

	// the big one cannot enter the corridor at all, so it is given the nearest ground in the hall
	const Path big = test::CallFindPath(map, traversability, from, to, nullptr, 4);
	ASSERT_FALSE(big.Empty()) << "it should still be told to walk as close as it can get";
	EXPECT_TRUE(test::PathIsSane(map, from, big, 4));
	EXPECT_TRUE(test::PathAvoidsWalls(map, from, big));
	for (const SearchmapPoint& tile : test::PathTiles(from, big)) {
		EXPECT_LE(tile.x, hallEdge) << "the big actor must not be routed into the nook";
	}
}

// === doors ===

namespace {

	// A corridor from S to E with one door across it, on every tile in turn.
	struct CorridorWithADoor {
		TestSearchMap map;
		int doorTile;
	};

	std::vector<CorridorWithADoor> CorridorsWithADoorAt(const char doorGlyph)
	{
		const TestSearchMap clear {
			"########",
			"#S....E#",
			"########"
		};
		const SearchmapPoint eye { clear.Start() };
		const SearchmapPoint target { clear.End() };

		std::vector<CorridorWithADoor> corridors;
		for (int doorX = eye.x + 1; doorX < target.x; ++doorX) {
			TestSearchMap withDoor { clear };
			withDoor.SetTile(doorX, eye.y, doorGlyph);
			corridors.push_back({ withDoor, doorX });
		}
		return corridors;
	}

}

// A shut door is impassable whichever kind it is, and opening it is a searchmap write, so the same
// drawing answers both ways round.
TEST(FindPathTest, ShutDoorBlocksTheOnlyCorridorUntilItOpens)
{
	for (const char doorGlyph : { Glyph::Door, Glyph::OpaqueDoor }) {
		TestSearchMap map {
			"##########",
			"#S..#...E#",
			"##########"
		};
		map.SetTile(4, 1, doorGlyph);

		const test::TestTraversability traversability { map };
		const Path closed = test::CallFindPath(map, traversability, map.Start(), map.End());
		EXPECT_TRUE(closed.Empty())
			<< "there is no way round a door filling a one tile corridor";

		map.SetTile(4, 1, Glyph::Floor);

		const Path opened = test::CallFindPath(map, traversability, map.Start(), map.End());
		ASSERT_FALSE(opened.Empty()) << "with the door open the corridor is just floor";
		EXPECT_TRUE(test::PathIsSane(map, map.Start(), opened));
		EXPECT_TRUE(test::PathAvoidsWalls(map, map.Start(), opened));
	}
}

// A non-opaque door can be seen through.
TEST(PathFinderTest, APlainDoorIsSeenThroughWhereverItStands)
{
	for (const CorridorWithADoor& corridor : CorridorsWithADoorAt(Glyph::Door)) {
		EXPECT_TRUE(PathFinder::IsVisibleLOS(corridor.map.Props(), SearchmapPoint { corridor.map.Start() },
						     SearchmapPoint { corridor.map.End() }, noSpeed, noCircle))
			<< "a shut wooden door can be seen through, but one on tile " << corridor.doorTile
			<< " stopped sight";
	}
}

// The other half of the same property - opaque doors cannot be seen through.
//
// DISABLED: with no actor speed the tile space line walk strides two whole tiles, so an opaque
// door blocks or not according to whether the stride happens to land on it - today it blocks on
// the odd columns and is invisible on the even ones. Same defect as the === the line walk ===
// tests state in general.
TEST(PathFinderTest, DISABLED_AnOpaqueDoorBlocksSightWhereverItStands)
{
	for (const CorridorWithADoor& corridor : CorridorsWithADoorAt(Glyph::OpaqueDoor)) {
		EXPECT_FALSE(PathFinder::IsVisibleLOS(corridor.map.Props(), SearchmapPoint { corridor.map.Start() },
						      SearchmapPoint { corridor.map.End() }, noSpeed, noCircle))
			<< "an opaque door cannot be seen through, but sight passed through one on tile "
			<< corridor.doorTile;
	}
}

// === travel tiles ===

// An area transition strip is walkable ground: GetBlockedTile() adds PASSABLE wherever it sees
// TRAVEL, so a route may cross one instead of treating it as an obstacle.
TEST(FindPathTest, RoutesAcrossATravelTile)
{
	const TestSearchMap map {
		"##########",
		"#S..T...E#",
		"##########"
	};

	const Point from = map.Start();
	const test::TestTraversability traversability { map };
	const Path path = test::CallFindPath(map, traversability, from, map.End());

	ASSERT_FALSE(path.Empty());
	EXPECT_TRUE(test::PathIsSane(map, from, path));
	EXPECT_TRUE(test::PathAvoidsWalls(map, from, path));

	const std::vector<SearchmapPoint> tiles = test::PathTiles(from, path);
	EXPECT_NE(std::find(tiles.begin(), tiles.end(), SearchmapPoint(4, 1)), tiles.end())
		<< "the corridor is one tile wide, so the travel tile is the only way past";
}

// An actor standing on a travel tile keeps both marks, the tile is still a transition, it just has somebody on it
TEST(TilePropsTest, ActorMarkOnTravelRendersApartFromOneOnPlainFloor)
{
	// circle size 2 is a 3x3 footprint, so this NPC covers the travel tiles either side of it
	const TestSearchMap npc {
		"#####",
		"#TbT#",
		"#####"
	};
	const test::MapRows expectedNPC {
		"#####",
		"#nNn#",
		"#####"
	};
	EXPECT_TRUE(npc.Matches(expectedNPC));

	// and the party member's own pair of glyphs, so the two alphabets stay apart
	const TestSearchMap pc {
		"#####",
		"#T2T#",
		"#####"
	};
	const test::MapRows expectedPC {
		"#####",
		"#pPp#",
		"#####"
	};
	EXPECT_TRUE(pc.Matches(expectedPC));
}

// === waypoint orientations ===

// Every waypoint faces the way the leg arriving at it was going. The source is not a waypoint, so
// the first leg starts from it.
TEST(FindPathTest, WaypointsFaceAlongTheirLegs)
{
	const TestSearchMap map {
		"#####################",
		"#.........#....E....#",
		"#.........#.........#",
		"#..S......#.........#",
		"#.........#.........#",
		"#...................#",
		"#.........#.........#",
		"#####################"
	};

	const Point from = map.Start();
	const test::TestTraversability traversability { map };
	const Path path = test::CallFindPath(map, traversability, from, map.End());

	ASSERT_FALSE(path.Empty());
	EXPECT_EQ(test::PathOrients(path), test::ForwardOrients(from, path));
}

// PF_BACKAWAY lets an actor retreat without turning round: a waypoint nearly collinear with its
// neighbours faces back the way it came instead.
TEST(FindPathTest, BackAwayTurnsNearlyCollinearWaypointsRound)
{
	const TestSearchMap map {
		"########",
		"#S######",
		"#.######",
		"#..#####",
		"##.#####",
		"##..####",
		"###.####",
		"###E####",
		"########"
	};

	const Point from = map.Start();
	const test::TestTraversability traversability { map };

	const Path plain = test::CallFindPath(map, traversability, from, map.End(), nullptr, 1, PF_SIGHT);
	const Path backing = test::CallFindPath(map, traversability, from, map.End(), nullptr, 1,
						PF_SIGHT | PF_BACKAWAY);

	ASSERT_FALSE(plain.Empty());
	ASSERT_FALSE(backing.Empty());
	// the flag must not change where the actor goes, only which way it faces getting there
	EXPECT_EQ(test::PathTiles(from, plain), test::PathTiles(from, backing));

	EXPECT_EQ(test::PathOrients(plain), test::ForwardOrients(from, plain));
	EXPECT_EQ(test::PathOrients(backing), test::BackAwayOrients(from, backing));
	EXPECT_GT(test::CountBackAwayOrients(from, backing), 0u)
		<< "this map has to actually trip the collinearity threshold, or the test is vacuous";
}

// A route whose corners are sharp gives the check nothing to catch, so the flag is a no-op.
TEST(FindPathTest, BackAwayLeavesSharpCornersAlone)
{
	const TestSearchMap map {
		"########",
		"#S.....#",
		"######.#",
		"#......#",
		"#.######",
		"#.....E#",
		"########"
	};

	const Point from = map.Start();
	const test::TestTraversability traversability { map };
	const Path backing = test::CallFindPath(map, traversability, from, map.End(), nullptr, 1,
						PF_SIGHT | PF_BACKAWAY);

	ASSERT_FALSE(backing.Empty());
	EXPECT_EQ(test::CountBackAwayOrients(from, backing), 0u) << "the corners here are all square";
	EXPECT_EQ(test::PathOrients(backing), test::ForwardOrients(from, backing));
}

// An actor backing away faces the threat it is retreating from, not the way it is going.
//
// DISABLED: Theta* collapses a straight run to a single waypoint, and the PF_BACKAWAY check in
// FindPath() (PathFinder.cpp, the `resultPath &&` guard) deliberately skips the first built node
// for the sake of the iwd ar1015 beetles. That node is the only one here, so on the plainest
// retreat - the case PF_BACKAWAY exists for - the flag changes nothing. The exception should be
// revisited once the waypoint-collapse and orientation issues are fixed; it may become unnecessary.
TEST(FindPathTest, DISABLED_BackAwayFacesTheThreatOnAStraightRetreat)
{
	// the threat is behind S, and the actor backs away due east
	const TestSearchMap map {
		"###############",
		"#S...........E#",
		"###############"
	};

	const Point from = map.Start();
	const test::TestTraversability traversability { map };
	const Path retreat = test::CallFindPath(map, traversability, from, map.End(), nullptr, 1,
						PF_SIGHT | PF_BACKAWAY);

	ASSERT_FALSE(retreat.Empty());
	const Point last = retreat.GetLastStep().point;
	EXPECT_EQ(retreat.GetLastStep().orient, GetOrient(last, from))
		<< "an actor backing away should be facing what it is backing away from";
}

// === algorithm quality ===

// Lazy Theta* only falls back to A* when the straight line from the grandparent is blocked, which
// in the open maps above almost never happens. A serpentine has a wall across nearly every line,
// so the fallback runs on most expansions and the route comes out as a chain of short legs.
TEST(FindPathTest, SerpentineMazeForcesTheAStarFallback)
{
	const TestSearchMap map {
		"###########",
		"#S........#",
		"#########.#",
		"#.........#",
		"#.#########",
		"#.........#",
		"#########.#",
		"#.......E.#",
		"###########"
	};

	const Point from = map.Start();
	const test::TestTraversability traversability { map };
	const Path path = test::CallFindPath(map, traversability, from, map.End());

	ASSERT_FALSE(path.Empty()) << "the maze has exactly one way through and it must be found";
	EXPECT_TRUE(test::PathIsSane(map, from, path));
	EXPECT_TRUE(test::PathAvoidsWalls(map, from, path));

	// a range rather than a number: the corner count is a tuning detail, but a route which has
	// collapsed to one or two legs cannot have gone round the bends
	EXPECT_GE(path.Size(), 6);
	EXPECT_LE(path.Size(), 8);
}

// The weighted heuristic makes the search greedy, so a goal walled off behind its own opening -
// facing away from the caller - is where sub-optimality could turn into failure or into a
// grotesque detour. The bound is deliberately loose: the pathfinder does not promise the shortest
// route, and a tight bound here would break on every retune of HEURISTIC_WEIGHT.
TEST(FindPathTest, FindsTheWayIntoAConcaveTrap)
{
	const TestSearchMap map {
		"###############",
		"#.............#",
		"#....#####....#",
		"#....#...#....#",
		"#.S..#.E.#....#",
		"#....#...#....#",
		"#....#.#.#....#",
		"#....#.#.#....#",
		"#....#.#.#....#",
		"#......#......#",
		"###############"
	};

	const Point from = map.Start();
	const test::TestTraversability traversability { map };
	const Path path = test::CallFindPath(map, traversability, from, map.End());

	ASSERT_FALSE(path.Empty()) << "the goal is reachable";
	EXPECT_TRUE(test::PathIsSane(map, from, path));
	EXPECT_TRUE(test::PathAvoidsWalls(map, from, path));
	EXPECT_LT(test::PathLength(from, path), 3u * unsigned(Distance(from, map.End())))
		<< "a detour can be accepted, a wild one cannot";
}

// === repeatability ===

// FindPath() keeps its working set in thread_local buffers sized by the map's area and clears
// them per run, so a run must neither depend on nor disturb the one before it. The shrinking case
// is the interesting one: the buffers keep the bigger map's capacity while the index arithmetic
// switches to the smaller map's width.
TEST(FindPathTest, RunsDoNotLeakIntoEachOther)
{
	// the big map is a maze, so a run leaves plenty of state behind to be disturbed
	const TestSearchMap big {
		"###########",
		"#S........#",
		"#########.#",
		"#.........#",
		"#.#########",
		"#.........#",
		"#########.#",
		"#.......E.#",
		"###########"
	};
	const TestSearchMap small {
		"######",
		"#S.###",
		"##..E#",
		"######"
	};
	const test::TestTraversability bigTraversability { big };
	const test::TestTraversability smallTraversability { small };

	const Path bigFirst = test::CallFindPath(big, bigTraversability, big.Start(), big.End());
	const Path smallAfterBig = test::CallFindPath(small, smallTraversability, small.Start(), small.End());
	const Path smallAgain = test::CallFindPath(small, smallTraversability, small.Start(), small.End());
	const Path bigAgain = test::CallFindPath(big, bigTraversability, big.Start(), big.End());

	ASSERT_FALSE(bigFirst.Empty());
	ASSERT_FALSE(smallAfterBig.Empty());
	ASSERT_GT(bigFirst.Size(), 1u) << "the big map has to produce a route with corners in it";
	EXPECT_EQ(smallAfterBig, smallAgain) << "the same request twice has to give the same answer";
	EXPECT_EQ(bigFirst, bigAgain) << "and the smaller map in between must not have disturbed it";
	EXPECT_EQ(SearchmapPoint(smallAfterBig.GetLastStep().point),
		  SearchmapPoint { small.End() })
		<< "the small map's route has to end on the small map's goal";
}

// === position adjustment ===

// AdjustPosition() is the undirected fallback: it scans outwards for somewhere passable. It picks
// with RandomFlip(), so what can be asserted is that the answer is somewhere the actor could
// stand, not which one it is.
TEST(PathFinderTest, AdjustPositionFindsPassableGround)
{
	const TestSearchMap map {
		"##########",
		"#XXXXXXXX#",
		"#XXXXXXXX#",
		"#XXXXX...#",
		"##########"
	};

	// the only floor in the map is (6,3) to (8,3)
	SearchmapPoint goal(2, 1);
	PathFinder::AdjustPosition(map.Props(), goal);
	EXPECT_TRUE(bool(PathFinder::GetBlockedTile(map.Props(), goal) & PathMapFlags::PASSABLE))
		<< "should have been adjusted to a passable tile, adjusted instead to impassable (" << goal.x << ',' << goal.y << ")";

	EXPECT_EQ(goal.y, 3);

	// a goal off the map is pulled inside it first
	SearchmapPoint far(99, 99);
	PathFinder::AdjustPosition(map.Props(), far);
	EXPECT_LT(far.x, map.Width());
	EXPECT_LT(far.y, map.Height());
	EXPECT_TRUE(bool(PathFinder::GetBlockedTile(map.Props(), far) & PathMapFlags::PASSABLE));
}

// === projectile lines ===

// CalculateLinePath() is what projectiles fly along, and its flags decide what a wall means:
// GL_NORMAL ends the flight there, GL_PASS ignores it, GL_REBOUND flies on but flips the
// orientation it stamps while it is over the wall.
TEST(PathFinderTest, LinePathStopsPassesOrReboundsAtAWall)
{
	const TestSearchMap map {
		"##########",
		"#........#",
		"#........#",
		"#S...#..E#",
		"#........#",
		"##########"
	};

	const Point start = map.Start();
	const Point dest = map.End();

	const Path stopped = PathFinder::CalculateLinePath(map.Props(), start, dest, 1, E, GL_NORMAL);
	const Path passed = PathFinder::CalculateLinePath(map.Props(), start, dest, 1, E, GL_PASS);
	const Path rebounded = PathFinder::CalculateLinePath(map.Props(), start, dest, 1, E, GL_REBOUND);

	ASSERT_FALSE(stopped.Empty());
	EXPECT_EQ(SearchmapPoint(stopped.GetLastStep().point), SearchmapPoint(5, 3))
		<< "GL_NORMAL ends the line on the wall it ran into";
	EXPECT_LT(stopped.Size(), passed.Size());

	EXPECT_EQ(SearchmapPoint(passed.GetLastStep().point), SearchmapPoint { dest })
		<< "GL_PASS carries on to the target";

	// GL_PASS keeps facing east the whole way; GL_REBOUND turns to face west over the wall. Only
	// the facing is asserted: where a rebounded line should *end up* is an open question, since
	// CalculateLinePath() still carries a TODO about mirroring the destination.
	const std::vector<orient_t> passedOrients = test::PathOrients(passed);
	const std::vector<orient_t> reboundedOrients = test::PathOrients(rebounded);
	EXPECT_EQ(std::count(passedOrients.begin(), passedOrients.end(), W), 0);
	EXPECT_GT(std::count(reboundedOrients.begin(), reboundedOrients.end(), W), 0);
	EXPECT_EQ(std::count(passedOrients.begin(), passedOrients.end(), E), long(passed.Size()));
}

// Bouncing off a wall has to change where a line ends, not only which way it faces.
//
// DISABLED: GL_REBOUND turns the orientation round at the wall but never mirrors the destination
// - the engine carries a TODO saying as much - so the rebounded line lands exactly where one that
// ignored the wall would have.
TEST(PathFinderTest, DISABLED_AReboundedLineDoesNotEndWhereAnUnimpededOneWould)
{
	const TestSearchMap map {
		"##########",
		"#........#",
		"#........#",
		"#S...#..E#",
		"#........#",
		"##########"
	};

	const Path passed = PathFinder::CalculateLinePath(map.Props(), map.Start(), map.End(), 1, E, GL_PASS);
	const Path rebounded = PathFinder::CalculateLinePath(map.Props(), map.Start(), map.End(), 1, E, GL_REBOUND);

	ASSERT_FALSE(passed.Empty());
	ASSERT_FALSE(rebounded.Empty());
	EXPECT_NE(rebounded.GetLastStep().point, passed.GetLastStep().point)
		<< "bouncing off a wall has to change where the line ends, not only which way it faces";
}

// CalculateLineEnd() has to stay on the map however far the line was told to run.
TEST(PathFinderTest, LineEndIsClampedToTheMap)
{
	const TestSearchMap map {
		"##########",
		"#........#",
		"#........#",
		"#S.......#",
		"#........#",
		"##########"
	};

	const PathNode end = PathFinder::CalculateLineEnd(map.Props(), map.Start(), 100, E);

	EXPECT_LE(end.point.x, (map.Width() - 1) * 16);
	EXPECT_LE(end.point.y, (map.Height() - 1) * 12);
	EXPECT_GE(end.point.x, 1);
	EXPECT_GE(end.point.y, 1);
	EXPECT_EQ(end.orient, E) << "and it still faces the way it was sent";
}

// === clearing an actor's footprint ===

// Clearing one actor's mark repaints its neighbours, because PaintSearchMap() cannot tell whose
// mark it is erasing: the footprints overlap, so a plain clear would punch a hole in the
// neighbour
TEST(TilePropsTest, ClearingAnActorKeepsAnOverlappingNeighbourMarked)
{
	// two circle size 2 actors three tiles apart, so their 3x3 footprints share a column
	TestSearchMap map {
		"#########",
		"#.......#",
		"#..b.b..#",
		"#.......#",
		"#########"
	};
	const test::MapRows expectedInitial {
		"#########",
		"#.NNNNN.#",
		"#.NNNNN.#",
		"#.NNNNN.#",
		"#########"
	};
	EXPECT_TRUE(map.Matches(expectedInitial))
		<< "the two footprints together cover one block";

	std::vector<ActorSearchMapData> snapshots;
	for (size_t i = 0; i < map.Actors().size(); ++i) {
		ActorSearchMapData data;
		// identity is compared, never dereferenced, same as everywhere else in these tests
		data.identity = reinterpret_cast<const Actor*>(map.ActorIdentityOf(i)); // NOSONAR
		data.pos = map.ActorPosOf(i);
		data.smPos = SearchmapPoint { data.pos };
		data.circleSize = map.ActorCircleSizeOf(i);
		data.isPC = false;
		data.blocksSearchMap = true;
		snapshots.push_back(data);
	}

	const ActorSearchMapData& leaving = snapshots[0];
	PathFinder::ClearSearchMapFor(snapshots, reinterpret_cast<const Movable*>(leaving.identity), // NOSONAR
				      leaving.pos, leaving.smPos, leaving.circleSize, map.Props());

	// the first actor's own tiles are free again, and everything the second one covers is not
	const test::MapRows expectedAfterClear {
		"#########",
		"#...NNN.#",
		"#...NNN.#",
		"#...NNN.#",
		"#########"
	};
	EXPECT_TRUE(map.Matches(expectedAfterClear))
		<< "the shared column at x == 4 belongs to the actor which is still standing there";
}

// === degenerate requests ===

// None of these should crash or produce a path which cannot be walked.
TEST(FindPathTest, DegenerateRequests)
{
	const TestSearchMap map {
		"#####",
		"#S.E#",
		"#####"
	};

	const test::TestTraversability traversability { map };
	const Point from = map.Start();

	EXPECT_TRUE(test::CallFindPath(map, traversability, from, from).Empty())
		<< "there is nowhere to walk to";
	EXPECT_TRUE(test::CallFindPath(map, traversability, TestSearchMap::Nav(99, 99), map.End()).Empty())
		<< "a caller outside the map is refused rather than clamped";

	// a click off the map is brought back onto it
	const Path offMap = test::CallFindPath(map, traversability, from, TestSearchMap::Nav(99, 99));
	ASSERT_FALSE(offMap.Empty());
	EXPECT_TRUE(test::PathIsSane(map, from, offMap));

	// a one tile map has nowhere to go, from anywhere
	const TestSearchMap single {
		"."
	};
	const test::TestTraversability singleTraversability { single };
	EXPECT_TRUE(test::CallFindPath(single, singleTraversability, TestSearchMap::Nav(0, 0),
				       TestSearchMap::Nav(0, 0))
			    .Empty());
	EXPECT_TRUE(test::CallFindPath(single, singleTraversability, TestSearchMap::Nav(0, 0),
				       TestSearchMap::Nav(99, 99))
			    .Empty());
}

// === circle size footprints ===
//
// A circle size names two footprints, and they are deliberately different sizes:
//
//   blocking, radius circleSize - 1, stamped by PaintSearchMap(). The ground the actor takes up
//                                    as far as everybody else is concerned, so what other actors
//                                    have to path around.
//   standing, radius circleSize - 2, asked about by GetBlockedInRadiusTile(). The ground the
//                                    actor itself needs to stand on, so what decides whether it
//                                    fits somewhere. It is a query, not state, so
//                                    MatchesStandingFootprint() has to probe for it.
//
// Blocking is the larger of the two by one whole size, and deliberately so. PaintSearchMap()
// says why, in as many words:
//
//     Note: this is a larger circle than the one tested in GetBlocked.
//     This means that an actor can get closer to a wall than to another
//     actor. This matches the behaviour of the original BG2.
//
// GetBlockedInRadiusTile() also clamps its size up to 2, so circle sizes 1 and 2 share a standing
// footprint of a single tile - which is why the two smallest creatures fit through the same gaps.
TEST(TilePropsTest, FootprintsOfCircleSizeOne)
{
	const TestSearchMap map {
		"...",
		".1.",
		"..."
	};
	const test::MapRows blocking {
		"...",
		".P.",
		"..."
	};
	const test::MapRows standing {
		"...",
		".P.",
		"..."
	};

	const TestSearchMap::DrawnActor& pc = map.Actors()[0];
	EXPECT_TRUE(map.Matches(blocking));
	EXPECT_TRUE(map.MatchesStandingFootprint(pc.tile, pc.circleSize, standing));
}

TEST(TilePropsTest, FootprintsOfCircleSizeTwo)
{
	const TestSearchMap map {
		".....",
		".....",
		"..2..",
		".....",
		"....."
	};
	const test::MapRows blocking {
		".....",
		".PPP.",
		".PPP.",
		".PPP.",
		"....."
	};
	const test::MapRows standing {
		".....",
		".....",
		"..P..",
		".....",
		"....."
	};

	const TestSearchMap::DrawnActor& pc = map.Actors()[0];
	EXPECT_TRUE(map.Matches(blocking));
	EXPECT_TRUE(map.MatchesStandingFootprint(pc.tile, pc.circleSize, standing));
}

TEST(TilePropsTest, FootprintsOfCircleSizeThree)
{
	const TestSearchMap map {
		".......",
		".......",
		".......",
		"...3...",
		".......",
		".......",
		"......."
	};
	const test::MapRows blocking {
		".......",
		"..PPP..",
		".PPPPP.",
		".PPPPP.",
		".PPPPP.",
		"..PPP..",
		"......."
	};
	const test::MapRows standing {
		".......",
		".......",
		"..PPP..",
		"..PPP..",
		"..PPP..",
		".......",
		"......."
	};

	const TestSearchMap::DrawnActor& pc = map.Actors()[0];
	EXPECT_TRUE(map.Matches(blocking));
	EXPECT_TRUE(map.MatchesStandingFootprint(pc.tile, pc.circleSize, standing));
}

TEST(TilePropsTest, FootprintsOfCircleSizeFour)
{
	const TestSearchMap map {
		".........",
		".........",
		".........",
		".........",
		"....4....",
		".........",
		".........",
		".........",
		"........."
	};
	const test::MapRows blocking {
		".........",
		"...PPP...",
		"..PPPPP..",
		".PPPPPPP.",
		".PPPPPPP.",
		".PPPPPPP.",
		"..PPPPP..",
		"...PPP...",
		"........."
	};
	const test::MapRows standing {
		".........",
		".........",
		"...PPP...",
		"..PPPPP..",
		"..PPPPP..",
		"..PPPPP..",
		"...PPP...",
		".........",
		"........."
	};

	const TestSearchMap::DrawnActor& pc = map.Actors()[0];
	EXPECT_TRUE(map.Matches(blocking));
	EXPECT_TRUE(map.MatchesStandingFootprint(pc.tile, pc.circleSize, standing));
}

TEST(TilePropsTest, FootprintsOfCircleSizeFive)
{
	const TestSearchMap map {
		"...........",
		"...........",
		"...........",
		"...........",
		"...........",
		".....5.....",
		"...........",
		"...........",
		"...........",
		"...........",
		"..........."
	};
	const test::MapRows blocking {
		"...........",
		"...PPPPP...",
		"..PPPPPPP..",
		".PPPPPPPPP.",
		".PPPPPPPPP.",
		".PPPPPPPPP.",
		".PPPPPPPPP.",
		".PPPPPPPPP.",
		"..PPPPPPP..",
		"...PPPPP...",
		"..........."
	};
	const test::MapRows standing {
		"...........",
		"...........",
		"....PPP....",
		"...PPPPP...",
		"..PPPPPPP..",
		"..PPPPPPP..",
		"..PPPPPPP..",
		"...PPPPP...",
		"....PPP....",
		"...........",
		"..........."
	};

	const TestSearchMap::DrawnActor& pc = map.Actors()[0];
	EXPECT_TRUE(map.Matches(blocking));
	EXPECT_TRUE(map.MatchesStandingFootprint(pc.tile, pc.circleSize, standing));
}

TEST(TilePropsTest, FootprintsOfCircleSizeSix)
{
	const TestSearchMap map {
		".............",
		".............",
		".............",
		".............",
		".............",
		".............",
		"......6......",
		".............",
		".............",
		".............",
		".............",
		".............",
		"............."
	};
	const test::MapRows blocking {
		".............",
		"....PPPPP....",
		"...PPPPPPP...",
		"..PPPPPPPPP..",
		".PPPPPPPPPPP.",
		".PPPPPPPPPPP.",
		".PPPPPPPPPPP.",
		".PPPPPPPPPPP.",
		".PPPPPPPPPPP.",
		"..PPPPPPPPP..",
		"...PPPPPPP...",
		"....PPPPP....",
		"............."
	};
	const test::MapRows standing {
		".............",
		".............",
		"....PPPPP....",
		"...PPPPPPP...",
		"..PPPPPPPPP..",
		"..PPPPPPPPP..",
		"..PPPPPPPPP..",
		"..PPPPPPPPP..",
		"..PPPPPPPPP..",
		"...PPPPPPP...",
		"....PPPPP....",
		".............",
		"............."
	};

	const TestSearchMap::DrawnActor& pc = map.Actors()[0];
	EXPECT_TRUE(map.Matches(blocking));
	EXPECT_TRUE(map.MatchesStandingFootprint(pc.tile, pc.circleSize, standing));
}

TEST(TilePropsTest, FootprintsOfCircleSizeSeven)
{
	const TestSearchMap map {
		"...............",
		"...............",
		"...............",
		"...............",
		"...............",
		"...............",
		"...............",
		".......7.......",
		"...............",
		"...............",
		"...............",
		"...............",
		"...............",
		"...............",
		"..............."
	};
	const test::MapRows blocking {
		"...............",
		".....PPPPP.....",
		"...PPPPPPPPP...",
		"..PPPPPPPPPPP..",
		"..PPPPPPPPPPP..",
		".PPPPPPPPPPPPP.",
		".PPPPPPPPPPPPP.",
		".PPPPPPPPPPPPP.",
		".PPPPPPPPPPPPP.",
		".PPPPPPPPPPPPP.",
		"..PPPPPPPPPPP..",
		"..PPPPPPPPPPP..",
		"...PPPPPPPPP...",
		".....PPPPP.....",
		"..............."
	};
	const test::MapRows standing {
		"...............",
		"...............",
		".....PPPPP.....",
		"....PPPPPPP....",
		"...PPPPPPPPP...",
		"..PPPPPPPPPPP..",
		"..PPPPPPPPPPP..",
		"..PPPPPPPPPPP..",
		"..PPPPPPPPPPP..",
		"..PPPPPPPPPPP..",
		"...PPPPPPPPP...",
		"....PPPPPPP....",
		".....PPPPP.....",
		"...............",
		"..............."
	};

	const TestSearchMap::DrawnActor& pc = map.Actors()[0];
	EXPECT_TRUE(map.Matches(blocking));
	EXPECT_TRUE(map.MatchesStandingFootprint(pc.tile, pc.circleSize, standing));
}

TEST(TilePropsTest, FootprintsOfCircleSizeEight)
{
	const TestSearchMap map {
		".................",
		".................",
		".................",
		".................",
		".................",
		".................",
		".................",
		".................",
		"........8........",
		".................",
		".................",
		".................",
		".................",
		".................",
		".................",
		".................",
		"................."
	};
	const test::MapRows blocking {
		".................",
		"......PPPPP......",
		"....PPPPPPPPP....",
		"...PPPPPPPPPPP...",
		"..PPPPPPPPPPPPP..",
		"..PPPPPPPPPPPPP..",
		".PPPPPPPPPPPPPPP.",
		".PPPPPPPPPPPPPPP.",
		".PPPPPPPPPPPPPPP.",
		".PPPPPPPPPPPPPPP.",
		".PPPPPPPPPPPPPPP.",
		"..PPPPPPPPPPPPP..",
		"..PPPPPPPPPPPPP..",
		"...PPPPPPPPPPP...",
		"....PPPPPPPPP....",
		"......PPPPP......",
		"................."
	};
	const test::MapRows standing {
		".................",
		".................",
		"......PPPPP......",
		"....PPPPPPPPP....",
		"...PPPPPPPPPPP...",
		"...PPPPPPPPPPP...",
		"..PPPPPPPPPPPPP..",
		"..PPPPPPPPPPPPP..",
		"..PPPPPPPPPPPPP..",
		"..PPPPPPPPPPPPP..",
		"..PPPPPPPPPPPPP..",
		"...PPPPPPPPPPP...",
		"...PPPPPPPPPPP...",
		"....PPPPPPPPP....",
		"......PPPPP......",
		".................",
		"................."
	};

	const TestSearchMap::DrawnActor& pc = map.Actors()[0];
	EXPECT_TRUE(map.Matches(blocking));
	EXPECT_TRUE(map.MatchesStandingFootprint(pc.tile, pc.circleSize, standing));
}

// The routing consequence of the footprints above: the standing footprint is what has to fit, so
// the slit takes both of the sizes that stand on a single tile and refuses the first one that
// does not.
TEST(FindPathTest, TheSlitTakesTheSizesWhoseStandingFootprintFits)
{
	// the slit at (10,5) is the only way across the divider
	const TestSearchMap map {
		"#####################",
		"#.........#.........#",
		"#.........#.........#",
		"#.........#.........#",
		"#.........#.........#",
		"#....S.........E....#",
		"#.........#.........#",
		"#.........#.........#",
		"#.........#.........#",
		"#.........#.........#",
		"#####################"
	};

	const Point from = map.Start();
	const test::TestTraversability traversability { map };

	const Path one = test::CallFindPath(map, traversability, from, map.End(), nullptr, 1);
	const Path two = test::CallFindPath(map, traversability, from, map.End(), nullptr, 2);
	const Path three = test::CallFindPath(map, traversability, from, map.End(), nullptr, 3);

	ASSERT_FALSE(one.Empty());
	EXPECT_TRUE(test::PathIsSane(map, from, one));
	EXPECT_EQ(one, two) << "the same standing footprint is the same request";
	EXPECT_TRUE(three.Empty()) << "a size 3 actor stands on 3x3 and the slit is one tile wide";
}
}
