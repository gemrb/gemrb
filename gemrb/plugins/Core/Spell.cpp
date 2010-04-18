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

#include "win32def.h"
#include "Actor.h"
#include "Spell.h"
#include "Projectile.h"
#include "ProjectileServer.h"
#include "Interface.h"
#include "Game.h"

SPLExtHeader::SPLExtHeader(void)
{
	features = NULL;
}

SPLExtHeader::~SPLExtHeader(void)
{
	delete [] features;
}

Spell::Spell(void)
{
	ext_headers = NULL;
	casting_features = NULL;
}

Spell::~Spell(void)
{
	//Spell is in the core, so this is not needed, i guess (Avenger)
	//core->FreeSPLExt(ext_headers, casting_features);
	delete [] ext_headers;
	delete [] casting_features;
}

int Spell::GetHeaderIndexFromLevel(int level) const
{
	if (level<0) return -1;
	if (Flags & SF_SIMPLIFIED_DURATION) {
		return level;
	}
	int block_index;
	for(block_index=0;block_index<ExtHeaderCount-1;block_index++) {
		if (ext_headers[block_index+1].RequiredLevel>level) {
			return block_index;
		}
	}
	return ExtHeaderCount-1;
}

//-1 will return cfb
//0 will always return first spell block
//otherwise set to caster level
static EffectRef fx_casting_glow_ref={"CastingGlow",NULL,-1};

void Spell::AddCastingGlow(EffectQueue *fxqueue, ieDword duration)
{
	Effect *fx = EffectQueue::CreateEffect(fx_casting_glow_ref, 0, CastingGraphics, FX_DURATION_ABSOLUTE);
	fx->Duration = core->GetGame()->GameTime + duration;
	fx->InventorySlot = 0xffff;
	fx->Projectile = 0;
	fxqueue->AddEffect(fx);
	//AddEffect creates a copy, we need to destroy the original
	delete fx;
}

EffectQueue *Spell::GetEffectBlock(Scriptable *self, Point &pos, int block_index, ieDword pro) const
{
	Effect *features;
	int count;

	//iwd2 has this hack
	if (block_index>=0) {
		if (Flags & SF_SIMPLIFIED_DURATION) {
			features = ext_headers[0].features;
			count = ext_headers[0].FeatureCount;
		} else {
			features = ext_headers[block_index].features;
			count = ext_headers[block_index].FeatureCount;
		}
	} else {
		features = casting_features;
		count = CastingFeatureCount;
	}
	EffectQueue *fxqueue = new EffectQueue();

	for (int i=0;i<count;i++) {
		if (Flags & SF_SIMPLIFIED_DURATION) {
			//hack the effect according to Level
			//fxqueue->AddEffect will copy the effect,
			//so we don't risk any overwriting
			if (EffectQueue::HasDuration(features+i)) {
				features[i].Duration = (TimePerLevel*block_index+TimeConstant)*7;
			}
		}
		//fill these for completeness, inventoryslot is a good way
		//to discern a spell from an item effect
		features[i].InventorySlot = 0xffff;
		if (features[i].Target != FX_TARGET_SELF) {
			features[i].Projectile = pro;
			fxqueue->AddEffect( features+i );
		} else {
			Actor *target = (self->Type==ST_ACTOR)?(Actor *) self:NULL;
			features[i].Projectile = 0;
			features[i].PosX=pos.x;
			features[i].PosY=pos.y;
			//FIXME (r7193):
			//This is bad, effects should be able to affect non living targets
			//This is done by NULL target, the position should be enough
			//to tell which non-actor object is affected
			if (target) {
				core->ApplyEffect(features+i, target, self);
			}
		}
	}
	return fxqueue;
}

Projectile *Spell::GetProjectile(Scriptable *self, int header, Point &target) const
{
	SPLExtHeader *seh = GetExtHeader(header);
	if (!seh) {
		printMessage("Spell", "Cannot retrieve spell header!!! ",RED);
		printf("required header: %d, maximum: %d\n", header, (int) ExtHeaderCount);
		return NULL;
	}
	Projectile *pro = core->GetProjectileServer()->GetProjectileByIndex(seh->ProjectileAnimation);
	if (seh->FeatureCount) {
		pro->SetEffects(GetEffectBlock(self, target, header, seh->ProjectileAnimation));
	}
	return pro;
}

//get the casting distance of the spell
//it depends on the casting level of the actor
//if actor isn't given, then the first header is used
//TODO: fix casting level for all class combos
unsigned int Spell::GetCastingDistance(Actor *actor) const
{
	int level = 1;
	if(actor) {
		level = actor->GetStat(IE_LEVEL);
		if(SpellType==IE_SPL_WIZARD) {
			level+=actor->GetStat(IE_CASTINGLEVELBONUSMAGE);
		}
		else if(SpellType==IE_SPL_PRIEST) {
			level+=actor->GetStat(IE_CASTINGLEVELBONUSCLERIC);
		}
	}

	if(level<1) level=1;
	int idx = GetHeaderIndexFromLevel(level);
	SPLExtHeader *seh = GetExtHeader(idx);
	if (!seh) {
		printMessage("Spell", "Cannot retrieve spell header!!! ",RED);
		printf("required header: %d, maximum: %d\n", idx, (int) ExtHeaderCount);
		return 0;
	}
	return (unsigned int) seh->Range;
}
