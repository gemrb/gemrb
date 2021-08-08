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

#include "ActorMgr.h"
#include "AnimationMgr.h"
#include "Cache.h"
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
#include "ResourceDesc.h"
#include "ScriptedAnimation.h"
#include "Spell.h"
#include "SpellMgr.h"
#include "StoreMgr.h"
#include "VEFObject.h"
#include "Scriptable/Actor.h"
#include "System/FileStream.h"

#include <cstdio>

namespace GemRB {

static void ReleaseItem(void *poi)
{
	delete ((Item *) poi);
}

static void ReleaseSpell(void *poi)
{
	delete ((Spell *) poi);
}

static void ReleaseEffect(void *poi)
{
	delete ((Effect *) poi);
}

GEM_EXPORT GameData* gamedata;

GameData::GameData()
{
	factory = new Factory();
}

GameData::~GameData()
{
	delete factory;
}

void GameData::ClearCaches()
{
	ItemCache.RemoveAll(ReleaseItem);
	SpellCache.RemoveAll(ReleaseSpell);
	EffectCache.RemoveAll(ReleaseEffect);
	PaletteCache.clear ();
	colors.clear();

	while (!stores.empty()) {
		Store *store = stores.begin()->second;
		stores.erase(stores.begin());
		delete store;
	}
}

Actor *GameData::GetCreature(const char* ResRef, unsigned int PartySlot)
{
	DataStream* ds = GetResource( ResRef, IE_CRE_CLASS_ID );
	auto actormgr = GetImporter<ActorMgr>(IE_CRE_CLASS_ID, ds);
	if (!actormgr) {
		return 0;
	}
	Actor* actor = actormgr->GetActor(PartySlot);
	return actor;
}

int GameData::LoadCreature(const char* ResRef, unsigned int PartySlot, bool character, int VersionOverride)
{
	DataStream *stream;

	Actor* actor;
	if (character) {
		char nPath[_MAX_PATH], fName[16];
		snprintf( fName, sizeof(fName), "%s.chr", ResRef);
		PathJoin(nPath, core->config.GamePath, "characters", fName, nullptr);
		stream = FileStream::OpenFile(nPath);
		auto actormgr = GetImporter<ActorMgr>(IE_CRE_CLASS_ID, stream);
		if (!actormgr) {
			return -1;
		}
		actor = actormgr->GetActor(PartySlot);
	} else {
		actor = GetCreature(ResRef, PartySlot);
	}

	if ( !actor ) {
		return -1;
	}

	if (VersionOverride != -1) {
		actor->version = VersionOverride;
	}

	//both fields are of length 9, make this sure!
	actor->Area = core->GetGame()->CurrentArea;
	if (actor->BaseStats[IE_STATE_ID] & STATE_DEAD) {
		actor->SetStance( IE_ANI_TWITCH );
	} else {
		actor->SetStance( IE_ANI_AWAKE );
	}
	actor->SetOrientation( 0, false );

	if ( PartySlot != 0 ) {
		return core->GetGame()->JoinParty( actor, JP_JOIN|JP_INITPOS );
	}
	else {
		return core->GetGame()->AddNPC( actor );
	}
}

/** Loads a 2DA Table, returns -1 on error or the Table Index on success */
AutoTable GameData::LoadTable(const char *tabname, bool silent)
{
	ResRef resRef = tabname;
	if (tables.count(resRef)) {
		return tables.at(resRef);
	}

	DataStream* str = GetResource(tabname, IE_2DA_CLASS_ID, silent);
	if (!str) {
		return nullptr;
	}
	PluginHolder<TableMgr> tm = MakePluginHolder<TableMgr>(IE_2DA_CLASS_ID);
	if (!tm) {
		delete str;
		return nullptr;
	}
	if (!tm->Open(str)) {
		return nullptr;
	}
	
	tables[resRef] = tm;
	return tm;
}

AutoTable GameData::LoadTable(const ResRef& resRef, bool silent)
{
	return LoadTable(resRef.CString(), silent);
}
/** Gets the index of a loaded table, returns -1 on error */
AutoTable GameData::GetTable(const ResRef &resRef) const
{
	if (tables.count(resRef)) {
		return tables.at(resRef);
	}
	return nullptr;
}

void GameData::ResetTables()
{
	tables.clear();
}

PaletteHolder GameData::GetPalette(const ResRef& resname)
{
	auto iter = PaletteCache.find(resname);
	if (iter != PaletteCache.end())
		return iter->second;

	ResourceHolder<ImageMgr> im = GetResourceHolder<ImageMgr>(resname);
	if (im == nullptr) {
		PaletteCache[resname] = nullptr;
		return NULL;
	}

	PaletteHolder palette = MakeHolder<Palette>();
	im->GetPalette(256,palette->col);
	palette->named=true;
	PaletteCache[resname] = palette;
	return palette;
}

Item* GameData::GetItem(const ResRef &resname, bool silent)
{
	Item *item = (Item *) ItemCache.GetResource(resname);
	if (item) {
		return item;
	}
	DataStream* str = GetResource(resname, IE_ITM_CLASS_ID, silent);
	PluginHolder<ItemMgr> sm = GetImporter<ItemMgr>(IE_ITM_CLASS_ID, str);
	if (!sm) {
		return nullptr;
	}

	item = new Item();
	item->Name = resname;
	sm->GetItem( item );

	ItemCache.SetAt(resname, (void *) item);
	return item;
}

//you can supply name for faster access
void GameData::FreeItem(Item const *itm, const ResRef &name, bool free)
{
	int res;

	res = ItemCache.DecRef((const void *) itm, name, free);
	if (res<0) {
		error("Core", "Corrupted Item cache encountered (reference count went below zero), Item name is: %.8s\n", name.CString());
	}
	if (res) return;
	if (free) delete itm;
}

Spell* GameData::GetSpell(const ResRef &resname, bool silent)
{
	Spell *spell = (Spell *) SpellCache.GetResource(resname);
	if (spell) {
		return spell;
	}
	DataStream* str = GetResource( resname, IE_SPL_CLASS_ID, silent );
	PluginHolder<SpellMgr> sm = MakePluginHolder<SpellMgr>(IE_SPL_CLASS_ID);
	if (!sm) {
		delete ( str );
		return NULL;
	}
	if (!sm->Open(str)) {
		return NULL;
	}

	spell = new Spell();
	spell->Name = resname;
	sm->GetSpell( spell, silent );

	SpellCache.SetAt(resname, (void *) spell);
	return spell;
}

void GameData::FreeSpell(const Spell *spl, const ResRef &name, bool free)
{
	int res = SpellCache.DecRef((const void *) spl, name, free);
	if (res<0) {
		error("Core", "Corrupted Spell cache encountered (reference count went below zero), Spell name is: %.8s or %.8s\n",
			name.CString(), spl->Name.CString());
	}
	if (res) return;
	if (free) delete spl;
}

Effect* GameData::GetEffect(const ResRef &resname)
{
	Effect *effect = (Effect *) EffectCache.GetResource(resname);
	if (effect) {
		return effect;
	}
	DataStream* str = GetResource( resname, IE_EFF_CLASS_ID );
	PluginHolder<EffectMgr> em = MakePluginHolder<EffectMgr>(IE_EFF_CLASS_ID);
	if (!em) {
		delete ( str );
		return NULL;
	}
	if (!em->Open(str)) {
		return NULL;
	}

	effect = em->GetEffect();
	if (effect == NULL) {
		return NULL;
	}

	EffectCache.SetAt(resname, (void *) effect);
	return effect;
}

void GameData::FreeEffect(const Effect *eff, const ResRef &name, bool free)
{
	int res = EffectCache.DecRef((const void *) eff, name, free);
	if (res<0) {
		error("Core", "Corrupted Effect cache encountered (reference count went below zero), Effect name is: %.8s\n", name.CString());
	}
	if (res) return;
	if (free) delete eff;
}

//if the default setup doesn't fit for an animation
//create a vvc for it!
ScriptedAnimation* GameData::GetScriptedAnimation(const ResRef &effect, bool doublehint)
{
	ScriptedAnimation *ret = NULL;

	if (Exists( effect, IE_VVC_CLASS_ID, true ) ) {
		DataStream *ds = GetResource( effect, IE_VVC_CLASS_ID );
		ret = new ScriptedAnimation(ds);
	} else {
		AnimationFactory *af = (AnimationFactory *)
			GetFactoryResource(effect, IE_BAM_CLASS_ID);
		if (af) {
			ret = new ScriptedAnimation();
			ret->LoadAnimationFactory( af, doublehint?2:0);
		}
	}
	if (ret) {
		ret->ResName = effect;
	}
	return ret;
}

VEFObject* GameData::GetVEFObject(const char *effect, bool doublehint)
{
	VEFObject *ret = NULL;

	if (Exists( effect, IE_VEF_CLASS_ID, true ) ) {
		DataStream *ds = GetResource( effect, IE_VEF_CLASS_ID );
		ret = new VEFObject();
		ret->ResName = effect;
		ret->LoadVEF(ds);
	} else {
		if (Exists( effect, IE_2DA_CLASS_ID, true ) ) {
			ret = new VEFObject();
			ret->Load2DA(effect);
		} else {
			ScriptedAnimation *sca = GetScriptedAnimation(effect, doublehint);
			if (sca) {
				ret = new VEFObject(sca);
			}
		}
	}
	return ret;
}

// Return single BAM frame as a sprite. Use if you want one frame only,
// otherwise it's not efficient
Holder<Sprite2D> GameData::GetBAMSprite(const ResRef &resRef, int cycle, int frame, bool silent)
{
	Holder<Sprite2D> tspr;
	const AnimationFactory* af = (const AnimationFactory *)
		GetFactoryResource(resRef, IE_BAM_CLASS_ID, silent);
	if (!af) return 0;
	if (cycle == -1)
		tspr = af->GetFrameWithoutCycle( (unsigned short) frame );
	else
		tspr = af->GetFrame( (unsigned short) frame, (unsigned char) cycle );
	return tspr;
}

Holder<Sprite2D> GameData::GetAnySprite(const char *resRef, int cycle, int frame, bool silent)
{
	Holder<Sprite2D> img = gamedata->GetBAMSprite(resRef, cycle, frame, silent);
	if (img) return img;

	// try static image formats to support PNG
	ResourceHolder<ImageMgr> im = GetResourceHolder<ImageMgr>(resRef);
	if (im) {
		img = im->GetSprite2D();
	}
	return img;
}

FactoryObject* GameData::GetFactoryResource(const char* resname, SClass_ID type, bool silent)
{
	int fobjindex = factory->IsLoaded(ResRef(resname), type);
	// already cached
	if ( fobjindex != -1)
		return factory->GetFactoryObject( fobjindex );

	// empty resref
	if (!resname || !strcmp(resname, "")) return nullptr;

	switch (type) {
	case IE_BAM_CLASS_ID:
	{
		DataStream* str = GetResource(resname, type, silent);
		if (str) {
			auto importer = GetImporter<AnimationMgr>(IE_BAM_CLASS_ID, str);
			if (importer == nullptr) {
				return nullptr;
			}

			AnimationFactory* af = importer->GetAnimationFactory(resname);
			factory->AddFactoryObject( af );
			return af;
		}
		return NULL;
	}
	case IE_BMP_CLASS_ID:
	{
		ResourceHolder<ImageMgr> img = GetResourceHolder<ImageMgr>(resname, silent);
		if (img) {
			ImageFactory* fact = img->GetImageFactory( resname );
			factory->AddFactoryObject( fact );
			return fact;
		}

		return NULL;
	}
	default:
		Log(MESSAGE, "KEYImporter", "%s files are not supported.",
			core->TypeExt(type));
		return NULL;
	}
}

FactoryObject* GameData::GetFactoryResource(const ResRef& resname, SClass_ID type, bool silent)
{
	return GetFactoryResource(resname.CString(), type, silent);
}

void GameData::AddFactoryResource(FactoryObject* res)
{
	factory->AddFactoryObject(res);
}

Store* GameData::GetStore(const ResRef &resRef)
{
	StoreMap::iterator it = stores.find(resRef);
	if (it != stores.end()) {
		return it->second;
	}

	DataStream* str = gamedata->GetResource(resRef, IE_STO_CLASS_ID);
	PluginHolder<StoreMgr> sm = MakePluginHolder<StoreMgr>(IE_STO_CLASS_ID);
	if (sm == nullptr) {
		delete ( str );
		return NULL;
	}
	if (!sm->Open(str)) {
		return NULL;
	}

	Store* store = sm->GetStore(new Store());
	if (store == NULL) {
		return NULL;
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

	if (!str.Create(store->Name, IE_STO_CLASS_ID)) {
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

	int rowCount = itemsnd->GetRowCount();
	int colCount = itemsnd->GetColumnCount();
	for (int i = 0; i < rowCount; i++) {
		ItemSounds[i] = {};
		for (int j = 0; j < colCount; j++) {
			ResRef snd = MakeLowerCaseResRef(itemsnd->QueryField(i, j));
			if (snd == ResRef("*")) break;
			ItemSounds[i].push_back(snd);
		}
	}
}

bool GameData::GetItemSound(ResRef &Sound, ieDword ItemType, const char *ID, ieDword Col)
{
	Sound.Reset();

	if (ItemSounds.empty()) {
		ReadItemSounds();
	}

	if (Col >= ItemSounds[ItemType].size()) {
		return false;
	}

	if (ID && ID[1] == 'A') {
		//the last 4 item sounds are used for '1A', '2A', '3A' and '4A' (pst)
		//item animation types
		ItemType = ItemSounds.size()-4 + ID[0]-'1';
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

int GameData::GetRacialTHAC0Bonus(ieDword proficiency, const char *raceName)
{
	static bool loadedRacialTHAC0 = false;
	if (!loadedRacialTHAC0) {
		raceTHAC0Bonus = LoadTable("racethac", true);
		loadedRacialTHAC0 = true;
	}

	// not all games have the table
	if (!raceTHAC0Bonus || !raceName) return 0;

	char profString[5];
	snprintf(profString, sizeof(profString), "%u", proficiency);
	return atoi(raceTHAC0Bonus->QueryField(profString, raceName));
}

bool GameData::HasInfravision(const char *raceName)
{
	if (!racialInfravision) {
		racialInfravision = LoadTable("racefeat", true);
	}
	if (!raceName) return false;

	return atoi(racialInfravision->QueryField(raceName, "VALUE")) & 1;
}

int GameData::GetSpellAbilityDie(const Actor *target, int which)
{
	static bool loadedSpellAbilityDie = false;
	if (!loadedSpellAbilityDie) {
		spellAbilityDie = LoadTable("clssplab", true);
		if (!spellAbilityDie) {
			Log(ERROR, "GameData", "GetSpellAbilityDie failed loading clssplab.2da!");
			return 6;
		}
		loadedSpellAbilityDie = true;
	}

	ieDword cls = target->GetActiveClass();
	if (cls >= spellAbilityDie->GetRowCount()) cls = 0;
	return atoi(spellAbilityDie->QueryField(cls, which));
}

int GameData::GetTrapSaveBonus(ieDword level, int cls)
{
	if (!core->HasFeature(GF_3ED_RULES)) return 0;

	if (!trapSaveBonus) {
		trapSaveBonus = LoadTable("trapsave", true);
	}

	return atoi(trapSaveBonus->QueryField(level - 1, cls - 1));
}

int GameData::GetTrapLimit(Scriptable *trapper)
{
	if (!trapLimit) {
		trapLimit = LoadTable("traplimt", true);
	}

	if (trapper->Type != ST_ACTOR) {
		return 6; // not using table default, since EE's file has it at 0
	}

	const Actor *caster = (Actor *) trapper;
	ieDword kit = caster->GetStat(IE_KIT);
	const char *rowName;
	if (kit != 0x4000) { // KIT_BASECLASS
		rowName = caster->GetKitName(kit);
	} else {
		ieDword cls = caster->GetActiveClass();
		rowName = caster->GetClassName(cls);
	}

	return atoi(trapLimit->QueryField(rowName, "LIMIT"));
}

int GameData::GetSummoningLimit(ieDword sex)
{
	if (!summoningLimit) {
		summoningLimit = LoadTable("summlimt", true);
	}

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
	return atoi(summoningLimit->QueryField(row, 0));
}

const Color& GameData::GetColor(const char *row)
{
	// preload converted colors
	if (colors.empty()) {
		AutoTable colorTable = LoadTable("colors", true);
		for (size_t r = 0; r < colorTable->GetRowCount(); r++) {
			ieDword c = strtounsigned<ieDword>(colorTable->QueryField(r, 0));
			colors[colorTable->GetRowName(r)] = Color(c);
		}
	}
	const auto it = colors.find(row);
	if (it != colors.end()) {
		return it->second;
	}
	return ColorRed;
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

		int rows = bonusTable->GetRowCount();
		int cols = bonusTable->GetColumnCount();
		weaponStyleAPRBonusMax.h = rows;
		weaponStyleAPRBonusMax.w = cols;
		weaponStyleAPRBonus.resize(rows * cols);
		for (int i = 0; i < rows; i++) {
			for (int j = 0; j < cols; j++) {
				int tmp = atoi(bonusTable->QueryField(i, j));
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
		Log(ERROR, "GameData", "Cannot find %s.2da.", tableName.CString());
		return false;
	}

	size_t count = tm->GetRowCount();
	data.resize(count);
	for (size_t i = 0; i < count; i++) {
		data[i] = MakeLowerCaseResRef(tm->QueryField(i, 0));
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
	ieDword rowCount = tab->GetRowCount();
	spellProt.resize(rowCount);
	for (ieDword i = 0; i < rowCount; i++) {
		ieDword stat = core->TranslateStat(tab->QueryField(i, 0));
		spellProt[i].stat = (ieWord) stat;
		spellProt[i].value = strtounsigned<ieDword>(tab->QueryField(i, 1));
		spellProt[i].relation = strtounsigned<ieWord>(tab->QueryField(i, 2));
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

}
