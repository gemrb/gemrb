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
#include "Scriptable/Actor.h"
#include "System/FileStream.h"

#include <cstdio>

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

static void ReleasePalette(void *poi)
{
	//we allow nulls, but we shouldn't release them
	if (!poi) return;
	//as long as palette has its own refcount, this should be Release
	((Palette *) poi)->Release();
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
	PaletteCache.RemoveAll(ReleasePalette);

	while (!stores.empty()) {
		Store *store = stores.begin()->second;
		stores.erase(stores.begin());
		delete store;
	}
}

Actor *GameData::GetCreature(const char* ResRef, unsigned int PartySlot)
{
	DataStream* ds = GetResource( ResRef, IE_CRE_CLASS_ID );
	if (!ds)
		return 0;

	PluginHolder<ActorMgr> actormgr(IE_CRE_CLASS_ID);
	if (!actormgr->Open(ds)) {
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
		PathJoin( nPath, core->GamePath, "characters", fName, NULL );
		stream = FileStream::OpenFile(nPath);
		PluginHolder<ActorMgr> actormgr(IE_CRE_CLASS_ID);
		if (!actormgr->Open(stream)) {
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
	memcpy(actor->Area, core->GetGame()->CurrentArea, sizeof(actor->Area) );
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
int GameData::LoadTable(const ieResRef ResRef, bool silent)
{
	int ind = GetTableIndex( ResRef );
	if (ind != -1) {
		tables[ind].refcount++;
		return ind;
	}
	//print("(%s) Table not found... Loading from file", ResRef);
	DataStream* str = GetResource( ResRef, IE_2DA_CLASS_ID, silent );
	if (!str) {
		return -1;
	}
	PluginHolder<TableMgr> tm(IE_2DA_CLASS_ID);
	if (!tm) {
		delete str;
		return -1;
	}
	if (!tm->Open(str)) {
		return -1;
	}
	Table t;
	t.refcount = 1;
	strncpy( t.ResRef, ResRef, 8 );
	t.tm = tm;
	ind = -1;
	for (size_t i = 0; i < tables.size(); i++) {
		if (tables[i].refcount == 0) {
			ind = ( int ) i;
			break;
		}
	}
	if (ind != -1) {
		tables[ind] = t;
		return ind;
	}
	tables.push_back( t );
	return ( int ) tables.size() - 1;
}
/** Gets the index of a loaded table, returns -1 on error */
int GameData::GetTableIndex(const char* ResRef) const
{
	for (size_t i = 0; i < tables.size(); i++) {
		if (tables[i].refcount == 0)
			continue;
		if (strnicmp( tables[i].ResRef, ResRef, 8 ) == 0)
			return ( int ) i;
	}
	return -1;
}
/** Gets a Loaded Table by its index, returns NULL on error */
Holder<TableMgr> GameData::GetTable(unsigned int index) const
{
	if (index >= tables.size()) {
		return NULL;
	}
	if (tables[index].refcount == 0) {
		return NULL;
	}
	return tables[index].tm;
}

/** Frees a Loaded Table, returns false on error, true on success */
bool GameData::DelTable(unsigned int index)
{
	if (index==0xffffffff) {
		tables.clear();
		return true;
	}
	if (index >= tables.size()) {
		return false;
	}
	if (tables[index].refcount == 0) {
		return false;
	}
	tables[index].refcount--;
	if (tables[index].refcount == 0)
		if (tables[index].tm)
			tables[index].tm.release();
	return true;
}

Palette *GameData::GetPalette(const ieResRef resname)
{
	Palette *palette = (Palette *) PaletteCache.GetResource(resname);
	if (palette) {
		return palette;
	}
	//additional hack for allowing NULL's
	if (PaletteCache.RefCount(resname)!=-1) {
		return NULL;
	}
	ResourceHolder<ImageMgr> im(resname);
	if (im == NULL) {
		PaletteCache.SetAt(resname, NULL);
		return NULL;
	}

	palette = new Palette();
	im->GetPalette(256,palette->col);
	palette->named=true;
	PaletteCache.SetAt(resname, (void *) palette);
	return palette;
}

void GameData::FreePalette(Palette *&pal, const ieResRef name)
{
	int res;

	if (!pal) {
		return;
	}
	if (!name || !name[0]) {
		if(pal->named) {
			error("GameData", "Palette is supposed to be named, but got no name!\n");
		} else {
			pal->Release();
			pal=NULL;
		}
		return;
	}
	if (!pal->named) {
		error("GameData", "Unnamed palette, it should be %s!\n", name);
	}
	res=PaletteCache.DecRef((void *) pal, name, true);
	if (res<0) {
		error("Core", "Corrupted Palette cache encountered (reference count went below zero), Palette name is: %.8s\n", name);
	}
	if (!res) {
		pal->Release();
	}
	pal = NULL;
}

Item* GameData::GetItem(const ieResRef resname)
{
	Item *item = (Item *) ItemCache.GetResource(resname);
	if (item) {
		return item;
	}
	DataStream* str = GetResource( resname, IE_ITM_CLASS_ID );
	PluginHolder<ItemMgr> sm(IE_ITM_CLASS_ID);
	if (!sm) {
		delete ( str );
		return NULL;
	}
	if (!sm->Open(str)) {
		return NULL;
	}

	item = new Item();
	//this is required for storing the 'source'
	strnlwrcpy(item->Name, resname, 8);
	sm->GetItem( item );
	if (item == NULL) {
		return NULL;
	}

	ItemCache.SetAt(resname, (void *) item);
	return item;
}

//you can supply name for faster access
void GameData::FreeItem(Item const *itm, const ieResRef name, bool free)
{
	int res;

	res=ItemCache.DecRef((void *) itm, name, free);
	if (res<0) {
		error("Core", "Corrupted Item cache encountered (reference count went below zero), Item name is: %.8s\n", name);
	}
	if (res) return;
	if (free) delete itm;
}

Spell* GameData::GetSpell(const ieResRef resname, bool silent)
{
	Spell *spell = (Spell *) SpellCache.GetResource(resname);
	if (spell) {
		return spell;
	}
	DataStream* str = GetResource( resname, IE_SPL_CLASS_ID, silent );
	PluginHolder<SpellMgr> sm(IE_SPL_CLASS_ID);
	if (!sm) {
		delete ( str );
		return NULL;
	}
	if (!sm->Open(str)) {
		return NULL;
	}

	spell = new Spell();
	//this is required for storing the 'source'
	strnlwrcpy(spell->Name, resname, 8);
	sm->GetSpell( spell, silent );
	if (spell == NULL) {
		return NULL;
	}

	SpellCache.SetAt(resname, (void *) spell);
	return spell;
}

void GameData::FreeSpell(Spell *spl, const ieResRef name, bool free)
{
	int res;

	res=SpellCache.DecRef((void *) spl, name, free);
	if (res<0) {
		error("Core", "Corrupted Spell cache encountered (reference count went below zero), Spell name is: %.8s or %.8s\n",
			name, spl->Name);
	}
	if (res) return;
	if (free) delete spl;
}

Effect* GameData::GetEffect(const ieResRef resname)
{
	Effect *effect = (Effect *) EffectCache.GetResource(resname);
	if (effect) {
		return effect;
	}
	DataStream* str = GetResource( resname, IE_EFF_CLASS_ID );
	PluginHolder<EffectMgr> em(IE_EFF_CLASS_ID);
	if (!em) {
		delete ( str );
		return NULL;
	}
	if (!em->Open(str)) {
		return NULL;
	}

	effect = em->GetEffect(new Effect() );
	if (effect == NULL) {
		return NULL;
	}

	EffectCache.SetAt(resname, (void *) effect);
	return effect;
}

void GameData::FreeEffect(Effect *eff, const ieResRef name, bool free)
{
	int res;

	res=EffectCache.DecRef((void *) eff, name, free);
	if (res<0) {
		error("Core", "Corrupted Effect cache encountered (reference count went below zero), Effect name is: %.8s\n", name);
	}
	if (res) return;
	if (free) delete eff;
}

//if the default setup doesn't fit for an animation
//create a vvc for it!
ScriptedAnimation* GameData::GetScriptedAnimation( const char *effect, bool doublehint)
{
	ScriptedAnimation *ret = NULL;

	if (Exists( effect, IE_VVC_CLASS_ID, true ) ) {
		DataStream *ds = GetResource( effect, IE_VVC_CLASS_ID );
		ret = new ScriptedAnimation(ds);
	} else {
		AnimationFactory *af = (AnimationFactory *)
			GetFactoryResource( effect, IE_BAM_CLASS_ID, IE_NORMAL );
		if (af) {
			ret = new ScriptedAnimation();
			ret->LoadAnimationFactory( af, doublehint?2:0);
		}
	}
	if (ret) {
		strnlwrcpy(ret->ResName, effect, 8);
	}
	return ret;
}

// Return single BAM frame as a sprite. Use if you want one frame only,
// otherwise it's not efficient
Sprite2D* GameData::GetBAMSprite(const ieResRef ResRef, int cycle, int frame)
{
	Sprite2D *tspr;
	AnimationFactory* af = ( AnimationFactory* )
		GetFactoryResource( ResRef, IE_BAM_CLASS_ID, IE_NORMAL );
	if (!af) return 0;
	if (cycle == -1)
		tspr = af->GetFrameWithoutCycle( (unsigned short) frame );
	else
		tspr = af->GetFrame( (unsigned short) frame, (unsigned char) cycle );
	return tspr;
}

void* GameData::GetFactoryResource(const char* resname, SClass_ID type,
	unsigned char mode, bool silent)
{
	int fobjindex = factory->IsLoaded(resname,type);
	// already cached
	if ( fobjindex != -1)
		return factory->GetFactoryObject( fobjindex );

	// empty resref
	if (!strcmp(resname, ""))
		return NULL;

	switch (type) {
	case IE_BAM_CLASS_ID:
	{
		DataStream* ret = GetResource( resname, type, silent );
		if (ret) {
			PluginHolder<AnimationMgr> ani(IE_BAM_CLASS_ID);
			if (!ani)
				return NULL;
			if (!ani->Open(ret))
				return NULL;
			AnimationFactory* af = ani->GetAnimationFactory( resname, mode );
			factory->AddFactoryObject( af );
			return af;
		}
		return NULL;
	}
	case IE_BMP_CLASS_ID:
	{
		ResourceHolder<ImageMgr> img(resname);
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

Store* GameData::GetStore(const ieResRef ResRef)
{
	StoreMap::iterator it = stores.find(ResRef);
	if (it != stores.end()) {
		return it->second;
	}

	DataStream* str = gamedata->GetResource(ResRef, IE_STO_CLASS_ID);
	PluginHolder<StoreMgr> sm(IE_STO_CLASS_ID);
	if (sm == NULL) {
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
	strnlwrcpy(store->Name, ResRef, 8);
	// The key needs to last as long as the store,
	// so use the one we just copied.
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

	PluginHolder<StoreMgr> sm(IE_STO_CLASS_ID);
	if (sm == NULL) {
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
