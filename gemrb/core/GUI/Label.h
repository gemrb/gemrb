// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * @file Label.h
 * Declares Label widget for displaying static texts
 * @author GemRB Development Team
 */

#ifndef LABEL_H
#define LABEL_H

#include "exports.h"

#include "GUI/Control.h"
#include "GUI/TextSystem/Font.h"

namespace GemRB {

struct Color;

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

	Label(const Region& frame, Holder<Font> font, const String& string);

	/** This function sets the actual Label Text */
	void SetText(String string) override;
	/** Sets the Foreground Font Color */
	void SetColors(const Color& col, const Color& bg);
	/** Set the font being used */
	void SetFont(Holder<Font> newFont) { font = std::move(newFont); }
	/** Sets the Alignment of Text */
	void SetAlignment(unsigned char newAlignment);
	/** Simply returns the pointer to the text, don't modify it! */
	String QueryText() const override;

private: // Private attributes
	/** Text String Buffer */
	String Text;
	/** Font for Text Writing */
	Holder<Font> font;
	Font::PrintColors colors;

	/** Alignment Variable */
	unsigned char Alignment;
};

}

#endif
