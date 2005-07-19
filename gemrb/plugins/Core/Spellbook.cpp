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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Spellbook.cpp,v 1.28 2005/07/19 20:02:33 avenger_teambg Exp $
 *
 */

#include "Interface.h"

static ieResRef *spelllist = NULL;
static ieResRef *innatelist = NULL;
static ieResRef *songlist = NULL;
static ieResRef *shapelist = NULL;

static bool SBInitialized = false;
static int NUM_SPELL_TYPES = 3;
/* temporarily out
static ieResRef *ResolveSpellName(ieDword index)
{
	if (spelllist) {
		return spelllist+index;
	}
	return NULL;
}

static ieResRef *ResolveInnateName(ieDword index)
{
	if (innatelist) {
		return innatelist+index;
	}
	return NULL;
}

static ieResRef *ResolveSongName(ieDword index)
{
	if (songlist) {
		return songlist+index;
	}
	return NULL;
}

static ieResRef *ResolveShapeName(ieDword index)
{
	if (shapelist) {
		return shapelist+index;
	}
	return NULL;
}
*/
static ieResRef *GetSpellTable(const ieResRef tableresref, int column)
{
	int table = core->LoadTable(tableresref);
	if (table<0) {
		return NULL;
	}
	TableMgr *tab = core->GetTable((unsigned int) table);
	int count = tab->GetRowCount();
	ieResRef *reslist = (ieResRef *) malloc (sizeof(ieResRef) * count);
	for(int i = 0; i<count;i++) {
		strnuprcpy(reslist[i], tab->QueryField(i, column), 8);
	}
	core->DelTable((unsigned int) table);
	return reslist;
}

Spellbook::Spellbook()
{
	if (!SBInitialized) {
		printMessage("Spellbook","Spellbook is not initialized, assuming BG2", LIGHT_RED);
		NUM_SPELL_TYPES = 3;
		SBInitialized = true;
	}
	spells = new std::vector<CRESpellMemorization*> [NUM_SPELL_TYPES];
}


bool Spellbook::InitializeSpellbook()
{
	if (!SBInitialized) {
		SBInitialized=true;
		if (core->HasFeature(GF_HAS_SPELLLIST)) {
			spelllist = GetSpellTable("listspll",7); //this is fucked up
			innatelist = GetSpellTable("listinnt",0);
			songlist = GetSpellTable("listsong",0);
			shapelist = GetSpellTable("listshap",0);
		}
		return true;
	}
	return false;
}

void Spellbook::ReleaseMemory()
{
	if(spelllist) {
		free(spelllist);
		spelllist=NULL;
	}
	if(innatelist) {
		free(innatelist);
		innatelist=NULL;
	}
	if(songlist) {
		free(songlist);
		songlist=NULL;
	}
	SBInitialized=false;
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

//flags bits
// 1 - unmemorize it
bool Spellbook::HaveSpell(int spellid, ieDword flags)
{
	int type = spellid/1000;
	if (type>4) {
		return false;
	}
	type = sections[type];
	spellid = spellid % 1000;

	for (unsigned int j = 0; j < spells[type].size(); j++) {
		CRESpellMemorization* sm = spells[type][j];
		for (unsigned int k = 0; k < sm->memorized_spells.size(); k++) {
			CREMemorizedSpell* ms = sm->memorized_spells[k];
			if (ms->Flags) {
				if (atoi(ms->SpellResRef+4)==spellid) {
					if (flags&HS_DEPLETE) {
						ms->Flags=0;
					}
					return true;
				}
			}
		}
	}
	return false;
}

//if resref=="" then it is a haveanyspell
bool Spellbook::HaveSpell(const char *resref, ieDword flags)
{
	for (int i = 0; i < NUM_SPELL_TYPES; i++) {
		for (unsigned int j = 0; j < spells[i].size(); j++) {
			CRESpellMemorization* sm = spells[i][j];
			for (unsigned int k = 0; k < sm->memorized_spells.size(); k++) {
				CREMemorizedSpell* ms = sm->memorized_spells[k];
				if (ms->Flags) {
					if (resref[0] && stricmp(ms->SpellResRef, resref) ) {
						continue;
					}
					if (flags&HS_DEPLETE) {
						ms->Flags=0;
					}
					return true;
				}
			}
		}
	}
	return false;
}

int Spellbook::GetTypes() const
{
	return NUM_SPELL_TYPES;
}

unsigned int Spellbook::GetSpellLevelCount(int type) const
{
	return spells[type].size();
}

unsigned int Spellbook::GetTotalPageCount() const
{
	unsigned int total = 0;
	for(int type =0; type<NUM_SPELL_TYPES; type++) {
		total += spells[type].size();
	}
	return total;
}

unsigned int Spellbook::GetTotalKnownSpellsCount() const
{
	unsigned int total = 0;
	for(int type =0; type<NUM_SPELL_TYPES; type++) {
		unsigned int level = spells[type].size();
		while(level--) {
			total += GetKnownSpellsCount(type, level);
		}
	}
	return total;
}

unsigned int Spellbook::GetTotalMemorizedSpellsCount() const
{
	unsigned int total = 0;
	for(int type =0; type<NUM_SPELL_TYPES; type++) {
		unsigned int level = spells[type].size();
		while(level--) {
			total += GetMemorizedSpellsCount(type, level);
		}
	}
	return total;
}

unsigned int Spellbook::GetKnownSpellsCount(int type, unsigned int level) const
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
		sm->Level = level;
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

unsigned int Spellbook::GetMemorizedSpellsCount(int type, unsigned int level) const
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
	//when loading, level starts on 0
	unsigned int level = sm->Level;
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

void Spellbook::SetMemorizableSpellsCount(int Value, int type, unsigned int level, bool bonus)
{
	int diff;

	if (type >= NUM_SPELL_TYPES) {
		return;
	}
	if ( level >= spells[type].size() ) {
		CRESpellMemorization *sm = new CRESpellMemorization();
		sm->Type = type;
		sm->Level = level;
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

int Spellbook::GetMemorizableSpellsCount(int type, unsigned int level, bool bonus) const
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
	if (sm->Number2 <= sm->memorized_spells.size())
		return false;

	CREMemorizedSpell* mem_spl = new CREMemorizedSpell();
	strncpy( mem_spl->SpellResRef, spell->SpellResRef, 8 );
	mem_spl->Flags = usable ? 1 : 0; // FIXME: is it all it's used for?

	sm->memorized_spells.push_back( mem_spl );

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

			//Never ever use field length qualifiers it is not portable, if you need to convert, convert to compatible values, anyway we don't need this!
			//printf ( "type: %d: L: %d; N1: %d; N2: %d; T: %d; KC: %d; MC: %d\n", i,
			//	 sm->Level, sm->Number, sm->Number2, sm->Type, (int) sm->known_spells.size(), (int) sm->memorized_spells.size() );

			if (sm->known_spells.size()) 
				printf( " Known spells:\n" );
			for (k = 0; k < sm->known_spells.size(); k++) {
				CREKnownSpell* spl = sm->known_spells[k];
				if (!spl) continue;

				printf ( " %2d: %8s L: %d T: %d\n", k, spl->SpellResRef, spl->Level, spl->Type );
			}

			if (sm->memorized_spells.size()) 
				printf( " Memorized spells:\n" );
			for (k = 0; k < sm->memorized_spells.size (); k++) {
				CREMemorizedSpell* spl = sm->memorized_spells[k];
				if (!spl) continue;

				printf ( " %2u: %8s %x\n", k, spl->SpellResRef, spl->Flags );
			}
		}
	}
}
