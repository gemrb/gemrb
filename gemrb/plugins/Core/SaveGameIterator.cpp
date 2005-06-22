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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/SaveGameIterator.cpp,v 1.27 2005/06/22 15:55:26 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "SaveGameIterator.h"
#include "Interface.h"

SaveGame::SaveGame(char* path, char* name, char* prefix, int pCount)
{
	strncpy( Prefix, prefix, sizeof( Prefix ) );
	strncpy( Path, path, sizeof( Path ) );
	strncpy( Name, name, sizeof( Name ) );
	PortraitCount = pCount;
	char nPath[_MAX_PATH];
	struct stat my_stat;
	sprintf( nPath, "%s%s%s.bmp", Path, SPathDelimiter, Prefix );
#ifndef WIN32
	ResolveFilePath( nPath );
#endif
	memset(&my_stat,0,sizeof(my_stat));
	stat( nPath, &my_stat );
	strftime( Date, _MAX_PATH, "%c", localtime( &my_stat.st_mtime ) );
}

SaveGame::~SaveGame()
{
}

DataStream* SaveGame::GetPortrait(int index)
{
		if (index > PortraitCount) {
			return NULL;
		}
		char nPath[_MAX_PATH];
		sprintf( nPath, "%s%sPORTRT%d.bmp", Path, SPathDelimiter, index );
#ifndef WIN32
		ResolveFilePath( nPath );
#endif
		FileStream* fs = new FileStream();
		fs->Open( nPath, true );
		return fs;
}

DataStream* SaveGame::GetScreen()
{
		char nPath[_MAX_PATH];
		sprintf( nPath, "%s%s%s.bmp", Path, SPathDelimiter, Prefix );
#ifndef WIN32
		ResolveFilePath( nPath );
#endif
		FileStream* fs = new FileStream();
		fs->Open( nPath, true );
		return fs;
}

DataStream* SaveGame::GetGame()
{
		char nPath[_MAX_PATH];
		sprintf( nPath, "%s%s%s.gam", Path, SPathDelimiter, Prefix );
#ifndef WIN32
		ResolveFilePath( nPath );
#endif
		FileStream* fs = new FileStream();
		fs->Open( nPath, true );
		return fs;
}

DataStream* SaveGame::GetWmap()
{
		char nPath[_MAX_PATH];
		sprintf( nPath, "%s%s%s.wmp", Path, SPathDelimiter, "worldmap" );
#ifndef WIN32
		ResolveFilePath( nPath );
#endif
		FileStream* fs = new FileStream();
		fs->Open( nPath, true );
		return fs;
}

DataStream* SaveGame::GetSave()
{
		char nPath[_MAX_PATH];
		sprintf( nPath, "%s%s%s.sav", Path, SPathDelimiter, Prefix );
#ifndef WIN32
		ResolveFilePath( nPath );
#endif
		FileStream* fs = new FileStream();
		fs->Open( nPath, true );
		return fs;
}

SaveGameIterator::SaveGameIterator(void)
{
	loaded = false;
}

SaveGameIterator::~SaveGameIterator(void)
{
	for (charlist::iterator i = save_slots.begin();i!=save_slots.end();i++) {
		free (*i);
	}
//	unsigned int i = save_slots.size();
//	while (i--) {
//		free( save_slots[i] );
//	}
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
	if (access( ftmp, R_OK ))
		return false;

	return true;
}

bool SaveGameIterator::RescanSaveGames()
{
	loaded = true;

	// delete old entries
	for (charlist::iterator i = save_slots.begin();i!=save_slots.end();i++) {
		free (*i);
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
			charlist::iterator i;

			for (i=save_slots.begin(); i!=save_slots.end() && stricmp((*i), de->d_name)<0; i++);
			save_slots.insert( i, strdup( de->d_name ) );
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

char *SaveGameIterator::GetSaveName(int index)
{
	if (index < 0 || index >= GetSaveGameCount())
		return NULL;

	charlist::iterator i=save_slots.begin();
	while (index--) {
		i++;
	}
	return (*i);
}

SaveGame* SaveGameIterator::GetSaveGame(int index)
{
	char* slotname  = GetSaveName(index);

	int prtrt = 0;
	char Path[_MAX_PATH];
	snprintf( Path, _MAX_PATH, "%s%s%s%s", core->SavePath, PlayMode(),
		 SPathDelimiter, slotname );


	char savegameName[_MAX_PATH]={0};
	int savegameNumber = 0;

	int cnt = sscanf( slotname, SAVEGAME_DIRECTORY_MATCHER, &savegameNumber, savegameName );
	if (cnt != 2) {
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

int SaveGameIterator::ExistingSlotName(int index)
{
	char strindex[10];

	snprintf(strindex, sizeof(strindex), "%09d", index);
	int idx = 0;
	for (charlist::iterator i = save_slots.begin();i!=save_slots.end();i++) {
		if (!strnicmp((*i), strindex, 9) ) {
			return idx;
		}
		idx++;
	}
	return -1;
}

int SaveGameIterator::CreateSaveGame(int index, const char *slotname)
{
	char Path[_MAX_PATH];
	char FName[12];

	//some of these restrictions might not be needed
	if (core->GetCurrentStore()) {
		return 1; //can't save while store is open
	}
	GameControl *gc = core->GetGameControl();
	if (gc && (gc->GetDialogueFlags()&DF_IN_DIALOG) ) {
		return 2; //can't save while in dialog?
	}

	//if index is not an existing savegame, we create a unique slotname
	if (index < 0 || index >= GetSaveGameCount()) {
		index=GetSaveGameCount();
		//leave space for autosaves
		//probably the hardcoded slot names should be read by this object
		//in that case 7 == size of hardcoded slot names array (savegame.2da)
		if (index<7) {
			index=7; 
		}
		while (ExistingSlotName(index) !=-1 ) {
			index++;
		}
	} else {
		int oldindex = ExistingSlotName(index);
		if (oldindex>=0) {
			char *oldslotname = GetSaveName(oldindex);
			snprintf( Path, _MAX_PATH, "%s%s%s%s", core->SavePath, PlayMode(), SPathDelimiter, oldslotname );
			core->DelTree(Path, false);
			rmdir(Path);
		}
	}
	snprintf( Path, _MAX_PATH, "%s%s%s%09d-%s", core->SavePath, PlayMode(), SPathDelimiter, index, slotname );
	mkdir(Path,S_IWRITE|S_IREAD);
	//save files here

	Game *game = core->GetGame();
	//saving areas to cache currently in memory
	size_t mc = game->GetLoadedMapCount();
	while (mc--) {
		Map *map = game->GetMap(mc);
		if (core->SwapoutArea(map)) {
			return -1;
		}
	}
	
	//compress files in cache named: .STO and .ARE
	//no .CRE would be saved in cache
	if (core->CompressSave(Path)) {
		return -1;
	}

	//Create .gam file from Game() object
	if (core->WriteGame(Path)) {
		return -1;
	}

	//Create .wmp file from WorldMap() object
	if (core->WriteWorldMap(Path)) {
		return -1;
	}

	//Create portraits
	ImageMgr *im = (ImageMgr *) core->GetInterface(IE_BMP_CLASS_ID);
	for (int i=0;i<game->GetPartySize(false); i++) {
		Actor *actor = game->GetPC(i, false);
		DataStream *str = core->GetResourceMgr()->GetResource(actor->GetPortrait(true), IE_BMP_CLASS_ID);
		if (str) {
			snprintf( Path, _MAX_PATH, "%s%s%s%09d-%s", core->SavePath, PlayMode(), SPathDelimiter, index, slotname);
			snprintf( FName, sizeof(FName), "PORTRT%d", i );
			FileStream outfile;
			outfile.Create(Path, FName, IE_BMP_CLASS_ID);
			im->Open( str, true);
			im->PutImage(&outfile,1);
		}
	}
	//area preview

	core->FreeInterface(im);
	return 0;
}

void SaveGameIterator::DeleteSaveGame(int index)
{
	char* slotname  = GetSaveName(index);
	if (!slotname) {
		return;
	}

	char Path[_MAX_PATH];
	snprintf( Path, _MAX_PATH, "%s%s%s%s", core->SavePath, PlayMode(), SPathDelimiter, slotname );
	core->DelTree( Path, false ); //remove all files from folder
	rmdir( Path );

	charlist::iterator i=save_slots.begin();
	while (index--) {
		i++;
	}

	free( (*i));
	save_slots.erase(i);

//	loaded = false;
}
