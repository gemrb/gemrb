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

#ifndef ACTORBLOCK_H
#define ACTORBLOCK_H

#include "exports.h"

#include "CharAnimations.h"
#include "Inventory.h"
#include "PathFinder.h"
#include "Sprite2D.h"
#include "TileOverlay.h"
#include "Variables.h"

#include <list>

class Action;
class Actor;
class Door;
class GameScript;
class Gem_Polygon;
class Highlightable;
class Movable;
class Scriptable;
class Selectable;
class SpriteCover;

#define MAX_SCRIPTS		8
#define MAX_GROUND_ICON_DRAWN   3
#define MAX_TIMER		256

/** The distance of operating a trigger, container, etc. */
#define MAX_OPERATING_DISTANCE      40 //a search square is 16x12
/** The distance between PC's who are about to enter a new area */
#define MAX_TRAVELING_DISTANCE      400

#define SCR_OVERRIDE 0
#define SCR_AREA	 1
#define SCR_SPECIFICS 2
#define SCR_RESERVED  3
#define SCR_CLASS    4
#define SCR_RACE	 5
#define SCR_GENERAL  6
#define SCR_DEFAULT  7

//pst trap flags (portal)
#define PORTAL_CURSOR 1
#define PORTAL_TRAVEL 2
//trigger flags
#define TRAP_INVISIBLE  1
#define TRAP_RESET      2
#define TRAVEL_PARTY    4
#define TRAP_DETECTABLE 8
//#define TRAP_16	 16
//#define TRAP_32	 32
#define TRAP_NPC	64
//#define TRAP_128	128
#define TRAP_DEACTIVATED  256
#define TRAVEL_NONPC      512
#define TRAP_USEPOINT       1024 //override usage point of travel regions
#define INFO_DOOR	 2048 //info trigger blocked by door

//door flags
#define DOOR_OPEN        1
#define DOOR_LOCKED      2
#define DOOR_RESET       4   //reset trap
#define DOOR_DETECTABLE  8   //trap detectable
#define DOOR_16          16  //unknown
#define DOOR_32          32  //unknown
#define DOOR_LINKED      64   //info trigger linked to this door
#define DOOR_SECRET      128  //door is secret
#define DOOR_FOUND       256  //secret door found
#define DOOR_TRANSPARENT 512  //obscures vision
#define DOOR_KEY         1024 //key removed when used
#define DOOR_SLIDE       2048 //impeded blocks ignored

//container flags
#define CONT_LOCKED      1
#define CONT_RESET       8
#define CONT_DISABLED    32

//internal actor flags
#define IF_GIVEXP     1     //give xp for this death
#define IF_JUSTDIED   2     //Died() will return true
#define IF_FROMGAME   4     //this is an NPC or PC
#define IF_REALLYDIED 8     //real death happened, actor will be set to dead
#define IF_NORECTICLE 16    //draw recticle (target mark)
#define IF_NOINT      32    //cannot interrupt the actions of this actor (save is not possible!)
#define IF_CLEANUP    64    //actor died chunky death, or other total destruction
#define IF_RUNNING    128   //actor is running
//these bits could be set by a WalkTo
#define IF_RUNFLAGS   (IF_RUNNING|IF_NORECTICLE|IF_NOINT)
#define IF_BECAMEVISIBLE 0x100//actor just became visible (trigger event)
#define IF_INITIALIZED   0x200
#define IF_USEDSAVE      0x400  //actor needed saving throws
#define IF_TARGETGONE    0x800  //actor's target is gone (trigger event)
#define IF_USEEXIT       0x1000 //
#define IF_INTRAP        0x2000 //actor is currently in a trap (intrap trigger event)
#define IF_WASINDIALOG   0x4000 //actor just left dialog

//scriptable flags
#define IF_ACTIVE        0x10000
#define IF_CUTSCENEID    0x20000
#define IF_VISIBLE       0x40000
#define IF_ONCREATION    0x80000
#define IF_IDLE          0x100000
#define IF_PARTYRESTED   0x200000 //party rested trigger event

//the actor should stop attacking
#define IF_STOPATTACK (IF_JUSTDIED|IF_REALLYDIED|IF_CLEANUP|IF_IDLE)

//CheckTravel return value
#define CT_CANTMOVE       0 //inactive
#define CT_ACTIVE         1 //actor can move
#define CT_GO_CLOSER      2 //entire team would move, but not close enough
#define CT_WHOLE          3 //team can move
#define CT_SELECTED       4 //not all selected actors are there
#define CT_MOVE_SELECTED  5 //all selected can move

//bits for binary trigger bitfield
#define BT_DIE            1
#define BT_ONCREATION     2
#define BT_BECAMEVISIBLE  4
#define BT_WASINDIALOG    8
#define BT_PARTYRESTED    16
#define BT_VACANT         32

//xp bonus types (for xpbonus.2da)
#define XP_LOCKPICK   0
#define XP_DISARM     1
#define XP_LEARNSPELL 2

typedef enum ScriptableType { ST_ACTOR = 0, ST_PROXIMITY = 1, ST_TRIGGER = 2,
ST_TRAVEL = 3, ST_DOOR = 4, ST_CONTAINER = 5, ST_AREA = 6, ST_GLOBAL = 7 } ScriptableType;

typedef std::list<ieDword *> TriggerObjects;

//#define SEA_RESET		0x00000002
//#define SEA_PARTY_REQUIRED	0x00000004

class GEM_EXPORT Scriptable {
public:
	Scriptable(ScriptableType type);
	virtual ~Scriptable(void);
private:
	TriggerObjects tolist;
	ieDword bittriggers;
	unsigned long startTime;
	unsigned long interval;
	unsigned long WaitCounter;
	// script_timers should probably be a std::map to
	// conserve memory (usually at most 2 ids are used)
	ieDword script_timers[MAX_TIMER];
protected: //let Actor access this
	Map *area;
	ieVariable scriptName;
	ieDword InternalFlags; //for triggers
	Scriptable* CutSceneId;
	ieResRef Dialog;
	std::list< Action*> actionQueue;
	Action* CurrentAction;
public:
	int CurrentActionState;
	ieWord CurrentActionTarget;
	bool CurrentActionInterruptable;
	ieDword lastDelay;
	ieDword lastRunTime;
	Variables* locals;
	ScriptableType Type;
	Point Pos;
	ieStrRef DialogName;
	GameScript* Scripts[MAX_SCRIPTS];
	char* overHeadText;
	Point overHeadTextPos;
	unsigned char textDisplaying;
	unsigned long timeStartDisplaying;
	ieDword UnselectableTimer;
	ieDword TriggerID; //for sendtrigger
	ieDword LastTrigger;  // also LastClosed
	ieDword LastTriggerObject; // hack until someone fixes triggers
	ieDword LastEntered;  // also LastOpened
	ieDword LastDisarmed; // also LastAttacker
	ieDword LastDisarmFailed; //also LastTarget
	ieDword LastUnlocked;
	ieDword LastOpenFailed; // also LastPickpocketFailed
	ieDword LastPickLockFailed;
	int LastOrder;
	ieDword LastOrderer;
	ieDword LastSpellOnMe;  //Last spell cast on this scriptable
	ieDword LastCasterOnMe; //Last spellcaster on this scriptable
	ieDword LastSpellSeen;  //Last spell seen to be cast
	ieDword LastCasterSeen; //Last spellcaster seen
	Point LastTargetPos;
	int SpellHeader;
public:
	/** Gets the Dialog ResRef */
	const char* GetDialog(void) const
	{
		return Dialog;
	}
	void SetDialog(const char *resref) {
		strnuprcpy(Dialog, resref, 8);
	}
	void SetScript(const ieResRef aScript, int idx, bool ai=false);
	void SetWait(unsigned long time);
	unsigned long GetWait() const;
	void LeaveDialog();
	Scriptable *GetCutsceneID() const;
	void ClearCutsceneID();
	void SetCutsceneID(Scriptable *csid);
	void Interrupt();
	void NoInterrupt();
	void Hide();
	void Unhide();
	void Activate();
	void Deactivate();
	void PartyRested();
	ieDword GetInternalFlag();
	const char* GetScriptName() const;
	Map* GetCurrentArea() const;
	void SetMap(Map *map);
	void SetScript(int index, GameScript* script);
	void DisplayHeadText(const char* text);
	void FixHeadTextPos();
	void SetScriptName(const char* text);
	//call this to deny script running in the next AI cycle
	void DelayedEvent();
	//call this to enable script running as soon as possible
	void ImmediateEvent();
	bool IsPC() const;
	void ExecuteScript(int scriptCount);
	void AddAction(Action* aC);
	void AddActionInFront(Action* aC);
	Action* GetCurrentAction() const { return CurrentAction; }
	Action* GetNextAction() const;
	Action* PopNextAction();
	void ClearActions();
	void ReleaseCurrentAction();
	bool InMove() const;
	void ProcessActions(bool force);
	//these functions handle clearing of triggers that resulted a
	//true condition (whole triggerblock returned true)
	void InitTriggers();
	void ClearTriggers();
	void SetBitTrigger(ieDword bittrigger);
	void AddTrigger(ieDword *actorref);
	/* re/draws overhead text on the map screen */
	void DrawOverheadText(const Region &screen);
	/* actor/scriptable casts spell */
	int CastSpellPoint( const ieResRef SpellResRef, const Point &Target, bool deplete, bool instant = false );
	int CastSpell( const ieResRef SpellResRef, Scriptable* Target, bool deplete, bool instant = false );
	/* spellcasting finished */
	void CastSpellPointEnd( const ieResRef SpellResRef);
	void CastSpellEnd( const ieResRef SpellResRef);
	ieWord GetGlobalID();
	/** timer functions (numeric ID, not saved) */
	bool TimerActive(ieDword ID);
	bool TimerExpired(ieDword ID);
	void StartTimer(ieDword ID, ieDword expiration);
	virtual char* GetName(int /*which*/) const { return NULL; }
private:
	/* used internally to handle start of spellcasting */
	int SpellCast(const ieResRef SpellResRef, bool instant);
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
	void SetCircle(int size, const Color &color, Sprite2D* normal_circle, Sprite2D* selected_circle);

	/* store SpriteCover */
	void SetSpriteCover(SpriteCover* c);
	/* get stored SpriteCover */
	SpriteCover* GetSpriteCover() const { return cover; }
	/* want dithered SpriteCover */
	int WantDither();
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
public:
	bool IsOver(const Point &Pos) const;
	void DrawOutline() const;
	void SetCursor(unsigned char CursorIndex);
	const char* GetKey(void) const
	{
		if (KeyResRef[0]) return KeyResRef;
		return NULL;
	}
	void SetTrapDetected(int x);
	void TryDisarm(Actor *actor);
	//detect trap, set skill to 256 if you want sure fire
	void DetectTrap(int skill);
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
public:
	Movable(ScriptableType type);
	virtual ~Movable(void);
	Point Destination;
	ieDword timeStartStep;
	Sprite2D* lastFrame;
	ieResRef Area;
public:
	PathNode *GetNextStep(int x);
	int GetPathLength();
//inliners to protect data consistency
	inline PathNode * GetNextStep() {
		if (!step) {
			DoStep((unsigned int) ~0);
		}
		return step;
	}

	inline unsigned char GetOrientation() const {
		return Orientation;
	}

	inline unsigned char GetNextFace() {
		//slow turning
		if (Orientation != NewOrientation) {
			if ( ( (NewOrientation-Orientation) & (MAX_ORIENT-1) ) <= MAX_ORIENT/2) {
				Orientation++;
			} else {
				Orientation--;
			}
			Orientation = Orientation&(MAX_ORIENT-1);
		}

		return Orientation;
	}
	inline unsigned char GetStance() const {
		return StanceID;
	}

	inline void SetOrientation(int value, bool slow) {
		//MAX_ORIENT == 16, so we can do this
		NewOrientation = (unsigned char) (value&(MAX_ORIENT-1));
		if (!slow) {
			Orientation = NewOrientation;
		}
	}

	void SetStance(unsigned int arg);
	void SetAttackMoveChances(ieWord *amc);
	bool DoStep(unsigned int walk_speed, ieDword time = 0);
	void AddWayPoint(const Point &Des);
	void RunAwayFrom(const Point &Des, int PathLength, int flags);
	void RandomWalk(bool can_stop, bool run);
	void MoveLine(int steps, int Pass, ieDword Orient);
	void FixPosition();
	void WalkTo(const Point &Des, int MinDistance = 0);
	void MoveTo(const Point &Des);
	void ClearPath();
	void DrawTargetPoint(const Region &vp);
	/* returns the most likely position of this actor */
	Point GetMostLikelyPosition();
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

class GEM_EXPORT Door : public Highlightable {
public:
	Door(TileOverlay* Overlay);
	~Door(void);
public:
	ieVariable LinkedInfo;
	ieResRef ID; //WED ID
	TileOverlay* overlay;
	unsigned short* tiles;
	int tilecount;
	ieDword Flags;
	int closedIndex;
	//trigger areas
	Gem_Polygon* open;
	Gem_Polygon* closed;
	//impeded blocks
	Point* open_ib; //impeded blocks stored in a Point array
	int oibcount;
	Point* closed_ib;
	int cibcount;
	//wallgroup covers
	unsigned int open_wg_index;
	unsigned int open_wg_count;
	unsigned int closed_wg_index;
	unsigned int closed_wg_count;
	Point toOpen[2];
	ieResRef OpenSound;
	ieResRef CloseSound;
	ieResRef LockSound;
	ieResRef UnLockSound;
	ieDword DiscoveryDiff;
	ieDword LockDifficulty; //this is a dword?
	ieStrRef OpenStrRef;
	ieStrRef NameStrRef;
	ieDword  Unknown54;     //unused in tob
private:
	void SetWallgroups(int count, int value);
	void ImpedeBlocks(int count, Point *points, unsigned char value);
	void UpdateDoor();
	bool BlockedOpen(int Open, int ForceOpen);
public:
	void ToggleTiles(int State, int playsound = false);
	void SetName(const char* Name); // sets door ID
	void SetTiles(unsigned short* Tiles, int count);
	void SetDoorLocked(int Locked, int playsound);
	void SetDoorOpen(int Open, int playsound, ieDword ID);
	void SetPolygon(bool Open, Gem_Polygon* poly);
	int IsOpen() const;
	void TryPickLock(Actor *actor);
	void TryBashLock(Actor* actor) ;
	bool TryUnlock(Actor *actor);
	void TryDetectSecret(int skill);
	void DebugDump() const;
	int TrapResets() const { return Flags & DOOR_RESET; }
	void SetNewOverlay(TileOverlay *Overlay);
};

class GEM_EXPORT Container : public Highlightable {
public:
	Container(void);
	~Container(void);
	void SetContainerLocked(bool lock);
	//turns the container to a pile
	void DestroyContainer();
	//removes an item from the container's inventory
	CREItem *RemoveItem(unsigned int idx, unsigned int count);
	//adds an item to the container's inventory
	int AddItem(CREItem *item);
	//draws the ground icons
	void DrawPile(bool highlight, Region screen, Color tint);
	//returns dithering option
	int WantDither();
	int IsOpen() const;
	void TryPickLock(Actor *actor);
	void TryBashLock(Actor* actor) ;
	bool TryUnlock(Actor *actor);
	void DebugDump() const;
	int TrapResets() const { return Flags & CONT_RESET; }
private:
	//updates the ground icons for a pile
	void RefreshGroundIcons();
	void FreeGroundIcons();
	void CreateGroundIconCover();
public:
	Point toOpen;
	ieWord Type;
	ieDword Flags;
	ieWord LockDifficulty;
	Inventory inventory;
	ieStrRef OpenFail;
	//these are not saved
	Sprite2D *groundicons[3];
	SpriteCover *groundiconcover;
	//keyresref is stored in Highlightable
};

class GEM_EXPORT InfoPoint : public Highlightable {
public:
	InfoPoint(void);
	~InfoPoint(void);
	//returns true if trap has been triggered, tumble skill???
	bool TriggerTrap(int skill, ieDword ID);
	//call this to check if an actor entered the trigger zone
	bool Entered(Actor *actor);
	//checks if the actor may use this travel trigger
	int CheckTravel(Actor *actor);
	void DebugDump() const;
	int TrapResets() const { return Flags & TRAP_RESET; }
	bool CanDetectTrap() const;
	bool PossibleToSeeTrap() const;
	bool IsPortal() const;

public:
	ieResRef Destination;
	ieVariable EntranceName;
	ieDword Flags;
	//overheadtext contains the string, but we have to save this
	ieStrRef StrRef;
	Point UsePoint;
	Point TalkPos;
};

#endif
