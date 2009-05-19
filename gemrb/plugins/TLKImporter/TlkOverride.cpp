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
 * $Id: TLKImp.cpp 4692 2007-06-10 09:08:38Z avenger_teambg $
 *
 * TlkOverride.cpp: implementation of the saved game specific
 * (dynamic) part of the talk table (tlk)
 */

#include <cstdio>
#include "TlkOverride.h"

#define SEGMENT_SIZE  512

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
	toh_str = GetAuxHdr();
	tot_str = GetAuxTlk();
	if (toh_str == NULL) {
		return false;
	}
	if (tot_str == NULL) {
		return false;
	}

	char Signature[8];
	toh_str->Read( Signature, 4 );
	if (strncmp( Signature, "TLK ", 4 ) != 0) {
		printf( "[TLKImporter]: Not a valid TOH File.\n" );
		return false;
	}
	toh_str->Seek( 8, GEM_CURRENT_POS );
	toh_str->ReadDword( &AuxCount );

	tot_str->Read( Signature, 8 );
	if (strncmp( Signature, "\xff\xff\xff\xff\xff\xff\xff\xff",8) !=0) {
		printf( "[TLKImporter]: Not a valid TOT File.\n" );
		return false;
	}

	return true;
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

//this function handles the .tot and .toh files
//finds and returns a string referenced by an strref
char* CTlkOverride::LocateString(ieStrRef strref)
{
	ieDword strref2;
	ieDword offset;

	if (!toh_str) return NULL;
	toh_str->Seek(20,GEM_STREAM_START);
	for(ieDword i=0;i<AuxCount;i++) {
		toh_str->ReadDword(&strref2);
		toh_str->Seek(20,GEM_CURRENT_POS);
		toh_str->ReadDword(&offset);
		if (strref2==strref) {
			return LocateString2(offset);
		}    
	}
	return NULL;
}

//this function handles all of the .tlk override mechanism with caching
//strings it once found
//it is possible to turn off caching
char* CTlkOverride::ResolveAuxString(ieStrRef strref, int &Length)
{
	if (!this) {
		Length = 0;
		char *string = ( char* ) malloc( Length+1 );
		string[Length] = 0;
		return string;
	}

#ifdef CACHE_TLK_OVERRIDE
	StringMapType::iterator tmp = stringmap.find(strref);
	if (tmp!=stringmap.end()) {
		return CS((*tmp).second);
	}
#endif

	char *string = LocateString(strref);
	if (!string) {
		Length = 0;
		string = ( char* ) malloc( Length+1 );
		string[Length] = 0;
	} else {
		Length = strlen(string);
	}
#ifdef CACHE_TLK_OVERRIDE
	stringmap[strref]=CS(string);
#endif
	return string;
}

DataStream* CTlkOverride::GetAuxHdr()
{
	char nPath[_MAX_PATH];
	sprintf( nPath, "%s%sdefault.toh", core->CachePath, SPathDelimiter );
#ifndef WIN32
	ResolveFilePath( nPath );
#endif
	FileStream* fs = new FileStream();
	if (fs->Open( nPath, true )) {
		return fs;
	}
	delete fs;
	return NULL;
}

DataStream* CTlkOverride::GetAuxTlk()
{
	char nPath[_MAX_PATH];
	sprintf( nPath, "%s%sdefault.tot", core->CachePath, SPathDelimiter );
#ifndef WIN32
	ResolveFilePath( nPath );
#endif
	FileStream* fs = new FileStream();
	if (fs->Open( nPath, true )) {
		return fs;
	}
	delete fs;
	return NULL;
}

