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

#include "globals.h"

#include "DataFileMgr.h"

#include <cstring>
#include <unordered_map>
#include <vector>

namespace GemRB {

class INIImporter : public DataFileMgr {
private:
	std::vector<KeyValueGroup> tags;

public:
	INIImporter() noexcept = default;
	~INIImporter(void) override = default;
	bool Open(std::unique_ptr<DataStream> stream) override;

	KeyValueGroupIterator begin() const override;
	KeyValueGroupIterator end() const override;
	KeyValueGroupIterator find(StringView Tag) const override;

	size_t GetTagsCount() const override;
	StringView GetKeyAsString(StringView Tag, StringView Key, StringView Default = StringView()) const override;
	int GetKeyAsInt(StringView Tag, StringView Key, int Default) const override;
	float GetKeyAsFloat(StringView Tag, StringView Key, float Default) const override;
	bool GetKeyAsBool(StringView Tag, StringView Key, bool Default) const override;

private:
	template<typename T>
	T GetAs(StringView Tag, StringView Key, const T Default) const
	{
		auto result = find(Tag);
		if (result != end()) {
			return result->GetAs<T>(Key, Default);
		}

		return Default;
	}
};

}

#endif
