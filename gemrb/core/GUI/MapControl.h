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

#include "exports-core.h"

#include "AnimationFactory.h"

#include "GUI/Control.h"

namespace GemRB {

class Map;
class MapNote;

/**
 * @class MapControl
 * Widget displaying current area map, with a viewport rectangle
 * and PCs' ground circles
 */

class GEM_EXPORT MapControl : public Control {
private:
	enum NOTE_STATE : Control::value_t {
		NO_NOTES = 0,
		VIEW_NOTES = 1,
		SET_NOTE = 2,
		REVEAL = 3,
		EDIT_NOTE = 4
	};

	Region mosRgn;
	Point notePos;

	std::shared_ptr<const AnimationFactory> mapFlags;

public:
	// Small map bitmap
	Holder<Sprite2D> MapMOS;
	// current map
	Map* MyMap = nullptr;
	// The MapControl can set the text of this label directly
	Control* LinkedLabel = nullptr;

	MapControl(const Region& frame, std::shared_ptr<const AnimationFactory> af);

	bool IsAnimated() const override { return true; } // map must constantly update actor positions

	void UpdateState(value_t) override;

private:
	/** Call event handler on click */
	void ClickHandle(const MouseEvent&) const;
	/** Move viewport */
	void UpdateViewport(Point p);
	void UpdateCursor();
	void UpdateMap();

	const MapNote* MapNoteAtPoint(const Point& p) const;

	Region GetViewport() const;
	void WillDraw(const Region& /*drawFrame*/, const Region& /*clip*/) override;
	/** Draws the Control on the Output Display */
	void DrawSelf(const Region& drawFrame, const Region& clip) override;
	void DrawFog(const Region& rgn) const;

	Point ConvertPointToGame(Point) const;
	Point ConvertPointFromGame(Point) const;

	/** Key Press Event */
	bool OnKeyPress(const KeyboardEvent& Key, unsigned short Mod) override;
	/** Mouse Over Event */
	bool OnMouseOver(const MouseEvent&) override;
	bool OnMouseDrag(const MouseEvent& /*me*/) override;
	/** Mouse Button Down */
	bool OnMouseDown(const MouseEvent& /*me*/, unsigned short Mod) override;
	/** Mouse Button Up */
	bool OnMouseUp(const MouseEvent& /*me*/, unsigned short Mod) override;
};

}

#endif
