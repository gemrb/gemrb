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
 * $Id$
 *
 */

#include "../../includes/win32def.h"
#include <cassert>
#include "TableMgr.h"
#include "ResourceMgr.h"
#include "Audio.h" //pst (react to death sounds)
#include "Actor.h"
#include "Interface.h"
#include "../../includes/strrefs.h"
#include "../../includes/overlays.h"
#include "Item.h"
#include "Spell.h"
#include "Projectile.h"
#include "Game.h"
#include "GameScript.h"
#include "GameControl.h" //checking for dialog
#include "ScriptEngine.h"
#include "GSUtils.h" //needed for DisplayStringCore
#include "Video.h"
#include "damages.h"
#include "ProjectileServer.h"

extern Interface* core;
#ifdef WIN32
extern HANDLE hConsole;
#endif

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

static int sharexp = SX_DIVIDE;
static int classcount = -1;
static char **clericspelltables = NULL;
static char **druidspelltables = NULL;
static char **wizardspelltables = NULL;
static int *turnlevels = NULL;
static int *xpbonus = NULL;
static int xpbonustypes = -1;
static int xpbonuslevels = -1;
static int **levelslots = NULL;
static int *dualswap = NULL;
static int *maxhpconbon = NULL;
static ieVariable IWDDeathVarFormat = "KILL_%s_CNT";
static ieVariable DeathVarFormat = "SPRITE_IS_DEAD%s";
static ieVariable CounterNames[4]={"GOOD","LAW","LADY","MURDER"};

static int FistRows = -1;
typedef ieResRef FistResType[MAX_LEVEL+1];

static FistResType *fistres = NULL;
static ieResRef DefaultFist = {"FIST"};

//item usability array
struct ItemUseType {
	ieResRef table; //which table contains the stat usability flags
	ieByte stat;	//which actor stat we talk about
	ieByte mcol;	//which column should be matched against the stat
	ieByte vcol;	//which column has the bit value for it
	ieByte which;	//which item dword should be used (1 = kit)
};

static ItemUseType *itemuse = NULL;
static int usecount = -1;

//item animation override array
struct ItemAnimType {
	ieResRef itemname;
	ieByte animation;
};

static ItemAnimType *itemanim = NULL;
static int animcount = -1;

static int fiststat = IE_CLASS;

//conversion for 3rd ed
static int isclass[11]={0,0,0,0,0,0,0,0,0,0,0};

static const int mcwasflags[11] = {
	MC_WAS_FIGHTER, MC_WAS_MAGE, MC_WAS_THIEF, 0, 0, MC_WAS_CLERIC,
	MC_WAS_DRUID, 0, 0, MC_WAS_RANGER, 0};
static const char *isclassnames[11] = {
	"FIGHTER", "MAGE", "THIEF", "BARBARIAN", "BARD", "CLERIC",
	"DRUID", "MONK", "PALADIN", "RANGER", "SORCERER" };
static const int levelslotsiwd2[11]={IE_LEVELFIGHTER,IE_LEVELMAGE,IE_LEVELTHIEF,
	IE_LEVELBARBARIAN,IE_LEVELBARD,IE_LEVELCLERIC,IE_LEVELDRUID,IE_LEVELMONK,
	IE_LEVELPALADIN,IE_LEVELRANGER,IE_LEVELSORCEROR};

//stat values are 0-255, so a byte is enough
static ieByte featstats[MAX_FEATS]={0
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

static ActionButtonRow *GUIBTDefaults = NULL; //qslots row count
ActionButtonRow DefaultButtons = {ACT_TALK, ACT_WEAPON1, ACT_WEAPON2,
 ACT_NONE, ACT_NONE, ACT_NONE, ACT_NONE, ACT_NONE, ACT_NONE, ACT_NONE,
 ACT_NONE, ACT_INNATE};
static int QslotTranslation = FALSE;
static int DeathOnZeroStat = TRUE;
static ieDword TranslucentShadows = 0;
static int ProjectileSize = 0;  //the size of the projectile immunity bitfield (dwords)

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

static int *mxsplwis = NULL;
static int spllevels;

//for every game except IWD2 we need to reverse TOHIT
static int ReverseToHit=TRUE;
static int CheckAbilities=FALSE;

//internal flags for calculating to hit
#define WEAPON_FIST	0
#define WEAPON_MELEE       1
#define WEAPON_RANGED      2
#define WEAPON_STYLEMASK   15
#define WEAPON_LEFTHAND    16
#define WEAPON_USESTRENGTH 32

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

	LastProtected = 0;
	LastFollowed = 0;
	LastCommander = 0;
	LastHelp = 0;
	LastSeen = 0;
	LastMarked = 0;
	LastHeard = 0;
	PCStats = NULL;
	LastCommand = 0; //used by order
	LastShout = 0; //used by heard
	LastDamage = 0;
	LastDamageType = 0;
	HotKey = 0;
	attackcount = 0;
	attacksperround = 0;
	nextattack = 0;
	InTrap = 0;
	PathTries = 0;
	TargetDoor = NULL;
	attackProjectile = NULL;
	lastInit = 0;
	roundTime = 0;
	lastattack = 0;

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
	projectileImmunity = (ieDword *) calloc(ProjectileSize,sizeof(ieDword));
	TalkCount = 0;
	InteractCount = 0; //numtimesinteracted depends on this
	appearance = 0xffffff; //might be important for created creatures
	version = 0;
	//these are used only in iwd2 so we have to default them
	for(i=0;i<7;i++) {
		BaseStats[IE_HATEDRACE2+i]=0xff;
	}
	//this one is saved only for PC's
	ModalState = 0;
	//set it to a neutral value
	ModalSpell[0] = '*';
	//this one is saved, but not loaded?
	localID = globalID = 0;
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

void Actor::SetText(const char* ptr, unsigned char type)
{
	size_t len = strlen( ptr ) + 1;
	//32 is the maximum possible length of the actor name in the original games
	if (len>32) len=33;
	if (type!=2) {
		LongName = ( char * ) realloc( LongName, len );
		memcpy( LongName, ptr, len );
	}
	if (type!=1) {
		ShortName = ( char * ) realloc( ShortName, len );
		memcpy( ShortName, ptr, len );
	}
}

void Actor::SetText(int strref, unsigned char type)
{
	if (type!=2) {
		if (LongName) free(LongName);
		LongName = core->GetString( strref );
	}
	if (type!=1) {
		if (ShortName) free(ShortName);
		ShortName = core->GetString( strref );
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
				printMessage("Actor"," ",YELLOW);
				printf("Animation ID %x is supposed to be real colored (no recoloring), patched creature\n", AnimID);
			}
			BaseStats[IE_COLORCOUNT]=0;
		}
	}
	anims = new CharAnimations( AnimID&0xffff, BaseStats[IE_ARMOR_TYPE]);
	if(anims->ResRef[0] == 0) {
		delete anims;
		anims = NULL;
		printMessage("Actor", " ",LIGHT_RED);
		printf("Missing animation for %s\n",LongName);
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
}

CharAnimations* Actor::GetAnims()
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

void Actor::SetCircleSize()
{
	const Color *color;
	int color_index;

	if (!anims)
		return;

	if (UnselectableTimer) {
		color = &magenta;
		color_index = 4;
	} else if (Modified[IE_STATE_ID] & STATE_PANIC) {
		color = &yellow;
		color_index = 5;
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

void ApplyClab(Actor *actor, const char *clab, int level)
{
	AutoTable table(clab);
	if (table) {
		int row = table->GetRowCount();
		for(int i=0;i<level;i++) {
			for (int j=0;j<row;j++) {
				const char *res = table->QueryField(j,i);
				if (!memcmp(res,"AP_",3)) {
					core->ApplySpell(res+2, actor, actor, 0);
				}
				else if (!memcmp(res,"GA_",3)) {
					actor->LearnSpell(res+2, 0);
				}
				else if (!memcmp(res,"FA_",3)) {//iwd2 only
					int x=atoi(res+3);
					core->DisplayStringName(x,0xffffff,actor,0);
				}
				else if (!memcmp(res,"FS_",3)) {//iwd2? (song?)
					int x=atoi(res+3);
					core->DisplayStringName(x,0xffffff,actor,0);
				}
				else if (!memcmp(res,"RA_",3)) {//iwd2
					int x=atoi(res+3);
					core->DisplayStringName(x,0xffffff,actor,0);
				}
			}
		}
	}
}

#define BG2_KITMASK  0xffffc000
#define KIT_BARBARIAN 0x4000
#define KIT_BASECLASS 0x40000000

//applies a kit on the character (only bg2)
bool Actor::ApplyKit(ieDword Value)
{
	//get current unmodified level (i guess)
	int level = GetXPLevel(false);
	AutoTable table("kitlist");
	if (table) {
		ieDword row;
		//find row by unusability
		row = table->GetRowCount();
		while (row) {
			row--;
			ieDword Unusability = (ieDword) strtol(table->QueryField(row, 6),NULL,0);
			if (Value == Unusability) {
				goto found_row;
			}
		}
		//if it wasn't found, try the bg2 kit format
		if ((Value&BG2_KITMASK)==KIT_BARBARIAN) {
			row = (Value<<16);
		}
		//cannot find kit
		if (table->GetRowCount()>=row) {
			return false;
		}
found_row:
		ieDword cls = (ieDword) atoi(table->QueryField(row, 7));
		if (cls!=BaseStats[IE_CLASS]) {
			//cannot apply kit, because the class doesn't fit
			return false;
		}
		const char *clab = table->QueryField(row, 4);
		ApplyClab(this, clab, level);
	}
	return true;
}

//call this when morale or moralebreak changed
//cannot use old or new value, because it is called two ways
void pcf_morale (Actor *actor, ieDword /*oldValue*/, ieDword /*newValue*/)
{
	if ((actor->Modified[IE_MORALE]<=actor->Modified[IE_MORALEBREAK]) && (actor->Modified[IE_MORALEBREAK] != 0) ) {
		actor->Panic();
	}
	//for new colour
	actor->SetCircleSize();
}

void pcf_ea (Actor *actor, ieDword /*oldValue*/, ieDword /*newValue*/)
{
	actor->SetCircleSize();
}

//this is a good place to recalculate level up stuff
void pcf_level (Actor *actor, ieDword /*oldValue*/, ieDword /*newValue*/)
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
}

void pcf_class (Actor *actor, ieDword /*oldValue*/, ieDword /*newValue*/)
{
	actor->InitButtons(actor->Modified[IE_CLASS], false);
}

void pcf_animid(Actor *actor, ieDword /*oldValue*/, ieDword newValue)
{
	actor->SetAnimationID(newValue);
}

static const ieDword fullwhite[7]={ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT};

static const ieDword fullstone[7]={STONE_GRADIENT,STONE_GRADIENT,STONE_GRADIENT,STONE_GRADIENT,STONE_GRADIENT,STONE_GRADIENT,STONE_GRADIENT};

void pcf_state(Actor *actor, ieDword /*oldValue*/, ieDword State)
{
	if (State & STATE_PETRIFIED) {
		actor->SetLockedPalette(fullstone);
		return;
	}
	if (State & STATE_FROZEN) {
		actor->SetLockedPalette(fullwhite);
		return;
	}
	if (actor->InParty) core->SetEventFlag(EF_PORTRAIT);
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
	if ((signed) actor->BaseStats[IE_HITPOINTS]>(signed) actor->Modified[IE_MAXHITPOINTS]) {
		actor->BaseStats[IE_HITPOINTS]=actor->Modified[IE_MAXHITPOINTS];
	}

	if ((signed) hp>(signed) actor->Modified[IE_MAXHITPOINTS]) {
		hp=actor->Modified[IE_MAXHITPOINTS];
	}
	if ((signed) hp<(signed) actor->Modified[IE_MINHITPOINTS]) {
		hp=actor->Modified[IE_MINHITPOINTS];
	}
	if ((signed) hp<=0) {
		actor->Die(NULL);
	}
	actor->Modified[IE_HITPOINTS]=hp;
	if (actor->InParty) core->SetEventFlag(EF_PORTRAIT);
}

void pcf_maxhitpoint(Actor *actor, ieDword /*oldValue*/, ieDword hp)
{
	if ((signed) hp<(signed) actor->Modified[IE_HITPOINTS]) {
		actor->Modified[IE_HITPOINTS]=hp;
		//passing 0 because it is ignored anyway
		pcf_hitpoint(actor, 0, hp);
	}
}

void pcf_minhitpoint(Actor *actor, ieDword /*oldValue*/, ieDword hp)
{
	if ((signed) hp>(signed) actor->Modified[IE_HITPOINTS]) {
		actor->Modified[IE_HITPOINTS]=hp;
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
	pcf_hitpoint(actor, 0, actor->Modified[IE_HITPOINTS]);
}

void pcf_stat_cha(Actor *actor, ieDword /*oldValue*/, ieDword newValue)
{
	pcf_stat(actor, newValue, IE_CHR);
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
	if (sca) {
		if (flag) {
			sca->ZPos=-1;
		}
		actor->AddVVCell(sca);
	}
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
	if (newValue&1) {
		handle_overlay(actor, OV_BOUNCE);
		return;
	}
	if (oldValue&1) {
		//it seems we have to remove it abruptly
		actor->RemoveVVCell(hc_overlays[OV_BOUNCE], false);
	}
}

//no separate values (changes are permanent)
void pcf_fatigue(Actor *actor, ieDword /*oldValue*/, ieDword newValue)
{
	actor->BaseStats[IE_FATIGUE]=newValue;
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
100,100,100,100,100,100,100,100,100,200,200,200,200,200,100,100,//1f
200,200,MAX_LEVEL,255,25,100,25,25,25,25,25,999999999,999999999,999999999,25,25,//2f
200,255,200,100,100,200,200,25,5,100,1,1,100,1,1,0,//3f
511,1,1,1,MAX_LEVEL,MAX_LEVEL,1,9999,25,100,100,255,1,20,20,25,//4f
25,1,1,255,25,25,255,255,25,255,255,255,255,255,255,255,//5f
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,//6f
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,//7f
255,255,255,255,255,255,255,100,100,100,255,5,5,255,1,1,//8f
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
pcf_stat_dex,pcf_stat_con,pcf_stat_cha,NULL, NULL, pcf_gold, pcf_morale, NULL, //2f
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
		if (turnlevels) {
			free(turnlevels);
			turnlevels=NULL;
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
		if (maxhpconbon) {
			free(maxhpconbon);
			maxhpconbon=NULL;
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
	}
	if (GUIBTDefaults) {
		free (GUIBTDefaults);
		GUIBTDefaults=NULL;
	}
	classcount = -1;
}

#define COL_HATERACE      0   //ranger type racial enemy
#define COL_CLERIC_SPELL  1   //cleric spells
#define COL_MAGE_SPELL    2   //mage spells
#define COL_STARTXP       3   //starting xp
#define COL_BARD_SKILL    4   //bard skills
#define COL_THIEF_SKILL   5   //thief skills

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

static void InitActorTables()
{
	int i, j;

	if (core->HasFeature(GF_IWD_DEATHVARFORMAT)) {
		memcpy(DeathVarFormat, IWDDeathVarFormat, sizeof(ieVariable));
	}

	if (core->HasFeature(GF_CHALLENGERATING)) {
		sharexp=SX_DIVIDE|SX_CR;
	} else {
		sharexp=SX_DIVIDE;
	}
	ReverseToHit = core->HasFeature(GF_REVERSE_TOHIT);
	CheckAbilities = core->HasFeature(GF_CHECK_ABILITIES);
	DeathOnZeroStat = core->HasFeature(GF_DEATH_ON_ZERO_STAT);

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

		ieDword bitmask = 1;

		for(i = 0; i<classcount; i++) {
			const char *field;
			int turnlevel = atoi(tm->QueryField( i, 7));
			turnlevels[i]=turnlevel;

			field = tm->QueryField( i, 0 );
			if (field[0]!='*') {
				isclass[ISDRUID] |= bitmask;
				druidspelltables[i]=strdup(field);
			}
			field = tm->QueryField( i, 1 );
			if (field[0]!='*') {
				isclass[ISCLERIC] |= bitmask;
				clericspelltables[i]=strdup(field);
			}

			field = tm->QueryField( i, 2 );
			if (field[0]!='*') {
				isclass[ISMAGE] |= bitmask;
				wizardspelltables[i]=strdup(field);
			}

			// field 3 holds the starting xp

			field = tm->QueryField( i, 4 );
			if (field[0]!='*') {
				isclass[ISBARD] |= bitmask;
			}

			field = tm->QueryField( i, 5 );
			if (field[0]!='*') {
				isclass[ISTHIEF] |= bitmask;
			}

			field = tm->QueryField( i, 6 );
			if (field[0]!='*') {
				isclass[ISPALADIN] |= bitmask;
			}

			// field 7 holds the turn undead level

			field = tm->QueryField( i, 8 );
			if (atoi(field)==2) {
				isclass[ISSORCERER] |= bitmask;
			}

			field = tm->QueryField( i, 9 );
			if (field[0]!='*') {
				isclass[ISRANGER] |= bitmask;
			}

			field = tm->QueryField( i, 10 );
			if (!strnicmp(field, "CLABMO", 6)) {
				isclass[ISMONK] |= bitmask;
			}
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
				if (tmp[0]!='*') {
					csound[i]=tmp[0];
				}
			}
		}
	}

	tm.load("qslots");
	GUIBTDefaults = (ActionButtonRow *) calloc( classcount,sizeof(ActionButtonRow) );

	for (i = 0; i < classcount; i++) {
		memcpy(GUIBTDefaults+i, &DefaultButtons, sizeof(ActionButtonRow));
		if (tm) {
			for (int j=0;j<MAX_QSLOTS;j++) {
				GUIBTDefaults[i][j+3]=(ieByte) atoi( tm->QueryField(i,j) );
			}
		}
	}

	tm.load("itemuse");
	if (tm) {
		usecount = tm->GetRowCount();
		itemuse = new ItemUseType[usecount];
		for (i = 0; i < usecount; i++) {
			itemuse[i].stat = (ieByte) atoi( tm->QueryField(i,0) );
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
		unsigned int tmp;

		for(i=0;i<MAX_FEATS;i++) {
			//we need the MULTIPLE column only
			//it stores the FEAT_* stat index, and could be taken multiple
			//times
			tmp = core->TranslateStat(tm->QueryField(i,0));
			if (tmp>=MAX_STATS) {
				printMessage("Actor","Invalid stat value in featreq.2da",YELLOW);
			}
			featstats[i] = (ieByte) tmp;
		}
	}

	//default all hp con bonuses to 9; this should be updated below
	//TODO: check iwd2
	maxhpconbon = (int *) calloc(classcount, sizeof(int));
	for (i = 0; i < classcount; i++) {
		maxhpconbon[i] = 9;
	}
	tm.load("classes");
	if (tm && !core->HasFeature(GF_IWD2_SCRIPTNAME)) {
		AutoTable hptm;
		//iwd2 just uses levelslotsiwd2 instead
		printf("Examining classes.2da\n");

		//when searching the levelslots, you must search for
		//levelslots[BaseStats[IE_CLASS]-1] as there is no class id of 0
		levelslots = (int **) calloc(classcount, sizeof(int*));
		dualswap = (int *) calloc(classcount, sizeof(int));
		ieDword tmpindex;
		for (i=0; i<classcount; i++) {
			//make sure we have a valid classid, then decrement
			//it to get the correct array index
			tmpindex = atoi(tm->QueryField(i, 5));
			if (!tmpindex)
				continue;
			tmpindex--;

			printf("\tID: %d ", tmpindex);
			//only create the array if it isn't yet made
			//i.e. barbarians would overwrite fighters in bg2
			if (levelslots[tmpindex]) {
				printf ("Already Found!\n");
				continue;
			}

			const char* classname = tm->GetRowName(i);
			printf("Name: %s ", classname);
			int classis = 0;
			//default all levelslots to 0
			levelslots[tmpindex] = (int *) calloc(ISCLASSES, sizeof(int));

			//single classes only worry about IE_LEVEL
			ieDword tmpclass = atoi(tm->QueryField(i, 4));
			if (!tmpclass) {
				classis = IsClassFromName(classname);
				if (classis>=0) {
					printf("Classis: %d ", classis);
					levelslots[tmpindex][classis] = IE_LEVEL;
					//get the max hp con bonus
					hptm.load(tm->QueryField(i, 6));
					if (hptm) {
						int tmphp = 0;
						int rollscolumn = hptm->GetColumnIndex("ROLLS");
						while (atoi(hptm->QueryField(tmphp, rollscolumn)))
							tmphp++;
						printf("TmpHP: %d ", tmphp);
						if (tmphp) maxhpconbon[tmpindex] = tmphp;
					}
				}
				continue;
			}

			//we have to account for dual-swap in the multiclass field
			ieDword numfound = 1;
			ieDword tmpbits = bitcount (tmpclass);

			//we need all the classnames of the multi to compare with the order we load them in
			//because the original game set the levels based on name order, not bit order
			char **classnames = (char **) calloc(tmpbits, sizeof(char *));
			classnames[0] = strdup((char*)strtok((char*)classname, "_"));
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
						printf("Classis: %d ", classis);

						//warrior take presedence
						if (!foundwarrior) {
							foundwarrior = (classis==ISFIGHTER||classis==ISRANGER||classis==ISPALADIN||
								classis==ISBARBARIAN);
							hptm.load(tm->QueryField(currentname, "HP"));
							if (hptm) {
								int tmphp = 0;
								int rollscolumn = hptm->GetColumnIndex("ROLLS");
								while (atoi(hptm->QueryField(tmphp, rollscolumn)))
									tmphp++;
								//make sure we at least set the first class
								if ((tmphp>maxhpconbon[tmpindex])||foundwarrior||numfound==0)
									maxhpconbon[tmpindex]=tmphp;
							}
						}
					}

					//save the MC_WAS_ID of the first class in the dual-class
					if (numfound==0 && tmpbits==2) {
						if (strcmp(classnames[0], currentname) == 0) {
							dualswap[tmpindex] = strtol(tm->QueryField(classname, "MC_WAS_ID"), NULL, 0);
						}
					} else if (numfound==1 && tmpbits==2 && !dualswap[tmpindex]) {
						dualswap[tmpindex] = strtol(tm->QueryField(classname, "MC_WAS_ID"), NULL, 0);
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
			printf("HPCON: %d ", maxhpconbon[tmpindex]);
			printf("DS: %d\n", dualswap[tmpindex]);
		}
		/*this could be enabled to ensure all levelslots are filled with at least 0's;
		 *however, the access code should ensure this never happens
		for (i=0; i<classcount; i++) {
			if (!levelslots[i]) {
				levelslots[i] = (int *) calloc(ISCLASSES, sizeof(int *));
			}
		}*/
	}
	printf("Finished examining classes.2da\n");

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
				if (tmp<0) tmp  = -2*tmp-1;
				else       tmp *=  2;
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
	if (flags&&AA_BLEND) {
		//pst anims need this?
		sca->SetBlend();
	}
	if (gradient!=-1) {
		sca->SetPalette(gradient, 4);
	}
	AddVVCell(sca);
}

void Actor::PlayDamageAnimation(int type, bool hit)
{
	int i;

	switch(type) {
		case 0: case 1: case 2: case 3: //blood
			i = (int) GetStat(IE_ANIMATION_ID)>>16;
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

	unsigned int previous = Modified[StatIndex];
	if (Modified[StatIndex]!=Value) {
		Modified[StatIndex] = Value;
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
ieDword Actor::GetBase(unsigned int StatIndex)
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

const unsigned char *Actor::GetStateString()
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

/** call this after load, to apply effects */
void Actor::RefreshEffects(EffectQueue *fx)
{
	ieDword previous[MAX_STATS];

	//put all special cleanup calls here
	CharAnimations* anims = GetAnims();
	if (anims) {
		unsigned int location;
		for (location = 0; location < 32; ++location) {
			anims->ColorMods[location].type = RGBModifier::NONE;
			anims->ColorMods[location].speed = 0;
		}
	}
	spellbook.ClearBonus();
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
	memset(projectileImmunity,0,ProjectileSize*sizeof(ieDword));

	//initialize base stats
	bool first = !(InternalFlags&IF_INITIALIZED);

	if (first) {
		InternalFlags|=IF_INITIALIZED;
		memcpy( previous, BaseStats, MAX_STATS * sizeof( ieDword ) );
	} else {
		memcpy( previous, Modified, MAX_STATS * sizeof( ieDword ) );
	}
	memcpy( Modified, BaseStats, MAX_STATS * sizeof( ieDword ) );
	if (PCStats) memset( PCStats->PortraitIcons, -1, sizeof(PCStats->PortraitIcons) );

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

	fxqueue.ApplyAllEffects( this );

	// IE_CLASS is >classcount for non-PCs/NPCs
	if (BaseStats[IE_CLASS] <= (ieDword)classcount)
		RefreshPCStats();

	for (unsigned int i=0;i<MAX_STATS;i++) {
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
	if (PCStats && memcmp(PCStats->PreviousPortraitIcons, PCStats->PortraitIcons, sizeof(PCStats->PreviousPortraitIcons))) {
		core->SetEventFlag(EF_PORTRAIT);
	}
	if (PCStats) {
		memcpy( PCStats->PreviousPortraitIcons, PCStats->PortraitIcons, sizeof(PCStats->PreviousPortraitIcons) );
	}
}

// refresh stats on creatures (PC or NPC) with a valid class (not animals etc)
// internal use only, and this is maybe a stupid name :)
void Actor::RefreshPCStats() {
	//calculate hp bonus
	int bonus;
	int bonlevel = GetXPLevel(true);
	int oldlevel, oldbonus;
	oldlevel = oldbonus = 0;
	ieDword bonindex = BaseStats[IE_CLASS]-1;

	//we must limit the levels to the max allowable
	if (bonlevel>maxhpconbon[bonindex])
		bonlevel = maxhpconbon[bonindex];

	if (IsDualInactive()) {
		//we apply the inactive hp bonus if it's better than the new hp bonus, so that we
		//never lose hp, only gain, on leveling
		oldlevel = IsDualSwap() ? BaseStats[IE_LEVEL] : BaseStats[IE_LEVEL2];
		bonlevel = IsDualSwap() ? BaseStats[IE_LEVEL2] : BaseStats[IE_LEVEL];
		oldlevel = (oldlevel > maxhpconbon[bonindex]) ? maxhpconbon[bonindex] : oldlevel;
		if (Modified[IE_MC_FLAGS] & (MC_WAS_FIGHTER|MC_WAS_RANGER)) {
			oldbonus = core->GetConstitutionBonus(STAT_CON_HP_WARRIOR, Modified[IE_CON]);
		} else {
			oldbonus = core->GetConstitutionBonus(STAT_CON_HP_NORMAL, Modified[IE_CON]);
		}
	}

	// warrior (fighter, barbarian, ranger, or paladin) or not
	// GetClassLevel now takes into consideration inactive dual-classes
	if (IsWarrior()) {
		bonus = core->GetConstitutionBonus(STAT_CON_HP_WARRIOR,Modified[IE_CON]);
	} else {

		bonus = core->GetConstitutionBonus(STAT_CON_HP_NORMAL,Modified[IE_CON]);
	}
	bonus *= bonlevel;
	oldbonus *= oldlevel;
	bonus = (oldbonus > bonus) ? oldbonus : bonus;

	//morale recovery every xth AI cycle
	int mrec = GetStat(IE_MORALERECOVERYTIME);
	if (mrec) {
		if (!(core->GetGame()->GameTime%mrec)) {
			NewBase(IE_MORALE,1,MOD_ADDITIVE);
		}
	}

	if (bonus<0 && (Modified[IE_MAXHITPOINTS]+bonus)<=0) {
		bonus=1-Modified[IE_MAXHITPOINTS];
	}

	//get the wspattack bonuses for proficiencies
	WeaponInfo wi;
	ITMExtHeader *header = GetWeapon(wi, false);
	ieDword stars;
	int dualwielding = IsDualWielding();
	if (header && (wi.prof <= MAX_STATS)) {
		stars = GetStat(wi.prof)&PROFS_MASK;
		if (stars >= (unsigned)wspattack_rows) {
			stars = wspattack_rows-1;
		}

		int tmplevel = GetWarriorLevel();
		if (tmplevel >= wspattack_cols) {
			tmplevel = wspattack_cols-1;
		} else if (tmplevel < 0) {
			tmplevel = 0;
		}

		//HACK: attacks per round bonus for monks should only apply to fists
		if (isclass[ISMONK]&(1<<BaseStats[IE_CLASS])) {
			if (BaseStats[IE_NUMBEROFATTACKS] == 2) {
				AutoTable tm("monkbon");
				if (tm) {
					SetBase(IE_NUMBEROFATTACKS, 2 + atoi(tm->QueryField(0, GetMonkLevel()-1)));
				}
			}
		} else {
			//wspattack appears to only effect warriors
			int defaultattacks = 2 + 2*dualwielding;
			if (tmplevel) {
				SetBase(IE_NUMBEROFATTACKS, defaultattacks+wspattack[stars][tmplevel]);
			} else {
				SetBase(IE_NUMBEROFATTACKS, defaultattacks);
			}
		}
	}

	//we still apply the maximum bonus to dead characters, but don't apply
	//to current HP, or we'd have dead characters showing as having hp
	Modified[IE_MAXHITPOINTS]+=bonus;
	if(BaseStats[IE_STATE_ID]&STATE_DEAD)
		bonus = 0;
	Modified[IE_HITPOINTS]+=bonus;

	// apply the intelligence and wisdom bonus to lore
	Modified[IE_LORE] += core->GetLoreBonus(0, Modified[IE_INT]) + core->GetLoreBonus(0, Modified[IE_WIS]);
}

void Actor::RollSaves()
{
	if (InternalFlags&IF_USEDSAVE) {
		SavingThrow[0]=(ieByte) core->Roll(1,SAVEROLL,0);
		SavingThrow[1]=(ieByte) core->Roll(1,SAVEROLL,0);
		SavingThrow[2]=(ieByte) core->Roll(1,SAVEROLL,0);
		SavingThrow[3]=(ieByte) core->Roll(1,SAVEROLL,0);
		SavingThrow[4]=(ieByte) core->Roll(1,SAVEROLL,0);
		InternalFlags&=~IF_USEDSAVE;
	}
}

/** returns true if actor made the save against saving throw type */
bool Actor::GetSavingThrow(ieDword type, int modifier)
{
	assert(type<5);
	InternalFlags|=IF_USEDSAVE;
	int ret = SavingThrow[type];
	if (ret == 1) return false;
	if (ret == SAVEROLL) return true;
	ret += modifier;
	return ret > (int) GetStat(IE_SAVEVSDEATH+type);
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

	switch(type) {
		case I_INSULT: start=VB_INSULT; count=3; break;
		case I_COMPLIMENT: start=VB_COMPLIMENT; count=3; break;
		case I_SPECIAL: start=VB_SPECIAL; count=3; break;
		default:
			return;
	}

	count=rand()%count;
	while(count && StrRefs[start+count]!=0xffff) count--;
	if(count>=0) {
		DisplayStringCore(this, start+count, DS_CONSOLE|DS_CONST );
	}
}

void Actor::Response(int type)
{
	int start;
	int count;

	switch(type) {
		case I_INSULT: start=VB_RESP_INS; count=3; break;
		case I_COMPLIMENT: start=VB_RESP_COMP; count=3; break;
		default:
			return;
	}

	count=rand()%count;
	while(count && StrRefs[start+count]!=0xffff) count--;
	if(count>=0) {
		DisplayStringCore(this, start+count, DS_CONSOLE|DS_CONST );
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
		DisplayStringCore(this, VB_REACT, DS_CONSOLE|DS_CONST );
		break;
	case '1':
		DisplayStringCore(this, VB_REACT_S, DS_CONSOLE|DS_CONST );
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

			ieDword len = core->GetAudioDrv()->Play( resref );
			ieDword counter = ( AI_UPDATE_TIME * len ) / 1000;
			if (counter != 0)
				SetWait( counter );
			break;
		}
	}
}

//call this only from gui selects
void Actor::SelectActor()
{
	DisplayStringCore(this, VB_SELECT, DS_CONSOLE|DS_CONST );
}

void Actor::Panic()
{
	if (GetStat(IE_STATE_ID)&STATE_PANIC) {
		//already in panic
		return;
	}
	SetBaseBit(IE_STATE_ID, STATE_PANIC, true);
	DisplayStringCore(this, VB_PANIC, DS_CONSOLE|DS_CONST );
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
		DisplayStringCore(this, VB_HOSTILE, DS_CONSOLE|DS_CONST );
	} else {
		DisplayStringCore(this, VB_DIALOG, DS_CONSOLE|DS_CONST );
	}
}

static EffectRef fx_cure_sleep_ref={"Cure:Sleep",NULL,-1};

void Actor::GetHit()
{
	SetStance( IE_ANI_DAMAGE );
	DisplayStringCore(this, VB_DAMAGE, DS_CONSOLE|DS_CONST );
	if (Modified[IE_STATE_ID]&STATE_SLEEP) {
		if (Modified[IE_EXTSTATE_ID]&EXTSTATE_NO_WAKEUP) {
			return;
		}
		Effect *fx = EffectQueue::CreateEffect(fx_cure_sleep_ref, 0, 0, FX_DURATION_INSTANT_PERMANENT);
		fxqueue.AddEffect(fx);
	}
}

bool Actor::HandleCastingStance(const ieResRef SpellResRef, bool deplete)
{
	if (deplete) {
		if (! spellbook.HaveSpell( SpellResRef, HS_DEPLETE )) {
			SetStance(IE_ANI_READY);
			return true;
		}
	}
	SetStance(IE_ANI_CAST);
	return false;
}

//returns actual damage
int Actor::Damage(int damage, int damagetype, Scriptable *hitter, int modtype)
{
	//add lastdamagetype up
	LastDamageType|=damagetype;
	if(hitter && hitter->Type==ST_ACTOR) {
		LastHitter=((Actor *) hitter)->GetID();
	} else {
		//Maybe it should be something impossible like 0xffff, and use 'Someone'
		LastHitter=GetID();
	}
	//note: the lower 2 bits are actually modifier types
	// this is processed elsewhere and sent as modtype
	switch(modtype)
	{
	case MOD_ADDITIVE:
		damage = -NewBase(IE_HITPOINTS, (ieDword) -damage, MOD_ADDITIVE);
		break;
	case MOD_ABSOLUTE:
		damage = -NewBase(IE_HITPOINTS, (ieDword) damage, MOD_ABSOLUTE);
		break;
	case MOD_PERCENT:
		damage = -NewBase(IE_HITPOINTS, (ieDword) damage, MOD_PERCENT);
		break;
	default:
		//this shouldn't happen
		printMessage("Actor","Invalid damagetype!\n",RED);
		return 0;
	}
	LastDamage=damage;
	InternalFlags|=IF_ACTIVE;
	int chp = (signed) Modified[IE_HITPOINTS];
	int damagelevel = 3;
	if (damage<5) {
		damagelevel = 1;
	} else if (damage<10) {
		damagelevel = 2;
	} else {
		NewBase(IE_MORALE, (ieDword) -1, MOD_ADDITIVE);
		if (chp<-10) {
			damagelevel = 0; //chunky death
		}
		else {
			damagelevel = 3;
		}
	}
	GetHit();
	unsigned int damagetype_str_id = 0;
	if (damagetype & (DAMAGE_FIRE|DAMAGE_MAGICFIRE) ) {
		PlayDamageAnimation(DL_FIRE+damagelevel);
		damagetype_str_id = STR_DMG_FIRE;
	} else if (damagetype & (DAMAGE_COLD|DAMAGE_MAGICCOLD) ) {
		PlayDamageAnimation(DL_COLD+damagelevel);
		damagetype_str_id = STR_DMG_COLD;
	} else if (damagetype & (DAMAGE_ELECTRICITY) ) {
		PlayDamageAnimation(DL_ELECTRICITY+damagelevel);
		damagetype_str_id = STR_DMG_ELECTRIC;
	} else if (damagetype & (DAMAGE_ACID) ) {
		PlayDamageAnimation(DL_ACID+damagelevel);
		damagetype_str_id = STR_DMG_ACID;
	} else if (damagetype & (DAMAGE_MAGIC) ) {
		PlayDamageAnimation(DL_DISINTEGRATE+damagelevel);
		damagetype_str_id = STR_DMG_MAGIC;
	} else {
		if (damagetype & (DAMAGE_POISON) ) {
			damagetype_str_id = STR_DMG_POISON;
		} else if (damagetype & (DAMAGE_MISSILE|DAMAGE_PIERCINGMISSILE|DAMAGE_CRUSHINGMISSILE) ) {
			damagetype_str_id = STR_DMG_MISSILE;
		} else if (damagetype & (DAMAGE_SLASHING) ) {
			damagetype_str_id = STR_DMG_SLASHING;
		} else if (damagetype & (DAMAGE_PIERCING) ) {
			damagetype_str_id = STR_DMG_PIERCING;
		} else if (damagetype & (DAMAGE_STUNNING|DAMAGE_SOULEATER|DAMAGE_DISEASE|DAMAGE_CHUNKING) ) {
			damagetype_str_id = STR_DMG_OTHER;
		} else if (damagetype == DAMAGE_CRUSHING) { // DAMAGE_CRUSHING is 0, so we can't compare it as the rest
			damagetype_str_id = STR_DMG_CRUSHING;
		}
		PlayDamageAnimation(damagelevel);
	}
	DisplayCombatFeedback(damage, damagetype_str_id, (Actor *)hitter);

	if (InParty) {
		if (chp<(signed) Modified[IE_MAXHITPOINTS]/10) {
			core->Autopause(AP_WOUNDED);
		}
		if (damage>0) {
			core->Autopause(AP_HIT);
			core->SetEventFlag(EF_PORTRAIT);
		}
	}
	return damage;
}

//TODO: handle pst
//TODO: handle resisted damage
//TODO: handle bonus damage
void Actor::DisplayCombatFeedback (unsigned int damage, unsigned int damagetype_str_id, Actor *hitter)
{
	bool detailed = false;
	const char *type_name = "unknown";
	if (core->GetStringReference(STR_DMG_SLASHING) != (ieStrRef) -1) { // how and iwd2
		if (damagetype_str_id) {
			type_name = core->GetString(core->GetStringReference(damagetype_str_id), 0);
		}
		detailed = true;
	}

	if (damage > 0) {
		printMessage("Actor", " ", GREEN);
		printf("%d damage taken.\n", damage);
		char dmg_str[8];
		snprintf(dmg_str, 8, "%d", damage);

		if (detailed) {
			// 3 choices depending on resistance and boni
			// iwd2 also has two Tortoise Shell (spell) absorption strings
			core->GetTokenDictionary()->SetAtCopy( "TYPE", type_name);
			core->GetTokenDictionary()->SetAtCopy( "AMOUNT", dmg_str);
			core->GetTokenDictionary()->SetAtCopy( "DAMAGER", hitter->GetName(0) );
			/*if (bonus-resistance > 0) {
				//Takes <AMOUNT> <TYPE> damage from <DAMAGER> (<RESISTED> damage bonus)
				core->GetTokenDictionary()->SetAtCopy( "RESISTED", bonus);
				core->DisplayConstantStringName(STR_DAMAGE3, 0xffffff, this);
			} else if (bonus-resistance < 0) {
				//Takes <AMOUNT> <TYPE> damage from <DAMAGER> (<RESISTED> damage resisted)
				core->GetTokenDictionary()->SetAtCopy( "RESISTED", resistance);
				core->DisplayConstantStringName(STR_DAMAGE2, 0xffffff, this);
			} else {*/
				//Takes <AMOUNT> <TYPE> damage from <DAMAGER>
				core->DisplayConstantStringName(STR_DAMAGE1, 0xffffff, this);
			//}
		} else if (stricmp( core->GameType, "pst" ) == 0) {
			if(0) printf("TODO: pst floating text\n");
		} else if (core->GetStringReference(STR_DAMAGE_IMMUNITY) == (ieStrRef) -1 && core->GetStringReference(STR_DAMAGE1) != (ieStrRef) -1) {
			// bg1 and how
			// "Damage Taken"
			core->DisplayConstantStringName(STR_DAMAGE1, 0xffffff, this);
		} else { //bg2
			//<DAMAGER> did <AMOUNT> damage to <DAMAGEE>
			core->GetTokenDictionary()->SetAtCopy( "DAMAGEE", GetName(0) );
			// wipe the DAMAGER token, so we can color it
			core->GetTokenDictionary()->SetAtCopy( "DAMAGER", "" );
			core->GetTokenDictionary()->SetAtCopy( "AMOUNT", dmg_str);
			core->DisplayConstantStringName(STR_DAMAGE1, 0xffffff, hitter);
		}
	} else {
		// this will also display for hits on stoneskins and mirrorimages
		printMessage("Actor", " ", GREEN);
		printf("is immune to damage type %d (strrefs.h reference): %s.\n", damagetype_str_id, type_name);

		if (detailed) {
			//<DAMAGEE> was immune to my <TYPE> damage
			core->GetTokenDictionary()->SetAtCopy( "DAMAGEE", GetName(0) );
			core->GetTokenDictionary()->SetAtCopy( "TYPE", type_name );
			core->DisplayConstantStringName(STR_DAMAGE_IMMUNITY, 0xffffff, hitter);
		} else if (core->GetStringReference(STR_DAMAGE_IMMUNITY) != (ieStrRef) -1 && core->GetStringReference(STR_DAMAGE1) != (ieStrRef) -1) {
			// bg2
			//<DAMAGEE> was immune to my damage.
			core->GetTokenDictionary()->SetAtCopy( "DAMAGEE", GetName(0) );
			core->DisplayConstantStringName(STR_DAMAGE_IMMUNITY, 0xffffff, hitter);
		} // else: other games don't display anything
	}

}

//Just to quickly inspect debug maximum values
#if 0
void Actor::DumpMaxValues()
{
	int symbol = core->LoadSymbol( "stats" );
	SymbolMgr *sym = core->GetSymbol( symbol );

	for(int i=0;i<MAX_STATS;i++) {
		printf("%d (%s) %d\n", i, sym->GetValue(i), maximum_values[i]);
	}
}
#endif

void Actor::DebugDump()
{
	unsigned int i;

	printf( "Debugdump of Actor %s:\n", LongName );
	printf ("Scripts:");
	for (i = 0; i < MAX_SCRIPTS; i++) {
		const char* poi = "<none>";
		if (Scripts[i] && Scripts[i]->script) {
			poi = Scripts[i]->GetName();
		}
		printf( " %.8s", poi );
	}
	printf( "\nArea:       %.8s   ", Area );
	printf( "Dialog:     %.8s\n", Dialog );
	printf( "Global ID:  %d   Local ID:  %d\n", globalID, localID);
	printf( "Script name:%.32s\n", scriptName );
	printf( "TalkCount:  %d   ", TalkCount );
	printf( "PartySlot:  %d\n", InParty );
	printf( "Allegiance: %d   current allegiance:%d\n", BaseStats[IE_EA], Modified[IE_EA] );
	printf( "Class:      %d   current class:%d\n", BaseStats[IE_CLASS], Modified[IE_CLASS] );
	printf( "Race:       %d   current race:%d\n", BaseStats[IE_RACE], Modified[IE_RACE] );
	printf( "Gender:     %d   current gender:%d\n", BaseStats[IE_SEX], Modified[IE_SEX] );
	printf( "Specifics:  %d   current specifics:%d\n", BaseStats[IE_SPECIFIC], Modified[IE_SPECIFIC] );
	printf( "Alignment:  %x   current alignment:%x\n", BaseStats[IE_ALIGNMENT], Modified[IE_ALIGNMENT] );
	printf( "Morale:     %d   current morale:%d\n", BaseStats[IE_MORALE], Modified[IE_MORALE] );
	printf( "Moralebreak:%d   Morale recovery:%d\n", Modified[IE_MORALEBREAK], Modified[IE_MORALERECOVERYTIME] );
	printf( "Visualrange:%d (Explorer: %d)\n", Modified[IE_VISUALRANGE], Modified[IE_EXPLORE] );
	printf( "HP:         %d   current HP:%d\n", BaseStats[IE_HITPOINTS], Modified[IE_HITPOINTS] );
	printf( "Mod[IE_ANIMATION_ID]: 0x%04X\n", Modified[IE_ANIMATION_ID] );
	printf( "Colors:    ");
	if (core->HasFeature(GF_ONE_BYTE_ANIMID) ) {
		for(i=0;i<Modified[IE_COLORCOUNT];i++) {
			printf("   %d", Modified[IE_COLORS+i]);
		}
	}
	else {
		for(i=0;i<7;i++) {
			printf("   %d", Modified[IE_COLORS+i]);
		}
	}
	printf ("\nAnimate ID: %x\n", Modified[IE_ANIMATION_ID]);
	printf( "WaitCounter: %d\n", (int) GetWait());
	printf( "LastTarget: %d %s\n", LastTarget, GetActorNameByID(LastTarget));
	printf( "LastTalked: %d %s\n", LastTalkedTo, GetActorNameByID(LastTalkedTo));
	inventory.dump();
	spellbook.dump();
	fxqueue.dump();
#if 0
	DumpMaxValues();
#endif
}

const char* Actor::GetActorNameByID(ieDword ID) const
{
	Actor *actor = GetCurrentArea()->GetActorByGlobalID(ID);
	if (!actor) {
		return "<NULL>";
	}
	return actor->GetScriptName();
}

void Actor::SetMap(Map *map, ieWord LID, ieWord GID)
{
	//Did we have an area?
	bool effinit=!GetCurrentArea();
	Scriptable::SetMap(map); //now we have an area
	localID = LID;
	globalID = GID;

	//These functions are called once when the actor is first put in
	//the area. It already has all the items (including fist) at this
	//time and it is safe to call effects.
	//This hack is to delay the equipping effects until the actor has
	//an area (and the game object is also existing)
	if (effinit) {
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
			SetEquippedQuickSlot( inventory.GetEquipped(), EquippedHeader );
		}
	}
}

void Actor::SetPosition(const Point &position, int jump, int radius)
{
	PathTries = 0;
	ClearPath();
	Point p;
	p.x = position.x/16;
	p.y = position.y/12;
	if (jump && !(Modified[IE_DONOTJUMP] & DNJ_FIT) && size ) {
		GetCurrentArea()->AdjustPosition( p, radius );
	}
	p.x = p.x * 16 + 8;
	p.y = p.y * 12 + 6;
	MoveTo( p );
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

	float classcount = 0;
	float average = 0;
	if (core->HasFeature(GF_IWD2_SCRIPTNAME)) {
		// iwd2
		for (int i=0; i < 11; i++) {
			if (stats[levelslotsiwd2[i]] > 0) classcount++;
		}
		average = stats[IE_CLASSLEVELSUM] / classcount + 0.5;
	}
	else {
		int levels[3]={stats[IE_LEVEL], stats[IE_LEVEL2], stats[IE_LEVEL3]};
		average = levels[0];
		classcount = 1;
		if (IsDualClassed()) {
			// dualclassed
			if (levels[1] > 0) {
				classcount++;
				average += levels[1];
			}
		}
		else if (IsMultiClassed()) {
				//classcount is the number of on bits in the MULTI field
				classcount = bitcount (multiclass);
				for (int i=1; i<classcount; i++)
					average += levels[i];
		} //else single classed
		average = average / classcount + 0.5;
	}
	return ieDword(average);
}

/** maybe this would be more useful if we calculate with the strength too
*/
int Actor::GetEncumbrance()
{
	return inventory.GetWeight();
}

//receive turning
void Actor::Turn(Scriptable *cleric, ieDword turnlevel)
{
	//this is safely hardcoded i guess
	if (Modified[IE_GENERAL]!=GEN_UNDEAD) {
		return;
	}
	//determine if we see the cleric (distance)

	//determine alignment (if equals, then no turning)

	//determine panic or destruction
	//we get the modified level
	if (turnlevel>GetXPLevel(true)) {
		Die(cleric);
	} else {
		Panic();
	}
}

void Actor::Resurrect()
{
	if (!(Modified[IE_STATE_ID ] & STATE_DEAD)) {
		return;
	}
	InternalFlags&=IF_FROMGAME; //keep these flags (what about IF_INITIALIZED)
	InternalFlags|=IF_ACTIVE|IF_VISIBLE; //set these flags
	SetBase(IE_STATE_ID, 0);
	SetBase(IE_MORALE, 10);
	SetBase(IE_HITPOINTS, BaseStats[IE_MAXHITPOINTS]);
	ClearActions();
	ClearPath();
	SetStance(IE_ANI_EMERGE);
	//readjust death variable on resurrection
	if (core->HasFeature(GF_HAS_KAPUTZ) && (AppearanceFlags&APP_DEATHVAR)) {
		ieVariable DeathVar;

		snprintf(DeathVar,sizeof(ieVariable),"%s_DEAD",scriptName);
		Game *game=core->GetGame();
		ieDword value=0;

		game->kaputz->Lookup(DeathVar, value);
		if (value) {
			game->kaputz->SetAt(DeathVar, value-1);
		}
	}
	//clear effects?
}

void Actor::Die(Scriptable *killer)
{
	int i,j;

	if (InternalFlags&IF_REALLYDIED) {
		return; //can die only once
	}

	int minhp=Modified[IE_MINHITPOINTS];
	if (minhp) { //can't die
		SetBase(IE_HITPOINTS, minhp);
		return;
	}
	//Can't simply set Selected to false, game has its own little list
	Game *game = core->GetGame();
	game->SelectActor(this, false, SELECT_NORMAL);
	game->OutAttack(GetID());

	// actions are NOT cleared on death
	ClearPath();
	SetModal( 0 );
	DisplayStringCore(this, VB_DIE, DS_CONSOLE|DS_CONST );

	// clearing the search map here means it's not blocked during death animations
	// this is perhaps not ideal, but matches other searchmap code which uses
	// GA_NO_DEAD to exclude IF_JUSTDIED actors as well as dead ones
	area->ClearSearchMapFor(this);

	//JUSTDIED will be removed when the Die() trigger executed
	//otherwise it is the same as REALLYDIED
	InternalFlags|=IF_REALLYDIED|IF_JUSTDIED;
	SetStance( IE_ANI_DIE );

	if (InParty) {
		game->PartyMemberDied(this);
		core->Autopause(AP_DEAD);
	} else {
		Actor *act=NULL;
		if (!killer) {
			killer = area->GetActorByGlobalID(LastHitter);
		}

		if (killer) {
			if (killer->Type==ST_ACTOR) {
				act = (Actor *) killer;
			}
		}

		if (act && act->InParty) {
			//adjust kill statistics here
			PCStatsStruct *stat = act->PCStats;
			if (stat) {
				stat->NotifyKill(Modified[IE_XPVALUE], ShortStrRef);
			}
			InternalFlags|=IF_GIVEXP;
		}
	}

	// XP seems to be handed at out at the moment of death
	if (InternalFlags&IF_GIVEXP) {
		//give experience to party
		game->ShareXP(Modified[IE_XPVALUE], sharexp );
		//handle reputation here
	}

	// death variables are updated at the moment of death
	if (KillVar[0]) {
		ieDword value = 0;
		if (core->HasFeature(GF_HAS_KAPUTZ) ) {
			game->kaputz->Lookup(KillVar, value);
			game->kaputz->SetAt(KillVar, value+1);
		} else {
			game->locals->Lookup(KillVar, value);
			game->locals->SetAt(KillVar, value+1);
		}
	}
	if (scriptName[0]) {
		ieVariable varname;
		ieDword value = 0;

		if (core->HasFeature(GF_HAS_KAPUTZ) ) {
			if (AppearanceFlags&APP_DEATHVAR) {
				snprintf(varname, 32, "%s_DEAD", scriptName);
				game->kaputz->Lookup(varname, value);
				game->kaputz->SetAt(varname, value+1);
			}
			if (AppearanceFlags&APP_DEATHTYPE) {
				snprintf(varname, 32, "KILL_%s", KillVar);
				game->kaputz->Lookup(varname, value);
				game->kaputz->SetAt(varname, value+1);
			}
		} else {
			snprintf(varname, 32, DeathVarFormat, scriptName);
			game->locals->Lookup(varname, value);
			game->locals->SetAt(varname, value+1);
		}
	}

	//death counters for PST
	j=APP_GOOD;
	for(i=0;i<4;i++) {
		if (AppearanceFlags&j) {
			ieDword value = 0;
			game->locals->Lookup(CounterNames[i], value);
			game->locals->SetAt(CounterNames[i], value+DeathCounters[i]);
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
			ieDword value = 0;
			area->locals->Lookup(varname, value);
			// i am guessing that we shouldn't decrease below 0
			if (value > 0) {
				area->locals->SetAt(varname, value-1);
			}
		}
	}

	//a plot critical creature has died (iwd2)
	if (BaseStats[IE_MC_FLAGS]&MC_PLOT_CRITICAL) {
		core->GetGUIScriptEngine()->RunFunction("DeathWindowPlot", false);
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
}

bool Actor::CheckOnDeath()
{
	if (InternalFlags&IF_CLEANUP) {
		return true;
	}
	if (InternalFlags&IF_JUSTDIED) {
		if (lastRunTime == 0 || CurrentAction || GetNextAction()) {
			return false; //actor is currently dying, let him die first
		}
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
	if (gc && (globalID == gc->targetID || globalID == gc->speakerID)) {
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
	DropItem("",0);

	//remove all effects that are not 'permanent after death' here
	//permanent after death type is 9
	SetBaseBit(IE_STATE_ID, STATE_DEAD, true);

	// party actors are never removed
	if (InParty) return false;

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
				CheckWeaponQuickSlot(which);
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
	//disabling quick weapon slots for certain classes
	for(i=0;i<2;i++) {
		int which = ACT_WEAPON3+i;
		// Assuming that ACT_WEAPON3 and 4 are always in the first two spots
		if (PCStats->QSlots[i]!=which) {
			SetupQuickSlot(which, 0xffff, 0xffff);
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


void Actor::SetupQuickSlot(unsigned int which, int slot, int headerindex)
{
	if (!PCStats) return;
	PCStats->InitQuickSlot(which, slot, headerindex);
	//something changed about the quick items
	core->SetEventFlag(EF_ACTION);
}

bool Actor::ValidTarget(int ga_flags) const
{
	if (Immobile()) return false;
	//scripts can still see this type of actor

	if (ga_flags&GA_NO_HIDDEN) {
		if (Modified[IE_AVATARREMOVAL]) return false;
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
		if (Modified[IE_STATE_ID] & STATE_CANTLISTEN) return false;
		//can't talk to hostile
		if (Modified[IE_EA]>=EA_EVILCUTOFF) return false;
		break;
	}
	if (ga_flags&GA_NO_DEAD) {
		if (InternalFlags&IF_JUSTDIED) return false;
		if (Modified[IE_STATE_ID] & STATE_DEAD) return false;
	}
	if (ga_flags&GA_SELECT) {
		if (UnselectableTimer) return false;
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
	printf ("AnimID: %04X\n", NewAnimID);
	SetBase( IE_ANIMATION_ID, NewAnimID);
	//SetAnimationID ( NewAnimID );
}

void Actor::GetPrevAnimation()
{
	int RowNum = anims->AvatarsRowNum + 1;
	if (RowNum>=CharAnimations::GetAvatarsCount() )
		RowNum = 0;
	int NewAnimID = CharAnimations::GetAvatarStruct(RowNum)->AnimID;
	printf ("AnimID: %04X\n", NewAnimID);
	SetBase( IE_ANIMATION_ID, NewAnimID);
	//SetAnimationID ( NewAnimID );
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
	if (!wield) {
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
ITMExtHeader *Actor::GetWeapon(WeaponInfo &wi, bool leftorright)
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

	//select first weapon header
	ITMExtHeader *which = item->GetWeaponHeader(GetAttackStyle() == WEAPON_RANGED);
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
	//return which->Range+1;
}

void Actor::GetNextStance()
{
	static int Stance = IE_ANI_AWAKE;

	if (--Stance < 0) Stance = MAX_ANIMS-1;
	printf ("StanceID: %d\n", Stance);
	SetStance( Stance );
}

int Actor::LearnSpell(const ieResRef spellname, ieDword flags)
{
	if (spellbook.HaveSpell(spellname, 0) ) {
		return LSR_KNOWN;
	}
	Spell *spell = gamedata->GetSpell(spellname);
	if (!spell) {
		return LSR_INVALID; //not existent spell
	}

	if (flags & LS_STATS) {
		// chance to learn roll
		if (core->Roll(1,100,0) > core->GetIntelligenceBonus(0, GetStat(IE_INT))) {
			return LSR_FAILED;
		}
	}

	int explev = spellbook.LearnSpell(spell, flags&LS_MEMO);
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
		core->DisplayConstantStringName(tmp, 0xbcefbc, this);
	}
	if (flags&LS_ADDXP) {
		AddExperience(XP_LEARNSPELL, explev);
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
			core->DisplayConstantString(STR_TARGETBUSY,0xff0000);
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

void Actor::SetModal(ieDword newstate)
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
	//come here only if success
	ModalState = newstate;
}

//this is just a stub function for now, attackstyle could be melee/ranged
//even spells got this attack style
int Actor::GetAttackStyle()
{
	WeaponInfo wi;
	//Non NULL if the equipped slot is a projectile or a throwing weapon
	//TODO some weapons have both melee and ranged capability
	if (GetRangedWeapon(wi) != NULL) return WEAPON_RANGED;
	return WEAPON_MELEE;
}

void Actor::SetTarget( Scriptable *target)
{
	if (target->Type==ST_ACTOR) {
		Actor *tar = (Actor *) target;
		LastTarget = tar->GetID();
		tar->LastAttacker = GetID();
		//we tell the game object that this creature
		//must be added to the list of combatants
		core->GetGame()->InAttack(tar->LastAttacker);
	}
	SetOrientation( GetOrient( target->Pos, Pos ), false );
}

//in case of LastTarget = 0
void Actor::StopAttack()
{
	SetStance(IE_ANI_READY);
	core->GetGame()->OutAttack(GetID());
	InternalFlags|=IF_TARGETGONE; //this is for the trigger!
	if (InParty) {
		core->Autopause(AP_NOTARGET);
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
	return 0;
}

//calculate how many attacks will be performed
//in the next round
//only called when Game thinks we are in attack
//so it is safe to do cleanup here (it will be called only once)
void Actor::InitRound(ieDword gameTime, bool secondround)
{
	lastInit = gameTime;

	//roundTime will equal 0 if we aren't attacking something
	if (roundTime) {
		//only perform calculations at the beginning of the round!
		if (((gameTime-roundTime)%ROUND_SIZE != 0) || \
		(roundTime == lastInit)) {
			return;
		}
	}

	//reset variables used in PerformAttack
	attackcount = 0;
	attacksperround = 0;
	nextattack = 0;
	lastattack = 0;

	//we set roundTime to zero on any of the following returns, because this
	//is guaranteed to be the start of a round, and we only want roundTime
	//if we are attacking this round
	if (InternalFlags&IF_STOPATTACK) {
		core->GetGame()->OutAttack(GetID());
		roundTime = 0;
		return;
	}

	if (!LastTarget) {
		StopAttack();
		roundTime = 0;
		return;
	}

	//if held or disabled, etc, then cannot continue attacking
	ieDword state = GetStat(IE_STATE_ID);
	if (state&STATE_CANTMOVE) {
		roundTime = 0;
		return;
	}
	if (Immobile()) {
		roundTime = 0;
		return;
	}

	//add one for second round to get an extra attack only if we
	//are x/2 attacks per round
	attackcount = GetStat(IE_NUMBEROFATTACKS);
	if (secondround) {
		attackcount++;
	}
	//all numbers of attacks are stored at twice their value
	attackcount >>= 1;

	//make sure we always get at least 1apr
	if (attackcount < 1)      attackcount = 1;

	//set our apr and starting round time
	attacksperround = attackcount;
	roundTime = gameTime;

	//print a little message :)
	printMessage("InitRound", " ", WHITE);
	printf("Name: %s | Attacks: %d | Start: %d\n", ShortName, attacksperround, gameTime);

	// this might not be the right place, but let's give it a go
	if (attacksperround && InParty) {
		core->Autopause(AP_ENDROUND);
	}
}

bool Actor::GetCombatDetails(int &tohit, bool leftorright, WeaponInfo& wi, ITMExtHeader *&header, ITMExtHeader *&hittingheader, \
		ieDword &Flags, int &DamageBonus, int &speed, int &CriticalBonus, int &style)
{
	tohit = GetStat(IE_TOHIT);
	speed = -GetStat(IE_PHYSICALSPEED);
	leftorright = leftorright && IsDualWielding();
	header = GetWeapon(wi,leftorright);
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
		Flags = WEAPON_MELEE;
		break;
	case ITEM_AT_PROJECTILE: //throwing weapon
		Flags = WEAPON_RANGED;
		break;
	case ITEM_AT_BOW:
		rangedheader = GetRangedWeapon(wi);
		if (!rangedheader) {
			//display out of ammo verbal constant if there is any???
			//DisplayStringCore(this, VB_OUTOFAMMO, DS_CONSOLE|DS_CONST );
			SetStance(IE_ANI_READY);
			//set some trigger?
			return false;
		}
		Flags = WEAPON_RANGED;
		//The bow can give some bonuses, but the core attack is made by the arrow.
		hittingheader = rangedheader;
		THAC0Bonus += rangedheader->THAC0Bonus;
		DamageBonus += rangedheader->DamageBonus;
		break;
	default:
		//item is unsuitable for fight
		SetStance(IE_ANI_READY);
		return false;
	}//melee or ranged
	if (leftorright) Flags|=WEAPON_LEFTHAND;
	//this flag is set by the bow in case of projectile launcher.
	if (header->RechargeFlags&IE_ITEM_USESTRENGTH) Flags|=WEAPON_USESTRENGTH;

	//add in proficiency bonuses
	ieDword stars;
	if (wi.prof && (wi.prof <= MAX_STATS)) {
		stars = GetStat(wi.prof)&PROFS_MASK;

		//hit/damage/speed bonuses from wspecial
		if ((signed)stars > wspecial_max) {
			stars = wspecial_max;
		}
		THAC0Bonus += wspecial[stars][0];
		DamageBonus += wspecial[stars][1];
		speed += wspecial[stars][2];
		 // add non-proficiency penalty, which is missing from the table
		if (stars == 0) THAC0Bonus -= 4;
	}

	if (IsDualWielding() && wsdualwield) {
		//add dual wielding penalty
		stars = GetStat(IE_PROFICIENCY2WEAPON)&PROFS_MASK;
		if (stars > STYLE_MAX) stars = STYLE_MAX;

		style = 1000*stars + IE_PROFICIENCY2WEAPON;
		THAC0Bonus += wsdualwield[stars][leftorright?1:0];
	} else if (wi.itemflags&(IE_INV_ITEM_TWOHANDED) && (Flags&WEAPON_MELEE) && wstwohanded) {
		//add two handed profs bonus
		stars = GetStat(IE_PROFICIENCY2HANDED)&PROFS_MASK;
		if (stars > STYLE_MAX) stars = STYLE_MAX;

		style = 1000*stars + IE_PROFICIENCY2HANDED;
		DamageBonus += wstwohanded[stars][0];
		CriticalBonus = wstwohanded[stars][1];
		speed += wstwohanded[stars][2];
	} else if (Flags&WEAPON_MELEE) {
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

	//second parameter is left or right hand flag
	tohit = GetToHit(THAC0Bonus, Flags);
	return true;
}

int Actor::GetToHit(int bonus, ieDword Flags)
{
	int tohit = bonus;

	//get our dual wielding modifier
	if (IsDualWielding()) {
		if (Flags&WEAPON_LEFTHAND) {
			tohit += GetStat(IE_HITBONUSLEFT);
		} else {
			tohit += GetStat(IE_HITBONUSRIGHT);
		}
	}

	//get attack style (melee or ranged)
	switch(Flags&WEAPON_STYLEMASK) {
		case WEAPON_MELEE:
			tohit += GetStat(IE_MELEETOHIT);
			break;
		case WEAPON_FIST:
			tohit += GetStat(IE_FISTHIT);
			break;
		case WEAPON_RANGED:
			tohit += GetStat(IE_MISSILEHITBONUS);
			//add dexterity bonus
			tohit += core->GetDexterityBonus(STAT_DEX_MISSILE, GetStat(IE_DEX));
			break;
	}
	//add strength bonus if we need
	if (Flags&WEAPON_USESTRENGTH) {
		tohit += core->GetStrengthBonus(0,GetStat(IE_STR), GetStat(IE_STREXTRA) );
	}

	if (ReverseToHit) {
		tohit = (signed)GetStat(IE_TOHIT)-tohit;
	} else {
		tohit += GetStat(IE_TOHIT);
	}
	return tohit;
}

static const int weapon_damagetype[] = {DAMAGE_CRUSHING, DAMAGE_PIERCING,
	DAMAGE_CRUSHING, DAMAGE_SLASHING, DAMAGE_MISSILE, DAMAGE_STUNNING};

int Actor::GetDefense(int DamageType)
{
	//dexterity bonus.
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

	if (ReverseToHit) {
		defense = GetStat(IE_ARMORCLASS)-defense;
	} else {
		defense += GetStat(IE_ARMORCLASS);
	}
//Defense bonus are stocked < 0 in 2da files.
	return defense + core->GetDexterityBonus(STAT_DEX_AC, GetStat(IE_DEX) );
}

void Actor::PerformAttack(ieDword gameTime)
{
	// start a new round if we really don't have one yet
	if (!roundTime) {
		InitRound(gameTime, false);
	}

	//only return if we don't have any attacks left this round
	if (attackcount==0) return;

	// this check shouldn't be necessary, but it causes a divide-by-zero below,
	// so i would like it to be clear if it ever happens
	if (attacksperround==0) {
		printMessage("Actor", "APR was 0 in PerformAttack!\n", RED);
		return;
	}

	//don't continue if we can't make the attack yet
	//we check lastattack because we will get the same gameTime a few times
	if ((nextattack > gameTime) || (gameTime == lastattack)) {
		// fuzzie added the following line as part of the UpdateActorState hack below
		lastattack = gameTime;
		return;
	}

	if (InternalFlags&IF_STOPATTACK) {
		// this should be avoided by the AF_ALIVE check by all the calling actions
		printMessage("Actor", "Attack by dead actor!", LIGHT_RED);
		return;
	}

	if (!LastTarget) {
		printMessage("Actor", "Attack without valid target ID!", LIGHT_RED);
		return;
	}
	//get target
	Actor *target = area->GetActorByGlobalID(LastTarget);

	if (target && (target->GetStat(IE_STATE_ID)&STATE_DEAD)) {
		target = NULL;
	}

	if (!target) {
		printMessage("Actor", "Attack without valid target!", LIGHT_RED);
		return;
	}

	printf("Performattack for %s, target is: %s\n", ShortName, target->ShortName);

	//which hand is used
	//we do apr - attacksleft so we always use the main hand first
	bool leftorright = (bool) ((attacksperround-attackcount)&1);

	WeaponInfo wi;
	ITMExtHeader *header = NULL;
	ITMExtHeader *hittingheader = NULL;
	int tohit;
	ieDword Flags;
	int DamageBonus, CriticalBonus;
	int speed, style;

	//will return false on any errors (eg, unusable weapon)
	if (!GetCombatDetails(tohit, leftorright, wi, header, hittingheader, Flags, DamageBonus, speed, CriticalBonus, style)) {
		return;
	}

	//if this is the first call of the round, we need to update next attack
	if (nextattack == 0) {
		//FIXME: figure out exactly how initiative is calculated
		int initiative = core->Roll(1, 5, GetXPLevel(true)/(-8));
		int spdfactor = hittingheader->Speed + speed + initiative;
		if (spdfactor<0) spdfactor = 0;
		if (spdfactor>10) spdfactor = 10;

		//(round_size/attacks_per_round)*(initiative) is the first delta
		nextattack = ROUND_SIZE*spdfactor/(attacksperround*10) + gameTime;

		//we can still attack this round if we have a speed factor of 0
		if (nextattack > gameTime) {
			return;
		}
	}

	if((PersonalDistance(this, target) > wi.range*10) /*|| (!GetCurrentArea()->IsVisible(Pos, target->Pos))*/) {
		// this is a temporary double-check, remove when bugfixed
		printMessage("Actor", "Attack action didn't bring us close enough!", LIGHT_RED);
		return;
	}

	SetStance(AttackStance);

	//figure out the time for our next attack since the old time has the initiative
	//in it, we only have to add the basic delta
	attackcount--;
	nextattack += (ROUND_SIZE/attacksperround);
	lastattack = gameTime;

	//debug messages
	if (leftorright && IsDualWielding()) {
		printMessage("Attack","(Off) ", YELLOW);
	} else {
		printMessage("Attack","(Main) ", GREEN);
	}
	if (attacksperround) {
		printf("Left: %d | ", attackcount);
		printf("Next: %d ", nextattack);
	}

	int roll = core->Roll(1,ATTACKROLL,0);
	if (roll==1) {
		//critical failure
		printBracket("Critical Miss", RED);
		printf("\n");
		DisplayStringCore(this, VB_CRITMISS, DS_CONSOLE|DS_CONST );
		if (Flags&WEAPON_RANGED) {//no need for this with melee weapon!
			UseItem(wi.slot, (ieDword)-2, target, UI_MISS);
		} else if (core->HasFeature(GF_BREAKABLE_WEAPONS)) {
			//break sword
			//TODO: this appears to be a random roll on-hit (perhaps critical failure
			// too); we use 1% (1d20*1d5==1)
			if ((header->RechargeFlags&IE_ITEM_BREAKABLE) && core->Roll(1,5,0) == 1) {
				inventory.BreakItemSlot(wi.slot);
			}
		}
		return;
	}
	//damage type is?
	//modify defense with damage type
	ieDword damagetype = hittingheader->DamageType;
	int damage = core->Roll(hittingheader->DiceThrown, hittingheader->DiceSides, DamageBonus);
	printf("| Damage %dd%d%+d = %d ",hittingheader->DiceThrown, hittingheader->DiceSides, DamageBonus, damage);
	int damageluck = (int) GetStat(IE_DAMAGELUCK);
	if (damage<damageluck) {
		damage = damageluck;
	}

	if (roll >= (ATTACKROLL - (int) GetStat(IE_CRITICALHITBONUS) - CriticalBonus)) {
		//critical success
		printBracket("Critical Hit", GREEN);
		printf("\n");
		DisplayStringCore(this, VB_CRITHIT, DS_CONSOLE|DS_CONST );
		ModifyDamage (target, damage, weapon_damagetype[damagetype], &wi, true);
		UseItem(wi.slot, Flags&WEAPON_RANGED?-2:-1, target, 0, damage);
		return;
	}

	//get target's defense against attack
	int defense = target->GetDefense(damagetype);

	bool success;
	if(ReverseToHit) {
		success = roll > tohit - defense;
	} else {
		success = tohit + roll > defense;
	}

	if (!success) {
		//hit failed
		if (Flags&WEAPON_RANGED) {//Launch the projectile anyway
			UseItem(wi.slot, (ieDword)-2, target, UI_MISS);
		}
		printBracket("Missed", LIGHT_RED);
		printf("\n");
		return;
	}
	printBracket("Hit", GREEN);
	printf("\n");
	ModifyDamage (target, damage, weapon_damagetype[damagetype], &wi, false);
	UseItem(wi.slot, Flags&WEAPON_RANGED?-2:-1, target, 0, damage);
}

static EffectRef fx_stoneskin_ref={"StoneSkinModifier",NULL,-1};
static EffectRef fx_stoneskin2_ref={"StoneSkin2Modifier",NULL,-1};
static EffectRef fx_mirrorimage_ref={"MirrorImageModifier",NULL,-1};
static EffectRef fx_aegis_ref={"Aegis",NULL,-1};

void Actor::ModifyDamage(Actor *target, int &damage, int damagetype, WeaponInfo *wi, bool critical)
{

	int mirrorimages = target->Modified[IE_MIRRORIMAGES];
	if (mirrorimages) {
		if (core->Roll(1,mirrorimages+1,0)!=1) {
			target->fxqueue.DecreaseParam1OfEffect(fx_mirrorimage_ref, 1);
			target->Modified[IE_MIRRORIMAGES]--;
			damage = 0;
			return;
		}
	}

	// only check stone skins if damage type is physical or magical
	// DAMAGE_CRUSHING is 0, so we can't AND with it to check for its presence
	printf("DAMAGETYPE:%d\n", damagetype);
	if (!(damagetype & ~(DAMAGE_PIERCING|DAMAGE_SLASHING|DAMAGE_MISSILE|DAMAGE_MAGIC))) {
		int stoneskins = target->Modified[IE_STONESKINS];
		if (stoneskins) {
			target->fxqueue.DecreaseParam1OfEffect(fx_stoneskin_ref, 1);
			target->fxqueue.DecreaseParam1OfEffect(fx_aegis_ref, 1);
			target->Modified[IE_STONESKINS]--;
			damage = 0;
			return;
		}

		stoneskins = target->Modified[IE_STONESKINSGOLEM];
		if (stoneskins) {
			target->fxqueue.DecreaseParam1OfEffect(fx_stoneskin2_ref, 1);
			target->Modified[IE_STONESKINSGOLEM]--;
			damage = 0;
			return;
		}
	}

	if (target->fxqueue.WeaponImmunity(wi->enchantment, wi->itemflags)) {
		damage = 0;
	}

	//check casting failure
	if (damage<0) damage = 0;
	if (!damage) {
		DisplayStringCore(this, VB_TIMMUNE, DS_CONSOLE|DS_CONST );
		return;
	}

	if (critical) {
		//a critical surely raises the morale?
		NewBase(IE_MORALE, 1, MOD_ADDITIVE);
		int head = inventory.GetHeadSlot();
		if ((head!=-1) && target->inventory.HasItemInSlot("",(ieDword) head)) {
			//critical hit is averted by helmet
			core->DisplayConstantStringName(STR_NO_CRITICAL, 0xffffff, target);
		} else {
			damage <<=1; //critical damage is always double?
			core->timer->SetScreenShake(16,16,8);
		}
	}
	return;
}

void Actor::UpdateActorState(ieDword gameTime) {
	//apply the modal effect on the beginning of each round
	if (((gameTime-roundTime)%ROUND_SIZE==0) && ModalState) {
		if (!ModalSpell[0]) {
			printMessage("Actor","Modal Spell Effect was not set!\n", YELLOW);
			ModalSpell[0]='*';
		} else if(ModalSpell[0]!='*') {
			core->ApplySpell(ModalSpell, this, this, 0);
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
			printMessage("Attack","(Leaving attack)", GREEN);
			core->GetGame()->OutAttack(GetID());
		}

		roundTime = 0;
		lastattack = 0;
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
	if (shift == 15) {
		value = 0;
		for (index=0;index<4;index++) {
			value |= gradient<<=8;
		}
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
	LastFollowed = actor->GetID();
	FollowOffset.x = xoffset;
	FollowOffset.y = yoffset;
}

//if days == 0, it means full healing
void Actor::Heal(int days)
{
	if (days) {
		SetBase(IE_HITPOINTS, BaseStats[IE_HITPOINTS]+days*2);
	} else {
		SetBase(IE_HITPOINTS, BaseStats[IE_MAXHITPOINTS]);
	}
}

//this function should handle dual classing and multi classing
void Actor::AddExperience(int exp)
{
	SetBase(IE_XP,BaseStats[IE_XP]+exp);
}

void Actor::AddExperience(int type, int level)
{
	if (type>=xpbonustypes) {
		return;
	}
	unsigned int l = (unsigned int) (level - 1);

	if (l>=(unsigned int) xpbonuslevels) {
		l=xpbonuslevels-1;
	}
	int xp = xpbonus[type*xpbonuslevels+l];
	AddExperience(xp);
	core->DisplayConstantStringValue(STR_GOTXP, 0xbcefbc, (ieDword) xp);
}

bool Actor::Schedule(ieDword gametime)
{
	if (!(InternalFlags&IF_VISIBLE) ) {
		return false;
	}

	//check for schedule
	ieDword bit = 1<<((gametime/ROUND_SIZE)%7200/300);
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

void Actor::WalkTo(Point &Des, ieDword flags, int MinDistance)
{
	PathTries = 0;
	if (InternalFlags&IF_REALLYDIED) {
		return;
	}
	SetRunFlags(flags);
	// is this true???
	if (Des.x==-2 && Des.y==-2) {
		Point p((ieWord) Modified[IE_SAVEDXPOS], (ieWord) Modified[IE_SAVEDYPOS] );
		Movable::WalkTo(p, MinDistance);
	} else {
		Movable::WalkTo(Des, MinDistance);
	}
}

//there is a similar function in Map for stationary vvcs
void Actor::DrawVideocells(Region &screen, vvcVector &vvcCells, Color &tint)
{
	Map* area = GetCurrentArea();

	for (unsigned int i = 0; i < vvcCells.size(); i++) {
		ScriptedAnimation* vvc = vvcCells[i];
/* we don't allow holes anymore
		if (!vvc)
			continue;
*/

		// actually this is better be drawn by the vvc
		bool endReached = vvc->Draw(screen, Pos, tint, area, WantDither(), GetOrientation());
		if (endReached) {
			delete vvc;
			vvcCells.erase(vvcCells.begin()+i);
			continue;
		}
	}
}

void Actor::DrawActorSprite(Region &screen, int cx, int cy, Region& bbox,
			SpriteCover*& newsc, Animation** anims,
			unsigned char Face, Color& tint)
{
	CharAnimations* ca = GetAnims();
	int PartCount = ca->GetTotalPartCount();
	Video* video = core->GetVideoDriver();
	Region vp = video->GetViewport();

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

			unsigned int flags = TranslucentShadows ? BLIT_TRANSSHADOW : 0;
			if (!ca->lockPalette) flags|=BLIT_TINTED;

			video->BlitGameSprite( nextFrame, cx + screen.x, cy + screen.y,
				flags, tint, newsc, ca->GetPartPalette(partnum), &screen);
		}
	}
}


static const int OrientdX[16] = { 0, -4, -7, -9, -10, -9, -7, -4, 0, 4, 7, 9, 10, 9, 7, 4 };
static const int OrientdY[16] = { 10, 9, 7, 4, 0, -4, -7, -9, -10, -9, -7, -4, 0, 4, 7, 9 };
static const unsigned int MirrorImageLocation[8] = { 4, 12, 8, 0, 6, 14, 10, 2 };
static const unsigned int MirrorImageZOrder[8] = { 2, 4, 6, 0, 1, 7, 5, 3 };

void Actor::Draw(Region &screen)
{
	Map* area = GetCurrentArea();

	int cx = Pos.x;
	int cy = Pos.y;
	int explored = Modified[IE_DONOTJUMP]&DNJ_UNHINDERED;
	//check the deactivation condition only if needed
	//this fixes dead actors disappearing from fog of war (they should be permanently visible)
	if ((!area->IsVisible( Pos, explored) || (InternalFlags&IF_REALLYDIED) ) &&	(InternalFlags&IF_ACTIVE) ) {
//    if ((!area->IsVisible( Pos, explored) || (InternalFlags&IF_JUSTDIED) ) &&	(InternalFlags&IF_ACTIVE) ) {
		//finding an excuse why we don't hybernate the actor
		if (Modified[IE_ENABLEOFFSCREENAI])
			return;
		if (LastTarget) //currently attacking someone
			return;
		if (CurrentAction)
			return;
		if (GetNextStep())
			return;
		if (GetNextAction())
			return;
		if (GetWait()) //would never stop waiting
			return;
		//turning actor inactive if there is no action next turn
		InternalFlags|=IF_IDLE;
	}

	if (InTrap) {
		area->ClearTrap(this, InTrap-1);
	}

	// if an actor isn't visible, should we still handle animations, draw
	// video cells, etc? let us assume not, for now..
	if (!(InternalFlags & IF_VISIBLE)) {
		return;
	}

	//visual feedback
	CharAnimations* ca = GetAnims();
	if (!ca)
		return;

	//explored or visibilitymap (bird animations are visible in fog)
	//0 means opaque
	int NoCircle = Modified[IE_NOCIRCLE];
	int Trans = Modified[IE_TRANSLUCENT];

	if (Trans>255) {
		Trans=255;
	}

	//iwd has this flag saved in the creature
	if (Modified[IE_AVATARREMOVAL]) {
		Trans = 255;
		NoCircle = 1;
	}

	int Frozen = Immobile();
	int State = Modified[IE_STATE_ID];

	if (State&STATE_DEAD) {
		NoCircle = 1;
	}

	if (State&STATE_STILL) {
		Frozen = 1;
	}

	//adjust invisibility for enemies
	if (Modified[IE_EA]>EA_GOODCUTOFF) {
		if (State&STATE_INVISIBLE) {
			Trans = 256;
		}
	}

	//can't move this, because there is permanent blur state where
	//there is no effect (just state bit)
	if ((State&STATE_BLUR) && Trans < 128) {
		Trans = 128;
	}
	Color tint = area->LightMap->GetPixel( cx / 16, cy / 12);
	tint.a = (ieByte) (255-Trans);

	unsigned int heightmapindex = area->HeightMap->GetPixelIndex( cx / 16, cy / 12);
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

	bool drawcircle = (NoCircle == 0);
	if ((core->GetGameControl()->GetScreenFlags()&SF_CUTSCENE)) {
		// ground circles are not drawn in cutscenes
		// they SHOULD be drawn for at least white speaker circles
		// (eg, via VerbalConstant), please fix :)
		drawcircle = false;
	}
	if (BaseStats[IE_STATE_ID]&STATE_DEAD || InternalFlags&IF_JUSTDIED) { 
		drawcircle = false;
	}
	bool drawtarget = drawcircle;
	// we always show circle/target on pause
	if (drawcircle && !(core->GetGameControl()->GetDialogueFlags() & DF_FREEZE_SCRIPTS)) {
		// check marker feedback level
		ieDword markerfeedback = 4;
		core->GetDictionary()->Lookup("GUI Feedback Level", markerfeedback);
		if (Over) {
			// picked creature
			drawcircle = markerfeedback >= 1;
		} else if (Selected) {
			// selected creature
			drawcircle = markerfeedback >= 2;
		} else if (IsPC()) {
			// selectable
			drawcircle = markerfeedback >= 3;
		} else if (Modified[IE_EA] >= EA_EVILCUTOFF) {
			// hostile
			drawcircle = markerfeedback >= 5;
		} else {
			// all
			drawcircle = markerfeedback >= 6;
		}
		drawtarget = Selected && markerfeedback >= 4;
	}
	if (drawcircle) {
		DrawCircle(vp);
	}
	if (drawtarget) {
		DrawTargetPoint(vp);
	}

	unsigned char StanceID = GetStance();
	unsigned char Face = GetNextFace();
	Animation** anims = ca->GetAnimation( StanceID, Face );
	if (anims) {
		// update bounding box and such
		int PartCount = ca->GetTotalPartCount();
		Sprite2D* nextFrame = 0;
		nextFrame = anims[0]->GetFrame(anims[0]->GetCurrentFrame());

		//make actor unselectable and unselected when it is not moving
		//dead, petriefied, frozen, paralysed etc.
		if (Frozen) {
			if (Selected!=0x80) {
				Selected = 0x80;
				core->GetGame()->SelectActor(this, false, SELECT_NORMAL);
			}
		}
		//If you find a better place for it, I'll really be glad to put it there
		//IN BG1 and BG2, this is at the ninth frame...
		if(attackProjectile && (anims[0]->GetCurrentFrame() == 8/*anims[0]->GetFramesCount()/2*/)) {
			GetCurrentArea()->AddProjectile(attackProjectile, Pos, LastTarget);
			attackProjectile = NULL;
		}
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

		// advance animations one frame (in sync)
		if (Frozen)
			anims[0]->LastFrame();
		else
			anims[0]->NextFrame();

		for (int part = 1; part < PartCount; ++part) {
			if (anims[part])
				anims[part]->GetSyncedNextFrame(anims[0]);
		}

		if (anims[0]->endReached) {
			if (HandleActorStance() ) {
				anims[0]->endReached = false;
				anims[0]->SetPos(0);
			}
		}

		ca->PulseRGBModifiers();
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
/*
		if (nextstance == IE_ANI_READY) {
			if (!core->GetGame()->CombatCounter) {
				nextstance = IE_ANI_AWAKE;
			}
		}
*/
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

void Actor::GetSoundFrom2DA(ieResRef Sound, unsigned int index)
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
		case VB_SELECT:
			index = 36+rand()%4;
			break;
	}
	strnlwrcpy(Sound, tab->QueryField (index, 0), 8);
}

void Actor::GetSoundFromINI(ieResRef Sound, unsigned int index)
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
			resource = core->GetResDataINI()->GetKeyAsString(section, "at1sound","");
			break;
		case VB_DAMAGE:
			resource = core->GetResDataINI()->GetKeyAsString(section, "hitsound","");
			break;
		case VB_DIE:
			resource = core->GetResDataINI()->GetKeyAsString(section, "dfbsound","");
			break;
		case VB_SELECT:
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

void Actor::ResolveStringConstant(ieResRef Sound, unsigned int index)
{
	//resolving soundset (bg1/bg2 style)
	if (PCStats && PCStats->SoundSet[0]&& csound[index]) {
		snprintf(Sound, sizeof(ieResRef), "%s%c", PCStats->SoundSet, csound[index]);
		return;
	}

	Sound[0]=0;

	if (core->HasFeature(GF_RESDATA_INI)) {
		GetSoundFromINI(Sound, index);
	} else {
		GetSoundFrom2DA(Sound, index);
	}
}

void Actor::SetActionButtonRow(ActionButtonRow &ar)
{
	for(int i=0;i<MAX_QSLOTS;i++) {
		ieByte tmp = ar[i+3];
		if (QslotTranslation) {
			tmp=gemrb2iwd[tmp];
		}
		PCStats->QSlots[i]=tmp;
	}
}

//the first 3 buttons are untouched by this function
void Actor::GetActionButtonRow(ActionButtonRow &ar)
{
	InitButtons(GetStat(IE_CLASS), false);
	for(int i=0;i<GUIBT_COUNT-3;i++) {
		ieByte tmp=PCStats->QSlots[i];
		if (QslotTranslation) {
			if (tmp>=90) { //quick weapons
				tmp=16+tmp%10;
			} else if (tmp>=80) { //quick items
				tmp=9+tmp%10;
			} else if (tmp>=70) { //quick spells
				tmp=3+tmp%10;
			} else {
				tmp=iwd2gemrb[tmp];
			}
		}
		ar[i+3]=tmp;
	}
	memcpy(ar,DefaultButtons,3*sizeof(ieByte) );
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
		SmallPortrait[i] = 'S';
		LargePortrait[i] = 'M';
	}
}

void Actor::SetSoundFolder(const char *soundset)
{
	if (core->HasFeature(GF_SOUNDFOLDERS)) {
		char filepath[_MAX_PATH];

		strnlwrcpy(PCStats->SoundFolder, soundset, 32);
		PathJoin(filepath,core->GamePath,"sounds",PCStats->SoundFolder,0);
		char *fp = FindInDir(filepath, "?????01", true);
		if (fp) {
			fp[5] = 0;
		} else {
			fp = FindInDir(filepath, "????01", true);
			if (fp) {
				fp[4] = 0;
			}
		}
		if (fp) {
			strnlwrcpy(PCStats->SoundSet, fp, sizeof(PCStats->SoundSet));
			free(fp);
		}
	} else {
		strnlwrcpy(PCStats->SoundSet, soundset, sizeof(PCStats->SoundSet));
		PCStats->SoundFolder[0]=0;
	}
}

bool Actor::HasVVCCell(const ieResRef resource)
{
	int j = true;
	vvcVector *vvcCells=&vvcShields;
retry:
	size_t i=vvcCells->size();
	while (i--) {
		ScriptedAnimation *vvc = (*vvcCells)[i];
		if (vvc == NULL) {
			continue;
		}
		if ( strnicmp(vvc->ResName, resource, 8) == 0) {
			return true;
		}
	}
	vvcCells=&vvcOverlays;
	if (j) { j = false; goto retry; }
	return false;
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
ScriptedAnimation *Actor::FindOverlay(int index)
{
	vvcVector *vvcCells;

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
	if (hours) {
		//do remove effects
		int remaining = hours*10;
		//removes hours*10 fatigue points
		NewStat (IE_FATIGUE, -remaining, MOD_ADDITIVE);
		NewStat (IE_INTOXICATION, -remaining, MOD_ADDITIVE);
		//restore hours*10 spell levels
		//rememorization starts with the lower spell levels?
		inventory.ChargeAllItems (remaining);
		for (int level = 1; level<16; level++) {
			if (level<remaining) {
				break;
			}
			while (remaining>0) {
				remaining -= RestoreSpellLevel(level,0);
			}
		}
	} else {
		SetBase (IE_FATIGUE, 0);
		SetBase (IE_INTOXICATION, 0);
		inventory.ChargeAllItems (0);
		spellbook.ChargeAllSpells ();
	}
}

//returns the actual slot from the quickslot
int Actor::GetQuickSlot(int slot)
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
	//creatures and such
	if (PCStats) {
		if (header==-1) {
			header = PCStats->QuickWeaponHeaders[slot];
		} else {
			PCStats->QuickWeaponHeaders[slot]=header;
		}
		slot = PCStats->QuickWeaponSlots[slot]-inventory.GetWeaponSlot();
	}
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
bool Actor::UseItemPoint(ieDword slot, ieDword header, Point &target, ieDword flags)
{
	CREItem *item = inventory.GetSlotItem(slot);
	if (!item)
		return false;

	ieResRef tmpresref;
	strnuprcpy(tmpresref, item->ItemResRef, sizeof(ieResRef));

	Item *itm = gamedata->GetItem(tmpresref);
	if (!itm) return false; //quick item slot contains invalid item resref
	//item is depleted for today
	if(itm->UseCharge(item->Usages, header, false)==CHG_DAY) {
		return false;
	}

	Projectile *pro = itm->GetProjectile(slot, header, flags&UI_MISS);
	ChargeItem(slot, header, item, itm, flags&UI_SILENT);
	gamedata->FreeItem(itm,tmpresref, false);
	if (pro) {
		pro->SetCaster(globalID);
		GetCurrentArea()->AddProjectile(pro, Pos, target);
		return true;
	}
	return false;
}

static EffectRef fx_damage_ref={"Damage",NULL,-1};

bool Actor::UseItem(ieDword slot, ieDword header, Scriptable* target, ieDword flags, int damage)
{
	if (target->Type!=ST_ACTOR) {
		return UseItemPoint(slot, header, target->Pos, flags);
	}

	Actor *tar = (Actor *) target;
	CREItem *item = inventory.GetSlotItem(slot);
	if (!item)
		return false;

	ieResRef tmpresref;
	strnuprcpy(tmpresref, item->ItemResRef, sizeof(ieResRef));

	Item *itm = gamedata->GetItem(tmpresref);
	if (!itm) return false; //quick item slot contains invalid item resref
	//item is depleted for today
	if (itm->UseCharge(item->Usages, header, false)==CHG_DAY) {
		return false;
	}

	Projectile *pro = itm->GetProjectile(slot, header, flags&UI_MISS);
	ChargeItem(slot, header, item, itm, flags&UI_SILENT);
	gamedata->FreeItem(itm,tmpresref, false);
	if (pro) {
		//ieDword is unsigned!!
		pro->SetCaster(globalID);
		if(((int)header < 0) && !(flags&UI_MISS)) { //using a weapon
			ITMExtHeader *which = itm->GetWeaponHeader(header == (ieDword)-2);
			Effect* AttackEffect = EffectQueue::CreateEffect(fx_damage_ref, damage, weapon_damagetype[which->DamageType], FX_DURATION_INSTANT_LIMITED);
			AttackEffect->Projectile = which->ProjectileAnimation;
			AttackEffect->Target = FX_TARGET_PRESET;
			pro->GetEffects()->AddEffect(AttackEffect, true);
			//AddEffect created a copy, the original needs to be scrapped
			delete AttackEffect;
			attackProjectile = pro;
		} else //launch it now as we are not attacking
			GetCurrentArea()->AddProjectile(pro, Pos, tar->globalID);
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

	if (InParty) {
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

			anims->nextStanceID=IE_ANI_READY;
			anims->autoSwitchOnEnd=true;
		}
	}

	switch(itm->UseCharge(item->Usages, header, true)) {
		case CHG_DAY:
			break;
		case CHG_NOSOUND: //remove item
			inventory.BreakItemSlot(slot);
			break;
		case CHG_BREAK: //both
			if (!silent) {
				core->PlaySound(DS_ITEM_GONE);
			}
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
	if ((int) cls >= classcount) {
		memcpy(&myrow, &DefaultButtons, sizeof(ActionButtonRow));
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
		if (projHeader->ProjectileType == 0) return; /* no ammo yet? */
		AttackStance = IE_ANI_SHOOT;
		anims->RangedType = projHeader->ProjectileType - 1;
		//bows ARE one handed, from an anim POV at least
		anims->SetWeaponType(IE_ANI_WEAPON_1H);
		return;
	}
	if(header && (header->AttackType == ITEM_AT_PROJECTILE)) {
		AttackStance = IE_ANI_ATTACK_SLASH; //That's it!!
		return;
	}
	AttackStance =  IE_ANI_ATTACK;
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

void Actor::SetupFist()
{
	int slot = core->QuerySlot( 0 );
	assert (core->QuerySlotEffects(slot)==SLOT_EFFECT_FIST);
	int row = GetBase(fiststat);
	int col = GetXPLevel(false);

	if (FistRows<0) {
		FistRows=0;
		AutoTable fist("fistweap");
		if (fist) {
			//default value
			strnlwrcpy( DefaultFist, fist->QueryField( (unsigned int) -1), 8);
			FistRows = fist->GetRowCount();
			fistres = new FistResType[FistRows];
			for (int i=0;i<FistRows;i++) {
				int maxcol = fist->GetColumnCount(i)-1;
				for (int cols = 0;cols<MAX_LEVEL;cols++) {
					strnlwrcpy( fistres[i][cols], fist->QueryField( i, cols>maxcol?maxcol:cols ), 8);
				}
				*(int *) fistres[i] = atoi(fist->GetRowName( i));
			}
		}
	}
	if (col>MAX_LEVEL) col=MAX_LEVEL;
	if (col<1) col=1;

	const char *ItemResRef = DefaultFist;
	for (int i = 0;i<FistRows;i++) {
		if (*(int *) fistres[i] == row) {
			ItemResRef = fistres[i][col];
		}
	}
	inventory.SetSlotItemRes(ItemResRef, slot);
}

static ieDword ResolveTableValue(const char *resref, ieDword stat, ieDword mcol, ieDword vcol) {
	long ret = 0;
	//don't close this table, it can mess with the guiscripts
	int table = gamedata->LoadTable(resref);
	TableMgr *tm = gamedata->GetTable(table);
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

static ieDword GetKitIndex (ieDword kit, const char *resref="kitlist")
{
	int kitindex = 0;

	if ((kit&BG2_KITMASK) == KIT_BARBARIAN) {
		kitindex = kit&0xfff;
	}

	// carefully looking for kit by the usability flag
	// since the barbarian kit id clashes with the no-kit value
	if (kitindex == 0 && kit != KIT_BARBARIAN) {
		TableMgr *tm = gamedata->GetTable(gamedata->LoadTable(resref) );
		if (tm) {
			kitindex = tm->FindTableValue(6, kit);
			if (kitindex < 0) {
				kitindex = 0;
			}
		}
	}

	return (ieDword)kitindex;
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
			//printf("failed usability: itemvalue %d, stat %d, stat value %d\n", itemvalue, itemuse[i].stat, stat);
			return STR_CANNOT_USE_ITEM;
		}
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
bool Actor::HasSpellState(unsigned int spellstate)
{
	if (spellstate>=192) return false;
	unsigned int pos = IE_SPLSTATE_ID1+(spellstate>>5);
	unsigned int bit = 1<<(spellstate&31);
	if (Modified[pos]&bit) return true;
	return false;
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
	if (classid>=32) return;

	//recalculate all level based changes
	pcf_level(this,0,0);

	//even though the original didn't allow a cleric/paladin dual or multiclass
	//we shouldn't restrict the possibility by using "else if" here
	if (isclass[ISCLERIC]&(1<<classid)) {
		turnundeadlevel += GetClericLevel()+1-turnlevels[classid];
		if (turnundeadlevel<0) turnundeadlevel=0;
	}
	if (isclass[ISPALADIN]&(1<<classid)) {
		turnundeadlevel += GetPaladinLevel()+1-turnlevels[classid];
		if (turnundeadlevel<0) turnundeadlevel=0;
	}

	ieDword backstabdamagemultiplier=GetThiefLevel();
	if (backstabdamagemultiplier) {
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
		printf("\n");
		if (backstabdamagemultiplier>7) backstabdamagemultiplier=7;
	}

	// monk's level dictated ac and ac vs missiles bonus
	// attacks per round bonus will be handled elsewhere, since it only applies to fist apr
	if (isclass[ISMONK]&(1<<classid)) {
		AutoTable tm("monkbon");
		if (tm) {
			int level = GetMonkLevel();
			BaseStats[IE_ARMORCLASS] = DEFAULTAC - atoi(tm->QueryField(1, level-1));
			BaseStats[IE_ACMISSILEMOD] = - atoi(tm->QueryField(2, level-1));
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
	ieDword backstabdamagemultiplier = (BaseStats[IE_LEVELTHIEF]+1)/2;
	int layonhandsamount = (int) BaseStats[IE_LEVELPALADIN];
	if (layonhandsamount) {
		layonhandsamount *= BaseStats[IE_CHR]/2-5;
		if (layonhandsamount<1) layonhandsamount = 1;
	}

	for (i=0;i<11;i++) {
		int tmp;

		if (turnlevels[i+1]) {
			tmp = BaseStats[IE_LEVELBARBARIAN+i]+1-turnlevels[i+1];
			if (tmp<0) tmp=0;
			if (tmp>turnundeadlevel) turnundeadlevel=tmp;
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
	//we have to calculate multiclass for further code
	AutoTable tm("classes");
	if (tm) {
		// currently we need only the MULTI value
		char tmpmulti[8];
		long tmp;
		strcpy(tmpmulti, tm->QueryField(tm->FindTableValue(5, BaseStats[IE_CLASS]), 4));
		if (!valid_number(tmpmulti, tmp))
			multiclass = 0;
		else
			multiclass = (ieDword)tmp;
	}

	if (core->HasFeature(GF_IWD2_SCRIPTNAME)) {
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

Actor *Actor::CopySelf() const
{
	Actor *newActor = new Actor();

	newActor->SetText(GetName(0),0);
	newActor->SetText(GetName(1),1);
	memcpy(newActor->BaseStats, BaseStats, sizeof(BaseStats) );

	//IF_INITIALIZED shouldn't be set here, yet
	newActor->SetMCFlag(MC_EXPORTABLE, BM_NAND);

	//the creature importer does this too
	memcpy(newActor->Modified,newActor->BaseStats, sizeof(Modified) );

	//these need to be called too to have a valid inventory
	newActor->inventory.SetSlotCount(inventory.GetSlotCount());
	newActor->SetupFist();

	//copy the running effects
	EffectQueue *newFXQueue = fxqueue.CopySelf();

	area->AddActor(newActor);
	newActor->SetPosition( Pos, CC_CHECK_IMPASSABLE, 0 );
	newActor->SetOrientation(GetOrientation(),0);
	newActor->SetStance( IE_ANI_READY );

	//and apply them
	newActor->RefreshEffects(newFXQueue);
	return newActor;
}

ieDword Actor::GetClassLevel(const ieDword id) const
{
	if (id>=ISCLASSES)
		return 0;

	//return iwd2 value if appropriate
	if (version==22)
		return BaseStats[levelslotsiwd2[id]];

	//houston, we gots a problem!
	if (!levelslots || !dualswap)
		return 0;

	//only works with PC's
	ieDword	classid = BaseStats[IE_CLASS]-1;
	if (classid>=(ieDword)classcount || !levelslots[classid])
		return 0;

	//get the levelid (IE_LEVEL,*2,*3)
	ieDword levelid = levelslots[classid][id];
	if (!levelid)
		return 0;

	//do dual-swap
	if (IsDualClassed()) {
		//if the old class is inactive, and it is the class
		//being searched for, return 0
		if (IsDualInactive() && ((Modified[IE_MC_FLAGS]&MC_WAS_ANY)==(ieDword)mcwasflags[id]))
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
