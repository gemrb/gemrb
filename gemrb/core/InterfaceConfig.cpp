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
	char datadir[_MAX_PATH];
	char path[_MAX_PATH];
	char name[_MAX_PATH];

	strlcpy(name, appName, _MAX_PATH);
	assert(name[0]);

#if TARGET_OS_MAC
	// CopyGemDataPath would give us bundle resources dir
	CopyHomePath(datadir, _MAX_PATH);
	PathAppend(datadir, PACKAGE);
#else
	CopyGemDataPath(datadir, _MAX_PATH);
#endif
	PathJoinExt(path, datadir, name, "cfg");
	
	FileStream cfgStream;
	if (cfgStream.Open(path)) {
		return LoadFromStream(cfgStream);
	}

#ifdef SYSCONF_DIR
	PathJoinExt( path, SYSCONF_DIR, name, "cfg" );
	if (cfgStream.Open(path))
	{
		return LoadFromStream(cfgStream);
	}
#endif

#ifndef ANDROID
	// Now try ~/.gemrb folder
	CopyHomePath(datadir, _MAX_PATH);
	char confpath[_MAX_PATH] = ".";
	strcat(confpath, name);
	PathJoin(datadir, datadir, confpath, nullptr);
	PathJoinExt( path, datadir, name, "cfg" );
	
	if (cfgStream.Open(path))
	{
		return LoadFromStream(cfgStream);
	}
#endif
	// Don't try with default binary name if we have tried it already
	if (strcmp( name, PACKAGE ) != 0) {
		PathJoinExt(path, datadir, PACKAGE, "cfg");

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

InterfaceConfig LoadFromArgs(int argc, char *argv[])
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
	return settings;
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
