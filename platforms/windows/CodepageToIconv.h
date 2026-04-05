// SPDX-FileCopyrightText: 2020 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CODEPAGE_TO_ICONV_H
#define CODEPAGE_TO_ICONV_H

#include <array>
#include <cstdint>
#include <utility>

namespace GemRB {

using CodepageIconvMapEntry = std::pair<uint32_t, const char*>;
using CodepageIconvMap = std::array<CodepageIconvMapEntry, 57>;

extern const CodepageIconvMap codepageIconvMap;

const char* GetIconvNameForCodepage(uint32_t);

}

#endif
