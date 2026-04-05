// SPDX-FileCopyrightText: 2023 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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

	count = 3
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
