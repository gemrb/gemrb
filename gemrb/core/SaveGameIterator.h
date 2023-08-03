/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

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
	int CreateSaveGame(Holder<SaveGame>, StringView slotname, bool force = false) const;
	int CreateSaveGame(int index, bool mqs = false) const;
	Holder<SaveGame> GetSaveGame(StringView slotname);
private:
	bool RescanSaveGames();
	static Holder<SaveGame> BuildSaveGame(std::string slotname);
	void PruneQuickSave(StringView folder) const;
};

}

#endif
