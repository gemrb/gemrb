class GameControl;

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

#ifndef _DEBUG
#define _DEBUG
#endif

//dialog flags
#define DF_IN_DIALOG   1
#define DF_TALKCOUNT   2
#define DF_UNBREAKABLE 4

//screen flags
#define SF_DISABLEMOUSE  1
#define SF_CENTERONACTOR 2
#define SF_ALWAYSCENTER  4
#define SF_GUIENABLED    8

class GEM_EXPORT GameControl : public Control {
public:
	GameControl(void);
	~GameControl(void);
public:
	/** Draws the Control on the Output Display */
	void Draw(unsigned short x, unsigned short y);
	/** Sets the Text of the current control */
	int SetText(const char* string, int pos = 0);
	/** Sets the current area to Display */
	Map *SetCurrentArea(int Index);
	/** Sets the InfoTextColor, used in PST
	*/
	void SetInfoTextColor(Color color);
private:
	Actor* lastActor;
	std::vector< Actor*> selected;
	std::vector< Actor*> highlighted;
	std::vector< Scriptable*> infoTexts;
	Color* InfoTextPalette;
	bool DrawSelectionRect;
	bool MouseIsDown;
	Region SelectionRect;
	short StartX, StartY;
	int action;
public:
	char HotKey;
	Door* overDoor;
	Container* overContainer;
	InfoPoint* overInfoPoint;
private:
	unsigned char lastCursor;
	short moveX, moveY;
	unsigned short lastMouseX, lastMouseY;
	int DebugFlags;
	short pfsX, pfsY;
	PathNode* drawPath;
	unsigned long AIUpdateCounter;
	int ScreenFlags;
//	bool GUIEnabled;
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
	void MoveToPointFormation(Actor *actor, int GameX, int GameY);
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
private:
	void ChangeMap();
	/* evaluates a dialog trigger block */
	bool EvaluateDialogTrigger(Scriptable* target, DialogString* trigger);
};

#endif
