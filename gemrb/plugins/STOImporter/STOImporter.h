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

#ifndef STOIMP_H
#define STOIMP_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "../../includes/ie_types.h"
#include "../Core/Store.h"
#include "../Core/StoreMgr.h"


class STOImp : public StoreMgr {
private:
	DataStream* str;
	bool autoFree;
	int version;

public:
	STOImp(void);
	~STOImp(void);
	bool Open(DataStream* stream, bool autoFree = true);
	Store* GetStore(Store *store);

	//returns saved size, updates internal offsets before save
	int GetStoredFileSize(Store *st);
	//saves file
	int PutStore(DataStream *stream, Store *store);

	void release(void)
	{
		delete this;
	}
private:
	void GetItem(STOItem *item);
	void GetDrink(STODrink *drink);
	void GetCure(STOCure *cure);
	void GetPurchasedCategories(Store* s);

	int PutItems(DataStream *stream, Store* s);
	int PutDrinks(DataStream *stream, Store* s);
	int PutCures(DataStream *stream, Store* s);
	int PutPurchasedCategories(DataStream *stream, Store* s);
	int PutHeader(DataStream *stream, Store *store);
};


#endif
