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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Inventory.cpp,v 1.5 2004/04/07 20:12:31 avenger_teambg Exp $
 *
 */

#include <stdio.h>
#include "../../includes/win32def.h"
#include "Inventory.h"


Inventory::Inventory()
{
	for (int i = 0; i < INVENTORY_SIZE; i++) {
		slots.push_back( NULL );
	}
}

Inventory::~Inventory()
{
	for (size_t i = 0; i < slots.size(); i++) {
		if(slots[i]) {
			delete( slots[i] );
		}
	}
}

/** if you supply a null string, then it checks if the slot is empty */
bool Inventory::HasItemInSlot(const char *resref, int slot)
{
	CREItem *item = slots[slot];
	if(!item) {
		return false;
	}
	if (strnicmp( item->ItemResRef, resref, 8 )==0) {
		return true;
	}
	return false;
}

int Inventory::CountItems(const char *resref, bool stacks)
{
	int count = 0;
	int slot = slots.size();
	while(slot--) {
		CREItem *item = slots[slot];
		if(!item) {
			continue;
		}
		if (strnicmp( item->ItemResRef, resref,8 ) == 0) {
			if (stacks && item->Flags&IE_ITEM_STACKED) {
				count+=item->Usages[0];
			}
			else {
				count++;
			}
		}
	}
	return count;
}

bool Inventory::HasItem(const char *resref, int flags)
{
	int slot = slots.size();
	while(slot--) {
		CREItem *item = slots[slot];
		if(!item) {
			continue;
		}
		if (strnicmp(item->ItemResRef, resref,8) == 0) {
			if(flags&2) { //identified?
				if(item->Flags&IE_ITEM_IDENTIFIED) {
					return true;
				}
				continue; //not identified
			}
			return true;
		}
	}
	return false;
}

/** if resref is NULL, then destroy ALL items */
/** if flags == 1 then destroy only destructable items */
void Inventory::DestroyItem(const char *resref, int flags)
{
	int slot = slots.size();
	while(slot--) {
		CREItem *item = slots[slot];
		if(!item) {
			continue;
		}
		if(flags&1) {
			//IsDestructible(item->ItemResRef);
		}
		if(resref && strnicmp(item->ItemResRef, resref, 8) ) {
			continue;
		}
		//we need to acknowledge that the item was destroyed
		//use unequip stuff, decrease encumbrance etc,
		//until that, we simply erase it
		delete item;
		slots[slot] = NULL;
	}
}

void Inventory::SetSlotItem(CREItem* item, int slot)
{
	int d = slot - slots.size() + 1;
	for ( ; d > 0 ; d--) 
		slots.push_back( NULL );
	if(slots[slot]) {
		delete slots[slot];
	}
	slots[slot] = item;
}

int Inventory::AddSlotItem(CREItem* item, int slot, CREItem **res_item)
{
	// FIXME: join items if possible
	for (size_t i = 0; i < slots.size(); i++) {
		if (!slots[i]) {
			slots[i] = item;
			*res_item = NULL;
			return ( int ) i;
		}
		if (ItemsAreCompatible( slots[i], item )) {
		}
	}

	*res_item = item;
	return -1;
}

bool Inventory::ItemsAreCompatible(CREItem* target, CREItem* source)
{
	if (!target) {
		return true;
	}

	if (!strnicmp( target->ItemResRef, source->ItemResRef,8 )) {
		// FIXME: some magic code belongs here ....
		// this should check if the items are stacking or not
		return true;
	}
	return false;
}

#if 0
CREItem* Inventory::GetSlotItem(int index)
{
	if (index >= slots.size()) {
		return NULL;
	}
	return slots[index];
}

// Returns index of first empty slot or slot with the same
//    item and not full stack. On fail returns -1
int Inventory::FindCandidateSlot(CREItem* item, int first_slot)
{
	if (first_slot >= slots.size())
		return -1;

	for (size_t i = first_slot; i < slots.size(); i++) {
	}

	return -1;
}

bool Inventory::CanTakeItem(CREItem* item)
{
}

int Inventory::AddSlotItem(CREItem* item)
{
	// FIXME: join items if possible
	for (size_t i = 0; i < slots.size(); i++) {
		if (!slots[i]) {
			slots[i] = item;
			return ( int ) i;
		}
	}
	slots.push_back( item );
	return ( int ) slots.size() - 1;
}

int Inventory::DelSlotItem(unsigned int index, bool autoFree)
{
	if (index >= slots.size()) {
		return -1;
	}
	if (!slots[index]) {
		return -1;
	}
	if (autoFree) {
		delete( slots[index] );
	}
	slots[index] = NULL;
	return 0;
}

#endif

void Inventory::dump()
{
	printf( "INVENTORY:\n" );
	for (size_t i = 0; i < slots.size(); i++) {
		CREItem* itm = slots[i];

		if (!itm) {
			continue;
		}

		printf ( "%2d: %8s   %d (%d %d %d) %lx\n", i, itm->ItemResRef, itm->Unknown08, itm->Usages[0], itm->Usages[1], itm->Usages[2], itm->Flags );
	}
}
