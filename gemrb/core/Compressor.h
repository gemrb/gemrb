// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef COMPRESSOR_H
#define COMPRESSOR_H

#include "Plugin.h"

#include "Streams/DataStream.h"

namespace GemRB {

class GEM_EXPORT Compressor : public Plugin {
public:
	/** decompresses a datastream (memory or file) to a FILE * stream */
	virtual int Decompress(DataStream* dest, DataStream* source, unsigned int size_guess = 0) const = 0;
	/** compresses a datastream (memory or file) to another DataStream */
	virtual int Compress(DataStream* dest, DataStream* source) const = 0;
};

}

#endif
