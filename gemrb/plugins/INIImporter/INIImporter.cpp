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

#include "win32def.h"
#include "INIImporter.h"
#include "Interface.h"

INIImp::INIImp(void)
{
	str = NULL;
	autoFree = false;
}

INIImp::~INIImp(void)
{
	if (str && autoFree) {
		delete( str );
	}
	for (unsigned int i = 0; i < tags.size(); i++)
		delete( tags[i] );
}

bool INIImp::Open(DataStream* stream, bool autoFree)
{
	if (stream == NULL) {
		return false;
	}
	if (str && this->autoFree) {
		delete( str );
	}
	str = stream;
	this->autoFree = autoFree;
	int cnt = 0;
	char* strbuf = ( char* ) malloc( 4097 );
	INITag* lastTag = NULL;
	do {
		cnt = str->ReadLine( strbuf, 4096 );
		if (cnt == -1)
			break;
		if (cnt == 0)
			continue;
		if (strbuf[0] == ';') //Rem
			continue;
		if (strbuf[0] == '[') {
			// this is a tag
			char* sbptr = strbuf + 1;
			char* TagName = sbptr;
			while (*sbptr != '\0') {
				if (*sbptr == ']') {
					*sbptr = 0;
					break;
				}
				sbptr++;
			}
			INITag* it = new INITag( TagName );
			tags.push_back( it );
			lastTag = it;
			continue;
		}
		if (lastTag == NULL)
			continue;
		if (lastTag->AddLine( strbuf )) {
			printMessage("INIImporter","", LIGHT_RED);
			printf("Bad Line in file: %s, Section: [%s], Entry: '%s'\n", stream->filename, lastTag->GetTagName(), strbuf);
		}

	} while (true);
	free( strbuf );
	return true;
}

int INIImp::GetKeysCount(const char* Tag)
{
	for (unsigned int i = 0; i < tags.size(); i++) {
		const char* TagName = tags[i]->GetTagName();
		if (stricmp( TagName, Tag ) == 0) {
			return tags[i]->GetKeyCount();
		}
	}
	return 0;
}

const char* INIImp::GetKeyNameByIndex(const char* Tag, int index)
{
	for (unsigned int i = 0; i < tags.size(); i++) {
		const char* TagName = tags[i]->GetTagName();
		if (stricmp( TagName, Tag ) == 0) {
			return tags[i]->GetKeyNameByIndex(index);
		}
	}
	return NULL;
}

const char* INIImp::GetKeyAsString(const char* Tag, const char* Key,
	const char* Default)
{
	for (unsigned int i = 0; i < tags.size(); i++) {
		const char* TagName = tags[i]->GetTagName();
		if (stricmp( TagName, Tag ) == 0) {
			return tags[i]->GetKeyAsString( Key, Default );
		}
	}
	return Default;
}

int INIImp::GetKeyAsInt(const char* Tag, const char* Key,
	const int Default)
{
	for (unsigned int i = 0; i < tags.size(); i++) {
		const char* TagName = tags[i]->GetTagName();
		if (stricmp( TagName, Tag ) == 0) {
			return tags[i]->GetKeyAsInt( Key, Default );
		}
	}
	return Default;
}

float INIImp::GetKeyAsFloat(const char* Tag, const char* Key,
	const float Default)
{
	for (unsigned int i = 0; i < tags.size(); i++) {
		const char* TagName = tags[i]->GetTagName();
		if (stricmp( TagName, Tag ) == 0) {
			return tags[i]->GetKeyAsFloat( Key, Default );
		}
	}
	return Default;
}

bool INIImp::GetKeyAsBool(const char* Tag, const char* Key,
	const bool Default)
{
	for (unsigned int i = 0; i < tags.size(); i++) {
		const char* TagName = tags[i]->GetTagName();
		if (stricmp( TagName, Tag ) == 0) {
			return tags[i]->GetKeyAsBool( Key, Default );
		}
	}
	return Default;
}

#include "plugindef.h"

GEMRB_PLUGIN(0xB62F6D7, "INI File Importer")
PLUGIN_CLASS(IE_INI_CLASS_ID, INIImp)
END_PLUGIN()
