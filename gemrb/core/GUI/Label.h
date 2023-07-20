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
 * @file Label.h
 * Declares Label widget for displaying static texts
 * @author GemRB Developement Team
 */

#ifndef LABEL_H
#define LABEL_H

#include "GUI/Control.h"
#include "GUI/TextSystem/Font.h"

#include "RGBAColor.h"
#include "exports.h"

namespace GemRB {

class Palette;

/**
 * @class Label
 * Label widget for displaying static texts in the GUI
 */

class GEM_EXPORT Label : public Control {
private:
	/** Draws the Control on the Output Display */
	void DrawSelf(const Region& drawFrame, const Region& clip) override;

public:
	enum LabelFlags {
		UseColor = 1 // when set the label is prented with the PrintColors
	};

	Label(const Region& frame, Font* font, const String& string);

	/** This function sets the actual Label Text */
	void SetText(String string) override;
	/** Sets the Foreground Font Color */
	void SetColors(const Color& col, const Color& bg);
	/** Set the font being used */
	void SetFont(Font* newFont) { font = newFont; }
	/** Sets the Alignment of Text */
	void SetAlignment(unsigned char newAlignment);
	/** Simply returns the pointer to the text, don't modify it! */
	String QueryText() const override;

private: // Private attributes
	/** Text String Buffer */
	String Text;
	/** Font for Text Writing */
	Font* font;
	Font::PrintColors colors;

	/** Alignment Variable */
	unsigned char Alignment;
};

}

#endif
