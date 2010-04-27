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

#include "win32def.h"
#include "strrefs.h"
#include "SaveGameIterator.h"
#include "Interface.h"
#include "SaveGameMgr.h"
#include "GameControl.h"
#include "Video.h"
#include "ImageWriter.h"
#include "ImageMgr.h"
#include "GameData.h" // For ResourceHolder
#include <set>
#include <algorithm>
#include <iterator>
#include <cassert>

TypeID SaveGame::ID = { "SaveGame" };

/** Extract date from save game ds into Date. */
static void ParseGameDate(DataStream *ds, char *Date)
{
	Date[0] = '\0';

	char Signature[8];
	ieDword GameTime;
	ds->Read(Signature, 8);
	ds->ReadDword(&GameTime);
	delete ds;
	if (memcmp(Signature,"GAME",4) ) {
		return;
	}

	int hours = ((int)GameTime)/300;
	int days = hours/24;
	hours -= days*24;
	char *a=NULL,*b=NULL,*c=NULL;

	core->GetTokenDictionary()->SetAtCopy("GAMEDAYS", days);
	if (days) {
		if (days==1) a=core->GetString(10698);
		else a=core->GetString(10697);
	}
	core->GetTokenDictionary()->SetAtCopy("HOUR", hours);
	if (hours || !a) {
		if (a) b=core->GetString(10699);
		if (hours==1) c=core->GetString(10701);
		else c=core->GetString(10700);
	}
	if (b) {
		strcat(Date, a);
		strcat(Date, " ");
		strcat(Date, b);
		strcat(Date, " ");
		if (c)
			strcat(Date, c);
	} else {
		if (a)
			strcat(Date, a);
		if (c)
			strcat(Date, c);
	}
	core->FreeString(a);
	core->FreeString(b);
	core->FreeString(c);
}

SaveGame::SaveGame(const char* path, const char* name, const char* prefix, const char* slotname, int pCount, int saveID)
{
	strncpy( Prefix, prefix, sizeof( Prefix ) );
	strncpy( Path, path, sizeof( Path ) );
	strncpy( Name, name, sizeof( Name ) );
	strncpy( SlotName, slotname, sizeof( SlotName ) );
	PortraitCount = pCount;
	SaveID = saveID;
	char nPath[_MAX_PATH];
	struct stat my_stat;
	PathJoinExt(nPath, Path, Prefix, "bmp");
	memset(&my_stat,0,sizeof(my_stat));
	stat( nPath, &my_stat );
	strftime( Date, _MAX_PATH, "%c", localtime( &my_stat.st_mtime ) );
	manager.AddSource(Path, Name, PLUGIN_RESOURCE_DIRECTORY);
	ParseGameDate(GetGame(), GameDate);
}

SaveGame::~SaveGame()
{
}

Sprite2D* SaveGame::GetPortrait(int index)
{
	if (index > PortraitCount) {
		return NULL;
	}
	char nPath[_MAX_PATH];
	sprintf( nPath, "PORTRT%d", index );
	ResourceHolder<ImageMgr> im(nPath, manager, true);
	if (!im)
		return NULL;
	return im->GetSprite2D();
}

Sprite2D* SaveGame::GetPreview()
{
	ResourceHolder<ImageMgr> im(Prefix, manager, true);
	if (!im)
		return NULL;
	return im->GetSprite2D();
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

const char* SaveGame::GetGameDate()
{
	return GameDate;
}

SaveGameIterator::SaveGameIterator(void)
{
}

SaveGameIterator::~SaveGameIterator(void)
{
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
	PathJoin(dtmp, Path, slotname, NULL);

	struct stat fst;
	if (stat( dtmp, &fst ))
		return false;

	if (! S_ISDIR( fst.st_mode ))
		return false;

	char ftmp[_MAX_PATH];
	PathJoinExt(ftmp, dtmp, core->GameNameResRef, "bmp");

	if (access( ftmp, R_OK )) {
		printMessage("SaveGameIterator"," ",YELLOW);
		printf("Ignoring slot %s because of no appropriate preview!\n", dtmp);
		return false;
	}

	PathJoinExt(ftmp, dtmp, core->WorldMapName, "wmp");
	if (access( ftmp, R_OK )) {
		printMessage("SaveGameIterator"," ",YELLOW);
		printf("Ignoring slot %s because of no appropriate worldmap!\n", dtmp);
		return false;
	}

	return true;
}

struct iless {
	bool operator () (const char *lhs, const char* rhs)
	{
		return stricmp(lhs, rhs) < 0;
	}
};

bool SaveGameIterator::RescanSaveGames()
{
	// delete old entries
	save_slots.clear();

	char Path[_MAX_PATH];
	PathJoin(Path, core->SavePath, SaveDir(), NULL);

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

	std::set<char*,iless> slots;
	do {
		if (IsSaveGameSlot( Path, de->d_name )) {
			slots.insert(strdup(de->d_name));
		}
	} while (( de = readdir( dir ) ) != NULL);
	closedir( dir ); //No other files in the directory, close it

	std::transform(slots.begin(), slots.end(), back_inserter(save_slots), GetSaveGame);
	return true;
}

const std::vector<Holder<SaveGame> >& SaveGameIterator::GetSaveGames()
{
	RescanSaveGames();

	return save_slots;
}

Holder<SaveGame> SaveGameIterator::GetSaveGame(const char *slotname)
{
	if (!slotname) {
		return NULL;
	}

	int prtrt = 0;
	char Path[_MAX_PATH];
	//lets leave space for the filenames
	PathJoin(Path, core->SavePath, SaveDir(), slotname, NULL);

	char savegameName[_MAX_PATH]={0};
	int savegameNumber = 0;

	int cnt = sscanf( slotname, SAVEGAME_DIRECTORY_MATCHER, &savegameNumber, savegameName );
	//maximum pathlength == 240, without 8+3 filenames
	if ( (cnt != 2) || (strlen(Path)>240) ) {
		printf( "Invalid savegame directory '%s' in %s.\n", slotname, Path );
		return NULL;
	}

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

	SaveGame* sg = new SaveGame( Path, savegameName, core->GameNameResRef, slotname, prtrt, savegameNumber );
	return sg;
}

void SaveGameIterator::PruneQuickSave(const char *folder)
{
	char from[_MAX_PATH];
	char to[_MAX_PATH];

	//storing the quicksave ages in an array
	std::vector<int> myslots;
	for (charlist::iterator m = save_slots.begin();m!=save_slots.end();m++) {
		int tmp = IsQuickSaveSlot(folder, (*m)->GetSlotName() );
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

/** Save game to given directory */
static bool DoSaveGame(const char *Path)
{
	Game *game = core->GetGame();
	//saving areas to cache currently in memory
	unsigned int mc = (unsigned int) game->GetLoadedMapCount();
	while (mc--) {
		Map *map = game->GetMap(mc);
		if (core->SwapoutArea(map)) {
			return false;
		}
	}

	//compress files in cache named: .STO and .ARE
	//no .CRE would be saved in cache
	if (core->CompressSave(Path)) {
		return false;
	}

	//Create .gam file from Game() object
	if (core->WriteGame(Path)) {
		return false;
	}

	//Create .wmp file from WorldMap() object
	if (core->WriteWorldMap(Path)) {
		return false;
	}

	ImageWriter *im = (ImageWriter *) core->GetInterface( PLUGIN_IMAGE_WRITER_BMP );
	if (!im) {
		printMessage( "SaveGameIterator", "Couldn't create the BMPWriter!\n", LIGHT_RED );
		return false;
	}

	//Create portraits
	for (int i = 0; i < game->GetPartySize( false ); i++) {
		Sprite2D* portrait = core->GetGameControl()->GetPortraitPreview( i );
		if (portrait) {
			char FName[_MAX_PATH];
			snprintf( FName, sizeof(FName), "PORTRT%d", i );
			FileStream outfile;
			outfile.Create( Path, FName, IE_BMP_CLASS_ID );
			im->PutImage( &outfile, portrait );
		}
	}

	// Create area preview
	Sprite2D* preview = core->GetGameControl()->GetPreview();
	FileStream outfile;
	outfile.Create( Path, core->GameNameResRef, IE_BMP_CLASS_ID );
	im->PutImage( &outfile, preview );

	im->release();
	return true;
}

int CanSave()
{
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
	return 0;
}

static void CreateSavePath(char *Path, int index, const char *slotname)
{
	PathJoin( Path, core->SavePath, SaveDir(), NULL );

	//if the path exists in different case, don't make it again
	mkdir(Path,S_IWRITE|S_IREAD|S_IEXEC);
	chmod(Path,S_IWRITE|S_IREAD|S_IEXEC);
	//keep the first part we already determined existing

	char dir[_MAX_PATH];
	snprintf( dir, _MAX_PATH, "%09d-%s", index, slotname );
	snprintf( dir, _MAX_PATH, "%09d-%s", (int)index, slotname );
	PathJoin(Path, Path, dir, NULL);
	//this is required in case the old slot wasn't recognised but still there
	core->DelTree(Path, false);
	mkdir(Path,S_IWRITE|S_IREAD|S_IEXEC);
	chmod(Path,S_IWRITE|S_IREAD|S_IEXEC);
}

int SaveGameIterator::CreateSaveGame(int index, bool mqs)
{
	AutoTable tab("savegame");
	const char *slotname = NULL;
	if (tab) {
		slotname = tab->QueryField(index);
	}

	if (mqs) {
		assert(index==1);
		PruneQuickSave(slotname);
	}

	//if index is not an existing savegame, we create a unique slotname
	for (size_t i = 0; i < save_slots.size(); ++i) {
		Holder<SaveGame> save = save_slots[i];
		if (save->GetSaveID() == index) {
			DeleteSaveGame(save);
			break;
		}
	}
	char Path[_MAX_PATH];
	CreateSavePath(Path, index, slotname);

	if (!DoSaveGame(Path)) {
		return -1;
	}

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

int SaveGameIterator::CreateSaveGame(Holder<SaveGame> save, const char *slotname)
{
	if (!slotname) {
		return -1;
	}

	if (int cansave = CanSave())
		return cansave;

	int index;
	if (save) {
		index = save->GetSaveID();

		DeleteSaveGame(save);
		save->release();
	} else {
		//leave space for autosaves
		//probably the hardcoded slot names should be read by this object
		//in that case 7 == size of hardcoded slot names array (savegame.2da)
		index = 7;
		for (size_t i = 0; i < save_slots.size(); ++i) {
			Holder<SaveGame> save = save_slots[i];
			if (save->GetSaveID() >= index) {
				index = save->GetSaveID() + 1;
			}
		}
	}

	char Path[_MAX_PATH];
	CreateSavePath(Path, index, slotname);

	if (!DoSaveGame(Path)) {
		return -1;
	}

	// Save succesful
	core->DisplayConstantString(STR_SAVESUCCEED, 0xbcefbc);
	if (core->GetGameControl()) {
		core->GetGameControl()->SetDisplayText(STR_SAVESUCCEED, 30);
	}
	return 0;
}

void SaveGameIterator::DeleteSaveGame(Holder<SaveGame> game)
{
       if (!game) {
               return;
       }

       core->DelTree( game->GetPath(), false ); //remove all files from folder
       rmdir( game->GetPath() );
}
