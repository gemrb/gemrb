/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2023 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef GAMECONTROLDEFS_H
#define GAMECONTROLDEFS_H

// dialog flags
#define DF_IN_DIALOG          1
#define DF_TALKCOUNT          2
#define DF_UNBREAKABLE        4
#define DF_FREEZE_SCRIPTS     8
#define DF_INTERACT           16
#define DF_IN_CONTAINER       32
#define DF_OPENCONTINUEWINDOW 64
#define DF_OPENENDWINDOW      128
#define DF_POSTPONE_SCRIPTS   256

// scroll flags for tracking arrow key scrolling
static constexpr unsigned int SK_LEFT = 1;
static constexpr unsigned int SK_RIGHT = 2;
static constexpr unsigned int SK_UP = 4;
static constexpr unsigned int SK_DOWN = 8;

// screen flags
// !!! Keep these synchronized with GUIDefines.py !!!
enum class ScreenFlags : unsigned int {
	CenterOnActor = 0,
	AlwaysCenter = 1,
	Cutscene = 2, // don't push new actions onto the action queue
	PlayingMovie = 3,

	count = 4
};

// target modes and types
// !!! Keep these synchronized with GUIDefines.py !!!
enum class TargetMode {
	None,
	Talk,
	Attack,
	Cast,
	Defend,
	Pick
};

#endif // GAMECONTROLDEFS_H
