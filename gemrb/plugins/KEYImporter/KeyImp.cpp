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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id$
 *
 */

#include "../../includes/win32def.h"
#include "KeyImp.h"
#include "../../includes/globals.h"
#include "../Core/FileStream.h"
#include "../Core/Interface.h"
#include "../Core/ArchiveImporter.h"
#include "../Core/AnimationMgr.h"
#include "../Core/ImageMgr.h"
#include "../Core/ImageFactory.h"
#include "../Core/Factory.h"
#ifndef WIN32
#include <unistd.h>
#endif

KeyImp::KeyImp(void)
{
#ifndef WIN32
	// FIXME: Use FindInPath() and do NOT compile out for Windows
	if (core->CaseSensitive) {
		char path[_MAX_PATH];
		PathJoin( path, core->GamePath, core->GameOverridePath, NULL );
		if (!dir_exists( path )) {
			core->GameOverridePath[0] = toupper( core->GameOverridePath[0] );
		}
		PathJoin( path, core->GamePath, core->GameDataPath, NULL );
		if (!dir_exists( path )) {
			core->GameDataPath[0] = toupper( core->GameDataPath[0] );
		}
	}
#endif
}

KeyImp::~KeyImp(void)
{
	for (unsigned int i = 0; i < biffiles.size(); i++) {
		free( biffiles[i].name );
	}
}

bool KeyImp::LoadResFile(const char* resfile)
{
	unsigned int i;
	char fn[_MAX_PATH] = {
		0
	};
#ifndef WIN32
	if (core->CaseSensitive) {
		ExtractFileFromPath( fn, resfile );
		char* newname = FindInDir( core->GamePath, fn );
		if (newname) {
			PathJoin( fn, core->GamePath, newname, NULL );
			free( newname );
		}
	} else
#endif
	{
		strcpy( fn, resfile );
	}
	printMessage( "KEYImporter", "Opening ", WHITE );
	printf( "%s...", fn );
	FileStream* f = new FileStream();
	if (!f->Open( fn )) {
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
#ifndef WIN32
		for (int p = 0; p < ASCIIZLen; p++) {
			//some MAC versions use : as delimiter
			if (be.name[p] == '\\' || be.name[p] == ':')
				be.name[p] = PathDelimiter;
		}
		if (core->CaseSensitive) {
			char fullPath[_MAX_PATH], tmpPath[_MAX_PATH] = {0},
				fn[_MAX_PATH] = {0};
			char* ptr = strrchr( be.name, PathDelimiter );
			if (ptr) {
				strncpy( tmpPath, be.name, ptr - be.name );
				char* paths[6] = { core->GamePath, core->CD1, core->CD2, core->CD3, core->CD4, core->CD5};
				char* dirname;
				for (int i = 0; i < 6; i++) {
					dirname = FindInDir( paths[i], tmpPath );
					if (dirname) {
						strncpy( tmpPath, dirname, sizeof(tmpPath) );
						FixPath( tmpPath, 1);
						break;
					}
				}
				if (!dirname) {
					strncpy( tmpPath, be.name, ( ptr + 1 ) - be.name );
				}
			}
			PathJoin( fullPath, core->GamePath, tmpPath, NULL );
			if (ptr) {
				ExtractFileFromPath( fn, be.name );
			} else {
				strcpy( fn, be.name );
			}
			char* newname = FindInDir( fullPath, fn );
			if (newname) {
				PathJoin( be.name, tmpPath, newname, NULL );
				free( newname );
			}
		}
#endif
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

static bool FindIn(const char *BasePath, const char *Path, const char *ResRef, SClass_ID Type)
{
	char p[_MAX_PATH], f[_MAX_PATH] = {0};
	strncpy(f, ResRef, 8);
	f[8] = 0;
	strcat(f, core->TypeExt(Type));
	strlwr(f);
	PathJoin( p, BasePath, Path, f, NULL );
	ResolveFilePath(p);
	FILE * exist = fopen(p, "rb");
	if(exist) {
		fclose(exist);
		return true;
	}
	return false;
}

static FileStream *SearchIn(const char * BasePath,const char * Path,const char * ResRef, SClass_ID Type, const char *foundMessage)
{
	char p[_MAX_PATH], f[_MAX_PATH] = {0};
	strncpy(f, ResRef, 8);
	f[8] = 0;
	strcat(f, core->TypeExt(Type));
	strlwr(f);
	PathJoin( p, BasePath, Path, f, NULL );
	ResolveFilePath(p);
	FILE * exist = fopen(p, "rb");
	if(exist) {
		fclose(exist);
		FileStream * fs = new FileStream();
		if(!fs) return NULL;
		fs->Open(p, true);
		printBracket(foundMessage, LIGHT_GREEN); printf("\n");
		return fs;
	}
	return NULL;
}

bool KeyImp::HasResource(const char* resname, SClass_ID type, bool silent)
{
	char path[_MAX_PATH];
	//Search it in the GemRB override Directory
	PathJoin( path, "override", core->GameType, NULL ); //this shouldn't change
	if (FindIn( core->CachePath, "", resname, type)) return true;
	if (FindIn( core->GemRBOverridePath, path, resname, type)) return true;
	if (FindIn( core->GamePath, core->GameOverridePath, resname, type)) return true;
	if (FindIn( core->GamePath, core->GameSoundsPath, resname, type)) return true;
	if (FindIn( core->GamePath, core->GameScriptsPath, resname, type)) return true;
	if (FindIn( core->GamePath, core->GamePortraitsPath, resname, type)) return true;
	if (FindIn( core->GamePath, core->GameDataPath, resname, type)) return true;

	unsigned int ResLocator;
	if (resources.Lookup( resname, type, ResLocator )) {
		return true;
	}
	if (silent)
		return false;

	printMessage( "KEYImporter", "Searching for ", WHITE );
	printf( "%.8s%s...", resname, core->TypeExt( type ) );
	printStatus( "NOT FOUND", YELLOW );
	return false;
}

DataStream* KeyImp::GetResource(const char* resname, SClass_ID type)
{
	char path[_MAX_PATH];
	char BasePath[_MAX_PATH] = {
		0
	};
	printMessage( "KEYImporter", "Searching for ", WHITE );
	printf( "%.8s%s...", resname, core->TypeExt( type ) );
	//Search it in the GemRB override Directory
	PathJoin( path, "override", core->GameType, NULL ); //this shouldn't change

	FileStream *fs;

	fs=SearchIn( core->CachePath, "", resname, type,
		"Found in Cache" );
	if (fs) return fs;

	fs=SearchIn( core->GemRBOverridePath, path, resname, type,
		"Found in GemRB Override" );
	if (fs) return fs;

	fs=SearchIn( core->GamePath, core->GameOverridePath, resname, type,
		"Found in Override" );
	if (fs) return fs;

	fs=SearchIn( core->GamePath, core->GameSoundsPath, resname, type,
		"Found in Sounds" );
	if (fs) return fs;

	fs=SearchIn( core->GamePath, core->GameScriptsPath, resname, type,
		"Found in Scripts" );
	if (fs) return fs;

	fs=SearchIn( core->GamePath, core->GamePortraitsPath, resname, type,
		"Found in Portraits" );
	if (fs) return fs;

	fs=SearchIn( core->GamePath, core->GameDataPath, resname, type,
		"Found in Data" );
	if (fs) return fs;

	unsigned int ResLocator;

	//the word masking is a hack for synonyms, currently used for bcs==bs
	if (resources.Lookup( resname, type&0xffff, ResLocator )) {
		if (!core->IsAvailable( IE_BIF_CLASS_ID )) {
			printf( "[ERROR]\nAn Archive Plug-in is not Available\n" );
			return NULL;
		}
		int bifnum = ( ResLocator & 0xFFF00000 ) >> 20;
		FILE* exist = NULL;
		if (exist == NULL) {
			PathJoin( path, core->GamePath, biffiles[bifnum].name, NULL );
			exist = fopen( path, "rb" );
			if (!exist) {
				path[0] = toupper( path[0] );
				exist = fopen( path, "rb" );
			}
			if (!exist) {
				PathJoin( path, core->GamePath, biffiles[bifnum].name, NULL );
				strcpy( path + strlen( path ) - 4, ".cbf" );
				//strcpy( path, core->GamePath );
				//strncat( path, biffiles[bifnum].name,
				//	strlen( biffiles[bifnum].name ) - 4 );
				//strcat( path, ".cbf" );
				exist = fopen( path, "rb" );
				if (!exist) {
					path[0] = toupper( path[0] );
					exist = fopen( path, "rb" );
				}
			}
		}
		if (exist == NULL) {
			int CD;
			if (( biffiles[bifnum].BIFLocator & ( 1 << 2 ) ) != 0) {
				strcpy( BasePath, core->CD1 );
				CD = 1;
			} else if (( biffiles[bifnum].BIFLocator & ( 1 << 3 ) ) != 0) {
				strcpy( BasePath, core->CD2 );
				CD = 2;
			} else if (( biffiles[bifnum].BIFLocator & ( 1 << 4 ) ) != 0) {
				strcpy( BasePath, core->CD3 );
				CD = 3;
			} else if (( biffiles[bifnum].BIFLocator & ( 1 << 5 ) ) != 0) {
				strcpy( BasePath, core->CD4 );
				CD = 4;
			} else if (( biffiles[bifnum].BIFLocator & ( 1 << 6 ) ) != 0) {
				strcpy( BasePath, core->CD5 );
				CD = 5;
			} else {
					printStatus( "ERROR", LIGHT_RED );
				printf( "Cannot find %s... Resource unavailable.\n",
					biffiles[bifnum].name );
					return NULL;
			}
			PathJoin( path, BasePath, biffiles[bifnum].name, NULL );
#ifndef WIN32
			ResolveFilePath(path);
#endif
			exist = fopen( path, "rb" );
			if (exist == NULL && core->GameOnCD) {
				core->WaitForDisc( CD, BasePath );
				exist = fopen( path, "rb" );
			}
			if (exist == NULL) {
				//Trying CBF Extension
				PathJoin( path, BasePath, biffiles[bifnum].name, NULL );
				strcpy( path + strlen( path ) - 4, ".cbf" );

				//strcpy( path, BasePath );
				//strncat( path, biffiles[bifnum].name,
				//	strlen( biffiles[bifnum].name ) - 4 );
				//strcat( path, ".cbf" );
#ifndef WIN32
				ResolveFilePath(path);
#endif
				exist = fopen( path, "rb" );
				if (!exist) {
					printStatus( "ERROR", LIGHT_RED );
					printf( "Cannot find %s\n", path );
					return NULL;
				}
			}

			fclose( exist );
		} else
			fclose( exist );
		ArchiveImporter* ai = ( ArchiveImporter* )
			core->GetInterface( IE_BIF_CLASS_ID );
		if (ai->OpenArchive( path ) == GEM_ERROR) {
			printf("Cannot open archive %s\n", path );
			core->FreeInterface( ai );
			return NULL;
		}
		DataStream* ret = ai->GetStream( ResLocator, type );
		core->FreeInterface( ai );
		if (ret) {
			strnlwrcpy( ret->filename, resname, 8 );
			strcat( ret->filename, core->TypeExt( type ) );
		}
		return ret;
	}
	printStatus( "ERROR", LIGHT_RED );
	return NULL;
}

void* KeyImp::GetFactoryResource(const char* resname, SClass_ID type,
	unsigned char mode)
{
	int fobjindex = gamedata->GetFactory()->IsLoaded(resname,type);
	// already cached
	if ( fobjindex != -1)
		return gamedata->GetFactory()->GetFactoryObject( fobjindex );

	switch (type) {
	case IE_BAM_CLASS_ID:
	{
		DataStream* ret = GetResource( resname, type );
		if (ret) {
			AnimationMgr* ani = ( AnimationMgr* )
				core->GetInterface( IE_BAM_CLASS_ID );
			if (!ani)
				return NULL;
			ani->Open( ret, true );
			AnimationFactory* af = ani->GetAnimationFactory( resname, mode );
			core->FreeInterface( ani );
			gamedata->GetFactory()->AddFactoryObject( af );
			return af;
		}
		return NULL;
	}
	case IE_BMP_CLASS_ID:
	{
		// check PNG first
		DataStream* ret = GetResource( resname, IE_PNG_CLASS_ID );
		if (ret) {
			ImageMgr* img = (ImageMgr*) core->GetInterface( IE_PNG_CLASS_ID );
			if (img) {
				img->Open( ret, true );
				ImageFactory* fact = img->GetImageFactory( resname );
				core->FreeInterface( img );
				gamedata->GetFactory()->AddFactoryObject( fact );
				return fact;
			}
		}

		ret = GetResource( resname, IE_BMP_CLASS_ID );
		if (ret) {
			ImageMgr* img = (ImageMgr*) core->GetInterface( IE_BMP_CLASS_ID );
			if (img) {
				img->Open( ret, true );
				ImageFactory* fact = img->GetImageFactory( resname );
				core->FreeInterface( img );
				gamedata->GetFactory()->AddFactoryObject( fact );
				return fact;
			}
		}

		return NULL;
	}
	default:
		printf( "\n" );
		printMessage( "KEYImporter", " ", WHITE );
		printf( "%s files are not supported.\n", core->TypeExt( type ) );
		return NULL;
	}
}
