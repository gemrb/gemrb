/* GemRB - Infinity Engine Emulator
* Copyright (C) 2003-2005 The GemRB Project
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

#include "Interface.h"

#include "exports.h"
#include "globals.h"
#include "strrefs.h"
#include "ie_cursors.h"

#include "ActorMgr.h"
#include "AmbientMgr.h"
#include "AnimationMgr.h"
#include "ArchiveImporter.h"
#include "Calendar.h"
#include "DataFileMgr.h"
#include "Debug.h"
#include "DialogHandler.h"
#include "DialogMgr.h"
#include "DisplayMessage.h"
#include "EffectMgr.h"
#include "EffectQueue.h"
#include "Factory.h"
#include "FontManager.h"
#include "Game.h"
#include "GameScript/GameScript.h"
#include "ItemMgr.h"
#include "KeyMap.h"
#include "MapMgr.h"
#include "MoviePlayer.h"
#include "MusicMgr.h"
#include "Palette.h"
#include "PluginLoader.h"
#include "PluginMgr.h"
#include "Predicates.h"
#include "ProjectileServer.h"
#include "SaveGameIterator.h"
#include "SaveGameMgr.h"
#include "ScriptedAnimation.h"
#include "SoundMgr.h"
#include "SpellMgr.h"
#include "StoreMgr.h"
#include "SymbolMgr.h"
#include "TileMap.h"
#include "VEFObject.h"
#include "Video/Video.h"
#include "WorldMapMgr.h"
#include "GUI/Button.h"
#include "GUI/Console.h"
#include "GUI/EventMgr.h"
#include "GUI/GameControl.h"
#include "GUI/GUIFactory.h"
#include "GUI/GUIScriptInterface.h"
#include "GUI/Label.h"
#include "GUI/MapControl.h"
#include "GUI/TextArea.h"
#include "GUI/WindowManager.h"
#include "GUI/WorldMapControl.h"
#include "RNG.h"
#include "Scriptable/Container.h"
#include "Streams/FileStream.h"
#include "System/FileFilters.h"

#include <utility>
#include <vector>

#ifdef WIN32
#include "CodepageToIconv.h"
#elif defined(HAVE_LANGINFO_H)
#include <langinfo.h>
#endif

namespace GemRB {

GEM_EXPORT PluginHolder<Video> VideoDriver;
GEM_EXPORT Interface* core = NULL;

struct AbilityTables {
	using AbilityTable = std::vector<ieWordSigned>;
	
	const int tableSize = 0;
	AbilityTable strmod;
	AbilityTable strmodex;
	AbilityTable intmod;
	AbilityTable dexmod;
	AbilityTable conmod;
	AbilityTable chrmod;
	AbilityTable lorebon;
	AbilityTable wisbon;
	
	explicit AbilityTables(int MaximumAbility) noexcept
	: tableSize(MaximumAbility + 1),
	strmod(tableSize * 4),
	strmodex(101 * 4),
	intmod(tableSize * 5),
	dexmod(tableSize * 3),
	conmod(tableSize * 5),
	chrmod(tableSize),
	lorebon(tableSize),
	wisbon(tableSize)
	{
		if (!ReadAbilityTable("strmod", strmod, 4, tableSize)) {
			Log(ERROR, "Interface", "unable to read 'strmod' ability table!");
		}
		
		//3rd ed doesn't have strmodex, but has a maximum of 40
		if (!ReadAbilityTable("strmodex", strmodex, 4, 101) && MaximumAbility <= 25) {
			Log(ERROR, "Interface", "unable to read 'strmodex' ability table!");
		}
		
		if (!ReadAbilityTable("intmod", intmod, 5, tableSize)) {
			Log(ERROR, "Interface", "unable to read 'intmod' ability table!");
		}
		
		if (!ReadAbilityTable("hpconbon", conmod, 5, tableSize)) {
			Log(ERROR, "Interface", "unable to read 'hpconbon' ability table!");
		}
		
		if (!core->HasFeature(GFFlags::RULES_3ED)) {
			//no lorebon in iwd2???
			if (!ReadAbilityTable("lorebon", lorebon, 1, tableSize)) {
				Log(ERROR, "Interface", "unable to read 'lorebon' ability table!");
			}
			
			//no dexmod in iwd2???
			if (!ReadAbilityTable("dexmod", dexmod, 3, tableSize)) {
				Log(ERROR, "Interface", "unable to read 'dexmod' ability table!");
			}
		}
		//this table is a single row (not a single column)
		if (!ReadAbilityTable("chrmodst", chrmod, tableSize, 1)) {
			Log(ERROR, "Interface", "unable to read 'chrmodst' ability table!");
		}

		if (gamedata->Exists("wisxpbon", IE_2DA_CLASS_ID, true)) {
			if (!ReadAbilityTable("wisxpbon", wisbon, 1, tableSize)) {
				Log(ERROR, "Interface", "unable to read 'wisxpbon' ability table!");
			}
		}
	}
	
private:
	bool ReadAbilityTable(const ResRef& tablename, AbilityTable& table, int columns, int rows) const
	{
		AutoTable tab = gamedata->LoadTable(tablename);
		if (!tab) {
			return false;
		}
		//this is a hack for rows not starting at 0 in some cases
		int fix = 0;
		const char * tmp = tab->GetRowName(0).c_str();
		if (tmp && (tmp[0]!='0')) {
			fix = atoi(tmp);
			for (int i=0;i<fix;i++) {
				for (int j=0;j<columns;j++) {
					table[rows*j+i] = tab->QueryFieldSigned<ieWordSigned>(0,j);
				}
			}
		}
		for (int j=0;j<columns;j++) {
			for( int i=0;i<rows-fix;i++) {
				table[rows*j+i+fix] = tab->QueryFieldSigned<ieWordSigned>(i,j);
			}
		}
		return true;
	}
};

static std::unique_ptr<AbilityTables> abilityTables;

static const char* const IWD2DeathVarFormat = "_DEAD{}";
static const char* DeathVarFormat = "SPRITE_IS_DEAD{}";

static const ieWord IDT_FAILURE = 0;
static const ieWord IDT_CRITRANGE = 1;
static const ieWord IDT_CRITMULTI = 2;
static const ieWord IDT_SKILLPENALTY = 3;

// FIXME: DragOp should be initialized with the button we are dragging from
// for now use a dummy until we truly implement this as a drag event
Control ItemDragOp::dragDummy = Control(Region());

ItemDragOp::ItemDragOp(CREItem* item)
: ControlDragOp(&dragDummy), item(item) {
	Item* i = gamedata->GetItem(item->ItemResRef);
	assert(i);
	Holder<Sprite2D> pic = gamedata->GetAnySprite(i->ItemIcon, -1, 1);
	if (pic == nullptr) {
		// use any / the smaller icon if the dragging one is unavailable
		pic = gamedata->GetBAMSprite(i->ItemIcon, -1, 0);
	}

	cursor = pic;

	// FIXME: this VarName is not consistant
	dragDummy.BindDictVariable("itembutton", Control::INVALID_VALUE);
}

Interface::Interface(CoreSettings&& cfg)
: config(std::move(cfg))
{
	Log(MESSAGE, "Core", "GemRB core version v" VERSION_GEMRB " loading ...");

	core = this;
	
	SetDebugMode(DebugMode(config.debugMode));
	
#if defined(WIN32)
	const uint32_t codepage = GetACP();
	const char* iconvCode = GetIconvNameForCodepage(codepage);

	if (nullptr == iconvCode) {
		throw CIE(fmt::format("Mapping of codepage {} unknown to iconv.", codepage));
	}
	config.SystemEncoding = iconvCode;
#elif defined(HAVE_LANGINFO_H)
	config.SystemEncoding = nl_langinfo(CODESET);
#endif

	gamedata = new GameData();
	sgiterator = new SaveGameIterator();

	if (!MakeDirectories(config.CachePath)) {
		throw CIE(fmt::format("Unable to create cache directory '{}'", config.CachePath));
	}

	if (StupidityDetector(config.CachePath)) {
		throw CIE(fmt::format("Cache path {} doesn't exist, not a folder or contains alien files!", config.CachePath));
	}
	if (!config.KeepCache) DelTree(config.CachePath, false);
	
	vars = std::move(config.vars);
	vars.Set("MaxPartySize", config.MaxPartySize); // for simple GUIScript access

	LoadPlugins();
	InitVideo();

	// ask the driver if a touch device is in use
	EventMgr::TouchInputEnabled = config.TouchInput < 0 ? VideoDriver->TouchInputEnabled() : config.TouchInput;
	EventMgr::DCDelay = config.DoubleClickDelay;
	Control::ActionRepeatDelay = config.ActionRepeatDelay;
	GameControl::DebugFlags = config.DebugFlags;

	ieDword brightness = vars.Get("Brightness Correction", 10);
	ieDword contrast = vars.Get("Gamma Correction", 5);

	Log(MESSAGE, "Core", "Initializing search path...");
	if (!IsAvailable(PLUGIN_RESOURCE_DIRECTORY)) {
		throw CIE("no DirectoryImporter!");
	}

	path_t path = config.CachePath;
	if (!gamedata->AddSource(path, "Cache", PLUGIN_RESOURCE_DIRECTORY)) {
		throw CIE("The cache path couldn't be registered, please check!");
	}

	for (const auto& modPath : config.ModPath) {
		gamedata->AddSource(modPath, "Mod paths", PLUGIN_RESOURCE_CACHEDDIRECTORY);
	}

	path = PathJoin(config.GemRBOverridePath, "override", config.GameType);
	if (config.GameType == "auto") {
		gamedata->AddSource(path, "GemRB Override", PLUGIN_RESOURCE_NULL);
	} else {
		gamedata->AddSource(path, "GemRB Override", PLUGIN_RESOURCE_CACHEDDIRECTORY);
	}

	path = PathJoin(config.GemRBOverridePath, "override", "shared");
	gamedata->AddSource(path, "shared GemRB Override", PLUGIN_RESOURCE_CACHEDDIRECTORY);

	path = PathJoin(config.GamePath, config.GameOverridePath);
	gamedata->AddSource(path, "Override", PLUGIN_RESOURCE_CACHEDDIRECTORY);

	// GAME sounds are intentionally not cached, in IWD there are directory structures,
	// that are not cacheable, also it is totally pointless (this fixed charsounds in IWD)
	path = PathJoin(config.GamePath, config.GameSoundsPath);
	gamedata->AddSource(path, "Sounds", PLUGIN_RESOURCE_DIRECTORY);

	path = PathJoin(config.GamePath, config.GameMoviesPath);
	gamedata->AddSource(path, "Movies", PLUGIN_RESOURCE_DIRECTORY);

	path = PathJoin(config.GamePath, config.GameScriptsPath);
	gamedata->AddSource(path, "Scripts", PLUGIN_RESOURCE_CACHEDDIRECTORY);

	path = PathJoin(config.GamePath, config.GamePortraitsPath);
	gamedata->AddSource(path, "Portraits", PLUGIN_RESOURCE_CACHEDDIRECTORY);

	path = PathJoin(config.GamePath, config.GameDataPath);
	gamedata->AddSource(path, "Data", PLUGIN_RESOURCE_CACHEDDIRECTORY);

	// accomodating silly installers that create a data/Data/.* structure
	path = PathJoin(config.GamePath, config.GameDataPath, "Data");
	gamedata->AddSource(path, "Data", PLUGIN_RESOURCE_CACHEDDIRECTORY);

	// IWD2 movies are on the CD but not in the BIF
	for (size_t i = 0; i < MAX_CD; i++) {
		const std::string description = fmt::format("CD{}/data", i);
		for (size_t j = 0; j < config.CD[i].size(); j++) {
			path = PathJoin(config.CD[i][j], config.GameDataPath);
			gamedata->AddSource(path, description, PLUGIN_RESOURCE_CACHEDDIRECTORY);
		}
	}

	// most of the old gemrb override files can be found here,
	// so they have a lower priority than the game files and can more easily be modded
	path = PathJoin(config.GemRBUnhardcodedPath, "unhardcoded", config.GameType);
	if (config.GameType == "auto") {
		gamedata->AddSource(path, "GemRB Unhardcoded data", PLUGIN_RESOURCE_NULL);
	} else {
		gamedata->AddSource(path, "GemRB Unhardcoded data", PLUGIN_RESOURCE_CACHEDDIRECTORY);
	}
	path = PathJoin(config.GemRBUnhardcodedPath, "unhardcoded", "shared");
	gamedata->AddSource(path, "shared GemRB Unhardcoded data", PLUGIN_RESOURCE_CACHEDDIRECTORY);

	Log(MESSAGE, "Core", "Initializing KEY Importer...");
	path_t ChitinPath = PathJoin(config.GamePath, "chitin.key");
	if (!gamedata->AddSource(ChitinPath, "chitin.key", PLUGIN_RESOURCE_KEY)) {
		Log(FATAL, "Core", "Failed to load \"chitin.key\"");
		Log(ERROR, "Core", "This means:\n- you set the GamePath config variable incorrectly,\n\
- you passed a bad game path to GemRB on the command line,\n\
- you are not running GemRB from within a game dir,\n\
- or the game is running (Windows only).");
		throw CIE("The path must point to a game directory with a readable chitin.key file.");
	}

	fogRenderer = std::make_unique<FogRenderer>(config.SpriteFoW);

	Log(MESSAGE, "Core", "Initializing GUI Script Engine...");
	SetNextScript("Start"); // Start is the first script executed
	guiscript = MakePluginHolder<ScriptEngine>(IE_GUI_SCRIPT_CLASS_ID);
	if (guiscript == nullptr) {
		throw CIE("Missing GUI Script Engine.");
	}
	if (!guiscript->Init()) {
		throw CIE("Failed to initialize GUI Script.");
	}

	// re-set the gemrb override path, since we now have the correct GameType if 'auto' was used
	path = PathJoin(config.GemRBOverridePath, "override", config.GameType);
	gamedata->AddSource(path, "GemRB Override", PLUGIN_RESOURCE_CACHEDDIRECTORY, RM_REPLACE_SAME_SOURCE);
	path_t unhardcodedTypePath = PathJoin(config.GemRBUnhardcodedPath, "unhardcoded", config.GameType);
	gamedata->AddSource(unhardcodedTypePath, "GemRB Unhardcoded data", PLUGIN_RESOURCE_CACHEDDIRECTORY, RM_REPLACE_SAME_SOURCE);

	// Purposely add the font directory last since we will only ever need it at engine load time.
	if (config.CustomFontPath[0]) gamedata->AddSource(config.CustomFontPath, "CustomFonts", PLUGIN_RESOURCE_DIRECTORY);

	Log(MESSAGE, "Core", "Reading Game Options...");
	LoadGemRBINI();

	// SDL2 driver requires the display to be created prior to sprite creation (opengl context)
	// we also need the display to exist to create sprites using the display format
	ieDword fullscreen = vars.Get("Full Screen", 0);

	int createDisplayResult =
	VideoDriver->CreateDisplay(
				Size(config.Width, config.Height),
				config.Bpp,
				fullscreen,
				config.GameName.c_str(),
				config.CapFPS == 0
		);

	if (createDisplayResult == GEM_ERROR) {
		throw CIE("Cannot initialize shaders.");
	}
	VideoDriver->SetGamma(brightness, contrast);

	// load the game ini (baldur.ini, torment.ini, icewind.ini ...)
	// read from our version of the config if it is present
	path_t gemrbINI = "gem-" + INIConfig;
	path_t tmp;
	path_t ini_path = PathJoin(config.SavePath, gemrbINI);
	if (!FileExists(ini_path)) {
		ini_path = PathJoin(config.GamePath, gemrbINI);
	}
	if (FileExists(ini_path)) {
		tmp = INIConfig;
		INIConfig = gemrbINI;
	} else {
		ini_path = PathJoin(config.GamePath, INIConfig);
		Log(MESSAGE,"Core", "Loading original game options from {}", ini_path);
	}
	if (!InitializeVarsWithINI(ini_path)) {
		Log(WARNING, "Core", "Unable to set dictionary default values!");
	}

	// We use this for the game's state exclusively
	ieDword maxRefreshRate = vars.Get("Maximum Frame Rate", 30);
	// the originals used double ticks for haste handling
	Time.ticksPerSec = maxRefreshRate / 2;

	// set up the tooltip delay which we store in milliseconds
	ieDword tooltipDelay = vars.Get("Tooltips", 0);
	WindowManager::SetTooltipDelay(tooltipDelay * Tooltip::DELAY_FACTOR / 10);

	// restore the game config name if we read it from our version
	if (!tmp.empty()) {
		INIConfig = tmp;
	} else {
		tmp = INIConfig;
	}
	// also store it for base GAM and SAV files
	strtok(&tmp[0], ".");
	GameNameResRef = tmp;

	Log(MESSAGE, "Core", "Reading Encoding Table...");
	if (!LoadEncoding()) {
		Log(ERROR, "Core", "Cannot Load Encoding.");
	}

	Log(MESSAGE, "Core", "Creating Projectile Server...");
	projserv = new ProjectileServer();

	Log(MESSAGE, "Core", "Checking for Dialogue Manager...");
	strings = MakePluginHolder<StringMgr>(IE_TLK_CLASS_ID);
	Log(MESSAGE, "Core", "Loading Dialog.tlk file...");
	path_t strpath = PathJoin(config.GamePath, "dialog.tlk");
	FileStream* fs = FileStream::OpenFile(strpath);

	if (!fs) {
		// EE multi language deployment
		strpath = PathJoin(config.GamePath, config.GameLanguagePath, "dialog.tlk");
		fs = FileStream::OpenFile(strpath);

		if (!fs) {
			throw CIE("Cannot find Dialog.tlk.");
		}
	}
	strings->Open(fs);

	// does the language use an extra tlk?
	if (strings->HasAltTLK()) {
		strings2 = MakePluginHolder<StringMgr>(IE_TLK_CLASS_ID);
		Log(MESSAGE, "Core", "Loading DialogF.tlk file...");
		strpath = PathJoin(config.GamePath, "dialogf.tlk");
		fs = FileStream::OpenFile(strpath);
		if (!fs) {
			// try EE-style paths
			strpath = PathJoin(config.GamePath, config.GameLanguagePath, "dialogf.tlk");
			fs = FileStream::OpenFile(strpath);
		}
		if (!fs) {
			Log(ERROR, "Core", "Cannot find DialogF.tlk. Let us know which translation you are using.");
			Log(ERROR, "Core", "Falling back to main TLK file, so female text may be wrong!");
			strings2 = strings;
		} else {
			strings2->Open(fs);
		}
	}

	Log(MESSAGE, "Core", "Loading palettes...");
	LoadPalette<16>(Palette16, palettes16);
	LoadPalette<32>(Palette32, palettes32);
	LoadPalette<256>(Palette256, palettes256);
	Log(MESSAGE, "Core", "Palettes loaded.");

	Log(MESSAGE, "Core", "Initializing stock sounds...");
	if (!gamedata->ReadResRefTable(ResRef("defsound"), gamedata->defaultSounds)) {
		throw CIE("Cannot find defsound.2da.");
	}

	LoadSprites();
	LoadFonts();
	gamedata->PreloadColors();

	Log(MESSAGE, "Core", "Initializing string constants...");
	displaymsg = new DisplayMessage();

	auto guifact = GetImporter<GUIFactory>(IE_CHU_CLASS_ID);
	if (!guifact) {
		throw CIE("Failed to load Window Manager.");
	}

	Log(MESSAGE, "Core", "Initializing Window Manager...");
	winmgr = new WindowManager(VideoDriver, guifact);
	RegisterScriptableWindow(winmgr->GetGameWindow(), "GAMEWIN", 0);
	winmgr->SetCursorFeedback(WindowManager::CursorFeedback(config.MouseFeedback));

	QuitFlag = QF_CHANGESCRIPT;

	InitAudio();
	
	if (HasFeature( GFFlags::HAS_PARTY_INI )) {
		Log(MESSAGE, "Core", "Loading precreated teams setup...");
		INIparty = MakePluginHolder<DataFileMgr>(IE_INI_CLASS_ID);
		path_t tINIparty = PathJoin(config.GamePath, "Party.ini");
		fs = FileStream::OpenFile(tINIparty);
		if (!INIparty->Open(fs)) {
			Log(WARNING, "Core", "Failed to load precreated teams.");
		}
	}

	if (HasFeature(GFFlags::IWD2_DEATHVARFORMAT)) {
		DeathVarFormat = IWD2DeathVarFormat;
	}

	if (HasFeature( GFFlags::HAS_BEASTS_INI )) {
		Log(MESSAGE, "Core", "Loading beasts definition File...");
		INIbeasts = MakePluginHolder<DataFileMgr>(IE_INI_CLASS_ID);
		path_t tINIbeasts = PathJoin(config.GamePath, "beast.ini");
		fs = FileStream::OpenFile(tINIbeasts);
		if (!INIbeasts->Open(fs)) {
			Log(WARNING, "Core", "Failed to load beast definitions.");
		}

		Log(MESSAGE, "Core", "Loading quests definition File...");
		INIquests = MakePluginHolder<DataFileMgr>(IE_INI_CLASS_ID);
		path_t tINIquests = PathJoin(config.GamePath, "quests.ini");
		FileStream* fs2 = FileStream::OpenFile(tINIquests);
		if (!INIquests->Open(fs2)) {
			Log(WARNING, "Core", "Failed to load quest definitions.");
		}
	}

	Log(MESSAGE, "Core", "Initializing Inventory Management...");
	InitItemTypes();

	Log(MESSAGE, "Core", "Initializing random treasure...");
	bool ret = ReadRandomItems();
	if (!ret) {
		Log(WARNING, "Core", "Failed to initialize random treasure.");
	}
	
	abilityTables = std::make_unique<AbilityTables>(MaximumAbility);

	Log(MESSAGE, "Core", "Reading game time table...");
	ret = ReadGameTimeTable();
	if (!ret) {
		throw CIE("Failed to read game time table...");
	}

	ret = ReadDamageTypeTable();
	Log(MESSAGE, "Core", "Reading damage type table...");
	if (!ret) {
		Log(WARNING, "Core", "Reading damage type table...");
	}

	Log(MESSAGE, "Core", "Reading game script tables...");
	InitializeIEScript();

	Log(MESSAGE, "Core", "Initializing keymap tables...");
	keymap = new KeyMap();
	ret = keymap->InitializeKeyMap("keymap.ini", "keymap");
	if (!ret) {
		Log(WARNING, "Core", "Failed to initialize keymaps.");
	}

	Log(MESSAGE, "Core", "Core Initialization Complete!");

#ifdef HAVE_REALPATH
	if (unhardcodedTypePath[0] == '.') {
		// canonicalize the relative path; usually from running from the build dir
		char *absolutePath = realpath(unhardcodedTypePath.c_str(), NULL);
		if (absolutePath) {
			unhardcodedTypePath = absolutePath;
			free(absolutePath);
		}
	}
#endif
	// dump the potentially changed unhardcoded path to a file that weidu looks at automatically to get our search paths
	path_t pathString = fmt::format("GemRB_Data_Path = {}", unhardcodedTypePath);
	strpath = PathJoin(config.GamePath, "gemrb_path.txt");
	FileStream *pathFile = new FileStream();
	// don't abort if something goes wrong, since it should never happen and it's not critical
	if (pathFile->Create(strpath)) {
		pathFile->Write(pathString.c_str(), pathString.length());
		pathFile->Close();
	}
	delete pathFile;

	EventMgr::EventCallback ToggleConsole = [this](const Event& e) {
		if (e.type != Event::KeyDown) return false;

		guiscript->RunFunction("Console", "ToggleConsole");
		return true;
	};
	EventMgr::RegisterHotKeyCallback(ToggleConsole, ' ', GEM_MOD_CTRL);
}

Interface::~Interface() noexcept
{
	WindowManager::CursorMouseUp = NULL;
	WindowManager::CursorMouseDown = NULL;

	delete winmgr;

	//destroy the highest objects in the hierarchy first!
	// here gamectrl is either null (no game) or already taken out by its window (game loaded)
	assert(game == nullptr);
	delete calendar;
	delete worldmap;
	delete keymap;

	// fonts need to be destroyed before TTF plugin
	PluginMgr::Get()->RunCleanup();

	slotTypes.clear();
	itemtypedata.clear();

	delete sgiterator;

	DamageInfoMap.clear();

	delete projserv;

	delete displaymsg;
	delete TooltipBG;

	delete gamedata;
	gamedata = NULL;

	// Removing all stuff from Cache, except bifs
	if (!config.KeepCache) DelTree(config.CachePath, true);
}

GameControl* Interface::StartGameControl()
{
	assert(gamectrl == nullptr);

	Region screen(0, 0, config.Width, config.Height);
	gamectrl = new GameControl(screen);
	gamectrl->AssignScriptingRef(0, "GC");

	return gamectrl;
}

/* handle main loop events that might destroy or create windows
thus cannot be called from DrawWindows directly
these events are pending until conditions are right
*/
void Interface::HandleEvents()
{
	if (EventFlag&EF_SELECTION) {
		EventFlag&=~EF_SELECTION;
		guiscript->RunFunction( "GUICommonWindows", "SelectionChanged", false);
	}

	if (EventFlag&EF_UPDATEANIM) {
		EventFlag&=~EF_UPDATEANIM;
		guiscript->RunFunction( "GUICommonWindows", "UpdateAnimation", false);
	}

	if (EventFlag&EF_PORTRAIT) {
		EventFlag&=~EF_PORTRAIT;

		const Window* win = GetWindow(0, "PORTWIN");
		if (win) {
			guiscript->RunFunction( "GUICommonWindows", "UpdatePortraitWindow" );
		}
	}

	if (EventFlag&EF_ACTION) {
		EventFlag&=~EF_ACTION;

		const Window* win = GetWindow(0, "ACTWIN");
		if (win) {
			guiscript->RunFunction( "GUICommonWindows", "UpdateActionsWindow" );
		}
	}

	if (EventFlag&EF_CONTROL) {
		// handle this before clearing EF_CONTROL
		// otherwise we do this every frame
		ToggleViewsVisible(!(game->ControlStatus & CS_HIDEGUI), "HIDE_CUT");

		EventFlag&=~EF_CONTROL;
		guiscript->RunFunction( "MessageWindow", "UpdateControlStatus" );

		return;
	}
	if (EventFlag&EF_SHOWMAP) {
		EventFlag &= ~EF_SHOWMAP;
		guiscript->RunFunction("GUIMA", "ShowMap");
		return;
	}

	if (EventFlag&EF_SEQUENCER) {
		EventFlag&=~EF_SEQUENCER;
		guiscript->RunFunction( "GUIMG", "OpenSequencerWindow" );
		return;
	}

	if (EventFlag&EF_IDENTIFY) {
		EventFlag&=~EF_IDENTIFY;
		// FIXME: Implement this.
		guiscript->RunFunction( "GUICommonWindows", "OpenIdentifyWindow" );
		return;
	}
	if (EventFlag&EF_OPENSTORE) {
		EventFlag&=~EF_OPENSTORE;
		guiscript->RunFunction( "GUISTORE", "OpenStoreWindow" );
		return;
	}

	if (EventFlag&EF_EXPANSION) {
		EventFlag&=~EF_EXPANSION;
		guiscript->RunFunction( "Game", "GameExpansion", false );
		return;
	}

	if (EventFlag&EF_CREATEMAZE) {
		EventFlag&=~EF_CREATEMAZE;
		guiscript->RunFunction( "Maze", "CreateMaze", false );
		return;
	}

	if ((EventFlag&EF_RESETTARGET) && gamectrl) {
		EventFlag&=~EF_RESETTARGET;
		EventFlag|=EF_TARGETMODE;
		gamectrl->ResetTargetMode();
		return;
	}

	if ((EventFlag&EF_TARGETMODE) && gamectrl) {
		EventFlag&=~EF_TARGETMODE;
		gamectrl->UpdateTargetMode();
		return;
	}

	if (EventFlag&EF_TEXTSCREEN) {
		EventFlag&=~EF_TEXTSCREEN;
		winmgr->SetCursorFeedback(WindowManager::CursorFeedback(core->config.MouseFeedback));
		guiscript->RunFunction( "TextScreen", "StartTextScreen" );
		return;
	}
}

/* handle main loop events that might destroy or create windows
thus cannot be called from DrawWindows directly
*/
void Interface::HandleFlags() noexcept
{
	//clear events because the context changed
	EventFlag = EF_CONTROL;

	if (QuitFlag&(QF_QUITGAME|QF_EXITGAME) ) {
		// closing windows must come before tearing anything else down
		// some window close handlers expect game/gamecontrol to be valid
		// let them run, then start tearing things down
		winmgr->CloseAllWindows();
		// when reaching this, quitflag should be 1 or 2
		// if Exitgame was set, we'll set Start.py too
		QuitGame (QuitFlag&QF_EXITGAME);
	}

	if (QuitFlag & (QF_QUITGAME | QF_EXITGAME | QF_LOADGAME | QF_ENTERGAME)) {
		delete winmgr->GetGameWindow()->RemoveSubview(gamectrl);
		gamectrl = nullptr;
		winmgr->GetGameWindow()->SetVisible(false);
		//clear cutscenes; clear fade/screenshake effects
		timer = GlobalTimer();
		QuitFlag &= ~QF_QUITGAME;
	}
	
	if (QuitFlag & QF_EXITGAME) {
		QuitFlag = QF_KILL;
		return;
	}

	if (QuitFlag&QF_LOADGAME) {
		QuitFlag &= ~QF_LOADGAME;

		LoadGame(LoadGameIndex, VersionOverride );
		LoadGameIndex.reset();
		//after loading a game, always check if the game needs to be upgraded
	}

	if (QuitFlag&QF_ENTERGAME) {
		winmgr->CloseAllWindows();
		QuitFlag &= ~QF_ENTERGAME;
		if (game) {
			EventFlag|=EF_EXPANSION;

			Log(MESSAGE, "Core", "Setting up the Console...");
			guiscript->RunFunction("Console", "OnLoad");

			winmgr->FadeColor = Color();

			GameControl* gc = StartGameControl();
			guiscript->LoadScript( "Game" );
			guiscript->RunFunction( "Game", "EnterGame" );

			//switch map to protagonist
			const Actor* actor = GetFirstSelectedPC(true);
			if (actor) {
				gc->ChangeMap(actor, true);
			}

			//rearrange party slots
			game->ConsolidateParty();

			Window* gamewin = winmgr->GetGameWindow();
			gamewin->AddSubviewInFrontOfView(gc);
			gamewin->SetDisabled(false);
			gamewin->SetVisible(true);
			gamewin->Focus();
		} else {
			Log(ERROR, "Core", "No game to enter...");
			QuitFlag = QF_QUITGAME;
		}
	}

	if (QuitFlag&QF_CHANGESCRIPT) {
		QuitFlag &= ~QF_CHANGESCRIPT;
		guiscript->LoadScript(nextScript);
		guiscript->RunFunction(nextScript.c_str(), "OnLoad");
	}
}

bool Interface::ReadGameTimeTable()
{
	AutoTable table = gamedata->LoadTable("gametime");
	if (!table) {
		return false;
	}

	Time.round_sec = table->QueryFieldUnsigned<unsigned int>("ROUND_SECONDS", "DURATION");
	Time.turn_sec = table->QueryFieldUnsigned<unsigned int>("TURN_SECONDS", "DURATION");
	Time.round_size = Time.round_sec * Time.defaultTicksPerSec;
	Time.rounds_per_turn = Time.turn_sec / Time.round_sec;
	Time.attack_round_size = table->QueryFieldUnsigned<unsigned int>("ATTACK_ROUND", "DURATION");
	Time.hour_sec = 300; // move to table if pst turns out to be different
	Time.hour_size = Time.hour_sec * Time.defaultTicksPerSec;
	Time.day_sec = Time.hour_sec * 24; // move to table if pst turns out to be different
	Time.day_size = Time.day_sec * Time.defaultTicksPerSec;
	Time.fade_reset = table->QueryFieldUnsigned<unsigned int>("FADE_RESET", "DURATION");

	return true;
}

//Static
const char *Interface::GetDeathVarFormat()
{
	return DeathVarFormat;
}

bool Interface::ReadMusicTable(const ResRef& tablename, int col) {
	AutoTable tm = gamedata->LoadTable(tablename);
	if (!tm)
		return false;

	for (TableMgr::index_t i = 0; i < tm->GetRowCount(); i++) {
		musiclist.emplace_back(tm->QueryField(i, col));
	}

	return true;
}

bool Interface::ReadDamageTypeTable() {
	AutoTable tm = gamedata->LoadTable("dmginfo");
	if (!tm)
		return false;

	DamageInfoStruct di;
	for (TableMgr::index_t i = 0; i < tm->GetRowCount(); i++) {
		di.strref = DisplayMessage::GetStringReference(HCStrings(tm->QueryFieldUnsigned<uint16_t>(i, 0)));
		di.resist_stat = TranslateStat(tm->QueryField(i, 1));
		di.value = strtounsigned<unsigned int>(tm->QueryField(i, 2).c_str(), nullptr, 16);
		di.iwd_mod_type = tm->QueryFieldSigned<int>(i, 3);
		di.reduction = tm->QueryFieldSigned<int>(i, 4);
		DamageInfoMap.emplace(di.value, di);
	}

	return true;
}

bool Interface::ReadSoundChannelsTable() const
{
	AutoTable tm = gamedata->LoadTable("sndchann");
	if (!tm) {
		return false;
	}

	TableMgr::index_t ivol = tm->GetColumnIndex("VOLUME");
	TableMgr::index_t irev = tm->GetColumnIndex("REVERB");
	for (TableMgr::index_t i = 0; i < tm->GetRowCount(); i++) {
		auto rowname = tm->GetRowName(i);
		// translate some alternative names for the IWDs
		if (rowname == "ACTION") rowname = "ACTIONS";
		else if (rowname == "SWING") rowname = "SWINGS";
		AudioDriver->SetChannelVolume(rowname, tm->QueryFieldSigned<int>(i, ivol));
		if (irev != TableMgr::npos) {
			AudioDriver->SetChannelReverb(rowname, atof(tm->QueryField(i, irev).c_str()));
		}
	}
	return true;
}

const static ieVariable star("*");
const ieVariable& Interface::GetMusicPlaylist(size_t SongType) const {
	if (SongType >= musiclist.size()) return star;
	return musiclist[SongType];
}

void Interface::DisableMusicPlaylist(size_t SongType)
{
	if (SongType < musiclist.size()) {
		musiclist[SongType] = star;
	}
}

/** this is the main loop */
void Interface::Main()
{
	int speed = vars.Get("Mouse Scroll Speed", 10);
	SetMouseScrollSpeed(speed);

	// We had 36 at 30fps originally, so more fps
	// should not speed this up unless adjusted
	auto scrollSpeed = [=](int fps) {
		return std::max<int>(1080.0 * (speed / 36.0) / fps, 1);
	};

	if (config.CapFPS == 0) {
		int refreshRate = VideoDriver->GetDisplayRefreshRate();
		if (refreshRate > 0) {
			SetMouseScrollSpeed(scrollSpeed(refreshRate));
		}
	} else if (config.CapFPS >= 30) {
		SetMouseScrollSpeed(scrollSpeed(config.CapFPS));
	}

	auto fps = GetTextFont();
	// TODO: if we ever want to support dynamic resolution changes this will break
	Region fpsRgn(0, config.Height - 30, 80, 30);
	String fpsstring = u"???.??? fps";
	// set for printing
	fpsRgn.x = 5;
	fpsRgn.y = 0;

	tick_t frame = 0;
	tick_t time = GetMilliseconds();
	tick_t timebase = time;
	tick_t lastGameUpdate = 0;

	double frames = 0.0;

	do {
		for (auto it = timers.begin(); it != timers.end();) {
			if (it->IsRunning()) {
				it->Update(time);
				++it;
			} else {
				it = timers.erase(it);
			}
		}

		//don't change script when quitting is pending
		while (QuitFlag && QuitFlag != QF_KILL) {
			HandleFlags();
		}
		
		if (gamectrl) {
			//eventflags are processed only when there is a game
			if (EventFlag) {
				HandleEvents();
			}
			HandleGUIBehaviour(gamectrl);
		}

		time = GetMilliseconds();

		static const tick_t oneTick = 1000 / Time.ticksPerSec;
		bool doGameStateUpdate = time - lastGameUpdate >= oneTick;
		if (doGameStateUpdate) {
			GameLoop();
			// TODO: find other animations that need to be synchronized
			// we can create a manager for them and everything can be updated at once
			GlobalColorCycle.AdvanceTime(time);
			lastGameUpdate = time;
		}

		winmgr->DrawWindows();
		if (config.DrawFPS) {
			frame++;
			if (time - timebase > 1000) {
				frames = ( frame * 1000.0 / ( time - timebase ) );
				timebase = time;
				frame = 0;
				fpsstring = fmt::format(u"{:.3f} fps", frames);
			}
			auto lock = winmgr->DrawHUD();
			VideoDriver->DrawRect( fpsRgn, ColorBlack );
			fps->Print(fpsRgn, String(fpsstring), IE_FONT_ALIGN_MIDDLE | IE_FONT_SINGLE_LINE, {ColorWhite, ColorBlack});
		}
	} while (VideoDriver->SwapBuffers(config.CapFPS) == GEM_OK && !(QuitFlag&QF_KILL));
	QuitGame(0);
}

void Interface::InitVideo() const
{
	Log(MESSAGE, "Core", "Initializing Video Driver...");
	VideoDriver = std::static_pointer_cast<Video>(PluginMgr::Get()->GetDriver(&Video::ID, config.VideoDriverName));
	if (!VideoDriver) {
		throw CIE("No Video Driver Available.");
	}
	if (VideoDriver->Init() == GEM_ERROR) {
		throw CIE("Cannot Initialize Video Driver.");
	}
}

void Interface::InitAudio()
{
	Log(MESSAGE, "Core", "Starting up the Sound Driver...");
	AudioDriver = std::static_pointer_cast<Audio>(PluginMgr::Get()->GetDriver(&Audio::ID, config.AudioDriverName));
	if (AudioDriver == nullptr) {
		throw CIE("Failed to load sound driver.");
	}
	if (!AudioDriver->Init()) {
		throw CIE("Failed to initialize sound driver.");
	}

	Log(MESSAGE, "Core", "Initializing Music Manager...");
	music = MakePluginHolder<MusicMgr>(IE_MUS_CLASS_ID);
	if (!music) {
		throw CIE("Failed to load Music Manager.");
	}

	Log(MESSAGE, "Core", "Loading music list...");
	bool ret = true;
	if (HasFeature( GFFlags::HAS_SONGLIST )) {
		ret = ReadMusicTable("songlist", 1);
	} else {
		/*since bg1 and pst has no .2da for songlist,
		we must supply one in the gemrb/override folder.
		It should be: music.2da, first column is a .mus filename*/
		ret = ReadMusicTable("music", 0);
	}
	if (!ret) {
		Log(WARNING, "Core", "Didn't find music list.");
	}

	int resdata = HasFeature( GFFlags::RESDATA_INI );
	if (resdata || HasFeature(GFFlags::SOUNDS_INI) ) {
		Log(MESSAGE, "Core", "Loading resource data File...");
		INIresdata = MakePluginHolder<DataFileMgr>(IE_INI_CLASS_ID);
		StringView sv(resdata ? "resdata" : "sounds");
		DataStream* ds = gamedata->GetResourceStream(sv, IE_INI_CLASS_ID);
		if (!INIresdata->Open(ds)) {
			Log(WARNING, "Core", "Failed to load resource data.");
		}
	}

	Log(MESSAGE, "Core", "Setting up SFX channels...");
	ret = ReadSoundChannelsTable();
	if (!ret) {
		Log(WARNING, "Core", "Failed to read channel table.");
	}
}

void Interface::LoadPlugins() const
{
	plugin_flags_t pluginFlags;
	if (!config.SkipPlugin.empty()) {
		pluginFlags[config.SkipPlugin] = PluginFlagsType::SKIP;
	}

	if (!config.DelayPlugin.empty()) {
		pluginFlags[config.DelayPlugin] = PluginFlagsType::DELAY;
	}

	Log(MESSAGE, "Core", "Starting Plugin Manager...");
	const PluginMgr *plugin = PluginMgr::Get();
#if TARGET_OS_MAC
	// search the bundle plugins first
	// since bundle plugins are loaded first dyld will give them precedence
	// if duplicates are found in the PluginsPath
	path_t bundlePluginsPath = BundlePath(PLUGINS);
	ResolveFilePath(bundlePluginsPath);
#ifndef STATIC_LINK
	GemRB::LoadPlugins(bundlePluginsPath, pluginFlags);
#endif
#endif
#ifndef STATIC_LINK
	GemRB::LoadPlugins(config.PluginsPath, pluginFlags);
#endif
	if (plugin && plugin->GetPluginCount()) {
		Log(MESSAGE, "Core", "Plugin Loading Complete...");
	} else {
		throw CIE("Plugin Loading Failed, check path...");
	}
	plugin->RunInitializers(config);

	for (const auto& type : {IE_2DA_CLASS_ID, IE_INI_CLASS_ID, IE_TLK_CLASS_ID, IE_BAM_CLASS_ID}) {
		if (!IsAvailable(type)) {
			throw CIE("Missing required plugin for " + TypeExt(type));
		}
	}
}

void Interface::LoadSprites()
{
	Log(MESSAGE, "Core", "Loading Cursors...");
	auto anim = gamedata->GetFactoryResourceAs<const AnimationFactory>(MainCursorsImage, IE_BAM_CLASS_ID);
	size_t CursorCount = 0;
	if (anim) {
		CursorCount = anim->GetCycleCount();
		Cursors.reserve(CursorCount);
		for (size_t i = 0; i < CursorCount; i++) {
			Cursors.push_back(anim->GetFrame(0, (ieByte) i));
		}
	} else {
		// support non-BAM cursors
		// load MainCursorsImage + XX until all images are exhausted
		// same layout as in the originals, with odd indices having the pressed cursor image
		path_t fileName;
		while (CursorCount < 99) {
			fileName = fmt::format("{}{:02}", MainCursorsImage, CursorCount);
			ResourceHolder<ImageMgr> im = gamedata->GetResourceHolder<ImageMgr>(fileName, true);
			if (!im) break;
			Cursors.push_back(im->GetSprite2D());
			CursorCount++;
		}
	}

	// this is the last existing cursor type
	if (CursorCount<IE_CURSOR_WAY) {
		throw CIE(fmt::format("Failed to load enough cursors ({} < {}).", CursorCount, IE_CURSOR_WAY));
	}
	WindowManager::CursorMouseUp = Cursors[0];
	WindowManager::CursorMouseDown = Cursors[1];

	// Load ground circle bitmaps (PST only)
	Log(MESSAGE, "Core", "Loading Ground circle bitmaps...");
	for (int size = 0; size < MAX_CIRCLE_SIZE; size++) {
		if (!GroundCircleBam[size].IsEmpty()) {
			anim = gamedata->GetFactoryResourceAs<const AnimationFactory>(GroundCircleBam[size], IE_BAM_CLASS_ID);
			if (!anim || anim->GetCycleCount() != 6) {
				// unknown type of circle anim
				throw CIE("Failed Loading Ground circle bitmaps...");
			}

			for (int i = 0; i < 6; i++) {
				Holder<Sprite2D> sprite = anim->GetFrame(0, (ieByte) i);
				if (GroundCircleScale[size]) {
					sprite = VideoDriver->SpriteScaleDown(sprite, GroundCircleScale[size]);
				}
				GroundCircles[size][i] = sprite;
			}
		}
	}
}

void Interface::LoadFonts()
{
	Log(MESSAGE, "Core", "Loading Fonts...");
	AutoTable tab = gamedata->LoadTable("fonts");
	if (!tab) {
		throw CIE("Cannot find fonts.2da.");
	}

	// FIXME: we used to try and share like fonts
	// this only ever had a benefit when using non-bam fonts
	// it could potentially save a a few megs of space (nice for handhelds)
	// but we should re-enable this by simply letting Font instances share their atlas data

	TableMgr::index_t count = tab->GetRowCount();
	for (TableMgr::index_t row = 0; row < count; ++row) {
		const auto& rowName = tab->GetRowName(row);

		ResRef resref = tab->QueryField(rowName, "RESREF");
		const auto& font_name = tab->QueryField(rowName, "FONT_NAME");
		ieWord font_size = tab->QueryFieldUnsigned<ieWord>( rowName, "PX_SIZE" ); // not available in BAM fonts.
		FontStyle font_style = (FontStyle)tab->QueryFieldSigned<int>(rowName, "STYLE"); // not available in BAM fonts.
		bool background = tab->QueryFieldSigned<int>(rowName, "BACKGRND") != 0;

		Holder<Font> fnt;
		ResourceHolder<FontManager> fntMgr = gamedata->GetResourceHolder<FontManager>(font_name);
		if (fntMgr) fnt = fntMgr->GetFont(font_size, font_style, background);

		if (!fnt) {
			error("Core", "Unable to load font resource: {} for ResRef {} (check fonts.2da)", font_name, resref);
		} else {
			fonts[resref] = fnt;

			Log(MESSAGE, "Core", "Loaded Font: {} for ResRef {}", font_name, resref);
		}
	}

	Log(MESSAGE, "Core", "Fonts Loaded...");
}

bool Interface::IsAvailable(SClass_ID filetype) const
{
	return PluginMgr::Get()->IsAvailable( filetype );
}

WorldMap *Interface::GetWorldMap() const
{
	size_t index = worldmap->FindAndSetCurrentMap(game->CurrentArea);
	return worldmap->GetWorldMap(index);
}

WorldMap *Interface::GetWorldMap(const ResRef& area) const
{
	size_t index = worldmap->FindAndSetCurrentMap(area);
	return worldmap->GetWorldMap(index);
}

ProjectileServer* Interface::GetProjectileServer() const noexcept
{
	return projserv;
}

FogRenderer& Interface::GetFogRenderer()
{
	return *fogRenderer;
}

PluginHolder<Audio> Interface::GetAudioDrv() const
{
	return AudioDriver;
}

ieStrRef Interface::UpdateString(ieStrRef strref, const String& text) const
{
	String current = GetString(strref, STRING_FLAGS::NONE);
	if (current != text) {
		return strings->UpdateString( strref, text );
	} else {
		return strref;
	}
}

String Interface::GetString(ieStrRef strref, STRING_FLAGS options) const
{
	STRING_FLAGS flags = STRING_FLAGS::NONE;

	if (!(options & STRING_FLAGS::STRREFOFF)) {
		flags =
			static_cast<STRING_FLAGS>(
				vars.Get("Strref On", UnderType(STRING_FLAGS::NONE))
			);
	}
	
	if (core->HasFeature(GFFlags::ALL_STRINGS_TAGGED)) {
		//tagged text, bg1 and iwd don't mark them specifically, all entries are tagged
		options |= STRING_FLAGS::RESOLVE_TAGS;
	}

	if (strings2 && strref != ieStrRef::INVALID && bool(strref & ieStrRef::ALTREF)) {
		return strings2->GetString(strref, flags | options);
	} else {
		return strings->GetString(strref, flags | options);
	}
}

std::string Interface::GetMBString(ieStrRef strref, STRING_FLAGS options) const
{
	String string = GetString(strref, options);
	return MBStringFromString(string);
}

void Interface::SetFeature(GFFlags flag)
{
	GameFeatures[flag] = true;
}

void Interface::ClearFeature(GFFlags flag)
{
	GameFeatures[flag] = false;
}

bool Interface::HasFeature(GFFlags flag) const
{
	return GameFeatures[flag];
}

static const EnumArray<GFFlags, StringView> game_flags {
		"HasKaputz",          //0 GF_HAS_KAPUTZ
		"AllStringsTagged",   //1 GF_ALL_STRINGS_TAGGED
		"HasSongList",        //2 GF_HAS_SONGLIST
		"TeamMovement",       //3 GF_TEAM_MOVEMENT
		"UpperButtonText",    //4 GF_UPPER_BUTTON_TEXT
		"LowerLabelText",     //5 GF_LOWER_LABEL_TEXT
		"HasPartyIni",        //6 GF_HAS_PARTY_INI
		"SoundFolders",       //7 GF_SOUNDFOLDERS
		"IgnoreButtonFrames", //8 GF_IGNORE_BUTTON_FRAMES
		"OneByteAnimationID", //9 GF_ONE_BYTE_ANIMID
		"HasDPLAYER",         //10GF_HAS_DPLAYER
		"HasEXPTABLE",        //11GF_HAS_EXPTABLE
		"HasBeastsIni",       //12GF_HAS_BEASTS_INI
		"HasEEEffects",       //13GF_HAS_EE_EFFECTS
		"HasPickSound",       //14GF_HAS_PICK_SOUND
		"IWDMapDimensions",   //15GF_IWD_MAP_DIMENSIONS
		"AutomapINI",         //16GF_AUTOMAP_INI
		"SmallFog",           //17GF_SMALL_FOG
		"ReverseDoor",        //18GF_REVERSE_DOOR
		"ProtagonistTalks",   //19GF_PROTAGONIST_TALKS
		"HasSpellList",       //20GF_HAS_SPELLLIST
		"IWD2ScriptName",     //21GF_IWD2_SCRIPTNAME
		"DialogueScrolls",    //22GF_DIALOGUE_SCROLLS
		"KnowWorld",          //23GF_KNOW_WORLD
		"ReverseToHit",       //24GF_REVERSE_TOHIT
		"SaveForHalfDamage",  //25GF_SAVE_FOR_HALF
		"CharNameIsGabber",   //26GF_CHARNAMEISGABBER
		"MagicBit",           //27GF_MAGICBIT
		"CheckAbilities",     //28GF_CHECK_ABILITIES
		"ChallengeRating",    //29GF_CHALLENGERATING
		"SpellBookIconHack",  //30GF_SPELLBOOKICONHACK
		"EnhancedEffects",    //31GF_ENHANCED_EFFECTS
		"DeathOnZeroStat",    //32GF_DEATH_ON_ZERO_STAT
		"SpawnIni",           //33GF_SPAWN_INI
		"IWD2DeathVarFormat", //34GF_IWD2_DEATHVARFORMAT
		"HasResDataIni",      //35GF_RESDATA_INI
		"OverrideCursorPos",  //36GF_OVERRIDE_CURSORPOS
		"BreakableWeapons",   //37GF_BREAKABLE_WEAPONS
		"3EdRules",           //38GF_3ED_RULES
		"LevelslotPerClass",  //39GF_LEVELSLOT_PER_CLASS
		"SelectiveMagicRes",  //40GF_SELECTIVE_MAGIC_RES
		"HasHideInShadows",   //41GF_HAS_HIDE_IN_SHADOWS
		"AreaVisitedVar",     //42GF_AREA_VISITED_VAR
		"ProperBackstab",     //43GF_PROPER_BACKSTAB
		"OnScreenText",       //44GF_ONSCREEN_TEXT
		"HasSpecificDamageBonus", //45GF_SPECIFIC_DMG_BONUS
		"StrrefSaveGame",     //46GF_STRREF_SAVEGAME
		"SimplifiedDisruption",//47GF_SIMPLE_DISRUPTION
		"BiographyIsRes",     //48GF_BIOGRAPHY_RES
		"NoBiography",        //49GF_NO_BIOGRAPHY
		"StealIsAttack",      //50GF_STEAL_IS_ATTACK
		"CutsceneAreascripts",//51GF_CUTSCENE_AREASCRIPTS
		"FlexibleWorldmap",   //52GF_FLEXIBLE_WMAP
		"AutoSearchHidden",   //53GF_AUTOSEARCH_HIDDEN
		"PSTStateFlags",      //54GF_PST_STATE_FLAGS
		"NoDropCanMove",      //55GF_NO_DROP_CAN_MOVE
		"JournalHasSections", //56GF_JOURNAL_HAS_SECTIONS
		"CastingSounds",      //57GF_CASTING_SOUNDS
		"EnhancedCastingSounds", //58GF_CASTING_SOUNDS2
		"ForceAreaScript",    //59GF_FORCE_AREA_SCRIPT
		"AreaOverride",       //60GF_AREA_OVERRIDE
		"NoNewVariables",     //61GF_NO_NEW_VARIABLES
		"HasSoundsIni",       //62GF_SOUNDS_INI
		"HasNoNPCFlag",       //63GF_USEPOINT_400
		"HasUsePointFlag",    //64GF_USEPOINT_200
		"HasFloatMenu",       //65GF_HAS_FLOAT_MENU
		"NoUndroppable",      //67GF_NO_UNDROPPABLE
		"StartActive",        //68GF_START_ACTIVE
		"HasInfopointDialogs", //69GF_INFOPOINT_DIALOGS
		"ImplicitAreaAnimBackground", //70GF_IMPLICIT_AREAANIM_BACKGROUND
		"HealOn100Plus",      //71GF_HEAL_ON_100PLUS
		"InPartyAllowsDead",  //72GF_IN_PARTY_ALLOWS_DEAD
		"ZeroTimerIsValid",   //73GF_ZERO_TIMER_IS_VALID
		"ShopsRechargeItems", //74GF_SHOP_RECHARGE
		"MeleeHeaderUsesProjectile", //75GF_MELEEHEADER_USESPROJECTILE
		"ForceDialogPause",   //76GF_FORCE_DIALOGPAUSE
		"RandomBanterDialogs",//77GF_RANDOM_BANTER_DIALOGS
		"FixedMoraleOpcode",  //79GF_FIXED_MORALE_OPCODE
		"Happiness",          //80GF_HAPPINESS
		"EfficientORTrigger", //81GF_EFFICIENT_OR
		"LayeredWaterTiles",  //82GF_LAYERED_WATER_TILES
		"ClearingActionOverride", //83GF_CLEARING_ACTIONOVERRIDE
		"DamageInnocentRep",  //84GF_DAMAGE_INNOCENT_REP
		"HasWeaponSets" // GF_HAS_WEAPON_SETS
};

/** Loads gemrb.ini */
void Interface::LoadGemRBINI()
{
	DataStream* inifile = gamedata->GetResourceStream("gemrb", IE_INI_CLASS_ID);
	if (!inifile) {
		throw CIE("could not open gemrb.ini");
	}

	Log(MESSAGE, "Core", "Loading game type-specific GemRB setup '{}'",
		inifile->originalfile);

	PluginHolder<DataFileMgr> ini = MakePluginHolder<DataFileMgr>(IE_INI_CLASS_ID);
	ini->Open(inifile);

	ResRef tooltipBG;
	// Resrefs are already initialized in Interface::Interface()
#define ASSIGN_RESREF(resref, name) \
	resref = ini->GetKeyAsString("resources", name, "")

	ASSIGN_RESREF(MainCursorsImage, "MainCursorsImage");
	ASSIGN_RESREF(TextCursorBam, "TextCursorBAM"); //console cursor
	ASSIGN_RESREF(ScrollCursorBam, "ScrollCursorBAM");
	ASSIGN_RESREF(ButtonFontResRef, "ButtonFont");
	ASSIGN_RESREF(TooltipFontResRef, "TooltipFont");
	ASSIGN_RESREF(MovieFontResRef, "MovieFont");
	ASSIGN_RESREF(tooltipBG, "TooltipBack");
	ASSIGN_RESREF(TextFontResRef, "TextFont");
	ASSIGN_RESREF(Palette16, "Palette16");
	ASSIGN_RESREF(Palette32, "Palette32");
	ASSIGN_RESREF(Palette256, "Palette256");

#undef ASSIGN_RESREF

	//which stat determines the fist weapon (defaults to class)
	Actor::SetFistStat(ini->GetKeyAsInt( "resources", "FistStat", IE_CLASS));

	int ttMargin = ini->GetKeyAsInt( "resources", "TooltipMargin", 10 );

	if (!tooltipBG.IsEmpty()) {
		auto anim = gamedata->GetFactoryResourceAs<const AnimationFactory>(tooltipBG, IE_BAM_CLASS_ID);
		Log(MESSAGE, "Core", "Initializing Tooltips...");
		if (anim) {
			TooltipBG = new TooltipBackground(anim->GetFrame(0, 0), anim->GetFrame(0, 1), anim->GetFrame(0, 2) );
			// FIXME: this is an arbitrary heuristic and speed
			TooltipBG->SetAnimationSpeed((ttMargin == 5) ? 10 : 0);
			TooltipBG->SetMargin(ttMargin);
		}
	}

	// if unset, manually populate GameName (window title)
	if (config.GameName == GEMRB_STRING) {
		auto sv = ini->GetKeyAsString("resources", "WindowTitle", GEMRB_STRING);
		if (sv.empty()) {
			config.GameName += " running unknown game";
		} else {
			config.GameName += " running " + StringFromView<std::string>(sv);
		}
	}

	// These are values for how long a single step is, see Movable::DoStep.
	// They were found via trial-and-error, trying to match
	// the speeds from the original games.
	gamedata->SetStepTime(ini->GetKeyAsInt("resources", "StepTime", 566)); // Defaults to BG2's value

	// The format of GroundCircle can be:
	// GroundCircleBAM1 = wmpickl/3
	// to denote that the bitmap should be scaled down 3x
	for (int size = 0; size < MAX_CIRCLE_SIZE; size++) {
		const std::string& name =  fmt::format("GroundCircleBAM{}", size + 1);
		auto sv = ini->GetKeyAsString("resources", name);
		if (sv) {
			size_t pos = FindFirstOf(sv, "/");
			if (pos != StringView::npos) {
				GroundCircleScale[size] = atoi(&sv[pos] + 1);
				GroundCircleBam[size] = ResRef(sv.begin(), ResRef::size_type(pos));
			} else {
				GroundCircleBam[size] = sv;
			}
		}
	}

	auto sv = ini->GetKeyAsString("resources", "INIConfig");
	if (sv)
		INIConfig = StringFromView<std::string>(sv);

	MaximumAbility = ini->GetKeyAsInt ("resources", "MaximumAbility", 25 );
	gamedata->SetTextSpeed(ini->GetKeyAsInt("resources", "TextScreenSpeed", 100));

	for (const GFFlags flag : EnumIterator<GFFlags>()) {
		const bool set = ini->GetKeyAsBool("resources", game_flags[flag], false);
		if (set) {
			SetFeature(flag);
		} else {
			ClearFeature(flag);
		}
	}

	// fix the resolution default if needed
	config.Width = std::max(config.Width, ini->GetKeyAsInt("resources", "MinWidth", 800));
	config.Height = std::max(config.Height, ini->GetKeyAsInt("resources", "MinHeight", 600));
}

/** Load the encoding table selected in gemrb.cfg */
bool Interface::LoadEncoding()
{
	DataStream* inifile = gamedata->GetResourceStream(config.Encoding, IE_INI_CLASS_ID);
	if (! inifile) {
		return false;
	}

	Log(MESSAGE, "Core", "Loading encoding definition for {}: '{}'", config.Encoding,
		inifile->originalfile);

	PluginHolder<DataFileMgr> ini = MakePluginHolder<DataFileMgr>(IE_INI_CLASS_ID);
	ini->Open(inifile);

	TLKEncoding.encoding = StringFromView<std::string>(ini->GetKeyAsString("encoding", "TLKEncoding", TLKEncoding.encoding));
	TLKEncoding.zerospace = ini->GetKeyAsBool("encoding", "NoSpaces", false);

	// TODO: lists are incomplete
	// maybe want to externalize this
	// list compiled form wiki: https://gemrb.org/Text-encodings.html
	std::vector<std::string> wideEncodings = {
		// Chinese
		"GBK", "BIG5",
		// Korean
		"EUCKR",
		// Japanese
		"SJIS",
	};

	for (const auto& encoding : wideEncodings) {
		if (TLKEncoding.encoding == encoding) {
			TLKEncoding.widechar = true;
			break;
		}
	}

	std::vector<std::string> multibyteEncodings = {
		"UTF-8",
	};

	for (const auto& encoding : multibyteEncodings) {
		if (TLKEncoding.encoding == encoding) {
			TLKEncoding.multibyte = true;
			break;
		}
	}

	return true;
}

/** Returns a preloaded Font */
Holder<Font> Interface::GetFont(const ResRef& ResRef) const
{
	auto i = fonts.find(ResRef);
	if (i != fonts.end()) {
		return i->second;
	}
	return nullptr;
}

Holder<Font> Interface::GetTextFont() const
{
	return GetFont( TextFontResRef );
}

Holder<Font> Interface::GetButtonFont() const
{
	return GetFont( ButtonFontResRef );
}

/** Get GUI Script Manager */
PluginHolder<ScriptEngine> Interface::GetGUIScriptEngine() const
{
	return guiscript;
}

//NOTE: if there were more summoned creatures, it will return only the last
Actor *Interface::SummonCreature(const ResRef& resource, const ResRef& animRes, Scriptable *Owner, const Actor *target, const Point &position, int eamod, int level, Effect *fx, bool sexmod)
{
	static EffectRef fx_summon_disable_ref = { "AvatarRemovalModifier", -1 };
	//maximum number of monsters summoned
	int cnt=10;
	Actor * ab = NULL;
	const Actor *summoner = nullptr;

	Map *map;
	if (target) {
		map = target->GetCurrentArea();
	} else if (Owner) {
		map = Owner->GetCurrentArea();
	} else {
		map = game->GetCurrentArea();
	}
	if (!map) {
		delete fx;
		return nullptr;
	}

	summoner = Scriptable::As<Actor>(Owner);

	while (cnt--) {
		Actor *tmp = gamedata->GetCreature(resource);
		if (!tmp) {
			ab = nullptr;
			break;
		}

		//if summoner is an actor, filter out opponent summons
		//this is done by clearing the filter when appropriate
		//non actors and neutrals can summon as much as they want
		ieDword flag = GA_NO_DEAD|GA_NO_ALLY|GA_NO_ENEMY;

		if (summoner) {
			tmp->objects.LastSummoner = Owner->GetGlobalID();
			ieDword ea = summoner->GetStat(IE_EA);
			if (ea<=EA_GOODCUTOFF) {
				flag &= ~GA_NO_ALLY;
			} else if (ea>=EA_EVILCUTOFF) {
				flag &= ~GA_NO_ENEMY;
			}
		}

		// mark the summon, but only if they don't have a special sex already
		if (sexmod && tmp->BaseStats[IE_SEX] < SEX_EXTRA && tmp->BaseStats[IE_SEX] != SEX_ILLUSION) {
			tmp->SetBase(IE_SEX, SEX_SUMMON);
		}

		// only allow up to the summoning limit of new summoned creatures
		// the summoned creatures have a special IE_SEX
		// but also only limit the party, so spore colonies can reproduce more
		ieDword sex = tmp->GetStat(IE_SEX);
		int limit = gamedata->GetSummoningLimit(sex);
		if (limit && sexmod && map->CountSummons(flag, sex) >= limit && summoner && summoner->InParty) {
			//summoning limit reached
			displaymsg->DisplayConstantString(HCStrings::SummoningLimit, GUIColors::WHITE);
			delete tmp;
			break;
		}

		ab = tmp;
		//Always use Base stats for the recently summoned creature

		bool enemyally = true;
		if (summoner && (eamod == EAM_SOURCEALLY || eamod == EAM_SOURCEENEMY)) {
			enemyally = summoner->GetStat(IE_EA) > EA_GOODCUTOFF;
		} else if (target) {
			enemyally = target->GetBase(IE_EA) > EA_GOODCUTOFF;
		}

		switch (eamod) {
		case EAM_SOURCEALLY:
		case EAM_ALLY:
			if (enemyally) {
				ab->SetBase(IE_EA, EA_ENEMY); //is this the summoned EA?
			} else {
				ab->SetBase(IE_EA, EA_CONTROLLED); //is this the summoned EA?
			}
			break;
		case EAM_SOURCEENEMY:
		case EAM_ENEMY:
			if (enemyally) {
				ab->SetBase(IE_EA, EA_CONTROLLED); //is this the summoned EA?
			} else {
				ab->SetBase(IE_EA, EA_ENEMY); //is this the summoned EA?
			}
			break;
		case EAM_NEUTRAL:
			ab->SetBase(IE_EA, EA_NEUTRAL);
			break;
		default:
			break;
		}

		map->AddActor(ab, true);
		ab->SetPosition(position, true, Size(), ab->circleSize);
		ab->RefreshEffects();

		// guessing, since this trigger was unused in the originals — likely duplicating LastSummoner
		if (Owner) {
			Owner->AddTrigger(TriggerEntry(trigger_summoned, ab->GetGlobalID()));
		}

		if (!animRes.IsEmpty()) {
			ScriptedAnimation* vvc = gamedata->GetScriptedAnimation(animRes, false);
			if (vvc) {
				//This is the final position of the summoned creature
				//not the original target point
				vvc->Pos = ab->Pos;
				//force vvc to play only once
				vvc->PlayOnce();
				map->AddVVCell( new VEFObject(vvc) );

				//set up the summon disable effect
				Effect *newfx = EffectQueue::CreateEffect(fx_summon_disable_ref, 0, 1, FX_DURATION_ABSOLUTE);
				if (newfx) {
					newfx->Duration = vvc->GetSequenceDuration(Time.defaultTicksPerSec) * 9 / 10 + core->GetGame()->GameTime;
					ApplyEffect(newfx, ab, ab);
				}
			}
		}

		//remove the xp value of friendly summons
		if (ab->BaseStats[IE_EA]<EA_GOODCUTOFF) {
			ab->SetBase(IE_XPVALUE, 0);
		}
		if (fx) {
			ApplyEffect(new Effect(*fx), ab, Owner);
		}

		//this check should happen after the fact
		// FIXME: shouldn't this be IE_XPVALUE, the CR? IE_XP holds the actual xp gained, only useful for pcs
		level -= ab->GetBase(IE_XP);
		if(level<0 || ab->GetBase(IE_XP) == 0) {
			break;
		}

	}
	
	delete fx;
	return ab;
}

inline void SetGroupViewFlags(const std::vector<View*>& views, unsigned int flags, BitOp op)
{
	std::vector<View*>::const_iterator it = views.begin();
	for (; it != views.end(); ++it) {
		View* view = *it;
		view->SetFlags(flags, op);
	}
}

void Interface::ToggleViewsVisible(bool visible, const ScriptingGroup_t& group)
{
	if (game && group == "HIDE_CUT") {
		game->SetControlStatus(CS_HIDEGUI, visible ? BitOp::NAND : BitOp::OR);
	}

	std::vector<View*> views = GetViews(group);
	SetGroupViewFlags(views, View::Invisible, visible ? BitOp::NAND : BitOp::OR);
}

void Interface::ToggleViewsEnabled(bool enabled, const ScriptingGroup_t& group) const
{
	std::vector<View*> views = GetViews(group);
	SetGroupViewFlags(views, View::Disabled, enabled ? BitOp::NAND : BitOp::OR);
}

void Interface::LoadInitialValues(const ResRef& name, ieVarsMap& map) const
{
	// we only support PST's var.var for now
	path_t nPath = PathJoin(config.GamePath, "var.var");
	FileStream fs;
	if (!fs.Open(nPath)) {
		return;
	}

	char buffer[41];
	ieDword value;
	buffer[40] = 0;
	ieVariable varname;

	// first value is useless
	if (!fs.Read(buffer, 40)) return;
	if (fs.ReadDword(value) != 4) return;

	while (fs.Remains()) {
		// read data
		if (!fs.Read(buffer, 40)) return;
		if (fs.ReadDword(value) != 4) return;
		// is it the type we want? if not, skip
		if (!name.BeginsWith(StringView(buffer, 6))) continue;
		// copy variable (types got 2 extra spaces, and the name is padded too)
		// (true = uppercase, needed for original engine save compat, see 315b8f2e)
		varname = MakeVariable(StringView(buffer + 8, 32));
		StringToUpper(varname);
		map[varname] = value;
	}
}

bool Interface::IsFreezed() const
{
	return !update_scripts;
}

void Interface::GameLoop(void)
{
	update_scripts = false;
	GameControl *gc = GetGameControl();
	if (gc) {
		update_scripts = !(gc->GetDialogueFlags() & DF_FREEZE_SCRIPTS);
	}

	bool do_update = GSUpdate(update_scripts);

	if (game) {
		if (gc && !game->selected.empty()) {
			gc->ChangeMap(GetFirstSelectedPC(true), false);
		}
		//in multi player (if we ever get to it), only the server must call this
		if (do_update) {
			// the game object will run the area scripts as well
			game->UpdateScripts();
		}
	}
}

/** handles hardcoded gui behaviour */
void Interface::HandleGUIBehaviour(GameControl* gc)
{
	//this variable is used all over in the following hacks
	int flg = gc->GetDialogueFlags();

	//the following part is a series of hardcoded gui behaviour

	//initiating dialog
	if (flg & DF_IN_DIALOG) {
		// -3 noaction
		// -2 close
		// -1 open
		// choose option
		ieDword var = vars.Get("DialogChoose", -3);

		if ((int) var == -2) {
			// TODO: this seems to never be called? (EndDialog is called from elsewhere instead)
			gc->dialoghandler->EndDialog();
		} else if ( (int)var !=-3) {
			if ( (int) var == -1) {
				guiscript->RunFunction( "GUIWORLD", "DialogStarted" );
			}
			gc->dialoghandler->DialogChoose(var);
			if (!(gc->GetDialogueFlags() & (DF_OPENCONTINUEWINDOW | DF_OPENENDWINDOW)))
				guiscript->RunFunction( "GUIWORLD", "NextDialogState" );

			// the last node of a dialog can have a new-dialog action! don't interfere in that case
			ieDword newvar = vars.Get("DialogChoose", 0);

			if (var == (ieDword) -1 || newvar != (ieDword) -1) {
				vars.Set("DialogChoose", -3);
			}
		}
		if (flg & DF_OPENCONTINUEWINDOW) {
			guiscript->RunFunction( "GUIWORLD", "OpenContinueMessageWindow" );
			gc->SetDialogueFlags(DF_OPENCONTINUEWINDOW|DF_OPENENDWINDOW, BitOp::NAND);
		} else if (flg & DF_OPENENDWINDOW) {
			guiscript->RunFunction( "GUIWORLD", "OpenEndMessageWindow" );
			gc->SetDialogueFlags(DF_OPENCONTINUEWINDOW|DF_OPENENDWINDOW, BitOp::NAND);
		}
	}

	//handling container
	if (CurrentContainer && UseContainer) {
		if (!(flg & DF_IN_CONTAINER) ) {
			gc->SetDialogueFlags(DF_IN_CONTAINER, BitOp::OR);
			guiscript->RunFunction( "Container", "OpenContainerWindow" );
		}
	} else {
		if (flg & DF_IN_CONTAINER) {
			gc->SetDialogueFlags(DF_IN_CONTAINER, BitOp::NAND);
			guiscript->RunFunction( "Container", "CloseContainerWindow" );
		}
	}
	//end of gui hacks
}

Tooltip Interface::CreateTooltip() const
{
	Font::PrintColors colors;
	colors.fg = displaymsg->GetColor(GUIColors::TOOLTIP);
	colors.bg = displaymsg->GetColor(GUIColors::TOOLTIPBG);

	TooltipBackground* bg = NULL;
	if (TooltipBG) {
		bg = new TooltipBackground(*TooltipBG);
	}
	return Tooltip(u"", GetFont(TooltipFontResRef), colors, bg);
}

/** Get the Save game manager */
SaveGameIterator* Interface::GetSaveGameIterator() const
{
	return sgiterator;
}

void Interface::AskAndExit()
{
	// if askExit is 1 then we are trying to quit a second time and should instantly do so
	ieDword askExit = vars.Get("AskAndExit", 0);

	if (game && !askExit) {
		SetPause(PauseState::On);
		vars.Set("AskAndExit", 1);

		guiscript->RunFunction("GUIOPT", "OpenQuitMsgWindow");
		Log(MESSAGE, "Info", "Press ctrl-c (or close the window) again to quit GemRB.\n");
	} else {
		QuitFlag |= QF_EXITGAME;
	}
}

/** Returns the variables dictionary */
variables_t& Interface::GetDictionary() {
	return vars;
}

/** Returns the token dictionary */
Interface::tokens_t& Interface::GetTokenDictionary() {
	return tokens;
}

const String& Interface::GetToken(const ieVariable& key, const String& fallback) const {
	auto lookup = tokens.find(key);
	if (lookup != tokens.cend()) {
		return lookup->second;
	}

	return fallback;
}

/** Get the Music Manager */
PluginHolder<MusicMgr> Interface::GetMusicMgr() const
{
	return music;
}
/** Loads an IDS Table, returns -1 on error or the Symbol Table Index on success */
int Interface::LoadSymbol(const ResRef& ref)
{
	int ind = GetSymbolIndex(ref);
	if (ind != -1) {
		return ind;
	}
	DataStream* str = gamedata->GetResourceStream(ref, IE_IDS_CLASS_ID);
	if (!str) {
		return -1;
	}
	PluginHolder<SymbolMgr> sm = MakePluginHolder<SymbolMgr>(IE_IDS_CLASS_ID);
	if (!sm) {
		delete str;
		return -1;
	}
	if (!sm->Open(str)) {
		return -1;
	}
	Symbol s = { sm, ref };
	ind = -1;
	for (size_t i = 0; i < symbols.size(); i++) {
		if (!symbols[i].sm) {
			ind = ( int ) i;
			break;
		}
	}
	if (ind != -1) {
		symbols[ind] = s;
		return ind;
	}
	symbols.push_back( s );
	return ( int ) symbols.size() - 1;
}
/** Gets the index of a loaded Symbol Table, returns -1 on error */
int Interface::GetSymbolIndex(const ResRef& ref) const
{
	for (size_t i = 0; i < symbols.size(); i++) {
		if (!symbols[i].sm)
			continue;
		if (symbols[i].symbolName == ref)
			return ( int ) i;
	}
	return -1;
}
/** Gets a Loaded Symbol Table by its index, returns NULL on error */
PluginHolder<SymbolMgr> Interface::GetSymbol(unsigned int index) const
{
	if (index >= symbols.size()) {
		return {};
	}
	if (!symbols[index].sm) {
		return {};
	}
	return symbols[index].sm;
}
/** Frees a Loaded Symbol Table, returns false on error, true on success */
bool Interface::DelSymbol(unsigned int index)
{
	if (index >= symbols.size()) {
		return false;
	}
	if (!symbols[index].sm) {
		return false;
	}
	symbols[index].sm.reset();
	return true;
}
/** Plays a Movie */
int Interface::PlayMovie(const ResRef& movieRef)
{
	if (IsStar(movieRef)) return -1;
	ResRef actualMovieRef = movieRef;

	//check whether there is an override for this movie
	std::string sound_resref;
	AutoTable mvesnd = gamedata->LoadTable("mvesnd", true);
	if (mvesnd) {
		TableMgr::index_t row = mvesnd->GetRowIndex(movieRef);
		if (row != TableMgr::npos) {
			TableMgr::index_t mvecol = mvesnd->GetColumnIndex("override");
			if (mvecol != TableMgr::npos) {
				actualMovieRef = mvesnd->QueryField(row, mvecol);
			}
			TableMgr::index_t sndcol = mvesnd->GetColumnIndex("sound_override");
			if (sndcol != TableMgr::npos) {
				sound_resref = mvesnd->QueryField(row, sndcol);
			}
		}
	}

	//shutting down music and ambients before movie
	if (music)
		music->HardEnd();
	AmbientMgr *ambim = AudioDriver->GetAmbientMgr();
	if (ambim) ambim->Deactivate();

	ResourceHolder<MoviePlayer> mp = gamedata->GetResourceHolder<MoviePlayer>(actualMovieRef);
	if (!mp) {
		return -1;
	}

	//one of these two should exist (they both mean the same thing)
	ieDword subtitles = vars.Get("Dialog Movie Subtitles", 1);
	if (!subtitles) {
		subtitles = vars.Get("Dialog Subtitles", 0);
	}

	mp->EnableSubtitles(subtitles);

	class IESubtitles : public MoviePlayer::SubtitleSet {
		using FrameMap = std::map<size_t, ieStrRef>;
		FrameMap subs;
		mutable size_t nextSubFrame = 0;
		mutable String cachedSub;

	public:
		// default color taken from BGEE.lua
		IESubtitles(Holder<Font> fnt, const AutoTable& sttable, const Color& col = Color(0xe9, 0xe2, 0xca, 0xff))
		: MoviePlayer::SubtitleSet(fnt, col)
		{
			for (TableMgr::index_t i = 0; i < sttable->GetRowCount(); ++i) {
				const auto& rowName = sttable->GetRowName(i);
				if (!std::isdigit(rowName[0])) continue; // this skips the initial palette rows (red, blue, green)

				int frameField = sttable->QueryFieldSigned<int>(i, 0);
				ieStrRef strField = sttable->QueryFieldAsStrRef(i, 1);
				subs[frameField] = strField;
			}
		}

		size_t NextSubtitleFrame() const override {
			return nextSubFrame;
		}

		const String& SubtitleAtFrame(size_t frameNum) const override {
			// FIXME: we cant go backwards now... we dont need to, but this is ugly
			if (frameNum >= NextSubtitleFrame()) {
				FrameMap::const_iterator it;
				it = subs.upper_bound(frameNum);
				nextSubFrame = it->first;
				if (it != subs.begin()) {
					--it;
				}
				cachedSub = core->GetString(it->second);
			}

			return cachedSub;
		}
	};

	AutoTable sttable = gamedata->LoadTable(movieRef);
	auto font = GetFont(MovieFontResRef);
	if (sttable && font) {
		int r = sttable->QueryFieldSigned<int>("red", "frame");
		int g = sttable->QueryFieldSigned<int>("green", "frame");
		int b = sttable->QueryFieldSigned<int>("blue", "frame");

		if (r || g || b) {
			mp->SetSubtitles(new IESubtitles(font, sttable, Color(r, g, b, 0xff)));
		} else {
			mp->SetSubtitles(new IESubtitles(font, sttable));
		}
	}

	Holder<SoundHandle> sound_override;
	if (!sound_resref.empty()) {
		sound_override = AudioDriver->PlayRelative(sound_resref, SFX_CHAN_NARRATOR);
	}

	// clear whatever is currently on screen
	SetCutSceneMode(true);
	Region screen(0, 0, config.Width, config.Height);
	Window* win = winmgr->MakeWindow(screen);
	win->SetFlags(Window::Borderless|Window::NoSounds, BitOp::OR);
	winmgr->PresentModalWindow(win);
	WindowManager::CursorFeedback cur = winmgr->SetCursorFeedback(WindowManager::MOUSE_NONE);
	winmgr->DrawWindows();

	mp->Play(win);
	win->Close();
	winmgr->SetCursorFeedback(cur);
	SetCutSceneMode(false);
	if (sound_override) {
		sound_override->Stop();
		sound_override.reset();
	}

	//restarting music
	if (music)
		music->Start();
	if (ambim) ambim->Activate();

	//Setting the movie name to 1
	vars.Set(movieRef, 1);
	return 0;
}

int Interface::Roll(int dice, int size, int add) const
{
	if (dice < 1) {
		return add;
	}
	if (size < 1) {
		return add;
	}
	if (dice > 100) {
		return add + dice * size / 2;
	}
	for (int i = 0; i < dice; i++) {
		add += RAND(1, size);
	}
	return add;
}

DirectoryIterator Interface::GetResourceDirectory(RESOURCE_DIRECTORY dir) const
{
	path_t resourcePath;
	DirectoryIterator::FileFilterPredicate filter;
	switch (dir) {
		case DIRECTORY_CHR_PORTRAITS:
			resourcePath = config.GamePortraitsPath;
			filter = std::make_shared<ExtFilter>("BMP");
			if (IsAvailable(IE_PNG_CLASS_ID)) {
				// chain an ORed filter for png
				filter = std::make_shared<OrPredicate<path_t>>(filter, std::make_shared<ExtFilter>("PNG"));
			}
			break;
		case DIRECTORY_CHR_SOUNDS:
			resourcePath = config.GameSoundsPath;
			if (!HasFeature( GFFlags::SOUNDFOLDERS ))
				filter = std::make_shared<ExtFilter>("WAV");
			break;
		case DIRECTORY_CHR_EXPORTS:
			resourcePath = config.GameCharactersPath;
			filter = std::make_shared<ExtFilter>("CHR");
			break;
		case DIRECTORY_CHR_SCRIPTS:
			resourcePath = config.GameScriptsPath;
			filter = std::make_shared<ExtFilter>("BS");
			filter = std::make_shared<OrPredicate<path_t>>(filter, std::make_shared<ExtFilter>("BCS"));
			break;
		default:
			error("Interface", "Unknown resource directory type: {}!", dir);
	}

	DirectoryIterator dirIt(PathJoin(config.GamePath, resourcePath));
	dirIt.SetFilterPredicate(filter);
	return dirIt;
}

bool Interface::InitializeVarsWithINI(const path_t& iniFileName)
{
	if (!core->IsAvailable( IE_INI_CLASS_ID ))
		return false;

	PluginHolder<DataFileMgr> defaults = nullptr;
	PluginHolder<DataFileMgr>  overrides = nullptr;

	PluginHolder<DataFileMgr> ini = MakePluginHolder<DataFileMgr>(IE_INI_CLASS_ID);
	FileStream* iniStream = FileStream::OpenFile(iniFileName);
	// if filename is not set we assume we are creating defaults without an INI
	bool opened = ini->Open(iniStream);
	if (iniFileName[0] && !opened) {
		Log(WARNING, "Core", "Unable to read defaults from '{}'. Using GemRB default values.", iniFileName);
	} else {
		overrides = ini;
	}
	if (!opened || iniFileName[0] == 0) {
		delete iniStream; // Open deletes it itself on success
	}

	PluginHolder<DataFileMgr> gemINI = MakePluginHolder<DataFileMgr>(IE_INI_CLASS_ID);
	DataStream* gemINIStream = gamedata->GetResourceStream("defaults", IE_INI_CLASS_ID);

	if (!gemINIStream || !gemINI->Open(gemINIStream)) {
		Log(WARNING, "Core", "Unable to load GemRB default values.");
		defaults = ini;
	} else {
		defaults = gemINI;
	}
	if (!overrides) {
		overrides = defaults;
	}

	for (int i = 0; i < defaults->GetTagsCount(); i++) {
		const StringView tag = defaults->GetTagNameByIndex(i);
		for (int j = 0; j < defaults->GetKeysCount(tag); j++) {
			auto key = std::string{defaults->GetKeyNameByIndex(tag, j).c_str()};
			//skip any existing entries. GemRB.cfg has priority
			auto lookup = vars.Get(key);
			if (lookup == nullptr) {
				ieDword defaultVal = defaults->GetKeyAsInt(tag, key, 0);
				vars.Set(key, overrides->GetKeyAsInt(tag, key, defaultVal));
			}
		}
	}

	// handle a few special cases
	if (!overrides->GetKeyAsInt("Config", "Sound", 1))
		config.AudioDriverName = "null";

	if (overrides->GetKeyAsInt("Game Options", "Cheats", 1)) {
		EnableCheatKeys(1);
	}

	// copies
	if (!overrides->GetKeyAsInt("Game Options", "Darkvision", 1)) {
		vars.Set("Infravision", 0);
	}

	if (!config.Width || !config.Height) {
		config.Height = overrides->GetKeyAsInt("Config", "ConfigHeight", config.Height);
		int tmpWidth = overrides->GetKeyAsInt("Config", "ConfigWidth", 0);
		if (!tmpWidth) {
			// Resolution is stored as width only. assume 4|3 ratio.
			config.Width = overrides->GetKeyAsInt("Program Options", "Resolution", config.Width);
			config.Height = 0.75 * config.Width;
		}
	}
	return true;
}

/** Saves the gemrb config variables from the whitelist to gem-INIConfig
 *  If GamePath is not writable, it tries SavePath
 */
bool Interface::SaveConfig()
{
	path_t gemrbINI;
	if (INIConfig.compare(0, 4, "gem-") != 0) {
		gemrbINI = "gem-" + INIConfig;
	}
	path_t ini_path = PathJoin(config.GamePath, gemrbINI);
	FileStream *fs = new FileStream();
	if (!fs->Create(ini_path)) {
		ini_path = PathJoin(config.SavePath, gemrbINI);
		if (!fs->Create(ini_path)) {
			delete fs;
			return false;
		}
	}

	PluginHolder<DataFileMgr> defaultsINI = MakePluginHolder<DataFileMgr>(IE_INI_CLASS_ID);
	DataStream* INIStream = gamedata->GetResourceStream("defaults", IE_INI_CLASS_ID);

	if (INIStream && defaultsINI->Open(INIStream)) {
		// dump the formatted default config options to the file
		std::string contents;
		for (int i = 0; i < defaultsINI->GetTagsCount(); i++) {
			const StringView tag = defaultsINI->GetTagNameByIndex(i);
			// write section header
			AppendFormat(contents, "[{}]\n", tag);
			for (int j = 0; j < defaultsINI->GetKeysCount(tag); j++) {
				auto key = std::string{defaultsINI->GetKeyNameByIndex(tag, j).c_str()};
				auto lookup = vars.Get(key);
				assert(lookup != nullptr);
				AppendFormat(contents, "{} = {}\n", key, *lookup);
			}
		}

		fs->Write(contents.c_str(), contents.size());
	} else {
		Log(ERROR, "Core", "Unable to open GemRB defaults. Cannot determine what values to write to {}.", ini_path);
	}

	delete fs;
	return true;
}

// this is more of a workaround than anything else
// needed for cases where the script runner is gone before finishing his queue
void Interface::SetCutSceneRunner(Scriptable *runner) {
	CutSceneRunner = runner;
}

/** Enables/Disables the Cut Scene Mode */
void Interface::SetCutSceneMode(bool active)
{
	GameControl *gc = GetGameControl();
	if (gc) {
		gc->SetCutSceneMode( active );
	}

	ToggleViewsVisible(!active, "HIDE_CUT");

	if (active) {
		GetGUIScriptEngine()->RunFunction("GUICommonWindows", "CloseTopWindow");
	} else {
		SetCutSceneRunner(NULL);
	}
}

/** returns true if in dialogue or cutscene */
bool Interface::InCutSceneMode() const
{
	const GameControl *gc = GetGameControl();
	if (!gc || gc->InDialog() || (gc->GetScreenFlags() & SF_CUTSCENE)) {
		return true;
	}
	return false;
}

/** Updates the Game Script Engine State */
bool Interface::GSUpdate(bool update)
{
	if (update) {
		return timer.Update();
	} else {
		timer.Freeze();
		return false;
	}
}

void Interface::QuitGame(int BackToMain)
{
	SetCutSceneMode(false);

	//shutting down ingame music
	//(do it before deleting the game)
	if (music) {
		music->HardEnd();
	}
	// stop any ambients which are still enqueued
	if (AudioDriver) {
		AmbientMgr *ambim = AudioDriver->GetAmbientMgr();
		if (ambim) ambim->Deactivate();
		AudioDriver->Stop(); // also kill sounds
	}
	//delete game, worldmap
	if (game) {
		delete game;
		game=NULL;
	}
	if (worldmap) {
		delete worldmap;
		worldmap=NULL;
	}
	if (BackToMain) {
		SetNextScript("Start");
	}
	GSUpdate(true);
}

void Interface::SetupLoadGame(Holder<SaveGame> sg, int ver_override)
{
	LoadGameIndex = std::move(sg);
	VersionOverride = ver_override;
	QuitFlag |= QF_LOADGAME;
}

void Interface::LoadGame(Holder<SaveGame> sg, int ver_override)
{
	// This function has rather painful error handling,
	// as it should swap all the objects or none at all
	// and the loading can fail for various reasons

	// Yes, it uses goto. Other ways seemed too awkward for me.

	gamedata->SaveAllStores();
	strings->CloseAux();
	tokens.clear(); //clearing the token dictionary

	if(calendar) delete calendar;
	calendar = new Calendar;

	DataStream* gamStr = nullptr;
	DataStream* savStr = nullptr;
	DataStream* wmpStr1 = nullptr;
	DataStream* wmpStr2 = nullptr;

	Game* newGame = nullptr;
	WorldMapArray* newWorldmap = nullptr;

	LoadProgress(10);
	if (!config.KeepCache) DelTree(config.CachePath, true);
	LoadProgress(15);

	saveGameAREExtractor = SaveGameAREExtractor(sg);

	// These are here because of the goto
	PluginHolder<SaveGameMgr> gamMgr;
	PluginHolder<WorldMapMgr> wmpMgr = MakePluginHolder<WorldMapMgr>(IE_WMP_CLASS_ID);
	AmbientMgr* ambim = core->GetAudioDrv()->GetAmbientMgr();

	if (sg == nullptr) {
		//Load the Default Game
		gamStr = gamedata->GetResourceStream(GameNameResRef, IE_GAM_CLASS_ID);
		savStr = nullptr;
		wmpStr1 = gamedata->GetResourceStream(WorldMapName[0], IE_WMP_CLASS_ID);
		if (!WorldMapName[1].IsEmpty()) {
			wmpStr2 = gamedata->GetResourceStream(WorldMapName[1], IE_WMP_CLASS_ID);
		}
	} else {
		gamStr = sg->GetGame();
		savStr = sg->GetSave();

		// extract the save early for ees, since they moved the wmp inside
		if (core->HasFeature(GFFlags::HAS_EE_EFFECTS)) {
			PluginHolder<ArchiveImporter> ai = MakePluginHolder<ArchiveImporter>(IE_SAV_CLASS_ID);
			if (ai && ai->DecompressSaveGame(savStr, saveGameAREExtractor) != GEM_OK) {
				goto cleanup;
			}
			delete savStr;
			savStr = nullptr;
			wmpStr1 = gamedata->GetResourceStream(WorldMapName[0], IE_WMP_CLASS_ID, true);
		} else {
			wmpStr1 = sg->GetWmap(0);
		}

		if (!WorldMapName[1].IsEmpty()) { // TODO: ee
			wmpStr2 = sg->GetWmap(1);
			if (!wmpStr2) {
				//upgrade an IWD game to HOW
				wmpStr2 = gamedata->GetResourceStream(WorldMapName[1], IE_WMP_CLASS_ID);
			}
		}
	}

	if (!gamStr || !(wmpStr1 || wmpStr2)) {
		goto cleanup;
	}

	// Load GAM file
	gamMgr = GetImporter<SaveGameMgr>(IE_GAM_CLASS_ID, gamStr);
	if (!gamMgr) goto cleanup;

	newGame = gamMgr->LoadGame(new Game(), ver_override);
	if (!newGame) goto cleanup;
	UpdateActorConfig(); // so things that require Game can properly rerun

	// Load WMP (WorldMap) file
	if (!wmpMgr) goto cleanup;
	if (!wmpMgr->Open(wmpStr1, wmpStr2)) goto cleanup;

	newWorldmap = wmpMgr->GetWorldMapArray();
	wmpStr1 = nullptr;
	wmpStr2 = nullptr;

	LoadProgress(20);
	// Unpack SAV (archive) file to Cache dir, if we haven't done it above
	if (savStr) {
		PluginHolder<ArchiveImporter> ai = MakePluginHolder<ArchiveImporter>(IE_SAV_CLASS_ID);
		if (ai && ai->DecompressSaveGame(savStr, saveGameAREExtractor) != GEM_OK) {
			goto cleanup;
		}
		delete savStr;
		savStr = nullptr;
	}

	// rarely caused crashes while loading, so stop the ambients
	if (ambim) {
		ambim->Reset();
	}

	// Let's assume that now is everything loaded OK and swap the objects
	delete game;
	delete worldmap;

	game = newGame;
	worldmap = newWorldmap;

	strings->OpenAux();
	LoadProgress(70);
	return;
cleanup:
	// Something went wrong, so try to clean after itself
	delete newGame;
	delete newWorldmap;
	delete wmpStr1;
	delete wmpStr2;
	delete savStr;

	error("Core", "Unable to load game.");
}

/* replace the current world map but sync areas available in old and new */
void Interface::UpdateWorldMap(const ResRef& wmResRef)
{
	DataStream* wmp_str = gamedata->GetResourceStream(wmResRef, IE_WMP_CLASS_ID);
	PluginHolder<WorldMapMgr> wmp_mgr = MakePluginHolder<WorldMapMgr>(IE_WMP_CLASS_ID);

	if (!wmp_str || !wmp_mgr || !wmp_mgr->Open(wmp_str, NULL)) {
		Log(ERROR, "Core", "Could not update world map {}", wmResRef);
		return;
	}

	WorldMapArray *new_worldmap = wmp_mgr->GetWorldMapArray();
	WorldMap *wm = worldmap->GetWorldMap(0);
	WorldMap *nwm = new_worldmap->GetWorldMap(0);

	unsigned int ec = wm->GetEntryCount();
	//update status of the previously existing areas
	for (unsigned int i = 0; i < ec; i++) {
		const WMPAreaEntry *ae = wm->GetEntry(i);
		WMPAreaEntry* nae = nwm->GetArea(ae->AreaResRef);
		if (nae) {
			nae->SetAreaStatus(ae->GetAreaStatus(), BitOp::SET);
		}
	}

	delete worldmap;
	worldmap = new_worldmap;
	WorldMapName[0] = wmResRef;
}

/* swapping out old resources */
void Interface::UpdateMasterScript()
{
	if (game) {
		game->SetScript( GlobalScript, 0 );
	}

	PluginHolder<WorldMapMgr> wmp_mgr = MakePluginHolder<WorldMapMgr>(IE_WMP_CLASS_ID);
	if (! wmp_mgr)
		return;

	if (worldmap) {
		DataStream *wmp_str1 = gamedata->GetResourceStream(WorldMapName[0], IE_WMP_CLASS_ID);
		DataStream *wmp_str2 = gamedata->GetResourceStream(WorldMapName[1], IE_WMP_CLASS_ID);

		if (!wmp_mgr->Open(wmp_str1, wmp_str2)) {
			delete wmp_str1;
			delete wmp_str2;
		}

		delete worldmap;
		worldmap = wmp_mgr->GetWorldMapArray();
	}
}

void Interface::InitItemTypes()
{
	AutoTable it = gamedata->LoadTable("itemtype");
	if (!it) {
		throw CIE("Could not open itemtype table.");
	}
	ItemTypes = it->GetRowCount(); //number of itemtypes

	TableMgr::index_t InvSlotTypes = it->GetColumnCount();
	if (InvSlotTypes > 32) { //bit count limit
		InvSlotTypes = 32;
	}
	//make sure unsigned int is 32 bits
	slotmatrix.resize(ItemTypes);
	for (TableMgr::index_t i = 0; i < ItemTypes; i++) {
		unsigned int value = 0;
		unsigned int k = 1;
		for (TableMgr::index_t j = 0; j < InvSlotTypes; ++j) {
			if (it->QueryFieldSigned<long>(i,j)) {
				value |= k;
			}
			k <<= 1;
		}
		//we let any items in the inventory
		slotmatrix[i] = value | SLOT_INVENTORY;
	}

	//itemtype data stores (armor failure and critical damage multipliers), critical range
	itemtypedata.reserve(ItemTypes);
	for (TableMgr::index_t i = 0; i < ItemTypes; i++) {
		itemtypedata.emplace_back(4);
		//default values in case itemdata is missing (it is needed only for iwd2)
		if (slotmatrix[i] & SLOT_WEAPON) {
			itemtypedata[i][IDT_FAILURE] = 0; // armor malus
			itemtypedata[i][IDT_CRITRANGE] = 20; // crit range
			itemtypedata[i][IDT_CRITMULTI] = 2; // crit multiplier
			itemtypedata[i][IDT_SKILLPENALTY] = 0; // skill check malus
		}
	}
	AutoTable af = gamedata->LoadTable("itemdata");
	if (af) {
		TableMgr::index_t armcount = af->GetRowCount();
		TableMgr::index_t colcount = af->GetColumnCount();
		for (TableMgr::index_t i = 0; i < armcount; i++) {
			uint16_t itemtype = af->QueryFieldUnsigned<uint16_t>(i, 0);
			if (itemtype < ItemTypes) {
				// we don't need the itemtype column, since it is equal to the position
				for (TableMgr::index_t j = 0; j < colcount - 1; ++j) {
					itemtypedata[itemtype][j] = af->QueryFieldSigned<int>(i, j+1);
				}
			}
		}
	}

	//slottype describes the inventory structure
	Inventory::Init();
	AutoTable st = gamedata->LoadTable("slottype");
	if (!st) {
		throw CIE("Could not open slottype table.");
	}
	
	SlotTypes = st->GetRowCount();
	//make sure unsigned int is 32 bits
	slotTypes.resize(SlotTypes);
	for (unsigned int row = 0; row < SlotTypes; row++) {
		bool alias;
		ieDword i = strtounsigned<ieDword>(st->GetRowName(row).c_str());
		if (i>=SlotTypes) continue;
		if (slotTypes[i].slotEffects != 100) { // SLOT_EFFECT_ALIAS
			slotTypes[row].slot = i;
			i=row;
			alias = true;
		} else {
			slotTypes[row].slot = i;
			alias = false;
		}
		slotTypes[i].slotType = st->QueryFieldUnsigned<ieDword>(row, 0);
		slotTypes[i].slotID = st->QueryFieldUnsigned<ieDword>(row, 1);
		slotTypes[i].slotResRef = st->QueryField(row, 2);
		slotTypes[i].slotTip = st->QueryFieldUnsigned<ieDword>(row, 3);
		slotTypes[i].slotFlags = st->QueryFieldUnsigned<ieDword>(row, 5);
		//don't fill sloteffects for aliased slots (pst)
		if (alias) {
			continue;
		}
		slotTypes[i].slotEffects = st->QueryFieldUnsigned<ieDword>(row, 4);
		//setting special slots
		if (slotTypes[i].slotType & SLOT_ITEM) {
			if (slotTypes[i].slotType & SLOT_INVENTORY) {
				Inventory::SetInventorySlot(i);
			} else {
				Inventory::SetQuickSlot(i);
			}
		}
		switch (slotTypes[i].slotEffects) {
			//fist slot, not saved, default weapon
		case SLOT_EFFECT_FIST: Inventory::SetFistSlot(i); break;
			//magic weapon slot, overrides all weapons
		case SLOT_EFFECT_MAGIC: Inventory::SetMagicSlot(i); break;
			//weapon slot, Equipping marker is relative to it
		case SLOT_EFFECT_MELEE: Inventory::SetWeaponSlot(i); break;
			//ranged slot
		case SLOT_EFFECT_MISSILE: Inventory::SetRangedSlot(i); break;
			//right hand
		case SLOT_EFFECT_LEFT: Inventory::SetShieldSlot(i); break;
			//head (for averting critical hit)
		case SLOT_EFFECT_HEAD: Inventory::SetHeadSlot(i); break;
			//armor slot
		case SLOT_EFFECT_ITEM: Inventory::SetArmorSlot(i); break;
		default:;
		}
	}
}

ieDword Interface::FindSlot(unsigned int idx) const
{
	for (ieDword i = 0; i < SlotTypes; i++) {
		if (idx == slotTypes[i].slot) {
			return i;
		}
	}
	return 0;
}

ieDword Interface::QuerySlot(unsigned int idx) const
{
	if (idx>=SlotTypes) {
		return 0;
	}
	return slotTypes[idx].slot;
}

ieDword Interface::QuerySlotType(unsigned int idx) const
{
	if (idx>=SlotTypes) {
		return 0;
	}
	return slotTypes[idx].slotType;
}

ieDword Interface::QuerySlotID(unsigned int idx) const
{
	if (idx>=SlotTypes) {
		return 0;
	}
	return slotTypes[idx].slotID;
}

ieDword Interface::QuerySlottip(unsigned int idx) const
{
	if (idx>=SlotTypes) {
		return 0;
	}
	return slotTypes[idx].slotTip;
}

ieDword Interface::QuerySlotEffects(unsigned int idx) const
{
	if (idx>=SlotTypes) {
		return 0;
	}
	return slotTypes[idx].slotEffects;
}

ieDword Interface::QuerySlotFlags(unsigned int idx) const
{
	if (idx>=SlotTypes) {
		return 0;
	}
	return slotTypes[idx].slotFlags;
}

const ResRef& Interface::QuerySlotResRef(unsigned int idx) const
{
	if (idx>=SlotTypes) {
		static ResRef blank;
		return blank;
	}
	return slotTypes[idx].slotResRef;
}

int Interface::GetArmorFailure(unsigned int itemtype) const
{
	if (itemtype >= ItemTypes) {
		return 0;
	}
	if (slotmatrix[itemtype]&SLOT_ARMOUR) return itemtypedata[itemtype][IDT_FAILURE];
	return 0;
}

int Interface::GetShieldFailure(unsigned int itemtype) const
{
	if (itemtype >= ItemTypes) {
		return 0;
	}
	if (slotmatrix[itemtype]&SLOT_SHIELD) return itemtypedata[itemtype][IDT_FAILURE];
	return 0;
}

int Interface::GetArmorPenalty(unsigned int itemtype) const
{
	if (itemtype >= ItemTypes) {
		return 0;
	}
	if (slotmatrix[itemtype]&SLOT_ARMOUR) return itemtypedata[itemtype][IDT_SKILLPENALTY];
	return 0;
}

int Interface::GetShieldPenalty(unsigned int itemtype) const
{
	if (itemtype >= ItemTypes) {
		return 0;
	}
	if (slotmatrix[itemtype]&SLOT_SHIELD) return itemtypedata[itemtype][IDT_SKILLPENALTY];
	return 0;
}

int Interface::GetCriticalMultiplier(unsigned int itemtype) const
{
	if (itemtype >= ItemTypes) {
		return 2;
	}
	if (slotmatrix[itemtype]&SLOT_WEAPON) return itemtypedata[itemtype][IDT_CRITMULTI];
	return 2;
}

int Interface::GetCriticalRange(unsigned int itemtype) const
{
	if (itemtype >= ItemTypes) {
		return 20;
	}
	if (slotmatrix[itemtype]&SLOT_WEAPON) return itemtypedata[itemtype][IDT_CRITRANGE];
	return 20;
}

// checks the itemtype vs. slottype, and also checks the usability flags
// vs. Actor's stats (alignment, class, race, kit etc.)
int Interface::CanUseItemType(int slottype, const Item *item, const Actor *actor, bool feedback, bool equipped) const
{
	//inventory is a special case, we allow any items to enter it
	if ( slottype==-1 ) {
		return SLOT_INVENTORY;
	}
	//if we look for ALL slot types, then SLOT_SHIELD shouldn't interfere
	//with twohandedness
	//As long as this is an Item, use the ITEM constant
	//switch for IE_INV_ITEM_* if it is a CREItem
	if (item->Flags&IE_ITEM_TWO_HANDED) {
		//if the item is twohanded and there are more slots, drop the shield slot
		if (slottype&~SLOT_SHIELD) {
			slottype&=~SLOT_SHIELD;
		}
		if (slottype&SLOT_SHIELD) {
			//cannot equip twohanded in offhand
			if (feedback) displaymsg->DisplayConstantString(HCStrings::NotInOffhand, GUIColors::WHITE);
			return 0;
		}
	}

	if (item->ItemType >= ItemTypes) {
		//invalid itemtype
		if (feedback) displaymsg->DisplayConstantString(HCStrings::WrongItemType, GUIColors::WHITE);
		return 0;
	}

	//if actor is supplied, check its usability fields
	if (actor) {
		//custom strings
		ieStrRef str = actor->Disabled(item->Name, item->ItemType);
		if (str != ieStrRef::INVALID && !equipped) {
			if (feedback) displaymsg->DisplayString(str, GUIColors::WHITE, STRING_FLAGS::NONE);
			return 0;
		}

		//constant strings
		HCStrings idx = actor->Unusable(item);
		if (idx != HCStrings::count) {
			if (feedback) displaymsg->DisplayConstantString(idx, GUIColors::WHITE);
			return 0;
		}
	}

	//if any bit is true, the answer counts as true
	int ret = (slotmatrix[item->ItemType]&slottype);
	if (ret && actor && actor->RequiresUMD(item)) {
		// set an ugly marker for Use magic device, but only if the item is already known to be usable
		ret |= SLOT_UMD;
	}

	if (!ret) {
		if (feedback) displaymsg->DisplayConstantString(HCStrings::WrongItemType, GUIColors::WHITE);
		return 0;
	}

	//this warning comes only when feedback is enabled
	if (!feedback) {
		return ret;
	}

	// this was, but that disabled equipping of amber earrings in PST
	// if (slotmatrix[item->ItemType]&(SLOT_QUIVER|SLOT_WEAPON|SLOT_ITEM)) {
	if (!(ret & (SLOT_QUIVER | SLOT_WEAPON | SLOT_ITEM))) {
		return ret;
	}

	// don't ruin the return variable, it contains the usable slot bits
	int flg = 0;
	if (ret & SLOT_QUIVER) {
		if (item->GetWeaponHeader(true)) flg = 1;
	}

	if (ret & SLOT_WEAPON) {
		//melee
		if (item->GetWeaponHeader(false)) flg = 1;
		//ranged
		if (!flg && item->GetWeaponHeader(true)) flg = 1;
	}

	if (ret & SLOT_ITEM) {
		if (item->GetEquipmentHeaderNumber(0) != 0xffff) flg = 1;
	}

	if (!flg) {
		displaymsg->DisplayConstantString(HCStrings::ItemUnusable, GUIColors::WHITE);
		return 0;
	}

	return ret;
}

int Interface::CheckItemType(const Item* item, int slotType) const
{
	return slotmatrix[item->ItemType] & slotType;
}

Label* Interface::GetMessageLabel() const
{
	return GetControl<Label>("MsgSys", 1);
}

TextArea* Interface::GetMessageTextArea() const
{
	return GetControl<TextArea>("MsgSys", 0);
}

void Interface::SetFeedbackLevel(int level)
{
	FeedbackLevel = level;
}


bool Interface::HasFeedback(int type) const
{
	return FeedbackLevel & type;
}

static const char* const saved_extensions[] = { ".are", ".sto", ".blb", nullptr };
static const char* const saved_extensions_last[] = { ".tot", ".toh", nullptr };

//returns the priority of the file to be saved
//2 - save
//1 - save last
//0 - don't save
int Interface::SavedExtension(const path_t& filename) const
{
	size_t pos = filename.find_first_of('.');
	if (pos == path_t::npos) return 0;

	for (const auto ext : saved_extensions) {
		if (ext && !stricmp(ext, &filename[pos])) return 2;
	}
	for (const auto ext : saved_extensions_last) {
		if (ext && !stricmp(ext, &filename[pos])) return 1;
	}
	return 0;
}

static const char* const protected_extensions[] = { ".exe", ".dll", ".so", nullptr };

//returns true if file should be saved
bool Interface::ProtectedExtension(const path_t& filename) const
{
	size_t pos = filename.find_first_of('.');
	if (pos == path_t::npos) return false;
	int i=0;
	while(protected_extensions[i]) {
		if (!stricmp(protected_extensions[i], &filename[pos])) return true;
		i++;
	}
	return false;
}

void Interface::RemoveFromCache(const ResRef& resref, SClass_ID ClassID) const
{
	path_t filename = PathJoinExt(config.CachePath, resref, TypeExt(ClassID));
	unlink(filename.c_str());
}

//this function checks if the path is eligible as a cache
//if it contains a directory, or suspicious file extensions
//we bail out, because the cache will be purged regularly.
bool Interface::StupidityDetector(const path_t& path) const
{
	DirectoryIterator dir(path);
	// scan everything
	dir.SetFlags(DirectoryIterator::All, true);

	if (!dir) {
		Log(ERROR, "Interface", "**cannot open**");
		return true;
	}
	do {
		const path_t& name = dir.GetName();
		if (dir.IsDirectory()) {
			// since we can't use DirectoryIterator::Hidden, exclude the special dirs manually
			if (name[0] == '.' && name[1] == '\0') {
					continue;
			}
			if (name[0] == '.' && name[1] == '.' && name[2] == '\0') {
					continue;
			}
			Log(ERROR, "Interface", "**contains another dir**");
			return true; //a directory in there???
		}
		if (ProtectedExtension(name) ) {
			Log(ERROR, "Interface", "**contains alien files**");
			return true; //an executable file in there???
		}
	} while (++dir);
	//ok, we got a good conscience
	return false;
}

void Interface::DelTree(const path_t& path, bool onlysave) const
{
	if (path.empty()) return; //Don't delete the root filesystem :)

	DirectoryIterator dir(path);
	dir.SetFlags(DirectoryIterator::Files);
	if (!dir) {
		return;
	}
	do {
		const path_t& name = dir.GetName();
		if (!onlysave || SavedExtension(name) ) {
			path_t dtmp = dir.GetFullPath();
			unlink(dtmp.c_str());
		}
	} while (++dir);
}

void Interface::LoadProgress(int percent)
{
	WindowManager::CursorFeedback cur = winmgr->SetCursorFeedback(WindowManager::MOUSE_NONE);
	winmgr->DrawWindows();
	winmgr->SetCursorFeedback(cur);

	Control* prog = GetControl<Control>("LOAD_PROG", 0);
	if (prog) {
		// loadwin is NULL when LoadMap is called and passes false for the loadscreen param
		prog->SetValue(percent);
		// ensure the EndLoadScreen callback runs only once per each started load screen
		if (percent == 100) prog->SetDisabled(true);
	}

	VideoDriver->SwapBuffers();
}

void Interface::ReleaseDraggedItem()
{
	DraggedItem = nullptr;
	winmgr->GetGameWindow()->SetCursor(nullptr);
}

void Interface::DragItem(CREItem *item, const ResRef& /*Picture*/)
{
	//We should drop the dragged item and pick this up,
	//we shouldn't have a valid DraggedItem at this point.
	//Anyway, if there is still a dragged item, it will be destroyed.
	if (DraggedItem) {
		Log(WARNING, "Core", "Forgot to call ReleaseDraggedItem when leaving inventory (item destroyed)!");
		delete DraggedItem->item;
		DraggedItem = nullptr;
	}

	if (!item) return;

	DraggedItem.reset(new ItemDragOp(item));
	winmgr->GetGameWindow()->SetCursor(DraggedItem->cursor);
}

bool Interface::ReadItemTable(const ResRef& TableName, const path_t& Prefix)
{
	AutoTable tab = gamedata->LoadTable(TableName);
	if (!tab) {
		return false;
	}

	TableMgr::index_t i = tab->GetRowCount();
	for (TableMgr::index_t j = 0; j < i; j++) {
		ResRef ItemName;
		if (!Prefix.empty()) {
			ItemName.Format("{}{:02d}", Prefix, (j + 1) % 100);
		} else {
			ItemName = tab->GetRowName(j);
		}

		TableMgr::index_t l = tab->GetColumnCount(j);
		if (l<1) continue;
		int cl = atoi(tab->GetColumnName(0).c_str());
		std::vector<ResRef> refs;
		for(TableMgr::index_t k=0;k<l;k++) {
			refs.push_back(tab->QueryField(j, k));
		}
		RtRows.emplace(ItemName, ItemList(std::move(refs), cl));
	}
	return true;
}

bool Interface::ReadRandomItems()
{
	//rt norm or rt fury
	ieDword difflev = vars.Get("Nightmare Mode", 0);
	RtRows.clear();

	AutoTable tab = gamedata->LoadTable("randitem");
	if (!tab) {
		return false;
	}
	if (difflev>=tab->GetColumnCount()) {
		difflev = tab->GetColumnCount()-1;
	}

	//the gold item
	GoldResRef = tab->QueryField(size_t(0), size_t(0));
	if (IsStar(GoldResRef)) {
		return false;
	}
	ResRef randTreasureRef = tab->QueryField(1, difflev);
	int i = atoi(randTreasureRef.c_str());
	if (i<1) {
		ReadItemTable(randTreasureRef, ""); //reading the table itself
		return true;
	}
	if (i>5) {
		i=5;
	}
	while(i--) {
		randTreasureRef = tab->QueryField(2 + i, difflev);
		ReadItemTable(randTreasureRef,tab->GetRowName(2 + i));
	}
	return true;
}

CREItem *Interface::ReadItem(DataStream *str) const
{
	CREItem *itm = new CREItem();
	if (ReadItem(str, itm)) return itm;
	delete itm;
	return NULL;
}

CREItem *Interface::ReadItem(DataStream *str, CREItem *itm) const
{
	str->ReadResRef( itm->ItemResRef );
	str->ReadWord(itm->Expired);
	str->ReadWord(itm->Usages[0]);
	str->ReadWord(itm->Usages[1]);
	str->ReadWord(itm->Usages[2]);
	str->ReadDword(itm->Flags);
	if (ResolveRandomItem(itm)) {
		SanitizeItem(itm);
		return itm;
	}
	return NULL;
}

//Make sure the item attributes are valid
//we don't update all flags here because some need to be set later (like
//unmovable items in containers (e.g. the bg2 portal key) so that they
//can actually be picked up)
void Interface::SanitizeItem(CREItem *item) const
{
	//the stacked flag will be set by the engine if the item is indeed stacked
	//this is to fix buggy saves so TakeItemNum works
	//the equipped bit is also reset
	item->Flags &= ~(IE_INV_ITEM_STACKED|IE_INV_ITEM_EQUIPPED);

	//this is for converting IWD items magic flag
	if ((item->Flags & IE_INV_ITEM_UNDROPPABLE) && HasFeature(GFFlags::MAGICBIT)) {
		item->Flags |= IE_INV_ITEM_MAGICAL;
		item->Flags &= ~IE_INV_ITEM_UNDROPPABLE;
	}
	if (core->HasFeature(GFFlags::NO_UNDROPPABLE)) {
		item->Flags &= ~IE_INV_ITEM_UNDROPPABLE;
	}

	const Item *itm = gamedata->GetItem(item->ItemResRef, true);
	if (!itm) return;

	if (itm->Flags & IE_ITEM_NO_DISPEL) item->Flags |= IE_INV_ITEM_NO_DISPEL;

	item->MaxStackAmount = itm->MaxStackAmount;
	//if item is stacked mark it as so
	if (itm->MaxStackAmount) {
		item->Flags |= IE_INV_ITEM_STACKED;
		if (item->Usages[0] == 0) {
			item->Usages[0] = 1;
		}
	} else {
		//set charge counters for non-rechargeable items if their charge is zero
		//set charge counters for items not using charges to one
		for (int i = 0; i < CHARGE_COUNTERS; i++) {
			const ITMExtHeader *h = itm->GetExtHeader(i);
			if (!h) {
				item->Usages[i] = 0;
				continue;
			}

			if (item->Usages[i] != 0 && h->Charges == 0) {
				item->Usages[i] = 1;
				continue;
			}

			if (item->Usages[i] == 0 && !(h->RechargeFlags & IE_ITEM_RECHARGE)) {
				// HACK: the original (bg2) allows for 0 charged gems
				item->Usages[i] = std::max<ieWord>(1, h->Charges);
			}
		}
	}

	// simply adding the item flags to the slot
	item->Flags |= itm->Flags << 8;
	// some slot flags might be affected by the item flags
	if (!(item->Flags & IE_INV_ITEM_CRITICAL)) {
		item->Flags |= IE_INV_ITEM_DESTRUCTIBLE;
	}

	// pst has no stolen flag, but "steel" in its place
	if ((item->Flags & IE_INV_ITEM_STOLEN2) && !HasFeature(GFFlags::PST_STATE_FLAGS)) {
		item->Flags |= IE_INV_ITEM_STOLEN;
	}

	// auto identify basic items
	if (!itm->LoreToID) {
		item->Flags |= IE_INV_ITEM_IDENTIFIED;
	}

	gamedata->FreeItem(itm, item->ItemResRef, false);
}

// special iwd logic for random loot UP entries
static ResRef UpgradeRandomItem(const ResRef& pickedItem, const ResRef& workResRef)
{
	static const std::string upgradeProgressionStr = "lmrvvvvv";
	ResRef inputResRef = workResRef;
	StringToLower(inputResRef);
	uint8_t nameLength = inputResRef.length();
	char lastCharInInput = inputResRef[nameLength - 1];
	int currentTierIndex = int(upgradeProgressionStr.find_first_of(lastCharInInput));
	if (currentTierIndex == -1) {
		// invalid tier
		return inputResRef;
	}

	char lastCharInSelected = pickedItem[pickedItem.length() - 1];
	int upgradeAmount;
	switch (lastCharInSelected) {
		case '1':
		case '2':
		case '3':
			upgradeAmount = lastCharInSelected - '3' + 3;
			break;
		case 'P':
		case 'p':
			if (pickedItem.length() == 2) {
				// was just "up"
				upgradeAmount = 1;
				break;
			}
			// intentional fallthrough
		default:
			// invalid input
			return inputResRef;
	}

	char targetUpgradeTier = upgradeProgressionStr[currentTierIndex + upgradeAmount];
	if (lastCharInInput != targetUpgradeTier) {
		// replace last character in inputResRef to upgraded tier
		inputResRef[nameLength - 1] = targetUpgradeTier;
	}
	return inputResRef;
}

//This function generates random items based on the randitem.2da file
//there could be a loop, but we don't want to freeze, so there is a limit
// table entries are either:
// - working items
// - working items with a *N suffix, setting the stack count (eg. BOLT04*5)
// - references to other random tables to be resolved (eg. RNDMAG02 meaning the second row of RNDMAG.2da)
// - numbers only — gold amount
// - "UP", which causes a pick from one of the next rows (RT01_L -> RT01_M -> RT01_R -> RT01_V)
bool Interface::ResolveRandomItem(CREItem *itm) const
{
	if (RtRows.empty()) return true;

	auto pickFromRow = [this](const ResRef& itemRef)  {
		const ItemList& itemList = RtRows.at(itemRef);
		int idx;
		if (itemList.WeightOdds) {
			// instead of 1d19 we calculate with 2d10 (which also has 19 possible values)
			idx = Roll(2, (int(itemList.ResRefs.size()) + 1) / 2, -2);
		} else {
			idx = Roll(1, int(itemList.ResRefs.size()), -1);
		}
		return itemList.ResRefs[idx];
	};

	for (int loop = 0; loop < 10; loop++) {
		if (RtRows.count(itm->ItemResRef) == 0) {
			const Item* item = gamedata->GetItem(itm->ItemResRef, true);
			if (!item) {
				Log(ERROR, "Interface", "Nonexistent random item (bad table entry) detected: {}", itm->ItemResRef);
				return false;
			}
			// try detecting malformed / placeholder items, present in iwd2
			gamedata->FreeItem(item, itm->ItemResRef, false);
			return item->ItemName != ieStrRef::INVALID || item->ItemNameIdentified != ieStrRef::INVALID || item->ItemType || item->GetExtHeader(0);
		}

		ResRef pickedItem = pickFromRow(itm->ItemResRef);
		uint8_t nameLength = itm->ItemResRef.length();
		if (pickedItem == "UP" && nameLength > 2 && itm->ItemResRef[nameLength - 2] == '_') {
			ResRef newItem = UpgradeRandomItem(pickedItem, itm->ItemResRef);
			if (newItem == pickedItem) return false;
			pickedItem = pickFromRow(newItem);
		}

		// Explode to ResRef, so that there is a null terminator for strtounsigned
		auto parts = Explode<ResRef, ResRef>(pickedItem, '*', 1);
		ieWord diceSides;
		bool isGold = false;
		if (parts.size() > 1) {
			// create a stack
			diceSides = strtounsigned<ieWord>(parts[1].c_str(), nullptr, 10);
		} else {
			// gold or regular item (can have leading digits)
			char* endptr;
			diceSides = strtounsigned<ieWord>(parts[0].c_str(), &endptr, 10);
			if (*endptr) {
				diceSides = 1;
			} else {
				isGold = true;
			}
		}

		if (isGold) {
			itm->ItemResRef = GoldResRef;
			itm->Usages[0] = diceSides;
		} else {
			itm->ItemResRef = parts[0];
			if (itm->ItemResRef == "no_drop") {
				return false;
			}
			itm->Usages[0] = static_cast<ieWord>(Roll(1, diceSides, 0));
		}
	}
	Log(ERROR, "Interface", "Loop detected while generating random item: {}", itm->ItemResRef);
	return false;
}

Container *Interface::GetCurrentContainer()
{
	return CurrentContainer;
}

int Interface::CloseCurrentContainer()
{
	UseContainer = false;
	if ( !CurrentContainer) {
		return -1;
	}
	//remove empty ground piles on closeup
	CurrentContainer->GetCurrentArea()->TMap->CleanupContainer(CurrentContainer);
	CurrentContainer = NULL;
	return 0;
}

void Interface::SetCurrentContainer(const Actor *actor, Container *arg, bool flag)
{
	//abort action if the first selected PC isn't the original actor
	if (actor!=GetFirstSelectedPC(false)) {
		CurrentContainer = nullptr;
		return;
	}
	CurrentContainer = arg;
	UseContainer = flag;
}

Store *Interface::GetCurrentStore()
{
	return CurrentStore;
}

void Interface::CloseCurrentStore()
{
	gamedata->SaveStore(CurrentStore);
	CurrentStore = NULL;
}

Store *Interface::SetCurrentStore(const ResRef &resName, ieDword owner)
{
	if (CurrentStore) {
		if (CurrentStore->Name == resName) {
			return CurrentStore;
		}

		//not simply delete the old store, but save it
		CloseCurrentStore();
	}

	CurrentStore = gamedata->GetStore(resName);
	if (CurrentStore == NULL) {
		return NULL;
	}
	if (owner) {
		CurrentStore->SetOwnerID(owner);
	}
	return CurrentStore;
}

void Interface::SetMouseScrollSpeed(int speed) {
	mousescrollspd = (speed+1)*2;
}

int Interface::GetMouseScrollSpeed() const {
	return mousescrollspd;
}

ieStrRef Interface::GetRumour(const ResRef& dlgref)
{
	PluginHolder<DialogMgr> dm = GetImporter<DialogMgr>(IE_DLG_CLASS_ID, gamedata->GetResourceStream(dlgref, IE_DLG_CLASS_ID));
	Dialog *dlg = dm->GetDialog();

	if (!dlg) {
		Log(ERROR, "Interface", "Cannot load dialog: {}", dlgref);
		return ieStrRef(-1);
	}
	Scriptable *pc = game->GetSelectedPCSingle(false);

	// forcefully rerandomize
	RandomNumValue = RAND<int>();
	ieStrRef ret = ieStrRef(-1);

	int i = dlg->FindRandomState( pc );
	if (i>=0 ) {
		ret = dlg->GetState( i )->StrRef;
	}
	delete dlg;
	return ret;
}

//plays stock sound listed in defsound.2da
Holder<SoundHandle> Interface::PlaySound(size_t index, unsigned int channel) const
{
	if (index <= gamedata->defaultSounds.size()) {
		return AudioDriver->PlayRelative(gamedata->defaultSounds[index], channel);
	}
	return NULL;
}

Actor *Interface::GetFirstSelectedPC(bool forced)
{
	Actor *ret = nullptr;
	int slot = 0;
	int partySize = game->GetPartySize(false);
	if (!partySize) return nullptr;

	for (int i = 0; i < partySize; i++) {
		Actor* actor = game->GetPC(i, false);
		if (!actor->IsSelected()) continue;

		if (actor->InParty < slot || !ret) {
			ret = actor;
			slot = actor->InParty;
		}
	}

	if (forced && !ret) {
		return game->FindPC(1);
	}
	return ret;
}

Actor *Interface::GetFirstSelectedActor()
{
	if (!game->selected.empty()) {
		return game->selected[0];
	}
	return NULL;
}

bool Interface::HasCurrentArea() const
{
	if (!game) return false;
	return game->GetCurrentArea() != nullptr;
}

//this is used only for the console
Holder<Sprite2D> Interface::GetCursorSprite() const
{
	Holder<Sprite2D> spr = gamedata->GetBAMSprite(TextCursorBam, 0, 0);
	if (spr && HasFeature(GFFlags::OVERRIDE_CURSORPOS)) {
		spr->Frame.x = 1;
		spr->Frame.y = spr->Frame.h - 1;
	}
	return spr;
}

Holder<Sprite2D> Interface::GetScrollCursorSprite(orient_t orient, int spriteNum) const
{
	// map orientation to sprite frame
	orient = ReduceToHalf(orient); // there are only 8 valid directions instead of 16
	uint8_t frame = 6 - (orient / 2);
	frame = Clamp<uint8_t>(frame, 0, 7);
	return gamedata->GetBAMSprite(ScrollCursorBam, frame, spriteNum, true);
}

/* we should return -1 if it isn't gold, otherwise return the gold value */
int Interface::CanMoveItem(const CREItem *item) const
{
	//This is an inventory slot, switch to IE_ITEM_* if you use Item
	if (item->Flags & IE_INV_ITEM_UNDROPPABLE && !HasFeature(GFFlags::NO_DROP_CAN_MOVE)) {
		return 0;
	}
	//not gold, we allow only one single coin ResRef, this is good
	//for all of the original games
	if (GoldResRef != item->ItemResRef) {
		return -1;
	}
	//gold, returns the gold value (stack size)
	return item->Usages[0];
}

// dealing with applying effects
void Interface::ApplySpell(const ResRef& spellRef, Actor *actor, Scriptable *caster, int level) const
{
	Spell *spell = gamedata->GetSpell(spellRef);
	if (!spell) {
		return;
	}

	int header = spell->GetHeaderIndexFromLevel(level);

	auto block = spell->GetEffectBlock(caster, actor->Pos, header, level);
	ApplyEffectQueue(&block, actor, caster, actor->Pos);
}

void Interface::ApplySpellPoint(const ResRef& spellRef, Map* area, const Point &pos, Scriptable *caster, int level) const
{
	Spell *spell = gamedata->GetSpell(spellRef);
	if (!spell) {
		return;
	}
	int header = spell->GetHeaderIndexFromLevel(level);
	Projectile *pro = spell->GetProjectile(caster, header, level, pos);
	pro->SetCaster(caster->GetGlobalID(), level);
	area->AddProjectile(pro, caster->Pos, pos);
}

//-1 means the effect was reflected back to the caster
//0 means the effect was resisted and should be removed
//1 means the effect was applied
int Interface::ApplyEffect(Effect *effect, Actor *actor, Scriptable *caster) const
{
	if (!effect) {
		return 0;
	}

	EffectQueue fxqueue;
	fxqueue.AddEffect(effect);
	int res = ApplyEffectQueue(&fxqueue, actor, caster);
	return res;
}

int Interface::ApplyEffectQueue(EffectQueue *fxqueue, Actor *actor, Scriptable *caster) const
{
	Point p(-1, -1); //the effect should have all its coordinates already set
	return ApplyEffectQueue(fxqueue, actor, caster, p);
}

//FIXME: AddAllEffects will directly apply the effects outside of the mechanisms of Actor::RefreshEffects
//This means, pcf functions may not be executed when the effect is first applied
//Adding this new effect block via RefreshEffects is possible, but that might apply existing effects twice

int Interface::ApplyEffectQueue(EffectQueue *fxqueue, Actor *actor, Scriptable *caster, Point p) const
{
	int res = fxqueue->CheckImmunity ( actor );
	if (res) {
		if (res == -1 && caster) {
			//bounced back at a nonliving caster
			if (caster->Type!=ST_ACTOR) {
				return 0;
			}
			actor = (Actor *) caster;
		}
		fxqueue->SetOwner( caster );

		if (fxqueue->AddAllEffects( actor, p)==FX_NOT_APPLIED) {
			res=0;
		}
	}
	return res;
}

Effect *Interface::GetEffect(const ResRef& resname, int level, const Point &p)
{
	//Don't free this reference, it is cached!
	Effect *effect = gamedata->GetEffect(resname);
	if (!effect) {
		return NULL;
	}
	if (!level) {
		level = 1;
	}
	effect->Power = level;
	effect->Pos = p;
	return effect;
}

// dealing with saved games
int Interface::SwapoutArea(Map *map) const
{
	//refuse to save ambush areas, for example
	if (map->AreaFlags & AF_NOSAVE) {
		Log(DEBUG, "Core", "Not saving area {}",
			map->GetScriptName());
		RemoveFromCache(map->GetScriptRef(), IE_ARE_CLASS_ID);
		return 0;
	}

	auto mm = GetImporter<MapMgr>(IE_ARE_CLASS_ID);
	if (mm == nullptr) {
		return -1;
	}
	int size = mm->GetStoredFileSize (map);
	if (size > 0) {
		//created streams are always autofree (close file on destruct)
		//this one will be destructed when we return from here
		FileStream str;

		str.Create(map->GetScriptName().c_str(), IE_ARE_CLASS_ID);
		int ret = mm->PutArea (&str, map);
		if (ret <0) {
			Log(WARNING, "Core", "Area removed: {}",
				map->GetScriptName());
			RemoveFromCache(map->GetScriptRef(), IE_ARE_CLASS_ID);
		}
	} else {
		Log(WARNING, "Core", "Area removed: {}",
			map->GetScriptName());
		RemoveFromCache(map->GetScriptRef(), IE_ARE_CLASS_ID);
	}
	//make sure the stream isn't connected to sm, or it will be double freed
	return 0;
}

int Interface::WriteCharacter(StringView name, const Actor* actor) const
{
	path_t Path = PathJoin(config.GamePath, config.GameCharactersPath);
	if (!actor) {
		return -1;
	}
	auto gm = GetImporter<ActorMgr>(IE_CRE_CLASS_ID);
	if (gm == nullptr) {
		return -1;
	}

	FileStream str;
	if (!str.Create(Path, name.c_str(), IE_CHR_CLASS_ID)
		|| gm->PutActor(&str, actor, true) < 0) {
		Log(WARNING, "Core", "Character cannot be saved: {}", name);
		return -1;
	}

	//write the BIO string
	if (!HasFeature(GFFlags::NO_BIOGRAPHY)) {
		str.Create(Path, name.c_str(), IE_BIO_CLASS_ID);
		//never write the string reference into this string
		std::string mbstr = GetMBString(actor->GetVerbalConstant(Verbal::Bio), STRING_FLAGS::STRREFOFF);
		str.Write(mbstr.data(), mbstr.length());
	}
	return 0;
}

int Interface::WriteGame(const path_t& folder)
{
	PluginHolder<SaveGameMgr> gm = GetImporter<SaveGameMgr>(IE_GAM_CLASS_ID);
	if (gm == nullptr) {
		return -1;
	}

	int size = gm->GetStoredFileSize (game);
	if (size > 0) {
		//created streams are always autofree (close file on destruct)
		//this one will be destructed when we return from here
		FileStream str;

		str.Create(folder, GameNameResRef.c_str(), IE_GAM_CLASS_ID);
		int ret = gm->PutGame (&str, game);
		if (ret <0) {
			Log(WARNING, "Core", "Game cannot be saved: {}", folder);
			return -1;
		}
	} else {
		Log(WARNING, "Core", "Internal error, game cannot be saved: {}", folder);
		return -1;
	}
	return 0;
}

int Interface::WriteWorldMap(const path_t& folder)
{
	PluginHolder<WorldMapMgr> wmm = MakePluginHolder<WorldMapMgr>(IE_WMP_CLASS_ID);
	if (wmm == nullptr) {
		return -1;
	}

	if (!WorldMapName[1].IsEmpty()) {
		worldmap->SetSingle(false);
	}

	int size1 = wmm->GetStoredFileSize (worldmap, 0);
	int size2 = 1; //just a dummy value

	//if size is 0 for the first worldmap, then there is a problem
	if (!worldmap->IsSingle() && (size1>0) ) {
		size2=wmm->GetStoredFileSize (worldmap, 1);
	}

	int ret = 0;
	if ((size1 < 0) || (size2<0) ) {
		ret=-1;
	} else {
		//created streams are always autofree (close file on destruct)
		//this one will be destructed when we return from here
		FileStream str1;
		FileStream str2;

		str1.Create(folder, WorldMapName[0].c_str(), IE_WMP_CLASS_ID);
		if (!worldmap->IsSingle()) {
			str2.Create(folder, WorldMapName[1].c_str(), IE_WMP_CLASS_ID);
		}
		ret = wmm->PutWorldMap (&str1, &str2, worldmap);
	}
	if (ret <0) {
		Log(WARNING, "Core", "Internal error, worldmap cannot be saved: {}", folder);
		return -1;
	}
	return 0;
}

static bool IsBlobSaveItem(const path_t& path) {
	auto areExt = path.find(".blb");
	auto pathLength = path.length();
	return areExt != path_t::npos && areExt == pathLength - 4;
}

int Interface::CompressSave(const path_t& folder, bool overrideRunning)
{
	FileStream str;

	str.Create(folder, GameNameResRef.c_str(), IE_SAV_CLASS_ID);
	DirectoryIterator dir(config.CachePath);
	if (!dir) {
		return GEM_ERROR;
	}
	PluginHolder<ArchiveImporter> ai = MakePluginHolder<ArchiveImporter>(IE_SAV_CLASS_ID);
	ai->CreateArchive( &str);

	tick_t startTime = GetMilliseconds();
	// If we override the savegame we are running to fetch AREs from, it has already dumped
	// itself as "ares.blb" into the cache folder. Otherwise, just copy directly.
	if (!overrideRunning && saveGameAREExtractor.copyRetainedAREs(&str) == GEM_ERROR) {
		Log(ERROR, "Interface", "Failed to copy ARE files into new save game.");
		return GEM_ERROR;
	}

	dir.SetFlags(DirectoryIterator::Files);
	//.tot and .toh should be saved last, because they are updated when an .are is saved
	int priority=2;
	while(priority) {
		do {
			const path_t& name = dir.GetName();
			if (SavedExtension(name) == priority) {
				path_t dtmp = dir.GetFullPath();
				FileStream fs;
				if (!fs.Open(dtmp)) {
					Log(ERROR, "Interface", "Failed to open \"{}\".", dtmp);
				}

				if (IsBlobSaveItem(dtmp)) {
					if (overrideRunning) {
						saveGameAREExtractor.updateSaveGame(str.GetPos());
						ai->AddToSaveGameCompressed(&str, &fs);
					}
				} else {
					ai->AddToSaveGame(&str, &fs);
				}
			}
		} while (++dir);
		//reopen list for the second round
		priority--;
		if (priority>0) {
			dir.Rewind();
		}
	}

	tick_t endTime = GetMilliseconds();
	Log(WARNING, "Core", "{} ms (compressing SAV file)", endTime - startTime);
	return GEM_OK;
}

int Interface::GetMaximumAbility() const { return MaximumAbility; }

int Interface::GetStrengthBonus(int column, int value, int ex) const
{
	//to hit, damage, open doors, weight allowance
	if (column<0 || column>3)
		return -9999;

	value = Clamp(value, 0, MaximumAbility);

	int bonus = 0;
	// only 18 (human max) has the differentiating extension
	if (value == 18 && !HasFeature(GFFlags::RULES_3ED)) {
		ex = Clamp(ex, 0, 100);
		bonus += abilityTables->strmodex[column*101+ex];
	}

	return abilityTables->strmod[column*(MaximumAbility+1)+value] + bonus;
}

//The maze columns are used only in the maze spell, no need to restrict them further
int Interface::GetIntelligenceBonus(int column, int value) const
{
	//learn spell, max spell level, max spell number on level, maze duration dice, maze duration dice size
	if (column<0 || column>4) return -9999;

	return abilityTables->intmod[column*(MaximumAbility+1)+value];
}

int Interface::GetDexterityBonus(int column, int value) const
{
	//no dexmod in iwd2 and only one type of modifier
	if (HasFeature(GFFlags::RULES_3ED)) {
		return value/2-5;
	}

	//reaction, missile, ac
	if (column<0 || column>2)
		return -9999;

	return abilityTables->dexmod[column*(MaximumAbility+1)+value];
}

int Interface::GetConstitutionBonus(int column, int value) const
{
	//no conmod in iwd2 and also no regenation bonus
	if (HasFeature(GFFlags::RULES_3ED)) {
		if (column == STAT_CON_HP_REGEN) {
			return 0;
		}
		return value/2-5;
	}

	//normal, warrior, minimum, regen hp, regen fatigue
	if (column<0 || column>4)
		return -9999;

	return abilityTables->conmod[column*(MaximumAbility+1)+value];
}

int Interface::GetCharismaBonus(int column, int /*value*/) const
{
	// store price reduction
	if (column<0 || column>(MaximumAbility-1))
		return -9999;

	return abilityTables->chrmod[column];
}

int Interface::GetLoreBonus(int column, int value) const
{
	//no lorebon in iwd2 - lore is a skill
	if (HasFeature(GFFlags::RULES_3ED)) return 0;

	if (column<0 || column>0)
		return -9999;

	return abilityTables->lorebon[value];
}

int Interface::GetWisdomBonus(int column, int value) const
{
	if (abilityTables->wisbon.empty()) return 0;

	// xp bonus
	if (column<0 || column>0)
		return -9999;

	return abilityTables->wisbon[value];
}

PauseState Interface::TogglePause() const
{
	const GameControl *gc = GetGameControl();
	if (!gc) return PauseState::Off;
	PauseState pause = (PauseState) ((gc->GetDialogueFlags() & DF_FREEZE_SCRIPTS) == 0);
	if (SetPause(pause)) return pause;
	return (PauseState) ((gc->GetDialogueFlags() & DF_FREEZE_SCRIPTS) > 0);
}

bool Interface::SetPause(PauseState pause, int flags) const
{
	GameControl *gc = GetGameControl();

	//don't allow soft pause in cutscenes and dialog
	if (!(flags&PF_FORCED) && InCutSceneMode()) gc = NULL;

	if (gc && ((bool)(gc->GetDialogueFlags()&DF_FREEZE_SCRIPTS) != (bool)pause)) { // already paused
		HCStrings strref;
		if (pause == PauseState::On) {
			strref = HCStrings::Paused;
			gc->SetDialogueFlags(DF_FREEZE_SCRIPTS, BitOp::OR);
		} else {
			strref = HCStrings::Unpaused;
			gc->SetDialogueFlags(DF_FREEZE_SCRIPTS, BitOp::NAND);
		}
		if (!(flags&PF_QUIET) ) {
			if (pause == PauseState::On) gc->SetDisplayText(strref, 0); // time 0 = removed instantly on unpause (for pst)
			displaymsg->DisplayConstantString(strref, GUIColors::RED);
		}
		return true;
	}
	return false;
}

bool Interface::Autopause(AUTOPAUSE flag, Scriptable* target) const
{
	AUTOPAUSE autopause_flags = vars.GetAs<AUTOPAUSE>("Auto Pause State", AUTOPAUSE::UNUSABLE);

	if (!(autopause_flags & AUTOPAUSE(1 << ieDword(flag)))) {
		return false;
	}

	if (!SetPause(PauseState::On, PF_QUIET)) {
		return false;
	}

	displaymsg->DisplayConstantStringName(HCStrings(ieDword(HCStrings::ApUnusable) + ieDword(flag)), GUIColors::RED, target);

	ieDword centerOnAutoPause = vars.Get("Auto Pause Center", 0);

	if (centerOnAutoPause && target) {
		GameControl* gc = GetGameControl();
		gc->MoveViewportTo(target->Pos, true);
		Actor* tar = Scriptable::As<Actor>(target);
		if (tar && tar->GetStat(IE_EA) < EA_GOODCUTOFF) {
			core->GetGame()->SelectActor(tar, true, SELECT_REPLACE);
		}
	}
	return true;
}

void Interface::RegisterOpcodes(int count, const EffectDesc *opcodes) const
{
	EffectQueue_RegisterOpcodes(count, opcodes);
}

void Interface::GetResRefFrom2DA(const ResRef& resref, ResRef& resource1, ResRef& resource2, ResRef& resource3) const
{
	resource1.Reset();
	resource2.Reset();
	resource3.Reset();

	AutoTable tab = gamedata->LoadTable(resref);
	if (tab) {
		TableMgr::index_t cols = tab->GetColumnCount();
		TableMgr::index_t row = RAND<TableMgr::index_t>(0, tab->GetRowCount() - 1);
		resource1 = tab->QueryField(row, 0);
		if (cols > 1) {
			resource2 = tab->QueryField(row, 1);
		}
		if (cols > 2) {
			resource3 = tab->QueryField(row, 2);
		}
	}
}

std::vector<ieDword> Interface::GetListFrom2DAInternal(const ResRef& resref) const
{
	std::vector<ieDword> ret;

	AutoTable tab = gamedata->LoadTable(resref);
	if (tab) {
		ret.resize(tab->GetRowCount());
		for (size_t i = 0; i < ret.size(); ++i) {
			ret.at(i) = tab->QueryFieldUnsigned<ieDword>(i, 0);
		}
	}

	return ret;
}

const std::vector<ieDword>* Interface::GetListFrom2DA(const ResRef& tablename)
{
	auto listptr = lists.Get(tablename);
	if (listptr == nullptr) {
		listptr = &lists.Set(tablename, GetListFrom2DAInternal(tablename));
	}

	return listptr;
}

//returns a numeric value associated with a stat name (symbol) from stats.ids
ieDword Interface::TranslateStat(const std::string& statName)
{
	ieDword tmp;
	if (valid_unsignednumber(statName.c_str(), tmp)) {
		return tmp;
	}

	int symbol = LoadSymbol( "stats" );
	auto sym = GetSymbol( symbol );
	if (!sym) {
		error("Core", "Cannot load statistic name mappings.");
	}
	ieDword stat = sym->GetValue(statName);
	if (stat==(ieDword) ~0) {
		Log(WARNING, "Core", "Cannot translate symbol: {}", statName);
	}
	return stat;
}

// Calculates an arbitrary stat bonus, based on tables.
// the master table contains the table names (as row names) and the used stat
// the subtables contain stat value/bonus pairs.
// Optionally an override stat value can be specified (needed for use in pcfs).
int Interface::ResolveStatBonus(const Actor* actor, const ResRef& tableName, ieDword flags, int value)
{
	AutoTable mtm = gamedata->LoadTable(tableName);
	if (!mtm) {
		Log(ERROR, "Core", "Cannot resolve stat bonus.");
		return -1;
	}
	TableMgr::index_t count = mtm->GetRowCount();
	if (count< 1) {
		return 0;
	}
	int ret = 0;
	// tables for additive modifiers of bonus type
	for (TableMgr::index_t i = 0; i < count; i++) {
		ResRef subTableName = mtm->GetRowName(i);
		int checkcol = mtm->QueryFieldSigned<int>(i,1);
		unsigned int readcol = mtm->QueryFieldUnsigned<unsigned int>(i,2);
		int stat = TranslateStat(mtm->QueryField(i, 0));
		if (!(flags&1)) {
			value = actor->GetSafeStat(stat);
		}
		auto tm = gamedata->LoadTable(subTableName);
		if (!tm) continue;

		TableMgr::index_t row;
		if (checkcol == -1) {
			// use the row names
			row = tm->GetRowIndex(fmt::to_string(value));
		} else {
			// use the checkcol column (default of 0)
			row = tm->FindTableValue(checkcol, value, 0);
		}
		if (row != TableMgr::npos) {
			ret += tm->QueryFieldSigned<int>(row, readcol);
		}
	}
	return ret;
}

// see 8cff52b3c8 if this needs to be resurrected at some point
void Interface::WaitForDisc(int disc_number, const path_t& path)
{
	vars.Set("WaitForDisc", disc_number);

	GetGUIScriptEngine()->RunFunction( "GUICommonWindows", "OpenWaitForDiscWindow" );
	do {
		winmgr->DrawWindows();
		for (const auto& cd : config.CD[disc_number - 1]) {
			path_t name = PathJoin(cd, path);
			if (FileExists(name)) {
				GetGUIScriptEngine()->RunFunction( "GUICommonWindows", "OpenWaitForDiscWindow" );
				return;
			}
		}

	} while (VideoDriver->SwapBuffers() == GEM_OK);
}

Timer& Interface::SetTimer(const EventHandler& handler, tick_t interval, int repeats)
{
	timers.emplace_back(interval, handler, repeats);
	return timers.back();
}

void Interface::SetNextScript(const path_t& script)
{
	nextScript = script;
	QuitFlag |= QF_CHANGESCRIPT;
}

}
