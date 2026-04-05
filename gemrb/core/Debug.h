// SPDX-FileCopyrightText: 2023 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef DEBUG_H
#define DEBUG_H

#include "globals.h"

namespace GemRB {

enum class DebugMode : uint32_t {
	NONE = 0,
	REFERENCE = 1, // unused
	CUTSCENE = 2,
	VARIABLES = 4,
	ACTIONS = 8,
	TRIGGERS = 16,
	VIEWS = 32,
	WINDOWS = 64,
	FONTS = 128,
	TEXT = 256,
	PATHFINDER = 512,
	PROJECTILES = 1024,
};

bool InDebugMode(DebugMode modes) noexcept;
bool SetDebugMode(DebugMode modes, BitOp op = BitOp::SET) noexcept;

}

#endif // DEBUG_H
