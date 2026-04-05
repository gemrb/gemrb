// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef DPADSOFTKEYBOARD_H
#define DPADSOFTKEYBOARD_H

#include "GUI/EventMgr.h"

#include <stdint.h>
#include <vector>

namespace GemRB {

class DPadSoftKeyboard {
private:
	static const int TOTAL_CHARACTERS_DPAD = 37;
	bool inputActive = false;
	bool emptyInput = false;
	bool currentUpper = false;
	int32_t currentCharIndex = 0;
	std::vector<int32_t> inputIndexes;

	char dpadKeys[TOTAL_CHARACTERS_DPAD] = {
		//lowercase letters
		97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,
		//space
		32,
		//nums
		48, 49, 50, 51, 52, 53, 54, 55, 56, 57
	};

public:
	void StartInput();
	void StopInput();
	bool IsInputActive() const;
	Event GetTextEvent() const;
	void ToggleUppercase();
	void RemoveCharacter();
	void AddCharacter();
	void NextCharacter();
	void PreviousCharacter();
};

}

#endif
