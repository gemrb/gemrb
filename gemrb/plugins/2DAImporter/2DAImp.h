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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/2DAImporter/2DAImp.h,v 1.18 2004/11/13 15:32:27 avenger_teambg Exp $
 *
 */

#ifndef P2DAIMP_H
#define P2DAIMP_H

#include "../Core/TableMgr.h"
#include "../../includes/globals.h"

typedef std::vector< char*> RowEntry;

class p2DAImp : public TableMgr {
private:
	DataStream* str;
	bool autoFree;
	std::vector< char*> colNames;
	std::vector< char*> rowNames;
	std::vector< char*> ptrs;
	std::vector< RowEntry> rows;
	char defVal[32];
public:
	p2DAImp(void);
	~p2DAImp(void);
	bool Open(DataStream* stream, bool autoFree = true);
	/** Returns the actual number of Rows in the Table */
	inline int GetRowCount()
	{
		return ( int ) rows.size();
	}
	/** Returns the actual number of Columns in the Table */
	inline int GetColumnCount(unsigned int row = 0)
	{
		if (rows.size() <= row) {
			return 0;
		}
		return ( int ) rows[row].size();
	}
	/** Returns a pointer to a zero terminated 2da element,
		if it cannot return a value, it returns the default */
	inline char* QueryField(unsigned int row = 0, unsigned int column = 0) const
	{
		if (rows.size() <= row) {
			return ( char * ) defVal;
		}
		if (rows[row].size() <= column) {
			return ( char * ) defVal;
		}
		return rows[row][column];
	};
	/** Returns a pointer to a zero terminated 2da element,
		 uses column name and row name to search the field,
	  may return NULL */
	inline char* QueryField(const char* row, const char* column) const
	{
    unsigned int i;

		int rowi = -1, coli = -1;
		for (i = 0; i < rowNames.size(); i++) {
			if (stricmp( rowNames[i], row ) == 0) {
				rowi = i;
				break;
			}
		}
		if (rowi == -1) {
			return ( char * ) defVal;
		}
		for (i = 0; i < colNames.size(); i++) {
			if (stricmp( colNames[i], column ) == 0) {
				coli = i;
				break;
			}
		}
		if (coli == -1) {
			return ( char * ) defVal;
		}
		if (rows[rowi].size() <= ( unsigned int ) coli) {
			return ( char * ) defVal;
		}
		return rows[rowi][coli];
	};

	inline int GetRowIndex(const char* string) const
	{
		for (unsigned int index = 0; index < rowNames.size(); index++) {
			if (stricmp( rowNames[index], string ) == 0) {
				return index;
			}
		}
		return -1;
	};

	inline char* GetRowName(unsigned int index) const
	{
		if (index < rowNames.size()) {
			return rowNames[index];
		}
		return NULL;
	};
public:
	void release(void)
	{
		delete this;
	}
};

#endif
