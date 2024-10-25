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
 */

/**
 * @file SymbolMgr.h
 * Declares SymbolMgr class, abstract loader for symbol tables  (.IDS files)
 * @author The GemRB Project
 */

#ifndef SYMBOLMGR_H
#define SYMBOLMGR_H

#include "Plugin.h"

#include "Streams/DataStream.h"

#include <memory>

namespace GemRB {

/**
 * @class SymbolMgr
 * Abstract loader for symbol tables (.IDS files)
 */

class GEM_EXPORT SymbolMgr : public Plugin {
public:
	virtual bool Open(std::unique_ptr<DataStream> stream) = 0;
	/// Returns -1 if string isn't found.
	virtual int GetValue(StringView text) const = 0;
	virtual const std::string& GetValue(int val) const = 0;
	virtual const std::string& GetStringIndex(size_t Index) const = 0;
	virtual int GetValueIndex(size_t Index) const = 0;
	virtual int FindValue(int val) const = 0;
	virtual int FindString(StringView str) const = 0;
	virtual size_t GetSize() const = 0;
	virtual int GetHighestValue() const = 0;
};

}

#endif // ! SYMBOLMGR_H
