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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

// This class represents the .itm (item) files of the engine
// Items are all the weapons, armor, carriable quest objects, etc.

#include "Item.h"

#include "win32def.h"

#include "Interface.h"
#include "Projectile.h"
#include "ProjectileServer.h"

namespace GemRB {

ITMExtHeader::ITMExtHeader(void)
{
	features = NULL;
}

ITMExtHeader::~ITMExtHeader(void)
{
	delete [] features;
}

Item::Item(void)
{
	ext_headers = NULL;
	equipping_features = NULL;
}

Item::~Item(void)
{
	//core->FreeITMExt( ext_headers, equipping_features );
	delete [] ext_headers;
	delete [] equipping_features;
}

//-1 will return equipping feature block
//otherwise returns the n'th feature block
EffectQueue *Item::GetEffectBlock(Scriptable *self, const Point &pos, int usage, ieDwordSigned invslot, ieDword pro) const
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

	//collecting all self affecting effects in a single queue, so the random value is rolled only once
	EffectQueue *fxqueue = new EffectQueue();
	EffectQueue *selfqueue = new EffectQueue();
	Actor *target = (self->Type==ST_ACTOR)?(Actor *) self:NULL;

	for (int i=0;i<count;i++) {
		Effect *fx = features+i;
		fx->InventorySlot = invslot;
		fx->CasterLevel = ITEM_CASTERLEVEL; //items all have casterlevel 10
		if (usage >= 0) {
			//this is not coming from the item header, but from the recharge flags
			fx->SourceFlags = ext_headers[usage].RechargeFlags;
		} else {
			fx->SourceFlags = 0;
		}
		if (fx->Target != FX_TARGET_SELF) {
			fx->Projectile = pro;
			fxqueue->AddEffect( fx );
		} else {
			//Actor *target = (self->Type==ST_ACTOR)?(Actor *) self:NULL;
			fx->Projectile = 0;
			fx->PosX=pos.x;
			fx->PosY=pos.y;
			if (target) {
				//core->ApplyEffect(fx, target, self);
				selfqueue->AddEffect(fx);
			}
		}
	}
	if (target && selfqueue->GetEffectsCount()) {
		core->ApplyEffectQueue(selfqueue, target, self);
	}

	//adding a pulse effect for weapons (PST)
	//if it is an equipping effect block
	if ((usage==-1) && (WieldColor!=0xffff)) {
		if (Flags&IE_ITEM_PULSATING) {
			Effect *tmp = BuildGlowEffect(WieldColor);
			if (tmp) {
				tmp->InventorySlot = invslot;
				tmp->Projectile=pro;
				fxqueue->AddEffect( tmp );
				delete tmp;
			}
		}
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
	return -1;
}

int Item::GetWeaponHeaderNumber(bool ranged) const
{
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
		return ehc;
	}
	return 0xffff; //invalid extheader number
}

int Item::GetEquipmentHeaderNumber(int cnt) const
{
	for(int ehc=0; ehc<ExtHeaderCount; ehc++) {
		ITMExtHeader *ext_header = GetExtHeader(ehc);
		if (ext_header->Location!=ITEM_LOC_EQUIPMENT) {
			continue;
		}
		if (ext_header->AttackType!=ITEM_AT_MAGIC) {
			continue;
		}

		if (cnt) {
			cnt--;
			continue;
		}
		return ehc;
	}
	return 0xffff; //invalid extheader number
}

ITMExtHeader *Item::GetWeaponHeader(bool ranged) const
{
	//start from the beginning
	return GetExtHeader(GetWeaponHeaderNumber(ranged)) ;
}

int Item::UseCharge(ieWord *Charges, int header, bool expend) const
{
	ITMExtHeader *ieh = GetExtHeader(header);
	if (!ieh) return 0;
	int type = ieh->ChargeDepletion;

	int ccount = 0;
	if ((header>=CHARGE_COUNTERS) || (header<0) || MaxStackAmount) {
		header = 0;
	}
	ccount=Charges[header];

	//if the item started from 0 charges, then it isn't depleting
	if (ieh->Charges==0) {
		return CHG_NONE;
	}
	if (expend) {
		Charges[header] = --ccount;
	}

	if (ccount>0) {
		return CHG_NONE;
	}
	if (type == CHG_NONE) {
		Charges[header]=0;
	}
	return type;
}

//returns a projectile loaded with the effect queue
Projectile *Item::GetProjectile(Scriptable *self, int header, const Point &target, ieDwordSigned invslot, int miss) const
{
	ITMExtHeader *eh = GetExtHeader(header);
	if (!eh) {
		return NULL;
	}
	ieDword idx = eh->ProjectileAnimation;
	Projectile *pro = core->GetProjectileServer()->GetProjectileByIndex(idx);
	int usage ;
	if (header>= 0)
		usage = header;
	else
		usage = GetWeaponHeaderNumber(header==-2);
	if (!miss) {
		EffectQueue *fx = GetEffectBlock(self, target, usage, invslot, idx);
		pro->SetEffects(fx);
	}
	return pro;
}

//this is the implementation of the weapon glow effect in PST
static EffectRef glow_ref = { "Color:PulseRGB", -1 };
//this type of colour uses PAL32, a PST specific palette
#define PALSIZE 32
static Color ActorColor[PALSIZE];

Effect *Item::BuildGlowEffect(int gradient) const
{
	//palette entry to to RGB conversion
	core->GetPalette( gradient, PALSIZE, ActorColor );
	ieDword rgb = (ActorColor[16].r<<16) | (ActorColor[16].g<<8) | ActorColor[16].b;
	ieDword location = 0;
	ieDword speed = 128;
	Effect *fx = EffectQueue::CreateEffect(glow_ref, rgb, location|(speed<<16), FX_DURATION_INSTANT_WHILE_EQUIPPED);
	return fx;
}

unsigned int Item::GetCastingDistance(int idx) const
{
	ITMExtHeader *seh = GetExtHeader(idx);
	if (!seh) {
		Log(ERROR, "Item", "Cannot retrieve item header!!! required header: %d, maximum: %d",
			idx, (int) ExtHeaderCount);
		return 0;
	}
	return (unsigned int) seh->Range;
}

}
