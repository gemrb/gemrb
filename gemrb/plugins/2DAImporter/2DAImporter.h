/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef P2DAIMPORTER_H
#define P2DAIMPORTER_H

#include "TableMgr.h"

#include "globals.h"

#include <cstring>
#include <vector>

namespace GemRB {

using RowEntry = std::vector<char*>;

class p2DAImporter : public TableMgr {
private:
	std::vector< char*> colNames;
	std::vector< char*> rowNames;
	std::vector< char*> ptrs;
	std::vector< RowEntry> rows;
	std::string defVal;
public:
	p2DAImporter(void);
	p2DAImporter(const p2DAImporter&) = delete;
	~p2DAImporter() override;
	p2DAImporter& operator=(const p2DAImporter&) = delete;
	bool Open(DataStream* stream) override;
	/** Returns the actual number of Rows in the Table */
	index_t GetRowCount() const override;

	index_t GetColNamesCount() const override;

	/** Returns the actual number of Columns in the Table */
	index_t GetColumnCount(index_t row = 0) const override;
	/** Returns a pointer to a zero terminated 2da element,
		if it cannot return a value, it returns the default */
	const char* QueryField(index_t row = 0, index_t column = 0) const override;
	const char* QueryDefault() const override;

	index_t GetRowIndex(const char* string) const override;
	index_t GetColumnIndex(const char* string) const override;

	const char* GetColumnName(index_t index) const override;
	const char* GetRowName(index_t index) const override;

	index_t FindTableValue(index_t col, long val, index_t start) const override;
	index_t FindTableValue(index_t col, const char* val, index_t start) const override;
	index_t FindTableValue(const char* col, long val, index_t start) const override;
	index_t FindTableValue(const char* col, const char* val, index_t start) const override;
};

}

#endif
