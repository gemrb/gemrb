// SPDX-FileCopyrightText: 2026 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

// Terrain-level pathfinder tests. Everything here runs on a TileProps built from an ASCII
// literal, so no game data, no Interface and no video driver are involved.
// See SearchMapBuilder.h for the glyphs.

#include "SearchMapBuilder.h"

#include "../../core/PathFinder.h"

#include <gtest/gtest-spi.h>
#include <gtest/gtest.h>

namespace GemRB {

using test::TestSearchMap;

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
	EXPECT_NONFATAL_FAILURE(test::ParseSearchMapChar('@'), "unknown searchmap glyph '@'");

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
	for (char g = '1'; g <= '8'; ++g) {
		EXPECT_TRUE(test::IsActorGlyph(g));
	}
	for (char g = 'a'; g <= 'h'; ++g) {
		EXPECT_TRUE(test::IsActorGlyph(g));
	}

	// terrain never counts as an actor
	EXPECT_FALSE(test::IsActorGlyph('.'));
	EXPECT_FALSE(test::IsActorGlyph('#'));
	EXPECT_FALSE(test::IsActorGlyph('D'));

	// sanity: just outside either range
	EXPECT_FALSE(test::IsActorGlyph('0'));
	EXPECT_FALSE(test::IsActorGlyph('9'));
	EXPECT_FALSE(test::IsActorGlyph('i'));
	EXPECT_FALSE(test::IsActorGlyph('A'));

	// digits are party members, letters are NPCs
	EXPECT_EQ(test::ActorGlyphFlag('1'), PathMapFlags::PC);
	EXPECT_EQ(test::ActorGlyphFlag('8'), PathMapFlags::PC);
	EXPECT_EQ(test::ActorGlyphFlag('a'), PathMapFlags::NPC);
	EXPECT_EQ(test::ActorGlyphFlag('h'), PathMapFlags::NPC);

	// both alphabets run over the same circleSize range of 1 to 8
	uint16_t circleSize;
	char glyph;
	for (circleSize = 1, glyph = '1'; glyph <= '8'; ++glyph, ++circleSize) {
		EXPECT_EQ(test::ActorGlyphCircleSize(glyph), circleSize);
	}
	for (circleSize = 1, glyph = 'a'; glyph <= 'h'; ++glyph, ++circleSize) {
		EXPECT_EQ(test::ActorGlyphCircleSize(glyph), circleSize);
	}

	// the terrain an actor stands on is plain floor; the footprint is painted afterwards
	for (char g = '1'; g <= '8'; ++g) {
		EXPECT_EQ(test::ParseSearchMapChar(g), PathMapFlags::PASSABLE);
	}
	for (char g = 'a'; g <= 'h'; ++g) {
		EXPECT_EQ(test::ParseSearchMapChar(g), PathMapFlags::PASSABLE);
	}
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

TEST(SearchMapBuilderTest, RendersFlagsBackToGlyphs)
{
	EXPECT_EQ(test::RenderSearchMapChar(PathMapFlags::PASSABLE), '.');
	EXPECT_EQ(test::RenderSearchMapChar(PathMapFlags::SIDEWALL), '#');
	EXPECT_EQ(test::RenderSearchMapChar(PathMapFlags::IMPASSABLE), 'X');
	EXPECT_EQ(test::RenderSearchMapChar(PathMapFlags::PASSABLE | PathMapFlags::TRAVEL), 'T');
	EXPECT_EQ(test::RenderSearchMapChar(PathMapFlags::PASSABLE | PathMapFlags::PC), 'P');
	EXPECT_EQ(test::RenderSearchMapChar(PathMapFlags::PASSABLE | PathMapFlags::NPC), 'N');
	EXPECT_EQ(test::RenderSearchMapChar(PathMapFlags::PASSABLE | PathMapFlags::TRAVEL | PathMapFlags::PC), 'p');

	// an actor mark somewhere it can never legally be
	EXPECT_EQ(test::RenderSearchMapChar(PathMapFlags::SIDEWALL | PathMapFlags::PC), '!');
	EXPECT_EQ(test::RenderSearchMapChar(PathMapFlags::PC), '!');
	EXPECT_EQ(test::RenderSearchMapChar(PathMapFlags::PASSABLE | PathMapFlags::DOOR_IMPASSABLE | PathMapFlags::NPC), '!');
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
		"#...##...#",
		"#..2##2..#",
		"#...##...#",
		"##########"
	};

	// both actors press right up against the middle wall, and it stays a wall
	const test::MapRows expected {
		"##########",
		"#.PP##PP.#",
		"#.PP##PP.#",
		"#.PP##PP.#",
		"##########"
	};
	ASSERT_TRUE(map.Matches(expected));

	const Point west = TestSearchMap::Nav(2, 2);
	const Point east = TestSearchMap::Nav(7, 2);

	EXPECT_FALSE(PathFinder::IsWalkableTo(map.Props(), west, east, true, noSpeed, noCircle));
	EXPECT_FALSE(PathFinder::IsWalkableTo(map.Props(), west, east, false, noSpeed, noCircle))
		<< "ignoring actors must not ignore the wall between them";

	// the wall also still blocks sight
	EXPECT_FALSE(PathFinder::IsVisibleLOS(map.Props(), SearchmapPoint(2, 2), SearchmapPoint(7, 2), noSpeed, noCircle));

	// ... while the untouched column on the far side of the room is walkable
	EXPECT_TRUE(PathFinder::IsWalkableTo(map.Props(), TestSearchMap::Nav(1, 1), TestSearchMap::Nav(1, 3), true, noSpeed, noCircle));
}

// An actor in the way is only an obstacle while actors are blocking; open floor beneath it
// must stay walkable for the ignore-actors queries the pathfinder makes.
TEST(PathFinderTest, ActorBlocksOnlyWhenActorsAreBlocking)
{
	const TestSearchMap map {
		"########",
		"#..a...#",
		"########"
	};

	const test::MapRows expected {
		"########",
		"#..N...#",
		"########"
	};
	ASSERT_TRUE(map.Matches(expected));

	const Point from = TestSearchMap::Nav(1, 1);
	const Point to = TestSearchMap::Nav(6, 1);

	EXPECT_FALSE(PathFinder::IsWalkableTo(map.Props(), from, to, true, noSpeed, noCircle));
	EXPECT_TRUE(PathFinder::IsWalkableTo(map.Props(), from, to, false, noSpeed, noCircle));
}

}
