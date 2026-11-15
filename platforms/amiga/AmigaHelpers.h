// SPDX-FileCopyrightText: 2026 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef AMIGAHELPERS_H
#define AMIGAHELPERS_H

#include "../../gemrb/core/Streams/DataStream.h"

#include <cstddef>

namespace GemRB {

bool AmigaMMap(void* fileHandle, strpos_t size, char*& data);

};

#endif
