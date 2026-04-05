// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * @file DataFileMgr.h
 * Declares DataFileMgr class, abstract loader for .INI files
 * @author The GemRB Project
 */


#ifndef DATAFILEMGR_H
#define DATAFILEMGR_H

#include "Plugin.h"

#include "Streams/DataStream.h"
#include "Strings/StringMap.h"

#include <algorithm>
#include <memory>
#include <vector>

namespace GemRB {

/**
 * @class DataFileMgr
 * Abstract loader for .INI files
 */

class KeyValueGroup {
public:
	using KeyValueMap = StringMap<std::string>;

private:
	KeyValueMap kvMap;
	const std::string name;

public:
	explicit KeyValueGroup(std::string name)
		: name(std::move(name)) {}

	const std::string& GetName() const
	{
		return name;
	}

	auto begin() const
	{
		return kvMap.begin();
	}

	auto end() const
	{
		return kvMap.end();
	}

	size_t size() const
	{
		return kvMap.size();
	}

	template<typename T>
	T GetAs(StringView key, const T) const = delete;

	bool AddLine(StringView iniLine)
	{
		auto equalsPos = FindFirstOf(iniLine, "=");
		if (equalsPos == std::string::npos) {
			return false;
		}

		auto keyStart = FindFirstNotOf(iniLine, " ");
		if (keyStart == std::string::npos) {
			return true;
		}

		auto key = RTrimCopy(SubStr(iniLine, keyStart, equalsPos - keyStart));

		auto valueStart = FindFirstNotOf(iniLine, " ", equalsPos + 1);
		if (valueStart == std::string::npos) {
			return true;
		}

		auto valueEnd = FindLastNotOf(iniLine, " ");
		if (valueEnd == std::string::npos) {
			return true;
		}

		auto value = SubStr(iniLine, valueStart, valueEnd - valueStart + 1);
		kvMap.Set(key, value.MakeString());

		return true;
	}
};

template<>
inline StringView KeyValueGroup::GetAs<StringView>(StringView key, const StringView Default) const
{
	auto result = kvMap.Get(key);
	if (result != nullptr) {
		return static_cast<StringView>(*result);
	}

	return Default;
}

template<>
inline int KeyValueGroup::GetAs<int>(StringView key, const int Default) const
{
	auto result = kvMap.Get(key);
	if (result != nullptr) {
		return atoi(result->c_str());
	}

	return Default;
}

template<>
inline float KeyValueGroup::GetAs<float>(StringView key, const float Default) const
{
	auto result = kvMap.Get(key);
	if (result != nullptr) {
		return static_cast<float>(atof(result->c_str()));
	}

	return Default;
}

template<>
inline bool KeyValueGroup::GetAs<bool>(StringView key, const bool Default) const
{
	auto result = kvMap.Get(key);
	if (result != nullptr) {
		const auto str = result->c_str();

		if (stricmp(str, "true") == 0) {
			return true;
		}

		if (stricmp(str, "false") == 0) {
			return false;
		}

		return atoi(str) != 0;
	}

	return Default;
}

class GEM_EXPORT DataFileMgr : public Plugin {
public:
	using KeyValueGroupIterator = std::vector<KeyValueGroup>::const_iterator;

	virtual bool Open(std::unique_ptr<DataStream> stream) = 0;

	virtual KeyValueGroupIterator begin() const = 0;
	virtual KeyValueGroupIterator end() const = 0;
	virtual KeyValueGroupIterator find(StringView Tag) const = 0;

	virtual size_t GetTagsCount() const = 0;
	virtual StringView GetKeyAsString(StringView Tag, StringView Key, StringView Default = StringView()) const = 0;
	virtual int GetKeyAsInt(StringView Tag, StringView Key, int Default) const = 0;
	virtual float GetKeyAsFloat(StringView Tag, StringView Key, float Default) const = 0;
	virtual bool GetKeyAsBool(StringView Tag, StringView Key, bool Default) const = 0;
};

}

#endif
