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

#ifndef TLKIMPORTER_H
#define TLKIMPORTER_H

#include "ie_types.h"

#include "StringMgr.h"
#include "TlkOverride.h"

namespace GemRB {

struct gt_type {
	int type;
	ieStrRef male;
	ieStrRef female;
};

class TLKImporter : public StringMgr {
private:
	DataStream* str = nullptr;

	//Data
	ieWord Language = 0;
	ieDword StrRefCount = 0;
	ieDword Offset = 0;
	CTlkOverride *OverrideTLK = nullptr;
	ResRefMap<gt_type> gtmap;
	int charname = 0;

public:
	TLKImporter(void);
	~TLKImporter(void) override;
	/** open string refs coming from saved game */
	void OpenAux() override;
	/** purge string defs coming from saved game */
	void CloseAux() final;
	bool Open(DataStream* stream) override;
	/** construct a new custom string */
	ieStrRef UpdateString(ieStrRef strref, const String& newvalue) override;
	/** resolve a string reference */
	String GetString(ieStrRef strref, STRING_FLAGS flags = STRING_FLAGS::NONE) override;
	StringBlock GetStringBlock(ieStrRef strref, STRING_FLAGS flags = STRING_FLAGS::NONE) override;
	bool HasAltTLK() const override;
private:
	/** resolves day and monthname tokens */
	void GetMonthName(int dayandmonth);
	String ResolveTags(const String& source);
	String BuiltinToken(const ieVariable& Token);
	ieStrRef ClassStrRef(int slot) const;
	ieStrRef RaceStrRef(int slot) const;
	ieStrRef GenderStrRef(int slot, ieStrRef malestrref, ieStrRef femalestrref) const;
	String Gabber() const;
	String CharName(int slot) const;
};

}

#endif
