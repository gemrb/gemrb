// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef SAVIMPORTER_H
#define SAVIMPORTER_H

#include "ArchiveImporter.h"

#include "Streams/DataStream.h"

namespace GemRB {

class SAVImporter : public ArchiveImporter {
public:
	SAVImporter() noexcept = default;
	int DecompressSaveGame(DataStream* compressed, SaveGameAREExtractor&) override;
	int AddToSaveGame(DataStream* str, DataStream* uncompressed) override;
	int AddToSaveGameCompressed(DataStream* str, DataStream* compressed) override;
	int CreateArchive(DataStream* compressed) override;
};

}

#endif
