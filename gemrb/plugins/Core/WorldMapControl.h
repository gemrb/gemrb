class WorldMapControl;

#ifndef WORLDMAPCONTROL_H
#define WORLDMAPCONTROL_H

#include "Control.h"
#include "Interface.h"
#include "Dialog.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

#ifndef _DEBUG
#define _DEBUG
#endif

class GEM_EXPORT WorldMapControl : public Control {
public:
	WorldMapControl(void);
	~WorldMapControl(void);
public:
	/** Draws the Control on the Output Display */
	void Draw(unsigned short x, unsigned short y);
	/** Sets the Text of the current control */
	int SetText(const char* string, int pos = 0) { return 0; };
	int ScrollX, ScrollY;
	unsigned short lastMouseX, lastMouseY;
	bool MouseIsDown;
#if 0
	/** Sets the current area to Display */
	Map *SetCurrentArea(int Index);
	/** Sets the InfoTextColor, used in PST
	*/
	void SetInfoTextColor(Color color);
	Color* InfoTextPalette;
	bool DrawSelectionRect;
	Region SelectionRect;
	short StartX, StartY;
	int action;
private:
	unsigned char lastCursor;
	short moveX, moveY;
	int DebugFlags;
	short pfsX, pfsY;
	PathNode* drawPath;
	unsigned long AIUpdateCounter;
	int ScreenFlags;
//	bool GUIEnabled;
public: //Events
	/** Key Press Event */
	void OnKeyPress(unsigned char Key, unsigned short Mod);
#endif
	/** Mouse Over Event */
	void OnMouseOver(unsigned short x, unsigned short y);
	/** Mouse Button Down */
	void OnMouseDown(unsigned short x, unsigned short y, unsigned char Button,
		unsigned short Mod);
	/** Mouse Button Up */
	void OnMouseUp(unsigned short x, unsigned short y, unsigned char Button,
		unsigned short Mod);
	/** Key Release Event */
	void OnKeyRelease(unsigned char Key, unsigned short Mod);
	/** Special Key Press */
	void OnSpecialKeyPress(unsigned char Key);
#if 0
private:
	void CalculateSelection(unsigned short x, unsigned short y);
	void ResizeDel(Window* win, unsigned char type);
	void ResizeAdd(Window* win, unsigned char type);
	void ReadFormations();
	unsigned char LeftCount, BottomCount, RightCount, TopCount;
	DialogState* ds;
	Dialog* dlg;
public:
	Actor* speaker, * target;
	int DialogueFlags;
public:
	void DeselectAll();
	/* Selects one or all PC */
	void SelectActor(int whom);
	void SetCutSceneMode(bool active);
	void HideGUI();
	void UnhideGUI();
	void TryToAttack(Actor *source, Actor *target);
	void TryToTalk(Actor *source, Actor *target);
	void HandleDoor(Door *door, Actor *actor);
	bool HandleActiveRegion(InfoPoint *trap, Actor *actor);
	void MoveToPointFormation(Actor *actor, int WorldmapX, int WorldmapY);
	void InitDialog(Actor* speaker, Actor* target, const char* dlgref);
	void EndDialog(bool try_to_break=false);
	void DialogChoose(unsigned int choose);
	/** finds the first true condition in a dialog */
	int FindFirstState(Scriptable* target, Dialog* dlg);
	void DisplayString(Scriptable* target);
	/* Displays a string in the textarea */
	void DisplayString(const char* Text);
	/* Displays a string on screen */
	void DisplayString(int X, int Y, const char *Text);
	Actor *GetLastActor() { return lastActor; }
	//changes map to the current PC
	void ChangeMap(Actor *pc, bool forced);
private:
	/* evaluates a dialog trigger block */
	bool EvaluateDialogTrigger(Scriptable* target, DialogString* trigger);
#endif
};

#endif
