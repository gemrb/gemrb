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

#ifndef STOIMPORTER_H
#define STOIMPORTER_H

#include "StoreMgr.h"

namespace GemRB {


class STOImporter : public StoreMgr {
private:
	DataStream* str = nullptr;
	int version = 0;

public:
	STOImporter() noexcept = default;
	STOImporter(const STOImporter&) = delete;
	~STOImporter() override;
	STOImporter& operator=(const STOImporter&) = delete;
	bool Open(DataStream* stream) override;
	Store* GetStore(Store* store) override;

	//returns saved size, updates internal offsets before save
	void CalculateStoredFileSize(Store* st) const;
	//saves file
	bool PutStore(DataStream* stream, Store* store) override;

private:
	void GetItem(STOItem* item, const Store* s);
	void GetDrink(STODrink* drink);
	void GetCure(STOCure* cure);
	void GetPurchasedCategories(Store* s);

	void PutItems(DataStream* stream, const Store* s) const;
	void PutDrinks(DataStream* stream, const Store* s) const;
	void PutCures(DataStream* stream, const Store* s) const;
	void PutPurchasedCategories(DataStream* stream, const Store* s) const;
	void PutHeader(DataStream* stream, const Store* store);
};


}

#endif
