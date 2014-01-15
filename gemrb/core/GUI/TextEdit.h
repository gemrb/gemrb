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

#include "RGBAColor.h"
#include "exports.h"

#include "Font.h"

namespace GemRB {

class Palette;

// !!! Keep these synchronized with GUIDefines.py
#define IE_GUI_EDIT_ON_CHANGE      0x03000000
#define IE_GUI_EDIT_ON_DONE        0x03000001
#define IE_GUI_EDIT_ON_CANCEL      0x03000002

//this is stored in 'Value' of Control class
#define IE_GUI_EDIT_NUMBER         1

/**
 * @class TextEdit
 * Widget displaying single line text input field
 */

class GEM_EXPORT TextEdit : public Control {
protected:
	/** Draws the Control on the Output Display */
	void DrawInternal(Region& drawFrame);
	bool HasBackground() { return Back; }
public:
	TextEdit(const Region& frame, unsigned short maxLength, unsigned short x, unsigned short y);
	~TextEdit(void);
	/** Set Font */
	void SetFont(Font* f);
	Font *GetFont();
	/** Set Cursor */
	void SetCursor(Sprite2D* cur);
	/** Set BackGround */
	void SetBackGround(Sprite2D* back);
	/** Sets the Text of the current control */
	void SetText(const char* string);
	/** Gets the Text of the current control */
	const String& QueryText() const;
	/** Sets the buffer length */
	void SetBufferLength(ieWord buflen);
	/** Sets the alignment */
	void SetAlignment(unsigned char Alignment);
private:
	/** Text Editing Cursor Sprite */
	Sprite2D* Cursor;
	/** Text Font */
	Font* font;
	unsigned char Alignment;
	/** Background */
	Sprite2D* Back;
	/** Max Edit Text Length */
	unsigned short max;
	/** Client area position */
	unsigned short FontPosX, FontPosY;
	/** Text Buffer */
	unsigned char* Buffer;
	/** Cursor Position */
	unsigned short CurPos;
	/** Color Palette */
	Palette* palette;
public: //Events
	/** Key Press Event */
	bool OnKeyPress(unsigned char Key, unsigned short Mod);
	/** Special Key Press */
	bool OnSpecialKeyPress(unsigned char Key);
	/** Set handler for specified event */
	bool SetEvent(int eventType, EventHandler handler);
	void SetFocus(bool focus);
	/** OnChange Scripted Event Function Name */
	EventHandler EditOnChange;
	EventHandler EditOnDone;
	EventHandler EditOnCancel;
};

}

#endif
