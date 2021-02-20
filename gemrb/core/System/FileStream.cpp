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
 *
 *
 */

#include "System/FileStream.h"

#include "Interface.h"

namespace GemRB {

#ifdef WIN32

#define TCHAR_NAME(name) \
	TCHAR t_name[MAX_PATH] = {0}; \
	mbstowcs(t_name, name, MAX_PATH - 1);

#endif

FileStream::FileStream(File&& f)
: str(std::move(f)), opened(true), created(true)
{}

FileStream::FileStream(void)
{
	opened = false;
	created = false;
}

DataStream* FileStream::Clone()
{
	return OpenFile(originalfile);
}

void FileStream::Close()
{
	str = File();
	opened = false;
	created = false;
}

void FileStream::FindLength()
{
	size = str.Length();
	Pos = 0;
}

bool FileStream::Open(const char* fname)
{
	Close();

	if (!file_exists(fname)) {
		return false;
	}

	if (!str.OpenRO(fname)) {
		return false;
	}
	opened = true;
	created = false;
	FindLength();
	ExtractFileFromPath( filename, fname );
	strlcpy( originalfile, fname, _MAX_PATH);
	return true;
}

bool FileStream::Modify(const char* fname)
{
	Close();

	if (!str.OpenRW(fname)) {
		return false;
	}
	opened = true;
	created = true;
	FindLength();
	ExtractFileFromPath( filename, fname );
	strlcpy( originalfile, fname, _MAX_PATH);
	Pos = 0;
	return true;
}

//Creating file in the cache
bool FileStream::Create(const char* fname, SClass_ID ClassID)
{
	return Create(core->CachePath, fname, ClassID);
}

bool FileStream::Create(const char *folder, const char* fname, SClass_ID ClassID)
{
	char path[_MAX_PATH];
	char filename[_MAX_PATH];
	ExtractFileFromPath( filename, fname );
	PathJoinExt(path, folder, filename, core->TypeExt(ClassID));
	return Create(path);
}

//Creating file outside of the cache
bool FileStream::Create(const char *path)
{
	Close();

	ExtractFileFromPath( filename, path );
	strlcpy(originalfile, path, _MAX_PATH);

	if (!str.OpenNew(originalfile)) {
		return false;
	}
	opened = true;
	created = true;
	Pos = 0;
	size = 0;
	return true;
}

int FileStream::Read(void* dest, unsigned int length)
{
	if (!opened) {
		return GEM_ERROR;
	}
	//we don't allow partial reads anyway, so it isn't a problem that
	//i don't adjust length here (partial reads are evil)
	if (Pos+length>size ) {
		return GEM_ERROR;
	}
	size_t c = str.Read(dest, length);
	if (c != length) {
		return GEM_ERROR;
	}
	if (Encrypted) {
		ReadDecrypted( dest, c );
	}
	Pos += c;
	return c;
}

int FileStream::Write(const void* src, unsigned int length)
{
	if (!created) {
		return GEM_ERROR;
	}
	// do encryption here if needed

	size_t c = str.Write(src, length);
	if (c != length) {
		return GEM_ERROR;
	}
	Pos += c;
	if (Pos>size) {
		size = Pos;
	}
	return c;
}

int FileStream::Seek(int newpos, int type)
{
	if (!opened && !created) {
		return GEM_ERROR;
	}
	switch (type) {
		case GEM_STREAM_END:
			str.SeekStart(size - newpos);
			Pos = size - newpos;
			break;
		case GEM_CURRENT_POS:
			str.SeekCurrent(newpos);
			Pos += newpos;
			break;

		case GEM_STREAM_START:
			str.SeekStart(newpos);
			Pos = newpos;
			break;

		default:
			return GEM_ERROR;
	}
	if (Pos>size) {
		print("[Streams]: Invalid seek position %ld in file %s(limit: %ld)", Pos, filename, size);
		return GEM_ERROR;
	}
	return GEM_OK;
}

FileStream* FileStream::OpenFile(const char* filename)
{
	FileStream *fs = new FileStream();
	if (fs->Open(filename))
		return fs;

	delete fs;
	return NULL;
}

}
