// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
	NONE = 0,
	STRREFON = 1,
	SOUND = 2,
	SPEECH = 4,
	ALLOW_ZERO = 8, // 0 strref is allowed
	RESOLVE_TAGS = 16,
	STRREFOFF = 256,
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

#endif // ! STRINGMGR_H
