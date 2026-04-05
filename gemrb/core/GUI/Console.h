// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
