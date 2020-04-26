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
