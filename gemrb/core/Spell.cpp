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

#include "ie_stats.h"

#include "Game.h"
#include "Interface.h"
#include "Projectile.h"
#include "ProjectileServer.h"

#include "Logging/Logging.h"
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

	static const SpellTables& Get()
	{
		static SpellTables tables;
		return tables;
	}

private:
	SpellTables()
	{
		EffectRef dmgref = { "Damage", -1 };
		damageOpcode = EffectQueue::ResolveEffect(dmgref);

		pstflags = core->HasFeature(GFFlags::PST_STATE_FLAGS);
		AutoTable tm = gamedata->LoadTable("splfocus", true);
		if (tm) {
			TableMgr::index_t schoolcount = tm->GetRowCount();

			spellfocus.resize(schoolcount);
			for (TableMgr::index_t i = 0; i < schoolcount; i++) {
				ieDword stat = core->TranslateStat(tm->QueryField(i, 0));
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

void Spell::AddCastingGlow(EffectQueue* fxqueue, ieDword duration, int gender) const
{
	char g, t;
	Effect* fx;
	ResRef Resource;

	int cgsound = CastingSound;
	if (cgsound >= 0 && duration > 1) {
		//bg2 style
		if (cgsound & 0x100) {
			//if duration is less than 3, use only the background sound
			g = 's';
			if (duration > 3) {
				switch (gender) {
					//selection of these sounds is almost purely on whim
					//though the sounds of devas/demons are probably better this way
					//all other cases (mostly elementals/animals) don't have sound
					//externalise if you don't mind another 2da
					case SEX_MALE:
					case SEX_SUMMON_DEMON:
						g = 'm';
						break;
					case SEX_FEMALE:
					case SEX_BOTH:
						g = 'f';
						break;
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
		if (SpellType == IE_SPL_PRIEST) {
			t = 'p';
		} else {
			t = 'm';
		}
		//check if bg1
		if (!core->HasFeature(GFFlags::CASTING_SOUNDS) && !core->HasFeature(GFFlags::CASTING_SOUNDS2)) {
			Resource.Format("CAS_P{}{:01d}{}", t, std::min(cgsound, 9), g);
		} else {
			Resource.Format("CHA_{}{}{:02d}", g, t, std::min(cgsound & 0xff, 99));
		}
		// only actors have fxqueue's and also the parent function checks for that
		Actor* caster = (Actor*) fxqueue->GetOwner();
		caster->casting_sound = core->GetAudioPlayback().PlayDirectional(Resource, SFXChannel::Casting, caster->Pos, caster->GetOrientation());
	}

	fx = EffectQueue::CreateEffect(fx_casting_glow_ref, 0, CastingGraphics, FX_DURATION_ABSOLUTE);
	fx->Duration = core->GetGame()->GameTime + duration;
	fx->InventorySlot = 0xffff;
	fx->Projectile = 0;
	fxqueue->AddEffect(fx);
}

// pst did some nasty hardcoding ...
static void AdjustPSTDurations(const Spell* spl, Effect& fx, size_t ignoreFx)
{
	if (fx.TimingMode != 0 && fx.TimingMode != 3) return;
	if (fx.Opcode == 195) return; // pointless to touch fx_tint_screen and at least spwi308 excluded it manually

	static AutoTable durationOverride = gamedata->LoadTable("spldurat", true);
	if (!durationOverride) return;
	auto row = durationOverride->GetRowIndex(spl->Name);
	if (row == TableMgr::npos) return;

	size_t skipFx = durationOverride->QueryFieldUnsigned<ieDword>(row, 2);
	if (skipFx == ignoreFx) return;

	fx.Duration = durationOverride->QueryFieldUnsigned<ieDword>(row, 0);
	fx.Duration += durationOverride->QueryFieldUnsigned<ieDword>(row, 1) * fx.CasterLevel;
	ieDword maxDuration = durationOverride->QueryFieldUnsigned<ieDword>(row, 4);
	if (maxDuration != 0) {
		fx.Duration = std::min(fx.Duration, maxDuration);
	}
	// onset delay override
	ieWord special = durationOverride->QueryFieldUnsigned<ieWord>(row, 3);
	if (special != 0) {
		fx.IsVariable = special;
	}
}

EffectQueue Spell::GetEffectBlock(Scriptable* self, const Point& pos, int block_index, int level, ieDword pro)
{
	bool pstFriendly = false;
	std::vector<Effect>* features;
	size_t count;
	const auto& tables = SpellTables::Get();
	Actor* caster = Scriptable::As<Actor>(self);

	//iwd2 has this hack
	if (block_index >= 0) {
		if (Flags & SF_SIMPLIFIED_DURATION) {
			features = &ext_headers[0].features;
			count = ext_headers[0].features.size();
		} else {
			features = &ext_headers[block_index].features;
			count = ext_headers[block_index].features.size();
			if (tables.pstflags && !(ext_headers[block_index].Hostile & 4)) {
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

		fx.CasterLevel = level;
		// hack the effect according to Level
		if ((Flags & SF_SIMPLIFIED_DURATION) && block_index >= 0 && fx.HasDuration()) {
			fx.Duration = (TimePerLevel * block_index + TimeConstant) * core->Time.round_sec;
		} else if (tables.pstflags) {
			// many pst spells require unhacking and we can't use the simplified duration bit and custom projectiles for all
			// luckily this was redone in pstee simply with extra extended headers
			// NOTE: some of these have several headers for increased range per level, but
			//       simplified duration always operates only on the first, so we can use it even less
			AdjustPSTDurations(this, fx, i);
		}
		//fill these for completeness, inventoryslot is a good way
		//to discern a spell from an item effect

		fx.InventorySlot = 0xffff;
		//the hostile flag is used to determine if this was an attack
		fx.SourceFlags = Flags;
		//pst spells contain a friendly flag in the spell header
		// while iwd spells never set this bit
		if (fx.Opcode == tables.damageOpcode && !pstFriendly) {
			fx.SourceFlags |= SF_HOSTILE;
		}
		fx.CasterID = self ? self->GetGlobalID() : 0; // needed early for check_type, reset later
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
				if (stat > 0) {
					switch (caster->Modified[stat]) {
						case 0:
							break;
						case 1:
							fx.SavingThrowBonus += tables.spellfocus[fx.PrimaryType].val1;
							break;
						default:
							fx.SavingThrowBonus += tables.spellfocus[fx.PrimaryType].val2;
							break;
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

		if (fx.Target == FX_TARGET_SELF) {
			fx.Projectile = 0;
			fx.Pos = pos;
			// effects should be able to affect non living targets
			//This is done by NULL target, the position should be enough
			//to tell which non-actor object is affected
			selfqueue.AddEffect(new Effect(fx));
		} else {
			fx.Projectile = pro;
			fxqueue.AddEffect(new Effect(fx));
		}
	}
	if (self && selfqueue) {
		core->ApplyEffectQueue(&selfqueue, caster, self);
	}
	return fxqueue;
}

Projectile* Spell::GetProjectile(Scriptable* self, int header, int level, const Point& target)
{
	const SPLExtHeader* seh = GetExtHeader(header);
	if (!seh) {
		Log(ERROR, "Spell", "Cannot retrieve spell header!!! required header: {}, maximum: {}",
		    header, ext_headers.size());
		return NULL;
	}
	Projectile* pro = core->GetProjectileServer()->GetProjectileByIndex(seh->ProjectileAnimation);
	if (seh->features.size()) {
		EffectQueue fxqueue = GetEffectBlock(self, target, header, level, seh->ProjectileAnimation);
		pro->SetEffects(std::move(fxqueue));
	}
	if (seh->ProjectileAnimation == 108 && pro->Extension && pro->ExtFlags & PEF_LINE) {
		// fix hardcoded iwd scorcher having no damage payload; supposedly caused
		// 3-18 points of fire damage to the target, no save, while anyone in the flame's
		// path took 2-16, but they are allowed a save for half
		// we just do 3d6 with a save to everyone
		EffectQueue& fxqueue = pro->GetEffects();
		static EffectRef dmgRef = { "Damage", -1 };
		EffectQueue::ResolveEffect(dmgRef);
		if (!fxqueue.HasEffect(dmgRef)) {
			Effect* fx = EffectQueue::CreateEffect(dmgRef, 0, 0x80000, FX_DURATION_INSTANT_PERMANENT);
			fx->DiceThrown = 3;
			fx->DiceSides = 6;
			fx->SavingThrowType = core->HasFeature(GFFlags::RULES_3ED) ? 8 : 1;
			fx->IsSaveForHalfDamage = 1;
			fx->Target = FX_TARGET_PRESET;
			fxqueue.AddEffect(fx);
		}
	}
	pro->Range = GetCastingDistance(self);
	pro->form = seh->SpellForm;
	return pro;
}

//get the casting distance of the spell
//it depends on the casting level of the actor
//if actor isn't given, then the first header is used
unsigned int Spell::GetCastingDistance(Scriptable* Sender) const
{
	int level = 0;
	unsigned int limit = Sender->GetVisualRange();
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (actor) {
		level = actor->GetCasterLevel(SpellType);
	}

	if (level < 1) {
		level = 1;
	}
	int idx = GetHeaderIndexFromLevel(level);
	const SPLExtHeader* seh = GetExtHeader(idx);
	if (!seh) {
		Log(ERROR, "Spell", "Cannot retrieve spell header!!! required header: {}, maximum: {}",
		    idx, ext_headers.size());
		return 0;
	}

	if (seh->Target == TARGET_DEAD) {
		return 0x7fffffff;
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
