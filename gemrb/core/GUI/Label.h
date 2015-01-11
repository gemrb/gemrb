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

// !!! Keep these synchronized with GUIDefines.py !!!
#define IE_GUI_LABEL_ON_PRESS      0x06000000

/**
 * @class Label
 * Label widget for displaying static texts in the GUI
 */

class GEM_EXPORT Label : public Control {
protected:
	/** Draws the Control on the Output Display */
	void DrawInternal(Region& drawFrame);
	bool HasBackground() { return false; }
public: 
	Label(const Region& frame, Font* font, const String& string);
	~Label();
	/** This function sets the actual Label Text */
	using Control::SetText;
	void SetText(const String& string);
	/** Sets the Foreground Font Color */
	void SetColor(Color col, Color bac);
	/** Set the font being used */
	void SetFont(Font *font) { this->font = font; }
	/** Sets the Alignment of Text */
	void SetAlignment(unsigned char Alignment);
	/** Simply returns the pointer to the text, don't modify it! */
	String QueryText() const;

	/** Mouse Button Down */
	void OnMouseUp(unsigned short x, unsigned short y, unsigned short Button,
		unsigned short Mod);
	/** Set handler for specified event */
	bool SetEvent(int eventType, ControlEventHandler handler);
	/** Use the RGB Color for the Font */
	bool useRGB;
	/** OnPress Scripted Event Function Name */
	ControlEventHandler LabelOnPress;
private: // Private attributes
	/** Text String Buffer */
	String Text;
	/** Font for Text Writing */
	Font* font;
	/** Foreground & Background Colors */
	Palette* palette;

	/** Alignment Variable */
	unsigned char Alignment;
};

}

#endif
