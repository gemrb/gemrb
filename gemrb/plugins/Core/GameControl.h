#ifndef GAMECONTROL_H
#define GAMECONTROL_H

#include "Control.h"
#include "Map.h"
#include "PathFinder.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT GameControl : public Control
{
public:
	GameControl(void);
	~GameControl(void);
public:
	/** Draws the Control on the Output Display */
	void Draw(unsigned short x, unsigned short y);
	/** Sets the Text of the current control */
	int SetText(const char * string, int pos = 0);
	/** Sets the current area to Display */
	void SetCurrentArea(int Index);
private:
	int MapIndex;
	ActorBlock * lastActor;
	std::vector<ActorBlock*> selected;
	std::vector<ActorBlock*> highlighted;
	std::vector<InfoPoint*> infoPoints;
	Color * InfoTextPalette;
	bool DrawSelectionRect;
	bool MouseIsDown;
	Region SelectionRect;
	short StartX, StartY;
	Door * overDoor;
	Container * overContainer;
	InfoPoint * overInfoPoint;
	unsigned char lastCursor;
	short moveX, moveY;
	unsigned short lastMouseX, lastMouseY;
	int DebugFlags;
	short pfsX, pfsY;
	PathNode * drawPath;
public: //Events
	/** Key Press Event */
	void OnKeyPress(unsigned char Key, unsigned short Mod);
	/** Key Release Event */
	void OnKeyRelease(unsigned char Key, unsigned short Mod);
	/** Mouse Over Event */
	void OnMouseOver(unsigned short x, unsigned short y);
	/** Mouse Button Down */
	void OnMouseDown(unsigned short x, unsigned short y, unsigned char Button, unsigned short Mod);
	/** Mouse Button Up */
	void OnMouseUp(unsigned short x, unsigned short y, unsigned char Button, unsigned short Mod);
	/** Special Key Press */
	void OnSpecialKeyPress(unsigned char Key);
private:
	void CalculateSelection(unsigned short x, unsigned short y);
};

#endif
