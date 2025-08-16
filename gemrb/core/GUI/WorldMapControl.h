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

#include "exports-core.h"

#include "GUI/Control.h"
#include "GUI/GUIAnimation.h"

namespace GemRB {

class Font;
class WMPAreaEntry;

/**
 * @class WorldMapControl
 * Widget displaying "world" map, with particular locations and possibly
 * allowing travelling between areas.
 */

class GEM_EXPORT WorldMapControl : public Control,
				   public View::Scrollable {
private:
	/** Draws the Control on the Output Display */
	void WillDraw(const Region& /*drawFrame*/, const Region& /*clip*/) override;
	void DrawSelf(const Region& drawFrame, const Region&) override;

public:
	WorldMapControl(const Region& frame, Holder<Font> font);
	WorldMapControl(const Region& frame, Holder<Font> font, const Color& normal, const Color& selected, const Color& notvisited);

	/** Allows modification of the scrolling factor from outside */
	void ScrollDelta(const Point& delta) override;
	void ScrollTo(const Point& pos) override;

	Point Pos;
	/** pointer to last pointed area */
	WMPAreaEntry* Area = nullptr;
	Holder<Sprite2D> areaIndicator;
	ColorAnimation hoverAnim;

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
	Holder<Font> ftext;
	//current area
	ResRef currentArea;

	/** Label color of a visited area */

	Color color_normal;
	/** Label color of a currently selected area */
	Color color_selected;
	/** Label color of a not yet visited area */
	Color color_notvisited;
};

}

#endif
