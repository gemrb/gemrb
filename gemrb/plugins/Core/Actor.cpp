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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Actor.cpp,v 1.218 2006/10/06 23:01:09 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "TableMgr.h"
#include "ResourceMgr.h"
#include "SoundMgr.h" //pst (react to death sounds)
#include "Actor.h"
#include "Interface.h"
#include "../../includes/strrefs.h"
#include "Item.h"
#include "Spell.h"
#include "Game.h"
#include "GameScript.h"
#include "GSUtils.h" //needed for DisplayStringCore
#include "Video.h"
#include <cassert>
#include "damages.h"

extern Interface* core;
#ifdef WIN32
extern HANDLE hConsole;
#endif

static Color green = {
	0x00, 0xff, 0x00, 0xff
};
static Color red = {
	0xff, 0x00, 0x00, 0xff
};
static Color yellow = {
	0xff, 0xff, 0x00, 0xff
};
static Color cyan = {
	0x00, 0xff, 0xff, 0xff
};
static Color magenta = {
	0xff, 0x00, 0xff, 0xff
};

static int classcount=-1;
static char **clericspelltables=NULL;
static char **wizardspelltables=NULL;

static ActionButtonRow *GUIBTDefaults = NULL; //qslots row count
ActionButtonRow DefaultButtons = {ACT_TALK, ACT_WEAPON1, ACT_WEAPON2,
 ACT_NONE, ACT_NONE, ACT_NONE, ACT_NONE, ACT_NONE, ACT_NONE, ACT_NONE,
 ACT_NONE, ACT_INNATE};
static int QslotTranslation = false;

static char iwd2gemrb[32]={
	0,0,20,2,22,25,0,14,
	15,23,13,0,1,24,8,21,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0
};
static char gemrb2iwd[32]={
	11,12,3,71,72,73,0,0, //0
	14,80,83,82,81,10,7,8, //8
	0,0,0,0,2,15,4,9, //16
	13,5,0,0,0,0,0,0  //24
};

//letters for char sound resolution bg1/bg2
static char csound[VCONST_COUNT];

static void InitActorTables();

static ieDword TranslucentShadows;

#define DAMAGE_LEVELS 13
#define ATTACKROLL    20
#define SAVEROLL      20
#define DEFAULTAC     10

static ieResRef d_main[DAMAGE_LEVELS]={
	"BLOODS","BLOODM","BLOODL","BLOODCR", //blood
	"SPFIRIMP","SPFIRIMP","SPFIRIMP",     //fire
	"SPSHKIMP","SPSHKIMP","SPSHKIMP",     //spark
	"SPFIRIMP","SPFIRIMP","SPFIRIMP",     //ice
};
static ieResRef d_splash[DAMAGE_LEVELS]={
	"","","","",
	"SPBURN","SPBURN","SPBURN", //flames
	"SPSPARKS","SPSPARKS","SPSPARKS", //sparks
	"","","",
};

#define BLOOD_GRADIENT 19
#define FIRE_GRADIENT 19
#define ICE_GRADIENT 71
#define STONE_GRADIENT 93

static int d_gradient[DAMAGE_LEVELS]={
	BLOOD_GRADIENT,BLOOD_GRADIENT,BLOOD_GRADIENT,BLOOD_GRADIENT,
	FIRE_GRADIENT,FIRE_GRADIENT,FIRE_GRADIENT,
	-1,-1,-1,
	ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT,
};
//the possible hardcoded overlays (they got separate stats)
#define OVERLAY_COUNT  8
#define OV_ENTANGLE    0
#define OV_SANCTUARY   1
#define OV_MINORGLOBE  2
#define OV_SHIELDGLOBE 3
#define OV_GREASE      4
#define OV_WEB         5
#define OV_BOUNCE      6  //bouncing
#define OV_BOUNCE2     7  //bouncing activated
static ieResRef overlay[OVERLAY_COUNT]={"SPENTACI","SANCTRY","MINORGLB","SPSHIELD",
"GREASED","WEBENTD","SPTURNI2","SPTURNI"};

//for every game except IWD2 we need to reverse TOHIT
static bool REVERSE_TOHIT=true;

//internal flags for calculating to hit
#define WEAPON_FIST        0
#define WEAPON_MELEE       1
#define WEAPON_RANGED      2
#define WEAPON_STYLEMASK   15
#define WEAPON_LEFTHAND    16
#define WEAPON_USESTRENGTH 32

Actor::Actor()
	: Moveble( ST_ACTOR )
{
	int i;

	for (i = 0; i < MAX_STATS; i++) {
		BaseStats[i] = 0;
		Modified[i] = 0;
	}
	Dialog[0] = 0;
	SmallPortrait[0] = 0;
	LargePortrait[0] = 0;

	anims = NULL;

	LongName = NULL;
	ShortName = NULL;
	LongStrRef = (ieStrRef) -1;
	ShortStrRef = (ieStrRef) -1;

	Leader = 0;
	LastProtected = 0;
	LastCommander = 0;
	LastHelp = 0;
	LastSeen = 0;
	LastHeard = 0;
	PCStats = NULL;
	LastCommand = 0; //used by order
	LastShout = 0; //used by heard
	LastDamage = 0;
	LastDamageType = 0;
	HotKey = 0;
	attackcount = 0;
	initiative = 0;

	inventory.SetInventoryType(INVENTORY_CREATURE);
	fxqueue.SetOwner( this );
	inventory.SetOwner( this );
	if (classcount<0) {
		InitActorTables();

		TranslucentShadows = 0;
		core->GetDictionary()->Lookup("Translucent Shadows",
			TranslucentShadows);
	}
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
	//this one is saved, but not loaded?
	localID = globalID = 0;
}

Actor::~Actor(void)
{
	unsigned int i;

	if (anims) {
		delete( anims );
	}
	core->FreeString( LongName );
	core->FreeString( ShortName );
	if (PCStats) {
		delete PCStats;
	}
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
}

void Actor::SetDefaultActions(bool qslot, ieByte slot1, ieByte slot2, ieByte slot3)
{
	QslotTranslation=qslot;
	DefaultButtons[0]=slot1;
	DefaultButtons[1]=slot2;
	DefaultButtons[2]=slot3;
}

void Actor::SetText(char* ptr, unsigned char type)
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
			recover = anims->palette;
		}
		//increase refcount hack so the palette won't get sunk
		if (recover) {
			recover->IncRef();
		}
		delete( anims );
	}
	//hacking PST no palette
	if (core->HasFeature(GF_ONE_BYTE_ANIMID) ) {
		if ((AnimID&0xf000)==0xe000) {
			if (BaseStats[IE_COLORCOUNT]) {
				printf("[Actor] Animation ID %x is supposed to be real colored (no recoloring), patched creature\n", AnimID);
			}
			BaseStats[IE_COLORCOUNT]=0;
		}
	}
	anims = new CharAnimations( AnimID&0xffff, BaseStats[IE_ARMOR_TYPE]);
	if (anims) {
		//if we have a recovery palette, then set it back
		anims->palette=recover;
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
	} else {
		printMessage("Actor", " ",LIGHT_RED);
		printf("Missing animation for %s\n",LongName);
	}
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
	Color* color;
	int color_index;

	if (Modified[IE_UNSELECTABLE]) {
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

	if (!anims)
		return;
	int csize = anims->GetCircleSize() - 1;
	if (csize >= MAX_CIRCLE_SIZE) 
		csize = MAX_CIRCLE_SIZE - 1;

	SetCircle( anims->GetCircleSize(), *color, core->GroundCircles[csize][color_index], core->GroundCircles[csize][(color_index == 0) ? 3 : color_index] );
}

//call this when morale or moralebreak changed
void pcf_morale (Actor *actor, ieDword /*Value*/)
{
	if(actor->Modified[IE_MORALE]<=actor->Modified[IE_MORALEBREAK] ) {
		actor->Panic();
	}
	//for new colour
	actor->SetCircleSize();
}

void pcf_ea (Actor *actor, ieDword /*Value*/)
{
	actor->SetCircleSize();
}

void pcf_class (Actor *actor, ieDword Value)
{
	actor->InitButtons(Value);
}

void pcf_animid(Actor *actor, ieDword Value)
{
	actor->SetAnimationID(Value);
}

static void SetLockedPalette(Actor *actor, ieDword *gradients)
{
	CharAnimations *anims = actor->GetAnims();
	if (!anims) return; //cannot apply it (yet)
	if (anims->lockPalette) return;
	//force initialisation of animation
	anims->SetColors( gradients );
	anims->GetAnimation(0,0);
	if (anims->palette) {
		anims->lockPalette=true;
	}
}

static void UnlockPalette(Actor *actor)
{
	CharAnimations *anims = actor->GetAnims();
	if (anims) {
		anims->lockPalette=false;
		anims->SetColors(actor->Modified+IE_COLORS);
	}
}

ieDword fullwhite[7]={ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT};

ieDword fullstone[7]={STONE_GRADIENT,STONE_GRADIENT,STONE_GRADIENT,STONE_GRADIENT,STONE_GRADIENT,STONE_GRADIENT,STONE_GRADIENT};

void pcf_state(Actor *actor, ieDword Value)
{
	if (Value & STATE_PETRIFIED) {
		SetLockedPalette( actor, fullstone);
		return;
	}
	if (Value & STATE_FROZEN) {
		SetLockedPalette(actor, fullwhite);
		return;
	}
	UnlockPalette(actor);
}

void pcf_hitpoint(Actor *actor, ieDword Value)
{
	if ((signed) Value>(signed) actor->Modified[IE_MAXHITPOINTS]) {
		Value=actor->Modified[IE_MAXHITPOINTS];
	}
	if ((signed) Value<(signed) actor->Modified[IE_MINHITPOINTS]) {
		Value=actor->Modified[IE_MINHITPOINTS];
	}
	if ((signed) Value<=0) {
		actor->Die(NULL);
	}
	actor->Modified[IE_MINHITPOINTS]=Value;
}

void pcf_maxhitpoint(Actor *actor, ieDword Value)
{
	if ((signed) Value<(signed) actor->Modified[IE_HITPOINTS]) {
		actor->Modified[IE_HITPOINTS]=Value;
		pcf_hitpoint(actor,Value);
	}
}

void pcf_minhitpoint(Actor *actor, ieDword Value)
{
	if ((signed) Value>(signed) actor->Modified[IE_HITPOINTS]) {
		actor->Modified[IE_HITPOINTS]=Value;
		pcf_hitpoint(actor,Value);
	}
}

void pcf_con(Actor *actor, ieDword Value)
{
	if ((signed) Value<=0) {
		actor->Die(NULL);
	}
	pcf_hitpoint(actor, actor->Modified[IE_HITPOINTS]);
}

void pcf_stat(Actor *actor, ieDword Value)
{
	if ((signed) Value<=0) {
		actor->Die(NULL);
	}
}

void pcf_gold(Actor *actor, ieDword /*Value*/)
{
	//this function will make a party member automatically donate their
	//gold to the party pool, not the same as in the original engine
	if (actor->InParty) {
		Game *game = core->GetGame();
		game->AddGold ( actor->BaseStats[IE_GOLD] );
		actor->BaseStats[IE_GOLD]=0;
	}
}

//de/activates the entangle overlay
void pcf_entangle(Actor *actor, ieDword Value)
{
	if (Value) {
		if (actor->HasVVCCell(overlay[OV_ENTANGLE]))
			return;
		ScriptedAnimation *sca = core->GetScriptedAnimation(overlay[OV_ENTANGLE]);
		actor->AddVVCell(sca);
	} else {
		actor->RemoveVVCell(overlay[OV_ENTANGLE], true);
	}
}

//de/activates the sanctuary overlay
//the sanctuary effect draws the globe half transparent

void pcf_sanctuary(Actor *actor, ieDword Value)
{
	if (Value) {
		if (!actor->HasVVCCell(overlay[OV_SANCTUARY])) {
			ScriptedAnimation *sca = core->GetScriptedAnimation(overlay[OV_SANCTUARY]);
			actor->AddVVCell(sca);
		}
		SetLockedPalette(actor, fullwhite);
		return;
	}
	actor->RemoveVVCell(overlay[OV_SANCTUARY], true);
	UnlockPalette(actor);
}

//de/activates the prot from missiles overlay
void pcf_shieldglobe(Actor *actor, ieDword Value)
{
	if (Value) {
		if (actor->HasVVCCell(overlay[OV_SHIELDGLOBE]))
			return;
		ScriptedAnimation *sca = core->GetScriptedAnimation(overlay[OV_SHIELDGLOBE]);
		actor->AddVVCell(sca);
	} else {
		actor->RemoveVVCell(overlay[OV_SHIELDGLOBE], true);
	}
}

//de/activates the globe of invul. overlay
void pcf_minorglobe(Actor *actor, ieDword Value)
{
	if (Value) {
		if (actor->HasVVCCell(overlay[OV_MINORGLOBE]))
			return;
		ScriptedAnimation *sca = core->GetScriptedAnimation(overlay[OV_MINORGLOBE]);
		actor->AddVVCell(sca);
	} else {
		actor->RemoveVVCell(overlay[OV_MINORGLOBE], true);
	}
}

//de/activates the grease background
void pcf_grease(Actor *actor, ieDword Value)
{
	if (Value) {
		if (actor->HasVVCCell(overlay[OV_GREASE]))
			return;
		actor->add_animation(overlay[OV_GREASE], -1, -1, false);
	} else {
		actor->RemoveVVCell(overlay[OV_GREASE], true);
	}
}

//de/activates the web overlay
//the web effect also immobilizes the actor!
void pcf_web(Actor *actor, ieDword Value)
{
	if (Value) {
		if (actor->HasVVCCell(overlay[OV_WEB]))
			return;
		actor->add_animation(overlay[OV_WEB], -1, 0, false);
	} else {
		actor->RemoveVVCell(overlay[OV_WEB], true);
	}
}

//de/activates the spell bounce background
void pcf_bounce(Actor *actor, ieDword Value)
{
	switch(Value) {
	case 1: //bounce passive
		if (actor->HasVVCCell(overlay[OV_BOUNCE]))
			return;
		actor->add_animation(overlay[OV_BOUNCE], -1, 0, false);
		break;
	case 2: //activated bounce
		if (actor->HasVVCCell(overlay[OV_BOUNCE2]))
			return;
		actor->add_animation(overlay[OV_BOUNCE2], -1, 0, true);
		break;
	default:
		actor->RemoveVVCell(overlay[OV_BOUNCE], true);
	}
}

//no separate values (changes are permanent)
void pcf_fatigue(Actor *actor, ieDword Value)
{
	actor->BaseStats[IE_FATIGUE]=Value;
}

//no separate values (changes are permanent)
void pcf_intoxication(Actor *actor, ieDword Value)
{
	actor->BaseStats[IE_INTOXICATION]=Value;
}

void pcf_color(Actor *actor, ieDword /*Value*/)
{
	CharAnimations *anims = actor->GetAnims();
	if (anims) {
		anims->SetColors(actor->Modified+IE_COLORS);
	}
}

void pcf_armorlevel(Actor *actor, ieDword Value)
{
	CharAnimations *anims = actor->GetAnims();
	if (anims) {
		anims->SetArmourLevel(Value);
	}
}

static int maximum_values[256]={
32767,32767,20,100,100,100,100,25,5,25,25,25,25,25,100,100,//0f
100,100,100,100,100,100,100,100,100,200,200,200,200,200,100,100,//1f
200,200,50,255,25,100,25,25,25,25,25,999999999,999999999,999999999,25,25,//2f
200,255,200,100,100,200,200,25,5,100,1,1,100,1,1,1,//3f
1,1,1,1,50,50,1,9999,25,100,100,255,1,20,20,25,//4f
25,1,1,255,25,25,255,255,25,5,5,5,5,5,5,5,//5f
5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,//6f
5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,//7f
5,5,5,5,5,5,5,100,100,100,255,5,5,255,1,1,//8f
1,25,25,30,1,1,1,25,-1,100,100,1,255,255,255,255,//9f
255,255,255,255,255,255,20,255,255,1,20,255,999999999,999999999,1,1,//af
999999999,999999999,0,0,0,0,0,0,0,0,0,0,0,0,0,0,//bf
0,0,0,0,0,0,0,25,25,255,255,255,255,65535,-1,-1,//cf - 207
-1,-1,-1,-1,-1,-1,-1,-1,40,255,65535,3,255,255,255,255,//df - 223
255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,//ef - 239
40,40,40,40, 40,40,40,40, 40,40,40,40, 255,65535,65535,15//ff
};

typedef void (*PostChangeFunctionType)(Actor *actor, ieDword Value);
static PostChangeFunctionType post_change_functions[256]={
pcf_hitpoint, pcf_maxhitpoint, NULL, NULL, NULL, NULL, NULL, NULL,
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL, //0f
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL,
NULL,NULL,NULL,NULL, NULL, NULL, pcf_fatigue, pcf_intoxication, //1f
NULL,NULL,NULL,NULL, pcf_stat, NULL, pcf_stat, pcf_stat,
pcf_stat,pcf_con,NULL,NULL, NULL, pcf_gold, pcf_morale, NULL, //2f
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL,
NULL,NULL,NULL,NULL, NULL, NULL, pcf_entangle, pcf_sanctuary, //3f
pcf_minorglobe, pcf_shieldglobe, pcf_grease, pcf_web, NULL, NULL, NULL, NULL,
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
NULL,NULL,NULL,NULL, NULL, pcf_animid,pcf_state, NULL, //cf
pcf_color,pcf_color,pcf_color,pcf_color, pcf_color, pcf_color, pcf_color, NULL,
NULL,NULL,NULL,pcf_armorlevel, NULL, NULL, NULL, NULL, //df
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL,
pcf_class,NULL,pcf_ea,NULL, NULL, NULL, NULL, NULL, //ef
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL,
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
		}
		if (wizardspelltables) {
			for (i=0;i<classcount;i++) {
				if (wizardspelltables[i]) {
					free(wizardspelltables[i]);
				}
			}
			free(wizardspelltables);
		}
	}
	if (GUIBTDefaults) {
		free (GUIBTDefaults);
		GUIBTDefaults=NULL;
	}
	classcount=-1;
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

static void InitActorTables()
{
	int i;

	REVERSE_TOHIT=(bool) core->HasFeature(GF_REVERSE_TOHIT);
	//this table lists skill groups assigned to classes
	//it is theoretically possible to create hybrid classes
	int table = core->LoadTable( "clskills" );
	TableMgr *tm = core->GetTable( table );
	if (tm) {
		classcount = tm->GetRowCount();
	} else {
		classcount = 0; //well
	}
	clericspelltables = (char **) calloc(classcount, sizeof(char*));
	wizardspelltables = (char **) calloc(classcount, sizeof(char*));
	for(i = 0; i<classcount; i++) {
		const char *spelltablename = tm->QueryField( i, 1 );
		if (spelltablename[0]!='*') {
			clericspelltables[i]=strdup(spelltablename);
		}
		spelltablename = tm->QueryField( i, 2 );
		if (spelltablename[0]!='*') {
			wizardspelltables[i]=strdup(spelltablename);
		}
	}
	core->DelTable( table );

	i = core->GetMaximumAbility();
	maximum_values[IE_STR]=i;
	maximum_values[IE_INT]=i;
	maximum_values[IE_DEX]=i;
	maximum_values[IE_CON]=i;
	maximum_values[IE_CHR]=i;
	maximum_values[IE_WIS]=i;

	//initializing the vvc resource references
	table = core->LoadTable( "damage" );
	tm = core->GetTable( table );
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
		core->DelTable( table );
	}

	table = core->LoadTable( "overlay" );
	tm = core->GetTable( table );
	if (tm) {
		for (i=0;i<OVERLAY_COUNT;i++) {
			const char *tmp = tm->QueryField( i, 0 );
			strnlwrcpy(overlay[i], tmp, 8);
		}
		core->DelTable( table );
	}

	//csound for bg1/bg2
	memset(csound,0,sizeof(csound));
	if (!core->HasFeature(GF_SOUNDFOLDERS)) {
		table = core->LoadTable( "csound" );
		tm = core->GetTable( table );
		if (tm) {
			for(i=0;i<VCONST_COUNT;i++) {
				const char *tmp = tm->QueryField( i, 0 );
				if (tmp[0]!='*') {
					csound[i]=tmp[0];
				}
			}
		}
	}

	table = core->LoadTable( "qslots" );
	tm = core->GetTable( table );
	GUIBTDefaults = (ActionButtonRow *) calloc( classcount,sizeof(ActionButtonRow) );

	for (i = 0; i < classcount; i++) {
		memcpy(GUIBTDefaults+i, &DefaultButtons, sizeof(ActionButtonRow));
		if (tm) {
			for (int j=0;j<MAX_QSLOTS;j++) {
				GUIBTDefaults[i][j+3]=(ieByte) atoi( tm->QueryField(i,j) );
			}
		}
	}
	if (tm) {
		core->DelTable( table );
	}
}

void Actor::add_animation(const ieResRef resource, int gradient, int height, bool playonce)
{
	ScriptedAnimation *sca = core->GetScriptedAnimation(resource);
	if (!sca)
		return;
	sca->ZPos=height;
	if (playonce) {
		sca->PlayOnce();
	}
	if (gradient!=-1) {
		sca->SetPalette(gradient, 4);
	}
	AddVVCell(sca);
}

void Actor::PlayDamageAnimation(int type)
{
	int i;

	switch(type) {
		case 0: case 1: case 2: case 3: //blood
			i = (int) GetStat(IE_ANIMATION_ID)>>16;
			if (!i) i = d_gradient[type];
			add_animation(d_main[type], i, 0, true);
			break;
		case 4: case 5: case 7: //fire
			add_animation(d_main[type], d_gradient[type], 0, true);
			for(i=DL_FIRE;i<=type;i++) {
				add_animation(d_splash[i], d_gradient[i], 0, true);
			}
			break;
		case 8: case 9: case 10: //electricity
			add_animation(d_main[type], d_gradient[type], 0, true);
			for(i=DL_ELECTRICITY;i<=type;i++) {
				add_animation(d_splash[i], d_gradient[i], 0, true);
			}
			break;
		case 11: case 12: case 13://cold
			add_animation(d_main[type], d_gradient[type], 0, true);
			break;
		case 14: case 15: case 16://acid
			add_animation(d_main[type], d_gradient[type], 0, true);
			break;
		case 17: case 18: case 19://disintegrate
			add_animation(d_main[type], d_gradient[type], 0, true);
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

	if (pcf && Modified[StatIndex]!=Value) {
		Modified[StatIndex] = Value;
		PostChangeFunctionType f = post_change_functions[StatIndex];
		if (f) (*f)(this, Value);
	} else {
		Modified[StatIndex] = Value;
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

	BaseStats[StatIndex] = Value;
	//if already initialized, then the modified stats
	//need to run the post change function (stat change can kill actor)
	SetStat (StatIndex, Value+diff, InternalFlags&IF_INITIALIZED);
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

void Actor::AddPortraitIcon(ieByte icon)
{
	if (!PCStats) {
		return;
	}
	ieWord *Icons = PCStats->PortraitIcons;

	int i;

	for(i=0;i<MAX_PORTRAIT_ICONS;i++) {
		if (icon == (Icons[i]&0xff)) {
			return;
		}
	}
	if (i<MAX_PORTRAIT_ICONS) {
		Icons[i]=icon;
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
void Actor::RefreshEffects()
{
	ieDword previous[MAX_STATS];

	bool first = !(InternalFlags&IF_INITIALIZED);

	if (PCStats) {
		memset( PCStats->PortraitIcons, -1, sizeof(PCStats->PortraitIcons) );
	}
	if (first) {
		InternalFlags|=IF_INITIALIZED;
	} else {	
		memcpy( previous, Modified, MAX_STATS * sizeof( *Modified ) );
	}
	memcpy( Modified, BaseStats, MAX_STATS * sizeof( *Modified ) );
	fxqueue.ApplyAllEffects( this );

	//calculate hp bonus
	int bonus;

	//fighter or not (still very primitive model, we need multiclass)
	if(Modified[IE_CLASS]==0) {
		bonus = core->GetConstitutionBonus(STAT_CON_HP_WARRIOR,Modified[IE_CON]);
	} else {
		bonus = core->GetConstitutionBonus(STAT_CON_HP_NORMAL,Modified[IE_CON]);
	}
	bonus *= GetXPLevel( true );

	Modified[IE_MAXHITPOINTS]+=bonus;
	Modified[IE_HITPOINTS]+=bonus;

	for (unsigned int i=0;i<MAX_STATS;i++) {
		if (first || (Modified[i]!=previous[i]) ) {
			PostChangeFunctionType f = post_change_functions[i];
			if (f) {
				(*f)(this, Modified[i]);
			}
		}
	}
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

void Actor::ReactToDeath(const char * deadname)
{
	int table = core->LoadTable( "death" );
	TableMgr *tm = core->GetTable( table );
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
                ieDword len = core->GetSoundMgr()->Play( value );
                ieDword counter = ( AI_UPDATE_TIME * len ) / 1000;
                if (counter != 0)
                        SetWait( counter );
		break;
	}
	}
	core->DelTable(table);
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
	if( Modified[IE_MC_FLAGS]&MC_NO_TALK)
		return;
	if (Modified[IE_EA]>=EA_EVILCUTOFF) {
		DisplayStringCore(this, VB_HOSTILE, DS_CONSOLE|DS_CONST );
	} else {
		DisplayStringCore(this, VB_DIALOG, DS_CONSOLE|DS_CONST );
	}
}

//returns actual damage
int Actor::Damage(int damage, int damagetype, Actor *hitter)
{
	//recalculate damage based on resistances and difficulty level
	//the lower 2 bits are actually modifier types
	NewBase(IE_HITPOINTS, (ieDword) -damage, damagetype&3);
	NewBase(IE_MORALE, (ieDword) -1, MOD_ADDITIVE);
	//this is just a guess, probably morale is much more complex
	//add lastdamagetype up
	LastDamageType|=damagetype;
	LastDamage=damage;
	LastHitter=hitter->GetID();
	InternalFlags|=IF_ACTIVE;
	int chp = (signed) Modified[IE_HITPOINTS];
	int damagelevel = 2;
	if (damage<5) {
		damagelevel = 0;
	} else if (damage<10) {
		damagelevel = 1;
	} else {
		if (chp<-10) {
			damagelevel=3; //chunky death
		}
		else {
			damagelevel = 2;
		}
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
		PlayDamageAnimation(damagelevel);
	}
	DisplayStringCore(this, VB_DAMAGE, DS_CONSOLE|DS_CONST );
	if (InParty) {
		if (chp<(signed) Modified[IE_MAXHITPOINTS]/10) {
			core->Autopause(AP_WOUNDED);
		}
		if (damage>0) {
			core->Autopause(AP_HIT);
		}
	}
	return damage;
}

void Actor::DebugDump()
{
	unsigned int i;

	printf( "Debugdump of Actor %s:\n", LongName );
	for (i = 0; i < MAX_SCRIPTS; i++) {
		const char* poi = "<none>";
		if (Scripts[i] && Scripts[i]->script) {
			poi = Scripts[i]->GetName();
		}
		printf( "Script %d: %s\n", i, poi );
	}
	printf( "Area:       %.8s   ", Area );
	printf( "Dialog:     %.8s\n", Dialog );
	printf( "Global ID:  %d   Local ID:  %d\n", globalID, localID);
	printf( "Script name:%.32s\n", scriptName );
	printf( "TalkCount:  %d   ", TalkCount );
	printf( "PartySlot:  %d\n", InParty );
	printf( "Allegiance: %d   current allegiance:%d\n", BaseStats[IE_EA], Modified[IE_EA] );
	printf( "Morale:     %d   current morale:%d\n", BaseStats[IE_MORALE], Modified[IE_MORALE] );
	printf( "Moralebreak:%d   Morale recovery:%d\n", Modified[IE_MORALEBREAK], Modified[IE_MORALERECOVERYTIME] );
	printf( "Visualrange:%d (Explorer: %d)\n", Modified[IE_VISUALRANGE], Modified[IE_EXPLORE] );
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
/* not too important right now
	ieDword tmp=0;
	core->GetGame()->locals->Lookup("APPEARANCE",tmp);
	printf( "\nDisguise: %d\n", tmp);
*/
	printf ("\nAnimate ID: %x\n", Modified[IE_ANIMATION_ID]);
	printf( "WaitCounter: %d\n", (int) GetWait());
	printf( "LastTarget: %d %s\n", LastTarget, GetActorNameByID(LastTarget));
	printf( "LastTalked: %d %s\n", LastTalkedTo, GetActorNameByID(LastTalkedTo));
	inventory.dump();
	spellbook.dump();
	fxqueue.dump();
}

const char* Actor::GetActorNameByID(ieDword ID)
{
	Actor *actor = GetCurrentArea()->GetActorByGlobalID(ID);
	if (!actor) {
		return "<NULL>";
	}
	return actor->GetScriptName();
}

void Actor::SetMap(Map *map, ieWord LID, ieWord GID)
{
	Scriptable::SetMap(map);
	localID = LID;
	globalID = GID;
}

void Actor::SetPosition(Map *map, Point &position, int jump, int radius)
{
	ClearPath();
	Point p;
	p.x = position.x/16;
	p.y = position.y/12;
	if (jump && !(Modified[IE_DONOTJUMP] & DNJ_FIT) && size ) {
		map->AdjustPosition( p, radius );
	}
	area = map;
	p.x = p.x * 16 + 8;
	p.y = p.y * 12 + 6;
	MoveTo( p );
}

/* this is returning the level of the character for xp calculations 
	 later it could calculate with dual/multiclass, 
	 also with iwd2's 3rd ed rules, this is why it is a separate function */
ieDword Actor::GetXPLevel(int modified) const
{
	if (modified) {
		return Modified[IE_LEVEL];
	}
	return BaseStats[IE_LEVEL];
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
	SetBase(IE_HITPOINTS, BaseStats[IE_MAXHITPOINTS]);
	ClearActions();
	ClearPath();
	SetStance(IE_ANI_EMERGE);
	//clear effects?
}

void Actor::Die(Scriptable *killer)
{
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

	ClearPath();
	SetModal( 0 );
	DisplayStringCore(this, VB_DIE, DS_CONSOLE|DS_CONST );

	//JUSTDIED will be removed when the Die() trigger executed
	//otherwise it is the same as REALLYDIED
	InternalFlags|=IF_REALLYDIED|IF_JUSTDIED;
	SetStance( IE_ANI_DIE );

	if (InParty) {
		game->PartyMemberDied(this);
		core->Autopause(AP_DEAD);
	} else {
		Actor *act=NULL;
		
		if (killer) {
			if (killer->Type==ST_ACTOR) {
				act = (Actor *) killer;
			}
		}
		if (act && act->InParty) {
			//adjust game statistics here
			//game->KillStat(this, killer);
			InternalFlags|=IF_GIVEXP;
		}
	}
	//ensure that the scripts of the actor will run as soon as possible
	ImmediateEvent();
}

void Actor::SetPersistent(int partyslot)
{
	InParty = (ieByte) partyslot;
	InternalFlags|=IF_FROMGAME;
}

void Actor::DestroySelf()
{
	InternalFlags|=IF_CLEANUP;
}

bool Actor::CheckOnDeath()
{
	if (InternalFlags&IF_CLEANUP) return true;
	if (!(InternalFlags&IF_REALLYDIED) ) return false;
	//don't mess with the already deceased
	if (Modified[IE_STATE_ID]&STATE_DEAD) return false;
	//we need to check animID here, if it has not played the death
	//sequence yet, then we could return now
	ClearActions();

	Game *game = core->GetGame();
	if (InternalFlags&IF_GIVEXP) {
		//give experience to party
		game->ShareXP(Modified[IE_XPVALUE], true );
		//handle reputation here
		//
	}

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
			snprintf(varname, 32, "%s_DEAD", scriptName);
			game->kaputz->Lookup(varname, value);
			game->kaputz->SetAt(varname, value+1);
		} else {
			snprintf(varname, 32, "SPRITE_IS_DEAD%s", scriptName);
			game->locals->Lookup(varname, value);
			game->locals->SetAt(varname, value+1);
		}
	}

	DropItem("",0);
	//remove all effects that are not 'permanent after death' here
	//permanent after death type is 9
	SetBaseBit(IE_STATE_ID, STATE_DEAD, true);
	if (Modified[IE_MC_FLAGS]&MC_REMOVE_CORPSE) return true;
	if (Modified[IE_MC_FLAGS]&MC_KEEP_CORPSE) return false;
	//if chunked death, then return true
	if(LastDamageType&DAMAGE_CHUNKING) {
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
/** which is a 'use quickitem' action */
void Actor::GetItemSlotInfo(ItemExtHeader *item, int which)
{
	ieWord idx;
	ieWord headerindex;

	memset(item, 0, sizeof(ItemExtHeader) );
	if (!PCStats) return; //not a player character
	PCStats->GetSlotAndIndex(which,idx,headerindex);
	if (headerindex==0xffff) return; //headerindex is invalid
	CREItem *slot = inventory.GetSlotItem(idx);
	if (!slot) return; //quick item slot is empty
	Item *itm = core->GetItem(slot->ItemResRef);
	if (!itm) return; //quick item slot contains invalid item resref
	ITMExtHeader *ext_header = itm->GetExtHeader(headerindex);
	if (!ext_header) return; //item has no extended header, or header index is wrong
	memcpy(item->itemname, slot->ItemResRef, sizeof(ieResRef) );
	item->headerindex = headerindex;
	memcpy(&(item->AttackType), &(ext_header->AttackType),
 ((char *) &(item->itemname)) -((char *) &(item->AttackType)) );
	if (headerindex>2) {
		item->Charges=0;
	} else {
		item->Charges=slot->Usages[headerindex];
	}
	core->FreeItem(itm,slot->ItemResRef, false);
}

void Actor::ReinitQuickSlots()
{
	if (!PCStats) {
		return;
	}
	int i=sizeof(PCStats->QSlots);
	while (i--) {
		int slot;
		int headerindex = 0xffff;
		int which;
		if (i<0) which = ACT_WEAPON4+i+1;
		else which = PCStats->QSlots[i];
		switch (which) {
			case ACT_WEAPON1:
			case ACT_WEAPON2:
			case ACT_WEAPON3:
			case ACT_WEAPON4:
				slot = inventory.GetWeaponSlot()+(which-ACT_WEAPON1);
				break;
				//WARNING:this cannot be condensed, because the symbols don't come in order!!!
			case ACT_QSLOT1: slot = inventory.GetQuickSlot(); break;
			case ACT_QSLOT2: slot = inventory.GetQuickSlot()+1; break;
			case ACT_QSLOT3: slot = inventory.GetQuickSlot()+2; break;
			case ACT_QSLOT4: slot = inventory.GetQuickSlot()+3; break;
			case ACT_QSLOT5: slot = inventory.GetQuickSlot()+4; break;
			default:;
				slot = 0;
		}
		if (!slot) continue;
		//if magic items are equipped the equipping info doesn't change
		//(afaik)
		if (inventory.HasItemInSlot("", slot)) {
			headerindex = 0;
		} else {
			if (core->QuerySlotEffects(slot)==SLOT_EFFECT_MELEE) {
				slot = inventory.GetFistSlot();
				headerindex = 0;
			} else {
				slot = 0xffff;
			}
		}
		PCStats->InitQuickSlot(which, (ieWord) slot, (ieWord) headerindex);
	}

	//disabling quick weapon slots for certain classes
	for(i=0;i<2;i++) {
		int which = ACT_WEAPON3+i;
		if (PCStats->QSlots[i]!=which) {
			PCStats->InitQuickSlot(which, 0xffff, 0);
		}
	}
}

bool Actor::ValidTarget(int ga_flags)
{
	if (Immobile()) return false;
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
		if (Modified[IE_UNSELECTABLE]) return false;
	}
	return true;
}

//returns true if it won't be destroyed with an area
//in this case it shouldn't be saved with the area either
//it will be saved in the savegame
bool Actor::Persistent()
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
	SetAnimationID ( NewAnimID );
}

//slot is the projectile slot
int Actor::GetRangedWeapon(ITMExtHeader *&which)
{
	unsigned int slot = inventory.FindRangedWeapon();
	CREItem *wield = inventory.GetSlotItem(slot);
	if (!wield) {
		return 0;
	}
	Item *item = core->GetItem(wield->ItemResRef);
	if (!item) {
		return 0;
	}
	which = item->GetWeaponHeader(true);
	core->FreeItem(item, wield->ItemResRef, false);
	return 0;
}

//returns weapon header currently used
//if range is nonzero, then the returned header is valid
unsigned int Actor::GetWeapon(ITMExtHeader *&which, bool leftorright)
{
	CREItem *wield = inventory.GetUsedWeapon(leftorright);
	if (!wield) {
		return 0;
	}
	Item *item = core->GetItem(wield->ItemResRef);
	if (!item) {
		return 0;
	}

	//select first weapon header
	which = item->GetWeaponHeader(false);
	//make sure we use 'false' in this freeitem
	//so 'which' won't point into invalid memory
	core->FreeItem(item, wield->ItemResRef, false);
	if (!which) {	
		return 0;
	}
	if (which->Location!=ITEM_LOC_WEAPON) {
		return 0;
	}
	return which->Range+1;
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
	Spell *spell = core->GetSpell(spellname);
	if (!spell) {
		return LSR_INVALID; //not existent spell
	}
	//from now on, you must delete spl if you don't push back it
	CREKnownSpell *spl = new CREKnownSpell();
	strncpy(spl->SpellResRef, spellname, 8);
	spl->Type = spell->SpellType;
	if ( spl->Type == IE_SPELL_TYPE_INNATE ) {
		spl->Level = 0;
	}
	else {
		spl->Level = (ieWord) (spell->SpellLevel-1);
	}
	bool ret=spellbook.AddKnownSpell(spl->Type, spl->Level, spl);
	if (!ret) {
		delete spl;
		return LSR_INVALID;
	}
	if (flags&LS_ADDXP) {
		AddExperience(spl->Level*100);		
	}
	return LSR_OK;
}

const char *Actor::GetDialog(bool checks) const
{
	if (!checks) {
		return Dialog;
	}
	if (Modified[IE_EA]>=EA_EVILCUTOFF) {
		return NULL;
	}

	if ( (InternalFlags & IF_NOINT) && CurrentAction) {
		core->DisplayConstantString(STR_TARGETBUSY,0xff0000);
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
	SetWait( 1 );
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

int Actor::Immobile()
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
	attackcount = 0;

	if (InternalFlags&IF_STOPATTACK) {
		core->GetGame()->OutAttack(GetID());
		return;
	}

	if (!LastTarget) {
		StopAttack();
		return;
	}
 
	//if held or disabled, etc, then cannot continue attacking
	ieDword state = GetStat(IE_STATE_ID);
	if (state&STATE_CANTMOVE) {
		return;
	}
	if (Immobile()) {
		return;
	}

	SetStance(IE_ANI_ATTACK);
	//last chance to disable attacking
	//
	attackcount = GetStat(IE_NUMBEROFATTACKS);
	if (secondround) {
		attackcount++;
	}
	attackcount/=2;

	//d10
	int tmp = core->Roll(1, 10, 0);// GetStat(IE_WEAPONSPEED)-GetStat(IE_PHYSICALSPEED) );
	if (state & STATE_SLOWED) tmp <<= 1;
	if (state & STATE_HASTED) tmp >>= 1;
	
	if (tmp<0) tmp=0;
	else if(tmp>0x10) tmp=0x10;
	
	initiative = (ieDword) (gameTime+tmp);
}

int Actor::GetToHit(int bonus, ieDword Flags)
{
	int tohit = GetStat(IE_TOHIT);
	if (REVERSE_TOHIT) {
		tohit = ATTACKROLL-tohit;
	}
	tohit += bonus;

	if (Flags&WEAPON_LEFTHAND) {
		tohit += GetStat(IE_HITBONUSLEFT);
	} else {
		tohit += GetStat(IE_HITBONUSRIGHT);
	}
	//get attack style (melee or ranged)
	switch(Flags&WEAPON_STYLEMASK) {
		case WEAPON_MELEE:
			tohit += GetStat(IE_MELEEHIT);
			break;
		case WEAPON_FIST:
			tohit += GetStat(IE_FISTHIT);
			break;
		case WEAPON_RANGED:
			tohit += GetStat(IE_MISSILEHITBONUS);
			//add dexterity bonus
			break;
	}
	//add strength bonus if we need
	if (Flags&WEAPON_USESTRENGTH) {
		tohit += core->GetStrengthBonus(0,GetStat(IE_STR), GetStat(IE_STREXTRA) );
	}
	return tohit;
}

void Actor::PerformAttack(ieDword gameTime)
{
	if (!attackcount) {
		if (initiative) {
			if (InParty) {
				core->Autopause(AP_ENDROUND);
			}
			initiative = 0;
		}
		return;
	}
	if (initiative>gameTime) {
		return;
	}
	attackcount--;
	if (!LastTarget) {
		StopAttack();
		return;
	}
	//get target
	Actor *target = area->GetActorByGlobalID(LastTarget);

	if (target && (target->GetStat(IE_STATE_ID)&STATE_DEAD)) {
		target = NULL;
	}

	if (!target) {
		LastTarget = 0;
		return;
	}
	//which hand is used
	bool leftorright = (bool) (attackcount&1);
	ITMExtHeader *header;
	//can't reach target, zero range shouldn't be allowed
	if (GetWeapon(header,leftorright)*10<PersonalDistance(this, target)+1) {
		return;
	}
	ieDword Flags;
	ITMExtHeader *rangedheader = NULL;
	switch(header->AttackType) {
	case ITEM_AT_MELEE:
		Flags = WEAPON_MELEE;		
		break;
	case ITEM_AT_PROJECTILE: //throwing weapon
		Flags = WEAPON_RANGED;
		break;
	case ITEM_AT_BOW:
		if (!GetRangedWeapon(rangedheader)) {
			//out of ammo event
			//try to refill
			SetStance(IE_ANI_READY);
			return;
		}
		SetStance(IE_ANI_READY);
		return;
	default:
		//item is unsuitable for fight
		SetStance(IE_ANI_READY);
		return;
	}//melee or ranged
	if (leftorright) Flags|=WEAPON_LEFTHAND;
	if (header->RechargeFlags&IE_ITEM_USESTRENGTH) Flags|=WEAPON_USESTRENGTH;

	//second parameter is left or right hand flag
	int tohit = GetToHit(header->THAC0Bonus, Flags);

	int roll = core->Roll(1,ATTACKROLL,0);
	if (roll==1) {
		//critical failure
		return;
	}
	//damage type is?
	//modify defense with damage type
	ieDword damagetype = header->DamageType;
	int damage = core->Roll(header->DiceThrown, header->DiceSides, header->DamageBonus);
	int damageluck = (int) GetStat(IE_DAMAGELUCK);
	if (damage<damageluck) {
		damage = damageluck;
	}

	if (roll>=ATTACKROLL-(int) GetStat(IE_CRITICALHITBONUS) ) {
		//critical success
		DealDamage (target, damage, damagetype, true);
		return;
	}
	tohit += roll;

	//get target's defense against attack
	int defense = target->GetStat(IE_ARMORCLASS);
	defense += core->GetDexterityBonus(STAT_DEX_AC, target->GetStat(IE_DEX) );
	if (REVERSE_TOHIT) {
		defense = DEFAULTAC - defense;
	}

	if (tohit<defense) {
		//hit failed
		return;
	}
	DealDamage (target, damage, damagetype, false);
}

static int weapon_damagetype[] = {DAMAGE_CRUSHING, DAMAGE_PIERCING,
	DAMAGE_CRUSHING, DAMAGE_SLASHING, DAMAGE_MISSILE, DAMAGE_STUNNING};

void Actor::DealDamage(Actor *target, int damage, int damagetype, bool critical)
{
	if (damage<0) damage = 0;
	if (critical) {
		//a critical surely raises the morale?
		NewBase(IE_MORALE, 1, MOD_ADDITIVE);
		damage <<=1; //critical damage is always double?
		//check if critical hit is averted by helmet
	}
	if (damagetype>4) damagetype = 0;
	target->Damage(damage, weapon_damagetype[damagetype], this);
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
/*
	if (anims) {
		anims->SetColors(Modified+IE_COLORS);
	}
*/
}

void Actor::SetLeader(Actor *actor, int xoffset, int yoffset)
{
	Leader = actor->GetID();
	XF = xoffset;
	YF = yoffset;
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

void Actor::RemoveTimedEffects()
{
}

bool Actor::Schedule(ieDword gametime)
{
	if (!(InternalFlags&IF_VISIBLE) ) {
		return false;
	}

	//check for schedule
	ieDword bit = 1<<(gametime%7200/300);
	if (appearance & bit) {
		return true;
	}
	return false;
}

void Actor::NewPath()
{
	Point tmp = Destination;
	ClearPath();
	Moveble::WalkTo(tmp, size );
}

void Actor::WalkTo(Point &Des, ieDword flags, int MinDistance)
{
	if (InternalFlags&IF_REALLYDIED) {
		return;
	}
	InternalFlags &= ~IF_RUNFLAGS;
	InternalFlags |= (flags & IF_RUNFLAGS);
	// is this true???
	if (Des.x==-2 && Des.y==-2) {
		Point p((ieWord) Modified[IE_SAVEDXPOS], (ieWord) Modified[IE_SAVEDYPOS] );
		Moveble::WalkTo(p, MinDistance);
	} else {
		Moveble::WalkTo(Des, MinDistance);
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
		bool endReached = vvc->Draw(screen, Pos, tint, area, WantDither());
		if (endReached) {
			delete vvc;
			vvcCells.erase(vvcCells.begin()+i);
			continue;
		}
	}
}

void Actor::Draw(Region &screen)
{
	Map* area = GetCurrentArea();

	int cx = Pos.x;
	int cy = Pos.y;
	int explored = Modified[IE_DONOTJUMP]&DNJ_UNHINDERED;
	//check the deactivation condition only if needed
	//this fixes dead actors disappearing from fog of war (they should be permanently visible)
	if ((!area->IsVisible( Pos, explored) || (InternalFlags&IF_JUSTDIED) ) &&	(InternalFlags&IF_ACTIVE) ) {
		//finding an excuse why we don't hybernate the actor
		if (Modified[IE_ENABLEOFFSCREENAI])
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

	//visual feedback
	CharAnimations* ca = GetAnims();
	if (!ca)
		return;

	if (Modified[IE_AVATARREMOVAL]) {
		return;
	}

	//explored or visibilitymap (bird animations are visible in fog)
	//0 means opaque
	int Trans = Modified[IE_TRANSLUCENT];
	//int Trans = Modified[IE_TRANSLUCENT] * 255 / 100;
	if (Trans>255) {
		Trans=255;
	}
	int Frozen = Immobile();
	int State = Modified[IE_STATE_ID];
	if (State&STATE_STILL) {
		Frozen = 1;
	}

	if (State&STATE_INVISIBLE) {
		//enemies/neutrals are fully invisible if invis flag 2 set
		if (Modified[IE_EA]>EA_GOODCUTOFF) {
			if (State&STATE_INVIS2)
				Trans=256;
			else
				Trans=128;
		} else {
			Trans=256;
		}
	}
	//friendlies are half transparent at best
	if (Trans>128) {
		if (Modified[IE_EA]<=EA_GOODCUTOFF) {
			Trans=128;
		}
	}
	//no visual feedback
	if (Trans>255) {
		return;
	}

	if (State&STATE_BLUR) {
		if (Trans>192) Trans = 192;
	}

	Color tint = area->LightMap->GetPixel( cx / 16, cy / 12);
	tint.a = (ieByte) (255-Trans);

	//draw videocells under the actor
	DrawVideocells(screen, vvcShields, tint);

	Video* video = core->GetVideoDriver();
	Region vp = video->GetViewport();

	if (( !Modified[IE_NOCIRCLE] ) && ( !( State & STATE_DEAD ) )) {
		DrawCircle(vp);
		DrawTargetPoint(vp);
	}
	
	unsigned char StanceID = GetStance();
	Animation** anims = ca->GetAnimation( StanceID, GetNextFace() );
	if (anims) {
		int PartCount = ca->GetPartCount();
		Animation* masteranim = anims[0];
		for (int part = 0; part < PartCount; ++part) {
			Animation* anim = anims[part];
			Sprite2D* nextFrame;
			if (part == 0) {
				if (Frozen) {
					nextFrame = anim->LastFrame();
					if (Selected!=0x80) {
						Selected = 0x80;
						core->GetGame()->SelectActor(this, false, SELECT_NORMAL);
					}
				} else {
					nextFrame = anim->NextFrame();
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
						// of the real bounding box of this frame.
						// Shouldn't matter much, though. (wjp)
						newBBox.x = cx + masteranim->animArea.x;
						newBBox.y = cy + masteranim->animArea.y;
						newBBox.w = masteranim->animArea.w;
						newBBox.h = masteranim->animArea.h;
					}
					lastFrame = nextFrame;
					SetBBox( newBBox );
				}
			} else {
				nextFrame = anim->GetSyncedNextFrame(masteranim);
			}
			if (nextFrame && BBox.InsideRegion( vp ) ) {
				SpriteCover* sc = GetSpriteCover();
				if (!sc || !sc->Covers(cx, cy, nextFrame->XPos, nextFrame->YPos, nextFrame->Width, nextFrame->Height)) {
					// the masteranim contains the animarea for
					// the entire multi-part animation
					sc = area->BuildSpriteCover(cx, cy, -masteranim->animArea.x, -masteranim->animArea.y, masteranim->animArea.w, masteranim->animArea.h, WantDither() );
					//this will delete previous spritecover
					SetSpriteCover(sc);

				}
				assert(sc->Covers(cx, cy, nextFrame->XPos, nextFrame->YPos, nextFrame->Width, nextFrame->Height));

				unsigned int flags = TranslucentShadows ? BLIT_TRANSSHADOW : 0;
				if (!ca->lockPalette) flags|=BLIT_TINTED;

				video->BlitGameSprite( nextFrame, cx + screen.x, cy + screen.y,
					 flags, tint, sc, ca->palette, &screen);
			}
		}
		if (masteranim->endReached) {
			if (HandleActorStance() ) {
				masteranim->endReached = false;
			}
		}
	}
		
	//draw videocells over the actor
	DrawVideocells(screen, vvcOverlays, tint);

	//text feedback
	DrawOverheadText(screen);
}

/* Handling automatic stance changes */
bool Actor::HandleActorStance()
{
	CharAnimations* ca = GetAnims();
	int StanceID = GetStance();

	int x = rand()%1000;
	if (ca->autoSwitchOnEnd) {
		SetStance( ca->nextStanceID );
		return true;
	}
	if ((StanceID==IE_ANI_AWAKE) && !x ) {
		SetStance( IE_ANI_HEAD_TURN );
		return true;
	}
	if ((StanceID==IE_ANI_READY) && !GetNextAction()) {
		SetStance( IE_ANI_AWAKE );
		return true;
	}
	return false;
}

void Actor::ResolveStringConstant(ieResRef Sound, unsigned int index)
{
	TableMgr * tab;

	//resolving soundset (bg1/bg2 style)
	if (PCStats && PCStats->SoundSet[0]&& csound[index]) {
		snprintf(Sound, sizeof(ieResRef), "%s%c", PCStats->SoundSet, csound[index]);
		return;
	}

	Sound[0]=0;
	int table=core->LoadTable( anims->ResRef );

	if (table<0) {
		return;
	}
	tab = core->GetTable( table );
	if (!tab) {
		goto end;
	}

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
end:
	core->DelTable( table );

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
	if (PCStats->QSlots[0]==0xff) {
		InitButtons(GetStat(IE_CLASS));
	}
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
			strnlwrcpy(PCStats->SoundSet, fp, 6);
			free(fp);
		}
	} else {
		strnlwrcpy(PCStats->SoundSet, soundset, 6);
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
			typemask = ~1;
			break;
		case 1: //allow only cleric
			typemask = ~2;
			break;
		default:
			typemask = 0;
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
int Actor::SetEquippedQuickSlot(int slot)
{
	//creatures and such
	if (!PCStats) {
		if (inventory.SetEquippedSlot(slot)) {
			return 0;
		}
		return STR_MAGICWEAPON;
	}

	//player characters
	if (inventory.SetEquippedSlot(PCStats->QuickWeaponSlots[slot]-inventory.GetWeaponSlot())) {
		return 0;
	}
	return STR_MAGICWEAPON;
}

bool Actor::UseItem(int slot, Scriptable* /*target*/)
{
	CREItem *item = inventory.GetSlotItem(slot);
	if (!item)
		return false;
	Item *itm = core->GetItem(item->ItemResRef);
	if (!itm) return false; //quick item slot contains invalid item resref
	//
	//
	core->FreeItem(itm,item->ItemResRef, false);
	return true;
}

bool Actor::IsReverseToHit()
{
	return REVERSE_TOHIT;
}

void Actor::InitButtons(ieDword cls)
{
	if (!PCStats) {
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
	if (feat>3*sizeof(ieDword)) {
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

int Actor::GetFeat(unsigned int feat)
{
	if (feat>3*sizeof(ieDword)) {
		return -1;
	}
	if (Modified[IE_FEATS1+(feat>>5)]&(1<<(feat&31)) ) {
		return 1;
	}
	return 0;
}

