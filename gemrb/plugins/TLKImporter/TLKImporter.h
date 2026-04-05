// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
	std::unique_ptr<CTlkOverride> OverrideTLK;
	ResRefMap<gt_type> gtmap;
	int charname = 0;
	bool hasEndingNewline = false;

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
	ieStrRef GetNextStrRef() const final { return OverrideTLK->GetNextStrRef(); };

private:
	/** resolves day and monthname tokens */
	void GetMonthName(int dayandmonth);
	String ResolveTags(const String& source);
	String BuiltinToken(const ieVariable& Token);
	String ExternalToken(const ieVariable& token) const;
	ieStrRef ClassStrRef(int slot) const;
	ieStrRef RaceStrRef(int slot) const;
	ieStrRef GenderStrRef(int slot, ieStrRef malestrref, ieStrRef femalestrref) const;
	String Gabber() const;
	String CharName(int slot) const;
};

}

#endif
