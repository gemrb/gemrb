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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/CachedFileStream.cpp,v 1.26 2004/04/18 14:25:58 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "CachedFileStream.h"
#include "Interface.h"

extern Interface* core;

CachedFileStream::CachedFileStream(char* stream, bool autoFree)
{
	char fname[_MAX_PATH];
	ExtractFileFromPath( fname, stream );

	char path[_MAX_PATH];
	strcpy( path, core->CachePath );
	strcat( path, fname );
	str = _fopen( path, "rb" );
	if (str == NULL) {
		if (core->GameOnCD) {
			_FILE* src = _fopen( stream, "rb" );
#ifdef _DEBUG
			core->CachedFileStreamPtrCount++;
#endif
			_FILE* dest = _fopen( path, "wb" );
#ifdef _DEBUG
			core->CachedFileStreamPtrCount++;
#endif
			void* buff = malloc( 1024 * 1000 );
			do {
				size_t len = _fread( buff, 1, 1024 * 1000, src );
				_fwrite( buff, 1, len, dest );
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
			str = _fopen( path, "rb" );
		} else {
			str = _fopen( stream, "rb" );
		}
	}
#ifdef _DEBUG
	core->CachedFileStreamPtrCount++;
#endif
	startpos = 0;
	_fseek( str, 0, SEEK_END ); 
	size = _ftell( str );
	_fseek( str, 0, SEEK_SET );
	strcpy( filename, fname );
	strcpy( originalfile, stream );
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
	strcpy( cpath, core->CachePath );
	strcat( cpath, cfs->filename );
	str = _fopen( cpath, "rb" );
	if (str == NULL) {
		str = _fopen( cfs->originalfile, "rb" );
		if (str == NULL) {
			printf( "\nDANGER WILL ROBINSON!!! str == NULL\nI'll wait a second hoping to open the file..." );
		}
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
	autoFree = false; //File stream destructor hack
}

int CachedFileStream::Read(void* dest, unsigned int length)
{
	//we don't allow partial reads anyway, so it isn't a problem that
	//i don't adjust length here (partial reads are evil)
	if(Pos+length>size ) {
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
	if(Pos>=size) {
abort();
		return GEM_ERROR;
	}
	return GEM_OK;
}

unsigned long CachedFileStream::Size()
{
	return size;
}
/** No descriptions */
int CachedFileStream::ReadLine(void* buf, unsigned int maxlen)
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
		if (_feof( str ))
			break;
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
	}
	p[i] = 0;
	return i;
}
