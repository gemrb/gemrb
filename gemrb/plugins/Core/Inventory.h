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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Inventory.h,v 1.14 2004/05/05 19:13:03 avenger_teambg Exp $
 *
 */

/* Class implementing creature's inventory and (maybe) item management */

#ifndef INVENTORY_H
#define INVENTORY_H

#include <vector>
#include "../../includes/win32def.h"
#include "../../includes/ie_types.h"

#include "Store.h"
class Map;

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

typedef enum ieInventoryType {
	INVENTORY_HEAP = 0,
	INVENTORY_CREATURE = 1,
} ieInventoryType;

typedef enum ieCREItemFlagBits {
	IE_ITEM_IDENTIFIED = 1,
	IE_ITEM_UNSTEALABLE = 2,
	IE_ITEM_STOLEN = 4,
	IE_ITEM_UNDROPPABLE =8,
	//just recently acquired
	IE_ITEM_ACQUIRED = 0x10,	//this is a gemrb extension
	//is this item destructible normally?
	IE_ITEM_DESTRUCTIBLE = 0x20,    //this is a gemrb extension
	//is this item already equipped?
	IE_ITEM_EQUIPPED = 0x40,	//this is a gemrb extension 
	//is this item stackable?
	IE_ITEM_STACKED = 0x80,	        //this is a gemrb extension
} ieCREItemFlagBits;

typedef struct CREItem {
	ieResRef  ItemResRef;
	ieWord Unknown08;
	ieWord Usages[3];
	ieDword Flags;
} CREItem;

class GEM_EXPORT Inventory {
private:
	std::vector<CREItem*> Slots;
	int InventoryType;
	int Changed;
	int Weight;

public: 
	Inventory();
	virtual ~Inventory();

	/** adds an item to the inventory */
	void AddItem(CREItem *item);
	/** returns the count items in the inventory */
	int CountItems(const char *resref, bool charges);
	/** looks for a particular item in a slot */
	bool HasItemInSlot(const char *resref, int slot);
	/** looks for a particular item in the inventory */
	/** flags: see ieCREItemFlagBits */
	bool HasItem(const char *resref, ieDword flags);

	void CalculateWeight(void);
	void SetInventoryType(int arg);

	/** returns number of all slots in the inventory */
	int GetSlotCount() { return Slots.size(); };

	/** sets inventory size, for the first time */
	void SetSlotCount(unsigned int size);


	/** returns CREItem in specified slot. if count !=0 it
	** splits the item and returns only requuested amount */
	CREItem* RemoveItem(unsigned int slot, unsigned int count = 0);

	/** adds CREItem to the inventory. If slot == -1, finds
	** first eligible slot, eventually splitting the item to
	** more slots. Returns true if successful.
	** FIXME: it should allow for cases when part of the item's amount
	** can go in and part can't. Maybe return number instead? or CRE?*/
	int AddSlotItem(CREItem* item, unsigned int slot, CREItem** res_item);

	int AddSlotItem(STOItem* item, unsigned int slot, CREItem** res_item, int count);

	/** flags: see ieCREItemFlagBits */
	void DestroyItem(const char *resref, ieDword flags);
	/** flags: see ieCREItemFlagBits */
	void SetItemFlags(CREItem* item, ieDword flags);
	void SetSlotItem(CREItem* item, unsigned int slot);
	int GetWeight() {return Weight;}

	bool ItemsAreCompatible(CREItem* target, CREItem* source);
	void DropItemAtLocation(const char *resref, unsigned int flags, Map *map, unsigned short x, unsigned short y);

	void dump();
};

#endif
