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
#include "Sprite2D.h"
#include "GUI/EventMgr.h"
#include "GUI/TextSystem/Font.h"

namespace GemRB {

Console::Console(const Region& frame)
: Control(frame), History(5), textContainer(Region(0,0,0,frame.h), core->GetTextFont(), NULL)
{
	HistPos = 0;

	EventMgr::EventCallback* cb = new MethodCallback<Console, const Event&, bool>(this, &Console::HandleHotKey);
	if (!EventMgr::RegisterHotKeyCallback(cb, ' ', GEM_MOD_CTRL)) {
		delete cb;
	}

	Palette* palette = new Palette( ColorWhite, ColorBlack );
	textContainer.SetPalette(palette);
	palette->release();

	textContainer.SetAlignment(IE_FONT_ALIGN_LEFT | IE_FONT_ALIGN_MIDDLE | IE_FONT_SINGLE_LINE);
	AddSubviewInFrontOfView(&textContainer);
}

Console::~Console()
{
	RemoveSubview(&textContainer);
}

bool Console::HandleHotKey(const Event& e)
{
	if (e.type != Event::KeyDown) return false;

	// the only hot key console registers is for hiding / showing itself
	if (IsVisible()) {
		window->Close();
	} else {
		window->Focus();
	}
	return true;
}

bool Console::OnMouseDown(const MouseEvent& me, unsigned short mod)
{
	textContainer.SetFlags(View::IgnoreEvents, OP_NAND);
	textContainer.MouseDown(me, mod);
	textContainer.SetFlags(View::IgnoreEvents, OP_OR);
	return true;
}

/** Draws the Console on the Output Display */
void Console::DrawSelf(Region drawFrame, const Region& /*clip*/)
{
	Video* video = core->GetVideoDriver();
	video->DrawRect( drawFrame, ColorBlack );
}

/** Sets the Text of the current control */
void Console::SetText(const String& string)
{
	Region rect(Point(), Dimensions());
	textContainer.DeleteContentsInRect(rect);
	textContainer.AppendText(string);
}

/** Key Press Event */
bool Console::OnKeyPress(const KeyboardEvent& key, unsigned short mod)
{
	switch (key.keycode) {
		case GEM_UP:
			HistoryBack();
			break;
		case GEM_DOWN:
			HistoryForward();
			break;
		case GEM_RETURN:
			{
				String text = textContainer.Text();
				if (text.length()) {
					char* cBuf = MBCStringFromString(text);
					assert(cBuf);
					ScriptEngine::FunctionParameters params;
					params.push_back(ScriptEngine::Parameter(cBuf));
					core->GetGUIScriptEngine()->RunFunction("Console", "Exec", params);
					free(cBuf);
					HistoryAdd();
					HistPos = 0;

					SetText(L"");
					MarkDirty();
				}
			}
			break;
		default:
			if (textContainer.KeyPress(key, mod)) {
				return true;
			}
			break;
	}
	return false;
}

void Console::HistoryBack()
{
	String text = textContainer.Text();
	if (text.length() && HistPos == 0 && History.Retrieve(HistPos) != text) {
		HistoryAdd();
		HistPos++;
	}

	SetText(History.Retrieve(HistPos));
	if (++HistPos >= (int)History.Size()) {
		HistPos--;
	}
}

void Console::HistoryForward()
{
	if (--HistPos < 0) {
		SetText(L"");
		HistPos++;
	} else {
		SetText(History.Retrieve(HistPos));
	}
}

void Console::HistoryAdd(bool force)
{
	String text = textContainer.Text();
	if (force || text.length()) {
		History.Append(text, !force);
	}
}

bool Console::SetEvent(int /*eventType*/, ControlEventHandler /*handler*/)
{
	return false;
}

}
