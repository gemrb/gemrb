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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef KEYMAP_H
#define KEYMAP_H

#include <unordered_map>

#include "exports.h"
#include "ie_types.h"

namespace GemRB {

class Function {
public:
	ieVariable moduleName;
	ieVariable function;
	int group;
	int key;

	Function(const ieVariable& m, const ieVariable& f, int g, int key);
};

class GEM_EXPORT KeyMap {
private:
	std::unordered_map<std::string, Function> keymap;
public:
	KeyMap();
	KeyMap(const KeyMap&) = delete;
	~KeyMap();
	KeyMap& operator=(const KeyMap&) = delete;
	bool InitializeKeyMap(const char* inifile, const ResRef& keyfile);
	bool ResolveKey(unsigned short key, int group) const;
	bool ResolveName(const char* name, int group) const;

	Function* LookupFunction(std::string name);
};
}

#endif
