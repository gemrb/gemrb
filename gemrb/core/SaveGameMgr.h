// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SAVEGAMEMGR_H
#define SAVEGAMEMGR_H

#include "Game.h"
#include "Plugin.h"

namespace GemRB {

class GEM_EXPORT SaveGameMgr : public ImporterBase {
public:
	virtual std::unique_ptr<Game> LoadGame(std::unique_ptr<Game> newGame, GAMVersion verOverride = GAMVersion::GemRB) = 0;

	virtual int GetStoredFileSize(const Game* game) = 0;
	virtual int PutGame(DataStream* stream, Game* game) const = 0;
};

}

#endif
