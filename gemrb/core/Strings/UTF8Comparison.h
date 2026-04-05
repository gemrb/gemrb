// SPDX-FileCopyrightText: 2023 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef UTF8_COMPARISON_H
#define UTF8_COMPARISON_H

#include "exports.h"

namespace GemRB {

// true if equal under C.UTF-8 `tolower` regime
GEM_EXPORT bool UTF8_stricmp(const char* a, const char* b);

}

#endif
