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

#include "Strings/StringView.h"
#include "Plugin.h"

namespace GemRB {

class DataStream;

/**
 * @class TableMgr
 * Abstract loader for Table objects (.2DA files)
 */

class GEM_EXPORT TableMgr : public Plugin {
public:
	static constexpr TypeID ID = { "Table" };
	using index_t = uint32_t;
	static constexpr index_t npos = -1;
	using key_t = StringView;

	TableMgr() noexcept = default;
	/** Returns the actual number of Rows in the Table */
	virtual index_t GetRowCount() const = 0;
	/** Returns the number of Columns in the Table */
	virtual index_t GetColNamesCount() const = 0;
	/** Returns the actual number of Columns in a row */
	virtual index_t GetColumnCount(index_t row = 0) const = 0;
	/** Returns a pointer to a zero terminated 2da element,
	 * 0,0 returns the default value, it may return NULL */
	virtual const std::string& QueryField(index_t row, index_t column) const = 0;

	/** Returns a pointer to a zero terminated 2da element,
	 * uses column name and row name to search the field */
	const std::string& QueryField(const key_t& row, const key_t& column) const
	{
		return QueryField(GetRowIndex(row), GetColumnIndex(column));
	}
	
	template <typename RET_T, typename ROW_T, typename COL_T>
	RET_T QueryFieldUnsigned(const ROW_T& row, const COL_T& column) const {
		return strtounsigned<RET_T>(QueryField(row, column).c_str());
	}
	
	template <typename RET_T, typename ROW_T, typename COL_T>
	RET_T QueryFieldSigned(const ROW_T& row, const COL_T& column) const {
		return strtosigned<RET_T>(QueryField(row, column).c_str());
	}
	
	template <typename ROW_T, typename COL_T>
	ieStrRef QueryFieldAsStrRef(const ROW_T& row, const COL_T& column) const {
		auto field = QueryFieldUnsigned<ieDword>(row, column);
		return static_cast<ieStrRef>(field);
	}
	/** Returns default value of table. */
	virtual const std::string& QueryDefault() const = 0;
	virtual index_t GetColumnIndex(const key_t& colname) const = 0;
	virtual index_t GetRowIndex(const key_t& rowname) const = 0;
	virtual const std::string& GetColumnName(index_t index) const = 0;
	/** Returns a Row Name, returns NULL on error */
	virtual const std::string& GetRowName(index_t index) const = 0;
	virtual index_t FindTableValue(index_t column, long value, index_t start = 0) const = 0;
	virtual index_t FindTableValue(index_t column, const key_t& value, index_t start = 0) const = 0;
	virtual index_t FindTableValue(const key_t& column, long value, index_t start = 0) const = 0;
	virtual index_t FindTableValue(const key_t& column, const key_t& value, index_t start = 0) const = 0;

	/** Opens a Table File */
	virtual bool Open(DataStream* stream) = 0;
};

using AutoTable = std::shared_ptr<TableMgr>;

}

#endif  // ! TABLEMGR_H
