// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef GAMESCRIPT_H
#define GAMESCRIPT_H

#include "exports.h"

#include "Debug.h"

#include "GameScript/Targets.h"
#include "Logging/Logging.h"
#include "Scriptable/Actor.h"
#include "Streams/DataStream.h"

#include <cstdio>
#include <vector>

namespace GemRB {

//escapearea flags
enum class EscapeArea {
	None,
	Destroy, // destroy actor at the exit (otherwise move to new place)
	DestroyNoSee // no need to see the exit either
};

//displaystring flags
#define DS_WAIT     1
#define DS_HEAD     2
#define DS_CONSOLE  4
#define DS_CONST    8
#define DS_NONAME   16
#define DS_SILENT   32
#define DS_SPEECH   64
#define DS_AREA     128
#define DS_QUEUE    256
#define DS_CIRCLE   512
#define DS_RESOLVED 1024
#define DS_APPEND   2048

//markspellandobject (iwd2)
#define MSO_IGNORE_SEE     1
#define MSO_IGNORE_INVALID 2
#define MSO_RANDOM_SPELL   4
#define MSO_IGNORE_HAVE    8
#define MSO_IGNORE_RANGE   16
#define MSO_IGNORE_NULL    32

// delta.ids (pst)
#define DM_LOWER 1
#define DM_RAISE 2
#define DM_SET   3

//attack core flags
#define AC_NO_SOUND 1
#define AC_RUNNING  2

//spelll core flags
#define SC_NO_DEAD     1
#define SC_RANGE_CHECK 2
#define SC_DEPLETE     4
#define SC_SETLEVEL    8
#define SC_INSTANT     16
#define SC_AURA_CHECK  32
#define SC_NOINTERRUPT 64

#define ACF_OVERRIDE        1 // was this action invoked via ActionOverride?
#define ACF_REALLOW_SCRIPTS 0x1000 // gemrb internal
#define ACF_MISSING_OBJECT  0x2000 // used for detection of [ANYONE], since it has the same signature as no object, but is not the same
#define ACF_PRECOMPILED     0x4000 // whether it came from a bcs
#define ACF_FOLLOW_DONE     0x10000000 // Bubb: written during CGameSprite::Follow(), I believe it means the MoveToPoint() ended with ACTION_DONE
// NOTE: if it ever becomes useful, this is where it came into play
// CGameSprite::Follow() is used in the Follow action, as well as when Leader() is used in
// combination with FollowObjectFormation(), Formation(), MoveToObject(), and MoveToPoint().
// Leader() is also used internally when processing the "Follow the Leader" group formation.

//trigger flags stored in triggers in .bcs files
#define TF_NEGATE         1 //negate trigger result
#define TF_APPLIED        2 //set in living when trigger applied
#define TF_ADDED          4 //set in scriptable when trigger added/applied
#define TF_MISSING_OBJECT 8 // used for detection of [ANYONE], since it has the same signature as no object, but is not the same
#define TF_PRECOMPILED    16 // whether it came from a bcs

#define MAX_OBJECT_FIELDS 10
#define MAX_NESTING       5

using StringParam = FixedSizeString<64, strnicmp>;
static_assert(std::is_standard_layout<StringParam>::value, "Fixed Size String must be standard layout for use in unions");

class Canary {
private:
	volatile unsigned long canary;

protected:
	Canary() noexcept // protected constructor
	{
		canary = (unsigned long) 0xdeadbeef;
	}
	Canary(const Canary&) = delete;
	Canary(Canary&&) noexcept = default;
	Canary& operator=(const Canary&) = delete;
	virtual ~Canary() noexcept // protected destructor
	{
		AssertCanary("Destroying Canary");
		canary = 0xdddddddd;
	}
	void AssertCanary(const char* msg) const
	{
		if (!CheckCanary()) {
			error("Canary Died", "Canary([{:#10x}]) != 0xdeadbeef. Message: {}", canary, msg);
		}
	}
	bool CheckCanary() const
	{
		return (canary == (unsigned long) 0xdeadbeef);
	}
};

class GEM_EXPORT Object : protected Canary {
public:
	int objectFields[MAX_OBJECT_FIELDS] {}; // eg. [PC.0.0.UNDEAD]
	int objectFilters[MAX_NESTING] {}; // eg. Myself or LastTargetedBy(LastAttackerOf(Player1))
	Region objectRect {};

	union {
		StringParam objectName;
		ieVariable objectNameVar;
		ResRef objectNameRef;
	};

public:
	Object() noexcept
		: objectName() {};

	std::string dump(bool print = true) const;
	void Release()
	{
		delete this;
	}
	bool isNull() const;
};

class GEM_EXPORT Trigger final : protected Canary {
public:
	Trigger() noexcept
		: string0Parameter(), string1Parameter() {};
	~Trigger() final
	{
		if (objectParameter) {
			objectParameter->Release();
			objectParameter = nullptr;
		}
	}
	int Evaluate(Scriptable* Sender) const;

	unsigned short triggerID = 0;
	int int0Parameter = 0;
	int flags = 0;
	int int1Parameter = 0;
	int int2Parameter = 0;
	Point pointParameter;
	Object* objectParameter = nullptr;

	union {
		StringParam string0Parameter;
		ieVariable variable0Parameter;
		ResRef resref0Parameter;
	};

	union {
		StringParam string1Parameter;
		ieVariable variable1Parameter;
		ResRef resref1Parameter;
	};

	std::string dump() const;

	void Release()
	{
		delete this;
	}
};

class GEM_EXPORT Condition final : protected Canary {
public:
	Condition() noexcept = default;
	Condition(Condition&&) noexcept = default;
	~Condition() noexcept override
	{
		for (auto& trigger : triggers) {
			if (trigger) {
				trigger->Release();
				trigger = nullptr;
			}
		}
	}
	void Release()
	{
		delete this;
	}
	bool Evaluate(Scriptable* Sender) const;

	std::vector<Trigger*> triggers;
};

class GEM_EXPORT Action final : public std::enable_shared_from_this<Action>, protected Canary {
	Action() noexcept
		: string0Parameter(), string1Parameter()
	{}

public:
	~Action() noexcept override
	{
		for (auto& object : objects) {
			if (object) {
				object->Release();
				object = nullptr;
			}
		}
	}

	static Holder<Action> MakeAction() noexcept;

	std::string dump() const;

	unsigned short actionID = 0;
	Object* objects[3] {};
	int int0Parameter = 0;
	Point pointParameter;
	int int1Parameter = 0;
	int int2Parameter = 0;

	union {
		StringParam string0Parameter; // keep largest type first to 0 fill everything
		ieVariable variable0Parameter;
		ResRef resref0Parameter;
	};

	union {
		StringParam string1Parameter; // keep largest type first to 0 fill everything
		ieVariable variable1Parameter;
		ResRef resref1Parameter;
	};

	uint32_t flags = 0;

};

class GEM_EXPORT Response final : protected Canary {
public:
	Response() noexcept = default;
	~Response() noexcept override
	{
		for (const auto& action : actions) {
			if (action) {
				if (action.use_count() > 2) {
					Log(DEBUG, "GameScript", "Residue action {} with refcount {}", action->actionID, action.use_count());
				}
			}
		}
	}
	void Release()
	{
		delete this;
	}
	int Execute(Scriptable* Sender);

	unsigned char weight = 0;
	std::vector<Holder<Action>> actions;
};

class GEM_EXPORT ResponseSet final : protected Canary {
public:
	~ResponseSet() final
	{
		for (auto& response : responses) {
			response->Release();
			response = nullptr;
		}
	}
	void Release()
	{
		delete this;
	}
	int Execute(Scriptable* Sender);

	std::vector<Response*> responses;
};

class GEM_EXPORT ResponseBlock final : protected Canary {
public:
	ResponseBlock() noexcept = default;
	~ResponseBlock() noexcept override
	{
		condition = nullptr;
		if (responseSet) {
			responseSet->Release();
			responseSet = nullptr;
		}
	}
	void Release()
	{
		delete this;
	}

	Holder<Condition> condition = nullptr;
	ResponseSet* responseSet = nullptr;
};

class GEM_EXPORT Script final : protected Canary {
public:
	~Script() noexcept override
	{
		for (auto& responseBlock : responseBlocks) {
			if (responseBlock) {
				responseBlock->Release();
				responseBlock = nullptr;
			}
		}
	}

	std::vector<ResponseBlock*> responseBlocks;

	void Release()
	{
		delete this;
	}
};

using TriggerFunction = int (*)(Scriptable*, const Trigger*);
using ActionFunction = void (*)(Scriptable*, Holder<Action>);
using ObjectFunction = Targets* (*) (const Scriptable*, Targets*, int ga_flags);
using IDSFunction = int (*)(const Actor*, int parameter);

#define TF_NONE         0
#define TF_CONDITION    1 //this isn't a trigger, just a condition (0x4000)
#define TF_SAVED        2 //trigger is in svtriobj.ids
#define TF_MERGESTRINGS 8 //same value as actions' mergestring
#define TF_HAS_OBJECT   16 // whether it has an object parameter

struct TriggerLink {
	const char* Name;
	TriggerFunction Function;
	short Flags;
};

//createcreature flags
#define CC_OFFSET           1
#define CC_OBJECT           2
#define CC_OFFSCREEN        3
#define CC_MASK             3
#define CC_CHECK_IMPASSABLE 4 //adjust position (searchmap)
#define CC_PLAY_ANIM        8 //play animation
#define CC_STRING1          16 //resref is in second string
#define CC_CHECK_OVERLAP    32 //other actors
#define CC_COPY             64 //copy appearance
#define CC_SCRIPTNAME       128 //scriptname in 2nd string

//begindialog flags
#define BD_STRING0   0
#define BD_TARGET    1
#define BD_SOURCE    2
#define BD_RESERVED  3 //playerX resref
#define BD_INTERACT  4 //banter dialogs
#define BD_LOCMASK   7 //where is the dialog resref
#define BD_TALKCOUNT 8 //increases talkcount
#define BD_SETDIALOG 16 //also sets dialog (for string0)
#define BD_CHECKDIST 32 //checks distance, if needs, walks up
#define BD_OWN       64 //source == target, works for player only
#define BD_INTERRUPT 128 //interrupts action
#define BD_NUMERIC   256 //target is numeric
#define BD_ITEM      512 //talk to an item
#define BD_NOEMPTY   1024 //don't display '... has nothing to say to you'

#define AF_NONE         0
#define AF_IMMEDIATE    1
#define AF_CONTINUE     2
#define AF_MASK         3 //none, immediate or continue
#define AF_BLOCKING     4
#define AF_MERGESTRINGS 8
//we could use this flag to restrict player scripts from using dangerous
//opcodes, it would be a very useful and easy to implement feature!
#define AF_RESTRICTED 16
#define AF_DIALOG        32 // whether it starts a dialog
#define AF_SCRIPTLEVEL   64 //this hack will transfer scriptlevel to int0parameter at runtime (changecurrentscript relies on it)
#define AF_INVALID       128
#define AF_DIRECT        256 //this hack will transfer target from gamecontrol to object1 at compile time
#define AF_ALIVE         512 //only alive actors can do this
#define AF_CHASE         1024 // ??? actions involving movement ???
#define AF_SLEEP         2048 //only awake actors can do this
#define AF_DLG_INSTANT   4096 //instant dialog actions
#define AF_SCR_INSTANT   8192 //instant script actions
#define AF_FILE_MASK     (AF_SCR_INSTANT - 1) // limit of flags defined in original ids files
#define AF_INSTANT       (AF_DLG_INSTANT | AF_SCR_INSTANT) //only iwd2 treats them separately; 12288
#define AF_IWD2_OVERRIDE 16384 // marking actions that require special attention when clearing during ActionOverride
#define AF_HAS_OBJECT    32768 // whether it has an object parameter

enum RunAwayFlags {
	LeaveArea = 1,
	NoInterrupt = 2,
	UsePoint = 4
};

struct ActionLink {
	const char* Name;
	ActionFunction Function;
	uint16_t Flags;
};

struct ObjectLink {
	const char* Name;
	ObjectFunction Function;
};

struct IDSLink {
	const char* Name;
	IDSFunction Function;
};

#define MAX_TRIGGERS    300
#define MAX_ACTIONS     500
#define MAX_OBJECTS     256
#define AI_SCRIPT_LEVEL 4 //the script level of special ai scripts

template<typename... ARGS>
extern void ScriptDebugLog(DebugMode bit, const char* message, ARGS&&... args)
{
	if (!InDebugMode(bit)) return;

	Log(DEBUG, "GameScript", message, std::forward<ARGS>(args)...);
}
extern int RandomNumValue;
extern int NextTriggerObjectID;
extern unsigned int MAX_OPERATING_DISTANCE;

class GEM_EXPORT GameScript {
public:
	GameScript(const ResRef& ResRef, Scriptable* Myself,
		   int ScriptLevel = 0, bool AIScript = false);
	GameScript(const GameScript&) = delete;
	~GameScript();
	GameScript& operator=(const GameScript&) = delete;

	bool dead = false; // Script replaced itself with another and should be deleted when done running
	bool running = false; // Script is currently running so defer any deletion to caller

	ResRef GetName() const { return Name; }
	static void ExecuteString(Scriptable* Sender, std::string string);
	static int EvaluateString(Scriptable* Sender, const char* String);
	static void ExecuteAction(Scriptable* Sender, Holder<Action> aC);

	bool Update(bool* continuing = NULL, bool* done = NULL);
	void EvaluateAllBlocks(bool testConditions = false);

private: //Internal Functions
	Script* CacheScript(const ResRef& ResRef, bool AIScript);
	ResponseBlock* ReadResponseBlock(DataStream* stream);
	ResponseSet* ReadResponseSet(DataStream* stream);
	Response* ReadResponse(DataStream* stream);
	static int InParty(Scriptable* Sender, const Trigger* parameters, bool allowdead);

	// Internal variables
	Scriptable* const MySelf;
	ResRef Name;
	Script* script;
	size_t lastResponseBlock = -1;
	int scriptlevel;

public: //Script Functions
	static int ID_Alignment(const Actor* actor, int parameter);
	static int ID_Allegiance(const Actor* actor, int parameter);
	static int ID_AVClass(const Actor* actor, int parameter);
	static int ID_Class(const Actor* actor, int parameter);
	static int ID_ClassMask(const Actor* actor, int parameter);
	static int ID_Faction(const Actor* actor, int parameter);
	static int ID_Gender(const Actor* actor, int parameter);
	static int ID_General(const Actor* actor, int parameter);
	static int ID_Race(const Actor* actor, int parameter);
	static int ID_Specific(const Actor* actor, int parameter);
	static int ID_Subrace(const Actor* actor, int parameter);
	static int ID_Team(const Actor* actor, int parameter);

	//Triggers
	static int ActionListEmpty(Scriptable* Sender, const Trigger* parameters);
	static int ActuallyInCombat(Scriptable* Sender, const Trigger* parameters);
	static int Acquired(Scriptable* Sender, const Trigger* parameters);
	static int Alignment(Scriptable* Sender, const Trigger* parameters);
	static int Allegiance(Scriptable* Sender, const Trigger* parameters);
	static int AnimationID(Scriptable* Sender, const Trigger* parameters);
	static int AnimState(Scriptable* Sender, const Trigger* parameters);
	static int AnyPCOnMap(Scriptable* Sender, const Trigger* parameters);
	static int AnyPCSeesEnemy(Scriptable* Sender, const Trigger* parameters);
	static int AreaCheck(Scriptable* Sender, const Trigger* parameter);
	static int AreaCheckAllegiance(Scriptable* Sender, const Trigger* parameters);
	static int AreaCheckObject(Scriptable* Sender, const Trigger* parameter);
	static int AreaFlag(Scriptable* Sender, const Trigger* parameter);
	static int AreaRestDisabled(Scriptable* Sender, const Trigger* parameter);
	static int AreaStartsWith(Scriptable* Sender, const Trigger* parameter); //InWatchersKeep
	static int AreaType(Scriptable* Sender, const Trigger* parameter);
	static int Assign(Scriptable* /*Sender*/, const Trigger* /*parameters*/);
	static int AtLocation(Scriptable* Sender, const Trigger* parameter);
	static int AttackedBy(Scriptable* Sender, const Trigger* parameters);
	static int BecameVisible(Scriptable* Sender, const Trigger* parameters);
	static int BeenInParty(Scriptable* Sender, const Trigger* /*parameters*/);
	static int BitCheck(Scriptable* Sender, const Trigger* parameters);
	static int BitCheckExact(Scriptable* Sender, const Trigger* parameters);
	static int BitGlobal_Trigger(Scriptable* Sender, const Trigger* parameters);
	static int BouncingSpellLevel(Scriptable* Sender, const Trigger* parameters);
	static int BreakingPoint(Scriptable* Sender, const Trigger* parameters);
	static int ButtonDisabled(Scriptable* Sender, const Trigger* parameters);
	static int CalendarDay(Scriptable* Sender, const Trigger* parameters);
	static int CalendarDayGT(Scriptable* Sender, const Trigger* parameters);
	static int CalendarDayLT(Scriptable* Sender, const Trigger* parameters);
	static int CalledByName(Scriptable* Sender, const Trigger* parameters);
	static int CanEquipRanged(Scriptable* Sender, const Trigger* /*parameters*/);
	static int CanTurn(Scriptable* Sender, const Trigger* parameters);
	static int ChargeCount(Scriptable* Sender, const Trigger* parameters);
	static int CharName(Scriptable* Sender, const Trigger* parameters);
	static int CheckAreaDiffLevel(Scriptable* /*Sender*/, const Trigger* parameters);
	static int CheckDoorFlags(Scriptable* Sender, const Trigger* parameters);
	static int CheckPartyAverageLevel(Scriptable* Sender, const Trigger* parameters);
	static int CheckPartyLevel(Scriptable* Sender, const Trigger* parameters);
	static int CheckSkill(Scriptable* Sender, const Trigger* parameters);
	static int CheckSkillGT(Scriptable* Sender, const Trigger* parameters);
	static int CheckSkillLT(Scriptable* Sender, const Trigger* parameters);
	static int CheckSpellState(Scriptable* Sender, const Trigger* parameters);
	static int CheckStat(Scriptable* Sender, const Trigger* parameters);
	static int CheckStatGT(Scriptable* Sender, const Trigger* parameters);
	static int CheckStatLT(Scriptable* Sender, const Trigger* parameters);
	static int Class(Scriptable* Sender, const Trigger* parameters);
	static int ClassEx(Scriptable* Sender, const Trigger* parameters);
	static int ClassLevel(Scriptable* Sender, const Trigger* parameters);
	static int ClassLevelGT(Scriptable* Sender, const Trigger* parameters);
	static int ClassLevelLT(Scriptable* Sender, const Trigger* parameters);
	static int Clicked(Scriptable* Sender, const Trigger* parameters);
	static int Closed(Scriptable* Sender, const Trigger* parameters);
	static int CombatCounter(Scriptable* Sender, const Trigger* parameters);
	static int CombatCounterGT(Scriptable* Sender, const Trigger* parameters);
	static int CombatCounterLT(Scriptable* Sender, const Trigger* parameters);
	static int Contains(Scriptable* Sender, const Trigger* parameters);
	static int CreatureHidden(Scriptable* Sender, const Trigger* parameters);
	static int CurrentAmmo(Scriptable* Sender, const Trigger* parameters);
	static int CurrentAreaIs(Scriptable* Sender, const Trigger* parameters);
	static int CutSceneBroken(Scriptable* /*Sender*/, const Trigger* /*parameters*/);
	static int DamageTaken(Scriptable* Sender, const Trigger* parameters);
	static int DamageTakenGT(Scriptable* Sender, const Trigger* parameters);
	static int DamageTakenLT(Scriptable* Sender, const Trigger* parameters);
	static int Dead(Scriptable* Sender, const Trigger* parameters);
	static int Delay(Scriptable* Sender, const Trigger* parameters);
	static int Detect(Scriptable* Sender, const Trigger* parameters);
	static int Detected(Scriptable* Sender, const Trigger* parameters);
	static int Die(Scriptable* Sender, const Trigger* parameters);
	static int Died(Scriptable* Sender, const Trigger* parameters);
	static int Difficulty(Scriptable* Sender, const Trigger* parameters);
	static int DifficultyGT(Scriptable* Sender, const Trigger* parameters);
	static int DifficultyLT(Scriptable* Sender, const Trigger* parameters);
	static int Disarmed(Scriptable* Sender, const Trigger* parameters);
	static int DisarmFailed(Scriptable* Sender, const Trigger* parameters);
	static int E(Scriptable* /*Sender*/, const Trigger* parameters);
	static int Entered(Scriptable* Sender, const Trigger* parameters);
	static int EntirePartyOnMap(Scriptable* Sender, const Trigger* parameters);
	static int Exists(Scriptable* Sender, const Trigger* parameters);
	static int ExtendedStateCheck(Scriptable* Sender, const Trigger* parameters);
	static int ExtraProficiency(Scriptable* Sender, const Trigger* parameters);
	static int ExtraProficiencyGT(Scriptable* Sender, const Trigger* parameters);
	static int ExtraProficiencyLT(Scriptable* Sender, const Trigger* parameters);
	static int Eval(Scriptable* /*Sender*/, const Trigger* /*parameters*/);
	static int Faction(Scriptable* Sender, const Trigger* parameters);
	static int FallenPaladin(Scriptable* Sender, const Trigger* parameters);
	static int FallenRanger(Scriptable* Sender, const Trigger* parameters);
	static int False(Scriptable* Sender, const Trigger* parameters);
	static int ForceMarkedSpell_Trigger(Scriptable* Sender, const Trigger* parameters);
	static int Frame(Scriptable* Sender, const Trigger* parameters);
	static int Gender(Scriptable* Sender, const Trigger* parameters);
	static int General(Scriptable* Sender, const Trigger* parameters);
	static int G_Trigger(Scriptable* Sender, const Trigger* parameters);
	static int Global(Scriptable* Sender, const Trigger* parameters);
	static int GlobalAndGlobal_Trigger(Scriptable* Sender, const Trigger* parameters);
	static int GlobalBAndGlobal_Trigger(Scriptable* Sender, const Trigger* parameters);
	static int GlobalBAndGlobalExact(Scriptable* Sender, const Trigger* parameters);
	static int GlobalBitGlobal_Trigger(Scriptable* Sender, const Trigger* parameters);
	static int GlobalGT(Scriptable* Sender, const Trigger* parameters);
	static int GlobalGTGlobal(Scriptable* Sender, const Trigger* parameters);
	static int GlobalLT(Scriptable* Sender, const Trigger* parameters);
	static int GlobalLTGlobal(Scriptable* Sender, const Trigger* parameters);
	static int GlobalOrGlobal_Trigger(Scriptable* Sender, const Trigger* parameters);
	static int GlobalsEqual(Scriptable* Sender, const Trigger* parameters);
	static int GlobalsGT(Scriptable* Sender, const Trigger* parameters);
	static int GlobalsLT(Scriptable* Sender, const Trigger* parameters);
	static int GlobalTimerExact(Scriptable* Sender, const Trigger* parameters);
	static int GlobalTimerExpired(Scriptable* Sender, const Trigger* parameters);
	static int GlobalTimerNotExpired(Scriptable* Sender, const Trigger* parameters);
	static int GlobalTimerStarted(Scriptable* Sender, const Trigger* parameters);
	static int GGT_Trigger(Scriptable* Sender, const Trigger* parameters);
	static int GLT_Trigger(Scriptable* Sender, const Trigger* parameters);
	static int GT(Scriptable* /*Sender*/, const Trigger* parameters);
	static int Happiness(Scriptable* Sender, const Trigger* parameters);
	static int HappinessGT(Scriptable* Sender, const Trigger* parameters);
	static int HappinessLT(Scriptable* Sender, const Trigger* parameters);
	static int HarmlessClosed(Scriptable* Sender, const Trigger* parameters);
	static int HarmlessEntered(Scriptable* Sender, const Trigger* parameters);
	static int HarmlessOpened(Scriptable* Sender, const Trigger* parameters);
	static int HasBounceEffects(Scriptable* Sender, const Trigger* parameters);
	static int HasDLC(Scriptable* /*Sender*/, const Trigger* /*parameters*/);
	static int HasImmunityEffects(Scriptable* Sender, const Trigger* parameters);
	static int HasInnateAbility(Scriptable* Sender, const Trigger* parameters);
	static int HasItem(Scriptable* Sender, const Trigger* parameters);
	static int HasItemCategory(Scriptable* Sender, const Trigger* parameters);
	static int HasItemEquipped(Scriptable* Sender, const Trigger* parameters);
	static int HasItemEquippedReal(Scriptable* Sender, const Trigger* parameters);
	static int HasItemSlot(Scriptable* Sender, const Trigger* parameters);
	static int HasItemType(Scriptable* Sender, const Trigger* parameters);
	static int HasItemTypeSlot(Scriptable* Sender, const Trigger* parameters);
	static int HasWeaponEquipped(Scriptable* Sender, const Trigger* parameters);
	static int HaveAnySpells(Scriptable* Sender, const Trigger* parameters);
	static int HaveSpellParty(Scriptable* Sender, const Trigger* parameters);
	static int HaveSpell(Scriptable* Sender, const Trigger* parameters);
	static int HaveUsableWeaponEquipped(Scriptable* Sender, const Trigger* parameters);
	static int Heard(Scriptable* Sender, const Trigger* parameters);
	static int Help_Trigger(Scriptable* Sender, const Trigger* parameters);
	static int HelpEX(Scriptable* Sender, const Trigger* parameters);
	static int HitBy(Scriptable* Sender, const Trigger* parameters);
	static int HotKey(Scriptable* Sender, const Trigger* parameters);
	static int HP(Scriptable* Sender, const Trigger* parameters);
	static int HPGT(Scriptable* Sender, const Trigger* parameters);
	static int HPLost(Scriptable* Sender, const Trigger* parameters);
	static int HPLostGT(Scriptable* Sender, const Trigger* parameters);
	static int HPLostLT(Scriptable* Sender, const Trigger* parameters);
	static int HPLT(Scriptable* Sender, const Trigger* parameters);
	static int HPPercent(Scriptable* Sender, const Trigger* parameters);
	static int HPPercentGT(Scriptable* Sender, const Trigger* parameters);
	static int HPPercentLT(Scriptable* Sender, const Trigger* parameters);
	static int ImmuneToSpellLevel(Scriptable* Sender, const Trigger* parameters);
	static int InActiveArea(Scriptable* Sender, const Trigger* parameter);
	static int InCutSceneMode(Scriptable* Sender, const Trigger* parameter);
	static int InLine(Scriptable* Sender, const Trigger* parameter);
	static int InMyArea(Scriptable* Sender, const Trigger* parameter);
	static int InMyGroup(Scriptable* Sender, const Trigger* parameter);
	static int InParty(Scriptable* Sender, const Trigger* parameters);
	static int InPartyAllowDead(Scriptable* Sender, const Trigger* parameters);
	static int InPartySlot(Scriptable* Sender, const Trigger* parameters);
	static int InteractingWith(Scriptable* Sender, const Trigger* parameters);
	static int Internal(Scriptable* Sender, const Trigger* parameters);
	static int InternalGT(Scriptable* Sender, const Trigger* parameters);
	static int InternalLT(Scriptable* Sender, const Trigger* parameters);
	static int InTrap(Scriptable* Sender, const Trigger* parameters);
	static int InventoryFull(Scriptable* Sender, const Trigger* parameter);
	static int InWeaponRange(Scriptable* Sender, const Trigger* parameter);
	static int INI(Scriptable* /*Sender*/, const Trigger* parameters);
	static int IsAClown(Scriptable* Sender, const Trigger* parameters);
	static int IsActive(Scriptable* Sender, const Trigger* parameters);
	static int IsCreatureAreaFlag(Scriptable* Sender, const Trigger* parameters);
	static int IsCreatureHiddenInShadows(Scriptable* Sender, const Trigger* parameters);
	static int IsGabber(Scriptable* Sender, const Trigger* parameters);
	static int IsExtendedNight(Scriptable* Sender, const Trigger* parameters);
	static int IsFacingObject(Scriptable* Sender, const Trigger* parameters);
	static int IsFacingSavedRotation(Scriptable* Sender, const Trigger* parameters);
	static int IsForcedRandomEncounterActive(Scriptable* /*Sender*/, const Trigger* /*parameters*/);
	static int IsInGuardianMantle(Scriptable* Sender, const Trigger* parameters);
	static int IsLocked(Scriptable* Sender, const Trigger* parameters);
	static int IsMarkedSpell(Scriptable* Sender, const Trigger* parameters);
	static int IsOverMe(Scriptable* Sender, const Trigger* parameters);
	static int IsPathCriticalObject(Scriptable* Sender, const Trigger* parameters);
	static int IsPlayerNumber(Scriptable* Sender, const Trigger* parameters);
	static int IsRotation(Scriptable* Sender, const Trigger* parameters);
	static int IsSpellTargetValid(Scriptable* Sender, const Trigger* parameters);
	static int IsTeamBitOn(Scriptable* Sender, const Trigger* parameters);
	static int IsTouchGUI(Scriptable* /*Sender*/, const Trigger* /*parameters*/);
	static int IsValidForPartyDialog(Scriptable* Sender, const Trigger* parameters);
	static int IsWeaponRanged(Scriptable* Sender, const Trigger* parameters);
	static int IsWeather(Scriptable* Sender, const Trigger* parameters);
	static int ItemIsIdentified(Scriptable* Sender, const Trigger* parameters);
	static int Joins(Scriptable* Sender, const Trigger* parameters);
	static int Kit(Scriptable* Sender, const Trigger* parameters);
	static int Killed(Scriptable* Sender, const Trigger* parameters);
	static int KnowSpell(Scriptable* Sender, const Trigger* parameters);
	static int LastMarkedObject_Trigger(Scriptable* Sender, const Trigger* parameters);
	static int LastPersonTalkedTo(Scriptable* Sender, const Trigger* parameters);
	static int Leaves(Scriptable* Sender, const Trigger* parameters);
	static int Level(Scriptable* Sender, const Trigger* parameters);
	static int LevelGT(Scriptable* Sender, const Trigger* parameters);
	static int LevelLT(Scriptable* Sender, const Trigger* parameters);
	static int LevelInClass(Scriptable* Sender, const Trigger* parameters);
	static int LevelInClassGT(Scriptable* Sender, const Trigger* parameters);
	static int LevelInClassLT(Scriptable* Sender, const Trigger* parameters);
	static int LevelParty(Scriptable* Sender, const Trigger* parameters);
	static int LevelPartyGT(Scriptable* Sender, const Trigger* parameters);
	static int LevelPartyLT(Scriptable* Sender, const Trigger* parameters);
	static int LocalsEqual(Scriptable* Sender, const Trigger* parameters);
	static int LocalsGT(Scriptable* Sender, const Trigger* parameters);
	static int LocalsLT(Scriptable* Sender, const Trigger* parameters);
	static int LOS(Scriptable* Sender, const Trigger* parameters);
	static int LT(Scriptable* /*Sender*/, const Trigger* parameters);
	static int ModalState(Scriptable* Sender, const Trigger* parameters);
	static int Morale(Scriptable* Sender, const Trigger* parameters);
	static int MoraleGT(Scriptable* Sender, const Trigger* parameters);
	static int MoraleLT(Scriptable* Sender, const Trigger* parameters);
	static int MovementRate(Scriptable* Sender, const Trigger* parameters);
	static int MovementRateGT(Scriptable* Sender, const Trigger* parameters);
	static int MovementRateLT(Scriptable* Sender, const Trigger* parameters);
	static int NamelessBitTheDust(Scriptable* Sender, const Trigger* parameters);
	static int NearbyDialog(Scriptable* Sender, const Trigger* parameters);
	static int NearLocation(Scriptable* Sender, const Trigger* parameters);
	static int NearSavedLocation(Scriptable* Sender, const Trigger* parameters);
	static int NightmareModeOn(Scriptable* Sender, const Trigger* parameters);
	static int NotStateCheck(Scriptable* Sender, const Trigger* parameters);
	static int NullDialog(Scriptable* Sender, const Trigger* parameters);
	static int NumBouncingSpellLevel(Scriptable* Sender, const Trigger* parameters);
	static int NumBouncingSpellLevelGT(Scriptable* Sender, const Trigger* parameters);
	static int NumBouncingSpellLevelLT(Scriptable* Sender, const Trigger* parameters);
	static int NumCreatures(Scriptable* Sender, const Trigger* parameters);
	static int NumCreaturesAtMyLevel(Scriptable* Sender, const Trigger* parameters);
	static int NumCreaturesGT(Scriptable* Sender, const Trigger* parameters);
	static int NumCreaturesGTMyLevel(Scriptable* Sender, const Trigger* parameters);
	static int NumCreaturesLT(Scriptable* Sender, const Trigger* parameters);
	static int NumCreaturesLTMyLevel(Scriptable* Sender, const Trigger* parameters);
	static int NumCreatureVsParty(Scriptable* Sender, const Trigger* parameters);
	static int NumCreatureVsPartyGT(Scriptable* Sender, const Trigger* parameters);
	static int NumCreatureVsPartyLT(Scriptable* Sender, const Trigger* parameters);
	static int NumDead(Scriptable* Sender, const Trigger* parameters);
	static int NumDeadGT(Scriptable* Sender, const Trigger* parameters);
	static int NumDeadLT(Scriptable* Sender, const Trigger* parameters);
	static int NumImmuneToSpellLevel(Scriptable* Sender, const Trigger* parameters);
	static int NumImmuneToSpellLevelGT(Scriptable* Sender, const Trigger* parameters);
	static int NumImmuneToSpellLevelLT(Scriptable* Sender, const Trigger* parameters);
	static int NumItems(Scriptable* Sender, const Trigger* parameters);
	static int NumItemsGT(Scriptable* Sender, const Trigger* parameters);
	static int NumItemsLT(Scriptable* Sender, const Trigger* parameters);
	static int NumItemsParty(Scriptable* Sender, const Trigger* parameters);
	static int NumItemsPartyGT(Scriptable* Sender, const Trigger* parameters);
	static int NumItemsPartyLT(Scriptable* Sender, const Trigger* parameters);
	static int NumMirrorImages(Scriptable* Sender, const Trigger* parameters);
	static int NumMirrorImagesGT(Scriptable* Sender, const Trigger* parameters);
	static int NumMirrorImagesLT(Scriptable* Sender, const Trigger* parameters);
	static int NumKilledByParty(Scriptable* /*Sender*/, const Trigger* parameters);
	static int NumKilledByPartyGT(Scriptable* /*Sender*/, const Trigger* parameters);
	static int NumKilledByPartyLT(Scriptable* /*Sender*/, const Trigger* parameters);
	static int NumTimesInteracted(Scriptable* Sender, const Trigger* parameters);
	static int NumTimesInteractedGT(Scriptable* Sender, const Trigger* parameters);
	static int NumTimesInteractedLT(Scriptable* Sender, const Trigger* parameters);
	static int NumTimesInteractedObject(Scriptable* Sender, const Trigger* parameters);
	static int NumTimesInteractedObjectGT(Scriptable* Sender, const Trigger* parameters);
	static int NumTimesInteractedObjectLT(Scriptable* Sender, const Trigger* parameters);
	static int NumTimesTalkedTo(Scriptable* Sender, const Trigger* parameters);
	static int NumTimesTalkedToGT(Scriptable* Sender, const Trigger* parameters);
	static int NumTimesTalkedToLT(Scriptable* Sender, const Trigger* parameters);
	static int NumTrappingSpellLevel(Scriptable* Sender, const Trigger* parameters);
	static int NumTrappingSpellLevelGT(Scriptable* Sender, const Trigger* parameters);
	static int NumTrappingSpellLevelLT(Scriptable* Sender, const Trigger* parameters);
	static int ObjectActionListEmpty(Scriptable* Sender, const Trigger* parameters);
	static int OnCreation(Scriptable* Sender, const Trigger* parameters);
	static int OnIsland(Scriptable* Sender, const Trigger* parameters);
	static int OnScreen(Scriptable* Sender, const Trigger* parameters);
	static int Opened(Scriptable* Sender, const Trigger* parameters);
	static int OpenFailed(Scriptable* Sender, const Trigger* parameters);
	static int OpenState(Scriptable* Sender, const Trigger* parameters);
	static int Or(Scriptable* Sender, const Trigger* parameters);
	static int OriginalClass(Scriptable* Sender, const Trigger* parameters);
	static int OutOfAmmo(Scriptable* Sender, const Trigger* parameters);
	static int OwnsFloaterMessage(Scriptable* Sender, const Trigger* parameters);
	static int PartyCountEQ(Scriptable* Sender, const Trigger* parameters);
	static int PartyCountGT(Scriptable* Sender, const Trigger* parameters);
	static int PartyCountLT(Scriptable* Sender, const Trigger* parameters);
	static int PartyCountAliveEQ(Scriptable* Sender, const Trigger* parameters);
	static int PartyCountAliveGT(Scriptable* Sender, const Trigger* parameters);
	static int PartyCountAliveLT(Scriptable* Sender, const Trigger* parameters);
	static int PartyGold(Scriptable* Sender, const Trigger* parameters);
	static int PartyGoldGT(Scriptable* Sender, const Trigger* parameters);
	static int PartyGoldLT(Scriptable* Sender, const Trigger* parameters);
	static int PartyHasItem(Scriptable* Sender, const Trigger* parameters);
	static int PartyHasItemIdentified(Scriptable* Sender, const Trigger* parameters);
	static int PartyMemberDied(Scriptable* Sender, const Trigger* parameters);
	static int PartyRested(Scriptable* Sender, const Trigger* parameters);
	static int PCCanSeePoint(Scriptable* Sender, const Trigger* parameters);
	static int PCInStore(Scriptable* Sender, const Trigger* parameters);
	static int PersonalSpaceDistance(Scriptable* Sender, const Trigger* parameters);
	static int PickLockFailed(Scriptable* Sender, const Trigger* parameters);
	static int PickpocketFailed(Scriptable* Sender, const Trigger* parameters);
	static int Proficiency(Scriptable* Sender, const Trigger* parameters);
	static int ProficiencyGT(Scriptable* Sender, const Trigger* parameters);
	static int ProficiencyLT(Scriptable* Sender, const Trigger* parameters);
	static int Race(Scriptable* Sender, const Trigger* parameters);
	static int RandomNum(Scriptable* Sender, const Trigger* parameters);
	static int RandomNumGT(Scriptable* Sender, const Trigger* parameters);
	static int RandomNumLT(Scriptable* Sender, const Trigger* parameters);
	static int RandomStatCheck(Scriptable* Sender, const Trigger* parameters);
	static int Range(Scriptable* Sender, const Trigger* parameters);
	static int Reaction(Scriptable* Sender, const Trigger* parameters);
	static int ReactionLT(Scriptable* Sender, const Trigger* parameters);
	static int ReactionGT(Scriptable* Sender, const Trigger* parameters);
	static int RealGlobalTimerExact(Scriptable* Sender, const Trigger* parameters);
	static int RealGlobalTimerExpired(Scriptable* Sender, const Trigger* parameters);
	static int RealGlobalTimerNotExpired(Scriptable* Sender, const Trigger* parameters);
	static int ReceivedOrder(Scriptable* Sender, const Trigger* parameters);
	static int Reputation(Scriptable* Sender, const Trigger* parameters);
	static int ReputationGT(Scriptable* Sender, const Trigger* parameters);
	static int ReputationLT(Scriptable* Sender, const Trigger* parameters);
	static int Reset(Scriptable* Sender, const Trigger* parameters);
	static int School(Scriptable* Sender, const Trigger* parameters);
	static int SecretDoorDetected(Scriptable* Sender, const Trigger* parameters);
	static int See(Scriptable* Sender, const Trigger* parameters);
	static int Sequence(Scriptable* Sender, const Trigger* parameters);
	static int SetLastMarkedObject(Scriptable* Sender, const Trigger* parameters);
	static int SetMarkedSpell_Trigger(Scriptable* Sender, const Trigger* parameters);
	static int SetSpellTarget(Scriptable* Sender, const Trigger* parameters);
	static int Specifics(Scriptable* Sender, const Trigger* parameters);
	static int SpellCast(Scriptable* Sender, const Trigger* parameters);
	static int SpellCastInnate(Scriptable* Sender, const Trigger* parameters);
	static int SpellCastOnMe(Scriptable* Sender, const Trigger* parameters);
	static int SpellCastPriest(Scriptable* Sender, const Trigger* parameters);
	static int StateCheck(Scriptable* Sender, const Trigger* parameters);
	static int StealFailed(Scriptable* Sender, const Trigger* parameters);
	static int StoreHasItem(Scriptable* Sender, const Trigger* parameters);
	static int StoryModeOn(Scriptable* Sender, const Trigger* /*parameters*/);
	static int StuffGlobalRandom(Scriptable* Sender, const Trigger* parameters);
	static int SubRace(Scriptable* Sender, const Trigger* parameters);
	static int Summoned(Scriptable* Sender, const Trigger* parameters);
	static int SummoningLimit(Scriptable* Sender, const Trigger* parameters);
	static int SummoningLimitGT(Scriptable* Sender, const Trigger* parameters);
	static int SummoningLimitLT(Scriptable* Sender, const Trigger* parameters);
	static int Switch(Scriptable* /*Sender*/, const Trigger* /*parameters*/);
	static int SystemVariable_Trigger(Scriptable* Sender, const Trigger* parameters);
	static int TargetUnreachable(Scriptable* Sender, const Trigger* parameters);
	static int Team(Scriptable* Sender, const Trigger* parameters);
	static int Time(Scriptable* Sender, const Trigger* parameters);
	static int TimeGT(Scriptable* Sender, const Trigger* parameters);
	static int TimeLT(Scriptable* Sender, const Trigger* parameters);
	static int TimeOfDay(Scriptable* Sender, const Trigger* parameters);
	static int TimerActive(Scriptable* Sender, const Trigger* parameters);
	static int TimerExpired(Scriptable* Sender, const Trigger* parameters);
	static int TimeStopCounter(Scriptable* /*Sender*/, const Trigger* parameters);
	static int TimeStopCounterGT(Scriptable* /*Sender*/, const Trigger* parameters);
	static int TimeStopCounterLT(Scriptable* /*Sender*/, const Trigger* parameters);
	static int TimeStopObject(Scriptable* Sender, const Trigger* parameters);
	static int TookDamage(Scriptable* Sender, const Trigger* parameters);
	static int TotalItemCnt(Scriptable* Sender, const Trigger* parameters);
	static int TotalItemCntExclude(Scriptable* Sender, const Trigger* parameters);
	static int TotalItemCntExcludeGT(Scriptable* Sender, const Trigger* parameters);
	static int TotalItemCntExcludeLT(Scriptable* Sender, const Trigger* parameters);
	static int TotalItemCntGT(Scriptable* Sender, const Trigger* parameters);
	static int TotalItemCntLT(Scriptable* Sender, const Trigger* parameters);
	static int TrapTriggered(Scriptable* Sender, const Trigger* parameters);
	static int TriggerTrigger(Scriptable* Sender, const Trigger* parameters);
	static int TriggerSetGlobal(Scriptable* Sender, const Trigger* parameters);
	static int True(Scriptable* Sender, const Trigger* parameters);
	static int TurnedBy(Scriptable* Sender, const Trigger* parameters);
	static int Unlocked(Scriptable* Sender, const Trigger* parameters);
	static int UnselectableVariable(Scriptable* Sender, const Trigger* parameters);
	static int UnselectableVariableGT(Scriptable* Sender, const Trigger* parameters);
	static int UnselectableVariableLT(Scriptable* Sender, const Trigger* parameters);
	static int Unusable(Scriptable* Sender, const Trigger* parameters);
	static int UsedExit(Scriptable* Sender, const Trigger* parameters);
	static int Vacant(Scriptable* Sender, const Trigger* parameters);
	static int WalkedToTrigger(Scriptable* Sender, const Trigger* parameters);
	static int WasInDialog(Scriptable* Sender, const Trigger* parameters);
	static int WeaponCanDamage(Scriptable* Sender, const Trigger* parameters);
	static int WeaponEffectiveVs(Scriptable* Sender, const Trigger* parameters);
	static int Xor(Scriptable* Sender, const Trigger* parameters);
	static int XP(Scriptable* Sender, const Trigger* parameters);
	static int XPGT(Scriptable* Sender, const Trigger* parameters);
	static int XPLT(Scriptable* Sender, const Trigger* parameters);

	//Actions
	static void Activate(Scriptable* Sender, Holder<Action> parameters);
	static void ActivatePortalCursor(Scriptable* Sender, Holder<Action> parameters);
	static void AddAreaFlag(Scriptable* Sender, Holder<Action> parameters);
	static void AddAreaType(Scriptable* Sender, Holder<Action> parameters);
	static void AddExperienceParty(Scriptable* Sender, Holder<Action> parameters);
	static void AddExperiencePartyCR(Scriptable* Sender, Holder<Action> parameters);
	static void AddExperiencePartyGlobal(Scriptable* Sender, Holder<Action> parameters);
	static void AddFamiliar(Scriptable* Sender, Holder<Action> parameters);
	static void AddFeat(Scriptable* Sender, Holder<Action> parameters);
	static void AddGlobals(Scriptable* Sender, Holder<Action> parameters);
	static void AddHP(Scriptable* Sender, Holder<Action> parameters);
	static void AddJournalEntry(Scriptable* Sender, Holder<Action> parameters);
	static void AddKit(Scriptable* Sender, Holder<Action> parameters);
	static void AddMapnote(Scriptable* Sender, Holder<Action> parameters);
	static void AddSpecialAbility(Scriptable* Sender, Holder<Action> parameters);
	static void AddStoreItem(Scriptable* /*Sender*/, Holder<Action> parameters);
	static void AddSuperKit(Scriptable* Sender, Holder<Action> parameters);
	static void AddWayPoint(Scriptable* Sender, Holder<Action> parameters);
	static void AddWorldmapAreaFlag(Scriptable* /*Sender*/, Holder<Action> parameters);
	static void AddXP2DA(Scriptable* Sender, Holder<Action> parameters);
	static void AddXPObject(Scriptable* Sender, Holder<Action> parameters);
	static void AddXPVar(Scriptable* Sender, Holder<Action> parameters);
	static void AddXPWorth(Scriptable* Sender, Holder<Action> parameters);
	static void AdvanceTime(Scriptable* Sender, Holder<Action> parameters);
	static void Ally(Scriptable* Sender, Holder<Action> parameters);
	static void AmbientActivate(Scriptable* Sender, Holder<Action> parameters);
	static void AnkhegEmerge(Scriptable* Sender, Holder<Action> parameters);
	static void AnkhegHide(Scriptable* Sender, Holder<Action> parameters);
	static void ApplyDamage(Scriptable* Sender, Holder<Action> parameters);
	static void ApplyDamagePercent(Scriptable* Sender, Holder<Action> parameters);
	static void ApplySpell(Scriptable* Sender, Holder<Action> parameters);
	static void ApplySpellPoint(Scriptable* Sender, Holder<Action> parameters);
	static void AttachTransitionToDoor(Scriptable* Sender, Holder<Action> parameters);
	static void Attack(Scriptable* Sender, Holder<Action> parameters);
	static void AttackNoSound(Scriptable* Sender, Holder<Action> parameters);
	static void AttackOneRound(Scriptable* Sender, Holder<Action> parameters);
	static void AttackReevaluate(Scriptable* Sender, Holder<Action> parameters);
	static void BanterBlockFlag(Scriptable* Sender, Holder<Action> parameters);
	static void BanterBlockTime(Scriptable* Sender, Holder<Action> parameters);
	static void BashDoor(Scriptable* Sender, Holder<Action> parameters);
	static void BattleSong(Scriptable* Sender, Holder<Action> parameters);
	static void Berserk(Scriptable* Sender, Holder<Action> parameters);
	static void BitClear(Scriptable* Sender, Holder<Action> parameters);
	static void BitGlobal(Scriptable* Sender, Holder<Action> parameters);
	static void BreakInstants(Scriptable* Sender, Holder<Action> parameters);
	static void Calm(Scriptable* Sender, Holder<Action> parameters);
	static void ChangeAIScript(Scriptable* Sender, Holder<Action> parameters);
	static void ChangeAIType(Scriptable* Sender, Holder<Action> parameters);
	static void ChangeAlignment(Scriptable* Sender, Holder<Action> parameters);
	static void ChangeAnimation(Scriptable* Sender, Holder<Action> parameters);
	static void ChangeAnimationNoEffect(Scriptable* Sender, Holder<Action> parameters);
	static void ChangeClass(Scriptable* Sender, Holder<Action> parameters);
	static void ChangeColor(Scriptable* Sender, Holder<Action> parameters);
	static void ChangeDestination(Scriptable* Sender, Holder<Action> parameters);
	static void ChangeDialogue(Scriptable* Sender, Holder<Action> parameters);
	static void ChangeEnemyAlly(Scriptable* Sender, Holder<Action> parameters);
	static void ChangeGender(Scriptable* Sender, Holder<Action> parameters);
	static void ChangeGeneral(Scriptable* Sender, Holder<Action> parameters);
	static void ChangeRace(Scriptable* Sender, Holder<Action> parameters);
	static void ChangeSpecifics(Scriptable* Sender, Holder<Action> parameters);
	static void ChangeStat(Scriptable* Sender, Holder<Action> parameters);
	static void ChangeStatGlobal(Scriptable* Sender, Holder<Action> parameters);
	static void ChangeStoreMarkup(Scriptable* Sender, Holder<Action> parameters);
	static void ChangeTileState(Scriptable* Sender, Holder<Action> parameters);
	static void ChunkCreature(Scriptable* Sender, Holder<Action> parameters);
	static void ClearActions(Scriptable* Sender, Holder<Action> parameters);
	static void ClearAllActions(Scriptable* Sender, Holder<Action> parameters);
	static void ClearPartyEffects(Scriptable* Sender, Holder<Action> parameters);
	static void ClearSpriteEffects(Scriptable* Sender, Holder<Action> parameters);
	static void ClickLButtonObject(Scriptable* Sender, Holder<Action> parameters);
	static void ClickLButtonPoint(Scriptable* Sender, Holder<Action> parameters);
	static void ClickRButtonObject(Scriptable* Sender, Holder<Action> parameters);
	static void ClickRButtonPoint(Scriptable* Sender, Holder<Action> parameters);
	static void CloseDoor(Scriptable* Sender, Holder<Action> parameters);
	static void ContainerEnable(Scriptable* Sender, Holder<Action> parameters);
	static void Continue(Scriptable* Sender, Holder<Action> parameters);
	static void CopyGroundPilesTo(Scriptable* Sender, Holder<Action> parameters);
	static void CreateCreature(Scriptable* Sender, Holder<Action> parameters);
	static void CreateCreatureAtLocation(Scriptable* Sender, Holder<Action> parameters);
	static void CreateCreatureAtFeet(Scriptable* Sender, Holder<Action> parameters);
	static void CreateCreatureCopyPoint(Scriptable* Sender, Holder<Action> parameters);
	static void CreateCreatureDoor(Scriptable* Sender, Holder<Action> parameters);
	static void CreateCreatureImpassable(Scriptable* Sender, Holder<Action> parameters);
	static void CreateCreatureImpassableAllowOverlap(Scriptable* Sender,
							 Holder<Action> parameters);
	static void CreateCreatureObject(Scriptable* Sender, Holder<Action> parameters);
	static void CreateCreatureObjectCopy(Scriptable* Sender, Holder<Action> parameters);
	static void CreateCreatureObjectDoor(Scriptable* Sender, Holder<Action> parameters);
	static void CreateCreatureObjectOffset(Scriptable* Sender, Holder<Action> parameters);
	static void CreateCreatureObjectOffScreen(Scriptable* Sender, Holder<Action> parameters);
	static void CreateCreatureOffScreen(Scriptable* Sender, Holder<Action> parameters);
	static void CreateItem(Scriptable* Sender, Holder<Action> parameters);
	static void CreateItemNumGlobal(Scriptable* Sender, Holder<Action> parameters);
	static void CreatePartyGold(Scriptable* Sender, Holder<Action> parameters);
	static void CreateVisualEffect(Scriptable* Sender, Holder<Action> parameters);
	static void CreateVisualEffectObject(Scriptable* Sender,
					     Holder<Action> parameters);
	static void CreateVisualEffectObjectSticky(Scriptable* Sender,
						   Holder<Action> parameters);
	static void CutSceneID(Scriptable* Sender, Holder<Action> parameters);
	static void Damage(Scriptable* Sender, Holder<Action> parameters);
	static void DayNight(Scriptable* Sender, Holder<Action> parameters);
	static void Deactivate(Scriptable* Sender, Holder<Action> parameters);
	static void Debug(Scriptable* Sender, Holder<Action> parameters);
	static void DemoEnd(Scriptable* Sender, Holder<Action> parameters);
	static void DestroyAllDestructableEquipment(Scriptable* Sender,
						    Holder<Action> parameters);
	static void DestroyAllEquipment(Scriptable* Sender, Holder<Action> parameters);
	static void DestroyGold(Scriptable* Sender, Holder<Action> parameters);
	static void DestroyGroundPiles(Scriptable* Sender, Holder<Action> /*parameters*/);
	static void DestroyItem(Scriptable* Sender, Holder<Action> parameters);
	static void DestroyPartyGold(Scriptable* Sender, Holder<Action> parameters);
	static void DestroyPartyItem(Scriptable* Sender, Holder<Action> parameters);
	static void DestroyPartyItemNum(Scriptable* Sender, Holder<Action> parameters);
	static void DestroySelf(Scriptable* Sender, Holder<Action> parameters);
	static void DetectSecretDoor(Scriptable* Sender, Holder<Action> parameters);
	static void Dialogue(Scriptable* Sender, Holder<Action> parameters);
	static void DialogueForceInterrupt(Scriptable* Sender, Holder<Action> parameters);
	static void DialogueInterrupt(Scriptable* Sender, Holder<Action> parameters);
	static void DisableFogDither(Scriptable* Sender, Holder<Action> parameters);
	static void DisableSpriteDither(Scriptable* Sender, Holder<Action> parameters);
	static void DisplayMessage(Scriptable* Sender, Holder<Action> parameters);
	static void DisplayString(Scriptable* Sender, Holder<Action> parameters);
	static void DisplayStringHead(Scriptable* Sender, Holder<Action> parameters);
	static void DisplayStringHeadNoLog(Scriptable* Sender, Holder<Action> parameters);
	static void DisplayStringHeadOwner(Scriptable* Sender, Holder<Action> parameters);
	static void DisplayStringNoName(Scriptable* Sender, Holder<Action> parameters);
	static void DisplayStringNoNameHead(Scriptable* Sender, Holder<Action> parameters);
	static void DisplayStringWait(Scriptable* Sender, Holder<Action> parameters);
	static void DoubleClickLButtonObject(Scriptable* Sender, Holder<Action> parameters);
	static void DoubleClickLButtonPoint(Scriptable* Sender, Holder<Action> parameters);
	static void DoubleClickRButtonObject(Scriptable* Sender, Holder<Action> parameters);
	static void DoubleClickRButtonPoint(Scriptable* Sender, Holder<Action> parameters);
	static void DropInventory(Scriptable* Sender, Holder<Action> parameters);
	static void DropInventoryEX(Scriptable* Sender, Holder<Action> parameters);
	static void DropItem(Scriptable* Sender, Holder<Action> parameters);
	static void EnableFogDither(Scriptable* Sender, Holder<Action> parameters);
	static void EnablePortalTravel(Scriptable* Sender, Holder<Action> parameters);
	static void EnableSpriteDither(Scriptable* Sender, Holder<Action> parameters);
	static void EndCredits(Scriptable* Sender, Holder<Action> parameters);
	static void EndCutSceneMode(Scriptable* Sender, Holder<Action> parameters);
	static void Enemy(Scriptable* Sender, Holder<Action> parameters);
	static void EscapeArea(Scriptable* Sender, Holder<Action> parameters);
	static void EscapeAreaDestroy(Scriptable* Sender, Holder<Action> parameters);
	static void EscapeAreaNoSee(Scriptable* Sender, Holder<Action> parameters);
	static void EscapeAreaObject(Scriptable* Sender, Holder<Action> parameters);
	static void EscapeAreaObjectNoSee(Scriptable* Sender, Holder<Action> parameters);
	static void EquipItem(Scriptable* Sender, Holder<Action> parameters);
	static void EquipMostDamagingMelee(Scriptable* Sender, Holder<Action> parameters);
	static void EquipRanged(Scriptable* Sender, Holder<Action> parameters);
	static void EquipWeapon(Scriptable* Sender, Holder<Action> parameters);
	static void ExitPocketPlane(Scriptable* Sender, Holder<Action> parameters);
	static void ExpansionEndCredits(Scriptable* Sender, Holder<Action> parameters);
	static void Explore(Scriptable* Sender, Holder<Action> parameters);
	static void ExploreMapChunk(Scriptable* Sender, Holder<Action> parameters);
	static void ExportParty(Scriptable* Sender, Holder<Action> parameters);
	static void Face(Scriptable* Sender, Holder<Action> parameters);
	static void FaceObject(Scriptable* Sender, Holder<Action> parameters);
	static void FaceSavedLocation(Scriptable* Sender, Holder<Action> parameters);
	static void FadeFromColor(Scriptable* Sender, Holder<Action> parameters);
	static void FadeToAndFromColor(Scriptable* Sender, Holder<Action> parameters);
	static void FadeToColor(Scriptable* Sender, Holder<Action> parameters);
	static void FakeEffectExpiryCheck(Scriptable* Sender, Holder<Action> parameters);
	static void FillSlot(Scriptable* Sender, Holder<Action> parameters);
	static void FindTraps(Scriptable* Sender, Holder<Action> parameters);
	static void FixEngineRoom(Scriptable* Sender, Holder<Action> parameters);
	static void FloatMessageFixed(Scriptable* Sender, Holder<Action> parameters);
	static void FloatMessageFixedRnd(Scriptable* Sender, Holder<Action> parameters);
	static void FloatMessageRnd(Scriptable* Sender, Holder<Action> parameters);
	static void FloatRebus(Scriptable* Sender, Holder<Action> parameters);
	static void Follow(Scriptable* Sender, Holder<Action> parameters);
	static void FollowCreature(Scriptable* Sender, Holder<Action> parameters);
	static void FollowObjectFormation(Scriptable* Sender, Holder<Action> parameters);
	static void ForceAIScript(Scriptable* Sender, Holder<Action> parameters);
	static void ForceAttack(Scriptable* Sender, Holder<Action> parameters);
	static void ForceFacing(Scriptable* Sender, Holder<Action> parameters);
	static void ForceHide(Scriptable* Sender, Holder<Action> parameters);
	static void ForceLeaveAreaLUA(Scriptable* Sender, Holder<Action> parameters);
	static void ForceMarkedSpell(Scriptable* Sender, Holder<Action> parameters);
	static void ForceRandomEncounter(Scriptable* Sender, Holder<Action> parameters);
	static void ForceSpell(Scriptable* Sender, Holder<Action> parameters);
	static void ForceSpellRange(Scriptable* Sender, Holder<Action> parameters);
	static void ForceSpellPoint(Scriptable* Sender, Holder<Action> parameters);
	static void ForceSpellPointRange(Scriptable* Sender, Holder<Action> parameters);
	static void ForceUseContainer(Scriptable* Sender, Holder<Action> parameters);
	static void Formation(Scriptable* Sender, Holder<Action> parameters);
	static void FullHeal(Scriptable* Sender, Holder<Action> parameters);
	static void GenerateMaze(Scriptable* Sender, Holder<Action> parameters);
	static void GeneratePartyMember(Scriptable* Sender, Holder<Action> parameters);
	static void GetItem(Scriptable* Sender, Holder<Action> parameters);
	static void GetStat(Scriptable* Sender, Holder<Action> parameters);
	static void GiveItem(Scriptable* Sender, Holder<Action> parameters);
	static void GiveObjectGoldGlobal(Scriptable* Sender, Holder<Action> parameters);
	static void GiveOrder(Scriptable* Sender, Holder<Action> parameters);
	static void GivePartyAllEquipment(Scriptable* Sender, Holder<Action> parameters);
	static void GivePartyGold(Scriptable* Sender, Holder<Action> parameters);
	static void GivePartyGoldGlobal(Scriptable* Sender, Holder<Action> parameters);
	static void GlobalAddGlobal(Scriptable* Sender, Holder<Action> parameters);
	static void GlobalAndGlobal(Scriptable* Sender, Holder<Action> parameters);
	static void GlobalBAnd(Scriptable* Sender, Holder<Action> parameters);
	static void GlobalBAndGlobal(Scriptable* Sender, Holder<Action> parameters);
	static void GlobalBitGlobal(Scriptable* Sender, Holder<Action> parameters);
	static void GlobalBOr(Scriptable* Sender, Holder<Action> parameters);
	static void GlobalBOrGlobal(Scriptable* Sender, Holder<Action> parameters);
	static void GlobalMax(Scriptable* Sender, Holder<Action> parameters);
	static void GlobalMaxGlobal(Scriptable* Sender, Holder<Action> parameters);
	static void GlobalMin(Scriptable* Sender, Holder<Action> parameters);
	static void GlobalMinGlobal(Scriptable* Sender, Holder<Action> parameters);
	static void GlobalOrGlobal(Scriptable* Sender, Holder<Action> parameters);
	static void GlobalSetGlobal(Scriptable* Sender, Holder<Action> parameters);
	static void GlobalShL(Scriptable* Sender, Holder<Action> parameters);
	static void GlobalShLGlobal(Scriptable* Sender, Holder<Action> parameters);
	static void GlobalShout(Scriptable* Sender, Holder<Action> parameters);
	static void GlobalShR(Scriptable* Sender, Holder<Action> parameters);
	static void GlobalShRGlobal(Scriptable* Sender, Holder<Action> parameters);
	static void GlobalSubGlobal(Scriptable* Sender, Holder<Action> parameters);
	static void GlobalXor(Scriptable* Sender, Holder<Action> parameters);
	static void GlobalXorGlobal(Scriptable* Sender, Holder<Action> parameters);
	static void GroupAttack(Scriptable* Sender, Holder<Action> parameters);
	static void Help(Scriptable* Sender, Holder<Action> parameters);
	static void Hide(Scriptable* Sender, Holder<Action> parameters);
	static void HideAreaOnMap(Scriptable* Sender, Holder<Action> parameters);
	static void HideCreature(Scriptable* Sender, Holder<Action> parameters);
	static void HideGUI(Scriptable* Sender, Holder<Action> parameters);
	static void IncInternal(Scriptable* Sender, Holder<Action> parameters);
	static void IncMoraleAI(Scriptable* Sender, Holder<Action> parameters);
	static void IncrementChapter(Scriptable* Sender, Holder<Action> parameters);
	static void IncrementExtraProficiency(Scriptable* Sender, Holder<Action> parameters);
	static void IncrementGlobal(Scriptable* Sender, Holder<Action> parameters);
	static void IncrementGlobalOnce(Scriptable* Sender, Holder<Action> parameters);
	static void IncrementKillStat(Scriptable* Sender, Holder<Action> parameters);
	static void IncrementProficiency(Scriptable* Sender, Holder<Action> parameters);
	static void Interact(Scriptable* Sender, Holder<Action> parameters);
	static void JoinParty(Scriptable* Sender, Holder<Action> parameters);
	static void JumpToObject(Scriptable* Sender, Holder<Action> parameters);
	static void JumpToPoint(Scriptable* Sender, Holder<Action> parameters);
	static void JumpToPointInstant(Scriptable* Sender, Holder<Action> parameters);
	static void JumpToSavedLocation(Scriptable* Sender, Holder<Action> parameters);
	static void Kill(Scriptable* Sender, Holder<Action> parameters);
	static void KillFloatMessage(Scriptable* Sender, Holder<Action> parameters);
	static void Leader(Scriptable* Sender, Holder<Action> parameters);
	static void LeaveAreaLUA(Scriptable* Sender, Holder<Action> parameters);
	static void LeaveAreaLUAEntry(Scriptable* Sender, Holder<Action> parameters);
	static void LeaveAreaLUAPanic(Scriptable* Sender, Holder<Action> parameters);
	static void LeaveAreaLUAPanicEntry(Scriptable* Sender, Holder<Action> parameters);
	static void LeaveParty(Scriptable* Sender, Holder<Action> parameters);
	static void Lock(Scriptable* Sender, Holder<Action> parameters);
	static void LockScroll(Scriptable* Sender, Holder<Action> parameters);
	static void MakeGlobal(Scriptable* Sender, Holder<Action> parameters);
	static void MakeGlobalOverride(Scriptable* Sender, Holder<Action> parameters);
	static void MakeUnselectable(Scriptable* Sender, Holder<Action> parameters);
	static void MarkObject(Scriptable* Sender, Holder<Action> parameters);
	static void MarkSpellAndObject(Scriptable* Sender, Holder<Action> parameters);
	static void MatchHP(Scriptable* Sender, Holder<Action> parameters);
	static void MoraleDec(Scriptable* Sender, Holder<Action> parameters);
	static void MoraleInc(Scriptable* Sender, Holder<Action> parameters);
	static void MoraleSet(Scriptable* Sender, Holder<Action> parameters);
	static void MoveBetweenAreas(Scriptable* Sender, Holder<Action> parameters);
	static void MoveBetweenAreasEffect(Scriptable* Sender, Holder<Action> parameters);
	static void MoveContainerContents(Scriptable* Sender, Holder<Action> parameters);
	static void MoveCursorPoint(Scriptable* Sender, Holder<Action> parameters);
	static void MoveGlobal(Scriptable* Sender, Holder<Action> parameters);
	static void MoveGlobalObject(Scriptable* Sender, Holder<Action> parameters);
	static void MoveGlobalObjectOffScreen(Scriptable* Sender, Holder<Action> parameters);
	static void MoveGlobalsTo(Scriptable* Sender, Holder<Action> parameters);
	static void MoveInventory(Scriptable* Sender, Holder<Action> parameters);
	static void MoveToCampaign(Scriptable* /*Sender*/, Holder<Action> parameters);
	static void MoveToCenterOfScreen(Scriptable* Sender, Holder<Action> parameters);
	static void MoveToExpansion(Scriptable* Sender, Holder<Action> parameters);
	static void MoveToObject(Scriptable* Sender, Holder<Action> parameters);
	static void MoveToObjectFollow(Scriptable* Sender, Holder<Action> parameters);
	static void MoveToObjectNoInterrupt(Scriptable* Sender, Holder<Action> parameters);
	static void MoveToObjectUntilSee(Scriptable* Sender, Holder<Action> parameters);
	static void MoveToOffset(Scriptable* Sender, Holder<Action> parameters);
	static void MoveToPoint(Scriptable* Sender, Holder<Action> parameters);
	static void MoveToPointNoInterrupt(Scriptable* Sender, Holder<Action> parameters);
	static void MoveToPointNoRecticle(Scriptable* Sender, Holder<Action> parameters);
	static void MoveToSavedLocation(Scriptable* Sender, Holder<Action> parameters);
	static void MoveViewPoint(Scriptable* Sender, Holder<Action> parameters);
	static void MoveViewPointUntilDone(Scriptable* Sender, Holder<Action> parameters);
	static void MoveViewObject(Scriptable* Sender, Holder<Action> parameters);
	static void MultiPlayerSync(Scriptable* Sender, Holder<Action> parameters);
	static void NIDSpecial1(Scriptable* Sender, Holder<Action> parameters);
	static void NIDSpecial2(Scriptable* Sender, Holder<Action> parameters);
	static void NoAction(Scriptable* Sender, Holder<Action> parameters);
	static void OpenDoor(Scriptable* Sender, Holder<Action> parameters);
	static void OverrideAreaDifficulty(Scriptable* Sender, Holder<Action> parameters);
	static void Panic(Scriptable* Sender, Holder<Action> parameters);
	static void PauseGame(Scriptable* Sender, Holder<Action> parameters);
	static void PermanentStatChange(Scriptable* Sender, Holder<Action> parameters);
	static void PickLock(Scriptable* Sender, Holder<Action> parameters);
	static void PickPockets(Scriptable* Sender, Holder<Action> parameters);
	static void PickUpItem(Scriptable* Sender, Holder<Action> parameters);
	static void PlayBardSong(Scriptable* Sender, Holder<Action> parameters);
	static void PlayDead(Scriptable* Sender, Holder<Action> parameters);
	static void PlayDeadInterruptible(Scriptable* Sender, Holder<Action> parameters);
	static void PlayerDialogue(Scriptable* Sender, Holder<Action> parameters);
	static void PlaySequence(Scriptable* Sender, Holder<Action> parameters);
	static void PlaySequenceGlobal(Scriptable* Sender, Holder<Action> parameters);
	static void PlaySequenceTimed(Scriptable* Sender, Holder<Action> parameters);
	static void PlaySound(Scriptable* Sender, Holder<Action> parameters);
	static void PlaySoundNotRanged(Scriptable* Sender, Holder<Action> parameters);
	static void PlaySoundPoint(Scriptable* Sender, Holder<Action> parameters);
	static void Plunder(Scriptable* Sender, Holder<Action> parameters);
	static void Polymorph(Scriptable* Sender, Holder<Action> parameters);
	static void PolymorphCopy(Scriptable* Sender, Holder<Action> parameters);
	static void PolymorphCopyBase(Scriptable* Sender, Holder<Action> parameters);
	static void ProtectObject(Scriptable* Sender, Holder<Action> parameters);
	static void ProtectPoint(Scriptable* Sender, Holder<Action> parameters);
	static void QuitGame(Scriptable* Sender, Holder<Action> parameters);
	static void RandomFly(Scriptable* Sender, Holder<Action> parameters);
	static void RandomRun(Scriptable* Sender, Holder<Action> parameters);
	static void RandomTurn(Scriptable* Sender, Holder<Action> parameters);
	static void RandomWalk(Scriptable* Sender, Holder<Action> parameters);
	static void RandomWalkContinuous(Scriptable* Sender, Holder<Action> parameters);
	static void RealSetGlobalTimer(Scriptable* Sender, Holder<Action> parameters);
	static void ReallyForceSpell(Scriptable* Sender, Holder<Action> parameters);
	static void ReallyForceSpellDead(Scriptable* Sender, Holder<Action> parameters);
	static void ReallyForceSpellPoint(Scriptable* Sender, Holder<Action> parameters);
	static void Recoil(Scriptable* Sender, Holder<Action> parameters);
	static void RegainPaladinHood(Scriptable* Sender, Holder<Action> parameters);
	static void RegainRangerHood(Scriptable* Sender, Holder<Action> parameters);
	static void RemoveAreaFlag(Scriptable* Sender, Holder<Action> parameters);
	static void RemoveAreaType(Scriptable* Sender, Holder<Action> parameters);
	static void RemoveFamiliar(Scriptable* Sender, Holder<Action> parameters);
	static void RemoveJournalEntry(Scriptable* Sender, Holder<Action> parameters);
	static void RemoveMapnote(Scriptable* Sender, Holder<Action> parameters);
	static void RemovePaladinHood(Scriptable* Sender, Holder<Action> parameters);
	static void RemoveRangerHood(Scriptable* Sender, Holder<Action> parameters);
	static void RemoveSpell(Scriptable* Sender, Holder<Action> parameters);
	static void RemoveStoreItem(Scriptable* /*Sender*/, Holder<Action> parameters);
	static void RemoveTraps(Scriptable* Sender, Holder<Action> parameters);
	static void RemoveWorldmapAreaFlag(Scriptable* /*Sender*/, Holder<Action> parameters);
	static void ReputationInc(Scriptable* Sender, Holder<Action> parameters);
	static void ReputationSet(Scriptable* Sender, Holder<Action> parameters);
	static void ResetMorale(Scriptable* Sender, Holder<Action> parameters);
	static void ResetPlayerAI(Scriptable* Sender, Holder<Action> parameters);
	static void RestorePartyLocation(Scriptable* Sender, Holder<Action> parameters);
	static void Rest(Scriptable* Sender, Holder<Action> parameters);
	static void RestNoSpells(Scriptable* Sender, Holder<Action> parameters);
	static void RestoreZoomLevel(Scriptable* Sender, Holder<Action> parameters);
	static void RestParty(Scriptable* Sender, Holder<Action> parameters);
	static void RestPartyEx(Scriptable* Sender, Holder<Action> parameters);
	static void RestUntilHealed(Scriptable* Sender, Holder<Action> parameters);
	static void ReturnToSavedLocation(Scriptable* Sender, Holder<Action> parameters);
	static void ReturnToSavedLocationDelete(Scriptable* Sender, Holder<Action> parameters);
	static void ReturnToStartLocation(Scriptable* Sender, Holder<Action> parameters);
	static void RevealAreaOnMap(Scriptable* Sender, Holder<Action> parameters);
	static void RunAwayFrom(Scriptable* Sender, Holder<Action> parameters);
	static void RunAwayFromNoInterrupt(Scriptable* Sender, Holder<Action> parameters);
	static void RunAwayFromNoInterruptNoLeaveArea(Scriptable* Sender, Holder<Action> parameters);
	static void RunAwayFromNoLeaveArea(Scriptable* Sender, Holder<Action> parameters);
	static void RunFollow(Scriptable* Sender, Holder<Action> parameters);
	static void RunningAttack(Scriptable* Sender, Holder<Action> parameters);
	static void RunningAttackNoSound(Scriptable* Sender, Holder<Action> parameters);
	static void RunToObject(Scriptable* Sender, Holder<Action> parameters);
	static void RunToPoint(Scriptable* Sender, Holder<Action> parameters);
	static void RunToPointNoRecticle(Scriptable* Sender, Holder<Action> parameters);
	static void RunToSavedLocation(Scriptable* Sender, Holder<Action> parameters);
	static void SaveGame(Scriptable* Sender, Holder<Action> parameters);
	static void SaveLocation(Scriptable* Sender, Holder<Action> parameters);
	static void SaveObjectLocation(Scriptable* Sender, Holder<Action> parameters);
	static void ScreenShake(Scriptable* Sender, Holder<Action> parameters);
	static void SelectWeaponAbility(Scriptable* Sender, Holder<Action> parameters);
	static void SendTrigger(Scriptable* Sender, Holder<Action> parameters);
	static void SetApparentName(Scriptable* Sender, Holder<Action> parameters);
	static void SetAreaFlags(Scriptable* Sender, Holder<Action> parameters);
	static void SetAreaRestFlag(Scriptable* Sender, Holder<Action> parameters);
	static void SetAreaScript(Scriptable* Sender, Holder<Action> parameters);
	static void SetArmourLevel(Scriptable* Sender, Holder<Action> parameters);
	static void SetBeenInPartyFlags(Scriptable* Sender, Holder<Action> parameters);
	static void SetBestWeapon(Scriptable* Sender, Holder<Action> parameters);
	static void SetCursorState(Scriptable* Sender, Holder<Action> parameters);
	static void SetCreatureAreaFlag(Scriptable* Sender, Holder<Action> parameters);
	static void SetCriticalPathObject(Scriptable* Sender, Holder<Action> parameters);
	static void SetCutSceneBreakable(Scriptable* /*Sender*/, Holder<Action> parameters);
	static void SetDialogue(Scriptable* Sender, Holder<Action> parameters);
	static void SetDialogueRange(Scriptable* Sender, Holder<Action> parameters);
	static void SetDoorFlag(Scriptable* Sender, Holder<Action> parameters);
	static void SetDoorLocked(Scriptable* Sender, Holder<Action> parameters);
	static void SetEncounterProbability(Scriptable* Sender, Holder<Action> parameters);
	static void SetExtendedNight(Scriptable* Sender, Holder<Action> parameters);
	static void SetFaction(Scriptable* Sender, Holder<Action> parameters);
	static void SetPCStatsTokens(Scriptable* Sender, Holder<Action> parameters);
	static void SetGabber(Scriptable* Sender, Holder<Action> parameters);
	static void SetGlobal(Scriptable* Sender, Holder<Action> parameters);
	static void SetGlobalRandom(Scriptable* Sender, Holder<Action> parameters);
	static void SetGlobalTimer(Scriptable* Sender, Holder<Action> parameters);
	static void SetGlobalTimerOnce(Scriptable* Sender, Holder<Action> parameters);
	static void SetGlobalTimerRandom(Scriptable* Sender, Holder<Action> parameters);
	static void SetGlobalTint(Scriptable* Sender, Holder<Action> parameters);
	static void SetHomeLocation(Scriptable* Sender, Holder<Action> parameters);
	static void SetHP(Scriptable* Sender, Holder<Action> parameters);
	static void SetHPPercent(Scriptable* Sender, Holder<Action> parameters);
	static void SetInternal(Scriptable* Sender, Holder<Action> parameters);
	static void SetInterrupt(Scriptable* Sender, Holder<Action> parameters);
	static void SetItemFlags(Scriptable* Sender, Holder<Action> parameters);
	static void SetLeavePartyDialogFile(Scriptable* Sender, Holder<Action> parameters);
	static void SetMarkedSpell(Scriptable* Sender, Holder<Action> parameters);
	static void SetMasterArea(Scriptable* Sender, Holder<Action> parameters);
	static void SetMazeEasier(Scriptable* Sender, Holder<Action> parameters);
	static void SetMazeHarder(Scriptable* Sender, Holder<Action> parameters);
	static void SetMoraleAI(Scriptable* Sender, Holder<Action> parameters);
	static void SetMusic(Scriptable* Sender, Holder<Action> parameters);
	static void SetMyTarget(Scriptable* Sender, Holder<Action> parameters);
	static void SetNamelessClass(Scriptable* Sender, Holder<Action> parameters);
	static void SetNamelessDeath(Scriptable* Sender, Holder<Action> parameters);
	static void SetNamelessDeathParty(Scriptable* Sender, Holder<Action> parameters);
	static void SetNamelessDisguise(Scriptable* Sender, Holder<Action> parameters);
	static void SetNoOneOnTrigger(Scriptable* Sender, Holder<Action> parameters);
	static void SetNumTimesTalkedTo(Scriptable* Sender, Holder<Action> parameters);
	static void SetOriginalClass(Scriptable* Sender, Holder<Action> parameters);
	static void SetPlayerSound(Scriptable* Sender, Holder<Action> parameters);
	static void SetQuestDone(Scriptable* Sender, Holder<Action> parameters);
	static void SetRegularName(Scriptable* Sender, Holder<Action> parameters);
	static void SetRestEncounterChance(Scriptable* Sender, Holder<Action> parameters);
	static void SetRestEncounterProbabilityDay(Scriptable* Sender, Holder<Action> parameters);
	static void SetRestEncounterProbabilityNight(Scriptable* Sender, Holder<Action> parameters);
	static void SetSavedLocation(Scriptable* Sender, Holder<Action> parameters);
	static void SetSavedLocationPoint(Scriptable* Sender, Holder<Action> parameters);
	static void SetScriptName(Scriptable* Sender, Holder<Action> parameters);
	static void SetSelection(Scriptable* Sender, Holder<Action> parameters);
	static void SetStartPos(Scriptable* Sender, Holder<Action> parameters);
	static void SetTeam(Scriptable* Sender, Holder<Action> parameters);
	static void SetTeamBit(Scriptable* Sender, Holder<Action> parameters);
	static void SetTextColor(Scriptable* Sender, Holder<Action> parameters);
	static void SetToken(Scriptable* Sender, Holder<Action> parameters);
	static void SetToken2DA(Scriptable* Sender, Holder<Action> parameters);
	static void SetTokenGlobal(Scriptable* Sender, Holder<Action> parameters);
	static void SetTokenObject(Scriptable* Sender, Holder<Action> parameters);
	static void SetTrackString(Scriptable* Sender, Holder<Action> parameters);
	static void SetupWish(Scriptable* Sender, Holder<Action> parameters);
	static void SetupWishObject(Scriptable* Sender, Holder<Action> parameters);
	static void SetVisualRange(Scriptable* Sender, Holder<Action> parameters);
	static void SetWorldmap(Scriptable* /*Sender*/, Holder<Action> parameters);
	static void SetZoomViewport(Scriptable* Sender, Holder<Action> parameters);
	static void SG(Scriptable* Sender, Holder<Action> parameters);
	static void Shout(Scriptable* Sender, Holder<Action> parameters);
	static void SmallWait(Scriptable* Sender, Holder<Action> parameters);
	static void SmallWaitRandom(Scriptable* Sender, Holder<Action> parameters);
	static void SoundActivate(Scriptable* Sender, Holder<Action> parameters);
	static void SpawnPtActivate(Scriptable* Sender, Holder<Action> parameters);
	static void SpawnPtDeactivate(Scriptable* Sender, Holder<Action> parameters);
	static void SpawnPtSpawn(Scriptable* Sender, Holder<Action> parameters);
	static void Spell(Scriptable* Sender, Holder<Action> parameters);
	static void SpellCastEffect(Scriptable* Sender, Holder<Action> parameters);
	static void SpellHitEffectPoint(Scriptable* Sender, Holder<Action> parameters);
	static void SpellHitEffectSprite(Scriptable* Sender, Holder<Action> parameters);
	static void SpellNoDec(Scriptable* Sender, Holder<Action> parameters);
	static void SpellPoint(Scriptable* Sender, Holder<Action> parameters);
	static void SpellPointNoDec(Scriptable* Sender, Holder<Action> parameters);
	static void StartCombatCounter(Scriptable* Sender, Holder<Action> parameters);
	static void StartCutScene(Scriptable* Sender, Holder<Action> parameters);
	static void StartCutSceneEx(Scriptable* Sender, Holder<Action> parameters);
	static void StartCutSceneMode(Scriptable* Sender, Holder<Action> parameters);
	static void StartDialogue(Scriptable* Sender, Holder<Action> parameters);
	static void StartDialogueInterrupt(Scriptable* Sender, Holder<Action> parameters);
	static void StartDialogueNoSet(Scriptable* Sender, Holder<Action> parameters);
	static void StartDialogueNoSetInterrupt(Scriptable* Sender,
						Holder<Action> parameters);
	static void StartDialogueOverride(Scriptable* Sender, Holder<Action> parameters);
	static void StartDialogueOverrideInterrupt(Scriptable* Sender,
						   Holder<Action> parameters);
	static void StartMovie(Scriptable* Sender, Holder<Action> parameters);
	static void StartMusic(Scriptable* Sender, Holder<Action> parameters);
	static void StartRainNow(Scriptable* Sender, Holder<Action> parameters);
	static void StartRandomTimer(Scriptable* Sender, Holder<Action> parameters);
	static void StartSong(Scriptable* Sender, Holder<Action> parameters);
	static void StartStore(Scriptable* Sender, Holder<Action> parameters);
	static void StartTimer(Scriptable* Sender, Holder<Action> parameters);
	static void StateOverrideFlag(Scriptable* Sender, Holder<Action> parameters);
	static void StateOverrideTime(Scriptable* Sender, Holder<Action> parameters);
	static void StaticPalette(Scriptable* Sender, Holder<Action> parameters);
	static void StaticStart(Scriptable* Sender, Holder<Action> parameters);
	static void StaticStop(Scriptable* Sender, Holder<Action> parameters);
	static void StopMoving(Scriptable* Sender, Holder<Action> parameters);
	static void StorePartyLocation(Scriptable* Sender, Holder<Action> parameters);
	static void StoreZoomLevel(Scriptable* Sender, Holder<Action> parameters);
	static void Swing(Scriptable* Sender, Holder<Action> parameters);
	static void SwingOnce(Scriptable* Sender, Holder<Action> parameters);
	static void TakeCreatureItems(Scriptable* Sender, Holder<Action> parameters);
	static void TakeItemList(Scriptable* Sender, Holder<Action> parameters);
	static void TakeItemListParty(Scriptable* Sender, Holder<Action> parameters);
	static void TakeItemListPartyNum(Scriptable* Sender, Holder<Action> parameters);
	static void TakeItemReplace(Scriptable* Sender, Holder<Action> parameters);
	static void TakeObjectGoldGlobal(Scriptable* Sender, Holder<Action> parameters);
	static void TakePartyGold(Scriptable* Sender, Holder<Action> parameters);
	static void TakePartyItem(Scriptable* Sender, Holder<Action> parameters);
	static void TakePartyItemAll(Scriptable* Sender, Holder<Action> parameters);
	static void TakePartyItemNum(Scriptable* Sender, Holder<Action> parameters);
	static void TakePartyItemRange(Scriptable* Sender, Holder<Action> parameters);
	static void TeleportParty(Scriptable* Sender, Holder<Action> parameters);
	static void TextScreen(Scriptable* Sender, Holder<Action> parameters);
	static void ToggleDoor(Scriptable* Sender, Holder<Action> parameters);
	static void TimedMoveToPoint(Scriptable* Sender, Holder<Action> parameters);
	static void TransformItem(Scriptable* Sender, Holder<Action> parameters);
	static void TransformItemAll(Scriptable* Sender, Holder<Action> parameters);
	static void TransformPartyItem(Scriptable* Sender, Holder<Action> parameters);
	static void TransformPartyItemAll(Scriptable* Sender, Holder<Action> parameters);
	static void TriggerActivation(Scriptable* Sender, Holder<Action> parameters);
	static void TriggerWalkTo(Scriptable* Sender, Holder<Action> parameters);
	static void Turn(Scriptable* Sender, Holder<Action> parameters);
	static void TurnAMT(Scriptable* Sender, Holder<Action> parameters);
	static void UndoExplore(Scriptable* Sender, Holder<Action> parameters);
	static void Unhide(Scriptable* Sender, Holder<Action> parameters);
	static void UnhideGUI(Scriptable* Sender, Holder<Action> parameters);
	static void Unlock(Scriptable* Sender, Holder<Action> parameters);
	static void UnlockScroll(Scriptable* Sender, Holder<Action> parameters);
	static void UseContainer(Scriptable* Sender, Holder<Action> parameters);
	static void UseDoor(Scriptable* Sender, Holder<Action> parameters);
	static void UseItem(Scriptable* Sender, Holder<Action> parameters);
	static void UseItemPoint(Scriptable* Sender, Holder<Action> parameters);
	static void VerbalConstant(Scriptable* Sender, Holder<Action> parameters);
	static void VerbalConstantHead(Scriptable* Sender, Holder<Action> parameters);
	static void Wait(Scriptable* Sender, Holder<Action> parameters);
	static void WaitAnimation(Scriptable* Sender, Holder<Action> parameters);
	static void WaitRandom(Scriptable* Sender, Holder<Action> parameters);
	static void Weather(Scriptable* Sender, Holder<Action> parameters);
	static void XEquipItem(Scriptable* Sender, Holder<Action> parameters);
	static void ZoomLock(Scriptable* Sender, Holder<Action> parameters);

	//Objects
	static Targets* BestAC(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* EighthFarthestEnemyOf(const Scriptable* Sender, Targets* parameters, int gaFlags);
	static Targets* EighthNearest(const Scriptable* Sender, Targets* parameters, int ga_flagss);
	static Targets* EighthNearestAllyOf(const Scriptable* Sender, Targets* parameters, int gaFlags);
	static Targets* EighthNearestDoor(const Scriptable* Sender, Targets* parameters, int ga_flagss);
	static Targets* EighthNearestEnemyOf(const Scriptable* Sender, Targets* parameters, int ga_flagss);
	static Targets* EighthNearestEnemyOfType(const Scriptable* Sender, Targets* parameters, int ga_flagss);
	static Targets* EighthNearestMyGroupOfType(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* Familiar(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* FamiliarSummoner(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* Farthest(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* FarthestEnemyOf(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* FifthFarthestEnemyOf(const Scriptable* Sender, Targets* parameters, int gaFlags);
	static Targets* FifthNearest(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* FifthNearestAllyOf(const Scriptable* Sender, Targets* parameters, int gaFlags);
	static Targets* FifthNearestDoor(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* FifthNearestEnemyOf(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* FifthNearestEnemyOfType(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* FifthNearestMyGroupOfType(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* FourthFarthestEnemyOf(const Scriptable* Sender, Targets* parameters, int gaFlags);
	static Targets* FourthNearest(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* FourthNearestAllyOf(const Scriptable* Sender, Targets* parameters, int gaFlags);
	static Targets* FourthNearestDoor(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* FourthNearestEnemyOf(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* FourthNearestEnemyOfType(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* FourthNearestMyGroupOfType(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* Gabber(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* GroupOf(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* LastAttackerOf(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* LastCommandedBy(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* LastHeardBy(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* LastHelp(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* LastHitter(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* LastKilled(const Scriptable* Sender, Targets* parameters, int gaFlags);
	static Targets* LastMarkedObject(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* LastSeenBy(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* LastSummonerOf(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* LastTalkedToBy(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* LastTargetedBy(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* LastTrigger(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* LeaderOf(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* LeastDamagedOf(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* MostDamagedOf(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* Myself(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* MyTarget(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* Nearest(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* NearestAllyOf(const Scriptable* Sender, Targets* parameters, int gaFlags);
	static Targets* NearestDoor(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* NearestEnemyOf(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* NearestEnemyOfType(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* NearestEnemySummoned(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* NearestMyGroupOfType(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* NearestPC(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* NinthFarthestEnemyOf(const Scriptable* Sender, Targets* parameters, int gaFlags);
	static Targets* NinthNearest(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* NinthNearestAllyOf(const Scriptable* Sender, Targets* parameters, int gaFlags);
	static Targets* NinthNearestDoor(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* NinthNearestEnemyOf(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* NinthNearestEnemyOfType(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* NinthNearestMyGroupOfType(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* Nothing(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* PartySlot1(const Scriptable* Sender, Targets* parameters, int gaFlags);
	static Targets* PartySlot2(const Scriptable* Sender, Targets* parameters, int gaFlags);
	static Targets* PartySlot3(const Scriptable* Sender, Targets* parameters, int gaFlags);
	static Targets* PartySlot4(const Scriptable* Sender, Targets* parameters, int gaFlags);
	static Targets* PartySlot5(const Scriptable* Sender, Targets* parameters, int gaFlags);
	static Targets* PartySlot6(const Scriptable* Sender, Targets* parameters, int gaFlags);
	static Targets* Player1(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* Player1Fill(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* Player2(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* Player2Fill(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* Player3(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* Player3Fill(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* Player4(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* Player4Fill(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* Player5(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* Player5Fill(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* Player6(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* Player6Fill(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* Protagonist(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* ProtectedBy(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* ProtectorOf(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* SecondFarthestEnemyOf(const Scriptable* Sender, Targets* parameters, int gaFlags);
	static Targets* SecondNearest(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* SecondNearestAllyOf(const Scriptable* Sender, Targets* parameters, int gaFlags);
	static Targets* SecondNearestDoor(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* SecondNearestEnemyOf(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* SecondNearestEnemyOfType(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* SecondNearestMyGroupOfType(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* SelectedCharacter(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* SeventhFarthestEnemyOf(const Scriptable* Sender, Targets* parameters, int gaFlags);
	static Targets* SeventhNearest(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* SeventhNearestAllyOf(const Scriptable* Sender, Targets* parameters, int gaFlags);
	static Targets* SeventhNearestDoor(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* SeventhNearestEnemyOf(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* SeventhNearestEnemyOfType(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* SeventhNearestMyGroupOfType(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* SixthFarthestEnemyOf(const Scriptable* Sender, Targets* parameters, int gaFlags);
	static Targets* SixthNearest(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* SixthNearestAllyOf(const Scriptable* Sender, Targets* parameters, int gaFlags);
	static Targets* SixthNearestDoor(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* SixthNearestEnemyOf(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* SixthNearestEnemyOfType(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* SixthNearestMyGroupOfType(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* SpellTarget(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* StrongestOf(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* StrongestOfMale(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* TenthFarthestEnemyOf(const Scriptable* Sender, Targets* parameters, int gaFlags);
	static Targets* TenthNearest(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* TenthNearestAllyOf(const Scriptable* Sender, Targets* parameters, int gaFlags);
	static Targets* TenthNearestDoor(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* TenthNearestEnemyOf(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* TenthNearestEnemyOfType(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* TenthNearestMyGroupOfType(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* ThirdFarthestEnemyOf(const Scriptable* Sender, Targets* parameters, int gaFlags);
	static Targets* ThirdNearest(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* ThirdNearestAllyOf(const Scriptable* Sender, Targets* parameters, int gaFlags);
	static Targets* ThirdNearestDoor(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* ThirdNearestEnemyOf(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* ThirdNearestEnemyOfType(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* ThirdNearestMyGroupOfType(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* WeakestOf(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* WorstAC(const Scriptable* Sender, Targets* parameters, int ga_flags);

	/*GemRB extensions/actions*/
	static void RunAwayFromPoint(Scriptable* Sender, Holder<Action> parameters);
	static void UnMakeGlobal(Scriptable* Sender, Holder<Action> parameters);
	static void UnloadArea(Scriptable* Sender, Holder<Action> parameters);

	/*GemRB extensions/objects*/
	static Targets* Player7(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* Player7Fill(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* PartySlot7(const Scriptable* Sender, Targets* parameters, int gaFlags);
	static Targets* Player8(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* Player8Fill(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* PartySlot8(const Scriptable* Sender, Targets* parameters, int gaFlags);
	static Targets* Player9(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* Player9Fill(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* PartySlot9(const Scriptable* Sender, Targets* parameters, int gaFlags);
	static Targets* Player10(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* Player10Fill(const Scriptable* Sender, Targets* parameters, int ga_flags);
	static Targets* PartySlot10(const Scriptable* Sender, Targets* parameters, int gaFlags);
};

GEM_EXPORT Holder<Action> GenerateAction(std::string String);
GEM_EXPORT Holder<Action> GenerateActionDirect(std::string string, const Scriptable* object);
GEM_EXPORT Trigger* GenerateTrigger(std::string string);

void InitializeIEScript();

}

#endif
