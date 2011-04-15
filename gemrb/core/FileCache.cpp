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
 */

#include "FileCache.h"

#include "Compressor.h"
#include "Interface.h"
#include "System/FileStream.h"
#include "System/VFS.h"

DataStream* CacheCompressedStream(DataStream *stream, const char* filename, int length, bool overwrite)
{
	if (!core->IsAvailable(PLUGIN_COMPRESSION_ZLIB)) {
		print( "No Compression Manager Available.\nCannot Load Compressed File.\n" );
		return NULL;
	}

	char fname[_MAX_PATH];
	ExtractFileFromPath(fname, filename);
	char path[_MAX_PATH];
	PathJoin(path, core->CachePath, fname, NULL);

	if (overwrite || !file_exists(path)) {
		FileStream out;
		if (!out.Create(path)) {
			printMessage("FileCache", " ", RED);
			print( "Cannot write %s.\n", path );
			return NULL;
		}

		PluginHolder<Compressor> comp(PLUGIN_COMPRESSION_ZLIB);
		if (comp->Decompress(&out, stream, length) != GEM_OK)
			return NULL;
	} else {
		stream->Seek(length, GEM_CURRENT_POS);
	}
	return FileStream::OpenFile(path);
}

DataStream* CacheFile(const char* path)
{
	if (!core->GameOnCD)
		return FileStream::OpenFile(path);

	char filename[_MAX_PATH];
	char cachedfile[_MAX_PATH];
	ExtractFileFromPath(filename, path);
	PathJoin(cachedfile, core->CachePath, filename, NULL);

	if (!file_exists(cachedfile)) {    // File was not found in cache
		FileStream* src = FileStream::OpenFile(path);
		FileStream* dest = FileStream::OpenFile(cachedfile);
		if (!src || !dest) {
			print("CachdFile failed to write to cached file '%s' (from '%s')\n", cachedfile, path);
			abort();
		}

		size_t blockSize = 1024 * 1000;
		char buff[1024 * 1000];
		do {
			if (blockSize > src->Remains())
				blockSize = src->Remains();
			size_t len = src->Read(buff, blockSize);
			size_t c = dest->Write(buff, len);
			if (c != len) {
				print("CacheFile failed to write to cached file '%s' (from '%s')\n", cachedfile, path);
				abort();
			}
		} while (src->Remains());
		delete src;
		delete dest;
	}
	return FileStream::OpenFile(cachedfile);
}
