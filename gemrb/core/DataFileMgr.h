/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

/**
 * @file DataFileMgr.h
 * Declares DataFileMgr class, abstract loader for .INI files
 * @author The GemRB Project
 */


#ifndef DATAFILEMGR_H
#define DATAFILEMGR_H

#include "Plugin.h"
#include "System/DataStream.h"

namespace GemRB {

/**
 * @class DataFileMgr
 * Abstract loader for .INI files
 */

class GEM_EXPORT DataFileMgr : public Plugin {
public:
	DataFileMgr(void);
	virtual ~DataFileMgr(void);
	virtual bool Open(DataStream* stream) = 0;
	virtual int GetTagsCount() const = 0;
	virtual const char* GetTagNameByIndex(int index) const = 0;
	virtual int GetKeysCount(const char* Tag) const = 0;
	virtual const char* GetKeyNameByIndex(const char* Tag, int index) const = 0;
	virtual const char* GetKeyAsString(const char* Tag, const char* Key,
		const char* Default) const = 0;
	virtual int GetKeyAsInt(const char* Tag, const char* Key,
		const int Default) const = 0;
	virtual float GetKeyAsFloat(const char* Tag, const char* Key,
		const float Default) const = 0;
	virtual bool GetKeyAsBool(const char* Tag, const char* Key,
		const bool Default) const = 0;
};

}

#endif
