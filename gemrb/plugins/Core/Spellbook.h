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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Spellbook.h,v 1.19 2006/04/11 16:32:35 avenger_teambg Exp $
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

#include <vector>
#include "../../includes/win32def.h"
#include "../../includes/ie_types.h"

class Actor;

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

#define MAX_SPELL_LEVEL 16

//HaveSpell flags
#define HS_DEPLETE 1

//LearnSpell flags
#define LS_ADDXP  1
#define LS_LEARN  2
#define LS_STATS  4

//LearnSpell return values
#define LSR_OK      0
#define LSR_KNOWN   1  //already knows
#define LSR_INVALID 2  //invalid resref
#define LSR_FAILED  3  //failed stat roll
#define LSR_STAT    4  //insufficient stat (can't learn the spell due to low stat)
#define LSR_LEVEL   5  //insufficient level (low mage, etc level)
#define LSR_FULL    6  //can't learn more spells of this level (due to level)

// !!! Keep these synchronized with GUIDefines.py !!!
typedef enum ieSpellType {
	IE_SPELL_TYPE_PRIEST = 0,
	IE_SPELL_TYPE_WIZARD = 1,
	IE_SPELL_TYPE_INNATE = 2
} ieSpellType;

typedef enum ieIWD2SpellType {
	IE_IWD2_SPELL_BARD = 0,
	IE_IWD2_SPELL_CLERIC = 1,
	IE_IWD2_SPELL_DRUID = 2,
	IE_IWD2_SPELL_PALADIN = 3,
	IE_IWD2_SPELL_RANGER = 4,
	IE_IWD2_SPELL_SORCEROR = 5,
	IE_IWD2_SPELL_WIZARD = 6,
	IE_IWD2_SPELL_DOMAIN = 7,
	IE_IWD2_SPELL_INNATE = 8,
	IE_IWD2_SPELL_SONG = 9,
	IE_IWD2_SPELL_SHAPE = 10
} ieIWD2SPellType;

typedef struct {
	ieResRef SpellResRef;
	ieWord Level;
	ieWord Type;
} CREKnownSpell;

typedef struct {
	ieResRef SpellResRef;
	ieDword Flags;
} CREMemorizedSpell;

typedef struct {
	ieWord  Level;
	ieWord  Number;
	ieWord  Number2;
	ieWord  Type;

	std::vector<CREKnownSpell*> known_spells;
	std::vector<CREMemorizedSpell*> memorized_spells;
} CRESpellMemorization;


/**
 * @class Spellbook
 * Class implementing creature's spellbook and (maybe) spell management 
 */

class GEM_EXPORT Spellbook {
private:
	std::vector<CRESpellMemorization*> *spells;

public: 
	Spellbook();
	~Spellbook();
	static bool InitializeSpellbook();
	static void ReleaseMemory();

	void FreeSpellPage(CRESpellMemorization* sm);
	/** Check if the spell exists, optionally deplete it (casting) */
	bool HaveSpell(const char *resref, ieDword flags);
	bool HaveSpell(int spellid, ieDword flags);

	bool AddSpellMemorization(CRESpellMemorization* sm);
	int GetTypes() const;
	unsigned int GetSpellLevelCount(int type) const;
	unsigned int GetTotalPageCount() const;
	unsigned int GetTotalKnownSpellsCount() const;
	unsigned int GetTotalMemorizedSpellsCount() const;
	unsigned int GetKnownSpellsCount(int type, unsigned int level) const;
	/** removes a spell from memory/book */
	void RemoveSpell(ieResRef ResRef);
	/** adds a spell to the book */
	bool AddKnownSpell(int type, unsigned int level, CREKnownSpell *spl);
	CREKnownSpell* GetKnownSpell(int type, unsigned int level, unsigned int index);
	unsigned int GetMemorizedSpellsCount(int type, unsigned int level) const;
	CREMemorizedSpell* GetMemorizedSpell(int type, unsigned int level, unsigned int index);

	int GetMemorizableSpellsCount(int type, unsigned int level, bool bonus) const;
	void SetMemorizableSpellsCount(int Value, int type, unsigned int level, bool bonus);

	/** Adds spell from known to memorized */
	bool MemorizeSpell(CREKnownSpell* spl, bool usable);

	/** Removes memorized spell */
	bool UnmemorizeSpell(CREMemorizedSpell* spl);

	/** finds the first spell needing to rememorize */
	CREMemorizedSpell* FindUnchargedSpell(int type, int level=0);

	/** Sets spell from memorized as 'not-yet-cast' */
	bool ChargeSpell(CREMemorizedSpell* spl);

	/** Sets spell from memorized as 'already-cast' */
	bool DepleteSpell(CREMemorizedSpell* spl);

	/** picks the highest spell of type and makes it 'already cast' */
	bool DepleteSpell(int type);

	/** recharges all spells */
	void ChargeAllSpells();

	bool CastSpell( ieResRef SpellResRef, Actor* Source, Actor* Target );

	/** Dumps spellbook to stdout for debugging */
	void dump();
};

#endif
