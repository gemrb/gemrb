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

#ifndef GAMEDATA_H
#define GAMEDATA_H

#include "../../includes/SClassID.h"
#include "../../includes/ie_types.h"
#include "Cache.h"

class TableMgr;
class Palette;
class Item;
class Spell;
struct Effect;
class ScriptedAnimation;
class Factory;
class Actor;
class Sprite2D;
class Resource;
class TypeID;

struct Table {
	TableMgr * tm;
	char ResRef[8];
	unsigned int refcount;
};

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT GameData
{
public:
	GameData();
	~GameData();

	void ClearCaches();

	Factory* GetFactory() const { return factory; }

	/** Returns actor */
	Actor *GetCreature(const char *ResRef, unsigned int PartySlot=0);
	/** Returns a PC index, by loading a creature */
	int LoadCreature(const char *ResRef, unsigned int PartySlot, bool character=false);


	// 2DA table functions.
	// (See also the AutoTable class)

	/** Loads a 2DA Table, returns -1 on error or the Table Index on success */
	int LoadTable(const char * ResRef);
	/** Gets the index of a loaded table, returns -1 on error */
	int GetTableIndex(const char * ResRef) const;
	/** Gets a Loaded Table by its index, returns NULL on error */
	TableMgr * GetTable(unsigned int index) const;
	/** Frees a Loaded Table, returns false on error, true on success */
	bool DelTable(unsigned int index);

	/** returns true if resource exists */
	bool Exists(const char *ResRef, SClass_ID type, bool silent=false);

	Resource* GetResource(const char* resname, const TypeID *type, bool silent = false) const;

	Palette* GetPalette(const ieResRef resname);
	void FreePalette(Palette *&pal, const ieResRef name=NULL);
	
	Item* GetItem(const ieResRef resname);
	void FreeItem(Item const *itm, const ieResRef name, bool free=false);
	Spell* GetSpell(const ieResRef resname, bool silent=false);
	void FreeSpell(Spell *spl, const ieResRef name, bool free=false);
	Effect* GetEffect(const ieResRef resname);
	void FreeEffect(Effect *eff, const ieResRef name, bool free=false);

	/** creates a vvc/bam animation object at point */
	ScriptedAnimation* GetScriptedAnimation( const char *ResRef, bool doublehint);

	/** returns a single sprite (not cached) from a BAM resource */
	Sprite2D* GetBAMSprite(const ieResRef ResRef, int cycle, int frame);

private:
	Cache ItemCache;
	Cache SpellCache;
	Cache EffectCache;
	Cache PaletteCache;
	Factory* factory;
	std::vector<Table> tables;
};


#endif
