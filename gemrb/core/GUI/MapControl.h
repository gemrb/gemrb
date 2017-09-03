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
 *
 */

/**
 * @file MapControl.h
 * Declares MapControl, widget for displaying current area map
 */

#ifndef MAPCONTROL_H
#define MAPCONTROL_H

#include "GUI/Control.h"

#include "exports.h"

namespace GemRB {

// !!! Keep these synchronized with GUIDefines.py !!!
#define IE_GUI_MAP_ON_PRESS     	0x09000000
#define IE_GUI_MAP_ON_RIGHT_PRESS	0x09000005
class Map;

/**
 * @class MapControl
 * Widget displaying current area map, with a viewport rectangle
 * and PCs' ground circles
 */

class GEM_EXPORT MapControl : public Control {
private:
	Region mosRgn;
	Point notePos;
	
public:
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

	MapControl(const Region& frame);
	~MapControl(void);

	/** Refreshes the control after its associated variable has changed */
	void UpdateState(unsigned int Sum);
	
	bool IsAnimated() const { return true; }

	/** Key Press Event */
	bool OnKeyPress(const KeyboardEvent& Key, unsigned short Mod);
	/** Mouse Over Event */
	void OnMouseOver(const MouseEvent&);
	void OnMouseDrag(const MouseEvent& /*me*/);
	/** Mouse Button Down */
	void OnMouseDown(const MouseEvent& /*me*/, unsigned short Mod);
	/** Mouse Button Up */
	void OnMouseUp(const MouseEvent& /*me*/, unsigned short Mod);

private:
	/** Call event handler on click */
	void ClickHandle();
	/** Move viewport */
	void UpdateViewport(Point p);
	
	void WillDraw();
	/** Draws the Control on the Output Display */
	void DrawSelf(Region drawFrame, const Region& clip);
	void DrawFog(const Region& rgn);
	
	Point ConvertPointToGame(Point) const;
	Point ConvertPointFromGame(Point) const;
};

}

#endif
