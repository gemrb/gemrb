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

FileStream::FileStream(void)
{
	opened = false;
	created = false;
	str = NULL;
	autoFree = false;
}

FileStream::~FileStream(void)
{
	if (autoFree && str) {
#ifdef _DEBUG
		core->FileStreamPtrCount--;
#endif
		_fclose( str );
	}
	str = NULL;
}

bool FileStream::Open(const char* fname, bool aF)
{
	if (str && autoFree) {
#ifdef _DEBUG
		core->FileStreamPtrCount--;
#endif
		_fclose( str );
		str = NULL;
	}

	if (!file_exists(fname)) {
		return false;
	}

	autoFree = aF;
	str = _fopen( fname, "rb" );
	if (str == NULL) {
		return false;
	}
#ifdef _DEBUG
	core->FileStreamPtrCount++;
#endif
	opened = true;
	created = false;
	//FIXME: this is a very lame way to tell the file length
	_fseek( str, 0, SEEK_END );
	size = _ftell( str );
	_fseek( str, 0, SEEK_SET );
	ExtractFileFromPath( filename, fname );
	strncpy( originalfile, fname, _MAX_PATH);
	Pos = 0;
	return true;
}

bool FileStream::Modify(const char* fname, bool aF)
{
	if (str && autoFree) {
#ifdef _DEBUG
		core->FileStreamPtrCount--;
#endif
		_fclose( str );
	}
	autoFree = aF;
	str = _fopen( fname, "r+b" );
	if (str == NULL) {
		return false;
	}
#ifdef _DEBUG
	core->FileStreamPtrCount++;
#endif
	opened = true;
	created = true;
	//FIXME: this is a very lame way to tell the file length
	_fseek( str, 0, SEEK_END );
	size = _ftell( str );
	_fseek( str, 0, SEEK_SET );
	ExtractFileFromPath( filename, fname );
	strncpy( originalfile, fname, _MAX_PATH);
	Pos = 0;
	return true;
}

//Creating file in the cache
//Create is ALWAYS autofree
bool FileStream::Create(const char* fname, SClass_ID ClassID)
{
	return Create(core->CachePath, fname, ClassID);
}

//Creating file outside of the cache
bool FileStream::Create(const char *folder, const char* fname, SClass_ID ClassID)
{
	if (str && autoFree) {
#ifdef _DEBUG
		core->FileStreamPtrCount--;
#endif
		_fclose( str );
	}
	autoFree = true;
	ExtractFileFromPath( filename, fname );
	strcpy( originalfile, folder );
	strcat( originalfile, SPathDelimiter);
	strcat( originalfile, filename );
	strcat( originalfile, core->TypeExt( ClassID ) );
	str = _fopen( originalfile, "wb" );
	if (str == NULL) {
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
	size_t c = _fread( dest, 1, length, str );
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

	size_t c = _fwrite( src, 1, length, str );
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
			_fseek( str, size - newpos, SEEK_SET);
			Pos = size - newpos;
			break;
		case GEM_CURRENT_POS:
			_fseek( str, newpos, SEEK_CUR );
			Pos += newpos;
			break;

		case GEM_STREAM_START:
			_fseek( str, newpos, SEEK_SET );
			Pos = newpos;
			break;

		default:
			return GEM_ERROR;
	}
	if (Pos>size) {
		printf("[Streams]: Invalid seek position %ld in file %s (limit: %ld)\n",Pos, filename, size);
		return GEM_ERROR;
	}
	return GEM_OK;
}

/** No descriptions */
int FileStream::ReadLine(void* buf, unsigned int maxlen)
{
	if(!maxlen) {
		return 0;
	}
	unsigned char * p = ( unsigned char * ) buf;
	if (_feof( str )) {
		p[0]=0;
		return -1;
	}
	if (Pos >= size) {
		p[0]=0;
		return -1;
	}
	unsigned int i = 0;
	while (i < ( maxlen - 1 )) {
		int ch = _fgetc( str );
		if (Pos == size)
			break;
		if (Encrypted) {
			ch ^= GEM_ENCRYPTION_KEY[Pos & 63];
		}
		Pos++;
		if (( ( char ) ch ) == '\n')
			break;
		if (( ( char ) ch ) == '\t')
			ch = ' ';
		if (( ( char ) ch ) != '\r')
			p[i++] = ch;
		//Warning:this feof implementation reads forward one byte
		if (_feof( str ))
			break;
	}
	p[i] = 0;
	return i;
}
