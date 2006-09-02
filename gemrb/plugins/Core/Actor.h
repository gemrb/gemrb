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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Actor.h,v 1.129 2006/09/02 21:24:47 avenger_teambg Exp $
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
#include "ScriptedAnimation.h"
#include "ActorBlock.h"
#include "EffectQueue.h"
#include "PCStatStruct.h"

class Map;
class ScriptedAnimation;

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

//'do not jump' flags
#define DNJ_FIT        1
#define DNJ_UNHINDERED 2
#define DNJ_JUMP       4
#define DNJ_BIRD       (DNJ_FIT|DNJ_UNHINDERED)

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

#define GUIBT_COUNT  12

#define VCONST_COUNT 100

typedef ieByte ActionButtonRow[GUIBT_COUNT];

typedef std::vector< ScriptedAnimation*> vvcVector;

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
	ieByte InParty;
	char* LongName, * ShortName;
	ieStrRef ShortStrRef, LongStrRef;
	ieStrRef StrRefs[VCONST_COUNT];

	ieWord AppearanceFlags1;
	ieWord AppearanceFlags2;

	ieVariable KillVar; //this second field is present in pst and iwd1

	Inventory inventory;
	Spellbook spellbook;
	//savefile version (creatures embedded in area)
	int version;
	//in game or area actor header
	ieDword TalkCount;
	ieDword InteractCount; //this is accessible in iwd2, probably exists in other games too
	ieDword appearance;
	ieDword ModalState;
	ieWord globalID;
	ieWord localID;
public:
	ieDword Leader;
	#define LastTarget LastDisarmFailed
	//ieDword LastTarget; use lastdisarmfailed
	#define LastAttacker LastDisarmed
	//ieDword LastAttacker; use lastdisarmed
	#define LastHitter LastEntered
	//ieDword LastHitter; use lastentered
	#define LastSummoner LastTrigger
	//ieDword LastSummoner; use lasttrigger
	#define LastTalkedTo LastUnlocked
	//ieDword LastTalkedTo; use lastunlocked
	ieDword LastProtected;
	ieDword LastCommander;
	ieDword LastHelp;
	ieDword LastSeen;
	ieDword LastHeard;
	ieDword HotKey; 

	int LastCommand;   //lastcommander
	int LastShout;     //lastheard
	int LastDamage;    //lasthitter
	int LastDamageType;//lasthitter
	int XF, YF;        //follow leader in this offset

	EffectQueue fxqueue;
	vvcVector vvcOverlays;
	vvcVector vvcShields;
private:
	//this stuff don't get saved
	CharAnimations* anims;
	ieByte SavingThrow[5];
	//how many attacks in this round
	int attackcount;
	//when the next attack is scheduled (gametime+initiative)
	ieDword initiative;

	/** fixes the palette */
	void SetupColors();
	/** debugging function, gets the scripting name of an actor referenced by a global ID */
	const char* GetActorNameByID(ieDword ID);
	/* if Lasttarget is gone, call this */
	void StopAttack();
public:
	Actor(void);
	~Actor(void);
	/** releases memory */
	static void ReleaseMemory();
	/** sets game specific default data about action buttons */
	static void SetDefaultActions(bool qslot, ieByte slot1, ieByte slot2, ieByte slot3);
	/** prints useful information on console */
	void DebugDump();
	/** fixes the feet circle */
	void SetCircleSize();
	/** places the actor on the map with a unique object ID */
	void SetMap(Map *map, ieWord LID, ieWord GID);
	/** sets the actor's position, calculating with the nojump flag*/
	void SetPosition(Map *map, Point &position, int jump, int radius=0);
	/** you better use SetStat, this stuff is only for special cases*/
	void SetAnimationID(unsigned int AnimID);
	/** returns the animations */
	CharAnimations* GetAnims();
	/** Re/Inits the Modified vector */
	void RefreshEffects();
	/** gets saving throws */
	void RollSaves();
	/** returns a saving throw */
	bool GetSavingThrow(ieDword type, int modifier);
	/** Returns true if the actor is targetable */
	bool ValidTarget(int ga_flags);
	/** Returns a Stat value */
	ieDword GetStat(unsigned int StatIndex) const;
	/** Sets a Stat Value (unsaved) */
	bool SetStat(unsigned int StatIndex, ieDword Value, int pcf);
	/** Returns the difference */
	int GetMod(unsigned int StatIndex);
	/** Returns a Stat Base Value */
	ieDword GetBase(unsigned int StatIndex);
	/** Sets a Base Stat Value */
	bool SetBase(unsigned int StatIndex, ieDword Value);
	/** set/resets a Base Stat bit */
	bool SetBaseBit(unsigned int StatIndex, ieDword Value, bool setreset);
	/** Sets the modified value in different ways, returns difference */
	int NewStat(unsigned int StatIndex, ieDword ModifierValue, ieDword ModifierType);
	/** Modifies the base stat value in different ways, returns difference */
	int NewBase(unsigned int StatIndex, ieDword ModifierValue, ieDword ModifierType);
	void SetLeader(Actor *actor, int xoffset=0, int yoffset=0);
	ieDword GetID()
	{
		return (localID<<16) | globalID;
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
	void SetSoundFolder(const char *soundset);
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
	ieDword GetXPLevel(int modified) const;

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
	void Turn(Scriptable *cleric, ieDword turnlevel);
	/* call this on gui selects */
	void SelectActor();
	/* sets the actor in panic (turn/morale break) */
	void Panic();
	/* sets a multi class flag (actually this is a lot of else too) */
	void SetMCFlag(ieDword bitmask, int op);
	/* called when someone died in the party */
	void ReactToDeath(const char *deadname);
	/* called when someone talks to Actor */
	void DialogInterrupt();
	/* deals damage to this actor */
	int Damage(int damage, int damagetype, Actor *hitter);
	/* drops items from inventory to current spot */
	void DropItem(const ieResRef resref, unsigned int flags);
	void DropItem(int slot, unsigned int flags);
	/* returns item information in quickitem slot */
	void GetItemSlotInfo(ItemExtHeader *item, int which);
	/* returns spell information in quickspell slot */
	void GetSpellSlotInfo(SpellExtHeader *spell, int which);
	/* updates quickslots */
	void ReinitQuickSlots();
	/* returns true if the actor is PC/joinable*/
	bool Persistent();
	/* assigns actor to party slot, 0 = NPC, areas won't remove it */
	void SetPersistent(int partyslot);
	/* resurrects actor */
	void Resurrect();
	/* removes actor in the next update cycle */
	void DestroySelf();
	/* schedules actor to die */
	void Die(Scriptable *killer);
	/* debug function */
	void GetNextAnimation();
	/* debug function */
	void GetNextStance();
	/* learns the given spell, possibly receive XP */
	int LearnSpell(const ieResRef resref, ieDword flags);
	/* returns the ranged weapon header associated with the currently equipped projectile */
	int GetRangedWeapon(ITMExtHeader *&which);
	/* Returns current weapon range and extended header
	   if range is nonzero, then which is valid */
	unsigned int GetWeapon(ITMExtHeader *&which, bool leftorright=false);
	/* Creates player statistics */
	void CreateStats();
	/* Heals actor by days */
	void Heal(int days);
	/* Receive experience (handle dual/multi class) */
	void AddExperience(int exp);
	/* Sets the modal state after checks */
	void SetModal(ieDword newstate);
	/* returns current attack style */
	int GetAttackStyle();
	/* sets target for immediate attack */
	void SetTarget( Scriptable *actor);
	/* starts combat round, possibly one more attacks in every second round*/
	void InitRound(ieDword gameTime, bool secondround);
	/* gets the to hit value */
	int GetToHit(int bonus, ieDword Flags);
	/* performs attack against target */
	void PerformAttack(ieDword initiative);
	/* deal damage to target */
	void DealDamage(Actor *target, int damage,int damagetype, bool critical);
	/* sets a colour gradient stat, handles location */
	void SetColor( ieDword idx, ieDword grd);
	void RemoveTimedEffects();
	bool Schedule(ieDword gametime);
	/* call this when path needs to be changed */
	void NewPath();
	/* overridden method, won't walk if dead */
	void WalkTo(Point &Des, ieDword flags, int MinDistance = 0);
	/* resolve string constant (sound will be altered) */
	void ResolveStringConstant(ieResRef sound, unsigned int index);
	/* sets the quick slots */
	void SetActionButtonRow(ActionButtonRow &ar);
	/* updates the quick slots */
	void GetActionButtonRow(ActionButtonRow &qs);

	/* Handling automatic stance changes */
	bool HandleActorStance();

	/* if necessary, advance animation and draw actor */
	void Draw(Region &screen);

	/* add mobile vvc (spell effects) to actor's list */
	void AddVVCell(ScriptedAnimation* vvc);
	/* remove a vvc from the list, graceful means animated removal */
	void RemoveVVCell(const ieResRef vvcname, bool graceful);
	/* returns true if actor already has the overlay */
	bool HasVVCCell(const ieResRef resource);
	/* draw videocells */
	void DrawVideocells(Region &screen, vvcVector &vvcCells, Color &tint);

	void add_animation(const ieResRef resource, int gradient, int height, bool playonce);
	void PlayDamageAnimation(int x);
	/* restores a spell of maximum maxlevel level, type is a mask of disabled spells */
	int RestoreSpellLevel(ieDword maxlevel, ieDword typemask);
	/* rememorizes spells, cures fatigue, etc */
	void Rest(int hours);
	/* adds a state icon to the list */
	void AddPortraitIcon(ieByte icon);
	/* disables a state icon in the list, doesn't remove it! */
	void DisablePortraitIcon(ieByte icon);
	/* returns which slot belongs to the quickweapon slot */
	int GetQuickSlot(int slot);
	/* Sets equipped Quick slot */
	int SetEquippedQuickSlot(int slot);
	/* Uses an item on the target */
	bool UseItem(int slot, Scriptable *target);
	/* If it returns true, then default AC=10 and the lesser the better */
	bool IsReverseToHit();
	void InitButtons(ieDword cls);
	void SetFeat(unsigned int feat, int mode);
	int GetFeat(unsigned int feat);
	/* Returns nonzero if the caster is held */
	int Immobile();
};
#endif
