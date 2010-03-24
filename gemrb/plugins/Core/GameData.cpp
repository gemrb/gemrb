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
* $Id$
*
*/

#include <cstdio>
#include "GameData.h"

#include "Actor.h"
#include "Cache.h"
#include "Factory.h"
#include "Item.h"
#include "Spell.h"
#include "Effect.h"
#include "ActorMgr.h"
#include "FileStream.h"
#include "Interface.h"
#include "SpellMgr.h"
#include "ItemMgr.h"
#include "EffectMgr.h"
#include "ResourceMgr.h"
#include "Game.h"
#include "ResourceDesc.h"
#include "AnimationMgr.h"
#include "ImageMgr.h"
#include "ImageFactory.h"

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

// TODO: this is duplicated from Interface.cpp
#define FreeInterfaceVector(type, variable, member) \
{ \
	std::vector<type>::iterator i; \
	for(i = (variable).begin(); i != (variable).end(); ++i) { \
	if ((*i).refcount) { \
		core->FreeInterface((*i).member); \
		(*i).refcount = 0; \
	} \
	} \
}

GameData::GameData()
{
	factory = new Factory();
}

GameData::~GameData()
{
	FreeInterfaceVector( Table, tables, tm );
	delete factory;
}

void GameData::ClearCaches()
{
	ItemCache.RemoveAll(ReleaseItem);
	SpellCache.RemoveAll(ReleaseSpell);
	EffectCache.RemoveAll(ReleaseEffect);
	PaletteCache.RemoveAll(ReleasePalette);
}

Actor *GameData::GetCreature(const char* ResRef, unsigned int PartySlot)
{
	DataStream* ds = core->GetResourceMgr()->GetResource( ResRef, IE_CRE_CLASS_ID );
	if (!ds)
		return 0;

	ActorMgr* actormgr = ( ActorMgr* ) core->GetInterface( IE_CRE_CLASS_ID );
	if (!actormgr->Open( ds, true )) {
		core->FreeInterface( actormgr );
		return 0;
	}
	Actor* actor = actormgr->GetActor(PartySlot);
	core->FreeInterface( actormgr );
	return actor;
}

int GameData::LoadCreature(const char* ResRef, unsigned int PartySlot, bool character)
{
	DataStream *stream;

	Actor* actor;
	if (character) {
		char nPath[_MAX_PATH], fName[16];
		snprintf( fName, sizeof(fName), "%s.chr", ResRef);
		PathJoin( nPath, core->GamePath, "characters", fName, NULL );
		ResolveFilePath( nPath );
		FileStream *fs = new FileStream();
		fs -> Open( nPath, true );
		stream = (DataStream *) fs;
		ActorMgr* actormgr = ( ActorMgr* ) core->GetInterface( IE_CRE_CLASS_ID );
		if (!actormgr->Open( stream, true )) {
			core->FreeInterface( actormgr );
			return -1;
		}
		actor = actormgr->GetActor(PartySlot);
		core->FreeInterface( actormgr );
	} else {
		actor = GetCreature(ResRef, PartySlot);
	}

	if ( !actor ) {
		return -1;
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
int GameData::LoadTable(const ieResRef ResRef)
{
	int ind = GetTableIndex( ResRef );
	if (ind != -1) {
		tables[ind].refcount++;
		return ind;
	}
	//printf("(%s) Table not found... Loading from file\n", ResRef);
	DataStream* str = core->GetResourceMgr()->GetResource( ResRef, IE_2DA_CLASS_ID );
	if (!str) {
		return -1;
	}
	TableMgr* tm = ( TableMgr* ) core->GetInterface( IE_2DA_CLASS_ID );
	if (!tm) {
		delete str;
		return -1;
	}
	if (!tm->Open( str, true )) {
		core->FreeInterface( tm );
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
TableMgr* GameData::GetTable(unsigned int index) const
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
		FreeInterfaceVector( Table, tables, tm );
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
		core->FreeInterface( tables[index].tm );
	return true;
}
bool GameData::Exists(const char *ResRef, SClass_ID type, bool silent)
{
	// TODO: check various caches before going to KeyImp
	return core->GetResourceMgr()->HasResource( ResRef, type, silent );
}

Resource* GameData::GetResource(const char* resname, const TypeID *type, bool silent) const
{
	return core->GetResourceMgr()->GetResource(resname, core->GetPluginMgr()->GetResourceDesc(type) , silent);
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
	ImageMgr* im = ( ImageMgr* )
		GetResource( resname, &ImageMgr::ID );
	if (im == NULL) {
		PaletteCache.SetAt(resname, NULL);
		core->FreeInterface( im );
		return NULL;
	}

	palette = new Palette();
	im->GetPalette(0,256,palette->col);
	core->FreeInterface( im );
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
			printf("Palette is supposed to be named, but got no name!\n");
			abort();
		} else {
			pal->Release();
			pal=NULL;
		}
		return;
	}
	if (!pal->named) {
		printf("Unnamed palette, it should be %s!\n", name);
		abort();
	}
	res=PaletteCache.DecRef((void *) pal, name, true);
	if (res<0) {
		printMessage( "Core", "Corrupted Palette cache encountered (reference count went below zero), ", LIGHT_RED );
		printf( "Palette name is: %.8s\n", name);
		abort();
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
	DataStream* str = core->GetResourceMgr()->GetResource( resname, IE_ITM_CLASS_ID );
	ItemMgr* sm = ( ItemMgr* ) core->GetInterface( IE_ITM_CLASS_ID );
	if (sm == NULL) {
		delete ( str );
		return NULL;
	}
	if (!sm->Open( str, true )) {
		core->FreeInterface( sm );
		return NULL;
	}

	item = new Item();
	//this is required for storing the 'source'
	strnlwrcpy(item->Name, resname, 8);
	sm->GetItem( item );
	if (item == NULL) {
		core->FreeInterface( sm );
		return NULL;
	}

	core->FreeInterface( sm );
	ItemCache.SetAt(resname, (void *) item);
	return item;
}

//you can supply name for faster access
void GameData::FreeItem(Item const *itm, const ieResRef name, bool free)
{
	int res;

	res=ItemCache.DecRef((void *) itm, name, free);
	if (res<0) {
		printMessage( "Core", "Corrupted Item cache encountered (reference count went below zero), ", LIGHT_RED );
		printf( "Item name is: %.8s\n", name);
		abort();
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
	DataStream* str = core->GetResourceMgr()->GetResource( resname, IE_SPL_CLASS_ID, silent );
	SpellMgr* sm = ( SpellMgr* ) core->GetInterface( IE_SPL_CLASS_ID );
	if (sm == NULL) {
		delete ( str );
		return NULL;
	}
	if (!sm->Open( str, true )) {
		core->FreeInterface( sm );
		return NULL;
	}

	spell = new Spell();
	//this is required for storing the 'source'
	strnlwrcpy(spell->Name, resname, 8);
	sm->GetSpell( spell, silent );
	if (spell == NULL) {
		core->FreeInterface( sm );
		return NULL;
	}

	core->FreeInterface( sm );

	SpellCache.SetAt(resname, (void *) spell);
	return spell;
}

void GameData::FreeSpell(Spell *spl, const ieResRef name, bool free)
{
	int res;

	res=SpellCache.DecRef((void *) spl, name, free);
	if (res<0) {
		printMessage( "Core", "Corrupted Spell cache encountered (reference count went below zero), ", LIGHT_RED );
		printf( "Spell name is: %.8s or %.8s\n", name, spl->Name);
		abort();
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
	DataStream* str = core->GetResourceMgr()->GetResource( resname, IE_EFF_CLASS_ID );
	EffectMgr* em = ( EffectMgr* ) core->GetInterface( IE_EFF_CLASS_ID );
	if (em == NULL) {
		delete ( str );
		return NULL;
	}
	if (!em->Open( str, true )) {
		core->FreeInterface( em );
		return NULL;
	}

	effect = em->GetEffect(new Effect() );
	if (effect == NULL) {
		core->FreeInterface( em );
		return NULL;
	}

	core->FreeInterface( em );

	EffectCache.SetAt(resname, (void *) effect);
	return effect;
}

void GameData::FreeEffect(Effect *eff, const ieResRef name, bool free)
{
	int res;

	res=EffectCache.DecRef((void *) eff, name, free);
	if (res<0) {
		printMessage( "Core", "Corrupted Effect cache encountered (reference count went below zero), ", LIGHT_RED );
		printf( "Effect name is: %.8s\n", name);
		abort();
	}
	if (res) return;
	if (free) delete eff;
}

//if the default setup doesn't fit for an animation
//create a vvc for it!
ScriptedAnimation* GameData::GetScriptedAnimation( const char *effect, bool doublehint)
{
	ScriptedAnimation *ret = NULL;

	if (Exists( effect, IE_VVC_CLASS_ID ) ) {
		DataStream *ds = core->GetResourceMgr()->GetResource( effect, IE_VVC_CLASS_ID );
		ret = new ScriptedAnimation(ds, true);
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
		DataStream* ret = core->GetResourceMgr()->GetResource( resname, type, silent );
		if (ret) {
			AnimationMgr* ani = ( AnimationMgr* )
				core->GetInterface( IE_BAM_CLASS_ID );
			if (!ani)
				return NULL;
			ani->Open( ret, true );
			AnimationFactory* af = ani->GetAnimationFactory( resname, mode );
			core->FreeInterface( ani );
			factory->AddFactoryObject( af );
			return af;
		}
		return NULL;
	}
	case IE_BMP_CLASS_ID:
	{
		ImageMgr* img = (ImageMgr*) GetResource( resname, &ImageMgr::ID );
		if (img) {
			ImageFactory* fact = img->GetImageFactory( resname );
			core->FreeInterface( img );
			factory->AddFactoryObject( fact );
			return fact;
		}

		return NULL;
	}
	default:
		printf( "\n" );
		printMessage( "KEYImporter", " ", WHITE );
		printf( "%s files are not supported.\n", core->TypeExt( type ) );
		return NULL;
	}
}
