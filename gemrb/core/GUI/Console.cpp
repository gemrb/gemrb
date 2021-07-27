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

#include "Interface.h"
#include "System/FileStream.h"

namespace GemRB {

constexpr size_t HistoryMaxSize = 5;

Console::Console(const Region& frame, TextArea* ta)
: TextEdit(frame, -1, Point(3, 3)), History(HistoryMaxSize)
{
	ControlEventHandler OnReturn = [this](const Control*) {
		Execute(QueryText());
	};
	SetAction(OnReturn, TextEdit::Action::Done);

	if (ta) {
		textArea = ta;
		ControlEventHandler handler = [this](const Control* c) {
			auto val = c->GetValue();
			size_t histSize = History.Size();
			size_t selected = histSize - val;
			if (EventMgr::ModState(GEM_MOD_ALT)) {
				Execute(c->QueryText());
				textArea->SelectAvailableOption(-1);
			} else {
				HistPos = selected;
				SetText(c->QueryText());
			}
			SetFocus();
		};
		textArea->SetAction(handler, TextArea::Action::Select);
	}

	LoadHistory();
}

void Console::UpdateTextArea()
{
	if (textArea) {
		std::vector<SelectOption> options;
		for (size_t i = History.Size(); i > 0; --i) {
			options.push_back(History.Retrieve(i - 1));
			options.back().first = int(History.Size() - i) + 1;
		}
		
		textArea->SetValue(-1);
		textArea->SetSelectOptions(options, false);
		// TODO: if we add a method to TextArea to return the TextContainer for a given select option
		// then we can change the color to red for failed commands and green for successfull ones
		// and the highlight can be just a darker shade of those
	}
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

void Console::HistorySetPos(size_t pos)
{
	HistPos = Clamp<size_t>(pos, 0, History.Size());

	if (HistPos == History.Size()) {
		SetText(L"");
		if (textArea) {
			textArea->SelectAvailableOption(-1);
		}
	} else if (textArea) {
		textArea->SelectAvailableOption(History.Size() - HistPos - 1);
	} else {
		SetText(History.Retrieve(HistPos).second);
	}
}

void Console::HistoryBack()
{
	if (HistPos == History.Size()) {
		HistorySetPos(0);
	} else {
		HistorySetPos(HistPos + 1);
	}
}

void Console::HistoryForward()
{
	HistorySetPos(HistPos - 1);
}

void Console::HistoryAdd(bool force)
{
	const String& text = QueryText();
	if (force || text.length()) {
		History.Append(std::make_pair(-1, text), !force);
		UpdateTextArea();
		HistorySetPos(History.Size());
	}
}

// dump the last few commands to a file
void Console::SaveHistory() const
{
	std::string commands;
	char command[100];

	size_t histSize = std::min<size_t>(History.Size(), 10UL);
	for (size_t i = histSize - 1; signed(i) >= 0; i--) {
		const String& cmd = History.Retrieve(i).second;
		snprintf(command, sizeof(command), "%ls\n", cmd.c_str());
		commands += command;
	}

	char filePath[_MAX_PATH + 20];
	PathJoin(filePath, core->config.GamePath, "gemrb_console.txt", nullptr);
	FileStream *histFile = new FileStream();
	if (histFile->Create(filePath)) {
		histFile->Write(commands.c_str(), commands.size());
		histFile->Close();
	}
	delete histFile;
}

void Console::LoadHistory()
{
	char filePath[_MAX_PATH + 20];
	PathJoin(filePath, core->config.GamePath, "gemrb_console.txt", nullptr);
	FileStream *histFile = FileStream::OpenFile(filePath);
	if (histFile) {
		char line[_MAX_PATH];
		while (histFile->Remains()) {
			if (histFile->ReadLine(line, _MAX_PATH) == -1) break;

			// set history
			wchar_t tmp[100];
			swprintf(tmp, 100, L"%s", line);
			History.Append(std::make_pair(-1, tmp));
		}
	}
	delete histFile;

	UpdateTextArea();
}

}
