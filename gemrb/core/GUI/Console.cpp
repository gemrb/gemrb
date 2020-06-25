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

constexpr size_t HistoryMaxSize = 5;

Console::Console(const Region& frame, TextArea* ta)
: Control(frame), History(HistoryMaxSize), textContainer(Region(0, std::max<int>(0, frame.h - 25), 0, 25), core->GetTextFont(), NULL)
{
	textArea = ta;
	HistPos = 0;

	EventMgr::EventCallback cb = METHOD_CALLBACK(&Console::HandleHotKey, this);
	EventMgr::RegisterHotKeyCallback(cb, ' ', GEM_MOD_CTRL);

	Palette* palette = new Palette( ColorWhite, ColorBlack );
	textContainer.SetPalette(palette);
	palette->release();
	textContainer.SetMargin(3);

	textContainer.SetAlignment(IE_FONT_ALIGN_LEFT | IE_FONT_ALIGN_MIDDLE | IE_FONT_SINGLE_LINE);
	AddSubviewInFrontOfView(&textContainer);
	
	if (textArea) {
		Size s = Dimensions();
		s.h -= 25;
		textArea->SetFrameSize(s);
		AddSubviewInFrontOfView(textArea);
		
		ControlEventHandler handler = [this](Control* c) {
			auto val = c->GetValue();
			size_t histSize = History.Size();
			size_t selected = histSize - val;
			if (selected <= histSize) {
				HistPos = selected;
				SetText(c->QueryText());
			}
			this->window->SetFocused(this);
		};
		textArea->SetAction(handler, TextArea::Action::Select);
	}
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
		window->SetFocused(this);
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

void Console::OnTextInput(const TextEvent& te)
{
	textContainer.TextInput(te);
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
	MarkDirty();
}

void Console::UpdateTextArea()
{
	if (textArea) {
		std::vector<SelectOption> options;
		for (auto item : History) {
			options.push_back(item);
		}
		textArea->SetValue(-1);
		textArea->SetSelectOptions(options, false, nullptr, &SelectOptionHover, &SelectOptionSelected);
		//textArea->SelectAvailableOption(History.Size() - HistPos);
		// TODO: if we add a method to TextArea to return the TextContainer for a given select option
		// then we can change the color to red for failed commands and green for successfull ones
		// and the highlight can be just a darker shade of those
	}
	
	MarkDirty();
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

					SetText(L"");
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
	if (HistPos < History.Size()) {
		HistPos++;
		if (textArea) {
			textArea->SelectAvailableOption(History.Size() - HistPos);
		} else {
			SetText(History.Retrieve(HistPos - 1).second);
		}
	}
}

void Console::HistoryForward()
{
	if (HistPos <= 1) {
		SetText(L"");
		HistPos = 0;
		if (textArea) {
			textArea->SelectAvailableOption(-1);
		}
	} else {
		--HistPos;
		if (textArea) {
			textArea->SelectAvailableOption(History.Size() - HistPos);
		} else {
			SetText(History.Retrieve(HistPos - 1).second);
		}
	}
}

void Console::HistoryAdd(bool force)
{
	String text = textContainer.Text();
	if (force || text.length()) {
		History.Append(std::make_pair(History.Size(), text), !force);
		HistPos = 0;
		UpdateTextArea();
	}
}

bool Console::SetEvent(int /*eventType*/, ControlEventHandler /*handler*/)
{
	return false;
}

}
