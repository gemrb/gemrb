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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Spellbook.h,v 1.4 2004/04/18 19:20:49 avenger_teambg Exp $
 *
 */

/* Class implementing creature's spellbook and (maybe) spell management */

#ifndef SPELLBOOK_H
#define SPELLBOOK_H

#include <vector>
#include "../../includes/win32def.h"
#include "../../includes/ie_types.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

const int NUM_SPELL_TYPES = 3;

typedef enum ieSpellType {
	IE_SPELL_TYPE_PRIEST = 0,
	IE_SPELL_TYPE_WIZARD = 1,
	IE_SPELL_TYPE_INNATE = 2,
} ieSpellType;

typedef struct CREKnownSpell {
	ieResRef SpellResRef;
	ieWord Level;
	ieWord Type;
} CREKnownSpell;

typedef struct CREMemorizedSpell {
	ieResRef SpellResRef;
	ieDword Flags;
} CREMemorizedSpell;

typedef struct CRECastSpell {    // IWD2 only
	ieDword Type;
	ieDword TotalCount;
	ieDword RemainingCount;
	ieDword Unknown0B;
} CRECastSpell;

typedef struct CRESpellMemorization {
	ieWord  Level;
	ieWord  Number;
	ieWord  Number2;
	ieWord  Type;
	ieDword MemorizedIndex;
	ieDword MemorizedCount;

	std::vector<CREKnownSpell*> known_spells;
	std::vector<CREMemorizedSpell*> memorized_spells;
} CRESpellMemorization;


class GEM_EXPORT Spellbook {
private:
	std::vector<CRESpellMemorization*> spells[3];

public: 
	Spellbook();
	virtual ~Spellbook();

	void FreeSpellPage(CRESpellMemorization* sm);
	bool HaveSpell(const char *resref, ieDword flags);
	bool HaveSpell(int spellid, ieDword flags);

	//int AddKnownSpell(CREKnownSpell* spell);
	bool AddSpellMemorization(CRESpellMemorization* sm);
	//int AddMemorizedSpell(CREMemorizedSpell* spell);

	int GetKnownSpellsCount(int type, int level);
	int GetMemorizedSpellsCount(int type, int level);

	//int GetKnownSpellsCountOnLevel( int level );
	//int GetMemorizedSpellsCountOnLevel( int level );

	//CREKnownSpell* GetKnownSpell(int index) { return known_spells[index]; };
	//CREMemorizedSpell* GetMemorizedSpell(int type, int level, int index) { return memorized_spells[index]; };

	/** Adds spell from known to memorized */
	bool MemorizeSpell(CREKnownSpell* spl, bool usable);

	/** Removes memorized spell */
	bool UnmemorizeSpell(CREMemorizedSpell* spl);

	/** Sets index'th spell from memorized as 'not-yet-cast' */
	bool ChargeSpell(CREMemorizedSpell* spl);

	/** Sets index'th spell from memorized as 'already-cast' */
	bool DepleteSpell(CREMemorizedSpell* spl);

	void ChargeAllSpells();

	/** Dumps spellbook to stdout */
	void dump();
};

#endif
