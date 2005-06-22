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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Game.h,v 1.57 2005/06/22 21:21:15 avenger_teambg Exp $
 *
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
#include "Variables.h"

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
#define CS_HIDEGUI   16  //hide all gui
#define CS_ACTION    32  //hide action pane
#define CS_PORTRAIT  64  //hide portrait pane

//Weather bits
#define WB_NORMAL    0
#define WB_RAIN      1
#define WB_SNOW      2
#define WB_FOG       3
#define WB_LIGHTNING 8
#define WB_START     0x80

typedef struct PCStruct {
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
	ieWord   QuickWeaponSlot[4];
	unsigned char Unknown94[8];
	ieResRef QuickSpellResRef[3];
	ieWord   QuickItemSlot[3];
	unsigned char UnknownBA[6];
	char Name[32];
	ieDword  TalkCount;
} PCStruct;

#define IE_GAM_JOURNAL 0
#define IE_GAM_QUEST_UNSOLVED 1
#define IE_GAM_QUEST_DONE  2
#define IE_GAM_JOURNAL_USER 3

typedef struct GAMJournalEntry {
	ieStrRef Text;
	ieDword  GameTime; // in game time seconds
	ieByte   Chapter;
	ieByte   unknown09;
	ieByte   Section;
	ieByte   Group;   // this is a GemRB extension
} GAMJournalEntry;


class GEM_EXPORT Game : public Scriptable {
public:
	Game(void);
	~Game(void);
private:
	std::vector< Actor*> PCs;
	std::vector< Actor*> NPCs;
	std::vector< Map*> Maps;
	std::vector< GAMJournalEntry*> Journals;
	std::vector< char*> mastarea;
	int MapIndex;
public:
	std::vector< Actor*> selected;
	int version;
	Variables* kaputz;
	ieByte* beasts;
	ieByte* mazedata; //only in PST
	ieResRef Familiars[9];
	ieDword CombatCounter;

	/** index of PC selected in non-walking environment (shops, inventory...) */
	int SelectedSingle;

public:
	ieDword GameTime;
	ieDword RealTime;
	ieWord  WhichFormation;
	ieWord  Formations[5];
	ieDword PartyGold;
	ieDword WeatherBits;
	ieDword Unknown48;
	ieDword Reputation;
	ieDword ControlStatus;    // used in bg2, iwd (where you can switch panes off)
	ieResRef AnotherArea;
	ieResRef CurrentArea;
	ieResRef LoadMos;
public:
	/* returns the PC's slot count for partyID*/
	int FindPlayer(unsigned int partyID);
	/* returns actor by slot */
	Actor* GetPC(unsigned int slot, bool onlyalive);
	/* finds an actor in party by party ID, returns Actor, if not there, returns NULL*/
	Actor* FindPC(unsigned int partyID);
	Actor* FindNPC(unsigned int partyID);
	/* finds an actor in party, returns slot, if not there, returns -1*/
	int InParty(Actor* pc) const;
	/* finds an actor in store, returns slot, if not there, returns -1*/
	int InStore(Actor* pc) const;
	/* finds an actor in party by scripting name*/
	Actor* FindPC(const char *deathvar);
	/* finds an actor in store by scripting name*/
	Actor* FindNPC(const char *deathvar);
	/* joins party */
	int JoinParty(Actor* pc, bool join=true);
	/* return current party size */
	int GetPartySize(bool onlyalive) const;
	/* returns the npcs count */
	int GetNPCCount() const { return (int)NPCs.size(); }
	/* select PC for non-walking environment (shops, inventory, ...) */
	bool SelectPCSingle(int index);
	/* get index of selected PC for non-walking env (shops, inventory, ...) */
	int GetSelectedPCSingle() const;
	/* (De)selects actor. */
	bool SelectActor( Actor* actor, bool select, unsigned flags );

	/* return current party level count for xp calculations */
	int GetPartyLevel(bool onlyalive) const;
	/* removes actor from party (if in there) */
	int LeaveParty(Actor* pc);
	/* returns slot*/
	int DelPC(unsigned int slot, bool autoFree = false);
	int DelNPC(unsigned int slot, bool autoFree = false);
	/* returns map in index */
	Map* GetMap(unsigned int index) const;
	/* returns a map from area name, loads it if needed */
	/* use it for the biggest safety, change = true will change the current map */
	Map* GetMap(const char *areaname, bool change);
	/* returns slot of the map if found */
	int FindMap(const char *ResRef);
	/* use GetCurrentArea() */
	//Map * GetCurrentMap();
	int AddMap(Map* map);
	/* determine if area is master area*/
	bool MasterArea(const char *area);
	/* dynamically adding an area to master areas*/
	void SetMasterArea(const char *area);
	/* returns slot of the map, if it was already loaded,
	 	don't load it again, set changepf == true,
		if you want to change the pathfinder too. */
	int LoadMap(const char* ResRef);
	int DelMap(unsigned int index, int forced = 0);
	int AddNPC(Actor* npc);
	Actor* GetNPC(unsigned int Index);

	//journal entries
	void DeleteJournalEntry(ieStrRef strref);
	void DeleteJournalGroup(ieByte Group);
	/* adds a journal entry from dialog data */
	/* time and chapter are calculated on the fly */
	void AddJournalEntry(ieStrRef strref, int section, int group);
	/* adds a journal entry while loading the .gam structure */
	void AddJournalEntry(GAMJournalEntry* entry);
	int GetJournalCount() const;
	GAMJournalEntry* GetJournalEntry(unsigned int Index);

	char *GetFamiliar(unsigned int Index);

	bool IsBeastKnown(unsigned int Index) const {
		if (!beasts) {
			return false;
		}
		return beasts[Index] != 0;
	}
	void SetBeastKnown(unsigned int Index) const {
		if (!beasts) {
			return;
		}
		beasts[Index] = 1;
	}
	void ShareXP(int XP, bool divide);
	bool EveryoneStopped() const;
	bool EveryoneNearPoint(const char *area, Point &p, int flags) const;
	bool PartyMemberDied() const;
	/* increments chapter variable and refreshes kill stats */
	void IncrementChapter();
	/* sets party reputation */
	void SetReputation(int r);
	/* sets the gamescreen control status (pane states, dialog textarea size) */
	void SetControlStatus(int value, int operation);
	void StartRainOrSnow(bool conditional, int weather);
	size_t GetLoadedMapCount() const { return Maps.size(); }
};

#endif
