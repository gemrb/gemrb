// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GamepadControl.h"

void GamepadControl::ConfigurePointer(int pointerSpeed, int pointerAccel, int deadL, int deadR)
{
	joyPointerSpeed = pointerSpeed / JOY_SPEED_MOD;
	joyPointerAccel = pointerAccel / 100.0F;
	deadZoneL = deadL;
	deadZoneR = deadR;
}

float GamepadControl::GetPointerSpeed() const
{
	return joyPointerSpeed;
}

void GamepadControl::SetGamepadPosition(int x, int y)
{
	xAxisFloatPos = x;
	yAxisFloatPos = y;
}

void GamepadControl::HandleAxisEvent(uint8_t axis, int16_t value)
{
	if (axis == SDL_CONTROLLER_AXIS_LEFTX) {
		if (std::abs(value) > deadZoneL) {
			xAxisLValue = value;
		} else {
			xAxisLValue = 0;
		}
	} else if (axis == SDL_CONTROLLER_AXIS_LEFTY) {
		if (std::abs(value) > deadZoneL) {
			yAxisLValue = value;
		} else {
			yAxisLValue = 0;
		}
	} else if (axis == SDL_CONTROLLER_AXIS_RIGHTX) {
		if (std::abs(value) > deadZoneR) {
			xAxisRValue = value;
		} else {
			xAxisRValue = 0;
		}
	} else if (axis == SDL_CONTROLLER_AXIS_RIGHTY) {
		if (std::abs(value) > deadZoneR) {
			yAxisRValue = value;
		} else {
			yAxisRValue = 0;
		}
	}
}
