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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Actor.h,v 1.31 2004/04/21 17:41:38 avenger_teambg Exp $
 *
 */

#ifndef ACTOR_H
#define ACTOR_H

#include <vector>
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

class GEM_EXPORT Actor : public Moveble {
public:
	//CRE DATA FIELDS
	long BaseStats[MAX_STATS];
	long Modified[MAX_STATS];
	char Dialog[9];
	char SmallPortrait[9];
	char LargePortrait[9];
	/** 0: NPC, 1-6 party slot */
	unsigned char InParty;
	char* LongName, * ShortName;
	unsigned long StrRefs[100];

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
	bool DeleteMe;
	bool FromGame;

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

private:
	void SetCircleSize();
public:
	Actor(void);
	~Actor(void);
	/** prints useful information on console */
	void DebugDump();
	/** sets the actor's position, calculating with the nojump flag*/
	void SetPosition(Map *map, unsigned int XPos, unsigned int YPos, bool jump);
	void SetAnimationID(unsigned short AnimID);
	/** returns the animations */
	CharAnimations* GetAnims();
	/** Inits the Modified vector */
	void Init();
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
		if (which) {
			return LongName;
		}
		return ShortName;
	}
	/** Gets the DeathVariable */
	char* GetScriptName(void)
	{
		//return ScriptName;
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
};
#endif
