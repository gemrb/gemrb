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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/TextEdit.cpp,v 1.18 2004/05/09 17:36:26 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "TextEdit.h"
#include "Interface.h"

extern Interface* core;

TextEdit::TextEdit(unsigned short maxLength)
{
	max = maxLength;
	Buffer = ( unsigned char * ) malloc( max + 1 );
	font = NULL;
	Cursor = NULL;
	Back = NULL;
	CurPos = 0;
	Buffer[0] = 0;
	EditOnChange[0] = 0;
	Color white = {0xff, 0xff, 0xff, 0x00}, black = {0x00, 0x00, 0x00, 0x00};
	palette = core->GetVideoDriver()->CreatePalette( white, black );
}

TextEdit::~TextEdit(void)
{
	Video *video = core->GetVideoDriver();
	video->FreePalette( palette );
	free( Buffer );
	video->FreeSprite( Back );
}

/** Draws the Control on the Output Display */
void TextEdit::Draw(unsigned short x, unsigned short y)
{
	if (!Changed) {
		return;
	}
	if (hasFocus) {
		font->Print( Region( x + XPos, y + YPos, Width, Height ), Buffer,
				palette, IE_FONT_ALIGN_LEFT | IE_FONT_ALIGN_MIDDLE, true,
				NULL, NULL, Cursor, CurPos );
	} else {
		font->Print( Region( x + XPos, y + YPos, Width, Height ), Buffer,
				palette, IE_FONT_ALIGN_LEFT | IE_FONT_ALIGN_MIDDLE, true );
	}
	Changed = false;
}

/** Set Font */
void TextEdit::SetFont(Font* f)
{
	if (f != NULL) {
		font = f;
	}
	Changed = true;
}

/** Set Cursor */
void TextEdit::SetCursor(Sprite2D* cur)
{
	if (cur != NULL) {
		Cursor = cur;
	}
	Changed = true;
}

/** Set BackGround */
void TextEdit::SetBackGround(Sprite2D* back)
{
	//if 'back' is NULL then no BackGround will be drawn
	if (Back)
		core->GetVideoDriver()->FreeSprite(Back);
	Back = back;
	Changed = true;
}

/** Key Press Event */
void TextEdit::OnKeyPress(unsigned char Key, unsigned short Mod)
{
	( ( Window * ) Owner )->Invalidate();
	Changed = true;
	if (Key >= 0x20) {
		int len = ( int ) strlen( ( char* ) Buffer );
		if (len + 1 < max) {
			for (int i = len; i > CurPos; i--) {
				Buffer[i] = Buffer[i - 1];
			}
			Buffer[CurPos] = Key;
			Buffer[len + 1] = 0;
			CurPos++;
		}
	}
	RunEventHandler( EditOnChange );
}
/** Special Key Press */
void TextEdit::OnSpecialKeyPress(unsigned char Key)
{
	int len;

	( ( Window * ) Owner )->Invalidate();
	Changed = true;
	switch (Key) {
		case GEM_LEFT:
			if (CurPos > 0)
				CurPos--;
			break;
		case GEM_RIGHT:
			len = ( int ) strlen( ( char * ) Buffer );
			if (CurPos < len) {
				CurPos++;
			}
			break;
		case GEM_DELETE:
			len = ( int ) strlen( ( char * ) Buffer );
			if (CurPos < len) {
				for (int i = CurPos; i < len; i++) {
					Buffer[i] = Buffer[i + 1];
				}
			}
			break;		
		case GEM_BACKSP:
			if (CurPos != 0) {
				int len = ( int ) strlen( ( char* ) Buffer );
				for (int i = CurPos; i < len; i++) {
					Buffer[i - 1] = Buffer[i];
				}
				Buffer[len - 1] = 0;
				CurPos--;
			}
			break;
	}
	RunEventHandler( EditOnChange );
}

/** Sets the Text of the current control */
int TextEdit::SetText(const char* string, int pos)
{
	strncpy( ( char * ) Buffer, string, max );
	( ( Window * ) Owner )->Invalidate();
	return 0;
}

/** Simply returns the pointer to the text, don't modify it! */
const char* TextEdit::QueryText()
{
	return ( const char * ) Buffer;
}
