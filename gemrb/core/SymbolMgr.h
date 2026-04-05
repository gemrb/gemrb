// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
	virtual void AddSymbol(StringView str, int val) = 0;
	virtual size_t GetSize() const = 0;
	virtual int GetHighestValue() const = 0;
};

}

#endif // ! SYMBOLMGR_H
