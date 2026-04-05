// SPDX-FileCopyrightText: 2023 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Debug.h"

#include "EnumFlags.h"

namespace GemRB {

static DebugMode DebugModes;

bool InDebugMode(DebugMode mode) noexcept
{
	return (DebugModes & mode) != DebugMode::NONE;
}

bool SetDebugMode(DebugMode modes, BitOp op) noexcept
{
	return SetBits(DebugModes, modes, op);
}

}
