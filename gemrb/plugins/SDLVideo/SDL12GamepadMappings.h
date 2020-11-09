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

#ifndef SDL12GAMEPADMAPPINGS_H
#define SDL12GAMEPADMAPPINGS_H

#ifdef VITA
#define SDL_CONTROLLER_AXIS_LEFTX 0
#define SDL_CONTROLLER_AXIS_LEFTY 1
#define SDL_CONTROLLER_AXIS_RIGHTX 2
#define SDL_CONTROLLER_AXIS_RIGHTY 3
#define SDL_CONTROLLER_BUTTON_A 2
#define SDL_CONTROLLER_BUTTON_B 1
#define SDL_CONTROLLER_BUTTON_X 3
#define SDL_CONTROLLER_BUTTON_Y 0
#define SDL_CONTROLLER_BUTTON_BACK 11
#define SDL_CONTROLLER_BUTTON_START 10
#define SDL_CONTROLLER_BUTTON_LEFTSHOULDER 4
#define SDL_CONTROLLER_BUTTON_RIGHTSHOULDER 5
#define SDL_CONTROLLER_BUTTON_DPAD_UP 8
#define SDL_CONTROLLER_BUTTON_DPAD_DOWN 6
#define SDL_CONTROLLER_BUTTON_DPAD_LEFT 7
#define SDL_CONTROLLER_BUTTON_DPAD_RIGHT 9
#else
//XBone mapping on Linux. Some buttons aren't working in SDL12
#define SDL_CONTROLLER_AXIS_LEFTX 0
#define SDL_CONTROLLER_AXIS_LEFTY 1
#define SDL_CONTROLLER_AXIS_RIGHTX 2
#define SDL_CONTROLLER_AXIS_RIGHTY 3
#define SDL_CONTROLLER_BUTTON_A 0
#define SDL_CONTROLLER_BUTTON_B 1
#define SDL_CONTROLLER_BUTTON_X 3
#define SDL_CONTROLLER_BUTTON_Y 4
#define SDL_CONTROLLER_BUTTON_BACK 100
#define SDL_CONTROLLER_BUTTON_START 11
#define SDL_CONTROLLER_BUTTON_LEFTSHOULDER 6
#define SDL_CONTROLLER_BUTTON_RIGHTSHOULDER 7
#define SDL_CONTROLLER_BUTTON_DPAD_UP 101
#define SDL_CONTROLLER_BUTTON_DPAD_DOWN 102
#define SDL_CONTROLLER_BUTTON_DPAD_LEFT 103
#define SDL_CONTROLLER_BUTTON_DPAD_RIGHT 104
#endif

#endif
