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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Inventory.h,v 1.33 2005/06/17 19:33:06 avenger_teambg Exp $
 *
 */

/* Class implementing creatures' and containers' inventory and item management */

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

//slottypes
#define SLOT_HELM      1
#define SLOT_ARMOUR    2
#define SLOT_SHIELD    4
#define SLOT_GLOVE     8
#define SLOT_RING      16
#define SLOT_AMULET    32
#define SLOT_BELT      64
#define SLOT_BOOT      128
#define SLOT_WEAPON    256
#define SLOT_QUIVER    512
#define SLOT_CLOAK     1024
#define SLOT_ITEM      2048  //quick item
//
#define SLOT_INVENTORY 0x8000
#define SLOT_ANY       -1

//weapon slot types (1000==not equipped)
#define IW_NO_EQUIPPED  1000
#define IW_QUIVER        -22   // (-22, -23, -24)

typedef enum ieInventoryType {
	INVENTORY_HEAP = 0,
	INVENTORY_CREATURE = 1
} ieInventoryType;

// !!! Keep these synchronized with GUIDefines.py !!!
typedef enum ieCREItemFlagBits {
	IE_INV_ITEM_IDENTIFIED = 1,
	IE_INV_ITEM_UNSTEALABLE = 2,
	IE_INV_ITEM_STOLEN = 4,
	IE_INV_ITEM_UNDROPPABLE =8,
	//just recently acquired
	IE_INV_ITEM_ACQUIRED = 0x10,	//this is a gemrb extension
	//is this item destructible normally?
	IE_INV_ITEM_DESTRUCTIBLE = 0x20,//this is a gemrb extension
	//is this item already equipped?
	IE_INV_ITEM_EQUIPPED = 0x40,	//this is a gemrb extension
	//selected for sale, using the same bit, hope it is ok
	IE_INV_ITEM_SELECTED = 0x40,    //this is a gemrb extension
	//is this item stackable?
	IE_INV_ITEM_STACKED = 0x80	//this is a gemrb extension
} ieCREItemFlagBits;

class GEM_EXPORT CREItem {
public:
	ieResRef ItemResRef;
	ieWord PurchasedAmount; //actually, this field is useless in creatures
	ieWord Usages[3];
	ieDword Flags;
	// 2 cached values from associated item. LEAVE IT SIGNED!
	int Weight;
	int StackAmount;

	CREItem()
	{
		Weight=0;
		StackAmount=0;
	}
};

class GEM_EXPORT Inventory {
private:
	std::vector<CREItem*> Slots;
	int InventoryType;
	int Changed;
	int Weight;

	ieDword Equipped;

public: 
	Inventory();
	virtual ~Inventory();
	
	/** removes an item from the inventory, destroys slot
	    use it only for containers */
	CREItem *GetItem(size_t idx);
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
	int GetSlotCount() { return (int)Slots.size(); };

	/** sets inventory size, for the first time */
	void SetSlotCount(unsigned int size);


	/** returns CREItem in specified slot. if count !=0 it
	** splits the item and returns only requested amount*/
	CREItem* RemoveItem(unsigned int slot, unsigned int count = 0);
	/** returns slot of removed item, you can delete the removed item */
	int RemoveItem(const char* resref, unsigned int flags, CREItem **res_item);

	/** adds CREItem to the inventory. If slot == -1, finds
	** first eligible slot, eventually splitting the item to
	** more slots. Returns 2 if completely successful, 1 if partially, 0 else.*/
	int AddSlotItem(CREItem* item, int slot);
	/** Adds STOItem to the inventory, it is never wielded, action might be STA_STEAL or STA_BUY */
	/** The amount of items is stored in PurchasedAmount */
	int AddSlotItem(STOItem* item, int action);

	/** flags: see ieCREItemFlagBits */
	/** count == ~0 means to destroy all */
	/** returns the number of destroyed items */
	unsigned int DestroyItem(const char *resref, ieDword flags, ieDword count);
	/** flags: see ieCREItemFlagBits */
	void SetItemFlags(CREItem* item, ieDword flags);
	void SetSlotItem(CREItem* item, unsigned int slot);
	int GetWeight() {return Weight;}

	bool ItemsAreCompatible(CREItem* target, CREItem* source);
	/*finds the first slot of named item, if resref is empty, finds the first filled! slot*/
	int FindItem(const char *resref, unsigned int flags);
	void DropItemAtLocation(const char *resref, unsigned int flags, Map *map, Point &loc);
	void SetEquippedSlot(ieDword slotcode)  { Equipped = slotcode; }
	ieDword GetEquippedSlot()  { return Equipped; }
	// Returns item in specified slot. Does NOT change inventory
	CREItem* GetSlotItem(unsigned int slot);
	// Returns equipped weapon
	CREItem *GetUsedWeapon();
	// Returns a slot which might be empty, or capable of holding item (or part of it)
	int FindCandidateSlot(int slottype, size_t first_slot, const char *resref = NULL);

	void dump();
private:
	int FindRanged();
	void KillSlot(unsigned int index);
};

#endif
