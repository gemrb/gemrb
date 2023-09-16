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

#include "GUI/Label.h"

#include "GameData.h"
#include "Interface.h"
#include "Sprite2D.h"
#include "GUI/Window.h"

namespace GemRB {

Label::Label(const Region& frame, Holder<Font> font, const String& string)
	: Control(frame)
{
	ControlType = IE_GUI_LABEL;
	font = std::move(font);

	SetAlignment(IE_FONT_ALIGN_CENTER|IE_FONT_ALIGN_MIDDLE);
	SetFlags(IgnoreEvents, BitOp::OR);
	SetText(string);
}

/** Draws the Control on the Output Display */
void Label::DrawSelf(const Region& rgn, const Region& /*clip*/)
{
	if (font && Text.length()) {
		if (flags & UseColor) {
			font->Print(rgn, Text, Alignment, colors);
		} else {
			font->Print(rgn, Text, Alignment);
		}
	}
}
/** This function sets the actual Label Text */
void Label::SetText(String string)
{
	Text = std::move(string);
	if (Alignment == IE_FONT_ALIGN_CENTER
		&& core->HasFeature( GFFlags::LOWER_LABEL_TEXT )) {
		StringToLower(Text);
	}
	MarkDirty();
}

void Label::SetColors(const Color& col, const Color& bg)
{
	colors.fg = col;
	colors.bg = bg;
	MarkDirty();
}

void Label::SetAlignment(unsigned char newAlignment)
{
	if (!font || frame.h <= font->LineHeight) {
		// FIXME: is this a poor way of determinine if we are single line?
		newAlignment |= IE_FONT_SINGLE_LINE;
	} else if (frame.h < font->LineHeight * 2) {
		newAlignment |= IE_FONT_NO_CALC;
	}
	Alignment = newAlignment;
	if (newAlignment == IE_FONT_ALIGN_CENTER && core->HasFeature(GFFlags::LOWER_LABEL_TEXT)) {
		StringToLower(Text);
	}
	MarkDirty();
}

/** Simply returns the pointer to the text, don't modify it! */
String Label::QueryText() const
{
	return Text;
}

}
