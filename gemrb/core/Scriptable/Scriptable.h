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

#ifndef SCRIPTABLE_H
#define SCRIPTABLE_H

#include "exports.h"
#include "ie_cursors.h"
#include "ie_types.h"

#include "CharAnimations.h"
#include "OverHeadText.h"

#include <list>
#include <map>
#include <memory>

namespace GemRB {

class Action;
class Actor;
class Container;
class Door;
class GameScript;
class Gem_Polygon;
class Highlightable;
class InfoPoint;
class Map;
class Movable;
class Object;
struct PathListNode;
class Projectile;
class Scriptable;
class Selectable;
class Spell;
class Sprite2D;

#define MAX_GROUND_ICON_DRAWN   3

/** The distance between PC's who are about to enter a new area */
#define MAX_TRAVELING_DISTANCE      400

// script levels / slots (scrlev.ids)
#define SCR_OVERRIDE  0
#define SCR_AREA      1 // iwd2: special 1
#define SCR_SPECIFICS 2 // iwd2: team
#define SCR_RESERVED  3 // iwd2: pc-customizable scripts were saved here
#define SCR_CLASS     4 // iwd2: special 2
#define SCR_RACE      5 // iwd2: combat
#define SCR_GENERAL   6 // iwd2: special 3
#define SCR_DEFAULT   7 // iwd2: movement
#define MAX_SCRIPTS   8

//pst trap flags (portal)
#define PORTAL_CURSOR 1
#define PORTAL_TRAVEL 2

//trigger flags
#define TRAP_INVISIBLE  1
#define TRAP_RESET      2
#define TRAVEL_PARTY    4
#define TRAP_DETECTABLE 8
//#define TRAP_ENEMY	 16 // "trap set off by enemy" in NI, unused
#define TRAP_TUTORIAL	 32 //active only when in tutorial mode
#define TRAP_NPC	64 // "trap set off by NPC"
#define TRAP_SILENT	128 // "trigger silent" / "no string", used in pst
#define TRAP_DEACTIVATED  256
#define _TRAVEL_NONPC      512
#define _TRAP_USEPOINT       1024 //override usage point of travel regions (used for sound in PST traps)
#define INFO_DOOR	 2048 //info trigger blocked by door

//internal actor flags
#define IF_GIVEXP     1     //give xp for this death
#define IF_JUSTDIED   2     //Died() will return true
#define IF_FROMGAME   4     //this is an NPC or PC
#define IF_REALLYDIED 8     //real death happened, actor will be set to dead
#define IF_NORETICLE  16    //draw reticle (target mark)
#define IF_NOINT      32    //cannot interrupt the actions of this actor (save is not possible!)
#define IF_CLEANUP    64    //actor died chunky death, or other total destruction
#define IF_RUNNING    128   //actor is running
//these bits could be set by a WalkTo
#define IF_RUNFLAGS   (IF_RUNNING|IF_NORETICLE|IF_NOINT)
//#define IF_BECAMEVISIBLE 0x100//actor just became visible (trigger event)
#define IF_INITIALIZED   0x200
#define IF_USEDSAVE      0x400  //actor needed saving throws
#define IF_GOTAREA    0x800     //actor already moved to an area
#define IF_USEEXIT       0x1000 //
#define IF_INTRAP        0x2000 //actor is currently in a trap (intrap trigger event)
//#define IF_WASINDIALOG   0x4000 //actor just left dialog
#define IF_PST_WMAPPING  0x8000 // trying to use the worldmap for travel

//scriptable flags
#define IF_ACTIVE        0x10000
#define IF_VISIBLE       0x40000
//#define IF_ONCREATION    0x80000
#define IF_IDLE          0x100000
//#define IF_PARTYRESTED   0x200000 //party rested trigger event
#define IF_FORCEUPDATE   0x400000
#define IF_TRIGGER_AP    0x800000
#define IF_DUMPED        0x1000000

//the actor should stop attacking
#define IF_STOPATTACK (IF_JUSTDIED|IF_REALLYDIED|IF_CLEANUP|IF_IDLE)

//CheckTravel return value
#define CT_CANTMOVE       0 //inactive
#define CT_ACTIVE         1 //actor can move
#define CT_GO_CLOSER      2 //entire team would move, but not close enough
#define CT_WHOLE          3 //team can move
#define CT_SELECTED       4 //not all selected actors are there
#define CT_MOVE_SELECTED  5 //all selected can move

//xp bonus types (for xpbonus.2da)
#define XP_LOCKPICK   0
#define XP_DISARM     1
#define XP_LEARNSPELL 2
#define XP_PICKPOCKET  3 // gemrb extension

#define MAX_PATH_TRIES 8
#define MAX_BUMP_BACK_TRIES 16
#define MAX_RAND_WALK 10

using ScriptableType = enum ScriptableType { ST_ACTOR = 0, ST_PROXIMITY = 1, ST_TRIGGER = 2,
	ST_TRAVEL = 3, ST_DOOR = 4, ST_CONTAINER = 5, ST_AREA = 6, ST_GLOBAL = 7, ST_ANY = 8 };

enum {
	trigger_acquired = 0x1, // unused and broken in the original
	trigger_attackedby = 0x2,
	trigger_help = 0x3,
	trigger_joins = 0x4,
	trigger_leaves = 0x5,
	trigger_receivedorder = 0x6,
	trigger_said = 0x7, // unused in the originals; if added, don't forget to set LastTrigger
	trigger_turnedby = 0x8,
	trigger_unusable = 0x9,
	trigger_alignment = 0xa,
	trigger_allegiance = 0xb,
	trigger_class = 0xc,
	trigger_exists = 0xd,
	trigger_general = 0xe,
	trigger_hpgt = 0x11,
	trigger_morale = 0x14,
	trigger_race = 0x17,
	trigger_range = 0x18,
	trigger_reputation = 0x19,
	trigger_specifics = 0x1d,
	trigger_hitby = 0x20,
	trigger_hotkey = 0x21,
	trigger_timerexpired = 0x22, // handled internally through TimerExpired
	trigger_trigger = 0x24,
	trigger_die = 0x25,
	trigger_targetunreachable = 0x26,
	trigger_heard = 0x2f,
	trigger_becamevisible = 0x33,
	trigger_oncreation = 0x36,
	trigger_statecheck = 0x37,
	trigger_reaction = 0x3c,
	trigger_inparty = 0x43,
	trigger_checkstat = 0x44,
	trigger_died = 0x4a,
	trigger_killed = 0x4b,
	trigger_entered = 0x4c,
	trigger_gender = 0x4c,
	trigger_opened = 0x52,
	trigger_closed = 0x53,
	trigger_detected = 0x54,
	trigger_reset = 0x55,
	trigger_disarmed = 0x56,
	trigger_unlocked = 0x57,
	trigger_breakingpoint = 0x5c,
	trigger_pickpocketfailed = 0x5d,
	trigger_stealfailed = 0x5e,
	trigger_disarmfailed  = 0x5f,
	trigger_picklockfailed = 0x60,
	trigger_clicked = 0x70,
	trigger_triggerclick = 0x79, // pst; we map it to trigger_clicked
	trigger_traptriggered = 0x87, // bg2
	trigger_partymemberdied = 0x88, // bg2
	trigger_spellcast = 0x91, // bg2
	trigger_partyrested = 0x93, // bg2
	trigger_vacant = 0x94, // pst
	trigger_summoned = 0x97, // bg2
	trigger_harmlessopened = 0x9d, // pst
	trigger_harmlessclosed = 0x9e, // pst
	trigger_harmlessentered = 0x9f, // pst
	trigger_spellcastonme = 0xa1, // bg2
	trigger_nulldialog = 0xa4, // pst, checked directly in NullDialog
	trigger_wasindialog = 0xa5, // pst
	trigger_spellcastpriest = 0xa6, // bg2
	trigger_spellcastinnate = 0xa7, // bg2
	trigger_namelessbitthedust = 0xab, // pst
	trigger_failedtoopen = 0xaf, // pst
	trigger_tookdamage = 0xcc, // bg2
	trigger_walkedtotrigger = 0xd6, // bg2
	trigger_secreddoordetected = 0x100 // ees
};

// flags for TriggerEntry
enum {
	// has the effect queue (if any) been processed since this trigger
	// was added? (for fx_cast_spell_on_condition)
	TEF_PROCESSED_EFFECTS = 1
};

struct TriggerEntry {
	explicit TriggerEntry(unsigned short id) : triggerID(id) { }
	TriggerEntry(unsigned short id, ieDword p1) : triggerID(id), param1(p1) { }
	TriggerEntry(unsigned short id, ieDword p1, ieDword p2) : triggerID(id), param1(p1), param2(p2) { }

	unsigned short triggerID;
	ieDword param1 = 0;
	ieDword param2 = 0;
	unsigned int flags = 0;
};

// Stored objects.
struct StoredObjects {
	ieDword LastAttacker = 0;
	ieDword LastCommander = 0;
	ieDword LastProtector = 0;
	ieDword LastProtectee = 0;
	// LastTargetedBy we always compute
	ieDword LastHitter = 0;
	ieDword LastHelp = 0;
	ieDword LastTrigger = 0;
	ieDword LastSeen = 0;
	ieDword LastTalker = 0;
	ieDword LastHeard = 0;
	ieDword LastSummoner = 0;
	ieDword LastFollowed = 0; // gemrb extension (LeaderOf)
	ieDword LastMarked = 0; // iwd2
	ieDword MyTarget = 0; // iwd2, has nothing to do with LastTarget
	ieDword LastKilled = 0; // ees
	int LastMarkedSpell = 0; // iwd2

	ieDword LastSpellTarget = 0;
	Point LastTargetPos;
	ieDword LastTarget = 0;
	// gemrb extension, persists across actions; remove if LastTarget ever gets the same persistence
	ieDword LastTargetPersistent = 0;

	// this is used by GUIScript :(
	ieDword LastSpellOnMe = 0xffffffff; // last spell cast on this scriptable
};

class GEM_EXPORT Scriptable {
public:
	explicit Scriptable(ScriptableType type);
	Scriptable(const Scriptable&) = delete;
	virtual ~Scriptable();
	Scriptable& operator=(const Scriptable&) = delete;
private:
	tick_t WaitCounter = 0;
	std::map<ieDword, ieDword> scriptTimers;
	ieDword globalID = 0;
protected: //let Actor access this
	std::list<TriggerEntry> triggers;
	Map *area = nullptr;
	ieVariable scriptName;
	ieDword InternalFlags = 0; // for triggers
	ResRef Dialog;
	std::list< Action*> actionQueue;
	Action* CurrentAction = nullptr;
public:
	ScriptableType Type = ST_ACTOR;
	Point Pos;
	Region BBox;
	ieStrRef DialogName = ieStrRef::INVALID;

	// State relating to the currently-running action.
	int CurrentActionState = 0;
	ieDword CurrentActionTarget = 0;
	bool CurrentActionInterruptible = true;
	ieDword CurrentActionTicks = 0;

	std::array<GameScript*, MAX_SCRIPTS> Scripts {};
	int scriptLevel = 0; // currently running script slot

	// The number of times this was updated.
	ieDword Ticks = 0;
	// The same, after adjustment for being slowed/hasted.
	ieDword AdjustedTicks = 0;
	// The number of times TickScripting() was run.
	ieDword ScriptTicks = 0;
	// The number of times since TickScripting() tried to do anything.
	ieDword IdleTicks = 0;
	// The number of ticks since the last spellcast
	ieDword AuraCooldown = 0;
	// The countdown for forced activation by triggers.
	ieDword TriggerCountdown = 0;

	// more scripting state
	ieVarsMap locals;
	OverHeadText overHead{this};
	StoredObjects objects {};
	ieDword UnselectableTimer = 0;
	ieDword UnselectableType = 0;
	unsigned char weightsAsCases = 0;

	// spellcasting state
	int SpellHeader = 0;
	ResRef SpellResRef;
	bool InterruptCasting = false;
public:
	
	template <class RETURN, class PARAM>
	static constexpr auto As(PARAM* obj)
	-> typename std::conditional_t<std::is_const<PARAM>::value, const RETURN*, RETURN*> {
		static_assert(std::is_base_of<Scriptable, RETURN>::value, "Attempted bad Scriptable cast!");
		// dynamic_cast will return nullptr if the cast is invalid
		return dynamic_cast<decltype(As<RETURN, PARAM>(obj))>(obj);
	}

	template <class RETURN>
	RETURN* As() {
		return Scriptable::As<RETURN>(this);
	}
	
	template <class RETURN>
	constexpr RETURN* As() const {
		return Scriptable::As<RETURN>(this);
	}

	/** Gets the Dialog ResRef */
	ResRef GetDialog() const
	{
		return Dialog;
	}
	void SetDialog(const ResRef &resref);
	void SetFloatingText(char*);
	void SetScript(const ResRef &aScript, int idx, bool ai = false);
	void SetSpellResRef(const ResRef& resref);
	void SetWait(tick_t time);
	tick_t GetWait() const;
	void LeftDialog();
	void Interrupt();
	void NoInterrupt();
	void Hide();
	void Unhide();
	void Activate();
	void Deactivate();
	void PartyRested();
	ieDword GetInternalFlag() const;
	void SetInternalFlag(unsigned int value, BitOp mode);
	const ieVariable& GetScriptName() const;
	Map* GetCurrentArea() const;
	void SetMap(Map *map);
	void SetScriptName(const ieVariable& text);
	//call this to enable script running as soon as possible
	void ImmediateEvent();
	bool IsPC() const;
	virtual void Update();
	void TickScripting();
	virtual void ExecuteScript(int scriptCount);
	void AddAction(std::string actStr);
	void AddAction(Action* aC);
	void AddActionInFront(Action* aC);
	Action* GetCurrentAction() const { return CurrentAction; }
	Action* GetNextAction() const;
	Action* PopNextAction();
	void ClearActions(int skipFlags = 0);
	virtual void Stop(int flags = 0);
	virtual void ReleaseCurrentAction();
	bool InMove() const;
	void ProcessActions();
	//these functions handle clearing of triggers that resulted a
	//true condition (whole triggerblock returned true)
	void ClearTriggers();
	void AddTrigger(TriggerEntry trigger);
	void SetLastTrigger(ieDword triggerID, ieDword scriptableID);
	bool MatchTrigger(unsigned short id, ieDword param = 0) const;
	bool MatchTriggerWithObject(short unsigned int id, const Object *obj, ieDword param = 0) const;
	const TriggerEntry *GetMatchingTrigger(unsigned short id, unsigned int notflags = 0) const;
	void SendTriggerToAll(TriggerEntry entry, int extraFlags = 0);
	/* re/draws overhead text on the map screen */
	void DrawOverheadText();
	virtual Region DrawingRegion() const;
	/* check if casting is allowed at all */
	int CanCast(const ResRef& SpellRef, bool verbose = true);
	/* check for and trigger a wild surge */
	int CheckWildSurge();
	void SpellcraftCheck(const Actor *caster, const ResRef& spellRef);
	/* internal spellcasting shortcuts */
	void DirectlyCastSpellPoint(const Point& target, const ResRef& spellRef, int level, bool keepStance, bool deplete);
	void DirectlyCastSpell(Scriptable* target, const ResRef& spellRef, int level, bool keepStance, bool deplete);
	/* actor/scriptable casts spell */
	int CastSpellPoint(const Point& target, bool deplete, bool instant = false, bool noInterrupt = false, int level = 0);
	int CastSpell(Scriptable* target, bool deplete, bool instant = false, bool noInterrupt = false, int level = 0);
	/* spellcasting finished */
	void CastSpellPointEnd(int level, bool keepStance);
	void CastSpellEnd(int level, bool keepStance);
	ieDword GetGlobalID() const { return globalID; }
	/** timer functions (numeric ID, not saved) */
	bool TimerActive(ieDword ID);
	bool TimerExpired(ieDword ID);
	void StartTimer(ieDword ID, ieDword expiration);
	String GetName() const;
	bool AuraPolluted();
	ieDword GetLocal(const ieVariable& key, ieDword fallback) const;
	virtual std::string dump() const = 0;
private:
	/* used internally to handle start of spellcasting */
	int SpellCast(bool instant, Scriptable* target = nullptr, int level = 0);
	/* also part of the spellcasting process, creating the projectile */
	void CreateProjectile(const ResRef& spellResRef, ieDword tgt, int level, bool fake);
	/* do some magic for the weird/awesome wild surges */
	bool HandleHardcodedSurge(const ResRef& surgeSpell, const Spell *spl, Actor *caster);
	void ModifyProjectile(Projectile* &pro, Spell* spl, ieDword tgt, int level);
	void ResetCastingState(Actor* caster);
	void DisplaySpellCastMessage(ieDword tgt, const Spell* spl) const;
};

class GEM_EXPORT Selectable : public Scriptable {
public:
	using Scriptable::Scriptable;
public:
	ieWord Selected = 0; // could be 0x80 for unselectable
	bool Over = false;
	Color selectedColor = ColorBlack;
	Color overColor = ColorBlack;
	Holder<Sprite2D> circleBitmap[2] = {};
	int circleSize = 0;
	float_t sizeFactor = 1.0f;
public:
	void SetBBox(const Region &newBBox);
	void DrawCircle(const Point& p) const;
	bool IsOver(const Point &Pos) const;
	void SetOver(bool over);
	bool IsSelected() const;
	void Select(int Value);
	void SetCircle(int size, float_t, const Color &color, Holder<Sprite2D> normal_circle, Holder<Sprite2D> selected_circle);
	int CircleSize2Radius() const;
};

class GEM_EXPORT Highlightable : public Scriptable {
public:
	using Scriptable::Scriptable;
	virtual int TrapResets() const = 0;
	virtual bool CanDetectTrap() const { return true; }
	virtual bool PossibleToSeeTrap() const;
public:
	std::shared_ptr<Gem_Polygon> outline = nullptr;
	Color outlineColor = ColorBlack;
	ieDword Cursor = IE_CURSOR_NORMAL;
	bool Highlight = false;
	Point TrapLaunch = Point(-1, -1);
	ieWord TrapDetectionDiff = 0;
	ieWord TrapRemovalDiff = 0;
	ieWord Trapped = 0;
	ieWord TrapDetected = 0;
	ResRef KeyResRef;
	//play this wav file when stepping on the trap (on PST)
	ResRef EnterWav;
public:
	bool IsOver(const Point &Place) const;
	void DrawOutline(Point origin) const;
	void SetCursor(unsigned char CursorIndex);

	void SetTrapDetected(int x);
	void TryDisarm(Actor* actor);
	//detect trap, set skill to 256 if you want sure fire
	void DetectTrap(int skill, ieDword actorID);
	//returns true if trap is visible, only_detected must be true
	//if you want to see discovered traps, false is for cheats
	bool VisibleTrap(int only_detected) const;
	//returns true if trap has been triggered, tumble skill???
	virtual bool TriggerTrap(int skill, ieDword ID);
	bool TryUnlock(Actor *actor, bool removekey) const;
};

class GEM_EXPORT Movable : public Selectable {
private: //these seem to be sensitive, so get protection
	unsigned char StanceID = 0;
	orient_t Orientation = S;
	orient_t NewOrientation = S;
	std::array<ieWord, 3> AttackMovements = { 100, 0, 0 };

	PathListNode* path = nullptr; // whole path
	PathListNode* step = nullptr; // actual step
	unsigned int prevTicks = 0;
	int bumpBackTries = 0;
	bool pathAbandoned = false;
protected:
	ieDword timeStartStep = 0;
	//the # of previous tries to pick up a new walkpath
	int pathTries = 0;
	int randomBackoff = 0;
	Point oldPos = Pos;
	bool bumped = false;
	int pathfindingDistance = circleSize;
	int randomWalkCounter = 0;
public:
	inline int GetRandomBackoff() const
	{
		return randomBackoff;
	}
	void Backoff();
	inline void DecreaseBackoff()
	{
		randomBackoff--;
	}
	using Selectable::Selectable;
	Movable(const Movable&) = delete;
	~Movable() override;
	Movable& operator=(const Movable&) = delete;

	Point Destination = Pos;
	ResRef AreaName;
	Point HomeLocation;//spawnpoint, return here after rest
	ieWord maxWalkDistance = 0; // maximum random walk distance from home
public:
	inline void ImpedeBumping() { oldPos = Pos; bumped = false; }
	void AdjustPosition();
	void BumpAway();
	void BumpBack();
	inline bool IsBumped() const { return bumped; }
	PathListNode *GetNextStep(int x) const;
	inline PathListNode *GetPath() const { return path; };
	inline int GetPathTries() const	{ return pathTries; }
	inline void IncrementPathTries() { pathTries++; }
	inline void ResetPathTries() { pathTries = 0; }
	int GetPathLength() const;
//inliners to protect data consistency
	inline PathListNode * GetStep() {
		if (!step && area) {
			DoStep((unsigned int) ~0);
		}
		return step;
	}

	inline bool IsMoving() const {
		return (StanceID == IE_ANI_WALK || StanceID == IE_ANI_RUN);
	}

	orient_t GetNextFace() const;

	inline orient_t GetOrientation() const {
		return Orientation;
	}

	inline unsigned char GetStance() const {
		return StanceID;
	}

	void SetStance(unsigned int arg);
	void SetOrientation(orient_t value, bool slow);
	void SetOrientation(const Point& from, const Point& to, bool slow);
	void SetAttackMoveChances(const std::array<ieWord, 3>& amc);
	virtual void DoStep(unsigned int walkScale, ieDword time = 0);
	void AddWayPoint(const Point &Des);
	void RunAwayFrom(const Point &Des, int PathLength, bool noBackAway);
	void RandomWalk(bool can_stop, bool run);
	int GetRandomWalkCounter() const { return randomWalkCounter; };
	void MoveLine(int steps, orient_t Orient);
	void WalkTo(const Point &Des, int MinDistance = 0);
	void MoveTo(const Point &Des);
	void Stop(int flags = 0) override;
	void ClearPath(bool resetDestination = true);
	void HandleAnkhegStance(bool emerge);

	/* returns the most likely position of this actor */
	Point GetMostLikelyPosition() const;
	virtual bool BlocksSearchMap() const = 0;
};
}

#endif
