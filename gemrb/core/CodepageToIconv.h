#ifndef CODEPAGE_TO_ICONV_H
#define CODEPAGE_TO_ICONV_H

#include <array>
#include <utility>

namespace GemRB {

using CodepageIconvMapEntry = std::pair<uint32_t, const char*>;
using CodepageIconvMap = std::array<CodepageIconvMapEntry, 56>;

extern const CodepageIconvMap codepageIconvMap;

const char* GetIconvNameForCodepage(uint32_t);

}

#endif
