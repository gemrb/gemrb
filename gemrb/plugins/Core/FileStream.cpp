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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/FileStream.cpp,v 1.34 2005/04/30 22:48:19 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "FileStream.h"
#include "Interface.h"

extern Interface* core;

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
	}
	autoFree = aF;
	str = _fopen( fname, "rb" );
	if (str == NULL) {
		return false;
	}
#ifdef _DEBUG
	core->FileStreamPtrCount++;
#endif
	startpos = 0;
	opened = true;
	created = false;
	_fseek( str, 0, SEEK_END );
	size = _ftell( str );
	_fseek( str, 0, SEEK_SET );
	ExtractFileFromPath( filename, fname );
	Pos = 0;
	return true;
}

bool FileStream::Open(_FILE* stream, int spos, int maxsize, bool aF)
{
	if (str && autoFree) {
#ifdef _DEBUG
		core->FileStreamPtrCount--;
#endif
		_fclose( str );
	}
	autoFree = aF;
	str = stream;
	if (str == NULL) {
		return false;
	}
#ifdef _DEBUG
	core->FileStreamPtrCount++;
#endif
	startpos = spos;
	opened = true;
	created = false;
	size = maxsize;
	strcpy( filename, "" );
	_fseek( str, spos, SEEK_SET );
	Pos = 0;
	return true;
}

//Create is ALWAYS autofree
bool FileStream::Create(const char* fname)
{
	char path[_MAX_PATH];

	if (str && autoFree) {
#ifdef _DEBUG
		core->FileStreamPtrCount--;
#endif
		_fclose( str );
	}
	autoFree = true;
	ExtractFileFromPath( filename, fname );
	strcpy( path, core->CachePath );
	strcat( path, filename );
	str = _fopen( path, "wb" );
	if (str == NULL) {
		return false;
	}
	opened = false;
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

int FileStream::Write(void* src, unsigned int length)
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
	//this is needed only if you want to Seek in a written file
	if (Pos>size) {
		size = Pos;
	}
	return c;
}

int FileStream::Seek(int newpos, int type)
{
	//change this to !opened & !created if you want to Seek in written file
	if (!opened) {
		return GEM_ERROR;
	}
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
	if (Pos>size) {
		printf("[Streams]: Invalid seek position: %ld (limit: %ld)\n",Pos, size);
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
		if (_feof( str ))
			break;
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
	}
	p[i] = 0;
	return i;
}

unsigned long FileStream::GetStartPos() const
{
	return startpos;
}
