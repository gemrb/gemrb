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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

/**
 * @file TextArea.h
 * Declares TextArea widget for displaying long paragraphs of text
 * @author The GemRB Project
 */

#ifndef TEXTAREA_H
#define TEXTAREA_H

#include "GUI/Control.h"
#include "GUI/ScrollBar.h"

#include "RGBAColor.h"
#include "exports.h"

#include "Font.h"

namespace GemRB {

// Keep these synchronized with GUIDefines.py
// 0x05 is the control type of TextArea
#define IE_GUI_TEXTAREA_ON_CHANGE   0x05000000
#define IE_GUI_TEXTAREA_OUT_OF_TEXT 0x05000001

// TextArea flags, keep these in sync too
// the control type is intentionally left out
#define IE_GUI_TEXTAREA_SELECTABLE   1
#define IE_GUI_TEXTAREA_AUTOSCROLL   2
#define IE_GUI_TEXTAREA_SMOOTHSCROLL 4
#define IE_GUI_TEXTAREA_HISTORY      8
#define IE_GUI_TEXTAREA_SPEAKER      16
#define IE_GUI_TEXTAREA_ALT_FONT     32   //this one disables drop capitals
#define IE_GUI_TEXTAREA_EDITABLE     64

// internal flags
#define TA_INITIALS    1
#define TA_BITEMYTAIL  2

/**
 * @class TextArea
 * Widget capable of displaying long paragraphs of text.
 * It is usually scrolled with a ScrollBar widget
 */

class GEM_EXPORT TextArea : public Control {
public:
	TextArea(Color hitextcolor, Color initcolor, Color lowtextcolor);
	~TextArea(void);
	/** global configuration */
	static void SetNoteString(const char *s);
	/** Draws the Control on the Output Display */
	void Draw(unsigned short x, unsigned short y);
	/** Set the TextArea value to the line number containing the string parameter */
	void SelectText(const char *select);
	/** Sets the Actual Text */
	void SetText(const char* text);
	/** Sets text */
	void SetText(const std::vector<char*>& text);
	/** Clears the textarea */
	void Clear();
	/** Discards scrolled out lines from the textarea */
	/** preserving 'keeplines' lines for scroll back history */
	void DiscardLines();
	/** Appends a String to the current Text */
	int AppendText(const char* text, int pos = 0);
	/** Deletes `count' lines (either last or top lines)*/
	void PopLines(unsigned int count, bool top = false);
	/** Deletes last lines up to current 'minrow' */
	void PopMinRow()
	{
		PopLines((unsigned int) (lines.size()-minrow));
	}
	/** adds empty lines so minrow will be the uppermost visible row */
	void PadMinRow();
	/** Sets up scrolling, tck is the scrolling speed */
	void SetupScroll(unsigned long tck);
	/** Per Pixel scrolling */
	void ScrollToY(unsigned long y, Control* sender);
	/** Sets the Fonts */
	void SetFonts(Font* init, Font* text);
	/** Returns Number of Rows */
	int GetRowCount();
	/** Returns the length of a Row */
	int GetRowLength(unsigned int row);
	/** Returns Number of Visible Rows */
	int GetVisibleRowCount();
	/** Returns Starting Row */
	int GetTopIndex();
	/** Returns total height of the text */
	int GetRowHeight();
	/** Set Starting Row */
	void SetRow(int row);
	/** Sets preserved lines */
	void SetPreservedRow(int arg);
	/** Set Selectable */
	void SetSelectable(bool val);
	/** Set Minimum Selectable Row (to the current ceiling) */
	void SetMinRow(bool enable);
	/** Copies the current TextArea content to another TextArea control */
	void CopyTo(TextArea* ta);
	/** Returns the selected text */
	const char* QueryText();
	/** Marks textarea for redraw with a new value */
	void RedrawTextArea(const char* VariableName, unsigned int Sum);
	int SetScrollBar(Control *ptr);
private: // Private attributes
	std::vector< char*> lines;
	std::vector< int> lrows;
	int seltext;
	/** minimum selectable row */
	int minrow;
	/** lines to be kept even if scrolled out */
	int keeplines;
	unsigned long TextYPos;
	/** timer for scrolling */
	unsigned long starttime;
	/** timer ticks for scrolling (speed) */
	unsigned long ticks;
	/** Number of Text Rows */
	int rows;
	/** Starting Row */
	int startrow;
	/** Text Colors */
	Palette* palette;
	Palette* initpalette;
	Palette* selected;
	Palette* lineselpal;
	/** a hack for smooth windows, drop capitals */
	ieDword InternalFlags;
	/** Fonts */
	Font* finit, * ftext;
	ieResRef PortraitResRef;

	/** Text Editing Cursor Sprite */
	Sprite2D* Cursor;
	unsigned short CurPos, CurLine;

private: //internal functions
	void CalcRowCount();
	void UpdateControls();
	void RefreshSprite(const char *portrait);

public: //Events
	/** Key Press Event */
	void OnKeyPress(unsigned char Key, unsigned short Mod);
	/** Special Key Press */
	void OnSpecialKeyPress(unsigned char Key);
	/** Mousewheel scroll */
	void OnMouseWheelScroll(short x, short y);
	/** Mouse Over Event */
	void OnMouseOver(unsigned short x, unsigned short y);
	/** Mouse Button Up */
	void OnMouseUp(unsigned short x, unsigned short y, unsigned short Button,
		unsigned short Mod);
	/** Mouse button down*/
	void OnMouseDown(unsigned short x, unsigned short y, unsigned short Button,
		unsigned short Mod);
	/** Set handler for specified event */
	bool SetEvent(int eventType, EventHandler handler);
	void SetFocus(bool focus);
	/** OnChange Scripted Event Function Name */
	EventHandler TextAreaOnChange;
	/** OutOfText Scripted Event Function Name */
	EventHandler TextAreaOutOfText;
};

}

#endif
