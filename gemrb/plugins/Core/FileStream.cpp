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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/FileStream.cpp,v 1.17 2003/11/25 13:48:02 balrog994 Exp $
 *
 */

#include "../../includes/win32def.h"
#include "FileStream.h"

#ifdef _DEBUG
#include "Interface.h"
extern Interface * core;
#endif

FileStream::FileStream(void)
{
	opened = false;
	str = NULL;
	autoFree = false;
}

FileStream::~FileStream(void)
{
	if(autoFree && str) {
#ifdef _DEBUG
		core->FileStreamPtrCount--;
#endif
		fclose(str);
	}
}

bool FileStream::Open(const char * filename, bool autoFree)
{
	if(str && this->autoFree) {
#ifdef _DEBUG
		core->FileStreamPtrCount--;
#endif
		fclose(str);
	}
	this->autoFree = autoFree;
	str = fopen(filename, "rb");
	if(str == NULL) {
		return false;
	}
#ifdef _DEBUG
	core->FileStreamPtrCount++;
#endif
	startpos = 0;
	opened = true;
	fseek(str, 0, SEEK_END);
	size = ftell(str)+1;
	fseek(str, 0, SEEK_SET);
	ExtractFileFromPath(this->filename, filename);
	Pos = 0;
	return true;
}

bool FileStream::Open(FILE * stream, int startpos, int size, bool autoFree)
{
	if(str && this->autoFree) {
#ifdef _DEBUG
		core->FileStreamPtrCount--;
#endif
		fclose(str);
	}
	this->autoFree = autoFree;
	str = stream;
	if(str == NULL)
		return false;
#ifdef _DEBUG
	core->FileStreamPtrCount++;
#endif
	this->startpos = startpos;
	opened = true;
	this->size = size;
	strcpy(filename, "");
	fseek(str, startpos, SEEK_SET);
	Pos = 0;
	return true;
}

int FileStream::Read(void * dest, int length)
{
	if(!opened)
		return GEM_ERROR;
	size_t c = fread(dest, 1, length, str);
	//if(feof(str)) { /* slightly modified by brian  oct 11 2003*/
	//	return GEM_EOF;
	//}
	if(c < 0) {
		return GEM_ERROR;
	}
	if(Encrypted)
		ReadDecrypted(dest, c);
	Pos+=c;
	return (int)c;
}

int FileStream::Seek(int pos, int startpos)
{
	if(!opened)
		return GEM_ERROR;
	switch(startpos) 
		{
		case GEM_CURRENT_POS:
			fseek(str, pos, SEEK_CUR);
			Pos+=pos;
		break;

		case GEM_STREAM_START:
			fseek(str, this->startpos + pos, SEEK_SET);
			Pos = pos;
		break;

		default:
			return GEM_ERROR;
		}
	return GEM_OK;
}

unsigned long FileStream::Size()
{
	return size;	
}
/** No descriptions */
int FileStream::ReadLine(void * buf, int maxlen)
{
	if(feof(str))
		return -1;
	unsigned char *p = (unsigned char*)buf;
	int i = 0;
	while(i < (maxlen-1)) {
		int ch = fgetc(str);
		if(feof(str))
			 break;
		if(Pos==size)
			break;
		if(Encrypted) {
			ch^=GEM_ENCRYPTION_KEY[Pos&63];
		}
		Pos++;
		if(ch == '\n')
			break;
		if(ch == '\t')
			ch = ' ';
		if(ch != '\r')
			p[i++] = ch;
	}
	p[i] = 0;
	return i;
}
