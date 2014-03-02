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

#include "DialogHandler.h"
#include "Font.h"
#include "TextContainer.h"

#include <vector>

namespace GemRB {

// Keep these synchronized with GUIDefines.py
// 0x05 is the control type of TextArea
#define IE_GUI_TEXTAREA_ON_CHANGE   0x05000000

// TextArea flags, keep these in sync too
// the control type is intentionally left out
#define IE_GUI_TEXTAREA_SELECTABLE   1	// listbox
#define IE_GUI_TEXTAREA_AUTOSCROLL   2
#define IE_GUI_TEXTAREA_SMOOTHSCROLL 4	// chapter text
#define IE_GUI_TEXTAREA_HISTORY      8	// message window
#define IE_GUI_TEXTAREA_SPEAKER      16	// message window
#define IE_GUI_TEXTAREA_ALT_FONT     32	// this one disables drop capitals, unused
#define IE_GUI_TEXTAREA_EDITABLE     64

/**
 * @class TextArea
 * Widget capable of displaying long paragraphs of text.
 * It is usually scrolled with a ScrollBar widget
 */

class GEM_EXPORT TextArea : public Control {
protected:
	/** Draws the Control on the Output Display */
	void DrawInternal(Region& drawFrame);
	bool NeedsDraw();
	bool HasBackground() { return false; }
public:
	TextArea(const Region& frame, Font* text, Font* caps,
			 Color hitextcolor, Color initcolor, Color lowtextcolor);
	~TextArea(void);
	/** Set the TextArea value to the line number containing the string parameter */
	void SelectText(const char *select);
	/** Sets the Actual Text */
	void SetText(const char* text);
	/** Clears the textarea */
	void Clear();
	/** Appends a String to the current Text */
	void AppendText(const char* text);
	void AppendText(String* text);
	/** Inserts a String into the current Text at pos */
	int InsertText(const char* text, int pos);
	/** Sets up auto scrolling (chapter text) */
	void SetupScroll();
	/** Per Pixel scrolling */
	void ScrollToY(unsigned long y, Control* sender);

	/** Returns total height of the text */
	int GetRowHeight() const;
	void SetDialogOptions(const std::vector<DialogOption>&,
						  const Color* color, const Color* hiColor);
	/** Set Starting Row */
	void SetRow(int row);
	/** Set Selectable */
	void SetSelectable(bool val);

	/** Returns the selected text */
	const String& QueryText() const;
	/** Marks textarea for redraw with a new value */
	void UpdateState(const char* VariableName, unsigned int Sum);
	int SetScrollBar(Control *ptr);
private: // Private attributes
	// dialog handling
	typedef std::pair<int, TextSpan*> DialogOptionSpan;
	std::vector<DialogOptionSpan> dialogOptSpans;
	TextContainer* dialogOptions;
	TextSpan* hoverSpan, *selectedSpan;
	// standard text display
	TextContainer* textContainer;
	// TODO: we need a circular TextContainer subclass for the message window

	int TextYPos;
	/** timer for scrolling */
	unsigned long starttime;
	/** timer ticks for scrolling (speed) */
	unsigned long ticks;
	/** Number of Text Rows */
	int rows;

	/** Text Colors */
	Palette* palette;		// standard text color
	Palette* hoverPal;		// palette for hovering options (dialog/listbox)
	Palette* selectedPal;	// selected list box option, but also normal palette for dialog options.
	Palette* initpalette;	// palette for finit. used only is some cases.

	/** Fonts */
	Font* finit, * ftext;
	ieResRef PortraitResRef;

	/** Text Editing Cursor Sprite */
	Sprite2D* Cursor;
	size_t CurPos;

private: //internal functions
	void ClearDialogOptions();
	void CalcRowCount();
	void UpdateControls();
	void RefreshSprite(const char *portrait);

public: //Events
	/** Key Press Event */
	bool OnKeyPress(unsigned char Key, unsigned short Mod);
	/** Special Key Press */
	bool OnSpecialKeyPress(unsigned char Key);
	/** Mousewheel scroll */
	void OnMouseWheelScroll(short x, short y);
	/** Mouse Over Event */
	void OnMouseOver(unsigned short x, unsigned short y);
	/** Mouse Button Up */
	void OnMouseUp(unsigned short x, unsigned short y,
				   unsigned short Button, unsigned short Mod);
	/** Mouse button down*/
	void OnMouseDown(unsigned short x, unsigned short y,
					 unsigned short Button, unsigned short Mod);
	/** Set handler for specified event */
	bool SetEvent(int eventType, EventHandler handler);
	void SetFocus(bool focus);
	/** OnChange Scripted Event Function Name */
	EventHandler TextAreaOnChange;
};

}

#endif
