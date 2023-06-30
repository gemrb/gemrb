/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2013 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef __GemRB__InterfaceConfig__
#define __GemRB__InterfaceConfig__

#include <unordered_map>

#include "exports.h"

#include "Streams/DataStream.h"

namespace GemRB {

class GEM_EXPORT InterfaceConfig
{
private:
	std::unordered_map<std::string, std::string> configVars;

public:
	using key_t = std::string;
	using value_t = std::string;

	InterfaceConfig& operator=(const InterfaceConfig&) = delete;

	void SetKeyValuePair(const key_t& key, const value_t& value);
	const value_t* GetValueForKey(const key_t& key) const;
	//const std::string* GetValueForKey(std::string* key) const;
};

// the defacto config class
// any platform can use it, but many will want to have their own
// that coan create a InterfaceConfig instance from non cfg sources
class GEM_EXPORT CFGConfig : public InterfaceConfig
{
private:
	bool isValid = false;

private:
	bool InitWithINIData(DataStream* cfgStream);

public:
	CFGConfig(int argc, char *argv[]);

	bool IsValidConfig() const { return isValid; };
};

}

#endif /* defined(__GemRB__InterfaceConfig__) */
