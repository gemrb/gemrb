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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Inventory.cpp,v 1.1 2004/03/29 23:52:29 edheldil Exp $
 *
 */

#include <stdio.h>
#include "../../includes/win32def.h"
// #include "Interface.h"
#include "Inventory.h"




Inventory::Inventory()
{
	//	memset( slots, 0, sizeof( slots ) );
	for (int i = 0; i < INVENTORY_SIZE; i++) {
		slots.push_back( NULL );
	}
}

Inventory::~Inventory()
{
	for (size_t i = 0; i < slots.size(); i++) {
		delete( slots[i] );
	}
}

void Inventory::SetSlotItem(CREItem* item, int slot)
{
	int d = slot - slots.size() + 1;
	for ( ; d > 0 ; d--) 
		slots.push_back( NULL );
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

	if (!strcasecmp( target->ItemResRef, source->ItemResRef )) {
		// FIXME: some magic code belongs here ....

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

		if (!itm) continue;

		printf ( "%2d: %8s   %d (%d %d %d) %x\n", i, itm->ItemResRef, itm->Unknown08, itm->Usages[0], itm->Usages[1], itm->Usages[2], itm->Flags );
	}
}
