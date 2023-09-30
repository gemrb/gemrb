/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2009 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include <tuple>
#include <utility>

#include "KeyMap.h"
#include "Interface.h"
#include "Logging/Logging.h"
#include "TableMgr.h"
#include "ScriptEngine.h"
#include "Streams/FileStream.h"

namespace GemRB {

bool KeyMap::InitializeKeyMap(const path_t& inifile, const ResRef& tablefile)
{
	AutoTable kmtable = gamedata->LoadTable(tablefile);

	if (!kmtable) {
		return false;
	}

	path_t tINIkeymap = PathJoin(core->config.GamePath, inifile);
	DataStream* config = FileStream::OpenFile(tINIkeymap);
	if (!config) {
		config = gamedata->GetResourceStream(inifile.substr(0, inifile.size() - 4), IE_INI_CLASS_ID);
	}
	if (!config) {
		Log(WARNING, "KeyMap", "There is no '{}' file...", inifile);
		return false;
	}

	const ieVariable defaultModuleName = kmtable->QueryField("Default", "MODULE");
	const ieVariable defaultFunction = kmtable->QueryField("Default", "FUNCTION");
	int defaultGroup = kmtable->QueryFieldSigned<int>("Default", "GROUP");
	std::string line;
	while (config->ReadLine(line) != DataStream::Error) {
		if (line.length() == 0 ||
			(line[0] == '#') ||
			(line[0] == '[') ||
			(line[0] == '\r') ||
			(line[0] == '\n') ||
			(line[0] == ';')) {
			continue;
		}
		
		StringToLower(line);
		auto parts = Explode<std::string, std::string>(line, '=', 1);
		if (parts.size() < 2) {
			parts.emplace_back();
		}
		
		auto& val = parts[1];
		if (val.length() == 0) continue;
		LTrim(val);

		if (val.length() > 1 || keymap.Get(val) != nullptr) {
			Log(WARNING, "KeyMap", "Ignoring key {}", val);
			continue;
		}
		
		auto& key = parts[0];
		RTrim(key);
		//change internal spaces to underscore
		std::replace(key.begin(), key.end(), ' ', '_');

		Function func;
		func.key = val[0];

		if (kmtable->GetRowIndex(key) != TableMgr::npos) {
			func.moduleName = kmtable->QueryField(key, "MODULE");
			func.function = kmtable->QueryField(key, "FUNCTION");
			func.group = kmtable->QueryFieldSigned<int>(key, "GROUP");
		} else {
			func.moduleName = defaultModuleName;
			func.function = defaultFunction;
			func.group = defaultGroup;
		}

		// lookup by either key or name
		keymap.Set(val, func);
		keymap.Set(key, func);
	}
	delete config;
	return true;
}

//group can be:
//main gamecontrol
bool KeyMap::ResolveKey(unsigned short key, int group) const
{
	// FIXME: key is 2 bytes, but we ignore one. Some non english keyboards won't like this.
	char keystr[2] = {(char)key, 0};
	if (key < 128) {
		Log(MESSAGE, "KeyMap", "Looking up key: {} ({}) ", key, keystr);
	} else {
		// We'd have to convert the codepoint into UTF-8 for non-ASCII numbers
		Log(MESSAGE, "KeyMap", "Looking up key: {}", key);
	}

	return ResolveName(keystr, group);
}

bool KeyMap::ResolveName(const StringView& name, int group) const
{
	auto lookup = keymap.Get(name);
	if (lookup == nullptr) {
		return false;
	}

	if (lookup->group != group) {
		return false;
	}

	Log(MESSAGE, "KeyMap", "RunFunction({}::{})", lookup->moduleName, lookup->function);
	core->GetGUIScriptEngine()->RunFunction(lookup->moduleName.c_str(), lookup->function.c_str());
	return true;
}

const KeyMap::Function* KeyMap::LookupFunction(const StringView& key)
{
	return keymap.Get(key);
}

}
