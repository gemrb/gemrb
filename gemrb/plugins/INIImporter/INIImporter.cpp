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

#include "Logging/Logging.h"

using namespace GemRB;

// standard INI file importer
bool INIImporter::Open(std::unique_ptr<DataStream> str)
{
	std::string strbuf;
	KeyValueGroup* lastTag = NULL;
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
				Log(WARNING, "INIImporter", "Skipping empty section in '{}', entry: '{}'", str->filename, lastTag->GetName());
				tags.pop_back();
			}

			startedSection = true;
			tags.emplace_back(std::move(tag));
			lastTag = &tags[tags.size() - 1];
			continue;
		}
		if (lastTag == NULL)
			continue;
		if (lastTag->AddLine(strbuf)) {
			startedSection = false;
		} else {
			Log(ERROR, "INIImporter", "Bad Line in file: {}, Section: [{}], Entry: '{}'",
				str->filename, lastTag->GetName(), strbuf);
		}
	}

	return true;
}

INIImporter::KeyValueGroupIterator INIImporter::begin() const {
	return tags.begin();
}

INIImporter::KeyValueGroupIterator INIImporter::end() const {
	return tags.end();
}

INIImporter::KeyValueGroupIterator INIImporter::find(StringView tag) const {
	return std::find_if(tags.begin(), tags.end(), [tag](const auto& t) {
		return stricmp(t.GetName().c_str(), tag.c_str()) == 0;
	});
}

size_t INIImporter::GetTagsCount() const {
	return tags.size();
}

StringView INIImporter::GetKeyAsString(StringView Tag, StringView Key, StringView Default) const
{
	return GetAs<StringView>(Tag, Key, Default);
}

int INIImporter::GetKeyAsInt(StringView Tag, StringView Key, const int Default) const
{
	return GetAs<int>(Tag, Key, Default);
}

float INIImporter::GetKeyAsFloat(StringView Tag, StringView Key, const float Default) const
{
	return GetAs<float>(Tag, Key, Default);
}

bool INIImporter::GetKeyAsBool(StringView Tag, StringView Key, const bool Default) const
{
	return GetAs<bool>(Tag, Key, Default);
}

#include "plugindef.h"

GEMRB_PLUGIN(0xB62F6D7, "INI File Importer")
PLUGIN_CLASS(IE_INI_CLASS_ID, INIImporter)
END_PLUGIN()
