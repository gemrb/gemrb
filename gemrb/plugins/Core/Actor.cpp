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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Actor.cpp,v 1.140 2005/12/03 20:48:44 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "TableMgr.h"
#include "Actor.h"
#include "Interface.h"
#include "../../includes/strrefs.h"
#include "Item.h"
#include "Spell.h"
#include "Game.h"
#include "GameScript.h"
#include "GSUtils.h"    //needed for DisplayStringCore

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
/*
static Color green_dark = {
	0x00, 0x80, 0x00, 0xff
};
static Color red_dark = {
	0x80, 0x00, 0x00, 0xff
};
static Color yellow_dark = {
	0x80, 0x80, 0x00, 0xff
};
static Color cyan_dark = {
	0x00, 0x80, 0x80, 0xff
};
*/

static int classcount=-1;
static char **clericspelltables=NULL;
static char **wizardspelltables=NULL;
//static int constitution_normal[26];
//static int constitution_fighter[26];

static void InitActorTables();

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

	InternalFlags = 0;
	inventory.SetInventoryType(INVENTORY_CREATURE);
	fxqueue.SetOwner( this );
	inventory.SetOwner( this );
	if (classcount<0) {
		InitActorTables();
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
	if (anims) {
		delete( anims );
	}
	if (LongName) {
		free( LongName );
	}
	if (ShortName) {
		free( ShortName );
	}
	if (PCStats) {
		delete PCStats;
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
		//only animtype==7 (mirror3) uses this feature
		//this is a hardcoded hack, but works for all engine type
		if (anims->GetAnimType()!=IE_ANI_CODE_MIRROR_3) {
			BaseStats[IE_DONOTJUMP]=0;
		} else {
			BaseStats[IE_DONOTJUMP]=3;
		}
		SetCircleSize();
		anims->SetColors(BaseStats+IE_COLORS);
	} else {
		printf("[Actor] Missing animation for %s\n",LongName);
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
	if (Modified[IE_UNSELECTABLE]) {
		color = &magenta;
	} else if (GetMod(IE_MORALE)<0) {//if current morale < the max morale ?
		color = &yellow;
	} else if (Modified[IE_STATE_ID] & STATE_PANIC) {
		color = &yellow;
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
				break;

			case EA_ENEMY:
			case EA_GOODBUTRED:
			case EA_EVILCUTOFF:
				color = &red;
				break;
			default:
				color = &cyan;
				break;
		}
	}
	SetCircle( anims->GetCircleSize(), *color );
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
	if ((signed) Value<=0) {
		actor->Die(NULL);
	}
	if ((signed) Value>(signed) actor->Modified[IE_MAXHITPOINTS]) {
		actor->Modified[IE_HITPOINTS]=actor->Modified[IE_MAXHITPOINTS];
	}
}

void pcf_maxhitpoint(Actor *actor, ieDword Value)
{
	if ((signed) Value<=0) {
		actor->Die(NULL);
	}
	if ((signed) Value<(signed) actor->Modified[IE_HITPOINTS]) {
		actor->Modified[IE_HITPOINTS]=actor->Modified[IE_MAXHITPOINTS];
	}
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
1,1,1,1,50,50,1,9999,25,100,100,1,1,20,20,25,//4f
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
pcf_stat,pcf_stat,NULL,NULL, NULL, pcf_gold, NULL, NULL, //2f
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL,
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL, //3f
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL,
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL, //4f
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL,
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
NULL,NULL,NULL,NULL, NULL, NULL, NULL, NULL, 
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
	if (classcount>=0) {
		if (clericspelltables) {
			for (int i=0;i<classcount;i++) {
				if (clericspelltables[i]) {
					free (clericspelltables[i]);
				}
			}
			free(clericspelltables);
		}
		if (wizardspelltables) {
			for (int i=0;i<classcount;i++) {
				if (wizardspelltables[i]) {
					free(wizardspelltables[i]);
				}
			}
			free(wizardspelltables);
		}
	}
	classcount=-1;
}

static void InitActorTables()
{
	int i;

	int table = core->LoadTable( "clskills" );
	TableMgr *tm = core->GetTable( table );
	classcount = tm->GetRowCount();
	clericspelltables = (char **) calloc(classcount, sizeof(char*));
	wizardspelltables = (char **) calloc(classcount, sizeof(char*));
	for(i = 0; i<classcount; i++) {
		char *spelltablename = tm->QueryField( i, 1 );
		if (spelltablename[0]!='*') {
			clericspelltables[i]=strdup(spelltablename);
		}
		spelltablename = tm->QueryField( i, 2 );
		if (spelltablename[0]!='*') {
			wizardspelltables[i]=strdup(spelltablename);
		}
	}
	core->DelTable( table );
/*
	//these will be moved to core
	table = core->LoadTable( "hpconbon" );
	tm = core->GetTable( table );
	for(i=0;i<26;i++) {
		constitution_normal[i] = atoi(tm->QueryField( i, 1) );
		constitution_fighter[i] = atoi(tm->QueryField( i, 2) );
	}
	core->DelTable( table );
*/
	i = core->GetMaximumAbility();
	maximum_values[IE_STR]=i;
	maximum_values[IE_INT]=i;
	maximum_values[IE_DEX]=i;
	maximum_values[IE_CON]=i;
	maximum_values[IE_CHR]=i;
	maximum_values[IE_WIS]=i;
}

bool Actor::SetStat(unsigned int StatIndex, ieDword Value)
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
	Modified[StatIndex] = Value;
	PostChangeFunctionType f = post_change_functions[StatIndex];
	if (f) (*f)(this, Value);
	return true;
}
/* use core->GetConstitutionBonus(0,0,Modified[IE_CON])
int Actor::GetHPMod()
{
	return constitution_normal[Modified[IE_CON]];
}
*/
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
	SetStat (StatIndex, Value);
	return true;
}
/** call this after load, to apply effects */
void Actor::Init()
{
	memcpy( Modified, BaseStats, MAX_STATS * sizeof( *Modified ) );
	fxqueue.ApplyAllEffects( this );
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
			SetStat(StatIndex, Modified[StatIndex]+ModifierValue);
			break;
		case MOD_ABSOLUTE:
			//straight stat change
			SetStat(StatIndex, ModifierValue);
			break;
		case MOD_PERCENT:
			//percentile
			SetStat(StatIndex, BaseStats[StatIndex] * 100 / ModifierValue);
			break;
	}
	return Modified[StatIndex] - oldmod;
}

void Actor::Panic()
{
	SetStat(IE_MORALE,0);
	SetStat(IE_STATE_ID,GetStat(IE_STATE_ID)|STATE_PANIC);
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
	Active|=SCR_ACTIVE;
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
	printf( "Area:       %.8s\n", Area );
	printf( "Dialog:     %.8s\n", Dialog );
	printf( "Script name:%.32s\n", scriptName );
	printf( "TalkCount:  %d\n", TalkCount );
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
	ieDword tmp=0;
	core->GetGame()->locals->Lookup("APPEARANCE",tmp);
	printf( "\nDisguise: %d\n", tmp);
	printf( "WaitCounter: %d\n", (int) GetWait());
	inventory.dump();
	spellbook.dump();
	fxqueue.dump();
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
	if (jump && !(GetStat( IE_DONOTJUMP )&1) && anims->GetCircleSize() ) {
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
	InternalFlags = 0;
	SetStat(IE_STATE_ID, 0);
	SetStat(IE_HITPOINTS, 255);
	ClearActions();
	ClearPath();
	SetStance(IE_ANI_EMERGE);
	//clear effects?
}

void Actor::Die(Scriptable *killer)
{
	int minhp=Modified[IE_MINHITPOINTS];
	if (minhp) { //can't die
		SetStat(IE_HITPOINTS, minhp);
		return;
	}
	//Can't simply set Selected to false, game has its own little list
	core->GetGame()->SelectActor(this, false, SELECT_NORMAL);
	ClearPath();
	SetModal( 0 );
	DisplayStringCore(this, VB_DIE, DS_CONSOLE|DS_CONST );
	if (!InParty) {
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

	//JUSTDIED will be removed when the Die() trigger executed
	//otherwise it is the same as REALLYDIED
	InternalFlags|=IF_REALLYDIED|IF_JUSTDIED;
	SetStance( IE_ANI_DIE );
	if (InParty) {
		core->Autopause(AP_DEAD);
	}
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
	Modified[IE_STATE_ID] |= STATE_DEAD;
	if (Modified[IE_MC_FLAGS]&MC_REMOVE_CORPSE) return true;
	if (Modified[IE_MC_FLAGS]&MC_KEEP_CORPSE) return false;
	//if chunked death, then return true
	return false;
}

/* this will create a heap at location, and transfer the item(s) */
void Actor::DropItem(ieResRef resref, unsigned int flags)
{
	inventory.DropItemAtLocation( resref, flags, area, Pos );
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

int Actor::LearnSpell(ieResRef spellname, ieDword flags)
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
	ieByte shift = idx/16;
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
		NewStat(IE_HITPOINTS, days * 2, MOD_ADDITIVE);
	} else {
		SetStat(IE_HITPOINTS, GetStat(IE_MAXHITPOINTS));
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
	if (!(Active&SCR_VISIBLE) ) {
		return false;
	}

	//check for schedule
	ieDword bit = 1<<(gametime%7200/300);
	if (appearance & bit) {
		return true;
	}
	return false;
}

void Actor::WalkTo(Point &Des, int MinDistance)
{
	if (InternalFlags&IF_REALLYDIED) {
		return;
	}
	Moveble::WalkTo(Des, MinDistance);
}

void Actor::DrawOverheadText(Region &screen)
{
	unsigned long time;
	GetTime( time );

	if (!textDisplaying)
		return;
	if (( time - timeStartDisplaying ) >= 6000) {
		textDisplaying = 0;
	}

	Font* font = core->GetFont( 1 );
	int cs = anims?anims->GetCircleSize():0;
	Region rgn( Pos.x-100+screen.x, Pos.y - cs * 50 + screen.y, 200, 400 );
	font->Print( rgn, ( unsigned char * ) overHeadText,
	NULL, IE_FONT_ALIGN_CENTER | IE_FONT_ALIGN_TOP, false );
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
		case VB_DIE:
			index = 10;
			break;
	}
	strnuprcpy(Sound, tab->QueryField (index, 0), 8);
end:
	core->DelTable( table );

}
