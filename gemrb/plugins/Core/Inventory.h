/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Inventory.h,v 1.59 2006/12/30 23:47:26 avenger_teambg Exp $
 *
 */

/**
 * @file Inventory.h
 * Declares Inventory, class implementing creatures' and containers' 
 * inventory and item management 
 * @author The GemRB Project
 */

#ifndef INVENTORY_H
#define INVENTORY_H

#include <vector>
#include "../../includes/win32def.h"
#include "../../includes/ie_types.h"

#include "Store.h"
#include "Item.h"  //needs item for itmextheader
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
#define SLOT_SCROLL    4096
#define SLOT_BAG       8192
#define SLOT_POTION    16384

#define SLOT_INVENTORY 0x8000
#define SLOT_ANY       -1

//weapon slot types (1000==not equipped)
#define IW_NO_EQUIPPED  1000

/** Inventory types */
typedef enum ieInventoryType {
	INVENTORY_HEAP = 0,
	INVENTORY_CREATURE = 1
} ieInventoryType;

// !!! Keep these synchronized with GUIDefines.py !!!
typedef enum ieCREItemFlagBits {
	IE_INV_ITEM_IDENTIFIED = 1,
	IE_INV_ITEM_UNSTEALABLE = 2,
	IE_INV_ITEM_STOLEN = 4,
	//in iwd/iwd2 this flag means 'magical', some hack is needed
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
	IE_INV_ITEM_STACKED = 0x80,	//this is a gemrb extension
	//these flags are coming from the original item, but these are immutable
	IE_INV_ITEM_CRITICAL = 0x100, //coming from original item
	IE_INV_ITEM_TWOHANDED = 0x200,
	IE_INV_ITEM_MOVABLE = 0x400, //same as undroppable
	IE_INV_ITEM_UNKNOWN800 = 0x800, //displayable in shop???
	IE_INV_ITEM_CURSED = 0x1000, //item is cursed
	IE_INV_ITEM_UNKNOWN2000 = 0x2000, //totally unknown
	IE_INV_ITEM_MAGICAL = 0x4000, //magical
	IE_INV_ITEM_BOW = 0x8000, //
	IE_INV_ITEM_SILVER = 0x10000,
	IE_INV_ITEM_COLDIRON = 0x20000,
	IE_INV_ITEM_STOLEN2 = 0x40000, //same as 4
	IE_INV_ITEM_CONVERSIBLE = 0x80000,
	IE_INV_ITEM_PULSATING = 0x100000
} ieCREItemFlagBits;

//equip flags
#define EQUIP_ANY   0
#define EQUIP_MELEE 1
#define EQUIP_RANGED 2

//FIXME:
//actually this header shouldn't be THIS large, i was just
//lazy to pick the interesting elements
//it could be possible that some elements need to be added from the
//item header itself
struct ItemExtHeader {
	ieDword slot;
	ieDword headerindex;
	//from itmextheader
	ieByte AttackType;
	ieByte IDReq;
	ieByte Location;
	ieByte unknown1;
	ieResRef UseIcon;
	ieByte Target;
	ieByte TargetNumber;
	ieWord Range;
	ieWord ProjectileType;
	ieWord Speed;
	ieWord THAC0Bonus;
	ieWord DiceSides;
	ieWord DiceThrown;
	ieWord DamageBonus;
	ieWord DamageType;
	ieWord FeatureCount;
	ieWord FeatureOffset;
	ieWord Charges;
	ieWord ChargeDepletion;
	ieDword RechargeFlags; //this is a bitfield with many bits
	ieWord ProjectileAnimation;
	ieWord MeleeAnimation[3];
	int ProjectileQualifier; //this is a derived value determined on load time
	//other data
	ieResRef itemname;
};

/**
 * @class CREItem
 * Class holding Item instance specific values and providing link between 
 * an Inventory and a stack of Items.
 * It's keeping info on whether Item was identified, for example.
 */

class GEM_EXPORT CREItem {
public:
	ieResRef ItemResRef;
	ieWord PurchasedAmount; //actually, this field is useless in creatures
	ieWord Usages[3];
	ieDword Flags;
	// 2 cached values from associated item. LEAVE IT SIGNED!
	/** Weight of items in the stack */
	int Weight;
	/** Amount of items in this stack */
	int StackAmount;

	CREItem()
	{
		Weight=-1; //invalid weight
		StackAmount=0;
	}
};

/**
 * @class Inventory
 * Class implementing creatures' and containers' inventory and item management
 */

class GEM_EXPORT Inventory {
private:
	std::vector<CREItem*> Slots;
	Actor* Owner;
	int InventoryType;
	int Changed;
	/** Total weight of all items in Inventory */
	int Weight;

	int Equipped;
	/** this isn't saved */
	ieDword ItemExcl;
public: 
	Inventory();
	virtual ~Inventory();
	
	/** Removes an item from the inventory, destroys slot.
	 * Use it for containers only */
	CREItem *GetItem(unsigned int idx);
	/** adds an item to the inventory */
	void AddItem(CREItem *item);
	/** Returns number of items in the inventory */
	int CountItems(const char *resref, bool charges);
	/** looks for a particular item in a slot */
	bool HasItemInSlot(const char *resref, unsigned int slot);
	/** Looks for a particular item in the inventory. */
	/* flags: see ieCREItemFlagBits */
	bool HasItem(const char *resref, ieDword flags);

	void CalculateWeight(void);
	void SetInventoryType(int arg);
	void SetOwner(Actor* act) { Owner = act; };

	/** returns number of all slots in the inventory */
	int GetSlotCount() { return (int)Slots.size(); };

	/** sets inventory size, for the first time */
	void SetSlotCount(unsigned int size);


	/** Returns CREItem in specified slot. 
	 * If count !=0 it splits the item and returns only requested amount */
	CREItem* RemoveItem(unsigned int slot, unsigned int count = 0);
	/** returns slot of removed item, you can delete the removed item */
	int RemoveItem(const char* resref, unsigned int flags, CREItem **res_item);

	/** adds CREItem to the inventory. If slot == -1, finds
	** first eligible slot, eventually splitting the item to
	** more slots. If slot == -3 then finds the first empty inventory slot
	** Returns 2 if completely successful, 1 if partially, 0 else.
	** slottype is an optional filter for searching eligible slots */
	int AddSlotItem(CREItem* item, int slot, int slottype=-1);
	/** Adds STOItem to the inventory, it is never wielded, action might be STA_STEAL or STA_BUY */
	/** The amount of items is stored in PurchasedAmount */
	int AddStoreItem(STOItem* item, int action);

	/** flags: see ieCREItemFlagBits */
	/** count == ~0 means to destroy all */
	/** returns the number of destroyed items */
	unsigned int DestroyItem(const char *resref, ieDword flags, ieDword count);
	/** flags: see ieCREItemFlagBits */
	void SetItemFlags(CREItem* item, ieDword flags);
	void SetSlotItem(CREItem* item, unsigned int slot);
	int GetWeight() {return Weight;}

	bool ItemsAreCompatible(CREItem* target, CREItem* source);
	//depletes charged items
	int DepleteItem(ieDword flags);
	/** Finds the first slot of named item, if resref is empty, finds the first filled! slot */
	int FindItem(const char *resref, unsigned int flags);
	bool DropItemAtLocation(unsigned int slot, unsigned int flags, Map *map, Point &loc);
	bool DropItemAtLocation(const char *resref, unsigned int flags, Map *map, Point &loc);
	bool SetEquippedSlot(int slotcode);
	int GetEquipped();
	//right hand
	int GetEquippedSlot();
	//left hand
	int GetShieldSlot();
	void AddSlotEffects( CREItem* slot, int type );
	//void AddAllEffects();
	/** Returns item in specified slot. Does NOT change inventory */
	CREItem* GetSlotItem(unsigned int slot);
	bool ChangeItemFlag(unsigned int slot, ieDword value, int mode);
	bool EquipItem(unsigned int slot);
	bool UnEquipItem(unsigned int slot, bool removecurse);
	/** Returns equipped weapon */
	CREItem *GetUsedWeapon(bool leftorright);
	/** returns slot of launcher weapon currently equipped */
	int FindRangedWeapon(); 
	/** returns slot of launcher weapon for specified projectile type */
	int FindTypedRangedWeapon(unsigned int type);
	/** returns slot of launcher weapon for projectile in specified slot */
	int FindSlotRangedWeapon(unsigned int slot);
	/** Returns a slot which might be empty, or capable of holding item (or part of it) */
	int FindCandidateSlot(int slottype, size_t first_slot, const char *resref = NULL);
	/** Creates an item in the slot*/
	void SetSlotItemRes(const ieResRef ItemResRef, int Slot, int Charge0=1, int Charge1=0, int Charge2=0);
	/** Adds item to slot*/
	void AddSlotItemRes(const ieResRef ItemResRef, int Slot, int Charge0=1, int Charge1=0, int Charge2=0);
	/** breaks the item (weapon) in slot */
	void BreakItemSlot(ieDword slot);
	/** Lists all items in the Inventory on terminal for debugging */
	void dump();
	/** Equips best weapon */
	void EquipBestWeapon(int flags);
	/** returns the struct of the usable items, returns true if there are more */
	bool GetEquipmentInfo(ItemExtHeader *array, int startindex, int count);
	/** uses the item in the given slot */
	bool UseItem(unsigned int slotindex, unsigned int headerindex, Actor *target);
	/** returns the exclusion bits */
	ieDword GetEquipExclusion() const;
	/** returns if a slot is temporarily blocked */
	bool IsSlotBlocked(int slot);
	//setting important constants
	static void Init(int mb);
	static void SetFistSlot(int arg);
	static void SetMagicSlot(int arg);
	static void SetWeaponSlot(int arg);
	static void SetRangedSlot(int arg);
	static void SetQuickSlot(int arg);
	static void SetInventorySlot(int arg);
	static void SetShieldSlot(int arg);
	static int GetFistSlot();
	static int GetMagicSlot();
	static int GetWeaponSlot();
	static int GetRangedSlot();
	static int GetQuickSlot();
	static int GetInventorySlot();
private:
	int FindRangedProjectile(unsigned int type);
	// called by KillSlot
	void RemoveSlotEffects( CREItem* slot );
	void KillSlot(unsigned int index);
	inline Item *GetItemPointer(ieDword slot, CREItem *&Slot);
	void UpdateWeaponAnimation();
	void UpdateShieldAnimation(Item *it);
};

#endif
