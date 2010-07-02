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

#include "ArchiveImporter.h"
#include "Interface.h"
#include "ResourceDesc.h"
#include "System/FileStream.h"

KEYImporter::KEYImporter(void)
{
}

KEYImporter::~KEYImporter(void)
{
	for (unsigned int i = 0; i < biffiles.size(); i++) {
		free( biffiles[i].name );
	}
}

/*
static bool exists(char *file)
{
	FILE *f = fopen( file, "rb" );
	if (f) {
		fclose(f);
		return true;
	}
	return false;
}
*/

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

static bool PathExists(BIFEntry *entry, std::vector<std::string> pathlist)
{
	size_t i;
	
	for(i=0;i<pathlist.size();i++) {
printf("Trying %s of %ld\n", pathlist[i].c_str(), pathlist.size() );

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

	if (core->GameOnCD) {
		int mask = 1<<2;
		for(int cd = 1; cd<=MAX_CD; cd++) {
			if ((entry->BIFLocator & mask) != 0) {
				entry->cd = cd;
				break;
			}
		}

		if (!entry->cd) {
			printStatus( "ERROR", LIGHT_RED );
			printf( "Cannot find %s... Resource unavailable.\n",
					entry->name );
			entry->found = false;
			return;
		}
		entry->found = PathExists(entry, core->CD[entry->cd-1]);
		return;
	}

	for (int i = 0; i < MAX_CD; i++) {
		if (PathExists(entry, core->CD[i]) ) {
			entry->found = true;
			return;
		}
	}

	printMessage( "KEYImporter", " ", WHITE );
	printf( "Cannot find %s...", entry->name );
	printStatus( "ERROR", LIGHT_RED );
}

bool KEYImporter::Open(const char *resfile, const char *desc)
{
	description = desc;
	if (!core->IsAvailable( IE_BIF_CLASS_ID )) {
		printf( "[ERROR]\nAn Archive Plug-in is not Available\n" );
		return false;
	}
	unsigned int i;
	// NOTE: Interface::Init has already resolved resfile.
	printMessage( "KEYImporter", "Opening ", WHITE );
	printf( "%s...", resfile );
	FileStream* f = new FileStream();
	if (!f->Open( resfile )) {
		printStatus( "ERROR", LIGHT_RED );
		printMessage( "KEYImporter", "Cannot open Chitin.key\n", LIGHT_RED );
		textcolor( WHITE );
		delete( f );
		return false;
	}
	printStatus( "OK", LIGHT_GREEN );
	printMessage( "KEYImporter", "Checking file type...", WHITE );
	char Signature[8];
	f->Read( Signature, 8 );
	if (strncmp( Signature, "KEY V1  ", 8 ) != 0) {
		printStatus( "ERROR", LIGHT_RED );
		printMessage( "KEYImporter", "File has an Invalid Signature.\n",
			LIGHT_RED );
		textcolor( WHITE );
		delete( f );
		return false;
	}
	printStatus( "OK", LIGHT_GREEN );
	printMessage( "KEYImporter", "Reading Resources...\n", WHITE );
	ieDword BifCount, ResCount, BifOffset, ResOffset;
	f->ReadDword( &BifCount );
	f->ReadDword( &ResCount );
	f->ReadDword( &BifOffset );
	f->ReadDword( &ResOffset );
	printMessage( "KEYImporter", " ", WHITE );
	printf( "BIF Files Count: %d (Starting at %d Bytes)\n", BifCount,
		BifOffset );
	printMessage( "KEYImporter", " ", WHITE );
	printf( "RES Count: %d (Starting at %d Bytes)\n", ResCount, ResOffset );
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
		if (be.name[0] == PathDelimiter) {
			// totl has '\data\zcMHar.bif' in the key file, and the CaseSensitive
			// code breaks with that extra slash, so simple fix: remove it
			ASCIIZLen--;
			for (int p = 0; p < ASCIIZLen; p++)
				be.name[p] = be.name[p + 1];
			// (if you change this, try moving to ar9700 for testing)
		}
		FindBIF(&be);
		biffiles.push_back( be );
	}
	f->Seek( ResOffset, GEM_STREAM_START );
	resources.InitHashTable( ResCount < 17 ? 17 : ResCount );
	for (i = 0; i < ResCount; i++) {
		RESEntry re;
		f->ReadResRef( re.ResRef );
		f->ReadWord( &re.Type );
		f->ReadDword( &re.ResLocator );
		resources.SetAt( re.ResRef, re.Type, re.ResLocator );
	}
	printMessage( "KEYImporter", "Resources Loaded...", WHITE );
	printStatus( "OK", LIGHT_GREEN );
	delete( f );
	return true;
}

bool KEYImporter::HasResource(const char* resname, SClass_ID type)
{
	unsigned int ResLocator;
	return resources.Lookup( resname, type, ResLocator );
}

bool KEYImporter::HasResource(const char* resname, const ResourceDesc &type)
{
	return HasResource(resname, type.GetKeyType());
}

static void FindBIFOnCD(BIFEntry *entry)
{
	if (file_exists(entry->path)) {
		entry->found = true;
		return;
	}

	core->WaitForDisc( entry->cd, entry->path );

	entry->found = PathExists(entry, core->CD[entry->cd-1]);
}

DataStream* KEYImporter::GetStream(const char *resname, ieWord type)
{
	unsigned int ResLocator;

	if (type == 0)
		return NULL;
	if (resources.Lookup( resname, type, ResLocator )) {
		int bifnum = ( ResLocator & 0xFFF00000 ) >> 20;

		if (core->GameOnCD && (biffiles[bifnum].cd != 0))
			FindBIFOnCD(&biffiles[bifnum]);
		if (!biffiles[bifnum].found) {
			printf( "Cannot find %s... Resource unavailable.\n",
					biffiles[bifnum].name );
			return NULL;
		}

		PluginHolder<ArchiveImporter> ai(IE_BIF_CLASS_ID);
		if (ai->OpenArchive( biffiles[bifnum].path ) == GEM_ERROR) {
			printf("Cannot open archive %s\n", biffiles[bifnum].path );
			return NULL;
		}
		DataStream* ret = ai->GetStream( ResLocator, type );
		if (ret) {
			strnlwrcpy( ret->filename, resname, 8 );
			strcat( ret->filename, core->TypeExt( type ) );
			return ret;
		}
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
