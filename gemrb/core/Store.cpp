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

STOItem::STOItem() {
	ItemResRef[0] = 0;
	PurchasedAmount = Flags = Weight = MaxStackAmount = AmountInStock = InfiniteSupply = 0;
	for (int i=0; i<CHARGE_COUNTERS; i++) Usages[i] = 0;
	triggers = NULL;
}

STOItem::STOItem(CREItem *item) {
	CopyResRef(ItemResRef, item->ItemResRef);
	PurchasedAmount = 0; // Expired in STOItem
	memcpy(Usages, item->Usages, sizeof(ieWord)*CHARGE_COUNTERS);
	Flags = item->Flags;
	Weight = item->Weight;
	MaxStackAmount = item->MaxStackAmount;
	AmountInStock = InfiniteSupply = 0;
	triggers = NULL;
}

STOItem::~STOItem(void)
{
	if (triggers) triggers->Release();
}

Store::Store(void)
{
	HasTriggers = false;
	purchased_categories = NULL;
	drinks = NULL;
	cures = NULL;
	version = 0;
	StoreOwnerID = 0;
	StoreName = 0;
	Type = Flags = Lore = IDPrice = AvailableRooms = Capacity = 0;
	SellMarkup = BuyMarkup = DepreciationRate = StealFailureChance = 0;
	PurchasedCategoriesOffset = DrinksOffset = CuresOffset = ItemsOffset = 0;
	PurchasedCategoriesCount = DrinksCount = CuresCount = ItemsCount = 0;
}

Store::~Store(void)
{
	unsigned int i;

	for (i = 0; i < items.size(); i++) {
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

	Condition *triggers = items[slot]->triggers;
	if (triggers) {
		Scriptable *shopper = game->GetSelectedPCSingle(false);
		return triggers->Evaluate(shopper) != 0;
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

bool Store::IsBag() const
{
	return (Type==STT_BG2CONT || Type==STT_IWD2CONT);
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

	// can't buy/sell if store doesn't allow it at all
	if (!(Flags&IE_STORE_SELL))
		ret &= ~IE_STORE_SELL;
	if (!(Flags&IE_STORE_BUY))
		ret &= ~IE_STORE_BUY;

	if (pc && (Type<STT_BG2CONT) ) {
		//don't allow selling of non destructible items
		if (!(invflags&IE_INV_ITEM_DESTRUCTIBLE )) {
			ret &= ~IE_STORE_SELL;
		}

		//don't allow selling of critical items (they could still be put in bags) ... unless the shop is special
		if ((invflags&IE_INV_ITEM_CRITICAL) && !(Flags&IE_STORE_BUYCRITS)) {
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
STOItem *Store::GetItem(unsigned int idx, bool usetrigger)
{
	if (!HasTriggers || !usetrigger) {
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
	//gemrb extension, some shops won't recharge items
	//containers' behaviour is inverted
	//bag      0   1   0   1
	//flag     0   0   1   1
	//recharge 1   0   0   1
	if (IsBag() != !(Flags&IE_STORE_RECHARGE)) {
		bool feature = core->HasFeature(GF_SHOP_RECHARGE);
		for (int i=0;i<CHARGE_COUNTERS;i++) {
			ITMExtHeader *h = itm->GetExtHeader(i);
			if (!h) {
				item->Usages[i] = 0;
				continue;
			}
			if ((feature || h->RechargeFlags&IE_ITEM_RECHARGE)
			    && item->Usages[i] < h->Charges)
				item->Usages[i] = h->Charges;
		}
	}
	gamedata->FreeItem(itm, item->ItemResRef, 0);
}

void Store::IdentifyItem(CREItem *item) const
{
	if ((item->Flags & IE_INV_ITEM_IDENTIFIED) || IsBag()) {
		return;
	}

	Item *itm = gamedata->GetItem(item->ItemResRef);
	if (!itm) {
		return;
	}

	if (itm->LoreToID <= Lore) {
		item->Flags |= IE_INV_ITEM_IDENTIFIED;
	}
	gamedata->FreeItem(itm, item->ItemResRef, 0);
}

void Store::AddItem(CREItem *item)
{
	IdentifyItem(item);
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

	temp = new STOItem(item);
	temp->AmountInStock = 1;
	if (temp->MaxStackAmount && temp->Usages[0] > 1) {
		// For now, we do what bg2 does and add new items in stacks of 1.
		temp->AmountInStock = item->Usages[0];
		temp->Usages[0] = 1;
	}
	items.push_back (temp );
	ItemsCount++;
}

void Store::RemoveItem( STOItem *itm )
{
	size_t i = items.size();
	while(i--) {
		if (items[i]==itm) {
			items.erase(items.begin()+i);
			ItemsCount--;
			break;
		}
	}
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
