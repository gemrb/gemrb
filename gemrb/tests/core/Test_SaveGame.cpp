// SPDX-FileCopyrightText: 2026 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

// FIXME: remove once fixed, this is excluding non-linux build bots
#if defined(USE_OPENGL_BACKEND) || (!defined(__APPLE__) && !defined(WIN32))

	#include "../../core/GUI/GameControl.h"
	#include "../../core/Interface.h"
	#include "../../core/InterfaceConfig.h"
	#include "../../core/Logging/Loggers/Stdio.h"
	#include "../../core/Logging/Logging.h"
	#include "../../core/Map.h"
	#include "../../core/PluginMgr.h"
	#include "../../core/SaveGameIterator.h"
	#include "../../core/SaveGameMgr.h"
	#include "../../core/Streams/FileStream.h"

	#include <gtest/gtest.h>

namespace GemRB {

const std::list<path_t> availableGameTypes { "bg1", "bg2", "bgee", "bg2ee", "pst", "iwd", "how", "iwd2" };
const path_t saveINI = PathJoin("tests", "resources", "saves", "saveTesting.ini");

struct SaveGameTest : public testing::TestWithParam<path_t> {
	static Interface* gemrb;
	static std::set<path_t> cfgList;
	static bool inited;

	static void SetUpTestSuite()
	{
		setlocale(LC_ALL, "");

		// set up all the possible per-game configs with GameType, GamePath, SavePath
		// we expect a special ini file with GameType = GamePath pairs
		// reimplement parsing since we can't link to INIImporter
		auto stream = std::make_unique<FileStream>();
		if (!stream->Open(saveINI)) {
			return;
		}

		std::string line;
		while (stream->ReadLine(line) != DataStream::Error) {
			if (line.empty() || line[0] == ';' || line[0] == '[') {
				continue;
			}
			// grab GamePath / GameType pairs from user
			auto parts = Explode<std::string, std::string>(line, '=', 1);
			if (parts.size() < 2) continue;

			// create temporary test config file
			std::string& gameType = parts[0];
			std::string& gamePath = parts[1];
			TrimString(gameType);
			TrimString(gamePath);
			gamePath = ResolveFilePath(gamePath);

			if (!DirExists(gamePath)) continue;

			std::string contents = "GemRBPath = ../../../gemrb/\nPluginsPath = ../plugins\n";
			contents.append("SavePath = ../../../gemrb/tests/resources\n");
			contents.append("UseAsLibrary = 1\nAudioDriver = none\n");
			contents.append(fmt::format("GameType = {}\nGamePath = {}\n", gameType, gamePath));

			path_t cfgPath = PathJoinExt<false>(".", gameType, "cfg"); // ends up in $build/gemrb/core/
			FileStream cfgFile;
			if (!cfgFile.Create(cfgPath)) continue;
			inited = true;

			cfgFile.Write(contents.c_str(), contents.size());
			cfgFile.Close();
			cfgList.insert(cfgPath);
		}

		if (!inited) GTEST_SKIP() << "Skipping test group due to no path being set in " << saveINI;
	}

	static void TearDownTestSuite()
	{
		// cleanup to prevent a delay and crash on exit
		if (core) core->SetGame(nullptr);
		VideoDriver.reset();
		delete gemrb;
	}
};

Interface* SaveGameTest::gemrb = nullptr;
std::set<path_t> SaveGameTest::cfgList;
bool SaveGameTest::inited = false;

TEST_P(SaveGameTest, LoadAndResaveGameTest)
{
	path_t gameType = GetParam();
	auto iter = SaveGameTest::cfgList.find(fmt::format("./{}.cfg", gameType)); // update if cfgPath above changes!
	if (iter == SaveGameTest::cfgList.end()) {
		GTEST_SKIP() << "Skipping test due to unconfigured game type " << gameType << " in " << saveINI;
	}

	const char* argv[] = { "tester", "-c", iter->c_str() };
	auto cfg = LoadFromArgs(3, const_cast<char**>(argv));
	ToggleLogging(true);
	SetMainLogLevel(DEBUG);
	// no need for AddLogWriter(createStdioLogWriter()) — already done by MapTest
	SaveGameTest::gemrb = new Interface(std::move(cfg));

	///////////////////////////
	// LOAD the test save
	String saveName = StringFromUtf8(fmt::format("{}", gameType));
	SetTokenAsString("SaveDir", u"saves");
	auto sgi = core->GetSaveGameIterator().get();
	ASSERT_TRUE(sgi != nullptr);
	auto save = sgi->GetSaveGame(saveName);
	if (save == nullptr) {
		VideoDriver.reset();
		GTEST_SKIP() << gameType << " game is missing a save game in gemrb/core/tests/resources/saves/!";
	}

	// set PlayMode?
	bool iwd2 = core->HasFeature(GFFlags::RULES_3ED);
	core->LoadGame(save, iwd2 ? GAMVersion::IWD2 : GAMVersion::GemRB);
	Game* game = core->GetGame();
	ASSERT_TRUE(game != nullptr);
	Log(DEBUG, "SaveGameTest", "{} game loaded successfully!\n", gameType);

	///////////////////////////
	// SAVE back
	// run the minimum of QF_ENTERGAME code to set up more of the system needed for saving
	game->ConsolidateParty();
	GameControl* gc = core->StartGameControl();
	ASSERT_TRUE(gc != nullptr);
	const Actor* actor = core->GetFirstSelectedPC(true);
	ASSERT_TRUE(actor != nullptr);
	gc->ChangeMap(actor, true);
	// the above call doesn't init the actor positions, so redo it manually
	// NOTE: call MoveBetweenAreasCore if AddActor won't be enough in comparisons
	int i = game->GetPartySize(false);
	while (i--) {
		Actor* pc = game->GetPC(i, false);
		game->GetCurrentArea()->AddActor(pc, true);
	}
	ASSERT_TRUE(actor->GetCurrentArea() != nullptr);

	// try quicksaving (simpler code)
	SetTokenAsString("SaveDir", u"saves"); // something resets it since the last call
	core->config.SavePath = "tests/resources"; // set to build dir destination, so we don't pollute the source
	int ret = sgi->CreateSaveGame(1, false);
	ASSERT_TRUE(ret == GEM_OK);
	Log(DEBUG, "SaveGameTest", "{} game saved successfully!\n", gameType);

	////////////////////////////////////////////////////////
	// compare the original and resaved files for changes
	Log(DEBUG, "SaveGameTest", "Starting checking for differences between the two saved games");
	auto cmd = fmt::format("tests/resources/saves/saveDiffer.sh {} {}", gameType, core->INIConfig);
	ret = system(cmd.c_str());
	EXPECT_EQ(ret, 0);
	if (!ret) Log(DEBUG, "SaveGameTest", "{} game is identical to loaded game!\n", gameType);

	// try reseting the state a bit
	core->SetGame(nullptr);
	VideoDriver.reset();
}

INSTANTIATE_TEST_SUITE_P(
	SaveLoadGameTest,
	SaveGameTest,
	testing::ValuesIn(availableGameTypes));

}
#endif
