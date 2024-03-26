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

#include "2DAImporter.h"

#include "Logging/Logging.h"
#include "Streams/FileStream.h"

using namespace GemRB;

static bool StringCompKey(const std::string& str, TableMgr::key_t key)
{
	return stricmp(str.c_str(), key.c_str()) == 0;
}

TableMgr::index_t p2DAImporter::npos = TableMgr::npos;

bool p2DAImporter::Open(std::unique_ptr<DataStream> str)
{
	str->CheckEncrypted();

	std::string line;
	str->ReadLine(line);
	LTrim(line);
	if (line.compare(0, 8, "2DA V1.0") != 0) {
		Log(WARNING, "2DAImporter", "Bad signature ({})! Complaining, but not ignoring...", str->filename);
		// we don't care about this, so exptable.2da of iwd2 won't cause a bigger problem
		// also, certain creatures are described by 2da's without signature.
		// return false;
	}
	str->ReadLine(line);
	auto pos = line.find_first_of(' ');
	if (pos != std::string::npos) {
		defVal = line.substr(0, pos);
	} else { // no whitespace
		defVal = line;
	}
	
	auto NextLine = [&]() -> bool {
		while (str->ReadLine(line) != DataStream::Error) {
			if (line[0] != '#') { // allow comments
				return true;
			}
		}
		return false;
	};
	
	NextLine();
	size_t end = line.find_first_not_of(WHITESPACE_STRING);
	if (end != std::string::npos)
		colNames = Explode<StringView, cell_t>(StringView(line, end), ' ');
	
	rowNames.reserve(10);
	rows.reserve(10);
	while (NextLine()) {
		pos = line.find_first_of(' ');
		if (pos == std::string::npos) {
			if (line.empty()) continue;
			// row with no data, but a valid row name (eg. first row in iwd music.2da)
			rowNames.emplace_back(line);
			rows.emplace_back();
			continue;
		}

		rowNames.emplace_back(line.substr(0, pos));
		
		auto sv = StringView(&line[pos + 1], line.length() - pos - 1);
		rows.emplace_back(Explode<StringView, cell_t>(sv, ' ', std::max<size_t>(1, colNames.size() - 1)));
	}

	assert(rows.size() < std::numeric_limits<index_t>::max());
	return true;
}

/** Returns the actual number of Rows in the Table */
p2DAImporter::index_t p2DAImporter::GetRowCount() const
{
	return static_cast<index_t>(rows.size());
}

p2DAImporter::index_t p2DAImporter::GetColNamesCount() const
{
	return static_cast<index_t>(colNames.size());
}

/** Returns the actual number of Columns in the Table */
p2DAImporter::index_t p2DAImporter::GetColumnCount(index_t row) const
{
	if (rows.size() <= row) {
		return 0;
	}
	return static_cast<index_t>(rows[row].size());
}
/** Returns a pointer to a zero terminated 2da element,
	if it cannot return a value, it returns the default */
const std::string& p2DAImporter::QueryField(index_t row, index_t column) const
{
	if (rows.size() <= row) {
		return defVal;
	}
	if (rows[row].size() <= column) {
		return defVal;
	}
	if (rows[row][column] == "*") {
		return defVal;
	}
	return rows[row][column];
}

const std::string& p2DAImporter::QueryDefault() const
{
	return defVal;
}

p2DAImporter::index_t p2DAImporter::GetRowIndex(const key_t& key) const
{
	for (index_t index = 0; index < rowNames.size(); index++) {
		if (StringCompKey(rowNames[index], key)) {
			return index;
		}
	}
	return npos;
}

p2DAImporter::index_t p2DAImporter::GetColumnIndex(const key_t& key) const
{
	for (index_t index = 0; index < colNames.size(); index++) {
		if (StringCompKey(colNames[index], key)) {
			return index;
		}
	}
	return npos;
}

const static std::string blank;
const std::string& p2DAImporter::GetColumnName(index_t index) const
{
	if (index < colNames.size()) {
		return colNames[index];
	}
	return blank;
}

const std::string& p2DAImporter::GetRowName(index_t index) const
{
	if (index < rowNames.size()) {
		return rowNames[index];
	}
	return blank;
}

p2DAImporter::index_t p2DAImporter::FindTableValue(index_t col, long val, index_t start) const
{
	index_t max = GetRowCount();
	for (index_t row = start; row < max; row++) {
		const std::string& ret = QueryField( row, col );
		long Value;
		if (valid_signednumber(ret.c_str(), Value) && (Value == val))
			return row;
	}
	return npos;
}

p2DAImporter::index_t p2DAImporter::FindTableValue(index_t col, const key_t& val, index_t start) const
{
	index_t max = GetRowCount();
	for (index_t row = start; row < max; row++) {
		const std::string& ret = QueryField( row, col );
		if (StringCompKey(ret, val))
			return row;
	}
	return npos;
}

p2DAImporter::index_t p2DAImporter::FindTableValue(const key_t& col, long val, index_t start) const
{
	index_t coli = GetColumnIndex(col);
	return FindTableValue(coli, val, start);
}

p2DAImporter::index_t p2DAImporter::FindTableValue(const key_t& col, const key_t& val, index_t start) const
{
	index_t coli = GetColumnIndex(col);
	return FindTableValue(coli, val, start);
}

#include "plugindef.h"

GEMRB_PLUGIN(0xB22F938, "2DA File Importer")
PLUGIN_CLASS(IE_2DA_CLASS_ID, p2DAImporter)
END_PLUGIN()
