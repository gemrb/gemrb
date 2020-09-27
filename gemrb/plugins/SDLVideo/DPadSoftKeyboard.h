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

#ifndef DPADSOFTKEYBOARD_H
#define DPADSOFTKEYBOARD_H

#include <stdint.h>
#include <vector>

class DPadSoftKeyboard
{
private:
	static const int TOTAL_CHARACTERS_DPAD = 37;
	bool inputActive = false;
	bool emptyInput = false;
	bool currentUpper = false;
	int32_t currentCharIndex;
	std::vector <int32_t> inputIndexes;

	unsigned char dpadKeys[TOTAL_CHARACTERS_DPAD] = 
	{
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
	unsigned char GetCurrentKeyValue() const;
	void ToggleUppercase();
	void RemoveCharacter();
	void AddCharacter();
	void NextCharacter();
	void PreviousCharacter();
};

#endif
