/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Game.h,v 1.36 2004/08/05 20:41:07 guidoj Exp $
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
	unsigned char Unknown28[100];
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
public:
	Variables* globals;
	ieByte* familiars;
	int MapIndex;
	ieDword CombatCounter;

	/** index of PC selected in non-walking environment (shops, inventory...) */
	int SelectedSingle;

public:
	ieDword GameTime;
	ieWord  WhichFormation;
	ieWord  Formations[5];
	ieDword PartyGold;
	ieDword Unknown1c;
	ieDword PCOffset;
	ieDword PCCount;
	ieDword UnknownOffset;
	ieDword UnknownCount;
	ieDword NPCOffset;
	ieDword NPCCount;
	ieDword GLOBALOffset;
	ieDword GLOBALCount;
	char AREResRef[9];
	ieDword Unknown48;
	ieDword JournalCount;
	ieDword JournalOffset;
	ieDword Reputation;
	ieDword UnknownOffset54;
	ieDword UnknownCount58;
	ieDword KillVarsOffset;
	ieDword KillVarsCount;
	ieDword FamiliarsOffset;  // offset to known creatures on PST
	char AnotherArea[9];
	char CurrentArea[9];
	char LoadMos[9];
public:
	/* returns the PC's slot count for partyID*/
	int FindPlayer(unsigned int partyID);
	/* returns actor by slot */
	Actor* GetPC(unsigned int slot);
	/* finds an actor in party by party ID, returns Actor, if not there, returns NULL*/
	Actor* FindPC(unsigned int partyID);
	Actor* FindNPC(unsigned int partyID);
	/* finds an actor in party, returns slot, if not there, returns -1*/
	int InParty(Actor* pc);
	/* finds an actor in store, returns slot, if not there, returns -1*/
	int InStore(Actor* pc);
	/* finds an actor in party by scripting name*/
	Actor* FindPC(const char *deathvar);
	/* finds an actor in store by scripting name*/
	Actor* FindNPC(const char *deathvar);
	/* joins party */
	int JoinParty(Actor* pc);
	/* return current party size */
	int GetPartySize(bool onlyalive);
	/* returns the npcs count */
	int GetNPCCount() { return (int)NPCs.size(); }
	/* select PC for non-walking environment (shops, inventory, ...) */
	bool SelectPCSingle(int index);
	/* get index of selected PC for non-walking env (shops, inventory, ...) */
	int GetSelectedPCSingle();
	/* return current party level count for xp calculations */
	int GetPartyLevel(bool onlyalive);
	/* removes actor from party (if in there) */
	int LeaveParty(Actor* pc);
	/* returns slot*/
	int SetPC(Actor* pc);
	int DelPC(unsigned int slot, bool autoFree = false);
	int DelNPC(unsigned int slot, bool autoFree = false);
	/* returns map in index */
	Map* GetMap(unsigned int index);
	/* returns a map from area name, loads it if needed */
	/* use it for the biggest safety */
	Map* GetMap(const char *areaname);
	/* returns slot of the map if found */
	int FindMap(const char *ResRef);
        Map * GetCurrentMap();
	int AddMap(Map* map);
	/* determine if area is master area*/
	bool MasterArea(const char *area);
	/* returns slot of the map, if it was already loaded,
	 	don't load it again, set changepf == true,
		if you want to change the pathfinder too. */
	int LoadMap(const char* ResRef);
	int DelMap(unsigned int index, bool autoFree = false);
	int AddNPC(Actor* npc);
	Actor* GetNPC(unsigned int Index);
	/* adds a journal entry from dialog data */
	/* time and chapter are calculated on the fly */
	void AddJournalEntry(ieStrRef strref, int section, int group);
	/* adds a journal entry while loading the .gam structure */
	void AddJournalEntry(GAMJournalEntry* entry);
	int GetJournalCount();
	GAMJournalEntry* GetJournalEntry(unsigned int Index);
	void DeleteJournalGroup(ieByte Group);
	void DeleteJournalEntry(ieStrRef strref);

	bool IsBeastKnown(unsigned int Index) {
		return familiars[Index] != 0;
	}
	void SetBeastKnown(unsigned int Index) {
		familiars[Index] = 1;
	}
	void ShareXP(int XP);
	bool EveryoneStopped();
	bool EveryoneNearPoint(const char *area, int x, int y, bool canmove);
	bool PartyMemberDied();
};

#endif
