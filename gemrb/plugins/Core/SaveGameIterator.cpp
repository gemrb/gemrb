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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/SaveGameIterator.cpp,v 1.17 2004/02/29 15:24:59 hrk Exp $
 *
 */

#include "../../includes/win32def.h"
#include "SaveGameIterator.h"
#include "Interface.h"

extern Interface* core;

SaveGameIterator::SaveGameIterator(void)
{
}

SaveGameIterator::~SaveGameIterator(void)
{
}

static void DelTree(char* Pt)
{
	char Path[_MAX_PATH];
	strcpy( Path, Pt );
	DIR* dir = opendir( Path );
	if (dir == NULL) {
		return;
	}
	struct dirent* de = readdir( dir );  //Lookup the first entry in the Directory
	if (de == NULL) {
		closedir( dir );
		return;
	}
	do {
		char dtmp[_MAX_PATH];
		struct stat fst;
		sprintf( dtmp, "%s%s%s", Path, SPathDelimiter, de->d_name );
		stat( dtmp, &fst );
		if (S_ISDIR( fst.st_mode ))
			continue;
		if (de->d_name[0] == '.')
			continue;
		unlink( dtmp );
	} while (( de = readdir( dir ) ) != NULL);
	closedir( dir );
}

static const char* PlayMode()
{
	unsigned long playmode = 1;

	core->GetDictionary()->Lookup( "PlayMode", playmode );
	if (playmode == 2) {
		return "mpsave";
	}
	return "save";
}

int SaveGameIterator::GetSaveGameCount()
{
	int count = 0;
	char Path[_MAX_PATH];
	const char* SaveFolder = PlayMode();
	sprintf( Path, "%s%s", core->GamePath, SaveFolder );
	DIR* dir = opendir( Path );
	if (dir == NULL) //If we cannot open the Directory
	{
		return -1;
	}
	struct dirent* de = readdir( dir );  //Lookup the first entry in the Directory
	if (de == NULL) {
		closedir( dir );
		return -1;
	}
	do {
		//Iterate through all the available modules to load
		char dtmp[_MAX_PATH];
		struct stat fst;
		sprintf( dtmp, "%s%s%s", Path, SPathDelimiter, de->d_name );
		stat( dtmp, &fst );
		if (S_ISDIR( fst.st_mode )) {
			if (de->d_name[0] == '.')
				continue;
			char ftmp[_MAX_PATH];
			sprintf( ftmp, "%s%s%s.bmp", dtmp, SPathDelimiter,
				core->GameNameResRef );
#ifndef WIN32
			ResolveFilePath( ftmp );
#endif
			FILE* exist = fopen( ftmp, "rb" );
			if (!exist)
				continue;
			fclose( exist );
			char savegameName[_MAX_PATH];
			int savegameNumber = 0;
			int cnt = sscanf( de->d_name, SAVEGAME_DIRECTORY_MATCHER, &savegameNumber, savegameName );
			if (cnt == 2) { //The matcher got matched correctly.
				printf( "[Number = %d, Name = %s]\n", savegameNumber, savegameName );
				count++;
			}
			else { //The matcher didn't match: either this is not a valid directory or the SAVEGAME_DIRECTORY_MATCHER needs updating.
				printf( "[Invalid savegame directory '%s' in %s.\n", de->d_name, Path);
			}
		}
	} while (( de = readdir( dir ) ) != NULL);
	closedir( dir );  //No other files in the directory, close it
	return count;
}

SaveGame* SaveGameIterator::GetSaveGame(int index, bool Remove)
{
	int count = -1, prtrt = 0;
	char Path[_MAX_PATH];
	char dtmp[_MAX_PATH];
	const char* SaveFolder = PlayMode();
	sprintf( Path, "%s%s", core->GamePath, SaveFolder );
	DIR* dir = opendir( Path );
	if (dir == NULL) //If we cannot open the Directory
	{
		return NULL;
	}
	struct dirent* de = readdir( dir );  //Lookup the first entry in the Directory
	if (de == NULL) //If no entry exists just return
	{
		return NULL;
	}
	char savegameName[_MAX_PATH];
	int savegameNumber = 0;
	do {
		//Iterate through all the available modules to load
		struct stat fst;
		sprintf( dtmp, "%s%s%s", Path, SPathDelimiter, de->d_name );
		stat( dtmp, &fst );
		if (S_ISDIR( fst.st_mode )) {
			if (de->d_name[0] == '.')
				continue;
			char ftmp[_MAX_PATH];
			sprintf( ftmp, "%s%s%s.bmp", dtmp, SPathDelimiter,
				core->GameNameResRef );
#ifndef WIN32
			ResolveFilePath( ftmp );
#endif
			FILE* exist = fopen( ftmp, "rb" );
			if (!exist)
				continue;
			fclose( exist );
			int cnt = sscanf( de->d_name, SAVEGAME_DIRECTORY_MATCHER, &savegameNumber, savegameName );
			if (cnt == 2) {
				printf( "[Number = %d, Name = %s]\n", savegameNumber, savegameName );
				count++;
			}
			else {
				printf( "[Invalid savegame directory '%s' in %s.\n", de->d_name, Path);
			}
			if (count == index) {
				if (Remove) {
					sprintf( Path, "%s%s%s%s", core->GamePath, SaveFolder,
						SPathDelimiter, de->d_name );
					DelTree( Path );
					rmdir( Path );
					break;
				}
				sprintf( Path, "%s%s%s%s", core->GamePath, SaveFolder,
					SPathDelimiter, de->d_name );
				DIR* ndir = opendir( Path );
				//If we cannot open the Directory
				if (ndir == NULL) {
					closedir( dir );
					return NULL;
				}
				struct dirent* de2 = readdir( ndir );  //Lookup the first entry in the Directory
				if (de2 == NULL) {
					// No first entry!!!
					closedir( dir );
					closedir( ndir );
					return NULL;
				}
				do {
					if (strnicmp( de2->d_name, "PORTRT", 6 ) == 0)
						prtrt++;
				} while (( de2 = readdir( ndir ) ) != NULL);
				closedir( ndir );  //No other files in the directory, close it
				break;
			}
		}
	} while (( de = readdir( dir ) ) != NULL);
	closedir( dir );  //No other files in the directory, close it
	if (Remove || ( de == NULL )) {
		return NULL;
	}
	SaveGame* sg = new SaveGame( dtmp, savegameName, core->GameNameResRef, prtrt );
	return sg;
}
