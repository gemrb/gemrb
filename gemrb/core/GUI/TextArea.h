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

#ifndef TEXTAREA_H
#define TEXTAREA_H

#include "GUI/Control.h"
#include "GUI/ScrollBar.h"

#include "RGBAColor.h"
#include "exports.h"

#include "Font.h"
#include "TextContainer.h"

#include <vector>

namespace GemRB {

// Keep these synchronized with GUIDefines.py
// 0x05 is the control type of TextArea
#define IE_GUI_TEXTAREA_ON_CHANGE   0x05000000 // text change event (keyboard, etc)
#define IE_GUI_TEXTAREA_ON_SELECT	0x05000001 // selection event such as dialog or a listbox

// TextArea flags, keep these in sync too
// the control type is intentionally left out
#define IE_GUI_TEXTAREA_AUTOSCROLL   1
#define IE_GUI_TEXTAREA_SMOOTHSCROLL 2	// chapter text
#define IE_GUI_TEXTAREA_HISTORY      4	// message window
#define IE_GUI_TEXTAREA_EDITABLE     8

typedef std::pair<int, String> SelectOption;

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
	void FlagsChanging(ieDword);
public:
	TextArea(const Region& frame, Font* text);
	TextArea(const Region& frame, Font* text, Font* caps,
			 Color hitextcolor, Color initcolor, Color lowtextcolor);
	~TextArea(void);

	/** Sets the Actual Text */
	void SetText(const char* text);
	/** Clears the textarea */
	void ClearText();
	/** Appends a String to the current Text */
	void AppendText(const char* text);
	void AppendText(const String& text);
	/** Inserts a String into the current Text at pos */
	int InsertText(const char* text, int pos);
	/** Sets up auto scrolling (chapter text) */
	void SetupScroll();
	/** Per Pixel scrolling */
	void ScrollToY(unsigned long y, Control* sender);

	/** Returns total height of the text */
	int GetRowHeight() const;
	void SetSelectOptions(const std::vector<SelectOption>&, bool numbered,
						  const Color* color, const Color* hiColor, const Color* selColor);
	/** Set Starting Row */
	void SetRow(int row);
	/** Set Selectable */
	void SetSelectable(bool val);

	/** Returns the selected text */
	const String& QueryText() const;
	/** Marks textarea for redraw with a new value */
	void UpdateState(const char* VariableName, unsigned int optIdx);
	int SetScrollBar(Control *ptr);
private: // Private attributes
	// dialog and listbox handling
	typedef std::pair<int, TextSpan*> OptionSpan;
	std::vector<OptionSpan> OptSpans;
	TextSpan* hoverSpan, *selectedSpan;
	// dialog options container
	ContentContainer* selectOptions;
	// standard text display container
	TextContainer* textContainer;
	// wrapper containing both of the above
	ContentContainer contentWrapper;

	int TextYPos;
	/** timer for scrolling */
	unsigned long starttime;
	/** timer ticks for scrolling (speed) */
	unsigned long ticks;
	/** Number of Text Rows */
	int rows;

	/** Fonts */
	Font* finit, * ftext;

	/** OnChange Scripted Event Function Name */
	ControlEventHandler TextAreaOnChange;
	ControlEventHandler TextAreaOnSelect;

private: //internal functions
	enum PALETTE_TYPE {
		PALETTE_NORMAL = 0,	// standard text color
		PALETTE_OPTIONS,	// normal palette for selectable options (dialog/listbox)
		PALETTE_HOVER,		// palette for hovering options (dialog/listbox)
		PALETTE_SELECTED,	// selected list box/dialog option.
		PALETTE_INITIALS,	// palette for finit. used only is some cases.

		PALETTE_TYPE_COUNT
	};
	Palette* palettes[PALETTE_TYPE_COUNT];
	Palette* palette; // shortcut for palettes[PALETTE_NORMAL]
	bool needScrollUpdate;

	void Init();
	void SetPalette(const Color*, PALETTE_TYPE);

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
	bool SetEvent(int eventType, ControlEventHandler handler);
	void SetFocus(bool focus);

	void ClearSelectOptions();
};

}

#endif
