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

#include "ie_types.h"

#include "Store.h"

namespace GemRB {


class STOImporter : public StoreMgr {
private:
	DataStream* str;
	int version;

public:
	STOImporter(void);
	~STOImporter(void);
	bool Open(DataStream* stream);
	Store* GetStore(Store *store);

	//returns saved size, updates internal offsets before save
	void CalculateStoredFileSize(Store *st);
	//saves file
	bool PutStore(DataStream *stream, Store *store);

private:
	void GetItem(STOItem *item, Store* s);
	void GetDrink(STODrink *drink);
	void GetCure(STOCure *cure);
	void GetPurchasedCategories(Store* s);

	void PutItems(DataStream *stream, Store* s);
	void PutDrinks(DataStream *stream, Store* s);
	void PutCures(DataStream *stream, Store* s);
	void PutPurchasedCategories(DataStream *stream, Store* s);
	void PutHeader(DataStream *stream, Store *store);
};


}

#endif
