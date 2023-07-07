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
#include "Map.h"

#include <vector>

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
#define SF_CENTERONACTOR 1  //
#define SF_ALWAYSCENTER  2
#define SF_CUTSCENE      4 //don't push new actions onto the action queue

// target modes and types
// !!! Keep these synchronized with GUIDefines.py !!!
#define TARGET_MODE_NONE    0
#define TARGET_MODE_TALK    1
#define TARGET_MODE_ATTACK  2
#define TARGET_MODE_CAST    3
#define TARGET_MODE_DEFEND  4
#define TARGET_MODE_PICK    5

/**
 * @class GameControl
 * Widget displaying areas, where most of the game 'happens'.
 * It allows for interacting with PCs, NPCs and the rest of the world.
 * It's also a very core part of GemRB, as some processes are driven from it.
 * It's always assigned Control index 0.
 */

class GEM_EXPORT GameControl : public View {
private:
	ieDword lastActorID = 0;
	ieDword trackerID = 0;
	ieDword distance = 0;  //tracking distance
	std::vector<Actor*> highlighted;

	bool isSelectionRect = false;
	bool isFormationRotation = false;

	// mouse coordinates represented in game coordinates
	Point gameClickPoint;
	Point screenMousePos;
	Point vpOrigin;
	bool updateVPTimer = true;
	double formationBaseAngle = 0.0;

	// currently selected targeting type, such as talk, attack, cast, ...
	// private to enforce proper cursor changes
	int target_mode = TARGET_MODE_NONE;
	int lastCursor = 0;
	Point vpVector;
	int numScrollCursor = 0;
	PathListNode* drawPath = nullptr;
	unsigned int ScreenFlags = SF_CENTERONACTOR;
	unsigned int DialogueFlags = DF_FREEZE_SCRIPTS;
	String DisplayText;
	unsigned int DisplayTextTime = 0;
	bool AlwaysRun = false;
	Actor *user = nullptr; //the user of item or spell

	Door* overDoor = nullptr;
	Container* overContainer = nullptr;
	InfoPoint* overInfoPoint = nullptr;

	EventMgr::TapMonitorId eventMonitors[2];

public:
	static uint32_t DebugFlags;
	static uint8_t DebugPropVal;

	DialogHandler *dialoghandler = nullptr;
	//the name of the spell to cast
	ResRef spellName;
	//using spell or item
	int spellOrItem = 0; // -1 = item, otherwise the spell type
	//the user of spell or item
	Actor *spellUser = nullptr;
	int spellSlot = 0; // or inventorySlot/itemHeader
	int spellIndex = 0;
	int spellCount = 0; //multiple targeting
	// allow targeting allies, enemies and/or neutrals (bitmask)
	int target_types = 0;

private:
	using FormationPoints = std::vector<Point>;
	Map* CurrentArea() const;

	Region SelectionRect() const;
	/** Draws an arrow on the edge of the screen based on the point (points at offscreen actors) */
	void DrawArrowMarker(const Point& p, const Color& color) const;
	void DrawFormation(const std::vector<Actor*>& actors, const Point& formationPoint, double angle) const;
	
	Point GetFormationPoint(const Point& origin, size_t pos, double angle, const FormationPoints& exclude = FormationPoints()) const;
	FormationPoints GetFormationPoints(const Point& origin, const std::vector<Actor*>& actors, double angle) const;
	
	void Scroll(const Point& amt);

	void HandleContainer(Container *container, Actor *actor);
	void HandleDoor(Door *door, Actor *actor);

	void UpdateCursor();
	bool IsDisabledCursor() const override;

	void PerformSelectedAction(const Point& p);
	void CommandSelectedMovement(const Point& p, bool append = false, bool tryToRun = false) const;

	bool OnGlobalMouseMove(const Event&);

	/** Draws the Control on the Output Display */
	void DrawSelf(const Region& drawFrame, const Region& clip) override;
	void WillDraw(const Region& /*drawFrame*/, const Region& /*clip*/) override;
	
	bool CanLockFocus() const override { return true; };
	void FlagsChanged(unsigned int /*oldflags*/) override;
	
	void DebugPaint(const Point& p, bool sample) const noexcept;

public:
	explicit GameControl(const Region& frame);
	~GameControl(void) override;

	// GameControl always needs to redraw unless we arent in a game (disabled)
	bool IsAnimated() const override { return !IsDisabled(); }
	void DrawTargetReticle(uint16_t size, const Color& color, const Point& p) const;
	/** Draws the target reticle for Actor movement. */
	void DrawTargetReticle(const Movable* target, const Point& point) const;
	/** Sets multiple quicksaves flag*/
	//static void MultipleQuickSaves(int arg);
	void SetTracker(const Actor *actor, ieDword dist);

	void DrawTooltip(const Point& p) const;
	String TooltipText() const override;

	void SetTargetMode(int mode);
	int GetTargetMode() const { return target_mode; }
	bool SetScreenFlags(unsigned int value, BitOp mode);
	void SetDialogueFlags(unsigned int value, BitOp mode);
	int GetScreenFlags() const { return ScreenFlags; }
	int GetDialogueFlags() const { return DialogueFlags; }
	bool InDialog() const { return DialogueFlags & DF_IN_DIALOG; }
	void SetDisplayText(const String& text, unsigned int time);
	void SetDisplayText(HCStrings text, unsigned int time);
	void ClearMouseState();
	Point GameMousePos() const;

	void MoveViewportUnlockedTo(Point, bool center);
	bool MoveViewportTo(Point, bool center, int speed = 0);
	Region Viewport() const;

	/** Selects one or all PC */
	void SelectActor(int whom, int type = -1);
	void SetCutSceneMode(bool active);
	void TryToAttack(Actor *source, const Actor *target) const;
	void TryToCast(Actor *source, const Point &p);
	void TryToCast(Actor *source, const Actor *target);
	void TryToDefend(Actor *source, const Actor *target) const;
	void TryToTalk(Actor *source, const Actor *target) const;
	void TryToPick(Actor *source, const Scriptable *tgt) const;
	void TryToDisarm(Actor *source, const InfoPoint *tgt) const;
	void PerformActionOn(Actor *actor);
	void ResetTargetMode();
	void UpdateTargetMode();

	// returns the default cursor fitting the targeting mode
	Holder<Sprite2D> GetTargetActionCursor() const;
	Holder<Sprite2D> Cursor() const override;

	bool HandleActiveRegion(InfoPoint *trap, Actor *actor, const Point& p);

	void MakeSelection(bool extend = false);
	void InitFormation(const Point&, bool rotating);
	Point GetFormationOffset(size_t formation, uint8_t pos) const;
	/** calls MoveToPoint or RunToPoint */
	void CreateMovement(Actor *actor, const Point &p, bool append = true, bool tryToRun = false) const;
	bool ShouldTriggerWorldMap(const Actor *pc) const;
	/** checks if the actor should be running instead of walking */
	bool CanRun(const Actor *actor) const;
	bool ShouldRun(const Actor *actor) const;
	/** Displays a string over an object */
	void DisplayString(Scriptable* target) const;
	/** Displays a string on screen */
	void DisplayString(const Point &p, const char *Text);
	Actor *GetLastActor() const;
	void SetLastActor(Actor* lastActor);
	/** changes map to the current PC */
	void ChangeMap(const Actor *pc, bool forced);
	/** Sets up targeting with spells or items */
	void SetupItemUse(int slot, size_t header, Actor *actor, int targettype, int cnt);
	/** Page is the spell type + spell level info */
	void SetupCasting(const ResRef& spellname, int type, int level, int slot, Actor *actor, int targettype, int cnt);
	void ToggleAlwaysRun();
	int GetOverheadOffset() const;
	void TryDefaultTalk() const;

protected:
	//Events
	/** Key Press Event */
	bool OnKeyPress(const KeyboardEvent& Key, unsigned short Mod) override;
	/** Key Release Event */
	bool OnKeyRelease(const KeyboardEvent& Key, unsigned short Mod) override;
	/** Mouse Over Event */
	bool OnMouseOver(const MouseEvent&) override;
	bool OnMouseDrag(const MouseEvent& /*me*/) override;

	bool OnTouchDown(const TouchEvent& /*te*/, unsigned short /*Mod*/) override;
	bool OnTouchUp(const TouchEvent& /*te*/, unsigned short /*Mod*/) override;
	bool OnTouchGesture(const GestureEvent& gesture) override;

	/** Currently only deals with the GEM_TAB exception */
	bool DispatchEvent(const Event& event) const;
	
	/** Mouse Button Down */
	bool OnMouseDown(const MouseEvent& /*me*/, unsigned short Mod) override;
	/** Mouse Button Up */
	bool OnMouseUp(const MouseEvent& /*me*/, unsigned short Mod) override;
	bool OnMouseWheelScroll(const Point& delta) override;
	
	bool OnControllerButtonDown(const ControllerEvent& ce) override;
};

}

#endif
