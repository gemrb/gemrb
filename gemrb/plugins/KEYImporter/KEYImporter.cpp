/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "KEYImporter.h"

#include "win32def.h"
#include "globals.h"

#include "IndexedArchive.h"
#include "Interface.h"
#include "ResourceDesc.h"
#include "System/FileStream.h"

using namespace GemRB;

KEYImporter::KEYImporter(void)
{
	description = NULL;
}

KEYImporter::~KEYImporter(void)
{
	free(description);
	for (unsigned int i = 0; i < biffiles.size(); i++) {
		free( biffiles[i].name );
	}
}

static char* AddCBF(char *file)
{
	// This is safe in single-threaded, since the
	// return value is passed straight to PathJoin.
	static char cbf[_MAX_PATH];
	strcpy(cbf,file);
	char *dot = strrchr(cbf, '.');
	if (dot)
		strcpy(dot, ".cbf");
	else
		strcat(cbf, ".cbf");
	return cbf;
}

static bool PathExists(BIFEntry *entry, const char *path)
{
	PathJoin(entry->path, path, entry->name, NULL);
	if (file_exists(entry->path)) {
		return true;
	}
	PathJoin(entry->path, path, AddCBF(entry->name), NULL);
	if (file_exists(entry->path)) {
		return true;
	}

	return false;
}

static bool PathExists(BIFEntry *entry, const std::vector<std::string> &pathlist)
{
	size_t i;
	
	for(i=0;i<pathlist.size();i++) {
		if (PathExists(entry, pathlist[i].c_str() )) {
			return true;
		}
	}

	return false;
}

static void FindBIF(BIFEntry *entry)
{
	entry->cd = 0;
	entry->found = PathExists(entry, core->GamePath);
	if (entry->found) {
		return;
	}

	for (int i = 0; i < MAX_CD; i++) {
		if (PathExists(entry, core->CD[i]) ) {
			entry->found = true;
			entry->cd = i;
			return;
		}
	}

	Log(ERROR, "KEYImporter", "Cannot find %s...", entry->name);
}

bool KEYImporter::Open(const char *resfile, const char *desc)
{
	free(description);
	description = strdup(desc);
	if (!core->IsAvailable( IE_BIF_CLASS_ID )) {
		Log(ERROR, "KEYImporter", "An Archive Plug-in is not Available");
		return false;
	}
	unsigned int i;
	// NOTE: Interface::Init has already resolved resfile.
	Log(MESSAGE, "KEYImporter", "Opening %s...", resfile);
	FileStream* f = FileStream::OpenFile(resfile);
	if (!f) {
		// Check for backslashes (false escape characters)
		// this check probably belongs elsewhere (e.g. ResolveFilePath)
		if (strstr( resfile, "\\ " )) {
			Log(MESSAGE, "KEYImporter", "Escaped space(s) detected in path!. Do not escape spaces in your GamePath!");
		}
		Log(ERROR, "KEYImporter", "Cannot open Chitin.key");
		Log(ERROR, "KeyImporter", "This means you set the GamePath config variable incorrectly.");
		Log(ERROR, "KeyImporter", "It must point to the directory that holds a readable Chitin.key");
		return false;
	}
	Log(MESSAGE, "KEYImporter", "Checking file type...");
	char Signature[8];
	f->Read( Signature, 8 );
	if (strncmp( Signature, "KEY V1  ", 8 ) != 0) {
		Log(ERROR, "KEYImporter", "File has an Invalid Signature.");
		delete( f );
		return false;
	}
	Log(MESSAGE, "KEYImporter", "Reading Resources...");
	ieDword BifCount, ResCount, BifOffset, ResOffset;
	f->ReadDword( &BifCount );
	f->ReadDword( &ResCount );
	f->ReadDword( &BifOffset );
	f->ReadDword( &ResOffset );
	Log(MESSAGE, "KEYImporter", "BIF Files Count: %d (Starting at %d Bytes)",
			BifCount, BifOffset );
	Log(MESSAGE, "KEYImporter", "RES Count: %d (Starting at %d Bytes)",
		ResCount, ResOffset);
	f->Seek( BifOffset, GEM_STREAM_START );

	ieDword BifLen, ASCIIZOffset;
	ieWord ASCIIZLen;
	for (i = 0; i < BifCount; i++) {
		BIFEntry be;
		f->Seek( BifOffset + ( 12 * i ), GEM_STREAM_START );
		f->ReadDword( &BifLen );
		f->ReadDword( &ASCIIZOffset );
		f->ReadWord( &ASCIIZLen );
		f->ReadWord( &be.BIFLocator );
		be.name = ( char * ) malloc( ASCIIZLen );
		f->Seek( ASCIIZOffset, GEM_STREAM_START );
		f->Read( be.name, ASCIIZLen );
		for (int p = 0; p < ASCIIZLen; p++) {
			//some MAC versions use : as delimiter
			if (be.name[p] == '\\' || be.name[p] == ':')
				be.name[p] = PathDelimiter;
		}
		FindBIF(&be);
		biffiles.push_back( be );
	}
	f->Seek( ResOffset, GEM_STREAM_START );

	MapKey key;
	ieDword ResLocator;

	// limit to 32k buckets
	// only ~1% of the bg2 entries are of bucket lenght >4
	resources.init(ResCount > 32 * 1024 ? 32 * 1024 : ResCount, ResCount);

	for (i = 0; i < ResCount; i++) {
		f->ReadResRef(key.ref);
		f->ReadWord(&key.type);
		f->ReadDword(&ResLocator);

		// seems to be always the last entry?
		if (key.ref[0] != 0)
			resources.set(key, ResLocator);
	}

	Log(MESSAGE, "KEYImporter", "Resources Loaded...");
	delete( f );
	return true;
}

bool KEYImporter::HasResource(const char* resname, SClass_ID type)
{
	return resources.has(resname, type);
}

bool KEYImporter::HasResource(const char* resname, const ResourceDesc &type)
{
	return HasResource(resname, type.GetKeyType());
}

DataStream* KEYImporter::GetStream(const char *resname, ieWord type)
{
	if (type == 0)
		return NULL;

	const ieDword *ResLocator = resources.get(resname, type);
	if (!ResLocator)
		return 0;

	unsigned int bifnum = ( *ResLocator & 0xFFF00000 ) >> 20;

	if (!biffiles[bifnum].found) {
		print("Cannot find %s... Resource unavailable.",
				biffiles[bifnum].name );
		return NULL;
	}

	PluginHolder<IndexedArchive> ai(IE_BIF_CLASS_ID);
	if (ai->OpenArchive( biffiles[bifnum].path ) == GEM_ERROR) {
		print("Cannot open archive %s", biffiles[bifnum].path);
		return NULL;
	}

	DataStream* ret = ai->GetStream( *ResLocator, type );
	if (ret) {
		strnlwrcpy( ret->filename, resname, 8 );
		strcat( ret->filename, "." );
		strcat( ret->filename, core->TypeExt( type ) );
		return ret;
	}

	return NULL;
}

DataStream* KEYImporter::GetResource(const char* resname, SClass_ID type)
{
	//the word masking is a hack for synonyms, currently used for bcs==bs
	return GetStream(resname, type&0xFFFF);
}

DataStream* KEYImporter::GetResource(const char* resname, const ResourceDesc &type)
{
	return GetStream(resname, type.GetKeyType());
}

#include "plugindef.h"

GEMRB_PLUGIN(0x1DFDEF80, "KEY File Importer")
PLUGIN_CLASS(PLUGIN_RESOURCE_KEY, KEYImporter)
END_PLUGIN()
