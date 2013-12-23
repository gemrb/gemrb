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

#include "GUI/Console.h"

#include "win32def.h"

#include "GameData.h"
#include "Interface.h"
#include "Palette.h"
#include "ScriptEngine.h"
#include "Video.h"
#include "GUI/EventMgr.h"

namespace GemRB {

Console::Console(const Region& frame)
	: Control(frame)
{
	Cursor = NULL;
	Back = NULL;
	max = 128;
	Buffer = ( unsigned char * ) malloc( max );
	Buffer[0] = 0;
	for(size_t i=0;i<HISTORY_SIZE;i++) {
		History[i] = ( unsigned char * ) malloc( max );
		History[i][0] = 0;
	}
	CurPos = 0;
	HistPos = 0;
	HistMax = 0;
	palette = NULL;
}

Console::~Console(void)
{
	free( Buffer );
	for (size_t i=0;i<HISTORY_SIZE;i++) {
		free( History[i] );
	}
	Video *video = core->GetVideoDriver();

	gamedata->FreePalette( palette );
	video->FreeSprite( Cursor );
}

/** Draws the Console on the Output Display */
void Console::Draw(unsigned short x, unsigned short y)
{
	if (Back) {
		core->GetVideoDriver()->BlitSprite( Back, 0, y, true );
	}

	Region r( (short)x + XPos, (short)y + YPos, Width, Height );
	core->GetVideoDriver()->DrawRect( r, ColorBlack );
	font->Print( r, Buffer, palette,
			IE_FONT_ALIGN_LEFT | IE_FONT_ALIGN_MIDDLE, true, NULL,
			Cursor, CurPos, true );
}
/** Set Font */
void Console::SetFont(Font* f)
{
	if (f != NULL) {
		font = f;
	}
}
/** Set Cursor */
void Console::SetCursor(Sprite2D* cur)
{
	if (cur != NULL) {
		Cursor = cur;
	}
}
/** Set BackGround */
void Console::SetBackGround(Sprite2D* back)
{
	//if 'back' is NULL then no BackGround will be drawn
	Back = back;
}
/** Sets the Text of the current control */
void Console::SetText(const char* string)
{
	strlcpy( ( char * ) Buffer, string, max );
}
/** Key Press Event */
bool Console::OnKeyPress(unsigned char Key, unsigned short /*Mod*/)
{
	if (Key >= 0x20) {
		size_t len = strlen( ( char* ) Buffer );
		if (len + 1 < max) {
			for (size_t i = len; i > CurPos; i--) {
				Buffer[i] = Buffer[i - 1];
			}
			Buffer[CurPos++] = Key;
			Buffer[len + 1] = 0;
		}
		return true;
	}
	return false;
}
/** Special Key Press */
bool Console::OnSpecialKeyPress(unsigned char Key)
{
	size_t len;

	switch (Key) {
		case GEM_BACKSP:
			if (CurPos != 0) {
				size_t len = strlen( ( const char * ) Buffer );
				for (size_t i = CurPos; i < len; i++) {
					Buffer[i - 1] = Buffer[i];
				}
				Buffer[len - 1] = 0;
				CurPos--;
			}
			break;
		case GEM_HOME:
			CurPos = 0;
			break;
		case GEM_END:
			CurPos = (unsigned short) strlen( (const char * ) Buffer);
			break;
		case GEM_UP:
			HistoryBack();
			break;
		case GEM_DOWN:
			HistoryForward();
			break;
		case GEM_LEFT:
			if (CurPos > 0)
				CurPos--;
			break;
		case GEM_RIGHT:
			len = strlen( ( const char * ) Buffer );
			if (CurPos < len) {
				CurPos++;
			}
			break;
		case GEM_DELETE:
			len = strlen( ( const char * ) Buffer );
			if (CurPos < len) {
				for (size_t i = CurPos; i < len; i++) {
					Buffer[i] = Buffer[i + 1];
				}
			}
			break;			
		case GEM_RETURN:
			core->GetGUIScriptEngine()->ExecString( ( char* ) Buffer );
			HistoryAdd(false);
			Buffer[0] = 0;
			CurPos = 0;
			HistPos = 0;
			Changed = true;
			break;
	}
	return true;
}

//ctrl-up
void Console::HistoryBack()
{
	HistoryAdd(false);
	if (HistPos < HistMax-1 && Buffer[0]) {
		HistPos++;
	}
	memcpy(Buffer, History[HistPos], max);
	CurPos = (unsigned short) strlen ((const char *) Buffer);
}

//ctrl-down
void Console::HistoryForward()
{
	HistoryAdd(false);
	if (HistPos == 0) {
		Buffer[0]=0;
		CurPos=0;
		return;
	}
	HistPos--;
	memcpy(Buffer, History[HistPos], max);
	CurPos = (unsigned short) strlen ((const char *) Buffer);
}

void Console::HistoryAdd(bool force)
{
	int i;

	if (!force && !Buffer[0])
		return;
	for (i=0;i<HistMax;i++) {
		if (!strnicmp((const char *) History[i],(const char *) Buffer,max) )
			return;
	}
	if (History[0][0]) {
		for (i=HISTORY_SIZE-1; i>0; i--) {
			memcpy(History[i], History[i-1], max);
		}
	}
	memcpy(History[0], Buffer, max);
	if (HistMax<HISTORY_SIZE) {
		HistMax++;
	}
}

void Console::SetFocus(bool focus)
{
	Control::SetFocus(focus);
	if (hasFocus) {
		core->GetVideoDriver()->ShowSoftKeyboard();
	}
}

bool Console::SetEvent(int /*eventType*/, EventHandler /*handler*/)
{
	return false;
}

}
