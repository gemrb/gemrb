/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2013 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef __GemRB__InterfaceConfig__
#define __GemRB__InterfaceConfig__

#include "exports-core.h"

#include "Strings/StringMap.h"
#include "System/VFS.h"

#include <stdexcept>
#include <string>
#include <vector>

// This is changed by both cmake and AppVeyor (CmakeLists.txt and .appveyor.yml)
#define VERSION_GEMRB "0.9.4-git"

#define GEMRB_STRING "GemRB v" VERSION_GEMRB
#define PACKAGE      "GemRB"

//the maximum supported game CD count
#define MAX_CD 6

namespace GemRB {

class CoreInitializationException : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

#define CIE CoreInitializationException

using variables_t = StringMap<int32_t>;

struct CoreSettings {
	path_t GamePath = ".";
	path_t GameDataPath = "data";
	path_t GameOverridePath = "override";
	path_t GameSoundsPath = "sounds";
	path_t GameScriptsPath = "scripts";
	path_t GamePortraitsPath = "portraits";
	path_t GameCharactersPath = "characters";
	path_t GameLanguagePath = "lang/en_US";
	path_t GameMoviesPath = "movies";
	path_t SavePath;
	path_t CachePath = "./Cache2";
	std::vector<path_t> CD[MAX_CD];
	std::vector<path_t> ModPath;
	path_t CustomFontPath = "/usr/share/fonts/TTF";

	path_t GemRBPath = GemDataPath();
	path_t GemRBOverridePath;
	path_t GemRBUnhardcodedPath;
	path_t PluginsPath;
	path_t GUIScriptsPath;
#ifdef WIN32 // TODO: we should make this a build time option
	bool CaseSensitive = false; // this is just the default value, so CD1/CD2 will be resolved
#else
	bool CaseSensitive = true;
#endif

	std::string GameName = GEMRB_STRING;
	std::string GameType = "auto";
	std::string Encoding = "default";

	int GamepadPointerSpeed = 10;
	bool UseSoftKeyboard = false; // TODO: reevaluate the need for this, see comments in StartTextInput
	unsigned short NumFingScroll = 2;
	unsigned short NumFingKboard = 3;
	unsigned short NumFingInfo = 2;
	int MouseFeedback = 0;
	int8_t EdgeScrollOffset = 5;

	int Width = 640;
	int Height = 480;
	int Bpp = 32;
	bool DrawFPS = false;
	int CapFPS = 0;
	bool FullScreen = false;
	bool SpriteFoW = false;
	uint32_t debugMode = 0;
	bool Logging = true;
	int LogColor = -1; // -1 is to automatically determine
	bool CheatFlag = true; /** Cheats enabled? */
	int MaxPartySize = 6;
	int GUIEnhancements = 23;

	bool KeepCache = false;
	bool MultipleQuickSaves = false;
	bool UseAsLibrary = false;
	// once GemRB own format is working well, this might be set to 0
	int SaveAsOriginal = 1; // if true, saves files in compatible mode
	std::string VideoDriverName = "sdl"; // consider deprecating? It's now a hidden option
	std::string AudioDriverName = "openal";
	std::string SkipPlugin;
	std::string DelayPlugin;

	int DoubleClickDelay = 250;
	uint32_t DebugFlags = 0;
	uint32_t ActionRepeatDelay = 250;
	int TouchInput = -1;
	std::string SystemEncoding = "UTF-8";
	std::string ScaleQuality = "best";

	variables_t vars;
};

using InterfaceConfig = StringMap<std::string>;

GEM_EXPORT CoreSettings LoadFromArgs(int argc, char* argv[]);
GEM_EXPORT CoreSettings LoadFromDictionary(InterfaceConfig);
GEM_EXPORT InterfaceConfig LoadFromCFG(const path_t& file);
GEM_EXPORT void SanityCheck(const char* ver = VERSION_GEMRB);

}

#endif /* defined(__GemRB__InterfaceConfig__) */
