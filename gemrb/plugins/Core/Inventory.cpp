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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Inventory.cpp,v 1.13 2004/04/17 22:23:00 avenger_teambg Exp $
 *
 */

#include <stdio.h>
#include "../../includes/win32def.h"
#include "Inventory.h"

static int Inited=-1;

Inventory::Inventory()
{
	InventoryType = INVENTORY_HEAP;
	Changed = false;
	Weight = 0;
}

Inventory::~Inventory()
{
	for (size_t i = 0; i < Slots.size(); i++) {
		if(Slots[i]) {
			delete( Slots[i] );
			Slots[i] = NULL;
		}
	}
}

int Inventory::CalculateWeight()
{
	if(!Changed) {
		return Weight;
	}
	
//returns the weight of stored items
	return Weight;
}

void Inventory::SetInventoryType(int arg)
{
	InventoryType = arg;
}

void Inventory::SetSlotCount(unsigned int size)
{
	if(Slots.size()) {
		printf("Inventory size changed???\n");
		//we don't allow reassignment,
		//if you want this, delete the previous Slots here
		abort(); 
	}
	Slots.assign((size_t) size, NULL);
}

/** if you supply a null string, then it checks if the slot is empty */
bool Inventory::HasItemInSlot(const char *resref, int slot)
{
	CREItem *item = Slots[slot];
	if(!item) {
		return false;
	}
	if (strnicmp( item->ItemResRef, resref, 8 )==0) {
		return true;
	}
	return false;
}

/** counts the items in the inventory, if stacks == 1 then stacks are 
    accounted for their heap size */
int Inventory::CountItems(const char *resref, bool stacks)
{
	int count = 0;
	int slot = Slots.size();
	while(slot--) {
		CREItem *item = Slots[slot];
		if(!item) {
			continue;
		}
		if (stacks && (item->Flags&IE_ITEM_STACKED) ) {
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
bool Inventory::HasItem(const char *resref, ieDword flags)
{
	int slot = Slots.size();
	while(slot--) {
		CREItem *item = Slots[slot];
		if(!item) {
			continue;
		}
		if( (flags&item->Flags)!=flags) {
				continue;
		}
		if (strnicmp(item->ItemResRef, resref,8) ) {
			continue;
		}
		return true;
	}
	return false;
}

/** if resref is NULL, then destroy ALL items
    this function can look for stolen, equipped, identified, destructible
    etc, items. You just have to specify the flags in the bitmask
    specifying 1 in a bit signifies a requirement */
void Inventory::DestroyItem(const char *resref, ieDword flags)
{
	int slot = Slots.size();
	while(slot--) {
		CREItem *item = Slots[slot];
		if(!item) {
			continue;
		}
		// here you can simply destroy all items of a specific
                // type
		if( (flags&item->Flags)!=flags) {
				continue;
		}
		if(resref && strnicmp(item->ItemResRef, resref, 8) ) {
			continue;
		}
		//we need to acknowledge that the item was destroyed
		//use unequip stuff, decrease encumbrance etc,
		//until that, we simply erase it
		delete item;
		Slots[slot] = NULL;
		Changed = true;
	}
}

CREItem *Inventory::GetItem(unsigned int slot, unsigned int count)
{
	CREItem *item;

	if (slot>=Slots.size() ) {
		printf("Invalid slot!\n");
		abort();
	}
	Changed = true;
	item = Slots[slot];
	if(!count || !(item->Flags&IE_ITEM_STACKED) ) {
		Slots[slot] = NULL;
		return item;
	}
	if(count >= item->Usages[0]) {
		Slots[slot] = NULL;
		return item;
	}

	CREItem *returned = new CREItem(*item);
	item->Usages[0]-=count;
	returned->Usages[0]=count;
	return returned;
}

void Inventory::SetSlotItem(CREItem* item, unsigned int slot)
{
	if (slot>=Slots.size() ) {
		printf("Invalid slot!\n");
		abort();
	}
	Changed = true;
	if(Slots[slot]) {
		delete Slots[slot];
	}
	Slots[slot] = item;
}

int Inventory::AddSlotItem(CREItem* item, unsigned int slot, CREItem **res_item)
{
	// FIXME: join items if possible
	Changed = true;
	for (size_t i = 0; i < Slots.size(); i++) {
		if (!Slots[i]) {
			Slots[i] = item;
			*res_item = NULL;
			return ( int ) i;
		}

		if (ItemsAreCompatible( Slots[i], item )) {
 			//calculate with the max movable stock
			int chunk=item->Usages[0];
			if(!chunk) {
				continue;
			}
			Slots[i]->Usages[0]+=chunk;
			item->Usages[0]-=chunk;
			if(item->Usages[0]==0) {
				*res_item = NULL;
				return ( int ) i;
			}
		}
	}

	*res_item = item;
	return -1;
}

int Inventory::AddSlotItem(STOItem* item, unsigned int slot, CREItem **res_item, int count)
{
	CREItem *temp;
	CREItem *remains;
	int ret = -1;

	for(int i=0;i<count;i++) {
	//the first part of a STOItem is essentially a CREItem
		temp = new CREItem;
		memcpy(&temp, item, sizeof(CREItem) ); 
		if(!item->InfiniteSupply) {
			if(!item->AmountInStock) {
				break;
			}
			item->AmountInStock--;
		}
		ret=AddSlotItem(temp, 0, &remains);
		//FIXME: drop remains at feet of actor
		if(remains) {
			delete remains;
		}
	}
	return ret;
}

bool Inventory::ItemsAreCompatible(CREItem* target, CREItem* source)
{
	if (!target) {
		return true;
	}

	if(!(source->Flags&IE_ITEM_STACKED) ) {
		return false;
	}

	if (!strnicmp( target->ItemResRef, source->ItemResRef,8 )) {
		if(target->Flags&IE_ITEM_STACKED) {
			return true;
		}
	}
	return false;
}

#if 0
CREItem* Inventory::GetSlotItem(int index)
{
	if (index >= Slots.size()) {
		return NULL;
	}
	return Slots[index];
}

// Returns index of first empty slot or slot with the same
//    item and not full stack. On fail returns -1
int Inventory::FindCandidateSlot(CREItem* item, int first_slot)
{
	if (first_slot >= Slots.size())
		return -1;

	for (size_t i = first_slot; i < Slots.size(); i++) {
	}

	return -1;
}

bool Inventory::CanTakeItem(CREItem* item)
{
}

int Inventory::AddSlotItem(CREItem* item)
{
	// FIXME: join items if possible
	for (size_t i = 0; i < Slots.size(); i++) {
		if (!Slots[i]) {
			Slots[i] = item;
			return ( int ) i;
		}
	}
	Slots.push_back( item );
	return ( int ) Slots.size() - 1;
}

int Inventory::DelSlotItem(unsigned int index, bool autoFree)
{
	if (index >= Slots.size()) {
		return -1;
	}
	if (!Slots[index]) {
		return -1;
	}
	if (autoFree) {
		delete( Slots[index] );
	}
	Slots[index] = NULL;
	return 0;
}

#endif

void Inventory::dump()
{
	printf( "INVENTORY:\n" );
	for (size_t i = 0; i < Slots.size(); i++) {
		CREItem* itm = Slots[i];

		if (!itm) {
			continue;
		}

		printf ( "%2d: %8s   %d (%d %d %d) %lx\n", i, itm->ItemResRef, itm->Unknown08, itm->Usages[0], itm->Usages[1], itm->Usages[2], itm->Flags );
	}
}
