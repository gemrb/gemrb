/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "CREImporter.h"

#include "ie_stats.h"
#include "win32def.h"

#include "EffectMgr.h"
#include "GameData.h"
#include "Interface.h"
#include "PluginMgr.h"
#include "TableMgr.h"
#include "GameScript/GameScript.h"

#include <cassert>

using namespace GemRB;

#define MAXCOLOR 12
typedef unsigned char ColorSet[MAXCOLOR];

static int RandColor=-1;
static int RandRows=-1;
static ColorSet* randcolors=NULL;
static int MagicBit;

static void Initializer()
{
	MagicBit = core->HasFeature(GF_MAGICBIT);
}

//one column, these don't have a level
static ieResRef* innlist;   //IE_IWD2_SPELL_INNATE
static int inncount=-1;     
static ieResRef* snglist;   //IE_IWD2_SPELL_SONG
static int sngcount=-1;
static ieResRef* shplist;   //IE_IWD2_SPELL_SHAPE
static int shpcount=-1;

struct LevelAndKit
{
	unsigned int level;
	unsigned int kit;
};

class SpellEntry
{
public:
	~SpellEntry();
	SpellEntry();
	const ieResRef *GetSpell() const;
	const ieResRef *FindSpell(unsigned int level, unsigned int kit) const;
	int FindSpell(unsigned int kit) const;
	bool Equals(const char *spl) const;
	void SetSpell(const char *spl);
	void AddLevel(unsigned int level, unsigned int kit);
private:
	ieResRef spell;
	LevelAndKit *levels;
	int count;
};

SpellEntry::SpellEntry()
{
	levels = NULL;
	spell[0]=0;
	count = 0;
}

SpellEntry::~SpellEntry()
{
	free(levels);
	levels = NULL;
}

const ieResRef *SpellEntry::GetSpell() const
{
	return &spell;
}

const ieResRef *SpellEntry::FindSpell(unsigned int level, unsigned int kit) const
{
	int i = count;
	while(i--) {
		if (levels[i].level==level && levels[i].kit==kit) {
			return &spell;
		}
	}
	return NULL;
}

int SpellEntry::FindSpell(unsigned int kit) const
{
	int i = count;
	while(i--) {
		if (levels[i].kit==kit) {
			return levels[i].level;
		}
	}
	return 0;
}

static int FindSpell(ieResRef spellref, SpellEntry* list, int listsize)
{
	int i = listsize;
	while (i--) {
		if (list[i].Equals(spellref)) {
			return i;
		}
	}
	return -1;
}

bool SpellEntry::Equals(const char *spl) const
{
	return !strnicmp(spell, spl, sizeof(ieResRef));
}

void SpellEntry::SetSpell(const char *spl)
{
	strnlwrcpy(spell, spl, 8);
}

void SpellEntry::AddLevel(unsigned int level,unsigned int kit)
{
	if(!level) {
		return;
	}

	level--; // convert to 0-based for internal use
	for(int i=0;i<count;i++) {
		if(levels[i].kit==kit && levels[i].level==level) {
			Log(WARNING, "CREImporter", "Skipping duplicate spell list table entry for: %s", spell);
			return;
		}
	}
	levels = (LevelAndKit *) realloc(levels, sizeof(LevelAndKit) * (count+1) );
	levels[count].kit=kit;
	levels[count].level=level;
	count++;
}

static int IsInnate(ieResRef name)
{
	for(int i=0;i<inncount;i++) {
		if(!strnicmp(name, innlist[i], 8) ) {
			return i;
		}
	}
	return -1;
}

static int IsSong(ieResRef name)
{
	for(int i=0;i<sngcount;i++) {
		if(!strnicmp(name, snglist[i], 8) ) {
			return i;
		}
	}
	return -1;
}

static int IsShape(ieResRef name)
{
	for(int i=0;i<shpcount;i++) {
		if(!strnicmp(name, shplist[i], 8) ) {
			return i;
		}
	}
	return -1;
}

static SpellEntry* spllist=NULL;
static int splcount=-1;
static SpellEntry* domlist=NULL;
static int domcount=-1;
static SpellEntry* maglist=NULL;
static int magcount=-1;

static int IsDomain(ieResRef name, unsigned short &level, unsigned int kit)
{
	for(int i=0;i<domcount;i++) {
		if (domlist[i].Equals(name) ) {
			level = domlist[i].FindSpell(kit);
				return i;
		}
	}
	return -1;
}

static int IsSpecial(ieResRef name, unsigned short &level, unsigned int kit)
{
	for(int i=0;i<magcount;i++) {
		if (maglist[i].Equals(name) ) {
			level = maglist[i].FindSpell(kit);
				return i;
		}
	}
	return -1;
}

static int SpellType(ieResRef name, unsigned short &level, unsigned int clsmsk)
{
	for (int i = 0;i<splcount;i++) {
		if (spllist[i].Equals(name) ) {
			for(int type=0;type<7;type++) {
				if (clsmsk & i) {
					level = spllist[i].FindSpell(type);
				}
			}
		}
	}
	return 0;
}

int CREImporter::FindSpellType(char *name, unsigned short &level, unsigned int clsmsk, unsigned int kit) const
{
	level = 0;
	if (IsSong(name)>=0) return IE_IWD2_SPELL_SONG;
	if (IsShape(name)>=0) return IE_IWD2_SPELL_SHAPE;
	if (IsInnate(name)>=0) return IE_IWD2_SPELL_INNATE;
	if (IsDomain(name, level, kit)>=0) return IE_IWD2_SPELL_DOMAIN;
	if (IsSpecial(name, level, kit)>=0) return IE_IWD2_SPELL_WIZARD;
	return SpellType(name, level, clsmsk);
}

//int CREImporter::ResolveSpellName(ieResRef name, int level, ieIWD2SpellType type) const
int ResolveSpellName(ieResRef name, int level, ieIWD2SpellType type)
{
	int i;

	if (level>=MAX_SPELL_LEVEL) {
		return -1;
	}
	switch(type)
	{
	case IE_IWD2_SPELL_INNATE:
		for(i=0;i<inncount;i++) {
			if(!strnicmp(name, innlist[i], 8) ) {
				return i;
			}
		}
		break;
	case IE_IWD2_SPELL_SONG:
		for(i=0;i<sngcount;i++) {
			if(!strnicmp(name, snglist[i], 8) ) {
				return i;
			}
		}
		break;
	case IE_IWD2_SPELL_SHAPE:
		for(i=0;i<shpcount;i++) {
			if(!strnicmp(name, shplist[i], 8) ) {
				return i;
			}
		}
		break;
	case IE_IWD2_SPELL_DOMAIN:
	default:
		for(i=0;i<splcount;i++) {
			if (spllist[i].Equals(name) ) return i;
		}
	}
	return -1;
}

// just returns the integer part of the log
// perfect for deducing kit values, since they are bitfields and we don't care about any noise
static int log2(int value)
{
	int pow = -1;
	while (value) {
		value = value>>1;
		pow++;
	}
	return pow;
}

//input: index, level, type, kit
static const ieResRef *ResolveSpellIndex(int index, int level, ieIWD2SpellType type, int kit)
{
	const ieResRef *ret;

	if (level>=MAX_SPELL_LEVEL) {
		return NULL;
	}

	switch(type)
	{
	case IE_IWD2_SPELL_INNATE:
		if (index>=inncount) {
			return NULL;
		}
		return &innlist[index];
	case IE_IWD2_SPELL_SONG:
		if (index>=sngcount) {
			return NULL;
		}
		return &snglist[index];
	case IE_IWD2_SPELL_SHAPE:
		if (index>=shpcount) {
			return NULL;
		}
		return &shplist[index];
	case IE_IWD2_SPELL_DOMAIN:
		if (index>=splcount) {
			return NULL;
		}
		// translate the actual kit to a column index to make them comparable
		// luckily they are in order
		kit = log2(kit/0x8000); // 0x8000 is the first cleric kit
		ret = domlist[index].FindSpell(level, kit);
		if (ret) {
			return ret;
		}
		// sigh, retry with wizard spells, since the table does not cover everything npcs have
		kit = -1;
		type = IE_IWD2_SPELL_WIZARD;
		break;
	case IE_IWD2_SPELL_WIZARD:
		if (index>=splcount) {
			break;
		}
		// translate the actual kit to a column index to make them comparable
		kit = log2(kit/0x40); // 0x40 is the first mage kit
		//if it is a specialist spell, return it now
		ret = maglist[index].FindSpell(level, kit);
		if ( ret) {
			return ret;
		}
		//fall through
	default:
		kit = -1;
		//comes later
		break;
	}

	// type matches the table columns (0-bard to 6-wizard)
	ret = spllist[index].FindSpell(level, type);
	if (!ret) {
		// some npcs have spells at odd levels, so the lookup just failed
		// eg. slayer knights of xvim with sppr325 at level 2 instead of 3
		Log(ERROR, "CREImporter", "Spell (%d of type %d) found at unexpected level (%d)!", index, type, level);
		int level2 = spllist[index].FindSpell(type);
		// grrr, some rows have no levels set - they're all 0, but with a valid resref, so just return that
		if (!level2) {
			Log(DEBUG, "CREImporter", "Spell entry (%d) without any levels set!", index);
			return spllist[index].GetSpell();
		}
		ret = spllist[index].FindSpell(level2, type);
		if (ret) Log(DEBUG, "CREImporter", "The spell was found at level %d!", level2);
	}
	if (ret || (kit==-1) ) {
		return ret;
	}

	error("CREImporter", "Doing extra mage spell lookups!");
	// FIXME: is this really needed? reachable only if wizard index was too high
	kit = log2(kit/0x40); // 0x40 is the first mage kit
	int i;
	for(i=0;i<magcount;i++) {
		if (maglist[i].Equals(*ret)) {
			return maglist[i].FindSpell(level, kit);
		}
	}
	return NULL;
}

static void ReleaseMemoryCRE()
{
	if (randcolors) {
		delete [] randcolors;
		randcolors = NULL;
	}
	RandColor = -1;

	if (spllist) {
		delete [] spllist;
		spllist = NULL;
	}
	splcount = -1;

	if (domlist) {
		delete [] domlist;
		domlist = NULL;
	}
	domcount = -1;

	if (maglist) {
		delete [] maglist;
		maglist = NULL;
	}
	magcount = -1;

	if (innlist) {
		free(innlist);
		innlist = NULL;
	}
	inncount = -1;

	if(snglist) {
		free(snglist);
		snglist = NULL;
	}
	sngcount = -1;

	if(shplist) {
		free(shplist);
		shplist = NULL;
	}
	shpcount = -1;
}

static ieResRef *GetSpellTable(const ieResRef tableresref, int &count)
{
	count = 0;
	AutoTable tab(tableresref);
	if (!tab)
		return 0;

	int column = tab->GetColumnCount()-1;
	if (column<0) {
		return 0;
	}

	count = tab->GetRowCount();
	ieResRef *reslist = (ieResRef *) malloc (sizeof(ieResRef) * count);
	for(int i = 0; i<count;i++) {
		strnlwrcpy(reslist[i], tab->QueryField(i, column), 8);
	}
	return reslist;
}

// different tables, but all use listspll.2da for the spell indices
static SpellEntry *GetKitSpell(const ieResRef tableresref, int &count)
{
	count = 0;
	AutoTable tab(tableresref);
	if (!tab)
		return 0;

	int column = tab->GetColumnCount()-1;
	if (column<1) {
		return 0;
	}

	count = tab->GetRowCount();
	SpellEntry *reslist;
	bool indexlist = false;
	if (!strnicmp(tableresref, "listspll", 8)) {
		indexlist = true;
		reslist = new SpellEntry[count];
	} else {
		reslist = new SpellEntry[splcount]; // needs to be the same size for the simple index lookup we do!
	}
	int index;
	for(int i = 0;i<count;i++) {
		if (indexlist) {
			index = i;
		} else {
			// find the correct index in listspll.2da
			ieResRef spellref;
			strnlwrcpy(spellref, tab->QueryField(i, column), 8);
			// the table has disabled spells in it and they all have the first two chars replaced by '*'
			if (spellref[0] == '*') {
				continue;
			}
			index = FindSpell(spellref, spllist, splcount);
			assert (index != -1);
		}
		reslist[index].SetSpell(tab->QueryField(i, column));
		for(int col=0;col<column;col++) {
			reslist[index].AddLevel(atoi(tab->QueryField(i, col)), col);
		}
	}
	return reslist;
}

static void InitSpellbook()
{
	if (splcount!=-1) {
		return;
	}

	if (core->HasFeature(GF_HAS_SPELLLIST)) {
		innlist = GetSpellTable("listinnt", inncount);
		snglist = GetSpellTable("listsong", sngcount);
		shplist = GetSpellTable("listshap", shpcount);
		spllist = GetKitSpell("listspll", splcount); // need to init this one first, since the other two rely on it
		maglist = GetKitSpell("listmage", magcount);
		domlist = GetKitSpell("listdomn", domcount);
	}
}

CREImporter::CREImporter(void)
{
	str = NULL;
	TotSCEFF = 0xff;
	CREVersion = 0xff;
	InitSpellbook();
}

CREImporter::~CREImporter(void)
{
	delete str;
}

bool CREImporter::Open(DataStream* stream)
{
	if (stream == NULL) {
		return false;
	}
	delete str;
	str = stream;
	char Signature[8];
	str->Read( Signature, 8 );
	IsCharacter = false;
	if (strncmp( Signature, "CHR ",4) == 0) {
		IsCharacter = true;
		//skips chr signature, reads cre signature
		if (!SeekCreHeader(Signature)) {
			return false;
		}
	} else {
		CREOffset = 0;
	}
	if (strncmp( Signature, "CRE V1.0", 8 ) == 0) {
		CREVersion = IE_CRE_V1_0;
		return true;
	}
	if (strncmp( Signature, "CRE V1.2", 8 ) == 0) {
		CREVersion = IE_CRE_V1_2;
		return true;
	}
	if (strncmp( Signature, "CRE V2.2", 8 ) == 0) {
		CREVersion = IE_CRE_V2_2;
		return true;
	}
	if (strncmp( Signature, "CRE V9.0", 8 ) == 0) {
		CREVersion = IE_CRE_V9_0;
		return true;
	}
	if (strncmp( Signature, "CRE V0.0", 8 ) == 0) {
		CREVersion = IE_CRE_GEMRB;
		return true;
	}

	Log(ERROR, "CREImporter", "Not a CRE File or File Version not supported: %8.8s", Signature);
	return false;
}

void CREImporter::SetupSlotCounts()
{
	switch (CREVersion) {
		case IE_CRE_V1_2: //pst
			QWPCount=4;
			QSPCount=3;
			QITCount=5;
			break;
		case IE_CRE_GEMRB: //own
			QWPCount=8;
			QSPCount=9;
			QITCount=5;
			break;
		case IE_CRE_V2_2: //iwd2
			QWPCount=8;
			QSPCount=9;
			QITCount=3;
			break;
		default: //others
			QWPCount=4;
			QSPCount=3;
			QITCount=3;
			break;
	}
}

void CREImporter::WriteChrHeader(DataStream *stream, Actor *act)
{
	char Signature[8];
	char filling[10];
	ieVariable name;
	ieDword tmpDword, CRESize;
	ieWord tmpWord;

	CRESize = GetStoredFileSize (act);
	switch (CREVersion) {
		case IE_CRE_V9_0: //iwd/HoW
			memcpy(Signature, "CHR V1.0",8);
			tmpDword = 0x64; //headersize
			TotSCEFF = 1;
			break;
		case IE_CRE_V1_0: //bg1
			memcpy(Signature, "CHR V1.0",8);
			tmpDword = 0x64; //headersize
			TotSCEFF = 0;
			break;
		case IE_CRE_V1_1: //bg2 (fake)
			memcpy(Signature, "CHR V2.0",8);
			tmpDword = 0x64; //headersize
			TotSCEFF = 1;
			break;
		case IE_CRE_V1_2: //pst
			memcpy(Signature, "CHR V1.2",8);
			tmpDword = 0x68; //headersize
			TotSCEFF = 0;
			break;
		case IE_CRE_V2_2: //iwd2
			memcpy(Signature, "CHR V2.2",8);
			tmpDword = 0x21c; //headersize
			TotSCEFF = 1;
			break;
		case IE_CRE_GEMRB: //own format
			memcpy(Signature, "CHR V0.0",8);
			tmpDword = 0x1dc; //headersize (iwd2-9x8+8)
			TotSCEFF = 1;
			break;
		default:
			Log(ERROR, "CREImporter", "Unknown CHR version!");
			return;
	}
	stream->Write( Signature, 8);
	memset( Signature,0,sizeof(Signature));
	memset( name,0,sizeof(name));
	strncpy( name, act->GetName(0), sizeof(name) );
	stream->Write( name, 32);

	stream->WriteDword( &tmpDword); //cre offset (chr header size)
	stream->WriteDword( &CRESize);  //cre size

	SetupSlotCounts();
	int i;
	for (i=0;i<QWPCount;i++) {
		tmpWord = act->PCStats->QuickWeaponSlots[i];
		stream->WriteWord (&tmpWord);
	}
	for (i=0;i<QWPCount;i++) {
		tmpWord = act->PCStats->QuickWeaponHeaders[i];
		stream->WriteWord (&tmpWord);
	}
	for (i=0;i<QSPCount;i++) {
		stream->WriteResRef (act->PCStats->QuickSpells[i]);
	}
	//This is 9 for IWD2 and GemRB
	if (QSPCount==9) {
		//NOTE: the gemrb internal format stores
		//0xff or 0xfe in case of innates and bardsongs
		memset(filling,0,sizeof(filling));
		memcpy(filling,act->PCStats->QuickSpellClass,MAX_QSLOTS);
		for(i=0;i<MAX_QSLOTS;i++) {
			if ( (ieByte) filling[i]>=0xfe) filling[i]=0;
		}
		stream->Write( filling, 10);
	}
	for (i=0;i<QITCount;i++) {
		tmpWord = act->PCStats->QuickItemSlots[i];
		stream->WriteWord (&tmpWord);
	}
	for (i=0;i<QITCount;i++) {
		tmpWord = act->PCStats->QuickItemHeaders[i];
		stream->WriteWord (&tmpWord);
	}
	switch (CREVersion) {
	case IE_CRE_V2_2:
		//gemrb format doesn't save these redundantly
		for (i=0;i<QSPCount;i++) {
			if (act->PCStats->QuickSpellClass[i]==0xff) {
				stream->WriteResRef (act->PCStats->QuickSpells[i]);
			} else {
				//empty field
				stream->Write( Signature, 8);
			}
		}
		for (i=0;i<QSPCount;i++) {
			if (act->PCStats->QuickSpellClass[i]==0xfe) {
				stream->WriteResRef (act->PCStats->QuickSpells[i]);
			} else {
				//empty field
				stream->Write( Signature, 8);
			}
		}
		//fallthrough
	case IE_CRE_GEMRB:
		for (i=0;i<QSPCount;i++) {
			tmpDword = act->PCStats->QSlots[i+3];
			stream->WriteDword( &tmpDword);
		}
		for (i=0;i<13;i++) {
			stream->WriteWord (&tmpWord);
		}
		stream->Write( act->PCStats->SoundFolder, 32);
		stream->Write( act->PCStats->SoundSet, 8);
		for (i=0;i<ES_COUNT;i++) {
			tmpDword = act->PCStats->ExtraSettings[i];
			stream->WriteDword( &tmpDword);
		}
		//Reserved
		tmpDword = 0;
		for (i=0;i<16;i++) {
			stream->WriteDword( &tmpDword);
		}
		break;
	default:
		break;
	}
}

void CREImporter::ReadChrHeader(Actor *act)
{
	ieVariable name;
	char Signature[8];
	ieDword offset, size;
	ieDword tmpDword;
	ieWord tmpWord;
	ieByte tmpByte;

	act->CreateStats();
	str->Rewind();
	str->Read (Signature, 8);
	str->Read (name, 32);
	name[32]=0;
	if (name[0]) {
		act->SetName( name, 0 ); //setting longname
	}
	str->ReadDword( &offset);
	str->ReadDword( &size);
	SetupSlotCounts();
	int i;
	for (i=0;i<QWPCount;i++) {
		str->ReadWord (&tmpWord);
		act->PCStats->QuickWeaponSlots[i]=tmpWord;
	}
	for (i=0;i<QWPCount;i++) {
		str->ReadWord (&tmpWord);
		act->PCStats->QuickWeaponHeaders[i]=tmpWord;
	}
	for (i=0;i<QSPCount;i++) {
		str->ReadResRef (act->PCStats->QuickSpells[i]);
	}
	if (QSPCount==9) {
		str->Read (act->PCStats->QuickSpellClass,9);
		str->Read (&tmpByte, 1);
	}
	for (i=0;i<QITCount;i++) {
		str->ReadWord (&tmpWord);
		act->PCStats->QuickItemSlots[i]=tmpWord;
	}
	for (i=0;i<QITCount;i++) {
		str->ReadWord (&tmpWord);
		act->PCStats->QuickItemHeaders[i]=tmpWord;
	}

	//here comes the version specific read
	switch (CREVersion) {
	case IE_CRE_V2_2:
		//gemrb format doesn't save these redundantly
		for (i=0;i<QSPCount;i++) {
			str->ReadResRef(Signature);
			if (Signature[0]) {
				act->PCStats->QuickSpellClass[i]=0xff;
				memcpy(act->PCStats->QuickSpells[i], Signature, sizeof(ieResRef));
			}
		}
		for (i=0;i<QSPCount;i++) {
			str->ReadResRef(Signature);
			if (Signature[0]) {
				act->PCStats->QuickSpellClass[i]=0xfe;
				memcpy(act->PCStats->QuickSpells[i], Signature, sizeof(ieResRef));
			}
		}
		//fallthrough
	case IE_CRE_GEMRB:
		for (i=0;i<QSPCount;i++) {
			str->ReadDword( &tmpDword);
			act->PCStats->QSlots[i+3] = (ieByte) tmpDword;
		}
		str->Seek(26, GEM_CURRENT_POS);
		str->Read( act->PCStats->SoundFolder, 32);
		str->Read( act->PCStats->SoundSet, 8);
		for (i=0;i<ES_COUNT;i++) {
			str->ReadDword( &act->PCStats->ExtraSettings[i] );
		}
		//Reserved
		str->Seek(64, GEM_CURRENT_POS);
		break;
	default:
		break;
	}
}

bool CREImporter::SeekCreHeader(char *Signature)
{
	str->Seek(32, GEM_CURRENT_POS);
	str->ReadDword( &CREOffset );
	str->Seek(CREOffset, GEM_STREAM_START);
	str->Read( Signature, 8);
	return true;
}

CREMemorizedSpell* CREImporter::GetMemorizedSpell()
{
	CREMemorizedSpell* spl = new CREMemorizedSpell();

	str->ReadResRef( spl->SpellResRef );
	str->ReadDword( &spl->Flags );

	return spl;
}

CREKnownSpell* CREImporter::GetKnownSpell()
{
	CREKnownSpell* spl = new CREKnownSpell();

	str->ReadResRef( spl->SpellResRef );
	str->ReadWord( &spl->Level );
	str->ReadWord( &spl->Type );

	return spl;
}

void CREImporter::ReadScript(Actor *act, int ScriptLevel)
{
	ieResRef aScript;
	str->ReadResRef( aScript );
	act->SetScript( aScript, ScriptLevel, act->InParty!=0);
}

CRESpellMemorization* CREImporter::GetSpellMemorization(Actor *act)
{
	ieWord Level, Type, Number, Number2;

	str->ReadWord( &Level );
	str->ReadWord( &Number );
	str->ReadWord( &Number2 );
	str->ReadWord( &Type );
	str->ReadDword( &MemorizedIndex );
	str->ReadDword( &MemorizedCount );

	CRESpellMemorization* spl = act->spellbook.GetSpellMemorization(Type, Level);
	assert(spl && spl->SlotCount == 0 && spl->SlotCountWithBonus == 0); // unused
	spl->SlotCount = Number;
	spl->SlotCountWithBonus = Number;

	return spl;
}

void CREImporter::SetupColor(ieDword &stat)
{
	if (RandColor==-1) {
		RandColor=0;
		RandRows=0;
		AutoTable rndcol("randcolr", true);
		if (rndcol) {
			RandColor = rndcol->GetColumnCount();
			RandRows = rndcol->GetRowCount();
			if (RandRows>MAXCOLOR) RandRows=MAXCOLOR;
		}
		if (RandRows>1 && RandColor>0) {
			randcolors = new ColorSet[RandColor];
			int cols = RandColor;
			while(cols--)
			{
				for (int i=0;i<RandRows;i++) {
					randcolors[cols][i]=atoi( rndcol->QueryField( i, cols ) );
				}
				randcolors[cols][0]-=200;
			}
		}
		else {
			RandColor=0;
		}
	}

	if (stat<200) return;
	if (RandColor>0) {
		stat-=200;
		//assuming an ordered list, so looking in the middle first
		int i;
		for (i=(int) stat;i>=0;i--) {
			if (randcolors[i][0]==stat) {
				stat=randcolors[i][rand()%RandRows];
				return;
			}
		}
		for (i=(int) stat+1;i<RandColor;i++) {
			if (randcolors[i][0]==stat) {
				stat=randcolors[i][rand()%RandRows];
				return;
			}
		}
	}
}

void CREImporter::ReadDialog(Actor *act)
{
	ieResRef Dialog;

	str->ReadResRef(Dialog);
	//Hacking NONE to no error
	if (strnicmp(Dialog,"NONE",8) == 0) {
		Dialog[0]=0;
	}
	act->SetDialog(Dialog);
}

Actor* CREImporter::GetActor(unsigned char is_in_party)
{
	if (!str)
		return NULL;
	Actor* act = new Actor();
	if (!act)
		return NULL;
	act->InParty = is_in_party;
	str->ReadDword( &act->LongStrRef );
	//Beetle name in IWD needs the allow zero flag
	char* poi = core->GetString( act->LongStrRef, IE_STR_ALLOW_ZERO );
	act->SetName( poi, 1 ); //setting longname
	free( poi );
	str->ReadDword( &act->ShortStrRef );
	poi = core->GetString( act->ShortStrRef );
	act->SetName( poi, 2 ); //setting shortname (for tooltips)
	free( poi );
	act->BaseStats[IE_VISUALRANGE] = 30; //this is just a hack
	act->BaseStats[IE_DIALOGRANGE] = 15; //this is just a hack
	str->ReadDword( &act->BaseStats[IE_MC_FLAGS] );
	str->ReadDword( &act->BaseStats[IE_XPVALUE] );
	str->ReadDword( &act->BaseStats[IE_XP] );
	str->ReadDword( &act->BaseStats[IE_GOLD] );
	str->ReadDword( &act->BaseStats[IE_STATE_ID] );
	ieWord tmp;
	ieWordSigned tmps;
	str->ReadWordSigned( &tmps );
	act->BaseStats[IE_HITPOINTS]=(ieDwordSigned)tmps;
	str->ReadWord( &tmp );
	act->BaseStats[IE_MAXHITPOINTS]=tmp;
	str->ReadDword( &act->BaseStats[IE_ANIMATION_ID] );//animID is a dword
	ieByte tmp2[7];
	str->Read( tmp2, 7);
	for (int i=0;i<7;i++) {
		ieDword t = tmp2[i];
		// apply RANDCOLR.2DA transformation
		SetupColor(t);
		t |= t << 8;
		t |= t << 16;
		act->BaseStats[IE_COLORS+i]=t;
	}

	str->Read( &TotSCEFF, 1 );
	if (CREVersion==IE_CRE_V1_0 && TotSCEFF) {
		CREVersion = IE_CRE_V1_1;
	}
	// saving in original version requires the original version
	// otherwise it is set to 0 at construction time
	if (core->SaveAsOriginal) {
		act->version = CREVersion;
	}
	str->ReadResRef( act->SmallPortrait );
	if (act->SmallPortrait[0]==0) {
		memcpy(act->SmallPortrait, "NONE\0\0\0\0", 8);
	}
	str->ReadResRef( act->LargePortrait );
	if (act->LargePortrait[0]==0) {
		memcpy(act->LargePortrait, "NONE\0\0\0\0", 8);
	}

	unsigned int Inventory_Size;

	switch(CREVersion) {
		case IE_CRE_GEMRB:
			Inventory_Size = GetActorGemRB(act);
			break;
		case IE_CRE_V1_2:
			Inventory_Size=46;
			GetActorPST(act);
			break;
		case IE_CRE_V1_1: //bg2 (fake version)
		case IE_CRE_V1_0: //bg1 too
			Inventory_Size=38;
			GetActorBG(act);
			break;
		case IE_CRE_V2_2:
			Inventory_Size=50;
			GetActorIWD2(act);
			break;
		case IE_CRE_V9_0:
			Inventory_Size=38;
			GetActorIWD1(act);
			break;
		default:
			Log(ERROR, "CREImporter", "Unknown creature signature: %d\n", CREVersion);
			delete act;
			return NULL;
	}

	// Read saved effects
	if (core->IsAvailable(IE_EFF_CLASS_ID) ) {
		ReadEffects( act );
	} else {
		Log(ERROR, "CREImporter", "Effect importer is unavailable!");
	}
	// Reading inventory, spellbook, etc
	ReadInventory( act, Inventory_Size );

	if (IsCharacter) {
		ReadChrHeader(act);
	}

	act->InitStatsOnLoad();

	return act;
}

void CREImporter::GetActorPST(Actor *act)
{
	int i;
	ieByte tmpByte;
	ieWord tmpWord;

	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_REPUTATION]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_HIDEINSHADOWS]=tmpByte;
	str->ReadWord( &tmpWord );
	//skipping a word
	str->ReadWord( &tmpWord );
	act->AC.SetNatural((ieWordSigned) tmpWord);
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACCRUSHINGMOD]=(ieWordSigned) tmpWord;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACMISSILEMOD]=(ieWordSigned) tmpWord;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACPIERCINGMOD]=(ieWordSigned) tmpWord;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACSLASHINGMOD]=(ieWordSigned) tmpWord;
	str->Read( &tmpByte, 1 );
	act->ToHit.SetBase((ieByteSigned) tmpByte);
	str->Read( &tmpByte, 1 );
	tmpByte = tmpByte * 2;
	if (tmpByte>10) tmpByte-=11;
	act->BaseStats[IE_NUMBEROFATTACKS]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSDEATH]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSWANDS]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSPOLY]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSBREATH]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSSPELL]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTFIRE]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTCOLD]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTELECTRICITY]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTACID]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMAGIC]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMAGICFIRE]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMAGICCOLD]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTSLASHING]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTCRUSHING]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTPIERCING]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMISSILE]=(ieByteSigned) tmpByte;
	//this is used for unused prof points count
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_FREESLOTS]=tmpByte; //using another field than usually
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SETTRAPS]=tmpByte; //this is unused in pst
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LORE]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LOCKPICKING]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_STEALTH]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_TRAPS]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_PICKPOCKET]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_FATIGUE]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_INTOXICATION]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LUCK]=(ieByteSigned) tmpByte;
	//last byte is actually an undead level (according to IE dev info)
	for (i=0;i<21;i++) {
		str->Read( &tmpByte, 1 );
		act->BaseStats[IE_PROFICIENCYBASTARDSWORD+i]=tmpByte;
	}
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_TRACKING]=tmpByte;
	//scriptname of tracked creature (according to IE dev info)
	str->Seek( 32, GEM_CURRENT_POS );
	for (i=0;i<100;i++) {
		str->ReadDword( &act->StrRefs[i] );
	}
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LEVEL]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LEVEL2]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LEVEL3]=tmpByte;
	//this is rumoured to be IE_SEX, but we use the gender field for this
	str->Read( &tmpByte, 1 );
	//skipping a byte
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_STR]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_STREXTRA]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_INT]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_WIS]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_DEX]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_CON]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_CHR]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_MORALE]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_MORALEBREAK]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_HATEDRACE]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_MORALERECOVERYTIME]=tmpByte;
	str->Read( &tmpByte, 1 );
	//skipping a byte
	str->ReadDword( &act->BaseStats[IE_KIT] );
	ReadScript(act, SCR_OVERRIDE);
	ReadScript(act, SCR_CLASS);
	ReadScript(act, SCR_RACE);
	ReadScript(act, SCR_GENERAL);
	ReadScript(act, SCR_DEFAULT);

	str->Seek( 36, GEM_CURRENT_POS );
	//the overlays are not fully decoded yet
	//they are a kind of effect block (like our vvclist)
	str->ReadDword( &OverlayOffset );
	str->ReadDword( &OverlayMemorySize );
	str->ReadDword( &act->BaseStats[IE_XP_MAGE] ); // Exp for secondary class
	str->ReadDword( &act->BaseStats[IE_XP_THIEF] ); // Exp for tertiary class
	for (i = 0; i<10; i++) {
		str->ReadWord( &tmpWord );
		act->BaseStats[IE_INTERNAL_0+i]=tmpWord;
	}
	//good, law, lady, murder
	for (i=0;i<4;i++) {
		str->Read( &tmpByte, 1);
		act->DeathCounters[i]=(ieByteSigned) tmpByte;
	}
	ieVariable KillVar; //use this as needed
	str->Read(KillVar,32);
	KillVar[32]=0;
	str->Seek( 3, GEM_CURRENT_POS ); // dialog radius, feet circle size???

	str->Read( &tmpByte, 1 );

	str->ReadDword( &act->AppearanceFlags );

	for (i = 0; i < 7; i++) {
		str->ReadWord( &tmpWord );
		act->BaseStats[IE_COLORS+i] = tmpWord;
	}
	act->BaseStats[IE_COLORCOUNT] = tmpByte; //hack

	str->Seek(31, GEM_CURRENT_POS);
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SPECIES]=tmpByte; // offset: 0x311
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_TEAM]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_FACTION]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_EA]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_GENERAL]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RACE]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_CLASS]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SPECIFIC]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SEX]=tmpByte;
	str->Seek( 5, GEM_CURRENT_POS );
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_ALIGNMENT]=tmpByte;
	str->Seek( 4, GEM_CURRENT_POS );
	ieVariable scriptname;
	str->Read( scriptname, 32);
	scriptname[32]=0;
	act->SetScriptName(scriptname);
	strnspccpy(act->KillVar, KillVar, 32);
	memset(act->IncKillVar, 0, 32);

	str->ReadDword( &KnownSpellsOffset );
	str->ReadDword( &KnownSpellsCount );
	str->ReadDword( &SpellMemorizationOffset );
	str->ReadDword( &SpellMemorizationCount );
	str->ReadDword( &MemorizedSpellsOffset );
	str->ReadDword( &MemorizedSpellsCount );

	str->ReadDword( &ItemSlotsOffset );
	str->ReadDword( &ItemsOffset );
	str->ReadDword( &ItemsCount );
	str->ReadDword( &EffectsOffset );
	str->ReadDword( &EffectsCount ); //also variables

	ReadDialog(act);
}

void CREImporter::ReadInventory(Actor *act, unsigned int Inventory_Size)
{
	ieWord *indices = (ieWord *) calloc(Inventory_Size, sizeof(ieWord));
	//CREItem** items;
	unsigned int i,j,k;
	ieWordSigned eqslot;
	ieWord eqheader;

	act->inventory.SetSlotCount(Inventory_Size+1);
	str->Seek( ItemSlotsOffset+CREOffset, GEM_STREAM_START );

	//first read the indices
	for (i = 0;i<Inventory_Size; i++) {
		str->ReadWord(indices+i);
	}
	//this word contains the equipping info (which slot is selected)
	// 0,1,2,3 - weapon slots
	// 1000 - fist
	// -24,-23,-22,-21 - quiver
	//the equipping effects are delayed until the actor gets an area
	str->ReadWordSigned(&eqslot);
	//the equipped slot's selected ability is stored here
	str->ReadWord(&eqheader);
	act->inventory.SetEquipped(eqslot, eqheader);

	//read the item entries based on the previously read indices
	//an item entry may be read multiple times if the indices are repeating
	for (i = 0;i<Inventory_Size;) {
		//the index was intentionally increased here, the fist slot isn't saved
		ieWord index = indices[i++];
		if (index != 0xffff) {
			if (index>=ItemsCount) {
				Log(ERROR, "CREImporter", "Invalid item index (%d) in creature!", index);
				continue;
			}
			//20 is the size of CREItem on disc (8+2+3x2+4)
			str->Seek( ItemsOffset+index*20 + CREOffset, GEM_STREAM_START );
			//the core allocates this item data
			CREItem *item = core->ReadItem(str);
			int Slot = core->QuerySlot(i);
			if (item) {
				act->inventory.SetSlotItem(item, Slot);
			} else {
				Log(ERROR, "CREImporter", "Invalid item index (%d) in creature!", index);
			}
		}
	}

	free (indices);

	// Reading spellbook
	CREKnownSpell **known_spells=(CREKnownSpell **) calloc(KnownSpellsCount, sizeof(CREKnownSpell *) );
	CREMemorizedSpell **memorized_spells=(CREMemorizedSpell **) calloc(MemorizedSpellsCount, sizeof(CREMemorizedSpell *) );

	str->Seek( KnownSpellsOffset+CREOffset, GEM_STREAM_START );
	for (i = 0; i < KnownSpellsCount; i++) {
		known_spells[i]=GetKnownSpell();
	}

	str->Seek( MemorizedSpellsOffset+CREOffset, GEM_STREAM_START );
	for (i = 0; i < MemorizedSpellsCount; i++) {
		memorized_spells[i]=GetMemorizedSpell();
	}

	str->Seek( SpellMemorizationOffset+CREOffset, GEM_STREAM_START );
	for (i = 0; i < SpellMemorizationCount; i++) {
		CRESpellMemorization* sm = GetSpellMemorization(act);

		j=KnownSpellsCount;
		while(j--) {
			CREKnownSpell* spl = known_spells[j];
			if (!spl) {
				continue;
			}
			if ((spl->Type == sm->Type) && (spl->Level == sm->Level)) {
				sm->known_spells.push_back( spl );
				known_spells[j] = NULL;
				continue;
			}
		}
		for (j = 0; j < MemorizedCount; j++) {
			k = MemorizedIndex+j;
			assert(k < MemorizedSpellsCount);
			if (memorized_spells[k]) {
				sm->memorized_spells.push_back( memorized_spells[k]);
				memorized_spells[k] = NULL;
				continue;
			}
			Log(WARNING, "CREImporter", "Duplicate memorized spell(%d) in creature!", k);
		}
	}

	i=KnownSpellsCount;
	while(i--) {
		if (known_spells[i]) {
			Log(WARNING, "CREImporter", "Dangling spell in creature: %s!",
				known_spells[i]->SpellResRef);
			delete known_spells[i];
		}
	}
	free(known_spells);

	i=MemorizedSpellsCount;
	while(i--) {
		if (memorized_spells[i]) {
			Log(WARNING, "CREImporter", "Dangling spell in creature: %s!",
				memorized_spells[i]->SpellResRef);
			delete memorized_spells[i];
		}
	}
	free(memorized_spells);
}

void CREImporter::ReadEffects(Actor *act)
{
	unsigned int i;

	str->Seek( EffectsOffset+CREOffset, GEM_STREAM_START );

	for (i = 0; i < EffectsCount; i++) {
		Effect fx;
		GetEffect( &fx );
		// NOTE: AddEffect() allocates a new effect
		act->fxqueue.AddEffect( &fx ); // FIXME: don't reroll dice, time, etc!!
	}
}

void CREImporter::GetEffect(Effect *fx)
{
	PluginHolder<EffectMgr> eM(IE_EFF_CLASS_ID);

	eM->Open( str, false );
	if (TotSCEFF) {
		eM->GetEffectV20( fx );
	} else {
		eM->GetEffectV1( fx );
	}
}

ieDword CREImporter::GetActorGemRB(Actor *act)
{
	ieByte tmpByte;
	ieWord tmpWord;

	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_REPUTATION]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_HIDEINSHADOWS]=tmpByte;
	//skipping a word( useful for something)
	str->ReadWord( &tmpWord );
	str->ReadWord( &tmpWord );
	act->AC.SetNatural((ieWordSigned) tmpWord);
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACCRUSHINGMOD]=(ieWordSigned) tmpWord;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACMISSILEMOD]=(ieWordSigned) tmpWord;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACPIERCINGMOD]=(ieWordSigned) tmpWord;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACSLASHINGMOD]=(ieWordSigned) tmpWord;
	str->Read( &tmpByte, 1 );
	act->ToHit.SetBase((ieByteSigned) tmpByte);
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_NUMBEROFATTACKS]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSDEATH]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSWANDS]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSPOLY]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSBREATH]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSSPELL]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTFIRE]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTCOLD]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTELECTRICITY]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTACID]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMAGIC]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMAGICFIRE]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMAGICCOLD]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTSLASHING]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTCRUSHING]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTPIERCING]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMISSILE]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_DETECTILLUSIONS]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SETTRAPS]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LORE]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LOCKPICKING]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_STEALTH]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_TRAPS]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_PICKPOCKET]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_FATIGUE]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_INTOXICATION]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LUCK]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	//these could be used to save iwd2 skills
	//TODO: gemrb format
	act->BaseStats[IE_TRACKING]=tmpByte;
	for (int i=0;i<100;i++) {
		str->ReadDword( &act->StrRefs[i] );
	}
	return 0;
}

void CREImporter::GetActorBG(Actor *act)
{
	int i;
	ieByte tmpByte;
	ieWord tmpWord;

	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_REPUTATION]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_HIDEINSHADOWS]=tmpByte;
	str->ReadWord( &tmpWord );
	//skipping a word
	str->ReadWord( &tmpWord );
	act->AC.SetNatural((ieWordSigned) tmpWord);
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACCRUSHINGMOD]=(ieWordSigned) tmpWord;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACMISSILEMOD]=(ieWordSigned) tmpWord;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACPIERCINGMOD]=(ieWordSigned) tmpWord;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACSLASHINGMOD]=(ieWordSigned) tmpWord;
	str->Read( &tmpByte, 1 );
	act->ToHit.SetBase((ieByteSigned) tmpByte);
	str->Read( &tmpByte, 1 );
	tmpWord = tmpByte * 2;
	if (tmpWord>10) tmpWord-=11;
	act->BaseStats[IE_NUMBEROFATTACKS]=(ieByte) tmpWord;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSDEATH]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSWANDS]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSPOLY]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSBREATH]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSSPELL]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTFIRE]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTCOLD]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTELECTRICITY]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTACID]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMAGIC]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMAGICFIRE]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMAGICCOLD]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTSLASHING]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTCRUSHING]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTPIERCING]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMISSILE]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_DETECTILLUSIONS]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SETTRAPS]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LORE]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LOCKPICKING]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_STEALTH]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_TRAPS]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_PICKPOCKET]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_FATIGUE]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_INTOXICATION]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LUCK]=(ieByteSigned) tmpByte;
	for (i=0;i<21;i++) {
		str->Read( &tmpByte, 1 );
		act->BaseStats[IE_PROFICIENCYBASTARDSWORD+i]=tmpByte;
	}

	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_TRACKING]=tmpByte;
	str->Seek( 32, GEM_CURRENT_POS );
	for (i=0;i<100;i++) {
		str->ReadDword( &act->StrRefs[i] );
	}
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LEVEL]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LEVEL2]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LEVEL3]=tmpByte;
	//this is rumoured to be IE_SEX, but we use the gender field for this
	str->Read( &tmpByte, 1);
	//skipping a byte
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_STR]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_STREXTRA]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_INT]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_WIS]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_DEX]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_CON]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_CHR]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_MORALE]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_MORALEBREAK]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_HATEDRACE]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_MORALERECOVERYTIME]=tmpByte;
	str->Read( &tmpByte, 1);
	//skipping a byte
	str->ReadDword( &act->BaseStats[IE_KIT] );
	act->BaseStats[IE_KIT] = ((act->BaseStats[IE_KIT] & 0xffff) << 16) +
		((act->BaseStats[IE_KIT] & 0xffff0000) >> 16);
	ReadScript(act, SCR_OVERRIDE);
	ReadScript(act, SCR_CLASS);
	ReadScript(act, SCR_RACE);
	ReadScript(act, SCR_GENERAL);
	ReadScript(act, SCR_DEFAULT);

	str->Read( &tmpByte, 1);
	act->BaseStats[IE_EA]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_GENERAL]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_RACE]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_CLASS]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_SPECIFIC]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_SEX]=tmpByte;
	str->Seek( 5, GEM_CURRENT_POS );
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_ALIGNMENT]=tmpByte;
	str->Seek( 4, GEM_CURRENT_POS );
	ieVariable scriptname;
	str->Read( scriptname, 32);
	scriptname[32]=0;
	act->SetScriptName(scriptname);
	memset(act->KillVar, 0, 32);
	memset(act->IncKillVar, 0, 32);

	str->ReadDword( &KnownSpellsOffset );
	str->ReadDword( &KnownSpellsCount );
	str->ReadDword( &SpellMemorizationOffset );
	str->ReadDword( &SpellMemorizationCount );
	str->ReadDword( &MemorizedSpellsOffset );
	str->ReadDword( &MemorizedSpellsCount );

	str->ReadDword( &ItemSlotsOffset );
	str->ReadDword( &ItemsOffset );
	str->ReadDword( &ItemsCount );
	str->ReadDword( &EffectsOffset );
	str->ReadDword( &EffectsCount );

	ReadDialog(act);
}

void CREImporter::GetIWD2Spellpage(Actor *act, ieIWD2SpellType type, int level, int count)
{
	ieDword spellindex;
	ieDword totalcount;
	ieDword memocount;
	ieDword tmpDword;

	int check = 0;
	CRESpellMemorization* sm = act->spellbook.GetSpellMemorization(type, level);
	assert(sm && sm->SlotCount == 0 && sm->SlotCountWithBonus == 0); // unused
	while(count--) {
		str->ReadDword(&spellindex);
		str->ReadDword(&totalcount);
		str->ReadDword(&memocount);
		str->ReadDword(&tmpDword);
		check+=totalcount;
		const ieResRef *tmp = ResolveSpellIndex(spellindex, level, type, act->BaseStats[IE_KIT]);
		if(tmp) {
			CREKnownSpell *known = new CREKnownSpell;
			known->Level=level;
			known->Type=type;
			strnlwrcpy(known->SpellResRef,*tmp,8);
			sm->known_spells.push_back(known);
			while(memocount--) {
				if(totalcount) {
					totalcount--;
				} else {
					Log(ERROR, "CREImporter", "More spells still known than memorised.");
					break;
				}
				CREMemorizedSpell *memory = new CREMemorizedSpell;
				memory->Flags=1;
				strnlwrcpy(memory->SpellResRef,*tmp,8);
				sm->memorized_spells.push_back(memory);
			}

			while(totalcount--) {
				CREMemorizedSpell *memory = new CREMemorizedSpell;
				memory->Flags=0;
				strnlwrcpy(memory->SpellResRef,*tmp,8);
				sm->memorized_spells.push_back(memory);
			}
		} else {
			error("CREImporter", "Unresolved spell index: %d level:%d, type: %d",
				spellindex, level+1, type);
		}
	}
	// hacks for domain spells, since their count is not stored and also always 1
	// NOTE: luckily this does not cause save game incompatibility
	str->ReadDword(&tmpDword);
	if (type == IE_IWD2_SPELL_DOMAIN) {
		sm->SlotCount = 1;
	} else {
		sm->SlotCount = (ieWord) tmpDword;
	}
	str->ReadDword(&tmpDword);
	if (type == IE_IWD2_SPELL_DOMAIN) {
		sm->SlotCountWithBonus = 1;
	} else {
		sm->SlotCountWithBonus = (ieWord) tmpDword;
	}
}

void CREImporter::GetActorIWD2(Actor *act)
{
	int i;
	ieByte tmpByte;
	ieWord tmpWord;

	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_REPUTATION]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_HIDEINSHADOWS]=tmpByte;
	str->ReadWord( &tmpWord );
	act->AC.SetNatural((ieWordSigned) tmpWord);
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACCRUSHINGMOD]=(ieWordSigned) tmpWord;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACMISSILEMOD]=(ieWordSigned) tmpWord;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACPIERCINGMOD]=(ieWordSigned) tmpWord;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACSLASHINGMOD]=(ieWordSigned) tmpWord;
	str->Read( &tmpByte, 1 );
	act->ToHit.SetBase((ieByteSigned) tmpByte);//Unknown in CRE V2.2
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_NUMBEROFATTACKS]=tmpByte;//Unknown in CRE V2.2
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSDEATH]=(ieByteSigned) tmpByte;//Fortitude Save in V2.2
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSWANDS]=(ieByteSigned) tmpByte;//Reflex Save in V2.2
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSPOLY]=(ieByteSigned) tmpByte;// will Save in V2.2
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTFIRE]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTCOLD]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTELECTRICITY]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTACID]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMAGIC]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMAGICFIRE]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMAGICCOLD]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTSLASHING]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTCRUSHING]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTPIERCING]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMISSILE]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_MAGICDAMAGERESISTANCE]=(ieByteSigned) tmpByte;
	str->Seek( 4, GEM_CURRENT_POS );
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_FATIGUE]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_INTOXICATION]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LUCK]=(ieByteSigned) tmpByte;
	str->Seek( 34, GEM_CURRENT_POS ); //unknowns
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_CLASSLEVELSUM]=tmpByte; //total levels
	str->Read( & tmpByte, 1 );
	act->BaseStats[IE_LEVELBARBARIAN]=tmpByte;
	str->Read( & tmpByte, 1 );
	act->BaseStats[IE_LEVELBARD]=tmpByte;
	str->Read( & tmpByte, 1 );
	act->BaseStats[IE_LEVELCLERIC]=tmpByte;
	str->Read( & tmpByte, 1 );
	act->BaseStats[IE_LEVELDRUID]=tmpByte;
	str->Read( & tmpByte, 1 );
	act->BaseStats[IE_LEVELFIGHTER]=tmpByte;
	str->Read( & tmpByte, 1 );
	act->BaseStats[IE_LEVELMONK]=tmpByte;
	str->Read( & tmpByte, 1 );
	act->BaseStats[IE_LEVELPALADIN]=tmpByte;
	str->Read( & tmpByte, 1 );
	act->BaseStats[IE_LEVELRANGER]=tmpByte;
	str->Read( & tmpByte, 1 );
	act->BaseStats[IE_LEVELTHIEF]=tmpByte;
	str->Read( & tmpByte, 1 );
	act->BaseStats[IE_LEVELSORCERER]=tmpByte;
	str->Read( & tmpByte, 1 );
	act->BaseStats[IE_LEVELMAGE]=tmpByte;
	str->Seek( 22, GEM_CURRENT_POS ); //levels for classes
	for (i=0;i<64;i++) {
		str->ReadDword( &act->StrRefs[i] );
	}
	ReadScript( act, SCR_AREA);
	ReadScript( act, SCR_RESERVED);
	str->Seek( 4, GEM_CURRENT_POS );
	str->ReadDword( &act->BaseStats[IE_FEATS1]);
	str->ReadDword( &act->BaseStats[IE_FEATS2]);
	str->ReadDword( &act->BaseStats[IE_FEATS3]);
	str->Seek( 12, GEM_CURRENT_POS );
	//proficiencies
	for (i=0;i<26;i++) {
		str->Read( &tmpByte, 1);
		act->BaseStats[IE_PROFICIENCYBASTARDSWORD+i]=tmpByte;
	}
	//skills
	str->Seek( 38, GEM_CURRENT_POS );
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_ALCHEMY]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_ANIMALS]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_BLUFF]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_CONCENTRATION]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_DIPLOMACY]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_TRAPS]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_HIDEINSHADOWS]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_INTIMIDATE]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_LORE]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_STEALTH]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_LOCKPICKING]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_PICKPOCKET]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_SEARCH]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_SPELLCRAFT]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_MAGICDEVICE]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_TRACKING]=tmpByte;
	str->Seek( 50, GEM_CURRENT_POS );
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_CR]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_HATEDRACE]=tmpByte;
	//we got 7 more hated races
	for (i=0;i<7;i++) {
		str->Read( &tmpByte, 1 );
		act->BaseStats[IE_HATEDRACE2+i]=tmpByte;
	}
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SUBRACE]=tmpByte;
	str->ReadWord( &tmpWord );
	//skipping 2 bytes, one is SEX (could use it for sounds)
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_STR]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_INT]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_WIS]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_DEX]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_CON]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_CHR]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_MORALE]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_MORALEBREAK]=tmpByte;
	str->Read( &tmpByte, 1 );
	//HatedRace is a list of races, so this is skipped here
	//act->BaseStats[IE_HATEDRACE]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_MORALERECOVERYTIME]=tmpByte;
	//No KIT word order magic for IWD2
	str->ReadDword( &act->BaseStats[IE_KIT] );
	ReadScript(act, SCR_OVERRIDE);
	ReadScript(act, SCR_CLASS);
	ReadScript(act, SCR_RACE);
	ReadScript(act, SCR_GENERAL);
	ReadScript(act, SCR_DEFAULT);
	//new scripting flags, one on each byte
	str->Read( &tmpByte, 1); //hidden
	if (tmpByte) {
		act->BaseStats[IE_AVATARREMOVAL]=tmpByte;
	}
	str->Read( &act->SetDeathVar, 1); //set death variable
	str->Read( &act->IncKillCount, 1); //increase kill count
	str->Read( &act->UnknownField, 1);
	for (i = 0; i<5; i++) {
		str->ReadWord( &tmpWord );
		act->BaseStats[IE_INTERNAL_0+i]=tmpWord;
	}
	ieVariable KillVar;
	str->Read(KillVar,32);
	KillVar[32]=0;
	strnspccpy(act->KillVar, KillVar, 32);
	str->Read(KillVar,32);
	KillVar[32]=0;
	strnspccpy(act->IncKillVar, KillVar, 32);
	str->Seek( 2, GEM_CURRENT_POS);
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_SAVEDXPOS] = tmpWord;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_SAVEDYPOS] = tmpWord;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_SAVEDFACE] = tmpWord;

	str->Seek( 15, GEM_CURRENT_POS );
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_TRANSLUCENT]=tmpByte;
	str->Read( &tmpByte, 1); //fade speed
	str->Read( &tmpByte, 1); //spec. flags
	str->Read( &tmpByte, 1); //invisible
	str->ReadWord( &tmpWord); //unknown
	str->Read( &tmpByte, 1); //unused skill points
	act->BaseStats[IE_UNUSED_SKILLPTS] = tmpByte;
	str->Seek( 124, GEM_CURRENT_POS );
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_EA]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_GENERAL]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_RACE]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_CLASS]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_SPECIFIC]=tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_SEX]=tmpByte;
	str->Seek( 5, GEM_CURRENT_POS );
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_ALIGNMENT]=tmpByte;
	str->Seek( 4, GEM_CURRENT_POS );
	ieVariable scriptname;
	str->Read( scriptname, 32);
	scriptname[32]=0;
	act->SetScriptName(scriptname);

	KnownSpellsOffset = 0;
	KnownSpellsCount = 0;
	SpellMemorizationOffset = 0;
	SpellMemorizationCount = 0;
	MemorizedSpellsOffset = 0;
	MemorizedSpellsCount = 0;
	//6 bytes unknown, 600 bytes spellbook offsets
	//skipping spellbook offsets
	ieWord tmp1, tmp2, tmp3;
	str->ReadWord( &tmp1);
	str->ReadWord( &tmp2);
	str->ReadWord( &tmp3);
	ieDword ClassSpellOffsets[8*9];

	//spellbook spells
	for (i=0;i<7*9;i++) {
		str->ReadDword(ClassSpellOffsets+i);
	}
	ieDword ClassSpellCounts[8*9];
	for (i=0;i<7*9;i++) {
		str->ReadDword(ClassSpellCounts+i);
	}

	//domain spells
	for (i=7*9;i<8*9;i++) {
		str->ReadDword(ClassSpellOffsets+i);
	}
	for (i=7*9;i<8*9;i++) {
		str->ReadDword(ClassSpellCounts+i);
	}

	ieDword InnateOffset, InnateCount;
	ieDword SongOffset, SongCount;
	ieDword ShapeOffset, ShapeCount;
	str->ReadDword( &InnateOffset );
	str->ReadDword( &InnateCount );
	str->ReadDword( &SongOffset );
	str->ReadDword( &SongCount );
	str->ReadDword( &ShapeOffset );
	str->ReadDword( &ShapeCount );
	//str->Seek( 606, GEM_CURRENT_POS);

	str->ReadDword( &ItemSlotsOffset );
	str->ReadDword( &ItemsOffset );
	str->ReadDword( &ItemsCount );
	str->ReadDword( &EffectsOffset );
	str->ReadDword( &EffectsCount );

	ReadDialog(act);

	for(i=0;i<8;i++) {
		for(int lev=0;lev<9;lev++) {
			//if everything is alright, then seeking is not needed
			assert(str->GetPos() == CREOffset+ClassSpellOffsets[i*9+lev]);
			GetIWD2Spellpage(act, (ieIWD2SpellType) i, lev, ClassSpellCounts[i*9+lev]);
		}
	}
	str->Seek(CREOffset+InnateOffset, GEM_STREAM_START);
	GetIWD2Spellpage(act, IE_IWD2_SPELL_INNATE, 0, InnateCount);

	str->Seek(CREOffset+SongOffset, GEM_STREAM_START);
	GetIWD2Spellpage(act, IE_IWD2_SPELL_SONG, 0, SongCount);

	str->Seek(CREOffset+ShapeOffset, GEM_STREAM_START);
	GetIWD2Spellpage(act, IE_IWD2_SPELL_SHAPE, 0, ShapeCount);
}

void CREImporter::GetActorIWD1(Actor *act) //9.0
{
	int i;
	ieByte tmpByte;
	ieWord tmpWord;

	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_REPUTATION]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_HIDEINSHADOWS]=tmpByte;
	str->ReadWord( &tmpWord );
	//skipping a word
	str->ReadWord( &tmpWord );
	act->AC.SetNatural((ieWordSigned) tmpWord);
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACCRUSHINGMOD]=(ieWordSigned) tmpWord;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACMISSILEMOD]=(ieWordSigned) tmpWord;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACPIERCINGMOD]=(ieWordSigned) tmpWord;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_ACSLASHINGMOD]=(ieWordSigned) tmpWord;
	str->Read( &tmpByte, 1 );
	act->ToHit.SetBase((ieByteSigned) tmpByte);
	str->Read( &tmpByte, 1 );
	tmpByte = tmpByte * 2;
	if (tmpByte>10) tmpByte-=11;
	act->BaseStats[IE_NUMBEROFATTACKS]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSDEATH]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSWANDS]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSPOLY]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSBREATH]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SAVEVSSPELL]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTFIRE]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTCOLD]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTELECTRICITY]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTACID]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMAGIC]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMAGICFIRE]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMAGICCOLD]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTSLASHING]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTCRUSHING]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTPIERCING]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_RESISTMISSILE]=(ieByteSigned) tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_DETECTILLUSIONS]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SETTRAPS]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LORE]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LOCKPICKING]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_STEALTH]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_TRAPS]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_PICKPOCKET]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_FATIGUE]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_INTOXICATION]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LUCK]=(ieByteSigned) tmpByte;
	for (i=0;i<21;i++) {
		str->Read( &tmpByte, 1 );
		act->BaseStats[IE_PROFICIENCYBASTARDSWORD+i]=tmpByte;
	}
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_TRACKING]=tmpByte;
	str->Seek( 32, GEM_CURRENT_POS );
	for (i=0;i<100;i++) {
		str->ReadDword( &act->StrRefs[i] );
	}
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LEVEL]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LEVEL2]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LEVEL3]=tmpByte;
	//this is rumoured to be IE_SEX, but we use the gender field for this
	str->Read( &tmpByte, 1 );
	//skipping a byte
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_STR]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_STREXTRA]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_INT]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_WIS]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_DEX]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_CON]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_CHR]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_MORALE]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_MORALEBREAK]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_HATEDRACE]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_MORALERECOVERYTIME]=tmpByte;
	str->Read( &tmpByte, 1 );
	//skipping a byte
	str->ReadDword( &act->BaseStats[IE_KIT] );
	act->BaseStats[IE_KIT] = ((act->BaseStats[IE_KIT] & 0xffff) << 16) +
		((act->BaseStats[IE_KIT] & 0xffff0000) >> 16);
	ReadScript(act, SCR_OVERRIDE);
	ReadScript(act, SCR_CLASS);
	ReadScript(act, SCR_RACE);
	ReadScript(act, SCR_GENERAL);
	ReadScript(act, SCR_DEFAULT);
	//new scripting flags, one on each byte
	str->Read( &tmpByte, 1); //hidden
	if (tmpByte) {
		act->BaseStats[IE_AVATARREMOVAL]=tmpByte;
	}
	str->Read( &act->SetDeathVar, 1); //set death variable
	str->Read( &act->IncKillCount, 1); //increase kill count
	str->Read( &act->UnknownField, 1);
	for (i = 0; i<5; i++) {
		str->ReadWord( &tmpWord );
		act->BaseStats[IE_INTERNAL_0+i]=tmpWord;
	}
	ieVariable KillVar;
	str->Read(KillVar,32); //use these as needed
	KillVar[32]=0;
	strnspccpy(act->KillVar, KillVar, 32);
	str->Read(KillVar,32);
	KillVar[32]=0;
	strnspccpy(act->IncKillVar, KillVar, 32);
	str->Seek( 2, GEM_CURRENT_POS);
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_SAVEDXPOS] = tmpWord;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_SAVEDYPOS] = tmpWord;
	str->ReadWord( &tmpWord );
	act->BaseStats[IE_SAVEDFACE] = tmpWord;
	str->Seek( 18, GEM_CURRENT_POS );
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_EA] = tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_GENERAL] = tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_RACE] = tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_CLASS] = tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_SPECIFIC] = tmpByte;
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_SEX] = tmpByte;
	str->Seek( 5, GEM_CURRENT_POS );
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_ALIGNMENT]=tmpByte;
	str->Seek( 4, GEM_CURRENT_POS );
	ieVariable scriptname;
	str->Read( scriptname, 32);
	scriptname[32]=0;
	act->SetScriptName(scriptname);

	str->ReadDword( &KnownSpellsOffset );
	str->ReadDword( &KnownSpellsCount );
	str->ReadDword( &SpellMemorizationOffset );
	str->ReadDword( &SpellMemorizationCount );
	str->ReadDword( &MemorizedSpellsOffset );
	str->ReadDword( &MemorizedSpellsCount );

	str->ReadDword( &ItemSlotsOffset );
	str->ReadDword( &ItemsOffset );
	str->ReadDword( &ItemsCount );
	str->ReadDword( &EffectsOffset );
	str->ReadDword( &EffectsCount );

	ReadDialog(act);
}

int CREImporter::GetStoredFileSize(Actor *actor)
{
	int headersize;
	unsigned int Inventory_Size;
	unsigned int i;

	CREVersion = actor->version;
	switch (CREVersion) {
		case IE_CRE_GEMRB:
			headersize = 0x2d4;
			//minus fist
			Inventory_Size=actor->inventory.GetSlotCount()-1;
			TotSCEFF = 1;
			break;
		case IE_CRE_V1_1://totsc/bg2/tob (still V1.0, but large effects)
		case IE_CRE_V1_0://bg1
			headersize = 0x2d4;
			Inventory_Size=38;
			//we should know it is bg1
			if (actor->version == IE_CRE_V1_1) {
				TotSCEFF = 1;
			} else {
				TotSCEFF = 0;
			}
			break;
		case IE_CRE_V1_2: //pst
			headersize = 0x378;
			Inventory_Size=46;
			TotSCEFF = 0;
			break;
		case IE_CRE_V2_2://iwd2
			headersize = 0x62e; //with offsets
			Inventory_Size=50;
			TotSCEFF = 1;
			break;
		case IE_CRE_V9_0://iwd
			headersize = 0x33c;
			Inventory_Size=38;
			TotSCEFF = 1;
			break;
		default:
			return -1;
	}
	KnownSpellsOffset = headersize;

	if (actor->version==IE_CRE_V2_2) { //iwd2
		int type, level;

		for (type=IE_IWD2_SPELL_BARD;type<IE_IWD2_SPELL_DOMAIN;type++) for(level=0;level<9;level++) {
			headersize += GetIWD2SpellpageSize(actor, (ieIWD2SpellType) type, level)*16+8;
		}
		for(level=0;level<9;level++) {
			headersize += GetIWD2SpellpageSize(actor, IE_IWD2_SPELL_DOMAIN, level)*16+8;
		}
		for (type=IE_IWD2_SPELL_INNATE;type<NUM_IWD2_SPELLTYPES;type++) {
			headersize += GetIWD2SpellpageSize(actor, (ieIWD2SpellType) type, 0)*16+8;
		}
	} else {//others
		//adding known spells
		KnownSpellsCount = actor->spellbook.GetTotalKnownSpellsCount();
		headersize += KnownSpellsCount * 12;
		SpellMemorizationOffset = headersize;

		//adding spell pages
		SpellMemorizationCount = actor->spellbook.GetTotalPageCount();
		headersize += SpellMemorizationCount * 16;
		MemorizedSpellsOffset = headersize;

		MemorizedSpellsCount = actor->spellbook.GetTotalMemorizedSpellsCount();
		headersize += MemorizedSpellsCount * 12;
	}
	EffectsOffset = headersize;

	//adding effects
	EffectsCount = actor->fxqueue.GetSavedEffectsCount();
	VariablesCount = actor->locals->GetCount();
	if (VariablesCount) {
		TotSCEFF=1;
	}
	if (TotSCEFF) {
		headersize += (VariablesCount + EffectsCount) * 264;
	} else {
		//if there are variables, then TotSCEFF is set
		headersize += EffectsCount * 48;
	}
	ItemsOffset = headersize;

	//counting items (calculating item storage)
	ItemsCount = 0;
	for (i=0;i<Inventory_Size;i++) {
		unsigned int j = core->QuerySlot(i+1);
		CREItem *it = actor->inventory.GetSlotItem(j);
		if (it) {
			ItemsCount++;
		}
	}
	headersize += ItemsCount * 20;
	ItemSlotsOffset = headersize;

	//adding itemslot table size and equipped slot fields
	return headersize + (Inventory_Size)*sizeof(ieWord)+sizeof(ieWord)*2;
}

int CREImporter::PutInventory(DataStream *stream, Actor *actor, unsigned int size)
{
	unsigned int i;
	ieDword tmpDword;
	ieWord tmpWord;
	ieWord ItemCount = 0;
	ieWord *indices =(ieWord *) malloc(size*sizeof(ieWord) );

	for (i=0;i<size;i++) {
		indices[i]=(ieWord) -1;
	}

	for (i=0;i<size;i++) {
		//ignore first element, getinventorysize makes space for fist
		unsigned int j = core->QuerySlot(i+1);
		CREItem *it = actor->inventory.GetSlotItem( j );
		if (!it) {
			continue;
		}
		stream->WriteResRef( it->ItemResRef);
		stream->WriteWord( &it->Expired);
		stream->WriteWord( &it->Usages[0]);
		stream->WriteWord( &it->Usages[1]);
		stream->WriteWord( &it->Usages[2]);
		tmpDword = it->Flags;
		//IWD uses this bit differently
		if (MagicBit) {
			if (it->Flags&IE_INV_ITEM_MAGICAL) {
				tmpDword|=IE_INV_ITEM_UNDROPPABLE;
			} else {
				tmpDword&=~IE_INV_ITEM_UNDROPPABLE;
			}
		}
		stream->WriteDword( &tmpDword);
		indices[i] = ItemCount++;
	}
	for (i=0;i<size;i++) {
		stream->WriteWord( indices+i);
	}
	tmpWord = (ieWord) actor->inventory.GetEquipped();
	stream->WriteWord( &tmpWord);
	tmpWord = (ieWord) actor->inventory.GetEquippedHeader();
	stream->WriteWord( &tmpWord);
	free(indices);
	return 0;
}

int CREImporter::PutHeader(DataStream *stream, Actor *actor)
{
	char Signature[8];
	ieByte tmpByte;
	ieWord tmpWord;
	ieDword tmpDword;
	int i;
	char filling[51];

	memset(filling,0,sizeof(filling));
	memcpy( Signature, "CRE V0.0", 8);
	Signature[5]+=CREVersion/10;
	if (actor->version!=IE_CRE_V1_1) {
		Signature[7]+=CREVersion%10;
	}
	stream->Write( Signature, 8);
	stream->WriteDword( &actor->LongStrRef);
	stream->WriteDword( &actor->ShortStrRef);
	stream->WriteDword( &actor->BaseStats[IE_MC_FLAGS]);
	stream->WriteDword( &actor->BaseStats[IE_XPVALUE]);
	stream->WriteDword( &actor->BaseStats[IE_XP]);
	stream->WriteDword( &actor->BaseStats[IE_GOLD]);
	stream->WriteDword( &actor->BaseStats[IE_STATE_ID]);
	tmpWord = actor->BaseStats[IE_HITPOINTS];
	//decrease the hp back to the one without constitution bonus
	// (but only player classes can have it)
	tmpWord = (ieWord) (tmpWord - actor->GetHpAdjustment(actor->GetXPLevel(false)));
	stream->WriteWord( &tmpWord);
	tmpWord = actor->BaseStats[IE_MAXHITPOINTS];
	stream->WriteWord( &tmpWord);
	stream->WriteDword( &actor->BaseStats[IE_ANIMATION_ID]);
	for (i=0;i<7;i++) {
		Signature[i] = (char) actor->BaseStats[IE_COLORS+i];
	}
	//old effect type
	Signature[7] = TotSCEFF;
	stream->Write( Signature, 8);
	stream->WriteResRef( actor->SmallPortrait);
	stream->WriteResRef( actor->LargePortrait);
	tmpByte = actor->BaseStats[IE_REPUTATION];
	stream->Write( &tmpByte, 1 );
	tmpByte = actor->BaseStats[IE_HIDEINSHADOWS];
	stream->Write( &tmpByte, 1 );
	//from here it differs, slightly
	tmpWord = actor->AC.GetNatural();
	stream->WriteWord( &tmpWord);
	//iwd2 doesn't store this a second time,
	//probably gemrb format shouldn't either?
	if (actor->version != IE_CRE_V2_2) {
		tmpWord = actor->AC.GetNatural();
		stream->WriteWord( &tmpWord);
	}
	tmpWord = actor->BaseStats[IE_ACCRUSHINGMOD];
	stream->WriteWord( &tmpWord);
	tmpWord = actor->BaseStats[IE_ACMISSILEMOD];
	stream->WriteWord( &tmpWord);
	tmpWord = actor->BaseStats[IE_ACPIERCINGMOD];
	stream->WriteWord( &tmpWord);
	tmpWord = actor->BaseStats[IE_ACSLASHINGMOD];
	stream->WriteWord( &tmpWord);
	tmpByte = actor->ToHit.GetBase();
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_NUMBEROFATTACKS];
	if (actor->version == IE_CRE_V2_2) {
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_SAVEFORTITUDE];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_SAVEREFLEX];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_SAVEWILL];
		stream->Write( &tmpByte, 1);
	} else {
		if (actor->version!=IE_CRE_GEMRB) {
			if (tmpByte&1) tmpByte = tmpByte/2+6;
			else tmpByte /=2;
		}
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_SAVEVSDEATH];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_SAVEVSWANDS];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_SAVEVSPOLY];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_SAVEVSBREATH];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_SAVEVSSPELL];
		stream->Write( &tmpByte, 1);
	}
	tmpByte = actor->BaseStats[IE_RESISTFIRE];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_RESISTCOLD];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_RESISTELECTRICITY];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_RESISTACID];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_RESISTMAGIC];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_RESISTMAGICFIRE];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_RESISTMAGICCOLD];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_RESISTSLASHING];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_RESISTCRUSHING];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_RESISTPIERCING];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_RESISTMISSILE];
	stream->Write( &tmpByte, 1);
	if (actor->version == IE_CRE_V2_2) {
		tmpByte = actor->BaseStats[IE_MAGICDAMAGERESISTANCE];
		stream->Write( &tmpByte, 1);
		stream->Write( Signature, 4);
	} else {
		tmpByte = actor->BaseStats[IE_DETECTILLUSIONS];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_SETTRAPS];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_LORE];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_LOCKPICKING];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_STEALTH];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_TRAPS];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_PICKPOCKET];
		stream->Write( &tmpByte, 1);
	}
	tmpByte = actor->BaseStats[IE_FATIGUE];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_INTOXICATION];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_LUCK];
	stream->Write( &tmpByte, 1);

	if (actor->version == IE_CRE_V2_2) {
		//this is rather fuzzy
		//turnundead level, + 33 bytes of zero
		tmpByte = actor->BaseStats[IE_TURNUNDEADLEVEL];
		stream->Write(&tmpByte, 1);
		stream->Write( filling,33);
		//total levels
		tmpByte = actor->BaseStats[IE_CLASSLEVELSUM];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_LEVELBARBARIAN];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_LEVELBARD];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_LEVELCLERIC];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_LEVELDRUID];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_LEVELFIGHTER];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_LEVELMONK];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_LEVELPALADIN];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_LEVELRANGER];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_LEVELTHIEF];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_LEVELSORCERER];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_LEVELMAGE];
		stream->Write( &tmpByte, 1);
		//some stuffing
		stream->Write( filling, 22);
		//string references
		for (i=0;i<64;i++) {
			stream->WriteDword( &actor->StrRefs[i]);
		}
		stream->WriteResRef( actor->Scripts[SCR_AREA]->GetName() );
		stream->WriteResRef( actor->Scripts[SCR_RESERVED]->GetName() );
		//unknowns before feats
		stream->Write( filling,4);
		//feats
		stream->WriteDword( &actor->BaseStats[IE_FEATS1]);
		stream->WriteDword( &actor->BaseStats[IE_FEATS2]);
		stream->WriteDword( &actor->BaseStats[IE_FEATS3]);
		stream->Write( filling, 12);
		//proficiencies
		for (i=0;i<26;i++) {
			tmpByte = actor->BaseStats[IE_PROFICIENCYBASTARDSWORD+i];
			stream->Write( &tmpByte, 1);
		}
		stream->Write( filling, 38);
		//alchemy
		tmpByte = actor->BaseStats[IE_ALCHEMY];
		stream->Write( &tmpByte, 1);
		//animals
		tmpByte = actor->BaseStats[IE_ANIMALS];
		stream->Write( &tmpByte, 1);
		//bluff
		tmpByte = actor->BaseStats[IE_BLUFF];
		stream->Write( &tmpByte, 1);
		//concentration
		tmpByte = actor->BaseStats[IE_CONCENTRATION];
		stream->Write( &tmpByte, 1);
		//diplomacy
		tmpByte = actor->BaseStats[IE_DIPLOMACY];
		stream->Write( &tmpByte, 1);
		//disarm trap
		tmpByte = actor->BaseStats[IE_TRAPS];
		stream->Write( &tmpByte, 1);
		//hide
		tmpByte = actor->BaseStats[IE_HIDEINSHADOWS];
		stream->Write( &tmpByte, 1);
		//intimidate
		tmpByte = actor->BaseStats[IE_INTIMIDATE];
		stream->Write( &tmpByte, 1);
		//lore
		tmpByte = actor->BaseStats[IE_LORE];
		stream->Write( &tmpByte, 1);
		//move silently
		tmpByte = actor->BaseStats[IE_STEALTH];
		stream->Write( &tmpByte, 1);
		//open lock
		tmpByte = actor->BaseStats[IE_LOCKPICKING];
		stream->Write( &tmpByte, 1);
		//pickpocket
		tmpByte = actor->BaseStats[IE_PICKPOCKET];
		stream->Write( &tmpByte, 1);
		//search
		tmpByte = actor->BaseStats[IE_SEARCH];
		stream->Write( &tmpByte, 1);
		//spellcraft
		tmpByte = actor->BaseStats[IE_SPELLCRAFT];
		stream->Write( &tmpByte, 1);
		//use magic device
		tmpByte = actor->BaseStats[IE_MAGICDEVICE];
		stream->Write( &tmpByte, 1);
		//tracking
		tmpByte = actor->BaseStats[IE_TRACKING];
		stream->Write( &tmpByte, 1);
		stream->Write( filling, 50);
		tmpByte = actor->BaseStats[IE_CR];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_HATEDRACE];
		stream->Write( &tmpByte, 1);
		for (i=0;i<7;i++) {
			tmpByte = actor->BaseStats[IE_HATEDRACE2+i];
			stream->Write( &tmpByte, 1);
		}
		tmpByte = actor->BaseStats[IE_SUBRACE];
		stream->Write( &tmpByte, 1);
		stream->Write( filling, 1); //unknown
		tmpByte = actor->BaseStats[IE_SEX]; //
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_STR];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_INT];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_WIS];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_DEX];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_CON];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_CHR];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_MORALE];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_MORALEBREAK];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_MORALERECOVERYTIME];
		stream->Write( &tmpByte, 1);
		// unknown byte
		stream->Write( &filling,1);
		// no kit word order magic for iwd2
		stream->WriteDword( &actor->BaseStats[IE_KIT] );
		stream->WriteResRef( actor->Scripts[SCR_OVERRIDE]->GetName() );
		stream->WriteResRef( actor->Scripts[SCR_CLASS]->GetName() );
		stream->WriteResRef( actor->Scripts[SCR_RACE]->GetName() );
		stream->WriteResRef( actor->Scripts[SCR_GENERAL]->GetName() );
		stream->WriteResRef( actor->Scripts[SCR_DEFAULT]->GetName() );
	} else {
		for (i=0;i<21;i++) {
			tmpByte = actor->BaseStats[IE_PROFICIENCYBASTARDSWORD+i];
			stream->Write( &tmpByte, 1);
		}
		tmpByte = actor->BaseStats[IE_TRACKING];
		stream->Write( &tmpByte, 1);
		stream->Write( filling, 32);
		for (i=0;i<100;i++) {
			stream->WriteDword( &actor->StrRefs[i]);
		}
		tmpByte = actor->BaseStats[IE_LEVEL];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_LEVEL2];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_LEVEL3];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_SEX]; //
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_STR];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_STREXTRA];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_INT];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_WIS];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_DEX];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_CON];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_CHR];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_MORALE];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_MORALEBREAK];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_HATEDRACE];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_MORALERECOVERYTIME];
		stream->Write( &tmpByte, 1);
		// unknown byte
		stream->Write( &Signature,1);
		tmpDword = ((actor->BaseStats[IE_KIT] & 0xffff) << 16) +
			((actor->BaseStats[IE_KIT] & 0xffff0000) >> 16);
		stream->WriteDword( &tmpDword );
		stream->WriteResRef( actor->Scripts[SCR_OVERRIDE]->GetName() );
		stream->WriteResRef( actor->Scripts[SCR_CLASS]->GetName() );
		stream->WriteResRef( actor->Scripts[SCR_RACE]->GetName() );
		stream->WriteResRef( actor->Scripts[SCR_GENERAL]->GetName() );
		stream->WriteResRef( actor->Scripts[SCR_DEFAULT]->GetName() );
	}
	//now follows the fuzzy part in separate putactor... functions
	return 0;
}

int CREImporter::PutActorGemRB(DataStream *stream, Actor *actor, ieDword InvSize)
{
	ieByte tmpByte;
	char filling[5];

	memset(filling,0,sizeof(filling));
	//similar in all engines
	tmpByte = actor->BaseStats[IE_EA];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_GENERAL];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_RACE];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_CLASS];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_SPECIFIC];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_SEX];
	stream->Write( &tmpByte, 1);
	stream->Write( filling, 5); //unknown bytes
	tmpByte = actor->BaseStats[IE_ALIGNMENT];
	stream->Write( &tmpByte, 1);
	stream->WriteDword( &InvSize ); //saving the inventory size to this unused part
	stream->Write( actor->GetScriptName(), 32);
	return 0;
}

int CREImporter::PutActorBG(DataStream *stream, Actor *actor)
{
	ieByte tmpByte;
	char filling[5];

	memset(filling,0,sizeof(filling));
	//similar in all engines
	tmpByte = actor->BaseStats[IE_EA];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_GENERAL];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_RACE];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_CLASS];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_SPECIFIC];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_SEX];
	stream->Write( &tmpByte, 1);
	stream->Write( filling, 5); //unknown bytes
	tmpByte = actor->BaseStats[IE_ALIGNMENT];
	stream->Write( &tmpByte, 1);
	stream->Write( filling,4); //this is called ID in iwd2, and contains 2 words
	stream->Write( actor->GetScriptName(), 32);
	return 0;
}

int CREImporter::PutActorPST(DataStream *stream, Actor *actor)
{
	ieByte tmpByte;
	ieWord tmpWord;
	int i;
	char filling[44];

	memset(filling,0,sizeof(filling));
	stream->Write(filling, 44); //11*4 totally unknown
	stream->WriteDword( &actor->BaseStats[IE_XP_MAGE]);
	stream->WriteDword( &actor->BaseStats[IE_XP_THIEF]);
	for (i = 0; i<10; i++) {
		tmpWord = actor->BaseStats[IE_INTERNAL_0];
		stream->WriteWord( &tmpWord );
	}
	for (i = 0; i<4; i++) {
		tmpByte = (ieByte) (actor->DeathCounters[i]);
		stream->Write( &tmpByte, 1);
	}
	stream->Write( actor->KillVar, 32);
	stream->Write( filling,3); //unknown
	tmpByte=actor->BaseStats[IE_COLORCOUNT];
	stream->Write( &tmpByte, 1);
	stream->WriteDword( &actor->AppearanceFlags);

	for (i=0;i<7;i++) {
		tmpWord = actor->BaseStats[IE_COLORS+i];
		stream->WriteWord( &tmpWord);
	}
	stream->Write(filling,31);
	tmpByte = actor->BaseStats[IE_SPECIES];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_TEAM];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_FACTION];
	stream->Write( &tmpByte, 1);
	//similar in all engines
	tmpByte = actor->BaseStats[IE_EA];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_GENERAL];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_RACE];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_CLASS];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_SPECIFIC];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_SEX];
	stream->Write( &tmpByte, 1);
	stream->Write( filling, 5); //unknown bytes
	tmpByte = actor->BaseStats[IE_ALIGNMENT];
	stream->Write( &tmpByte, 1);
	stream->Write( filling,4); //this is called ID in iwd2, and contains 2 words
	stream->Write( actor->GetScriptName(), 32);
	return 0;
}

int CREImporter::PutActorIWD1(DataStream *stream, Actor *actor)
{
	ieByte tmpByte;
	ieWord tmpWord;
	int i;
	char filling[52];

	memset(filling,0,sizeof(filling));
	tmpByte=(ieByte) actor->BaseStats[IE_AVATARREMOVAL];
	stream->Write( &tmpByte, 1);
	stream->Write( &actor->SetDeathVar, 1);
	stream->Write( &actor->IncKillCount, 1);
	stream->Write( &actor->UnknownField, 1); //unknown
	for (i=0;i<5;i++) {
		tmpWord = actor->BaseStats[IE_INTERNAL_0+i];
		stream->WriteWord( &tmpWord);
	}
	stream->Write( actor->KillVar, 32); //some variable names in iwd
	stream->Write( actor->IncKillVar, 32); //some variable names in iwd
	stream->Write( filling, 2);
	tmpWord = actor->BaseStats[IE_SAVEDXPOS];
	stream->WriteWord( &tmpWord);
	tmpWord = actor->BaseStats[IE_SAVEDYPOS];
	stream->WriteWord( &tmpWord);
	tmpWord = actor->BaseStats[IE_SAVEDFACE];
	stream->WriteWord( &tmpWord);
	stream->Write( filling, 18);
	//similar in all engines
	tmpByte = actor->BaseStats[IE_EA];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_GENERAL];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_RACE];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_CLASS];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_SPECIFIC];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_SEX];
	stream->Write( &tmpByte, 1);
	stream->Write( filling, 5); //unknown bytes
	tmpByte = actor->BaseStats[IE_ALIGNMENT];
	stream->Write( &tmpByte, 1);
	stream->Write( filling,4); //this is called ID in iwd2, and contains 2 words
	stream->Write( actor->GetScriptName(), 32);
	return 0;
}

int CREImporter::PutActorIWD2(DataStream *stream, Actor *actor)
{
	ieByte tmpByte;
	ieWord tmpWord;
	int i;
	char filling[124];

	memset(filling,0,sizeof(filling));
	tmpByte=(ieByte) actor->BaseStats[IE_AVATARREMOVAL];
	stream->Write( &tmpByte, 1);
	stream->Write( &actor->SetDeathVar, 1);
	stream->Write( &actor->IncKillCount, 1);
	stream->Write( &actor->UnknownField, 1); //unknown
	for (i=0;i<5;i++) {
		tmpWord = actor->BaseStats[IE_INTERNAL_0+i];
		stream->WriteWord( &tmpWord);
	}
	stream->Write( actor->KillVar, 32); //some variable names in iwd
	stream->Write( actor->IncKillVar, 32); //some variable names in iwd
	stream->Write( filling, 2);
	tmpWord = actor->BaseStats[IE_SAVEDXPOS];
	stream->WriteWord( &tmpWord);
	tmpWord = actor->BaseStats[IE_SAVEDYPOS];
	stream->WriteWord( &tmpWord);
	tmpWord = actor->BaseStats[IE_SAVEDFACE];
	stream->WriteWord( &tmpWord);
	stream->Write( filling, 15);
	tmpByte = actor->BaseStats[IE_TRANSLUCENT];
	stream->Write(&tmpByte, 1);
	stream->Write( filling, 5); //fade speed, spec flags, invisible
	tmpByte = actor->BaseStats[IE_UNUSED_SKILLPTS];
	stream->Write( &tmpByte, 1);
	stream->Write( filling, 124);
	//similar in all engines
	tmpByte = actor->BaseStats[IE_EA];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_GENERAL];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_RACE];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_CLASS];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_SPECIFIC];
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_SEX];
	stream->Write( &tmpByte, 1);
	stream->Write( filling, 5); //unknown bytes
	tmpByte = actor->BaseStats[IE_ALIGNMENT];
	stream->Write( &tmpByte, 1);
	stream->Write( filling,4); //this is called ID in iwd2, and contains 2 words
	stream->Write( actor->GetScriptName(), 32);
	//3 unknown words
	stream->WriteWord( &tmpWord);
	stream->WriteWord( &tmpWord);
	stream->WriteWord( &tmpWord);
	return 0;
}

int CREImporter::PutKnownSpells( DataStream *stream, Actor *actor)
{
	int type=actor->spellbook.GetTypes();
	for (int i=0;i<type;i++) {
		unsigned int level = actor->spellbook.GetSpellLevelCount(i);
		for (unsigned int j=0;j<level;j++) {
			unsigned int count = actor->spellbook.GetKnownSpellsCount(i, j);
			for (unsigned int k=0;k<count;k++) {
				CREKnownSpell *ck = actor->spellbook.GetKnownSpell(i, j, k);
				stream->WriteResRef(ck->SpellResRef);
				stream->WriteWord( &ck->Level);
				stream->WriteWord( &ck->Type);
			}
		}
	}
	return 0;
}

int CREImporter::PutSpellPages( DataStream *stream, Actor *actor)
{
	ieWord tmpWord;
	ieDword tmpDword;
	ieDword SpellIndex = 0;

	int type=actor->spellbook.GetTypes();
	for (int i=0;i<type;i++) {
		unsigned int level = actor->spellbook.GetSpellLevelCount(i);
		for (unsigned int j=0;j<level;j++) {
			tmpWord = j; //+1
			stream->WriteWord( &tmpWord);
			tmpWord = actor->spellbook.GetMemorizableSpellsCount(i,j,false);
			stream->WriteWord( &tmpWord);
			tmpWord = actor->spellbook.GetMemorizableSpellsCount(i,j,true);
			stream->WriteWord( &tmpWord);
			tmpWord = i;
			stream->WriteWord( &tmpWord);
			stream->WriteDword( &SpellIndex);
			tmpDword = actor->spellbook.GetMemorizedSpellsCount(i,j, false);
			stream->WriteDword( &tmpDword);
			SpellIndex += tmpDword;
		}
	}
	return 0;
}

int CREImporter::PutMemorizedSpells(DataStream *stream, Actor *actor)
{
	int type=actor->spellbook.GetTypes();
	for (int i=0;i<type;i++) {
		unsigned int level = actor->spellbook.GetSpellLevelCount(i);
		for (unsigned int j=0;j<level;j++) {
			unsigned int count = actor->spellbook.GetMemorizedSpellsCount(i,j, false);
			for (unsigned int k=0;k<count;k++) {
				CREMemorizedSpell *cm = actor->spellbook.GetMemorizedSpell(i,j,k);

				stream->WriteResRef( cm->SpellResRef);
				stream->WriteDword( &cm->Flags);
			}
		}
	}
	return 0;
}

int CREImporter::PutEffects( DataStream *stream, Actor *actor)
{
	PluginHolder<EffectMgr> eM(IE_EFF_CLASS_ID);
	assert(eM != NULL);

	std::list< Effect* >::const_iterator f=actor->fxqueue.GetFirstEffect();
	for(unsigned int i=0;i<EffectsCount;i++) {
		const Effect *fx = actor->fxqueue.GetNextSavedEffect(f);

		assert(fx!=NULL);

		if (TotSCEFF) {
			eM->PutEffectV2(stream, fx);
		} else {
			ieWord tmpWord;
			ieByte tmpByte;
			char filling[60];

			memset(filling,0,sizeof(filling) );

			tmpWord = (ieWord) fx->Opcode;
			stream->WriteWord( &tmpWord);
			tmpByte = (ieByte) fx->Target;
			stream->Write(&tmpByte, 1);
			tmpByte = (ieByte) fx->Power;
			stream->Write(&tmpByte, 1);
			stream->WriteDword( &fx->Parameter1);
			stream->WriteDword( &fx->Parameter2);
			tmpByte = (ieByte) fx->TimingMode;
			stream->Write(&tmpByte, 1);
			tmpByte = (ieByte) fx->Resistance;
			stream->Write(&tmpByte, 1);
			stream->WriteDword( &fx->Duration);
			tmpByte = (ieByte) fx->ProbabilityRangeMax;
			stream->Write(&tmpByte, 1);
			tmpByte = (ieByte) fx->ProbabilityRangeMin;
			stream->Write(&tmpByte, 1);
			stream->Write(fx->Resource, 8);
			stream->WriteDword( &fx->DiceThrown );
			stream->WriteDword( &fx->DiceSides );
			stream->WriteDword( &fx->SavingThrowType );
			stream->WriteDword( &fx->SavingThrowBonus );
			//isvariable
			stream->Write( filling,4 );
		}
	}
	return 0;
}

//add as effect!
int CREImporter::PutVariables( DataStream *stream, Actor *actor)
{
	char filling[104];
	Variables::iterator pos=NULL;
	const char *name;
	ieDword tmpDword, value;

	for (unsigned int i=0;i<VariablesCount;i++) {
		memset(filling,0,sizeof(filling) );
		pos = actor->locals->GetNextAssoc( pos, name, value);
		stream->Write(filling,8);
		tmpDword = FAKE_VARIABLE_OPCODE;
		stream->WriteDword( &tmpDword);
		stream->Write(filling,8); //type, power
		stream->WriteDword( &value); //param #1
		stream->Write(filling,4); //param #2
		//HoW has an assertion to ensure timing is nonzero (even for variables)
		value = 1;
		stream->WriteDword( &value); //timing
		//duration, chance, resource, dices, saves
		stream->Write( filling, 32);
		tmpDword = FAKE_VARIABLE_MARKER;
		stream->WriteDword( &tmpDword); //variable marker
		stream->Write( filling, 92); //23 * 4
		strnspccpy(filling, name, 32);
		stream->Write( filling, 104); //32 + 72
	}
	return 0;
}

//Don't forget to add 8 for the totals/bonus fields
ieDword CREImporter::GetIWD2SpellpageSize(Actor *actor, ieIWD2SpellType type, int level) const
{
	CRESpellMemorization* sm = actor->spellbook.GetSpellMemorization(type, level);
	ieDword cnt = sm->known_spells.size();
	return cnt;
}

int CREImporter::PutIWD2Spellpage(DataStream *stream, Actor *actor, ieIWD2SpellType type, int level)
{
	ieDword ID, max, known;

	CRESpellMemorization* sm = actor->spellbook.GetSpellMemorization(type, level);
	for (unsigned int k = 0; k < sm->known_spells.size(); k++) {
		CREKnownSpell *ck = sm->known_spells[k];
		ID = ResolveSpellName(ck->SpellResRef, level, type);
		stream->WriteDword ( &ID);
		max = actor->spellbook.CountSpells(ck->SpellResRef, type, 1);
		known = actor->spellbook.CountSpells(ck->SpellResRef, type, 0);
		stream->WriteDword ( &max);
		stream->WriteDword ( &known);
		//unknown field (always 0)
		known = 0;
		stream->WriteDword (&known);
	}

	max = sm->SlotCount;
	known = sm->SlotCountWithBonus;
	stream->WriteDword ( &max);
	stream->WriteDword ( &known);
	return 0;
}

/* this function expects GetStoredFileSize to be called before */
int CREImporter::PutActor(DataStream *stream, Actor *actor, bool chr)
{
	ieDword tmpDword=0;
	int ret;

	if (!stream || !actor) {
		return -1;
	}

	IsCharacter = chr;
	if (chr) {
		WriteChrHeader( stream, actor );
	}
	assert(TotSCEFF==0 || TotSCEFF==1);

	CREOffset = stream->GetPos(); // for asserts

	ret = PutHeader( stream, actor);
	if (ret) {
		return ret;
	}
	//here comes the fuzzy part
	ieDword Inventory_Size;

	switch (CREVersion) {
		case IE_CRE_GEMRB:
			//don't add fist
			Inventory_Size=(ieDword) actor->inventory.GetSlotCount()-1;
			ret = PutActorGemRB(stream, actor, Inventory_Size);
			break;
		case IE_CRE_V1_2:
			Inventory_Size=46;
			ret = PutActorPST(stream, actor);
			break;
		case IE_CRE_V1_1:
		case IE_CRE_V1_0: //bg1/bg2
			Inventory_Size=38;
			ret = PutActorBG(stream, actor);
			break;
		case IE_CRE_V2_2:
			Inventory_Size=50;
			ret = PutActorIWD2(stream, actor);
			break;
		case IE_CRE_V9_0:
			Inventory_Size=38;
			ret = PutActorIWD1(stream, actor);
			break;
		default:
			return -1;
	}
	if (ret) {
		return ret;
	}

	//writing offsets and counts
	if (actor->version==IE_CRE_V2_2) {
		int type, level;

		//class spells
		for (type=IE_IWD2_SPELL_BARD;type<IE_IWD2_SPELL_DOMAIN;type++) for(level=0;level<9;level++) {
			tmpDword = GetIWD2SpellpageSize(actor, (ieIWD2SpellType) type, level);
			stream->WriteDword(&KnownSpellsOffset);
			KnownSpellsOffset+=tmpDword*16+8;
		}
		for (type=IE_IWD2_SPELL_BARD;type<IE_IWD2_SPELL_DOMAIN;type++) for(level=0;level<9;level++) {
			tmpDword = GetIWD2SpellpageSize(actor, (ieIWD2SpellType) type, level);
			stream->WriteDword(&tmpDword);
		}
		//domain spells
		for (level=0;level<9;level++) {
			tmpDword = GetIWD2SpellpageSize(actor, IE_IWD2_SPELL_DOMAIN, level);
			stream->WriteDword(&KnownSpellsOffset);
			KnownSpellsOffset+=tmpDword*16+8;
		}
		for (level=0;level<9;level++) {
			tmpDword = GetIWD2SpellpageSize(actor, IE_IWD2_SPELL_DOMAIN, level);
			stream->WriteDword(&tmpDword);
		}
		//innates, shapes, songs
		for (type=IE_IWD2_SPELL_INNATE;type<NUM_IWD2_SPELLTYPES;type++) {
			tmpDword = GetIWD2SpellpageSize(actor, (ieIWD2SpellType) type, 0);
			stream->WriteDword(&KnownSpellsOffset);
			KnownSpellsOffset+=tmpDword*16+8;
			stream->WriteDword(&tmpDword);
		}
	} else {
		stream->WriteDword( &KnownSpellsOffset);
		stream->WriteDword( &KnownSpellsCount);
		stream->WriteDword( &SpellMemorizationOffset );
		stream->WriteDword( &SpellMemorizationCount );
		stream->WriteDword( &MemorizedSpellsOffset );
		stream->WriteDword( &MemorizedSpellsCount );
	}
	stream->WriteDword( &ItemSlotsOffset );
	stream->WriteDword( &ItemsOffset );
	stream->WriteDword( &ItemsCount );
	stream->WriteDword( &EffectsOffset );
	tmpDword = EffectsCount+VariablesCount;
	stream->WriteDword( &tmpDword );
	tmpDword = 0;
	stream->WriteResRef( actor->GetDialog(false) );
	//spells, spellbook etc

	if (actor->version==IE_CRE_V2_2) {
		int type, level;

		//writing out spell page headers
		for (type=IE_IWD2_SPELL_BARD;type<IE_IWD2_SPELL_DOMAIN;type++) for(level=0;level<9;level++) {
			PutIWD2Spellpage(stream, actor, (ieIWD2SpellType) type, level);
		}

		//writing out domain page headers
		for (level=0;level<9;level++) {
			PutIWD2Spellpage(stream, actor, IE_IWD2_SPELL_DOMAIN, level);
		}

		//innates, shapes, songs
		for (type = IE_IWD2_SPELL_INNATE; type<NUM_IWD2_SPELLTYPES; type ++) {
			PutIWD2Spellpage(stream, actor, (ieIWD2SpellType) type, 0);
		}
	} else {
		assert(stream->GetPos() == CREOffset+KnownSpellsOffset);
		ret = PutKnownSpells( stream, actor);
		if (ret) {
			return ret;
		}
		assert(stream->GetPos() == CREOffset+SpellMemorizationOffset);
		ret = PutSpellPages( stream, actor);
		if (ret) {
			return ret;
		}
		assert(stream->GetPos() == CREOffset+MemorizedSpellsOffset);
		ret = PutMemorizedSpells( stream, actor);
		if (ret) {
			return ret;
		}
	}

	assert(stream->GetPos() == CREOffset+EffectsOffset);
	ret = PutEffects(stream, actor);
	if (ret) {
		return ret;
	}
	//effects and variables
	ret = PutVariables(stream, actor);
	if (ret) {
		return ret;
	}

	//items and inventory slots
	assert(stream->GetPos() == CREOffset+ItemsOffset);
	ret = PutInventory( stream, actor, Inventory_Size);
	if (ret) {
		return ret;
	}
	return 0;
}

#include "plugindef.h"

GEMRB_PLUGIN(0xE507B60, "CRE File Importer")
PLUGIN_CLASS(IE_CRE_CLASS_ID, CREImporter)
PLUGIN_INITIALIZER(Initializer)
PLUGIN_CLEANUP(ReleaseMemoryCRE)
END_PLUGIN()
