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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/MemoryStream.cpp,v 1.7 2003/11/25 13:48:03 balrog994 Exp $
 *
 */

#include "../../includes/win32def.h"
#include "MemoryStream.h"

MemoryStream::MemoryStream(void * buffer, int length)
{
	ptr = buffer;
	this->length = length;
	Pos = 0;
	strcpy(filename, "");
}

MemoryStream::~MemoryStream(void)
{
}

int MemoryStream::Read(void * dest, int length)
{
	if(length+Pos > this->length)
		return GEM_ERROR;
	Byte * p = (Byte*)ptr + Pos;
	memcpy(dest, p, length);
	if(Encrypted)
		ReadDecrypted(dest,length);
	Pos+=length;
	return GEM_OK;
}

int MemoryStream::Seek(int arg_pos, int startpos)
{
	switch(startpos) {
		case GEM_CURRENT_POS:
			{
			if((Pos + arg_pos) < 0)
				return GEM_ERROR;
			if((Pos + arg_pos) >= length)
				return GEM_ERROR;
			Pos+=arg_pos;
			}
		break;

		case GEM_STREAM_START:
			{
			if(arg_pos >= length)
				return GEM_ERROR;
			Pos = length;
			}
		break;

		default:
			return GEM_ERROR;
	}
	return GEM_OK;
}

unsigned long MemoryStream::Size()
{
	return length;
}
/** No descriptions */
int MemoryStream::ReadLine(void * buf, int maxlen)
{
	if(Pos>=length)
		return -1;
	unsigned char *p = (unsigned char*)buf;
	int i = 0;
	while(i < (maxlen-1)) {
		Byte ch = *((Byte*)ptr + Pos);
		if(Pos==length)
			break;
		if(Encrypted)
			p[i]^=GEM_ENCRYPTION_KEY[Pos&63];
		Pos++;
		if(ch == '\n')
			break;
		if(ch == '\t')
			ch = ' ';
		if(ch != '\r')
			p[i++] = ch;
	}
	p[i] = 0;
	return i-1;
}
