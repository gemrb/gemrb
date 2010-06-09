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

#include "CachedFileStream.h"

#include "win32def.h"

#include "Interface.h"

CachedFileStream::CachedFileStream(const char* stream, bool autoFree)
{
	ExtractFileFromPath( filename, stream );
	PathJoin( originalfile, core->CachePath, filename, NULL );

	str = _fopen( originalfile, "rb" );
	if (str == NULL) {    // File was not found in cache
		if (core->GameOnCD) {
			_FILE* src = _fopen( stream, "rb" );
#ifdef _DEBUG
			core->CachedFileStreamPtrCount++;
#endif
			_FILE* dest = _fopen( originalfile, "wb" );
#ifdef _DEBUG
			core->CachedFileStreamPtrCount++;
#endif
			void* buff = malloc( 1024 * 1000 );
			do {
				size_t len = _fread( buff, 1, 1024 * 1000, src );
				size_t c = _fwrite( buff, 1, len, dest );
				if (c != len) {
					printf("CachedFileStream failed to write to cached file '%s' (from '%s')\n", originalfile, stream);
					abort();
				}
			} while (!_feof( src ));
			free( buff );
			_fclose( src );
#ifdef _DEBUG
			core->CachedFileStreamPtrCount--;
#endif
			_fclose( dest );
#ifdef _DEBUG
			core->CachedFileStreamPtrCount--;
#endif
		} else {  // Don't cache files already on hdd
			strncpy(originalfile, stream, _MAX_PATH);
		}
		str = _fopen( originalfile, "rb" );
	}
#ifdef _DEBUG
	core->CachedFileStreamPtrCount++;
#endif
	startpos = 0;
	_fseek( str, 0, SEEK_END ); 
	size = _ftell( str );
	_fseek( str, 0, SEEK_SET );
	Pos = 0;
	this->autoFree = autoFree;
}

CachedFileStream::CachedFileStream(CachedFileStream* cfs, int startpos,
	int size, bool autoFree)
{
	this->size = size;
	this->startpos = startpos;
	this->autoFree = autoFree;
	char cpath[_MAX_PATH];
	PathJoin( cpath, core->CachePath, cfs->filename, NULL );
	str = _fopen( cpath, "rb" );
	if (str == NULL) {
		str = _fopen( cfs->originalfile, "rb" );
		if (str == NULL) {
			printf( "Can't open stream (maybe leaking?)\n" );
			return;
		}
		strncpy( originalfile, cfs->originalfile, sizeof(originalfile) );
		strncpy( filename, cfs->filename, sizeof(filename) );
	} else {
		strncpy( originalfile, cpath, sizeof(originalfile) );
		strncpy( filename, cfs->filename, sizeof(filename) );
	}
#ifdef _DEBUG
	core->CachedFileStreamPtrCount++;
#endif
	_fseek( str, startpos, SEEK_SET );
	
	
	Pos = 0;
}

CachedFileStream::~CachedFileStream(void)
{
	if (autoFree && str) {
#ifdef _DEBUG
		core->CachedFileStreamPtrCount--;
#endif
		_fclose( str );
	}
	str = NULL;
	//autoFree = false; //File stream destructor hack
}

int CachedFileStream::Read(void* dest, unsigned int length)
{
	//we don't allow partial reads anyway, so it isn't a problem that
	//i don't adjust length here (partial reads are evil)
	if (Pos+length>size ) {
		return GEM_ERROR;
	}

	unsigned int c = (unsigned int) _fread( dest, 1, length, str );
	if (c != length) {
		return GEM_ERROR;
	}
	if (Encrypted) {
		ReadDecrypted( dest, c );
	}
	Pos += c;
	return c;
}

int CachedFileStream::Write(const void* src, unsigned int length)
{
	// do encryption here if needed

	unsigned int c = (unsigned int) _fwrite( src, 1, length, str );
	if (c != length) {
		return GEM_ERROR;
	}
	Pos += c;
	//this is needed only if you want to Seek in a written file
	if (Pos>size) {
		size = Pos;
	}
	return c;
}

int CachedFileStream::Seek(int newpos, int type)
{
	switch (type) {
		case GEM_CURRENT_POS:
			_fseek( str, newpos, SEEK_CUR );
			Pos += newpos;
			break;

		case GEM_STREAM_START:
			_fseek( str, startpos + newpos, SEEK_SET );
			Pos = newpos;
			break;

		default:
			return GEM_ERROR;
	}
	//we went past the buffer
	if (Pos>size) {
		printf("[Streams]: Invalid seek position: %ld (limit: %ld)\n",Pos, size);
		return GEM_ERROR;
	}
	return GEM_OK;
}

/** No descriptions */
int CachedFileStream::ReadLine(void* buf, unsigned int maxlen)
{
	if (!maxlen) {
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
		if (Encrypted)
			ch ^= GEM_ENCRYPTION_KEY[Pos & 63];
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
