/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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

#ifndef IDSIMPORTER_H
#define IDSIMPORTER_H

#include "SymbolMgr.h"

#include <vector>

namespace GemRB {

struct Pair {
	int val;
	char* str;
};

class IDSImporter : public SymbolMgr {
private:
	std::vector< Pair> pairs;
	std::vector< char*> ptrs;

public:
	IDSImporter(void);
	~IDSImporter(void);
	bool Open(DataStream* stream);
	int GetValue(const char* txt) const;
	char* GetValue(int val) const;
	char* GetStringIndex(unsigned int Index) const;
	int GetValueIndex(unsigned int Index) const;
	int FindString(char *str, int len) const;
	int FindValue(int val) const;
	int GetSize() const { return pairs.size(); }
};

#endif
}


