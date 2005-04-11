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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Spellbook.cpp,v 1.21 2005/04/11 17:40:16 avenger_teambg Exp $
 *
 */

#include <stdio.h>
#include "../../includes/win32def.h"
#include "Spellbook.h"

Spellbook::Spellbook()
{
}

Spellbook::~Spellbook()
{
	for (int i = 0; i < NUM_SPELL_TYPES; i++) {
		for (unsigned int j = 0; j < spells[i].size(); j++) {
			if (spells[i][j]) {
				FreeSpellPage( spells[i][j] );
				spells[i][j] = NULL;
			}
		}
	}
}

void Spellbook::FreeSpellPage(CRESpellMemorization *sm)
{
	unsigned int i = sm->known_spells.size();
	while(i--) {
		delete sm->known_spells[i];
	}
	i = sm->memorized_spells.size();
	while(i--) {
		delete sm->memorized_spells[i];
	}
	delete sm;
}

//ITEM, SPPR, SPWI, SPIN, SPCL
int sections[]={3,0,1,2,2};

bool Spellbook::HaveSpell(int spellid, ieDword /*flags*/)
{
	int type = spellid/1000;
	type = sections[type];
	spellid = spellid % 1000;

	for (unsigned int j = 0; j < spells[type].size(); j++) {
		CRESpellMemorization* sm = spells[type][j];
		for (unsigned int k = 0; k < sm->memorized_spells.size(); k++) {
			CREMemorizedSpell* ms = sm->memorized_spells[k];
			if (ms->Flags&1) {
				if (atoi(ms->SpellResRef+4)==spellid) {
					return true;
				}
			}
		}
	}
	return false;
}

//if resref=="" then it is a haveanyspell
bool Spellbook::HaveSpell(const char *resref, ieDword /*flags*/)
{
	for (int i = 0; i < NUM_SPELL_TYPES; i++) {
		for (unsigned int j = 0; j < spells[i].size(); j++) {
			CRESpellMemorization* sm = spells[i][j];
			for (unsigned int k = 0; k < sm->memorized_spells.size(); k++) {
				CREMemorizedSpell* ms = sm->memorized_spells[k];
				if (ms->Flags&1) {
					if (resref[0] && stricmp(ms->SpellResRef, resref) ) {
						continue;
					}
					return true;
				}
			}
		}
	}
	return false;
}

unsigned int Spellbook::GetKnownSpellsCount(int type, unsigned int level)
{
	if (type >= NUM_SPELL_TYPES || level >= spells[type].size())
		return 0;
	return spells[type][level]->known_spells.size();
}

bool Spellbook::AddKnownSpell(int type, unsigned int level, CREKnownSpell *spl)
{
	if (type >= NUM_SPELL_TYPES) {
		return false;
	}
	if ( level >= spells[type].size() ) {
		CRESpellMemorization *sm = new CRESpellMemorization();
		sm->Type = type;
		sm->Level = level+1;
		sm->Number = sm->Number2 = 0;
		if ( !AddSpellMemorization(sm) ) {
			delete sm;
			return false;
		}
	}
	spells[type][level]->known_spells.push_back(spl);
	return true;
}

CREKnownSpell* Spellbook::GetKnownSpell(int type, unsigned int level, unsigned int index)
{
	if (type >= NUM_SPELL_TYPES || level >= spells[type].size() || index >= spells[type][level]->known_spells.size())
		return NULL;
	return spells[type][level]->known_spells[index];
}

unsigned int Spellbook::GetMemorizedSpellsCount(int type, unsigned int level)
{
	if (type >= NUM_SPELL_TYPES || level >= spells[type].size())
		return 0;
	return spells[type][level]->memorized_spells.size();
}

CREMemorizedSpell* Spellbook::GetMemorizedSpell(int type, unsigned int level, unsigned int index)
{
	if (type >= NUM_SPELL_TYPES || level >= spells[type].size() || index >= spells[type][level]->memorized_spells.size())
		return NULL;
	return spells[type][level]->memorized_spells[index];
}

bool Spellbook::AddSpellMemorization(CRESpellMemorization* sm)
{
	std::vector<CRESpellMemorization*>* s = &spells[sm->Type];
	unsigned int level = sm->Level-1;
	if (level > 8 ) {
		return false;
	}

	while (s->size() <= level ) {
		s->push_back( NULL );
	}

	if ((*s)[level]) {
		return false;
	}
	(*s)[level] = sm;
	return true;
}

void Spellbook::SetMemorizableSpellsCount(int Value, ieSpellType type, unsigned int level, bool bonus)
{
	int diff;

	if (type >= NUM_SPELL_TYPES) {
		return;
	}
	if ( level >= spells[type].size() ) {
		CRESpellMemorization *sm = new CRESpellMemorization();
		sm->Type = type;
		sm->Level = level+1;
		sm->Number = sm->Number2 = 0;
		if ( !AddSpellMemorization(sm) ) {
			delete sm;
			return;
		}
	}
	CRESpellMemorization* sm = spells[type][level];
	if (bonus) {
		sm->Number2=Value;
	}
	else {
		diff=sm->Number2-sm->Number;
		sm->Number=Value;
		sm->Number2=Value+diff;
	}
}

int Spellbook::GetMemorizableSpellsCount(ieSpellType type, unsigned int level, bool bonus)
{
	if (type >= NUM_SPELL_TYPES || level >= spells[type].size())
		return 0;
	CRESpellMemorization* sm = spells[type][level];
	if (bonus)
		return sm->Number2;
	return sm->Number;
}

bool Spellbook::MemorizeSpell(CREKnownSpell* spell, bool usable)
{
	CRESpellMemorization* sm = spells[spell->Type][spell->Level];
	if (sm->Number2 <= sm->MemorizedCount)
		return false;

	CREMemorizedSpell* mem_spl = new CREMemorizedSpell();
	strncpy( mem_spl->SpellResRef, spell->SpellResRef, 8 );
	mem_spl->Flags = usable ? 1 : 0; // FIXME: is it all it's used for?

	sm->memorized_spells.push_back( mem_spl );
	sm->MemorizedCount++;

	return true;
}

bool Spellbook::UnmemorizeSpell(CREMemorizedSpell* spell)
{
	for (int i = 0; i < NUM_SPELL_TYPES; i++) {
		std::vector< CRESpellMemorization* >::iterator sm;
		for (sm = spells[i].begin(); sm != spells[i].end(); sm++) {
			std::vector< CREMemorizedSpell* >::iterator s;
			for (s = (*sm)->memorized_spells.begin(); s != (*sm)->memorized_spells.end(); s++) {
				if (*s == spell) {

					delete *s;
					(*sm)->memorized_spells.erase( s );
					(*sm)->MemorizedCount--;
					return true;
				}
			}
		}
	}

	return false;
}

void Spellbook::ChargeAllSpells()
{
	for (int i = 0; i < NUM_SPELL_TYPES; i++) {
		for (unsigned int j = 0; j < spells[i].size(); j++) {
			CRESpellMemorization* sm = spells[i][j];

			for (unsigned int k = 0; k < sm->memorized_spells.size(); k++)
				ChargeSpell( sm->memorized_spells[k] );
		}
	}
}

bool Spellbook::ChargeSpell(CREMemorizedSpell* spl)
{
	spl->Flags = 1;
	return true;
}

bool Spellbook::DepleteSpell(CREMemorizedSpell* spl)
{
	spl->Flags = 0;
	return true;
}

void Spellbook::dump()
{
	unsigned int k;

	printf( "SPELLBOOK:\n" );
	for (int i = 0; i < NUM_SPELL_TYPES; i++) {
		for (unsigned int j = 0; j < spells[i].size(); j++) {
			CRESpellMemorization* sm = spells[i][j];
			//if (!sm || !sm->Number) continue;
			if (!sm) continue;

			printf ( "type: %d: L: %d; N1: %d; N2: %d; T: %d; MI: %d; MC: %d\n", i,
				 sm->Level, sm->Number, sm->Number2, sm->Type, sm->MemorizedIndex, sm->MemorizedCount );

			if (sm->known_spells.size()) 
				printf( "  Known spells:\n" );
			for (k = 0; k < sm->known_spells.size(); k++) {
				CREKnownSpell* spl = sm->known_spells[k];
				if (!spl) continue;

				printf ( "  %2d: %8s  L: %d  T: %d\n", k, spl->SpellResRef, spl->Level, spl->Type );
			}

			if (sm->memorized_spells.size()) 
				printf( "  Memorized spells:\n" );
			for (k = 0; k < sm->memorized_spells.size (); k++) {
				CREMemorizedSpell* spl = sm->memorized_spells[k];
				if (!spl) continue;

				printf ( "  %2u: %8s  %x\n", k, spl->SpellResRef, spl->Flags );
			}
		}
	}
}
