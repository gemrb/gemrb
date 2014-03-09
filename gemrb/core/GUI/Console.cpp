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

#include "Font.h"
#include "GameData.h"
#include "Interface.h"
#include "Palette.h"
#include "ScriptEngine.h"
#include "Sprite2D.h"
#include "Video.h"
#include "GUI/EventMgr.h"

namespace GemRB {

Console::Console(const Region& frame)
	: Control(frame), History(5)
{
	Cursor = NULL;
	Back = NULL;
	max = 128;
	Buffer.reserve(max);
	CurPos = 0;
	HistPos = 0;
	palette = NULL;
	font = NULL;
}

Console::~Console(void)
{
	Video *video = core->GetVideoDriver();

	gamedata->FreePalette( palette );
	video->FreeSprite( Cursor );
}

/** Draws the Console on the Output Display */
void Console::DrawInternal(Region& drawFrame)
{
	if (Back) {
		core->GetVideoDriver()->BlitSprite( Back, 0, drawFrame.y, true );
	}

	Video* video = core->GetVideoDriver();
	video->DrawRect( drawFrame, ColorBlack );
	font->Print( drawFrame, Buffer, palette,
			IE_FONT_ALIGN_LEFT | IE_FONT_ALIGN_MIDDLE);
	ieWord w = font->StringSize(Buffer.substr(0, CurPos)).w;
	ieWord vcenter = (drawFrame.h / 2) + (Cursor->Height / 2);
	video->BlitSprite(Cursor, w + drawFrame.x, vcenter + drawFrame.y, true);
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
void Console::SetText(const String& string)
{
	Buffer = string;
}
/** Key Press Event */
bool Console::OnKeyPress(unsigned char Key, unsigned short /*Mod*/)
{
	if (Key >= 0x20) {
		if (Buffer.length() < max) {
			Buffer.insert(CurPos++, 1, Key);
		}
		return true;
	}
	return false;
}
/** Special Key Press */
bool Console::OnSpecialKeyPress(unsigned char Key)
{
	switch (Key) {
		case GEM_BACKSP:
			if (CurPos != 0) {
				Buffer.erase(--CurPos, 1);
			}
			break;
		case GEM_HOME:
			CurPos = 0;
			break;
		case GEM_END:
			CurPos = Buffer.length();
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
			if (CurPos < Buffer.length()) {
				CurPos++;
			}
			break;
		case GEM_DELETE:
			if (CurPos < Buffer.length()) {
				Buffer.erase(CurPos, 1);
			}
			break;			
		case GEM_RETURN:
			char* cBuf = new char[max+1];
			// FIXME: depends on locale setting
			wcstombs(cBuf, Buffer.c_str(), max);
			// FIXME: should prepend "# coding=<encoding name>" as per http://www.python.org/dev/peps/pep-0263/
			core->GetGUIScriptEngine()->ExecString( cBuf );
			delete[] cBuf;
			HistoryAdd();
			Buffer.erase();
			CurPos = 0;
			HistPos = 0;
			break;
	}
	return true;
}

void Console::HistoryBack()
{
	if (Buffer[0] && HistPos == 0 && History.Retrieve(HistPos) != Buffer) {
		HistoryAdd();
		HistPos++;
	}
	Buffer = History.Retrieve(HistPos);
	CurPos = Buffer.length();
	if (++HistPos >= (int)History.Size()) {
		HistPos--;
	}
}

void Console::HistoryForward()
{
	if (--HistPos < 0) {
		Buffer.erase();
		HistPos++;
	} else {
		Buffer = History.Retrieve(HistPos);
	}
	CurPos = Buffer.length();
}

void Console::HistoryAdd(bool force)
{
	if (force || Buffer.length()) {
		History.Append(Buffer, !force);
	}
}

void Console::SetFocus(bool focus)
{
	Control::SetFocus(focus);
	if (hasFocus) {
		core->GetVideoDriver()->ShowSoftKeyboard();
	}
}

bool Console::SetEvent(int /*eventType*/, ControlEventHandler /*handler*/)
{
	return false;
}

}
