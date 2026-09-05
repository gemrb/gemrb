// SPDX-FileCopyrightText: 2026 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

// A headless Interface with the demo game loaded, for tests that need live engine state.

#ifndef TESTS_LIVEGAMEFIXTURE_H
#define TESTS_LIVEGAMEFIXTURE_H

#include "../../core/GameData.h"
#include "../../core/Interface.h"
#include "../../core/InterfaceConfig.h"
#include "../../core/Logging/Logging.h"
#include "../../core/PathFinderScheduler.h"
#include "../../core/PluginMgr.h"
#include "../../core/SaveGameMgr.h"

#include <clocale>
#include <gtest/gtest.h>
#include <memory>

namespace GemRB {

class LiveGameTest : public testing::Test {
public:
	/**
	 * The engine, as a function-local static: a static data member would need an out-of-line
	 * definition, which a header shared by several test files cannot have before C++17.
	 */
	static std::unique_ptr<Interface>& Engine()
	{
		static std::unique_ptr<Interface> engine;
		return engine;
	}

	static void SetUpTestSuite()
	{
		setlocale(LC_ALL, "");
		const char* argv[] = { "tester", "-c", "../../tester.cfg" };
		auto cfg = LoadFromArgs(3, const_cast<char**>(argv));
		ToggleLogging(true);
		SetMainLogLevel(ERROR);
		Engine() = std::make_unique<Interface>(std::move(cfg));

		auto gamStream = gamedata->GetResourceStream("gem-demo", IE_GAM_CLASS_ID);
		auto gamMgr = GetImporter<SaveGameMgr>(IE_GAM_CLASS_ID, gamStream);
		auto gam = gamMgr->LoadGame(std::make_unique<Game>(), GAMVersion::GemRB);
		core->SetGame(std::move(gam));

		core->StartGameControl();

		// a fresh GameControl comes up with DF_FREEZE_SCRIPTS set, which is the paused state.
		// Map::UpdateScripts() returns immediately on it, so nothing at all runs until the
		// game is unpaused. PF_QUIET keeps it away from displaymsg, PF_FORCED from the
		// cutscene check.
		core->SetPause(PauseState::Off, PF_QUIET | PF_FORCED);

		// Interface::Init() has already started the PathFinderScheduler.
		// Restart it explicitly in immediate mode here.
		// The RequestPath() for most tests should compute synchronously on this
		// thread, and a frame's movement never depends on when a worker happens to finish.
		PathFinderScheduler::Stop();
		PathFinderScheduler::Start(0, "immediate");
	}

	static void TearDownTestSuite()
	{
		core->SetGame(nullptr);
		VideoDriver.reset();
		Engine().reset();
		// Interface destruction does not clear the global observer. Later
		// filesystem tests consult it when deciding whether to resolve case.
		core = nullptr;
		PluginMgr::Get()->RunCleanup();
	}
};


}

#endif
