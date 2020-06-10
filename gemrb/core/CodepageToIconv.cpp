/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2020 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include <algorithm>

#include "CodepageToIconv.h"

namespace GemRB {

const char* GetIconvNameForCodepage(uint32_t codepage) {
	const CodepageIconvMapEntry searchItem{codepage, ""};

	auto entry =
		std::lower_bound(
			codepageIconvMap.cbegin(),
			codepageIconvMap.cend(),
			searchItem,
			[](const CodepageIconvMapEntry& a, const CodepageIconvMapEntry& b) { return a.first < b.first; }
		);

	if (entry != codepageIconvMap.cend() && entry->first == codepage) {
		return entry->second;
	}

	return nullptr;
}

}
