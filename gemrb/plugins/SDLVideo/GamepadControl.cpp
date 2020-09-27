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

#include "GamepadControl.h"

void GamepadControl::SetPointerSpeed(int pointerSpeed)
{
    joyPointerSpeed = pointerSpeed / JOY_SPEED_MOD;
}

float GamepadControl::GetPointerSpeed()
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
        if(std::abs(value) > JOY_L_DEADZONE) {
            xAxisLValue = value;
        } else {
            xAxisLValue = 0;
        }
    } else if (axis == SDL_CONTROLLER_AXIS_LEFTY) {
        if(std::abs(value) > JOY_L_DEADZONE) {
            yAxisLValue = value;
        } else {
            yAxisLValue = 0;
        }
    } else if (axis == SDL_CONTROLLER_AXIS_RIGHTX) {
        xAxisRValue = value;
    } else if (axis == SDL_CONTROLLER_AXIS_RIGHTY) {
        yAxisRValue = value;
    }
}
