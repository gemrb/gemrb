/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2020 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "DPadSoftKeyboard.h"

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

unsigned char DPadSoftKeyboard::GetCurrentKeyValue() const
{
	unsigned char modKeyValue = dpadKeys[currentCharIndex];

	if (currentUpper && dpadKeys[currentCharIndex] >= 97 && dpadKeys[currentCharIndex] <= 122) {
		modKeyValue -= 32;
	}

	return modKeyValue;
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
