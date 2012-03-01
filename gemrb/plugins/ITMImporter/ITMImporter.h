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

#ifndef ITMIMPORTER_H
#define ITMIMPORTER_H

#include "ItemMgr.h"

#include "ie_types.h"

#include "Item.h"

namespace GemRB {

#define ITM_VER_BG 10
#define ITM_VER_PST 11
#define ITM_VER_IWD2 20

class ITMImporter : public ItemMgr {
private:
	DataStream* str;
	int version;

public:
	ITMImporter(void);
	~ITMImporter(void);
	bool Open(DataStream* stream);
	Item* GetItem(Item *s);
private:
	void GetExtHeader(Item *s, ITMExtHeader* eh);
	void GetFeature(Effect *f);
};


}

#endif
