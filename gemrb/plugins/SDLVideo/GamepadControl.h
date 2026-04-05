// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
	float GetPointerSpeed() const;
	void SetGamepadPosition(int x, int y);
	void HandleAxisEvent(uint8_t axis, int16_t value);
};

#endif
