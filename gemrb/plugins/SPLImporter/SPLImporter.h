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
