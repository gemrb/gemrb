// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * @file MapControl.h
 * Declares MapControl, widget for displaying current area map
 */

#ifndef MAPCONTROL_H
#define MAPCONTROL_H

#include "exports.h"

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
