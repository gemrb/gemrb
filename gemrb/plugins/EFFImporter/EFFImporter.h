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

#ifndef EFFIMPORTER_H
#define EFFIMPORTER_H

#include "EffectMgr.h"

namespace GemRB {

class EFFImporter : public EffectMgr {
private:
	DataStream* str = nullptr;
	bool autoFree = false;
	int version = 0;

public:
	EFFImporter() noexcept = default;
	EFFImporter(const EFFImporter&) = delete;
	~EFFImporter() override;
	EFFImporter& operator=(const EFFImporter&) = delete;
	// We need this autoFree, since Effects are included inline
	// in other file types, without a size header.
	bool Open(DataStream* stream, bool autoFree = true) override;
	Effect* GetEffect() override;
	Effect* GetEffectV1() override;
	Effect* GetEffectV20() override;
	void PutEffectV2(DataStream* stream, const Effect* fx) override; // used in the area and cre importer
};


}

#endif
