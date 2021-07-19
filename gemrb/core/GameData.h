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
#include "iless.h"

#include "Cache.h"
#include "Holder.h"
#include "Resource.h"
#include "ResourceManager.h"
#include "TableMgr.h"

#include <map>
#include <unordered_map>
#include <vector>

namespace GemRB {

static const ResRef SevenEyes[7]={"spin126","spin127","spin128","spin129","spin130","spin131","spin132"};

class Actor;
struct Effect;
class Factory;
class FactoryObject;
class Item;
class Palette;
using PaletteHolder = Holder<Palette>;
class ScriptedAnimation;
class Spell;
class Sprite2D;
class Store;
class TableMgr;
class VEFObject;

struct Table {
	Holder<TableMgr> tm;
	ResRef resRef;
	unsigned int refcount;
};

class GEM_EXPORT GameData : public ResourceManager
{
public:
	GameData();
	~GameData();

	void ClearCaches();

	/** Returns actor */
	Actor *GetCreature(const char *ResRef, unsigned int PartySlot=0);
	/** Returns a PC index, by loading a creature */
	int LoadCreature(const char *ResRef, unsigned int PartySlot, bool character=false, int VersionOverride=-1);


	// 2DA table functions.
	// (See also the AutoTable class)

	/** Loads a 2DA Table, returns -1 on error or the Table Index on success */
	int LoadTable(const ResRef &ResRef, bool silent=false);
	/** Gets the index of a loaded table, returns -1 on error */
	int GetTableIndex(const ResRef& resRef) const;
	/** Gets a Loaded Table by its index, returns NULL on error */
	Holder<TableMgr> GetTable(size_t index) const;
	/** Frees a Loaded Table, returns false on error, true on success */
	bool DelTable(unsigned int index);

	PaletteHolder GetPalette(const ResRef& resname);
	void FreePalette(PaletteHolder &pal, const ResRef &name = ResRef());

	Item* GetItem(const ResRef &resname, bool silent=false);
	void FreeItem(Item const *itm, const ResRef &name, bool free=false);
	Spell* GetSpell(const ResRef &resname, bool silent=false);
	void FreeSpell(Spell *spl, const ResRef &name, bool free=false);
	Effect* GetEffect(const ResRef &resname);
	void FreeEffect(Effect *eff, const ResRef &name, bool free=false);

	/** creates a vvc/bam animation object at point */
	ScriptedAnimation* GetScriptedAnimation( const char *ResRef, bool doublehint);

	/** creates a composite vef/2da animation */
	VEFObject* GetVEFObject( const char *ResRef, bool doublehint);

	/** returns a single sprite (not cached) from a BAM resource */
	Holder<Sprite2D> GetBAMSprite(const ResRef &resRef, int cycle, int frame, bool silent=false);

	/* returns a single BAM or static image sprite, checking in that order */
	Holder<Sprite2D> GetAnySprite(const char *resRef, int cycle, int frame, bool silent = true);

	/** returns factory resource, currently works only with animations */
	FactoryObject* GetFactoryResource(const char* resname, SClass_ID type, bool silent=false);

	void AddFactoryResource(FactoryObject* res);

	Store* GetStore(const ResRef &resRef);
	/// Saves a store to the cache and frees it.
	void SaveStore(Store* store);
	/// Saves all stores in the cache
	void SaveAllStores();

	// itemsnd.2da functions
	bool GetItemSound(ResRef &Sound, ieDword ItemType, const char *ID, ieDword Col);
	int GetSwingCount(ieDword ItemType);

	int GetRacialTHAC0Bonus(ieDword proficiency, const char *raceName);
	bool HasInfravision(const char *raceName);
	int GetSpellAbilityDie(const Actor *target, int which);
	int GetTrapSaveBonus(ieDword level, int cls);
	int GetTrapLimit(Scriptable *trapper);
	int GetSummoningLimit(ieDword sex);
	const Color& GetColor(const char *row);
	int GetWeaponStyleAPRBonus(int row, int col);
	bool ReadResRefTable(const ResRef& tableName, std::vector<ResRef>& data) const;
	inline int GetStepTime() const { return stepTime; }
	inline void SetStepTime(int st) { stepTime = st; }
	inline int GetTextSpeed() const { return TextScreenSpeed; }
	inline void SetTextSpeed(int speed) { TextScreenSpeed = speed; }
private:
	void ReadItemSounds();
private:
	Cache ItemCache;
	Cache SpellCache;
	Cache EffectCache;
	std::unordered_map<ResRef, PaletteHolder, ResRef::Hash> PaletteCache;
	Factory* factory;
	std::vector<Table> tables;
	typedef std::map<const char*, Store*, iless> StoreMap;
	StoreMap stores;
	std::map<ieDword, std::vector<const char*> > ItemSounds;
	AutoTable racialInfravision;
	AutoTable raceTHAC0Bonus;
	AutoTable spellAbilityDie;
	AutoTable trapSaveBonus;
	AutoTable trapLimit;
	AutoTable summoningLimit;
	std::vector<int> weaponStyleAPRBonus;
	std::map<const char*, Color, iless> colors;
	int stepTime = 0;
	int TextScreenSpeed = 0;
	Size weaponStyleAPRBonusMax{};

public:
	std::vector<ResRef> defaultSounds;
	std::vector<ResRef> castingGlows;
	std::vector<int> castingSounds;
	std::vector<ResRef> spellHits;
};

extern GEM_EXPORT GameData * gamedata;

template <class T>
using ResourceHolder = Holder<T>;

template <class T>
inline ResourceHolder<T> GetResourceHolder(const char* resname, bool silent = false, bool useCorrupt = false)
{
	return Holder<T>(static_cast<T*>(gamedata->GetResource(resname, &T::ID, silent, useCorrupt)));
}

template <class T>
inline ResourceHolder<T> GetResourceHolder(const char* resname, const ResourceManager& manager, bool silent = false)
{
	return Holder<T>(static_cast<T*>(manager.GetResource(resname,&T::ID,silent)));
}

template <class T>
inline ResourceHolder<T> GetResourceHolder(const ResRef &resref, bool silent = false, bool useCorrupt = false)
{
	return Holder<T>(static_cast<T*>(gamedata->GetResource(resref.CString(), &T::ID, silent, useCorrupt)));
}

template <class T>
inline ResourceHolder<T> GetResourceHolder(const ResRef &resref, const ResourceManager& manager, bool silent = false)
{
	return Holder<T>(static_cast<T*>(manager.GetResource(resref.CString(), &T::ID,silent)));
}

}

#endif
