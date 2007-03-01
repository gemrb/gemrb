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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Id$
 *
 */

#include "../../includes/win32def.h"
#include "Item.h"
#include "Interface.h"

ITMExtHeader::ITMExtHeader(void)
{
}

ITMExtHeader::~ITMExtHeader(void)
{
	delete [] features;
}

Item::Item(void)
{
}

Item::~Item(void)
{
	core->FreeITMExt( ext_headers, equipping_features );
}

//-1 will return equipping feature block
//otherwise returns the n'th feature block
EffectQueue *Item::GetEffectBlock(int usage) const
{
	Effect *features;
	int count;

	if (usage>=ExtHeaderCount) {
		return NULL;
	}
	if (usage>=0) {
		features = ext_headers[usage].features;
		count = ext_headers[usage].FeatureCount;
	} else {
		features = equipping_features;
		count = EquippingFeatureCount;
	}
	EffectQueue *fxqueue = new EffectQueue();
	
	for (int i=0;i<count;i++) {
		fxqueue->AddEffect( features+i );
	}
	return fxqueue;
}

/** returns the average damage this weapon would cause */
int Item::GetDamagePotential(bool ranged, ITMExtHeader *&header) const
{
	header = GetWeaponHeader(ranged);
	if (header) {
		return header->DiceThrown*(header->DiceSides+1)/2+header->DamageBonus;
	}
	return 0;
}

ITMExtHeader *Item::GetWeaponHeader(bool ranged) const
{
	//start from the beginning
	for(int ehc=0; ehc<ExtHeaderCount; ehc++) {
		ITMExtHeader *ext_header = GetExtHeader(ehc);
		if (ext_header->Location!=ITEM_LOC_WEAPON) {
			continue;
		}
		unsigned char AType = ext_header->AttackType;
		if (ranged) {
			if ((AType!=ITEM_AT_PROJECTILE) && (AType!=ITEM_AT_BOW) ) {
				continue;
			}
		} else {
			if (AType!=ITEM_AT_MELEE) {
				continue;
			}
		}
		return ext_header;
	}
	return NULL;
}
