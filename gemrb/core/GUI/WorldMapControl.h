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

namespace GemRB {

class Font;
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

class GEM_EXPORT WorldMapControl : public Control, public View::Scrollable {
private:
	/** Draws the Control on the Output Display */
	void DrawSelf(Region drawFrame, const Region&) override;

public:
	WorldMapControl(const Region& frame, Font *font, int direction);

	/** Allows modification of the scrolling factor from outside */
	void ScrollDelta(const Point& delta) override;
	void ScrollTo(const Point& pos) override;
	/** Sets the exit direction (we need this to calculate distances) */
	void SetDirection(int direction);
	/** Set color for one type of area labels */
	void SetColor(int which, Color color);
	void SetOverrideIconPalette(bool ipOverride) { OverrideIconPalette = ipOverride; };
	Point Pos;
	/** pointer to last pointed area */
	WMPAreaEntry *Area;

protected:
	/** Mouse Over Event */
	bool OnMouseOver(const MouseEvent& /*me*/) override;
	bool OnMouseDrag(const MouseEvent& /*me*/) override;
	/** Mouse Leave Event */
	void OnMouseLeave(const MouseEvent& /*me*/, const DragOp*) override;
	/** Mouse Button Down */
	bool OnMouseDown(const MouseEvent& /*me*/, unsigned short Mod) override;
	/** Mouse Button Up */
	bool OnMouseUp(const MouseEvent& /*me*/, unsigned short Mod) override;
	/** Mouse Wheel Event */
	bool OnMouseWheelScroll(const Point& delta) override;

	bool OnKeyPress(const KeyboardEvent& /*Key*/, unsigned short /*Mod*/) override;

private:
	//font for printing area names
	Font* ftext;
	//current area
	ieResRef currentArea;
	// bg1 needs entry icon recoloring, as the data palettes are a pure bw gradient
	bool OverrideIconPalette;
	/** Label color of a visited area */
	Holder<Palette> pal_normal;
	/** Label color of a currently selected area */
	Holder<Palette> pal_selected;
	/** Label color of a not yet visited area */
	Holder<Palette> pal_notvisited;

};

}

#endif
