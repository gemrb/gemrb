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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Actor.h,v 1.43 2004/08/05 22:55:34 avenger_teambg Exp $
 *
 */

#ifndef ACTOR_H
#define ACTOR_H

#include <vector>
#include "../../includes/ie_types.h"
#include "Animation.h"
#include "CharAnimations.h"
#include "ActorBlock.h"

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

//stat modifier type
#define MOD_ADDITIVE  0
#define MOD_ABSOLUTE  1
#define MOD_PERCENT   2

//internal actor flags
#define IF_GIVEXP     1     //give xp for this death
#define IF_JUSTDIED   2     //Died() will return true
#define IF_FROMGAME   4     //this is an NPC or PC
#define IF_REALLYDIED 8     //real death happened, actor will be set to dead

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

class GEM_EXPORT Actor : public Moveble {
public:
	//CRE DATA FIELDS
	long BaseStats[MAX_STATS];
	long Modified[MAX_STATS];
	char Dialog[9];
	char SmallPortrait[9];
	char LargePortrait[9];
	/** 0: NPC, 1-8 party slot */
	unsigned char InParty;
	char* LongName, * ShortName;
	ieStrRef StrRefs[100];

	ieWord AppearanceFlags1;
	ieWord AppearanceFlags2;

	char KillVar[32]; //this second field is present in pst and iwd1
	// for remapping palette
	ieByte ColorsCount;
	ieWord Colors[7];
	ieByte ColorPlacements[7];

	ieWord unknown2F2;
	ieByte unknown2F4;
	ieDword unknown2FC[5];
	ieByte unknown310;

	ieDword KnownSpellsOffset;
	ieDword KnownSpellsCount;
	ieDword SpellMemorizationOffset;
	ieDword SpellMemorizationCount;
	ieDword MemorizedSpellsOffset;
	ieDword MemorizedSpellsCount;
	ieDword ItemSlotsOffset;
	ieDword ItemsOffset;
	ieDword ItemsCount;

	Inventory inventory;
	Spellbook spellbook;
public:
	//this stuff don't get saved
	CharAnimations* anims;
	int InternalFlags;

	Actor *LastTalkedTo;
	Actor *LastAttacker;
	Actor *LastHitter;
	Actor *LastProtecter;
	Actor *LastProtected;
	Actor *LastCommander;
	Actor *LastHelp;
	Actor *LastSeen;
	Actor *LastHeard;
	Actor *LastSummoner;
	int LastDamage;
	int LastDamageType;

private:
	void SetCircleSize();
public:
	Actor(void);
	~Actor(void);
	/** prints useful information on console */
	void DebugDump();
	/** sets the actor's position, calculating with the nojump flag*/
	void SetPosition(Map *map, unsigned int XPos, unsigned int YPos, int jump, int radius=0);
	void SetAnimationID(unsigned short AnimID);
	/** returns the animations */
	CharAnimations* GetAnims();
	/** Inits the Modified vector */
	void Init();
	/** Returns true if the actor is targetable */
	bool ValidTarget(int ga_flags);
	/** Returns a Stat value */
	long GetStat(unsigned int StatIndex);
	/** Sets a Stat Value (unsaved) */
	bool SetStat(unsigned int StatIndex, long Value);
	/** Returns the difference */
	long GetMod(unsigned int StatIndex);
	/** Returns a Stat Base Value */
	long GetBase(unsigned int StatIndex);
	/** Sets a Stat Base Value */
	bool SetBase(unsigned int StatIndex, long Value);
	/** Sets the modified value in different ways, returns difference */
	int NewStat(unsigned int StatIndex, long ModifierValue, long ModifierType);
	/** Sets the Scripting Name (death variable) */
	void SetScriptName(const char* string)
	{
		if (string == NULL) {
			return;
		}
		strncpy( scriptName, string, 32 );
	}
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
	char* GetName(int which)
	{
		if(which==-1) which=!TalkCount;
		if (which) {
			return LongName;
		}
		return ShortName;
	}
	/** Gets the DeathVariable */
	char* GetScriptName(void)
	{
		return scriptName;
	}
	/** Gets a Script ResRef */
	char* GetScript(int ScriptIndex)
	{
		//return Scripts[ScriptIndex];
		return NULL;
	}
	/** Gets the Character's level for XP calculations */
	int GetXPLevel(int modified);

	/** Gets the Dialog ResRef */
	char* GetDialog(void)
	{
		return Dialog;
	}
	/** Gets the Portrait ResRef */
	char* GetPortrait(int which)
	{
		return which ? SmallPortrait : LargePortrait;
	}
	void SetText(char* ptr, unsigned char type);
	void SetText(int strref, unsigned char type);
	/* returns carried weight atm, could calculate with strength*/
	int GetEncumbrance();
	/* checks on death of actor, returns true if it should be removed*/
	bool CheckOnDeath();
	/* deals damage to this actor */
	int Damage(int damage, int damagetype, Actor *hitter);
	/* drops items from inventory to current spot */
	void DropItem(const char *resref, unsigned int flags);
	/* returns true if the actor is PC/joinable*/
	bool Persistent();
	/* schedules actor to die*/
	void Die(Scriptable *killer);
};
#endif
