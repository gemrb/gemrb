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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Game.h,v 1.21 2004/03/19 23:03:40 avenger_teambg Exp $
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
	unsigned short Unknown0;
	unsigned short PartyOrder;
	unsigned long OffsetToCRE;
	unsigned long CRESize;
	ieResRef CREResRef;
	unsigned long Orientation;
	ieResRef Area;
	unsigned short XPos;
	unsigned short YPos;
	unsigned short ViewXPos;
	unsigned short ViewYPos;
	unsigned char Unknown28[100];
	unsigned short QuickWeaponSlot[4];
	unsigned char Unknown94[8];
	ieResRef QuickSpellResRef[3];
	unsigned short QuickItemSlot[3];
	unsigned char UnknownBA[6];
} PCStruct;


typedef struct GAMJournalEntry {
	ieStrRef Text;
	ieDword Time; // in seconds
	ieByte Chapter;
	ieByte unknown09;
	ieByte Section;
	ieByte unknown0B;
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
	int PartySize;
public:
	unsigned int GameTime;
	unsigned short WhichFormation;
	unsigned short Formations[5];
	unsigned long PartyGold;
	unsigned long Unknown1c;
	unsigned long PCOffset;
	unsigned long PCCount;
	unsigned long UnknownOffset;
	unsigned long UnknownCount;
	unsigned long NPCOffset;
	unsigned long NPCCount;
	unsigned long GLOBALOffset;
	unsigned long GLOBALCount;
	char AREResRef[9];
	unsigned long Unknown48;
	unsigned long JournalCount;
	unsigned long JournalOffset;
	unsigned long Reputation;
	unsigned long UnknownOffset54;
	unsigned long UnknownCount58;
	unsigned long KillVarsOffset;
	unsigned long KillVarsCount;
	unsigned long FamiliarsOffset;  // offset to known creatures on PST
	char AnotherArea[9];
	char CurrentArea[9];
	unsigned char Unknowns[84];
	
public:
	/* returns actor by slot */
	Actor* GetPC(unsigned int slot);
	/* finds an actor in party by party ID, returns Actor, if not there, returns NULL*/
	Actor* FindPC(unsigned int partyID);
	/* finds an actor in party, returns slot, if not there, returns -1*/
	int InParty(Actor* pc);
	/* finds an actor in store, returns slot, if not there, returns -1*/
	int InStore(Actor* pc);
	/* joins party (if already an npc) */
	int JoinParty(Actor* pc);
	/* return current party size */
	int GetPartySize();
	/* leaves party (if in there) */
	int LeaveParty(Actor* pc);
	/*returns slot*/
	int SetPC(Actor* pc);
	int DelPC(unsigned int slot, bool autoFree = false);
	int DelNPC(unsigned int slot, bool autoFree = false);
	Map* GetMap(unsigned int index);
        Map * GetCurrentMap();
	int AddMap(Map* map);
	int LoadMap(char* ResRef);
	int DelMap(unsigned int index, bool autoFree = false);
	int AddNPC(Actor* npc);
	Actor* GetNPC(unsigned int Index);
	void AddJournalEntry(GAMJournalEntry* entry);
	int GetJournalCount();
	GAMJournalEntry* GetJournalEntry(unsigned int Index);
	bool IsBeastKnown(unsigned int Index) {
		return familiars[Index] != 0;
	}
	void SetBeastKnown(unsigned int Index) {
		familiars[Index] = 1;
	}
	void ShareXP(int XP);
};

#endif
