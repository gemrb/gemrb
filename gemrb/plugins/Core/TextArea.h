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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/TextArea.h,v 1.20 2004/11/01 16:06:42 avenger_teambg Exp $
 *
 */

#ifndef TEXTAREA_H
#define TEXTAREA_H

#include "Control.h"
#include "../../includes/RGBAColor.h"
#include "Font.h"
#include "ScrollBar.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT TextArea : public Control {
public:
	TextArea(Color hitextcolor, Color initcolor, Color lowtextcolor);
	~TextArea(void);
	/** Draws the Control on the Output Display */
	void Draw(unsigned short x, unsigned short y);
	/** Sets the Scroll Bar Pointer */
	void SetScrollBar(Control* ptr);
	/** Sets the Actual Text */
	int SetText(const char* text, int pos = 0);
	/** Appends a String to the current Text */
	int AppendText(const char* text, int pos = 0);
	/** Deletes last `count' lines */ 
	void PopLines(unsigned int count);
	/** Sets the Fonts */
	void SetFonts(Font* init, Font* text);
	/** Returns Number of Rows */
	int GetRowCount();
	/** Returns Starting Row */
	int GetTopIndex();
	/** Set Starting Row */
	void SetRow(int row);
	/** Set Selectable */
	void SetSelectable(bool val);
	/** Set Minimum Selectable Row (to the current ceiling) */
	void SetMinRow(bool enable);
	/** Copies the current TextArea content to another TextArea control */
	void CopyTo(TextArea* ta);
	/** Returns the selected text */
	const char* QueryText();
	/** Redraws the textarea with a new value */
	void RedrawTextArea(char*, unsigned int);
	/** Scrolls automatically to the bottom when the text changes */
	bool AutoScroll;
private: // Private attributes
	bool Selectable;
	std::vector< char*> lines;
	std::vector< int> lrows;
	int seltext;
	/** minimum selectable row */
	int minrow;
	///** Text Buffer */
	//unsigned char * Buffer;
	/** Number of Text Rows */
	int rows;
	/** Starting Row */
	int startrow;
	/** Attached Scroll Bar Pointer*/
	Control* sb;
	/** Text Colors */
	Color* palette;
	Color* initpalette;
	Color* selected;
	Color* lineselpal;
	/** Fonts */
	Font* finit, * ftext;
	void CalcRowCount();
public: //Events
	/** Key Press Event */
	void OnKeyPress(unsigned char Key, unsigned short Mod);
	/** Special Key Press */
	void OnSpecialKeyPress(unsigned char Key);
	/** Mouse Over Event */
	void OnMouseOver(unsigned short x, unsigned short y);
	/** Mouse Button Up */
	void OnMouseUp(unsigned short x, unsigned short y, unsigned char Button,
		unsigned short Mod);
	/** OnChange Scripted Event Function Name */
	EventHandler TextAreaOnChange;
};

#endif
