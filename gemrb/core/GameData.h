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

#ifndef GAMEDATA_H
#define GAMEDATA_H

#include "SClassID.h"
#include "exports.h"
#include "ie_types.h"

#include "Cache.h"
#include "CharAnimations.h"
#include "DisplayMessage.h"
#include "Effect.h"
#include "Factory.h"
#include "Holder.h"
#include "Item.h"
#include "Palette.h"
#include "Resource.h"
#include "ResourceManager.h"
#include "Spell.h"
#include "SrcMgr.h"
#include "TableMgr.h"

#include "Scriptable/Actor.h"

#include <map>
#include <unordered_map>
#include <vector>

namespace GemRB {

static const ResRef SevenEyes[7] = { "spin126", "spin127", "spin128", "spin129", "spin130", "spin131", "spin132" };

class Actor;
class ScriptedAnimation;
class Sprite2D;
class Store;
class VEFObject;

struct IWDIDSEntry {
	ieDword value;
	ieWord stat = USHRT_MAX;
	ieWord relation;
};

struct SpecialSpellType {
	ResRef resref;
	int flags;
	int amount;
	int bonus_limit;
};
enum SpecialSpell : int8_t {
	Identify = 1, // any spell that cannot be cast from the menu
	Silence = 2, // any spell that can be cast in silence
	Surge = 4, // any spell that cannot be cast during a wild surge
	Rest = 8, // any spell that is cast upon rest if memorized
	HealAll = 16 // any healing spell that is cast upon rest at more than one target (healing circle, mass cure)
};

struct SurgeSpell {
	ResRef spell;
	ieStrRef message;
};

// item usability array
struct ItemUseType {
	ResRef table; // which table contains the stat usability flags
	ieByte stat; // which actor stat we talk about
	ieByte mcol; // which column should be matched against the stat
	ieByte vcol; // which column has the bit value for it
	ieByte which; // which item dword should be used (1 = kit)
};

class GEM_EXPORT GameData : public ResourceManager {
public:
	GameData() = default;
	GameData(const GameData&) = delete;
	GameData& operator=(const GameData&) = delete;
	~GameData();

	using index_t = uint16_t;

	/** Returns actor */
	Actor* GetCreature(const ResRef& creature, unsigned int PartySlot = 0);
	/** Returns a PC index, by loading a creature */
	int LoadCreature(const ResRef& creature, unsigned int PartySlot, bool character = false, int VersionOverride = -1);


	// 2DA table functions.
	// (See also the AutoTable class)

	/** Loads a 2DA Table, returns -1 on error or the Table Index on success */
	AutoTable LoadTable(const ResRef& tableRef, bool silent = false);

	Holder<Palette> GetPalette(const ResRef& resname);

	Item* GetItem(const ResRef& resname, bool silent = false);
	void FreeItem(Item const* itm, const ResRef& name, bool free = false);
	Spell* GetSpell(const ResRef& resname, bool silent = false);
	void FreeSpell(const Spell* spl, const ResRef& name, bool free = false);
	Effect* GetEffect(const ResRef& resname);
	void FreeEffect(const Effect* eff, const ResRef& name, bool free = false);

	/** creates a vvc/bam animation object at point */
	ScriptedAnimation* GetScriptedAnimation(const ResRef& resRef, bool doublehint);

	/** creates a composite vef/2da animation */
	VEFObject* GetVEFObject(const ResRef& vefRef, bool doublehint);

	/** returns a single sprite (not cached) from a BAM resource */
	Holder<Sprite2D> GetBAMSprite(const ResRef& resRef, int cycle, int frame, bool silent = false);

	/* returns a single BAM or static image sprite, checking in that order */
	Holder<Sprite2D> GetAnySprite(const ResRef& resRef, int cycle, int frame, bool silent = true);

	/** returns factory resource, currently works only with animations */
	Factory::object_t GetFactoryResource(const ResRef& resName, SClass_ID type, bool silent = false);

	template<typename T>
	std::shared_ptr<T> GetFactoryResourceAs(const ResRef& resName, SClass_ID type, bool silent = false)
	{
		static_assert(std::is_base_of<FactoryObject, T>::value, "T must be a FactoryObject.");
		return std::static_pointer_cast<T>(GetFactoryResource(resName, type, silent));
	}

	template<typename T, typename... ARGS>
	std::shared_ptr<T> AddFactoryResource(ARGS&&... args)
	{
		static_assert(std::is_base_of<FactoryObject, T>::value, "T must be a FactoryObject.");
		auto obj = std::make_shared<T>(std::forward<ARGS>(args)...);
		factory.AddFactoryObject(obj);
		return obj;
	}

	Store* GetStore(const ResRef& resRef);
	/// Saves a store to the cache and frees it.
	void SaveStore(Store* store);
	/// Saves all stores in the cache
	void SaveAllStores();

	// itemsnd.2da functions
	bool GetItemSound(ResRef& Sound, ieDword ItemType, AnimRef ID, ieDword Col);
	int GetSwingCount(ieDword ItemType);

	int GetRacialTHAC0Bonus(ieDword proficiency, const std::string& raceName);
	bool HasInfravision(const std::string& raceName);
	int GetSpellAbilityDie(const Actor* target, int which);
	int GetTrapSaveBonus(ieDword level, int cls);
	int GetTrapLimit(Scriptable* trapper);
	int GetSummoningLimit(ieDword sex);
	void PreloadColors();
	const Color& GetColor(const TableMgr::key_t& row) const;
	void ModifyColor(GUIColors idx, const Color& newColor);
	int GetWeaponStyleAPRBonus(int row, int col);
	int GetReputationMod(int column);
	/** Returns the virtual worldmap entry of a sub-area (pst-only) */
	int GetAreaAlias(const ResRef& areaName);
	int GetSpecialSpell(const ResRef& resref);
	const std::vector<SpecialSpellType>& GetSpecialSpells() const { return SpecialSpells; }
	int CheckSpecialSpell(const ResRef& resRef, const Actor* actor);
	const SurgeSpell& GetSurgeSpell(unsigned int idx);
	bool ReadResRefTable(const ResRef& tableName, std::vector<ResRef>& data);
	const IWDIDSEntry& GetSpellProt(index_t idx);
	ResRef GetFist(int cls, int level);
	int GetMonkBonus(int bonusType, int level);
	int GetWeaponStyleBonus(int style, int stars, int bonusType);
	int GetWSpecialBonus(int bonusType, int stars);
	const std::vector<int>& GetBonusSpells(int ability);
	ieByte GetItemAnimation(const ResRef& itemRef);
	const std::vector<ItemUseType>& GetItemUse();
	int GetMiscRule(const TableMgr::key_t& rowName);
	int GetDifficultyMod(ieDword mod, Difficulty difficulty);
	int GetXPBonus(ieDword bonusType, ieDword level);
	int GetVBData(const TableMgr::key_t& rowName);

	inline int GetStepTime() const { return stepTime; }
	inline void SetStepTime(int st) { stepTime = st; }
	inline int GetTextSpeed() const { return TextScreenSpeed; }
	inline void SetTextSpeed(int speed) { TextScreenSpeed = speed; }

private:
	void ReadItemSounds();
	void ReadSpellProtTable();

private:
	ResRefRCCache<Item> ItemCache;
	ResRefRCCache<Spell> SpellCache;
	ResRefRCCache<Effect> EffectCache;
	ResRefMap<Holder<Palette>> PaletteCache;
	Factory factory;
	ResRefMap<AutoTable> tables;
	using StoreMap = ResRefMap<Store*>;
	StoreMap stores;
	std::map<size_t, std::vector<ResRef>> ItemSounds;
	ResRefMap<ieDword> AreaAliasTable;
	std::vector<int> weaponStyleAPRBonus;
	std::map<std::string, Color> colors;
	std::vector<IWDIDSEntry> spellProt;
	std::vector<SpecialSpellType> SpecialSpells;
	std::vector<SurgeSpell> SurgeSpells;
	std::vector<std::vector<int>> bonusSpells;
	int stepTime = 0;
	int TextScreenSpeed = 0;
	Size weaponStyleAPRBonusMax {};
	// 4 styles and 4 star levels, 7 bonus types
	std::array<std::array<std::array<int, 7>, 4>, 4> weaponStyleBoni {};
	ResRefMap<ieByte> itemAnims;
	std::vector<ItemUseType> itemUse;

public:
	std::vector<ResRef> defaultSounds;
	std::vector<ResRef> castingGlows;
	std::vector<ResRef> castingHits;
	std::vector<int> castingSounds;
	std::vector<ResRef> spellHits;
	SrcMgr SrcManager;
};

extern GEM_EXPORT GameData* gamedata;

}

#endif
