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
 * $Id$
 *
 */

#include <cstdio>
#include "Spellbook.h"
#include "Interface.h"
#include "Spell.h"
#include "TableMgr.h"
#include "Actor.h"
#include "Projectile.h"

static ieResRef *spelllist = NULL;
static ieResRef *innatelist = NULL;
static ieResRef *songlist = NULL;
static ieResRef *shapelist = NULL;

static bool SBInitialized = false;
static int NUM_BOOK_TYPES = 3;
static bool IWD2Style = false;

//spell header-->spell book type conversion (iwd2 is different)
static const int spelltypes[NUM_SPELL_TYPES]={
	IE_SPELL_TYPE_INNATE, IE_SPELL_TYPE_WIZARD, IE_SPELL_TYPE_PRIEST,
	IE_SPELL_TYPE_WIZARD, IE_SPELL_TYPE_INNATE, IE_SPELL_TYPE_SONG
};

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
	AutoTable tab(tableresref);
	if (!tab)
		return 0;

	int count = tab->GetRowCount();
	ieResRef *reslist = (ieResRef *) malloc (sizeof(ieResRef) * count);
	for(int i = 0; i<count;i++) {
		strnlwrcpy(reslist[i], tab->QueryField(i, column), 8);
	}
	return reslist;
}

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
			spelllist = GetSpellTable("listspll",7); //this is fucked up
			innatelist = GetSpellTable("listinnt",0);
			songlist = GetSpellTable("listsong",0);
			shapelist = GetSpellTable("listshap",0);
			NUM_BOOK_TYPES=NUM_IWD2_SPELLTYPES; //iwd2 spell types
		} else {
			NUM_BOOK_TYPES=NUM_SPELLTYPES; //bg/pst/iwd1 spell types
		}
	}
	return;
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
						DepleteSpell(ms);
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
						DepleteSpell(ms);
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
	return (unsigned int) spells[type].size();
}

unsigned int Spellbook::GetTotalPageCount() const
{
	unsigned int total = 0;
	for(int type =0; type<NUM_BOOK_TYPES; type++) {
		total += GetSpellLevelCount(type);
	}
	return total;
}

unsigned int Spellbook::GetTotalKnownSpellsCount() const
{
	unsigned int total = 0;
	for(int type =0; type<NUM_BOOK_TYPES; type++) {
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
	for(int type =0; type<NUM_BOOK_TYPES; type++) {
		unsigned int level = GetSpellLevelCount(type);
		while(level--) {
			total += GetMemorizedSpellsCount(type, level);
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
			}
		}
	}
}

//removes spell from both memorized/book
void Spellbook::RemoveSpell(const ieResRef ResRef)
{
	for(int type =0; type<NUM_BOOK_TYPES; type++) {
		std::vector< CRESpellMemorization* >::iterator sm;
		for (sm = spells[type].begin(); sm != spells[type].end(); sm++) {
			std::vector< CREKnownSpell* >::iterator ks;

			for (ks = (*sm)->known_spells.begin(); ks != (*sm)->known_spells.end(); ks++) {
				if (strnicmp(ResRef, (*ks)->SpellResRef, sizeof(ResRef) ) ) {
					continue;
				}
				delete *ks;
				(*sm)->known_spells.erase(ks);
				RemoveMemorization(*sm, ResRef);
				ks--;
			}
		}
	}
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

unsigned int Spellbook::GetMemorizedSpellsCount(int type) const
{
	unsigned int count = 0;
	size_t i=GetSpellLevelCount(type);
	while(i--) {
		count += (unsigned int) spells[type][i]->memorized_spells.size();
	}
	return count;
}

unsigned int Spellbook::GetMemorizedSpellsCount(int type, unsigned int level) const
{
	if (type >= NUM_BOOK_TYPES)
		return 0;
	if(level >= GetSpellLevelCount(type))
		return 0;
	return (unsigned int) spells[type][level]->memorized_spells.size();
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

	while (s->size() <= level ) {
		s->push_back( NULL );
	}

	if ((*s)[level]) {
		return false;
	}
	(*s)[level] = sm;
	return true;
}

//apply the wisdom bonus on all spell levels for type
//count is optimally the count of spell levels
void Spellbook::BonusSpells(int type, int count, int *bonuses)
{
	int level = GetSpellLevelCount(type);
	if (level>count) level=count;
	for(int i=0;i<level;i++) {
		CRESpellMemorization* sm = GetSpellMemorization(type, i);
		sm->Number2+=bonuses[i];
	}
}

//call this in every ai cycle when recalculating spell bonus
//TODO:add in wisdom bonus here
void Spellbook::ClearBonus()
{
	int type;

	for(type=0;type<NUM_BOOK_TYPES;type++) {
		int level = GetSpellLevelCount(type);
		for(int i=0;i<level;i++) {
			CRESpellMemorization* sm = GetSpellMemorization(type, i);
			sm->Number2=sm->Number;
		}
	}
}

CRESpellMemorization *Spellbook::GetSpellMemorization(unsigned int type, unsigned int level)
{
	CRESpellMemorization *sm;
	if (level>= GetSpellLevelCount(type) || !(sm = spells[type][level]) ) {
		sm = new CRESpellMemorization();
		sm->Type = (ieWord) type;
		sm->Level = (ieWord) level;
		sm->Number = sm->Number2 = 0;
		if ( !AddSpellMemorization(sm) ) {
			delete sm;
			return NULL;
		}
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
	if (sm->Number2 <= sm->memorized_spells.size())
		return false;

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
				if (ret->Flags==0) {
					return ret;
				}
			}
		}
	}
	return NULL;
}

void Spellbook::ChargeAllSpells()
{
	for (int i = 0; i < NUM_BOOK_TYPES; i++) {
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
				return true;
			}
		}
	}
	return false;
}

bool Spellbook::DepleteSpell(int type, unsigned int page, unsigned int slot)
{
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
	return DepleteSpell(cms);
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
	if (spellinfo.size()==0) {
		GenerateSpellInfo();
	}
	int actual = 0;
	bool ret = false;
	for (unsigned int i = 0; i<spellinfo.size(); i++) {
		if ( !(type & (1<<spellinfo[i]->type)) ) {
			continue;
		}
		if(startindex>0) {
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

SpellExtHeader *Spellbook::FindSpellInfo(unsigned int level, unsigned int type, const ieResRef spellname)
{
	size_t i = spellinfo.size();
	while(i--) {
		if( (spellinfo[i]->level==level) &&
			(spellinfo[i]->type==type) &&
			!strnicmp(spellinfo[i]->spellname, spellname, 8)) {
				return spellinfo[i];
		}
	}
	return NULL;
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
				Spell *spl = gamedata->GetSpell(slot->SpellResRef);
				if (!spl)
					continue;
				ieDword level = 0;
				SpellExtHeader *seh = FindSpellInfo(sm->Level, sm->Type, slot->SpellResRef);
				if (seh) {
					seh->count++;
					continue;
				}
				seh = new SpellExtHeader;
				spellinfo.push_back( seh );

				memcpy(seh->spellname, slot->SpellResRef, sizeof(ieResRef) );
				int ehc;

				for(ehc=0;ehc<spl->ExtHeaderCount-1;ehc++) {
					if (level<spl->ext_headers[ehc+1].RequiredLevel) {
						break;
					}
				}
				SPLExtHeader *ext_header = spl->ext_headers+ehc;
				seh->headerindex = ehc;
				seh->level = sm->Level;
				seh->type = sm->Type;
				seh->slot = k;
				seh->count = 1;
				seh->SpellForm = ext_header->SpellForm;
				memcpy(seh->MemorisedIcon, ext_header->MemorisedIcon,sizeof(ieResRef) );
				seh->Target = ext_header->Target;
				seh->TargetNumber = ext_header->TargetNumber;
				seh->Range = ext_header->Range;
				seh->Projectile = ext_header->ProjectileAnimation;
				seh->CastingTime = (ieWord) ext_header->CastingTime;
				seh->strref = spl->SpellName;
				gamedata->FreeSpell(spl, slot->SpellResRef, false);
			}
		}
	}
}

void Spellbook::dump()
{
	unsigned int k;

	printf( "SPELLBOOK:\n" );
	for (int i = 0; i < NUM_BOOK_TYPES; i++) {
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
