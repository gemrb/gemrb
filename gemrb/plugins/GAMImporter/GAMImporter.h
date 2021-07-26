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

#include "SaveGameMgr.h"

#include "ActorMgr.h"

namespace GemRB {

#define GAM_VER_GEMRB  0 
#define GAM_VER_BG  10   
#define GAM_VER_IWD 11  
#define GAM_VER_PST 12 
#define GAM_VER_BG2 20 
#define GAM_VER_TOB 21 
#define GAM_VER_IWD2 22

class GAMImporter : public SaveGameMgr {
private:
	int version;
	unsigned int PCSize;
	ieDword PCOffset, PCCount;
	ieDword MazeOffset;
	ieDword NPCOffset, NPCCount;
	ieDword GlobalOffset, GlobalCount;
	ieDword JournalOffset, JournalCount;
	ieDword KillVarsOffset, KillVarsCount;
	ieDword FamiliarsOffset;
	ieDword SavedLocOffset, SavedLocCount;
	ieDword PPLocOffset, PPLocCount;
public:
	GAMImporter(void);

	Game* LoadGame(Game *newGame, int ver_override = 0) override;

	int GetStoredFileSize(Game *game) override;
	/* stores a gane in the savegame folder */
	int PutGame(DataStream *stream, Game *game) override;
private:
	bool Import(DataStream* stream) override;

	Actor* GetActor(const Holder<ActorMgr>& aM, bool is_in_party);
	void GetPCStats(PCStatsStruct* ps, bool extended);
	GAMJournalEntry* GetJournalEntry();

	int PutHeader(DataStream *stream, Game *game) const;
	int PutActor(DataStream *stream, Actor *ac, ieDword CRESize, ieDword CREOffset, ieDword version);
	int PutPCs(DataStream *stream, const Game *game);
	int PutNPCs(DataStream *stream, const Game *game);
	int PutJournals(DataStream *stream, const Game *game) const;
	int PutVariables( DataStream *stream, const Game *game) const;
	int PutKillVars(DataStream *stream, const Game *game) const;
	void GetMazeHeader(void *memory);
	void GetMazeEntry(void *memory);
	void PutMazeHeader(DataStream *stream, void *memory);
	void PutMazeEntry(DataStream *stream, void *memory);
	int PutMaze(DataStream *stream, const Game *game);
	int PutFamiliars(DataStream *stream, Game *game) const;
	int PutSavedLocations(DataStream *stream, Game *game) const;
	int PutPlaneLocations(DataStream *stream, Game *game) const;
};

#endif
}


