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
	inline index_t GetRowCount() const override
	{
		return rows.size();
	}

	inline index_t GetColNamesCount() const override
	{
		return colNames.size();
	}

	/** Returns the actual number of Columns in the Table */
	inline index_t GetColumnCount(index_t row = 0) const override
	{
		if (rows.size() <= row) {
			return 0;
		}
		return rows[row].size();
	}
	/** Returns a pointer to a zero terminated 2da element,
		if it cannot return a value, it returns the default */
	inline const char* QueryField(index_t row = 0, index_t column = 0) const override
	{
		if (rows.size() <= row) {
			return defVal.c_str();
		}
		if (rows[row].size() <= column) {
			return defVal.c_str();
		}
		if (rows[row][column][0]=='*' && !rows[row][column][1]) {
			return defVal.c_str();
		}
		return rows[row][column];
	}

	const char* QueryDefault() const override
	{
		return defVal.c_str();
	}

	inline index_t GetRowIndex(const char* string) const override
	{
		for (index_t index = 0; index < rowNames.size(); index++) {
			if (stricmp( rowNames[index], string ) == 0) {
				return index;
			}
		}
		return npos;
	}

	inline index_t GetColumnIndex(const char* string) const override
	{
		for (index_t index = 0; index < colNames.size(); index++) {
			if (stricmp( colNames[index], string ) == 0) {
				return index;
			}
		}
		return npos;
	}

	inline const char* GetColumnName(index_t index) const override
	{
		if (index < colNames.size()) {
			return colNames[index];
		}
		return "";
	}

	inline const char* GetRowName(index_t index) const override
	{
		if (index < rowNames.size()) {
			return rowNames[index];
		}
		return "";
	}

	inline index_t FindTableValue(index_t col, long val, index_t start) const override
	{
		index_t max = GetRowCount();
		for (index_t row = start; row < max; row++) {
			const char* ret = QueryField( row, col );
			long Value;
			if (valid_signednumber(ret, Value) && (Value == val))
				return row;
		}
		return npos;
	}

	inline index_t FindTableValue(index_t col, const char* val, index_t start) const override
	{
		index_t max = GetRowCount();
		for (index_t row = start; row < max; row++) {
			const char* ret = QueryField( row, col );
			if (stricmp(ret, val) == 0)
				return row;
		}
		return npos;
	}

	inline index_t FindTableValue(const char* col, long val, index_t start) const override
	{
		index_t coli = GetColumnIndex(col);
		return FindTableValue(coli, val, start);
	}

	inline index_t FindTableValue(const char* col, const char* val, index_t start) const override
	{
		index_t coli = GetColumnIndex(col);
		return FindTableValue(coli, val, start);
	}
};

}

#endif
