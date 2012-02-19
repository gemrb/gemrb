/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2007 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 * TlkOverride.cpp: implementation of the saved game specific
 * (dynamic) part of the talk table (tlk)
 */

#include "TlkOverride.h"

#include <cstdio>
#include <cassert>

CTlkOverride::CTlkOverride()
{
	tot_str=NULL;
	toh_str=NULL;
}

CTlkOverride::~CTlkOverride()
{
	CloseResources();
}

char *CTlkOverride::CS(const char *src)
{
	if(!src) return NULL;
	int len=strlen(src)+1;
	char *ret = (char *) malloc(len);
	memcpy(ret, src, len);
	return ret;
}

bool CTlkOverride::Init()
{
	CloseResources();
	//Creation of the headers should be game specific, some games don't have these
	toh_str = GetAuxHdr(true);
	tot_str = GetAuxTlk(true);
	if (toh_str == NULL) {
		return false;
	}
	if (tot_str == NULL) {
		return false;
	}

	char Signature[8];

	memset(Signature,0,8);
	toh_str->Read( Signature, 4 );
	if (strncmp( Signature, "TLK ", 4 ) != 0) {
		Log(ERROR, "TLKImporter", "Not a valid TOH file.");
		return false;
	}
	toh_str->Seek( 8, GEM_CURRENT_POS );
	toh_str->ReadDword( &AuxCount );

	tot_str->ReadDword( &FreeOffset );
	tot_str->Read(Signature,4);
	if (strncmp( Signature, "\xff\xff\xff\xff",4) !=0) {
		Log(ERROR, "TLKImporter", "Not a valid TOT file.");
		return false;
	}

	return true;
}

void CTlkOverride::UpdateFreeOffset(ieDword NewFree)
{
	if (NewFree!=0xffffffff) {
		tot_str->Seek(NewFree,GEM_STREAM_START);
		tot_str->WriteDword( &FreeOffset);
	}
	FreeOffset=NewFree;
	tot_str->Seek(0, GEM_STREAM_START);
	tot_str->WriteDword( &FreeOffset );
}

void CTlkOverride::CloseResources()
{
	if (toh_str) {
		delete toh_str;
		toh_str=NULL;
	}
	if (tot_str) {
		delete tot_str;
		tot_str=NULL;
	}
#ifdef CACHE_TLK_OVERRIDE
	stringmap.clear();
#endif
}

//gets the length of a stored string which might span more than one segment
ieDword CTlkOverride::GetLength()
{
	ieDword tmp;
	char buffer[SEGMENT_SIZE];

	ieDword length = 0;
	do
	{
		memset(buffer,0,sizeof(buffer));
		tot_str->Read(buffer, SEGMENT_SIZE);
		tot_str->ReadDword(&tmp);
		if (tmp!=0xffffffff) {
			tot_str->Seek(tmp+8,GEM_STREAM_START);
			length+=SEGMENT_SIZE;
		}
	}
	while(tmp!=0xffffffff);
	length += strlen(buffer);
	return length;
}

//returns a string stored at a given offset of the .tot file
char* CTlkOverride::LocateString2(ieDword offset)
{
	if (!tot_str) {
		return NULL;
	}

	if (tot_str->Seek(offset+8, GEM_STREAM_START)!=GEM_OK) {
		return NULL;
	}
	ieDword length = GetLength();
	//assuming char is one byte
	char *ret = (char *) malloc(length+1);
	char *pos = ret;
	ret[length]=0;
	while(length) {
		tot_str->Seek(offset+8, GEM_STREAM_START);
		ieDword tmp = length>SEGMENT_SIZE?SEGMENT_SIZE:length;
		tot_str->Read(pos, tmp);
		tot_str->Seek(SEGMENT_SIZE-tmp, GEM_CURRENT_POS);
		tot_str->ReadDword(&offset);
		length-=tmp;
		pos+=tmp;
	}
	return ret;
}

ieStrRef CTlkOverride::UpdateString(ieStrRef strref, const char *newvalue)
{
	ieDword memoffset = 0;
	bool tookfree = false;
	ieDword offset = LocateString(strref);

	if (offset==0xffffffff) {
		strref=GetNewStrRef();
		offset=LocateString(strref);
		assert(strref!=0xffffffff);
	}

	ieDword length = strlen(newvalue);
	if(length>65535) length=65535;
	length++;

	//set the backpointer of the first string segment
	ieDword backp = 0xffffffff;

	do
	{
		//fill the backpointer
		tot_str->Seek(offset+4, GEM_STREAM_START);
		tot_str->WriteDword(&backp);
		backp = offset;
		ieDword tmp = length>SEGMENT_SIZE?SEGMENT_SIZE:length;
		tot_str->Write(newvalue+memoffset, tmp);
		length-=tmp;
		memoffset+=tmp;
		tot_str->Seek(backp+SEGMENT_SIZE+8, GEM_STREAM_START);
		tot_str->ReadDword(&offset);

		//end of string
		if(!length) {
			if(offset!=0xffffffff) {
				tookfree = true;
			}
			tot_str->Seek(-4,GEM_CURRENT_POS);
			backp = offset+4;
			offset = 0xffffffff;
			tot_str->WriteDword(&offset);
			break;
		}

		if (offset==0xffffffff) {
			//no more space, but we need some
			offset = FreeOffset;
			tookfree = true;
			if (offset == 0xffffffff) {
				//to the end of file
				offset = tot_str->Size();
			}
		}
		tot_str->Seek(-4,GEM_CURRENT_POS);
		tot_str->WriteDword(&offset);
	}
	while(length);

	//adjust the free list
	if (tookfree) {
		UpdateFreeOffset(backp);
	}
	return strref;
}

ieStrRef CTlkOverride::GetNewStrRef()
{
	EntryType entry;

	memset(&entry,0,sizeof(entry));

	if (!AuxCount) {
		entry.strref = STRREF_START;
		entry.offset = 8;
	} else {
		toh_str->Seek(sizeof(entry), GEM_STREAM_END );
		toh_str->ReadDword(&entry.strref);
		toh_str->Read(entry.dummy, 20);
		entry.strref++;
		entry.offset = tot_str->Size();
	}
	toh_str->Seek(0,GEM_STREAM_END);
	toh_str->WriteDword(&entry.strref);
	toh_str->Write(entry.dummy, 20);
	toh_str->WriteDword(&entry.offset);
	AuxCount++;
	toh_str->Seek(8,GEM_STREAM_START);
	toh_str->WriteDword(&AuxCount);
	return entry.strref;
}

ieDword CTlkOverride::LocateString(ieStrRef strref)
{
	ieDword strref2;
	ieDword offset;

	if (!toh_str) return 0xffffffff;
	toh_str->Seek(TOH_HEADER_SIZE,GEM_STREAM_START);
	for(ieDword i=0;i<AuxCount;i++) {
		toh_str->ReadDword(&strref2);
		toh_str->Seek(20,GEM_CURRENT_POS);
		toh_str->ReadDword(&offset);
		if (strref2==strref) {
			return offset;
		}
	}
	return 0xffffffff;
}

//this function handles all of the .tlk override mechanism with caching
//strings it once found
//it is possible to turn off caching
char* CTlkOverride::ResolveAuxString(ieStrRef strref, int &Length)
{
	char *string;

	if (!this) {
		Length = 0;
		string = ( char* ) malloc( 1 );
		string[0] = 0;
		return string;
	}

#ifdef CACHE_TLK_OVERRIDE
	StringMapType::iterator tmp = stringmap.find(strref);
	if (tmp!=stringmap.end()) {
		return CS((*tmp).second);
	}
#endif

	ieDword offset = LocateString(strref);
	if (offset!=0xffffffff) {
		string = LocateString2(offset);
		Length = strlen(string);
	} else {
		Length = 0;
		string = ( char* ) malloc( 1 );
		string[0] = 0;
	}
#ifdef CACHE_TLK_OVERRIDE
	stringmap[strref]=CS(string);
#endif
	return string;
}

DataStream* CTlkOverride::GetAuxHdr(bool create)
{
	char nPath[_MAX_PATH];
	char Signature[TOH_HEADER_SIZE];

	PathJoin( nPath, core->CachePath, "default.toh", NULL );
	FileStream* fs = new FileStream();
retry:
	if (fs->Modify(nPath)) {
		return fs;
	}
	if (create) {
		fs->Create( "default", IE_TOH_CLASS_ID);
		memset(Signature,0,sizeof(Signature));
		memcpy(Signature,"TLK ",4);
		fs->Write(Signature, sizeof(Signature));
		create = false;
		goto retry;
	}
	delete fs;
	return NULL;
}

DataStream* CTlkOverride::GetAuxTlk(bool create)
{
	char nPath[_MAX_PATH];
	PathJoin( nPath, core->CachePath, "default.tot", NULL );
	FileStream* fs = new FileStream();
retry:
	if (fs->Modify(nPath)) {
		return fs;
	}
	if (create) {
		fs->Create( "default", IE_TOT_CLASS_ID);
		create = false;
		goto retry;
	}
	delete fs;
	return NULL;
}

