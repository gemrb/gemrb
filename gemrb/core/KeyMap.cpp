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

#include "KeyMap.h"
#include "Interface.h"
#include "TableMgr.h"
#include "ScriptEngine.h"
#include "Streams/FileStream.h"

namespace GemRB {

Function::Function(const char *m, const char *f, int g, int k)
{
	// make sure the module and function names are no longer than 32 characters, or they will be truncated
	moduleName = m;
	function = f;
	group = g;
	key = k;
}

KeyMap::KeyMap()
{
	keymap.SetType(GEM_VARIABLES_POINTER);
}

static void ReleaseFunction(void *fun)
{
	delete (Function *) fun;
}

KeyMap::~KeyMap()
{
	keymap.RemoveAll(ReleaseFunction);
}

bool KeyMap::InitializeKeyMap(const char* inifile, const ResRef& tablefile)
{
	AutoTable kmtable = gamedata->LoadTable(tablefile);

	if (!kmtable) {
		return false;
	}

	char tINIkeymap[_MAX_PATH];
	PathJoin(tINIkeymap, core->config.GamePath, inifile, nullptr);
	FileStream* config = FileStream::OpenFile( tINIkeymap );

	if (config == NULL) {
		Log(WARNING, "KeyMap", "There is no '{}' file...", inifile);
		return false;
	}
	
	FixedSizeString<64> name;
	char value[_MAX_PATH + 3];
	while (config->Remains()) {
		char line[_MAX_PATH];

		if (config->ReadLine(line, _MAX_PATH) == -1)
			break;

		if ((line[0] == '#') ||
			( line[0] == '[' ) ||
			( line[0] == '\r' ) ||
			( line[0] == '\n' ) ||
			( line[0] == ';' )) {
			continue;
		}

		value[0] = 0;

		//ignore possible space after the =, sadly we cannot do the same with
		//spaces before it
		if (sscanf( line, "%[^=]= %[^\r\n]", name.begin(), value )!=2)
			continue;

		StringToLower(name);
		name.RTrim();

		//change internal spaces to underscore
		std::replace(name.begin(), name.end(), ' ', '_');

		size_t l = strlen(value);
		if (l > 1 || keymap.HasKey(value)) {
			Log(WARNING, "KeyMap", "Ignoring key {}", value);
			continue;
		}

		const char *moduleName;
		const char *function;
		const char *group;

		if (kmtable->GetRowIndex(name) != TableMgr::npos) {
			moduleName = kmtable->QueryField(name, "MODULE").c_str();
			function = kmtable->QueryField(name, "FUNCTION").c_str();
			group = kmtable->QueryField(name, "GROUP").c_str();
		} else {
			moduleName = kmtable->QueryField("Default","MODULE").c_str();
			function = kmtable->QueryField("Default","FUNCTION").c_str();
			group = kmtable->QueryField("Default","GROUP").c_str();
			Log(MESSAGE, "KeyMap", "Adding key {} with function {}::{}", value, moduleName, function);
		}
		Function *fun = new Function(moduleName, function, atoi(group), tolower(value[0]));

		// lookup by either key or name
		keymap.SetAt(value, fun);
		keymap.SetAt(name, new Function(*fun));
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
	Log(MESSAGE, "KeyMap", "Looking up key: {}({}) ", key, keystr);

	return ResolveName(keystr, group);
}

bool KeyMap::ResolveName(const char* name, int group) const
{
	void *tmp;
	if (!keymap.Lookup(name, tmp) ) {
		return false;
	}

	Function* fun = (Function *)tmp;

	if (fun->group!=group) {
		return false;
	}

	Log(MESSAGE, "KeyMap", "RunFunction({}::{})", fun->moduleName, fun->function);
	core->GetGUIScriptEngine()->RunFunction(fun->moduleName, fun->function);
	return true;
}

Function* KeyMap::LookupFunction(std::string key)
{
	// FIXME: Variables::MyCompareKey is already case insensitive AFICT
	StringToLower(key);

	void *tmp;
	if (!keymap.Lookup(key.c_str(), tmp) ) {
		return nullptr;
	}

	return (Function *)tmp;
}

}
