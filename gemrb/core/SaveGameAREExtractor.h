/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2021 The GemRB Project
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
