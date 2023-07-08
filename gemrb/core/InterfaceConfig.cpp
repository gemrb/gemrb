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

#include <stdexcept>

namespace GemRB {

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

		settings[key] = std::move(val);
	}
	return settings;
}

static InterfaceConfig LoadDefaultCFG(const char* appName)
{
	// nothing passed in on CLI, so search for gemrb.cfg
	path_t datadir;
	char path[_MAX_PATH];
	path_t name = appName;

#if TARGET_OS_MAC
	// GemDataPath would give us bundle resources dir
	datadir = HomePath();
	PathAppend(datadir, PACKAGE);
#else
	datadir = GemDataPath();
#endif
	PathJoinExt(path, datadir.c_str(), name.c_str(), "cfg");
	
	FileStream cfgStream;
	if (cfgStream.Open(path)) {
		return LoadFromStream(cfgStream);
	}

#ifdef SYSCONF_DIR
	PathJoinExt( path, SYSCONF_DIR, name.c_str(), "cfg" );
	if (cfgStream.Open(path))
	{
		return LoadFromStream(cfgStream);
	}
#endif

#ifndef ANDROID
	// Now try ~/.gemrb folder
	datadir = HomePath();
	char confpath[_MAX_PATH] = ".";
	strcat(confpath, name.c_str());
	char tmp[_MAX_PATH];
	strlcpy(tmp, datadir.c_str(), _MAX_PATH);
	PathJoin(tmp, datadir.c_str(), confpath, nullptr);
	datadir = tmp;
	PathJoinExt(path, datadir.c_str(), name.c_str(), "cfg");
	
	if (cfgStream.Open(path))
	{
		return LoadFromStream(cfgStream);
	}
#endif
	// Don't try with default binary name if we have tried it already
	if (name != PACKAGE) {
		PathJoinExt(path, datadir.c_str(), PACKAGE, "cfg");

		if (cfgStream.Open(path))
		{
			return LoadFromStream(cfgStream);
		}

#ifdef SYSCONF_DIR
		PathJoinExt(path, SYSCONF_DIR, PACKAGE, "cfg");
		
		if (cfgStream.Open(path))
		{
			return LoadFromStream(cfgStream);
		}
#endif
	}
	// if all else has failed try current directory
	PathJoinExt(path, "./", PACKAGE, "cfg");
	
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
		if (cfg.count(key)) {
			field = atoi(cfg[key].c_str());
			cfg.erase(key);
		}
	};

	CONFIG_INT("Bpp", config.Bpp);
	CONFIG_INT("CaseSensitive", config.CaseSensitive);
	CONFIG_INT("DoubleClickDelay", config.DoubleClickDelay);
	CONFIG_INT("DrawFPS", config.DrawFPS);
	CONFIG_INT("CapFPS", config.CapFPS);
	CONFIG_INT("EnableCheatKeys", config.CheatFlag);
	CONFIG_INT("GCDebug", config.DebugFlags);
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
	
	auto CONFIG_STRING = [&cfg](const std::string& key, auto& field) {
		if (cfg.count(key)) {
			field = cfg[key];
			cfg.erase(key);
		}
	};
	
	auto CONFIG_PATH = [&CONFIG_STRING](const std::string& key, path_t& field, const path_t& fallback = "") {
		CONFIG_STRING(key, field);
		
		if (field.empty()) {
			field = fallback;
		}
		
		ResolveFilePath(field);
		FixPath(field, true);
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
	FixPath(config.CachePath, false);

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
		assert(strnlen(appDir, _MAX_PATH/2) < _MAX_PATH/2);
		char tmp[_MAX_PATH];
		PathJoin(tmp, appDir, DATA_DIR, nullptr);
		config.GemRBPath = tmp;
	}
#endif

	CONFIG_PATH("GUIScriptsPath", config.GUIScriptsPath, config.GemRBPath);
	CONFIG_PATH("GamePath", config.GamePath);
	// guess a few paths in case this one is bad; two levels deep for the fhs layout
	char testPath[_MAX_PATH];
	bool gameFound = true;
	if (!PathJoin(testPath, config.GamePath.c_str(), "chitin.key", nullptr)) {
		Log(WARNING, "Interface", "Invalid GamePath detected ({}), guessing from the current dir!", testPath);
		if (PathJoin(testPath, "..", "chitin.key", nullptr)) {
			config.GamePath = "..";
		} else {
			if (PathJoin(testPath, "..", "..", "chitin.key", nullptr)) {
				config.GamePath = "../..";
			} else {
				gameFound = false;
			}
		}
	}
	// if nothing was found still, check for the internal demo
	// it's generic, but not likely to work outside of AppImages and maybe apple bundles
	if (!gameFound) {
		Log(WARNING, "Interface", "Invalid GamePath detected, looking for the GemRB demo!");
		if (PathJoin(testPath, "..", "demo", "chitin.key", nullptr)) {
			PathJoin(testPath, "..", "demo", nullptr);
			config.GamePath = testPath;
			config.GameType = "demo";
		} else if (PathJoin(testPath, config.GemRBPath.c_str(), "demo", "chitin.key", nullptr)) {
			PathJoin(testPath, config.GemRBPath.c_str(), "demo", nullptr);
			config.GamePath = testPath;
			config.GameType = "demo";
		}
	}

	CONFIG_PATH("GemRBOverridePath", config.GemRBOverridePath, config.GemRBPath);
	CONFIG_PATH("GemRBUnhardcodedPath", config.GemRBUnhardcodedPath, config.GemRBPath);
#ifdef PLUGIN_DIR
	CONFIG_PATH("PluginsPath", config.PluginsPath, PLUGIN_DIR);
#else
	CONFIG_PATH("PluginsPath", config.PluginsPath, "");
	if (!config.PluginsPath[0]) {
		PathJoin(testPath, config.GemRBPath.c_str(), "plugins", nullptr);
		config.PluginsPath = testPath;
	}
#endif

	CONFIG_PATH("SavePath", config.SavePath, config.GamePath);

	CONFIG_STRING("AudioDriver", config.AudioDriverName);
	CONFIG_STRING("VideoDriver", config.VideoDriverName);
	CONFIG_STRING("Encoding", config.Encoding);
	
	auto value = cfg["ModPath"];
	if (value.length()) {
		config.ModPath = Explode<std::string, std::string>(value, PathListSeparator);
		for (std::string& path : config.ModPath) {
			ResolveFilePath(path);
		}
	}

	for (int i = 0; i < MAX_CD; i++) {
		char keyname[] = { 'C', 'D', char('1'+i), '\0' };
		value = cfg[keyname];
		if (value.length()) {
			config.CD[i] = Explode<std::string, std::string>(value, PathListSeparator);
			for (std::string& path : config.CD[i]) {
				ResolveFilePath(path);
			}
		} else {
			// nothing in config so create our own
			char name[_MAX_PATH];

			PathJoin(name, config.GamePath.c_str(), keyname, nullptr);
			config.CD[i].emplace_back(name);
			PathJoin(name, config.GamePath.c_str(), config.GameDataPath.c_str(), keyname, nullptr);
			config.CD[i].emplace_back(name);
		}
	}
	
	// everything else still remaining in cfg is populated into the variables
	for (const auto& pair : cfg) {
		config.vars[pair.first] = atoi(pair.second.c_str());
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
			auto CFGsettings = LoadFromCFG(argv[++i]);
			// settings passed on the CLI override anything in the file
			settings.insert(CFGsettings.begin(), CFGsettings.end());
			loadedCFG = true;
		} else if (stricmp(argv[i], "-q") == 0) {
			// quiet mode
			settings["AudioDriver"] = "none";
		} else {
			// assume a path was passed, soft force configless startup
			settings["GamePath"] = argv[i];
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
		auto CFGsettings = LoadDefaultCFG(appName);
		// settings passed on the CLI override anything in the file
		settings.insert(CFGsettings.begin(), CFGsettings.end());
	}
	return LoadFromDictionary(std::move(settings));
}

InterfaceConfig LoadFromCFG(const char* file)
{
	FileStream cfgStream;
	if (!cfgStream.Open(file)) {
		throw std::runtime_error(std::string("File not found: ") + file);
	}

	return LoadFromStream(cfgStream);
}

} // namespace GemRB
