// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * @file Progressbar.h
 * Declares Progressbar widget for displaying progress of loading and saving games
 */

#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H

#include "exports.h"

#include "Sprite2D.h"

#include "GUI/Control.h"

namespace GemRB {

class Animation;

/**
 * @class Progressbar
 * Widget for displaying progressbars, mainly on loading/saving screens
 */

class GEM_EXPORT Progressbar : public Control {
private:
	/** Draws the Control on the Output Display */
	void DrawSelf(const Region& drawFrame, const Region& clip) override;

public:
	struct Action {
		// !!! Keep these synchronized with GUIDefines.py !!!
		static const Control::Action EndReached = ACTION_CUSTOM(0); // progress bar reaches 100%
	};

	Progressbar(const Region& frame, unsigned short KnobStepsCount);
	Progressbar(const Progressbar&) = delete;
	Progressbar& operator=(const Progressbar&) = delete;

	bool IsOpaque() const override;

	void SetImages(Holder<Sprite2D> bg, Holder<Sprite2D> cap);
	/** Sets a bam resource for progressbar */
	void SetAnimation(Holder<Animation> arg);
	/** Sets the mos coordinates for the progressbar filler mos/cap */
	void SetSliderPos(const Point& knob, const Point& cap);
	/** Refreshes a progressbar which is associated with VariableName */
	void UpdateState(value_t) override;

private: // Private attributes
	Holder<Sprite2D> BackGround2; //mos resource for the filling of the bar
	/** Knob Steps Count */
	unsigned int KnobStepsCount;
	Point KnobPos; //relative coordinates for Background2
	Point CapPos; //relative coordinates for PBarCap

	/** The mos for the progressbar cap (linear progressbar) */
	Holder<Sprite2D> PBarCap;
	/** The bam cycle whose frames work as a progressbar (animated progressbar) */
	Holder<Animation> PBarAnim;
};

}

#endif
