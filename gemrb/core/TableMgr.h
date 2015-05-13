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

/**
 * @file TableMgr.h
 * Declares TableMgr class, abstract loader for Table objects (.2DA files)
 * @author The GemRB Project
 */


#ifndef TABLEMGR_H
#define TABLEMGR_H

#include "globals.h"

#include "Plugin.h"

namespace GemRB {

/**
 * @class TableMgr
 * Abstract loader for Table objects (.2DA files)
 */

class GEM_EXPORT TableMgr : public Plugin {
public: 
	TableMgr();
	virtual ~TableMgr();
	/** Returns the actual number of Rows in the Table */
	virtual ieDword GetRowCount() const = 0;
	/** Returns the number of Columns in the Table */
	virtual ieDword GetColNamesCount() const = 0;
	/** Returns the actual number of Columns in a row */
	virtual ieDword GetColumnCount(unsigned int row = 0) const = 0;
	/** Returns a pointer to a zero terminated 2da element,
	 * 0,0 returns the default value, it may return NULL */
	virtual const char* QueryField(size_t row = 0, size_t column = 0) const = 0;
	/** Returns a pointer to a zero terminated 2da element,
	 * uses column name and row name to search the field,
	 * may return NULL */
	virtual const char* QueryField(const char* row, const char* column) const = 0;
	/** Returns default value of table. */
	virtual const char* QueryDefault() const = 0;
	virtual int GetColumnIndex(const char* colname) const = 0;
	virtual int GetRowIndex(const char* rowname) const = 0;
	virtual const char* GetColumnName(unsigned int index) const = 0;
	/** Returns a Row Name, returns NULL on error */
	virtual const char* GetRowName(unsigned int index) const = 0;
	virtual int FindTableValue(unsigned int column, long value, int start = 0) const = 0;
	virtual int FindTableValue(unsigned int column, const char* value, int start = 0) const = 0;
	virtual int FindTableValue(const char* column, long value, int start = 0) const = 0;
	virtual int FindTableValue(const char* column, const char* value, int start = 0) const = 0;

	/** Opens a Table File */
	virtual bool Open(DataStream* stream) = 0;
};

/**
 *  Utility class to automatically handle loading a table,
 *  and obtain and free a reference to it.
 */
class GEM_EXPORT AutoTable
{
public:
	AutoTable();
	AutoTable(const char* ResRef, bool silent=false);
	~AutoTable();
	AutoTable(const AutoTable &);
	AutoTable& operator=(const AutoTable&);

	bool load(const char* ResRef, bool silent=false);
	void release();
	bool ok() const { return table; }
	operator bool() const { return table; }

	const TableMgr& operator*() const { return *table; }
	const TableMgr* operator->() const { return &*table; }
	const TableMgr* ptr() const { return &*table; }

private:
	Holder<TableMgr> table;
	unsigned int tableref;
};


}

#endif  // ! TABLEMGR_H
