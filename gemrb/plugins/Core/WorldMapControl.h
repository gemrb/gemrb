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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/WorldMapControl.h,v 1.10 2005/11/24 17:44:09 wjpalenstijn Exp $
 *
 */

/**
 * @file WorldMapControl.h
 * Declares WorldMapControl, widget for displaying world map
 */

class WorldMapControl;
class WMPAreaEntry;

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

/**
 * @class WorldMapControl
 * Widget displaying "world" map, with particular locations and possibly
 * allowing travelling between areas.
 */

class GEM_EXPORT WorldMapControl : public Control {
public:
	WorldMapControl(void);
	~WorldMapControl(void);
public:
	/** Allows modification of the scrolling factor from outside */
	void AdjustScrolling(short x, short y);
	/** Draws the Control on the Output Display */
	void Draw(unsigned short x, unsigned short y);
	/** Sets the exit direction (we need this to calculate distances) */
	void SetDirection(int direction);
	/** Sets the Text of the current control */
	int SetText(const char* /*string*/, int /*pos*/) { return 0; };  
	int ScrollX, ScrollY;
	unsigned short lastMouseX, lastMouseY;
	bool MouseIsDown;
	/** the palette for drawing the area titles */
	Color *text_pal;
	/** pointer to last pointed area */
	WMPAreaEntry *Area;
private:
	unsigned char lastCursor;
	/** Mouse Over Event */
	void OnMouseOver(unsigned short x, unsigned short y);
	/** Mouse Leave Event */
	void OnMouseLeave(unsigned short x, unsigned short y);
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
