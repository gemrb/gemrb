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
	: Control(frame), FontPos(p)
{
	ControlType = IE_GUI_EDIT;
	max = maxLength;
	Alignment = IE_FONT_ALIGN_MIDDLE | IE_FONT_ALIGN_LEFT;
	font = NULL;
	CurPos = 0;
	Text.reserve(max);

	//Original engine values
	//Color white = {0xc8, 0xc8, 0xc8, 0x00}, black = {0x3c, 0x3c, 0x3c, 0x00};
	palette = new Palette( ColorWhite, ColorBlack );

	Sprite2D* cursor = core->GetCursorSprite();
	SetCursor(cursor);
	cursor->release();
}

TextEdit::~TextEdit(void)
{
	gamedata->FreePalette( palette );
}

void TextEdit::SetAlignment(unsigned char Alignment)
{
    this->Alignment = Alignment;
    MarkDirty();
}

/** Draws the Control on the Output Display */
void TextEdit::DrawSelf(Region rgn, const Region& /*clip*/)
{
	ieWord yOff = FontPos.y;
	Video* video = core->GetVideoDriver();
	Sprite2D* cursor = Cursor();

	if (!font)
		return;

	//The aligning of textedit fields is done by absolute positioning (FontPosX, FontPosY)
	if (IsFocused()) {
		font->Print( Region( rgn.Origin() + FontPos, frame.Dimensions() ),
					Text, palette, Alignment );
		int w = font->StringSize(Text.substr(0, CurPos)).w;
		ieWord vcenter = (rgn.h / 2) + (cursor->Height / 2);
		if (w > rgn.w) {
			int rows = (w / rgn.w);
			vcenter += rows * font->LineHeight;
			w = w - (rgn.w * rows);
		}
		video->BlitSprite(cursor, w + rgn.x + FontPos.x, yOff + vcenter + rgn.y);
	} else {
		font->Print( Region( rgn.x + FontPos.x, rgn.y - yOff, rgn.w, rgn.h ), Text,
				palette, Alignment );
	}
}

/** Set Font */
void TextEdit::SetFont(Font* f)
{
	if (f != NULL) {
		font = f;
		MarkDirty();
		return;
	}
	Log(ERROR, "TextEdit", "Invalid font set!");
}

Font *TextEdit::GetFont() { return font; }

/** Key Press Event */
bool TextEdit::OnKeyPress(const KeyboardEvent& Key, unsigned short /*Mod*/)
{
	MarkDirty();
	switch (Key.keycode) {
		case GEM_HOME:
			CurPos = 0;
			break;
		case GEM_END:
			CurPos = Text.length();
			break;
		case GEM_LEFT:
			if (CurPos > 0)
				CurPos--;
			break;
		case GEM_RIGHT:
			if (CurPos < Text.length()) {
				CurPos++;
			}
			break;
		case GEM_DELETE:
			if (CurPos < Text.length()) {
				Text.erase(CurPos, 1);
			}
			break;		
		case GEM_BACKSP:
			if (CurPos != 0) {
				Text.erase(--CurPos, 1);
			}
			break;
		case GEM_RETURN:
            {
				PerformAction(Action::Done);
            }
			break;
		default:
			if (Key.character) {
				if (Text.length() < max) {
					Text.insert(CurPos++, 1, Key.character);
				}
				break;
			}
			return false;
	}
	PerformAction(Action::Change);
	return true;
}

void TextEdit::SetFocus()
{
	Control::SetFocus();
	if (IsFocused()) {
		core->GetVideoDriver()->ShowSoftKeyboard();
	}
}

/** Sets the Text of the current control */
void TextEdit::SetText(const String& string)
{
	Text = string;
	if (Text.length() > max) CurPos = max + 1;
	else CurPos = Text.length();
	MarkDirty();
}

void TextEdit::SetBufferLength(ieWord buflen)
{
	if(buflen<1) return;
	if(buflen!=max) {
		Text.resize(buflen);
		max = buflen;
	}
}

/** Simply returns the pointer to the text, don't modify it! */
String TextEdit::QueryText() const
{
	return Text;
}

}
