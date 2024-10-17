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

//This class represents the inventory of stores (.sto), area containers (.are)
//or actors (.cre).

#include "Inventory.h"

#include "strrefs.h"

#include "CharAnimations.h"
#include "DisplayMessage.h"
#include "Game.h"
#include "GameData.h"
#include "Interface.h"
#include "Item.h"
#include "Map.h"
#include "ScriptEngine.h"

#include "GameScript/GSUtils.h"
#include "Scriptable/Actor.h"
#include "fmt/ranges.h"

#include <cstdio>

namespace GemRB {

static int SLOT_HEAD = -1;
static int SLOT_MAGIC = -1;
static int SLOT_FIST = -1;
static int SLOT_MELEE = -1;
static int LAST_MELEE = -1;
static int SLOT_RANGED = -1;
static int LAST_RANGED = -1;
static int SLOT_QUICK = -1;
static int LAST_QUICK = -1;
static int SLOT_INV = -1;
static int LAST_INV = -1;
static int SLOT_LEFT = -1;
static int SLOT_ARMOR = -1;

//IWD2 style slots
static bool IWD2 = false;

[[noreturn]]
static void InvalidSlot(int slot)
{
	error("Inventory", "Invalid slot: {}!", slot);
}

void ItemExtHeader::CopyITMExtHeader(const ITMExtHeader &src)
{
	AttackType = src.AttackType;
	UseIcon = src.UseIcon;
	Tooltip = src.Tooltip;
	Target = src.Target;
	TargetNumber = src.TargetNumber;
	Range = src.Range;
	Charges = src.Charges;
	ChargeDepletion = src.ChargeDepletion;
	ProjectileAnimation = src.ProjectileAnimation;
}

//This inline function returns both an item pointer and the slot data.
//slot is a dynamic slot number (SLOT_*)
inline Item *Inventory::GetItemPointer(ieDword slot, CREItem *&item) const
{
	item = GetSlotItem(slot);
	if (!item) return NULL;
	if (item->ItemResRef.IsEmpty()) return nullptr;
	return gamedata->GetItem(item->ItemResRef);
}

void Inventory::Init()
{
	SLOT_MAGIC=-1;
	SLOT_FIST=-1;
	SLOT_MELEE=-1;
	LAST_MELEE=-1;
	SLOT_RANGED=-1;
	LAST_RANGED=-1;
	SLOT_QUICK=-1;
	LAST_QUICK=-1;
	SLOT_LEFT=-1;
	SLOT_ARMOR=-1;
	IWD2 = core->HasFeature(GFFlags::HAS_WEAPON_SETS);
}

Inventory::~Inventory()
{
	for (auto& slot : Slots) {
		delete slot;
		slot = nullptr;
	}
}

// duplicates the source inventory into the current one
// also changes the items to not drop, so simulacrum and similar don't become factories
void Inventory::CopyFrom(const Actor *source)
{
	if (!source) {
		return;
	}

	SetSlotCount(source->inventory.GetSlotCount());

	// allocate the items and mark them undroppable
	CREItem *tmp;
	const CREItem *item;
	for (size_t i = 0; i < source->inventory.Slots.size(); i++) {
		item = source->inventory.Slots[i];
		if (item) {
			tmp = new CREItem();
			memcpy(tmp, item, sizeof(CREItem));
			tmp->Flags |= IE_INV_ITEM_UNDROPPABLE;
			int ret = AddSlotItem(tmp, i);
			if (ret != ASI_SUCCESS) {
				delete tmp;
			}
		}
	}

	// preserve the equipped status
	Equipped = source->inventory.GetEquipped();
	EquippedHeader = source->inventory.GetEquippedHeader();

	CalculateWeight();
}

void Inventory::AddItem(CREItem *item)
{
	if (!item) return; //invalid items get no slot
	Slots.push_back(item);
	CalculateWeight();
}

void Inventory::CalculateWeight()
{
	Weight = 0;
	for (const auto slot : Slots) {
		if (!slot) {
			continue;
		}
		if (slot->Weight == -1) {
			const Item *itm = gamedata->GetItem(slot->ItemResRef, true);
			if (!itm) {
				Log(ERROR, "Inventory", "Invalid item: {}!", slot->ItemResRef);
				slot->Weight = 0;
				continue;
			}

			slot->Weight = itm->Weight;
			gamedata->FreeItem(itm, slot->ItemResRef, false);

			// some items can't be dropped once they've been picked up,
			// e.g. the portal key in BG2
			if (!(slot->Flags & IE_INV_ITEM_MOVABLE)) {
				slot->Flags |= IE_INV_ITEM_UNDROPPABLE;
			}
		} else {
			slot->Flags &= ~IE_INV_ITEM_ACQUIRED;
		}
		if (slot->Weight > 0) {
			Weight += slot->Weight * ((slot->Usages[0] && slot->MaxStackAmount) ? slot->Usages[0] : 1);
		}
	}

	if (Owner) {
		Owner->SetBase(IE_ENCUMBRANCE, Weight);
	}
}

void Inventory::AddSlotEffects(ieDword index)
{
	CREItem* slot;

	const Item *itm = GetItemPointer(index, slot);
	if (!itm) {
		Log(ERROR, "Inventory", "Invalid item equipped...");
		return;
	}
	ItemExcl|=itm->ItemExcl;
	ieDword pos = itm->ItemType/32;
	ieDword bit = itm->ItemType%32;
	if (pos<8) {
		ItemTypes[pos]|=1<<bit;
	}

	ieWord gradient = itm->GetWieldedGradient();
	if (gradient!=0xffff) {
		Owner->SetBase(IE_COLORS, gradient);
	}

	//get the equipping effects
	// always refresh, as even if eqfx is null, other effects may have been selfapplied from the block
	Owner->AddEffects(itm->GetEffectBlock(Owner, Owner->Pos, -1, index, 0));
	gamedata->FreeItem(itm, slot->ItemResRef, false);
	//call gui for possible paperdoll animation changes
	if (Owner->InParty) {
		core->SetEventFlag(EF_UPDATEANIM);
	}
}

//no need to know the item effects 'personally', the equipping slot
//is stored in them
void Inventory::RemoveSlotEffects(size_t index)
{
	if (Owner->fxqueue.RemoveEquippingEffects(index)) {
		Owner->RefreshEffects();
		//call gui for possible paperdoll animation changes
		if (Owner->InParty) {
			core->SetEventFlag(EF_UPDATEANIM);
		}
	}
}

void Inventory::SetInventoryType(ieInventoryType arg)
{
	InventoryType = arg;
}

void Inventory::SetSlotCount(size_t size)
{
	if (!Slots.empty()) {
		error("Core", "Inventory size changed???");
		//we don't allow reassignment,
		//if you want this, delete the previous Slots here
	}
	Slots.assign(size, nullptr);
}

/** if you supply a "" string, then it checks if the slot is empty */
bool Inventory::HasItemInSlot(const ResRef& resref, unsigned int slot) const
{
	if (slot >= Slots.size()) {
		return false;
	}
	const CREItem *item = Slots[slot];
	if (!item) {
		return false;
	}
	if (resref.IsEmpty() || item->ItemResRef == resref) {
		return true;
	}
	return false;
}

bool Inventory::IsSlotEmpty(unsigned int slot) const
{
	return !HasItemInSlot("", slot);
}

bool Inventory::HasItemType(ieDword type) const
{
	if (type>255) return false;
	int idx = type/32;
	int bit = type%32;
	return (ItemTypes[idx] & (1<<bit) )!=0;
}

/** counts the items in the inventory, if stacks == 1 then stacks are
		accounted for their heap size */
int Inventory::CountItems(const ResRef &resRef, bool stacks, bool checkBags) const
{
	int count = 0;
	size_t slot = Slots.size();
	while (slot--) {
		const CREItem* item = Slots[slot];
		if (!item) continue;
		if (resRef.IsEmpty() || item->ItemResRef == resRef) {
			if (stacks && (item->Flags & IE_INV_ITEM_STACKED)) {
				count += item->Usages[0];
				assert(count != 0);
			} else if (resRef.IsEmpty() && static_cast<int>(slot) == SLOT_FIST) {
				continue;
			} else {
				count++;
			}
			continue; // ignore potential of bags with the same name
		}
		if (!checkBags) continue;

		// maybe in a bag?
		const Item* itemStore = gamedata->GetItem(item->ItemResRef);
		if (!itemStore) continue;
		if (core->CheckItemType(itemStore, SLOT_BAG)) {
			count += StoreCountItems(item->ItemResRef, resRef);
		}
		gamedata->FreeItem(itemStore, item->ItemResRef);
	}

	return count;
}

/** this function can look for stolen, equipped, identified, destructible
		etc, items. You just have to specify the flags in the bitmask
		specifying 1 in a bit signifies a requirement */
bool Inventory::HasItem(const ResRef &resref, ieDword flags) const
{
	size_t slot = Slots.size();
	while(slot--) {
		const CREItem *item = Slots[slot];
		if (!item) {
			continue;
		}
		if ( (flags&item->Flags)!=flags) {
				continue;
		}
		if (item->ItemResRef != resref) {
			continue;
		}
		return true;
	}
	return false;
}

void Inventory::KillSlot(size_t index)
{
	if (InventoryType == ieInventoryType::HEAP) {
		Slots.erase(Slots.begin()+index);
		return;
	}
	const CREItem *item = Slots[index];
	if (!item) {
		CacheAllWeaponInfo();
		return;
	}

	//the used up item vanishes from the quickslot bar
	if (Owner->IsSelected()) {
		core->SetEventFlag( EF_ACTION );
	}

	Slots[index] = NULL;
	CalculateWeight();

	int effect = core->QuerySlotEffects((unsigned int) index);
	if (!effect) {
		CacheAllWeaponInfo();
		return;
	}
	RemoveSlotEffects(index);
	const Item *itm = gamedata->GetItem(item->ItemResRef, true);
	//this cannot happen, but stuff happens!
	if (!itm) {
		error("Inventory", "Invalid item: {}!", item->ItemResRef);
	}
	ItemExcl &= ~itm->ItemExcl;

	int eqslot = GetEquippedSlot();
	ieDword equip;
	bool recache = true;
	switch (effect) {
		case SLOT_EFFECT_LEFT:
			UpdateShieldAnimation(nullptr);
			break;
		case SLOT_EFFECT_MISSILE:
			//getting a new projectile of the same type
			if (eqslot == (int) index && Equipped < 0) {
				// always get the projectile weapon header (this quiver was equipped)
				const ITMExtHeader *header = itm->GetWeaponHeader(true);
				// remove potential launcher effects too
				RemoveSlotEffects(FindTypedRangedWeapon(header->ProjectileQualifier));
				equip = FindRangedProjectile(header->ProjectileQualifier);
				if (equip != IW_NO_EQUIPPED) {
					recache = !EquipItem(GetWeaponSlot(equip));
				} else {
					recache = !EquipBestWeapon(EQUIP_MELEE);
				}
			}
			UpdateWeaponAnimation();
			break;
		case SLOT_EFFECT_MAGIC:
		case SLOT_EFFECT_MELEE:
			// reset Equipped if it was the removed item
			if (eqslot == (int)index) {
				recache = false;
				if ((int) index == SLOT_MAGIC) {
					recache = !EquipBestWeapon(EQUIP_MELEE | EQUIP_RANGED | EQUIP_FORCE);
					break;
				}
				SetEquippedSlot(IW_NO_EQUIPPED, 0);
				UpdateWeaponAnimation();
				break;
			}

			if (Equipped >= 0) {
				UpdateWeaponAnimation();
				break;
			}

			// always get the projectile weapon header (this is a bow, because Equipped is negative)
			const ITMExtHeader *header;
			header = itm->GetWeaponHeader(true);
			if (!header) {
				UpdateWeaponAnimation();
				break;
			}

			// find the equipped type
			int type;
			int weaponslot;
			type = header->ProjectileQualifier;
			weaponslot = FindTypedRangedWeapon(type);
			if (weaponslot == SLOT_FIST) { // a ranged weapon was not found - freshly unequipped
				recache = !EquipBestWeapon(EQUIP_MELEE);
				UpdateWeaponAnimation();
				break;
			}

			if (type != header->ProjectileQualifier) {
				UpdateWeaponAnimation();
				break;
			}

			const CREItem *item2;
			item2 = Slots[weaponslot];
			if (!item2) {
				UpdateWeaponAnimation();
				break;
			}

			const Item *itm2;
			itm2 = gamedata->GetItem(item2->ItemResRef, true);
			if (!itm2) {
				UpdateWeaponAnimation();
				break;
			}

			equip = FindRangedProjectile(header->ProjectileQualifier);
			if (equip != IW_NO_EQUIPPED) {
				recache = !EquipItem(GetWeaponSlot(equip));
			} else {
				recache = !EquipBestWeapon(EQUIP_MELEE);
			}
			gamedata->FreeItem(itm2, item2->ItemResRef, false);

			// reset Equipped if it is a ranged weapon slot
			// but not magic weapon slot!

			UpdateWeaponAnimation();
			break;
		case SLOT_EFFECT_HEAD:
			Owner->SetUsedHelmet({});
			recache = false;
			break;
		case SLOT_EFFECT_ITEM:
			//remove the armor type only if this item is responsible for it
			if ((ieDword) (itm->AnimationType[0]-'1') == Owner->GetBase(IE_ARMOR_TYPE)) {
				Owner->SetBase(IE_ARMOR_TYPE, 0);
			}
			recache = false;
			break;
		case SLOT_EFFECT_ALIAS:
			break;
		default:
			Log(FATAL, "Inventory", "Unknown slot effect type set: {}", effect);
			break;
	}
	if (recache) CacheAllWeaponInfo();
	gamedata->FreeItem(itm, item->ItemResRef, false);
}
/** if resref is "", then destroy ALL items
this function can look for stolen, equipped, identified, destructible
etc, items. You just have to specify the flags in the bitmask
specifying 1 in a bit signifies a requirement */
unsigned int Inventory::DestroyItem(const ResRef& resref, ieDword flags, ieDword count)
{
	unsigned int destructed = 0;
	size_t slot = Slots.size();

	while(slot--) {
		//ignore the fist slot
		if (slot == (unsigned int)SLOT_FIST) {
			continue;
		}

		CREItem *item = Slots[slot];
		if (!item) {
			continue;
		}
		// here you can simply destroy all items of a specific type
		if ( (flags&item->Flags)!=flags) {
			continue;
		}
		if (!resref.IsEmpty() && item->ItemResRef != resref) {
			continue;
		}
		//we need to acknowledge that the item was destroyed
		//use unequip stuff etc,
		//until that, we simply erase it
		ieDword removed;

		if (item->Flags&IE_INV_ITEM_STACKED) {
			removed=item->Usages[0];
			if (count && (removed + destructed > count) ) {
				removed = count - destructed;
				item = RemoveItem( (unsigned int) slot, removed );
			}
			else {
				KillSlot(slot);
			}
		} else {
			removed=1;
			KillSlot(slot);
		}
		delete item;
		destructed+=removed;
		if (count && (destructed>=count) )
			break;
	}
	if (destructed && Owner && Owner->InParty) {
		displaymsg->DisplayMsgCentered(HCStrings::LostItem, FT_ANY, GUIColors::XPCHANGE);
	}

	return destructed;
}

CREItem *Inventory::RemoveItem(unsigned int slot, unsigned int count)
{
	CREItem *item;

	if (slot >= Slots.size() ) {
		InvalidSlot(slot);
	}
	item = Slots[slot];

	if (!item) {
		return NULL;
	}

	if (!count || !(item->Flags & IE_INV_ITEM_STACKED) || (count >= item->Usages[0])) {
		KillSlot(slot);
		return item;
	}

	CREItem *returned = new CREItem(*item);
	item->Usages[0]-=count;
	returned->Usages[0]=(ieWord) count;
	CalculateWeight();
	return returned;
}

//flags set disable item transfer
//except for undroppable which is opposite (and shouldn't be set)
int Inventory::RemoveItem(const ResRef& resref, unsigned int flags, CREItem **res_item, int count)
{
	size_t slot = Slots.size();
	unsigned int mask = (flags^IE_INV_ITEM_UNDROPPABLE);
	if (core->HasFeature(GFFlags::NO_DROP_CAN_MOVE) ) {
		mask &= ~IE_INV_ITEM_UNDROPPABLE;
	}
	while(slot--) {
		const CREItem *item = Slots[slot];
		if (!item) {
			continue;
		}

		if (flags && (mask&item->Flags)==flags) {
			continue;
		}
		if (!flags && (mask&item->Flags)!=0) {
			continue;
		}
		if (resref[0] && item->ItemResRef != resref) {
			continue;
		}
		*res_item=RemoveItem( (unsigned int) slot, count);
		return (int) slot;
	}
	*res_item = NULL;
	return -1;
}

void Inventory::SetSlotItem(CREItem* item, unsigned int slot)
{
	if (slot >= Slots.size() ) {
		InvalidSlot(slot);
	}

	delete Slots[slot];
	Slots[slot] = item;

	CalculateWeight();

	//update the action bar next time
	if (Owner->IsSelected()) {
		core->SetEventFlag( EF_ACTION );
	}
}

int Inventory::AddSlotItem(CREItem* item, int slot, int slottype, bool ranged)
{
	// just dump into containers if a specific slot wasn't requested
	if (InventoryType == ieInventoryType::HEAP && slot < 0) {
		AddItem(item);
		return ASI_SUCCESS;
	}

	int twohanded = item->Flags&IE_INV_ITEM_TWOHANDED;
	if (slot >= 0) {
		if ((unsigned)slot >= Slots.size()) {
			InvalidSlot(slot);
		}

		//check for equipping weapons
		if (WhyCantEquip(slot, twohanded, ranged) != HCStrings::count) {
			return ASI_FAILED;
		}

		if (!Slots[slot]) {
			item->Flags |= IE_INV_ITEM_ACQUIRED;
			SetSlotItem(item, slot);
			EquipItem(slot);
			return ASI_SUCCESS;
		}

		return MergeItems(slot, item);
	}

	bool which = (slot == SLOT_AUTOEQUIP);
	int res = ASI_FAILED;
	int max = (int) Slots.size();
	for (int i = 0;i<max;i++) {
		//never autoequip in the magic slot!
		if (i==SLOT_MAGIC)
			continue;
		if ((i<SLOT_INV || i>LAST_INV)!=which)
			continue;
		if (!(core->QuerySlotType(i)&slottype))
			continue;
		//the slot has been disabled for this actor
		if (i >= SLOT_MELEE && i <= LAST_MELEE && Owner->GetQuickSlot(i - SLOT_MELEE) == 0xffff) {
			continue;
		}
		int part_res = AddSlotItem (item, i);
		if (part_res == ASI_SUCCESS) return ASI_SUCCESS;
		else if (part_res == ASI_PARTIAL) res = ASI_PARTIAL;
	}

	return res;
}

//Used by FillSlot
void Inventory::TryEquipAll(int slot)
{
	for(int i=SLOT_INV;i<=LAST_INV;i++) {
		CREItem *item = Slots[i];
		if (!item) {
			continue;
		}

		Slots[i]=NULL;
		if (AddSlotItem(item, slot) == ASI_SUCCESS) {
			return;
		}
		//try to stuff it back, it should work
		if (AddSlotItem(item, i) != ASI_SUCCESS) {
			delete item;
		}
	}
}

int Inventory::AddStoreItem(STOItem* item, StoreActionType action)
{
	CREItem *temp;
	int ret = -1;

	// item->PurchasedAmount is the number of items bought
	// (you can still add grouped objects in a single step,
	// just set up STOItem)
	while (item->PurchasedAmount) {
		//the first part of a STOItem is essentially a CREItem
		temp = new CREItem(item);

		//except the Expired flag
		temp->Expired=0;
		if (action == StoreActionType::Steal && !core->HasFeature(GFFlags::PST_STATE_FLAGS)) {
			temp->Flags |= IE_INV_ITEM_STOLEN; // "steel" in pst
		}
		temp->Flags &= ~IE_INV_ITEM_SELECTED;
		
		ret = AddSlotItem( temp, SLOT_ONLYINVENTORY );
		if (ret != ASI_SUCCESS) {
			delete temp;
			break;
		}
		if (item->InfiniteSupply!=-1) {
			if (!item->AmountInStock) {
				break;
			}
			item->AmountInStock--;
		}
		item->PurchasedAmount--;
	}
	return ret;
}

/* could the source item be dropped on the target item to merge them */
bool Inventory::ItemsAreCompatible(const CREItem* target, const CREItem* source) const
{
	if (!target) {
		//this isn't always ok, please check!
		Log(WARNING, "Inventory", "Null item encountered by ItemsAreCompatible()");
		return true;
	}

	if (!(source->Flags&IE_INV_ITEM_STACKED) ) {
		return false;
	}

	return target->ItemResRef == source->ItemResRef;
}

//depletes a magical item
//if flags==0 then magical weapons are not harmed
int Inventory::DepleteItem(ieDword flags) const
{
	for (auto item : Slots) {
		if (!item) {
			continue;
		}

		//don't harm critical items
		//don't harm nonmagical items
		//don't harm indestructible items
		// ees supposedly also check if the item is droppable (IE_INV_ITEM_UNDROPPABLE)
		if ( (item->Flags&(IE_INV_ITEM_CRITICAL|IE_INV_DEPLETABLE)) != IE_INV_DEPLETABLE) {
			continue;
		}

		//if flags = 0 then weapons are not depleted
		if (!flags) {
			const Item *itm = gamedata->GetItem(item->ItemResRef, true);
			if (!itm) {
				Log(WARNING, "Inventory", "Invalid item to deplete: {}!", item->ItemResRef);
				continue;
			}
			//if the item is usable in weapon slot, then it is weapon
			int weapon = core->CheckItemType(itm, SLOT_WEAPON);
			gamedata->FreeItem( itm, item->ItemResRef, false );
			if (weapon)
				continue;
		}
		//deplete item
		item->Usages.fill(0);
	}
	return -1;
}

// if flags is 0, skips undroppable items
// if flags is IE_INV_ITEM_UNDROPPABLE, doesn't skip undroppable items
// TODO: once all callers have been checked, this can be reversed to make more sense
// you can pass "" for the name to scan the whole inventory
int Inventory::FindItem(const ResRef &resref, unsigned int flags, unsigned int skip) const
{
	unsigned int mask = (flags^IE_INV_ITEM_UNDROPPABLE);
	if (core->HasFeature(GFFlags::NO_DROP_CAN_MOVE) ) {
		mask &= ~IE_INV_ITEM_UNDROPPABLE;
	}
	for (size_t i = 0; i < Slots.size(); i++) {
		const CREItem *item = Slots[i];
		if (!item) {
			continue;
		}
		if ( mask & item->Flags ) {
			continue;
		}
		if (!resref.IsEmpty() && item->ItemResRef != resref) {
			continue;
		}
		if (skip) {
			skip--;
		} else {
			return (int) i;
		}
	}
	return -1;
}

bool Inventory::DropItemAtLocation(unsigned int slot, unsigned int flags, Map *map, const Point &loc)
{
	if (slot >= Slots.size()) {
		return false;
	}
	//these slots will never 'drop' the item
	if ((slot==(unsigned int) SLOT_FIST) || (slot==(unsigned int) SLOT_MAGIC)) {
		return false;
	}

	CREItem *item = Slots[slot];
	if (!item) {
		return false;
	}
	//if you want to drop undoppable items, simply set IE_INV_UNDROPPABLE
	//by default, it won't drop them
	if ( ((flags^IE_INV_ITEM_UNDROPPABLE)&item->Flags)!=flags) {
		return false;
	}
	if (!map) {
		return false;
	}
	map->AddItemToLocation(loc, item);
	KillSlot(slot);
	return true;
}

bool Inventory::DropItemAtLocation(const ResRef& resRef, unsigned int flags, Map *map, const Point &loc)
{
	bool dropped = false;

	if (!map) {
		return false;
	}

	//this loop is going from start
	for (size_t i = 0; i < Slots.size(); i++) {
		//these slots will never 'drop' the item
		if ((i==(unsigned int) SLOT_FIST) || (i==(unsigned int) SLOT_MAGIC)) {
			continue;
		}
		CREItem *item = Slots[i];
		if (!item) {
			continue;
		}
		//if you want to drop undroppable items, simply set IE_INV_UNDROPPABLE
		//by default, it won't drop them
		if ( ((flags^IE_INV_ITEM_UNDROPPABLE)&item->Flags)!=flags) {
				continue;
		}
		if (!resRef.IsEmpty() && item->ItemResRef != resRef) {
			continue;
		}
		// mark it as unequipped, so it doesn't cause problems in stores
		item->Flags &= ~ IE_INV_ITEM_EQUIPPED;
		map->AddItemToLocation(loc, item);
		dropped = true;
		KillSlot(i);
		//if it isn't all items then we stop here
		if (!resRef.IsEmpty())
			break;
	}

	//dropping gold too
	if (resRef.IsEmpty()) {
		if (!Owner->GetBase(IE_GOLD)) {
			return dropped;
		}
		CREItem *gold = new CREItem();
		if (CreateItemCore(gold, core->GoldResRef, static_cast<int>(Owner->BaseStats[IE_GOLD]), 0, 0)) {
			map->AddItemToLocation(loc, gold);
		} else {
			delete gold;
		}
		Owner->BaseStats[IE_GOLD] = 0;
	}
	return dropped;
}

CREItem *Inventory::GetSlotItem(ieDword slot) const
{
	if (slot >= Slots.size() ) {
		InvalidSlot(slot);
	}
	return Slots[slot];
}

ieDword Inventory::GetItemFlag(unsigned int slot) const
{
	const CREItem *item = GetSlotItem(slot);
	if (!item) {
		return 0;
	}
	return item->Flags;
}

bool Inventory::ChangeItemFlag(ieDword slot, ieDword arg, BitOp op) const
{
	CREItem *item = GetSlotItem(slot);
	if (!item) {
		return false;
	}
	SetBits(item->Flags, arg, op);
	return true;
}

//this is the low level equipping
//all checks have been made previously
bool Inventory::EquipItem(ieDword slot)
{
	if (!Owner) {
		// called on a pile
		return false;
	}

	const ITMExtHeader *header;
	const CREItem *item = GetSlotItem(slot);
	if (!item) {
		return false;
	}

	// add effects of an item just being equipped to actor's effect queue
	int effect = core->QuerySlotEffects( slot );
	const Item *itm = gamedata->GetItem(item->ItemResRef, true);
	if (!itm) {
		Log(ERROR, "Inventory", "Invalid item Equipped: {} Slot: {}", item->ItemResRef, slot);
		return false;
	}
	
	Owner->ClearCurrentStanceAnims();
	
	int armorLevel = itm->AnimationType[0] - '1';
	int weaponslot;
	ieDword equip = IW_NO_EQUIPPED;
	switch (effect) {
	case SLOT_EFFECT_FIST:
		SetEquippedSlot(IW_NO_EQUIPPED, 0);
		break;
	case SLOT_EFFECT_LEFT:
		//no idea if the offhand weapon has style, or simply the right
		//hand style is dominant
		CacheAllWeaponInfo();
		UpdateShieldAnimation(itm);
		break;
	case SLOT_EFFECT_MELEE:
		//if weapon is bow, then find quarrel for it and equip that
		weaponslot = GetWeaponQuickSlot(slot);
		EquippedHeader = 0;
		if (Owner->PCStats) {
			int eheader = Owner->PCStats->GetHeaderForSlot(slot);
			if (eheader >= 0) {
				EquippedHeader = eheader;
			}
		}
		header = itm->GetExtHeader(EquippedHeader);
		if (!header) return false; // in case of broken saves
		if (header->AttackType == ITEM_AT_BOW || (header->AttackType == ITEM_AT_PROJECTILE && !header->Charges)) {
			// find the ranged projectile associated with it, this returns equipped code
			equip = FindRangedProjectile(header->ProjectileQualifier);
			// this is the real item slot of the quarrel
			slot = equip + SLOT_MELEE;
		}
		if (header->AttackType != ITEM_AT_BOW && equip == IW_NO_EQUIPPED) {
			// this is always 0-3
			equip = weaponslot;
			slot = GetWeaponSlot(weaponslot);
		}
		if (equip != IW_NO_EQUIPPED) {
			Owner->SetupQuickSlot(ACT_WEAPON1 + weaponslot, slot, EquippedHeader);
		}
		SetEquippedSlot(equip, EquippedHeader);
		effect = 0; // SetEquippedSlot will already call AddSlotEffects
		break;
	case SLOT_EFFECT_MISSILE:
		//Get the ranged header of the projectile (so we theoretically allow shooting of daggers)
		EquippedHeader = itm->GetWeaponHeaderNumber(true);
		header = itm->GetExtHeader(EquippedHeader);
		if (!header) return false; // in case of broken saves
		weaponslot = FindTypedRangedWeapon(header->ProjectileQualifier);
		if (weaponslot != SLOT_FIST) {
			weaponslot -= SLOT_MELEE;
			SetEquippedSlot((ieWordSigned) (slot - SLOT_MELEE), EquippedHeader);
			// it is unsure if we can have multiple equipping headers for bows/arrow
			// it is unclear which item's header index should go there
			Owner->SetupQuickSlot(ACT_WEAPON1+weaponslot, slot, 0);
		}
		UpdateWeaponAnimation();
		break;
	case SLOT_EFFECT_HEAD:
		Owner->SetUsedHelmet(itm->AnimationType);
		break;
	case SLOT_EFFECT_ITEM:
		//adjusting armour level if needed
		if (armorLevel >= IE_ANI_NO_ARMOR && armorLevel <= IE_ANI_HEAVY_ARMOR) {
			Owner->SetBase(IE_ARMOR_TYPE, armorLevel);
		} else {
			UpdateShieldAnimation(itm);
		}
		break;
	default: // none, SLOT_EFFECT_MAGIC (handled indirectly elsewhere) or alias
		break;
	}
	gamedata->FreeItem(itm, item->ItemResRef, false);
	if (effect) {
		AddSlotEffects( slot );
	}
	return true;
}

//the removecurse flag will check if it is possible to move the item to the inventory
//after a remove curse spell
bool Inventory::UnEquipItem(ieDword slot, bool removecurse) const
{
	CREItem *item = GetSlotItem(slot);
	if (!item) {
		return false;
	}
	if (item->Flags & IE_INV_ITEM_UNDROPPABLE && !core->HasFeature(GFFlags::NO_DROP_CAN_MOVE)) {
		return false;
	}

	if (!removecurse && item->Flags & IE_INV_ITEM_CURSED && core->QuerySlotEffects(slot)) {
		return false;
	}
	
	Owner->ClearCurrentStanceAnims();
	item->Flags &= ~IE_INV_ITEM_EQUIPPED; //no idea if this is needed, won't hurt
	return true;
}

// find the projectile
// type = 1 - bow
//        2 - xbow
//        4 - sling
//returns equipped code
int Inventory::FindRangedProjectile(unsigned int type) const
{
	for(int i=SLOT_RANGED;i<=LAST_RANGED;i++) {
		CREItem *Slot;

		const Item *itm = GetItemPointer(i, Slot);
		if (!itm) continue;
		const ITMExtHeader *ext_header = itm->GetExtHeader(0);
		unsigned int weapontype = 0;
		if (ext_header) {
			weapontype = ext_header->ProjectileQualifier;
		}
		gamedata->FreeItem(itm, Slot->ItemResRef, false);
		if (weapontype & type) {
			return i-SLOT_MELEE;
		}
	}
	return IW_NO_EQUIPPED;
}

// find which bow is attached to the projectile marked by 'Equipped'
// returns slotcode
int Inventory::FindRangedWeapon() const
{
	if (Equipped>=0) return SLOT_FIST;
	return FindSlotRangedWeapon(GetEquippedSlot());
}

int Inventory::FindSlotRangedWeapon(ieDword slot) const
{
	if ((int)slot >= SLOT_MELEE) return SLOT_FIST;
	CREItem *Slot;
	const Item *itm = GetItemPointer(slot, Slot);
	if (!itm) return SLOT_FIST;

	//always look for a ranged header when looking for a projectile/projector
	const ITMExtHeader *ext_header = itm->GetWeaponHeader(true);
	unsigned int type = 0;
	if (ext_header) {
		type = ext_header->ProjectileQualifier;
	}
	gamedata->FreeItem(itm, Slot->ItemResRef, false);
	return FindTypedRangedWeapon(type);
}


// find bow for a specific projectile type
int Inventory::FindTypedRangedWeapon(unsigned int type) const
{
	if (!type) {
		return SLOT_FIST;
	}
	for(int i=SLOT_MELEE;i<=LAST_MELEE;i++) {
		CREItem *Slot;

		const Item *itm = GetItemPointer(i, Slot);
		if (!itm) continue;
		//always look for a ranged header when looking for a projectile/projector
		const ITMExtHeader *ext_header = itm->GetWeaponHeader(true);
		int weapontype = 0;
		if (ext_header && (ext_header->AttackType == ITEM_AT_BOW || (ext_header->AttackType == ITEM_AT_PROJECTILE && !ext_header->Charges))) {
			weapontype = ext_header->ProjectileQualifier;
		}
		gamedata->FreeItem(itm, Slot->ItemResRef, false);
		if (weapontype & type) {
			return i;
		}
	}
	return SLOT_FIST;
}

void Inventory::SetHeadSlot(int arg) { SLOT_HEAD=arg; }
void Inventory::SetFistSlot(int arg) { SLOT_FIST=arg; }
void Inventory::SetMagicSlot(int arg) { SLOT_MAGIC=arg; }
void Inventory::SetWeaponSlot(int arg)
{
	if (SLOT_MELEE==-1) {
		SLOT_MELEE=arg;
	}
	LAST_MELEE=arg;
}

//ranged slots should be before MELEE slots
void Inventory::SetRangedSlot(int arg)
{
	assert(SLOT_MELEE!=-1);
	if (SLOT_RANGED==-1) {
		SLOT_RANGED=arg;
	}
	LAST_RANGED=arg;
}

void Inventory::SetQuickSlot(int arg)
{
	if (SLOT_QUICK==-1) {
		SLOT_QUICK=arg;
	}
	LAST_QUICK=arg;
}

void Inventory::SetInventorySlot(int arg)
{
	if (SLOT_INV==-1) {
		SLOT_INV=arg;
	}
	LAST_INV=arg;
}

void Inventory::SetArmorSlot(int arg)
{
	if (SLOT_ARMOR==-1) {
		SLOT_ARMOR=arg;
	}
}

//multiple shield slots are allowed
//but in this case they should be interspersed with melee slots
void Inventory::SetShieldSlot(int arg)
{
	if (SLOT_LEFT!=-1) {
		assert(SLOT_MELEE+1==SLOT_LEFT);
		return;
	}
	SLOT_LEFT=arg;
}

int Inventory::GetHeadSlot()
{
	return SLOT_HEAD;
}

int Inventory::GetFistSlot()
{
	return SLOT_FIST;
}

int Inventory::GetMagicSlot()
{
	return SLOT_MAGIC;
}

int Inventory::GetWeaponSlot()
{
	return SLOT_MELEE;
}

int Inventory::GetWeaponQuickSlot(int weaponslot)
{
	int slot = weaponslot-SLOT_MELEE;
	if (IWD2 && (slot>=0 && slot<=7) ) slot/=2;
	return slot;
}

int Inventory::GetWeaponSlot(int quickslot)
{
	if (IWD2 && (quickslot>=0 && quickslot<=3) ) quickslot*=2;
	return quickslot+SLOT_MELEE;
}

int Inventory::GetQuickSlot()
{
	return SLOT_QUICK;
}

int Inventory::GetInventorySlot()
{
	return SLOT_INV;
}

int Inventory::GetArmorSlot()
{
	return SLOT_ARMOR;
}

//if shield slot is empty, call again for fist slot!
int Inventory::GetShieldSlot() const
{
	if (IWD2) {
		//actually, in IWD2, the equipped slot never becomes IW_NO_EQUIPPED, it is always 0-3
		//this is just a hack to prevent invalid shots from happening
		if (Equipped == IW_NO_EQUIPPED) return SLOT_MELEE+1;

		if (Equipped>=0 && Equipped<=3) {
			return Equipped*2+SLOT_MELEE+1;
		}
		//still, what about magic weapons...
		return -1;
	}
	return SLOT_LEFT;
}

int Inventory::GetEquippedSlot() const
{
	if (Equipped == IW_NO_EQUIPPED) {
		return SLOT_FIST;
	}
	if (IWD2 && Equipped>=0) {
		//Equipped should never become IW_NO_EQUIPPED, this is just a hack to cover the bug
		//about it still becoming invalid
		if (Equipped >= 4) {
			return SLOT_MELEE;
		}
		return Equipped*2+SLOT_MELEE;
	}
	return Equipped+SLOT_MELEE;
}

bool Inventory::SetEquippedSlot(ieWordSigned slotcode, ieWord header, bool noFX)
{
	EquippedHeader = header;
	
	//doesn't work if magic slot is used, refresh the magic slot just in case
	if (MagicSlotEquipped() && (slotcode!=SLOT_MAGIC-SLOT_MELEE)) {
		Equipped = SLOT_MAGIC-SLOT_MELEE;
		CacheAllWeaponInfo();
		UpdateWeaponAnimation();
		return false;
	}

	//if it is an illegal code, make it fist
	if ((size_t) (GetWeaponSlot(slotcode))>Slots.size()) {
		slotcode=IW_NO_EQUIPPED;
		EquippedHeader = 0;
	}

	unsigned int oldslot = GetEquippedSlot();
	int newslot = GetWeaponSlot(slotcode);

	//remove previous slot effects
	if (Equipped != IW_NO_EQUIPPED) {
		RemoveSlotEffects(oldslot);
		//for projectiles we may need to remove the launcher effects too
		int oldeffects = core->QuerySlotEffects(oldslot);
		if (oldeffects == SLOT_EFFECT_MISSILE) {
			size_t launcher = FindSlotRangedWeapon(oldslot);
			if (launcher != (unsigned int) SLOT_FIST) {
				RemoveSlotEffects(launcher);
			}
		}
	}

	//unequipping (fist slot will be used now)
	if (slotcode == IW_NO_EQUIPPED || IsSlotEmpty(newslot)) {
		Equipped = IW_NO_EQUIPPED;
		//fist slot equipping effects
		AddSlotEffects(SLOT_FIST);
		CacheAllWeaponInfo();
		UpdateWeaponAnimation();
		return true;
	}

	//equipping a weapon
	Equipped = slotcode;
	int effects = core->QuerySlotEffects( newslot);
	if (effects) {
		CREItem* item = GetSlotItem(newslot);
		item->Flags|=IE_INV_ITEM_EQUIPPED;
		if (!noFX) {
			AddSlotEffects(newslot);

			//in case of missiles also look for an appropriate launcher
			if (effects == SLOT_EFFECT_MISSILE) {
				newslot = FindRangedWeapon();
				AddSlotEffects(newslot);
			}
		}
	}
	CacheAllWeaponInfo();
	UpdateWeaponAnimation();
	return true;
}

int Inventory::GetEquipped() const
{
	return Equipped;
}

int Inventory::GetEquippedHeader() const
{
	return EquippedHeader;
}

// store this internally just like Equipped/EquippedHeader if it turns into a hot path
const ITMExtHeader *Inventory::GetEquippedExtHeader(int header) const
{
	int slot; // Equipped holds the projectile, not the weapon
	const CREItem *itm = GetUsedWeapon(false, slot); // check the main hand only
	if (!itm) return NULL;
	const Item *item = gamedata->GetItem(itm->ItemResRef, true);
	if (!item) return NULL;
	return item->GetExtHeader(header);
}

void Inventory::SetEquipped(ieWordSigned slot, ieWord header)
{
	Equipped = slot;
	EquippedHeader = header;
	// CacheAllWeaponInfo will be called later in all callers
}

void Inventory::CacheAllWeaponInfo() const
{
	CacheWeaponInfo(false);
	if (Owner->IsDualWielding()) {
		CacheWeaponInfo(true);
	} else {
		WeaponInfo& wi = Owner->weaponInfo[1];
		wi.extHeader = nullptr;
		wi.item = nullptr;
		wi.wflags = 0;
	}
}

void Inventory::CacheWeaponInfo(bool leftOrRight) const
{
	WeaponInfo& wi = Owner->weaponInfo[leftOrRight];
	wi.slot = GetEquippedSlot();
	bool ranged = (core->QuerySlotEffects(wi.slot) & SLOT_EFFECT_MISSILE) == SLOT_EFFECT_MISSILE; // detect ammo slot
	wi.extHeader = nullptr; // for the error paths; properly set at the end
	wi.item = nullptr;
	wi.wflags = 0;

	const CREItem* weapon;
	if (ranged) {
		weapon = GetSlotItem(wi.slot); // actually the projectile
	} else {
		weapon = GetUsedWeapon(leftOrRight, wi.slot);
	}
	if (!weapon) {
		return;
	}
	const Item *item = gamedata->GetItem(weapon->ItemResRef, true);
	if (!item) {
		Log(WARNING, "Actor", "Missing or invalid weapon item: {}!", weapon->ItemResRef);
		return;
	}
	wi.item = item;

	wi.itemflags = weapon->Flags;
	wi.critmulti = core->GetCriticalMultiplier(item->ItemType);
	wi.critrange = core->GetCriticalRange(item->ItemType);

	// fetch info from selected weapon/ammo header
	const ITMExtHeader* hittingHeader;
	int headerIdx = 0; // can't use EquippedHeader because we might be dealing with the offhand in combination with a multiheader item
	if (item->GetExtHeaderCount() > 1U) {
		if (Owner->PCStats) {
			headerIdx = Owner->PCStats->GetHeaderForSlot(wi.slot);
			if (headerIdx == -1) { // not a quickslot (eg. left hand / shield slot)
				headerIdx = 0;
			}
		} else {
			headerIdx = EquippedHeader;
			// play it absolutely safe
			if (!item->GetExtHeader(headerIdx)) headerIdx = 0;
		}
	}
	hittingHeader = item->GetExtHeader(headerIdx);

	if (!hittingHeader) return; // ar2808 elf has the misc3b moonblade equipped ... and it has no headers
	if (hittingHeader->AttackType == ITEM_AT_PROJECTILE) ranged = true; // throwing weapon

	if (ranged) {
		wi.backstabbing = hittingHeader->RechargeFlags & IE_ITEM_BACKSTAB;
		wi.wflags = WEAPON_RANGED;

		// deal with data we need to use from the launcher
		int tmpSlot = wi.slot; // GetUsedWeapon will modify it!
		const CREItem* launcher = GetUsedWeapon(false, tmpSlot);
		const Item* launcherItem = gamedata->GetItem(launcher->ItemResRef, true);
		assert(launcherItem);
		const ITMExtHeader* launcherHeader = launcherItem->GetExtHeader(0);
		assert(launcherHeader);
		// range has to be from the weapon, the projectile has no effect on it
		wi.range = launcherHeader->Range + 1;
		if (launcher->ItemResRef != weapon->ItemResRef) { // don't add bonuses without both launcher and ammo or they would be doubled
			wi.launcherDmgBonus = launcherHeader->DamageBonus; // save the original bonus
			wi.launcherTHAC0Bonus = launcherHeader->THAC0Bonus;
		} else {
			wi.launcherDmgBonus = 0;
			wi.launcherTHAC0Bonus = 0;
		}
		wi.itemtype = launcherItem->ItemType;
		wi.prof = launcherItem->WeaProf;
		// the magic of the bow and the arrow do not add up
		wi.enchantment = std::max(item->Enchantment, launcherItem->Enchantment);
		if (launcherHeader->RechargeFlags & IE_ITEM_KEEN) {
			// both launchers and ammo can be keen (00arow87)
			wi.critrange--;
		}

		gamedata->FreeItem(launcherItem, launcher->ItemResRef, false);
	} else {
		wi.wflags = WEAPON_MELEE;
		if (leftOrRight && Owner->IsDualWielding()) {
			wi.wflags |= WEAPON_LEFTHAND;
		}
		wi.enchantment = item->Enchantment;
		wi.itemtype = item->ItemType;
		wi.prof = item->WeaProf;
		wi.launcherDmgBonus = 0;
		wi.launcherTHAC0Bonus = 0;

		// any melee weapon usable by a single class thief is game (UAI does not affect this)
		// but also check a bit in the recharge flags (modder extension)
		wi.backstabbing = !(item->UsabilityBitmask & 0x400000) || (hittingHeader->RechargeFlags & IE_ITEM_BACKSTAB);
		wi.range = hittingHeader->Range + 1;
		if (IWD2) {
			// iwd2 doesn't set the usability mask
			wi.backstabbing = true;
		}
	}

	// make sure we use 'false' in this FreeItem
	// so the extended header won't point into invalid memory
	gamedata->FreeItem(item, weapon->ItemResRef, false);

	// these flags are set by the launcher, not ammo
	if (hittingHeader->RechargeFlags & IE_ITEM_USESTRENGTH) wi.wflags |= WEAPON_USESTRENGTH;
	if (hittingHeader->RechargeFlags & IE_ITEM_USESTRENGTH_DMG) wi.wflags |= WEAPON_USESTRENGTH_DMG;
	if (hittingHeader->RechargeFlags & IE_ITEM_USESTRENGTH_HIT) wi.wflags |= WEAPON_USESTRENGTH_HIT;
	// this flag is set in dagger/shortsword by the loader
	if (hittingHeader->RechargeFlags & IE_ITEM_USEDEXTERITY) wi.wflags |= WEAPON_FINESSE;
	if (hittingHeader->RechargeFlags & IE_ITEM_BREAKABLE) wi.wflags |= WEAPON_BREAKABLE;
	// also copy these flags (they match their WEAPON_ counterparts)
	wi.wflags |= hittingHeader->RechargeFlags & (IE_ITEM_KEEN | IE_ITEM_BYPASS);

	if (hittingHeader->RechargeFlags & IE_ITEM_KEEN) {
		//this is correct, the threat range is only increased by one in the original engine
		wi.critrange--;
	}
	if (hittingHeader->Location != ITEM_LOC_WEAPON) {
		return;
	}
	wi.extHeader = hittingHeader;
}

bool Inventory::FistsEquipped() const
{
	return Equipped == IW_NO_EQUIPPED;
}

bool Inventory::MagicSlotEquipped() const
{
	if (SLOT_MAGIC != -1) {
		return Slots[SLOT_MAGIC] != NULL;
	}
	return false;
}

//returns the fist weapon if there is nothing else
//This will return the actual weapon, I mean the bow in the case of bow+arrow combination
CREItem *Inventory::GetUsedWeapon(bool leftorright, int &slot) const
{
	CREItem *ret;

	if (SLOT_MAGIC!=-1) {
		slot = SLOT_MAGIC;
		ret = GetSlotItem(slot);
		if (ret && !ret->ItemResRef.IsEmpty()) {
			return ret;
		}
	}
	if (leftorright) {
		//no shield slot
		slot = GetShieldSlot();
		if (slot>=0) {
			ret = GetSlotItem(slot);
			if (ret) {
				return ret;
			} else {
				//we don't want to return fist for shield slot
				return NULL;
			}
		} else {
			// nothing in the shield slot, so nothing in the right hand, so just quit
			return NULL;
		}
	}
	slot = GetEquippedSlot();
	if((core->QuerySlotEffects(slot) & SLOT_EFFECT_MISSILE) == SLOT_EFFECT_MISSILE) {
		slot = FindRangedWeapon();
	}
	ret = GetSlotItem(slot);
	if (!ret) {
		//return fist weapon
		slot = SLOT_FIST;
		ret = GetSlotItem(slot);
	}
	return ret;
}

// Returns index of first empty slot or slot with the same
// item and not full stack. On fail returns -1
// Can be used to check for full inventory
int Inventory::FindCandidateSlot(int slottype, size_t first_slot, const ResRef& resref) const
{
	if (first_slot >= Slots.size())
		return -1;

	for (size_t i = first_slot; i < Slots.size(); i++) {
		if (!(core->QuerySlotType( (unsigned int) i) & slottype) ) {
			continue;
		}

		const CREItem *item = Slots[i];

		if (!item) {
			return (int) i; //this is a good empty slot
		}
		if (!resref) {
			continue;
		}
		if (!(item->Flags&IE_INV_ITEM_STACKED) ) {
			continue;
		}
		if (item->ItemResRef != resref) {
			continue;
		}
		// check if the item fits in this slot, we use the cached
		// stackamount value
		if (item->Usages[0]<item->MaxStackAmount) {
			return (int) i;
		}
	}

	return -1;
}

void Inventory::AddSlotItemRes(const ResRef& ItemResRef, int SlotID, int Charge0, int Charge1, int Charge2)
{
	CREItem *TmpItem = new CREItem();
	if (CreateItemCore(TmpItem, ItemResRef, Charge0, Charge1, Charge2)) {
		int ret = AddSlotItem( TmpItem, SlotID );
		if (ret != ASI_SUCCESS) {
			// put the remainder on the ground
			Map *area = core->GetGame()->GetCurrentArea();
			if (area) {
				// create or reuse the existing pile
				area->AddItemToLocation(Owner->Pos, TmpItem);
			} else {
				Log(ERROR, "Inventory", "AddSlotItemRes: argh, no area and the inventory is full, bailing out!");
				delete TmpItem;
			}
		}
	} else {
		delete TmpItem;
	}
}

void Inventory::SetSlotItemRes(const ResRef& ItemResRef, size_t SlotID, int Charge0, int Charge1, int Charge2)
{
	if (!ItemResRef.IsEmpty()) {
		CREItem *TmpItem = new CREItem();
		if (CreateItemCore(TmpItem, ItemResRef, Charge0, Charge1, Charge2)) {
			SetSlotItem(TmpItem, (unsigned int) SlotID);
		} else {
			delete TmpItem;
		}
	} else {
		//if the item isn't creatable, we still destroy the old item
		KillSlot(SlotID);
	}
}

ieWord Inventory::GetShieldItemType() const
{
	ieWord ret;
	CREItem *Slot;
	int slotNum = GetShieldSlot();

	if (slotNum < 0) {
		return 0xffff;
	}
	const Item *itm = GetItemPointer(slotNum, Slot);
	if (!itm) return 0xffff;
	ret = itm->ItemType;
	gamedata->FreeItem(itm, Slot->ItemResRef);
	return ret;
}

ieWord Inventory::GetArmorItemType() const
{
	ieWord ret;
	CREItem *Slot;
	int slotNum = GetArmorSlot();

	if (slotNum < 0) {
		return 0xffff;
	}
	const Item *itm = GetItemPointer(slotNum, Slot);
	if (!itm) return 0xffff;
	ret = itm->ItemType;
	gamedata->FreeItem(itm, Slot->ItemResRef);
	return ret;
}

void Inventory::BreakItemSlot(ieDword slot)
{
	ResRef newItem;
	CREItem *Slot;

	const Item *itm = GetItemPointer(slot, Slot);
	if (!itm) return;
	//if it is the magic weapon slot, don't break it, just remove it, because it couldn't be removed
	//or for pst, just remove it as there is no breaking (the replacement item is a sound)
	// also don't break ranged weapons (eg. when running out of throwing axes)
	if (slot == (unsigned int) SLOT_MAGIC || core->HasFeature(GFFlags::HAS_PICK_SOUND) || Owner->weaponInfo[Owner->usedLeftHand].wflags & WEAPON_RANGED) {
		newItem.Reset();
	} else {
		newItem = itm->ReplacementItem;
	}
	gamedata->FreeItem( itm, Slot->ItemResRef, true );
	//this depends on setslotitemres using setslotitem
	SetSlotItemRes(newItem, slot, 0,0,0);
	ieDword slotEffects = core->QuerySlotEffects(slot);
	bool replaced = false;
	if (slotEffects == SLOT_EFFECT_MELEE) {
		replaced = EquipBestWeapon(EQUIP_MELEE);
	} else if (slotEffects == SLOT_EFFECT_MISSILE) {
		replaced = EquipBestWeapon(EQUIP_RANGED);
	}
	if (!replaced) CacheAllWeaponInfo();
}

std::string Inventory::dump(bool print) const
{
	std::string buffer("INVENTORY:\n");
	for (unsigned int i = 0; i < Slots.size(); i++) {
		const CREItem* itm = Slots[i];

		if (!itm) {
			continue;
		}

		AppendFormat(buffer, "{}: {} - {} Flags: {:#x} ", i, itm->ItemResRef, itm->Usages, itm->Flags);
		AppendFormat(buffer, "Wt: {} x {}Lb\n", (itm->Usages[0] && itm->MaxStackAmount) ? itm->Usages[0] : 1, itm->Weight);
	}

	AppendFormat(buffer, "Equipped: {}       EquippedHeader: {}\n", Equipped, EquippedHeader);
	AppendFormat(buffer, "Total weight: {}\n", Weight);
	if (print) Log(DEBUG, "Inventory", "{}", buffer);
	return buffer;
}

bool Inventory::CanEquipRanged(int& maxDamage, ieDword& bestSlot) const
{
	int maxSlot = static_cast<int>(Slots.size());
	for (int i = 0; i < maxSlot; i++) {
		// look only at ranged weapons and ranged melee weapons like throwing daggers
		if (!(i >= SLOT_RANGED && i < LAST_RANGED) && !(i >= SLOT_MELEE && i < LAST_MELEE)) {
			continue;
		}

		CREItem* slot;
		const Item* itm = GetItemPointer(i, slot);
		if (!itm) continue;
		// cannot change equipment when holding a cursed weapon
		if (slot->Flags & IE_INV_ITEM_CURSED) {
			bestSlot = SLOT_FIST;
			return false;
		}

		// best ranged
		const ITMExtHeader* header;
		int damage = itm->GetDamagePotential(true, header);
		if (damage > maxDamage) {
			bestSlot = i;
			maxDamage = damage;
		}
		gamedata->FreeItem(itm, slot->ItemResRef, false);
	}
	return maxDamage != -1;
}

bool Inventory::EquipBestWeapon(int flags)
{
	int damage = -1;
	ieDword bestSlot = SLOT_FIST;
	const ITMExtHeader *header;
	CREItem *Slot;

	//cannot change equipment when holding magic weapons
	if (Equipped == SLOT_MAGIC - SLOT_MELEE && !(flags & EQUIP_FORCE)) {
		return false;
	}

	if (flags&EQUIP_RANGED) {
		CanEquipRanged(damage, bestSlot);
		if (int(bestSlot) == SLOT_FIST) return false;
	}

	if (flags&EQUIP_MELEE) {
		for (int i = SLOT_MELEE; i <= LAST_MELEE; i++) {
			const Item *itm = GetItemPointer(i, Slot);
			if (!itm) continue;
			//cannot change equipment when holding a cursed weapon
			if (Slot->Flags & IE_INV_ITEM_CURSED) {
				return false;
			}
			//the Slot flag is enough for this
			//though we need animation type/damagepotential anyway
			if (Slot->Flags&IE_INV_ITEM_BOW) continue;
			//best melee
			int tmp = itm->GetDamagePotential(false, header);
			if (tmp>damage) {
				bestSlot = i;
				damage = tmp;
			}
			gamedata->FreeItem( itm, Slot->ItemResRef, false );
		}
	}

	bool equipped = EquipItem(bestSlot);
	UpdateWeaponAnimation();
	return equipped;
}

// returns true if there are more item usages not fitting in given vector
bool Inventory::GetEquipmentInfo(std::vector<ItemExtHeader>& headerList, int startindex, int count) const
{
	enum class IDNeeded {
		No, // id is not important
		Yes, // id is important
		Never // shouldn't id
	};

	int pos = 0;
	int actual = 0;
	for(unsigned int idx=0;idx<Slots.size();idx++) {
		if (!core->QuerySlotEffects(idx)) {
			continue;
		}
		CREItem *slot;

		const Item *itm = GetItemPointer(idx, slot);
		if (!itm) {
			continue;
		}
		for(size_t ehc = 0; ehc < itm->ext_headers.size(); ++ehc) {
			const ITMExtHeader *ext_header = &itm->ext_headers[ehc];
			if (ext_header->Location!=ITEM_LOC_EQUIPMENT) {
				continue;
			}
			//skipping if we cannot use the item
			int idreq1 = (slot->Flags&IE_INV_ITEM_IDENTIFIED);
			IDNeeded idreq2 = static_cast<IDNeeded>(ext_header->IDReq);
			if (idreq2 == IDNeeded::Never && idreq1) continue;
			if (idreq2 == IDNeeded::Yes && !idreq1) continue;

			actual++;
			if (actual <= startindex) {
				continue;
			}

			// store the item, return if we can't store more
			if (!count) {
				gamedata->FreeItem(itm, slot->ItemResRef, false);
				return true;
			}
			count--;
			headerList[pos].CopyITMExtHeader(*ext_header);
			headerList[pos].itemName = slot->ItemResRef;
			headerList[pos].slot = idx;
			headerList[pos].headerindex = ehc;
			if (!ext_header->Charges) {
				headerList[pos].Charges = 0xffff;
				pos++;
				continue;
			}

			// don't modify ehc, it is a counter
			if (ehc >= slot->Usages.size()) {
				headerList[pos].Charges = slot->Usages[0];
			} else {
				headerList[pos].Charges = slot->Usages[ehc];
			}
			pos++;
		}
		gamedata->FreeItem(itm, slot->ItemResRef, false);
	}

	return false;
}

//The slot index value is optional, if you supply it,
// then ItemExcl will be returned as if the item was already unequipped
ieDword Inventory::GetEquipExclusion(int index) const
{
	if (index<0) {
		return ItemExcl;
	}
	CREItem *slot;
	const Item *itm = GetItemPointer(index, slot);
	if (!itm) {
		return ItemExcl;
	}
	ieDword ret = ItemExcl&~itm->ItemExcl;
	gamedata->FreeItem(itm, slot->ItemResRef, false);
	return ret;
}

void Inventory::UpdateShieldAnimation(const Item *it)
{
	AnimRef AnimationType;
	unsigned char WeaponType = IE_ANI_WEAPON_1H;

	if (it) {
		AnimationType = it->AnimationType;
		if (core->CheckItemType(it, SLOT_WEAPON)) {
			WeaponType = IE_ANI_WEAPON_2W;
		}
	}
	Owner->SetUsedShield(AnimationType, WeaponType);
}

void Inventory::UpdateWeaponAnimation()
{
	int slot = GetEquippedSlot();
	int effect = core->QuerySlotEffects( slot );
	if (effect == SLOT_EFFECT_MISSILE) {
		// ranged weapon
		slot = FindRangedWeapon();
	}

	AnimRef AnimationType;
	std::array<ieWord, 3> meleeAnimation = { 100, 0, 0 };
	CREItem *Slot;

	const ITMExtHeader *header = nullptr;
	const Item *itm = GetItemPointer(slot, Slot);
	if (!itm) {
		return;
	}

	itm->GetDamagePotential(false, header);
	AnimationType = itm->AnimationType;
	unsigned char WeaponType = IE_ANI_WEAPON_INVALID;
	// for twohanded flag, you don't need itm
	if (Slot->Flags & IE_INV_ITEM_TWOHANDED) {
		WeaponType = IE_ANI_WEAPON_2H;
	} else {
		// Examine shield slot to check if we're using two weapons
		int shieldSlot = GetShieldSlot();
		const CREItem* si = nullptr;
		if (shieldSlot > 0) {
			si = GetSlotItem(static_cast<ieDword>(shieldSlot));
		}
		if (si) {
			const Item* it = gamedata->GetItem(si->ItemResRef, true);
			assert(it);
			if (core->CheckItemType(it, SLOT_WEAPON)) {
				WeaponType = IE_ANI_WEAPON_2W;
			}
			gamedata->FreeItem(it, si->ItemResRef, false);
		}

		if (WeaponType == IE_ANI_WEAPON_INVALID) {
			WeaponType = IE_ANI_WEAPON_1H;
		}
	}

	if (header) meleeAnimation = header->MeleeAnimation;
	if (itm)
		gamedata->FreeItem( itm, Slot->ItemResRef, false );
	Owner->SetUsedWeapon(AnimationType, meleeAnimation, WeaponType);
}

//this function will also check disabled slots (if that feature will be imped)
bool Inventory::IsSlotBlocked(int slot) const
{
	if (slot<SLOT_MELEE) return false;
	if (slot>LAST_MELEE) return false;
	int otherslot;
	if (IWD2) {
		otherslot = slot+1;
	} else {
		otherslot = SLOT_LEFT;
	}
	return !IsSlotEmpty(otherslot);
}

inline HCStrings Inventory::CanUseShieldSlot(int slot, bool ranged) const
{
	const CREItem* item = GetSlotItem(slot);
	if (!item) return HCStrings::count;

	if (item->Flags & IE_INV_ITEM_TWOHANDED) {
		return HCStrings::TwohandedUsed;
	} else if (item->Flags & IE_INV_ITEM_NOT_OFFHAND) {
		return HCStrings::TwohandedUsed;
	} else if (ranged) {
		return HCStrings::NoRangedOffhand;
	}
	return HCStrings::count;
}

HCStrings Inventory::WhyCantEquip(int slot, int twohanded, bool ranged) const
{
	// check only for hand slots
	if (IWD2) {
		if (slot < SLOT_MELEE || slot > LAST_MELEE + 1) { // up to last adjacent shield slot
			return HCStrings::count;
		}
	} else if ((slot < SLOT_MELEE || slot > LAST_MELEE) && slot != SLOT_LEFT) {
		return HCStrings::count;
	}

	//magic items have the highest priority
	if (MagicSlotEquipped()) {
		//magic weapon is in use
		return HCStrings::MagicWeapon;
	}

	//can't equip in shield slot if a weapon slot is twohanded or ranged
	for (int i=SLOT_MELEE; i<=LAST_MELEE;i++) {
		//see GetShieldSlot
		int otherslot;
		if (IWD2) {
			otherslot = i+1;
		} else {
			otherslot = SLOT_LEFT;
		}
		if (slot != otherslot) continue;

		HCStrings shieldSlotUsable = CanUseShieldSlot(i, ranged);
		if (shieldSlotUsable != HCStrings::count) {
			return shieldSlotUsable;
		}
	}

	if (twohanded) {
		if (IWD2) {
			if (slot>=SLOT_MELEE&&slot<=LAST_MELEE && (slot-SLOT_MELEE)&1) {
				return HCStrings::NotInOffhand;
			}
		} else if (slot == SLOT_LEFT) {
			return HCStrings::NotInOffhand;
		}
		if (IsSlotBlocked(slot)) {
		//cannot equip two handed while shield slot is in use?
			return HCStrings::OffhandUsed;
		}
	}
	return HCStrings::count;
}

//recharge items on rest, if rest was partial, recharge only 'hours'
//if this latter functionality is unwanted, then simply don't recharge if
//hours != 0
void Inventory::ChargeAllItems(int hours) const
{
	//this loop is going from start
	for (auto item : Slots) {
		if (!item) {
			continue;
		}

		const Item *itm = gamedata->GetItem(item->ItemResRef, true);
		if (!itm) continue;
		for (size_t h = 0; h < item->Usages.size(); h++) {
			const ITMExtHeader *header = itm->GetExtHeader(h);
			if (!header || !(header->RechargeFlags & IE_ITEM_RECHARGE)) {
				continue;
			}

			unsigned short add = header->Charges;
			if (hours && add > hours) add = hours;
			item->Usages[h] = std::min<ieWord>(add + item->Usages[h], header->Charges);
		}
		gamedata->FreeItem( itm, item->ItemResRef, false );
	}
}

#define ITM_STEALING (IE_INV_ITEM_UNSTEALABLE | IE_INV_ITEM_MOVABLE | IE_INV_ITEM_EQUIPPED) //0x442
int Inventory::FindStealableItem()
{
	unsigned int slotcnt = Slots.size();
	int start = core->Roll(1, slotcnt, -1);
	int inc = start & 1 ? 1 : -1;

	Log(DEBUG, "Inventory", "Start Slot: {}, increment: {}", start, inc);
	for (unsigned int i = 0; i < slotcnt; ++i) {
		int slot = ((signed) slotcnt - 1 + start + (signed) i * inc) % slotcnt;
		const CREItem *item = Slots[slot];
		//can't steal empty slot
		if (!item) continue;
		//bit 1 is stealable slot
		if (!(core->QuerySlotFlags(slot)&1) ) continue;
		//can't steal equipped weapon
		int realslot = core->QuerySlot(slot);
		if (GetEquippedSlot() == realslot) continue;
		if (GetShieldSlot() == realslot) continue;
		//can't steal flagged items
		if ((item->Flags & ITM_STEALING) != IE_INV_ITEM_MOVABLE) continue;
		return slot;
	}
	return -1;
}

// extension to allow more or less than head gear to avert critical hits:
// If an item with bit 25 set is equipped in a non-helmet slot, aversion is enabled
// If an item with bit 25 set is equipped in a helmet slot, aversion is disabled
bool Inventory::ProvidesCriticalAversion() const
{
	int maxSlot = (int) Slots.size();
	for (int i = 0; i < maxSlot; i++) {
		const CREItem *item = Slots[i];
		if (!item || InBackpack(i)) { // ignore items in the backpack
			continue;
		}
		// weapon, but not equipped
		if (!((i == SLOT_ARMOR) || (i == SLOT_HEAD)) && !(item->Flags & IE_INV_ITEM_EQUIPPED)) {
			continue;
		}

		const Item *itm = gamedata->GetItem(item->ItemResRef, true);
		if (!itm) {
			continue;
		}
		//if the item is worn on head, toggle crits must be 0, otherwise it must be 1
		//this flag is only stored in the item header, so we need to make some efforts
		//to get to it - low 24 bits
		ieDword flag = itm->Flags;
		gamedata->FreeItem( itm, item->ItemResRef, false );
		bool togglesCrits = (flag&IE_ITEM_TOGGLE_CRITS);
		bool isHelmet = (i == SLOT_HEAD);
		if (togglesCrits ^ isHelmet) return true;
	}
	return false;
}

int Inventory::MergeItems(int slot, CREItem *item)
{
	CREItem *slotitem = Slots[slot];
	if (slotitem->MaxStackAmount && ItemsAreCompatible(slotitem, item)) {
		//calculate with the max movable stock
		int chunk = item->Usages[0];
		if (slotitem->Usages[0] + chunk > slotitem->MaxStackAmount) {
			chunk = slotitem->MaxStackAmount - slotitem->Usages[0];
		}
		if (chunk<=0) {
			return ASI_FAILED;
		}

		slotitem->Flags |= IE_INV_ITEM_ACQUIRED;
		slotitem->Usages[0] = (ieWord) (slotitem->Usages[0] + chunk);
		item->Usages[0] = (ieWord) (item->Usages[0] - chunk);
		if (Owner) EquipItem(slot);
		CalculateWeight();
		if (item->Usages[0] == 0) {
			delete item;
			return ASI_SUCCESS;
		}
		return ASI_PARTIAL;
	}
	return ASI_FAILED;
}

int Inventory::InBackpack(int slot) const
{
	if (static_cast<size_t>(slot) >= Slots.size()) {
		InvalidSlot(slot);
	}
	return slot >= SLOT_INV && slot <= LAST_INV;
}

}
