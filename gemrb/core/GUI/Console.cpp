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
#include "Streams/FileStream.h"

namespace GemRB {

Console::Console(const Region& frame, TextArea* ta)
: TextEdit(frame, -1, Point(3, 3))
{
	ControlEventHandler onReturn = [this](const Control*) {
		Execute(QueryText());
	};
	SetAction(std::move(onReturn), TextEdit::Action::Done);

	if (ta) {
		textArea = ta;
		textArea->SetAction([this](const Control* c) {
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
		}, TextArea::Action::Select);
	}

	LoadHistory();
}

Console::~Console() noexcept
{
	SaveHistory();
}

void Console::UpdateTextArea()
{
	if (textArea) {
		std::vector<SelectOption> options;
		for (size_t i = 0; i < History.Size(); ++i) {
			options.push_back(History.Retrieve(i));
			options.back().first = int(i) + 1;
		}
		
		textArea->SetValue(INVALID_VALUE);
		textArea->SetSelectOptions(options, false);
		// TODO: if we add a method to TextArea to return the TextContainer for a given select option
		// then we can change the color to red for failed commands and green for successful ones
		// and the highlight can be just a darker shade of those
	}
}

bool Console::Execute(const String& text)
{
	bool success = false;
	if (text.length()) {
		auto ret = core->GetGUIScriptEngine()->RunFunction("Console", "Exec", text);
		success = !ret.IsNull();
		HistoryAdd();
	}
	return success;
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
		SetText(u"");
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
void Console::SaveHistory() const noexcept
{
	std::string commands;

	size_t histSize = std::min(History.Size(), HistoryMaxSize);
	for (size_t i = histSize - 1; signed(i) >= 0; i--) {
		const String& cmd = History.Retrieve(i).second;
		commands += fmt::format("{}\n", fmt::WideToChar{cmd});
	}

	path_t filePath = PathJoin<false>(core->config.GamePath, "gemrb_console.txt");
	FileStream *histFile = new FileStream();
	if (histFile->Create(filePath)) {
		histFile->Write(commands.c_str(), commands.size());
		histFile->Close();
	}
	delete histFile;
}

void Console::LoadHistory()
{
	path_t filePath = PathJoin(core->config.GamePath, "gemrb_console.txt");
	FileStream *histFile = FileStream::OpenFile(filePath);
	if (histFile) {
		std::string line;
		while (histFile->ReadLine(line) != DataStream::Error) {
			History.Append(std::make_pair(-1, StringFromUtf8(line)));
		}
	}
	delete histFile;

	UpdateTextArea();
}

}
