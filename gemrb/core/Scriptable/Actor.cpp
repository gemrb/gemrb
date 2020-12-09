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
#include "win32def.h"

#include "Bitmap.h"
#include "DataFileMgr.h"
#include "DialogHandler.h" // checking for dialog
#include "Game.h"
#include "GlobalTimer.h"
#include "DisplayMessage.h"
#include "GameData.h"
#include "Image.h"
#include "Item.h"
#include "PolymorphCache.h" // stupid polymorph cache hack
#include "Projectile.h"
#include "ProjectileServer.h"
#include "ScriptEngine.h"
#include "Spell.h"
#include "Sprite2D.h"
#include "TableMgr.h"
#include "Video.h"
#include "damages.h"
#include "GameScript/GSUtils.h" //needed for DisplayStringCore
#include "GameScript/GameScript.h"
#include "GUI/GameControl.h"
#include "RNG.h"
#include "Scriptable/InfoPoint.h"
#include "System/StringBuffer.h"
#include "StringMgr.h"

#include <cmath>

namespace GemRB {

//configurable?
ieDword ref_lightness = 43;

static const Color white = {
	0xff, 0xff, 0xff, 0xff
};
static const Color green = {
	0x00, 0xff, 0x00, 0xff
};
static const Color red = {
	0xff, 0x00, 0x00, 0xff
};
static const Color yellow = {
	0xff, 0xff, 0x00, 0xff
};
static const Color cyan = {
	0x00, 0xff, 0xff, 0xff
};
static const Color magenta = {
	0xff, 0x00, 0xff, 0xff
};

static int sharexp = SX_DIVIDE|SX_COMBAT;
static int classcount = -1;
static int extraslots = -1;
static char **clericspelltables = NULL;
static char **druidspelltables = NULL;
static char **wizardspelltables = NULL;
static int *turnlevels = NULL;
static int *booktypes = NULL;
static int *xpbonus = NULL;
static int *xpcap = NULL;
static int *defaultprof = NULL;
static int *castingstat = NULL;
static int *iwd2spltypes = NULL;
static int xpbonustypes = -1;
static int xpbonuslevels = -1;
static int **levelslots = NULL;
static int *dualswap = NULL;
static int *multi = NULL;
static int *maxLevelForHpRoll = NULL;
static std::map<int, std::vector<int> > skillstats;
static std::map<int, int> stat2skill;
static int **afcomments = NULL;
static int afcount = -1;
static ieVariable CounterNames[4]={"GOOD","LAW","LADY","MURDER"};
//I keep the zero index the same as core rules (default setting)
static int dmgadjustments[6]={0, -50, -25, 0, 50, 100}; //default, easy, normal, core rules, hard, nightmare
//XP adjustments on easy setting (need research on the amount)
//Seems like bg1 halves xp, bg2 doesn't have any impact
static int xpadjustments[6]={0, 0, 0, 0, 0, 0};
static int luckadjustments[6]={0, 0, 0, 0, 0, 0};

static int FistRows = -1;
static int *wmlevels[20];
typedef ieResRef FistResType[MAX_LEVEL+1];

static FistResType *fistres = NULL;
static int *fistresclass = NULL;
static ieResRef DefaultFist = {"FIST"};

//verbal constant specific data
static int VCMap[VCONST_COUNT];
static ieDword sel_snd_freq = 0;
static ieDword cmd_snd_freq = 0;
static ieDword crit_hit_scr_shake = 1;
static ieDword bored_time = 3000;
static ieDword footsteps = 1;
static ieDword always_dither = 1;
static ieDword war_cries = 1;
static ieDword GameDifficulty = DIFF_CORE;
static ieDword NoExtraDifficultyDmg = 0;
//the chance to issue one of the rare select verbal constants
#define RARE_SELECT_CHANCE 5
//these are the max number of select sounds -- the size of the pool to choose from
static int NUM_RARE_SELECT_SOUNDS = 2; //in bg and pst it is actually 4
#define NUM_SELECT_SOUNDS 6 //in bg1 this is 4 but doesn't need to be checked
#define NUM_MC_SELECT_SOUNDS 4 //number of main charater select sounds

#define MAX_FEATV 4294967295U // 1<<32-1 (used for the triple-stat feat handling)

//item usability array
struct ItemUseType {
	ieResRef table; //which table contains the stat usability flags
	ieByte stat;	//which actor stat we talk about
	ieByte mcol;	//which column should be matched against the stat
	ieByte vcol;	//which column has the bit value for it
	ieByte which;	//which item dword should be used (1 = kit)
};

static ieResRef featspells[ES_COUNT];
static ItemUseType *itemuse = NULL;
static int usecount = -1;
static bool pstflags = false;
static bool nocreate = false;
static bool third = false;
static bool raresnd = false;
static bool iwd2class = false;
//used in many places, but different in engines
static ieDword state_invisible = STATE_INVISIBLE;
static AutoTable extspeed;
static AutoTable wspecial;

//item animation override array
struct ItemAnimType {
	ieResRef itemname;
	ieByte animation;
};

static ItemAnimType *itemanim = NULL;
static int animcount = -1;

static int fiststat = IE_CLASS;

//conversion for 3rd ed
static int isclass[ISCLASSES]={0,0,0,0,0,0,0,0,0,0,0,0,0};

static const int mcwasflags[ISCLASSES] = {
	MC_WAS_FIGHTER, MC_WAS_MAGE, MC_WAS_THIEF, 0, 0, MC_WAS_CLERIC,
	MC_WAS_DRUID, 0, 0, MC_WAS_RANGER, 0, 0, 0};
static const char *isclassnames[ISCLASSES] = {
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
	std::vector<int> indices;
	std::vector<ieDword> ids;
	std::vector<char*> clabs;
	std::vector<char*> kitNames;
	char *clab;
	char *className;
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

static int wspattack_rows = 0;
static int wspattack_cols = 0;
static int **wspattack = NULL;

//holds the weapon style bonuses
#define STYLE_MAX 3
static int **wsdualwield = NULL;
static int **wstwohanded = NULL;
static int **wsswordshield = NULL;
static int **wssingle = NULL;

//unhardcoded monk bonuses
static int **monkbon = NULL;
static unsigned int monkbon_cols = 0;
static unsigned int monkbon_rows = 0;

// reputation modifiers
#define CLASS_PCCUTOFF 32
#define CLASS_INNOCENT 155
#define CLASS_FLAMINGFIST 156

static ActionButtonRow *GUIBTDefaults = NULL; //qslots row count
static ActionButtonRow2 *OtherGUIButtons = NULL;
ActionButtonRow DefaultButtons = {ACT_TALK, ACT_WEAPON1, ACT_WEAPON2,
	ACT_QSPELL1, ACT_QSPELL2, ACT_QSPELL3, ACT_CAST, ACT_USE, ACT_QSLOT1, ACT_QSLOT2,
	ACT_QSLOT3, ACT_INNATE};
static int QslotTranslation = false;
static int DeathOnZeroStat = true;
static int IWDSound = false;
static ieDword TranslucentShadows = 0;
static int ProjectileSize = 0; //the size of the projectile immunity bitfield (dwords)
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
static char csound[VCONST_COUNT];

static void InitActorTables();

#define DAMAGE_LEVELS 19
#define ATTACKROLL    20
#define SAVEROLL      20
#define DEFAULTAC     10

//TODO: externalise
#define TURN_PANIC_LVL_MOD 3
#define TURN_DEATH_LVL_MOD 7
#define REPUTATION_FALL 60

static ieResRef d_main[DAMAGE_LEVELS] = {
	//slot 0 is not used in the original engine
	"BLOODCR","BLOODS","BLOODM","BLOODL", //blood
	"SPFIRIMP","SPFIRIMP","SPFIRIMP",     //fire
	"SPSHKIMP","SPSHKIMP","SPSHKIMP",     //spark
	"SPFIRIMP","SPFIRIMP","SPFIRIMP",     //ice
	"SHACID","SHACID","SHACID",           //acid
	"SPDUSTY2","SPDUSTY2","SPDUSTY2"      //disintegrate
};
static ieResRef d_splash[DAMAGE_LEVELS] = {
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

static ieResRef hc_overlays[OVERLAY_COUNT]={"SANCTRY","SPENTACI","SPMAGGLO","SPSHIELD",
"GREASED","WEBENTD","MINORGLB","","","","","","","","","","","","","","",
"","","","SPTURNI2","SPTURNI","","","","","",""};
static ieDword hc_locations = 0;
static int hc_flags[OVERLAY_COUNT];
#define HC_INVISIBLE 1

static int *mxsplwis = NULL;
static int spllevels;

// thieving skill dexterity and race boni vectors
std::vector<std::vector<int> > skilldex;
std::vector<std::vector<int> > skillrac;

// subset of races.2da
std::map<unsigned int, int> favoredMap;
std::map<unsigned int, const char *> raceID2Name;

// iwd2 class to-hit and apr tables read into a single object
std::map<char *, std::vector<BABTable> > IWD2HitTable;
typedef std::map<char *, std::vector<BABTable> >::iterator IWD2HitTableIter;
std::map<int, char *> BABClassMap; // maps classis (not id!) to the BAB table

std::vector<ModalStatesStruct> ModalStates;
std::map<int, int> numWeaponSlots;

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
static EffectRef fx_damage_bonus_modifier_ref = { "DamageBonusModifier2", -1 };
static EffectRef fx_display_portrait_icon_ref = { "Icon:Display", -1 };
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
static EffectRef fx_remove_invisible_state_ref = { "ForceVisible", -1 };
static EffectRef fx_remove_sanctuary_ref = { "Cure:Sanctuary", -1 };
static EffectRef fx_disable_button_ref = { "DisableButton", -1 };
static EffectRef fx_damage_reduction_ref = { "DamageReduction", -1 };
static EffectRef fx_missile_damage_reduction_ref = { "MissileDamageReduction", -1 };
static EffectRef fx_smite_evil_ref = { "SmiteEvil", -1 };

//used by iwd2
static ieResRef resref_cripstr={"cripstr"};
static ieResRef resref_dirty={"dirty"};
static ieResRef resref_arterial={"artstr"};

static const int weapon_damagetype[] = {DAMAGE_CRUSHING, DAMAGE_PIERCING,
	DAMAGE_CRUSHING, DAMAGE_SLASHING, DAMAGE_MISSILE, DAMAGE_STUNNING};

//internal flags for calculating to hit
#define WEAPON_FIST        0
#define WEAPON_MELEE       1
#define WEAPON_RANGED      2
#define WEAPON_STYLEMASK   15
#define WEAPON_LEFTHAND    16
#define WEAPON_USESTRENGTH 32
#define WEAPON_USESTRENGTH_DMG 64
#define WEAPON_USESTRENGTH_HIT 128
#define WEAPON_FINESSE     256
#define WEAPON_BYPASS      0x10000
#define WEAPON_KEEN        0x20000

static int avBase, avStance;
struct avType {
	ieResRef avresref;
	AutoTable avtable;
	int stat;
};
static avType *avPrefix;
static int avCount = -1;

void ReleaseMemoryActor()
{
	if (mxsplwis) {
		//calloc'd x*y integer matrix
		free (mxsplwis);
		mxsplwis = NULL;
	}

	if (fistres) {
		delete [] fistres;
		fistres = NULL;
		delete [] fistresclass;
		fistresclass = NULL;
	}

	if (itemuse) {
		delete [] itemuse;
		itemuse = NULL;
	}

	if (itemanim) {
		delete [] itemanim;
		itemanim = NULL;
	}
	FistRows = -1;
	extspeed.release();
	wspecial.release();
}

Actor::Actor()
	: Movable( ST_ACTOR )
{
	int i;

	for (i = 0; i < MAX_STATS; i++) {
		BaseStats[i] = 0;
		Modified[i] = 0;
	}
	PrevStats = NULL;
	SmallPortrait[0] = 0;
	LargePortrait[0] = 0;

	anims = NULL;
	shadowAnimations = NULL;
	ShieldRef[0]=0;
	HelmetRef[0]=0;
	WeaponRef[0]=0;
	for (i = 0; i < EXTRA_ACTORCOVERS; ++i)
		extraCovers[i] = NULL;

	LongName = NULL;
	ShortName = NULL;
	LongStrRef = (ieStrRef) -1;
	ShortStrRef = (ieStrRef) -1;

	playedCommandSound = false;

	PCStats = NULL;
	LastDamage = 0;
	LastDamageType = 0;
	LastExit = 0;
	attackcount = 0;
	secondround = 0;
	//AttackStance = IE_ANI_ATTACK;
	attacksperround = 0;
	nextattack = 0;
	nextWalk = 0;
	lastattack = 0;
	InTrap = 0;
	ResetPathTries();
	TargetDoor = 0;
	attackProjectile = NULL;
	lastInit = 0;
	roundTime = 0;
	panicMode = PANIC_NONE;
	nextComment = 100 + RAND(0, 350); // 7-30s delay
	nextBored = 0;
	FatigueComplaintDelay = 0;

	inventory.SetInventoryType(INVENTORY_CREATURE);

	fxqueue.SetOwner( this );
	inventory.SetOwner( this );
	if (classcount<0) {
		//This block is executed only once, when the first actor is loaded
		InitActorTables();

		TranslucentShadows = 0;
		core->GetDictionary()->Lookup("Translucent Shadows", TranslucentShadows);
		//get the needed size to store projectile immunity bitflags in Dwords
		ProjectileSize = (core->GetProjectileServer()->GetHighestProjectileNumber()+31)/32;
		//allowing 1024 bits (1024 projectiles ought to be enough for everybody)
		//the rest of the projectiles would still work, but couldn't be resisted
		if (ProjectileSize>32) {
			ProjectileSize=32;
		}
	}
	multiclass = 0;
	projectileImmunity = (ieDword *) calloc(ProjectileSize,sizeof(ieDword));
	AppearanceFlags = 0;
	SetDeathVar = IncKillCount = UnknownField = 0;
	memset( DeathCounters, 0, sizeof(DeathCounters) );
	InParty = 0;
	TalkCount = 0;
	InteractCount = 0; //numtimesinteracted depends on this
	appearance = 0xffffff; //might be important for created creatures
	RemovalTime = ~0;
	Spawned = false;
	version = 0;
	//these are used only in iwd2 so we have to default them
	for(i=0;i<7;i++) {
		BaseStats[IE_HATEDRACE2+i]=0xff;
	}
	//this one is saved only for PC's
	Modal.State = MS_NONE;
	//set it to a neutral value
	Modal.Spell[0] = '*';
	Modal.LingeringSpell[0] = '*';
	Modal.LastApplyTime = 0;
	Modal.LingeringCount = 0;
	Modal.FirstApply = 1;
	BackstabResRef[0] = '*';
	//this one is not saved
	GotLUFeedback = false;
	RollSaves();
	WMLevelMod = 0;
	TicksLastRested = LastFatigueCheck = 0;
	remainingTalkSoundTime = lastTalkTimeCheckAt = 0;
	WeaponType = AttackStance = 0;
	DifficultyMargin = disarmTrap = 0;
	spellStates = (ieDword *) calloc(SpellStatesSize, sizeof(ieDword));
	weapSlotCount = 4;
	// delay all maxhp checks until we completely load all effects
	checkHP = 2;
	checkHPTime = 0;

	polymorphCache = NULL;
	memset(&wildSurgeMods, 0, sizeof(wildSurgeMods));
	AC.SetOwner(this);
	ToHit.SetOwner(this);
}

Actor::~Actor(void)
{
	unsigned int i;

	delete anims;

	core->FreeString( LongName );
	core->FreeString( ShortName );

	delete PCStats;

	for (i = 0; i < vvcOverlays.size(); i++) {
		if (vvcOverlays[i]) {
			delete vvcOverlays[i];
			vvcOverlays[i] = NULL;
		}
	}
	for (i = 0; i < vvcShields.size(); i++) {
		if (vvcShields[i]) {
			delete vvcShields[i];
			vvcShields[i] = NULL;
		}
	}
	for (i = 0; i < EXTRA_ACTORCOVERS; i++)
		delete extraCovers[i];

	delete attackProjectile;
	delete polymorphCache;

	free(projectileImmunity);
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

void Actor::SetName(const char* ptr, unsigned char type)
{
	size_t len = strlen( ptr ) + 1;
	//32 is the maximum possible length of the actor name in the original games
	if (len>32) len=33;
	if (type!=2) {
		LongName = ( char * ) realloc( LongName, len );
		memcpy( LongName, ptr, len );
		LongName[len-1]=0;
		core->StripLine( LongName, len );
	}
	if (type!=1) {
		ShortName = ( char * ) realloc( ShortName, len );
		memcpy( ShortName, ptr, len );
		ShortName[len-1]=0;
		core->StripLine( ShortName, len );
	}
}

void Actor::SetName(int strref, unsigned char type)
{
	if (type!=2) {
		if (LongName) free(LongName);
		LongName = core->GetCString( strref, IE_STR_REMOVE_NEWLINE );
		LongStrRef = strref;
	}
	if (type!=1) {
		if (ShortName) free(ShortName);
		ShortName = core->GetCString( strref, IE_STR_REMOVE_NEWLINE );
		ShortStrRef = strref;
	}
}

void Actor::SetAnimationID(unsigned int AnimID)
{
	//if the palette is locked, then it will be transferred to the new animation
	Palette *recover = NULL;
	ieResRef paletteResRef;

	if (anims) {
		if (anims->lockPalette) {
			recover = anims->palette[PAL_MAIN];
		}
		// Take ownership so the palette won't be deleted
		if (recover) {
			CopyResRef(paletteResRef, anims->PaletteResRef[PAL_MAIN]);
			if (recover->named) {
				recover = gamedata->GetPalette(paletteResRef);
			} else {
				recover->acquire();
			}
		}
		delete( anims );
	}
	//hacking PST no palette
	if (core->HasFeature(GF_ONE_BYTE_ANIMID) ) {
		if ((AnimID&0xf000)==0xe000) {
			if (BaseStats[IE_COLORCOUNT]) {
				Log(WARNING, "Actor", "Animation ID %x is supposed to be real colored (no recoloring), patched creature", AnimID);
			}
			BaseStats[IE_COLORCOUNT]=0;
		}
	}
	anims = new CharAnimations( AnimID&0xffff, BaseStats[IE_ARMOR_TYPE]);
	if(anims->ResRef[0] == 0) {
		delete anims;
		anims = NULL;
		Log(ERROR, "Actor", "Missing animation for %s", LongName);
		return;
	}
	anims->SetOffhandRef(ShieldRef);
	anims->SetHelmetRef(HelmetRef);
	anims->SetWeaponRef(WeaponRef);

	//if we have a recovery palette, then set it back
	assert(anims->palette[PAL_MAIN] == 0);
	anims->palette[PAL_MAIN] = recover;
	if (recover) {
		anims->lockPalette = true;
		CopyResRef(anims->PaletteResRef[PAL_MAIN], paletteResRef);
	}
	//bird animations are not hindered by searchmap
	//only animations with a space of 0 in avatars.2da files use this feature
	if (anims->GetCircleSize() != 0) {
		BaseStats[IE_DONOTJUMP]=0;
	} else {
		BaseStats[IE_DONOTJUMP]=DNJ_BIRD;
	}
	SetCircleSize();
	anims->SetColors(BaseStats+IE_COLORS);

	// PST and EE 2.0+ use an ini to define animation data, including walk and run speed
	// the rest had it hardcoded
	if (!core->HasFeature(GF_RESDATA_INI)) {
		// handle default speed and per-animation overrides
		int row = -1;
		if (extspeed.ok()) {
			char animHex[10];
			snprintf(animHex, 10, "0x%04X", AnimID);
			row = extspeed->FindTableValue((unsigned int) 0, animHex);
			if (row != -1) {
				int rate = atoi(extspeed->QueryField(row, 1));
				SetBase(IE_MOVEMENTRATE, rate);
			}
		} else {
			Log(MESSAGE, "Actor", "No moverate.2da found, using animation (0x%04X) for speed fallback!", AnimID);
		}
		if (row == -1) {
			Animation **anim = anims->GetAnimation(IE_ANI_WALK, 0);
			if (anim && anim[0]) {
				SetBase(IE_MOVEMENTRATE, anim[0]->GetFrameCount());
			} else {
				Log(WARNING, "Actor", "Unable to determine movement rate for animation 0x%04X!", AnimID);
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
ieDword Actor::GetStat(unsigned int StatIndex) const
{
	if (StatIndex >= MAX_STATS) {
		return 0xdadadada;
	}
	return Modified[StatIndex];
}

/** Always return a final stat value not partially calculated ones */
ieDword Actor::GetSafeStat(unsigned int StatIndex) const
{
	if (StatIndex >= MAX_STATS) {
		return 0xdadadada;
	}
	if (PrevStats) return PrevStats[StatIndex];
	return Modified[StatIndex];
}

void Actor::SetCircleSize()
{
	const Color *color;
	int color_index;

	if (!anims)
		return;

	const GameControl *gc = core->GetGameControl();
	float oscillationFactor = 1.0f;

	if (UnselectableTimer) {
		color = &magenta;
		color_index = 4;
	} else if (Modified[IE_STATE_ID] & STATE_PANIC) {
		color = &yellow;
		color_index = 5;
	} else if (Modified[IE_CHECKFORBERSERK]) {
		color = &yellow;
		color_index = 5;
	} else if (gc && (((gc->GetDialogueFlags()&DF_IN_DIALOG) && gc->dialoghandler->IsTarget(this)) || remainingTalkSoundTime > 0)) {
		color = &white;
		color_index = 3; //?? made up

		if (remainingTalkSoundTime > 0) {
			/**
			 * Approximation: pulsating at about 2Hz over a notable radius growth.
			 * Maybe check this relation for dragons and rats, too.
			 */
			oscillationFactor = 1.1f + std::sin(remainingTalkSoundTime * (4 * M_PI) / 1000) * 0.1f;
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
				color = &green;
				color_index = 0;
				break;
			case EA_EVILCUTOFF:
				color = &yellow;
				color_index = 5;
				break;
			case EA_ENEMY:
			case EA_GOODBUTRED:
			case EA_CHARMEDPC:
				color = &red;
				color_index = 1;
				break;
			default:
				color = &cyan;
				color_index = 2;
				break;
		}
	}

	// circle size 0 won't display, so we can ignore it when clamping
	int csize = Clamp(anims->GetCircleSize(), 1, MAX_CIRCLE_SIZE) - 1;

	SetCircle( anims->GetCircleSize(), oscillationFactor, *color, core->GroundCircles[csize][color_index], core->GroundCircles[csize][(color_index == 0) ? 3 : color_index] );
}

static void ApplyClab_internal(Actor *actor, const char *clab, int level, bool remove, int diff)
{
	AutoTable table(clab);
	if (!table) return;

	int row = table->GetRowCount();
	int maxLevel = level;
	// don't remove clabs from levels we haven't attained yet, just in case they contain non-sticky
	// permanent effects like the charisma degradation in the oozemaster
	if (remove) maxLevel -= diff;
	for(int i=0; i<maxLevel; i++) {
		for (int j=0; j<row; j++) {
			const char *res = table->QueryField(j,i);
			if (res[0]=='*') continue;

			if (!memcmp(res,"AP_",3)) {
				if (remove) {
					actor->fxqueue.RemoveAllEffects(res+3);
				} else {
					core->ApplySpell(res+3, actor, actor, 0);
				}
			} else if (!memcmp(res,"GA_",3)) {
				if (remove) {
					actor->spellbook.RemoveSpell(res+3);
				} else {
					actor->LearnSpell(res+3, LS_MEMO);
				}
			} else if (!memcmp(res,"FA_",3)) {//iwd2 only: innate name strref
				//memorize these?
				// we now learn them just to get the feedback string out
				if (remove) {
					actor->fxqueue.RemoveAllEffects(res+3);
				} else {
					actor->LearnSpell(res+3, LS_MEMO|LS_LEARN, IE_IWD2_SPELL_INNATE);
					actor->spellbook.RemoveSpell(res+3);
					core->ApplySpell(res+3, actor, actor, 0);
				}
			} else if (!memcmp(res,"FS_",3)) {//iwd2 only: song name strref (used by unused kits)
				//don't memorize these?
				if (remove) {
					actor->fxqueue.RemoveAllEffects(res+3);
				} else {
					actor->LearnSpell(res+3, LS_LEARN, IE_IWD2_SPELL_SONG);
					actor->spellbook.RemoveSpell(res+3);
					core->ApplySpell(res+3, actor, actor, 0);
				}
			} else if (!memcmp(res,"RA_",3)) {//iwd2 only
				//remove ability
				int x=atoi(res+3);
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
static int GetIWD2KitIndex (ieDword kit, ieDword baseclass=0, bool strict=false)
{
	if (!kit) return -1;

	if (baseclass != 0) {
		std::vector<ieDword> kits = class2kits[baseclass].ids;
		std::vector<ieDword>::iterator it = kits.begin();
		for (int idx=0; it != kits.end(); it++, idx++) {
			if (kit & (*it)) return class2kits[baseclass].indices[idx];
		}
		if (strict) return -1;
		Log(DEBUG, "Actor", "GetIWD2KitIndex: didn't find kit %d at expected class %d, recalculating!", kit, baseclass);
	}

	// no class info passed, so infer it
	std::map<int, ClassKits>::iterator clskit = class2kits.begin();
	for (int cidx=0; clskit != class2kits.end(); clskit++, cidx++) {
		std::vector<ieDword> kits = class2kits[cidx].ids;
		std::vector<ieDword>::iterator it = kits.begin();
		for (int kidx=0; it != kits.end(); it++, kidx++) {
			if (kit & (*it)) {
				return class2kits[cidx].indices[kidx];
			}
		}
	}

	Log(ERROR, "Actor", "GetIWD2KitIndex: didn't find kit %d for any class, ignoring!", kit);
	return -1;
}

ieDword Actor::GetKitIndex (ieDword kit, ieDword baseclass) const
{
	int kitindex = 0;

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
		if (kitindex < 0) {
			kitindex = 0;
		}
	}

	return (ieDword)kitindex;
}

//applies a kit on the character
bool Actor::ApplyKit(bool remove, ieDword baseclass, int diff)
{
	ieDword kit = GetStat(IE_KIT);
	ieDword kitclass = 0;
	int row = GetKitIndex(kit, baseclass);
	const char *clab = NULL;
	ieDword max = 0;
	ieDword cls = GetStat(IE_CLASS);
	Holder<TableMgr> tm;

	// iwd2 has support for multikit characters, so we have more work
	// at the same time each baseclass has its own level stat, so the logic is cleaner
	// NOTE: in iwd2 there are no pure class options for classes with kits, a kit has to be choosen
	// even generalist mages are a kit the same way as in the older games
	if (iwd2class) {
		// callers always pass a baseclass (only exception are actions not present in iwd2: addkit and addsuperkit)
		assert(baseclass != 0);
		row = GetIWD2KitIndex(kit, baseclass, true);
		bool kitMatchesClass = row > 0;

		if (!kit || !kitMatchesClass) {
			// pure class
			clab = class2kits[baseclass].clab;
		} else {
			// both kit and baseclass are fine and the kit is of this baseclass
			std::vector<ieDword> kits = class2kits[baseclass].ids;
			std::vector<ieDword>::iterator it = kits.begin();
			for (int idx=0; it != kits.end(); it++, idx++) {
				if (kit & (*it)) {
					clab = class2kits[baseclass].clabs[idx];
					break;
				}
			}
		}
		assert(clab != NULL);
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
			std::vector<int> kits = class2kits[cidx].indices;
			std::vector<int>::iterator it = kits.begin();
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
			Log(ERROR, "Actor", "ApplyKit: could not look up the requested kit (%d), skipping!", kit);
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

void Actor::ApplyClab(const char *clab, ieDword max, int remove, int diff)
{
	if (clab && clab[0]!='*') {
		if (max) {
			//singleclass
			if (remove != 2) {
				ApplyClab_internal(this, clab, max, true, diff);
			}
			if (remove != 1) {
				ApplyClab_internal(this, clab, max, false, 0);
			}
		}
	}
}

//call this when morale or moralebreak changed
//cannot use old or new value, because it is called two ways
static void pcf_morale (Actor *actor, ieDword /*oldValue*/, ieDword /*newValue*/)
{
	if (!actor->ShouldModifyMorale()) return;

	if ((actor->Modified[IE_MORALE]<=actor->Modified[IE_MORALEBREAK]) && (actor->Modified[IE_MORALEBREAK] != 0) ) {
		actor->Panic(core->GetGame()->GetActorByGlobalID(actor->LastAttacker), core->Roll(1,3,0) );
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
	if (!core->HasFeature(GF_HAPPINESS)) return;

	int newHappiness = GetHappiness(actor, core->GetGame()->Reputation);
	if (newHappiness == actor->PCStats->Happiness) return;

	actor->PCStats->Happiness = newHappiness;
	switch (newHappiness) {
		case -80: actor->VerbalConstant(VB_UNHAPPY, 1, DS_QUEUE); break;
		case -160: actor->VerbalConstant(VB_UNHAPPY_SERIOUS, 1, DS_QUEUE); break;
		case -300: actor->VerbalConstant(VB_BREAKING_POINT, 1, DS_QUEUE);
			if (actor != core->GetGame()->GetPC(0, false)) core->GetGame()->LeaveParty(actor);
			break;
		case 80: actor->VerbalConstant(VB_HAPPY, 1, DS_QUEUE); break;
		default: break; // case 0
	}
}

// make paladins and rangers fallen if the reputations drops enough
static void pcf_reputation(Actor *actor, ieDword oldValue, ieDword newValue)
{
	if (oldValue == newValue) return;
	if (actor->InParty && newValue <= REPUTATION_FALL) {
		if (actor->GetRangerLevel()) {
			GameScript::RemoveRangerHood(actor, NULL);
		} else if (actor->GetPaladinLevel()) {
			GameScript::RemovePaladinHood(actor, NULL);
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
		switch(booktypes[classIdx]) {
		case 2:
			// arcane sorcerer-style
			if (third) {
				sorcerer = 1 << iwd2spltypes[classIdx];
			} else {
				sorcerer = 1<<IE_SPELL_TYPE_WIZARD;
			}
			break;
		case 3:
			// divine caster with sorc. style spells
			if (third) {
				sorcerer = 1 << iwd2spltypes[classIdx];
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
		int eyeCount = 7;
		for (ieDword mask = EXTSTATE_EYE_MIND; mask <= EXTSTATE_EYE_STONE; mask <<= 1) {
			if (State & mask) --eyeCount;
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
			actor->VerbalConstant(VB_HURT, 1, DS_QUEUE);
		}
	}
	// don't fire off events if nothing changed, which can happen when called indirectly
	if (actor->BaseStats[IE_HITPOINTS] != hp || actor->Modified[IE_HITPOINTS] != hp) {
		actor->BaseStats[IE_HITPOINTS] = hp;
		actor->Modified[IE_HITPOINTS] = hp;
		if (actor->InParty) core->SetEventFlag(EF_PORTRAIT);
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
		if (DeathOnZeroStat) {
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
	pcf_hitpoint(actor, 0, actor->BaseStats[IE_HITPOINTS]);
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
	unsigned int pc = actor->InParty;
	if (pc && !actor->GotLUFeedback) {
		char varname[16];
		snprintf(varname, sizeof(varname), "CheckLevelUp%d", pc);
		core->GetGUIScriptEngine()->RunFunction("GUICommonWindows", "CheckLevelUp", true, pc);
		ieDword NeedsLevelUp = 0;
		core->GetDictionary()->Lookup(varname, NeedsLevelUp);
		if (NeedsLevelUp == 1) {
			displaymsg->DisplayConstantStringName(STR_LEVELUP, DMC_WHITE, actor);
			actor->GotLUFeedback = true;
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
	if (actor->FindOverlay(idx))
		return;
	ieDword flag = hc_locations&(1<<idx);
	ScriptedAnimation *sca = gamedata->GetScriptedAnimation(hc_overlays[idx], false);
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

	if (flag) {
		sca->ZPos=-1;
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
		actor->RemoveVVCell(hc_overlays[OV_ENTANGLE], true);
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
				actor->RemoveVVCell(hc_overlays[i], true);
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
		actor->RemoveVVCell(hc_overlays[OV_SHIELDGLOBE], true);
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
		actor->RemoveVVCell(hc_overlays[OV_MINORGLOBE], true);
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
		actor->RemoveVVCell(hc_overlays[OV_GREASE], true);
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
		actor->RemoveVVCell(hc_overlays[OV_WEB], true);
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
		//it seems we have to remove it abruptly
		actor->RemoveVVCell(hc_overlays[OV_BOUNCE], false);
	}
}

static void pcf_alignment(Actor *actor, ieDword /*oldValue*/, ieDword /*newValue*/)
{
	UpdateHappiness(actor);
}

static void pcf_avatarremoval(Actor *actor, ieDword /*oldValue*/, ieDword newValue)
{
	Map *map = actor->GetCurrentArea();
	if (!map) return;
	map->BlockSearchMap(actor->Pos, actor->size, newValue > 0 ? PATH_MAP_UNMARKED : PATH_MAP_NPC);
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
		anims->SetColors(actor->Modified+IE_COLORS);
	}
}

static void pcf_armorlevel(Actor *actor, ieDword /*oldValue*/, ieDword newValue)
{
	CharAnimations *anims = actor->GetAnims();
	if (anims) {
		anims->SetArmourLevel(newValue);
	}
}

static unsigned int maximum_values[MAX_STATS]={
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

typedef void (*PostChangeFunctionType)(Actor *actor, ieDword oldValue, ieDword newValue);
static PostChangeFunctionType post_change_functions[MAX_STATS]={
pcf_hitpoint, pcf_maxhitpoint, NULL, NULL, NULL, NULL, NULL, NULL,
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL, //0f
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL,
NULL,NULL,NULL,NULL, NULL, NULL, NULL, pcf_intoxication, //1f
NULL,NULL,pcf_level_fighter,NULL, pcf_stat_str, NULL, pcf_stat_int, pcf_stat_wis,
pcf_stat_dex,pcf_stat_con,pcf_stat_cha,NULL, pcf_xp, pcf_gold, pcf_morale, NULL, //2f
pcf_reputation,NULL,NULL,NULL, NULL, NULL, NULL, NULL,
NULL,NULL,NULL,NULL, NULL, NULL, pcf_entangle, pcf_sanctuary, //3f
pcf_minorglobe, pcf_shieldglobe, pcf_grease, pcf_web, pcf_level_mage, pcf_level_thief, NULL, NULL,
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL, //4f
NULL,NULL,NULL,pcf_minhitpoint, NULL, NULL, NULL, NULL,
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL, //5f
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL,
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL, //6f
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL,
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL, //7f
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL,
NULL,NULL,NULL,NULL, NULL, NULL, pcf_berserk, NULL, //8f
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL,
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL, //9f
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL,
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL, //af
NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL, //bf
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL,
NULL,pcf_avatarremoval,NULL,NULL, pcf_dbutton, pcf_animid,pcf_state, pcf_extstate, //cf
pcf_color,pcf_color,pcf_color,pcf_color, pcf_color, pcf_color, pcf_color, NULL,
NULL, pcf_alignment, pcf_dbutton, pcf_armorlevel, NULL, NULL, NULL, NULL, //df
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL,
pcf_class,NULL,pcf_ea,NULL, NULL, NULL, NULL, NULL, //ef
pcf_level_barbarian,pcf_level_bard,pcf_level_cleric,pcf_level_druid, pcf_level_monk, pcf_level_paladin, pcf_level_ranger, pcf_level_sorcerer,
NULL, NULL, NULL, NULL, pcf_morale, pcf_bounce, NULL, NULL //ff
};

/** call this from ~Interface() */
void Actor::ReleaseMemory()
{
	if (classcount>=0) {
		if (clericspelltables) {
			for (int i = 0; i < classcount; i++) {
				free (clericspelltables[i]);
			}
			free(clericspelltables);
			clericspelltables=NULL;
		}
		if (druidspelltables) {
			for (int i = 0; i < classcount; i++) {
				free (druidspelltables[i]);
			}
			free(druidspelltables);
			druidspelltables=NULL;
		}
		if (wizardspelltables) {
			for (int i = 0; i < classcount; i++) {
				free(wizardspelltables[i]);
			}
			free(wizardspelltables);
			wizardspelltables=NULL;
		}

		if (defaultprof) {
			free(defaultprof);
			defaultprof=NULL;
		}

		if (turnlevels) {
			free(turnlevels);
			turnlevels=NULL;
		}

		if (booktypes) {
			free(booktypes);
			booktypes=NULL;
		}

		if (castingstat) {
			free(castingstat);
			castingstat=NULL;
		}

		if (iwd2spltypes) {
			free(iwd2spltypes);
			iwd2spltypes = NULL;
		}

		if (xpbonus) {
			free(xpbonus);
			xpbonus=NULL;
			xpbonuslevels = -1;
			xpbonustypes = -1;
		}

		if (xpcap) {
			free(xpcap);
			xpcap = NULL;
		}

		if (levelslots) {
			for (int i = 0; i < classcount; i++) {
				free(levelslots[i]);
			}
			free(levelslots);
			levelslots=NULL;
		}
		if (dualswap) {
			free(dualswap);
			dualswap=NULL;
		}
		if (multi) {
			free(multi);
			multi=NULL;
		}
		if (maxLevelForHpRoll) {
			free(maxLevelForHpRoll);
			maxLevelForHpRoll=NULL;
		}
		skillstats.clear();

		if (afcomments) {
			for(int i = 0; i < afcount; i++) {
				free(afcomments[i]);
			}
			free(afcomments);
			afcomments=NULL;
		}

		if (wspattack) {
			for (int i = 0; i < wspattack_rows; i++) {
				free(wspattack[i]);
			}
			free(wspattack);
			wspattack=NULL;
		}
		if (wsdualwield) {
			for (int i = 0; i<= STYLE_MAX; i++) {
				free(wsdualwield[i]);
			}
			free(wsdualwield);
			wsdualwield=NULL;
		}
		if (wstwohanded) {
			for (int i = 0; i<= STYLE_MAX; i++) {
				free(wstwohanded[i]);
			}
			free(wstwohanded);
			wstwohanded=NULL;
		}
		if (wsswordshield) {
			for (int i = 0; i<= STYLE_MAX; i++) {
				free(wsswordshield[i]);
			}
			free(wsswordshield);
			wsswordshield=NULL;
		}
		if (wssingle) {
			for (int i = 0; i<= STYLE_MAX; i++) {
				free(wssingle[i]);
			}
			free(wssingle);
			wssingle=NULL;
		}
		if (monkbon) {
			for (unsigned i=0; i<monkbon_rows; i++) {
				free(monkbon[i]);
			}
			free(monkbon);
			monkbon=NULL;
		}
		for (auto wml : wmlevels) {
			free(wml);
			wml = NULL;
		}
		skilldex.clear();
		skillrac.clear();
		IWD2HitTable.clear();
		BABClassMap.clear();
		ModalStates.clear();
		for (auto clskit : class2kits) {
			free(clskit.second.clab);
			free(clskit.second.className);
			for (auto kit : clskit.second.clabs) {
				free(kit);
			}
			for (auto kit : clskit.second.kitNames) {
				free(kit);
			}
		}
	}
	if (GUIBTDefaults) {
		free (GUIBTDefaults);
		GUIBTDefaults=NULL;
	}
	if (OtherGUIButtons) {
		free (OtherGUIButtons);
	}
	classcount = -1;
}

#define COL_MAIN       0
#define COL_SPARKS     1
#define COL_GRADIENT   2

/* returns the ISCLASS for the class based on name */
static int IsClassFromName (const char* name)
{
	for (int i=0; i<ISCLASSES; i++) {
		if (strcmp(name, isclassnames[i]) == 0)
			return i;
	}
	return -1;
}

GEM_EXPORT void UpdateActorConfig()
{
	unsigned int tmp = 0;
	core->GetDictionary()->Lookup("Critical Hit Screen Shake", crit_hit_scr_shake);
	core->GetDictionary()->Lookup("Selection Sounds Frequency", sel_snd_freq);
	core->GetDictionary()->Lookup("Effect Text Level", tmp);
	core->SetFeedbackLevel(tmp);
	core->GetDictionary()->Lookup("Command Sounds Frequency", cmd_snd_freq);
	// only pst has the whole gamut for these two options
	if (!(tmp & FT_SELECTION)) sel_snd_freq = 0;
	if (!(tmp & FT_ACTIONS)) cmd_snd_freq = 0;
	core->GetDictionary()->Lookup("Bored Timeout", bored_time);
	core->GetDictionary()->Lookup("Footsteps", footsteps);
	//FIXME: Drop all actors' SpriteCover.
	//the actor will change dithering only after selected/moved (its spritecover was updated)
	core->GetDictionary()->Lookup("Always Dither", always_dither);
	core->GetDictionary()->Lookup("Attack Sounds", war_cries);

	//Handle Game Difficulty and Nightmare Mode
	GameDifficulty = 0;
	core->GetDictionary()->Lookup("Nightmare Mode", GameDifficulty);
	if (GameDifficulty) {
		GameDifficulty = DIFF_INSANE;
	} else {
		GameDifficulty = 0;
		core->GetDictionary()->Lookup("Difficulty Level", GameDifficulty);
		GameDifficulty++; // slider starts at 0, real levels at 1
	}
	GameDifficulty = Clamp((int) GameDifficulty, DIFF_EASY, DIFF_INSANE);

	// iwd has a config option for leniency
	core->GetDictionary()->Lookup("Suppress Extra Difficulty Damage", NoExtraDifficultyDmg);
}

static void ReadModalStates()
{
	AutoTable table("modal");
	if (!table) return;

	ModalStatesStruct ms;
	for (unsigned short i = 0; i < table->GetRowCount(); i++) {
		CopyResRef(ms.spell, table->QueryField(i, 0));
		strlcpy(ms.action, table->QueryField(i, 1), 16);
		ms.entering_str = atoi(table->QueryField(i, 2));
		ms.leaving_str = atoi(table->QueryField(i, 3));
		ms.failed_str = atoi(table->QueryField(i, 4));
		ms.aoe_spell = atoi(table->QueryField(i, 5));
		ms.repeat_msg = atoi(table->QueryField(i, 6));
		ModalStates.push_back(ms);
	}
}

static void InitActorTables()
{
	int i, j;

	UpdateActorConfig();
	pstflags = core->HasFeature(GF_PST_STATE_FLAGS) != 0;
	nocreate = core->HasFeature(GF_NO_NEW_VARIABLES) != 0;
	third = core->HasFeature(GF_3ED_RULES) != 0;
	raresnd = core->HasFeature(GF_RARE_ACTION_VB) != 0;
	iwd2class = core->HasFeature(GF_LEVELSLOT_PER_CLASS) != 0;
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

	if (core->HasFeature(GF_CHALLENGERATING)) {
		sharexp=SX_DIVIDE|SX_COMBAT|SX_CR;
	} else {
		sharexp=SX_DIVIDE|SX_COMBAT;
	}
	ReverseToHit = core->HasFeature(GF_REVERSE_TOHIT);
	CheckAbilities = core->HasFeature(GF_CHECK_ABILITIES);
	DeathOnZeroStat = core->HasFeature(GF_DEATH_ON_ZERO_STAT);
	IWDSound = core->HasFeature(GF_SOUNDS_INI);
	NUM_RARE_SELECT_SOUNDS = core->GetRareSelectSoundCount();

	//this table lists various level based xp bonuses
	AutoTable tm("xpbonus", true);
	if (tm) {
		xpbonustypes = tm->GetRowCount();
		if (xpbonustypes == 0) {
			xpbonuslevels = 0;
		} else {
			xpbonuslevels = tm->GetColumnCount(0);
			xpbonus = (int *) calloc(xpbonuslevels*xpbonustypes, sizeof(int));
			for (i = 0; i<xpbonustypes; i++) {
				for(j = 0; j<xpbonuslevels; j++) {
					xpbonus[i*xpbonuslevels+j] = atoi(tm->QueryField(i,j));
				}
			}
		}
	} else {
		xpbonustypes = 0;
		xpbonuslevels = 0;
	}
	//this table lists skill groups assigned to classes
	//it is theoretically possible to create hybrid classes
	tm.load("clskills");
	if (tm) {
		classcount = tm->GetRowCount();
		memset (isclass,0,sizeof(isclass));
		clericspelltables = (char **) calloc(classcount, sizeof(char*));
		druidspelltables = (char **) calloc(classcount, sizeof(char*));
		wizardspelltables = (char **) calloc(classcount, sizeof(char*));
		turnlevels = (int *) calloc(classcount, sizeof(int));
		booktypes = (int *) calloc(classcount, sizeof(int));
		defaultprof = (int *) calloc(classcount, sizeof(int));
		castingstat = (int *) calloc(classcount, sizeof(int));
		iwd2spltypes = (int *) calloc(classcount, sizeof(int));

		ieDword bitmask = 1;

		for(i = 0; i<classcount; i++) {
			const char *field;
			const char *rowname = tm->GetRowName(i);

			field = tm->QueryField(rowname, "DRUIDSPELL");
			if (field[0]!='*') {
				isclass[ISDRUID] |= bitmask;
				druidspelltables[i]=strdup(field);
			}
			field = tm->QueryField(rowname, "CLERICSPELL");
			if (field[0]!='*') {
				// iwd2 has no DRUIDSPELL
				if (third && !strnicmp(field, "MXSPLDRD", 8)) {
					isclass[ISDRUID] |= bitmask;
					druidspelltables[i]=strdup(field);
				} else {
					isclass[ISCLERIC] |= bitmask;
					clericspelltables[i]=strdup(field);
				}
			}

			field = tm->QueryField(rowname, "MAGESPELL");
			if (field[0]!='*') {
				isclass[ISMAGE] |= bitmask;
				wizardspelltables[i]=strdup(field);
			}

			// field 3 holds the starting xp

			field = tm->QueryField(rowname, "BARDSKILL");
			if (field[0]!='*') {
				isclass[ISBARD] |= bitmask;
			}

			field = tm->QueryField(rowname, "THIEFSKILL");
			if (field[0]!='*') {
				isclass[ISTHIEF] |= bitmask;
			}

			field = tm->QueryField(rowname, "LAYHANDS");
			if (field[0]!='*') {
				isclass[ISPALADIN] |= bitmask;
			}

			field = tm->QueryField(rowname, "TURNLEVEL");
			turnlevels[i]=atoi(field);

			field = tm->QueryField(rowname, "BOOKTYPE");
			booktypes[i]=atoi(field);
			//if booktype == 3 then it is a 'divine sorcerer' class
			//we shouldn't hardcode iwd2 classes this heavily
			if (booktypes[i]==2) {
				isclass[ISSORCERER] |= bitmask;
			}

			if (third) {
				field = tm->QueryField(rowname, "CASTING"); // COL_HATERACE but different name
				castingstat[i] = atoi(field);

				field = tm->QueryField(rowname, "SPLTYPE");
				iwd2spltypes[i] = atoi(field);
			}

			field = tm->QueryField(rowname, "HATERACE");
			if (field[0]!='*') {
				isclass[ISRANGER] |= bitmask;
			}

			field = tm->QueryField(rowname, "ABILITIES");
			if (!strnicmp(field, "CLABMO", 6)) {
				isclass[ISMONK] |= bitmask;
			}
			// everyone but pst (none at all) and iwd2 (different table)
			class2kits[i].clab = strdup(field);
			class2kits[i].className = strdup(rowname);

			field = tm->QueryField(rowname, "NO_PROF");
			defaultprof[i]=atoi(field);

			bitmask <<=1;
		}
	} else {
		classcount = 0; //well
	}

	i = core->GetMaximumAbility();
	maximum_values[IE_STR]=i;
	maximum_values[IE_INT]=i;
	maximum_values[IE_DEX]=i;
	maximum_values[IE_CON]=i;
	maximum_values[IE_CHR]=i;
	maximum_values[IE_WIS]=i;
	if (ReverseToHit) {
		//all games except iwd2
		maximum_values[IE_ARMORCLASS]=20;
	} else {
		//iwd2
		maximum_values[IE_ARMORCLASS]=199;
	}

	//initializing the vvc resource references
	tm.load("damage");
	if (tm) {
		for (i=0;i<DAMAGE_LEVELS;i++) {
			const char *tmp = tm->QueryField( i, COL_MAIN );
			strnlwrcpy(d_main[i], tmp, 8);
			if (d_main[i][0]=='*') {
				d_main[i][0]=0;
			}
			tmp = tm->QueryField( i, COL_SPARKS );
			strnlwrcpy(d_splash[i], tmp, 8);
			if (d_splash[i][0]=='*') {
				d_splash[i][0]=0;
			}
			tmp = tm->QueryField( i, COL_GRADIENT );
			d_gradient[i]=atoi(tmp);
		}
	}

	tm.load("overlay");
	if (tm) {
		ieDword mask = 1;
		for (i=0;i<OVERLAY_COUNT;i++) {
			const char *tmp = tm->QueryField( i, 0 );
			strnlwrcpy(hc_overlays[i], tmp, 8);
			if (atoi(tm->QueryField( i, 1))) {
				hc_locations|=mask;
			}
			tmp = tm->QueryField( i, 2 );
			hc_flags[i] = atoi(tmp);
			mask<<=1;
		}
	}

	//csound for bg1/bg2
	memset(csound,0,sizeof(csound));
	if (!core->HasFeature(GF_SOUNDFOLDERS)) {
		tm.load("csound");
		if (tm) {
			for(i=0;i<VCONST_COUNT;i++) {
				const char *tmp = tm->QueryField( i, 0 );
				switch(tmp[0]) {
					case '*': break;
					//I have no idea what this ! mean
					case '!': csound[i]=tmp[1]; break;
					default: csound[i]=tmp[0]; break;
				}
			}
		}
	}

	tm.load("qslots");
	GUIBTDefaults = (ActionButtonRow *) calloc( classcount+1,sizeof(ActionButtonRow) );

	//leave room for default row at 0
	for (i = 0; i <= classcount; i++) {
		memcpy(GUIBTDefaults+i, &DefaultButtons, sizeof(ActionButtonRow));
		if (tm && i) {
			for (int j=0;j<MAX_QSLOTS;j++) {
				GUIBTDefaults[i][j+3]=(ieByte) atoi( tm->QueryField(i-1,j) );
			}
		}
	}

	tm.load("qslot2", true);
	if (tm) {
		extraslots = tm->GetRowCount();
		OtherGUIButtons = (ActionButtonRow2 *) calloc( extraslots, sizeof (ActionButtonRow2) );

		for (i=0; i<extraslots; i++) {
			long tmp = 0;
			valid_number( tm->QueryField(i,0), tmp );
			OtherGUIButtons[i].clss = (ieByte) tmp;
			memcpy(OtherGUIButtons[i].buttons, &DefaultButtons, sizeof(ActionButtonRow));
			for (int j=0;j<GUIBT_COUNT;j++) {
				OtherGUIButtons[i].buttons[j]=(ieByte) atoi( tm->QueryField(i,j+1) );
			}
		}
	}

	tm.load("mdfeats", true);
	if (tm) {
		for (i=0; i<ES_COUNT; i++) {
			strnuprcpy(featspells[i], tm->QueryField(i,0), sizeof(ieResRef)-1 );
		}
	}

	tm.load("itemuse");
	if (tm) {
		usecount = tm->GetRowCount();
		itemuse = new ItemUseType[usecount];
		for (i = 0; i < usecount; i++) {
			itemuse[i].stat = (ieByte) core->TranslateStat( tm->QueryField(i,0) );
			strnlwrcpy(itemuse[i].table, tm->QueryField(i,1),8 );
			itemuse[i].mcol = (ieByte) atoi( tm->QueryField(i,2) );
			itemuse[i].vcol = (ieByte) atoi( tm->QueryField(i,3) );
			itemuse[i].which = (ieByte) atoi( tm->QueryField(i,4) );
			//limiting it to 0 or 1 to avoid crashes
			if (itemuse[i].which!=1) {
				itemuse[i].which=0;
			}
		}
	}

	tm.load("itemanim", true);
	if (tm) {
		animcount = tm->GetRowCount();
		itemanim = new ItemAnimType[animcount];
		for (i = 0; i < animcount; i++) {
			strnlwrcpy(itemanim[i].itemname, tm->QueryField(i,0),8 );
			itemanim[i].animation = (ieByte) atoi( tm->QueryField(i,1) );
		}
	}

	// iwd2 has mxsplbon instead, since all casters get a bonus with high enough stats (which are not always wisdom)
	// luckily, they both use the same format
	if (third) {
		tm.load("mxsplbon");
	} else {
		tm.load("mxsplwis");
	}
	if (tm) {
		spllevels = tm->GetColumnCount(0);
		int max = core->GetMaximumAbility();
		mxsplwis = (int *) calloc(max*spllevels, sizeof(int));
		for (i = 0; i < spllevels; i++) {
			for(int j = 0; j < max; j++) {
				int k = atoi(tm->GetRowName(j))-1;
				if (k>=0 && k<max) {
					mxsplwis[k*spllevels+i]=atoi(tm->QueryField(j,i));
				}
			}
		}
	}

	tm.load("featreq", true);
	if (tm) {
		unsigned int stat, max;

		for(i=0;i<MAX_FEATS;i++) {
			//we need the MULTIPLE and MAX_LEVEL columns
			//MULTIPLE: the FEAT_* stat index
			//MAX_LEVEL: how many times it could be taken
			stat = core->TranslateStat(tm->QueryField(i,0));
			if (stat>=MAX_STATS) {
				Log(WARNING, "Actor", "Invalid stat value in featreq.2da");
			}
			max = atoi(tm->QueryField(i,1));
			//boolean feats can only be taken once, the code requires featmax for them too
			if (stat && (max<1)) max=1;
			featstats[i] = (ieByte) stat;
			featmax[i] = (ieByte) max;
		}
	}

	maxLevelForHpRoll = (int *) calloc(classcount, sizeof(int));
	xpcap = (int *) calloc(classcount, sizeof(int));
	AutoTable xpcapt("xpcap");
	std::map<std::string, int> className2ID;

	tm.load("classes");
	if (!tm) {
		error("Actor", "Missing classes.2da!");
	}
	if (iwd2class) {
		// we need to set up much less here due to a saner class/level system in 3ed
		Log(MESSAGE, "Actor", "Examining IWD2-style classes.2da");
		AutoTable tht;
		for (i=0; i<(int)tm->GetRowCount(); i++) {
			const char *classname = tm->GetRowName(i);
			int classis = IsClassFromName(classname);
			ieDword classID = atoi(tm->QueryField(classname, "ID"));
			ieDword classcol = atoi(tm->QueryField(classname, "CLASS")); // only real classes have this column at 0
			const char *clab = tm->QueryField(classname, "CLAB");
			if (classcol) {
				// kit ids are in hex
				classID = strtoul(tm->QueryField(classname, "ID"), NULL, 16);
				class2kits[classcol].indices.push_back(i);
				class2kits[classcol].ids.push_back(classID);
				class2kits[classcol].clabs.push_back(strdup(clab));
				class2kits[classcol].kitNames.push_back(strdup(classname));
				continue;
			} else if (i < classcount) {
				// populate classesiwd2
				// we need the id of the isclass name, not the current one
				ieDword cid = atoi(tm->QueryField(isclassnames[i], "ID"));
				classesiwd2[i] = cid;

				class2kits[classID].clab = strdup(clab);
			} else {
				// new class out of order
				Log(FATAL, "Actor", "New classes should precede any kits in classes.2da! Aborting ...");
			}

			xpcap[classis] = atoi(xpcapt->QueryField(classname, "VALUE"));

			// set up the tohit/apr tables
			char tohit[9];
			strnuprcpy(tohit, tm->QueryField(classname, "TOHIT"), 8);
			BABClassMap[classis] = strdup(tohit);
			// the tables repeat, but we need to only load one copy
			// FIXME: the attempt at skipping doesn't work!
			IWD2HitTableIter it = IWD2HitTable.find(tohit);
			if (it == IWD2HitTable.end()) {
				tht.load(tohit, true);
				if (!tht || !tohit[0]) {
					error("Actor", "TOHIT table for %s does not exist!", classname);
				}
				ieDword row;
				BABTable bt;
				std::vector<BABTable> btv;
				btv.reserve(tht->GetRowCount());
				for (row = 0; row < tht->GetRowCount(); row++) {
					bt.level = atoi(tht->GetRowName(row));
					bt.bab = atoi(tht->QueryField(row, 0));
					bt.apr = atoi(tht->QueryField(row, 1));
					btv.push_back(bt);
				}
				IWD2HitTable.insert(std::make_pair (BABClassMap[classis], btv));
			}

			StringBuffer buffer;
			buffer.appendFormatted("\tID: %d, ", classID);
			buffer.appendFormatted("Name: %s, ", classname);
			buffer.appendFormatted("Classis: %d, ", classis);
			buffer.appendFormatted("ToHit: %s ", tohit);
			buffer.appendFormatted("XPCap: %d", xpcap[classis]);

			Log(DEBUG, "Actor", buffer);
		}
	} else {
		AutoTable hptm;
		//iwd2 just uses levelslotsiwd2 instead
		Log(MESSAGE, "Actor", "Examining classes.2da");

		//when searching the levelslots, you must search for
		//levelslots[BaseStats[IE_CLASS]-1] as there is no class id of 0
		levelslots = (int **) calloc(classcount, sizeof(int*));
		dualswap = (int *) calloc(classcount, sizeof(int));
		multi = (int *) calloc(classcount, sizeof(int));
		ieDword tmpindex;

		for (i=0; i<classcount; i++) {
			const char* classname = tm->GetRowName(i);
			//make sure we have a valid classid, then decrement
			//it to get the correct array index
			tmpindex = atoi(tm->QueryField(classname, "ID"));
			if (!tmpindex)
				continue;
			className2ID[classname] = tmpindex;
			tmpindex--;

			StringBuffer buffer;
			buffer.appendFormatted("\tID: %d ", tmpindex);
			//only create the array if it isn't yet made
			//i.e. barbarians would overwrite fighters in bg2
			if (levelslots[tmpindex]) {
				buffer.appendFormatted("Already Found!");
				Log(DEBUG, "Actor", buffer);
				continue;
			}

			buffer.appendFormatted("Name: %s ", classname);

			xpcap[tmpindex] = atoi(xpcapt->QueryField(classname, "VALUE"));
			buffer.appendFormatted("XPCAP: %d ", xpcap[tmpindex]);

			int classis = 0;
			//default all levelslots to 0
			levelslots[tmpindex] = (int *) calloc(ISCLASSES, sizeof(int));

			//single classes only worry about IE_LEVEL
			long tmpclass = 0;
			valid_number(tm->QueryField(classname, "MULTI"), tmpclass);
			multi[tmpindex] = (ieDword) tmpclass;
			if (!tmpclass) {
				classis = IsClassFromName(classname);
				if (classis>=0) {
					//store the original class ID as iwd2 compatible ISCLASS (internal class number)
					classesiwd2[classis] = tmpindex+1;

					buffer.appendFormatted("Classis: %d ", classis);
					levelslots[tmpindex][classis] = IE_LEVEL;
					//get the last level when we can roll for HP
					hptm.load(tm->QueryField(classname, "HP"), true);
					if (hptm) {
						int tmphp = 0;
						int rollscolumn = hptm->GetColumnIndex("ROLLS");
						while (atoi(hptm->QueryField(tmphp, rollscolumn)))
							tmphp++;
						buffer.appendFormatted("HPROLLMAXLVL: %d", tmphp);
						if (tmphp) maxLevelForHpRoll[tmpindex] = tmphp;
					}
				}
				Log(DEBUG, "Actor", buffer);
				continue;
			}

			//we have to account for dual-swap in the multiclass field
			ieDword numfound = 1;
			ieDword tmpbits = core->CountBits (tmpclass);

			//we need all the classnames of the multi to compare with the order we load them in
			//because the original game set the levels based on name order, not bit order
			char **classnames = (char **) calloc(tmpbits, sizeof(char *));
			classnames[0] = strtok(strdup(classname), "_");
			while (numfound<tmpbits && (classnames[numfound] = strdup(strtok(NULL, "_")))) {
				numfound++;
			}
			numfound = 0;
			bool foundwarrior = false;
			for (j=0; j<classcount; j++) {
				//no sense continuing if we've found all to be found
				if (numfound==tmpbits)
					break;
				if ((1<<j)&tmpclass) {
					//save the IE_LEVEL information
					const char* currentname = tm->GetRowName((ieDword)(tm->FindTableValue("ID", j+1)));
					classis = IsClassFromName(currentname);
					if (classis>=0) {
						//search for the current class in the split of the names to get it's
						//correct order
						for (ieDword k=0; k<tmpbits; k++) {
							if (strcmp(classnames[k], currentname) == 0) {
								int tmplevel = 0;
								if (k==0) tmplevel = IE_LEVEL;
								else if (k==1) tmplevel = IE_LEVEL2;
								else tmplevel = IE_LEVEL3;
								levelslots[tmpindex][classis] = tmplevel;
							}
						}
						buffer.appendFormatted("Classis: %d ", classis);

						//warrior take precedence
						if (!foundwarrior) {
							foundwarrior = (classis==ISFIGHTER||classis==ISRANGER||classis==ISPALADIN||
								classis==ISBARBARIAN);
							hptm.load(tm->QueryField(currentname, "HP"), true);
							if (hptm) {
								int tmphp = 0;
								int rollscolumn = hptm->GetColumnIndex("ROLLS");
								while (atoi(hptm->QueryField(tmphp, rollscolumn)))
									tmphp++;
								//make sure we at least set the first class
								if ((tmphp>maxLevelForHpRoll[tmpindex])||foundwarrior||numfound==0)
									maxLevelForHpRoll[tmpindex]=tmphp;
							}
						}
					}

					//save the MC_WAS_ID of the first class in the dual-class
					if (numfound==0 && tmpbits==2) {
						if (strcmp(classnames[0], currentname) == 0) {
							dualswap[tmpindex] = strtol(tm->QueryField(currentname, "MC_WAS_ID"), NULL, 0);
						}
					} else if (numfound==1 && tmpbits==2 && !dualswap[tmpindex]) {
						dualswap[tmpindex] = strtol(tm->QueryField(currentname, "MC_WAS_ID"), NULL, 0);
					}
					numfound++;
				}
			}

			for (j=0; j<(signed)tmpbits; j++) {
				if (classnames[j]) {
					free(classnames[j]);
				}
			}
			free(classnames);

			buffer.appendFormatted("HPROLLMAXLVL: %d ", maxLevelForHpRoll[tmpindex]);
			buffer.appendFormatted("DS: %d ", dualswap[tmpindex]);
			buffer.appendFormatted("MULTI: %d", multi[tmpindex]);
			Log(DEBUG, "Actor", buffer);
		}
		/*this could be enabled to ensure all levelslots are filled with at least 0's;
		*however, the access code should ensure this never happens
		for (i=0; i<classcount; i++) {
			if (!levelslots[i]) {
				levelslots[i] = (int *) calloc(ISCLASSES, sizeof(int *));
			}
		}*/
	}
	Log(MESSAGE, "Actor", "Finished examining classes.2da");

	// set the default weapon slot count for the inventory gui — if we're not in iwd2 already
	if (!iwd2class) {
		tm.load("numwslot", true);
		if (tm) {
			int rowcount = tm->GetRowCount();
			for (i = 0; i < rowcount; i++) {
				const char* cls = tm->GetRowName(i);
				auto it = className2ID.find(cls);
				int id = 0;
				if (it != className2ID.end()) id = it->second;
				numWeaponSlots[id] = std::min(4, atoi(tm->QueryField(i, 0)));
			}
		}
	}
	className2ID.clear();

	// slurp up kitlist.2da; only iwd2 has both class and kit info in the same table
	if (!iwd2class) {
		tm.load("kitlist", true);
		if (!tm) {
			error("Actor", "Missing kitlist.2da!");
		}
		for (unsigned int i=0; i < tm->GetRowCount(); i++) {
			char rowName[6];
			snprintf(rowName, sizeof(rowName), "%d", i);
			// kit usability is in hex and is sometimes used as the kit ID,
			// while other times ID is the baseclass constant or-ed with the index
			ieDword kitUsability = strtoul(tm->QueryField(rowName, "UNUSABLE"), NULL, 16);
			int classID = atoi(tm->QueryField(rowName, "CLASS"));
			const char *clab = tm->QueryField(rowName, "ABILITIES");
			const char *kitName = tm->QueryField(rowName, "ROWNAME");
			class2kits[classID].indices.push_back(i);
			class2kits[classID].ids.push_back(kitUsability);
			class2kits[classID].clabs.push_back(strdup(clab));
			class2kits[classID].kitNames.push_back(strdup(kitName));
		}
	}

	//pre-cache hit/damage/speed bonuses for weapons
	wspecial.load("wspecial", true);

	//pre-cache attack per round bonuses
	tm.load("wspatck");
	if (tm) {
		wspattack_rows = tm->GetRowCount();
		wspattack_cols = tm->GetColumnCount();
		wspattack = (int **) calloc(wspattack_rows, sizeof(int *));

		int tmp = 0;
		for (i=0; i<wspattack_rows; i++) {
			wspattack[i] = (int *) calloc(wspattack_cols, sizeof(int));
			for (int j=0; j<wspattack_cols; j++) {
				tmp = atoi(tm->QueryField(i, j));
				//negative values relate to x/2, so we adjust them
				//positive values relate to x, so we must times by 2
				if (tmp<0) {
					tmp  = -2*tmp-1;
				}
				else {
					tmp *=  2;
				}
				wspattack[i][j] = tmp;
			}
		}
	}

	//dual-wielding table
	tm.load("wstwowpn", true);
	if (tm) {
		wsdualwield = (int **) calloc(STYLE_MAX+1, sizeof(int *));
		int cols = tm->GetColumnCount();
		for (i=0; i<=STYLE_MAX; i++) {
			wsdualwield[i] = (int *) calloc(cols, sizeof(int));
			for (int j=0; j<cols; j++) {
				wsdualwield[i][j] = atoi(tm->QueryField(i, j));
			}
		}
	}

	//two-handed table
	tm.load("wstwohnd", true);
	if (tm) {
		wstwohanded = (int **) calloc(STYLE_MAX+1, sizeof(int *));
		int cols = tm->GetColumnCount();
		for (i=0; i<=STYLE_MAX; i++) {
			wstwohanded[i] = (int *) calloc(cols, sizeof(int));
			for (int j=0; j<cols; j++) {
				wstwohanded[i][j] = atoi(tm->QueryField(i, j));
			}
		}
	}

	//shield table
	tm.load("wsshield", true);
	if (tm) {
		wsswordshield = (int **) calloc(STYLE_MAX+1, sizeof(int *));
		int cols = tm->GetColumnCount();
		for (i=0; i<=STYLE_MAX; i++) {
			wsswordshield[i] = (int *) calloc(cols, sizeof(int));
			for (int j=0; j<cols; j++) {
				wsswordshield[i][j] = atoi(tm->QueryField(i, j));
			}
		}
	}

	//single-handed table
	tm.load("wssingle");
	if (tm) {
		wssingle = (int **) calloc(STYLE_MAX+1, sizeof(int *));
		int cols = tm->GetColumnCount();
		for (i=0; i<=STYLE_MAX; i++) {
			wssingle[i] = (int *) calloc(cols, sizeof(int));
			for (int j=0; j<cols; j++) {
				wssingle[i][j] = atoi(tm->QueryField(i, j));
			}
		}
	}

	//unhardcoded monk bonus table
	tm.load("monkbon", true);
	if (tm) {
		monkbon_rows = tm->GetRowCount();
		monkbon_cols = tm->GetColumnCount();
		monkbon = (int **) calloc(monkbon_rows, sizeof(int *));
		for (unsigned i=0; i<monkbon_rows; i++) {
			monkbon[i] = (int *) calloc(monkbon_cols, sizeof(int));
			for (unsigned j=0; j<monkbon_cols; j++) {
				monkbon[i][j] = atoi(tm->QueryField(i, j));
			}
		}
	}

	//wild magic level modifiers
	for(i=0;i<20;i++) {
		wmlevels[i]=(int *) calloc(MAX_LEVEL,sizeof(int) );
	}
	tm.load("lvlmodwm", true);
	if (tm) {
		int maxrow = tm->GetRowCount();
		for (i=0;i<20;i++) {
			for(j=0;j<MAX_LEVEL;j++) {
				int row = maxrow;
				if (j<row) row=j;
				wmlevels[i][j]=strtol(tm->QueryField(row,i), NULL, 0);
			}
		}
	}

	// verbal constant remapping, if omitted, it is an 1-1 mapping
	// TODO: allow disabled VC slots
	for (i=0;i<VCONST_COUNT;i++) {
		VCMap[i]=i;
	}
	tm.load("vcremap");
	if (tm) {
		int rows = tm->GetRowCount();

		for (i=0;i<rows;i++) {
			int row = atoi(tm->QueryField(i,0));
			if (row<0 || row>=VCONST_COUNT) continue;
			int value = atoi(tm->QueryField(i,1));
			if (value<0 || value>=VCONST_COUNT) continue;
			VCMap[row]=value;
		}
	}

	//initializing the skill->stats conversion table (used in iwd2)
	tm.load("skillsta", true);
	if (tm) {
		int rowcount = tm->GetRowCount();
		int colcount = tm->GetColumnCount();
		for (i = 0; i < rowcount; i++) {
			skillstats[i] = std::vector<int>();
			int j, val;
			for(j = 0; j < colcount; j++) {
				// the stat and ability columns need conversion into numbers
				if (j < 2) {
					val = core->TranslateStat(tm->QueryField(i, j));
					if (j == 0) {
						stat2skill[val] = i;
					}
				} else {
					val = atoi(tm->QueryField(i, j));
				}
				skillstats[i].push_back (val);
			}
		}
	}

	//initializing area flag comments
	tm.load("comment");
	if (tm) {
		int rowcount = tm->GetRowCount();
		afcount = rowcount;
		if (rowcount) {
			afcomments = (int **) calloc(rowcount, sizeof(int *) );
			while(rowcount--) {
				afcomments[rowcount]=(int *) malloc(3*sizeof(int) );
				for(i=0;i<3;i++) {
					afcomments[rowcount][i] = strtol(tm->QueryField(rowcount,i), NULL, 0);
				}
			}
		}
	}

	// dexterity modifier for thieving skills
	tm.load("skilldex");
	if (tm) {
		int skilldexNCols = tm->GetColumnCount();
		int skilldexNRows = tm->GetRowCount();
		skilldex.reserve(skilldexNRows);

		for (i = 0; i < skilldexNRows; i++) {
			skilldex.push_back (std::vector<int>());
			skilldex[i].reserve(skilldexNCols+1);
			for(j = -1; j < skilldexNCols; j++) {
				if (j == -1) {
					skilldex[i].push_back (atoi(tm->GetRowName(i)));
				} else {
					skilldex[i].push_back (atoi(tm->QueryField(i, j)));
				}
			}
		}
	}

	// race modifier for thieving skills
	tm.load("skillrac");
	int value = 0;
	int racetable = core->LoadSymbol("race");
	int subracetable = core->LoadSymbol("subrace");
	Holder<SymbolMgr> race = NULL;
	Holder<SymbolMgr> subrace = NULL;
	if (racetable != -1) {
		race = core->GetSymbol(racetable);
	}
	if (subracetable != -1) {
		subrace = core->GetSymbol(subracetable);
	}
	if (tm) {
		int cols = tm->GetColumnCount();
		int rows = tm->GetRowCount();
		skillrac.reserve(rows);

		for (i = 0; i < rows; i++) {
			skillrac.push_back (std::vector<int>());
			skillrac[i].reserve(cols+1);
			for(j = -1; j < cols; j++) {
				if (j == -1) {
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
					skillrac[i].push_back (value);
				} else {
					skillrac[i].push_back (atoi(tm->QueryField(i, j)));
				}
			}
		}
	}

	//difficulty level based modifiers
	tm.load("difflvls");
	if (tm) {
		memset(xpadjustments, 0, sizeof(xpadjustments) );
		memset(dmgadjustments, 0, sizeof(dmgadjustments) );
		memset(luckadjustments, 0, sizeof(luckadjustments) );
		for (i=0; i<6; i++) {
			dmgadjustments[i] = atoi(tm->QueryField(0, i) );
			xpadjustments[i] = atoi(tm->QueryField(1, i) );
			luckadjustments[i] = atoi(tm->QueryField(2, i) );
		}
	}

	//preload stat derived animation tables
	tm.load("avprefix");
	delete [] avPrefix;
	avBase = 0;
	avCount = -1;
	if (tm) {
		int count = tm->GetRowCount();
		if (count> 0 && count<8) {
			avCount = count-1;
			avPrefix = new avType[count];
			avBase = strtoul(tm->QueryField(0),NULL, 0);
			const char *poi = tm->QueryField(0,1);
			if (*poi!='*') {
				avStance = strtoul(tm->QueryField(0,1),NULL, 0);
			} else {
				avStance = -1;
			}
			for (i=0;i<avCount;i++) {
				strnuprcpy(avPrefix[i].avresref, tm->QueryField(i+1), 8);
				avPrefix[i].avtable.load(avPrefix[i].avresref);
				if (avPrefix[i].avtable) {
					avPrefix[i].stat = core->TranslateStat(avPrefix[i].avtable->QueryField(0));
				} else {
					avPrefix[i].stat = -1;
				}
			}
		}
	}

	// races table
	tm.load("races");
	if (tm && !pstflags) {
		int racesNRows = tm->GetRowCount();

		for (i = 0; i < racesNRows; i++) {
			int raceID = strtol(tm->QueryField(i, 3), NULL, 0);
			int favClass = strtol(tm->QueryField(i, 8), NULL, 0);
			const char *raceName = tm->GetRowName(i);
			favoredMap.insert(std::make_pair(raceID, favClass));
			raceID2Name.insert(std::make_pair(raceID, raceName));
		}
	}

	// IWD, IWD2 and BG:EE have this
	int splstatetable = core->LoadSymbol("splstate");
	if (splstatetable != -1) {
		Holder<SymbolMgr> splstate = core->GetSymbol(splstatetable);
		int numstates = splstate->GetHighestValue();
		if (numstates > 0) {
			//rounding up
			// iwd1 has a practically empty ids though, so force a minimum
			SpellStatesSize = std::max(6, (numstates >> 5) + 1);
		}
	} else {
		SpellStatesSize = 6;
	}

	// movement rate adjustments
	extspeed.load("moverate", true);

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
	anims->SetColors(Modified+IE_COLORS);
}

void Actor::AddAnimation(const ieResRef resource, int gradient, int height, int flags)
{
	ScriptedAnimation *sca = gamedata->GetScriptedAnimation(resource, false);
	if (!sca)
		return;
	sca->ZPos=height;
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
int Actor::GetCriticalType() const
{
	long ret = 0;
	AutoTable tm("crits", true);
	if (!tm) return 0;
	//the ID of this PC (first 2 rows are empty)
	int row = BaseStats[IE_SPECIFIC];
	//defaults to 0
	valid_number(tm->QueryField(row, 1), ret);
	return (int) ret;
}

//Plays personal critical damage animation for PST PC's melee attacks
void Actor::PlayCritDamageAnimation(int type)
{
	AutoTable tm("crits");
	if (!tm) return;
	//the ID's are in column 1, selected by specifics by GetCriticalType
	int row = tm->FindTableValue (1, type);
	if (row>=0) {
		//the animations are listed in column 0
		AddAnimation(tm->QueryField(row, 0), -1, 45, AA_PLAYONCE|AA_BLEND);
	}
}

void Actor::PlayDamageAnimation(int type, bool hit)
{
	int i;
	int flags = AA_PLAYONCE;
	int height = 22;
	if (pstflags) {
		flags |= AA_BLEND;
		height = 45; // empirical like in fx_visual_spell_hit
	}

	Log(COMBAT, "Actor", "Damage animation type: %d", type);

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
			if(hit) {
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

ieDword Actor::ClampStat(unsigned int StatIndex, ieDword Value) const
{
	if (StatIndex < MAX_STATS) {
		if ((signed) Value < -100) {
			Value = (ieDword) -100;
		} else {
			if (maximum_values[StatIndex] > 0) {
				if ((signed)Value > 0 && Value > maximum_values[StatIndex]) {
					Value = maximum_values[StatIndex];
				}
			}
		}
	}
	return Value;
}

bool Actor::SetStat(unsigned int StatIndex, ieDword Value, int pcf)
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
			if (f) (*f)(this, previous, Value);
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
bool Actor::SetBase(unsigned int StatIndex, ieDword Value)
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

bool Actor::SetBaseNoPCF(unsigned int StatIndex, ieDword Value)
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

bool Actor::SetBaseBit(unsigned int StatIndex, ieDword Value, bool setreset)
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

const unsigned char *Actor::GetStateString() const
{
	if (!PCStats) {
		return NULL;
	}
	ieByte *tmp = PCStats->PortraitIconString;
	ieWord *Icons = PCStats->PortraitIcons;
	int j=0;
	for (int i=0;i<MAX_PORTRAIT_ICONS;i++) {
		if (!(Icons[i]&0xff00)) {
			tmp[j++]=(ieByte) ((Icons[i]&0xff)+66);
		}
	}
	tmp[j]=0;
	return tmp;
}

void Actor::AddPortraitIcon(ieByte icon)
{
	if (!PCStats) {
		return;
	}
	ieWord *Icons = PCStats->PortraitIcons;

	for(int i=0;i<MAX_PORTRAIT_ICONS;i++) {
		if (Icons[i]==0xffff) {
			Icons[i]=icon;
			return;
		}
		if (icon == (Icons[i]&0xff)) {
			return;
		}
	}
}

void Actor::DisablePortraitIcon(ieByte icon)
{
	if (!PCStats) {
		return;
	}
	ieWord *Icons = PCStats->PortraitIcons;
	int i;

	for(i=0;i<MAX_PORTRAIT_ICONS;i++) {
		if (icon == (Icons[i]&0xff)) {
			Icons[i]=0xff00|icon;
			return;
		}
	}
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


/** call this after load, to apply effects */
void Actor::RefreshEffects(EffectQueue *fx)
{
	ieDword previous[MAX_STATS];

	//put all special cleanup calls here
	CharAnimations* anims = GetAnims();
	if (anims) {
		anims->CheckColorMod();
	}
	spellbook.ClearBonus();
	memset(BardSong,0,sizeof(ieResRef));
	memset(projectileImmunity,0,ProjectileSize*sizeof(ieDword));

	//initialize base stats
	bool first = !(InternalFlags&IF_INITIALIZED);

	if (first) {
		InternalFlags|=IF_INITIALIZED;
		memcpy( previous, BaseStats, MAX_STATS * sizeof( ieDword ) );
	} else {
		memcpy( previous, Modified, MAX_STATS * sizeof( ieDword ) );
	}
	PrevStats = &previous[0];

	memcpy( Modified, BaseStats, MAX_STATS * sizeof( ieDword ) );
	if (PCStats) {
		memset( PCStats->PortraitIcons, -1, sizeof(PCStats->PortraitIcons) );
	}
	if (SpellStatesSize) {
		memset(spellStates, 0, sizeof(ieDword) * SpellStatesSize);
	}
	AC.ResetAll();
	ToHit.ResetAll(); // effects can result in the change of any of the boni, so we need to reset all

	if (fx) {
		fx->SetOwner(this);
		fx->AddAllEffects(this, Pos);
		delete fx;
		//copy back the original stats, because the effects
		//will be reapplied in ApplyAllEffects again
		memcpy( Modified, BaseStats, MAX_STATS * sizeof( ieDword ) );
		if (SpellStatesSize) {
			memset(spellStates, 0, sizeof(ieDword) * SpellStatesSize);
		}
		//also clear the spell bonuses just given, they will be
		//recalculated below again
		spellbook.ClearBonus();
		//AC.ResetAll(); // TODO: check if this is needed
		//ToHit.ResetAll();
	}

	unsigned int i;

	// some VVCs are controlled by stats (and so by PCFs), the rest have 'effect_owned' set
	for (i = 0; i < vvcOverlays.size(); i++) {
		if (vvcOverlays[i] && vvcOverlays[i]->effect_owned) vvcOverlays[i]->active = false;
	}
	for (i = 0; i < vvcShields.size(); i++) {
		if (vvcShields[i] && vvcShields[i]->effect_owned) vvcShields[i]->active = false;
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

	if (previous[IE_PUPPETID]) {
		CheckPuppet(core->GetGame()->GetActorByGlobalID(previous[IE_PUPPETID]), previous[IE_PUPPETTYPE]);
	}

	//move this further down if needed
	PrevStats = NULL;

	for (std::list<TriggerEntry>::iterator m = triggers.begin(); m != triggers.end (); m++) {
		m->flags |= TEF_PROCESSED_EFFECTS;

		// snap out of charm if the charmer hurt us
		if (m->triggerID == trigger_attackedby) {
			Actor *attacker = core->GetGame()->GetActorByGlobalID(LastAttacker);
			if (attacker) {
				int revertToEA = 0;
				if (Modified[IE_EA] == EA_CHARMED && attacker->GetStat(IE_EA) <= EA_GOODCUTOFF) {
					revertToEA = EA_ENEMY;
				} else if (Modified[IE_EA] == EA_CHARMEDPC && attacker->GetStat(IE_EA) >= EA_EVILCUTOFF) {
					revertToEA = EA_PC;
				}
				if (revertToEA) {
					// remove only the plain charm effect
					Effect *charmfx = fxqueue.HasEffectWithParam(fx_set_charmed_state_ref, 1);
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
		core->GetGame()->locals->Lookup("APPEARANCE", pst_appearance);
	}
	if (Modified[IE_SEX] != BaseStats[IE_SEX] && pst_appearance == 0) {
		UpdateAnimationID(true);
	}

	//delayed HP adjustment hack (after max HP modification)
	//as it's triggered by PCFs from the previous tick, it should probably run before current PCFs
	if (first && checkHP == 2) {
		//could not set this in the constructor
		checkHPTime = core->GetGame()->GameTime;
	} else if (checkHP && checkHPTime != core->GetGame()->GameTime) {
		checkHP = 0;
		pcf_hitpoint(this, 0, BaseStats[IE_HITPOINTS]);
	}

	for (i=0;i<MAX_STATS;i++) {
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
	if (mxsplwis) {
		if (spellbook.IsIWDSpellBook()) {
			// check each class separately for the casting stat and booktype (luckily there is no bonus for domain spells)
			for (i=0; i < ISCLASSES; i++) {
				int level = GetClassLevel(i);
				int booktype = booksiwd2[i]; // ieIWD2SpellType
				if (!level || booktype == -1) {
					continue;
				}
				level = Modified[castingstat[classesiwd2[i]]];
				if (level--) {
					spellbook.BonusSpells(booktype, spllevels, mxsplwis+spllevels*level);
				}
			}
		} else {
			int level = Modified[IE_WIS];
			if (level--) {
				spellbook.BonusSpells(IE_SPELL_TYPE_PRIEST, spllevels, mxsplwis+spllevels*level);
			}
		}
	}

	// iwd2 barbarian speed increase isn't handled like for monks (normal clab)!?
	if (third && GetBarbarianLevel()) {
		Modified[IE_MOVEMENTRATE] += 1;
	}

	// check if any new portrait icon was removed or added
	if (PCStats) {
		if (memcmp(PCStats->PreviousPortraitIcons, PCStats->PortraitIcons, sizeof(PCStats->PreviousPortraitIcons))) {
			core->SetEventFlag(EF_PORTRAIT);
			memcpy( PCStats->PreviousPortraitIcons, PCStats->PortraitIcons, sizeof(PCStats->PreviousPortraitIcons) );
		}
	}
	if (Immobile()) {
		timeStartStep = core->GetGame()->Ticks;
	}
}

int Actor::GetProficiency(int proftype) const
{
	switch(proftype) {
	case -2: //hand to hand old style
		return 1;
	case -1: //no proficiency
		return 0;
	default:
		//bg1 style proficiencies
		if(proftype>=0 && proftype<=IE_EXTRAPROFICIENCY20-IE_PROFICIENCYBASTARDSWORD) {
			return GetStat(IE_PROFICIENCYBASTARDSWORD+proftype);
		}

		//bg2 style proficiencies
		if (proftype>=IE_PROFICIENCYBASTARDSWORD && proftype<=IE_EXTRAPROFICIENCY20) {
			return GetStat(proftype);
		}
		return 0;
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
	if (!third) {
		if (bonlevel>maxLevelForHpRoll[bonindex]) {
			bonlevel = maxLevelForHpRoll[bonindex];
		}
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
			if (bonlevel+oldlevel > maxLevelForHpRoll[bonindex]) {
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

	if (third) {
		//toughness feat bonus (could be unhardcoded as a max hp bonus based on level if you want)
		bonus += Modified[IE_FEAT_TOUGHNESS]*3;
	}

	//we still apply the maximum bonus to dead characters, but don't apply
	//to current HP, or we'd have dead characters showing as having hp
	Modified[IE_MAXHITPOINTS]+=bonus;
	// applying the bonus to the current hitpoints is trickier, since we don't want to cause regeneration
	/* the following is not reliable, since the hp may become exactly oldmax via other means too
	ieDword oldmax = Modified[IE_MAXHITPOINTS];
	if (!(BaseStats[IE_STATE_ID]&STATE_DEAD)) {
		// for now only apply it to fully healed actors iff the bonus is positive (fixes starting hp)
		if (BaseStats[IE_HITPOINTS] == oldmax && bonus > 0) {
			BaseStats[IE_HITPOINTS] += bonus;
		}
	}*/
}

// refresh stats on creatures (PC or NPC) with a valid class (not animals etc)
// internal use only, and this is maybe a stupid name :)
void Actor::RefreshPCStats() {
	RefreshHP();

	Game *game = core->GetGame();
	//morale recovery every xth AI cycle ... except for pst pcs
	int mrec = GetStat(IE_MORALERECOVERYTIME);
	if (mrec && ShouldModifyMorale()) {
		if (!(game->GameTime%mrec)) {
			int morale = (signed) BaseStats[IE_MORALE];
			if (morale < 10) {
				NewBase(IE_MORALE, 1, MOD_ADDITIVE);
			} else if (morale > 10) {
				NewBase(IE_MORALE, (ieDword) -1, MOD_ADDITIVE);
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
	WeaponInfo wi;
	ITMExtHeader *header = GetWeapon(wi, false);
	ieDword stars;
	int dualwielding = IsDualWielding();
	stars = GetProficiency(wi.prof)&PROFS_MASK;

	//tenser's transformation makes the actor have at least proficient in any weapon
	if (!stars && HasSpellState(SS_TENSER)) stars = 1;

	if (header) {
		if (stars >= (unsigned)wspattack_rows) {
			stars = wspattack_rows-1;
		}

		int tmplevel = GetWarriorLevel();
		if (tmplevel >= wspattack_cols) {
			tmplevel = wspattack_cols-1;
		} else if (tmplevel < 0) {
			tmplevel = 0;
		}

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
			// FIXME: but this isn't universally true or improved haste couldn't double the total apr! For the above case, we're half apr off.
			if (tmplevel) {
				int mod = Modified[IE_NUMBEROFATTACKS] - BaseStats[IE_NUMBEROFATTACKS];
				BaseStats[IE_NUMBEROFATTACKS] = defaultattacks+wspattack[stars][tmplevel];
				if (GetAttackStyle() == WEAPON_RANGED) { // FIXME: should actually check if a set-apr opcode variant was used
					Modified[IE_NUMBEROFATTACKS] += wspattack[stars][tmplevel]; // no default
				} else {
					Modified[IE_NUMBEROFATTACKS] = BaseStats[IE_NUMBEROFATTACKS] + mod;
				}
			} else {
				SetBase(IE_NUMBEROFATTACKS, defaultattacks + wspattack[stars][0]);
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
	Modified[IE_LUCK] += luckadjustments[GameDifficulty - 1];

	// regenerate actors with high enough constitution
	int rate = GetConHealAmount();
	if (rate && !(game->GameTime % rate)) {
		if (core->HasFeature(GF_AREA_OVERRIDE) && game->GetPC(0, false) == this) {
			NewBase(IE_HITPOINTS, 1, MOD_ADDITIVE);
			// eeeh, no token (Heal: 1)
			if (Modified[IE_HITPOINTS] < Modified[IE_MAXHITPOINTS]) {
				String text = *core->GetString(28895) + L"1"; // FIXME
				if (core->HasFeedback(FT_STATES)) displaymsg->DisplayString(text, DMC_BG2XPGREEN, this);
			}
		} else{
			NewBase(IE_HITPOINTS, 1, MOD_ADDITIVE);
		}
	}

	// adjust thieving skills with dex and race
	// table header is in this order:
	// PICK_POCKETS  OPEN_LOCKS  FIND_TRAPS  MOVE_SILENTLY  HIDE_IN_SHADOWS  DETECT_ILLUSION  SET_TRAPS
	Modified[IE_PICKPOCKET] += GetSkillBonus(1);
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
	Game *game = core->GetGame();
	if (!game) return rate;

	if (core->HasFeature(GF_AREA_OVERRIDE) && game->GetPC(0, false) == this) {
		rate = core->GetConstitutionBonus(STAT_CON_TNO_REGEN, Modified[IE_CON]);
	} else {
		rate = core->GetConstitutionBonus(STAT_CON_HP_REGEN, Modified[IE_CON]);
		rate *= AI_UPDATE_TIME;
	}
	return rate;
}

// add fatigue every 4 hours since resting and check if the actor is penalised for it
void Actor::UpdateFatigue()
{
	Game *game = core->GetGame();
	if (!InParty || !game->GameTime) {
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

	if (!core->HasFeature(GF_AREA_OVERRIDE)) {
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
			VerbalConstant(VB_TIRED);
		}
	}
}

void Actor::RollSaves()
{
	if (InternalFlags&IF_USEDSAVE) {
		SavingThrow[0]=(ieByte) core->Roll(1, SAVEROLL, 0);
		SavingThrow[1]=(ieByte) core->Roll(1, SAVEROLL, 0);
		SavingThrow[2]=(ieByte) core->Roll(1, SAVEROLL, 0);
		SavingThrow[3]=(ieByte) core->Roll(1, SAVEROLL, 0);
		SavingThrow[4]=(ieByte) core->Roll(1, SAVEROLL, 0);
		InternalFlags&=~IF_USEDSAVE;
	}
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

#define SAVECOUNT 5
static int savingthrows[SAVECOUNT]={IE_SAVEVSSPELL, IE_SAVEVSBREATH, IE_SAVEVSDEATH, IE_SAVEVSWANDS, IE_SAVEVSPOLY};

/** returns true if actor made the save against saving throw type */
bool Actor::GetSavingThrow(ieDword type, int modifier, const Effect *fx)
{
	assert(type<SAVECOUNT);
	InternalFlags|=IF_USEDSAVE;
	int ret = SavingThrow[type];
	// NOTE: assuming criticals apply to iwd2 too
	if (ret == 1) return false;
	if (ret == SAVEROLL) return true;

	if (!third) {
		ret += modifier + GetStat(IE_LUCK);

		// potentially display feedback, but do some rate limiting, since each effect in a spell ends up here
		static ieDword prevType = -1;
		static int prevRoll = -1;
		static Actor *prevActor = NULL;
		if (core->HasFeedback(FT_COMBAT) && prevType != type && prevActor != this && prevRoll != ret) {
			// "Save Vs Death" in all games except pst: "Save Vs. Death:"
			String *str = core->GetString(displaymsg->GetStringReference(STR_SAVE_SPELL + type));
			wchar_t tmp[4];
			swprintf(tmp, sizeof(tmp)/sizeof(tmp[0]), L" %d", ret);
			String msg = *str + tmp;
			delete str;
			displaymsg->DisplayStringName(msg, DMC_WHITE, this);
		}
		prevType = type;
		prevActor = this;
		prevRoll = ret;
		return ret > (int) GetStat(savingthrows[type]);
	}

	int roll = ret;
	// NOTE: we use GetStat, assuming the stat save bonus can never be negated like some others
	int save = GetStat(savingthrows[type]);
	// intentionally not adding luck, which seems to have been handled separately
	// eg. 11hfamlk.itm uses an extra opcode for the saving throw bonus
	ret = roll + save + modifier;
	assert(fx);
	int spellLevel = fx->SpellLevel;
	int saveBonus = fx->SavingThrowBonus;
	int saveDC = 10 + spellLevel + saveBonus;

	// handle special bonuses (eg. vs poison, which doesn't have a separate stat any more)
	// same hardcoded list as in the original
	if (savingthrows[type] == IE_SAVEFORTITUDE && fx->Opcode == 25) {
		if (BaseStats[IE_RACE] == 4 /* DWARF */) ret += 2;
		if (HasFeat(FEAT_SNAKE_BLOOD)) ret += 2;
		if (HasFeat(FEAT_RESIST_POISON)) ret += 4;
	}

	// the original had a sourceType == TRIGGER check, but we handle more than ST_TRIGGER
	Scriptable *caster = area->GetScriptableByGlobalID(fx->CasterID);
	if (savingthrows[type] == IE_SAVEREFLEX && caster && caster->Type != ST_ACTOR) {
		// loop over all classes and add TRAPSAVE.2DA values to the bonus
		for (int cls = 0; cls < ISCLASSES; cls++) {
			int level = GetClassLevel(cls);
			if (!level) continue;
			ret += gamedata->GetTrapSaveBonus(level, classesiwd2[cls]);
		}
	}

	if (savingthrows[type] == IE_SAVEWILL) {
		// aura of courage
		if (Modified[IE_EA] < EA_GOODCUTOFF && stricmp(fx->Source, "SPWI420")) {
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
		if (caster && caster->Type == ST_ACTOR) {
			const Actor *cleric = (Actor *) caster;
			if (cleric->GetClericLevel() && BaseStats[IE_KIT] & 0x200000) saveDC += 1;
			// the original limited this to domain spells, but that's pretty lame
		}
	}

	// general bonuses
	// TODO: Heart of Fury upgraded creature get +5
	// FIXME: externalize these two two difflvls.2da
	if (Modified[IE_EA] != EA_PC && GameDifficulty == DIFF_EASY) ret -= 4;
	if (Modified[IE_EA] != EA_PC && GameDifficulty == DIFF_NORMAL) ret -= 2;
	// (half)elven resistance to enchantment, gnomish to illusions and dwarven to spells
	if ((BaseStats[IE_RACE] == 2 || BaseStats[IE_RACE] == 3) && fx->PrimaryType == 4) ret += 2;
	if (BaseStats[IE_RACE] == 6 && fx->PrimaryType == 5) ret += 2;
	if (BaseStats[IE_RACE] == 4 && fx->Resistance <= FX_CAN_RESIST_CAN_DISPEL) ret += 2;
	// monk's clear mind and mage specialists
	if (GetMonkLevel() >= 3 && fx->PrimaryType == 4) ret += 2;
	if (GetMageLevel() && (1 << (fx->PrimaryType + 5)) & BaseStats[IE_KIT]) ret += 2;

	// handle animal taming last
	// must roll a Will Save of 5 + player's total skill or higher to save
	if (stricmp(fx->Source, "SPIN108") && fx->Opcode == 5) {
		saveDC = 5;
		const Actor *caster = core->GetGame()->GetActorByGlobalID(fx->CasterID);
		if (caster) {
			saveDC += caster->GetSkill(IE_ANIMALS);
		}
	}

	if (ret > saveDC) {
		// ~Saving throw result: (d20 + save + bonuses) %d + %d  + %d vs. (10 + spellLevel + saveMod)  10 + %d + %d - Success!~
		displaymsg->DisplayRollStringName(40974, DMC_LIGHTGREY, this, roll, save, modifier, spellLevel, saveBonus);
		return true;
	} else {
		// ~Saving throw result: (d20 + save + bonuses) %d + %d  + %d vs. (10 + spellLevel + saveMod)  10 + %d + %d - Failed!~
		displaymsg->DisplayRollStringName(40975, DMC_LIGHTGREY, this, roll, save, modifier, spellLevel, saveBonus);
		return false;
	}
}

/** implements a generic opcode function, modify modified stats
returns the change
*/
int Actor::NewStat(unsigned int StatIndex, ieDword ModifierValue, ieDword ModifierType)
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
				Log(ERROR, "Actor", "Invalid modifier value (0) passed to NewStat: %d (%s)!", ModifierType, LongName);
				break;
			}
			SetStat(StatIndex, BaseStats[StatIndex] / ModifierValue, 1);
			break;
		case MOD_MODULUS:
			if (ModifierValue == 0) {
				Log(ERROR, "Actor", "Invalid modifier value (0) passed to NewStat: %d (%s)!", ModifierType, LongName);
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
			Log(ERROR, "Actor", "Invalid modifier type passed to NewStat: %d (%s)!", ModifierType, LongName);
	}
	return Modified[StatIndex] - oldmod;
}

int Actor::NewBase(unsigned int StatIndex, ieDword ModifierValue, ieDword ModifierType)
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
				Log(ERROR, "Actor", "Invalid modifier value (0) passed to NewBase: %d (%s)!", ModifierType, LongName);
				break;
			}
			SetBase(StatIndex, BaseStats[StatIndex] / ModifierValue);
			break;
		case MOD_MODULUS:
			if (ModifierValue == 0) {
				Log(ERROR, "Actor", "Invalid modifier value (0) passed to NewBase: %d (%s)!", ModifierType, LongName);
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
			Log(ERROR, "Actor", "Invalid modifier type passed to NewBase: %d (%s)!", ModifierType, LongName);
	}
	return BaseStats[StatIndex] - oldmod;
}

inline int CountElements(const char *s, char separator)
{
	int ret = 1;
	while(*s) {
		if (*s==separator) ret++;
		s++;
	}
	return ret;
}

void Actor::Interact(int type) const
{
	int start;
	int count;
	bool queue = false;

	switch(type&0xff) {
		case I_INSULT: start=VB_INSULT; break;
		case I_COMPLIMENT: start=VB_COMPLIMENT; break;
		case I_SPECIAL: start=VB_SPECIAL; break;
		case I_INSULT_RESP: start=VB_RESP_INS; queue=true; break;
		case I_COMPL_RESP: start=VB_RESP_COMP; queue=true; break;
		default:
			return;
	}
	if (type&0xff00) {
		//PST style fixed slots
		start+=((type&0xff00)>>8)-1;
		count = 1;
	} else {
		//BG1 style random slots
		count = 3;
	}
	VerbalConstant(start, count, queue ? DS_QUEUE : 0);
}

ieStrRef Actor::GetVerbalConstant(int index) const
{
	if (index<0 || index>=VCONST_COUNT) {
		return (ieStrRef) -1;
	}

	int idx = VCMap[index];

	if (idx<0 || idx>=VCONST_COUNT) {
		return (ieStrRef) -1;
	}
	return StrRefs[idx];
}

ieStrRef Actor::GetVerbalConstant(int start, int count) const
{
	while (count > 0 && GetVerbalConstant(start+count-1) == (ieStrRef) -1) {
		count--;
	}
	if (count > 0) {
		return GetVerbalConstant(start+RAND(0, count-1));
	}
	return (ieStrRef) -1;
}

bool Actor::VerbalConstant(int start, int count, int flags) const
{
	if (start!=VB_DIE) {
		//can't talk when dead
		if (Modified[IE_STATE_ID] & (STATE_CANTLISTEN)) return false;
	}

	if (count < 0) {
		return false;
	}

	flags ^= DS_CONSOLE|DS_SPEECH|DS_CIRCLE;

	//If we are main character (has SoundSet) we have to check a corresponding wav file exists
	bool found = false;
	if (PCStats && PCStats->SoundSet[0]) {
		ieResRef soundref;
		char chrsound[256];
		do {
			count--;
			ResolveStringConstant(soundref, start+count);
			GetSoundFolder(chrsound, 1, soundref);
			if (gamedata->Exists(chrsound, IE_WAV_CLASS_ID, true)) {
				DisplayStringCore((Scriptable *) this, start + RAND(0, count), flags|DS_CONST);
				found = true;
				break;
			}
		} while (count > 0);
	} else { //If we are anyone else we have to check there is a corresponding strref
		ieStrRef str = GetVerbalConstant(start, count);
		if (str != (ieStrRef) -1) {
			DisplayStringCore((Scriptable *) this, str, flags);
			found = true;
		}
	}
	return found;
}

void Actor::DisplayStringOrVerbalConstant(int str, int vcstat, int vccount) const {
	int strref = displaymsg->GetStringReference(str);
	if (strref != -1) {
		DisplayStringCore((Scriptable *) this, strref, DS_CONSOLE|DS_CIRCLE);
	} else {
		VerbalConstant(vcstat, vccount);
	}
}

bool Actor::HasSpecialDeathReaction(const char *deadname) const
{
	AutoTable tm("death");
	if (!tm) return false;
	const char *value = tm->QueryField (scriptName, deadname);
	return value && value[0] != '0';
}

void Actor::ReactToDeath(const char * deadname)
{
	AutoTable tm("death");
	if (!tm) return;
	// lookup value based on died's scriptingname and ours
	// if value is 0 - use reactdeath
	// if value is 1 - use reactspecial
	// if value is string - use playsound instead (pst)
	const char *value = tm->QueryField (scriptName, deadname);
	switch (value[0]) {
	case '0':
		VerbalConstant(VB_REACT, 1, DS_QUEUE);
		break;
	case '1':
		VerbalConstant(VB_REACT_S, 1, DS_QUEUE);
		break;
	default:
		{
			int count = CountElements(value,',');
			if (count<=0) break;
			count = core->Roll(1,count,-1);
			ieResRef resref;
			while(count--) {
				while(*value && *value!=',') value++;
				if (*value==',') value++;
			}
			CopyResRef(resref, value);
			for(count=0;count<8 && resref[count]!=',';count++) {};
			resref[count]=0;

			unsigned int len = 0;
			unsigned int channel = SFX_CHAN_CHAR0 + InParty - 1;
			core->GetAudioDrv()->Play(resref, channel, &len);
			ieDword counter = ( AI_UPDATE_TIME * len ) / 1000;
			if (counter != 0)
				SetWait( counter );
			break;
		}
	}
}

//issue area specific comments
void Actor::GetAreaComment(int areaflag) const
{
	for(int i=0;i<afcount;i++) {
		if (afcomments[i][0]&areaflag) {
			int vc = afcomments[i][1];
			if (afcomments[i][2]) {
				if (!core->GetGame()->IsDay()) {
					vc++;
				}
			}
			VerbalConstant(vc);
			return;
		}
	}
}

static int CheckInteract(const char *talker, const char *target)
{
	AutoTable interact("interact");
	if (!interact)
		return I_NONE;
	const char *value = interact->QueryField(talker, target);
	if (!value)
		return I_NONE;

	int tmp = 0;
	int x = 0;
	int ln = strlen(value);

	if (ln>1) { // PST
		//we round the length up, so the last * will be also chosen
		x = core->Roll(1,(ln+1)/2,-1)*2;
		//convert '1', '2' and '3' to 0x100,0x200,0x300 respectively, all the rest becomes 0
		//it is no problem if we hit the zero terminator in case of an odd length
		tmp = value[x+1]-'0';
		if ((ieDword) tmp>3) tmp=0;
		tmp <<= 8;
	}

	switch(value[x]) {
		case '*':
			return I_DIALOG;
		case 's':
			return tmp+I_SPECIAL;
		case 'c':
			return tmp+I_COMPLIMENT;
		case 'i':
			return tmp+I_INSULT;
		case 'I':
			return tmp+I_INSULT_RESP;
		case 'C':
			return tmp+I_COMPL_RESP;
	}
	return I_NONE;
}

void Actor::HandleInteractV1(const Actor *target)
{
	LastTalker = target->GetGlobalID();
	char tmp[50];
	snprintf(tmp, sizeof(tmp), "Interact(\"%s\")", target->GetScriptName());
	AddAction(GenerateAction(tmp));
}

int Actor::HandleInteract(const Actor *target) const
{
	int type = CheckInteract(scriptName, target->GetScriptName());

	//no interaction at all
	if (type==I_NONE) return -1;
	//banter dialog interaction
	if (type==I_DIALOG) return 0;

	Interact(type);
	switch(type)
	{
	case I_COMPLIMENT:
		target->Interact(I_COMPL_RESP);
		break;
	case I_INSULT:
		target->Interact(I_INSULT_RESP);
		break;
	}
	return 1;
}

bool Actor::GetPartyComment()
{
	Game *game = core->GetGame();

	//not an NPC
	if (BaseStats[IE_MC_FLAGS] & MC_EXPORTABLE) return false;
	// don't bother if we're not around
	if (GetCurrentArea() != game->GetCurrentArea()) return false;
	ieDword size = game->GetPartySize(true);
	//don't even bother, again
	if (size<2) return false;

	if (core->Roll(1, 2, -1)) return false;

	for (unsigned int i = core->Roll(1, size, 0), n = 0; n < size; i++, n++) {
		const Actor *target = game->GetPC(i % size, true);
		if (target==this) continue;
		if (target->BaseStats[IE_MC_FLAGS]&MC_EXPORTABLE) continue; //not NPC
		if (target->GetCurrentArea()!=GetCurrentArea()) continue;

		if (core->HasFeature(GF_RANDOM_BANTER_DIALOGS)) {
			if (core->Roll(1, 50, 0) == 1) { // TODO: confirm frequency
				//V1 interact
				HandleInteractV1(target);
				return true;
			}
		}

		//simplified interact
		switch(HandleInteract(target)) {
		case -1: return false;
		case 1: return true;
		default:
			//V2 interact
			LastTalker = target->GetGlobalID();
			Action *action = GenerateActionDirect("Interact([-1])", target);
			if (action) {
				AddActionInFront(action);
			} else {
				Log(ERROR, "Actor", "Cannot generate banter action");
			}
			return true;
		}
	}
	return false;
}

//call this only from gui selects
void Actor::PlaySelectionSound()
{
	playedCommandSound = false;
	// pst uses a slider in lieu of buttons, so the frequency value is off by 1
	unsigned int frequency = sel_snd_freq + pstflags;
	if (!pstflags && frequency > 2) frequency = 5;
	switch (frequency) {
		case 1:
			return;
		case 2:
			if (core->Roll(1,100,0) > 20) return;
			break;
		// pst-only
		case 3:
			if (core->Roll(1, 100, 0) > 50) return;
			break;
		case 4:
			if (core->Roll(1, 100, 0) > 80) return;
			break;
		default:;
	}

	//drop the rare selection comment 5% of the time
	if (InParty && core->Roll(1,100,0) <= RARE_SELECT_CHANCE){
		//rare select on main character for BG1 won't work atm
		VerbalConstant(VB_SELECT_RARE, NUM_RARE_SELECT_SOUNDS, DS_CIRCLE);
	} else {
		//checks if we are main character to limit select sounds
		if (PCStats && PCStats->SoundSet[0]) {
			VerbalConstant(VB_SELECT, NUM_MC_SELECT_SOUNDS, DS_CIRCLE);
		} else {
			VerbalConstant(VB_SELECT, NUM_SELECT_SOUNDS, DS_CIRCLE);
		}
	}
}

bool Actor::PlayWarCry(int range) const
{
	if (!war_cries) return false;
	return VerbalConstant(VB_ATTACK, range, DS_CIRCLE);
}

#define SEL_ACTION_COUNT_COMMON  3
#define SEL_ACTION_COUNT_ALL     7

//call this when a PC receives a command from GUI
void Actor::CommandActor(Action* action, bool clearPath)
{
	Scriptable::Stop(); // stop what you were doing
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
			if (pstflags) {
				if (core->Roll(1,100,0)>50) return;
			}
			break;
		case 4:
			if (pstflags) {
				if (core->Roll(1, 100, 0) > 80) return;
			}
			break;
		default:;
	}

	if (core->GetFirstSelectedPC(false) == this) {
		// bg2 uses up the traditional space for rare select sound slots for more action (command) sounds
		VerbalConstant(VB_COMMAND, raresnd ? SEL_ACTION_COUNT_ALL : SEL_ACTION_COUNT_COMMON, DS_CIRCLE);
	}
}

//Generates an idle action (party banter, area comment, bored)
void Actor::IdleActions(bool nonidle)
{
	//do we have an area
	Map *map = GetCurrentArea();
	if (!map) return;
	//and not in panic
	if (panicMode!=PANIC_NONE) return;

	Game *game = core->GetGame();
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

	ieDword time = game->GameTime;
	//did scripts disable us
	if (game->BanterBlockFlag || (game->BanterBlockTime>time) ) {
		return;
	}

	if (time/nextComment > 1) { // first run, not adjusted for game time yet
		nextComment += time;
	}

	//drop an area comment, party oneliner or initiate party banter (with Interact)
	//party comments have a priority, but they happen half of the time, at most
	if (nextComment<time) {
		if (nextComment && !Immobile() && !GetPartyComment()) {
			GetAreaComment(map->AreaType);
		}
		nextComment = time+core->Roll(5,1000,bored_time/2);
		return;
	}

	//drop the bored one liner if there was no action for some time
	//if bored timeout is disabled, don't bother to set the new time
	if (nonidle || (!nextBored && bored_time) || InMove() || Immobile()) {
		nextBored = time + core->Roll(1, 30, bored_time);
	} else {
		if (bored_time && nextBored && nextBored < time) {
			int x = std::max(10U, bored_time / 10);
			nextBored = time+core->Roll(1,30,x);
			VerbalConstant(VB_BORED);
		}

		// display idle animation
		int x = RAND(0, 25);
		if (!x && (GetStance() == IE_ANI_AWAKE)) {
			SetStance(IE_ANI_HEAD_TURN);
		}
	}
}

void Actor::PlayExistenceSounds()
{
	//only non-joinable chars can have existence sounds
	if (Persistent()) return;

	Game *game = core->GetGame();
	ieDword time = game->GameTime;
	if (time/nextComment > 1) { // first run, not adjusted for game time yet
		nextComment += time;
	}

	if (nextComment >= time) return;

	ieDword delay = Modified[IE_EXISTANCEDELAY];
	if (delay == (ieDword) -1) return;

	Audio *audio = core->GetAudioDrv();
	int xpos, ypos;
	audio->GetListenerPos(xpos, ypos);
	Point listener(xpos, ypos);
	if (nextComment && !Immobile() && WithinAudibleRange(this, listener)) {
		//setup as an ambient
		ieStrRef strref = GetVerbalConstant(VB_EXISTENCE, 5);
		if (strref != (ieStrRef) -1) {
			StringBlock sb = core->strings->GetStringBlock(strref);
			if (sb.Sound[0]) {
				unsigned int vol = 100;
				core->GetDictionary()->Lookup("Volume Ambients", vol);
				int stream = audio->SetupNewStream(Pos.x, Pos.y, 0, vol, true, 50); // REFERENCE_DISTANCE
				if (stream != -1) {
					int audioLength = audio->QueueAmbient(stream, sb.Sound);
					if (audioLength > 0) {
						SetAnimatedTalking(audioLength);
					}
					audio->ReleaseStream(stream, false);
				}
			}
		}
	}
	if (delay == 0) {
		delay = VOODOO_EXISTENCE_DELAY_DEFAULT;
	}
	nextComment = time + RAND(delay*1/4, delay*7/4);
}

bool Actor::OverrideActions()
{
	//TODO:: implement forced actions that mess with scripting (panic, confusion, etc)
	// domination and dire charm: force the actors to be useful (trivial ai)
	Action *action;
	if ((Modified[IE_STATE_ID] & STATE_CHARMED) && (BaseStats[IE_EA] <= EA_GOODCUTOFF) && Modified[IE_EA] == EA_CHARMEDPC) {
		Effect *charm = fxqueue.HasEffect(fx_set_charmed_state_ref);
		if (!charm) return false;

		// skip regular charm
		switch (charm->Parameter2) {
			case 2:
			case 3:
			case 5:
			case 1002:
			case 1003:
			case 1005:
				action = GenerateAction("AttackReevaluate([GOODCUTOFF],10)");
				if (action) {
					AddActionInFront(action);
					return true;
				} else {
					Log(ERROR, "Actor", "Cannot generate override action");
				}
				break;
			default:
				break;
		}
	}
	return false;
}

void Actor::Panic(const Scriptable *attacker, int panicmode)
{
	if (GetStat(IE_STATE_ID)&STATE_PANIC) {
		print("Already panicked");
		//already in panic
		return;
	}
	if (InParty) core->GetGame()->SelectActor(this, false, SELECT_NORMAL);
	VerbalConstant(VB_PANIC);

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

void Actor::SetMCFlag(ieDword arg, int op)
{
	ieDword tmp = BaseStats[IE_MC_FLAGS];
	core->SetBits(tmp, arg, op);
	SetBase(IE_MC_FLAGS, tmp);
}

void Actor::DialogInterrupt() const
{
	//if dialoginterrupt was set, no verbal constant
	if ( Modified[IE_MC_FLAGS]&MC_NO_TALK)
		return;

	/* this part is unsure */
	if (Modified[IE_EA]>=EA_EVILCUTOFF) {
		VerbalConstant(VB_HOSTILE);
	} else {
		if (TalkCount) {
			VerbalConstant(VB_DIALOG);
		} else {
			VerbalConstant(VB_INITIALMEET);
		}
	}
}

void Actor::GetHit(int damage, int spellLevel)
{
	if (!Immobile() && !(InternalFlags & IF_REALLYDIED)) {
		SetStance( IE_ANI_DAMAGE );
		VerbalConstant(VB_DAMAGE);
	}

	if (Modified[IE_STATE_ID]&STATE_SLEEP) {
		if (Modified[IE_EXTSTATE_ID]&EXTSTATE_NO_WAKEUP || HasSpellState(SS_NOAWAKE)) {
			return;
		}
		Effect *fx = EffectQueue::CreateEffect(fx_cure_sleep_ref, 0, 0, FX_DURATION_INSTANT_PERMANENT);
		fxqueue.AddEffect(fx);
		delete fx;
	}
	if (CheckSpellDisruption(damage, spellLevel)) {
		InterruptCasting = true;
	}
}

// this has no effect in adnd
// iwd2 however has two mechanisms: spell disruption and concentration checks:
// - disruption is checked when a caster is damaged while casting
// - concentration is checked when casting is taking place <= 5' from an enemy
bool Actor::CheckSpellDisruption(int damage, int spellLevel) const
{
	if (core->HasFeature(GF_SIMPLE_DISRUPTION)) {
		return LuckyRoll(1, 20, 0) < (damage + spellLevel);
	}
	if (!third) {
		return true;
	}

	if (!LastSpellTarget && LastTargetPos.isempty()) {
		// not casting, nothing to do
		return false;
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
		displaymsg->DisplayRollStringName(39842, DMC_LIGHTGREY, this, roll, concentration, bonus, damage, spellLevel);
	}
	int chance = (roll + concentration + bonus) > (10 + damage + spellLevel);
	if (chance) {
		return false;
	}
	return true;
}

bool Actor::HandleCastingStance(const ieResRef SpellResRef, bool deplete, bool instant)
{
	if (deplete) {
		if (! spellbook.HaveSpell( SpellResRef, HS_DEPLETE )) {
			SetStance(IE_ANI_READY);
			return true;
		}
	}
	if (!instant) {
		SetStance(IE_ANI_CAST);
	}
	return false;
}

bool Actor::AttackIsStunning(int damagetype) const {
	//stunning damagetype
	if (damagetype & DAMAGE_STUNNING) {
		return true;
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
			delete fx;
			// ~Cleave feat adds another level %d attack.~
			// uses the max tohit bonus (tested), but game always displayed "level 1"
			displaymsg->DisplayRollStringName(39846, DMC_LIGHTGREY, this, ToHit.GetTotal());
		}
	}
}


//returns actual damage
int Actor::Damage(int damage, int damagetype, Scriptable *hitter, int modtype, int critical, int saveflags)
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

	//add lastdamagetype up ? maybe
	//FIXME: what does original do?
	LastDamageType |= damagetype;

	Actor *act = NULL;
	if (hitter && hitter->Type == ST_ACTOR) {
		act = (Actor *) hitter;
	}

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

	if (damage && !(saveflags&SF_BYPASS_MIRROR_IMAGE)) {
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
			int adjustmentPercent = dmgadjustments[GameDifficulty - 1];
			if (!NoExtraDifficultyDmg || adjustmentPercent < 0) {
				damage += (damage * adjustmentPercent)/100;
			}
		}
	}

	int resisted = 0;
	if (damage) {
		ModifyDamage (hitter, damage, resisted, damagetype);
	}
	DisplayCombatFeedback(damage, resisted, damagetype, hitter);

	if (damage > 0) {
		// instant chunky death if the actor is petrified or frozen
		bool allowChunking = !Modified[IE_DISABLECHUNKING] && GameDifficulty > DIFF_NORMAL;
		if (Modified[IE_STATE_ID] & (STATE_FROZEN|STATE_PETRIFIED) && allowChunking) {
			damage = 123456; // arbitrarily high for death; won't be displayed
			LastDamageType |= DAMAGE_CHUNKING;
		}
		// chunky death when you're reduced below -10 hp
		if ((ieDword) damage >= Modified[IE_HITPOINTS] + 10 && allowChunking) {
			LastDamageType |= DAMAGE_CHUNKING;
		}
		// mark LastHitter for repeating damage effects (eg. to get xp from melfing trolls)
		if (act && LastHitter == 0) {
			LastHitter = act->GetGlobalID();
		}
	}

	if (BaseStats[IE_HITPOINTS] <= (ieDword) damage) {
		// common fists do normal damage, but cause sleeping for a round instead of death
		if (Modified[IE_MINHITPOINTS] <= 0 && AttackIsStunning(damagetype)) {
			// stack unconsciousness carefully to avoid replaying the stance changing
			Effect *sleep = fxqueue.HasEffectWithParamPair(fx_sleep_ref, 0, 0);
			if (sleep) {
				sleep->Duration += core->Time.round_sec;
			} else {
				Effect *fx = EffectQueue::CreateEffect(fx_sleep_ref, 0, 0, FX_DURATION_INSTANT_LIMITED);
				fx->Duration = core->Time.round_sec; // 1 round
				core->ApplyEffect(fx, this, this);
				delete fx;
			}
			//reduce damage to keep 1 hp
			damage = Modified[IE_HITPOINTS] - 1;
		}
	}

	// also apply reputation damage if we hurt (but not killed) an innocent
	if (Modified[IE_CLASS] == CLASS_INNOCENT && !core->InCutSceneMode()) {
		if (act && act->GetStat(IE_EA) <= EA_CONTROLLABLE) {
			core->GetGame()->SetReputation(core->GetGame()->Reputation + core->GetReputationMod(1));
		}
	}

	int chp = (signed) BaseStats[IE_HITPOINTS];
	if (damage > 0) {
		//if this kills us, check if attacker could cleave
		if (act && (damage > chp)) {
			act->CheckCleave();
		}
		GetHit(damage, 3); // FIXME: carry over the correct spellLevel
		//fixme: implement applytrigger, copy int0 into LastDamage there
		LastDamage = damage;
		AddTrigger(TriggerEntry(trigger_tookdamage, damage)); // FIXME: lastdamager? LastHitter is not set for spell damage
		AddTrigger(TriggerEntry(trigger_hitby, LastHitter, damagetype)); // FIXME: currently lastdamager, should it always be set regardless of damage?

		// impact morale when hp thresholds (50 %, 25 %) are crossed for the first time
		int currentRatio = 100 * chp / (signed) BaseStats[IE_MAXHITPOINTS];
		int newRatio = 100 * (chp + damage) / (signed) BaseStats[IE_MAXHITPOINTS];
		if (ShouldModifyMorale()) {
			if (currentRatio > 50 && newRatio < 25) {
				NewBase(IE_MORALE, (ieDword) -4, MOD_ADDITIVE);
			} else if (currentRatio > 50 && newRatio < 50) {
				NewBase(IE_MORALE, (ieDword) -2, MOD_ADDITIVE);
			} else if (currentRatio > 25 && newRatio < 25) {
				NewBase(IE_MORALE, (ieDword) -2, MOD_ADDITIVE);
			}
		}
	}

	// can be negative if we're healing on 100%+ resistance
	// do this after GetHit, so VB_HURT isn't overriden by VB_DAMAGE, just (dis)played later
	if (damage != 0) {
		NewBase(IE_HITPOINTS, (ieDword) -damage, MOD_ADDITIVE);
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
	} else if (damagetype & (DAMAGE_ELECTRICITY)) {
		PlayDamageAnimation(DL_ELECTRICITY + damagelevel);
	} else if (damagetype & (DAMAGE_ACID)) {
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
			core->Autopause(AP_WOUNDED, this);
		}
		if (damage > 0) {
			core->Autopause(AP_HIT, this);
			core->SetEventFlag(EF_PORTRAIT);
		}
	}
	return damage;
}

void Actor::DisplayCombatFeedback (unsigned int damage, int resisted, int damagetype, Scriptable *hitter)
{
	// shortcircuit for disintegration, which wouldn't hit any of the below
	if (damage == 0 && resisted == 0) return;

	bool detailed = false;
	const char *type_name = "unknown";
	if (DisplayMessage::HasStringReference(STR_DAMAGE_DETAIL1)) { // how and iwd2
		std::multimap<ieDword, DamageInfoStruct>::iterator it;
		it = core->DamageInfoMap.find(damagetype);
		if (it != core->DamageInfoMap.end()) {
			type_name = core->GetCString(it->second.strref, 0);
		}
		detailed = true;
	}

	if (damage > 0 && resisted != DR_IMMUNE) {
		Log(COMBAT, "Actor", "%d %s damage taken.\n", damage, type_name);

		if (!core->HasFeedback(FT_STATES)) goto hitsound;

		if (detailed) {
			// 3 choices depending on resistance and boni
			// iwd2 also has two Tortoise Shell (spell) absorption strings
			core->GetTokenDictionary()->SetAtCopy( "TYPE", type_name);
			core->GetTokenDictionary()->SetAtCopy( "AMOUNT", damage);

			int strref;
			if (resisted < 0) {
				//Takes <AMOUNT> <TYPE> damage from <DAMAGER> (<RESISTED> damage bonus)
				core->GetTokenDictionary()->SetAtCopy( "RESISTED", abs(resisted));
				strref = STR_DAMAGE_DETAIL3;
			} else if (resisted > 0) {
				//Takes <AMOUNT> <TYPE> damage from <DAMAGER> (<RESISTED> damage resisted)
				core->GetTokenDictionary()->SetAtCopy( "RESISTED", abs(resisted));
				strref = STR_DAMAGE_DETAIL2;
			} else {
				//Takes <AMOUNT> <TYPE> damage from <DAMAGER>
				strref = STR_DAMAGE_DETAIL1;
			}
			if (hitter && hitter->Type == ST_ACTOR) {
				core->GetTokenDictionary()->SetAtCopy( "DAMAGER", hitter->GetName(1) );
			} else {
				// variant without damager
				strref -= (STR_DAMAGE_DETAIL1 - STR_DAMAGE1);
			}
			displaymsg->DisplayConstantStringName(strref, DMC_WHITE, this);
		} else if (core->HasFeature(GF_ONSCREEN_TEXT) ) {
			//TODO: handle pst properly (decay, queueing, color)
			wchar_t dmg[10];
			swprintf(dmg, sizeof(dmg)/sizeof(dmg[0]), L"%d", damage);
			SetOverheadText(dmg, true);
		} else if (!displaymsg->HasStringReference(STR_DAMAGE2) || !hitter || hitter->Type != ST_ACTOR) {
			// bg1 and iwd
			// or any traps or self-infliction (also for bg1)
			// construct an i18n friendly "Damage Taken (damage)", since there's no token
			String* msg = core->GetString(displaymsg->GetStringReference(STR_DAMAGE1), 0);
			wchar_t dmg[10];
			swprintf(dmg, sizeof(dmg)/sizeof(dmg[0]), L" (%d)", damage);
			displaymsg->DisplayStringName(*msg + dmg, DMC_WHITE, this);
			delete msg;
		} else { //bg2
			//<DAMAGER> did <AMOUNT> damage to <DAMAGEE>
			core->GetTokenDictionary()->SetAtCopy( "DAMAGEE", GetName(1) );
			// wipe the DAMAGER token, so we can color it
			core->GetTokenDictionary()->SetAtCopy( "DAMAGER", "" );
			core->GetTokenDictionary()->SetAtCopy( "AMOUNT", damage);
			displaymsg->DisplayConstantStringName(STR_DAMAGE2, DMC_WHITE, hitter);
		}
	} else {
		if (resisted == DR_IMMUNE) {
			Log(COMBAT, "Actor", "is immune to damage type: %s.\n", type_name);
			if (hitter && hitter->Type == ST_ACTOR) {
				if (detailed) {
					//<DAMAGEE> was immune to my <TYPE> damage
					core->GetTokenDictionary()->SetAtCopy( "DAMAGEE", GetName(1) );
					core->GetTokenDictionary()->SetAtCopy( "TYPE", type_name );
					displaymsg->DisplayConstantStringName(STR_DAMAGE_IMMUNITY, DMC_WHITE, hitter);
				} else if (displaymsg->HasStringReference(STR_DAMAGE_IMMUNITY) && displaymsg->HasStringReference(STR_DAMAGE1)) {
					// bg2
					//<DAMAGEE> was immune to my damage.
					core->GetTokenDictionary()->SetAtCopy( "DAMAGEE", GetName(1) );
					displaymsg->DisplayConstantStringName(STR_DAMAGE_IMMUNITY, DMC_WHITE, hitter);
				} // else: other games don't display anything
			}
		} else {
			// mirror image or stoneskin: no message
		}
	}

	hitsound:
	//Play hit sounds, for pst, resdata contains the armor level
	DataFileMgr *resdata = core->GetResDataINI();
	PlayHitSound(resdata, damagetype, false);
}

void Actor::PlayWalkSound()
{
	ieDword thisTime;
	ieResRef Sound;

	thisTime = GetTickCount();
	if (thisTime<nextWalk) return;
	int cnt = anims->GetWalkSoundCount();
	if (!cnt) return;

	cnt=core->Roll(1,cnt,-1);
	strnuprcpy(Sound, anims->GetWalkSound(), sizeof(ieResRef)-1 );
	area->ResolveTerrainSound(Sound, Pos);

	if (Sound[0] == '*') return;

	int l = strlen(Sound);
	/* IWD1, HOW, IWD2 sometimes append numbers here, not letters. */
	if (core->HasFeature(GF_SOUNDFOLDERS) && 0 == memcmp(Sound, "FS_", 3)) {
		if (l < 8) {
			Sound[l] = cnt + 0x31;
			Sound[l+1] = 0;
		}
	} else {
		if (cnt) {
			if (l < 8) {
				Sound[l] = cnt + 0x60; // append 'a'-'g'
				Sound[l+1] = 0;
			}
		}
	}

	unsigned int len = 0;
	unsigned int channel = InParty ? SFX_CHAN_WALK_CHAR : SFX_CHAN_WALK_MONSTER;
	core->GetAudioDrv()->Play(Sound, channel, Pos.x, Pos.y, 0, &len);
	nextWalk = thisTime + len;
}

// guesses from audio:               bone  chain studd leather splint none other plate
static const char *armor_types[8] = { "BN", "CH", "CL", "LR", "ML", "MM", "MS", "PT" };
static const char *dmg_types[5] = { "PC", "SL", "BL", "ML", "RK" };

//Play hit sounds (HIT_0<dtype><armor>)
//IWDs have H_<dmgtype>_<armor> (including level from 1 to max 5), eg H_ML_MM3
void Actor::PlayHitSound(DataFileMgr *resdata, int damagetype, bool suffix) const
{
	int type;
	bool levels = true;

	switch(damagetype) {
		case DAMAGE_PIERCING: type = 1; break; //piercing
		case DAMAGE_SLASHING: type = 2; break; //slashing
		case DAMAGE_CRUSHING: type = 3; break; //crushing
		case DAMAGE_MISSILE: type = 4; break;  //missile
		case DAMAGE_ELECTRICITY: type = 5; levels = false; break; //electricity
		case DAMAGE_COLD: type = 6; levels = false; break;     //cold
		case DAMAGE_MAGIC: type = 7; levels = false; break;
		case DAMAGE_STUNNING: type = -3; break;
		default: return;                       //other
	}

	ieResRef Sound;
	int armor = 0;

	if (resdata) {
		char section[12];
		unsigned int animid=BaseStats[IE_ANIMATION_ID];
		if(core->HasFeature(GF_ONE_BYTE_ANIMID)) {
			animid&=0xff;
		}

		snprintf(section,10,"%d", animid);

		if (type<0) {
			type = -type;
		} else {
			armor = resdata->GetKeyAsInt(section, "armor",0);
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

	if (core->HasFeature(GF_IWD2_SCRIPTNAME)) {
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

		snprintf(Sound, 9, "H_%s_%s%d", dmg_types[type-1], armor_types[armor], core->Roll(1, 3, 0));
	} else {
		if (levels) {
			snprintf(Sound, 9, "HIT_0%d%c%c", type, armor+'A', suffix?'1':0);
		} else {
			snprintf(Sound, 9, "HIT_0%d%c", type, suffix?'1':0);
		}
	}
	core->GetAudioDrv()->Play(Sound, SFX_CHAN_HITS, Pos.x, Pos.y);
}

// Play swing sounds
// <wtype><n> in bgs, but it gets quickly complicated with iwds (SW_<wname>) and pst, which add a bunch of exceptions
// ... so they're just fully stored in itemsnd.2da
// iwds also have five sounds of hitting armor (SW_SWD01) that we ignore
// pst has a lot of duplicates (1&3&6, 2&5, 8&9, 4, 7, 10) and little variation (all 6 entries for 8 are identical)
void Actor::PlaySwingSound(WeaponInfo &wi) const
{
	ieResRef sound;
	ieDword itemType = wi.itemtype;
	int isCount = gamedata->GetSwingCount(itemType);

	if (isCount == -2) {
		// monsters with non-standard items, none or something else
		int stance = GetStance();
		if (stance == IE_ANI_ATTACK_SLASH || stance == IE_ANI_ATTACK_BACKSLASH || stance == IE_ANI_ATTACK_JAB || stance == IE_ANI_SHOOT) {
			GetSoundFromFile(sound, 100+stance);
		}
	} else {
		// swing sounds start at column 3 (index 2)
		int isChoice = core->Roll(1, isCount, -1) + 2;
		if (!gamedata->GetItemSound(sound, itemType, NULL, isChoice)) return;
	}

	core->GetAudioDrv()->Play(sound, SFX_CHAN_SWINGS, Pos.x, Pos.y);
}

//Just to quickly inspect debug maximum values
#if 0
void Actor::dumpMaxValues()
{
	int symbol = core->LoadSymbol( "stats" );
	if (symbol !=-1) {
		SymbolMgr *sym = core->GetSymbol( symbol );

		for(int i=0;i<MAX_STATS;i++) {
			print("%d(%s) %d", i, sym->GetValue(i), maximum_values[i]);
		}
	}
}
#endif

void Actor::dump() const
{
	StringBuffer buffer;
	dump(buffer);
	Log(DEBUG, "Actor", buffer);
}

void Actor::dump(StringBuffer& buffer) const
{
	buffer.appendFormatted( "Debugdump of Actor %s (%s, %s):\n", LongName, ShortName, GetName(-1) );
	buffer.append("Scripts:");
	for (unsigned int i = 0; i < MAX_SCRIPTS; i++) {
		const char* poi = "<none>";
		if (Scripts[i]) {
			poi = Scripts[i]->GetName();
		}
		buffer.appendFormatted( " %.8s", poi );
	}
	buffer.append("\n");
	buffer.appendFormatted("Area:       %.8s ([%d.%d])\n", Area, Pos.x, Pos.y);
	buffer.appendFormatted("Dialog:     %.8s    TalkCount:  %d\n", Dialog, TalkCount);
	buffer.appendFormatted("Global ID:  %d   PartySlot: %d\n", GetGlobalID(), InParty);
	buffer.appendFormatted("Script name:%.32s    Current action: %d    Total: %ld\n", scriptName, CurrentAction ? CurrentAction->actionID : -1, (long) actionQueue.size());
	buffer.appendFormatted("Int. Flags: 0x%x    ", InternalFlags);
	buffer.appendFormatted("MC Flags: 0x%x    ", Modified[IE_MC_FLAGS]);
	buffer.appendFormatted("Allegiance: %d   current allegiance:%d\n", BaseStats[IE_EA], Modified[IE_EA] );
	buffer.appendFormatted("Class:      %d   current class:%d    Kit: %d (base: %d)\n", BaseStats[IE_CLASS], Modified[IE_CLASS], Modified[IE_KIT], BaseStats[IE_KIT] );
	buffer.appendFormatted("Race:       %d   current race:%d\n", BaseStats[IE_RACE], Modified[IE_RACE] );
	buffer.appendFormatted("Gender:     %d   current gender:%d\n", BaseStats[IE_SEX], Modified[IE_SEX] );
	buffer.appendFormatted("Specifics:  %d   current specifics:%d\n", BaseStats[IE_SPECIFIC], Modified[IE_SPECIFIC] );
	buffer.appendFormatted("Alignment:  %x   current alignment:%x\n", BaseStats[IE_ALIGNMENT], Modified[IE_ALIGNMENT] );
	buffer.appendFormatted("Morale:     %d   current morale:%d\n", BaseStats[IE_MORALE], Modified[IE_MORALE] );
	buffer.appendFormatted("Moralebreak:%d   Morale recovery:%d\n", Modified[IE_MORALEBREAK], Modified[IE_MORALERECOVERYTIME] );
	buffer.appendFormatted("Visualrange:%d (Explorer: %d)\n", Modified[IE_VISUALRANGE], Modified[IE_EXPLORE] );
	buffer.appendFormatted("Fatigue: %d (current: %d)   Luck: %d\n", BaseStats[IE_FATIGUE], Modified[IE_FATIGUE], Modified[IE_LUCK]);
	buffer.appendFormatted("Movement rate: %d (current: %d)\n\n", BaseStats[IE_MOVEMENTRATE], Modified[IE_MOVEMENTRATE]);

	//this works for both level slot style
	buffer.appendFormatted("Levels (average: %d):\n", GetXPLevel(true));
	for (unsigned int i = 0; i < ISCLASSES; i++) {
		int level = GetClassLevel(i);
		if (level) {
			buffer.appendFormatted("%s: %d    ", isclassnames[i], level);
		}
	}
	buffer.appendFormatted("\n");

	buffer.appendFormatted("current HP:%d\n", BaseStats[IE_HITPOINTS] );
	buffer.appendFormatted("Mod[IE_ANIMATION_ID]: 0x%04X ResRef:%.8s Stance: %d\n", Modified[IE_ANIMATION_ID], anims->ResRef, GetStance() );
	buffer.appendFormatted("TURNUNDEADLEVEL: %d current: %d\n", BaseStats[IE_TURNUNDEADLEVEL], Modified[IE_TURNUNDEADLEVEL]);
	buffer.appendFormatted("Colors:    ");
	if (core->HasFeature(GF_ONE_BYTE_ANIMID) ) {
		for(unsigned int i = 0; i < Modified[IE_COLORCOUNT]; i++) {
			buffer.appendFormatted("   %d", Modified[IE_COLORS+i]);
		}
	} else {
		for(unsigned int i = 0; i < 7; i++) {
			buffer.appendFormatted("   %d", Modified[IE_COLORS+i]);
		}
	}
	buffer.append("\n");
	buffer.appendFormatted("WaitCounter: %d\n", (int) GetWait());
	buffer.appendFormatted("LastTarget: %d %s    ", LastTarget, GetActorNameByID(LastTarget));
	buffer.appendFormatted("LastSpellTarget: %d %s\n", LastSpellTarget, GetActorNameByID(LastSpellTarget));
	buffer.appendFormatted("LastTalked: %d %s\n", LastTalker, GetActorNameByID(LastTalker));
	inventory.dump(buffer);
	spellbook.dump(buffer);
	fxqueue.dump(buffer);
}

const char* Actor::GetActorNameByID(ieDword ID) const
{
	Actor *actor = GetCurrentArea()->GetActorByGlobalID(ID);
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
	if (BlocksSearchMap()) map->BlockSearchMap(Pos, size, IsPartyMember() ? PATH_MAP_PC : PATH_MAP_NPC);
}

// Position should be a navmap point
void Actor::SetPosition(const Point &nmptTarget, int jump, int radiusx, int radiusy)
{
	ResetPathTries();
	ClearPath(true);
	Point p, q;
	p.x = nmptTarget.x / 16;
	p.y = nmptTarget.y / 12;

	q = p;
	lastFrame = NULL;
	if (jump && !(Modified[IE_DONOTJUMP] & DNJ_FIT) && size ) {
		Map *map = GetCurrentArea();
		//clear searchmap so we won't block ourselves
		map->ClearSearchMapFor(this);
		map->AdjustPosition( p, radiusx, radiusy );
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
	const ieDword *stats;

	if (modified) {
		stats = Modified;
	}
	else {
		stats = BaseStats;
	}

	int clscount = 0;
	float average = 0;
	if (iwd2class) {
		// iwd2
		return stats[IE_CLASSLEVELSUM];
	} else {
		unsigned int levels[3]={stats[IE_LEVEL], stats[IE_LEVEL2], stats[IE_LEVEL3]};
		average = levels[0];
		clscount = 1;
		if (IsDualClassed()) {
			// dualclassed
			if (levels[1] > 0) {
				clscount++;
				average += levels[1];
			}
		}
		else if (IsMultiClassed()) {
				//clscount is the number of on bits in the MULTI field
				clscount = core->CountBits (multiclass);
				assert(clscount && clscount <= 3);
				for (int i=1; i<clscount; i++)
					average += levels[i];
		} //else single classed
		average = average / (float) clscount + 0.5;
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
			Log(WARNING, "Actor", "Unhandled SPL type %d, using average casting level!", spelltype);
		}
		break;
	}
	// if nothing was found, use the average level
	if (!level && !flags) level = GetXPLevel(true);

	return level;
}

int Actor::GetWildMod(int level)
{
	if (GetStat(IE_KIT) == KIT_WILDMAGE) {
		// avoid rerolling the mod, since we get called multiple times per each cast
		// TODO: also handle a reroll to 0
		if (!WMLevelMod) {
			if (level>=MAX_LEVEL) level=MAX_LEVEL;
			if(level<1) level=1;
			WMLevelMod = wmlevels[core->Roll(1,20,-1)][level-1];

			core->GetTokenDictionary()->SetAtCopy("LEVELDIF", abs(WMLevelMod));
			if (core->HasFeedback(FT_STATES)) {
				if (WMLevelMod > 0) {
					displaymsg->DisplayConstantStringName(STR_CASTER_LVL_INC, DMC_WHITE, this);
				} else if (WMLevelMod < 0) {
					displaymsg->DisplayConstantStringName(STR_CASTER_LVL_DEC, DMC_WHITE, this);
				}
			}
		}
		return WMLevelMod;
	}
	return 0;
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
			displaymsg->DisplayConstantStringName(STR_HALFSPEED, DMC_WHITE, this);
		}
		return 2;
	}
	if (feedback && core->HasFeedback(FT_STATES)) {
		displaymsg->DisplayConstantStringName(STR_CANTMOVE, DMC_WHITE, this);
	}
	return 123456789; // large enough to round to 0 when used as a divisor
}

int Actor::CalculateSpeed(bool feedback) const
{
	if (core->HasFeature(GF_RESDATA_INI)) {
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
	if (core->HasFeature(GF_ONE_BYTE_ANIMID)) {
		animid = animid & 0xff;
	}
	assert(animid < (ieDword)CharAnimations::GetAvatarsCount());
	const AvatarStruct *avatar = CharAnimations::GetAvatarStruct(animid);
	int newSpeed = 0;
	if (avatar->RunScale && (GetInternalFlag() & IF_RUNNING)) {
		newSpeed = avatar->RunScale;
	} else if (avatar->WalkScale) {
		newSpeed = avatar->WalkScale;
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
	bool evilcleric = false;

	if (!turnlevel) {
		return;
	}

	//determine if we see the cleric (distance)
	if (!CanSee(cleric, this, true, GA_NO_DEAD)) {
		return;
	}

	if ((cleric->Type==ST_ACTOR) && GameScript::ID_Alignment((Actor *)cleric,AL_EVIL) ) {
		evilcleric = true;
	}

	//a little adjustment of the level to get a slight randomness on who is turned
	unsigned int level = GetXPLevel(true)-(GetGlobalID()&3);

	//this is safely hardcoded i guess
	if (Modified[IE_GENERAL]!=GEN_UNDEAD) {
		level = GetPaladinLevel();
		if (evilcleric && level) {
			AddTrigger(TriggerEntry(trigger_turnedby, cleric->GetGlobalID()));
			if (turnlevel >= level+TURN_DEATH_LVL_MOD) {
				if (gamedata->Exists("panic", IE_SPL_CLASS_ID)) {
					core->ApplySpell("panic", this, cleric, level);
				} else {
					print("Panic from turning!");
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
	if (turnlevel >= level+TURN_DEATH_LVL_MOD) {
		if (evilcleric) {
			Effect *fx = fxqueue.CreateEffect(control_creature_ref, GEN_UNDEAD, 3, FX_DURATION_INSTANT_LIMITED);
			if (!fx) {
				fx = fxqueue.CreateEffect(control_undead_ref, GEN_UNDEAD, 3, FX_DURATION_INSTANT_LIMITED);
			}
			if (fx) {
				fx->Duration = core->Time.round_sec;
				fx->Target = FX_TARGET_PRESET;
				core->ApplyEffect(fx, this, cleric);
				delete fx;
				return;
			}
			//fallthrough for bg1
		}
		Die(cleric);
	} else if (turnlevel >= level+TURN_PANIC_LVL_MOD) {
		print("Panic from turning!");
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
	SetBase(IE_STATE_ID, 0);
	SetBase(IE_AVATARREMOVAL, 0);
	if (!destPoint.isnull()) {
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
	if (core->HasFeature(GF_HAS_KAPUTZ) && (AppearanceFlags&APP_DEATHVAR)) {
		const size_t len = snprintf(DeathVar, sizeof(ieVariable), "%s_DEAD", scriptName);
		if (len > sizeof(ieVariable)) {
			Log(ERROR, "Actor", "Scriptname %s (name: %s) is too long for generating death globals!", scriptName, LongName);
		}
		ieDword value=0;

		game->kaputz->Lookup(DeathVar, value);
		if (value>0) {
			game->kaputz->SetAt(DeathVar, value-1);
		}
	// not bothering with checking actor->SetDeathVar, since the SetAt nocreate parameter is true
	} else if (!core->HasFeature(GF_HAS_KAPUTZ)) {
		size_t len = snprintf(DeathVar, 32, core->GetDeathVarFormat(), scriptName);
		if (len > 32) {
			Log(ERROR, "Actor", "Scriptname %s (name: %s) is too long for generating death globals (on resurrect)!", scriptName, LongName);
		}
		game->locals->SetAt(DeathVar, 0, true);
	}

	ResetCommentTime();
	//clear effects?
}

static const char *GetVarName(const char *table, int value)
{
	int symbol = core->LoadSymbol( table );
	if (symbol!=-1) {
		Holder<SymbolMgr> sym = core->GetSymbol( symbol );
		return sym->GetValue( value );
	}
	return NULL;
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

	std::vector<Actor *>::iterator poi;
	for (poi = neighbours.begin(); poi != neighbours.end(); poi++) {
		// NOTE: currently also sending the trigger to ourselves — prevent if it causes problems
		(*poi)->AddTrigger(TriggerEntry(trigger_died, GetGlobalID()));

		// allies take a hit on morale and nobody cares about neutrals
		if (!(*poi)->ShouldModifyMorale()) continue;
		int pea = (*poi)->GetStat(IE_EA);
		if (ea == EA_PC && pea == EA_PC) {
			(*poi)->NewBase(IE_MORALE, (ieDword) -1, MOD_ADDITIVE);
		} else if (OfType(this, *poi)) {
			(*poi)->NewBase(IE_MORALE, (ieDword) -1, MOD_ADDITIVE);
		// are we an enemy of poi, regardless if we're good or evil?
		} else if (abs(ea - pea) > 30) {
			(*poi)->NewBase(IE_MORALE, 2, MOD_ADDITIVE);
		}
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

	displaymsg->DisplayConstantStringName(STR_DEATH, DMC_WHITE, this);
	VerbalConstant(VB_DIE);

	// remove poison, hold, casterhold, stun and its icon
	Effect *newfx;
	newfx = EffectQueue::CreateEffect(fx_cure_poisoned_state_ref, 0, 0, FX_DURATION_INSTANT_PERMANENT);
	core->ApplyEffect(newfx, this, this);
	delete newfx;
	newfx = EffectQueue::CreateEffect(fx_cure_hold_state_ref, 0, 0, FX_DURATION_INSTANT_PERMANENT);
	core->ApplyEffect(newfx, this, this);
	delete newfx;
	newfx = EffectQueue::CreateEffect(fx_unpause_caster_ref, 0, 100, FX_DURATION_INSTANT_PERMANENT);
	core->ApplyEffect(newfx, this, this);
	delete newfx;
	newfx = EffectQueue::CreateEffect(fx_cure_stun_state_ref, 0, 0, FX_DURATION_INSTANT_PERMANENT);
	core->ApplyEffect(newfx, this, this);
	delete newfx;
	newfx = EffectQueue::CreateEffect(fx_remove_portrait_icon_ref, 0, 37, FX_DURATION_INSTANT_PERMANENT);
	core->ApplyEffect(newfx, this, this);
	delete newfx;

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
	if (GetStance() != IE_ANI_DIE) {
		SetStance(IE_ANI_DIE);
	}
	AddTrigger(TriggerEntry(trigger_die));
	SendDiedTrigger();
	if (pstflags) {
		AddTrigger(TriggerEntry(trigger_namelessbitthedust));
	}

	Actor *act=NULL;
	if (!killer) {
		// TODO: is this right?
		killer = area->GetActorByGlobalID(LastHitter);
	}

	bool killerPC = false;
	if (killer && killer->Type == ST_ACTOR) {
		act = (Actor *) killer;
		// for unknown reasons the original only sends the trigger if the killer is ok
		if (act && !(act->GetStat(IE_STATE_ID) & (STATE_DEAD|STATE_PETRIFIED|STATE_FROZEN))) {
			killer->AddTrigger(TriggerEntry(trigger_killed, GetGlobalID()));
			if (act->ShouldModifyMorale()) act->NewBase(IE_MORALE, 3, MOD_ADDITIVE);
		}
		killerPC = act->InParty > 0;
	}

	if (InParty) {
		game->PartyMemberDied(this);
		core->Autopause(AP_DEAD, this);
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
				repmod = core->GetReputationMod(0);
			} else if (Modified[IE_CLASS] == CLASS_FLAMINGFIST) {
				repmod = core->GetReputationMod(3);
			}
			if (GameScript::ID_Alignment(this,AL_EVIL) ) {
				repmod += core->GetReputationMod(7);
			}
			if (repmod) {
				game->SetReputation(game->Reputation + repmod);
			}
		}
	}

	ReleaseCurrentAction();
	ClearPath(true);
	SetModal( MS_NONE );

	if (InParty && killerPC) {
		game->locals->SetAt("PM_KILLED", 1, nocreate);
	}

	// EXTRACOUNT is updated at the moment of death
	if (Modified[IE_SEX] == SEX_EXTRA || (Modified[IE_SEX] >= SEX_EXTRA2 && Modified[IE_SEX] <= SEX_MAXEXTRA)) {
		// if gender is set to one of the EXTRA values, then at death, we have to decrease
		// the relevant EXTRACOUNT area variable. scripts use this to check how many actors
		// of this extra id are still alive (for example, see the ToB challenge scripts)
		ieVariable varname;
		if (Modified[IE_SEX] == SEX_EXTRA) {
			snprintf(varname, 32, "EXTRACOUNT");
		} else {
			snprintf(varname, 32, "EXTRACOUNT%d", 2 + (Modified[IE_SEX] - SEX_EXTRA2));
		}

		Map *area = GetCurrentArea();
		if (area) {
			ieDword value = 0;
			area->locals->Lookup(varname, value);
			// i am guessing that we shouldn't decrease below 0
			if (value > 0) {
				area->locals->SetAt(varname, value - 1);
			}
		}
	}

	//a plot critical creature has died (iwd2)
	//FIXME: BG2 uses the same field for special creatures (alternate melee damage)
	if (BaseStats[IE_MC_FLAGS]&MC_PLOT_CRITICAL) {
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
		SetupQuickSlot(i + ACT_WEAPON1, inventory.GetWeaponSlot(i), 0);
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
	GameControl *gc = core->GetGameControl();
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
	if (core->HasFeature(GF_HAS_KAPUTZ)) {
		const char *format = AppearanceFlags & APP_ADDKILL ? "KILL_%s" : "%s";

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
			IncrementDeathVariable(game->kaputz, "%s_DEAD", scriptName);
		}

	} else {
		IncrementDeathVariable(game->locals, "%s", KillVar);
		IncrementDeathVariable(game->locals, core->GetDeathVarFormat(), scriptName);
	}

	IncrementDeathVariable(game->locals, "%s", IncKillVar);

	ieDword value;
	if (scriptName[0] && SetDeathVar) {
		ieVariable varname;
		value = 0;
		size_t len = snprintf(varname, 32, "%s_DEAD", scriptName);
		game->locals->Lookup(varname, value);
		game->locals->SetAt(varname, 1, nocreate);
		if (len > 32) {
			Log(ERROR, "Actor", "Scriptname %s (name: %s) is too long for generating death globals!", scriptName, LongName);
		}
		if (value) {
			IncrementDeathVariable(game->locals, "%s_KILL_CNT", scriptName, 1);
		}
	}

	if (IncKillCount) {
		// racial dead count
		int racetable = core->LoadSymbol("race");
		if (racetable != -1) {
			Holder<SymbolMgr> race = core->GetSymbol(racetable);
			IncrementDeathVariable(game->locals, "KILL_%s_CNT", race->GetValue(Modified[IE_RACE]));
		}
	}

	//death counters for PST
	for (int i = 0, j = APP_GOOD; i < 4; i++) {
		if (AppearanceFlags & j) {
			value = 0;
			game->locals->Lookup(CounterNames[i], value);
			game->locals->SetAt(CounterNames[i], value + DeathCounters[i], nocreate);
		}
		j += j;
	}

	if (disintegrated) return true;

	// party actors are never removed
	if (Persistent()) {
		// hide the corpse artificially
		SetBase(IE_AVATARREMOVAL, 1);
		return false;
	}

	//TODO: verify removal times
	ieDword time = core->GetGame()->GameTime;
	if (!pstflags && Modified[IE_MC_FLAGS]&MC_REMOVE_CORPSE) {
		RemovalTime = time;
		return true;
	}
	if (Modified[IE_MC_FLAGS]&MC_KEEP_CORPSE) return false;
	RemovalTime = time + core->Time.day_size; // keep corpse around for a day

	//if chunked death, then return true
	ieDword gore = 0;
	core->GetDictionary()->Lookup("Gore", gore);
	if (LastDamageType & DAMAGE_CHUNKING) {
		if (gore) {
			// TODO: play chunky animation #128
			// chunks are projectiles
		}
		RemovalTime = time;
		return true;
	}
	return false;
}

ieDword Actor::IncrementDeathVariable(Variables *vars, const char *format, const char *name, ieDword start) const {
	if (name && name[0]) {
		ieVariable varname;
		size_t len = snprintf(varname, 32, format, name);
		vars->Lookup(varname, start);
		vars->SetAt(varname, start + 1, nocreate);
		if (len > 32) {
			Log(ERROR, "Actor", "Scriptname %s (name: %s) is too long for generating death globals!", name, LongName);
		}
	}
	return start;
}

/* this will create a heap at location, and transfer the item(s) */
void Actor::DropItem(const ieResRef resref, unsigned int flags)
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
void Actor::GetItemSlotInfo(ItemExtHeader *item, int which, int header)
{
	ieWord idx;
	ieWord headerindex;

	memset(item, 0, sizeof(ItemExtHeader) );
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
	Item *itm = gamedata->GetItem(slot->ItemResRef, true);
	if (!itm) {
		Log(WARNING, "Actor", "Invalid quick slot item: %s!", slot->ItemResRef);
		return; //quick item slot contains invalid item resref
	}
	ITMExtHeader *ext_header = itm->GetExtHeader(headerindex);
	//item has no extended header, or header index is wrong
	if (!ext_header) return;
	memcpy(item->itemname, slot->ItemResRef, sizeof(ieResRef) );
	item->slot = idx;
	item->headerindex = headerindex;
	memcpy(&(item->AttackType), &(ext_header->AttackType),
		((char *) &(item->itemname)) -((char *) &(item->AttackType)) );
	if (headerindex>=CHARGE_COUNTERS) {
		item->Charges=0;
	} else {
		item->Charges=slot->Usages[headerindex];
	}
	gamedata->FreeItem(itm,slot->ItemResRef, false);
}

void Actor::ReinitQuickSlots()
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
		int which = IWD2GemrbQslot(i);

		switch (which) {
			case ACT_WEAPON1:
			case ACT_WEAPON2:
			case ACT_WEAPON3:
			case ACT_WEAPON4:
				CheckWeaponQuickSlot(which-ACT_WEAPON1);
				slot = 0;
				break;
				//WARNING:this cannot be condensed, because the symbols don't come in order!!!
			case ACT_QSLOT1: slot = inventory.GetQuickSlot(); break;
			case ACT_QSLOT2: slot = inventory.GetQuickSlot()+1; break;
			case ACT_QSLOT3: slot = inventory.GetQuickSlot()+2; break;
			case ACT_QSLOT4: slot = inventory.GetQuickSlot()+3; break;
			case ACT_QSLOT5: slot = inventory.GetQuickSlot()+4; break;
			case ACT_IWDQITEM: slot = inventory.GetQuickSlot(); break;
			case ACT_IWDQITEM+1: slot = inventory.GetQuickSlot()+1; break;
			case ACT_IWDQITEM+2: slot = inventory.GetQuickSlot()+2; break;
			case ACT_IWDQITEM+3: slot = inventory.GetQuickSlot()+3; break;
			case ACT_IWDQITEM+4: slot = inventory.GetQuickSlot()+4; break;
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

		if (!inventory.HasItemInSlot("", slot)) {
			SetupQuickSlot(which, 0xffff, 0xffff);
		} else {
			ieWord idx;
			ieWord headerindex;
			PCStats->GetSlotAndIndex(which,idx,headerindex);
			if (idx != slot || headerindex == 0xffff) {
				// If slot just became filled, set it to filled
				SetupQuickSlot(which,slot,0);
			}
		}
	}

	//these are always present
	CheckWeaponQuickSlot(0);
	CheckWeaponQuickSlot(1);
	if (weapSlotCount > 2) {
		for(i=2; i<weapSlotCount; i++) {
			CheckWeaponQuickSlot(i);
		}
	} else {
	//disabling quick weapon slots for certain classes
		for(i=0;i<2;i++) {
			int which = ACT_WEAPON3+i;
			// Assuming that ACT_WEAPON3 and 4 are always in the first two spots
			if (PCStats->QSlots[i+3]!=which) {
				SetupQuickSlot(which, 0xffff, 0xffff);
			}
		}
	}
}

void Actor::CheckWeaponQuickSlot(unsigned int which)
{
	if (!PCStats) return;

	bool empty = false;
	// If current quickweaponslot doesn't contain an item, reset it to fist
	int slot = PCStats->QuickWeaponSlots[which];
	int header = PCStats->QuickWeaponHeaders[which];
	if (!inventory.HasItemInSlot("", slot) || header == 0xffff) {
		//a quiver just went dry, falling back to fist
		empty = true;
	} else {
		// If current quickweaponslot contains ammo, and bow not found, reset

		if (core->QuerySlotEffects(slot) == SLOT_EFFECT_MISSILE) {
			const CREItem *slotitm = inventory.GetSlotItem(slot);
			assert(slotitm);
			Item *itm = gamedata->GetItem(slotitm->ItemResRef, true);
			assert(itm);
			ITMExtHeader *ext_header = itm->GetExtHeader(header);
			if (ext_header) {
				int type = ext_header->ProjectileQualifier;
				int weaponslot = inventory.FindTypedRangedWeapon(type);
				if (weaponslot == inventory.GetFistSlot()) {
					empty = true;
				}
			} else {
				empty = true;
			}
			gamedata->FreeItem(itm,slotitm->ItemResRef, false);
		}
	}

	if (empty)
		SetupQuickSlot(ACT_WEAPON1+which, inventory.GetFistSlot(), 0);
}

//if dual stuff needs to be handled on load too, improve this method with it
int Actor::GetHpAdjustment(int multiplier, bool modified) const
{
	int val;

	// only player classes get this bonus
	if (!HasPlayerClass()) {
		return 0;
	}

	const ieDword *stats;
	if (modified) {
		stats = Modified;
	} else {
		stats = BaseStats;
	}

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

	ieWord animID = ( ieWord ) BaseStats[IE_ANIMATION_ID];
	//this is required so the actor has animation already
	SetAnimationID( animID );

	// Setting up derived stats
	if (BaseStats[IE_STATE_ID] & STATE_DEAD) {
		SetStance( IE_ANI_TWITCH );
		Deactivate();
		InternalFlags|=IF_REALLYDIED;
	} else {
		if (BaseStats[IE_STATE_ID] & STATE_SLEEP) {
			SetStance( IE_ANI_SLEEP );
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
	memcpy(Modified, BaseStats, sizeof(Modified));
}

//most feats are simulated via spells (feat<xx>)
void Actor::ApplyFeats()
{
	ieResRef feat;

	for(int i=0;i<MAX_FEATS;i++) {
		int level = GetFeat(i);
		snprintf(feat, sizeof(ieResRef), "FEAT%02x", i);
		if (level) {
			if (gamedata->Exists(feat, IE_SPL_CLASS_ID, true)) {
				core->ApplySpell(feat, this, this, level);
			}
		}
	}
	//apply scripted feats
	if (InParty) {
		core->GetGUIScriptEngine()->RunFunction("LUCommon","ApplyFeats", true, InParty);
	} else {
		core->GetGUIScriptEngine()->RunFunction("LUCommon","ApplyFeats", true, GetGlobalID());
	}
}

void Actor::ApplyExtraSettings()
{
	if (!PCStats) return;
	for (int i=0;i<ES_COUNT;i++) {
		if (featspells[i][0] && featspells[i][0] != '*') {
			if (PCStats->ExtraSettings[i]) {
				core->ApplySpell(featspells[i], this, this, PCStats->ExtraSettings[i]);
			}
		}
	}
}

void Actor::SetupQuickSlot(unsigned int which, int slot, int headerindex)
{
	if (!PCStats) return;
	PCStats->InitQuickSlot(which, slot, headerindex);
	//something changed about the quick items
	core->SetEventFlag(EF_ACTION);
}

bool Actor::ValidTarget(int ga_flags, const Scriptable *checker) const
{
	//scripts can still see this type of actor

	if (ga_flags&GA_NO_SELF) {
		if (checker && checker == this) return false;
	}

	if (ga_flags&GA_NO_UNSCHEDULED) {
		if (Modified[IE_AVATARREMOVAL]) return false;

		Game *game = core->GetGame();
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
	int RowNum = anims->AvatarsRowNum - 1;
	if (RowNum<0)
		RowNum = CharAnimations::GetAvatarsCount() - 1;
	int NewAnimID = CharAnimations::GetAvatarStruct(RowNum)->AnimID;
	print("AnimID: %04X", NewAnimID);
	SetBase( IE_ANIMATION_ID, NewAnimID);
}

void Actor::GetPrevAnimation()
{
	int RowNum = anims->AvatarsRowNum + 1;
	if (RowNum>=CharAnimations::GetAvatarsCount() )
		RowNum = 0;
	int NewAnimID = CharAnimations::GetAvatarStruct(RowNum)->AnimID;
	print("AnimID: %04X", NewAnimID);
	SetBase( IE_ANIMATION_ID, NewAnimID);
}

//slot is the projectile slot
//This will return the projectile item.
ITMExtHeader *Actor::GetRangedWeapon(WeaponInfo &wi) const
{
//EquippedSlot is the projectile. To get the weapon, use inventory.GetUsedWeapon()
	wi.slot = inventory.GetEquippedSlot();
	const CREItem *wield = inventory.GetSlotItem(wi.slot);
	if (!wield) {
		return NULL;
	}
	Item *item = gamedata->GetItem(wield->ItemResRef, true);
	if (!item) {
		Log(WARNING, "Actor", "Missing or invalid ranged weapon item: %s!", wield->ItemResRef);
		return NULL;
	}
	//The magic of the bow and the arrow do not add up
	if (item->Enchantment > wi.enchantment) {
		wi.enchantment = item->Enchantment;
	}
	wi.itemflags = wield->Flags;
	//not resetting wi.itemtype, since we want it to remain the one of the launcher
	//wi.range is not set, the projectile has no effect on range?

	ITMExtHeader *which = item->GetWeaponHeader(true);
	gamedata->FreeItem(item, wield->ItemResRef, false);
	return which;
}

int Actor::IsDualWielding() const
{
	int slot;
	//if the shield slot is a weapon, we're dual wielding
	const CREItem *wield = inventory.GetUsedWeapon(true, slot);
	if (!wield || slot == inventory.GetFistSlot() || slot == inventory.GetMagicSlot()) {
		return 0;
	}

	Item *itm = gamedata->GetItem(wield->ItemResRef, true);
	if (!itm) {
		Log(WARNING, "Actor", "Missing or invalid wielded weapon item: %s!", wield->ItemResRef);
		return 0;
	}

	//if the item is usable in weapon slot, then it is weapon
	int weapon = core->CanUseItemType( SLOT_WEAPON, itm );
	gamedata->FreeItem( itm, wield->ItemResRef, false );
	//is just weapon>0 ok?
	return (weapon>0)?1:0;
}

//returns weapon header currently used (bow in case of bow+arrow)
//if range is nonzero, then the returned header is valid
ITMExtHeader *Actor::GetWeapon(WeaponInfo &wi, bool leftorright) const
{
	//only use the shield slot if we are dual wielding
	leftorright = leftorright && IsDualWielding();

	const CREItem *wield = inventory.GetUsedWeapon(leftorright, wi.slot);
	if (!wield) {
		return 0;
	}
	Item *item = gamedata->GetItem(wield->ItemResRef, true);
	if (!item) {
		Log(WARNING, "Actor", "Missing or invalid weapon item: %s!", wield->ItemResRef);
		return 0;
	}

	wi.enchantment = item->Enchantment;
	wi.itemtype = item->ItemType;
	wi.itemflags = wield->Flags;
	wi.prof = item->WeaProf;
	wi.critmulti = core->GetCriticalMultiplier(item->ItemType);
	wi.critrange = core->GetCriticalRange(item->ItemType);

	//select first weapon header
	// except you can never dualwield two ranged (thrown) weapons
	ITMExtHeader *which;
	if (!leftorright && GetAttackStyle() == WEAPON_RANGED) {
		which = item->GetWeaponHeader(true);
		if (which) {
			wi.backstabbing = which->RechargeFlags & IE_ITEM_BACKSTAB;
		} else {
			wi.backstabbing = false;
		}
		wi.wflags |= WEAPON_RANGED;
	} else {
		which = item->GetWeaponHeader(false);
		// any melee weapon usable by a single class thief is game (UAI does not affect this)
		// but also check a bit in the recharge flags (modder extension)
		if (which) {
			wi.backstabbing = !(item->UsabilityBitmask & 0x400000) || (which->RechargeFlags & IE_ITEM_BACKSTAB);
		} else {
			wi.backstabbing = !(item->UsabilityBitmask & 0x400000);
		}
		if (third) {
			// iwd2 doesn't set the usability mask
			wi.backstabbing = true;
		}
	}

	if (which && (which->RechargeFlags&IE_ITEM_KEEN)) {
		//this is correct, the threat range is only increased by one in the original engine
		wi.critrange--;
	}

	//make sure we use 'false' in this freeitem
	//so 'which' won't point into invalid memory
	gamedata->FreeItem(item, wield->ItemResRef, false);
	if (!which) {
		return 0;
	}
	if (which->Location!=ITEM_LOC_WEAPON) {
		return 0;
	}
	wi.range = which->Range+1;
	return which;
}

void Actor::GetNextStance()
{
	static int Stance = IE_ANI_AWAKE;

	if (--Stance < 0) Stance = MAX_ANIMS-1;
	print("StanceID: %d", Stance);
	SetStance( Stance );
}

int Actor::LearnSpell(const ieResRef spellname, ieDword flags, int bookmask, int level)
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
	int tmp = spell->SpellName;
	if (flags&LS_LEARN) {
		core->GetTokenDictionary()->SetAt("SPECIALABILITYNAME", core->GetCString(tmp));
		switch (spell->SpellType) {
		case IE_SPL_INNATE:
			tmp = STR_GOTABILITY;
			break;
		case IE_SPL_SONG:
			tmp = STR_GOTSONG;
			break;
		default:
			tmp = STR_GOTSPELL;
			break;
		}
	} else tmp = 0;
	gamedata->FreeSpell(spell, spellname, false);
	if (!explev) {
		return LSR_INVALID;
	}
	if (tmp) {
		displaymsg->DisplayConstantStringName(tmp, DMC_BG2XPGREEN, this);
	}
	if (flags&LS_ADDXP && !(flags&LS_NOXP)) {
		int xp = CalculateExperience(XP_LEARNSPELL, explev);
		const Game *game = core->GetGame();
		game->ShareXP(xp, SX_DIVIDE);
	}
	return LSR_OK;
}

void Actor::SetDialog(const ieResRef resref)
{
	CopyResRef(Dialog, resref);
}

const char *Actor::GetDialog(int flags) const
{
	if (!flags) {
		return Dialog;
	}
	if (Modified[IE_EA]>=EA_EVILCUTOFF) {
		return NULL;
	}

	if ( (InternalFlags & IF_NOINT) && CurrentAction) {
		if (flags>1) {
			core->GetTokenDictionary()->SetAtCopy("TARGET", ShortName);
			displaymsg->DisplayConstantString(STR_TARGETBUSY, DMC_RED);
		}
		return NULL;
	}
	return Dialog;
}

std::list<int> Actor::ListLevels() const
{
	std::list<int> levels (ISCLASSES, 0);
	if (third) {
		std::list<int>::iterator it;
		int i = 0;
		for (it=levels.begin(); it != levels.end(); it++) {
			*it = GetClassLevel(i++);
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

const char* Actor::GetScript(int ScriptIndex) const
{
	if (Scripts[ScriptIndex]) {
		return Scripts[ScriptIndex]->GetName();
	} else {
		return "NONE\0\0\0\0";
	}
}

void Actor::SetModal(ieDword newstate, bool force)
{
	switch(newstate) {
		case MS_NONE:
			break;
		case MS_BATTLESONG:
			break;
		case MS_DETECTTRAPS:
			break;
		case MS_STEALTH:
			break;
		case MS_TURNUNDEAD:
			break;
		default:
			return;
	}

	if (Modal.State != newstate) {
		Modal.FirstApply = 1;
	}

	if (Modal.State == MS_BATTLESONG && Modal.State != newstate && HasFeat(FEAT_LINGERING_SONG)) {
		strnlwrcpy(Modal.LingeringSpell, Modal.Spell, 8);
		Modal.LingeringCount = 2;
	}

	if (IsSelected()) {
		// display the turning-off message
		if (Modal.State != MS_NONE && core->HasFeedback(FT_MISC)) {
			displaymsg->DisplayStringName(ModalStates[Modal.State].leaving_str, DMC_WHITE, this, IE_STR_SOUND|IE_STR_SPEECH);
		}

		//update the action bar
		if (Modal.State != newstate || newstate != MS_NONE)
			core->SetEventFlag(EF_ACTION);

		// when called with the same state twice, toggle to MS_NONE
		if (!force && Modal.State == newstate) {
			Modal.State = MS_NONE;
		} else {
			Modal.State = newstate;
		}
	} else {
		Modal.State = newstate;
	}
}

void Actor::SetModalSpell(ieDword state, const char *spell)
{
	if (spell) {
		strnlwrcpy(Modal.Spell, spell, 8);
	} else {
		if (state >= ModalStates.size()) {
			Modal.Spell[0] = 0;
		} else {
			if (state==MS_BATTLESONG) {
				if (BardSong[0]) {
					strnlwrcpy(Modal.Spell, BardSong, 8);
					return;
				}
			}
			strnlwrcpy(Modal.Spell, ModalStates[state].spell, 8);
		}
	}
}

//even spells got this attack style
int Actor::GetAttackStyle() const
{
	WeaponInfo wi;
	// Some weapons have both melee and ranged capability, eg. bg2's rifthorne (ax1h09)
	// so we check the equipped header's attack type: 2-projectile and 4-launcher
	// It is more complicated than it seems because the equipped header is the one of the projectile for launchers
	ITMExtHeader *rangedheader = GetRangedWeapon(wi);
	if (!PCStats) {
		// fall back to simpler logic that works most of the time
		//Non NULL if the equipped slot is a projectile or a throwing weapon
		if (rangedheader) return WEAPON_RANGED;
		return WEAPON_MELEE;
	}

	ITMExtHeader *eh;
	if (inventory.MagicSlotEquipped()) {
		// this should be fine, as long as we default to melee,
		// since there are no "magic" weapons with switchable headers
		eh = rangedheader;
	} else {
		int qh = PCStats->GetHeaderForSlot(inventory.GetEquippedSlot());
		eh = inventory.GetEquippedExtHeader(qh);
	}
	if (!eh) return WEAPON_MELEE; // default to melee
	if (eh->AttackType && eh->AttackType%2 == 0) return WEAPON_RANGED;
	return WEAPON_MELEE;
}

void Actor::AttackedBy( Actor *attacker)
{
	AddTrigger(TriggerEntry(trigger_attackedby, attacker->GetGlobalID()));
	if (attacker->GetStat(IE_EA) != EA_PC && Modified[IE_EA] != EA_PC) {
		LastAttacker = attacker->GetGlobalID();
	}
	if (InParty) {
		core->Autopause(AP_ATTACKED, this);
	}
}

void Actor::FaceTarget( Scriptable *target)
{
	if (!target) return;
	SetOrientation( GetOrient( target->Pos, Pos ), false );
}

//in case of LastTarget = 0
void Actor::StopAttack()
{
	SetStance(IE_ANI_READY);
	lastattack = 0;
	secondround = 0;
	//InternalFlags|=IF_TARGETGONE; //this is for the trigger!
	if (InParty) {
		core->Autopause(AP_NOTARGET, this);
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

void Actor::DoStep(unsigned int walkScale, ieDword time)
{
	if (Immobile()) {
		return;
	}
	Movable::DoStep(walkScale, time);
}

ieDword Actor::GetNumberOfAttacks()
{
	int bonus = 0;

	if (third) {
		int base = SetBaseAPRandAB (true);
		// add the offhand extra attack
		// TODO: check effects too
		bonus = 2 * IsDualWielding();
		return base + bonus;
	} else {
		if (monkbon != NULL && inventory.FistsEquipped()) {
			unsigned int level = GetMonkLevel();
			if (level>=monkbon_cols) level=monkbon_cols-1;
			if (level>0) {
				bonus = monkbon[0][level-1];
			}
		}

		return GetStat(IE_NUMBEROFATTACKS)+bonus;
	}
}
static const int BaseAttackBonusDecrement = 5; // iwd2; number of tohit points for another attack per round
static int SetLevelBAB(int level, ieDword index)
{
	if (!level) {
		return 0;
	}
	assert(index < BABClassMap.size());

	IWD2HitTableIter table = IWD2HitTable.find(BABClassMap[index]);
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
	int i;

	if (!third) {
		ToHit.SetBase(BaseStats[IE_TOHIT]);
		return 0;
	}

	for (i = 0; i<ISCLASSES; i++) {
		int level = GetClassLevel(i);
		if (level) {
			// silly monks, always wanting to be special
			if (i == ISMONK) {
				MonkLevel = level;
				if (MonkLevel+LevelSum == Modified[IE_CLASSLEVELSUM]) {
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
	if (CheckRapidShot && HasSpellState(SS_RAPIDSHOT)) {
		WeaponInfo wi;
		ITMExtHeader *HittingHeader = GetRangedWeapon(wi);
		if (HittingHeader) {
			ieDword AttackTypeLowBits = HittingHeader->AttackType & 0xFF; // this is done by the original; leaving in case we expand
			if (AttackTypeLowBits == ITEM_AT_BOW || AttackTypeLowBits == ITEM_AT_PROJECTILE) {
				// rapid shot gives another attack and since it is computed from the BAB, we just increase that ...
				// but monk get their speedy handy work only for fists, so we can't use the passed pBABDecrement
				pBAB += BaseAttackBonusDecrement;
			}
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
	Log(MESSAGE, "InitRound", "Name: %s | Attacks: %d | Start: %d",
		ShortName, attacksperround, gameTime);

	// this might not be the right place, but let's give it a go
	if (attacksperround && InParty) {
		core->Autopause(AP_ENDROUND, this);
	}
}

// a simplified check from GetCombatDetails for use in AttackCore
bool Actor::WeaponIsUsable(bool leftorright, ITMExtHeader *header) const
{
	WeaponInfo wi;
	if (!header) {
		header = GetWeapon(wi, leftorright && IsDualWielding());
		if (!header) {
			return false;
		}
	}
	ITMExtHeader *rangedheader;
	switch(header->AttackType) {
		case ITEM_AT_MELEE:
		case ITEM_AT_PROJECTILE: //throwing weapon
			break;
		case ITEM_AT_BOW:
			rangedheader = GetRangedWeapon(wi);
			if (!rangedheader) {
				return false;
			}
			break;
		default:
			//item is unsuitable for fight
			return false;
	}
	return true;
}

bool Actor::GetCombatDetails(int &tohit, bool leftorright, WeaponInfo& wi, ITMExtHeader *&header, ITMExtHeader *&hittingheader, \
		int &DamageBonus, int &speed, int &CriticalBonus, int &style, const Actor *target)
{
	SetBaseAPRandAB(true);
	speed = -(int)GetStat(IE_PHYSICALSPEED);
	ieDword dualwielding = IsDualWielding();
	header = GetWeapon(wi, leftorright && dualwielding);
	if (!header) {
		return false;
	}
	style = 0;
	CriticalBonus = 0;
	hittingheader = header;
	ITMExtHeader *rangedheader = NULL;
	int THAC0Bonus = hittingheader->THAC0Bonus;
	DamageBonus = hittingheader->DamageBonus;

	switch(hittingheader->AttackType) {
	case ITEM_AT_MELEE:
		wi.wflags = WEAPON_MELEE;
		break;
	case ITEM_AT_PROJECTILE: //throwing weapon
		wi.wflags = WEAPON_RANGED;
		break;
	case ITEM_AT_BOW:
		rangedheader = GetRangedWeapon(wi);
		if (!rangedheader) {
			//display out of ammo verbal constant if there were any
			//VerbalConstant(VB_OUTOFAMMO); // FUTURE: gemrb extension
			//SetStance(IE_ANI_READY);
			//set some trigger?
			return false;
		}
		wi.wflags = WEAPON_RANGED;
		wi.launcherdmgbon = DamageBonus; // save the original (launcher) bonus
		//The bow can give some bonuses, but the core attack is made by the arrow.
		hittingheader = rangedheader;
		THAC0Bonus += rangedheader->THAC0Bonus;
		DamageBonus += rangedheader->DamageBonus;
		break;
	default:
		//item is unsuitable for fight
		wi.wflags = 0;
		return false;
	}//melee or ranged
	if (ReverseToHit) THAC0Bonus = -THAC0Bonus;
	ToHit.SetWeaponBonus(THAC0Bonus);

	//TODO easier copying of recharge flags into wflags
	//this flag is set by the bow in case of projectile launcher.
	if (header->RechargeFlags&IE_ITEM_USESTRENGTH) wi.wflags|=WEAPON_USESTRENGTH;
	if (header->RechargeFlags&IE_ITEM_USESTRENGTH_DMG) wi.wflags |= WEAPON_USESTRENGTH_DMG;
	if (header->RechargeFlags&IE_ITEM_USESTRENGTH_HIT) wi.wflags |= WEAPON_USESTRENGTH_HIT;
	// this flag is set in dagger/shortsword by the loader
	if (header->RechargeFlags&IE_ITEM_USEDEXTERITY) wi.wflags|=WEAPON_FINESSE;
	//also copy these flags (they match their WEAPON_ counterparts)
	wi.wflags|=header->RechargeFlags&(IE_ITEM_KEEN|IE_ITEM_BYPASS);

	if (wi.wflags & WEAPON_RANGED && (hittingheader->RechargeFlags&IE_ITEM_KEEN)) {
		// all the previous checks were against the launcher, but ammo can be keen too (00arow87)
		wi.critrange--;
	}

	// get our dual wielding modifier
	if (dualwielding) {
		if (leftorright) {
			DamageBonus += GetStat(IE_DAMAGEBONUSLEFT);
		} else {
			DamageBonus += GetStat(IE_DAMAGEBONUSRIGHT);
		}
	}
	DamageBonus += GetStat(IE_DAMAGEBONUS);
	leftorright = leftorright && dualwielding;
	if (leftorright) wi.wflags|=WEAPON_LEFTHAND;

	//add in proficiency bonuses
	ieDword stars = GetProficiency(wi.prof)&PROFS_MASK;

	//tenser's transformation makes the actor proficient in any weapons
	// also conjured weapons are wielded without penalties
	if (!stars && (HasSpellState(SS_TENSER) || inventory.MagicSlotEquipped())) {
		stars = 1;
	}

	//hit/damage/speed bonuses from wspecial (with tohit inverted in adnd)
	static ieDword wspecialMax = wspecial->GetRowCount() - 1;
	if (stars > wspecialMax) {
		stars = wspecialMax;
	}

	int prof = 0;
	// iwd2 adds a -4 nonprof penalty (others below, since their table is bad and actual values differ by class)
	// but everyone is proficient with fists
	// cheesily limited to party only (10gob hits it - practically can't hit you otherwise)
	if (InParty && !inventory.FistsEquipped()) {
		prof += atoi(wspecial->QueryField(stars, 0));
	}

	wi.profdmgbon = atoi(wspecial->QueryField(stars, 1));
	DamageBonus += wi.profdmgbon;
	// only bg2 wspecial.2da has this column, but all have 0 as the default table value, so this lookup is fine
	speed += atoi(wspecial->QueryField(stars, 2));
	// add non-proficiency penalty, which is missing from the table in non-iwd2
	// stored negative
	if (stars == 0 && !third) {
		ieDword clss = GetActiveClass();
		//Is it a PC class?
		if (clss < (ieDword) classcount) {
			// but skip fists, since they don't have a proficiency
			if (!inventory.FistsEquipped()) {
				prof += defaultprof[clss];
			}
		} else {
			//it is not clear what is the penalty for non player classes
			prof -= 4;
		}
	}

	if (dualwielding && wsdualwield) {
		//add dual wielding penalty
		stars = GetStat(IE_PROFICIENCY2WEAPON)&PROFS_MASK;
		if (stars > STYLE_MAX) stars = STYLE_MAX;

		style = 1000*stars + IE_PROFICIENCY2WEAPON;
		prof += wsdualwield[stars][leftorright?1:0];
	} else if (wi.itemflags&(IE_INV_ITEM_TWOHANDED) && (wi.wflags&WEAPON_MELEE) && wstwohanded) {
		//add two handed profs bonus
		stars = GetStat(IE_PROFICIENCY2HANDED)&PROFS_MASK;
		if (stars > STYLE_MAX) stars = STYLE_MAX;

		style = 1000*stars + IE_PROFICIENCY2HANDED;
		DamageBonus += wstwohanded[stars][0];
		CriticalBonus = wstwohanded[stars][1];
		speed += wstwohanded[stars][2];
	} else if (wi.wflags&WEAPON_MELEE) {
		int slot;
		CREItem *weapon = inventory.GetUsedWeapon(true, slot);
		if(wssingle && weapon == NULL) {
			//NULL return from GetUsedWeapon means no shield slot
			stars = GetStat(IE_PROFICIENCYSINGLEWEAPON)&PROFS_MASK;
			if (stars > STYLE_MAX) stars = STYLE_MAX;

			style = 1000*stars + IE_PROFICIENCYSINGLEWEAPON;
			CriticalBonus = wssingle[stars][1];
		} else if (wsswordshield && weapon) {
			stars = GetStat(IE_PROFICIENCYSWORDANDSHIELD)&PROFS_MASK;
			if (stars > STYLE_MAX) stars = STYLE_MAX;

			style = 1000*stars + IE_PROFICIENCYSWORDANDSHIELD;
		} else {
			// no bonus
		}
	} else {
		// ranged - no bonus
	}

	// racial enemies suffer 4hp more in all games
	int favoredEnemy = GetRacialEnemyBonus(target);
	if (GetRangerLevel() && favoredEnemy) {
		DamageBonus += favoredEnemy;
	}

	// Elves get a racial THAC0 bonus with swords and bows, halflings with slings
	if (raceID2Name.count(BaseStats[IE_RACE])) {
		const char *raceName = raceID2Name[BaseStats[IE_RACE]];
		prof += gamedata->GetRacialTHAC0Bonus(wi.prof, raceName);
	}

	if (third) {
		// iwd2 gives a dualwielding bonus when using a simple weapon in the offhand
		// it is limited to shortswords and daggers, which also have this flag set
		// the bonus is applied to both hands
		if (dualwielding) {
			if (leftorright) {
				if (wi.wflags&WEAPON_FINESSE) {
					prof += 2;
				}
			} else {
				// lookup the offhand
				ITMExtHeader *header2;
				WeaponInfo wi2;
				header2 = GetWeapon(wi2, true);
				if (header2->RechargeFlags&IE_ITEM_USEDEXTERITY) { // identical to the WEAPON_FINESSE check
					prof += 2;
				}
			}
		}
	} else {
		prof = -prof;
	}
	ToHit.SetProficiencyBonus(prof);

	// get the remaining boni
	// FIXME: merge
	tohit = GetToHit(wi.wflags, target);

	//pst increased critical hits
	if (pstflags && (Modified[IE_STATE_ID]&STATE_CRIT_ENH)) {
		CriticalBonus--;
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
	int prof = 0;
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
		if (third) {
			// rangers wearing light or no armor gain ambidexterity and
			//  two-weapon-fighting feats for free
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
			// +2 main, +2 off with a simple weapons in the off hand (handled in GetCombatDetails)
			if (twoWeaponFighting) {
				prof += 2;
			}
			if (Flags&WEAPON_LEFTHAND) {
				prof -= 6;
			} else {
				prof -= 10;
				if (ambidextrous) {
					prof += 4;
				}
			}
		}
	}
	ToHit.SetProficiencyBonus(ToHit.GetProficiencyBonus()+prof);

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
		if ((Flags&WEAPON_STYLEMASK) != WEAPON_RANGED) {
			if (target->GetAttackStyle() == WEAPON_RANGED) {
				generic += 4;
			}
		}

		// melee vs. unarmed
		generic += target->MeleePenalty() - MeleePenalty();

		// add +4 attack bonus vs racial enemies
		if (GetRangerLevel()) {
			generic += GetRacialEnemyBonus(target);
		}
		generic += fxqueue.BonusAgainstCreature(fx_tohit_vs_creature_ref, target);
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
				// weapon finesse is not cummulative
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
	if (!IsDualWielding() && wssingle && wsswordshield) {
		WeaponInfo wi;
		ITMExtHeader* header;
		header = GetWeapon(wi, false);
		//make sure we're wielding a single melee weapon
		if (header && (header->AttackType == ITEM_AT_MELEE)) {
			int slot;
			ieDword stars;
			if (inventory.GetUsedWeapon(true, slot) == NULL) {
				//single-weapon style applies to all ac
				stars = GetStat(IE_PROFICIENCYSINGLEWEAPON)&PROFS_MASK;
				if (stars>STYLE_MAX) stars = STYLE_MAX;
				defense += wssingle[stars][0];
			} else if (weapon_damagetype[DamageType] == DAMAGE_MISSILE) {
				//sword-shield style applies only to missile ac
				stars = GetStat(IE_PROFICIENCYSWORDANDSHIELD)&PROFS_MASK;
				if (stars>STYLE_MAX) stars = STYLE_MAX;
				defense += wsswordshield[stars][0];
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

void Actor::PerformAttack(ieDword gameTime)
{
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

	if (!LastTarget) {
		Log(ERROR, "Actor", "Attack without valid target ID!");
		return;
	}
	//get target
	Actor *target = area->GetActorByGlobalID(LastTarget);
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

	Log(DEBUG, "Actor", "Performattack for %s, target is: %s", ShortName, target->ShortName);

	//which hand is used
	//we do apr - attacksleft so we always use the main hand first
	// however, in 3ed, only one attack can be made by the offhand
	bool leftorright;
	if (third) {
		leftorright = false;
		// make only the last attack with the offhand (iwd2)
		if (attackcount == 1 && IsDualWielding()) {
			leftorright = true;
		}
	} else {
		leftorright = (bool) ((attacksperround-attackcount)&1);
	}

	WeaponInfo wi;
	ITMExtHeader *header = NULL;
	ITMExtHeader *hittingheader = NULL;
	int tohit;
	int DamageBonus, CriticalBonus;
	int speed, style;

	//will return false on any errors (eg, unusable weapon)
	if (!GetCombatDetails(tohit, leftorright, wi, header, hittingheader, DamageBonus, speed, CriticalBonus, style, target)) {
		return;
	}

	if (PCStats) {
		// make a copy of wi.slot, since GetUsedWeapon can modify it
		int wislot = wi.slot;
		CREItem *slot = inventory.GetUsedWeapon(leftorright && IsDualWielding(), wislot);
		//if slot was null, then GetCombatDetails returned false
		PCStats->RegisterFavourite(slot->ItemResRef, FAV_WEAPON);
	}

	//if this is the first call of the round, we need to update next attack
	if (nextattack == 0) {
		// initiative calculation (lucky 1d6-1 + item speed + speed stat + constant):
		// speed contains the bonus from the physical speed stat and the proficiency level
		int spdfactor = hittingheader->Speed + speed;
		if (spdfactor<0) spdfactor = 0;
		// -3: k/2 in the original, hardcoded to 6; -1 for the difference in rolls - the original rolled 0-5
		spdfactor += LuckyRoll(1, 6, -4, LR_NEGATIVE);
		if (spdfactor<0) spdfactor = 0;
		if (spdfactor>10) spdfactor = 10;

		//(round_size/attacks_per_round)*(initiative) is the first delta
		nextattack = core->Time.round_size*spdfactor/(attacksperround*10) + gameTime;

		//we can still attack this round if we have a speed factor of 0
		if (nextattack > gameTime) {
			return;
		}
	}

	if (!WithinPersonalRange(this, target, GetWeaponRange(wi)) || (GetCurrentArea() != target->GetCurrentArea())) {
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

	StringBuffer buffer;
	//debug messages
	if (leftorright && IsDualWielding()) {
		buffer.append("(Off) ");
	} else {
		buffer.append("(Main) ");
	}
	if (attacksperround) {
		buffer.appendFormatted("Left: %d | ", attackcount);
		buffer.appendFormatted("Next: %d ", nextattack);
	}
	if (fxqueue.HasEffectWithParam(fx_puppetmarker_ref, 1) || fxqueue.HasEffectWithParam(fx_puppetmarker_ref, 2)) { // illusions can't hit
		ResetState();
		buffer.append("[Missed (puppet)]");
		Log(COMBAT, "Attack", buffer);
		return;
	}

	// iwd2 smite evil only lasts for one attack, but has an insane duration, so remove it manually
	if (HasSpellState(SS_SMITEEVIL)) {
		fxqueue.RemoveAllEffects(fx_smite_evil_ref);
	}

	// check for concealment first (iwd2), both our enemies' and from our phasing problems
	int concealment = (GetStat(IE_ETHEREALNESS)>>8) + (target->GetStat(IE_ETHEREALNESS) & 0x64);
	if (concealment) {
		if (LuckyRoll(1, 100, 0) < concealment) {
			// can we retry?
			if (!HasFeat(FEAT_BLIND_FIGHT) || LuckyRoll(1, 100, 0) < concealment) {
				// Missed <TARGETNAME> due to concealment.
				core->GetTokenDictionary()->SetAtCopy("TARGETNAME", target->GetName(-1));
				if (core->HasFeedback(FT_COMBAT)) displaymsg->DisplayConstantStringName(STR_CONCEALED_MISS, DMC_WHITE, this);
				buffer.append("[Concealment Miss]");
				Log(COMBAT, "Attack", buffer);
				ResetState();
				return;
			}
		}
	}

	// iwd2 rerolls to check for criticals (cf. manual page 45) - the second roll just needs to hit; on miss, it degrades to a normal hit
	// CriticalBonus is negative, it is added to the minimum roll needed for a critical hit
	// IE_CRITICALHITBONUS is positive, it is subtracted
	int roll = LuckyRoll(1, ATTACKROLL, 0, LR_CRITICAL);
	int criticalroll = roll + (int) GetStat(IE_CRITICALHITBONUS) - CriticalBonus;
	if (third) {
		int ThreatRangeMin = wi.critrange;
		ThreatRangeMin -= ((int) GetStat(IE_CRITICALHITBONUS) - CriticalBonus); // TODO: move to GetCombatDetails
		criticalroll = LuckyRoll(1, ATTACKROLL, 0, LR_CRITICAL);
		if (criticalroll < ThreatRangeMin || GetStat(IE_SPECFLAGS)&SPECF_CRITIMMUNITY) {
			// make it an ordinary hit
			criticalroll = 1;
		} else {
			// make sure it will be a critical hit
			criticalroll = ATTACKROLL;
		}
	}

	//damage type is?
	//modify defense with damage type
	ieDword damagetype = hittingheader->DamageType;
	int damage = 0;

	if (hittingheader->DiceThrown<256) {
		damage += LuckyRoll(hittingheader->DiceThrown, hittingheader->DiceSides, DamageBonus, LR_DAMAGELUCK);
		if (damage < 0) damage = 0; // bad luck, effects and/or profs on lowlevel chars
	} else {
		damage = 0;
	}

	bool critical = criticalroll>=ATTACKROLL;
	bool success = critical;
	int defense = target->GetDefense(damagetype, wi.wflags, this);
	int rollMod = (ReverseToHit) ? defense : tohit;
	if (!critical) {
		// autohit immobile enemies (true for atleast stun, sleep, timestop)
		if (target->Immobile() || (target->GetStat(IE_STATE_ID) & STATE_SLEEP)) {
			success = true;
		} else if (roll == 1) {
			success = false;
		} else {
			success = (roll + rollMod) > ((ReverseToHit) ? tohit : defense);
		}
	}

	if (target->GetStat(IE_EXTSTATE_ID) & EXTSTATE_EYE_SWORD) {
		target->fxqueue.RemoveAllEffects(fx_eye_sword_ref);
		target->spellbook.RemoveSpell(SevenEyes[EYE_SWORD]);
		target->SetBaseBit(IE_EXTSTATE_ID, EXTSTATE_EYE_SWORD, false);
		success = false;
		roll = 2; // avoid chance critical misses
	}

	if (core->HasFeedback(FT_TOHIT)) {
		// log the roll
		wchar_t rollLog[100];
		const wchar_t *fmt = L"%ls %d %ls %d = %d : %ls";
		String *leftRight, *hitMiss;
		if (leftorright && displaymsg->HasStringReference(STR_ATTACK_ROLL_L)) {
			leftRight = core->GetString(displaymsg->GetStringReference(STR_ATTACK_ROLL_L));
		} else {
			leftRight = core->GetString(displaymsg->GetStringReference(STR_ATTACK_ROLL));
		}
		if (success) {
			hitMiss = core->GetString(displaymsg->GetStringReference(STR_HIT));
		} else {
			hitMiss = core->GetString(displaymsg->GetStringReference(STR_MISS));
		}
		swprintf(rollLog, 100, fmt, leftRight->c_str(), roll, (rollMod >= 0) ? L"+" : L"-", abs(rollMod), roll + rollMod, hitMiss->c_str());
		displaymsg->DisplayStringName(rollLog, DMC_WHITE, this);
		delete leftRight;
		delete hitMiss;
	}

	if (roll == 1) {
		//critical failure
		buffer.append("[Critical Miss]");
		Log(COMBAT, "Attack", buffer);
		if (core->HasFeedback(FT_COMBAT)) displaymsg->DisplayConstantStringName(STR_CRITICAL_MISS, DMC_WHITE, this);
		VerbalConstant(VB_CRITMISS);
		if (wi.wflags & WEAPON_RANGED) {//no need for this with melee weapon!
			UseItem(wi.slot, (ieDword) -2, target, UI_MISS|UI_NOAURA);
		} else if (core->HasFeature(GF_BREAKABLE_WEAPONS) && InParty) {
			//break sword
			// a random roll on-hit (perhaps critical failure too)
			//  in 0,5% (1d20*1d10==1) cases
			if ((header->RechargeFlags & IE_ITEM_BREAKABLE) && core->Roll(1, 10, 0) == 1) {
				inventory.BreakItemSlot(wi.slot);
				inventory.EquipBestWeapon(EQUIP_MELEE);
			}
		}
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
		Log(COMBAT, "Attack", buffer);
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
		Log(COMBAT, "Attack", buffer);
		if (core->HasFeedback(FT_COMBAT)) displaymsg->DisplayConstantStringName(STR_CRITICAL_HIT, DMC_WHITE, this);
		VerbalConstant(VB_CRITHIT);
	} else {
		//normal success
		buffer.append("[Hit]");
		Log(COMBAT, "Attack", buffer);
	}
	UseItem(wi.slot, wi.wflags&WEAPON_RANGED?-2:-1, target, (critical?UI_CRITICAL:0)|UI_NOAURA, damage);
	ResetState();
}

int Actor::GetWeaponRange(const WeaponInfo &wi) const
{
	return std::min(wi.range, Modified[IE_VISUALRANGE]);
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
		Log(COMBAT, "DamageReduction", "Damage resistance (%d) is completely from damage reduction.", resisted);
		return resisted;
	}
	if (remaining == total) {
		Log(COMBAT, "DamageReduction", "No weapon enchantment breach — full damage reduction and resistance used.");
		return resisted;
	} else {
		Log(COMBAT, "DamageReduction", "Ignoring %d of %d damage reduction due to weapon enchantment breach.", total-remaining, total);
		return resisted - (total-remaining);
	}
}

/*Always call this on the suffering actor */
void Actor::ModifyDamage(Scriptable *hitter, int &damage, int &resisted, int damagetype)
{
	Actor *attacker = NULL;

	if (hitter && hitter->Type==ST_ACTOR) {
		attacker = (Actor *) hitter;
	}

	//guardian mantle for PST
	if (attacker && (Modified[IE_IMMUNITY]&IMM_GUARDIAN) ) {
		//if the hitter doesn't make the spell save, the mantle works and the damage is 0
		if (!attacker->GetSavingThrow(0,-4) ) {
			damage = 0;
			return;
		}
	}

	// only check stone skins if damage type is physical or magical
	// DAMAGE_CRUSHING is 0, so we can't AND with it to check for its presence
	if (!(damagetype & ~(DAMAGE_PIERCING|DAMAGE_SLASHING|DAMAGE_MISSILE|DAMAGE_MAGIC))) {
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
			Log(ERROR, "ModifyDamage", "Unhandled damagetype:%d", damagetype);
		} else if (it->second.resist_stat) {
			// check for bonuses for specific damage types
			if (core->HasFeature(GF_SPECIFIC_DMG_BONUS) && attacker) {
				int bonus = attacker->fxqueue.BonusForParam2(fx_damage_bonus_modifier_ref, it->second.iwd_mod_type);
				if (bonus) {
					resisted -= int (damage * bonus / 100.0);
					Log(COMBAT, "ModifyDamage", "Bonus damage of %d(%+d%%), neto: %d", int(damage * bonus / 100.0), bonus, -resisted);
				}
			}
			// damage type with a resistance stat
			if (third) {
				// flat resistance, eg. 10/- or eg. 5/+2 for physical types
				// for actors we need special care for damage reduction - traps (...) don't have enchanted weapons
				if (attacker && it->second.reduction) {
					WeaponInfo wi;
					attacker->GetWeapon(wi, 0); // FIXME: use a cheaper way to share the weaponEnchantment + this might have been the left hand
					ieDword weaponEnchantment = wi.enchantment;
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
					Log(DEBUG, "ModifyDamage", "Ignoring bad damage resistance value (%d).", resistance);
				}
				resisted += (int) (damage * resistance/100.0);
				damage -= resisted;
			}
			Log(COMBAT, "ModifyDamage", "Resisted %d of %d at %d%% resistance to %d", resisted, damage+resisted, GetSafeStat(it->second.resist_stat), damagetype);
			// PST and BG1 may actually heal on negative damage
			if (!core->HasFeature(GF_HEAL_ON_100PLUS)) {
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
				attacker->DisplayStringOrVerbalConstant(STR_WEAPONINEFFECTIVE, VB_TIMMUNE);
			}
			core->Autopause(AP_UNUSABLE, this);
		}
	}
}

void Actor::UpdateActorState(ieDword gameTime) {
	if (Modal.LastApplyTime == gameTime) {
		return;
	}

	int roundFraction = (gameTime-roundTime) % GetAdjustedTime(core->Time.round_size);

	//actually, iwd2 has autosearch, also, this is useful for dayblindness
	//apply the modal effect about every second (pst and iwds have round sizes that are not multiples of 15)
	// FIXME: split dayblindness out of detect.spl and only run that each tick + simplify this check
	if (InParty && core->HasFeature(GF_AUTOSEARCH_HIDDEN) && (third || ((roundFraction%AI_UPDATE_TIME) == 0))) {
		core->ApplySpell("detect", this, this, 0);
	}

	ieDword state = Modified[IE_STATE_ID];

	// each round also re-confuse the actor
	if (!roundFraction) {
		if (BaseStats[IE_CHECKFORBERSERK]) {
			BaseStats[IE_CHECKFORBERSERK]--;
		}
		if (state & STATE_CONFUSED) {
			const char* actionString = NULL;
			int tmp = core->Roll(1,3,0);
			switch (tmp) {
			case 2:
				actionString = "RandomWalk()";
				break;
			case 1:
				// HACK: replace with [0] (ANYONE) once we support that (Nearest matches Sender like in the original)
				if (RAND(0,1)) {
					actionString = "Attack(NearestEnemyOf(Myself))";
				} else {
					actionString = "Attack([PC])";
				}
				break;
			default:
				actionString = "NoAction()";
				break;
			}
			Action *action = GenerateAction( actionString );
			if (action) {
				ReleaseCurrentAction();
				AddActionInFront(action);
				print("Confusion: added %s at %d (%d)", actionString, gameTime-roundTime, roundTime);
			}
			return;
		}

		if (Modified[IE_CHECKFORBERSERK] && !LastTarget && SeeAnyOne(false, false) ) {
			Action *action = GenerateAction( "Berserk()" );
			if (action) {
				ReleaseCurrentAction();
				AddActionInFront(action);
			}
			return;
		}
	}

	// this is a HACK, fuzzie can't work out where else to do this for now
	// but we shouldn't be resetting rounds/attacks just because the actor
	// wandered away, the action code should probably be responsible somehow
	// see also line above (search for comment containing UpdateActorState)!
	if (LastTarget && lastattack && lastattack < (gameTime - 1)) {
		Actor *target = area->GetActorByGlobalID(LastTarget);
		if (!target || target->GetStat(IE_STATE_ID)&STATE_DEAD) {
			StopAttack();
		} else {
			Log(COMBAT, "Attack", "(Leaving attack)");
		}

		lastattack = 0;
	}

	if (Modal.State == MS_NONE && !Modal.LingeringCount) {
		return;
	}

	//apply the modal effect on the beginning of each round
	if (roundFraction == 0) {
		// handle lingering modal spells like bardsong in iwd2
		if (Modal.LingeringCount && Modal.LingeringSpell[0]) {
			Modal.LingeringCount--;
			ApplyModal(Modal.LingeringSpell);
		}
		if (Modal.State == MS_NONE) {
			return;
		}

		// some states and timestop disable modal actions
		// interestingly the original doesn't include STATE_DISABLED, STATE_FROZEN/STATE_PETRIFIED
		if (Immobile() || (state & (STATE_CONFUSED | STATE_DEAD | STATE_HELPLESS | STATE_PANIC | STATE_BERSERK | STATE_SLEEP))) {
			return;
		}

		//we can set this to 0
		Modal.LastApplyTime = gameTime;

		if (!Modal.Spell[0]) {
			Log(WARNING, "Actor", "Modal Spell Effect was not set!");
			Modal.Spell[0]='*';
		} else if (Modal.Spell[0]!='*') {
			if (ModalSpellSkillCheck()) {
				ApplyModal(Modal.Spell);

				// some modals notify each round, some only initially
				bool feedback = ModalStates[Modal.State].repeat_msg || Modal.FirstApply;
				Modal.FirstApply = 0;
				if (InParty && feedback && core->HasFeedback(FT_MISC)) {
					displaymsg->DisplayStringName(ModalStates[Modal.State].entering_str, DMC_WHITE, this, IE_STR_SOUND|IE_STR_SPEECH);
				}
			} else {
				if (InParty && core->HasFeedback(FT_MISC)) {
					displaymsg->DisplayStringName(ModalStates[Modal.State].failed_str, DMC_WHITE, this, IE_STR_SOUND|IE_STR_SPEECH);
				}
				Modal.State = MS_NONE;
			}
		}

		// shut everyone up, so they don't whine if the actor is on a long hiding-in-shadows recon mission
		core->GetGame()->ResetPartyCommentTimes();
	}

}

void Actor::ApplyModal(ieResRef modalSpell)
{
	unsigned int aoe = ModalStates[Modal.State].aoe_spell;
	if (aoe == 1) {
		core->ApplySpellPoint(modalSpell, GetCurrentArea(), Pos, this, 0);
	} else if (aoe == 2) {
		// target actors around us manually
		// used for iwd2 songs, as the spells don't use an aoe projectile
		if (!area) return;
		std::vector<Actor *> neighbours = area->GetAllActorsInRadius(Pos, GA_NO_LOS|GA_NO_DEAD|GA_NO_UNSCHEDULED, GetSafeStat(IE_VISUALRANGE)/2);
		std::vector<Actor *>::iterator neighbour;
		for (neighbour = neighbours.begin(); neighbour != neighbours.end(); ++neighbour) {
			core->ApplySpell(modalSpell, *neighbour, this, 0);
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

void Actor::SetColorMod( ieDword location, RGBModifier::Type type, int speed,
			unsigned char r, unsigned char g, unsigned char b,
			int phase) const
{
	CharAnimations* ca = GetAnims();
	if (!ca) return;

	if (location == 0xff) {
		if (phase && ca->GlobalColorMod.locked) return;
		ca->GlobalColorMod.locked = !phase;
		ca->GlobalColorMod.type = type;
		ca->GlobalColorMod.speed = speed;
		ca->GlobalColorMod.rgb.r = r;
		ca->GlobalColorMod.rgb.g = g;
		ca->GlobalColorMod.rgb.b = b;
		ca->GlobalColorMod.rgb.a = 0;
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
	ca->ColorMods[location].rgb.r = r;
	ca->ColorMods[location].rgb.g = g;
	ca->ColorMods[location].rgb.b = b;
	ca->ColorMods[location].rgb.a = 0;
	if (phase >= 0)
		ca->ColorMods[location].phase = phase;
	else {
		if (ca->ColorMods[location].phase > 2*speed)
			ca->ColorMods[location].phase = 0;
	}
}

void Actor::SetLeader(Actor *actor, int xoffset, int yoffset)
{
	LastFollowed = actor->GetGlobalID();
	FollowOffset.x = xoffset;
	FollowOffset.y = yoffset;
}

//if hp <= 0, it means full healing
void Actor::Heal(int hp)
{
	if (hp > 0) {
		SetBase(IE_HITPOINTS, BaseStats[IE_HITPOINTS] + hp);
	} else {
		SetBase(IE_HITPOINTS, Modified[IE_MAXHITPOINTS]);
	}
}

void Actor::AddExperience(int exp, int combat)
{
	int bonus = core->GetWisdomBonus(0, Modified[IE_WIS]);
	int adjustmentPercent = xpadjustments[GameDifficulty - 1];
	// the "Suppress Extra Difficulty Damage" also switches off the XP bonus
	if (combat && (!NoExtraDifficultyDmg || adjustmentPercent < 0)) {
		bonus += adjustmentPercent;
	}
	bonus += GetFavoredPenalties();
	exp = ((exp * (100 + bonus)) / 100) + BaseStats[IE_XP];
	if (xpcap != NULL) {
		int classid = GetActiveClass() - 1;
		if (xpcap[classid] > 0 && exp > xpcap[classid]) {
			exp = xpcap[classid];
		}
	}
	SetBase(IE_XP, exp);
}

static bool is_zero(const int& value) {
	return value == 0;
}

// for each class pair that is out of level sync for more than 1 level and
// one of them isn't a favored class, incur a 20% xp penalty (cummulative)
int Actor::GetFavoredPenalties() const
{
	if (!third) return 0;
	if (!PCStats) return 0;

	std::list<int> classLevels(PCStats->ClassLevels);
	classLevels.remove_if(is_zero);
	int classCount = classLevels.size();
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
		classLevels.erase((classLevels.end())--);
		classCount--;
	} else {
		// remove() kills all elements with the same value, so we have to jump through hoops
		classLevels.remove(flevel);
		int diff = classCount - classLevels.size();
		if (diff == classCount) return 0; // all class were at the same level
		for (int i=1; i < diff; i++) {
			// re-add missing levels (all but one)
			classLevels.push_back(flevel);
		}
		classCount = classLevels.size();
		if (classCount == 1) return 0; // only one class besides favored
	}

	// finally compare adjacent levels - if they're more than 1 apart
	int penalty = 0;
	std::list<int>::iterator it;
	for (it=++classLevels.begin(); it != classLevels.end(); it++) {
		int level1 = *(--it);
		int level2 = *(++it);
		if (level2 - level1 > 1) penalty++;
	}

	return -20*penalty;
}

int Actor::CalculateExperience(int type, int level) const
{
	if (type>=xpbonustypes) {
		return 0;
	}
	unsigned int l = (unsigned int) (level - 1);

	if (l>=(unsigned int) xpbonuslevels) {
		l=xpbonuslevels-1;
	}
	return xpbonus[type*xpbonuslevels+l];
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
	Point tmp = Destination;
	if (GetPathTries() > MAX_PATH_TRIES) {
		ClearPath(true);
		ResetPathTries();
		return;
	}
	WalkTo(tmp, InternalFlags, pathfindingDistance);
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

int Actor::WantDither() const
{
	if (always_dither) return 2;
	return Selectable::WantDither();
}

//there is a similar function in Map for stationary vvcs
void Actor::DrawVideocells(const Region &screen, vvcVector &vvcCells, const Color &tint)
{
	Map* area = GetCurrentArea();

	for (unsigned int i = 0; i < vvcCells.size(); i++) {
		ScriptedAnimation* vvc = vvcCells[i];

		// actually this is better be drawn by the vvc
		bool endReached = vvc->Draw(screen, Pos, tint, area, WantDither(), GetOrientation(), BBox.h);
		if (endReached) {
			delete vvc;
			vvcCells.erase(vvcCells.begin()+i);
			continue;
		}
		if (!vvc->active) {
			vvc->SetPhase(P_RELEASE);
		}
	}
}

void Actor::DrawActorSprite(const Region &screen, int cx, int cy, const Region& bbox,
			SpriteCover*& newsc, Animation** anims,
			unsigned char Face, const Color& tint, bool useShadowPalette)
{
	CharAnimations* ca = GetAnims();
	int PartCount = ca->GetTotalPartCount();
	Video* video = core->GetVideoDriver();
	Region vp = video->GetViewport();
	ieDword flags = TranslucentShadows ? BLIT_TRANSSHADOW : 0;
	if (!ca->lockPalette) flags |= BLIT_TINTED;
	const Game* game = core->GetGame();
	// when time stops, almost everything turns dull grey, the caster and immune actors being the most notable exceptions
	if (game->TimeStoppedFor(this)) {
		flags |= BLIT_GREY;
	}

	// display current frames in the right order
	const int* zOrder = ca->GetZOrder(Face);
	for (int part = 0; part < PartCount; ++part) {
		int partnum = part;
		if (zOrder) partnum = zOrder[part];
		Animation* anim = anims[partnum];
		Sprite2D* nextFrame = 0;
		if (anim)
			nextFrame = anim->GetFrame(anim->GetCurrentFrame());
		if (nextFrame && bbox.IntersectsRegion( vp ) ) {
			if (!newsc || !newsc->Covers(cx, cy, nextFrame->XPos, nextFrame->YPos, nextFrame->Width, nextFrame->Height)) {
				// the first anim contains the animarea for
				// the entire multi-part animation
				newsc = area->BuildSpriteCover(cx,
					cy, -anims[0]->animArea.x,
					-anims[0]->animArea.y,
					anims[0]->animArea.w,
					anims[0]->animArea.h, WantDither() );
			}
			assert(newsc->Covers(cx, cy, nextFrame->XPos, nextFrame->YPos, nextFrame->Width, nextFrame->Height));

			Palette* palette = useShadowPalette ? ca->GetShadowPalette() : ca->GetPartPalette(partnum);
			video->BlitGameSprite( nextFrame, cx + screen.x, cy + screen.y,
				flags, tint, newsc, palette, &screen);
		}
	}
}


static const int OrientdX[16] = { 0, -4, -7, -9, -10, -9, -7, -4, 0, 4, 7, 9, 10, 9, 7, 4 };
static const int OrientdY[16] = { 10, 9, 7, 4, 0, -4, -7, -9, -10, -9, -7, -4, 0, 4, 7, 9 };
static const unsigned int MirrorImageLocation[8] = { 4, 12, 8, 0, 6, 14, 10, 2 };
static const unsigned int MirrorImageZOrder[8] = { 2, 4, 6, 0, 1, 7, 5, 3 };

bool Actor::ShouldHibernate() const
{
	//finding an excuse why we don't hybernate the actor
	if (Modified[IE_ENABLEOFFSCREENAI])
		return false;
	if (LastTarget) //currently attacking someone
		return false;
	if (!LastTargetPos.isempty()) //currently casting at the ground
		return false;
	if (LastSpellTarget) //currently casting at someone
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
	return true;
}

void Actor::UpdateAnimations()
{
	// TODO: move this
	if (InTrap) {
		area->ClearTrap(this, InTrap-1);
	}

	//make actor unselectable and unselected when it is not moving
	//dead, petrified, frozen, paralysed or unavailable to player
	if (!ValidTarget(GA_SELECT|GA_NO_ENEMY|GA_NO_NEUTRAL)) {
		core->GetGame()->SelectActor(this, false, SELECT_NORMAL);
	}

	CharAnimations* ca = GetAnims();
	if (!ca) {
		return;
	}

	ca->PulseRGBModifiers();

	unsigned char StanceID = GetStance();
	unsigned char Face = GetNextFace();
	Animation** anims = ca->GetAnimation( StanceID, Face );
	if (!anims) {
		return;
	}
	Animation **shadowAnimations = ca->GetShadowAnimation(StanceID, Face);

	//If you find a better place for it, I'll really be glad to put it there
	//IN BG1 and BG2, this is at the ninth frame... (depends on the combat bitmap, which we don't handle yet)
	// however some critters don't have that long animations (eg. squirrel 0xC400)
	if (attackProjectile) {
		unsigned int frameCount = anims[0]->GetFrameCount();
		unsigned int currentFrame = anims[0]->GetCurrentFrame();
		if ((frameCount > 8 && currentFrame == 8) || (frameCount <= 8 && currentFrame == frameCount/2)) {
			GetCurrentArea()->AddProjectile(attackProjectile, Pos, LastTarget, false);
			attackProjectile = NULL;
		}
	}

	// advance first (main) animation by one frame (in sync)
	if (Immobile()) {
		// update animation, continue last-displayed frame
		anims[0]->LastFrame();
		if (shadowAnimations) {
			shadowAnimations[0]->LastFrame();
		}
	} else {
		// update animation, maybe advance a frame (if enough time has passed)
		anims[0]->NextFrame();
		if (shadowAnimations) {
			shadowAnimations[0]->NextFrame();
		}
	}

	// update all other animation parts, in sync with the first part
	int PartCount = ca->GetTotalPartCount();
	for (int part = 1; part < PartCount; ++part) {
		if (anims[part])
			anims[part]->GetSyncedNextFrame(anims[0]);
	}

	if (anims[0]->endReached) {
		if (HandleActorStance()) {
			// restart animation
			anims[0]->endReached = false;
			anims[0]->SetPos(0);

			if (shadowAnimations) {
				shadowAnimations[0]->endReached = false;
				shadowAnimations[0]->SetPos(0);
			}
		}
	} else {
		//check if walk sounds need to be played
		//dialog, pause game
		if (!(core->GetGameControl()->GetDialogueFlags()&(DF_IN_DIALOG|DF_FREEZE_SCRIPTS) ) ) {
			//footsteps option set, stance
			if (footsteps && (GetStance() == IE_ANI_WALK)) {
				// sound lengths ensure proper pacing; haste was ignored
				PlayWalkSound();
			}
		}
	}
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

	if (Modified[IE_AVATARREMOVAL]) {
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

	return true;
}

bool Actor::HasBodyHeat() const
{
	if (Modified[IE_STATE_ID]&(STATE_DEAD|STATE_FROZEN|STATE_PETRIFIED) ) return false;
	if (GetAnims()->GetFlags()&AV_NO_BODY_HEAT) return false;
	return true;
}

void Actor::Draw(const Region &screen)
{
	const Map *area = GetCurrentArea();
	if (!area) {
		InternalFlags &= ~IF_TRIGGER_AP;
		return;
	}

	int cx = Pos.x;
	int cy = Pos.y;
	int explored = Modified[IE_DONOTJUMP]&DNJ_UNHINDERED;
	//check the deactivation condition only if needed
	//this fixes dead actors disappearing from fog of war (they should be permanently visible)
	if ((!area->IsVisible( Pos, explored) || (InternalFlags&IF_REALLYDIED) ) && (InternalFlags&IF_ACTIVE) ) {
		//turning actor inactive if there is no action next turn
		if (ShouldHibernate()) {
			InternalFlags|=IF_IDLE;
		}
		if (!(InternalFlags&IF_REALLYDIED)) {
			// for a while this didn't return (disable drawing) if about to hibernate;
			// Avenger said (aa10aaed) "we draw the actor now for the last time".
			InternalFlags &= ~IF_TRIGGER_AP;
			return;
		}
	}

	// if an actor isn't visible, should we still draw video cells?
	// let us assume not, for now..
	if (!(InternalFlags & IF_VISIBLE)) {
		InternalFlags &= ~IF_TRIGGER_AP;
		return;
	}

	//iwd has this flag saved in the creature
	if (Modified[IE_AVATARREMOVAL]) {
		return;
	}

	//visual feedback
	CharAnimations* ca = GetAnims();
	if (!ca) {
		InternalFlags &= ~IF_TRIGGER_AP;
		return;
	}

	//explored or visibilitymap (bird animations are visible in fog)
	//0 means opaque
	int Trans = Modified[IE_TRANSLUCENT];

	if (Trans>255) {
		Trans=255;
	}

	int State = Modified[IE_STATE_ID];

	//adjust invisibility for enemies
	if (Modified[IE_EA]>EA_GOODCUTOFF) {
		if (State&state_invisible) {
			Trans = 255;
		}
	}

	//can't move this, because there is permanent blur state where
	//there is no effect (just state bit)
	if ((State&STATE_BLUR) && Trans < 128) {
		Trans = 128;
	}
	Color tint = area->LightMap->GetPixel( cx / 16, cy / 12);
	tint.a = (ieByte) (255-Trans);

	unsigned char heightmapindex = area->HeightMap->GetAt( cx / 16, cy / 12);
	if (heightmapindex > 15) {
		// there are 8bpp lightmaps (eg, bg2's AR1300) and fuzzie
		// cannot work out how they work, so here is an incorrect
		// hack (probably). please fix!
		heightmapindex = 15;
	}

	//don't use cy for area map access beyond this point
	cy -= heightmapindex;

	//draw videocells under the actor
	DrawVideocells(screen, vvcShields, tint);

	Video* video = core->GetVideoDriver();
	Region vp = video->GetViewport();

	bool shoulddrawcircle = ShouldDrawCircle();
	bool drawcircle = shoulddrawcircle;
	const GameControl *gc = core->GetGameControl();
	if (gc->GetScreenFlags()&SF_CUTSCENE) {
		// ground circles are not drawn in cutscenes
		drawcircle = false;
	}
	// the speaker should get a circle even in cutscenes
	if (shoulddrawcircle && (gc->GetDialogueFlags()&DF_IN_DIALOG) && gc->dialoghandler->IsTarget(this)) {
		drawcircle = true;
	}
	bool drawtarget = false;
	// we always show circle/target on pause
	if (drawcircle && !(gc->GetDialogueFlags() & DF_FREEZE_SCRIPTS)) {
		// check marker feedback level
		ieDword markerfeedback = 4;
		core->GetDictionary()->Lookup("GUI Feedback Level", markerfeedback);
		if (Over) {
			// picked creature, should always be true
			drawcircle = true;
		} else if (Selected) {
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

	if (remainingTalkSoundTime > 0) {
		unsigned int currentTick = GetTickCount();
		unsigned int diffTime = currentTick - lastTalkTimeCheckAt;
		lastTalkTimeCheckAt = currentTick;

		if (diffTime >= remainingTalkSoundTime) {
			remainingTalkSoundTime = 0;
		} else {
			remainingTalkSoundTime -= diffTime;
		}
		SetCircleSize();
	}

	if (drawcircle) {
		DrawCircle(vp);
		drawtarget = ((Selected || Over) && !(InternalFlags&IF_NORETICLE) && Modified[IE_EA] <= EA_CONTROLLABLE && Destination != Pos);
	}
	if (drawtarget) {
		gc->DrawTargetReticle(Destination, (size - 1) * 4, true, Over, Selected); //we could set this to !paused if we wanted to only animate when not paused
	}

	unsigned char StanceID = GetStance();
	unsigned char Face = GetNextFace();
	Animation** anims = ca->GetAnimation( StanceID, Face );
	if (anims) {
		if (Immobile() || !shoulddrawcircle) {
			//set the last frame if actor is died and deactivated
			if (!(InternalFlags&(IF_ACTIVE|IF_IDLE)) && (StanceID==IE_ANI_TWITCH) ) {
				anims[0]->SetPos(anims[0]->GetFrameCount()-1);
			}
		}

		int PartCount = ca->GetTotalPartCount();
		Sprite2D* nextFrame = anims[0]->GetFrame(anims[0]->GetCurrentFrame());

		// update bounding box and such
		if (nextFrame && lastFrame != nextFrame) {
			Region newBBox;
			if (PartCount == 1) {
				newBBox.x = cx - nextFrame->XPos;
				newBBox.w = nextFrame->Width;
				newBBox.y = cy - nextFrame->YPos;
				newBBox.h = nextFrame->Height;
			} else {
				// FIXME: currently using the animarea instead
				// of the real bounding box of this (multi-part) frame.
				// Shouldn't matter much, though. (wjp)
				newBBox.x = cx + anims[0]->animArea.x;
				newBBox.y = cy + anims[0]->animArea.y;
				newBBox.w = anims[0]->animArea.w;
				newBBox.h = anims[0]->animArea.h;
			}
			lastFrame = nextFrame;
			SetBBox( newBBox );
		}

		// Drawing the actor:
		// * mirror images:
		//     Drawn without transparency, unless fully invisible.
		//     Order: W, E, N, S, NW, SE, NE, SW
		//     Uses extraCovers 3-10
		// * blurred copies (3 of them)
		//     Drawn with transparency.
		//     distance between copies depends on IE_MOVEMENTRATE
		//     TODO: actually, the direction is the real movement direction,
		//	not the (rounded) direction given Face
		//     Uses extraCovers 0-2
		// * actor itself
		//     Uses main spritecover
		//
		//comments by Avenger:
		// currently we don't have a real direction, but the orientation field
		// could be used with higher granularity. When we need the face value
		// it could be divided so it will become a 0-15 number.
		//

		SpriteCover *sc = 0, *newsc = 0;
		int blurx = cx;
		int blury = cy;
		int blurdx = (OrientdX[Face]*(int)Modified[IE_MOVEMENTRATE])/20;
		int blurdy = (OrientdY[Face]*(int)Modified[IE_MOVEMENTRATE])/20;
		Color mirrortint = tint;
		//mirror images are also half transparent when invis
		//if (mirrortint.a > 0) mirrortint.a = 255;

		int i;

		// mirror images behind the actor
		for (i = 0; i < 4; ++i) {
			unsigned int m = MirrorImageZOrder[i];
			if (m < Modified[IE_MIRRORIMAGES]) {
				Region sbbox = BBox;
				int dir = MirrorImageLocation[m];
				int icx = cx + 3*OrientdX[dir];
				int icy = cy + 3*OrientdY[dir];
				Point iPos(icx, icy);
				if (area->GetBlocked(iPos.x / 16, iPos.y / 12) & (PATH_MAP_PASSABLE | PATH_MAP_ACTOR)) {
					sbbox.x += 3*OrientdX[dir];
					sbbox.y += 3*OrientdY[dir];
					newsc = sc = extraCovers[3+m];
					DrawActorSprite(screen, icx, icy, sbbox, newsc,
						anims, Face, mirrortint);
					if (newsc != sc) {
						delete sc;
						extraCovers[3+m] = newsc;
					}
				}
			} else {
				delete extraCovers[3+m];
				extraCovers[3+m] = NULL;
			}
		}

		// blur sprites behind the actor
		if (State & STATE_BLUR) {
			if (Face < 4 || Face >= 12) {
				Region sbbox = BBox;
				sbbox.x -= 4*blurdx; sbbox.y -= 4*blurdy;
				blurx -= 4*blurdx; blury -= 4*blurdy;
				for (i = 0; i < 3; ++i) {
					sbbox.x += blurdx; sbbox.y += blurdy;
					blurx += blurdx; blury += blurdy;
					newsc = sc = extraCovers[i];
					DrawActorSprite(screen, blurx, blury, sbbox, newsc,
						anims, Face, tint);
					if (newsc != sc) {
						delete sc;
						extraCovers[i] = newsc;
					}
				}
			}
		}

		const Game* game = core->GetGame();
		ieDword flags = !ca->lockPalette ? BLIT_TINTED : 0;
		game->ApplyGlobalTint(tint, flags);

		// infravision, independent of light map and global light
		if ( HasBodyHeat() &&
			game->PartyHasInfravision() &&
			!game->IsDay() &&
			(area->AreaType & AT_OUTDOOR) && !(area->AreaFlags & AF_DREAM)) {
			tint.r = 255;

			/* IWD2: infravision is white, not red. */
			if(core->HasFeature(GF_3ED_RULES)) {
				tint.g = tint.b = 255;
			} else {
				tint.g = tint.b = 120;
			}
		}

		newsc = sc = GetSpriteCover();
		Animation **shadowAnimations = ca->GetShadowAnimation(StanceID, Face);
		if (shadowAnimations) {
			DrawActorSprite(screen, cx, cy, BBox, newsc, shadowAnimations, Face, tint, true);
		}

		// actor itself
		DrawActorSprite(screen, cx, cy, BBox, newsc, anims, Face, tint);
		if (newsc != sc) SetSpriteCover(newsc);

		// blur sprites in front of the actor
		if (State & STATE_BLUR) {
			if (Face >= 4 && Face < 12) {
				Region sbbox = BBox;
				for (i = 0; i < 3; ++i) {
					sbbox.x -= blurdx; sbbox.y -= blurdy;
					blurx -= blurdx; blury -= blurdy;
					newsc = sc = extraCovers[i];
					DrawActorSprite(screen, blurx, blury, sbbox, newsc,
						anims, Face, tint);
					if (newsc != sc) {
						delete sc;
						extraCovers[i] = newsc;
					}
				}
			}
		}

		// mirror images in front of the actor
		for (i = 4; i < 8; ++i) {
			unsigned int m = MirrorImageZOrder[i];
			if (m < Modified[IE_MIRRORIMAGES]) {
				Region sbbox = BBox;
				int dir = MirrorImageLocation[m];
				int icx = cx + 3*OrientdX[dir];
				int icy = cy + 3*OrientdY[dir];
				Point iPos(icx, icy);
				if (area->GetBlocked(iPos.x / 16, iPos.y / 12) & (PATH_MAP_PASSABLE | PATH_MAP_ACTOR)) {
					sbbox.x += 3*OrientdX[dir];
					sbbox.y += 3*OrientdY[dir];
					newsc = sc = extraCovers[3+m];
					DrawActorSprite(screen, icx, icy, sbbox, newsc,
						anims, Face, mirrortint);
					if (newsc != sc) {
						delete sc;
						extraCovers[3+m] = newsc;
					}
				}
			} else {
				delete extraCovers[3+m];
				extraCovers[3+m] = NULL;
			}
		}
	}

	//draw videocells over the actor
	DrawVideocells(screen, vvcOverlays, tint);

	// display pc hitpoints if requested
	// limit the invocation count to save resources (the text is drawn repeatedly anyway)
	ieDword tmp = 0;
	core->GetDictionary()->Lookup("HP Over Head", tmp);
	if (tmp && Persistent() && (core->GetGame()->GameTime % (core->Time.round_size/2) == 0)) { // smaller delta to skip fading
		DisplayHeadHPRatio();
	}

	// trigger on-enemy-sighted autopause
	if (!(InternalFlags & IF_TRIGGER_AP)) {
		// always recheck in case of EA changes (npc going hostile)
		if (Modified[IE_EA] > EA_EVILCUTOFF && !(InternalFlags & IF_STOPATTACK)) {
			InternalFlags |= IF_TRIGGER_AP;
			core->Autopause(AP_ENEMY, this);
		}
	}
}

/* Handling automatic stance changes */
bool Actor::HandleActorStance()
{
	CharAnimations* ca = GetAnims();
	int StanceID = GetStance();

	if (ca->autoSwitchOnEnd) {
		int nextstance = ca->nextStanceID;
		SetStance( nextstance );
		ca->autoSwitchOnEnd = false;
		return nextstance != ca->previousStanceID;
	}
	int x = RAND(0, 25);
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

bool Actor::GetSoundFromFile(ieResRef Sound, unsigned int index) const
{
	// only dying ignores the incapacity to vocalize
	if (index != VB_DIE) {
		if (Modified[IE_STATE_ID] & STATE_CANTLISTEN) return false;
	}

	if (core->HasFeature(GF_RESDATA_INI)) {
		return GetSoundFromINI(Sound, index);
	} else {
		return GetSoundFrom2DA(Sound, index);
	}
}

bool Actor::GetSoundFrom2DA(ieResRef Sound, unsigned int index) const
{
	if (!anims) return false;

	AutoTable tab(anims->ResRef);
	if (!tab) return false;

	switch (index) {
		// TODO: research whether this VB should be split into 5x VB_BATTLE_CRY and 4x VB_ATTACK (like in NI)
		// wasn't played if the weapon wasn't of type misc (so just the swing sound if any)
		case VB_ATTACK:
			index = 0;
			break;
		case VB_DAMAGE:
			index = 8;
			break;
		case VB_DIE:
			index = 10;
			break;
		//TODO: one day we should implement verbal constant groups
		case VB_DIALOG:
		case VB_SELECT:
		case VB_SELECT+1:
		case VB_SELECT+2:
			index = 36; // Selection (yes, the row names are inconsistently capitalized)
			break;
		// entries without VB equivalents
		case 100+IE_ANI_SHOOT:
			index = 16; // SHOOT
			break;
		// these three supposedly never worked, at least not in bg2 (https://www.gibberlings3.net/forums/topic/19034-animation-2da-files)
		case 100+IE_ANI_ATTACK_SLASH:
			index = 22; // ATTACK_SLASH
			break;
		case 100+IE_ANI_ATTACK_BACKSLASH:
			index = 24; // ATTACK_BACKSLASH
			break;
		case 100+IE_ANI_ATTACK_JAB:
			index = 26; // ATTACK_JAB
			break;
		// TODO: research whether this entry is ever different from ATTACK and what's the difference in use
		case 200:
			index = 34; // Battle_Cry
			break;
		default:
			Log(WARNING, "Actor", "TODO:Cannot determine 2DA rowcount for index: %d", index);
			return false;
	}
	Log(MESSAGE, "Actor", "Getting sound 2da %.8s entry: %s",
		anims->ResRef, tab->GetRowName(index) );
	int col = core->Roll(1,tab->GetColumnCount(index),-1);
	strnlwrcpy(Sound, tab->QueryField (index, col), 8);
	return true;
}

//Get the monster sound from a global .ini file.
//It is ResData.ini in PST and Sounds.ini in IWD/HoW
bool Actor::GetSoundFromINI(ieResRef Sound, unsigned int index) const
{
	const char *resource = "";
	char section[12];
	unsigned int animid=BaseStats[IE_ANIMATION_ID];
	if(core->HasFeature(GF_ONE_BYTE_ANIMID)) {
		animid&=0xff;
	}
	snprintf(section,10,"%d", animid);

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
	 *   att2-att4
	 *   fall
	 *   fidget
	 */
	switch(index) {
		case VB_ATTACK:
			resource = core->GetResDataINI()->GetKeyAsString(section, IWDSound?"att1":"at1sound","");
			break;
		case VB_DAMAGE:
			resource = core->GetResDataINI()->GetKeyAsString(section, IWDSound?"damage":"hitsound","");
			break;
		case VB_DIE:
			resource = core->GetResDataINI()->GetKeyAsString(section, IWDSound?"death":"dfbsound","");
			break;
		case VB_SELECT:
			//this isn't in PST, apparently
			if (IWDSound) {
				resource = core->GetResDataINI()->GetKeyAsString(section, "selected","");
			}
			break;
		// entries without VB equivalents
		case 100+IE_ANI_SHOOT:
		case 100+IE_ANI_ATTACK_SLASH:
		case 100+IE_ANI_ATTACK_BACKSLASH:
		case 100+IE_ANI_ATTACK_JAB:
			// FIXME: complete guess
			resource = core->GetResDataINI()->GetKeyAsString(section, IWDSound?"att2":"at2sound","");
			break;
		case 200: // battle cry
			if (IWDSound) {
				resource = core->GetResDataINI()->GetKeyAsString(section, "btlcry", "");
			}
			break;
	}

	int count = CountElements(resource, ',');
	int slot = RAND(0, count - 1);
	while (slot--) {
		while (*resource && *resource != ',') resource++;
		if (*resource == ',') resource++;
	}
	size_t len = strcspn(resource, ",");
	assert(len < sizeof(ieResRef));
	strlcpy(Sound, resource, len + 1);

	return true;
}

void Actor::ResolveStringConstant(ieResRef Sound, unsigned int index) const
{
	if (PCStats && PCStats->SoundSet[0]) {
		//resolving soundset (bg1/bg2 style)
		size_t len;
		if (csound[index]) {
			len = snprintf(Sound, sizeof(ieResRef), "%s%c", PCStats->SoundSet, csound[index]);
			if (len > sizeof(ieResRef)) Log(ERROR, "Actor", "Actor %s has too long soundset name: %s", LongName, PCStats->SoundSet);
			return;
		}
		//icewind style
		len = snprintf(Sound, sizeof(ieResRef), "%s%02d", PCStats->SoundSet, VCMap[index]);
		if (len > sizeof(ieResRef)) Log(ERROR, "Actor", "Actor %s has too long soundset name: %s", LongName, PCStats->SoundSet);
		return;
	}

	Sound[0]=0;

	if (core->HasFeature(GF_RESDATA_INI)) {
		GetSoundFromINI(Sound, index);
	} else {
		GetSoundFrom2DA(Sound, index);
	}

	//Empty resrefs
	if (Sound[0]=='*') Sound[0]=0;
	else if(!strncmp(Sound,"nosound",8) ) Sound[0]=0;
}

void Actor::SetActionButtonRow(ActionButtonRow &ar)
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
	ieByte tmp = actslot;
	if (QslotTranslation && slotindex>2) {
		if (tmp > ACT_IWDQSONG) { //quick songs
			tmp = 110 + tmp%10;
		} else if (tmp > ACT_IWDQSPEC) { //quick abilities
			tmp = 90 + tmp%10;
		} else if (tmp > ACT_IWDQITEM) { //quick items
			tmp = 80 + tmp%10;
		} else if (tmp > ACT_IWDQSPELL) { //quick spells
			tmp = 70 + tmp%10;
		} else if (tmp > ACT_BARD) { //spellbooks
			tmp = 50 + tmp%10;
		} else if (tmp >= 32) { // here be dragons
			Log(ERROR, "Actor", "Bad slot index passed to SetActionButtonRow!");
		} else {
			tmp = gemrb2iwd[tmp];
		}
	}
	return tmp;
}

int Actor::IWD2GemrbQslot (int slotindex) const
{
	ieByte tmp = PCStats->QSlots[slotindex];
	//the first three buttons are hardcoded in gemrb
	//don't mess with them
	if (QslotTranslation && slotindex>2) {
		if (tmp>=110) { //quick songs
			tmp = ACT_IWDQSONG + tmp%10;
		} else if (tmp>=90) { //quick abilities
			tmp = ACT_IWDQSPEC + tmp%10;
		} else if (tmp>=80) { //quick items
			tmp = ACT_IWDQITEM + tmp%10;
		} else if (tmp>=70) { //quick spells
			tmp = ACT_IWDQSPELL + tmp%10;
		} else if (tmp>=50) { //spellbooks
			tmp = ACT_BARD + tmp%10;
		} else if (tmp>=32) { // here be dragons
			Log(ERROR, "Actor", "Bad slot index passed to IWD2GemrbQslot!");
		} else {
			tmp = iwd2gemrb[tmp];
		}
	}
	return tmp;
}

// debug function; only works on pc classes
void Actor::dumpQSlots() const
{
	ActionButtonRow r;
	memcpy(&r, GUIBTDefaults + GetActiveClass(), sizeof(ActionButtonRow));
	StringBuffer buffer, buffer2, buffer3;

	buffer.append("Current  default: ");
	buffer2.append("IWD2gem  default: ");
	buffer3.append("gem2IWD2 default: ");
	for(int i=0; i<GUIBT_COUNT; i++) {
		ieByte tmp = r[i];
		buffer.appendFormatted("%3d ", tmp);
		buffer2.appendFormatted("%3d ", IWD2GemrbQslot(tmp));
		buffer3.appendFormatted("%3d ", Gemrb2IWD2Qslot(tmp, i));
	}
	buffer.appendFormatted("(class: %d)", GetStat(IE_CLASS));
	Log(DEBUG, "Actor", buffer);
//	Log(DEBUG, "Actor", buffer2);
//	Log(DEBUG, "Actor", buffer3);

	buffer.clear();
	buffer2.clear();
	buffer3.clear();
	buffer.append("Current  QSlots:  ");
	buffer2.append("IWD2gem  QSlots:  ");
	buffer3.append("gem2IWD2 QSlots:  ");
	for(int i=0; i<GUIBT_COUNT; i++) {
		ieByte tmp = PCStats->QSlots[i];
		buffer.appendFormatted("%3d ", tmp);
		buffer2.appendFormatted("%3d ", IWD2GemrbQslot(tmp));
		buffer3.appendFormatted("%3d ", Gemrb2IWD2Qslot(tmp, i));
	}
	Log(DEBUG, "Actor", buffer);
	Log(DEBUG, "Actor", buffer2);
	Log(DEBUG, "Actor", buffer3);
}

void Actor::SetPortrait(const char* ResRef, int Which)
{
	int i;

	if (ResRef == NULL) {
		return;
	}
	if (InParty) {
		core->SetEventFlag(EF_PORTRAIT);
	}

	if(Which!=1) {
		CopyResRef( SmallPortrait, ResRef );
	}
	if(Which!=2) {
		CopyResRef( LargePortrait, ResRef );
	}
	if(!Which) {
		for (i = 0; i < 8 && ResRef[i]; i++) {};
		if (SmallPortrait[i-1] != 'S' && SmallPortrait[i-1] != 's') {
			SmallPortrait[i] = 'S';
		}
		if (LargePortrait[i-1] != 'M' && LargePortrait[i-1] != 'm') {
			LargePortrait[i] = 'M';
		}
	}
}

void Actor::SetSoundFolder(const char *soundset)
{
	if (core->HasFeature(GF_SOUNDFOLDERS)) {
		char filepath[_MAX_PATH];

		strnlwrcpy(PCStats->SoundFolder, soundset, 32);
		PathJoin(filepath, core->GamePath, "sounds", PCStats->SoundFolder, NULL);
		char file[_MAX_PATH];

		if (FileGlob(file, filepath, "??????01")) {
			file[6] = '\0';
		} else if (FileGlob(file, filepath, "?????01")) {
			file[5] = '\0';
		} else if (FileGlob(file, filepath, "????01")) {
			file[4] = '\0';
		} else {
			return;
		}
		strnlwrcpy(PCStats->SoundSet, file, 8);
	} else {
		strnlwrcpy(PCStats->SoundSet, soundset, 8);
		PCStats->SoundFolder[0]=0;
	}
}

void Actor::GetSoundFolder(char *soundset, int full, ieResRef overrideSet) const
{
	ieResRef set;
	if (overrideSet) {
		CopyResRef(set, overrideSet);
	} else {
		CopyResRef(set, PCStats->SoundSet);
	}

	if (core->HasFeature(GF_SOUNDFOLDERS)) {
		strnlwrcpy(soundset, PCStats->SoundFolder, 32);
		if (full) {
			strcat(soundset,"/");
			strncat(soundset, set, 9);
		}
	}
	else {
		strnlwrcpy(soundset, set, 8);
	}
}

bool Actor::HasVVCCell(const ieResRef resource) const
{
	return GetVVCCell(resource) != NULL;
}

ScriptedAnimation *Actor::GetVVCCell(const vvcVector *vvcCells, const ieResRef resource) const
{
	size_t i=vvcCells->size();
	while (i--) {
		ScriptedAnimation *vvc = (*vvcCells)[i];
		if (vvc == NULL) {
			continue;
		}
		if ( strnicmp(vvc->ResName, resource, 8) == 0) {
			return vvc;
		}
	}
	return NULL;
}

ScriptedAnimation *Actor::GetVVCCell(const ieResRef resource) const
{
	ScriptedAnimation *vvc = GetVVCCell(&vvcShields, resource);
	if (vvc) return vvc;
	return GetVVCCell(&vvcOverlays, resource);
}

void Actor::RemoveVVCell(const ieResRef resource, bool graceful)
{
	bool j = true;
	vvcVector *vvcCells=&vvcShields;
retry:
	size_t i=vvcCells->size();
	while (i--) {
		ScriptedAnimation *vvc = (*vvcCells)[i];
		if (vvc == NULL) {
			continue;
		}
		if ( strnicmp(vvc->ResName, resource, 8) == 0) {
			if (graceful) {
				vvc->SetPhase(P_RELEASE);
			} else {
				delete vvc;
				vvcCells->erase(vvcCells->begin()+i);
			}
		}
	}
	vvcCells=&vvcOverlays;
	if (j) { j = false; goto retry; }
}

//this is a faster version of hasvvccell, because it knows where to look
//for the overlay, it also returns the vvc for further manipulation
//use this for the seven eyes overlay
ScriptedAnimation *Actor::FindOverlay(int index) const
{
	const vvcVector *vvcCells;

	if (index >= OVERLAY_COUNT) return NULL;

	if (hc_locations&(1<<index)) vvcCells=&vvcShields;
	else vvcCells=&vvcOverlays;

	const char *resRef = hc_overlays[index];

	size_t i=vvcCells->size();
	while (i--) {
		ScriptedAnimation *vvc = (*vvcCells)[i];
		if (vvc == NULL) {
			continue;
		}
		if ( strnicmp(vvc->ResName, resRef, 8) == 0) {
			return vvc;
		}
	}
	return NULL;
}

void Actor::AddVVCell(ScriptedAnimation* vvc)
{
	vvcVector *vvcCells;

	//if the vvc was not created, don't try to add it
	if (!vvc) {
		return;
	}
	if (vvc->YPos < 0) {
		vvcCells=&vvcShields;
	} else {
		vvcCells=&vvcOverlays;
	}
	size_t i=vvcCells->size();
	while (i--) {
		if ((*vvcCells)[i] == NULL) {
			(*vvcCells)[i] = vvc;
			return;
		}
	}
	vvcCells->push_back( vvc );
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
	if (inventory.HasItemInSlot("",inventory.GetMagicSlot())) {
		return inventory.GetMagicSlot();
	}
	if (!PCStats) {
		return slot+inventory.GetWeaponSlot();
	}
	return PCStats->QuickWeaponSlots[slot];
}

//marks the quickslot as equipped
int Actor::SetEquippedQuickSlot(int slot, int header)
{
	if (!PCStats) {
		if (header<0) header=0;
		inventory.SetEquippedSlot(slot, header);
		return 0;
	}


	if ((slot<0) || (slot == IW_NO_EQUIPPED) ) {
		if (slot == IW_NO_EQUIPPED) {
			slot = inventory.GetFistSlot();
		}
		int i;
		for(i=0;i<MAX_QUICKWEAPONSLOT;i++) {
			if(slot+inventory.GetWeaponSlot()==PCStats->QuickWeaponSlots[i]) {
				slot = i;
				break;
			}
		}
		//if it is the fist slot and not currently used, then set it up
		if (i==MAX_QUICKWEAPONSLOT) {
			inventory.SetEquippedSlot(IW_NO_EQUIPPED, 0);
			return 0;
		}
	}

	assert(slot<MAX_QUICKWEAPONSLOT);
	if (header==-1) {
		header = PCStats->QuickWeaponHeaders[slot];
	}
	else {
		PCStats->QuickWeaponHeaders[slot]=header;
	}
	slot = inventory.GetWeaponQuickSlot(PCStats->QuickWeaponSlots[slot]);
	if (inventory.SetEquippedSlot(slot, header)) {
		return 0;
	}
	return STR_MAGICWEAPON;
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

	ieResRef tmpresref;
	strnuprcpy(tmpresref, item->ItemResRef, sizeof(ieResRef)-1);

	Item *itm = gamedata->GetItem(tmpresref, true);
	if (!itm) {
		Log(WARNING, "Actor", "Invalid quick slot item: %s!", tmpresref);
		return false; //quick item slot contains invalid item resref
	}
	//item is depleted for today
	if(itm->UseCharge(item->Usages, header, false)==CHG_DAY) {
		return false;
	}

	Projectile *pro = itm->GetProjectile(this, header, target, slot, flags&UI_MISS);
	ChargeItem(slot, header, item, itm, flags&UI_SILENT, !(flags&UI_NOCHARGE));
	gamedata->FreeItem(itm,tmpresref, false);
	ResetCommentTime();
	if (pro) {
		pro->SetCaster(GetGlobalID(), ITEM_CASTERLEVEL);
		GetCurrentArea()->AddProjectile(pro, Pos, target);
		return true;
	}
	return false;
}

void Actor::ModifyWeaponDamage(WeaponInfo &wi, Actor *target, int &damage, bool &critical)
{
	//Calculate weapon based damage bonuses (strength bonus, dexterity bonus, backstab)
	bool weaponImmunity = target->fxqueue.WeaponImmunity(wi.enchantment, wi.itemflags);
	int multiplier = Modified[IE_BACKSTABDAMAGEMULTIPLIER];
	int extraDamage = 0; // damage unaffected by the critical multiplier

	if (third) {
		// 3ed sneak attack
		if (multiplier > 0) {
			extraDamage = GetSneakAttackDamage(target, wi, multiplier, weaponImmunity);
		}
	} else if (multiplier > 1) {
		// aDnD backstabbing
		damage = GetBackstabDamage(target, wi, multiplier, damage);
	}

	damage += WeaponDamageBonus(wi);

	if (weaponImmunity) {
		//'my weapon has no effect'
		damage = 0;
		critical = false;
		if (InParty) {
			if (core->HasFeedback(FT_COMBAT)) DisplayStringOrVerbalConstant(STR_WEAPONINEFFECTIVE, VB_TIMMUNE);
			core->Autopause(AP_UNUSABLE, this);
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
			if (core->HasFeedback(FT_COMBAT)) displaymsg->DisplayConstantStringName(STR_NO_CRITICAL, DMC_WHITE, target);
			critical = false;
		} else {
			VerbalConstant(VB_CRITHIT);
			//multiply the damage with the critical multiplier
			damage *= wi.critmulti;

			// check if critical hit needs a screenshake
			if (crit_hit_scr_shake && (InParty || target->InParty) && core->GetVideoDriver()->GetViewport().PointInside(Pos) ) {
				core->timer->SetScreenShake(10,-10,AI_UPDATE_TIME);
			}

			//apply the dirty fighting spell
			if (HasFeat(FEAT_DIRTY_FIGHTING) ) {
				core->ApplySpell(resref_dirty, target, this, multiplier);
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
	if (invisible || always || target->Immobile() || IsBehind(target)) {
		bool dodgy = target->GetStat(IE_UNCANNY_DODGE) & 0x200;
		// if true, we need to be 4+ levels higher to still manage a sneak attack
		if (dodgy) {
			if (GetStat(IE_CLASSLEVELSUM) >= target->GetStat(IE_CLASSLEVELSUM) + 4) {
				dodgy = false;
			}
		}
		if (target->Modified[IE_DISABLEBACKSTAB] || weaponImmunity || dodgy) {
			if (core->HasFeedback(FT_COMBAT)) displaymsg->DisplayConstantString (STR_BACKSTAB_FAIL, DMC_WHITE);
			wi.backstabbing = false;
		} else {
			if (wi.backstabbing) {
				// first check for feats that change the sneak dice
				// special effects on hit for arterial strike (-1d6) and hamstring (-2d6)
				// both are available at level 10+ (5d6), so it's safe to decrease multiplier without checking
				if (BackstabResRef[0]!='*') {
					if (stricmp(BackstabResRef, resref_arterial)) {
						// ~Sneak attack for %d inflicts hamstring damage (Slowed)~
						multiplier -= 2;
						sneakAttackDamage = LuckyRoll(multiplier, 6, 0, 0, target);
						displaymsg->DisplayRollStringName(39829, DMC_LIGHTGREY, this, sneakAttackDamage);
					} else {
						// ~Sneak attack for %d scores arterial strike (Inflicts bleeding wound)~
						multiplier--;
						sneakAttackDamage = LuckyRoll(multiplier, 6, 0, 0, target);
						displaymsg->DisplayRollStringName(39828, DMC_LIGHTGREY, this, sneakAttackDamage);
					}
					core->ApplySpell(BackstabResRef, target, this, multiplier);
					//do we need this?
					BackstabResRef[0]='*';
					if (HasFeat(FEAT_CRIPPLING_STRIKE) ) {
						core->ApplySpell(resref_cripstr, target, this, multiplier);
					}
				}
				if (!sneakAttackDamage) {
					sneakAttackDamage = LuckyRoll(multiplier, 6, 0, 0, target);
					// ~Sneak Attack for %d~
					//displaymsg->DisplayRollStringName(25053, DMC_LIGHTGREY, this, extraDamage);
					if (core->HasFeedback(FT_COMBAT)) displaymsg->DisplayConstantStringValue (STR_BACKSTAB, DMC_WHITE, sneakAttackDamage);
				}
			} else {
				// weapon is unsuitable for sneak attack
				if (core->HasFeedback(FT_COMBAT)) displaymsg->DisplayConstantString (STR_BACKSTAB_BAD, DMC_WHITE);
			}
		}
	}
	return sneakAttackDamage;
}

int Actor::GetBackstabDamage(Actor *target, WeaponInfo &wi, int multiplier, int damage) const {
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

	if (!(core->HasFeature(GF_PROPER_BACKSTAB) && !IsBehind(target)) || (always&0x5)) {
		if (target->Modified[IE_DISABLEBACKSTAB]) {
			// The backstab seems to have failed
			if (core->HasFeedback(FT_COMBAT)) displaymsg->DisplayConstantString (STR_BACKSTAB_FAIL, DMC_WHITE);
			wi.backstabbing = false;
		} else {
			if (wi.backstabbing) {
				backstabDamage = multiplier * damage;
				// display a simple message instead of hardcoding multiplier names
				if (core->HasFeedback(FT_COMBAT)) displaymsg->DisplayConstantStringValue (STR_BACKSTAB, DMC_WHITE, multiplier);
			} else {
				// weapon is unsuitable for backstab
				if (core->HasFeedback(FT_COMBAT)) displaymsg->DisplayConstantString (STR_BACKSTAB_BAD, DMC_WHITE);
			}
		}
	}

	return backstabDamage;
}

bool Actor::UseItem(ieDword slot, ieDword header, Scriptable* target, ieDword flags, int damage)
{
	if (target->Type!=ST_ACTOR) {
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

	Actor *tar = (Actor *) target;
	CREItem *item = inventory.GetSlotItem(slot);
	if (!item)
		return false;

	ieResRef tmpresref;
	strnuprcpy(tmpresref, item->ItemResRef, sizeof(ieResRef)-1);

	Item *itm = gamedata->GetItem(tmpresref);
	if (!itm) {
		Log(WARNING, "Actor", "Invalid quick slot item: %s!", tmpresref);
		return false; //quick item slot contains invalid item resref
	}
	//item is depleted for today
	if (itm->UseCharge(item->Usages, header, false)==CHG_DAY) {
		return false;
	}

	Projectile *pro = itm->GetProjectile(this, header, target->Pos, slot, flags&UI_MISS);
	ChargeItem(slot, header, item, itm, flags&UI_SILENT, !(flags&UI_NOCHARGE));
	gamedata->FreeItem(itm,tmpresref, false);
	ResetCommentTime();
	if (pro) {
		pro->SetCaster(GetGlobalID(), ITEM_CASTERLEVEL);
		if (flags & UI_FAKE) {
			delete pro;
		} else if (((int)header < 0) && !(flags&UI_MISS)) { //using a weapon
			bool ranged = header == (ieDword)-2;
			ITMExtHeader *which = itm->GetWeaponHeader(ranged);
			Effect* AttackEffect = EffectQueue::CreateEffect(fx_damage_ref, damage, (weapon_damagetype[which->DamageType])<<16, FX_DURATION_INSTANT_LIMITED);
			AttackEffect->Projectile = which->ProjectileAnimation;
			AttackEffect->Target = FX_TARGET_PRESET;
			AttackEffect->Parameter3 = 1;
			if (pstflags) {
				AttackEffect->IsVariable = GetCriticalType();
			} else {
				AttackEffect->IsVariable = flags&UI_CRITICAL;
			}
			pro->GetEffects()->AddEffect(AttackEffect, true);
			if (ranged) {
				fxqueue.AddWeaponEffects(pro->GetEffects(), fx_ranged_ref);
			} else {
				// TODO: EEs add a bit to fx_melee for only applying with monk fists: parameter2 of 4 (no other value supported)
				fxqueue.AddWeaponEffects(pro->GetEffects(), fx_melee_ref);
				// ignore timestop
				pro->TFlags |= PTF_TIMELESS;
			}
			//AddEffect created a copy, the original needs to be scrapped
			delete AttackEffect;
			attackProjectile = pro;
		} else //launch it now as we are not attacking
			GetCurrentArea()->AddProjectile(pro, Pos, tar->GetGlobalID(), false);
		return true;
	}
	return false;
}

void Actor::ChargeItem(ieDword slot, ieDword header, CREItem *item, Item *itm, bool silent, bool expend)
{
	if (!itm) {
		item = inventory.GetSlotItem(slot);
		if (!item)
			return;
		itm = gamedata->GetItem(item->ItemResRef, true);
	}
	if (!itm) {
		Log(WARNING, "Actor", "Invalid quick slot item: %s!", item->ItemResRef);
		return; //quick item slot contains invalid item resref
	}

	if (IsSelected()) {
		core->SetEventFlag( EF_ACTION );
	}

	if (!silent) {
		ieByte stance = AttackStance;
		for (int i=0;i<animcount;i++) {
			if ( strnicmp(item->ItemResRef, itemanim[i].itemname, 8) == 0) {
				stance = itemanim[i].animation;
			}
		}
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

void Actor::InitButtons(ieDword cls, bool forced)
{
	if (!PCStats) {
		return;
	}
	if ( (PCStats->QSlots[0]!=0xff) && !forced) {
		return;
	}

	ActionButtonRow myrow;
	if (cls >= (ieDword) classcount) {
		memcpy(&myrow, &DefaultButtons, sizeof(ActionButtonRow));
		for (int i=0;i<extraslots;i++) {
			if (cls==OtherGUIButtons[i].clss) {
				memcpy(&myrow, &OtherGUIButtons[i].buttons, sizeof(ActionButtonRow));
				break;
			}
		}
	} else {
		memcpy(&myrow, GUIBTDefaults+cls, sizeof(ActionButtonRow));
	}
	SetActionButtonRow(myrow);
}

void Actor::SetFeat(unsigned int feat, int mode)
{
	if (feat>=MAX_FEATS) {
		return;
	}
	ieDword mask = 1<<(feat&31);
	ieDword idx = feat>>5;
	switch (mode) {
		case OP_SET: case OP_OR:
			BaseStats[IE_FEATS1+idx]|=mask;
			break;
		case OP_NAND:
			BaseStats[IE_FEATS1+idx]&=~mask;
			break;
		case OP_XOR:
			BaseStats[IE_FEATS1+idx]^=mask;
			break;
	}
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
		SetFeat(feat, OP_OR);
		if (featstats[feat]) SetBase(featstats[feat], value);
	} else {
		SetFeat(feat, OP_NAND);
		if (featstats[feat]) SetBase(featstats[feat], 0);
	}

	if (init) {
		 ApplyFeats();
	}
}

void Actor::SetUsedWeapon(const char (&AnimationType)[2], ieWord* MeleeAnimation, int wt)
{
	memcpy(WeaponRef, AnimationType, sizeof(WeaponRef) );
	if (wt != -1) WeaponType = wt;
	if (!anims)
		return;
	anims->SetWeaponRef(AnimationType);
	anims->SetWeaponType(WeaponType);
	SetAttackMoveChances(MeleeAnimation);
	if (InParty) {
		//update the paperdoll weapon animation
		core->SetEventFlag(EF_UPDATEANIM);
	}
	WeaponInfo wi;
	ITMExtHeader *header = GetWeapon(wi);

	if(header && ((header->AttackType == ITEM_AT_BOW) ||
		(header->AttackType == ITEM_AT_PROJECTILE && header->ProjectileQualifier))) {
		ITMExtHeader* projHeader = GetRangedWeapon(wi);
		if (projHeader->ProjectileQualifier == 0) return; /* no ammo yet? */
		AttackStance = IE_ANI_SHOOT;
		anims->SetRangedType(projHeader->ProjectileQualifier-1);
		//bows ARE one handed, from an anim POV at least
		anims->SetWeaponType(IE_ANI_WEAPON_1H);
		return;
	}
	if(header && (header->AttackType == ITEM_AT_PROJECTILE)) {
		AttackStance = IE_ANI_ATTACK_SLASH; //That's it!!
		return;
	}
	AttackStance = IE_ANI_ATTACK;
}

void Actor::SetUsedShield(const char (&AnimationType)[2], int wt)
{
	memcpy(ShieldRef, AnimationType, sizeof(ShieldRef) );
	if (wt != -1) WeaponType = wt;
	if (AnimationType[0] == ' ' || AnimationType[0] == 0)
		if (WeaponType == IE_ANI_WEAPON_2W)
			WeaponType = IE_ANI_WEAPON_1H;

	if (!anims)
		return;
	anims->SetOffhandRef(AnimationType);
	anims->SetWeaponType(WeaponType);
	if (InParty) {
		//update the paperdoll weapon animation
		core->SetEventFlag(EF_UPDATEANIM);
	}
}

void Actor::SetUsedHelmet(const char (&AnimationType)[2])
{
	memcpy(HelmetRef, AnimationType, sizeof(HelmetRef) );
	if (!anims)
		return;
	anims->SetHelmetRef(AnimationType);
	if (InParty) {
		//update the paperdoll weapon animation
		core->SetEventFlag(EF_UPDATEANIM);
	}
}

// initializes the fist data the first time it is called
void Actor::SetupFistData() const
{
	if (FistRows >= 0) {
		return;
	}

	FistRows = 0;
	AutoTable fist("fistweap");
	if (fist) {
		strnlwrcpy(DefaultFist, fist->QueryDefault(), 8);
		FistRows = fist->GetRowCount();
		fistres = new FistResType[FistRows];
		fistresclass = new int[FistRows];
		for (int i = 0; i < FistRows; i++) {
			int maxcol = fist->GetColumnCount(i) - 1;
			for (int cols = 0; cols < MAX_LEVEL; cols++) {
				strnlwrcpy(fistres[i][cols], fist->QueryField(i, cols > maxcol ? maxcol : cols), 8);
			}
			fistresclass[i] = atoi(fist->GetRowName(i));
		}
	}

}

void Actor::SetupFist()
{
	int slot = core->QuerySlot( 0 );
	assert (core->QuerySlotEffects(slot)==SLOT_EFFECT_FIST);
	int row = GetBase(fiststat);
	int col = GetXPLevel(false);

	if (col>MAX_LEVEL) col=MAX_LEVEL;
	if (col<1) col=1;

	SetupFistData();

	const char *ItemResRef = DefaultFist;
	for (int i = 0;i<FistRows;i++) {
		if (fistresclass[i] == row) {
			ItemResRef = fistres[i][col];
		}
	}
	CREItem *currentFist = inventory.GetSlotItem(slot);
	if (!currentFist || stricmp(currentFist->ItemResRef, ItemResRef)) {
		inventory.SetSlotItemRes(ItemResRef, slot);
	}
}

static ieDword ResolveTableValue(const char *resref, ieDword stat, ieDword mcol, ieDword vcol) {
	long ret = 0;
	//don't close this table, it can mess with the guiscripts
	int table = gamedata->LoadTable(resref);
	if (table == -1) return 0;
	Holder<TableMgr> tm = gamedata->GetTable(table);
	if (tm) {
		unsigned int row;
		if (mcol == 0xff) {
			row = stat;
		} else {
			row = tm->FindTableValue(mcol, stat);
			if (row==0xffffffff) {
				return 0;
			}
		}
		if (valid_number(tm->QueryField(row, vcol), ret)) {
			return (ieDword) ret;
		}
	}

	return 0;
}

int Actor::CheckUsability(const Item *item) const
{
	ieDword itembits[2]={item->UsabilityBitmask, item->KitUsability};
	int kitignore = 0;

	for (int i=0;i<usecount;i++) {
		ieDword itemvalue = itembits[itemuse[i].which];
		ieDword stat = GetStat(itemuse[i].stat);
		ieDword mcol = itemuse[i].mcol;
		//if we have a kit, we just use its index for the lookup
		if (itemuse[i].stat==IE_KIT) {
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

		if (!iwd2class && itemuse[i].stat == IE_CLASS) {
			// account for inactive duals
			stat = GetActiveClass();
		}

		if (iwd2class && itemuse[i].stat == IE_CLASS) {
			// in iwd2 any class mixin can enable the use, but the stat only holds the first class;
			// it also means we shouldn't check all kits (which we do last)!
			// Eg. a paladin of Mystra/sorcerer is allowed to use wands,
			// but the kit check would fail, since paladins and their kits aren't.
			stat = GetClassMask();
			if (stat & ~itemvalue) {
				if (Modified[IE_KIT] == 0) continue;
			} else {
				return STR_CANNOT_USE_ITEM;
			}

			// classes checked out, but we're kitted ...
			// ignore kits from "unusable" classes
			for (int j=0; j < ISCLASSES; j++) {
				if (Modified[levelslotsiwd2[j]] == 0) continue;
				if ((1<<(classesiwd2[j] - 1)) & ~itemvalue) continue;

				std::vector<ieDword> kits = class2kits[classesiwd2[j]].ids;
				std::vector<ieDword>::iterator it = kits.begin();
				for ( ; it != kits.end(); it++) {
					kitignore |= *it;
				}
			}
			continue;
		}

		stat = ResolveTableValue(itemuse[i].table, stat, mcol, itemuse[i].vcol);

no_resolve:
		if (stat&itemvalue) {
			//print("failed usability: itemvalue %d, stat %d, stat value %d", itemvalue, itemuse[i].stat, stat);
			return STR_CANNOT_USE_ITEM;
		}
	}

	return 0;
}

//this one is the same, but returns strrefs based on effects
ieStrRef Actor::Disabled(ieResRef name, ieDword type) const
{
	Effect *fx;

	fx = fxqueue.HasEffectWithResource(fx_cant_use_item_ref, name);
	if (fx) {
		return fx->Parameter1;
	}

	fx = fxqueue.HasEffectWithParam(fx_cant_use_item_type_ref, type);
	if (fx) {
		return fx->Parameter1;
	}
	return 0;
}

//checks usability only
int Actor::Unusable(const Item *item) const
{
	if (!GetStat(IE_CANUSEANYITEM)) {
		int unusable = CheckUsability(item);
		if (unusable) {
			return unusable;
		}
	}
	// iesdp says this is always checked?
	if (item->MinLevel>GetXPLevel(true)) {
		return STR_CANNOT_USE_ITEM;
	}

	if (!CheckAbilities) {
		return 0;
	}

	if (item->MinStrength>GetStat(IE_STR)) {
		return STR_CANNOT_USE_ITEM;
	}

	if (item->MinStrength==18) {
		if (GetStat(IE_STR)==18) {
			if (item->MinStrengthBonus>GetStat(IE_STREXTRA)) {
				return STR_CANNOT_USE_ITEM;
			}
		}
	}

	if (item->MinIntelligence>GetStat(IE_INT)) {
		return STR_CANNOT_USE_ITEM;
	}
	if (item->MinDexterity>GetStat(IE_DEX)) {
		return STR_CANNOT_USE_ITEM;
	}
	if (item->MinWisdom>GetStat(IE_WIS)) {
		return STR_CANNOT_USE_ITEM;
	}
	if (item->MinConstitution>GetStat(IE_CON)) {
		return STR_CANNOT_USE_ITEM;
	}
	if (item->MinCharisma>GetStat(IE_CHR)) {
		return STR_CANNOT_USE_ITEM;
	}
	//note, weapon proficiencies shouldn't be checked here
	//missing proficiency causes only attack penalty
	return 0;
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
bool Actor::SetSpellState(unsigned int spellstate)
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
	int idx;

	idx = projectile/32;
	if (idx>ProjectileSize) {
		return 0;
	}
	return projectileImmunity[idx]&(1<<(projectile&31));
}

void Actor::AddProjectileImmunity(ieDword projectile)
{
	projectileImmunity[projectile/32]|=1<<(projectile&31);
}

//2nd edition rules
void Actor::CreateDerivedStatsBG()
{
	int turnundeadlevel = 0;
	int classid = BaseStats[IE_CLASS];

	//this works only for PC classes
	if (classid>=CLASS_PCCUTOFF) return;

	//recalculate all level based changes
	pcf_level(this,0,0);

	// barbarian immunity to backstab was hardcoded
	if (GetBarbarianLevel()) {
		BaseStats[IE_DISABLEBACKSTAB] = 1;
	}

	for (int i=0;i<ISCLASSES;i++) {
		int tmp;

		if (classesiwd2[i]>=(ieDword) classcount) continue;
		int tl = turnlevels[classesiwd2[i]];
		if (tl) {
			tmp = GetClassLevel(i)+1-tl;
			//adding up turn undead levels, but this is probably moot
			//anyway, you will be able to create custom priest/paladin classes
			if (tmp>0) {
				turnundeadlevel+=tmp;
			}
		}
	}

	ieDword backstabdamagemultiplier=GetThiefLevel();
	if (backstabdamagemultiplier) {
		// swashbucklers can't backstab, but backstab.2da only has THIEF in it
		if (BaseStats[IE_KIT] == KIT_SWASHBUCKLER) {
			backstabdamagemultiplier = 1;
		} else {
			AutoTable tm("backstab");
			//fallback to a general algorithm (bg2 backstab.2da version) if we can't find backstab.2da
			// assassin's AP_SPCL332 (increase backstab by one) is not effecting this at all,
			// it's just applied later
			// stalkers work by just using the effect, since they're not thieves
			if (tm)	{
				ieDword cols = tm->GetColumnCount();
				if (backstabdamagemultiplier >= cols) backstabdamagemultiplier = cols;
				backstabdamagemultiplier = atoi(tm->QueryField(0, backstabdamagemultiplier));
			} else {
				backstabdamagemultiplier = (backstabdamagemultiplier+7)/4;
			}
			if (backstabdamagemultiplier>5) backstabdamagemultiplier=5;
		}
	}

	weapSlotCount = numWeaponSlots[GetActiveClass()];
	ReinitQuickSlots();

	// monk's level dictated ac and ac vs missiles bonus
	// attacks per round bonus will be handled elsewhere, since it only applies to fist apr
	if (isclass[ISMONK]&(1<<classid)) {
		unsigned int level = GetMonkLevel()-1;
		if (level < monkbon_cols) {
			AC.SetNatural(DEFAULTAC - monkbon[1][level]);
			BaseStats[IE_ACMISSILEMOD] = - monkbon[2][level];
		}
	}

	BaseStats[IE_TURNUNDEADLEVEL]=turnundeadlevel;
	BaseStats[IE_BACKSTABDAMAGEMULTIPLIER]=backstabdamagemultiplier;
	BaseStats[IE_LAYONHANDSAMOUNT]=GetPaladinLevel()*2;
}

//3rd edition rules
void Actor::CreateDerivedStatsIWD2()
{
	int i;
	int turnundeadlevel = 0;
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

	for (i=0;i<ISCLASSES;i++) {
		int tmp;

		if (classesiwd2[i]>=(ieDword) classcount) continue;
		int tl = turnlevels[classesiwd2[i]];
		if (tl) {
			tmp = GetClassLevel(i)+1-tl;
			if (tmp>0) {
				//the levels add up (checked)
				turnundeadlevel+=tmp;
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
			multiclass = multi[cls];
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
	return core->CountBits(Modified[IE_MC_FLAGS] & MC_WAS_ANY) == 1;
}

Actor *Actor::CopySelf(bool mislead) const
{
	Actor *newActor = new Actor();

	newActor->SetName(GetName(0),0);
	newActor->SetName(GetName(1),1);
	newActor->version = version;
	memcpy(newActor->BaseStats, BaseStats, sizeof(BaseStats) );
	// illusions aren't worth any xp and don't explore
	newActor->BaseStats[IE_XPVALUE] = 0;
	newActor->BaseStats[IE_EXPLORE] = 0;

	//IF_INITIALIZED shouldn't be set here, yet
	newActor->SetMCFlag(MC_EXPORTABLE, OP_NAND);

	//the creature importer does this too
	memcpy(newActor->Modified,newActor->BaseStats, sizeof(Modified) );

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

	//copy the running effects
	EffectQueue *newFXQueue = fxqueue.CopySelf();

	area->AddActor(newActor, true);
	newActor->SetPosition( Pos, CC_CHECK_IMPASSABLE, 0 );
	newActor->SetOrientation(GetOrientation(), false);
	newActor->SetStance( IE_ANI_READY );

	//and apply them
	newActor->RefreshEffects(newFXQueue);
	return newActor;
}

//high level function, used by scripting
ieDword Actor::GetLevelInClass(ieDword classid) const
{
	if (version==22) {
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
ieDword Actor::GetClassLevel(const ieDword isclass) const
{
	if (isclass>=ISCLASSES)
		return 0;

	//return iwd2 value if appropriate
	if (version==22)
		return BaseStats[levelslotsiwd2[isclass]];

	//houston, we got a problem!
	if (!levelslots || !dualswap)
		return 0;

	//only works with PC's
	ieDword	classid = BaseStats[IE_CLASS]-1;
	if (!HasPlayerClass() || !levelslots[classid])
		return 0;

	//handle barbarians specially, since they're kits and not in levelslots
	if ((isclass == ISBARBARIAN) && levelslots[classid][ISFIGHTER] && (BaseStats[IE_KIT] == KIT_BARBARIAN)) {
		return BaseStats[IE_LEVEL];
	}

	//get the levelid (IE_LEVEL,*2,*3)
	ieDword levelid = levelslots[classid][isclass];
	if (!levelid)
		return 0;

	//do dual-swap
	if (IsDualClassed()) {
		//if the old class is inactive, and it is the class
		//being searched for, return 0
		if (IsDualInactive() && ((Modified[IE_MC_FLAGS]&MC_WAS_ANY)==(ieDword)mcwasflags[isclass]))
			return 0;
	}
	return BaseStats[levelid];
}

bool Actor::IsDualInactive() const
{
	if (!IsDualClassed()) return 0;

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
	return (ieDword)dualswap[tmpclass]==(Modified[IE_MC_FLAGS]&MC_WAS_ANY);
}

ieDword Actor::GetWarriorLevel() const
{
	if (!IsWarrior()) return 0;

	ieDword warriorlevels[4] = {
		GetBarbarianLevel(),
		GetFighterLevel(),
		GetPaladinLevel(),
		GetRangerLevel()
	};

	ieDword highest = 0;
	for (int i=0; i<4; i++) {
		if (warriorlevels[i] > highest) {
			highest = warriorlevels[i];
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
		memcpy(LastArea, Area, 8);
		memset(UsedExit, 0, sizeof(ieVariable));
		if (LastExit) {
			const char *ipName = NULL;
			Scriptable *ip = area->GetInfoPointByGlobalID(LastExit);
			if (ip) ipName = ip->GetScriptName();
			if (ipName && ipName[0]) {
				snprintf(UsedExit, sizeof(ieVariable), "%s", ipName);
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
int Actor::LuckyRoll(int dice, int size, int add, ieDword flags, Actor* opponent) const
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
		int roll = core->Roll(1, dice*size, 0);
		if (critical && (roll == 1 || roll == size)) {
			return roll;
		} else {
			return add + dice * (size + bonus) / 2;
		}
	}

	int roll, result = 0, misses = 0, hits = 0;
	for (int i = 0; i < dice; i++) {
		roll = core->Roll(1, size, 0);
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

		delete newfx;

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
	delete newfx;
}

void Actor::ResetState()
{
	CureInvisibility();
	CureSanctuary();
	SetModal(MS_NONE);
	ResetCommentTime();
}

// doesn't check the range, but only that the azimuth and the target
// orientation match with a +/-2 allowed difference
bool Actor::IsBehind(Actor* target) const
{
	unsigned char tar_orient = target->GetOrientation();
	// computed, since we don't care where we face
	unsigned char my_orient = GetOrient(target->Pos, Pos);

	signed char diff;
	for (int i=-2; i <= 2; i++) {
		diff = my_orient+i;
		if (diff >= MAX_ORIENT) diff -= MAX_ORIENT;
		if (diff <= -1) diff += MAX_ORIENT;
		if (diff == (signed)tar_orient) return true;
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
	case MS_BATTLESONG:
		if (GetBardLevel()) {
			return !(Modified[IE_STATE_ID] & STATE_SILENCED);
		}
		return false;
	case MS_DETECTTRAPS:
		if (Modified[IE_TRAPS]<=0) return false;
		return true;
	case MS_TURNUNDEAD:
		if (Modified[IE_TURNUNDEADLEVEL]<=0) return false;
			return true;
	case MS_STEALTH:
			return TryToHide();
	case MS_NONE:
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
	delete newfx;

	if (!third) {
		return;
	}

	int bonus = actor->GetAbilityBonus(IE_DEX);
	switch (reason) {
		case 0:
			// ~Failed hide in shadows check! Hide in shadows check %d vs. D20 roll %d (%d Dexterity ability modifier)~
			displaymsg->DisplayRollStringName(39300, DMC_LIGHTGREY, actor, skill-bonus, roll, bonus);
			break;
		case 1:
			// ~Failed hide in shadows because you were seen by creature! Hide in Shadows check %d vs. creature's Level+Wisdom+Race modifier  %d + %d D20 Roll.~
			displaymsg->DisplayRollStringName(39298, DMC_LIGHTGREY, actor, skill, targetDC, roll);
			break;
		case 2:
			// ~Failed hide in shadows because you were heard by creature! Hide in Shadows check %d vs. creature's Level+Wisdom+Race modifier  %d + %d D20 Roll.~
			displaymsg->DisplayRollStringName(39297, DMC_LIGHTGREY, actor, skill, targetDC, roll);
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

	ieDword skill;
	if (core->HasFeature(GF_HAS_HIDE_IN_SHADOWS)) {
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
	if (!continuation) VerbalConstant(VB_HIDE);
	if (!third) return true;

	// ~Successful hide in shadows check! Hide in shadows check %d vs. D20 roll %d (%d Dexterity ability modifier)~
	displaymsg->DisplayRollStringName(39299, DMC_LIGHTGREY, this, skill/7, roll, GetAbilityBonus(IE_DEX));
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
			displaymsg->DisplayRollStringName(28379, DMC_LIGHTGREY, this, skill, targetDC, roll);
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
			displaymsg->DisplayRollStringName(112, DMC_LIGHTGREY, this, skill, targetDC, roll);
		}
	}

	// NOTE: the original checked lightness for creatures that were both deaf and blind (or if nobody is around)
	// check TryToHide if it ever becomes important (crelight.2da)

	return true;
}

//cannot target actor (used by GUI)
bool Actor::Untargetable(ieResRef spellRef) const
{
	if (spellRef[0]) {
		Spell *spl = gamedata->GetSpell(spellRef, true);
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
	ieResRef spellres;

	ResolveSpellName(spellres, spellnum);

	//cheap substitute of the original hardcoded feature, returns true if already affected by the exact spell
	//no (spell)state checks based on every effect in the spell
	//FIXME: create a more compatible version if needed
	if (fxqueue.HasSource(spellres)) return true;
	//return true if caster cannot cast
	if (!caster->CanCast(spellres, false)) return true;

	if (!range) return false;

	int srange = GetSpellDistance(spellres, caster);
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
	if (col < (*it).size()) {
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
	if (col < (*it).size()) {
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
	Game *game = core->GetGame();
	if (bored_time) {
		nextBored = game->GameTime + core->Roll(1, 30, bored_time);
		nextComment = game->GameTime + core->Roll(5, 1000, bored_time/2);
	} else {
		nextBored = 0;
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
	int armorSlot = inventory.GetArmorSlot();
	CREItem *armorItem = inventory.GetSlotItem(armorSlot);
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
		shieldPenalty -= 1;
		if (shieldPenalty < 0) {
			shieldPenalty = 0;
		}
	}
	if (profcheck) {
		if (HasFeat(FEAT_SHIELD_PROF)) {
			shieldPenalty = 0;
		} else {
			penalty += shieldPenalty;
		}
	} else {
		penalty += shieldPenalty;
	}
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
	bool canSeeInvisibles = false;
	if (checker && checker->Type == ST_ACTOR) {
		canSeeInvisibles = ((Actor *) checker)->GetSafeStat(IE_SEEINVISIBLE);
	}
	bool invisible = GetSafeStat(IE_STATE_ID) & state_invisible;
	if (!canSeeInvisibles && (invisible || HasSpellState(SS_SANCTUARY))) {
		return true;
	}

	return false;
}

int Actor::UpdateAnimationID(bool derived)
{
	if (avCount<0) return 1;
	// the base animation id
	int AnimID = avBase;
	int StatID = derived?GetSafeStat(IE_ANIMATION_ID):avBase;
	if (AnimID<0 || StatID<AnimID || StatID>AnimID+0x1000) return 1; //no change
	if (!InParty) return 1; //too many bugs caused by buggy game data, we change only PCs

	// tables for additive modifiers of the animation id (race, gender, class)
	for (int i = 0; i < avCount; i++) {
		const TableMgr *tm = avPrefix[i].avtable.ptr();
		if (!tm) {
			return -3;
		}
		StatID = avPrefix[i].stat;
		StatID = derived?GetSafeStat(StatID):GetBase( StatID );

		const char *poi = tm->QueryField( StatID );
		AnimID += strtoul( poi, NULL, 0 );
	}
	if (BaseStats[IE_ANIMATION_ID]!=(unsigned int) AnimID) {
		SetBase(IE_ANIMATION_ID, (unsigned int) AnimID);
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

void Actor::MovementCommand(char *command)
{
	UseExit(0);
	Stop();
	AddAction( GenerateAction( command ) );
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

	wchar_t tmpstr[10];
	swprintf(tmpstr, 10, L"%d/%d\0", Modified[IE_HITPOINTS], Modified[IE_MAXHITPOINTS]);
	SetOverheadText(tmpstr);
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
	if (neighbours.size() == 0) return true;

	// so there is someone out to get us and we should do the real concentration check
	int roll = LuckyRoll(1, 20, 0);
	// TODO: the manual replaces the con bonus with an int one (verify!)
	int concentration = GetStat(IE_CONCENTRATION);
	int bonus = GetAbilityBonus(IE_INT);
	if (HasFeat(FEAT_COMBAT_CASTING)) {
		bonus += 4;
	}

	Spell* spl = gamedata->GetSpell(SpellResRef, true);
	if (!spl) return true;
	int spellLevel = spl->SpellLevel;
	gamedata->FreeSpell(spl, SpellResRef, false);

	if (roll + concentration + bonus < 15 + spellLevel) {
		if (InParty) {
			displaymsg->DisplayRollStringName(39258, DMC_LIGHTGREY, this, roll + concentration, 15 + spellLevel, bonus);
		} else {
			displaymsg->DisplayRollStringName(39265, DMC_LIGHTGREY, this);
		}
		return false;
	} else {
		if (InParty) {
			// ~Successful spell casting concentration check! Check roll %d vs. difficulty %d (%d bonus)~
			displaymsg->DisplayRollStringName(39257, DMC_LIGHTGREY, this, roll + concentration, 15 + spellLevel, bonus);
		}
	}
	return true;
}

// shorthand wrapper for throw-away effects
void Actor::ApplyEffectCopy(Effect *oldfx, EffectRef &newref, Scriptable *Owner, ieDword param1, ieDword param2)
{
	Effect *newfx = EffectQueue::CreateEffectCopy(oldfx, newref, param1, param2);
	if (newfx) {
		core->ApplyEffect(newfx, this, Owner);
		delete newfx;
	} else {
		Log(ERROR, "Actor", "Failed to create effect copy for %s! Target: %s, Owner: %s", newref.Name, GetName(1), Owner->GetName(1));
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
	for (int isclass = 0; isclass < ISCLASSES; isclass++) {
		oldclass = classesiwd2[isclass];
		if (mcwas == mcwasflags[isclass]) break;
	}
	if (!oldclass) {
		error("Actor", "Actor %s has incorrect MC_WAS flags (%x)!", GetName(1), mcwas);
	}

	int newclassmask = multiclass & ~(1 << (oldclass - 1));
	for (int newclass = 1, mask = 1; mask <= newclassmask; newclass++, mask <<= 1) {
		if (newclassmask == mask) return newclass;
	}

	// can be hit when starting a dual class
	Log(ERROR, "Actor", "Dual-classed actor %s (old class %d) has wrong multiclass bits (%d), using old class!", GetName(1), oldclass, multiclass);
	return oldclass;
}

// like IsDualInactive(), but accounts for the possibility of the active (second) class being kitted
bool Actor::IsKitInactive() const
{
	if (third) return false;
	if (!IsDualInactive()) return false;

	ieDword baseclass = GetActiveClass();
	ieDword kit = GetStat(IE_KIT);
	std::vector<ieDword> kits = class2kits[baseclass].ids;
	std::vector<ieDword>::iterator it = kits.begin();
	for (int idx = 0; it != kits.end(); it++, idx++) {
		if (kit & (*it)) return false;
	}
	return true;
}

// account for haste/slow affecting the metabolism (regeneration etc.)
// handled by AdjustedTicks in the original
unsigned int Actor::GetAdjustedTime(unsigned int time) const
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

ieDword Actor::GetClassID (const ieDword isclass) {
	return classesiwd2[isclass];
}

const char *Actor::GetClassName(ieDword classID) const
{
	return class2kits[classID].className;
}

// NOTE: returns first kit name for multikit chars
const char *Actor::GetKitName(ieDword kitID) const
{
	std::map<int, ClassKits>::iterator clskit = class2kits.begin();
	for (int cidx = 0; clskit != class2kits.end(); clskit++, cidx++) {
		std::vector<ieDword> kits = class2kits[cidx].ids;
		std::vector<ieDword>::iterator it = kits.begin();
		for (int kidx = 0; it != kits.end(); it++, kidx++) {
			if (kitID & (*it)) {
				return class2kits[cidx].kitNames[kidx];
			}
		}
	}
	return "";
}

void Actor::SetAnimatedTalking (unsigned int length) {
	remainingTalkSoundTime = std::max(remainingTalkSoundTime, length);
	lastTalkTimeCheckAt = GetTickCount();
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
	CREItem *itm = inventory.GetSlotItem(inventory.GetArmorSlot());
	if (!itm) return '1';
	Item *item = gamedata->GetItem(itm->ItemResRef, true);
	if (!item) return '1';
	bool wearingRobes = item->AnimationType[1] == 'W';

	if (mageAnimation ^ wearingRobes) return '1';
	return item->AnimationType[0];
}

const char* Actor::GetArmorSound() const
{
	// Character has mage animation or is a non-mage wearing mage robes
	if ((BaseStats[IE_ANIMATION_ID] & 0xF00) == 0x200) return "";
	char armorCode = GetArmorCode();
	if (armorCode == '1') {
		return "";
	}

	char *sound = new char[9];
	int maxChar = 6;
	if (armorCode == '4') maxChar = 8;
	if (IWDSound) {
		// all three iwds have this pattern: a_chain1-6, a_lthr1-6, a_plate1-8
		const char* suffixes = "12345678";
		int idx = RAND(0, maxChar-1);
		if (armorCode == '2') {
			strcpy(sound, "A_LTHR");
			sound[6] = suffixes[idx];
			sound[7] = '\0';
		} else if (armorCode == '3') {
			strcpy(sound, "A_CHAIN");
			sound[7] = suffixes[idx];
			sound[8] = '\0';
		} else { // 4
			strcpy(sound, "A_PLATE");
			sound[7] = suffixes[idx];
			sound[8] = '\0';
		}
	} else {
		// generate a 1 letter suffix or emulate an empty string
		// ARM_04G and ARM_04H exist, but couldn't be picked by the original function
		const char* suffixes = "abcdefgh";
		int idx = RAND(0, maxChar);
		char randomASCII = '\0';
		if (idx < maxChar) randomASCII = suffixes[idx];

		strcpy(sound, "ARM_0");
		sound[5] = armorCode;
		sound[6] = randomASCII;
		sound[7] = '\0';
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

	Game *game = core->GetGame();
	if (!game) return;
	if (game->CombatCounter) return;

	const char *armorSound = GetArmorSound();
	if (armorSound[0]) {
		core->GetAudioDrv()->Play(armorSound, SFX_CHAN_ARMOR, Pos.x, Pos.y);
		delete[] armorSound;
	}
}

bool Actor::ShouldModifyMorale() const
{
	// pst ignores it for pcs, treating it more like reputation
	return !pstflags || Modified[IE_EA] != EA_PC;
}

}
