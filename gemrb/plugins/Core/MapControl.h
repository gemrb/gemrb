class MapControl;

#ifndef MAPCONTROL_H
#define MAPCONTROL_H

#include "Control.h"
#include "Interface.h"

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

class GEM_EXPORT MapControl : public Control {
public:
	int ScrollX, ScrollY;
	unsigned short lastMouseX, lastMouseY;
	bool MouseIsDown;
	bool ConvertToGame;
	// Small map bitmap
	Sprite2D* MapMOS;
	// current map
	Map *MyMap;
	// map flags
	Sprite2D *Flag[8];
	// The MapControl can set the text of this label directly
	Control *LinkedLabel;
	// Size of big map (area) in pixels
	short MapWidth, MapHeight;
	// Size of area viewport. FIXME: hack!
	short ViewWidth, ViewHeight;
	short XCenter, YCenter;

	MapControl(void);
	~MapControl(void);
	/** redraws the control after its associated variable has changed */
	void RedrawMapControl(char *VariableName, unsigned int Sum);
	/** Draws the Control on the Output Display */
	void Draw(unsigned short XWin, unsigned short YWin);
	/** Compute parameters after changes in control's or screen geometry */
	void Realize();
	/** Sets the Text of the current control */
	int SetText(const char* /*string*/, int /*pos*/) { return 0; };

	/** Key Press Event */
	void OnKeyPress(unsigned char Key, unsigned short Mod);
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
};

#endif
