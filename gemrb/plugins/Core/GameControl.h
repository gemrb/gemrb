#ifndef GAMECONTROL_H
#define GAMECONTROL_H

#include "Control.h"
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
	void SetCurrentArea(Map * map)
	{
		area = map;
	}
private:
	Map * area;
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
};

#endif
