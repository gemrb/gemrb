/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

/**
 * @file TextEdit.h
 * Declares TextEdit widget for displaying single line text input field
 * @author The GemRB Project
 */

#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include "GUI/Control.h"
#include "GUI/TextSystem/Font.h"

#include "RGBAColor.h"
#include "exports.h"

namespace GemRB {

class Palette;

//this is stored in 'Value' of Control class
#define IE_GUI_EDIT_NUMBER         1

/**
 * @class TextEdit
 * Widget displaying single line text input field
 */

class GEM_EXPORT TextEdit : public Control {
private:
    /** Text Editing Cursor Sprite */
    Sprite2D* Cursor;
    /** Text Font */
    Font* font;
    unsigned char Alignment;
    
    /** Max Edit Text Length */
    unsigned short max;
    /** Client area position */
    Point FontPos;
    /** Text Buffer */
    String Text;
    /** Cursor Position */
    unsigned short CurPos;
    /** Color Palette */
    Palette* palette;

private:
	/** Draws the Control on the Output Display */
	void DrawSelf(Region drawFrame, const Region& clip);

public:
	struct Action {
		// !!! Keep these synchronized with GUIDefines.py !!!
		static const Control::Action Change = Control::ValueChange; // text change event (keyboard, etc)
		static const Control::Action Done = ACTION_CUSTOM(0);
		static const Control::Action Cancel = ACTION_CUSTOM(1);
	};

	TextEdit(const Region& frame, unsigned short maxLength, Point p);
	~TextEdit(void);

	/** Set Font */
	void SetFont(Font* f);
	Font *GetFont();
	/** Set Cursor */
	void SetCursor(Sprite2D* cur);

	/** Sets the Text of the current control */
	void SetText(const String& string);
	/** Gets the Text of the current control */
	String QueryText() const;
	/** Sets the buffer length */
	void SetBufferLength(ieWord buflen);
	/** Sets the alignment */
	void SetAlignment(unsigned char Alignment);
    
    /** Key Press Event */
    bool OnKeyPress(const KeyboardEvent& Key, unsigned short Mod);
    
    void SetFocus();
};

}

#endif
