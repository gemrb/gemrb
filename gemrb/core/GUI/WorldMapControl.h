// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * @file WorldMapControl.h
 * Declares WorldMapControl, widget for displaying world map
 */


#ifndef WORLDMAPCONTROL_H
#define WORLDMAPCONTROL_H

#include "exports.h"

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
