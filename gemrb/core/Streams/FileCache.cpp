// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "FileCache.h"

#include "Compressor.h"
#include "Interface.h"
#include "PluginMgr.h"

#include "Logging/Logging.h"
#include "Streams/FileStream.h"
#if defined(SUPPORTS_MEMSTREAM)
	#include "Streams/MappedFileMemoryStream.h"
#endif
#include "System/VFS.h"

namespace GemRB {

DataStream* CacheCompressedStream(DataStream* stream, const path_t& filename, int length, bool overwrite)
{
	path_t fname = ExtractFileFromPath(filename);
	path_t path = PathJoin(core->config.CachePath, fname);

	if (overwrite || !FileExists(path)) {
		FileStream out;
		if (!out.Create(path)) {
			Log(ERROR, "FileCache", "Cannot write {}.", path);
			return NULL;
		}

		PluginHolder<Compressor> comp = MakePluginHolder<Compressor>(PLUGIN_COMPRESSION_ZLIB);
		if (comp->Decompress(&out, stream, length) != GEM_OK)
			return NULL;
	} else {
		stream->Seek(length, GEM_CURRENT_POS);
	}
#if defined(SUPPORTS_MEMSTREAM)
	return new MappedFileMemoryStream { path };
#else
	return FileStream::OpenFile(path);
#endif
}

}
