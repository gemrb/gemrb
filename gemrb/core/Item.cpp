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

#include "voodooconst.h"

#include "Interface.h"
#include "Projectile.h"
#include "ProjectileServer.h"
#include "Scriptable/Actor.h"

namespace GemRB {

ITMExtHeader::~ITMExtHeader()
{
	for (const auto& feature : features) {
		delete feature;
	}
}

Item::~Item()
{
	for (const auto& feature : equipping_features) {
		delete feature;
	}
}

ieStrRef Item::GetItemName(bool identified) const
{
	if (identified) {
		if (static_cast<int>(ItemNameIdentified) >= 0) return ItemNameIdentified;
		return ItemName;
	}
	if (static_cast<int>(ItemName) >= 0) {
		return ItemName;
	}
	return ItemNameIdentified;
}

ieStrRef Item::GetItemDesc(bool identified) const
{
	if (identified) {
		if (static_cast<int>(ItemDescIdentified) >= 0) return ItemDescIdentified;
		return ItemDesc;
	}
	if (static_cast<int>(ItemDesc) >= 0) {
		return ItemDesc;
	}
	return ItemDescIdentified;
}

//-1 will return equipping feature block
//otherwise returns the n'th feature block
EffectQueue Item::GetEffectBlock(Scriptable *self, const Point &pos, int usage, ieDwordSigned invslot, ieDword pro) const
{
	Effect *const *features = nullptr;
	size_t count;

	if (usage >= int(ext_headers.size())) {
		return {};
	}
	if (usage>=0) {
		features = ext_headers[usage].features.data();
		count = ext_headers[usage].features.size();
	} else {
		features = equipping_features.data();
		count = EquippingFeatureCount;
	}

	//collecting all self affecting effects in a single queue, so the random value is rolled only once
	EffectQueue fxqueue;
	EffectQueue selfqueue;
	Actor* target = Scriptable::As<Actor>(self);

	for (size_t i = 0; i < count; ++i) {
		Effect *fx = features[i];
		fx->InventorySlot = invslot;
		fx->CasterLevel = ITEM_CASTERLEVEL; //items all have casterlevel 10
		fx->CasterID = self->GetGlobalID();
		if (usage >= 0) {
			//this is not coming from the item header, but from the recharge flags
			fx->SourceFlags = ext_headers[usage].RechargeFlags;
		} else {
			fx->SourceFlags = 0;
		}

		if (fx->Target != FX_TARGET_PRESET && EffectQueue::OverrideTarget(fx)) {
			fx->Target = FX_TARGET_PRESET;
		}

		if (fx->Target != FX_TARGET_SELF) {
			fx->Projectile = pro;
			fxqueue.AddEffect(new Effect(*fx));
		} else {
			fx->Projectile = 0;
			fx->Pos = pos;
			if (target) {
				//core->ApplyEffect(fx, target, self);
				selfqueue.AddEffect(new Effect(*fx));
			}
		}
	}
	if (target && selfqueue.GetEffectsCount()) {
		core->ApplyEffectQueue(&selfqueue, target, self);
	}

	//adding a pulse effect for weapons (PST)
	//if it is an equipping effect block
	if (usage == -1 && WieldColor != 0xffff && Flags & IE_ITEM_PULSATING) {
		Effect *tmp = BuildGlowEffect(WieldColor);
		if (tmp) {
			tmp->InventorySlot = invslot;
			tmp->Projectile=pro;
			fxqueue.AddEffect(tmp);
		}
	}
	return fxqueue;
}

/** returns the average damage this weapon would cause */
// there might not be any target, so we can't consider also AltDiceThrown ...
int Item::GetDamagePotential(bool ranged, const ITMExtHeader *&header) const
{
	header = GetWeaponHeader(ranged);
	if (header) {
		return header->DiceThrown*(header->DiceSides+1)/2+header->DamageBonus;
	}
	return -1;
}

int Item::GetWeaponHeaderNumber(bool ranged) const
{
	for (size_t ehc = 0; ehc < ext_headers.size(); ehc++) {
		const ITMExtHeader *ext_header = &ext_headers[ehc];
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
		return int(ehc);
	}
	return 0xffff; //invalid extheader number
}

int Item::GetEquipmentHeaderNumber(int cnt) const
{
	for (size_t ehc = 0; ehc < ext_headers.size(); ehc++) {
		const ITMExtHeader *ext_header = &ext_headers[ehc];
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
		return int(ehc);
	}
	return 0xffff; //invalid extheader number
}

const ITMExtHeader *Item::GetWeaponHeader(bool ranged) const
{
	//start from the beginning
	return GetExtHeader(GetWeaponHeaderNumber(ranged)) ;
}

// returns the requested extended header
// -1 will return melee weapon header, -2 the ranged one
const ITMExtHeader* Item::GetExtHeader(int which) const
{
	if (which < 0) return GetWeaponHeader(which == -2);

	if (static_cast<int>(ext_headers.size()) <= which) {
		return nullptr;
	}
	return &ext_headers[which];
}

int Item::UseCharge(ieWord *Charges, int header, bool expend) const
{
	const ITMExtHeader *ieh = GetExtHeader(header);
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
	const ITMExtHeader *eh = GetExtHeader(header);
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
		pro->SetEffects(GetEffectBlock(self, target, usage, invslot, idx));
	}
	pro->Range = eh->Range;
	return pro;
}

//this is the implementation of the weapon glow effect in PST
Effect *Item::BuildGlowEffect(int gradient) const
{
	static EffectRef glowRef = { "Color:PulseRGB", -1 };
	//this type of colour uses PAL32, a PST specific palette
	//palette entry to RGB conversion
	const auto& pal32 = core->GetPalette32(gradient);
	ieDword rgb = (pal32[16].r << 16) | (pal32[16].g << 8) | pal32[16].b;
	ieDword location = 0;
	ieDword speed = 128 << 16;
	Effect* fx = EffectQueue::CreateEffect(glowRef, rgb, location | speed, FX_DURATION_INSTANT_WHILE_EQUIPPED);
	return fx;
}

unsigned int Item::GetCastingDistance(int idx) const
{
	const ITMExtHeader *seh = GetExtHeader(idx);
	if (!seh) {
		Log(ERROR, "Item", "Cannot retrieve item header!!! required header: {}, maximum: {}",
			idx, (int) ext_headers.size());
		return 0;
	}
	return (unsigned int) seh->Range;
}

static EffectRef fx_damage_ref = { "Damage", -1 };
// returns a vector with details about any extended headers containing fx_damage
std::vector<DMGOpcodeInfo> Item::GetDamageOpcodesDetails(const ITMExtHeader *header) const
{
	ieDword damage_opcode = EffectQueue::ResolveEffect(fx_damage_ref);
	std::multimap<ieDword, DamageInfoStruct>::iterator it;
	std::vector<DMGOpcodeInfo> damage_opcodes;
	if (!header) return damage_opcodes;
	for (const Effect* fx : header->features) {
		if (fx->Opcode == damage_opcode) {
			// damagetype is the same as in dmgtype.ids but GemRB uses those values
			// shifted by two bytes
			// 0-3 -> 0 (crushing)
			// 2^16+[0-3] -> 1 (acid)
			// 2^17+[0-3] -> 2 (cold)
			// 2^18+[0-3] -> 4 (electricity)
			// and so on. Should be fine up until DAMAGE_MAGICFIRE, where we may start making wrong lookups
			ieDword damagetype = fx->Parameter2 >> 16;
			it = core->DamageInfoMap.find(damagetype);
			if (it == core->DamageInfoMap.end()) {
				Log(ERROR, "Combat", "Unhandled damagetype: {}", damagetype);
				continue;
			}
			DMGOpcodeInfo damage;
			// it's lower case instead of title case, but let's see how long it takes for anyone to notice - 26.12.2012
			damage.TypeName = core->GetString(it->second.strref, STRING_FLAGS::NONE);
			damage.DiceThrown = fx->DiceThrown;
			damage.DiceSides = fx->DiceSides;
			damage.DiceBonus = fx->Parameter1;
			damage.Chance = fx->ProbabilityRangeMax - fx->ProbabilityRangeMin;
			damage_opcodes.push_back(damage);
		}
	}
	return damage_opcodes;
}


}
