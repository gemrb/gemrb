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

#include "IDSImporter.h"

#include "globals.h"

using namespace GemRB;

const std::string blank;

bool IDSImporter::Open(std::unique_ptr<DataStream> str)
{
	str->CheckEncrypted();
	std::string line;
	str->ReadLine(line, 10);
	if (line[0] != 'I') {
		str->Rewind();
	}
	while (str->ReadLine(line) != DataStream::Error) {
		if (line.length() == 0) {
			continue;
		}

		auto parts = Explode<std::string, std::string>(line, ' ', 1);
		if (parts.size() < 2) {
			continue; // bad data?
		}

		int val = strtosigned<int>(parts[0].c_str(), nullptr, 0);
		StringToLower(parts[1]);
		pairs.emplace_back(val, std::move(parts[1]));
	}

	return true;
}

int IDSImporter::GetValue(StringView txt) const
{
	for (const auto& pair : pairs) {
		if (stricmp(pair.second.c_str(), txt.c_str()) == 0) {
			return pair.first;
		}
	}
	return -1;
}

const std::string& IDSImporter::GetValue(int val) const
{
	for (const auto& pair : pairs) {
		if (pair.first == val) {
			return pair.second;
		}
	}
	return blank;
}

const std::string& IDSImporter::GetStringIndex(size_t Index) const
{
	if (Index >= pairs.size()) {
		return blank;
	}
	return pairs[Index].second;
}

int IDSImporter::GetValueIndex(size_t Index) const
{
	assert(Index < pairs.size());
	return pairs[Index].first;
}

int IDSImporter::FindString(StringView str) const
{
	int i = static_cast<int>(pairs.size());
	while (i--) {
		if (strnicmp(pairs[i].second.c_str(), str.c_str(), str.length()) == 0) {
			return i;
		}
	}
	return -1;
}

int IDSImporter::FindValue(int val) const
{
	int i = static_cast<int>(pairs.size());
	while (i--) {
		if (pairs[i].first == val) {
			return i;
		}
	}
	return -1;
}

void IDSImporter::AddSymbol(StringView str, int val)
{
	pairs.emplace_back(val, str.c_str());
}

int IDSImporter::GetHighestValue() const
{
	int i = static_cast<int>(pairs.size());
	if (!i) {
		return -1;
	}
	//the highest value is likely at the last line
	i--;
	int max = pairs[i].first;
	while (i--) {
		if (pairs[i].first > max) {
			max = pairs[i].first;
		}
	}
	return max;
}


#include "plugindef.h"

GEMRB_PLUGIN(0x1F41B94C, "IDS File Importer")
PLUGIN_CLASS(IE_IDS_CLASS_ID, IDSImporter)
END_PLUGIN()
