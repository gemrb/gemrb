/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2022 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

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
