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

#include "CharAnimations.h"
#include "Variables.h"

#include <list>
#include <map>

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
struct PathNode;
class Scriptable;
class Selectable;
class Spell;
class Sprite2D;
class SpriteCover;

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
//#define TRAP_SILENT	128 // "trigger silent", unused
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

#define MAX_PATH_TRIES 8
#define MAX_BUMP_BACK_TRIES 16
#define MAX_RAND_WALK 10

typedef enum ScriptableType { ST_ACTOR = 0, ST_PROXIMITY = 1, ST_TRIGGER = 2,
ST_TRAVEL = 3, ST_DOOR = 4, ST_CONTAINER = 5, ST_AREA = 6, ST_GLOBAL = 7 } ScriptableType;

enum {
	trigger_acquired = 0x1,
	trigger_attackedby = 0x2,
	trigger_help = 0x3,
	trigger_joins = 0x4,
	trigger_leaves = 0x5,
	trigger_receivedorder = 0x6,
	trigger_said = 0x7,
	trigger_turnedby = 0x8,
	trigger_unusable = 0x9,
	trigger_hitby = 0x20,
	trigger_hotkey = 0x21,
	trigger_timerexpired = 0x22,
	trigger_trigger = 0x24,
	trigger_die = 0x25,
	trigger_targetunreachable = 0x26,
	trigger_heard = 0x2f,
	trigger_becamevisible = 0x33,
	trigger_oncreation = 0x36,
	trigger_died = 0x4a,
	trigger_killed = 0x4b,
	trigger_entered = 0x4c,
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
	trigger_triggerclick = 0x79, // pst
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
	trigger_nulldialog = 0xa4, // pst
	trigger_wasindialog = 0xa5, // pst
	trigger_spellcastpriest = 0xa6, // bg2
	trigger_spellcastinnate = 0xa7, // bg2
	trigger_namelessbitthedust = 0xab, // pst
	trigger_failedtoopen = 0xaf, // pst
	trigger_tookdamage = 0xcc, // bg2
	trigger_walkedtotrigger = 0xd6 // bg2
};

// flags for TriggerEntry
enum {
	// has the effect queue (if any) been processed since this trigger
	// was added? (for fx_cast_spell_on_condition)
	TEF_PROCESSED_EFFECTS = 1
};

struct TriggerEntry {
	TriggerEntry(unsigned short id) : triggerID(id), param1(0), param2(0), flags(0) { }
	TriggerEntry(unsigned short id, ieDword p1) : triggerID(id), param1(p1), param2(0), flags(0) { }
	TriggerEntry(unsigned short id, ieDword p1, ieDword p2) : triggerID(id), param1(p1), param2(p2), flags(0) { }

	unsigned short triggerID;
	ieDword param1;
	ieDword param2;
	unsigned int flags;
};

//typedef std::list<ieDword *> TriggerObjects;

//#define SEA_RESET		0x00000002
//#define SEA_PARTY_REQUIRED	0x00000004

class GEM_EXPORT Scriptable {
public:
	Scriptable(ScriptableType type);
	virtual ~Scriptable(void);
private:
	unsigned long WaitCounter;
	std::map<ieDword,ieDword> script_timers;
	ieDword globalID;
protected: //let Actor access this
	std::list<TriggerEntry> triggers;
	Map *area;
	ieVariable scriptName;
	ieDword InternalFlags; //for triggers
	ieResRef Dialog;
	std::list< Action*> actionQueue;
	Action* CurrentAction;

	// Variables for overhead text.
	Point overHeadTextPos;
	bool overheadTextDisplaying;
	unsigned long timeStartDisplaying;
	String OverheadText;
public:
	// State relating to the currently-running action.
	int CurrentActionState;
	ieDword CurrentActionTarget;
	bool CurrentActionInterruptable;
	ieDword CurrentActionTicks;

	// The number of times this was updated.
	ieDword Ticks;
	// The same, after adjustment for being slowed/hasted.
	ieDword AdjustedTicks;
	// The number of times UpdateActions() was run.
	ieDword ScriptTicks;
	// The number of times since UpdateActions() tried to do anything.
	ieDword IdleTicks;
	// The number of ticks since the last spellcast
	ieDword AuraTicks;
	// The countdown for forced activation by triggers.
	ieDword TriggerCountdown;

	Variables* locals;
	ScriptableType Type;
	Point Pos;

	ieStrRef DialogName;

	GameScript* Scripts[MAX_SCRIPTS];
	int scriptlevel;

	ieDword UnselectableTimer;

	// Stored objects.
	ieDword LastAttacker;
	ieDword LastCommander;
	ieDword LastProtector;
	ieDword LastProtectee;
	ieDword LastTargetedBy;
	ieDword LastHitter;
	ieDword LastHelp;
	ieDword LastTrigger;
	ieDword LastSeen;
	ieDword LastTalker;
	ieDword LastHeard;
	ieDword LastSummoner;
	ieDword LastFollowed; // gemrb extension (LeaderOf)
	ieDword LastMarked; // iwd2
	ieDword MyTarget = 0; // iwd2, has nothing to do with LastTarget

	int LastMarkedSpell; // iwd2

	// this is used by GUIScript :(
	ieDword LastSpellOnMe;  //Last spell cast on this scriptable

	ieDword LastTarget, LastSpellTarget;
	ieDword LastTargetPersistent; // gemrb extension, persists across actions; remove if LastTarget ever gets the same persistence
	Point LastTargetPos;
	int SpellHeader;
	ieResRef SpellResRef;
	bool InterruptCasting;
public:
	/** Gets the Dialog ResRef */
	const char* GetDialog(void) const
	{
		return Dialog;
	}
	void SetDialog(const char *resref);
	void SetFloatingText(char*);
	void SetScript(const ieResRef aScript, int idx, bool ai=false);
	void SetSpellResRef(ieResRef resref);
	void SetWait(unsigned long time);
	unsigned long GetWait() const;
	void LeftDialog();
	void Interrupt();
	void NoInterrupt();
	void Hide();
	void Unhide();
	void Activate();
	void Deactivate();
	void PartyRested();
	ieDword GetInternalFlag() const;
	void SetInternalFlag(unsigned int value, int mode);
	const char* GetScriptName() const;
	Map* GetCurrentArea() const;
	void SetMap(Map *map);
	void SetOverheadText(const String& text, bool display = true);
	const String& GetOverheadText() { return OverheadText; };
	bool DisplayOverheadText(bool);
	bool OverheadTextIsDisplaying() const { return overheadTextDisplaying; }
	void FixHeadTextPos();
	void SetScriptName(const char* text);
	//call this to enable script running as soon as possible
	void ImmediateEvent();
	bool IsPC() const;
	virtual void Update();
	void TickScripting();
	virtual void ExecuteScript(int scriptCount);
	void AddAction(Action* aC);
	void AddActionInFront(Action* aC);
	Action* GetCurrentAction() const { return CurrentAction; }
	Action* GetNextAction() const;
	Action* PopNextAction();
	void ClearActions();
	virtual void Stop();
	virtual void ReleaseCurrentAction();
	bool InMove() const;
	void ProcessActions();
	//these functions handle clearing of triggers that resulted a
	//true condition (whole triggerblock returned true)
	void InitTriggers();
	void AddTrigger(TriggerEntry trigger);
	bool MatchTrigger(unsigned short id, ieDword param = 0);
	bool MatchTriggerWithObject(short unsigned int id, const Object *obj, ieDword param = 0);
	const TriggerEntry *GetMatchingTrigger(unsigned short id, unsigned int notflags = 0);
	void SendTriggerToAll(TriggerEntry entry);
	/* re/draws overhead text on the map screen */
	void DrawOverheadText(const Region &screen);
	/* check if casting is allowed at all */
	int CanCast(const ieResRef SpellRef, bool verbose = true);
	/* check for and trigger a wild surge */
	int CheckWildSurge();
	void SpellcraftCheck(const Actor *caster, const ieResRef SpellRef);
	/* internal spellcasting shortcuts */
	void DirectlyCastSpellPoint(const Point &target, ieResRef spellref, int level, int no_stance, bool deplete);
	void DirectlyCastSpell(Scriptable *target, ieResRef spellref, int level, int no_stance, bool deplete);
	/* actor/scriptable casts spell */
	int CastSpellPoint( const Point &Target, bool deplete, bool instant = false, bool nointerrupt = false );
	int CastSpell( Scriptable* Target, bool deplete, bool instant = false, bool nointerrupt = false );
	/* spellcasting finished */
	void CastSpellPointEnd(int level, int no_stance);
	void CastSpellEnd(int level, int no_stance);
	ieDword GetGlobalID() const { return globalID; }
	/** timer functions (numeric ID, not saved) */
	bool TimerActive(ieDword ID);
	bool TimerExpired(ieDword ID);
	void StartTimer(ieDword ID, ieDword expiration);
	virtual char* GetName(int /*which*/) const { return NULL; }
	bool AuraPolluted();
private:
	/* used internally to handle start of spellcasting */
	int SpellCast(bool instant, Scriptable *target = NULL);
	/* also part of the spellcasting process, creating the projectile */
	void CreateProjectile(const ieResRef SpellResRef, ieDword tgt, int level, bool fake);
	/* do some magic for the weird/awesome wild surges */
	bool HandleHardcodedSurge(ieResRef surgeSpellRef, Spell *spl, Actor *caster);
	void ResetCastingState(Actor* caster);
	void DisplaySpellCastMessage(ieDword tgt, Spell *spl);
};

class GEM_EXPORT Selectable : public Scriptable {
public:
	Selectable(ScriptableType type);
	virtual ~Selectable(void);
public:
	Region BBox;
	ieWord Selected; //could be 0x80 for unselectable
	bool Over;
	Color selectedColor;
	Color overColor;
	Sprite2D *circleBitmap[2];
	int size;
	float sizeFactor;
private:
	// current SpriteCover for wallgroups
	SpriteCover* cover;
public:
	void SetBBox(const Region &newBBox);
	void DrawCircle(const Region &vp);
	bool IsOver(const Point &Pos) const;
	void SetOver(bool over);
	bool IsSelected() const;
	void Select(int Value);
	void SetCircle(int size, float, const Color &color, Sprite2D* normal_circle, Sprite2D* selected_circle);

	/* store SpriteCover */
	void SetSpriteCover(SpriteCover* c);
	/* get stored SpriteCover */
	SpriteCover* GetSpriteCover() const { return cover; }
	/* want dithered SpriteCover */
	int WantDither() const;
};

class GEM_EXPORT Highlightable : public Scriptable {
public:
	Highlightable(ScriptableType type);
	virtual ~Highlightable(void);
	virtual int TrapResets() const = 0;
	virtual bool CanDetectTrap() const { return true; }
	virtual bool PossibleToSeeTrap() const;
public:
	Gem_Polygon* outline;
	Color outlineColor;
	ieDword Cursor;
	bool Highlight;
	Point TrapLaunch;
	ieWord TrapDetectionDiff;
	ieWord TrapRemovalDiff;
	ieWord Trapped;
	ieWord TrapDetected;
	ieResRef KeyResRef;
	//play this wav file when stepping on the trap (on PST)
	ieResRef EnterWav;
public:
	bool IsOver(const Point &Place) const;
	void DrawOutline() const;
	void SetCursor(unsigned char CursorIndex);
	const char* GetKey(void) const
	{
		if (KeyResRef[0]) return KeyResRef;
		return NULL;
	}
	void SetTrapDetected(int x);
	void TryDisarm(const Actor *actor);
	//detect trap, set skill to 256 if you want sure fire
	void DetectTrap(int skill, ieDword actorID);
	//returns true if trap is visible, only_detected must be true
	//if you want to see discovered traps, false is for cheats
	bool VisibleTrap(int only_detected) const;
	//returns true if trap has been triggered, tumble skill???
	virtual bool TriggerTrap(int skill, ieDword ID);
	bool TryUnlock(Actor *actor, bool removekey);
};

class GEM_EXPORT Movable : public Selectable {
private: //these seem to be sensitive, so get protection
	unsigned char StanceID;
	unsigned char Orientation, NewOrientation;
	ieWord AttackMovements[3];

	PathNode* path; //whole path
	PathNode* step; //actual step
	unsigned int prevTicks;
	int bumpBackTries;
	bool pathAbandoned;
protected:
	ieDword timeStartStep;
	//the # of previous tries to pick up a new walkpath
	int pathTries;
	int randomBackoff;
	Point oldPos;
	bool bumped;
	int pathfindingDistance;
	int randomWalkCounter;
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
	Movable(ScriptableType type);
	virtual ~Movable(void);
	Point Destination;
	Sprite2D* lastFrame;
	ieResRef Area;
	Point HomeLocation;//spawnpoint, return here after rest
	ieWord maxWalkDistance;//maximum random walk distance from home
public:
	inline void ImpedeBumping() { oldPos = Pos; bumped = false; }
	void AdjustPosition();
	void BumpAway();
	void BumpBack();
	inline bool IsBumped() const { return bumped; }
	PathNode *GetNextStep(int x) const;
	inline PathNode *GetPath() const { return path; };
	inline int GetPathTries() const	{ return pathTries; }
	inline void IncrementPathTries() { pathTries++; }
	inline void ResetPathTries() { pathTries = 0; }
	int GetPathLength() const;
//inliners to protect data consistency
	inline PathNode * GetStep() {
		if (!step) {
			DoStep((unsigned int) ~0);
		}
		return step;
	}

	inline bool IsMoving() const {
		return (StanceID == IE_ANI_WALK || StanceID == IE_ANI_RUN);
	}

	unsigned char GetNextFace();

	inline unsigned char GetOrientation() const {
		return Orientation;
	}

	inline unsigned char GetStance() const {
		return StanceID;
	}

	void SetStance(unsigned int arg);
	void SetOrientation(int value, bool slow);
	void SetAttackMoveChances(ieWord *amc);
	virtual void DoStep(unsigned int walkScale, ieDword time = 0);
	void AddWayPoint(const Point &Des);
	void RunAwayFrom(const Point &Des, int PathLength, bool noBackAway);
	void RandomWalk(bool can_stop, bool run);
	int GetRandomWalkCounter() const { return randomWalkCounter; };
	void MoveLine(int steps, ieDword Orient);
	void WalkTo(const Point &Des, int MinDistance = 0);
	void MoveTo(const Point &Des);
	void Stop();
	void ClearPath(bool resetDestination = true);

	/* returns the most likely position of this actor */
	Point GetMostLikelyPosition() const;
	virtual bool BlocksSearchMap() const = 0;
};

//Tiled objects are not used (and maybe not even implemented correctly in IE)
//they seem to be most closer to a door and probably obsoleted by it
//are they scriptable?
class GEM_EXPORT TileObject {
public:
	TileObject(void);
	~TileObject(void);
	void SetOpenTiles(unsigned short *indices, int count);
	void SetClosedTiles(unsigned short *indices, int count);

public:
	ieVariable Name;
	ieResRef Tileset; //or wed door ID?
	ieDword Flags;
	unsigned short* opentiles;
	ieDword opencount;
	unsigned short* closedtiles;
	ieDword closedcount;
};

}

#endif
