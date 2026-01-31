/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef GAMIMPORTER_H
	#define GAMIMPORTER_H

	#include "versions.h"

	#include "ActorMgr.h"
	#include "SaveGameMgr.h"

namespace GemRB {

class PCStatsStruct;

class GAMImporter : public SaveGameMgr {
private:
	GAMVersion version = GAMVersion::GemRB;
	unsigned int PCSize = 0;
	ieDword PCOffset = 0;
	ieDword PCCount = 0;
	ieDword MazeOffset = 0;
	ieDword NPCOffset = 0;
	ieDword NPCCount = 0;
	ieDword GlobalOffset = 0;
	ieDword GlobalCount = 0;
	ieDword JournalOffset = 0;
	ieDword JournalCount = 0;
	ieDword KillVarsOffset = 0;
	ieDword KillVarsCount = 0;
	ieDword FamiliarsOffset = 0;
	ieDword SavedLocOffset = 0;
	ieDword SavedLocCount = 0;
	ieDword PPLocOffset = 0;
	ieDword PPLocCount = 0;

public:
	GAMImporter() noexcept = default;

	Game* LoadGame(Game* newGame, GAMVersion override = GAMVersion::GemRB) override;

	int GetStoredFileSize(const Game* game) override;
	/* stores a gane in the savegame folder */
	int PutGame(DataStream* stream, Game* game) const override;

private:
	bool Import(DataStream* stream) override;

	Actor* GetActor(const std::shared_ptr<ActorMgr>& aM, bool is_in_party);
	void GetPCStats(PCStatsStruct& ps, bool extended);
	GAMJournalEntry* GetJournalEntry();

	int PutHeader(DataStream* stream, const Game* game) const;
	int PutActor(DataStream* stream, const Actor* ac, ieDword CRESize, ieDword CREOffset, GAMVersion newVersion) const;
	int PutPCs(DataStream* stream, const Game* game) const;
	int PutNPCs(DataStream* stream, const Game* game) const;
	int PutJournals(DataStream* stream, const Game* game) const;
	int PutVariables(DataStream* stream, const Game* game) const;
	int PutKillVars(DataStream* stream, const Game* game) const;
	void GetMazeHeader(void* memory) const;
	void GetMazeEntry(void* memory) const;
	void PutMazeHeader(DataStream* stream, void* memory) const;
	void PutMazeEntry(DataStream* stream, void* memory) const;
	int PutMaze(DataStream* stream, const Game* game) const;
	int PutFamiliars(DataStream* stream, const Game* game) const;
	int PutSavedLocations(DataStream* stream, Game* game) const;
	int PutPlaneLocations(DataStream* stream, Game* game) const;
};

#endif
}
