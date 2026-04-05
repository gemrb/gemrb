// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef ARCHIVEIMPORTER_H
#define ARCHIVEIMPORTER_H

#include "globals.h"

#include "Plugin.h"
#include "SaveGameAREExtractor.h"

namespace GemRB {

class GEM_EXPORT ArchiveImporter : public Plugin {
public:
	virtual int CreateArchive(DataStream* stream) = 0;
	//decompressing a .sav file similar to CBF
	virtual int DecompressSaveGame(DataStream* compressed, SaveGameAREExtractor&) = 0;
	virtual int AddToSaveGame(DataStream* str, DataStream* uncompressed) = 0;
	virtual int AddToSaveGameCompressed(DataStream* str, DataStream* compressed) = 0;
};

}

#endif
