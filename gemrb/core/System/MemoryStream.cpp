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

#include "System/MemoryStream.h"

#include "win32def.h"

#include <cstring>

MemoryStream::MemoryStream(void* buffer, int length, bool autoFree)
{
	ptr = buffer;
	size = length;
	Pos = 0;
	strcpy( filename, "" );
	this->autoFree = autoFree;
}

MemoryStream::~MemoryStream(void)
{
	if (autoFree) {
		free( ptr );
	}
}

int MemoryStream::Read(void* dest, unsigned int length)
{
	if (length + Pos > size) {
		return GEM_ERROR;
	}
	ieByte* p = ( ieByte* ) ptr + Pos;
	memcpy( dest, p, length );
	if (Encrypted) {
		ReadDecrypted( dest, length );
	}
	Pos += length;
	return GEM_OK;
}

int MemoryStream::Seek(int newpos, int type)
{
	switch (type) {
		case GEM_CURRENT_POS:
			if (( Pos + newpos ) > size) {
				printf("[Streams]: Invalid seek\n");
				return GEM_ERROR;
			}
			Pos += newpos;
			break;

		case GEM_STREAM_START:
			if ((unsigned long) newpos > size) {
				printf("[Streams]: Invalid seek\n");
				return GEM_ERROR;
			}
			Pos = newpos;
			break;

		default:
			return GEM_ERROR;
	}
	return GEM_OK;
}

/** No descriptions */
int MemoryStream::ReadLine(void* buf, unsigned int maxlen)
{
	if(!maxlen) {
		return 0;
	}
	unsigned char * p = ( unsigned char * ) buf;
	if (Pos >= size) {
		p[0]=0;
		return -1;
	}
	unsigned int i = 0;
	while (i < ( maxlen - 1 )) {
		ieByte ch = *( ( ieByte* ) ptr + Pos );
		if (Pos == size)
			break;
		if (Encrypted)
			p[i] ^= GEM_ENCRYPTION_KEY[Pos & 63];
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
