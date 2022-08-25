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

class p2DAImporter : public TableMgr {
private:
	using cell_t = std::string;
	using row_t = std::vector<cell_t>;

	std::vector<cell_t> colNames;
	std::vector<cell_t> rowNames;
	std::vector<row_t> rows;
	std::string defVal;
public:
	p2DAImporter& operator=(const p2DAImporter&) = delete;
	bool Open(DataStream* stream) override;
	/** Returns the actual number of Rows in the Table */
	index_t GetRowCount() const override;

	index_t GetColNamesCount() const override;

	/** Returns the actual number of Columns in the Table */
	index_t GetColumnCount(index_t row = 0) const override;
	/** Returns a pointer to a zero terminated 2da element,
		if it cannot return a value, it returns the default */
	const std::string& QueryField(index_t row, index_t column) const override;
	const std::string& QueryDefault() const override;

	index_t GetRowIndex(const key_t& string) const override;
	index_t GetColumnIndex(const key_t& string) const override;

	const std::string& GetColumnName(index_t index) const override;
	const std::string& GetRowName(index_t index) const override;

	index_t FindTableValue(index_t col, long val, index_t start) const override;
	index_t FindTableValue(index_t col, const key_t& val, index_t start) const override;
	index_t FindTableValue(const key_t& col, long val, index_t start) const override;
	index_t FindTableValue(const key_t& col, const key_t& val, index_t start) const override;
};

}

#endif
