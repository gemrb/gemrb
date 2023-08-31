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
#include "ie_stats.h" // using definitions as described in stats.ids
#include "ie_types.h"
#include "strrefs.h"

#include "Audio.h"
#include "CombatInfo.h"
#include "EffectQueue.h"
#include "Game.h"
#include "Inventory.h"
#include "Palette.h"
#include "Polygon.h"
#include "Spellbook.h"
#include "Video/Video.h"

#include <array>
#include <map>
#include <set>
#include <vector>

namespace GemRB {

class Animation;
class ArmorClass;
class CharAnimations;
class DataFileMgr;
class Map;
class ScriptedAnimation;
class ToHitStats;
struct PolymorphCache;

}

namespace GemRB {

enum CREVersion {
	GemRB,
	V1_0 = 10, // bg1
	V1_1 = 11, // bg2 (still V1.0)
	V1_2 = 12, // pst
	V2_2 = 22, // iwd2
	V9_0 = 90 // iwd
};

#define MAX_STATS 256
#define MAX_LEVEL 128
#define MAX_FEATS 96 //3*sizeof(ieDword)

//lucky roll
#define LR_CRITICAL    1
#define LR_DAMAGELUCK  2
#define LR_NEGATIVE    4

// modal states
enum class Modal : ieWord {
	None,
	BattleSong,
	DetectTraps,
	Stealth,
	TurnUndead,
	ShamanDance,

	count
};

//stat modifier type
#define MOD_ADDITIVE  0
#define MOD_ABSOLUTE  1
#define MOD_PERCENT   2
#define MOD_MULTIPLICATIVE 3
#define MOD_DIVISIVE  4
#define MOD_MODULUS   5
#define MOD_LOGAND    6
#define MOD_LOGOR     7
#define MOD_BITAND    8
#define MOD_BITOR     9
#define MOD_INVERSE   10

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
#define DIFF_INSANE        5
#define DIFF_NIGHTMARE     6 // rather check against the ini var where needed

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

#define GA_ONLY_BUMPABLE 32768
#define GA_CAN_BUMP 65536
#define GA_BIGBAD 131072

//interact types
#define I_NONE       0
#define I_INSULT     1
#define I_COMPLIMENT 2
#define I_SPECIAL    3
#define I_INSULT_RESP 4
#define I_COMPL_RESP 5
#define I_DIALOG     6

//flags for UseItem
#define UI_SILENT    1       //no sound when used up
#define UI_MISS      2       //ranged miss (projectile has no effects)
#define UI_CRITICAL  4       //a critical hit happened
#define UI_FAKE      8       //deplete the item but don't actually apply its effects
#define UI_NOAURA    16      //ignore spellcasting aura checks
#define UI_NOCHARGE  32      //don't deplete the item

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
#define APP_BUDDY        0x8000      // unused; supposedly: npcs will turn hostile if this one dies
#define APP_DEAD         0x40000000  //used by the engine to prevent dying twice

#define DC_GOOD   0
#define DC_LAW    1
#define DC_LADY   2
#define DC_MURDER 3

// used for distinguishing damage immunity from high damage resistance
#define DR_IMMUNE 999999

constexpr int STYLE_STAR_MAX = 3;

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

using ActionButtonRow = std::array<ieByte, GUIBT_COUNT>;
struct ActionButtonRow2 {
	ActionButtonRow buttons;
	ieByte clss;
};

// internal flags for calculating to hit and similar
#define WEAPON_FIST        0
#define WEAPON_MELEE       1
#define WEAPON_RANGED      2
#define WEAPON_STYLEMASK   15
#define WEAPON_LEFTHAND    16
#define WEAPON_USESTRENGTH 32
#define WEAPON_USESTRENGTH_DMG 64
#define WEAPON_USESTRENGTH_HIT 128
#define WEAPON_FINESSE     256
#define WEAPON_BYPASS      0x10000
#define WEAPON_KEEN        0x20000
#define WEAPON_BREAKABLE   0x40000

struct WeaponInfo {
	int slot = 0;
	ieDword enchantment = 0;
	unsigned int range = 0;
	ieDword itemtype = 0;
	ieDword itemflags = 0;
	ieByte prof = 0;
	bool backstabbing = false;
	ieDword wflags = 0;
	int critmulti = 0; // critical hit multiplier (usually 2)
	int critrange = 0; // the lower value of the critical range (eg. 19 in 19-20/x3)
	int profdmgbon = 0;
	int launcherDmgBonus = 0;
	int launcherTHAC0Bonus = 0;
	const Item* item = nullptr;
	const ITMExtHeader* extHeader = nullptr;
};

struct BABTable {
	ieDword level;
	int bab; // basic attack bonus
	int apr; // attacks per round
};

struct ModalStatesStruct {
	ResRef spell;
	ieVariable action;
	ieStrRef entering_str;
	ieStrRef leaving_str;
	ieStrRef failed_str;
	unsigned int aoe_spell;
	unsigned int repeat_msg;
};

struct ModalState {
	Modal State = Modal::None;
	ResRef Spell; // apply this (modal) spell once per round
	ResRef LingeringSpell; // apply this spell once per round if the effects are lingering
	char LingeringCount = 0; // the count of rounds for which the modal spell will be reapplied after the state ends
	ieDword LastApplyTime = 0; // last time the modal spell was applied
	bool FirstApply = true; // running for the first time?
};

struct SaveInfo {
	ieByte savingThrow[5]{};
	// for feedback rate limiting
	ieDword prevType = ieDword(-1);
	int prevRoll = -1;
};

enum DamageFlags {
	DrainFromTarget = 0,
	DrainFromSource = 1,
	FistOnly = 2,
	DrainFromTargetNC = 4,
	DrainFromSourceNC = 8,
	NoFeedback = 0x10,
	CapToTarget = 0x20,
	CapToSource = 0x40,
	SaveForHalf = 0x80,
	FailForHalf = 0x100,
	NoAwake = 0x200
};

// verbal constant (bg2 soundoff.ids / sndslot.ids), we have a lookup table (vcremap) for other games
// an index into VCMap
enum class Verbal : unsigned int {
	InitialMeet = 0,
	Panic = 1,
	Happy = 2,
	Unhappy = 3, // UNHAPPY_ANNOYED
	UnhappySerious = 4,
	BreakingPoint = 5,
	Leader = 6,
	Tired = 7,
	Bored = 8,
	BattleCry = 9, // 5 battle cries
	Attack1 = 14,
	Attack2 = 15,
	Attack3 = 16,
	Attack4 = 17,
	Damage = 18,
	Die = 19,
	Hurt = 20,
	AreaForest = 21, // these 5 are handled through comment.2da / Actor::GetAreaComment
	AreaCity = 22,
	AreaDungeon = 23,
	AreaDay = 24,
	AreaNight = 25,
	Select = 26,
	Select2 = 27,
	Select3 = 28,
	Select4 = 29,
	Select5 = 30,
	Select6 = 31,
	Command = 32, // -34 selected action; -38 rare action, -43 interaction
	Insult = 44, // -46
	Compliment = 47, // -49
	Special = 50, // -52
	React = 53, // REACT_TO_DIE_GENERAL
	ReactSpecific = 54, // REACT_TO_DIE_SPECIFIC
	Resp2Compliment = 55, // -57; 55 also double labeled as MISCELLANEOUS in iwd/bg2ee sndslot.ids
	Resp2Insult = 58, // -60
	Hostile = 61,
	Dialog = 62, // DIALOG_DEFAULT
	SelectRare = 63, // -64
	CritHit = 65,
	CritMiss = 66,
	WeaponIneffective = 67, // TARGET_IMMUNE
	InventoryFull = 68,
	PickedPocket = 69, // same as Existence1, but only used by joinables
	Existence1 = 69, // same as PickedPocket
	Hide = 70, // ees have this as Existence1 in sndslot.ids??
	SpellDisrupted = 71,
	TrapSet = 72,
	Existence4 = 73, // unused in bg2, NI appears to have a gap of 1 — this should be Existence5 already
	Bio = 74, // Existence5, biography for npcs

	Noop = 97, // to disable a VB
	Select7 = 98, // iwd-only, gets remapped in vcremap.2da
	Attack0 = 99, // for our internal use only
	count = 100
};

GEM_EXPORT void UpdateActorConfig(); //call this from guiscripts when some variable has changed

bool VVCSort(const ScriptedAnimation* lhs, const ScriptedAnimation* rhs);
using vvcSet = std::multiset<ScriptedAnimation*, decltype(VVCSort)*>;
using vvcDict = std::multimap<ResRef, ScriptedAnimation*>;

class GEM_EXPORT Actor : public Movable {
public:
	using stat_t = ieDword;
	using stats_t = std::array<stat_t, MAX_STATS>;
	//CRE DATA FIELDS
	stats_t BaseStats {};
	stats_t Modified {};
	stat_t* PrevStats = nullptr;
	ieByteSigned DeathCounters[4]{}; // PST specific (good, law, lady, murder)

	ResRef BardSong;               //custom bard song (updated by fx)
	ResRef BackstabResRef = "*";         //apply on successful backstab

	PCStatsStruct* PCStats = nullptr;
	PCStatsStruct::StateArray previousStates;
	ResRef SmallPortrait;
	ResRef LargePortrait;
	/** 0: NPC, 1-8 party slot */
	ieByte InParty = 0;
	
	ieStrRef ShortStrRef = ieStrRef(-1);
	ieStrRef LongStrRef = ieStrRef(-1);
	EnumArray<Verbal, ieStrRef> StrRefs;

	ieDword AppearanceFlags = 0;

	ieVariable KillVar; //this second field is present in pst, iwd1 and iwd2
	ieVariable IncKillVar; // iwd1, iwd2

	// boolean fields from iwd1 and iwd2
	ieByte SetDeathVar = 0;
	ieByte IncKillCount = 0;
	ieByte UnknownField = 0;

	Inventory inventory;
	Spellbook spellbook;
	//savefile version (creatures embedded in area)
	int creVersion = 0;
	//in game or area actor header
	ieDword TalkCount = 0;
	ieDword RemovalTime = 0;
	//FIXME: this is definitely not the same in bg2, in bg2 there are joinable npcs
	//which keep a matrix of counters
	ieDword InteractCount = 0; // this is accessible in iwd2, probably exists in other games too
	ieDword appearance = 0xffffff; // schedule
	ArmorClass AC;
	ToHitStats ToHit;
	WeaponInfo weaponInfo[2]{};
	ModalState Modal{};

	bool usedLeftHand = false; // which weaponInfo index was used in an attack last
	ieDword LastExit = 0;    // the global ID of the exit to be used
	ieVariable UsedExit; // name of the exit, since global id is not stable after loading a new area
	ResRef LastArea;
	AnimRef ShieldRef;
	AnimRef HelmetRef;
	AnimRef WeaponRef;
	unsigned char WeaponType = 0;
	ieDword multiclass = 0;
	bool GotLUFeedback = false;
	int WMLevelMod = 0;

	int LastDamageType = 0;
	int LastDamage = 0;
	Point FollowOffset;//follow lastfollowed at this offset
	bool Spawned = false; // has been created by a spawn point

	ieDword TargetDoor = 0;

	EffectQueue fxqueue;
	
	vvcDict vfxDict;
	vvcSet vfxQueue = vvcSet(VVCSort); // sorted so we can distinguish effects infront and behind
	std::vector<bool> projectileImmunity; // classic bitfield
	Holder<SoundHandle> casting_sound;
	ieDword roundTime = 0;           // these are timers for attack rounds
	ieDword panicMode = PANIC_NONE;  // runaway, berserk or randomwalk
	ieDword nextComment = 0; // do something random (area comment, interaction)
	int FatigueComplaintDelay = 0;   // stagger tired messages
	ieDword lastInit = 0;
	//how many attacks left in this round, must be public for cleave opcode
	int attackcount = 0;

	PolymorphCache* polymorphCache = nullptr; // fx_polymorph etc
	WildSurgeSpellMods wildSurgeMods{};
	ieByte DifficultyMargin = 0;
	ieDword* spellStates = nullptr;
	// delay all maxhp checks until we completely load all effects
	// set after modifying maxhp, adjusts hp next tick
	int checkHP = 2;
	// to determine that a tick has passed
	ieDword checkHPTime = 0;
	/**
	 * We don't know how to profit of them, but PST needs them saved.
	 * Otherwise, some actors are badly drawn, like TNO but not Morte.
	 * bit 0 for a "plasma" effect: palette color entries shift by one index position per cycle update
	 * bit 1 is for enabling pulsating for the particular color range (we store them in IE_COLOR*)
	 *   it periodically reduces brightness to ~50% and back to full
	 */
	ieByte pstColorBytes[10]{};
	
	Region drawingRegion;
private:
	String LongName;
	String ShortName;

	CharAnimations* anims = nullptr;
	
	using AnimationPart = std::pair<Animation*, Holder<Palette>>;
	struct {
		std::vector<AnimationPart> anim;
		std::vector<AnimationPart> shadow;
	} currentStance;

	SaveInfo lastSave;
	ieByte weapSlotCount = 4;
	int walkScale = 0;
	// true when command has been played after select
	bool playedCommandSound = false;
	//true every second round of attack
	bool secondround = false;
	int attacksperround = 0;
	//time of our next attack
	ieDword nextattack = 0;
	ieDword nextWalk = 0;
	ieDword lastattack = 0;
	//trap we're trying to disarm
	ieDword disarmTrap = 0;
	ieDword InTrap = 0;
	char AttackStance = 0;
	/*The projectile bringing the current attack*/
	Projectile* attackProjectile = nullptr;
	tick_t TicksLastRested = 0;
	tick_t LastFatigueCheck = 0;
	tick_t remainingTalkSoundTime = 0;
	tick_t lastTalkTimeCheckAt = 0;
	ieDword lastScriptCheck = 0;
	int lastConBonus;
	/** paint the actor itself. Called internally by Draw() */
	void DrawActorSprite(const Point& p, BlitFlags flags,
						 const std::vector<AnimationPart>& anims, const Color& tint) const;

	/** fixes the palette */
	void SetupColors();
	/** debugging function, gets the scripting name of an actor referenced by a global ID */
	ieVariable GetActorNameByID(ieDword ID) const;
	/* checks a weapon quick slot and resets it to fist if it is empty */
	void CheckWeaponQuickSlot(unsigned int which) const;
	/* helper for usability checks */
	HCStrings CheckUsability(const Item* item) const;
	/* Set up all the missing stats on load time, or after level up */
	void CreateDerivedStatsBG();
	/* Set up all the missing stats on load time, or after level up */
	void CreateDerivedStatsIWD2();
	/* Gets the given ISCLASS level */
	ieDword GetClassLevel(ieDword isClass) const;
	/* Returns true if the dual class is backwards */
	bool IsDualSwap() const;
	/* returns the weapon proficiency stat of the actor */
	int GetProficiency(ieByte proftype) const;
	int GetNonProficiencyPenalty(int stars) const;
	int GetProficiencyBonus(int& style, bool leftOrRight, int& damageBonus, int& speedBonus, int& criticalBonus) const;
	/** Re/Inits the Modified vector for PCs/NPCs */
	void RefreshPCStats();
	void RefreshHP();
	bool ShouldDrawCircle() const;
	bool HasBodyHeat() const;
	void UpdateFatigue();
	int GetSneakAttackDamage(Actor *target, WeaponInfo &wi, int &multiplier, bool weaponImmunity);
	int GetBackstabDamage(const Actor *target, WeaponInfo &wi, int multiplier, int damage) const;
	/** for IE_EXISTANCEDELAY */
	void PlayExistenceSounds();
	TableMgr::index_t GetKitIndex (ieDword kit, ieDword baseclass=0) const;
	char GetArmorCode() const;
	ResRef GetArmorSound() const;

	bool AdvanceAnimations();
	void UpdateDrawingRegion();
	/* applies modal spell etc, if needed */
	void UpdateModalState(ieDword gameTime);

	int CalculateSpeedFromRate(bool feedback) const;
	int CalculateSpeedFromINI(bool feedback) const;
	void IncrementDeathVariable(Game::kaputz_t& vars, const char *format, StringView name) const;

	stats_t ResetStats(bool init);
	void RefreshEffects(bool init, const stats_t& prev);

public:
	Actor(void);
	Actor(const Actor&) = delete;
	~Actor() override;
	Actor& operator=(const Actor&) = delete;
	/** sets game specific parameter (which stat should determine the fist weapon type */
	static void SetFistStat(ieDword stat);
	/** sets game specific default data about action buttons */
	static void SetDefaultActions(int qslot, ieByte slot1, ieByte slot2, ieByte slot3);
	/** prints useful information on console */
	std::string dump() const override;
	/** fixes the feet circle */
	void SetCircleSize();
	/** places the actor on the map */
	void SetMap(Map *map);
	/** sets the actor's position, calculating with the nojump flag*/
	void SetPosition(const Point &nmptTarget, int jump, int radiusx = 0, int radiusy = 0, int size = -1);
	/** you better use SetStat, this stuff is only for special cases*/
	void SetAnimationID(unsigned int AnimID);
	/** returns the animations */
	CharAnimations* GetAnims() const;
	/** returns the gender of actor for cg sound - illusions are tricky */
	ieDword GetCGGender() const;
	/** some hardcoded effects in puppetmaster based on puppet type */
	void CheckPuppet(Actor *puppet, ieDword type);
	/** Re/Inits the Modified vector */
	void RefreshEffects();
	void AddEffects(EffectQueue&& eqfx);
	/** gets saving throws */
	void RollSaves();
	/** returns a saving throw */
	bool GetSavingThrow(ieDword type, int modifier, const Effect *fx = nullptr);
	/** Returns true if the actor is targetable */
	bool ValidTarget(int ga_flags, const Scriptable *checker = NULL) const;
	/** Clamps a stat value to the valid range for the respective stat */
	stat_t ClampStat(unsigned int StatIndex, stat_t Value) const;
	/** Returns a Stat value */
	stat_t GetStat(unsigned int StatIndex) const;
	/** Returns a safe Stat value, one, that is not partially computed */
	stat_t GetSafeStat(unsigned int StatIndex) const;
	/** Sets a Stat Value (unsaved) */
	bool SetStat(unsigned int StatIndex, stat_t Value, int pcf);
	/** Returns the difference */
	int GetMod(unsigned int StatIndex) const;
	/** Returns a Stat Base Value */
	ieDword GetBase(unsigned int StatIndex) const;
	/** Sets a Base Stat Value */
	bool SetBase(unsigned int StatIndex, stat_t Value);
	bool SetBaseNoPCF(unsigned int StatIndex, stat_t Value);
	/** set/resets a Base Stat bit */
	bool SetBaseBit(unsigned int StatIndex, stat_t Value, bool setreset);
	/** Sets the modified value in different ways, returns difference */
	int NewStat(unsigned int StatIndex, stat_t ModifierValue, ieDword ModifierType);
	/** Modifies the base stat value in different ways, returns difference */
	int NewBase(unsigned int StatIndex, stat_t ModifierValue, ieDword ModifierType);
	void SetLeader(const Actor* actor, int offset = 0);
	/** Sets the Icon ResRef */
	//Which - 0 both, 1 Large, 2 Small
	void SetPortrait(const ResRef&, int Which=0);
	void SetSoundFolder(const String& soundset) const;
	/* Use overrideSet to replace PCStats->SoundSet */
	String GetSoundFolder(int flag, const ResRef& overrideSet) const;
	
	/** Gets the Character Long Name/Short Name */
	const String& GetShortName() const
	{
		return ShortName;
	}
	
	const String& GetLongName() const
	{
		return LongName;
	}
	
	const String& GetDefaultName() const
	{
		return TalkCount ? GetLongName() : GetShortName();
	}
	
	ResRef GetScript(int ScriptIndex) const;
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
	ResRef GetDialog(int flags=GD_NORMAL) const;
	void SetDialog(const ResRef &resref);
	/** Gets the Portrait */
	Holder<Sprite2D> CopyPortrait(int which) const;

	/** Gets the attack projectile */
	Projectile* GetAttackProjectile()
	{
		return attackProjectile;
	}
	void SetName(String str, unsigned char type);
	void SetName(ieStrRef strref, unsigned char type);
	/* Returns by how much movement speed should be divided to account for loot weight */
	int GetEncumbranceFactor(bool feedback) const;
	/* calculates speed, encumbrance etc */
	int CalculateSpeed(bool feedback) const;
	int GetSpeed() const { return walkScale; }
	void SetSpeed(bool feedback) { walkScale = CalculateSpeed(feedback); }
	/* checks on death of actor, returns true if it should be removed*/
	bool CheckOnDeath();
	/* receives undead turning message */
	void Turn(Scriptable *cleric, ieDword turnlevel);
	/* call this on gui selects */
	void PlaySelectionSound(bool force = false);
	/* play a roar if the setting isn't disabled */
	void PlayWarCry(int range) const;
	/* call this when adding actions via gui */
	void CommandActor(Action* action, bool clearPath=true);
	/** handle panic and other involuntary actions that mess with scripting */
	bool OverrideActions();
	/** handle idle actions, that shouldn't mess with scripting */
	void IdleActions(bool nonidle);
	/* sets the actor in panic (turn/morale break) */
	void Panic(const Scriptable *attacker, int panicmode);
	/* sets a multi class flag (actually this is a lot of else too) */
	void SetMCFlag(ieDword bitmask, BitOp op);
	/* inlined dialogue start */
	void Interact(int type) const;
	/* returns a remapped verbal constant strref */
	ieStrRef GetVerbalConstant(Verbal index) const;
	/* returns a random remapped verbal constant strref */
	ieStrRef GetVerbalConstant(Verbal start, int count) const;
	/* displaying a random verbal constant */
	bool VerbalConstant(Verbal start, int count = 1, int flags = 0) const;
	/* display string or verbal constant depending on what is available */
	void DisplayStringOrVerbalConstant(HCStrings str, Verbal vcStat, int vcCount = 1) const;
	/* inlined dialogue response */
	void Response(int type) const;
	tick_t ReactToDeath(const ieVariable& deadname) const;
	/* sends trigger_died to everyone in visual range */
	void SendDiedTrigger() const;
	/* called when someone talks to Actor */
	void DialogInterrupt() const;
	/* called when actor was hit */
	void GetHit(int damage = 0, bool killingBlow = false);
	/* checks whether taking damage should disrupt spellcasting */
	bool CheckSpellDisruption(int damage) const;
	/* called when actor starts to cast a spell*/
	bool HandleCastingStance(const ResRef& spellResRef, bool deplete, bool instant);
	/* check if the actor is silenced - for casting purposes */
	bool CheckSilenced() const;
	/* check and perform a cleave movement */
	void CheckCleave();
	/* deals damage to this actor */
	int Damage(int damage, int damagetype, Scriptable* hitter, int modtype = MOD_ADDITIVE, int critical = 0, int saveflags = 0, int specialFlags = 0);
	/* displays the damage taken and other details (depends on the game type) */
	void DisplayCombatFeedback(unsigned int damage, int resisted, int damagetype, const Scriptable *hitter);
	/* play a random footstep sound */
	void PlayWalkSound();
	/* play the proper hit sound (in pst) */
	void PlayHitSound(const DataFileMgr *resdata, int damagetype, bool suffix) const;
	void PlaySwingSound(const WeaponInfo &wi) const;
	/* drops items from inventory to current spot */
	void DropItem(const ResRef& resref, unsigned int flags);
	void DropItem(int slot, unsigned int flags);
	/* returns item information in quickitem slot */
	void GetItemSlotInfo(ItemExtHeader *item, int which, int header) const;
	/* returns spell information in quickspell slot */
	void GetSpellSlotInfo(SpellExtHeader *spell, int which);
	/* updates quickslots */
	void ReinitQuickSlots() const;
	/* actor is in trap */
	void SetInTrap(ieDword tmp);
	/* sets some of the internal flags */
	void SetRunFlags(ieDword flags);
	bool IsRunning() const { return InternalFlags & IF_RUNFLAGS; }
	/* applies the kit abilities, returns false if kit is not applicable */
	bool ApplyKit(bool remove, ieDword baseclass=0, int diff=0);
	/* applies the class abilities*/
	void ApplyClab(const ResRef& clab, ieDword max, int remove, int diff);
	/* calls InitQuickSlot in PCStatStruct */
	void SetupQuickSlot(unsigned int which, ieWord slot, ieWord headerIndex) const;
	/* returns true if the actor is PC/joinable*/
	bool Persistent() const;
	/* assigns actor to party slot, 0 = NPC, areas won't remove it */
	void SetPersistent(int partyslot);
	/* resurrects actor */
	void Resurrect(const Point &destPoint);
	/* removes actor in the next update cycle */
	void DestroySelf();
	/* schedules actor to die */
	void Die(Scriptable *killer, bool grantXP = true);
	/* debug function */
	void GetNextAnimation();
	/* debug function */
	void GetPrevAnimation();
	/* debug function */
	void GetNextStance();
	void ClearCurrentStanceAnims();
	/* learns the given spell, possibly receive XP */
	int LearnSpell(const ResRef& resref, ieDword flags, int bookmask=-1, int level=-1);
	/* Returns current weapon range and extended header
	if range is nonzero, then which is valid */
	const ITMExtHeader* GetWeapon(bool leftOrRight) const;
	/* Creates player statistics */
	void CreateStats();
	/* Heals actor */
	void Heal(int hp);
	/* Receive experience (handle dual/multi class) */
	void AddExperience(int exp, int combat);
	/* Sets the modal state after checks */
	void SetModal(enum Modal newstate, bool force = true);
	/* Sets the modal spell after checks */
	void SetModalSpell(enum Modal state, const ResRef& spell);
	/* casts the modal spell if any */
	void ApplyModal(const ResRef& modalSpell);
	/* adds the combatants to the attackers list */
	void AttackedBy(const Actor *actor);
	/* reorients to face target (for immediate attack) */
	void FaceTarget(const Scriptable *actor);
	/* returns the number of attacks (handles monk barehanded bonus) */
	ieDword GetNumberOfAttacks();
	/* starts combat round*/
	void InitRound(ieDword gameTime);
	/* returns melee penalty */
	int MeleePenalty() const;
	/* gets the to hit value */
	int GetToHit(ieDword Flags, const Actor *target);
	void GetTHAbilityBonus(ieDword Flags);
	/* gets the defense against an attack */
	int GetDefense(int DamageType, ieDword wflags, const Actor *attacker) const;
	/* returns the number of allocated proficiency points (stars) */
	int GetStars(stat_t proficiency) const;
	static bool IsCriticalEffectEligible(const WeaponInfo& wi, const Effect* fx);
	/* get the current hit bonus */
	bool GetCombatDetails(int& tohit, bool leftorright, \
		int& DamageBonus, int& speed, int& CriticalBonus, int& style, const Actor* target);
	/* performs attack against target */
	void PerformAttack(ieDword gameTime);
	/* returns the adjusted weapon range, since items have odd values stored */
	unsigned int GetWeaponRange(bool leftOrRight) const;
	/* filter out any damage reduction that is cancelled by high weapon enchantment and return the resulting resistance */
	int GetDamageReduction(int resist_stat, ieDword weaponEnchantment) const;
	/* calculates strength (dexterity) based damage adjustment */
	int WeaponDamageBonus(const WeaponInfo &wi) const;
	/* handles critical, backstab, etc. damage modifications */
	void ModifyWeaponDamage(WeaponInfo &wi, Actor *target, int &damage, bool &critical);
	/* adjusts damage dealt to this actor, handles mirror images  */
	void ModifyDamage(Scriptable *hitter, int &damage, int &resisted, int damagetype);
	/* returns the hp adjustment based on constitution */
	int GetHpAdjustment(int multiplier, bool modified=true) const;
	/* does all the housekeeping after loading the actor from file */
	void InitStatsOnLoad();
	/* sets a colour gradient stat, handles location */
	void SetColor( ieDword idx, ieDword grd);
	/* sets an RGB colour modification effect; location 0xff for global */
	void SetColorMod( ieDword location, RGBModifier::Type type, int speed,
					 const Color&, int phase = -1) const;
	bool Schedule(ieDword gametime, bool checkhide) const;
	void NewPath();
	/* overridden method, won't walk if dead */
	void WalkTo(const Point &Des, ieDword flags, int MinDistance = 0);
	/* resolve string constant (sound will be altered) */
	void GetVerbalConstantSound(ResRef& sound, Verbal index, bool resolved = false) const;
	bool GetSoundFromFile(ResRef& sound, Verbal index) const;
	bool GetSoundFromINI(ResRef& sound, Verbal index) const;
	bool GetSoundFrom2DA(ResRef& sound, Verbal index) const;
	/* start bg1-style banter dialog */
	void HandleInteractV1(const Actor *target);
	/* generate party banter, return true if successful */
	bool GetPartyComment(const Actor* target);
	/* sets the quick slots */
	void SetActionButtonRow(const ActionButtonRow &ar) const;
	/* updates the quick slots */
	void GetActionButtonRow(ActionButtonRow &qs);
	/* converts the iwd2 qslot index to our internal representation */
	ieByte IWD2GemrbQslot(int slotindex) const;
	int Gemrb2IWD2Qslot(ieByte actslot, int slotindex) const;
	void dumpQSlots() const;

	/* Handling automatic stance changes */
	bool HandleActorStance();
	void UpdateActorState();
	/* update internal per frame state and return true if state is suitable for drawing the actor */
	bool UpdateDrawingState();
	Region DrawingRegion() const override;
	int GetElevation() const;
	bool ShouldDrawReticle() const;
	void DoStep(unsigned int newWalkScale, ieDword time = 0) override;
	void Draw(const Region &screen, Color baseTint, Color tint, BlitFlags flags) const;

	/* add mobile vvc (spell effects) to actor's list */
	void AddVVCell(ScriptedAnimation* vvc);
	/* remove a vvc from the list, graceful means animated removal */
	void RemoveVVCells(const ResRef& vvcname);
	/* returns true if actor already has the overlay (slow) */
	bool HasVVCCell(const ResRef& resource) const;
	/* returns overlay if actor already has it (slow) */
	std::pair<vvcDict::const_iterator, vvcDict::const_iterator>
	GetVVCCells(const ResRef& resource) const;
	/* returns the vvc pointer to a hardcoded overlay */
	/* if it exists (faster than hasvvccell) */
	ScriptedAnimation *FindOverlay(int index) const;

	void SetLockedPalette(const ieDword *gradients);
	void UnlockPalette();
	void AddAnimation(const ResRef& resource, int gradient, int height, int flags);
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
	ieWord GetCriticalType() const;
	/* restores a spell of maximum maxlevel level, type is a mask of disabled spells */
	int RestoreSpellLevel(ieDword maxlevel, ieDword typemask);
	/* rememorizes spells, cures fatigue, etc */
	void Rest(int hours);
	int GetConHealAmount() const;
	/* adds a state icon to the list */
	void AddPortraitIcon(ieByte icon) const;
	/* disables a state icon in the list, doesn't remove it! */
	void DisablePortraitIcon(ieByte icon) const;
	/* returns which slot belongs to the quickweapon slot */
	int GetQuickSlot(int slot) const;
	/* Sets equipped Quick slot, if header is -1, then use the current one */
	HCStrings SetEquippedQuickSlot(int slot, int header);
	/* Uses an item on the target or point */
	bool TryUsingMagicDevice(const Item* item, ieDword header);
	bool RequiresUMD(const Item* item) const;
	bool UseItemPoint(ieDword slot, ieDword header, const Point &point, ieDword flags);
	bool UseItem(ieDword slot, ieDword header, const Scriptable *target, ieDword flags, int damage = 0);
	/* Deducts a charge from an item */
	void ChargeItem(ieDword slot, ieDword header, CREItem *item, const Item *itm, bool silent, bool expend = true);
	/* If it returns true, then default AC=10 and the lesser the better */
	static int IsReverseToHit();
	/* initialize the action buttons based on class. If forced, it will override
		previously customized or set buttons. */
	void InitButtons(ieDword cls, bool forced) const;
	int GetMaxEncumbrance() const;
	int GetAbilityBonus(unsigned int ability, int value = -1) const;
	int GetSkillStat(unsigned int skill) const;
	int GetSkill(unsigned int skill, bool ids=false) const;
	int GetFeat(unsigned int feat) const;
	void SetFeat(unsigned int feat, BitOp mode);
	void SetFeatValue(unsigned int feat, int value, bool init = true);
	void SetUsedWeapon(AnimRef, const ieWord *MeleeAnimation,
		unsigned char WeaponType = IE_ANI_WEAPON_INVALID);
	void SetUsedShield(AnimRef, unsigned char WeaponType = IE_ANI_WEAPON_INVALID);
	void SetUsedHelmet(AnimRef);
	void SetupFist();
	/* Returns nonzero if the caster is held */
	int Immobile() const;
	/* Returns strref if the item is unusable due to name/type restrictions */
	ieStrRef Disabled(const ResRef& name, ieDword type) const;
	/* Returns constant string if the item is unusable */
	HCStrings Unusable(const Item* item) const;
	/* Sets all clown colour to the given gradient */
	void SetGradient(ieDword gradient);
	/* Enables an overlay */
	void SetOverlay(unsigned int overlay);
	/* Checks and sets a spellstate if it wasn't set yet */
	bool SetSpellState(unsigned int spellstate) const;
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
	/* Resets the internal multiclass bitfield */
	void ResetMC();
	/* Checks if the actor is multiclassed (excluding dualclassed actors)) */
	bool IsMultiClassed() const;
	/* Checks if the actor is dualclassed */
	bool IsDualClassed() const;
	/* Returns an exact copy of this actor */
	Actor *CopySelf(bool mislead) const;
	static ieDword GetClassID(ieDword isClass);
	const std::string& GetClassName(ieDword classID) const;
	const std::string& GetKitName(ieDword kitID) const;
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
	int GetFavoredPenalties() const;
	bool BlocksSearchMap() const override;
	bool CannotPassEntrance(ieDword exitID) const;
	void UseExit(ieDword exitID);
	//int GetReaction() const;
	/* Similar to Roll, but takes luck into account */
	int LuckyRoll(int dice, int size, int add, ieDword flags = LR_CRITICAL, const Actor* opponent = nullptr) const;
	/* removes normal invisibility (type 0) */
	void CureInvisibility();
	/* removes sanctuary */
	void CureSanctuary();
	/* resets the invisibility, sanctuary and modal states */
	void ResetState();
	/* checks whether the actor is behind the target */
	bool IsBehind(const Actor* target) const;
	/* checks whether the target is the actor's racial enemy */
	int GetRacialEnemyBonus(const Actor *target) const;
	/* checks whether the actor can stay in the current modal state */
	bool ModalSpellSkillCheck();
	/* check if this actor is seen by or seeing anyone */
	bool SeeAnyOne(bool enemy, bool seen) const;
	/* does all the game logic checks to see if the actor can hide */
	bool TryToHide();
	bool TryToHideIWD2();
	/* checks if the alignment matches one of the masking constants */
	//bool MatchesAlignmentMask(ieDword mask);
	/** untargetable by spells/attack due to invisibility or sanctuary */
	bool Untargetable(const ResRef& spellRef) const;
	/* returns true if this it is futile to try to harm actor (dead/sanctuaried) */
	bool InvalidSpellTarget() const;
	/* returns true if the spell is useless to cast on target
	or the spell's range is smaller than range */
	bool InvalidSpellTarget(int spellnum, Actor *caster, int range) const;
	/* function to get a class level used by scripting */
	ieDword GetLevelInClass(ieDword classid) const;
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
	int GetArmorWeightClass(ieWord armorType) const;
	int GetTotalArmorFailure() const;
	int GetArmorFailure(int &armor, int &shield) const;
	bool IsDead() const;
	bool IsInvisibleTo(const Scriptable *checker) const;
	int UpdateAnimationID(bool derived);
	void MovementCommand(std::string command);
	/* shows hp/maxhp as overhead text */
	bool HasVisibleHP() const;
	void DisplayHeadHPRatio();
	/* if Lasttarget is gone, call this */
	void StopAttack();
	int SetBaseAPRandAB(bool CheckRapidShot);
	int BAB2APR(int pBAB, int pBABDecrement, int CheckRapidShot) const;
	/* set to trap id if current action is disarm; unset after */
	void SetDisarmingTrap(ieDword trapId) { disarmTrap = trapId; }
	ieDword GetDisarmingTrap() const { return disarmTrap; }
	void ReleaseCurrentAction() override;
	bool ConcentrationCheck() const;
	void ApplyEffectCopy(const Effect *oldfx, EffectRef &newref, Scriptable *Owner, ieDword param1, ieDword param2);
	tick_t GetLastRested() const { return TicksLastRested; }
	void IncreaseLastRested(int inc) { TicksLastRested += inc; LastFatigueCheck += inc; }
	bool WasClass(ieDword oldClassID) const;
	ieDword GetActiveClass() const;
	bool IsKitInactive() const;
	const std::string& GetRaceName() const;
	unsigned int GetSubRace() const;
	std::list<int> ListLevels() const;
	void ChangeSorcererType (ieDword classIdx);
	tick_t GetAdjustedTime(tick_t time) const;
	void SetAnimatedTalking(tick_t);
	bool HasPlayerClass() const;
	void PlayArmorSound() const;
	bool ShouldModifyMorale() const;
	bool HibernateIfAble();
	bool ForceScriptCheck();
};
}

#endif
