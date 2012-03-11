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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

//This class represents .sto (store) files of the game.
//Inns, pubs, temples, backpacks are also implemented by stores.

#include "Store.h"

#include "win32def.h"

#include "Game.h"
#include "GameData.h"
#include "Interface.h"
#include "Item.h"
#include "GameScript/GameScript.h"

namespace GemRB {

Store::Store(void)
{
	HasTriggers = false;
	purchased_categories = NULL;
	drinks = NULL;
	cures = NULL;
	version = 0;
	StoreOwnerID = 0;
}

Store::~Store(void)
{
	unsigned int i;

	for (i = 0; i < items.size(); i++) {
		if (items[i]->trigger)
			items[i]->trigger->Release();
		delete( items[i] );
	}
	if(drinks)
		free(drinks);
	if(cures)
		free(cures);
	if (purchased_categories)
		free( purchased_categories );
}

bool Store::IsItemAvailable(unsigned int slot) const
{
	Game * game = core->GetGame();
	//0     - not infinite, not conditional
	//-1    - infinite
	//other - pst trigger ref

	Trigger *trigger = items[slot]->trigger;
	if (trigger) {
		return trigger->Evaluate(game->GetPC(game->GetSelectedPCSingle(), false))!=0;
	}
	return true;
}

int Store::GetRealStockSize()
{
	int count=ItemsCount;
	if (!HasTriggers) {
		return count;
	}
	for (unsigned int i=0;i<ItemsCount;i++) {
		if (!IsItemAvailable(i) ) {
			count--;
		}
	}
	return count;
}

int Store::AcceptableItemType(ieDword type, ieDword invflags, bool pc) const
{
	int ret;

	//don't allow any movement of undroppable items
	if (invflags&IE_INV_ITEM_UNDROPPABLE ) {
		ret = 0;
	} else {
		ret = IE_STORE_BUY|IE_STORE_SELL|IE_STORE_STEAL;
	}
	if (invflags&IE_INV_ITEM_UNSTEALABLE) {
		ret &= ~IE_STORE_STEAL;
	}
	if (!(invflags&IE_INV_ITEM_IDENTIFIED) ) {
		ret |= IE_STORE_ID;
	}
	if (pc && (Type<STT_BG2CONT) ) {
		//can't sell critical items
		if (!(invflags&IE_INV_ITEM_DESTRUCTIBLE )) {
			ret &= ~IE_STORE_SELL;
		}
		//don't allow selling of non destructible items
		//don't allow selling of critical items (they could still be put in bags)
		if ((invflags&(IE_INV_ITEM_DESTRUCTIBLE|IE_INV_ITEM_CRITICAL))!=IE_INV_ITEM_DESTRUCTIBLE) {
			ret &= ~IE_STORE_SELL;
		}

		//check if store buys stolen items
		if ((invflags&IE_INV_ITEM_STOLEN) && !(Flags&IE_STORE_FENCE) ) {
			ret &= ~IE_STORE_SELL;
		}
	}

	if (!pc) {
		return ret;
	}

	for (ieDword i=0;i<PurchasedCategoriesCount;i++) {
		if (type==purchased_categories[i]) {
			return ret;
		}
	}

	//Even if the store doesn't purchase the item, it can still ID it
	return ret & ~IE_STORE_SELL;
}

STOCure *Store::GetCure(unsigned int idx) const
{
	if (idx>=CuresCount) {
		return NULL;
	}
	return cures+idx;
}

STODrink *Store::GetDrink(unsigned int idx) const
{
	if (idx>=DrinksCount) {
		return NULL;
	}
	return drinks+idx;
}

//We need this weirdness for PST item lookup
STOItem *Store::GetItem(unsigned int idx)
{
	if (!HasTriggers) {
		if (idx>=items.size()) {
			return NULL;
		}
		return items[idx];
	}

	for (unsigned int i=0;i<ItemsCount;i++) {
		if (IsItemAvailable(i)) {
			if (!idx) {
				return items[i];
			}
			idx--;
		}
	}
	return NULL;
}

unsigned int Store::FindItem(const ieResRef itemname, bool usetrigger) const
{
	for (unsigned int i=0;i<ItemsCount;i++) {
		if (usetrigger) {
			if (!IsItemAvailable(i) ) {
				continue;
			}
		}
		STOItem *temp = items[i];
		if (!strnicmp(itemname, temp->ItemResRef, 8) ) {
			return i;
		}
	}
	return (unsigned int) -1;
}

STOItem *Store::FindItem(CREItem *item, bool exact)
{
	for (unsigned int i=0;i<ItemsCount;i++) {
		if (!IsItemAvailable(i) ) {
			continue;
		}
		STOItem *temp = items[i];

		if (strnicmp(item->ItemResRef, temp->ItemResRef, 8) ) {
			continue;
		}
		if (exact) {
			// Infinite supply means we don't have to worry about keeping track of amounts.
			if (temp->InfiniteSupply==-1) {
				return temp;
			}
			// Check if we have a non-stackable item with a different number of charges.
			if (!item->MaxStackAmount && memcmp(temp->Usages, item->Usages, sizeof(item->Usages))) {
				continue;
			}
		}
		return temp;
	}
	return NULL;
}

//some stores can recharge items - in original engine apparently all stores
//did this. In gemrb there is a flag.
void Store::RechargeItem(CREItem *item)
{
	Item *itm = gamedata->GetItem(item->ItemResRef);
	if (!itm) {
		return;
	}
	if (!itm->LoreToID) {
		item->Flags |= IE_INV_ITEM_IDENTIFIED;
	}
	//gemrb extension, some shops won't recharge items
	//containers' behaviour is inverted
	//bag      0   1   0   1
	//flag     0   0   1   1
	//recharge 1   0   0   1
	bool bag = (Type==STT_BG2CONT || Type==STT_IWD2CONT);
	if (bag != !(Flags&IE_STORE_RECHARGE)) {
		for (int i=0;i<CHARGE_COUNTERS;i++) {
			ITMExtHeader *h = itm->GetExtHeader(i);
			if (!h) {
				item->Usages[i]=0;
				continue;
			}
			if (h->RechargeFlags&IE_ITEM_RECHARGE) {
				item->Usages[i] = h->Charges;
			}
		}
	}
	gamedata->FreeItem(itm, item->ItemResRef, 0);
}

void Store::AddItem(CREItem *item)
{
	RechargeItem(item);
	STOItem *temp = FindItem(item, true);

	if (temp) {
		if (temp->InfiniteSupply!=-1) {
			if (item->MaxStackAmount) {
				// Stacked, so increase usages.
				ieDword newAmount = 1;
				if (!temp->Usages[0]) temp->Usages[0] = 1;
				if (item->Usages[0] != temp->Usages[0] && item->Usages[0] > 0) {
					// Stack sizes differ!
					// For now, we do what bg2 does and just round up.
					newAmount = item->Usages[0] / temp->Usages[0];
					if (item->Usages[0] % temp->Usages[0])
						newAmount++;
				}
				temp->AmountInStock += newAmount;
			} else {
				// Not stacked, so just increase the amount.
				temp->AmountInStock++;
			}
		}
		return;
	}

	temp = new STOItem();
	//It is important to initialize these fields, if STOItem ever changes to
	//a real class from struct, make sure the fields are cleared
	memset( temp, 0, sizeof (STOItem ) );
	memcpy( temp, item, sizeof( CREItem ) );
	temp->AmountInStock = 1;
	if (temp->MaxStackAmount && temp->Usages[0] > 1) {
		// For now, we do what bg2 does and add new items in stacks of 1.
		temp->AmountInStock = item->Usages[0];
		temp->Usages[0] = 1;
	}
	items.push_back (temp );
	ItemsCount++;
}

void Store::RemoveItem( unsigned int idx )
{
	if (items.size()!=ItemsCount) {
		error("Store", "Inconsistent store");
	}
	if (ItemsCount<=idx) {
		return;
	}
	items.erase(items.begin()+idx);
	ItemsCount--;
}

ieDword Store::GetOwnerID() const
{
	return StoreOwnerID;
}

void Store::SetOwnerID(ieDword owner)
{
	StoreOwnerID = owner;
}

}
