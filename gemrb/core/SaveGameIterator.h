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

#define SAVEGAME_DIRECTORY_MATCHER "%d - %[A-Za-z0-9- _+*#%&|()=!?':;]"

class GEM_EXPORT SaveGameIterator {
private:
	typedef std::vector<Holder<SaveGame> > charlist;
	charlist save_slots;

public:
	SaveGameIterator(void);
	~SaveGameIterator(void);
	const charlist& GetSaveGames();
	void DeleteSaveGame(Holder<SaveGame>);
	int CreateSaveGame(Holder<SaveGame>, const char *slotname);
	int CreateSaveGame(int index, bool mqs = false);
	Holder<SaveGame> GetSaveGame(const char *slotname);
private:
	bool RescanSaveGames();
	static Holder<SaveGame> BuildSaveGame(const char *slotname);
	void PruneQuickSave(const char *folder);
};

}

#endif
