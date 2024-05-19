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

#include "strrefs.h"

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
#include "GUI/WindowManager.h"
#include "Scriptable/Actor.h"
#include "Streams/FileStream.h"
#include "System/VFS.h"
#include "fmt/chrono.h"

#include <cassert>
#include <set>
#include <ctime>

#ifdef VITA
#include <dirent.h>
#endif

namespace GemRB {

const TypeID SaveGame::ID = { "SaveGame" };

/** Extract date from save game ds into Date. */
static std::string ParseGameDate(DataStream *ds)
{
	char Signature[8];
	ieDword GameTime;
	ds->Read(Signature, 8);
	ds->ReadDword(GameTime);
	if (memcmp(Signature, "GAME", 4) != 0) {
		return "ERROR";
	}
	// bg1 displays 7 hours less in game, sigh
	if (core->HasFeature(GFFlags::BREAKABLE_WEAPONS)) {
		GameTime -= 2100;

		// also read the Chapter global, since bg1 displays it
		// we don't want to parse the whole dictionary, so just estimate it by
		// grabbing the chapter from the last journal entry
		ieDword journalCount;
		ieByte chapter = 0;
		ds->Seek(0x4c, GEM_STREAM_START);
		ds->ReadDword(journalCount);
		if (journalCount) {
			ieDword journalOffset;
			ds->ReadDword(journalOffset);
			// seek to the last 12 byte entry and skip the text and time fields
			ds->Seek(journalOffset + (journalCount - 1) * 12 + 8, GEM_STREAM_START);
			ds->Read(&chapter, 1);
		}

		SetTokenAsString("CHAPTER0", chapter);
	}
	delete ds;

	int hours = ((int)GameTime)/core->Time.hour_sec;
	int days = hours/24;
	hours -= days*24;
	std::string a;
	std::string b;
	std::string c;

	// pst has a nice single string for everything 41277 (individual ones lack tokens)
	SetTokenAsString("GAMEDAYS", days);
	SetTokenAsString("HOUR", hours);
	ieStrRef dayMsg = DisplayMessage::GetStringReference(HCStrings::Day);
	ieStrRef daysMsg = DisplayMessage::GetStringReference(HCStrings::Days);
	if (dayMsg == daysMsg) {
		return core->GetMBString(ieStrRef::DATE2);
	}

	if (days) {
		if (days == 1) {
			a = core->GetMBString(dayMsg, STRING_FLAGS::NONE);
		} else {
			a = core->GetMBString(daysMsg, STRING_FLAGS::NONE);
		}
	}
	if (hours || a.empty()) {
		if (!a.empty()) b=core->GetMBString(ieStrRef::DATE1); // and
		if (hours == 1) {
			c = core->GetMBString(DisplayMessage::GetStringReference(HCStrings::Hour), STRING_FLAGS::NONE);
		} else {
			c = core->GetMBString(DisplayMessage::GetStringReference(HCStrings::Hours), STRING_FLAGS::NONE);
		}
	}
	
	if (!b.empty()) {
		return a + " " + b + " " + c;
	} else {
		return a + c;
	}
}

SaveGame::SaveGame(path_t path, const path_t& name, const ResRef& prefix, std::string slotname, int pCount, int saveID)
: Path(std::move(path)), Prefix(prefix), SlotName(std::move(slotname))
{
	static const auto DATE_FMT = FMT_STRING("{:%a %Od %b %T %EY}");
	PortraitCount = pCount;
	SaveID = saveID;
	struct stat my_stat;
	path_t nPath = PathJoinExt(Path, Prefix, "bmp");
	memset(&my_stat, 0, sizeof(my_stat));
	if (stat(nPath.c_str(), &my_stat)) {
		Log(ERROR, "SaveGameIterator", "Stat call failed, using dummy time!");
		Date = fmt::format(DATE_FMT, fmt::localtime(0));
	} else {
		Date = fmt::format(DATE_FMT, fmt::localtime(my_stat.st_mtime));
	}
	manager.AddSource(Path, name, PLUGIN_RESOURCE_DIRECTORY);
	Name = StringFromUtf8(name);
}

Holder<Sprite2D> SaveGame::GetPortrait(int index) const
{
	if (index > PortraitCount) {
		return NULL;
	}

	path_t nPath = fmt::format("PORTRT{}", index);
	ResourceHolder<ImageMgr> im = manager.GetResourceHolder<ImageMgr>(nPath, true);
	if (!im)
		return NULL;
	return im->GetSprite2D();
}

Holder<Sprite2D> SaveGame::GetPreview() const
{
	ResourceHolder<ImageMgr> im = manager.GetResourceHolder<ImageMgr>(Prefix, true);
	if (!im)
		return NULL;
	return im->GetSprite2D();
}

DataStream* SaveGame::GetGame() const
{
	return manager.GetResourceStream(Prefix, IE_GAM_CLASS_ID, true);
}

DataStream* SaveGame::GetWmap(int idx) const
{
	return manager.GetResourceStream(core->WorldMapName[idx], IE_WMP_CLASS_ID, true);
}

DataStream* SaveGame::GetSave() const
{
	return manager.GetResourceStream(Prefix, IE_SAV_CLASS_ID, true);
}

const std::string& SaveGame::GetGameDate() const
{
	if (GameDate.empty())
		GameDate = ParseGameDate(GetGame());
	return GameDate;
}

// mission pack save dir or the main one?
static path_t SaveDir()
{
	return MBStringFromString(core->GetToken("SaveDir", u"save"));
}

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
static int IsQuickSaveSlot(StringView match, StringView slotname)
{
	char savegameName[255];
	int savegameNumber = 0;
	int cnt = sscanf(slotname.c_str(), SAVEGAME_DIRECTORY_MATCHER, &savegameNumber, savegameName);
	if (cnt != 2) {
		return 0;
	}
	if (strnicmp(savegameName, match.c_str(), sizeof(savegameName)) != 0)
	{
		return 0;
	}
	return savegameNumber;
}
/*
 * Return true if directory Path/slotname is a potential save game
 * slot, otherwise return false.
 */
static bool IsSaveGameSlot(const path_t& Path, const path_t& slotname)
{
	char savegameName[255];
	int savegameNumber = 0;

	if (slotname[0] == '.')
		return false;

	int cnt = sscanf(slotname.c_str(), SAVEGAME_DIRECTORY_MATCHER, &savegameNumber, savegameName);
	if (cnt != 2) {
		//The matcher didn't match: either this is not a valid dir
		//or the SAVEGAME_DIRECTORY_MATCHER needs updating.
		Log(ERROR, "SaveGameIterator", "Invalid savegame directory '{}' in {}.",
			slotname, Path);
		return false;
	}

	//The matcher got matched correctly.
	path_t dtmp = PathJoin(Path, slotname);

	path_t ftmp = PathJoinExt(dtmp, core->GameNameResRef, "bmp");

	if (!FileExists(ftmp)) {
		Log(WARNING, "SaveGameIterator", "Ignoring slot {} because of no appropriate preview!", dtmp);
		return false;
	}

	// no worldmaps in saves in ees
	if (core->HasFeature(GFFlags::HAS_EE_EFFECTS)) return true;

	ftmp = PathJoinExt(dtmp, core->WorldMapName[0], "wmp");
	if (!FileExists(ftmp)) {
		return false;
	}

	if (core->WorldMapName[1]) {
		ftmp = PathJoinExt(dtmp, core->WorldMapName[1], "wmp");
		if (!FileExists(ftmp)) {
			Log(WARNING, "SaveGameIterator", "Ignoring slot {} because of no appropriate second worldmap!", dtmp);
			return false;
		}
	}

	return true;
}

bool SaveGameIterator::RescanSaveGames()
{
	// delete old entries
	save_slots.clear();

	path_t Path = PathJoin(core->config.SavePath, SaveDir());

	DirectoryIterator dir(Path);
	// create the save game directory at first access
	if (!dir) {
		if (!MakeDirectories(Path)) {
			Log(ERROR, "SaveGameIterator", "Unable to create save game directory '{}'", Path);
			return false;
		}
		dir.Rewind();
	}
	if (!dir) { //If we cannot open the Directory
		return false;
	}

	std::set<std::string> slots;
	dir.SetFlags(DirectoryIterator::Directories);
	do {
		const path_t& name = dir.GetName();
		if (IsSaveGameSlot(Path, name)) {
			slots.emplace(name);
		}
	} while (++dir);

	for (const auto& slot : slots) {
		auto saveGame = BuildSaveGame(slot);
		if (saveGame) {
			save_slots.push_back(saveGame);
		}
	}

	return true;
}

const std::vector<Holder<SaveGame> >& SaveGameIterator::GetSaveGames()
{
	RescanSaveGames();

	return save_slots;
}

Holder<SaveGame> SaveGameIterator::GetSaveGame(const String& name)
{
	RescanSaveGames();

	for (const auto& saveSlot : save_slots) {
		if (saveSlot->GetName() == name)
			return saveSlot;
	}
	return NULL;
}

Holder<SaveGame> SaveGameIterator::BuildSaveGame(std::string slotname)
{
	int prtrt = 0;
	//lets leave space for the filenames
	path_t Path = PathJoin(core->config.SavePath, SaveDir(), slotname);

	char savegameName[255]={0};
	int savegameNumber = 0;

	sscanf(slotname.c_str(), SAVEGAME_DIRECTORY_MATCHER, &savegameNumber, savegameName);

	DirectoryIterator dir(Path);
	if (!dir) {
		return NULL;
	}
	do {
		if (strnicmp(dir.GetName().c_str(), "PORTRT", 6) == 0)
			prtrt++;
	} while (++dir);

	return MakeHolder<SaveGame>(Path, savegameName, core->GameNameResRef, std::move(slotname), prtrt, savegameNumber);
}

void SaveGameIterator::PruneQuickSave(StringView folder) const
{
	auto FormatQuickSavePath = [folder](int i)
	{
		return fmt::format(FMT_STRING("{}{}{}{:09d}-{}"), core->config.SavePath, SaveDir(), SPathDelimiter, i, folder);
	};

	//storing the quicksave ages in an array
	std::vector<int> myslots;
	for (const auto& saveSlot : save_slots) {
		int tmp = IsQuickSaveSlot(folder, saveSlot->GetSlotName());
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
	if (hole<size) {
		//prune second path
		std::string from = FormatQuickSavePath(myslots[hole]);
		myslots.erase(myslots.begin()+hole);
		core->DelTree(from, false);
		rmdir(from.c_str());
	}
	//shift paths, always do this, because they are aging
	size = myslots.size();
	for (size_t i = size; i > 0; i--) {
		std::string from = FormatQuickSavePath(myslots[i]);
		std::string to = FormatQuickSavePath(myslots[i]+1);
		int errnum = rename(from.c_str(), to.c_str());
		if (errnum) {
			error("SaveGameIterator", "Rename error {} when pruning quicksaves!", errnum);
		}
	}
}

/** Save game to given directory */
static bool DoSaveGame(const path_t& Path, bool overrideRunning)
{
	const Game *game = core->GetGame();
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
	if (core->CompressSave(Path, overrideRunning)) {
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

	PluginHolder<ImageWriter> im = MakePluginHolder<ImageWriter>(PLUGIN_IMAGE_WRITER_BMP);
	if (!im) {
		Log(ERROR, "SaveGameIterator", "Couldn't create the BMPWriter!");
		return false;
	}

	//Create portraits
	for (int i = 0; i < game->GetPartySize( false ); i++) {
		const Actor *actor = game->GetPC( i, false );
		Holder<Sprite2D> portrait = actor->CopyPortrait(true);

		if (portrait) {
			path_t fname = fmt::format("PORTRT{}", i);
			FileStream outfile;
			outfile.Create(Path, fname, IE_BMP_CLASS_ID);
			// NOTE: we save the true portrait size, even tho the preview buttons arent (always) the same
			// we do this because: 1. the GUI should be able to use whatever size it wants
			// and 2. its more appropriate to have a flag on the buttons to do the scaling/cropping
			im->PutImage(&outfile, portrait);
		}
	}

	// Create area preview
	// FIXME: the preview should be passed in by the caller!

	WindowManager* wm = core->GetWindowManager();
	Holder<Sprite2D> preview = wm->GetScreenshot(wm->GetGameWindow());

	// scale down to get more of the screen and reduce the size
	preview = VideoDriver->SpriteScaleDown(preview, 5);
	FileStream outfile;
	outfile.Create( Path, core->GameNameResRef.c_str(), IE_BMP_CLASS_ID );
	im->PutImage( &outfile, preview );

	return true;
}

static EffectRef fx_disable_rest_ref = { "DisableRest", -1 };
static int CanSave()
{
	//some of these restrictions might not be needed
	// NOTE: can't save  during a rest, chapter information or movie (ref CantSaveMovie)
	// is handled automatically, but without a message
	if (core->InCutSceneMode()) {
		displaymsg->DisplayMsgCentered(HCStrings::CantSave, FT_ANY, GUIColors::XPCHANGE);
		return 1;
	}

	const Store *store = core->GetCurrentStore();
	if (store) {
		displaymsg->DisplayMsgCentered(HCStrings::CantSaveStore, FT_ANY, GUIColors::XPCHANGE);
		return 1; //can't save while store is open
	}
	const GameControl *gc = core->GetGameControl();
	if (!gc) {
		displaymsg->DisplayMsgCentered(HCStrings::CantSave, FT_ANY, GUIColors::XPCHANGE);
		return -1; //no gamecontrol!!!
	}
	if (gc->InDialog()) {
		displaymsg->DisplayMsgCentered(HCStrings::CantSaveDialog, FT_ANY, GUIColors::XPCHANGE);
		return 2; //can't save while in dialog
	}

	const Game *game = core->GetGame();
	if (!game) {
		displaymsg->DisplayMsgCentered(HCStrings::CantSave, FT_ANY, GUIColors::XPCHANGE);
		return -1;
	}
	if (game->CombatCounter) {
		displaymsg->DisplayMsgCentered(HCStrings::CantSaveCombat, FT_ANY, GUIColors::XPCHANGE);
		return 3;
	}

	const Map *map = game->GetCurrentArea();
	if (!map) {
		displaymsg->DisplayMsgCentered(HCStrings::CantSave, FT_ANY, GUIColors::XPCHANGE);
		return -1;
	}

	proIterator pIter;
	map->GetProjectileCount(pIter);
	if (map->GetNextTrap(pIter, 1)) {
		// can't save while AOE spells are in effect
		displaymsg->DisplayMsgCentered(HCStrings::CantSave, FT_ANY, GUIColors::XPCHANGE);
		return 10;
	}

	if (map->AreaFlags&AF_NOSAVE) {
		//cannot save in area
		displaymsg->DisplayMsgCentered(HCStrings::CantSave, FT_ANY, GUIColors::XPCHANGE);
		return 4;
	}

	int i = game->GetPartySize(true);
	while(i--) {
		const Actor *actor = game->GetPC(i, true);
		// can't save while (party) actors are in helpless or dead states
		// STATE_NOSAVE tracks actors not to be stored in GAM, not game saveability
		if (actor->GetStat(IE_STATE_ID) & (STATE_NOSAVE|STATE_MINDLESS)) {
			//some actor is in nosave state
			displaymsg->DisplayMsgCentered(HCStrings::CantSaveNoCtrl, FT_ANY, GUIColors::XPCHANGE);
			return 5;
		}
		if (actor->GetCurrentArea()!=map) {
			//scattered
			displaymsg->DisplayMsgCentered(HCStrings::CantSave, FT_ANY, GUIColors::XPCHANGE);
			return 6;
		}
		if (map->AnyEnemyNearPoint(actor->Pos)) {
			displaymsg->DisplayMsgCentered(HCStrings::CantSaveMonsters, FT_ANY, GUIColors::XPCHANGE);
			return 7;
		}

		// check the EE no resting/saving opcode
		const Effect* fx = actor->fxqueue.HasEffect(fx_disable_rest_ref);
		if (fx && fx->Parameter2 != 0) {
			displaymsg->DisplayString(ieStrRef(fx->Parameter1), GUIColors::XPCHANGE, STRING_FLAGS::SOUND);
			return 9;
		}
	}

	Point pc1 =  game->GetPC(0, true)->Pos;
	std::vector<Actor *> nearActors = map->GetAllActorsInRadius(pc1, GA_NO_DEAD|GA_NO_UNSCHEDULED, 15);
	for (const auto& neighbour : nearActors) {
		if (neighbour->GetInternalFlag() & IF_NOINT) {
			// dialog about to start or similar
			displaymsg->DisplayMsgCentered(HCStrings::CantSaveDialog2, FT_ANY, GUIColors::XPCHANGE);
			return 8;
		}
	}

	return 0;
}

static bool CreateSavePath(path_t& path, int index, StringView slotname)
{
	path = PathJoin(core->config.SavePath, SaveDir());

	//if the path exists in different case, don't make it again
	if (!MakeDirectory(path)) {
		Log(ERROR, "SaveGameIterator", "Unable to create save game directory '{}'", path);
		return false;
	}
	//keep the first part we already determined existing

	path_t dir = fmt::format("{:09d}-{}", index, slotname);
	path = PathJoin(path, dir);
	//this is required in case the old slot wasn't recognised but still there
	core->DelTree(path, false);
	if (!MakeDirectory(path)) {
		Log(ERROR, "SaveGameIterator", "Unable to create save game directory '{}'", path);
		return false;
	}
	return true;
}

int SaveGameIterator::CreateSaveGame(int index, bool mqs) const
{
	AutoTable tab = gamedata->LoadTable("savegame");
	StringView slotname;
	int qsave = 0;

	if (tab) {
		slotname = tab->QueryField(index, 0);
		qsave = tab->QueryFieldSigned<int>(index, 1);
	}

	if (mqs) {
		assert(qsave);
		PruneQuickSave(slotname);
	}

	if (int cansave = CanSave())
		return cansave;

	bool overrideRunning = false;
	//if index is not an existing savegame, we create a unique slotname
	for (const auto& save : save_slots) {
		if (save->GetSaveID() != index) continue;

		if (core->saveGameAREExtractor.isRunningSaveGame(*save)) {
			overrideRunning = true;
			if (core->saveGameAREExtractor.createCacheBlob() == GEM_ERROR) {
				return GEM_ERROR;
			}
		}

		DeleteSaveGame(save);
		break;
	}
	path_t Path;
	if (!CreateSavePath(Path, index, slotname)) {
		displaymsg->DisplayMsgCentered(HCStrings::CantSave, FT_ANY, GUIColors::XPCHANGE);
		return GEM_ERROR;
	}

	if (!DoSaveGame(Path.c_str(), overrideRunning)) {
		displaymsg->DisplayMsgCentered(HCStrings::CantSave, FT_ANY, GUIColors::XPCHANGE);
		return GEM_ERROR;
	}

	// Save successful / Quick-save successful
	if (qsave) {
		displaymsg->DisplayMsgCentered(HCStrings::QSaveSuccess, FT_ANY, GUIColors::XPCHANGE);
	} else {
		displaymsg->DisplayMsgCentered(HCStrings::SaveSuccess, FT_ANY, GUIColors::XPCHANGE);
	}
	return GEM_OK;
}

int SaveGameIterator::CreateSaveGame(Holder<SaveGame> save, const String& slotname, bool force) const {
	auto mbSlotName = MBStringFromString(slotname);
	return CreateSaveGame(std::move(save), StringView { mbSlotName }, force);
}

int SaveGameIterator::CreateSaveGame(Holder<SaveGame> save, StringView slotname, bool force) const
{
	if (!slotname) {
		return GEM_ERROR;
	}

	int cannotSave = CanSave();
	if (cannotSave && !force) {
		return cannotSave;
	}

	int index;
	bool overrideRunning = false;

	if (save) {
		index = save->GetSaveID();
		if (core->saveGameAREExtractor.isRunningSaveGame(*save)) {
			overrideRunning = true;

			if (core->saveGameAREExtractor.createCacheBlob() == GEM_ERROR) {
				return GEM_ERROR;
			}
		}

		DeleteSaveGame(save);
		save.reset();
	} else {
		//leave space for autosaves
		//probably the hardcoded slot names should be read by this object
		//in that case 7 == size of hardcoded slot names array (savegame.2da)
		index = 7;
		for (const auto& save2 : save_slots) {
			if (save2->GetSaveID() >= index) {
				index = save2->GetSaveID() + 1;
			}
		}

		// we were called with an empty slot, so make sure that's true
		// normal and expansion saves could have the same name, but we skip them
		path_t basePath = PathJoin(core->config.SavePath, SaveDir());
		path_t path = basePath;
		if (!MakeDirectory(basePath)) {
			Log(ERROR, "SaveGameIterator", "Unable to create base save game directory '{}'", path);
			return GEM_ERROR;
		}
		index--;
		while (DirExists(path)) {
			index++;
			path_t dir = fmt::format("{:09d}-{}", index, slotname);
			path = PathJoin(basePath, dir);
		}
	}

	path_t Path;
	if (!CreateSavePath(Path, index, slotname)) {
		displaymsg->DisplayMsgCentered(HCStrings::CantSave, FT_ANY, GUIColors::XPCHANGE);
		return GEM_ERROR;
	}

	if (!DoSaveGame(Path.c_str(), overrideRunning)) {
		displaymsg->DisplayMsgCentered(HCStrings::CantSave, FT_ANY, GUIColors::XPCHANGE);
		return GEM_ERROR;
	}

	// Save successful
	displaymsg->DisplayMsgCentered(HCStrings::SaveSuccess, FT_ANY, GUIColors::XPCHANGE);
	return GEM_OK;
}

void SaveGameIterator::DeleteSaveGame(const Holder<SaveGame>& game) const
{
	if (!game) {
		return;
	}

	core->DelTree(game->GetPath(), false); //remove all files from folder
	rmdir(game->GetPath().c_str());
}

}
