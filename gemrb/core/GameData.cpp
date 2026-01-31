/* GemRB - Infinity Engine Emulator
* Copyright (C) 2003-2005 The GemRB Project
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

#include "GameData.h"

#include "globals.h"
#include "ie_stats.h"

#include "ActorMgr.h"
#include "AnimationMgr.h"
#include "CharAnimations.h"
#include "Effect.h"
#include "EffectMgr.h"
#include "Factory.h"
#include "Game.h"
#include "ImageFactory.h"
#include "ImageMgr.h"
#include "Interface.h"
#include "Item.h"
#include "ItemMgr.h"
#include "PluginMgr.h"
#include "ResourceSource.h"
#include "ScriptedAnimation.h"
#include "Spell.h"
#include "SpellMgr.h"
#include "StoreMgr.h"
#include "VEFObject.h"

#include "Logging/Logging.h"
#include "Scriptable/Actor.h"
#include "Streams/FileStream.h"

namespace GemRB {

GEM_EXPORT GameData* gamedata;

GameData::~GameData()
{
	PaletteCache.clear();

	while (!stores.empty()) {
		Store* store = stores.begin()->second;
		stores.erase(stores.begin());
		delete store;
	}
}

Actor* GameData::GetCreature(const ResRef& creature, unsigned int PartySlot)
{
	DataStream* ds = GetResourceStream(creature, IE_CRE_CLASS_ID);
	auto actormgr = GetImporter<ActorMgr>(IE_CRE_CLASS_ID, ds);
	if (!actormgr) {
		return 0;
	}
	Actor* actor = actormgr->GetActor(PartySlot);
	return actor;
}

int GameData::LoadCreature(const ResRef& creature, unsigned int PartySlot, bool character, CREVersion versionOverride)
{
	DataStream* stream;

	Actor* actor;
	if (character) {
		path_t file = fmt::format("{}.chr", creature);
		path_t gamePath = PathJoin(core->config.GamePath, core->config.GameCharactersPath, file);
		path_t profilePath = PathJoin(UserProfilePath(), core->config.GameProfile, core->config.GameCharactersPath, file);
		path_t nPath = profilePath;
		if (!FileExists(nPath)) {
			nPath = gamePath;
		}

		stream = FileStream::OpenFile(nPath);
		auto actormgr = GetImporter<ActorMgr>(IE_CRE_CLASS_ID, stream);
		if (!actormgr) {
			return -1;
		}
		actor = actormgr->GetActor(PartySlot);
	} else {
		actor = GetCreature(creature, PartySlot);
	}

	if (!actor) {
		return -1;
	}

	if (versionOverride != CREVersion::GemRB) {
		actor->creVersion = versionOverride;
	}

	actor->AreaName = core->GetGame()->CurrentArea;
	if (actor->BaseStats[IE_STATE_ID] & STATE_DEAD) {
		actor->SetStance(IE_ANI_TWITCH);
	} else {
		actor->SetStance(IE_ANI_AWAKE);
	}
	actor->SetOrientation(S, false);

	if (PartySlot == 0) {
		return core->GetGame()->AddNPC(actor);
	} else {
		return core->GetGame()->JoinParty(actor, JP_JOIN | JP_INITPOS);
	}
}

/** Loads a 2DA Table, returns -1 on error or the Table Index on success */
AutoTable GameData::LoadTable(const ResRef& tableRef, bool silent)
{
	if (tables.count(tableRef)) {
		return tables.at(tableRef);
	}

	PluginHolder<TableMgr> tm = MakePluginHolder<TableMgr>(IE_2DA_CLASS_ID);
	if (!tm) {
		return nullptr;
	}

	DataStream* str = GetResourceStream(tableRef, IE_2DA_CLASS_ID, silent);
	if (!str) {
		return nullptr;
	}

	if (!tm->Open(std::unique_ptr<DataStream> { str })) {
		return nullptr;
	}

	tables[tableRef] = tm;
	return tm;
}

Holder<Palette> GameData::GetPalette(const ResRef& resname)
{
	auto iter = PaletteCache.find(resname);
	if (iter != PaletteCache.end())
		return iter->second;

	ResourceHolder<ImageMgr> im = GetResourceHolder<ImageMgr>(resname);
	if (im == nullptr) {
		PaletteCache[resname] = nullptr;
		return NULL;
	}

	Holder<Palette> palette = MakeHolder<Palette>(true);
	im->GetPalette(256, *palette);
	PaletteCache[resname] = palette;
	return palette;
}

Item* GameData::GetItem(const ResRef& resname, bool silent)
{
	if (resname.IsEmpty()) {
		return nullptr;
	}

	Item* item = ItemCache.GetResource(resname);
	if (item) {
		return item;
	}
	DataStream* str = GetResourceStream(resname, IE_ITM_CLASS_ID, silent);
	PluginHolder<ItemMgr> sm = GetImporter<ItemMgr>(IE_ITM_CLASS_ID, str);
	if (!sm) {
		return nullptr;
	}

	auto newItem = ItemCache.SetAt(resname).first;
	newItem->Name = resname;
	sm->GetItem(newItem);

	return newItem;
}

//you can supply name for faster access
void GameData::FreeItem(Item const* /*itm*/, const ResRef& name, bool free)
{
	ItemCache.DecRef(name, free);
}

Spell* GameData::GetSpell(const ResRef& resname, bool silent)
{
	if (resname.IsEmpty()) {
		return nullptr;
	}

	Spell* spell = SpellCache.GetResource(resname);
	if (spell) {
		return spell;
	}
	DataStream* str = GetResourceStream(resname, IE_SPL_CLASS_ID, silent);
	PluginHolder<SpellMgr> sm = MakePluginHolder<SpellMgr>(IE_SPL_CLASS_ID);
	if (!sm) {
		delete str;
		return nullptr;
	}
	if (!sm->Open(str)) {
		return nullptr;
	}

	auto newSpell = SpellCache.SetAt(resname).first;
	newSpell->Name = resname;
	sm->GetSpell(newSpell, silent);

	return newSpell;
}

void GameData::FreeSpell(const Spell* /*spl*/, const ResRef& name, bool free)
{
	SpellCache.DecRef(name, free);
}

Effect* GameData::GetEffect(const ResRef& resname)
{
	const Effect* effect = EffectCache.GetResource(resname);
	if (effect) {
		return new Effect(*effect);
	}
	DataStream* str = GetResourceStream(resname, IE_EFF_CLASS_ID);
	PluginHolder<EffectMgr> em = MakePluginHolder<EffectMgr>(IE_EFF_CLASS_ID);
	if (!em) {
		delete str;
		return nullptr;
	}
	if (!em->Open(str)) {
		return nullptr;
	}

	auto newEffect = em->GetEffect();
	if (newEffect == nullptr) {
		return nullptr;
	}

	EffectCache.SetAt(resname, *newEffect);

	auto effectCopy = new Effect(std::move(*newEffect));
	delete newEffect;

	return effectCopy;
}

void GameData::FreeEffect(const Effect* /*eff*/, const ResRef& name, bool free)
{
	EffectCache.DecRef(name, free);
}

//if the default setup doesn't fit for an animation
//create a vvc for it!
ScriptedAnimation* GameData::GetScriptedAnimation(const ResRef& effect, bool doublehint)
{
	ScriptedAnimation* ret = NULL;

	if (Exists(effect, IE_VVC_CLASS_ID, true)) {
		DataStream* ds = GetResourceStream(effect, IE_VVC_CLASS_ID);
		ret = new ScriptedAnimation(ds);
	} else {
		auto af = GetFactoryResourceAs<AnimationFactory>(effect, IE_BAM_CLASS_ID);
		if (af) {
			ret = new ScriptedAnimation();
			ret->LoadAnimationFactory(*af, doublehint ? 2 : 0);
		}
	}
	if (ret) {
		ret->ResName = effect;
	}
	return ret;
}

VEFObject* GameData::GetVEFObject(const ResRef& vefRef, bool doublehint)
{
	VEFObject* ret = nullptr;

	if (Exists(vefRef, IE_VEF_CLASS_ID, true)) {
		DataStream* ds = GetResourceStream(vefRef, IE_VEF_CLASS_ID);
		ret = new VEFObject();
		ret->ResName = vefRef;
		ret->LoadVEF(ds);
	} else {
		if (Exists(vefRef, IE_2DA_CLASS_ID, true)) {
			ret = new VEFObject();
			ret->Load2DA(vefRef);
		} else {
			ScriptedAnimation* sca = GetScriptedAnimation(vefRef, doublehint);
			if (sca) {
				ret = new VEFObject(sca);
			}
		}
	}
	return ret;
}

// Return single BAM frame as a sprite. Use if you want one frame only,
// otherwise it's not efficient
Holder<Sprite2D> GameData::GetBAMSprite(const ResRef& resRef, int cycle, int frame, bool silent)
{
	Holder<Sprite2D> tspr;
	auto af = GetFactoryResourceAs<const AnimationFactory>(resRef, IE_BAM_CLASS_ID, silent);
	if (!af) return nullptr;
	if (cycle == -1)
		tspr = af->GetFrameWithoutCycle((unsigned short) frame);
	else
		tspr = af->GetFrame((unsigned short) frame, (unsigned char) cycle);
	return tspr;
}

Holder<Sprite2D> GameData::GetAnySprite(const ResRef& resRef, int cycle, int frame, bool silent)
{
	Holder<Sprite2D> img = GetBAMSprite(resRef, cycle, frame, silent);
	if (img) return img;
	if (frame > 0) return nullptr;

	// try static image formats to support PNG
	ResourceHolder<ImageMgr> im = GetResourceHolder<ImageMgr>(resRef);
	if (im) {
		img = im->GetSprite2D();
	}
	return img;
}

Factory::object_t GameData::GetFactoryResource(const ResRef& resName, SClass_ID type, bool silent)
{
	if (resName.IsEmpty()) return nullptr;

	// already cached?
	int objectIndex = factory.IsLoaded(resName, type);
	if (objectIndex != -1) return factory.GetFactoryObject(objectIndex);

	switch (type) {
		case IE_BAM_CLASS_ID:
			{
				DataStream* str = GetResourceStream(resName, type, silent);
				if (str) {
					auto importer = GetImporter<AnimationMgr>(IE_BAM_CLASS_ID, str);
					if (importer == nullptr) {
						return nullptr;
					}

					auto af = importer->GetAnimationFactory(resName);
					factory.AddFactoryObject(af);
					return af;
				}
				return NULL;
			}
		case IE_BMP_CLASS_ID:
			{
				ResourceHolder<ImageMgr> img = GetResourceHolder<ImageMgr>(resName, silent);
				if (img) {
					auto fact = img->GetImageFactory(resName);
					factory.AddFactoryObject(fact);
					return fact;
				}

				return NULL;
			}
		default:
			Log(MESSAGE, "KEYImporter", "{} files are not supported!", TypeExt(type));
			return nullptr;
	}
}

Store* GameData::GetStore(const ResRef& resRef)
{
	StoreMap::iterator it = stores.find(resRef);
	if (it != stores.end()) {
		return it->second;
	}

	DataStream* str = GetResourceStream(resRef, IE_STO_CLASS_ID);
	PluginHolder<StoreMgr> sm = MakePluginHolder<StoreMgr>(IE_STO_CLASS_ID);
	if (sm == nullptr) {
		delete str;
		return nullptr;
	}
	if (!sm->Open(str)) {
		return nullptr;
	}

	Store* store = sm->GetStore(new Store());
	if (store == nullptr) {
		return nullptr;
	}

	store->Name = resRef;
	stores[store->Name] = store;
	return store;
}

void GameData::SaveStore(Store* store)
{
	if (!store)
		return;
	StoreMap::iterator it = stores.find(store->Name);
	if (it == stores.end()) {
		error("GameData", "Saving a store that wasn't cached.");
	}

	PluginHolder<StoreMgr> sm = MakePluginHolder<StoreMgr>(IE_STO_CLASS_ID);
	if (sm == nullptr) {
		error("GameData", "Can't save store to cache.");
	}

	FileStream str;

	if (!str.Create(store->Name.c_str(), IE_STO_CLASS_ID)) {
		error("GameData", "Can't create file while saving store.");
	}
	if (!sm->PutStore(&str, store)) {
		error("GameData", "Error saving store.");
	}

	stores.erase(it);
	delete store;
}

void GameData::SaveAllStores()
{
	while (!stores.empty()) {
		SaveStore(stores.begin()->second);
	}
}

void GameData::ReadItemSounds()
{
	AutoTable itemsnd = LoadTable("itemsnd");
	if (!itemsnd) {
		return;
	}

	TableMgr::index_t rowCount = itemsnd->GetRowCount();
	TableMgr::index_t colCount = itemsnd->GetColumnCount();
	for (TableMgr::index_t i = 0; i < rowCount; i++) {
		ItemSounds[i] = {};
		for (TableMgr::index_t j = 0; j < colCount; j++) {
			ResRef snd = itemsnd->QueryField(i, j);
			if (snd == ResRef("*")) break;
			ItemSounds[i].push_back(snd);
		}
	}
}

bool GameData::GetItemSound(ResRef& Sound, ieDword ItemType, AnimRef ID, ieDword Col)
{
	Sound.Reset();

	if (ItemSounds.empty()) {
		ReadItemSounds();
	}

	if (Col >= ItemSounds[ItemType].size()) {
		return false;
	}

	if (!ID.IsEmpty() && ID[1] == 'A') {
		//the last 4 item sounds are used for '1A', '2A', '3A' and '4A' (pst)
		//item animation types
		ItemType = ItemSounds.size() - 4 + ID[0] - '1';
	}

	if (ItemType >= (ieDword) ItemSounds.size()) {
		return false;
	}
	Sound = ItemSounds[ItemType][Col];
	return true;
}

int GameData::GetSwingCount(ieDword ItemType)
{
	if (ItemSounds.empty()) {
		ReadItemSounds();
	}

	// everything but the unrelated preceding columns (IS_SWINGOFFSET)
	return ItemSounds[ItemType].size() - 2;
}

int GameData::GetRacialTHAC0Bonus(ieDword proficiency, const std::string& raceName)
{
	static bool ignore = false;
	if (ignore) {
		return 0;
	}

	AutoTable raceTHAC0Bonus = LoadTable("racethac", true);
	// not all games have the table
	if (!raceTHAC0Bonus) {
		ignore = true;
		return 0;
	}

	return raceTHAC0Bonus->QueryFieldSigned<int>(fmt::to_string(proficiency), raceName);
}

bool GameData::HasInfravision(const std::string& raceName)
{
	AutoTable racialInfravision = LoadTable("racefeat", true);

	if (!racialInfravision) return false;
	return racialInfravision->QueryFieldSigned<int>(raceName, "VALUE") & 1;
}

int GameData::GetSpellAbilityDie(const Actor* target, int which)
{
	AutoTable spellAbilityDie = LoadTable("clssplab", true);
	if (!spellAbilityDie) {
		return 6;
	}

	ieDword cls = target->GetActiveClass();
	if (cls >= spellAbilityDie->GetRowCount()) cls = 0;
	return spellAbilityDie->QueryFieldSigned<int>(cls, which);
}

int GameData::GetTrapSaveBonus(ieDword level, int cls)
{
	if (!core->HasFeature(GFFlags::RULES_3ED)) return 0;

	AutoTable trapSaveBonus = LoadTable("trapsave", true);
	if (!trapSaveBonus) return 0;
	return trapSaveBonus->QueryFieldSigned<int>(level - 1, cls - 1);
}

int GameData::GetTrapLimit(Scriptable* trapper)
{
	AutoTable trapLimit = LoadTable("traplimt", true);

	if (trapper->Type != ST_ACTOR || !trapLimit) {
		return 6; // not using table default, since EE's file has it at 0
	}

	const Actor* caster = (Actor*) trapper;
	ieDword kit = caster->GetStat(IE_KIT);
	std::string rowName;
	if (kit != 0x4000) { // KIT_BASECLASS
		rowName = caster->GetKitName(kit);
	} else {
		ieDword cls = caster->GetActiveClass();
		rowName = caster->GetClassName(cls);
	}

	return trapLimit->QueryFieldSigned<int>(rowName, "LIMIT");
}

int GameData::GetSummoningLimit(ieDword sex)
{
	AutoTable summoningLimit = LoadTable("summlimt", true);
	if (!summoningLimit) return 6;

	unsigned int row = 1000;
	switch (sex) {
		case SEX_SUMMON:
		case SEX_SUMMON_DEMON:
			row = 0;
			break;
		case SEX_BOTH:
			row = 1;
			break;
		default:
			break;
	}
	return summoningLimit->QueryFieldSigned<int>(row, 0);
}

// preload converted colors
void GameData::PreloadColors()
{
	AutoTable colorTable = LoadTable("colors", true);
	assert(colorTable);
	for (TableMgr::index_t r = 0; r < colorTable->GetRowCount(); r++) {
		ieDword c = colorTable->QueryFieldUnsigned<ieDword>(r, 0);
		colors[colorTable->GetRowName(r)] = Color(c);
	}
}

const Color& GameData::GetColor(const TableMgr::key_t& row) const
{
	const auto it = colors.find(row.c_str());
	if (it != colors.end()) {
		return it->second;
	}
	return ColorRed;
}

void GameData::ModifyColor(GUIColors idx, const Color& newColor)
{
	colors[displaymsg->GetColorName(idx)] = newColor;
}

// wspatck bonus handling
int GameData::GetWeaponStyleAPRBonus(int row, int col)
{
	// preload optimized version, since this gets called each tick several times
	if (weaponStyleAPRBonusMax.IsZero()) {
		AutoTable bonusTable = LoadTable("wspatck", true);
		if (!bonusTable) {
			weaponStyleAPRBonusMax.w = -1;
			return 0;
		}

		TableMgr::index_t rows = bonusTable->GetRowCount();
		TableMgr::index_t cols = bonusTable->GetColumnCount();
		weaponStyleAPRBonusMax.h = rows;
		weaponStyleAPRBonusMax.w = cols;
		weaponStyleAPRBonus.resize(rows * cols);
		for (TableMgr::index_t i = 0; i < rows; i++) {
			for (TableMgr::index_t j = 0; j < cols; j++) {
				int tmp = bonusTable->QueryFieldSigned<int>(i, j);
				// negative values relate to x/2, so we adjust them
				// positive values relate to x, so we must times by 2
				if (tmp < 0) {
					tmp = -2 * tmp - 1;
				} else {
					tmp *= 2;
				}
				weaponStyleAPRBonus[i * cols + j] = tmp;
			}
		}
	} else if (weaponStyleAPRBonusMax.w == -1) {
		return 0;
	}

	if (row >= weaponStyleAPRBonusMax.h) {
		row = weaponStyleAPRBonusMax.h - 1;
	}
	if (col >= weaponStyleAPRBonusMax.w) {
		col = weaponStyleAPRBonusMax.w - 1;
	}
	return weaponStyleAPRBonus[row * weaponStyleAPRBonusMax.w + col];
}

bool GameData::ReadResRefTable(const ResRef& tableName, std::vector<ResRef>& data)
{
	if (!data.empty()) {
		data.clear();
	}
	AutoTable tm = LoadTable(tableName);
	if (!tm) {
		Log(ERROR, "GameData", "Cannot find {}.2da.", tableName);
		return false;
	}

	TableMgr::index_t count = tm->GetRowCount();
	data.resize(count);
	for (TableMgr::index_t i = 0; i < count; i++) {
		data[i] = tm->QueryField(i, 0);
		// * marks an empty resource
		if (IsStar(data[i])) {
			data[i].Reset();
		}
	}
	return true;
}

void GameData::ReadSpellProtTable()
{
	AutoTable tab = LoadTable("splprot");
	if (!tab) {
		return;
	}
	TableMgr::index_t rowCount = tab->GetRowCount();
	spellProt.resize(rowCount);
	for (TableMgr::index_t i = 0; i < rowCount; ++i) {
		ieDword stat = core->TranslateStat(tab->QueryField(i, 0));
		spellProt[i].stat = (ieWord) stat;
		spellProt[i].value = tab->QueryFieldUnsigned<ieDword>(i, 1);
		spellProt[i].relation = tab->QueryFieldUnsigned<ieWord>(i, 2);
	}
}

static const IWDIDSEntry badEntry = {};
const IWDIDSEntry& GameData::GetSpellProt(index_t idx)
{
	if (spellProt.empty()) {
		ReadSpellProtTable();
	}

	if (idx >= spellProt.size()) {
		return badEntry;
	}
	return spellProt[idx];
}

int GameData::GetReputationMod(int column)
{
	assert(column >= 0 && column <= 8);
	AutoTable reputationMod = LoadTable("reputati", true);
	if (!reputationMod) return false;

	int reputation = core->GetGame()->Reputation / 10 - 1;
	if (reputation > 19) {
		reputation = 19;
	} else if (reputation < 0) {
		reputation = 0;
	}

	return reputationMod->QueryFieldSigned<int>(reputation, column);
}

// reads to and from table of area name mappings for the pst worldmap
int GameData::GetAreaAlias(const ResRef& areaName)
{
	static bool ignore = false;
	if (ignore) {
		return -1;
	}

	if (AreaAliasTable.empty()) {
		AutoTable table = LoadTable("wmaplay", true);
		if (!table) {
			ignore = true;
			return -1;
		}

		TableMgr::index_t idx = table->GetRowCount();
		while (idx--) {
			ResRef key = table->GetRowName(idx);
			ieDword value = table->QueryFieldUnsigned<ieDword>(idx, 0);
			AreaAliasTable[key] = value;
		}
	}

	if (AreaAliasTable.count(areaName)) {
		return AreaAliasTable.at(areaName);
	}
	return -1;
}

int GameData::GetSpecialSpell(const ResRef& resref)
{
	static bool ignore = false;
	if (ignore) {
		return -1;
	}

	if (SpecialSpells.empty()) {
		AutoTable table = LoadTable("splspec");
		if (!table) {
			ignore = true;
			return 0;
		}

		TableMgr::index_t specialSpellsCount = table->GetRowCount();
		SpecialSpells.resize(specialSpellsCount);
		for (TableMgr::index_t i = 0; i < specialSpellsCount; ++i) {
			SpecialSpells[i].resref = table->GetRowName(i);
			SpecialSpells[i].flags = table->QueryFieldSigned<int>(i, 0);
			SpecialSpells[i].amount = table->QueryFieldSigned<int>(i, 1);
			SpecialSpells[i].bonus_limit = table->QueryFieldSigned<int>(i, 2);
		}
	}

	for (const auto& spell : SpecialSpells) {
		if (resref == spell.resref) {
			return spell.flags;
		}
	}
	return 0;
}

// disable spells based on some circumstances
int GameData::CheckSpecialSpell(const ResRef& resRef, const Actor* actor)
{
	int sp = GetSpecialSpell(resRef);

	// the identify spell is always disabled on the menu
	if (sp & SpecialSpell::Identify) {
		return SpecialSpell::Identify;
	}

	// if actor is silenced, and spell cannot be cast in silence, disable it
	if (!(sp & SpecialSpell::Silence) && actor->CheckSilenced()) {
		return SpecialSpell::Silence;
	}

	// disable spells causing surges to be cast while in a surge (prevents nesting)
	if (sp & SpecialSpell::Surge) {
		return SpecialSpell::Surge;
	}

	return 0;
}

const SurgeSpell& GameData::GetSurgeSpell(unsigned int idx)
{
	if (SurgeSpells.empty()) {
		AutoTable table = LoadTable("wildmag");
		assert(table);

		SurgeSpell ss;
		for (TableMgr::index_t i = 0; i < table->GetRowCount(); i++) {
			ss.spell = table->QueryField(i, 0);
			ss.message = table->QueryFieldAsStrRef(i, 1);
			// comment ignored
			SurgeSpells.push_back(ss);
		}
	}
	assert(idx < SurgeSpells.size());

	return SurgeSpells[idx];
}

ResRef GameData::GetFist(int cls, int level)
{
	static bool ignore = false;
	static ResRef defaultFist = "FIST";
	if (ignore) {
		return defaultFist;
	}

	AutoTable fistWeap = LoadTable("fistweap");
	if (!fistWeap) {
		ignore = true;
		return defaultFist;
	}

	// extend the last level value to infinity
	static int cols = fistWeap->GetColumnCount();
	if (level >= cols) level = cols - 1;

	TableMgr::index_t row = fistWeap->GetRowIndex(fmt::to_string(cls));
	return fistWeap->QueryField(row, level);
}

// read from our unhardcoded monk bonus table
int GameData::GetMonkBonus(int bonusType, int level)
{
	// not a monk?
	if (level == 0) return 0;

	static bool ignore = false;
	if (ignore) {
		return 0;
	}

	AutoTable monkBon = LoadTable("monkbon");
	if (!monkBon) {
		ignore = true;
		return 0;
	}

	// extend the last level value to infinity
	static int cols = monkBon->GetColumnCount();
	if (level >= cols) level = cols;

	return monkBon->QueryFieldSigned<int>(bonusType, level - 1);
}

// AC  CRITICALHITBONUS   DAMAGEBONUS   THAC0BONUSRIGHT   THAC0BONUSLEFT   PHYSICALSPEED   ACVSMISSLE
int GameData::GetWeaponStyleBonus(int style, int stars, int bonusType)
{
	if (stars == 0) return 0;

	static std::array<ResRef, 4> weaponStyles = { "wstwowpn", "wstwohnd", "wsshield", "wssingle" };
	static std::array<short, 4> ignore = { 0 };
	if (ignore[style] == 1) {
		return 0;
	}

	if (ignore[style] == 0) {
		AutoTable styleTable = LoadTable(weaponStyles[style]);
		if (!styleTable) {
			ignore[style] = 1;
			return 0;
		}

		TableMgr::index_t cols = styleTable->GetColumnCount();
		for (int star = 0; star <= STYLE_STAR_MAX; star++) {
			for (TableMgr::index_t bonus = 0; bonus < cols; bonus++) {
				weaponStyleBoni[style][star][bonus] = styleTable->QueryFieldSigned<int>(star, bonus);
			}
		}
		ignore[style] = 2;
	}

	return weaponStyleBoni[style][stars][bonusType];
}

int GameData::GetWSpecialBonus(int bonusType, int stars)
{
	static bool ignore = false;
	if (ignore) {
		return 0;
	}

	AutoTable wSpecial = LoadTable("wspecial", true);
	if (!wSpecial) {
		ignore = true;
		return 0;
	}

	static int rows = static_cast<int>(wSpecial->GetRowCount());
	if (stars >= rows) stars = rows - 1;

	return wSpecial->QueryFieldSigned<int>(stars, bonusType);
}

const std::vector<int>& GameData::GetBonusSpells(int ability)
{
	static bool ignore = false;
	static const std::vector<int> NoBonus(9, 0);
	if (ignore || !ability) {
		return NoBonus;
	}

	if (bonusSpells.empty()) {
		// iwd2 has mxsplbon instead, since all casters get a bonus with high enough stats (which are not always wisdom)
		// luckily, they both use the same format
		AutoTable mxSplBon;
		if (core->HasFeature(GFFlags::RULES_3ED)) {
			mxSplBon = LoadTable("mxsplbon");
		} else {
			mxSplBon = LoadTable("mxsplwis");
		}
		if (!mxSplBon) {
			ignore = true;
			return NoBonus;
		}

		auto splLevels = mxSplBon->GetColumnCount();
		int maxStat = core->GetMaximumAbility();
		bonusSpells.resize(maxStat); // wastes some memory, but makes addressing easier
		for (TableMgr::index_t row = 0; row < mxSplBon->GetRowCount(); ++row) {
			int statValue = atoi(mxSplBon->GetRowName(row).c_str()) - 1;
			assert(statValue >= 0 && statValue < maxStat);
			std::vector<int> bonuses(splLevels);
			for (TableMgr::index_t i = 0; i < splLevels; i++) {
				bonuses[i] = mxSplBon->QueryFieldSigned<int>(row, i);
			}
			bonusSpells[statValue] = std::move(bonuses);
		}
	}

	return bonusSpells[ability - 1];
}

ieByte GameData::GetItemAnimation(const ResRef& itemRef)
{
	static bool ignore = false;
	if (ignore) {
		return 0;
	}

	if (itemAnims.empty()) {
		AutoTable table = LoadTable("itemanim");
		if (!table) {
			ignore = true;
			return 0;
		}

		for (TableMgr::index_t i = 0; i < table->GetRowCount(); ++i) {
			ResRef item = table->GetRowName(i);
			itemAnims[item] = table->QueryFieldUnsigned<ieByte>(i, 0);
		}
	}

	const auto& itemIt = itemAnims.find(itemRef);
	if (itemIt == itemAnims.end()) {
		return 0;
	}
	return itemIt->second;
}

const std::vector<ItemUseType>& GameData::GetItemUse()
{
	static bool ignore = false;
	static const std::vector<ItemUseType> NoData {};
	if (ignore) {
		return NoData;
	}

	if (itemUse.empty()) {
		AutoTable table = LoadTable("itemuse");
		if (!table) {
			ignore = true;
			return NoData;
		}

		TableMgr::index_t tableCount = table->GetRowCount();
		itemUse.resize(tableCount);
		for (TableMgr::index_t i = 0; i < tableCount; i++) {
			itemUse[i].stat = static_cast<ieByte>(core->TranslateStat(table->QueryField(i, 0)));
			itemUse[i].table = table->QueryField(i, 1);
			itemUse[i].mcol = table->QueryFieldUnsigned<ieByte>(i, 2);
			itemUse[i].vcol = table->QueryFieldUnsigned<ieByte>(i, 3);
			itemUse[i].which = table->QueryFieldUnsigned<ieByte>(i, 4);
			// limiting it to 0 or 1 to avoid crashes
			if (itemUse[i].which != 1) {
				itemUse[i].which = 0;
			}
		}
	}

	return itemUse;
}

int GameData::GetMiscRule(const TableMgr::key_t& rowName)
{
	AutoTable miscRule = LoadTable("miscrule", true);
	assert(miscRule);

	return miscRule->QueryFieldSigned<int>(rowName, "VALUE");
}

int GameData::GetDifficultyMod(ieDword mod, Difficulty difficulty)
{
	static bool ignore = false;
	if (ignore) {
		return 0;
	}

	AutoTable difficultyLevels = LoadTable("difflvls");
	if (!difficultyLevels) {
		ignore = true;
		return 0;
	}

	return difficultyLevels->QueryFieldSigned<int>(mod, UnderType(difficulty));
}

int GameData::GetXPBonus(ieDword bonusType, ieDword level)
{
	static bool ignore = false;
	if (ignore) {
		return 0;
	}

	AutoTable xpBonus = LoadTable("xpbonus");
	if (!xpBonus) {
		ignore = true;
		return 0;
	}

	// use the highest bonus for levels outside table bounds
	if (level > xpBonus->GetColumnCount()) {
		level = xpBonus->GetColumnCount();
	}

	return xpBonus->QueryFieldSigned<int>(bonusType, level - 1);
}

int GameData::GetVBData(const TableMgr::key_t& rowName)
{
	AutoTable vbDetails = LoadTable("vcdetail", true);
	assert(vbDetails);

	return vbDetails->QueryFieldSigned<int>(rowName, "VALUE");
}

}
