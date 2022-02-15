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
	char defVal[32];
public:
	p2DAImporter(void);
	p2DAImporter(const p2DAImporter&) = delete;
	~p2DAImporter() override;
	p2DAImporter& operator=(const p2DAImporter&) = delete;
	bool Open(DataStream* stream) override;
	/** Returns the actual number of Rows in the Table */
	inline ieDword GetRowCount() const override
	{
		return ( ieDword ) rows.size();
	}

	inline ieDword GetColNamesCount() const override
	{
		return (ieDword) colNames.size();
	}

	/** Returns the actual number of Columns in the Table */
	inline ieDword GetColumnCount(unsigned int row = 0) const override
	{
		if (rows.size() <= row) {
			return 0;
		}
		return ( ieDword ) rows[row].size();
	}
	/** Returns a pointer to a zero terminated 2da element,
		if it cannot return a value, it returns the default */
	inline const char* QueryField(size_t row = 0, size_t column = 0) const override
	{
		if (rows.size() <= row) {
			return (const char *) defVal;
		}
		if (rows[row].size() <= column) {
			return (const char *) defVal;
		}
		if (rows[row][column][0]=='*' && !rows[row][column][1]) {
			return (const char *) defVal;
		}
		return rows[row][column];
	}

	const char* QueryDefault() const override
	{
		return defVal;
	}

	inline int GetRowIndex(const char* string) const override
	{
		for (unsigned int index = 0; index < rowNames.size(); index++) {
			if (stricmp( rowNames[index], string ) == 0) {
				return int(index);
			}
		}
		return -1;
	}

	inline int GetColumnIndex(const char* string) const override
	{
		for (unsigned int index = 0; index < colNames.size(); index++) {
			if (stricmp( colNames[index], string ) == 0) {
				return int(index);
			}
		}
		return -1;
	}

	inline const char* GetColumnName(unsigned int index) const override
	{
		if (index < colNames.size()) {
			return colNames[index];
		}
		return "";
	}

	inline const char* GetRowName(unsigned int index) const override
	{
		if (index < rowNames.size()) {
			return rowNames[index];
		}
		return "";
	}

	inline int FindTableValue(unsigned int col, long val, int start) const override
	{
		ieDword max = GetRowCount();
		for (ieDword row = start; row < max; row++) {
			const char* ret = QueryField( row, col );
			long Value;
			if (valid_signednumber(ret, Value) && (Value == val))
				return int(row);
		}
		return -1;
	}

	inline int FindTableValue(unsigned int col, const char* val, int start) const override
	{
		ieDword max = GetRowCount();
		for (ieDword row = start; row < max; row++) {
			const char* ret = QueryField( row, col );
			if (stricmp(ret, val) == 0)
				return int(row);
		}
		return -1;
	}

	inline int FindTableValue(const char* col, long val, int start) const override
	{
		ieDword coli = GetColumnIndex(col);
		return FindTableValue(coli, val, start);
	}

	inline int FindTableValue(const char* col, const char* val, int start) const override
	{
		ieDword coli = GetColumnIndex(col);
		return FindTableValue(coli, val, start);
	}
};

}

#endif
