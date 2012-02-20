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
 */

#include "SAVImporter.h"

#include "win32def.h"

#include "Compressor.h"
#include "FileCache.h"
#include "Interface.h"
#include "PluginMgr.h"

using namespace GemRB;

SAVImporter::SAVImporter()
{
}

SAVImporter::~SAVImporter()
{
}

int SAVImporter::DecompressSaveGame(DataStream *compressed)
{
	char Signature[8];
	compressed->Read( Signature, 8 );
	if (strncmp( Signature, "SAV V1.0", 8 ) ) {
		return GEM_ERROR;
	}
	int All = compressed->Remains();
	int Current;
	int percent, last_percent = 20;
	if (!All) return GEM_ERROR;
	do {
		ieDword fnlen, complen, declen;
		compressed->ReadDword( &fnlen );
		if (!fnlen) {
			Log(ERROR, "SAVImporter", "Corrupt Save Detected");
			return GEM_ERROR;
		}
		char* fname = ( char* ) malloc( fnlen );
		compressed->Read( fname, fnlen );
		strlwr(fname);
		compressed->ReadDword( &declen );
		compressed->ReadDword( &complen );
		print("Decompressing %s", fname);
		DataStream* cached = CacheCompressedStream(compressed, fname, complen, true);
		free( fname );
		if (!cached)
			return GEM_ERROR;
		delete cached;
		Current = compressed->Remains();
		//starting at 20% going up to 70%
		percent = (20 + (All - Current) * 50 / All);
		if (percent - last_percent > 5) {
			core->LoadProgress(percent);
			last_percent = percent;
		}
	}
	while(Current);
	return GEM_OK;
}

//this one can create .sav files only
int SAVImporter::CreateArchive(DataStream *compressed)
{
	if (!compressed) {
		return GEM_ERROR;
	}
	char Signature[8];

	memcpy(Signature,"SAV V1.0",8);
	compressed->Write(Signature, 8);

	return GEM_OK;
}

int SAVImporter::AddToSaveGame(DataStream *str, DataStream *uncompressed)
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

	PluginHolder<Compressor> comp(PLUGIN_COMPRESSION_ZLIB);
	comp->Compress( str, uncompressed );

	//writing compressed length (calculated)
	unsigned long Pos2 = str->GetPos();
	complen = Pos2-Pos-sizeof(ieDword); //calculating the compressed stream size
	str->Seek(Pos, GEM_STREAM_START); //going back to the placeholder
	str->WriteDword( &complen);       //updating size
	str->Seek(Pos2, GEM_STREAM_START);//resuming work
	return GEM_OK;
}

#include "plugindef.h"

GEMRB_PLUGIN(0xCDF132C, "SAV File Importer")
PLUGIN_CLASS(IE_SAV_CLASS_ID, SAVImporter)
END_PLUGIN()
