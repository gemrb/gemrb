// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef INIIMPORTER_H
#define INIIMPORTER_H

#include "DataFileMgr.h"

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
