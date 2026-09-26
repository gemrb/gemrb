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

	// the live actors reach the cache the pathfinder consults
	live.RefreshTraversability();
	EXPECT_GT(live.StateAt(drawn.ActorPosOf(0)), TraversabilityCache::TraversabilityCellValueEmpty);
	EXPECT_GT(live.StateAt(drawn.ActorPosOf(1)), TraversabilityCache::TraversabilityCellValueEmpty);
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

// === live traversability cache ===

// The live counterpart of TestTraversability: what the real Map's cache holds for the actors the
// frame loop is actually moving. The mock proves how FindPath() reads a hand-built cache; these
// prove the cache the engine builds frame after frame agrees with where its actors really are, and
// that a route asked against it takes them into account.
class TraversabilityLiveTest : public GameMapTest {
protected:
	/**
	 * A path found from the map's own live cache, with an explicit acting identity - the same
	 * phrasing ScheduleFindPath() gives FindPath() for a walk.
	 */
	static Path FindPathOnLive(const TestGameMap& live, const Point& from, const Point& to,
				   const Movable* /*self*/, unsigned int circleSize, int flags)
	{
		ActorPathContext actor;
		actor.circleSize = circleSize;
		return PathFinder::FindPath(live.GetMap()->GetTraversabilityCacheData(), live.Drawing().Props(),
					    from, to, actor, 0, flags);
	}

	static bool PathUsesTile(const Point& from, const Path& path, const SearchmapPoint& tile)
	{
		for (const SearchmapPoint& passed : test::PathTiles(from, path)) {
			if (passed == tile) return true;
		}
		return false;
	}
};

// The cache has to follow a walking actor: the tile it leaves goes back to empty, the tile it
// stands on carries its token and its identity. A cache that only ever added would leave a phantom
// blocker on the start tile and make the actor's own next route impossible.
TEST_F(TraversabilityLiveTest, CacheFollowsTheActorAsItWalks)
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
	live.RefreshTraversability();

	const Point home = actor->Pos;
	const Point goal = drawn.End();
	EXPECT_EQ(live.StateAt(home), TraversabilityCache::TraversabilityCellValueActor);
	EXPECT_EQ(live.StateAt(goal), TraversabilityCache::TraversabilityCellValueEmpty);

	actor->WalkTo(goal, 0, 0);
	ASSERT_TRUE(actor->InMove());
	ASSERT_LT(WalkUntilStopped(live, actor), 200);
	ASSERT_EQ(actor->Pos, goal);

	live.RefreshTraversability();
	EXPECT_EQ(live.StateAt(home), TraversabilityCache::TraversabilityCellValueEmpty)
		<< "the start tile has to go back to empty once the actor has left it";
	EXPECT_EQ(live.StateAt(goal), TraversabilityCache::TraversabilityCellValueActor);
}

// Bumpability is not a constant: an actor in a moving stance cannot be shoved aside, and the cache
// has to re-mark it when that changes, or the pathfinder would route through a walking actor the
// movement then cannot bump.
TEST_F(TraversabilityLiveTest, MovingActorIsMarkedNonTraversable)
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
	live.RefreshTraversability();
	ASSERT_EQ(live.StateAt(actor->Pos), TraversabilityCache::TraversabilityCellValueActor) << "idle first";

	actor->WalkTo(drawn.End(), 0, 0);
	TestGameLoop::RunFrame();
	ASSERT_TRUE(actor->InMove());
	live.RefreshTraversability();
	EXPECT_GE(live.StateAt(actor->Pos), TraversabilityCache::TraversabilityCellValueActorNonTraversable)
		<< "a moving actor is at least as solid as a non-traversable one";

	actor->ClearPath(true);
	TestGameLoop::RunFrame();
	live.RefreshTraversability();
	EXPECT_EQ(live.StateAt(actor->Pos), TraversabilityCache::TraversabilityCellValueActor)
		<< "stopped again, so bumpable again";
}

// The token scheme, live: two bumpable actors on one tile are two tokens, and when one leaves it
// subtracts exactly its own token - the cell is not emptied while the other still stands there,
// and the departing actor's identity is not left behind. A cell holds a single identity, so it
// cannot name both actors at once; the token count is the part that has to be exact.
TEST_F(TraversabilityLiveTest, CacheCountsAndClearsOverlappingActors)
{
	TestGameMap live {
		"#######",
		"#.....#",
		"#######"
	};
	const Point here = SearchmapPoint(3, 1).ToNavmapCenter();
	const Point there = SearchmapPoint(5, 1).ToNavmapCenter();
	Actor* first = live.SpawnActor(1, PathMapFlags::PC, here);
	Actor* second = live.SpawnActor(1, PathMapFlags::NPC, here);
	ASSERT_NE(first, nullptr);
	ASSERT_NE(second, nullptr);

	live.RefreshTraversability();
	ASSERT_EQ(live.StateAt(here), 2) << "two bumpable actors, two tokens";

	// teleport the second one away; the cache has to subtract exactly its token, leaving the other
	second->SetPos(there);
	TestGameLoop::RunFrame();
	live.RefreshTraversability();
	EXPECT_EQ(live.StateAt(here), TraversabilityCache::TraversabilityCellValueActor) << "one token left";
	EXPECT_EQ(live.StateAt(there), TraversabilityCache::TraversabilityCellValueActor);
}

// A beast summoned mid-game is a new actor on a map whose cache already holds the settled state of
// everyone else: the next update has to notice it and stamp its footprint without dropping anyone
// who was already cached. The scene is a summoner already standing, then an actor added after the
// cache has seen him.
TEST_F(TraversabilityLiveTest, LiveCachePicksUpAnActorAddedAfterTheCacheSettled)
{
	TestGameMap live {
		"#########",
		"#1....E.#",
		"#########"
	};
	Actor* resident = live.ActorOf(0);
	ASSERT_NE(resident, nullptr);
	const Point residentPos = resident->Pos;
	const Point summonPos(live.Drawing().End());

	// settle the cache on the resident alone
	live.RefreshTraversability();
	ASSERT_EQ(live.StateAt(residentPos), TraversabilityCache::TraversabilityCellValueActor);
	ASSERT_EQ(live.StateAt(summonPos), TraversabilityCache::TraversabilityCellValueEmpty);

	// the summon appears on the map, after the cache has already been built
	Actor* summoned = live.SpawnActor(1, PathMapFlags::NPC, summonPos);
	ASSERT_NE(summoned, nullptr);
	TestGameLoop::RunFrame();
	live.RefreshTraversability();

	EXPECT_EQ(live.StateAt(summonPos), TraversabilityCache::TraversabilityCellValueActor)
		<< "the newcomer's tile has to become occupied";
	EXPECT_EQ(live.StateAt(residentPos), TraversabilityCache::TraversabilityCellValueActor)
		<< "the actor already cached must not be lost when another is added";
}

// The other half: a summon that leaves the map (dies, is banished, leaves the area) must surrender
// its cells on the next update, without disturbing the actor standing next to it.
TEST_F(TraversabilityLiveTest, LiveCacheForgetsAnActorRemovedFromTheMap)
{
	TestGameMap live {
		"#########",
		"#1E.....#",
		"#########"
	};
	Actor* resident = live.ActorOf(0);
	ASSERT_NE(resident, nullptr);
	const Point residentPos = resident->Pos;
	const Point guestPos(live.Drawing().End());

	Actor* guest = live.SpawnActor(1, PathMapFlags::NPC, guestPos);
	ASSERT_NE(guest, nullptr);
	live.RefreshTraversability();
	ASSERT_EQ(live.StateAt(guestPos), TraversabilityCache::TraversabilityCellValueActor);

	// gone from the map list, but nothing has told the cache yet
	live.GetMap()->RemoveActor(guest);
	TestGameLoop::RunFrame();
	live.RefreshTraversability();

	EXPECT_EQ(live.StateAt(guestPos), TraversabilityCache::TraversabilityCellValueEmpty)
		<< "a removed actor must not leave a phantom blocker behind";
	EXPECT_EQ(live.StateAt(residentPos), TraversabilityCache::TraversabilityCellValueActor)
		<< "removing one actor must not touch its neighbour";
}

// The point of the cache: a blocked tile is a tile a route cannot step on. The loop here is the
// geometry that makes it decidable - the straightener cannot draw a line across the flanking wall,
// so the only way through is around, and the only question is whether the top corridor's blocker
// counts.
TEST_F(TraversabilityLiveTest, LiveCacheRoutesAroundABlockingActor)
{
	TestGameMap live {
		"#############",
		"#S....b....E#",
		"#.#########.#",
		"#...........#",
		"#############"
	};
	const TestSearchMap& drawn = live.Drawing();
	const Point from = drawn.Start();
	const Point to = drawn.End();
	const Point blockerPos(drawn.ActorPosOf(0));
	const SearchmapPoint blockerTile { blockerPos };

	constexpr int actorsBlock = PF_SIGHT | PF_ACTORS_ARE_BLOCKING;
	constexpr int actorsPass = PF_SIGHT;

	Actor* blocker = live.ActorOf(0);
	ASSERT_NE(blocker, nullptr);
	TestGameLoop::RunFrame();
	live.RefreshTraversability();
	ASSERT_GT(live.StateAt(blockerPos), TraversabilityCache::TraversabilityCellValueEmpty);

	// people are solid: the top corridor is plugged, so the route has to take the bottom one
	const Path around = FindPathOnLive(live, from, to, nullptr, 1, actorsBlock);
	ASSERT_FALSE(around.Empty()) << "the bottom corridor is an open detour";
	EXPECT_FALSE(PathUsesTile(from, around, blockerTile)) << "the blocker's tile has to be avoided";

	// bumpable and not blocking: the direct top corridor is available again
	const Path through = FindPathOnLive(live, from, to, nullptr, 1, actorsPass);
	ASSERT_FALSE(through.Empty());
	EXPECT_TRUE(PathUsesTile(from, through, blockerTile))
		<< "a bumpable actor only blocks while actors are blocking";
}

// A moving actor has no bumpable token, so it blocks a route even when the caller is willing to
// bump: routing through a walking actor would mean shoving aside something that will not budge.
// The probe is a second actor's walk down the same one tile corridor: filing that request is also
// what makes the frame's traversability cache update happen.
TEST_F(TraversabilityLiveTest, LiveCacheTreatsAMovingActorAsAlwaysBlocking)
{
	TestGameMap live {
		"#############",
		"#1....2....E#",
		"#############"
	};
	Actor* walker = live.ActorOf(0);
	Actor* blocker = live.ActorOf(1);
	ASSERT_NE(walker, nullptr);
	ASSERT_NE(blocker, nullptr);
	const Point to = live.Drawing().End();

	// the blocker sets off down the corridor and is soon in its moving stance
	blocker->WalkTo(to, 0, 0);
	ASSERT_TRUE(blocker->InMove());
	for (int i = 0; i < 2; ++i) {
		TestGameLoop::RunFrame();
	}
	ASSERT_TRUE(blocker->IsInMovingStance());

	// the walker asks for the same corridor; its request drives the cache update, and the walking
	// blocker has to read as solid, so there is no way through for it either
	walker->WalkTo(to, 0, 0);
	EXPECT_FALSE(walker->InMove()) << "a walking actor blocks the corridor even for a walker that would bump";
}

// An actor's own tile is not an obstacle to its own route; the identity stored with the token is
// what draws that line, and it has to survive the live update.
TEST_F(TraversabilityLiveTest, LiveCacheLetsAnActorIgnoreItsOwnTile)
{
	TestGameMap live {
		"#########",
		"#...2...#",
		"#########"
	};
	const Point from = SearchmapPoint(4, 1).ToNavmapCenter();
	const Point to = SearchmapPoint(7, 1).ToNavmapCenter();
	Actor* actor = live.ActorOf(0);
	ASSERT_NE(actor, nullptr);
	TestGameLoop::RunFrame();
	live.RefreshTraversability();

	constexpr int actorsBlock = PF_SIGHT | PF_ACTORS_ARE_BLOCKING;
	EXPECT_FALSE(FindPathOnLive(live, from, to, actor, 2, actorsBlock).Empty())
		<< "an actor must be able to walk out of its own footprint";
	const Point strangerFrom = SearchmapPoint(1, 1).ToNavmapCenter();
	EXPECT_TRUE(FindPathOnLive(live, strangerFrom, to, nullptr, 2, actorsBlock).Empty())
		<< "the very same footprint has to stop anyone else in a one tile corridor";
}

TEST_F(TraversabilityLiveTest, IsOverConsistentFromEveryPixelOfItsTiles)
{
	const TestGameMap live {
		"XXX",
		".1X",
		"..."
	};
	Actor* pc = live.ActorOf(0);
	ASSERT_NE(pc, nullptr);
	const Point top = pc->SMPos.ToNavmapOrigin();
	const Point center = pc->SMPos.ToNavmapCenter();

	auto checkFromPixels = [&pc](const Point& base, const Point& test, bool negate) {
		for (int oy = 0; oy < SEARCHMAP_TILE_HEIGHT; ++oy) {
			for (int ox = 0; ox < SEARCHMAP_TILE_WIDTH; ++ox) {
				pc->SetPos(base + Point(ox, oy));
				if (negate) {
					EXPECT_FALSE(pc->IsOver(test)) << pc->Pos.x << ", " << pc->Pos.y << ": is over test point";
				} else {
					EXPECT_TRUE(pc->IsOver(test)) << pc->Pos.x << ", " << pc->Pos.y << ": not over test point";
				}
			}
		}
	};

	// are we always over ourselves?
	checkFromPixels(top, center, false);

	// are we over neighbours? Testing 8 external edge points
	Point test;
	for (int oy = 0; oy < 3; ++oy) {
		for (int ox = 0; ox < 3; ++ox) {
			test.x = (ox + 2) * SEARCHMAP_TILE_WIDTH / 2 + 1 * (ox - 1);
			test.y = (oy + 2) * SEARCHMAP_TILE_HEIGHT / 2 + 1 * (oy - 1);
			if (test == center) continue;
			checkFromPixels(top, test, true);
		}
	}
}

// === bumping through a crowd ===

// The crowd both bump tests share: a PC with a half-circle of five NPCs around his northern half,
// the only free ground behind him, and a goal (E) straight across the crowd on the far side.
// Repeating an actor glyph gives one actor per cell, so the 'b's are size-2 NPCs and '2' is a
// size-2 PC. The ring is spaced so a shove has somewhere to put each actor and they can find their
// way home; a tighter ring piles them onto each other's tiles and they never get back.
//
// Size 2 is load bearing. For a circle size below 2, Selectable::IsOverCircle() used to treat every actor
// as a 33x25 pixel box, so Map::GetActor() at a collision point returned whichever adjacent actor
// sat first in the list rather than the one in the way: the walker walks through the blocker and
// the bump is aimed at somebody off to the side. Circle size 2 uses the real ellipse and picks the
// actor in front - the size the game's own characters use.
static std::vector<std::string> HalfCircleAroundPc()
{
	return {
		"#############",
		"#.....E.....#",
		"#...........#",
		"#...........#",
		"#...b.b.b...#",
		"#...b.2.b...#",
		"#...........#",
		"#...........#",
		"#############"
	};
}
static std::vector<std::string> HalfCircleAroundSmallPc()
{
	return {
		"#############",
		"#.....E.....#",
		"#...........#",
		"#...........#",
		"#....aaa....#",
		"#....a1a....#",
		"#...........#",
		"#...........#",
		"#############"
	};
}

class BumpTest : public GameMapTest, public ::testing::WithParamInterface<std::vector<std::string>> {
protected:
	// The PC at the centre, with the goal straight above him and the crowd filling his northern half.
	static constexpr int pcX = 6;
	static constexpr int pcY = 5;

	static Actor* PcOf(const TestGameMap& live)
	{
		for (size_t i = 0; i < live.ActorCount(); ++i) {
			Actor* actor = live.ActorOf(i);
			if (actor && actor->InParty) return actor;
		}
		ADD_FAILURE() << "the drawing has to carry a party member";
		return nullptr;
	}

	// The NPCs around the PC, in reading order, together with where they started.
	static std::vector<Actor*> CrowdOf(const TestGameMap& live, std::vector<Point>& home)
	{
		std::vector<Actor*> crowd;
		for (size_t i = 0; i < live.ActorCount(); ++i) {
			Actor* actor = live.ActorOf(i);
			if (!actor || actor->InParty) continue;
			crowd.push_back(actor);
			home.push_back(actor->Pos);
		}
		return crowd;
	}

	static bool CrowdIsHome(const std::vector<Actor*>& crowd, const std::vector<Point>& home)
	{
		for (size_t i = 0; i < crowd.size(); ++i) {
			if (crowd[i]->Pos != home[i]) return false;
		}
		return true;
	}
};

// Bumpable: the walker shoulders through the half-circle. Every NPC the walker touches has to
// actually leave its tile, and once the walker is past, every one of them has to find its way back
// to where it stood before.
TEST_P(BumpTest, ABumpableCrowdIsPushedAsideAndComesBack)
{
	TestGameMap live { GetParam() };
	Actor* pc = PcOf(live);
	ASSERT_NE(pc, nullptr);
	pc->SetBase(IE_EA, EA_PC);

	std::vector<Point> home;
	std::vector<Actor*> crowd = CrowdOf(live, home);
	ASSERT_EQ(crowd.size(), size_t(5));
	for (Actor* npc : crowd) npc->SetBase(IE_EA, EA_NEUTRAL);

	ASSERT_TRUE(pc->ValidTarget(GA_CAN_BUMP)) << "the walker has to be allowed to bump";
	for (Actor* npc : crowd) {
		ASSERT_TRUE(npc->ValidTarget(GA_ONLY_BUMPABLE)) << "the crowd has to be bumpable for this run";
	}
	// without this the walker would not be walking into anybody at all
	ASSERT_TRUE(bool(live.Drawing().At(pcX, pcY - 1) & PathMapFlags::ACTOR)) << "the crowd has to block the searchmap";

	const Point goal = live.Drawing().End();
	pc->WalkTo(goal, 0, 0);
	ASSERT_TRUE(pc->InMove());

	bool anybodyMoved = false;
	for (int frame = 0; frame < 400 && pc->InMove(); ++frame) {
		TestGameLoop::RunFrame();
		if (!CrowdIsHome(crowd, home)) anybodyMoved = true;
	}
	EXPECT_TRUE(anybodyMoved) << "the walker must actually shove the bumpable crowd aside";
	EXPECT_EQ(pc->Pos, goal) << "the walker has to come out on the far side of the crowd";

	// the shoved actors settle back home once the walker is out of the way
	for (int frame = 0; frame < 400; ++frame) {
		TestGameLoop::RunFrame();
		if (CrowdIsHome(crowd, home)) break;
	}
	for (size_t i = 0; i < crowd.size(); ++i) {
		EXPECT_EQ(crowd[i]->Pos, home[i]) << "NPC " << i << " has to come back to where it started";
	}
}

// Solid: the crowd cannot be moved, so the walker has to find the way around the half-circle. No
// NPC may move for the whole run, and the walker may not be stopped by the wall of bodies.
TEST_P(BumpTest, DISABLED_ASolidCrowdIsWalkedAroundWithoutMoving)
{
	TestGameMap live { GetParam() };
	Actor* pc = PcOf(live);
	ASSERT_NE(pc, nullptr);
	pc->SetBase(IE_EA, EA_PC);

	std::vector<Point> home;
	std::vector<Actor*> crowd = CrowdOf(live, home);
	ASSERT_EQ(crowd.size(), size_t(5));
	for (Actor* npc : crowd) npc->SetBase(IE_EA, EA_EVILCUTOFF);

	ASSERT_TRUE(pc->ValidTarget(GA_CAN_BUMP)) << "this run turns off bumping on the crowd, not the walker";
	for (Actor* npc : crowd) {
		ASSERT_FALSE(npc->ValidTarget(GA_ONLY_BUMPABLE)) << "the crowd has to be solid for this run";
	}
	ASSERT_TRUE(bool(live.Drawing().At(pcX, pcY - 1) & PathMapFlags::ACTOR)) << "the crowd has to block the searchmap";
	// the scenario is only a detour if a path around the crowd exists
	const Point goal = live.Drawing().End();
	live.RefreshTraversability();
	{
		ActorPathContext ctx;
		ctx.circleSize = pc->circleSize;
		const Path around = PathFinder::FindPath(live.GetMap()->GetTraversabilityCacheData(), live.Drawing().Props(),
							 pc->Pos, goal, ctx, 0, PF_SIGHT | PF_ACTORS_ARE_BLOCKING);
		ASSERT_FALSE(around.Empty()) << "the open ground behind the PC has to offer a way around the solid crowd";
	}
	pc->WalkTo(goal, 0, 0);
	ASSERT_TRUE(pc->InMove());

	for (int frame = 0; frame < 400 && pc->InMove(); ++frame) {
		TestGameLoop::RunFrame();
		ASSERT_TRUE(CrowdIsHome(crowd, home)) << "a solid NPC moved on frame " << frame;
	}
	EXPECT_EQ(pc->Pos, goal) << "the walker still has to get around the solid crowd";
	EXPECT_TRUE(CrowdIsHome(crowd, home)) << "no NPC in a solid crowd may move";
}

INSTANTIATE_TEST_SUITE_P(AllAreas, BumpTest, testing::Values(HalfCircleAroundPc(), HalfCircleAroundSmallPc()));
}

#endif
