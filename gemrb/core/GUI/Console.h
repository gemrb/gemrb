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
 * @file Console.h
 * Declares Console widget, input field for direct poking into GemRB innards.
 * @author The GemRB Project
 */

#ifndef CONSOLE_H
#define CONSOLE_H

#include "GUI/Control.h"
#include "GUI/TextArea.h"

namespace GemRB {

class Palette;

/**
 * @class Console
 * Widget displaying debugging console, input field for direct poking
 * into GemRB innards.
 * The console accepts and executes python statements and has already
 * GemRB python module loaded, so almost any command
 * from GUIScripts can be used.
 */

/** the number of remembered lines in the cheat console*/
#define HISTORY_SIZE 5

class Console : public Control {
public:
	Console(void);
	~Console(void);
	/** Draws the Console on the Output Display */
	void Draw(unsigned short x, unsigned short y);
	/** Set Font */
	void SetFont(Font* f);
	/** Set Cursor */
	void SetCursor(Sprite2D* cur);
	/** Set BackGround */
	void SetBackGround(Sprite2D* back);
	/** Sets the Text of the current control */
	void SetText(const char* string);
private:
	/** Text Editing Cursor Sprite */
	Sprite2D* Cursor;
	/** Text Font */
	Font* font;
	/** Background */
	Sprite2D* Back;
	/** Max Edit Text Length */
	unsigned short max;
	/** Text Buffer */
	unsigned char* Buffer;
	/** History Buffer */
	unsigned char* History[HISTORY_SIZE];
	/** Cursor Position */
	unsigned short CurPos;
	/** History Position and size */
	unsigned short HistPos, HistMax;
	/** Color Palette */
	Palette* palette;

public: //Events
	/** Key Press Event */
	bool OnKeyPress(unsigned char Key, unsigned short Mod);
	/** Special Key Press */
	bool OnSpecialKeyPress(unsigned char Key);
	void SetFocus(bool focus);
	bool SetEvent(int eventType, EventHandler handler);
private:
	void HistoryBack();
	void HistoryForward();
	void HistoryAdd(bool force);
};

}

#endif
