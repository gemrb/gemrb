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

/**
 * @file Console.h
 * Declares Console widget, input field for direct poking into GemRB innards.
 * @author The GemRB Project
 */

#ifndef CONSOLE_H
#define CONSOLE_H

#include "CircularBuffer.h"

#include "GUI/TextArea.h"
#include "GUI/TextEdit.h"

namespace GemRB {

/**
 * @class Console
 * Widget displaying debugging console, input field for direct poking
 * into GemRB innards.
 * The console accepts and executes python statements and has already
 * GemRB python module loaded, so almost any command
 * from GUIScripts can be used.
 */

constexpr size_t HistoryMaxSize = 10;

class GEM_EXPORT Console final : public TextEdit {
private:
	/** History Buffer */
	CircularBuffer<SelectOption> History { HistoryMaxSize };
	/** History Position and size */
	TextArea* textArea = nullptr;
	// an index into Hiostory, except when > History.size() which indicates a new entry
	size_t HistPos = 0;

public:
	Console(const Region& frame, TextArea* ta);
	~Console() noexcept override;

	bool Execute(const String&);

private:
	void UpdateTextArea();
	void HistoryBack();
	void HistoryForward();
	void HistoryAdd(bool force = false);
	void HistorySetPos(size_t);
	void LoadHistory();
	void SaveHistory() const noexcept;

protected:
	/** Key Press Event */
	bool OnKeyPress(const KeyboardEvent& Key, unsigned short Mod) override;
};

}

#endif
