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

#include "Game.h"
#include "GameData.h"
#include "Interface.h"
#include "Item.h"
#include "GameScript/GameScript.h"

namespace GemRB {

STOItem::STOItem(const CREItem *item) {
	CopyCREItem(item);
}

// beware of triggers leaking if you're writing to a non-blank STOItem
void STOItem::CopyCREItem(const CREItem *item) {
	ItemResRef = item->ItemResRef;
	PurchasedAmount = 0; // Expired in STOItem
	Usages[0] = item->Usages[0];
	Usages[1] = item->Usages[1];
	Usages[2] = item->Usages[2];
	Flags = item->Flags;
	Weight = item->Weight;
	MaxStackAmount = item->MaxStackAmount;
	AmountInStock = InfiniteSupply = 0;
	triggers = nullptr;
}

STOItem::~STOItem(void)
{
	if (triggers) triggers->Release();
}

Store::~Store(void)
{
	for (auto item : items) {
		delete item;
	}
	for (auto cure : cures) {
		delete cure;
	}
	for (auto drink : drinks) {
		delete drink;
	}
}

bool Store::IsItemAvailable(unsigned int slot) const
{
	const Game *game = core->GetGame();
	//0     - not infinite, not conditional
	//-1    - infinite
	//other - pst trigger ref

	const Condition *triggers = items[slot]->triggers;
	if (triggers) {
		Scriptable *shopper = game->GetSelectedPCSingle(false);
		return triggers->Evaluate(shopper) != 0;
	}
	return true;
}

int Store::GetRealStockSize() const
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
	return Type == StoreType::BG2CONT || Type == StoreType::IWD2CONT;
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

	if (pc && Type < StoreType::BG2CONT) {

		//don't allow selling of critical items (they could still be put in bags) ... unless the shop is special
		if ((invflags&IE_INV_ITEM_CRITICAL) && (!(Flags&IE_STORE_BUYCRITS))) {
			ret &= ~IE_STORE_SELL;
		}
		if ((invflags&IE_INV_ITEM_CRITICAL) && (core->HasFeature(GFFlags::SELLABLE_CRITS_NO_CONV))) {
			if (invflags&IE_INV_ITEM_CONVERSABLE) {
				ret &= ~IE_STORE_SELL;
			} else { 
				ret |= IE_STORE_SELL;
			}
		}
				//don't allow selling of non destructible items
		if (!(invflags&IE_INV_ITEM_DESTRUCTIBLE )) {
			ret &= ~IE_STORE_SELL;
		}
	
		//check if store buys stolen items
		if ((invflags&IE_INV_ITEM_STOLEN) && !(Flags&IE_STORE_FENCE)) {
			ret &= ~IE_STORE_SELL;
		}
	}

	if (!pc) {
		return ret;
	}

	for (const auto& category : purchased_categories) {
		if (type == category) {
			return ret;
		}
	}

	//Even if the store doesn't purchase the item, it can still ID it
	return ret & ~IE_STORE_SELL;
}

STOCure *Store::GetCure(unsigned int idx) const
{
	if (idx >= CuresCount) {
		return nullptr;
	}
	return cures[idx];
}

STODrink *Store::GetDrink(unsigned int idx) const
{
	if (idx >= DrinksCount) {
		return nullptr;
	}
	return drinks[idx];
}

//We need this weirdness for PST item lookup
STOItem *Store::GetItem(unsigned int idx, bool usetrigger) const
{
	if (!HasTriggers || !usetrigger) {
		if (idx >= items.size()) {
			return nullptr;
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

unsigned int Store::FindItem(const ResRef &itemname, bool usetrigger) const
{
	for (unsigned int i=0;i<ItemsCount;i++) {
		if (usetrigger) {
			if (!IsItemAvailable(i) ) {
				continue;
			}
		}
		const STOItem *temp = items[i];
		if (itemname == temp->ItemResRef) {
			return i;
		}
	}
	return (unsigned int) -1;
}

STOItem *Store::FindItem(const CREItem *item, bool exact) const
{
	for (unsigned int i=0;i<ItemsCount;i++) {
		if (!IsItemAvailable(i) ) {
			continue;
		}
		STOItem *temp = items[i];

		if (item->ItemResRef != temp->ItemResRef) {
			continue;
		}
		if (exact) {
			// Infinite supply means we don't have to worry about keeping track of amounts.
			if (temp->InfiniteSupply == -1) {
				return temp;
			}
			// Check if we have a non-stackable item with a different number of charges.
			if (!item->MaxStackAmount && memcmp(temp->Usages, item->Usages, sizeof(item->Usages)) != 0) {
				continue;
			}
		}
		return temp;
	}
	return nullptr;
}

unsigned int Store::CountItems(const ResRef& itemRef) const
{
	unsigned int count = 0;
	for (unsigned int i = 0; i < ItemsCount; i++) {
		const STOItem* storeItem = items[i];
		if (itemRef == storeItem->ItemResRef) {
			count += storeItem->AmountInStock;
		}
	}
	return count;
}

//some stores can recharge items - in original engine apparently all stores
//did this. In gemrb there is a flag.
void Store::RechargeItem(CREItem *item) const
{
	const Item *itm = gamedata->GetItem(item->ItemResRef);
	if (!itm) {
		return;
	}
	//gemrb extension, some shops won't recharge items
	//containers' behaviour is inverted
	//bag      0   1   0   1
	//flag     0   0   1   1
	//recharge 1   0   0   1
	if (IsBag() != !(Flags&IE_STORE_RECHARGE)) {
		bool feature = core->HasFeature(GFFlags::SHOP_RECHARGE);
		for (int i=0;i<CHARGE_COUNTERS;i++) {
			const ITMExtHeader *h = itm->GetExtHeader(i);
			if (!h) {
				item->Usages[i] = 0;
				continue;
			}
			if ((feature || h->RechargeFlags&IE_ITEM_RECHARGE)
			    && item->Usages[i] < h->Charges)
				item->Usages[i] = h->Charges;
		}
	}
	gamedata->FreeItem(itm, item->ItemResRef, false);
}

void Store::IdentifyItem(CREItem *item) const
{
	if ((item->Flags & IE_INV_ITEM_IDENTIFIED) || IsBag()) {
		return;
	}

	const Item *itm = gamedata->GetItem(item->ItemResRef);
	if (!itm) {
		return;
	}

	if (itm->LoreToID <= Lore) {
		item->Flags |= IE_INV_ITEM_IDENTIFIED;
	}
	gamedata->FreeItem(itm, item->ItemResRef, false);
}

void Store::AddItem(CREItem *item)
{
	IdentifyItem(item);
	RechargeItem(item);
	STOItem *temp = FindItem(item, true);

	if (temp) {
		if (temp->InfiniteSupply == -1) {
			return;
		}

		if (!item->MaxStackAmount) {
			// Not stacked, so just increase the amount.
			temp->AmountInStock++;
			return;
		}

		// Stacked, so increase usages.
		ieDword newAmount = 1;
		if (!temp->Usages[0]) temp->Usages[0] = 1;
		if (item->Usages[0] == temp->Usages[0] || item->Usages[0] == 0) {
			// same stack size
			temp->AmountInStock += newAmount;
			return;
		}

		// Stack sizes differ!
		// For now, we do what bg2 does and just round up.
		newAmount = item->Usages[0] / temp->Usages[0];
		if (item->Usages[0] % temp->Usages[0]) {
			newAmount++;
		}
		temp->AmountInStock += newAmount;
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

void Store::RemoveItem(const STOItem *itm)
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

void Store::RemoveItemByName(const ResRef& itemName, ieDword count)
{
	unsigned int idx = FindItem(itemName, false);
	if (idx == (unsigned int) -1) return;
	STOItem* si = GetItem(idx, false);
	if (count && si->AmountInStock > count) {
		si->AmountInStock -= count;
	} else {
		RemoveItem(si);
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
