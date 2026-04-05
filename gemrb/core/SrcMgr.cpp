// SPDX-FileCopyrightText: 2022 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

// these two classes handle using and caching of pst SRC files

#include "SrcMgr.h"

#include "GameData.h"

#include "Streams/DataStream.h"

namespace GemRB {

const SrcVector* SrcMgr::GetSrc(const ResRef& resource)
{
	auto lookup = srcs.find(resource);
	if (lookup != srcs.cend()) {
		return &lookup->second;
	}

	return &srcs.emplace(resource, resource).first->second;
}

SrcVector::SrcVector(const ResRef& resource)
{
	key = resource;

	DataStream* str = gamedata->GetResourceStream(resource, IE_SRC_CLASS_ID, true);
	if (!str) {
		return;
	}

	ieDword size = 0;
	str->ReadDword(size);
	if (size == ieDword(DataStream::Error)) return;
	strings.resize(size);

	while (size--) {
		str->ReadStrRef(strings[size].ref);
		str->ReadDword(strings[size].weight);
		totalWeight += strings[size].weight;
	}

	delete str;
}

// random weighted choice
ieStrRef SrcVector::RandomRef() const
{
	size_t choice = RAND<size_t>(0, totalWeight - 1);
	if (totalWeight == strings.size()) return strings[choice].ref;

	size_t sum = 0;
	size_t weightedChoice = 0;
	for (const auto& srcPair : strings) {
		if (choice <= sum) {
			break;
		}
		sum += srcPair.weight;
		weightedChoice++;
	}
	return strings.at(weightedChoice).ref;
}

}
