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
#include "voodooconst.h"

#include "EffectMgr.h"
#include "GameData.h"
#include "Interface.h"
#include "PluginMgr.h"
#include "RNG.h"
#include "TableMgr.h"
#include "GameScript/GameScript.h"

#include <cassert>

using namespace GemRB;

std::map<ieDword, std::vector<unsigned char>> randcolors;

//one column, these don't have a level
static std::vector<ResRef> innlist; //IE_IWD2_SPELL_INNATE
static std::vector<ResRef> snglist; //IE_IWD2_SPELL_SONG
static std::vector<ResRef> shplist; //IE_IWD2_SPELL_SHAPE
static const ResRef EmptyResRef;

struct LevelAndKit
{
	unsigned int level;
	unsigned int kit;
	
	LevelAndKit(unsigned int level, unsigned int kit) noexcept
	: level(level), kit(kit)
	{}
};

class SpellEntry
{
public:
	const ResRef& GetSpell() const;
	const ResRef& FindSpell(unsigned int level, unsigned int kit) const;
	int FindSpell(unsigned int kit) const;
	bool Equals(const ResRef& spl) const;
	void SetSpell(const ResRef& spl);
	void AddLevel(unsigned int level, unsigned int kit);
private:
	ResRef spell;
	std::vector<LevelAndKit> levels;
};

const ResRef& SpellEntry::GetSpell() const
{
	return spell;
}

const ResRef& SpellEntry::FindSpell(unsigned int level, unsigned int kit) const
{
	for (const auto& entry : levels) {
		if (entry.kit == kit && entry.level == level) {
			return spell;
		}
	}
	return EmptyResRef;
}

int SpellEntry::FindSpell(unsigned int kit) const
{
	for (const auto& entry : levels) {
		if (entry.kit == kit) {
			return entry.level;
		}
	}
	return -1;
}

static int FindSpell(const ResRef& spellref, std::vector<SpellEntry*>& list)
{
	size_t i = list.size();
	while (i--) {
		if (list[i] && list[i]->Equals(spellref)) {
			return static_cast<int>(i);
		}
	}
	return -1;
}

bool SpellEntry::Equals(const ResRef& spl) const
{
	return spell == spl;
}

void SpellEntry::SetSpell(const ResRef& spl)
{
	spell = spl;
}

void SpellEntry::AddLevel(unsigned int level,unsigned int kit)
{
	if(!level) {
		return;
	}

	level--; // convert to 0-based for internal use
	for (const auto& entry : levels) {
		if (entry.kit == kit && entry.level == level) {
			Log(WARNING, "CREImporter", "Skipping duplicate spell list table entry for: {}", spell);
			return;
		}
	}
	
	levels.emplace_back(level, kit);
}

static int IsInnate(const ResRef& name)
{
	int innateCount = innlist.size();
	for (int i = 0; i < innateCount; i++) {
		if (name == innlist[i]) {
			return i;
		}
	}
	return -1;
}

static int IsSong(const ResRef& name)
{
	int sngCount = snglist.size();
	for (int i = 0; i < sngCount; i++) {
		if (name == snglist[i]) {
			return i;
		}
	}
	return -1;
}

static int IsShape(const ResRef& name)
{
	int shpCount = shplist.size();
	for (int i = 0; i < shpCount; i++) {
		if (name == shplist[i]) {
			return i;
		}
	}
	return -1;
}

static std::vector<SpellEntry*> splList;
static std::vector<SpellEntry*> domList;
static std::vector<SpellEntry*> magList;

static int IsDomain(const ResRef& name, unsigned short &level, unsigned int kit)
{
	size_t splCount = splList.size();
	for (size_t i = 0; i < splCount; i++) {
		if (domList[i] && domList[i]->Equals(name)) {
			int lev = domList[i]->FindSpell(kit);
			if (lev == -1) return -1;
			level = lev;
			return i;
		}
	}
	return -1;
}

/*static int IsSpecial(ResRef name, unsigned short &level, unsigned int kit)
{
	for(int i=0;i<magcount;i++) {
		if (maglist[i].Equals(name) ) {
			level = maglist[i].FindSpell(kit);
				return i;
		}
	}
	return -1;
}*/

ieWord CREImporter::FindSpellType(const ResRef& name, unsigned short &level, unsigned int clsMask, unsigned int kit) const
{
	level = 0;
	if (IsSong(name)>=0) return IE_IWD2_SPELL_SONG;
	if (IsShape(name)>=0) return IE_IWD2_SPELL_SHAPE;
	if (IsInnate(name)>=0) return IE_IWD2_SPELL_INNATE;
// there is no gui page for specialists spells, so let's skip them here
// otherwise their overlap causes bards and sorcerers to have their spells
// on the wizard page
//	if (IsSpecial(name, level, kit)>=0) return IE_IWD2_SPELL_WIZARD;

	// strict domain spell check, so we don't steal the spells from other books
	// still needs to happen first or the laxer check below can misclassify
	// first translate the actual kit to a column index to make them comparable
	// luckily they are in order
	int kit2 = std::log2(kit/0x8000); // 0x8000 is the first cleric kit
	if (IsDomain(name, level, kit2) >= 0) return IE_IWD2_SPELL_DOMAIN;

	// try harder for the rest
	for (const auto& spell : splList) {
		if (!spell || !spell->Equals(name)) {
			continue;
		}

		// iterate over table columns ("kits" - book types)
		for (ieWord type = IE_IWD2_SPELL_BARD; type < IE_IWD2_SPELL_DOMAIN; type++) {
			if (!(clsMask & (1 << type))) continue;

			int level2 = spell->FindSpell(type);
			if (level2 == -1) {
				Log(ERROR, "CREImporter", "Spell ({} of type {}) found without a level set! Using 1!", name, type);
				level2 = 0; // internal 0-indexed level
			}
			level = level2;
			// FIXME: returning the first will misplace spells for multiclasses
			return type;
		}
	}

	Log(ERROR, "CREImporter", "Could not find spell ({}) booktype! {}, {}!", name, clsMask, kit);
	// pseudorandom fallback
	return IE_IWD2_SPELL_WIZARD;
}

static int ResolveSpellName(const ResRef& name, int level, ieIWD2SpellType type)
{
	if (level>=MAX_SPELL_LEVEL) {
		return -1;
	}

	int ret;
	size_t splCount = splList.size();
	switch(type)
	{
	case IE_IWD2_SPELL_INNATE:
		ret = IsInnate(name);
		if (ret != -1) return ret;
		break;
	case IE_IWD2_SPELL_SONG:
		ret = IsSong(name);
		if (ret != -1) return ret;
		break;
	case IE_IWD2_SPELL_SHAPE:
		ret = IsShape(name);
		if (ret != -1) return ret;
		break;
	case IE_IWD2_SPELL_DOMAIN:
	default:
		for (size_t i = 0; i < splCount; i++) {
			if (splList[i] && splList[i]->Equals(name)) return static_cast<int>(i);
		}
	}
	return -1;
}

//input: index, level, type, kit
static const ResRef& ResolveSpellIndex(int index, int level, ieIWD2SpellType type, int kit)
{
	if (level>=MAX_SPELL_LEVEL) {
		return EmptyResRef;
	}

	switch (type) {
	case IE_IWD2_SPELL_INNATE:
		if (index >= static_cast<int>(innlist.size())) {
			return EmptyResRef;
		}
		return innlist[index];
	case IE_IWD2_SPELL_SONG:
		if (index >= static_cast<int>(snglist.size())) {
			return EmptyResRef;
		}
		return snglist[index];
	case IE_IWD2_SPELL_SHAPE:
		if (index >= static_cast<int>(shplist.size())) {
			return EmptyResRef;
		}
		return shplist[index];
	case IE_IWD2_SPELL_DOMAIN:
		if (index >= static_cast<int>(splList.size())) {
			return EmptyResRef;
		}
		// translate the actual kit to a column index to make them comparable
		// luckily they are in order
		kit = std::log2(kit/0x8000); // 0x8000 is the first cleric kit
		{
			const SpellEntry* entry = domList[index];
			if (entry) {
				const ResRef& ret = entry->FindSpell(level, kit);
				if (!ret.IsEmpty()) {
					return ret;
				}
			}
		}
		// sigh, retry with wizard spells, since the table does not cover everything npcs have
		kit = -1;
		type = IE_IWD2_SPELL_WIZARD;
		break;
	case IE_IWD2_SPELL_WIZARD:
		if (index >= static_cast<int>(splList.size())) {
			break;
		}
		// translate the actual kit to a column index to make them comparable
		kit = std::log2(kit/0x40); // 0x40 is the first mage kit
		//if it is a specialist spell, return it now
		{
			const SpellEntry* entry = magList[index];
			if (entry) {
				const ResRef& ret = entry->FindSpell(level, kit);
				if (!ret.IsEmpty()) {
					return ret;
				}
			}
		}
		//fall through
	default:
		kit = -1;
		//comes later
		break;
	}

	// type matches the table columns (0-bard to 6-wizard)
	const ResRef& ret = splList[index]->FindSpell(level, type);
	if (ret.IsEmpty()) {
		// some npcs have spells at odd levels, so the lookup just failed
		// eg. slayer knights of xvim with sppr325 at level 2 instead of 3
		Log(ERROR, "CREImporter", "Spell ({} of type {}) found at unexpected level ({})!", index, type, level);
		int level2 = splList[index]->FindSpell(type);
		// grrr, some rows have no levels set - they're all 0, but with a valid resref, so just return that
		if (level2 == -1) {
			Log(DEBUG, "CREImporter", "Spell entry ({}) without any levels set!", index);
			return splList[index]->GetSpell();
		}
		const ResRef& ret2 = splList[index]->FindSpell(level2, type);
		if (!ret2.IsEmpty()) {
			Log(DEBUG, "CREImporter", "The spell was found at level {}!", level2);
			return ret2;
		}
	}
	if (!ret.IsEmpty() || kit == -1) {
		return ret;
	}

	error("CREImporter", "Doing extra mage spell lookups!");
}

static void ReleaseMemoryCRE()
{
	randcolors.clear();

	for (auto spl : splList) {
		delete spl;
	}
	for (auto spl : domList) {
		delete spl;
	}
	for (auto spl : magList) {
		delete spl;
	}

	innlist.clear();
	snglist.clear();
	shplist.clear();
}

static void GetSpellTable(const ResRef& tableRef, std::vector<ResRef>& list)
{
	AutoTable tab = gamedata->LoadTable(tableRef);
	if (!tab) return;

	TableMgr::index_t column = tab->GetColumnCount() - 1;
	if (column == TableMgr::npos) return;

	TableMgr::index_t count = tab->GetRowCount();
	list.resize(count);
	for (TableMgr::index_t i = 0; i < count; ++i) {
		list[i] = tab->QueryField(i, column);
	}
}

// different tables, but all use listspll.2da for the spell indices
static void GetKitSpell(const ResRef& tableRef, std::vector<SpellEntry*>& list)
{
	AutoTable tab = gamedata->LoadTable(tableRef);
	if (!tab) return;

	TableMgr::index_t lastCol = tab->GetColumnCount() - 1; // the last column is not numeric, so we'll skip it
	if (lastCol < 1) {
		return;
	}

	TableMgr::index_t count = tab->GetRowCount();
	bool indexlist = false;
	if (tableRef == "listspll") {
		indexlist = true;
		list.resize(count);
	} else {
		list.resize(splList.size()); // needs to be the same size for the simple index lookup we do!
	}
	TableMgr::index_t index;
	for (TableMgr::index_t i = 0; i < count; ++i) {
		if (indexlist) {
			index = i;
		} else {
			// find the correct index in listspll.2da
			ResRef spellRef = tab->QueryField(i, lastCol);
			// the table has disabled spells in it and they all have the first two chars replaced by '*'
			if (IsStar(spellRef)) {
				continue;
			}
			index = FindSpell(spellRef, splList);
			assert (index != TableMgr::npos);
		}
		list[index] = new SpellEntry;
		list[index]->SetSpell(tab->QueryField(i, lastCol));
		for (TableMgr::index_t col = 0; col < lastCol; ++col) {
			list[index]->AddLevel(tab->QueryFieldSigned<int>(i, col), col);
		}
	}
}

static void InitSpellbook()
{
	if (!splList.empty()) {
		return;
	}

	if (core->HasFeature(GF_HAS_SPELLLIST)) {
		GetSpellTable("listinnt", innlist);
		GetSpellTable("listsong", snglist);
		GetSpellTable("listshap", shplist);
		GetKitSpell("listspll", splList); // need to init this one first, since the other two rely on it
		GetKitSpell("listmage", magList);
		GetKitSpell("listdomn", domList);
	}
}

CREImporter::CREImporter(void)
{
	InitSpellbook();
}

bool CREImporter::Import(DataStream* str)
{
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
		CREVersion = CREVersion::V1_0;
		return true;
	}
	if (strncmp( Signature, "CRE V1.2", 8 ) == 0) {
		CREVersion = CREVersion::V1_2;
		return true;
	}
	if (strncmp( Signature, "CRE V2.2", 8 ) == 0) {
		CREVersion = CREVersion::V2_2;
		return true;
	}
	if (strncmp( Signature, "CRE V9.0", 8 ) == 0) {
		CREVersion = CREVersion::V9_0;
		return true;
	}
	if (strncmp( Signature, "CRE V0.0", 8 ) == 0) {
		CREVersion = CREVersion::GemRB;
		return true;
	}

	Log(ERROR, "CREImporter", "Not a CRE File or File Version not supported: {}", Signature);
	return false;
}

void CREImporter::SetupSlotCounts()
{
	switch (CREVersion) {
		case CREVersion::V1_2: // pst
			QWPCount=4;
			QSPCount=3;
			QITCount=5;
			break;
		case CREVersion::GemRB: // own
			QWPCount=8;
			QSPCount=9;
			QITCount=5;
			break;
		case CREVersion::V2_2: // iwd2
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

void CREImporter::WriteChrHeader(DataStream *stream, const Actor *act)
{
	char Signature[8];
	ieDword tmpDword, CRESize;
	ieWord tmpWord;

	CRESize = GetStoredFileSize (act);
	switch (CREVersion) {
		case CREVersion::V9_0: // iwd/HoW
			memcpy(Signature, "CHR V1.0",8);
			tmpDword = 0x64; //headersize
			TotSCEFF = 1;
			break;
		case CREVersion::V1_0: // bg1
			memcpy(Signature, "CHR V1.0",8);
			tmpDword = 0x64; //headersize
			TotSCEFF = 0;
			break;
		case CREVersion::V1_1: // bg2 (fake)
			memcpy(Signature, "CHR V2.0",8);
			tmpDword = 0x64; //headersize
			TotSCEFF = 1;
			break;
		case CREVersion::V1_2: // pst
			memcpy(Signature, "CHR V1.2",8);
			tmpDword = 0x68; //headersize
			TotSCEFF = 0;
			break;
		case CREVersion::V2_2: // iwd2
			memcpy(Signature, "CHR V2.2",8);
			tmpDword = 0x21c; //headersize
			TotSCEFF = 1;
			break;
		case CREVersion::GemRB: // own format
			memcpy(Signature, "CHR V0.0",8);
			tmpDword = 0x1dc; //headersize (iwd2-9x8+8)
			TotSCEFF = 1;
			break;
		default:
			Log(ERROR, "CREImporter", "Unknown CHR version!");
			return;
	}
	stream->Write( Signature, 8);
	std::string tmpstr = MBStringFromString(act->GetShortName());
	stream->WriteVariable(ieVariable(tmpstr));
	stream->WriteDword(tmpDword); //cre offset (chr header size)
	stream->WriteDword(CRESize);  //cre size

	SetupSlotCounts();
	for (int i = 0; i < QWPCount; i++) {
		tmpWord = act->PCStats->QuickWeaponSlots[i];
		stream->WriteWord(tmpWord);
	}
	for (int i = 0; i < QWPCount; i++) {
		tmpWord = act->PCStats->QuickWeaponHeaders[i];
		stream->WriteWord(tmpWord);
	}
	for (int i = 0; i < QSPCount; i++) {
		stream->WriteResRef (act->PCStats->QuickSpells[i]);
	}
	//This is 9 for IWD2 and GemRB
	if (QSPCount==9) {
		//NOTE: the gemrb internal format stores
		//0xff or 0xfe in case of innates and bardsongs
		char filling[10] = {};
		memcpy(filling,act->PCStats->QuickSpellBookType,MAX_QSLOTS);
		for (int i = 0; i < MAX_QSLOTS; i++) {
			if ( (ieByte) filling[i]>=0xfe) filling[i]=0;
		}
		stream->Write( filling, 10);
	}
	for (int i = 0; i < QITCount; i++) {
		tmpWord = act->PCStats->QuickItemSlots[i];
		stream->WriteWord(tmpWord);
	}
	for (int i = 0; i < QITCount; i++) {
		tmpWord = act->PCStats->QuickItemHeaders[i];
		stream->WriteWord(tmpWord);
	}
	switch (CREVersion) {
	case CREVersion::V2_2:
		//gemrb format doesn't save these redundantly
		for (int i = 0; i < QSPCount; i++) {
			if (act->PCStats->QuickSpellBookType[i] == 0xff) {
				stream->WriteResRef (act->PCStats->QuickSpells[i]);
			} else {
				stream->WriteFilling(8);
			}
		}
		for (int i = 0; i < QSPCount; i++) {
			if (act->PCStats->QuickSpellBookType[i] == 0xfe) {
				stream->WriteResRef (act->PCStats->QuickSpells[i]);
			} else {
				stream->WriteFilling(8);
			}
		}
		//fallthrough
	case CREVersion::GemRB:
		for (int i = 0; i < QSPCount; i++) {
			tmpDword = act->PCStats->QSlots[i+3];
			stream->WriteDword(tmpDword);
		}
		for (int i = 0; i < 13; i++) {
			stream->WriteWord(tmpWord);
		}
		stream->WriteVariableLC(act->PCStats->SoundFolder);
		stream->WriteResRef(act->PCStats->SoundSet);
		for (const auto& setting : act->PCStats->ExtraSettings) {
			stream->WriteDword(setting);
		}
		//Reserved
		tmpDword = 0;
		for (int i = 0; i < 16; i++) {
			stream->WriteDword(tmpDword);
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
	str->ReadVariable(name);
	if (name[0]) {
		String* str = StringFromCString(name.c_str());
		assert(str);
		act->SetName(std::move(*str), 0); //setting longname
		delete str;
	}
	str->ReadDword(offset);
	str->ReadDword(size);
	SetupSlotCounts();
	for (int i = 0; i < QWPCount; i++) {
		str->ReadWord(tmpWord);
		act->PCStats->QuickWeaponSlots[i]=tmpWord;
	}
	for (int i = 0; i < QWPCount; i++) {
		str->ReadWord(tmpWord);
		act->PCStats->QuickWeaponHeaders[i]=tmpWord;
	}
	for (int i = 0; i < QSPCount; i++) {
		str->ReadResRef (act->PCStats->QuickSpells[i]);
	}
	if (QSPCount==9) {
		str->Read(act->PCStats->QuickSpellBookType, 9);
		str->Read (&tmpByte, 1);
	}
	for (int i = 0; i < QITCount; i++) {
		str->ReadWord(tmpWord);
		act->PCStats->QuickItemSlots[i]=tmpWord;
	}
	for (int i = 0; i < QITCount; i++) {
		str->ReadWord(tmpWord);
		act->PCStats->QuickItemHeaders[i]=tmpWord;
	}

	ResRef spell;
	//here comes the version specific read
	switch (CREVersion) {
	case CREVersion::V2_2:
		//gemrb format doesn't save these redundantly
		// quick innates and quick songs
		for (int i = 0; i < QSPCount; i++) {
			str->ReadResRef(spell);
			// there's a fixed number of buttons, so we can save some space by storing both types in the same field
			if (!spell.IsEmpty()) {
				act->PCStats->QuickSpellBookType[i] = 0xff;
				act->PCStats->QuickSpells[i] = spell;
			}
		}
		for (int i = 0; i < QSPCount; i++) {
			str->ReadResRef(spell);
			if (!spell.IsEmpty()) {
				act->PCStats->QuickSpellBookType[i] = 0xfe;
				act->PCStats->QuickSpells[i] = spell;
			}
		}
		//fallthrough
	case CREVersion::GemRB:
		for (int i = 0; i < QSPCount; i++) {
			str->ReadDword(tmpDword);
			act->PCStats->QSlots[i+3] = (ieByte) tmpDword;
		}
		str->Seek(26, GEM_CURRENT_POS);
		str->ReadVariable(act->PCStats->SoundFolder);
		str->ReadResRef(act->PCStats->SoundSet);
		for (auto& setting : act->PCStats->ExtraSettings) {
			str->ReadDword(setting);
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
	str->ReadDword(CREOffset);
	str->Seek(CREOffset, GEM_STREAM_START);
	str->Read( Signature, 8);
	return true;
}

CREMemorizedSpell* CREImporter::GetMemorizedSpell()
{
	CREMemorizedSpell* spl = new CREMemorizedSpell();

	str->ReadResRef( spl->SpellResRef );
	str->ReadDword(spl->Flags); // was split into flags word and two alignment bytes

	return spl;
}

CREKnownSpell* CREImporter::GetKnownSpell()
{
	CREKnownSpell* spl = new CREKnownSpell();

	str->ReadResRef(spl->SpellResRef);
	str->ReadWord(spl->Level);
	str->ReadWord(spl->Type);

	return spl;
}

void CREImporter::ReadScript(Actor *act, int ScriptLevel)
{
	ResRef aScript;
	str->ReadResRef(aScript);
	act->SetScript(aScript, ScriptLevel, act->InParty != 0);
}

CRESpellMemorization* CREImporter::GetSpellMemorization(Actor *act)
{
	ieWord Level, Type, Number, Number2;

	str->ReadWord(Level);
	str->ReadWord(Number);
	str->ReadWord(Number2);
	str->ReadWord(Type);
	str->ReadDword(MemorizedIndex);
	str->ReadDword(MemorizedCount);

	CRESpellMemorization* spl = act->spellbook.GetSpellMemorization(Type, Level);
	assert(spl && spl->SlotCount == 0 && spl->SlotCountWithBonus == 0); // unused
	spl->SlotCount = Number;
	spl->SlotCountWithBonus = Number; // Number2? Doesn't look like it's different in the data

	return spl;
}

void CREImporter::SetupColor(ieDword &stat) const
{
	static TableMgr::index_t RandColor = 1;
	if (RandColor == 0) return;

	TableMgr::index_t RandRows = 0;
	if (randcolors.empty()) {
		AutoTable rndcol = gamedata->LoadTable("randcolr", true);
		if (rndcol) {
			RandColor = rndcol->GetColumnCount();
			RandRows = rndcol->GetRowCount();
		}
		if (RandRows <= 1 || RandColor == 0) {
			RandColor = 0;
			return;
		}

		for (TableMgr::index_t cols = RandColor - 1; cols != 0; cols--) {
			int color = rndcol->QueryFieldSigned<int>(0, cols);
			randcolors[color] = std::vector<unsigned char>(RandRows - 1);
			for (TableMgr::index_t i = 1; i < RandRows; i++) {
				randcolors[color][i - 1] = rndcol->QueryFieldUnsigned<unsigned char>(i, cols);
			}
		}
	}

	// random indices start at 200
	if (stat < randcolors.begin()->first) return;
	RandRows = ieDword(randcolors.begin()->second.size());

	auto colors = randcolors.find(stat);
	if (colors == randcolors.end()) {
		Log(ERROR, "CREImporter", "Missing random color index in randcolr.2da: {}", stat);
		// fall back to browns
		stat = randcolors.begin()->second[RAND<TableMgr::index_t>(0, RandRows - 1)];
		return;
	}

	stat = colors->second[RAND<ieDword>(ieDword(0), RandRows - 1)];
}

void CREImporter::ReadDialog(Actor *act)
{
	ResRef Dialog;
	str->ReadResRef(Dialog);
	// avoiding a literal NONE to not error, since the file doesn't exist
	if (Dialog == "NONE") {
		Dialog.Reset();
	}
	act->SetDialog(Dialog);
}

Actor* CREImporter::GetActor(unsigned char is_in_party)
{
	Actor* act = new Actor();
	act->InParty = is_in_party;
	str->ReadStrRef(act->LongStrRef);
	//Beetle name in IWD needs the allow zero flag
	String poi = core->GetString( act->LongStrRef, STRING_FLAGS::ALLOW_ZERO );
	act->SetName(std::move(poi), 1); //setting longname
	str->ReadStrRef(act->ShortStrRef);
	if (act->ShortStrRef == (ieStrRef) -1) {
		act->ShortStrRef = act->LongStrRef;
	}
	poi = core->GetString(act->ShortStrRef);
	act->SetName(std::move(poi), 2); //setting shortname (for tooltips)
	act->BaseStats[IE_VISUALRANGE] = VOODOO_VISUAL_RANGE; // not stored anywhere
	act->BaseStats[IE_DIALOGRANGE] = VOODOO_DIALOG_RANGE;
	str->ReadDword(act->BaseStats[IE_MC_FLAGS]);
	str->ReadDword(act->BaseStats[IE_XPVALUE]);
	str->ReadDword(act->BaseStats[IE_XP]);
	str->ReadDword(act->BaseStats[IE_GOLD]);
	str->ReadDword(act->BaseStats[IE_STATE_ID]);
	ieWord tmp;
	ieWordSigned tmps;
	str->ReadScalar(tmps);
	act->BaseStats[IE_HITPOINTS]=(ieDwordSigned)tmps;
	if (tmps <= 0 && ((ieDwordSigned) act->BaseStats[IE_XPVALUE]) < 0) {
		act->BaseStats[IE_STATE_ID] |= STATE_DEAD;
	}
	str->ReadWord(tmp);
	act->BaseStats[IE_MAXHITPOINTS]=tmp;
	str->ReadDword(act->BaseStats[IE_ANIMATION_ID]);//animID is a dword
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
	if (CREVersion== CREVersion::V1_0 && TotSCEFF) {
		CREVersion = CREVersion::V1_1;
	}
	// saving in original version requires the original version
	// otherwise it is set to 0 at construction time
	if (core->config.SaveAsOriginal) {
		act->creVersion = CREVersion;
	}
	str->ReadResRef( act->SmallPortrait );
	if (act->SmallPortrait.IsEmpty()) {
		act->SmallPortrait = "NONE";
	}
	str->ReadResRef( act->LargePortrait );
	if (act->LargePortrait.IsEmpty()) {
		act->LargePortrait = "NONE";
	}

	unsigned int Inventory_Size;

	switch(CREVersion) {
		case CREVersion::GemRB:
			Inventory_Size = GetActorGemRB(act);
			break;
		case CREVersion::V1_2:
			Inventory_Size=46;
			GetActorPST(act);
			break;
		case CREVersion::V1_1: // bg2 (fake version)
		case CREVersion::V1_0: // bg1 too
			Inventory_Size=38;
			GetActorBG(act);
			break;
		case CREVersion::V2_2:
			Inventory_Size=50;
			GetActorIWD2(act);
			break;
		case CREVersion::V9_0:
			Inventory_Size=38;
			GetActorIWD1(act);
			break;
		default:
			Log(ERROR, "CREImporter", "Unknown creature signature: {}\n", CREVersion);
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
	ReadSpellbook(act);

	if (IsCharacter) {
		ReadChrHeader(act);
	}

	act->InitStatsOnLoad();

	return act;
}

void CREImporter::GetActorPST(Actor *act)
{
	ieByte tmpByte;
	ieWord tmpWord;

	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_REPUTATION]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_HIDEINSHADOWS]=tmpByte;
	str->ReadWord(tmpWord);
	//skipping a word
	str->ReadWord(tmpWord);
	act->AC.SetNatural((ieWordSigned) tmpWord);
	str->ReadWord(tmpWord);
	act->BaseStats[IE_ACCRUSHINGMOD]=(ieWordSigned) tmpWord;
	str->ReadWord(tmpWord);
	act->BaseStats[IE_ACMISSILEMOD]=(ieWordSigned) tmpWord;
	str->ReadWord(tmpWord);
	act->BaseStats[IE_ACPIERCINGMOD]=(ieWordSigned) tmpWord;
	str->ReadWord(tmpWord);
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
	// last byte is actually an undead level (according to IE dev info) - IE_UNDEADLEVEL
	for (int i = 0; i < 21; i++) {
		str->Read( &tmpByte, 1 );
		act->BaseStats[IE_PROFICIENCYBASTARDSWORD+i]=tmpByte;
	}
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_TRACKING]=tmpByte;
	//scriptname of tracked creature (according to IE dev info)
	str->Seek( 32, GEM_CURRENT_POS );
	for (auto& ref : act->StrRefs) {
		str->ReadStrRef(ref);
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
	str->ReadDword(act->BaseStats[IE_KIT]);
	ReadScript(act, SCR_OVERRIDE);
	ReadScript(act, SCR_CLASS);
	ReadScript(act, SCR_RACE);
	ReadScript(act, SCR_GENERAL);
	ReadScript(act, SCR_DEFAULT);

	str->Seek( 36, GEM_CURRENT_POS );
	//the overlays are not fully decoded yet
	//they are a kind of effect block (like our vvclist)
	// NOTE: rendered obsolete by our implementation of pst fx_overlay
	// no creature comes with this preset
	str->ReadDword(OverlayOffset);
	str->ReadDword(OverlayMemorySize);
	str->ReadDword(act->BaseStats[IE_XP_MAGE]); // Exp for secondary class
	str->ReadDword(act->BaseStats[IE_XP_THIEF]); // Exp for tertiary class
	for (int i = 0; i < 10; i++) {
		str->ReadWord(tmpWord);
		act->BaseStats[IE_INTERNAL_0+i]=tmpWord;
	}
	//good, law, lady, murder
	for (auto& counter : act->DeathCounters) {
		str->Read( &tmpByte, 1);
		counter = (ieByteSigned) tmpByte;
	}
	ieVariable KillVar; //use this as needed
	str->ReadVariable(KillVar);
	str->Read(&tmpByte, 1);
	act->BaseStats[IE_DIALOGRANGE] = tmpByte;
	str->Seek(2, GEM_CURRENT_POS); // circle size is only here (not in resdata.ini), but we have it in our own avatars.2da; plus an unknown byte

	str->Read( &tmpByte, 1 );

	str->ReadDword(act->AppearanceFlags);

	// just overwrite the bg1 color stat range, since it's not used in pst
	for (int i = 0; i < 7; i++) {
		str->ReadWord(tmpWord);
		act->BaseStats[IE_COLORS+i] = tmpWord;
	}
	act->BaseStats[IE_COLORCOUNT] = tmpByte;
	str->Read(act->pstColorBytes, 10); // color location in IESDP, sort of a palette index and flags
	str->Seek(21, GEM_CURRENT_POS);
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
	str->ReadVariable(scriptname);
	act->SetScriptName(scriptname);
	act->KillVar = MakeVariable(KillVar);
	act->IncKillVar.Reset();

	str->ReadDword(KnownSpellsOffset);
	str->ReadDword(KnownSpellsCount);
	str->ReadDword(SpellMemorizationOffset);
	str->ReadDword(SpellMemorizationCount);
	str->ReadDword(MemorizedSpellsOffset);
	str->ReadDword(MemorizedSpellsCount);

	str->ReadDword(ItemSlotsOffset);
	str->ReadDword(ItemsOffset);
	str->ReadDword(ItemsCount);
	str->ReadDword(EffectsOffset);
	str->ReadDword(EffectsCount); //also variables

	ReadDialog(act);
}

void CREImporter::ReadInventory(Actor *act, unsigned int Inventory_Size)
{
	act->inventory.SetSlotCount(Inventory_Size + 1);
	str->Seek(ItemSlotsOffset + CREOffset, GEM_STREAM_START);

	//first read the indices
	std::vector<ieWord> indices(Inventory_Size);
	for (auto& idx : indices) {
		str->ReadWord(idx);
	}

	ieWordSigned eqslot;
	ieWord eqheader;
	//this word contains the equipping info (which slot is selected)
	// 0,1,2,3 - weapon slots
	// 1000 - fist
	// -24,-23,-22,-21 - quiver
	// -1 is one of the plain inventory slots, but creatures like belhif.cre have it set as the equipped slot; see below
	//the equipping effects are delayed until the actor gets an area
	str->ReadScalar(eqslot);
	//the equipped slot's selected ability is stored here
	str->ReadWord(eqheader);
	act->inventory.SetEquipped(eqslot, eqheader);

	//read the item entries based on the previously read indices
	//an item entry may be read multiple times if the indices are repeating
	for (unsigned int i = 0; i < Inventory_Size;) {
		//the index was intentionally increased here, the fist slot isn't saved
		ieWord index = indices[i++];
		if (index != 0xffff) {
			if (index >= ItemsCount) {
				Log(ERROR, "CREImporter", "Invalid item index ({}) in creature!", index);
				continue;
			}
			//20 is the size of CREItem on disc (8+2+3x2+4)
			str->Seek(ItemsOffset + index * 20 + CREOffset, GEM_STREAM_START);
			//the core allocates this item data
			CREItem *item = core->ReadItem(str);
			int Slot = core->QuerySlot(i);
			if (item) {
				act->inventory.SetSlotItem(item, Slot);
			} else {
				Log(ERROR, "CREImporter", "Invalid item index ({}) in creature!", index);
			}
		}
	}

	// now that we have all items, check if we need to jump through hoops to get a proper equipped slot
	// move to fx_summon_creature2 if it turns out something else relies on having nothing equipped
	if (eqslot == -1) {
		act->inventory.SetEquipped(0, eqheader); // just reset Equipped, so EquipBestWeapon does its job
		act->inventory.EquipBestWeapon(EQUIP_MELEE);
	}

	indices.clear();
}

void CREImporter::ReadSpellbook(Actor *act)
{
	// Reading spellbook
	std::vector<CREKnownSpell*> knownSpells;
	std::vector<CREMemorizedSpell*> memorizedSpells;
	knownSpells.resize(KnownSpellsCount);
	memorizedSpells.resize(MemorizedSpellsCount);

	str->Seek(KnownSpellsOffset + CREOffset, GEM_STREAM_START);
	for (auto& knownSpell : knownSpells) {
		knownSpell = GetKnownSpell();
	}

	str->Seek(MemorizedSpellsOffset + CREOffset, GEM_STREAM_START);
	for (auto& memorizedSpell : memorizedSpells) {
		memorizedSpell = GetMemorizedSpell();
	}

	str->Seek(SpellMemorizationOffset + CREOffset, GEM_STREAM_START);
	for (unsigned int i = 0; i < SpellMemorizationCount; i++) {
		CRESpellMemorization* sm = GetSpellMemorization(act);

		unsigned int j = KnownSpellsCount;
		while (j--) {
			CREKnownSpell* spl = knownSpells[j];
			if (!spl) {
				continue;
			}
			if (spl->Type == sm->Type && spl->Level == sm->Level) {
				sm->known_spells.push_back(spl);
				knownSpells[j] = nullptr;
			}
		}
		for (unsigned int idx = 0; idx < MemorizedCount; idx++) {
			unsigned int k = MemorizedIndex + idx;
			assert(k < MemorizedSpellsCount);
			if (memorizedSpells[k]) {
				sm->memorized_spells.push_back(memorizedSpells[k]);
				memorizedSpells[k] = nullptr;
				continue;
			}
			Log(WARNING, "CREImporter", "Duplicate memorized spell({}) in creature!", k);
		}
	}

	for (auto knownSpell : knownSpells) {
		if (knownSpell) {
			Log(WARNING, "CREImporter", "Dangling known spell in creature: {}!",
				knownSpell->SpellResRef);
			delete knownSpell;
		}
	}
	knownSpells.clear();

	for (auto memorizedSpell : memorizedSpells) {
		if (memorizedSpell) {
			Log(WARNING, "CREImporter", "Dangling spell in creature: {}!",
				memorizedSpell->SpellResRef);
			delete memorizedSpell;
		}
	}
	memorizedSpells.clear();
}

void CREImporter::ReadEffects(Actor *act)
{
	str->Seek( EffectsOffset+CREOffset, GEM_STREAM_START );

	for (unsigned int i = 0; i < EffectsCount; i++) {
		act->fxqueue.AddEffect(GetEffect());
	}
}

Effect *CREImporter::GetEffect()
{
	PluginHolder<EffectMgr> eM = MakePluginHolder<EffectMgr>(IE_EFF_CLASS_ID);

	eM->Open(str, false);
	if (TotSCEFF) {
		return eM->GetEffectV20();
	} else {
		return eM->GetEffectV1();
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
	str->ReadWord(tmpWord);
	str->ReadWord(tmpWord);
	act->AC.SetNatural((ieWordSigned) tmpWord);
	str->ReadWord(tmpWord);
	act->BaseStats[IE_ACCRUSHINGMOD]=(ieWordSigned) tmpWord;
	str->ReadWord(tmpWord);
	act->BaseStats[IE_ACMISSILEMOD]=(ieWordSigned) tmpWord;
	str->ReadWord(tmpWord);
	act->BaseStats[IE_ACPIERCINGMOD]=(ieWordSigned) tmpWord;
	str->ReadWord(tmpWord);
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
	for (ieStrRef& StrRef : act->StrRefs) {
		str->ReadStrRef(StrRef);
	}
	return 0;
}

void CREImporter::GetActorBG(Actor *act)
{
	ieByte tmpByte;
	ieWord tmpWord;

	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_REPUTATION]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_HIDEINSHADOWS]=tmpByte;
	str->ReadWord(tmpWord);
	//skipping a word, labeled ArmorClass vs ArmorClassBase, so probably just the last computed value and thus useless
	str->ReadWord(tmpWord);
	act->AC.SetNatural((ieWordSigned) tmpWord);
	str->ReadWord(tmpWord);
	act->BaseStats[IE_ACCRUSHINGMOD]=(ieWordSigned) tmpWord;
	str->ReadWord(tmpWord);
	act->BaseStats[IE_ACMISSILEMOD]=(ieWordSigned) tmpWord;
	str->ReadWord(tmpWord);
	act->BaseStats[IE_ACPIERCINGMOD]=(ieWordSigned) tmpWord;
	str->ReadWord(tmpWord);
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
	for (int i = 0; i < 21; i++) {
		str->Read( &tmpByte, 1 );
		act->BaseStats[IE_PROFICIENCYBASTARDSWORD+i]=tmpByte;
	}

	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_TRACKING]=tmpByte;
	str->Seek( 32, GEM_CURRENT_POS );
	for (auto& ref : act->StrRefs) {
		str->ReadStrRef(ref);
	}
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LEVEL]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LEVEL2]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_LEVEL3]=tmpByte;
	//this is IE_SEX, but we use the gender field for this
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
	//skipping a byte, labeled MageSpecUpperWorld, while kit was MageSpecialization
	str->ReadDword(act->BaseStats[IE_KIT]);
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
	str->Seek(5, GEM_CURRENT_POS); // 5x SpecialCase in bg2/ee
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_ALIGNMENT]=tmpByte;
	str->Seek(4, GEM_CURRENT_POS); // dword labeled Instance
	ieVariable scriptname;
	str->ReadVariable(scriptname);
	act->SetScriptName(scriptname);
	act->KillVar.Reset();
	act->IncKillVar.Reset();

	str->ReadDword(KnownSpellsOffset);
	str->ReadDword(KnownSpellsCount);
	str->ReadDword(SpellMemorizationOffset);
	str->ReadDword(SpellMemorizationCount);
	str->ReadDword(MemorizedSpellsOffset);
	str->ReadDword(MemorizedSpellsCount);

	str->ReadDword(ItemSlotsOffset);
	str->ReadDword(ItemsOffset);
	str->ReadDword(ItemsCount);
	str->ReadDword(EffectsOffset);
	str->ReadDword(EffectsCount);

	ReadDialog(act);
}

void CREImporter::GetIWD2Spellpage(Actor *act, ieIWD2SpellType type, int level, int count)
{
	ieDword spellindex;
	ieDword totalcount;
	ieDword memocount;
	ieDword tmpDword;

	int i = count;
	CRESpellMemorization* sm = act->spellbook.GetSpellMemorization(type, level);
	assert(sm && sm->SlotCount == 0 && sm->SlotCountWithBonus == 0); // unused
	while(i--) {
		str->ReadDword(spellindex);
		str->ReadDword(totalcount);
		str->ReadDword(memocount);
		str->ReadDword(tmpDword);
		const ResRef& tmp = ResolveSpellIndex(spellindex, level, type, act->BaseStats[IE_KIT]);
		if (tmp.IsEmpty()) {
			error("CREImporter", "Unresolved spell index: {} level:{}, type: {}",
				spellindex, level+1, type);
		}

		CREKnownSpell *known = new CREKnownSpell;
		known->Level = level;
		known->Type = type;
		known->SpellResRef = tmp;
		sm->known_spells.push_back(known);
		while (memocount--) {
			if (totalcount) {
				totalcount--;
			} else {
				Log(ERROR, "CREImporter", "More spells still known than memorised.");
				break;
			}
			CREMemorizedSpell *memory = new CREMemorizedSpell;
			memory->Flags = 1;
			memory->SpellResRef = tmp;
			sm->memorized_spells.push_back(memory);
		}
		while(totalcount--) {
			CREMemorizedSpell *memory = new CREMemorizedSpell;
			memory->Flags = 0;
			memory->SpellResRef = tmp;
			sm->memorized_spells.push_back(memory);
		}
	}
	// hacks for domain spells, since their count is not stored and also always 1
	// NOTE: luckily this does not cause save game incompatibility
	str->ReadDword(tmpDword);
	if (type == IE_IWD2_SPELL_DOMAIN && count > 0) {
		sm->SlotCount = 1;
	} else {
		sm->SlotCount = (ieWord) tmpDword;
	}
	str->ReadDword(tmpDword);
	if (type == IE_IWD2_SPELL_DOMAIN && count > 0) {
		sm->SlotCountWithBonus = 1;
	} else {
		sm->SlotCountWithBonus = (ieWord) tmpDword;
	}
}

void CREImporter::GetActorIWD2(Actor *act)
{
	ieByte tmpByte;
	ieWord tmpWord;

	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_REPUTATION]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_HIDEINSHADOWS]=tmpByte;
	str->ReadWord(tmpWord);
	act->AC.SetNatural((ieWordSigned) tmpWord);
	str->ReadWord(tmpWord);
	act->BaseStats[IE_ACCRUSHINGMOD]=(ieWordSigned) tmpWord;
	str->ReadWord(tmpWord);
	act->BaseStats[IE_ACMISSILEMOD]=(ieWordSigned) tmpWord;
	str->ReadWord(tmpWord);
	act->BaseStats[IE_ACPIERCINGMOD]=(ieWordSigned) tmpWord;
	str->ReadWord(tmpWord);
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
	for (int i = 0; i < 64; i++) {
		str->ReadStrRef(act->StrRefs[i]);
	}
	ReadScript( act, SCR_SPECIFICS);
	ReadScript( act, SCR_AREA);
	str->Seek( 4, GEM_CURRENT_POS );
	str->ReadDword(act->BaseStats[IE_FEATS1]);
	str->ReadDword(act->BaseStats[IE_FEATS2]);
	str->ReadDword(act->BaseStats[IE_FEATS3]);
	str->Seek( 12, GEM_CURRENT_POS );
	//proficiencies
	for (int i = 0; i < 26; i++) {
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
	for (int i = 0; i < 7; i++) {
		str->Read( &tmpByte, 1 );
		act->BaseStats[IE_HATEDRACE2+i]=tmpByte;
	}
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_SUBRACE]=tmpByte;
	str->ReadWord(tmpWord);
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
	str->ReadDword(act->BaseStats[IE_KIT]);
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
	for (int i = 0; i < 5; i++) {
		str->ReadWord(tmpWord);
		act->BaseStats[IE_INTERNAL_0+i]=tmpWord;
	}
	ieVariable KillVar;
	str->ReadVariable(KillVar);
	act->KillVar = MakeVariable(KillVar);
	str->ReadVariable(KillVar);
	act->IncKillVar = MakeVariable(KillVar);
	str->Seek( 2, GEM_CURRENT_POS);
	str->ReadWord(tmpWord);
	act->BaseStats[IE_SAVEDXPOS] = tmpWord;
	str->ReadWord(tmpWord);
	act->BaseStats[IE_SAVEDYPOS] = tmpWord;
	str->ReadWord(tmpWord);
	act->BaseStats[IE_SAVEDFACE] = tmpWord;

	str->Seek( 15, GEM_CURRENT_POS );
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_TRANSLUCENT]=tmpByte;
	str->Read( &tmpByte, 1); //fade speed
	str->Read( &tmpByte, 1); //spec. flags
	act->BaseStats[IE_SPECFLAGS] = tmpByte;
	str->Read( &tmpByte, 1); //invisible
	str->ReadWord(tmpWord); //unknown
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
	str->Seek( 5, GEM_CURRENT_POS ); // object.ids references that we don't save
	str->Read( &tmpByte, 1);
	act->BaseStats[IE_ALIGNMENT]=tmpByte;
	str->Seek( 4, GEM_CURRENT_POS );
	ieVariable scriptname;
	str->ReadVariable(scriptname);
	act->SetScriptName(scriptname);

	KnownSpellsOffset = 0;
	KnownSpellsCount = 0;
	SpellMemorizationOffset = 0;
	SpellMemorizationCount = 0;
	MemorizedSpellsOffset = 0;
	MemorizedSpellsCount = 0;
	// skipping class (probably redundant), class mask (calculated)
	str->Seek(6, GEM_CURRENT_POS);
	ieDword ClassSpellOffsets[8*9];

	//spellbook spells
	for (int i = 0; i < 7 * 9; i++) {
		str->ReadDword(ClassSpellOffsets[i]);
	}
	ieDword ClassSpellCounts[8*9];
	for (int i = 0; i < 7 * 9; i++) {
		str->ReadDword(ClassSpellCounts[i]);
	}

	//domain spells
	for (int i = 7*9; i < 8 * 9; i++) {
		str->ReadDword(ClassSpellOffsets[i]);
	}
	for (int i = 7*9; i < 8 * 9; i++) {
		str->ReadDword(ClassSpellCounts[i]);
	}

	ieDword InnateOffset, InnateCount;
	ieDword SongOffset, SongCount;
	ieDword ShapeOffset, ShapeCount;
	str->ReadDword(InnateOffset);
	str->ReadDword(InnateCount);
	str->ReadDword(SongOffset);
	str->ReadDword(SongCount);
	str->ReadDword(ShapeOffset);
	str->ReadDword(ShapeCount);

	str->ReadDword(ItemSlotsOffset);
	str->ReadDword(ItemsOffset);
	str->ReadDword(ItemsCount);
	str->ReadDword(EffectsOffset);
	str->ReadDword(EffectsCount);

	ReadDialog(act);

	for (int i = 0; i < 8; i++) {
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
	ieByte tmpByte;
	ieWord tmpWord;

	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_REPUTATION]=tmpByte;
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_HIDEINSHADOWS]=tmpByte;
	str->ReadWord(tmpWord);
	//skipping a word
	str->ReadWord(tmpWord);
	act->AC.SetNatural((ieWordSigned) tmpWord);
	str->ReadWord(tmpWord);
	act->BaseStats[IE_ACCRUSHINGMOD]=(ieWordSigned) tmpWord;
	str->ReadWord(tmpWord);
	act->BaseStats[IE_ACMISSILEMOD]=(ieWordSigned) tmpWord;
	str->ReadWord(tmpWord);
	act->BaseStats[IE_ACPIERCINGMOD]=(ieWordSigned) tmpWord;
	str->ReadWord(tmpWord);
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
	for (int i = 0; i < 21; i++) {
		str->Read( &tmpByte, 1 );
		act->BaseStats[IE_PROFICIENCYBASTARDSWORD+i]=tmpByte;
	}
	str->Read( &tmpByte, 1 );
	act->BaseStats[IE_TRACKING]=tmpByte;
	str->Seek( 32, GEM_CURRENT_POS );
	for (auto& ref : act->StrRefs) {
		str->ReadStrRef(ref);
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
	str->ReadDword(act->BaseStats[IE_KIT]);
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
	for (int i = 0; i < 5; i++) {
		str->ReadWord(tmpWord);
		act->BaseStats[IE_INTERNAL_0+i]=tmpWord;
	}
	ieVariable KillVar;
	str->ReadVariable(KillVar); // use these as needed
	act->KillVar = MakeVariable(KillVar);
	str->ReadVariable(KillVar);
	act->IncKillVar = MakeVariable(KillVar);
	str->Seek( 2, GEM_CURRENT_POS);
	str->ReadWord(tmpWord);
	act->BaseStats[IE_SAVEDXPOS] = tmpWord;
	str->ReadWord(tmpWord);
	act->BaseStats[IE_SAVEDYPOS] = tmpWord;
	str->ReadWord(tmpWord);
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
	str->ReadVariable(scriptname);
	act->SetScriptName(scriptname);

	str->ReadDword(KnownSpellsOffset);
	str->ReadDword(KnownSpellsCount);
	str->ReadDword(SpellMemorizationOffset);
	str->ReadDword(SpellMemorizationCount);
	str->ReadDword(MemorizedSpellsOffset);
	str->ReadDword(MemorizedSpellsCount);

	str->ReadDword(ItemSlotsOffset);
	str->ReadDword(ItemsOffset);
	str->ReadDword(ItemsCount);
	str->ReadDword(EffectsOffset);
	str->ReadDword(EffectsCount);

	ReadDialog(act);
}

int CREImporter::GetStoredFileSize(const Actor *actor)
{
	int headersize;
	unsigned int Inventory_Size;

	CREVersion = actor->creVersion;
	switch (CREVersion) {
		case CREVersion::GemRB:
			headersize = 0x2d4;
			//minus fist
			Inventory_Size=actor->inventory.GetSlotCount()-1;
			TotSCEFF = 1;
			break;
		case CREVersion::V1_1: // totsc/bg2/tob (still V1.0, but large effects)
		case CREVersion::V1_0: // bg1
			headersize = 0x2d4;
			Inventory_Size=38;
			//we should know it is bg1
			if (actor->creVersion == CREVersion::V1_1) {
				TotSCEFF = 1;
			} else {
				TotSCEFF = 0;
			}
			break;
		case CREVersion::V1_2: // pst
			headersize = 0x378;
			Inventory_Size=46;
			TotSCEFF = 0;
			break;
		case CREVersion::V2_2:// iwd2
			headersize = 0x62e; // with offsets
			Inventory_Size=50;
			TotSCEFF = 1;
			break;
		case CREVersion::V9_0:// iwd
			headersize = 0x33c;
			Inventory_Size=38;
			TotSCEFF = 1;
			break;
		default:
			return -1;
	}
	KnownSpellsOffset = headersize;

	if (actor->creVersion == CREVersion::V2_2) { // iwd2
		for (int type = IE_IWD2_SPELL_BARD; type < IE_IWD2_SPELL_DOMAIN; type++) {
			for (int level = 0; level < 9; level++) {
				headersize += GetIWD2SpellpageSize(actor, (ieIWD2SpellType) type, level) * 16 + 8;
			}
		}
		for (int level = 0; level < 9; level++) {
			headersize += GetIWD2SpellpageSize(actor, IE_IWD2_SPELL_DOMAIN, level)*16+8;
		}
		for (int type = IE_IWD2_SPELL_INNATE; type < NUM_IWD2_SPELLTYPES; type++) {
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
	ItemSlotsOffset = headersize;

	//adding itemslot table size and equipped slot fields
	headersize += Inventory_Size * sizeof(ieWord) + sizeof(ieWord) * 2;
	ItemsOffset = headersize;

	//counting items (calculating item storage)
	ItemsCount = 0;
	for (unsigned int i = 0; i < Inventory_Size; i++) {
		unsigned int j = core->QuerySlot(i+1);
		const CREItem *it = actor->inventory.GetSlotItem(j);
		if (it) {
			ItemsCount++;
		}
	}
	headersize += ItemsCount * 20;

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

	return headersize;
}

int CREImporter::PutInventory(DataStream *stream, const Actor *actor, unsigned int size) const
{
	ieDword tmpDword;
	ieWord tmpWord;
	ieWord ItemCount = 0;
	std::vector<ieWord> indices(size, -1);

	for (unsigned int i = 0; i < size; i++) {
		//ignore first element, getinventorysize makes space for fist
		unsigned int j = core->QuerySlot(i+1);
		const CREItem *it = actor->inventory.GetSlotItem(j);
		if (it) {
			indices[i] = ItemCount++;
		}
		stream->WriteWord(indices[i]);
	}

	tmpWord = (ieWord) actor->inventory.GetEquipped();
	stream->WriteWord(tmpWord);
	tmpWord = (ieWord) actor->inventory.GetEquippedHeader();
	stream->WriteWord(tmpWord);

	for (unsigned int i = 0; i < size; i++) {
		//ignore first element, getinventorysize makes space for fist
		unsigned int j = core->QuerySlot(i+1);
		const CREItem *it = actor->inventory.GetSlotItem(j);
		if (!it) {
			continue;
		}
		stream->WriteResRef( it->ItemResRef);
		stream->WriteWord(it->Expired);
		stream->WriteWord(it->Usages[0]);
		stream->WriteWord(it->Usages[1]);
		stream->WriteWord(it->Usages[2]);
		tmpDword = it->Flags;
		//IWD uses this bit differently
		if (core->HasFeature(GF_MAGICBIT)) {
			if (it->Flags&IE_INV_ITEM_MAGICAL) {
				tmpDword|=IE_INV_ITEM_UNDROPPABLE;
			} else {
				tmpDword&=~IE_INV_ITEM_UNDROPPABLE;
			}
		}
		stream->WriteDword(tmpDword);
	}
	return 0;
}

int CREImporter::PutHeader(DataStream *stream, const Actor *actor) const
{
	char Signature[8];
	ieByte tmpByte;
	ieWord tmpWord;
	ieDword tmpDword;

	memcpy( Signature, "CRE V0.0", 8);
	Signature[5]+=CREVersion/10;
	if (actor->creVersion != CREVersion::V1_1) {
		Signature[7]+=CREVersion%10;
	}
	stream->Write( Signature, 8);
	stream->WriteStrRef(actor->LongStrRef);
	stream->WriteStrRef(actor->ShortStrRef);
	stream->WriteDword(actor->BaseStats[IE_MC_FLAGS]);
	stream->WriteDword(actor->BaseStats[IE_XPVALUE]);
	stream->WriteDword(actor->BaseStats[IE_XP]);
	stream->WriteDword(actor->BaseStats[IE_GOLD]);
	stream->WriteDword(actor->BaseStats[IE_STATE_ID]);
	tmpWord = actor->BaseStats[IE_HITPOINTS];
	//decrease the hp back to the one without constitution bonus
	// (but only player classes can have it)
	tmpWord = (ieWord) (tmpWord - actor->GetHpAdjustment(actor->GetXPLevel(false), false));
	stream->WriteWord(tmpWord);
	tmpWord = actor->BaseStats[IE_MAXHITPOINTS];
	stream->WriteWord(tmpWord);
	stream->WriteDword(actor->BaseStats[IE_ANIMATION_ID]);
	for (int i = 0; i < 7; i++) {
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
	stream->WriteWord(tmpWord);
	//iwd2 doesn't store this a second time,
	//probably gemrb format shouldn't either?
	if (actor->creVersion != CREVersion::V2_2) {
		tmpWord = actor->AC.GetNatural();
		stream->WriteWord(tmpWord);
	}
	tmpWord = actor->BaseStats[IE_ACCRUSHINGMOD];
	stream->WriteWord(tmpWord);
	tmpWord = actor->BaseStats[IE_ACMISSILEMOD];
	stream->WriteWord(tmpWord);
	tmpWord = actor->BaseStats[IE_ACPIERCINGMOD];
	stream->WriteWord(tmpWord);
	tmpWord = actor->BaseStats[IE_ACSLASHINGMOD];
	stream->WriteWord(tmpWord);
	tmpByte = actor->ToHit.GetBase();
	stream->Write( &tmpByte, 1);
	tmpByte = actor->BaseStats[IE_NUMBEROFATTACKS];
	if (actor->creVersion == CREVersion::V2_2) {
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_SAVEFORTITUDE];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_SAVEREFLEX];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_SAVEWILL];
		stream->Write( &tmpByte, 1);
	} else {
		if (actor->creVersion != CREVersion::GemRB) {
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
	if (actor->creVersion == CREVersion::V2_2) {
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

	if (actor->creVersion == CREVersion::V2_2) {
		//this is rather fuzzy
		//turnundead level, + 33 bytes of zero
		tmpByte = actor->BaseStats[IE_TURNUNDEADLEVEL];
		stream->Write(&tmpByte, 1);
		stream->WriteFilling(33);
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
		stream->WriteFilling(22);
		//string references
		for (int i = 0; i < 64; i++) {
			stream->WriteStrRef(actor->StrRefs[i]);
		}
		stream->WriteResRef( actor->GetScript(SCR_AREA) );
		stream->WriteResRef( actor->GetScript(SCR_RESERVED) );
		//unknowns before feats
		stream->WriteFilling(4);
		//feats
		stream->WriteDword(actor->BaseStats[IE_FEATS1]);
		stream->WriteDword(actor->BaseStats[IE_FEATS2]);
		stream->WriteDword(actor->BaseStats[IE_FEATS3]);
		stream->WriteFilling(12);
		//proficiencies
		for (int i = 0; i < 26; i++) {
			tmpByte = actor->BaseStats[IE_PROFICIENCYBASTARDSWORD+i];
			stream->Write( &tmpByte, 1);
		}
		stream->WriteFilling(38);
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
		stream->WriteFilling(50);
		tmpByte = actor->BaseStats[IE_CR];
		stream->Write( &tmpByte, 1);
		tmpByte = actor->BaseStats[IE_HATEDRACE];
		stream->Write( &tmpByte, 1);
		for (int i = 0; i < 7; i++) {
			tmpByte = actor->BaseStats[IE_HATEDRACE2+i];
			stream->Write( &tmpByte, 1);
		}
		tmpByte = actor->BaseStats[IE_SUBRACE];
		stream->Write( &tmpByte, 1);
		stream->WriteFilling(1); //unknown
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
		stream->WriteFilling(1);
		// no kit word order magic for iwd2
		stream->WriteDword(actor->BaseStats[IE_KIT]);
		stream->WriteResRef( actor->GetScript(SCR_OVERRIDE) );
		stream->WriteResRef( actor->GetScript(SCR_CLASS) );
		stream->WriteResRef( actor->GetScript(SCR_RACE) );
		stream->WriteResRef( actor->GetScript(SCR_GENERAL) );
		stream->WriteResRef( actor->GetScript(SCR_DEFAULT) );
	} else {
		for (int i = 0; i < 21; i++) {
			tmpByte = actor->BaseStats[IE_PROFICIENCYBASTARDSWORD+i];
			stream->Write( &tmpByte, 1);
		}
		tmpByte = actor->BaseStats[IE_TRACKING];
		stream->Write( &tmpByte, 1);
		stream->WriteFilling(32);
		for (const auto& ref : actor->StrRefs) {
			stream->WriteStrRef(ref);
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
		stream->WriteFilling(1);
		tmpDword = ((actor->BaseStats[IE_KIT] & 0xffff) << 16) +
			((actor->BaseStats[IE_KIT] & 0xffff0000) >> 16);
		stream->WriteDword(tmpDword);
		stream->WriteResRef( actor->GetScript(SCR_OVERRIDE) );
		stream->WriteResRef( actor->GetScript(SCR_CLASS) );
		stream->WriteResRef( actor->GetScript(SCR_RACE) );
		stream->WriteResRef( actor->GetScript(SCR_GENERAL) );
		stream->WriteResRef( actor->GetScript(SCR_DEFAULT) );
	}
	//now follows the fuzzy part in separate putactor... functions
	return 0;
}

int CREImporter::PutActorGemRB(DataStream *stream, const Actor *actor, ieDword InvSize) const
{
	ieByte tmpByte;
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
	stream->WriteFilling(5); //unknown bytes
	tmpByte = actor->BaseStats[IE_ALIGNMENT];
	stream->Write( &tmpByte, 1);
	stream->WriteDword(InvSize); //saving the inventory size to this unused part
	stream->WriteVariable(actor->GetScriptName());
	return 0;
}

int CREImporter::PutActorBG(DataStream *stream, const Actor *actor) const
{
	ieByte tmpByte;
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
	stream->WriteFilling(5); //unknown bytes
	tmpByte = actor->BaseStats[IE_ALIGNMENT];
	stream->Write( &tmpByte, 1);
	stream->WriteFilling(4); //this is called ID in iwd2, and contains 2 words
	stream->WriteVariable(actor->GetScriptName());
	return 0;
}

int CREImporter::PutActorPST(DataStream *stream, const Actor *actor) const
{
	ieByte tmpByte;
	ieWord tmpWord;

	stream->WriteFilling(44); //11*4 totally unknown
	stream->WriteDword(actor->BaseStats[IE_XP_MAGE]);
	stream->WriteDword(actor->BaseStats[IE_XP_THIEF]);
	for (int i = 0; i < 10; i++) {
		tmpWord = actor->BaseStats[IE_INTERNAL_0];
		stream->WriteWord(tmpWord);
	}
	for (const auto& counter : actor->DeathCounters) {
		tmpByte = (ieByte) counter;
		stream->Write( &tmpByte, 1);
	}
	stream->WriteVariable(actor->KillVar);
	stream->WriteFilling(3); //unknown
	tmpByte=actor->BaseStats[IE_COLORCOUNT];
	stream->Write( &tmpByte, 1);
	stream->WriteDword(actor->AppearanceFlags);

	for (int i = 0; i < 7; i++) {
		tmpWord = actor->BaseStats[IE_COLORS+i];
		stream->WriteWord(tmpWord);
	}
	stream->Write(actor->pstColorBytes, 10);
	stream->WriteFilling(21);
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
	stream->WriteFilling(5); //unknown bytes
	tmpByte = actor->BaseStats[IE_ALIGNMENT];
	stream->Write( &tmpByte, 1);
	stream->WriteFilling(4); //this is called ID in iwd2, and contains 2 words
	stream->WriteVariable(actor->GetScriptName());
	return 0;
}

int CREImporter::PutActorIWD1(DataStream *stream, const Actor *actor) const
{
	ieByte tmpByte;
	ieWord tmpWord;

	tmpByte=(ieByte) actor->BaseStats[IE_AVATARREMOVAL];
	stream->Write( &tmpByte, 1);
	stream->Write( &actor->SetDeathVar, 1);
	stream->Write( &actor->IncKillCount, 1);
	stream->Write( &actor->UnknownField, 1); //unknown
	for (int i = 0; i < 5; i++) {
		tmpWord = actor->BaseStats[IE_INTERNAL_0+i];
		stream->WriteWord(tmpWord);
	}
	stream->WriteVariable(actor->KillVar); // some variable names in iwd
	stream->WriteVariable(actor->IncKillVar); // some variable names in iwd
	stream->WriteFilling(2);
	tmpWord = actor->BaseStats[IE_SAVEDXPOS];
	stream->WriteWord(tmpWord);
	tmpWord = actor->BaseStats[IE_SAVEDYPOS];
	stream->WriteWord(tmpWord);
	tmpWord = actor->BaseStats[IE_SAVEDFACE];
	stream->WriteWord(tmpWord);
	stream->WriteFilling(18);
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
	stream->WriteFilling(5); //unknown bytes
	tmpByte = actor->BaseStats[IE_ALIGNMENT];
	stream->Write( &tmpByte, 1);
	stream->WriteFilling(4); //this is called ID in iwd2, and contains 2 words
	stream->WriteVariable(actor->GetScriptName());
	return 0;
}

int CREImporter::PutActorIWD2(DataStream *stream, const Actor *actor) const
{
	ieByte tmpByte;
	ieWord tmpWord;
	ieDword tmpDword;

	tmpByte=(ieByte) actor->BaseStats[IE_AVATARREMOVAL];
	stream->Write( &tmpByte, 1);
	stream->Write( &actor->SetDeathVar, 1);
	stream->Write( &actor->IncKillCount, 1);
	stream->Write( &actor->UnknownField, 1); //unknown
	for (int i = 0; i < 5; i++) {
		tmpWord = actor->BaseStats[IE_INTERNAL_0+i];
		stream->WriteWord(tmpWord);
	}
	stream->WriteVariable(actor->KillVar); // some variable names in iwd
	stream->WriteVariable(actor->IncKillVar); // some variable names in iwd
	stream->WriteFilling(2);
	tmpWord = actor->BaseStats[IE_SAVEDXPOS];
	stream->WriteWord(tmpWord);
	tmpWord = actor->BaseStats[IE_SAVEDYPOS];
	stream->WriteWord(tmpWord);
	tmpWord = actor->BaseStats[IE_SAVEDFACE];
	stream->WriteWord(tmpWord);
	stream->WriteFilling(15);
	tmpByte = actor->BaseStats[IE_TRANSLUCENT];
	stream->Write(&tmpByte, 1);
	stream->WriteFilling(1); //fade speed
	tmpByte = actor->BaseStats[IE_SPECFLAGS];
	stream->Write(&tmpByte, 1);
	stream->WriteFilling(3); //invisible, 2 unknowns
	tmpByte = actor->BaseStats[IE_UNUSED_SKILLPTS];
	stream->Write( &tmpByte, 1);
	stream->WriteFilling(124);
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
	stream->WriteFilling(5); //unknown bytes
	tmpByte = actor->BaseStats[IE_ALIGNMENT];
	stream->Write( &tmpByte, 1);
	stream->WriteFilling(4); //this is called ID in iwd2, and contains 2 words
	stream->WriteVariable(actor->GetScriptName());
	tmpByte = actor->BaseStats[IE_CLASS];
	stream->Write(&tmpByte, 1);
	stream->WriteFilling(1);
	tmpDword = actor->GetClassMask();
	stream->WriteDword(tmpDword);
	return 0;
}

int CREImporter::PutKnownSpells(DataStream *stream, const Actor *actor) const
{
	int type=actor->spellbook.GetTypes();
	for (int i=0;i<type;i++) {
		unsigned int level = actor->spellbook.GetSpellLevelCount(i);
		for (unsigned int j=0;j<level;j++) {
			unsigned int count = actor->spellbook.GetKnownSpellsCount(i, j);
			for (int k=count-1;k>=0;k--) {
				const CREKnownSpell *ck = actor->spellbook.GetKnownSpell(i, j, k);
				assert(ck);
				stream->WriteResRef(ck->SpellResRef);
				stream->WriteWord(ck->Level);
				stream->WriteWord(ck->Type);
			}
		}
	}
	return 0;
}

int CREImporter::PutSpellPages(DataStream *stream, const Actor *actor) const
{
	ieWord tmpWord;
	ieDword tmpDword;
	ieDword SpellIndex = 0;

	int type=actor->spellbook.GetTypes();
	for (int i=0;i<type;i++) {
		unsigned int level = actor->spellbook.GetSpellLevelCount(i);
		for (unsigned int j=0;j<level;j++) {
			tmpWord = j; //+1
			stream->WriteWord(tmpWord);
			tmpWord = actor->spellbook.GetMemorizableSpellsCount(i,j,false);
			stream->WriteWord(tmpWord);
			tmpWord = actor->spellbook.GetMemorizableSpellsCount(i,j,true);
			stream->WriteWord(tmpWord);
			tmpWord = i;
			stream->WriteWord(tmpWord);
			stream->WriteDword(SpellIndex);
			tmpDword = actor->spellbook.GetMemorizedSpellsCount(i,j, false);
			stream->WriteDword(tmpDword);
			SpellIndex += tmpDword;
		}
	}
	return 0;
}

int CREImporter::PutMemorizedSpells(DataStream *stream, const Actor *actor) const
{
	int type=actor->spellbook.GetTypes();
	for (int i=0;i<type;i++) {
		unsigned int level = actor->spellbook.GetSpellLevelCount(i);
		for (unsigned int j=0;j<level;j++) {
			unsigned int count = actor->spellbook.GetMemorizedSpellsCount(i,j, false);
			for (unsigned int k=0;k<count;k++) {
				const CREMemorizedSpell *cm = actor->spellbook.GetMemorizedSpell(i, j, k);
				assert(cm);
				stream->WriteResRef( cm->SpellResRef);
				stream->WriteDword(cm->Flags);
			}
		}
	}
	return 0;
}

int CREImporter::PutEffects( DataStream *stream, const Actor *actor) const
{
	PluginHolder<EffectMgr> eM = MakePluginHolder<EffectMgr>(IE_EFF_CLASS_ID);
	assert(eM != nullptr);

	auto f = actor->fxqueue.GetFirstEffect();
	for(unsigned int i=0;i<EffectsCount;i++) {
		const Effect *fx = actor->fxqueue.GetNextSavedEffect(f);

		assert(fx!=NULL);

		if (TotSCEFF) {
			eM->PutEffectV2(stream, fx);
		} else {
			ieWord tmpWord;
			ieByte tmpByte;

			tmpWord = (ieWord) fx->Opcode;
			stream->WriteWord(tmpWord);
			tmpByte = (ieByte) fx->Target;
			stream->Write(&tmpByte, 1);
			tmpByte = (ieByte) fx->Power;
			stream->Write(&tmpByte, 1);
			stream->WriteDword(fx->Parameter1);
			stream->WriteDword(fx->Parameter2);
			tmpByte = (ieByte) fx->TimingMode;
			stream->Write(&tmpByte, 1);
			tmpByte = (ieByte) fx->Resistance;
			stream->Write(&tmpByte, 1);
			stream->WriteDword(fx->Duration);
			tmpByte = (ieByte) fx->ProbabilityRangeMax;
			stream->Write(&tmpByte, 1);
			tmpByte = (ieByte) fx->ProbabilityRangeMin;
			stream->Write(&tmpByte, 1);
			stream->WriteResRef(fx->Resource);
			stream->WriteDword(fx->DiceThrown);
			stream->WriteDword(fx->DiceSides);
			stream->WriteDword(fx->SavingThrowType);
			stream->WriteDword(fx->SavingThrowBonus);
			//isvariable
			stream->WriteFilling(4);
		}
	}
	return 0;
}

//add as effect!
int CREImporter::PutVariables(DataStream *stream, const Actor *actor) const
{
	Variables::iterator pos=NULL;
	Variables::key_t name;
	ieDword tmpDword, value;

	for (unsigned int i=0;i<VariablesCount;i++) {
		pos = actor->locals->GetNextAssoc(pos, name, value);
		stream->WriteFilling(8);
		tmpDword = FAKE_VARIABLE_OPCODE;
		stream->WriteDword(tmpDword);
		stream->WriteFilling(8); //type, power
		stream->WriteDword(value); //param #1
		stream->WriteFilling(4); //param #2
		//HoW has an assertion to ensure timing is nonzero (even for variables)
		value = 1;
		stream->WriteDword(value); //timing
		//duration, chance, resource, dices, saves
		stream->WriteFilling(32);
		tmpDword = FAKE_VARIABLE_MARKER;
		stream->WriteDword(tmpDword); //variable marker
		stream->WriteFilling(92); //23 * 4
		stream->WriteVariable(ieVariable(name));
		stream->WriteFilling(72); //32 + 72
	}
	return 0;
}

//Don't forget to add 8 for the totals/bonus fields
ieDword CREImporter::GetIWD2SpellpageSize(const Actor *actor, ieIWD2SpellType type, int level) const
{
	return actor->spellbook.GetKnownSpellsCount(type, level);
}

int CREImporter::PutIWD2Spellpage(DataStream *stream, const Actor *actor, ieIWD2SpellType type, int level) const
{
	ieDword max, known;

	int knownCount = actor->spellbook.GetKnownSpellsCount(type, level);
	for (int i = 0; i < knownCount; i++) {
		const CREKnownSpell* knownSpell = actor->spellbook.GetKnownSpell(type, level, i);
		ieDword ID = ResolveSpellName(knownSpell->SpellResRef, level, type);
		stream->WriteDword(ID);
		max = actor->spellbook.CountSpells(knownSpell->SpellResRef, type, 1);
		known = actor->spellbook.CountSpells(knownSpell->SpellResRef, type, 0);
		stream->WriteDword(max);
		stream->WriteDword(known);
		//unknown field (always 0)
		known = 0;
		stream->WriteDword(known);
	}

	max = actor->spellbook.GetMemorizableSpellsCount(type, level, false);
	known = actor->spellbook.GetMemorizableSpellsCount(type, level, true);
	stream->WriteDword(max);
	stream->WriteDword(known);
	return 0;
}

/* this function expects GetStoredFileSize to be called before */
int CREImporter::PutActor(DataStream *stream, const Actor *actor, bool chr)
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
		case CREVersion::GemRB:
			//don't add fist
			Inventory_Size=(ieDword) actor->inventory.GetSlotCount()-1;
			ret = PutActorGemRB(stream, actor, Inventory_Size);
			break;
		case CREVersion::V1_2:
			Inventory_Size=46;
			ret = PutActorPST(stream, actor);
			break;
		case CREVersion::V1_1:
		case CREVersion::V1_0: // bg1/bg2
			Inventory_Size=38;
			ret = PutActorBG(stream, actor);
			break;
		case CREVersion::V2_2:
			Inventory_Size=50;
			ret = PutActorIWD2(stream, actor);
			break;
		case CREVersion::V9_0:
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
	if (actor->creVersion == CREVersion::V2_2) {
		int type, level;

		//class spells
		for (type=IE_IWD2_SPELL_BARD;type<IE_IWD2_SPELL_DOMAIN;type++) for(level=0;level<9;level++) {
			tmpDword = GetIWD2SpellpageSize(actor, (ieIWD2SpellType) type, level);
			stream->WriteDword(KnownSpellsOffset);
			KnownSpellsOffset+=tmpDword*16+8;
		}
		for (type=IE_IWD2_SPELL_BARD;type<IE_IWD2_SPELL_DOMAIN;type++) for(level=0;level<9;level++) {
			tmpDword = GetIWD2SpellpageSize(actor, (ieIWD2SpellType) type, level);
			stream->WriteDword(tmpDword);
		}
		//domain spells
		for (level=0;level<9;level++) {
			tmpDword = GetIWD2SpellpageSize(actor, IE_IWD2_SPELL_DOMAIN, level);
			stream->WriteDword(KnownSpellsOffset);
			KnownSpellsOffset+=tmpDword*16+8;
		}
		for (level=0;level<9;level++) {
			tmpDword = GetIWD2SpellpageSize(actor, IE_IWD2_SPELL_DOMAIN, level);
			stream->WriteDword(tmpDword);
		}
		//innates, shapes, songs
		for (type=IE_IWD2_SPELL_INNATE;type<NUM_IWD2_SPELLTYPES;type++) {
			tmpDword = GetIWD2SpellpageSize(actor, (ieIWD2SpellType) type, 0);
			stream->WriteDword(KnownSpellsOffset);
			KnownSpellsOffset+=tmpDword*16+8;
			stream->WriteDword(tmpDword);
		}
	} else {
		stream->WriteDword(KnownSpellsOffset);
		stream->WriteDword(KnownSpellsCount);
		stream->WriteDword(SpellMemorizationOffset);
		stream->WriteDword(SpellMemorizationCount);
		stream->WriteDword(MemorizedSpellsOffset);
		stream->WriteDword(MemorizedSpellsCount);
	}
	stream->WriteDword(ItemSlotsOffset);
	stream->WriteDword(ItemsOffset);
	stream->WriteDword(ItemsCount);
	stream->WriteDword(EffectsOffset);
	tmpDword = EffectsCount+VariablesCount;
	stream->WriteDword(tmpDword);
	stream->WriteResRef( actor->GetDialog(false) );
	//spells, spellbook etc

	if (actor->creVersion == CREVersion::V2_2) {
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

	//items and inventory slots
	assert(stream->GetPos() == CREOffset+ItemSlotsOffset);
	ret = PutInventory( stream, actor, Inventory_Size);
	if (ret) {
		return ret;
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

	return 0;
}

#include "plugindef.h"

GEMRB_PLUGIN(0xE507B60, "CRE File Importer")
PLUGIN_CLASS(IE_CRE_CLASS_ID, ImporterPlugin<CREImporter>)
PLUGIN_CLEANUP(ReleaseMemoryCRE)
END_PLUGIN()
