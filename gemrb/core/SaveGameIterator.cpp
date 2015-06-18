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

#include "SaveGameIterator.h"

#include "iless.h"
#include "strrefs.h"
#include "win32def.h"

#include "DisplayMessage.h"
#include "GameData.h" // For ResourceHolder
#include "ImageMgr.h"
#include "ImageWriter.h"
#include "Interface.h"
#include "PluginMgr.h"
#include "SaveGameMgr.h"
#include "Sprite2D.h"
#include "TableMgr.h"
#include "GUI/GameControl.h"
#include "Scriptable/Actor.h"
#include "System/FileStream.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <cassert>
#include <set>
#include <time.h>

namespace GemRB {

const TypeID SaveGame::ID = { "SaveGame" };

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
		strcpy(Date, "ERROR");
		return;
	}

	int hours = ((int)GameTime)/300;
	int days = hours/24;
	hours -= days*24;
	char *a=NULL,*b=NULL,*c=NULL;

	core->GetTokenDictionary()->SetAtCopy("GAMEDAYS", days);
	if (days) {
		if (days==1) a=core->GetCString(10698);
		else a=core->GetCString(10697);
	}
	core->GetTokenDictionary()->SetAtCopy("HOUR", hours);
	if (hours || !a) {
		if (a) b=core->GetCString(10699);
		if (hours==1) c=core->GetCString(10701);
		else c=core->GetCString(10700);
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
	strlcpy( Prefix, prefix, sizeof( Prefix ) );
	strlcpy( Path, path, sizeof( Path ) );
	strlcpy( Name, name, sizeof( Name ) );
	strlcpy( SlotName, slotname, sizeof( SlotName ) );
	PortraitCount = pCount;
	SaveID = saveID;
	char nPath[_MAX_PATH];
	struct stat my_stat;
	PathJoinExt(nPath, Path, Prefix, "bmp");
	memset(&my_stat,0,sizeof(my_stat));
	if (stat(nPath, &my_stat)) {
		Log(ERROR, "SaveGameIterator", "Stat call failed, using dummy time!");
		strlcpy(Date, "Sun 31 Feb 00:00:01 2099", _MAX_PATH);
	} else {
		strftime(Date, _MAX_PATH, "%c", localtime((time_t*)&my_stat.st_mtime));
	}
	manager.AddSource(Path, Name, PLUGIN_RESOURCE_DIRECTORY);
	GameDate[0] = '\0';
}

SaveGame::~SaveGame()
{
}

Sprite2D* SaveGame::GetPortrait(int index) const
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

Sprite2D* SaveGame::GetPreview() const
{
	ResourceHolder<ImageMgr> im(Prefix, manager, true);
	if (!im)
		return NULL;
	return im->GetSprite2D();
}

DataStream* SaveGame::GetGame() const
{
	return manager.GetResource(Prefix, IE_GAM_CLASS_ID, true);
}

DataStream* SaveGame::GetWmap(int idx) const
{
	return manager.GetResource(core->WorldMapName[idx], IE_WMP_CLASS_ID, true);
}

DataStream* SaveGame::GetSave() const
{
	return manager.GetResource(Prefix, IE_SAV_CLASS_ID, true);
}

const char* SaveGame::GetGameDate() const
{
	if (GameDate[0] == '\0')
		ParseGameDate(GetGame(), GameDate);
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
		Log(ERROR, "SaveGameIterator", "Invalid savegame directory '%s' in %s.",
			slotname, Path);
		return false;
	}

	//The matcher got matched correctly.
	char dtmp[_MAX_PATH];
	PathJoin(dtmp, Path, slotname, NULL);

	char ftmp[_MAX_PATH];
	PathJoinExt(ftmp, dtmp, core->GameNameResRef, "bmp");

	if (access( ftmp, R_OK )) {
		Log(WARNING, "SaveGameIterator", "Ignoring slot %s because of no appropriate preview!", dtmp);
		return false;
	}

	PathJoinExt(ftmp, dtmp, core->WorldMapName[0], "wmp");
	if (access( ftmp, R_OK )) {
		Log(WARNING, "SaveGameIterator", "Ignoring slot %s because of no appropriate worldmap!", dtmp);
		return false;
	}

	/* we might need something here as well
	PathJoinExt(ftmp, dtmp, core->WorldMapName[1], "wmp");
	if (access( ftmp, R_OK )) {
		Log(WARNING, "SaveGameIterator", "Ignoring slot %s because of no appropriate worldmap!", dtmp);
		return false;
	}
	*/

	return true;
}

bool SaveGameIterator::RescanSaveGames()
{
	// delete old entries
	save_slots.clear();

	char Path[_MAX_PATH];
	PathJoin(Path, core->SavePath, SaveDir(), NULL);

	DirectoryIterator dir(Path);
	// create the save game directory at first access
	if (!dir) {
		if (!MakeDirectories(Path)) {
			Log(ERROR, "SaveGameIterator", "Unable to create save game directory '%s'", Path);
			return false;
		}
		dir.Rewind();
	}
	if (!dir) { //If we cannot open the Directory
		return false;
	}

	std::set<char*,iless> slots;
	do {
		const char *name = dir.GetName();
		if (dir.IsDirectory() && IsSaveGameSlot( Path, name )) {
			slots.insert(strdup(name));
		}
	} while (++dir);

	for (std::set<char*,iless>::iterator i = slots.begin(); i != slots.end(); i++) {
		save_slots.push_back(BuildSaveGame(*i));
		free(*i);
	}

	return true;
}

const std::vector<Holder<SaveGame> >& SaveGameIterator::GetSaveGames()
{
	RescanSaveGames();

	return save_slots;
}

Holder<SaveGame> SaveGameIterator::GetSaveGame(const char *name)
{
	RescanSaveGames();

	for (std::vector<Holder<SaveGame> >::iterator i = save_slots.begin(); i != save_slots.end(); i++) {
		if (strcmp(name, (*i)->GetName()) == 0)
			return *i;
	}
	return NULL;
}

Holder<SaveGame> SaveGameIterator::BuildSaveGame(const char *slotname)
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
		Log(WARNING, "SaveGame" "Invalid savegame directory '%s' in %s.", slotname, Path );
		return NULL;
	}

	DirectoryIterator dir(Path);
	if (!dir) {
		return NULL;
	}
	do {
		if (strnicmp( dir.GetName(), "PORTRT", 6 ) == 0)
			prtrt++;
	} while (++dir);

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
		int errnum = rename(from, to);
		if (errnum) {
			error("SaveGameIterator", "Rename error %d when pruning quicksaves!\n", errnum);
		}
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

	gamedata->SaveAllStores();

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

	PluginHolder<ImageWriter> im(PLUGIN_IMAGE_WRITER_BMP);
	if (!im) {
		Log(ERROR, "SaveGameIterator", "Couldn't create the BMPWriter!");
		return false;
	}

	//Create portraits
	for (int i = 0; i < game->GetPartySize( false ); i++) {
		Actor *actor = game->GetPC( i, false );
		Sprite2D* portrait = actor->CopyPortrait(true);

		if (portrait) {
			char FName[_MAX_PATH];
			snprintf( FName, sizeof(FName), "PORTRT%d", i );
			FileStream outfile;
			outfile.Create( Path, FName, IE_BMP_CLASS_ID );
			// NOTE: we save the true portrait size, even tho the preview buttons arent (always) the same
			// we do this because: 1. the GUI should be able to use whatever size it wants
			// and 2. its more appropriate to have a flag on the buttons to do the scaling/cropping
			im->PutImage( &outfile, portrait );
			Sprite2D::FreeSprite(portrait);
		}
	}

	// Create area preview
	// FIXME: the preview shoudl be passed in by the caller!
	/*
	Sprite2D* preview = NULL;
	FileStream outfile;
	outfile.Create( Path, core->GameNameResRef, IE_BMP_CLASS_ID );
	im->PutImage( &outfile, preview );
	*/

	return true;
}

static int CanSave()
{
	//some of these restrictions might not be needed
	Store * store = core->GetCurrentStore();
	if (store) {
		displaymsg->DisplayConstantString(STR_CANTSAVESTORE, DMC_BG2XPGREEN);
		return 1; //can't save while store is open
	}
	GameControl *gc = core->GetGameControl();
	if (!gc) {
		displaymsg->DisplayConstantString(STR_CANTSAVE, DMC_BG2XPGREEN);
		return -1; //no gamecontrol!!!
	}
	if (gc->GetDialogueFlags()&DF_IN_DIALOG) {
		displaymsg->DisplayConstantString(STR_CANTSAVEDIALOG, DMC_BG2XPGREEN);
		return 2; //can't save while in dialog
	}

	//TODO: can't save while in combat
	Game *game = core->GetGame();
	if (!game) {
		displaymsg->DisplayConstantString(STR_CANTSAVE, DMC_BG2XPGREEN);
		return -1;
	}
	if (game->CombatCounter) {
		displaymsg->DisplayConstantString(STR_CANTSAVECOMBAT, DMC_BG2XPGREEN);
		return 3;
	}

	Map *map = game->GetCurrentArea();
	if (!map) {		
		displaymsg->DisplayConstantString(STR_CANTSAVE, DMC_BG2XPGREEN);
		return -1;
	}

	if (map->AreaFlags&AF_NOSAVE) {
		//cannot save in area
		displaymsg->DisplayConstantString(STR_CANTSAVEMONS, DMC_BG2XPGREEN);
		return 4;
	}

	int i = game->GetPartySize(true);
	while(i--) {
		Actor *actor = game->GetPC(i, true);
		//TODO: can't save while (party) actors are in helpless states
		if (actor->GetStat(IE_STATE_ID) & STATE_NOSAVE) {
			//some actor is in nosave state
			displaymsg->DisplayConstantString(STR_CANTSAVENOCTRL, DMC_BG2XPGREEN);
			return 5;
		}
		if (actor->GetCurrentArea()!=map) {
			//scattered
			displaymsg->DisplayConstantString(STR_CANTSAVE, DMC_BG2XPGREEN);
			return 6;
		}

                if (map->AnyEnemyNearPoint(actor->Pos)) {
                        displaymsg->DisplayConstantString( STR_CANTSAVEMONS, DMC_BG2XPGREEN );
                        return 7;
                }

	}

	//TODO: can't save while AOE spells are in effect -> CANTSAVE
	//TODO: can't save while IF_NOINT is set on any actor -> CANTSAVEDIALOG2 (dialog about to start)
	//TODO: can't save  during a rest, chapter information or movie -> CANTSAVEMOVIE

	return 0;
}

static bool CreateSavePath(char *Path, int index, const char *slotname) WARN_UNUSED;
static bool CreateSavePath(char *Path, int index, const char *slotname)
{
	PathJoin( Path, core->SavePath, SaveDir(), NULL );

	//if the path exists in different case, don't make it again
	if (!MakeDirectory(Path)) {
		Log(ERROR, "SaveGameIterator", "Unable to create save game directory '%s'", Path);
		return false;
	}
	//keep the first part we already determined existing

	char dir[_MAX_PATH];
	snprintf( dir, _MAX_PATH, "%09d-%s", index, slotname );
	PathJoin(Path, Path, dir, NULL);
	//this is required in case the old slot wasn't recognised but still there
	core->DelTree(Path, false);
	if (!MakeDirectory(Path)) {
		Log(ERROR, "SaveGameIterator", "Unable to create save game directory '%s'", Path);
		return false;
	}
	return true;
}

int SaveGameIterator::CreateSaveGame(int index, bool mqs)
{
	AutoTable tab("savegame");
	const char *slotname = NULL;
	int qsave = 0;

	if (tab) {
		slotname = tab->QueryField(index);
		qsave = atoi(tab->QueryField(index, 1));
	}

	if (mqs) {
		assert(qsave);
		PruneQuickSave(slotname);
	}

	if (int cansave = CanSave())
		return cansave;

	//if index is not an existing savegame, we create a unique slotname
	for (size_t i = 0; i < save_slots.size(); ++i) {
		Holder<SaveGame> save = save_slots[i];
		if (save->GetSaveID() == index) {
			DeleteSaveGame(save);
			break;
		}
	}
	char Path[_MAX_PATH];
	GameControl *gc = core->GetGameControl();

	if (!CreateSavePath(Path, index, slotname)) {
		displaymsg->DisplayConstantString(STR_CANTSAVE, DMC_BG2XPGREEN);
		if (gc) {
			gc->SetDisplayText(STR_CANTSAVE, 30);
		}
		return -1;
	}

	if (!DoSaveGame(Path)) {
		displaymsg->DisplayConstantString(STR_CANTSAVE, DMC_BG2XPGREEN);
		if (gc) {
			gc->SetDisplayText(STR_CANTSAVE, 30);
		}
		return -1;
	}

	// Save succesful / Quick-save succesful
	if (qsave) {
		displaymsg->DisplayConstantString(STR_QSAVESUCCEED, DMC_BG2XPGREEN);
		if (gc) {
			gc->SetDisplayText(STR_QSAVESUCCEED, 30);
		}
	} else {
		displaymsg->DisplayConstantString(STR_SAVESUCCEED, DMC_BG2XPGREEN);
		if (gc) {
			gc->SetDisplayText(STR_SAVESUCCEED, 30);
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

	GameControl *gc = core->GetGameControl();
	int index;

	if (save) {
		index = save->GetSaveID();

		DeleteSaveGame(save);
		save.release();
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
	if (!CreateSavePath(Path, index, slotname)) {
		displaymsg->DisplayConstantString(STR_CANTSAVE, DMC_BG2XPGREEN);
		if (gc) {
			gc->SetDisplayText(STR_CANTSAVE, 30);
		}
		return -1;
	}

	if (!DoSaveGame(Path)) {
		displaymsg->DisplayConstantString(STR_CANTSAVE, DMC_BG2XPGREEN);
		if (gc) {
			gc->SetDisplayText(STR_CANTSAVE, 30);
		}
		return -1;
	}

	// Save succesful
	displaymsg->DisplayConstantString(STR_SAVESUCCEED, DMC_BG2XPGREEN);
	if (gc) {
		gc->SetDisplayText(STR_SAVESUCCEED, 30);
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

}
