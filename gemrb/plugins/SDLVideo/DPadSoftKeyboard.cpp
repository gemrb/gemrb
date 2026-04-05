// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DPadSoftKeyboard.h"

using namespace GemRB;

void DPadSoftKeyboard::StartInput()
{
	inputActive = true;
	emptyInput = true;
	currentUpper = true;
	currentCharIndex = 0;
	inputIndexes.clear();
}

void DPadSoftKeyboard::StopInput()
{
	inputActive = false;
}

bool DPadSoftKeyboard::IsInputActive() const
{
	return inputActive;
}

Event DPadSoftKeyboard::GetTextEvent() const
{
	char modKeyValue = dpadKeys[currentCharIndex];

	if (currentUpper && dpadKeys[currentCharIndex] >= 97 && dpadKeys[currentCharIndex] <= 122) {
		modKeyValue -= 32;
	}

	char string[2] = { modKeyValue, '\0' };
	return EventMgr::CreateTextEvent(string);
}

void DPadSoftKeyboard::ToggleUppercase()
{
	if (emptyInput) {
		emptyInput = false;
	}

	if (dpadKeys[currentCharIndex] >= 97 && dpadKeys[currentCharIndex] <= 122) {
		currentUpper = !currentUpper;
	}
}

void DPadSoftKeyboard::RemoveCharacter()
{
	if (inputIndexes.empty()) {
		emptyInput = true;
		currentUpper = true;
		currentCharIndex = 0;
	} else {
		currentCharIndex = inputIndexes.back();
		inputIndexes.pop_back();
		if (inputIndexes.empty()) {
			currentUpper = true;
		}
	}
}

void DPadSoftKeyboard::AddCharacter()
{
	if (emptyInput) {
		emptyInput = false;
	} else {
		currentUpper = false;
		inputIndexes.push_back(currentCharIndex);
		currentCharIndex = 0;
	}
}

void DPadSoftKeyboard::NextCharacter()
{
	if (emptyInput) {
		emptyInput = false;
	} else {
		++currentCharIndex;
		if (currentCharIndex >= TOTAL_CHARACTERS_DPAD) {
			currentCharIndex = 0;
		}
	}
}

void DPadSoftKeyboard::PreviousCharacter()
{
	if (emptyInput) {
		emptyInput = false;
	} else {
		--currentCharIndex;
		if (currentCharIndex < 0) {
			currentCharIndex = TOTAL_CHARACTERS_DPAD - 1;
		}
	}
}
