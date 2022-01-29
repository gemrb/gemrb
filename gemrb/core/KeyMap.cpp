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

#define KEYLENGTH 64

Function::Function(const char *m, const char *f, int g, int k)
{
	// make sure the module and function names are no longer than 32 characters, or they will be truncated
	strlcpy(moduleName, m, sizeof(moduleName));
	strlcpy(function, f, sizeof(function));
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

bool KeyMap::InitializeKeyMap(const char *inifile, const char *tablefile)
{
	AutoTable kmtable = gamedata->LoadTable(tablefile);

	if (!kmtable) {
		return false;
	}

	char tINIkeymap[_MAX_PATH];
	PathJoin(tINIkeymap, core->config.GamePath, inifile, nullptr);
	FileStream* config = FileStream::OpenFile( tINIkeymap );

	if (config == NULL) {
		Log(WARNING, "KeyMap", "There is no '%s' file...", inifile);
		return false;
	}
	char name[KEYLENGTH+1], value[_MAX_PATH + 3];
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

		name[0] = 0;
		value[0] = 0;

		//ignore possible space after the =, sadly we cannot do the same with
		//spaces before it
		if (sscanf( line, "%[^=]= %[^\r\n]", name, value )!=2)
			continue;

		strnlwrcpy(name,name,KEYLENGTH);
		//remove trailing spaces (bg1 ini file contains them)
		char *nameend = name + strlen( name ) - 1;
		while (nameend >= name && strchr( " \t\r\n", *nameend )) {
			*nameend-- = '\0';
		}

		//change internal spaces to underscore
		for(int c=0;c<KEYLENGTH;c++) if (name[c]==' ') name[c]='_';

		size_t l = strlen(value);
		if (l > 1 || keymap.HasKey(value)) {
			Log(WARNING, "KeyMap", "Ignoring key %s", value);
			continue;
		}

		const char *moduleName;
		const char *function;
		const char *group;

		if (kmtable->GetRowIndex(name)>=0 ) {
			moduleName = kmtable->QueryField(name, "MODULE");
			function = kmtable->QueryField(name, "FUNCTION");
			group = kmtable->QueryField(name, "GROUP");
		} else {
			moduleName = kmtable->QueryField("Default","MODULE");
			function = kmtable->QueryField("Default","FUNCTION");
			group = kmtable->QueryField("Default","GROUP");
			Log(MESSAGE, "KeyMap", "Adding key %s with function %s::%s", value, moduleName, function);
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
	Log(MESSAGE, "KeyMap", "Looking up key: %c(%s) ", key, keystr);

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

	Log(MESSAGE, "KeyMap", "RunFunction(%s::%s)", fun->moduleName.CString(), fun->function.CString());
	core->GetGUIScriptEngine()->RunFunction(fun->moduleName, fun->function);
	return true;
}

Function* KeyMap::LookupFunction(const char* name)
{
	char* key = strdup(name);
	strlwr(key);

	void *tmp;
	if (!keymap.Lookup(name, tmp) ) {
		free(key);
		return NULL;
	}

	free(key);
	return (Function *)tmp;
}

}
