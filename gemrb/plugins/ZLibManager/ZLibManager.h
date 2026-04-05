// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef ZLIBMANAGER_H
#define ZLIBMANAGER_H

#include "Compressor.h"

namespace GemRB {

class ZLibManager : public Compressor {
public:
	ZLibManager() noexcept = default;
	// ZLib Decompression Routine
	int Decompress(DataStream* dest, DataStream* source, unsigned int size_guess) const override;
	// ZLib Compression
	int Compress(DataStream* dest, DataStream* source) const override;
};

}

#endif
