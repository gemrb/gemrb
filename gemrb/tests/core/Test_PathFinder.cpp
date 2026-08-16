// SPDX-FileCopyrightText: 2026 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

// Terrain-level pathfinder tests. Everything here runs on a TileProps built from an ASCII
// literal, so no game data, no Interface and no video driver are involved.
// See SearchMapBuilder.h for the glyphs.

#include "SearchMapBuilder.h"

#include "../../core/PathFinder.h"

#include <algorithm>
#include <gtest/gtest-spi.h>
#include <gtest/gtest.h>
#include <map>
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

// traversability
TEST(TraversabilityTest, BumpableActorBlocksOnlyWhenActorsAreBlocking)
{
	// a one tile corridor with a blocker standing in it, so it cannot be walked around
	const TestSearchMap map {
		"#########",
		"#S..a..E#",
		"#########"
	};

	const char selfTag = 0;
	const test::ActorIdentity self = test::MakeActorIdentity(selfTag);

	const Point from = map.Start();
	const Point to = map.End();

	const test::TestTraversability traversability { map, true };

	// intending to bump: the blocker is transparent, so the route runs straight through it
	const Path bumping = test::CallFindPath(map, traversability, from, to, self, 1, PF_SIGHT);
	EXPECT_FALSE(bumping.Empty()) << "a bumpable actor must not stop a route that would bump it";

	// treating actors as solid: there is no way past in a one tile corridor
	const Path yielding = test::CallFindPath(map, traversability, from, to, self, 1,
						 PF_SIGHT | PF_ACTORS_ARE_BLOCKING);
	EXPECT_TRUE(yielding.Empty()) << "a blocking actor plugs the only corridor, so there is no route";
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

	// nothing to route around, so the pathfinder returns a single waypoint on E and the whole
	// corridor between is the line walk getting there
	const test::MapRows expected {
		"##########",
		"#*******@#",
		"##########"
	};
	EXPECT_TRUE(map.MatchesWithPath(map.Start(), path, expected));
}

TEST(FindPathTest, FindPathOnStillSeesTheDrawnActors)
{
	// one tile wide, so the drawn NPC cannot be walked around
	const TestSearchMap map {
		"#########",
		"#S..a..E#",
		"#########"
	};

	const test::TestTraversability traversability { map };
	ASSERT_EQ(traversability.StateAt(map.ActorPosOf(0)), TraversabilityCache::TraversabilityCellValueActor)
		<< "the glyph has to reach the cache FindPathOn() will build";

	EXPECT_FALSE(test::CallFindPath(map, traversability, map.Start(), map.End()).Empty())
		<< "a bumpable actor is transparent under the default flags";

	EXPECT_TRUE(test::CallFindPath(map, traversability, map.Start(), map.End(), nullptr, 1, PF_SIGHT | PF_ACTORS_ARE_BLOCKING).Empty())
		<< "actor blocks the only corridor once actors are treated as blocking";
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

	// it has to come round the open bottom row, so it has to get more >1 waypoint
	EXPECT_GT(path.Size(), 1u);

	// it must never set foot on the barrier
	for (const SearchmapPoint& tile : test::PathTiles(start, path)) {
		EXPECT_NE(map.At(tile.x, tile.y), PathMapFlags::SIDEWALL)
			<< "route enters the wall at (" << tile.x << ',' << tile.y << ')';
	}
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
		"#..1..4........E....#",
		"#.........#.........#",
		"#.........#.........#",
		"#.........#.........#",
		"#.........#.........#",
		"#####################"
	};

	constexpr size_t smallActor = 0;
	constexpr size_t bigActor = 1;
	ASSERT_EQ(map.Actors().size(), 2);
	ASSERT_EQ(map.ActorCircleSizeOf(smallActor), 1);
	ASSERT_EQ(map.ActorCircleSizeOf(bigActor), 4);

	// bumpable, so neither walker is stopped by the other's mark; only the terrain is in play
	const test::TestTraversability traversability { map, true };
	const Point to = map.End();

	const Path small = test::PathDrawnActor(map, traversability, smallActor, to);
	ASSERT_FALSE(small.Empty()) << "a one tile wide actor fits through a one tile wide slit";
	EXPECT_TRUE(test::PathAvoidsWalls(map, map.ActorPosOf(smallActor), small));

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
	EXPECT_TRUE(map.MatchesWithPath(map.Start(), path, expected));
}
}
