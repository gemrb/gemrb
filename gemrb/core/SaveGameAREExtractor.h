// SPDX-FileCopyrightText: 2021 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef SAVE_GAME_ARE_EXTRACTOR_H
#define SAVE_GAME_ARE_EXTRACTOR_H

#include "exports.h"

#include "Resource.h"
#include "SaveGame.h"

namespace GemRB {

/**
 * This thing knows the currently loaded game, and SAVImporter already told
 * us where to find what ARE files. So we can extract them only when required.
 */
class GEM_EXPORT SaveGameAREExtractor {
private:
	using RegistryT = ResRefMap<unsigned long>;

	Holder<SaveGame> saveGame;
	RegistryT areLocations;
	RegistryT newAreLocations;

	int32_t extractByEntry(const ResRef&, RegistryT::const_iterator);

public:
	explicit SaveGameAREExtractor(Holder<SaveGame> saveGame = nullptr);

	int32_t copyRetainedAREs(DataStream*, bool trackLocations = false);
	int32_t createCacheBlob();
	int32_t extractARE(const ResRef& resRef);
	bool isRunningSaveGame(const SaveGame&) const;
	void registerLocation(const ResRef& resRef, unsigned long);
	void registerNewLocation(const path_t&, unsigned long);
	void updateSaveGame(size_t offset);
};

}

#endif
