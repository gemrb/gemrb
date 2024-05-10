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

#include "InterfaceConfig.h"

#include "Logging/Logging.h"
#include "Streams/FileStream.h"
#include "System/VFS.h"

namespace GemRB {

void SanityCheck(const char *ver)
{
	if (strcmp(ver, VERSION_GEMRB) != 0) {
		error("Core", "Version check failed: core version {} doesn't match caller's version {}!", VERSION_GEMRB, ver);
	}
}

static InterfaceConfig LoadFromStream(DataStream& cfgStream)
{
	InterfaceConfig settings;
	int lineno = 0;
	std::string line;
	while (cfgStream.ReadLine(line) != DataStream::Error) {
		lineno++;

		// skip leading blanks from name
		auto pos = line.find_first_not_of(WHITESPACE_STRING);

		// ignore empty or comment lines
		if (pos == std::string::npos || line[pos] == '#') {
			continue;
		}
		
		auto parts = Explode<std::string, std::string>(line, '=', 1);
		if (parts.size() < 2) {
			Log(WARNING, "Config", "Invalid line {}: {}", lineno, line);
			continue;
		}
		
		auto& key = parts[0];
		TrimString(key);
		
		auto& val = parts[1];
		TrimString(val);

		settings.Set(key, std::move(val));
	}
	return settings;
}

static InterfaceConfig LoadDefaultCFG(const char* appName)
{
	// nothing passed in on CLI, so search for gemrb.cfg
	path_t datadir;
	path_t name = appName;

#if TARGET_OS_MAC
	// GemDataPath would give us bundle resources dir
	datadir = HomePath();
	PathAppend(datadir, PACKAGE);
#else
	datadir = GemDataPath();
#endif
	path_t path = PathJoinExt(datadir, name, "cfg");
	
	FileStream cfgStream;
	if (cfgStream.Open(path)) {
		return LoadFromStream(cfgStream);
	}

#ifndef ANDROID
#ifndef WIN32
	// Now try XDG_CONFIG_HOME, with the standard fallback of ~/.config/gemrb
	datadir = HomePath();
	path_t confpath;
	const char* home = getenv("XDG_CONFIG_HOME");
	if (home) {
		confpath = PathJoin(home, "gemrb");
	} else {
		confpath = PathJoin(datadir, ".config", "gemrb");
	}
	path = PathJoinExt(confpath, name, "cfg");
	if (cfgStream.Open(path)) {
		return LoadFromStream(cfgStream);
	}
#endif

	// Now try ~/.gemrb folder
	path_t tmp = PathJoin(datadir, ".gemrb");
	path = PathJoinExt(tmp, name, "cfg");
	if (cfgStream.Open(path))
	{
		Log(WARNING, "Interface", "~/.gemrb as a config location is deprecated, please use ~/.config/gemrb!");
		return LoadFromStream(cfgStream);
	}
#endif

#ifdef SYSCONF_DIR
	path = PathJoinExt(SYSCONF_DIR, name, "cfg");
	if (cfgStream.Open(path)) {
		return LoadFromStream(cfgStream);
	}
#endif

	// Don't try with default binary name if we have tried it already
	if (name != PACKAGE) {
		path = PathJoinExt(datadir, PACKAGE, "cfg");

		if (cfgStream.Open(path))
		{
			return LoadFromStream(cfgStream);
		}

#ifdef SYSCONF_DIR
		path = PathJoinExt(SYSCONF_DIR, PACKAGE, "cfg");
		
		if (cfgStream.Open(path))
		{
			return LoadFromStream(cfgStream);
		}
#endif
	}
	// if all else has failed try current directory
	path = PathJoinExt(".", PACKAGE, "cfg");
	
	if (cfgStream.Open(path))
	{
		return LoadFromStream(cfgStream);
	}
	
	return {}; // we don't require a config
}

CoreSettings LoadFromDictionary(InterfaceConfig cfg)
{
	CoreSettings config;
	
	auto CONFIG_INT = [&cfg](const std::string& key, auto& field) {
		using INT = std::remove_reference_t<decltype(field)>;

		if (cfg.Contains(key)) {
			field = INT(atoi(cfg.Get(key)->c_str()));
			cfg.Erase(key);
		}
	};

	CONFIG_INT("Bpp", config.Bpp);
	CONFIG_INT("CaseSensitive", config.CaseSensitive);
	CONFIG_INT("DoubleClickDelay", config.DoubleClickDelay);
	CONFIG_INT("DrawFPS", config.DrawFPS);
	CONFIG_INT("CapFPS", config.CapFPS);
	CONFIG_INT("EnableCheatKeys", config.CheatFlag);
	CONFIG_INT("GCDebug", config.DebugFlags);
	CONFIG_INT("GUIEnhancements", config.GUIEnhancements);
	CONFIG_INT("Height", config.Height);
	CONFIG_INT("KeepCache", config.KeepCache);
	CONFIG_INT("MaxPartySize", config.MaxPartySize);
	config.MaxPartySize = std::min(std::max(1, config.MaxPartySize), 10);
	CONFIG_INT("MouseFeedback", config.MouseFeedback);
	CONFIG_INT("MultipleQuickSaves", config.MultipleQuickSaves);
	CONFIG_INT("RepeatKeyDelay", config.ActionRepeatDelay);
	CONFIG_INT("SaveAsOriginal", config.SaveAsOriginal);
	CONFIG_INT("SpriteFogOfWar", config.SpriteFoW);
	CONFIG_INT("DebugMode", config.debugMode);
	CONFIG_INT("TouchInput", config.TouchInput);
	CONFIG_INT("Width", config.Width);
	CONFIG_INT("UseSoftKeyboard", config.UseSoftKeyboard);
	CONFIG_INT("NumFingScroll", config.NumFingScroll);
	CONFIG_INT("NumFingKboard", config.NumFingKboard);
	CONFIG_INT("NumFingInfo", config.NumFingInfo);
	CONFIG_INT("GamepadPointerSpeed", config.GamepadPointerSpeed);
	CONFIG_INT("Logging", config.Logging);
	CONFIG_INT("LogColor", config.LogColor);

	auto CONFIG_STRING = [&cfg](const std::string& key, auto& field) {
		if (cfg.Contains(key)) {
			field = *cfg.Get(key);
			cfg.Erase(key);
		}
	};
	
	auto CONFIG_PATH = [&CONFIG_STRING](const std::string& key, path_t& field, const path_t& fallback = "") {
		CONFIG_STRING(key, field);
		
		if (field.empty()) {
			field = fallback;
		}
		
		ResolveFilePath(field);
	};

	// TODO: make CustomFontPath default cross platform
	CONFIG_PATH("CustomFontPath", config.CustomFontPath);
	CONFIG_PATH("GameCharactersPath", config.GameCharactersPath);
	CONFIG_PATH("GameDataPath", config.GameDataPath);
	CONFIG_PATH("GameOverridePath", config.GameOverridePath);
	CONFIG_PATH("GamePortraitsPath", config.GamePortraitsPath);
	CONFIG_PATH("GameScriptsPath", config.GameScriptsPath);
	CONFIG_PATH("GameSoundsPath", config.GameSoundsPath);
	CONFIG_PATH("GameLanguagePath", config.GameLanguagePath);
	CONFIG_PATH("GameMoviesPath", config.GameMoviesPath);

	// Path configuration
	CONFIG_PATH("GemRBPath", config.GemRBPath);
	CONFIG_PATH("CachePath", config.CachePath);

	// AppImage doesn't support relative urls at all
	// we set the path to the data dir to cover unhardcoded and co,
	// while plugins are statically linked, so it doesn't matter for them
	// Also, support running from eg. KDevelop AppImages by checking the name.
#if defined(DATA_DIR) && defined(__linux__)
	const char* appDir = getenv("APPDIR");
	const char* appImageFile = getenv("ARGV0");
	if (appDir) {
		Log(DEBUG, "Interface", "Found APPDIR: {}", appDir);
		Log(DEBUG, "Interface", "Found AppImage/ARGV0: {}", appImageFile);
	}
	if (appDir && appImageFile && strcasestr(appImageFile, "gemrb") != nullptr) {
		config.GemRBPath = PathJoin(appDir, DATA_DIR);
	}
#endif

	CONFIG_STRING("GameType", config.GameType);
	CONFIG_PATH("GUIScriptsPath", config.GUIScriptsPath, config.GemRBPath);
	CONFIG_PATH("GamePath", config.GamePath);
	// guess a few paths in case this one is bad; two levels deep for the fhs layout
	bool gameFound = true;
	path_t testPath = PathJoin(config.GamePath, "chitin.key");
	if (!FileExists(testPath)) {
		Log(WARNING, "Interface", "Invalid GamePath detected ({}), guessing from the current dir!", testPath);
		testPath = PathJoin("..", "chitin.key");
		if (FileExists(testPath)) {
			config.GamePath = "..";
		} else {
			testPath = PathJoin("..", "..", "chitin.key");
			if (FileExists(testPath)) {
				config.GamePath = PathJoin("..", "..");
			} else {
				gameFound = false;
			}
		}
	}
	// if nothing was found still, check for the internal demo
	// it's generic, but not likely to work outside of AppImages and maybe apple bundles
	if (!gameFound) {
		Log(WARNING, "Interface", "Invalid GamePath detected, looking for the GemRB demo!");
		testPath = PathJoin("..", "demo", "chitin.key");
		if (FileExists(testPath)) {
			config.GamePath = PathJoin("..", "demo");
			config.GameType = "demo";
		} else {
			testPath = PathJoin(config.GemRBPath, "demo", "chitin.key");
			if (FileExists(testPath)) {
				config.GamePath = PathJoin(config.GemRBPath, "demo");
				config.GameType = "demo";
			}
		}
	}

	CONFIG_PATH("GemRBOverridePath", config.GemRBOverridePath, config.GemRBPath);
	CONFIG_PATH("GemRBUnhardcodedPath", config.GemRBUnhardcodedPath, config.GemRBPath);
#ifdef PLUGIN_DIR
	CONFIG_PATH("PluginsPath", config.PluginsPath, PLUGIN_DIR);
#else
	CONFIG_PATH("PluginsPath", config.PluginsPath, "");
	if (!config.PluginsPath[0]) {
		config.PluginsPath = PathJoin(config.GemRBPath, "plugins");
	}
#endif

	CONFIG_PATH("SavePath", config.SavePath, config.GamePath);

	CONFIG_STRING("AudioDriver", config.AudioDriverName);
	CONFIG_STRING("VideoDriver", config.VideoDriverName);
	CONFIG_STRING("SkipPlugin", config.SkipPlugin);
	CONFIG_STRING("DelayPlugin", config.DelayPlugin);
	CONFIG_STRING("Encoding", config.Encoding);
	CONFIG_STRING("ScaleQuality", config.ScaleQuality);

	auto value = cfg.Get("ModPath", "");
	if (value.length()) {
		config.ModPath = Explode<std::string, std::string>(value, PathListSeparator);
		for (path_t& path : config.ModPath) {
			ResolveFilePath(path);
		}
	}

	for (int i = 0; i < MAX_CD; i++) {
		char keyname[] = { 'C', 'D', char('1'+i), '\0' };
		value = cfg.Get(keyname, "");
		if (value.length()) {
			config.CD[i] = Explode<std::string, std::string>(value, PathListSeparator);
			for (path_t& path : config.CD[i]) {
				ResolveFilePath(path);
			}
		} else {
			// nothing in config so create our own
			path_t name = PathJoin(config.GamePath, keyname);
			config.CD[i].emplace_back(name);
			name = PathJoin(config.GamePath, config.GameDataPath, keyname);
			config.CD[i].emplace_back(name);
		}
	}
	
	// everything else still remaining in cfg is populated into the variables
	for (const auto& pair : cfg) {
		config.vars.Set(pair.first, atoi(pair.second.c_str()));
	}
	
	return config;
}

CoreSettings LoadFromArgs(int argc, char *argv[])
{
	InterfaceConfig settings;
	bool loadedCFG = false;
	// skip arg0 (it is just gemrb)
	for (int i=1; i < argc; i++) {
		if (stricmp(argv[i], "-c") == 0) {
			// settings passed on the CLI override anything in the file
			settings.Merge(LoadFromCFG(argv[++i]));
			loadedCFG = true;
		} else if (stricmp(argv[i], "-q") == 0) {
			// quiet mode
			settings.Set("AudioDriver", "none");
		} else if (stricmp(argv[i], "--color") == 0) {
			if (i < argc - 1) settings.Set("LogColor", argv[++i]);
		} else {
			// assume a path was passed, soft force configless startup
			settings.Set("GamePath", argv[i]);
		}
	}

	if (loadedCFG == false)
	{
		// Find basename of this program. It does the same as basename (3),
		// but that's probably missing on some archs
		const char* appName = strrchr(argv[0], PathDelimiter);
		if (appName) {
			appName++;
		} else {
			appName = argv[0];
		}
		// settings passed on the CLI override anything in the file
		settings.Merge(LoadDefaultCFG(appName));
	}
	return LoadFromDictionary(std::move(settings));
}

InterfaceConfig LoadFromCFG(const path_t& file)
{
	FileStream cfgStream;
	if (!cfgStream.Open(file)) {
		throw CIE(std::string("File not found: ") + file);
	}

	return LoadFromStream(cfgStream);
}

} // namespace GemRB
