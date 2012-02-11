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

//This class represents the .spl (spell) files of the game.

#include "Spell.h"

#include "win32def.h"

#include "Audio.h"
#include "Game.h"
#include "Interface.h"
#include "Projectile.h"
#include "ProjectileServer.h"
#include "Scriptable/Actor.h"

struct SpellFocus {
	ieDword stat;
	ieDword val1;
	ieDword val2;
};

static int pstflags = false;
static int inited = false;
SpellFocus *spellfocus = NULL;
int schoolcount = -1;

static void InitSpellTables()
{
	pstflags = !!core->HasFeature(GF_PST_STATE_FLAGS);
	AutoTable tm("splfocus");
	if (tm) {
		schoolcount = tm->GetRowCount();

		spellfocus = new SpellFocus [schoolcount];
		for(int i = 0; i<schoolcount; i++) {
			ieDword stat = core->TranslateStat(tm->QueryField(i, 0));
			ieDword val1 = atoi(tm->QueryField(i, 1));
			ieDword val2 = atoi(tm->QueryField(i, 2));
			spellfocus[i].stat = stat;
			spellfocus[i].val1 = val1;
			spellfocus[i].val2 = val2;
		}
	}
}


void ReleaseMemorySpell()
{
	inited = false;
	delete [] spellfocus;
}

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
	if (!inited) {
		inited = true;
		InitSpellTables();
	}
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
static EffectRef fx_casting_glow_ref = { "CastingGlow", -1 };

void Spell::AddCastingGlow(EffectQueue *fxqueue, ieDword duration, int gender)
{
	char g, t;
	Effect *fx;
	ieResRef Resource;

	int cgsound = CastingSound;
	if (cgsound>=0 && duration > 1) {
		//bg2 style
		if(cgsound&0x100) {
			switch(gender) {
			default: g = 'm'; break;
			case SEX_FEMALE: g = 'f'; break;
			case SEX_OTHER: case SEX_NEITHER: g = 's'; break;
			}
		} else {
			//how style

			switch(gender) {
			default: g = 'm'; break;
			case SEX_FEMALE: g = 'f'; break;
			}
		}
		if (SpellType==IE_SPL_PRIEST) {
			t = 'p';
		} else {
			t = 'm';
		}
		snprintf(Resource, 9,"CHA_%c%c%02d", g, t, cgsound&0xff);
		// only actors have fxqueue's and also the parent function checks for that
		Actor *caster = (Actor *) fxqueue->GetOwner();
		caster->casting_sound = core->GetAudioDrv()->Play(Resource, caster->Pos.x, caster->Pos.y);
	}

	fx = EffectQueue::CreateEffect(fx_casting_glow_ref, 0, CastingGraphics, FX_DURATION_ABSOLUTE);
	fx->Duration = core->GetGame()->GameTime + duration;
	fx->InventorySlot = 0xffff;
	fx->Projectile = 0;
	fxqueue->AddEffect(fx);
	//AddEffect creates a copy, we need to destroy the original
	delete fx;
}

EffectQueue *Spell::GetEffectBlock(Scriptable *self, const Point &pos, int block_index, int level, ieDword pro) const
{
	bool pst_hostile = false;
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
			if (pstflags && !(ext_headers[block_index].Hostile&4)) {
				pst_hostile = true;
			}
		}
	} else {
		features = casting_features;
		count = CastingFeatureCount;
	}
	EffectQueue *fxqueue = new EffectQueue();
	EffectQueue *selfqueue = NULL;

	for (int i=0;i<count;i++) {
		Effect *fx = features+i;

		if ((Flags & SF_SIMPLIFIED_DURATION) && (block_index>=0)) {
			//hack the effect according to Level
			//fxqueue->AddEffect will copy the effect,
			//so we don't risk any overwriting
			if (EffectQueue::HasDuration(features+i)) {
				fx->Duration = (TimePerLevel*block_index+TimeConstant)*core->Time.round_sec;
			}
		}
		//fill these for completeness, inventoryslot is a good way
		//to discern a spell from an item effect

		fx->InventorySlot = 0xffff;
		//the hostile flag is used to determine if this was an attack
		fx->SourceFlags = Flags;
		//pst spells contain a friendly flag in the spell header
		if (pst_hostile) {
			fx->SourceFlags|=SF_HOSTILE;
		}
		fx->CasterLevel = level;

		// apply the stat-based spell duration modifier
		if (self->Type == ST_ACTOR) {
			Actor *caster = (Actor *) self;
			if (caster->Modified[IE_SPELLDURATIONMODMAGE] && SpellType == IE_SPL_WIZARD) {
				fx->Duration = (fx->Duration * caster->Modified[IE_SPELLDURATIONMODMAGE]) / 100;
			} else if (caster->Modified[IE_SPELLDURATIONMODPRIEST] && SpellType == IE_SPL_PRIEST) {
				fx->Duration = (fx->Duration * caster->Modified[IE_SPELLDURATIONMODPRIEST]) / 100;
			}

			//evaluate spell focus feats
			//TODO: the usual problem: which saving throw is better? Easy fix in the data file.
			if (fx->PrimaryType<(ieDword) schoolcount) {
				ieDword stat = spellfocus[fx->PrimaryType].stat;
				if (stat>0) {
					switch (caster->Modified[stat]) {
						case 0: break;
						case 1: fx->SavingThrowBonus += spellfocus[fx->PrimaryType].val1; break;
						default: fx->SavingThrowBonus += spellfocus[fx->PrimaryType].val2; break;
					}
				}
			}
		}

		if (fx->Target != FX_TARGET_SELF) {
			fx->Projectile = pro;
			fxqueue->AddEffect( fx );
		} else {
			fx->Projectile = 0;
			fx->PosX=pos.x;
			fx->PosY=pos.y;
			if (!selfqueue) {
				selfqueue = new EffectQueue();
			}
			// effects should be able to affect non living targets
			//This is done by NULL target, the position should be enough
			//to tell which non-actor object is affected
			selfqueue->AddEffect( fx );
		}
	}
	if (selfqueue) {
		Actor *target = (self->Type==ST_ACTOR)?(Actor *) self:NULL;
		core->ApplyEffectQueue(selfqueue, target, self);
		delete selfqueue;
	}
	return fxqueue;
}

Projectile *Spell::GetProjectile(Scriptable *self, int header, int level, const Point &target) const
{
	SPLExtHeader *seh = GetExtHeader(header);
	if (!seh) {
		printMessage("Spell", "Cannot retrieve spell header!!! required header: %d, maximum: %d\n", RED,
			header, (int) ExtHeaderCount);
		return NULL;
	}
	Projectile *pro = core->GetProjectileServer()->GetProjectileByIndex(seh->ProjectileAnimation);
	if (seh->FeatureCount) {
		pro->SetEffects(GetEffectBlock(self, target, header, level, seh->ProjectileAnimation));
	}
	return pro;
}

//get the casting distance of the spell
//it depends on the casting level of the actor
//if actor isn't given, then the first header is used
unsigned int Spell::GetCastingDistance(Scriptable *Sender) const
{
	int level = 0;
	Actor *actor = NULL;
	if (Sender && Sender->Type==ST_ACTOR) {
		actor = (Actor *) Sender;
		level = actor->GetCasterLevel(SpellType);
	}

	if (level<1) {
		level = 1;
	}
	int idx = GetHeaderIndexFromLevel(level);
	SPLExtHeader *seh = GetExtHeader(idx);
	if (!seh) {
		printMessage("Spell", "Cannot retrieve spell header!!! required header: %d, maximum: %d\n", RED,
			idx, (int) ExtHeaderCount);
		return 0;
	}

	if (seh->Target==TARGET_DEAD) {
		return 0xffffffff;
	}
	return (unsigned int) seh->Range;
}

static EffectRef fx_damage_ref = { "Damage", -1 };
// checks if any of the extended headers contains fx_damage
bool Spell::ContainsDamageOpcode() const
{
	ieDword damage_opcode = EffectQueue::ResolveEffect(fx_damage_ref);
	for (int h=0; h< ExtHeaderCount; h++) {
		for (int i=0; i< ext_headers[h].FeatureCount; i++) {
			Effect *fx = ext_headers[h].features+i;
			if (fx->Opcode == damage_opcode) {
				return true;
			}
		}
		if (Flags & SF_SIMPLIFIED_DURATION) { // iwd2 has only one header
			break;
		}
	}
	return false;
}
