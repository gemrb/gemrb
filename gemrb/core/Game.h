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
 * @file Game.h
 * Declares Game class, object representing current game state.
 * @author The GemRB Project
 */


#ifndef GAME_H
#define GAME_H

#include "exports.h"
#include "ie_types.h"

#include "Callback.h"

#include "Scriptable/PCStatStruct.h"
#include "Scriptable/Scriptable.h"
#include "Video/Video.h"

#include <array>
#include <atomic>
#include <vector>

namespace GemRB {

class Actor;
class Map;
class Particles;
class TableMgr;

//the size of the bestiary register
#define BESTIARY_SIZE 260

//ShareXP flags
#define SX_DIVIDE 1 //divide XP among team members
#define SX_CR     2 //use challenge rating resolution
#define SX_COMBAT 4 //combat xp, adjusted by difficulty

//joinparty flags
#define JP_JOIN    1 //refresh join time
#define JP_INITPOS 2 //init startpos
#define JP_SELECT  4 //select the actor after joining
#define JP_OVERRIDE 8 // replace actor, if any

//protagonist mode
#define PM_NO   0 //no death checks
#define PM_YES  1 //if protagonist dies, game over
#define PM_TEAM 2 //if team dies, game over

// Flags bits for SelectActor()
// !!! Keep these synchronized with GUIDefines.py !!!
#define SELECT_NORMAL  0x00
#define SELECT_REPLACE 0x01 // when selecting actor, deselect all others
#define SELECT_QUIET   0x02 // do not run handler when changing selection

// Flags bits for EveryoneNearPoint()
enum ENP {
	CanMove = 1, // also check if the PC can move
	OnlySelect = 2, // check only selected PC
	Familars = 4, // also check familiars
};

// GUI Control Status flags (saved in game)
#define CS_PARTY_AI       1 //enable party AI
#define CS_MEDIUM         2 //medium dialog
#define CS_LARGE          6 //large dialog, both bits set
#define CS_DIALOGSIZEMASK 6
#define CS_DIALOG         8 //dialog is running
#define CS_HIDEGUI        16 //hide all gui
#define CS_ACTION         32 //hide action pane
#define CS_PORTRAIT       64 //hide portrait pane

//Weather bits
#define WB_NORMAL     0
#define WB_RAIN       1
#define WB_SNOW       2
#define WB_FOG        3
#define WB_TYPEMASK   3
#define WB_LIGHTRAIN  4
#define WB_MEDIUMRAIN 8
#define WB_HEAVYRAIN  12
#define WB_RAINMASK   12
#define WB_LIGHTWIND  0x10
#define WB_MEDWIND    0x20
#define WB_STRONGWING 0x30
#define WB_WINDMASK   0x30

#define WB_RARELIGHTNING  0x40
#define WB_MEDLIGHTNING   0x80
#define WB_HEAVYLIGHTNING 0xc0
#define WB_LIGHTNINGMASK  0xc0
#define WB_INCREASESTORM  0x100
#define WB_HASWEATHER     0x200

// Rest flags
enum RestChecks {
	NoCheck = 0,
	Area = 1, // is it allowed at all?
	Scattered = 2, // is the party together?
	InControl = 4, // are pcs controllable?
	Enemies = 8
};

// Song types, index in ARE song section (hardcoded and in musics.ids for scripts (iwd))
#define SONG_DAY   0
#define SONG_NIGHT 1
// SONG_BATTLE_WIN
#define SONG_BATTLE 3
// SONG_BATTLE_LOSE
// SONG_MISC0-4

enum class JournalSection : uint8_t {
	Main,
	Unsolved,
	Solved,
	User,
	UserBit
};

/**
 * @struct GAMJournalEntry
 * Single entry in a journal
 */

struct GAMJournalEntry {
	ieStrRef Text;
	ieDword GameTime; // in game time seconds
	ieByte Chapter;
	ieByte unknown09;
	ieByte Section;
	ieByte Group; // this is a GemRB extension
};

// Saved location of party member.
struct GAMLocationEntry {
	ResRef AreaResRef;
	Point Pos;
};

//pst maze data structures (TODO: create a separate class?)
struct maze_entry {
	ieDword me_override;
	ieDword accessible;
	ieDword valid;
	ieDword trapped;
	ieDword traptype;
	ieWord walls;
	ieDword visited;
};

struct maze_header {
	ieDword maze_sizex, maze_sizey;
	ieDword pos1x, pos1y; //nordom's position
	ieDword pos2x, pos2y; //main hall position
	ieDword pos3x, pos3y; //foyer entrance
	ieDword pos4x, pos4y; //unknown
	ieDword trapcount; //based on map size
	ieDword initialized; //set to 1
	ieDword unknown2c; //unknown
	ieDword unknown30; //unknown
};

#define MAZE_ENTRY_SIZE          sizeof(maze_entry)
#define MAZE_HEADER_SIZE         sizeof(maze_header)
#define MAZE_MAX_DIM             8
#define MAZE_ENTRY_COUNT         (MAZE_MAX_DIM * MAZE_MAX_DIM)
#define MAZE_DATA_SIZE           (MAZE_ENTRY_COUNT * MAZE_ENTRY_SIZE + MAZE_HEADER_SIZE)
#define MAZE_DATA_SIZE_HARDCODED 1720

//maze header indices
#define MH_POS1X     0
#define MH_POS1Y     1
#define MH_POS2X     2
#define MH_POS2Y     3
#define MH_POS3X     4
#define MH_POS3Y     5
#define MH_POS4X     6
#define MH_POS4Y     7
#define MH_TRAPCOUNT 8
#define MH_INITED    9
#define MH_UNKNOWN2C 10
#define MH_UNKNOWN30 11

//maze entry indices
#define ME_OVERRIDE   0
#define ME_VALID      1
#define ME_ACCESSIBLE 2
#define ME_TRAP       3
#define ME_WALLS      4
#define ME_VISITED    5

//ME_WALL bitfields
#define WALL_SOUTH 1
#define WALL_NORTH 2
#define WALL_EAST  4
#define WALL_WEST  8

#define MAX_CRLEVEL 32

// expansion types
constexpr unsigned int GAME_TOB = 5;

using CRRow = int[MAX_CRLEVEL];

/**
 * @class Game
 * Object representing current game state, mostly party.
 */

class GEM_EXPORT Game : public Scriptable {
public:
	using kaputz_t = ieVarsMap;

	Game(void);
	~Game(void) override;

private:
	std::vector<Actor*> PCs;
	std::vector<Actor*> NPCs;
	std::vector<Map*> Maps;
	std::vector<GAMJournalEntry*> Journals;
	std::vector<GAMLocationEntry*> savedpositions;
	std::vector<GAMLocationEntry*> planepositions;
	std::vector<ResRef> mastarea;
	std::vector<std::vector<ResRef>> npclevels;
	CRRow* crtable = nullptr;
	ResRef restmovies[8];
	ResRef daymovies[8];
	ResRef nightmovies[8];
	int MapIndex = -1;
	ResRef Familiars[9];

public:
	std::vector<Actor*> selected;
	int version = 0;
	kaputz_t kaputz;
	std::array<ieByte, BESTIARY_SIZE> beasts;
	ieByte* mazedata = nullptr; //only in PST
	ieDword CombatCounter = 0;
	ieDword StateOverrideFlag = 0;
	ieDword StateOverrideTime = 0;
	ieDword BanterBlockFlag = 0;
	ieDword BanterBlockTime = 0;
	int nextBored = 0;

	/** Index of PC selected in non-walking environment (shops, inventory...) */
	int SelectedSingle = 1;
	/** 0 if the protagonist's death doesn't cause game over */
	/** 1 if the protagonist's death causes game over */
	/** 2 if no check is needed (pst) */
	int protagonist = PM_YES;
	/** if party size exceeds this amount, a callback will be called */
	size_t partysize = 6;
	std::atomic_uint32_t GameTime { 0 };
	ieDword RealTime = 0;
	ieWord WhichFormation = 0; // 0-4 index into Formations, not an actual formation!
	ieWord Formations[5] {};
	ieDword PartyGold = 0;
	ieWord NPCAreaViewed = 0;
	ieWord WeatherBits = 0;
	ieDword CurrentLink = 0; //named currentLink in original engine (set to -1)
	ieDword Reputation = 0;
	ieDword ControlStatus = 0; // used in bg2, iwd (where you can switch panes off)
	ieDword Expansion = 0; // mostly used by BG2. IWD games set it to 3 on newgame
	ieDword zoomLevel = 0; // ee-style zoom, 0 or 100: default zoom level, >100: zoomed out, <100: zoomed in
	ResRef AnotherArea;
	ResRef CurrentArea;
	ResRef PreviousArea; //move here if the worldmap exit is illegal?
	ResRef LastMasterArea; // last party-visited master area
	ResRef LoadMos;
	ResRef TextScreen;

	// EE-only stuff that we don't really need yet
	ResRef RandomEncounterArea;
	ResRef CurrentWorldMap;
	ResRef CurrentCampaign; // eg. "SOD"
	ieDword FamiliarOwner = 0; // IWDEE: which player has the familiar? InParty - 1
	FixedSizeString<20> RandomEncounterEntry;

	Particles* weather = nullptr;
	int event_timer = 0;
	EventHandler event_handler = nullptr;
	bool hasInfra = false;
	bool familiarBlock = false;
	bool PartyAttack = false;
	bool HOFMode = false;

private:
	/** reads the challenge rating table */
	void LoadCRTable();
	Actor* timestopper = nullptr;
	ieDword timestopEnd = 0;

public:
	/** Returns the PC's slot count for partyID */
	int FindPlayer(unsigned int partyID) const;
	/** Returns actor by slot */
	Actor* GetPC(size_t slot, bool onlyAlive) const;
	/** Finds an actor in party by party ID, returns Actor, if not there, returns NULL*/
	Actor* FindPC(unsigned int partyID) const;
	Actor* FindNPC(unsigned int partyID) const;
	/** Finds a global actor by global ID */
	Actor* GetGlobalActorByGlobalID(ieDword globalID) const;
	/** Finds an actor in party, returns slot, if not there, returns -1*/
	int InParty(const Actor* pc) const;
	/** Finds an actor in store, returns slot, if not there, returns -1*/
	int InStore(const Actor* pc) const;
	/** Finds an actor in party by scripting name*/
	Actor* FindPC(const ieVariable& deathVar) const;
	/** Finds an actor in store by scripting name*/
	Actor* FindNPC(const ieVariable& deathVar) const;
	/** Sets the area and position of the actor to the starting position */
	void InitActorPos(Actor* actor) const;
	/** Joins party */
	int JoinParty(Actor* pc, int join = JP_JOIN);
	/** Return current party size */
	int GetPartySize(bool onlyAlive) const;
	/** Returns the npcs count */
	int GetNPCCount() const { return (int) NPCs.size(); }
	/** Sends the hotkey trigger to all selected pcs */
	void SendHotKey(unsigned long key) const;
	/** Select PC for non-walking environment (shops, inventory, ...) */
	bool SelectPCSingle(int index);
	/** Get index of selected PC for non-walking env (shops, inventory, ...) */
	int GetSelectedPCSingle() const;
	Actor* GetSelectedPCSingle(bool onlyAlive) const;
	/** (De)selects actor. */
	bool SelectActor(Actor* actor, bool select, unsigned flags);

	/** Return current party level count for xp calculations */
	int GetTotalPartyLevel(bool onlyAlive) const;
	/** Reassigns inparty numbers, call it after party creation */
	void ConsolidateParty() const;
	/** Removes actor from party (if in there) */
	int LeaveParty(Actor* pc, bool returnCriticalItems = true);
	/** Returns slot*/
	int DelPC(unsigned int slot, bool autoFree = false);
	int DelNPC(unsigned int slot, bool autoFree = false);
	/** Returns map in index */
	Map* GetMap(unsigned int index) const;
	/** Returns a map from area name, loads it if needed
	 * use it for the biggest safety, change = true will change the current map */
	Map* GetMap(const ResRef& areaName, bool change);
	/** Returns slot of the map if found */
	int FindMap(const ResRef& resRef) const;
	int AddMap(Map* map);
	/** Determine if area is master area*/
	bool MasterArea(const ResRef& area) const;
	/** Dynamically adding an area to master areas*/
	void SetMasterArea(const ResRef& area);
	/** place persistent actors in the fresly loaded area*/
	void PlacePersistents(Map* map, const ResRef& resRef);
	/** Returns slot of the map, if it was already loaded,
	 * don't load it again, set changepf == true,
	 * if you want to change the pathfinder too. */
	int LoadMap(const ResRef& resRef, bool loadScreen);
	int DelMap(unsigned int index, int forced = 0);
	int AddNPC(Actor* npc);
	Actor* GetNPC(unsigned int Index) const;
	void SwapPCs(unsigned int pc1, unsigned int pc2) const;
	bool IsDay() const;
	/** checks if the actor should be replaced via npclevel.2da and then does it */
	bool CheckForReplacementActor(size_t i);

	//journal entries
	/** Deletes one or all journal entries if strref is -1 */
	void DeleteJournalEntry(ieStrRef strRef);
	/** Delete entries of the same group */
	void DeleteJournalGroup(ieByte group);
	/** Adds a journal entry from dialog data.
	 * Time and chapter are calculated on the fly
	 * Returns false if the entry already exists */
	bool AddJournalEntry(ieStrRef strRef, JournalSection section, ieByte group, ieStrRef feedback = ieStrRef::INVALID);
	/** Adds a journal entry while loading the .gam structure */
	void AddJournalEntry(GAMJournalEntry* entry);
	unsigned int GetJournalCount() const;
	GAMJournalEntry* FindJournalEntry(ieStrRef strRef) const;
	GAMJournalEntry* GetJournalEntry(unsigned int index) const;

	//saved locations
	unsigned int GetSavedLocationCount() const;
	void ClearSavedLocations();
	GAMLocationEntry* GetSavedLocationEntry(unsigned int index);

	//plane locations
	unsigned int GetPlaneLocationCount() const;
	void ClearPlaneLocations();
	GAMLocationEntry* GetPlaneLocationEntry(unsigned int index);

	const ResRef& GetFamiliar(size_t index) const;
	void SetFamiliar(const ResRef& familiar, size_t index);

	bool IsBeastKnown(unsigned int index) const
	{
		if (index >= BESTIARY_SIZE) {
			return false;
		}
		return beasts[index] != 0;
	}
	void SetBeastKnown(unsigned int index)
	{
		if (index >= BESTIARY_SIZE) {
			return;
		}
		beasts[index] = 1;
	}
	ieWord GetFormation() const
	{
		if (WhichFormation > 4) {
			return 0;
		}
		return Formations[WhichFormation];
	}

	/** converts challenge rating to xp */
	int GetXPFromCR(int cr) const;
	/** shares XP among all party members */
	void ShareXP(int XP, int flags) const;
	/** returns true if we should start the party overflow window */
	bool PartyOverflow() const;
	/** returns true if any pc is attacker or being attacked */
	bool AnyPCInCombat() const;
	/** returns true if the party death condition is true */
	bool EveryoneDead() const;
	/** returns true if no one moves */
	bool EveryoneStopped() const;
	bool EveryoneNearPoint(const Map* map, const Point& p, int flags) const;
	/** a party member just died now */
	void PartyMemberDied(const Actor*) const;
	/** Increments chapter variable and refreshes kill stats */
	void IncrementChapter();
	/** Sets party reputation */
	void SetReputation(ieDword r, ieDword min = 10);
	/** Sets the gamescreen control status (pane states, dialog textarea size) */
	bool SetControlStatus(unsigned int value, BitOp operation);
	/** Sets party size (1-32000) */
	void SetPartySize(int value);
	/** Sets a guiscript function to happen after x AI cycles have elapsed */
	void SetTimedEvent(EventHandler func, int count);
	/** Sets protagonist mode to 0-none,1-protagonist,2-team */
	void SetProtagonistMode(int value);
	void StartRainOrSnow(bool conditional, ieWord weather);
	size_t GetLoadedMapCount() const { return Maps.size(); }
	/** Adds or removes gold */
	void AddGold(int add);
	/** Adds ticks to game time */
	void AdvanceTime(ieDword add, bool fatigue = true);
	/** Runs the script engine on the global script and the area scripts
	areas run scripts on door, infopoint, container, actors too */
	void UpdateScripts();
	/** checks if resting is possible */
	bool CanPartyRest(RestChecks checks, ieStrRef* err = nullptr) const;
	/** runs area functionality, sets partyrested trigger */
	bool RestParty(RestChecks checks, int dream, int hp);
	/** timestop effect initiated by actor */
	void TimeStop(Actor* actor, ieDword end);
	/** check if the passed actor is a victim of timestop */
	bool TimeStoppedFor(const Actor* target = nullptr) const;
	/** updates the infravision info */
	void Infravision();
	/** applies the global tint if it is needed */
	void ApplyGlobalTint(Color& tint, BlitFlags& flags) const;
	/** gets the colour which should be applied over the game area,
	may return NULL */
	const Color* GetGlobalTint() const;
	/** returns true if party has infravision */
	bool PartyHasInfravision() const { return hasInfra; }
	/** draw weather */
	void DrawWeather(bool update);
	/** updates current area music */
	void ChangeSong(bool always = true, bool force = true) const;
	/** sets expansion mode */
	void SetExpansion(ieDword value);
	/** Dumps information about the object */
	std::string dump() const override;
	/** Finds an actor by global ID */
	Actor* GetActorByGlobalID(ieDword objectID) const;
	/** Allocates maze data */
	ieByte* AllocateMazeData();
	/** Checks if any timestop effects are active */
	bool IsTimestopActive() const;
	int RemainingTimestop() const;
	Actor* GetTimestopOwner() const { return timestopper; };
	void SetTimestopOwner(Actor* owner) { timestopper = owner; };
	/** Checks the bounty encounters (used in bg1) */
	bool RandomEncounter(ResRef& baseArea) const;
	/** Resets the area and bored comment timers of the whole party */
	void ResetPartyCommentTimes() const;
	void ReversePCs() const;
	bool OnlyNPCsSelected() const;
	void MovePCs(const ResRef& targetArea, const Point& targetPoint, int orientation) const;
	void MoveFamiliars(const ResRef& targetArea, const Point& targetPoint, int orientation) const;
	bool IsTargeted(ieDword gid) const;
	// GLOBAL is just the LOCALS of the Game Scriptable, but we want to avoid any confusion
	ieDword GetGlobal(const ieVariable& key, ieDword fallback) const { return GetLocal(key, fallback); };
	bool CheckPartyBanter() const;
	void CheckBored();
	void CheckAreaComment();

private:
	ResRef* GetDream(Map* area);
	bool RestPartyInternal(RestChecks checks, int hp, int& hours);
	bool CastOnRest() const;
	void PlayerDream() const;
	void TextDream();
};

}

#endif // ! GAME_H
