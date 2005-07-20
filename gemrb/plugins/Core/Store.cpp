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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Store.cpp,v 1.14 2005/07/20 21:46:30 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "Store.h"
#include "Interface.h"

Store::Store(void)
{
	HasTriggers=false;
	purchased_categories=NULL;
	drinks=NULL;
	cures=NULL;
	version=0;
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

	int trigger = (int) items[slot]->InfiniteSupply;
	if (trigger>0) {
		char *TriggerCode = core->GetString( trigger );
		return GameScript::EvaluateString (game->GetPC(game->GetSelectedPCSingle(), false),TriggerCode)!=0;
	}
	return false;
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

	if (invflags&IE_INV_ITEM_UNDROPPABLE) {
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
		if (!(invflags&IE_INV_ITEM_DESTRUCTIBLE)) {
			ret &= ~IE_STORE_SELL;
		}
		//check if store buys stolen items
		if ((invflags&IE_INV_ITEM_STOLEN) && !(Type&IE_STORE_FENCE) ) {
			ret &= ~IE_STORE_SELL;
		}
	}

	if (!pc) {
		return ret;
	}

	for (ieDword i=0;i<PurchasedCategoriesCount;i++) {
		printf ("%d == %d\n", type, purchased_categories[i]);
		if (type==purchased_categories[i]) {
			return ret;
		}
	}
	return 0;
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

unsigned int Store::FindItem(ieResRef itemname, bool usetrigger) const
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
		if(exact) {
			if (temp->InfiniteSupply==(ieDword) -1) {
				return temp;
			}
			//check if we could simply merge the item into the stock or need a new entry
			if ((temp->StackAmount>=99) || memcmp(temp->Usages, item->Usages, sizeof(item->Usages))) {
				continue;
			}
		}
		return temp;
	}
	return NULL;
}

void Store::AddItem(CREItem *item)
{
	STOItem *temp = FindItem(item, true);

	if (temp) {
		if (temp->InfiniteSupply!=(ieDword) -1) {
			temp->StackAmount++;
		}
		return;
	}

	temp = new STOItem();
	//wonder if this is needed
	//memset( temp, 0, sizeof (STOItem ) );
	memcpy( temp, item, sizeof( CREItem ) );
	items.push_back (temp );
	ItemsCount++;
}

void Store::RemoveItem( unsigned int idx )
{
	if (items.size()!=ItemsCount) {
		printMessage("Store","Inconsistent store", LIGHT_RED);
		abort();
	}
	if (ItemsCount<=idx) {
		return;
	}
	items.erase(items.begin()+idx);
	ItemsCount--;
}
