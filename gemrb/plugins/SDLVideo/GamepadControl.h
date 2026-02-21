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

#ifndef GAMEPADCONTROL_H
#define GAMEPADCONTROL_H

#include <SDL.h>
#include <cstdlib>
#include <stdint.h>

#if !SDL_VERSION_ATLEAST(1, 3, 0)
	#include "SDL12GamepadMappings.h"
#endif

class GamepadControl {
private:
	const float JOY_SPEED_MOD = 500.0f;

public:
	int16_t deadZoneL = 5000;
	int16_t deadZoneR = 5000;

	float joyPointerSpeed = 0;
	float joyPointerAccel = 1.03F;

	float xAxisFloatPos = 0;
	float yAxisFloatPos = 0;
	uint32_t lastAxisMovementTime = 0;
	int16_t xAxisLValue = 0;
	int16_t yAxisLValue = 0;
	int16_t xAxisRValue = 0;
	int16_t yAxisRValue = 0;

	bool gamepadScrollRightKeyPressed = false;
	bool gamepadScrollLeftKeyPressed = false;
	bool gamepadScrollUpKeyPressed = false;
	bool gamepadScrollDownKeyPressed = false;

	void ConfigurePointer(int pointerSpeed, int pointerAccel, int deadZoneL, int deadZoneR);
	float GetPointerSpeed();
	void SetGamepadPosition(int x, int y);
	void HandleAxisEvent(uint8_t axis, int16_t value);
};

#endif
