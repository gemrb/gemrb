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

#include "win32def.h"

#include "Interface.h"

namespace GemRB {

#ifdef WIN32
struct FileStream::File {
private:
	HANDLE file;
public:
	File() : file() {}
	void Close() { CloseHandle(file); }
	size_t Length() {
		LARGE_INTEGER size;
		DWORD high;
		DWORD low = GetFileSize(file, &high);
		if (low != 0xFFFFFFFF || GetLastError() == NO_ERROR) {
			size.LowPart = low;
			size.HighPart = high;
			return size.QuadPart;
		}
		return 0;
	}
	bool OpenRO(const char *name) {
		file = CreateFile(name,
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL, NULL);
		return (file != INVALID_HANDLE_VALUE);
	}
	bool OpenRW(const char *name) {
		file = CreateFile(name,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL, NULL);
		return (file != INVALID_HANDLE_VALUE);
	}
	bool OpenNew(const char *name) {
		file = CreateFile(name,
			GENERIC_WRITE,
			FILE_SHARE_READ,
			NULL,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL, NULL);
		return (file != INVALID_HANDLE_VALUE);
	}
	size_t Read(void* ptr, size_t length) {
		unsigned long read;
		if (!ReadFile(file, ptr, length, &read, NULL))
			return 0;
		return read;
	}
	size_t Write(const void* ptr, size_t length) {
		unsigned long wrote;
		if (!WriteFile(file, ptr, length, &wrote, NULL))
			return 0;
		return wrote;
	}
	bool SeekStart(int offset)
	{
		return SetFilePointer(file, offset, NULL, FILE_BEGIN) != 0xffffffff;
	}
	bool SeekCurrent(int offset)
	{
		return SetFilePointer(file, offset, NULL, FILE_CURRENT) != 0xffffffff;
	}
	bool SeekEnd(int offset)
	{
		return SetFilePointer(file, offset, NULL, FILE_END) != 0xffffffff;
	}
};
#else
struct FileStream::File {
private:
	FILE* file;
public:
	File() : file(NULL) {}
	void Close() { fclose(file); }
	size_t Length() {
		fseek(file, 0, SEEK_END);
		size_t size = ftell(file);
		fseek(file, 0, SEEK_SET);
		return size;
	}
	bool OpenRO(const char *name) {
		return (file = fopen(name, "rb"));
	}
	bool OpenRW(const char *name) {
		return (file = fopen(name, "r+b"));
	}
	bool OpenNew(const char *name) {
		return (file = fopen(name, "wb"));
	}
	size_t Read(void* ptr, size_t length) {
		return fread(ptr, 1, length, file);
	}
	size_t Write(const void* ptr, size_t length) {
		return fwrite(ptr, 1, length, file);
	}
	bool SeekStart(int offset)
	{
		return !fseek(file, offset, SEEK_SET);
	}
	bool SeekCurrent(int offset)
	{
		return !fseek(file, offset, SEEK_CUR);
	}
	bool SeekEnd(int offset)
	{
		return !fseek(file, offset, SEEK_END);
	}
};
#endif

FileStream::FileStream(void)
{
	opened = false;
	created = false;
	str = new File();
}

DataStream* FileStream::Clone()
{
	return OpenFile(originalfile);
}

FileStream::~FileStream(void)
{
	Close();
	delete str;
}

void FileStream::Close()
{
	if (opened) {
#ifdef _DEBUG
		core->FileStreamPtrCount--;
#endif
		str->Close();
	}
	opened = false;
	created = false;
}

void FileStream::FindLength()
{
	size = str->Length();
	Pos = 0;
}

bool FileStream::Open(const char* fname)
{
	Close();

	if (!file_exists(fname)) {
		return false;
	}

	if (!str->OpenRO(fname)) {
		return false;
	}
#ifdef _DEBUG
	core->FileStreamPtrCount++;
#endif
	opened = true;
	created = false;
	FindLength();
	ExtractFileFromPath( filename, fname );
	strncpy( originalfile, fname, _MAX_PATH);
	return true;
}

bool FileStream::Modify(const char* fname)
{
	Close();

	if (!str->OpenRW(fname)) {
		return false;
	}
#ifdef _DEBUG
	core->FileStreamPtrCount++;
#endif
	opened = true;
	created = true;
	FindLength();
	ExtractFileFromPath( filename, fname );
	strncpy( originalfile, fname, _MAX_PATH);
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
	strncpy(originalfile, path, _MAX_PATH);

	if (!str->OpenNew(originalfile)) {
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
	size_t c = str->Read(dest, length);
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

	size_t c = str->Write(src, length);
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
			str->SeekStart(size - newpos);
			Pos = size - newpos;
			break;
		case GEM_CURRENT_POS:
			str->SeekCurrent(newpos);
			Pos += newpos;
			break;

		case GEM_STREAM_START:
			str->SeekStart(newpos);
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
