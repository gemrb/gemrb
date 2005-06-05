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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/STOImporter/STOImp.h,v 1.5 2005/06/05 10:55:00 avenger_teambg Exp $
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

	int PutItem(DataStream *stream, STOItem *item);
	int PutDrink(DataStream *stream, STODrink *drink);
	int PutCure(DataStream *stream, STOCure *cure);
	int PutPurchasedCategories(DataStream *stream, Store* s);
	int PutHeader(DataStream *stream, Store *store);
};


#endif
