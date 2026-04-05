// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GUI/Label.h"

#include "RGBAColor.h"

#include "Interface.h"

namespace GemRB {

Label::Label(const Region& frame, Holder<Font> fnt, const String& string)
	: Control(frame)
{
	ControlType = IE_GUI_LABEL;
	font = std::move(fnt);

	SetAlignment(IE_FONT_ALIGN_CENTER | IE_FONT_ALIGN_MIDDLE);
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
	if (Alignment == IE_FONT_ALIGN_CENTER && core->HasFeature(GFFlags::LOWER_LABEL_TEXT)) {
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

String Label::QueryText() const
{
	return Text;
}

}
