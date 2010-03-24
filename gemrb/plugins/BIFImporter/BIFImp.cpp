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
 * $Id$
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
		compressed->ReadDword( &fnlen );
		char* fname = ( char* ) malloc( fnlen );
		compressed->Read( fname, fnlen );
		strlwr(fname);
		compressed->ReadDword( &declen );
		compressed->ReadDword( &complen );
		PathJoin( path, core->CachePath, fname, NULL );
		printf( "Decompressing %s\n",fname );
		free( fname );
		if (!core->IsAvailable( IE_COMPRESSION_CLASS_ID ))
			return GEM_ERROR;
		FILE *in_cache = fopen( path, "wb" );
		if (!in_cache) {
			printMessage("BIFImporter", " ", RED);
			printf( "Cannot write %s.\n", path );	
			return GEM_ERROR;
		}
		Compressor* comp = ( Compressor* )
			core->GetInterface( IE_COMPRESSION_CLASS_ID );
		if (comp->Decompress( in_cache, compressed ) != GEM_OK) {
			return GEM_ERROR;
		}
		core->FreeInterface( comp );
		fclose( in_cache );
		Current = compressed->Remains();
		//starting at 40% going up to 90%
		core->LoadProgress( 40+(All-Current)*50/All );
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
	//baaah, we dump output right in the stream, we get the compressed length
	//only after the compressed data was written
	complen = 0xcdcdcdcd; //placeholder
	unsigned long Pos = str->GetPos(); //storing the stream position
	str->WriteDword( &complen);

	Compressor* comp = ( Compressor* )
		core->GetInterface( IE_COMPRESSION_CLASS_ID );  
	comp->Compress( str, uncompressed );
	core->FreeInterface( comp );

	//writing compressed length (calculated)
	unsigned long Pos2 = str->GetPos();
	complen = Pos2-Pos-sizeof(ieDword); //calculating the compressed stream size
	str->Seek(Pos, GEM_STREAM_START); //going back to the placeholder
	str->WriteDword( &complen);       //updating size
	str->Seek(Pos2, GEM_STREAM_START);//resuming work
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
	if (fread( &Signature, 1, 8, in_cache ) != 8) {
		fclose ( in_cache );
		return GEM_ERROR;
	}
	fclose( in_cache );
	//normal bif, not in cache
	if (strncmp( Signature, "BIFFV1  ", 8 ) == 0) {
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
		compressed->ReadDword( &fnlen );
		char* fname = ( char* ) malloc( fnlen );
		compressed->Read( fname, fnlen );
		strlwr(fname);
		compressed->ReadDword( &declen );
		compressed->ReadDword( &complen );
		PathJoin( path, core->CachePath, fname, NULL );
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
		if (!core->IsAvailable( IE_COMPRESSION_CLASS_ID )) {
			printMessage("BIFImporter", "No Compression Manager Available.", RED);
			return GEM_ERROR;
		}
		in_cache = fopen( path, "wb" );
		if (!in_cache) {
			printMessage("BIFImporter", " ", RED);
			printf( "Cannot write %s.\n", path );
			return GEM_ERROR;
		}
		Compressor* comp = ( Compressor* )
			core->GetInterface( IE_COMPRESSION_CLASS_ID );
		if (comp->Decompress( in_cache, compressed ) != GEM_OK) {
			return GEM_ERROR;
		}
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
		PathJoin( path, core->CachePath, compressed->filename, NULL );
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
		ieDword unCompBifSize;
		compressed->ReadDword( &unCompBifSize );
		printf( "\nDecompressing file: [..........]" );
		fflush(stdout);
		in_cache = fopen( path, "wb" );
		if (!in_cache) {
			printMessage("BIFImporter", " ", RED);
			printf( "Cannot write %s.\n", path );	
			return GEM_ERROR;
		}
		ieDword finalsize = 0;
		int laststep = 0;
		while (finalsize < unCompBifSize) {
			compressed->Seek( 8, GEM_CURRENT_POS );
			if (comp->Decompress( in_cache, compressed ) != GEM_OK) {
				return GEM_ERROR;
			}
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

DataStream* BIFImp::GetStream(unsigned long Resource, unsigned long Type, bool silent)
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
		ieDword srcResLoc = Resource & 0x3FFF;
		for (ieDword i = 0; i < fentcount; i++) {
			if (( fentries[i].resLocator & 0x3FFF ) == srcResLoc) {
				s = new CachedFileStream( stream, fentries[i].dataOffset,
							fentries[i].fileSize );
				break;
			}
		}
	}
	if (!silent) {
		if (s) {
			printStatus( "FOUND", LIGHT_GREEN );
		} else {
			printStatus( "ERROR", LIGHT_RED );
		}
	}
	return s;
}

void BIFImp::ReadBIF(void)
{
	ieDword foffset;
	stream->ReadDword( &fentcount );
	stream->ReadDword( &tentcount );
	stream->ReadDword( &foffset );
	stream->Seek( foffset, GEM_STREAM_START );
	fentries = new FileEntry[fentcount];
	tentries = new TileEntry[tentcount];
	if (!fentries || !tentries) {
		if (fentries) {
			delete fentries;
			fentries = NULL;
		}
		if (tentries) {
			delete tentries;
			tentries = NULL;
		}
		return;
	}
	unsigned int i;

	for (i=0;i<fentcount;i++) {
		stream->ReadDword( &fentries[i].resLocator);
		stream->ReadDword( &fentries[i].dataOffset);
		stream->ReadDword( &fentries[i].fileSize);
		stream->ReadWord( &fentries[i].type);
		stream->ReadWord( &fentries[i].u1);
	}
	for (i=0;i<tentcount;i++) {
		stream->ReadDword( &tentries[i].resLocator);
		stream->ReadDword( &tentries[i].dataOffset);
		stream->ReadDword( &tentries[i].tilesCount);
		stream->ReadDword( &tentries[i].tileSize);
		stream->ReadWord( &tentries[i].type);
		stream->ReadWord( &tentries[i].u1);
	}
}

#include "../../includes/plugindef.h"

GEMRB_PLUGIN(0xC7F133C, "BIF File Importer")
PLUGIN_CLASS(IE_BIF_CLASS_ID, BIFImp)
END_PLUGIN()
