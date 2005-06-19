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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/BIFImporter/BIFImp.cpp,v 1.21 2005/06/19 22:59:33 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "../Core/Compressor.h"
#include "../Core/FileStream.h"
#include "../Core/CachedFileStream.h"
#include "BIFImp.h"
#include "../Core/Interface.h"

BIFImp::BIFImp(void)
{
	stream = NULL;
	fentries = NULL;
	tentries = NULL;
}

BIFImp::~BIFImp(void)
{
	if (stream) {
		delete( stream );
	}
	if (fentries) {
		delete[] fentries;
	}
	if (tentries) {
		delete[] tentries;
	}
}

int BIFImp::DecompressSaveGame(DataStream *compressed)
{
	char Signature[8];
	compressed->Read( Signature, 8 );
	if (strncmp( Signature, "SAV V1.0", 8 ) ) {
		return GEM_ERROR;
	}
	int All = compressed->Remains();
	int Current;
	if (!All) return GEM_ERROR;
	do {
		ieDword fnlen, complen, declen;
		compressed->Read( &fnlen, 4 );
		char* fname = ( char* ) malloc( fnlen );
		compressed->Read( fname, fnlen );
		strlwr(fname);
		compressed->Read( &declen, 4 );
		compressed->Read( &complen, 4 );
		strcpy( path, core->CachePath );
		strcat( path, fname );
		printf( "Decompressing %s\n",fname );
		free( fname );
		if (!core->IsAvailable( IE_COMPRESSION_CLASS_ID ))
			return GEM_ERROR;
		FILE *in_cache = fopen( path, "wb" );
		Compressor* comp = ( Compressor* )
			core->GetInterface( IE_COMPRESSION_CLASS_ID );
		comp->Decompress( in_cache, compressed );
		core->FreeInterface( comp );
		fclose( in_cache );
		Current = compressed->Remains();
		//starting at 25% going up to 75%
		core->LoadProgress( 25+(All-Current)*50/All );
	}
	while(Current);
	return GEM_OK;
}

//this one can create .sav files only
int BIFImp::CreateArchive(DataStream *compressed)
{
	if (stream) {
		delete( stream );
		stream = NULL;
	}
	if (!compressed) {
		return GEM_ERROR;
	}
	char Signature[8];
	
	memcpy(Signature,"SAV V1.0",8);
	compressed->Write(Signature, 8);

	return GEM_OK;
}

int BIFImp::AddToSaveGame(DataStream *str, DataStream *uncompressed)
{
	ieDword fnlen, declen, complen;

	fnlen = strlen(uncompressed->filename)+1;
	declen = uncompressed->Size();
	str->WriteDword( &fnlen);
	str->Write( uncompressed->filename, fnlen);
	str->WriteDword( &declen);
	//baaah
	complen = 0xcdcdcdcd;
	unsigned long Pos = str->GetPos();
	str->WriteDword( &complen);

	Compressor* comp = ( Compressor* )
		core->GetInterface( IE_COMPRESSION_CLASS_ID );  
	comp->Compress( str, uncompressed );
	core->FreeInterface( comp );

	//writing compressed length (calculated)
	unsigned long Pos2 = str->GetPos();
	str->Seek(Pos, GEM_STREAM_START);
	complen = Pos2-Pos;
	str->WriteDword( &complen);
	str->Seek(Pos2, GEM_STREAM_START);
	return GEM_OK;
}

int BIFImp::OpenArchive(const char* filename)
{
	if (stream) {
		delete( stream );
		stream = NULL;
	}
	FILE* in_cache = fopen( filename, "rb" );
	if( !in_cache) {
		return GEM_ERROR;
	}
	char Signature[8];
	fread( &Signature, 1, 8, in_cache );
	//normal bif, not in cache
	if (strncmp( Signature, "BIFFV1  ", 8 ) == 0) {
		fclose( in_cache );
		stream = new CachedFileStream( filename );
		stream->Read( Signature, 8 );
		strcpy( path, filename );
		ReadBIF();
		return GEM_OK;
	}
	//not found as normal bif
	//checking compression type
	FileStream* compressed = new FileStream();
	compressed->Open( filename, true );
	compressed->Read( Signature, 8 );
	if (strncmp( Signature, "BIF V1.0", 8 ) == 0) {
		ieDword fnlen, complen, declen;
		compressed->Read( &fnlen, 4 );
		char* fname = ( char* ) malloc( fnlen );
		compressed->Read( fname, fnlen );
		compressed->Read( &declen, 4 );
		compressed->Read( &complen, 4 );
		strcpy( path, core->CachePath );
		strcat( path, fname );
		free( fname );
		in_cache = fopen( path, "rb" );
		if (in_cache) {
			//printf("Found in Cache\n");
			fclose( in_cache );
			delete( compressed );
			stream = new CachedFileStream( path );
			stream->Read( Signature, 8 );
			if (strncmp( Signature, "BIFFV1  ", 8 ) == 0)
				ReadBIF();
			else
				return GEM_ERROR;
			return GEM_OK;
		}
		printf( "Decompressing\n" );
		if (!core->IsAvailable( IE_COMPRESSION_CLASS_ID ))
			return GEM_ERROR;
		in_cache = fopen( path, "wb" );
		Compressor* comp = ( Compressor* )
			core->GetInterface( IE_COMPRESSION_CLASS_ID );
		comp->Decompress( in_cache, compressed );
		core->FreeInterface( comp );
		fclose( in_cache );
		delete( compressed );
		stream = new CachedFileStream( path );
		stream->Read( Signature, 8 );
		if (strncmp( Signature, "BIFFV1  ", 8 ) == 0)
			ReadBIF();
		else
			return GEM_ERROR;
		return GEM_OK;
	}

	if (strncmp( Signature, "BIFCV1.0", 8 ) == 0) {
		//printf("'BIFCV1.0' Compressed File Found\n");
		strcpy( path, core->CachePath );
		strcat( path, compressed->filename );
		in_cache = fopen( path, "rb" );
		if (in_cache) {
			//printf("Found in Cache\n");
			fclose( in_cache );
			delete( compressed );
			stream = new CachedFileStream( path );
			stream->Read( Signature, 8 );
			if (strncmp( Signature, "BIFFV1  ", 8 ) == 0) {
				ReadBIF();				
			} else
				return GEM_ERROR;
			return GEM_OK;
		}
		printf( "Decompressing\n" );
		if (!core->IsAvailable( IE_COMPRESSION_CLASS_ID ))
			return GEM_ERROR;
		Compressor* comp = ( Compressor* )
			core->GetInterface( IE_COMPRESSION_CLASS_ID );
		int unCompBifSize;
		compressed->Read( &unCompBifSize, 4 );
		printf( "\nDecompressing file: [..........]" );
		fflush(stdout);
		in_cache = fopen( path, "wb" );
		int finalsize = 0;
		int laststep = 0;
		while (finalsize < unCompBifSize) {
			compressed->Seek( 8, GEM_CURRENT_POS );
			comp->Decompress( in_cache, compressed );
			finalsize = ftell( in_cache );
			if (( int ) ( finalsize * ( 10.0 / unCompBifSize ) ) != laststep) {
				laststep++;
				printf( "\b\b\b\b\b\b\b\b\b\b\b" );
				int l;

				for (l = 0; l < laststep; l++)
					printf( "|" );
				for (; l < 10; l++)//l starts from laststep
					printf( "." );
				printf( "]" );
				fflush(stdout);
			}
		}
		printf( "\n" );
		core->FreeInterface( comp );
		fclose( in_cache );
		delete( compressed );
		stream = new CachedFileStream( path );
		stream->Read( Signature, 8 );
		if (strncmp( Signature, "BIFFV1  ", 8 ) == 0)
			ReadBIF();
		else
			return GEM_ERROR;
		return GEM_OK;
	}
	delete (compressed);
	return GEM_ERROR;
}

DataStream* BIFImp::GetStream(unsigned long Resource, unsigned long Type)
{
	DataStream* s = NULL;
	if (Type == IE_TIS_CLASS_ID) {
		unsigned int srcResLoc = Resource & 0xFC000;
		for (unsigned int i = 0; i < tentcount; i++) {
			if (( tentries[i].resLocator & 0xFC000 ) == srcResLoc) {
				s = new CachedFileStream( stream, tentries[i].dataOffset,
							tentries[i].tileSize * tentries[i].tilesCount );
				break;
			}
		}
	} else {
		unsigned int srcResLoc = Resource & 0x3FFF;
		for (unsigned int i = 0; i < fentcount; i++) {
			if (( fentries[i].resLocator & 0x3FFF ) == srcResLoc) {
				s = new CachedFileStream( stream, fentries[i].dataOffset,
							fentries[i].fileSize );
				break;
			}
		}
	}
	if (s) {
		printStatus( "FOUND", LIGHT_GREEN );
	} else {
		printStatus( "ERROR", LIGHT_RED );
	}
	return s;
}

void BIFImp::ReadBIF(void)
{
	ieDword foffset;
	stream->Read( &fentcount, 4 );
	stream->Read( &tentcount, 4 );
	stream->Read( &foffset, 4 );
	stream->Seek( foffset, GEM_STREAM_START );
	fentries = new FileEntry[fentcount];
	stream->Read( fentries, fentcount * sizeof( FileEntry ) );
	tentries = new TileEntry[tentcount];
	stream->Read( tentries, tentcount * sizeof( TileEntry ) );
}
