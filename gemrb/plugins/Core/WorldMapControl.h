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
	/** Allows modification of the scrolling factor from outside */
	void AdjustScrolling(short x, short y);
	/** Draws the Control on the Output Display */
	void Draw(unsigned short x, unsigned short y);
	/** Sets the Text of the current control */
	int SetText(const char* /*string*/, int /*pos*/) { return 0; };
	int ScrollX, ScrollY;
	unsigned short lastMouseX, lastMouseY;
	bool MouseIsDown;
private:
	unsigned char lastCursor;
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
