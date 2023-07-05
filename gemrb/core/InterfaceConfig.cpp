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

#include "globals.h"

#include "InterfaceConfig.h"
#include "Logging/Logging.h"

#include "Streams/FileStream.h"
#include "System/VFS.h"

namespace GemRB {

void InterfaceConfig::SetKeyValuePair(const key_t& key, const value_t& value)
{
	configVars[key] = value;
}

const InterfaceConfig::value_t* InterfaceConfig::GetValueForKey(const key_t& key) const
{
	const auto result = configVars.find(key);

	return result != configVars.cend() ? &result->second : nullptr;
}

CFGConfig::CFGConfig(int argc, char *argv[])
	: InterfaceConfig()
{
	FileStream* config = new FileStream();
	// skip arg0 (it is just gemrb)
	for (int i=1; i < argc; i++) {
		if (stricmp(argv[i], "-c") == 0) {
			const char* filename = argv[++i];

			if (!config->Open(filename)) {
				// Explicitly specified cfg file HAS to be present
				Log(FATAL, "Config", "Failed to open config file \"{}\".", filename);
			}
			isValid = InitWithINIData(config);
		} else if (stricmp(argv[i], "-q") == 0) {
			// quiet mode
			SetKeyValuePair("AudioDriver", "none");
		} else {
			// assume a path was passed, soft force configless startup
			SetKeyValuePair("GamePath", argv[i]);
			isValid = true;
		}
	}
	if (!isValid) {
		// nothing passed in on CLI, so search for gemrb.cfg
		char datadir[_MAX_PATH];
		char path[_MAX_PATH];
		char name[_MAX_PATH];

		// Find basename of this program. It does the same as basename (3),
		// but that's probably missing on some archs
		const char* appName = strrchr(argv[0], PathDelimiter);
		if (appName) {
			appName++;
		} else {
			appName = argv[0];
		}

		strlcpy(name, appName, _MAX_PATH);
		assert(name[0]);

#if TARGET_OS_MAC
		// CopyGemDataPath would give us bundle resources dir
		CopyHomePath(datadir, _MAX_PATH);
		PathAppend(datadir, PACKAGE);
#else
		CopyGemDataPath(datadir, _MAX_PATH);
#endif
		PathJoinExt( path, datadir, name, "cfg" );

#define ATTEMPT_INIT \
if (config->Open(path) \
	&& InitWithINIData(config)) { \
		goto done; \
	}

		ATTEMPT_INIT;

#ifdef SYSCONF_DIR
		PathJoinExt( path, SYSCONF_DIR, name, "cfg" );
		ATTEMPT_INIT
#endif

#ifndef ANDROID
		// Now try ~/.gemrb folder
		CopyHomePath(datadir, _MAX_PATH);
		char confpath[_MAX_PATH] = ".";
		strcat(confpath, name);
		PathJoin(datadir, datadir, confpath, nullptr);
		PathJoinExt( path, datadir, name, "cfg" );
		ATTEMPT_INIT;
#endif
		// Don't try with default binary name if we have tried it already
		if (strcmp( name, PACKAGE ) != 0) {
			PathJoinExt( path, datadir, PACKAGE, "cfg" );

			ATTEMPT_INIT;

#ifdef SYSCONF_DIR
			PathJoinExt( path, SYSCONF_DIR, PACKAGE, "cfg" );
			ATTEMPT_INIT;
#endif
		}
		// if all else has failed try current directory
		PathJoinExt(path, "./", PACKAGE, "cfg");
		ATTEMPT_INIT;
	}
#undef ATTEMPT_INIT
done:
	delete config;
}

bool CFGConfig::InitWithINIData(DataStream* cfgStream)
{
	if (cfgStream == NULL) {
		return false;
	}

	if (isValid) {
		Log(WARNING, "Config", "attempting to replace config values with contents of {}", cfgStream->filename);
	} else {
		Log(MESSAGE, "Config", "attempting to initialize config with {} found at:", cfgStream->filename);
	}
	Log(MESSAGE, "Config", "{}", cfgStream->originalfile);

	isValid = false;
	int lineno = 0;
	std::string line;
	while (cfgStream->ReadLine(line) != DataStream::Error) {
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

		SetKeyValuePair(key, val);
	}
	isValid = true;
	return true;
}

}

