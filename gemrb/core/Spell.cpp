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

#include "voodooconst.h"

#include "Audio.h"
#include "Game.h"
#include "Interface.h"
#include "Projectile.h"
#include "ProjectileServer.h"
#include "Scriptable/Actor.h"

namespace GemRB {

struct SpellTables {
	struct SpellFocus {
		ieDword stat;
		ieDword val1;
		ieDword val2;
	};
	
	std::vector<SpellFocus> spellfocus;
	unsigned int damageOpcode = 0;
	bool pstflags = false;
	
	static const SpellTables& Get() {
		static SpellTables tables;
		return tables;
	}
private:
	SpellTables() {
		EffectRef dmgref = { "Damage", -1 };
		damageOpcode = EffectQueue::ResolveEffect(dmgref);

		pstflags = core->HasFeature(GF_PST_STATE_FLAGS);
		AutoTable tm = gamedata->LoadTable("splfocus", true);
		if (tm) {
			TableMgr::index_t schoolcount = tm->GetRowCount();

			spellfocus.resize(schoolcount);
			for (TableMgr::index_t i = 0; i < schoolcount; i++) {
				ieDword stat = core->TranslateStat(tm->QueryField(i, 0).c_str());
				ieDword val1 = tm->QueryFieldUnsigned<ieDword>(i, 1);
				ieDword val2 = tm->QueryFieldUnsigned<ieDword>(i, 2);
				spellfocus[i].stat = stat;
				spellfocus[i].val1 = val1;
				spellfocus[i].val2 = val2;
			}
		}
	}
};

int Spell::GetHeaderIndexFromLevel(int level) const
{
	if (level < 0 || ext_headers.empty()) return -1;
	if (Flags & SF_SIMPLIFIED_DURATION) {
		return level;
	}

	for (size_t headerIndex = 0; headerIndex < ext_headers.size() - 1; ++headerIndex) {
		if (ext_headers[headerIndex + 1].RequiredLevel > level) {
			return int(headerIndex);
		}
	}
	return int(ext_headers.size()) - 1;
}

//-1 will return cfb
//0 will always return first spell block
//otherwise set to caster level
static EffectRef fx_casting_glow_ref = { "CastingGlow", -1 };

void Spell::AddCastingGlow(EffectQueue *fxqueue, ieDword duration, int gender) const
{
	char g, t;
	Effect *fx;
	ResRef Resource;

	int cgsound = CastingSound;
	if (cgsound>=0 && duration > 1) {
		//bg2 style
		if(cgsound&0x100) {
			//if duration is less than 3, use only the background sound
			g = 's';
			if (duration>3) {
				switch(gender) {
				//selection of these sounds is almost purely on whim
				//though the sounds of devas/demons are probably better this way
				//all other cases (mostly elementals/animals) don't have sound
				//externalise if you don't mind another 2da
				case SEX_MALE: case SEX_SUMMON_DEMON: g = 'm'; break;
				case SEX_FEMALE: case SEX_BOTH: g = 'f'; break;
				}
			}
		} else {
			//how style, no pure background sound
			if (gender == SEX_FEMALE) {
				g = 'f';
			} else {
				g = 'm';
			}
		}
		if (SpellType==IE_SPL_PRIEST) {
			t = 'p';
		} else {
			t = 'm';
		}
		//check if bg1
		if (!core->HasFeature(GF_CASTING_SOUNDS) && !core->HasFeature(GF_CASTING_SOUNDS2)) {
			Resource.Format("CAS_P{}{:01d}{}", t, std::min(cgsound, 9), g);
		} else {
			Resource.Format("CHA_{}{}{:02d}", g, t, std::min(cgsound & 0xff, 99));
		}
		// only actors have fxqueue's and also the parent function checks for that
		Actor *caster = (Actor *) fxqueue->GetOwner();
		caster->casting_sound = core->GetAudioDrv()->Play(Resource, SFX_CHAN_CASTING, caster->Pos);
	}

	fx = EffectQueue::CreateEffect(fx_casting_glow_ref, 0, CastingGraphics, FX_DURATION_ABSOLUTE);
	fx->Duration = core->GetGame()->GameTime + duration;
	fx->InventorySlot = 0xffff;
	fx->Projectile = 0;
	fxqueue->AddEffect(fx);
}

EffectQueue Spell::GetEffectBlock(Scriptable *self, const Point &pos, int block_index, int level, ieDword pro)
{
	bool pstFriendly = false;
	std::vector<Effect>* features;
	size_t count;
	const auto& tables = SpellTables::Get();
	Actor *caster = self->As<Actor>();

	//iwd2 has this hack
	if (block_index>=0) {
		if (Flags & SF_SIMPLIFIED_DURATION) {
			features = &ext_headers[0].features;
			count = ext_headers[0].features.size();
		} else {
			features = &ext_headers[block_index].features;
			count = ext_headers[block_index].features.size();
			if (tables.pstflags && !(ext_headers[block_index].Hostile&4)) {
				pstFriendly = true;
			}
		}
	} else {
		features = &casting_features;
		count = CastingFeatureCount;
	}
	EffectQueue fxqueue;
	EffectQueue selfqueue;

	for (size_t i = 0; i < count; ++i) {
		Effect& fx = features->at(i);

		// hack the effect according to Level
		if ((Flags & SF_SIMPLIFIED_DURATION) && block_index >= 0 && fx.HasDuration()) {
			fx.Duration = (TimePerLevel * block_index + TimeConstant) * core->Time.round_sec;
		}
		//fill these for completeness, inventoryslot is a good way
		//to discern a spell from an item effect

		fx.InventorySlot = 0xffff;
		//the hostile flag is used to determine if this was an attack
		fx.SourceFlags = Flags;
		//pst spells contain a friendly flag in the spell header
		// while iwd spells never set this bit
		if (fx.Opcode == tables.damageOpcode && !pstFriendly) {
			fx.SourceFlags|=SF_HOSTILE;
		}
		fx.CasterID = self ? self->GetGlobalID() : 0; // needed early for check_type, reset later
		fx.CasterLevel = level;
		fx.SpellLevel = SpellLevel;

		// apply the stat-based spell duration modifier
		if (caster) {
			if (caster->Modified[IE_SPELLDURATIONMODMAGE] && SpellType == IE_SPL_WIZARD) {
				fx.Duration = (fx.Duration * caster->Modified[IE_SPELLDURATIONMODMAGE]) / 100;
			} else if (caster->Modified[IE_SPELLDURATIONMODPRIEST] && SpellType == IE_SPL_PRIEST) {
				fx.Duration = (fx.Duration * caster->Modified[IE_SPELLDURATIONMODPRIEST]) / 100;
			}

			//evaluate spell focus feats
			//TODO: the usual problem: which saving throw is better? Easy fix in the data file.
			if (fx.PrimaryType < tables.spellfocus.size()) {
				ieDword stat = tables.spellfocus[fx.PrimaryType].stat;
				if (stat>0) {
					switch (caster->Modified[stat]) {
						case 0: break;
						case 1: fx.SavingThrowBonus += tables.spellfocus[fx.PrimaryType].val1; break;
						default: fx.SavingThrowBonus += tables.spellfocus[fx.PrimaryType].val2; break;
					}
				}
			}
		}

		// item revisions uses a bunch of fx_cast_spell with spells that have effects with no target set
		if (fx.Target == FX_TARGET_UNKNOWN) {
			fx.Target = FX_TARGET_PRESET;
		}

		if (fx.Target != FX_TARGET_PRESET && EffectQueue::OverrideTarget(&fx)) {
			fx.Target = FX_TARGET_PRESET;
		}

		if (fx.Target != FX_TARGET_SELF) {
			fx.Projectile = pro;
			fxqueue.AddEffect(new Effect(fx));
		} else {
			fx.Projectile = 0;
			fx.Pos = pos;
			// effects should be able to affect non living targets
			//This is done by NULL target, the position should be enough
			//to tell which non-actor object is affected
			selfqueue.AddEffect(new Effect(fx));
		}
	}
	if (self && selfqueue) {
		core->ApplyEffectQueue(&selfqueue, caster, self);
	}
	return fxqueue;
}

Projectile *Spell::GetProjectile(Scriptable *self, int header, int level, const Point &target)
{
	const SPLExtHeader *seh = GetExtHeader(header);
	if (!seh) {
		Log(ERROR, "Spell", "Cannot retrieve spell header!!! required header: {}, maximum: {}",
			header, ext_headers.size());
		return NULL;
	}
	Projectile *pro = core->GetProjectileServer()->GetProjectileByIndex(seh->ProjectileAnimation);
	if (seh->features.size()) {
		pro->SetEffects(GetEffectBlock(self, target, header, level, seh->ProjectileAnimation));
	}
	pro->Range = GetCastingDistance(self);
	return pro;
}

//get the casting distance of the spell
//it depends on the casting level of the actor
//if actor isn't given, then the first header is used
unsigned int Spell::GetCastingDistance(Scriptable *Sender) const
{
	int level = 0;
	unsigned int limit = VOODOO_VISUAL_RANGE;
	Actor* actor = Sender->As<Actor>();
	if (actor) {
		level = actor->GetCasterLevel(SpellType);
		limit = actor->GetStat(IE_VISUALRANGE);
	}

	if (level<1) {
		level = 1;
	}
	int idx = GetHeaderIndexFromLevel(level);
	const SPLExtHeader *seh = GetExtHeader(idx);
	if (!seh) {
		Log(ERROR, "Spell", "Cannot retrieve spell header!!! required header: {}, maximum: {}",
			idx, ext_headers.size());
		return 0;
	}

	if (seh->Target==TARGET_DEAD) {
		return 0xffffffff;
	}
	return std::min((unsigned int) seh->Range, limit);
}

// checks if any of the extended headers contains fx_damage
bool Spell::ContainsDamageOpcode() const
{
	for (const SPLExtHeader& header : ext_headers) {
		for (const Effect& fx : header.features) {
			if (fx.Opcode == SpellTables::Get().damageOpcode) {
				return true;
			}
		}
		if (Flags & SF_SIMPLIFIED_DURATION) { // iwd2 has only one header
			break;
		}
	}
	return false;
}

}
