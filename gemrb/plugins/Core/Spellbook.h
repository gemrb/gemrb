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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Spellbook.h,v 1.1 2004/03/29 23:52:29 edheldil Exp $
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

//const int INVENTORY_SIZE = 38;

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

	//CREMemorizedSpell* MemorizedPtr;
} CRESpellMemorization;




class GEM_EXPORT Spellbook {
private:
	std::vector<CREKnownSpell*> known_spells;
	std::vector<CRESpellMemorization*> spell_memorization;
	std::vector<CREMemorizedSpell*> memorized_spells;
	std::vector<CRECastSpell*> cast_spells;

public: 
	Spellbook();
	virtual ~Spellbook();

	int AddKnownSpell(CREKnownSpell* spell);
	int AddSpellMemorization(CRESpellMemorization* sm);
	int AddMemorizedSpell(CREMemorizedSpell* spell);

	int GetKnownSpellsCount() { return known_spells.size(); };
	int GetMemorizedSpellsCount() { return memorized_spells.size(); };

	int GetKnownSpellsCountOnLevel( int level );
	int GetMemorizedSpellsCountOnLevel( int level );

	CREKnownSpell* GetKnownSpell(int index) { return known_spells[index]; };
	CREMemorizedSpell* GetMemorizedSpell(int index) { return memorized_spells[index]; };

	/** Adds index'th spell from known to memorized */
	bool MemorizeSpell(int index, bool usable);

	/** Removes index'th memorized spell */
	bool UnmemorizeSpell(int index);

	/** Sets index'th spell from memorized as 'not-yet-cast' */
	bool ChargeSpell(int index);

	/** Sets index'th spell from memorized as 'already-cast' */
	bool DepleteSpell(int index);

	/** Dumps spellbook to stdout */
	void dump();
};

#endif
