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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "Interface.h"

#include "defsounds.h" // for DS_TOOLTIP
#include "exports.h"
#include "globals.h"
#include "strrefs.h"
#include "win32def.h"
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
#include "GameData.h"
#include "GlobalTimer.h"
#include "ImageMgr.h"
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
#include "ScriptEngine.h"
#include "ScriptedAnimation.h"
#include "SoundMgr.h"
#include "SpellMgr.h"
#include "StoreMgr.h"
#include "StringMgr.h"
#include "SymbolMgr.h"
#include "TileMap.h"
#include "VEFObject.h"
#include "Video.h"
#include "WindowMgr.h"
#include "WorldMapMgr.h"
#include "GameScript/GameScript.h"
#include "GUI/Button.h"
#include "GUI/Console.h"
#include "GUI/EventMgr.h"
#include "GUI/GameControl.h"
#include "GUI/Label.h"
#include "GUI/MapControl.h"
#include "GUI/TextArea.h"
#include "GUI/Window.h"
#include "GUI/WorldMapControl.h"
#include "Scriptable/Container.h"
#include "System/FileStream.h"
#include "System/VFS.h"
#include "System/StringBuffer.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <cstdlib>
#include <time.h>
#include <vector>

namespace GemRB {

GEM_EXPORT Interface* core = NULL;

//use DialogF.tlk if the protagonist is female, that's why we leave space
static const char dialogtlk[] = "dialog.tlk\0";

static int MaximumAbility = 25;
static ieWordSigned *strmod = NULL;
static ieWordSigned *strmodex = NULL;
static ieWordSigned *intmod = NULL;
static ieWordSigned *dexmod = NULL;
static ieWordSigned *conmod = NULL;
static ieWordSigned *chrmod = NULL;
static ieWordSigned *lorebon = NULL;
static ieWordSigned *wisbon = NULL;
static int **reputationmod = NULL;
static ieVariable IWD2DeathVarFormat = "_DEAD%s";
static ieVariable DeathVarFormat = "SPRITE_IS_DEAD%s";

static ieWord IDT_FAILURE = 0;
static ieWord IDT_CRITRANGE = 1;
static ieWord IDT_CRITMULTI = 2;
static ieWord IDT_SKILLPENALTY = 3;

static int MagicBit = 0;

Interface::Interface()
	: ButtonFontResRef("STONESML"), MovieFontResRef("STONESML"), TextFontResRef("FLOATTXT"), TooltipFontResRef("STONESML"),
	TLKEncoding()
{
	Log(MESSAGE, "Core", "GemRB Core Version v%s Loading...", VERSION_GEMRB );

	// default to the correct endianswitch
	ieWord endiantest = 1;
	if (((char *)&endiantest)[1] == 1) {
		// big-endian
		DataStream::SetEndianSwitch(true);
	}

	unsigned int i;
	for(i=0;i<256;i++) {
		pl_uppercase[i]=(ieByte) toupper(i);
		pl_lowercase[i]=(ieByte) tolower(i);
	}

	projserv = NULL;
	VideoDriverName = "sdl";
	AudioDriverName = "openal";
	vars = NULL;
	tokens = NULL;
	lists = NULL;
	RtRows = NULL;
	sgiterator = NULL;
	game = NULL;
	calendar = NULL;
	keymap = NULL;
	worldmap = NULL;
	CurrentStore = NULL;
	CurrentContainer = NULL;
	UseContainer = false;
	InfoTextPalette = NULL;
	timer = NULL;
	displaymsg = NULL;
	evntmgr = NULL;
	console = NULL;
	slottypes = NULL;
	slotmatrix = NULL;

	ModalWindow = NULL;
	modalShadow = MODAL_SHADOW_NONE;

	tooltip_x = 0;
	tooltip_y = 0;
	tooltip_currtextw = 0;
	tooltip_ctrl = NULL;
	plugin_flags = NULL;

	pal16 = NULL;
	pal32 = NULL;
	pal256 = NULL;

	GUIEnhancements = 0;

	CursorCount = 0;
	Cursors = NULL;

	mousescrollspd = 10;

	GameType[0] = '\0';
	ConsolePopped = false;
	CheatFlag = false;
	FogOfWar = 1;
	QuitFlag = QF_NORMAL;
	EventFlag = EF_CONTROL;
#ifndef WIN32
	CaseSensitive = true; //this is the default value, so CD1/CD2 will be resolved
#else
	CaseSensitive = false;
#endif
	SkipIntroVideos = false;
	DrawFPS = false;
	TouchScrollAreas = false;
	UseSoftKeyboard = false;
	KeepCache = false;
	NumFingInfo = 2;
	NumFingKboard = 3;
	NumFingScroll = 2;
	MouseFeedback = 0;
	TooltipDelay = 100;
	IgnoreOriginalINI = 0;
	Bpp = 32;
	GUIScriptsPath[0] = 0;
	GamePath[0] = 0;
	SavePath[0] = 0;
	GemRBPath[0] = 0;
	PluginsPath[0] = 0;
	CachePath[0] = 0;
	GemRBOverridePath[0] = 0;
	GemRBUnhardcodedPath[0] = 0;
	GameName[0] = 0;
	CustomFontPath[0] = 0;
	GameOverridePath[0] = 0;
	GameSoundsPath[0] = 0;
	GameScriptsPath[0] = 0;
	GamePortraitsPath[0] = 0;
	GameCharactersPath[0] = 0;
	GameDataPath[0] = 0;

	strlcpy( INIConfig, "baldur.ini", sizeof(INIConfig) );

	CopyResRef( ScrollCursorBam, "CURSARW" );
	CopyResRef( GlobalScript, "BALDUR" );
	CopyResRef( WorldMapName[0], "WORLDMAP" );
	CopyResRef( WorldMapName[1], "" );
	CopyResRef( Palette16, "MPALETTE" );
	CopyResRef( Palette32, "PAL32" );
	CopyResRef( Palette256, "MPAL256" );
	strcpy( TooltipBackResRef, "\0" );
	for (int size = 0; size < MAX_CIRCLE_SIZE; size++) {
		CopyResRef(GroundCircleBam[size], "");
		GroundCircleScale[size] = 0;
	}

	TooltipMargin = 10;

	TooltipBack = NULL;
	DraggedItem = NULL;
	DraggedPortrait = 0;
	DefSound = NULL;
	DSCount = -1;
	memset(GameFeatures, 0, sizeof( GameFeatures ));
	//GameFeatures = 0;
	//GameFeatures2 = 0;
	memset( WindowFrames, 0, sizeof( WindowFrames ));
	memset( GroundCircles, 0, sizeof( GroundCircles ));
	memset(FogSprites, 0, sizeof( FogSprites ));
	AreaAliasTable = NULL;
	ItemExclTable = NULL;
	ItemDialTable = NULL;
	ItemDial2Table = NULL;
	ItemTooltipTable = NULL;
	update_scripts = false;
	SpecialSpellsCount = -1;
	SpecialSpells = NULL;
	Encoding = "default";
	TLKEncoding.encoding = "ISO-8859-1";
	MagicBit = HasFeature(GF_MAGICBIT);

	gamedata = new GameData();
}

//2da lists are ieDword lists allocated by malloc
static void Release2daList(void *poi)
{
	free( (ieDword *) poi);
}

static void ReleaseItemList(void *poi)
{
	delete ((ItemList *) poi);
}

static void FreeAbilityTables()
{
	if (strmod) {
		free(strmod);
	}
	strmod = NULL;
	if (strmodex) {
		free(strmodex);
	}
	strmodex = NULL;
	if (intmod) {
		free(intmod);
	}
	intmod = NULL;
	if (dexmod) {
		free(dexmod);
	}
	dexmod = NULL;
	if (conmod) {
		free(conmod);
	}
	conmod = NULL;
	if (chrmod) {
		free(chrmod);
	}
	chrmod = NULL;
	if (lorebon) {
		free(lorebon);
	}
	lorebon = NULL;
	if (wisbon) {
		free(wisbon);
	}
	wisbon = NULL;
}

void Interface::FreeResRefTable(ieResRef *&table, int &count)
{
	if (table) {
		free( table );
		count = -1;
	}
}

static void ReleaseItemTooltip(void *poi)
{
	free(poi);
}

Interface::~Interface(void)
{
	DragItem(NULL,NULL);
	delete AreaAliasTable;

	if (music) {
		music->HardEnd();
	}
	// stop any ambients which are still enqueued
	if (AudioDriver) {
		AmbientMgr *ambim = AudioDriver->GetAmbientMgr();
		if (ambim) ambim->deactivate();
	}
	//destroy the highest objects in the hierarchy first!
	delete game;
	delete calendar;
	delete worldmap;
	delete keymap;

	FreeAbilityTables();

	if (reputationmod) {
		for (unsigned int i=0; i<20; i++) {
			if (reputationmod[i]) {
				free(reputationmod[i]);
			}
		}
		free(reputationmod);
		reputationmod=NULL;
	}

	if (SpecialSpells) {
		free(SpecialSpells);
	}
	SurgeSpells.clear();

	std::map<IeResRef, Font*>::iterator fit = fonts.begin();
	for (; fit != fonts.end(); ++fit)
		delete (*fit).second;
	// fonts need to be destroyed before TTF plugin
	PluginMgr::Get()->RunCleanup();

	ReleaseMemoryActor();
	ReleaseMemorySpell();
	EffectQueue_ReleaseMemory();
	CharAnimations::ReleaseMemory();

	FreeResRefTable(DefSound, DSCount);

	free( slottypes );
	free( slotmatrix );
	itemtypedata.clear();

	delete sgiterator;

	if (Cursors) {
		for (int i = 0; i < CursorCount; i++) {
			Sprite2D::FreeSprite( Cursors[i] );
		}
		delete[] Cursors;
	}

	std::vector<Window*>::iterator wit = windows.begin();
	for (; wit != windows.end(); ++wit) \
		delete *wit;

	size_t i;
	for (i = 0; i < musiclist.size(); i++) {
		free((void *)musiclist[i]);
	}

	DamageInfoMap.clear();

	ModalStates.clear();

	delete plugin_flags;

	delete projserv;

	delete console;

	delete pal256;
	delete pal32;
	delete pal16;

	delete timer;
	delete displaymsg;

	if (video) {

		for(i=0;i<sizeof(FogSprites)/sizeof(Sprite2D *);i++ ) {
			Sprite2D::FreeSprite(FogSprites[i]);
		}

		for(i=0;i<4;i++) {
			Sprite2D::FreeSprite(WindowFrames[i]);
		}

		for (int size = 0; size < MAX_CIRCLE_SIZE; size++) {
			for(i=0;i<6;i++) {
				Sprite2D::FreeSprite(GroundCircles[size][i]);
			}
		}

		if (TooltipBack) {
			for(i=0;i<3;i++) {
				//freesprite checks for null pointer
				Sprite2D::FreeSprite(TooltipBack[i]);
			}
			delete[] TooltipBack;
		}
		if (InfoTextPalette) {
			gamedata->FreePalette(InfoTextPalette);
		}

		video->SetCursor(NULL, VID_CUR_DRAG);
		video->SetCursor(NULL, VID_CUR_UP);
		video->SetCursor(NULL, VID_CUR_DOWN);
	}

	delete evntmgr;

	delete vars;
	delete tokens;
	if (lists) {
		lists->RemoveAll(Release2daList);
		delete lists;
	}

	if (RtRows) {
		RtRows->RemoveAll(ReleaseItemList);
		delete RtRows;
	}
	if (ItemExclTable) {
		ItemExclTable->RemoveAll(NULL);
		delete ItemExclTable;
	}
	if (ItemDialTable) {
		ItemDialTable->RemoveAll(NULL);
		delete ItemDialTable;
	}
	if (ItemDial2Table) {
		ItemDial2Table->RemoveAll(NULL);
		delete ItemDial2Table;
	}
	if (ItemTooltipTable) {
		ItemTooltipTable->RemoveAll(ReleaseItemTooltip);
		delete ItemTooltipTable;
	}

	Map::ReleaseMemory();
	Actor::ReleaseMemory();

	gamedata->ClearCaches();
	delete gamedata;
	gamedata = NULL;

	// Removing all stuff from Cache, except bifs
	if (!KeepCache) DelTree((const char *) CachePath, true);

	AudioDriver.release();
	video.release();
}

void Interface::SetWindowFrame(int i, Sprite2D *Picture)
{
	Sprite2D::FreeSprite(WindowFrames[i]);
	WindowFrames[i]=Picture;
}

GameControl* Interface::StartGameControl()
{
	//making sure that our window is the first one
	if (ConsolePopped) {
		PopupConsole();
	}
	DelAllWindows();//deleting ALL windows
	gamedata->DelTable(0xffffu); //dropping ALL tables
	Window* gamewin = new Window( 0xffff, 0, 0, (ieWord) Width, (ieWord) Height );
	gamewin->WindowPack[0]=0;
	GameControl* gc = new GameControl(Region(0, 0, Width, Height));
	gc->Owner = gamewin;
	gc->ControlID = 0x00000000;
	gc->ControlType = IE_GUI_GAMECONTROL;
	gamewin->AddControl( gc );
	AddWindow( gamewin );
	SetVisible( 0, WINDOW_VISIBLE );
	//setting the focus to the game control
	evntmgr->SetFocused(gamewin, gc);
	if (guiscript->LoadScript( "MessageWindow" )) {
		guiscript->RunFunction( "MessageWindow", "OnLoad" );
		gc->SetGUIHidden(false);
	}

	return gc;
}

/* handle main loop events that might destroy or create windows
thus cannot be called from DrawWindows directly
these events are pending until conditions are right
*/
void Interface::HandleEvents()
{
	GameControl *gc = GetGameControl();
	if (gc && (!gc->Owner || !gc->Owner->Visible)) {
		gc=NULL;
	}

	if (EventFlag&EF_SELECTION) {
		EventFlag&=~EF_SELECTION;
		guiscript->RunFunction( "GUICommonWindows", "SelectionChanged", false);
	}

	if (EventFlag&EF_UPDATEANIM) {
		EventFlag&=~EF_UPDATEANIM;
		guiscript->RunFunction( "GUICommonWindows", "UpdateAnimation", false);
	}

	if (EventFlag&EF_PORTRAIT) {
		ieDword tmp = (ieDword) ~0;
		vars->Lookup( "PortraitWindow", tmp );
		if (tmp != (ieDword) ~0) {
			EventFlag&=~EF_PORTRAIT;
			guiscript->RunFunction( "GUICommonWindows", "UpdatePortraitWindow" );
		}
	}

	if (EventFlag&EF_ACTION) {
		ieDword tmp = (ieDword) ~0;
		vars->Lookup( "ActionsWindow", tmp );
		if (tmp != (ieDword) ~0) {
			EventFlag&=~EF_ACTION;
			guiscript->RunFunction( "GUICommonWindows", "UpdateActionsWindow" );
		}
	}

	if ((EventFlag&EF_CONTROL) && gc) {
		EventFlag&=~EF_CONTROL;
		guiscript->RunFunction( "MessageWindow", "UpdateControlStatus" );
		//this is the only value we can use here
		gc->SetGUIHidden(game->ControlStatus & CS_HIDEGUI);
		return;
	}
	if ((EventFlag&EF_SHOWMAP) && gc) {
		ieDword tmp = (ieDword) ~0;
		vars->Lookup( "OtherWindow", tmp );
		if (tmp == (ieDword) ~0) {
			EventFlag &= ~EF_SHOWMAP;
			guiscript->RunFunction( "GUIMA", "ShowMap" );
		}
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
		guiscript->RunFunction( "MessageWindow", "GameExpansion", false );
		return;
	}

	if (EventFlag&EF_CREATEMAZE) {
		EventFlag&=~EF_CREATEMAZE;
		guiscript->RunFunction( "Maze", "CreateMaze", false );
		return;
	}

	if ((EventFlag&EF_RESETTARGET) && gc) {
		EventFlag&=~EF_RESETTARGET;
		EventFlag|=EF_TARGETMODE;
		gc->ResetTargetMode();
		return;
	}

	if ((EventFlag&EF_TARGETMODE) && gc) {
		EventFlag&=~EF_TARGETMODE;
		gc->UpdateTargetMode();
		return;
	}

	if (EventFlag&EF_TEXTSCREEN) {
		EventFlag&=~EF_TEXTSCREEN;
		video->SetMouseEnabled(true);
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

	if (QuitFlag&(QF_QUITGAME|QF_EXITGAME) ) {
		// when reaching this, quitflag should be 1 or 2
		// if Exitgame was set, we'll set Start.py too
		QuitGame (QuitFlag&QF_EXITGAME);
		QuitFlag &= ~(QF_QUITGAME|QF_EXITGAME);
	}

	if (QuitFlag&QF_LOADGAME) {
		QuitFlag &= ~QF_LOADGAME;
		LoadGame(LoadGameIndex.get(), VersionOverride );
		LoadGameIndex.release();
		//after loading a game, always check if the game needs to be upgraded
	}

	if (QuitFlag&QF_ENTERGAME) {
		QuitFlag &= ~QF_ENTERGAME;
		if (game) {
			EventFlag|=EF_EXPANSION;
			timer->Init();

			//rearrange party slots
			game->ConsolidateParty();
			GameControl* gc = StartGameControl();
			//switch map to protagonist
			Actor* actor = GetFirstSelectedPC(true);
			if (actor) {
				gc->ChangeMap(actor, true);
			}
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

static bool GenerateAbilityTables()
{
	FreeAbilityTables();

	//range is: 0 - maximumability
	int tablesize = MaximumAbility+1;
	strmod = (ieWordSigned *) malloc (tablesize * 4 * sizeof(ieWordSigned) );
	if (!strmod)
		return false;
	strmodex = (ieWordSigned *) malloc (101 * 4 * sizeof(ieWordSigned) );
	if (!strmodex)
		return false;
	intmod = (ieWordSigned *) malloc (tablesize * 5 * sizeof(ieWordSigned) );
	if (!intmod)
		return false;
	dexmod = (ieWordSigned *) malloc (tablesize * 3 * sizeof(ieWordSigned) );
	if (!dexmod)
		return false;
	conmod = (ieWordSigned *) malloc (tablesize * 5 * sizeof(ieWordSigned) );
	if (!conmod)
		return false;
	chrmod = (ieWordSigned *) malloc (tablesize * 1 * sizeof(ieWordSigned) );
	if (!chrmod)
		return false;
	lorebon = (ieWordSigned *) malloc (tablesize * 1 * sizeof(ieWordSigned) );
	if (!lorebon)
		return false;
	wisbon = (ieWordSigned *) malloc (tablesize * 1 * sizeof(ieWordSigned) );
	if (!wisbon)
		return false;
	return true;
}

bool Interface::ReadAbilityTable(const ieResRef tablename, ieWordSigned *mem, int columns, int rows)
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
				mem[rows*j+i]=(ieWordSigned) strtol(tab->QueryField(0,j),NULL,0 );
			}
		}
	}
	for (int j=0;j<columns;j++) {
		for( int i=0;i<rows-fix;i++) {
			mem[rows*j+i+fix] = (ieWordSigned) strtol(tab->QueryField(i,j),NULL,0 );
		}
	}
	return true;
}

bool Interface::ReadAbilityTables()
{
	bool ret = GenerateAbilityTables();
	if (!ret)
		return ret;
	ret = ReadAbilityTable("strmod", strmod, 4, MaximumAbility + 1);
	if (!ret)
		return ret;
	ret = ReadAbilityTable("strmodex", strmodex, 4, 101);
	//3rd ed doesn't have strmodex, but has a maximum of 40
	if (!ret && (MaximumAbility<=25) )
		return ret;
	ret = ReadAbilityTable("intmod", intmod, 5, MaximumAbility + 1);
	if (!ret)
		return ret;
	ret = ReadAbilityTable("hpconbon", conmod, 5, MaximumAbility + 1);
	if (!ret)
		return ret;
	if (!HasFeature(GF_3ED_RULES)) {
		//no lorebon in iwd2???
		ret = ReadAbilityTable("lorebon", lorebon, 1, MaximumAbility + 1);
		if (!ret)
			return ret;
		//no dexmod in iwd2???
		ret = ReadAbilityTable("dexmod", dexmod, 3, MaximumAbility + 1);
		if (!ret)
			return ret;
	}
	//this table is a single row (not a single column)
	ret = ReadAbilityTable("chrmodst", chrmod, MaximumAbility + 1, 1);
	if (!ret)
		return ret;
	if (HasFeature(GF_WISDOM_BONUS)) {
		ret = ReadAbilityTable("wisxpbon", wisbon, 1, MaximumAbility + 1);
		if (!ret)
			return ret;
	}
	return true;
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

	return true;
}

bool Interface::ReadSpecialSpells()
{
	int i;
	bool result = true;

	AutoTable table("splspec");
	if (table) {
		SpecialSpellsCount = table->GetRowCount();
		SpecialSpells = (SpellDescType *) malloc( sizeof(SpellDescType) * SpecialSpellsCount);
		for (i=0;i<SpecialSpellsCount;i++) {
			strnlwrcpy(SpecialSpells[i].resref, table->GetRowName(i),8 );
			//if there are more flags, compose this value into a bitfield
			SpecialSpells[i].value = atoi(table->QueryField(i,0) );
		}
	} else {
		result = false;
	}

	table.load("wildmag");
	if (table) {
		SurgeSpell ss;
		for (i = 0; (unsigned)i < table->GetRowCount(); i++) {
			CopyResRef(ss.spell, table->QueryField(i, 0));
			ss.message = strtol(table->QueryField(i, 1), NULL, 0);
			// comment ignored
			SurgeSpells.push_back(ss);
		}
	} else {
		result = false;
	}

	return result;
}

int Interface::GetSpecialSpell(const ieResRef resref)
{
	for (int i=0;i<SpecialSpellsCount;i++) {
		if (!strnicmp(resref, SpecialSpells[i].resref, sizeof(ieResRef))) {
			return SpecialSpells[i].value;
		}
	}
	return 0;
}

//disable spells based on some circumstances
int Interface::CheckSpecialSpell(const ieResRef resref, Actor *actor)
{
	int sp = GetSpecialSpell(resref);

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

bool Interface::ReadAuxItemTables()
{
	int idx;
	bool flag = true;

	if (ItemExclTable) {
		ItemExclTable->RemoveAll(NULL);
	} else {
		ItemExclTable = new Variables();
		ItemExclTable->SetType(GEM_VARIABLES_INT);
	}

	AutoTable aa;

	//don't report error when the file doesn't exist
	if (aa.load("itemexcl")) {
		idx = aa->GetRowCount();
		while (idx--) {
			ieResRef key;

			strnlwrcpy(key,aa->GetRowName(idx),8);
			ieDword value = strtol(aa->QueryField(idx,0),NULL,0);
			ItemExclTable->SetAt(key, value);
		}
	}
	if (ItemDialTable) {
		ItemDialTable->RemoveAll(NULL);
	} else {
		ItemDialTable = new Variables();
		ItemDialTable->SetType(GEM_VARIABLES_INT);
	}
	if (ItemDial2Table) {
		ItemDial2Table->RemoveAll(NULL);
	} else {
		ItemDial2Table = new Variables();
		ItemDial2Table->SetType(GEM_VARIABLES_STRING);
	}

	//don't report error when the file doesn't exist
	if (aa.load("itemdial")) {
		idx = aa->GetRowCount();
		while (idx--) {
			ieResRef key, dlgres;

			strnlwrcpy(key,aa->GetRowName(idx),8);
			ieDword value = strtol(aa->QueryField(idx,0),NULL,0);
			ItemDialTable->SetAt(key, value);
			strnlwrcpy(dlgres,aa->QueryField(idx,1),8);
			ItemDial2Table->SetAtCopy(key, dlgres);
		}
	}

	if (ItemTooltipTable) {
		ItemTooltipTable->RemoveAll(ReleaseItemTooltip);
	} else {
		ItemTooltipTable = new Variables();
		ItemTooltipTable->SetType(GEM_VARIABLES_POINTER);
	}

	//don't report error when the file doesn't exist
	if (aa.load("tooltip")) {
		idx = aa->GetRowCount();
		while (idx--) {
			ieResRef key;
			int *tmppoi = (int *) malloc(sizeof(int)*3);

			strnlwrcpy(key,aa->GetRowName(idx),8);
			for (int i=0;i<3;i++) {
				tmppoi[i] = atoi(aa->QueryField(idx,i));
			}
			ItemTooltipTable->SetAt(key, (void*)tmppoi);
		}
	}
	return flag;
}

//Static
const char *Interface::GetDeathVarFormat()
{
	return DeathVarFormat;
}

int Interface::GetItemExcl(const ieResRef itemname) const
{
	ieDword value;

	if (ItemExclTable && ItemExclTable->Lookup(itemname, value)) {
		return (int) value;
	}
	return 0;
}

int Interface::GetItemTooltip(const ieResRef itemname, int header, int identified)
{
	int *value = NULL;

	if (ItemTooltipTable) {
		void* lookup = NULL;
		ItemTooltipTable->Lookup(itemname, lookup);
		value = (int*)lookup;
	}
	if (value && (value[header]>=0)) {
		return value[header];
	}
	Item *item = gamedata->GetItem(itemname, true);
	if (!item) {
		return -1;
	}
	int ret = identified?item->ItemNameIdentified:item->ItemName;
	gamedata->FreeItem(item, itemname, 0);
	return ret;
}

int Interface::GetItemDialStr(const ieResRef itemname) const
{
	ieDword value;

	if (ItemDialTable && ItemDialTable->Lookup(itemname, value)) {
		return (int) value;
	}
	return -1;
}

//second value is the item dialog resource returned by this method
int Interface::GetItemDialRes(const ieResRef itemname, ieResRef retval) const
{
	if (ItemDial2Table && ItemDial2Table->Lookup(itemname, retval, sizeof(ieResRef))) {
		return 1;
	}
	return 0;
}

bool Interface::ReadAreaAliasTable(const ieResRef tablename)
{
	if (AreaAliasTable) {
		AreaAliasTable->RemoveAll(NULL);
	} else {
		AreaAliasTable = new Variables();
		AreaAliasTable->SetType(GEM_VARIABLES_INT);
	}

	AutoTable aa(tablename);
	if (!aa) {
		//don't report error when the file doesn't exist
		return true;
	}

	int idx = aa->GetRowCount();
	while (idx--) {
		ieResRef key;

		strnlwrcpy(key,aa->GetRowName(idx),8);
		ieDword value = atoi(aa->QueryField(idx,0));
		AreaAliasTable->SetAt(key, value);
	}
	return true;
}

//this isn't const
int Interface::GetAreaAlias(const ieResRef areaname) const
{
	ieDword value;

	if (AreaAliasTable && AreaAliasTable->Lookup(areaname, value)) {
		return (int) value;
	}
	return -1;
}

bool Interface::ReadMusicTable(const ieResRef tablename, int col) {
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
		di.value = strtol(tm->QueryField(i, 2), (char **) NULL, 16);
		di.iwd_mod_type = atoi(tm->QueryField(i, 3));
		di.reduction = atoi(tm->QueryField(i, 4));
		DamageInfoMap.insert(std::make_pair ((ieDword)di.value, di));
	}

	return true;
}

bool Interface::ReadReputationModTable() {
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

bool Interface::ReadModalStates()
{
	AutoTable table("modal");
	if (!table)
		return false;

	ModalStatesStruct ms;
	for (unsigned short i = 0; i < table->GetRowCount(); i++) {
		CopyResRef(ms.spell, table->QueryField(i, 0));
		strncpy(ms.action, table->QueryField(i, 1), 16);
		ms.entering_str = atoi(table->QueryField(i, 2));
		ms.leaving_str = atoi(table->QueryField(i, 3));
		ms.failed_str = atoi(table->QueryField(i, 4));
		ms.aoe_spell = atoi(table->QueryField(i, 5));
		ModalStates.push_back(ms);
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
	if (vars->Lookup("Tooltips", TooltipDelay)) {
		// the games store the slider position*10, not the actual delay
		TooltipDelay *= TOOLTIP_DELAY_FACTOR/10;
	}

	Font* fps = GetFont( TextFontResRef );
	// TODO: if we ever want to support dynamic resolution changes this will break
	const Region fpsRgn( 0, Height - 30, 100, 30 );
	wchar_t fpsstring[20] = {L"???.??? fps"};

	unsigned long frame = 0, time, timebase;
	timebase = GetTickCount();
	double frames = 0.0;
	Palette* palette = new Palette( ColorWhite, ColorBlack );
	do {
		//don't change script when quitting is pending

		while (QuitFlag && QuitFlag != QF_KILL) {
			HandleFlags();
		}
		//eventflags are processed only when there is a game
		if (EventFlag && game) {
			HandleEvents();
		}
		HandleGUIBehaviour();

		GameLoop();
		DrawWindows(true);
		if (DrawFPS) {
			frame++;
			time = GetTickCount();
			if (time - timebase > 1000) {
				frames = ( frame * 1000.0 / ( time - timebase ) );
				timebase = time;
				frame = 0;
				swprintf(fpsstring, sizeof(fpsstring), L"%.3f fps", frames);
			}
			video->DrawRect( fpsRgn, ColorBlack );
			fps->Print( fpsRgn, String(fpsstring), palette,
					   IE_FONT_ALIGN_LEFT | IE_FONT_ALIGN_MIDDLE );
		}
		if (TickHook)
			TickHook();
	} while (video->SwapBuffers() == GEM_OK && !(QuitFlag&QF_KILL));
	gamedata->FreePalette( palette );
}

int Interface::ReadResRefTable(const ieResRef tablename, ieResRef *&data)
{
	int count = 0;

	if (data) {
		free(data);
		data = NULL;
	}
	AutoTable tm(tablename);
	if (!tm) {
		Log(ERROR, "Core", "Cannot find %s.2da.", tablename);
		return 0;
	}
	count = tm->GetRowCount();
	data = (ieResRef *) calloc( count, sizeof(ieResRef) );
	for (int i = 0; i < count; i++) {
		strnlwrcpy( data[i], tm->QueryField( i, 0 ), 8 );
		//* marks an empty resource
		if (data[i][0]=='*') {
			data[i][0]=0;
		}
	}
	return count;
}

int Interface::LoadSprites()
{
	ieDword i;
	int size;
	if (!IsAvailable( IE_2DA_CLASS_ID )) {
		Log(ERROR, "Core", "No 2DA Importer Available.");
		return GEM_ERROR;
	}

	Log(MESSAGE, "Core", "Loading Cursors...");
	AnimationFactory* anim;
	anim = (AnimationFactory*) gamedata->GetFactoryResource("cursors", IE_BAM_CLASS_ID);
	if (anim)
	{
		CursorCount = anim->GetCycleCount();
		Cursors = new Sprite2D * [CursorCount];
		for (int i = 0; i < CursorCount; i++) {
			Cursors[i] = anim->GetFrame( 0, (ieByte) i );
		}
	}

	// this is the last existing cursor type
	if (CursorCount<IE_CURSOR_WAY) {
		Log(ERROR, "Core", "Failed to load enough cursors (%d < %d).",
				CursorCount, IE_CURSOR_WAY);
		return GEM_ERROR;
	}
	video->SetCursor( Cursors[0], VID_CUR_UP );
	video->SetCursor( Cursors[1], VID_CUR_DOWN );

	// Load fog-of-war bitmaps
	anim = (AnimationFactory*) gamedata->GetFactoryResource("fogowar", IE_BAM_CLASS_ID);
	Log(MESSAGE, "Core", "Loading Fog-Of-War bitmaps...");
	if (!anim || anim->GetCycleSize( 0 ) != 8) {
		// unknown type of fog anim
		Log(ERROR, "Core", "Failed to load Fog-of-War bitmaps.");
		return GEM_ERROR;
	}

	FogSprites[0] = NULL;
	FogSprites[1] = anim->GetFrame( 0, 0 );
	FogSprites[2] = anim->GetFrame( 1, 0 );
	FogSprites[3] = anim->GetFrame( 2, 0 );

	FogSprites[4] = video->MirrorSpriteVertical( FogSprites[1], false );

	FogSprites[5] = NULL;

	FogSprites[6] = video->MirrorSpriteVertical( FogSprites[3], false );

	FogSprites[7] = NULL;

	FogSprites[8] = video->MirrorSpriteHorizontal( FogSprites[2], false );

	FogSprites[9] = video->MirrorSpriteHorizontal( FogSprites[3], false );

	FogSprites[10] = NULL;
	FogSprites[11] = NULL;

	FogSprites[12] = video->MirrorSpriteHorizontal( FogSprites[6], false );

	FogSprites[16] = anim->GetFrame( 3, 0 );
	FogSprites[17] = anim->GetFrame( 4, 0 );
	FogSprites[18] = anim->GetFrame( 5, 0 );
	FogSprites[19] = anim->GetFrame( 6, 0 );

	FogSprites[20] = video->MirrorSpriteVertical( FogSprites[17], false );

	FogSprites[21] = NULL;

	FogSprites[23] = NULL;

	FogSprites[24] = video->MirrorSpriteHorizontal( FogSprites[18], false );

	FogSprites[25] = anim->GetFrame( 7, 0 );

	{
		Sprite2D *tmpsprite = video->MirrorSpriteVertical( FogSprites[25], false );
		FogSprites[22] = video->MirrorSpriteHorizontal( tmpsprite, false );
		Sprite2D::FreeSprite( tmpsprite );
	}

	FogSprites[26] = NULL;
	FogSprites[27] = NULL;

	{
		Sprite2D *tmpsprite = video->MirrorSpriteVertical( FogSprites[19], false );
		FogSprites[28] = video->MirrorSpriteHorizontal( tmpsprite, false );
		Sprite2D::FreeSprite( tmpsprite );
	}

	i = 0;
	vars->Lookup("3D Acceleration", i);
	if (i) {
		for(i=0;i<sizeof(FogSprites)/sizeof(Sprite2D *);i++ ) {
			if (FogSprites[i]) {
				Sprite2D* alphasprite = video->CreateAlpha( FogSprites[i] );
				Sprite2D::FreeSprite ( FogSprites[i] );
				FogSprites[i] = alphasprite;
			}
		}
	}

	// Load ground circle bitmaps (PST only)
	//block required due to msvc6.0 incompatibility
	for (size = 0; size < MAX_CIRCLE_SIZE; size++) {
		if (GroundCircleBam[size][0]) {
			anim = (AnimationFactory*) gamedata->GetFactoryResource(GroundCircleBam[size], IE_BAM_CLASS_ID);
			if (!anim || anim->GetCycleCount() != 6) {
				// unknown type of circle anim
				Log(ERROR, "Core", "Loading Ground circle bitmaps...");
				return GEM_ERROR;
			}

			for (int i = 0; i < 6; i++) {
				Sprite2D* sprite = anim->GetFrame( 0, (ieByte) i );
				if (GroundCircleScale[size]) {
					GroundCircles[size][i] = video->SpriteScaleDown( sprite, GroundCircleScale[size] );
					Sprite2D::FreeSprite( sprite );
				} else {
					GroundCircles[size][i] = sprite;
				}
			}
		}
	}

	Log(MESSAGE, "Core", "Loading Ground circle bitmaps...");

	if (TooltipBackResRef[0]) {
		anim = (AnimationFactory*) gamedata->GetFactoryResource(TooltipBackResRef, IE_BAM_CLASS_ID);
		Log(MESSAGE, "Core", "Initializing Tooltips...");
		if (!anim) {
			Log(ERROR, "Core", "Failed to initialize tooltips.");
			return GEM_ERROR;
		}
		TooltipBack = new Sprite2D * [3];
		for (int i = 0; i < 3; i++) {
			TooltipBack[i] = anim->GetFrame( 0, (ieByte) i );
			TooltipBack[i]->XPos = 0;
			TooltipBack[i]->YPos = 0;
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

		IeResRef resref = tab->QueryField(rowName, "RESREF");
		int needpalette = atoi(tab->QueryField(rowName, "NEED_PALETTE"));
		const char* font_name = tab->QueryField( rowName, "FONT_NAME" );
		ieWord font_size = atoi( tab->QueryField( rowName, "PX_SIZE" ) ); // not available in BAM fonts.
		FontStyle font_style = (FontStyle)atoi( tab->QueryField( rowName, "STYLE" ) ); // not available in BAM fonts.

		Palette* pal = NULL;
		if (needpalette) {
			Color fore = {0xff, 0xff, 0xff, 0};
			Color back = {0x00, 0x00, 0x00, 0};
			const char* colorString = tab->QueryField( rowName, "COLOR" );
			if (colorString) {
				sscanf(colorString, "0x%2hhx%2hhx%2hhx%2hhx", &fore.r, &fore.g, &fore.b, &fore.a);
			}
			if (TooltipFontResRef == resref) {
				if (fore.a != 0xff) {
					// FIXME: should have an explaination
					back = fore;
					fore = Color();
				}
			}
			pal = new Palette(fore, back);
		}

		Font* fnt = NULL;
		ResourceHolder<FontManager> fntMgr(font_name);
		if (fntMgr) fnt = fntMgr->GetFont(font_size, font_style, pal);
		gamedata->FreePalette(pal);

		if (!fnt) {
			Log(WARNING, "Core", "Unable to load font resource: %s for ResRef %s", font_name, resref.CString());
		} else {
			fonts[resref] = fnt;
			Log(MESSAGE, "Core", "Loaded Font: %s for ResRef %s", font_name, resref.CString());
		}
	}

	Log(MESSAGE, "Core", "Fonts Loaded...");
	return GEM_OK;
}

int Interface::Init(InterfaceConfig* config)
{
	if (!config) {
		Log(FATAL, "Core", "No Configuration context.");
		return GEM_ERROR;
	}

	//once GemRB own format is working well, this might be set to 0
	SaveAsOriginal = 1;

	plugin_flags = new Variables();
	plugin_flags->SetType( GEM_VARIABLES_INT );

	Log(MESSAGE, "Core", "Initializing the Event Manager...");
	evntmgr = new EventMgr();

	lists = new Variables();
	if (!lists) {
		Log(FATAL, "Core", "Failed to allocate Lists dictionary.");
		return GEM_ERROR;
	}
	lists->SetType( GEM_VARIABLES_POINTER );

	vars = new Variables();
	if (!vars) {
		Log(FATAL, "Core", "Failed to allocate Variables dictionary");
		return GEM_ERROR;
	}
	vars->SetType( GEM_VARIABLES_INT );
	vars->ParseKey(true);

	const char* value = NULL;
#define CONFIG_INT(key, var) \
		value = config->GetValueForKey(key); \
		if (value) \
			var ( atoi( value ) ); \
		value = NULL;

	CONFIG_INT("Bpp", Bpp =);
	vars->SetAt("BitsPerPixel", Bpp); //put into vars so that reading from game.ini wont overwrite
	CONFIG_INT("CaseSensitive", CaseSensitive =);
	CONFIG_INT("DoubleClickDelay", evntmgr->SetDCDelay);
	CONFIG_INT("DrawFPS", DrawFPS = );
	CONFIG_INT("EnableCheatKeys", EnableCheatKeys);
	CONFIG_INT("EndianSwitch", DataStream::SetEndianSwitch);
	CONFIG_INT("FogOfWar", FogOfWar = );
	ieDword FullScreen = 0;
	CONFIG_INT("FullScreen", FullScreen = );
	vars->SetAt("Full Screen", FullScreen); //put into vars so that reading from game.ini wont overwrite
	CONFIG_INT("GUIEnhancements", GUIEnhancements = );
	CONFIG_INT("TouchScrollAreas", TouchScrollAreas = );
	CONFIG_INT("Height", Height = );
	CONFIG_INT("KeepCache", KeepCache = );
	CONFIG_INT("MultipleQuickSaves", MultipleQuickSaves = );
	CONFIG_INT("RepeatKeyDelay", evntmgr->SetRKDelay);
	CONFIG_INT("SaveAsOriginal", SaveAsOriginal = );
	CONFIG_INT("ScriptDebugMode", SetScriptDebugMode);
	CONFIG_INT("SkipIntroVideos", SkipIntroVideos = );
	CONFIG_INT("TooltipDelay", TooltipDelay = );
	CONFIG_INT("Width", Width = );
	CONFIG_INT("IgnoreOriginalINI", IgnoreOriginalINI = );
	CONFIG_INT("UseSoftKeyboard", UseSoftKeyboard = );
	CONFIG_INT("NumFingScroll", NumFingScroll = );
	CONFIG_INT("NumFingKboard", NumFingKboard = );
	CONFIG_INT("NumFingInfo", NumFingInfo = );
	CONFIG_INT("MouseFeedback", MouseFeedback = );

#undef CONFIG_INT

#define CONFIG_STRING(key, var, default) \
		value = config->GetValueForKey(key); \
		if (value && value[0]) { \
			strlcpy(var, value, sizeof(var)); \
		} else if (default && default[0]) { \
			strlcpy(var, default, sizeof(var)); \
		} else var[0] = '\0'; \
		value = NULL;

	CONFIG_STRING("GameName", GameName, GEMRB_STRING);
	CONFIG_STRING("GameType", GameType, "auto");
	// tob type is obsolete
	if (stricmp( GameType, "tob" ) == 0) {
		strlcpy( GameType, "bg2", sizeof(GameType) );
	}

#undef CONFIG_STRING

// assumes that default value does not need to be resolved or fixed in any way
#define CONFIG_PATH(key, var, default) \
		value = config->GetValueForKey(key); \
		if (value && value[0]) { \
			strlcpy(var, value, sizeof(var)); \
			ResolveFilePath(var); \
			FixPath(var, true); \
		} else if (default && default[0]) { \
			strlcpy(var, default, sizeof(var)); \
		} else var[0] = '\0'; \
		value = NULL;

	// TODO: make CustomFontPath cross platform and possibly dynamic
	CONFIG_PATH("CustomFontPath", CustomFontPath, "/usr/share/fonts/TTF");
	CONFIG_PATH("GameCharactersPath", GameCharactersPath, "characters");
	CONFIG_PATH("GameDataPath", GameDataPath, "data");
	CONFIG_PATH("GameOverridePath", GameOverridePath, "override");
	CONFIG_PATH("GamePortraitsPath", GamePortraitsPath, "portraits");
	CONFIG_PATH("GameScriptsPath", GameScriptsPath, "scripts");
	CONFIG_PATH("GameSoundsPath", GameSoundsPath, "sounds");

	// Path configureation
	CONFIG_PATH("GemRBPath", GemRBPath,
				CopyGemDataPath(GemRBPath, _MAX_PATH));

	CONFIG_PATH("CachePath", CachePath, "./Cache");
	FixPath( CachePath, false );

	CONFIG_PATH("GUIScriptsPath", GUIScriptsPath, GemRBPath);
	CONFIG_PATH("GamePath", GamePath, "");

	CONFIG_PATH("GemRBOverridePath", GemRBOverridePath, GemRBPath);
	CONFIG_PATH("GemRBUnhardcodedPath", GemRBUnhardcodedPath, GemRBPath);
#ifdef PLUGINDIR
	CONFIG_PATH("PluginsPath", PluginsPath, PLUGINDIR);
#else
	CONFIG_PATH("PluginsPath", PluginsPath, "");
	if (!PluginsPath[0]) {
		PathJoin( PluginsPath, GemRBPath, "plugins", NULL );
	}
#endif

	CONFIG_PATH("SavePath", SavePath, GamePath);
#undef CONFIG_STRING

#define CONFIG_STRING(key, var) \
		value = config->GetValueForKey(key); \
		if (value) \
			var = value; \
		value = NULL;

	CONFIG_STRING("AudioDriver", AudioDriverName);
	CONFIG_STRING("VideoDriver", VideoDriverName);
	CONFIG_STRING("Encoding", Encoding);
#undef CONFIG_STRING

	value = config->GetValueForKey("ModPath");
	if (value) {
		for (char *path = strtok((char*)value,SPathListSeparator);
			 path;
			 path = strtok(NULL,SPathListSeparator)) {
			ModPath.push_back(path);
			ResolveFilePath(ModPath.back());
		}
	}
	value = config->GetValueForKey("SkipPlugin");
	if (value) {
		plugin_flags->SetAt( value, PLF_SKIP );
	}
	value = config->GetValueForKey("DelayPlugin");
	if (value) {
		plugin_flags->SetAt( value, PLF_DELAY );
	}

	int i = 0;
	for(i = 0; i < MAX_CD; i++) {
		char keyname[] = { 'C', 'D', char('1'+i), '\0' };
		value = config->GetValueForKey(keyname);
		if (value) {
			for(char *path = strtok((char*)value, SPathListSeparator);
				path;
				path = strtok(NULL,SPathListSeparator)) {
				CD[i].push_back(path);
				ResolveFilePath(CD[i].back());
			}
		} else {
			// nothing in config so create our own
			char name[_MAX_PATH];

			PathJoin(name, GamePath, keyname, NULL);
			CD[i].push_back(name);
			PathJoin(name, GamePath, GameDataPath, keyname, NULL);
			CD[i].push_back(name);
		}
	}

	if (!MakeDirectories(CachePath)) {
		error("Core", "Unable to create cache directory '%s'", CachePath);
	}

	if ( StupidityDetector( CachePath )) {
		Log(ERROR, "Core", "Cache path %s doesn't exist, not a folder or contains alien files!", CachePath );
		return false;
	}
	if (!KeepCache) DelTree((const char *) CachePath, false);

	Log(MESSAGE, "Core", "Starting Plugin Manager...");
	PluginMgr *plugin = PluginMgr::Get();
#if TARGET_OS_MAC
	// search the bundle plugins first
	// since bundle plugins are loaded first dyld will give them precedence
	// if duplicates are found in the PluginsPath
	char bundlePluginsPath[_MAX_PATH];
	CopyBundlePath(bundlePluginsPath, sizeof(bundlePluginsPath), PLUGINS);
	ResolveFilePath(bundlePluginsPath);
	LoadPlugins(bundlePluginsPath);
#endif
	LoadPlugins(PluginsPath);
	if (plugin && plugin->GetPluginCount()) {
		Log(MESSAGE, "Core", "Plugin Loading Complete...");
	} else {
		Log(FATAL, "Core", "Plugin Loading Failed, check path...");
		return GEM_ERROR;
	}
	plugin->RunInitializers();

	time_t t;
	t = time( NULL );
	srand( ( unsigned int ) t );

	Log(MESSAGE, "Core", "GemRB Core Initialization...");
	Log(MESSAGE, "Core", "Initializing Video Driver...");
	video = ( Video * ) PluginMgr::Get()->GetDriver(&Video::ID, VideoDriverName.c_str());
	if (!video) {
		Log(FATAL, "Core", "No Video Driver Available.");
		return GEM_ERROR;
	}
	if (video->Init() == GEM_ERROR) {
		Log(FATAL, "Core", "Cannot Initialize Video Driver.");
		return GEM_ERROR;
	}
	ieDword brightness = 10;
	ieDword contrast = 5;

	// SDL2 driver requires the display to be created prior to sprite creation (opengl context)
	// we also need the display to exist to create sprites using the display format
	vars->Lookup("Full Screen", FullScreen);
	video->CreateDisplay( Width, Height, Bpp, FullScreen, GameName);
	vars->Lookup("Brightness Correction", brightness);
	vars->Lookup("Gamma Correction", contrast);
	video->SetGamma(brightness, contrast);

	Color defcolor={255,255,255,200};
	SetInfoTextColor(defcolor);

	{
		Log(MESSAGE, "Core", "Initializing Search Path...");
		if (!IsAvailable( PLUGIN_RESOURCE_DIRECTORY )) {
			Log(FATAL, "Core", "no DirectoryImporter!");
			return GEM_ERROR;
		}

		char path[_MAX_PATH];

		PathJoin( path, CachePath, NULL);
		if (!gamedata->AddSource(path, "Cache", PLUGIN_RESOURCE_DIRECTORY)) {
			Log(FATAL, "Core", "The cache path couldn't be registered, please check!");
			return GEM_ERROR;
		}

		size_t i;
		for (i = 0; i < ModPath.size(); ++i)
			gamedata->AddSource(ModPath[i].c_str(), "Mod paths", PLUGIN_RESOURCE_CACHEDDIRECTORY);

		PathJoin( path, GemRBOverridePath, "override", GameType, NULL);
		if (!strcmp( GameType, "auto" ))
			gamedata->AddSource(path, "GemRB Override", PLUGIN_RESOURCE_NULL);
		else
			gamedata->AddSource(path, "GemRB Override", PLUGIN_RESOURCE_CACHEDDIRECTORY);

		PathJoin( path, GemRBOverridePath, "override", "shared", NULL);
		gamedata->AddSource(path, "shared GemRB Override", PLUGIN_RESOURCE_CACHEDDIRECTORY);

		PathJoin( path, GamePath, GameOverridePath, NULL);
		gamedata->AddSource(path, "Override", PLUGIN_RESOURCE_CACHEDDIRECTORY);

		//GAME sounds are intentionally not cached, in IWD there are directory structures,
		//that are not cacheable, also it is totally pointless (this fixed charsounds in IWD)
		PathJoin( path, GamePath, GameSoundsPath, NULL);
		gamedata->AddSource(path, "Sounds", PLUGIN_RESOURCE_DIRECTORY);

		PathJoin( path, GamePath, GameScriptsPath, NULL);
		gamedata->AddSource(path, "Scripts", PLUGIN_RESOURCE_CACHEDDIRECTORY);

		PathJoin( path, GamePath, GamePortraitsPath, NULL);
		gamedata->AddSource(path, "Portraits", PLUGIN_RESOURCE_CACHEDDIRECTORY);

		PathJoin( path, GamePath, GameDataPath, NULL);
		gamedata->AddSource(path, "Data", PLUGIN_RESOURCE_CACHEDDIRECTORY);

		//IWD2 movies are on the CD but not in the BIF
		char *description = strdup("CD1/data");
		for (i = 0; i < MAX_CD; i++) {
			for (size_t j=0;j<CD[i].size();j++) {
				description[2]='1'+i;
				PathJoin( path, CD[i][j].c_str(), GameDataPath, NULL);
				gamedata->AddSource(path, description, PLUGIN_RESOURCE_CACHEDDIRECTORY);
			}
		}
		free(description);

		// most of the old gemrb override files can be found here,
		// so they have a lower priority than the game files and can more easily be modded
		PathJoin( path, GemRBUnhardcodedPath, "unhardcoded", GameType, NULL);
		if (!strcmp(GameType, "auto")) {
			gamedata->AddSource(path, "GemRB Unhardcoded data", PLUGIN_RESOURCE_NULL);
		} else {
			gamedata->AddSource(path, "GemRB Unhardcoded data", PLUGIN_RESOURCE_CACHEDDIRECTORY);
		}
		PathJoin( path, GemRBUnhardcodedPath, "unhardcoded", "shared", NULL);
		gamedata->AddSource(path, "shared GemRB Unhardcoded data", PLUGIN_RESOURCE_CACHEDDIRECTORY);
	}

	{
		Log(MESSAGE, "Core", "Initializing KEY Importer...");
		char ChitinPath[_MAX_PATH];
		PathJoin( ChitinPath, GamePath, "chitin.key", NULL );
		if (!gamedata->AddSource(ChitinPath, "chitin.key", PLUGIN_RESOURCE_KEY)) {
			Log(FATAL, "Core", "Failed to load \"chitin.key\"");
			return GEM_ERROR;
		}
	}

	Log(MESSAGE, "Core", "Initializing GUI Script Engine...");
	guiscript = PluginHolder<ScriptEngine>(IE_GUI_SCRIPT_CLASS_ID);
	if (guiscript == NULL) {
		Log(FATAL, "Core", "Missing GUI Script Engine.");
		return GEM_ERROR;
	}
	if (!guiscript->Init()) {
		Log(FATAL, "Core", "Failed to initialize GUI Script.");
		return GEM_ERROR;
	}
	strcpy( NextScript, "Start" );

	{
		// re-set the gemrb override path, since we now have the correct GameType if 'auto' was used
		char path[_MAX_PATH];
		PathJoin( path, GemRBOverridePath, "override", GameType, NULL);
		gamedata->AddSource(path, "GemRB Override", PLUGIN_RESOURCE_CACHEDDIRECTORY, RM_REPLACE_SAME_SOURCE);
		PathJoin( path, GemRBUnhardcodedPath, "unhardcoded", GameType, NULL);
		gamedata->AddSource(path, "GemRB Unhardcoded data", PLUGIN_RESOURCE_CACHEDDIRECTORY, RM_REPLACE_SAME_SOURCE);
	}

	// Purposely add the font directory last since we will only ever need it at engine load time.
	if (CustomFontPath[0]) gamedata->AddSource(CustomFontPath, "CustomFonts", PLUGIN_RESOURCE_DIRECTORY);

	Log(MESSAGE, "Core", "Reading Game Options...");
	if (!LoadGemRBINI()) {
		Log(FATAL, "Core", "Cannot Load INI.");
		return GEM_ERROR;
	}

	// load the game ini (baldur.ini, torment.ini, icewind.ini ...)
	// read from our version of the config if it is present
	char ini_path[_MAX_PATH] = { '\0' };
	char gemrbINI[_MAX_PATH] = { '\0' };
	char tmp[_MAX_PATH] = { '\0' };
	snprintf(gemrbINI, sizeof(gemrbINI), "gem-%s", INIConfig);
	PathJoin(ini_path, GamePath, gemrbINI, NULL);
	if (file_exists(ini_path)) {
		strlcpy(tmp, INIConfig, sizeof(tmp));
		strlcpy(INIConfig, gemrbINI, sizeof(INIConfig));
	}
	if (!IgnoreOriginalINI) {
		PathJoin( ini_path, GamePath, INIConfig, NULL );
		Log(MESSAGE,"Core", "Loading original game options from %s", ini_path);
	}
	if (!InitializeVarsWithINI(ini_path)) {
		Log(WARNING, "Core", "Unable to set dictionary default values!");
	}

	// restore the game config name if we read it from our version
	if (tmp[0]) {
		strlcpy(INIConfig, tmp, sizeof(INIConfig));
	}

	for (i = 0; i < 8; i++) {
		if (INIConfig[i] == '.')
			break;
		GameNameResRef[i] = INIConfig[i];
	}
	GameNameResRef[i] = 0;

	Log(MESSAGE, "Core", "Reading Encoding Table...");
	if (!LoadEncoding()) {
		Log(ERROR, "Core", "Cannot Load Encoding.");
	}

	Log(MESSAGE, "Core", "Creating Projectile Server...");
	projserv = new ProjectileServer();
	if (!projserv->GetHighestProjectileNumber()) {
		Log(ERROR, "Core", "No projectiles are available...");
	}

	Log(MESSAGE, "Core", "Checking for Dialogue Manager...");
	if (!IsAvailable( IE_TLK_CLASS_ID )) {
		Log(FATAL, "Core", "No TLK Importer Available.");
		return GEM_ERROR;
	}
	strings = PluginHolder<StringMgr>(IE_TLK_CLASS_ID);
	Log(MESSAGE, "Core", "Loading Dialog.tlk file...");
	char strpath[_MAX_PATH];
	PathJoin( strpath, GamePath, dialogtlk, NULL );
	FileStream* fs = FileStream::OpenFile(strpath);
	if (!fs) {
		Log(FATAL, "Core", "Cannot find Dialog.tlk.");
		return GEM_ERROR;
	}
	strings->Open(fs);

	{
		Log(MESSAGE, "Core", "Loading Palettes...");
		ResourceHolder<ImageMgr> pal16im(Palette16);
		if (pal16im)
			pal16 = pal16im->GetImage();
		ResourceHolder<ImageMgr> pal32im(Palette32);
		if (pal32im)
			pal32 = pal32im->GetImage();
		ResourceHolder<ImageMgr> pal256im(Palette256);
		if (pal256im)
			pal256 = pal256im->GetImage();
		if (!pal16 || !pal32 || !pal256) {
			Log(FATAL, "Core", "No palettes found.");
			return GEM_ERROR;
		}
		Log(MESSAGE, "Core", "Palettes Loaded");
	}

	if (!IsAvailable( IE_BAM_CLASS_ID )) {
		Log(FATAL, "Core", "No BAM Importer Available.");
		return GEM_ERROR;
	}

	Log(MESSAGE, "Core", "Initializing stock sounds...");
	DSCount = ReadResRefTable ("defsound", DefSound);
	if (DSCount == 0) {
		Log(FATAL, "Core", "Cannot find defsound.2da.");
		return GEM_ERROR;
	}

	Log(MESSAGE, "Core", "Broadcasting Event Manager...");
	video->SetEventMgr( evntmgr );
	Log(MESSAGE, "Core", "Initializing Window Manager...");
	windowmgr = PluginHolder<WindowMgr>(IE_CHU_CLASS_ID);
	if (windowmgr == NULL) {
		Log(FATAL, "Core", "Failed to load Window Manager.");
		return GEM_ERROR;
	}

	int ret = LoadSprites();
	if (ret) return ret;

	ret = LoadFonts();
	if (ret) return ret;

	QuitFlag = QF_CHANGESCRIPT;

	Log(MESSAGE, "Core", "Starting up the Sound Driver...");
	AudioDriver = ( Audio * ) PluginMgr::Get()->GetDriver(&Audio::ID, AudioDriverName.c_str());
	if (AudioDriver == NULL) {
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

	//no need of strdup, variables do copy the key!
	vars->SetAt( "SkipIntroVideos", (unsigned long)SkipIntroVideos );
	vars->SetAt( "GUIEnhancements", (unsigned long)GUIEnhancements );
	vars->SetAt( "TouchScrollAreas", (unsigned long)TouchScrollAreas );

	Log(MESSAGE, "Core", "Initializing Token Dictionary...");
	tokens = new Variables();
	if (!tokens) {
		Log(FATAL, "Core", "Failed to allocate Token dictionary.");
		return GEM_ERROR;
	}
	tokens->SetType( GEM_VARIABLES_STRING );

	Log(MESSAGE, "Core", "Initializing Music Manager...");
	music = PluginHolder<MusicMgr>(IE_MUS_CLASS_ID);
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
		INIresdata = PluginHolder<DataFileMgr>(IE_INI_CLASS_ID);
		DataStream* ds = gamedata->GetResource(resdata? "resdata":"sounds", IE_INI_CLASS_ID);
		if (!INIresdata->Open(ds)) {
			Log(WARNING, "Core", "Failed to load resource data.");
		}
	}

	if (HasFeature( GF_HAS_PARTY_INI )) {
		Log(MESSAGE, "Core", "Loading precreated teams setup...");
		INIparty = PluginHolder<DataFileMgr>(IE_INI_CLASS_ID);
		char tINIparty[_MAX_PATH];
		PathJoin( tINIparty, GamePath, "Party.ini", NULL );
		FileStream* fs = FileStream::OpenFile( tINIparty );
		if (!INIparty->Open(fs)) {
			Log(WARNING, "Core", "Failed to load precreated teams.");
		}
	}

	if (HasFeature(GF_IWD2_DEATHVARFORMAT)) {
		memcpy(DeathVarFormat, IWD2DeathVarFormat, sizeof(ieVariable));
	}

	if (HasFeature( GF_HAS_BEASTS_INI )) {
		Log(MESSAGE, "Core", "Loading beasts definition File...");
		INIbeasts = PluginHolder<DataFileMgr>(IE_INI_CLASS_ID);
		char tINIbeasts[_MAX_PATH];
		PathJoin( tINIbeasts, GamePath, "beast.ini", NULL );
		// FIXME: crashes if file does not open
		FileStream* fs = FileStream::OpenFile( tINIbeasts );
		if (!INIbeasts->Open(fs)) {
			Log(WARNING, "Core", "Failed to load beast definitions.");
		}

		Log(MESSAGE, "Core", "Loading quests definition File...");
		INIquests = PluginHolder<DataFileMgr>(IE_INI_CLASS_ID);
		char tINIquests[_MAX_PATH];
		PathJoin( tINIquests, GamePath, "quests.ini", NULL );
		// FIXME: crashes if file does not open
		FileStream* fs2 = FileStream::OpenFile( tINIquests );
		if (!INIquests->Open(fs2)) {
			Log(WARNING, "Core", "Failed to load quest definitions.");
		}
	}
	game = NULL;
	calendar = NULL;
	keymap = NULL;

	Log(MESSAGE, "Core", "Bringing up the Global Timer...");
	timer = new GlobalTimer();
	if (!timer) {
		Log(FATAL, "Core", "Failed to create global timer.");
		return GEM_ERROR;
	}

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

	Log(MESSAGE, "Core", "Initializing ability tables...");
	ret = ReadAbilityTables();
	if (!ret) {
		Log(FATAL, "Core", "Failed to initialize ability tables...");
		return GEM_ERROR;
	}

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

	Log(MESSAGE, "Core", "Reading item tables...");
	ret = ReadAuxItemTables();
	if (!ret) {
		Log(FATAL, "Core", "Failed to read item tables...");
		return GEM_ERROR;
	}

	ret = ReadDamageTypeTable();
	Log(MESSAGE, "Core", "Reading damage type table...");
	if (!ret) {
		Log(WARNING, "Core", "Reading damage type table...");
	}

	Log(MESSAGE, "Core", "Reading modal states table...");
	ret = ReadModalStates();
	if (!ret) {
		Log(ERROR, "Core", "Failed to modal states table...");
	}

	Log(MESSAGE, "Core", "Reading game script tables...");
	InitializeIEScript();

	Log(MESSAGE, "Core", "Initializing keymap tables...");
	keymap = new KeyMap();
	ret = keymap->InitializeKeyMap("keymap.ini", "keymap");
	if (!ret) {
		Log(WARNING, "Core", "Failed to initialize keymaps.");
	}

	Log(MESSAGE, "Core", "Setting up the Console...");
	console = new Console(Region(0, 0, Width, 25));
	console->SetFont( GetFont(MovieFontResRef) );
	Sprite2D* cursor = GetCursorSprite();
	if (!cursor) {
		Log(ERROR, "Core", "Failed to load cursor sprite.");
	} else
		console->SetCursor (cursor);

	Log(MESSAGE, "Core", "Core Initialization Complete!");
	return GEM_OK;
}

bool Interface::IsAvailable(SClass_ID filetype) const
{
	return PluginMgr::Get()->IsAvailable( filetype );
}

WorldMap *Interface::GetWorldMap(const char *map)
{
	int index = worldmap->FindAndSetCurrentMap(map?map:game->CurrentArea);
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
			Log(ERROR, "Interface", "No extension associated to class ID: %lu", (unsigned long) type );
	}
	return NULL;
}

void Interface::FreeString(char *&str) const
{
	if (str) {
		free(str);
	}
	str = NULL;
}

ieStrRef Interface::UpdateString(ieStrRef strref, const char *text) const
{
	return strings->UpdateString( strref, text );
}

char* Interface::GetCString(ieStrRef strref, ieDword options) const
{
	ieDword flags = 0;

	if (!(options & IE_STR_STRREFOFF)) {
		vars->Lookup( "Strref On", flags );
	}
	return strings->GetCString( strref, flags | options );
}

String* Interface::GetString(ieStrRef strref, ieDword options) const
{
	ieDword flags = 0;

	if (!(options & IE_STR_STRREFOFF)) {
		vars->Lookup( "Strref On", flags );
	}
	return strings->GetString( strref, flags | options );
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

static const char *game_flags[GF_COUNT+1]={
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
		"AutomapIni",         //16GF_AUTOMAP_INI
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
		"HasWisdomBonusTable",//47GF_WISDOM_BONUS
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
		"SkipUpdateHack",     //74GF_SKIPUPDATE_HACK
		"MeleeHeaderUsesProjectile", //75GF_MELEEHEADER_USESPROJECTILE
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
	PluginHolder<DataFileMgr> ini(IE_INI_CLASS_ID);
	ini->Open(inifile);

	const char *s;

	// Resrefs are already initialized in Interface::Interface()
	s = ini->GetKeyAsString( "resources", "CursorBAM", NULL );
	if (s)
		strnlwrcpy( CursorBam, s, 8 ); //console cursor

	s = ini->GetKeyAsString( "resources", "ScrollCursorBAM", NULL );
	if (s)
		strnlwrcpy( ScrollCursorBam, s, 8 );

	s = ini->GetKeyAsString( "resources", "ButtonFont", NULL );
	if (s)
		ButtonFontResRef = s;

	s = ini->GetKeyAsString( "resources", "TooltipFont", NULL );
	if (s)
		TooltipFontResRef = s;

	s = ini->GetKeyAsString( "resources", "MovieFont", NULL );
	if (s)
		MovieFontResRef = s;

	s = ini->GetKeyAsString( "resources", "TooltipBack", NULL );
	if (s)
		strnlwrcpy( TooltipBackResRef, s, 8 );

	//which stat determines the fist weapon (defaults to class)
	Actor::SetFistStat(ini->GetKeyAsInt( "resources", "FistStat", IE_CLASS));

	TooltipMargin = ini->GetKeyAsInt( "resources", "TooltipMargin", TooltipMargin );

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
				strlcpy( GroundCircleBam[size], s, pos - s + 1 );
			} else {
				strcpy( GroundCircleBam[size], s );
			}
		}
	}

	s = ini->GetKeyAsString( "resources", "INIConfig", NULL );
	if (s)
		strcpy( INIConfig, s );

	s = ini->GetKeyAsString( "resources", "Palette16", NULL );
	if (s)
		strcpy( Palette16, s );

	s = ini->GetKeyAsString( "resources", "Palette32", NULL );
	if (s)
		strcpy( Palette32, s );

	s = ini->GetKeyAsString( "resources", "Palette256", NULL );
	if (s)
		strcpy( Palette256, s );

	MaximumAbility = ini->GetKeyAsInt ("resources", "MaximumAbility", 25 );

	RedrawTile = ini->GetKeyAsInt( "resources", "RedrawTile", 0 )!=0;

	for (int i=0;i<GF_COUNT;i++) {
		if (!game_flags[i]) {
			error("Core", "Fix the game flags!\n");
		}
		SetFeature( ini->GetKeyAsInt( "resources", game_flags[i], 0 ), i );
		//printMessage("Option", "", GREEN);
		//print("%s = %s", game_flags[i], HasFeature(i)?"yes":"no");
	}

	ForceStereo = ini->GetKeyAsInt( "resources", "ForceStereo", 0 );

	return true;
}

/** Load the encoding table selected in gemrb.cfg */
bool Interface::LoadEncoding()
{
	DataStream* inifile = gamedata->GetResource( Encoding.c_str(), IE_INI_CLASS_ID );
	if (! inifile) {
		return false;
	}

	Log(MESSAGE, "Core", "Loading encoding definition for %s: '%s'", Encoding.c_str(),
		inifile->originalfile);

	PluginHolder<DataFileMgr> ini(IE_INI_CLASS_ID);
	ini->Open(inifile);

	TLKEncoding.encoding = ini->GetKeyAsString("encoding", "TLKEncoding", TLKEncoding.encoding.c_str());
	TLKEncoding.zerospace = ini->GetKeyAsBool("encoding", "NoSpaces", 0);

	//TextArea::SetNoteString( ini->GetKeyAsString( "strings", "NoteString", NULL ) );

	// TODO: lists are incomplete
	// maybe want to externalize this
	// list compiled form wiki: http://www.gemrb.org/wiki/doku.php?id=engine:encodings
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

/** No descriptions */
Color* Interface::GetPalette(unsigned index, int colors, Color *pal) const
{
	Image *img;
	if (colors == 32) {
		img = pal32;
	} else if (colors <= 32) {
		img = pal16;
	} else if (colors == 256) {
		img = pal256;
	} else {
		return pal;
	}
	if (index >= img->GetHeight()) {
		index = 0;
	}
	for (int i = 0; i < colors; i++) {
		pal[i] = img->GetPixel(i, index);
	}
	return pal;
}
/** Returns a preloaded Font */
Font* Interface::GetFont(const IeResRef& ResRef) const
{
	if (fonts.find(ResRef) != fonts.end()) {
		return fonts.at(ResRef);
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

/** Returns the Event Manager */
EventMgr* Interface::GetEventMgr() const
{
	return evntmgr;
}

/** Returns the Window Manager */
WindowMgr* Interface::GetWindowMgr() const
{
	return windowmgr.get();
}

/** Get GUI Script Manager */
ScriptEngine* Interface::GetGUIScriptEngine() const
{
	return guiscript.get();
}

static EffectRef fx_summon_disable_ref = { "AvatarRemovalModifier", -1 };

//NOTE: if there were more summoned creatures, it will return only the last
Actor *Interface::SummonCreature(const ieResRef resource, const ieResRef vvcres, Scriptable *Owner, Actor *target, const Point &position, int eamod, int level, Effect *fx, bool sexmod)
{
	//maximum number of monsters summoned
	int cnt=10;
	Actor * ab = NULL;

	//TODO:
	//decrease the number of summoned creatures with the number of already summoned creatures here
	//the summoned creatures have a special IE_SEX
	Map *map;
	if (target) {
		map = target->GetCurrentArea();
	} else if (Owner) {
		map = Owner->GetCurrentArea();
	} else {
		map = game->GetCurrentArea();
	}

	if (map) while(cnt--) {
		Actor *tmp = gamedata->GetCreature(resource);
		if (!tmp) {
			return NULL;
		}
		ieDword sex = tmp->GetStat(IE_SEX);
		//TODO: make this external
		int limit = 0;
		switch (sex) {
		case SEX_SUMMON: case SEX_SUMMON_DEMON:
			limit = 5;
			break;
		case SEX_BOTH:
			limit = 1;
			break;
		}

		//if summoner is an actor, filter out opponent summons
		//this is done by clearing the filter when appropriate
		//non actors and neutrals can summon as much as they want
		ieDword flag = GA_NO_DEAD|GA_NO_ALLY|GA_NO_ENEMY;

		if (Owner && Owner->Type==ST_ACTOR) {
			tmp->LastSummoner = Owner->GetGlobalID();
			Actor *owner = (Actor *) Owner;
			ieDword ea = owner->GetStat(IE_EA);
			if (ea<=EA_GOODCUTOFF) {
				flag &= ~GA_NO_ALLY;
			} else if (ea>=EA_EVILCUTOFF) {
				flag &= ~GA_NO_ENEMY;
			}
		}

		if (limit && sexmod && map->CountSummons(flag, sex)>=limit) {
			//summoning limit reached
			displaymsg->DisplayConstantString(STR_SUMMONINGLIMIT, DMC_WHITE);
			delete tmp;
			break;
		}

		ab = tmp;
		//Always use Base stats for the recently summoned creature

		int enemyally;

		if (eamod==EAM_SOURCEALLY || eamod==EAM_SOURCEENEMY) {
			if (Owner && Owner->Type==ST_ACTOR) {
				enemyally = ((Actor *) Owner)->GetStat(IE_EA)>EA_GOODCUTOFF;
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

		// mark the summon, but only if they don't have a special sex already
		if (sexmod && ab->BaseStats[IE_SEX] < SEX_EXTRA) {
			ab->SetBase(IE_SEX, SEX_SUMMON);
		}

		map->AddActor(ab, true);
		ab->SetPosition(position, true, 0);
		ab->RefreshEffects(NULL);

		if (vvcres[0]) {
			ScriptedAnimation* vvc = gamedata->GetScriptedAnimation(vvcres, false);
			if (vvc) {
				//This is the final position of the summoned creature
				//not the original target point
				vvc->XPos=ab->Pos.x;
				vvc->YPos=ab->Pos.y;
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

void Interface::RedrawControls(const char *varname, unsigned int value)
{
	for (unsigned int i = 0; i < windows.size(); i++) {
		Window *win = windows[i];
		if (win != NULL && win->Visible!=WINDOW_INVALID) {
			win->RedrawControls(varname, value);
		}
	}
}

void Interface::RedrawAll()
{
	for (unsigned int i = 0; i < windows.size(); i++) {
		Window *win = windows[i];
		if (win != NULL && win->Visible!=WINDOW_INVALID) {
			win->Invalidate();
		}
	}
}

/** Loads a WindowPack (CHUI file) in the Window Manager */
bool Interface::LoadWindowPack(const char* name)
{
	DataStream* stream = gamedata->GetResource( name, IE_CHU_CLASS_ID );
	if (stream == NULL) {
		Log(ERROR, "Interface", "Error: Cannot find %s.chu", name );
		return false;
	}
	if (!GetWindowMgr()->Open(stream)) {
		Log(ERROR, "Interface", "Error: Cannot Load %s.chu", name );
		return false;
	}

	CopyResRef( WindowPack, name );
	return true;
}

/** Loads a Window in the Window Manager */
int Interface::LoadWindow(unsigned short WindowID)
{
	unsigned int i;
	GameControl *gc = GetGameControl ();

	for (i = 0; i < windows.size(); i++) {
		Window *win = windows[i];
		if (win == NULL)
			continue;
		if (win->Visible==WINDOW_INVALID) {
			continue;
		}
		if (win->WindowID == WindowID &&
			!strnicmp( WindowPack, win->WindowPack, sizeof(WindowPack) )) {
			SetOnTop( i );
			win->Invalidate();
			if (gc)
				gc->SetScrolling( false );
			return i;
		}
	}
	Window* win = windowmgr->GetWindow( WindowID );
	if (win == NULL) {
		return -1;
	}
	memcpy( win->WindowPack, WindowPack, sizeof(WindowPack) );

	int slot = -1;
	for (i = 0; i < windows.size(); i++) {
		if (windows[i] == NULL) {
			slot = i;
			break;
		}
	}
	if (slot == -1) {
		windows.push_back( win );
		slot = ( int ) windows.size() - 1;
	} else {
		windows[slot] = win;
	}
	win->Invalidate();
	if (gc)
		gc->SetScrolling( false );
	return slot;
}
// FIXME: it's a clone of LoadWindow
/** Creates a Window in the Window Manager */
int Interface::CreateWindow(unsigned short WindowID, int XPos, int YPos, unsigned int Width, unsigned int Height, char* Background)
{
	unsigned int i;

	for (i = 0; i < windows.size(); i++) {
		if (windows[i] == NULL)
			continue;
		if (windows[i]->WindowID == WindowID && !stricmp( WindowPack,
			windows[i]->WindowPack )) {
			SetOnTop( i );
			windows[i]->Invalidate();
			return i;
		}
	}

	Window* win = new Window( WindowID, (ieWord) XPos, (ieWord) YPos, (ieWord) Width, (ieWord) Height );
	if (Background[0]) {
		ResourceHolder<ImageMgr> mos(Background);
		if (mos != NULL) {
			win->SetBackGround( mos->GetSprite2D(), true );
		}
	}

	strcpy( win->WindowPack, WindowPack );

	int slot = -1;
	for (i = 0; i < windows.size(); i++) {
		if (windows[i] == NULL) {
			slot = i;
			break;
		}
	}
	if (slot == -1) {
		windows.push_back( win );
		slot = ( int ) windows.size() - 1;
	} else {
		windows[slot] = win;
	}
	win->Invalidate();
	return slot;
}

/** Sets a Window on the Top */
void Interface::SetOnTop(int Index)
{
	std::vector<int>::iterator t;
	for(t = topwin.begin(); t != topwin.end(); ++t) {
		if((*t) == Index) {
			topwin.erase(t);
			break;
		}
	}
	if(topwin.size() != 0)
		topwin.insert(topwin.begin(), Index);
	else
		topwin.push_back(Index);
}
/** Add a window to the Window List */
void Interface::AddWindow(Window * win)
{
	int slot = -1;
	for(unsigned int i = 0; i < windows.size(); i++) {
		Window *w = windows[i];

		if(w==NULL) {
			slot = i;
			break;
		}
	}
	if(slot == -1) {
		windows.push_back(win);
	}
	else
		windows[slot] = win;
	win->Invalidate();
}

/** Get a Control on a Window */
int Interface::GetControl(unsigned short WindowIndex, unsigned long ControlID) const
{
	if (WindowIndex >= windows.size()) {
		return -1;
	}
	Window* win = windows[WindowIndex];
	if (win == NULL) {
		return -1;
	}
	int i = 0;
	while (true) {
		Control* ctrl = win->GetControl( (unsigned short) i );
		if (ctrl == NULL)
			return -1;
		if (ctrl->ControlID == ControlID)
			return i;
		i++;
	}
}
/** Adjust the Scrolling factor of a control (worldmap atm) */
int Interface::AdjustScrolling(unsigned short WindowIndex,
		unsigned short ControlIndex, short x, short y)
{
	if (WindowIndex >= windows.size()) {
		return -1;
	}
	Window* win = windows[WindowIndex];
	if (win == NULL) {
		return -1;
	}
	Control* ctrl = win->GetControl( ControlIndex );
	if (ctrl == NULL) {
		return -1;
	}
	switch(ctrl->ControlType) {
		case IE_GUI_WORLDMAP:
			((WorldMapControl *) ctrl)->AdjustScrolling(x,y);
			break;
		default: //doesn't work for these
			return -1;
	}
	return 0;
}

/** Set the Tooltip text of a Control */
int Interface::SetTooltip(unsigned short WindowIndex,
		unsigned short ControlIndex, const char* string, int Function)
{
	if (WindowIndex >= windows.size()) {
		return -1;
	}
	Window* win = windows[WindowIndex];
	if (win == NULL) {
		return -1;
	}
	Control* ctrl = win->GetControl( ControlIndex );
	if (ctrl == NULL) {
		return -1;
	}

	if (Function) {
		win->FunctionBar = true;
		evntmgr->SetFunctionBar(win);
		ctrl->SetFunctionNumber(Function-1);
	}
	return ctrl->SetTooltip( string );
}

void Interface::DisplayTooltip(int x, int y, Control *ctrl)
{
	if (tooltip_ctrl && tooltip_ctrl == ctrl && tooltip_x == x && tooltip_y == y)
		return;
	tooltip_x = x;
	tooltip_y = y;
	tooltip_currtextw = 0;
	if (x && y && tooltip_ctrl != ctrl) {
		// use a sound handle so we can stop previous unroll sounds
		if (tooltip_sound) {
			tooltip_sound->Stop();
			tooltip_sound.release();
		}
		// exactly like PlaySound(DS_TOOLTIP) but storing the handle
		tooltip_sound = AudioDriver->Play(DefSound[DS_TOOLTIP]);
	}
	tooltip_ctrl = ctrl;
}

int Interface::GetVisible(unsigned short WindowIndex) const
{
	if (WindowIndex >= windows.size()) {
		return -1;
	}
	Window* win = windows[WindowIndex];
	if (win == NULL) {
		return -1;
	}
	return win->Visible;
}
/** Set a Window Visible Flag */
int Interface::SetVisible(unsigned short WindowIndex, int visible)
{
	if (WindowIndex >= windows.size()) {
		return -1;
	}
	Window* win = windows[WindowIndex];
	if (win == NULL) {
		return -1;
	}
	if (visible!=WINDOW_FRONT) {
		win->Visible = (char) visible;
	}
	switch (visible) {
		case WINDOW_GRAYED:
			win->Invalidate();
			win->DrawWindow();
			//here is a fallthrough
		case WINDOW_INVISIBLE:
			//hiding the viewport if the gamecontrol window was made invisible
			if (win->WindowID==65535) {
				video->SetViewport( 0,0,0,0 );
			}
			evntmgr->DelWindow( win );
			break;

		case WINDOW_VISIBLE:
			if (win->WindowID==65535) {
				video->SetViewport( win->XPos, win->YPos, win->Width, win->Height);
			}
			//here is a fallthrough
		case WINDOW_FRONT:
			if (win->Visible==WINDOW_VISIBLE) {
				evntmgr->AddWindow( win );
				if (win->FunctionBar) {
					evntmgr->SetFunctionBar( win );
				}
			}
			win->Invalidate();
			SetOnTop( WindowIndex );
			break;
	}
	return 0;
}


/** Set the Status of a Control in a Window */
int Interface::SetControlStatus(unsigned short WindowIndex,
		unsigned short ControlIndex, unsigned long Status)
{
	//don't set the status of an already invalidated window
	Window* win = GetWindow(WindowIndex);
	if (win == NULL) {
		return -1;
	}
	Control* ctrl = win->GetControl( ControlIndex );
	if (ctrl == NULL) {
		return -1;
	}
	if (Status&IE_GUI_CONTROL_FOCUSED) {
		evntmgr->SetFocused( win, ctrl);
	}

	//check if the status parameter was intended to use with this control
	//Focus will sadly break this at the moment, because it is common for all control types
	int check = (Status >> 24) & 0xff;
	if ( (check!=0x7f) && (ctrl->ControlType != check) ) {
		return -2;
	}

	switch (ctrl->ControlType) {
		case IE_GUI_BUTTON:
			//Button
			{
				Button* btn = ( Button* ) ctrl;
				btn->SetState( ( unsigned char ) ( Status & 0x7f ) );
			}
			break;
		case IE_GUI_WORLDMAP:
			break;
		default:
			ctrl->Value = Status & 0x7f;
			break;
	}
	return 0;
}

/** Show a Window in Modal Mode */
int Interface::ShowModal(unsigned short WindowIndex, MODAL_SHADOW Shadow)
{
	if (WindowIndex >= windows.size()) {
		Log(ERROR, "Core", "Window not found");
		return -1;
	}
	Window* win = windows[WindowIndex];
	if (win == NULL) {
		Log(ERROR, "Core", "Window already freed");
		return -1;
	}
	win->Visible = WINDOW_FRONT;
	//don't destroy the other window handlers
	//evntmgr->Clear();
	SetOnTop( WindowIndex );
	evntmgr->AddWindow( win );
	evntmgr->SetFocused( win, NULL );
	win->Invalidate();

	modalShadow = Shadow;
	ModalWindow = win;
	return 0;
}

bool Interface::IsFreezed()
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

	if ( gc && game && (game->selected.size() > 0) ) {
		gc->ChangeMap(GetFirstSelectedPC(true), false);
	}
	//in multi player (if we ever get to it), only the server must call this
	if (do_update) {
		// the game object will run the area scripts as well
		game->UpdateScripts();
	}
}

/** handles hardcoded gui behaviour */
void Interface::HandleGUIBehaviour(void)
{
	GameControl *gc = GetGameControl();
	if (gc) {
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
				gc->SetDialogueFlags(DF_OPENCONTINUEWINDOW|DF_OPENENDWINDOW, BM_NAND);
			} else if (flg & DF_OPENENDWINDOW) {
				guiscript->RunFunction( "GUIWORLD", "OpenEndMessageWindow" );
				gc->SetDialogueFlags(DF_OPENCONTINUEWINDOW|DF_OPENENDWINDOW, BM_NAND);
			}
		}

		//handling container
		if (CurrentContainer && UseContainer) {
			if (!(flg & DF_IN_CONTAINER) ) {
				gc->SetDialogueFlags(DF_IN_CONTAINER, BM_OR);
				guiscript->RunFunction( "CommonWindow", "OpenContainerWindow" );
			}
		} else {
			if (flg & DF_IN_CONTAINER) {
				gc->SetDialogueFlags(DF_IN_CONTAINER, BM_NAND);
				guiscript->RunFunction( "CommonWindow", "CloseContainerWindow" );
			}
		}
		//end of gui hacks
	}
}

void Interface::DrawWindows(bool allow_delete)
{
	//here comes the REAL drawing of windows
	static bool modalShield = false;
	if (ModalWindow) {
		if (!modalShield) {
			// only draw the shield layer once
			Color shieldColor = Color(); // clear
			if (modalShadow == MODAL_SHADOW_GRAY) {
				shieldColor.a = 128;
			} else if (modalShadow == MODAL_SHADOW_BLACK) {
				shieldColor.a = 0xff;
			}
			video->DrawRect( Region( 0, 0, Width, Height ), shieldColor );
			RedrawAll(); // wont actually have any effect until the modal window is dismissed.
			modalShield = true;
		}
		ModalWindow->DrawWindow();
		return;
	}
	modalShield = false;

	size_t i = topwin.size();
	while(i--) {
		unsigned int t = topwin[i];

		if ( t >=windows.size() )
			continue;

		//visible ==1 or 2 will be drawn
		Window* win = windows[t];
		if (win != NULL) {
			if (win->Visible == WINDOW_INVALID) {
				if (allow_delete) {
					topwin.erase(topwin.begin()+i);
					evntmgr->DelWindow( win );
					delete win;
					windows[t]=NULL;
				}
			} else if (win->Visible) {
				win->DrawWindow();
			}
		}
	}

	// draw the console
	if (ConsolePopped) {
		console->Draw(0, 0);
	}
}

void Interface::DrawTooltip ()
{
	if (! tooltip_ctrl || !tooltip_ctrl->Tooltip)
		return;

	Font* fnt = GetFont( TooltipFontResRef );
	if (!fnt) {
		return;
	}
	String* tooltip_text = tooltip_ctrl->Tooltip;

	int w1 = 0;
	int w2 = 0;
	int strw = fnt->StringSize( *tooltip_text ).w + 8;
	int w = strw;
	int h = fnt->maxHeight;

	if (TooltipBack) {
		// animate BG tooltips
		// TODO: make tooltip animation an option instead
		// of following hard-coded check!
		if (TooltipMargin == 5) {
			// TODO: make speed an option
			int tooltip_anim_speed = 15;
			if (tooltip_currtextw < strw) {
				tooltip_currtextw += tooltip_anim_speed;
			}
			if (tooltip_currtextw > strw) {
				tooltip_currtextw = strw;
			}
			w = tooltip_currtextw;
		}

		h = TooltipBack[0]->Height;
		w1 = TooltipBack[1]->Width;
		w2 = TooltipBack[2]->Width;
		int margins = TooltipMargin*2;
		w += margins;
		strw += margins;
		int strwmax = TooltipBack[0]->Width - margins;
		//multiline in case of too much text
		if (w > TooltipBack[0]->Width) {
			w = TooltipBack[0]->Width;
			strw = strwmax;
		} else if (strw > strwmax)
			strw = strwmax;
	}

	int strx = tooltip_x - strw / 2;
	int y = tooltip_y - h / 2;
	// Ensure placement within the screen
	if (strx < 0) strx = 0;
	else if (strx + strw + w1 + w2 > Width)
		strx = Width - strw - w1 - w2;
	if (y < 0) y = 0;
	else if (y + h > Height)
		y = Height - h;

	int x = strx + ((strw - w) / 2);

	Region clip = Region( x, y, w, h );
	if (TooltipBack) {
		video->BlitSprite( TooltipBack[0], x + TooltipMargin - (TooltipBack[0]->Width - w) / 2, y, true, &clip );
		video->BlitSprite( TooltipBack[1], x, y, true );
		video->BlitSprite( TooltipBack[2], x + w, y, true );
	}

	if (TooltipBack) {
		clip.x += TooltipBack[1]->Width;
		clip.w -= TooltipBack[2]->Width;
		strx += TooltipMargin;
	}
	Region textr = Region( strx, y, strw, h );

	Region oldclip;
	// clip drawing to the control bounds, then restore after drawing
	video->GetClipRect(oldclip);
	video->SetClipRect(&clip);
	fnt->Print( textr, *tooltip_text, NULL,
			   IE_FONT_ALIGN_CENTER | IE_FONT_ALIGN_MIDDLE );
	video->SetClipRect(&oldclip);
}

//interface for higher level functions, if the window was
//marked for deletion it is not returned
Window* Interface::GetWindow(unsigned short WindowIndex) const
{
	if (WindowIndex < windows.size()) {
		Window *win = windows[WindowIndex];
		if (win && (win->Visible!=WINDOW_INVALID) ) {
			return win;
		}
	}
	return NULL;
}

// this function will determine if wnd is a valid window pointer
// by checking if its WindowID is the same as the reference
bool Interface::IsValidWindow(unsigned short WindowID, Window *wnd) const
{
	size_t WindowIndex = windows.size();
	while (WindowIndex--) {
		if (windows[WindowIndex] == wnd) {
			return wnd->WindowID == WindowID;
		}
	}
	return false;
}

//this function won't delete the window, just mark it for deletion
//it will be deleted in the next DrawWindows cycle
//regardless, the window deleted is inaccessible for gui scripts and
//other high level functions from now
int Interface::DelWindow(unsigned short WindowIndex)
{
	if (WindowIndex >= windows.size()) {
		return -1;
	}
	Window* win = windows[WindowIndex];
	if ((win == NULL) || (win->Visible==WINDOW_INVALID) ) {
		Log(ERROR, "Core", "Window deleted again");
		return -1;
	}
	if (win == ModalWindow) {
		ModalWindow = NULL;
	}
	evntmgr->DelWindow( win );
	win->release();
	//re-capturing new (old) modal window if any
	size_t tw = topwin.size();
	for(size_t i=0;i<tw;i++) {
		Window *tmp = windows[topwin[i]];
		if (tmp->Visible==WINDOW_FRONT) {
			ModalWindow = tmp;
			break;
		}
	}
	return 0;
}

void Interface::DelAllWindows()
{
	vars->SetAt("MessageWindow", (ieDword) ~0);
	vars->SetAt("OptionsWindow", (ieDword) ~0);
	vars->SetAt("PortraitWindow", (ieDword) ~0);
	vars->SetAt("ActionsWindow", (ieDword) ~0);
	vars->SetAt("TopWindow", (ieDword) ~0);
	vars->SetAt("OtherWindow", (ieDword) ~0);
	vars->SetAt("FloatWindow", (ieDword) ~0);
	for(unsigned int WindowIndex=0; WindowIndex<windows.size();WindowIndex++) {
		Window* win = windows[WindowIndex];
		delete win;
	}
	windows.clear();
	topwin.clear();
	evntmgr->Clear();
	ModalWindow = NULL;
}

/** Popup the Console */
void Interface::PopupConsole()
{
	ConsolePopped = !ConsolePopped;
	RedrawAll();
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
		if (ConsolePopped) {
			PopupConsole();
		}
		SetPause(PAUSE_ON);
		vars->SetAt("AskAndExit", 1);

		LoadWindowPack("GUIOPT");
		guiscript->RunFunction("GUIOPT", "OpenQuitMsgWindow");
		Log(MESSAGE, "Info", "Press ctrl-c (or close the window) again to quit GemRB.\n");
	} else {
		ExitGemRB();
	}
}

void Interface::ExitGemRB()
{
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
	PluginHolder<SymbolMgr> sm(IE_IDS_CLASS_ID);
	if (!sm) {
		delete str;
		return -1;
	}
	if (!sm->Open(str)) {
		return -1;
	}
	Symbol s;
	strncpy( s.ResRef, ResRef, 8 );
	s.sm = sm;
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
		if (strnicmp( symbols[i].ResRef, ResRef, 8 ) == 0)
			return ( int ) i;
	}
	return -1;
}
/** Gets a Loaded Symbol Table by its index, returns NULL on error */
Holder<SymbolMgr> Interface::GetSymbol(unsigned int index) const
{
	if (index >= symbols.size()) {
		return Holder<SymbolMgr>();
	}
	if (!symbols[index].sm) {
		return Holder<SymbolMgr>();
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
	symbols[index].sm.release();
	return true;
}
/** Plays a Movie */
int Interface::PlayMovie(const char* ResRef)
{
	ResourceHolder<MoviePlayer> mp(ResRef);
	if (!mp) {
		return -1;
	}

	ieDword subtitles = 0;
	Font *SubtitleFont = NULL;
	Palette *palette = NULL;
	ieDword *frames = NULL;
	ieDword *strrefs = NULL;
	int cnt = 0;
	int offset = 0;

	//one of these two should exist (they both mean the same thing)
	vars->Lookup("Display Movie Subtitles", subtitles);
	if (subtitles) {
		//HoW flag
		cnt=-3;
		offset = 3;
	} else {
		//ToB flag
		vars->Lookup("Display Subtitles", subtitles);
	}
	AutoTable sttable;
	if (subtitles && sttable.load(ResRef)) {
		cnt += sttable->GetRowCount();
		if (cnt>0) {
			frames = (ieDword *) malloc(cnt * sizeof(ieDword) );
			strrefs = (ieDword *) malloc(cnt * sizeof(ieDword) );
		} else {
			cnt = 0;
		}
		if (frames && strrefs) {
			for (int i=0;i<cnt;i++) {
				frames[i] = atoi (sttable->QueryField(i+offset, 0) );
				strrefs[i] = atoi (sttable->QueryField(i+offset, 1) );
			}
		}
		int r = atoi(sttable->QueryField("red", "frame"));
		int g = atoi(sttable->QueryField("green", "frame"));
		int b = atoi(sttable->QueryField("blue", "frame"));
		SubtitleFont = GetFont (MovieFontResRef); //will change
		if (r || g || b) {
			if (SubtitleFont) {
				Color fore = {(unsigned char) r,(unsigned char) g,(unsigned char) b, 0x00};
				Color back = {0x00, 0x00, 0x00, 0x00};
				palette = new Palette( fore, back );
			}
		}
	}

	//shutting down music and ambients before movie
	if (music)
		music->HardEnd();
	AmbientMgr *ambim = AudioDriver->GetAmbientMgr();
	if (ambim) ambim->deactivate();
	video->SetMovieFont(SubtitleFont, palette );
	mp->CallBackAtFrames(cnt, frames, strrefs);
	mp->Play();
	gamedata->FreePalette( palette );
	if (frames)
		free(frames);
	if (strrefs)
		free(strrefs);
	//restarting music
	if (music)
		music->Start();
	if (ambim) ambim->activate();
	//this will fix redraw all windows as they looked like
	//before the movie
	RedrawAll();

	//Setting the movie name to 1
	vars->SetAt( ResRef, 1 );
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
		add += rand() % size + 1;
	}
	return add;
}

DirectoryIterator Interface::GetResourceDirectory(RESOURCE_DIRECTORY dir)
{
	struct ExtFilter : DirectoryIterator::FileFilterPredicate {
		char extension[9];
		ExtFilter(const char* ext) {
			memcpy(extension, ext, sizeof(extension));
		}

		bool operator()(const char* fname) const {
			char* extpos = strrchr(fname, '.');
			if (extpos) {
				extpos++;
				return stricmp(extpos, extension) == 0;
			}
			return false;
		}
	};

	char Path[_MAX_PATH];
	const char* resourcePath = NULL;
	DirectoryIterator::FileFilterPredicate* filter = NULL;
	switch (dir) {
		case DIRECTORY_CHR_PORTRAITS:
			resourcePath = GamePortraitsPath;
			filter = new ExtFilter("BMP");
			if (IsAvailable(IE_PNG_CLASS_ID)) {
				// chain an ORed filter for png
				filter = new OrPredicate<const char*>(filter, new ExtFilter("PNG"));
			}
			break;
		case DIRECTORY_CHR_SOUNDS:
			resourcePath = GameSoundsPath;
			filter = new ExtFilter("WAV");
			break;
		case DIRECTORY_CHR_EXPORTS:
			resourcePath = GameCharactersPath;
			filter = new ExtFilter("CHR");
			break;
	}

	PathJoin( Path, GamePath, resourcePath, NULL );
	DirectoryIterator dirIt(Path);
	dirIt.SetFilterPredicate(filter);
	return dirIt;
}

bool Interface::InitializeVarsWithINI(const char* iniFileName)
{
	if (!core->IsAvailable( IE_INI_CLASS_ID ))
		return false;

	DataFileMgr* defaults = NULL;
	DataFileMgr* overrides = NULL;

	PluginHolder<DataFileMgr> ini(IE_INI_CLASS_ID);
	FileStream* iniStream = FileStream::OpenFile(iniFileName);
	// if filename is not set we assume we are creating defaults without an INI
	if (iniFileName[0] && !ini->Open(iniStream)) {
		Log(WARNING, "Core", "Unable to read defaults from '%s'. Using GemRB default values.", iniFileName);
	} else {
		overrides = ini.get();
	}

	PluginHolder<DataFileMgr> gemINI(IE_INI_CLASS_ID);
	DataStream* gemINIStream = gamedata->GetResource( "defaults", IE_INI_CLASS_ID );

	if (!gemINIStream || !gemINI->Open(gemINIStream)) {
		Log(WARNING, "Core", "Unable to load GemRB default values.");
		defaults = ini.get();
	} else {
		defaults = gemINI.get();
		if (!overrides) {
			overrides = defaults;
		}
	}

	for (int i = 0; i < defaults->GetTagsCount(); i++) {
		const char* tag = defaults->GetTagNameByIndex(i);
		for (int j = 0; j < defaults->GetKeysCount(tag); j++) {
			ieDword nothing;
			const char* key = defaults->GetKeyNameByIndex(tag, j);
			//skip any existing entries. GemRB.cfg has priority
			if (!vars->Lookup(key, nothing)) {
				ieDword defaultVal = defaults->GetKeyAsInt(tag, key, 0);
				vars->SetAt(key, overrides->GetKeyAsInt(tag, key, defaultVal));
			}
		}
	}

	// handle a few special cases
	if (!overrides->GetKeyAsInt("Config", "Sound", 1))
		AudioDriverName = "null";

	if (overrides->GetKeyAsInt("Game Options", "Cheats", 1)) {
		EnableCheatKeys(1);
	}

	// copies
	if (!overrides->GetKeyAsInt("Game Options", "Darkvision", 1)) {
		vars->SetAt("Infravision", (ieDword)0);
	}

	if (!Width || !Height) {
		Height = overrides->GetKeyAsInt("Config", "ConfigHeight", Height);
		int tmpWidth = overrides->GetKeyAsInt("Config", "ConfigWidth", 0);
		if (!tmpWidth) {
			// Resolution is stored as width only. assume 4|3 ratio.
			Width = overrides->GetKeyAsInt("Program Options", "Resolution", Width);
			Height = 0.75 * Width;
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
	char gemrbINI[_MAX_PATH] = { '\0' };
	if (strncmp(INIConfig, "gem-", 4)) {
		snprintf(gemrbINI, sizeof(gemrbINI), "gem-%s", INIConfig);
	}
	PathJoin(ini_path, GamePath, gemrbINI, NULL);
	FileStream *fs = new FileStream();
	if (!fs->Create(ini_path)) {
		PathJoin(ini_path, SavePath, gemrbINI, NULL);
		if (!fs->Create(ini_path)) {
			return false;
		}
	}

	PluginHolder<DataFileMgr> defaultsINI(IE_INI_CLASS_ID);
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

/** Enables/Disables the Cut Scene Mode */
void Interface::SetCutSceneMode(bool active)
{
	GameControl *gc = GetGameControl();

	if (gc) {
		// don't mess with controls/etc if we're already in a cutscene
		if (active == (bool)(gc->GetScreenFlags()&SF_CUTSCENE))
			return;

		gc->SetCutSceneMode( active );
	}
	if (game) {
		if (active) {
			game->ControlStatus |= CS_HIDEGUI;
		} else {
			game->ControlStatus &= ~CS_HIDEGUI;
		}
		SetEventFlag(EF_CONTROL);
	}
	video->SetMouseEnabled(!active);
}

/** returns true if in dialogue or cutscene */
bool Interface::InCutSceneMode() const
{
	GameControl *gc = GetGameControl();
	if (!gc || (gc->GetDialogueFlags()&DF_IN_DIALOG) || (gc->GetScreenFlags()&(SF_DISABLEMOUSE|SF_CUTSCENE))) {
		return true;
	}
	return false;
}

/** Updates the Game Script Engine State */
bool Interface::GSUpdate(bool update_scripts)
{
	if(update_scripts) {
		return timer->Update();
	}
	else {
		timer->Freeze();
		return false;
	}
}

void Interface::QuitGame(int BackToMain)
{
	SetCutSceneMode(false);
	if (timer) {
		//clear cutscenes
		//clear fade/screenshake effects
		timer->Init();
		timer->SetFadeFromColor(0);
	}

	DelAllWindows(); //delete all windows, including GameControl

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
		strcpy(NextScript, "Start");
		QuitFlag |= QF_CHANGESCRIPT;
	}
	GSUpdate(true);
}

void Interface::SetupLoadGame(Holder<SaveGame> sg, int ver_override)
{
	LoadGameIndex = sg;
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
	if (!KeepCache) DelTree((const char *) CachePath, true);
	LoadProgress(15);

	if (sg == NULL) {
		//Load the Default Game
		gam_str = gamedata->GetResource( GameNameResRef, IE_GAM_CLASS_ID );
		sav_str = NULL;
		wmp_str1 = gamedata->GetResource( WorldMapName[0], IE_WMP_CLASS_ID );
		if (WorldMapName[1][0]) {
			wmp_str2 = gamedata->GetResource( WorldMapName[1], IE_WMP_CLASS_ID );
		}
	} else {
		gam_str = sg->GetGame();
		sav_str = sg->GetSave();
		wmp_str1 = sg->GetWmap(0);
		if (WorldMapName[1][0]) {
			wmp_str2 = sg->GetWmap(1);
			if (!wmp_str2) {
				//upgrade an IWD game to HOW
				wmp_str2 = gamedata->GetResource( WorldMapName[1], IE_WMP_CLASS_ID );
			}
		}
	}

	// These are here because of the goto
	PluginHolder<SaveGameMgr> gam_mgr(IE_GAM_CLASS_ID);
	PluginHolder<WorldMapMgr> wmp_mgr(IE_WMP_CLASS_ID);

	if (!gam_str || !(wmp_str1 || wmp_str2) )
		goto cleanup;

	// Load GAM file
	if (!gam_mgr)
		goto cleanup;

	if (!gam_mgr->Open(gam_str))
		goto cleanup;

	new_game = gam_mgr->LoadGame(new Game(), ver_override);
	if (!new_game)
		goto cleanup;

	gam_str = NULL;

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
		PluginHolder<ArchiveImporter> ai(IE_SAV_CLASS_ID);
		if (ai) {
			if (ai->DecompressSaveGame(sav_str) != GEM_OK) {
				goto cleanup;
			}
		}
		delete sav_str;
		sav_str = NULL;
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

	error("Core", "Unable to load game.");

	delete new_game;
	delete new_worldmap;

	delete gam_str;
	delete wmp_str1;
	delete wmp_str2;
	delete sav_str;
}

/* replace the current world map but sync areas available in old and new */
void Interface::UpdateWorldMap(ieResRef wmResRef)
{
	DataStream* wmp_str = gamedata->GetResource(wmResRef, IE_WMP_CLASS_ID);
	PluginHolder<WorldMapMgr> wmp_mgr(IE_WMP_CLASS_ID);

	if (!wmp_str || !wmp_mgr || !wmp_mgr->Open(wmp_str, NULL)) {
		Log(ERROR, "Core", "Could not update world map %s", wmResRef);
		return;
	}

	WorldMapArray *new_worldmap = wmp_mgr->GetWorldMapArray();
	WorldMap *wm = worldmap->GetWorldMap(0);
	WorldMap *nwm = new_worldmap->GetWorldMap(0);

	unsigned int i, ni;
	unsigned int ec = wm->GetEntryCount();
	//update status of the previously existing areas
	for(i=0;i<ec;i++) {
		WMPAreaEntry *ae = wm->GetEntry(i);
		WMPAreaEntry *nae = nwm->GetArea(ae->AreaResRef, ni);
		if (nae != NULL) {
			nae->SetAreaStatus(ae->GetAreaStatus(), BM_SET);
		}
	}
	
	delete worldmap;
	worldmap = new_worldmap;
	CopyResRef(WorldMapName[0], wmResRef);
}

/* swapping out old resources */
void Interface::UpdateMasterScript()
{
	if (game) {
		game->SetScript( GlobalScript, 0 );
	}

	PluginHolder<WorldMapMgr> wmp_mgr(IE_WMP_CLASS_ID);
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

bool Interface::HideGCWindow()
{
	Window *window = GetWindow( 0 );
	// in the beginning, there's no window at all
	if (! window)
		return false;

	Control* gc = window->GetControl(0);
	if (gc->ControlType!=IE_GUI_GAMECONTROL) {
		return false;
	}
	SetVisible(0, WINDOW_INVISIBLE);
	return true;
}

void Interface::UnhideGCWindow()
{
	Window *window = GetWindow( 0 );
	if (!window)
		return;
	Control* gc = window->GetControl(0);
	if (gc->ControlType!=IE_GUI_GAMECONTROL)
		return;
	SetVisible(0, WINDOW_VISIBLE);
}

GameControl *Interface::GetGameControl() const
{
	Window *window = GetWindow( 0 );
	// in the beginning, there's no window at all
	if (! window)
		return NULL;

	Control* gc = window->GetControl(0);
	if (gc == NULL) {
		return NULL;
	}
	if (gc->ControlType!=IE_GUI_GAMECONTROL) {
		return NULL;
	}
	return (GameControl *) gc;
}

bool Interface::InitItemTypes()
{
	int i;

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
		for (i=0;i<ItemTypes;i++) {
			unsigned int value = 0;
			unsigned int k = 1;
			for (int j=0;j<InvSlotTypes;j++) {
				if (strtol(it->QueryField(i,j),NULL,0) ) {
					value |= k;
				}
				k <<= 1;
			}
			//we let any items in the inventory
			slotmatrix[i] = (ieDword) value | SLOT_INVENTORY;
		}
	}

	//itemtype data stores (armor failure and critical damage multipliers), critical range
	itemtypedata.reserve(ItemTypes);
	for (i=0;i<ItemTypes;i++) {
		itemtypedata.push_back(std::vector<int>(4));
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
		int j;
		for (i = 0; i < armcount; i++) {
			int itemtype = (ieWord) atoi( af->QueryField(i,0) );
			if (itemtype<ItemTypes) {
				// we don't need the itemtype column, since it is equal to the position
				for (j=0; j < colcount-1; j++) {
					itemtypedata[itemtype][j] = atoi(af->QueryField(i, j+1));
				}
			}
		}
	}

	//slottype describes the inventory structure
	Inventory::Init();
	AutoTable st("slottype");
	if (slottypes) {
		free(slottypes);
		slottypes = NULL;
	}
	SlotTypes = 0;
	if (st) {
		SlotTypes = st->GetRowCount();
		//make sure unsigned int is 32 bits
		slottypes = (SlotType *) malloc(SlotTypes * sizeof(SlotType) );
		memset(slottypes, -1, SlotTypes * sizeof(SlotType) );
		for (unsigned int row = 0; row < SlotTypes; row++) {
			bool alias;
			unsigned int i = (ieDword) strtol(st->GetRowName(row),NULL,0 );
			if (i>=SlotTypes) continue;
			if (slottypes[i].sloteffects!=0xffffffffu) {
				slottypes[row].slot = i;
				i=row;
				alias = true;
			} else {
				slottypes[row].slot = i;
				alias = false;
			}
			slottypes[i].slottype = (ieDword) strtol(st->QueryField(row,0),NULL,0 );
			slottypes[i].slotid = (ieDword) strtol(st->QueryField(row,1),NULL,0 );
			strnlwrcpy( slottypes[i].slotresref, st->QueryField(row,2), 8 );
			slottypes[i].slottip = (ieDword) strtol(st->QueryField(row,3),NULL,0 );
			slottypes[i].slotflags = (ieDword) strtol(st->QueryField(row,5),NULL,0 );
			//don't fill sloteffects for aliased slots (pst)
			if (alias) {
				continue;
			}
			slottypes[i].sloteffects = (ieDword) strtol(st->QueryField(row,4),NULL,0 );
			//setting special slots
			if (slottypes[i].slottype&SLOT_ITEM) {
				if (slottypes[i].slottype&SLOT_INVENTORY) {
					Inventory::SetInventorySlot(i);
				} else {
					Inventory::SetQuickSlot(i);
				}
			}
			switch (slottypes[i].sloteffects) {
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
	ieDword i;

	for (i=0;i<SlotTypes;i++) {
		if (idx==slottypes[i].slot) {
			break;
		}
	}
	return i;
}

ieDword Interface::QuerySlot(unsigned int idx) const
{
	if (idx>=SlotTypes) {
		return 0;
	}
	return slottypes[idx].slot;
}

ieDword Interface::QuerySlotType(unsigned int idx) const
{
	if (idx>=SlotTypes) {
		return 0;
	}
	return slottypes[idx].slottype;
}

ieDword Interface::QuerySlotID(unsigned int idx) const
{
	if (idx>=SlotTypes) {
		return 0;
	}
	return slottypes[idx].slotid;
}

ieDword Interface::QuerySlottip(unsigned int idx) const
{
	if (idx>=SlotTypes) {
		return 0;
	}
	return slottypes[idx].slottip;
}

ieDword Interface::QuerySlotEffects(unsigned int idx) const
{
	if (idx>=SlotTypes) {
		return 0;
	}
	return slottypes[idx].sloteffects;
}

ieDword Interface::QuerySlotFlags(unsigned int idx) const
{
	if (idx>=SlotTypes) {
		return 0;
	}
	return slottypes[idx].slotflags;
}

const char *Interface::QuerySlotResRef(unsigned int idx) const
{
	if (idx>=SlotTypes) {
		return "";
	}
	return slottypes[idx].slotresref;
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
int Interface::CanUseItemType(int slottype, Item *item, Actor *actor, bool feedback, bool equipped) const
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

	if (!ret) {
		if (feedback) displaymsg->DisplayConstantString(STR_WRONGITEMTYPE, DMC_WHITE);
		return 0;
	}

	//this warning comes only when feedback is enabled
	if (feedback) {
		//this was, but that disabled equipping of amber earrings in PST
		//if (slotmatrix[item->ItemType]&(SLOT_QUIVER|SLOT_WEAPON|SLOT_ITEM)) {
		if (ret&(SLOT_QUIVER|SLOT_WEAPON|SLOT_ITEM)) {
			//don't ruin the return variable, it contains the usable slot bits
			int flg = 0;
			if (ret&SLOT_QUIVER) {
				if (item->GetWeaponHeader(true)) flg = 1;
			}

			if (ret&SLOT_WEAPON) {
				//melee
				if (item->GetWeaponHeader(false)) flg = 1;
				//ranged
				if (item->GetWeaponHeader(true)) flg = 1;
			}

			if (ret&SLOT_ITEM) {
				if (item->GetEquipmentHeaderNumber(0)!=0xffff) flg = 1;
			}

			if (!flg) {
				displaymsg->DisplayConstantString(STR_UNUSABLEITEM, DMC_WHITE);
				return 0;
			}
		}
	}

	return ret;
}

Label *Interface::GetMessageLabel() const
{
	ieDword WinIndex = (ieDword) -1;
	ieDword TAIndex = (ieDword) -1;

	vars->Lookup( "OtherWindow", WinIndex );
	if (( WinIndex != (ieDword) -1 ) &&
		( vars->Lookup( "MessageLabel", TAIndex ) )) {
		Window* win = GetWindow( (unsigned short) WinIndex );
		if (win) {
			Control *ctrl = win->GetControl( (unsigned short) TAIndex );
			if (ctrl && ctrl->ControlType==IE_GUI_LABEL)
				return (Label *) ctrl;
		}
	}
	return NULL;
}

TextArea *Interface::GetMessageTextArea() const
{
	ieDword WinIndex = (ieDword) -1;
	ieDword TAIndex = (ieDword) -1;

	vars->Lookup( "MessageWindow", WinIndex );
	if (( WinIndex != (ieDword) -1 ) &&
		( vars->Lookup( "MessageTextArea", TAIndex ) )) {
		Window* win = GetWindow( (unsigned short) WinIndex );
		if (win) {
			Control *ctrl = win->GetControl( (unsigned short) TAIndex );
			if (ctrl && ctrl->ControlType==IE_GUI_TEXTAREA)
				return (TextArea *) ctrl;
		}
	}
	return NULL;
}

static const char *saved_extensions[]={".are",".sto",0};
static const char *saved_extensions_last[]={".tot",".toh",0};

//returns the priority of the file to be saved
//2 - save
//1 - save last
//0 - don't save
int Interface::SavedExtension(const char *filename)
{
	const char *str=strchr(filename,'.');
	if (!str) return 0;
	int i=0;
	while(saved_extensions[i]) {
		if (!stricmp(saved_extensions[i], str) ) return 2;
		i++;
	}
	i=0;
	while(saved_extensions_last[i]) {
		if (!stricmp(saved_extensions_last[i], str) ) return 1;
		i++;
	}
	return 0;
}

static const char *protected_extensions[]={".exe",".dll",".so",0};

//returns true if file should be saved
bool Interface::ProtectedExtension(const char *filename)
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

void Interface::RemoveFromCache(const ieResRef resref, SClass_ID ClassID)
{
	char filename[_MAX_PATH];

	PathJoinExt(filename, CachePath, resref, TypeExt(ClassID));
	unlink ( filename);
}

//this function checks if the path is eligible as a cache
//if it contains a directory, or suspicious file extensions
//we bail out, because the cache will be purged regularly.
bool Interface::StupidityDetector(const char* Pt)
{
	char Path[_MAX_PATH];
	strcpy( Path, Pt );
	DirectoryIterator dir(Path);
	if (!dir) {
		print("\n**cannot open**");
		return true;
	}
	do {
		const char *name = dir.GetName();
		if (dir.IsDirectory()) {
			if (name[0] == '.') {
				if (name[1] == '\0')
					continue;
				if (name[1] == '.' && name[2] == '\0')
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

void Interface::DelTree(const char* Pt, bool onlysave)
{
	char Path[_MAX_PATH];

	if (!Pt[0]) return; //Don't delete the root filesystem :)
	strcpy( Path, Pt );
	DirectoryIterator dir(Path);
	if (!dir) {
		return;
	}
	do {
		const char *name = dir.GetName();
		if (dir.IsDirectory())
			continue;
		// FIXME: we need a more universal isHidden type method on DirectoryIterator
		if (name[0] == '.')
			continue;
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
	RedrawControls("Progress", percent);
	RedrawAll();
	DrawWindows();
	video->SwapBuffers();
}

void Interface::ReleaseDraggedItem()
{
	DraggedItem=NULL; //shouldn't free this
	video->SetCursor (NULL, VID_CUR_DRAG);
}

void Interface::DragItem(CREItem *item, const ieResRef Picture)
{
	//We should drop the dragged item and pick this up,
	//we shouldn't have a valid DraggedItem at this point.
	//Anyway, if there is still a dragged item, it will be destroyed.
	if (DraggedItem) {
		Log(WARNING, "Core", "Forgot to call ReleaseDraggedItem when leaving inventory (item destroyed)!");
		delete DraggedItem;
	}
	DraggedItem = item;
	if (video) {
		Sprite2D* DraggedCursor = NULL;
		if (item) {
			DraggedCursor = gamedata->GetBAMSprite(Picture, 0, 0);
			if (!DraggedCursor) {
				// use any / the smaller icon if the dragging one is unavailable
				DraggedCursor = gamedata->GetBAMSprite(Picture, -1, 0);
			}
		}
		video->SetCursor (DraggedCursor, VID_CUR_DRAG);
		if (DraggedCursor) DraggedCursor->release();
	}
}

void Interface::SetDraggedPortrait(int dp, int idx)
{
	if (idx<0) idx=14;
	DraggedPortrait = dp;
	if (dp) {
		video->SetCursor(Cursors[idx], VID_CUR_DRAG);
	} else {
		video->SetCursor(NULL, VID_CUR_DRAG);
	}
}

bool Interface::ReadItemTable(const ieResRef TableName, const char * Prefix)
{
	ieResRef ItemName;
	int i,j;

	AutoTable tab(TableName);
	if (!tab) {
		return false;
	}
	i=tab->GetRowCount();
	for(j=0;j<i;j++) {
		if (Prefix) {
			snprintf(ItemName,sizeof(ItemName),"%s%02d",Prefix, j+1);
		} else {
			strnlwrcpy(ItemName,tab->GetRowName(j), 8);
		}
		//Variable elements are free'd, so we have to use malloc
		//well, not anymore, we can use ReleaseFunction
		int l=tab->GetColumnCount(j);
		if (l<1) continue;
		int cl = atoi(tab->GetColumnName(0));
		ItemList *itemlist = new ItemList(l, cl);
		for(int k=0;k<l;k++) {
			strnlwrcpy(itemlist->ResRefs[k],tab->QueryField(j,k), 8);
		}
		RtRows->SetAt(ItemName, (void*)itemlist);
	}
	return true;
}

bool Interface::ReadRandomItems()
{
	ieResRef RtResRef;
	int i;

	ieDword difflev=0; //rt norm or rt fury
	vars->Lookup("Nightmare Mode", difflev);
	if (RtRows) {
		RtRows->RemoveAll(ReleaseItemList);
	}
	else {
		RtRows=new Variables(10, 17); //block size, hash table size
		if (!RtRows) {
			return false;
		}
		RtRows->SetType( GEM_VARIABLES_POINTER );
	}
	AutoTable tab("randitem");
	if (!tab) {
		return false;
	}
	if (difflev>=tab->GetColumnCount()) {
		difflev = tab->GetColumnCount()-1;
	}

	//the gold item
	strnlwrcpy( GoldResRef, tab->QueryField((unsigned int) 0,(unsigned int) 0), 8);
	if ( GoldResRef[0]=='*' ) {
		return false;
	}
	strnlwrcpy( RtResRef, tab->QueryField( 1, difflev ), 8);
	i=atoi( RtResRef );
	if (i<1) {
		ReadItemTable( RtResRef, 0 ); //reading the table itself
		return true;
	}
	if (i>5) {
		i=5;
	}
	while(i--) {
		strnlwrcpy( RtResRef, tab->QueryField(2+i,difflev), 8);
		ReadItemTable( RtResRef,tab->GetRowName(2+i) );
	}
	return true;
}

CREItem *Interface::ReadItem(DataStream *str)
{
	CREItem *itm = new CREItem();
	if (ReadItem(str, itm)) return itm;
	delete itm;
	return NULL;
}

CREItem *Interface::ReadItem(DataStream *str, CREItem *itm)
{
	str->ReadResRef( itm->ItemResRef );
	str->ReadWord( &itm->Expired );
	str->ReadWord( &itm->Usages[0] );
	str->ReadWord( &itm->Usages[1] );
	str->ReadWord( &itm->Usages[2] );
	str->ReadDword( &itm->Flags );
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
	if (GF_NO_UNDROPPABLE) {
		item->Flags &= ~IE_INV_ITEM_UNDROPPABLE;
	}

	Item *itm = gamedata->GetItem(item->ItemResRef, true);
	if (itm) {
		//set charge counters for non-rechargeable items if their charge is zero
		//set charge counters for items not using charges to one
		for (int i = 0; i < CHARGE_COUNTERS; i++) {
			ITMExtHeader *h = itm->GetExtHeader(i);
			if (h) {
				if (item->Usages[i] == 0) {
					if (!(h->RechargeFlags&IE_ITEM_RECHARGE)) {
						//HACK: the original (bg2) allows for 0 charged gems
						if (h->Charges) {
							item->Usages[i] = h->Charges;
						} else {
							item->Usages[i] = 1;
						}
					}
				} else if (h->Charges == 0) {
					item->Usages[i] = 1;
				}
			} else {
				item->Usages[i] = 0;
			}
		}

		//simply adding the item flags to the slot
		item->Flags |= (itm->Flags<<8);
		//some slot flags might be affected by the item flags
		if (!(item->Flags & IE_INV_ITEM_CRITICAL)) {
			item->Flags |= IE_INV_ITEM_DESTRUCTIBLE;
		}
		//this is for converting IWD items magic flag
		if (MagicBit) {
			if (item->Flags&IE_INV_ITEM_UNDROPPABLE) {
				item->Flags|=IE_INV_ITEM_MAGICAL;
				item->Flags&=~IE_INV_ITEM_UNDROPPABLE;
			}
		}

		if (!(item->Flags & IE_INV_ITEM_MOVABLE)) {
			item->Flags |= IE_INV_ITEM_UNDROPPABLE;
		}

		if (item->Flags & IE_INV_ITEM_STOLEN2) {
			item->Flags |= IE_INV_ITEM_STOLEN;
		}

		//auto identify basic items
		if (!itm->LoreToID) {
			item->Flags |= IE_INV_ITEM_IDENTIFIED;
		}

		//if item is stacked mark it as so
		if (itm->MaxStackAmount) {
			item->Flags |= IE_INV_ITEM_STACKED;
		}

		item->MaxStackAmount = itm->MaxStackAmount;

		gamedata->FreeItem(itm, item->ItemResRef, false);
	}
}

#define MAX_LOOP 10

//This function generates random items based on the randitem.2da file
//there could be a loop, but we don't want to freeze, so there is a limit
bool Interface::ResolveRandomItem(CREItem *itm)
{
	if (!RtRows) return true;
	for(int loop=0;loop<MAX_LOOP;loop++) {
		int i,j,k;
		char *endptr;
		ieResRef NewItem;

		void* lookup;
		if ( !RtRows->Lookup( itm->ItemResRef, lookup ) ) {
			if (!gamedata->Exists(itm->ItemResRef, IE_ITM_CLASS_ID)) {
				Log(ERROR, "Interface", "Nonexistent random item (bad table entry) detected: %s", itm->ItemResRef);
				return false;
			}
			return true;
		}
		ItemList *itemlist = (ItemList*)lookup;
		if (itemlist->WeightOdds) {
			//instead of 1d19 we calculate with 2d10 (which also has 19 possible values)
			i=Roll(2,(itemlist->Count+1)/2,-2);
		} else {
			i=Roll(1,itemlist->Count,-1);
		}
		strnlwrcpy( NewItem, itemlist->ResRefs[i], 8);
		char *p=(char *) strchr(NewItem,'*');
		if (p) {
			*p=0; //doing this so endptr is ok
			k=strtol(p+1,NULL,10);
		} else {
			k=1;
		}
		j=strtol(NewItem,&endptr,10);
		if (j<1) {
			j=1;
		}
		if (*endptr) {
			strnlwrcpy(itm->ItemResRef, NewItem, 8);
		} else {
			strnlwrcpy(itm->ItemResRef, GoldResRef, 8);
		}
		if ( !memcmp( itm->ItemResRef,"no_drop",8 ) ) {
			itm->ItemResRef[0]=0;
		}
		if (!itm->ItemResRef[0]) {
			return false;
		}
		itm->Usages[0]=(ieWord) Roll(j,k,0);
	}
	Log(ERROR, "Interface", "Loop detected while generating random item:%s",
		itm->ItemResRef);
	return false;
}

//now that we store spell name in spl, i guess, we shouldn't pass 'ieResRef name'
//these functions are needed because Win32 doesn't allow freeing memory from
//another dll. So we allocate all commonly used memories from core
ITMExtHeader *Interface::GetITMExt(int count)
{
	return new ITMExtHeader[count];
}

SPLExtHeader *Interface::GetSPLExt(int count)
{
	return new SPLExtHeader[count];
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
	memset(fx,0,sizeof(Effect));
	fx->Opcode=opcode;
	return fx;
}

Effect *Interface::GetFeatures(int count)
{
	return new Effect[count];
}

WorldMapArray *Interface::NewWorldMapArray(int count)
{
	return new WorldMapArray(count);
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

Store *Interface::SetCurrentStore(const ieResRef resname, ieDword owner)
{
	if (CurrentStore) {
		if (!strnicmp(CurrentStore->Name, resname, 8)) {
			return CurrentStore;
		}

		//not simply delete the old store, but save it
		CloseCurrentStore();
	}

	CurrentStore = gamedata->GetStore(resname);
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

int Interface::GetMouseScrollSpeed() {
	return mousescrollspd;
}

ieStrRef Interface::GetRumour(const ieResRef dlgref)
{
	PluginHolder<DialogMgr> dm(IE_DLG_CLASS_ID);
	dm->Open(gamedata->GetResource(dlgref, IE_DLG_CLASS_ID));
	Dialog *dlg = dm->GetDialog();

	if (!dlg) {
		Log(ERROR, "Interface", "Cannot load dialog: %s", dlgref);
		return (ieStrRef) -1;
	}
	Scriptable *pc=game->GetPC( game->GetSelectedPCSingle(), false );

	ieStrRef ret = (ieStrRef) -1;
	int i = dlg->FindRandomState( pc );
	if (i>=0 ) {
		ret = dlg->GetState( i )->StrRef;
	}
	delete dlg;
	return ret;
}

//plays stock sound listed in defsound.2da
void Interface::PlaySound(int index)
{
	if (index<=DSCount) {
		AudioDriver->Play(DefSound[index]);
	}
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
	if (game->selected.size()) {
		return game->selected[0];
	}
	return NULL;
}

//this is used only for the console
Sprite2D *Interface::GetCursorSprite()
{
	Sprite2D *spr = gamedata->GetBAMSprite(CursorBam, 0, 0);
	if (spr)
	{
		if(HasFeature(GF_OVERRIDE_CURSORPOS))
		{
			spr->XPos=1;
			spr->YPos=spr->Height-1;
		}
	}
	return spr;
}

Sprite2D *Interface::GetScrollCursorSprite(int frameNum, int spriteNum)
{
	return gamedata->GetBAMSprite(ScrollCursorBam, frameNum, spriteNum, true);
}

/* we should return -1 if it isn't gold, otherwise return the gold value */
int Interface::CanMoveItem(const CREItem *item) const
{
	//This is an inventory slot, switch to IE_ITEM_* if you use Item
	if (!HasFeature(GF_NO_DROP_CAN_MOVE) ) {
		if (item->Flags & IE_INV_ITEM_UNDROPPABLE)
			return 0;
	}
	//not gold, we allow only one single coin ResRef, this is good
	//for all of the original games
	if (strnicmp(item->ItemResRef, GoldResRef, 8 ) )
		return -1;
	//gold, returns the gold value (stack size)
	return item->Usages[0];
}

// dealing with applying effects
void Interface::ApplySpell(const ieResRef resname, Actor *actor, Scriptable *caster, int level)
{
	Spell *spell = gamedata->GetSpell(resname);
	if (!spell) {
		return;
	}

	int header = spell->GetHeaderIndexFromLevel(level);
	EffectQueue *fxqueue = spell->GetEffectBlock(caster, actor->Pos, header, level);

	ApplyEffectQueue(fxqueue, actor, caster, actor->Pos);
	delete fxqueue;
}

void Interface::ApplySpellPoint(const ieResRef resname, Map* area, const Point &pos, Scriptable *caster, int level)
{
	Spell *spell = gamedata->GetSpell(resname);
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
int Interface::ApplyEffect(Effect *effect, Actor *actor, Scriptable *caster)
{
	if (!effect) {
		return 0;
	}

	EffectQueue *fxqueue = new EffectQueue();
	//AddEffect now copies the fx data, please delete your effect reference
	//if you created it. (Don't delete cached references)
	fxqueue->AddEffect( effect );
	int res = ApplyEffectQueue(fxqueue, actor, caster);
	delete fxqueue;
	return res;
}

int Interface::ApplyEffectQueue(EffectQueue *fxqueue, Actor *actor, Scriptable *caster)
{
	Point p;
	p.empty(); //the effect should have all its coordinates already set
	return ApplyEffectQueue(fxqueue, actor, caster, p);
}

//FIXME: AddAllEffects will directly apply the effects outside of the mechanisms of Actor::RefreshEffects
//This means, pcf functions may not be executed when the effect is first applied
//Adding this new effect block via RefreshEffects is possible, but that might apply existing effects twice

int Interface::ApplyEffectQueue(EffectQueue *fxqueue, Actor *actor, Scriptable *caster, Point p)
{
	int res = fxqueue->CheckImmunity ( actor );
	if (res) {
		if (res == -1 ) {
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

Effect *Interface::GetEffect(const ieResRef resname, int level, const Point &p)
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
	effect->PosX=p.x;
	effect->PosY=p.y;
	return effect;
}

// dealing with saved games
int Interface::SwapoutArea(Map *map)
{
	//refuse to save ambush areas, for example
	if (map->AreaFlags & AF_NOSAVE) {
		Log(DEBUG, "Core", "Not saving area %s",
			map->GetScriptName());
		RemoveFromCache(map->GetScriptName(), IE_ARE_CLASS_ID);
		return 0;
	}

	PluginHolder<MapMgr> mm(IE_ARE_CLASS_ID);
	if (mm == NULL) {
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

	PathJoin( Path, GamePath, GameCharactersPath, NULL );
	if (!actor) {
		return -1;
	}
	PluginHolder<ActorMgr> gm(IE_CRE_CLASS_ID);
	if (gm == NULL) {
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
	PluginHolder<SaveGameMgr> gm(IE_GAM_CLASS_ID);
	if (gm == NULL) {
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
	PluginHolder<WorldMapMgr> wmm(IE_WMP_CLASS_ID);
	if (wmm == NULL) {
		return -1;
	}

	if (WorldMapName[1][0]) {
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

int Interface::CompressSave(const char *folder)
{
	FileStream str;

	str.Create( folder, GameNameResRef, IE_SAV_CLASS_ID );
	DirectoryIterator dir(CachePath);
	if (!dir) {
		return -1;
	}
	PluginHolder<ArchiveImporter> ai(IE_SAV_CLASS_ID);
	ai->CreateArchive( &str);

	//.tot and .toh should be saved last, because they are updated when an .are is saved
	int priority=2;
	while(priority) {
		do {
			const char *name = dir.GetName();
			if (dir.IsDirectory())
				continue;
			if (name[0] == '.')
				continue;
			if (SavedExtension(name)==priority) {
				char dtmp[_MAX_PATH];
				dir.GetFullPath(dtmp);
				FileStream fs;
				fs.Open(dtmp);
				ai->AddToSaveGame(&str, &fs);
			}
		} while (++dir);
		//reopen list for the second round
		priority--;
		if (priority>0) {
			dir.Rewind();
		}
	}
	return 0;
}

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
		bonus += strmodex[column*101+ex];
	}

	return strmod[column*(MaximumAbility+1)+value] + bonus;
}

//The maze columns are used only in the maze spell, no need to restrict them further
int Interface::GetIntelligenceBonus(int column, int value) const
{
	//learn spell, max spell level, max spell number on level, maze duration dice, maze duration dice size
	if (column<0 || column>4) return -9999;

	return intmod[column*(MaximumAbility+1)+value];
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

	return dexmod[column*(MaximumAbility+1)+value];
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

	return conmod[column*(MaximumAbility+1)+value];
}

int Interface::GetCharismaBonus(int column, int /*value*/) const
{
	// store price reduction
	if (column<0 || column>(MaximumAbility-1))
		return -9999;

	return chrmod[column];
}

int Interface::GetLoreBonus(int column, int value) const
{
	//no lorebon in iwd2 - lore is a skill
	if (HasFeature(GF_3ED_RULES)) return 0;

	if (column<0 || column>0)
		return -9999;

	return lorebon[value];
}

int Interface::GetWisdomBonus(int column, int value) const
{
	//no wismod in iwd2
	if (HasFeature(GF_3ED_RULES)) {
		return value/2-5;
	}

	if (!HasFeature(GF_WISDOM_BONUS)) return 0;

	// xp bonus
	if (column<0 || column>0)
		return -9999;

	return wisbon[value];
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

PauseSetting Interface::TogglePause()
{
	GameControl *gc = GetGameControl();
	if (!gc) return PAUSE_OFF;
	PauseSetting pause = (PauseSetting)(~gc->GetDialogueFlags()&DF_FREEZE_SCRIPTS);
	if (SetPause(pause)) return pause;
	return (PauseSetting)(gc->GetDialogueFlags()&DF_FREEZE_SCRIPTS);
}

bool Interface::SetPause(PauseSetting pause, int flags)
{
	GameControl *gc = GetGameControl();

	//don't allow soft pause in cutscenes and dialog
	if (!(flags&PF_FORCED) && InCutSceneMode()) gc = NULL;

	if (gc && ((bool)(gc->GetDialogueFlags()&DF_FREEZE_SCRIPTS) != (bool)pause)) { // already paused
		int strref;
		if (pause) {
			strref = STR_PAUSED;
			gc->SetDialogueFlags(DF_FREEZE_SCRIPTS, BM_OR);
		} else {
			strref = STR_UNPAUSED;
			gc->SetDialogueFlags(DF_FREEZE_SCRIPTS, BM_NAND);
		}
		if (!(flags&PF_QUIET) ) {
			if (pause) gc->SetDisplayText(strref, 0); // time 0 = removed instantly on unpause (for pst)
			displaymsg->DisplayConstantString(strref, DMC_RED);
		}
		return true;
	}
	return false;
}

bool Interface::Autopause(ieDword flag, Scriptable* target)
{
	ieDword autopause_flags = 0;
	vars->Lookup("Auto Pause State", autopause_flags);

	if ((autopause_flags & (1<<flag))) {
		if (SetPause(PAUSE_ON, PF_QUIET)) {
			displaymsg->DisplayConstantString(STR_AP_UNUSABLE+flag, DMC_RED);

			ieDword autopause_center = 0;
			vars->Lookup("Auto Pause Center", autopause_center);
			if (autopause_center && target) {
				Point screenPos = target->Pos;
				core->GetVideoDriver()->ConvertToScreen(screenPos.x, screenPos.y);
				GetGameControl()->Center(screenPos.x, screenPos.y);
				if (target->Type == ST_ACTOR && ((Actor *)target)->GetStat(IE_EA) < EA_GOODCUTOFF) {
					core->GetGame()->SelectActor((Actor *)target, true, SELECT_REPLACE);
				}
			}
			return true;
		}
	}
	return false;
}

void Interface::RegisterOpcodes(int count, const EffectDesc *opcodes)
{
	EffectQueue_RegisterOpcodes(count, opcodes);
}

void Interface::SetInfoTextColor(const Color &color)
{
	if (InfoTextPalette) {
		gamedata->FreePalette(InfoTextPalette);
	}
	InfoTextPalette = new Palette(color, ColorBlack);
}

//todo row?
void Interface::GetResRefFrom2DA(const ieResRef resref, ieResRef resource1, ieResRef resource2, ieResRef resource3)
{
	if (!resource1) {
		return;
	}
	resource1[0]=0;
	if (resource2) {
		resource2[0]=0;
	}
	if (resource3) {
		resource3[0]=0;
	}
	AutoTable tab(resref);
	if (tab) {
		unsigned int cols = tab->GetColumnCount();
		unsigned int row = (unsigned int) Roll(1,tab->GetRowCount(),-1);
		strnuprcpy(resource1, tab->QueryField(row,0), 8);
		if (resource2 && cols>1)
			strnuprcpy(resource2, tab->QueryField(row,1), 8);
		if (resource3 && cols>2)
			strnuprcpy(resource3, tab->QueryField(row,2), 8);
	}
}

ieDword *Interface::GetListFrom2DAInternal(const ieResRef resref)
{
	ieDword *ret;

	AutoTable tab(resref);
	if (tab) {
		ieDword cnt = tab->GetRowCount();
		ret = (ieDword *) malloc((1+cnt)*sizeof(ieDword));
		ret[0]=cnt;
		while(cnt) {
			ret[cnt]=strtol(tab->QueryField(cnt-1, 0),NULL, 0);
			cnt--;
		}
		return ret;
	}
	ret = (ieDword *) malloc(sizeof(ieDword));
	ret[0]=0;
	return ret;
}

ieDword* Interface::GetListFrom2DA(const ieResRef tablename)
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
	long tmp;

	if (valid_number(stat_name, tmp)) {
		return (ieDword) tmp;
	}

	int symbol = LoadSymbol( "stats" );
	Holder<SymbolMgr> sym = GetSymbol( symbol );
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
	Holder<TableMgr> mtm = gamedata->GetTable( mastertable );
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
		int checkcol = strtol(mtm->QueryField(i,1), NULL, 0);
		unsigned int readcol = strtol(mtm->QueryField(i,2), NULL, 0);
		int stat = TranslateStat(mtm->QueryField(i,0) );
		if (!(flags&1)) {
			value = actor->GetSafeStat(stat);
		}
		int table = gamedata->LoadTable( tablename );
		Holder<TableMgr> tm = gamedata->GetTable( table );
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
			ret += strtol(tm->QueryField(row, readcol), NULL, 0);
		}
	}
	return ret;
}

void Interface::WaitForDisc(int disc_number, const char* path)
{
	GetDictionary()->SetAt( "WaitForDisc", (ieDword) disc_number );

	GetGUIScriptEngine()->RunFunction( "GUICommonWindows", "OpenWaitForDiscWindow" );
	do {
		DrawWindows();
		for (size_t i=0;i<CD[disc_number-1].size();i++) {
			char name[_MAX_PATH];

			PathJoin(name, CD[disc_number-1][i].c_str(),path,NULL);
			if (file_exists (name)) {
				GetGUIScriptEngine()->RunFunction( "GUICommonWindows", "OpenWaitForDiscWindow" );
				return;
			}
		}

	} while (video->SwapBuffers() == GEM_OK);
}

// remove the extraneus EOL newline and carriage return
void Interface::StripLine(char * string, size_t size) {
	if (size >= 2 && string[size-2] == '\n') {
		string[size-2] = '\0';
	}
	if (size >= 3 && string[size-3] == '\r') {
		string[size-3] = '\0'; // remove the carriage return too
	}
}

void Interface::SetTickHook(EventHandler hook)
{
	TickHook = hook;
}

void Interface::SetNextScript(const char *script)
{
	strlcpy( NextScript, script, sizeof(NextScript) );
	QuitFlag |= QF_CHANGESCRIPT;
}

void Interface::SanityCheck(const char *ver) {
	if (strcmp(ver, VERSION_GEMRB)) {
		error("Core", "version check failed: core version %s doesn't match caller's version %s\n", VERSION_GEMRB, ver);
	}
}

}
