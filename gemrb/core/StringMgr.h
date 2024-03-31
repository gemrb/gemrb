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
#include "Resource.h"
#include "Streams/DataStream.h"

namespace GemRB {

/**
 * @struct StringBlock
 * Text and its associated sound.
 */

struct StringBlock {
	String text;
	ResRef Sound;

	StringBlock() noexcept = default;

	StringBlock(String text, const ResRef& soundRef) noexcept
	: text(std::move(text)), Sound(soundRef) {}
	
	StringBlock(StringBlock&&) noexcept = default;
	StringBlock(const StringBlock&) = delete;
};

enum class STRING_FLAGS : uint32_t {
	NONE			= 0,
	STRREFON		= 1,
	SOUND 			= 2,
	SPEECH			= 4,
	ALLOW_ZERO		= 8, // 0 strref is allowed
	RESOLVE_TAGS	= 16,
	STRREFOFF		= 256,
	// iwd2 also used 0x8000 to mark removed ctlk entries for recycling
};

/**
 * @class StringMgr
 * Abstract loader for StringBlock objects (strings in .TLK files)
 */

class GEM_EXPORT StringMgr : public Plugin {
public:
	virtual void OpenAux() = 0;
	virtual void CloseAux() = 0;
	virtual bool Open(DataStream* stream) = 0;
	virtual String GetString(ieStrRef strref, STRING_FLAGS flags = STRING_FLAGS::NONE) = 0;
	virtual StringBlock GetStringBlock(ieStrRef strref, STRING_FLAGS flags = STRING_FLAGS::NONE) = 0;
	virtual ieStrRef UpdateString(ieStrRef strref, const String& text) = 0;
	virtual bool HasAltTLK() const = 0;
	virtual ieStrRef GetNextStrRef() const = 0;
};

}

#endif  // ! STRINGMGR_H
