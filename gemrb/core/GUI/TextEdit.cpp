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

	textContainer.SetAlignment(IE_FONT_ALIGN_MIDDLE | IE_FONT_ALIGN_LEFT | IE_FONT_SINGLE_LINE);
	textContainer.SetColors(ColorWhite, ColorBlack);
	AddSubviewInFrontOfView(&textContainer);

	textContainer.callback = METHOD_CALLBACK(&TextEdit::TextChanged, this);

	max = maxLength;
	textContainer.SetMargin(p.y, p.x);

	SetFlags(Alpha | Numeric, BitOp::OR);

	textContainer.SetEventProxy(this);
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
void TextEdit::SetFont(Holder<Font> f)
{
	textContainer.SetFont(std::move(f));
}

/** Key Press Event */
bool TextEdit::OnKeyPress(const KeyboardEvent& key, unsigned short mod)
{
	if (key.keycode == GEM_RETURN) {
		PerformAction(Action::Done);
		return true;
	}

	// textContainer.OnKeyPress only handles deletion and navigation
	// text is handled in TextEdit::OnTextInput
	if (textContainer.OnKeyPress(key, mod)) {
		PerformAction(Action::Change);
		return true;
	}

	return false;
}

bool TextEdit::OnMouseDown(const MouseEvent& me, unsigned short mod)
{
	return textContainer.OnMouseDown(me, mod);
}

void TextEdit::OnTextInput(const TextEvent& te)
{
	size_t allowedChars = std::min(max - QueryText().length(), te.text.length());
	size_t i = (Flags() & (Alpha | Numeric)) ? 0 : allowedChars;
	for (; i < allowedChars; ++i) {
		wchar_t chr = te.text[i];
		if ((isalpha(chr) || ispunct(chr)) && (Flags() & Alpha) == 0) {
			break;
		} else if (isdigit(chr) && (Flags() & Numeric) == 0) {
			break;
		}
	}

	textContainer.InsertText(te.text.substr(0, i));
}

/** Sets the Text of the current control */
void TextEdit::SetText(String string)
{
	Region rect(Point(), Dimensions());
	textContainer.DeleteContentsInRect(rect);

	if (string.length() > max) {
		textContainer.AppendText(string.substr(0, max));
	} else {
		textContainer.AppendText(std::move(string));
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

void TextEdit::TextChanged(const TextContainer& /*tc*/)
{
	PerformAction(Action::Change);
}

/** Simply returns the pointer to the text, don't modify it! */
String TextEdit::QueryText() const
{
	return textContainer.Text();
}

void TextEdit::SetBackground(const ResRef& bg, TextEditBG type)
{
	bgMos[type] = bg;
}

void TextEdit::SetBackground(TextEditBG type)
{
	auto mos = gamedata->GetResourceHolder<ImageMgr>(bgMos[type]);
	if (mos) {
		Holder<Sprite2D> img = mos->GetSprite2D();
		View::SetBackground(std::move(img));
	}
}

}
