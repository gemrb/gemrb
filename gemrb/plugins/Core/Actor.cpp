/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Actor.cpp,v 1.70 2004/10/09 15:27:22 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "TableMgr.h"
#include "Actor.h"
#include "Interface.h"

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

void InitSpellTables()
{
	int skilltable = core->LoadTable( "clskills" );
	TableMgr *tm = core->GetTable( skilltable );
	classcount = tm->GetRowCount();
	clericspelltables = (char **) calloc(classcount, sizeof(char*));
	wizardspelltables = (char **) calloc(classcount, sizeof(char*));
	for(int i = 0; i<classcount; i++) {
		char *spelltablename = tm->QueryField( i, 1 );
		if(spelltablename[0]!='*') {
			clericspelltables[i]=strdup(spelltablename);
		}
		spelltablename = tm->QueryField( i, 2 );
		if(spelltablename[0]!='*') {
			wizardspelltables[i]=strdup(spelltablename);
		}
	}
	core->DelTable( skilltable );
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

	LastTalkedTo = NULL;
	LastAttacker = NULL;
	LastHitter = NULL;
	LastProtecter = NULL;
	LastProtected = NULL;
	LastCommander = NULL;
	LastHelp = NULL;
	LastSeen = NULL;
	LastHeard = NULL;
	LastSummoner = NULL;
	LastDamage = 0;
	LastDamageType = 0;

	InternalFlags = 0;
	inventory.SetInventoryType(INVENTORY_CREATURE);
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
}

void Actor::SetText(char* ptr, unsigned char type)
{
	size_t len = strlen( ptr ) + 1;
	//32 is the maximum possible length of the actor name in the original games
	if(len>32) len=33; 
	if(type!=2) {
		LongName = ( char * ) realloc( LongName, len );
		memcpy( LongName, ptr, len );
	}
	if(type!=1) {
		ShortName = ( char * ) realloc( ShortName, len );
		memcpy( ShortName, ptr, len );
	}
}

void Actor::SetText(int strref, unsigned char type)
{
	if(type!=2) {
		if(LongName) free(LongName);
		LongName = core->GetString( strref );
	}
	if(type!=1) {
		if(ShortName) free(ShortName);
		ShortName = core->GetString( strref );
	}
}

void Actor::SetAnimationID(unsigned int AnimID)
{
	if (anims) {
		delete( anims );
	}
	//hacking PST no palette
	if(core->HasFeature(GF_ONE_BYTE_ANIMID) )
	{
		if(AnimID&0x8000) {
if(BaseStats[IE_COLORCOUNT])
printf("The following anim is supposed to be real colored (no recoloring)\n");
			BaseStats[IE_COLORCOUNT]=0;
		}
	}
	anims = new CharAnimations( AnimID, BaseStats[IE_ARMOR_TYPE]);
	if (anims) {
		SetCircleSize();
		anims->SetupColors(BaseStats+IE_COLORS);
	}
}

CharAnimations* Actor::GetAnims()
{
	return anims;
}

/** Returns a Stat value (Base Value + Mod) */
ieDword Actor::GetStat(unsigned int StatIndex)
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
	} else if (GetMod(IE_MORALEBREAK)<0) {
		color = &yellow;
	} else if (Modified[IE_STATE_ID] & STATE_PANIC) {
		color = &yellow;
	} else {
		switch (Modified[IE_EA]) {
			case PC:
			case FAMILIAR:
			case ALLY:
			case CONTROLLED:
			case CHARMED:
			case EVILBUTGREEN:
			case GOODCUTOFF:
				color = &green;
				break;

			case ENEMY:
			case GOODBUTRED:
			case EVILCUTOFF:
				color = &red;
				break;
			default:
				color = &cyan;
				break;
		}
	}
	SetCircle( anims->GetCircleSize(), *color );
}

bool Actor::SetStat(unsigned int StatIndex, ieDword Value)
{
	if (StatIndex >= MAX_STATS) {
		return false;
	}
	Modified[StatIndex] = Value;
	switch (StatIndex) {
		case IE_REPUTATION:
			if(InParty==1) {
				core->GetGame()->Reputation=Modified[IE_REPUTATION];
			}
			break;
		case IE_GOLD:
			if(InParty) {
				core->GetGame()->PartyGold+=Modified[IE_GOLD];
				Modified[IE_GOLD]=0;
			}
			break;
		case IE_ANIMATION_ID:
			SetAnimationID( (ieWord) Value );
			break;
		case IE_METAL_COLOR:
		case IE_MINOR_COLOR:
		case IE_MAJOR_COLOR:
		case IE_SKIN_COLOR:
		case IE_LEATHER_COLOR:
		case IE_ARMOR_COLOR:
		case IE_HAIR_COLOR:
			anims->SetupColors(Modified+IE_COLORS);
			break;
		case IE_EA:
		case IE_UNSELECTABLE:
		case IE_MORALEBREAK:
			SetCircleSize();
			break;
		case IE_HITPOINTS:
			if((signed) Value<=0) {
				Die(NULL);
			}
			if((signed) Value>(signed) Modified[IE_MAXHITPOINTS]) {
				Modified[IE_HITPOINTS]=Modified[IE_MAXHITPOINTS];
			}
			break;
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
	SetStat (StatIndex, Value);
	return true;
}
/** call this after load, before applying effects */
void Actor::Init()
{
	memcpy( Modified, BaseStats, MAX_STATS * sizeof( *Modified ) );
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

//returns actual damage
int Actor::Damage(int damage, int damagetype, Actor *hitter)
{
//recalculate damage based on resistances and difficulty level
//the lower 2 bits are actually modifier types
	NewStat(IE_HITPOINTS,-damage, damagetype&3);
	LastDamageType=damagetype;
	LastDamage=damage;
	LastHitter=hitter;
	return damage;
}

void Actor::DebugDump()
{
	unsigned int i;

	printf( "Debugdump of Actor %s:\n", LongName );
	for (i = 0; i < MAX_SCRIPTS; i++) {
		const char* poi = "<none>";
		if (Scripts[i] && Scripts[i]->script) {
			poi = Scripts[i]->script->GetName();
		}
		printf( "Script %d: %s\n", i, poi );
	}
	printf( "Area:       %.8s\n", Area );
	printf( "Dialog:     %.8s\n", Dialog );
	printf( "Script name:%.32s\n", scriptName );
	printf( "TalkCount:  %d\n", TalkCount );
	printf( "PartySlot:  %d\n", InParty );
	printf( "Allegiance: %d\n", BaseStats[IE_EA] );
	printf( "Visualrange:%d\n", Modified[IE_VISUALRANGE] );
	printf( "Mod[IE_EA]: %d\n", Modified[IE_EA] );
	printf( "Mod[IE_ANIMATION_ID]: 0x%04X\n", Modified[IE_ANIMATION_ID] );
	if (core->HasFeature(GF_ONE_BYTE_ANIMID) ) {
		for(i=0;i<Modified[IE_COLORCOUNT];i++) {
			printf("Colors #%d:  %d\n",i, Modified[IE_COLORS+i]);
		}
	}
	else {
		for(i=0;i<7;i++) {
			printf("Colors #%d:  %d\n",i, Modified[IE_COLORS+i]);
		}
	}
	ieDword tmp=0;
	core->GetGame()->globals->Lookup("APPEARANCE",tmp);
	printf( "Disguise: %d\n", tmp);
	inventory.dump();
	spellbook.dump();
}

void Actor::SetPosition(Map *map, Point &position, int jump, int radius)
{
	ClearPath();
	Point p;
	p.x = position.x/16;
	p.y = position.y/12;
	if (jump && !GetStat( IE_DONOTJUMP ) && anims->GetCircleSize() ) {
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
int Actor::GetXPLevel(int modified)
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

void Actor::Die(Scriptable *killer)
{
	Selected=false;
	int minhp=Modified[IE_MINHITPOINTS];
	if(minhp) { //can't die
		SetStat(IE_HITPOINTS, minhp);
	}
	InternalFlags|=IF_JUSTDIED;
	if(!InParty) {
		Actor *act=NULL;
		
		if(killer) {
			if(killer->Type==ST_ACTOR) {
				act = (Actor *) killer;
			}
		}
		if(act && act->InParty) {
			//adjust game statistics here
			//game->KillStat(this, killer);
			InternalFlags|=IF_GIVEXP;
		}
	}

	if(Modified[IE_HITPOINTS]<=0) {
		InternalFlags|=IF_REALLYDIED;
	}
        StanceID = IE_ANI_DIE;
}

bool Actor::CheckOnDeath()
{
	if(!(InternalFlags&IF_REALLYDIED) ) return false;
	//don't mess with the already deceased
	if(Modified[IE_STATE_ID]&STATE_DEAD) return false;
	//we need to check animID here, if it has not played the death
	//sequence yet, then we could return now

	if(InternalFlags&IF_GIVEXP) {
		//give experience to party
		core->GetGame()->ShareXP(GetStat(IE_XPVALUE), true );
		//handle reputation here
		//
	}
	DropItem("",0);
	//remove all effects that are not 'permanent after death' here
	//permanent after death type is 9
	Active = false; //we deactivate its scripts here, do we need it?
	Modified[IE_STATE_ID] |= STATE_DEAD;
	if(Modified[IE_MC_FLAGS]&MC_REMOVE_CORPSE) return true;
	if(Modified[IE_MC_FLAGS]&MC_KEEP_CORPSE) return false;
	//if chunked death, then return true
	return false;
}

/* this will create a heap at location, and transfer the item(s) */
void Actor::DropItem(const char *resref, unsigned int flags)
{
	Map *map = core->GetGame()->GetMap(Area);
	inventory.DropItemAtLocation( resref, flags, map, Pos );
}

bool Actor::ValidTarget(int ga_flags)
{
	switch(ga_flags&GA_ACTION) {
	case GA_PICK:
		if(Modified[IE_STATE_ID] & STATE_CANTSTEAL) return false;
		break;
	case GA_TALK:
		//can't talk to dead
		if(Modified[IE_STATE_ID] & STATE_CANTLISTEN) return false;
		//can't talk to hostile
		if(Modified[IE_EA]>=EVILCUTOFF) return false;
		break;
	}
	if(ga_flags&GA_NO_DEAD) {
		if(InternalFlags&IF_JUSTDIED) return false;
		if(Modified[IE_STATE_ID] & STATE_DEAD) return false;
	}
	if(ga_flags&GA_SELECT) {
		if(Modified[IE_UNSELECTABLE]) return false;
	}
	return true;
}

//returns true if it won't be destroyed with an area
//in this case it shouldn't be saved with the area either
//it will be saved in the savegame
bool Actor::Persistent()
{
	if(InParty) return true;
	if(InternalFlags&IF_FROMGAME) return true;
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

void Actor::GetNextStance()
{
	static int Stance = IE_ANI_AWAKE;

	if (--Stance < 0) Stance = MAX_ANIMS-1;
	printf ("StanceID: %d\n", Stance);
	StanceID = Stance;
}

//we have to determine if the actor is able to cast priest or mage
//spells (IWD2 has a different system)
//-1 means the actor can't memorize that type of spells
int Actor::GetMemorizableSpellsCount(ieSpellType Type, int Level)
{
	if( classcount < 0 ) {
		InitSpellTables();
	}
	int ActorLevel = GetStat(IE_LEVEL);
	int MCFlags = GetStat(IE_MC_FLAGS);
	int Class = GetStat(IE_CLASS);
	if( Class>=classcount ) {
		//non playable class can't memorize
		return -1;
	}
	char *spelltablename=NULL;

	switch(Type) {
		case IE_SPELL_TYPE_PRIEST:
			if( MCFlags & (MC_FALLEN_PALADIN|MC_FALLEN_RANGER) ) {
				return -1;
			}
			spelltablename = clericspelltables[Class];
			break;
		case IE_SPELL_TYPE_WIZARD:
			spelltablename = wizardspelltables[Class];
			break;
		case IE_SPELL_TYPE_INNATE:  //just for completeness
			return -1;
	}
	if( spelltablename==NULL ) {
		return -1;
	}
	int spelltable = core->LoadTable( spelltablename );
	if( spelltable < 0 ) {
		//this is an error, but we don't crash
		return -1;
	}
	TableMgr *tm = core->GetTable( spelltable );
	int count = atoi(tm->QueryField( ActorLevel, Level ) );
	//keep it cached
	//core->DelTable( spelltable );
	if( count <= 0 ) {
		return 0;
	}
	if( Type == IE_SPELL_TYPE_PRIEST ) {
		spelltable = core->LoadTable( "mxsplwis" );
		if( spelltable >= 0 ) {
			tm = core->GetTable( spelltable );
			ActorLevel = GetStat(IE_WIS)-13;
			if( ActorLevel >= 0 ) {
				count += atoi(tm->QueryField( ActorLevel, Level ));
			}
			//we keep it cached
			//core->DelTable( spelltable );
		}
	}
	if( count <= 0 ) {
		return 0;
	}
	return count;
}

#if 0
//IWD2 has a more complex system, including bardsongs, druid shapes, etc
int Actor::GetMemorizableSpellsCountIWD2(ieSpellType Type, int Level)
{
	if( classcount < 0 ) {
		InitSpellTables();
	}
	int ActorLevel = GetStat(IE_LEVEL);
	int MCFlags = GetStat(IE_MC_FLAGS);
	int Class = GetStat(IE_CLASS);
	if( Class>=classcount ) {
		//non playable class can't memorize
		return -1;
	}
	char *spelltablename=NULL;

	switch(Type) {
		case IE_SPELL_TYPE_PRIEST:
			if( MCFlags & (MC_FALLEN_PALADIN|MC_FALLEN_RANGER) ) {
				return -1;
			}
			spelltablename = clericspelltables[Class];
			break;
		case IE_SPELL_TYPE_WIZARD:
			spelltablename = wizardspelltables[Class];
			break;
		case IE_SPELL_TYPE_INNATE:  //just for completeness
			return -1;
	}
	if( spelltablename==NULL ) {
		return -1;
	}
	int spelltable = core->LoadTable( spelltablename );
	if( spelltable < 0 ) {
		//this is an error, but we don't crash
		return -1;
	}
	TableMgr *tm = core->GetTable( spelltable );
	int count = atoi(tm->QueryField( ActorLevel, Level ) );
	//keep it cached
	//core->DelTable( spelltable );
	if( Type == IE_SPELL_TYPE_PRIEST ) {
		spelltable = core->LoadTable( "mxsplbon" );
		if( spelltable < 0 ) {
			tm = core->GetTable( spelltable );
			ActorLevel = GetStat(IE_WIS)-12;
			if( ActorLevel >= 0 ) {
				count += atoi(tm->QueryField( ActorLevel, Level ));
			}
			//we keep it cached
			//core->DelTable( spelltable );
		}
	}
	if( count <= 0 ) {
		return 0;
	}
	return count;
}
#endif //0
