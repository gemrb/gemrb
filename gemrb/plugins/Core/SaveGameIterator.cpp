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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/SaveGameIterator.cpp,v 1.25 2005/06/14 22:29:38 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "SaveGameIterator.h"
#include "Interface.h"

SaveGameIterator::SaveGameIterator(void)
{
	loaded = false;
}

SaveGameIterator::~SaveGameIterator(void)
{
	unsigned int i = save_slots.size();
	while(i--) {
		free( save_slots[i] );
	}
}

static const char* PlayMode()
{
	ieDword playmode = 1;

	core->GetDictionary()->Lookup( "PlayMode", playmode );
	if (playmode == 2) {
		return "mpsave";
	}
	return "save";
}

/*
 * Return true if directory Path/slotname is a potential save game
 *   slot, otherwise return false.
 */
static bool IsSaveGameSlot(const char* Path, const char* slotname)
{
	char savegameName[_MAX_PATH];
	int savegameNumber = 0;

	if (slotname[0] == '.')
		return false;

	int cnt = sscanf( slotname, SAVEGAME_DIRECTORY_MATCHER, &savegameNumber, savegameName );
	if (cnt != 2) { 
		//The matcher didn't match: either this is not a valid dir
		//   or the SAVEGAME_DIRECTORY_MATCHER needs updating.
		printf( "Invalid savegame directory '%s' in %s.\n", slotname, Path );
		return false;
	}

	//The matcher got matched correctly.
	printf( "[Number = %d, Name = %s]\n", savegameNumber, savegameName );


	char dtmp[_MAX_PATH];
	snprintf( dtmp, _MAX_PATH, "%s%s%s", Path, SPathDelimiter, slotname );

	struct stat  fst;
	if (stat( dtmp, &fst ))
		return false;

	if (! S_ISDIR( fst.st_mode ))
		return false;

	char ftmp[_MAX_PATH];
	snprintf( ftmp, _MAX_PATH, "%s%s%s.bmp", dtmp, SPathDelimiter,
		 core->GameNameResRef );

#ifndef WIN32
	ResolveFilePath( ftmp );
#endif
	//FILE* exist = fopen( ftmp, "rb" );
	//if (!exist)
	//	return false;
	//fclose( exist );
	if (access( ftmp, R_OK ))
		return false;

	return true;
}

bool SaveGameIterator::RescanSaveGames()
{
	loaded = true;

	// delete old entries
	unsigned int i = save_slots.size();
	while(i--) {
		free( save_slots[i] );
	}
	save_slots.clear();

	char Path[_MAX_PATH];
	snprintf( Path, _MAX_PATH, "%s%s", core->SavePath, PlayMode() );

	DIR* dir = opendir( Path );
	if (dir == NULL) //If we cannot open the Directory
	{
		return false;
	}
	struct dirent* de = readdir( dir );  //Lookup the first entry in the Directory
	if (de == NULL) {
		closedir( dir );
		return false;
	}
	do {
		if (IsSaveGameSlot( Path, de->d_name )) {
			save_slots.push_back( strdup( de->d_name ) );
		}

	} while (( de = readdir( dir ) ) != NULL);
	closedir( dir );  //No other files in the directory, close it

	return true;
}

int SaveGameIterator::GetSaveGameCount()
{
	if (! loaded && ! RescanSaveGames())
		return -1;

	return save_slots.size();
}

SaveGame* SaveGameIterator::GetSaveGame(int index)
{
	if (index < 0 || index >= GetSaveGameCount())
		return NULL;

	char* slotname  = save_slots[index];


	int prtrt = 0;
	char Path[_MAX_PATH];
	snprintf( Path, _MAX_PATH, "%s%s%s%s", core->SavePath, PlayMode(),
		 SPathDelimiter, slotname );


	char savegameName[_MAX_PATH]={0};
	int savegameNumber = 0;

	int cnt = sscanf( slotname, SAVEGAME_DIRECTORY_MATCHER, &savegameNumber, savegameName );
	if(cnt != 2) {
		printf( "Invalid savegame directory '%s' in %s.\n", slotname, Path );
		return false;
	}
	printf( "[Number = %d, Name = %s]\n", savegameNumber, savegameName );

	DIR* ndir = opendir( Path );
	//If we cannot open the Directory
	if (ndir == NULL) {
		return NULL;
	}
	struct dirent* de2 = readdir( ndir );  //Lookup the first entry in the Directory
	if (de2 == NULL) {
		// No first entry!!!
		closedir( ndir );
		return NULL;
	}
	do {
		if (strnicmp( de2->d_name, "PORTRT", 6 ) == 0)
			prtrt++;
	} while (( de2 = readdir( ndir ) ) != NULL);
	closedir( ndir );  //No other files in the directory, close it

	SaveGame* sg = new SaveGame( Path, savegameName, core->GameNameResRef, prtrt );
	return sg;
}

int SaveGameIterator::CreateSaveGame(int index, const char *slotname)
{
	char Path[_MAX_PATH];

	if (index < 0 || index >= GetSaveGameCount()) {
		index=GetSaveGameCount();
	} else {
		char* oldslotname  = save_slots[index];
		snprintf( Path, _MAX_PATH, "%s%s%s%s", core->SavePath, PlayMode(), SPathDelimiter, oldslotname );
		core->DelTree(Path, false);
		rmdir(Path);
	}
	snprintf( Path, _MAX_PATH, "%s%s%s%s", core->SavePath, PlayMode(), SPathDelimiter, slotname );
	mkdir(Path,S_IWRITE|S_IREAD);
	//save files here
	//some of these restrictions might not be needed
	if (core->GetCurrentStore()) {
		return 1; //can't save while store is open
	}
	GameControl *gc = core->GetGameControl();
	if (gc && (gc->GetDialogueFlags()&DF_IN_DIALOG) ) {
		return 2; //can't save while in dialog?
	}

	Game *game = core->GetGame();
	//saving areas to cache currently in memory
	size_t mc = game->GetLoadedMapCount();
	while(mc--) {
		Map *map = game->GetMap(mc);
		if(core->SwapoutArea(map)) {
			return -1;
		}
	}
	
	//compress files in cache named: .STO and .ARE
	//no .CRE would be saved in cache
	if(core->CompressSave(Path)) {
		return -1;
	}

	//Create .gam file from Game() object
	if(core->WriteGame(Path)) {
		return -1;
	}

	//Create .wmp file from WorldMap() object
	if(core->WriteWorldMap(Path)) {
		return -1;
	}

	//Create .bmp's (area preview and portraits)
	return 0;
}

void SaveGameIterator::DeleteSaveGame(int index)
{
	if (index < 0 || index >= GetSaveGameCount())
		return;

	char* slotname  = save_slots[index];


	char Path[_MAX_PATH];
	snprintf( Path, _MAX_PATH, "%s%s%s%s", core->SavePath, PlayMode(), SPathDelimiter, slotname );
	core->DelTree( Path, false ); //remove all files from folder
	rmdir( Path );

	delete slotname;
	save_slots.erase(save_slots.begin()+index);

	loaded = false;
}
