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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Actor.h,v 1.75 2005/06/26 13:57:51 avenger_teambg Exp $
 *
 */

#ifndef ACTOR_H
#define ACTOR_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <vector>
#include "../../includes/ie_types.h"
#include "Animation.h"
#include "CharAnimations.h"
#include "ActorBlock.h"
#include "EffectQueue.h"

class Map;

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

/** USING DEFINITIONS AS DESCRIBED IN STATS.IDS */
#include "../../includes/ie_stats.h"
#include "../../includes/ie_types.h"

#include "Inventory.h"
#include "Spellbook.h"

#define MAX_STATS 256

//modal states
#define MS_NONE        0
#define MS_BATTLESONG  1
#define MS_DETECTTRAPS 2
#define MS_STEALTH     3
#define MS_TURNUNDEAD  4

//stat modifier type
#define MOD_ADDITIVE  0
#define MOD_ABSOLUTE  1
#define MOD_PERCENT   2

//internal actor flags
#define IF_GIVEXP     1     //give xp for this death
#define IF_JUSTDIED   2     //Died() will return true
#define IF_FROMGAME   4     //this is an NPC or PC
#define IF_REALLYDIED 8     //real death happened, actor will be set to dead
#define IF_NORECTICLE 16    //draw recticle (target mark)
#define IF_NOINT      32    //cannot interrupt the actions of this actor (save is not possible!)
#define IF_CLEANUP    64    //actor died chunky death, or other total destruction

/** flags for GetActor */
//default action
#define GA_DEFAULT  0
//actor selected for talk
#define GA_TALK     1
//actor selected for attack
#define GA_ATTACK   2
//actor selected for spell target
#define GA_SPELL    3
//actor selected for pick pockets
#define GA_PICK     4

//action mask
#define GA_ACTION   15
//unselectable actor may not be selected (can still block)
#define GA_SELECT   16
//dead actor may not be selected
#define GA_NO_DEAD  32

class GEM_EXPORT PCStatsStruct {
public:
	ieStrRef  BestKilledName;
	ieDword   BestKilledXP;
	ieDword   AwayTime;
	ieDword   JoinDate;
	ieDword   unknown10;
	ieDword   KillsChapterXP;
	ieDword   KillsChapterCount;
	ieDword   KillsTotalXP;
	ieDword   KillsTotalCount;
	ieResRef  FavouriteSpells[4];
	ieWord    FavouriteSpellsCount[4];
	ieResRef  FavouriteWeapons[4];
	ieWord    FavouriteWeaponsCount[4];
public:
	PCStatsStruct();
	void IncrementChapter();
};

class GEM_EXPORT Actor : public Moveble {
public:
	//CRE DATA FIELDS
	ieDword BaseStats[MAX_STATS];
	ieDword Modified[MAX_STATS];
	PCStatsStruct*  PCStats;
	ieResRef Dialog;
	ieResRef SmallPortrait;
	ieResRef LargePortrait;
	/** 0: NPC, 1-8 party slot */
	unsigned char InParty;
	char* LongName, * ShortName;
	ieStrRef ShortStrRef, LongStrRef;
	ieStrRef StrRefs[100];

	ieWord AppearanceFlags1;
	ieWord AppearanceFlags2;

	char KillVar[33]; //this second field is present in pst and iwd1

	Inventory inventory;
	Spellbook spellbook;
	//savefile version (creatures embedded in area)
	int version;
	//in game or area actor header
	ieDword TalkCount;
	ieDword InteractCount; //this is accessible in iwd2, probably exists in other games too
	ieDword appearance;
	ieDword ModalState;
public:
	Actor *LastTarget;
	Actor *LastTalkedTo;
	Actor *LastAttacker;
	Actor *LastHitter;
	Actor *LastProtected;
	Actor *LastCommander;
	Actor *LastHelp;
	Actor *LastSeen;
	Actor *LastHeard;
	Actor *LastSummoner;
	//this is an ugly ugly hack, the triggers are stored on a pointer
	//these triggers are not pointers by nature
	Actor *HotKey; 

	int LastCommand;   //lastcommander
	int LastShout;     //lastheard
	int LastDamage;    //lasthitter
	int LastDamageType;//lasthitter

	EffectQueue fxqueue;
private:
	//this stuff don't get saved
	CharAnimations* anims;

	/* fixes the palette */
	void SetupColors();

public:
	Actor(void);
	~Actor(void);
	/** prints useful information on console */
	void DebugDump();
	/* fixes the feet circle */
	void SetCircleSize();
	/** sets the actor's position, calculating with the nojump flag*/
	void SetPosition(Map *map, Point &position, int jump, int radius=0);
	/* you better use SetStat, this stuff is only for special cases*/
	void SetAnimationID(unsigned int AnimID);
	/** returns the animations */
	CharAnimations* GetAnims();
	/** Inits the Modified vector */
	void Init();
	/** Returns true if the actor is targetable */
	bool ValidTarget(int ga_flags);
	/** Returns a Stat value */
	ieDword GetStat(unsigned int StatIndex) const;
	/** Sets a Stat Value (unsaved) */
	bool SetStat(unsigned int StatIndex, ieDword Value);
	/** Returns the difference */
	int GetMod(unsigned int StatIndex);
	/** Returns a Stat Base Value */
	ieDword GetBase(unsigned int StatIndex);
	/** Sets a Stat Base Value */
	bool SetBase(unsigned int StatIndex, ieDword Value);
	/** Sets the modified value in different ways, returns difference */
	int NewStat(unsigned int StatIndex, ieDword ModifierValue, ieDword ModifierType);
	/** Sets the Dialog ResRef */
	void SetDialog(const char* ResRef)
	{
		if (ResRef == NULL) {
			return;
		}
		strncpy( Dialog, ResRef, 8 );
		printf("Setting Dialog for %s: %.8s\n",LongName, Dialog);
	}
	/** Sets the Icon ResRef */
	//Which - 0 both, 1 Large, 2 Small
	void SetPortrait(const char* ResRef, int Which=0)
	{
		int i;

		if (ResRef == NULL) {
			return;
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
			for (i = 0; i < 8 && ResRef[i]; i++);
			SmallPortrait[i] = 'S';
			LargePortrait[i] = 'M';
		}
	}
	/** Gets the Character Long Name/Short Name */
	char* GetName(int which) const
	{
		if(which==-1) which=!TalkCount;
		if (which) {
			return LongName;
		}
		return ShortName;
	}
	/** Gets the DeathVariable */
	const char* GetScriptName(void) const 
	{
		return scriptName;
	}
	/** Gets a Script ResRef */
	const char* GetScript(int ScriptIndex) const;
	/** Gets the Character's level for XP calculations */
	int GetXPLevel(int modified) const;

	/** Gets the Dialog ResRef */
	const char* GetDialog(bool checks=false) const;
	/** Gets the Portrait ResRef */
	const char* GetPortrait(int which) const
	{
		return which ? SmallPortrait : LargePortrait;
	}

	void SetText(char* ptr, unsigned char type);
	void SetText(int strref, unsigned char type);
	/* returns carried weight atm, could calculate with strength*/
	int GetEncumbrance();
	/* checks on death of actor, returns true if it should be removed*/
	bool CheckOnDeath();
	/* receives undead turning message */
	void Turn(Scriptable *cleric, int turnlevel);
	/* sets the actor in panic (turn/morale break) */
	void Panic();
	/* deals damage to this actor */
	int Damage(int damage, int damagetype, Actor *hitter);
	/* drops items from inventory to current spot */
	void DropItem(ieResRef resref, unsigned int flags);
	/* returns true if the actor is PC/joinable*/
	bool Persistent();
	/* resurrects actor */
	void Resurrect();
	/* schedules actor to die */
	void Die(Scriptable *killer);
	/* debug function */
	void GetNextAnimation();
	/* debug function */
	void GetNextStance();
	/* returns the count of memorizable spells of type at the given level */
	//int GetMemorizableSpellsCount(ieSpellType Type, int Level);
	int LearnSpell(ieResRef resref, ieDword flags);
	/* Returns weapon range */
	int GetWeaponRange();
	/* Creates player statistics */
	void CreateStats();
	int GetHPMod();
	/* Sets the modal state after checks */
	void SetModal(ieDword newstate);
};
#endif
