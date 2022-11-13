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

#include "INIImporter.h"

#include "Interface.h"

using namespace GemRB;

bool INITag::AddLine(std::string iniLine)
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

StringView INITag::GetKeyAsString(StringView Key, StringView Default) const
{
	for (const auto& pair : pairs) {
		if (stricmp(Key.c_str(), pair.Name.c_str()) == 0) {
			return pair.Value;
		}
	}
	return Default;
}

int INITag::GetKeyAsInt(StringView Key, const int Default) const
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

float INITag::GetKeyAsFloat(StringView Key, const float Default) const
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
	return static_cast<float>(atof(ret));
}

bool INITag::GetKeyAsBool(StringView Key, const bool Default) const
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

// standard INI file importer
bool INIImporter::Open(DataStream* str)
{
	if (str == NULL) {
		return false;
	}

	std::string strbuf;
	INITag* lastTag = NULL;
	bool startedSection = false;
	while (str->ReadLine(strbuf) != DataStream::Error) {
		if (strbuf.length() == 0)
			continue;
		if (strbuf[0] == ';') //Rem
			continue;
		if (strbuf[0] == '[') {
			// this is a tag
			auto pos = strbuf.find_first_of(']');
			std::string tag = strbuf.substr(1, pos - 1);

			// ignore empty sections
			// pst ar0502.ini has this garbage:
			// [guard4]
			// /.../
			// [guard5] <-- bad, but harmless
			//
			// [guard5]
			// /.../
			// [guard5] <-- bad and overrides contentful section
			//
			// [guard6]
			// /.../
			if (startedSection) {
				Log(WARNING, "INIImporter", "Skipping empty section in '{}', entry: '{}'", str->filename, lastTag->GetTagName());
				tags.pop_back();
			}

			startedSection = true;
			tags.emplace_back(std::move(tag));
			lastTag = &tags[tags.size() - 1];
			continue;
		}
		if (lastTag == NULL)
			continue;
		if (lastTag->AddLine(std::move(strbuf))) {
			startedSection = false;
		} else {
			Log(ERROR, "INIImporter", "Bad Line in file: {}, Section: [{}], Entry: '{}'",
				str->filename, lastTag->GetTagName(), strbuf);
		}
	}
	delete str;
	return true;
}

int INIImporter::GetKeysCount(StringView Tag) const
{
	for (const auto& tag : tags) {
		const std::string& TagName = tag.GetTagName();
		if (stricmp(TagName.c_str(), Tag.c_str()) == 0) {
			return tag.GetKeyCount();
		}
	}
	return 0;
}

StringView INIImporter::GetKeyNameByIndex(StringView Tag, int index) const
{
	for (const auto& tag : tags) {
		const std::string& TagName = tag.GetTagName();
		if (stricmp(TagName.c_str(), Tag.c_str()) == 0) {
			return tag.GetKeyNameByIndex(index);
		}
	}
	return StringView();
}

StringView INIImporter::GetKeyAsString(StringView Tag, StringView Key, StringView Default) const
{
	for (const auto& tag : tags) {
		const std::string& TagName = tag.GetTagName();
		if (stricmp(TagName.c_str(), Tag.c_str()) == 0) {
			return tag.GetKeyAsString(Key, Default);
		}
	}
	return Default;
}

int INIImporter::GetKeyAsInt(StringView Tag, StringView Key, const int Default) const
{
	for (const auto& tag : tags) {
		const std::string& TagName = tag.GetTagName();
		if (stricmp(TagName.c_str(), Tag.c_str()) == 0) {
			return tag.GetKeyAsInt(Key, Default);
		}
	}
	return Default;
}

float INIImporter::GetKeyAsFloat(StringView Tag, StringView Key, const float Default) const
{
	for (const auto& tag : tags) {
		const std::string& TagName = tag.GetTagName();
		if (stricmp(TagName.c_str(), Tag.c_str()) == 0) {
			return tag.GetKeyAsFloat(Key, Default);
		}
	}
	return Default;
}

bool INIImporter::GetKeyAsBool(StringView Tag, StringView Key, const bool Default) const
{
	for (const auto& tag : tags) {
		const std::string& TagName = tag.GetTagName();
		if (stricmp(TagName.c_str(), Tag.c_str()) == 0) {
			return tag.GetKeyAsBool(Key, Default);
		}
	}
	return Default;
}

#include "plugindef.h"

GEMRB_PLUGIN(0xB62F6D7, "INI File Importer")
PLUGIN_CLASS(IE_INI_CLASS_ID, INIImporter)
END_PLUGIN()
