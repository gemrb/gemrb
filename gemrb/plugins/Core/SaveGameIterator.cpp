/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "../../includes/win32def.h"
#include "../../includes/strrefs.h"
#include "SaveGameIterator.h"
#include "Interface.h"
#include "SaveGameMgr.h"
#include "GameControl.h"
#include "Video.h"
#include <vector>
#include <cassert>

SaveGame::SaveGame(char* path, char* name, char* prefix, int pCount, int saveID)
{
	strncpy( Prefix, prefix, sizeof( Prefix ) );
	strncpy( Path, path, sizeof( Path ) );
	strncpy( Name, name, sizeof( Name ) );
	PortraitCount = pCount;
	SaveID = saveID;
	char nPath[_MAX_PATH];
	struct stat my_stat;
	sprintf( nPath, "%s%s%s.bmp", Path, SPathDelimiter, Prefix );
	ResolveFilePath( nPath );
	memset(&my_stat,0,sizeof(my_stat));
	stat( nPath, &my_stat );
	strftime( Date, _MAX_PATH, "%c", localtime( &my_stat.st_mtime ) );
	manager.AddSource(path, Name, PLUGIN_RESOURCE_DIRECTORY);
}

SaveGame::~SaveGame()
{
}

ImageMgr* SaveGame::GetPortrait(int index)
{
	if (index > PortraitCount) {
		return NULL;
	}
	char nPath[_MAX_PATH];
	sprintf( nPath, "PORTRT%d", index );
	return static_cast<ImageMgr*>(manager.GetResource(nPath, &ImageMgr::ID, true));
}

ImageMgr* SaveGame::GetScreen()
{
	return static_cast<ImageMgr*>(manager.GetResource(Prefix, &ImageMgr::ID, true));
}

DataStream* SaveGame::GetGame()
{
	return manager.GetResource(Prefix, IE_GAM_CLASS_ID, true);
}

DataStream* SaveGame::GetWmap()
{
	return manager.GetResource(core->WorldMapName, IE_WMP_CLASS_ID, true);
}

DataStream* SaveGame::GetSave()
{
	return manager.GetResource(Prefix, IE_SAV_CLASS_ID, true);
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
}

void SaveGameIterator::Invalidate()
{
	loaded = false;
}

/* mission pack save */
static const char* SaveDir()
{
	ieDword playmode = 0;
	core->GetDictionary()->Lookup( "SaveDir", playmode );
	if (playmode == 1) {
		return "mpsave";
	}
	return "save";
}

#define FormatQuickSavePath(destination, i) \
	 snprintf(destination,sizeof(destination),"%s%s%s%09d-%s", \
		core->SavePath,SaveDir(), SPathDelimiter,i,folder);

/*
 * Returns the first 0 bit position of an integer
 */
static int GetHole(int n)
{
	int mask = 1;
	int value = 0;
	while(n&mask) {
		mask<<=1;
		value++;
	}
	return value;
}

/*
 * Returns the age of a quickslot entry. Returns 0 if it isn't a quickslot
 */
static int IsQuickSaveSlot(const char* match, const char* slotname)
{
	char savegameName[_MAX_PATH];
	int savegameNumber = 0;
	int cnt = sscanf( slotname, SAVEGAME_DIRECTORY_MATCHER, &savegameNumber, savegameName );
	if (cnt != 2) {
		return 0;
	}
	if (stricmp(savegameName, match) )
	{
		return 0;
	}
	return savegameNumber;
}
/*
 * Return true if directory Path/slotname is a potential save game
 * slot, otherwise return false.
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
		//or the SAVEGAME_DIRECTORY_MATCHER needs updating.
		printMessage( "SaveGameIterator", " ", LIGHT_RED );
		printf( "Invalid savegame directory '%s' in %s.\n", slotname, Path );
		return false;
	}

	//The matcher got matched correctly.
	char dtmp[_MAX_PATH];
	snprintf( dtmp, _MAX_PATH, "%s%s%s", Path, SPathDelimiter, slotname );

	struct stat fst;
	if (stat( dtmp, &fst ))
		return false;

	if (! S_ISDIR( fst.st_mode ))
		return false;

	char ftmp[_MAX_PATH];
	snprintf( ftmp, _MAX_PATH, "%s%s%s.bmp", dtmp, SPathDelimiter,
		 core->GameNameResRef );

	ResolveFilePath( ftmp );
	if (access( ftmp, R_OK )) {
		printMessage("SaveGameIterator"," ",YELLOW);
		printf("Ignoring slot %s because of no appropriate preview!\n", dtmp);
		return false;
	}

	snprintf( ftmp, _MAX_PATH, "%s%s%s.wmp", dtmp, SPathDelimiter,
		 core->WorldMapName );
	ResolveFilePath( ftmp );
	if (access( ftmp, R_OK )) {
		printMessage("SaveGameIterator"," ",YELLOW);
		printf("Ignoring slot %s because of no appropriate worldmap!\n", dtmp);
		return false;
	}

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
	snprintf( Path, _MAX_PATH, "%s%s", core->SavePath, SaveDir() );

	ResolveFilePath( Path );
	DIR* dir = opendir( Path );
	// create the save game directory at first access
	if (dir == NULL) {
		mkdir(Path,S_IWRITE|S_IREAD|S_IEXEC);
		chmod(Path,S_IWRITE|S_IREAD|S_IEXEC);
		dir = opendir( Path );
	}
	if (dir == NULL) { //If we cannot open the Directory
		return false;
	}
	struct dirent* de = readdir( dir ); //Lookup the first entry in the Directory
	if (de == NULL) {
		closedir( dir );
		return false;
	}
	do {
		if (IsSaveGameSlot( Path, de->d_name )) {
			charlist::iterator i;

			for (i=save_slots.begin(); i!=save_slots.end() && stricmp((*i), de->d_name)<0; i++) ;
			save_slots.insert( i, strdup( de->d_name ) );
		}
	} while (( de = readdir( dir ) ) != NULL);
	closedir( dir ); //No other files in the directory, close it
	return true;
}

int SaveGameIterator::GetSaveGameCount()
{
	if (! loaded && ! RescanSaveGames())
		return -1;

	return (int) save_slots.size();
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
	char* slotname = GetSaveName(index);
	if (!slotname) {
		return NULL;
	}

	int prtrt = 0;
	char Path[_MAX_PATH];
	//lets leave space for the filenames
	snprintf( Path, _MAX_PATH, "%s%s%s%s", core->SavePath, SaveDir(),
		 SPathDelimiter, slotname );

	char savegameName[_MAX_PATH]={0};
	int savegameNumber = 0;

	int cnt = sscanf( slotname, SAVEGAME_DIRECTORY_MATCHER, &savegameNumber, savegameName );
	//maximum pathlength == 240, without 8+3 filenames
	if ( (cnt != 2) || (strlen(Path)>240) ) {
		printf( "Invalid savegame directory '%s' in %s.\n", slotname, Path );
		return NULL;
	}

	ResolveFilePath( Path );
	DIR* ndir = opendir( Path );
	//If we cannot open the Directory
	if (ndir == NULL) {
		return NULL;
	}
	struct dirent* de2 = readdir( ndir ); //Lookup the first entry in the Directory
	if (de2 == NULL) {
		// No first entry!!!
		closedir( ndir );
		return NULL;
	}
	do {
		if (strnicmp( de2->d_name, "PORTRT", 6 ) == 0)
			prtrt++;
	} while (( de2 = readdir( ndir ) ) != NULL);
	closedir( ndir ); //No other files in the directory, close it

	SaveGame* sg = new SaveGame( Path, savegameName, core->GameNameResRef, prtrt, savegameNumber );
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

void SaveGameIterator::PruneQuickSave(const char *folder)
{
	char from[_MAX_PATH];
	char to[_MAX_PATH];

	//storing the quicksave ages in an array
	std::vector<int> myslots;
	for (charlist::iterator m = save_slots.begin();m!=save_slots.end();m++) {
		int tmp = IsQuickSaveSlot(folder, (*m) );
		if (tmp) {
			size_t pos = myslots.size();
			while(pos-- && myslots[pos]>tmp) ;
			myslots.insert(myslots.begin()+pos+1,tmp);
		}
	}
	//now we got an integer array in myslots
	size_t size = myslots.size();

	if (!size) {
		return;
	}

	int n=myslots[size-1];
	size_t hole = GetHole(n);
	size_t i;
	if (hole<size) {
		//prune second path
		FormatQuickSavePath(from, myslots[hole]);
		myslots.erase(myslots.begin()+hole);
		core->DelTree(from, false);
		rmdir(from);
	}
	//shift paths, always do this, because they are aging
	size = myslots.size();
	for(i=size;i--;) {
		FormatQuickSavePath(from, myslots[i]);
		FormatQuickSavePath(to, myslots[i]+1);
		rename(from,to);
	}
}

int SaveGameIterator::CreateSaveGame(int index, const char *slotname, bool mqs)
{
	char Path[_MAX_PATH];
	char FName[12];

	//some of these restrictions might not be needed
	Store * store = core->GetCurrentStore();
	if (store) {
		return 1; //can't save while store is open
	}
	GameControl *gc = core->GetGameControl();
	if (!gc) {
		return -1; //no gamecontrol!!!
	}
	if (gc->GetDialogueFlags()&DF_IN_DIALOG) {
		return 2; //can't save while in dialog?
	}
	//TODO: can't save while in combat
	//TODO: can't save while (party) actors are in helpless states
	//TODO: can't save while AOE spells are in effect
	//TODO: can't save while IF_NOINT is set on an actor

	GetSaveGameCount(); //forcing reload
	if (mqs) {
		assert(index==1);
		PruneQuickSave(slotname);
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
		snprintf( Path, _MAX_PATH, "%09d-%s", index, slotname );
	} else {
		// the existing filename has the original index of the previous save
		// this is usually bad since the gui sends the current one
		SaveGame *save = GetSaveGame(index);
		if (!save) return -1;
		if (save->GetSaveID() != index) {
			// stop gemrb from deleting all our save games
			printf("gemrb's buggy save code is trying to delete slot %d\n", save->GetSaveID());
			printf("that is not the slot %d we were trying to save to, erroring out!\n", index);
			delete save;
			return -1;
		}
		DeleteSaveGame(index);
		snprintf( Path, _MAX_PATH, "%09d-%s", save->GetSaveID(), slotname );
                delete save;
	}
	save_slots.insert( save_slots.end(), strdup( Path ) );
	snprintf( Path, _MAX_PATH, "%s%s", core->SavePath, SaveDir() );

	//if the path exists in different case, don't make it again
	ResolveFilePath( Path );
	mkdir(Path,S_IWRITE|S_IREAD|S_IEXEC);
	chmod(Path,S_IWRITE|S_IREAD|S_IEXEC);
	//keep the first part we already determined existing
	int len = strlen(Path);
	snprintf( Path+len, _MAX_PATH-len, "%s%09d-%s", SPathDelimiter, index, slotname );
	ResolveFilePath( Path );
	//this is required in case the old slot wasn't recognised but still there
	core->DelTree(Path, false);
	mkdir(Path,S_IWRITE|S_IREAD|S_IEXEC);
	chmod(Path,S_IWRITE|S_IREAD|S_IEXEC);
	//save files here

	Game *game = core->GetGame();
	//saving areas to cache currently in memory
	unsigned int mc = (unsigned int) game->GetLoadedMapCount();
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

	ImageMgr *im = (ImageMgr *) core->GetInterface( IE_BMP_CLASS_ID );

	//Create portraits
	for (int i = 0; i < game->GetPartySize( false ); i++) {
		Sprite2D* portrait = core->GetGameControl()->GetPortraitPreview( i );
		if (portrait) {
			snprintf( FName, sizeof(FName), "PORTRT%d", i );
			FileStream outfile;
			outfile.Create( Path, FName, IE_BMP_CLASS_ID );
			im->OpenFromImage( portrait, true );
			im->PutImage( &outfile );
		}
	}

	// Create area preview
	Sprite2D* preview = core->GetGameControl()->GetPreview();
	FileStream outfile;
	outfile.Create( Path, core->GameNameResRef, IE_BMP_CLASS_ID );
	im->OpenFromImage( preview, true );
	im->PutImage( &outfile );

	core->FreeInterface(im);
	loaded = false;
	// Save succesful / Quick-save succesful
	if (index == 1) {
		core->DisplayConstantString(STR_QSAVESUCCEED, 0xbcefbc);
		if (core->GetGameControl()) {
			core->GetGameControl()->SetDisplayText(STR_QSAVESUCCEED, 30);
		}
	} else {
		core->DisplayConstantString(STR_SAVESUCCEED, 0xbcefbc);
		if (core->GetGameControl()) {
			core->GetGameControl()->SetDisplayText(STR_SAVESUCCEED, 30);
		}
	}
	return 0;
}

void SaveGameIterator::DeleteSaveGame(int index)
{
	char* slotname = GetSaveName(index);
	if (!slotname) {
		return;
	}

	char Path[_MAX_PATH];
	snprintf( Path, _MAX_PATH, "%s%s%s%s", core->SavePath, SaveDir(), SPathDelimiter, slotname );
	core->DelTree( Path, false ); //remove all files from folder
	rmdir( Path );

	charlist::iterator i=save_slots.begin();
	while (index--) {
		i++;
	}

	free( (*i));
	save_slots.erase(i);
}
