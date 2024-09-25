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

#include <cstdio>

namespace GemRB {

static bool SBInitialized = false;
static int NUM_BOOK_TYPES = 3;
static bool IWD2Style = false;

//spell header-->spell book type conversion (iwd2 is different)
static const ieWord spelltypes[NUM_SPELL_TYPES] = {
	IE_SPELL_TYPE_INNATE, IE_SPELL_TYPE_WIZARD, IE_SPELL_TYPE_PRIEST,
	IE_SPELL_TYPE_WIZARD, IE_SPELL_TYPE_INNATE, IE_SPELL_TYPE_SONG
};

Spellbook::Spellbook()
{
	if (!SBInitialized) {
		InitializeSpellbook();
	}
	spells = new std::vector<CRESpellMemorization*> [NUM_BOOK_TYPES];

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
		if (core->HasFeature(GFFlags::HAS_SPELLLIST)) {
			NUM_BOOK_TYPES=NUM_IWD2_SPELLTYPES; //iwd2 spell types
			IWD2Style = true;
		} else {
			NUM_BOOK_TYPES=NUM_SPELLTYPES; //bg/pst/iwd1 spell types
			if (core->HasFeature(GFFlags::IWD_MAP_DIMENSIONS)) NUM_BOOK_TYPES++; // make iwd songs full members
			IWD2Style = false;
		}
	}
}

void Spellbook::ReleaseMemory()
{
	SBInitialized=false;
}

Spellbook::~Spellbook()
{
	for (int i = 0; i < NUM_BOOK_TYPES; i++) {
		for (auto& page : spells[i]) {
			if (page) {
				FreeSpellPage(page);
				page = nullptr;
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

// TODO: exclude slayer, pocket plane, perhaps also bhaal innates?
void Spellbook::CopyFrom(const Actor *source)
{
	if (!source) {
		return;
	}

	// clear it first
	for (int i = 0; i < NUM_BOOK_TYPES; i++) {
		for (auto& page : spells[i]) {
			if (page) {
				FreeSpellPage(page);
				page = nullptr;
			}
		}
		spells[i].clear();
	}
	ClearSpellInfo();

	const Spellbook &wikipedia = source->spellbook;

	for (int t = 0; t < NUM_BOOK_TYPES; t++) {
		for (const auto& wm: wikipedia.spells[t]) {
			unsigned int k;
			CRESpellMemorization *sm = new CRESpellMemorization();
			spells[t].push_back(sm);
			sm->Level = wm->Level;
			sm->SlotCount = wm->SlotCount;
			sm->SlotCountWithBonus = wm->SlotCountWithBonus;
			sm->Type = wm->Type;
			for (k = 0; k < wm->known_spells.size(); k++) {
				CREKnownSpell *tmp_known = new CREKnownSpell();
				sm->known_spells.push_back(tmp_known);
				*tmp_known = *wm->known_spells[k];
			}
			for (k = 0; k < wm->memorized_spells.size(); k++) {
				CREMemorizedSpell *tmp_mem = new CREMemorizedSpell();
				sm->memorized_spells.push_back(tmp_mem);
				*tmp_mem = *wm->memorized_spells[k];
			}
		}
	}

	sorcerer = wikipedia.sorcerer;
}

//ITEM, SPPR, SPWI, SPIN, SPCL
const int sections[] = { 3, 0, 1, 2, 2 };
// domain spells are of all types, so look them up in all cases
// ignore songs and shapes altogether
const int arcanetypes[] = { IE_IWD2_SPELL_BARD, IE_IWD2_SPELL_SORCERER, IE_IWD2_SPELL_WIZARD, IE_IWD2_SPELL_DOMAIN };
const int divinetypes[] = { IE_IWD2_SPELL_CLERIC, IE_IWD2_SPELL_DRUID, IE_IWD2_SPELL_PALADIN, IE_IWD2_SPELL_RANGER, IE_IWD2_SPELL_DOMAIN };
const int* const alltypes[2] = { divinetypes, arcanetypes };

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
	unsigned int count = GetSpellLevelCount(type);
	for (unsigned int j = 0; j < count; j++) {
		const CRESpellMemorization* sm = spells[type][j];
		for (auto& ms : sm->memorized_spells) {
			if (!ms->Flags) continue;
			if (atoi(ms->SpellResRef.c_str() + 4) != spellid) continue;

			if (!(flags & HS_DEPLETE)) return true;

			if (DepleteSpell(ms) && (sorcerer & (1 << type))) {
				DepleteLevel(sm, ms->SpellResRef);
			}
			return true;
		}
	}
	return false;
}

//returns count of memorized spells of a given name/type
int Spellbook::CountSpells(const ResRef& resref, unsigned int type, int flag) const
{
	int i = type;
	int max = i + 1;
	int count = 0;

	if (resref.IsEmpty()) {
		return 0;
	}

	if (type == 0xffffffff) {
		i = 0;
		max = NUM_BOOK_TYPES;
	}

	while(i < max) {
		for (const auto& spellMemo : spells[i]) {
			for (const auto& spell : spellMemo->memorized_spells) {
				if (spell->SpellResRef != resref) continue;

				if (flag || spell->Flags) {
					count++;
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
		const CRESpellMemorization* sm = spells[type][j];
		for (const auto& knownSpell : sm->known_spells) {
			if (atoi(knownSpell->SpellResRef.c_str() + 4) == spellid) {
				return true;
			}
		}
	}
	return false;
}

//if resref=="" then it is a knownanyspell
bool Spellbook::KnowSpell(const ResRef& resref) const
{
	for (int i = 0; i < NUM_BOOK_TYPES; i++) {
		for (const auto& spellMemo : spells[i]) {
			for (const auto& knownSpell : spellMemo->known_spells) {
				if (knownSpell->SpellResRef != resref) {
					continue;
				}
				return true;
			}
		}
	}
	return false;
}

//if resref=="" then it is a haveanyspell
bool Spellbook::HaveSpell(const ResRef &resref, ieDword flags)
{
	for (int i = 0; i < NUM_BOOK_TYPES; i++) {
		for (auto& sm : spells[i]) {
			for (const auto& ms : sm->memorized_spells) {
				if (!ms->Flags) continue;
				if (ms->SpellResRef != resref) {
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
void Spellbook::RemoveMemorization(CRESpellMemorization* sm, const ResRef& resRef)
{
	for (auto ms = sm->memorized_spells.begin(); ms != sm->memorized_spells.end(); ) {
		if (resRef != (*ms)->SpellResRef) {
			++ms;
			continue;
		}
		delete *ms;
		ms = sm->memorized_spells.erase(ms);
	}
}

//removes one instance of spell (from creknownspell)
bool Spellbook::RemoveSpell(const CREKnownSpell* spell)
{
	for (int i = 0; i < NUM_BOOK_TYPES; i++) {
		for (const auto& spellMemo : spells[i]) {
			for (auto ks = spellMemo->known_spells.begin(); ks != spellMemo->known_spells.end(); ++ks) {
				if (*ks == spell) {
					ResRef resRef = (*ks)->SpellResRef;
					delete *ks;
					spellMemo->known_spells.erase(ks);
					RemoveMemorization(spellMemo, resRef);
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
	for (const auto& spellMemo : spells[type]) {
		for (auto ks = spellMemo->known_spells.begin(); ks != spellMemo->known_spells.end(); ++ks) {
			if (atoi((*ks)->SpellResRef.c_str() + 4) == spellid) {
				ResRef resRef = (*ks)->SpellResRef;
				delete *ks;
				ks = spellMemo->known_spells.erase(ks);
				RemoveMemorization(spellMemo, resRef);
				--ks;
				ClearSpellInfo();
			}
		}
	}
}

//removes spell from both memorized/book
void Spellbook::RemoveSpell(const ResRef& resRef, bool onlyknown)
{
	for (int type = 0; type<NUM_BOOK_TYPES; type++) {
		for (const auto& spellMemo : spells[type]) {
			for (auto ks = spellMemo->known_spells.begin(); ks != spellMemo->known_spells.end(); ) {
				if ((*ks)->SpellResRef != resRef) {
					++ks;
					continue;
				}
				delete *ks;
				ks = spellMemo->known_spells.erase(ks);
				if (!onlyknown) RemoveMemorization(spellMemo, resRef);
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
	spl->SpellResRef = spell->Name;
	spl->Level = 0;
	if (IWD2Style) {
		auto gm = GetImporter<ActorMgr>(IE_CRE_CLASS_ID);
		// is there an override (domain spells)?
		if (level == -1) {
			level = spell->SpellLevel-1;
		}
		spl->Level = static_cast<ieWord>(level);
		spl->Type = gm->FindSpellType(spell->Name, spl->Level, clsmsk, kit);
	} else {
		//not IWD2
		if (spell->SpellType<6) {
			spl->Type = spelltypes[spell->SpellType];
			if (spell->SpellLevel == 0) { // totemic druid has some broken innates (fixed by fixpack)
				spell->SpellLevel = 1;
			}
			spl->Level = static_cast<ieWord>(spell->SpellLevel - 1);
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
	if (1 << type == innate || type == IE_IWD2_SPELL_SONG || type == IE_SPELL_TYPE_SONG) {
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
	for (const auto& spellMemo : spells[type]) {
		if (!real) {
			count += (unsigned int) spellMemo->memorized_spells.size();
			continue;
		}

		for (const auto& memorized : spellMemo->memorized_spells) {
			if (memorized->Flags) count++;
		}
	}
	return count;
}

unsigned int Spellbook::GetMemorizedSpellsCount(int type, unsigned int level, bool real) const
{
	if (type >= NUM_BOOK_TYPES) return 0;
	if (level >= GetSpellLevelCount(type)) return 0;

	if (real) {
		unsigned int count = 0;
		for (const auto& memorized : spells[type][level]->memorized_spells) {
			if (memorized->Flags) count++;
		}
		return count;
	}
	return (unsigned int) spells[type][level]->memorized_spells.size();
}

unsigned int Spellbook::GetMemorizedSpellsCount(const ResRef& name, int type, bool real) const
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
	while (t >= 0) {
		for (const auto& spellMemo : spells[t]) {
			for (const auto& cms : spellMemo->memorized_spells) {
				if (cms->SpellResRef != name) continue;
				if (!real || cms->Flags) j++;
			}
		}
		if (type >= 0) break;
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

// apply the (wisdom) bonus on all spell levels for type
void Spellbook::BonusSpells(int type, int abilityLevel)
{
	const auto& bonuses = gamedata->GetBonusSpells(abilityLevel);
	if (bonuses.empty() || bonuses[0] == 0) return;

	unsigned int level = GetSpellLevelCount(type);
	assert(level <= bonuses.size());
	for (unsigned int i = 0; i < level; i++) {
		CRESpellMemorization* sm = GetSpellMemorization(type, i);
		// don't give access to new spell levels through these boni
		if (sm->SlotCountWithBonus) {
			sm->SlotCountWithBonus += bonuses[i];
		}
	}
}

//call this in every ai cycle when recalculating spell bonus
//TODO:add in wisdom bonus here
void Spellbook::ClearBonus()
{
	for (int type = 0; type < NUM_BOOK_TYPES; type++) {
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
//bonus is cumulative, but not saved
void Spellbook::SetMemorizableSpellsCount(int Value, int type, unsigned int level, bool bonus)
{
	if (type >= NUM_BOOK_TYPES) {
		return;
	}

	CRESpellMemorization* sm = GetSpellMemorization(type, level);
	if (bonus) {
		if (!Value) {
			Value=sm->SlotCountWithBonus;
		}
		// if can't cast w/o a bonus then can't cast at all!
		if (sm->SlotCount) {
			sm->SlotCountWithBonus = (ieWord) (sm->SlotCountWithBonus + Value);
		}
	} else {
		int oldBonus = sm->SlotCountWithBonus - sm->SlotCount;
		sm->SlotCount = (ieWord) Value;
		sm->SlotCountWithBonus = (ieWord) (Value + oldBonus);
	}
}

int Spellbook::GetMemorizableSpellsCount(int type, unsigned int level, bool bonus) const
{
	if (type >= NUM_BOOK_TYPES || level >= GetSpellLevelCount(type))
		return 0;
	const CRESpellMemorization* sm = spells[type][level];
	if (bonus)
		return sm->SlotCountWithBonus;
	return sm->SlotCount;
}

bool Spellbook::MemorizeSpell(const CREKnownSpell* spell, bool usable)
{
	ieWord spellType = spell->Type;
	CRESpellMemorization* sm = spells[spellType][spell->Level];
	if (sm->SlotCountWithBonus <= sm->memorized_spells.size() && !(innate & (1<<spellType))) {
		//it is possible to have sorcerer type spellbooks for any spellbook type
		if (! (sorcerer & (1<<spellType) ) )
			return false;
	}

	CREMemorizedSpell* mem_spl = new CREMemorizedSpell();
	mem_spl->SpellResRef = spell->SpellResRef;
	mem_spl->Flags = usable ? 1 : 0; // FIXME: is it all it's used for?

	sm->memorized_spells.push_back( mem_spl );
	ClearSpellInfo();
	return true;
}

bool Spellbook::UnmemorizeSpell(const CREMemorizedSpell* spell)
{
	for (int i = 0; i < NUM_BOOK_TYPES; i++) {
		for (const auto& spellMemo : spells[i]) {
			for (auto s = spellMemo->memorized_spells.begin(); s != spellMemo->memorized_spells.end(); ++s) {
				if (*s == spell) {
					delete *s;
					spellMemo->memorized_spells.erase(s);
					ClearSpellInfo();
					return true;
				}
			}
		}
	}

	return false;
}

bool Spellbook::UnmemorizeSpell(const ResRef& spellRef, bool deplete, uint8_t flags)
{
	for (int type = 0; type<NUM_BOOK_TYPES; type++) {
		for (const auto& sm : spells[type]) {
			for (auto s = sm->memorized_spells.begin(); s != sm->memorized_spells.end(); ++s) {
				if (spellRef != (*s)->SpellResRef) {
					continue;
				}
				// only depleted?
				if (flags == 1 && (*s)->Flags != 0) {
					continue;
				}
				// only non-depleted?
				if (flags == 2 && (*s)->Flags == 0) {
					continue;
				}

				if (deplete) {
					(*s)->Flags = 0;
				} else {
					delete *s;
					sm->memorized_spells.erase(s);
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
		for (const auto& spellMemo : spells[i]) {
			if (level && (spellMemo->Level != level - 1)) {
				continue;
			}

			for (auto spell : spellMemo->memorized_spells) {
				if (spell->Flags != 0) continue;
				return spell;
			}
		}
	}
	return NULL;
}

//creates sorcerer style memory for the given spell type
void Spellbook::CreateSorcererMemory(int type)
{
	for (auto spellMemo : spells[type]) {
		size_t cnt = spellMemo->memorized_spells.size();
		while(cnt--) {
			delete spellMemo->memorized_spells[cnt];
		}
		spellMemo->memorized_spells.clear();
		for (const auto& ck : spellMemo->known_spells) {
			cnt = spellMemo->SlotCountWithBonus;
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

		for (const auto& spellMemo : spells[i]) {
			for (auto& memorizedSpell : spellMemo->memorized_spells) {
				ChargeSpell(memorizedSpell);
			}
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
		const CRESpellMemorization* sm = spells[type][j];

		for (auto& spell : sm->memorized_spells) {
			if (!DepleteSpell(spell)) continue;

			if (sorcerer & (1 << type)) {
				DepleteLevel(sm, spell->SpellResRef);
			}
			return true;
		}
	}
	return false;
}

void Spellbook::DepleteLevel(const CRESpellMemorization* sm, const ResRef& except) const
{
	ResRef last;

	for (auto& cms : sm->memorized_spells) {
		//sorcerer spells are created in orderly manner
		if (cms->Flags && last != cms->SpellResRef && except != cms->SpellResRef) {
			last = cms->SpellResRef;
			cms->Flags=0;
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
	if (spellinfo.empty()) {
		GenerateSpellInfo();
	}
	int actual = 0;
	bool ret = false;
	for (const auto& extHeader : spellinfo) {
		if (!(type & (1 << extHeader->type))) {
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
		*(array + actual) = *extHeader;
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
int Spellbook::FindSpellInfo(SpellExtHeader *array, const ResRef& spellName, unsigned int type)
{
	if (spellinfo.empty()) {
		GenerateSpellInfo();
	}
	int offset = 0;
	for (unsigned int i = 0; i<spellinfo.size(); i++) {
		// take the offset into account, since we need per-type indices
		if (type && !(1<<(spellinfo[i]->type) & type)) {
			offset++;
			continue;
		}
		if (spellName != spellinfo[i]->spellName) continue;
		*array = *spellinfo[i];
		return i-offset+1;
	}
	return 0;
}

SpellExtHeader *Spellbook::FindSpellInfo(unsigned int level, unsigned int type, const ResRef& spellname) const
{
	size_t i = spellinfo.size();
	while (i--) {
		if (spellinfo[i]->level != level) continue;
		if (spellinfo[i]->type != type) continue;
		if (spellinfo[i]->spellName == spellname) {
			return spellinfo[i];
		}
	}
	return nullptr;
}

void Spellbook::AddSpellInfo(unsigned int sm_level, unsigned int sm_type, const ResRef& spellname, unsigned int idx)
{
	const Spell *spl = gamedata->GetSpell(spellname, true);
	if (!spl)
		return;
	if (spl->ext_headers.size() < 1)
		return;

	ieDword level = 0;
	SpellExtHeader *seh = FindSpellInfo(sm_level, sm_type, spellname);
	if (seh) {
		seh->count++;
		return;
	}

	seh = new SpellExtHeader;
	spellinfo.push_back( seh );

	seh->spellName = spellname;
	size_t ehc = 0;

	for (; ehc < spl->ext_headers.size() - 1; ++ehc) {
		if (level<spl->ext_headers[ehc+1].RequiredLevel) {
			break;
		}
	}

	const SPLExtHeader *ext_header = &spl->ext_headers[ehc];
	seh->headerindex = ehc;
	seh->level = sm_level;
	seh->type = sm_type;
	seh->slot = idx;
	seh->count = 1;
	seh->SpellForm = ext_header->SpellForm;
	seh->memorisedIcon = ext_header->memorisedIcon;
	seh->Target = ext_header->Target;
	seh->TargetNumber = ext_header->TargetNumber;
	seh->Range = ext_header->Range;
	seh->Projectile = ext_header->ProjectileAnimation;
	seh->CastingTime = (ieWord) ext_header->CastingTime;
	seh->strref = spl->SpellName;
	gamedata->FreeSpell(spl, spellname, false);
}

void Spellbook::SetCustomSpellInfo(const std::vector<ResRef>& data, const ResRef &spell, int type)
{
	ClearSpellInfo();
	if (!data.empty()) {
		for (const auto& datum : data) {
			AddSpellInfo(0, 0, datum, -1);
		}
		return;
	}

	//if data is not set, use the known spells list to set up the spellinfo list
	for(int i = 0; i<NUM_BOOK_TYPES; i++) {
		if (!((1 << i) & type)) continue;

		for (const auto& spellMemo : spells[i]) {
			for (const auto& slot : spellMemo->known_spells) {
				if (!slot) continue;
				// skip the spell itself
				if (slot->SpellResRef == spell) continue;

				AddSpellInfo(spellMemo->Level, spellMemo->Type, slot->SpellResRef, -1);
			}
		}
	}
}

// grouping the castable spells
void Spellbook::GenerateSpellInfo()
{
	ClearSpellInfo(); //just in case
	for (int i = 0; i < NUM_BOOK_TYPES; i++) {
		for (const auto& spellMemo : spells[i]) {
			for (unsigned int k = 0; k < spellMemo->memorized_spells.size(); k++) {
				const CREMemorizedSpell* slot = spellMemo->memorized_spells[k];
				if (!slot)
					continue;
				if (!slot->Flags)
					continue;
				AddSpellInfo(spellMemo->Level, spellMemo->Type, slot->SpellResRef, k);
			}
		}
	}
}

std::string Spellbook::dump(bool print) const
{
	std::string buffer;
	buffer.append( "SPELLBOOK:\n" );
	for (int i = 0; i < NUM_BOOK_TYPES; i++) {
		for (const auto& spellMemo : spells[i]) {
			if (!spellMemo->known_spells.empty()) {
				buffer.append(" Known spells:\n");
			}
			int idx = 0;
			for (const auto& knownSpell : spellMemo->known_spells) {
				if (!knownSpell) continue;
				AppendFormat(buffer, " {:2d}: {} L: {} T: {}\n", idx, knownSpell->SpellResRef, knownSpell->Level, knownSpell->Type);
				idx++;
			}

			if (!spellMemo->memorized_spells.empty())
				buffer.append( " Memorized spells:\n" );
			idx = 0;
			for (const auto& memorizedSpell : spellMemo->memorized_spells) {
				if (!memorizedSpell) continue;
				AppendFormat(buffer, " {:2d}: {} {:#x}\n", idx, memorizedSpell->SpellResRef, memorizedSpell->Flags);
				idx++;
			}
		}
	}
	if (print) Log(DEBUG, "Spellbook", "{}", buffer);
	return buffer;
}

}
