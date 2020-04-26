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
#include "ResourceManager.h"
#include "TableMgr.h"

#include <map>
#include <vector>

namespace GemRB {

static const ieResRef SevenEyes[7]={"spin126","spin127","spin128","spin129","spin130","spin131","spin132"};

class Actor;
struct Effect;
class Factory;
class Item;
class Palette;
class ScriptedAnimation;
class Spell;
class Sprite2D;
class Store;
class TableMgr;
class VEFObject;

struct Table {
	Holder<TableMgr> tm;
	ieResRef ResRef;
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
	int LoadTable(const ieResRef ResRef, bool silent=false);
	/** Gets the index of a loaded table, returns -1 on error */
	int GetTableIndex(const char * ResRef) const;
	/** Gets a Loaded Table by its index, returns NULL on error */
	Holder<TableMgr> GetTable(unsigned int index) const;
	/** Frees a Loaded Table, returns false on error, true on success */
	bool DelTable(unsigned int index);

	Palette* GetPalette(const ieResRef resname);
	void FreePalette(Palette *&pal, const ieResRef name=NULL);
	
	Item* GetItem(const ieResRef resname, bool silent=false);
	void FreeItem(Item const *itm, const ieResRef name, bool free=false);
	Spell* GetSpell(const ieResRef resname, bool silent=false);
	void FreeSpell(Spell *spl, const ieResRef name, bool free=false);
	Effect* GetEffect(const ieResRef resname);
	void FreeEffect(Effect *eff, const ieResRef name, bool free=false);

	/** creates a vvc/bam animation object at point */
	ScriptedAnimation* GetScriptedAnimation( const char *ResRef, bool doublehint);

	/** creates a composite vef/2da animation */
	VEFObject* GetVEFObject( const char *ResRef, bool doublehint);

	/** returns a single sprite (not cached) from a BAM resource */
	Sprite2D* GetBAMSprite(const ieResRef ResRef, int cycle, int frame, bool silent=false);

	/** returns factory resource, currently works only with animations */
	void* GetFactoryResource(const char* resname, SClass_ID type,
		unsigned char mode = IE_NORMAL, bool silent=false);

	Store* GetStore(const ieResRef ResRef);
	/// Saves a store to the cache and frees it.
	void SaveStore(Store* store);
	/// Saves all stores in the cache
	void SaveAllStores();

	// itemsnd.2da functions
	bool GetItemSound(ieResRef &Sound, ieDword ItemType, const char *ID, ieDword Col);
	int GetSwingCount(ieDword ItemType);

	int GetRacialTHAC0Bonus(ieDword proficiency, const char *raceName);
private:
	void ReadItemSounds();
private:
	Cache ItemCache;
	Cache SpellCache;
	Cache EffectCache;
	Cache PaletteCache;
	Factory* factory;
	std::vector<Table> tables;
	typedef std::map<const char*, Store*, iless> StoreMap;
	StoreMap stores;
	std::map<ieDword, std::vector<const char*> > ItemSounds;
	AutoTable raceTHAC0Bonus;
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

}

#endif
