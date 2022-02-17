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

#include "Interface.h"
#include "Streams/FileStream.h"

using namespace GemRB;

#define SIGNLENGTH 256      //if a 2da has longer default value, change this

p2DAImporter::p2DAImporter(void)
{
	colNames.reserve(10);
	rowNames.reserve(10);
	ptrs.reserve(10);
	rows.reserve(10);
}

p2DAImporter::~p2DAImporter(void)
{
	for (auto& ptr : ptrs) {
		free(ptr);
	}
}

bool p2DAImporter::Open(DataStream* str)
{
	if (str == NULL) {
		return false;
	}
	char Signature[SIGNLENGTH];
	str->CheckEncrypted();

	str->ReadLine( Signature, sizeof(Signature) );
	const char* strp = Signature;
	while (*strp == ' ')
		strp++;
	if (strncmp( strp, "2DA V1.0", 8 ) != 0) {
		Log(WARNING, "2DAImporter", "Bad signature ({})! Complaining, but not ignoring...", str->filename);
		// we don't care about this, so exptable.2da of iwd2 won't cause a bigger problem
		// also, certain creatures are described by 2da's without signature.
		// return false;
	}
	str->ReadLine( Signature, sizeof(Signature) );
	const char* token = strtok(Signature, " ");
	if (token) {
		defVal = token;
	} else { // no whitespace
		defVal = Signature;
	}
	bool colHead = true;
	int row = 0;
	
	constexpr int MAXLENGTH = 8192;
	char buffer[MAXLENGTH]; // we can increase this if needed, but beware since it is a stack buffer
	while (true) {
		strret_t len = str->ReadLine(buffer, MAXLENGTH);
		if (len <= 0) {
			break;
		}
		if (buffer[0] == '#') { // allow comments
			continue;
		}

		ptrs.push_back(strdup(buffer));
		char* line = ptrs.back();
		if (colHead) {
			colHead = false;
			char* cell = strtok(line, " ");
			while (cell != nullptr) {
				colNames.push_back(cell);
				cell = strtok(nullptr, " ");
			}
		} else {
			char* cell = strtok(line, " ");
			if (cell == nullptr) continue;

			rowNames.push_back(cell);
			rows.emplace_back();
			rows[row].reserve(10);
			cell = strtok(nullptr, " ");
			while (cell != nullptr) {
				rows[row].push_back(cell);
				cell = strtok(nullptr, " ");
			}
			row++;
		}
	}
	delete str;
	return true;
}

/** Returns the actual number of Rows in the Table */
p2DAImporter::index_t p2DAImporter::GetRowCount() const
{
	return rows.size();
}

p2DAImporter::index_t p2DAImporter::GetColNamesCount() const
{
	return colNames.size();
}

/** Returns the actual number of Columns in the Table */
p2DAImporter::index_t p2DAImporter::GetColumnCount(index_t row) const
{
	if (rows.size() <= row) {
		return 0;
	}
	return rows[row].size();
}
/** Returns a pointer to a zero terminated 2da element,
	if it cannot return a value, it returns the default */
const char* p2DAImporter::QueryField(index_t row, index_t column) const
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

const char* p2DAImporter::QueryDefault() const
{
	return defVal.c_str();
}

p2DAImporter::index_t p2DAImporter::GetRowIndex(const char* string) const
{
	for (index_t index = 0; index < rowNames.size(); index++) {
		if (stricmp( rowNames[index], string ) == 0) {
			return index;
		}
	}
	return npos;
}

p2DAImporter::index_t p2DAImporter::GetColumnIndex(const char* string) const
{
	for (index_t index = 0; index < colNames.size(); index++) {
		if (stricmp( colNames[index], string ) == 0) {
			return index;
		}
	}
	return npos;
}

const char* p2DAImporter::GetColumnName(index_t index) const
{
	if (index < colNames.size()) {
		return colNames[index];
	}
	return "";
}

const char* p2DAImporter::GetRowName(index_t index) const
{
	if (index < rowNames.size()) {
		return rowNames[index];
	}
	return "";
}

p2DAImporter::index_t p2DAImporter::FindTableValue(index_t col, long val, index_t start) const
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

p2DAImporter::index_t p2DAImporter::FindTableValue(index_t col, const char* val, index_t start) const
{
	index_t max = GetRowCount();
	for (index_t row = start; row < max; row++) {
		const char* ret = QueryField( row, col );
		if (stricmp(ret, val) == 0)
			return row;
	}
	return npos;
}

p2DAImporter::index_t p2DAImporter::FindTableValue(const char* col, long val, index_t start) const
{
	index_t coli = GetColumnIndex(col);
	return FindTableValue(coli, val, start);
}

p2DAImporter::index_t p2DAImporter::FindTableValue(const char* col, const char* val, index_t start) const
{
	index_t coli = GetColumnIndex(col);
	return FindTableValue(coli, val, start);
}

#include "plugindef.h"

GEMRB_PLUGIN(0xB22F938, "2DA File Importer")
PLUGIN_CLASS(IE_2DA_CLASS_ID, p2DAImporter)
END_PLUGIN()
