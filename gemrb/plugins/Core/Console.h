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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Console.h,v 1.3 2003/11/25 13:48:02 balrog994 Exp $
 *
 */

#ifndef CONSOLE_H
#define CONSOLE_H

#include "Control.h"
#include "TextArea.h"

class Console :
	public Control
{
public:
	Console(void);
	~Console(void);
	/** Draws the Console on the Output Display */
	void Draw(unsigned short x, unsigned short y);
	/** Set Font */
	void SetFont(Font * f);
	/** Set Cursor */
	void SetCursor(Sprite2D * cur);
	/** Set BackGround */
	void SetBackGround(Sprite2D * back);
	/** Sets the Text of the current control */
	int SetText(const char * string, int pos = 0);
	/** Output for Script Messages */
	TextArea * ta;
private:
	/** Text Editing Cursor Sprite */
	Sprite2D * Cursor;
	/** Text Font */
	Font * font;
	/** Background */
	Sprite2D * Back;
	/** Max Edit Text Length */
	unsigned short max;
	/** Text Buffer */
	unsigned char * Buffer;
	/** Cursor Position */
	unsigned short CurPos;
	/** Color Palette */
	Color * palette;
	
public: //Events
	/** Key Press Event */
	void OnKeyPress(unsigned char Key, unsigned short Mod);
	/** Special Key Press */
	void OnSpecialKeyPress(unsigned char Key);
};

#endif
