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
#include "Interface.h"
#include "FileCache.h"
#include "System/FileStream.h"
#include "SaveGameAREExtractor.h"

namespace GemRB {

SaveGameAREExtractor::SaveGameAREExtractor(SaveGame *saveGame)
	: saveGame(saveGame)
{
	if (saveGame != nullptr) {
		saveGame->acquire();
	}
}

SaveGameAREExtractor::~SaveGameAREExtractor() {
	if (saveGame != nullptr) {
		saveGame->release();
	}
}

int32_t SaveGameAREExtractor::copyRetainedAREs(DataStream *destStream, bool trackLocations) {
	if (saveGame == nullptr) {
		return GEM_OK;
	}

	auto saveGameStream = saveGame->GetSave();
	if (saveGameStream == nullptr) {
		return GEM_ERROR;
	}

	if (trackLocations) {
		newAreLocations.clear();
	}

	using BufferT = std::array<uint8_t, 4096>;
	BufferT buffer{};
	int32_t i = 0;

	size_t relativeLocation = 0;
	for (auto it = areLocations.cbegin(); it != areLocations.cend(); ++it, ++i) {
		relativeLocation += 4 + it->first.size() + 1;

		ieDword complen, declen;
		saveGameStream->Seek(it->second, GEM_STREAM_START);
		saveGameStream->ReadDword(declen);
		saveGameStream->ReadDword(complen);

		ieDword nameLength = ieDword(it->first.size() + 1);
		destStream->WriteDword(nameLength);
		destStream->Write(it->first.c_str(), nameLength);
		destStream->WriteDword(declen);
		destStream->WriteDword(complen);

		if (trackLocations) {
			newAreLocations.emplace(std::make_pair(it->first, relativeLocation));
			relativeLocation += 8 + complen;
		}

		BufferT::size_type remaining = complen;
		while (remaining > 0) {
			auto copySize = std::min(buffer.size(), remaining);
			saveGameStream->Read(buffer.data(), copySize);
			destStream->Write(buffer.data(), copySize);
			remaining -= copySize;
		}
	}

	delete saveGameStream;

	return i;
}

int32_t SaveGameAREExtractor::createCacheBlob() {
	if (areLocations.empty()) {
		return 0;
	}

	const char *blobFile = "ares.blb";
	char path[_MAX_PATH];
	PathJoin(path, core->config.CachePath, blobFile, nullptr);

	FileStream cacheStream;

	if (!cacheStream.Create(path)) {
		Log(ERROR, "SaveGameAREExtractor", "Cannot write to cache: %s.", path);
		return GEM_ERROR;
	}

	int32_t areEntries = copyRetainedAREs(&cacheStream, true);

	return areEntries;
}

int32_t SaveGameAREExtractor::extractARE(const char *name) {
	std::string key{name};
	StringToLower(key);
	key.append(".are");

	auto it = areLocations.find(key);
	if (it != areLocations.cend() && extractByEntry(key, it) != GEM_OK) {
		return GEM_ERROR;
	}

	return GEM_OK;
}

int32_t SaveGameAREExtractor::extractByEntry(const std::string& key, RegistryT::const_iterator it) {
	auto saveGameStream = saveGame->GetSave();
	if (saveGameStream == nullptr) {
		return GEM_ERROR;
	}

	ieDword complen, declen;
	saveGameStream->Seek(it->second, GEM_STREAM_START);
	saveGameStream->ReadDword(declen);
	saveGameStream->ReadDword(complen);

	DataStream* cached = CacheCompressedStream(saveGameStream, key.c_str(), complen, true);

	int32_t returnValue = GEM_OK;
	if (cached != nullptr) {
		delete cached;
	} else {
		returnValue = GEM_ERROR;
	}

	delete saveGameStream;
	areLocations.erase(it);

	return returnValue;
}

bool SaveGameAREExtractor::isRunningSaveGame(const SaveGame& otherGame) const
{
	if (saveGame == nullptr) {
		return false;
	}

	return saveGame->GetSaveID() == otherGame.GetSaveID();
}

void SaveGameAREExtractor::registerLocation(const char *name, unsigned long pos) {
	std::string key{name};
	StringToLower(key);

	areLocations.emplace(std::make_pair(std::move(key), pos));
}

void SaveGameAREExtractor::changeSaveGame(SaveGame *saveGame) {
	if (this->saveGame != nullptr) {
		this->saveGame->release();
	}

	this->saveGame = saveGame;
	if (this->saveGame != nullptr) {
		this->saveGame->acquire();
	}

	areLocations.clear();
	newAreLocations.clear();
}

void SaveGameAREExtractor::updateSaveGame(size_t offset) {
	if (saveGame == nullptr) {
		return;
	}

	areLocations = std::move(newAreLocations);

	for (auto it = areLocations.begin(); it != areLocations.end(); ++it) {
		it->second += offset;
	}
}

}
