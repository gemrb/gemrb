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
	MapControl(void);
	~MapControl(void);
	/** Draws the Control on the Output Display */
	void Draw(unsigned short x, unsigned short y);
	/** Sets the Text of the current control */
	int SetText(const char* string, int pos = 0) { return 0; };
	int ScrollX, ScrollY;
	unsigned short lastMouseX, lastMouseY;
	bool MouseIsDown;
	Sprite2D* MapMOS;

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
