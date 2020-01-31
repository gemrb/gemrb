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

#include "ActorMgr.h"
#include "GameData.h"
#include "Interface.h"
#include "PluginMgr.h"
#include "Projectile.h"
#include "Spell.h"
#include "TableMgr.h"
#include "Scriptable/Actor.h"
#include "System/StringBuffer.h"

#include <cstdio>

namespace GemRB {

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
	sorcerer = 0;
	if (IWD2Style) {
		innate = 1<<IE_IWD2_SPELL_INNATE;
	} else {
		innate = 1<<IE_SPELL_TYPE_INNATE;
	}
}

void Spellbook::InitializeSpellbook()
{
	if (!SBInitialized) {
		SBInitialized=true;
		if (core->HasFeature(GF_HAS_SPELLLIST)) {
			NUM_BOOK_TYPES=NUM_IWD2_SPELLTYPES; //iwd2 spell types
			IWD2Style = true;
		} else {
			NUM_BOOK_TYPES=NUM_SPELLTYPES; //bg/pst/iwd1 spell types
			IWD2Style = false;
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
			sm->SlotCount = wm->SlotCount;
			sm->SlotCountWithBonus = wm->SlotCountWithBonus;
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
// domain spells are of all types, so look them up in all cases
// ignore songs and shapes altogether
int arcanetypes[] = {IE_IWD2_SPELL_BARD, IE_IWD2_SPELL_SORCERER, IE_IWD2_SPELL_WIZARD, IE_IWD2_SPELL_DOMAIN};
int divinetypes[] = {IE_IWD2_SPELL_CLERIC, IE_IWD2_SPELL_DRUID, IE_IWD2_SPELL_PALADIN, IE_IWD2_SPELL_RANGER, IE_IWD2_SPELL_DOMAIN};
int *alltypes[2] = {divinetypes, arcanetypes};

int inline GetType(int spellid, unsigned int &bookcount, int &idx)
{
	int type = spellid/1000;
	if (type>4) {
		return -1;
	}
	if (IWD2Style) {
		if (type == 1) {
			// check divine
			idx = 0;
			bookcount = sizeof(divinetypes)/sizeof(int);
		} else if (type == 2) {
			// check arcane
			idx = 1;
			bookcount = sizeof(arcanetypes)/sizeof(int);
		} else if (type == 3) {
			type = IE_IWD2_SPELL_INNATE;
		}
	} else {
		type = sections[type];
		if (type >= NUM_BOOK_TYPES) {
			return -1;
		}
	}
	return type;
}

//flags bits
// 1 - unmemorize it
bool Spellbook::HaveSpell(int spellid, ieDword flags)
{
	int idx = -1;
	unsigned int bookcount;
	int type = GetType(spellid, bookcount, idx);
	if (type == -1) {
		return false;
	}

	spellid = spellid % 1000;

	if (idx == -1) {
		return HaveSpell(spellid, type, flags);
	} else {
		for (unsigned int book = 0; book < bookcount; book++) {
			if (HaveSpell(spellid, alltypes[idx][book], flags)) {
				return true;
			}
		}
	}
	return false;
}
bool Spellbook::HaveSpell(int spellid, int type, ieDword flags)
{
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

//returns count of memorized spells of a given name/type
int Spellbook::CountSpells(const char *resref, unsigned int type, int flag) const
{
	int i, max;
	int count = 0;

	if (type==0xffffffff) {
		i=0;
		max = NUM_BOOK_TYPES;
	} else {
		i = type;
		max = i+1;
	}

	while(i < max) {
		for (unsigned int j = 0; j < spells[i].size(); j++) {
			CRESpellMemorization* sm = spells[i][j];
			for (unsigned int k = 0; k < sm->memorized_spells.size(); k++) {
				CREMemorizedSpell* ms = sm->memorized_spells[k];
				if (resref[0] && !stricmp(ms->SpellResRef, resref) ) {
					if (flag || ms->Flags) {
						count++;
					}
				}
			}
		}
		i++;
	}
	return count;
}

bool Spellbook::KnowSpell(int spellid) const
{
	int idx = -1;
	unsigned int bookcount;
	int type = GetType(spellid, bookcount, idx);
	if (type == -1) {
		return false;
	}
	spellid = spellid % 1000;

	if (idx == -1) {
		return KnowSpell(spellid, type);
	} else {
		for (unsigned int book = 0; book < bookcount; book++) {
			if (KnowSpell(spellid, alltypes[idx][book])) {
				return true;
			}
		}
	}
	return false;
}

bool Spellbook::KnowSpell(int spellid, int type) const
{
	for (unsigned int j = 0; j < GetSpellLevelCount(type); j++) {
		CRESpellMemorization* sm = spells[type][j];
		for (unsigned int k = 0; k < sm->known_spells.size(); k++) {
			CREKnownSpell* ks = sm->known_spells[k];
			if (atoi(ks->SpellResRef+4)==spellid) {
				return true;
			}
		}
	}
	return false;
}

//if resref=="" then it is a knownanyspell
bool Spellbook::KnowSpell(const char *resref) const
{
	for (int i = 0; i < NUM_BOOK_TYPES; i++) {
		for (unsigned int j = 0; j < spells[i].size(); j++) {
			CRESpellMemorization* sm = spells[i][j];
			for (unsigned int k = 0; k < sm->known_spells.size(); k++) {
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

				if (!ms->Flags) continue;
				if (resref[0] && stricmp(ms->SpellResRef, resref)) {
					continue;
				}

				if (flags&HS_DEPLETE) {
					if (DepleteSpell(ms) && (sorcerer & (1<<i))) {
						DepleteLevel(sm, ms->SpellResRef);
					}
				}
				return true;
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

	for (ms = sm->memorized_spells.begin(); ms != sm->memorized_spells.end(); ++ms) {
		if (strnicmp(ResRef, (*ms)->SpellResRef, sizeof(ieResRef) ) ) {
			continue;
		}
		delete *ms;
		ms = sm->memorized_spells.erase(ms);
		--ms;
	}
}

//removes one instance of spell (from creknownspell)
bool Spellbook::RemoveSpell(CREKnownSpell* spell)
{
	for (int i = 0; i < NUM_BOOK_TYPES; i++) {
		std::vector< CRESpellMemorization* >::iterator sm;
		for (sm = spells[i].begin(); sm != spells[i].end(); ++sm) {
			std::vector< CREKnownSpell* >::iterator ks;
			for (ks = (*sm)->known_spells.begin(); ks != (*sm)->known_spells.end(); ++ks) {
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
	int idx = -1;
	unsigned int bookcount;
	int type = GetType(spellid, bookcount, idx);
	if (type == -1) {
		return;
	}
	spellid = spellid % 1000;

	if (idx == -1) {
		RemoveSpell(spellid, type);
	} else {
		for (unsigned int book = 0; book < bookcount; book++) {
			RemoveSpell(spellid, alltypes[idx][book]);
		}
	}

}

void Spellbook::RemoveSpell(int spellid, int type)
{
	std::vector< CRESpellMemorization* >::iterator sm;
	for (sm = spells[type].begin(); sm != spells[type].end(); ++sm) {
		std::vector< CREKnownSpell* >::iterator ks;

		for (ks = (*sm)->known_spells.begin(); ks != (*sm)->known_spells.end(); ++ks) {
			if (atoi((*ks)->SpellResRef+4)==spellid) {
				ieResRef ResRef;

				memcpy(ResRef, (*ks)->SpellResRef, sizeof(ieResRef) );
				delete *ks;
				ks = (*sm)->known_spells.erase(ks);
				RemoveMemorization(*sm, ResRef);
				--ks;
				ClearSpellInfo();
			}
		}
	}
}

//removes spell from both memorized/book
void Spellbook::RemoveSpell(const ieResRef ResRef, bool onlyknown)
{
	for (int type = 0; type<NUM_BOOK_TYPES; type++) {
		std::vector< CRESpellMemorization* >::iterator sm;
		for (sm = spells[type].begin(); sm != spells[type].end(); ++sm) {
			std::vector< CREKnownSpell* >::iterator ks;

			for (ks = (*sm)->known_spells.begin(); ks != (*sm)->known_spells.end(); ++ks) {
				if (strnicmp(ResRef, (*ks)->SpellResRef, sizeof(ieResRef) ) ) {
					continue;
				}
				delete *ks;
				ks = (*sm)->known_spells.erase(ks);
				if (!onlyknown) RemoveMemorization(*sm, ResRef);
				--ks;
				ClearSpellInfo();
			}
		}
	}
}

void Spellbook::SetBookType(int bt)
{
	sorcerer |= bt;
}

//returns the page group of the spellbook this spelltype belongs to
//psionics are stored in the mage spell list
//wizard/priest are trivial
//songs are stored elsewhere
//wildshapes are marked as innate, they need some hack to get stored
//in the right group
//the rest are stored as innate

int Spellbook::LearnSpell(Spell *spell, int memo, unsigned int clsmsk, unsigned int kit, int level)
{
	CREKnownSpell *spl = new CREKnownSpell();
	CopyResRef(spl->SpellResRef, spell->Name);
	spl->Level = 0;
	if (IWD2Style) {
		PluginHolder<ActorMgr> gm(IE_CRE_CLASS_ID);
		// is there an override (domain spells)?
		if (level == -1) {
			level = spell->SpellLevel-1;
		}
		spl->Level = level;
		spl->Type = gm->FindSpellType(spell->Name, spl->Level, clsmsk, kit);
	} else {
		//not IWD2
		if (spell->SpellType<6) {
			spl->Type = spelltypes[spell->SpellType];
			if (spell->SpellLevel == 0) { // totemic druid has some broken innates (fixed by fixpack)
				spell->SpellLevel = 1;
			}
			spl->Level = spell->SpellLevel-1;
		} else {
			spl->Type = IE_SPELL_TYPE_INNATE;
		}
	}

	bool ret=AddKnownSpell(spl, memo);
	if (!ret) {
		delete spl;
		return 0;
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
		sm->SlotCount = sm->SlotCountWithBonus = 0;
		if ( !AddSpellMemorization(sm) ) {
			delete sm;
			return false;
		}
	}

	spells[type][level]->known_spells.push_back(spl);
	if (1<<type == innate || 1<<type == 1<<IE_IWD2_SPELL_SONG) {
		spells[type][level]->SlotCount++;
		spells[type][level]->SlotCountWithBonus++;
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
		newsm->SlotCount = newsm->SlotCountWithBonus = 0;
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
		// don't give access to new spell levels through these boni
		if (sm->SlotCountWithBonus) {
			sm->SlotCountWithBonus+=bonuses[i];
		}
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
			sm->SlotCountWithBonus=sm->SlotCount;
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
		sm->SlotCount = sm->SlotCountWithBonus = 0;
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
			Value=sm->SlotCountWithBonus;
		}
                //if can't cast w/o bonus then can't cast at all!
                if (sm->SlotCount)
                    sm->SlotCountWithBonus=(ieWord) (sm->SlotCountWithBonus+Value);
	}
	else {
		diff=sm->SlotCountWithBonus-sm->SlotCount;
		sm->SlotCount=(ieWord) Value;
		sm->SlotCountWithBonus=(ieWord) (Value+diff);
	}
}

int Spellbook::GetMemorizableSpellsCount(int type, unsigned int level, bool bonus) const
{
	if (type >= NUM_BOOK_TYPES || level >= GetSpellLevelCount(type))
		return 0;
	CRESpellMemorization* sm = spells[type][level];
	if (bonus)
		return sm->SlotCountWithBonus;
	return sm->SlotCount;
}

bool Spellbook::MemorizeSpell(CREKnownSpell* spell, bool usable)
{
	ieWord spellType = spell->Type;
	CRESpellMemorization* sm = spells[spellType][spell->Level];
	if (sm->SlotCountWithBonus <= sm->memorized_spells.size() && !(innate & (1<<spellType))) {
		//it is possible to have sorcerer type spellbooks for any spellbook type
		if (! (sorcerer & (1<<spellType) ) )
			return false;
	}

	CREMemorizedSpell* mem_spl = new CREMemorizedSpell();
	CopyResRef( mem_spl->SpellResRef, spell->SpellResRef );
	mem_spl->Flags = usable ? 1 : 0; // FIXME: is it all it's used for?

	sm->memorized_spells.push_back( mem_spl );
	ClearSpellInfo();
	return true;
}

bool Spellbook::UnmemorizeSpell(CREMemorizedSpell* spell)
{
	for (int i = 0; i < NUM_BOOK_TYPES; i++) {
		std::vector< CRESpellMemorization* >::iterator sm;
		for (sm = spells[i].begin(); sm != spells[i].end(); ++sm) {
			std::vector< CREMemorizedSpell* >::iterator s;
			for (s = (*sm)->memorized_spells.begin(); s != (*sm)->memorized_spells.end(); ++s) {
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

bool Spellbook::UnmemorizeSpell(const ieResRef ResRef, bool deplete, bool onlydepleted)
{
	for (int type = 0; type<NUM_BOOK_TYPES; type++) {
		std::vector< CRESpellMemorization* >::iterator sm;
		for (sm = spells[type].begin(); sm != spells[type].end(); ++sm) {
			std::vector< CREMemorizedSpell* >::iterator s;
			for (s = (*sm)->memorized_spells.begin(); s != (*sm)->memorized_spells.end(); ++s) {
				if (strnicmp(ResRef, (*s)->SpellResRef, sizeof(ieResRef) ) ) {
					continue;
				}
				if (onlydepleted && (*s)->Flags != 0) {
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
CREMemorizedSpell* Spellbook::FindUnchargedSpell(int type, int level) const
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
			cnt = sm->SlotCountWithBonus;
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

//type = 0 means any type
int Spellbook::FindSpellInfo(SpellExtHeader *array, const ieResRef spellname, unsigned int type)
{
	memset(array, 0, sizeof(SpellExtHeader) );
	if (spellinfo.size() == 0) {
		GenerateSpellInfo();
	}
	int offset = 0;
	for (unsigned int i = 0; i<spellinfo.size(); i++) {
		// take the offset into account, since we need per-type indices
		if (type && !(1<<(spellinfo[i]->type) & type)) {
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
	Spell *spl = gamedata->GetSpell(spellname, true);
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

void Spellbook::dump() const
{
	StringBuffer buffer;
	dump(buffer);
	Log(DEBUG, "Spellbook", buffer);
}

void Spellbook::dump(StringBuffer& buffer) const
{
	unsigned int k;

	buffer.append( "SPELLBOOK:\n" );
	for (int i = 0; i < NUM_BOOK_TYPES; i++) {
		for (unsigned int j = 0; j < spells[i].size(); j++) {
			CRESpellMemorization* sm = spells[i][j];

			if (sm->known_spells.size())
				buffer.append( " Known spells:\n" );
			for (k = 0; k < sm->known_spells.size(); k++) {
				CREKnownSpell* spl = sm->known_spells[k];
				if (!spl) continue;

				buffer.appendFormatted ( " %2d: %8s L: %d T: %d\n", k, spl->SpellResRef, spl->Level, spl->Type );
			}

			if (sm->memorized_spells.size())
				buffer.append( " Memorized spells:\n" );
			for (k = 0; k < sm->memorized_spells.size (); k++) {
				CREMemorizedSpell* spl = sm->memorized_spells[k];
				if (!spl) continue;

				buffer.appendFormatted ( " %2u: %8s %x\n", k, spl->SpellResRef, spl->Flags );
			}
		}
	}
}

}
