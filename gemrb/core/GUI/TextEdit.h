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

#include "EnumIndex.h"

#include "GUI/Control.h"
#include "GUI/TextSystem/TextContainer.h"

enum class TextEditBG : uint8_t {
	Normal,
	Editing,
	Over,

	count
};

namespace GemRB {

/**
 * @class TextEdit
 * Widget displaying single line text input field
 */

class GEM_EXPORT TextEdit : public Control {
private:
	TextContainer textContainer;

	/** Max Edit Text Length */
	size_t max;
	EnumArray<TextEditBG, ResRef> bgMos; // the second two are rarely used alternative backgrounds

private:
	void TextChanged(const TextContainer& tc);

protected:
	// TextContainer can respond to keys by itself, but we want to interpose so we can capture return/esc
	// we simply forward all other key presses. For this to work textContainer needs View::IgnoreEvents set
	bool OnKeyPress(const KeyboardEvent& Key, unsigned short Mod) override;

	// this forwards to textContainer. only needed because we set View::IgnoreEvents on textContainer in order to interpose key events
	bool OnMouseDown(const MouseEvent& /*me*/, unsigned short /*Mod*/) override;
	void OnTextInput(const TextEvent& /*te*/) override;

public:
	struct Action {
		// !!! Keep these synchronized with GUIDefines.py !!!
		static const Control::Action Change = Control::ValueChange; // text change event (keyboard, etc)
		static const Control::Action Done = ACTION_CUSTOM(0);
		static const Control::Action Cancel = ACTION_CUSTOM(1); // FIXME: unused, how do we cancel?
	};

	enum TextEditFlags {
		// !!! Keep these synchronized with GUIDefines.py !!!
		Alpha = 1,		// TextEdit accepts alpha input
		Numeric = 2 	// TextEdit accepts numeric input
	};

	TextEdit(const Region& frame, unsigned short maxLength, Point p);
	TextEdit(const TextEdit&) = delete;
	~TextEdit() override;
	TextEdit& operator=(const TextEdit&) = delete;

	// these all forward to the underlying TextContainer
	void SetFont(Holder<Font> f);

	/** Sets the Text of the current control */
	void SetText(String string) override;
	/** Gets the Text of the current control */
	String QueryText() const override;
	/** Sets the buffer length */
	void SetBufferLength(size_t buflen);
	/** Sets the alignment */
	void SetAlignment(unsigned char Alignment);
	/** Sets one of the background images */
	void SetBackground(const ResRef& bg, TextEditBG type);
	void SetBackground(TextEditBG type);

	void DidFocus() override { textContainer.DidFocus(); }
	void DidUnFocus() override { textContainer.DidUnFocus(); }
};

}

#endif
