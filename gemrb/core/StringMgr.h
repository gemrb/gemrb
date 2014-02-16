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

/**
 * @file StringMgr.h
 * Declares StringMgr class, loader for StringBlock objects (.TLK files)
 * @author The GemRB Project
 */


#ifndef STRINGMGR_H
#define STRINGMGR_H

#include "Plugin.h"
#include "System/DataStream.h"

namespace GemRB {

/**
 * @struct StringBlock
 * Text and its associated sound.
 */

struct StringBlock {
	String text;
	ieResRef Sound;

	StringBlock(const String& text, const ieResRef soundRef)
	: text(text) {
		memcpy(Sound, soundRef, sizeof(ieResRef));
	}
	StringBlock()
	: text(), Sound() {}
};

/**
 * @class StringMgr
 * Abstract loader for StringBlock objects (strings in .TLK files)
 */

class GEM_EXPORT StringMgr : public Plugin {
public:
	StringMgr(void);
	virtual ~StringMgr(void);
	virtual void OpenAux() = 0;
	virtual void CloseAux() = 0;
	virtual bool Open(DataStream* stream) = 0;
	virtual char* GetCString(ieStrRef strref, unsigned int flags = 0) = 0;
	virtual String* GetString(ieStrRef strref, unsigned int flags = 0) = 0;
	virtual StringBlock GetStringBlock(ieStrRef strref, unsigned int flags = 0) = 0;
	virtual ieStrRef UpdateString(ieStrRef strref, const char *text) = 0;
};

}

#endif  // ! STRINGMGR_H
