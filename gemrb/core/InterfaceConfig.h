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

#include "exports.h"

#include "StringMap.h"

#include "System/DataStream.h"

namespace GemRB {

class GEM_EXPORT InterfaceConfig
{
private:
	StringMap* configVars;

public:
	InterfaceConfig(int argc, char *argv[]);
	virtual ~InterfaceConfig();

	void SetKeyValuePair(const char* key, const char* value);
	const char* GetValueForKey(const char* key) const;
	const std::string* GetKeyValuePair(std::string* key) const;
};

// the defacto config class
// any platform can use it, but many will want to have their own
// that coan create a InterfaceConfig instance from non cfg sources
class GEM_EXPORT CFGConfig : public InterfaceConfig
{
private:
	bool isValid;

private:
	bool InitWithINIData(DataStream* const cfgStream);

public:
	CFGConfig(int argc, char *argv[]);
	~CFGConfig();

	bool IsValidConfig() {return isValid;};
};

}

#endif /* defined(__GemRB__InterfaceConfig__) */
