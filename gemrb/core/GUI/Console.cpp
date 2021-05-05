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
#include "ScriptEngine.h"

namespace GemRB {

constexpr size_t HistoryMaxSize = 5;

Console::Console(const Region& frame, TextArea* ta)
: TextEdit(frame, -1, Point(3, 3)), History(HistoryMaxSize)
{
	ControlEventHandler OnReturn = [this](Control*) {
		Execute(QueryText());
	};
	SetAction(OnReturn, TextEdit::Action::Done);

	if (ta) {
		textArea = ta;
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
		default:
			return TextEdit::OnKeyPress(key, mod);
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
	const String& text = QueryText();
	if (force || text.length()) {
		History.Append(std::make_pair(History.Size(), text), !force);
		HistPos = 0;
		UpdateTextArea();
	}
}

}
