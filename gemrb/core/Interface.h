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

/**
 * @file Interface.h
 * Declaration of Interface class, central interconnect for various GemRB parts
 */

#ifndef INTERFACE_H
#define INTERFACE_H

#include "SClassID.h"
#include "exports.h"

#include "Audio.h" // needed for _MSC_VER and SoundHandle (everywhere)
#include "Cache.h"
#include "Callback.h"
#include "GameData.h"
#include "GlobalTimer.h"
#include "GUI/Control.h"
#include "GUI/Tooltip.h"
#include "GUI/Window.h"
#include "GUI/GUIFactory.h"
#include "FogRenderer.h"
#include "Holder.h"
#include "ImageMgr.h"
#include "InterfaceConfig.h"
#include "Resource.h"
#include "Timer.h"
#include "Variables.h"
#include "SaveGameAREExtractor.h"
#include "StringMgr.h"
#include "System/VFS.h"

#include <map>
#include <string>
#include <vector>
#include <unordered_map>

namespace GemRB {

class Actor;
class Audio;
class CREItem;
class Calendar;
class Container;
class DataFileMgr;
struct Effect;
class EffectQueue;
class EffectDesc;
class Factory;
class Font;
class Game;
class GameControl;
class GlobalTimer;
class ITMExtHeader;
class Item;
class KeyMap;
class Label;
class Map;
class MusicMgr;
class Palette;
using PaletteHolder = Holder<Palette>;
class ProjectileServer;
class SPLExtHeader;
class SaveGame;
class SaveGameIterator;
class ScriptEngine;
class ScriptedAnimation;
class Spell;
class Sprite2D;
class Store;
class SymbolMgr;
class TableMgr;
class TextArea;
class Variables;
class Video;
class WindowManager;
class WorldMap;
class WorldMapArray;

struct Symbol {
	std::shared_ptr<SymbolMgr> sm;
	ResRef symbolName;
};

struct SlotType {
	ieDword slot;
	ieDword slotType;
	ieDword slotTip;
	ieDword slotID;
	ieDword slotEffects = 100; // SLOT_EFFECT_ALIAS
	ieDword slotFlags;
	ResRef slotResRef;
};

struct DamageInfoStruct {
	ieStrRef strref;
	unsigned int resist_stat;
	unsigned int value;
	int iwd_mod_type;
	int reduction;
	// maybe also add the ac bonus and/or the DL_ constants
};

struct TimeStruct {
	const unsigned int defaultTicksPerSec = 15;
	unsigned int ai_update_time;
	unsigned int round_sec;
	unsigned int turn_sec;
	unsigned int round_size; // in ticks
	unsigned int rounds_per_turn;
	unsigned int attack_round_size;
	unsigned int hour_sec;
	unsigned int hour_size;
	unsigned int day_sec;
	unsigned int day_size;
	unsigned int fade_reset;

	int GetHour(unsigned int time) const { return (time / ai_update_time) % day_sec / hour_sec; }
	tick_t Ticks2Ms(unsigned int ticks) const { return ticks * 1000 / ai_update_time; }
};

// cache of speldesc.2da entries
struct SpellDescType {
	ResRef resref;
	ieStrRef value;
	// pst also has a SOUND_EFFECT column, but we use it from GUISTORE.py directly
};

class ItemList {
public:
	std::vector<ResRef> ResRefs;
	//if count is odd and the column titles start with 2, the random roll should be 2d((c+1)/2)-1
	bool WeightOdds;

	ItemList(std::vector<ResRef> refs, int label)
	: ResRefs(std::move(refs))
	{
		if ((ResRefs.size() & 1) && label == 2) {
			WeightOdds=true;
		} else {
			WeightOdds=false;
		}
	}
};

//quitflags
#define QF_NORMAL        0
#define QF_QUITGAME      1 // Quit current game
#define QF_EXITGAME      2 // Quit current game and then set QF_KILL
#define QF_CHANGESCRIPT  4
#define QF_LOADGAME      8
#define QF_ENTERGAME     16
#define QF_KILL			32 // Exit GemRB

//events that are called out of drawwindow
//they wait until the condition is right
#define EF_CONTROL       1        //updates the game window statuses
#define EF_SHOWMAP       2        //starts worldmap
#define EF_PORTRAIT      4        //updates portraits
#define EF_ACTION        8        //updates the actions bar
#define EF_UPDATEANIM    16       //updates avatar animation
#define EF_SEQUENCER     32       //starts sequencer/contingency creation
#define EF_IDENTIFY      64       //starts identify screen
#define EF_SELECTION     128      //selection changed
#define EF_OPENSTORE     256      //open store window
#define EF_EXPANSION     512      //upgrade game request
#define EF_CREATEMAZE    1024     //call the maze generator
#define EF_RESETTARGET   2048     //reset the mouse cursor
#define EF_TARGETMODE    4096     //update the mouse cursor
#define EF_TEXTSCREEN    8192     //start a textscreen

//autopause
enum class AUTOPAUSE : ieDword {
	UNUSABLE     = 0,
	ATTACKED     = 1,
	HIT          = 2,
	WOUNDED      = 3,
	DEAD         = 4,
	NOTARGET     = 5,
	ENDROUND     = 6,
	ENEMY        = 7,
	TRAP         = 8,
	SPELLCAST    = 9,
	GENERIC      = 10  //needed for Android stuff
};

//pause flags
#define PF_QUIET  1        //no feedback
#define PF_FORCED 2        //pause even in cutscene/dialog

/** ea relations (derivated from 2 actor's EA value) */
#define EAR_FRIEND  0
#define EAR_NEUTRAL 1
#define EAR_HOSTILE 2

/** Max size of actor's ground circle (PST) */
#define MAX_CIRCLE_SIZE  3

/** Summoning */
#define EAM_SOURCEALLY 0
#define EAM_SOURCEENEMY 1
#define EAM_ENEMY   2
#define EAM_ALLY    3
#define EAM_NEUTRAL 4
#define EAM_DEFAULT 5
//
#define STAT_CON_HP_NORMAL   0
#define STAT_CON_HP_WARRIOR  1
#define STAT_CON_HP_MIN      2
#define STAT_CON_HP_REGEN    3
#define STAT_CON_FATIGUE     4
#define STAT_CON_TNO_REGEN   4

#define STAT_DEX_REACTION    0
#define STAT_DEX_MISSILE     1
#define STAT_DEX_AC          2

#define STAT_INT_LEARN       0
#define STAT_INT_MAXLEVEL    1
#define STAT_INT_MAXNUMBER   2

//sloteffects (querysloteffect returns it)
#define SLOT_EFFECT_NONE     0
#define SLOT_EFFECT_ITEM     1 //normal equipped item
#define SLOT_EFFECT_FIST     2 //fist slot
#define SLOT_EFFECT_MAGIC    3 //magic weapon slot
#define SLOT_EFFECT_MELEE    4 //normal weapon slot
#define SLOT_EFFECT_MISSILE  5 //quiver slots
#define SLOT_EFFECT_LEFT     6 //shield (left hand) slot
#define SLOT_EFFECT_HEAD     7 //head slot
#define SLOT_EFFECT_ALIAS    100 // marker for aliased slots

enum PluginFlagsType {
	PLF_NORMAL,
	PLF_SKIP,
	PLF_DELAY
};

enum PauseSetting {
	PAUSE_OFF = 0,
	PAUSE_ON = 1
};

enum RESOURCE_DIRECTORY {
	DIRECTORY_CHR_PORTRAITS,
	DIRECTORY_CHR_SOUNDS,
	DIRECTORY_CHR_EXPORTS,
	DIRECTORY_CHR_SCRIPTS
};

enum FeedbackType {
	FT_TOHIT = 1,
	FT_COMBAT = 2,
	FT_ACTIONS = 4, // handled by Actor::CommandActor
	FT_STATES = 8,
	FT_SELECTION = 16, // handled by Actor::PlaySelectionSound
	FT_MISC = 32,
	FT_CASTING = 64,

	FT_ANY = 0xffff
};

enum DebugModeBits {
	ID_REFERENCE = 1,
	ID_CUTSCENE = 2,
	ID_VARIABLES = 4,
	ID_ACTIONS = 8,
	ID_TRIGGERS = 16,
	ID_VIEWS = 32,
	ID_WINDOWS = 64,
	ID_FONTS = 128,
	ID_TEXT = 256,
	ID_PATHFINDER = 512
};

// TODO: there is no reason why this can't be generated directly from
// the inventory button drag event using the button value as the slot id
// to get the appropriate CREItem
struct ItemDragOp : public Control::ControlDragOp {
	CREItem* item;

	explicit ItemDragOp(CREItem* item);
	
private:
	static Control dragDummy;
};

struct CFGConfigData {
	char GamePath[_MAX_PATH]{};
	char GameDataPath[_MAX_PATH]{};
	char GameOverridePath[_MAX_PATH]{};
	char GameSoundsPath[_MAX_PATH]{};
	char GameScriptsPath[_MAX_PATH]{};
	char GamePortraitsPath[_MAX_PATH]{};
	char GameCharactersPath[_MAX_PATH]{};
	char GameLanguagePath[_MAX_PATH]{};
	char GameMoviesPath[_MAX_PATH]{};
	char SavePath[_MAX_PATH]{};
	char CachePath[_MAX_PATH]{};
	std::vector<std::string> CD[MAX_CD];
	std::vector<std::string> ModPath;
	char CustomFontPath[_MAX_PATH]{};

	char GemRBPath[_MAX_PATH]{};
	char GemRBOverridePath[_MAX_PATH]{};
	char GemRBUnhardcodedPath[_MAX_PATH]{};
	char PluginsPath[_MAX_PATH]{};
	char GUIScriptsPath[_MAX_PATH]{};
	bool CaseSensitive = true;

	std::string GameName;
	std::string GameType;
	std::string Encoding = "default";

	int GamepadPointerSpeed = 10;
	bool UseSoftKeyboard = false; // TODO: reevaluate the need for this, see comments in StartTextInput
	unsigned short NumFingScroll = 2;
	unsigned short NumFingKboard = 3;
	unsigned short NumFingInfo = 2;
	int MouseFeedback = 0;

	int Width = 640;
	int Height = 480;
	int Bpp = 32;
	bool DrawFPS = false;
	int CapFPS = 0;
	bool SpriteFoW = false;
	int debugMode = 0;
	bool CheatFlag = false; /** Cheats enabled? */
	int MaxPartySize = 6;

	bool KeepCache = false;
	bool MultipleQuickSaves = false;
	// once GemRB own format is working well, this might be set to 0
	int SaveAsOriginal = 1; // if true, saves files in compatible mode
	std::string VideoDriverName = "sdl"; // consider deprecating? It's now a hidden option
	std::string AudioDriverName = "openal";
};

/**
 * @class Interface
 * Central interconnect for all GemRB parts, driving functions and utility functions possibly belonging to a better place
 */

class GEM_EXPORT Interface
{
private:
	// dirvers must be deallocated last (keep them at the top)
	// we hold onto resources (sprites etc) in Interface that must be destroyed prior to the respective driver
	std::shared_ptr<Video> video;
	std::shared_ptr<Audio> AudioDriver;

	ProjectileServer* projserv = nullptr;

	WindowManager* winmgr = nullptr;
	std::shared_ptr<GUIFactory> guifact;
	std::shared_ptr<ScriptEngine> guiscript;
	std::shared_ptr<FogRenderer> fogRenderer;
	GameControl* gamectrl = nullptr;
	SaveGameIterator *sgiterator = nullptr;
	Variables * vars;
	Variables * tokens;
	Variables * lists;
	std::shared_ptr<MusicMgr> music;
	std::vector<Symbol> symbols;
	std::shared_ptr<DataFileMgr> INIparty;
	std::shared_ptr<DataFileMgr> INIbeasts;
	std::shared_ptr<DataFileMgr> INIquests;
	std::shared_ptr<DataFileMgr> INIresdata;
	Game* game = nullptr;
	Calendar* calendar = nullptr;
	WorldMapArray* worldmap = nullptr;
	EnumBitset<GFFlags> GameFeatures;
	ResRef MainCursorsImage;
	ResRef TextCursorBam;
	ResRef ScrollCursorBam;
	ResRef GroundCircleBam[MAX_CIRCLE_SIZE];
	int GroundCircleScale[MAX_CIRCLE_SIZE]{};

	std::map<ResRef, Font*> fonts;
	ResRef ButtonFontResRef;
	ResRef MovieFontResRef;
	ResRef TextFontResRef;
	ResRef TooltipFontResRef;
	std::string DefaultWindowTitle;

	TooltipBackground* TooltipBG = nullptr;

	ResRef Palette16;
	ResRef Palette32;
	ResRef Palette256;
	std::vector<ColorPal<256>> palettes256;
	std::vector<ColorPal<32>>  palettes32;
	std::vector<ColorPal<16>>  palettes16;

	std::vector<ieDword> slotmatrix; // itemtype vs slottype
	std::vector<std::vector<int> > itemtypedata; //armor failure, critical multiplier, critical range
	std::vector<SlotType> slotTypes;
	TableMgr::index_t ItemTypes = 0;

	// Currently dragged item or NULL
	std::unique_ptr<ItemDragOp> DraggedItem;
	// Current Store
	Store* CurrentStore = nullptr;
	// Index of current container
	Container* CurrentContainer = nullptr;
	bool UseContainer = false;
	// Scrolling speed
	int mousescrollspd = 10;
	bool update_scripts = false;
	/** Next Script Name */
	std::string nextScript;

	std::deque<Timer> timers;
	KeyMap *keymap = nullptr;
	Scriptable *CutSceneRunner = nullptr;

	int MaximumAbility = 0;

public:
	const char * SystemEncoding;
	EncodingStruct TLKEncoding;
	std::shared_ptr<StringMgr> strings;
	std::shared_ptr<StringMgr> strings2;
	GlobalTimer timer;
	int QuitFlag = QF_NORMAL;
	int EventFlag = EF_CONTROL;
	Holder<SaveGame> LoadGameIndex;
	SaveGameAREExtractor saveGameAREExtractor;
	int VersionOverride = 0;
	size_t SlotTypes = 0; // this is the same as the inventory size
	ResRef GlobalScript = "BALDUR";
	ResRef WorldMapName[2] = { "WORLDMAP", "" };

	std::vector<Holder<Sprite2D> > Cursors;
	Holder<Sprite2D> GroundCircles[MAX_CIRCLE_SIZE][6] {};
	std::vector<ieVariable> musiclist;
	std::multimap<ieDword, DamageInfoStruct> DamageInfoMap;
	TimeStruct Time{};
public:
	Interface() noexcept;
	~Interface() noexcept;
	
	Interface(const Interface&) = delete;
	
	int Init(const InterfaceConfig* config);
	//TODO: Core Methods in Interface Class
	void SetFeature(GFFlags flag);
	void ClearFeature(GFFlags flag);
	bool HasFeature(GFFlags flag) const;
	bool IsAvailable(SClass_ID filetype) const;
	const char * TypeExt(SClass_ID type) const;
	ProjectileServer* GetProjectileServer() const noexcept;
	Video * GetVideoDriver() const;
	FogRenderer& GetFogRenderer();
	/* create or change a custom string */
	ieStrRef UpdateString(ieStrRef strref, const String& text) const;
	/* returns a newly created string */
	String GetString(ieStrRef strref, STRING_FLAGS options = STRING_FLAGS::NONE) const;
	std::string GetMBString(ieStrRef strref, STRING_FLAGS options = STRING_FLAGS::NONE) const;
	/** returns a gradient set */
	const ColorPal<16>& GetPalette16(uint8_t idx) const { return (idx >= palettes16.size()) ? palettes16[0] : palettes16[idx]; }
	const ColorPal<32>& GetPalette32(uint8_t idx) const { return (idx >= palettes32.size()) ? palettes32[0] : palettes32[idx]; }
	const ColorPal<256>& GetPalette256(uint8_t idx) const { return (idx >= palettes256.size()) ? palettes256[0] : palettes256[idx]; }
	/** Returns a preloaded Font */
	Font* GetFont(const ResRef&) const;
	Font* GetTextFont() const;
	/** Returns the button font */
	Font * GetButtonFont() const;
	/** Get GUI Script Manager */
	ScriptEngine * GetGUIScriptEngine() const;
	/** core for summoning creatures, returns the last created Actor
	may apply a single fx on the summoned creature normally an unsummon effect */
	Actor *SummonCreature(const ResRef& resource, const ResRef& animRes, Scriptable *Owner, const Actor *target, const Point &position, int eamod, int level, Effect *fx, bool sexmod = true);
	/** Get the Window Manager */
	WindowManager *GetWindowManager() const { return winmgr; };
	/** Loads a Window in the Window Manager */
	Window* LoadWindow(ScriptingId WindowID, const ScriptingGroup_t& ref, Window::WindowPosition = Window::PosCentered);
	/** Creates a Window in the Window Manager */
#undef CreateWindow // Win32 might define this, so nix it
	Window* CreateWindow(unsigned short WindowID, const Region&);
	void ToggleViewsVisible(bool visible, const ScriptingGroup_t& group);
	void ToggleViewsEnabled(bool enabled, const ScriptingGroup_t& group) const;

	Tooltip CreateTooltip() const;
	/** returns the label which should receive game messages (overrides messagetextarea) */
	Label *GetMessageLabel() const;
	/** returns the textarea of the main game screen */
	TextArea *GetMessageTextArea() const;
	void SetFeedbackLevel(int level);
	/** returns true if the passed feedback type is enabled */
	bool HasFeedback(int type) const;
	/** Get the SaveGameIterator */
	SaveGameIterator * GetSaveGameIterator() const;
	/** Get the Variables Dictionary */
	Variables * GetDictionary() const;
	/** Get the Token Dictionary */
	Variables * GetTokenDictionary() const;
	/** Get the Music Manager */
	MusicMgr * GetMusicMgr() const;
	/** Loads an IDS Table, returns -1 on error or the Symbol Table Index on success */
	int LoadSymbol(const ResRef&);
	/** Gets the index of a loaded Symbol Table, returns -1 on error */
	int GetSymbolIndex(const ResRef&) const;
	/** Gets a Loaded Symbol Table by its index, returns NULL on error */
	std::shared_ptr<SymbolMgr> GetSymbol(unsigned int index) const;
	/** Frees a Loaded Symbol Table, returns false on error, true on success */
	bool DelSymbol(unsigned int index);
	/** Plays a Movie */
	int PlayMovie(const ResRef& movieRef);
	/** Generates traditional random number xdy+z */
	int Roll(int dice, int size, int add) const;
	/** Loads a Game Compiled Script */
	int LoadScript(const char * ResRef);
	/** store the scriptable running the cutscene */
	void SetCutSceneRunner(Scriptable *runner);
	Scriptable *GetCutSceneRunner() const { return CutSceneRunner; };
	/** Enables/Disables the CutScene Mode */
	void SetCutSceneMode(bool active);
	/** returns true if in cutscene mode */
	bool InCutSceneMode() const;
	/** Updates the Game Script Engine State */
	bool GSUpdate(bool update);
	/** Get the Party INI Interpreter */
	DataFileMgr * GetPartyINI() const
	{
		return INIparty.get();
	}
	DataFileMgr * GetBeastsINI() const
	{
		return INIbeasts.get();
	}
	DataFileMgr * GetQuestsINI() const
	{
		return INIquests.get();
	}
	DataFileMgr * GetResDataINI() const
	{
		return INIresdata.get();
	}
	/** Gets the Game class */
	Game * GetGame() const
	{
		return game;
	}
	/** Gets the Calendar class */
	Calendar * GetCalendar() const
	{
		return calendar;
	}

	/** Gets the KeyMap class */
	KeyMap * GetKeyMap() const
	{
		return keymap;
	}

	/** Gets the WorldMap class, returns the current worldmap or the first worldmap containing the area*/
	WorldMap* GetWorldMap() const;
	WorldMap* GetWorldMap(const ResRef& area) const;
	GameControl *GetGameControl() const { return game ? gamectrl : nullptr; }
	/** if backtomain is not null then goes back to main screen */
	void QuitGame(int backtomain);
	/** sets up load game */
	void SetupLoadGame(Holder<SaveGame> save, int ver_override);
	/** load saved game by index (-1 is default), ver_override is an optional parameter
	to override the saved game's version */
	void LoadGame(SaveGame *save, int ver_override);
	/** reloads the world map from a resource file */
	void UpdateWorldMap(const ResRef& wmResRef);
	/** fix changes in global script/worldmap*/
	void UpdateMasterScript();

	DirectoryIterator GetResourceDirectory(RESOURCE_DIRECTORY) const;

	size_t GetInventorySize() const { return SlotTypes-1; }
	ieDword FindSlot(unsigned int idx) const;
	ieDword QuerySlot(unsigned int idx) const;
	ieDword QuerySlotType(unsigned int idx) const;
	ieDword QuerySlottip(unsigned int idx) const;
	ieDword QuerySlotID(unsigned int idx) const;
	ieDword QuerySlotFlags(unsigned int idx) const;
	ieDword QuerySlotEffects(unsigned int idx) const;
	const ResRef& QuerySlotResRef(unsigned int idx) const;
	int GetArmorFailure(unsigned int itemtype) const;
	int GetShieldFailure(unsigned int itemtype) const;
	int GetArmorPenalty(unsigned int itemtype) const;
	int GetShieldPenalty(unsigned int itemtype) const;
	int GetCriticalMultiplier(unsigned int itemtype) const;
	int GetCriticalRange(unsigned int itemtype) const;
	/*returns true if an itemtype is acceptable for a slottype, also checks the usability flags */
	int CanUseItemType(int slottype, const Item *item, const Actor *actor = nullptr, bool feedback = false, bool equipped = false) const;
	int CheckItemType(const Item* item, int slotType) const;
	/*removes single file from cache*/
	void RemoveFromCache(const ResRef& resref, SClass_ID SClassID) const;
	/*removes all files from directory*/
	void DelTree(const char *path, bool onlysaved) const;
	/*returns 0,1,2 based on how the file should be saved */
	int SavedExtension(const char *filename) const;
	/*returns true if the file should never be deleted accidentally */
	bool ProtectedExtension(const char *filename) const;
	/*returns true if the directory path isn't good as a Cache */
	bool StupidityDetector(const char* Pt) const;
	bool InDebugMode(int mode) const { return config.debugMode & mode; };
	void SetDebugMode(int mode) { config.debugMode = mode; };
	/*handles the load screen*/
	void LoadProgress(int percent);

	void DragItem(CREItem* item, const ResRef& Picture);
	const ItemDragOp* GetDraggedItem() const { return DraggedItem.get(); }
	/* use this only when the dragged item is dropped */
	void ReleaseDraggedItem();
	CREItem *ReadItem(DataStream *str) const;
	CREItem *ReadItem(DataStream *str, CREItem *itm) const;
	void SanitizeItem(CREItem *item) const;
	bool ResolveRandomItem(CREItem *itm) const;
	ieStrRef GetRumour(const ResRef& resname);
	Container *GetCurrentContainer();
	int CloseCurrentContainer();
	void SetCurrentContainer(const Actor *actor, Container *arg, bool flag = false);
	Store *GetCurrentStore();
	void CloseCurrentStore();
	Store *SetCurrentStore(const ResRef &resName, ieDword owner);
	void SetMouseScrollSpeed(int speed);
	int GetMouseScrollSpeed() const;

	/** plays stock gui sound referenced by index */
	Holder<SoundHandle> PlaySound(size_t idx, unsigned int channel);
	/** returns the first selected PC, if forced is set, then it returns
	first PC if none was selected */
	Actor *GetFirstSelectedPC(bool forced);
	Actor *GetFirstSelectedActor();
	/** is an area loaded? (prefer Game::GetCurrentArea if including Game.h makes sense) */
	bool HasCurrentArea() const;
	/** returns a cursor sprite (not cached) */
	Holder<Sprite2D> GetCursorSprite();
	/** returns a scroll cursor sprite */
	Holder<Sprite2D> GetScrollCursorSprite(orient_t orient, int spriteNum) const;
	/** returns 0 for unmovable, -1 for movable items, otherwise it
	returns gold value! */
	int CanMoveItem(const CREItem *item) const;
	int GetRareSelectSoundCount() const;
	int GetMaximumAbility() const;
	int GetStrengthBonus(int column, int value, int ex) const;
	int GetIntelligenceBonus(int column, int value) const;
	int GetDexterityBonus(int column, int value) const;
	int GetConstitutionBonus(int column, int value) const;
	int GetCharismaBonus(int column, int value) const;
	int GetLoreBonus(int column, int value) const;
	int GetWisdomBonus(int column, int value) const;

	/** applies the spell on the target */
	void ApplySpell(const ResRef& spellRef, Actor *target, Scriptable *caster, int level) const;
	/** applies the spell on the area or on a scriptable object */
	void ApplySpellPoint(const ResRef& spellRef, Map *area, const Point &pos, Scriptable *caster, int level) const;
	/** applies a single effect on the target */
	int ApplyEffect(Effect *fx, Actor *target, Scriptable *caster) const;
	/** applies an effect queue on the target */
	int ApplyEffectQueue(EffectQueue *fxqueue, Actor *actor, Scriptable *caster) const;
	int ApplyEffectQueue(EffectQueue *fxqueue, Actor *actor, Scriptable *caster, Point p) const;
	Effect *GetEffect(const ResRef& resname, int level, const Point &p);
	/** dumps an area object to the cache */
	int SwapoutArea(Map *map) const;
	/** saves (exports a character to the characters folder */
	int WriteCharacter(StringView name, const Actor *actor);
	/** saves the game object to the destination folder */
	int WriteGame(const char *folder);
	/** saves the worldmap object to the destination folder */
	int WriteWorldMap(const char *folder);
	/** saves the .are and .sto files to the destination folder */
	int CompressSave(const char *folder, bool overrideRunning);
	/** toggles the pause. returns either PAUSE_ON or PAUSE_OFF to reflect the script state after toggling. */
	PauseSetting TogglePause() const;
	/** returns true the passed pause setting was applied. false otherwise. */
	bool SetPause(PauseSetting pause, int flags = 0) const;
	/** receives an autopause reason, returns true if autopause was accepted and successful */
	bool Autopause(AUTOPAUSE flag, Scriptable *target) const;
	/** registers engine opcodes */
	void RegisterOpcodes(int count, const EffectDesc *opcodes) const;
	/** reads a list of resrefs into an array, returns array size */
	bool ReadResRefTable(const ResRef& tablename, std::vector<ResRef>& data);
	/** Returns up to 3 resources from resref, choosing rows randomly
	unwanted return variables could be omitted */
	void GetResRefFrom2DA(const ResRef& resref, ResRef& resource1, ResRef& resource2, ResRef& resource3) const;
	/** returns a numeric list read from a 2da. The 0th element is the number of elements in the list */
	std::vector<ieDword>* GetListFrom2DA(const ResRef& resref);
	/** translates a stat symbol to numeric value */
	ieDword TranslateStat(const std::string& statName);
	/** resolves a stat bonus based on multiple stats */
	int ResolveStatBonus(const Actor* actor, const ResRef& tableName, ieDword flags = 0, int value = 0);
	/** Opens CD prompt window and waits for the specified disc */
	void WaitForDisc(int disc_number, const char* path) const;
	/** Returns the music playlist corresponding to the provided type */
	const ieVariable& GetMusicPlaylist(size_t SongType) const;
	void DisableMusicPlaylist(size_t SongType);
	/** Returns the DeathVarFormat of the day */
	static const char *GetDeathVarFormat();
	/** Saves config variables to a file */
	bool SaveConfig();
private:
	int LoadSprites();
	int LoadFonts();
	bool LoadGemRBINI();
	/** Load the encoding table selected in gemrb.cfg */
	bool LoadEncoding();

	template<int SIZE>
	bool LoadPalette(const ResRef& resref, std::vector<ColorPal<SIZE>>& palettes) const
	{
		static_assert(SIZE == 16 || SIZE == 32 || SIZE == 256, "invalid palette size");

		ResourceHolder<ImageMgr> palim = GetResourceHolder<ImageMgr>(resref);
		if (palim) {
			auto image = palim->GetSprite2D();
			int height = image->Frame.h;
			palettes.resize(height);
			Region clip(0, 0, SIZE, height);
			auto it = image->GetIterator(IPixelIterator::Direction::Forward, IPixelIterator::Direction::Forward, clip);
			auto end = it.end(it);
			for (; it != end; ++it) {
				const Point& p = it.Position();
				palettes[p.y][p.x] = it.ReadRGBA();
			}
			return true;
		}
		return false;
	}

	bool InitializeVarsWithINI(const char * iniFileName);
	bool InitItemTypes();
	bool ReadRandomItems();
	bool ReadItemTable(const ResRef& item, const char *Prefix);
	bool ReadMusicTable(const ResRef& name, int col);
	bool ReadDamageTypeTable();
	bool ReadGameTimeTable();
	bool ReadSoundChannelsTable() const;

	/** handles the QuitFlag bits (main loop events) */
	void HandleFlags() noexcept;
	/** handles the EventFlag bits (conditional events) */
	void HandleEvents();
	/** handles hardcoded gui behaviour */
	void HandleGUIBehaviour(GameControl*);
	/** Creates a game control, closes all other windows */
	GameControl* StartGameControl();
	/** Executes everything (non graphical) in the main game loop */
	void GameLoop(void);
	/** the internal (without cache) part of GetListFrom2DA */
	std::vector<ieDword>* GetListFrom2DAInternal(const ResRef& resref);

public:
	CFGConfigData config;
	ResRef GameNameResRef;
	ResRef GoldResRef; //MISC07.itm
	ResRefMap<ItemList> RtRows;

	std::string INIConfig = "baldur.ini";
	bool DitherSprites = true;
	bool UseCorruptedHack = false;
	int FeedbackLevel = 0;

	Variables *plugin_flags;
	/** The Main program loop */
	void Main(void);
	/** returns true if the game is paused */
	bool IsFreezed() const;
	void AskAndExit();
	/** CheatKey support */
	inline void EnableCheatKeys(int Flag)
	{
		config.CheatFlag = Flag > 0;
	}

	inline bool CheatEnabled() const
	{
		return config.CheatFlag;
	}

	inline void SetEventFlag(int Flag)
	{
		EventFlag|=Flag;
	}
	inline void ResetEventFlag(int Flag)
	{
		EventFlag&=~Flag;
	}
	inline void ResetActionBar()
	{
		vars->SetAt("ActionLevel", 0, false);
		SetEventFlag(EF_ACTION);
	}

	static void SanityCheck(const char *ver);

	/** Set Next Script */
	void SetNextScript(const char *script);

	Audio* GetAudioDrv(void) const;

	Timer& SetTimer(const EventHandler&, tick_t interval, int repeats = -1);
};

extern GEM_EXPORT Interface * core;

}

#endif
