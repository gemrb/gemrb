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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/DataStream.cpp,v 1.12 2004/09/12 15:52:21 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "DataStream.h"

static bool EndianSwitch = false;

DataStream::DataStream(void)
{
	Pos = 0;
	Encrypted = false;
}

DataStream::~DataStream(void)
{
}

void DataStream::SetEndianSwitch(int tmp)
{
	EndianSwitch = !! tmp;
}

bool DataStream::IsEndianSwitch()
{
	return EndianSwitch;
}

/** Returns true if the stream is encrypted */
bool DataStream::CheckEncrypted()
{
	ieWord two;
	Seek( 0, GEM_STREAM_START );
	Read( &two, 2 );
	if (two == 0xFFFF) {
		Pos = 0;
		Encrypted = true;
		return true;
	}
	Seek( 0, GEM_STREAM_START );
	Encrypted = false;
	return false;
}
/** No descriptions */
void DataStream::ReadDecrypted(void* buf, unsigned int size)
{
	for (unsigned int i = 0; i < size; i++)
		( ( unsigned char * ) buf )[i] ^= GEM_ENCRYPTION_KEY[( Pos + i ) & 63];
}
unsigned long DataStream::Remains()
{
	return Size()-Pos;
}

int DataStream::ReadWord(ieWord *dest)
{
	int len = Read(dest, 2);
	if (EndianSwitch) {
		unsigned char tmp;
		tmp=((unsigned char *) dest)[0];
		((unsigned char *) dest)[0]=((unsigned char *) dest)[1];
		((unsigned char *) dest)[1]=tmp;
	}
	return len;
}

int DataStream::ReadDword(ieDword *dest)
{
	int len = Read(dest, 4);
	if (EndianSwitch) {
		unsigned char tmp;
		tmp=((unsigned char *) dest)[0];
		((unsigned char *) dest)[0]=((unsigned char *) dest)[3];
		((unsigned char *) dest)[3]=tmp;
		tmp=((unsigned char *) dest)[1];
		((unsigned char *) dest)[1]=((unsigned char *) dest)[2];
		((unsigned char *) dest)[2]=tmp;
	}
	return len;
}
