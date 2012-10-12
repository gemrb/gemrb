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
#include "win32def.h"

#include "Audio.h" //pst (react to death sounds)
#include "Bitmap.h"
#include "DataFileMgr.h"
#include "DialogHandler.h" // checking for dialog
#include "Game.h"
#include "GlobalTimer.h"
#include "DisplayMessage.h"
#include "GameData.h"
#include "Image.h"
#include "Interface.h"
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
#include "System/StringBuffer.h"

#include <cassert>

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
static char **classabilities = NULL;
static int *turnlevels = NULL;
static int *booktypes = NULL;
static int *xpbonus = NULL;
static int *defaultprof = NULL;
static int xpbonustypes = -1;
static int xpbonuslevels = -1;
static int **levelslots = NULL;
static int *dualswap = NULL;
static int *multi = NULL;
static int *maxLevelForHpRoll = NULL;
static int *skillstats = NULL;
static int *skillabils = NULL;
static int skillcount = -1;
static int **afcomments = NULL;
static int afcount = -1;
static ieVariable CounterNames[4]={"GOOD","LAW","LADY","MURDER"};
//I keep the zero index the same as core rules (default setting)
static int dmgadjustments[6]={0, -50, -25, 0, 50, 100}; //default, easy, normal, core rules, hard, nightmare
//XP adjustments on easy setting (need research on the amount)
//Seems like bg1 halves xp, bg2 doesn't have any impact
static int xpadjustments[6]={0, 0, 0, 0, 0, 0};

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
static ieDword GameDifficulty = DIFF_CORE;
static ieDword NoExtraDifficultyDmg = 0;
//the chance to issue one of the rare select verbal constants
#define RARE_SELECT_CHANCE 5
//these are the max number of select sounds -- the size of the pool to choose from
#define NUM_RARE_SELECT_SOUNDS 2 //in bg and pst it is actually 4 TODO: check
#define NUM_SELECT_SOUNDS 6 //in bg1 this is 4 but doesn't need to be checked
#define NUM_MC_SELECT_SOUNDS 4 //number of main charater select sounds

//item usability array
struct ItemUseType {
	ieResRef table; //which table contains the stat usability flags
	ieByte stat;	//which actor stat we talk about
	ieByte mcol;	//which column should be matched against the stat
	ieByte vcol;	//which column has the bit value for it
	ieByte which;	//which item dword should be used (1 = kit)
};

static const char *skillcolumns[12]={
	"DRUIDSPELL", "CLERICSPELL", "MAGESPELL", NULL, "BARDSKILL", "THIEFSKILL",
	"LAYHANDS", "TURNLEVEL", "BOOKTYPE", "HATERACE", "ABILITIES", "NO_PROF",
};

static ieResRef featspells[ES_COUNT];
static ItemUseType *itemuse = NULL;
static int usecount = -1;
static bool pstflags = false;
static bool nocreate = false;
static bool third = false;
static bool raresnd = false;
//used in many places, but different in engines
static ieDword state_invisible = STATE_INVISIBLE;

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
	IE_LEVELPALADIN, IE_LEVELRANGER, IE_LEVELSORCEROR, IE_LEVELCLASS12, IE_LEVELCLASS13};

#define BGCLASSCNT 23
//fighter is the default level here
//fixme, make this externalized
//this map could probably be auto-generated BG2 class ID -> ISCLASS
static const int levelslotsbg[BGCLASSCNT]={ISFIGHTER, ISMAGE, ISFIGHTER, ISCLERIC, ISTHIEF,
	ISBARD, ISPALADIN, 0, 0, 0, 0, ISDRUID, ISRANGER, 0,0,0,0,0,0,ISSORCERER, ISMONK,
	ISCLASS12, ISCLASS13};
//this map could probably be auto-generated (isClass -> IWD2 class ID)
//autogenerating for non IWD2 now!!!
static unsigned int classesiwd2[ISCLASSES]={5, 11, 9, 1, 2, 3, 4, 6, 7, 8, 10, 12, 13};
//this map could probably be auto-generated (isClass -> IWD2 book ID)
static const unsigned int booksiwd2[ISCLASSES]={0, 1<<IE_IWD2_SPELL_WIZARD, 0, 0, 
 1<<IE_IWD2_SPELL_BARD, 1<<IE_IWD2_SPELL_CLERIC, 1<<IE_IWD2_SPELL_DRUID, 0,
 1<<IE_IWD2_SPELL_PALADIN, 1<<IE_IWD2_SPELL_RANGER, 1<<IE_IWD2_SPELL_SORCEROR, 0, 0};

//stat values are 0-255, so a byte is enough
static ieByte featstats[MAX_FEATS]={0
};
static ieByte featmax[MAX_FEATS]={0
};

//holds the wspecial table for weapon prof bonuses
#define WSPECIAL_COLS 3
static int wspecial_max = 0;
static int wspattack_rows = 0;
static int wspattack_cols = 0;
static int **wspecial = NULL;
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
static ieDword hc_locations=0x2ba80030;
static int hc_flags[OVERLAY_COUNT];
#define HC_INVISIBLE 1

static int *mxsplwis = NULL;
static int spllevels;

// thieving skill dexterity and race boni vectors
std::vector<std::vector<int> > skilldex;
std::vector<std::vector<int> > skillrac;

//for every game except IWD2 we need to reverse TOHIT
static int ReverseToHit=true;
static int CheckAbilities=false;

static EffectRef fx_sleep_ref = { "State:Helpless", -1 };
static EffectRef fx_cleave_ref = { "Cleave", -1 };
static EffectRef fx_tohit_vs_creature_ref = { "ToHitVsCreature", -1 };
static EffectRef fx_damage_vs_creature_ref = { "DamageVsCreature", -1 };
static EffectRef fx_mirrorimage_ref = { "MirrorImageModifier", -1 };
static EffectRef fx_set_charmed_state_ref = { "State:Charmed", -1 };
static EffectRef fx_cure_sleep_ref = { "Cure:Sleep", -1 };
//bg2 and iwd1
static EffectRef control_creature_ref = { "ControlCreature", -1 };
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

//used by iwd2
static ieResRef resref_cripstr={"cripstr"};
static ieResRef resref_dirty={"dirty"};

static const int weapon_damagetype[] = {DAMAGE_CRUSHING, DAMAGE_PIERCING,
	DAMAGE_CRUSHING, DAMAGE_SLASHING, DAMAGE_MISSILE, DAMAGE_STUNNING};

//internal flags for calculating to hit
#define WEAPON_FIST        0
#define WEAPON_MELEE       1
#define WEAPON_RANGED      2
#define WEAPON_STYLEMASK   15
#define WEAPON_LEFTHAND    16
#define WEAPON_USESTRENGTH 32
#define WEAPON_FINESSE     64
#define WEAPON_BYPASS      0x10000
#define WEAPON_KEEN        0x20000

/* counts the on bits in a number */
ieDword bitcount (ieDword n)
{
	ieDword count=0;
	while (n) {
		count += n & 0x1u;
		n >>= 1;
	}
			return count;
}

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
	PathTries = 0;
	TargetDoor = 0;
	attackProjectile = NULL;
	lastInit = 0;
	roundTime = 0;
	modalTime = 0;
	modalSpellLingering = 0;
	panicMode = PANIC_NONE;
	nextComment = 0;
	nextBored = 0;

	inventory.SetInventoryType(INVENTORY_CREATURE);
	Equipped = 0;
	EquippedHeader = 0;

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
	HomeLocation.x = 0;
	HomeLocation.y = 0;
	version = 0;
	//these are used only in iwd2 so we have to default them
	for(i=0;i<7;i++) {
		BaseStats[IE_HATEDRACE2+i]=0xff;
	}
	//this one is saved only for PC's
	ModalState = 0;
	//set it to a neutral value
	ModalSpell[0] = '*';
	LingeringModalSpell[0] = '*';
	BackstabResRef[0] = '*';
	//this one is not saved
	GotLUFeedback = false;
	RollSaves();
	WMLevelMod = 0;

	polymorphCache = NULL;
	memset(&wildSurgeMods, 0, sizeof(wildSurgeMods));
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
		core->StripLine( LongName, len );
	}
	if (type!=1) {
		ShortName = ( char * ) realloc( ShortName, len );
		memcpy( ShortName, ptr, len );
		core->StripLine( ShortName, len );
	}
}

void Actor::SetName(int strref, unsigned char type)
{
	if (type!=2) {
		if (LongName) free(LongName);
		LongName = core->GetString( strref, IE_STR_REMOVE_NEWLINE );
		LongStrRef = strref;
	}
	if (type!=1) {
		if (ShortName) free(ShortName);
		ShortName = core->GetString( strref, IE_STR_REMOVE_NEWLINE );
		ShortStrRef = strref;
	}
}

void Actor::SetAnimationID(unsigned int AnimID)
{
	//if the palette is locked, then it will be transferred to the new animation
	Palette *recover = NULL;

	if (anims) {
		if (anims->lockPalette) {
			recover = anims->palette[PAL_MAIN];
		}
		// Take ownership so the palette won't be deleted
		if (recover) {
			recover->IncRef();
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
	}
	//bird animations are not hindered by searchmap
	//only animtype==7 (bird) uses this feature
	//this is a hardcoded hack, but works for all engine type
	if (anims->GetAnimType()!=IE_ANI_BIRD) {
		BaseStats[IE_DONOTJUMP]=0;
	} else {
		BaseStats[IE_DONOTJUMP]=DNJ_BIRD;
	}
	SetCircleSize();
	anims->SetColors(BaseStats+IE_COLORS);

	//Speed is determined by the number of frames in each cycle of its animation
	// (beware! GetAnimation has side effects!)
	// TODO: we should have a more efficient way to look this up
	Animation** anim = anims->GetAnimation(IE_ANI_WALK, 0);
	if (anim && anim[0]) {
		SetBase(IE_MOVEMENTRATE, anim[0]->GetFrameCount()) ;
	} else {
		Log(WARNING, "Actor", "Unable to determine movement rate for animation %04x!", AnimID);
	}

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

	GameControl *gc = core->GetGameControl();
	if (UnselectableTimer) {
		color = &magenta;
		color_index = 4;
	} else if (Modified[IE_STATE_ID] & STATE_PANIC) {
		color = &yellow;
		color_index = 5;
	} else if (gc && gc->dialoghandler->targetID == GetGlobalID() && (gc->GetDialogueFlags()&DF_IN_DIALOG)) {
		color = &white;
		color_index = 3; //?? made up
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

			case EA_ENEMY:
			case EA_GOODBUTRED:
			case EA_EVILCUTOFF:
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

	int csize = anims->GetCircleSize() - 1;
	if (csize >= MAX_CIRCLE_SIZE)
		csize = MAX_CIRCLE_SIZE - 1;

	SetCircle( anims->GetCircleSize(), *color, core->GroundCircles[csize][color_index], core->GroundCircles[csize][(color_index == 0) ? 3 : color_index] );
}

static void ApplyClab_internal(Actor *actor, const char *clab, int level, bool remove)
{
	AutoTable table(clab);
	if (table) {
		int row = table->GetRowCount();
		for(int i=0;i<level;i++) {
			for (int j=0;j<row;j++) {
				const char *res = table->QueryField(j,i);
				if (res[0]=='*') continue;

				if (!memcmp(res,"AP_",3)) {
					if (remove) {
						actor->fxqueue.RemoveAllEffects(res+3);
					} else {
						core->ApplySpell(res+3, actor, actor, 0);
					}
				}
				else if (!memcmp(res,"GA_",3)) {
					if (remove) {
						actor->spellbook.RemoveSpell(res+3);
					} else {
						actor->LearnSpell(res+3, LS_MEMO);
					}
				}
				else if (!memcmp(res,"FA_",3)) {//iwd2 only
					//memorize these
					int x=atoi(res+3);
					ieResRef resref;
					ResolveSpellName(resref, x);
					actor->LearnSpell(resref, LS_MEMO);
				}
				else if (!memcmp(res,"FS_",3)) {//iwd2 only
					//don't memorize these
					int x=atoi(res+3);
					ieResRef resref;
					ResolveSpellName(resref, x);
					actor->LearnSpell(resref, 0);
				}
				else if (!memcmp(res,"RA_",3)) {//iwd2 only
					//remove ability
					int x=atoi(res+3);
					actor->spellbook.RemoveSpell(x);
				}
			}
		}
	}
}

#define BG2_KITMASK  0xffffc000
#define KIT_BASECLASS 0x4000
#define KIT_SWASHBUCKLER 0x100000
#define KIT_BARBARIAN 0x40000000

//TODO: make kitlist column 6 stored internally
static ieDword GetKitIndex (ieDword kit, const char *resref="kitlist")
{
	int kitindex = 0;

	if ((kit&BG2_KITMASK) == KIT_BASECLASS) {
		kitindex = kit&0xfff;
	}

	if (kitindex == 0) {
		Holder<TableMgr> tm = gamedata->GetTable(gamedata->LoadTable(resref) );
		if (tm) {
			kitindex = tm->FindTableValue(6, kit);
			if (kitindex < 0) {
				kitindex = 0;
			}
		}
	}

	return (ieDword)kitindex;
}

static ieDword GetKitUsability(ieDword kit, const char *resref="kitlist")
{
	if ((kit&BG2_KITMASK) == KIT_BASECLASS) {
		int kitindex = kit&0xfff;
		Holder<TableMgr> tm = gamedata->GetTable(gamedata->LoadTable(resref) );
		if (tm) {
			return strtol(tm->QueryField(kitindex, 6), NULL, 0 );
		}
	}
	if (kit&KIT_BASECLASS) return 0;
	return kit;
}

//applies a kit on the character (only bg2)
bool Actor::ApplyKit(bool remove)
{
	ieDword kit = GetStat(IE_KIT);
	ieDword kitclass = 0;
	ieDword row = GetKitIndex(kit);
	const char *clab = NULL;
	ieDword max = 0;

	if (row) {
		//kit abilities
		Holder<TableMgr> tm = gamedata->GetTable(gamedata->LoadTable("kitlist"));
		if (tm) {
			kitclass = (ieDword) atoi(tm->QueryField(row, 7));
			clab = tm->QueryField(row, 4);
		}
	}

	//multi class
	if (multiclass) {
		ieDword msk = 1;
		for(unsigned int i=1;(i<(unsigned int) classcount) && (msk<=multiclass);i++) {
			if (multiclass & msk) {
				max = GetLevelInClass(i);
				// don't apply/remove the old kit clab if the kit is disabled
				if (i==kitclass && !IsDualClassed()) {
					ApplyClab(clab, max, remove);
				} else {
					ApplyClab(classabilities[i], max, remove);
				}
			}
			msk+=msk;
		}
		return true;
	}
	//single class
	ieDword cls = GetStat(IE_CLASS);
	if (cls>=(ieDword) classcount) {
		return false;
	}
	max = GetLevelInClass(cls);
	if (kitclass==cls) {
		ApplyClab(clab, max, remove);
	} else {
		ApplyClab(classabilities[cls], max, remove);
	}
	return true;
}

void Actor::ApplyClab(const char *clab, ieDword max, bool remove)
{
	if (clab[0]!='*') {
		if (max) {
			//singleclass
			if (remove) {
				ApplyClab_internal(this, clab, max, true);
			} else {
				ApplyClab_internal(this, clab, max, true);
				ApplyClab_internal(this, clab, max, false);
			}
		}
	}
}

//call this when morale or moralebreak changed
//cannot use old or new value, because it is called two ways
void pcf_morale (Actor *actor, ieDword /*oldValue*/, ieDword /*newValue*/)
{
	if ((actor->Modified[IE_MORALE]<=actor->Modified[IE_MORALEBREAK]) && (actor->Modified[IE_MORALEBREAK] != 0) ) {
		actor->Panic(core->GetGame()->GetActorByGlobalID(actor->LastAttacker), core->Roll(1,3,0) );
	} else if (actor->Modified[IE_STATE_ID]&STATE_PANIC) {
		// recover from panic, since morale has risen again
		// but only if we have really just recovered, so panic from other
		// sources isn't affected
		if ((actor->Modified[IE_MORALE]-1 == actor->Modified[IE_MORALEBREAK]) || (actor->Modified[IE_MORALEBREAK] == 0) ) {
			actor->SetBaseBit(IE_STATE_ID, STATE_PANIC, false);
		}
	}
	//for new colour
	actor->SetCircleSize();
}

void pcf_ea (Actor *actor, ieDword /*oldValue*/, ieDword newValue)
{
	if (actor->Selected && (newValue>EA_GOODCUTOFF) ) {
		core->GetGame()->SelectActor(actor, false, SELECT_NORMAL);
	}
	actor->SetCircleSize();
}

//this is a good place to recalculate level up stuff
void pcf_level (Actor *actor, ieDword oldValue, ieDword newValue)
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
		actor->ApplyKit(false);
	}
	actor->GotLUFeedback = false;
}

void pcf_class (Actor *actor, ieDword /*oldValue*/, ieDword newValue)
{
	//Call forced initbuttons in old style systems, and soft initbuttons
	//in case of iwd2. Maybe we need a custom quickslots flag here.
	actor->InitButtons(newValue, !core->HasFeature(GF_LEVELSLOT_PER_CLASS));

	int sorcerer=0;
	if (newValue<(ieDword) classcount) {
		switch(booktypes[newValue]) {
		case 2: sorcerer = 1<<IE_SPELL_TYPE_WIZARD; break; //sorcerer
		case 3: sorcerer = 1<<IE_SPELL_TYPE_PRIEST; break; //divine caster with sorc. style spells
		case 5: sorcerer = 1<<IE_IWD2_SPELL_SHAPE; break;  //divine caster with sorc style shapes (iwd2 druid)
		default: break;
		}
	}
	actor->spellbook.SetBookType(sorcerer);
}

void pcf_animid(Actor *actor, ieDword /*oldValue*/, ieDword newValue)
{
	actor->SetAnimationID(newValue);
}

static const ieDword fullwhite[7]={ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT};

static const ieDword fullstone[7]={STONE_GRADIENT,STONE_GRADIENT,STONE_GRADIENT,STONE_GRADIENT,STONE_GRADIENT,STONE_GRADIENT,STONE_GRADIENT};

void pcf_state(Actor *actor, ieDword /*oldValue*/, ieDword State)
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
void pcf_extstate(Actor *actor, ieDword oldValue, ieDword State)
{
	if ((oldValue^State)&EXTSTATE_SEVEN_EYES) {
		ieDword mask = EXTSTATE_EYE_MIND;
		int eyeCount = 7;
		for (int i=0;i<7;i++)
		{
			if (State&mask) eyeCount--;
			mask<<=1;
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

void pcf_hitpoint(Actor *actor, ieDword /*oldValue*/, ieDword hp)
{
	int maxhp = (signed) actor->GetSafeStat(IE_MAXHITPOINTS);
	if ((signed) hp>maxhp) {
		hp=maxhp;
	}

	int minhp = (signed) actor->GetSafeStat(IE_MINHITPOINTS);
	if (minhp && (signed) hp<minhp) {
		hp=minhp;
	}
	if ((signed) hp<=0) {
		actor->Die(NULL);
	}
	actor->BaseStats[IE_HITPOINTS]=hp;
	actor->Modified[IE_HITPOINTS]=hp;
	if (actor->InParty) core->SetEventFlag(EF_PORTRAIT);
}

void pcf_maxhitpoint(Actor *actor, ieDword /*oldValue*/, ieDword hp)
{
	if ((signed) hp<(signed) actor->BaseStats[IE_HITPOINTS]) {
		actor->BaseStats[IE_HITPOINTS]=hp;
		//passing 0 because it is ignored anyway
		pcf_hitpoint(actor, 0, hp);
	}
}

void pcf_minhitpoint(Actor *actor, ieDword /*oldValue*/, ieDword hp)
{
	if ((signed) hp>(signed) actor->BaseStats[IE_HITPOINTS]) {
		actor->BaseStats[IE_HITPOINTS]=hp;
		//passing 0 because it is ignored anyway
		pcf_hitpoint(actor, 0, hp);
	}
}

void pcf_stat(Actor *actor, ieDword newValue, ieDword stat)
{
	if ((signed) newValue<=0) {
		if (DeathOnZeroStat) {
			actor->Die(NULL);
		} else {
			actor->Modified[stat]=1;
		}
	}
}

void pcf_stat_str(Actor *actor, ieDword /*oldValue*/, ieDword newValue)
{
	pcf_stat(actor, newValue, IE_STR);
}

void pcf_stat_int(Actor *actor, ieDword /*oldValue*/, ieDword newValue)
{
	pcf_stat(actor, newValue, IE_INT);
}

void pcf_stat_wis(Actor *actor, ieDword /*oldValue*/, ieDword newValue)
{
	pcf_stat(actor, newValue, IE_WIS);
}

void pcf_stat_dex(Actor *actor, ieDword /*oldValue*/, ieDword newValue)
{
	pcf_stat(actor, newValue, IE_DEX);
}

void pcf_stat_con(Actor *actor, ieDword /*oldValue*/, ieDword newValue)
{
	pcf_stat(actor, newValue, IE_CON);
	pcf_hitpoint(actor, 0, actor->BaseStats[IE_HITPOINTS]);
}

void pcf_stat_cha(Actor *actor, ieDword /*oldValue*/, ieDword newValue)
{
	pcf_stat(actor, newValue, IE_CHR);
}

void pcf_xp(Actor *actor, ieDword /*oldValue*/, ieDword /*newValue*/)
{
	// check if we reached a new level
	unsigned int pc = actor->InParty;
	if (pc && !actor->GotLUFeedback) {
		char varname[16];
		sprintf(varname, "CheckLevelUp%d", pc);
		core->GetGUIScriptEngine()->RunFunction("GUICommonWindows", "CheckLevelUp", true, pc);
		ieDword NeedsLevelUp = 0;
		core->GetDictionary()->Lookup(varname, NeedsLevelUp);
		if (NeedsLevelUp == 1) {
			displaymsg->DisplayConstantStringName(STR_LEVELUP, DMC_WHITE, actor);
			actor->GotLUFeedback = true;
		}
	}
}

void pcf_gold(Actor *actor, ieDword /*oldValue*/, ieDword /*newValue*/)
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

	// always draw it for party members; the rest must not be invisible to have it;
	// this is just a guess, maybe there are extra conditions (MC_HIDDEN? IE_AVATARREMOVAL?)
	if (hc_flags[idx] & HC_INVISIBLE && (!actor->InParty && actor->Modified[IE_STATE_ID] & state_invisible)) {
		return;
	}

	if (flag) {
		sca->ZPos=-1;
	}
	actor->AddVVCell(sca);
}

//de/activates the entangle overlay
void pcf_entangle(Actor *actor, ieDword oldValue, ieDword newValue)
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
void pcf_sanctuary(Actor *actor, ieDword oldValue, ieDword newValue)
{
	ieDword changed = newValue^oldValue;
	ieDword mask = 1;
	for (int i=0;i<32;i++) {
		if (changed&mask) {
			if (newValue&mask) {
				handle_overlay(actor, i);
			} else {
				actor->RemoveVVCell(hc_overlays[i], true);
			}
		}
		mask<<=1;
	}
}

//de/activates the prot from missiles overlay
void pcf_shieldglobe(Actor *actor, ieDword oldValue, ieDword newValue)
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
void pcf_minorglobe(Actor *actor, ieDword oldValue, ieDword newValue)
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
void pcf_grease(Actor *actor, ieDword oldValue, ieDword newValue)
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
void pcf_web(Actor *actor, ieDword oldValue, ieDword newValue)
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
void pcf_bounce(Actor *actor, ieDword oldValue, ieDword newValue)
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

//no separate values (changes are permanent)
void pcf_fatigue(Actor *actor, ieDword oldValue, ieDword newValue)
{
	// get the old fatigue luck modifier
	int luckMod = core->ResolveStatBonus(actor, "fatigue", 1, oldValue);
	actor->BaseStats[IE_FATIGUE] = newValue;
	// compensate for the change and modify luck
	if ((int)newValue < 0) {
		// need to just undo the bonus/malus from before
		luckMod = - luckMod;
	} else {
		luckMod = core->ResolveStatBonus(actor, "fatigue") - luckMod;
	}
	// the default value isn't 0, but the maximum penalty and we need to ignore
	// it due to the constitution bonus possibly putting the value out of range
	if (luckMod < -80) luckMod = 0;
	actor->SetBase(IE_LUCK, actor->BaseStats[IE_LUCK] + luckMod);

	// reuse the luck mod to determine when the actor is tired
	if (luckMod < 0) {
		actor->VerbalConstant(VB_TIRED, 1);
	}
}

//no separate values (changes are permanent)
void pcf_intoxication(Actor *actor, ieDword /*oldValue*/, ieDword newValue)
{
	actor->BaseStats[IE_INTOXICATION]=newValue;
}

void pcf_color(Actor *actor, ieDword /*oldValue*/, ieDword /*newValue*/)
{
	CharAnimations *anims = actor->GetAnims();
	if (anims) {
		anims->SetColors(actor->Modified+IE_COLORS);
	}
}

void pcf_armorlevel(Actor *actor, ieDword /*oldValue*/, ieDword newValue)
{
	CharAnimations *anims = actor->GetAnims();
	if (anims) {
		anims->SetArmourLevel(newValue);
	}
}

static int maximum_values[MAX_STATS]={
32767,32767,20,100,100,100,100,25,10,25,25,25,25,25,100,100,//0f
100,100,100,100,100,100,100,100,100,100,255,255,255,255,100,100,//1f
200,200,MAX_LEVEL,255,25,100,25,25,25,25,25,999999999,999999999,999999999,25,25,//2f
200,255,200,100,100,200,200,25,5,100,1,1,100,1,1,0,//3f
511,1,1,1,MAX_LEVEL,MAX_LEVEL,1,9999,25,100,100,255,1,20,20,25,//4f
25,1,1,255,25,25,255,255,25,255,255,255,255,255,255,255,//5f
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,//6f
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,//7f
255,255,255,255,255,255,255,100,100,100,999999,5,5,999999,1,1,//8f
1,25,25,30,1,1,1,25,0,100,100,1,255,255,255,255,//9f
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
NULL,NULL,NULL,NULL, NULL, NULL, pcf_fatigue, pcf_intoxication, //1f
NULL,NULL,pcf_level,NULL, pcf_stat_str, NULL, pcf_stat_int, pcf_stat_wis,
pcf_stat_dex,pcf_stat_con,pcf_stat_cha,NULL, pcf_xp, pcf_gold, pcf_morale, NULL, //2f
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL,
NULL,NULL,NULL,NULL, NULL, NULL, pcf_entangle, pcf_sanctuary, //3f
pcf_minorglobe, pcf_shieldglobe, pcf_grease, pcf_web, pcf_level, pcf_level, NULL, NULL,
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL, //4f
NULL,NULL,NULL,pcf_minhitpoint, NULL, NULL, NULL, NULL,
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL, //5f
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL,
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL, //6f
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL,
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL, //7f
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL,
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL, //8f
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL,
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL, //9f
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL,
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL, //af
NULL,NULL,NULL,NULL, pcf_morale, pcf_bounce, NULL, NULL,
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL, //bf
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL,
NULL,NULL,NULL,NULL, NULL, pcf_animid,pcf_state, pcf_extstate, //cf
pcf_color,pcf_color,pcf_color,pcf_color, pcf_color, pcf_color, pcf_color, NULL,
NULL,NULL,NULL,pcf_armorlevel, NULL, NULL, NULL, NULL, //df
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL,
pcf_class,NULL,pcf_ea,NULL, NULL, NULL, NULL, NULL, //ef
pcf_level,pcf_level,pcf_level,pcf_level, pcf_level, pcf_level, pcf_level, pcf_level,
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL //ff
};

/** call this from ~Interface() */
void Actor::ReleaseMemory()
{
	int i;

	if (classcount>=0) {
		if (clericspelltables) {
			for (i=0;i<classcount;i++) {
				if (clericspelltables[i]) {
					free (clericspelltables[i]);
				}
			}
			free(clericspelltables);
			clericspelltables=NULL;
		}
		if (druidspelltables) {
			for (i=0;i<classcount;i++) {
				if (druidspelltables[i]) {
					free (druidspelltables[i]);
				}
			}
			free(druidspelltables);
			druidspelltables=NULL;
		}
		if (wizardspelltables) {
			for (i=0;i<classcount;i++) {
				if (wizardspelltables[i]) {
					free(wizardspelltables[i]);
				}
			}
			free(wizardspelltables);
			wizardspelltables=NULL;
		}
		if (classabilities) {
			for (i=0;i<classcount;i++) {
				if (classabilities[i]) {
					free (classabilities[i]);
				}
			}
			free(classabilities);
			classabilities=NULL;
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

		if (xpbonus) {
			free(xpbonus);
			xpbonus=NULL;
			xpbonuslevels = -1;
			xpbonustypes = -1;
		}
		if (levelslots) {
			for (i=0; i<classcount; i++) {
				if (levelslots[i]) {
					free(levelslots[i]);
				}
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
		if (skillstats) {
			free(skillstats);
			skillstats=NULL;
		}
		if (skillabils) {
			free(skillabils);
			skillabils=NULL;
		}

		if (afcomments) {
			for(i=0;i<afcount;i++) {
				if(afcomments[i]) {
					free(afcomments[i]);
				}
			}
			free(afcomments);
			afcomments=NULL;
		}

		if (wspecial) {
			for (i=0; i<=wspecial_max; i++) {
				if (wspecial[i]) {
					free(wspecial[i]);
				}
			}
			free(wspecial);
			wspecial=NULL;
		}
		if (wspattack) {
			for (i=0; i<wspattack_rows; i++) {
				if (wspattack[i]) {
					free(wspattack[i]);
				}
			}
			free(wspattack);
			wspattack=NULL;
		}
		if (wsdualwield) {
			for (i=0; i<=STYLE_MAX; i++) {
				if (wsdualwield[i]) {
					free(wsdualwield[i]);
				}
			}
			free(wsdualwield);
			wsdualwield=NULL;
		}
		if (wstwohanded) {
			for (i=0; i<=STYLE_MAX; i++) {
				if (wstwohanded[i]) {
					free(wstwohanded[i]);
				}
			}
			free(wstwohanded);
			wstwohanded=NULL;
		}
		if (wsswordshield) {
			for (i=0; i<=STYLE_MAX; i++) {
				if (wsswordshield[i]) {
					free(wsswordshield[i]);
				}
			}
			free(wsswordshield);
			wsswordshield=NULL;
		}
		if (wssingle) {
			for (i=0; i<=STYLE_MAX; i++) {
				if (wssingle[i]) {
					free(wssingle[i]);
				}
			}
			free(wssingle);
			wssingle=NULL;
		}
		if (monkbon) {
			for (unsigned i=0; i<monkbon_rows; i++) {
				if (monkbon[i]) {
					free(monkbon[i]);
				}
			}
			free(monkbon);
			monkbon=NULL;
		}
		for(i=0;i<20;i++) {
			free(wmlevels[i]);
			wmlevels[i]=NULL;
		}
		skilldex.clear();
		skillrac.clear();
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

#define COL_DRUID_SPELL   0   //ranger type racial enemy
#define COL_CLERIC_SPELL  1   //cleric spells
#define COL_MAGE_SPELL    2   //mage spells
#define COL_STARTXP       3   //starting xp
#define COL_BARD_SKILL    4   //bard skills
#define COL_THIEF_SKILL   5   //thief skills
#define COL_LAYHANDS      6   //paladin ability
#define COL_TURNLEVEL     7   //paladin/cleric
#define COL_BOOKTYPE      8   //sorceror
#define COL_HATERACE      9   //ranger special
#define COL_ABILITIES     10  //clab
#define COL_NO_PROF       11  //default proficiency penalty

#define COL_MAIN       0
#define COL_SPARKS     1
#define COL_GRADIENT   2

/* returns the ISCLASS for the class based on name */
int IsClassFromName (const char* name)
{
	//TODO: is there a better way of doing this?
	for (int i=0; i<ISCLASSES; i++) {
		if (strcmp(name, isclassnames[i]) == 0)
			return i;
	}
	return -1;
}

GEM_EXPORT void UpdateActorConfig()
{
	core->GetDictionary()->Lookup("Critical Hit Screen Shake", crit_hit_scr_shake);
	core->GetDictionary()->Lookup("Selection Sounds Frequency", sel_snd_freq);
	core->GetDictionary()->Lookup("Command Sounds Frequency", cmd_snd_freq);
	core->GetDictionary()->Lookup("Bored Timeout", bored_time);
	core->GetDictionary()->Lookup("Footsteps", footsteps);
	//FIXME: Drop all actors' SpriteCover.
	//the actor will change dithering only after selected/moved (its spritecover was updated)
	core->GetDictionary()->Lookup("Always Dither", always_dither);

	//Handle Game Difficulty and Nightmare Mode
	GameDifficulty = 0;
	core->GetDictionary()->Lookup("Nightmare Mode", GameDifficulty);
	if (GameDifficulty) {
		GameDifficulty = DIFF_NIGHTMARE;
	} else {
		GameDifficulty = 0;
		core->GetDictionary()->Lookup("Difficulty Level", GameDifficulty);
	}
	if (GameDifficulty>DIFF_NIGHTMARE) GameDifficulty = DIFF_NIGHTMARE;

	// iwd has a config option for leniency
	core->GetDictionary()->Lookup("Suppress Extra Difficulty Damage", NoExtraDifficultyDmg);
}

static void InitActorTables()
{
	int i, j;

	UpdateActorConfig();
	pstflags = !!core->HasFeature(GF_PST_STATE_FLAGS);
	nocreate = !!core->HasFeature(GF_NO_NEW_VARIABLES);
	third = !!core->HasFeature(GF_3ED_RULES);
	raresnd = !!core->HasFeature(GF_RARE_ACTION_VB);

	if (pstflags) {
		state_invisible=STATE_PST_INVIS;
	} else {
		state_invisible=STATE_INVISIBLE;
	}

	//I'm not sure if xp is reduced or not in iwd2 on easy setting
	if (core->HasFeature(GF_CHALLENGERATING)) {
		sharexp=SX_DIVIDE|SX_CR;
	} else {
		sharexp=SX_DIVIDE|SX_COMBAT;
	}
	ReverseToHit = core->HasFeature(GF_REVERSE_TOHIT);
	CheckAbilities = core->HasFeature(GF_CHECK_ABILITIES);
	DeathOnZeroStat = core->HasFeature(GF_DEATH_ON_ZERO_STAT);
	IWDSound = core->HasFeature(GF_SOUNDS_INI);

	//this table lists various level based xp bonuses
	AutoTable tm("xpbonus");
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
		classabilities = (char **) calloc(classcount, sizeof(char*));
		defaultprof = (int *) calloc(classcount, sizeof(int));

		ieDword bitmask = 1;

		for(i = 0; i<classcount; i++) {
			const char *field;
			const char *rowname = tm->GetRowName(i);

			field = tm->QueryField( rowname, skillcolumns[COL_DRUID_SPELL] );
			if (field[0]!='*') {
				isclass[ISDRUID] |= bitmask;
				druidspelltables[i]=strdup(field);
			}
			field = tm->QueryField( rowname, skillcolumns[COL_CLERIC_SPELL] );
			if (field[0]!='*') {
				isclass[ISCLERIC] |= bitmask;
				clericspelltables[i]=strdup(field);
			}

			field = tm->QueryField( rowname, skillcolumns[COL_MAGE_SPELL] );
			if (field[0]!='*') {
				isclass[ISMAGE] |= bitmask;
				wizardspelltables[i]=strdup(field);
			}

			// field 3 holds the starting xp

			field = tm->QueryField( rowname, skillcolumns[COL_BARD_SKILL] );
			if (field[0]!='*') {
				isclass[ISBARD] |= bitmask;
			}

			field = tm->QueryField( rowname, skillcolumns[COL_THIEF_SKILL] );
			if (field[0]!='*') {
				isclass[ISTHIEF] |= bitmask;
			}

			field = tm->QueryField( rowname, skillcolumns[COL_LAYHANDS] );
			if (field[0]!='*') {
				isclass[ISPALADIN] |= bitmask;
			}

			field = tm->QueryField( rowname, skillcolumns[COL_TURNLEVEL] );
			turnlevels[i]=atoi(field);

			field = tm->QueryField( rowname, skillcolumns[COL_BOOKTYPE] );
			booktypes[i]=atoi(field);
			//if booktype == 3 then it is a 'divine sorceror' class
			//we shouldn't hardcode iwd2 classes this heavily
			if (booktypes[i]==2) {
				isclass[ISSORCERER] |= bitmask;
			}

			field = tm->QueryField( rowname, skillcolumns[COL_HATERACE] );
			if (field[0]!='*') {
				isclass[ISRANGER] |= bitmask;
			}

			field = tm->QueryField( rowname, skillcolumns[COL_ABILITIES] );
			if (!strnicmp(field, "CLABMO", 6)) {
				isclass[ISMONK] |= bitmask;
			}
			classabilities[i]=strdup(field);

			field = tm->QueryField( rowname, skillcolumns[COL_NO_PROF] );
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

	tm.load("qslot2");
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

	tm.load("mdfeats");
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

	tm.load("itemanim");
	if (tm) {
		animcount = tm->GetRowCount();
		itemanim = new ItemAnimType[animcount];
		for (i = 0; i < animcount; i++) {
			strnlwrcpy(itemanim[i].itemname, tm->QueryField(i,0),8 );
			itemanim[i].animation = (ieByte) atoi( tm->QueryField(i,1) );
		}
	}

	tm.load("mxsplwis");
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

	tm.load("featreq");
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

	//TODO: check iwd2
	maxLevelForHpRoll = (int *) calloc(classcount, sizeof(int));
	tm.load("classes");
	if (tm && !core->HasFeature(GF_LEVELSLOT_PER_CLASS)) {
		AutoTable hptm;
		//iwd2 just uses levelslotsiwd2 instead
		Log(MESSAGE, "Actor", "Examining classes.2da");

		//when searching the levelslots, you must search for
		//levelslots[BaseStats[IE_CLASS]-1] as there is no class id of 0
		levelslots = (int **) calloc(classcount, sizeof(int*));
		dualswap = (int *) calloc(classcount, sizeof(int));
		multi = (int *) calloc(classcount, sizeof(int));
		ieDword tmpindex;

		memset(classesiwd2, 0 , sizeof(classesiwd2) );
		for (i=0; i<classcount; i++) {
			//make sure we have a valid classid, then decrement
			//it to get the correct array index
			tmpindex = atoi(tm->QueryField(i, 5));
			if (!tmpindex)
				continue;
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

			const char* classname = tm->GetRowName(i);
			buffer.appendFormatted("Name: %s ", classname);
			int classis = 0;
			//default all levelslots to 0
			levelslots[tmpindex] = (int *) calloc(ISCLASSES, sizeof(int));

			//single classes only worry about IE_LEVEL
			long tmpclass = 0; //atoi(tm->QueryField(i, 4));
			valid_number( tm->QueryField(i, 4), tmpclass);
			multi[tmpindex] = (ieDword) tmpclass;
			if (!tmpclass) {
				classis = IsClassFromName(classname);
				if (classis>=0) {
					//store the original class ID as iwd2 compatible ISCLASS (internal class number)
					classesiwd2[classis] = tmpindex+1;

					buffer.appendFormatted("Classis: %d ", classis);
					levelslots[tmpindex][classis] = IE_LEVEL;
					//get the last level when we can roll for HP
					hptm.load(tm->QueryField(i, 6), true);
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
			ieDword tmpbits = bitcount (tmpclass);

			//we need all the classnames of the multi to compare with the order we load them in
			//because the original game set the levels based on name order, not bit order
			char **classnames = (char **) calloc(tmpbits, sizeof(char *));
			classnames[0] = (char*)strtok(strdup((char*)classname), "_");
			while (numfound<tmpbits && (classnames[numfound] = strdup(strtok(NULL, "_")))) {
				numfound++;
			}
			numfound = 0;
			bool foundwarrior = false;
			for (int j=0; j<classcount; j++) {
				//no sense continuing if we've found all to be found
				if (numfound==tmpbits)
					break;
				if ((1<<j)&tmpclass) {
					//save the IE_LEVEL information
					const char* currentname = tm->GetRowName((ieDword)(tm->FindTableValue(5, j+1)));
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

			if (classnames) {
				for (ieDword j=0; j<tmpbits; j++) {
					if (classnames[j]) {
						free(classnames[j]);
					}
				}
				free(classnames);
				classnames = NULL;
			}

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

	//pre-cache hit/damage/speed bonuses for weapons
	tm.load("wspecial");
	if (tm) {
		//load in the identifiers
		wspecial_max = tm->GetRowCount()-1;
		int cols = tm->GetColumnCount();
		wspecial = (int **) calloc(wspecial_max+1, sizeof(int *));

		for (i=0; i<=wspecial_max; i++) {
			wspecial[i] = (int *) calloc(WSPECIAL_COLS, sizeof(int));
			for (int j=0; j<cols; j++) {
				wspecial[i][j] = atoi(tm->QueryField(i, j));
			}
		}
	}

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
	tm.load("wstwowpn");
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
	tm.load("wstwohnd");
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

	//two-handed table
	tm.load("wsshield");
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

	//two-handed table
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
	tm.load("monkbon");
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
	tm.load("lvlmodwm");
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
	tm.load("skillsta");
	if (tm) {
		int rowcount = tm->GetRowCount();
		skillcount = rowcount;
		if (rowcount) {
			skillstats = (int *) malloc(rowcount * sizeof(int) );
			skillabils = (int *) malloc(rowcount * sizeof(int) );
			while(rowcount--) {
				skillstats[rowcount]=core->TranslateStat(tm->QueryField(rowcount,0));
				skillabils[rowcount]=core->TranslateStat(tm->QueryField(rowcount,1));
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
	Holder<SymbolMgr> race = NULL;
	if (racetable != -1) {
		race = core->GetSymbol(racetable);
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
						value = race->GetValue(tm->GetRowName(i));
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
		for (i=0; i<6; i++) {
			dmgadjustments[i] = atoi(tm->QueryField(0, i) );
			xpadjustments[i] = atoi(tm->QueryField(1, i) );
		}
	}
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
	if (HasSpellState(SS_BLINK)) base += 20;
	//FIXME: IWD2 has this as 20, other games as 50
	if (HasSpellState(SS_DEAF)) base += 20;
	if (!arcana) return base;

	ieDword armor = -GetArmorFailure();

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
	int dexbonus;

	if (third) {
		dexbonus = GetAbilityBonus(IE_DEX);
	} else {
		dexbonus = core->GetDexterityBonus(STAT_DEX_AC, GetStat(IE_DEX) );
	}

	if (dexbonus) {
		// FIXME: the maximum dexterity bonus isn't stored(?),
		// but can usually be calculated from 8-AC+enchantment (or 8-AC if you don't count that bonus in AC already)
		// or from the armor penalty (works for light armors)
		ieWord armtype = inventory.GetArmorItemType();
		int armor = (int) core->GetArmorFailure(armtype);

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

	//and the monk wisdom bonus
	if (third) {
		//if the monk has a shield equipped, no bonus
		int itemtype = inventory.GetShieldItemType();
		//items with critical range are weapons, not shields, so they are ok
		//empty hand is also ok
		if (GetStat(IE_LEVELMONK) && (itemtype==0xffff || core->GetArmorFailure(itemtype))) {
			//too many variables, recycling
			itemtype = GetAbilityBonus(IE_WIS);
			//add the bonus only if it is a bonus (the dexbonus is negative)
			if (itemtype>0) {
				dexbonus += itemtype;
			}
		}
	}
	return dexbonus;
}

//Returns the personal critical damage type in a binary compatible form (PST)
//TODO: may want to preload the crits table to avoid spam in the log
int Actor::GetCriticalType() const
{
	long ret = 0;
	AutoTable tm("crits");
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
		AddAnimation(tm->QueryField(row, 0), -1, 0, AA_PLAYONCE);
	}
}

void Actor::PlayDamageAnimation(int type, bool hit)
{
	int i;

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
				AddAnimation(d_main[type], i, 0, AA_PLAYONCE);
			}
			break;
		case 4: case 5: case 6: //fire
			if(hit) {
				AddAnimation(d_main[type], d_gradient[type], 0, AA_PLAYONCE);
			}
			for(i=DL_FIRE;i<=type;i++) {
				AddAnimation(d_splash[i], d_gradient[i], 0, AA_PLAYONCE);
			}
			break;
		case 7: case 8: case 9: //electricity
			if (hit) {
				AddAnimation(d_main[type], d_gradient[type], 0, AA_PLAYONCE);
			}
			for(i=DL_ELECTRICITY;i<=type;i++) {
				AddAnimation(d_splash[i], d_gradient[i], 0, AA_PLAYONCE);
			}
			break;
		case 10: case 11: case 12://cold
			if (hit) {
				AddAnimation(d_main[type], d_gradient[type], 0, AA_PLAYONCE);
			}
			break;
		case 13: case 14: case 15://acid
			if (hit) {
				AddAnimation(d_main[type], d_gradient[type], 0, AA_PLAYONCE);
			}
			break;
		case 16: case 17: case 18://disintegrate
			if (hit) {
				AddAnimation(d_main[type], d_gradient[type], 0, AA_PLAYONCE);
			}
			break;
	}
}

bool Actor::SetStat(unsigned int StatIndex, ieDword Value, int pcf)
{
	if (StatIndex >= MAX_STATS) {
		return false;
	}
	if ( (signed) Value<-100) {
		Value = (ieDword) -100;
	}
	else {
		if ( maximum_values[StatIndex]>0) {
			if ( (signed) Value>maximum_values[StatIndex]) {
				Value = (ieDword) maximum_values[StatIndex];
			}
		}
	}

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

int Actor::GetMod(unsigned int StatIndex)
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
	if ( maximum_values[StatIndex]) {
		if ( (signed) Value>maximum_values[StatIndex]) {
			Value = (ieDword) maximum_values[StatIndex];
		}
	}

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
	if ( maximum_values[StatIndex]) {
		if ( (signed) Value>maximum_values[StatIndex]) {
			Value = (ieDword) maximum_values[StatIndex];
		}
	}

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
ieDword Actor::GetCGGender()
{
	ieDword gender = Modified[IE_SEX];
	if (gender == SEX_ILLUSION) {
		Actor *master = core->GetGame()->GetActorByGlobalID(Modified[IE_PUPPETMASTERID]);
		if (master) {
			gender = master->Modified[IE_SEX];
		}
	}

	return gender;
}

#define PI_PROJIMAGE  77

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
	/* these apply resrefs should be on a list as a trigger+resref */
	memset(applyWhenHittingMelee,0,sizeof(ieResRef));
	memset(applyWhenHittingRanged,0,sizeof(ieResRef));
	memset(applyWhenNearLiving,0,sizeof(ieResRef));
	memset(applyWhen50Damage,0,sizeof(ieResRef));
	memset(applyWhen90Damage,0,sizeof(ieResRef));
	memset(applyWhenEnemySighted,0,sizeof(ieResRef));
	memset(applyWhenPoisoned,0,sizeof(ieResRef));
	memset(applyWhenHelpless,0,sizeof(ieResRef));
	memset(applyWhenAttacked,0,sizeof(ieResRef));
	memset(applyWhenBeingHit,0,sizeof(ieResRef));
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

	if (fx) {
		fx->SetOwner(this);
		fx->AddAllEffects(this, Pos);
		delete fx;
		//copy back the original stats, because the effects
		//will be reapplied in ApplyAllEffects again
		memcpy( Modified, BaseStats, MAX_STATS * sizeof( ieDword ) );
		//also clear the spell bonuses just given, they will be
		//recalculated below again
		spellbook.ClearBonus();
	}

	unsigned int i;

	// some VVCs are controlled by stats (and so by PCFs), the rest have 'effect_owned' set
	for (i = 0; i < vvcOverlays.size(); i++) {
		if (vvcOverlays[i] && vvcOverlays[i]->effect_owned) vvcOverlays[i]->active = false;
	}
	for (i = 0; i < vvcShields.size(); i++) {
		if (vvcShields[i] && vvcShields[i]->effect_owned) vvcShields[i]->active = false;
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

	// IE_CLASS is >classcount for non-PCs/NPCs
	if (BaseStats[IE_CLASS] > 0 && BaseStats[IE_CLASS] < (ieDword)classcount)
		RefreshPCStats();

	for (i=0;i<MAX_STATS;i++) {
		if (first || Modified[i]!=previous[i]) {
			PostChangeFunctionType f = post_change_functions[i];
			if (f) {
				(*f)(this, previous[i], Modified[i]);
			}
		}
	}
	//add wisdom bonus spells
	if (!spellbook.IsIWDSpellBook() && mxsplwis) {
		int level = Modified[IE_WIS];
		if (level--) {
			spellbook.BonusSpells(IE_SPELL_TYPE_PRIEST, spllevels, mxsplwis+spllevels*level);
		}
	}

	// check if any new portrait icon was removed or added
	if (PCStats) {
		if (memcmp(PCStats->PreviousPortraitIcons, PCStats->PortraitIcons, sizeof(PCStats->PreviousPortraitIcons))) {
			core->SetEventFlag(EF_PORTRAIT);
			memcpy( PCStats->PreviousPortraitIcons, PCStats->PortraitIcons, sizeof(PCStats->PreviousPortraitIcons) );
		}
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
	int bonus;
	// this is wrong for dual-classed (so we override it later)
	// and sometimes marginally wrong for multi-classed (but we usually round the average up)
	int bonlevel = GetXPLevel(true);
	int oldlevel, oldbonus;
	oldlevel = oldbonus = 0;
	ieDword bonindex = BaseStats[IE_CLASS]-1;

	//we must limit the levels to the max allowable
	if (bonlevel>maxLevelForHpRoll[bonindex])
		bonlevel = maxLevelForHpRoll[bonindex];

	if (IsDualClassed()) {
		// just the old consititution bonus
		oldlevel = IsDualSwap() ? BaseStats[IE_LEVEL] : BaseStats[IE_LEVEL2];
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

	//toughness feat bonus (could be unhardcoded as a max hp bonus based on level if you want)
	bonus += Modified[IE_FEAT_TOUGHNESS]*3;

	//we still apply the maximum bonus to dead characters, but don't apply
	//to current HP, or we'd have dead characters showing as having hp
	Modified[IE_MAXHITPOINTS]+=bonus;
	//	if(BaseStats[IE_STATE_ID]&STATE_DEAD)
	//		bonus = 0;
	//	BaseStats[IE_HITPOINTS]+=bonus;
}

// refresh stats on creatures (PC or NPC) with a valid class (not animals etc)
// internal use only, and this is maybe a stupid name :)
void Actor::RefreshPCStats() {
	RefreshHP();

	Game *game = core->GetGame();
	//morale recovery every xth AI cycle
	int mrec = GetStat(IE_MORALERECOVERYTIME);
	if (mrec) {
		if (!(game->GameTime%mrec)) {
			if (BaseStats[IE_MORALE] < 10) {
				NewBase(IE_MORALE, 1, MOD_ADDITIVE);
			} else if (BaseStats[IE_MORALE] > 10) {
				NewBase(IE_MORALE, (ieDword) -1, MOD_ADDITIVE);
			}
		}
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
				SetBase(IE_NUMBEROFATTACKS, defaultattacks); // TODO: check if this shouldn't get +wspattack[stars][0]
			}
		} else {
			// unproficient user - force defaultattacks
			SetBase(IE_NUMBEROFATTACKS, defaultattacks);
		}
	}

	// apply the intelligence and wisdom bonus to lore
	Modified[IE_LORE] += core->GetLoreBonus(0, Modified[IE_INT]) + core->GetLoreBonus(0, Modified[IE_WIS]);

	// add fatigue every 4 hours
	// but make sure we're in the game already or players will start tired due to getting called with equipping effects before
	if (game->GameTime && !(game->GameTime % 18000)) {
		NewBase(IE_FATIGUE, 1, MOD_ADDITIVE);
	}
	if (core->ResolveStatBonus(this, "fatigue")) {
		AddPortraitIcon(39); //PI_FATIGUE from FXOpcodes.cpp
	} else {
		DisablePortraitIcon(39); //PI_FATIGUE from FXOpcodes.cpp
	}

	// regenerate actors with high enough constitution
	int rate = core->GetConstitutionBonus(STAT_CON_HP_REGEN, Modified[IE_CON]);
	if (rate && !(game->GameTime % (rate*AI_UPDATE_TIME))) {
		NewBase(IE_HITPOINTS, 1, MOD_ADDITIVE);
	}

	// adjust thieving skills with dex and race
	// table header is in this order:
	// PICK_POCKETS  OPEN_LOCKS  FIND_TRAPS  MOVE_SILENTLY  HIDE_IN_SHADOWS  DETECT_ILLUSION  SET_TRAPS
	Modified[IE_PICKPOCKET] += GetSkillBonus(1);
	Modified[IE_LOCKPICKING] += GetSkillBonus(2);
	Modified[IE_TRAPS] += GetSkillBonus(3);
	Modified[IE_STEALTH] += GetSkillBonus(4);
	Modified[IE_HIDEINSHADOWS] += GetSkillBonus(5);
	Modified[IE_DETECTILLUSIONS] += GetSkillBonus(6);
	Modified[IE_SETTRAPS] += GetSkillBonus(7);
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

#define SAVECOUNT 5
static int savingthrows[SAVECOUNT]={IE_SAVEVSSPELL, IE_SAVEVSBREATH, IE_SAVEVSDEATH, IE_SAVEVSWANDS, IE_SAVEVSPOLY};

/** returns true if actor made the save against saving throw type */
bool Actor::GetSavingThrow(ieDword type, int modifier)
{
	assert(type<SAVECOUNT);
	InternalFlags|=IF_USEDSAVE;
	int ret = SavingThrow[type];
	if (ret == 1) return false;
	if (ret == SAVEROLL) return true;
	ret += modifier + GetStat(IE_LUCK);
	return ret > (int) GetStat(savingthrows[type]);
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
			SetStat(StatIndex, Modified[StatIndex]+ModifierValue, 0);
			break;
		case MOD_ABSOLUTE:
			//straight stat change
			SetStat(StatIndex, ModifierValue, 0);
			break;
		case MOD_PERCENT:
			//percentile
			SetStat(StatIndex, BaseStats[StatIndex] * ModifierValue / 100, 0);
			break;
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

void Actor::Interact(int type)
{
	int start;
	int count;

	switch(type&0xff) {
		case I_INSULT: start=VB_INSULT; break;
		case I_COMPLIMENT: start=VB_COMPLIMENT; break;
		case I_SPECIAL: start=VB_SPECIAL; break;
		case I_INSULT_RESP: start=VB_RESP_INS; break;
		case I_COMPL_RESP: start=VB_RESP_COMP; break;
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
	VerbalConstant(start, count);
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

void Actor::VerbalConstant(int start, int count) const
{
	if (start!=VB_DIE) {
		//can't talk when dead
		if (Modified[IE_STATE_ID] & (STATE_CANTLISTEN)) return;
	}

	//If we are main character (has SoundSet) we have to check a corresponding wav file exists
	if (PCStats && PCStats->SoundSet[0]) {
		ieResRef soundref;
		ResolveStringConstant(soundref, start+count-1);
		while (count > 0 && !gamedata->Exists(soundref, IE_WAV_CLASS_ID, true)) {
			count--;
			ResolveStringConstant(soundref, start+count-1);
		}
		if (count > 0){
			DisplayStringCore((Scriptable *const) this, start + rand()%count, DS_CONSOLE|DS_CONST|DS_SPEECH);
		}
	} else { //If we are anyone else we have to check there is a corresponding strref
		while(count > 0 && GetVerbalConstant(start+count-1) == (ieStrRef) -1 ) {
			count--;
		}
		if(count > 0) {
			DisplayStringCore((Scriptable *const) this, GetVerbalConstant(start+rand()%count), DS_CONSOLE|DS_SPEECH);
		}
	}
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
		VerbalConstant(VB_REACT, 1);
		break;
	case '1':
		VerbalConstant(VB_REACT_S, 1);
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
			strncpy(resref, value, 8);
			for(count=0;count<8 && resref[count]!=',';count++) {};
			resref[count]=0;

			unsigned int len = 0;
			core->GetAudioDrv()->Play( resref, &len );
			ieDword counter = ( AI_UPDATE_TIME * len ) / 1000;
			if (counter != 0)
				SetWait( counter );
			break;
		}
	}

	// take a hit on morale
	NewBase(IE_MORALE, (ieDword) -1, MOD_ADDITIVE);
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
			VerbalConstant(vc, 1);
			return;
		}
	}
}

static int CheckInteract(const char *talker, const char *target)
{
	AutoTable interact("interact");
	if(!interact)
		return 0;
	const char *value = interact->QueryField(talker, target);
	if(!value)
		return 0;

	int tmp = 0;
	int x = 0;
	int ln = strlen(value);

	if (ln>1) {
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

int Actor::HandleInteract(Actor *target)
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

	//don't even bother
	if (game->NpcInParty<2) return false;
	ieDword size = game->GetPartySize(true);
	//don't even bother, again
	if (size<2) return false;

	if(core->Roll(1,2,-1)) {
		return false;
	}

	for(unsigned int i=core->Roll(1,size,0);i<2*size;i++) {
		Actor *target = game->GetPC(i%size, true);
		if (target==this) continue;
		if (target->BaseStats[IE_MC_FLAGS]&MC_EXPORTABLE) continue; //not NPC
		if (target->GetCurrentArea()!=GetCurrentArea()) continue;

		//simplified interact
		switch(HandleInteract(target)) {
			case -1: return false;
			case 1: return true;
			default:
			//V2 interact
			char Tmp[40];
			LastTalker = target->GetGlobalID();
			strncpy(Tmp,"Interact([-1])", sizeof(Tmp) );
			Action *action = GenerateActionDirect(Tmp, target);
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
	switch (sel_snd_freq) {
		case 0:
			return;
		case 1:
			if (core->Roll(1,100,0) > 20) return;
		default:;
	}

	//drop the rare selection comment 5% of the time
	if (InParty && core->Roll(1,100,0) <= RARE_SELECT_CHANCE){
		//rare select on main character for BG1 won't work atm
		VerbalConstant(VB_SELECT_RARE, NUM_RARE_SELECT_SOUNDS);
	} else {
		//checks if we are main character to limit select sounds
		if (PCStats && PCStats->SoundSet[0]) {
			VerbalConstant(VB_SELECT, NUM_MC_SELECT_SOUNDS);
		} else {
			VerbalConstant(VB_SELECT, NUM_SELECT_SOUNDS);
		}
	}
}

#define SEL_ACTION_COUNT_COMMON  3
#define SEL_ACTION_COUNT_ALL     7

//call this when a PC receives a command from GUI
void Actor::CommandActor()
{
	switch (cmd_snd_freq) {
		case 0:
			return;
		case 1:
			if (playedCommandSound) return;
			playedCommandSound = true;
		case 2:
			//PST has 4 states and rare sounds
			if (raresnd) {
				if (core->Roll(1,100,0)>50) return;
			}
		default:;
	}
	//if GF_RARE_ACTION_VB is set, don't select the last 4 options frequently
	VerbalConstant(VB_COMMAND,(raresnd && core->Roll(1, 100,0)<75)?SEL_ACTION_COUNT_COMMON:SEL_ACTION_COUNT_ALL);
}

//Generates an idle action (party banter, area comment, bored)
void Actor::IdleActions(bool nonidle)
{
	//only party [N]PCs talk
	if (!InParty) return;
	//if they got an area
	Map *map = GetCurrentArea();
	if (!map) return;
	//and not in panic
	if (panicMode!=PANIC_NONE) return;

	Game *game = core->GetGame();
	//there is no combat
	if (game->CombatCounter) return;
	//and they are on the current area
	if (map!=game->GetCurrentArea()) return;

	ieDword time = game->GameTime;

	//don't mess with cutscenes, dialogue, or when scripts disabled us
	if (core->InCutSceneMode() || game->BanterBlockFlag || (game->BanterBlockTime>time) ) {
		return;
	}

	//drop an area comment, party oneliner or initiate party banter (with Interact)
	//party comments have a priority, but they happen half of the time, at most
	if (nextComment<time) {
		if (nextComment && !Immobile()) {
			if (!GetPartyComment()) {
				GetAreaComment(map->AreaType);
			}
		}
		nextComment = time+core->Roll(5,1000,bored_time/2);
		return;
	}

	//drop the bored one liner is there was no action for some time
	if (nonidle || !nextBored || InMove() || Immobile()) {
		//if not in party or bored timeout is disabled, don't bother to set the new time
		if (InParty && bored_time) {
			nextBored=time+core->Roll(1,30,bored_time);
		}
	} else {
		if (nextBored<time) {
			int x = bored_time / 10;
			if (x<10) x = 10;
			nextBored = time+core->Roll(1,30,x);
			VerbalConstant(VB_BORED, 1);
		}
	}
}

bool Actor::OverrideActions()
{
	//TODO:: implement forced actions that mess with scripting (panic, confusion, etc)
	return false;
}

void Actor::Panic(Scriptable *attacker, int panicmode)
{
	if (GetStat(IE_STATE_ID)&STATE_PANIC) {
		print("Already paniced");
		//already in panic
		return;
	}
	if (InParty) core->GetGame()->SelectActor(this, false, SELECT_NORMAL);
	SetBaseBit(IE_STATE_ID, STATE_PANIC, true);
	VerbalConstant(VB_PANIC, 1 );

	Action *action;
	char Tmp[40];
	if (panicmode == PANIC_RUNAWAY && (!attacker || attacker->Type!=ST_ACTOR)) {
		panicmode = PANIC_RANDOMWALK;
	}

	switch(panicmode) {
	case PANIC_RUNAWAY:
		strncpy(Tmp,"RunAwayFromNoInterrupt([-1])", sizeof(Tmp) );
		action = GenerateActionDirect(Tmp, attacker);
		break;
	case PANIC_RANDOMWALK:
		strncpy(Tmp,"RandomWalk()", sizeof(Tmp) );
		action = GenerateAction( Tmp );
		break;
	case PANIC_BERSERK:
		if (Modified[IE_EA]<EA_GOODCUTOFF) {
			strncpy(Tmp,"GroupAttack('[EVILCUTOFF]'", sizeof(Tmp) );
		} else {
			strncpy(Tmp,"GroupAttack('[GOODCUTOFF]'", sizeof(Tmp) );
		}
		action = GenerateAction( Tmp );
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
	switch (op) {
	case BM_SET: tmp = arg; break;
	case BM_OR: tmp |= arg; break;
	case BM_NAND: tmp &= ~arg; break;
	case BM_XOR: tmp ^= arg; break;
	case BM_AND: tmp &= arg; break;
	}
	SetBase(IE_MC_FLAGS, tmp);
}

void Actor::DialogInterrupt()
{
	//if dialoginterrupt was set, no verbal constant
	if ( Modified[IE_MC_FLAGS]&MC_NO_TALK)
		return;

	/* this part is unsure */
	if (Modified[IE_EA]>=EA_EVILCUTOFF) {
		VerbalConstant(VB_HOSTILE, 1 );
	} else {
		if (TalkCount) {
			VerbalConstant(VB_DIALOG, 1);
		} else {
			VerbalConstant(VB_INITIALMEET, 1);
		}
	}
}

void Actor::GetHit()
{
	if (!Immobile()) {
		SetStance( IE_ANI_DAMAGE );
	}
	VerbalConstant(VB_DAMAGE, 1 );

	if (Modified[IE_STATE_ID]&STATE_SLEEP) {
		if (Modified[IE_EXTSTATE_ID]&EXTSTATE_NO_WAKEUP) {
			return;
		}
		Effect *fx = EffectQueue::CreateEffect(fx_cure_sleep_ref, 0, 0, FX_DURATION_INSTANT_PERMANENT);
		fxqueue.AddEffect(fx);
	}
	InterruptCasting = true;
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

	//cheese to avoid one shotting newbie player
/* FIXME: decode exact conditions
	if ( InParty && (Modified[IE_MAXHITPOINTS]<20) && (damage>Modified[IE_MAXHITPOINTS]) ) {
		return true;
	}
*/
	return false;
}

bool Actor::CheckSilenced()
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

	//add lastdamagetype up ? maybe
	//FIXME: what does original do?
	LastDamageType|=damagetype;
	Actor *act=NULL;
	if (!hitter) {
		// TODO: check this
		hitter = area->GetActorByGlobalID(LastHitter);
	}

	if (hitter) {
		if (hitter->Type==ST_ACTOR) {
			act = (Actor *) hitter;
		}
	}

	switch(modtype)
	{
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
		Log(ERROR, "Actor", "Invalid damagetype!");
		return 0;
	}

	int resisted = 0;

	if (!(saveflags&SF_BYPASS_MIRROR_IMAGE)) {
		int mirrorimages = Modified[IE_MIRRORIMAGES];
		if (mirrorimages) {
			if (LuckyRoll(1,mirrorimages+1,0) != 1) {
				fxqueue.DecreaseParam1OfEffect(fx_mirrorimage_ref, 1);
				Modified[IE_MIRRORIMAGES]--;
				damage = 0;
			}
		}
	}

	if (!(saveflags&SF_IGNORE_DIFFICULTY)) {
		// adjust enemy damage according to difficulty settings:
		// -50%, -25%, 0, 50%, 100%, 150%
		if (Modified[IE_EA] < EA_GOODCUTOFF) {
			int adjustmentPercent = dmgadjustments[GameDifficulty];
			if (!NoExtraDifficultyDmg || adjustmentPercent < 0) {
				damage += (damage * adjustmentPercent)/100;
			}
		}
	}

	if (damage) {
		ModifyDamage (hitter, damage, resisted, damagetype);
	}
	DisplayCombatFeedback(damage, resisted, damagetype, hitter);

	if (damage>0) {
		// instant chunky death if the actor is petrified or frozen
		if (Modified[IE_STATE_ID] & (STATE_FROZEN|STATE_PETRIFIED) && !Modified[IE_DISABLECHUNKING] && (GameDifficulty > DIFF_NORMAL) ) {
			damage = 123456; // arbitrarily high for death; won't be displayed
			LastDamageType |= DAMAGE_CHUNKING;
		}
	}

	if (BaseStats[IE_HITPOINTS] <= (ieDword) damage) {
		// common fists do normal damage, but cause sleeping for a round instead of death
		if (Modified[IE_MINHITPOINTS]<=0 && AttackIsStunning(damagetype) ) {
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
			damage = Modified[IE_HITPOINTS]-1;
		}
	}

	NewBase(IE_HITPOINTS, (ieDword) -damage, MOD_ADDITIVE);

	// also apply reputation damage if we hurt (but not killed) an innocent
	if (Modified[IE_CLASS] == CLASS_INNOCENT && !core->InCutSceneMode()) {
		if (act && act->GetStat(IE_EA) <= EA_CONTROLLABLE) {
			core->GetGame()->SetReputation(core->GetGame()->Reputation + core->GetReputationMod(1));
		}
	}

	int chp = (signed) BaseStats[IE_HITPOINTS];
	if (damage > 0) {
		//if this kills us, check if attacker could cleave
		if (act && (damage>chp)) {
			act->CheckCleave();
		}
		GetHit();
		//fixme: implement applytrigger, copy int0 into LastDamage there
		LastDamage = damage;
		AddTrigger(TriggerEntry(trigger_tookdamage, damage)); // FIXME: lastdamager? LastHitter is not set for spell damage
		AddTrigger(TriggerEntry(trigger_hitby, LastHitter)); // FIXME: lastdamager? LastHitter is not set for spell damage
	}

	InternalFlags|=IF_ACTIVE;
	int damagelevel = 0; //FIXME: this level is never used
	if (damage<10) {
		damagelevel = 1;
	} else {
		NewBase(IE_MORALE, (ieDword) -1, MOD_ADDITIVE);
		damagelevel = 2;
	}

	if (damagetype & (DAMAGE_FIRE|DAMAGE_MAGICFIRE) ) {
		PlayDamageAnimation(DL_FIRE+damagelevel);
	} else if (damagetype & (DAMAGE_COLD|DAMAGE_MAGICCOLD) ) {
		PlayDamageAnimation(DL_COLD+damagelevel);
	} else if (damagetype & (DAMAGE_ELECTRICITY) ) {
		PlayDamageAnimation(DL_ELECTRICITY+damagelevel);
	} else if (damagetype & (DAMAGE_ACID) ) {
		PlayDamageAnimation(DL_ACID+damagelevel);
	} else if (damagetype & (DAMAGE_MAGIC) ) {
		PlayDamageAnimation(DL_DISINTEGRATE+damagelevel);
	} else {
		if (chp<-10) {
			PlayDamageAnimation(critical<<8);
		} else {
			PlayDamageAnimation(DL_BLOOD+damagelevel);
		}
	}

	if (InParty) {
		if (chp<(signed) Modified[IE_MAXHITPOINTS]/10) {
			core->Autopause(AP_WOUNDED, this);
		}
		if (damage>0) {
			core->Autopause(AP_HIT, this);
			core->SetEventFlag(EF_PORTRAIT);
		}
	}
	return damage;
}

//TODO: handle pst
void Actor::DisplayCombatFeedback (unsigned int damage, int resisted, int damagetype, Scriptable *hitter)
{
	bool detailed = false;
	const char *type_name = "unknown";
	if (displaymsg->HasStringReference(STR_DMG_SLASHING)) { // how and iwd2
		std::multimap<ieDword, DamageInfoStruct>::iterator it;
		it = core->DamageInfoMap.find(damagetype);
		if (it != core->DamageInfoMap.end()) {
			type_name = core->GetString(it->second.strref, 0);
		}
		detailed = true;
	}

	if (damage > 0 && resisted != DR_IMMUNE) {
		Log(COMBAT, "Actor", "%d %s damage taken.\n", damage, type_name);

		if (detailed) {
			// 3 choices depending on resistance and boni
			// iwd2 also has two Tortoise Shell (spell) absorption strings
			core->GetTokenDictionary()->SetAtCopy( "TYPE", type_name);
			core->GetTokenDictionary()->SetAtCopy( "AMOUNT", damage);
			if (hitter && hitter->Type == ST_ACTOR) {
				core->GetTokenDictionary()->SetAtCopy( "DAMAGER", hitter->GetName(1) );
			} else {
				core->GetTokenDictionary()->SetAtCopy( "DAMAGER", "trap" );
			}
			if (resisted < 0) {
				//Takes <AMOUNT> <TYPE> damage from <DAMAGER> (<RESISTED> damage bonus)
				core->GetTokenDictionary()->SetAtCopy( "RESISTED", abs(resisted));
				displaymsg->DisplayConstantStringName(STR_DAMAGE3, DMC_WHITE, this);
			} else if (resisted > 0) {
				//Takes <AMOUNT> <TYPE> damage from <DAMAGER> (<RESISTED> damage resisted)
				core->GetTokenDictionary()->SetAtCopy( "RESISTED", abs(resisted));
				displaymsg->DisplayConstantStringName(STR_DAMAGE2, DMC_WHITE, this);
			} else {
				//Takes <AMOUNT> <TYPE> damage from <DAMAGER>
				displaymsg->DisplayConstantStringName(STR_DAMAGE1, DMC_WHITE, this);
			}
		} else if (core->HasFeature(GF_ONSCREEN_TEXT) ) {
			if(0) print("TODO: pst floating text");
		} else if (!displaymsg->HasStringReference(STR_DAMAGE2) || !hitter || hitter->Type != ST_ACTOR) {
			// bg1 and iwd
			// or any traps or self-infliction (also for bg1)
			// construct an i18n friendly "Damage Taken (damage)", since there's no token
			char tmp[64];
			const char* msg = core->GetString(displaymsg->GetStringReference(STR_DAMAGE1), 0);
			snprintf(tmp, sizeof(tmp), "%s (%d)", msg, damage);
			displaymsg->DisplayStringName(tmp, DMC_WHITE, this);
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
	if (cnt) {
		int len = strlen(Sound);
		if (len<8) {
			Sound[len]=cnt+0x60;
		}
	}
	unsigned int len = 0;
	core->GetAudioDrv()->Play( Sound,Pos.x,Pos.y, 0, &len );
	nextWalk = thisTime + len;
}

//Play hit sounds (HIT_0<dtype><armor>)
void Actor::PlayHitSound(DataFileMgr *resdata, int damagetype, bool suffix)
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

		if (armor<0 || armor>35) return;
		if (type<0) {
			type = -type;
		} else {
			armor = resdata->GetKeyAsInt(section, "armor",0);
		}
	} else {
		//hack for stun (always first armortype)
		if (type<0) {
			type = -type;
		} else {
			armor = Modified[IE_ARMOR_TYPE];
		}
	}

	if (levels) {
		snprintf(Sound,9,"HIT_0%d%c%c",type, armor+'A', suffix?'1':0);
	} else {
		snprintf(Sound,9,"HIT_0%d%c",type, suffix?'1':0);
	}
	core->GetAudioDrv()->Play( Sound,Pos.x,Pos.y );
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
	unsigned int i;

	buffer.appendFormatted( "Debugdump of Actor %s (%s, %s):\n", LongName, ShortName, GetName(-1) );
	buffer.append("Scripts:");
	for (i = 0; i < MAX_SCRIPTS; i++) {
		const char* poi = "<none>";
		if (Scripts[i]) {
			poi = Scripts[i]->GetName();
		}
		buffer.appendFormatted( " %.8s", poi );
	}
	buffer.append("\n");
	buffer.appendFormatted("Area:       %.8s   ", Area );
	buffer.appendFormatted("Dialog:     %.8s\n", Dialog );
	buffer.appendFormatted("Global ID:  %d   PartySlot: %d\n", GetGlobalID(), InParty);
	buffer.appendFormatted("Script name:%.32s    Current action: %d\n", scriptName, CurrentAction ? CurrentAction->actionID : -1);
	buffer.appendFormatted("Int. Flags: 0x%x ", InternalFlags);
	buffer.appendFormatted("TalkCount:  %d   ", TalkCount );
	buffer.appendFormatted("Allegiance: %d   current allegiance:%d\n", BaseStats[IE_EA], Modified[IE_EA] );
	buffer.appendFormatted("Class:      %d   current class:%d\n", BaseStats[IE_CLASS], Modified[IE_CLASS] );
	buffer.appendFormatted("Race:       %d   current race:%d\n", BaseStats[IE_RACE], Modified[IE_RACE] );
	buffer.appendFormatted("Gender:     %d   current gender:%d\n", BaseStats[IE_SEX], Modified[IE_SEX] );
	buffer.appendFormatted("Specifics:  %d   current specifics:%d\n", BaseStats[IE_SPECIFIC], Modified[IE_SPECIFIC] );
	buffer.appendFormatted("Alignment:  %x   current alignment:%x\n", BaseStats[IE_ALIGNMENT], Modified[IE_ALIGNMENT] );
	buffer.appendFormatted("Morale:     %d   current morale:%d\n", BaseStats[IE_MORALE], Modified[IE_MORALE] );
	buffer.appendFormatted("Moralebreak:%d   Morale recovery:%d\n", Modified[IE_MORALEBREAK], Modified[IE_MORALERECOVERYTIME] );
	buffer.appendFormatted("Visualrange:%d (Explorer: %d)\n", Modified[IE_VISUALRANGE], Modified[IE_EXPLORE] );

	//this works for both level slot style
	buffer.appendFormatted("Levels:\n");
	for (i = 0;i<ISCLASSES;i++) {
		int level = GetClassLevel(i);
		if (level) {
			buffer.appendFormatted("%s: %d    ", isclassnames[i], level);
		}
	}
	buffer.appendFormatted("\nSummary: %d\n", GetXPLevel(true) );

	buffer.appendFormatted("current HP:%d\n", BaseStats[IE_HITPOINTS] );
	buffer.appendFormatted("Mod[IE_ANIMATION_ID]: 0x%04X ResRef:%.8s\n", Modified[IE_ANIMATION_ID], anims->ResRef );
	buffer.appendFormatted("TURNUNDEADLEVEL: %d current: %d\n", BaseStats[IE_TURNUNDEADLEVEL], Modified[IE_TURNUNDEADLEVEL]);
	buffer.appendFormatted("Colors:    ");
	if (core->HasFeature(GF_ONE_BYTE_ANIMID) ) {
		for(i=0;i<Modified[IE_COLORCOUNT];i++) {
			buffer.appendFormatted("   %d", Modified[IE_COLORS+i]);
		}
	} else {
		for(i=0;i<7;i++) {
			buffer.appendFormatted("   %d", Modified[IE_COLORS+i]);
		}
	}
	buffer.append("\n");
	buffer.appendFormatted("WaitCounter: %d\n", (int) GetWait());
	buffer.appendFormatted("LastTarget: %d %s\n", LastTarget, GetActorNameByID(LastTarget));
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
	//now we have an area
	Scriptable::SetMap(map);
	//unless we just lost it, in that case clear up some fields and leave
	if (!map) {
		//more bits may or may not be needed
		InternalFlags &=~IF_CLEANUP;
		return;
	}

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
			case SLOT_EFFECT_MELEE:
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
		if (Equipped!=IW_NO_EQUIPPED) {
			inventory.EquipItem( Equipped+inventory.GetWeaponSlot());
		} else {
			inventory.EquipItem(inventory.GetFistSlot());
		}
		SetEquippedQuickSlot( inventory.GetEquipped(), EquippedHeader );
	}
}

void Actor::SetPosition(const Point &position, int jump, int radiusx, int radiusy)
{
	PathTries = 0;
	ClearPath();
	Point p, q;
	p.x = position.x/16;
	p.y = position.y/12;
	q = p;
	if (jump && !(Modified[IE_DONOTJUMP] & DNJ_FIT) && size ) {
		Map *map = GetCurrentArea();
		//clear searchmap so we won't block ourselves
		map->ClearSearchMapFor(this);
		map->AdjustPosition( p, radiusx, radiusy );
	}
	if (p==q) {
		MoveTo( position );
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
	if (core->HasFeature(GF_LEVELSLOT_PER_CLASS)) {
		// iwd2
		for (int i=0; i < ISCLASSES; i++) {
			if (stats[levelslotsiwd2[i]] > 0) clscount++;
		}
		average = stats[IE_CLASSLEVELSUM] / (float) clscount + 0.5;
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
				clscount = bitcount (multiclass);
				for (int i=1; i<clscount; i++)
					average += levels[i];
		} //else single classed
		average = average / (float) clscount + 0.5;
	}
	return ieDword(average);
}

// returns the guessed caster level by passed spell type
// FIXME: add iwd2 support (should be more precise, as there are more class types)
// FIXME: add more logic for cross-type kits (like avengers)?
ieDword Actor::GetBaseCasterLevel(int spelltype, int flags) const
{
	int level = 0;

	switch(spelltype)
	{
	case IE_SPL_PRIEST:
		level = GetClericLevel();
		if (!level) level = GetDruidLevel();
		if (!level) level = GetPaladinLevel();
		if (!level) level = GetRangerLevel();
		break;
	case IE_SPL_WIZARD:
		level = GetMageLevel();
		if (!level) level = GetSorcererLevel();
		if (!level) level = GetBardLevel();
		break;
	}
	// if nothing was found, use the average level
	if (!level && !flags) level = GetXPLevel(true);

	return level;
}

int Actor::GetWildMod(int level)
{
	if(GetStat(IE_KIT)&0x1e) {
		// avoid rerolling the mod, since we get called multiple times per each cast
		if (!WMLevelMod) {
			if (level>=MAX_LEVEL) level=MAX_LEVEL;
			if(level<1) level=1;
			WMLevelMod = wmlevels[core->Roll(1,20,-1)][level-1];

			core->GetTokenDictionary()->SetAtCopy("LEVELDIF", abs(WMLevelMod));
			if (WMLevelMod > 0) {
				displaymsg->DisplayConstantStringName(STR_CASTER_LVL_INC, DMC_WHITE, this);
			} else if (WMLevelMod < 0) {
				displaymsg->DisplayConstantStringName(STR_CASTER_LVL_DEC, DMC_WHITE, this);
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
	if (BaseStats[IE_CLASS] == 0 || BaseStats[IE_CLASS] >= (ieDword) classcount) {
		strict = 0;
	}
	return GetBaseCasterLevel(IE_SPL_PRIEST, strict) + GetBaseCasterLevel(IE_SPL_WIZARD, strict);
}

int Actor::CalculateSpeed(bool feedback)
{
	int speed = GetStat(IE_MOVEMENTRATE);
	inventory.CalculateWeight();
	int encumbrance = inventory.GetWeight();
	SetStat(IE_ENCUMBRANCE, encumbrance, false);
	int maxweight;
	//if (third) {
	//	//LOL this won't get us far
	//	maxweight = GetAbilityBonus(IE_STR);
	//} else {
		maxweight = core->GetStrengthBonus(3,GetStat(IE_STR), GetStat(IE_STREXTRA) );
	//}
	if (HasFeat(FEAT_STRONG_BACK)) maxweight += maxweight/2;

	if(encumbrance<=maxweight) {
		return speed;
	}
	if(encumbrance<=maxweight*2) {
		if (feedback) {
			displaymsg->DisplayConstantStringName(STR_HALFSPEED, DMC_WHITE, this);
			//print slow speed
		}
		return speed/2;
	}
	if (feedback) {
		displaymsg->DisplayConstantStringName(STR_CANTMOVE, DMC_WHITE, this);
	}
	return 0;
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

//TODO: needs a way to respawn at a point
void Actor::Resurrect()
{
	if (!(Modified[IE_STATE_ID ] & STATE_DEAD)) {
		return;
	}
	InternalFlags&=IF_FROMGAME; //keep these flags (what about IF_INITIALIZED)
	InternalFlags|=IF_ACTIVE|IF_VISIBLE; //set these flags
	SetBase(IE_STATE_ID, 0);
	SetBase(IE_MORALE, 10);
	//resurrect spell sets the hitpoints to maximum in a separate effect
	//raise dead leaves it at 1 hp
	SetBase(IE_HITPOINTS, 1);
	ClearActions();
	ClearPath();
	SetStance(IE_ANI_EMERGE);
	Game *game = core->GetGame();
	//readjust death variable on resurrection
	if (core->HasFeature(GF_HAS_KAPUTZ) && (AppearanceFlags&APP_DEATHVAR)) {
		ieVariable DeathVar;

		snprintf(DeathVar,sizeof(ieVariable),"%s_DEAD",scriptName);
		ieDword value=0;

		game->kaputz->Lookup(DeathVar, value);
		if (value>0) {
			game->kaputz->SetAt(DeathVar, value-1);
		}
	}
	ResetCommentTime();
	//clear effects?
}

const char *GetVarName(const char *table, int value)
{
	int symbol = core->LoadSymbol( table );
	if (symbol!=-1) {
		Holder<SymbolMgr> sym = core->GetSymbol( symbol );
		return sym->GetValue( value );
	}
	return NULL;
}

void Actor::Die(Scriptable *killer)
{
	int i,j;

	if (InternalFlags&IF_REALLYDIED) {
		return; //can die only once
	}

	//Can't simply set Selected to false, game has its own little list
	Game *game = core->GetGame();
	game->SelectActor(this, false, SELECT_NORMAL);

	displaymsg->DisplayConstantStringName(STR_DEATH, DMC_WHITE, this);
	VerbalConstant(VB_DIE, 1 );

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
	SetStance( IE_ANI_DIE );
	AddTrigger(TriggerEntry(trigger_die));

	Actor *act=NULL;
	if (!killer) {
		// TODO: is this right?
		killer = area->GetActorByGlobalID(LastHitter);
	}

	if (killer) {
		if (killer->Type==ST_ACTOR) {
			act = (Actor *) killer;
		}
	}

	if (InParty) {
		game->PartyMemberDied(this);
		core->Autopause(AP_DEAD, this);
	} else {
		if (act) {
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

	//moved clear actions after xp is given
	//it clears some triggers, including the LastHitter
	ClearActions();
	ClearPath();
	SetModal( MS_NONE );

	ieDword value = 0;
	ieVariable varname;

	// death variables are updated at the moment of death
	if (KillVar[0]) {
		//don't use the raw killVar here (except when the flags explicitly ask for it)
		if (core->HasFeature(GF_HAS_KAPUTZ) ) {
			if (AppearanceFlags&APP_DEATHTYPE) {
				if (AppearanceFlags&APP_ADDKILL) {
					snprintf(varname, 32, "KILL_%s", KillVar);
				} else {
					snprintf(varname, 32, "%s", KillVar);
				}
				game->kaputz->Lookup(varname, value);
				game->kaputz->SetAt(varname, value+1, nocreate);
			}
		} else {
			// iwd/iwd2 path *sets* this var, so i changed it, not sure about pst above
			game->locals->SetAt(KillVar, 1, nocreate);
		}
	}

	if (core->HasFeature(GF_HAS_KAPUTZ) && (AppearanceFlags&APP_FACTION) ) {
		value = 0;
		const char *tmp = GetVarName("faction", BaseStats[IE_FACTION]);
		if (tmp && tmp[0]) {
			if (AppearanceFlags&APP_ADDKILL) {
				snprintf(varname, 32, "KILL_%s", tmp);
			} else {
				snprintf(varname, 32, "%s", tmp);
			}
			game->kaputz->Lookup(varname, value);
			game->kaputz->SetAt(varname, value+1, nocreate);
		}
	}
	if (core->HasFeature(GF_HAS_KAPUTZ) && (AppearanceFlags&APP_TEAM) ) {
		value = 0;
		const char *tmp = GetVarName("team", BaseStats[IE_TEAM]);
		if (tmp && tmp[0]) {
			if (AppearanceFlags&APP_ADDKILL) {
				snprintf(varname, 32, "KILL_%s", tmp);
			} else {
				snprintf(varname, 32, "%s", tmp);
			}
			game->kaputz->Lookup(varname, value);
			game->kaputz->SetAt(varname, value+1, nocreate);
		}
	}

	if (IncKillVar[0]) {
		value = 0;
		game->locals->Lookup(IncKillVar, value);
		game->locals->SetAt(IncKillVar, value + 1, nocreate);
	}

	if (scriptName[0]) {
		value = 0;
		if (core->HasFeature(GF_HAS_KAPUTZ) ) {
			if (AppearanceFlags&APP_DEATHVAR) {
				snprintf(varname, 32, "%s_DEAD", scriptName);
				game->kaputz->Lookup(varname, value);
				game->kaputz->SetAt(varname, value+1, nocreate);
			}
		} else {
			snprintf(varname, 32, core->GetDeathVarFormat(), scriptName);
			game->locals->Lookup(varname, value);
			game->locals->SetAt(varname, value+1, nocreate);
		}

		if (SetDeathVar) {
			value = 0;
			snprintf(varname, 32, "%s_DEAD", scriptName);
			game->locals->Lookup(varname, value);
			game->locals->SetAt(varname, 1, nocreate);
			if (value) {
				snprintf(varname, 32, "%s_KILL_CNT", scriptName);
				value = 1;
				game->locals->Lookup(varname, value);
				game->locals->SetAt(varname, value + 1, nocreate);
			}
		}
	}

	if (IncKillCount) {
		// racial dead count
		value = 0;
		int racetable = core->LoadSymbol("race");
		if (racetable != -1) {
			Holder<SymbolMgr> race = core->GetSymbol(racetable);
			const char *raceName = race->GetValue(Modified[IE_RACE]);
			if (raceName) {
				// todo: should probably not set this for humans in iwd?
				snprintf(varname, 32, "KILL_%s_CNT", raceName);
				game->locals->Lookup(varname, value);
				game->locals->SetAt(varname, value+1, nocreate);
			}
		}
	}

	//death counters for PST
	j=APP_GOOD;
	for(i=0;i<4;i++) {
		if (AppearanceFlags&j) {
			value = 0;
			game->locals->Lookup(CounterNames[i], value);
			game->locals->SetAt(CounterNames[i], value+DeathCounters[i], nocreate);
		}
		j+=j;
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
			value = 0;
			area->locals->Lookup(varname, value);
			// i am guessing that we shouldn't decrease below 0
			if (value > 0) {
				area->locals->SetAt(varname, value-1);
			}
		}
	}

	//a plot critical creature has died (iwd2)
	if (BaseStats[IE_MC_FLAGS]&MC_PLOT_CRITICAL) {
		core->GetGUIScriptEngine()->RunFunction("GUIWORLD", "DeathWindowPlot", false);
	}
	//ensure that the scripts of the actor will run as soon as possible
	ImmediateEvent();
}

void Actor::SetPersistent(int partyslot)
{
	InParty = (ieByte) partyslot;
	InternalFlags|=IF_FROMGAME;
	//if an actor is coming from a game, it should have these too
	CreateStats();
}

void Actor::DestroySelf()
{
	InternalFlags|=IF_CLEANUP;
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
	if (InternalFlags&IF_JUSTDIED || CurrentAction || GetNextAction()) {
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
	if (gc && (GetGlobalID() == gc->dialoghandler->targetID || GetGlobalID() == gc->dialoghandler->speakerID)) {
		return false;
	}

	//we need to check animID here, if it has not played the death
	//sequence yet, then we could return now
	ClearActions();
	//missed the opportunity of Died()
	InternalFlags&=~IF_JUSTDIED;

	// items seem to be dropped at the moment of death
	// .. but this can't go in Die() because that is called
	// from effects and dropping items might change effects!

	//destroy normal items if difficulty level is high enough
	if ((LastDamageType & DAMAGE_MAGIC) && (GameDifficulty>DIFF_CORE) ) {
		inventory.DestroyItem("", IE_INV_ITEM_DESTRUCTIBLE, (ieDword) ~0);
	}
	DropItem("",0);

	//remove all effects that are not 'permanent after death' here
	//permanent after death type is 9
	SetBaseBit(IE_STATE_ID, STATE_DEAD, true);

	// party actors are never removed
	if (Persistent()) return false;

	if (Modified[IE_MC_FLAGS]&MC_REMOVE_CORPSE) return true;
	if (Modified[IE_MC_FLAGS]&MC_KEEP_CORPSE) return false;
	//if chunked death, then return true
	if (LastDamageType&DAMAGE_CHUNKING) {
		//play chunky animation
		//chunks are projectiles
		return true;
	}
	return false;
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
	Item *itm = gamedata->GetItem(slot->ItemResRef);
	if (!itm) return; //quick item slot contains invalid item resref
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
		int which;
		if (i<0) which = ACT_WEAPON4+i+1;
		else which = PCStats->QSlots[i];
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
	if (version==22) {
		CheckWeaponQuickSlot(2);
		CheckWeaponQuickSlot(3);
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
			Item *itm = gamedata->GetItem(slotitm->ItemResRef);
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
int Actor::GetHpAdjustment(int multiplier) const
{
	int val;

	// only player classes get this bonus
	if (BaseStats[IE_CLASS] == 0 || BaseStats[IE_CLASS] >= (ieDword) classcount) {
		return 0;
	}

	// GetClassLevel/IsWarrior takes into consideration inactive dual-classes, so those would fail here
	if (IsWarrior()) {
		val = core->GetConstitutionBonus(STAT_CON_HP_WARRIOR,Modified[IE_CON]);
	} else {
		val = core->GetConstitutionBonus(STAT_CON_HP_NORMAL,Modified[IE_CON]);
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
	//default is 9 in Tob (is this true? or just most anims are 9?)
	SetBase(IE_MOVEMENTRATE,9);

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
	inventory.CalculateWeight();
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
		if (featspells[i] && featspells[i][0]!='*') {
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

bool Actor::ValidTarget(int ga_flags, Scriptable *checker) const
{
	//scripts can still see this type of actor

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
		break;
	}
	if (ga_flags&GA_NO_DEAD) {
		if (InternalFlags&IF_REALLYDIED) return false;
		if (Modified[IE_STATE_ID] & STATE_DEAD) return false;
	}
	if (ga_flags&GA_SELECT) {
		if (UnselectableTimer) return false;
		if (Immobile()) return false;
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
	Item *item = gamedata->GetItem(wield->ItemResRef);
	if (!item) {
		return NULL;
	}
	//The magic of the bow and the arrow add up?
	wi.enchantment += item->Enchantment;
	wi.itemflags = wield->Flags;
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
	if (!wield || slot == inventory.GetFistSlot()) {
		return 0;
	}

	Item *itm = gamedata->GetItem( wield->ItemResRef );
	if (!itm) {
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
	Item *item = gamedata->GetItem(wield->ItemResRef);
	if (!item) {
		return 0;
	}

	wi.enchantment = item->Enchantment;
	wi.itemflags = wield->Flags;
	wi.prof = item->WeaProf;
	wi.critmulti = core->GetCriticalMultiplier(item->ItemType);

	//select first weapon header
	ITMExtHeader *which;
	if (GetAttackStyle() == WEAPON_RANGED) {
		which = item->GetWeaponHeader(true);
		if (which) {
			wi.backstabbing = which->RechargeFlags & IE_ITEM_BACKSTAB;
		} else {
			wi.backstabbing = false;
		}
	} else {
		which = item->GetWeaponHeader(false);
		// any melee weapon usable by a single class thief is game (UAI does not affect this)
		// but also check a bit in the recharge flags (modder extension)
		if (which) {
			wi.backstabbing = !(item->UsabilityBitmask & 0x400000) || (which->RechargeFlags & IE_ITEM_BACKSTAB);
		} else {
			wi.backstabbing = !(item->UsabilityBitmask & 0x400000);
		}
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

int Actor::LearnSpell(const ieResRef spellname, ieDword flags)
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
		if (GetKitIndex(kit) && spell->PrimaryType) {
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

	int clsmsk = GetBookMask();
	int explev = spellbook.LearnSpell(spell, flags&LS_MEMO, clsmsk, kit );
	int tmp = spell->SpellName;
	if (flags&LS_LEARN) {
		core->GetTokenDictionary()->SetAt("SPECIALABILITYNAME", core->GetString(tmp));
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
		Game *game = core->GetGame();
		game->ShareXP(xp, SX_DIVIDE);
	}
	return LSR_OK;
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

void Actor::CreateStats()
{
	if (!PCStats) {
		PCStats = new PCStatsStruct();
	}
}

const char* Actor::GetScript(int ScriptIndex) const
{
	return Scripts[ScriptIndex]->GetName();
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

	if (ModalState == MS_BATTLESONG && ModalState != newstate && HasFeat(FEAT_LINGERING_SONG)) {
		strnlwrcpy(LingeringModalSpell, ModalSpell, 8);
		modalSpellLingering = 2;
	}

	if (IsSelected()) {
		// display the turning-off message
		if (ModalState != MS_NONE) {
			displaymsg->DisplayStringName(core->ModalStates[ModalState].leaving_str, DMC_WHITE, this, IE_STR_SOUND|IE_STR_SPEECH);
		}

		// when called with the same state twice, toggle to MS_NONE
		if (!force && ModalState == newstate) {
			ModalState = MS_NONE;
		} else {
			ModalState = newstate;
		}

		//update the action bar
		core->SetEventFlag(EF_ACTION);
	} else {
		ModalState = newstate;
	}
}

void Actor::SetModalSpell(ieDword state, const char *spell)
{
	if (spell) {
		strnlwrcpy(ModalSpell, spell, 8);
	} else {
		if (state >= core->ModalStates.size()) {
			ModalSpell[0] = 0;
		} else {
			if (state==MS_BATTLESONG) {
				if (BardSong[0]) {
					strnlwrcpy(ModalSpell, BardSong, 8);
					return;
				}
			}
			strnlwrcpy(ModalSpell, core->ModalStates[state].spell, 8);
		}
	}
}

//this is just a stub function for now, attackstyle could be melee/ranged
//even spells got this attack style
int Actor::GetAttackStyle() const
{
	WeaponInfo wi;
	//Non NULL if the equipped slot is a projectile or a throwing weapon
	//TODO some weapons have both melee and ranged capability
	if (GetRangedWeapon(wi) != NULL) return WEAPON_RANGED;
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
	SetOrientation( GetOrient( target->Pos, Pos ), false );
}

//in case of LastTarget = 0
void Actor::StopAttack()
{
	SetStance(IE_ANI_READY);
	secondround = 0;
	//InternalFlags|=IF_TARGETGONE; //this is for the trigger!
	if (InParty) {
		core->Autopause(AP_NOTARGET, this);
	}
}

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
	Game *game = core->GetGame();
	if (game && game->TimeStoppedFor(this)) {
		return 1;
	}

	return 0;
}

ieDword Actor::GetNumberOfAttacks() const
{
	int bonus = 0;

	if (monkbon != NULL && Equipped == IW_NO_EQUIPPED) {
		unsigned int level = GetMonkLevel();
		if (level>=monkbon_cols) level=monkbon_cols-1;
		if (level>0) {
			bonus = monkbon[0][level-1];
		}
	}

	return GetStat(IE_NUMBEROFATTACKS)+bonus;
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
	if (attackcount < 1) {
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

bool Actor::GetCombatDetails(int &tohit, bool leftorright, WeaponInfo& wi, ITMExtHeader *&header, ITMExtHeader *&hittingheader, \
		int &DamageBonus, int &speed, int &CriticalBonus, int &style, Actor *target) const
{
	tohit = GetStat(IE_TOHIT);
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
			//display out of ammo verbal constant if there is any???
			//VerbalConstant(VB_OUTOFAMMO, 1 );
			//SetStance(IE_ANI_READY);
			//set some trigger?
			return false;
		}
		wi.wflags = WEAPON_RANGED;
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
	//TODO easier copying of recharge flags into wflags
	//this flag is set by the bow in case of projectile launcher.
	if (header->RechargeFlags&IE_ITEM_USESTRENGTH) wi.wflags|=WEAPON_USESTRENGTH;
	// this flag is set in dagger/shortsword by the loader
	if (header->RechargeFlags&IE_ITEM_USEDEXTERITY) wi.wflags|=WEAPON_FINESSE;
	//also copy these flags
	wi.wflags|=header->RechargeFlags&(IE_ITEM_KEEN|IE_ITEM_IGNORESHIELD);

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
	if (!stars && HasSpellState(SS_TENSER)) stars = 1;

	//hit/damage/speed bonuses from wspecial
	if ((signed)stars > wspecial_max) {
		stars = wspecial_max;
	}

	if (wi.wflags&WEAPON_BYPASS) {
		//FIXME:this type of weapon ignores all armor, -4 is for balance?
		//or i just got lost a negation somewhere
		THAC0Bonus -= 4;
	} else {
		// everyone is proficient with fists
		if (Equipped != IW_NO_EQUIPPED) {
			THAC0Bonus += wspecial[stars][0];
		}
	}
	DamageBonus += wspecial[stars][1];
	speed += wspecial[stars][2];
	// add non-proficiency penalty, which is missing from the table
	if (stars == 0 && !third) {
		ieDword clss = BaseStats[IE_CLASS];
		//Is it a PC class?
		if (clss < (ieDword) classcount) {
			THAC0Bonus -= defaultprof[clss];
		} else {
			//it is not clear what is the penalty for non player classes
			THAC0Bonus += 4;
		}
	}

	if (dualwielding && wsdualwield) {
		//add dual wielding penalty
		stars = GetStat(IE_PROFICIENCY2WEAPON)&PROFS_MASK;
		if (stars > STYLE_MAX) stars = STYLE_MAX;

		style = 1000*stars + IE_PROFICIENCY2WEAPON;
		THAC0Bonus += wsdualwield[stars][leftorright?1:0];
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

	// TODO: Elves get a racial THAC0 bonus with all swords and bows in BG2 (but not daggers)

	//second parameter is left or right hand flag
	tohit = GetToHit(THAC0Bonus, wi.wflags, target);
	if (third) {
		// iwd2 gives a dualwielding bonus when using a simple weapon in the offhand
		// it is limited to shortswords and daggers, which also have this flag set
		if (dualwielding && (wi.wflags&WEAPON_FINESSE)) {
			tohit += 2;
		}
	}

	//pst increased critical hits
	if (pstflags && (Modified[IE_STATE_ID]&STATE_CRIT_ENH)) {
		CriticalBonus--;
	}
	return true;
}

int Actor::MeleePenalty() const
{
	if (GetMonkLevel()) return 0;
	if (inventory.GetEquippedSlot()!=IW_NO_EQUIPPED) return 4;
	return 0;
}

int Actor::GetToHit(int bonus, ieDword Flags, Actor *target) const
{
	int tohit = bonus;
	int strbonus = 0;

	//get our dual wielding modifier
	if (IsDualWielding()) {
		if (Flags&WEAPON_LEFTHAND) {
			tohit += GetStat(IE_HITBONUSLEFT);
		} else {
			tohit += GetStat(IE_HITBONUSRIGHT);
		}
		if (third) {
			// FIXME: externalise
			// penalites and boni for both hands:
			// -6 main, -10 off with no adjustments
			//  0 main, +4 off with ambidexterity
			// +2 main, +2 off with two weapon fighting
			// +2 main, +2 off with a simple weapons in the off hand (handled in GetCombatDetails)
			if (HasFeat(FEAT_TWO_WEAPON_FIGHTING)) {
				tohit += 2;
			}
			if (Flags&WEAPON_LEFTHAND) {
				tohit -= 6;
			} else {
				tohit -= 10;
				if (HasFeat(FEAT_AMBIDEXTERITY)) {
					tohit += 4;
				}
			}
		}
	}

	//add strength bonus if we need
	if (Flags&WEAPON_USESTRENGTH) {
		if (third) {
			strbonus = GetAbilityBonus(IE_STR );
		} else {
			strbonus = core->GetStrengthBonus(0,GetStat(IE_STR), GetStat(IE_STREXTRA) );
		}
	}

	//get attack style (melee or ranged)
	switch(Flags&WEAPON_STYLEMASK) {
		case WEAPON_MELEE:
			tohit += GetStat(IE_MELEETOHIT);
			if ((Flags&WEAPON_FINESSE) && HasFeat(FEAT_WEAPON_FINESSE) ) {
				int dexbonus;
				if (third) {
					dexbonus = GetAbilityBonus(IE_DEX );
				} else {
					//FIXME check with ToBEx if they want a new column for this
					dexbonus = core->GetDexterityBonus(STAT_DEX_MISSILE, GetStat(IE_DEX));
				}
				if (dexbonus>strbonus) strbonus = dexbonus;
			}
			break;
		case WEAPON_FIST:
			tohit += GetStat(IE_FISTHIT);
			break;
		case WEAPON_RANGED:
			tohit += GetStat(IE_MISSILEHITBONUS);
			//add dexterity bonus
			if (third) {
				tohit += GetAbilityBonus(IE_DEX );
			} else {
				tohit += core->GetDexterityBonus(STAT_DEX_MISSILE, GetStat(IE_DEX));
			}
			break;
	}
	tohit += strbonus;

	// check if there is any armor unproficiency penalty
	tohit += GetArmorFailure();

	if (target) {
		// if the target is using a ranged weapon while we're meleeing, we get a +4 bonus
		if ((Flags&WEAPON_STYLEMASK) != WEAPON_RANGED) {
			if (target->GetAttackStyle() == WEAPON_RANGED) {
				tohit += 4;
			}
		}

		// melee vs. unarmed
		tohit += target->MeleePenalty() - MeleePenalty();

		// add +4 attack bonus vs racial enemies
		if (GetRangerLevel()) {
			if (IsRacialEnemy(target)) {
				tohit += 4;
			}
		}
		tohit += fxqueue.BonusAgainstCreature(fx_tohit_vs_creature_ref, target);

	}


	if (ReverseToHit) {
		tohit = (signed)GetStat(IE_TOHIT)-tohit;
	} else {
		tohit += GetStat(IE_TOHIT);
	}
	return tohit;
}

int Actor::GetDefense(int DamageType, ieDword wflags, Actor *attacker) const
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


	if (! (wflags&WEAPON_BYPASS)) {
		if (ReverseToHit) {
			defense = GetStat(IE_ARMORCLASS)-defense;
		} else {
			defense += GetStat(IE_ARMORCLASS);
		}
	}
	//Dexterity bonus is stored negative in 2da files.
	defense += GetDexterityAC();

	if (attacker) {
		defense -= fxqueue.BonusAgainstCreature(fx_ac_vs_creature_type_ref,attacker);
	}
	return defense;
}

void Actor::PerformAttack(ieDword gameTime)
{
	if (InParty) {
		// TODO: this is temporary hack
		Game *game = core->GetGame();
		game->PartyAttack = true;
	}

	// if held or disabled, etc, then cannot continue attacking
	// TODO: should be in action
	ieDword state = GetStat(IE_STATE_ID);
	if (state&STATE_CANTMOVE || Immobile()) {
		// this is also part of the UpdateActorState hack below. sorry!
		lastattack = gameTime;
		return;
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

	//can't target invisible or dead opponent
	if (target) {
		ieDword flag = target->GetSafeStat(IE_STATE_ID);
		if (!GetSafeStat(IE_SEEINVISIBLE) ) flag &= STATE_DEAD|state_invisible;
		else flag &= STATE_DEAD;

		if (flag) {
			target = NULL;
		}
	}

	if (!target) {
		Log(WARNING, "Actor", "Attack without valid target!");
		return;
	}

	target->AttackedBy(this);

	print("Performattack for %s, target is: %s", ShortName, target->ShortName);

	//which hand is used
	//we do apr - attacksleft so we always use the main hand first
	bool leftorright = (bool) ((attacksperround-attackcount)&1);

	WeaponInfo wi;
	ITMExtHeader *header = NULL;
	ITMExtHeader *hittingheader = NULL;
	int tohit;
	//ieDword Flags;
	int DamageBonus, CriticalBonus;
	int speed, style;

	//will return false on any errors (eg, unusable weapon)
	if (!GetCombatDetails(tohit, leftorright, wi, header, hittingheader, DamageBonus, speed, CriticalBonus, style, target)) {
		return;
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

	// FIXME: use proper weapon range
	if((PersonalDistance(this, target) > wi.range*10) || (GetCurrentArea()!=target->GetCurrentArea() ) ) {
		// this is a temporary double-check, remove when bugfixed
		Log(ERROR, "Actor", "Attack action didn't bring us close enough!");
		return;
	}

	SetStance(AttackStance);

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
		buffer.append("[Missed]");
		Log(COMBAT, "Attack", buffer);
		return;
	}

	// iwd2 rerolls to check for criticals (cf. manual page 45) - the second roll just needs to hit; on miss, it degrades to a normal hit
	// CriticalBonus is negative, it is added to the minimum roll needed for a critical hit
	// IE_CRITICALHITBONUS is positive, it is subtracted
	int roll = LuckyRoll(1, ATTACKROLL, 0, LR_CRITICAL);
	int criticalroll = roll + (int) GetStat(IE_CRITICALHITBONUS) - CriticalBonus;
	if (third) {
		int ThreatRangeMin = ATTACKROLL; // FIXME: this is just the default
		if (header && (header->RechargeFlags&IE_ITEM_KEEN)) {
			//this is correct, the threat range is only increased by one in the original engine
			ThreatRangeMin--;
		}
		ThreatRangeMin -= ((int) GetStat(IE_CRITICALHITBONUS) - CriticalBonus); // TODO: move to GetCombatDetails
		criticalroll = LuckyRoll(1, ATTACKROLL, 0, LR_CRITICAL);
		if (criticalroll < ThreatRangeMin) {
			// make it an ordinary hit
			criticalroll = 1;
		} else {
			// make sure it will be a critical hit
			criticalroll = ATTACKROLL;
		}
	}
	if (roll==1) {
		//critical failure
		buffer.append("[Critical Miss]");
		Log(COMBAT, "Attack", buffer);
		displaymsg->DisplayConstantStringName(STR_CRITICAL_MISS, DMC_WHITE, this);
		VerbalConstant(VB_CRITMISS, 1);
		if (wi.wflags&WEAPON_RANGED) {//no need for this with melee weapon!
			UseItem(wi.slot, (ieDword)-2, target, UI_MISS);
		} else if (core->HasFeature(GF_BREAKABLE_WEAPONS)) {
			//break sword
			// a random roll on-hit (perhaps critical failure too)
			//  in 0,5% (1d20*1d10==1) cases
			if ((header->RechargeFlags&IE_ITEM_BREAKABLE) && core->Roll(1,10,0) == 1) {
				inventory.BreakItemSlot(wi.slot);
			}
		}
		ResetState();
		return;
	}
	//damage type is?
	//modify defense with damage type
	ieDword damagetype = hittingheader->DamageType;
	int damage = 0;

	if (hittingheader->DiceThrown<256) {
		damage += LuckyRoll(hittingheader->DiceThrown, hittingheader->DiceSides, DamageBonus, LR_DAMAGELUCK);
	} else {
		damage = 0;
	}

	bool critical = criticalroll>=ATTACKROLL;
	bool success = critical;

	if (!critical) {
		//get target's defense against attack
		int defense = target->GetDefense(damagetype, wi.wflags, this);

		if(ReverseToHit) {
			success = roll + defense > tohit;
		} else {
			success = tohit + roll > defense;
		}
		// autohit immobile enemies (true for atleast stun, sleep, timestop)
		if (target->Immobile() || (target->GetStat(IE_STATE_ID) & STATE_SLEEP)) {
			success = true;
		}
	}

	if (!success) {
		//hit failed
		if (wi.wflags&WEAPON_RANGED) {//Launch the projectile anyway
			UseItem(wi.slot, (ieDword)-2, target, UI_MISS);
		}
		ResetState();
		buffer.append("[Missed]");
		Log(COMBAT, "Attack", buffer);
		return;
	}

	ModifyWeaponDamage(wi, target, damage, critical);

	if (critical) {
		//critical success
		buffer.append("[Critical Hit]");
		Log(COMBAT, "Attack", buffer);
		displaymsg->DisplayConstantStringName(STR_CRITICAL_HIT, DMC_WHITE, this);
		VerbalConstant(VB_CRITHIT, 1);
	} else {
		//normal success
		buffer.append("[Hit]");
		Log(COMBAT, "Attack", buffer);
	}
	UseItem(wi.slot, wi.wflags&WEAPON_RANGED?-2:-1, target, critical?UI_CRITICAL:0, damage);
	ResetState();
}

int Actor::WeaponDamageBonus(const WeaponInfo &wi) const
{
	if (wi.wflags&WEAPON_USESTRENGTH) {
		if (third) {
			int bonus = GetAbilityBonus(IE_STR);
			// 150% bonus for twohanders
			if (wi.itemflags&IE_INV_ITEM_TWOHANDED) bonus+=bonus/2;
			return bonus;
		}
		return core->GetStrengthBonus(1, GetStat(IE_STR), GetStat(IE_STREXTRA) );
	}

	return 0;
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
			print("Unhandled damagetype:%d", damagetype);
		} else if (it->second.resist_stat) {
			// damage type with a resistance stat
			resisted = (int) (damage * (signed)GetSafeStat(it->second.resist_stat)/100.0);
			// check for bonuses for specific damage types
			if (core->HasFeature(GF_SPECIFIC_DMG_BONUS) && attacker) {
				int bonus = attacker->fxqueue.SpecificDamageBonus(it->second.iwd_mod_type);
				if (bonus) {
					resisted -= int (damage * bonus / 100.0);
					print("Bonus damage of %d(%+d%%), neto: %d", int(damage * bonus / 100.0), bonus, -resisted);
				}
			}
			damage -= resisted;
			print("Resisted %d of %d at %d%% resistance to %d", resisted, damage+resisted, GetSafeStat(it->second.resist_stat), damagetype);
			// TODO: PST and BG1 may actually heal on negative damage
			if (damage <= 0) resisted = DR_IMMUNE;
		}
	}

	if (damage<=0) {
		if (attacker && attacker->InParty) {
			attacker->VerbalConstant(VB_TIMMUNE, 1);
			core->Autopause(AP_UNUSABLE, this);
		}
	}
}

void Actor::UpdateActorState(ieDword gameTime) {
	if (modalTime==gameTime) {
		return;
	}

	//IWD2 has no autodetect, you actually should 'search'
	//actually, iwd2 has autosearch, also, this is useful for dayblindness
	if (InParty && core->HasFeature(GF_AUTOSEARCH_HIDDEN) ) {
		core->ApplySpell("detect", this, this, 0);
	}

	ieDword state = Modified[IE_STATE_ID];

	if ((state&STATE_CONFUSED) && !(gameTime&0x3f) ) {
		char Tmp[40];

		int tmp = core->Roll(1,3,0);
		switch(tmp) {
		case 2:
			strncpy(Tmp,"RandomWalk()",sizeof(Tmp));
			break;
		case 1:
			strncpy(Tmp,"Attack([0])",sizeof(Tmp));
			break;
		default:
			strncpy(Tmp,"NoAction()",sizeof(Tmp));
			break;
		}
		Action *action = GenerateAction( Tmp );
		if (action) {
			AddActionInFront(action);
		}
		return;
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

	if (ModalState == MS_NONE && !modalSpellLingering) {
		return;
	}

	//apply the modal effect on the beginning of each round
	if ((((gameTime-roundTime)%core->Time.round_size)==0)) {
		// handle lingering modal spells like bardsong in iwd2
		if (modalSpellLingering && LingeringModalSpell[0]) {
			modalSpellLingering--;
			if (core->ModalStates[ModalState].aoe_spell) {
				core->ApplySpellPoint(LingeringModalSpell, GetCurrentArea(), Pos, this, 0);
			} else {
				core->ApplySpell(LingeringModalSpell, this, this, 0);
			}
		}
		if (ModalState == MS_NONE) {
			return;
		}

		// some states and timestop disable modal actions
		// interestingly the original doesn't include STATE_DISABLED, STATE_FROZEN/STATE_PETRIFIED
		if (Immobile() || (state & (STATE_CONFUSED | STATE_DEAD | STATE_HELPLESS | STATE_PANIC | STATE_BERSERK | STATE_SLEEP))) {
			return;
		}

		//we can set this to 0
		modalTime = gameTime;

		if (!ModalSpell[0]) {
			Log(WARNING, "Actor", "Modal Spell Effect was not set!");
			ModalSpell[0]='*';
		} else if(ModalSpell[0]!='*') {
			if (ModalSpellSkillCheck()) {
				if (core->ModalStates[ModalState].aoe_spell) {
					core->ApplySpellPoint(ModalSpell, GetCurrentArea(), Pos, this, 0);
				} else {
					core->ApplySpell(ModalSpell, this, this, 0);
				}
				if (InParty) {
					displaymsg->DisplayStringName(core->ModalStates[ModalState].entering_str, DMC_WHITE, this, IE_STR_SOUND|IE_STR_SPEECH);
				}
			} else {
				if (InParty) {
					displaymsg->DisplayStringName(core->ModalStates[ModalState].failed_str, DMC_WHITE, this, IE_STR_SOUND|IE_STR_SPEECH);
				}
				ModalState = MS_NONE;
				// TODO: wait for a round until allowing new states?
			}
		}

		// shut everyone up, so they don't whine if the actor is on a long hiding-in-shadows recon mission
		core->GetGame()->ResetPartyCommentTimes();
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
	if (anims->lockPalette) {
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
			int phase)
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

//if days == 0, it means full healing
void Actor::Heal(int days)
{
	if (days) {
		SetBase(IE_HITPOINTS, BaseStats[IE_HITPOINTS]+days*2);
	} else {
		SetBase(IE_HITPOINTS, Modified[IE_MAXHITPOINTS]);
	}
}

void Actor::AddExperience(int exp, int combat)
{
	if (core->HasFeature(GF_WISDOM_BONUS)) {
		//TODO find out the 3ED variant
		exp = (exp * (100 + core->GetWisdomBonus(0, Modified[IE_WIS]))) / 100;
	}
	if (combat) {
		exp += exp*xpadjustments[GameDifficulty]/100;
	}
	SetBase(IE_XP,BaseStats[IE_XP]+exp);
}

int Actor::CalculateExperience(int type, int level)
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

bool Actor::Schedule(ieDword gametime, bool checkhide)
{
	if (checkhide) {
		if (!(InternalFlags&IF_VISIBLE) ) {
			return false;
		}
	}

	//check for schedule
	ieDword bit = 1<<((gametime/6)%7200/300);
	if (appearance & bit) {
		return true;
	}
	return false;
}

void Actor::NewPath()
{
	PathTries++;
	Point tmp = Destination;
	ClearPath();
	if (PathTries>10) {
		return;
	}
	Movable::WalkTo(tmp, size );
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

void Actor::WalkTo(const Point &Des, ieDword flags, int MinDistance)
{
	PathTries = 0;
	if (InternalFlags&IF_REALLYDIED) {
		return;
	}
	SetRunFlags(flags);
	ResetCommentTime();
	// is this true???
	if (Des.x==-2 && Des.y==-2) {
		Point p((ieWord) Modified[IE_SAVEDXPOS], (ieWord) Modified[IE_SAVEDYPOS] );
		Movable::WalkTo(p, MinDistance);
	} else {
		Movable::WalkTo(Des, MinDistance);
	}
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
			unsigned char Face, const Color& tint)
{
	CharAnimations* ca = GetAnims();
	int PartCount = ca->GetTotalPartCount();
	Video* video = core->GetVideoDriver();
	Region vp = video->GetViewport();
	unsigned int flags = TranslucentShadows ? BLIT_TRANSSHADOW : 0;
	if (!ca->lockPalette) flags |= BLIT_TINTED;
	Game* game = core->GetGame();
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
		if (nextFrame && bbox.InsideRegion( vp ) ) {
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

			video->BlitGameSprite( nextFrame, cx + screen.x, cy + screen.y,
				flags, tint, newsc, ca->GetPartPalette(partnum), &screen);
		}
	}
}


static const int OrientdX[16] = { 0, -4, -7, -9, -10, -9, -7, -4, 0, 4, 7, 9, 10, 9, 7, 4 };
static const int OrientdY[16] = { 10, 9, 7, 4, 0, -4, -7, -9, -10, -9, -7, -4, 0, 4, 7, 9 };
static const unsigned int MirrorImageLocation[8] = { 4, 12, 8, 0, 6, 14, 10, 2 };
static const unsigned int MirrorImageZOrder[8] = { 2, 4, 6, 0, 1, 7, 5, 3 };

bool Actor::ShouldHibernate() {
	//finding an excuse why we don't hybernate the actor
	if (Modified[IE_ENABLEOFFSCREENAI])
		return false;
	if (LastTarget) //currently attacking someone
		return false;
	if (InternalFlags&IF_JUSTDIED) // didn't have a chance to run a script
		return false;
	if (CurrentAction)
		return false;
	if (GetNextStep())
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
	if (Immobile() || !ShouldDrawCircle()) {
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

	//If you find a better place for it, I'll really be glad to put it there
	//IN BG1 and BG2, this is at the ninth frame...
	if(attackProjectile && (anims[0]->GetCurrentFrame() == 8/*anims[0]->GetFramesCount()/2*/)) {
		GetCurrentArea()->AddProjectile(attackProjectile, Pos, LastTarget, false);
		attackProjectile = NULL;
	}

	// advance first (main) animation by one frame (in sync)
	if (Immobile()) {
		// update animation, continue last-displayed frame
		anims[0]->LastFrame();
	} else {
		// update animation, maybe advance a frame (if enough time has passed)
		anims[0]->NextFrame();
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
		}
	} else {
		//check if walk sounds need to be played
		//dialog, pause game
		if (!(core->GetGameControl()->GetDialogueFlags()&(DF_IN_DIALOG|DF_FREEZE_SCRIPTS) ) ) {
			//footsteps option set, stance
			if (footsteps && (GetStance() == IE_ANI_WALK)) {
				//frame reached 0
				if (!anims[0]->GetCurrentFrame()) {
					PlayWalkSound();
				}
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
	Map* area = GetCurrentArea();

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
			return;
		}
	}

	// if an actor isn't visible, should we still draw video cells?
	// let us assume not, for now..
	if (!(InternalFlags & IF_VISIBLE)) {
		return;
	}

	//visual feedback
	CharAnimations* ca = GetAnims();
	if (!ca) {
		return;
	}

	//explored or visibilitymap (bird animations are visible in fog)
	//0 means opaque
	int Trans = Modified[IE_TRANSLUCENT];

	if (Trans>255) {
		Trans=255;
	}

	//iwd has this flag saved in the creature
	if (Modified[IE_AVATARREMOVAL]) {
		Trans = 255;
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
	GameControl *gc = core->GetGameControl();
	if (gc->GetScreenFlags()&SF_CUTSCENE) {
		// ground circles are not drawn in cutscenes
		drawcircle = false;
	}
	// the speaker should get a circle even in cutscenes
	if (gc->dialoghandler->targetID == GetGlobalID() && (gc->GetDialogueFlags()&DF_IN_DIALOG)) {
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
	if (drawcircle) {
		DrawCircle(vp);
		drawtarget = ((Selected || Over) && !(InternalFlags&IF_NORETICLE) && Modified[IE_EA] <= EA_CONTROLLABLE && GetPathLength());
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
				if (area->GetBlocked(iPos) & (PATH_MAP_PASSABLE|PATH_MAP_ACTOR)) {
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

		//infravision tint
		if ( HasBodyHeat() &&
			(area->GetLightLevel(Pos)<128) &&
			core->GetGame()->PartyHasInfravision()) {
			tint.r=255;
		}

		// actor itself
		newsc = sc = GetSpriteCover();
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
				if (area->GetBlocked(iPos) & (PATH_MAP_PASSABLE|PATH_MAP_ACTOR)) {
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
		return true;
	}
	int x = rand()%1000;
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

void Actor::GetSoundFrom2DA(ieResRef Sound, unsigned int index) const
{
	if (!anims) return;

	AutoTable tab(anims->ResRef);
	if (!tab)
		return;

	switch (index) {
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
			index = 36;
			break;
		default:
			Log(WARNING, "Actor", "TODO:Cannot determine 2DA rowcount for index: %d", index);
			return;
	}
	Log(MESSAGE, "Actor", "Getting sound 2da %.8s entry: %s",
		anims->ResRef, tab->GetRowName(index) );
	int col = core->Roll(1,tab->GetColumnCount(index),-1);
	strnlwrcpy(Sound, tab->QueryField (index, col), 8);
}

//Get the monster sound from a global .ini file.
//It is ResData.ini in PST and Sounds.ini in IWD/HoW
void Actor::GetSoundFromINI(ieResRef Sound, unsigned int index) const
{
	const char *resource = "";
	char section[12];
	unsigned int animid=BaseStats[IE_ANIMATION_ID];
	if(core->HasFeature(GF_ONE_BYTE_ANIMID)) {
		animid&=0xff;
	}
	snprintf(section,10,"%d", animid);

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
	}
	int count = CountElements(resource,',');
	if (count<=0) return;
	count = core->Roll(1,count,-1);
	while(count--) {
		while(*resource && *resource!=',') resource++;
			if (*resource==',') resource++;
	}
	strncpy(Sound, resource, 8);
	for(count=0;count<8 && Sound[count]!=',';count++) {};
	Sound[count]=0;
}

void Actor::ResolveStringConstant(ieResRef Sound, unsigned int index) const
{
	if (PCStats && PCStats->SoundSet[0]) {
		//resolving soundset (bg1/bg2 style)
		if (csound[index]) {
			snprintf(Sound, sizeof(ieResRef), "%s%c", PCStats->SoundSet, csound[index]);
			return;
		}
		//icewind style
		snprintf(Sound, sizeof(ieResRef), "%s%02d", PCStats->SoundSet, VCMap[index]);
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
		ieByte tmp = ar[i];
		if (QslotTranslation && i>2) {
			if (tmp>ACT_IWDQSONG) {//quick songs
				tmp = 110+tmp%10;
			} else if (tmp>ACT_IWDQSPEC) {//quick abilities
				tmp = 90+tmp%10;
			} else if (tmp>ACT_IWDQITEM) {//quick items
				tmp = 80+tmp%10;
			} else if (tmp>ACT_IWDQSPELL) {//quick spells
				tmp = 70+tmp%10;
			} else if (tmp>ACT_BARD) {//spellbooks
				tmp = 50+tmp%10;
			} else {
				tmp=gemrb2iwd[tmp];
			}
		}
		PCStats->QSlots[i]=tmp;
	}
}

void Actor::GetActionButtonRow(ActionButtonRow &ar)
{
	//at this point, we need the stats for the action button row
	//only controlled creatures (and pcs) get it
	CreateStats();
	InitButtons(GetStat(IE_CLASS), false);
	for(int i=0;i<GUIBT_COUNT;i++) {
		ieByte tmp=PCStats->QSlots[i];
		//the first three buttons are hardcoded in gemrb
		//don't mess with them
		if (QslotTranslation && i>2) {
			if (tmp>=110) { //quick songs
				tmp = ACT_IWDQSONG+tmp%10;
			} else if (tmp>=90) { //quick abilities
				tmp=ACT_IWDQSPEC+tmp%10;
			} else if (tmp>=80) { //quick items
				tmp=ACT_IWDQITEM+tmp%10;
			} else if (tmp>=70) { //quick spells
				tmp=ACT_IWDQSPELL+tmp%10;
			} else if (tmp>=50) { //spellbooks
				tmp=ACT_BARD+tmp%10;
			} else {
				tmp=iwd2gemrb[tmp];
			}
		}
		ar[i]=tmp;
	}
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
		memset( SmallPortrait, 0, 8 );
		strncpy( SmallPortrait, ResRef, 8 );
	}
	if(Which!=2) {
		memset( LargePortrait, 0, 8 );
		strncpy( LargePortrait, ResRef, 8 );
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

		//TODO: this could be simpler with *
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

void Actor::GetSoundFolder(char *soundset, int full) const
{
	if (core->HasFeature(GF_SOUNDFOLDERS)) {
		strnlwrcpy(soundset, PCStats->SoundFolder, 32);
		if (full) {
			strcat(soundset,"/");
			strncat(soundset, PCStats->SoundSet, 8);
		}
	}
	else {
		strnlwrcpy(soundset, PCStats->SoundSet, 8);
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

	if (index>31) return NULL;

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
	if (vvc->ZPos<0) {
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
	// this is stored in reversed order (negative malus, positive bonus)
	int fatigueBonus = - core->GetConstitutionBonus(STAT_CON_FATIGUE, Modified[IE_CON]);
	if (hours < 8) {
		//do remove effects
		int remaining = hours*10;
		//removes hours*10 fatigue points
		if (remaining < (signed)Modified[IE_FATIGUE]) {
			NewStat (IE_FATIGUE, -remaining, MOD_ADDITIVE);
		} else {
			NewStat (IE_FATIGUE, fatigueBonus, MOD_ABSOLUTE);
		}
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
		SetBase (IE_FATIGUE, fatigueBonus);
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
			Equipped = IW_NO_EQUIPPED;
			EquippedHeader = 0;
			inventory.SetEquippedSlot(Equipped, EquippedHeader);
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
	Equipped = (ieWordSigned) slot;
	EquippedHeader = (ieWord) header;
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

	ieResRef tmpresref;
	strnuprcpy(tmpresref, item->ItemResRef, sizeof(ieResRef)-1);

	Item *itm = gamedata->GetItem(tmpresref);
	if (!itm) return false; //quick item slot contains invalid item resref
	//item is depleted for today
	if(itm->UseCharge(item->Usages, header, false)==CHG_DAY) {
		return false;
	}

	Projectile *pro = itm->GetProjectile(this, header, target, slot, flags&UI_MISS);
	ChargeItem(slot, header, item, itm, flags&UI_SILENT);
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
	//ToBEx compatibility in the ALWAYSBACKSTAB field:
	//0 Normal conditions (attacker must be invisible, attacker must be in 90-degree arc behind victim)
	//1 Ignore invisible requirement and positioning requirement
	//2 Ignore invisible requirement only
	//4 Ignore positioning requirement only
	int multiplier=BaseStats[IE_BACKSTABDAMAGEMULTIPLIER];
	if (multiplier>1) {
		ieDword always = Modified[IE_ALWAYSBACKSTAB];
		if ((Modified[IE_STATE_ID] & state_invisible) || (always&0x3) ) {
			if ( !(core->HasFeature(GF_PROPER_BACKSTAB) && !IsBehind(target)) || (always&0x5) ) {
				if (target->Modified[IE_DISABLEBACKSTAB]) {
					// The backstab seems to have failed
					displaymsg->DisplayConstantString (STR_BACKSTAB_FAIL, DMC_WHITE);
					wi.backstabbing = false;
				} else {
					if (wi.backstabbing) {
						damage *= multiplier;
						// display a simple message instead of hardcoding multiplier names
						displaymsg->DisplayConstantStringValue (STR_BACKSTAB, DMC_WHITE, multiplier);
					} else {
						// weapon is unsuitable for backstab
						displaymsg->DisplayConstantString (STR_BACKSTAB_BAD, DMC_WHITE);
						wi.backstabbing = false;
					}
				}
			}
		}
	}
	damage += WeaponDamageBonus(wi);

	if (target->fxqueue.WeaponImmunity(wi.enchantment, wi.itemflags) ) {
		//'my weapon has no effect'
		damage = 0;
		critical = false;
		return;
	}

	//special effects on hit for arterial strike and hamstring
	if (damage>0 && wi.backstabbing && BackstabResRef[0]!='*') {
		core->ApplySpell(BackstabResRef, target, this, multiplier);
		//do we need this?
		BackstabResRef[0]='*';
		if (HasFeat(FEAT_CRIPPLING_STRIKE) ) {
			core->ApplySpell(resref_cripstr, target, this, multiplier);
		}
	}

	//critical protection a la PST
	if (pstflags && (target->Modified[IE_STATE_ID] & (ieDword) STATE_CRIT_PROT )) {
		critical = false;
	}

	if (critical) {
		if (target->inventory.ProvidesCriticalAversion()) {
			//critical hit is averted by helmet
			displaymsg->DisplayConstantStringName(STR_NO_CRITICAL, DMC_WHITE, target);
			critical = false;
		} else {
			//a critical surely raises the morale?
			//only if it is successful it raises the morale of the attacker
			VerbalConstant(VB_CRITHIT, 1);
			NewBase(IE_MORALE, 1, MOD_ADDITIVE);
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

	Actor *tar = (Actor *) target;
	CREItem *item = inventory.GetSlotItem(slot);
	if (!item)
		return false;

	ieResRef tmpresref;
	strnuprcpy(tmpresref, item->ItemResRef, sizeof(ieResRef)-1);

	Item *itm = gamedata->GetItem(tmpresref);
	if (!itm) return false; //quick item slot contains invalid item resref
	//item is depleted for today
	if (itm->UseCharge(item->Usages, header, false)==CHG_DAY) {
		return false;
	}

	Projectile *pro = itm->GetProjectile(this, header, target->Pos, slot, flags&UI_MISS);
	ChargeItem(slot, header, item, itm, flags&UI_SILENT);
	gamedata->FreeItem(itm,tmpresref, false);
	ResetCommentTime();
	if (pro) {
		//ieDword is unsigned!!
		pro->SetCaster(GetGlobalID(), ITEM_CASTERLEVEL);
		if(((int)header < 0) && !(flags&UI_MISS)) { //using a weapon
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
			if (ranged)
				fxqueue.AddWeaponEffects(pro->GetEffects(), fx_ranged_ref);
			else
				fxqueue.AddWeaponEffects(pro->GetEffects(), fx_melee_ref);
			//AddEffect created a copy, the original needs to be scrapped
			delete AttackEffect;
			attackProjectile = pro;
		} else //launch it now as we are not attacking
			GetCurrentArea()->AddProjectile(pro, Pos, tar->GetGlobalID(), false);
		return true;
	}
	return false;
}

void Actor::ChargeItem(ieDword slot, ieDword header, CREItem *item, Item *itm, bool silent)
{
	if (!itm) {
		item = inventory.GetSlotItem(slot);
		if (!item)
			return;
		itm = gamedata->GetItem(item->ItemResRef);
	}
	if (!itm) return; //quick item slot contains invalid item resref

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

	switch(itm->UseCharge(item->Usages, header, true)) {
		case CHG_DAY:
			break;
		case CHG_BREAK: //both
			if (!silent) {
				core->PlaySound(DS_ITEM_GONE);
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
		case BM_SET: case BM_OR:
			BaseStats[IE_FEATS1+idx]|=mask;
			break;
		case BM_NAND:
			BaseStats[IE_FEATS1+idx]&=~mask;
			break;
		case BM_XOR:
			BaseStats[IE_FEATS1+idx]^=mask;
			break;
	}
}

void Actor::SetFeatValue(unsigned int feat, int value)
{
	if (feat>=MAX_FEATS) {
		return;
	}

	//handle maximum and minimum values
	if (value<0) value = 0;
	else if (value>featmax[feat]) value = featmax[feat];

	if (value) {
		SetFeat(feat, BM_OR);
		if (featstats[feat]) SetBase(featstats[feat], value);
	} else {
		SetFeat(feat, BM_NAND);
		if (featstats[feat]) SetBase(featstats[feat], 0);
	}
	ApplyFeats();
}

void Actor::SetUsedWeapon(const char* AnimationType, ieWord* MeleeAnimation, int wt)
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

	if(header && (header->AttackType == ITEM_AT_BOW)) {
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

void Actor::SetUsedShield(const char* AnimationType, int wt)
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

void Actor::SetUsedHelmet(const char* AnimationType)
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
void Actor::SetupFistData()
{
	if (FistRows<0) {
		FistRows=0;
		AutoTable fist("fistweap");
		if (fist) {
			//default value
			strnlwrcpy( DefaultFist, fist->QueryField( (unsigned int) -1), 8);
			FistRows = fist->GetRowCount();
			fistres = new FistResType[FistRows];
			fistresclass = new int[FistRows];
			for (int i=0;i<FistRows;i++) {
				int maxcol = fist->GetColumnCount(i)-1;
				for (int cols = 0;cols<MAX_LEVEL;cols++) {
					strnlwrcpy( fistres[i][cols], fist->QueryField( i, cols>maxcol?maxcol:cols ), 8);
				}
				fistresclass[i] = atoi(fist->GetRowName(i));
			}
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
	inventory.SetSlotItemRes(ItemResRef, slot);
}

static ieDword ResolveTableValue(const char *resref, ieDword stat, ieDword mcol, ieDword vcol) {
	long ret = 0;
	//don't close this table, it can mess with the guiscripts
	int table = gamedata->LoadTable(resref);
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

int Actor::CheckUsability(Item *item) const
{
	ieDword itembits[2]={item->UsabilityBitmask, item->KitUsability};

	for (int i=0;i<usecount;i++) {
		ieDword itemvalue = itembits[itemuse[i].which];
		ieDword stat = GetStat(itemuse[i].stat);
		ieDword mcol = itemuse[i].mcol;
		//if we have a kit, we just we use it's index for the lookup
		if (itemuse[i].stat==IE_KIT) {
			stat = GetKitIndex(stat, itemuse[i].table);
			mcol = 0xff;
		}
		stat = ResolveTableValue(itemuse[i].table, stat, mcol, itemuse[i].vcol);
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
int Actor::Unusable(Item *item) const
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
	if (overlay>=32) return;
	Modified[IE_SANCTUARY]|=1<<overlay;
}

//returns true if spell state is already set or illegal
bool Actor::SetSpellState(unsigned int spellstate)
{
	if (spellstate>=192) return true;
	unsigned int pos = IE_SPLSTATE_ID1+(spellstate>>5);
	unsigned int bit = 1<<(spellstate&31);
	if (Modified[pos]&bit) return true;
	Modified[pos]|=bit;
	return false;
}

//returns true if spell state is already set
bool Actor::HasSpellState(unsigned int spellstate) const
{
	if (spellstate>=192) return false;
	unsigned int pos = IE_SPLSTATE_ID1+(spellstate>>5);
	unsigned int bit = 1<<(spellstate&31);
	if (Modified[pos]&bit) return true;
	return false;
}

//this is a very specific rule that might need an external table later
int Actor::GetAbilityBonus(unsigned int ability) const
{
	return GetStat(ability)/2-5;
}

int Actor::GetSkillStat(unsigned int skill) const
{
	if (skill>=(unsigned int) skillcount) return -1;
	return skillstats[skill];
}

int Actor::GetSkill(unsigned int skill) const
{
	if (skill>=(unsigned int) skillcount) return -1;
	int ret = GetStat(skillstats[skill])+GetAbilityBonus(skillabils[skill]);
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
	if (Modified[IE_FEATS1+(feat>>5)]&(1<<(feat&31)) ) {
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
	if (Modified[pos]&bit) return true;
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
		// HACK: swashbucklers can't backstab
		if (GetKitUsability(BaseStats[IE_KIT])==KIT_SWASHBUCKLER) {
			backstabdamagemultiplier = 1;
		} else {
			AutoTable tm("backstab");
			//fallback to a general algorithm (bg2 backstab.2da version) if we can't find backstab.2da
			//TODO: AP_SPCL332 (increase backstab by one) seems to not be effecting this at all
			//for assassins perhaps the effect is being called prior to this, and this overwrites it;
			//stalkers work correctly, which is even more odd, considering as they use the same
			//effect and backstabmultiplier would be 0 for them
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

	// monk's level dictated ac and ac vs missiles bonus
	// attacks per round bonus will be handled elsewhere, since it only applies to fist apr
	if (isclass[ISMONK]&(1<<classid)) {
		unsigned int level = GetMonkLevel()-1;
		if (level < monkbon_cols) {
			BaseStats[IE_ARMORCLASS] = DEFAULTAC - monkbon[1][level];
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

	// barbarian immunity to backstab was hardcoded
	if (GetBarbarianLevel()) {
		BaseStats[IE_DISABLEBACKSTAB] = 1;
	}

	//this is a bit too much hardcoded, backstab ability should be a class ability (like turn undead)
	ieDword backstabdamagemultiplier=GetThiefLevel();
	if (backstabdamagemultiplier) {
		backstabdamagemultiplier += GetMonkLevel();
		backstabdamagemultiplier += GetBardLevel();
		AutoTable tm("backstab");
		if (tm)	{
			ieDword cols = tm->GetColumnCount();
			if (backstabdamagemultiplier >= cols) backstabdamagemultiplier = cols-1;
			backstabdamagemultiplier = atoi(tm->QueryField(0, backstabdamagemultiplier));
		} else {
			backstabdamagemultiplier = (backstabdamagemultiplier+7)/4;
		}
		if (backstabdamagemultiplier>5) backstabdamagemultiplier=5;
	}

	int layonhandsamount = GetPaladinLevel() * GetAbilityBonus(IE_CHR);
	if (layonhandsamount<1) layonhandsamount = 1;

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
	BaseStats[IE_LAYONHANDSAMOUNT]=(ieDword) layonhandsamount;
}

//set up stuff here, like attack number, turn undead level
//and similar derived stats that change with level
void Actor::CreateDerivedStats()
{
	if (core->HasFeature(GF_LEVELSLOT_PER_CLASS)) {
		multiclass = 0;
	} else {
		ieDword cls = BaseStats[IE_CLASS]-1;
		if (cls>=(ieDword) classcount) {
			multiclass = 0;
		} else {
			multiclass = multi[cls];
		}
	}

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
	return (Modified[IE_MC_FLAGS] & MC_WAS_ANY) > 0;
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
	newActor->SetMCFlag(MC_EXPORTABLE, BM_NAND);

	//the creature importer does this too
	memcpy(newActor->Modified,newActor->BaseStats, sizeof(Modified) );

	//copy the inventory, but only if it is not the Mislead illusion
	if (mislead) {
		//these need to be called too to have a valid inventory
		newActor->inventory.SetSlotCount(inventory.GetSlotCount());
	} else {
		newActor->inventory.CopyFrom(this);
		newActor->Equipped = Equipped;
		newActor->EquippedHeader = EquippedHeader;
		if (PCStats) {
			newActor->CreateStats();
			memcpy(newActor->PCStats, PCStats, sizeof(PCStatsStruct));
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

	if (classid>BGCLASSCNT) {
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
	if (classid>=(ieDword)classcount || !levelslots[classid])
		return 0;

	//handle barbarians specially, since they're kits and not in levelslots
	if ( (isclass == ISBARBARIAN) && levelslots[classid][ISFIGHTER] && (GetKitUsability(BaseStats[IE_KIT]) == KIT_BARBARIAN) ) {
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
	if (tmpclass>=(ieDword)classcount) return false;
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
	return Modified[IE_DONOTJUMP] < 2;
}

//return true if the actor doesn't want to use an entrance
bool Actor::CannotPassEntrance(ieDword exitID) const
{
	if (LastExit!=exitID) {
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
		UsedExit = LastExit;
		memcpy(LastArea, Area, 8);
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
		return add + luck;
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
bool Actor::IsRacialEnemy(Actor* target) const
{
	if (Modified[IE_HATEDRACE] == target->Modified[IE_RACE]) {
		return true;
	} else if (third) {
		// iwd2 supports multiple racial enemies gained through level progression
		for (unsigned int i=0; i<7; i++) {
			if (Modified[IE_HATEDRACE2+i] == target->Modified[IE_RACE]) {
				return true;
			}
		}
	}
	return false;
}

bool Actor::ModalSpellSkillCheck() {
	switch(ModalState) {
	case MS_BATTLESONG:
		if (isclass[ISBARD]&(1<<Modified[IE_CLASS])) {
			return true;
		}
		/* do we need this */
		if (Modified[IE_STATE_ID]& STATE_SILENCED) {
			return true;
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

inline void HideFailed(Actor* actor)
{
	Effect *newfx;
	newfx = EffectQueue::CreateEffect(fx_disable_button_ref, 0, ACT_STEALTH, FX_DURATION_INSTANT_LIMITED);
	newfx->Duration = core->Time.round_sec; // 90 ticks, 1 round
	core->ApplyEffect(newfx, actor, actor);
	delete newfx;
}

bool Actor::TryToHide() {
	ieDword roll = LuckyRoll(1, 100, GetArmorFailure());
	if (roll == 1) {
		HideFailed(this);
		return false;
	}

	// check for disabled dualclassed thieves (not sure if we need it)

	if (Modified[IE_DISABLEDBUTTON] & (1<<ACT_STEALTH)) {
		HideFailed(this);
		return false;
	}

	// check if the actor is seen by enemy
	Actor** visActors = GetCurrentArea()->GetAllActorsInRadius(Pos, GA_NO_DEAD, Modified[IE_VISUALRANGE]);
	Actor** poi = visActors;
	bool seen = false;
	while (*poi && !seen) {
		Actor *toCheck = *poi++;
		if (Modified[IE_EA] >= EA_EVILCUTOFF)
			seen = toCheck->Modified[IE_EA] < EA_EVILCUTOFF;
		else
			seen = toCheck->Modified[IE_EA] > EA_GOODCUTOFF;
	}
	free(visActors);

	if (seen) {
		HideFailed(this);
		return false;
	}

	ieDword skill;
	if (core->HasFeature(GF_HAS_HIDE_IN_SHADOWS)) {
		skill = (GetStat(IE_HIDEINSHADOWS) + GetStat(IE_STEALTH))/2;
	} else {
		skill = GetStat(IE_STEALTH);
	}

	Game *game = core->GetGame();
	// check how bright our spot is
	ieDword lightness = game->GetCurrentArea()->GetLightLevel(Pos);
	// seems to be the color overlay at midnight; lightness of a point with rgb (200, 100, 100)
	// TODO: but our NightTint computes to a higher value, which one is bad?
	ieDword light_diff = int((lightness - ref_lightness) * 100 / (100 - ref_lightness)) / 2;
	ieDword chance = (100 - light_diff) * skill/100;

	if (roll > chance) {
		HideFailed(this);
		return false;
	}
	return true;
}

//cannot target actor (used by GUI)
bool Actor::Untargetable()
{
	return (GetSafeStat(IE_STATE_ID)&state_invisible) || HasSpellState(SS_SANCTUARY);
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
	if (!caster->CanCast(spellres)) return true;

	if (!range) return false;

	Spell *spl = gamedata->GetSpell(spellres);
	int srange = spl->GetCastingDistance(caster);
	gamedata->FreeSpell(spl, spellres, false);

	return srange<range;
}

bool Actor::PCInDark() const
{
	if (!this) return false;
	unsigned int level = area->GetLightLevel(Pos);
	if (level<ref_lightness) {
		return true;
	}
	return false;
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
	int classmask = 0;
	for (int i=0; i < ISCLASSES; i++) {
		if (Modified[levelslotsiwd2[i]] > 0) {
			classmask |= 1<<(booksiwd2[i]-1);
		}
	}

	return classmask;
}

// returns the combined dexterity and racial bonus to specified thieving skill
// column indices are 1-based, since 0 holds the rowname against which we do the lookup
int Actor::GetSkillBonus(unsigned int col) const
{
	if (skilldex.empty()) return 0;

	// race
	int lookup = Modified[IE_RACE];
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
	} else {
		nextBored = 0;
	}
	nextComment = game->GameTime + core->Roll(5, 1000, bored_time/2);
}

// Returns the armor check penalty.
// used for mapping the iwd2 armor feat to the equipped armor's weight class
// the armor weight class is roughly deduced from the penalty as following:
// 0,   none: none, robes
// 1-3, light: leather, studded
// 4-6, medium: hide, chain, scale
// 7-,  heavy: splint, plate, full plate
// the values are taken from our dehardcoded itemdata.2da
// FIXME: the penalites are too high, the items use a different value!
int Actor::GetArmorFailure() const
{
	if (!third) return 0;

	ieWord armorType = inventory.GetArmorItemType();
	int penalty = core->GetArmorFailure(armorType);
	int weightClass = 0;

	if (penalty >= 1 || penalty < 4) {
		weightClass = 1;
	} else if (penalty >= 4 || penalty < 7) {
		weightClass = 2;
	} else if (penalty >= 7) {
		weightClass = 3;
	}

	// ignore the penalty if we are proficient
	if (GetFeat(FEAT_ARMOUR_PROFICIENCY) >= weightClass) {
		penalty = 0;
	}

	// check also the shield penalty
	armorType = inventory.GetShieldItemType();
	if (!HasFeat(FEAT_SHIELD_PROF)) {
		penalty += core->GetShieldPenalty(armorType);
	}

	return -penalty;
}

// checks whether the actor is visible to another scriptable
// if flags is 1, it skips the EA check for STATE_INVISIBLE
bool Actor::IsInvisibleTo(Scriptable *checker) const
{
	if (Modified[IE_AVATARREMOVAL]) return true;

	bool seer = false;
	if (checker && checker->Type == ST_ACTOR) {
		seer = ((Actor *) checker)->GetSafeStat(IE_SEEINVISIBLE);
	}
	if (!seer && (Modified[IE_STATE_ID] & state_invisible)) {
		return true;
	}

	return false;
}

}
