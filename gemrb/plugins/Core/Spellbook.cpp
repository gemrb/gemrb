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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Spellbook.cpp,v 1.1 2004/03/29 23:52:29 edheldil Exp $
 *
 */

#include <stdio.h>
#include "../../includes/win32def.h"
// #include "Interface.h"
#include "Spellbook.h"




Spellbook::Spellbook()
{
}

Spellbook::~Spellbook()
{
	for (size_t i = 0; i < known_spells.size(); i++) {
		delete( known_spells[i] );
	}
	for (size_t i = 0; i < spell_memorization.size(); i++) {
		delete( spell_memorization[i] );
	}
	for (size_t i = 0; i < memorized_spells.size(); i++) {
		delete( memorized_spells[i] );
	}
	for (size_t i = 0; i < cast_spells.size(); i++) {
		delete( cast_spells[i] );
	}
}

int Spellbook::AddKnownSpell(CREKnownSpell* spell)
{
	for (size_t i = 0; i < known_spells.size(); i++) {
		if (!known_spells[i]) {
			known_spells[i] = spell;
			return ( int ) i;
		}
	}
	known_spells.push_back( spell );
	return known_spells.size() - 1;
}

int Spellbook::AddSpellMemorization(CRESpellMemorization* sm)
{
	for (size_t i = 0; i < spell_memorization.size(); i++) {
		if (!spell_memorization[i]) {
			spell_memorization[i] = sm;
			return ( int ) i;
		}
	}
	spell_memorization.push_back( sm );
	return spell_memorization.size() - 1;
}

int Spellbook::AddMemorizedSpell(CREMemorizedSpell* spell)
{
	//spell->MemorizedPtr = memorized_spells[spell->MemorizedIndex];
	//spell->MemorizedIndex = 0;

	for (size_t i = 0; i < memorized_spells.size(); i++) {
		if (!memorized_spells[i]) {
			memorized_spells[i] = spell;
			return ( int ) i;
		}
	}
	memorized_spells.push_back( spell );
	return memorized_spells.size() - 1;
}

bool Spellbook::MemorizeSpell(int index, bool usable)
{
	// assert index >= 0
	if (index >= known_spells.size()) return false; // FIXME: or -1?

	CREKnownSpell *spl = known_spells[index];
	CRESpellMemorization *sm = NULL;

	int cnt = spell_memorization.size();

	// Find match in spell memorization
	for (int i = 0; i < cnt; i++) {
		sm = spell_memorization[i];

		if (spl->Level != sm->Level || spl->Type != sm->Type)
			continue;

		// ok, we found a match

		// is there still room?
		if (sm->Number <= sm->MemorizedCount)
			return false;

		CREMemorizedSpell* mem_spl = new CREMemorizedSpell();
		strncpy( mem_spl->SpellResRef, spl->SpellResRef, 8 );
		mem_spl->Flags = usable ? 1 : 0; // FIXME: is it all it's used for?

		if (sm->MemorizedCount > 0) {
			int pos = sm->MemorizedIndex;
			//memorized_spells.insert( memorized_spells[pos], mem_spl );
			memorized_spells.insert( memorized_spells.begin() + pos, mem_spl );
			for (int j = 0; j < cnt; j++)
				if (spell_memorization[j]->MemorizedIndex > pos)
					spell_memorization[j]->MemorizedIndex++;
		}
		else {
			memorized_spells.push_back( mem_spl );
			sm->MemorizedIndex = memorized_spells.size() - 1;
		}
		sm->MemorizedCount++;

		return true;
	}

	return false;
}

bool Spellbook::UnmemorizeSpell(int index)
{
	// FIXME: not yet implemented
	return false;
}


bool Spellbook::ChargeSpell(int index)
{
	if (index < 0 || index >= memorized_spells.size())
		return false;

	memorized_spells[index]->Flags = 1;
	return true;
}

bool Spellbook::DepleteSpell(int index)
{
	if (index < 0 || index >= memorized_spells.size())
		return false;

	memorized_spells[index]->Flags = 0;
	return true;
}

void Spellbook::dump()
{
	printf( "SPELLBOOK:\n" );

	printf( "Known spells:\n" );
	for (size_t i = 0; i < known_spells.size(); i++) {
		CREKnownSpell* spl = known_spells[i];
		if (!spl) continue;

		printf ( "%2d: %8s  L: %d  T: %d\n", i, spl->SpellResRef, spl->Level, spl->Type );
	}

	printf( "Spell memorization:\n" );
	for (size_t i = 0; i < spell_memorization.size(); i++) {
		CRESpellMemorization* spl = spell_memorization[i];
		//if (!spl || !spl->Number) continue;
		if (!spl) continue;

		printf ( "%2d: L: %d; N1: %d; N2: %d; T: %d; MI: %ld; MC: %ld\n", i, spl->Level, spl->Number, spl->Number2, spl->Type, spl->MemorizedIndex, spl->MemorizedCount );
	}

	printf( "Memorized spells:\n" );
	for (size_t i = 0; i < memorized_spells.size(); i++) {
		CREMemorizedSpell* spl = memorized_spells[i];
		if (!spl) continue;

		printf ( "%2d: %8s  %x\n", i, spl->SpellResRef, spl->Flags );
	}
}
