/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

//This class represents the .cre (creature) files.
//Any player or non-player character is a creature.
//Actor is a scriptable object (Scriptable). See Scriptable.cpp

#include "Scriptable/Actor.h"

#include "ie_feats.h"
#include "overlays.h"
#include "strrefs.h"
#include "opcode_params.h"
#include "voodooconst.h"

#include "DataFileMgr.h"
#include "DialogHandler.h" // checking for dialog
#include "Game.h"
#include "GlobalTimer.h"
#include "Interface.h"
#include "DisplayMessage.h"
#include "GameData.h"
#include "ImageMgr.h"
#include "Item.h"
#include "PolymorphCache.h" // stupid polymorph cache hack
#include "Projectile.h"
#include "ProjectileServer.h"
#include "ScriptEngine.h"
#include "Spell.h"
#include "Sprite2D.h"
#include "TableMgr.h"
#include "damages.h"
#include "GameScript/GSUtils.h" //needed for DisplayStringCore
#include "GameScript/GameScript.h"
#include "GUI/GameControl.h"
#include "RNG.h"
#include "Scriptable/InfoPoint.h"
#include "ScriptedAnimation.h"
#include "System/FileFilters.h"
#include "StringMgr.h"

#include <cmath>
#include <string>

namespace GemRB {

static const std::string blank;

//configurable?
const ieDword ref_lightness = 43;

static int sharexp = SX_DIVIDE|SX_COMBAT;
static int classcount = -1;
static std::vector<int> turnLevelOffset;
static std::vector<int> bookTypes;
static std::vector<int> xpCap;
static std::vector<int> noProfPenalty;
static std::vector<int> castingStat;
static std::vector<int> iwd2SPLTypes;
static std::vector<std::vector<int>> levelStats;
static std::vector<int> dualSwap;
static std::vector<int> multiclassIDs;
static std::vector<int> maxLevelForHpRoll;
static std::map<TableMgr::index_t, std::vector<int> > skillstats;
static std::map<int, int> stat2skill;
static std::vector<std::vector<int>> wmLevelMods;
static const ieVariable CounterNames[4] = { "GOOD", "LAW", "LADY", "MURDER" };

//verbal constant specific data
static EnumArray<Verbal> VCMap;
static ieDword sel_snd_freq = 0;
static ieDword cmd_snd_freq = 0;
static ieDword crit_hit_scr_shake = 1;
static ieDword bored_time = 3000;
static ieDword footsteps = 1;
static ieDword war_cries = 1;
static ieDword GameDifficulty = DIFF_CORE;
static ieDword StoryMode = 0;
static ieDword NoExtraDifficultyDmg = 0;
static ieDword PreferSneakAttack = 0;
static int DifficultyLuckMod = 0;
static int DifficultyDamageMod = 0;
static int DifficultySaveMod = 0;

#define MAX_FEATV 4294967295U // 1<<32-1 (used for the triple-stat feat handling)

static ResRef featSpells[ES_COUNT];
static bool pstflags = false;
static bool nocreate = false;
static bool third = false;
static bool iwd2class = false;
//used in many places, but different in engines
static ieDword state_invisible = STATE_INVISIBLE;
static constexpr ieWord IT_SCROLL = 11;
static constexpr ieWord IT_WAND = 35;

static int fiststat = IE_CLASS;

//conversion for 3rd ed
static int isclass[ISCLASSES]={0,0,0,0,0,0,0,0,0,0,0,0,0};

static const int mcwasflags[ISCLASSES] = {
	MC_WAS_FIGHTER, MC_WAS_MAGE, MC_WAS_THIEF, 0, 0, MC_WAS_CLERIC,
	MC_WAS_DRUID, 0, 0, MC_WAS_RANGER, 0, 0, 0};
static std::string isclassnames[ISCLASSES] = {
	"FIGHTER", "MAGE", "THIEF", "BARBARIAN", "BARD", "CLERIC",
	"DRUID", "MONK", "PALADIN", "RANGER", "SORCERER", "CLASS12", "CLASS13" };
static const int levelslotsiwd2[ISCLASSES]={IE_LEVELFIGHTER, IE_LEVELMAGE, IE_LEVELTHIEF,
	IE_LEVELBARBARIAN, IE_LEVELBARD, IE_LEVELCLERIC, IE_LEVELDRUID, IE_LEVELMONK,
	IE_LEVELPALADIN, IE_LEVELRANGER, IE_LEVELSORCERER, IE_LEVELCLASS12, IE_LEVELCLASS13};

#define BGCLASSCNT 23
//fighter is the default level here
//fixme, make this externalized
//this map could probably be auto-generated BG2 class ID -> ISCLASS
static const int levelslotsbg[BGCLASSCNT]={ISFIGHTER, ISMAGE, ISFIGHTER, ISCLERIC, ISTHIEF,
	ISBARD, ISPALADIN, 0, 0, 0, 0, ISDRUID, ISRANGER, 0,0,0,0,0,0,ISSORCERER, ISMONK,
	ISCLASS12, ISCLASS13};
// map isClass -> (IWD2) class ID
static unsigned int classesiwd2[ISCLASSES] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// class -> kits map
struct ClassKits {
	std::vector<TableMgr::index_t> indices;
	std::vector<ieDword> ids;
	std::vector<ResRef> clabs;
	std::vector<std::string> kitNames;
	ResRef clab;
	std::string className;
};
static std::map<int, ClassKits> class2kits;

//this map could probably be auto-generated (isClass -> IWD2 book ID)
static const int booksiwd2[ISCLASSES]={-1, IE_IWD2_SPELL_WIZARD, -1, -1,
 IE_IWD2_SPELL_BARD, IE_IWD2_SPELL_CLERIC, IE_IWD2_SPELL_DRUID, -1,
 IE_IWD2_SPELL_PALADIN, IE_IWD2_SPELL_RANGER, IE_IWD2_SPELL_SORCERER, -1, -1};

//stat values are 0-255, so a byte is enough
static ieByte featstats[MAX_FEATS]={0
};
static ieByte featmax[MAX_FEATS]={0
};

// reputation modifiers
#define CLASS_PCCUTOFF 32
#define CLASS_INNOCENT 155
#define CLASS_FLAMINGFIST 156

static std::vector<ActionButtonRow> GUIBTDefaults; // qslots per-class rows
static std::vector<ActionButtonRow2> OtherGUIButtons;
ActionButtonRow DefaultButtons = {ACT_TALK, ACT_WEAPON1, ACT_WEAPON2,
	ACT_QSPELL1, ACT_QSPELL2, ACT_QSPELL3, ACT_CAST, ACT_USE, ACT_QSLOT1, ACT_QSLOT2,
	ACT_QSLOT3, ACT_INNATE};
static int QslotTranslation = false;
static int DeathOnZeroStat = true;
static int IWDSound = false;
static ieDword TranslucentShadows = 0;
static unsigned int SpellStatesSize = 0; //and this is for the spellStates bitfield

static const char iwd2gemrb[32] = {
	0,0,20,2,22,25,0,14,
	15,23,13,0,1,24,8,21,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0
};
static const char gemrb2iwd[32] = {
	11,12,3,71,72,73,0,0, //0
	14,80,83,82,81,10,7,8, //8
	0,0,0,0,2,15,4,9, //16
	13,5,0,0,0,0,0,0 //24
};

//letters for char sound resolution bg1/bg2
static EnumArray<Verbal, char> csound {'\0'};

static void InitActorTables();

#define DAMAGE_LEVELS 19

// ANIMATION1 in dmgtypes.2da, except it only has fire, electricity, cold AND no levels
// we have both maps in damage.2da, plus the gradient info in d_gradient
static ResRef d_main[DAMAGE_LEVELS] = {
	//slot 0 is not used in the original engine
	"BLOODCR","BLOODS","BLOODM","BLOODL", //blood
	"SPFIRIMP","SPFIRIMP","SPFIRIMP",     //fire
	"SPSHKIMP","SPSHKIMP","SPSHKIMP",     //spark
	"SPFIRIMP","SPFIRIMP","SPFIRIMP",     //ice
	"SHACID","SHACID","SHACID",           //acid
	"SPDUSTY2","SPDUSTY2","SPDUSTY2"      //disintegrate
};
// ANIMATION2 in dmgtypes.2da with the same limitations
static ResRef d_splash[DAMAGE_LEVELS] = {
	"","","","",
	"SPBURN","SPBURN","SPBURN", //flames
	"SPSPARKS","SPSPARKS","SPSPARKS", //sparks
	"","","",
	"","","",
	"","",""
};

#define BLOOD_GRADIENT 19
#define FIRE_GRADIENT 19
#define ICE_GRADIENT 71
#define STONE_GRADIENT 93

static int d_gradient[DAMAGE_LEVELS] = {
	BLOOD_GRADIENT,BLOOD_GRADIENT,BLOOD_GRADIENT,BLOOD_GRADIENT,
	FIRE_GRADIENT,FIRE_GRADIENT,FIRE_GRADIENT,
	-1,-1,-1,
	ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT,
	-1,-1,-1,
	-1,-1,-1
};

static ResRef hc_overlays[OVERLAY_COUNT]={"SANCTRY","SPENTACI","SPMAGGLO","SPSHIELD",
"GREASED","WEBENTD","MINORGLB","","","","","","","","","","","","","","",
"","","","SPTURNI2","SPTURNI","","","","","",""};
static ieDword hc_locations = 0;
static int hc_flags[OVERLAY_COUNT];
#define HC_INVISIBLE 1

// thieving skill dexterity and race boni vectors
std::vector<std::vector<int> > skilldex;
std::vector<std::vector<int> > skillrac;

// subset of races.2da
std::map<unsigned int, int> favoredMap;
std::map<unsigned int, std::string> raceID2Name;

// iwd2 class to-hit and apr tables read into a single object
ResRefMap<std::vector<BABTable>> IWD2HitTable;
std::map<int, ResRef> BABClassMap; // maps classis (not id!) to the BAB table

EnumArray<Modal, ModalStatesStruct> ModalStates;
std::map<int, ieByte> numWeaponSlots;

//for every game except IWD2 we need to reverse TOHIT
static int ReverseToHit=true;
static int CheckAbilities=false;

// from FXOpcodes
#define PI_DRUNK   5
#define PI_FATIGUE 39
#define PI_PROJIMAGE  77

static EffectRef fx_set_haste_state_ref = { "State:Hasted", -1 };
static EffectRef fx_set_slow_state_ref = { "State:Slowed", -1 };
static EffectRef fx_sleep_ref = { "State:Helpless", -1 };
static EffectRef fx_cleave_ref = { "Cleave", -1 };
static EffectRef fx_tohit_vs_creature_ref = { "ToHitVsCreature", -1 };
static EffectRef fx_damage_vs_creature_ref = { "DamageVsCreature", -1 };
static EffectRef fx_mirrorimage_ref = { "MirrorImageModifier", -1 };
static EffectRef fx_set_charmed_state_ref = { "State:Charmed", -1 };
static EffectRef fx_cure_sleep_ref = { "Cure:Sleep", -1 };
static EffectRef fx_to_hit_modifier_ref = { "ToHitModifier", -1 };
static EffectRef fx_damage_bonus_modifier1_ref = { "DamageBonusModifier" , -1 };
static EffectRef fx_damage_bonus_modifier_ref = { "DamageBonusModifier2", -1 };
static EffectRef fx_display_portrait_icon_ref = { "Icon:Display", -1 };
static EffectRef fx_set_stun_state_ref = { "State:Stun", -1 };
//bg2 and iwd1
static EffectRef control_creature_ref = { "ControlCreature", -1 };
//iwd1 and iwd2
static EffectRef fx_eye_sword_ref = { "EyeOfTheSword", -1 };
static EffectRef fx_eye_mage_ref = { "EyeOfTheMage", -1 };
//iwd2
static EffectRef control_undead_ref = { "ControlUndead2", -1 };
static EffectRef fx_cure_poisoned_state_ref = { "Cure:Poison", -1 };
static EffectRef fx_cure_hold_state_ref = { "Cure:Hold", -1 };
static EffectRef fx_cure_stun_state_ref = { "Cure:Stun", -1 };
static EffectRef fx_remove_portrait_icon_ref = { "Icon:Remove", -1 };
static EffectRef fx_unpause_caster_ref = { "Cure:CasterHold", -1 };
static EffectRef fx_ac_vs_creature_type_ref = { "ACVsCreatureType", -1 };
static EffectRef fx_puppetmarker_ref = { "PuppetMarker", -1 };
static EffectRef fx_stoneskin_ref = { "StoneSkinModifier", -1 };
static EffectRef fx_stoneskin2_ref = { "StoneSkin2Modifier", -1 };
static EffectRef fx_aegis_ref = { "Aegis", -1 };
static EffectRef fx_cloak_ref = { "Overlay", -1 };
static EffectRef fx_damage_ref = { "Damage", -1 };
static EffectRef fx_melee_ref = { "SetMeleeEffect", -1 };
static EffectRef fx_ranged_ref = { "SetRangedEffect", -1 };
static EffectRef fx_cant_use_item_ref = { "CantUseItem", -1 };
static EffectRef fx_cant_use_item_type_ref = { "CantUseItemType", -1 };
static EffectRef fx_item_usability_ref = { "Usability:ItemUsability", -1 };
static EffectRef fx_remove_invisible_state_ref = { "ForceVisible", -1 };
static EffectRef fx_remove_sanctuary_ref = { "Cure:Sanctuary", -1 };
static EffectRef fx_disable_button_ref = { "DisableButton", -1 };
static EffectRef fx_damage_reduction_ref = { "DamageReduction", -1 };
static EffectRef fx_missile_damage_reduction_ref = { "MissileDamageReduction", -1 };
static EffectRef fx_smite_evil_ref = { "SmiteEvil", -1 };
static EffectRef fx_attacks_per_round_modifier_ref = { "AttacksPerRoundModifier", -1 };
static EffectRef fx_minimum_base_stats_ref = { "MinimumBaseStats", -1 };
static EffectRef fx_animation_override_data_ref = { "AnimationOverrideData", -1 };
static EffectRef fx_enchantment_vs_creature_type_ref = { "EnchantmentVsCreatureType", -1 };
static EffectRef fx_enchantment_bonus_ref = { "EnchantmentBonus", -1 };
static EffectRef fx_save_vs_school_bonus_ref = { "SaveVsSchoolModifier", -1 };
static EffectRef fx_set_diseased_state_ref = { "State:Diseased", -1 };

//used by iwd2
static const ResRef CripplingStrikeRef = "cripstr";
static const ResRef DirtyFightingRef = "dirty";
static const ResRef ArterialStrikeRef = "artstr";

static const int weapon_damagetype[] = {DAMAGE_CRUSHING, DAMAGE_PIERCING,
	DAMAGE_CRUSHING, DAMAGE_SLASHING, DAMAGE_MISSILE, DAMAGE_STUNNING};

static int avBase, avStance;
struct avType {
	ResRef avresref;
	AutoTable avtable;
	int stat;
};

static std::vector<avType> avPrefix;

Actor::Actor()
	: Movable( ST_ACTOR )
{
	ResetPathTries();
	nextComment = 100 + RAND(0, 350); // 7-30s delay

	inventory.SetInventoryType(ieInventoryType::CREATURE);

	fxqueue.SetOwner( this );
	inventory.SetOwner( this );
	if (classcount<0) {
		//This block is executed only once, when the first actor is loaded
		InitActorTables();

		TranslucentShadows = core->GetVariable("Translucent Shadows", 0);
	}
	static size_t maxProjectileCount = core->GetProjectileServer()->GetHighestProjectileNumber();
	projectileImmunity.resize(maxProjectileCount);

	//these are used only in iwd2 so we have to default them
	for (int i = 0; i < 7; i++) {
		BaseStats[IE_HATEDRACE2+i]=0xff;
	}

	RollSaves();
	spellStates = (ieDword *) calloc(SpellStatesSize, sizeof(ieDword));

	AC.SetOwner(this);
	ToHit.SetOwner(this);
}

Actor::~Actor(void)
{
	delete anims;
	delete PCStats;

	for (ScriptedAnimation* vvc : vfxQueue) {
		delete vvc;
	}

	delete attackProjectile;
	delete polymorphCache;

	free(spellStates);
}

void Actor::SetFistStat(ieDword stat)
{
	fiststat = stat;
}

void Actor::SetDefaultActions(int qslot, ieByte slot1, ieByte slot2, ieByte slot3)
{
	QslotTranslation=qslot;
	DefaultButtons[0]=slot1;
	DefaultButtons[1]=slot2;
	DefaultButtons[2]=slot3;
}

void Actor::SetName(String str, unsigned char type)
{
	String* name = nullptr;
	if (type == 1) {
		name = &LongName;
	} else {
		name = &ShortName;
	}
	std::swap(*name, str);
	TrimString(*name);

	if (type == 0) {
		LongName = ShortName;
	}
}

void Actor::SetName(ieStrRef strref, unsigned char type)
{
	String name;
	if (type <= 1) {
		name = core->GetString(strref);
		LongStrRef = strref;
		if (type == 0)
			ShortStrRef = strref;
	} else {
		name = core->GetString(strref);
		ShortStrRef = strref;
	}
	SetName(std::move(name), type);
}

void Actor::SetAnimationID(unsigned int AnimID)
{
	//if the palette is locked, then it will be transferred to the new animation
	Holder<Palette> recover = nullptr;
	ResRef paletteResRef;

	if (anims) {
		if (anims->lockPalette) {
			recover = anims->PartPalettes[PAL_MAIN];
		}
		// Take ownership so the palette won't be deleted
		if (recover) {
			paletteResRef = anims->PaletteResRef[PAL_MAIN];
			if (recover->named) {
				recover = gamedata->GetPalette(paletteResRef);
			}
		}
		delete anims;
	}

	// hacking PST no palette
	if (core->HasFeature(GFFlags::ONE_BYTE_ANIMID) && (AnimID & 0xf000) == 0xe000) {
		if (BaseStats[IE_COLORCOUNT]) {
			Log(WARNING, "Actor", "Animation ID {:#x} is supposed to be real colored (no recoloring), patched creature", AnimID);
		}
		BaseStats[IE_COLORCOUNT] = 0;
	}

	anims = new CharAnimations(AnimID & 0xffff, BaseStats[IE_ARMOR_TYPE]);
	if (!anims || anims->ResRefBase.IsEmpty()) {
		delete anims;
		anims = nullptr;
		Log(ERROR, "Actor", "Missing animation for {}", fmt::WideToChar{GetName()});
		return;
	}
	anims->SetOffhandRef(ShieldRef);
	anims->SetHelmetRef(HelmetRef);
	anims->SetWeaponRef(WeaponRef);

	//if we have a recovery palette, then set it back
	assert(anims->PartPalettes[PAL_MAIN] == 0);
	anims->PartPalettes[PAL_MAIN] = recover;
	if (recover) {
		anims->lockPalette = true;
		anims->PaletteResRef[PAL_MAIN] = paletteResRef;
	}
	//bird animations are not hindered by searchmap
	//only animations with a space of 0 in avatars.2da files use this feature
	if (anims->GetCircleSize() != 0) {
		BaseStats[IE_DONOTJUMP]=0;
	} else {
		BaseStats[IE_DONOTJUMP]=DNJ_BIRD;
	}
	SetCircleSize();
	anims->SetColors(&BaseStats[IE_COLORS]);

	// PST and EE 2.0+ use an ini to define animation data, including walk and run speed
	// the rest had it hardcoded
	if (!core->HasFeature(GFFlags::RESDATA_INI)) {
		// handle default speed and per-animation overrides
		TableMgr::index_t row = TableMgr::npos;
		static AutoTable extspeed = gamedata->LoadTable("moverate", true);
		if (extspeed) {
			const std::string& animHex = fmt::format("{:#04x}", AnimID);
			row = extspeed->FindTableValue(0UL, animHex);
			if (row != TableMgr::npos) {
				int rate = extspeed->QueryFieldSigned<int>(row, 1);
				SetBase(IE_MOVEMENTRATE, rate);
			}
		} else {
			Log(MESSAGE, "Actor", "No moverate.2da found, using animation ({:#x}) for speed fallback!", AnimID);
		}
		if (row == TableMgr::npos) {
			const auto* anim = anims->GetAnimation(IE_ANI_WALK, S);
			if (anim) {
				SetBase(IE_MOVEMENTRATE, anim->at(0)->GetFrameCount());
			} else {
				Log(WARNING, "Actor", "Unable to determine movement rate for animation {:#x}!", AnimID);
			}
		}
	}

	// set internal speed too, since we may need it in the same tick (eg. csgolem in the bg2 intro)
	SetSpeed(false);
}

CharAnimations* Actor::GetAnims() const
{
	return anims;
}

/** Returns a Stat value (Base Value + Mod) */
Actor::stat_t Actor::GetStat(unsigned int StatIndex) const
{
	if (StatIndex >= MAX_STATS) {
		return 0xdadadada;
	}
	return Modified[StatIndex];
}

/** Always return a final stat value not partially calculated ones */
Actor::stat_t Actor::GetSafeStat(unsigned int StatIndex) const
{
	if (StatIndex >= MAX_STATS) {
		return 0xdadadada;
	}
	if (PrevStats) return PrevStats[StatIndex];
	return Modified[StatIndex];
}

void Actor::SetCircleSize()
{
	if (!anims)
		return;

	const GameControl *gc = core->GetGameControl();
	float oscillationFactor = 1.0f;
	Color color;
	int normalIdx;
	if (UnselectableTimer) {
		color = ColorMagenta;
		normalIdx = 4;
	} else if (Modified[IE_STATE_ID] & STATE_PANIC || Modified[IE_CHECKFORBERSERK]) {
		color = ColorYellow;
		normalIdx = 5;
	} else if (gc && ((gc->InDialog() && gc->dialoghandler->IsTarget(this)) || remainingTalkSoundTime > 0)) {
		color = ColorWhite;
		normalIdx = 3; //?? made up

		if (remainingTalkSoundTime > 0) {
			/**
			 * Approximation: pulsating at about 2Hz over a notable radius growth.
			 * Maybe check this relation for dragons and rats, too.
			 */
			oscillationFactor = 1.1F + float(std::sin(double(remainingTalkSoundTime) * (4 * M_PI) / 1000)) * 0.1F;
		}
	} else {
		switch (Modified[IE_EA]) {
			case EA_PC:
			case EA_FAMILIAR:
			case EA_ALLY:
			case EA_CONTROLLED:
			case EA_CHARMED:
			case EA_EVILBUTGREEN:
			case EA_GOODCUTOFF:
				color = ColorGreen;
				normalIdx = 0;
				break;
			case EA_EVILCUTOFF:
				color = ColorYellow;
				normalIdx = 5;
				break;
			case EA_ENEMY:
			case EA_GOODBUTRED:
			case EA_CHARMEDPC:
				color = ColorRed;
				normalIdx = 1;
				break;
			default:
				color = ColorCyan;
				normalIdx = 2;
				break;
		}
	}

	// circle size 0 won't display, so we can ignore it when clamping
	int csize = Clamp(anims->GetCircleSize(), 1, MAX_CIRCLE_SIZE) - 1;
	int selectedIdx = (normalIdx == 0) ? 3 : normalIdx;
	SetCircle(anims->GetCircleSize(), oscillationFactor, color, core->GroundCircles[csize][normalIdx], core->GroundCircles[csize][selectedIdx]);
}

static void ApplyClab_internal(Actor* actor, const ResRef& clab, int level, bool remove, int diff)
{
	AutoTable table = gamedata->LoadTable(clab);
	if (!table) return;

	TableMgr::index_t row = table->GetRowCount();
	int maxLevel = level;
	// don't remove clabs from levels we haven't attained yet, just in case they contain non-sticky
	// permanent effects like the charisma degradation in the oozemaster
	if (remove) maxLevel -= diff;
	for(int i=0; i<maxLevel; i++) {
		for (TableMgr::index_t j = 0; j < row; ++j) {
			const ieVariable res = table->QueryField(j, i); // not really a variable, we just need a big enough buffer
			if (IsStar(res)) continue;

			ResRef clabRef = ResRef(res.begin() + 3);
			if (res.BeginsWith("AP_")) {
				if (remove) {
					actor->fxqueue.RemoveAllEffects(clabRef);
				} else {
					core->ApplySpell(clabRef, actor, actor, 0);
				}
			} else if (res.BeginsWith("GA_")) {
				if (remove) {
					actor->spellbook.RemoveSpell(clabRef);
				} else {
					actor->LearnSpell(clabRef, LS_MEMO);
				}
			} else if (res.BeginsWith("FA_")) {//iwd2 only: innate name strref
				//memorize these?
				// we now learn them just to get the feedback string out
				if (remove) {
					actor->fxqueue.RemoveAllEffects(clabRef);
				} else {
					actor->LearnSpell(clabRef, LS_MEMO | LS_LEARN, IE_IWD2_SPELL_INNATE);
					actor->spellbook.RemoveSpell(clabRef);
					core->ApplySpell(clabRef, actor, actor, 0);
				}
			} else if (res.BeginsWith("FS_")) {//iwd2 only: song name strref (used by unused kits)
				//don't memorize these?
				if (remove) {
					actor->fxqueue.RemoveAllEffects(clabRef);
				} else {
					actor->LearnSpell(clabRef, LS_LEARN, IE_IWD2_SPELL_SONG);
					actor->spellbook.RemoveSpell(clabRef);
					core->ApplySpell(clabRef, actor, actor, 0);
				}
			} else if (res.BeginsWith("RA_")) {//iwd2 only
				//remove ability
				int x = atoi(res.c_str() + 3);
				actor->spellbook.RemoveSpell(x);
			}
		}
	}

}

#define BG2_KITMASK  0xffffc000
#define KIT_BASECLASS 0x4000
#define KIT_SWASHBUCKLER KIT_BASECLASS+12
#define KIT_WILDMAGE KIT_BASECLASS+30
#define KIT_BARBARIAN KIT_BASECLASS+31

// iwd2 supports multiple kits per actor, but sanely only one kit per class
static TableMgr::index_t GetIWD2KitIndex (ieDword kit, ieDword baseclass=0, bool strict=false)
{
	if (!kit) return TableMgr::npos;

	if (baseclass != 0) {
		int idx = 0;
		for (const auto& aKit : class2kits[baseclass].ids) {
			if (kit & aKit) return class2kits[baseclass].indices[idx];
			idx++;
		}

		if (strict) return TableMgr::npos;
		// this is also hit for kitted multiclasses like illusionist/thieves, who we take care of in the second loop
		if (iwd2class) Log(DEBUG, "Actor", "GetIWD2KitIndex: didn't find kit {} at expected class {}, recalculating!", kit, baseclass);
	}

	// no class info passed or dc/mc, so infer the kit's parent single class
	for (const auto& clsKitPair : class2kits) {
		int idx = 0;
		for (const auto& aKit : clsKitPair.second.ids) {
			if (kit & aKit) return clsKitPair.second.indices[idx];
			idx++;
		}
	}

	Log(ERROR, "Actor", "GetIWD2KitIndex: didn't find kit {} for any class, ignoring!", kit);
	return TableMgr::npos;
}

TableMgr::index_t Actor::GetKitIndex (ieDword kit, ieDword baseclass) const
{
	TableMgr::index_t kitindex = 0;

	if (iwd2class) {
		return GetIWD2KitIndex(kit, baseclass);
	}

	if ((kit&BG2_KITMASK) == KIT_BASECLASS) {
		kitindex = kit&0xfff;
		if (!kitindex && !baseclass) return 0;
	}

	if (kitindex == 0) {
		if (!baseclass) baseclass = GetActiveClass();
		kitindex = GetIWD2KitIndex(kit, baseclass);
		if (kitindex == TableMgr::npos) {
			kitindex = 0;
		}
	}

	return kitindex;
}

//applies a kit on the character
bool Actor::ApplyKit(bool remove, ieDword baseclass, int diff)
{
	ieDword kit = GetStat(IE_KIT);
	ieDword kitclass = 0;
	TableMgr::index_t row = GetKitIndex(kit, baseclass);
	ResRef clab;
	ieDword max = 0;
	ieDword cls = GetStat(IE_CLASS);
	PluginHolder<TableMgr> tm;

	// iwd2 has support for multikit characters, so we have more work
	// at the same time each baseclass has its own level stat, so the logic is cleaner
	// NOTE: in iwd2 there are no pure class options for classes with kits, a kit has to be choosen
	// even generalist mages are a kit the same way as in the older games
	if (iwd2class) {
		// callers always pass a baseclass (only exception are actions not present in iwd2: addkit and addsuperkit)
		assert(baseclass != 0);
		row = GetIWD2KitIndex(kit, baseclass, true);
		bool kitMatchesClass = row != TableMgr::npos;

		if (!kit || !kitMatchesClass) {
			// pure class
			clab = class2kits[baseclass].clab;
		} else {
			// both kit and baseclass are fine and the kit is of this baseclass
			int idx = 0;
			for (const auto& aKit : class2kits[baseclass].ids) {
				if (kit & aKit) {
					clab = class2kits[baseclass].clabs[idx];
					break;
				}
				idx++;
			}
		}
		assert(!clab.IsEmpty());
		cls = baseclass;
	} else if (row) {
		// bg2 kit abilities
		// this doesn't do a kitMatchesClass like above, since it is handled when applying the clab below
		// NOTE: a fighter/illusionist multiclass and illusionist/fighter dualclass would be good test cases, but they don't have any clabs
		// NOTE: multiclass characters will get the clabs applied for all classes at once, so up to three times, since there are three level stats
		// we can't rely on baseclass, since it will match only for combinations of fighters, mages and thieves.
		// TODO: fix it — one application ensures no problems with stacking permanent effects
		// NOTE: it can happen in normal play that we are leveling two classes at once, as some of the xp thresholds are shared (f/m at 250,000 xp).
		bool found = false;
		std::map<int, ClassKits>::iterator clskit = class2kits.begin();
		for (int cidx=0; clskit != class2kits.end(); clskit++, cidx++) {
			std::vector<TableMgr::index_t> kits = class2kits[cidx].indices;
			auto it = kits.begin();
			for (int kidx=0; it != kits.end(); it++, kidx++) {
				if (row == *it) {
					kitclass = cidx;
					clab = class2kits[cidx].clabs[kidx];
					found = true;
					clskit = --class2kits.end(); // break out of the outer loop too
					break;
				}
			}
		}
		if (!found) {
			Log(ERROR, "Actor", "ApplyKit: could not look up the requested kit ({}), skipping!", kit);
			return false;
		}
	}

	// a negative level diff happens when dualclassing due to three level stats being used and switched around
	if (diff < 0) diff = 0;

	//multi class
	if (multiclass) {
		ieDword msk = 1;
		for(unsigned int i=1;(i<(unsigned int) classcount) && (msk<=multiclass);i++) {
			if (multiclass & msk) {
				max = GetLevelInClass(i);
				// don't apply/remove the old kit clab if the kit is disabled
				if (i == kitclass && !IsKitInactive()) {
					// in case of dc reactivation, we already removed the clabs on activation of new class
					// so we shouldn't do it again as some of the effects could be permanent (oozemaster)
					if (IsDualClassed()) {
						ApplyClab(clab, max, 2, 0);
					} else {
						ApplyClab(clab, max, remove, diff);
					}
				} else {
					ApplyClab(class2kits[i].clab, max, remove, diff);
				}
			}
			msk+=msk;
		}
		return true;
	}
	//single class
	if (cls>=(ieDword) classcount) {
		return false;
	}
	max = GetLevelInClass(cls);
	// iwd2 has clabs for kits and classes in the same table
	if (kitclass==cls || iwd2class) {
		ApplyClab(clab, max, remove, diff);
	} else {
		ApplyClab(class2kits[cls].clab, max, remove, diff);
	}
	return true;
}

void Actor::ApplyClab(const ResRef& clab, ieDword max, int remove, int diff)
{
	if (clab && !IsStar(clab) && max) {
		// singleclass
		if (remove != 2) {
			ApplyClab_internal(this, clab, max, true, diff);
		}
		if (remove != 1) {
			ApplyClab_internal(this, clab, max, false, 0);
		}
	}
}

//call this when morale or moralebreak changed
//cannot use old or new value, because it is called two ways
static void pcf_morale (Actor *actor, ieDword /*oldValue*/, ieDword /*newValue*/)
{
	if (!actor->ShouldModifyMorale()) return;

	// no panic if we're doing something forcibly
	const Game* game = core->GetGame();
	bool overriding = actor->GetCurrentAction() && actor->GetCurrentAction()->flags & ACF_OVERRIDE;
	overriding = overriding || (game->StateOverrideFlag && game->StateOverrideTime);
	bool lowMorale = actor->Modified[IE_MORALE] <= actor->Modified[IE_MORALEBREAK];
	if (lowMorale && actor->Modified[IE_MORALEBREAK] != 0 && !overriding) {
		int panicMode = RAND(0, 2); // PANIC_RANDOMWALK etc.
		displaymsg->DisplayConstantStringName(HCStrings(int(HCStrings::MoraleBerserk) + panicMode), GUIColors::WHITE, actor);
		actor->Panic(game->GetActorByGlobalID(actor->objects.LastAttacker), panicMode + 1);
	} else if (actor->Modified[IE_STATE_ID]&STATE_PANIC) {
		// recover from panic, since morale has risen again
		// but only if we have really just recovered, so panic from other
		// sources isn't affected
		if ((actor->Modified[IE_MORALE]-1 == actor->Modified[IE_MORALEBREAK]) || (actor->Modified[IE_MORALEBREAK] == 0) ) {
			if (!third || !(actor->Modified[IE_SPECFLAGS]&SPECF_DRIVEN)) {
				actor->SetBaseBit(IE_STATE_ID, STATE_PANIC, false);
			}
		}
	}
	//for new colour
	actor->SetCircleSize();
}

static void UpdateHappiness(Actor *actor) {
	if (!actor->InParty) return;
	if (!core->HasFeature(GFFlags::HAPPINESS)) return;

	ieWordSigned newHappiness = GetHappiness(actor, core->GetGame()->Reputation);
	if (newHappiness == actor->PCStats->Happiness) return;

	actor->PCStats->Happiness = newHappiness;
	if (!actor->Ticks) return; // skip feedback on startup

	const Effect* fx;
	static EffectRef fx_ignore_breaking_point_ref = { "IgnoreReputationBreakingPoint", -1 };
	switch (newHappiness) {
		case -80:
			actor->VerbalConstant(Verbal::Unhappy, 1, DS_QUEUE);
			break;
		case -160:
			actor->VerbalConstant(Verbal::UnhappySerious, 1, DS_QUEUE);
			break;
		case -300:
			actor->VerbalConstant(Verbal::BreakingPoint, 1, DS_QUEUE);
			fx = actor->fxqueue.HasEffect(fx_ignore_breaking_point_ref);
			if (!fx && actor != core->GetGame()->GetPC(0, false)) core->GetGame()->LeaveParty(actor);
			break;
		case 80:
			actor->VerbalConstant(Verbal::Happy, 1, DS_QUEUE);
			break;
		default: break; // case 0
	}
}

// make paladins and rangers fallen if the reputations drops enough
static void pcf_reputation(Actor *actor, ieDword oldValue, ieDword newValue)
{
	static ieDword reputationFallCutOff = 10 * gamedata->GetMiscRule("REPUTATION_FALL_CUT_OFF");
	if (oldValue == newValue) return;
	if (actor->InParty && newValue <= reputationFallCutOff) {
		int match = 0;
		if (actor->GetRangerLevel()) {
			match = 1;
		} else if (actor->GetPaladinLevel()) {
			match = 2;
		}
		if (match == 0) return;

		// check if there is an exception for this kit
		AutoTable tm = gamedata->LoadTable("fallen", true);
		if (tm) {
			ieDword kit = actor->GetStat(IE_KIT);
			if (tm->QueryFieldSigned<int>(actor->GetKitName(kit), "FALLEN") == 0) return;
		}
		if (match == 1) {
			GameScript::RemoveRangerHood(actor, nullptr);
		} else {
			GameScript::RemovePaladinHood(actor, nullptr);
		}
	}
	UpdateHappiness(actor);
}

static void pcf_berserk(Actor *actor, ieDword /*oldValue*/, ieDword /*newValue*/)
{
	//needs for new color
	actor->SetCircleSize();
}

static void pcf_ea (Actor *actor, ieDword /*oldValue*/, ieDword newValue)
{
	if (actor->Selected && (newValue>EA_GOODCUTOFF) ) {
		core->GetGame()->SelectActor(actor, false, SELECT_NORMAL);
	}
	actor->SetCircleSize();
}

//this is a good place to recalculate level up stuff
// iwd2 has separate stats and requires more data for clab application
static void pcf_level (Actor *actor, ieDword oldValue, ieDword newValue, ieDword baseClass=0)
{
	ieDword sum =
		actor->GetFighterLevel()+
		actor->GetMageLevel()+
		actor->GetThiefLevel()+
		actor->GetBarbarianLevel()+
		actor->GetBardLevel()+
		actor->GetClericLevel()+
		actor->GetDruidLevel()+
		actor->GetMonkLevel()+
		actor->GetPaladinLevel()+
		actor->GetRangerLevel()+
		actor->GetSorcererLevel();
	actor->SetBase(IE_CLASSLEVELSUM,sum);
	actor->SetupFist();
	if (newValue!=oldValue) {
		actor->ApplyKit(false, baseClass, newValue-oldValue);
	}
	actor->GotLUFeedback = false;
	if (third && actor->PCStats) {
		actor->PCStats->UpdateClassLevels(actor->ListLevels());
	}
}

static void pcf_level_fighter (Actor *actor, ieDword oldValue, ieDword newValue)
{
	pcf_level(actor, oldValue, newValue, classesiwd2[ISFIGHTER]);
}

// on load, all pcfs are ran, so we need to take care not to scramble the sorcerer type
// both values are still 0 and checking equality guards against other meddling calls
// (if it turns out to be wrong, just check against Ticks == 0)
static void pcf_level_mage (Actor *actor, ieDword oldValue, ieDword newValue)
{
	pcf_level(actor, oldValue, newValue, classesiwd2[ISMAGE]);
	if (newValue != oldValue) actor->ChangeSorcererType(classesiwd2[ISMAGE]);
}

static void pcf_level_thief (Actor *actor, ieDword oldValue, ieDword newValue)
{
	pcf_level(actor, oldValue, newValue, classesiwd2[ISTHIEF]);
}

// all but iwd2 only have 3 level stats, so shortcircuit them
static void pcf_level_barbarian (Actor *actor, ieDword oldValue, ieDword newValue)
{
	if (!third) return;
	pcf_level(actor, oldValue, newValue, classesiwd2[ISBARBARIAN]);
}

static void pcf_level_bard (Actor *actor, ieDword oldValue, ieDword newValue)
{
	if (!third) return;
	pcf_level(actor, oldValue, newValue, classesiwd2[ISBARD]);
	if (newValue != oldValue) actor->ChangeSorcererType(classesiwd2[ISBARD]);
}

static void pcf_level_cleric (Actor *actor, ieDword oldValue, ieDword newValue)
{
	if (!third) return;
	pcf_level(actor, oldValue, newValue, classesiwd2[ISCLERIC]);
	if (newValue != oldValue) actor->ChangeSorcererType(classesiwd2[ISCLERIC]);
}

static void pcf_level_druid (Actor *actor, ieDword oldValue, ieDword newValue)
{
	if (!third) return;
	pcf_level(actor, oldValue, newValue, classesiwd2[ISDRUID]);
	if (newValue != oldValue) actor->ChangeSorcererType(classesiwd2[ISDRUID]);
}

static void pcf_level_monk (Actor *actor, ieDword oldValue, ieDword newValue)
{
	if (!third) return;
	pcf_level(actor, oldValue, newValue, classesiwd2[ISMONK]);
}

static void pcf_level_paladin (Actor *actor, ieDword oldValue, ieDword newValue)
{
	if (!third) return;
	pcf_level(actor, oldValue, newValue, classesiwd2[ISPALADIN]);
	if (newValue != oldValue) actor->ChangeSorcererType(classesiwd2[ISPALADIN]);
}

static void pcf_level_ranger (Actor *actor, ieDword oldValue, ieDword newValue)
{
	if (!third) return;
	pcf_level(actor, oldValue, newValue, classesiwd2[ISRANGER]);
	if (newValue != oldValue) actor->ChangeSorcererType(classesiwd2[ISRANGER]);
}

static void pcf_level_sorcerer (Actor *actor, ieDword oldValue, ieDword newValue)
{
	if (!third) return;
	pcf_level(actor, oldValue, newValue, classesiwd2[ISSORCERER]);
	if (newValue != oldValue) actor->ChangeSorcererType(classesiwd2[ISSORCERER]);
}

static void pcf_class (Actor *actor, ieDword /*oldValue*/, ieDword newValue)
{
	//Call forced initbuttons in old style systems, and soft initbuttons
	//in case of iwd2. Maybe we need a custom quickslots flag here.
	// also ensure multiclass is set early, since GetActiveClass relies on it
	actor->ResetMC();
	actor->InitButtons(actor->GetActiveClass(), !iwd2class);
	actor->ChangeSorcererType(newValue);
}

// sets (actually ORs in) the new spellbook type as a sorcerer-style one if needed
void Actor::ChangeSorcererType (ieDword classIdx)
{
	int sorcerer = 0;
	if (classIdx <(ieDword) classcount) {
		switch (bookTypes[classIdx]) {
		case 2:
			// arcane sorcerer-style
			if (third) {
				sorcerer = 1 << iwd2SPLTypes[classIdx];
			} else {
				sorcerer = 1<<IE_SPELL_TYPE_WIZARD;
			}
			break;
		case 3:
			// divine caster with sorc. style spells
			if (third) {
				sorcerer = 1 << iwd2SPLTypes[classIdx];
			} else {
				sorcerer = 1<<IE_SPELL_TYPE_PRIEST;
			}
			break;
		case 5: sorcerer = 1<<IE_IWD2_SPELL_SHAPE; break;  //divine caster with sorc style shapes (iwd2 druid)
		default: break;
		}
	}
	spellbook.SetBookType(sorcerer);
}

static void pcf_animid(Actor *actor, ieDword /*oldValue*/, ieDword newValue)
{
	actor->SetAnimationID(newValue);
}

static const ieDword fullwhite[7]={ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT};

static const ieDword fullstone[7]={STONE_GRADIENT,STONE_GRADIENT,STONE_GRADIENT,STONE_GRADIENT,STONE_GRADIENT,STONE_GRADIENT,STONE_GRADIENT};

static void pcf_state(Actor *actor, ieDword /*oldValue*/, ieDword State)
{
	if (actor->InParty) core->SetEventFlag(EF_PORTRAIT);
	if (State & STATE_PETRIFIED) {
		actor->SetLockedPalette(fullstone);
		return;
	}
	if (State & STATE_FROZEN) {
		actor->SetLockedPalette(fullwhite);
		return;
	}
	//it is not enough to check the new state
	core->GetGame()->Infravision();
	actor->UnlockPalette();
}

//changes based on extended state bits, right now it is only the seven eyes
//animation (used in how/iwd2)
static void pcf_extstate(Actor *actor, ieDword oldValue, ieDword State)
{
	if ((oldValue^State)&EXTSTATE_SEVEN_EYES) {
		orient_t eyeCount = NNW;
		for (ieDword mask = EXTSTATE_EYE_MIND; mask <= EXTSTATE_EYE_STONE; mask <<= 1) {
			if (State & mask) eyeCount = PrevOrientation(eyeCount);
		}
		ScriptedAnimation *sca = actor->FindOverlay(OV_SEVENEYES);
		if (sca) {
			sca->SetOrientation(eyeCount);
		}
		sca = actor->FindOverlay(OV_SEVENEYES2);
		if (sca) {
			sca->SetOrientation(eyeCount);
		}
	}
}

static void pcf_hitpoint(Actor *actor, ieDword oldValue, ieDword hp)
{
	if (actor->checkHP == 2) return;
	if (actor->GetInternalFlag() & IF_REALLYDIED) return;

	int maxhp = (signed) actor->GetSafeStat(IE_MAXHITPOINTS);
	// ERWAN.CRE from Victor's Improvement Pack has a max of 0 and still survives, grrr
	if (maxhp && (signed) hp > maxhp) {
		hp=maxhp;
	}

	int minhp = (signed) actor->GetSafeStat(IE_MINHITPOINTS);
	if (minhp && (signed) hp<minhp) {
		hp=minhp;
	}
	if ((signed) hp<=0) {
		actor->Die(NULL);
	} else {
		// in testing it popped up somewhere between 39% and 25.3% (single run) -> 1/3
		if (signed(3*oldValue) > maxhp && signed(3*hp) < maxhp) {
			actor->VerbalConstant(Verbal::Hurt, gamedata->GetVBData("SPECIAL_COUNT"), DS_QUEUE);
		}
	}

	actor->BaseStats[IE_HITPOINTS] = hp;
	actor->Modified[IE_HITPOINTS] = hp;
	// don't fire off events if nothing changed, which can happen when called indirectly
	if (oldValue != hp && actor->InParty) {
		core->SetEventFlag(EF_PORTRAIT);
	}
}

static void pcf_maxhitpoint(Actor *actor, ieDword /*oldValue*/, ieDword /*newValue*/)
{
	if (!actor->checkHP) {
		actor->checkHP = 1;
		actor->checkHPTime = core->GetGame()->GameTime;
	}
}

static void pcf_minhitpoint(Actor *actor, ieDword /*oldValue*/, ieDword hp)
{
	if ((signed) hp>(signed) actor->BaseStats[IE_HITPOINTS]) {
		actor->BaseStats[IE_HITPOINTS]=hp;
		//passing 0 because it is ignored anyway
		pcf_hitpoint(actor, 0, hp);
	}
}

static void pcf_stat(Actor *actor, ieDword newValue, ieDword stat)
{
	if ((signed) newValue<=0) {
		if (DeathOnZeroStat && !actor->fxqueue.HasEffectWithParam(fx_minimum_base_stats_ref, 1)) {
			actor->Die(NULL);
		} else {
			actor->Modified[stat]=1;
		}
	}
}

static void pcf_stat_str(Actor *actor, ieDword /*oldValue*/, ieDword newValue)
{
	pcf_stat(actor, newValue, IE_STR);
}

static void pcf_stat_int(Actor *actor, ieDword /*oldValue*/, ieDword newValue)
{
	pcf_stat(actor, newValue, IE_INT);
}

static void pcf_stat_wis(Actor *actor, ieDword oldValue, ieDword newValue)
{
	pcf_stat(actor, newValue, IE_WIS);
	if (third) {
		int oldBonus = actor->GetAbilityBonus(IE_WIS, oldValue);
		actor->Modified[IE_SAVEWILL] += actor->GetAbilityBonus(IE_WIS) - oldBonus;
	}
}

static void pcf_stat_dex(Actor *actor, ieDword oldValue, ieDword newValue)
{
	pcf_stat(actor, newValue, IE_DEX);
	if (third) {
		int oldBonus = actor->GetAbilityBonus(IE_DEX, oldValue);
		actor->Modified[IE_SAVEREFLEX] += actor->GetAbilityBonus(IE_DEX) - oldBonus;
	}
}

static void pcf_stat_con(Actor *actor, ieDword oldValue, ieDword newValue)
{
	pcf_stat(actor, newValue, IE_CON);
	if (!actor->checkHP) {
		pcf_hitpoint(actor, 0, actor->BaseStats[IE_HITPOINTS]);
	}
	if (third) {
		int oldBonus = actor->GetAbilityBonus(IE_CON, oldValue);
		actor->Modified[IE_SAVEFORTITUDE] += actor->GetAbilityBonus(IE_CON) - oldBonus;
	}
}

static void pcf_stat_cha(Actor *actor, ieDword /*oldValue*/, ieDword newValue)
{
	pcf_stat(actor, newValue, IE_CHR);
}

static void pcf_xp(Actor *actor, ieDword /*oldValue*/, ieDword /*newValue*/)
{
	// check if we reached a new level
	ieByte pc = actor->InParty;
	if (pc && !actor->GotLUFeedback) {
		const std::string& varName = fmt::format("CheckLevelUp{}", pc);
		ScriptEngine::FunctionParameters params;
		params.push_back(ScriptEngine::Parameter(pc));
		core->GetGUIScriptEngine()->RunFunction("GUICommonWindows", "CheckLevelUp", params, true);
		ieDword NeedsLevelUp = core->GetVariable(varName, 0);
		if (NeedsLevelUp == 1) {
			displaymsg->DisplayConstantStringName(HCStrings::LevelUp, GUIColors::WHITE, actor);
			actor->GotLUFeedback = true;
			core->SetEventFlag(EF_PORTRAIT);
		}
	}
}

static void pcf_gold(Actor *actor, ieDword /*oldValue*/, ieDword /*newValue*/)
{
	//this function will make a party member automatically donate their
	//gold to the party pool, not the same as in the original engine
	if (actor->InParty) {
		Game *game = core->GetGame();
		game->AddGold ( actor->BaseStats[IE_GOLD] );
		actor->BaseStats[IE_GOLD]=0;
	}
}

static void handle_overlay(Actor *actor, ieDword idx)
{
	if (idx >= OVERLAY_COUNT || actor->FindOverlay(idx)) return;

	ResRef overlayGfx = hc_overlays[idx];
	// ee allows overriding some overlay graphics directly via their effects
	static EffectRef fx_overlay_sanctuary_ref = { "Overlay:Sanctuary", -1 };
	static EffectRef fx_overlay_entangle_ref = { "Overlay:Entangle", -1 };
	static EffectRef fx_overlay_minorglobe_ref = { "Overlay:MinorGlobe", -1 };
	static EffectRef fx_overlay_shieldglobe_ref = { "Overlay:ShieldGlobe", -1 };
	static EffectRef fx_overlay_web_ref = { "Overlay:Web", -1 };
	static EffectRef fx_overlay_grease_ref = { "Overlay:Grease", -1 };
	static std::map<int, EffectRef> overlayRefs = { { OV_SANCTUARY, fx_overlay_sanctuary_ref }, { OV_ENTANGLE, fx_overlay_entangle_ref }, { OV_MINORGLOBE, fx_overlay_minorglobe_ref }, { OV_SHIELDGLOBE, fx_overlay_shieldglobe_ref }, { OV_WEB, fx_overlay_web_ref }, { OV_GREASE, fx_overlay_grease_ref } };
	if (idx <= OV_MINORGLOBE && idx != OV_SPELLTRAP) {
		const Effect* fx = actor->fxqueue.HasEffectWithParam(overlayRefs[idx], 1);
		if (fx && !fx->Resource.IsEmpty()) {
			overlayGfx = fx->Resource;
		}
	}
	ScriptedAnimation* sca = gamedata->GetScriptedAnimation(overlayGfx, false);

	if (!sca) {
		return;
	}
	// many are stored as bams and can't be translucent by default
	sca->SetBlend();

	// always draw it for party members; the rest must not be invisible to have it;
	// this is just a guess, maybe there are extra conditions (MC_HIDDEN? IE_AVATARREMOVAL?)
	if (!actor->InParty && actor->Modified[IE_STATE_ID] & state_invisible && !(hc_flags[idx] & HC_INVISIBLE)) {
		delete sca;
		return;
	}

	ieDword flag = hc_locations & (1 << idx);
	if (flag) {
		sca->ZOffset = -1;
	}
	actor->AddVVCell(sca);
}

//de/activates the entangle overlay
static void pcf_entangle(Actor *actor, ieDword oldValue, ieDword newValue)
{
	if (newValue&1) {
		handle_overlay(actor, OV_ENTANGLE);
	}
	if (oldValue&1) {
		actor->RemoveVVCells(hc_overlays[OV_ENTANGLE]);
	}
}

//de/activates the sanctuary and other overlays
//unlike IE, gemrb uses this stat for other overlay fields
//see the complete list in overlay.2da
//it loosely follows the internal representation of overlays in IWD2
static void pcf_sanctuary(Actor *actor, ieDword oldValue, ieDword newValue)
{
	ieDword changed = newValue^oldValue;
	ieDword mask = 1;
	if (!changed) return;
	for (int i=0; i<OVERLAY_COUNT; i++) {
		if (changed&mask) {
			if (newValue&mask) {
				handle_overlay(actor, i);
			} else if (oldValue&mask) {
				actor->RemoveVVCells(hc_overlays[i]);
			}
		}
		mask<<=1;
	}
}

//de/activates the prot from missiles overlay
static void pcf_shieldglobe(Actor *actor, ieDword oldValue, ieDword newValue)
{
	if (newValue&1) {
		handle_overlay(actor, OV_SHIELDGLOBE);
		return;
	}
	if (oldValue&1) {
		actor->RemoveVVCells(hc_overlays[OV_SHIELDGLOBE]);
	}
}

//de/activates the globe of invul. overlay
static void pcf_minorglobe(Actor *actor, ieDword oldValue, ieDword newValue)
{
	if (newValue&1) {
		handle_overlay(actor, OV_MINORGLOBE);
		return;
	}
	if (oldValue&1) {
		actor->RemoveVVCells(hc_overlays[OV_MINORGLOBE]);
	}
}

//de/activates the grease background
static void pcf_grease(Actor *actor, ieDword oldValue, ieDword newValue)
{
	if (newValue&1) {
		handle_overlay(actor, OV_GREASE);
		return;
	}
	if (oldValue&1) {
		actor->RemoveVVCells(hc_overlays[OV_GREASE]);
	}
}

//de/activates the web overlay
//the web effect also immobilizes the actor!
static void pcf_web(Actor *actor, ieDword oldValue, ieDword newValue)
{
	if (newValue&1) {
		handle_overlay(actor, OV_WEB);
		return;
	}
	if (oldValue&1) {
		actor->RemoveVVCells(hc_overlays[OV_WEB]);
	}
}

//de/activates the spell bounce background
static void pcf_bounce(Actor *actor, ieDword oldValue, ieDword newValue)
{
	if (newValue) {
		handle_overlay(actor, OV_BOUNCE);
		return;
	}
	if (oldValue) {
		actor->RemoveVVCells(hc_overlays[OV_BOUNCE]);
	}
}

static void pcf_alignment(Actor *actor, ieDword /*oldValue*/, ieDword /*newValue*/)
{
	UpdateHappiness(actor);
}

static void pcf_avatarremoval(Actor *actor, ieDword /*oldValue*/, ieDword newValue)
{
	const Map *map = actor->GetCurrentArea();
	if (!map) return;
	
	if (newValue) {
		map->ClearSearchMapFor(actor);
	} else {
		map->BlockSearchMapFor(actor);
	}
}

//spell casting or other buttons disabled/reenabled
static void pcf_dbutton(Actor *actor, ieDword /*oldValue*/, ieDword /*newValue*/)
{
	if (actor->IsSelected()) {
		core->SetEventFlag( EF_ACTION );
	}
}

//no separate values (changes are permanent)
static void pcf_intoxication(Actor *actor, ieDword /*oldValue*/, ieDword newValue)
{
	actor->BaseStats[IE_INTOXICATION]=newValue;
}

static void pcf_color(Actor *actor, ieDword /*oldValue*/, ieDword /*newValue*/)
{
	CharAnimations *anims = actor->GetAnims();
	if (anims) {
		anims->SetColors(&actor->Modified[IE_COLORS]);
	}
}

static void pcf_armorlevel(Actor *actor, ieDword /*oldValue*/, ieDword newValue)
{
	CharAnimations *anims = actor->GetAnims();
	if (anims) {
		anims->SetArmourLevel(newValue);
	}
}

static Actor::stats_t maximum_values = {
32767,32767,20,100,100,100,100,25,10,25,25,25,25,25,200,200,//0f
200,200,200,200,200,100,100,100,100,100,255,255,255,255,100,100,//1f
200,200,MAX_LEVEL,255,25,100,25,25,25,25,25,999999999,999999999,999999999,25,25,//2f
200,255,200,100,100,200,200,25,10,100,1,1,255,1,1,0,//3f
1023,1,1,1,MAX_LEVEL,MAX_LEVEL,1,9999,25,200,200,255,1,20,20,25,//4f
25,1,1,255,25,25,255,255,25,255,255,255,255,255,255,255,//5f
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,//6f
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,//7f
255,255,255,MAX_FEATV,MAX_FEATV,MAX_FEATV,255,100,100,100,999999,5,5,999999,1,1,//8f
1,25,25,255,1,1,1,25,0,100,100,1,255,255,255,255,//9f
255,255,255,255,255,255,20,255,255,1,20,255,999999999,999999999,1,1,//af
999999999,999999999,0,0,20,0,0,0,0,0,0,0,0,0,0,0,//bf
0,0,0,0,0,0,0,25,25,255,255,255,255,65535,0,0,//cf - 207
0,0,0,0,0,0,0,0,MAX_LEVEL,255,65535,3,255,255,255,255,//df - 223
255,255,255,255,255,255,255,255,255,255,255,255,65535,65535,15,0,//ef - 239
MAX_LEVEL,MAX_LEVEL,MAX_LEVEL,MAX_LEVEL, MAX_LEVEL,MAX_LEVEL,MAX_LEVEL,MAX_LEVEL, //0xf7 - 247
MAX_LEVEL,MAX_LEVEL,0,0,0,0,0,0//ff
};

using PostChangeFunctionType = void (*)(Actor *actor, ieDword oldValue, ieDword newValue);
static const PostChangeFunctionType post_change_functions[MAX_STATS] = {
	pcf_hitpoint, pcf_maxhitpoint, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, //0f
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, pcf_intoxication, //1f
	nullptr, nullptr, pcf_level_fighter, nullptr, pcf_stat_str, nullptr, pcf_stat_int, pcf_stat_wis,
	pcf_stat_dex, pcf_stat_con, pcf_stat_cha, nullptr, pcf_xp, pcf_gold, pcf_morale, nullptr, //2f
	pcf_reputation, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, pcf_entangle, pcf_sanctuary, //3f
	pcf_minorglobe, pcf_shieldglobe, pcf_grease, pcf_web, pcf_level_mage, pcf_level_thief, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, //4f
	nullptr, nullptr, nullptr, pcf_minhitpoint, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, //5f
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, //6f
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, //7f
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, pcf_berserk, nullptr, //8f
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, //9f
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, //af
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, //bf
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, pcf_avatarremoval, nullptr, nullptr, pcf_dbutton, pcf_animid, pcf_state, pcf_extstate, //cf
	pcf_color, pcf_color, pcf_color, pcf_color, pcf_color, pcf_color, pcf_color, nullptr,
	nullptr, pcf_alignment, pcf_dbutton, pcf_armorlevel, nullptr, nullptr, nullptr, nullptr, //df
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	pcf_class, nullptr, pcf_ea, nullptr, nullptr, nullptr, nullptr, nullptr, //ef
	pcf_level_barbarian, pcf_level_bard, pcf_level_cleric, pcf_level_druid, pcf_level_monk, pcf_level_paladin, pcf_level_ranger, pcf_level_sorcerer,
	nullptr, nullptr, nullptr, nullptr, pcf_morale, pcf_bounce, nullptr, nullptr //ff
};

#define COL_MAIN       0
#define COL_SPARKS     1
#define COL_GRADIENT   2

/* returns the ISCLASS for the class based on name */
static int IsClassFromName (const std::string& name)
{
	for (int i=0; i<ISCLASSES; i++) {
		if (name == isclassnames[i])
			return i;
	}
	return -1;
}

GEM_EXPORT void UpdateActorConfig()
{
	crit_hit_scr_shake = core->GetVariable("Critical Hit Screen Shake", 1);

	unsigned int effectTextLevel = core->GetVariable("Effect Text Level", 0);
	core->SetFeedbackLevel(effectTextLevel);

	sel_snd_freq = core->GetVariable("Selection Sounds Frequency", 0);
	cmd_snd_freq = core->GetVariable("Command Sounds Frequency", 0);
	// only pst has the whole gamut for these two options
	if (!(effectTextLevel & FT_SELECTION)) sel_snd_freq = 0;
	if (!(effectTextLevel & FT_ACTIONS)) cmd_snd_freq = 0;

	bored_time = core->GetVariable("Bored Timeout", 3000);
	footsteps = core->GetVariable("Footsteps", 1);
	war_cries = core->GetVariable("Attack Sounds", 1);
	PreferSneakAttack = core->GetVariable("3E Thief Sneak Attack", 0);

	//Handle Game Difficulty and Nightmare Mode
	// iwd2 had it saved in the GAM, iwd1 only relied on the ini value
	GameDifficulty = core->GetVariable("Nightmare Mode", 0);

	auto& vars = core->GetDictionary();
	Game *game = core->GetGame();
	if (GameDifficulty || (game && game->HOFMode)) {
		GameDifficulty = DIFF_INSANE;
		if (game) game->HOFMode = true;
		// also set it for GUIOPT
		vars["Difficulty Level"] = DIFF_INSANE - 1;
	} else {
		GameDifficulty = core->GetVariable("Difficulty Level", 0);
		GameDifficulty++; // slider starts at 0, real levels at 1
	}
	ieDword newMode = core->GetVariable("Story Mode", 0);
	if (newMode != StoryMode) {
		if (newMode) {
			GameDifficulty = DIFF_EASY;
			vars["Difficulty Level"] = DIFF_EASY - 1U;

			// add all the immunities and bonuses to party
			for (int i = 0; game && i < game->GetPartySize(false); i++) {
				Actor* pc = game->GetPC(i, false);
				core->ApplySpell("OHSMODE1", pc, pc, 0);
			}
		} else {
			// or remove them; they target OHSMODE1, so this is idempotent
			for (int i = 0; game && i < game->GetPartySize(false); i++) {
				Actor* pc = game->GetPC(i, false);
				core->ApplySpell("OHSMODE2", pc, pc, 0);
			}
		}
		StoryMode = newMode;
	}
	GameDifficulty = Clamp((int) GameDifficulty, DIFF_EASY, DIFF_INSANE);
	// cache hot path mods
	DifficultyLuckMod = gamedata->GetDifficultyMod(2, GameDifficulty);
	DifficultyDamageMod = gamedata->GetDifficultyMod(0, GameDifficulty);
	DifficultySaveMod = gamedata->GetDifficultyMod(3, GameDifficulty);

	// iwd has a config option for leniency
	NoExtraDifficultyDmg = core->GetVariable("Suppress Extra Difficulty Damage", 0);
}

static void ReadModalStates()
{
	AutoTable table = gamedata->LoadTable("modal");
	if (!table) return;

	if (table->GetRowCount() > (size_t) Modal::count) {
		Log(ERROR, "Actor", "Found new modal state in data! Nothing will happen.");
	}

	for (unsigned short i = 0; i < (size_t) Modal::count; i++) {
		ModalStates[i].spell = table->QueryField(i, 0);
		ModalStates[i].action = table->QueryField(i, 1);
		ModalStates[i].entering_str = table->QueryFieldAsStrRef(i, 2);
		ModalStates[i].leaving_str = table->QueryFieldAsStrRef(i, 3);
		ModalStates[i].failed_str = table->QueryFieldAsStrRef(i, 4);
		ModalStates[i].aoe_spell = table->QueryFieldUnsigned<unsigned int>(i, 5);
		ModalStates[i].repeat_msg = table->QueryFieldUnsigned<unsigned int>(i, 6);
	}
}

static void InitActorTables()
{
	UpdateActorConfig();
	pstflags = core->HasFeature(GFFlags::PST_STATE_FLAGS) != 0;
	nocreate = core->HasFeature(GFFlags::NO_NEW_VARIABLES) != 0;
	third = core->HasFeature(GFFlags::RULES_3ED) != 0;
	iwd2class = core->HasFeature(GFFlags::LEVELSLOT_PER_CLASS) != 0;
	// iwd2 has some different base class names
	if (iwd2class) {
		isclassnames[ISTHIEF] = "ROGUE";
		isclassnames[ISMAGE] = "WIZARD";
	}

	if (pstflags) {
		state_invisible=STATE_PST_INVIS;
	} else {
		state_invisible=STATE_INVISIBLE;
	}

	if (core->HasFeature(GFFlags::CHALLENGERATING)) {
		sharexp=SX_DIVIDE|SX_COMBAT|SX_CR;
	} else {
		sharexp=SX_DIVIDE|SX_COMBAT;
	}
	ReverseToHit = core->HasFeature(GFFlags::REVERSE_TOHIT);
	CheckAbilities = core->HasFeature(GFFlags::CHECK_ABILITIES);
	DeathOnZeroStat = core->HasFeature(GFFlags::DEATH_ON_ZERO_STAT);
	IWDSound = core->HasFeature(GFFlags::SOUNDS_INI);

	//this table lists skill groups assigned to classes
	//it is theoretically possible to create hybrid classes
	AutoTable tm = gamedata->LoadTable("clskills");
	if (tm) {
		classcount = tm->GetRowCount();
		noProfPenalty.resize(classcount);
		turnLevelOffset.resize(classcount);
		bookTypes.resize(classcount);
		castingStat.resize(classcount);
		iwd2SPLTypes.resize(classcount);

		ieDword bitmask = 1;

		for (int i = 0;  i < classcount; i++) {
			const char *field;
			const auto& rowname = tm->GetRowName(i);

			field = tm->QueryField(rowname, "DRUIDSPELL").c_str();
			if (field[0]!='*') {
				isclass[ISDRUID] |= bitmask;
			}
			field = tm->QueryField(rowname, "CLERICSPELL").c_str();
			if (field[0]!='*') {
				// iwd2 has no DRUIDSPELL
				if (third && !strnicmp(field, "MXSPLDRD", 8)) {
					isclass[ISDRUID] |= bitmask;
				} else {
					isclass[ISCLERIC] |= bitmask;
				}
			}

			field = tm->QueryField(rowname, "MAGESPELL").c_str();
			if (field[0]!='*') {
				isclass[ISMAGE] |= bitmask;
			}

			// field 3 holds the starting xp

			field = tm->QueryField(rowname, "BARDSKILL").c_str();
			if (field[0]!='*') {
				isclass[ISBARD] |= bitmask;
			}

			field = tm->QueryField(rowname, "THIEFSKILL").c_str();
			if (field[0]!='*') {
				isclass[ISTHIEF] |= bitmask;
			}

			field = tm->QueryField(rowname, "LAYHANDS").c_str();
			if (field[0]!='*') {
				isclass[ISPALADIN] |= bitmask;
			}

			turnLevelOffset[i] = tm->QueryFieldSigned<int>(rowname, "TURNLEVEL");
			bookTypes[i] = tm->QueryFieldSigned<int>(rowname, "BOOKTYPE");
			//if booktype == 3 then it is a 'divine sorcerer' class
			//we shouldn't hardcode iwd2 classes this heavily
			if (bookTypes[i] == 2) {
				isclass[ISSORCERER] |= bitmask;
			}

			if (third) {
				castingStat[i] = tm->QueryFieldSigned<int>(rowname, "CASTING"); // HATERACE column in other games
				iwd2SPLTypes[i] = tm->QueryFieldSigned<int>(rowname, "SPLTYPE");
			}

			field = tm->QueryField(rowname, "HATERACE").c_str();
			if (field[0]!='*') {
				isclass[ISRANGER] |= bitmask;
			}

			field = tm->QueryField(rowname, "ABILITIES").c_str();
			if (!strnicmp(field, "CLABMO", 6)) {
				isclass[ISMONK] |= bitmask;
			}
			// everyone but pst (none at all) and iwd2 (different table)
			class2kits[i].clab = ResRef(field);
			class2kits[i].className = rowname;

			noProfPenalty[i] = tm->QueryFieldSigned<int>(rowname, "NO_PROF");

			bitmask <<=1;
		}
	} else {
		classcount = 0; //well
	}

	int abilityMax = core->GetMaximumAbility();
	maximum_values[IE_STR] = abilityMax;
	maximum_values[IE_INT] = abilityMax;
	maximum_values[IE_DEX] = abilityMax;
	maximum_values[IE_CON] = abilityMax;
	maximum_values[IE_CHR] = abilityMax;
	maximum_values[IE_WIS] = abilityMax;
	if (ReverseToHit) {
		//all games except iwd2
		maximum_values[IE_ARMORCLASS]=20;
	} else {
		//iwd2
		maximum_values[IE_ARMORCLASS]=199;
	}

	//initializing the vvc resource references
	tm = gamedata->LoadTable("damage");
	if (tm) {
		for (int i = 0; i < DAMAGE_LEVELS; i++) {
			ResRef tmp = tm->QueryField( i, COL_MAIN );
			d_main[i] = tmp;
			if (IsStar(d_main[i])) {
				d_main[i].Reset();
			}
			tmp = tm->QueryField(i, COL_SPARKS);
			d_splash[i] = tmp;
			if (IsStar(d_splash[i])) {
				d_splash[i].Reset();
			}
			d_gradient[i] = tm->QueryFieldSigned<int>(i, COL_GRADIENT);
		}
	}

	tm = gamedata->LoadTable("overlay");
	if (tm) {
		ieDword mask = 1;
		for (int i =  0; i < OVERLAY_COUNT; i++) {
			hc_overlays[i] = tm->QueryField(i, 0);
			if (tm->QueryFieldSigned<int>( i, 1)) {
				hc_locations|=mask;
			}
			hc_flags[i] = tm->QueryFieldSigned<int>(i, 2);
			mask<<=1;
		}
	}

	//csound for bg1/bg2
	if (!core->HasFeature(GFFlags::SOUNDFOLDERS)) {
		tm = gamedata->LoadTable("csound");
		if (tm) {
			for (auto i : EnumIterator<Verbal>()) {
				const auto& suffix = tm->QueryField(UnderType(i), 0);
				switch(suffix[0]) {
					case '*': break;
					//I have no idea what this ! mean
					case '!': csound[i] = suffix[1]; break;
					default: csound[i] = suffix[0]; break;
				}
			}
		}
	}

	tm = gamedata->LoadTable("qslots");
	GUIBTDefaults.resize(classcount + 1);

	//leave room for default row at 0
	for (int i = 0; i <= classcount; i++) {
		GUIBTDefaults[i] = DefaultButtons;
		if (tm && i) {
			for (int j = 0; j < MAX_QSLOTS; j++) {
				GUIBTDefaults[i][j + 3] = tm->QueryFieldUnsigned<ieByte>(i - 1, j);
			}
		}
	}

	tm = gamedata->LoadTable("qslot2", true);
	if (tm) {
		TableMgr::index_t extraSlots = tm->GetRowCount();
		OtherGUIButtons.resize(extraSlots);

		for (TableMgr::index_t i = 0; i < extraSlots; i++) {
			ieByte cls = 0;
			valid_unsignednumber(tm->QueryField(i, 0).c_str(), cls);
			OtherGUIButtons[i].clss = cls;
			OtherGUIButtons[i].buttons = DefaultButtons;
			for (int j = 0; j < GUIBT_COUNT; j++) {
				OtherGUIButtons[i].buttons[j] = tm->QueryFieldUnsigned<ieByte>(i, j + 1);
			}
		}
	}

	tm = gamedata->LoadTable("mdfeats", true);
	if (tm) {
		for (int i = 0; i < ES_COUNT; i++) {
			featSpells[i] = tm->QueryField(i, 0);
		}
	}

	tm = gamedata->LoadTable("featreq", true);
	if (tm) {
		unsigned int stat, max;

		for (int i = 0; i < MAX_FEATS; i++) {
			//we need the MULTIPLE and MAX_LEVEL columns
			//MULTIPLE: the FEAT_* stat index
			//MAX_LEVEL: how many times it could be taken
			stat = core->TranslateStat(tm->QueryField(i, 0));
			if (stat>=MAX_STATS) {
				Log(WARNING, "Actor", "Invalid stat value in featreq.2da");
			}
			max = tm->QueryFieldUnsigned<unsigned int>(i,1);
			//boolean feats can only be taken once, the code requires featmax for them too
			if (stat && (max<1)) max=1;
			featstats[i] = (ieByte) stat;
			featmax[i] = (ieByte) max;
		}
	}

	if (classcount) maxLevelForHpRoll.resize(classcount);
	xpCap.resize(classcount);
	AutoTable xpcapt = gamedata->LoadTable("xpcap");
	std::map<std::string, int> className2ID;

	tm = gamedata->LoadTable("classes");
	if (!tm) {
		error("Actor", "Missing classes.2da!");
	}
	if (iwd2class) {
		// we need to set up much less here due to a saner class/level system in 3ed
		Log(MESSAGE, "Actor", "Examining IWD2-style classes.2da");
		AutoTable tht;
		for (int i = 0; i < (int) tm->GetRowCount(); i++) {
			const auto& classname = tm->GetRowName(i);
			int classis = IsClassFromName(classname);
			ieDword classID = tm->QueryFieldUnsigned<ieDword>(classname, "ID");
			ieDword classcol = tm->QueryFieldUnsigned<ieDword>(classname, "CLASS"); // only real classes have this column at 0
			ResRef clab = tm->QueryField(classname, "CLAB");
			if (classcol) {
				// kit ids are in hex
				classID = strtounsigned<ieDword>(tm->QueryField(classname, "ID").c_str(), nullptr, 16);
				class2kits[classcol].indices.push_back(i);
				class2kits[classcol].ids.push_back(classID);
				class2kits[classcol].clabs.emplace_back(clab);
				class2kits[classcol].kitNames.push_back(classname);
				continue;
			} else if (i < classcount) {
				// populate classesiwd2
				// we need the id of the isclass name, not the current one
				ieDword cid = tm->QueryFieldUnsigned<ieDword>(isclassnames[i], "ID");
				classesiwd2[i] = cid;

				class2kits[classID].clab = clab;
			} else {
				// new class out of order
				Log(FATAL, "Actor", "New classes should precede any kits in classes.2da! Aborting ...");
			}

			assert(classis != -1);
			xpCap[classis] = xpcapt->QueryFieldSigned<int>(classname, "VALUE");

			// set up the tohit/apr tables
			const ResRef tohit = tm->QueryField(classname, "TOHIT");
			BABClassMap[classis] = tohit;
			// the tables repeat, but we need to only load one copy
			// FIXME: the attempt at skipping doesn't work!
			const auto& it = IWD2HitTable.find(tohit);
			if (it == IWD2HitTable.end()) {
				tht = gamedata->LoadTable(tohit, true);
				if (!tht || !tohit[0]) {
					error("Actor", "TOHIT table for {} does not exist!", classname);
				}

				BABTable bt;
				std::vector<BABTable> btv;
				btv.reserve(tht->GetRowCount());
				for (TableMgr::index_t row = 0; row < tht->GetRowCount(); ++row) {
					bt.level = atoi(tht->GetRowName(row).c_str());
					bt.bab = tht->QueryFieldSigned<int>(row, 0);
					bt.apr = tht->QueryFieldSigned<int>(row, 1);
					btv.push_back(bt);
				}
				IWD2HitTable.emplace(BABClassMap[classis], btv);
			}

			std::string buffer;
			AppendFormat(buffer, "\tID: {}, ", classID);
			AppendFormat(buffer, "Name: {}, ", classname);
			AppendFormat(buffer, "Classis: {}, ", classis);
			AppendFormat(buffer, "ToHit: {} ", tohit);
			AppendFormat(buffer, "XPCap: {}", xpCap[classis]);

			Log(DEBUG, "Actor", "{}", buffer);
		}
	} else {
		AutoTable hptm;

		Log(MESSAGE, "Actor", "Examining classes.2da");
		// iwd2 just uses levelslotsiwd2 instead
		levelStats.resize(classcount);
		dualSwap.resize(classcount);
		multiclassIDs.resize(classcount);
		ieDword tmpindex;

		for (int i = 0; i < classcount; i++) {
			const std::string& classname = tm->GetRowName(i);
			//make sure we have a valid classid, then decrement
			//it to get the correct array index
			tmpindex = tm->QueryFieldUnsigned<ieDword>(classname, "ID");
			if (!tmpindex)
				continue;
			className2ID[classname] = tmpindex;
			tmpindex--;

			std::string buffer;
			AppendFormat(buffer, "\tID: {} ", tmpindex);
			//only create the array if it isn't yet made
			//i.e. barbarians would overwrite fighters in bg2
			if (!levelStats[tmpindex].empty()) {
				AppendFormat(buffer, "Already Found!");
				Log(DEBUG, "Actor", "{}", buffer);
				continue;
			}

			AppendFormat(buffer, "Name: {} ", classname);

			xpCap[tmpindex] = xpcapt->QueryFieldSigned<int>(classname, "VALUE");
			AppendFormat(buffer, "XPCAP: {} ", xpCap[tmpindex]);

			int classis = 0;
			// default all levelStats to 0
			levelStats[tmpindex].resize(ISCLASSES);

			//single classes only worry about IE_LEVEL
			ieDword tmpclass = 0;
			valid_unsignednumber(tm->QueryField(classname, "MULTI").c_str(), tmpclass);
			multiclassIDs[tmpindex] = tmpclass;
			if (!tmpclass) {
				classis = IsClassFromName(classname);
				if (classis>=0) {
					//store the original class ID as iwd2 compatible ISCLASS (internal class number)
					classesiwd2[classis] = tmpindex+1;

					AppendFormat(buffer, "Classis: {} ", classis);
					levelStats[tmpindex][classis] = IE_LEVEL;
					// get the last level when we can roll for HP / get a constitution bonus
					int conBonLevel = tm->QueryFieldSigned<int>(classname, "CONBONLVL");
					AppendFormat(buffer, "HPROLLMAXLVL: {}", conBonLevel);
					if (conBonLevel) maxLevelForHpRoll[tmpindex] = conBonLevel;
				}
				Log(DEBUG, "Actor", "{}", buffer);
				continue;
			}

			size_t tmpbits = CountBits (tmpclass);

			//we need all the classnames of the multi to compare with the order we load them in
			//because the original game set the levels based on name order, not bit order
			auto classnames = Explode<std::string, std::string>(classname, '_', tmpbits);
			bool foundwarrior = false;
			//we have to account for dual-swap in the multiclass field
			size_t numfound = 0;
			for (int j = 0; j < classcount; j++) {
				//no sense continuing if we've found all to be found
				if (numfound==tmpbits)
					break;
				if ((1<<j)&tmpclass) {
					//save the IE_LEVEL information
					const std::string& currentname = tm->GetRowName(tm->FindTableValue("ID", j + 1));
					classis = IsClassFromName(currentname);
					if (classis>=0) {
						//search for the current class in the split of the names to get it's
						//correct order
						for (ieDword k=0; k<tmpbits; k++) {
							if (currentname.compare(classnames[k].c_str()) == 0) {
								int tmplevel = 0;
								if (k==0) tmplevel = IE_LEVEL;
								else if (k==1) tmplevel = IE_LEVEL2;
								else tmplevel = IE_LEVEL3;
								levelStats[tmpindex][classis] = tmplevel;
							}
						}
						AppendFormat(buffer, "Classis: {} ", classis);

						//warrior take precedence
						if (!foundwarrior) {
							foundwarrior = (classis==ISFIGHTER||classis==ISRANGER||classis==ISPALADIN||
								classis==ISBARBARIAN);
							int conBonLevel = tm->QueryFieldSigned<int>(classname, "CONBONLVL");
							if ((conBonLevel > maxLevelForHpRoll[tmpindex]) || foundwarrior || numfound == 0) {
								maxLevelForHpRoll[tmpindex] = conBonLevel;
							}
						}
					}

					//save the MC_WAS_ID of the first class in the dual-class
					if (numfound==0 && tmpbits==2) {
						if (currentname.compare(classnames[0].c_str()) == 0) {
							dualSwap[tmpindex] = tm->QueryFieldSigned<int>(currentname, "MC_WAS_ID");
						}
					} else if (numfound == 1 && tmpbits == 2 && !dualSwap[tmpindex]) {
						dualSwap[tmpindex] = tm->QueryFieldSigned<int>(currentname, "MC_WAS_ID");
					}
					numfound++;
				}
			}

			AppendFormat(buffer, "HPROLLMAXLVL: {} ", maxLevelForHpRoll[tmpindex]);
			AppendFormat(buffer, "DS: {} ", dualSwap[tmpindex]);
			AppendFormat(buffer, "MULTI: {}", multiclassIDs[tmpindex]);
			Log(DEBUG, "Actor", "{}", buffer);
		}
	}
	Log(MESSAGE, "Actor", "Finished examining classes.2da");

	// set the default weapon slot count for the inventory gui — if we're not in iwd2 already
	if (!iwd2class) {
		tm = gamedata->LoadTable("numwslot", true);
		if (tm) {
			TableMgr::index_t rowcount = tm->GetRowCount();
			for (TableMgr::index_t i = 0; i < rowcount; i++) {
				const auto& cls = tm->GetRowName(i);
				auto it = className2ID.find(cls);
				int id = 0;
				if (it != className2ID.end()) id = it->second;
				numWeaponSlots[id] = std::min<ieByte>(4, tm->QueryFieldUnsigned<ieByte>(i, 0));
			}
		}
	}
	className2ID.clear();

	// slurp up kitlist.2da; only iwd2 has both class and kit info in the same table
	if (!iwd2class) {
		tm = gamedata->LoadTable("kitlist", true);
		if (!tm) {
			error("Actor", "Missing kitlist.2da!");
		}
		for (TableMgr::index_t i = 0; i < tm->GetRowCount(); ++i) {
			
			const std::string& rowName = fmt::to_string(i);
			// kit usability is in hex and is sometimes used as the kit ID,
			// while other times ID is the baseclass constant or-ed with the index
			ieDword kitUsability = strtounsigned<ieDword>(tm->QueryField(rowName, "UNUSABLE").c_str(), NULL, 16);
			int classID = tm->QueryFieldSigned<int>(rowName, "CLASS");
			ResRef clab = tm->QueryField(rowName, "ABILITIES");
			const std::string& kitName = tm->QueryField(rowName, "ROWNAME");
			class2kits[classID].indices.push_back(i);
			class2kits[classID].ids.push_back(kitUsability);
			class2kits[classID].clabs.emplace_back(clab);
			class2kits[classID].kitNames.emplace_back(kitName);
		}
	}

	//wild magic level modifiers
	tm = gamedata->LoadTable("lvlmodwm", true);
	if (tm) {
		int modRange = tm->GetColumnCount();
		wmLevelMods.resize(modRange);
		TableMgr::index_t maxrow = tm->GetRowCount();
		for (int i = 0; i < modRange; i++) {
			wmLevelMods[i].resize(MAX_LEVEL);
			for (TableMgr::index_t j = 0; j < MAX_LEVEL; j++) {
				TableMgr::index_t row = maxrow;
				if (j<row) row=j;
				wmLevelMods[i][j] = tm->QueryFieldSigned<int>(row, i);
			}
		}
	}

	// verbal constant remapping, if omitted, it is an 1-1 mapping
	for (auto i : EnumIterator<Verbal>()) {
		VCMap[i] = i;
	}
	tm = gamedata->LoadTable("vcremap");
	if (tm) {
		TableMgr::index_t rows = tm->GetRowCount();

		for (TableMgr::index_t i = 0; i < rows; i++) {
			int row = tm->QueryFieldSigned<int>(i,0);
			if (row < 0 || row >= int(Verbal::count)) continue;
			Verbal value = EnumIndex<Verbal>(tm->QueryFieldUnsigned<under_t<Verbal>>(i, 1));
			if (value >= Verbal::count) continue;
			VCMap[row] = value;
		}
	}

	//initializing the skill->stats conversion table (used in iwd2)
	tm = gamedata->LoadTable("skillsta", true);
	if (tm) {
		TableMgr::index_t rowcount = tm->GetRowCount();
		TableMgr::index_t colcount = tm->GetColumnCount();
		for (TableMgr::index_t i = 0; i < rowcount; i++) {
			skillstats[i] = std::vector<int>();
			for (TableMgr::index_t j = 0; j < colcount; j++) {
				int val;
				// the stat and ability columns need conversion into numbers
				if (j < 2) {
					val = core->TranslateStat(tm->QueryField(i, j));
					if (j == 0) {
						stat2skill[val] = i;
					}
				} else {
					val = tm->QueryFieldSigned<int>(i, j);
				}
				skillstats[i].push_back (val);
			}
		}
	}

	// dexterity modifier for thieving skills
	tm = gamedata->LoadTable("skilldex");
	if (tm) {
		TableMgr::index_t skilldexNCols = tm->GetColumnCount();
		TableMgr::index_t skilldexNRows = tm->GetRowCount();
		skilldex.reserve(skilldexNRows);

		for (TableMgr::index_t i = 0; i < skilldexNRows; i++) {
			skilldex.emplace_back();
			skilldex[i].reserve(skilldexNCols+1);
			skilldex[i].push_back(std::stoi(tm->GetRowName(i)));
			for (TableMgr::index_t j = 0; j < skilldexNCols; j++) {
				skilldex[i].push_back(tm->QueryFieldSigned<int>(i, j));
			}
		}
	}

	// race modifier for thieving skills
	tm = gamedata->LoadTable("skillrac");
	int value = 0;
	int racetable = core->LoadSymbol("race");
	int subracetable = core->LoadSymbol("subrace");
	PluginHolder<SymbolMgr> race;
	PluginHolder<SymbolMgr> subrace;
	if (racetable != -1) {
		race = core->GetSymbol(racetable);
	}
	if (subracetable != -1) {
		subrace = core->GetSymbol(subracetable);
	}
	if (tm) {
		TableMgr::index_t cols = tm->GetColumnCount();
		TableMgr::index_t rows = tm->GetRowCount();
		skillrac.reserve(rows);

		for (TableMgr::index_t i = 0; i < rows; i++) {
			skillrac.emplace_back();
			skillrac[i].reserve(cols+1);
			// figure out the value from the race name
			if (racetable == -1) {
				value = 0;
			} else {
				if (subracetable == -1) {
					value = race->GetValue(tm->GetRowName(i));
				} else {
					value = subrace->GetValue(tm->GetRowName(i));
				}
			}
			skillrac[i].push_back(value);
			for (TableMgr::index_t j = 0; j < cols; j++) {
				skillrac[i].push_back(tm->QueryFieldSigned<int>(i, j));
			}
		}
	}

	//preload stat derived animation tables
	tm = gamedata->LoadTable("avprefix");
	avBase = 0;
	if (tm) {
		TableMgr::index_t count = tm->GetRowCount();
		if (count> 0 && count<8) {
			avPrefix.resize(count - 1);
			avBase = tm->QueryFieldSigned<int>(0, 0);
			const auto& poi = tm->QueryField(0,1);
			if (poi[0] != '*') {
				avStance = tm->QueryFieldSigned<int>(0,1);
			} else {
				avStance = -1;
			}
			for (TableMgr::index_t i = 0; i < avPrefix.size(); i++) {
				avPrefix[i].avresref = tm->QueryField(i + 1, 0);
				avPrefix[i].avtable = gamedata->LoadTable(avPrefix[i].avresref);
				if (avPrefix[i].avtable) {
					avPrefix[i].stat = core->TranslateStat(avPrefix[i].avtable->QueryField(0, 0));
				} else {
					avPrefix[i].stat = -1;
				}
			}
		}
	}

	// races table
	tm = gamedata->LoadTable("races");
	if (tm && !pstflags) {
		TableMgr::index_t racesNRows = tm->GetRowCount();

		for (TableMgr::index_t i = 0; i < racesNRows; i++) {
			int raceID = tm->QueryFieldSigned<int>(i, 3);
			int favClass = tm->QueryFieldSigned<int>(i, 8);
			const auto& raceName = tm->GetRowName(i);
			favoredMap.emplace(raceID, favClass);
			raceID2Name.emplace(raceID, raceName);
		}
	}

	// IWD, IWD2 and BG:EE have this
	int splstatetable = core->LoadSymbol("splstate");
	if (splstatetable != -1) {
		auto splstate = core->GetSymbol(splstatetable);
		int numstates = splstate->GetHighestValue();
		if (numstates > 0) {
			//rounding up
			// iwd1 has a practically empty ids though, so force a minimum
			SpellStatesSize = std::max(6, (numstates >> 5) + 1);
		}
	} else {
		SpellStatesSize = 6;
	}

	// modal actions/state data
	ReadModalStates();
}

void Actor::SetLockedPalette(const ieDword *gradients)
{
	if (!anims) return; //cannot apply it (yet)
	anims->LockPalette(gradients);
}

void Actor::UnlockPalette()
{
	if (!anims) return;
	anims->lockPalette=false;
	anims->SetColors(&Modified[IE_COLORS]);
}

void Actor::AddAnimation(const ResRef& resource, int gradient, int height, int flags)
{
	ScriptedAnimation *sca = gamedata->GetScriptedAnimation(resource, false);
	if (!sca)
		return;
	sca->ZOffset = height;
	if (flags&AA_PLAYONCE) {
		sca->PlayOnce();
	}
	if (flags&AA_BLEND) {
		//pst anims need this?
		sca->SetBlend();
	}
	if (gradient!=-1) {
		sca->SetPalette(gradient, 4);
	}
	AddVVCell(sca);
}

ieDword Actor::GetSpellFailure(bool arcana) const
{
	ieDword base = arcana?Modified[IE_SPELLFAILUREMAGE]:Modified[IE_SPELLFAILUREPRIEST];
	if (HasSpellState(SS_DOMINATION)) base += 100;
	// blink's malus of 20% is handled in the effect
	// IWD2 has this as 20, other games as 50
	if (HasSpellState(SS_DEAF)) {
		base += 20;
		if (!third) base += 30;
	}
	if (!arcana) return base;

	ieDword armor = GetTotalArmorFailure();

	if (armor) {
		ieDword feat = GetFeat(FEAT_ARMORED_ARCANA);
		if (armor<feat) armor = 0;
		else armor -= feat;
	}

	return base+armor*5;
}

//dexterity AC (the lesser the better), do another negation for 3ED rules
int Actor::GetDexterityAC() const
{
	if (!third) {
		return core->GetDexterityBonus(STAT_DEX_AC, GetStat(IE_DEX));
	}

	int dexbonus = GetAbilityBonus(IE_DEX);
	if (dexbonus) {
		// the maximum dexterity bonus isn't stored,
		// but can reliably be calculated from 8-spell failure (except for robes, which have no limit)
		ieWord armtype = inventory.GetArmorItemType();
		int armor = core->GetArmorFailure(armtype);

		if (armor) {
			armor = 8-armor;
			if (dexbonus>armor) {
				dexbonus = armor;
			}
		}

		//blindness negates the dexbonus
		if ((GetStat(IE_STATE_ID)&STATE_BLIND) && !HasFeat(FEAT_BLIND_FIGHT)) {
			dexbonus = 0;
		}
	}
	return dexbonus;
}

//wisdom AC bonus for 3ed light monks
int Actor::GetWisdomAC() const
{
	if (!third || !GetStat(IE_LEVELMONK)) {
		return 0;
	}

	int bonus = 0;
	//if the monk has any typo of armor equipped, no bonus
	if (GetTotalArmorFailure() == 0) {
		bonus = GetAbilityBonus(IE_WIS);
	}
	return bonus;
}

//Returns the personal critical damage type in a binary compatible form (PST)
ieWord Actor::GetCriticalType() const
{
	AutoTable tm = gamedata->LoadTable("crits", true);
	if (!tm) return 0;
	//the ID of this PC (first 2 rows are empty)
	int row = BaseStats[IE_SPECIFIC];
	//defaults to 0
	return tm->QueryFieldUnsigned<ieWord>(row, 1);
}

//Plays personal critical damage animation for PST PC's melee attacks
void Actor::PlayCritDamageAnimation(int type)
{
	AutoTable tm = gamedata->LoadTable("crits");
	if (!tm) return;
	//the ID's are in column 1, selected by specifics by GetCriticalType
	TableMgr::index_t row = tm->FindTableValue (1, type);
	if (row != TableMgr::npos) {
		//the animations are listed in column 0
		AddAnimation(tm->QueryField(row, 0), -1, 45, AA_PLAYONCE|AA_BLEND);
	}
}

void Actor::PlayDamageAnimation(int type, bool hit)
{
	if (!anims) return;
	int i;
	int flags = AA_PLAYONCE;
	int height = 22;
	if (pstflags) {
		flags |= AA_BLEND;
		height = 45; // empirical like in fx_visual_spell_hit
	}

	Log(COMBAT, "Actor", "Damage animation type: {}", type);
	const Effect* fx;
	switch(type&255) {
		case 0:
			//PST specific personal criticals
			if (type&0xff00) {
				PlayCritDamageAnimation(type>>8);
				break;
			}
			//fall through
		case 1: case 2: case 3: //blood
			i = anims->GetBloodColor();
			if (!i) i = d_gradient[type];
			fx = fxqueue.HasEffectWithParam(fx_animation_override_data_ref, 2);
			if (fx) {
				i = fx->Parameter1;
			}
			if (hit) {
				AddAnimation(d_main[type], i, height, flags);
			}
			break;
		case 4: case 5: case 6: //fire
			if(hit) {
				AddAnimation(d_main[type], d_gradient[type], height, flags);
			}
			for(i=DL_FIRE;i<=type;i++) {
				AddAnimation(d_splash[i], d_gradient[i], height, flags);
			}
			break;
		case 7: case 8: case 9: //electricity
			if (hit) {
				AddAnimation(d_main[type], d_gradient[type], height, flags);
			}
			for(i=DL_ELECTRICITY;i<=type;i++) {
				AddAnimation(d_splash[i], d_gradient[i], height, flags);
			}
			break;
		case 10: case 11: case 12://cold
			if (hit) {
				AddAnimation(d_main[type], d_gradient[type], height, flags);
			}
			break;
		case 13: case 14: case 15://acid
			if (hit) {
				AddAnimation(d_main[type], d_gradient[type], height, flags);
			}
			break;
		case 16: case 17: case 18://disintegrate
			if (hit) {
				AddAnimation(d_main[type], d_gradient[type], height, flags);
			}
			break;
	}
}


Actor::stat_t Actor::ClampStat(unsigned int StatIndex, stat_t Value) const
{
	if (StatIndex >= MAX_STATS) {
		return Value;
	}

	if ((signed) Value < -100) {
		Value = (stat_t) -100;
	} else {
		if (maximum_values[StatIndex] > 0) {
			if ((signed) Value > 0 && Value > maximum_values[StatIndex]) {
				Value = maximum_values[StatIndex];
			}
		}
	}
	return Value;
}

bool Actor::SetStat(unsigned int StatIndex, stat_t Value, int pcf)
{
	if (StatIndex >= MAX_STATS) {
		return false;
	}
	Value = ClampStat(StatIndex, Value);

	unsigned int previous = GetSafeStat(StatIndex);
	if (Modified[StatIndex]!=Value) {
		Modified[StatIndex] = Value;
	}
	if (previous!=Value) {
		if (pcf) {
			PostChangeFunctionType f = post_change_functions[StatIndex];
			if (f) {
				(*f)(this, previous, Value);
			}
		}
	}
	return true;
}

int Actor::GetMod(unsigned int StatIndex) const
{
	if (StatIndex >= MAX_STATS) {
		return 0xdadadada;
	}
	return (signed) Modified[StatIndex] - (signed) BaseStats[StatIndex];
}
/** Returns a Stat Base Value */
ieDword Actor::GetBase(unsigned int StatIndex) const
{
	if (StatIndex >= MAX_STATS) {
		return 0xffff;
	}
	return BaseStats[StatIndex];
}

/** Sets a Stat Base Value */
/** If required, modify the modified value and run the pcf function */
bool Actor::SetBase(unsigned int StatIndex, stat_t Value)
{
	if (StatIndex >= MAX_STATS) {
		return false;
	}
	ieDword diff = Modified[StatIndex]-BaseStats[StatIndex];

	//maximize the base stat
	Value = ClampStat(StatIndex, Value);
	BaseStats[StatIndex] = Value;

	//if already initialized, then the modified stats
	//might need to run the post change function (stat change can kill actor)
	SetStat (StatIndex, Value+diff, InternalFlags&IF_INITIALIZED);
	return true;
}

bool Actor::SetBaseNoPCF(unsigned int StatIndex, stat_t Value)
{
	if (StatIndex >= MAX_STATS) {
		return false;
	}
	ieDword diff = Modified[StatIndex]-BaseStats[StatIndex];

	//maximize the base stat
	Value = ClampStat(StatIndex, Value);
	BaseStats[StatIndex] = Value;

	//if already initialized, then the modified stats
	//might need to run the post change function (stat change can kill actor)
	SetStat (StatIndex, Value+diff, 0);
	return true;
}

bool Actor::SetBaseBit(unsigned int StatIndex, stat_t Value, bool setreset)
{
	if (StatIndex >= MAX_STATS) {
		return false;
	}
	if (setreset) {
		BaseStats[StatIndex] |= Value;
	} else {
		BaseStats[StatIndex] &= ~Value;
	}
	//if already initialized, then the modified stats
	//need to run the post change function (stat change can kill actor)
	if (setreset) {
		SetStat (StatIndex, Modified[StatIndex]|Value, InternalFlags&IF_INITIALIZED);
	} else {
		SetStat (StatIndex, Modified[StatIndex]&~Value, InternalFlags&IF_INITIALIZED);
	}
	return true;
}

void Actor::AddPortraitIcon(ieByte icon) const
{
	if (!PCStats) {
		return;
	}
	PCStats->EnableState(icon);
}

void Actor::DisablePortraitIcon(ieByte icon) const
{
	if (!PCStats) {
		return;
	}
	PCStats->DisableState(icon);
}


//hack to get the proper casting sounds of copied images
ieDword Actor::GetCGGender() const
{
	ieDword gender = Modified[IE_SEX];
	if (gender == SEX_ILLUSION) {
		const Actor *master = core->GetGame()->GetActorByGlobalID(Modified[IE_PUPPETMASTERID]);
		if (master) {
			gender = master->Modified[IE_SEX];
		}
	}

	return gender;
}

void Actor::CheckPuppet(Actor *puppet, ieDword type)
{
	if (!puppet) return;
	if (puppet->Modified[IE_STATE_ID]&STATE_DEAD) return;

	switch(type) {
		case 1:
			Modified[IE_STATE_ID] |= state_invisible;
			//also set the improved invisibility flag where available
			if (!pstflags) {
				Modified[IE_STATE_ID]|=STATE_INVIS2;
			}
			break;
		case 2:
			if (InterruptCasting) {
				// dispel the projected image if there is any
				puppet->DestroySelf();
				return;
			}
			Modified[IE_HELD]=1;
			AddPortraitIcon(PI_PROJIMAGE);
			Modified[IE_STATE_ID]|=STATE_HELPLESS;
			break;
	}
	Modified[IE_PUPPETTYPE] = type;
	Modified[IE_PUPPETID] = puppet->GetGlobalID();
}

Actor::stats_t Actor::ResetStats(bool init)
{
	//put all special cleanup calls here
	if (anims) {
		anims->CheckColorMod();
	}
	spellbook.ClearBonus();
	BardSong.Reset();
	projectileImmunity.clear();

	if (PCStats) {
		PCStats->States = PCStatsStruct::StateArray();
	}
	if (SpellStatesSize) {
		memset(spellStates, 0, sizeof(ieDword) * SpellStatesSize);
	}
	AC.ResetAll();
	ToHit.ResetAll(); // effects can result in the change of any of the boni, so we need to reset all
	
	stats_t prev;
	if (init) {
		InternalFlags|=IF_INITIALIZED;
		prev = BaseStats;
	} else {
		prev = Modified;
	}
	PrevStats = &prev[0];

	//copy back the original stats, because the effects
	//will be reapplied in ApplyAllEffects again
	Modified = BaseStats;
	return prev;
}

/** call this after load, to apply effects */
void Actor::AddEffects(EffectQueue&& fx)
{
	bool first = !(InternalFlags&IF_INITIALIZED); //initialize base stats
	stats_t prev = ResetStats(first);
	
	fx.SetOwner(this);
	fx.AddAllEffects(this, Pos);

	if (SpellStatesSize) {
		memset(spellStates, 0, sizeof(ieDword) * SpellStatesSize);
	}
	//also clear the spell bonuses just given, they will be
	//recalculated below again
	spellbook.ClearBonus();
	//AC.ResetAll(); // TODO: check if this is needed
	//ToHit.ResetAll();
	
	RefreshEffects(first, prev);
}

void Actor::RefreshEffects(bool first, const stats_t& previous)
{
	// some VVCs are controlled by stats (and so by PCFs), the rest have 'effect_owned' set
	for (ScriptedAnimation* vvc : vfxQueue) {
		if (vvc->effect_owned) vvc->active = false;
	}

	// apply palette changes not caused by persistent effects
	if (Modified[IE_STATE_ID] & STATE_PETRIFIED) {
		SetLockedPalette(fullstone);
	} else if (Modified[IE_STATE_ID] & STATE_FROZEN) {
		SetLockedPalette(fullwhite);
	}

	// give the 3ed save bonus before applying the effects, since they may do extra rolls
	if (third) {
		Modified[IE_SAVEWILL] += GetAbilityBonus(IE_WIS);
		Modified[IE_SAVEREFLEX] += GetAbilityBonus(IE_DEX);
		Modified[IE_SAVEFORTITUDE] += GetAbilityBonus(IE_CON);
		// paladins add their charisma modifier to all saving throws
		if (GetPaladinLevel()) {
			Modified[IE_SAVEWILL] += GetAbilityBonus(IE_CHR);
			Modified[IE_SAVEREFLEX] += GetAbilityBonus(IE_CHR);
			Modified[IE_SAVEFORTITUDE] += GetAbilityBonus(IE_CHR);
		}
	}

	fxqueue.ApplyAllEffects( this );

	const Game* game = core->GetGame();
	if (previous[IE_PUPPETID]) {
		CheckPuppet(game->GetActorByGlobalID(previous[IE_PUPPETID]), previous[IE_PUPPETTYPE]);
	}

	//move this further down if needed
	PrevStats = NULL;

	for (auto& trigger : triggers) {
		trigger.flags |= TEF_PROCESSED_EFFECTS;

		// snap out of charm if the charmer hurt us
		if (trigger.triggerID == trigger_attackedby) {
			const Actor* attacker = game->GetActorByGlobalID(objects.LastAttacker);
			if (attacker) {
				int revertToEA = 0;
				if (Modified[IE_EA] == EA_CHARMED && attacker->GetStat(IE_EA) <= EA_GOODCUTOFF) {
					revertToEA = EA_ENEMY;
				} else if (Modified[IE_EA] == EA_CHARMEDPC && attacker->GetStat(IE_EA) >= EA_EVILCUTOFF) {
					revertToEA = EA_PC;
				}
				if (revertToEA) {
					// remove only the plain charm effect
					const Effect *charmfx = fxqueue.HasEffectWithParam(fx_set_charmed_state_ref, 1);
					if (!charmfx) charmfx = fxqueue.HasEffectWithParam(fx_set_charmed_state_ref, 1001);
					if (charmfx) {
						SetStat(IE_EA, revertToEA, 1);
						fxqueue.RemoveEffect(charmfx);
					}
				}
			}
		}
	}
	// we need to recalc these, since the stats or equipped gear may have changed (and this is relevant in iwd2)
	AC.SetWisdomBonus(GetWisdomAC());
	// FIXME: but the effects may reset this too and we shouldn't touch it in that case (flatfooted!)
	// flatfooted by invisible attacker: this is handled by GetDefense and ok
	AC.SetDexterityBonus(GetDexterityAC());

	if (HasPlayerClass()) {
		RefreshPCStats();
	}

	//if the animation ID was not modified by any effect, it may still be modified by something else
	// but not if pst is playing disguise tricks (GameScript::SetNamelessDisguise)
	ieDword pst_appearance = 0;
	if (pstflags) {
		pst_appearance = game->GetGlobal("APPEARANCE", 0);
	}
	if (Modified[IE_SEX] != BaseStats[IE_SEX] && pst_appearance == 0) {
		UpdateAnimationID(true);
	}

	//delayed HP adjustment hack (after max HP modification)
	//as it's triggered by PCFs from the previous tick, it should probably run before current PCFs
	if (first && checkHP == 2) {
		//could not set this in the constructor
		checkHPTime = game->GameTime;
	} else if (checkHP && checkHPTime != game->GameTime) {
		checkHP = 0;
		if (!(BaseStats[IE_STATE_ID] & STATE_DEAD)) pcf_hitpoint(this, 0, BaseStats[IE_HITPOINTS]);
	}

	// apply haste bonuses to attacks per round, possibly exceeding the limit of 5
	// this must be done after ALL other modifiers, including skill effects in RefreshPCStats() above
	if (GetStat(IE_STATE_ID) & STATE_HASTED) {
		int apr = GetStat(IE_NUMBEROFATTACKS);
		switch ((signed) GetStat(IE_IMPROVEDHASTE)) {
		case 1: // improved
			apr += apr; // x2
			break;
		case 0: // normal
			apr += 2 - apr % 2; // +1, round down
			break;
		case -1: // weak
			if (apr % 2) apr++; // round up
			break;
		}
		Modified[IE_NUMBEROFATTACKS] = apr;
	}

	for (int i=0; i < MAX_STATS; ++i) {
		if (first || Modified[i]!=previous[i]) {
			PostChangeFunctionType f = post_change_functions[i];
			if (f) {
				(*f)(this, previous[i], Modified[i]);
			}
		}
	}

	// manually update the overlays
	// we make sure to set them without pcfs, since they would trample each other otherwise
	if (Modified[IE_SANCTUARY] != BaseStats[IE_SANCTUARY]) {
		pcf_sanctuary(this, BaseStats[IE_SANCTUARY], Modified[IE_SANCTUARY]);
	}

	//add wisdom/casting_ability bonus spells
	if (spellbook.IsIWDSpellBook()) {
		// check each class separately for the casting stat and booktype (luckily there is no bonus for domain spells)
		for (int i = 0; i < ISCLASSES; ++i) {
			int level = GetClassLevel(i);
			int booktype = booksiwd2[i]; // ieIWD2SpellType
			if (!level || booktype == -1) {
				continue;
			}
			level = Modified[castingStat[classesiwd2[i]]];
			spellbook.BonusSpells(booktype, level);
		}
	} else {
		int level = Modified[IE_WIS];
		spellbook.BonusSpells(IE_SPELL_TYPE_PRIEST, level);
	}

	// iwd2 barbarian speed increase isn't handled like for monks (normal clab)!?
	if (third && GetBarbarianLevel()) {
		Modified[IE_MOVEMENTRATE] += 1;
	}

	// check if any new portrait icon was removed or added
	if (PCStats && PCStats->States != previousStates) {
		core->SetEventFlag(EF_PORTRAIT);
		previousStates = PCStats->States;
	}
	if (Immobile()) {
		timeStartStep = game->Ticks;
	}
}

void Actor::RefreshEffects()
{
	bool first = !(InternalFlags&IF_INITIALIZED); //initialize base stats
	RefreshEffects(first, ResetStats(first));
}

int Actor::GetProficiency(ieByte proftype) const
{
	switch(proftype) {
	case 254: // -2, hand to hand old style
		return 1;
	case 255: // -1, no proficiency
		return 0;
	default:
		// bg2 actually supported both styles of proficiencies, so take whichever is better
		// bg1 style proficiencies
		stat_t prof = 0;
		if (proftype <= IE_EXTRAPROFICIENCY20 - IE_PROFICIENCYBASTARDSWORD) {
			prof = GetStat(IE_PROFICIENCYBASTARDSWORD + proftype);
		}

		// bg2 style proficiencies
		// iwd2 feat-based ones are in the same range as IE_FEAT_BOW == IE_PROFICIENCYBASTARDSWORD
		if (proftype >= IE_PROFICIENCYBASTARDSWORD && proftype <= IE_EXTRAPROFICIENCY20) {
			prof = std::max(prof, GetStat(proftype));
		}
		return int(prof);
	}
}

// recalculates the constitution bonus to hp and adds it to the stat
void Actor::RefreshHP() {
	// calculate the hp bonus for each level
	//	single-classed characters:
	//		apply full constitution bonus for levels up (and including) to maxLevelForHpRoll
	//	dual-classed characters:
	//		while inactive, there is no consititution bonus and hp gain AT ALL
	//		afterwards, the same applies as for single-classed characters again
	//			consititution bonus is NOT taken from the max of classes
	//	multi-classed characters:
	//		apply the highest constitution bonus for levels up (and including) to maxLevelForHpRoll (already the max of the classes)
	//		BUT divide it by the number of classes (ideally the last one to levelup should get all the fractions)
	//	for levels after maxLevelForHpRoll there is NO constitution bonus anymore
	// IN IWD2, it's a simple level*conbon calculation without any fiddling
	int bonus;
	// this is wrong for dual-classed (so we override it later)
	// and sometimes marginally wrong for multi-classed (but we usually round the average up)
	int bonlevel = GetXPLevel(true);
	ieDword bonindex = BaseStats[IE_CLASS]-1;

	//we must limit the levels to the max allowable
	if (!third && bonlevel > maxLevelForHpRoll[bonindex]) {
		bonlevel = maxLevelForHpRoll[bonindex];
	}
	if (IsDualClassed()) {
		int oldbonus = 0;

		// just the old consititution bonus
		int oldlevel = IsDualSwap() ? BaseStats[IE_LEVEL] : BaseStats[IE_LEVEL2];
		bonlevel = IsDualSwap() ? BaseStats[IE_LEVEL2] : BaseStats[IE_LEVEL];
		oldlevel = (oldlevel > maxLevelForHpRoll[bonindex]) ? maxLevelForHpRoll[bonindex] : oldlevel;
		// give the bonus only for the levels where there were actually rolls
		// if we wanted to be really strict, the old bonindex and max roll level would need to be looked up
		if (oldlevel == maxLevelForHpRoll[bonindex]) {
			bonlevel = 0;
		} else {
			bonlevel -= oldlevel; // the actual number of "rolling" levels for the new bonus
			if (bonlevel + oldlevel > maxLevelForHpRoll[bonindex]) {
				bonlevel = maxLevelForHpRoll[bonindex] - oldlevel;
			}
		}
		if (bonlevel < 0) bonlevel = 0;
		if (Modified[IE_MC_FLAGS] & (MC_WAS_FIGHTER|MC_WAS_RANGER)) {
			oldbonus = core->GetConstitutionBonus(STAT_CON_HP_WARRIOR, Modified[IE_CON]);
		} else {
			oldbonus = core->GetConstitutionBonus(STAT_CON_HP_NORMAL, Modified[IE_CON]);
		}
		bonus = oldbonus * oldlevel;

		// but if the class is already reactivated ...
		if (!IsDualInactive()) {
			// add in the bonus for the levels of the new class
			// since there are no warrior to warrior dual-classes, just invert the previous check to get the right conmod
			if (Modified[IE_MC_FLAGS] & (MC_WAS_FIGHTER|MC_WAS_RANGER)) {
				bonus += bonlevel * core->GetConstitutionBonus(STAT_CON_HP_NORMAL, Modified[IE_CON]);
			} else {
				bonus += GetHpAdjustment(bonlevel);
			}
		}
	} else {
		bonus = GetHpAdjustment(bonlevel);
	}

	if (bonus<0 && (Modified[IE_MAXHITPOINTS]+bonus)<=0) {
		bonus=1-Modified[IE_MAXHITPOINTS];
	}

	//we still apply the maximum bonus to dead characters, but don't apply
	//to current HP, or we'd have dead characters showing as having hp
	Modified[IE_MAXHITPOINTS]+=bonus;

	// temporary con bonuses also modify current HP; this can kill! (EE behavior)
	// but skip on game load, while old bonuses are still re-applied
	if (!(BaseStats[IE_STATE_ID]&STATE_DEAD) && checkHP != 2 && bonus != lastConBonus) {
		BaseStats[IE_HITPOINTS] += bonus - lastConBonus;
	}
	lastConBonus = bonus;
}

// refresh stats on creatures (PC or NPC) with a valid class (not animals etc)
// internal use only, and this is maybe a stupid name :)
void Actor::RefreshPCStats() {
	RefreshHP();

	const Game *game = core->GetGame();
	//morale recovery every xth AI cycle ... except for pst pcs
	int mrec = GetStat(IE_MORALERECOVERYTIME);
	if (mrec && ShouldModifyMorale()) {
		if (!(game->GameTime%mrec)) {
			int morale = (signed) BaseStats[IE_MORALE];
			if (morale < 10) {
				NewBase(IE_MORALE, 1, MOD_ADDITIVE);
			} else if (morale > 10) {
				NewBase(IE_MORALE, (stat_t) -1, MOD_ADDITIVE);
			}
		}
	}

	// handle intoxication
	// the cutoff is at half of max, coinciding with where the intoxmod penalties start
	// TODO: intoxmod, intoxcon
	if (BaseStats[IE_INTOXICATION] >= 50) {
		AddPortraitIcon(PI_DRUNK);
	} else {
		DisablePortraitIcon(PI_DRUNK);
	}

	//get the wspattack bonuses for proficiencies
	const ITMExtHeader* header = GetWeapon(false);
	ieDword stars;
	int dualwielding = IsDualWielding();
	stars = GetProficiency(weaponInfo[0].prof) & PROFS_MASK;

	// tenser's transformation ensures the actor is at least proficient with any weapon
	if (!stars && HasSpellState(SS_TENSER)) stars = 1;

	if (header) {
		//wspattack appears to only effect warriors
		int defaultattacks = 2 + 2*dualwielding;
		if (stars) {
			// In bg2 the proficiency and warrior level bonus is added after effects, so also ranged weapons are affected,
			// since their rate of fire (apr) is set using an effect with a flat modifier.
			// SetBase will compensate only for the difference between the current two stats, not considering the default
			// example: actor with a bow gets 4 due to the equipping effect, while the wspatck bonus is 0-3
			// the adjustment results in a base of 2-5 (2+[0-3]) and the modified stat degrades to 4+(4-[2-5]) = 8-[2-5] = 3-6
			// instead of 4+[0-3] = 4-7
			// For a master ranger at level 14, the difference ends up as 2 (1 apr).
			ieDword warriorLevel = GetWarriorLevel();
			if (warriorLevel) {
				int mod = Modified[IE_NUMBEROFATTACKS] - BaseStats[IE_NUMBEROFATTACKS];
				int bonus = gamedata->GetWeaponStyleAPRBonus(stars, warriorLevel - 1);
				BaseStats[IE_NUMBEROFATTACKS] = defaultattacks + bonus;
				if (fxqueue.HasEffectWithParam(fx_attacks_per_round_modifier_ref, 1)) { // launcher sets base APR
					Modified[IE_NUMBEROFATTACKS] += bonus; // no default
				} else {
					Modified[IE_NUMBEROFATTACKS] = BaseStats[IE_NUMBEROFATTACKS] + mod;
				}
			} else {
				SetBase(IE_NUMBEROFATTACKS, defaultattacks + gamedata->GetWeaponStyleAPRBonus(stars, 0));
			}
		} else {
			// unproficient user - force defaultattacks
			SetBase(IE_NUMBEROFATTACKS, defaultattacks);
		}
	}

	// apply the intelligence and wisdom bonus to lore
	Modified[IE_LORE] += core->GetLoreBonus(0, Modified[IE_INT]) + core->GetLoreBonus(0, Modified[IE_WIS]);

	UpdateFatigue();

	// add luck bonus from difficulty
	Modified[IE_LUCK] += DifficultyLuckMod;

	// regenerate actors with high enough constitution
	int rate = GetConHealAmount();
	if (rate && !(game->GameTime % rate)) {
		NewBase(IE_HITPOINTS, 1, MOD_ADDITIVE);
		if (core->HasFeature(GFFlags::ONSCREEN_TEXT) && InParty && Modified[IE_HITPOINTS] < Modified[IE_MAXHITPOINTS]) {
			// eeeh, no token (Heal: 1)
			static const String text = fmt::format(u"{} 1", core->GetString(ieStrRef::HEAL));
			overHead.SetText(text);
		}
	}

	// adjust thieving skills with dex and race
	// table header is in this order:
	// PICK_POCKETS  OPEN_LOCKS  FIND_TRAPS  MOVE_SILENTLY  HIDE_IN_SHADOWS  DETECT_ILLUSION  SET_TRAPS
	Modified[IE_PICKPOCKET] += GetSkillBonus(1);
	if (Modified[IE_RACE] == 153 && !third) { // sigh, tieflings had hardcoded bonuses instead of a table entry
		Modified[IE_PICKPOCKET] += 20;
	}
	Modified[IE_LOCKPICKING] += GetSkillBonus(2);
	// these are governed by other stats in iwd2 (int) or don't exist (set traps)
	if (!third) {
		Modified[IE_TRAPS] += GetSkillBonus(3);
		Modified[IE_DETECTILLUSIONS] += GetSkillBonus(6);
		Modified[IE_SETTRAPS] += GetSkillBonus(7);
	}
	Modified[IE_STEALTH] += GetSkillBonus(4);
	Modified[IE_HIDEINSHADOWS] += GetSkillBonus(5);

	if (third) {
		ieDword LayOnHandsAmount = GetPaladinLevel();
		if (LayOnHandsAmount) {
			int mod = GetAbilityBonus(IE_CHR, Modified[IE_CHR]);
			if (mod > 1) {
				LayOnHandsAmount *= mod;
			}
		}
		BaseStats[IE_LAYONHANDSAMOUNT] = LayOnHandsAmount;
		Modified[IE_LAYONHANDSAMOUNT] = LayOnHandsAmount;
	}

}

int Actor::GetConHealAmount() const
{
	int rate = 0;
	const Game *game = core->GetGame();
	if (!game) return rate;

	if (core->HasFeature(GFFlags::AREA_OVERRIDE) && game->GetPC(0, false) == this) {
		rate = core->GetConstitutionBonus(STAT_CON_TNO_REGEN, Modified[IE_CON]);
	} else {
		rate = core->GetConstitutionBonus(STAT_CON_HP_REGEN, Modified[IE_CON]);
		rate *= core->Time.defaultTicksPerSec;
	}
	return rate;
}

// add fatigue every 4 hours since resting and check if the actor is penalised for it
void Actor::UpdateFatigue()
{
	const Game *game = core->GetGame();
	const GameControl* gc = core->GetGameControl();
	if (!InParty || !game->GameTime || !gc || gc->InDialog() || core->InCutSceneMode()) {
		return;
	}

	bool updated = false;
	if (!TicksLastRested) {
		// just loaded the game; approximate last rest
		TicksLastRested = game->GameTime - (2*core->Time.hour_size) * (2*GetBase(IE_FATIGUE)+1);
		updated = true;
	} else if (LastFatigueCheck) {
		ieDword FatigueDiff = (game->GameTime - TicksLastRested) / (4*core->Time.hour_size)
		                    - (LastFatigueCheck - TicksLastRested) / (4*core->Time.hour_size);
		if (FatigueDiff) {
			NewBase(IE_FATIGUE, FatigueDiff, MOD_ADDITIVE);
			updated = true;
		}
	}
	LastFatigueCheck = game->GameTime;

	if (!core->HasFeature(GFFlags::AREA_OVERRIDE)) {
		// pst has TNO regeneration stored there
		// shouldn't we check for our own flag, though?
		// FIXME: the Con bonus is applied dynamically, but this doesn't appear to conform to
		// the original engine behavior?  we should probably apply the bonus on rest
		int FatigueBonus = core->GetConstitutionBonus(STAT_CON_FATIGUE, Modified[IE_CON]);
		if ((signed) Modified[IE_FATIGUE] >= FatigueBonus) {
			Modified[IE_FATIGUE] -= FatigueBonus;
		} else {
			Modified[IE_FATIGUE] = 0;
		}
	}

	int LuckMod = core->ResolveStatBonus(this, "fatigue"); // fatigmod.2da
	Modified[IE_LUCK] += LuckMod;
	if (LuckMod < 0) {
		AddPortraitIcon(PI_FATIGUE);
		if (updated) {
			// stagger the complaint, so long travels don't cause a fatigue choir
			FatigueComplaintDelay = core->Roll(3, core->Time.round_size, 0) * 5;
		}
	} else {
		// the icon can be added manually; eg. by spcl321 in bg2 (berserker enrage)
		if (!fxqueue.HasEffectWithParam(fx_display_portrait_icon_ref, PI_FATIGUE)) {
			DisablePortraitIcon(PI_FATIGUE);
		}
		FatigueComplaintDelay = 0;
	}

	if (FatigueComplaintDelay) {
		FatigueComplaintDelay--;
		if (!FatigueComplaintDelay) {
			VerbalConstant(Verbal::Tired, gamedata->GetVBData("SPECIAL_COUNT"));
		}
	}
}

void Actor::RollSaves()
{
	static ieByte saveDiceSides = (ieByte) gamedata->GetMiscRule("SAVING_THROW_DICE_SIDES");
	if (InternalFlags&IF_USEDSAVE) {
		for (auto& save : lastSave.savingThrow) {
			save = RAND<ieByte>(1, saveDiceSides);
		}
		InternalFlags&=~IF_USEDSAVE;
	}
}

static int AdjustSaveVsSchool(int base, ieDword school, const Effect* fx)
{
	if (fx && fx->IsVariable == ieWord(school)) {
		if (fx->Parameter2 == 1) {
			base = fx->Parameter1;
		} else if (fx->Parameter2 == 2) {
			base = base * fx->Parameter1 / 100;
		} else {
			base += fx->Parameter1;
		}
	}
	return base;
}

//saving throws:
//type      bits in file    order in stats
//0  spells            1    4
//1  breath            2    3
//2  death             4    0
//3  wands             8    1
//4  polymorph        16    2

//iwd2 (luckily they use the same bits as it would be with bg2):
//0 not used
//1 not used
//2 fortitude          4   0
//3 reflex             8   1
//4 will              16   2

// in adnd, the stat represents the limit (DC) that the roll with all the boni has to pass
// since it is a derived stat, we also store the direct effect bonus/malus in it, but make sure to do it negated
// in 3ed, the stat is added to the roll and boni (not negated), then compared to some predefined value (DC)
static const std::array<int, 5> savingThrows = { IE_SAVEVSSPELL, IE_SAVEVSBREATH, IE_SAVEVSDEATH, IE_SAVEVSWANDS, IE_SAVEVSPOLY };

/** returns true if actor made the save against saving throw type */
bool Actor::GetSavingThrow(ieDword type, int modifier, const Effect *fx)
{
	assert(type < savingThrows.size());
	static int saveDiceSides = gamedata->GetMiscRule("SAVING_THROW_DICE_SIDES");
	InternalFlags|=IF_USEDSAVE;
	int ret = lastSave.savingThrow[type];
	// NOTE: assuming criticals apply to iwd2 too
	if (ret == 1) return false;
	if (ret == saveDiceSides) return true;
	const Effect* sfx = fxqueue.HasEffect(fx_save_vs_school_bonus_ref);

	if (!third) {
		ret += modifier + GetStat(IE_LUCK);

		// also take any "vs school" bonus into account
		if (fx && sfx) {
			ret = AdjustSaveVsSchool(ret, fx->PrimaryType, sfx);
		}

		// pst litany of curses has a bonus depending on number of curses known
		// due to a bug, it was a malus in the original
		if (pstflags && fx && fx->SourceRef == "spin101" && fx->Opcode != 45) {
			ret -= CheckVariable(nullptr, "Morte_Taunt", "GLOBAL");
		}

		// potentially display feedback, but do some rate limiting, since each effect in a spell ends up here
		if (core->HasFeedback(FT_COMBAT) && (lastSave.prevType != type || lastSave.prevRoll != ret)) {
			// "Save Vs Death" in all games except pst: "Save Vs. Death:"
			String msg = core->GetString(DisplayMessage::GetStringReference(HCStrings(ieDword(HCStrings::SaveSpell) + type)));
			msg += fmt::format(u" {}", ret);
			displaymsg->DisplayStringName(std::move(msg), GUIColors::WHITE, this);
		}
		lastSave.prevType = type;
		lastSave.prevRoll = ret;
		return ret > (int) GetStat(savingThrows[type]);
	}

	int roll = ret;
	// NOTE: we use GetStat, assuming the stat save bonus can never be negated like some others
	int save = GetStat(savingThrows[type]);
	// intentionally not adding luck, which seems to have been handled separately
	// eg. 11hfamlk.itm uses an extra opcode for the saving throw bonus
	ret = roll + save + modifier;
	assert(fx);
	int spellLevel = fx->SpellLevel;
	int saveBonus = fx->SavingThrowBonus;
	int saveDC = 10 + spellLevel + saveBonus;

	// handle special bonuses (eg. vs poison, which doesn't have a separate stat any more)
	// same hardcoded list as in the original
	if (savingThrows[type] == IE_SAVEFORTITUDE && fx->Opcode == 25) {
		if (BaseStats[IE_RACE] == 4 /* DWARF */) ret += 2;
		if (HasFeat(FEAT_SNAKE_BLOOD)) ret += 2;
		if (HasFeat(FEAT_RESIST_POISON)) ret += 4;
	}

	// the original had a sourceType == TRIGGER check, but we handle more than ST_TRIGGER
	Scriptable *caster = area ? area->GetScriptableByGlobalID(fx->CasterID) : nullptr;
	if (savingThrows[type] == IE_SAVEREFLEX && caster && caster->Type != ST_ACTOR) {
		// loop over all classes and add TRAPSAVE.2DA values to the bonus
		for (int cls = 0; cls < ISCLASSES; cls++) {
			int level = GetClassLevel(cls);
			if (!level) continue;
			ret += gamedata->GetTrapSaveBonus(level, classesiwd2[cls]);
		}
	}

	if (savingThrows[type] == IE_SAVEWILL) {
		// aura of courage
		if (Modified[IE_EA] < EA_GOODCUTOFF && fx->SourceRef != "SPWI420" && area) {
			// look if an ally paladin of at least level 2 is near
			std::vector<Actor *> neighbours = area->GetAllActorsInRadius(Pos, GA_NO_LOS|GA_NO_DEAD|GA_NO_UNSCHEDULED|GA_NO_ENEMY|GA_NO_NEUTRAL|GA_NO_SELF, 10);
			for (const Actor *ally : neighbours) {
				if (ally->GetPaladinLevel() >= 2 && !ally->CheckSilenced()) {
					ret += 4;
					break;
				}
			}
		}

		if (fx->Opcode == 24 && BaseStats[IE_RACE] == 5 /* HALFLING */) ret += 2;
		if (GetSubRace() == 0x20001 /* DROW */) ret += 2;

		// Tyrant's dictum for clerics of Bane
		const Actor* cleric = Scriptable::As<Actor>(caster);
		if (cleric && cleric->GetClericLevel() && BaseStats[IE_KIT] & 0x200000) {
			// the original limited this to domain spells, but that's pretty lame
			saveDC += 1;
		}
	}

	// general bonuses
	if (Modified[IE_EA] != EA_PC) ret += DifficultySaveMod;
	// (half)elven resistance to enchantment, gnomish to illusions and dwarven to spells
	if ((BaseStats[IE_RACE] == 2 || BaseStats[IE_RACE] == 3) && fx->PrimaryType == 4) ret += 2;
	if (BaseStats[IE_RACE] == 6 && fx->PrimaryType == 5) ret += 2;
	if (BaseStats[IE_RACE] == 4 && fx->Resistance <= FX_CAN_RESIST_CAN_DISPEL) ret += 2;
	// monk's clear mind and mage specialists
	if (GetMonkLevel() >= 3 && fx->PrimaryType == 4) ret += 2;
	if (GetMageLevel() && (1 << (fx->PrimaryType + 5)) & BaseStats[IE_KIT]) ret += 2;

	// handle animal taming last
	// must roll a Will Save of 5 + player's total skill or higher to save
	if (fx->SourceRef != "SPIN108" && fx->Opcode == 5) {
		saveDC = 5;
		const Actor *caster = core->GetGame()->GetActorByGlobalID(fx->CasterID);
		if (caster) {
			saveDC += caster->GetSkill(IE_ANIMALS);
		}
	}

	ret = AdjustSaveVsSchool(ret, fx->PrimaryType, sfx);

	if (ret > saveDC) {
		// ~Saving throw result: (d20 + save + bonuses) %d + %d  + %d vs. (10 + spellLevel + saveMod)  10 + %d + %d - Success!~
		displaymsg->DisplayRollStringName(ieStrRef::ROLL22, GUIColors::LIGHTGREY, this, roll, save, modifier, spellLevel, saveBonus);
		return true;
	} else {
		// ~Saving throw result: (d20 + save + bonuses) %d + %d  + %d vs. (10 + spellLevel + saveMod)  10 + %d + %d - Failed!~
		displaymsg->DisplayRollStringName(ieStrRef::ROLL23, GUIColors::LIGHTGREY, this, roll, save, modifier, spellLevel, saveBonus);
		return false;
	}
}

/** implements a generic opcode function, modify modified stats
returns the change
*/
int Actor::NewStat(unsigned int StatIndex, stat_t ModifierValue, ieDword ModifierType)
{
	int oldmod = Modified[StatIndex];

	switch (ModifierType) {
		case MOD_ADDITIVE:
			//flat point modifier
			SetStat(StatIndex, Modified[StatIndex]+ModifierValue, 1);
			break;
		case MOD_ABSOLUTE:
			//straight stat change
			SetStat(StatIndex, ModifierValue, 1);
			break;
		case MOD_PERCENT:
			//percentile
			SetStat(StatIndex, BaseStats[StatIndex] * ModifierValue / 100, 1);
			break;
		case MOD_MULTIPLICATIVE:
			SetStat(StatIndex, BaseStats[StatIndex] * ModifierValue, 1);
			break;
		case MOD_DIVISIVE:
			if (ModifierValue == 0) {
				Log(ERROR, "Actor", "Invalid modifier value (0) passed to NewStat: {} ({})!", ModifierType, fmt::WideToChar{GetName()});
				break;
			}
			SetStat(StatIndex, BaseStats[StatIndex] / ModifierValue, 1);
			break;
		case MOD_MODULUS:
			if (ModifierValue == 0) {
				Log(ERROR, "Actor", "Invalid modifier value (0) passed to NewStat: {} ({})!", ModifierType, fmt::WideToChar{GetName()});
				break;
			}
			SetStat(StatIndex, BaseStats[StatIndex] % ModifierValue, 1);
			break;
		case MOD_LOGAND:
			SetStat(StatIndex, BaseStats[StatIndex] && ModifierValue, 1);
			break;
		case MOD_LOGOR:
			SetStat(StatIndex, BaseStats[StatIndex] || ModifierValue, 1);
			break;
		case MOD_BITAND:
			SetStat(StatIndex, BaseStats[StatIndex] & ModifierValue, 1);
			break;
		case MOD_BITOR:
			SetStat(StatIndex, BaseStats[StatIndex] | ModifierValue, 1);
			break;
		case MOD_INVERSE:
			SetStat(StatIndex, !BaseStats[StatIndex], 1);
			break;
		default:
			Log(ERROR, "Actor", "Invalid modifier type passed to NewStat: {} ({})!", ModifierType, fmt::WideToChar{GetName()});
	}
	return Modified[StatIndex] - oldmod;
}

int Actor::NewBase(unsigned int StatIndex, stat_t ModifierValue, ieDword ModifierType)
{
	int oldmod = BaseStats[StatIndex];

	switch (ModifierType) {
		case MOD_ADDITIVE:
			//flat point modifier
			SetBase(StatIndex, BaseStats[StatIndex]+ModifierValue);
			break;
		case MOD_ABSOLUTE:
			//straight stat change
			SetBase(StatIndex, ModifierValue);
			break;
		case MOD_PERCENT:
			//percentile
			SetBase(StatIndex, BaseStats[StatIndex] * ModifierValue / 100);
			break;
		case MOD_MULTIPLICATIVE:
			SetBase(StatIndex, BaseStats[StatIndex] * ModifierValue);
			break;
		case MOD_DIVISIVE:
			if (ModifierValue == 0) {
				Log(ERROR, "Actor", "Invalid modifier value (0) passed to NewBase: {} ({})!", ModifierType, fmt::WideToChar{GetName()});
				break;
			}
			SetBase(StatIndex, BaseStats[StatIndex] / ModifierValue);
			break;
		case MOD_MODULUS:
			if (ModifierValue == 0) {
				Log(ERROR, "Actor", "Invalid modifier value (0) passed to NewBase: {} ({})!", ModifierType, fmt::WideToChar{GetName()});
				break;
			}
			SetBase(StatIndex, BaseStats[StatIndex] % ModifierValue);
			break;
		case MOD_LOGAND:
			SetBase(StatIndex, BaseStats[StatIndex] && ModifierValue);
			break;
		case MOD_LOGOR:
			SetBase(StatIndex, BaseStats[StatIndex] || ModifierValue);
			break;
		case MOD_BITAND:
			SetBase(StatIndex, BaseStats[StatIndex] & ModifierValue);
			break;
		case MOD_BITOR:
			SetBase(StatIndex, BaseStats[StatIndex] | ModifierValue);
			break;
		case MOD_INVERSE:
			SetBase(StatIndex, !BaseStats[StatIndex]);
			break;
		default:
			Log(ERROR, "Actor", "Invalid modifier type passed to NewBase: {} ({})!", ModifierType, fmt::WideToChar{GetName()});
	}
	return BaseStats[StatIndex] - oldmod;
}

void Actor::Interact(int type) const
{
	EnumIterator<Verbal> start;
	int count;
	bool queue = false;

	switch(type&0xff) {
		case I_INSULT:
			start = EnumIterator<Verbal>(Verbal::Insult);
			break;
		case I_COMPLIMENT:
			start = EnumIterator<Verbal>(Verbal::Compliment);
			break;
		case I_SPECIAL:
			start = EnumIterator<Verbal>(Verbal::Special);
			break;
		case I_INSULT_RESP:
			start = EnumIterator<Verbal>(Verbal::Resp2Insult);
			queue = true;
			break;
		case I_COMPL_RESP:
			start = EnumIterator<Verbal>(Verbal::Resp2Compliment);
			queue = true;
			break;
		default:
			return;
	}
	if (type&0xff00) {
		//PST style fixed slots
		start = start + (((type & 0xff00) >> 8) - 1);
		count = 1;
	} else {
		//BG1 style random slots
		count = 3;
	}
	VerbalConstant(*start, count, queue ? DS_QUEUE : 0);
}

ieStrRef Actor::GetVerbalConstant(Verbal index) const
{
	if (index >= Verbal::count) {
		return ieStrRef::INVALID;
	}

	return StrRefs[index];
}

ieStrRef Actor::GetVerbalConstant(Verbal start, int count) const
{
	auto beg = EnumIterator<Verbal>(start);
	auto end = beg + count;
	end = std::find_if(beg, end, [this](Verbal vc) {
		return GetVerbalConstant(vc) == ieStrRef::INVALID;
	});

	count = beg.distance(end);
	if (count > 0) {
		return GetVerbalConstant(*(beg + RAND(0, count - 1)));
	}
	return ieStrRef::INVALID;
}

bool Actor::VerbalConstant(Verbal start, int count, int flags) const
{
	assert(count > 0);
	start = VCMap[start];
	if (start != Verbal::Die) {
		//can't talk when dead
		if (Modified[IE_STATE_ID] & (STATE_CANTLISTEN)) return false;
	}

	flags ^= DS_CONSOLE|DS_SPEECH|DS_CIRCLE;

	//If we are main character (has SoundSet) we have to check a corresponding wav file exists
	bool found = false;
	if (PCStats && !PCStats->SoundSet.IsEmpty()) {
		ResRef soundRef;
		do {
			count--;
			int firstVB = static_cast<int>(start);
			GetVerbalConstantSound(soundRef, Verbal(firstVB + count));
			auto soundFolder = GetSoundFolder(1, soundRef);
			if (gamedata->Exists(soundFolder, IE_WAV_CLASS_ID, true) || gamedata->Exists(soundFolder, IE_OGG_CLASS_ID, true)) {
				DisplayStringCoreVC((Scriptable*) this, Verbal(firstVB + RAND(0, count)), flags | DS_CONST | DS_RESOLVED);
				found = true;
				break;
			}
		} while (count > 0);
	} else { //If we are anyone else we have to check there is a corresponding strref
		ieStrRef str = GetVerbalConstant(start, count);
		if (str != ieStrRef(-1)) {
			DisplayStringCore((Scriptable *) this, str, flags);
			found = true;
		}
	}
	return found;
}

void Actor::DisplayStringOrVerbalConstant(HCStrings str, Verbal vcStat, int vcCount) const
{
	ieStrRef strref = DisplayMessage::GetStringReference(str);
	if (strref != ieStrRef::INVALID) {
		DisplayStringCore((Scriptable *) this, strref, DS_CONSOLE|DS_CIRCLE);
	} else {
		VerbalConstant(vcStat, vcCount);
	}
}

tick_t Actor::ReactToDeath(const ieVariable& deadname) const
{
	AutoTable tm = gamedata->LoadTable("death");
	if (!tm) {
		// iwds don't have joinable npcs, so they don't ship a death.2da
		VerbalConstant(Verbal::React, gamedata->GetVBData("SPECIAL_COUNT"), DS_QUEUE);
		return 0;
	}
	// lookup value based on died's scriptingname and ours
	// if value is 0 - use reactdeath
	// if value is 1 - use reactspecial
	// if value is string - use playsound instead (pst)
	std::string value = tm->QueryField(scriptName, deadname);
	if (value[0] == '0') {
		VerbalConstant(Verbal::React, 1, DS_QUEUE);
		return 0;
	} else if (value[0] == '1') {
		VerbalConstant(Verbal::ReactSpecific, 1, DS_QUEUE);
		return 0;
	}

	// there can be several entries to choose from, eg.: NOR103,NOR104,NOR105
	auto elements = Explode<std::string, std::string>(value);
	size_t count = elements.size();
	if (count <= 0) return 0;

	int choice = core->Roll(1, int(count), -1);
	ResRef resRef = elements[choice];

	tick_t len = 0;
	unsigned int channel = SFX_CHAN_CHAR0 + InParty - 1;
	core->GetAudioDrv()->PlayRelative(resRef, channel, &len);
	return len;
}

static int CheckInteract(const ieVariable& talker, const ieVariable& target)
{
	AutoTable interact = gamedata->LoadTable("interact");
	if (!interact)
		return I_NONE;
	const char *value = interact->QueryField(talker, target).c_str();
	if (!value)
		return I_NONE;

	int offset = 0;
	int x = 0;
	int length = int(strlen(value));

	if (length > 1) { // PST
		//we round the length up, so the last * will be also chosen
		x = core->Roll(1, (length + 1) / 2, -1) * 2;
		//convert '1', '2' and '3' to 0x100,0x200,0x300 respectively, all the rest becomes 0
		//it is no problem if we hit the zero terminator in case of an odd length
		offset = value[x + 1] - '0';
		if (offset > 3) offset = 0;
		offset <<= 8;
	}

	switch(value[x]) {
		case '*':
			return I_DIALOG;
		case 's':
			return offset + I_SPECIAL;
		case 'c':
			return offset + I_COMPLIMENT;
		case 'i':
			return offset + I_INSULT;
		case 'I':
			return offset + I_INSULT_RESP;
		case 'C':
			return offset + I_COMPL_RESP;
		default:
			break;
	}
	return I_NONE;
}

void Actor::HandleInteractV1(const Actor *target)
{
	objects.LastTalker = target->GetGlobalID();
	std::string interAction = fmt::format("Interact(\"{}\")", target->GetScriptName());
	AddAction(GenerateAction(std::move(interAction)));
}

bool Actor::GetPartyComment(const Actor* target)
{
	// V1 interact
	if (core->HasFeature(GFFlags::RANDOM_BANTER_DIALOGS)) {
		HandleInteractV1(target);
		return true;
	}

	int type = CheckInteract(scriptName, target->GetScriptName());
	if (type == I_NONE) return false;
	// V2 interact - banter dialog interaction
	if (type == I_DIALOG) {
		objects.LastTalker = target->GetGlobalID();
		Action* action = GenerateActionDirect("Interact([-1])", target);
		assert(action);
		AddActionInFront(action);
		return true;
	}

	// simplified interact
	Interact(type);
	switch (type) {
		case I_COMPLIMENT:
			target->Interact(I_COMPL_RESP);
			break;
		case I_INSULT:
			target->Interact(I_INSULT_RESP);
			break;
	}
	return true;
}

//call this only from gui selects
void Actor::PlaySelectionSound(bool force)
{
	playedCommandSound = false;
	// pst uses a slider in lieu of buttons, so the frequency value is off by 1
	unsigned int frequency = sel_snd_freq + pstflags;
	if (force || (!pstflags && frequency > 2)) frequency = 5;
	switch (frequency) {
		case 1:
			return;
		case 2:
			if (RAND(1, 100) > 20) return;
			break;
		// pst-only
		case 3:
			if (RAND(1, 100) > 50) return;
			break;
		case 4:
			if (RAND(1, 100) > 80) return;
			break;
		default:;
	}

	//drop the rare selection comment 5% of the time
	bool found = false;
	static int rareSelectChance = gamedata->GetMiscRule("RARE_SELECT_CHANCE");
	if (InParty && RAND(1, 100) <= rareSelectChance) {
		//rare select on main character for BG1 won't work atm
		int numRareSelects = gamedata->GetVBData("RARE_SELECT_SOUNDS");
		found = VerbalConstant(Verbal::SelectRare, numRareSelects, DS_CIRCLE);
	} else {
		// in some games there are fewer soundset slots/files than the VBs
		if (PCStats && !PCStats->SoundSet.IsEmpty()) {
			static int numSelects = gamedata->GetVBData("MC_SELECT_SOUNDS");
			found = VerbalConstant(Verbal::Select, numSelects, DS_CIRCLE);
		} else {
			int numSelects = gamedata->GetVBData("SELECT_SOUNDS");
			found = VerbalConstant(Verbal::Select, numSelects, DS_CIRCLE);
		}
	}

	// if nothing was found, fall back to 2da/ini sounds
	if (!found) {
		ResRef sound;
		GetSoundFromFile(sound, Verbal::Select);
		core->GetAudioDrv()->Play(sound, SFX_CHAN_MONSTER, Pos);
	}
}

void Actor::PlayWarCry(int range) const
{
	if (!war_cries) return;

	bool found = VerbalConstant(Verbal::BattleCry, range, DS_CIRCLE);
	// for monsters also try their 2da/ini file sounds
	if (!found && !InParty) {
		ResRef sound;
		GetSoundFromFile(sound, Verbal::BattleCry);
		core->GetAudioDrv()->Play(sound, SFX_CHAN_MONSTER, Pos);
	}
}

//call this when a PC receives a command from GUI
void Actor::CommandActor(Action* action, bool clearPath)
{
	ClearActions(); // stop what you were doing
	if (clearPath) ClearPath(true);
	AddAction(action); // now do this new thing

	// pst uses a slider in lieu of buttons, so the frequency value is off by 1
	switch (cmd_snd_freq + pstflags) {
		case 1:
			return;
		case 2:
			if (playedCommandSound) return;
			playedCommandSound = true;
			// intentional fallthrough
		case 3:
			//PST has 4 states and rare sounds
			if (pstflags && RAND(1, 100) > 50) return;
			break;
		case 4:
			if (pstflags && RAND(1, 100) > 80) return;
			break;
		default:;
	}

	if (core->GetFirstSelectedPC(false) == this) {
		// bg2 uses up the traditional space for rare select sound slots for more action (command) sounds
		int numCommands = gamedata->GetVBData("COMMAND_COUNT");
		VerbalConstant(Verbal::Command, numCommands, DS_CIRCLE);
	}
}

//Generates an idle action (party banter, area comment, bored)
void Actor::IdleActions(bool nonidle)
{
	//do we have an area
	const Map *map = GetCurrentArea();
	if (!map) return;
	//and not in panic
	if (panicMode!=PANIC_NONE) return;

	const Game *game = core->GetGame();
	//there is no combat
	if (game->CombatCounter) {
		ResetCommentTime();
		return;
	}

	//and they are on the current area
	if (map!=game->GetCurrentArea()) return;

	//don't mess with cutscenes
	if (core->InCutSceneMode()) {
		ResetCommentTime();
		return;
	}

	//only party [N]PCs talk but others might play existence sounds
	if (!InParty) {
		PlayExistenceSounds();
		return;
	}

	if (!nonidle && !InMove() && !Immobile()) {
		// display idle animation
		int x = RAND(0, 24);
		if (!x && (GetStance() == IE_ANI_AWAKE)) {
			SetStance(IE_ANI_HEAD_TURN);
		}
	}
}

void Actor::PlayExistenceSounds()
{
	//only non-joinable chars can have existence sounds
	if (Persistent()) return;

	const Game *game = core->GetGame();
	ieDword time = game->GameTime;
	if (time/nextComment > 1) { // first run, not adjusted for game time yet
		nextComment += time;
	}

	if (nextComment >= time) return;

	ieDword delay = Modified[IE_EXISTANCEDELAY];
	if (delay == (ieDword) -1) return;
	if (delay == 0) {
		delay = VOODOO_EXISTENCE_DELAY_DEFAULT;
	}

	auto audio = core->GetAudioDrv();
	Point listener = audio->GetListenerPos();
	if (nextComment && !Immobile() && WithinAudibleRange(this, listener)) {
		//setup as an ambient
		ieStrRef strref = GetVerbalConstant(Verbal::Existence1, 5);
		if (strref == ieStrRef::INVALID) {
			nextComment = time + RAND(delay * 1 / 4, delay * 7 / 4);
			return;
		}

		StringBlock sb = core->strings->GetStringBlock(strref);
		if (sb.Sound.IsEmpty()) {
			nextComment = time + RAND(delay * 1 / 4, delay * 7 / 4);
			return;
		}

		ieDword vol = core->GetVariable("Volume Ambients", 100);
		int stream = audio->SetupNewStream(Pos.x, Pos.y, 0, ieWord(vol), true, 50); // REFERENCE_DISTANCE
		if (stream != -1) {
			tick_t audioLength = audio->QueueAmbient(stream, sb.Sound);
			if (audioLength > 0) {
				SetAnimatedTalking(audioLength);
			}
			audio->ReleaseStream(stream, false);
		}
	}

	nextComment = time + RAND(delay*1/4, delay*7/4);
}

static void ForceOverrideAction(Actor* actor, std::string actionString)
{
	Action* action = GenerateAction(std::move(actionString));
	assert(action);
	// the original was as agressive, clearing the queue and stopping movement
	actor->Stop();
	actor->AddAction(action);
}

static bool CheckCharmOverride(Actor* actor)
{
	if (!(actor->GetStat(IE_STATE_ID) & STATE_CHARMED)) return false;
	if (actor->GetBase(IE_EA) > EA_GOODCUTOFF) return false;
	if (actor->GetStat(IE_EA) != EA_CHARMEDPC) return false; // the original apparently didn't use EA_CHARMEDPC
	// already running AttackReevaluate?
	// the original ignored this condition for IE_EA > EA_GOODCUTOFF!?
	constexpr unsigned short AttackReevaluateID = 134;
	if (actor->GetCurrentAction() && actor->GetCurrentAction()->actionID == AttackReevaluateID) return false;

	const Effect* charm = actor->fxqueue.HasEffect(fx_set_charmed_state_ref);
	if (!charm) return false;

	// skip regular charm
	switch (charm->Parameter2) {
		case 2:
		case 3:
		case 5:
		case 1002:
		case 1003:
		case 1005:
			ForceOverrideAction(actor, "AttackReevaluate([GOODCUTOFF],10)");
			// EEs also marked a field as true to render the weapon over the portrait for slightly
			// longer than the action itself (m_bJustAttacked in Bubb's dump)
			return true;
		default:
			break;
	}
	return false;
}

static bool CheckConfusionOverride(Actor* actor)
{
	if (!(actor->GetStat(IE_STATE_ID) & STATE_CONFUSED)) return false;

	std::string actionString;
	actionString.reserve(35);
	switch (RAND(1, 3)) {
		case 1:
			// ees, maybe vanilla, called "GroupAttack([ANYONE])"
			// that's the same as Attack due to the param passed
			// HACK: replace with [0] (ANYONE) once we support that (Nearest matches Sender like in the original)
			if (RandomFlip()) {
				actionString = "Attack(NearestEnemyOf(Myself))";
			} else {
				actionString = "Attack([PC])";
			}
			break;
		case 2:
			if (core->HasFeature(GFFlags::HAS_EE_EFFECTS)) {
				actionString = "RunAwayFromNoInterruptNoLeaveArea(Nearest,99999999)";
			} else {
				actionString = "RandomWalk()";
			}
			break;
		default:
			actionString = "NoAction()";
			break;
	}
	ForceOverrideAction(actor, actionString);
	Log(DEBUG, "Actor", "Confusion: added {} at {}", actionString, int(core->GetGame()->GameTime));
	return true;
}

// forced actions that mess with scripting, eg. panic, confusion, berserking
bool Actor::OverrideActions()
{
	// but maybe not: if we're in dialog, cutscene or running an overriden action
	// the caller checks for this already
	// the original didn't check this, but it can be considered a bug
	const Game* game = core->GetGame();
	if (game->StateOverrideFlag && game->StateOverrideTime) return false;

	// domination and dire charm: force the actors to be useful (trivial ai)
	if (CheckCharmOverride(this)) return true;

	// berserking
	if (Modified[IE_STATE_ID] & STATE_BERSERK) {
		if (BaseStats[IE_CHECKFORBERSERK]) {
			BaseStats[IE_CHECKFORBERSERK]--;
		}
		// the original checked for Attack() / Berserk(), which LastTarget handled more generically
		if (Modified[IE_CHECKFORBERSERK] && !objects.LastTarget) {
			ForceOverrideAction(this, "Berserk()");
			return true;
		}
	}

	// each round also re-confuse the actor
	// use the combat round size as the original;  also skald song duration matches it
	int roundFraction = (game->GameTime - roundTime) % GetAdjustedTime(core->Time.attack_round_size);
	if (!roundFraction && CheckConfusionOverride(this)) return true;

	// feeblemind
	if (Modified[IE_STATE_ID] & STATE_FEEBLE) {
		ForceOverrideAction(this, "NoAction()");
	}

	// the original was adding NoAction in one more odd case that would break script processing
	// and for (initially) enemies no longer charmed, confused and feebleminded, which is redundant

	return false;
}

void Actor::Panic(const Scriptable *attacker, int panicmode)
{
	if (GetStat(IE_STATE_ID)&STATE_PANIC) {
		Log(DEBUG, "Actor", "Already panicked!");
		//already in panic
		return;
	}
	if (InParty) core->GetGame()->SelectActor(this, false, SELECT_NORMAL);
	VerbalConstant(Verbal::Panic, gamedata->GetVBData("SPECIAL_COUNT"));

	Action *action;
	if (panicmode == PANIC_RUNAWAY && (!attacker || attacker->Type!=ST_ACTOR)) {
		panicmode = PANIC_RANDOMWALK;
	}

	switch(panicmode) {
	case PANIC_RUNAWAY:
		action = GenerateActionDirect("RunAwayFromNoInterrupt([-1])", attacker);
		SetBaseBit(IE_STATE_ID, STATE_PANIC, true);
		break;
	case PANIC_RANDOMWALK:
		action = GenerateAction( "RandomWalk()" );
		SetBaseBit(IE_STATE_ID, STATE_PANIC, true);
		break;
	case PANIC_BERSERK:
		action = GenerateAction( "Berserk()" );
		BaseStats[IE_CHECKFORBERSERK]=3;
		//SetBaseBit(IE_STATE_ID, STATE_BERSERK, true);
		break;
	default:
		return;
	}
	if (action) {
		AddActionInFront(action);
	} else {
		Log(ERROR, "Actor", "Cannot generate panic action");
	}
}

void Actor::SetMCFlag(ieDword arg, BitOp op)
{
	ieDword flags = BaseStats[IE_MC_FLAGS];
	SetBits(flags, arg, op);
	SetBase(IE_MC_FLAGS, flags);
}

void Actor::DialogInterrupt() const
{
	//if dialoginterrupt was set, no verbal constant
	if ( Modified[IE_MC_FLAGS]&MC_NO_TALK)
		return;

	/* this part is unsure */
	if (Modified[IE_EA]>=EA_EVILCUTOFF) {
		VerbalConstant(Verbal::Hostile);
	} else if (TalkCount) {
		VerbalConstant(Verbal::Dialog);
	} else {
		VerbalConstant(Verbal::InitialMeet);
	}
}

void Actor::GetHit(int damage, bool killingBlow)
{
	if (!Immobile() && !(InternalFlags & IF_REALLYDIED) && !killingBlow) {
		SetStance( IE_ANI_DAMAGE );
		static int beingHitCount = gamedata->GetVBData("BEING_HIT_COUNT");
		if (!VerbalConstant(Verbal::Damage, beingHitCount)) {
			ResRef sound;
			GetSoundFromFile(sound, Verbal::Damage);
			core->GetAudioDrv()->Play(sound, SFX_CHAN_MONSTER, Pos);
		}
	}

	if (Modified[IE_STATE_ID]&STATE_SLEEP) {
		if (Modified[IE_EXTSTATE_ID]&EXTSTATE_NO_WAKEUP || HasSpellState(SS_NOAWAKE)) {
			return;
		}
		Effect *fx = EffectQueue::CreateEffect(fx_cure_sleep_ref, 0, 0, FX_DURATION_INSTANT_PERMANENT);
		fxqueue.AddEffect(fx);
	}
	if (CheckSpellDisruption(damage)) {
		InterruptCasting = true;
	}
}

// this has no effect in adnd
// iwd2 however has two mechanisms: spell disruption and concentration checks:
// - disruption is checked when a caster is damaged while casting
// - concentration is checked when casting is taking place <= 5' from an enemy
bool Actor::CheckSpellDisruption(int damage) const
{
	if (!objects.LastSpellTarget && objects.LastTargetPos.IsInvalid()) {
		// not casting, nothing to do
		return false;
	}

	const Spell* spl = gamedata->GetSpell(SpellResRef, true);
	if (!spl) return false;
	int spellLevel = spl->SpellLevel;
	gamedata->FreeSpell(spl, SpellResRef, false);

	if (core->HasFeature(GFFlags::SIMPLE_DISRUPTION)) {
		return LuckyRoll(1, 20, 0) < (damage + spellLevel);
	}
	if (!third) {
		return true;
	}

	int roll = core->Roll(1, 20, 0);
	int concentration = GetSkill(IE_CONCENTRATION);
	int bonus = 0;
	// combat casting bonus only applies when injured
	if (HasFeat(FEAT_COMBAT_CASTING) && Modified[IE_MAXHITPOINTS] != Modified[IE_HITPOINTS]) {
		bonus += 4;
	}
	// ~Spell Disruption check (d20 + Concentration + Combat Casting bonus) %d + %d + %d vs. (10 + damageTaken + spellLevel)  = 10 + %d + %d.~
	if (GameScript::ID_ClassMask(this, 0x6ee)) { // 0x6ee == CLASSMASK_GROUP_CASTERS
		// no spam for noncasters
		displaymsg->DisplayRollStringName(ieStrRef::ROLL19, GUIColors::LIGHTGREY, this, roll, concentration, bonus, damage, spellLevel);
	}
	bool failed = (roll + concentration + bonus) <= (10 + damage + spellLevel);
	return failed;
}

bool Actor::HandleCastingStance(const ResRef& spellResRef, bool deplete, bool instant)
{
	if (deplete && !spellbook.HaveSpell(spellResRef, HS_DEPLETE)) {
		SetStance(IE_ANI_READY);
		return true;
	}
	if (!instant) {
		SetStance(IE_ANI_CAST);
	}
	return false;
}

bool Actor::CheckSilenced() const
{
	if (!(Modified[IE_STATE_ID] & STATE_SILENCED)) return false;
	if (HasFeat(FEAT_SUBVOCAL_CASTING)) return false;
	if (HasSpellState(SS_VOCALIZE)) return false;
	return true;
}

void Actor::CheckCleave()
{
	int cleave = GetFeat(FEAT_CLEAVE);
	//feat level 1 only enables one cleave per round
	if ((cleave==1) && fxqueue.HasEffect(fx_cleave_ref) ) {
		cleave = 0;
	}
	if(cleave) {
		Effect * fx = EffectQueue::CreateEffect(fx_cleave_ref, attackcount, 0, FX_DURATION_INSTANT_LIMITED);
		if (fx) {
			fx->Duration = core->Time.round_sec;
			core->ApplyEffect(fx, this, this);
			// ~Cleave feat adds another level %d attack.~
			// uses the max tohit bonus (tested), but game always displayed "level 1"
			displaymsg->DisplayRollStringName(ieStrRef::ROLL20, GUIColors::LIGHTGREY, this, ToHit.GetTotal());
		}
	}
}

// NOTE: only does the visual part of chunking
static void ChunkActor(Actor* actor)
{
	ieDword gore = core->GetVariable("Gore", 0);
	if (!gore) return;

	// TODO: play chunky animation / particles #128
	actor->SetAnimationID(0x230); // EXPLODING_TORSO
}

//returns actual damage
int Actor::Damage(int damage, int damagetype, Scriptable* hitter, int modtype, int critical, int saveflags, int specialFlags)
{
	//won't get any more hurt
	if (InternalFlags & IF_REALLYDIED) {
		return 0;
	}
	// hidden creatures are immune too, iwd2 Targos palisade attack relies on it (12cspn1a.bcs)
	// same for seagulls and other non-jumpers
	if (Modified[IE_AVATARREMOVAL] || Modified[IE_DONOTJUMP] == DNJ_BIRD) {
		return 0;
	}

	// only pst has special crits and they share the same storage
	if (!pstflags) critical = 0;
	if (!core->HasFeature(GFFlags::HAS_EE_EFFECTS)) specialFlags = 0;

	//add lastdamagetype up ? maybe
	//FIXME: what does original do?
	LastDamageType |= damagetype;

	Actor* act = Scriptable::As<Actor>(hitter);

	switch (modtype) {
	case MOD_ADDITIVE:
		//bonus against creature should only affect additive damages or spells like harm would be deadly
		if (damage && act) {
			damage += act->fxqueue.BonusAgainstCreature(fx_damage_vs_creature_ref, this);
		}
		break;
	case MOD_ABSOLUTE:
		damage = GetBase(IE_HITPOINTS) - damage;
		break;
	case MOD_PERCENT:
		damage = GetStat(IE_MAXHITPOINTS) * damage / 100;
		break;
	default:
		//this shouldn't happen
		Log(ERROR, "Actor", "Invalid damage modifier type!");
		return 0;
	}

	if (GetStat(IE_EXTSTATE_ID) & EXTSTATE_EYE_MAGE) {
		if (damagetype & (DAMAGE_FIRE|DAMAGE_COLD|DAMAGE_ACID|DAMAGE_ELECTRICITY)) {
			fxqueue.RemoveAllEffects(fx_eye_mage_ref);
			spellbook.RemoveSpell(SevenEyes[EYE_MAGE]);
			SetBaseBit(IE_EXTSTATE_ID, EXTSTATE_EYE_MAGE, false);
			damage = 0;
		}
	}

	if (damage && (damagetype == DAMAGE_POISON || !(saveflags & SF_BYPASS_MIRROR_IMAGE))) {
		int mirrorimages = Modified[IE_MIRRORIMAGES];
		if (mirrorimages) {
			if (LuckyRoll(1, mirrorimages + 1, 0) != 1) {
				fxqueue.DecreaseParam1OfEffect(fx_mirrorimage_ref, 1);
				Modified[IE_MIRRORIMAGES]--;
				damage = 0;
			}
		}
	}

	if (!(saveflags&SF_IGNORE_DIFFICULTY) && act) {
		// adjust enemy damage according to difficulty settings:
		// -50%, -25%, 0, 50%, 100%, 150%
		if (act->GetStat(IE_EA) > EA_GOODCUTOFF) {
			int adjustmentPercent = DifficultyDamageMod;
			if (!NoExtraDifficultyDmg || adjustmentPercent < 0) {
				damage += (damage * adjustmentPercent)/100;
			}
		}
	}

	int resisted = 0;
	if (damage) {
		ModifyDamage (hitter, damage, resisted, damagetype);
	}

	if (!(specialFlags & DamageFlags::NoFeedback)) {
		DisplayCombatFeedback(damage, resisted, damagetype, hitter);
	}

	if (damage > 0) {
		// instant chunky death if the actor is petrified or frozen
		bool allowChunking = !Modified[IE_DISABLECHUNKING] && GameDifficulty > DIFF_NORMAL && !Modified[IE_MINHITPOINTS];
		if (Modified[IE_STATE_ID] & (STATE_FROZEN|STATE_PETRIFIED) && allowChunking) {
			damage = 123456; // arbitrarily high for death; won't be displayed
			LastDamageType |= DAMAGE_CHUNKING;
		}
		// chunky death when you're reduced below -10 hp
		if ((ieDword) damage >= Modified[IE_HITPOINTS] + 10 && allowChunking) {
			LastDamageType |= DAMAGE_CHUNKING;
		}
		// mark LastHitter for repeating damage effects (eg. to get xp from melfing trolls)
		if (act && objects.LastHitter == 0) {
			objects.LastHitter = act->GetGlobalID();
		}
	}

	if (BaseStats[IE_HITPOINTS] <= (ieDword) damage) {
		// common fists do normal damage, but cause sleeping for a round instead of death
		if (Modified[IE_MINHITPOINTS] <= 0 && damagetype & DAMAGE_STUNNING) {
			// stack unconsciousness carefully to avoid replaying the stance changing
			ieDword below1hp = (ieDword) damage - BaseStats[IE_HITPOINTS] + 1;
			Effect *sleep = const_cast<Effect*>(fxqueue.HasEffectWithParamPair(fx_sleep_ref, 0, 0)); // FIXME: const_cast
			if (sleep) {
				sleep->Duration += below1hp * 15;
			} else {
				Effect *fx = EffectQueue::CreateEffect(fx_sleep_ref, 0, 0, FX_DURATION_INSTANT_LIMITED);
				fx->Duration = 2 * core->Time.round_sec + below1hp * 15; // 2 rounds + 15s per hp below 1
				core->ApplyEffect(fx, this, this);
			}
			//reduce damage to keep 1 hp
			damage = Modified[IE_HITPOINTS] - 1;
		}
	}

	// are there any other limits on the damage?
	bool cap2Source = specialFlags & DamageFlags::CapToSource;
	bool cap2Target = specialFlags & DamageFlags::CapToTarget;
	bool invertTarget = specialFlags & (DamageFlags::DrainFromSource | DamageFlags::DrainFromSourceNC);
	int chp = (signed) BaseStats[IE_HITPOINTS];
	int casterHP = 0;
	if (act) {
		casterHP = (signed) act->GetBase(IE_HITPOINTS);
		if ((cap2Source && !invertTarget) || (cap2Target && invertTarget)) {
			// Damage inflicted is limited to amount available by target
			// it's repeating our later pcf_hitpoint check, but is important for use with the draining bits
			int minHP = (signed) GetSafeStat(IE_MINHITPOINTS);
			if (minHP && (chp - damage) < minHP) {
				damage = chp - minHP;
			}
		} else if ((cap2Target && !invertTarget) || (cap2Source && invertTarget)) {
			// Damage inflicted is limited to the caster's
			int maxHP = (signed) act->GetSafeStat(IE_MAXHITPOINTS);
			if (damage > (maxHP - casterHP)) {
				damage = maxHP - casterHP;
			}
		}
	}

	// also apply reputation damage if we hurt (but not killed) an innocent
	if (core->HasFeature(GFFlags::DAMAGE_INNOCENT_REP) &&
			Modified[IE_CLASS] == CLASS_INNOCENT &&
			!core->InCutSceneMode() &&
			act && act->GetStat(IE_EA) <= EA_CONTROLLABLE) {

		core->GetGame()->SetReputation(core->GetGame()->Reputation + gamedata->GetReputationMod(1));
	}

	if (damage > 0) {
		//if this kills us, check if attacker could cleave
		bool killed = (damage > chp) && !Modified[IE_MINHITPOINTS];
		if (act && killed) {
			act->CheckCleave();
		}

		// temporarily turn on an extended state, so we don't need to pass an extra param
		// don't bother unsetting, since it will be gone the next tick and a combination of a
		// sleeping target and several damage payloads in the same tick are unlikely
		if (specialFlags & DamageFlags::NoAwake) {
			Modified[IE_EXTSTATE_ID] |= EXTSTATE_NO_WAKEUP;
		}
		GetHit(damage, killed);

		//fixme: implement applytrigger, copy int0 into LastDamage there
		LastDamage = damage;
		AddTrigger(TriggerEntry(trigger_tookdamage, damage)); // FIXME: lastdamager? LastHitter is not set for spell damage
		AddTrigger(TriggerEntry(trigger_hitby, objects.LastHitter, damagetype)); // FIXME: currently lastdamager, should it always be set regardless of damage?

		// impact morale when hp thresholds (50 %, 25 %) are crossed for the first time
		int currentRatio = 100 * chp / (signed) BaseStats[IE_MAXHITPOINTS];
		int newRatio = 100 * (chp + damage) / (signed) BaseStats[IE_MAXHITPOINTS];
		if (ShouldModifyMorale()) {
			if (currentRatio > 50 && newRatio < 25) {
				NewBase(IE_MORALE, (stat_t) -4, MOD_ADDITIVE);
			} else if (currentRatio > 50 && newRatio < 50) {
				NewBase(IE_MORALE, (stat_t) -2, MOD_ADDITIVE);
			} else if (currentRatio > 25 && newRatio < 25) {
				NewBase(IE_MORALE, (stat_t) -2, MOD_ADDITIVE);
			}
		}

		 // TODO: drains currently ignore desired non-cumulativeness
		if (act && specialFlags & (DamageFlags::DrainFromTarget | DamageFlags::DrainFromTargetNC) && !invertTarget) {
			act->SetBase(IE_HITPOINTS, casterHP + damage);
		} else if (act && invertTarget) {
			BaseStats[IE_HITPOINTS] += damage; // should this mode prevent getting hit in the first place?
			act->SetBase(IE_HITPOINTS, casterHP - damage);
		}
	}

	// can be negative if we're healing on 100%+ resistance
	// do this after GetHit, so VB_HURT isn't overriden by VB_DAMAGE, just (dis)played later
	if (damage != 0) {
		NewBase(IE_HITPOINTS, (stat_t) -damage, MOD_ADDITIVE);
		// unstun for that one special stun type
		// run fx_cure_stun_state instead if it turns out not to be enough
		if (HasSpellState(SS_AWAKE)) fxqueue.RemoveAllEffectsWithParam(fx_set_stun_state_ref, 1);
	}

	InternalFlags |= IF_ACTIVE;
	int damagelevel;
	if (damage < 10) {
		damagelevel = 0;
	} else if (damage < 20) { // a guess; impacts what blood bam we play, while elemental damage types are unaffected
		damagelevel = 1;
	} else {
		damagelevel = 2;
	}

	if (damagetype & (DAMAGE_FIRE|DAMAGE_MAGICFIRE)) {
		PlayDamageAnimation(DL_FIRE + damagelevel);
	} else if (damagetype & (DAMAGE_COLD|DAMAGE_MAGICCOLD)) {
		PlayDamageAnimation(DL_COLD + damagelevel);
	} else if (damagetype & DAMAGE_ELECTRICITY) {
		PlayDamageAnimation(DL_ELECTRICITY + damagelevel);
	} else if (damagetype & DAMAGE_ACID) {
		PlayDamageAnimation(DL_ACID + damagelevel);
	} else if (damagetype & (DAMAGE_MAGIC|DAMAGE_DISINTEGRATE)) {
		PlayDamageAnimation(DL_DISINTEGRATE + damagelevel);
	} else {
		if (chp < -10) {
			PlayDamageAnimation(critical<<8);
		} else {
			PlayDamageAnimation(DL_BLOOD + damagelevel);
		}
	}

	if (InParty) {
		if (chp < (signed) Modified[IE_MAXHITPOINTS]/10) {
			core->Autopause(AUTOPAUSE::WOUNDED, this);
		}
		if (damage > 0) {
			core->Autopause(AUTOPAUSE::HIT, this);
			core->SetEventFlag(EF_PORTRAIT);
		}
	}
	return damage;
}

void Actor::DisplayCombatFeedback(unsigned int damage, int resisted, int damagetype, const Scriptable *hitter)
{
	// shortcircuit for disintegration, which wouldn't hit any of the below
	if (damage == 0 && resisted == 0) return;
	// skip in dialogs to avoid Saradush spam in non-pausing ones
	if (core->GetGameControl()->InDialog()) return;

	const Actor* damager = Scriptable::As<Actor>(hitter);
	bool detailed = false;
	String type_name = u"unknown";
	if (DisplayMessage::HasStringReference(HCStrings::DamageDetail1)) { // how and iwd2
		std::multimap<ieDword, DamageInfoStruct>::iterator it;
		it = core->DamageInfoMap.find(damagetype);
		if (it != core->DamageInfoMap.end()) {
			type_name = core->GetString(it->second.strref, STRING_FLAGS::NONE);
		}
		detailed = true;
	}

	auto& tokens = core->GetTokenDictionary();
	if (damage > 0 && resisted != DR_IMMUNE) {
		Log(COMBAT, "Actor", "{} {} damage taken.\n", damage, fmt::WideToChar{type_name});

		if (!core->HasFeedback(FT_STATES)) goto hitsound;

		if (detailed) {
			// 3 choices depending on resistance and boni
			// iwd2 also has two Tortoise Shell (spell) absorption strings
			tokens["TYPE"] = type_name;
			SetTokenAsString("AMOUNT", damage);

			HCStrings strref;
			if (resisted < 0) {
				//Takes <AMOUNT> <TYPE> damage from <DAMAGER> (<RESISTED> damage bonus)
				SetTokenAsString("RESISTED", abs(resisted));
				strref = HCStrings::DamageDetail3;
			} else if (resisted > 0) {
				//Takes <AMOUNT> <TYPE> damage from <DAMAGER> (<RESISTED> damage resisted)
				SetTokenAsString("RESISTED", abs(resisted));
				strref = HCStrings::DamageDetail2;
			} else {
				//Takes <AMOUNT> <TYPE> damage from <DAMAGER>
				strref = HCStrings::DamageDetail1;
			}
			if (damager) {
				tokens["DAMAGER"] = damager->GetName();
			} else {
				// variant without damager
				strref = HCStrings(int(strref) - (int(HCStrings::DamageDetail1) - int(HCStrings::Damage1)));
			}
			displaymsg->DisplayConstantStringName(strref, GUIColors::WHITE, this);
		} else if (core->HasFeature(GFFlags::ONSCREEN_TEXT) ) {
			auto color = GUIColors::WHITE;
			if (InParty) {
				color = GUIColors::RED;
			}
			// eeeh, no token (Damage: x)
			String text = fmt::format(u"{} {}", core->GetString(ieStrRef::DAMAGE), damage);
			overHead.SetText(std::move(text), true, true, displaymsg->GetColor(color));
		} else if (!DisplayMessage::HasStringReference(HCStrings::Damage2) || !damager) {
			// bg1 and iwd
			// or any traps or self-infliction (also for bg1)
			// construct an i18n friendly "Damage Taken (damage)", since there's no token
			String msg = core->GetString(DisplayMessage::GetStringReference(HCStrings::Damage1), STRING_FLAGS::NONE);
			String dmg = fmt::format(u" ({})", damage);
			displaymsg->DisplayStringName(msg + dmg, GUIColors::WHITE, this);
		} else { //bg2
			//<DAMAGER> did <AMOUNT> damage to <DAMAGEE>
			tokens["DAMAGEE"] = GetName();
			// wipe the DAMAGER token, so we can color it
			tokens["DAMAGER"] = String{};
			SetTokenAsString("AMOUNT", damage);
			displaymsg->DisplayConstantStringName(HCStrings::Damage2, GUIColors::WHITE, hitter);
		}
	} else if (resisted == DR_IMMUNE && damager) {
		Log(COMBAT, "Actor", "is immune to damage type {} from {}.\n", fmt::WideToChar{type_name}, fmt::WideToChar{damager->GetName()});
		if (detailed) {
			//<DAMAGEE> was immune to my <TYPE> damage
			tokens["DAMAGEE"] = GetName();
			tokens["TYPE"] = type_name;
			displaymsg->DisplayConstantStringName(HCStrings::DamageImmunity, GUIColors::WHITE, hitter);
		} else if (DisplayMessage::HasStringReference(HCStrings::DamageImmunity) && DisplayMessage::HasStringReference(HCStrings::Damage1)) {
			// bg2
			//<DAMAGEE> was immune to my damage.
			tokens["DAMAGEE"] = GetName();
			displaymsg->DisplayConstantStringName(HCStrings::DamageImmunity, GUIColors::WHITE, hitter);
		} // else: other games don't display anything
	} else if (resisted == DR_IMMUNE) {
		Log(COMBAT, "Actor", "is immune to damage type: {}.\n", fmt::WideToChar{type_name});
	} else {
		// mirror image or stoneskin: no message
	}

	hitsound:
	//Play hit sounds, for pst, resdata contains the armor level
	const DataFileMgr *resdata = core->GetResDataINI();
	PlayHitSound(resdata, damagetype, false);
}

void Actor::PlayWalkSound()
{
	tick_t thisTime = GetMilliseconds();
	if (thisTime<nextWalk) return;
	int chosenWalkSnd = anims->GetWalkSoundCount();
	if (!chosenWalkSnd) return;

	chosenWalkSnd = core->Roll(1, chosenWalkSnd, -1);
	ResRef walkSound = anims->GetWalkSound();
	ResRef Sound = area->ResolveTerrainSound(walkSound, Pos);
	if (Sound.IsEmpty()) Sound = walkSound;
	if (Sound.IsEmpty() || IsStar(Sound)) return;

	ResRef soundBase = Sound;
	char suffix = 0;
	uint8_t l = Sound.length();
	/* IWD1, HOW, IWD2 sometimes append numbers here, not letters. */
	if (core->HasFeature(GFFlags::SOUNDFOLDERS) && Sound.BeginsWith("FS_")) {
		suffix = char(chosenWalkSnd + 0x31);
	} else if (chosenWalkSnd) {
		suffix = char(chosenWalkSnd + 0x60); // 'a'-'g'
	}
	if (l < 8 && suffix != 0) {
		Sound.Format("{:.8}{}", soundBase, suffix);
	}

	tick_t len = 0;
	unsigned int channel = InParty ? SFX_CHAN_WALK_CHAR : SFX_CHAN_WALK_MONSTER;
	core->GetAudioDrv()->Play(Sound, channel, Pos, 0, &len);
	nextWalk = ieDword(thisTime + len);
}

// guesses from audio:               bone  chain studd leather splint none other plate
static const char* const armor_types[8] = { "BN", "CH", "CL", "LR", "ML", "MM", "MS", "PT" };
static const char* const dmg_types[5] = { "PC", "SL", "BL", "ML", "RK" };

//Play hit sounds (HIT_0<dtype><armor>)
//IWDs have H_<dmgtype>_<armor> (including level from 1 to max 5), eg H_ML_MM3
void Actor::PlayHitSound(const DataFileMgr *resdata, int damagetype, bool suffix) const
{
	int type;
	bool levels = true;

	// SOUND column in ee dmgtypes.2da
	switch(damagetype) {
		case DAMAGE_PIERCING: type = 1; break; //piercing
		case DAMAGE_SLASHING: type = 2; break; //slashing
		case DAMAGE_CRUSHING: type = 3; break; //crushing
		case DAMAGE_MISSILE: type = 4; break;  //missile
		case DAMAGE_ELECTRICITY: type = 5; levels = false; break; //electricity
		case DAMAGE_COLD:
		case DAMAGE_MAGICCOLD:
			type = 6;
			levels = false;
			break;
		case DAMAGE_MAGIC: type = 7; levels = false; break;
		case DAMAGE_STUNNING: type = -3; break;
		case DAMAGE_FIRE: // the only odd one out
		case DAMAGE_MAGICFIRE:
			core->GetAudioDrv()->Play("FIRE", SFX_CHAN_HITS, Pos);
			return;
		default: return;                       //other
	}


	int armor = 0;
	if (resdata) {
		unsigned int animid=BaseStats[IE_ANIMATION_ID];
		if(core->HasFeature(GFFlags::ONE_BYTE_ANIMID)) {
			animid&=0xff;
		}

		const std::string& section = fmt::format("{}", animid);
		if (type<0) {
			type = -type;
		} else {
			armor = resdata->GetKeyAsInt(section, "armor", 0);
		}
		if (armor<0 || armor>35) return;
	} else {
		//hack for stun (always first armortype)
		if (type<0) {
			type = -type;
		} else {
			armor = Modified[IE_ARMOR_TYPE];
		}
	}

	ResRef Sound;
	if (core->HasFeature(GFFlags::IWD2_SCRIPTNAME)) {
		// TODO: RE and unhardcode, especially the "armor" mapping
		// no idea what RK stands for, so use it for everything else
		if (type > 5) type = 5;
		armor = Modified[IE_ARMOR_TYPE];
		switch (armor) {
			case IE_ANI_NO_ARMOR: armor = 5; break;
			case IE_ANI_LIGHT_ARMOR: armor = core->Roll(1, 2, 1); break;
			case IE_ANI_MEDIUM_ARMOR: armor = 1; break;
			case IE_ANI_HEAVY_ARMOR: armor = 7; break;
			default: armor = 6; break;
		}

		Sound.Format("H_{}_{}{}", dmg_types[type - 1], armor_types[armor], RAND(1, 3));
	} else {
		if (levels) {
			Sound.Format("HIT_0{}{:c}{:c}", type, armor + 'A', suffix ? '1' : 0);
		} else {
			Sound.Format("HIT_0{}{:c}", type, suffix ? '1' : 0);
		}
	}
	core->GetAudioDrv()->Play(Sound, SFX_CHAN_HITS, Pos);
}

// Play swing sounds
// <wtype><n> in bgs, but it gets quickly complicated with iwds (SW_<wname>) and pst, which add a bunch of exceptions
// ... so they're just fully stored in itemsnd.2da
// iwds also have five sounds of hitting armor (SW_SWD01) that we ignore
// pst has a lot of duplicates (1&3&6, 2&5, 8&9, 4, 7, 10) and little variation (all 6 entries for 8 are identical)
//
// Overall, the sounds on attack are from:
// - the CRE sound slots (VerbalConstant): VB_ATTACK x4, VB_BATTLE_CRY x5
//   - the player customizable soundset mapped to the same slots
// - the animation 2da/ini with sounds per stance (shoot, and the 3 melee types)
// - individual hardcoded item sound overrides (eg. for ankhegs in bg1)
// - actual swing sounds associated to item types (itemsnd.2da)
//
// The order is:
// - battle cry on target acquisition, not each attack or each round
// - sound is looked up in the animation 2da/ini
// - sound is looked up in the CRE and played at the same time if set
//   - if it's also set in the 2da, the CRE choice will be exclusive for selection and battle cry slots
//   - we extend that with damage, die, and the swing stances/slots to mimic Infinity Sounds
//     - this way, creatures with the same animation can still have different combat sounds
// - if there's a hardcoded item override it's played at the same time
//   - like Infinity Sounds we disable them and just use the animation 2das that weren't available for bg1
// - the item type based swing sound is played (item type is unset for the hardcoded cases, so no overlap)
//
void Actor::PlaySwingSound(const WeaponInfo &wi) const
{
	// VBs: there's the 4 attack ones, but there's 5 in the 2da: attack + 3x melee + shoot
	// this extra ATTACK in 2das was always played, together with anything else
	ResRef sound;
	GetSoundFrom2DA(sound, Verbal::Attack0);
	if (!IsStar(sound)) core->GetAudioDrv()->Play(sound, SFX_CHAN_SWINGS, Pos);

	// the CRE attack was played only if the itemtype was 0/misc to avoid clashes with the hardcoded exceptions
	// TobExAL and Infinity Sounds prefer both to be played, so we match that, giving more choice to modders
	// they override any values in the 2da, which is something GetVerbalConstantSound handles for us
	int stance = GetStance();
	EnumIterator<Verbal> vb(Verbal::count);
	switch (stance) {
		case IE_ANI_ATTACK_SLASH:
			vb = EnumIterator<Verbal>(Verbal::Attack1);
			break;
		case IE_ANI_ATTACK_BACKSLASH:
			vb = EnumIterator<Verbal>(Verbal::Attack2);
			break;
		case IE_ANI_ATTACK_JAB:
			vb = EnumIterator<Verbal>(Verbal::Attack3);
			break;
		case IE_ANI_SHOOT:
			vb = EnumIterator<Verbal>(Verbal::Attack4);
			break;
		default:
			Log(WARNING, "Actor", "Unknown attack stance detected ({}) for {}, not playing creature swing sound!", stance, fmt::WideToChar { LongName });
			break;
	}
	if (vb != vb.end()) {
		bool found = VerbalConstant(*vb);
		// retry with 2da for soundsets, since they only checked one thing
		if (!found) {
			ResRef sound2;
			GetSoundFromFile(sound2, *vb);
			if (sound != sound2) core->GetAudioDrv()->Play(sound2, SFX_CHAN_SWINGS, Pos);
		}
	}

	// finally actual swing sounds
	ieDword itemType = wi.itemtype;
	int isCount = gamedata->GetSwingCount(itemType);
	if (isCount != -2) {
		// swing sounds start at column 3 (index 2)
		int isChoice = core->Roll(1, isCount, -1) + 2;
		if (!gamedata->GetItemSound(sound, itemType, AnimRef(), isChoice)) return;
	}
	core->GetAudioDrv()->Play(sound, SFX_CHAN_SWINGS, Pos);
}

//Just to quickly inspect debug maximum values
#if 0
void Actor::dumpMaxValues()
{
	int symbol = core->LoadSymbol( "stats" );
	if (symbol !=-1) {
		SymbolMgr *sym = core->GetSymbol( symbol );

		for(int i=0;i<MAX_STATS;i++) {
			Log(DEBUG, "Actor", "{}({}) {}", i, sym->GetValue(i), maximum_values[i]);
		}
	}
}
#endif

std::string Actor::dump() const
{
	std::string buffer;
	AppendFormat(buffer, "Debugdump of Actor {} ({}, {}):\n", fmt::WideToChar{GetName()}, fmt::WideToChar{GetShortName()}, fmt::WideToChar{GetDefaultName()});
	buffer.append("Scripts:");
	for (const auto script : Scripts) {
		ResRef poi = "<none>";
		if (script) {
			poi = script->GetName();
		}
		AppendFormat(buffer, " {}", poi);
	}
	buffer.append("\n");
	AppendFormat(buffer, "Area:       {} {}\n", Area, Pos);
	AppendFormat(buffer, "Dialog:     {}    TalkCount:  {}\n", Dialog, TalkCount);
	AppendFormat(buffer, "Global ID:  {}   PartySlot: {}\n", GetGlobalID(), InParty);
	AppendFormat(buffer, "Script name:{:<32}    Current action: {}    Total: {}\n", scriptName, CurrentAction ? CurrentAction->actionID : -1, actionQueue.size());
	AppendFormat(buffer, "Int. Flags: {:#x}    ", InternalFlags);
	AppendFormat(buffer, "MC Flags: {:#x}    ", Modified[IE_MC_FLAGS]);
	AppendFormat(buffer, "Allegiance: {}   current allegiance:{}\n", BaseStats[IE_EA], Modified[IE_EA] );
	AppendFormat(buffer, "Class:      {}   current class:{}    Kit: {} (base: {})\n", BaseStats[IE_CLASS], Modified[IE_CLASS], Modified[IE_KIT], BaseStats[IE_KIT] );
	AppendFormat(buffer, "Race:       {}   current race:{}\n", BaseStats[IE_RACE], Modified[IE_RACE] );
	AppendFormat(buffer, "Gender:     {}   current gender:{}\n", BaseStats[IE_SEX], Modified[IE_SEX] );
	AppendFormat(buffer, "Specifics:  {}   current specifics:{}\n", BaseStats[IE_SPECIFIC], Modified[IE_SPECIFIC] );
	AppendFormat(buffer, "Alignment:  {:#x}   current alignment:{:#x}\n", BaseStats[IE_ALIGNMENT], Modified[IE_ALIGNMENT] );
	AppendFormat(buffer, "Morale:     {}   current morale:{}\n", BaseStats[IE_MORALE], Modified[IE_MORALE] );
	AppendFormat(buffer, "Moralebreak:{}   Morale recovery:{}\n", Modified[IE_MORALEBREAK], Modified[IE_MORALERECOVERYTIME] );
	AppendFormat(buffer, "Visualrange:{} (Explorer: {})\n", Modified[IE_VISUALRANGE], Modified[IE_EXPLORE] );
	AppendFormat(buffer, "Fatigue: {} (current: {})   Luck: {}\n", BaseStats[IE_FATIGUE], Modified[IE_FATIGUE], Modified[IE_LUCK]);
	AppendFormat(buffer, "Movement rate: {} (current: {})\n\n", BaseStats[IE_MOVEMENTRATE], Modified[IE_MOVEMENTRATE]);

	//this works for both level slot style
	AppendFormat(buffer, "Levels (average: {}):\n", GetXPLevel(true));
	for (unsigned int i = 0; i < ISCLASSES; i++) {
		int level = GetClassLevel(i);
		if (level) {
			AppendFormat(buffer, "{}: {}    ", isclassnames[i], level);
		}
	}
	buffer.append("\n");

	AppendFormat(buffer, "current HP:{}\n", BaseStats[IE_HITPOINTS] );
	AppendFormat(buffer, "Mod[IE_ANIMATION_ID]: 0x{:^4X} ResRef:{} Stance: {}\n", Modified[IE_ANIMATION_ID], anims->ResRefBase, GetStance());
	AppendFormat(buffer, "TURNUNDEADLEVEL: {} current: {}\n", BaseStats[IE_TURNUNDEADLEVEL], Modified[IE_TURNUNDEADLEVEL]);
	AppendFormat(buffer, "Colors:    ");
	if (core->HasFeature(GFFlags::ONE_BYTE_ANIMID) ) {
		for(unsigned int i = 0; i < Modified[IE_COLORCOUNT]; i++) {
			AppendFormat(buffer, "   {}", Modified[IE_COLORS+i]);
		}
	} else {
		for(unsigned int i = 0; i < 7; i++) {
			AppendFormat(buffer, "   {}", Modified[IE_COLORS+i]);
		}
	}
	buffer.append("\n");
	AppendFormat(buffer, "WaitCounter: {}\n", GetWait());
	AppendFormat(buffer, "LastTarget: {} {}    ", objects.LastTarget, GetActorNameByID(objects.LastTarget));
	AppendFormat(buffer, "LastSpellTarget: {} {}\n", objects.LastSpellTarget, GetActorNameByID(objects.LastSpellTarget));
	AppendFormat(buffer, "LastTalked: {} {}\n", objects.LastTalker, GetActorNameByID(objects.LastTalker));
	buffer.append(inventory.dump(false));
	buffer.append(spellbook.dump(false));
	buffer.append(fxqueue.dump(false));
	Log(DEBUG, "Actor", "{}", buffer);
	return buffer;
}

ieVariable Actor::GetActorNameByID(ieDword ID) const
{
	const Actor *actor = GetCurrentArea()->GetActorByGlobalID(ID);
	if (!actor) {
		return "<NULL>";
	}
	return actor->GetScriptName();
}

void Actor::SetMap(Map *map)
{
	//Did we have an area?
	bool effinit=!GetCurrentArea();
	if (area && BlocksSearchMap()) area->ClearSearchMapFor(this);
	//now we have an area
	Scriptable::SetMap(map);
	//unless we just lost it, in that case clear up some fields and leave
	if (!map) {
		//more bits may or may not be needed
		InternalFlags &=~IF_CLEANUP;
		return;
	}
	InternalFlags &= ~IF_PST_WMAPPING;

	//These functions are called once when the actor is first put in
	//the area. It already has all the items (including fist) at this
	//time and it is safe to call effects.
	//This hack is to delay the equipping effects until the actor has
	//an area (and the game object is also existing)
	if (effinit) {
		//already initialized, no need of updating stuff
		if (InternalFlags & IF_GOTAREA) return;
		InternalFlags |= IF_GOTAREA;

		//apply feats
		ApplyFeats();
		//apply persistent feat spells
		ApplyExtraSettings();

		int SlotCount = inventory.GetSlotCount();
		for (int Slot = 0; Slot<SlotCount;Slot++) {
			int slottype = core->QuerySlotEffects( Slot );
			switch (slottype) {
			case SLOT_EFFECT_NONE:
			case SLOT_EFFECT_FIST:
			case SLOT_EFFECT_MELEE:
			case SLOT_EFFECT_MISSILE:
			// TODO: is SLOT_EFFECT_ALIAS missing here — at least for pst aliased weapon slots?
				break;
			default:
				inventory.EquipItem( Slot );
				break;
			}
		}
		//We need to convert this to signed 16 bits, because
		//it is actually a 16 bit number.
		//It is signed to have the correct math
		//when adding it to the base slot (SLOT_WEAPON) in
		//case of quivers. (weird IE magic)
		//The other word is the equipped header.
		//find a quiver for the bow, etc
		inventory.EquipItem(inventory.GetEquippedSlot());
		SetEquippedQuickSlot(inventory.GetEquipped(), inventory.GetEquippedHeader());
	}
	if (BlocksSearchMap()) map->BlockSearchMapFor(this);
}

// Position should be a navmap point
void Actor::SetPosition(const Point &nmptTarget, int jump, int radiusx, int radiusy, int size)
{
	ResetPathTries();
	ClearPath(true);
	Point p, q;
	p.x = nmptTarget.x / 16;
	p.y = nmptTarget.y / 12;

	q = p;
	if (jump && !(Modified[IE_DONOTJUMP] & DNJ_FIT) && size ) {
		const Map *map = GetCurrentArea();
		//clear searchmap so we won't block ourselves
		map->ClearSearchMapFor(this);
		map->AdjustPosition(p, radiusx, radiusy, size);
	}
	if (p==q) {
		MoveTo(nmptTarget);
	} else {
		p.x = p.x * 16 + 8;
		p.y = p.y * 12 + 6;
		MoveTo( p );
	}
}

/* this is returning the level of the character for xp calculations
	and the average level for dual/multiclass (rounded up),
	also with iwd2's 3rd ed rules, this is why it is a separate function */
ieDword Actor::GetXPLevel(int modified) const
{
	const stats_t& stats = modified ? Modified : BaseStats;

	size_t clscount = 0;
	stat_t average = 0;
	if (iwd2class) {
		// iwd2
		return stats[IE_CLASSLEVELSUM];
	} else {
		stat_t levels[3]={stats[IE_LEVEL], stats[IE_LEVEL2], stats[IE_LEVEL3]};
		average = levels[0];
		clscount = 1;
		if (IsDualClassed()) {
			// dualclassed
			if (levels[1] > 0) {
				clscount++;
				average += levels[1];
			}
		} else if (IsMultiClassed()) {
			//clscount is the number of on bits in the MULTI field
			clscount = CountBits (multiclass);
			assert(clscount && clscount <= 3);
			for (size_t i=1; i<clscount; i++)
				average += levels[i];
		} //else single classed
		average = stat_t(average / (float) clscount + 0.5);
	}
	return ieDword(average);
}

// returns the guessed caster level by passed spell type
// FIXME: add more logic for cross-type kits (like avengers)?
// FIXME: iwd2 does the right thing at least for spells cast from spellbooks;
//        that is, it takes the correct level, not first or average or min or max.
//        We need to propagate the spellbook info all through here. :/
//        NOTE: this is only problematic for multiclassed actors
ieDword Actor::GetBaseCasterLevel(int spelltype, int flags) const
{
	int level = 0;

	switch(spelltype)
	{
	case IE_SPL_PRIEST:
		level = GetClericLevel();
		if (!level) level = GetDruidLevel();
		if (!level) level = GetPaladinLevel();
		// for cleric/rangers, we can't tell from which class a spell is, unless unique, so we ignore the distinction
		if (!level) level = GetRangerLevel();
		break;
	case IE_SPL_WIZARD:
		level = GetMageLevel();
		if (!level) level = GetSorcererLevel();
		if (!level) level = GetBardLevel();
		break;
	default:
		// checking if anyone uses the psion, item and song types
		if (spelltype != IE_SPL_INNATE) {
			Log(WARNING, "Actor", "Unhandled SPL type {}, using average casting level!", spelltype);
		}
		break;
	}
	// if nothing was found, use the average level
	if (!level && !flags) level = GetXPLevel(true);

	return level;
}

int Actor::GetWildMod(int level)
{
	if (GetStat(IE_KIT) != KIT_WILDMAGE) {
		return 0;
	}

	// avoid rerolling the mod, since we get called multiple times per each cast
	// TODO: also handle a reroll to 0
	if (WMLevelMod) {
		return 0;
	}

	level = Clamp(level, 1, MAX_LEVEL);
	static int modRange = int(wmLevelMods.size());
	WMLevelMod = wmLevelMods[core->Roll(1, modRange, -1)][level - 1];

	SetTokenAsString("LEVELDIF", abs(WMLevelMod));
	if (core->HasFeedback(FT_STATES)) {
		if (WMLevelMod > 0) {
			displaymsg->DisplayConstantStringName(HCStrings::CasterLvlInc, GUIColors::WHITE, this);
		} else if (WMLevelMod < 0) {
			displaymsg->DisplayConstantStringName(HCStrings::CasterLvlDec, GUIColors::WHITE, this);
		}
	}
	return WMLevelMod;
}

int Actor::CastingLevelBonus(int level, int type)
{
	int bonus = 0;
	switch(type)
	{
	case IE_SPL_PRIEST:
		bonus = GetStat(IE_CASTINGLEVELBONUSCLERIC);
		break;
	case IE_SPL_WIZARD:
		bonus = GetWildMod(level) + GetStat(IE_CASTINGLEVELBONUSMAGE);
	}

	return bonus;
}

ieDword Actor::GetCasterLevel(int spelltype)
{
	int level = GetBaseCasterLevel(spelltype);
	return level + CastingLevelBonus(level, spelltype);
}

// this works properly with disabled dualclassed actors, since it ends up in GetClassLevel
ieDword Actor::GetAnyActiveCasterLevel() const
{
	int strict = 1;
	// only player classes will have levels in the correct slots
	if (!HasPlayerClass()) {
		strict = 0;
	}
	return GetBaseCasterLevel(IE_SPL_PRIEST, strict) + GetBaseCasterLevel(IE_SPL_WIZARD, strict);
}

int Actor::GetEncumbranceFactor(bool feedback) const
{
	int encumbrance = inventory.GetWeight();
	int maxWeight = GetMaxEncumbrance();

	if (encumbrance <= maxWeight || (BaseStats[IE_EA] > EA_GOODCUTOFF && !third)) {
		return 1;
	}
	if (encumbrance <= maxWeight * 2) {
		if (feedback && core->HasFeedback(FT_STATES)) {
			displaymsg->DisplayConstantStringName(HCStrings::HalfSpeed, GUIColors::WHITE, this);
		}
		return 2;
	}
	if (feedback && core->HasFeedback(FT_STATES)) {
		displaymsg->DisplayConstantStringName(HCStrings::CantMove, GUIColors::WHITE, this);
	}
	return 123456789; // large enough to round to 0 when used as a divisor
}

int Actor::CalculateSpeed(bool feedback) const
{
	if (core->HasFeature(GFFlags::RESDATA_INI)) {
		return CalculateSpeedFromINI(feedback);
	} else {
		return CalculateSpeedFromRate(feedback);
	}
}

// NOTE: for ini-based speed users this will only update their encumbrance, speed will be 0
int Actor::CalculateSpeedFromRate(bool feedback) const
{
	int movementRate = GetStat(IE_MOVEMENTRATE);
	int encumbranceFactor = GetEncumbranceFactor(feedback);
	if (BaseStats[IE_EA] > EA_GOODCUTOFF && !third) {
		// cheating bastards (drow in ar2401 for example)
	} else {
		movementRate /= encumbranceFactor;
	}
	if (movementRate) {
		return 1500 / movementRate;
	} else {
		return 0;
	}
}

int Actor::CalculateSpeedFromINI(bool feedback) const
{
	int encumbranceFactor = GetEncumbranceFactor(feedback);
	ieDword animid = BaseStats[IE_ANIMATION_ID];
	if (core->HasFeature(GFFlags::ONE_BYTE_ANIMID)) {
		animid = animid & 0xff;
	}
	assert(animid < (ieDword)CharAnimations::GetAvatarsCount());
	const AvatarStruct &avatar = CharAnimations::GetAvatarStruct(animid);
	int newSpeed = 0;
	if (avatar.RunScale && (GetInternalFlag() & IF_RUNNING)) {
		newSpeed = avatar.RunScale;
	} else if (avatar.WalkScale) {
		newSpeed = avatar.WalkScale;
	} else {
		// 3 pst animations don't have a walkscale set, but they're immobile, so the default of 0 is fine
	}

	// the speeds are already inverted, so we need to increase them to slow down
	if (encumbranceFactor <= 2) {
		newSpeed *= encumbranceFactor;
	} else {
		newSpeed = 0;
	}
	return newSpeed;
}

//receive turning
void Actor::Turn(Scriptable *cleric, ieDword turnlevel)
{
	assert(cleric);
	bool evilcleric = false;
	static ieDword turnPanicLevelMod = gamedata->GetMiscRule("TURN_PANIC_LVL_MOD");
	static ieDword turnDeathLevelMod = gamedata->GetMiscRule("TURN_DEATH_LVL_MOD");

	if (!turnlevel) {
		return;
	}

	//determine if we see the cleric (distance)
	if (!CanSee(cleric, this, true, GA_NO_DEAD)) {
		return;
	}

	const Actor* cleric2 = Scriptable::As<Actor>(cleric);
	if (cleric2 && GameScript::ID_Alignment(cleric2, AL_EVIL)) {
		evilcleric = true;
	}

	//a little adjustment of the level to get a slight randomness on who is turned
	unsigned int level = GetXPLevel(true)-(GetGlobalID()&3);

	//this is safely hardcoded i guess
	if (Modified[IE_GENERAL]!=GEN_UNDEAD) {
		level = GetPaladinLevel();
		if (evilcleric && level) {
			AddTrigger(TriggerEntry(trigger_turnedby, cleric->GetGlobalID()));
			if (turnlevel >= level + turnDeathLevelMod) {
				if (gamedata->Exists("panic", IE_SPL_CLASS_ID)) {
					core->ApplySpell(ResRef("panic"), this, cleric, level);
				} else {
					Log(DEBUG, "Actor", "Panic from turning!");
					Panic(cleric, PANIC_RUNAWAY);
				}
			}
		}
		return;
	}

	//determine alignment (if equals, then no turning)

	AddTrigger(TriggerEntry(trigger_turnedby, cleric->GetGlobalID()));

	//determine panic or destruction/control
	//we get the modified level
	if (turnlevel >= level + turnDeathLevelMod) {
		if (evilcleric) {
			Effect* fx = EffectQueue::CreateEffect(control_creature_ref, GEN_UNDEAD, 3, FX_DURATION_INSTANT_LIMITED);
			if (!fx) {
				fx = EffectQueue::CreateEffect(control_undead_ref, GEN_UNDEAD, 3, FX_DURATION_INSTANT_LIMITED);
			}
			if (fx) {
				fx->Duration = core->Time.round_sec;
				fx->Target = FX_TARGET_PRESET;
				core->ApplyEffect(fx, this, cleric);
				return;
			}
			//fallthrough for bg1
		}
		Die(cleric);
	} else if (turnlevel >= level + turnPanicLevelMod) {
		Log(DEBUG, "Actor", "Panic from turning!");
		Panic(cleric, PANIC_RUNAWAY);
	}
}

void Actor::Resurrect(const Point &destPoint)
{
	if (!(Modified[IE_STATE_ID ] & STATE_DEAD)) {
		return;
	}
	InternalFlags&=IF_FROMGAME; //keep these flags (what about IF_INITIALIZED)
	InternalFlags|=IF_ACTIVE|IF_VISIBLE; //set these flags
	SetBaseBit(IE_STATE_ID, STATE_DEAD, false);
	BaseStats[IE_GENERAL] = GEN_HUMANOID;
	SetBase(IE_STATE_ID, 0);
	SetBase(IE_AVATARREMOVAL, 0);
	if (!destPoint.IsZero()) {
		SetPosition(destPoint, CC_CHECK_IMPASSABLE, 0);
	}
	if (ShouldModifyMorale()) SetBase(IE_MORALE, 10);
	//resurrect spell sets the hitpoints to maximum in a separate effect
	//raise dead leaves it at 1 hp
	SetBase(IE_HITPOINTS, 1);
	Stop();
	SetStance(IE_ANI_EMERGE);
	Game *game = core->GetGame();
	//readjust death variable on resurrection
	ieVariable DeathVar;
	if (core->HasFeature(GFFlags::HAS_KAPUTZ) && (AppearanceFlags&APP_DEATHVAR)) {
		if (!DeathVar.Format("{}_DEAD", scriptName)) {
			Log(ERROR, "Actor", "Scriptname {} (name: {}) is too long for generating death globals!", scriptName, fmt::WideToChar{GetName()});
		}
		ieDword value=0;

		auto lookup = game->kaputz.find(DeathVar);
		if (lookup != game->kaputz.cend()) {
			value = lookup->second;
		}

		if (value > 0) {
			game->kaputz[DeathVar] = value - 1;
		}
	// not bothering with checking actor->SetDeathVar, since the SetAt nocreate parameter is true
	} else if (!core->HasFeature(GFFlags::HAS_KAPUTZ)) {
		if (!DeathVar.Format(Interface::GetDeathVarFormat(), scriptName)) {
			Log(ERROR, "Actor", "Scriptname {} (name: {}) is too long for generating death globals (on resurrect)!", scriptName, fmt::WideToChar{GetName()});
		}

		auto lookup = game->locals.find(DeathVar);
		if (lookup != game->locals.cend()) {
			lookup->second = 0;
		}
	}

	ResetCommentTime();
	//clear effects?
}

static const std::string& GetVarName(const ResRef& table, int value)
{
	int symbol = core->LoadSymbol( table );
	if (symbol!=-1) {
		auto sym = core->GetSymbol( symbol );
		return sym->GetValue( value );
	}
	return blank;
}

// [EA.FACTION.TEAM.GENERAL.RACE.CLASS.SPECIFIC.GENDER.ALIGN] has to be the same for both creatures
static bool OfType(const Actor *a, const Actor *b)
{
	bool same = a->GetStat(IE_EA) == b->GetStat(IE_EA) &&
		a->GetStat(IE_RACE) == b->GetStat(IE_RACE) &&
		a->GetStat(IE_GENERAL) == b->GetStat(IE_GENERAL) &&
		a->GetStat(IE_SPECIFIC) == b->GetStat(IE_SPECIFIC) &&
		a->GetStat(IE_CLASS) == b->GetStat(IE_CLASS) &&
		a->GetStat(IE_TEAM) == b->GetStat(IE_TEAM) &&
		a->GetStat(IE_FACTION) == b->GetStat(IE_FACTION) &&
		a->GetStat(IE_SEX) == b->GetStat(IE_SEX) &&
		a->GetStat(IE_ALIGNMENT) == b->GetStat(IE_ALIGNMENT);
	if (!same) return false;

	if (!third) return true;

	return a->GetStat(IE_SUBRACE) == b->GetStat(IE_SUBRACE);
}

void Actor::SendDiedTrigger() const
{
	if (!area) return;
	std::vector<Actor *> neighbours = area->GetAllActorsInRadius(Pos, GA_NO_LOS|GA_NO_DEAD|GA_NO_UNSCHEDULED, GetSafeStat(IE_VISUALRANGE));
	int ea = Modified[IE_EA];

	for (auto& neighbour : neighbours) {
		// NOTE: currently also sending the trigger to ourselves — prevent if it causes problems
		neighbour->AddTrigger(TriggerEntry(trigger_died, GetGlobalID()));

		// allies take a hit on morale and nobody cares about neutrals
		if (!neighbour->ShouldModifyMorale()) continue;
		int pea = neighbour->GetStat(IE_EA);
		if (ea == EA_PC && pea == EA_PC) {
			neighbour->NewBase(IE_MORALE, (stat_t) -1, MOD_ADDITIVE);
		} else if (OfType(this, neighbour)) {
			neighbour->NewBase(IE_MORALE, (stat_t) -1, MOD_ADDITIVE);
		// are we an enemy of neighbour, regardless if we're good or evil?
		} else if (abs(ea - pea) > 30) {
			neighbour->NewBase(IE_MORALE, 2, MOD_ADDITIVE);
		}
	}
}

static void UpdateOrCreateVariable(ieVarsMap& vars, const ieVariable& key, ieDword value) {
	auto lookup = vars.find(key);
	if (lookup != vars.cend()) {
		lookup->second = value;
	} else if (!nocreate) {
		vars[key] = value;
	}
}

static void IncrementOrCreateVariable(ieVarsMap& vars, const ieVariable& key, ieDword value) {
	auto lookup = vars.find(key);
	if (lookup != vars.cend()) {
		lookup->second += value;
	} else if (!nocreate) {
		vars[key] = value;
	}
}

void Actor::Die(Scriptable *killer, bool grantXP)
{
	if (InternalFlags&IF_REALLYDIED) {
		return; //can die only once
	}

	//Can't simply set Selected to false, game has its own little list
	Game *game = core->GetGame();
	game->SelectActor(this, false, SELECT_NORMAL);

	displaymsg->DisplayConstantStringName(HCStrings::Death, GUIColors::WHITE, this);
	bool found = VerbalConstant(Verbal::Die, gamedata->GetVBData("SPECIAL_COUNT"));
	if (found) {
		ResRef sound;
		GetSoundFromFile(sound, Verbal::Die);
		core->GetAudioDrv()->Play(sound, SFX_CHAN_MONSTER, Pos);
	}

	// remove poison, hold, casterhold, stun and its icon
	Effect *newfx;
	newfx = EffectQueue::CreateEffect(fx_cure_poisoned_state_ref, 0, 0, FX_DURATION_INSTANT_PERMANENT);
	core->ApplyEffect(newfx, this, this);

	newfx = EffectQueue::CreateEffect(fx_cure_hold_state_ref, 0, 0, FX_DURATION_INSTANT_PERMANENT);
	core->ApplyEffect(newfx, this, this);

	newfx = EffectQueue::CreateEffect(fx_unpause_caster_ref, 0, 100, FX_DURATION_INSTANT_PERMANENT);
	core->ApplyEffect(newfx, this, this);

	newfx = EffectQueue::CreateEffect(fx_cure_stun_state_ref, 0, 0, FX_DURATION_INSTANT_PERMANENT);
	core->ApplyEffect(newfx, this, this);

	newfx = EffectQueue::CreateEffect(fx_remove_portrait_icon_ref, 0, 37, FX_DURATION_INSTANT_PERMANENT);
	core->ApplyEffect(newfx, this, this);

	// clearing the search map here means it's not blocked during death animations
	// this is perhaps not ideal, but matches other searchmap code which uses
	// GA_NO_DEAD to exclude IF_JUSTDIED actors as well as dead ones
	if (area)
		area->ClearSearchMapFor(this);

	//JUSTDIED will be removed after the first script check
	//otherwise it is the same as REALLYDIED
	InternalFlags|=IF_REALLYDIED|IF_JUSTDIED;
	//remove IDLE so the actor gets a chance to die properly
	InternalFlags&=~IF_IDLE;

	if (LastDamageType & DAMAGE_CHUNKING) {
		ChunkActor(this);
	} else if (GetStance() != IE_ANI_DIE) {
		SetStance(IE_ANI_DIE);
	}
	BaseStats[IE_GENERAL] = GEN_DEAD;
	AddTrigger(TriggerEntry(trigger_die));
	SendDiedTrigger();
	if (pstflags && this == game->GetPC(0, false)) {
		AddTrigger(TriggerEntry(trigger_namelessbitthedust));
	}

	if (!killer) {
		// TODO: is this right?
		killer = area->GetActorByGlobalID(objects.LastHitter);
	}
	if (killer) killer->objects.LastKilled = GetGlobalID();
	Actor* act = Scriptable::As<Actor>(killer);
	bool killerPC = false;
	if (act) {
		// for unknown reasons the original only sends the trigger if the killer is ok
		if (!(act->GetStat(IE_STATE_ID) & (STATE_DEAD | STATE_PETRIFIED | STATE_FROZEN))) {
			killer->AddTrigger(TriggerEntry(trigger_killed, GetGlobalID()));
			if (act->ShouldModifyMorale()) act->NewBase(IE_MORALE, 3, MOD_ADDITIVE);
		}
		killerPC = act->InParty > 0;
	}

	if (InParty) {
		if (area) SendTriggerToAll(TriggerEntry(trigger_partymemberdied, GetGlobalID()), GA_NO_SELF | GA_NO_ENEMY);
		game->PartyMemberDied(this);
		core->Autopause(AUTOPAUSE::DEAD, this);
	} else {
		// sometimes we want to skip xp giving and favourite registration
		if (grantXP && act) {
			if (act->InParty) {
				//adjust kill statistics here
				PCStatsStruct *stat = act->PCStats;
				if (stat) {
					stat->NotifyKill(Modified[IE_XPVALUE], ShortStrRef);
				}
				InternalFlags|=IF_GIVEXP;
			}

			// friendly party summons' kills also grant xp
			if (act->Modified[IE_SEX] == SEX_SUMMON && act->Modified[IE_EA] == EA_CONTROLLED) {
				InternalFlags|=IF_GIVEXP;
			} else if (act->Modified[IE_EA] == EA_FAMILIAR) {
				// familiar's kills also grant xp
				InternalFlags|=IF_GIVEXP;
			}
		}
	}

	// XP seems to be handed at out at the moment of death
	if (InternalFlags&IF_GIVEXP) {
		//give experience to party
		game->ShareXP(Modified[IE_XPVALUE], sharexp );

		if (!InParty && act && act->GetStat(IE_EA) <= EA_CONTROLLABLE && !core->InCutSceneMode()) {
			// adjust reputation if the corpse was:
			// an innocent, a member of the Flaming Fist or something evil
			int repmod = 0;
			if (Modified[IE_CLASS] == CLASS_INNOCENT) {
				repmod = gamedata->GetReputationMod(0);
			} else if (Modified[IE_CLASS] == CLASS_FLAMINGFIST) {
				repmod = gamedata->GetReputationMod(3);
			}
			if (GameScript::ID_Alignment(this,AL_EVIL) ) {
				repmod += gamedata->GetReputationMod(7);
			}
			if (repmod) {
				game->SetReputation(game->Reputation + repmod);
			}
		}
	}

	ReleaseCurrentAction();
	ClearPath(true);
	SetModal(Modal::None);

	if (InParty && killerPC) {
		UpdateOrCreateVariable(game->locals, "PM_KILLED", 1);
	}

	// EXTRACOUNT is updated at the moment of death
	if (Modified[IE_SEX] == SEX_EXTRA || (Modified[IE_SEX] >= SEX_EXTRA2 && Modified[IE_SEX] <= SEX_MAXEXTRA)) {
		// if gender is set to one of the EXTRA values, then at death, we have to decrease
		// the relevant EXTRACOUNT area variable. scripts use this to check how many actors
		// of this extra id are still alive (for example, see the ToB challenge scripts)
		ieVariable varname;
		if (Modified[IE_SEX] == SEX_EXTRA) {
			varname = "EXTRACOUNT";
		} else {
			varname.Format("EXTRACOUNT{}", 2 + (Modified[IE_SEX] - SEX_EXTRA2));
		}

		if (area) {
			ieDword value = area->GetLocal(varname, 0);
			// i am guessing that we shouldn't decrease below 0
			if (value > 0) {
				area->locals[varname] = value - 1;
			}
		}
	}

	//a plot critical creature has died (iwd2)
	// BG2 uses the same field for special creatures (alternate melee damage): MC_LARGE_CREATURE
	if (third && BaseStats[IE_MC_FLAGS] & MC_PLOT_CRITICAL) {
		core->GetGUIScriptEngine()->RunFunction("GUIWORLD", "DeathWindowPlot", false);
	}
	//ensure that the scripts of the actor will run as soon as possible
	ImmediateEvent();
}

void Actor::SetPersistent(int partyslot)
{
	if (partyslot<0) {
		//demote actor to be saved in area (after moving between areas)
		InParty = 0;
		InternalFlags&=~IF_FROMGAME;
		return;
	}
	InParty = (ieByte) partyslot;
	InternalFlags|=IF_FROMGAME;
	//if an actor is coming from a game, it should have these too
	CreateStats();
	// ensure QSlots are set up to be what the class needs
	InitButtons(GetActiveClass(), false);

	if (PCStats->QuickWeaponSlots[0] != 0xffff) return;
	// ReinitQuickSlots does not take care of weapon slots, so do it manually
	for (int i = 0; i < 4; i++) {
		SetupQuickSlot(i + ACT_WEAPON1, Inventory::GetWeaponSlot(i), 0);
	}
	// call ReinitQuickSlots here if something needs it
}

void Actor::DestroySelf()
{
	InternalFlags|=IF_CLEANUP;
	RemovalTime = 0;
	// clear search map so that a new actor can immediately go there
	// (via ChangeAnimationCore)
	if (area)
		area->ClearSearchMapFor(this);
}

bool Actor::CheckOnDeath()
{
	if (InternalFlags&IF_CLEANUP) {
		return true;
	}
	// FIXME
	if (InternalFlags&IF_JUSTDIED || CurrentAction || GetNextAction() || GetStance() == IE_ANI_DIE) {
		return false; //actor is currently dying, let him die first
	}
	if (!(InternalFlags&IF_REALLYDIED) ) {
		return false;
	}
	//don't mess with the already deceased
	if (BaseStats[IE_STATE_ID]&STATE_DEAD) {
		return false;
	}
	// don't destroy actors currently in a dialog
	const GameControl *gc = core->GetGameControl();
	if (gc && gc->dialoghandler->InDialog(this)) {
		return false;
	}

	ClearActions();
	//missed the opportunity of Died()
	InternalFlags&=~IF_JUSTDIED;

	// items seem to be dropped at the moment of death in the original but this
	// can't go in Die() because that is called from effects and dropping items
	// might change effects! so we just drop everything here

	// disintegration destroys normal items if difficulty level is high enough
	bool disintegrated = LastDamageType & DAMAGE_DISINTEGRATE;
	if (disintegrated && GameDifficulty > DIFF_CORE) {
		inventory.DestroyItem("", IE_INV_ITEM_DESTRUCTIBLE, (ieDword) ~0);
	}
	// drop everything remaining, but ignore TNO, as he needs to keep his gear
	Game *game = core->GetGame();
	if (game->protagonist != PM_NO || GetScriptName() != game->GetPC(0, false)->GetScriptName()) {
		DropItem("", 0);
	}

	//remove all effects that are not 'permanent after death' here
	//permanent after death type is 9
	SetBaseBit(IE_STATE_ID, STATE_DEAD, true);

	// death variables are updated at the moment of death in the original
	if (core->HasFeature(GFFlags::HAS_KAPUTZ)) {
		const char* format = AppearanceFlags & APP_ADDKILL ? "KILL_{}" : "{}";

		//don't use the raw killVar here (except when the flags explicitly ask for it)
		if (AppearanceFlags & APP_DEATHTYPE) {
			IncrementDeathVariable(game->kaputz, format, KillVar);
		}
		if (AppearanceFlags & APP_FACTION) {
			IncrementDeathVariable(game->kaputz, format, GetVarName("faction", BaseStats[IE_FACTION]));
		}
		if (AppearanceFlags & APP_TEAM) {
			IncrementDeathVariable(game->kaputz, format, GetVarName("team", BaseStats[IE_TEAM]));
		}
		if (AppearanceFlags & APP_DEATHVAR) {
			IncrementDeathVariable(game->kaputz, "{}_DEAD", scriptName);
		}

	} else {
		IncrementDeathVariable(game->locals, "{}", KillVar);
		IncrementDeathVariable(game->locals, Interface::GetDeathVarFormat(), scriptName);
	}

	IncrementDeathVariable(game->locals, "{}", IncKillVar);

	if (scriptName[0] && SetDeathVar) {
		ieVariable varname;

		if (!varname.Format("{}_DEAD", scriptName)) {
			Log(ERROR, "Actor", "Scriptname {} (name: {}) is too long for generating death globals!", scriptName, fmt::WideToChar{GetName()});
		}

		UpdateOrCreateVariable(game->locals, varname, 1);
		IncrementDeathVariable(game->locals, "{}_KILL_CNT", scriptName);
	}

	if (IncKillCount) {
		// racial dead count
		int racetable = core->LoadSymbol("race");
		if (racetable != -1) {
			auto race = core->GetSymbol(racetable);
			IncrementDeathVariable(game->locals, "KILL_{}_CNT", race->GetValue(Modified[IE_RACE]));
		}
	}

	// death counters for PST: APP_GOOD, APP_LAW, APP_LADY, APP_MURDER
	for (int i = 0, j = APP_GOOD; i < 4; i++) {
		if (AppearanceFlags & j) {
			IncrementOrCreateVariable(game->locals, CounterNames[i], DeathCounters[i]);
		}
		j += j;
	}

	if (disintegrated) return true;

	// party actors are never removed
	// FIXME: even when chunking? Consider changing and adding fx_replace_creature-like handling
	if (Persistent()) {
		// hide the corpse artificially
		SetBase(IE_AVATARREMOVAL, 1);
		return false;
	}

	ieDword time = core->GetGame()->GameTime;
	if (!pstflags && Modified[IE_MC_FLAGS]&MC_REMOVE_CORPSE) {
		RemovalTime = time;
		return true;
	}
	if (Modified[IE_MC_FLAGS]&MC_KEEP_CORPSE) return false;
	RemovalTime = time + core->Time.day_size; // keep corpse around for a day

	//if chunked death, then return true
	if (LastDamageType & DAMAGE_CHUNKING) {
		RemovalTime = time;
		return true;
	}
	return false;
}

void Actor::IncrementDeathVariable(Game::kaputz_t& vars, const char *format, StringView name) const {
	if (!name.empty()) {
		ieVariable varname;
		if (!varname.Format(format, name)) {
			Log(ERROR, "Actor", "Scriptname {} (name: {}) is too long for generating death globals!", name, fmt::WideToChar{GetName()});
		}

		IncrementOrCreateVariable(vars, varname, 1);
	}
}

/* this will create a heap at location, and transfer the item(s) */
void Actor::DropItem(const ResRef& resref, unsigned int flags)
{
	if (inventory.DropItemAtLocation( resref, flags, area, Pos )) {
		ReinitQuickSlots();
	}
}

void Actor::DropItem(int slot , unsigned int flags)
{
	if (inventory.DropItemAtLocation( slot, flags, area, Pos )) {
		ReinitQuickSlots();
	}
}

/** returns quick item data */
/** if header==-1 which is a 'use quickitem' action */
/** if header is set, then which is the absolute slot index, */
/** and header is the header index */
void Actor::GetItemSlotInfo(ItemExtHeader *item, int which, int header) const
{
	ieWord idx;
	ieWord headerindex;

	if (header<0) {
		if (!PCStats) return; //not a player character
		PCStats->GetSlotAndIndex(which,idx,headerindex);
		if (headerindex==0xffff) return; //headerindex is invalid
	} else {
		idx=(ieWord) which;
		headerindex=(ieWord) header;
	}
	const CREItem *slot = inventory.GetSlotItem(idx);
	if (!slot) return; //quick item slot is empty
	const Item *itm = gamedata->GetItem(slot->ItemResRef, true);
	if (!itm) {
		Log(WARNING, "Actor", "Invalid quick slot item: {}!", slot->ItemResRef);
		return; //quick item slot contains invalid item resref
	}
	const ITMExtHeader *ext_header = itm->GetExtHeader(headerindex);
	//item has no extended header, or header index is wrong
	if (!ext_header) return;
	item->CopyITMExtHeader(*ext_header);
	item->itemName = slot->ItemResRef;
	item->slot = idx;
	item->headerindex = headerindex;
	if (headerindex>=CHARGE_COUNTERS) {
		item->Charges=0;
	} else {
		item->Charges=slot->Usages[headerindex];
	}
	gamedata->FreeItem(itm,slot->ItemResRef, false);
}

void Actor::ReinitQuickSlots() const
{
	if (!PCStats) {
		return;
	}

	// Note: (wjp, 20061226)
	// This function needs some rethinking.
	// It tries to satisfy two things at the moment:
	//   Fill quickslots when they are empty and an item is placed in the
	//      inventory slot corresponding to the quickslot
	//   Reset quickslots when an item is removed
	// Currently, it resets all slots when items are removed,
	// but it only refills the ACT_QSLOTn slots, not the ACT_WEAPONx slots.
	//
	// Refilling a weapon slot is possible, but essentially duplicates a lot
	// of code from Inventory::EquipItem() which performs the same steps for
	// the Inventory::Equipped slot.
	// Hopefully, weapons/arrows are never added to inventory slots without
	// EquipItem being called.

	int i=sizeof(PCStats->QSlots);
	while (i--) {
		int slot;
		ieByte which = IWD2GemrbQslot(i);

		switch (which) {
			case ACT_WEAPON1:
			case ACT_WEAPON2:
			case ACT_WEAPON3:
			case ACT_WEAPON4:
				CheckWeaponQuickSlot(which-ACT_WEAPON1);
				slot = 0;
				break;
				//WARNING:this cannot be condensed, because the symbols don't come in order!!!
			case ACT_QSLOT1: slot = Inventory::GetQuickSlot(); break;
			case ACT_QSLOT2: slot = Inventory::GetQuickSlot() + 1; break;
			case ACT_QSLOT3: slot = Inventory::GetQuickSlot() + 2; break;
			case ACT_QSLOT4: slot = Inventory::GetQuickSlot() + 3; break;
			case ACT_QSLOT5: slot = Inventory::GetQuickSlot() + 4; break;
			case ACT_IWDQITEM: slot = Inventory::GetQuickSlot(); break;
			case ACT_IWDQITEM + 1: slot = Inventory::GetQuickSlot() + 1; break;
			case ACT_IWDQITEM + 2: slot = Inventory::GetQuickSlot() + 2; break;
			case ACT_IWDQITEM + 3: slot = Inventory::GetQuickSlot() + 3; break;
			case ACT_IWDQITEM + 4: slot = Inventory::GetQuickSlot() + 4; break;
			// the rest are unavailable - only three slots in the actual inventory layout, 5 in the class for pst
			// case ACT_IWDQITEM+9:
			default:
				slot = 0;
		}
		if (!slot) continue;
		//if magic items are equipped the equipping info doesn't change
		//(afaik)

		// Note: we're now in the QSLOTn case
		// If slot is empty, reset quickslot to 0xffff/0xffff

		if (inventory.IsSlotEmpty(unsigned(slot))) {
			SetupQuickSlot(which, 0xffff, 0xffff);
		} else {
			ieWord idx;
			ieWord headerIndex;
			PCStats->GetSlotAndIndex(which, idx, headerIndex);
			if (idx != slot || headerIndex == 0xffff) {
				// If slot just became filled, set it to filled
				SetupQuickSlot(which, ieWord(slot), 0);
			}
		}
	}

	//these are always present
	CheckWeaponQuickSlot(0);
	CheckWeaponQuickSlot(1);
	if (weapSlotCount > 2) {
		for (unsigned int i = 2; i < weapSlotCount; i++) {
			CheckWeaponQuickSlot(i);
		}
	} else {
		// disabling quick weapon slots for certain classes
		for (unsigned int i = 0; i < 2; i++) {
			unsigned int which = ACT_WEAPON3 + i;
			// Assuming that ACT_WEAPON3 and 4 are always in the first two spots
			if (PCStats->QSlots[i + 3] != which) {
				SetupQuickSlot(which, 0xffff, 0xffff);
			}
		}
	}
}

void Actor::CheckWeaponQuickSlot(unsigned int which) const
{
	if (!PCStats) return;

	bool empty = false;
	// If current quickweaponslot doesn't contain an item, reset it to fist
	int slot = PCStats->QuickWeaponSlots[which];
	int header = PCStats->QuickWeaponHeaders[which];
	if (inventory.IsSlotEmpty(slot) || header == 0xffff) {
		//a quiver just went dry, falling back to fist
		empty = true;
	} else {
		// If current quickweaponslot contains ammo, and bow not found, reset

		if (core->QuerySlotEffects(slot) == SLOT_EFFECT_MISSILE) {
			const CREItem *slotitm = inventory.GetSlotItem(slot);
			assert(slotitm);
			const Item *itm = gamedata->GetItem(slotitm->ItemResRef, true);
			assert(itm);
			const ITMExtHeader *ext_header = itm->GetExtHeader(header);
			if (ext_header) {
				int type = ext_header->ProjectileQualifier;
				int weaponslot = inventory.FindTypedRangedWeapon(type);
				if (weaponslot == Inventory::GetFistSlot()) {
					empty = true;
				}
			} else {
				empty = true;
			}
			gamedata->FreeItem(itm,slotitm->ItemResRef, false);
		}
	}

	if (empty)
		SetupQuickSlot(ACT_WEAPON1 + which, ieWord(Inventory::GetFistSlot()), 0);
}

//if dual stuff needs to be handled on load too, improve this method with it
int Actor::GetHpAdjustment(int multiplier, bool modified) const
{
	int val;

	// only player classes get this bonus
	if (!HasPlayerClass()) {
		return 0;
	}

	const stats_t& stats = modified ? Modified : BaseStats;

	// GetClassLevel/IsWarrior takes into consideration inactive dual-classes, so those would fail here
	if (IsWarrior()) {
		val = core->GetConstitutionBonus(STAT_CON_HP_WARRIOR, stats[IE_CON]);
	} else {
		val = core->GetConstitutionBonus(STAT_CON_HP_NORMAL, stats[IE_CON]);
	}

	// ensure the change does not kill the actor
	if (BaseStats[IE_HITPOINTS] + val*multiplier <= 0) {
		// leave them with 1hp/level worth of hp
		// note: we return the adjustment and the actual setting of hp happens later
		return multiplier - BaseStats[IE_HITPOINTS];
	} else {
		return val * multiplier;
	}
}

void Actor::InitStatsOnLoad()
{
	//default is 9 in Tob, 6 in bg1
	SetBase(IE_MOVEMENTRATE, VOODOO_CHAR_SPEED);

	stat_t animID = BaseStats[IE_ANIMATION_ID];
	//this is required so the actor has animation already
	SetAnimationID(animID);

	// Setting up derived stats
	if (BaseStats[IE_STATE_ID] & STATE_DEAD) {
		SetStance( IE_ANI_TWITCH );
		Deactivate();
		InternalFlags|=IF_REALLYDIED;
	} else {
		if (BaseStats[IE_STATE_ID] & STATE_SLEEP) {
			SetStance( IE_ANI_SLEEP );
		} else if (anims && anims->GetAnimType() == IE_ANI_TWO_PIECE) {
			SetStance(IE_ANI_EMERGE);
			SetWait(15); // wait for it to play out
		} else {
			SetStance( IE_ANI_AWAKE );
		}
	}
	CreateDerivedStats();
	Modified[IE_CON]=BaseStats[IE_CON]; // used by GetHpAdjustment
	ieDword hp = BaseStats[IE_HITPOINTS] + GetHpAdjustment(GetXPLevel(false));
	BaseStats[IE_HITPOINTS]=hp;

	SetupFist();
	//initial setup of modified stats
	Modified = BaseStats;
}

//most feats are simulated via spells (feat<xx>)
void Actor::ApplyFeats()
{
	ResRef feat;

	for(int i=0;i<MAX_FEATS;i++) {
		int level = GetFeat(i);
		feat.Format("FEAT{:02x}", i);
		if (level) {
			if (gamedata->Exists(feat, IE_SPL_CLASS_ID, true)) {
				core->ApplySpell(feat, this, this, level);
			}
		}
	}
	//apply scripted feats
	ScriptEngine::FunctionParameters params;
	if (InParty) {
		params.push_back(ScriptEngine::Parameter(InParty));
	} else {
		params.push_back(ScriptEngine::Parameter(GetGlobalID()));
	}
	core->GetGUIScriptEngine()->RunFunction("LUCommon","ApplyFeats", params, true);
}

void Actor::ApplyExtraSettings()
{
	if (!PCStats) return;
	for (int i=0;i<ES_COUNT;i++) {
		if (!featSpells[i].IsEmpty() && !IsStar(featSpells[i])) {
			if (PCStats->ExtraSettings[i]) {
				core->ApplySpell(featSpells[i], this, this, PCStats->ExtraSettings[i]);
			}
		}
	}
}

void Actor::SetupQuickSlot(unsigned int which, ieWord slot, ieWord headerIndex) const
{
	if (!PCStats) return;
	PCStats->InitQuickSlot(which, slot, headerIndex);
	//something changed about the quick items
	core->SetEventFlag(EF_ACTION);
}

bool Actor::ValidTarget(int ga_flags, const Scriptable *checker) const
{
	//scripts can still see this type of actor

	if (ga_flags&GA_NO_SELF) {
		if (checker && checker == this) return false;
	}

	if (ga_flags & GA_NO_UNSCHEDULED && !InParty) {
		if (Modified[IE_AVATARREMOVAL]) return false;

		const Game *game = core->GetGame();
		if (game) {
			if (!Schedule(game->GameTime, true)) return false;
		}
	}

	if (ga_flags&GA_NO_HIDDEN) {
		if (IsInvisibleTo(checker)) return false;
	}

	if (ga_flags&GA_NO_ALLY) {
		if(InParty) return false;
		if(Modified[IE_EA]<=EA_GOODCUTOFF) return false;
	}

	if (ga_flags&GA_NO_ENEMY) {
		if(!InParty && (Modified[IE_EA]>=EA_EVILCUTOFF) ) return false;
	}

	if (ga_flags&GA_NO_NEUTRAL) {
		if((Modified[IE_EA]>EA_GOODCUTOFF) && (Modified[IE_EA]<EA_EVILCUTOFF) ) return false;
	}

	switch(ga_flags&GA_ACTION) {
	case GA_PICK:
		if (Modified[IE_STATE_ID] & STATE_CANTSTEAL) return false;
		break;
	case GA_TALK:
		//can't talk to dead
		if (Modified[IE_STATE_ID] & (STATE_CANTLISTEN^STATE_SLEEP)) return false;
		//can't talk to hostile
		if (Modified[IE_EA]>=EA_EVILCUTOFF) return false;
		// neither to bats and birds
		if (anims->GetCircleSize() == 0) return false;
		break;
	}
	if (ga_flags&GA_NO_DEAD) {
		if (InternalFlags&IF_REALLYDIED) return false;
		if (Modified[IE_STATE_ID] & STATE_DEAD) return false;
	}
	if (ga_flags&GA_SELECT) {
		if (UnselectableTimer) return false;
		if (Immobile()) return false;
		if (Modified[IE_STATE_ID] & (STATE_MINDLESS ^ (STATE_CHARMED|STATE_BERSERK))) {
			return false;
		}
		// charmed actors are only selectable if they were charmed by the party
		if ((Modified[IE_STATE_ID] & STATE_CHARMED) && Modified[IE_EA] == EA_CHARMEDPC) return false;
		if (Modified[IE_STATE_ID] & STATE_BERSERK) {
			if (Modified[IE_CHECKFORBERSERK]) return false;
		}
	}
	if (ga_flags & GA_ONLY_BUMPABLE) {
		if (core->InCutSceneMode()) return false;
		if (core->GetGame()->CombatCounter) return false;
		if (GetStat(IE_EA) >= EA_EVILCUTOFF) return false;
		// Skip sitting patrons
		if (GetStat(IE_ANIMATION_ID) >= 0x4000 && GetStat(IE_ANIMATION_ID) <= 0x4112) return false;
		if (IsMoving()) return false;
	}
	if (ga_flags & GA_CAN_BUMP) {
		if (core->InCutSceneMode()) return false;
		if (core->GetGame()->CombatCounter) return false;
		if (!((IsPartyMember() && GetStat(IE_EA) < EA_GOODCUTOFF) || GetStat(IE_NPCBUMP))) return false;
	}
	if (ga_flags & GA_BIGBAD) {
		ieDword animID = Modified[IE_ANIMATION_ID];
		// accept only a subset of ranges 0x1200-0x12FF and 0x1400-0x1FFF
		if (animID < 0x1200 || (animID >= 0x1300 && animID < 0x1400)) return false;
		if (animID >= 0x2000) return false;
		if ((animID & 0xf00) != 0x200 || (animID & 0xf) >= 9) return false;
	}
	return true;
}

//returns true if it won't be destroyed with an area
//in this case it shouldn't be saved with the area either
//it will be saved in the savegame
bool Actor::Persistent() const
{
	if (InParty) return true;
	if (InternalFlags&IF_FROMGAME) return true;
	return false;
}

//this is a reimplementation of cheatkey a/s of bg2
//cycling through animation/stance
// a - get next animation, s - get next stance

void Actor::GetNextAnimation()
{
	size_t RowNum = anims->AvatarsRowNum - 1;
	if (RowNum >= CharAnimations::GetAvatarsCount())
		RowNum = CharAnimations::GetAvatarsCount() - 1;
	int NewAnimID = CharAnimations::GetAvatarStruct(RowNum).AnimID;
	Log(DEBUG, "Actor", "AnimID: {:#X}", NewAnimID);
	SetBase( IE_ANIMATION_ID, NewAnimID);
}

void Actor::GetPrevAnimation()
{
	size_t RowNum = anims->AvatarsRowNum + 1;
	if (RowNum >= CharAnimations::GetAvatarsCount())
		RowNum = 0;
	int NewAnimID = CharAnimations::GetAvatarStruct(RowNum).AnimID;
	Log(DEBUG, "Actor", "AnimID: {:#X}", NewAnimID);
	SetBase( IE_ANIMATION_ID, NewAnimID);
}

int Actor::IsDualWielding() const
{
	// // if this function ever becomes redundant when populating WeaponInfo, it can then be simplified
	// if (inventory.MagicSlotEquipped() || inventory.FistsEquipped()) return 0; // probably not needed, but playing it safe
	// return (weaponInfo[0].extHeader && weaponInfo[1].extHeader) ? 1 : 0;
	int slot;
	//if the shield slot is a weapon, we're dual wielding
	const CREItem *wield = inventory.GetUsedWeapon(true, slot);
	if (!wield || slot == Inventory::GetFistSlot() || slot == Inventory::GetMagicSlot()) {
		return 0;
	}

	const Item *itm = gamedata->GetItem(wield->ItemResRef, true);
	if (!itm) {
		Log(WARNING, "Actor", "Missing or invalid wielded weapon item: {}!", wield->ItemResRef);
		return 0;
	}

	//if the item is usable in weapon slot, then it is weapon
	int weapon = core->CheckItemType(itm, SLOT_WEAPON);
	gamedata->FreeItem( itm, wield->ItemResRef, false );
	//is just weapon>0 ok?
	return (weapon>0)?1:0;
}

// returns weapon header currently used (arrow in case of bow + arrow)
const ITMExtHeader* Actor::GetWeapon(bool leftOrRight) const
{
	//only use the shield slot if we are dual wielding
	leftOrRight = leftOrRight && IsDualWielding();
	return weaponInfo[leftOrRight].extHeader;
}

void Actor::GetNextStance()
{
	static int Stance = IE_ANI_AWAKE;

	if (--Stance < 0) Stance = MAX_ANIMS-1;
	Log(DEBUG, "Actor", "StanceID: {}", Stance);
	SetStance( Stance );
}

int Actor::LearnSpell(const ResRef& spellname, ieDword flags, int bookmask, int level)
{
	//don't fail if the spell is also memorized (for innates)
	if (! (flags&LS_MEMO)) {
		if (spellbook.HaveSpell(spellname, 0) ) {
			return LSR_KNOWN;
		}
	}
	Spell *spell = gamedata->GetSpell(spellname);
	if (!spell) {
		return LSR_INVALID; //not existent spell
	}

	//innates are always memorized when gained
	if (spell->SpellType==IE_SPL_INNATE) {
		flags|=LS_MEMO;
	}

	ieDword kit = GetStat(IE_KIT);

	if ((flags & LS_STATS) && (GameDifficulty>DIFF_NORMAL) ) {
		// chance to learn roll
		int roll = LuckyRoll(1, 100, 0);
		// adjust the roll for specialist mages
		// doesn't work in bg1, since its spells don't have PrimaryType set (0 is NONE)
		if (!third && GetKitIndex(kit) && spell->PrimaryType) {
			if (kit == (unsigned) 1<<(spell->PrimaryType+5)) { // +5 since the kit values start at 0x40
				roll += 15;
			} else {
				roll -= 15;
			}
		}

		if (roll > core->GetIntelligenceBonus(0, GetStat(IE_INT))) {
			return LSR_FAILED;
		}
	}

	// only look it up if none was passed
	if (bookmask == -1) {
		bookmask = GetBookMask();
	}
	int explev = spellbook.LearnSpell(spell, flags&LS_MEMO, bookmask, kit, level);
	HCStrings message = HCStrings::count;
	if (flags&LS_LEARN) {
		core->GetTokenDictionary()["SPECIALABILITYNAME"] = core->GetString(spell->SpellName);
		switch (spell->SpellType) {
		case IE_SPL_INNATE:
			message = HCStrings::GotAbility;
			break;
		case IE_SPL_SONG:
			message = HCStrings::GotSong;
			break;
		default:
			message = HCStrings::GotSpell;
			break;
		}
	}
	gamedata->FreeSpell(spell, spellname, false);
	if (!explev) {
		return LSR_INVALID;
	}
	if (message != HCStrings::count) {
		displaymsg->DisplayConstantStringName(message, GUIColors::XPCHANGE, this);
	}
	if (flags&LS_ADDXP && !(flags&LS_NOXP)) {
		int xp = gamedata->GetXPBonus(XP_LEARNSPELL, explev);
		const Game *game = core->GetGame();
		game->ShareXP(xp, SX_DIVIDE);
	}
	return LSR_OK;
}

void Actor::SetDialog(const ResRef &resref)
{
	Dialog = resref;
}

Holder<Sprite2D> Actor::CopyPortrait(int which) const
{
	ResRef portrait = which ? SmallPortrait : LargePortrait;
	if (portrait == "none") return nullptr; // skip our fallback

	ResourceHolder<ImageMgr> im = gamedata->GetResourceHolder<ImageMgr>(portrait, true);
	return im ? im->GetSprite2D() : nullptr;
}

ResRef Actor::GetDialog(int flags) const
{
	if (!flags) {
		return Dialog;
	}
	if (Modified[IE_EA]>=EA_EVILCUTOFF) {
		return ResRef();
	}

	if ( (InternalFlags & IF_NOINT) && CurrentAction) {
		if (flags > GD_CHECK) {
			core->GetTokenDictionary()["TARGET"] = ShortName;
			displaymsg->DisplayConstantString(HCStrings::TargetBusy, GUIColors::RED);
		}
		return ResRef();
	}
	return Dialog;
}

std::list<int> Actor::ListLevels() const
{
	std::list<int> levels (ISCLASSES, 0);
	if (third) {
		int i = 0;
		for (auto& level : levels) {
			level = GetClassLevel(i++);
		}
	}
	return levels;
}

void Actor::CreateStats()
{
	if (!PCStats) {
		PCStats = new PCStatsStruct(ListLevels());
	}
}

ResRef Actor::GetScript(int ScriptIndex) const
{
	if (Scripts[ScriptIndex]) {
		return Scripts[ScriptIndex]->GetName();
	} else {
		return "NONE";
	}
}

// similar manipulation as PermanentStatChangeFeedback and DisplayMessage::StrRefs::Get
inline ieStrRef PersonalizePSTString(ieStrRef ref, const Actor* pc)
{
	if (pc->Modal.State != Modal::Stealth) return ref;
	if (!core->HasFeature(GFFlags::PST_STATE_FLAGS)) return ref;

	int pcOffset = 8;
	int specific = pc->GetStat(IE_SPECIFIC);
	const std::array<int, 8> spec2offset = { 0, 7, 5, 6, 4, 3, 2, 1 };
	if (specific >= 2 && specific <= 9) {
		pcOffset = spec2offset[specific - 2];
	}
	return ieStrRef(int(ref) + pcOffset);
}

void Actor::SetModal(enum Modal newstate, bool force)
{
	switch(newstate) {
		case Modal::None:
			break;
		case Modal::BattleSong:
			break;
		case Modal::DetectTraps:
			break;
		case Modal::Stealth:
			break;
		case Modal::TurnUndead:
			break;
		default:
			return;
	}

	if (Modal.State != newstate) {
		Modal.FirstApply = true;
	}

	if (Modal.State == Modal::BattleSong && Modal.State != newstate && HasFeat(FEAT_LINGERING_SONG)) {
		Modal.LingeringSpell = Modal.Spell;
		Modal.LingeringCount = 2;
	}

	if (IsSelected()) {
		// display the turning-off message
		if (Modal.State != Modal::None && core->HasFeedback(FT_MISC)) {
			ieStrRef leaving = PersonalizePSTString(ModalStates[Modal.State].leaving_str, this);
			displaymsg->DisplayStringName(leaving, GUIColors::WHITE, this, STRING_FLAGS::SOUND | STRING_FLAGS::SPEECH);
		}

		//update the action bar
		if (Modal.State != newstate || newstate != Modal::None) {
			core->SetEventFlag(EF_ACTION);
		}

		// when called with the same state twice, toggle to MS_NONE
		if (!force && Modal.State == newstate) {
			Modal.State = Modal::None;
		} else {
			Modal.State = newstate;
		}
	} else {
		Modal.State = newstate;
	}
}

void Actor::SetModalSpell(enum Modal state, const ResRef& spell)
{
	if (spell) {
		Modal.Spell = spell;
	} else {
		if (size_t(state) >= ModalStates.size) {
			Modal.Spell.Reset();
		} else {
			if (state == Modal::BattleSong && !BardSong.IsEmpty()) {
				Modal.Spell = BardSong;
				return;
			}
			Modal.Spell = ModalStates[state].spell;
		}
	}
}

void Actor::AttackedBy(const Actor *attacker)
{
	AddTrigger(TriggerEntry(trigger_attackedby, attacker->GetGlobalID()));
	if (attacker->GetStat(IE_EA) != EA_PC && Modified[IE_EA] != EA_PC) {
		objects.LastAttacker = attacker->GetGlobalID();
	}
	if (InParty) {
		core->Autopause(AUTOPAUSE::ATTACKED, this);
	}
}

void Actor::FaceTarget(const Scriptable *target)
{
	if (!target) return;
	SetOrientation(target->Pos, Pos, false);
}

//in case of LastTarget = 0
void Actor::StopAttack()
{
	SetStance(IE_ANI_READY);
	lastattack = 0;
	secondround = false;
	//InternalFlags|=IF_TARGETGONE; //this is for the trigger!
	if (InParty) {
		core->Autopause(AUTOPAUSE::NOTARGET, this);
	}
}

// checks for complete immobility — to the point of inaction
int Actor::Immobile() const
{
	if (GetStat(IE_CASTERHOLD)) {
		return 1;
	}
	if (GetStat(IE_HELD)) {
		return 1;
	}
	if (GetStat(IE_STATE_ID) & STATE_STILL) {
		return 1;
	}
	const Game *game = core->GetGame();
	if (game && game->TimeStoppedFor(this)) {
		return 1;
	}

	return 0;
}

void Actor::DoStep(unsigned int newWalkScale, ieDword time)
{
	if (Immobile()) {
		return;
	}
	Movable::DoStep(newWalkScale, time);
}

ieDword Actor::GetNumberOfAttacks()
{
	int base = 0;
	int bonus = 0;

	if (third) {
		base = SetBaseAPRandAB (true);
		// effects and everything else is stored with double values
		int modified = GetStat(IE_NUMBEROFATTACKS);
		if (modified > base) base = modified; // a heavyhanded approach, but should work
		// add the offhand extra attack
		bonus = 2 * IsDualWielding();
		// handle special effects
		const Effect* fx = fxqueue.HasEffectWithParam(fx_set_diseased_state_ref, RPD_SLOW);
		if (fx) bonus -= 2;
		fx = fxqueue.HasEffectWithParam(fx_set_diseased_state_ref, RPD_CONTAGION);
		if (fx) bonus -= 2;
	} else {
		base = GetStat(IE_NUMBEROFATTACKS);
		if (inventory.FistsEquipped()) {
			bonus = gamedata->GetMonkBonus(0, GetMonkLevel());
		}
	}
	return base + bonus;
}
static const int BaseAttackBonusDecrement = 5; // iwd2; number of tohit points for another attack per round
static int SetLevelBAB(int level, ieDword index)
{
	if (!level) {
		return 0;
	}
	assert(index < BABClassMap.size());

	const auto& table = IWD2HitTable.find(BABClassMap[index]);
	assert(table != IWD2HitTable.end());
	return table->second[level-1].bab;
}

// return the base APR derived from the base attack bonus, which we have to construct here too
//NOTE: this doesn't break iwd2 monsters, since they have their level stored as fighters (if not more)
int Actor::SetBaseAPRandAB(bool CheckRapidShot)
{
	int pBAB = 0;
	int pBABDecrement = BaseAttackBonusDecrement;
	ieDword MonkLevel = 0;
	ieDword LevelSum = 0;

	if (!third) {
		ToHit.SetBase(BaseStats[IE_TOHIT]);
		return 0;
	}

	for (int i = 0; i < ISCLASSES; i++) {
		int level = GetClassLevel(i);
		if (!level) continue;

		// silly monks, always wanting to be special
		if (i == ISMONK) {
			MonkLevel = level;
			if (MonkLevel + LevelSum == Modified[IE_CLASSLEVELSUM]) {
				// only the monk left to check, so skip the rest
				break;
			} else {
				continue;
			}
		}
		pBAB += SetLevelBAB(level, i);
		LevelSum += level;
		if (LevelSum == Modified[IE_CLASSLEVELSUM]) {
			// skip to apr calc, no need to check the other classes
			ToHit.SetBase(pBAB);
			ToHit.SetBABDecrement(pBABDecrement);
			return BAB2APR(pBAB, pBABDecrement, CheckRapidShot);
		}
	}

	if (MonkLevel) {
		// act as a rogue unless barefisted and without armor
		// multiclassed monks only use their monk levels when determining barefisted bab
		// check the spell failure instead of the skill penalty, since otherwise leather armor would also be treated as none
		if (!inventory.FistsEquipped() || GetTotalArmorFailure()) {
			pBAB += SetLevelBAB(MonkLevel, ISTHIEF);
		} else {
			pBABDecrement = 3;
			pBAB = SetLevelBAB(MonkLevel, ISMONK);
		}
		LevelSum += MonkLevel;
	}

	assert(LevelSum == Modified[IE_CLASSLEVELSUM]);
	ToHit.SetBase(pBAB);
	ToHit.SetBABDecrement(pBABDecrement);
	return BAB2APR(pBAB, pBABDecrement, CheckRapidShot);
}

int Actor::BAB2APR(int pBAB, int pBABDecrement, int CheckRapidShot) const
{
	if (CheckRapidShot && HasSpellState(SS_RAPIDSHOT) && weaponInfo[0].extHeader) {
		ieDword AttackTypeLowBits = weaponInfo[0].extHeader->AttackType & 0xFF; // this is done by the original; leaving in case we expand
		if (AttackTypeLowBits == ITEM_AT_BOW || AttackTypeLowBits == ITEM_AT_PROJECTILE) {
			// rapid shot gives another attack and since it is computed from the BAB, we just increase that ...
			// but monk get their speedy handy work only for fists, so we can't use the passed pBABDecrement
			pBAB += BaseAttackBonusDecrement;
		}
	}

	int APR = (pBAB - 1) / pBABDecrement + 1;
	//FIXME: why is it not using the other IWD2HitTable column? Less moddable this way
	// the original hardcoded this, but we can do better - all the data is already in the tables
	// HOWEVER, what to do with multiclass characters? -> check the monk table, since it is prone to have the highest values?
	// additionally, 5 is the real max, but not without dualwielding or effects
	if (APR > 4) {
		APR = 4;
	}
	// NOTE: we currently double the value, since it is stored doubled in other games and effects rely on it
	// if you want to change it, don't forget to do the same for the bonus in GetNumberOfAttacks
	return APR*2;
}

//calculate how many attacks will be performed
//in the next round
//only called when Game thinks we are in attack
//so it is safe to do cleanup here (it will be called only once)
void Actor::InitRound(ieDword gameTime)
{
	lastInit = gameTime;
	secondround = !secondround;

	//reset variables used in PerformAttack
	attackcount = 0;
	attacksperround = 0;
	nextattack = 0;
	lastattack = 0;

	//add one for second round to get an extra attack only if we
	//are x/2 attacks per round
	attackcount = GetNumberOfAttacks();
	if (secondround) {
		attackcount++;
	}
	//all numbers of attacks are stored at twice their value
	attackcount >>= 1;

	//make sure we always get at least 1apr
	// but only if it wasn't 0 from the start, like rats in Candlekeep
	if (attackcount < 1 && BaseStats[IE_NUMBEROFATTACKS] != 0) {
		attackcount = 1;
	}

	//set our apr and starting round time
	attacksperround = attackcount;
	roundTime = gameTime;

	//print a little message :)
	Log(MESSAGE, "InitRound", "Name: {} | Attacks: {} | Start: {}", fmt::WideToChar{ShortName}, attacksperround, gameTime);

	// this might not be the right place, but let's give it a go
	if (attacksperround && InParty) {
		core->Autopause(AUTOPAUSE::ENDROUND, this);
	}
}

int Actor::GetStars(stat_t proficiency) const
{
	int stars = GetStat(proficiency) & PROFS_MASK;
	if (stars > STYLE_STAR_MAX) stars = STYLE_STAR_MAX;
	return stars;
}

// iwd2 adds a -4 nonprof penalty, while others have values that differ by class
int Actor::GetNonProficiencyPenalty(int stars) const
{
	int prof = 0;

	// iwd2 mode ... but everyone is proficient with fists
	if (!inventory.FistsEquipped()) {
		prof += gamedata->GetWSpecialBonus(0, stars);
	}

	// add non-proficiency penalty for the rest of the games
	// stored negative
	if (stars == 0 && !third) {
		ieDword clss = GetActiveClass();
		// is it a PC class?
		if (clss < (ieDword) classcount) {
			// but skip fists, since they don't have a proficiency
			if (!inventory.FistsEquipped()) {
				prof += noProfPenalty[clss];
			}
		} else {
			// it is not clear what is the penalty for non player classes
			prof -= 4;
		}
	}

	return prof;
}

// adds weapon style and weapon-specific proficiency bonuses
// still ugly due to all the side-effects
int Actor::GetProficiencyBonus(int& style, bool leftOrRight, int& damageBonus, int& speedBonus, int& criticalBonus) const
{
	ieDword dualWielding = IsDualWielding();
	const WeaponInfo& wi = weaponInfo[leftOrRight && dualWielding];

	// Elves get a racial THAC0 bonus with swords and bows, halflings with slings
	int prof = gamedata->GetRacialTHAC0Bonus(wi.prof, GetRaceName());

	if (third) {
		if (!dualWielding) return prof;

		// iwd2 gives a dualwielding bonus when using a simple weapon in the offhand
		// it is limited to shortswords and daggers, which also have this flag set
		// the bonus is applied to both hands
		if (weaponInfo[1].wflags & WEAPON_FINESSE) {
			prof += 2;
		}

		// rangers wearing light or no armor gain ambidexterity and
		// two-weapon-fighting feats for free
		bool ambidextrous = HasFeat(FEAT_AMBIDEXTERITY);
		bool twoWeaponFighting = HasFeat(FEAT_TWO_WEAPON_FIGHTING);
		if (GetRangerLevel()) {
			ieWord armorType = inventory.GetArmorItemType();
			if (GetArmorWeightClass(armorType) <= 1) {
				ambidextrous = true;
				twoWeaponFighting = true;
			}
		}

		// FIXME: externalise
		// penalites and boni for both hands:
		// -6 main, -10 off with no adjustments
		//  0 main, +4 off with ambidexterity
		// +2 main, +2 off with two weapon fighting
		// +2 main, +2 off with a simple weapons in the off hand
		// so a minimum penalty of -2, -2
		if (twoWeaponFighting) {
			prof += 2;
		}
		if (wi.wflags & WEAPON_LEFTHAND) {
			prof -= 6;
		} else {
			prof -= 10;
			if (ambidextrous) {
				prof += 4;
			}
		}

		return prof;
	}

	int styleIdx = -1;
	int stars = 0;
	if (dualWielding) {
		// add dual wielding penalty
		stars = GetStars(IE_PROFICIENCY2WEAPON);
		style = 1000 * stars + IE_PROFICIENCY2WEAPON;
		styleIdx = 0;
		prof += gamedata->GetWeaponStyleBonus(0, stars, leftOrRight ? 4 : 3);
	} else if (wi.itemflags & IE_INV_ITEM_TWOHANDED && wi.wflags & WEAPON_MELEE) {
		// add two handed profs bonus
		stars = GetStars(IE_PROFICIENCY2HANDED);
		style = 1000 * stars + IE_PROFICIENCY2HANDED;
		styleIdx = 1;
	} else if (wi.wflags & WEAPON_MELEE) {
		int slot;
		const CREItem* weapon = inventory.GetUsedWeapon(true, slot);
		if (weapon == nullptr) {
			// no weapon means no shield slot
			stars = GetStars(IE_PROFICIENCYSINGLEWEAPON);
			style = 1000 * stars + IE_PROFICIENCYSINGLEWEAPON;
			styleIdx = 3;
		} else {
			// sword and shield
			stars = GetStars(IE_PROFICIENCYSWORDANDSHIELD);
			style = 1000 * stars + IE_PROFICIENCYSWORDANDSHIELD;
			styleIdx = 2;
		}
	} else {
		// ranged - no bonus
	}

	if (styleIdx != -1) {
		damageBonus += gamedata->GetWeaponStyleBonus(styleIdx, stars, 2);
		speedBonus += gamedata->GetWeaponStyleBonus(styleIdx, stars, 5);
		criticalBonus = gamedata->GetWeaponStyleBonus(styleIdx, stars, 1);
		if (styleIdx != 0) {
			// right hand bonus; dualwielding was already considered above
			prof += gamedata->GetWeaponStyleBonus(styleIdx, stars, 3);
		}
	}

	return prof;
}

bool Actor::GetCombatDetails(int& toHit, bool leftOrRight, int& damageBonus, \
		int& speed, int& criticalBonus, int& style, const Actor* target)
{
	SetBaseAPRandAB(true);
	ieDword dualwielding = IsDualWielding();
	WeaponInfo& wi = weaponInfo[leftOrRight && dualwielding];
	const ITMExtHeader* hittingheader = wi.extHeader;
	if (!hittingheader) return false; // item is unsuitable for a fight

	int THAC0Bonus = hittingheader->THAC0Bonus + wi.launcherTHAC0Bonus;
	if (ReverseToHit) THAC0Bonus = -THAC0Bonus;
	ToHit.SetWeaponBonus(THAC0Bonus);

	damageBonus = hittingheader->DamageBonus + wi.launcherDmgBonus;
	// get our dual wielding modifier
	if (dualwielding) {
		if (leftOrRight) {
			damageBonus += GetStat(IE_DAMAGEBONUSLEFT);
		} else {
			damageBonus += GetStat(IE_DAMAGEBONUSRIGHT);
		}
	}
	damageBonus += GetStat(IE_DAMAGEBONUS);

	// add in proficiency bonuses
	ieDword stars = GetProficiency(wi.prof)&PROFS_MASK;

	// tenser's transformation makes the actor proficient in any weapons
	// also conjured weapons are wielded without penalties
	if (!stars && (HasSpellState(SS_TENSER) || inventory.MagicSlotEquipped())) {
		stars = 1;
	}

	wi.profdmgbon = gamedata->GetWSpecialBonus(1, stars);
	damageBonus += wi.profdmgbon;
	speed = - (int) GetStat(IE_PHYSICALSPEED);
	// only bg2 wspecial.2da has this column, but all have 0 as the default
	// table value, so this lookup is fine
	speed += gamedata->GetWSpecialBonus(2, stars);

	// racial enemies suffer 4hp more in all games
	int favoredEnemy = GetRacialEnemyBonus(target);
	if (GetRangerLevel() && favoredEnemy) {
		damageBonus += favoredEnemy;
	}

	style = 0;
	criticalBonus = 0;
	int prof = GetNonProficiencyPenalty(stars);
	prof += GetProficiencyBonus(style, leftOrRight, damageBonus, speed, criticalBonus);
	if (ReverseToHit) {
		prof = -prof;
	}
	AutoTable classBonus = gamedata->LoadTable("clasthac", true);
	if (classBonus) { // bonuses are stored negative, so we apply them after the inversion above
		ieDword kit = Modified[IE_KIT];
		std::string className = GetClassName(GetActiveClass());
		prof += classBonus->QueryFieldSigned<int>("BONUS", GetKitName(kit));
		prof += classBonus->QueryFieldSigned<int>("BONUS", className);
	}
	ToHit.SetProficiencyBonus(prof);

	// get the remaining boni
	// FIXME: merge
	toHit = GetToHit(wi.wflags, target);

	//pst increased critical hits
	if (pstflags && (Modified[IE_STATE_ID]&STATE_CRIT_ENH)) {
		criticalBonus--;
	}
	return true;
}

int Actor::MeleePenalty() const
{
	if (GetMonkLevel()) return 0;
	if (inventory.FistsEquipped()) return -4;
	return 0;
}

//FIXME: can get called on its own and ToHit could erroneusly give weapon and some prof boni in that case
int Actor::GetToHit(ieDword Flags, const Actor *target)
{
	int generic = 0;
	int attacknum = attackcount;

	//get our dual wielding modifier
	if (IsDualWielding()) {
		if (Flags&WEAPON_LEFTHAND) {
			generic = GetStat(IE_HITBONUSLEFT);
			attacknum = 1; // shouldn't be needed, but let's play safe
		} else {
			generic = GetStat(IE_HITBONUSRIGHT);
			attacknum--; // remove 1, since it is for the other hand (otherwise we would never use the max tohit for this hand)
		}
	}

	// set up strength/dexterity boni
	GetTHAbilityBonus(Flags);

	// check if there is any armor unproficiency penalty
	int am = 0, sm = 0;
	GetArmorSkillPenalty(1, am, sm);
	ToHit.SetArmorBonus(-am);
	ToHit.SetShieldBonus(-sm);

	//get attack style (melee or ranged)
	switch(Flags&WEAPON_STYLEMASK) {
		case WEAPON_MELEE:
			generic += GetStat(IE_MELEETOHIT);
			break;
		case WEAPON_FIST:
			generic += GetStat(IE_FISTHIT);
			break;
		case WEAPON_RANGED:
			generic += GetStat(IE_MISSILEHITBONUS);
			break;
	}

	if (target) {
		// if the target is using a ranged weapon while we're meleeing, we get a +4 bonus
		if ((Flags & WEAPON_STYLEMASK) != WEAPON_RANGED && target->weaponInfo[0].wflags & WEAPON_RANGED) {
			generic += 4;
		}

		// melee vs. unarmed
		generic += target->MeleePenalty() - MeleePenalty();

		// add +4 attack bonus vs racial enemies
		if (GetRangerLevel()) {
			generic += GetRacialEnemyBonus(target);
		}
		generic += fxqueue.BonusAgainstCreature(fx_tohit_vs_creature_ref, target);

		// close-quarter ranged penalties; let's say roughly max 1 foot apart
		if (third && (Flags & WEAPON_STYLEMASK) == WEAPON_RANGED && WithinPersonalRange(target, this, 2)) {
			generic -= 4;
			if (!HasFeat(FEAT_PRECISE_SHOT)) {
				generic -= 4;
			}
		}
	}

	// add generic bonus
	generic += GetStat(IE_HITBONUS);

	// now this func is the only one to modify generic bonus, so no need to add
	if (ReverseToHit) {
		ToHit.SetGenericBonus(-generic);
		return ToHit.GetTotal();
	} else {
		ToHit.SetGenericBonus(generic); // flat out cumulative
		return ToHit.GetTotalForAttackNum(attacknum);
	}
}

void Actor::GetTHAbilityBonus(ieDword Flags)
{
	int dexbonus = 0, strbonus = 0;
	// add strength bonus (discarded for ranged weapons later)
	if (Flags&WEAPON_USESTRENGTH || Flags&WEAPON_USESTRENGTH_HIT) {
		if (third) {
			strbonus = GetAbilityBonus(IE_STR );
		} else {
			strbonus = core->GetStrengthBonus(0,GetStat(IE_STR), GetStat(IE_STREXTRA) );
		}
	}

	//get attack style (melee or ranged)
	switch(Flags&WEAPON_STYLEMASK) {
		case WEAPON_MELEE:
			if ((Flags&WEAPON_FINESSE) && HasFeat(FEAT_WEAPON_FINESSE) ) {
				if (third) {
					dexbonus = GetAbilityBonus(IE_DEX );
				} else {
					dexbonus = core->GetDexterityBonus(STAT_DEX_MISSILE, GetStat(IE_DEX));
				}
				// weapon finesse is not cumulative
				if (dexbonus > strbonus) {
					strbonus = 0;
				} else {
					dexbonus = 0;
				}
			}
			break;
		case WEAPON_RANGED:
			//add dexterity bonus
			if (third) {
				dexbonus = GetAbilityBonus(IE_DEX);
			} else {
				dexbonus = core->GetDexterityBonus(STAT_DEX_MISSILE, GetStat(IE_DEX));
			}
			// WEAPON_USESTRENGTH only affects weapon damage, WEAPON_USESTRENGTH_HIT unknown
			strbonus = 0;
			break;
		// no ability tohit bonus for WEAPON_FIST
	}

	// both strength and dex bonus are stored positive only in iwd2
	if (third) {
		ToHit.SetAbilityBonus(dexbonus + strbonus);
	} else {
		ToHit.SetAbilityBonus(-(dexbonus + strbonus));
	}
}

int Actor::GetDefense(int DamageType, ieDword wflags, const Actor *attacker) const
{
	//specific damage type bonus.
	int defense = 0;
	if(DamageType > 5)
		DamageType = 0;
	switch (weapon_damagetype[DamageType]) {
	case DAMAGE_CRUSHING:
		defense += GetStat(IE_ACCRUSHINGMOD);
		break;
	case DAMAGE_PIERCING:
		defense += GetStat(IE_ACPIERCINGMOD);
		break;
	case DAMAGE_SLASHING:
		defense += GetStat(IE_ACSLASHINGMOD);
		break;
	case DAMAGE_MISSILE:
		defense += GetStat(IE_ACMISSILEMOD);
		break;
	//What about stunning ?
	default :
		break;
	}


	//check for s/s and single weapon ac bonuses
	if (!IsDualWielding()) {
		const ITMExtHeader* header = GetWeapon(false);
		//make sure we're wielding a single melee weapon
		if (header && (header->AttackType == ITEM_AT_MELEE)) {
			int slot;
			ieDword stars;
			if (inventory.GetUsedWeapon(true, slot) == NULL) {
				//single-weapon style applies to all ac
				stars = GetStars(IE_PROFICIENCYSINGLEWEAPON);
				defense += gamedata->GetWeaponStyleBonus(3, stars, 0);
			} else if (weapon_damagetype[DamageType] == DAMAGE_MISSILE) {
				//sword-shield style applies only to missile ac
				stars = GetStars(IE_PROFICIENCYSWORDANDSHIELD);
				defense += gamedata->GetWeaponStyleBonus(2, stars, 6);
			}
		}
	}

	if (wflags&WEAPON_BYPASS) {
		if (ReverseToHit) {
			// deflection is used to store the armor value in adnd
			defense = AC.GetTotal() - AC.GetDeflectionBonus() + defense;
		} else {
			defense += AC.GetTotal() - AC.GetArmorBonus() - AC.GetShieldBonus();
		}
	} else {
		if (ReverseToHit) {
			defense = AC.GetTotal() + defense;
		} else {
			defense += AC.GetTotal();
		}
	}

	// is the attacker invisible? We don't care if we know the right uncanny dodge
	if (third && attacker && attacker->GetStat(state_invisible)) {
		if ((GetStat(IE_UNCANNY_DODGE) & 0x100) == 0) {
			// oops, we lose the dex bonus (like flatfooted)
			defense -= AC.GetDexterityBonus();
		}
	}

	if (attacker) {
		defense -= fxqueue.BonusAgainstCreature(fx_ac_vs_creature_type_ref,attacker);
	}
	return defense;
}

bool Actor::IsCriticalEffectEligible(const WeaponInfo& wi, const Effect* fx)
{
	// does it work only on the currently hitting weapon?
	if (fx->Parameter2 == 1 && fx->SourceRef != wi.item->Name) return false;

	// does it work on the currently hitting weapon's category (itemcat.ids / itemtype.2da)?
	if (fx->Parameter3 && fx->Parameter3 != wi.item->ItemType) return false;

	// does the attack type match?
	if (fx->IsVariable == 1 && wi.extHeader->AttackType != ITEM_AT_MELEE) return false;
	if (fx->IsVariable == 2 && (wi.extHeader->AttackType != ITEM_AT_BOW && wi.extHeader->AttackType != ITEM_AT_PROJECTILE)) return false;
	if (fx->IsVariable == 3 && wi.extHeader->AttackType != ITEM_AT_MAGIC) return false;

	return true;
}

static void ApplyCriticalEffect(Actor* actor, Actor* target, const WeaponInfo& wi, bool hit)
{
	static EffectRef fx_cast_on_critical_hit_ref = { "CastSpellOnCriticalHit", -1 };
	static EffectRef fx_cast_on_critical_miss_ref = { "CastSpellOnCriticalMiss", -1 };
	const Effect* fx;
	if (hit) {
		fx = actor->fxqueue.HasEffect(fx_cast_on_critical_hit_ref);
	} else {
		fx = actor->fxqueue.HasEffect(fx_cast_on_critical_miss_ref);
	}
	if (!fx) return;

	if (!Actor::IsCriticalEffectEligible(wi, fx)) return;

	core->ApplySpell(fx->Resource, target, actor, actor->GetXPLevel(false));
}

void Actor::PerformAttack(ieDword gameTime)
{
	static int attackRollDiceSides = gamedata->GetMiscRule("ATTACK_ROLL_DICE_SIDES");

	// don't let imprisoned or otherwise missing actors continue their attack
	if (Modified[IE_AVATARREMOVAL]) return;

	if (InParty) {
		// TODO: this is temporary hack
		Game *game = core->GetGame();
		game->PartyAttack = true;
	}

	if (!roundTime || (gameTime-roundTime > core->Time.attack_round_size)) { // the original didn't use a normal round
		// TODO: do we need cleverness for secondround here?
		InitRound(gameTime);
	}

	//only return if we don't have any attacks left this round
	if (attackcount==0) {
		// this is also part of the UpdateActorState hack below. sorry!
		lastattack = gameTime;
		return;
	}

	// this check shouldn't be necessary, but it causes a divide-by-zero below,
	// so i would like it to be clear if it ever happens
	if (attacksperround==0) {
		Log(ERROR, "Actor", "APR was 0 in PerformAttack!");
		return;
	}

	//don't continue if we can't make the attack yet
	//we check lastattack because we will get the same gameTime a few times
	if ((nextattack > gameTime) || (gameTime == lastattack)) {
		// fuzzie added the following line as part of the UpdateActorState hack below
		lastattack = gameTime;
		return;
	}

	if (IsDead()) {
		// this should be avoided by the AF_ALIVE check by all the calling actions
		Log(ERROR, "Actor", "Attack by dead actor!");
		return;
	}

	if (!objects.LastTarget) {
		Log(ERROR, "Actor", "Attack without valid target ID!");
		return;
	}
	//get target
	Actor* target = area->GetActorByGlobalID(objects.LastTarget);
	if (!target) {
		Log(WARNING, "Actor", "Attack without valid target!");
		return;
	}

	// also start CombatCounter if a pc is attacked
	if (!InParty && target->IsPartyMember()) {
		core->GetGame()->PartyAttack = true;
	}

	assert(!(target->IsInvisibleTo((Scriptable *) this) || (target->GetSafeStat(IE_STATE_ID) & STATE_DEAD)));
	target->AttackedBy(this);
	ieDword state = GetStat(IE_STATE_ID);
	if (state&STATE_BERSERK) {
		BaseStats[IE_CHECKFORBERSERK]=3;
	}

	Log(DEBUG, "Actor", "Performattack for {}, target is: {}", fmt::WideToChar{GetShortName()}, fmt::WideToChar{target->GetShortName()});

	//which hand is used
	//we do apr - attacksleft so we always use the main hand first
	// however, in 3ed, only one attack can be made by the offhand
	if (third) {
		usedLeftHand = false;
		// make only the last attack with the offhand (iwd2)
		if (attackcount == 1 && IsDualWielding()) {
			usedLeftHand = true;
		}
	} else {
		usedLeftHand = (bool) ((attacksperround - attackcount) & 1);
	}

	WeaponInfo& wi = weaponInfo[usedLeftHand];
	if (!wi.extHeader && usedLeftHand) {
		// nothing in left hand, use right
		wi = weaponInfo[0];
		usedLeftHand = false;
	}

	const ITMExtHeader* hittingheader = wi.extHeader;
	int tohit;
	int DamageBonus, CriticalBonus;
	int speed, style;

	//will return false on any errors (eg, unusable weapon)
	if (!GetCombatDetails(tohit, usedLeftHand, DamageBonus, speed, CriticalBonus, style, target)) {
		return;
	}

	if (PCStats) {
		PCStats->RegisterFavourite(weaponInfo[usedLeftHand && IsDualWielding()].item->Name, FAV_WEAPON);
	}

	//if this is the first call of the round, we need to update next attack
	if (nextattack == 0) {
		// initiative calculation (lucky 1d6-1 + item speed + speed stat + constant):
		// speed contains the bonus from the physical speed stat and the proficiency level
		int spdfactor = hittingheader->Speed + speed;
		if (spdfactor<0) spdfactor = 0;
		// -3: k/2 in the original, hardcoded to 6; -1 for the difference in rolls - the original rolled 0-5
		spdfactor = Clamp(spdfactor + LuckyRoll(1, 6, -4, LR_NEGATIVE), 0, 10);

		//(round_size/attacks_per_round)*(initiative) is the first delta
		nextattack = core->Time.round_size*spdfactor/(attacksperround*10) + gameTime;

		//we can still attack this round if we have a speed factor of 0
		if (nextattack > gameTime) {
			return;
		}
	}

	if (!WithinPersonalRange(this, target, GetWeaponRange(usedLeftHand)) || GetCurrentArea() != target->GetCurrentArea()) {
		// this is a temporary double-check, remove when bugfixed
		Log(ERROR, "Actor", "Attack action didn't bring us close enough!");
		return;
	}

	SetStance(AttackStance);
	PlaySwingSound(wi);

	//figure out the time for our next attack since the old time has the initiative
	//in it, we only have to add the basic delta
	attackcount--;
	nextattack += (core->Time.round_size/attacksperround);
	lastattack = gameTime;

	std::string buffer;
	//debug messages
	if (usedLeftHand && IsDualWielding()) {
		buffer.append("(Off) ");
	} else {
		buffer.append("(Main) ");
	}
	if (attacksperround) {
		AppendFormat(buffer, "Left: {} | ", attackcount);
		AppendFormat(buffer, "Next: {} ", nextattack);
	}
	if (fxqueue.HasEffectWithParam(fx_puppetmarker_ref, 1) || fxqueue.HasEffectWithParam(fx_puppetmarker_ref, 2)) { // illusions can't hit
		ResetState();
		buffer.append("[Missed (puppet)]");
		Log(COMBAT, "Attack", "{}", buffer);
		return;
	}

	// iwd2 smite evil only lasts for one attack, but has an insane duration, so remove it manually
	if (HasSpellState(SS_SMITEEVIL)) {
		fxqueue.RemoveAllEffects(fx_smite_evil_ref);
	}

	// check for concealment first (iwd2), both our enemies' and from our phasing problems
	int concealment = (GetStat(IE_ETHEREALNESS)>>8) + (target->GetStat(IE_ETHEREALNESS) & 0x64);
	if (concealment && LuckyRoll(1, 100, 0) < concealment) {
		// can we retry?
		if (!HasFeat(FEAT_BLIND_FIGHT) || LuckyRoll(1, 100, 0) < concealment) {
			// Missed <TARGETNAME> due to concealment.
			core->GetTokenDictionary()["TARGETNAME"] = target->GetDefaultName();
			if (core->HasFeedback(FT_COMBAT)) displaymsg->DisplayConstantStringName(HCStrings::ConcealedMiss, GUIColors::WHITE, this);
			buffer.append("[Concealment Miss]");
			Log(COMBAT, "Attack", "{}", buffer);
			ResetState();
			return;
		}
	}

	// iwd2 rerolls to check for criticals (cf. manual page 45) - the second roll just needs to hit; on miss, it degrades to a normal hit
	// CriticalBonus is negative, it is added to the minimum roll needed for a critical hit
	// IE_CRITICALHITBONUS is positive, it is subtracted
	int roll = LuckyRoll(1, attackRollDiceSides, 0, LR_CRITICAL);
	int criticalroll = roll + (int) GetStat(IE_CRITICALHITBONUS) - CriticalBonus;
	if (third) {
		int ThreatRangeMin = wi.critrange;
		ThreatRangeMin -= ((int) GetStat(IE_CRITICALHITBONUS) - CriticalBonus);
		criticalroll = LuckyRoll(1, attackRollDiceSides, 0, LR_CRITICAL);
		if (criticalroll < ThreatRangeMin || GetStat(IE_SPECFLAGS)&SPECF_CRITIMMUNITY) {
			// make it an ordinary hit
			criticalroll = 1;
		} else {
			// make sure it will be a critical hit
			criticalroll = attackRollDiceSides;
		}
	}

	//damage type is?
	//modify defense with damage type
	ieDword damagetype = hittingheader->DamageType;
	int damage = 0;

	if (hittingheader->DiceThrown<256) {
		// another bizarre 2E feature that's unused, but working
		if (!third && hittingheader->AltDiceSides && target->GetStat(IE_MC_FLAGS) & MC_LARGE_CREATURE) {
			// make sure not to discard other damage bonuses from above
			int dmgBon = DamageBonus - hittingheader->DamageBonus + hittingheader->AltDamageBonus;
			damage += LuckyRoll(hittingheader->AltDiceThrown, hittingheader->AltDiceSides, dmgBon, LR_DAMAGELUCK);
		} else {
			damage += LuckyRoll(hittingheader->DiceThrown, hittingheader->DiceSides, DamageBonus, LR_DAMAGELUCK);
		}
		if (damage <= 0) damage = 1; // bad luck, effects and/or profs on lowlevel chars
	}

	bool critical = criticalroll >= attackRollDiceSides;
	bool success = critical;
	int defense = target->GetDefense(damagetype, wi.wflags, this);
	int rollMod = ReverseToHit ? defense : tohit;
	if (!critical) {
		// autohit immobile enemies (true for atleast stun, sleep, timestop)
		if (target->Immobile() || (target->GetStat(IE_STATE_ID) & STATE_SLEEP)) {
			success = true;
		} else if (roll == 1) {
			success = false;
		} else {
			success = (roll + rollMod) > (ReverseToHit ? tohit : defense);
		}
	}

	if (target->GetStat(IE_EXTSTATE_ID) & EXTSTATE_EYE_SWORD) {
		target->fxqueue.RemoveAllEffects(fx_eye_sword_ref);
		target->spellbook.RemoveSpell(SevenEyes[EYE_SWORD]);
		target->SetBaseBit(IE_EXTSTATE_ID, EXTSTATE_EYE_SWORD, false);
		success = false;
		roll = 2; // avoid chance critical misses
	}

	const GameControl* gc = core->GetGameControl();
	if (core->HasFeedback(FT_TOHIT) && !gc->InDialog()) {
		// log the roll
		String leftRight;
		String hitMiss;
		if (usedLeftHand && DisplayMessage::HasStringReference(HCStrings::AttackRollLeft)) {
			leftRight = core->GetString(DisplayMessage::GetStringReference(HCStrings::AttackRollLeft));
		} else {
			leftRight = core->GetString(DisplayMessage::GetStringReference(HCStrings::AttackRoll));
		}
		if (success) {
			hitMiss = core->GetString(DisplayMessage::GetStringReference(HCStrings::Hit));
		} else {
			hitMiss = core->GetString(DisplayMessage::GetStringReference(HCStrings::Miss));
		}
		String rollLog = fmt::format(u"{} {} {} {} = {} : {}", leftRight, roll, (rollMod >= 0) ? u"+" : u"-", abs(rollMod), roll + rollMod, hitMiss);
		displaymsg->DisplayStringName(std::move(rollLog), GUIColors::WHITE, this);
	}

	int critMissThreshold = 1;
	static EffectRef fx_critical_miss_ref = { "CriticalMissModifier", -1 };
	const Effect* fx = fxqueue.HasEffect(fx_critical_miss_ref);
	if (fx && IsCriticalEffectEligible(wi, fx)) {
		critMissThreshold += fx->Parameter1;
	}
	if (roll <= critMissThreshold) {
		//critical failure
		buffer.append("[Critical Miss]");
		Log(COMBAT, "Attack", "{}", buffer);
		if (!gc->InDialog()) {
			displaymsg->DisplayMsgAtLocation(HCStrings::CriticalMiss, FT_COMBAT, this, this, GUIColors::WHITE);
			VerbalConstant(Verbal::CritMiss);
		}
		if (wi.wflags & WEAPON_RANGED) {//no need for this with melee weapon!
			UseItem(wi.slot, (ieDword) -2, target, UI_MISS|UI_NOAURA);
		} else if (core->HasFeature(GFFlags::BREAKABLE_WEAPONS) && InParty) {
			//break sword
			// a random roll on-hit (perhaps critical failure too)
			//  in 0,5% (1d20*1d10==1) cases
			if (wi.wflags & WEAPON_BREAKABLE && core->Roll(1, 10, 0) == 1) {
				inventory.BreakItemSlot(wi.slot);
				inventory.EquipBestWeapon(EQUIP_MELEE);
			}
		}
		ApplyCriticalEffect(this, target, wi, false);
		ResetState();
		return;
	}

	if (!success) {
		//hit failed
		if (wi.wflags&WEAPON_RANGED) {//Launch the projectile anyway
			UseItem(wi.slot, (ieDword)-2, target, UI_MISS|UI_NOAURA);
		}
		ResetState();
		buffer.append("[Missed]");
		Log(COMBAT, "Attack", "{}", buffer);
		return;
	}

	ModifyWeaponDamage(wi, target, damage, critical);

	if (third && target->GetStat(IE_MC_FLAGS) & MC_INVULNERABLE) {
		Log(DEBUG, "Actor", "Attacking invulnerable target, nulifying damage!");
		damage = 0;
	}

	if (critical) {
		//critical success
		buffer.append("[Critical Hit]");
		Log(COMBAT, "Attack", "{}", buffer);
		if (!gc->InDialog()) {
			displaymsg->DisplayMsgAtLocation(HCStrings::CriticalHit, FT_COMBAT, this, this, GUIColors::WHITE);
			VerbalConstant(Verbal::CritHit, gamedata->GetVBData("SPECIAL_COUNT"));
		}
		ApplyCriticalEffect(this, target, wi, true);
	} else {
		//normal success
		buffer.append("[Hit]");
		Log(COMBAT, "Attack", "{}", buffer);
	}
	UseItem(wi.slot, wi.wflags&WEAPON_RANGED?-2:-1, target, (critical?UI_CRITICAL:0)|UI_NOAURA, damage);
	ResetState();
}

unsigned int Actor::GetWeaponRange(bool leftOrRight) const
{
	return std::min(weaponInfo[leftOrRight].range, Modified[IE_VISUALRANGE]);
}

int Actor::WeaponDamageBonus(const WeaponInfo &wi) const
{
	if (wi.wflags&WEAPON_USESTRENGTH || wi.wflags&WEAPON_USESTRENGTH_DMG) {
		if (third) {
			int bonus = GetAbilityBonus(IE_STR);
			// 150% bonus for twohanders
			if (wi.itemflags&IE_INV_ITEM_TWOHANDED) bonus+=bonus/2;
			// only 50% for the offhand
			if (wi.wflags&WEAPON_LEFTHAND) bonus=bonus/2;
			return bonus;
		}
		return core->GetStrengthBonus(1, GetStat(IE_STR), GetStat(IE_STREXTRA) );
	}

	return 0;
}

// filter out any damage reduction that is cancelled by high weapon enchantment
// damage reduction (the effect and other uses) is implemented as normal resistance for physical damage, just with extra checks
int Actor::GetDamageReduction(int resist_stat, ieDword weaponEnchantment) const
{
	// this is the total, but some of it may have to be discarded
	int resisted = (signed)GetSafeStat(resist_stat);
	if (!resisted) {
		return 0;
	}
	int remaining = 0;
	int total = 0;
	if (resist_stat == IE_RESISTMISSILE) {
		remaining = fxqueue.SumDamageReduction(fx_missile_damage_reduction_ref, weaponEnchantment, total);
	} else {
		// the usual 3 physical types
		remaining = fxqueue.SumDamageReduction(fx_damage_reduction_ref, weaponEnchantment, total);
	}

	if (remaining == -1) {
		// no relevant effects were found, so the whole resistance value ignores enchantment checks
		return resisted;
	}
	if (remaining == resisted) {
		Log(COMBAT, "DamageReduction", "Damage resistance ({}) is completely from damage reduction.", resisted);
		return resisted;
	}
	if (remaining == total) {
		Log(COMBAT, "DamageReduction", "No weapon enchantment breach — full damage reduction and resistance used.");
		return resisted;
	} else {
		Log(COMBAT, "DamageReduction", "Ignoring {} of {} damage reduction due to weapon enchantment breach.", total-remaining, total);
		return resisted - (total-remaining);
	}
}

/*Always call this on the suffering actor */
void Actor::ModifyDamage(Scriptable *hitter, int &damage, int &resisted, int damagetype)
{
	Actor* attacker = Scriptable::As<Actor>(hitter);

	//guardian mantle for PST
	if (attacker && (Modified[IE_IMMUNITY]&IMM_GUARDIAN) ) {
		//if the hitter doesn't make the spell save, the mantle works and the damage is 0
		if (!attacker->GetSavingThrow(0, -4)) {
			damage = 0;
			return;
		}
	}

	// only check stone skins if damage type is physical
	// DAMAGE_CRUSHING is 0, so we can't AND with it to check for its presence
	if (!(damagetype & ~(DAMAGE_PIERCING | DAMAGE_SLASHING | DAMAGE_MISSILE))) {
		int stoneskins = Modified[IE_STONESKINS];
		if (stoneskins) {
			//pst style damage soaking from cloak of warding
			damage = fxqueue.DecreaseParam3OfEffect(fx_cloak_ref, damage, 0);
			if (!damage) {
				return;
			}

			fxqueue.DecreaseParam1OfEffect(fx_stoneskin_ref, 1);
			fxqueue.DecreaseParam1OfEffect(fx_aegis_ref, 1);

			Modified[IE_STONESKINS]--;
			damage = 0;
			return;
		}

		stoneskins = GetSafeStat(IE_STONESKINSGOLEM);
		if (stoneskins) {
			fxqueue.DecreaseParam1OfEffect(fx_stoneskin2_ref, 1);
			Modified[IE_STONESKINSGOLEM]--;
			damage = 0;
			return;
		}
	}

	if (damage>0) {
		// check damage type immunity / resistance / susceptibility
		std::multimap<ieDword, DamageInfoStruct>::iterator it;
		it = core->DamageInfoMap.find(damagetype);
		if (it == core->DamageInfoMap.end()) {
			Log(ERROR, "ModifyDamage", "Unhandled damagetype: {}", damagetype);
		} else if (it->second.resist_stat) {
			// check for bonuses for specific damage types
			if (core->HasFeature(GFFlags::SPECIFIC_DMG_BONUS) && attacker) {
				int bonus = attacker->fxqueue.BonusForParam2(fx_damage_bonus_modifier_ref, it->second.iwd_mod_type);
				if (bonus) {
					resisted -= int (damage * bonus / 100.0);
					Log(COMBAT, "ModifyDamage", "Bonus damage of {}({:+d}%), neto: {}", int(damage * bonus / 100.0), bonus, -resisted);
				}
			}
			// damage type with a resistance stat
			if (third) {
				// flat resistance, eg. 10/- or eg. 5/+2 for physical types
				// for actors we need special care for damage reduction - traps (...) don't have enchanted weapons
				if (attacker && it->second.reduction) {
					ieDword weaponEnchantment = attacker->weaponInfo[attacker->usedLeftHand].enchantment;
					// disregard other resistance boni when checking whether to skip reduction
					resisted = GetDamageReduction(it->second.resist_stat, weaponEnchantment);
				} else {
					resisted += (signed)GetSafeStat(it->second.resist_stat);
				}
				damage -= resisted;
			} else {
				int resistance = (signed)GetSafeStat(it->second.resist_stat);
				// avoid buggy data
				if ((unsigned)abs(resistance) > maximum_values[it->second.resist_stat]) {
					resistance = 0;
					Log(DEBUG, "ModifyDamage", "Ignoring bad damage resistance value ({}).", resistance);
				}
				resisted += (int) (damage * resistance/100.0);
				damage -= resisted;
			}
			Log(COMBAT, "ModifyDamage", "Resisted {} of {} at {}% resistance to {}", resisted, damage + resisted, GetSafeStat(it->second.resist_stat), damagetype);
			// PST and BG1 may actually heal on negative damage
			if (!core->HasFeature(GFFlags::HEAL_ON_100PLUS)) {
				if (damage <= 0) {
					resisted = DR_IMMUNE;
					damage = 0;
				}
			}
		}
	}

	// don't complain when sarevok is 100% resistant in the cutscene that grants you the slayer form
	if (damage <= 0 && !core->InCutSceneMode()) {
		if (attacker && attacker->InParty) {
			if (core->HasFeedback(FT_COMBAT)) {
				attacker->DisplayStringOrVerbalConstant(HCStrings::WeaponIneffective, Verbal::WeaponIneffective);
			}
			core->Autopause(AUTOPAUSE::UNUSABLE, this);
		}
	}
}

void Actor::UpdateActorState()
{
	if (InTrap) {
		area->ClearTrap(this, InTrap-1);
	}

	Game* game = core->GetGame();

	//make actor unselectable and unselected when it is not moving
	//dead, petrified, frozen, paralysed or unavailable to player
	// but skip paused actors
	if (!GetStat(IE_CASTERHOLD) && !ValidTarget(GA_SELECT | GA_NO_ENEMY | GA_NO_NEUTRAL)) {
		game->SelectActor(this, false, SELECT_NORMAL);
	}

	if (remainingTalkSoundTime > 0) {
		tick_t currentTick = GetMilliseconds();
		tick_t diffTime = currentTick - lastTalkTimeCheckAt;
		lastTalkTimeCheckAt = currentTick;

		if (diffTime >= remainingTalkSoundTime) {
			remainingTalkSoundTime = 0;
		} else {
			remainingTalkSoundTime -= diffTime;
		}
		SetCircleSize();
	}

	// display pc hitpoints if requested
	// limit the invocation count to save resources (the text is drawn repeatedly anyway)
	ieDword overheadHP = core->GetVariable("HP Over Head", 0);
	assert(game->GameTime);
	assert(core->Time.round_size);
	if (overheadHP && Persistent() && (game->GameTime % (core->Time.round_size / 2) == 0)) { // smaller delta to skip fading
		DisplayHeadHPRatio();
	}

	const auto& anim = currentStance.anim;
	if (attackProjectile) {
		// default so that the projectile fires if we dont have an animation for some reason
		unsigned int frameCount = anim.empty() ? 9 : anim[0].first->GetFrameCount();
		unsigned int currentFrame = anim.empty() ? 8 : anim[0].first->GetCurrentFrameIndex();

		//IN BG1 and BG2, this is at the ninth frame... (depends on the combat bitmap, which we don't handle yet)
		// however some critters don't have that long animations (eg. squirrel 0xC400)
		if ((frameCount > 8 && currentFrame == 8) || (frameCount <= 8 && currentFrame == frameCount/2)) {
			GetCurrentArea()->AddProjectile(attackProjectile, Pos, objects.LastTarget, false);
			attackProjectile = NULL;
		}
	}
	
	if (anim.empty()) {
		UpdateModalState(game->GameTime);
		return;
	}

	Animation* first = anim[0].first;
	
	if (first->endReached) {
		// possible stance change
		if (HandleActorStance()) {
			// restart animation for next time it is needed
			first->endReached = false;
			first->SetFrame(0);

			Animation* firstShadow = currentStance.shadow.empty() ? nullptr : currentStance.shadow[0].first;
			if (firstShadow) {
				firstShadow->endReached = false;
				firstShadow->SetFrame(0);
			}
		}
	} else {
		// check if walk sounds need to be played
		// dialog, pause game
		if (!(core->GetGameControl()->GetDialogueFlags() & (DF_IN_DIALOG | DF_FREEZE_SCRIPTS))) {
			// footsteps option set, stance
			if (footsteps && GetStance() == IE_ANI_WALK) {
				PlayWalkSound();
			}
		}
	}

	UpdateModalState(game->GameTime);
}

void Actor::UpdateModalState(ieDword gameTime)
{
	if (Modal.LastApplyTime == gameTime) {
		return;
	}

	// use the combat round size as the original;  also skald song duration matches it
	int roundFraction = (gameTime - roundTime) % GetAdjustedTime(core->Time.attack_round_size);

	//actually, iwd2 has autosearch, also, this is useful for dayblindness
	//apply the modal effect about every second (pst and iwds have round sizes that are not multiples of 15)
	// FIXME: split dayblindness out of detect.spl and only run that each tick + simplify this check
	if (InParty && core->HasFeature(GFFlags::AUTOSEARCH_HIDDEN) && (third || (roundFraction % core->Time.defaultTicksPerSec == 0))) {
		core->ApplySpell(ResRef("detect"), this, this, 0);
	}

	// this is a HACK, fuzzie can't work out where else to do this for now
	// but we shouldn't be resetting rounds/attacks just because the actor
	// wandered away, the action code should probably be responsible somehow
	// see also line above (search for comment containing UpdateActorState)!
	if (objects.LastTarget && lastattack && lastattack < (gameTime - 1)) {
		const Actor* target = area->GetActorByGlobalID(objects.LastTarget);
		if (!target || target->GetStat(IE_STATE_ID) & STATE_DEAD ||
			(target->GetStance() == IE_ANI_WALK && target->GetAnims()->GetAnimType() == IE_ANI_TWO_PIECE)) {
			StopAttack();
		} else {
			Log(COMBAT, "Attack", "(Leaving attack)");
		}

		lastattack = 0;
	}

	if (Modal.State == Modal::None && !Modal.LingeringCount) {
		return;
	}

	ieDword state = Modified[IE_STATE_ID];
	const Game* game = core->GetGame();

	//apply the modal effect on the beginning of each round
	if (roundFraction == 0) {
		// handle lingering modal spells like bardsong in iwd2
		if (Modal.LingeringCount && !Modal.LingeringSpell.IsEmpty()) {
			Modal.LingeringCount--;
			ApplyModal(Modal.LingeringSpell);
		}
		if (Modal.State == Modal::None) {
			return;
		}

		// some states and timestop disable modal actions
		// interestingly the original doesn't include STATE_DISABLED, STATE_FROZEN/STATE_PETRIFIED
		if (Immobile() || (state & (STATE_CONFUSED | STATE_DEAD | STATE_HELPLESS | STATE_PANIC | STATE_BERSERK | STATE_SLEEP))) {
			return;
		}

		//we can set this to 0
		Modal.LastApplyTime = gameTime;

		if (Modal.Spell.IsEmpty()) {
			Log(WARNING, "Actor", "Modal Spell Effect was not set!");
			Modal.Spell = "*";
		} else if (!IsStar(Modal.Spell)) {
			if (ModalSpellSkillCheck()) {
				ApplyModal(Modal.Spell);

				// some modals notify each round, some only initially
				bool feedback = ModalStates[Modal.State].repeat_msg || Modal.FirstApply;
				Modal.FirstApply = false;
				if (InParty && feedback && core->HasFeedback(FT_MISC)) {
					ieStrRef entering = PersonalizePSTString(ModalStates[Modal.State].entering_str, this);
					displaymsg->DisplayStringName(entering, GUIColors::WHITE, this, STRING_FLAGS::SOUND | STRING_FLAGS::SPEECH);
				}
			} else {
				if (InParty && core->HasFeedback(FT_MISC)) {
					ieStrRef failed = PersonalizePSTString(ModalStates[Modal.State].failed_str, this);
					displaymsg->DisplayStringName(failed, GUIColors::WHITE, this, STRING_FLAGS::SOUND | STRING_FLAGS::SPEECH);
				}
				Modal.State = Modal::None;
			}
		}

		// shut everyone up, so they don't whine if the actor is on a long hiding-in-shadows recon mission
		game->ResetPartyCommentTimes();
	}
}

void Actor::ApplyModal(const ResRef& modalSpell)
{
	unsigned int aoe = ModalStates[Modal.State].aoe_spell;
	if (aoe == 1) {
		core->ApplySpellPoint(modalSpell, GetCurrentArea(), Pos, this, 0);
	} else if (aoe == 2) {
		// target actors around us manually
		// used for iwd2 songs, as the spells don't use an aoe projectile
		if (!area) return;
		std::vector<Actor *> neighbours = area->GetAllActorsInRadius(Pos, GA_NO_LOS|GA_NO_DEAD|GA_NO_UNSCHEDULED, GetSafeStat(IE_VISUALRANGE)/2);
		for (const auto& neighbour : neighbours) {
			core->ApplySpell(modalSpell, neighbour, this, 0);
		}
	} else {
		core->ApplySpell(modalSpell, this, this, 0);
	}
}

//idx could be: 0-6, 16-22, 32-38, 48-54
//the colors are stored in 7 dwords
//maybe it would be simpler to store them in 28 bytes (without using stats?)
void Actor::SetColor( ieDword idx, ieDword grd)
{
	ieByte gradient = (ieByte) (grd&255);
	ieByte index = (ieByte) (idx&15);
	ieByte shift = (ieByte) (idx/16);
	ieDword value;

	//invalid value, would crash original IE
	if (index>6) {
		return;
	}

	//Don't modify the modified stats if the colors were locked (for this ai cycle)
	if (anims && anims->lockPalette) {
		return;
	}

	if (shift == 15) {
		// put gradient in all four bytes of value
		value = gradient;
		value |= (value << 8);
		value |= (value << 16);
		for (index=0;index<7;index++) {
			Modified[IE_COLORS+index] = value;
		}
	} else {
		//invalid value, would crash original IE
		if (shift>3) {
			return;
		}
		shift *= 8;
		value = gradient << shift;
		value |= Modified[IE_COLORS+index] & ~(255<<shift);
		Modified[IE_COLORS+index] = value;
	}
}

void Actor::SetColorMod(ieDword location, RGBModifier::Type type, int speed,
						const Color &color, int phase) const
{
	CharAnimations* ca = GetAnims();
	if (!ca) return;

	if (location == 0xff) {
		if (phase && ca->GlobalColorMod.locked) return;
		ca->GlobalColorMod.locked = !phase;
		ca->GlobalColorMod.type = type;
		ca->GlobalColorMod.speed = speed;
		ca->GlobalColorMod.rgb = color;
		if (phase >= 0)
			ca->GlobalColorMod.phase = phase;
		else {
			if (ca->GlobalColorMod.phase > 2*speed)
				ca->GlobalColorMod.phase=0;
		}
		return;
	}
	//00xx0yyy-->000xxyyy
	if (location&0xffffffc8) return; //invalid location
	location = (location &7) | ((location>>1)&0x18);
	if (phase && ca->ColorMods[location].locked) return;
	ca->ColorMods[location].type = type;
	ca->ColorMods[location].speed = speed;
	ca->ColorMods[location].rgb = color;
	if (phase >= 0)
		ca->ColorMods[location].phase = phase;
	else {
		if (ca->ColorMods[location].phase > 2*speed)
			ca->ColorMods[location].phase = 0;
	}
}

void Actor::SetLeader(const Actor* actor, int offset)
{
	objects.LastFollowed = actor->GetGlobalID();
	FollowOffset.x = offset;
	FollowOffset.y = offset;
}

//if hp <= 0, it means full healing
void Actor::Heal(int hp)
{
	if (hp > 0) {
		stat_t newHp = BaseStats[IE_HITPOINTS] + hp;
		SetBase(IE_HITPOINTS, std::min(newHp, Modified[IE_MAXHITPOINTS]));
	} else {
		SetBase(IE_HITPOINTS, Modified[IE_MAXHITPOINTS]);
	}
}

void Actor::AddExperience(int exp, int combat)
{
	int bonus = core->GetWisdomBonus(0, Modified[IE_WIS]);
	int adjustmentPercent = gamedata->GetDifficultyMod(1, GameDifficulty);
	// the "Suppress Extra Difficulty Damage" also switches off the XP bonus
	if (combat && (!NoExtraDifficultyDmg || adjustmentPercent < 0)) {
		bonus += adjustmentPercent;
	}
	bonus += GetFavoredPenalties();

	int xpStat = IE_XP;

	// decide which particular XP stat to add to (only for TNO's switchable classes)
	const Game* game = core->GetGame();
	if (pstflags && this == game->GetPC(0, false)) { // rule only applies to the protagonist
		switch (BaseStats[IE_CLASS]) {
			case 4:
				xpStat = IE_XP_THIEF;
				break;
			case 1:
				xpStat = IE_XP_MAGE;
				break;
			case 2:
			default: //just in case the character was modified
				break;
		}
	}

	exp = ((exp * (100 + bonus)) / 100) + BaseStats[xpStat];
	int classID = GetActiveClass() - 1;
	if (classID < classcount) {
		if (xpCap[classID] > 0 && exp > xpCap[classID]) {
			exp = xpCap[classID];
		}
	}
	SetBase(xpStat, exp);
}

static bool is_zero(const int& value) {
	return value == 0;
}

// for each class pair that is out of level sync for more than 1 level and
// one of them isn't a favored class, incur a 20% xp penalty (cumulative)
int Actor::GetFavoredPenalties() const
{
	if (!third) return 0;
	if (!PCStats) return 0;

	std::list<int> classLevels(PCStats->ClassLevels);
	classLevels.remove_if(is_zero);
	size_t classCount = classLevels.size();
	if (classCount == 1) return 0;

	unsigned int race = GetSubRace();
	int favored = favoredMap[race];
	// shortcuts for special case - "any" favored class
	if (favored == -1 && classCount == 2) return 0;

	int flevel = -1;
	if (favored != -1) {
		// get the favored class index from ID
		// different for (fe)males for some races, but stored in one value
		if (GetStat(IE_SEX) == 1) {
			favored = favored & 15;
		} else {
			favored = (favored>>8) & 15;
		}
		flevel = GetLevelInClass(favored);
	}

	classLevels.sort(); // ascending
	if (flevel == -1) {
		// any class - just remove the highest level
		classLevels.erase(--classLevels.end());
		classCount--;
	} else {
		// remove() kills all elements with the same value, so we have to jump through hoops
		classLevels.remove(flevel);
		size_t diff = classCount - classLevels.size();
		if (diff == classCount) return 0; // all class were at the same level
		for (size_t i = 1; i < diff; i++) {
			// re-add missing levels (all but one)
			classLevels.push_back(flevel);
		}
		classCount = classLevels.size();
		if (classCount == 1) return 0; // only one class besides favored
	}

	// finally compare adjacent levels - if they're more than 1 apart
	int penalty = 0;
	for (auto it = std::next(classLevels.begin()); it != classLevels.end(); ++it) {
		int level1 = *(--it);
		int level2 = *(++it);
		if (level2 - level1 > 1) penalty++;
	}

	return -20*penalty;
}

bool Actor::Schedule(ieDword gametime, bool checkhide) const
{
	if (checkhide) {
		if (!(InternalFlags&IF_VISIBLE) ) {
			return false;
		}
	}

	//check for schedule
	return GemRB::Schedule(appearance, gametime);
}


void Actor::SetInTrap(ieDword setreset)
{
	InTrap = setreset;
	if (setreset) {
		InternalFlags |= IF_INTRAP;
	} else {
		InternalFlags &= ~IF_INTRAP;
	}
}

void Actor::SetRunFlags(ieDword flags)
{
	InternalFlags &= ~IF_RUNFLAGS;
	InternalFlags |= (flags & IF_RUNFLAGS);
}

void Actor::NewPath()
{
	if (Destination == Pos) return;
	// WalkTo's and FindPath's first argument is passed by reference
	// And we don't want to modify Destination so we use a temporary
	Point savedDest = Destination;
	if (GetPathTries() > MAX_PATH_TRIES) {
		ClearPath(true);
		ResetPathTries();
		return;
	}
	WalkTo(savedDest, InternalFlags, pathfindingDistance);
	if (!GetPath()) {
		IncrementPathTries();
	}
}


void Actor::WalkTo(const Point &Des, ieDword flags, int MinDistance)
{
	ResetPathTries();
	if (InternalFlags & IF_REALLYDIED || walkScale == 0) {
		return;
	}
	SetRunFlags(flags);
	ResetCommentTime();
	Movable::WalkTo(Des, MinDistance);
}

void Actor::DrawActorSprite(const Point& p, BlitFlags flags,
							const std::vector<AnimationPart>& animParts, const Color& tint) const
{
	if (tint.a == 0) return;
	
	if (!anims->lockPalette) {
		flags |= BlitFlags::COLOR_MOD;
	}
	flags |= BlitFlags::ALPHA_MOD;

	for (const auto& part : animParts) {
		const Animation* anim = part.first;
		Holder<Palette> palette = part.second;

		Holder<Sprite2D> currentFrame = anim->CurrentFrame();
		if (currentFrame) {
			if (TranslucentShadows && palette) {
				ieByte tmpa = palette->col[1].a;
				palette->col[1].a /= 2;
				VideoDriver->BlitGameSpriteWithPalette(currentFrame, palette, p, flags, tint);
				palette->col[1].a = tmpa;
			} else {
				VideoDriver->BlitGameSpriteWithPalette(currentFrame, palette, p, flags, tint);
			}
		}
	}
}


static const int OrientdX[16] = { 0, -4, -7, -9, -10, -9, -7, -4, 0, 4, 7, 9, 10, 9, 7, 4 };
static const int OrientdY[16] = { 10, 9, 7, 4, 0, -4, -7, -9, -10, -9, -7, -4, 0, 4, 7, 9 };
static const unsigned int MirrorImageLocation[8] = { 4, 12, 8, 0, 6, 14, 10, 2 };
static const unsigned int MirrorImageZOrder[8] = { 2, 4, 6, 0, 1, 7, 5, 3 };

bool Actor::HibernateIfAble()
{
	//finding an excuse why we don't hybernate the actor
	if (Modified[IE_ENABLEOFFSCREENAI])
		return false;
	if (objects.LastTarget) // currently attacking someone
		return false;
	if (!objects.LastTargetPos.IsInvalid()) // currently casting at the ground
		return false;
	if (objects.LastSpellTarget) // currently casting at someone
		return false;
	if (InternalFlags&IF_JUSTDIED) // didn't have a chance to run a script
		return false;
	if (CurrentAction)
		return false;
	if (third && Modified[IE_MC_FLAGS]&MC_IGNORE_INHIBIT_AI)
		return false;
	if (InMove())
		return false;
	if (GetNextAction())
		return false;
	if (GetWait()) //would never stop waiting
		return false;
	// the EEs also have the condition of (EA < EA_EVILCUTOFF && EA_CONTROLLABLE < EA), practically only allowing neutrals
	
	InternalFlags |= IF_IDLE;
	return true;
}

// even if a creature is offscreen, they should still get an AI update every 3 ticks
bool Actor::ForceScriptCheck()
{
	if (!lastScriptCheck) lastScriptCheck = Ticks;

	lastScriptCheck++;
	if (lastScriptCheck - Ticks >= 3) {
		lastScriptCheck = Ticks;
		return true;
	}
	return false;
}

bool Actor::AdvanceAnimations()
{
	if (!anims) {
		return false;
	}
	
	anims->PulseRGBModifiers();
	
	ClearCurrentStanceAnims();

	unsigned char stanceID = GetStance();
	orient_t face = GetNextFace();
	const auto* stanceAnim = anims->GetAnimation(stanceID, face);
	
	if (stanceAnim == nullptr) {
		return false;
	}
	
	const auto* shadows = anims->GetShadowAnimation(stanceID, face);
	
	const auto count = anims->GetTotalPartCount();
	const auto zOrder = anims->GetZOrder(face);
	
	// display current frames in the right order
	for (int part = 0; part < count; ++part) {
		int partnum = part;
		if (zOrder) partnum = zOrder[part];
		Animation* anim = stanceAnim->at(partnum).get();
		if (anim) {
			currentStance.anim.emplace_back(anim, anims->GetPartPalette(partnum));
		}
		
		if (shadows) {
			Animation* shadowAnim = shadows->at(partnum).get();
			if (shadowAnim) {
				currentStance.shadow.emplace_back(shadowAnim, anims->GetShadowPalette());
			}
		}
	}
	
	Animation* first = currentStance.anim[0].first;
	Animation* firstShadow = currentStance.shadow.empty() ? nullptr : currentStance.shadow[0].first;
	
	// advance first (main) animation by one frame (in sync)
	if (Immobile()) {
		// update animation, continue last-displayed frame
		first->LastFrame();
		if (firstShadow) {
			firstShadow->LastFrame();
		}
	} else {
		// update animation, maybe advance a frame (if enough time has passed)
		first->NextFrame();
		if (firstShadow) {
			firstShadow->NextFrame();
		}
	}

	// update all other animation parts, in sync with the first part
	auto it = currentStance.anim.begin() + 1;
	for (; it != currentStance.anim.end(); ++it) {
		it->first->GetSyncedNextFrame(first);
	}
	
	it = currentStance.shadow.begin();
	if (it != currentStance.shadow.end()) {
		for (++it; it != currentStance.shadow.end(); ++it) {
			it->first->GetSyncedNextFrame(firstShadow);
		}
	}

	return true;
}

bool Actor::IsDead() const
{
	return InternalFlags & IF_STOPATTACK;
}

bool Actor::ShouldDrawCircle() const
{
	if (Modified[IE_NOCIRCLE]) {
		return false;
	}

	int State = Modified[IE_STATE_ID];

	if ((State&STATE_DEAD) || (InternalFlags&IF_REALLYDIED)) {
		return false;
	}

	//adjust invisibility for enemies
	if (Modified[IE_EA]>EA_GOODCUTOFF) {
		if (State&state_invisible) {
			return false;
		}
	}
	
	const GameControl* gc = core->GetGameControl();
	if (gc->GetScreenFlags()&SF_CUTSCENE) {
		// ground circles are not drawn in cutscenes
		// except for the speaker
		if (gc->dialoghandler->IsTarget(this) == false) {
			return false;
		}
	}

	// underground ankhegs
	if (GetStance() == IE_ANI_WALK && GetAnims()->GetAnimType() == IE_ANI_TWO_PIECE) {
		return false;
	}

	bool drawcircle = true; // we always show circle/target on pause
	if (!(gc->GetDialogueFlags() & DF_FREEZE_SCRIPTS)) {
		// check marker feedback level
		ieDword markerfeedback = core->GetVariable("GUI Feedback Level", 4);
		if (Selected) {
			// selected creature
			drawcircle = markerfeedback >= 2;
		} else if (IsPC()) {
			// selectable
			drawcircle = markerfeedback >= 3;
		} else if (Modified[IE_EA] >= EA_EVILCUTOFF) {
			// hostile
			drawcircle = markerfeedback >= 4;
		} else {
			// all
			drawcircle = markerfeedback >= 5;
		}
	}
	
	return drawcircle;
}

bool Actor::ShouldDrawReticle() const
{
	if (ShouldDrawCircle()){
		return (!(InternalFlags&IF_NORETICLE) && Modified[IE_EA] <= EA_CONTROLLABLE && Destination != Pos);
	}
	return false;
}

bool Actor::HasBodyHeat() const
{
	const Effect* fx = fxqueue.HasEffectWithParam(fx_animation_override_data_ref, 1);
	if (fx) return bool(fx->Parameter1);
	if (Modified[IE_STATE_ID]&(STATE_DEAD|STATE_FROZEN|STATE_PETRIFIED) ) return false;
	if (GetAnims()->GetFlags()&AV_NO_BODY_HEAT) return false;
	return true;
}

int Actor::GetElevation() const
{
	return area ? area->GetHeight(Pos) : 0;
}

bool Actor::UpdateDrawingState()
{
	for (auto it = vfxQueue.cbegin(); it != vfxQueue.cend();) {
		ScriptedAnimation* vvc = *it;

		// skip two overlays if fx_disable_overlay_modifier is in effect
		// add a flags field if this ever starts being used heavily (currently only bg2 demi-liches)
		if (Modified[IE_DISABLEOVERLAY] && (vvc->ResName == hc_overlays[OV_BOUNCE] || vvc->ResName == hc_overlays[OV_SPELLTRAP])) {
			++it;
			continue;
		}

		if ((vvc->SequenceFlags & IE_VVC_STATIC) == 0) {
			vvc->Pos = Pos;
		}
		
		bool endReached = vvc->UpdateDrawingState(GetOrientation());
		if (endReached) {
			vfxDict.erase(vfxDict.find(vvc->ResName)); // make sure to delete only one element
			it = vfxQueue.erase(it);
			delete vvc;
			continue;
		}

		if (!vvc->active) {
			vvc->SetPhase(P_RELEASE);
		}
		
		++it;
	}
	
	if (!AdvanceAnimations()) {
		return false;
	}
	
	UpdateDrawingRegion();
	return true;
}

void Actor::UpdateDrawingRegion()
{
	Region box(Pos, Size());
	
	auto ExpandBoxForAnimationParts = [&box, this](const std::vector<AnimationPart>& parts) {
		for (const auto& part : parts) {
			const Animation* anim = part.first;
			Holder<Sprite2D> animframe = anim->CurrentFrame();
			if (!animframe) continue;
			Region partBBox = animframe->Frame;
			partBBox.x = Pos.x - partBBox.x;
			partBBox.y = Pos.y - partBBox.y;
			box.ExpandToRegion(partBBox);
			assert(box.RectInside(partBBox));
		}
	};
	
	ExpandBoxForAnimationParts(currentStance.anim);
	ExpandBoxForAnimationParts(currentStance.shadow);
			
	box.y -= GetElevation();
	
	// BBox is the the box containing the actor and all its equipment, but nothing else
	SetBBox(box);
	
	int mirrorimages = Modified[IE_MIRRORIMAGES];
	for (int i = 0; i < mirrorimages; ++i) {
		int dir = MirrorImageLocation[i];
		
		Region mirrorBox = BBox;
		mirrorBox.x += 3 * OrientdX[dir];
		mirrorBox.y += 3 * OrientdY[dir];
		
		box.ExpandToRegion(mirrorBox);
	}
	
	if (Modified[IE_STATE_ID] & STATE_BLUR) {
		orient_t face = GetOrientation();
		int blurx = (OrientdX[face] * (int)Modified[IE_MOVEMENTRATE])/20;
		int blury = (OrientdY[face] * (int)Modified[IE_MOVEMENTRATE])/20;
		
		Region blurBox = BBox;
		blurBox.x -= blurx * 3;
		blurBox.y -= blury * 3;
		
		box.ExpandToRegion(blurBox);
	}
	
	for (const auto& vvc : vfxQueue) {
		Region r = vvc->DrawingRegion();
		if (vvc->SequenceFlags & IE_VVC_HEIGHT) r.y -= BBox.h;
		box.ExpandToRegion(r);
		assert(r.w <= box.w && r.h <= box.h);
	}

	// drawingRegion is the the box containing all gfx attached to the actor
	drawingRegion = box;
}

Region Actor::DrawingRegion() const
{
	return drawingRegion;
}

void Actor::Draw(const Region& vp, Color baseTint, Color tint, BlitFlags flags) const
{
	// if an actor isn't visible, should we still draw video cells?
	// let us assume not, for now..
	if (!(InternalFlags & IF_VISIBLE)) {
		return;
	}

	//iwd has this flag saved in the creature
	if (Modified[IE_AVATARREMOVAL]) {
		return;
	}

	if (!DrawingRegion().IntersectsRegion(vp)) {
		return;
	}

	//explored or visibilitymap (bird animations are visible in fog)
	//0 means opaque
	uint8_t trans = std::min<uint8_t>(Modified[IE_TRANSLUCENT], 255);

	int State = Modified[IE_STATE_ID];

	//adjust invisibility for enemies
	if (Modified[IE_EA] > EA_GOODCUTOFF && (State & state_invisible)) {
		trans = 255;
	} else if (State & STATE_BLUR) {
		//can't move this, because there is permanent blur state where
		//there is no effect (just state bit)
		trans = 128;
	}

	tint.a = 255 - trans;

	//draw videocells under the actor
	auto it = vfxQueue.cbegin();
	for (; it != vfxQueue.cend(); ++it) {
		const ScriptedAnimation* vvc = *it;
		if (vvc->YOffset >= 0) {
			break;
		}
		vvc->Draw(vp, baseTint, BBox.h, flags & (BlitFlags::STENCIL_MASK | BlitFlags::ALPHA_MOD));
	}

	if (ShouldDrawCircle()) {
		DrawCircle(vp.origin);
	}

	if (!currentStance.anim.empty()) {
		orient_t face = GetOrientation();
		// Drawing the actor:
		// * mirror images:
		//     Drawn without transparency, unless fully invisible.
		//     Order: W, E, N, S, NW, SE, NE, SW
		// * blurred copies (3 of them)
		//     Drawn with transparency.
		//     distance between copies depends on IE_MOVEMENTRATE
		//     TODO: actually, the direction is the real movement direction,
		//	not the (rounded) direction given Face
		// * actor itself
		//
		//comments by Avenger:
		// currently we don't have a real direction, but the orientation field
		// could be used with higher granularity. When we need the face value
		// it could be divided so it will become a 0-15 number.
		//
		
		if (AppearanceFlags & APP_HALFTRANS) flags |= BlitFlags::HALFTRANS;

		Point drawPos = Pos - vp.origin;
		drawPos.y -= GetElevation();

		// mirror images behind the actor
		for (int i = 0; i < 4; ++i) {
			unsigned int m = MirrorImageZOrder[i];
			if (m < Modified[IE_MIRRORIMAGES]) {
				int dir = MirrorImageLocation[m];
				int icx = drawPos.x + 3 * OrientdX[dir];
				int icy = drawPos.y + 3 * OrientdY[dir];
				Point iPos(icx, icy);
				// FIXME: I don't know if GetBlocked() is good enough
				// consider the possibility the mirror image is behind a wall (walls.second)
				// GetBlocked might be false, but we still should not draw the image
				// maybe the mirror image coordinates can never be beyond the width of a wall?
				if ((area->GetBlocked(iPos + vp.origin) & (PathMapFlags::PASSABLE | PathMapFlags::ACTOR)) != PathMapFlags::IMPASSABLE) {
					DrawActorSprite(iPos, flags, currentStance.anim, tint);
				}
			}
		}

		// blur sprites behind the actor
		int blurdx = (OrientdX[face]*(int)Modified[IE_MOVEMENTRATE])/20;
		int blurdy = (OrientdY[face]*(int)Modified[IE_MOVEMENTRATE])/20;
		Point blurPos = drawPos;
		if (State & STATE_BLUR) {
			if (face < 4 || face >= 12) {
				blurPos -= Point(4 * blurdx, 4 * blurdy);
				for (int i = 0; i < 3; ++i) {
					blurPos += Point(blurdx, blurdy);
					// FIXME: I don't think we ought to draw blurs that are behind a wall that the actor is in front of
					DrawActorSprite(blurPos, flags, currentStance.anim, tint);
				}
			}
		}

		if (!currentStance.shadow.empty()) {
			const Game* game = core->GetGame();
			// infravision, independent of light map and global light
			if (HasBodyHeat() &&
				game->PartyHasInfravision() &&
				!game->IsDay() &&
				(area->AreaType & AT_OUTDOOR) && !(area->AreaFlags & AF_DREAM)) {
				Color irTint = Color(255, 120, 120, tint.a);

				/* IWD2: infravision is white, not red. */
				if(core->HasFeature(GFFlags::RULES_3ED)) {
					irTint = Color(255, 255, 255, tint.a);
				}

				DrawActorSprite(drawPos, flags, currentStance.shadow, irTint);
			} else {
				DrawActorSprite(drawPos, flags, currentStance.shadow, tint);
			}
		}

		// actor itself
		DrawActorSprite(drawPos, flags, currentStance.anim, tint);

		// blur sprites in front of the actor
		if (State & STATE_BLUR) {
			if (face >= 4 && face < 12) {
				for (int i = 0; i < 3; ++i) {
					blurPos -= Point(blurdx, blurdy);
					DrawActorSprite(blurPos, flags, currentStance.anim, tint);
				}
			}
		}

		// mirror images in front of the actor
		for (int i = 4; i < 8; ++i) {
			unsigned int m = MirrorImageZOrder[i];
			if (m < Modified[IE_MIRRORIMAGES]) {
				int dir = MirrorImageLocation[m];
				int icx = drawPos.x + 3 * OrientdX[dir];
				int icy = drawPos.y + 3 * OrientdY[dir];
				Point iPos(icx, icy);
				// FIXME: I don't know if GetBlocked() is good enough
				// consider the possibility the mirror image is in front of a wall (walls.first)
				// GetBlocked might be false, but we still should not draw the image
				// maybe the mirror image coordinates can never be beyond the width of a wall?
				if ((area->GetBlocked(iPos + vp.origin) & (PathMapFlags::PASSABLE | PathMapFlags::ACTOR)) != PathMapFlags::IMPASSABLE) {
					DrawActorSprite(iPos, flags, currentStance.anim, tint);
				}
			}
		}
	}

	//draw videocells over the actor
	for (; it != vfxQueue.cend(); ++it) {
		const ScriptedAnimation* vvc = *it;
		vvc->Draw(vp, baseTint, BBox.h, flags & (BlitFlags::STENCIL_MASK | BlitFlags::ALPHA_MOD));
	}
}

/* Handling automatic stance changes */
bool Actor::HandleActorStance()
{
	CharAnimations* ca = GetAnims();
	int StanceID = GetStance();

	if (ca->autoSwitchOnEnd) {
		SetStance(ca->nextStanceID);
		ca->autoSwitchOnEnd = false;
		return true;
	}
	int x = RAND(0, 24);
	if ((StanceID==IE_ANI_AWAKE) && !x ) {
		SetStance( IE_ANI_HEAD_TURN );
		return true;
	}
	// added CurrentAction as part of blocking action fixes
	if ((StanceID==IE_ANI_READY) && !CurrentAction && !GetNextAction()) {
		SetStance( IE_ANI_AWAKE );
		return true;
	}
	if (StanceID == IE_ANI_ATTACK || StanceID == IE_ANI_ATTACK_JAB ||
		StanceID == IE_ANI_ATTACK_SLASH || StanceID == IE_ANI_ATTACK_BACKSLASH ||
		StanceID == IE_ANI_SHOOT)
	{
		SetStance( AttackStance );
		return true;
	}

	return false;
}

bool Actor::GetSoundFromFile(ResRef& sound, Verbal index) const
{
	// only dying ignores the incapacity to vocalize
	if (index != Verbal::Die) {
		if (Modified[IE_STATE_ID] & STATE_CANTLISTEN) return false;
	}

	if (core->HasFeature(GFFlags::RESDATA_INI)) {
		return GetSoundFromINI(sound, index);
	} else {
		return GetSoundFrom2DA(sound, index);
	}
}

// NOTE: picks a sound at random when the row has several
bool Actor::GetSoundFrom2DA(ResRef& sound, Verbal index) const
{
	if (!anims) return false;

	// check if there is an override (ToBExAL),
	// otherwise use the base animation prefix
	ResRef prefix = anims->ResRefBase;
	static AutoTable aniSndOverride = gamedata->LoadTable("anisndex", true);
	const std::string& row = fmt::format("0x{:4X}", Modified[IE_ANIMATION_ID]);
	ResRef file = aniSndOverride->QueryField(row, "File");
	if (!IsStar(file)) {
		prefix = file;
	}
	AutoTable tab = gamedata->LoadTable(prefix);
	if (!tab) return false;

	TableMgr::index_t idx = 0;
	switch (index) {
		case Verbal::Attack0:
			idx = 0;
			break;
		case Verbal::Damage:
			idx = 8;
			break;
		case Verbal::Die:
			idx = 10;
			break;
		case Verbal::BattleCry:
			idx = 34; // Battle_Cry
			break;
		case Verbal::Dialog:
		case Verbal::Select:
		case Verbal::Select2:
		case Verbal::Select3:
		case Verbal::Select4:
		case Verbal::Select5:
		case Verbal::Select6:
		case Verbal::Select7:
			idx = 36; // Selection (yes, the row names are inconsistently capitalized)
			break;
		// entries without VB equivalents
		case Verbal::Attack4:
			idx = 16; // SHOOT
			break;
		// these three supposedly never worked, at least not in bg2 (https://www.gibberlings3.net/forums/topic/19034-animation-2da-files)
		case Verbal::Attack1:
			idx = 22; // ATTACK_SLASH
			break;
		case Verbal::Attack2:
			idx = 24; // ATTACK_BACKSLASH
			break;
		case Verbal::Attack3:
			idx = 26; // ATTACK_JAB
			break;
		default:
			Log(WARNING, "Actor", "Cannot determine 2DA rowcount for index {} for {}, let us know!", idx, fmt::WideToChar { LongName });
			return false;
	}
	Log(MESSAGE, "Actor", "Getting sound 2da {} entry: {}", prefix, tab->GetRowName(idx));
	TableMgr::index_t col = RAND<TableMgr::index_t>(0, tab->GetColumnCount(idx) - 1);
	sound = tab->QueryField(idx, col);
	return true;
}

//Get the monster sound from a global .ini file.
//It is ResData.ini in PST and Sounds.ini in IWD/HoW
bool Actor::GetSoundFromINI(ResRef& sound, Verbal index) const
{
	unsigned int animid=BaseStats[IE_ANIMATION_ID];
	if(core->HasFeature(GFFlags::ONE_BYTE_ANIMID)) {
		animid&=0xff;
	}

	std::string section = fmt::to_string(animid);
	/* TODO: pst also has these, but we currently ignore them:
	 * Another form of randomization for attack animations (random pick of Attack1-3 [if defined] and this is its sound)
	 *    1x at2sound for each at1sound
	 *    2x at3sound
	 * Others:
	 *   33x cf1sound (stance (combat) fidget)
	 *    2x ms1sound (misc; both hammers hitting metal; ambient sounds for idle animations? Likely doesn't fit here)
	 *   19x sf1sound (stand (normal) fidget)
	 * 
	 * TODO: iwd:
	 *   att2-att4 used for 2nd-4th attack in the round?
	 *   fall mentioned as optional "falling down" in sndlist.txt
	 *   fidget (on IE_ANI_HEAD_TURN?)
	 */
	StringView resource;
	switch(index) {
		case Verbal::Attack0:
			// disabled by design in ees
			if (!core->HasFeature(GFFlags::HAS_EE_EFFECTS)) {
				resource = core->GetResDataINI()->GetKeyAsString(section, StringView(IWDSound ? "att1" : "at1sound"));
			}
			break;
		case Verbal::Damage:
			resource = core->GetResDataINI()->GetKeyAsString(section, StringView(IWDSound ? "damage" : "hitsound"));
			break;
		case Verbal::Die:
			resource = core->GetResDataINI()->GetKeyAsString(section, StringView(IWDSound ? "death" : "dfbsound"));
			break;
		case Verbal::Select:
			//this isn't in PST, apparently
			if (IWDSound) {
				resource = core->GetResDataINI()->GetKeyAsString(section, "selected");
			}
			break;
		case Verbal::BattleCry:
			if (IWDSound) {
				resource = core->GetResDataINI()->GetKeyAsString(section, "btlcry");
			}
			break;
		case Verbal::Attack1:
		case Verbal::Attack2:
		case Verbal::Attack3:
		case Verbal::Attack4:
			// FIXME: complete guess
			resource = core->GetResDataINI()->GetKeyAsString(section, StringView(IWDSound ? "att2" : "at2sound"));
			break;
		default:
			Log(WARNING, "Actor", "Cannot determine INI entry for index {} for {}, let us know!", int(index), fmt::WideToChar { LongName });
			return false;
	}

	auto elements = Explode<StringView, ResRef>(resource);
	size_t count = elements.size();
	if (count == 0) return false;

	int choice = core->Roll(1, int(count), -1);
	sound = elements[choice];

	return true;
}

void Actor::GetVerbalConstantSound(ResRef& Sound, Verbal index, bool resolved) const
{
	TableMgr::index_t idx = TableMgr::index_t(index);
	if (PCStats && !PCStats->SoundSet.IsEmpty()) {
		//resolving soundset (bg1/bg2 style)

		// handle nonstandard bg1 "default" soundsets first
		if (PCStats->SoundSet == "main") {
			static const char *suffixes[] = { "03", "08", "09", "10", "11", "17", "18", "19", "20", "21", "22", "38", "39" };
			static TableMgr::index_t VB2Suffix[] = { 9, 6, 7, 8, 20, 26, 27, 28, 32, 33, 34, 18, 19 };
			bool found = false;
			for (int i = 0; i < 13; i++) {
				if (VB2Suffix[i] == idx) {
					idx = i;
					found = true;
					break;
				}
			}
			if (!found) {
				Sound.Reset();
				return;
			}

			Sound.Format("{:.5}{:.2}", PCStats->SoundSet, suffixes[idx]);
			return;
		} else if (csound[idx]) {
			Sound.Format("{}{}", PCStats->SoundSet, csound[idx]);
			return;
		}

		//icewind style
		Sound.Format("{}{:02d}", PCStats->SoundSet, resolved ? Verbal(idx) : VCMap[idx]);
		return;
	}

	Sound.Reset();

	if (core->HasFeature(GFFlags::RESDATA_INI)) {
		GetSoundFromINI(Sound, Verbal(idx));
	} else {
		GetSoundFrom2DA(Sound, Verbal(idx));
	}

	//Empty resrefs
	if (IsStar(Sound) || Sound == "nosound") {
		Sound.Reset();
	}
}

void Actor::SetActionButtonRow(const ActionButtonRow &ar) const
{
	for(int i=0;i<GUIBT_COUNT;i++) {
		PCStats->QSlots[i] = ar[i];
	}
	if (QslotTranslation) dumpQSlots();
}

void Actor::GetActionButtonRow(ActionButtonRow &ar)
{
	//at this point, we need the stats for the action button row
	//only controlled creatures (and pcs) get it
	CreateStats();
	InitButtons(GetActiveClass(), false);
	for(int i=0;i<GUIBT_COUNT;i++) {
		ar[i] = IWD2GemrbQslot(i);
	}
}

int Actor::Gemrb2IWD2Qslot(ieByte actslot, int slotindex) const
{
	if (QslotTranslation && slotindex>2) {
		if (actslot > ACT_IWDQSONG) { // quick songs
			actslot = 110 + actslot % 10;
		} else if (actslot > ACT_IWDQSPEC) { // quick abilities
			actslot = 90 + actslot % 10;
		} else if (actslot > ACT_IWDQITEM) { // quick items
			actslot = 80 + actslot % 10;
		} else if (actslot > ACT_IWDQSPELL) { // quick spells
			actslot = 70 + actslot % 10;
		} else if (actslot > ACT_BARD) { // spellbooks
			actslot = 50 + actslot % 10;
		} else if (actslot >= 32) { // here be dragons
			Log(ERROR, "Actor", "Bad slot index passed to SetActionButtonRow!");
		} else {
			actslot = gemrb2iwd[actslot];
		}
	}
	return actslot;
}

ieByte Actor::IWD2GemrbQslot(int slotIndex) const
{
	ieByte qslot = PCStats->QSlots[slotIndex];
	//the first three buttons are hardcoded in gemrb
	//don't mess with them
	if (QslotTranslation && slotIndex > 2) {
		if (qslot >= 110) { //quick songs
			qslot = ACT_IWDQSONG + qslot % 10;
		} else if (qslot >= 90) { // quick abilities
			qslot = ACT_IWDQSPEC + qslot % 10;
		} else if (qslot >= 80) { // quick items
			qslot = ACT_IWDQITEM + qslot % 10;
		} else if (qslot >= 70) { // quick spells
			qslot = ACT_IWDQSPELL + qslot % 10;
		} else if (qslot >= 50) { // spellbooks
			qslot = ACT_BARD + qslot % 10;
		} else if (qslot >= 32) { // here be dragons
			Log(ERROR, "Actor", "Bad slot index passed to IWD2GemrbQslot!");
		} else {
			qslot = iwd2gemrb[qslot];
		}
	}
	return qslot;
}

// debug function; only works on pc classes
void Actor::dumpQSlots() const
{
	const ActionButtonRow& r = GUIBTDefaults[GetActiveClass()];
	std::string buffer;
	std::string buffer2;
	std::string buffer3;

	buffer.append("Current  default: ");
	buffer2.append("IWD2gem  default: ");
	buffer3.append("gem2IWD2 default: ");
	for(int i=0; i<GUIBT_COUNT; i++) {
		ieByte slot = r[i];
		AppendFormat(buffer, "{:3d} ", slot);
		AppendFormat(buffer2, "{:3d} ", IWD2GemrbQslot(slot));
		AppendFormat(buffer3, "{:3d} ", Gemrb2IWD2Qslot(slot, i));
	}
	AppendFormat(buffer, "(class: {})", GetStat(IE_CLASS));
	Log(DEBUG, "Actor", "{}", buffer);
//	Log(DEBUG, "Actor", buffer2);
//	Log(DEBUG, "Actor", buffer3);

	buffer.clear();
	buffer2.clear();
	buffer3.clear();
	buffer.append("Current  QSlots:  ");
	buffer2.append("IWD2gem  QSlots:  ");
	buffer3.append("gem2IWD2 QSlots:  ");
	for(int i=0; i<GUIBT_COUNT; i++) {
		ieByte slot = PCStats->QSlots[i];
		AppendFormat(buffer, "{:3d} ", slot);
		AppendFormat(buffer2, "{:3d} ", IWD2GemrbQslot(slot));
		AppendFormat(buffer3, "{:3d} ", Gemrb2IWD2Qslot(slot, i));
	}
	Log(DEBUG, "Actor", "{}", buffer);
	Log(DEBUG, "Actor", "{}", buffer2);
	Log(DEBUG, "Actor", "{}", buffer3);
}

void Actor::SetPortrait(const ResRef& portraitRef, int Which)
{
	if (!portraitRef) {
		return;
	}
	if (InParty) {
		core->SetEventFlag(EF_PORTRAIT);
	}

	if(Which!=1) {
		SmallPortrait = portraitRef;
	}
	if(Which!=2) {
		LargePortrait = portraitRef;
	}
	if(!Which) {
		// ensure they're properly terminated
		SmallPortrait.Format("{:.{}}S", SmallPortrait, 7);
		LargePortrait.Format("{:.{}}M", LargePortrait, 7);
	}
}

void Actor::SetSoundFolder(const String& soundset) const
{
	if (!core->HasFeature(GFFlags::SOUNDFOLDERS)) {
		PCStats->SoundSet = TLKStringFromString(soundset);
		return;
	}

	PCStats->SoundFolder = soundset;

	auto soundFolder = MBStringFromString(PCStats->SoundFolder);
	DirectoryIterator dirIt(PathJoin(core->config.GamePath, "sounds", soundFolder));
	dirIt.SetFilterPredicate(std::make_shared<EndsWithFilter>("01"));
	dirIt.SetFlags(DirectoryIterator::Directories);
	if (dirIt) {
		do {
			const path_t& name = dirIt.GetName();
			size_t end = FindFirstOf(name, ".");
			if (end != path_t::npos) {
				// need to truncate the "01" from the name, eg. HaFT_01.wav -> HaFT
				// but also 2df_007.wav -> 2df_0, meaning the data is less diverse than it may seem
				PCStats->SoundSet.Format("{:.{}}", name, int(&name[end] - 2 - name.c_str()));
				break;
			}
		} while (++dirIt);
	}
}

String Actor::GetSoundFolder(int full, const ResRef& overrideSet) const
{
	ResRef set;
	if (overrideSet.IsEmpty()) {
		set = PCStats->SoundSet;
	} else {
		set = overrideSet;
	}

	String wSet = StringFromResRef(set);
	String soundset;
	if (core->HasFeature(GFFlags::SOUNDFOLDERS)) {
		if (full) {
			soundset = fmt::format(u"{}{}{}", PCStats->SoundFolder, PathDelimiterW, wSet);
		} else {
			soundset = fmt::format(u"{}", PCStats->SoundFolder);
		}
	} else {
		soundset = wSet;
	}

	return soundset;
}

bool Actor::HasVVCCell(const ResRef &resource) const
{
	return GetVVCCells(resource).first != vfxDict.end();
}

bool VVCSort(const ScriptedAnimation* lhs, const ScriptedAnimation* rhs)
{
	return lhs->YOffset < rhs->YOffset;
}

std::pair<vvcDict::const_iterator, vvcDict::const_iterator>
Actor::GetVVCCells(const ResRef &resource) const
{
	return vfxDict.equal_range(resource);
}

void Actor::RemoveVVCells(const ResRef &resource)
{
	auto range = vfxDict.equal_range(resource);
	if (range.first != vfxDict.end()) {
		for (auto it = range.first; it != range.second; ++it) {
			ScriptedAnimation *vvc = it->second;
			vvc->SetPhase(P_RELEASE);
		}
	}
}

//this is a faster version of hasvvccell, because it knows where to look
//for the overlay, it also returns the vvc for further manipulation
//use this for the seven eyes overlay
ScriptedAnimation *Actor::FindOverlay(int index) const
{
	if (index >= OVERLAY_COUNT) return NULL;
	
	auto it = vfxDict.find(hc_overlays[index]);
	return (it != vfxDict.end()) ? it->second : nullptr;
}

void Actor::AddVVCell(ScriptedAnimation* vvc)
{
	assert(vvc);
	vvc->Pos = Pos;
	vfxDict.emplace(vvc->ResName, vvc);
	vfxQueue.insert(vvc);
	assert(vfxDict.size() == vfxQueue.size());
}

//returns restored spell level
int Actor::RestoreSpellLevel(ieDword maxlevel, ieDword type)
{
	int typemask;

	switch (type) {
		case 0: //allow only mage
			typemask = ~2;
			break;
		case 1: //allow only cleric
			typemask = ~1;
			break;
		default:
			//allow any (including innates)
			typemask = ~0;
	}
	for (int i=maxlevel;i>0;i--) {
		CREMemorizedSpell *cms = spellbook.FindUnchargedSpell(typemask, maxlevel);
		if (cms) {
			spellbook.ChargeSpell(cms);
			return i;
		}
	}
	return 0;
}
//replenishes spells, cures fatigue
void Actor::Rest(int hours)
{
	if (hours < 8) {
		// partial (interrupted) rest does not affect fatigue
		//do remove effects
		int remaining = hours*10;
		NewStat (IE_INTOXICATION, -remaining, MOD_ADDITIVE);
		//restore hours*10 spell levels
		//rememorization starts with the lower spell levels?
		inventory.ChargeAllItems (remaining);
		int level = 1;
		int memorizedSpell = 0;
		while (remaining > 0 && level < 16)
		{
			memorizedSpell = RestoreSpellLevel(level, -1);
			remaining -= memorizedSpell;
			if (memorizedSpell == 0)
			{
				level += 1;
			}
		}
	} else {
		TicksLastRested = LastFatigueCheck = core->GetGame()->GameTime;
		SetBase (IE_FATIGUE, 0);
		SetBase (IE_INTOXICATION, 0);
		inventory.ChargeAllItems (0);
		spellbook.ChargeAllSpells ();
	}
	ResetCommentTime();
}

//returns the actual slot from the quickslot
int Actor::GetQuickSlot(int slot) const
{
	assert(slot<8);
	if (!inventory.IsSlotEmpty(Inventory::GetMagicSlot())) {
		return Inventory::GetMagicSlot();
	}
	if (!PCStats) {
		return slot + Inventory::GetWeaponSlot();
	}
	return PCStats->QuickWeaponSlots[slot];
}

//marks the quickslot as equipped
HCStrings Actor::SetEquippedQuickSlot(int slot, int header)
{
	if (!PCStats) {
		inventory.SetEquippedSlot(ieWordSigned(slot), std::max<ieWord>(0, header));
		return HCStrings::count;
	}


	if ((slot<0) || (slot == IW_NO_EQUIPPED) ) {
		if (slot == IW_NO_EQUIPPED) {
			slot = Inventory::GetFistSlot();
		}
		int i;
		for(i=0;i<MAX_QUICKWEAPONSLOT;i++) {
			if (slot + Inventory::GetWeaponSlot() == PCStats->QuickWeaponSlots[i]) {
				slot = i;
				break;
			}
		}
		//if it is the fist slot and not currently used, then set it up
		if (i==MAX_QUICKWEAPONSLOT) {
			inventory.SetEquippedSlot(IW_NO_EQUIPPED, 0);
			return HCStrings::count;
		}
	}

	assert(slot<MAX_QUICKWEAPONSLOT);
	if (header==-1) {
		header = PCStats->QuickWeaponHeaders[slot];
	} else {
		PCStats->QuickWeaponHeaders[slot] = ieWord(header);
	}
	slot = Inventory::GetWeaponQuickSlot(PCStats->QuickWeaponSlots[slot]);
	if (inventory.SetEquippedSlot(ieWordSigned(slot), ieWord(header))) {
		return HCStrings::count;
	}
	return HCStrings::MagicWeapon;
}

// do we need Use magic device to succeed on a class usability check?
bool Actor::RequiresUMD(const Item* item) const
{
	if (!third) {
		return false;
	}
	if (item->ItemType != IT_WAND && item->ItemType != IT_SCROLL) {
		return false;
	}

	// we have to repeat some usability checks in case a thief or
	// bard got access via multiclassing and should skip the checks
	// OR, especially in the bard's case, if she already has access,
	// which is true for arcane scrolls and wands, but not divine.
	// Since it's a per-item thing, there might be exceptions as well
	if (!GetThiefLevel() && !GetBardLevel()) return false;

	// go through each class and check again
	ieDword levelSum = BaseStats[IE_CLASSLEVELSUM];
	for (ieDword i = 0; i < ISCLASSES; ++i) {
		if (!levelSum) break;
		ieDword level = GetClassLevel(i);
		if (!level) continue;
		levelSum -= level;

		// check if this class grants usability
		// stolen from CheckUsability() - refactor iff kits require it
		unsigned int classBit = 1 << (classesiwd2[i] - 1);
		ieDword itemvalue = item->UsabilityBitmask;
		if (classBit & ~itemvalue) {
			return false;
		}
	}

	return true;
}

bool Actor::TryUsingMagicDevice(const Item* item, ieDword header)
{
	if (!RequiresUMD(item)) return true;

	// from here on we know we have either a single class thief or
	// bard or one of them with a useless mixin class.
	// Nothing limiting left to check, as they must have positive
	// skill to get this far (or they couldn't have equipped the item)
	// and no usability yet.
	int skill = GetSkill(IE_MAGICDEVICE); // includes the charisma mod
	assert(skill > 0);

	int roll = LuckyRoll(1, 20, 0);
	// wands use effects directly -> fx->Power
	// scrolls use fx_cast_spell -> fx->Parameter1 (with 0 Power)
	const auto& effects = item->GetExtHeader(header)->features;
	const auto& fx = effects[0];
	int level = fx->Parameter1;
	if (fx->Power) {
		level = fx->Power;
	}

	// sorcerers.net suggests it's: 1d100 <= (skill + chaMod * 5) - level * 5;
	// other skill messages use a bunch of 7
	// but the string seems to be true in the original, which is also much more lenient
	bool success = (skill + roll) >= (level + 20);
	// 39304 = ~Use magic device check. Use magic device (skill + d20 roll + CHA modifier) = %d vs. (device's spell level + 20) = %d ( Spell level = %d ).~
	displaymsg->DisplayRollStringName(ieStrRef::ROLL14, GUIColors::LIGHTGREY, this, skill + roll, level + 20, level);

	if (success) {
		if (core->HasFeedback(FT_CASTING)) {
			displaymsg->DisplayStringName(core->GetString(ieStrRef::MD_SUCCESS), GUIColors::WHITE, this);
		}
		return true;
	}

	// don't play with powers you don't comprehend!
	if (core->HasFeedback(FT_CASTING)) {
		displaymsg->DisplayStringName(core->GetString(ieStrRef::MD_FAIL), GUIColors::WHITE, this);
	}
	Damage(core->Roll(level, 6, 0), DAMAGE_MAGIC, nullptr);
	return false;
}

//if target is a non living scriptable, then we simply shoot for its position
//the fx should get a NULL target, and handle itself by using the position
//(shouldn't crash when target is NULL)
bool Actor::UseItemPoint(ieDword slot, ieDword header, const Point &target, ieDword flags)
{
	CREItem *item = inventory.GetSlotItem(slot);
	if (!item)
		return false;
	// HACK: disable use when stunned (remove if stunned/petrified/etc actors stop running scripts)
	if (Immobile()) {
		return false;
	}

	// only one potion/wand per round
	if (!(flags&UI_NOAURA) && AuraPolluted()) {
		return false;
	}

	ResRef itemRef = item->ItemResRef;
	const Item *itm = gamedata->GetItem(itemRef, true);
	if (!itm) {
		Log(WARNING, "Actor", "Invalid quick slot item: {}!", itemRef);
		return false; //quick item slot contains invalid item resref
	}
	gamedata->FreeItem(itm, itemRef, false);

	if (!TryUsingMagicDevice(itm, header)) {
		ChargeItem(slot, header, item, itm, flags & UI_SILENT, !(flags & UI_NOCHARGE));
		AuraCooldown = core->Time.attack_round_size;
		return false;
	}

	//item is depleted for today
	if(itm->UseCharge(item->Usages, header, false)==CHG_DAY) {
		return false;
	}

	Projectile *pro = itm->GetProjectile(this, header, target, slot, flags&UI_MISS);
	ChargeItem(slot, header, item, itm, flags&UI_SILENT, !(flags&UI_NOCHARGE));
	if (!(flags & UI_NOAURA)) {
		AuraCooldown = core->Time.attack_round_size;
	}
	ResetCommentTime();
	if (pro) {
		pro->SetCaster(GetGlobalID(), ITEM_CASTERLEVEL);
		GetCurrentArea()->AddProjectile(pro, Pos, target);
		SetOrientation(target, Pos, false);
		return true;
	}
	return false;
}

static bool WeaponSlotMatchesHand(ieDword slot, const WeaponInfo& wi, const Effect* fx, bool leftOrRight)
{
	if (slot == 0 && fx->SourceRef == wi.item->Name) { // current weapon
		return true;
	} else if (slot == 1 && !leftOrRight) { // main hand weapon (or its ammo)
		return true;
	} else if (slot == 2 && leftOrRight) { // off-hand weapon
		return true;
	} else if (slot == 3) { // both weapons == all weapons
		return true;
	}
	return false;
}

// check Enchantment vs. creature type and plain enchantment bonuses
static ieDword AdjustEnchantment(const Actor* wielder, const Actor* target, const WeaponInfo& wi)
{
	ieDword enchantment = wi.enchantment;

	const Effect* fx = wielder->fxqueue.HasEffect(fx_enchantment_vs_creature_type_ref);
	if (fx && EffectQueue::match_ids(target, fx->Parameter2, fx->Parameter1) &&
		(!fx->Parameter4 || fx->Parameter4 == wi.item->ItemType) &&
		WeaponSlotMatchesHand(fx->Parameter3, wi, fx, wielder->usedLeftHand)) {
		enchantment = fx->IsVariable;
	}

	fx = wielder->fxqueue.HasEffect(fx_enchantment_bonus_ref);
	if (fx && (!fx->Parameter4 || fx->Parameter4 == wi.item->ItemType) &&
		WeaponSlotMatchesHand(fx->IsVariable, wi, fx, wielder->usedLeftHand)) {
		// the same list as fx_immune_to_weapon, but just goes up to 11
		bool match;
		switch (fx->Parameter2) {
			case 0: // enchantment level
				match = wi.enchantment <= fx->Parameter1;
				break;
			case 1: // all magical weapons
				match = wi.item->Flags & IE_ITEM_MAGICAL;
				break;
			case 2: // all non-magical weapons
				match = !(wi.item->Flags & IE_ITEM_MAGICAL);
				break;
			case 3: // all silver weapons
				match = wi.item->Flags & IE_ITEM_SILVER;
				break;
			case 4: // all non-silver weapons
				match = !(wi.item->Flags & IE_ITEM_SILVER);
				break;
			case 5: // all non-magical non-silver weapons
				match = !(wi.item->Flags & (IE_ITEM_MAGICAL | IE_ITEM_SILVER));
				break;
			case 6: // all twohanded
				match = wi.item->Flags & IE_ITEM_TWO_HANDED;
				break;
			case 7: // all not twohanded
				match = !(wi.item->Flags & IE_ITEM_TWO_HANDED);
				break;
			case 8: // all cursed
				match = wi.item->Flags & IE_ITEM_CURSED;
				break;
			case 9: // all non-cursed
				match = !(wi.item->Flags & IE_ITEM_CURSED);
				break;
			case 10: // all cold-iron
				match = wi.item->Flags & IE_ITEM_COLD_IRON;
				break;
			case 11: // all non cold-iron
				match = !(wi.item->Flags & IE_ITEM_COLD_IRON);
				break;
			default:
				match = false;
		}

		if (match) enchantment += fx->Parameter1;
	}

	return enchantment;
}

void Actor::ModifyWeaponDamage(WeaponInfo &wi, Actor *target, int &damage, bool &critical)
{
	//Calculate weapon based damage bonuses (strength bonus, dexterity bonus, backstab)
	ieDword adjustedEnchantment = AdjustEnchantment(this, target, wi);
	bool weaponImmunity = target->fxqueue.WeaponImmunity(adjustedEnchantment, wi.itemflags);
	int multiplier = Modified[IE_BACKSTABDAMAGEMULTIPLIER];
	int extraDamage = 0; // damage unaffected by the critical multiplier
	int level = static_cast<int>(GetXPLevel(false));

	if (third) {
		// 3ed sneak attack
		if (multiplier > 0) {
			extraDamage = GetSneakAttackDamage(target, wi, multiplier, weaponImmunity);
		}
	} else if (multiplier > 1) {
		// TODO: limit sneak attack to once per enemy? EEs did it via backstab.spl, used besides any custom BackstabResRef
		static const AutoTable sneakTable = gamedata->LoadTable("sneakatt", true);
		if (PreferSneakAttack && sneakTable) { // ee externalization
			std::string rowName = GetClassName(GetActiveClass());
			int rowIdx = sneakTable->GetRowIndex(rowName);
			int dice = sneakTable->QueryFieldSigned<int>(rowIdx, level - 1);
			extraDamage = LuckyRoll(dice, 6, 0, 0, target);
		} else if (PreferSneakAttack) {
			extraDamage = LuckyRoll(int(level / 4) + 1, 6, 0, 0, target); // 1d6 + 1d6 per 4 levels
		} else {
			// aDnD backstabbing
			damage = GetBackstabDamage(target, wi, multiplier, damage);
		}

		// crippling strike HoW or EE-style
		int spellPower = level;
		if (!BackstabResRef.IsEmpty()) { // ee externalization
			static const AutoTable cripTable = gamedata->LoadTable("crippstr", true);
			if (cripTable && PreferSneakAttack) {
				std::string rowName = GetClassName(GetActiveClass());
				int rowIdx = cripTable->GetRowIndex(rowName);
				spellPower = cripTable->QueryFieldSigned<int>(rowIdx, level - 1) + 1;
			}
			// extra spell delivery via fx_change_backstab
			core->ApplySpell(BackstabResRef, target, this, spellPower);
		} else if (PreferSneakAttack) {
			// Crippling Strike causes the victim to suffer a -1 to hit and damage rolls. The effect expires one turn later.
			int malus = - int((level - 1) / 4);
			Effect* fx = EffectQueue::CreateEffect(fx_to_hit_modifier_ref, malus, MOD_ADDITIVE, FX_DURATION_INSTANT_LIMITED);
			fx->Duration = core->Time.turn_sec;
			core->ApplyEffect(fx, target, this);
			Effect* fx2 = EffectQueue::CreateEffect(fx_damage_bonus_modifier1_ref, malus, MOD_ADDITIVE, FX_DURATION_INSTANT_LIMITED);
			fx2->Duration = core->Time.turn_sec;
			core->ApplyEffect(fx2, target, this);
		}
	}

	damage += WeaponDamageBonus(wi);

	if (weaponImmunity) {
		//'my weapon has no effect'
		damage = 0;
		critical = false;
		if (InParty) {
			if (core->HasFeedback(FT_COMBAT)) DisplayStringOrVerbalConstant(HCStrings::WeaponIneffective, Verbal::WeaponIneffective);
			core->Autopause(AUTOPAUSE::UNUSABLE, this);
		}
		return;
	}

	//critical protection a la PST
	if (pstflags && (target->Modified[IE_STATE_ID] & (ieDword) STATE_CRIT_PROT )) {
		critical = false;
	}

	if (critical) {
		if (target->inventory.ProvidesCriticalAversion()) {
			//critical hit is averted by helmet
			if (core->HasFeedback(FT_COMBAT)) displaymsg->DisplayConstantStringName(HCStrings::NoCritical, GUIColors::WHITE, target);
			critical = false;
		} else {
			//multiply the damage with the critical multiplier
			damage *= wi.critmulti;

			// check if critical hit needs a screenshake
			if (crit_hit_scr_shake && (InParty || target->InParty) ) {
				core->timer.SetScreenShake(Point(10, -10), core->Time.defaultTicksPerSec);
			}

			//apply the dirty fighting spell
			if (HasFeat(FEAT_DIRTY_FIGHTING) ) {
				core->ApplySpell(DirtyFightingRef, target, this, multiplier);
			}
		}
	}
	// add damage that is unaffected by criticals
	damage += extraDamage;
}

int Actor::GetSneakAttackDamage(Actor *target, WeaponInfo &wi, int &multiplier, bool weaponImmunity) {
	ieDword always = Modified[IE_ALWAYSBACKSTAB];
	bool invisible = Modified[IE_STATE_ID] & state_invisible;
	int sneakAttackDamage = 0;

	// TODO: should be rate limited (web says to once per 4 rounds?)
	// rogue is hidden or flanking OR the target is immobile (sleep ... stun)
	// or one of the stat overrides is set (unconfirmed for iwd2!)
	if (!invisible && !always && !target->Immobile() && !IsBehind(target)) {
		return 0;
	}

	bool dodgy = target->GetStat(IE_UNCANNY_DODGE) & 0x200;
	// if true, we need to be 4+ levels higher to still manage a sneak attack
	if (dodgy && GetStat(IE_CLASSLEVELSUM) >= target->GetStat(IE_CLASSLEVELSUM) + 4) {
		dodgy = false;
	}

	if (!target->Modified[IE_DISABLEBACKSTAB] && !weaponImmunity && !dodgy) {
		if (core->HasFeedback(FT_COMBAT)) displaymsg->DisplayConstantString(HCStrings::BackstabFail, GUIColors::WHITE);
		wi.backstabbing = false;
		return 0;
	}

	if (!wi.backstabbing) {
		// weapon is unsuitable for sneak attack
		if (core->HasFeedback(FT_COMBAT)) displaymsg->DisplayConstantString(HCStrings::BackstabBad, GUIColors::WHITE);
		return 0;
	}

	// first check for feats that change the sneak dice
	// special effects on hit for arterial strike (-1d6) and hamstring (-2d6)
	// both are available at level 10+ (5d6), so it's safe to decrease multiplier without checking
	if (!IsStar(BackstabResRef)) {
		if (BackstabResRef != ArterialStrikeRef) {
			// ~Sneak attack for %d inflicts hamstring damage (Slowed)~
			multiplier -= 2;
			sneakAttackDamage = LuckyRoll(multiplier, 6, 0, 0, target);
			displaymsg->DisplayRollStringName(ieStrRef::ROLL18, GUIColors::LIGHTGREY, this, sneakAttackDamage);
		} else {
			// ~Sneak attack for %d scores arterial strike (Inflicts bleeding wound)~
			multiplier--;
			sneakAttackDamage = LuckyRoll(multiplier, 6, 0, 0, target);
			displaymsg->DisplayRollStringName(ieStrRef::ROLL17, GUIColors::LIGHTGREY, this, sneakAttackDamage);
		}

		core->ApplySpell(BackstabResRef, target, this, multiplier);
		// do we need this?
		BackstabResRef.Reset();
		if (HasFeat(FEAT_CRIPPLING_STRIKE)) {
			core->ApplySpell(CripplingStrikeRef, target, this, multiplier);
		}
	}

	if (!sneakAttackDamage) {
		sneakAttackDamage = LuckyRoll(multiplier, 6, 0, 0, target);
		// ~Sneak Attack for %d~
		//displaymsg->DisplayRollStringName(25053, GUIColors::LIGHTGREY, this, extraDamage);
		if (core->HasFeedback(FT_COMBAT)) displaymsg->DisplayConstantStringValue(HCStrings::BackstabDamage, GUIColors::WHITE, sneakAttackDamage);
	}

	return sneakAttackDamage;
}

int Actor::GetBackstabDamage(const Actor *target, WeaponInfo &wi, int multiplier, int damage) const
{
	ieDword always = Modified[IE_ALWAYSBACKSTAB];
	bool invisible = Modified[IE_STATE_ID] & state_invisible;
	int backstabDamage = damage;

	//ToBEx compatibility in the ALWAYSBACKSTAB field:
	//0 Normal conditions (attacker must be invisible, attacker must be in 90-degree arc behind victim)
	//1 Ignore invisible requirement and positioning requirement
	//2 Ignore invisible requirement only
	//4 Ignore positioning requirement only
	if (!invisible && !(always&0x3)) {
		return backstabDamage;
	}

	if ((!core->HasFeature(GFFlags::PROPER_BACKSTAB) || !IsBehind(target)) && !(always & 0x5)) {
		return backstabDamage;
	}

	if (target->Modified[IE_DISABLEBACKSTAB]) {
		// The backstab seems to have failed
		if (core->HasFeedback(FT_COMBAT)) displaymsg->DisplayConstantString(HCStrings::BackstabFail, GUIColors::WHITE);
		wi.backstabbing = false;
	} else {
		if (wi.backstabbing) {
			backstabDamage = multiplier * damage;
			if (!core->HasFeedback(FT_COMBAT)) return backstabDamage;

			ieStrRef multiplierText;
			// generate "double", "triple", up to "sextuple" strref as needed
			if (multiplier >= 7) { // only in tob, but the strings are at a different offset
				multiplierText = ieStrRef(int(ieStrRef::TOB_SEPTUPLE) + multiplier - 7);
			} else {
				multiplierText = ieStrRef(int(DisplayMessage::GetStringReference(HCStrings::BackstabDouble, this)) + multiplier - 2);
			}
			if (multiplier < 7 || (core->HasFeature(GFFlags::JOURNAL_HAS_SECTIONS) && multiplier < 10)) {
				displaymsg->DisplayStringName(multiplierText, GUIColors::WHITE, this, STRING_FLAGS::SOUND);
			} else {
				// display a simple message for all other cases
				displaymsg->DisplayConstantStringValue(HCStrings::BackstabDamage, GUIColors::WHITE, multiplier);
			}
		} else if (core->HasFeedback(FT_COMBAT)) {
			// weapon is unsuitable for backstab
			displaymsg->DisplayConstantString(HCStrings::BackstabBad, GUIColors::WHITE);
		}
	}

	return backstabDamage;
}

bool Actor::UseItem(ieDword slot, ieDword header, const Scriptable* target, ieDword flags, int damage)
{
	assert(target);
	const Actor *tar = Scriptable::As<Actor>(target);
	if (!tar) {
		return UseItemPoint(slot, header, target->Pos, flags);
	}
	// HACK: disable use when stunned (remove if stunned/petrified/etc actors stop running scripts)
	if (Immobile()) {
		return false;
	}

	// only one potion per round; skip for our internal attack projectile
	if (!(flags&UI_NOAURA) && AuraPolluted()) {
		return false;
	}

	CREItem *item = inventory.GetSlotItem(slot);
	if (!item)
		return false;

	ResRef itemRef = item->ItemResRef;
	const Item *itm = gamedata->GetItem(itemRef);
	if (!itm) {
		Log(WARNING, "Actor", "Invalid quick slot item: {}!", itemRef);
		return false; //quick item slot contains invalid item resref
	}
	gamedata->FreeItem(itm, itemRef, false);

	if (!TryUsingMagicDevice(itm, header)) {
		ChargeItem(slot, header, item, itm, flags & UI_SILENT, !(flags & UI_NOCHARGE));
		AuraCooldown = core->Time.attack_round_size;
		return false;
	}

	//item is depleted for today
	if (itm->UseCharge(item->Usages, header, false)==CHG_DAY) {
		return false;
	}

	Projectile *pro = itm->GetProjectile(this, header, target->Pos, slot, flags&UI_MISS);

	// ChargeItem can break the item, now invalidating everything, so look things up in advance
	int weaponTypeIdx = 0;
	bool ranged = header == (ieDword) -2;
	ieDword projectileAnim = 0;
	if (((int) header < 0) && !(flags & UI_MISS)) { // using a weapon
		const ITMExtHeader* which = itm->GetWeaponHeader(ranged);
		if (!which) return false; // eg. misc8u equipped by saemon havarian (ppsaem3), part of the silver sword and actually has a header, just untyped
		weaponTypeIdx = which->DamageType;
		projectileAnim = which->ProjectileAnimation;
	}
	ChargeItem(slot, header, item, itm, flags&UI_SILENT, !(flags&UI_NOCHARGE));

	if (!(flags&UI_NOAURA)) {
		AuraCooldown = core->Time.attack_round_size;
	}
	ResetCommentTime();
	if (!pro) {
		return false;
	}

	pro->SetCaster(GetGlobalID(), ITEM_CASTERLEVEL);
	if (flags & UI_FAKE) {
		delete pro;
	} else if (((int) header < 0) && !(flags & UI_MISS)) { // using a weapon
		Effect* AttackEffect = EffectQueue::CreateEffect(fx_damage_ref, damage, weapon_damagetype[weaponTypeIdx] << 16, FX_DURATION_INSTANT_LIMITED);
		AttackEffect->Projectile = projectileAnim;
		AttackEffect->Target = FX_TARGET_PRESET;
		AttackEffect->Parameter3 = 1;
		AttackEffect->SourceType = 1;
		AttackEffect->SourceRef = itemRef;
		AttackEffect->CasterID = GetGlobalID();
		AttackEffect->VariableName = scriptName;
		if (pstflags) {
			AttackEffect->IsVariable = GetCriticalType();
		} else {
			AttackEffect->IsVariable = flags & UI_CRITICAL;
		}
		pro->GetEffects().AddEffect(AttackEffect, true);
		if (ranged) {
			fxqueue.AddWeaponEffects(&pro->GetEffects(), fx_ranged_ref);
		} else {
			// EEs add a a single bit to fx_melee for only applying with monk fists
			int param2 = (inventory.FistsEquipped() && GetMonkLevel()) ? 4 : 0;
			fxqueue.AddWeaponEffects(&pro->GetEffects(), fx_melee_ref, param2);
			// ignore timestop
			pro->TFlags |= PTF_TIMELESS;
		}
		attackProjectile = pro;
	} else { // launch it now as we are not attacking
		GetCurrentArea()->AddProjectile(pro, Pos, tar->GetGlobalID(), false);
		SetOrientation(target->Pos, Pos, false);
	}
	return true;
}

void Actor::ChargeItem(ieDword slot, ieDword header, CREItem *item, const Item *itm, bool silent, bool expend)
{
	if (!itm) {
		item = inventory.GetSlotItem(slot);
		if (!item)
			return;
		itm = gamedata->GetItem(item->ItemResRef, true);
	}
	if (!itm) {
		Log(WARNING, "Actor", "Invalid quick slot item: {}!", item->ItemResRef);
		return; //quick item slot contains invalid item resref
	}

	if (IsSelected()) {
		core->SetEventFlag( EF_ACTION );
	}

	if (!silent) {
		ieByte stance = gamedata->GetItemAnimation(item->ItemResRef);
		if (!stance) stance = AttackStance;

		if (stance!=0xff) {
			SetStance(stance);
			//play only one cycle of animations

			// this was crashing for fuzzie due to NULL anims
			if (anims) {
				anims->nextStanceID=IE_ANI_READY;
				anims->autoSwitchOnEnd=true;
			}
		}
	}

	switch(itm->UseCharge(item->Usages, header, expend)) {
		case CHG_DAY:
			break;
		case CHG_BREAK: //both
			if (!silent) {
				core->PlaySound(DS_ITEM_GONE, SFX_CHAN_GUI);
			}
			//fall through
		case CHG_NOSOUND: //remove item
			inventory.BreakItemSlot(slot);
			break;
		default: //don't do anything
			break;
	}
}

int Actor::IsReverseToHit()
{
	return ReverseToHit;
}

void Actor::InitButtons(ieDword cls, bool forced) const
{
	if (!PCStats) {
		return;
	}
	if ( (PCStats->QSlots[0]!=0xff) && !forced) {
		return;
	}

	ActionButtonRow& myrow = DefaultButtons;
	if (cls >= (ieDword) classcount) {
		for (const auto& otherButtons : OtherGUIButtons) {
			if (cls == otherButtons.clss) {
				myrow  = otherButtons.buttons;
				break;
			}
		}
	} else {
		myrow = GUIBTDefaults[cls];
	}
	SetActionButtonRow(myrow);
}

void Actor::SetFeat(unsigned int feat, BitOp mode)
{
	if (feat>=MAX_FEATS) {
		return;
	}
	ieDword mask = 1<<(feat&31);
	ieDword idx = feat>>5;
	
	SetBits(BaseStats[IE_FEATS1+idx], mask, mode);
}

void Actor::SetFeatValue(unsigned int feat, int value, bool init)
{
	if (feat>=MAX_FEATS) {
		return;
	}

	//handle maximum and minimum values
	if (value<0) value = 0;
	else if (value>featmax[feat]) value = featmax[feat];

	if (value) {
		SetFeat(feat, BitOp::OR);
		if (featstats[feat]) SetBase(featstats[feat], value);
	} else {
		SetFeat(feat, BitOp::NAND);
		if (featstats[feat]) SetBase(featstats[feat], 0);
	}

	if (init) {
		 ApplyFeats();
	}
}

void Actor::ClearCurrentStanceAnims()
{
	currentStance.anim.clear();
	currentStance.shadow.clear();
}

void Actor::SetUsedWeapon(AnimRef AnimationType, const ieWord* MeleeAnimation, unsigned char wt)
{
	WeaponRef = AnimationType;
	if (wt != IE_ANI_WEAPON_INVALID) WeaponType = wt;
	if (!anims)
		return;
		
	anims->SetWeaponRef(AnimationType);
	anims->SetWeaponType(WeaponType);
	ClearCurrentStanceAnims();
	SetAttackMoveChances(MeleeAnimation);
	if (InParty) {
		//update the paperdoll weapon animation
		core->SetEventFlag(EF_UPDATEANIM);
	}

	const ITMExtHeader* header = GetWeapon(false);
	if (header && header->AttackType == ITEM_AT_PROJECTILE && !header->ProjectileQualifier) {
		AttackStance = IE_ANI_ATTACK_SLASH; // that's it, "throw" it!
		return;
	}
	if (header && weaponInfo[0].wflags & WEAPON_RANGED) {
		if (header->ProjectileQualifier == 0) return; // no ammo yet?
		AttackStance = IE_ANI_SHOOT;
		anims->SetRangedType(header->ProjectileQualifier - 1);
		//bows ARE one handed, from an anim POV at least
		anims->SetWeaponType(IE_ANI_WEAPON_1H);
		return;
	}
	AttackStance = IE_ANI_ATTACK;
}

void Actor::SetUsedShield(AnimRef AnimationType, unsigned char wt)
{
	ShieldRef = AnimationType;
	if (wt != IE_ANI_WEAPON_INVALID) WeaponType = wt;
	if (AnimationType[0] == ' ' || AnimationType[0] == 0)
		if (WeaponType == IE_ANI_WEAPON_2W)
			WeaponType = IE_ANI_WEAPON_1H;

	if (!anims)
		return;
	
	anims->SetOffhandRef(AnimationType);
	anims->SetWeaponType(WeaponType);
	ClearCurrentStanceAnims();
	if (InParty) {
		//update the paperdoll weapon animation
		core->SetEventFlag(EF_UPDATEANIM);
	}
}

void Actor::SetUsedHelmet(AnimRef AnimationType)
{
	HelmetRef = AnimationType;
	if (!anims)
		return;
	
	anims->SetHelmetRef(AnimationType);
	ClearCurrentStanceAnims();
	if (InParty) {
		//update the paperdoll weapon animation
		core->SetEventFlag(EF_UPDATEANIM);
	}
}

void Actor::SetupFist()
{
	int slot = core->QuerySlot( 0 );
	assert (core->QuerySlotEffects(slot)==SLOT_EFFECT_FIST);
	int row = GetBase(fiststat);
	int col = GetXPLevel(false);
	col = Clamp(col, 1, MAX_LEVEL);

	ResRef ItemResRef = gamedata->GetFist(row, col);

	const CREItem *currentFist = inventory.GetSlotItem(slot);
	if (!currentFist || currentFist->ItemResRef != ItemResRef) {
		inventory.SetSlotItemRes(ItemResRef, slot);
	}
}

static ieDword ResolveTableValue(const ResRef& resref, ieDword stat, ieDword mcol, ieDword vcol) {
	//don't close this table, it can mess with the guiscripts
	auto tm = gamedata->LoadTable(resref);
	if (tm) {
		TableMgr::index_t row;
		if (mcol == 0xff) {
			row = stat;
		} else {
			row = tm->FindTableValue(mcol, stat);
			if (row==0xffffffff) {
				return 0;
			}
		}
		ieDword ret;
		if (valid_unsignednumber(tm->QueryField(row, vcol).c_str(), ret)) {
			return ret;
		}
	}

	return 0;
}

HCStrings Actor::CheckUsability(const Item* item) const
{
	ieDword itembits[2]={item->UsabilityBitmask, item->KitUsability};
	int kitignore = 0;

	const auto& itemUse = gamedata->GetItemUse();
	for (size_t i = 0; i < itemUse.size(); i++) {
		ieDword itemvalue = itembits[itemUse[i].which];
		stat_t stat = GetStat(itemUse[i].stat);
		ieDword mcol = itemUse[i].mcol;
		//if we have a kit, we just use its index for the lookup
		if (itemUse[i].stat == IE_KIT) {
			if (!iwd2class) {
				if (IsKitInactive()) continue;

				stat = GetKitIndex(stat);
				mcol = 0xff;
			} else {
				//iwd2 doesn't need translation from kit to usability, the kit value IS usability
				// But it's more complicated: check the comments below.
				// Skip undesired kits
				stat &= ~kitignore;
				goto no_resolve;
			}
		}

		if (!iwd2class && itemUse[i].stat == IE_CLASS) {
			// account for inactive duals
			stat = GetActiveClass();
		}

		if (iwd2class && itemUse[i].stat == IE_CLASS) {
			// in iwd2 any class mixin can enable the use, but the stat only holds the first class;
			// it also means we shouldn't check all kits (which we do last)!
			// Eg. a paladin of Mystra/sorcerer is allowed to use wands,
			// but the kit check would fail, since paladins and their kits aren't.
			stat = GetClassMask();

			// Use Magic Device can only be used on wands and scrolls, but also
			// only by bards and thieves (restriction handled by skilcost.2da)
			// the actual roll happens when the actor tries to use it
			if ((item->ItemType == IT_WAND || item->ItemType == IT_SCROLL) && GetSkill(IE_MAGICDEVICE) > 0) {
				continue;
			}

			// check everyone else
			if (stat & ~itemvalue) {
				if (Modified[IE_KIT] == 0) continue;
			} else {
				return HCStrings::CantUseItem;
			}

			// classes checked out, but we're kitted ...
			// ignore kits from "unusable" classes
			for (int j=0; j < ISCLASSES; j++) {
				if (Modified[levelslotsiwd2[j]] == 0) continue;
				if ((1<<(classesiwd2[j] - 1)) & ~itemvalue) continue;

				for (const auto& kit : class2kits[classesiwd2[j]].ids) {
					kitignore |= kit;
				}
			}
			continue;
		}

		stat = ResolveTableValue(itemUse[i].table, stat, mcol, itemUse[i].vcol);

no_resolve:
		if (stat&itemvalue) {
			//Log(DEBUG, "Actor", "failed usability: itemvalue {}, stat {}, stat value {}", itemvalue, itemuse[i].stat, stat);
			return HCStrings::CantUseItem;
		}
	}

	return HCStrings::count;
}

//this one is the same, but returns strrefs based on effects
ieStrRef Actor::Disabled(const ResRef& name, ieDword type) const
{
	const Effect *fx = fxqueue.HasEffectWithResource(fx_cant_use_item_ref, name);
	if (fx) {
		return ieStrRef(fx->Parameter1);
	}

	fx = fxqueue.HasEffectWithParam(fx_cant_use_item_type_ref, type);
	if (fx) {
		return ieStrRef(fx->Parameter1);
	}

	fx = fxqueue.HasEffectWithSource(fx_item_usability_ref, name);
	if (fx && fx->Parameter3 == 1) {
		return ieStrRef(fx->IsVariable);
	}
	return ieStrRef::INVALID;
}

//checks usability only
HCStrings Actor::Unusable(const Item* item) const
{
	// skip regular usability check if permission is granted by effect or HLA
	const Effect* fx = fxqueue.HasEffectWithSource(fx_item_usability_ref, item->Name);
	if (fx && fx->Parameter3 == 1) {
		return HCStrings::CantUseItem;
	}
	if (!GetStat(IE_CANUSEANYITEM) && !fx) {
		HCStrings unusable = CheckUsability(item);
		if (unusable != HCStrings::count) {
			return unusable;
		}
	}

	// iesdp says this is always checked?
	if (item->MinLevel>GetXPLevel(true)) {
		return HCStrings::CantUseItem;
	}

	if (!CheckAbilities) {
		return HCStrings::count;
	}

	if (item->MinStrength>GetStat(IE_STR)) {
		return HCStrings::CantUseItem;
	}

	if (item->MinStrength==18) {
		if (GetStat(IE_STR)==18) {
			if (item->MinStrengthBonus>GetStat(IE_STREXTRA)) {
				return HCStrings::CantUseItem;
			}
		}
	}

	if (item->MinIntelligence>GetStat(IE_INT)) {
		return HCStrings::CantUseItem;
	}
	if (item->MinDexterity>GetStat(IE_DEX)) {
		return HCStrings::CantUseItem;
	}
	if (item->MinWisdom>GetStat(IE_WIS)) {
		return HCStrings::CantUseItem;
	}
	if (item->MinConstitution>GetStat(IE_CON)) {
		return HCStrings::CantUseItem;
	}
	if (item->MinCharisma>GetStat(IE_CHR)) {
		return HCStrings::CantUseItem;
	}
	//note, weapon proficiencies shouldn't be checked here
	//missing proficiency causes only attack penalty
	return HCStrings::count;
}

//full palette will be shaded in gradient color
void Actor::SetGradient(ieDword gradient)
{
	gradient |= (gradient <<16);
	gradient |= (gradient <<8);
	for(int i=0;i<7;i++) {
		Modified[IE_COLORS+i]=gradient;
	}
}

//sets one bit of the sanctuary stat (used for overlays)
void Actor::SetOverlay(unsigned int overlay)
{
	if (overlay >= OVERLAY_COUNT) return;
	// need to run the pcf, so the vvcs get loaded
	SetStat(IE_SANCTUARY, Modified[IE_SANCTUARY] | (1<<overlay), 0);
}

//returns true if spell state is already set or illegal
bool Actor::SetSpellState(unsigned int spellstate) const
{
	if (spellstate >= SpellStatesSize << 5) return true;
	unsigned int pos = spellstate >> 5;
	unsigned int bit = 1 << (spellstate & 31);
	if (spellStates[pos] & bit) return true;
	spellStates[pos] |= bit;
	return false;
}

//returns true if spell state is already set
bool Actor::HasSpellState(unsigned int spellstate) const
{
	if (spellstate >= SpellStatesSize << 5) return false;
	unsigned int pos = spellstate >> 5;
	unsigned int bit = 1 << (spellstate & 31);
	if (spellStates[pos] & bit) return true;
	return false;
}

int Actor::GetMaxEncumbrance() const
{
	int max = core->GetStrengthBonus(3, GetStat(IE_STR), GetStat(IE_STREXTRA));
	if (HasFeat(FEAT_STRONG_BACK)) max += max/2;
	return max;
}

//this is a very specific rule that might need an external table later
int Actor::GetAbilityBonus(unsigned int ability, int value) const
{
	if (value == -1) { // invalid (default), use the current value
		return GetStat(ability)/2-5;
	} else {
		return value/2-5;
	}
}

int Actor::GetSkillStat(unsigned int skill) const
{
	if (skill >= skillstats.size()) return -1;
	return skillstats[skill][0];
}

int Actor::GetSkill(unsigned int skill, bool ids) const
{
	if (!ids) {
		// called with a stat, not a skill index
		skill = stat2skill[skill];
	}
	if (skill >= skillstats.size()) return -1;
	int ret = GetStat(skillstats[skill][0]);
	int base = GetBase(skillstats[skill][0]);
	int modStat = skillstats[skill][1];
	// only give other boni for trained skills or those that don't require it
	// untrained trained skills are not usable!
	// DEX is handled separately by GetSkillBonus and applied directly after effects
	if (base > 0 || skillstats[skill][2]) {
		if (modStat != IE_DEX) {
			ret += GetAbilityBonus(modStat);
		}
	} else {
		ret = 0;
	}
	if (ret<0) ret = 0;
	return ret;
}

//returns the numeric value of a feat, different from HasFeat
//for multiple feats
int Actor::GetFeat(unsigned int feat) const
{
	if (feat>=MAX_FEATS) {
		return -1;
	}
	if (BaseStats[IE_FEATS1+(feat>>5)]&(1<<(feat&31)) ) {
		//return the numeric stat value, instead of the boolean
		if (featstats[feat]) {
			return Modified[featstats[feat]];
		}
		return 1;
	}
	return 0;
}

//returns true if the feat exists
bool Actor::HasFeat(unsigned int featindex) const
{
	if (featindex>=MAX_FEATS) return false;
	unsigned int pos = IE_FEATS1+(featindex>>5);
	unsigned int bit = 1<<(featindex&31);
	if (BaseStats[pos]&bit) return true;
	return false;
}

ieDword Actor::ImmuneToProjectile(ieDword projectile) const
{
	if (projectile >= projectileImmunity.size()) {
		return 0;
	}
	return projectileImmunity[projectile];
}

void Actor::AddProjectileImmunity(ieDword projectile)
{
	std::vector<bool>::reference proBit = projectileImmunity[projectile];
	proBit = true;
}

//2nd edition rules
void Actor::CreateDerivedStatsBG()
{
	int turnundeadlevel = 0;
	int classid = BaseStats[IE_CLASS];
	static int defaultAC = gamedata->GetMiscRule("DEFAULT_AC");

	//this works only for PC classes
	if (classid>=CLASS_PCCUTOFF) return;

	//recalculate all level based changes
	pcf_level(this,0,0);

	// barbarian immunity to backstab was hardcoded
	if (GetBarbarianLevel()) {
		BaseStats[IE_DISABLEBACKSTAB] = 1;
	}

	for (int i=0;i<ISCLASSES;i++) {
		if (classesiwd2[i]>=(ieDword) classcount) continue;
		int tl = turnLevelOffset[classesiwd2[i]];
		if (!tl) continue;

		int adjustedTL = GetClassLevel(i) + 1 - tl;
		// adding up turn undead levels, but this is probably moot
		// anyway, you will be able to create custom priest/paladin classes
		if (adjustedTL > 0) {
			turnundeadlevel += adjustedTL;
		}
	}

	stat_t backstabdamagemultiplier = GetThiefLevel();
	if (backstabdamagemultiplier) {
		// swashbucklers can't backstab, but backstab.2da only has THIEF in it
		if (BaseStats[IE_KIT] == KIT_SWASHBUCKLER) {
			backstabdamagemultiplier = 1;
		} else {
			AutoTable tm = gamedata->LoadTable("backstab");
			//fallback to a general algorithm (bg2 backstab.2da version) if we can't find backstab.2da
			// assassin's AP_SPCL332 (increase backstab by one) is not effecting this at all,
			// it's just applied later
			// stalkers work by just using the effect, since they're not thieves
			if (tm)	{
				backstabdamagemultiplier = std::min(backstabdamagemultiplier, tm->GetColumnCount());
				backstabdamagemultiplier = tm->QueryFieldUnsigned<stat_t>(0, backstabdamagemultiplier);
			} else {
				backstabdamagemultiplier = (backstabdamagemultiplier+7)/4;
			}
			backstabdamagemultiplier = std::min(backstabdamagemultiplier, 5U);
		}
	}

	weapSlotCount = numWeaponSlots[GetActiveClass()];
	ReinitQuickSlots();

	// monk's level dictated ac and ac vs missiles bonus
	// attacks per round bonus will be handled elsewhere, since it only applies to fist apr
	if (isclass[ISMONK]&(1<<classid)) {
		unsigned int level = GetMonkLevel();
		AC.SetNatural(defaultAC - gamedata->GetMonkBonus(1, level));
		BaseStats[IE_ACMISSILEMOD] = - gamedata->GetMonkBonus(2, level);
	}

	BaseStats[IE_TURNUNDEADLEVEL]=turnundeadlevel;
	BaseStats[IE_BACKSTABDAMAGEMULTIPLIER]=backstabdamagemultiplier;
	BaseStats[IE_LAYONHANDSAMOUNT]=GetPaladinLevel()*2;
}

//3rd edition rules
void Actor::CreateDerivedStatsIWD2()
{
	int classid = BaseStats[IE_CLASS];

	// this works only for PC classes
	if (classid>=CLASS_PCCUTOFF) return;

	// recalculate all level based changes
	pcf_level(this, 0, 0, classid);

	// iwd2 does have backstab.2da but it is both unused and with bad data
	ieDword backstabdamagemultiplier = 0;
	int level = GetThiefLevel();
	if (level) {
		// +1d6 for each odd level
		backstabdamagemultiplier = (level + 1) / 2;
	}

	int turnundeadlevel = 0;
	for (int i = 0; i < ISCLASSES; i++) {
		if (classesiwd2[i]>=(ieDword) classcount) continue;
		int tl = turnLevelOffset[classesiwd2[i]];
		if (tl) {
			int adjustedTL = GetClassLevel(i) + 1 - tl;
			if (adjustedTL > 0) {
				//the levels add up (checked)
				turnundeadlevel += adjustedTL;
			}
		}
	}
	BaseStats[IE_TURNUNDEADLEVEL]=turnundeadlevel;
	BaseStats[IE_BACKSTABDAMAGEMULTIPLIER]=backstabdamagemultiplier;
}

void Actor::ResetMC()
{
	if (iwd2class) {
		multiclass = 0;
	} else {
		ieDword cls = BaseStats[IE_CLASS]-1;
		if (cls >= (ieDword) classcount) {
			multiclass = 0;
		} else {
			multiclass = multiclassIDs[cls];
		}
	}
}

//set up stuff here, like attack number, turn undead level
//and similar derived stats that change with level
void Actor::CreateDerivedStats()
{
	ResetMC();

	if (third) {
		CreateDerivedStatsIWD2();
	} else {
		CreateDerivedStatsBG();
	}

	// check for HoF upgrade
	const Game *game = core->GetGame();
	if (!InParty && game && game->HOFMode && !(BaseStats[IE_MC_FLAGS] & (MC_HOF_UPGRADED | MC_NO_NIGHTMARE_MODS))) {
		BaseStats[IE_MC_FLAGS] |= MC_HOF_UPGRADED;

		// our summons get less of an hp boost
		if (BaseStats[IE_EA] <= EA_CONTROLLABLE) {
			BaseStats[IE_MAXHITPOINTS] = 2 * BaseStats[IE_MAXHITPOINTS] + 20;
			BaseStats[IE_HITPOINTS] = 2 * BaseStats[IE_HITPOINTS] + 20;
		} else {
			BaseStats[IE_MAXHITPOINTS] = 3 * BaseStats[IE_MAXHITPOINTS] + 80;
			BaseStats[IE_HITPOINTS] = 3 * BaseStats[IE_HITPOINTS] + 80;
		}

		if (third) {
			BaseStats[IE_CR] += 10;
			BaseStats[IE_STR] += 10;
			BaseStats[IE_DEX] += 10;
			BaseStats[IE_CON] += 10;
			BaseStats[IE_INT] += 10;
			BaseStats[IE_WIS] += 10;
			BaseStats[IE_CHR] += 10;
			for (int i = 0; i < ISCLASSES; i++) {
				int level = GetClassLevel(i);
				if (!level) continue;
				BaseStats[levelslotsiwd2[i]] += 12;
			}
			// NOTE: this is a guess, reports vary
			// the attribute increase already contributes +5
			for (int i = 0; i <= IE_SAVEWILL - IE_SAVEFORTITUDE; i++) {
				BaseStats[savingThrows[i]] += 5;
			}
		} else {
			BaseStats[IE_NUMBEROFATTACKS] += 2; // 1 more APR
			ToHit.HandleFxBonus(5, true);
			if (BaseStats[IE_XPVALUE]) {
				BaseStats[IE_XPVALUE] = 2 * BaseStats[IE_XPVALUE] + 1000;
			}
			if (BaseStats[IE_GOLD]) {
				BaseStats[IE_GOLD] += 75;
			}
			if (BaseStats[IE_LEVEL]) {
				BaseStats[IE_LEVEL] += 12;
			}
			if (BaseStats[IE_LEVEL2]) {
				BaseStats[IE_LEVEL2] += 12;
			}
			if (BaseStats[IE_LEVEL3]) {
				BaseStats[IE_LEVEL3] += 12;
			}
			for (int savingThrow : savingThrows) {
				BaseStats[savingThrow]--;
			}
		}
	}
}
/* Checks if the actor is multiclassed (the MULTI column is positive) */
bool Actor::IsMultiClassed() const
{
	return (multiclass > 0);
}

/* Checks if the actor is dualclassed */
bool Actor::IsDualClassed() const
{
	// exclude the non-player classes
	if (!HasPlayerClass()) return false;

	// make sure only one bit is set, as critters like kuo toa have garbage in the mc bits
	return CountBits(Modified[IE_MC_FLAGS] & MC_WAS_ANY) == 1;
}

Actor *Actor::CopySelf(bool mislead) const
{
	Actor *newActor = new Actor();

	newActor->SetName(GetShortName(), 0);
	newActor->SetName(GetName(),1);
	newActor->SetScriptName("COPY");
	newActor->creVersion = creVersion;
	newActor->BaseStats = BaseStats;
	// illusions aren't worth any xp and don't explore
	newActor->BaseStats[IE_XPVALUE] = 0;
	newActor->BaseStats[IE_EXPLORE] = 0;

	//IF_INITIALIZED shouldn't be set here, yet
	newActor->SetMCFlag(MC_EXPORTABLE, BitOp::NAND);

	// adjust EA
	if (newActor->BaseStats[IE_EA] <= EA_GOODCUTOFF) {
		newActor->BaseStats[IE_EA] = EA_ALLY;
	} else if (newActor->BaseStats[IE_EA] >= EA_EVILCUTOFF) {
		newActor->BaseStats[IE_EA] = EA_ENEMY;
	} else {
		newActor->BaseStats[IE_EA] = EA_NEUTRAL;
	}

	//the creature importer does this too
	newActor->Modified = newActor->BaseStats;

	//copy the inventory, but only if it is not the Mislead illusion
	if (mislead) {
		//these need to be called too to have a valid inventory
		newActor->inventory.SetSlotCount(inventory.GetSlotCount());
	} else {
		newActor->inventory.CopyFrom(this);
		if (PCStats) {
			newActor->CreateStats();
			*newActor->PCStats = *PCStats;
		}
	}

	//copy the spellbook, if any
	if (!mislead) {
		newActor->spellbook.CopyFrom(this);
	}

	newActor->CreateDerivedStats();

	area->AddActor(newActor, true);
	newActor->SetPosition( Pos, CC_CHECK_IMPASSABLE, 0 );
	newActor->SetOrientation(GetOrientation(), false);
	newActor->SetStance( IE_ANI_READY );

	//copy the running effects
	newActor->AddEffects(EffectQueue(fxqueue));
	return newActor;
}

//high level function, used by scripting
ieDword Actor::GetLevelInClass(ieDword classid) const
{
	if (creVersion == CREVersion::V2_2) {
		//iwd2
		for (int i=0;i<ISCLASSES;i++) {
			if (classid==classesiwd2[i]) {
				return GetClassLevel(i);
			}
		}
		return 0;
	}

	if (classid >= BGCLASSCNT) {
		classid=0;
	}
	//other, levelslotsbg starts at 0 classid
	return GetClassLevel(levelslotsbg[classid]);
}

//lowlevel internal function, isclass is NOT the class id, but an internal index
ieDword Actor::GetClassLevel(const ieDword isClass) const
{
	if (isClass >= ISCLASSES)
		return 0;

	//return iwd2 value if appropriate
	if (creVersion == CREVersion::V2_2)
		return BaseStats[levelslotsiwd2[isClass]];

	//only works with PC's
	ieDword classID = BaseStats[IE_CLASS] - 1;
	if (!HasPlayerClass()) return 0;

	// handle barbarians specially, since they're kits and not in levelStats
	if ((isClass == ISBARBARIAN) && levelStats[classID][ISFIGHTER] && (BaseStats[IE_KIT] == KIT_BARBARIAN)) {
		return BaseStats[IE_LEVEL];
	}

	// get the level stat (IE_LEVEL,*2,*3)
	ieDword levelStat = levelStats[classID][isClass];
	if (!levelStat) return 0;

	//do dual-swap
	if (IsDualClassed()) {
		//if the old class is inactive, and it is the class
		//being searched for, return 0
		if (IsDualInactive() && ((Modified[IE_MC_FLAGS] & MC_WAS_ANY) == (ieDword) mcwasflags[isClass]))
			return 0;
	}
	return BaseStats[levelStat];
}

bool Actor::IsDualInactive() const
{
	if (!IsDualClassed()) return false;

	//we assume the old class is in IE_LEVEL2, unless swapped
	ieDword oldlevel = IsDualSwap() ? BaseStats[IE_LEVEL] : BaseStats[IE_LEVEL2];

	//since GetXPLevel returns the average of the 2 levels, oldclasslevel will
	//only be less than GetXPLevel when the new class surpasses it
	return oldlevel>=GetXPLevel(false);
}

bool Actor::IsDualSwap() const
{
	//the dualswap[class-1] holds the info
	if (!IsDualClassed()) return false;
	ieDword tmpclass = BaseStats[IE_CLASS]-1;
	if (!HasPlayerClass()) return false;
	return (ieDword) dualSwap[tmpclass] == (Modified[IE_MC_FLAGS] & MC_WAS_ANY);
}

ieDword Actor::GetWarriorLevel() const
{
	ieDword warriorlevels[4] = {
		GetBarbarianLevel(),
		GetFighterLevel(),
		GetPaladinLevel(),
		GetRangerLevel()
	};

	ieDword highest = 0;
	for (unsigned int warriorLevel : warriorlevels) {
		if (warriorLevel > highest) {
			highest = warriorLevel;
		}
	}

	return highest;
}

bool Actor::BlocksSearchMap() const
{
	return Modified[IE_DONOTJUMP] < DNJ_UNHINDERED &&
		!(InternalFlags & (IF_REALLYDIED | IF_JUSTDIED)) &&
		!Modified[IE_AVATARREMOVAL];
}

//return true if the actor doesn't want to use an entrance
bool Actor::CannotPassEntrance(ieDword exitID) const
{
	if (LastExit!=exitID) {
		return true;
	}

	if (InternalFlags & IF_PST_WMAPPING) {
		return true;
	}

	if (InternalFlags&IF_USEEXIT) {
		return false;
	}

	return true;
}

void Actor::UseExit(ieDword exitID) {
	if (exitID) {
		InternalFlags|=IF_USEEXIT;
	} else {
		InternalFlags&=~IF_USEEXIT;
		LastArea = Area;
		UsedExit.Reset();
		if (LastExit) {
			const Scriptable *ip = area->GetInfoPointByGlobalID(LastExit);
			if (ip) {
				const ieVariable& ipName = ip->GetScriptName();
				if (!ipName.IsEmpty()) {
					UsedExit = ipName;
				}
			}
		}
	}
	LastExit = exitID;
}

// luck increases the minimum roll per dice, but only up to the number of dice sides;
// luck does not affect critical hit chances:
// if critical is set, it will return 1/sides on a critical, otherwise it can never
// return a critical miss when luck is positive and can return a false critical hit
// Callees with LR_CRITICAL should check if the result matches 1 or size*dice.
int Actor::LuckyRoll(int dice, int size, int add, ieDword flags, const Actor* opponent) const
{
	assert(this != opponent);

	int luck;

	luck = (signed) GetSafeStat(IE_LUCK);

	//damageluck is additive with regular luck (used for maximized damage, righteous magic)
	if (flags&LR_DAMAGELUCK) {
		luck += (signed) GetSafeStat(IE_DAMAGELUCK);
	}

	//it is always the opponent's luck that decrease damage (or anything)
	if (opponent) luck -= opponent->GetSafeStat(IE_LUCK);

	if (flags&LR_NEGATIVE) {
		luck = -luck;
	}

	if (dice < 1 || size < 1) {
		return (add + luck > 1 ? add + luck : 1);
	}

	ieDword critical = flags&LR_CRITICAL;

	if (dice > 100) {
		int bonus;
		if (abs(luck) > size) {
			bonus = luck/abs(luck) * size;
		} else {
			bonus = luck;
		}
		int roll = RAND(1, dice * size);
		if (critical && (roll == 1 || roll == size)) {
			return roll;
		} else {
			return add + dice * (size + bonus) / 2;
		}
	}

	int roll, result = 0, misses = 0, hits = 0;
	for (int i = 0; i < dice; i++) {
		roll = RAND(1, size);
		if (roll == 1) {
			misses++;
		} else if (roll == size) {
			hits++;
		}
		roll += luck;
		if (roll > size) {
			roll = size;
		} else if (roll < 1) {
			roll = 1;
		}
		result += roll;
	}

	// ensure we can still return a critical failure/success
	if (critical && dice == misses) return 1;
	if (critical && dice == hits) return size*dice;

	// hack for critical mode, so overbearing luck does not cause a critical hit
	// FIXME: decouple the result from the critical info
	if (critical && result+add >= size*dice) {
		return size*dice - 1;
	} else {
		return result + add;
	}
}

// removes the (normal) invisibility state
void Actor::CureInvisibility()
{
	if (Modified[IE_STATE_ID] & state_invisible) {
		Effect *newfx;

		newfx = EffectQueue::CreateEffect(fx_remove_invisible_state_ref, 0, 0, FX_DURATION_INSTANT_PERMANENT);
		core->ApplyEffect(newfx, this, this);

		//not sure, but better than nothing
		if (! (Modified[IE_STATE_ID]&state_invisible)) {
			AddTrigger(TriggerEntry(trigger_becamevisible));
		}
	}
}

// removes the sanctuary effect
void Actor::CureSanctuary()
{
	// clear the overlay immediately
	pcf_sanctuary(this, Modified[IE_SANCTUARY], Modified[IE_SANCTUARY] & ~(1<<OV_SANCTUARY));

	Effect *newfx;
	newfx = EffectQueue::CreateEffect(fx_remove_sanctuary_ref, 0, 0, FX_DURATION_INSTANT_PERMANENT);
	core->ApplyEffect(newfx, this, this);
}

void Actor::ResetState()
{
	CureInvisibility();
	CureSanctuary();
	SetModal(Modal::None);
	ResetCommentTime();
}

// doesn't check the range, but only that the azimuth and the target
// orientation match with a +/-2 allowed difference
bool Actor::IsBehind(const Actor* target) const
{
	orient_t tarOrient = target->GetOrientation();
	// computed, since we don't care where we face
	orient_t myOrient = GetOrient(target->Pos, Pos);

	for (int i = -2; i <= 2; i++) {
		orient_t side = NextOrientation(myOrient, i);
		if (side == tarOrient) return true;
	}
	return false;
}

// checks all the actor's stats to see if the target is her racial enemy
int Actor::GetRacialEnemyBonus(const Actor *target) const
{
	if (!target) {
		return 0;
	}

	if (third) {
		int level = GetRangerLevel();
		if (Modified[IE_HATEDRACE] == target->Modified[IE_RACE]) {
			return (level+4)/5;
		}
		// iwd2 supports multiple racial enemies gained through level progression
		for (unsigned int i=0; i<7; i++) {
			if (Modified[IE_HATEDRACE2+i] == target->Modified[IE_RACE]) {
				return (level+4)/5-i-1;
			}
		}
		return 0;
	}
	if (Modified[IE_HATEDRACE] == target->Modified[IE_RACE]) {
		return 4;
	}
	return 0;
}

bool Actor::ModalSpellSkillCheck()
{
	switch(Modal.State) {
		case Modal::BattleSong:
			if (GetBardLevel()) {
				return !CheckSilenced();
			}
			return false;
		case Modal::DetectTraps:
			if (Modified[IE_TRAPS] <= 0) return false;
			return true;
		case Modal::TurnUndead:
			if (Modified[IE_TURNUNDEADLEVEL] <= 0) return false;
			return true;
		case Modal::Stealth:
			return TryToHide();
		case Modal::None:
		default:
			return false;
	}
}

inline void HideFailed(Actor* actor, int reason = -1, int skill = 0, int roll = 0, int targetDC = 0)
{
	Effect *newfx;
	newfx = EffectQueue::CreateEffect(fx_disable_button_ref, 0, ACT_STEALTH, FX_DURATION_INSTANT_LIMITED);
	newfx->Duration = core->Time.round_sec; // 90 ticks, 1 round
	core->ApplyEffect(newfx, actor, actor);

	if (!third) {
		return;
	}

	int bonus = actor->GetAbilityBonus(IE_DEX);
	switch (reason) {
		case 0:
			// ~Failed hide in shadows check! Hide in shadows check %d vs. D20 roll %d (%d Dexterity ability modifier)~
			displaymsg->DisplayRollStringName(ieStrRef::ROLL10, GUIColors::LIGHTGREY, actor, skill-bonus, roll, bonus);
			break;
		case 1:
			// ~Failed hide in shadows because you were seen by creature! Hide in Shadows check %d vs. creature's Level+Wisdom+Race modifier  %d + %d D20 Roll.~
			displaymsg->DisplayRollStringName(ieStrRef::ROLL8, GUIColors::LIGHTGREY, actor, skill, targetDC, roll);
			break;
		case 2:
			// ~Failed hide in shadows because you were heard by creature! Hide in Shadows check %d vs. creature's Level+Wisdom+Race modifier  %d + %d D20 Roll.~
			displaymsg->DisplayRollStringName(ieStrRef::ROLL7, GUIColors::LIGHTGREY, actor, skill, targetDC, roll);
			break;
		default:
			// no message
			break;
	}
}

//checks if we are seen, or seeing anyone
bool Actor::SeeAnyOne(bool enemy, bool seenby) const
{
	if (!area) return false;

	int flag = (seenby ? 0 : GA_NO_HIDDEN) | GA_NO_DEAD | GA_NO_UNSCHEDULED | GA_NO_SELF;
	if (enemy) {
		ieDword ea = GetSafeStat(IE_EA);
		if (ea>=EA_EVILCUTOFF) {
			flag|=GA_NO_ENEMY|GA_NO_NEUTRAL;
		} else if (ea<=EA_GOODCUTOFF) {
			flag|=GA_NO_ALLY|GA_NO_NEUTRAL;
		} else {
			return false; //neutrals got no enemy
		}
	}

	std::vector<Actor *> visActors = area->GetAllActorsInRadius(Pos, flag, seenby ? VOODOO_VISUAL_RANGE / 2 : GetSafeStat(IE_VISUALRANGE) / 2, this);
	bool seeEnemy = false;

	//we need to look harder if we look for seenby anyone
	for (const Actor *toCheck : visActors) {
		if (seenby) {
			if (WithinRange(toCheck, Pos, toCheck->GetStat(IE_VISUALRANGE) / 2)) {
				seeEnemy = true;
			}
		} else {
			seeEnemy = true;
		}
	}
	return seeEnemy;
}

bool Actor::TryToHide()
{
	if (Modified[IE_DISABLEDBUTTON] & (1<<ACT_STEALTH)) {
		HideFailed(this);
		return false;
	}

	// iwd2 is like the others only when trying to hide for the first time
	bool continuation = Modified[IE_STATE_ID] & state_invisible;
	if (third && continuation) {
		return TryToHideIWD2();
	}

	ieDword roll = 0;
	if (third) {
		roll = LuckyRoll(1, 20, GetArmorSkillPenalty(0));
	} else {
		roll = LuckyRoll(1, 100, GetArmorSkillPenalty(0));
		// critical failure
		if (roll == 1) {
			HideFailed(this);
			return false;
		}
	}

	// check for disabled dualclassed thieves (not sure if we need it)

	bool seen = SeeAnyOne(true, true);

	stat_t skill;
	if (core->HasFeature(GFFlags::HAS_HIDE_IN_SHADOWS)) {
		skill = (GetStat(IE_HIDEINSHADOWS) + GetStat(IE_STEALTH))/2;
	} else {
		skill = GetStat(IE_STEALTH);
	}

	if (seen) {
		HideFailed(this, 1);
	}

	if (third) {
		skill *= 7; // FIXME: temporary increase for the lightness percentage calculation
	}
	// TODO: figure out how iwd2 uses the area lightness and crelight.2da
	const Game *game = core->GetGame();
	// check how bright our spot is
	ieDword lightness = game->GetCurrentArea()->GetLightLevel(Pos);
	// seems to be the color overlay at midnight; lightness of a point with rgb (200, 100, 100)
	// TODO: but our NightTint computes to a higher value, which one is bad?
	ieDword light_diff = int((lightness - ref_lightness) * 100 / (100 - ref_lightness)) / 2;
	ieDword chance = (100 - light_diff) * skill/100;

	if (roll > chance) {
		HideFailed(this, 0, skill/7, roll);
		return false;
	}
	if (!continuation) VerbalConstant(Verbal::Hide);
	if (!third) return true;

	// ~Successful hide in shadows check! Hide in shadows check %d vs. D20 roll %d (%d Dexterity ability modifier)~
	displaymsg->DisplayRollStringName(ieStrRef::ROLL9, GUIColors::LIGHTGREY, this, skill/7, roll, GetAbilityBonus(IE_DEX));
	return true;
}

// skill check when trying to maintain invisibility: separate move silently and visibility check
bool Actor::TryToHideIWD2()
{
	int flags = GA_NO_DEAD | GA_NO_NEUTRAL | GA_NO_SELF | GA_NO_UNSCHEDULED;
	ieDword ea = GetSafeStat(IE_EA);
	if (ea >= EA_EVILCUTOFF) {
		flags |= GA_NO_ENEMY;
	} else if (ea <= EA_GOODCUTOFF) {
		flags |= GA_NO_ALLY;
	}
	std::vector<Actor *> neighbours = area->GetAllActorsInRadius(Pos, flags, Modified[IE_VISUALRANGE] / 2, this);
	ieDword roll = LuckyRoll(1, 20, GetArmorSkillPenalty(0));
	int targetDC = 0;

	// visibility check, you can try hiding while enemies are nearby
	ieDword skill = GetSkill(IE_HIDEINSHADOWS);
	for (const Actor *toCheck : neighbours) {
		if (toCheck->GetStat(IE_STATE_ID)&STATE_BLIND) {
			continue;
		}
		// we need to do an additional visual range check from the perspective of the observer
		if (!WithinRange(toCheck, Pos, toCheck->GetStat(IE_VISUALRANGE) / 2)) {
			continue;
		}
		// IE_CLASSLEVELSUM is set for all cres in iwd2 and use here was confirmed by RE
		// the third summand is a racial bonus (crehidemd.2da), but we use their search skill directly
		// the original actually multiplied the roll and DC by 5, making the check practically impossible to pass
		targetDC = toCheck->GetStat(IE_CLASSLEVELSUM) + toCheck->GetAbilityBonus(IE_WIS) + toCheck->GetStat(IE_SEARCH);
		bool seen = skill < roll + targetDC;
		if (seen) {
			HideFailed(this, 1, skill, roll, targetDC);
			return false;
		} else {
			// ~You were not seen by creature! Hide check %d vs. creature's Level+Wisdom+Race modifier  %d + %d D20 Roll.~
			displaymsg->DisplayRollStringName(ieStrRef::ROLL2, GUIColors::LIGHTGREY, this, skill, targetDC, roll);
		}
	}

	// we're stationary, so no need to check if we're making movement sounds
	if (!InMove()) {
		return true;
	}

	// separate move silently check
	skill = GetSkill(IE_STEALTH);
	for (const Actor *toCheck : neighbours) {
		if (toCheck->HasSpellState(SS_DEAF)) {
			continue;
		}
		// NOTE: pretending there is no hearing range, just as in the original

		// the third summand is a racial bonus from the nonexisting QUIETMOD column of crehidemd.2da,
		// but we're fair and use their search skill directly
		// the original actually multiplied the roll and DC by 5 and inverted the comparison, making the check practically impossible to pass
		targetDC = toCheck->GetStat(IE_CLASSLEVELSUM) + toCheck->GetAbilityBonus(IE_WIS) + toCheck->GetStat(IE_SEARCH);
		bool heard = skill < roll + targetDC;
		if (heard) {
			HideFailed(this, 2, skill, roll, targetDC);
			return false;
		} else {
			// ~You were not heard by creature! Move silently check %d vs. creature's Level+Wisdom+Race modifier  %d + %d D20 Roll.~
			displaymsg->DisplayRollStringName(ieStrRef::ROLL0, GUIColors::LIGHTGREY, this, skill, targetDC, roll);
		}
	}

	// NOTE: the original checked lightness for creatures that were both deaf and blind (or if nobody is around)
	// check TryToHide if it ever becomes important (crelight.2da)

	return true;
}

//cannot target actor (used by GUI)
bool Actor::Untargetable(const ResRef& spellRef) const
{
	if (!spellRef.IsEmpty()) {
		const Spell *spl = gamedata->GetSpell(spellRef, true);
		if (spl && (spl->Flags&SF_TARGETS_INVISIBLE)) {
			gamedata->FreeSpell(spl, spellRef, false);
			return false;
		}
		gamedata->FreeSpell(spl, spellRef, false);
	}
	return IsInvisibleTo(NULL);
}

//it is futile to try to harm target (used by AI scripts)
bool Actor::InvalidSpellTarget() const
{
	if (GetSafeStat(IE_STATE_ID) & STATE_DEAD) return true;
	if (HasSpellState(SS_SANCTUARY)) return true;
	return false;
}

bool Actor::InvalidSpellTarget(int spellnum, Actor *caster, int range) const
{
	ResRef spellRes;
	ResolveSpellName(spellRes, spellnum);

	//cheap substitute of the original hardcoded feature, returns true if already affected by the exact spell
	//no (spell)state checks based on every effect in the spell
	//FIXME: create a more compatible version if needed
	if (fxqueue.HasSource(spellRes)) return true;
	//return true if caster cannot cast
	if (!caster->CanCast(spellRes, false)) return true;

	if (!range) return false;

	int srange = GetSpellDistance(spellRes, caster, Pos);
	return srange<range;
}

int Actor::GetClassMask() const
{
	int classmask = 0;
	for (int i=0; i < ISCLASSES; i++) {
		if (Modified[levelslotsiwd2[i]] > 0) {
			classmask |= 1<<(classesiwd2[i]-1);
		}
	}

	return classmask;
}

int Actor::GetBookMask() const
{
	int bookmask = 0;
	for (int i=0; i < ISCLASSES; i++) {
		if (Modified[levelslotsiwd2[i]] > 0 && booksiwd2[i] >= 0) {
			bookmask |= 1 << booksiwd2[i];
		}
	}

	return bookmask;
}

// returns race for non-iwd2
unsigned int Actor::GetSubRace() const
{
	// race
	int lookup = Modified[IE_RACE];
	if (third) {
		// mangle with subrace if any
		int subrace = Modified[IE_SUBRACE];
		if (subrace) lookup = lookup<<16 | subrace;
	}
	return lookup;
}

// returns the combined dexterity and racial bonus to specified thieving skill
// column indices are 1-based, since 0 holds the rowname against which we do the lookup
int Actor::GetSkillBonus(unsigned int col) const
{
	if (skilldex.empty()) return 0;

	// race
	int lookup = GetSubRace();
	int bonus = 0;
	std::vector<std::vector<int> >::iterator it = skillrac.begin();
	// make sure we have a column, since the games have different amounts of thieving skills
	if (col < it->size()) {
		for ( ; it != skillrac.end(); it++) {
			if ((*it)[0] == lookup) {
				bonus = (*it)[col];
				break;
			}
		}
	}

	// dexterity
	lookup = Modified[IE_DEX];
	it = skilldex.begin();
	// make sure we have a column, since the games have different amounts of thieving skills
	if (col < it->size()) {
		for ( ; it != skilldex.end(); it++) {
			if ((*it)[0] == lookup) {
				bonus += (*it)[col];
				break;
			}
		}
	}
	return bonus;
}

bool Actor::IsPartyMember() const
{
	if (Modified[IE_EA]<=EA_FAMILIAR) return true;
	return InParty>0;
}

void Actor::ResetCommentTime()
{
	Game* game = core->GetGame();
	if (bored_time) {
		nextComment = game->GameTime + core->Roll(5, 1000, bored_time/2);
	} else {
		game->nextBored = 0;
		nextComment = game->GameTime + core->Roll(10, 500, 150);
	}
}

// this one is just a hack, so we can keep a bunch of other functions const
int Actor::GetArmorSkillPenalty(int profcheck) const
{
	int tmp1, tmp2;
	return GetArmorSkillPenalty(profcheck, tmp1, tmp2);
}

// Returns the armor check penalty.
// used for mapping the iwd2 armor feat to the equipped armor's weight class
// magical shields and armors get a +1 bonus
int Actor::GetArmorSkillPenalty(int profcheck, int &armor, int &shield) const
{
	if (!third) return 0;

	ieWord armorType = inventory.GetArmorItemType();
	int penalty = core->GetArmorPenalty(armorType);
	int weightClass = GetArmorWeightClass(armorType);

	// ignore the penalty if we are proficient
	if (profcheck && GetFeat(FEAT_ARMOUR_PROFICIENCY) >= weightClass) {
		penalty = 0;
	}
	bool magical = false;
	int armorSlot = Inventory::GetArmorSlot();
	const CREItem *armorItem = inventory.GetSlotItem(armorSlot);
	if (armorItem) {
		magical = armorItem->Flags&IE_INV_ITEM_MAGICAL;
	}
	if (magical) {
		penalty -= 1;
		if (penalty < 0) {
			penalty = 0;
		}
	}
	armor = penalty;

	// check also the shield penalty
	armorType = inventory.GetShieldItemType();
	int shieldPenalty = core->GetShieldPenalty(armorType);
	magical = false;
	armorSlot = inventory.GetShieldSlot();
	if (armorSlot != -1) { // there is a shield
		armorItem = inventory.GetSlotItem(armorSlot);
		if (armorItem) {
			magical = armorItem->Flags&IE_INV_ITEM_MAGICAL;
		}
	}
	if (magical) {
		shieldPenalty = std::max(0, shieldPenalty - 1);
	}
	if (profcheck && HasFeat(FEAT_SHIELD_PROF)) {
		shieldPenalty = 0;
	}
	penalty += shieldPenalty;
	shield = shieldPenalty;

	return -penalty;
}

// the armor weight class is perfectly deduced from the penalty as following:
// 0,   none: none, robes
// 1-3, light: leather, studded
// 4-6, medium: hide, chain, scale
// 7-,  heavy: splint, plate, full plate
// the values are taken from our dehardcoded itemdata.2da
int Actor::GetArmorWeightClass(ieWord armorType) const
{
	if (!third) return 0;

	int penalty = core->GetArmorPenalty(armorType);
	int weightClass = 0;

	if (penalty >= 1 && penalty < 4) {
		weightClass = 1;
	} else if (penalty >= 4 && penalty < 7) {
		weightClass = 2;
	} else if (penalty >= 7) {
		weightClass = 3;
	}
	return weightClass;
}

int Actor::GetTotalArmorFailure() const
{
	int armorfailure, shieldfailure;
	GetArmorFailure(armorfailure, shieldfailure);
	return armorfailure+shieldfailure;
}

int Actor::GetArmorFailure(int &armor, int &shield) const
{
	armor = shield = 0;
	if (!third) return 0;

	ieWord armorType = inventory.GetArmorItemType();
	int penalty = core->GetArmorFailure(armorType);
	armor = penalty;

	// check also the shield penalty
	armorType = inventory.GetShieldItemType();
	int shieldPenalty = core->GetShieldPenalty(armorType);
	penalty += shieldPenalty;
	shield = shieldPenalty;

	return -penalty;
}

// checks whether the actor is visible to another scriptable
bool Actor::IsInvisibleTo(const Scriptable *checker) const
{
	// consider underground ankhegs completely invisible to everyone
	if (GetStance() == IE_ANI_WALK && GetAnims()->GetAnimType() == IE_ANI_TWO_PIECE) {
		return true;
	}

	bool canSeeInvisibles = false;
	const Actor* checker2 = Scriptable::As<Actor>(checker);
	if (checker2) {
		canSeeInvisibles = checker2->GetSafeStat(IE_SEEINVISIBLE);
	}
	bool invisible = GetSafeStat(IE_STATE_ID) & state_invisible;
	if (!canSeeInvisibles && (invisible || HasSpellState(SS_SANCTUARY))) {
		return true;
	}

	return false;
}

int Actor::UpdateAnimationID(bool derived)
{
	// the base animation id
	int AnimID = avBase;
	int StatID = derived?GetSafeStat(IE_ANIMATION_ID):avBase;
	if (AnimID<0 || StatID<AnimID || StatID>AnimID+0x1000) return 1; //no change
	if (!InParty) return 1; //too many bugs caused by buggy game data, we change only PCs

	// tables for additive modifiers of the animation id (race, gender, class)
	for (const auto& av : avPrefix) {
		const AutoTable tm = av.avtable;
		if (!tm) {
			return -3;
		}
		StatID = av.stat;
		StatID = derived?GetSafeStat(StatID):GetBase( StatID );

		AnimID += tm->QueryFieldSigned<int>(StatID, 0);
	}
	if (BaseStats[IE_ANIMATION_ID] != (stat_t) AnimID) {
		SetBase(IE_ANIMATION_ID, (stat_t) AnimID);
	}
	if (!derived) {
		SetAnimationID(AnimID);
		//setting PST's starting stance to 18
		if (avStance !=-1) {
			SetStance( avStance );
		}
	}
	return 0;
}

void Actor::MovementCommand(std::string command)
{
	UseExit(0);
	Stop();
	AddAction(GenerateAction(std::move(command)));
	ProcessActions();
}

bool Actor::HasVisibleHP() const
{
	// sucks but this is set in different places
	if (!pstflags && GetStat(IE_MC_FLAGS) & MC_HIDE_HP) return false;
	if (HasSpellState(SS_NOHPINFO)) return false;
	if (GetStat(IE_EXTSTATE_ID) & EXTSTATE_NO_HP) return false;
	return true;
}

// shows hp/maxhp as overhead text
void Actor::DisplayHeadHPRatio()
{
	if (!HasVisibleHP()) return;

	overHead.SetText(fmt::format(u"{}/{}", Modified[IE_HITPOINTS], Modified[IE_MAXHITPOINTS]), true, false);
}

void Actor::ReleaseCurrentAction()
{
	disarmTrap = -1;
	Scriptable::ReleaseCurrentAction();
}

// concentration is annoying: besides special cases, every caster should
// check if there's an enemy nearby
bool Actor::ConcentrationCheck() const
{
	if (!third) return true;

	if (Modified[IE_SPECFLAGS]&SPECF_DRIVEN) return true;

	// anyone in a 5' radius?
	std::vector<Actor *> neighbours = area->GetAllActorsInRadius(Pos, GA_NO_DEAD|GA_NO_NEUTRAL|GA_NO_ALLY|GA_NO_SELF|GA_NO_UNSCHEDULED|GA_NO_HIDDEN, 5, this);
	if (neighbours.empty()) return true;

	// so there is someone out to get us and we should do the real concentration check
	int roll = LuckyRoll(1, 20, 0);
	int concentration = GetSkill(IE_CONCENTRATION);
	int bonus = 0;
	if (HasFeat(FEAT_COMBAT_CASTING)) {
		bonus += 4;
	}

	const Spell* spl = gamedata->GetSpell(SpellResRef, true);
	if (!spl) return true;
	int spellLevel = spl->SpellLevel;
	gamedata->FreeSpell(spl, SpellResRef, false);

	if (roll + concentration + bonus < 15 + spellLevel) {
		if (InParty) {
			displaymsg->DisplayRollStringName(ieStrRef::ROLL4, GUIColors::LIGHTGREY, this, roll + concentration, 15 + spellLevel, bonus);
		} else {
			displaymsg->DisplayRollStringName(ieStrRef::ROLL5, GUIColors::LIGHTGREY, this);
		}
		return false;
	} else {
		if (InParty) {
			// ~Successful spell casting concentration check! Check roll %d vs. difficulty %d (%d bonus)~
			displaymsg->DisplayRollStringName(ieStrRef::ROLL3, GUIColors::LIGHTGREY, this, roll + concentration, 15 + spellLevel, bonus);
		}
	}
	return true;
}

// shorthand wrapper for throw-away effects
void Actor::ApplyEffectCopy(const Effect *oldfx, EffectRef &newref, Scriptable *Owner, ieDword param1, ieDword param2)
{
	Effect *newfx = EffectQueue::CreateEffectCopy(oldfx, newref, param1, param2);
	if (newfx) {
		newfx->ProbabilityRangeMin = 0;
		newfx->ProbabilityRangeMax = 100;
		newfx->MinAffectedLevel = 0;
		newfx->MaxAffectedLevel = 0;
		newfx->SavingThrowType = 0;
		newfx->Resistance = FX_NO_RESIST_CAN_DISPEL;
		core->ApplyEffect(newfx, this, Owner);
	} else {
		Log(ERROR, "Actor", "Failed to create effect copy for {}! Target: {}, Owner: {}", newref.Name, fmt::WideToChar{GetName()}, fmt::WideToChar{Owner->GetName()});
	}
}

// check if we were the passed class at some point
// NOTE: does not ignore disabled dual classes!
bool Actor::WasClass(ieDword oldClassID) const
{
	if (oldClassID >= BGCLASSCNT) return false;

	int mcWas = BaseStats[IE_MC_FLAGS] & MC_WAS_ANY;
	if (!mcWas) {
		// not dualclassed
		return false;
	}

	int OldIsClassID = levelslotsbg[oldClassID];
	return mcwasflags[OldIsClassID] == mcWas;
}

// returns effective class, accounting for possible inactive dual
ieDword Actor::GetActiveClass() const
{
	if (!IsDualInactive()) {
		// on load, Modified is not populated yet
		if (Modified[IE_CLASS] == 0) return BaseStats[IE_CLASS];
		return Modified[IE_CLASS];
	}

	int mcwas = Modified[IE_MC_FLAGS] & MC_WAS_ANY;
	int oldclass;
	for (int isClass = 0; isClass < ISCLASSES; isClass++) {
		oldclass = classesiwd2[isClass];
		if (mcwas == mcwasflags[isClass]) break;
	}
	if (!oldclass) {
		error("Actor", "Actor {} has incorrect MC_WAS flags ({:#x})!", fmt::WideToChar{GetName()}, mcwas);
	}

	int newclassmask = multiclass & ~(1 << (oldclass - 1));
	for (int newclass = 1, mask = 1; mask <= newclassmask; newclass++, mask <<= 1) {
		if (newclassmask == mask) return newclass;
	}

	// can be hit when starting a dual class
	Log(ERROR, "Actor", "Dual-classed actor {} (old class {}) has wrong multiclass bits ({}), using old class!", fmt::WideToChar{GetName()}, oldclass, multiclass);
	return oldclass;
}

// like IsDualInactive(), but accounts for the possibility of the active (second) class being kitted
bool Actor::IsKitInactive() const
{
	if (third) return false;
	if (!IsDualInactive()) return false;

	ieDword baseclass = GetActiveClass();
	ieDword kit = GetStat(IE_KIT);
	for (const auto& aKit : class2kits[baseclass].ids) {
		if (kit & aKit) return false;
	}
	return true;
}

// account for haste/slow affecting the metabolism (regeneration etc.)
// handled by AdjustedTicks in the original
tick_t Actor::GetAdjustedTime(tick_t time) const
{
	// haste mode 2 (walk speed) has no effect and we have to check the opcode indirectly
	// otherwise it wouldn't work if the haste/slow effect is later in the queue
	if (fxqueue.HasEffectWithParam(fx_set_haste_state_ref, 0) || fxqueue.HasEffectWithParam(fx_set_haste_state_ref, 1)) {
		time /= 2;
	} else if (fxqueue.HasEffect(fx_set_slow_state_ref)) {
		time *= 2;
	}
	return time;
}

ieDword Actor::GetClassID(const ieDword isClass) {
	return classesiwd2[isClass];
}

const std::string& Actor::GetClassName(ieDword classID) const
{
	return class2kits[classID].className;
}

// NOTE: returns first kit name for multikit chars
const std::string& Actor::GetKitName(ieDword kitID) const
{
	for (const auto& clsKitPair : class2kits) {
		int kitIdx = 0;
		for (const auto& kit : clsKitPair.second.ids) {
			if (kitID & kit) {
				return clsKitPair.second.kitNames[kitIdx];
			}
			kitIdx++;
		}
	}
	return blank;
}

void Actor::SetAnimatedTalking (tick_t length) {
	remainingTalkSoundTime = std::max(remainingTalkSoundTime, length);
	lastTalkTimeCheckAt = GetMilliseconds();
}

bool Actor::HasPlayerClass() const
{
	// no need to check for dual/multiclassing, since for that all used classes have to be player classes
	int cid = BaseStats[IE_CLASS];
	// IE_CLASS is >classcount for non-PCs/NPCs
	return cid > 0 && cid < classcount;
}

// this is part of a REd function from the original (see #124)
char Actor::GetArmorCode() const
{
	bool mageAnimation = (BaseStats[IE_ANIMATION_ID] & 0xF00) == 0x200;
	// IE_ARMOR_TYPE + 1 is the armor code, but we also need to look up robes specifically as they have 3 types :(
	const CREItem* itm = inventory.GetSlotItem(Inventory::GetArmorSlot());
	if (!itm) return '1';
	const Item *item = gamedata->GetItem(itm->ItemResRef, true);
	if (!item) return '1';
	bool wearingRobes = item->AnimationType[1] == 'W';

	if (mageAnimation ^ wearingRobes) return '1';
	return item->AnimationType[0];
}

ResRef Actor::GetArmorSound() const
{
	// Character has mage animation or is a non-mage wearing mage robes
	if ((BaseStats[IE_ANIMATION_ID] & 0xF00) == 0x200) return "";
	char armorCode = GetArmorCode();
	if (armorCode == '1') {
		return "";
	}

	ResRef sound;
	int maxChar = 6;
	if (armorCode == '4') maxChar = 8;
	if (IWDSound) {
		// all three iwds have this pattern: a_chain1-6, a_lthr1-6, a_plate1-8
		const char* suffixes = "12345678";
		int idx = RAND(0, maxChar-1);
		if (armorCode == '2') {
			sound.Format("A_LTHR{}", suffixes[idx]);
		} else if (armorCode == '3') {
			sound.Format("A_CHAIN{}", suffixes[idx]);
		} else { // 4
			sound.Format("A_PLATE{}", suffixes[idx]);
		}
	} else {
		// generate a 1 letter suffix or emulate an empty string
		// ARM_04G and ARM_04H exist, but couldn't be picked by the original function
		const char* suffixes = "abcdefgh";
		int idx = RAND(0, maxChar);
		char randomASCII = '\0';
		if (idx < maxChar) randomASCII = suffixes[idx];
		sound.Format("ARM_0{}{}", armorCode, randomASCII);
	}
	return sound;
}

void Actor::PlayArmorSound() const
{
	// don't try immediately upon loading
	if (!Ticks) return;
	if (Modified[IE_STATE_ID] & STATE_SILENCED) return;
	// peculiar original behaviour: always for pcs, while the rest only clank if footstep sounds are on
	if (!footsteps && !InParty) return;
	// pst is missing the resources
	if (pstflags) return;

	const Game *game = core->GetGame();
	if (!game) return;
	if (game->CombatCounter) return;

	const ResRef armorSound = GetArmorSound();
	if (!armorSound.IsEmpty()) {
		core->GetAudioDrv()->Play(armorSound, SFX_CHAN_ARMOR, Pos);
	}
}

bool Actor::ShouldModifyMorale() const
{
	// pst ignores it for pcs, treating it more like reputation
	if (pstflags) {
		return Modified[IE_EA] != EA_PC;
	}

	// in HoF, everyone else becomes immune to morale failure ("Mental Fortitude" in iwd2)
	if (core->GetGame()->HOFMode) {
		return Modified[IE_EA] == EA_PC;
	}

	return true;
}

const std::string& Actor::GetRaceName() const
{
	if (raceID2Name.count(BaseStats[IE_RACE])) {
		return raceID2Name[BaseStats[IE_RACE]];
	} else {
		return blank;
	}
}

}
