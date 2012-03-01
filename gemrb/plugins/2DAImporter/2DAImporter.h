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

typedef std::vector< char*> RowEntry;

class p2DAImporter : public TableMgr {
private:
	DataStream* str;
	std::vector< char*> colNames;
	std::vector< char*> rowNames;
	std::vector< char*> ptrs;
	std::vector< RowEntry> rows;
	char defVal[32];
public:
	p2DAImporter(void);
	~p2DAImporter(void);
	bool Open(DataStream* stream);
	/** Returns the actual number of Rows in the Table */
	inline ieDword GetRowCount() const
	{
		return ( ieDword ) rows.size();
	}

	inline ieDword GetColNamesCount() const
	{
		return (ieDword) colNames.size();
	}

	/** Returns the actual number of Columns in the Table */
	inline ieDword GetColumnCount(unsigned int row = 0) const
	{
		if (rows.size() <= row) {
			return 0;
		}
		return ( ieDword ) rows[row].size();
	}
	/** Returns a pointer to a zero terminated 2da element,
		if it cannot return a value, it returns the default */
	inline const char* QueryField(unsigned int row = 0, unsigned int column = 0) const
	{
		if (rows.size() <= row) {
			return ( char * ) defVal;
		}
		if (rows[row].size() <= column) {
			return ( char * ) defVal;
		}
		if (rows[row][column][0]=='*' && !rows[row][column][1]) {
			return ( char * ) defVal;
		}
		return rows[row][column];
	}
	/** Returns a pointer to a zero terminated 2da element,
		 uses column name and row name to search the field */
	inline const char* QueryField(const char* row, const char* column) const
	{
		int rowi, coli;

		rowi = GetRowIndex(row);

		if (rowi < 0) {
			return ( char * ) defVal;
		}

		coli = GetColumnIndex(column);
		 
		if (coli < 0) {
			return ( char * ) defVal;
		}

		return QueryField((unsigned int) rowi, (unsigned int) coli);
	}

	virtual const char* QueryDefault() const
	{
		return defVal;
	}

	inline int GetRowIndex(const char* string) const
	{
		for (unsigned int index = 0; index < rowNames.size(); index++) {
			if (stricmp( rowNames[index], string ) == 0) {
				return (int) index;
			}
		}
		return -1;
	}

	inline int GetColumnIndex(const char* string) const
	{
		for (unsigned int index = 0; index < colNames.size(); index++) {
			if (stricmp( colNames[index], string ) == 0) {
				return (int) index;
			}
		}
		return -1;
	}

	inline const char* GetColumnName(unsigned int index) const
	{
		if (index < colNames.size()) {
			return colNames[index];
		}
		return "";
	}

	inline const char* GetRowName(unsigned int index) const
	{
		if (index < rowNames.size()) {
			return rowNames[index];
		}
		return "";
	}

	inline int FindTableValue(unsigned int col, long val, int start) const
	{
		ieDword row, max;
		
		max = GetRowCount();
		for (row = start; row < max; row++) {
			const char* ret = QueryField( row, col );
			long Value;
			if (valid_number( ret, Value ) && (Value == val) )
				return (int) row;
		}
		return -1;
	}

	inline int FindTableValue(unsigned int col, const char* val, int start) const
	{
		ieDword row, max;

		max = GetRowCount();
		for (row = start; row < max; row++) {
			const char* ret = QueryField( row, col );
			if (stricmp(ret, val) == 0)
				return (int) row;
		}
		return -1;
	}
};

}

#endif
