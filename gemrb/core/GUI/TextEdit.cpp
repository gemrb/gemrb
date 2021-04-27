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


#include "GUI/TextEdit.h"

#include "GameData.h"
#include "Interface.h"
#include "Sprite2D.h"
#include "GUI/EventMgr.h"
#include "GUI/Window.h"

namespace GemRB {

TextEdit::TextEdit(const Region& frame, unsigned short maxLength, Point p)
: Control(frame), textContainer(Region(Point(), Dimensions()), core->GetTextFont())
{
	ControlType = IE_GUI_EDIT;

	// FIXME: should we set IE_FONT_SINGLE_LINE?
	textContainer.SetAlignment(IE_FONT_ALIGN_MIDDLE | IE_FONT_ALIGN_LEFT);
	textContainer.SetColors(ColorWhite, ColorBlack);
	AddSubviewInFrontOfView(&textContainer);

	textContainer.callback = METHOD_CALLBACK(&TextEdit::TextChanged, this);

	max = maxLength;
	textContainer.SetMargin(p.y, p.x);

	SetFlags(Alpha|Numeric, OP_OR);
}

TextEdit::~TextEdit()
{
	RemoveSubview(&textContainer);
}

void TextEdit::SetAlignment(unsigned char align)
{
	textContainer.SetAlignment(align);
}

/** Set Font */
void TextEdit::SetFont(Font* f)
{
	textContainer.SetFont(f);
}

void TextEdit::WillDraw(const Region& /*drawFrame*/, const Region& /*clip*/)
{
	textContainer.SetFlags(View::IgnoreEvents, OP_NAND);
}

void TextEdit::DidDraw(const Region& /*drawFrame*/, const Region& /*clip*/)
{
	textContainer.SetFlags(View::IgnoreEvents, OP_OR);
}

/** Key Press Event */
bool TextEdit::OnKeyPress(const KeyboardEvent& key, unsigned short mod)
{
	if (key.keycode == GEM_RETURN) {
		PerformAction(Action::Done);
		return true;
	}

	if (QueryText().length() < max) {

		if ((isalpha(key.character) || ispunct(key.character)) && (Flags()&Alpha) == 0) {
			return false;
		} else if (isdigit(key.character) && (Flags()&Numeric) == 0) {
			return false;
		}

		textContainer.SetFlags(View::IgnoreEvents, OP_NAND);
		if (textContainer.KeyPress(key, mod)) {
			textContainer.SetFlags(View::IgnoreEvents, OP_OR);
			PerformAction(Action::Change);
			return true;
		}
		textContainer.SetFlags(View::IgnoreEvents, OP_OR);
	}
	return false;
}

bool TextEdit::OnMouseDown(const MouseEvent& me, unsigned short mod)
{
	textContainer.SetFlags(View::IgnoreEvents, OP_NAND);
	textContainer.MouseDown(me, mod);
	textContainer.SetFlags(View::IgnoreEvents, OP_OR);
	return true;
}

void TextEdit::OnTextInput(const TextEvent& te)
{
	textContainer.TextInput(te);
}

/** Sets the Text of the current control */
void TextEdit::SetText(const String& string)
{
	Region rect(Point(), Dimensions());
	textContainer.DeleteContentsInRect(rect);

	if (string.length() > max) {
		textContainer.AppendText(string.substr(0, max));
	} else {
		textContainer.AppendText(string);
	}
	textContainer.CursorEnd();
}

void TextEdit::SetBufferLength(size_t buflen)
{
	const String& text = QueryText();
	if (buflen < text.length()) {
		max = buflen;
		SetText(textContainer.Text());
	} else {
		max = buflen;
	}
}

void TextEdit::TextChanged(TextContainer& /*tc*/)
{
	PerformAction(Action::Change);
}

/** Simply returns the pointer to the text, don't modify it! */
String TextEdit::QueryText() const
{
	return textContainer.Text();
}

}
