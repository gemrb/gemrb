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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/ActorBlock.h,v 1.54 2005/02/19 19:09:45 avenger_teambg Exp $
 *
 */

class GameScript;
class Action;
class Scriptable;
class Selectable;
class Highlightable;
class Actor;
class Moveble;
class Door;

#ifndef ACTORBLOCK_H
#define ACTORBLOCK_H

#include "Sprite2D.h"
#include "CharAnimations.h"
#include "Polygon.h"
#include "TileOverlay.h"
#include "Variables.h"
#include "Inventory.h"
#include "PathFinder.h"
#include "ImageMgr.h"
#include <list>

#define STEP_TIME		150
#define MAX_SCRIPTS		8

#define SCR_OVERRIDE 0
#define SCR_AREA	 1
#define SCR_SPECIFICS 2
#define SCR_CLASS    3
#define SCR_UNKNOWN  4
#define SCR_RACE	 5
#define SCR_GENERAL  6
#define SCR_DEFAULT  7

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
//#define TRAP_1024       1024 //override?
#define INFO_DOOR	 2048 //info trigger linked to door

//door flags
#define DOOR_CLOSED      1
#define DOOR_LOCKED      2

//container flags
#define CONT_LOCKED      1
#define CONT_RESET       8
#define CONT_DISABLED    32

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

typedef enum ScriptableType { ST_ACTOR = 0, ST_PROXIMITY = 1, ST_TRIGGER = 2,
ST_TRAVEL = 3, ST_DOOR = 4, ST_CONTAINER = 5, ST_AREA = 6, ST_GLOBAL = 7 } ScriptableType;

//#define SEA_RESET		0x00000002
//#define SEA_PARTY_REQUIRED	0x00000004

class GEM_EXPORT Scriptable {
public:
	Scriptable(ScriptableType type);
	virtual ~Scriptable(void);
private:
	unsigned long startTime;
	unsigned long interval;
	unsigned long WaitCounter;
	bool neverExecuted;
public:
	Variables* locals;
	ScriptableType Type;
	Point Pos;
	Map *area;
	Scriptable* MySelf;
	Scriptable* CutSceneId;
	GameScript* Scripts[MAX_SCRIPTS];
	char* overHeadText;
	unsigned char textDisplaying;
	unsigned long timeStartDisplaying;
	char scriptName[33];
	bool Active;
	Scriptable* LastTrigger;
	Scriptable* LastEntered;
	std::list< Action*> actionQueue;
	Action* CurrentAction;
	bool resetAction;
	bool OnCreation;
	unsigned long playDeadCounter;
	Inventory inventory;
public:
	void SetScript(ieResRef aScript, int idx);
	void SetWait(unsigned long time);
	//void SetPosition(Point &Pos, int areaindex);
	void SetMySelf(Scriptable* MySelf);
	char* GetScriptName();
	void SetScript(int index, GameScript* script);
	void DisplayHeadText(const char* text);
	void SetScriptName(const char* text);
	void ExecuteScript(GameScript* Script);
	void AddAction(Action* aC);
	void AddActionInFront(Action* aC);
	Action* GetNextAction();
	Action* PopNextAction();
	void ClearActions();
	virtual void ProcessActions();
};

class GEM_EXPORT Selectable : public Scriptable {
public:
	Selectable(ScriptableType type);
	virtual ~Selectable(void);
public:
	Region BBox;
	bool Selected;
	bool Over;
	Color selectedColor;
	Color overColor;
	int size;
public:
	void SetBBox(Region &newBBox);
	void DrawCircle();
	bool IsOver(Point &Pos);
	void SetOver(bool over);
	bool IsSelected();
	void Select(bool Value);
	void SetCircle(int size, Color color);
};

class GEM_EXPORT Highlightable : public Scriptable {
public:
	Highlightable(ScriptableType type);
	virtual ~Highlightable(void);
public:
	Gem_Polygon* outline;
	Color outlineColor;
	unsigned long Cursor;
	bool Highlight;
	ieResRef DialogResRef;
	Point TrapLaunch;
	ieResRef KeyResRef;
public:
	bool IsOver(Point &Pos);
	void DrawOutline();
	/** Gets the Dialog ResRef */
	const char* GetDialog(void)
	{
		return DialogResRef;
	}
};

class GEM_EXPORT Moveble : public Selectable {
private: //these seem to be sensitive, so get protection
	unsigned char StanceID;
	unsigned char Orientation;
public:
	Moveble(ScriptableType type);
	virtual ~Moveble(void);
	Point Destination;
	PathNode* path;
	PathNode* step;
	unsigned long timeStartStep;
	Sprite2D* lastFrame;
	ieDword TalkCount;
	int InternalFlags;
	ieResRef Area;
public:
//inliners to protect data consistency
	inline unsigned char GetOrientation() {
		return Orientation;
	}
	inline unsigned char GetStance() {
		return StanceID;
	}

	inline void SetOrientation(int value) {
		Orientation=(unsigned char) value;
#ifdef _DEBUG
		if (Orientation>=MAX_ORIENT) {
			printf("We are badly oriented!\n");
			abort();
		}
#endif
	}

	void SetStance(unsigned int arg);
	void DoStep();
	void AddWayPoint(Point &Des);
	void RunAwayFrom(Point &Des, int PathLength, bool Backing);
	void WalkTo(Point &Des);
	void MoveTo(Point &Des);
	void ClearPath();
	void DrawTargetPoint();
};

class GEM_EXPORT Door : public Highlightable {
public:
	Door(TileOverlay* Overlay);
	~Door(void);
public:
	char Name[33]; //scriptable name
	char ID[9];    //WED ID
	TileOverlay* overlay;
	unsigned short* tiles;
	unsigned char count;
	unsigned int Flags;
	int closedIndex;
	Gem_Polygon* open;
	Gem_Polygon* closed;
	Point toOpen[2];
	ieResRef OpenSound;
	ieResRef CloseSound;
private:
	void ToggleTiles(int State, bool playsound = false);
public:
	void SetName(char* Name);
	void SetScriptName(char* Name);
	void SetTiles(unsigned short* Tiles, int count);
	void SetDoorLocked(bool Locked, bool playsound = false);
	void SetDoorClosed(bool Closed, bool playsound = false);
	void SetPolygon(bool Open, Gem_Polygon* poly);
	void SetCursor(unsigned char CursorIndex);
	void DebugDump();
};

class GEM_EXPORT Container : public Highlightable {
public:
	Container(void);
	~Container(void);
	void DebugDump();
public:
	char Name[33];
	Point toOpen;
	unsigned short Type;
	unsigned short LockDifficulty;
	unsigned int Flags;
	unsigned short TrapDetectionDiff;
	unsigned short TrapRemovalDiff;
	unsigned short Trapped;
	unsigned short TrapDetected;
	Inventory inventory;
};

class GEM_EXPORT InfoPoint : public Highlightable {
public:
	InfoPoint(void);
	~InfoPoint(void);
	//detect trap, set skill to 256 if you want sure fire
	void DetectTrap(int skill);
	//returns true if trap is visible, only_detected must be true
	//if you want to see discovered traps, false is for cheats
	bool VisibleTrap(bool only_detected);
	//returns true if trap has been triggered, tumble skill???
	bool TriggerTrap(int skill);
	//checks if the actor may use this travel trigger
	int CheckTravel(Actor *actor);
	void DebugDump();

public:
	char Name[33];
	ieResRef Destination;
	char EntranceName[33];
	unsigned int Flags;
	unsigned short TrapDetectionDifficulty;
	unsigned short TrapRemovalDifficulty;
//	unsigned short Trapped;
	unsigned short TrapDetected;
};

#endif
