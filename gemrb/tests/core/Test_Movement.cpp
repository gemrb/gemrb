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
	constexpr int noCircle = 0;
	const PathMapFlags crossed = PathFinder::GetBlockedInLine(drawn.Props(), from, to, false, noCircle);
	if (bool(crossed & (PathMapFlags::SIDEWALL | PathMapFlags::DOOR_IMPASSABLE))) {
		return testing::AssertionFailure()
			<< "step (" << from.x << ',' << from.y << ") -> (" << to.x << ',' << to.y << ") crosses a wall";
	}
	return testing::AssertionSuccess();
}

// The engine's rule (Movable::DoStep): a position is inside a wall only when all four pixels
// bracketing it are wall face. A segment probe would false-flag the corner passes the engine
// allows.
static testing::AssertionResult StoodClearOfWalls(const TestSearchMap& drawn, const Point& p)
{
	const auto wallAt = [&drawn](int x, int y) {
		return bool(PathFinder::GetBlockedTile(drawn.Props(), SearchmapPoint(Point(x, y))) & PathMapFlags::SIDEWALL);
	};
	if (wallAt(p.x, p.y) && wallAt(p.x + 1, p.y) && wallAt(p.x, p.y + 1) && wallAt(p.x + 1, p.y + 1)) {
		return testing::AssertionFailure()
			<< "(" << p.x << ',' << p.y << ") is demonstrably inside a wall";
	}
	return testing::AssertionSuccess();
}

// Runs frames until the actor stops moving, checking every single step against a wall. Returns
// how many frames it took, or the budget if it never arrived.
static int WalkUntilStopped(const TestGameMap& live, const Actor* actor, int frameBudget = 200)
{
	int frames = 0;
	for (; frames < frameBudget && actor->InMove(); ++frames) {
		TestGameLoop::RunFrame();
		EXPECT_TRUE(StoodClearOfWalls(live.Drawing(), actor->Pos)) << "on frame " << frames;
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

// Every size the glyph alphabet allows, has a creature with proper stats behind it.
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

// The complementary to the walk above, for actor which needs to stand on more than one tile.
TEST_F(MovementTest, StopsShortOfADestinationItDoesNotFitInto)
{
	TestGameMap live {
		"###############",
		"#.............#",
		"#.............#",
		"#...3........E#",
		"#.............#",
		"#.............#",
		"###############"
	};
	const TestSearchMap& drawn = live.Drawing();
	Actor* actor = live.ActorOf(0);
	ASSERT_NE(actor, nullptr);
	ASSERT_EQ(actor->circleSize, 3);

	const SearchmapPoint endTile { drawn.End() };
	// E lies against the wall: room enough for small actors, but not for this one:
	EXPECT_TRUE(bool(PathFinder::GetBlockedInRadiusTile(drawn.Props(), endTile, 2) & PathMapFlags::PASSABLE))
		<< "the size <=2 has to still fit";
	ASSERT_FALSE(bool(PathFinder::GetBlockedInRadiusTile(drawn.Props(), endTile, 3) & PathMapFlags::PASSABLE))
		<< "for this size the pathfinder should claim end position is not passable";

	actor->WalkTo(drawn.End(), 0, 0);
	ASSERT_FALSE(actor->GetPath().Empty()) << "an end it cannot stand on still has reachable neighbours";

	const int frames = WalkUntilStopped(live, actor);
	EXPECT_LT(frames, 200) << "the walk has to finish inside the frame budget";
	EXPECT_GT(frames, 1);

	// it stopped short of where it was sent
	EXPECT_NE(actor->Pos, drawn.End()) << "the destination had to be pulled back off the wall";
	EXPECT_LT(Distance(actor->Pos, drawn.End()), Distance(drawn.ActorPosOf(0), drawn.End()))
		<< "stopping short still means walking towards the end, not giving up at the start";
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

// An actor aimed at a destination behind a wall face has to stop, without standing in the wall.
// The path is built by hand and handed to the actor, not coming from: FindPath() would never return
// a leg into a wall, so only an injected path can put DoStep()'s wall guard on the spot.
// The actor walks the open floor, reaches the face and drops the path there.
TEST_F(MovementTest, StopsAgainstAWallFaceRatherThanWalkingIntoIt)
{
	const TestGameMap live {
		"###############",
		"#.............#",
		"#....##########",
		"#1...####...E.#",
		"#....##########",
		"#.............#",
		"###############"
	};
	const TestSearchMap& drawn = live.Drawing();
	Actor* actor = live.ActorOf(0);
	ASSERT_NE(actor, nullptr);

	const Point start = actor->Pos;
	const Point goal = drawn.End();
	ASSERT_FALSE(PathFinder::IsWalkableTo(drawn.Props(), start, goal, true, actor->circleSize));

	Path path;
	path.AppendStep({ goal, GetOrient(start, goal), false });
	FindPathRequest request;
	request.requestType = FindPathRequestType::WalkTo;
	request.destination = goal;
	actor->OnPathCalculated(std::move(path), request);

	ASSERT_FALSE(actor->GetPath().Empty()) << "the injected path should be what the actor walks";
	ASSERT_TRUE(actor->InMove());

	const int frames = WalkUntilStopped(live, actor);
	EXPECT_LT(frames, 200) << "the walk has to end inside the frame budget";
	EXPECT_FALSE(actor->InMove()) << "it has to give up rather than keep pushing";
	EXPECT_GT(actor->Pos.x, start.x) << "stopping at the start would mean it never tried";

	// it stopped on the floor, facing the wall face it refused to enter
	const SearchmapPoint under { actor->Pos };
	EXPECT_FALSE(bool(PathFinder::GetBlockedTile(drawn.Props(), under) & PathMapFlags::SIDEWALL))
		<< "stopped at (" << actor->Pos.x << ',' << actor->Pos.y << "), which is inside a wall";
	EXPECT_TRUE(bool(PathFinder::GetBlockedTile(drawn.Props(), SearchmapPoint(under.x + 1, under.y)) & PathMapFlags::SIDEWALL))
		<< "it should have walked up to the wall face, not stopped short of it at ("
		<< actor->Pos.x << ',' << actor->Pos.y << ')';
}

// BG1's AR0146: open floor one tile from a staircase of wall faces. The route from '2' to E is a
// 1:1 diagonal parallel to it, so every step threads the point four tiles share with a wall face
// on one side. Swept over every pixel of the starting tile: crossing such a corner puts the actor
// on a tile boundary, and the wall probe used to round onto the wall-face side and abandon the
// walk.
TEST_F(MovementTest, WalksADiagonalThatHugsAStaircaseOfWallFaces)
{
	const TestGameMap live {
		"######XX######X............",
		"#####XX######...........###",
		"####XX######...........##.#",
		"###XX######.........E.##...",
		"##XX######...........##....",
		"#XX######...........##.....",
		"XX######...........##......",
		"X######...........##.......",
		"######...........##........",
		"#####...........##.........",
		"####..........###..........",
		"##X..........##.........#..",
		"#X..........##..........##.",
		"X..........##............##",
		"XX........##..............#",
		"X........##................",
		"X.......##.................",
		".......##........###.......",
		"......##........#XX##......",
		"....2##........##XXX##.....",
		"....##........###X#X###....",
		"....#........####XXX####...",
		"............####XX#XX####..",
		"...........###XXX###XX####.",
		"..........##XXX######XX####"
	};
	const TestSearchMap& drawn = live.Drawing();
	Actor* actor = live.ActorOf(0);
	ASSERT_NE(actor, nullptr);

	const Point home = actor->Pos;
	const Point goal = drawn.End();

	std::vector<std::string> failures;
	// check starting from every possible navmap point from a starting tile
	for (int oy = -6; oy < 6; ++oy) {
		for (int ox = -8; ox < 8; ++ox) {
			const Point start(home.x + ox, home.y + oy);
			live.GetMap()->ClearSearchMapFor(actor);
			actor->ClearPath(true);
			actor->SetPos(start);

			actor->WalkTo(goal, 0, 0);
			if (actor->GetPath().Empty()) {
				failures.push_back(fmt::format("({},{}): no path at all", start.x, start.y));
				continue;
			}
			const size_t legs = actor->GetPath().Size();
			const Point last = actor->GetPath().GetStep(legs - 1).point;
			int f = 0;
			for (; f < 400 && actor->InMove(); ++f) {
				TestGameLoop::RunFrame();
			}
			if (actor->Pos != goal) {
				failures.push_back(fmt::format("({},{}): stopped at ({},{}) after {} frames; path had {} legs ending ({},{}), goal ({},{})",
							       start.x, start.y, actor->Pos.x, actor->Pos.y,
							       f, legs, last.x, last.y, goal.x, goal.y));
			}
		}
	}

	EXPECT_TRUE(failures.empty())
		<< failures.size() << " of 192 starting pixels never reached the goal, first few:\n"
		<< fmt::format("{}", fmt::join(failures.begin(), failures.begin() + std::min<size_t>(failures.size(), 8), "\n"));
}
}

#endif
