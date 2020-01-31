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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

/**
 * @file GameControl.h
 * Declares GameControl widget which is responsible for displaying areas,
 * interacting with PCs, NPCs and the rest of the game world.
 * @author The GemRB Project
 */

#ifndef GAMECONTROL_H
#define GAMECONTROL_H

#include "GUI/Control.h"

#include "exports.h"

#include "Dialog.h"
#include "Interface.h"
#include "Map.h"

namespace GemRB {

class GameControl;
class Window;
class DialogHandler;

//dialog flags
#define DF_IN_DIALOG      1
#define DF_TALKCOUNT      2
#define DF_UNBREAKABLE    4
#define DF_FREEZE_SCRIPTS 8
#define DF_INTERACT       16
#define DF_IN_CONTAINER   32
#define DF_OPENCONTINUEWINDOW 64
#define DF_OPENENDWINDOW 128
#define DF_POSTPONE_SCRIPTS 256

//screen flags
// !!! Keep these synchronized with GUIDefines.py !!!
#define SF_DISABLEMOUSE  1  //no mouse cursor
#define SF_CENTERONACTOR 2  //
#define SF_ALWAYSCENTER  4
#define SF_GUIENABLED    8  //
#define SF_LOCKSCROLL    16 //don't scroll
#define SF_CUTSCENE      32 //don't push new actions onto the action queue

// target modes and types
// !!! Keep these synchronized with GUIDefines.py !!!
#define TARGET_MODE_NONE    0
#define TARGET_MODE_TALK    1
#define TARGET_MODE_ATTACK  2
#define TARGET_MODE_CAST    3
#define TARGET_MODE_DEFEND  4
#define TARGET_MODE_PICK    5

static const unsigned long tp_steps[8]={3,2,1,0,1,2,3,4};

/**
 * @class GameControl
 * Widget displaying areas, where most of the game 'happens'.
 * It allows for interacting with PCs, NPCs and the rest of the world.
 * It's also a very core part of GemRB, as some processes are driven from it.
 * It's always assigned Control index 0.
 */

class GEM_EXPORT GameControl : public Control {
	enum WINDOW_GROUP {
		WINDOW_GROUP_LEFT,
		WINDOW_GROUP_BOTTOM,
		WINDOW_GROUP_RIGHT,
		WINDOW_GROUP_TOP,
		WINDOW_GROUP_COUNT
	};
	enum WINDOW_RESIZE_OPERATION {
		WINDOW_EXPAND = -1,
		WINDOW_CONTRACT = 1
	};
public:
	GameControl(const Region& frame);
	~GameControl(void);
protected:
	/** Draws the Control on the Output Display */
	void DrawInternal(Region& drawFrame);
public:
	// GameControl always needs to redraw
	bool NeedsDraw() const { return true; };
	/** Draws the target reticle for Actor movement. */
	void DrawTargetReticle(Point p, int size, bool animate, bool flash=false, bool actorSelected=false);
	/** Sets multiple quicksaves flag*/
	//static void MultipleQuickSaves(int arg);
	void SetTracker(Actor *actor, ieDword dist);
private:
	int windowGroupCounts[WINDOW_GROUP_COUNT];
	ieDword lastActorID;
	ieDword trackerID;
	ieDword distance;  //tracking distance
	std::vector< Actor*> highlighted;
	bool DrawSelectionRect;
	bool FormationRotation;
	bool MouseIsDown;
	bool DoubleClick;
	Region SelectionRect;
	Point FormationApplicationPoint;
	Point ClickPoint;
public:
	Door* overDoor;
	Container* overContainer;
	InfoPoint* overInfoPoint;

	// allow targeting allies, enemies and/or neutrals (bitmask)
	int target_types;
	unsigned short lastMouseX, lastMouseY;
private:
	// currently selected targeting type, such as talk, attack, cast, ...
	// private to enforce proper cursor changes
	int target_mode;
	unsigned char lastCursor;
	short moveX, moveY;
	int numScrollCursor;
	bool scrolling;
	int DebugFlags;
	Point pfs;
	PathNode* drawPath;
	unsigned long AIUpdateCounter;
	unsigned int ScreenFlags;
	unsigned int DialogueFlags;
	String* DisplayText;
	unsigned int DisplayTextTime;
	bool AlwaysRun;
public: //Events
	/** Key Press Event */
	bool OnKeyPress(unsigned char Key, unsigned short Mod);
	/** Key Release Event */
	bool OnKeyRelease(unsigned char Key, unsigned short Mod);
	/** Mouse Over Event */
	void OnMouseOver(unsigned short x, unsigned short y);
	/** Global Mouse Move Event */
	void OnGlobalMouseMove(short x, short y);
	/** Mouse Button Down */
	void OnMouseDown(unsigned short x, unsigned short y, unsigned short Button,
		unsigned short Mod);
	/** Mouse Button Up */
	void OnMouseUp(unsigned short x, unsigned short y, unsigned short Button,
		unsigned short Mod);
	void OnMouseWheelScroll(short x, short y);
	/** Special Key Press */
	bool OnSpecialKeyPress(unsigned char Key);
	void DisplayTooltip();
	void UpdateScrolling();
	void SetScrolling(bool scroll);
	void SetTargetMode(int mode);
	int GetTargetMode() { return target_mode; }
	void SetScreenFlags(unsigned int value, int mode);
	void SetDialogueFlags(unsigned int value, int mode);
	int GetScreenFlags() { return ScreenFlags; }
	int GetDialogueFlags() { return DialogueFlags; }
	void SetDisplayText(String* text, unsigned int time);
	void SetDisplayText(ieStrRef text, unsigned int time);
	/* centers viewport to the points specified */
	void Center(unsigned short x, unsigned short y);
	void ClearMouseState();
	void MoveViewportTo(int x, int y, bool center);
private:
	/** this function safely retrieves an Actor by ID */
	Actor *GetActorByGlobalID(ieDword ID);
	void CalculateSelection(const Point &p);
	void ResizeParentWindowFor(Window* win, int type, WINDOW_RESIZE_OPERATION);
	void ReadFormations();
	/** Draws an arrow on the edge of the screen based on the point (points at offscreen actors) */
	void DrawArrowMarker(const Region &screen, Point p, const Region &viewport, const Color& color);
	bool ShouldTriggerWorldMap(const Actor *pc) const;
	void ExecuteMovement(Actor *actor, unsigned short x, unsigned short y, bool createWaypoint);

private:
	Actor *user;     //the user of item or spell
public:
	DialogHandler *dialoghandler;
	//the name of the spell to cast
	ieResRef spellName;
	//using spell or item
	int spellOrItem; // -1 = item, otherwise the spell type
	//the user of spell or item
	Actor *spellUser;
	int spellSlot, spellIndex; //or inventorySlot/itemHeader
	int spellCount; //multiple targeting
public:
	/** Selects one or all PC */
	void SelectActor(int whom, int type = -1);
	void SetLastActor(Actor *actor, Actor *prevActor);
	void SetCutSceneMode(bool active);
	bool SetGUIHidden(bool hide);
	void TryToAttack(Actor *source, Actor *target);
	void TryToCast(Actor *source, const Point &p);
	void TryToCast(Actor *source, Actor *target);
	void TryToDefend(Actor *source, Actor *target);
	void TryToTalk(Actor *source, Actor *target);
	void TryToPick(Actor *source, Scriptable *tgt);
	void TryToDisarm(Actor *source, InfoPoint *tgt);
	void PerformActionOn(Actor *actor);
	void ResetTargetMode();
	void UpdateTargetMode();

	// returns the default cursor fitting the targeting mode 
	int GetDefaultCursor() const;
	//containers
	int GetCursorOverContainer(Container *overContainer) const;
	void HandleContainer(Container *container, Actor *actor);
	//doors
	int GetCursorOverDoor(Door *overDoor) const;
	void HandleDoor(Door *door, Actor *actor);
	//infopoints
	int GetCursorOverInfoPoint(InfoPoint *overInfoPoint) const;
	bool HandleActiveRegion(InfoPoint *trap, Actor *actor, Point &p);

	Point GetFormationOffset(ieDword formation, ieDword pos);
	Point GetFormationPoint(Map *map, unsigned int pos, Point src, Point p);
	/** calls MoveToPoint or RunToPoint */
	void CreateMovement(Actor *actor, const Point &p, bool append=true);
	/** checks if the actor should be running instead of walking */
	bool ShouldRun(Actor *actor) const;
	/** Displays a string over an object */
	void DisplayString(Scriptable* target);
	/** Displays a string on screen */
	void DisplayString(const Point &p, const char *Text);
	Actor *GetLastActor();
	/** changes map to the current PC */
	void ChangeMap(Actor *pc, bool forced);
	/** Returns game screenshot, with or without GUI controls */
	Sprite2D* GetScreenshot(const Region& rgn, bool show_gui = false );
	/** Returns current area preview for saving a game */
	Sprite2D* GetPreview();
	/** Returns PC portrait for a currently running game */
	Sprite2D* GetPortraitPreview(int pcslot);
	/** Sets up targeting with spells or items */
	void SetupItemUse(int slot, int header, Actor *actor, int targettype, int cnt);
	/** Page is the spell type + spell level info */
	void SetupCasting(ieResRef spellname, int type, int level, int slot, Actor *actor, int targettype, int cnt);
	bool SetEvent(int eventType, ControlEventHandler handler);
	void ToggleAlwaysRun();
};

}

#endif
