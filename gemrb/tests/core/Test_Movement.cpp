// SPDX-FileCopyrightText: 2026 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

// Movement tests: the real per-frame loop, with bumping, backoff and repathing.
// The live-game scaffolding lives in TestGameFixture.h; what is here is movement specific.

#if defined(USE_OPENGL_BACKEND) || (!defined(__APPLE__) && !defined(WIN32))

	#include "TestGameMap.h"

	#include <gtest/gtest.h>

namespace GemRB {


class MovementTest : public GameMapTest {
};

// === helpers ===

// The invariant that catches tunnelling. One frame moves the actor a good fraction of a tile,
// so it can stand on open floor before the frame and on open floor after it and still have
// passed straight through a wall in between. Speed 0 samples the segment as finely as the
// engine ever steps, so a wall cannot hide between two samples.
static testing::AssertionResult StepStayedOffWalls(const TestSearchMap& drawn, const Point& from, const Point& to)
{
	constexpr int noSpeed = 0;
	constexpr int noCircle = 0;
	const PathMapFlags crossed = PathFinder::GetBlockedInLine(drawn.Props(), from, to, false, noSpeed, noCircle);
	if (bool(crossed & (PathMapFlags::SIDEWALL | PathMapFlags::DOOR_IMPASSABLE))) {
		return testing::AssertionFailure()
			<< "step (" << from.x << ',' << from.y << ") -> (" << to.x << ',' << to.y << ") crosses a wall";
	}
	return testing::AssertionSuccess();
}

// Runs frames until the actor stops moving, checking every single step against a wall. Returns
// how many frames it took, or the budget if it never arrived.
static int WalkUntilStopped(const TestGameMap& live, Actor* actor, int frameBudget = 200)
{
	int frames = 0;
	for (; frames < frameBudget && actor->InMove(); ++frames) {
		const Point before = actor->Pos;
		TestGameLoop::RunFrame();
		EXPECT_TRUE(StepStayedOffWalls(live.Drawing(), before, actor->Pos)) << "on frame " << frames;
	}
	return frames;
}

// === TestGameMap infrastructure tests ===

// A drawn map can carry a real actor, and the real pathfinder answers for it.
TEST_F(MovementTest, DrawnMapCarriesARealActor)
{
	// '2' is a party member of circle size 2, standing where the glyph is
	const TestGameMap live {
		"##########",
		"#........#",
		"#..2....E#",
		"#........#",
		"##########"
	};
	const TestSearchMap& drawn = live.Drawing();

	ASSERT_EQ(live.ActorCount(), 1);
	Actor* actor = live.ActorOf(0);
	ASSERT_NE(actor, nullptr) << "the demo has to provide a creature data to walk around";

	EXPECT_NE(actor->GetAnims(), nullptr) << "UpdateScripts dereferences this for a moving actor";
	EXPECT_EQ(actor->GetCurrentArea(), live.GetMap());
	EXPECT_GT(actor->GetSpeed(), 0) << "a speed of 0 means DoStep() never moves it";
	EXPECT_EQ(actor->Pos, drawn.ActorPosOf(0)) << "the glyph says where it stands";
	EXPECT_EQ(actor->InParty, 1) << "a digit glyph is a party member";

	actor->WalkTo(drawn.End(), 0, 0);
	// the pathfiner scheduler runs in immediate mode, so above call is answered synchronously
	EXPECT_TRUE(actor->InMove()) << "WalkTo() should have put the actor in a moving state";
	EXPECT_FALSE(actor->GetPath().Empty()) << "WalkTo() should have produced a path";
}

// The glyph decides which creature is spawned, so a drawing gets the size it asked for
// and it properly honours `InParty` value
TEST_F(MovementTest, GlyphsDecideSizeAndParty)
{
	TestGameMap live {
		"###################",
		"#.................#",
		"#.................#",
		"#....1......b.....#",
		"#.................#",
		"#.................#",
		"#.................#",
		"###################"
	};
	const TestSearchMap& drawn = live.Drawing();

	ASSERT_EQ(live.ActorCount(), 2);

	// the small one is a party member, the letter is not
	EXPECT_EQ(live.ActorOf(0)->circleSize, 1);
	EXPECT_EQ(live.ActorOf(0)->InParty, 1);
	EXPECT_EQ(live.ActorOf(1)->circleSize, 2);
	EXPECT_EQ(live.ActorOf(1)->InParty, 0);

	// and both stand where they were drawn
	EXPECT_EQ(live.ActorOf(0)->Pos, drawn.ActorPosOf(0));
	EXPECT_EQ(live.ActorOf(1)->Pos, drawn.ActorPosOf(1));

	// the live actors reach the cache the pathfinder consults, each owning its own cell
	live.RefreshTraversability();
	EXPECT_GT(live.StateAt(drawn.ActorPosOf(0)), TraversabilityCache::TraversabilityCellValueEmpty);
	EXPECT_EQ(live.ActorAt(drawn.ActorPosOf(0)), live.ActorOf(0));
	EXPECT_EQ(live.ActorAt(drawn.ActorPosOf(1)), live.ActorOf(1));
}

// Every size the glyph alphabet allows, has a creature behind it.
// Sizes 3 to 8 are the test-only creatures in demo/override.
TEST_F(MovementTest, EveryDrawnSizeHasACreature)
{
	const TestGameMap live {
		"###########################",
		"#.1a.2b.3c.4d.5e.6f.7g.8h.#",
		"###########################"
	};

	constexpr size_t sizeCount = 8;
	ASSERT_EQ(live.ActorCount(), sizeCount * 2);
	for (size_t i = 0; i < sizeCount; ++i) {
		const uint16_t expectedCircleSize = i + 1;
		const size_t numberGlyphIdx = i * 2;
		const char numberGlyphChar = static_cast<char>(i) + test::Glyph::FirstPC;
		const size_t letterGlyphIdx = i * 2 + 1;
		const char letterGlyphChar = static_cast<char>(i) + test::Glyph::FirstNPC;
		// test number
		EXPECT_EQ(live.Drawing().ActorCircleSizeOf(numberGlyphIdx), expectedCircleSize) << "glyph " << numberGlyphChar;
		EXPECT_EQ(live.ActorOf(numberGlyphIdx)->circleSize, expectedCircleSize) << "live actor " << numberGlyphChar;
		// test letter
		EXPECT_EQ(live.Drawing().ActorCircleSizeOf(letterGlyphIdx), expectedCircleSize) << "glyph " << letterGlyphChar;
		EXPECT_EQ(live.ActorOf(letterGlyphIdx)->circleSize, expectedCircleSize) << "live actor " << letterGlyphChar;
	}
}

// The plain case: an open corridor, walked end to end.
// Test if basic assumptions about the simple walk hold.
TEST_F(MovementTest, WalksToItsDestination)
{
	TestGameMap live {
		"##########",
		"#........#",
		"#2......E#",
		"#........#",
		"##########"
	};
	const TestSearchMap& drawn = live.Drawing();
	Actor* actor = live.ActorOf(0);
	ASSERT_NE(actor, nullptr);

	actor->WalkTo(drawn.End(), 0, 0);
	ASSERT_FALSE(actor->GetPath().Empty()) << "the walk needs a path to follow";

	const int frames = WalkUntilStopped(live, actor);
	EXPECT_LT(frames, 200) << "the walk has to finish inside the frame budget";
	EXPECT_GT(frames, 1) << "arriving in one frame would mean it teleported, not walked";
	EXPECT_EQ(actor->Pos, drawn.End()) << "and it has to end up where it was sent";
}

// Live map uses as its searchmap the drawing's own buffer, so a drawing reads back the
// live state: in consequence the whole glyph vocabulary should work on live state.
// Test if simple walk can be asserted with ASCII map and actor doesn't leave any stale
// footprints on the searchmap.
TEST_F(MovementTest, LeavesNoStaleFootprintWhereItStarted)
{
	const TestGameMap live {
		"##########",
		"#........#",
		"#2......E#",
		"#........#",
		"##########"
	};
	const TestSearchMap& drawn = live.Drawing();
	Actor* actor = live.ActorOf(0);

	actor->WalkTo(drawn.End(), 0, 0);
	ASSERT_GT(WalkUntilStopped(live, actor), 1);

	// only the footprint it is standing in now, nothing left behind at the start
	const test::MapRows expected {
		"##########",
		"#......PP#",
		"#......PP#",
		"#......PP#",
		"##########"
	};
	EXPECT_TRUE(drawn.Matches(expected));
}

// The simple case of walk with obstacle: the direct line is blocked, so the route has to bend
// around a barrier. Verify every frame if the actor hasn't tunneled through wall.
TEST_F(MovementTest, WalksAroundABarrierWithoutCrossingIt)
{
	const TestGameMap live {
		"###########",
		"#2...#E...#",
		"#....#....#",
		"#....#....#",
		"#.........#",
		"###########"
	};
	const TestSearchMap& drawn = live.Drawing();
	Actor* actor = live.ActorOf(0);
	ASSERT_NE(actor, nullptr);

	// assert that the straight line between the two is blocked, so the per-frame check has something to
	// catch and is not quietly passing on every input
	ASSERT_FALSE(StepStayedOffWalls(drawn, drawn.ActorPosOf(0), drawn.End()))
		<< "the barrier has to lie across the direct line, or this test proves nothing";

	actor->WalkTo(drawn.End(), 0, 0);
	ASSERT_FALSE(actor->GetPath().Empty());

	const int frames = WalkUntilStopped(live, actor);
	EXPECT_LT(frames, 200);
	EXPECT_GT(frames, 1);
	EXPECT_EQ(actor->Pos, drawn.End());
}

}

#endif
