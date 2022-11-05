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

	bool AddLine(std::string iniLine)
	{
		auto equalsPos = iniLine.find_first_of('=');
		if (equalsPos == std::string::npos) {
			return false;
		}

		auto keyStart = iniLine.find_first_not_of(' ');
		if (keyStart == std::string::npos) return true;
		std::string key = iniLine.substr(keyStart, equalsPos - keyStart);
		key = key.substr(0, key.find_last_not_of(' ') + 1); // right trimming

		auto valueStart = iniLine.find_first_not_of(' ', equalsPos + 1);
		auto valueEnd = iniLine.find_last_not_of(' ');
		if (valueStart == std::string::npos) return true;
		if (valueEnd == std::string::npos) return true;
		std::string value = iniLine.substr(valueStart, valueEnd - valueStart + 1);

		INIPair p = { key, value };
		pairs.push_back(p);
		return true;
	}

	StringView GetKeyAsString(StringView Key, StringView Default) const
	{
		for (const auto& pair : pairs) {
			if (stricmp(Key.c_str(), pair.Name.c_str()) == 0) {
				return pair.Value;
			}
		}
		return Default;
	}

	int GetKeyAsInt(StringView Key, const int Default) const
	{
		const char* ret = nullptr;
		for (const auto& pair : pairs) {
			if (stricmp(Key.c_str(), pair.Name.c_str()) == 0) {
				ret = pair.Value.c_str();
				break;
			}
		}
		if (!ret) {
			return Default;
		}
		return atoi(ret);
	}

	float GetKeyAsFloat(StringView Key, const float Default) const
	{
		const char* ret = nullptr;
		for (const auto& pair : pairs) {
			if (stricmp(Key.c_str(), pair.Name.c_str()) == 0) {
				ret = pair.Value.c_str();
				break;
			}
		}
		if (!ret) {
			return Default;
		}
		return atof(ret);
	}

	bool GetKeyAsBool(StringView Key, const bool Default) const
	{
		const char* ret = nullptr;
		for (const auto& pair : pairs) {
			if (stricmp(Key.c_str(), pair.Name.c_str()) == 0) {
				ret = pair.Value.c_str();
				break;
			}
		}
		if (!ret) {
			return Default;
		}
		if (!stricmp(ret, "true")) {
			return true;
		}
		if (!stricmp(ret, "false")) {
			return false;
		}
		return atoi(ret) != 0;
	}
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
