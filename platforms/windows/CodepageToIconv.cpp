// SPDX-FileCopyrightText: 2020 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CodepageToIconv.h"

#include <algorithm>

namespace GemRB {

const char* GetIconvNameForCodepage(uint32_t codepage)
{
	const CodepageIconvMapEntry searchItem { codepage, "" };

	auto entry =
		std::lower_bound(
			codepageIconvMap.cbegin(),
			codepageIconvMap.cend(),
			searchItem,
			[](const CodepageIconvMapEntry& a, const CodepageIconvMapEntry& b) { return a.first < b.first; });

	if (entry != codepageIconvMap.cend() && entry->first == codepage) {
		return entry->second;
	}

	return nullptr;
}

}
