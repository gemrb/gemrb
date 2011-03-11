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

DataStream* CacheCompressedStream(DataStream *stream, const char* filename, int  length)
{
	if (!core->IsAvailable(PLUGIN_COMPRESSION_ZLIB)) {
		printf( "No Compression Manager Available.\nCannot Load Compressed File.\n" );
		return NULL;
	}

	char fname[_MAX_PATH];
	ExtractFileFromPath(fname, filename);
	char path[_MAX_PATH];
	PathJoin(path, core->CachePath, fname, NULL);

	if (!file_exists(path)) {
		FileStream out;
		if (!out.Create(path)) {
			printMessage("FileCache", " ", RED);
			printf( "Cannot write %s.\n", path );
			return NULL;
		}

		PluginHolder<Compressor> comp(PLUGIN_COMPRESSION_ZLIB);
		comp->Decompress(&out, stream, length);
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
		FILE* src = _fopen(path, "rb");
		FILE* dest = _fopen(cachedfile, "wb");
		char buff[1024 * 1000];
		do {
			size_t len = _fread(buff, 1, 1024 * 1000, src);
			size_t c = _fwrite(buff, 1, len, dest);
			if (c != len) {
				printf("CachedFileStream failed to write to cached file '%s' (from '%s')\n", cachedfile, path);
				abort();
			}
		} while (!_feof(src));
		_fclose(src);
		_fclose(dest);
	}
	return FileStream::OpenFile(cachedfile);
}
