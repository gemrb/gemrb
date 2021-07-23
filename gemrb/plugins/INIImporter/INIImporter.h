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
	char* Name, * Value;
};

class INITag {
private:
	std::vector< INIPair> pairs;
	char* TagName;
public:
	explicit INITag(const char* Name)
	{
		int len = ( int ) strlen( Name ) + 1;
		TagName = ( char * ) malloc( len );
		memcpy( TagName, Name, len );
	}

	~INITag()
	{
		free( TagName );
		for (auto& pair : pairs) {
			free(pair.Name);
			free(pair.Value);
		}
	}

	const char* GetTagName() const
	{
		return TagName;
	}

	int GetKeyCount() const
	{
		return (int) pairs.size();
	}

	const char* GetKeyNameByIndex(int index) const
	{
		return pairs[index].Name;
	}

	bool AddLine(char* Line)
	{
		INIPair p;
		char* equal = strchr( Line, '=' );
		if(!equal) {
			return true;
		}
		*equal = 0;
		char* NameKey = Line;
		char* ValueKey = equal + 1;
		if (!NameKey || !ValueKey) return true; // line starting or ending with sole =

		//Left Trimming
		while (*NameKey != '\0') {
			if (*NameKey != ' ')
				break;
			NameKey++;
		}
		while (*ValueKey != '\0') {
			if (*ValueKey != ' ')
				break;
			ValueKey++;
		}
		//Right Trimming
		int NameKeyLen = ( int ) strlen( NameKey );
		int ValueKeyLen = ( int ) strlen( ValueKey );
		char* endNameKey = NameKey + NameKeyLen - 1;
		char* endValueKey = ValueKey + ValueKeyLen - 1;
		while (endNameKey != NameKey) {
			if (*endNameKey != ' ')
				break;
			*endNameKey-- = 0;
			NameKeyLen--;
		}
		while (endValueKey != ValueKey) {
			if (*endValueKey != ' ')
				break;
			*endValueKey-- = 0;
			ValueKeyLen--;
		}
		//Allocating Buffers
		p.Name = ( char * ) malloc( NameKeyLen + 1 );
		p.Value = ( char * ) malloc( ValueKeyLen + 1 );
		//Copying buffers
		memcpy( p.Name, NameKey, NameKeyLen + 1 );
		memcpy( p.Value, ValueKey, ValueKeyLen + 1 );
		//Adding to Tag Pairs
		pairs.push_back( p );
		return false;
	}

	const char* GetKeyAsString(const char* Key, const char* Default) const
	{
		for (const auto pair : pairs) {
			if (stricmp(Key, pair.Name) == 0) {
				return pair.Value;
			}
		}
		return Default;
	}

	int GetKeyAsInt(const char* Key, const int Default) const
	{
		const char* ret = NULL;
		for (const auto pair : pairs) {
			if (stricmp(Key, pair.Name) == 0) {
				ret = pair.Value;
				break;
			}
		}
		if (!ret) {
			return Default;
		}
		return atoi( ret );
	}

	float GetKeyAsFloat(const char* Key, const float Default) const
	{
		const char* ret = NULL;
		for (const auto pair : pairs) {
			if (stricmp(Key, pair.Name) == 0) {
				ret = pair.Value;
				break;
			}
		}
		if (!ret) {
			return Default;
		}
		return atof( ret );
	}

	bool GetKeyAsBool(const char* Key, const bool Default) const
	{
		const char* ret = NULL;
		for (const auto pair : pairs) {
			if (stricmp(Key, pair.Name) == 0) {
				ret = pair.Value;
				break;
			}
		}
		if (!ret) {
			return Default;
		}
		if (!stricmp( ret, "true") ) {
			return true;
		}
		if (!stricmp( ret, "false") ) {
			return false;
		}
		return (atoi( ret ) )!=0;
	}
};

class INIImporter : public DataFileMgr {
private:
	std::vector< INITag*> tags;

public:
	INIImporter() = default;
	~INIImporter(void) override;
	bool Open(DataStream* stream) override;
	int GetTagsCount() const override
	{
		return ( int ) tags.size();
	}
	const char* GetTagNameByIndex(int index) const override
	{
		return tags[index]->GetTagName();
	}

	int GetKeysCount(const char* Tag) const override;
	const char* GetKeyNameByIndex(const char* Tag, int index) const override;
	const char* GetKeyAsString(const char* Tag, const char* Key,
		const char* Default) const override;
	int GetKeyAsInt(const char* Tag, const char* Key, 
		int Default) const override;
	float GetKeyAsFloat(const char* Tag, const char* Key, 
		float Default) const override;
	bool GetKeyAsBool(const char* Tag, const char* Key, 
		bool Default) const override;
};

}

#endif
