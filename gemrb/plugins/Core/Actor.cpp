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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Actor.cpp,v 1.174 2006/04/04 21:59:42 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "TableMgr.h"
#include "ResourceMgr.h"
#include "Actor.h"
#include "Interface.h"
#include "../../includes/strrefs.h"
#include "Item.h"
#include "Spell.h"
#include "Game.h"
#include "GameScript.h"
#include "GSUtils.h"    //needed for DisplayStringCore
#include "Video.h"
#include "SpriteCover.h"
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

static char iwd2gemrb[32]={
	0,0,20,2,22,25,0,14,
	15,23,13,0,1,24,8,21,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0
};
static char gemrb2iwd[32]={
	11,12,3,71,72,73,0,0,  //0
	14,80,83,82,81,10,7,8,  //8
	0,0,0,0,2,15,4,9,  //16
	13,5,0,0,0,0,0,0   //24
};

static void InitActorTables();

static ieDword TranslucentShadows;

#define DAMAGE_LEVELS 13
static ieResRef d_main[DAMAGE_LEVELS]={
	"BLOODS","BLOODM","BLOODL","BLOODCR", //blood
	"SPFIRIMP","SPFIRIMP","SPFIRIMP",     //fire
	"SPSHKIMP","SPSHKIMP","SPSHKIMP",     //spark
	"SPFIRIMP","SPFIRIMP","SPFIRIMP",     //ice
};
static ieResRef d_splash[DAMAGE_LEVELS]={
	"","","","",
	"SPBURN","SPBURN","SPBURN",   //flames
	"SPSPARKS","SPSPARKS","SPSPARKS", //sparks
	"","","",
};

#define BLOOD_GRADIENT 19
#define FIRE_GRADIENT 19
#define ICE_GRADIENT 71

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

PCStatsStruct::PCStatsStruct()
{
	BestKilledName = 0xffffffff;
	BestKilledXP = 0;
	AwayTime = 0;
	JoinDate = 0;
	unknown10 = 0;
	KillsChapterXP = 0;
	KillsChapterCount = 0;
	KillsTotalXP = 0;
	KillsTotalCount = 0;
	memset( FavouriteSpells, 0, sizeof(FavouriteSpells) );
	memset( FavouriteSpellsCount, 0, sizeof(FavouriteSpellsCount) );
	memset( FavouriteWeapons, 0, sizeof(FavouriteWeapons) );
	memset( FavouriteWeaponsCount, 0, sizeof(FavouriteWeaponsCount) );
	SoundSet[0]=0;
	SoundFolder[0]=0;
	QSlots[0]=0xff;
	memset( QuickSpells, 0, sizeof(QuickSpells) );
	memset( QuickSpellClass, 0xff, sizeof(QuickSpellClass) );
	memset( QuickItemSlots, 0, sizeof(QuickItemSlots) );
	memset( QuickWeaponSlots, 0, sizeof(QuickWeaponSlots) );
}

void PCStatsStruct::IncrementChapter()
{
	KillsChapterXP = 0;
	KillsChapterCount = 0;
}

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
	LastTarget = 0;
	LastTalkedTo = 0;
	LastAttacker = 0;
	LastHitter = 0;
	LastProtected = 0;
	LastCommander = 0;
	LastHelp = 0;
	LastSeen = 0;
	LastHeard = 0;
	LastSummoner = 0;
	PCStats = NULL;
	LastCommand = 0; //used by order
	LastShout = 0; //used by heard
	LastDamage = 0;
	LastDamageType = 0;
	HotKey = 0;

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
	if (anims) {
		delete( anims );
	}
	//hacking PST no palette
	if (core->HasFeature(GF_ONE_BYTE_ANIMID) )
	{
		if (AnimID&0x8000) {
			if (BaseStats[IE_COLORCOUNT]) {
				printf("[Actor] Animation ID %x is supposed to be real colored (no recoloring), patched creature\n", AnimID);
			}
			BaseStats[IE_COLORCOUNT]=0;
		}
	}
	anims = new CharAnimations( AnimID, BaseStats[IE_ARMOR_TYPE]);
	if (anims) {
		//bird animations are not hindered by searchmap
		//only animtype==7 (bird) uses this feature
		//this is a hardcoded hack, but works for all engine type
		if (anims->GetAnimType()!=IE_ANI_BIRD) {
			BaseStats[IE_DONOTJUMP]=0;
		} else {
			BaseStats[IE_DONOTJUMP]=3;
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
	} else if (GetMod(IE_MORALE)<0) {//if current morale < the max morale ?
		color = &yellow;
		color_index = 5;
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

void pcf_ea(Actor *actor, ieDword /*Value*/)
{
	actor->SetCircleSize();
}

void pcf_animid(Actor *actor, ieDword Value)
{
	actor->SetAnimationID(Value);
}

void pcf_hitpoint(Actor *actor, ieDword Value)
{
	if ((signed) Value>(signed) actor->Modified[IE_MAXHITPOINTS]) {
		Value=actor->Modified[IE_MAXHITPOINTS];
	}
	if ((signed) Value<(signed) actor->Modified[IE_MINHITPOINTS]) {
		Value=actor->Modified[IE_MINHITPOINTS];
	}
	if ((signed) Value+core->GetConstitutionBonus(0,actor->Modified[IE_CON])<=0) {
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
		game->AddGold ( actor->Modified[IE_GOLD] );
		actor->Modified[IE_GOLD]=0;
	}
	//additionally to party pool, gold changes are permanent
	//but this should be enforced by the effect system anyway
	//actor->BaseStats[IE_GOLD]=actor->Modified[IE_GOLD];
}

//this should be fixed one day
static Point dummypoint;

//de/activates the entangle overlay
void pcf_entangle(Actor *actor, ieDword Value)
{
	if (Value) {
		if (actor->HasVVCCell(overlay[OV_ENTANGLE]))
			return;
		ScriptedAnimation *sca = core->GetScriptedAnimation(overlay[OV_ENTANGLE], dummypoint,0);
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
		if (actor->HasVVCCell(overlay[OV_SANCTUARY]))
			return;
		ScriptedAnimation *sca = core->GetScriptedAnimation(overlay[OV_SANCTUARY], dummypoint,0);
		sca->Transparency|=IE_VVC_TRANSPARENT;
		actor->AddVVCell(sca);
	} else {
		actor->RemoveVVCell(overlay[OV_SANCTUARY], true);
	}
}

//de/activates the prot from missiles overlay
void pcf_shieldglobe(Actor *actor, ieDword Value)
{
	if (Value) {
		if (actor->HasVVCCell(overlay[OV_SHIELDGLOBE]))
			return;
		ScriptedAnimation *sca = core->GetScriptedAnimation(overlay[OV_SHIELDGLOBE], dummypoint,0);
		sca->Transparency|=IE_VVC_TRANSPARENT;
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
		ScriptedAnimation *sca = core->GetScriptedAnimation(overlay[OV_MINORGLOBE], dummypoint,0);
		sca->Transparency|=IE_VVC_TRANSPARENT;
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
		actor->add_animation(overlay[OV_GREASE], dummypoint, -1, -1);
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
		actor->add_animation(overlay[OV_WEB], dummypoint, -1, 0);
	} else {
		actor->RemoveVVCell(overlay[OV_WEB], true);
	}
}

//de/activates the grease background
void pcf_bounce(Actor *actor, ieDword Value)
{
	switch(Value) {
	case 1: //bounce passive
		if (actor->HasVVCCell(overlay[OV_BOUNCE]))
			return;
		actor->add_animation(overlay[OV_BOUNCE], dummypoint, -1, 0);
		break;
	case 2: //activated bounce
		if (actor->HasVVCCell(overlay[OV_BOUNCE2]))
			return;
		actor->add_animation(overlay[OV_BOUNCE2], dummypoint, -1, 0);
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
0,0,0,0,0,0,0,25,25,255,255,255,255,255,65535,-1,//cf
200,200,200,200,200,200,200,-1,-1,255,65535,3,255,255,255,255,//df
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,//ef
255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255//ff
};

typedef void (*PostChangeFunctionType)(Actor *actor, ieDword Value);
static PostChangeFunctionType post_change_functions[256]={
pcf_hitpoint, pcf_maxhitpoint, NULL, NULL, NULL, NULL, NULL, NULL,
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL, //0f
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL,
NULL,NULL,NULL,NULL, NULL, NULL, pcf_fatigue, pcf_intoxication, //1f
NULL,NULL,NULL,NULL, pcf_stat, NULL, pcf_stat, pcf_stat,
pcf_stat,pcf_con,NULL,NULL, NULL, pcf_gold, NULL, NULL, //2f
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
NULL,NULL,NULL,NULL, NULL, pcf_bounce, NULL, NULL, 
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL, //bf
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL,
NULL,NULL,NULL,NULL, pcf_ea, NULL, pcf_animid, NULL, //cf
pcf_color,pcf_color,pcf_color,pcf_color, pcf_color, pcf_color, pcf_color, NULL,
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL, //df
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL,
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL, //ef
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
}
//TODO: every actor must have an own vvcell list
//the area's vvc's are the stationary effects
//the actor's vvc's are moving with the actor
//this method adds a vvc to the actor
void Actor::add_animation(const ieResRef resource, Point &offset, int gradient, int height)
{
	ScriptedAnimation *sca = core->GetScriptedAnimation(resource, offset, height);
	if (gradient!=-1) {
		sca->SetPalette(gradient, 4);
	}
	AddVVCell(sca);
}

void Actor::PlayDamageAnimation(int type)
{
	int i;
	Point p(0,0);

	switch(type) {
		case 0: case 1: case 2: case 3: //blood
			add_animation(d_main[type], p, d_gradient[type], 0);
			break;
		case 4: case 5: case 7: //fire
			add_animation(d_main[type], p, d_gradient[type], 0);
			for(i=DL_FIRE;i<=type;i++) {
				add_animation(d_splash[i], p, d_gradient[i], 0);
			}
			break;
		case 8: case 9: case 10: //electricity
			add_animation(d_main[type], p, d_gradient[type], 0);
			for(i=DL_ELECTRICITY;i<=type;i++) {
				add_animation(d_splash[i], p, d_gradient[i], 0);
			}
			break;
		case 11: case 12: case 13://cold
			add_animation(d_main[type], p, d_gradient[type], 0);
			break;
		case 14: case 15: case 16://acid
			add_animation(d_main[type], p, d_gradient[type], 0);
			break;
		case 17: case 18: case 19://disintegrate
			add_animation(d_main[type], p, d_gradient[type], 0);
			break;
	}
}

bool Actor::SetStat(unsigned int StatIndex, ieDword Value, bool pcf)
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
bool Actor::SetBase(unsigned int StatIndex, ieDword Value)
{
	if (StatIndex >= MAX_STATS) {
		return false;
	}
	BaseStats[StatIndex] = Value;
	SetStat (StatIndex, Value, true);
	return true;
}
/** call this after load, to apply effects */
void Actor::Init(bool first)
{
	ieDword previous[MAX_STATS];

	if (!first) {
		memcpy( previous, Modified, MAX_STATS * sizeof( *Modified ) );
	}
	memcpy( Modified, BaseStats, MAX_STATS * sizeof( *Modified ) );
	fxqueue.ApplyAllEffects( this );
	for (unsigned int i=0;i<MAX_STATS;i++) {
		if (first || Modified[i]!=previous[i]) {
			PostChangeFunctionType f = post_change_functions[i];
			if (f) {
				(*f)(this, Modified[i]);
			}
		}
	}
}

/** implements a generic opcode function, modify modifier
	returns the change
*/
int Actor::NewStat(unsigned int StatIndex, ieDword ModifierValue, ieDword ModifierType)
{
	int oldmod = Modified[StatIndex];

	switch (ModifierType) {
		case MOD_ADDITIVE:
			//flat point modifier
			SetStat(StatIndex, Modified[StatIndex]+ModifierValue, false);
			break;
		case MOD_ABSOLUTE:
			//straight stat change
			SetStat(StatIndex, ModifierValue, false);
			break;
		case MOD_PERCENT:
			//percentile
			SetStat(StatIndex, BaseStats[StatIndex] * 100 / ModifierValue, false);
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
			SetBase(StatIndex, BaseStats[StatIndex] * 100 / ModifierValue);
			break;
	}
	return BaseStats[StatIndex] - oldmod;
}

void Actor::ReactToDeath(const char * /*deadname*/)
{
	// lookup value based on died's scriptingname and ours
	// if value is 0 - use reactdeath
	// if value is 1 - use reactspecial
	// if value is string - use playsound instead (pst)
	
	DisplayStringCore(this, VB_REACT, DS_CONSOLE|DS_CONST );
}

//call this only from gui selects
void Actor::SelectActor()
{
	DisplayStringCore(this, VB_SELECT, DS_CONSOLE|DS_CONST );
}

void Actor::Panic()
{
	SetBase(IE_MORALE,0);
	SetBase(IE_STATE_ID,GetStat(IE_STATE_ID)|STATE_PANIC);
	DisplayStringCore(this, VB_PANIC, DS_CONSOLE|DS_CONST );
}

void Actor::SetMCFlag(ieDword arg, int op)
{
	ieDword tmp = GetBase(IE_MC_FLAGS);
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
	if( GetStat(IE_MC_FLAGS)&MC_NO_TALK)
		return;
	if (GetStat(IE_EA)>=EA_EVILCUTOFF) {
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
	NewStat(IE_HITPOINTS, (ieDword) -damage, damagetype&3);
	NewStat(IE_MORALE, (ieDword) -1, MOD_ADDITIVE);
	//this is just a guess, probably morale is much more complex
	if(GetStat(IE_MORALE)<GetStat(IE_MORALEBREAK) ) {
		Panic();
	}
	LastDamageType=damagetype;
	LastDamage=damage;
	LastHitter=hitter->GetID();
	InternalFlags|=IF_ACTIVE;
	int damagelevel = 2;
	if (damage<5) {
		damagelevel = 0;
	} else if (damage<10) {
		damagelevel = 1;
	} else {
		if (((signed) GetStat(IE_HITPOINTS))<-10) {
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
		if (GetStat(IE_HITPOINTS)<GetStat(IE_MAXHITPOINTS)/10) {
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

//Last* ids should all be made ieWord?
const char* Actor::GetActorNameByID(ieWord ID)
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
	if (jump && !(GetStat( IE_DONOTJUMP )&DNJ_FIT) && size ) {
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
int Actor::GetXPLevel(int modified) const
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
void Actor::Turn(Scriptable *cleric, int turnlevel)
{
	//this is safely hardcoded i guess
	if (GetStat(IE_GENERAL)!=GEN_UNDEAD) {
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
	InternalFlags&=IF_FROMGAME;		 //keep these flags
	InternalFlags|=IF_ACTIVE|IF_VISIBLE;  //set these flags  
	SetBase(IE_STATE_ID, 0);
	SetBase(IE_HITPOINTS, GetBase(IE_MAXHITPOINTS));
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
	core->GetGame()->SelectActor(this, false, SELECT_NORMAL);
	ClearPath();
	SetModal( 0 );
	DisplayStringCore(this, VB_DIE, DS_CONSOLE|DS_CONST );

	//JUSTDIED will be removed when the Die() trigger executed
	//otherwise it is the same as REALLYDIED
	InternalFlags|=IF_REALLYDIED|IF_JUSTDIED;
	SetStance( IE_ANI_DIE );

	Game *game = core->GetGame();
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
		game->ShareXP(GetStat(IE_XPVALUE), true );
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
		char varname[33];
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
	BaseStats[IE_STATE_ID] |= STATE_DEAD;
	if (Modified[IE_MC_FLAGS]&MC_REMOVE_CORPSE) return true;
	if (Modified[IE_MC_FLAGS]&MC_KEEP_CORPSE) return false;
	//if chunked death, then return true
	return false;
}

/* this will create a heap at location, and transfer the item(s) */
void Actor::DropItem(const ieResRef resref, unsigned int flags)
{
	inventory.DropItemAtLocation( resref, flags, area, Pos );
}

void Actor::DropItem(int slot , unsigned int flags)
{
	inventory.DropItemAtLocation( slot, flags, area, Pos );
}
bool Actor::ValidTarget(int ga_flags)
{
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

int Actor::GetWeaponRange()
{
	CREItem *wield = inventory.GetUsedWeapon();
	if ( !wield) {
		//should return FIST if not wielding anything
		return 0;
	}
	Item *item = core->GetItem(wield->ItemResRef);
	if (!item) {
		return 0;
	}
	ITMExtHeader * which = item->GetExtHeader(0);
	if (!which) {
		core->FreeItem(item, wield->ItemResRef, false);
		return 0;
	}
	core->FreeItem(item, wield ->ItemResRef, false);
	return which->Range;
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
	if (GetStat(IE_EA)>=EA_EVILCUTOFF) {
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
	return 1;
}

void Actor::SetTarget( Scriptable *target)
{
	if (target->Type==ST_ACTOR) {
		Actor *tar = (Actor *) target;
		LastTarget = tar->GetID();
		tar->LastAttacker = GetID();
	}
	//calculate attack style
	//set stance correctly based on attack style
	SetOrientation( GetOrient( target->Pos, Pos ), false );
	SetStance( IE_ANI_ATTACK);
	SetWait( 1 );
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
		value = gradient << shift;
		value |= Modified[IE_COLORS+index] & ~(255<<shift);
		Modified[IE_COLORS+index] = value;
	}

	if (anims) {
		anims->SetColors(Modified+IE_COLORS);
	}
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
		SetBase(IE_HITPOINTS, GetBase(IE_HITPOINTS)+days*2);
	} else {
		SetBase(IE_HITPOINTS, GetStat(IE_MAXHITPOINTS));
	}
}

//this function should handle dual classing and multi classing
void Actor::AddExperience(int exp)
{
	SetBase(IE_XP,GetBase(IE_XP)+exp);
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

void Actor::WalkTo(Point &Des, ieDword flags, int MinDistance)
{
	if (InternalFlags&IF_REALLYDIED) {
		return;
	}
	InternalFlags &= ~IF_RUNFLAGS;
	InternalFlags |= (flags & IF_RUNFLAGS);
	// is this true???
	if (Des.x==-2 && Des.y==-2) {
		Point p(GetStat(IE_SAVEDXPOS), GetStat(IE_SAVEDYPOS) );
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
		if (!vvc)
			continue;

		// actually this is better be drawn by the vvc
		bool endReached = vvc->Draw(screen, Pos, tint, area, WantDither());
		if (endReached) {
			vvcCells[i] = NULL;
			delete( vvc );
			continue;
		}
	}
}


void Actor::Draw(Region &screen)
{
	Map* area = GetCurrentArea();

	int cx = Pos.x;
	int cy = Pos.y;
	int explored = Modified[IE_DONOTJUMP]&2;
	//check the deactivation condition only if needed
	//this fixes dead actors disappearing from fog of war (they should be permanently visible)
	if ((!area->IsVisible( Pos, explored) || (InternalFlags&IF_JUSTDIED) ) &&	(InternalFlags&IF_ACTIVE) ) {
		//finding an excuse why we don't hybernate the actor
		if (Modified[IE_ENABLEOFFSCREENAI])
			return;
		if (CurrentAction)
			return;
		if (path)
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

	//explored or visibilitymap (bird animations are visible in fog)
	//0 means opaque
	int Trans = Modified[IE_TRANSLUCENT];
	//int Trans = Modified[IE_TRANSLUCENT] * 255 / 100;
	if (Trans>255) {
		Trans=255;
	}
	int State = Modified[IE_STATE_ID];
	if (State&STATE_INVISIBLE) {
		//enemies/neutrals are fully invisible if invis flag 2 set
		if (GetStat(IE_EA)>EA_GOODCUTOFF) {
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
		if (GetStat(IE_EA)<=EA_GOODCUTOFF) {
			Trans=128;
		}
	}
	//no visual feedback
	if (Trans>255) {
		return;
	}

	Color tint = area->LightMap->GetPixel( cx / 16, cy / 12);
	tint.a = 255-Trans;

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
				nextFrame = anim->NextFrame();
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

				video->BlitGameSprite( nextFrame, cx + screen.x, cy + screen.y,
					 BLIT_TINTED | (TranslucentShadows ? BLIT_TRANSSHADOW : 0),
					 tint, sc, ca->palette, &screen);
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

//the first 3 buttons are untouched by this function
void Actor::GetActionButtonRow(ActionButtonRow &ar, int translation)
{
	if (PCStats->QSlots[0]==0xff) {
		for(int i=0;i<GUIBT_COUNT-3;i++) {
			ieByte tmp = ar[i+3];
			if (translation) {
				tmp=gemrb2iwd[tmp];
			}
			PCStats->QSlots[i]=tmp;
		}
		return;
	}
	for(int i=0;i<GUIBT_COUNT-3;i++) {
		ieByte tmp=PCStats->QSlots[i];
		if (translation) {
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
}

void Actor::SetSoundFolder(const char *soundset)
{
	if (core->HasFeature(GF_SOUNDFOLDERS)) {
		char path[_MAX_PATH];

		strnlwrcpy(PCStats->SoundFolder, soundset, 32);
		PathJoin(path,core->GamePath,"sounds",PCStats->SoundFolder,0);
		char *fp = FindInDir(path, "?????01", true);
		if (fp) {
			fp[5] = 0;
		} else {
			fp = FindInDir(path, "????01", true);
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
	unsigned int i=vvcCells->size();
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
	int j = true;
	vvcVector *vvcCells=&vvcShields;
retry:
	unsigned int i=vvcCells->size();
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
				(*vvcCells)[i]=NULL;
			}
		}
	}
	vvcCells=&vvcOverlays;
	if (j) { j = false; goto retry; }
}

void Actor::AddVVCell(ScriptedAnimation* vvc)
{
	vvcVector *vvcCells;

	if (vvc->ZPos<0) {
		vvcCells=&vvcShields;
	} else {
		vvcCells=&vvcOverlays;
	}
	unsigned int i=vvcCells->size();
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
		spellbook.ChargeAllSpells ();
	}
}
