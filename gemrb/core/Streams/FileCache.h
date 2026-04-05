// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef FILECACHE_H
#define FILECACHE_H

#include "Streams/DataStream.h"

namespace GemRB {

GEM_EXPORT DataStream* CacheCompressedStream(DataStream* stream, const path_t& filename, int length = 0, bool overwrite = false);

}

#endif
