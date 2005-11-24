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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/GameControl.h,v 1.79 2005/11/24 17:44:08 wjpalenstijn Exp $
 */

/**
 * @file GameControl.h
 * Declares GameControl widget which is responsible for displaying areas,
 * interacting with PCs, NPCs and the rest of the game world.
 * @author The GemRB Project
 */

class GameControl;
class Window;

#ifndef GAMECONTROL_H
#define GAMECONTROL_H

#include "Control.h"
#include "Interface.h"
#include "Dialog.h"
#include "Map.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

//dialog flags
#define DF_IN_DIALOG      1
#define DF_TALKCOUNT      2
#define DF_UNBREAKABLE    4
#define DF_FREEZE_SCRIPTS 8
#define DF_INTERACT       16
#define DF_IN_CONTAINER   32
#define DF_OPENCONTINUEWINDOW 64
#define DF_OPENENDWINDOW 128

//screen flags
// !!! Keep these synchronized with GUIDefines.py !!!
#define SF_DISABLEMOUSE  1
#define SF_CENTERONACTOR 2
#define SF_ALWAYSCENTER  4
#define SF_GUIENABLED    8
#define SF_LOCKSCROLL    16

// target modes
// !!! Keep these synchronized with GUIDefines.py !!!
#define TARGET_MODE_NONE    0x00
#define TARGET_MODE_TALK    0x01
#define TARGET_MODE_ATTACK  0x02
#define TARGET_MODE_CAST    0x04

#define TARGET_MODE_ALLY    0x10
#define TARGET_MODE_ENEMY   0x20
#define TARGET_MODE_NEUTRAL 0x40

//the distance of operating a trigger, container, etc.
#define MAX_OPERATING_DISTANCE      40 //a search square is 16x12
//the distance between PC's who are about to enter a new area 
#define MAX_TRAVELING_DISTANCE      400

/**
 * @class GameControl
 * Widget displaying areas, where most of the game 'happens'.
 * It allows for interacting with PCs, NPCs and the rest of the world.
 * It's also a very core part of GemRB, as some processes are driven from it.
 * It's always assigned Control index 0.
 */

class GEM_EXPORT GameControl : public Control {
public:
	GameControl(void);
	~GameControl(void);
public:
	/** Draws the Control on the Output Display */
	void Draw(unsigned short x, unsigned short y);
	/** Sets the Text of the current control */
	int SetText(const char* string, int pos = 0);
	/** Sets the InfoTextColor, used in PST */
	void SetInfoTextColor(Color color);
private:
	//Actor* lastActor;
	//using global ID which is safer
	ieWord lastActorID;
	std::vector< Actor*> highlighted;
	std::vector< Scriptable*> infoTexts;
	Color* InfoTextPalette;
	bool DrawSelectionRect;
	bool MouseIsDown;
	Region SelectionRect;
	short StartX, StartY;
	int action;
public:
	Door* overDoor;
	Container* overContainer;
	InfoPoint* overInfoPoint;
	int target_mode;
private:
	unsigned char lastCursor;
	short moveX, moveY;
	unsigned short lastMouseX, lastMouseY;
	int DebugFlags;
	Point pfs;
	PathNode* drawPath;
	unsigned long AIUpdateCounter;
	int ScreenFlags;
	int DialogueFlags;
public: //Events
	/** Key Press Event */
	void OnKeyPress(unsigned char Key, unsigned short Mod);
	/** Key Release Event */
	void OnKeyRelease(unsigned char Key, unsigned short Mod);
	/** Mouse Over Event */
	void OnMouseOver(unsigned short x, unsigned short y);
	/** Mouse Button Down */
	void OnMouseDown(unsigned short x, unsigned short y, unsigned char Button,
		unsigned short Mod);
	/** Mouse Button Up */
	void OnMouseUp(unsigned short x, unsigned short y, unsigned char Button,
		unsigned short Mod);
	/** Special Key Press */
	void OnSpecialKeyPress(unsigned char Key);
	void SetScreenFlags(int value, int mode);
	void SetDialogueFlags(int value, int mode);
	int GetScreenFlags() { return ScreenFlags; }
	int GetDialogueFlags() { return DialogueFlags; }
private:
	/** this function safely retrieves an Actor by ID */
	Actor *GetActorByGlobalID(ieWord ID);
	void CalculateSelection(Point &p);
	void ResizeDel(Window* win, unsigned char type);
	void ResizeAdd(Window* win, unsigned char type);
	void HandleWindowHide(const char *WindowName, const char *WindowPosition);
	void HandleWindowReveal(const char *WindowName, const char *WindowPosition);
	void ReadFormations();
	unsigned char LeftCount, BottomCount, RightCount, TopCount;
	DialogState* ds;
	Dialog* dlg;
public:
	//Actor* speaker;
	//Actor* target;
	ieWord speakerID;
	ieWord targetID;
public:
	Actor *GetTarget();
	Actor *GetSpeaker();
	void DeselectAll();
	/* Selects one or all PC */
	void SelectActor(int whom);
	void SetCutSceneMode(bool active);
	int HideGUI();
	int UnhideGUI();
	void TryToAttack(Actor *source, Actor *target);
	void TryToTalk(Actor *source, Actor *target);
	void HandleContainer(Container *container, Actor *actor);
	void HandleDoor(Door *door, Actor *actor);
	bool HandleActiveRegion(InfoPoint *trap, Actor *actor);
	void MoveToPointFormation(Actor *actor, Point p, int Orient);
	void InitDialog(Actor* speaker, Actor* target, const char* dlgref);
	void EndDialog(bool try_to_break=false);
	void DialogChoose(unsigned int choose);
	/* Displays a string over an object */
	void DisplayString(Scriptable* target);
	/* Displays a string on screen */
	void DisplayString(Point &p, const char *Text);
	Actor *GetLastActor();
	/* changes map to the current PC */
	void ChangeMap(Actor *pc, bool forced);
	/* returns current area preview for saving a game */
	Sprite2D* GetPreview();
};

#endif
