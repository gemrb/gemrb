// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef SPLIMPORTER_H
#define SPLIMPORTER_H

#include "SpellMgr.h"

namespace GemRB {


class SPLImporter : public SpellMgr {
private:
	DataStream* str = nullptr;
	int version = 0;

public:
	SPLImporter() noexcept = default;
	SPLImporter(const SPLImporter&) = delete;
	~SPLImporter() override;
	SPLImporter& operator=(const SPLImporter&) = delete;
	bool Open(DataStream* stream) override;
	Spell* GetSpell(Spell* spl, bool silent = false) override;

private:
	void GetExtHeader(const Spell* s, SPLExtHeader* eh);
	Effect* GetFeature(const Spell* s);
};


}

#endif
