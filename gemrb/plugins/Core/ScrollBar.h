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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/ScrollBar.h,v 1.13 2004/10/09 16:31:07 avenger_teambg Exp $
 *
 */

#ifndef SCROLLBAR_H
#define SCROLLBAR_H

#include "Control.h"
#include "TextArea.h"
#include "Sprite2D.h"

#define IE_GUI_SCROLLBAR_UP_UNPRESSED   0
#define IE_GUI_SCROLLBAR_UP_PRESSED 	1
#define IE_GUI_SCROLLBAR_DOWN_UNPRESSED 2
#define IE_GUI_SCROLLBAR_DOWN_PRESSED   3
#define IE_GUI_SCROLLBAR_TROUGH 		4
#define IE_GUI_SCROLLBAR_SLIDER 		5

#define UP_PRESS	 0x0001
#define DOWN_PRESS   0x0010
#define SLIDER_GRAB  0x0100

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT ScrollBar : public Control {
public:
	ScrollBar(void);
	~ScrollBar(void);
	/**sets position, updates associated stuff */
	void SetPos(int NewPos);
	/**redraws scrollbar if associated with VarName */
	void RedrawScrollBar(const char* VarName, int Sum);
	/**/
	void Draw(unsigned short x, unsigned short y);
private: //Private attributes
	/** Images for drawing the Scroll Bar */
	Sprite2D* Frames[6];
	/** Cursor Position */
	unsigned short Pos;
	/** Scroll Bar Status */
	unsigned short State;
	/** Sets the Text of the current control */
	int SetText(const char* string, int pos = 0);
public:
	void SetImage(unsigned char type, Sprite2D* img);
	/** Sets the Maximum Value of the ScrollBar */
	void SetMax(unsigned short Max);
	/** TextArea Associated Control */
	Control* ta;
public: // Public Events
	/** Mouse Button Down */
	void OnMouseDown(unsigned short x, unsigned short y, unsigned char Button,
		unsigned short Mod);
	/** Mouse Button Up */
	void OnMouseUp(unsigned short x, unsigned short y, unsigned char Button,
		unsigned short Mod);
	/** Mouse Over Event */
	void OnMouseOver(unsigned short x, unsigned short y);
	/** OnChange Scripted Event Function Name */
	EventHandler ScrollBarOnChange;
};

#endif
