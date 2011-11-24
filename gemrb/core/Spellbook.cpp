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

#include "Spellbook.h"

#include "GameData.h"
#include "Interface.h"
#include "Projectile.h"
#include "Spell.h"
#include "TableMgr.h"
#include "Scriptable/Actor.h"

#include <cstdio>

static bool SBInitialized = false;
static int NUM_BOOK_TYPES = 3;
static bool IWD2Style = false;

//spell header-->spell book type conversion (iwd2 is different)
static const int spelltypes[NUM_SPELL_TYPES]={
	IE_SPELL_TYPE_INNATE, IE_SPELL_TYPE_WIZARD, IE_SPELL_TYPE_PRIEST,
	IE_SPELL_TYPE_WIZARD, IE_SPELL_TYPE_INNATE, IE_SPELL_TYPE_SONG
};

Spellbook::Spellbook()
{
	if (!SBInitialized) {
		InitializeSpellbook();
	}
	spells = new std::vector<CRESpellMemorization*> [NUM_BOOK_TYPES];
}

void Spellbook::InitializeSpellbook()
{
	if (!SBInitialized) {
		SBInitialized=true;
		if (core->HasFeature(GF_HAS_SPELLLIST)) {
			NUM_BOOK_TYPES=NUM_IWD2_SPELLTYPES; //iwd2 spell types
		} else {
			NUM_BOOK_TYPES=NUM_SPELLTYPES; //bg/pst/iwd1 spell types
		}
	}
	return;
}

void Spellbook::ReleaseMemory()
{
	SBInitialized=false;
}

Spellbook::~Spellbook()
{
	for (int i = 0; i < NUM_BOOK_TYPES; i++) {
		for (unsigned int j = 0; j < spells[i].size(); j++) {
			if (spells[i][j]) {
				FreeSpellPage( spells[i][j] );
				spells[i][j] = NULL;
			}
		}
	}
	ClearSpellInfo();
	delete [] spells;
}

void Spellbook::FreeSpellPage(CRESpellMemorization *sm)
{
	size_t i = sm->known_spells.size();
	while(i--) {
		delete sm->known_spells[i];
	}
	i = sm->memorized_spells.size();
	while(i--) {
		delete sm->memorized_spells[i];
	}
	delete sm;
}

// FIXME: exclude slayer, all bhaal innates?
void Spellbook::CopyFrom(const Actor *source)
{
	if (!source) {
		return;
	}

	// clear it first
	for (int i = 0; i < NUM_BOOK_TYPES; i++) {
		for (unsigned int j = 0; j < spells[i].size(); j++) {
			if (spells[i][j]) {
				FreeSpellPage( spells[i][j] );
				spells[i][j] = NULL;
			}
		}
		spells[i].clear();
	}
	ClearSpellInfo();

	const Spellbook &wikipedia = source->spellbook;

	for (int t = 0; t < NUM_BOOK_TYPES; t++) {
		for (size_t i = 0; i < wikipedia.spells[t].size(); i++) {
			unsigned int k;
			CRESpellMemorization *wm = wikipedia.spells[t][i];
			CRESpellMemorization *sm = new CRESpellMemorization();
			spells[t].push_back(sm);
			sm->Level = wm->Level;
			sm->Number = wm->Number;
			sm->Number2 = wm->Number2;
			sm->Type = wm->Type;
			for (k = 0; k < wm->known_spells.size(); k++) {
				CREKnownSpell *tmp_known = new CREKnownSpell();
				sm->known_spells.push_back(tmp_known);
				memcpy(tmp_known, wm->known_spells[k], sizeof(CREKnownSpell));
			}
			for (k = 0; k < wm->memorized_spells.size(); k++) {
				CREMemorizedSpell *tmp_mem = new CREMemorizedSpell();
				sm->memorized_spells.push_back(tmp_mem);
				memcpy(tmp_mem, wm->memorized_spells[k], sizeof(CREMemorizedSpell));
			}
		}
	}

	sorcerer = wikipedia.sorcerer;
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
	if (type >= NUM_BOOK_TYPES) {
		return false;
	}
	spellid = spellid % 1000;

	for (unsigned int j = 0; j < GetSpellLevelCount(type); j++) {
		CRESpellMemorization* sm = spells[type][j];
		for (unsigned int k = 0; k < sm->memorized_spells.size(); k++) {
			CREMemorizedSpell* ms = sm->memorized_spells[k];
			if (ms->Flags) {
				if (atoi(ms->SpellResRef+4)==spellid) {
					if (flags&HS_DEPLETE) {
						if (DepleteSpell(ms) && (sorcerer & (1<<type) ) ) {
							DepleteLevel (sm, ms->SpellResRef);
						}
					}
					return true;
				}
			}
		}
	}
	return false;
}

bool Spellbook::KnowSpell(int spellid)
{
	int type = spellid/1000;
	if (type>4) {
		return false;
	}
	type = sections[type];
	if (type >= NUM_BOOK_TYPES) {
		return false;
	}
	spellid = spellid % 1000;

	for (unsigned int j = 0; j < GetSpellLevelCount(type); j++) {
		CRESpellMemorization* sm = spells[type][j];
		for (unsigned int k = 0; k < sm->memorized_spells.size(); k++) {
			CREKnownSpell* ks = sm->known_spells[k];
			if (atoi(ks->SpellResRef+4)==spellid) {
				return true;
			}
		}
	}
	return false;
}

//if resref=="" then it is a knownanyspell
bool Spellbook::KnowSpell(const char *resref)
{
	for (int i = 0; i < NUM_BOOK_TYPES; i++) {
		for (unsigned int j = 0; j < spells[i].size(); j++) {
			CRESpellMemorization* sm = spells[i][j];
			for (unsigned int k = 0; k < sm->memorized_spells.size(); k++) {
				CREKnownSpell* ks = sm->known_spells[k];
				if (resref[0] && stricmp(ks->SpellResRef, resref) ) {
					continue;
				}
				return true;
			}
		}
	}
	return false;
}

//if resref=="" then it is a haveanyspell
bool Spellbook::HaveSpell(const char *resref, ieDword flags)
{
	for (int i = 0; i < NUM_BOOK_TYPES; i++) {
		for (unsigned int j = 0; j < spells[i].size(); j++) {
			CRESpellMemorization* sm = spells[i][j];
			for (unsigned int k = 0; k < sm->memorized_spells.size(); k++) {
				CREMemorizedSpell* ms = sm->memorized_spells[k];
				if (ms->Flags) {
					if (resref[0] && stricmp(ms->SpellResRef, resref) ) {
						continue;
					}
					if (flags&HS_DEPLETE) {
						if (DepleteSpell(ms) && (sorcerer & (1<<i) ) ) {
							DepleteLevel (sm, ms->SpellResRef);
						}
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
	return NUM_BOOK_TYPES;
}

bool Spellbook::IsIWDSpellBook() const
{
	return IWD2Style;
}

unsigned int Spellbook::GetSpellLevelCount(int type) const
{
	assert(type < NUM_BOOK_TYPES);
	return (unsigned int) spells[type].size();
}

unsigned int Spellbook::GetTotalPageCount() const
{
	unsigned int total = 0;
	for (int type = 0; type < NUM_BOOK_TYPES; type++) {
		total += GetSpellLevelCount(type);
	}
	return total;
}

unsigned int Spellbook::GetTotalKnownSpellsCount() const
{
	unsigned int total = 0;
	for (int type = 0; type < NUM_BOOK_TYPES; type++) {
		unsigned int level = GetSpellLevelCount(type);
		while(level--) {
			total += GetKnownSpellsCount(type, level);
		}
	}
	return total;
}

unsigned int Spellbook::GetTotalMemorizedSpellsCount() const
{
	unsigned int total = 0;
	for (int type = 0; type < NUM_BOOK_TYPES; type++) {
		unsigned int level = GetSpellLevelCount(type);
		while(level--) {
			total += GetMemorizedSpellsCount(type, level, false);
		}
	}
	return total;
}

// returns the number of known spells of level (level+1)
unsigned int Spellbook::GetKnownSpellsCount(int type, unsigned int level) const
{
	if (type >= NUM_BOOK_TYPES || level >= GetSpellLevelCount(type))
		return 0;
	return (unsigned int) spells[type][level]->known_spells.size();
}

//called when a spell was removed from spellbook
//this one purges all instances of known spells of the same name from memory
void Spellbook::RemoveMemorization(CRESpellMemorization* sm, const ieResRef ResRef)
{
	std::vector< CREMemorizedSpell* >::iterator ms;

	for (ms = sm->memorized_spells.begin(); ms != sm->memorized_spells.end(); ms++) {
		if (strnicmp(ResRef, (*ms)->SpellResRef, sizeof(ieResRef) ) ) {
			continue;
		}
		delete *ms;
		sm->memorized_spells.erase(ms);
		ms--;
	}
}

//removes one instance of spell (from creknownspell)
bool Spellbook::RemoveSpell(CREKnownSpell* spell)
{
	for (int i = 0; i < NUM_BOOK_TYPES; i++) {
		std::vector< CRESpellMemorization* >::iterator sm;
		for (sm = spells[i].begin(); sm != spells[i].end(); sm++) {
			std::vector< CREKnownSpell* >::iterator ks;
			for (ks = (*sm)->known_spells.begin(); ks != (*sm)->known_spells.end(); ks++) {
				if (*ks == spell) {
					ieResRef ResRef;

					memcpy(ResRef, (*ks)->SpellResRef, sizeof(ieResRef) );
					delete *ks;
					(*sm)->known_spells.erase(ks);
					RemoveMemorization(*sm, ResRef);
					ClearSpellInfo();
					return true;
				}
			}
		}
	}
	return false;
}

//removes all instances of spellid (probably not needed)
//IWD2 clab files use it
void Spellbook::RemoveSpell(int spellid)
{
	int type = spellid/1000;
	if (type>4) {
		return;
	}
	type = sections[type];
	if (type >= NUM_BOOK_TYPES) {
		return;
	}
	spellid = spellid % 1000;
	std::vector< CRESpellMemorization* >::iterator sm;
	for (sm = spells[type].begin(); sm != spells[type].end(); sm++) {
		std::vector< CREKnownSpell* >::iterator ks;

		for (ks = (*sm)->known_spells.begin(); ks != (*sm)->known_spells.end(); ks++) {
			if (atoi((*ks)->SpellResRef+4)==spellid) {
				ieResRef ResRef;

				memcpy(ResRef, (*ks)->SpellResRef, sizeof(ieResRef) );
				delete *ks;
				(*sm)->known_spells.erase(ks);
				RemoveMemorization(*sm, ResRef);
				ks--;
				ClearSpellInfo();
			}
		}
	}
}

//removes spell from both memorized/book
void Spellbook::RemoveSpell(const ieResRef ResRef)
{
	for (int type = 0; type<NUM_BOOK_TYPES; type++) {
		std::vector< CRESpellMemorization* >::iterator sm;
		for (sm = spells[type].begin(); sm != spells[type].end(); sm++) {
			std::vector< CREKnownSpell* >::iterator ks;

			for (ks = (*sm)->known_spells.begin(); ks != (*sm)->known_spells.end(); ks++) {
				if (strnicmp(ResRef, (*ks)->SpellResRef, sizeof(ieResRef) ) ) {
					continue;
				}
				delete *ks;
				(*sm)->known_spells.erase(ks);
				RemoveMemorization(*sm, ResRef);
				ks--;
				ClearSpellInfo();
			}
		}
	}
}

void Spellbook::SetBookType(int bt)
{
	sorcerer = bt;
}

//returns the page group of the spellbook this spelltype belongs to
//psionics are stored in the mage spell list
//wizard/priest are trivial
//songs are stored elsewhere
//wildshapes are marked as innate, they need some hack to get stored
//in the right group
//the rest are stored as innate
int Spellbook::GetSpellType(int spelltype)
{
	if (IWD2Style) return spelltype;

	if (spelltype<6) {
		return spelltypes[spelltype];
	}
	return IE_SPELL_TYPE_INNATE;
}

int Spellbook::LearnSpell(Spell *spell, int memo)
{
	CREKnownSpell *spl = new CREKnownSpell();
	strncpy(spl->SpellResRef, spell->Name, 8);
	spl->Type = (ieWord) GetSpellType(spell->SpellType);
	if ( spl->Type == IE_SPELL_TYPE_INNATE) {
		spl->Level = 0;
	}
	else {
		spl->Level = (ieWord) (spell->SpellLevel-1);
	}
	bool ret=AddKnownSpell(spl, memo);
	if (!ret) {
		delete spl;
	}
	return spell->SpellLevel; // return only the spell level (xp is based on xpbonus)
}

//if flg is set, it will be also memorized
bool Spellbook::AddKnownSpell(CREKnownSpell *spl, int flg)
{
	int type = spl->Type;
	if (type >= NUM_BOOK_TYPES) {
		return false;
	}
	unsigned int level = spl->Level;
	if ( level >= GetSpellLevelCount(type) ) {
		CRESpellMemorization *sm = new CRESpellMemorization();
		sm->Type = (ieWord) type;
		sm->Level = (ieWord) level;
		sm->Number = sm->Number2 = 0;
		if ( !AddSpellMemorization(sm) ) {
			delete sm;
			return false;
		}
	}

	spells[type][level]->known_spells.push_back(spl);
	if (type==IE_SPELL_TYPE_INNATE) {
		spells[type][level]->Number++;
		spells[type][level]->Number2++;
	}
	if (flg) {
		MemorizeSpell(spl, true);
	}
	return true;
}

CREKnownSpell* Spellbook::GetKnownSpell(int type, unsigned int level, unsigned int index) const
{
	if (type >= NUM_BOOK_TYPES || level >= GetSpellLevelCount(type) || index >= spells[type][level]->known_spells.size())
		return NULL;
	return spells[type][level]->known_spells[index];
}

unsigned int Spellbook::GetMemorizedSpellsCount(int type, bool real) const
{
	unsigned int count = 0;
	size_t i=GetSpellLevelCount(type);
	while(i--) {
		if (real) {
			int j = spells[type][i]->memorized_spells.size();
			while(j--) {
				if (spells[type][i]->memorized_spells[j]->Flags) count++;
			}
		} else {
			count += (unsigned int) spells[type][i]->memorized_spells.size();
		}
	}
	return count;
}

unsigned int Spellbook::GetMemorizedSpellsCount(int type, unsigned int level, bool real) const
{
	if (type >= NUM_BOOK_TYPES)
		return 0;
	if (level >= GetSpellLevelCount(type))
		return 0;
	if (real) {
		unsigned int count = 0;
		int j = spells[type][level]->memorized_spells.size();
		while(j--) {
			if (spells[type][level]->memorized_spells[j]->Flags) count++;
		}
		return count;
	}
	return (unsigned int) spells[type][level]->memorized_spells.size();
}

unsigned int Spellbook::GetMemorizedSpellsCount(const ieResRef name, int type, bool real) const
{
	if (type >= NUM_BOOK_TYPES)
		return 0;
	int t;
	if (type<0) {
		t = NUM_BOOK_TYPES-1;
	} else {
		t = type;
	}

	int j = 0;
	while(t>=0) {
		unsigned int level = GetSpellLevelCount(t);
		while(level--) {
			int i = spells[t][level]->memorized_spells.size();
			while(i--) {
				CREMemorizedSpell *cms = spells[t][level]->memorized_spells[i];

				if (strnicmp(cms->SpellResRef, name, sizeof(ieResRef) ) ) continue;
				if (!real || cms->Flags) j++;
			}
		}
		if (type>=0) break;
		t--;
	}
	return j;
}

CREMemorizedSpell* Spellbook::GetMemorizedSpell(int type, unsigned int level, unsigned int index) const
{
	if (type >= NUM_BOOK_TYPES || level >= GetSpellLevelCount(type) || index >= spells[type][level]->memorized_spells.size())
		return NULL;
	return spells[type][level]->memorized_spells[index];
}

//creates a spellbook level
bool Spellbook::AddSpellMemorization(CRESpellMemorization* sm)
{
	if (sm->Type>=NUM_BOOK_TYPES) {
		return false;
	}
	std::vector<CRESpellMemorization*>* s = &spells[sm->Type];
	//when loading, level starts on 0
	unsigned int level = sm->Level;
	if (level > MAX_SPELL_LEVEL ) {
		return false;
	}

	while (s->size() < level ) {
		// this code previously added NULLs, leading to crashes,
		// so this is an attempt to make it not broken
		CRESpellMemorization *newsm = new CRESpellMemorization();
		newsm->Type = sm->Type;
		newsm->Level = (ieWord) s->size();
		newsm->Number = newsm->Number2 = 0;
		s->push_back( newsm );
	}

	// only add this one if necessary
	assert (s->size() == level);
	s->push_back(sm);
	return true;
}

//apply the wisdom bonus on all spell levels for type
//count is optimally the count of spell levels
void Spellbook::BonusSpells(int type, int count, int *bonuses)
{
	int level = GetSpellLevelCount(type);
	if (level>count) level=count;
	for (int i = 0; i < level; i++) {
		CRESpellMemorization* sm = GetSpellMemorization(type, i);
		sm->Number2+=bonuses[i];
	}
}

//call this in every ai cycle when recalculating spell bonus
//TODO:add in wisdom bonus here
void Spellbook::ClearBonus()
{
	int type;

	for (type = 0; type < NUM_BOOK_TYPES; type++) {
		int level = GetSpellLevelCount(type);
		for (int i = 0; i < level; i++) {
			CRESpellMemorization* sm = GetSpellMemorization(type, i);
			sm->Number2=sm->Number;
		}
	}
}

CRESpellMemorization *Spellbook::GetSpellMemorization(unsigned int type, unsigned int level)
{
	if (type >= (unsigned int)NUM_BOOK_TYPES)
		return NULL;

	CRESpellMemorization *sm;
	if (level >= GetSpellLevelCount(type)) {
		sm = new CRESpellMemorization();
		sm->Type = (ieWord) type;
		sm->Level = (ieWord) level;
		sm->Number = sm->Number2 = 0;
		if ( !AddSpellMemorization(sm) ) {
			delete sm;
			return NULL;
		}
		assert(sm == spells[type][level]);
	} else {
		sm = spells[type][level];
	}
	return sm;
}
//if bonus is not set, then sets the base value (adjusts bonus too)
//if bonus is set, then sets only the bonus
//if the bonus value is 0, then the bonus is double base value
//bonus is cummulative, but not saved
void Spellbook::SetMemorizableSpellsCount(int Value, int type, unsigned int level, bool bonus)
{
	int diff;

	if (type >= NUM_BOOK_TYPES) {
		return;
	}

	CRESpellMemorization* sm = GetSpellMemorization(type, level);
	if (bonus) {
		if (!Value) {
			Value=sm->Number;
		}
		sm->Number2=(ieWord) (sm->Number2+Value);
	}
	else {
		diff=sm->Number2-sm->Number;
		sm->Number=(ieWord) Value;
		sm->Number2=(ieWord) (Value+diff);
	}
}

int Spellbook::GetMemorizableSpellsCount(int type, unsigned int level, bool bonus) const
{
	if (type >= NUM_BOOK_TYPES || level >= GetSpellLevelCount(type))
		return 0;
	CRESpellMemorization* sm = spells[type][level];
	if (bonus)
		return sm->Number2;
	return sm->Number;
}

bool Spellbook::MemorizeSpell(CREKnownSpell* spell, bool usable)
{
	CRESpellMemorization* sm = spells[spell->Type][spell->Level];
	if (sm->Number2 <= sm->memorized_spells.size()) {
		//it is possible to have sorcerer type spellbooks for any spellbook type
		if (! (sorcerer & (1<<spell->Type) ) )
			return false;
	}

	CREMemorizedSpell* mem_spl = new CREMemorizedSpell();
	strncpy( mem_spl->SpellResRef, spell->SpellResRef, 8 );
	mem_spl->Flags = usable ? 1 : 0; // FIXME: is it all it's used for?

	sm->memorized_spells.push_back( mem_spl );
	ClearSpellInfo();
	return true;
}

bool Spellbook::UnmemorizeSpell(CREMemorizedSpell* spell)
{
	for (int i = 0; i < NUM_BOOK_TYPES; i++) {
		std::vector< CRESpellMemorization* >::iterator sm;
		for (sm = spells[i].begin(); sm != spells[i].end(); sm++) {
			std::vector< CREMemorizedSpell* >::iterator s;
			for (s = (*sm)->memorized_spells.begin(); s != (*sm)->memorized_spells.end(); s++) {
				if (*s == spell) {
					delete *s;
					(*sm)->memorized_spells.erase( s );
					ClearSpellInfo();
					return true;
				}
			}
		}
	}

	return false;
}

bool Spellbook::UnmemorizeSpell(const ieResRef ResRef, bool deplete)
{
	for (int type = 0; type<NUM_BOOK_TYPES; type++) {
		std::vector< CRESpellMemorization* >::iterator sm;
		for (sm = spells[type].begin(); sm != spells[type].end(); sm++) {
			std::vector< CREMemorizedSpell* >::iterator s;
			for (s = (*sm)->memorized_spells.begin(); s != (*sm)->memorized_spells.end(); s++) {
				if (strnicmp(ResRef, (*s)->SpellResRef, sizeof(ieResRef) ) ) {
					continue;
				}
				if (deplete) {
					(*s)->Flags = 0;
				} else {
					delete *s;
					(*sm)->memorized_spells.erase( s );
				}
				ClearSpellInfo();
				return true;
			}
		}
	}

	return false;
}

//bitfield disabling type: 1 - mage, 2 - cleric etc
//level: if set, then finds that level only
CREMemorizedSpell* Spellbook::FindUnchargedSpell(int type, int level)
{
	int mask=1;

	for (int i = 0; i < NUM_BOOK_TYPES; i++) {
		if (type&mask) {
			mask<<=1;
			continue;
		}
		mask<<=1;
		for (unsigned int j = 0; j<spells[i].size(); j++) {
			CRESpellMemorization* sm = spells[i][j];
			if (level && (sm->Level!=level-1)) {
				continue;
			}

			for (unsigned int k = 0; k < sm->memorized_spells.size(); k++) {
				CREMemorizedSpell *ret = sm->memorized_spells[k];
				if (ret->Flags == 0) {
					return ret;
				}
			}
		}
	}
	return NULL;
}

//creates sorcerer style memory for the given spell type
void Spellbook::CreateSorcererMemory(int type)
{
	for (size_t j = 0; j < spells[type].size(); j++) {
		CRESpellMemorization* sm = spells[type][j];

		size_t cnt = sm->memorized_spells.size();
		while(cnt--) {
			delete sm->memorized_spells[cnt];
		}
		sm->memorized_spells.clear();
		for (unsigned int k = 0; k < sm->known_spells.size(); k++) {
			CREKnownSpell *ck = sm->known_spells[k];
			cnt = sm->Number2;
			while(cnt--) {
				MemorizeSpell(ck, true);
			}
		}
	}
}

void Spellbook::ChargeAllSpells()
{
	int j = 1;
	for (int i = 0; i < NUM_BOOK_TYPES; j+=j,i++) {
		//this spellbook page type is sorcerer-like
		if (sorcerer&j ) {
			CreateSorcererMemory(i);
			continue;
		}

		for (unsigned int j = 0; j < spells[i].size(); j++) {
			CRESpellMemorization* sm = spells[i][j];

			for (unsigned int k = 0; k < sm->memorized_spells.size(); k++)
				ChargeSpell( sm->memorized_spells[k] );
		}
	}
}

//unmemorizes the highest level spell possible
//returns true if successful
bool Spellbook::DepleteSpell(int type)
{
	if (type>=NUM_BOOK_TYPES) {
		return false;
	}
	size_t j = GetSpellLevelCount(type);
	while(j--) {
		CRESpellMemorization* sm = spells[type][j];

		for (unsigned int k = 0; k < sm->memorized_spells.size(); k++) {
			if (DepleteSpell( sm->memorized_spells[k] )) {
				if (sorcerer & (1<<type) ) {
					DepleteLevel (sm, sm->memorized_spells[k]->SpellResRef);
				}
				return true;
			}
		}
	}
	return false;
}

void Spellbook::DepleteLevel(CRESpellMemorization* sm, const ieResRef except)
{
	size_t cnt = sm->memorized_spells.size();
	ieResRef last={""};

	for (size_t i = 0; i < cnt && cnt>0; i++) {
		CREMemorizedSpell *cms = sm->memorized_spells[i];
		//sorcerer spells are created in orderly manner
		if (cms->Flags && strncmp(last,cms->SpellResRef,8) && strncmp(except,cms->SpellResRef,8)) {
			memcpy(last, cms->SpellResRef, sizeof(ieResRef) );
			cms->Flags=0;
/*
			delete cms;
			sm->memorized_spells.erase(sm->memorized_spells.begin()+i);
			i--;
			cnt--;
*/
		}
	}
}

bool Spellbook::DepleteSpell(int type, unsigned int page, unsigned int slot)
{
	bool ret;

	if (NUM_BOOK_TYPES<=type) {
		return false;
	}
	if (spells[type].size()<=page) {
		return false;
	}
	CRESpellMemorization* sm = spells[page][type];
	if (sm->memorized_spells.size()<=slot) {
		return false;
	}

	CREMemorizedSpell* cms = sm->memorized_spells[slot];
	ret = DepleteSpell(cms);
	if (ret && (sorcerer & (1<<type) ) ) {
		DepleteLevel (sm, cms->SpellResRef);
	}

	return ret;
}

bool Spellbook::ChargeSpell(CREMemorizedSpell* spl)
{
	spl->Flags = 1;
	ClearSpellInfo();
	return true;
}

bool Spellbook::DepleteSpell(CREMemorizedSpell* spl)
{
	if (spl->Flags) {
		spl->Flags = 0;
		ClearSpellInfo();
		return true;
	}
	return false;
}

void Spellbook::ClearSpellInfo()
{
	size_t i = spellinfo.size();
	while(i--) {
		delete spellinfo[i];
	}
	spellinfo.clear();
}

bool Spellbook::GetSpellInfo(SpellExtHeader *array, int type, int startindex, int count)
{
	memset(array, 0, count * sizeof(SpellExtHeader) );
	if (spellinfo.size() == 0) {
		GenerateSpellInfo();
	}
	int actual = 0;
	bool ret = false;
	for (unsigned int i = 0; i<spellinfo.size(); i++) {
		if ( !(type & (1<<spellinfo[i]->type)) ) {
			continue;
		}
		if (startindex>0) {
			startindex--;
			continue;
		}
		if (actual>=count) {
			ret = true;
			break;
		}
		memcpy(array+actual, spellinfo[i], sizeof(SpellExtHeader));
		actual++;
	}
	return ret;
}

// returns the size of spellinfo vector, if type is nonzero it is used as filter
// for example type==1 lists the number of different mage spells
unsigned int Spellbook::GetSpellInfoSize(int type)
{
	size_t i = spellinfo.size();
	if (!i) {
		GenerateSpellInfo();
		i = spellinfo.size();
	}
	if (!type) {
		return (unsigned int) i;
	}
	unsigned int count = 0;
	while(i--) {
		if (1<<(spellinfo[i]->type)&type) {
			count++;
		}
	}
	return count;
}

int Spellbook::FindSpellInfo(SpellExtHeader *array, const ieResRef spellname, unsigned int type)
{
	memset(array, 0, sizeof(SpellExtHeader) );
	if (spellinfo.size() == 0) {
		GenerateSpellInfo();
	}
	int offset = 0;
	for (unsigned int i = 0; i<spellinfo.size(); i++) {
		// take the offset into account, since we need per-type indices
		if (!(spellinfo[i]->type & type)) {
			offset++;
			continue;
		}
		if (strnicmp(spellinfo[i]->spellname, spellname, sizeof(ieResRef) ) ) continue;
		memcpy(array, spellinfo[i], sizeof(SpellExtHeader));
		return i-offset+1;
	}
	return 0;
}

SpellExtHeader *Spellbook::FindSpellInfo(unsigned int level, unsigned int type, const ieResRef spellname)
{
	size_t i = spellinfo.size();
	while(i--) {
		if ( (spellinfo[i]->level==level) &&
			(spellinfo[i]->type==type) &&
			!strnicmp(spellinfo[i]->spellname, spellname, 8)) {
				return spellinfo[i];
		}
	}
	return NULL;
}

void Spellbook::AddSpellInfo(unsigned int sm_level, unsigned int sm_type, const ieResRef spellname, unsigned int idx)
{
	Spell *spl = gamedata->GetSpell(spellname);
	if (!spl)
		return;
	if (spl->ExtHeaderCount<1)
		return;

	ieDword level = 0;
	SpellExtHeader *seh = FindSpellInfo(sm_level, sm_type, spellname);
	if (seh) {
		seh->count++;
		return;
	}

	seh = new SpellExtHeader;
	spellinfo.push_back( seh );

	memcpy(seh->spellname, spellname, sizeof(ieResRef) );
	int ehc;

	for (ehc = 0; ehc < spl->ExtHeaderCount-1; ehc++) {
		if (level<spl->ext_headers[ehc+1].RequiredLevel) {
			break;
		}
	}

	SPLExtHeader *ext_header = spl->ext_headers+ehc;
	seh->headerindex = ehc;
	seh->level = sm_level;
	seh->type = sm_type;
	seh->slot = idx;
	seh->count = 1;
	seh->SpellForm = ext_header->SpellForm;
	memcpy(seh->MemorisedIcon, ext_header->MemorisedIcon,sizeof(ieResRef) );
	seh->Target = ext_header->Target;
	seh->TargetNumber = ext_header->TargetNumber;
	seh->Range = ext_header->Range;
	seh->Projectile = ext_header->ProjectileAnimation;
	seh->CastingTime = (ieWord) ext_header->CastingTime;
	seh->strref = spl->SpellName;
	gamedata->FreeSpell(spl, spellname, false);
}

void Spellbook::SetCustomSpellInfo(ieResRef *data, ieResRef spell, int type)
{
	ClearSpellInfo();
	if (data) {
		for(int i = 0; i<type;i++) {
			AddSpellInfo(0,0,data[i],-1);
		}
		return;
	}

	//if data is not set, use the known spells list to set up the spellinfo list
	for(int i = 0; i<NUM_BOOK_TYPES; i++) {
		if ((1<<i)&type) {
			for(unsigned int j = 0; j<spells[i].size(); j++) {
				CRESpellMemorization* sm = spells[i][j];

				for(unsigned int k=0;k<sm->known_spells.size(); k++) {
					CREKnownSpell* slot = sm->known_spells[k];
					if (!slot)
						continue;
					//skip the spell itself
					if (spell && !strnicmp(slot->SpellResRef, spell, sizeof(ieResRef)))
						continue;
					AddSpellInfo(sm->Level, sm->Type, slot->SpellResRef, -1);
				}
			}
		}
	}
}

// grouping the castable spells
void Spellbook::GenerateSpellInfo()
{
	ClearSpellInfo(); //just in case
	for (int i = 0; i < NUM_BOOK_TYPES; i++) {
		for (unsigned int j = 0; j < spells[i].size(); j++) {
			CRESpellMemorization* sm = spells[i][j];

			for (unsigned int k = 0; k < sm->memorized_spells.size(); k++) {
				CREMemorizedSpell* slot = sm->memorized_spells[k];
				if (!slot)
					continue;
				if (!slot->Flags)
					continue;
				AddSpellInfo(sm->Level, sm->Type, slot->SpellResRef, k);
			}
		}
	}
}

void Spellbook::dump()
{
	unsigned int k;

	print( "SPELLBOOK:\n" );
	for (int i = 0; i < NUM_BOOK_TYPES; i++) {
		for (unsigned int j = 0; j < spells[i].size(); j++) {
			CRESpellMemorization* sm = spells[i][j];

			if (sm->known_spells.size())
				print( " Known spells:\n" );
			for (k = 0; k < sm->known_spells.size(); k++) {
				CREKnownSpell* spl = sm->known_spells[k];
				if (!spl) continue;

				print ( " %2d: %8s L: %d T: %d\n", k, spl->SpellResRef, spl->Level, spl->Type );
			}

			if (sm->memorized_spells.size())
				print( " Memorized spells:\n" );
			for (k = 0; k < sm->memorized_spells.size (); k++) {
				CREMemorizedSpell* spl = sm->memorized_spells[k];
				if (!spl) continue;

				print ( " %2u: %8s %x\n", k, spl->SpellResRef, spl->Flags );
			}
		}
	}
}
