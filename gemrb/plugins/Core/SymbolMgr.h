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
 * $Id$
 *
 */

/**
 * @file SymbolMgr.h
 * Declares SymbolMgr class, abstract loader for symbol tables  (.IDS files)
 * @author The GemRB Project
 */


#ifndef SYMBOLMGR_H
#define SYMBOLMGR_H

/** GetValue returns this if text is not found in arrays */
#define SYMBOL_VALUE_NOT_LOCATED -65535

#include "Plugin.h"
#include "DataStream.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

/**
 * @class SymbolMgr
 * Abstract loader for symbol tables (.IDS files)
 */

class GEM_EXPORT SymbolMgr : public Plugin {
public:
	SymbolMgr(void);
	virtual ~SymbolMgr(void);
	virtual bool Open(DataStream* stream, bool autoFree = true) = 0;
	virtual int GetValue(const char* text) = 0;
	virtual char* GetValue(int val) = 0;
	virtual char* GetStringIndex(unsigned int Index) = 0;
	virtual int GetValueIndex(unsigned int Index) = 0;
	virtual int FindValue(int val) = 0;
	virtual int FindString(char *str, int len) = 0;
	virtual int GetSize() = 0;
};

#endif  // ! SYMBOLMGR_H
