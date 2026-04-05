// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef SAVEGAMEITERATOR_H
#define SAVEGAMEITERATOR_H

#include "exports.h"

#include "SaveGame.h"

#include <vector>

namespace GemRB {

#define SAVEGAME_DIRECTORY_MATCHER "%d - %[^\n\\/]"

class GEM_EXPORT SaveGameIterator {
private:
	using charlist = std::vector<Holder<SaveGame>>;
	charlist save_slots;

public:
	SaveGameIterator() noexcept = default;
	~SaveGameIterator() noexcept = default;
	const charlist& GetSaveGames();
	void DeleteSaveGame(const Holder<SaveGame>&) const;
	int CreateSaveGame(Holder<SaveGame> save, const String& slotname, bool force = false) const;
	int CreateSaveGame(Holder<SaveGame>, StringView slotname, bool force = false) const;
	int CreateSaveGame(int index, bool mqs = false) const;
	Holder<SaveGame> GetSaveGame(const String& slotname);

private:
	bool RescanSaveGames();
	static Holder<SaveGame> BuildSaveGame(std::string slotname);
	void PruneQuickSave(StringView folder) const;
};

}

#endif
