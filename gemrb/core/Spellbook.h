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
 * @file Spellbook.h
 * Declares Spellbook, class implementing creature's spellbook 
 * and (maybe) spell management 
 * @author The GemRB Project
 */

#ifndef SPELLBOOK_H
#define SPELLBOOK_H

#include "exports.h"
#include "ie_types.h"

#include <vector>

namespace GemRB {

class Actor;
class Spell;

#define MAX_SPELL_LEVEL 16

//HaveSpell flags
#define HS_DEPLETE 1

//LearnSpell flags
#define LS_ADDXP 1 //give xp for learning it
#define LS_LEARN 2 //give message when learned it
#define LS_STATS 4 //check stats (alignment, etc)
#define LS_MEMO  8 //memorize it instantly (add innate)
#define LS_NOXP  16 //disable giving of xp (LS_ADDXP)

//LearnSpell return values
#define LSR_OK      0
#define LSR_KNOWN   1 //already knows
#define LSR_INVALID 2 //invalid resref
#define LSR_FAILED  3 //failed stat roll
#define LSR_STAT    4 //insufficient stat (can't learn the spell due to low stat)
#define LSR_LEVEL   5 //insufficient level (low mage, etc level)
#define LSR_FULL    6 //can't learn more spells of this level (due to level)

// !!! Keep these synchronized with GUIDefines.py !!!
using ieSpellType = enum ieSpellType {
	IE_SPELL_TYPE_PRIEST = 0,
	IE_SPELL_TYPE_WIZARD = 1,
	IE_SPELL_TYPE_INNATE = 2,
	IE_SPELL_TYPE_SONG = 3 //not in spellbook
};

#define NUM_SPELLTYPES 3

using ieIWD2SpellType = enum ieIWD2SpellType {
	IE_IWD2_SPELL_BARD = 0,
	IE_IWD2_SPELL_CLERIC = 1,
	IE_IWD2_SPELL_DRUID = 2,
	IE_IWD2_SPELL_PALADIN = 3,
	IE_IWD2_SPELL_RANGER = 4,
	IE_IWD2_SPELL_SORCERER = 5,
	IE_IWD2_SPELL_WIZARD = 6,
	IE_IWD2_SPELL_DOMAIN = 7,
	IE_IWD2_SPELL_INNATE = 8,
	IE_IWD2_SPELL_SONG = 9,
	IE_IWD2_SPELL_SHAPE = 10
};

#define NUM_IWD2_SPELLTYPES 11

struct CREKnownSpell {
	ResRef SpellResRef;
	ieWord Level;
	ieWord Type;
};

struct CREMemorizedSpell {
	ResRef SpellResRef;
	ieDword Flags;
};

struct CRESpellMemorization {
	ieWord Level;
	ieWord SlotCount;
	ieWord SlotCountWithBonus;
	ieWord Type;

	std::vector<CREKnownSpell*> known_spells;
	std::vector<CREMemorizedSpell*> memorized_spells;
};

struct SpellExtHeader {
	ieDword level;
	ieDword count;
	ieDword type; //spelltype
	size_t headerindex;
	ieDword slot;
	//these come from the header
	ieByte SpellForm;
	ResRef memorisedIcon;
	ieByte Target;
	ieByte TargetNumber;
	ieWord Range;
	ieWord Projectile;
	ieWord CastingTime;
	//other data
	ResRef spellName;
	ieStrRef strref; //the spell's name
};

/**
 * @class Spellbook
 * Class implementing creature's spellbook and (maybe) spell management 
 */

class GEM_EXPORT Spellbook {
private:
	std::vector<CRESpellMemorization*>* spells;
	std::vector<SpellExtHeader*> spellinfo;
	int sorcerer = 0;
	int innate;

	/** Sets spell from memorized as 'already-cast' */
	bool DepleteSpell(CREMemorizedSpell* spl);
	/** Depletes a sorcerer type spellpage by one */
	void DepleteLevel(const CRESpellMemorization* sm, const ResRef& except) const;
	/** Adds a single spell to the spell info list */
	void AddSpellInfo(unsigned int level, unsigned int type, const ResRef& name, unsigned int idx);
	/** regenerates the spellinfo list */
	void GenerateSpellInfo();
	/** looks up the spellinfo list for an element */
	SpellExtHeader* FindSpellInfo(unsigned int level, unsigned int type, const ResRef& name) const;
	/** removes all instances of a spell from a given page */
	void RemoveMemorization(CRESpellMemorization* sm, const ResRef& resRef);
	/** adds a spell to the book, internal */
	bool AddKnownSpell(CREKnownSpell* spl, int memo);
	/** Adds a new CRESpellMemorization, to the *end* only */
	bool AddSpellMemorization(CRESpellMemorization* sm);

	bool HaveSpell(int spellid, int type, ieDword flags);
	bool KnowSpell(int spellid, int type) const;
	void RemoveSpell(int spellid, int type);

public:
	Spellbook();
	Spellbook(const Spellbook&) = delete;
	~Spellbook();
	Spellbook& operator=(const Spellbook&) = delete;
	static void InitializeSpellbook();
	static void ReleaseMemory();

	void FreeSpellPage(CRESpellMemorization* sm);
	/** duplicates the source spellbook into the current one */
	void CopyFrom(const Actor* source);
	/** Check if the spell is memorised, optionally deplete it (casting) */
	bool HaveSpell(const ResRef& resref, ieDword flags);
	bool HaveSpell(int spellid, ieDword flags);

	int CountSpells(const ResRef& resref, unsigned int type, int flag) const;
	/** Check if the spell is in the book */
	bool KnowSpell(const ResRef& resref, int type = -1, int level = -1) const;
	bool KnowSpell(int spellid) const;

	/** returns a CRESpellMemorization pointer */
	CRESpellMemorization* GetSpellMemorization(unsigned int type, unsigned int level);
	int GetTypes() const;
	bool IsIWDSpellBook() const;
	unsigned int GetSpellLevelCount(int type) const;
	unsigned int GetTotalPageCount() const;
	unsigned int GetTotalKnownSpellsCount() const;
	unsigned int GetTotalMemorizedSpellsCount() const;
	unsigned int GetKnownSpellsCount(int type, unsigned int level) const;
	/** adds the priest slot bonuses from mxsplwis */
	void BonusSpells(int type, int abilityLevel);
	/** clears up the spell bonuses before recalculation */
	void ClearBonus();
	/** removes a spell from memory/book */
	bool RemoveSpell(const CREKnownSpell* spell);
	/** this removes ALL spells of name ResRef */
	void RemoveSpell(const ResRef& resRef, bool onlyknown = false);
	/** this removes ALL spells matching spellid */
	void RemoveSpell(int spellid);

	/** sets the book type */
	void SetBookType(int clss);
	/** adds a spell to the book, returns experience if learned */
	int LearnSpell(Spell* spell, int memo, unsigned int clsmsk, unsigned int kit, int level = -1);
	CREKnownSpell* GetKnownSpell(int type, unsigned int level, unsigned int index) const;
	unsigned int GetMemorizedSpellsCount(int type, bool real) const;
	unsigned int GetMemorizedSpellsCount(int type, unsigned int level, bool real) const;
	unsigned int GetMemorizedSpellsCount(const ResRef& name, int type, bool real) const;
	CREMemorizedSpell* GetMemorizedSpell(int type, unsigned int level, unsigned int index) const;

	int GetMemorizableSpellsCount(int type, unsigned int level, bool bonus) const;
	void SetMemorizableSpellsCount(int Value, int type, unsigned int level, bool bonus);

	/** Adds spell from known to memorized */
	bool MemorizeSpell(const CREKnownSpell* spl, bool usable);

	/** Removes memorized spell */
	bool UnmemorizeSpell(const CREMemorizedSpell* spl);

	/** Removes (or just depletes) memorized spell by ResRef */
	bool UnmemorizeSpell(const ResRef& spellRef, bool deplete, uint8_t flags = 0);

	/** finds the first spell needing to rememorize */
	CREMemorizedSpell* FindUnchargedSpell(int type, int level = 0) const;

	/** Sets spell from memorized as 'not-yet-cast' */
	bool ChargeSpell(CREMemorizedSpell* spl);

	/** Sets spell from memorized as 'already-cast' */
	bool DepleteSpell(int type, unsigned int page, unsigned int slot);

	/** picks the highest spell of type and makes it 'already cast' */
	bool DepleteSpell(int type);

	/** recharges all spells */
	void ChargeAllSpells();

	/** creates sorcerer's selection of spells to memorise:
	selects all spells as many times as the spell page allows */
	void CreateSorcererMemory(int type);

	/** returns the number of distinct spells (generates spellinfo) */
	unsigned int GetSpellInfoSize(int type);

	/** generates a custom spellinfo list for fx_select_spell */
	void SetCustomSpellInfo(const std::vector<ResRef>& data, const ResRef& spell, int type);

	/** invalidates the spellinfo list */
	void ClearSpellInfo();

	/** lists spells of a type */
	bool GetSpellInfo(SpellExtHeader* array, int type, int startindex, int count);

	/** find the first spell matching resref (returns index+1) */
	int FindSpellInfo(SpellExtHeader* array, const ResRef& spellName, unsigned int type);

	/** Dumps spellbook to stdout for debugging */
	std::string dump(bool print = true) const;
};

}

#endif
