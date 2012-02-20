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

#include "TableMgr.h"

#include "GameData.h"
#include "Interface.h"

namespace GemRB {

TableMgr::TableMgr()
{
}
TableMgr::~TableMgr()
{
}


AutoTable::AutoTable()
{
}

AutoTable::AutoTable(const char* ResRef)
{
	load(ResRef);
}

AutoTable::AutoTable(const AutoTable& other)
{
	*this = other;
}

AutoTable& AutoTable::operator=(const AutoTable& other)
{
	if (other.table) {
		tableref = other.tableref;
		table = gamedata->GetTable(tableref);
	} else {
		table.release();
	}
	return *this;
}

bool AutoTable::load(const char* ResRef, bool silent)
{
	release();

	int ref = gamedata->LoadTable(ResRef, silent);
	if (ref == -1)
		return false;

	tableref = (unsigned int)ref;
	table = gamedata->GetTable(tableref);
	return true;
}

AutoTable::~AutoTable()
{
	release();
}

void AutoTable::release()
{
	if (table) {
		gamedata->DelTable(tableref);
		table.release();
	}
}


}
