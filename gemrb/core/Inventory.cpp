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

#include "win32def.h"
#include "strrefs.h"

#include "CharAnimations.h"
#include "DisplayMessage.h"
#include "Game.h"
#include "GameData.h"
#include "Interface.h"
#include "Item.h"
#include "Map.h"
#include "ScriptEngine.h"
#include "Scriptable/Actor.h"
#include "System/StringBuffer.h"

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
static int MagicBit = 0;

static void InvalidSlot(int slot)
{
	error("Inventory", "Invalid slot: %d!\n", slot);
}

//This inline function returns both an item pointer and the slot data.
//slot is a dynamic slot number (SLOT_*)
inline Item *Inventory::GetItemPointer(ieDword slot, CREItem *&item) const
{
	item = GetSlotItem(slot);
	if (!item) return NULL;
	if (!item->ItemResRef[0]) return NULL;
	return gamedata->GetItem(item->ItemResRef);
}

void Inventory::Init(int mb)
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
	//TODO: set this correctly
	IWD2 = false;
	MagicBit = mb;
}

Inventory::Inventory()
{
	Owner = NULL;
	InventoryType = INVENTORY_HEAP;
	Changed = false;
	Weight = 0;
	Equipped = IW_NO_EQUIPPED;
	EquippedHeader = 0;
	ItemExcl = 0;
	memset(ItemTypes, 0, sizeof(ItemTypes));
}

Inventory::~Inventory()
{
	for (size_t i = 0; i < Slots.size(); i++) {
		if (Slots[i]) {
			delete( Slots[i] );
			Slots[i] = NULL;
		}
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
	CREItem *tmp, *item;
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

	Changed = true;
	CalculateWeight();
}

CREItem *Inventory::GetItem(unsigned int slot)
{
	if (slot >= Slots.size() ) {
		InvalidSlot(slot);
		return NULL;
	}
	CREItem *item = Slots[slot];
	Slots.erase(Slots.begin()+slot);
	return item;
}

//This hack sets the charge counters for non-rechargeable items,
//if their charge is zero
static inline void HackCharges(CREItem *item)
{
	Item *itm = gamedata->GetItem( item->ItemResRef );
	if (itm) {
		for (int i=0;i<3;i++) {
			if (item->Usages[i]) {
				continue;
			}
			ITMExtHeader *h = itm->GetExtHeader(i);
			if (h && !(h->RechargeFlags&IE_ITEM_RECHARGE)) {
				//HACK: the original (bg2) allows for 0 charged gems
				if (h->Charges) {
					item->Usages[i] = h->Charges;
				} else {
					item->Usages[i] = 1;
				}
			}
		}
		gamedata->FreeItem( itm, item->ItemResRef, false );
	}
}

void Inventory::AddItem(CREItem *item)
{
	if (!item) return; //invalid items get no slot
	Slots.push_back(item);
	HackCharges(item);
	//this will update the flags (needed for unmovable items in containers)
	//but those *can* be picked up (like the bg2 portal key), so we skip it
	//Changed=true;
}

void Inventory::CalculateWeight() const
{
	if (!Changed) {
		return;
	}
	Weight = 0;
	for (size_t i = 0; i < Slots.size(); i++) {
		CREItem *slot = Slots[i];
		if (!slot) {
			continue;
		}
		if (slot->Weight == -1) {
			Item *itm = gamedata->GetItem( slot->ItemResRef );
			if (itm) {
				//simply adding the item flags to the slot
				slot->Flags |= (itm->Flags<<8);
				//some slot flags might be affected by the item flags
				if (!(slot->Flags & IE_INV_ITEM_CRITICAL)) {
					slot->Flags |= IE_INV_ITEM_DESTRUCTIBLE;
				}
				//this is for converting IWD items magic flag
				if (MagicBit) {
					if (slot->Flags&IE_INV_ITEM_UNDROPPABLE) {
						slot->Flags|=IE_INV_ITEM_MAGICAL;
						slot->Flags&=~IE_INV_ITEM_UNDROPPABLE;
					}
				}

				if (!(slot->Flags & IE_INV_ITEM_MOVABLE)) {
					slot->Flags |= IE_INV_ITEM_UNDROPPABLE;
				}

				if (slot->Flags & IE_INV_ITEM_STOLEN2) {
					slot->Flags |= IE_INV_ITEM_STOLEN;
				}

				//auto identify basic items
				if (!itm->LoreToID) {
					slot->Flags |= IE_INV_ITEM_IDENTIFIED;
				}

				//if item is stacked mark it as so
				if (itm->MaxStackAmount) {
					slot->Flags |= IE_INV_ITEM_STACKED;
				}

				slot->Weight = itm->Weight;
				slot->MaxStackAmount = itm->MaxStackAmount;
				gamedata->FreeItem( itm, slot->ItemResRef, false );
			}
			else {
				Log(ERROR, "Inventory", "Invalid item: %s!",
					slot->ItemResRef);
				slot->Weight = 0;
			}
		} else {
			slot->Flags &= ~IE_INV_ITEM_ACQUIRED;
		}
		if (slot->Weight > 0) {
			Weight += slot->Weight * ((slot->Usages[0] && slot->MaxStackAmount) ? slot->Usages[0] : 1);
		}
	}
	Changed = false;
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
	if (pos<4) {
		ItemTypes[pos]|=1<<bit;
	}

	ieWord gradient = itm->GetWieldedGradient();
	if (gradient!=0xffff) {
		Owner->SetBase(IE_COLORS, gradient);
	}

	//get the equipping effects
	EffectQueue *eqfx = itm->GetEffectBlock(Owner, Owner->Pos, -1, index, 0);
	gamedata->FreeItem( itm, slot->ItemResRef, false );

	Owner->RefreshEffects(eqfx);
	//call gui for possible paperdoll animation changes
	if (Owner->InParty) {
		core->SetEventFlag(EF_UPDATEANIM);
	}
}

//no need to know the item effects 'personally', the equipping slot
//is stored in them
void Inventory::RemoveSlotEffects(ieDword index)
{
	Owner->fxqueue.RemoveEquippingEffects(index);
	Owner->RefreshEffects(NULL);
	//call gui for possible paperdoll animation changes
	if (Owner->InParty) {
		core->SetEventFlag(EF_UPDATEANIM);
	}
}

void Inventory::SetInventoryType(int arg)
{
	InventoryType = arg;
}

void Inventory::SetSlotCount(unsigned int size)
{
	if (Slots.size()) {
		error("Core", "Inventory size changed???\n");
		//we don't allow reassignment,
		//if you want this, delete the previous Slots here
	}
	Slots.assign((size_t) size, NULL);
}

/** if you supply a "" string, then it checks if the slot is empty */
bool Inventory::HasItemInSlot(const char *resref, unsigned int slot) const
{
	if (slot >= Slots.size()) {
		return false;
	}
	const CREItem *item = Slots[slot];
	if (!item) {
		return false;
	}
	if (!resref[0]) {
		return true;
	}
	if (strnicmp( item->ItemResRef, resref, 8 )==0) {
		return true;
	}
	return false;
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
int Inventory::CountItems(const char *resref, bool stacks) const
{
	int count = 0;
	size_t slot = Slots.size();
	while(slot--) {
		const CREItem *item = Slots[slot];
		if (!item) {
			continue;
		}
		if (resref && resref[0]) {
			if (strnicmp(resref, item->ItemResRef, 8) )
				continue;
		}
		if (stacks && (item->Flags&IE_INV_ITEM_STACKED) ) {
			count+=item->Usages[0];
		}
		else {
			count++;
		}
	}
	return count;
}

/** this function can look for stolen, equipped, identified, destructible
		etc, items. You just have to specify the flags in the bitmask
		specifying 1 in a bit signifies a requirement */
bool Inventory::HasItem(const char *resref, ieDword flags) const
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
		if (resref[0] && strnicmp(item->ItemResRef, resref,8) ) {
			continue;
		}
		return true;
	}
	return false;
}

void Inventory::KillSlot(unsigned int index)
{
	if (InventoryType==INVENTORY_HEAP) {
		Slots.erase(Slots.begin()+index);
		return;
	}
	CREItem *item = Slots[index];
	if (!item) {
		return;
	}

	//the used up item vanishes from the quickslot bar
	if (Owner->IsSelected()) {
		core->SetEventFlag( EF_ACTION );
	}

	Slots[index] = NULL;
	int effect = core->QuerySlotEffects( index );
	if (!effect) {
		return;
	}
	RemoveSlotEffects( index );
	Item *itm = gamedata->GetItem(item->ItemResRef);
	//this cannot happen, but stuff happens!
	if (!itm) {
		return;
	}
	ItemExcl &= ~itm->ItemExcl;
	int eqslot = GetEquippedSlot();

	switch (effect) {
		case SLOT_EFFECT_LEFT:
			UpdateShieldAnimation(0);
			break;
		case SLOT_EFFECT_MISSILE:
			//getting a new projectile of the same type
			if (eqslot == (int) index) {
				if (Equipped < 0) {
					//always get the projectile weapon header (this quiver was equipped)
					ITMExtHeader *header = itm->GetWeaponHeader(true);
					Equipped = FindRangedProjectile(header->ProjectileQualifier);
					if (Equipped!=IW_NO_EQUIPPED) {
						EquipItem(GetEquippedSlot());
					} else {
						EquipItem(SLOT_FIST);
					}
				}
			}
			UpdateWeaponAnimation();
			break;
		case SLOT_EFFECT_MELEE:
			// reset Equipped if it was the removed item
			if (eqslot == (int)index)
				Equipped = IW_NO_EQUIPPED;
			else if (Equipped < 0) {
				//always get the projectile weapon header (this is a bow, because Equipped is negative)
				ITMExtHeader *header = itm->GetWeaponHeader(true);
				if (header) {
					//find the equipped type
					int type = header->ProjectileQualifier;
					int weaponslot = FindTypedRangedWeapon(type);
					CREItem *item2 = Slots[weaponslot];
					if (item2) {
						Item *itm2 = gamedata->GetItem(item2->ItemResRef);
						if (itm2) {
							if (type == header->ProjectileQualifier) {
								Equipped = FindRangedProjectile(header->ProjectileQualifier);
								if (Equipped!=IW_NO_EQUIPPED) {
									EquipItem(GetEquippedSlot());
								} else {
									EquipItem(SLOT_FIST);
								}
							}
							gamedata->FreeItem(itm2, item2->ItemResRef, false);
						}
					}
				}
			}
			// reset Equipped if it is a ranged weapon slot
			// but not magic weapon slot!

			UpdateWeaponAnimation();
			break;
		case SLOT_EFFECT_HEAD:
			Owner->SetUsedHelmet("");
			break;
		case SLOT_EFFECT_ITEM:
			//remove the armor type only if this item is responsible for it
			if ((ieDword) (itm->AnimationType[0]-'1') == Owner->GetBase(IE_ARMOR_TYPE)) {
				Owner->SetBase(IE_ARMOR_TYPE, 0);
			}
			break;
	}
	gamedata->FreeItem(itm, item->ItemResRef, false);
}
/** if resref is "", then destroy ALL items
this function can look for stolen, equipped, identified, destructible
etc, items. You just have to specify the flags in the bitmask
specifying 1 in a bit signifies a requirement */
unsigned int Inventory::DestroyItem(const char *resref, ieDword flags, ieDword count)
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
		if (resref[0] && strnicmp(item->ItemResRef, resref, 8) ) {
			continue;
		}
		//we need to acknowledge that the item was destroyed
		//use unequip stuff, decrease encumbrance etc,
		//until that, we simply erase it
		ieDword removed;

		if (item->Flags&IE_INV_ITEM_STACKED) {
			removed=item->Usages[0];
			if (count && (removed + destructed > count) ) {
				removed = count - destructed;
				item = RemoveItem( (unsigned int) slot, removed );
			}
			else {
				KillSlot( (unsigned int) slot);
			}
		} else {
			removed=1;
			KillSlot( (unsigned int) slot);
		}
		delete item;
		Changed = true;
		destructed+=removed;
		if (count && (destructed>=count) )
			break;
	}
	if (Changed && Owner && Owner->InParty) displaymsg->DisplayConstantString(STR_LOSTITEM, DMC_BG2XPGREEN);

	return destructed;
}

CREItem *Inventory::RemoveItem(unsigned int slot, unsigned int count)
{
	CREItem *item;

	if (slot >= Slots.size() ) {
		InvalidSlot(slot);
		return NULL;
	}
	Changed = true;
	item = Slots[slot];

	if (!item) {
		return NULL;
	}

	if (!count || !(item->Flags&IE_INV_ITEM_STACKED) ) {
		KillSlot(slot);
		return item;
	}
	if (count >= item->Usages[0]) {
		KillSlot(slot);
		return item;
	}

	CREItem *returned = new CREItem(*item);
	item->Usages[0]-=count;
	returned->Usages[0]=(ieWord) count;
	return returned;
}

//flags set disable item transfer
//except for undroppable and equipped, which are opposite (and shouldn't be set)
int Inventory::RemoveItem(const char *resref, unsigned int flags, CREItem **res_item, int count)
{
	size_t slot = Slots.size();
	unsigned int mask = (flags^(IE_INV_ITEM_UNDROPPABLE|IE_INV_ITEM_EQUIPPED));
	if (core->HasFeature(GF_NO_DROP_CAN_MOVE) ) {
		mask &= ~IE_INV_ITEM_UNDROPPABLE;
	}
	while(slot--) {
		CREItem *item = Slots[slot];
		if (!item) {
			continue;
		}

		if (flags && (mask&item->Flags)==flags) {
			continue;
		}
		if (!flags && (mask&item->Flags)!=0) {
			continue;
		}
		if (resref[0] && strnicmp(item->ItemResRef, resref, 8) ) {
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
		return;
	}
	Changed = true;
	if (Slots[slot]) {
		delete Slots[slot];
	}

	HackCharges(item);

	Slots[slot] = item;

	//update the action bar next time
	if (Owner->IsSelected()) {
		core->SetEventFlag( EF_ACTION );
	}
}

int Inventory::AddSlotItem(CREItem* item, int slot, int slottype)
{
	int twohanded = item->Flags&IE_INV_ITEM_TWOHANDED;
	if (slot >= 0) {
		if ((unsigned)slot >= Slots.size()) {
			InvalidSlot(slot);
			return ASI_FAILED;
		}

		//check for equipping weapons
		if (WhyCantEquip(slot,twohanded)) {
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

	bool which;
	if (slot==SLOT_AUTOEQUIP) {
		which=true;
	} else {
		which=false;
	}
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
		if (i>=SLOT_MELEE && i<=LAST_MELEE) {
			if (Owner->GetQuickSlot(i-SLOT_MELEE)==0xffff) {
				continue;
			}
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

int Inventory::AddStoreItem(STOItem* item, int action)
{
	CREItem *temp;
	int ret = -1;

	// item->PurchasedAmount is the number of items bought
	// (you can still add grouped objects in a single step,
	// just set up STOItem)
	while (item->PurchasedAmount) {
		//the first part of a STOItem is essentially a CREItem
		temp = new CREItem();
		memcpy( temp, item, sizeof( CREItem ) );
		//except the Expired flag
		temp->Expired=0;
		if (action==STA_STEAL) {
			temp->Flags |= IE_INV_ITEM_STOLEN;
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
	CalculateWeight();
	return ret;
}

/* could the source item be dropped on the target item to merge them */
bool Inventory::ItemsAreCompatible(CREItem* target, CREItem* source) const
{
	if (!target) {
		//this isn't always ok, please check!
		Log(WARNING, "Inventory", "Null item encountered by ItemsAreCompatible()");
		return true;
	}

	if (!(source->Flags&IE_INV_ITEM_STACKED) ) {
		return false;
	}

	if (!strnicmp( target->ItemResRef, source->ItemResRef,8 )) {
		return true;
	}
	return false;
}

//depletes a magical item
//if flags==0 then magical weapons are not harmed
int Inventory::DepleteItem(ieDword flags)
{
	for (size_t i = 0; i < Slots.size(); i++) {
		CREItem *item = Slots[i];
		if (!item) {
			continue;
		}

		//don't harm critical items
		//don't harm nonmagical items
		//don't harm indestructible items
		if ( (item->Flags&(IE_INV_ITEM_CRITICAL|IE_INV_DEPLETABLE)) != IE_INV_DEPLETABLE) {
			continue;
		}

		//if flags = 0 then weapons are not depleted
		if (!flags) {
			Item *itm = gamedata->GetItem( item->ItemResRef );
			if (!itm)
				continue;
			//if the item is usable in weapon slot, then it is weapon
			int weapon = core->CanUseItemType( SLOT_WEAPON, itm );
			gamedata->FreeItem( itm, item->ItemResRef, false );
			if (weapon)
				continue;
		}
		//deplete item
		item->Usages[0]=0;
		item->Usages[1]=0;
		item->Usages[2]=0;
	}
	return -1;
}

// if flags is 0, skips undroppable items
// if flags is IE_INV_ITEM_UNDROPPABLE, doesn't skip undroppable items
// TODO: once all callers have been checked, this can be reversed to make more sense
int Inventory::FindItem(const char *resref, unsigned int flags) const
{
	unsigned int mask = (flags^IE_INV_ITEM_UNDROPPABLE);
	if (core->HasFeature(GF_NO_DROP_CAN_MOVE) ) {
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
		if (resref[0] && strnicmp(item->ItemResRef, resref, 8) ) {
			continue;
		}
		return (int) i;
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
	Changed = true;
	KillSlot(slot);
	return true;
}

bool Inventory::DropItemAtLocation(const char *resref, unsigned int flags, Map *map, const Point &loc)
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
		//if you want to drop undoppable items, simply set IE_INV_UNDROPPABLE
		//by default, it won't drop them
		if ( ((flags^IE_INV_ITEM_UNDROPPABLE)&item->Flags)!=flags) {
				continue;
		}
		if (resref[0] && strnicmp(item->ItemResRef, resref, 8) ) {
			continue;
		}
		// mark it as unequipped, so it doesn't cause problems in stores
		item->Flags &= ~ IE_INV_ITEM_EQUIPPED;
		map->AddItemToLocation(loc, item);
		Changed = true;
		dropped = true;
		KillSlot((unsigned int) i);
		//if it isn't all items then we stop here
		if (resref[0])
			break;
	}

	//dropping gold too
	if (!resref[0]) {
		if (Owner->Type==ST_ACTOR) {
			Actor *act = (Actor *) Owner;
			if (! act->BaseStats[IE_GOLD]) {
				return dropped;
			}
			CREItem *gold = new CREItem();
		
			gold->Expired=0;
			gold->Flags=0;
			gold->Usages[1]=0;
			gold->Usages[2]=0;
			memcpy(gold->ItemResRef, core->GoldResRef, sizeof(ieResRef) );
			gold->Usages[0] = act->BaseStats[IE_GOLD];
			act->BaseStats[IE_GOLD] = 0;
			map->AddItemToLocation(loc, gold);
		}
	}
	return dropped;
}

CREItem *Inventory::GetSlotItem(ieDword slot) const
{
	if (slot >= Slots.size() ) {
		InvalidSlot(slot);
		return NULL;
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

bool Inventory::ChangeItemFlag(ieDword slot, ieDword arg, int op)
{
	CREItem *item = GetSlotItem(slot);
	if (!item) {
		return false;
	}
	switch (op) {
	case BM_SET: item->Flags = arg; break;
	case BM_OR: item->Flags |= arg; break;
	case BM_NAND: item->Flags &= ~arg; break;
	case BM_XOR: item->Flags ^= arg; break;
	case BM_AND: item->Flags &= arg; break;
	}
	return true;
}

//this is the low level equipping
//all checks have been made previously
bool Inventory::EquipItem(ieDword slot)
{
	ITMExtHeader *header;

	if (!Owner) {
		//maybe assertion too?
		return false;
	}
	CREItem *item = GetSlotItem(slot);
	if (!item) {
		return false;
	}

	int weaponslot;

	// add effects of an item just being equipped to actor's effect queue
	int effect = core->QuerySlotEffects( slot );
	Item *itm = gamedata->GetItem(item->ItemResRef);
	if (!itm) {
		print("Invalid item Equipped: %s Slot: %d", item->ItemResRef, slot);
		return false;
	}
	switch (effect) {
	case SLOT_EFFECT_LEFT:
		//no idea if the offhand weapon has style, or simply the right
		//hand style is dominant
		UpdateShieldAnimation(itm);
		break;
	case SLOT_EFFECT_MELEE:
		//if weapon is bow, then find quarrel for it and equip that
		weaponslot = GetWeaponQuickSlot(slot);
		EquippedHeader = 0;
		header = itm->GetExtHeader(EquippedHeader);
		if (header) {
			if (header->AttackType == ITEM_AT_BOW) {
				//find the ranged projectile associated with it, this returns equipped code
				Equipped = FindRangedProjectile(header->ProjectileQualifier);
				//this is the real item slot of the quarrel
				slot = Equipped + SLOT_MELEE;
			} else {
				//this is always 0-3
				Equipped = weaponslot;
				slot = GetWeaponSlot(weaponslot);
			}
			if (Equipped != IW_NO_EQUIPPED) {
				Owner->SetupQuickSlot(ACT_WEAPON1+weaponslot, slot, EquippedHeader);
			}
			SetEquippedSlot(Equipped, EquippedHeader);
			//don't clear effect in case of a launcher, we need to find it and add its effects too
			//slot is 'negative' for launchers
			if ((int) Equipped>=0) {
				effect = 0; // SetEquippedSlot will already call AddSlotEffects
			} else {
				effect = SLOT_EFFECT_MISSILE;
			}
			UpdateWeaponAnimation();
		}
		break;
	case SLOT_EFFECT_MISSILE:
		//Get the ranged header of the projectile (so we theoretically allow shooting of daggers)
		EquippedHeader = itm->GetWeaponHeaderNumber(true);
		header = itm->GetExtHeader(EquippedHeader);
		if (header) {
			weaponslot = FindTypedRangedWeapon(header->ProjectileQualifier);
			if (weaponslot != SLOT_FIST) {
				weaponslot -= SLOT_MELEE;
				SetEquippedSlot((ieWordSigned) (slot-SLOT_MELEE), EquippedHeader);
				//It is unsure if we can have multiple equipping headers for bows/arrow
				//It is unclear which item's header index should go there
				Owner->SetupQuickSlot(ACT_WEAPON1+weaponslot, slot, 0);
			}
			UpdateWeaponAnimation();
		}
		break;
	case SLOT_EFFECT_HEAD:
		Owner->SetUsedHelmet(itm->AnimationType);
		break;
	case SLOT_EFFECT_ITEM:
		//adjusting armour level if needed
		{
			int l = itm->AnimationType[0]-'1';
			if (l>=0 && l<=3) {
				Owner->SetBase(IE_ARMOR_TYPE, l);
			} else {
				UpdateShieldAnimation(itm);
			}
		}
		break;
	}
	gamedata->FreeItem(itm, item->ItemResRef, false);
	if (effect) {
		if (item->Flags & IE_INV_ITEM_CURSED) {
			item->Flags|=IE_INV_ITEM_UNDROPPABLE;
		}
		if (effect == SLOT_EFFECT_MISSILE) {
			slot = FindRangedWeapon();
		}
		AddSlotEffects( slot );
	}
	return true;
}

//the removecurse flag will check if it is possible to move the item to the inventory
//after a remove curse spell
bool Inventory::UnEquipItem(ieDword slot, bool removecurse)
{
	CREItem *item = GetSlotItem(slot);
	if (!item) {
		return false;
	}
	if (removecurse) {
		if (item->Flags & IE_INV_ITEM_MOVABLE) {
			item->Flags&=~IE_INV_ITEM_UNDROPPABLE;
		}
		if (FindCandidateSlot(SLOT_INVENTORY,0,item->ItemResRef)<0) {
			return false;
		}
	}
	if (!core->HasFeature(GF_NO_DROP_CAN_MOVE) || (item->Flags&IE_INV_ITEM_CURSED) ) {
		if (item->Flags & IE_INV_ITEM_UNDROPPABLE ) {
			return false;
		}
	}
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
		ITMExtHeader *ext_header = itm->GetExtHeader(0);
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
	Item *itm = GetItemPointer(slot, Slot);
	if (!itm) return SLOT_FIST;

	//always look for a ranged header when looking for a projectile/projector
	ITMExtHeader *ext_header = itm->GetWeaponHeader(true);
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
		ITMExtHeader *ext_header = itm->GetWeaponHeader(true);
		int weapontype = 0;
		if (ext_header) {
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
		IWD2=true;
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

bool Inventory::SetEquippedSlot(ieWordSigned slotcode, ieWord header)
{
	EquippedHeader = header;

	//doesn't work if magic slot is used, refresh the magic slot just in case
	if (HasItemInSlot("",SLOT_MAGIC) && (slotcode!=SLOT_MAGIC-SLOT_MELEE)) {
		Equipped = SLOT_MAGIC-SLOT_MELEE;
		UpdateWeaponAnimation();
		return false;
	}

	//if it is an illegal code, make it fist
	if ((size_t) (GetWeaponSlot(slotcode))>Slots.size()) {
		slotcode=IW_NO_EQUIPPED;
	}

	int oldslot = GetEquippedSlot();
	int newslot = GetWeaponSlot(slotcode);

	//unequipping (fist slot will be used now)
	if (slotcode == IW_NO_EQUIPPED || !HasItemInSlot("", newslot)) {
		if (Equipped != IW_NO_EQUIPPED) {
			RemoveSlotEffects( oldslot);
		}
		Equipped = IW_NO_EQUIPPED;
		//fist slot equipping effects
		AddSlotEffects(SLOT_FIST);
		UpdateWeaponAnimation();
		return true;
	}

	//equipping a weapon, but remove its effects first
	if (Equipped != IW_NO_EQUIPPED) {
		RemoveSlotEffects( oldslot);
	}

	Equipped = slotcode;
	int effects = core->QuerySlotEffects( newslot);
	if (effects) {
		CREItem* item = GetSlotItem(newslot);
		item->Flags|=IE_INV_ITEM_EQUIPPED;
		if (item->Flags & IE_INV_ITEM_CURSED) {
			item->Flags|=IE_INV_ITEM_UNDROPPABLE;
		}
		AddSlotEffects(newslot);
	}
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

//returns the fist weapon if there is nothing else
//This will return the actual weapon, I mean the bow in the case of bow+arrow combination
CREItem *Inventory::GetUsedWeapon(bool leftorright, int &slot) const
{
	CREItem *ret;

	if (SLOT_MAGIC!=-1) {
		slot = SLOT_MAGIC;
		ret = GetSlotItem(slot);
		if (ret && ret->ItemResRef[0]) {
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
int Inventory::FindCandidateSlot(int slottype, size_t first_slot, const char *resref)
{
	if (first_slot >= Slots.size())
		return -1;

	for (size_t i = first_slot; i < Slots.size(); i++) {
		if (!(core->QuerySlotType( (unsigned int) i) & slottype) ) {
			continue;
		}

		CREItem *item = Slots[i];

		if (!item) {
			return (int) i; //this is a good empty slot
		}
		if (!resref) {
			continue;
		}
		if (!(item->Flags&IE_INV_ITEM_STACKED) ) {
			continue;
		}
		if (strnicmp( item->ItemResRef, resref, 8 )!=0) {
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

void Inventory::AddSlotItemRes(const ieResRef ItemResRef, int SlotID, int Charge0, int Charge1, int Charge2)
{
	CREItem *TmpItem = new CREItem();
	strnlwrcpy(TmpItem->ItemResRef, ItemResRef, 8);
	TmpItem->Expired=0;
	TmpItem->Usages[0]=(ieWord) Charge0;
	TmpItem->Usages[1]=(ieWord) Charge1;
	TmpItem->Usages[2]=(ieWord) Charge2;
	TmpItem->Flags=0;
	if (core->ResolveRandomItem(TmpItem)) {
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
	CalculateWeight();
}

void Inventory::SetSlotItemRes(const ieResRef ItemResRef, int SlotID, int Charge0, int Charge1, int Charge2)
{
	if(ItemResRef[0]) {
		CREItem *TmpItem = new CREItem();
		strnlwrcpy(TmpItem->ItemResRef, ItemResRef, 8);
		TmpItem->Expired=0;
		TmpItem->Usages[0]=(ieWord) Charge0;
		TmpItem->Usages[1]=(ieWord) Charge1;
		TmpItem->Usages[2]=(ieWord) Charge2;
		TmpItem->Flags=0;
		if (core->ResolveRandomItem(TmpItem)) {
			SetSlotItem( TmpItem, SlotID );
		} else {
			delete TmpItem;
		}
	} else {
		//if the item isn't creatable, we still destroy the old item
		KillSlot( SlotID );
	}
	CalculateWeight();
}

ieWord Inventory::GetShieldItemType() const
{
	ieWord ret;
	CREItem *Slot;

	const Item *itm = GetItemPointer(GetShieldSlot(), Slot);
	if (!itm) return 0xffff;
	ret = itm->ItemType;
	gamedata->FreeItem( itm, Slot->ItemResRef, true );
	return ret;
}

ieWord Inventory::GetArmorItemType() const
{
	ieWord ret;
	CREItem *Slot;

	const Item *itm = GetItemPointer(GetArmorSlot(), Slot);
	if (!itm) return 0xffff;
	ret = itm->ItemType;
	gamedata->FreeItem( itm, Slot->ItemResRef, true );
	return ret;
}

void Inventory::BreakItemSlot(ieDword slot)
{
	ieResRef newItem;
	CREItem *Slot;

	const Item *itm = GetItemPointer(slot, Slot);
	if (!itm) return;
	//if it is the magic weapon slot, don't break it, just remove it, because it couldn't be removed
	if(slot ==(unsigned int) SLOT_MAGIC) {
		newItem[0]=0;
	} else {
		memcpy(newItem, itm->ReplacementItem,sizeof(newItem) );
	}
	gamedata->FreeItem( itm, Slot->ItemResRef, true );
	//this depends on setslotitemres using setslotitem
	SetSlotItemRes(newItem, slot, 0,0,0);
}

void Inventory::dump() const
{
	StringBuffer buffer;
	dump(buffer);
	Log(DEBUG, "Inventory", buffer);
}

void Inventory::dump(StringBuffer& buffer) const
{
	buffer.append( "INVENTORY:\n" );
	for (unsigned int i = 0; i < Slots.size(); i++) {
		CREItem* itm = Slots[i];

		if (!itm) {
			continue;
		}

		buffer.appendFormatted( "%2u: %8.8s - (%d %d %d) Fl:0x%x Wt: %d x %dLb\n", i, itm->ItemResRef, itm->Usages[0], itm->Usages[1], itm->Usages[2], itm->Flags, itm->MaxStackAmount, itm->Weight );
	}

	buffer.appendFormatted( "Equipped: %d\n", Equipped );
	Changed = true;
	CalculateWeight();
	buffer.appendFormatted( "Total weight: %d\n", Weight );
}

void Inventory::EquipBestWeapon(int flags)
{
	int i;
	int damage = -1;
	ieDword best_slot = SLOT_FIST;
	ITMExtHeader *header;
	CREItem *Slot;
	char AnimationType[2]={0,0};
	ieWord MeleeAnimation[3]={100,0,0};

	//cannot change equipment when holding magic weapons
	if (Equipped == SLOT_MAGIC-SLOT_MELEE) {
		return;
	}

	if (flags&EQUIP_RANGED) {
		for(i=SLOT_RANGED;i<LAST_RANGED;i++) {
			const Item *itm = GetItemPointer(i, Slot);
			if (!itm) continue;
			//best ranged
			int tmp = itm->GetDamagePotential(true, header);
			if (tmp>damage) {
				best_slot = i;
				damage = tmp;
				memcpy(AnimationType,itm->AnimationType,sizeof(AnimationType) );
				memcpy(MeleeAnimation,header->MeleeAnimation,sizeof(MeleeAnimation) );
			}
			gamedata->FreeItem( itm, Slot->ItemResRef, false );
		}

		//ranged melee weapons like throwing daggers (not bows!)
		for(i=SLOT_MELEE;i<=LAST_MELEE;i++) {
			const Item *itm = GetItemPointer(i, Slot);
			if (!itm) continue;
			//best ranged
			int tmp = itm->GetDamagePotential(true, header);
			if (tmp>damage) {
				best_slot = i;
				damage = tmp;
				memcpy(AnimationType,itm->AnimationType,sizeof(AnimationType) );
				memcpy(MeleeAnimation,header->MeleeAnimation,sizeof(MeleeAnimation) );
			}
			gamedata->FreeItem( itm, Slot->ItemResRef, false );
		}
	}

	if (flags&EQUIP_MELEE) {
		for(i=SLOT_MELEE;i<=LAST_MELEE;i++) {
			const Item *itm = GetItemPointer(i, Slot);
			if (!itm) continue;
			//the Slot flag is enough for this
			//though we need animation type/damagepotential anyway
			if (Slot->Flags&IE_INV_ITEM_BOW) continue;
			//best melee
			int tmp = itm->GetDamagePotential(false, header);
			if (tmp>damage) {
				best_slot = i;
				damage = tmp;
				memcpy(AnimationType,itm->AnimationType,sizeof(AnimationType) );
				memcpy(MeleeAnimation,header->MeleeAnimation,sizeof(MeleeAnimation) );
			}
			gamedata->FreeItem( itm, Slot->ItemResRef, false );
		}
	}

	EquipItem(best_slot);
	UpdateWeaponAnimation();
}

#define ID_NONEED  0   //id is not important
#define ID_NEED    1   //id is important
#define ID_NO      2   //shouldn't id

/* returns true if there are more item usages not fitting in given array */
bool Inventory::GetEquipmentInfo(ItemExtHeader *array, int startindex, int count)
{
	int pos = 0;
	int actual = 0;
	memset(array, 0, count * sizeof(ItemExtHeader) );
	for(unsigned int idx=0;idx<Slots.size();idx++) {
		if (!core->QuerySlotEffects(idx)) {
			continue;
		}
		CREItem *slot;

		const Item *itm = GetItemPointer(idx, slot);
		if (!itm) {
			continue;
		}
		for(int ehc=0;ehc<itm->ExtHeaderCount;ehc++) {
			ITMExtHeader *ext_header = itm->ext_headers+ehc;
			if (ext_header->Location!=ITEM_LOC_EQUIPMENT) {
				continue;
			}
			//skipping if we cannot use the item
			int idreq1 = (slot->Flags&IE_INV_ITEM_IDENTIFIED);
			int idreq2 = ext_header->IDReq;
			switch (idreq2) {
				case ID_NO:
					if (idreq1) continue;
					break;
				case ID_NEED:
					if (!idreq1) continue;
				default:;
			}

			actual++;
			if (actual>startindex) {

				//store the item, return if we can't store more
				if (!count) {
					gamedata->FreeItem(itm, slot->ItemResRef, false);
					return true;
				}
				count--;
				memcpy(array[pos].itemname, slot->ItemResRef, sizeof(ieResRef) );
				array[pos].slot = idx;
				array[pos].headerindex = ehc;
				int slen = ((char *) &(array[pos].itemname)) -((char *) &(array[pos].AttackType));
				memcpy(&(array[pos].AttackType), &(ext_header->AttackType), slen);
				if (ext_header->Charges) {
					//don't modify ehc, it is a counter
					if (ehc>=CHARGE_COUNTERS) {
						array[pos].Charges=slot->Usages[0];
					} else {
						array[pos].Charges=slot->Usages[ehc];
					}
				} else {
					array[pos].Charges=0xffff;
				}
				pos++;
			}
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

void Inventory::UpdateShieldAnimation(Item *it)
{
	char AnimationType[2]={0,0};
	int WeaponType = -1;

	if (it) {
		memcpy(AnimationType, it->AnimationType, 2);
		if (core->CanUseItemType(SLOT_WEAPON, it))
			WeaponType = IE_ANI_WEAPON_2W;
		else
			WeaponType = IE_ANI_WEAPON_1H;
	} else {
		WeaponType = IE_ANI_WEAPON_1H;
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
	int WeaponType = -1;

	char AnimationType[2]={0,0};
	ieWord MeleeAnimation[3]={100,0,0};
	CREItem *Slot;

	// TODO: fix bows?

	ITMExtHeader *header = 0;
	const Item *itm = GetItemPointer(slot, Slot);
	if (itm) {
		itm->GetDamagePotential(false, header);
		memcpy(AnimationType,itm->AnimationType,sizeof(AnimationType) );
		//for twohanded flag, you don't need itm
		if (Slot->Flags & IE_INV_ITEM_TWOHANDED)
			WeaponType = IE_ANI_WEAPON_2H;
		else {

			// Examine shield slot to check if we're using two weapons
			// TODO: for consistency, use same Item* access method as above
			bool twoweapon = false;
			int slot = GetShieldSlot();
			CREItem* si = NULL;
			if (slot>0) {
				si = GetSlotItem( (ieDword) slot );
			}
			if (si) {
				Item* it = gamedata->GetItem(si->ItemResRef);
				if (core->CanUseItemType(SLOT_WEAPON, it))
					twoweapon = true;
				gamedata->FreeItem(it, si->ItemResRef, false);
			}

			if (twoweapon)
				WeaponType = IE_ANI_WEAPON_2W;
			else
				WeaponType = IE_ANI_WEAPON_1H;
		}
	}

	if (header)
		memcpy(MeleeAnimation,header->MeleeAnimation, sizeof(MeleeAnimation) );
	if (itm)
		gamedata->FreeItem( itm, Slot->ItemResRef, false );
	Owner->SetUsedWeapon(AnimationType, MeleeAnimation, WeaponType);
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
	return HasItemInSlot("",otherslot);
}

inline bool Inventory::TwoHandedInSlot(int slot) const
{
	CREItem *item;

	item = GetSlotItem(slot);
	if (!item) return false;
	if (item->Flags&IE_INV_ITEM_TWOHANDED) {
		return true;
	}
	return false;
}

int Inventory::WhyCantEquip(int slot, int twohanded) const
{
	// check only for hand slots
	if ((slot<SLOT_MELEE || slot>LAST_MELEE) && (slot != SLOT_LEFT) ) {
		return 0;
	}

	//magic items have the highest priority
	if ( HasItemInSlot("", SLOT_MAGIC)) {
		//magic weapon is in use
		return STR_MAGICWEAPON;
	}

	//can't equip in shield slot if a weapon slot is twohanded
	for (int i=SLOT_MELEE; i<=LAST_MELEE;i++) {
		//see GetShieldSlot
		int otherslot;
		if (IWD2) {
			otherslot = i+1;
		} else {
			otherslot = SLOT_LEFT;
		}
		if (slot==otherslot) {
			if (TwoHandedInSlot(i)) {
				return STR_TWOHANDED_USED;
			}
		}
	}

	if (twohanded) {
		if (IWD2) {
			if (slot>=SLOT_MELEE&&slot<=LAST_MELEE && (slot-SLOT_MELEE)&1) {
				return STR_NOT_IN_OFFHAND;
			}
		} else {
			if (slot==SLOT_LEFT) {
				return STR_NOT_IN_OFFHAND;
			}
		}
		if (IsSlotBlocked(slot)) {
		//cannot equip two handed while shield slot is in use?
			return STR_OFFHAND_USED;
		}
	}
	return 0;
}

//recharge items on rest, if rest was partial, recharge only 'hours'
//if this latter functionality is unwanted, then simply don't recharge if
//hours != 0
void Inventory::ChargeAllItems(int hours)
{
	//this loop is going from start
	for (size_t i = 0; i < Slots.size(); i++) {
		CREItem *item = Slots[i];
		if (!item) {
			continue;
		}

		Item *itm = gamedata->GetItem( item->ItemResRef );
		if (!itm)
			continue;
		for(int h=0;h<CHARGE_COUNTERS;h++) {
			ITMExtHeader *header = itm->GetExtHeader(h);
			if (header && (header->RechargeFlags&IE_ITEM_RECHARGE)) {
				unsigned short add = header->Charges;
				if (hours && add>hours) add=hours;
				add+=item->Usages[h];
				if(add>header->Charges) add=header->Charges;
				item->Usages[h]=add;
			}
		}
		gamedata->FreeItem( itm, item->ItemResRef, false );
	}
}

#define ITM_STEALING (IE_INV_ITEM_UNSTEALABLE | IE_INV_ITEM_MOVABLE | IE_INV_ITEM_EQUIPPED) //0x442
unsigned int Inventory::FindStealableItem()
{
	unsigned int slot;
	int inc;

	slot = core->Roll(1, Slots.size(),-1);
	inc = slot&1?1:-1;

	print("Start Slot: %d, increment: %d", slot, inc);
	//as the unsigned value underflows, it will be greater than Slots.size()
	for(;slot<Slots.size(); slot+=inc) {
		CREItem *item = Slots[slot];
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
	return 0;
}

// extension to allow more or less than head gear to avert critical hits:
// If an item with bit 25 set is equipped in a non-helmet slot, aversion is enabled
// If an item with bit 25 set is equipped in a helmet slot, aversion is disabled
bool Inventory::ProvidesCriticalAversion()
{
	int maxSlot = (int) Slots.size();
	for (int i = 0; i < maxSlot; i++) {
		CREItem *item = Slots[i];
		if (!item || ((i>=SLOT_INV) && (i<=LAST_INV))) { // ignore items in the backpack
			continue;
		}
		// weapon, but not equipped
		if (!((i == SLOT_ARMOR) || (i == SLOT_HEAD)) && !(item->Flags & IE_INV_ITEM_EQUIPPED)) {
			continue;
		}

		Item *itm = gamedata->GetItem(item->ItemResRef);
		if (!itm) {
			continue;
		}
		//if the item is worn on head, toggle crits must be 0, otherwise it must be 1
		//this flag is only stored in the item header, so we need to make some efforts
		//to get to it (TODO convince ToBEx to move this bit into the accessible range?) - low 24 bits
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
		Changed = true;
		EquipItem(slot);
		if (item->Usages[0] == 0) {
			delete item;
			return ASI_SUCCESS;
		}
		return ASI_PARTIAL;
	}
	return ASI_FAILED;
}

}
