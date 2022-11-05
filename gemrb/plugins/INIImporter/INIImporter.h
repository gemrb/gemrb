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

#ifndef INIIMPORTER_H
#define INIIMPORTER_H

#include "DataFileMgr.h"

#include "globals.h"

#include <cstring>
#include <vector>

namespace GemRB {

struct INIPair {
	std::string Name;
	std::string Value;
};

class INITag {
private:
	std::vector< INIPair> pairs;
	std::string TagName;
public:
	explicit INITag(std::string Name) : TagName(std::move(Name)) {};

	const std::string& GetTagName() const
	{
		return TagName;
	}

	int GetKeyCount() const
	{
		return (int) pairs.size();
	}

	const std::string& GetKeyNameByIndex(int index) const
	{
		return pairs[index].Name;
	}

	bool AddLine(std::string iniLine);
	StringView GetKeyAsString(StringView Key, StringView Default) const;
	int GetKeyAsInt(StringView Key, const int Default) const;
	float GetKeyAsFloat(StringView Key, const float Default) const;
	bool GetKeyAsBool(StringView Key, const bool Default) const;
};

class INIImporter : public DataFileMgr {
private:
	std::vector<INITag> tags;

public:
	INIImporter() noexcept = default;
	~INIImporter(void) override = default;
	bool Open(DataStream* stream) override;
	int GetTagsCount() const override
	{
		return static_cast<int>(tags.size());
	}
	StringView GetTagNameByIndex(int index) const override
	{
		return tags[index].GetTagName();
	}

	int GetKeysCount(StringView Tag) const override;
	StringView GetKeyNameByIndex(StringView Tag, int index) const override;
	StringView GetKeyAsString(StringView Tag, StringView Key, StringView Default = StringView()) const override;
	int GetKeyAsInt(StringView Tag, StringView Key, int Default) const override;
	float GetKeyAsFloat(StringView Tag, StringView Key, float Default) const override;
	bool GetKeyAsBool(StringView Tag, StringView Key, bool Default) const override;
};

}

#endif
