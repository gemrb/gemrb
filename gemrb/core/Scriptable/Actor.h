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

#ifndef ACTOR_H
#define ACTOR_H

#include "Scriptable/Scriptable.h"

#include "Scriptable/PCStatStruct.h"

#include "exports.h"
#include "ie_types.h"

#include "Audio.h"
#include "CombatInfo.h"
#include "EffectQueue.h"
#include "Palette.h"

#include <vector>

namespace GemRB {

class Animation;
class ArmorClass;
class CharAnimations;
class DataFileMgr;
class Map;
class ScriptedAnimation;
class StringBuffer;
class ToHitStats;
struct PolymorphCache;

}

/** USING DEFINITIONS AS DESCRIBED IN STATS.IDS */
#include "ie_stats.h"

#include "Inventory.h"
#include "Spellbook.h"

namespace GemRB {

#define MAX_STATS 256
#define MAX_LEVEL 128
#define MAX_FEATS 96 //3*sizeof(ieDword)

//lucky roll
#define LR_CRITICAL    1
#define LR_DAMAGELUCK  2
#define LR_NEGATIVE    4

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

//add_animation flags (override vvc)
#define AA_PLAYONCE    1
#define AA_BLEND       2

//GetDialog flags
#define GD_NORMAL      0
#define GD_CHECK       1
#define GD_FEEDBACK    2 //(also check)

//Panic modes
#define PANIC_NONE       0
#define PANIC_BERSERK    1
#define PANIC_RUNAWAY    2
#define PANIC_RANDOMWALK 3

//Game Difficulty
#define DIFF_DEFAULT       0
#define DIFF_EASY          1
#define DIFF_NORMAL        2
#define DIFF_CORE          3
#define DIFF_HARD          4
#define DIFF_NIGHTMARE     5

/** flags for GetActor */
//default action
#define GA_DEFAULT  0
//actor selected for talk
#define GA_TALK     1
//actor selected for attack
#define GA_ATTACK   2
//actor selected for spell target
#define GA_SPELL    3
//actor selected for defend
#define GA_DEFEND   4
//actor selected for pick pockets
#define GA_PICK     5
//action mask
#define GA_ACTION   15
//unselectable actor may not be selected (can still block)
#define GA_SELECT   16
//dead actor may not be selected
#define GA_NO_DEAD  32
//any point could be selected (area effect)
#define GA_POINT    64
//hidden actor may not be selected
#define GA_NO_HIDDEN 128
//party members cannot be selected
#define GA_NO_ALLY  256
//only party members could be selected
#define GA_NO_ENEMY 512
//
#define GA_NO_NEUTRAL 1024
//cannot target self
#define GA_NO_SELF    2048
//try other areas too
//(unused and removed, see git history of the commit which
//added this comment for some clues if you really need it)
//#define GA_GLOBAL     4096

//line of sight is ignored (for GetAllActorsInRadius)
#define GA_NO_LOS 4096

// Detect() mode: IDS matching ignores invisibility
#define GA_DETECT 8192
//cannot target unscheduled actors
#define GA_NO_UNSCHEDULED 16384

#define VCONST_COUNT 100

//interact types
#define I_NONE       0
#define I_INSULT     1
#define I_COMPLIMENT 2
#define I_SPECIAL    3
#define I_INSULT_RESP 4
#define I_COMPL_RESP 5
#define I_DIALOG     6

// 3 for blur, 8 for mirror images
#define EXTRA_ACTORCOVERS 11

//flags for UseItem
#define UI_SILENT    1       //no sound when used up
#define UI_MISS      2       //ranged miss (projectile has no effects)
#define UI_CRITICAL  4       //a critical hit happened
#define UI_FAKE      8       //deplete the item but don't actually apply its effects
#define UI_NOAURA    16      //ignore spellcasting aura checks

//used to mask off current profs
#define PROFS_MASK  0x07

//locations of classes in the isclass/levelslots arrays
#define ISFIGHTER   0
#define ISMAGE      1
#define ISTHIEF     2
#define ISBARBARIAN 3
#define ISBARD      4
#define ISCLERIC    5
#define ISDRUID     6
#define ISMONK      7
#define ISPALADIN   8
#define ISRANGER    9
#define ISSORCERER  10
#define ISCLASS12   11
#define ISCLASS13   12

#define ISCLASSES   13

//appearance flags

#define APP_HALFTRANS    2           //half transparent
#define APP_DEATHVAR     16          //set death variable
#define APP_DEATHTYPE    32          //count creature type deaths
#define APP_ADDKILL      64          //prepend KILL_ to the creature type
#define APP_FACTION      128         //count killed faction
#define APP_TEAM         0x100       //count killed team
#define APP_INVULNERABLE 0x200       //invulnerable
#define APP_GOOD         0x400       //good count
#define APP_LAW          0x800       //law count
#define APP_LADY         0x1000      //lady count
#define APP_MURDER       0x2000      //murder count
#define APP_NOTURN       0x4000      //doesn't face gabber in dialogue
#define APP_BUDDY        0x8000      //npcs will turn hostile if this one dies
#define APP_DEAD         0x40000000  //used by the engine to prevent dying twice

#define DC_GOOD   0
#define DC_LAW    1
#define DC_LADY   2
#define DC_MURDER 3

// used for distinguishing damage immunity from high damage resistance
#define DR_IMMUNE 999999

// wild surge target change type
#define WSTC_SETTYPE   1 // change to this target type
#define WSTC_ADDTYPE   2 // affect also this target type
#define WSTC_RANDOMIZE 3 // choose a random target
struct WildSurgeSpellMods {
	unsigned int num_castings; // number of times to cast
	unsigned int num_wildrolls; // number of times to roll
	unsigned int projectile_id; // new projectile id
	unsigned int target_change_type; // settype, addtype, randomize
	unsigned int target_type; // type to use when target_change_type is not WSTC_RANDOMIZE
	unsigned int projectile_speed_mod; // factor in percents
	int saving_throw_mod;
};

typedef ieByte ActionButtonRow[GUIBT_COUNT];
struct ActionButtonRow2 {
	ActionButtonRow buttons;
	ieByte clss;
};

typedef std::vector< ScriptedAnimation*> vvcVector;

struct WeaponInfo {
	int slot;
	ieDword enchantment;
	unsigned int range;
	ieDword itemflags;
	ieDword prof;
	bool backstabbing;
	ieDword wflags;
	int critmulti; //critical hit multiplier (usually 2)
	int critrange; // the lower value of the critical range (eg. 19 in 19-20/x3)
	int profdmgbon;
	int launcherdmgbon;
	WeaponInfo(): slot(0), enchantment(0), range(0), itemflags(0), prof(0), backstabbing(false), wflags(0), critmulti(0), critrange(0), profdmgbon(0), launcherdmgbon(0) {};
};

struct BABTable {
	ieDword level;
	int bab; // basic attack bonus
	int apr; // attacks per round
};

extern void ReleaseMemoryActor();
GEM_EXPORT void UpdateActorConfig(); //call this from guiscripts when some variable has changed

class GEM_EXPORT Actor : public Movable {
public:
	//CRE DATA FIELDS
	ieDword BaseStats[MAX_STATS];
	ieDword Modified[MAX_STATS];
	ieDword *PrevStats;
	ieByteSigned DeathCounters[4];   //PST specific (good, law, lady, murder)

	ieResRef applyWhenHittingMelee;  //set melee effect
	ieResRef applyWhenHittingRanged; //set ranged effect
	ieResRef applyWhenNearLiving;    //cast spell on condition
	ieResRef applyWhen50Damage;      //cast spell on condition
	ieResRef applyWhen90Damage;      //cast spell on condition
	ieResRef applyWhenEnemySighted;  //cast spell on condition
	ieResRef applyWhenPoisoned;      //cast spell on condition
	ieResRef applyWhenHelpless;      //cast spell on condition
	ieResRef applyWhenAttacked;      //cast spell on condition
	ieResRef applyWhenBeingHit;      //cast spell on condition
	ieResRef ModalSpell;             //apply this spell once per round
	ieResRef LingeringModalSpell;    //apply this spell once per round if the effects are lingering
	ieResRef BardSong;               //custom bard song (updated by fx)
	ieResRef BackstabResRef;         //apply on successful backstab

	PCStatsStruct*  PCStats;
	ieResRef SmallPortrait;
	ieResRef LargePortrait;
	/** 0: NPC, 1-8 party slot */
	ieByte InParty;
	char* LongName, * ShortName;
	ieStrRef ShortStrRef, LongStrRef;
	ieStrRef StrRefs[VCONST_COUNT];

	ieDword AppearanceFlags;

	ieVariable KillVar; //this second field is present in pst, iwd1 and iwd2
	ieVariable IncKillVar; // iwd1, iwd2

	ieByte SetDeathVar, IncKillCount, UnknownField; // boolean fields from iwd1 and iwd2

	Inventory inventory;
	Spellbook spellbook;
	//savefile version (creatures embedded in area)
	int version;
	//in game or area actor header
	ieDword TalkCount;
	ieDword RemovalTime;
	//FIXME: this is definitely not the same in bg2, in bg2 there are joinable npcs
	//which keep a matrix of counters
	ieDword InteractCount; //this is accessible in iwd2, probably exists in other games too
	ieDword appearance;
	ieDword ModalState;
	int PathTries; //the # of previous tries to pick up a new walkpath
	ArmorClass AC;
	ToHitStats ToHit;
public:
	ieDword LastExit;    //the global ID of the exit to be used
	ieDword UsedExit;
	ieResRef LastArea;
	char ShieldRef[2];
	char HelmetRef[2];
	char WeaponRef[2];
	int WeaponType;
	ieDword multiclass;
	bool GotLUFeedback;
	int WMLevelMod;

	int LastDamageType;
	int LastDamage;
	Point FollowOffset;//follow lastfollowed at this offset
	Point HomeLocation;//spawnpoint, return here after rest
	bool Spawned;      //has been created by a spawn point

	ieDword TargetDoor;

	EffectQueue fxqueue;
	vvcVector vvcOverlays;
	vvcVector vvcShields;
	ieDword *projectileImmunity; //classic bitfield
	Holder<SoundHandle> casting_sound;
	ieDword roundTime;           //these are timers for attack rounds
	ieDword modalTime;           //last time the modal effect used
	char modalSpellLingering;    //the count of rounds for which the modal spell will be reapplied after the state ends
	ieDword panicMode;           //runaway, berserk or randomwalk
	ieDword nextComment;         //do something random (area comment, interaction)
	ieDword nextBored;           //do something when bored
	ieDword lastInit;
	int speed;
	//how many attacks left in this round, must be public for cleave opcode
	int attackcount;

	PolymorphCache *polymorphCache; // fx_polymorph etc
	WildSurgeSpellMods wildSurgeMods;
	ieByte DifficultyMargin;
private:
	//this stuff doesn't get saved
	CharAnimations* anims;
	SpriteCover* extraCovers[EXTRA_ACTORCOVERS];
	ieByte SavingThrow[5];
	// true when command has been played after select
	bool playedCommandSound;
	//true every second round of attack
	bool secondround;
	int attacksperround;
	//time of our next attack
	ieDword nextattack;
	ieDword nextWalk;
	ieDword lastattack;
	//trap we're trying to disarm
	ieDword disarmTrap;
	ieDword InTrap;
	char AttackStance;
	/*The projectile bringing the current attack*/
	Projectile* attackProjectile ;
	int TicksLastRested;
	/** paint the actor itself. Called internally by Draw() */
	void DrawActorSprite(const Region &screen, int cx, int cy, const Region& bbox,
				SpriteCover*& sc, Animation** anims,
				unsigned char Face, const Color& tint);

	/** fixes the palette */
	void SetupColors();
	/** debugging function, gets the scripting name of an actor referenced by a global ID */
	const char* GetActorNameByID(ieDword ID) const;
	/* checks a weapon quick slot and resets it to fist if it is empty */
	void CheckWeaponQuickSlot(unsigned int which);
	/* helper for usability checks */
	int CheckUsability(Item *item) const;
	/* Set up all the missing stats on load time, or after level up */
	void CreateDerivedStatsBG();
	/* Set up all the missing stats on load time, or after level up */
	void CreateDerivedStatsIWD2();
	/* Gets the given ISCLASS level */
	ieDword GetClassLevel (const ieDword isclass) const;
	/* Returns true if the dual class is backwards */
	bool IsDualSwap() const;
	/* returns the weapon proficiency stat of the actor */
	int GetProficiency(int proftype) const;
	/** Re/Inits the Modified vector for PCs/NPCs */
	void RefreshPCStats();
	void RefreshHP();
	bool ShouldHibernate();
	void ApplyClassClab(int cls, bool remove);
	bool ShouldDrawCircle() const;
	bool HasBodyHeat() const;
	void SetupFistData();
	void UpdateFatigue();
	int GetSneakAttackDamage(Actor *target, WeaponInfo &wi, int &multiplier, bool weaponImmunity);
	int GetBackstabDamage(Actor *target, WeaponInfo &wi, int multiplier, int damage) const;
public:
	Actor(void);
	~Actor(void);
	/** releases memory */
	static void ReleaseMemory();
	/** sets game specific parameter (which stat should determine the fist weapon type */
	static void SetFistStat(ieDword stat);
	/** sets game specific default data about action buttons */
	static void SetDefaultActions(int qslot, ieByte slot1, ieByte slot2, ieByte slot3);
	/** prints useful information on console */
	void dump() const;
	/** prints useful information to given buffer */
	void dump(StringBuffer&) const;
	/** fixes the feet circle */
	void SetCircleSize();
	/** places the actor on the map */
	void SetMap(Map *map);
	/** sets the actor's position, calculating with the nojump flag*/
	void SetPosition(const Point &position, int jump, int radiusx=0, int radiusy=0);
	/** you better use SetStat, this stuff is only for special cases*/
	void SetAnimationID(unsigned int AnimID);
	/** returns the animations */
	CharAnimations* GetAnims() const;
	/** returns the gender of actor for cg sound - illusions are tricky */
	ieDword GetCGGender();
	/** some hardcoded effects in puppetmaster based on puppet type */
	void CheckPuppet(Actor *puppet, ieDword type);
	/** Re/Inits the Modified vector */
	void RefreshEffects(EffectQueue *eqfx);
	/** gets saving throws */
	void RollSaves();
	/** returns a saving throw */
	bool GetSavingThrow(ieDword type, int modifier, int spellLevel=0, int saveBonus=0);
	/** Returns true if the actor is targetable */
	bool ValidTarget(int ga_flags, Scriptable *checker = NULL) const;
	/** Clamps a stat value to the valid range for the respective stat */
	ieDword ClampStat(unsigned int StatIndex, ieDword Value) const;
	/** Returns a Stat value */
	ieDword GetStat(unsigned int StatIndex) const;
	/** Returns a safe Stat value, one, that is not partially computed */
	ieDword GetSafeStat(unsigned int StatIndex) const;
	/** Sets a Stat Value (unsaved) */
	bool SetStat(unsigned int StatIndex, ieDword Value, int pcf);
	/** Returns the difference */
	int GetMod(unsigned int StatIndex);
	/** Returns a Stat Base Value */
	ieDword GetBase(unsigned int StatIndex) const;
	/** Sets a Base Stat Value */
	bool SetBase(unsigned int StatIndex, ieDword Value);
	bool SetBaseNoPCF(unsigned int StatIndex, ieDword Value);
	/** set/resets a Base Stat bit */
	bool SetBaseBit(unsigned int StatIndex, ieDword Value, bool setreset);
	/** Sets the modified value in different ways, returns difference */
	int NewStat(unsigned int StatIndex, ieDword ModifierValue, ieDword ModifierType);
	/** Modifies the base stat value in different ways, returns difference */
	int NewBase(unsigned int StatIndex, ieDword ModifierValue, ieDword ModifierType);
	void SetLeader(Actor *actor, int xoffset=0, int yoffset=0);
	/** Sets the Icon ResRef */
	//Which - 0 both, 1 Large, 2 Small
	void SetPortrait(const char* ResRef, int Which=0);
	void SetSoundFolder(const char *soundset);
	void GetSoundFolder(char *soundset, int flag) const;
	/** Gets the Character Long Name/Short Name */
	char* GetName(int which) const
	{
		if(which==-1) which=TalkCount;
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
	/** Guesses the (base) casting level */
	ieDword GetCasterLevel(int spelltype);
	ieDword GetBaseCasterLevel(int spelltype, int flags=0) const;
	ieDword GetAnyActiveCasterLevel() const;
	/** Returns the wild mage casting level modifier */
	int GetWildMod(int level);
	/** Returns any casting level modifier */
	int CastingLevelBonus(int level, int type);

	/** Gets the Dialog ResRef */
	const char* GetDialog(int flags=GD_NORMAL) const;
	void SetDialog(const ieResRef resref);
	/** Gets the Portrait ResRef */
	const char* GetPortrait(int which) const
	{
		return which ? SmallPortrait : LargePortrait;
	}

	/** Gets the attack projectile */
	Projectile* GetAttackProjectile()
	{
		return attackProjectile;
	}
	void SetName(const char* ptr, unsigned char type);
	void SetName(int strref, unsigned char type);
	/* calculates speed, encumbrance etc */
	int CalculateSpeed(bool feedback);
	/* checks on death of actor, returns true if it should be removed*/
	bool CheckOnDeath();
	/* receives undead turning message */
	void Turn(Scriptable *cleric, ieDword turnlevel);
	/* call this on gui selects */
	void PlaySelectionSound();
	/* call this when adding actions via gui */
	void CommandActor(Action* action);
	/** handle panic and other involuntary actions that mess with scripting */
	bool OverrideActions();
	/** handle idle actions, that shouldn't mess with scripting */
	void IdleActions(bool nonidle);
	/* sets the actor in panic (turn/morale break) */
	void Panic(Scriptable *attacker, int panicmode);
	/* sets a multi class flag (actually this is a lot of else too) */
	void SetMCFlag(ieDword bitmask, int op);
	/* inlined dialogue start */
	void Interact(int type);
	/* returns a remapped verbal constant strref */
	ieStrRef GetVerbalConstant(int index) const;
	/* displaying a random verbal constant */
	void VerbalConstant(int start, int count) const;
	/* display string or verbal constant depending on what is available */
	void DisplayStringOrVerbalConstant(int str, int vcstat, int vccount) const;
	/* inlined dialogue response */
	void Response(int type) const;
	/* called when someone died in the party */
	void ReactToDeath(const char *deadname);
	/* sends trigger_died to everyone in visual range */
	void SendDiedTrigger();
	/* called when someone talks to Actor */
	void DialogInterrupt();
	/* called when actor was hit */
	void GetHit(int damage=0, int spellLevel=0);
	/* checks whether taking damage should disrupt spellcasting */
	bool CheckSpellDisruption(int damage, int spellLevel);
	/* called when actor starts to cast a spell*/
	bool HandleCastingStance(const ieResRef SpellResRef, bool deplete, bool instant);
	/* check if the actor should be just knocked out by a lethal hit */
	bool AttackIsStunning(int damagetype) const;
	/* check if the actor is silenced - for casting purposes */
	bool CheckSilenced();
	/* check and perform a cleave movement */
	void CheckCleave();
	/* deals damage to this actor */
	int Damage(int damage, int damagetype, Scriptable *hitter, int modtype=MOD_ADDITIVE, int critical=0, int saveflags=0);
	/* displays the damage taken and other details (depends on the game type) */
	void DisplayCombatFeedback (unsigned int damage, int resisted, int damagetype, Scriptable *hitter);
	/* play a random footstep sound */
	void PlayWalkSound();
	/* play the proper hit sound (in pst) */
	void PlayHitSound(DataFileMgr *resdata, int damagetype, bool suffix);
	/* drops items from inventory to current spot */
	void DropItem(const ieResRef resref, unsigned int flags);
	void DropItem(int slot, unsigned int flags);
	/* returns item information in quickitem slot */
	void GetItemSlotInfo(ItemExtHeader *item, int which, int header);
	/* returns spell information in quickspell slot */
	void GetSpellSlotInfo(SpellExtHeader *spell, int which);
	/* updates quickslots */
	void ReinitQuickSlots();
	/* actor is in trap */
	void SetInTrap(ieDword tmp);
	/* sets some of the internal flags */
	void SetRunFlags(ieDword flags);
	/* applies the kit abilities, returns false if kit is not applicable */
	bool ApplyKit(bool remove);
	/* applies the class abilities*/
	void ApplyClab(const char *clab, ieDword max, bool remove);
	/* calls InitQuickSlot in PCStatStruct */
	void SetupQuickSlot(unsigned int which, int slot, int headerindex);
	/* returns true if the actor is PC/joinable*/
	bool Persistent() const;
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
	void GetPrevAnimation();
	/* debug function */
	void GetNextStance();
	/* learns the given spell, possibly receive XP */
	int LearnSpell(const ieResRef resref, ieDword flags);
	/* returns the ranged weapon header associated with the currently equipped projectile */
	ITMExtHeader *GetRangedWeapon(WeaponInfo &wi) const;
	/* Returns current weapon range and extended header
	if range is nonzero, then which is valid */
	ITMExtHeader* GetWeapon(WeaponInfo &wi, bool leftorright=false) const;
	/* Creates player statistics */
	void CreateStats();
	/* Heals actor */
	void Heal(int hp);
	/* Receive experience (handle dual/multi class) */
	void AddExperience(int exp, int combat);
	/* Calculate experience bonus */
	int CalculateExperience(int type, int level);
	/* Sets the modal state after checks */
	void SetModal(ieDword newstate, bool force=1);
	/* Sets the modal spell after checks */
	void SetModalSpell(ieDword state, const char *spell);
	/* returns current attack style */
	int GetAttackStyle() const;
	/* adds the combatants to the attackers list */
	void AttackedBy(Actor *actor);
	/* reorients to face target (for immediate attack) */
	void FaceTarget(Scriptable *actor);
	/* returns the number of attacks (handles monk barehanded bonus) */
	ieDword GetNumberOfAttacks();
	/* starts combat round*/
	void InitRound(ieDword gameTime);
	/* returns melee penalty */
	int MeleePenalty() const;
	/* gets the to hit value */
	int GetToHit(ieDword Flags, Actor* target);
	void GetTHAbilityBonus(ieDword Flags);
	/* gets the defense against an attack */
	int GetDefense(int DamageType, ieDword wflags, Actor *attacker);
	/* checks if something is wrong with the weapon we are using for the attack */
	bool WeaponIsUsable(bool leftorright, ITMExtHeader *header=NULL) const;
	/* get the current hit bonus */
	bool GetCombatDetails(int &tohit, bool leftorright, WeaponInfo &wi, ITMExtHeader *&header, ITMExtHeader *&hittingheader,\
		int &DamageBonus, int &speed, int &CriticalBonus, int &style, Actor *target);
	/* performs attack against target */
	void PerformAttack(ieDword gameTime);
	/* returns the adjusted weapon range, since items have odd values stored */
	int GetWeaponRange(const WeaponInfo &wi) const;
	/* filter out any damage reduction that is cancelled by high weapon enchantment and return the resulting resistance */
	int GetDamageReduction(int resist_stat, ieDword weaponEnchantment) const;
	/* calculates strength (dexterity) based damage adjustment */
	int WeaponDamageBonus(const WeaponInfo &wi) const;
	/* handles critical, backstab, etc. damage modifications */
	void ModifyWeaponDamage(WeaponInfo &wi, Actor *target, int &damage, bool &critical);
	/* adjusts damage dealt to this actor, handles mirror images  */
	void ModifyDamage(Scriptable *hitter, int &damage, int &resisted, int damagetype);
	/* applies modal spell etc, if needed */
	void UpdateActorState(ieDword gameTime);
	/* returns the hp adjustment based on constitution */
	int GetHpAdjustment(int multiplier) const;
	/* does all the housekeeping after loading the actor from file */
	void InitStatsOnLoad();
	/* sets a colour gradient stat, handles location */
	void SetColor( ieDword idx, ieDword grd);
	/* sets an RGB colour modification effect; location 0xff for global */
	void SetColorMod( ieDword location, RGBModifier::Type type, int speed,
		unsigned char r, unsigned char g, unsigned char b,
		int phase=-1 );
	bool Schedule(ieDword gametime, bool checkhide) const;
	/* call this when path needs to be changed */
	void NewPath();
	/* overridden method, won't walk if dead */
	void WalkTo(const Point &Des, ieDword flags, int MinDistance = 0);
	/* resolve string constant (sound will be altered) */
	void ResolveStringConstant(ieResRef sound, unsigned int index) const;
	void GetSoundFromINI(ieResRef Sound, unsigned int index) const;
	void GetSoundFrom2DA(ieResRef Sound, unsigned int index) const;
	/* generate area specific oneliner */
	void GetAreaComment(int areaflag) const;
	/* handle oneliner interaction, -1: unsuccessful (may comment area), 0: dialog banter, 1: oneliner */
	int HandleInteract(Actor *target);
	/* generate party banter, return true if successful */
	bool GetPartyComment();
	/* sets the quick slots */
	void SetActionButtonRow(ActionButtonRow &ar);
	/* updates the quick slots */
	void GetActionButtonRow(ActionButtonRow &qs);
	/* converts the iwd2 qslot index to our internal representation */
	int IWD2GemrbQslot (int slotindex);

	/* Handling automatic stance changes */
	bool HandleActorStance();

	/* if necessary, advance animation */
	void UpdateAnimations();
	/* if necessary, draw actor */
	void Draw(const Region &screen);
	bool DoStep(unsigned int walk_speed, ieDword time = 0);

	/* add mobile vvc (spell effects) to actor's list */
	void AddVVCell(ScriptedAnimation* vvc);
	/* remove a vvc from the list, graceful means animated removal */
	void RemoveVVCell(const ieResRef vvcname, bool graceful);
	/* returns true if actor already has the overlay (slow) */
	bool HasVVCCell(const ieResRef resource) const;
	/* returns overlay (or underlay) if actor already has it, faster */
	ScriptedAnimation *GetVVCCell(const vvcVector *vvcCells, const ieResRef resource) const;
	/* returns overlay if actor already has it (slow) */
	ScriptedAnimation *GetVVCCell(const ieResRef resource) const;
	/* returns the vvc pointer to a hardcoded overlay */
	/* if it exists (faster than hasvvccell) */
	ScriptedAnimation *FindOverlay(int index) const;
	int WantDither() const;
	/* draw videocells */
	void DrawVideocells(const Region &screen, vvcVector &vvcCells, const Color &tint);

	void SetLockedPalette(const ieDword *gradients);
	void UnlockPalette();
	void AddAnimation(const ieResRef resource, int gradient, int height, int flags);
	/* plays damage animation, if hit is not set, then plays only the splash part */
	void PlayDamageAnimation(int x, bool hit=true);
	void PlayCritDamageAnimation(int x);
	/* returns mage or cleric spell casting failure, iwd2 compatible */
	ieDword GetSpellFailure(bool arcana) const;
	/* returns the dexterity AC adjusted by armor, iwd2 compatible */
	int GetDexterityAC() const;
	/* returns the monk wisdom AC adjusted by armor (iwd2) */
	int GetWisdomAC() const;
	/* PST specific criticals */
	int GetCriticalType() const;
	/* restores a spell of maximum maxlevel level, type is a mask of disabled spells */
	int RestoreSpellLevel(ieDword maxlevel, ieDword typemask);
	/* rememorizes spells, cures fatigue, etc */
	void Rest(int hours);
	/* returns the portrait icons list */
	const unsigned char *GetStateString() const;
	/* adds a state icon to the list */
	void AddPortraitIcon(ieByte icon);
	/* disables a state icon in the list, doesn't remove it! */
	void DisablePortraitIcon(ieByte icon);
	/* returns which slot belongs to the quickweapon slot */
	int GetQuickSlot(int slot) const;
	/* Sets equipped Quick slot, if header is -1, then use the current one */
	int SetEquippedQuickSlot(int slot, int header);
	/* Uses an item on the target or point */
	bool UseItemPoint(ieDword slot, ieDword header, const Point &point, ieDword flags);
	bool UseItem(ieDword slot, ieDword header, Scriptable *target, ieDword flags, int damage = 0);
	/* Deducts a charge from an item */
	void ChargeItem(ieDword slot, ieDword header, CREItem *item, Item *itm, bool silent);
	/* If it returns true, then default AC=10 and the lesser the better */
	static int IsReverseToHit();
	/* initialize the action buttons based on class. If forced, it will override
		previously customized or set buttons. */
	void InitButtons(ieDword cls, bool forced);
	int GetMaxEncumbrance() const;
	int GetAbilityBonus(unsigned int ability, int value = -1) const;
	int GetSkillStat(unsigned int skill) const;
	int GetSkill(unsigned int skill) const;
	int GetFeat(unsigned int feat) const;
	void SetFeat(unsigned int feat, int mode);
	void SetFeatValue(unsigned int feat, int value);
	void SetUsedWeapon(const char *AnimationType, ieWord *MeleeAnimation,
		int WeaponType=-1);
	void SetUsedShield(const char *AnimationType, int WeaponType=-1);
	void SetUsedHelmet(const char *AnimationType);
	void SetupFist();
	/* Returns nonzero if the caster is held */
	int Immobile() const;
	/* Returns strref if the item is unusable due to name/type restrictions */
	ieStrRef Disabled(ieResRef name, ieDword type) const;
	/* Returns constant string if the item is unusable */
	int Unusable(Item *item) const;
	/* Sets all clown colour to the given gradient */
	void SetGradient(ieDword gradient);
	/* Enables an overlay */
	void SetOverlay(unsigned int overlay);
	/* Checks and sets a spellstate if it wasn't set yet */
	bool SetSpellState(unsigned int spellstate);
	/* Checks a spellstate */
	bool HasSpellState(unsigned int spellstate) const;
	/* Checks a feat */
	bool HasFeat(unsigned int featindex) const;
	/* Reports projectile immunity, nonzero if immune */
	ieDword ImmuneToProjectile(ieDword projectile) const;
	/* Sets projectile immunity */
	void AddProjectileImmunity(ieDword projectile);
	/* Apply feats */
	void ApplyFeats();
	/* reapply modal feat spells */
	void ApplyExtraSettings();
	/* Set up all the missing stats on load time, chargen, or after level up */
	void CreateDerivedStats();
	/* Checks if the actor is multiclassed (excluding dualclassed actors)) */
	bool IsMultiClassed() const;
	/* Checks if the actor is dualclassed */
	bool IsDualClassed() const;
	/* Returns an exact copy of this actor */
	Actor *CopySelf(bool mislead) const;
	/* Returns the actor's level of the given class */
	ieDword GetFighterLevel() const { return GetClassLevel(ISFIGHTER); }
	ieDword GetMageLevel() const { return GetClassLevel(ISMAGE); }
	ieDword GetThiefLevel() const { return GetClassLevel(ISTHIEF); }
	ieDword GetBarbarianLevel() const { return GetClassLevel(ISBARBARIAN); }
	ieDword GetBardLevel() const { return GetClassLevel(ISBARD); }
	ieDword GetClericLevel() const { return GetClassLevel(ISCLERIC); }
	ieDword GetDruidLevel() const { return GetClassLevel(ISDRUID); }
	ieDword GetMonkLevel() const { return GetClassLevel(ISMONK); }
	ieDword GetPaladinLevel() const { return GetClassLevel(ISPALADIN); }
	ieDword GetRangerLevel() const { return GetClassLevel(ISRANGER); }
	ieDword GetSorcererLevel() const { return GetClassLevel(ISSORCERER); }
	/* Returns true if the character is a warrior */
	ieDword GetWarriorLevel() const;
	bool IsWarrior() const { return (GetFighterLevel()||GetBarbarianLevel()||GetRangerLevel()||GetPaladinLevel()); }
	/* Returns true if the old class is inactive */
	bool IsDualInactive() const;
	/* true if we are dual-wielding */
	int IsDualWielding() const;
	bool BlocksSearchMap() const;
	bool CannotPassEntrance(ieDword exitID) const;
	void UseExit(ieDword exitID);
	//int GetReaction() const;
	/* Similar to Roll, but takes luck into account */
	int LuckyRoll(int dice, int size, int add, ieDword flags=LR_CRITICAL, Actor* opponent=NULL) const;
	/* removes normal invisibility (type 0) */
	void CureInvisibility();
	/* removes sanctuary */
	void CureSanctuary();
	/* resets the invisibility, sanctuary and modal states */
	void ResetState();
	/* checks whether the actor is behind the target */
	bool IsBehind(Actor* target) const;
	/* checks whether the target is the actor's racial enemy */
	int GetRacialEnemyBonus(Actor* target) const;
	/* checks whether the actor can stay in the current modal state */
	bool ModalSpellSkillCheck();
	/* check if this actor is seen by or seeing anyone */
	bool SeeAnyOne(bool enemy, bool seen);
	/* does all the game logic checks to see if the actor can hide */
	bool TryToHide();
	bool TryToHideIWD2();
	/* checks if the alignment matches one of the masking constants */
	//bool MatchesAlignmentMask(ieDword mask);
	/** untargetable by spells/attack due to invisibility or sanctuary */
	bool Untargetable();
	/* returns true if this it is futile to try to harm actor (dead/sanctuaried) */
	bool InvalidSpellTarget() const;
	/* returns true if the spell is useless to cast on target
	or the spell's range is smaller than range */
	bool InvalidSpellTarget(int spellnum, Actor *caster, int range) const;
	/* returns true if the lightmap under the actor is dark */
	bool PCInDark() const;
	/* function to get a class level used by scripting */
	ieDword GetLevelInClass (const ieDword classid) const;
	/* computes the actor's classmask (iwd2) */
	int GetClassMask() const;
	/* computes the actor's usable books (iwd2) */
	int GetBookMask() const;
	/* computes the thieving skill bonus from race and dexterity */
	int GetSkillBonus(unsigned int col) const;
	/* returns true for party members (and familiars) */
	bool IsPartyMember() const;
	/* resets the bored and area comment timers */
	void ResetCommentTime();
	/* returns the armor check penalty */
	int GetArmorSkillPenalty(int profcheck=1) const;
	int GetArmorSkillPenalty(int profcheck, int &armor, int &shield) const;
	int GetTotalArmorFailure() const;
	int GetArmorFailure(int &armor, int &shield) const;
	bool IsDead() const;
	bool IsInvisibleTo(Scriptable *checker) const;
	int UpdateAnimationID(bool derived);
	void MovementCommand(char *command);
	/* shows hp/maxhp as overhead text */
	void DisplayHeadHPRatio();
	/* if Lasttarget is gone, call this */
	void StopAttack();
	int SetBaseAPRandAB(bool CheckRapidShot);
	int BAB2APR(int pBAB, int pBABDecrement, int CheckRapidShot) const;
	/* set to trap id if current action is disarm; unset after */
	void SetDisarmingTrap(ieDword trapId) { disarmTrap = trapId; }
	ieDword GetDisarmingTrap() const { return disarmTrap; }
	void ReleaseCurrentAction();
	bool ConcentrationCheck() const;
};
}

#endif
