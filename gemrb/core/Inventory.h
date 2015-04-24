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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
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

#include "exports.h"
#include "ie_types.h"
#include "win32def.h"

#include "Item.h"  //needs item for itmextheader
#include "Store.h"

#include <vector>

namespace GemRB {

class Map;
class StringBuffer;

//AddSlotItem return values
#define ASI_FAILED     0
#define ASI_PARTIAL    1
#define ASI_SUCCESS    2
#define ASI_SWAPPED    3 //not returned normally, but Gui uses this value

//AddSlotItem extra slot ID's
#define SLOT_AUTOEQUIP     -1
#define SLOT_ONLYINVENTORY -3

//slottypes (bitfield)
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
#define SLOT_ANY       32767
#define SLOT_INVENTORY 32768
#define SLOT_ALL       65535

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
	IE_INV_ITEM_STOLEN = 4, // denotes steel items in pst
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
	IE_INV_ITEM_RESELLABLE = 0x800, //item will appear in shop when sold
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

#define IE_INV_DEPLETABLE (IE_INV_ITEM_MAGICAL|IE_INV_ITEM_DESTRUCTIBLE)

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
	ieStrRef Tooltip;
	ieWord Charges;

	//from itmextheader
	ieByte AttackType;
	ieByte IDReq;
	ieByte Location;
	ieByte unknown1;
	ieResRef UseIcon;
	ieByte Target;
	ieByte TargetNumber;
	ieWord Range;
	//This was commented out in ITMExtHeader
	//ieWord ProjectileType;
	ieWord Speed;
	ieWord THAC0Bonus;
	ieWord DiceSides;
	ieWord DiceThrown;
	ieWordSigned DamageBonus; //this must be signed!!!
	ieWord DamageType;
	ieWord FeatureCount;
	ieWord FeatureOffset;
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
	//recent research showed that this field is used by the create item
	//for days effect. This field shows the expiration in gametime hours
	ieWord Expired;
	ieWord Usages[CHARGE_COUNTERS];
	ieDword Flags;
	// 2 cached values from associated item. LEAVE IT SIGNED!
	/** Weight of each item in the stack */
	int Weight;
	/** Maximum amount of items in this stack */
	int MaxStackAmount;

	CREItem()
	{
		Weight=-1; //invalid weight
		MaxStackAmount=0;
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
	/// Flag indicating whether weight needs to be recalculated
	mutable int Changed;
	/** Total weight of all items in Inventory */
	mutable int Weight;

	ieWordSigned Equipped;
	ieWord EquippedHeader;
	/** this isn't saved */
	ieDword ItemExcl;
	ieDword ItemTypes[8]; //256 bits
public: 
	Inventory();
	virtual ~Inventory();

	/** duplicates the source inventory into the current one, marking items as undroppable */
	void CopyFrom(const Actor *source);
	/** Removes an item from the inventory, destroys slot.
	 * Use it for containers only */
	CREItem *GetItem(unsigned int idx);
	/** adds an item to the inventory */
	void AddItem(CREItem *item);
	/** Returns number of items in the inventory */
	int CountItems(const char *resref, bool charges) const;
	/** looks for a particular item in a slot */
	bool HasItemInSlot(const char *resref, unsigned int slot) const;
	/** returns true if contains one itemtype equipped */
	bool HasItemType(ieDword type) const;
	/** Looks for a particular item in the inventory.
	 * flags: see ieCREItemFlagBits */
	bool HasItem(const char *resref, ieDword flags) const;

	void CalculateWeight(void) const;
	void SetInventoryType(int arg);
	void SetOwner(Actor* act) { Owner = act; }

	/** returns number of all slots in the inventory */
	int GetSlotCount() const { return (int)Slots.size(); }

	/** sets inventory size, for the first time */
	void SetSlotCount(unsigned int size);


	/** Returns CREItem in specified slot. 
	 * If count !=0 it splits the item and returns only requested amount */
	CREItem* RemoveItem(unsigned int slot, unsigned int count = 0);
	/** returns slot of removed item, you can delete the removed item */
	int RemoveItem(const char* resref, unsigned int flags, CREItem **res_item, int count = 0);

	/** adds CREItem to the inventory. If slot == -1, finds
	** first eligible slot, eventually splitting the item to
	** more slots. If slot == -3 then finds the first empty inventory slot
	** Returns 2 if completely successful, 1 if partially, 0 else.
	** slottype is an optional filter for searching eligible slots */
	int AddSlotItem(CREItem* item, int slot, int slottype=-1);
	/** tries to equip all inventory items in a given slot */
	void TryEquipAll(int slot);
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
	int GetWeight() const {return Weight;}

	bool ItemsAreCompatible(CREItem* target, CREItem* source) const;
	//depletes charged items
	int DepleteItem(ieDword flags);
	//charges recharging items
	void ChargeAllItems(int hours);
	/** Finds the first slot of named item, if resref is empty, finds the first filled! slot */
	int FindItem(const char *resref, unsigned int flags, unsigned int skip=0) const;
	bool DropItemAtLocation(unsigned int slot, unsigned int flags, Map *map, const Point &loc);
	bool DropItemAtLocation(const char *resref, unsigned int flags, Map *map, const Point &loc);
	bool SetEquippedSlot(ieWordSigned slotcode, ieWord header);
	int GetEquipped() const;
	int GetEquippedHeader() const;
	ITMExtHeader *GetEquippedExtHeader(int header=0) const;
	void SetEquipped(ieWordSigned slot, ieWord header);
	//right hand
	int GetEquippedSlot() const;
	//left hand
	int GetShieldSlot() const;
	void AddSlotEffects( ieDword slot);
	//void AddAllEffects();
	/** Returns item in specified slot. Does NOT change inventory */
	CREItem* GetSlotItem(ieDword slot) const;
	/** Returns the item's inventory flags */
	ieDword GetItemFlag(unsigned int slot) const;
	/** Changes the inventory flags */
	bool ChangeItemFlag(ieDword slot, ieDword value, int mode);
	/** Equips the item, don't use it directly for weapons */
	bool EquipItem(ieDword slot);
	bool UnEquipItem(ieDword slot, bool removecurse);
	/** Returns equipped weapon, also its slot */
	CREItem *GetUsedWeapon(bool leftorright, int &slot) const;
	/** returns slot of launcher weapon currently equipped */
	int FindRangedWeapon() const; 
	/** returns slot of launcher weapon for specified projectile type */
	int FindTypedRangedWeapon(unsigned int type) const;
	/** returns slot of launcher weapon for projectile in specified slot */
	int FindSlotRangedWeapon(ieDword slot) const;
	/** Returns a slot which might be empty, or capable of holding item (or part of it) */
	int FindCandidateSlot(int slottype, size_t first_slot, const char *resref = NULL);
	/** Creates an item in the slot*/
	void SetSlotItemRes(const ieResRef ItemResRef, int Slot, int Charge0=1, int Charge1=0, int Charge2=0);
	/** Adds item to slot*/
	void AddSlotItemRes(const ieResRef ItemResRef, int Slot, int Charge0=1, int Charge1=0, int Charge2=0);
	/** returns the itemtype held in the left hand */
	ieWord GetShieldItemType() const;
	/** returns the itemtype of the item in the armor slot, mostly used in IWD2 */
	ieWord GetArmorItemType() const;
	/** breaks the item (weapon) in slot */
	void BreakItemSlot(ieDword slot);
	/** Lists all items in the Inventory on terminal for debugging */
	void dump() const;
	/// List all items in the Inventory to the given buffer */
	void dump(StringBuffer&) const;
	/** Equips best weapon */
	void EquipBestWeapon(int flags);
	/** returns the struct of the usable items, returns true if there are more */
	bool GetEquipmentInfo(ItemExtHeader *array, int startindex, int count);
	/** returns the exclusion bits */
	ieDword GetEquipExclusion(int index) const;
	/** returns if a slot is temporarily blocked */
	bool IsSlotBlocked(int slot) const;
	/** returns true if a two handed weapon is in slot */
	inline bool TwoHandedInSlot(int slot) const;
	/** returns the strref for the reason why the item cannot be equipped */
	int WhyCantEquip(int slot, int twohanded) const;
	/** returns a slot that has a stealable item */
	unsigned int FindStealableItem();
	/** checks if any equipped item provides critical hit aversion */
	bool ProvidesCriticalAversion();
	/** tries to merge the passed item with the one in the passed slot */
	int MergeItems(int slot, CREItem *item);
	bool FistsEquipped() const;
	//setting important constants
	static void Init();
	static void SetArmorSlot(int arg);
	static void SetHeadSlot(int arg);
	static void SetFistSlot(int arg);
	static void SetMagicSlot(int arg);
	static void SetWeaponSlot(int arg);
	static void SetRangedSlot(int arg);
	static void SetQuickSlot(int arg);
	static void SetInventorySlot(int arg);
	static void SetShieldSlot(int arg);
	static int GetArmorSlot();
	static int GetHeadSlot();
	static int GetFistSlot();
	static int GetMagicSlot();
	static int GetWeaponSlot();
	static int GetWeaponQuickSlot(int weaponslot);
	static int GetWeaponSlot(int quickslot);
	static int GetRangedSlot();
	static int GetQuickSlot();
	static int GetInventorySlot();
private:
	int FindRangedProjectile(unsigned int type) const;
	// called by KillSlot
	void RemoveSlotEffects( /*CREItem* slot*/ ieDword slot );
	void KillSlot(unsigned int index);
	inline Item *GetItemPointer(ieDword slot, CREItem *&Slot) const;
	void UpdateWeaponAnimation();
	void UpdateShieldAnimation(Item *it);
};

}

#endif
