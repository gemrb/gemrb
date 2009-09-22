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
 * $Id$
 *
 */

/**
 * @file Game.h
 * Declares Game class, object representing current game state.
 * @author The GemRB Project
 */


class Game;

#ifndef GAME_H
#define GAME_H

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

#include <vector>
#include "../../includes/ie_types.h"
#include "Actor.h"
#include "Map.h"
#include "Particles.h"
#include "Variables.h"

//the length of a round in game ticks?
//default (ROUND_SIZE) is 6 seconds: 15 (AI_UPDATE_TIME)*6 (ROUND_SECODS)=90
#define ROUND_SIZE     90
#define ROUND_SECONDS  6
#define ROUND_PER_TURN 10

//the size of the bestiary register
#define BESTIARY_SIZE 260

//ShareXP flags
#define SX_DIVIDE  1   //divide XP among team members
#define SX_CR      2   //use challenge rating resolution

//joinparty flags
#define JP_JOIN     1  //refresh join time
#define JP_INITPOS  2  //init startpos

//protagonist mode
#define PM_NO       0  //no death checks
#define PM_YES      1  //if protagonist dies, game over
#define PM_TEAM     2  //if team dies, game over

// Flags bits for SelectActor()
// !!! Keep these synchronized with GUIDefines.py !!!
#define SELECT_NORMAL   0x00
#define SELECT_REPLACE  0x01 // when selecting actor, deselect all others
#define SELECT_QUIET    0x02 // do not run handler when changing selection

// Flags bits for EveryoneNearPoint()
#define ENP_CANMOVE     1    // also check if the PC can move
#define ENP_ONLYSELECT  2    // check only selected PC

// GUI Control Status flags (saved in game)
#define CS_PARTY_AI  1   //enable party AI
#define CS_MEDIUM    2   //medium dialog
#define CS_LARGE     6   //large dialog, both bits set
#define CS_DIALOGSIZEMASK 6
#define CS_DIALOG    8   //dialog is running
#define CS_HIDEGUI   16  //hide all gui
#define CS_ACTION    32  //hide action pane
#define CS_PORTRAIT  64  //hide portrait pane
#define CS_MAPNOTES  128 //hide mapnotes

//Weather bits
#define WB_NORMAL    0
#define WB_RAIN      1
#define WB_SNOW      2
#define WB_FOG       3
#define WB_MASK      7
#define WB_LIGHTNING 8
#define WB_HASWEATHER 0x40
#define WB_START      0x80

//Rest flags
#define REST_NOAREA     1 //no area check
#define REST_NOSCATTER  2 //no scatter check
#define REST_NOMOVE     4 //no movement check
#define REST_NOCRITTER  8 //no hostiles check
/**
 * @struct PCStruct
 * Information about party member.
 */

struct PCStruct {
	ieWord   Selected;
	ieWord   PartyOrder;
	ieDword  OffsetToCRE;
	ieDword  CRESize;
	ieResRef CREResRef;
	ieDword  Orientation;
	ieResRef Area;
	ieWord   XPos;
	ieWord   YPos;
	ieWord   ViewXPos;
	ieWord   ViewYPos;
	ieWord   ModalState;
	ieWord   Happiness;
	unsigned char Unknown2c[96];
	ieWord   QuickWeaponSlot[MAX_QUICKWEAPONSLOT];
	ieWord   QuickWeaponHeader[MAX_QUICKWEAPONSLOT];
	ieResRef QuickSpellResRef[MAX_QSLOTS];
	ieWord   QuickItemSlot[MAX_QUICKITEMSLOT];
	ieWord   QuickItemHeader[MAX_QUICKITEMSLOT];
	char Name[32];
	ieDword  TalkCount;
	ieByte QSlots[MAX_QSLOTS];
	ieByte QuickSpellClass[MAX_QSLOTS];
};

#define IE_GAM_JOURNAL 0
#define IE_GAM_QUEST_UNSOLVED 1
#define IE_GAM_QUEST_DONE  2
#define IE_GAM_JOURNAL_USER 3

/**
 * @struct GAMJournalEntry
 * Single entry in a journal
 */

struct GAMJournalEntry {
	ieStrRef Text;
	ieDword  GameTime; // in game time seconds
	ieByte   Chapter;
	ieByte   unknown09;
	ieByte   Section;
	ieByte   Group;   // this is a GemRB extension
};

// Saved location of party member.
struct GAMLocationEntry {
	ieResRef AreaResRef;
	Point Pos;
};

#define MAX_CRLEVEL 32

typedef int CRRow[MAX_CRLEVEL];

/**
 * @class Game
 * Object representing current game state, mostly party.
 */

class GEM_EXPORT Game : public Scriptable {
public:
	Game(void);
	~Game(void);
private:
	std::vector< Actor*> PCs;
	std::vector< Actor*> NPCs;
	std::vector< Map*> Maps;
	std::vector< GAMJournalEntry*> Journals;
	std::vector< GAMLocationEntry*> savedpositions;
	std::vector< GAMLocationEntry*> planepositions;
	std::vector< char*> mastarea;
	std::vector< ieDword> Attackers;
	CRRow *crtable;
	ieResRef restmovies[8];
	ieResRef daymovies[8];
	ieResRef nightmovies[8];
	int MapIndex;
public:
	std::vector< Actor*> selected;
	int version;
	int Expansion;
	Variables* kaputz;
	ieByte* beasts;
	ieByte* mazedata; //only in PST
	ieResRef Familiars[9];
	ieDword CombatCounter;
	ieDword StateOverrideFlag, StateOverrideTime;
	ieDword BanterBlockFlag, BanterBlockTime;

	/** Index of PC selected in non-walking environment (shops, inventory...) */
	int SelectedSingle;
	/** 0 if the protagonist's death doesn't cause game over */
	/** 1 if the protagonist's death causes game over */
	/** 2 if no check is needed (pst) */
	int protagonist;
	/** if party size exceeds this amount, a callback will be called */
	size_t partysize;
	ieDword Ticks;
	ieDword interval; // 1000/AI_UPDATE (a tenth of a round in ms)
	ieDword GameTime;
	ieDword LastScriptUpdate; // GameTime at which UpdateScripts last ran
	ieDword RealTime;
	ieWord  WhichFormation;
	ieWord  Formations[5];
	ieDword PartyGold;
	ieWord NpcInParty;
	ieWord WeatherBits;
	ieDword Unknown48;
	ieDword Reputation;
	ieDword ControlStatus;// used in bg2, iwd (where you can switch panes off)
	ieResRef AnotherArea;
	ieResRef CurrentArea;
	ieResRef LoadMos;
	Actor *timestop_owner;
	ieDword timestop_end;
	Particles *weather;
	int event_timer;
	char event_handler[64]; //like in Control
private:
	/** reads the challenge rating table */
	void LoadCRTable();
public:
	/** Returns the PC's slot count for partyID */
	int FindPlayer(unsigned int partyID);
	/** Returns actor by slot */
	Actor* GetPC(unsigned int slot, bool onlyalive);
	/** Finds an actor in party by party ID, returns Actor, if not there, returns NULL*/
	Actor* FindPC(unsigned int partyID);
	Actor* FindNPC(unsigned int partyID);
	/** Finds an actor in party, returns slot, if not there, returns -1*/
	int InParty(Actor* pc) const;
	/** Finds an actor in store, returns slot, if not there, returns -1*/
	int InStore(Actor* pc) const;
	/** Finds an actor in party by scripting name*/
	Actor* FindPC(const char *deathvar);
	/** Finds an actor in store by scripting name*/
	Actor* FindNPC(const char *deathvar);
	/** Sets the area and position of the actor to the starting position */
	void InitActorPos(Actor *actor);
	/** Joins party */
	int JoinParty(Actor* pc, int join=JP_JOIN);
	/** Return current party size */
	int GetPartySize(bool onlyalive) const;
	/** Returns the npcs count */
	int GetNPCCount() const { return (int)NPCs.size(); }
	/** Sends the hotkey trigger to all selected pcs */
	void SetHotKey(unsigned long Key);
	/** Select PC for non-walking environment (shops, inventory, ...) */
	bool SelectPCSingle(int index);
	/** Get index of selected PC for non-walking env (shops, inventory, ...) */
	int GetSelectedPCSingle() const;
	/** (De)selects actor. */
	bool SelectActor( Actor* actor, bool select, unsigned flags );

	/** Return current party level count for xp calculations */
	int GetPartyLevel(bool onlyalive) const;
	/** Reassigns inparty numbers, call it after party creation */
	void ConsolidateParty();
	/** Removes actor from party (if in there) */
	int LeaveParty(Actor* pc);
	/** Returns slot*/
	int DelPC(unsigned int slot, bool autoFree = false);
	int DelNPC(unsigned int slot, bool autoFree = false);
	/** Returns map in index */
	Map* GetMap(unsigned int index) const;
	/** Returns a map from area name, loads it if needed
	 * use it for the biggest safety, change = true will change the current map */
	Map* GetMap(const char *areaname, bool change);
	/** Returns slot of the map if found */
	int FindMap(const char *ResRef);
	int AddMap(Map* map);
	/** Determine if area is master area*/
	bool MasterArea(const char *area);
	/** Dynamically adding an area to master areas*/
	void SetMasterArea(const char *area);
	/** Returns slot of the map, if it was already loaded,
	 * don't load it again, set changepf == true,
	 * if you want to change the pathfinder too. */
	int LoadMap(const char* ResRef);
	int DelMap(unsigned int index, int forced = 0);
	int AddNPC(Actor* npc);
	Actor* GetNPC(unsigned int Index);
	void SwapPCs(unsigned int Index1, unsigned int Index2);
	bool IsDay();
	void InAttack(ieDword globalID);
	void OutAttack(ieDword globalID);
	int AttackersOf(ieDword globalID, Map *area) const;

	//journal entries
	void DeleteJournalEntry(ieStrRef strref);
	void DeleteJournalGroup(int Group);
	/** Adds a journal entry from dialog data.
	 * Time and chapter are calculated on the fly
	 * Returns false if the entry already exists */
	bool AddJournalEntry(ieStrRef strref, int section, int group);
	/** Adds a journal entry while loading the .gam structure */
	void AddJournalEntry(GAMJournalEntry* entry);
	unsigned int GetJournalCount() const;
	GAMJournalEntry* FindJournalEntry(ieStrRef strref);
	GAMJournalEntry* GetJournalEntry(unsigned int Index);

	unsigned int GetSavedLocationCount() const;
	void ClearSavedLocations();
	GAMLocationEntry* GetSavedLocationEntry(unsigned int Index);

	unsigned int GetPlaneLocationCount() const;
	void ClearPlaneLocations();
	GAMLocationEntry* GetPlaneLocationEntry(unsigned int Index);

	char *GetFamiliar(unsigned int Index);

	bool IsBeastKnown(unsigned int Index) const {
		if (!beasts) {
			return false;
		}
		if (Index>=BESTIARY_SIZE) {
			return false;
		}
		return beasts[Index] != 0;
	}
	void SetBeastKnown(unsigned int Index) {
		if (!beasts) {
			return;
		}
		if (Index>=BESTIARY_SIZE) {
			return;
		}
		beasts[Index] = 1;
	}
	ieWord GetFormation() const {
		if (WhichFormation>4) {
			return 0;
		}
		return Formations[WhichFormation];
	}
	size_t GetAttackerCount() const {
		return Attackers.size();
	}

	/** converts challenge rating to xp */
	int GetXPFromCR(int cr);
	/** shares XP among all party members */
	void ShareXP(int XP, int flags);
	/** returns true if we should start the party overflow window */
	bool PartyOverflow() const;
	/** returns true if actor is an attacker or being attacked */
	bool PCInCombat(Actor *actor) const;
	/** returns true if any pc is attacker or being attacked */
	bool AnyPCInCombat() const;
	/** returns true if the party death condition is true */
	bool EveryoneDead() const;
	/** returns true if no one moves */
	bool EveryoneStopped() const;
	bool EveryoneNearPoint(Map *map, Point &p, int flags) const;
	/** returns true if a PC just died */
	int PartyMemberDied() const;
	/** a party member just died now */
	void PartyMemberDied(Actor *);
	/** Increments chapter variable and refreshes kill stats */
	void IncrementChapter();
	/** Sets party reputation */
	void SetReputation(ieDword r);
	/** Sets the gamescreen control status (pane states, dialog textarea size) */
	void SetControlStatus(int value, int operation);
	/** Sets party size (1-32000) */
	void SetPartySize(int value);
	/** Sets a guiscript function to happen after x AI cycles have elapsed */
	void SetTimedEvent(const char *fname, int count);
	/** Sets protagonist mode to 0-none,1-protagonist,2-team */
	void SetProtagonistMode(int value);
	void StartRainOrSnow(bool conditional, int weather);
	size_t GetLoadedMapCount() const { return Maps.size(); }
	/** Adds or removes gold */
	void AddGold(ieDword add);
	/** Adds ticks to game time */
	void AdvanceTime(ieDword add);
	/** Runs the script engine on the global script and the area scripts
	areas run scripts on door, infopoint, container, actors too */
	void UpdateScripts();
	/** runs area functionality, sets partyrested trigger */
	void RestParty(int checks, int dream, int hp);
	/** timestop effect initiated by actor */
	void TimeStop(Actor *actor, ieDword end);
	/** gets the colour which should be applied over the game area,
	may return NULL */
	const Color *GetGlobalTint() const;
	/** draw weather */
	void DrawWeather(Region &screen, bool update);
	/** updates current area music */
	void ChangeSong(bool force = true);
	/** sets expansion mode */
	void SetExpansion(int exp);
	/** Dumps information about the object */
	void DebugDump();
	/** Finds an actor by global ID */
	Actor *GetActorByGlobalID(ieWord objectID);
private:
	bool DetermineStartPosType(const TableMgr *strta);
	ieResRef *GetDream(Map *area);
};

#endif  // ! GAME_H
