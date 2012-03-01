/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

/**
 * @file WorldMapControl.h
 * Declares WorldMapControl, widget for displaying world map
 */


#ifndef WORLDMAPCONTROL_H
#define WORLDMAPCONTROL_H

#include "GUI/Control.h"

#include "exports.h"

#include "Dialog.h"
#include "Interface.h"

namespace GemRB {

class Palette;
class WMPAreaEntry;
class WorldMapControl;

// !!! Keep these synchronized with GUIDefines.py !!!
/** Which label color is set with SetColor() */
#define IE_GUI_WMAP_COLOR_BACKGROUND  0
#define IE_GUI_WMAP_COLOR_NORMAL      1
#define IE_GUI_WMAP_COLOR_SELECTED    2
#define IE_GUI_WMAP_COLOR_NOTVISITED  3


/**
 * @class WorldMapControl
 * Widget displaying "world" map, with particular locations and possibly
 * allowing travelling between areas.
 */

#define IE_GUI_WORLDMAP_ON_PRESS   0x08000000
#define IE_GUI_MOUSE_ENTER_WORLDMAP  0x08000002

class GEM_EXPORT WorldMapControl : public Control {
public:
	WorldMapControl(const char *fontname, int direction);
	~WorldMapControl(void);

	/** Allows modification of the scrolling factor from outside */
	void AdjustScrolling(short x, short y);
	/** Draws the Control on the Output Display */
	void Draw(unsigned short x, unsigned short y);
	/** Sets the exit direction (we need this to calculate distances) */
	void SetDirection(int direction);
	/** Set color for one type of area labels */
	void SetColor(int which, Color color);
	int ScrollX, ScrollY;
	unsigned short lastMouseX, lastMouseY;
	bool MouseIsDown;
	/** pointer to last pointed area */
	WMPAreaEntry *Area;
	/** Set handler for specified event */
	bool SetEvent(int eventType, EventHandler handler);
private:
	//font for printing area names
	Font* ftext;
	//mouse cursor
	unsigned char lastCursor;
	//current area
	ieResRef currentArea;
	/** Label color of a visited area */
	Palette *pal_normal;
	/** Label color of a currently selected area */
	Palette *pal_selected;
	/** Label color of a not yet visited area */
	Palette *pal_notvisited;
	/** guiscript Event when button pressed */
	EventHandler WorldMapControlOnPress;
	/** guiscript Event when mouse is over a reachable area */
	EventHandler WorldMapControlOnEnter;

	/** Mouse Over Event */
	void OnMouseOver(unsigned short x, unsigned short y);
	/** Mouse Leave Event */
	void OnMouseLeave(unsigned short x, unsigned short y);
	/** Mouse Button Down */
	void OnMouseDown(unsigned short x, unsigned short y, unsigned short Button,
		unsigned short Mod);
	/** Mouse Button Up */
	void OnMouseUp(unsigned short x, unsigned short y, unsigned short Button,
		unsigned short Mod);
	/** Mouse Wheel Event */
	void OnMouseWheelScroll(short x, short y);
	/** Key Release Event */
	void OnKeyRelease(unsigned char Key, unsigned short Mod);
	/** Special Key Press */
	void OnSpecialKeyPress(unsigned char Key);
	/** DisplayTooltip */
	void DisplayTooltip();
};

}

#endif
