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
#include "Video.h"
#include "GUI/EventMgr.h"
#include "GUI/Window.h"

namespace GemRB {

TextEdit::TextEdit(const Region& frame, unsigned short maxLength, unsigned short px, unsigned short py)
	: Control(frame)
{
	ControlType = IE_GUI_EDIT;
	max = maxLength;
	FontPosX = px;
	FontPosY = py;
	Alignment = IE_FONT_ALIGN_MIDDLE | IE_FONT_ALIGN_LEFT;
	Buffer = ( unsigned char * ) malloc( max + 1 );
	font = NULL;
	Cursor = NULL;
	Back = NULL;
	CurPos = 0;
	Buffer[0] = 0;
	ResetEventHandler( EditOnChange );
	ResetEventHandler( EditOnDone );
	ResetEventHandler( EditOnCancel );
	//Original engine values
	//Color white = {0xc8, 0xc8, 0xc8, 0x00}, black = {0x3c, 0x3c, 0x3c, 0x00};
	palette = core->CreatePalette( ColorWhite, ColorBlack );
}

TextEdit::~TextEdit(void)
{
	Video *video = core->GetVideoDriver();
	gamedata->FreePalette( palette );
	free( Buffer );
	video->FreeSprite( Back );
	video->FreeSprite( Cursor );
}

void TextEdit::SetAlignment(unsigned char Alignment)
{
    this->Alignment = Alignment;
    Changed = true;
}

/** Draws the Control on the Output Display */
void TextEdit::Draw(unsigned short x, unsigned short y)
{
	if (!Changed && !(Owner->Flags&WF_FLOAT)) {
		return;
	}
	Changed = false;
	if (Back) {
		core->GetVideoDriver()->BlitSprite( Back, x + XPos, y + YPos, true );

	}
	if (!font)
		return;

	//The aligning of textedit fields is done by absolute positioning (FontPosX, FontPosY)
	if (hasFocus) {
		font->Print( Region( x + XPos + FontPosX, y + YPos + FontPosY, Width, Height ), Buffer,
				palette, Alignment,
				true, NULL, Cursor, CurPos );
	} else {
		font->Print( Region( x + XPos + FontPosX, y + YPos + FontPosY, Width, Height ), Buffer,
				palette, Alignment, true );
	}
}

/** Set Font */
void TextEdit::SetFont(Font* f)
{
	if (f != NULL) {
		font = f;
		Changed = true;
		return;
	}
	Log(ERROR, "TextEdit", "Invalid font set!");
}

Font *TextEdit::GetFont() { return font; }

/** Set Cursor */
void TextEdit::SetCursor(Sprite2D* cur)
{
	core->GetVideoDriver()->FreeSprite( Cursor );
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
bool TextEdit::OnKeyPress(unsigned char Key, unsigned short /*Mod*/)
{
	if (Key >= 0x20) {
		if (Value && ( (Key<'0') || (Key>'9') ) )
			return false;
		Owner->Invalidate();
		Changed = true;
		int len = ( int ) strlen( ( char* ) Buffer );
		if (len + 1 < max) {
			for (int i = len; i > CurPos; i--) {
				Buffer[i] = Buffer[i - 1];
			}
			Buffer[CurPos] = Key;
			Buffer[len + 1] = 0;
			CurPos++;
		}
		RunEventHandler( EditOnChange );
		return true;
	}
	return false;
}
/** Special Key Press */
bool TextEdit::OnSpecialKeyPress(unsigned char Key)
{
	int len;

	Owner->Invalidate();
	Changed = true;
	switch (Key) {
		case GEM_HOME:
			CurPos = 0;
			break;
		case GEM_END:
			CurPos = (ieWord) strlen( (char * ) Buffer);
			break;
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
		case GEM_RETURN:
			RunEventHandler( EditOnDone );
	}
	RunEventHandler( EditOnChange );
	return true;
}

void TextEdit::SetFocus(bool focus)
{
	Control::SetFocus(focus);
	if (hasFocus) {
		core->GetVideoDriver()->ShowSoftKeyboard();
	}
}

/** Sets the Text of the current control */
void TextEdit::SetText(const char* string)
{
	int len = strlcpy( ( char * ) Buffer, string, max + 1 );
	if (len > max) CurPos = max + 1;
	else CurPos = len;
	if (Owner) {
		Owner->Invalidate();
	}
}

void TextEdit::SetBufferLength(ieWord buflen)
{
	if(buflen<1) return;
	if(buflen!=max) {
		Buffer = (unsigned char *) realloc(Buffer, buflen+1);
		max=(ieWord) buflen;
		Buffer[max]=0;
	}
}

/** Simply returns the pointer to the text, don't modify it! */
const char* TextEdit::QueryText() const
{
	return ( const char * ) Buffer;
}

bool TextEdit::SetEvent(int eventType, EventHandler handler)
{
	Changed = true;

	switch (eventType) {
	case IE_GUI_EDIT_ON_CHANGE:
		EditOnChange = handler;
		break;
	case IE_GUI_EDIT_ON_DONE:
		EditOnDone = handler;
		break;
	case IE_GUI_EDIT_ON_CANCEL:
		EditOnCancel = handler;
		break;
	default:
		return false;
	}

	return true;
}

}
