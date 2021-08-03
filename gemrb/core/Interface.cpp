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
#ifndef STATIC_LINK
#include "PluginLoader.h"
#endif
#include "PluginMgr.h"
#include "Predicates.h"
#include "ProjectileServer.h"
#include "SaveGameIterator.h"
#include "SaveGameMgr.h"
#include "ScriptedAnimation.h"
#include "SoundMgr.h"
#include "SpellMgr.h"
#include "StoreMgr.h"
#include "StringMgr.h"
#include "SymbolMgr.h"
#include "TileMap.h"
#include "VEFObject.h"
#include "Video/Video.h"
#include "WorldMapMgr.h"
#include "GUI/Button.h"
#include "GUI/Console.h"
#include "GUI/EventMgr.h"
#include "GUI/GameControl.h"
#include "GUI/GUIScriptInterface.h"
#include "GUI/Label.h"
#include "GUI/MapControl.h"
#include "GUI/TextArea.h"
#include "GUI/WindowManager.h"
#include "GUI/WorldMapControl.h"
#include "RNG.h"
#include "Scriptable/Container.h"
#include "System/FileStream.h"
#include "System/FileFilters.h"
#include "System/StringBuffer.h"

#include <utility>
#include <vector>

#ifdef WIN32
#include "CodepageToIconv.h"
#elif defined(HAVE_LANGINFO_H)
#include <langinfo.h>
#endif

namespace GemRB {

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
	
	AbilityTables(int MaximumAbility) noexcept
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
		
		if (!core->HasFeature(GF_3ED_RULES)) {
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
		AutoTable tab(tablename);
		if (!tab) {
			return false;
		}
		//this is a hack for rows not starting at 0 in some cases
		int fix = 0;
		const char * tmp = tab->GetRowName(0);
		if (tmp && (tmp[0]!='0')) {
			fix = atoi(tmp);
			for (int i=0;i<fix;i++) {
				for (int j=0;j<columns;j++) {
					table[rows*j+i] = strtosigned<ieWordSigned>(tab->QueryField(0,j));
				}
			}
		}
		for (int j=0;j<columns;j++) {
			for( int i=0;i<rows-fix;i++) {
				table[rows*j+i+fix] = strtosigned<ieWordSigned>(tab->QueryField(i,j));
			}
		}
		return true;
	}
};

static std::unique_ptr<AbilityTables> abilityTables;

static int **reputationmod = NULL;
static const char* const IWD2DeathVarFormat = "_DEAD%s";
static const char* DeathVarFormat = "SPRITE_IS_DEAD%s";
static int NumRareSelectSounds = 2;

static const ieWord IDT_FAILURE = 0;
static const ieWord IDT_CRITRANGE = 1;
static const ieWord IDT_CRITMULTI = 2;
static const ieWord IDT_SKILLPENALTY = 3;

static int MagicBit = 0;
static const char* DefaultSystemEncoding = "UTF-8";

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
	strnlwrcpy(dragDummy.VarName, "itembutton", MAX_VARIABLE_LENGTH - 1);
}

Interface::Interface()
{
	for (unsigned int i = 0; i < 256; i++) {
		pl_uppercase[i]=(ieByte) toupper(i);
		pl_lowercase[i]=(ieByte) tolower(i);
	}

	displaymsg = nullptr;

#ifdef WIN32
	config.CaseSensitive = false; // this is just the default value, so CD1/CD2 will be resolved
#endif

	SystemEncoding = DefaultSystemEncoding;
#if defined(WIN32) && defined(HAVE_ICONV)
	const uint32_t codepage = GetACP();
	const char* iconvCode = GetIconvNameForCodepage(codepage);

	if (nullptr == iconvCode) {
		error("Interface", "Mapping of codepage %u unknown to iconv.", codepage);
	}
	SystemEncoding = iconvCode;
#elif defined(HAVE_LANGINFO_H)
	SystemEncoding = nl_langinfo(CODESET);
#endif

	MagicBit = HasFeature(GF_MAGICBIT);

	gamedata = new GameData();

	plugin_flags = new Variables();
	plugin_flags->SetType( GEM_VARIABLES_INT );

	lists = new Variables();
	lists->SetType( GEM_VARIABLES_POINTER );

	vars = new Variables();
	vars->SetType( GEM_VARIABLES_INT );
	vars->ParseKey(true);

	tokens = new Variables();
	tokens->SetType( GEM_VARIABLES_STRING );
}

static void ReleaseItemList(void *poi)
{
	delete ((ItemList *) poi);
}

Interface::~Interface(void)
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

	if (reputationmod) {
		for (unsigned int i=0; i<20; i++) {
			if (reputationmod[i]) {
				free(reputationmod[i]);
			}
		}
		free(reputationmod);
		reputationmod=NULL;
	}

	SpecialSpells.clear();
	SurgeSpells.clear();

	std::map<ResRef, Font*>::iterator fit = fonts.begin();
	for (; fit != fonts.end(); ++fit)
		delete (*fit).second;
	// fonts need to be destroyed before TTF plugin
	PluginMgr::Get()->RunCleanup();

	ReleaseMemoryActor();

	slotTypes.clear();
	free( slotmatrix );
	itemtypedata.clear();

	delete sgiterator;

	for (auto& track : musiclist) {
		free((void *) track);
	}

	DamageInfoMap.clear();

	delete plugin_flags;

	delete projserv;

	delete displaymsg;
	delete TooltipBG;

	delete vars;
	delete tokens;
	if (lists) {
		lists->RemoveAll(free);
		delete lists;
	}

	Actor::ReleaseMemory();

	gamedata->ClearCaches();
	delete gamedata;
	gamedata = NULL;

	// Removing all stuff from Cache, except bifs
	if (!config.KeepCache) DelTree((const char *) config.CachePath, true);
}

GameControl* Interface::StartGameControl()
{
	assert(gamectrl == nullptr);

	gamedata->DelTable(0xffffu); //dropping ALL tables
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
void Interface::HandleFlags()
{
	//clear events because the context changed
	EventFlag = EF_CONTROL;

	// save console history, so it's preserved between runs and games
	if (QuitFlag & (QF_QUITGAME | QF_EXITGAME | QF_LOADGAME)) {
		const Console* console = GetControl<Console>("CONSOLE_CTL", 1);
		if (console) console->SaveHistory();
	}

	if (QuitFlag&(QF_QUITGAME|QF_EXITGAME) ) {
		// closing windows must come before tearing anything else down
		// some window close handlers expect game/gamecontrol to be valid
		// let them run, then start tearing things down
		winmgr->DestroyAllWindows();
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
		QuitFlag &= ~(QF_QUITGAME|QF_EXITGAME);
	}

	if (QuitFlag&QF_LOADGAME) {
		QuitFlag &= ~QF_LOADGAME;

		LoadGame(LoadGameIndex.get(), VersionOverride );
		LoadGameIndex.release();
		//after loading a game, always check if the game needs to be upgraded
	}

	if (QuitFlag&QF_ENTERGAME) {
		winmgr->DestroyAllWindows();
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
		guiscript->LoadScript( NextScript );
		guiscript->RunFunction( NextScript, "OnLoad" );
	}
}

bool Interface::ReadGameTimeTable()
{
	AutoTable table("gametime");
	if (!table) {
		return false;
	}

	Time.round_sec = atoi(table->QueryField("ROUND_SECONDS", "DURATION"));
	Time.turn_sec = atoi(table->QueryField("TURN_SECONDS", "DURATION"));
	Time.round_size = Time.round_sec * AI_UPDATE_TIME;
	Time.rounds_per_turn = Time.turn_sec / Time.round_sec;
	Time.attack_round_size = atoi(table->QueryField("ATTACK_ROUND", "DURATION"));
	Time.hour_sec = 300; // move to table if pst turns out to be different
	Time.hour_size = Time.hour_sec * AI_UPDATE_TIME;
	Time.day_sec = Time.hour_sec * 24; // move to table if pst turns out to be different
	Time.day_size = Time.day_sec * AI_UPDATE_TIME;

	return true;
}

bool Interface::ReadSpecialSpells()
{
	bool result = true;

	AutoTable table("splspec");
	if (table) {
		ieDword SpecialSpellsCount = table->GetRowCount();
		SpecialSpells.resize(SpecialSpellsCount);
		for (ieDword i = 0; i < SpecialSpellsCount; i++) {
			SpecialSpells[i].resref = table->GetRowName(i);
			//if there are more flags, compose this value into a bitfield
			SpecialSpells[i].flags = atoi(table->QueryField(i, 0));
			SpecialSpells[i].amount = atoi(table->QueryField(i, 1));
			SpecialSpells[i].bonus_limit = atoi(table->QueryField(i, 2));
		}
	} else {
		result = false;
	}

	table = AutoTable("wildmag");
	if (table) {
		SurgeSpell ss;
		for (ieDword i = 0; i < table->GetRowCount(); i++) {
			ss.spell = table->QueryField(i, 0);
			ss.message = strtounsigned<ieStrRef>(table->QueryField(i, 1));
			// comment ignored
			SurgeSpells.push_back(ss);
		}
	} else {
		result = false;
	}

	return result;
}

int Interface::GetSpecialSpell(const ResRef& resref) const
{
	for (const auto& spell : SpecialSpells) {
		if (resref == spell.resref) {
			return spell.flags;
		}
	}
	return 0;
}

//disable spells based on some circumstances
int Interface::CheckSpecialSpell(const ResRef& resRef, const Actor *actor) const
{
	int sp = GetSpecialSpell(resRef);

	//the identify spell is always disabled on the menu
	if (sp&SP_IDENTIFY) {
		return SP_IDENTIFY;
	}

	//if actor is silenced, and spell cannot be cast in silence, disable it
	if (actor->GetStat(IE_STATE_ID) & STATE_SILENCED ) {
		if (!(sp&SP_SILENCE)) {
			return SP_SILENCE;
		}
	}

	// disable spells causing surges to be cast while in a surge (prevents nesting)
	if (sp&SP_SURGE) {
		return SP_SURGE;
	}

	return 0;
}

//Static
const char *Interface::GetDeathVarFormat()
{
	return DeathVarFormat;
}

bool Interface::ReadAreaAliasTable(const ResRef& tablename)
{
	AreaAliasTable.clear();

	AutoTable aa(tablename);
	if (!aa) {
		//don't report error when the file doesn't exist
		return true;
	}

	int idx = aa->GetRowCount();
	while (idx--) {
		ResRef key = MakeLowerCaseResRef(aa->GetRowName(idx));
		ieDword value = atoi(aa->QueryField(idx,0));
		AreaAliasTable[key] = value;
	}
	return true;
}

int Interface::GetAreaAlias(const ResRef &areaname) const
{
	if (AreaAliasTable.count(areaname)) {
		return AreaAliasTable.at(areaname);
	}
	return -1;
}

bool Interface::ReadMusicTable(const ResRef& tablename, int col) {
	AutoTable tm(tablename);
	if (!tm)
		return false;

	for (unsigned int i = 0; i < tm->GetRowCount(); i++) {
		musiclist.push_back(strdup(tm->QueryField(i, col)));
	}

	return true;
}

bool Interface::ReadDamageTypeTable() {
	AutoTable tm("dmgtypes");
	if (!tm)
		return false;

	DamageInfoStruct di;
	for (ieDword i = 0; i < tm->GetRowCount(); i++) {
		di.strref = displaymsg->GetStringReference(atoi(tm->QueryField(i, 0)));
		di.resist_stat = TranslateStat(tm->QueryField(i, 1));
		di.value = strtounsigned<unsigned int>(tm->QueryField(i, 2), nullptr, 16);
		di.iwd_mod_type = atoi(tm->QueryField(i, 3));
		di.reduction = atoi(tm->QueryField(i, 4));
		DamageInfoMap.insert(std::make_pair(di.value, di));
	}

	return true;
}

bool Interface::ReadReputationModTable() const
{
	AutoTable tm("reputati");
	if (!tm)
		return false;

	reputationmod = (int **) calloc(21, sizeof(int *));
	int cols = tm->GetColumnCount();
	for (unsigned int i=0; i<20; i++) {
		reputationmod[i] = (int *) calloc(cols, sizeof(int));
		for (int j=0; j<cols; j++) {
			reputationmod[i][j] = atoi(tm->QueryField(i, j));
		}
	}

	return true;
}

bool Interface::ReadSoundChannelsTable() const
{
	AutoTable tm("sndchann");
	if (!tm) {
		return false;
	}

	int ivol = tm->GetColumnIndex("VOLUME");
	int irev = tm->GetColumnIndex("REVERB");
	for (ieDword i = 0; i < tm->GetRowCount(); i++) {
		const char *rowname = tm->GetRowName(i);
		// translate some alternative names for the IWDs
		if (!strcmp(rowname, "ACTION")) rowname = "ACTIONS";
		else if (!strcmp(rowname, "SWING")) rowname = "SWINGS";
		AudioDriver->SetChannelVolume(rowname, atoi(tm->QueryField(i, ivol)));
		if (irev != -1) {
			AudioDriver->SetChannelReverb(rowname, atof(tm->QueryField(i, irev)));
		}
	}
	return true;
}

//Not a constant anymore, we let the caller set the entry to zero
char *Interface::GetMusicPlaylist(int SongType) const {
	if (SongType < 0 || (unsigned int)SongType >= musiclist.size())
		return NULL;

	return musiclist[SongType];
}

/** this is the main loop */
void Interface::Main()
{
	ieDword speed = 10;

	vars->Lookup("Mouse Scroll Speed", speed);
	SetMouseScrollSpeed((int) speed);

	const Font* fps = GetTextFont();
	// TODO: if we ever want to support dynamic resolution changes this will break
	Region fpsRgn(0, config.Height - 30, 80, 30);
	wchar_t fpsstring[20] = {L"???.??? fps"};
	// set for printing
	fpsRgn.x = 5;
	fpsRgn.y = 0;

	tick_t frame = 0, time, timebase;
	time = GetTicks();
	timebase = time;
	double frames = 0.0;

	do {
		std::deque<Timer>::iterator it;
		for (it = timers.begin(); it != timers.end();) {
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

		GameLoop();
		// TODO: find other animations that need to be synchronized
		// we can create a manager for them and everything can be updated at once
		GlobalColorCycle.AdvanceTime(time);
		winmgr->DrawWindows();
		time = GetTicks();
		if (config.DrawFPS) {
			frame++;
			if (time - timebase > 1000) {
				frames = ( frame * 1000.0 / ( time - timebase ) );
				timebase = time;
				frame = 0;
				swprintf(fpsstring, sizeof(fpsstring)/sizeof(fpsstring[0]), L"%.3f fps", frames);
			}
			auto lock = winmgr->DrawHUD();
			video->DrawRect( fpsRgn, ColorBlack );
			fps->Print(fpsRgn, String(fpsstring), IE_FONT_ALIGN_MIDDLE | IE_FONT_SINGLE_LINE, {ColorWhite, ColorBlack});
		}
	} while (video->SwapBuffers() == GEM_OK && !(QuitFlag&QF_KILL));
	QuitGame(0);
}

int Interface::LoadSprites()
{
	if (!IsAvailable( IE_2DA_CLASS_ID )) {
		Log(ERROR, "Core", "No 2DA Importer Available.");
		return GEM_ERROR;
	}

	Log(MESSAGE, "Core", "Loading Cursors...");
	const AnimationFactory* anim;
	anim = (const AnimationFactory*) gamedata->GetFactoryResource(MainCursorsImage, IE_BAM_CLASS_ID);
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
		char fileName[32];
		while (CursorCount < 99) {
			snprintf(fileName, sizeof(fileName), "%.29s%02ld", MainCursorsImage.CString(), long(CursorCount));
			ResourceHolder<ImageMgr> im = GetResourceHolder<ImageMgr>(fileName, true);
			if (!im) break;
			Cursors.push_back(im->GetSprite2D());
			CursorCount++;
		}
	}

	// this is the last existing cursor type
	if (CursorCount<IE_CURSOR_WAY) {
		Log(ERROR, "Core", "Failed to load enough cursors (%ld < %d).", long(CursorCount), IE_CURSOR_WAY);
		return GEM_ERROR;
	}
	WindowManager::CursorMouseUp = Cursors[0];
	WindowManager::CursorMouseDown = Cursors[1];

	// Load fog-of-war bitmaps
	anim = (const AnimationFactory*) gamedata->GetFactoryResource("fogowar", IE_BAM_CLASS_ID);
	Log(MESSAGE, "Core", "Loading Fog-Of-War bitmaps...");
	if (!anim || anim->GetCycleSize( 0 ) != 8) {
		// unknown type of fog anim
		Log(ERROR, "Core", "Failed to load Fog-of-War bitmaps.");
		return GEM_ERROR;
	}

	FogSprites[1] = anim->GetFrame( 0, 0 );
	FogSprites[2] = anim->GetFrame( 1, 0 );
	FogSprites[3] = anim->GetFrame( 2, 0 );

	FogSprites[4] = video->MirrorSprite( FogSprites[1], BlitFlags::MIRRORY, false );

	assert(FogSprites[4]->renderFlags&BlitFlags::MIRRORY);

	FogSprites[6] = video->MirrorSprite( FogSprites[3], BlitFlags::MIRRORY, false );

	FogSprites[8] = video->MirrorSprite( FogSprites[2], BlitFlags::MIRRORX, false );
	assert(FogSprites[8]->renderFlags&BlitFlags::MIRRORX);
	FogSprites[9] = video->MirrorSprite( FogSprites[3], BlitFlags::MIRRORX, false );

	FogSprites[12] = video->MirrorSprite( FogSprites[6], BlitFlags::MIRRORX, false );
	assert(FogSprites[12]->renderFlags&BlitFlags::MIRRORX);

	// Load ground circle bitmaps (PST only)
	Log(MESSAGE, "Core", "Loading Ground circle bitmaps...");
	for (int size = 0; size < MAX_CIRCLE_SIZE; size++) {
		if (!GroundCircleBam[size].IsEmpty()) {
			anim = (const AnimationFactory*) gamedata->GetFactoryResource(GroundCircleBam[size], IE_BAM_CLASS_ID);
			if (!anim || anim->GetCycleCount() != 6) {
				// unknown type of circle anim
				Log(ERROR, "Core", "Failed Loading Ground circle bitmaps...");
				return GEM_ERROR;
			}

			for (int i = 0; i < 6; i++) {
				Holder<Sprite2D> sprite = anim->GetFrame(0, (ieByte) i);
				if (GroundCircleScale[size]) {
					sprite = video->SpriteScaleDown( sprite, GroundCircleScale[size] );
				}
				GroundCircles[size][i] = sprite;
			}
		}
	}

	return GEM_OK;
}

int Interface::LoadFonts()
{
	Log(MESSAGE, "Core", "Loading Fonts...");
	AutoTable tab("fonts");
	if (!tab) {
		Log(ERROR, "Core", "Cannot find fonts.2da.");
		return GEM_ERROR;
	}

	// FIXME: we used to try and share like fonts
	// this only ever had a benefit when using non-bam fonts
	// it could potentially save a a few megs of space (nice for handhelds)
	// but we should re-enable this by simply letting Font instances share their atlas data

	int count = tab->GetRowCount();
	const char* rowName = NULL;
	for (int row = 0; row < count; row++) {
		rowName = tab->GetRowName(row);

		ResRef resref = tab->QueryField(rowName, "RESREF");
		const char* font_name = tab->QueryField( rowName, "FONT_NAME" );
		ieWord font_size = atoi( tab->QueryField( rowName, "PX_SIZE" ) ); // not available in BAM fonts.
		FontStyle font_style = (FontStyle)atoi( tab->QueryField( rowName, "STYLE" ) ); // not available in BAM fonts.
		bool background = atoi(tab->QueryField(rowName, "BACKGRND"));

		Font* fnt = NULL;
		ResourceHolder<FontManager> fntMgr = GetResourceHolder<FontManager>(font_name);
		if (fntMgr) fnt = fntMgr->GetFont(font_size, font_style, background);

		if (!fnt) {
			error("Core", "Unable to load font resource: %s for ResRef %s (check fonts.2da)", font_name, resref.CString());
		} else {
			fonts[resref] = fnt;
			Log(MESSAGE, "Core", "Loaded Font: %s for ResRef %s", font_name, resref.CString());
		}
	}

	Log(MESSAGE, "Core", "Fonts Loaded...");
	return GEM_OK;
}

int Interface::Init(InterfaceConfig* cfg)
{
	Log(MESSAGE, "Core", "GemRB core version v" VERSION_GEMRB " loading ...");
	if (!cfg) {
		Log(FATAL, "Core", "No Configuration context.");
		return GEM_ERROR;
	}

	const char* value = NULL;
#define CONFIG_INT(key, var) \
		value = cfg->GetValueForKey(key); \
		if (value) \
			var ( atoi( value ) ); \
		value = nullptr

	CONFIG_INT("Bpp", config.Bpp =);
	CONFIG_INT("CaseSensitive", config.CaseSensitive =);
	CONFIG_INT("DoubleClickDelay", EventMgr::DCDelay = );
	CONFIG_INT("DrawFPS", config.DrawFPS =);
	CONFIG_INT("EnableCheatKeys", EnableCheatKeys);
	CONFIG_INT("EndianSwitch", DataStream::SetBigEndian);
	CONFIG_INT("GCDebug", GameControl::DebugFlags = );
	CONFIG_INT("Height", config.Height =);
	CONFIG_INT("KeepCache", config.KeepCache =);
	CONFIG_INT("MaxPartySize", config.MaxPartySize =);
	config.MaxPartySize = std::min(std::max(1, config.MaxPartySize), 10);
	vars->SetAt("MaxPartySize", config.MaxPartySize); // for simple GUIScript access
	CONFIG_INT("MouseFeedback", config.MouseFeedback =);
	CONFIG_INT("MultipleQuickSaves", config.MultipleQuickSaves =);
	CONFIG_INT("RepeatKeyDelay", Control::ActionRepeatDelay =);
	CONFIG_INT("SaveAsOriginal", config.SaveAsOriginal =);
	CONFIG_INT("DebugMode", config.debugMode =);
	int touchInput = -1;
	CONFIG_INT("TouchInput", touchInput =);
	CONFIG_INT("Width", config.Width =);
	CONFIG_INT("UseSoftKeyboard", config.UseSoftKeyboard =);
	CONFIG_INT("NumFingScroll", config.NumFingScroll =);
	CONFIG_INT("NumFingKboard", config.NumFingKboard =);
	CONFIG_INT("NumFingInfo", config.NumFingInfo =);
	CONFIG_INT("GamepadPointerSpeed", config.GamepadPointerSpeed =);

#undef CONFIG_INT

// first param is the preference name, second is the key from gemrb.cfg.
#define CONFIG_VARS_MAP(var, key) \
		value = cfg->GetValueForKey(key); \
		if (value) \
			vars->SetAt(var, atoi(value)); \
		value = nullptr

#define CONFIG_VARS(key) \
		CONFIG_VARS_MAP(key, key)

	// using vars because my hope is to expose (some of) these in a customized GUIOPT
	// map gemrb.cfg value to the value used by the options (usually an option from game.ini)
	CONFIG_VARS("GUIEnhancements");
	CONFIG_VARS("SkipIntroVideos");

	//put into vars so that reading from game.ini wont overwrite
	CONFIG_VARS_MAP("BitsPerPixel", "Bpp");
	CONFIG_VARS_MAP("Full Screen", "Fullscreen");

#undef CONFIG_VARS
#undef CONFIG_VARS_MAP

#define CONFIG_STRING(key, var, default) \
		value = cfg->GetValueForKey(key); \
		if (value && value[0]) { \
			strlcpy(var, value, sizeof(var)); \
		} else if (default && default[0]) { \
			strlcpy(var, default, sizeof(var)); \
		} else var[0] = '\0'; \
		value = nullptr

	CONFIG_STRING("GameName", config.GameName, GEMRB_STRING); // NOTE: potentially overriden below, once auto GameType is resolved
	CONFIG_STRING("GameType", config.GameType, "auto");

#undef CONFIG_STRING

// assumes that default value does not need to be resolved or fixed in any way
#define CONFIG_PATH(key, var, default) \
		value = cfg->GetValueForKey(key); \
		if (value && value[0]) { \
			strlcpy(var, value, sizeof(var)); \
			ResolveFilePath(var); \
			FixPath(var, true); \
		} else if (default && default[0]) { \
			strlcpy(var, default, sizeof(var)); \
		} else var[0] = '\0'; \
		value = nullptr

	// TODO: make CustomFontPath default cross platform
	CONFIG_PATH("CustomFontPath", config.CustomFontPath, "/usr/share/fonts/TTF");
	CONFIG_PATH("GameCharactersPath", config.GameCharactersPath, "characters");
	CONFIG_PATH("GameDataPath", config.GameDataPath, "data");
	CONFIG_PATH("GameOverridePath", config.GameOverridePath, "override");
	CONFIG_PATH("GamePortraitsPath", config.GamePortraitsPath, "portraits");
	CONFIG_PATH("GameScriptsPath", config.GameScriptsPath, "scripts");
	CONFIG_PATH("GameSoundsPath", config.GameSoundsPath, "sounds");

	// Path configuration
	CONFIG_PATH("GemRBPath", config.GemRBPath,
				CopyGemDataPath(config.GemRBPath, _MAX_PATH));

	CONFIG_PATH("CachePath", config.CachePath, "./Cache2");
	FixPath(config.CachePath, false);

	// AppImage doesn't support relative urls at all
	// we set the path to the data dir to cover unhardcoded and co,
	// while plugins are statically linked, so it doesn't matter for them
	// Also, support running from eg. KDevelop AppImages by checking the name.
#if defined(DATA_DIR) && defined(__linux__)
	const char* appDir = getenv("APPDIR");
	const char* appImageFile = getenv("ARGV0");
	if (appDir) {
		Log(DEBUG, "Interface", "Found APPDIR: %s", appDir);
		Log(DEBUG, "Interface", "Found AppImage/ARGV0: %s", appImageFile);
	}
	if (appDir && appImageFile && strcasestr(appImageFile, "gemrb") != nullptr) {
		assert(strnlen(appDir, _MAX_PATH/2) < _MAX_PATH/2);
		PathJoin(config.GemRBPath, appDir, DATA_DIR, nullptr);
	}
#endif

	CONFIG_PATH("GUIScriptsPath", config.GUIScriptsPath, config.GemRBPath);
	CONFIG_PATH("GamePath", config.GamePath, ".");
	// guess a few paths in case this one is bad; two levels deep for the fhs layout
	char testPath[_MAX_PATH];
	bool gameFound = true;
	if (!PathJoin(testPath, config.GamePath, "chitin.key", nullptr)) {
		Log(WARNING, "Interface", "Invalid GamePath detected, guessing from the current dir!");
		if (PathJoin(testPath, "..", "chitin.key", nullptr)) {
			strlcpy(config.GamePath, "..", sizeof(config.GamePath));
		} else {
			if (PathJoin(testPath, "..", "..", "chitin.key", nullptr)) {
				strlcpy(config.GamePath, "../..", sizeof(config.GamePath));
			} else {
				gameFound = false;
			}
		}
	}
	// if nothing was found still, check for the internal demo
	// it's generic, but not likely to work outside of AppImages and maybe apple bundles
	if (!gameFound) {
		Log(WARNING, "Interface", "Invalid GamePath detected, looking for the GemRB demo!");
		if (PathJoin(testPath, "..", "demo", "chitin.key", nullptr)) {
			PathJoin(config.GamePath, "..", "demo", nullptr);
			strlcpy(config.GameType, "demo", sizeof(config.GameType));
		} else {
			if (PathJoin(testPath, config.GemRBPath, "demo", "chitin.key", nullptr)) {
				PathJoin(config.GamePath, config.GemRBPath, "demo", nullptr);
				strlcpy(config.GameType, "demo", sizeof(config.GameType));
			}
		}
	}

	CONFIG_PATH("GemRBOverridePath", config.GemRBOverridePath, config.GemRBPath);
	CONFIG_PATH("GemRBUnhardcodedPath", config.GemRBUnhardcodedPath, config.GemRBPath);
#ifdef PLUGIN_DIR
	CONFIG_PATH("PluginsPath", config.PluginsPath, PLUGIN_DIR);
#else
	CONFIG_PATH("PluginsPath", config.PluginsPath, "");
	if (!config.PluginsPath[0]) {
		PathJoin(config.PluginsPath, config.GemRBPath, "plugins", nullptr);
	}
#endif

	CONFIG_PATH("SavePath", config.SavePath, config.GamePath);
#undef CONFIG_PATH

#define CONFIG_STRING(key, var) \
		value = cfg->GetValueForKey(key); \
		if (value) \
			var = value; \
		value = nullptr

	CONFIG_STRING("AudioDriver", config.AudioDriverName);
	CONFIG_STRING("VideoDriver", config.VideoDriverName);
	CONFIG_STRING("Encoding", config.Encoding);
#undef CONFIG_STRING

	value = cfg->GetValueForKey("ModPath");
	if (value) {
		for (char *path = strtok((char*)value,SPathListSeparator);
			 path;
			 path = strtok(NULL,SPathListSeparator)) {
			config.ModPath.emplace_back(path);
			ResolveFilePath(config.ModPath.back());
		}
	}
	value = cfg->GetValueForKey("SkipPlugin");
	if (value) {
		plugin_flags->SetAt( value, PLF_SKIP );
	}
	value = cfg->GetValueForKey("DelayPlugin");
	if (value) {
		plugin_flags->SetAt( value, PLF_DELAY );
	}

	for (int i = 0; i < MAX_CD; i++) {
		char keyname[] = { 'C', 'D', char('1'+i), '\0' };
		value = cfg->GetValueForKey(keyname);
		if (value) {
			for(char *path = strtok((char*)value, SPathListSeparator);
				path;
				path = strtok(NULL,SPathListSeparator)) {
				config.CD[i].emplace_back(path);
				ResolveFilePath(config.CD[i].back());
			}
		} else {
			// nothing in config so create our own
			char name[_MAX_PATH];

			PathJoin(name, config.GamePath, keyname, nullptr);
			config.CD[i].emplace_back(name);
			PathJoin(name, config.GamePath, config.GameDataPath, keyname, nullptr);
			config.CD[i].emplace_back(name);
		}
	}

	if (!MakeDirectories(config.CachePath)) {
		error("Core", "Unable to create cache directory '%s'", config.CachePath);
	}

	if (StupidityDetector(config.CachePath)) {
		Log(ERROR, "Core", "Cache path %s doesn't exist, not a folder or contains alien files!", config.CachePath);
		return GEM_ERROR;
	}
	if (!config.KeepCache) DelTree((const char *) config.CachePath, false);

	// potentially disable logging before plugins are loaded (the log file is a plugin)
	value = cfg->GetValueForKey("Logging");
	if (value) ToggleLogging(atoi(value));

	Log(MESSAGE, "Core", "Starting Plugin Manager...");
	const PluginMgr *plugin = PluginMgr::Get();
#if TARGET_OS_MAC
	// search the bundle plugins first
	// since bundle plugins are loaded first dyld will give them precedence
	// if duplicates are found in the PluginsPath
	char bundlePluginsPath[_MAX_PATH];
	CopyBundlePath(bundlePluginsPath, sizeof(bundlePluginsPath), PLUGINS);
	ResolveFilePath(bundlePluginsPath);
#ifndef STATIC_LINK
	LoadPlugins(bundlePluginsPath);
#endif
#endif
#ifndef STATIC_LINK
	LoadPlugins(config.PluginsPath);
#endif
	if (plugin && plugin->GetPluginCount()) {
		Log(MESSAGE, "Core", "Plugin Loading Complete...");
	} else {
		Log(FATAL, "Core", "Plugin Loading Failed, check path...");
		return GEM_ERROR;
	}
	plugin->RunInitializers();

	Log(MESSAGE, "Core", "GemRB Core Initialization...");
	Log(MESSAGE, "Core", "Initializing Video Driver...");
	video = std::shared_ptr<Video>(static_cast<Video*>(PluginMgr::Get()->GetDriver(&Video::ID, config.VideoDriverName.c_str())));
	if (!video) {
		Log(FATAL, "Core", "No Video Driver Available.");
		return GEM_ERROR;
	}
	if (video->Init() == GEM_ERROR) {
		Log(FATAL, "Core", "Cannot Initialize Video Driver.");
		return GEM_ERROR;
	}

	// ask the driver if a touch device is in use
	EventMgr::TouchInputEnabled = touchInput < 0 ? video->TouchInputEnabled() : touchInput;

	ieDword brightness = 10;
	ieDword contrast = 5;
	vars->Lookup("Brightness Correction", brightness);
	vars->Lookup("Gamma Correction", contrast);

	SetInfoTextColor(ColorWhite);

	{
		Log(MESSAGE, "Core", "Initializing Search Path...");
		if (!IsAvailable( PLUGIN_RESOURCE_DIRECTORY )) {
			Log(FATAL, "Core", "no DirectoryImporter!");
			return GEM_ERROR;
		}

		char path[_MAX_PATH];

		PathJoin(path, config.CachePath, nullptr);
		if (!gamedata->AddSource(path, "Cache", PLUGIN_RESOURCE_DIRECTORY)) {
			Log(FATAL, "Core", "The cache path couldn't be registered, please check!");
			return GEM_ERROR;
		}

		size_t i;
		for (i = 0; i < config.ModPath.size(); ++i)
			gamedata->AddSource(config.ModPath[i].c_str(), "Mod paths", PLUGIN_RESOURCE_CACHEDDIRECTORY);

		PathJoin(path, config.GemRBOverridePath, "override", config.GameType, nullptr);
		if (!strcmp(config.GameType, "auto"))
			gamedata->AddSource(path, "GemRB Override", PLUGIN_RESOURCE_NULL);
		else
			gamedata->AddSource(path, "GemRB Override", PLUGIN_RESOURCE_CACHEDDIRECTORY);

		PathJoin(path, config.GemRBOverridePath, "override", "shared", nullptr);
		gamedata->AddSource(path, "shared GemRB Override", PLUGIN_RESOURCE_CACHEDDIRECTORY);

		PathJoin(path, config.GamePath, config.GameOverridePath, nullptr);
		gamedata->AddSource(path, "Override", PLUGIN_RESOURCE_CACHEDDIRECTORY);

		//GAME sounds are intentionally not cached, in IWD there are directory structures,
		//that are not cacheable, also it is totally pointless (this fixed charsounds in IWD)
		PathJoin(path, config.GamePath, config.GameSoundsPath, nullptr);
		gamedata->AddSource(path, "Sounds", PLUGIN_RESOURCE_DIRECTORY);

		PathJoin(path, config.GamePath, config.GameScriptsPath, nullptr);
		gamedata->AddSource(path, "Scripts", PLUGIN_RESOURCE_CACHEDDIRECTORY);

		PathJoin(path, config.GamePath, config.GamePortraitsPath, nullptr);
		gamedata->AddSource(path, "Portraits", PLUGIN_RESOURCE_CACHEDDIRECTORY);

		PathJoin(path, config.GamePath, config.GameDataPath, nullptr);
		gamedata->AddSource(path, "Data", PLUGIN_RESOURCE_CACHEDDIRECTORY);

		// accomodating silly installers that create a data/Data/.* structure
		PathJoin(path, config.GamePath, config.GameDataPath, "Data", nullptr);
		gamedata->AddSource(path, "Data", PLUGIN_RESOURCE_CACHEDDIRECTORY);

		//IWD2 movies are on the CD but not in the BIF
		char *description = strdup("CD1/data");
		for (i = 0; i < MAX_CD; i++) {
			for (size_t j = 0; j < config.CD[i].size(); j++) {
				description[2]='1'+i;
				PathJoin(path, config.CD[i][j].c_str(), config.GameDataPath, nullptr);
				gamedata->AddSource(path, description, PLUGIN_RESOURCE_CACHEDDIRECTORY);
			}
		}
		free(description);

		// most of the old gemrb override files can be found here,
		// so they have a lower priority than the game files and can more easily be modded
		PathJoin(path, config.GemRBUnhardcodedPath, "unhardcoded", config.GameType, nullptr);
		if (!strcmp(config.GameType, "auto")) {
			gamedata->AddSource(path, "GemRB Unhardcoded data", PLUGIN_RESOURCE_NULL);
		} else {
			gamedata->AddSource(path, "GemRB Unhardcoded data", PLUGIN_RESOURCE_CACHEDDIRECTORY);
		}
		PathJoin(path, config.GemRBUnhardcodedPath, "unhardcoded", "shared", nullptr);
		gamedata->AddSource(path, "shared GemRB Unhardcoded data", PLUGIN_RESOURCE_CACHEDDIRECTORY);
	}

	{
		Log(MESSAGE, "Core", "Initializing KEY Importer...");
		char ChitinPath[_MAX_PATH];
		PathJoin(ChitinPath, config.GamePath, "chitin.key", nullptr);
		if (!gamedata->AddSource(ChitinPath, "chitin.key", PLUGIN_RESOURCE_KEY)) {
			Log(FATAL, "Core", "Failed to load \"chitin.key\"");
			Log(ERROR, "Core", "This means:\n- you set the GamePath config variable incorrectly,\n\
- you passed a bad game path to GemRB on the command line,\n\
- you are not running GemRB from within a game dir,\n\
- or the game is running (Windows only).");
			Log(ERROR, "Core", "The path must point to a game directory with a readable chitin.key file.");
			return GEM_ERROR;
		}
	}

	Log(MESSAGE, "Core", "Initializing GUI Script Engine...");
	SetNextScript("Start"); // Start is the first script executed
	guiscript = MakePluginHolder<ScriptEngine>(IE_GUI_SCRIPT_CLASS_ID);
	if (guiscript == nullptr) {
		Log(FATAL, "Core", "Missing GUI Script Engine.");
		return GEM_ERROR;
	}
	if (!guiscript->Init()) {
		Log(FATAL, "Core", "Failed to initialize GUI Script.");
		return GEM_ERROR;
	}

	// re-set the gemrb override path, since we now have the correct GameType if 'auto' was used
	char path[_MAX_PATH];
	PathJoin(path, config.GemRBOverridePath, "override", config.GameType, nullptr);
	gamedata->AddSource(path, "GemRB Override", PLUGIN_RESOURCE_CACHEDDIRECTORY, RM_REPLACE_SAME_SOURCE);
	char unhardcodedTypePath[_MAX_PATH * 2];
	PathJoin(unhardcodedTypePath, config.GemRBUnhardcodedPath, "unhardcoded", config.GameType, nullptr);
	gamedata->AddSource(unhardcodedTypePath, "GemRB Unhardcoded data", PLUGIN_RESOURCE_CACHEDDIRECTORY, RM_REPLACE_SAME_SOURCE);

	// Purposely add the font directory last since we will only ever need it at engine load time.
	if (config.CustomFontPath[0]) gamedata->AddSource(config.CustomFontPath, "CustomFonts", PLUGIN_RESOURCE_DIRECTORY);

	Log(MESSAGE, "Core", "Reading Game Options...");
	if (!LoadGemRBINI()) {
		Log(FATAL, "Core", "Cannot Load INI.");
		return GEM_ERROR;
	}

	// SDL2 driver requires the display to be created prior to sprite creation (opengl context)
	// we also need the display to exist to create sprites using the display format
	ieDword fullscreen = 0;
	vars->Lookup("Full Screen", fullscreen);
	if (video->CreateDisplay(Size(config.Width, config.Height), config.Bpp, fullscreen, config.GameName) == GEM_ERROR) {
		Log(FATAL, "Core", "Cannot initialize shaders.");
		return GEM_ERROR;
	}
	video->SetGamma(brightness, contrast);
	
	// if unset, manually populate GameName (window title)
	if (!stricmp(config.GameName, GEMRB_STRING)) {
		if (stricmp(DefaultWindowTitle.c_str(), GEMRB_STRING) != 0) {
			std::string title = GEMRB_STRING" running " + DefaultWindowTitle;
			strlcpy(config.GameName, title.c_str(), sizeof(config.GameName));
		} else {
			strlcpy(config.GameName, GEMRB_STRING" running unknown game", sizeof(config.GameName));
		}
		video->SetWindowTitle(config.GameName);
	}

	// load the game ini (baldur.ini, torment.ini, icewind.ini ...)
	// read from our version of the config if it is present
	char ini_path[_MAX_PATH+4] = { '\0' };
	char gemrbINI[_MAX_PATH+4] = { '\0' };
	char tmp[_MAX_PATH+4] = { '\0' };
	snprintf(gemrbINI, sizeof(gemrbINI), "gem-%s", INIConfig);
	PathJoin(ini_path, config.SavePath, gemrbINI, nullptr);
	if (!file_exists(ini_path)) {
		PathJoin(ini_path, config.GamePath, gemrbINI, nullptr);
	}
	if (file_exists(ini_path)) {
		strlcpy(tmp, INIConfig, sizeof(tmp));
		strlcpy(INIConfig, gemrbINI, sizeof(INIConfig));
	} else {
		PathJoin(ini_path, config.GamePath, INIConfig, nullptr);
		Log(MESSAGE,"Core", "Loading original game options from %s", ini_path);
	}
	if (!InitializeVarsWithINI(ini_path)) {
		Log(WARNING, "Core", "Unable to set dictionary default values!");
	}

	// set up the tooltip delay which we store in milliseconds
	ieDword tooltipDelay = 0;
	GetDictionary()->Lookup("Tooltips", tooltipDelay);
	WindowManager::SetTooltipDelay(tooltipDelay * Tooltip::DELAY_FACTOR / 10);

	// restore the game config name if we read it from our version
	if (tmp[0]) {
		strlcpy(INIConfig, tmp, sizeof(INIConfig));
	} else {
		strlcpy(tmp, INIConfig, sizeof(tmp));
	}
	// also store it for base GAM and SAV files
	strtok(tmp, ".");
	GameNameResRef = tmp;

	Log(MESSAGE, "Core", "Reading Encoding Table...");
	if (!LoadEncoding()) {
		Log(ERROR, "Core", "Cannot Load Encoding.");
	}

	Log(MESSAGE, "Core", "Creating Projectile Server...");
	projserv = new ProjectileServer();

	Log(MESSAGE, "Core", "Checking for Dialogue Manager...");
	if (!IsAvailable( IE_TLK_CLASS_ID )) {
		Log(FATAL, "Core", "No TLK Importer Available.");
		return GEM_ERROR;
	}
	strings = MakePluginHolder<StringMgr>(IE_TLK_CLASS_ID);
	Log(MESSAGE, "Core", "Loading Dialog.tlk file...");
	char strpath[_MAX_PATH];
	PathJoin(strpath, config.GamePath, "dialog.tlk", nullptr);
	FileStream* fs = FileStream::OpenFile(strpath);
	if (!fs) {
		Log(FATAL, "Core", "Cannot find Dialog.tlk.");
		return GEM_ERROR;
	}
	strings->Open(fs);

	// does the language use an extra tlk?
	if (strings->HasAltTLK()) {
		strings2 = MakePluginHolder<StringMgr>(IE_TLK_CLASS_ID);
		Log(MESSAGE, "Core", "Loading DialogF.tlk file...");
		char strpath[_MAX_PATH];
		PathJoin(strpath, config.GamePath, "dialogf.tlk", nullptr);
		FileStream* fs = FileStream::OpenFile(strpath);
		if (!fs) {
			Log(ERROR, "Core", "Cannot find DialogF.tlk. Let us know which translation you are using.");
			Log(ERROR, "Core", "Falling back to main TLK file, so female text may be wrong!");
			strings2 = strings;
		} else {
			strings2->Open(fs);
		}
	}

	{
		Log(MESSAGE, "Core", "Loading Palettes...");
		LoadPalette<16>(Palette16, palettes16);
		LoadPalette<32>(Palette32, palettes32);
		LoadPalette<256>(Palette256, palettes256);
		Log(MESSAGE, "Core", "Palettes Loaded");
	}

	if (!IsAvailable( IE_BAM_CLASS_ID )) {
		Log(FATAL, "Core", "No BAM Importer Available.");
		return GEM_ERROR;
	}

	Log(MESSAGE, "Core", "Initializing stock sounds...");
	if (!gamedata->ReadResRefTable(ResRef("defsound"), gamedata->defaultSounds)) {
		Log(FATAL, "Core", "Cannot find defsound.2da.");
		return GEM_ERROR;
	}

	int ret = LoadSprites();
	if (ret) return ret;

	ret = LoadFonts();
	if (ret) return ret;

	Log(MESSAGE, "Core", "Initializing Window Manager...");
	winmgr = new WindowManager(video);
	RegisterScriptableWindow(winmgr->GetGameWindow(), "GAMEWIN", 0);
	winmgr->SetCursorFeedback(WindowManager::CursorFeedback(config.MouseFeedback));

	guifact = GetImporter<GUIFactory>(IE_CHU_CLASS_ID);
	if (!guifact) {
		Log(FATAL, "Core", "Failed to load Window Manager.");
		return GEM_ERROR;
	}
	guifact->SetWindowManager(*winmgr);

	QuitFlag = QF_CHANGESCRIPT;

	Log(MESSAGE, "Core", "Starting up the Sound Driver...");
	AudioDriver = std::shared_ptr<Audio>(static_cast<Audio*>(PluginMgr::Get()->GetDriver(&Audio::ID, config.AudioDriverName.c_str())));
	if (AudioDriver == nullptr) {
		Log(FATAL, "Core", "Failed to load sound driver.");
		return GEM_ERROR;
	}
	if (!AudioDriver->Init()) {
		Log(FATAL, "Core", "Failed to initialize sound driver.");
		return GEM_ERROR;
	}

	Log(MESSAGE, "Core", "Allocating SaveGameIterator...");
	sgiterator = new SaveGameIterator();
	if (sgiterator == NULL) {
		Log(FATAL, "Core", "Failed to allocate SaveGameIterator.");
		return GEM_ERROR;
	}

	Log(MESSAGE, "Core", "Initializing Music Manager...");
	music = MakePluginHolder<MusicMgr>(IE_MUS_CLASS_ID);
	if (!music) {
		Log(FATAL, "Core", "Failed to load Music Manager.");
		return GEM_ERROR;
	}

	Log(MESSAGE, "Core", "Loading music list...");
	if (HasFeature( GF_HAS_SONGLIST )) {
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

	int resdata = HasFeature( GF_RESDATA_INI );
	if (resdata || HasFeature(GF_SOUNDS_INI) ) {
		Log(MESSAGE, "Core", "Loading resource data File...");
		INIresdata = MakePluginHolder<DataFileMgr>(IE_INI_CLASS_ID);
		DataStream* ds = gamedata->GetResource(resdata? "resdata":"sounds", IE_INI_CLASS_ID);
		if (!INIresdata->Open(ds)) {
			Log(WARNING, "Core", "Failed to load resource data.");
		}
	}

	Log(MESSAGE, "Core", "Setting up SFX channels...");
	ret = ReadSoundChannelsTable();
	if (!ret) {
		Log(WARNING, "Core", "Failed to read channel table.");
	}

	if (HasFeature( GF_HAS_PARTY_INI )) {
		Log(MESSAGE, "Core", "Loading precreated teams setup...");
		INIparty = MakePluginHolder<DataFileMgr>(IE_INI_CLASS_ID);
		char tINIparty[_MAX_PATH];
		PathJoin(tINIparty, config.GamePath, "Party.ini", nullptr);
		FileStream* fs = FileStream::OpenFile( tINIparty );
		if (!INIparty->Open(fs)) {
			Log(WARNING, "Core", "Failed to load precreated teams.");
		}
	}

	if (HasFeature(GF_IWD2_DEATHVARFORMAT)) {
		DeathVarFormat = IWD2DeathVarFormat;
	}

	if (HasFeature( GF_HAS_BEASTS_INI )) {
		Log(MESSAGE, "Core", "Loading beasts definition File...");
		INIbeasts = MakePluginHolder<DataFileMgr>(IE_INI_CLASS_ID);
		char tINIbeasts[_MAX_PATH];
		PathJoin(tINIbeasts, config.GamePath, "beast.ini", nullptr);
		FileStream* fs = FileStream::OpenFile( tINIbeasts );
		if (!INIbeasts->Open(fs)) {
			Log(WARNING, "Core", "Failed to load beast definitions.");
		}

		Log(MESSAGE, "Core", "Loading quests definition File...");
		INIquests = MakePluginHolder<DataFileMgr>(IE_INI_CLASS_ID);
		char tINIquests[_MAX_PATH];
		PathJoin(tINIquests, config.GamePath, "quests.ini", nullptr);
		FileStream* fs2 = FileStream::OpenFile( tINIquests );
		if (!INIquests->Open(fs2)) {
			Log(WARNING, "Core", "Failed to load quest definitions.");
		}
	}
	game = NULL;
	calendar = NULL;
	keymap = NULL;

	Log(MESSAGE, "Core", "Initializing effects...");
	ret = Init_EffectQueue();
	if (!ret) {
		Log(FATAL, "Core", "Failed to initialize effects.");
		return GEM_ERROR;
	}

	Log(MESSAGE, "Core", "Initializing Inventory Management...");
	ret = InitItemTypes();
	if (!ret) {
		Log(FATAL, "Core", "Failed to initialize inventory.");
		return GEM_ERROR;
	}

	Log(MESSAGE, "Core", "Initializing string constants...");
	displaymsg = new DisplayMessage();
	if (!displaymsg) {
		Log(FATAL, "Core", "Failed to initialize string constants.");
		return GEM_ERROR;
	}

	Log(MESSAGE, "Core", "Initializing random treasure...");
	ret = ReadRandomItems();
	if (!ret) {
		Log(WARNING, "Core", "Failed to initialize random treasure.");
	}
	
	abilityTables = make_unique<AbilityTables>(MaximumAbility);

	Log(MESSAGE, "Core", "Reading reputation mod table...");
	ret = ReadReputationModTable();
	if (!ret) {
		Log(WARNING, "Core", "Failed to read reputation mod table.");
	}

	if ( gamedata->Exists("WMAPLAY", IE_2DA_CLASS_ID) ) {
		Log(MESSAGE, "Core", "Initializing area aliases...");
		ret = ReadAreaAliasTable( "WMAPLAY" );
		if (!ret) {
			Log(WARNING, "Core", "Failed to load area aliases...");
		}
	}

	Log(MESSAGE, "Core", "Reading game time table...");
	ret = ReadGameTimeTable();
	if (!ret) {
		Log(FATAL, "Core", "Failed to read game time table...");
		return GEM_ERROR;
	}

	Log(MESSAGE, "Core", "Reading special spells table...");
	ret = ReadSpecialSpells();
	if (!ret) {
		Log(WARNING, "Core", "Failed to load special spells.");
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

	// dump the potentially changed unhardcoded path to a file that weidu looks at automatically to get our search paths
	char pathString[_MAX_PATH * 3];
#ifdef HAVE_REALPATH
	if (unhardcodedTypePath[0] == '.') {
		// canonicalize the relative path; usually from running from the build dir
		char *absolutePath = realpath(unhardcodedTypePath, NULL);
		if (absolutePath) {
			strlcpy(unhardcodedTypePath, absolutePath, sizeof(unhardcodedTypePath));
			free(absolutePath);
		}
	}
#endif
	snprintf(pathString, sizeof(pathString), "GemRB_Data_Path = %s", unhardcodedTypePath);
	PathJoin(strpath, config.GamePath, "gemrb_path.txt", nullptr);
	FileStream *pathFile = new FileStream();
	// don't abort if something goes wrong, since it should never happen and it's not critical
	if (pathFile->Create(strpath)) {
		pathFile->Write(pathString, strlen(pathString));
		pathFile->Close();
	}
	delete pathFile;

	EventMgr::EventCallback ToggleConsole = [this](const Event& e) {
		if (e.type != Event::KeyDown) return false;

		guiscript->RunFunction("Console", "ToggleConsole");
		return true;
	};
	EventMgr::RegisterHotKeyCallback(ToggleConsole, ' ', GEM_MOD_CTRL);

	return GEM_OK;
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

ProjectileServer* Interface::GetProjectileServer() const
{
	return projserv;
}

Video* Interface::GetVideoDriver() const
{
	return video.get();
}

Audio* Interface::GetAudioDrv(void) const {
	return AudioDriver.get();
}

const char* Interface::TypeExt(SClass_ID type) const
{
	switch (type) {
		case IE_2DA_CLASS_ID:
			return "2da";

		case IE_ACM_CLASS_ID:
			return "acm";

		case IE_ARE_CLASS_ID:
			return "are";

		case IE_BAM_CLASS_ID:
			return "bam";

		case IE_BCS_CLASS_ID:
			return "bcs";

		case IE_BS_CLASS_ID:
			return "bs";

		case IE_BIF_CLASS_ID:
			return "bif";

		case IE_BIO_CLASS_ID:
			if (HasFeature(GF_BIOGRAPHY_RES)) {
				return "res";
			}
			return "bio";

		case IE_BMP_CLASS_ID:
			return "bmp";

		case IE_PNG_CLASS_ID:
			return "png";

		case IE_CHR_CLASS_ID:
			return "chr";

		case IE_CHU_CLASS_ID:
			return "chu";

		case IE_CRE_CLASS_ID:
			return "cre";

		case IE_DLG_CLASS_ID:
			return "dlg";

		case IE_EFF_CLASS_ID:
			return "eff";

		case IE_GAM_CLASS_ID:
			return "gam";

		case IE_IDS_CLASS_ID:
			return "ids";

		case IE_INI_CLASS_ID:
			return "ini";

		case IE_ITM_CLASS_ID:
			return "itm";

		case IE_MOS_CLASS_ID:
			return "mos";

		case IE_MUS_CLASS_ID:
			return "mus";

		case IE_MVE_CLASS_ID:
			return "mve";

		case IE_OGG_CLASS_ID:
			return "ogg";

		case IE_PLT_CLASS_ID:
			return "plt";

		case IE_PRO_CLASS_ID:
			return "pro";

		case IE_SAV_CLASS_ID:
			return "sav";

		case IE_SPL_CLASS_ID:
			return "spl";

		case IE_SRC_CLASS_ID:
			return "src";

		case IE_STO_CLASS_ID:
			return "sto";

		case IE_TIS_CLASS_ID:
			return "tis";

		case IE_TLK_CLASS_ID:
			return "tlk";

		case IE_TOH_CLASS_ID:
			return "toh";

		case IE_TOT_CLASS_ID:
			return "tot";

		case IE_VAR_CLASS_ID:
			return "var";

		case IE_VEF_CLASS_ID:
			return "vef";

		case IE_VVC_CLASS_ID:
			return "vvc";

		case IE_WAV_CLASS_ID:
			return "wav";

		case IE_WED_CLASS_ID:
			return "wed";

		case IE_WFX_CLASS_ID:
			return "wfx";

		case IE_WMP_CLASS_ID:
			return "wmp";

		default:
			Log(ERROR, "Interface", "No extension associated to class ID: %lu", type);
	}
	return NULL;
}

ieStrRef Interface::UpdateString(ieStrRef strref, const char *text) const
{
	char *current = GetCString(strref, 0);
	bool changed = strcmp(text, current) != 0;
	free(current);
	if (changed) {
		return strings->UpdateString( strref, text );
	} else {
		return strref;
	}
}

char* Interface::GetCString(ieStrRef strref, ieDword options) const
{
	ieDword flags = 0;

	if (!(options & IE_STR_STRREFOFF)) {
		vars->Lookup( "Strref On", flags );
	}
	if (strings2 && (signed)strref != -1 && strref & IE_STR_ALTREF) {
		return strings2->GetCString(strref, flags | options);
	} else {
		return strings->GetCString(strref, flags | options);
	}
}

String* Interface::GetString(ieStrRef strref, ieDword options) const
{
	ieDword flags = 0;

	if (!(options & IE_STR_STRREFOFF)) {
		vars->Lookup( "Strref On", flags );
	}

	if (strings2 && (signed)strref != -1 && strref & IE_STR_ALTREF) {
		return strings2->GetString(strref, flags | options);
	} else {
		return strings->GetString(strref, flags | options);
	}
}

void Interface::SetFeature(int flag, int position)
{
	if (flag) {
		GameFeatures[position>>5] |= 1<<(position&31);
	} else {
		GameFeatures[position>>5] &= ~(1<<(position&31) );
	}
}

ieDword Interface::HasFeature(int position) const
{
	return GameFeatures[position>>5] & (1<<(position&31));
}

static const char* const game_flags[GF_COUNT + 1]={
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
		"HasDescIcon",        //13GF_HAS_DESC_ICON
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
		"RareActionSounds",   //66GF_RARE_ACTION_VB
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
		NULL                  //for our own safety, this marks the end of the pole
};

/** Loads gemrb.ini */
bool Interface::LoadGemRBINI()
{
	DataStream* inifile = gamedata->GetResource( "gemrb", IE_INI_CLASS_ID );
	if (! inifile) {
		return false;
	}

	Log(MESSAGE, "Core", "Loading game type-specific GemRB setup '%s'",
		inifile->originalfile);

	if (!IsAvailable( IE_INI_CLASS_ID )) {
		Log(ERROR, "Core", "No INI Importer Available.");
		return false;
	}
	PluginHolder<DataFileMgr> ini = MakePluginHolder<DataFileMgr>(IE_INI_CLASS_ID);
	ini->Open(inifile);

	ResRef tooltipBG;
	const char *s;
	// Resrefs are already initialized in Interface::Interface()
#define ASSIGN_RESREF(resref, name) \
	s = ini->GetKeyAsString( "resources", name, NULL ); \
	resref = s; s = nullptr

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
		const AnimationFactory* anim = (const AnimationFactory*) gamedata->GetFactoryResource(tooltipBG, IE_BAM_CLASS_ID);
		Log(MESSAGE, "Core", "Initializing Tooltips...");
		if (anim) {
			TooltipBG = new TooltipBackground(anim->GetFrame(0, 0), anim->GetFrame(0, 1), anim->GetFrame(0, 2) );
			// FIXME: this is an arbitrary heuristic and speed
			TooltipBG->SetAnimationSpeed((ttMargin == 5) ? 10 : 0);
			TooltipBG->SetMargin(ttMargin);
		}
	}

	DefaultWindowTitle = ini->GetKeyAsString("resources", "WindowTitle", GEMRB_STRING);

	// These are values for how long a single step is, see Movable::DoStep.
	// They were found via trial-and-error, trying to match
	// the speeds from the original games.
	gamedata->SetStepTime(ini->GetKeyAsInt("resources", "StepTime", 566)); // Defaults to BG2's value

	// The format of GroundCircle can be:
	// GroundCircleBAM1 = wmpickl/3
	// to denote that the bitmap should be scaled down 3x
	for (int size = 0; size < MAX_CIRCLE_SIZE; size++) {
		char name[30];
		sprintf( name, "GroundCircleBAM%d", size+1 );
		s = ini->GetKeyAsString( "resources", name, NULL );
		if (s) {
			const char *pos = strchr( s, '/' );
			if (pos) {
				GroundCircleScale[size] = atoi( pos+1 );
				char tmp[9];
				strlcpy(tmp, s, pos - s + 1);
				GroundCircleBam[size] = tmp;
			} else {
				GroundCircleBam[size] = s;
			}
		}
	}

	s = ini->GetKeyAsString( "resources", "INIConfig", NULL );
	if (s)
		strlcpy(INIConfig, s, sizeof(INIConfig));

	MaximumAbility = ini->GetKeyAsInt ("resources", "MaximumAbility", 25 );
	NumRareSelectSounds = ini->GetKeyAsInt("resources", "NumRareSelectSounds", 2);
	gamedata->SetTextSpeed(ini->GetKeyAsInt("resources", "TextScreenSpeed", 100));

	for (uint32_t i = 0; i < GF_COUNT; i++) {
		if (!game_flags[i]) {
			error("Core", "Fix the game flags!\n");
		}
		SetFeature( ini->GetKeyAsInt( "resources", game_flags[i], 0 ), i );
		//printMessage("Option", "", GREEN);
		//print("%s = %s", game_flags[i], HasFeature(i)?"yes":"no");
	}

	// fix the resolution default if needed
	config.Width = std::max(config.Width, ini->GetKeyAsInt("resources", "MinWidth", 800));
	config.Height = std::max(config.Height, ini->GetKeyAsInt("resources", "MinHeight", 600));

	return true;
}

/** Load the encoding table selected in gemrb.cfg */
bool Interface::LoadEncoding()
{
	DataStream* inifile = gamedata->GetResource(config.Encoding.c_str(), IE_INI_CLASS_ID);
	if (! inifile) {
		return false;
	}

	Log(MESSAGE, "Core", "Loading encoding definition for %s: '%s'", config.Encoding.c_str(),
		inifile->originalfile);

	PluginHolder<DataFileMgr> ini = MakePluginHolder<DataFileMgr>(IE_INI_CLASS_ID);
	ini->Open(inifile);

	TLKEncoding.encoding = ini->GetKeyAsString("encoding", "TLKEncoding", TLKEncoding.encoding.c_str());
	TLKEncoding.zerospace = ini->GetKeyAsBool("encoding", "NoSpaces", false);

	//TextArea::SetNoteString( ini->GetKeyAsString( "strings", "NoteString", NULL ) );

	// TODO: lists are incomplete
	// maybe want to externalize this
	// list compiled form wiki: https://gemrb.org/Text-encodings.html
	const char* wideEncodings[] = {
		// Chinese
		"GBK", "BIG5",
		// Korean
		"EUCKR",
		// Japanese
		"SJIS",
	};
	size_t listSize = sizeof(wideEncodings) / sizeof(wideEncodings[0]);

	for (size_t i = 0; i < listSize; i++) {
		if (TLKEncoding.encoding == wideEncodings[i]) {
			TLKEncoding.widechar = true;
			break;
		}
	}

	const char* multibyteEncodings[] = {
		"UTF-8",
	};
	listSize = sizeof(multibyteEncodings) / sizeof(multibyteEncodings[0]);

	for (size_t i = 0; i < listSize; i++) {
		if (TLKEncoding.encoding == multibyteEncodings[i]) {
			TLKEncoding.multibyte = true;
			break;
		}
	}

	const char *s;
	unsigned int cc = (unsigned int) ini->GetKeyAsInt ("charset", "CharCount", 0);
	if (cc>99) cc=99;
	while(cc--) {
		char key[10];
		snprintf(key,9,"Letter%d", cc+1);
		s = ini->GetKeyAsString( "charset", key, NULL );
		if (s) {
			const char *s2 = strchr(s,',');
			if (s2) {
				unsigned char upper = atoi(s);
				unsigned char lower = atoi(s2+1);
				pl_uppercase[lower] = upper;
				pl_lowercase[upper] = lower;
			}
		}
	}

	return true;
}

/** Returns a preloaded Font */
Font* Interface::GetFont(const ResRef& ResRef) const
{
	std::map<GemRB::ResRef,Font *>::const_iterator i = fonts.find(ResRef);
	if (i != fonts.end()) {
		return i->second;
	}
	return NULL;
}

Font* Interface::GetTextFont() const
{
	return GetFont( TextFontResRef );
}

Font* Interface::GetButtonFont() const
{
	return GetFont( ButtonFontResRef );
}

/** Get GUI Script Manager */
ScriptEngine* Interface::GetGUIScriptEngine() const
{
	return guiscript.get();
}

static EffectRef fx_summon_disable_ref = { "AvatarRemovalModifier", -1 };

//NOTE: if there were more summoned creatures, it will return only the last
Actor *Interface::SummonCreature(const ResRef& resource, const ResRef& animRes, Scriptable *Owner, const Actor *target, const Point &position, int eamod, int level, Effect *fx, bool sexmod)
{
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
	if (!map) return ab;

	if (Owner && Owner->Type == ST_ACTOR) {
		summoner = (Actor *) Owner;
	}

	while (cnt--) {
		Actor *tmp = gamedata->GetCreature(resource);
		if (!tmp) {
			return NULL;
		}

		//if summoner is an actor, filter out opponent summons
		//this is done by clearing the filter when appropriate
		//non actors and neutrals can summon as much as they want
		ieDword flag = GA_NO_DEAD|GA_NO_ALLY|GA_NO_ENEMY;

		if (summoner) {
			tmp->LastSummoner = Owner->GetGlobalID();
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
			displaymsg->DisplayConstantString(STR_SUMMONINGLIMIT, DMC_WHITE);
			delete tmp;
			break;
		}

		ab = tmp;
		//Always use Base stats for the recently summoned creature

		int enemyally;

		if (eamod==EAM_SOURCEALLY || eamod==EAM_SOURCEENEMY) {
			if (summoner) {
				enemyally = summoner->GetStat(IE_EA) > EA_GOODCUTOFF;
			} else {
				enemyally = true;
			}
		} else {
			if (target) {
				enemyally = target->GetBase(IE_EA)>EA_GOODCUTOFF;
			} else {
				enemyally = true;
			}
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
		ab->SetPosition(position, true, 0, 0, ab->size);
		ab->RefreshEffects(NULL);

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
					newfx->Duration = vvc->GetSequenceDuration(AI_UPDATE_TIME)*9/10 + core->GetGame()->GameTime;
					ApplyEffect(newfx, ab, ab);
				}
			}
		}

		//remove the xp value of friendly summons
		if (ab->BaseStats[IE_EA]<EA_GOODCUTOFF) {
			ab->SetBase(IE_XPVALUE, 0);
		}
		if (fx) {
			ApplyEffect(fx, ab, Owner);
		}

		//this check should happen after the fact
		level -= ab->GetBase(IE_XP);
		if(level<0 || ab->GetBase(IE_XP) == 0) {
			break;
		}

	}
	return ab;
}

/** Loads a Window in the Window Manager */
Window* Interface::LoadWindow(ScriptingId WindowID, const ScriptingGroup_t& ref, Window::WindowPosition pos)
{
	if (ref) // is the winpack changing?
		guifact->LoadWindowPack(ref);

	Window* win = GetWindow(WindowID, ref);
	if (!win) {
		win = guifact->GetWindow( WindowID );
	}
	if (win) {
		assert(win->GetScriptingRef());
		win->SetPosition(pos);
		winmgr->FocusWindow( win );
	}
	return win;
}

/** Creates a Window in the Window Manager */
Window* Interface::CreateWindow(unsigned short WindowID, const Region& frame)
{
	// FIXME: this will create a window under the current "window pack"
	// obviously its possible the id can conflict with an existing window
	return guifact->CreateWindow(WindowID, frame);
}

inline void SetGroupViewFlags(const std::vector<View*>& views, unsigned int flags, int op)
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
		game->SetControlStatus(CS_HIDEGUI, (visible) ? OP_NAND : OP_OR );
	}

	std::vector<View*> views = GetViews(group);
	SetGroupViewFlags(views, View::Invisible, (visible) ? OP_NAND : OP_OR);
}

void Interface::ToggleViewsEnabled(bool enabled, const ScriptingGroup_t& group) const
{
	std::vector<View*> views = GetViews(group);
	SetGroupViewFlags(views, View::Disabled, (enabled) ? OP_NAND : OP_OR);
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
		ieDword var = (ieDword) -3;
		vars->Lookup("DialogChoose", var);
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
			ieDword newvar = 0; vars->Lookup("DialogChoose", newvar);
			if (var == (ieDword) -1 || newvar != (ieDword) -1) {
				vars->SetAt("DialogChoose", (ieDword) -3);
			}
		}
		if (flg & DF_OPENCONTINUEWINDOW) {
			guiscript->RunFunction( "GUIWORLD", "OpenContinueMessageWindow" );
			gc->SetDialogueFlags(DF_OPENCONTINUEWINDOW|DF_OPENENDWINDOW, OP_NAND);
		} else if (flg & DF_OPENENDWINDOW) {
			guiscript->RunFunction( "GUIWORLD", "OpenEndMessageWindow" );
			gc->SetDialogueFlags(DF_OPENCONTINUEWINDOW|DF_OPENENDWINDOW, OP_NAND);
		}
	}

	//handling container
	if (CurrentContainer && UseContainer) {
		if (!(flg & DF_IN_CONTAINER) ) {
			gc->SetDialogueFlags(DF_IN_CONTAINER, OP_OR);
			guiscript->RunFunction( "CommonWindow", "OpenContainerWindow" );
		}
	} else {
		if (flg & DF_IN_CONTAINER) {
			gc->SetDialogueFlags(DF_IN_CONTAINER, OP_NAND);
			guiscript->RunFunction( "CommonWindow", "CloseContainerWindow" );
		}
	}
	//end of gui hacks
}

Tooltip Interface::CreateTooltip() const
{
	Font::PrintColors colors;
	colors.fg = gamedata->GetColor("TOOLTIP");
	colors.bg = gamedata->GetColor("TOOLTIPBG");

	TooltipBackground* bg = NULL;
	if (TooltipBG) {
		bg = new TooltipBackground(*TooltipBG);
	}
	return Tooltip(L"", GetFont(TooltipFontResRef), colors, bg);
}

/** Get the Sound Manager */
SaveGameIterator* Interface::GetSaveGameIterator() const
{
	return sgiterator;
}

void Interface::AskAndExit()
{
	// if askExit is 1 then we are trying to quit a second time and should instantly do so
	ieDword askExit = 0;
	vars->Lookup("AskAndExit", askExit);
	if (game && !askExit) {
		SetPause(PAUSE_ON);
		vars->SetAt("AskAndExit", 1);

		guifact->LoadWindowPack("GUIOPT");
		guiscript->RunFunction("GUIOPT", "OpenQuitMsgWindow");
		Log(MESSAGE, "Info", "Press ctrl-c (or close the window) again to quit GemRB.\n");
	} else {
		ExitGemRB();
	}
}

void Interface::ExitGemRB()
{
	const Console* console = GetControl<Console>("CONSOLE_CTL", 1);
	if (console) console->SaveHistory();

	QuitFlag |= QF_KILL;
}
/** Returns the variables dictionary */
Variables* Interface::GetDictionary() const
{
	return vars;
}
/** Returns the token dictionary */
Variables* Interface::GetTokenDictionary() const
{
	return tokens;
}
/** Get the Music Manager */
MusicMgr* Interface::GetMusicMgr() const
{
	return music.get();
}
/** Loads an IDS Table, returns -1 on error or the Symbol Table Index on success */
int Interface::LoadSymbol(const char* ResRef)
{
	int ind = GetSymbolIndex( ResRef );
	if (ind != -1) {
		return ind;
	}
	DataStream* str = gamedata->GetResource( ResRef, IE_IDS_CLASS_ID );
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
	Symbol s = { sm, ResRef };
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
int Interface::GetSymbolIndex(const char* ResRef) const
{
	for (size_t i = 0; i < symbols.size(); i++) {
		if (!symbols[i].sm)
			continue;
		if (symbols[i].symbolName == ResRef)
			return ( int ) i;
	}
	return -1;
}
/** Gets a Loaded Symbol Table by its index, returns NULL on error */
std::shared_ptr<SymbolMgr> Interface::GetSymbol(unsigned int index) const
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
int Interface::PlayMovie(const char* resref)
{
	const char *realResRef = resref;

	//check whether there is an override for this movie
	const char *sound_resref = NULL;
	AutoTable mvesnd = AutoTable("mvesnd", true);
	if (mvesnd) {
		int row = mvesnd->GetRowIndex(resref);
		if (row != -1) {
			int mvecol = mvesnd->GetColumnIndex("override");
			if (mvecol != -1) {
				realResRef = mvesnd->QueryField(row, mvecol);
			}
			int sndcol = mvesnd->GetColumnIndex("sound_override");
			if (sndcol != -1) {
				sound_resref = mvesnd->QueryField(row, sndcol);
			}
		}
	}

	ResourceHolder<MoviePlayer> mp = GetResourceHolder<MoviePlayer>(realResRef);
	if (!mp) {
		return -1;
	}

	//one of these two should exist (they both mean the same thing)
	ieDword subtitles = true;
	vars->Lookup("Display Movie Subtitles", subtitles);
	if (!subtitles) {
		vars->Lookup("Display Subtitles", subtitles);
	}

	mp->EnableSubtitles(subtitles);

	class IESubtitles : public MoviePlayer::SubtitleSet {
		using FrameMap = std::map<size_t, ieStrRef>;
		FrameMap subs;
		mutable size_t nextSubFrame;
		mutable String* cachedSub;

	public:
		// default color taken from BGEE.lua
		IESubtitles(class Font* fnt, const ResRef& resref, const Color& col = Color(0xe9, 0xe2, 0xca, 0xff))
		: MoviePlayer::SubtitleSet(fnt, col)
		{
			AutoTable sttable(resref);
			cachedSub = NULL;
			nextSubFrame = 0;

			for (size_t i = 0; i < sttable->GetRowCount(); ++i) {
				const char* frameField = sttable->QueryField(i, 0);
				const char* strField = sttable->QueryField(i, 1);
				if (frameField && strField) { // this skips the initial palette rows
					subs[atoi(frameField)] = atoi(strField);
				}
			}
		}

		~IESubtitles() override {
			delete cachedSub;
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
				delete cachedSub;
				cachedSub = core->GetString(it->second);
			}

			assert(cachedSub);
			return *cachedSub;
		}
	};

	AutoTable sttable(resref);
	Font* font = GetFont(MovieFontResRef);
	if (sttable && font) {
		int r = atoi(sttable->QueryField("red", "frame"));
		int g = atoi(sttable->QueryField("green", "frame"));
		int b = atoi(sttable->QueryField("blue", "frame"));

		if (r || g || b) {
			mp->SetSubtitles(new IESubtitles(font, resref, Color(r, g, b, 0xff)));
		} else {
			mp->SetSubtitles(new IESubtitles(font, resref));
		}
	}

	//shutting down music and ambients before movie
	if (music)
		music->HardEnd();
	AmbientMgr *ambim = AudioDriver->GetAmbientMgr();
	if (ambim) ambim->deactivate();

	Holder<SoundHandle> sound_override;
	if (sound_resref) {
		sound_override = AudioDriver->PlayRelative(sound_resref, SFX_CHAN_NARRATOR);
	}

	// clear whatever is currently on screen
	SetCutSceneMode(true);
	Region screen(0, 0, config.Width, config.Height);
	Window* win = winmgr->MakeWindow(screen);
	win->SetFlags(Window::Borderless|Window::NoSounds, OP_OR);
	winmgr->PresentModalWindow(win);
	WindowManager::CursorFeedback cur = winmgr->SetCursorFeedback(WindowManager::MOUSE_NONE);
	winmgr->DrawWindows();

	mp->Play(win);
	win->Close();
	winmgr->SetCursorFeedback(cur);
	SetCutSceneMode(false);
	if (sound_override) {
		sound_override->Stop();
		sound_override.release();
	}

	//restarting music
	if (music)
		music->Start();
	if (ambim) ambim->activate();

	//Setting the movie name to 1
	vars->SetAt( resref, 1 );
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
	char Path[_MAX_PATH];
	const char* resourcePath = NULL;
	DirectoryIterator::FileFilterPredicate* filter = NULL;
	switch (dir) {
		case DIRECTORY_CHR_PORTRAITS:
			resourcePath = config.GamePortraitsPath;
			filter = new ExtFilter("BMP");
			if (IsAvailable(IE_PNG_CLASS_ID)) {
				// chain an ORed filter for png
				filter = new OrPredicate<const char*>(filter, new ExtFilter("PNG"));
			}
			break;
		case DIRECTORY_CHR_SOUNDS:
			resourcePath = config.GameSoundsPath;
			if (!HasFeature( GF_SOUNDFOLDERS ))
				filter = new ExtFilter("WAV");
			break;
		case DIRECTORY_CHR_EXPORTS:
			resourcePath = config.GameCharactersPath;
			filter = new ExtFilter("CHR");
			break;
		case DIRECTORY_CHR_SCRIPTS:
			resourcePath = config.GameScriptsPath;
			filter = new ExtFilter("BS");
			filter = new OrPredicate<const char*>(filter, new ExtFilter("BCS"));
			break;
		default:
			error("Interface", "Unknown resource directory type: %d!", dir);
	}

	PathJoin(Path, config.GamePath, resourcePath, nullptr);
	DirectoryIterator dirIt(Path);
	dirIt.SetFilterPredicate(filter);
	return dirIt;
}

bool Interface::InitializeVarsWithINI(const char* iniFileName)
{
	if (!core->IsAvailable( IE_INI_CLASS_ID ))
		return false;

	const DataFileMgr* defaults = nullptr;
	const DataFileMgr* overrides = nullptr;

	PluginHolder<DataFileMgr> ini = MakePluginHolder<DataFileMgr>(IE_INI_CLASS_ID);
	FileStream* iniStream = FileStream::OpenFile(iniFileName);
	// if filename is not set we assume we are creating defaults without an INI
	bool opened = ini->Open(iniStream);
	if (iniFileName[0] && !opened) {
		Log(WARNING, "Core", "Unable to read defaults from '%s'. Using GemRB default values.", iniFileName);
	} else {
		overrides = ini.get();
	}
	if (!opened || iniFileName[0] == 0) {
		delete iniStream; // Open deletes it itself on success
	}

	PluginHolder<DataFileMgr> gemINI = MakePluginHolder<DataFileMgr>(IE_INI_CLASS_ID);
	DataStream* gemINIStream = gamedata->GetResource( "defaults", IE_INI_CLASS_ID );

	if (!gemINIStream || !gemINI->Open(gemINIStream)) {
		Log(WARNING, "Core", "Unable to load GemRB default values.");
		defaults = ini.get();
	} else {
		defaults = gemINI.get();
	}
	if (!overrides) {
		overrides = defaults;
	}

	for (int i = 0; i < defaults->GetTagsCount(); i++) {
		const char* tag = defaults->GetTagNameByIndex(i);
		for (int j = 0; j < defaults->GetKeysCount(tag); j++) {
			const char* key = defaults->GetKeyNameByIndex(tag, j);
			//skip any existing entries. GemRB.cfg has priority
			if (!vars->HasKey(key)) {
				ieDword defaultVal = defaults->GetKeyAsInt(tag, key, 0);
				vars->SetAt(key, overrides->GetKeyAsInt(tag, key, defaultVal));
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
		vars->SetAt("Infravision", (ieDword)0);
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
	char ini_path[_MAX_PATH] = { '\0' };
	char gemrbINI[_MAX_PATH+4] = { '\0' };
	if (strncmp(INIConfig, "gem-", 4) != 0) {
		snprintf(gemrbINI, sizeof(gemrbINI), "gem-%s", INIConfig);
	}
	PathJoin(ini_path, config.GamePath, gemrbINI, nullptr);
	FileStream *fs = new FileStream();
	if (!fs->Create(ini_path)) {
		PathJoin(ini_path, config.SavePath, gemrbINI, nullptr);
		if (!fs->Create(ini_path)) {
			delete fs;
			return false;
		}
	}

	PluginHolder<DataFileMgr> defaultsINI = MakePluginHolder<DataFileMgr>(IE_INI_CLASS_ID);
	DataStream* INIStream = gamedata->GetResource( "defaults", IE_INI_CLASS_ID );

	if (INIStream && defaultsINI->Open(INIStream)) {
		// dump the formatted default config options to the file
		StringBuffer contents;
		for (int i = 0; i < defaultsINI->GetTagsCount(); i++) {
			const char* tag = defaultsINI->GetTagNameByIndex(i);
			// write section header
			contents.appendFormatted("[%s]\n", tag);
			for (int j = 0; j < defaultsINI->GetKeysCount(tag); j++) {
				const char* key = defaultsINI->GetKeyNameByIndex(tag, j);
				ieDword value = 0;
				bool found = vars->Lookup(key, value);
				assert(found);
				contents.appendFormatted("%s = %d\n", key, value);
			}
		}

		fs->Write(contents.get().c_str(), contents.get().size());
	} else {
		Log(ERROR, "Core", "Unable to open GemRB defaults. Cannot determine what values to write to %s.", ini_path);
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
	if (!gc || (gc->GetDialogueFlags()&DF_IN_DIALOG) || (gc->GetScreenFlags()&SF_CUTSCENE)) {
		return true;
	}
	return false;
}

/** Updates the Game Script Engine State */
bool Interface::GSUpdate(bool update_scripts)
{
	if(update_scripts) {
		return timer.Update();
	}
	else {
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
		if (ambim) ambim->deactivate();
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

void Interface::LoadGame(SaveGame *sg, int ver_override)
{
	// This function has rather painful error handling,
	// as it should swap all the objects or none at all
	// and the loading can fail for various reasons

	// Yes, it uses goto. Other ways seemed too awkward for me.

	gamedata->SaveAllStores();
	strings->CloseAux();
	tokens->RemoveAll(NULL); //clearing the token dictionary

	if(calendar) delete calendar;
	calendar = new Calendar;

	DataStream* gam_str = NULL;
	DataStream* sav_str = NULL;
	DataStream* wmp_str1 = NULL;
	DataStream* wmp_str2 = NULL;

	Game* new_game = NULL;
	WorldMapArray* new_worldmap = NULL;

	LoadProgress(10);
	if (!config.KeepCache) DelTree((const char *) config.CachePath, true);
	LoadProgress(15);

	saveGameAREExtractor.changeSaveGame(sg);

	if (sg == NULL) {
		//Load the Default Game
		gam_str = gamedata->GetResource( GameNameResRef, IE_GAM_CLASS_ID );
		sav_str = NULL;
		wmp_str1 = gamedata->GetResource( WorldMapName[0], IE_WMP_CLASS_ID );
		if (!WorldMapName[1].IsEmpty()) {
			wmp_str2 = gamedata->GetResource( WorldMapName[1], IE_WMP_CLASS_ID );
		}
	} else {
		gam_str = sg->GetGame();
		sav_str = sg->GetSave();
		wmp_str1 = sg->GetWmap(0);
		if (!WorldMapName[1].IsEmpty()) {
			wmp_str2 = sg->GetWmap(1);
			if (!wmp_str2) {
				//upgrade an IWD game to HOW
				wmp_str2 = gamedata->GetResource( WorldMapName[1], IE_WMP_CLASS_ID );
			}
		}
	}

	// These are here because of the goto
	PluginHolder<SaveGameMgr> gam_mgr = GetImporter<SaveGameMgr>(IE_GAM_CLASS_ID, gam_str);
	PluginHolder<WorldMapMgr> wmp_mgr = MakePluginHolder<WorldMapMgr>(IE_WMP_CLASS_ID);
	AmbientMgr *ambim = core->GetAudioDrv()->GetAmbientMgr();

	if (!gam_str || !(wmp_str1 || wmp_str2) )
		goto cleanup;

	// Load GAM file
	if (!gam_mgr)
		goto cleanup;

	new_game = gam_mgr->LoadGame(new Game(), ver_override);
	if (!new_game)
		goto cleanup;

	// Load WMP (WorldMap) file
	if (!wmp_mgr)
		goto cleanup;

	if (!wmp_mgr->Open(wmp_str1, wmp_str2))
		goto cleanup;

	new_worldmap = wmp_mgr->GetWorldMapArray( );

	wmp_str1 = NULL;
	wmp_str2 = NULL;

	LoadProgress(20);
	// Unpack SAV (archive) file to Cache dir
	if (sav_str) {
		PluginHolder<ArchiveImporter> ai = MakePluginHolder<ArchiveImporter>(IE_SAV_CLASS_ID);
		if (ai) {
			if (ai->DecompressSaveGame(sav_str, saveGameAREExtractor) != GEM_OK) {
				goto cleanup;
			}
		}
		delete sav_str;
		sav_str = NULL;
	}

	// rarely caused crashes while loading, so stop the ambients
	if (ambim) {
		ambim->reset();
	}

	// Let's assume that now is everything loaded OK and swap the objects
	delete game;
	delete worldmap;

	game = new_game;
	worldmap = new_worldmap;

	strings->OpenAux();
	LoadProgress(70);
	return;
cleanup:
	// Something went wrong, so try to clean after itself
	delete new_game;
	delete new_worldmap;
	delete wmp_str1;
	delete wmp_str2;
	delete sav_str;

	error("Core", "Unable to load game.");
}

/* replace the current world map but sync areas available in old and new */
void Interface::UpdateWorldMap(const ResRef& wmResRef)
{
	DataStream* wmp_str = gamedata->GetResource(wmResRef, IE_WMP_CLASS_ID);
	PluginHolder<WorldMapMgr> wmp_mgr = MakePluginHolder<WorldMapMgr>(IE_SAV_CLASS_ID);

	if (!wmp_str || !wmp_mgr || !wmp_mgr->Open(wmp_str, NULL)) {
		Log(ERROR, "Core", "Could not update world map %s", wmResRef.CString());
		return;
	}

	WorldMapArray *new_worldmap = wmp_mgr->GetWorldMapArray();
	const WorldMap *wm = worldmap->GetWorldMap(0);
	const WorldMap *nwm = new_worldmap->GetWorldMap(0);

	unsigned int ni;
	unsigned int ec = wm->GetEntryCount();
	//update status of the previously existing areas
	for (unsigned int i = 0; i < ec; i++) {
		const WMPAreaEntry *ae = wm->GetEntry(i);
		WMPAreaEntry *nae = nwm->GetArea(ae->AreaResRef, ni);
		if (nae != NULL) {
			nae->SetAreaStatus(ae->GetAreaStatus(), OP_SET);
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
		DataStream *wmp_str1 = gamedata->GetResource( WorldMapName[0], IE_WMP_CLASS_ID );
		DataStream *wmp_str2 = gamedata->GetResource( WorldMapName[1], IE_WMP_CLASS_ID );

		if (!wmp_mgr->Open(wmp_str1, wmp_str2)) {
			delete wmp_str1;
			delete wmp_str2;
		}

		delete worldmap;
		worldmap = wmp_mgr->GetWorldMapArray();
	}
}

bool Interface::InitItemTypes()
{
	if (slotmatrix) {
		free(slotmatrix);
	}

	AutoTable it("itemtype");
	ItemTypes = 0;
	if (it) {
		ItemTypes = it->GetRowCount(); //number of itemtypes
		if (ItemTypes<0) {
			ItemTypes = 0;
		}
		int InvSlotTypes = it->GetColumnCount();
		if (InvSlotTypes > 32) { //bit count limit
			InvSlotTypes = 32;
		}
		//make sure unsigned int is 32 bits
		slotmatrix = (ieDword *) malloc(ItemTypes * sizeof(ieDword) );
		for (int i = 0; i < ItemTypes; i++) {
			unsigned int value = 0;
			unsigned int k = 1;
			for (int j=0;j<InvSlotTypes;j++) {
				if (strtosigned<long>(it->QueryField(i,j))) {
					value |= k;
				}
				k <<= 1;
			}
			//we let any items in the inventory
			slotmatrix[i] = value | SLOT_INVENTORY;
		}
	}

	//itemtype data stores (armor failure and critical damage multipliers), critical range
	itemtypedata.reserve(ItemTypes);
	for (int i = 0; i < ItemTypes; i++) {
		itemtypedata.emplace_back(4);
		//default values in case itemdata is missing (it is needed only for iwd2)
		if (slotmatrix[i] & SLOT_WEAPON) {
			itemtypedata[i][IDT_FAILURE] = 0; // armor malus
			itemtypedata[i][IDT_CRITRANGE] = 20; // crit range
			itemtypedata[i][IDT_CRITMULTI] = 2; // crit multiplier
			itemtypedata[i][IDT_SKILLPENALTY] = 0; // skill check malus
		}
	}
	AutoTable af("itemdata");
	if (af) {
		int armcount = af->GetRowCount();
		int colcount = af->GetColumnCount();
		for (int i = 0; i < armcount; i++) {
			int itemtype = (ieWord) atoi( af->QueryField(i,0) );
			if (itemtype<ItemTypes) {
				// we don't need the itemtype column, since it is equal to the position
				for (int j = 0; j < colcount - 1; j++) {
					itemtypedata[itemtype][j] = atoi(af->QueryField(i, j+1));
				}
			}
		}
	}

	//slottype describes the inventory structure
	Inventory::Init();
	AutoTable st("slottype");
	SlotTypes = 0;
	if (st) {
		SlotTypes = st->GetRowCount();
		//make sure unsigned int is 32 bits
		slotTypes.resize(SlotTypes);
		for (unsigned int row = 0; row < SlotTypes; row++) {
			bool alias;
			ieDword i = strtounsigned<ieDword>(st->GetRowName(row));
			if (i>=SlotTypes) continue;
			if (slotTypes[i].slotEffects != 100) { // SLOT_EFFECT_ALIAS
				slotTypes[row].slot = i;
				i=row;
				alias = true;
			} else {
				slotTypes[row].slot = i;
				alias = false;
			}
			slotTypes[i].slotType = strtounsigned<ieDword>(st->QueryField(row, 0));
			slotTypes[i].slotID = strtounsigned<ieDword>(st->QueryField(row, 1));
			slotTypes[i].slotResRef = st->QueryField(row, 2);
			slotTypes[i].slotTip = strtounsigned<ieDword>(st->QueryField(row, 3));
			slotTypes[i].slotFlags = strtounsigned<ieDword>(st->QueryField(row, 5));
			//don't fill sloteffects for aliased slots (pst)
			if (alias) {
				continue;
			}
			slotTypes[i].slotEffects = strtounsigned<ieDword>(st->QueryField(row, 4));
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
	return (it && st);
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

const char *Interface::QuerySlotResRef(unsigned int idx) const
{
	if (idx>=SlotTypes) {
		return "";
	}
	return slotTypes[idx].slotResRef;
}

int Interface::GetArmorFailure(unsigned int itemtype) const
{
	if (itemtype>=(unsigned int) ItemTypes) {
		return 0;
	}
	if (slotmatrix[itemtype]&SLOT_ARMOUR) return itemtypedata[itemtype][IDT_FAILURE];
	return 0;
}

int Interface::GetShieldFailure(unsigned int itemtype) const
{
	if (itemtype>=(unsigned int) ItemTypes) {
		return 0;
	}
	if (slotmatrix[itemtype]&SLOT_SHIELD) return itemtypedata[itemtype][IDT_FAILURE];
	return 0;
}

int Interface::GetArmorPenalty(unsigned int itemtype) const
{
	if (itemtype>=(unsigned int) ItemTypes) {
		return 0;
	}
	if (slotmatrix[itemtype]&SLOT_ARMOUR) return itemtypedata[itemtype][IDT_SKILLPENALTY];
	return 0;
}

int Interface::GetShieldPenalty(unsigned int itemtype) const
{
	if (itemtype>=(unsigned int) ItemTypes) {
		return 0;
	}
	if (slotmatrix[itemtype]&SLOT_SHIELD) return itemtypedata[itemtype][IDT_SKILLPENALTY];
	return 0;
}

int Interface::GetCriticalMultiplier(unsigned int itemtype) const
{
	if (itemtype>=(unsigned int) ItemTypes) {
		return 2;
	}
	if (slotmatrix[itemtype]&SLOT_WEAPON) return itemtypedata[itemtype][IDT_CRITMULTI];
	return 2;
}

int Interface::GetCriticalRange(unsigned int itemtype) const
{
	if (itemtype>=(unsigned int) ItemTypes) {
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
			if (feedback) displaymsg->DisplayConstantString(STR_NOT_IN_OFFHAND, DMC_WHITE);
			return 0;
		}
	}

	if ( (unsigned int) item->ItemType>=(unsigned int) ItemTypes) {
		//invalid itemtype
		if (feedback) displaymsg->DisplayConstantString(STR_WRONGITEMTYPE, DMC_WHITE);
		return 0;
	}

	//if actor is supplied, check its usability fields
	if (actor) {
		//constant strings
		int idx = actor->Unusable(item);
		if (idx) {
			if (feedback) displaymsg->DisplayConstantString(idx, DMC_WHITE);
			return 0;
		}
		//custom strings
		ieStrRef str = actor->Disabled(item->Name, item->ItemType);
		if (str && !equipped) {
			if (feedback) displaymsg->DisplayString(str, DMC_WHITE, 0);
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
		if (feedback) displaymsg->DisplayConstantString(STR_WRONGITEMTYPE, DMC_WHITE);
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
		displaymsg->DisplayConstantString(STR_UNUSABLEITEM, DMC_WHITE);
		return 0;
	}

	return ret;
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
int Interface::SavedExtension(const char *filename) const
{
	const char *str=strchr(filename,'.');
	if (!str) return 0;

	for (const auto ext : saved_extensions) {
		if (ext && !stricmp(ext, str)) return 2;
	}
	for (const auto ext : saved_extensions_last) {
		if (ext && !stricmp(ext, str)) return 1;
	}
	return 0;
}

static const char* const protected_extensions[] = { ".exe", ".dll", ".so", nullptr };

//returns true if file should be saved
bool Interface::ProtectedExtension(const char *filename) const
{
	const char *str=strchr(filename,'.');
	if (!str) return false;
	int i=0;
	while(protected_extensions[i]) {
		if (!stricmp(protected_extensions[i], str) ) return true;
		i++;
	}
	return false;
}

void Interface::RemoveFromCache(const ResRef& resref, SClass_ID ClassID) const
{
	char filename[_MAX_PATH];

	PathJoinExt(filename, config.CachePath, resref, TypeExt(ClassID));
	unlink ( filename);
}

//this function checks if the path is eligible as a cache
//if it contains a directory, or suspicious file extensions
//we bail out, because the cache will be purged regularly.
bool Interface::StupidityDetector(const char* Pt) const
{
	char Path[_MAX_PATH];
	if (strlcpy(Path, Pt, _MAX_PATH) >= _MAX_PATH) {
		Log(ERROR, "Interface", "Trying to check too long path: %s!", Pt);
		return true;
	}
	DirectoryIterator dir(Path);
	// scan everything
	dir.SetFlags(DirectoryIterator::All, true);

	if (!dir) {
		print("\n**cannot open**");
		return true;
	}
	do {
		const char *name = dir.GetName();
		if (dir.IsDirectory()) {
			// since we can't use DirectoryIterator::Hidden, exclude the special dirs manually
			if (name[0] == '.' && name[1] == '\0') {
					continue;
			}
			if (name[0] == '.' && name[1] == '.' && name[2] == '\0') {
					continue;
			}
			print("\n**contains another dir**");
			return true; //a directory in there???
		}
		if (ProtectedExtension(name) ) {
			print("\n**contains alien files**");
			return true; //an executable file in there???
		}
	} while (++dir);
	//ok, we got a good conscience
	return false;
}

void Interface::DelTree(const char* Pt, bool onlysave) const
{
	char Path[_MAX_PATH];

	if (!Pt[0]) return; //Don't delete the root filesystem :)
	if (strlcpy(Path, Pt, _MAX_PATH) >= _MAX_PATH) {
		Log(ERROR, "Interface", "Trying to delete too long path: %s!", Pt);
		return;
	}
	DirectoryIterator dir(Path);
	dir.SetFlags(DirectoryIterator::Files);
	if (!dir) {
		return;
	}
	do {
		const char *name = dir.GetName();
		if (!onlysave || SavedExtension(name) ) {
			char dtmp[_MAX_PATH];
			dir.GetFullPath(dtmp);
			unlink( dtmp );
		}
	} while (++dir);
}

void Interface::LoadProgress(int percent)
{
	vars->SetAt("Progress", percent);

	WindowManager::CursorFeedback cur = winmgr->SetCursorFeedback(WindowManager::MOUSE_NONE);
	winmgr->DrawWindows();
	winmgr->SetCursorFeedback(cur);

	const Window* loadwin = GetWindow(0, "LOADWIN");
	if (loadwin) {
		// loadwin is NULL when LoadMap is called and passes false for the loadscreen param
		loadwin->RedrawControls("Progress", percent);
	}

	video->SwapBuffers();
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

bool Interface::ReadItemTable(const ResRef& TableName, const char *Prefix)
{
	AutoTable tab(TableName);
	if (!tab) {
		return false;
	}

	int i = tab->GetRowCount();
	for (int j = 0; j < i; j++) {
		ResRef ItemName;
		if (Prefix) {
			ItemName.SNPrintF("%s%02d", Prefix, (j + 1) % 100);
		} else {
			ItemName = MakeUpperCaseResRef(tab->GetRowName(j));
		}

		int l=tab->GetColumnCount(j);
		if (l<1) continue;
		int cl = atoi(tab->GetColumnName(0));
		std::vector<ResRef> refs;
		for(int k=0;k<l;k++) {
			refs.push_back(MakeLowerCaseResRef(tab->QueryField(j, k)));
		}
		RtRows.insert(std::make_pair(ItemName, ItemList(std::move(refs), cl)));
	}
	return true;
}

bool Interface::ReadRandomItems()
{
	ieDword difflev=0; //rt norm or rt fury
	vars->Lookup("Nightmare Mode", difflev);
	RtRows.clear();
	
	AutoTable tab("randitem");
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
	ResRef randTreasureRef;
	randTreasureRef = tab->QueryField(1, difflev);
	int i = atoi(randTreasureRef);
	if (i<1) {
		ReadItemTable(randTreasureRef, 0); //reading the table itself
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
	if (MagicBit && (item->Flags & IE_INV_ITEM_UNDROPPABLE)) {
		item->Flags |= IE_INV_ITEM_MAGICAL;
		item->Flags &= ~IE_INV_ITEM_UNDROPPABLE;
	}
	if (core->HasFeature(GF_NO_UNDROPPABLE)) {
		item->Flags &= ~IE_INV_ITEM_UNDROPPABLE;
	}

	const Item *itm = gamedata->GetItem(item->ItemResRef, true);
	if (!itm) return;

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
	if ((item->Flags & IE_INV_ITEM_STOLEN2) && !HasFeature(GF_PST_STATE_FLAGS)) {
		item->Flags |= IE_INV_ITEM_STOLEN;
	}

	// auto identify basic items
	if (!itm->LoreToID) {
		item->Flags |= IE_INV_ITEM_IDENTIFIED;
	}

	gamedata->FreeItem(itm, item->ItemResRef, false);
}

#define MAX_LOOP 10

//This function generates random items based on the randitem.2da file
//there could be a loop, but we don't want to freeze, so there is a limit
bool Interface::ResolveRandomItem(CREItem *itm) const
{
	if (RtRows.empty()) return true;
	for(int loop=0;loop<MAX_LOOP;loop++) {
		char *endptr;
		char NewItem[9];

		if (RtRows.count(itm->ItemResRef) == 0) {
			if (!gamedata->Exists(itm->ItemResRef, IE_ITM_CLASS_ID)) {
				Log(ERROR, "Interface", "Nonexistent random item (bad table entry) detected: %s", itm->ItemResRef.CString());
				return false;
			}
			return true;
		}
		const ItemList& itemlist = RtRows.at(itm->ItemResRef);
		int i;
		if (itemlist.WeightOdds) {
			//instead of 1d19 we calculate with 2d10 (which also has 19 possible values)
			i=Roll(2,(itemlist.ResRefs.size() + 1)/2, -2);
		} else {
			i=Roll(1, itemlist.ResRefs.size(), -1);
		}
		strnlwrcpy( NewItem, itemlist.ResRefs[i], 8);
		char *p = strchr(NewItem, '*');
		int diceSides;
		if (p) {
			*p=0; //doing this so endptr is ok
			diceSides = strtosigned<int>(p + 1, nullptr, 10);
		} else {
			diceSides = 1;
		}
		int diceThrows = strtosigned<int>(NewItem, &endptr, 10);
		if (diceThrows <1) {
			diceThrows = 1;
		}
		if (*endptr) {
			itm->ItemResRef = MakeLowerCaseResRef(NewItem);
		} else {
			itm->ItemResRef = GoldResRef;
		}
		if (itm->ItemResRef == "no_drop") {
			itm->ItemResRef.Reset();
		}
		if (itm->ItemResRef.IsEmpty()) {
			return false;
		}
		itm->Usages[0] = (ieWord) Roll(diceThrows, diceSides, 0);
	}
	Log(ERROR, "Interface", "Loop detected while generating random item:%s",
		itm->ItemResRef.CString());
	return false;
}

Effect *Interface::GetEffect(ieDword opcode)
{
	if (opcode==0xffffffff) {
		return NULL;
	}
	Effect *fx = new Effect();
	if (!fx) {
		return NULL;
	}
	fx->Opcode=opcode;
	return fx;
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

void Interface::SetCurrentContainer(Actor *actor, Container *arg, bool flag)
{
	//abort action if the first selected PC isn't the original actor
	if (actor!=GetFirstSelectedPC(false)) {
		CurrentContainer = NULL;
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
	PluginHolder<DialogMgr> dm = GetImporter<DialogMgr>(IE_DLG_CLASS_ID, gamedata->GetResource(dlgref, IE_DLG_CLASS_ID));
	Dialog *dlg = dm->GetDialog();

	if (!dlg) {
		Log(ERROR, "Interface", "Cannot load dialog: %s", dlgref.CString());
		return ieStrRef(-1);
	}
	Scriptable *pc = game->GetSelectedPCSingle(false);

	// forcefully rerandomize
	RandomNumValue = RAND_ALL();
	ieStrRef ret = ieStrRef(-1);

	int i = dlg->FindRandomState( pc );
	if (i>=0 ) {
		ret = dlg->GetState( i )->StrRef;
	}
	delete dlg;
	return ret;
}

//plays stock sound listed in defsound.2da
Holder<SoundHandle> Interface::PlaySound(size_t index, unsigned int channel)
{
	if (index <= gamedata->defaultSounds.size()) {
		return AudioDriver->PlayRelative(gamedata->defaultSounds[index], channel);
	}
	return NULL;
}

Actor *Interface::GetFirstSelectedPC(bool forced)
{
	Actor *ret = NULL;
	int slot = 0;
	int partySize = game->GetPartySize( false );
	if (!partySize) return NULL;
	for (int i = 0; i < partySize; i++) {
		Actor* actor = game->GetPC( i,false );
		if (actor->IsSelected()) {
			if (actor->InParty<slot || !ret) {
				ret = actor;
				slot = actor->InParty;
			}
		}
	}

	if (forced && !ret) {
		return game->FindPC((unsigned int) 1);
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
Holder<Sprite2D> Interface::GetCursorSprite()
{
	Holder<Sprite2D> spr = gamedata->GetBAMSprite(TextCursorBam, 0, 0);
	if (spr && HasFeature(GF_OVERRIDE_CURSORPOS)) {
		spr->Frame.x = 1;
		spr->Frame.y = spr->Frame.h - 1;
	}
	return spr;
}

Holder<Sprite2D> Interface::GetScrollCursorSprite(int frameNum, int spriteNum)
{
	return gamedata->GetBAMSprite(ScrollCursorBam, frameNum, spriteNum, true);
}

/* we should return -1 if it isn't gold, otherwise return the gold value */
int Interface::CanMoveItem(const CREItem *item) const
{
	//This is an inventory slot, switch to IE_ITEM_* if you use Item
	if (item->Flags & IE_INV_ITEM_UNDROPPABLE && !HasFeature(GF_NO_DROP_CAN_MOVE)) {
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
	const Spell *spell = gamedata->GetSpell(spellRef);
	if (!spell) {
		return;
	}

	int header = spell->GetHeaderIndexFromLevel(level);
	EffectQueue *fxqueue = spell->GetEffectBlock(caster, actor->Pos, header, level);

	ApplyEffectQueue(fxqueue, actor, caster, actor->Pos);
	delete fxqueue;
}

void Interface::ApplySpellPoint(const ResRef& spellRef, Map* area, const Point &pos, Scriptable *caster, int level) const
{
	const Spell *spell = gamedata->GetSpell(spellRef);
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

	EffectQueue *fxqueue = new EffectQueue();
	fxqueue->AddEffect( effect );
	int res = ApplyEffectQueue(fxqueue, actor, caster);
	delete fxqueue;
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
		Log(DEBUG, "Core", "Not saving area %s",
			map->GetScriptName());
		RemoveFromCache(map->GetScriptName(), IE_ARE_CLASS_ID);
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

		str.Create( map->GetScriptName(), IE_ARE_CLASS_ID );
		int ret = mm->PutArea (&str, map);
		if (ret <0) {
			Log(WARNING, "Core", "Area removed: %s",
				map->GetScriptName());
			RemoveFromCache(map->GetScriptName(), IE_ARE_CLASS_ID);
		}
	} else {
		Log(WARNING, "Core", "Area removed: %s",
			map->GetScriptName());
		RemoveFromCache(map->GetScriptName(), IE_ARE_CLASS_ID);
	}
	//make sure the stream isn't connected to sm, or it will be double freed
	return 0;
}

int Interface::WriteCharacter(const char *name, Actor *actor)
{
	char Path[_MAX_PATH];

	PathJoin(Path, config.GamePath, config.GameCharactersPath, nullptr);
	if (!actor) {
		return -1;
	}
	auto gm = GetImporter<ActorMgr>(IE_CRE_CLASS_ID);
	if (gm == nullptr) {
		return -1;
	}

	//str is freed
	{
		FileStream str;

		if (!str.Create( Path, name, IE_CHR_CLASS_ID )
			|| (gm->PutActor(&str, actor, true) < 0)) {
			Log(WARNING, "Core", "Character cannot be saved: %s", name);
			return -1;
		}
	}

	//write the BIO string
	if (!HasFeature(GF_NO_BIOGRAPHY)) {
		FileStream str;

		str.Create( Path, name, IE_BIO_CLASS_ID );
		//never write the string reference into this string
		char *tmp = GetCString(actor->GetVerbalConstant(VB_BIO),IE_STR_STRREFOFF);
		str.Write (tmp, strlen(tmp));
		free(tmp);
	}
	return 0;
}

int Interface::WriteGame(const char *folder)
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

		str.Create( folder, GameNameResRef, IE_GAM_CLASS_ID );
		int ret = gm->PutGame (&str, game);
		if (ret <0) {
			Log(WARNING, "Core", "Game cannot be saved: %s", folder);
			return -1;
		}
	} else {
		Log(WARNING, "Core", "Internal error, game cannot be saved: %s", folder);
		return -1;
	}
	return 0;
}

int Interface::WriteWorldMap(const char *folder)
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

		str1.Create( folder, WorldMapName[0], IE_WMP_CLASS_ID );
		if (!worldmap->IsSingle()) {
			str2.Create( folder, WorldMapName[1], IE_WMP_CLASS_ID );
		}
		ret = wmm->PutWorldMap (&str1, &str2, worldmap);
	}
	if (ret <0) {
		Log(WARNING, "Core", "Internal error, worldmap cannot be saved: %s", folder);
		return -1;
	}
	return 0;
}

static bool IsBlobSaveItem(const char *path) {
	if (path == nullptr) {
		return false;
	}

	auto areExt = strstr(path, ".blb");
	auto pathLength = strlen(path);
	return areExt != nullptr && path + pathLength - 4 == areExt;
}

int Interface::CompressSave(const char *folder, bool overrideRunning)
{
	FileStream str;

	str.Create( folder, GameNameResRef, IE_SAV_CLASS_ID );
	DirectoryIterator dir(config.CachePath);
	if (!dir) {
		return GEM_ERROR;
	}
	PluginHolder<ArchiveImporter> ai = MakePluginHolder<ArchiveImporter>(IE_SAV_CLASS_ID);
	ai->CreateArchive( &str);

	tick_t startTime = GetTicks();
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
			const char *name = dir.GetName();
			if (SavedExtension(name)==priority) {
				char dtmp[_MAX_PATH];
				dir.GetFullPath(dtmp);
				FileStream fs;
				if (!fs.Open(dtmp)) {
					Log(ERROR, "Interface", "Failed to open \"%s\".", dtmp);
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

	tick_t endTime = GetTicks();
	Log(WARNING, "Core", "%lu ms (compressing SAV file)", endTime - startTime);
	return GEM_OK;
}

int Interface::GetRareSelectSoundCount() const { return NumRareSelectSounds; }

int Interface::GetMaximumAbility() const { return MaximumAbility; }

int Interface::GetStrengthBonus(int column, int value, int ex) const
{
	//to hit, damage, open doors, weight allowance
	if (column<0 || column>3)
		return -9999;

	if (value<0)
		value = 0;
	else if (value>MaximumAbility)
		value = MaximumAbility;

	int bonus = 0;
	// only 18 (human max) has the differentiating extension
	if (value == 18 && !HasFeature(GF_3ED_RULES)) {
		if (ex<0)
			ex=0;
		else if (ex>100)
			ex=100;
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
	if (HasFeature(GF_3ED_RULES)) {
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
	if (HasFeature(GF_3ED_RULES)) {
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
	if (HasFeature(GF_3ED_RULES)) return 0;

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

int Interface::GetReputationMod(int column) const
{
	int reputation = game->Reputation / 10 - 1;

	if (column<0 || column>8) {
		return -9999;
	}
	if (reputation > 19) {
		reputation = 19;
	}
	if (reputation < 0) {
		reputation = 0;
	}

	return reputationmod[reputation][column];
}

PauseSetting Interface::TogglePause() const
{
	const GameControl *gc = GetGameControl();
	if (!gc) return PAUSE_OFF;
	PauseSetting pause = (PauseSetting)(~gc->GetDialogueFlags()&DF_FREEZE_SCRIPTS);
	if (SetPause(pause)) return pause;
	return (PauseSetting)(gc->GetDialogueFlags()&DF_FREEZE_SCRIPTS);
}

bool Interface::SetPause(PauseSetting pause, int flags) const
{
	GameControl *gc = GetGameControl();

	//don't allow soft pause in cutscenes and dialog
	if (!(flags&PF_FORCED) && InCutSceneMode()) gc = NULL;

	if (gc && ((bool)(gc->GetDialogueFlags()&DF_FREEZE_SCRIPTS) != (bool)pause)) { // already paused
		int strref;
		if (pause) {
			strref = STR_PAUSED;
			gc->SetDialogueFlags(DF_FREEZE_SCRIPTS, OP_OR);
		} else {
			strref = STR_UNPAUSED;
			gc->SetDialogueFlags(DF_FREEZE_SCRIPTS, OP_NAND);
		}
		if (!(flags&PF_QUIET) ) {
			if (pause) gc->SetDisplayText(strref, 0); // time 0 = removed instantly on unpause (for pst)
			displaymsg->DisplayConstantString(strref, DMC_RED);
		}
		return true;
	}
	return false;
}

bool Interface::Autopause(ieDword flag, Scriptable* target) const
{
	ieDword autopause_flags = 0;
	vars->Lookup("Auto Pause State", autopause_flags);

	if (!(autopause_flags & (1 << flag))) {
		return false;
	}

	if (!SetPause(PAUSE_ON, PF_QUIET)) {
		return false;
	}

	displaymsg->DisplayConstantString(STR_AP_UNUSABLE + flag, DMC_RED);

	ieDword centerOnAutoPause = 0;
	vars->Lookup("Auto Pause Center", centerOnAutoPause);
	if (centerOnAutoPause && target) {
		GameControl* gc = GetGameControl();
		gc->MoveViewportTo(target->Pos, true);

		if (target->Type == ST_ACTOR && ((Actor *) target)->GetStat(IE_EA) < EA_GOODCUTOFF) {
			core->GetGame()->SelectActor((Actor *) target, true, SELECT_REPLACE);
		}
	}
	return true;
}

void Interface::RegisterOpcodes(int count, const EffectDesc *opcodes) const
{
	EffectQueue_RegisterOpcodes(count, opcodes);
}

void Interface::SetInfoTextColor(const Color &color)
{
	InfoTextColor = color;
}

//todo row?
void Interface::GetResRefFrom2DA(const ResRef& resref, ResRef& resource1, ResRef& resource2, ResRef& resource3) const
{
	resource1.Reset();
	resource2.Reset();
	resource3.Reset();

	AutoTable tab(resref);
	if (tab) {
		unsigned int cols = tab->GetColumnCount();
		unsigned int row = (unsigned int) Roll(1,tab->GetRowCount(),-1);
		resource1 = MakeUpperCaseResRef(tab->QueryField(row, 0));
		if (cols > 1) {
			resource2 = MakeUpperCaseResRef(tab->QueryField(row, 1));
		}
		if (cols > 2) {
			resource3 = MakeUpperCaseResRef(tab->QueryField(row, 2));
		}
	}
}

ieDword *Interface::GetListFrom2DAInternal(const ResRef& resref)
{
	ieDword *ret;

	AutoTable tab(resref);
	if (tab) {
		ieDword cnt = tab->GetRowCount();
		ret = (ieDword *) malloc((1+cnt)*sizeof(ieDword));
		ret[0]=cnt;
		while(cnt) {
			ret[cnt] = strtounsigned<ieDword>(tab->QueryField(cnt-1, 0));
			cnt--;
		}
		return ret;
	}
	ret = (ieDword *) malloc(sizeof(ieDword));
	ret[0]=0;
	return ret;
}

ieDword* Interface::GetListFrom2DA(const ResRef& tablename)
{
	ieDword *list = NULL;

	if (!lists->Lookup(tablename, (void *&) list)) {
		list = GetListFrom2DAInternal(tablename);
		lists->SetAt(tablename, list);
	}

	return list;
}

//returns a numeric value associated with a stat name (symbol) from stats.ids
ieDword Interface::TranslateStat(const char *stat_name)
{
	ieDword tmp;
	if (valid_unsignednumber(stat_name, tmp)) {
		return tmp;
	}

	int symbol = LoadSymbol( "stats" );
	auto sym = GetSymbol( symbol );
	if (!sym) {
		error("Core", "Cannot load statistic name mappings.\n");
	}
	ieDword stat = (ieDword) sym->GetValue( stat_name );
	if (stat==(ieDword) ~0) {
		Log(WARNING, "Core", "Cannot translate symbol: %s", stat_name);
	}
	return stat;
}

// Calculates an arbitrary stat bonus, based on tables.
// the master table contains the table names (as row names) and the used stat
// the subtables contain stat value/bonus pairs.
// Optionally an override stat value can be specified (needed for use in pcfs).
int Interface::ResolveStatBonus(Actor *actor, const char *tablename, ieDword flags, int value)
{
	int mastertable = gamedata->LoadTable( tablename );
	if (mastertable == -1) return -1;
	auto mtm = gamedata->GetTable( mastertable );
	if (!mtm) {
		Log(ERROR, "Core", "Cannot resolve stat bonus.");
		return -1;
	}
	int count = mtm->GetRowCount();
	if (count< 1) {
		return 0;
	}
	int ret = 0;
	// tables for additive modifiers of bonus type
	for (int i = 0; i < count; i++) {
		tablename = mtm->GetRowName(i);
		int checkcol = strtosigned<int>(mtm->QueryField(i,1));
		unsigned int readcol = strtounsigned<unsigned int>(mtm->QueryField(i,2));
		int stat = TranslateStat(mtm->QueryField(i,0) );
		if (!(flags&1)) {
			value = actor->GetSafeStat(stat);
		}
		int table = gamedata->LoadTable( tablename );
		if (table == -1) continue;
		auto tm = gamedata->GetTable( table );
		if (!tm) continue;

		int row;
		if (checkcol == -1) {
			// use the row names
			char tmp[30];
			snprintf(tmp, sizeof(tmp), "%d", value);
			row = tm->GetRowIndex(tmp);
		} else {
			// use the checkcol column (default of 0)
			row = tm->FindTableValue(checkcol, value, 0);
		}
		if (row>=0) {
			ret += strtosigned<int>(tm->QueryField(row, readcol));
		}
	}
	return ret;
}

void Interface::WaitForDisc(int disc_number, const char* path) const
{
	GetDictionary()->SetAt( "WaitForDisc", (ieDword) disc_number );

	GetGUIScriptEngine()->RunFunction( "GUICommonWindows", "OpenWaitForDiscWindow" );
	do {
		winmgr->DrawWindows();
		for (const auto& cd : config.CD[disc_number - 1]) {
			char name[_MAX_PATH];

			PathJoin(name, cd.c_str(), path, nullptr);
			if (file_exists (name)) {
				GetGUIScriptEngine()->RunFunction( "GUICommonWindows", "OpenWaitForDiscWindow" );
				return;
			}
		}

	} while (video->SwapBuffers() == GEM_OK);
}

Timer& Interface::SetTimer(const EventHandler& handler, tick_t interval, int repeats)
{
	timers.emplace_back(interval, handler, repeats);
	return timers.back();
}

void Interface::SetNextScript(const char *script)
{
	strlcpy( NextScript, script, sizeof(NextScript) );
	QuitFlag |= QF_CHANGESCRIPT;
}

void Interface::SanityCheck(const char *ver) {
	if (strcmp(ver, VERSION_GEMRB) != 0) {
		error("Core", "version check failed: core version %s doesn't match caller's version %s\n", VERSION_GEMRB, ver);
	}
}

}
