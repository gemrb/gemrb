// SPDX-FileCopyrightText: 2026 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

// A real Map built from an ASCII drawing, real Actors spawned from its glyphs, and the frame
// loop that drives them. Sits on top of LiveGameFixture.h, which supplies the live engine.

#ifndef TESTS_TESTGAMEMAP_H
#define TESTS_TESTGAMEMAP_H

#include "LiveGameFixture.h"
#include "SearchMapBuilder.h"

#include "../../core/Map.h"
#include "../../core/PathFinder.h"
#include "../../core/Scriptable/Actor.h"
#include "../../core/TileMap.h"
#include "../../includes/ie_stats.h"

#include <gtest/gtest.h>
#include <vector>

namespace GemRB {

using test::TestSearchMap;

namespace TestGameLoop {
	inline std::vector<Map*>& Maps();
}

/**
 * The live counterpart of a TestSearchMap: it builds a real GemRB::Map from ASCII glyph drawing
 * and spawns a real GemRB::Actor for every actor glyph.
 * Indices match Drawing().Actors(), so ActorPosOf(i) and  ActorCircleSizeOf(i) describe the same
 * actor ActorOf(i) returns.
 *
 * It owns the TestSearchMap and shares its TileProps with the map.
 * It is built with ActorPainting::Skip so the searchmap holds only what the engine itself put there.
 *
 * Because the buffer is shared, Drawing().Matches() reads back the *live* searchmap, actor
 * footprints and all.
 */
class TestGameMap {
public:
	TestGameMap(const test::MapRows rows)
		: drawn(rows, test::ActorPainting::Skip), map(MakeMapFor(drawn))
	{
		for (size_t i = 0; i < drawn.Actors().size(); ++i) {
			actors.push_back(Spawn(i));
		}
	}

	const TestSearchMap& Drawing() const noexcept { return drawn; }
	Map* GetMap() const noexcept { return map; }
	size_t ActorCount() const noexcept { return actors.size(); }

	/** The live actor for the nth glyph, in the drawing's reading order. */
	Actor* ActorOf(const size_t index) const
	{
		if (index >= actors.size()) {
			ADD_FAILURE() << "this map has no actor number " << index;
			return nullptr;
		}
		return actors[index];
	}

	/** Token sum on that navmap pixel, as FindPath() reads it. */
	TraversabilityCache::TraversabilityCellState StateAt(const Point& navPoint) const
	{
		return CellAt(navPoint).state;
	}

	/** Who the cache has standing there, for the ignore-myself comparison. */
	const Movable* ActorAt(const Point& navPoint) const
	{
		return CellAt(navPoint).occupyingActor;
	}

	/**
	 * Brings the cache up to date. A frame does this itself, so this is only for a test that
	 * wants to read the cache before running any; it is a no-op once a frame has done it.
	 */
	void RefreshTraversability() const
	{
		map->UpdateTraversabilityCache();
	}

private:
	static Map* MakeMapFor(const TestSearchMap& drawn)
	{
		auto* tileMap = new TileMap();
		tileMap->XCellCount = drawn.Width() / 4;
		tileMap->YCellCount = (drawn.Height() * 12) / 64 + 1;

		Map* map = new Map(tileMap, drawn.Props(), nullptr);
		core->GetGame()->AddMap(map);
		// Game keeps its own map list private, so the frame loop needs its own record
		TestGameLoop::Maps().push_back(map);
		return map;
	}


	static constexpr unsigned int testAnimationIdBase = 0x9000;
	/**
	 * Sizes 1 and 2 are the demo's own creatures. Anything bigger re-points `protagon` at a
	 * test-only row of avatars.2da (those rows have IDs equal to `testAnimationIdBase + circle_size`)
	 */
	Actor* Spawn(const size_t index)
	{
		const TestSearchMap::DrawnActor& glyph = drawn.Actors()[index];
		Actor* actor = gamedata->GetCreature(glyph.circleSize == 1 ? ResRef("rabbit") : ResRef("protagon"));
		if (!actor) {
			ADD_FAILURE() << "the demo has to provide a creature to spawn";
			return nullptr;
		}
		if (glyph.circleSize > 2) {
			actor->SetBase(IE_ANIMATION_ID, testAnimationIdBase + glyph.circleSize);
		}

		actor->InParty = glyph.flag == PathMapFlags::PC ? 1 : 0;

		// activate so that actor will get the RunScripts priority
		actor->Activate();
		actor->Pos = drawn.ActorPosOf(index);
		map->AddActor(actor, true);

		// the ASCII drawing promised a size and a live actor gets its circle size from
		// the game data; ensure they agree
		EXPECT_EQ(actor->circleSize, glyph.circleSize)
			<< "actor " << index << " was drawn at circle size " << glyph.circleSize
			<< " but its creature came up at " << actor->circleSize;
		return actor;
	}

	TraversabilityCache::TraversabilityCellData CellAt(const Point& navPoint) const
	{
		const size_t idx = size_t(navPoint.y) * (drawn.Width() * 16) + navPoint.x;
		return map->GetTraversabilityCacheData()[idx];
	}

	TestSearchMap drawn;
	Map* map = nullptr;
	std::vector<Actor*> actors;
};


/**
 * One frame of the real engine loop.
 *
 * Game::UpdateScripts() is the frame in game, but it also runs things we don't want in the headless
 * tests.
 * Keep only what wee need in tests:
 *
 *   DrainCompletedPathsEarly()  collects what the workers published since the last Sync(), so an
 *                               actor waiting on a path can claim it in this frame rather than
 *                               the next. Must come before DoStep() consumes foundPaths.
 *   Map::UpdateScripts()        the actor queues, per-actor update, and DoStepForActor with its
 *                               bumping and backoff.
 *   Sync(maps)                  hands new requests to the workers and takes back their results.
 */
namespace TestGameLoop {

	/**
	 * Every live map a test has built, in creation order. Sync() wants them all, and Game keeps
	 * its own list private, so TestGameMap records them here as it creates them.
	 */
	inline std::vector<Map*>& Maps()
	{
		static std::vector<Map*> maps;
		return maps;
	}

	inline void RunFrame()
	{
		Game* game = core->GetGame();

		// Game::Ticks is what Game's own Scriptable::Update() would advance, and DoStep()
		// returns on `time <= timeStartStep` without it
		++game->Ticks;

		PathFinderScheduler::DrainCompletedPathsEarly();
		for (Map* map : Maps()) {
			map->UpdateScripts();
		}
		PathFinderScheduler::Sync(Maps());

		game->AdvanceTime(1);
	}

	inline void RunFrames(const int count)
	{
		for (int i = 0; i < count; ++i) {
			RunFrame();
		}
	}

}

/**
 * The fixture for anything using TestGameMap: it clears the frame loop's map list on the way
 * out, so a later suite cannot drive frames over maps whose Game has already been destroyed.
 */
class GameMapTest : public LiveGameTest {
public:
	static void TearDownTestSuite()
	{
		TestGameLoop::Maps().clear();
		LiveGameTest::TearDownTestSuite();
	}
};

}

#endif
