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
		SpawnDrawnActors();
	}

	/**
	 * Same, from a container instead of a literal: a corpus built up front for a data-driven
	 * walker outlives the full expression that wrote the rows, so it has to own them.
	 */
	explicit TestGameMap(const std::vector<std::string>& rows)
		: drawn(rows, test::ActorPainting::Skip), map(MakeMapFor(drawn))
	{
		SpawnDrawnActors();
	}

	/**
	 * The map borrows this object's terrain, so it must leave the frame loop before the terrain
	 * goes away: a later test's RunFrame() would otherwise update a map whose searchmap has been
	 * freed. Game keeps its own map list private, so only this loop's record can be withdrawn.
	 */
	~TestGameMap()
	{
		auto& maps = TestGameLoop::Maps();
		for (auto it = maps.begin(); it != maps.end();) {
			if (*it == map) {
				it = maps.erase(it);
			} else {
				++it;
			}
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

	/** Token sum on that searchmap tile, as FindPath() reads it. */
	TraversabilityCache::TraversabilityCellState StateAt(const Point& navPoint) const
	{
		return CellAt(navPoint);
	}

	/**
	 * Brings the cache up to date. A frame does this itself, so this is only for a test that
	 * wants to read the cache before running any; it is a no-op once a frame has done it.
	 */
	void RefreshTraversability() const
	{
		map->UpdateTraversabilityCache();
	}

	/**
	 * Spawns one of the demo's creatures at an arbitrary position, for a map whose actor is
	 * placed by the test rather than drawn as a glyph. Sizes 1 and 2 are the demo's own
	 * creatures; bigger sizes re-point the animation at a test-only avatars.2da row.
	 */
	Actor* SpawnActor(uint16_t circleSize, PathMapFlags flag, const Point& pos)
	{
		Actor* actor = gamedata->GetCreature(circleSize == 1 ? ResRef("rabbit") : ResRef("protagon"));
		if (!actor) {
			ADD_FAILURE() << "the demo has to provide a creature to spawn";
			return nullptr;
		}
		if (circleSize > 2) {
			actor->SetBase(IE_ANIMATION_ID, testAnimationIdBase + circleSize);
		}

		actor->InParty = flag == PathMapFlags::PC ? 1 : 0;

		// activate so that actor will get the RunScripts priority
		actor->Activate();
		map->AddActor(actor, true);
		actor->SetPosition(pos, false);

		// the requested size has to agree with what the creature came up as
		EXPECT_EQ(actor->circleSize, circleSize)
			<< "spawned actor at circle size " << circleSize
			<< " but its creature came up at " << actor->circleSize;
		return actor;
	}

private:
	static Map* MakeMapFor(const TestSearchMap& drawn)
	{
		auto* tileMap = new TileMap();
		tileMap->XCellCount = drawn.Width() / 4;
		tileMap->YCellCount = (drawn.Height() * SEARCHMAP_TILE_HEIGHT) / 64 + 1;

		Map* map = new Map(tileMap, drawn.Props(), nullptr);
		core->GetGame()->AddMap(map);
		// Game keeps its own map list private, so the frame loop needs its own record
		TestGameLoop::Maps().push_back(map);
		return map;
	}


	static constexpr unsigned int testAnimationIdBase = 0x9000;

	void SpawnDrawnActors()
	{
		for (size_t i = 0; i < drawn.Actors().size(); ++i) {
			actors.push_back(Spawn(i));
		}
	}

	/**
	 * Sizes 1 and 2 are the demo's own creatures. Anything bigger re-points `protagon` at a
	 * test-only row of avatars.2da (those rows have IDs equal to `testAnimationIdBase + circle_size`)
	 */
	Actor* Spawn(const size_t index)
	{
		const TestSearchMap::DrawnActor& glyph = drawn.Actors()[index];
		return SpawnActor(glyph.circleSize, glyph.flag, drawn.ActorPosOf(index));
	}

	TraversabilityCache::TraversabilityCellState CellAt(const Point& navPoint) const
	{
		const SearchmapPoint tile { navPoint };
		const size_t idx = size_t(tile.y) * drawn.Width() + tile.x;
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

		// Game::AdvanceTime() calls RunFunction("Clock", "UpdateClock") on the hour, which
		// segfaults with no GUI script engine behind the tests. Keep the clock inside the first
		// hour.
		game->GameTime = 1;
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
