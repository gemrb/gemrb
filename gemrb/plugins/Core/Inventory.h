/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Inventory.h,v 1.4 2004/04/07 20:12:31 avenger_teambg Exp $
 *
 */

/* Class implementing creature's inventory and (maybe) item management */

#ifndef INVENTORY_H
#define INVENTORY_H

#include <vector>
#include "../../includes/win32def.h"
#include "../../includes/ie_types.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

const int INVENTORY_SIZE = 38;


typedef enum ieCREItemFlagBits {
	IE_ITEM_IDENTIFIED = 1,
	IE_ITEM_UNSTEALABLE = 2,
	IE_ITEM_STOLEN = 4,
	IE_ITEM_STACKED = 0x100, //this is a hack, refresh it for stacked items
} ieCREItemFlagBits;


typedef struct CREItem {
	ieResRef  ItemResRef;
	ieWord Unknown08;
	ieWord Usages[3];
	ieDword Flags;
} CREItem;



class GEM_EXPORT Inventory {
private:
	std::vector<CREItem*> slots;
	//CREItem* slots[38];

public: 
	Inventory();
	virtual ~Inventory();

	/** returns the count items in the inventory */
	int CountItems(const char *resref, bool charges);
	/** looks for a particular item in a slot */
	bool HasItemInSlot(const char *resref, int slot);
	/** looks for a particular item in the inventory */
	/** flags: 1 - equipped, 2 - identified */
	bool HasItem(const char *resref, int flags);

	/** returns number of all slots in the inventory */
	int GetSlotCount() { return slots.size(); };

	/** returns CREItem in specified slot. if count != -1 it
	** splits the item and returns only requuested amount */
	CREItem* GetItem(int slot, int count = -1) { return NULL; };

	/** adds CREItem to the inventory. If slot == -1, finds
	** first eligible slot, eventually splitting the item to
	** more slots. Returns true if successfull.
	** FIXME: it should allow for cases when part of the item's amount
	** can go in and part can't. Maybe return number instead? or CRE?*/
	int AddSlotItem(CREItem* item, int slot, CREItem** res_item);

	void DestroyItem(const char *resref, int flags);
	void SetSlotItem(CREItem* item, int slot);

	/** returns weight of whole inventory, i.e. encumbrance? */
	/** FIXME: but what about IWD2 containers? */
	int GetWeight() {return 0;};

	bool ItemsAreCompatible(CREItem* target, CREItem* source);

	void dump();
};

#endif
