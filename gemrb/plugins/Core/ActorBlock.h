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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/ActorBlock.h,v 1.38 2004/07/25 18:19:41 avenger_teambg Exp $
 *
 */

class GameScript;
class Action;
class Scriptable;
class Selectable;
class Highlightable;
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
//#define TRAP_16         16
//#define TRAP_32         32
#define TRAP_NPC        64
//#define TRAP_128        128
#define TRAP_DEACTIVATED  256
#define TRAVEL_NONPC      512
//#define TRAP_1024       1024 //override?
#define INFO_DOOR         2048 //info trigger linked to door

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

#define SEA_RESET		0x00000002
#define SEA_PARTY_REQUIRED	0x00000004

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
	unsigned short XPos, YPos;
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
	unsigned long EndAction;
	std::list< Action*> actionQueue;
	Action* CurrentAction;
	bool resetAction;
	bool OnCreation;
	unsigned long playDeadCounter;
	Inventory inventory;
public:
	void SetScript(const char* aScript, int idx);
	void SetWait(unsigned long time);
	void SetPosition(unsigned short XPos, unsigned short YPos);
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
private:
	bool Selected;
	bool Over;
	Color selectedColor;
	Color overColor;
	int size;
public:
	void SetBBox(Region newBBox);
	void DrawCircle();
	bool IsOver(unsigned short XPos, unsigned short YPos);
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
public:
	bool IsOver(unsigned short XPos, unsigned short YPos);
	void DrawOutline();
};

class GEM_EXPORT Moveble : public Selectable {
public:
	Moveble(ScriptableType type);
	virtual ~Moveble(void);
	unsigned short XDes, YDes;
	unsigned char Orientation;
	unsigned char AnimID;
	PathNode* path;
	PathNode* step;
	unsigned long timeStartStep;
	Sprite2D* lastFrame;
	char Area[9];
	long TalkCount;
public:
	void DoStep(ImageMgr* LightMap);
	void AddWayPoint(unsigned short XDes, unsigned short YDes);
	void RunAwayFrom(unsigned short XDes, unsigned short YDes, int PathLength, bool Backing);
	void WalkTo(unsigned short XDes, unsigned short YDes);
	void MoveTo(unsigned short XDes, unsigned short YDes);
	void ClearPath();
};

class GEM_EXPORT Door : public Highlightable {
public:
	Door(TileOverlay* Overlay);
	~Door(void);
public:
	char Name[9];
	TileOverlay* overlay;
	unsigned short* tiles;
	unsigned char count;
	unsigned int Flags;
	int closedIndex;
	Gem_Polygon* open;
	Gem_Polygon* closed;
	Point toOpen[2];
	char OpenSound[9];
	char CloseSound[9];
private:
	void ToggleTiles(int State, bool playsound = false);
public:
	void SetName(char* Name);
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
	unsigned short Locked;
	unsigned short TrapDetectionDiff;
	unsigned short TrapRemovalDiff;
	unsigned short Trapped;
	unsigned short TrapDetected;
	Point trapTarget;
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
	void DebugDump();
public:
	char Name[33];
	char Destination[9];
	char EntranceName[33];
	unsigned long Flags;
	unsigned short TrapDetectionDifficulty;
	unsigned short TrapRemovalDifficulty;
	unsigned short Trapped;
	unsigned short TrapDetected;
	unsigned short TrapLaunchX, TrapLaunchY;
	char KeyResRef[9];
	char DialogResRef[9];
};

#endif
