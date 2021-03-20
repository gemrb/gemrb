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

#include "GameData.h"
#include "Interface.h"
#include "Palette.h"
#include "ScriptEngine.h"
#include "Sprite2D.h"
#include "GUI/EventMgr.h"
#include "GUI/Label.h"
#include "GUI/TextSystem/Font.h"

namespace GemRB {

constexpr size_t HistoryMaxSize = 5;

Console::Console(const Region& frame, TextArea* ta)
: View(frame), History(HistoryMaxSize),
	textContainer(Region(0, 0, 0, 25), core->GetTextFont()),
	feedback(Region(0, 25, frame.w, (frame.h - 37) / 2), core->GetTextFont())
{
	// TODO: move all the control composition to Console.py
	textContainer.SetColors(ColorWhite, ColorBlack);

	textArea = ta;
	HistPos = 0;
	uint8_t align = IE_FONT_ALIGN_LEFT | IE_FONT_ALIGN_MIDDLE | IE_FONT_SINGLE_LINE;

	EventMgr::EventCallback cb = METHOD_CALLBACK(&Console::HandleHotKey, this);
	EventMgr::RegisterHotKeyCallback(cb, ' ', GEM_MOD_CTRL);
	
	AddSubviewInFrontOfView(&feedback);
	feedback.AssignScriptingRef(1, "CONSOLE");
	feedback.SetFlags(TextArea::AutoScroll | TextArea::ClearHistory, OP_OR);

	textContainer.SetMargin(3);

	textContainer.SetAlignment(align);
	AddSubviewInFrontOfView(&textContainer);
	
	if (textArea) {
		Region frame = feedback.Frame();
		frame.y = frame.y + frame.h;
		frame.h = 12;
		
		Label* label = new Label(frame, core->GetTextFont(), L"History:");

		AddSubviewInFrontOfView(label);
		label->SetAlignment(align);
		label->SetColors(ColorWhite, ColorBlack);
		label->SetFlags(Label::UseColor, OP_OR);
		
		frame.y = frame.y + frame.h;
		frame.h = feedback.Frame().h;
		
		textArea->SetFrame(frame);
		AddSubviewInFrontOfView(textArea);
		
		ControlEventHandler handler = [this](Control* c) {
			auto val = c->GetValue();
			size_t histSize = History.Size();
			size_t selected = histSize - val;
			if (selected <= histSize) {
				if (EventMgr::ModState(GEM_MOD_ALT)) {
					Execute(c->QueryText());
					textArea->SelectAvailableOption(-1);
				} else {
					HistPos = selected;
					SetText(c->QueryText());
				}
			}
			this->window->SetFocused(this);
		};
		textArea->SetAction(handler, TextArea::Action::Select);
	}

	static const Color trans(0, 0, 0, 128);
	SetBackground(nullptr, &trans);
}

Console::~Console()
{
	RemoveSubview(&feedback);
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

/** Sets the Text of the current control */
void Console::SetText(const String& string)
{
	Region rect(Point(), Dimensions());
	textContainer.DeleteContentsInRect(rect);
	textContainer.AppendText(string);
	textContainer.CursorEnd();
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
		textArea->SetSelectOptions(options, false);
		//textArea->SelectAvailableOption(History.Size() - HistPos);
		// TODO: if we add a method to TextArea to return the TextContainer for a given select option
		// then we can change the color to red for failed commands and green for successfull ones
		// and the highlight can be just a darker shade of those
	}
	
	MarkDirty();
}

bool Console::Execute(const String& text)
{
	bool ret = false;
	if (text.length()) {
		char* cBuf = MBCStringFromString(text);
		assert(cBuf);
		ScriptEngine::FunctionParameters params;
		params.push_back(ScriptEngine::Parameter(cBuf));
		ret = core->GetGUIScriptEngine()->RunFunction("Console", "Exec", params);
		free(cBuf);
		HistoryAdd();

		SetText(L"");
	}
	return ret;
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
			Execute(textContainer.Text());
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
