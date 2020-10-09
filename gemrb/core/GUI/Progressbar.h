/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2005 The GemRB Project
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
 * @file Progressbar.h
 * Declares Progressbar widget for displaying progress of loading and saving games
 */

#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H

#include "GUI/Control.h"

#include "exports.h"

#include "Animation.h"
#include "Sprite2D.h"

namespace GemRB {

/**
 * @class Progressbar
 * Widget for displaying progressbars, mainly on loading/saving screens
 */

class GEM_EXPORT Progressbar : public Control  {
private:
	/** Draws the Control on the Output Display */
	void DrawSelf(Region drawFrame, const Region& clip);

public:
	struct Action {
		// !!! Keep these synchronized with GUIDefines.py !!!
		static const Control::Action EndReached = ACTION_CUSTOM(0); // progress bar reaches 100%
	};

	Progressbar(const Region& frame, unsigned short KnobStepsCount);
	~Progressbar();

	bool IsOpaque() const { return BackGround && BackGround->HasTransparency() == false; }

	/** Sets the background images */
	void SetImage(Holder<Sprite2D> img, Holder<Sprite2D> img2);
	/** Sets a bam resource for progressbar */
	void SetAnimation(Animation *arg);
	/** Sets a mos resource for progressbar cap */
	void SetBarCap(Holder<Sprite2D> img3);
	/** Sets the mos coordinates for the progressbar filler mos/cap */
	void SetSliderPos(int x, int y, int x2, int y2);
	/** Refreshes a progressbar which is associated with VariableName */
	void UpdateState(unsigned int Sum);

private: // Private attributes
	/** BackGround Images. If smaller than the Control Size, the image will be tiled. */
	Holder<Sprite2D> BackGround;
	Holder<Sprite2D> BackGround2; //mos resource for the filling of the bar
	/** Knob Steps Count */
	unsigned int KnobStepsCount;
	int KnobXPos, KnobYPos; //relative coordinates for Background2
	int CapXPos, CapYPos; //relative coordinates for PBarCap

	/** The mos for the progressbar cap (linear progressbar) */
	Holder<Sprite2D> PBarCap;
	/** The bam cycle whose frames work as a progressbar (animated progressbar) */
	Animation *PBarAnim;

};

}

#endif
