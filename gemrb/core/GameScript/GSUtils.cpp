/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2005 The GemRB Project
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

#include "GameScript/GSUtils.h"
#include "GameScript/Matching.h"

#include "strrefs.h"
#include "defsounds.h"
#include "ie_feats.h"
#include "voodooconst.h"

#include "AmbientMgr.h"
#include "Audio.h"
#include "CharAnimations.h"
#include "DialogHandler.h"
#include "DisplayMessage.h"
#include "Game.h"
#include "GameData.h"
#include "GlobalTimer.h"
#include "Interface.h"
#include "Item.h"
#include "Map.h"
#include "Spell.h"
#include "StringMgr.h"
#include "TableMgr.h"
#include "TileMap.h"
#include "VEFObject.h"
#include "WorldMap.h"
#include "GUI/GameControl.h"
#include "RNG.h"
#include "Scriptable/Container.h"
#include "Scriptable/Door.h"
#include "Scriptable/InfoPoint.h"
#include "ScriptedAnimation.h"

#include <cstdio>

namespace GemRB {

//these tables will get freed by Core
std::shared_ptr<SymbolMgr> triggersTable;
std::shared_ptr<SymbolMgr> actionsTable;
std::shared_ptr<SymbolMgr> overrideTriggersTable;
std::shared_ptr<SymbolMgr> overrideActionsTable;
std::shared_ptr<SymbolMgr> objectsTable;
TriggerFunction triggers[MAX_TRIGGERS];
ActionFunction actions[MAX_ACTIONS];
short actionflags[MAX_ACTIONS];
short triggerflags[MAX_TRIGGERS];
ObjectFunction objects[MAX_OBJECTS];
IDSFunction idtargets[MAX_OBJECT_FIELDS];
ResRefRCCache<Script> BcsCache; //cache for scripts
int ObjectIDSCount = 7;
int MaxObjectNesting = 5;
bool HasAdditionalRect = false;
bool HasTriggerPoint = false;
//don't create new variables
bool NoCreate = false;
bool HasKaputz = false;
std::vector<ResRef> ObjectIDSTableNames;
int ObjectFieldsCount = 7;
int ExtraParametersCount = 0;
int RandomNumValue;
// reaction modifiers (by reputation and charisma)
#define MAX_REP_COLUMN 20
#define MAX_CHR_COLUMN 25
int rmodrep[MAX_REP_COLUMN];
int rmodchr[MAX_CHR_COLUMN];
ieWordSigned happiness[3][MAX_REP_COLUMN];
Gem_Polygon **polygons;

void InitScriptTables()
{
	//initializing the happiness table
	AutoTable tab = gamedata->LoadTable("happy");
	if (tab) {
		for (int alignment=0;alignment<3;alignment++) {
			for (int reputation=0;reputation<MAX_REP_COLUMN;reputation++) {
				happiness[alignment][reputation] = tab->QueryFieldSigned<ieWordSigned>(reputation, alignment);
			}
		}
	}

	//initializing the reaction mod. reputation table
	AutoTable rmr = gamedata->LoadTable("rmodrep");
	if (rmr) {
		for (int reputation=0; reputation<MAX_REP_COLUMN; reputation++) {
			rmodrep[reputation] = rmr->QueryFieldSigned<int>(0, reputation);
		}
	}

	//initializing the reaction mod. charisma table
	AutoTable rmc = gamedata->LoadTable("rmodchr");
	if (rmc) {
		for (int charisma=0; charisma<MAX_CHR_COLUMN; charisma++) {
			rmodchr[charisma] = rmc->QueryFieldSigned<int>(0, charisma);
		}
	}

	// see note in voodooconst.h
	if (core->HasFeature(GFFlags::AREA_OVERRIDE)) {
		MAX_OPERATING_DISTANCE = 40*3;
	}
}

int GetReaction(const Actor *target, const Scriptable *Sender)
{
	int rep;
	if (target->GetStat(IE_EA) == EA_PC) {
		rep = core->GetGame()->Reputation/10-1;
	} else {
		rep = target->GetStat(IE_REPUTATION)/10-1;
	}
	rep = Clamp(rep, 0, MAX_REP_COLUMN - 1);

	int chr = target->GetStat(IE_CHR) - 1;
	chr = Clamp(chr, 0, MAX_CHR_COLUMN - 1);

	int reaction = 10 + rmodrep[rep] + rmodchr[chr];

	// add -4 penalty when dealing with racial enemies
	const Actor* scr = Scriptable::As<Actor>(Sender);
	if (scr && target->GetRangerLevel()) {
		reaction -= target->GetRacialEnemyBonus(scr);
	}

	return reaction;
}

ieWordSigned GetHappiness(const Scriptable* Sender, int reputation)
{
	const Actor* ab = Scriptable::As<Actor>(Sender);
	if (!ab) {
		return 0;
	}

	int alignment = ab->GetStat(IE_ALIGNMENT)&AL_GE_MASK; //good / evil
	// handle unset alignment
	if (!alignment) {
		alignment = AL_GE_NEUTRAL;
	}
	reputation = Clamp(reputation, 10, 200);
	return happiness[alignment-1][reputation/10-1];
}

int GetHPPercent(const Scriptable *Sender)
{
	const Actor* ab = Scriptable::As<Actor>(Sender);
	if (!ab) {
		return 0;
	}

	int hp1 = ab->GetStat(IE_MAXHITPOINTS);
	if (hp1<1) {
		return 0;
	}
	int hp2 = ab->GetBase(IE_HITPOINTS);
	if (hp2<1) {
		return 0;
	}
	return hp2*100/hp1;
}

void HandleBitMod(ieDword &value1, ieDword value2, BitOp opcode)
{
	SetBits(value1, value2, opcode);
}

// SPIT is not in the original engine spec, it is reserved for the
// enchantable items feature
//                                              0       1       2       3       4
static const StringView spell_suffices[] = { "SPIT", "SPPR", "SPWI", "SPIN", "SPCL" };

//this function handles the polymorphism of Spell[RES] actions
//it returns spellres
bool ResolveSpellName(ResRef& spellRes, const Action *parameters)
{
	if (!parameters->resref0Parameter.IsEmpty()) {
		spellRes = parameters->resref0Parameter;
	} else {
		//resolve spell
		int type = parameters->int0Parameter/1000;
		int spellid = parameters->int0Parameter%1000;
		if (type>4) {
			return false;
		}
		spellRes.Format("{}{:03d}", spell_suffices[type], spellid);
	}
	return gamedata->Exists(spellRes, IE_SPL_CLASS_ID);
}

void ResolveSpellName(ResRef& spellRes, ieDword number)
{
	//resolve spell
	unsigned int type = number/1000;
	int spellid = number%1000;
	if (type>4) {
		type=0;
	}
	spellRes.Format("{}{:03d}", spell_suffices[type], spellid);
}

ieDword ResolveSpellNumber(const ResRef& spellRef)
{
	ResRef tmp;
	tmp.Format("{:.4}", spellRef);
	for (int i = 0; i < 5; i++) {
		if (tmp == spell_suffices[i]) {
			tmp = ResRef(spellRef.c_str() + 4);
			ieDword n = strtounsigned<ieDword>(tmp.c_str());
			if (!n) {
				return 0xffffffff;
			}
			return i * 1000 + n;
		}
	}
	return 0xffffffff;
}

bool ResolveItemName(ResRef& itemres, const Actor *act, ieDword Slot)
{
	const CREItem *itm = act->inventory.GetSlotItem(Slot);
	if(itm) {
		itemres = itm->ItemResRef;
		return gamedata->Exists(itemres, IE_ITM_CLASS_ID);
	}
	return false;
}

unsigned int StoreCountItems(const ResRef& storeName, const ResRef& itemName)
{
	const Store* store = gamedata->GetStore(storeName);
	if (!store) {
		Log(ERROR, "GameScript", "Store cannot be opened!");
		return 0;
	}

	unsigned int count = store->CountItems(itemName);
	return count;
}

bool StoreHasItemCore(const ResRef& storename, const ResRef& itemname)
{
	const Store* store = gamedata->GetStore(storename);
	if (!store) {
		Log(ERROR, "GameScript", "Store cannot be opened!");
		return false;
	}

	bool ret = false;
	//don't use triggers (pst style), it would be possible to create infinite loops
	if (store->FindItem(itemname, false) != (unsigned int)-1) {
		ret=true;
	}
	// Don't call gamedata->SaveStore, we don't change it, and it remains cached.
	return ret;
}

static bool StoreGetItemCore(CREItem &item, const ResRef& storename, const ResRef& itemname, unsigned int count)
{
	Store* store = gamedata->GetStore(storename);
	if (!store) {
		Log(ERROR, "GameScript", "Store cannot be opened!");
		return false;
	}

	//RemoveItem doesn't use trigger, and hopefully this will always run on bags (with no triggers)
	unsigned int idx = store->FindItem(itemname, false);
	if (idx == (unsigned int) -1) return false;

	STOItem *si = store->GetItem(idx, false);
	item.CopySTOItem(si);
	if (item.MaxStackAmount) {
		item.Usages[0] = count;
	}
	if (si->InfiniteSupply != -1) {
		if (si->AmountInStock > count) {
			si->AmountInStock -= count;
		} else {
			store->RemoveItem(si);
		}
		//store changed, save it
		gamedata->SaveStore(store);
	}
	return true;
}

void ClickCore(Scriptable *Sender, const MouseEvent& me, int speed)
{
	Point mp = me.Pos();
	const Map *map = Sender->GetCurrentArea();
	if (!map) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Size size = map->TMap->GetMapSize();
	Region r(Point(), size);
	if (!r.PointInside(mp)) {
		Sender->ReleaseCurrentAction();
		return;
	}
	GlobalTimer& timer = core->timer;
	timer.SetMoveViewPort( mp, speed, true );
	timer.DoStep(0);
	if (timer.ViewportIsMoving()) {
		Sender->AddActionInFront( Sender->GetCurrentAction() );
		Sender->SetWait(1);
		Sender->ReleaseCurrentAction();
		return;
	}

	GameControl* gc = core->GetGameControl();
	gc->MouseDown(me, 0);
	gc->MouseUp(me, 0);

	Sender->ReleaseCurrentAction();
}

void PlaySequenceCore(Scriptable *Sender, const Action *parameters, Animation::index_t value)
{
	Scriptable* tar;

	if (parameters->objects[1]) {
		tar = GetScriptableFromObject(Sender, parameters->objects[1]);
		if (!tar) {
			//could be an animation
			AreaAnimation* anim = Sender->GetCurrentArea( )->GetAnimation( parameters->objects[1]->objectNameVar);
			if (anim) {
				//set animation's cycle to parameters->int0Parameter;
				anim->sequence=value;
				anim->frame=0;
				//what else to be done???
				anim->InitAnimation();
			}
			return;
		}

	} else {
		tar = Sender;
	}

	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}
	// it should play out the sequence once and stop
	actor->SetStance( value );
	// it's a blocking action family, but the original didn't actually block
	// TODO: reset the stance back once done
}

void TransformItemCore(Actor *actor, const Action *parameters, bool onlyone)
{
	int i = actor->inventory.GetSlotCount();
	while(i--) {
		const CREItem *item = actor->inventory.GetSlotItem(i);
		if (!item) {
			continue;
		}
		if (item->ItemResRef != parameters->resref0Parameter) {
			continue;
		}
		actor->inventory.SetSlotItemRes(parameters->resref1Parameter, i, parameters->int0Parameter, parameters->int1Parameter, parameters->int2Parameter);
		if (onlyone) {
			break;
		}
	}
}

//check if an inventory (container or actor) has item (could be recursive ?)
bool HasItemCore(const Inventory *inventory, const ResRef& itemname, ieDword flags)
{
	if (itemname.IsEmpty()) return false;
	if (inventory->HasItem(itemname, flags)) {
		return true;
	}
	int i=inventory->GetSlotCount();
	while (i--) {
		//maybe we could speed this up if we mark bag items with a flags bit
		const CREItem *itemslot = inventory->GetSlotItem(i);
		if (!itemslot)
			continue;
		const Item *item = gamedata->GetItem(itemslot->ItemResRef);
		if (!item)
			continue;
		bool ret = false;
		if (core->CheckItemType(item, SLOT_BAG)) {
			//the store is the same as the item's name
			ret = StoreHasItemCore(itemslot->ItemResRef, itemname);
		}
		gamedata->FreeItem(item, itemslot->ItemResRef);
		if (ret) {
			return true;
		}
	}
	return false;
}

bool RemoveStoreItem(const ResRef& storeName, const ResRef& itemName)
{
	Store* store = gamedata->GetStore(storeName);
	if (!store) {
		Log(ERROR, "GameScript", "Store cannot be opened!");
		return false;
	}
	store->RemoveItemByName(itemName);
	// store changed, save it
	gamedata->SaveStore(store);
	return true;
}

//finds and takes an item from a container in the given inventory
static bool GetItemContainer(CREItem &itemslot2, const Inventory *inventory, const ResRef& itemname, int count)
{
	int i=inventory->GetSlotCount();
	while (i--) {
		//maybe we could speed this up if we mark bag items with a flags bit
		const CREItem *itemslot = inventory->GetSlotItem(i);
		if (!itemslot)
			continue;
		const Item *item = gamedata->GetItem(itemslot->ItemResRef);
		if (!item)
			continue;
		bool ret = core->CheckItemType(item, SLOT_BAG);
		gamedata->FreeItem(item, itemslot->ItemResRef);
		if (!ret)
			continue;
		//the store is the same as the item's name
		if (StoreGetItemCore(itemslot2, itemslot->ItemResRef, itemname, count)) {
			return true;
		}
	}
	return false;
}

void DisplayStringCoreVC(Scriptable* Sender, size_t vc, int flags)
{
	//no one hears you when you are in the Limbo!
	if (!Sender || !Sender->GetCurrentArea()) {
		return;
	}

	Log(MESSAGE, "GameScript", "Displaying string on: {}", Sender->GetScriptName());
	
	ieStrRef Strref = ieStrRef::INVALID;
	flags |= DS_CONST;

	const Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Log(ERROR, "GameScript", "Verbal constant not supported for non actors!");
		return;
	}

	if (vc >= VCONST_COUNT) {
		Log(ERROR, "GameScript", "Invalid verbal constant!");
		return;
	}

	Strref = actor->GetVerbalConstant(vc);
	if (Strref == ieStrRef::INVALID || (actor->GetStat(IE_MC_FLAGS) & MC_EXPORTABLE)) {
		//get soundset based string constant
		ResRef soundRef;
		actor->GetVerbalConstantSound(soundRef, vc);
		std::string sound;
		if (actor->PCStats && actor->PCStats->SoundFolder[0]) {
			sound = fmt::format("{}/{}", actor->PCStats->SoundFolder, soundRef);
		} else {
			sound = soundRef.c_str();
		}
		return DisplayStringCore(Sender, Strref, flags, sound.c_str());
	}
	DisplayStringCore(Sender, Strref, flags);
}

void DisplayStringCore(Scriptable* const Sender, ieStrRef Strref, int flags, const char* soundpath)
{
	if (Strref == ieStrRef::INVALID) return;

	// Check if subtitles are not enabled
	ieDword charactersubtitles = core->GetVariable("Subtitles", 0);

	// display the verbal constants in the console; some callers force this themselves
	if (charactersubtitles) {
		// to avoid spam in Saradush while subtitles are enabled, skip printing to MSGWIN if too far
		if (!(flags & DS_HEAD) || Sender->GetCurrentArea()->IsVisible(Sender->Pos)) {
			flags |= DS_CONSOLE;
		}
	}

	// PST does not echo verbal constants in the console, their strings
	// actually contain development related identifying comments
	// thus the console flag is unset.
	if (core->HasFeature(GFFlags::ONSCREEN_TEXT) || !charactersubtitles) {
		flags &= ~DS_CONSOLE;
	}

	ResRef buffer;
	if (soundpath == nullptr || soundpath[0] == '\0') {
		StringBlock sb = core->strings->GetStringBlock( Strref );
		if (!sb.Sound.IsEmpty()) {
			buffer = sb.Sound;
			soundpath = buffer.c_str();
		}
		if (!sb.text.empty()) {
			if (flags & DS_CONSOLE) {
				//can't play the sound here, we have to delay action
				//and for that, we have to know how long the text takes
				if(flags&DS_NONAME) {
					displaymsg->DisplayString(sb.text);
				} else {
					displaymsg->DisplayStringName(Strref, GUIColors::WHITE, Sender, STRING_FLAGS::NONE);
				}
			}
			if (flags & (DS_HEAD | DS_AREA)) {
				Sender->overHead.SetText(sb.text, true, false);
				if (flags & DS_AREA) {
					Sender->overHead.FixPos(Sender->Pos);
				}
			}
		}
	}

	if (soundpath && soundpath[0] && !(flags & DS_SILENT)) {
		ieDword speech = 0;
		Point pos = Sender->Pos;
		if (flags&DS_SPEECH) {
			speech = GEM_SND_SPEECH;
		}
		// disable position, but only for party
		Actor* actor = Scriptable::As<Actor>(Sender);
		if (!actor || actor->InParty ||
			core->InCutSceneMode() || core->GetGameControl()->InDialog()) {
			speech |= GEM_SND_RELATIVE;
			pos.reset();
		}
		if (flags&DS_QUEUE) speech|=GEM_SND_QUEUE;
		
		unsigned int channel = SFX_CHAN_DIALOG;
		if (flags & DS_CONST && actor) {
			if (actor->InParty > 0) {
				channel = SFX_CHAN_CHAR0 + actor->InParty - 1;
			} else if (actor->GetStat(IE_EA) >= EA_EVILCUTOFF) {
				channel = SFX_CHAN_MONSTER;
			}
		}
		
		tick_t len = 0;
		core->GetAudioDrv()->Play(StringView(soundpath), channel, pos, speech, &len);
		tick_t counter = (core->Time.defaultTicksPerSec * len) / 1000;

		if (actor && len > 0 && flags & DS_CIRCLE) {
			actor->SetAnimatedTalking(len);
		}

		if ((counter != 0) && (flags &DS_WAIT) )
			Sender->SetWait( counter );
	}
}

int CanSee(const Scriptable *Sender, const Scriptable *target, bool range, int seeflag, bool halveRange)
{
	if (target->Type==ST_ACTOR) {
		const Actor *tar = static_cast<const Actor*>(target);

		if (!tar->ValidTarget(seeflag, Sender)) {
			return 0;
		}
	}

	const Map *map = target->GetCurrentArea();
	//if (!(seeflag&GA_GLOBAL)) {
		if (!map || map!=Sender->GetCurrentArea() ) {
			return 0;
		}
	//}

	if (range) {
		unsigned int dist;
		bool los = true;
		if (Sender->Type == ST_ACTOR) {
			const Actor *snd = static_cast<const Actor*>(Sender);
			dist = snd->Modified[IE_VISUALRANGE];
			if (halveRange) dist /= 2;
		} else {
			dist = VOODOO_VISUAL_RANGE;
			los = false;
		}

		if (!WithinRange(target, Sender->Pos, dist)) {
			return 0;
		}
		if (!los) {
			return 1;
		}
	}

	return map->IsVisibleLOS(target->Pos, Sender->Pos);
}

//non actors can see too (reducing function to LOS)
//non actors can be seen too (reducing function to LOS)
int SeeCore(Scriptable *Sender, const Trigger *parameters, int justlos)
{
	//see dead; unscheduled actors are never visible, though
	int flags = GA_NO_UNSCHEDULED;

	if (parameters->int0Parameter) {
		flags |= GA_DETECT;
	} else {
		flags |= GA_NO_DEAD;
	}
	const Scriptable* tar = GetScriptableFromObject(Sender, parameters->objectParameter, flags);
	/* don't set LastSeen if this isn't an actor */
	if (!tar) {
		return 0;
	}

	// ignore invisible targets for direct matching
	if (! parameters->int0Parameter) {
		flags |= GA_NO_HIDDEN;
	}

	//both are actors
	if (CanSee(Sender, tar, true, flags) ) {
		if (justlos) {
			Sender->LastTrigger = tar->GetGlobalID();
			return 1;
		}
		// NOTE: Detect supposedly doesn't set LastMarked — disable on GA_DETECT if needed
		if (Sender->Type==ST_ACTOR && tar->Type==ST_ACTOR && Sender!=tar) {
			Actor* snd = static_cast<Actor*>(Sender);
			snd->LastSeen = tar->GetGlobalID();
			snd->LastMarked = tar->GetGlobalID();
		}
		Sender->LastTrigger = tar->GetGlobalID();
		return 1;
	}
	return 0;
}

//transfering item from Sender to target
//if target has no inventory, the item will be destructed
//if target can't get it, it will be dropped at its feet
int MoveItemCore(Scriptable *Sender, Scriptable *target, const ResRef& resref, int flags, int setflag, int count)
{
	Inventory *myinv;
	Map *map;
	// track whether we are dealing with our party and need to display feedback
	bool lostitem = false;
	bool gotitem = false;

	if (!target) {
		return MIC_INVALID;
	}
	map=Sender->GetCurrentArea();
	Actor* tmp = nullptr;
	switch(Sender->Type) {
		case ST_ACTOR:
			tmp = Scriptable::As<Actor>(Sender);
			myinv = &tmp->inventory;
			if (tmp->InParty) lostitem = true;
			break;
		case ST_CONTAINER:
			myinv=&((Container *) Sender)->inventory;
			break;
		default:
			return MIC_INVALID;
	}
	CREItem *item;
	myinv->RemoveItem(resref, flags, &item, count);

	//there was no item in the inventory itself, try with containers in the inventory
	if (!item) {
		item = new CREItem();

		if (count <= 0) count = 1;
		if (!GetItemContainer(*item, myinv, ResRef(resref), count)) {
			delete item;
			item = NULL;
		}
	}

	if (!item) {
		// nothing was removed
		return MIC_NOITEM;
	}

	item->Flags|=setflag;

	switch(target->Type) {
		case ST_ACTOR:
			tmp = Scriptable::As<Actor>(target);
			myinv = &tmp->inventory;
			if (tmp->InParty) gotitem = true;
			break;
		case ST_CONTAINER:
			myinv=&((Container *) target)->inventory;
			break;
		default:
			myinv = NULL;
			break;
	}
	if (lostitem && !gotitem) {
		displaymsg->DisplayMsgCentered(HCStrings::LostItem, FT_ANY, GUIColors::XPCHANGE);
	}

	if (!myinv) {
		delete item;
		return MIC_GOTITEM; // actually it was lost, not gained
	}
	if ( myinv->AddSlotItem(item, SLOT_ONLYINVENTORY) !=ASI_SUCCESS) {
		// drop it at my feet
		map->AddItemToLocation(target->Pos, item);
		if (gotitem) {
			tmp = Scriptable::As<Actor>(target);
			if (tmp && tmp->InParty) {
				tmp->VerbalConstant(VB_INVENTORY_FULL);
			}
			displaymsg->DisplayMsgCentered(HCStrings::InventoryFullItemDrop, FT_ANY, GUIColors::XPCHANGE);
		}
		return MIC_FULL;
	}
	if (gotitem && !lostitem) {
		displaymsg->DisplayMsgCentered(HCStrings::GotItem, FT_ANY, GUIColors::XPCHANGE);
	}
	return MIC_GOTITEM;
}

void PolymorphCopyCore(const Actor *src, Actor *tar)
{
	tar->SetBase(IE_ANIMATION_ID, src->GetStat(IE_ANIMATION_ID) );

	tar->SetBase(IE_ARMOR_TYPE, src->GetStat(IE_ARMOR_TYPE) );
	for (int i=0;i<7;i++) {
		tar->SetBase(IE_COLORS+i, src->GetStat(IE_COLORS+i) );
	}

	tar->SetName(src->GetShortName(), 0);
	tar->SetName(src->GetName(),1);
	//add more attribute copying
}

static bool InspectEdges(Point& walkableStartPoint, const Region& vp, int currentStep, int offset)
{
	bool isPassable = false;
	switch (currentStep) {
		// Top-edge with random x offset
		case 0:
			if (offset < vp.w) {
				walkableStartPoint.x = vp.x + offset;
				walkableStartPoint.y = vp.y;
				isPassable = true;
			}
			break;
		// Bottom-edge with random x offset
		case 1:
			if (offset < vp.w) {
				walkableStartPoint.x = vp.x + offset;
				walkableStartPoint.y = vp.y + vp.h;
				isPassable = true;
			}
			break;
		// Left-edge with random y offset
		case 2:
			if (offset < vp.h) {
				walkableStartPoint.x = vp.x;
				walkableStartPoint.y = vp.y + offset;
				isPassable = true;
			}
			break;
		// Right-edge with random y offset
		case 3:
			if (offset < vp.h) {
				walkableStartPoint.x = vp.x + vp.w;
				walkableStartPoint.y = vp.y + offset;
				isPassable = true;
			}
			break;
	}
	return isPassable;
}

// this complicated search has been reverse-engineered from the original
static Point FindOffScreenPoint(const Scriptable* Sender, int flags, int phase)
{
	Region vp0 = core->GetGameControl()->Viewport();
	// go for 640x480, so large viewports are less likely to interfere with scripting
	Region vp(vp0.x + (vp0.w - 640) / 2, vp0.y + (vp0.h - 480) / 2, 640, 480);
	Point vpCenter = vp.Center();
	int maxRandExclusive = std::max(vp.w, vp.h);
	int firstRandStep = RAND(0, maxRandExclusive);
	int currentStep = RAND(0, 3);
	int slowlyIncrements = 0;

	const Map *map = Sender->GetCurrentArea();
	Point walkableStartPoint;
	Point walkableGoal;
	if (flags & CC_OBJECT) {
		// the point of the target creature for CreateCreatureObjectOffscreen
		walkableGoal = Sender->Pos;
	} else {
		// center of the viewport for CreateCreatureOffscreen
		walkableGoal = vpCenter;
	}

	do {
		int finalRandStep = (firstRandStep + slowlyIncrements) % maxRandExclusive;

		for (int switchAttemptCounter = 0; switchAttemptCounter < 4; ++switchAttemptCounter) {
			bool isPassable = InspectEdges(walkableStartPoint, vp, currentStep, phase ? slowlyIncrements : finalRandStep);

			++currentStep;
			if (currentStep > 3) {
				currentStep = 0;
			}

			if (isPassable) {
				// Check if the search map allows a creature to be at walkableStartPoint (note: ignoring creatureSize)
				isPassable = bool(map->GetBlocked(walkableStartPoint) & PathMapFlags::PASSABLE);
			}
			if (!isPassable) continue;

			if (flags & CC_OBJECT) {
				// Check if walkableStartPoint can traverse to walkableGoal
				// finding a full path via Map::FindPath is too expensive, just consider the searchmap
				bool isWalkable = map->IsWalkableTo(walkableStartPoint, walkableGoal, true, nullptr);
				if (isWalkable) return walkableStartPoint;
			} else {
				// walkableStartPoint is the final point
				return walkableStartPoint;
			}
		}

		slowlyIncrements += 10;
	} while (slowlyIncrements < maxRandExclusive);

	if (phase) {
		// fallback in case nothing was found even in the second try
		return vpCenter;
	} else {
		return Point();
	}
}

void CreateCreatureCore(Scriptable* Sender, Action* parameters, int flags)
{
	Scriptable* tmp = GetScriptableFromObject(Sender, parameters->objects[1]);
	//if there is nothing to copy, don't spawn anything
	if (flags & CC_COPY && (!tmp || tmp->Type != ST_ACTOR)) {
		return;
	}

	Actor* ab;
	if (flags & CC_STRING1) {
		ab = gamedata->GetCreature(parameters->resref1Parameter);
	} else {
		ab = gamedata->GetCreature(parameters->resref0Parameter);
	}

	if (!ab) {
		Log(ERROR, "GameScript", "Failed to create creature! (missing creature file {}?)",
			parameters->string0Parameter);
		// maybe this should abort()?
		return;
	}

	//iwd2 allows an optional scriptname to be set
	//but bg2 doesn't have this feature
	//this way it works for both games
	if ((flags & CC_SCRIPTNAME) && !parameters->variable1Parameter.IsEmpty()) {
		ab->SetScriptName(parameters->variable1Parameter);
	}

	Point pnt;
	const Scriptable *referer = Sender;

	switch (flags & CC_MASK) {
		//creates creature just off the screen
		case CC_OFFSCREEN:
			// handle also the combo with CC_OBJECT, so we don't have fallthrough problems
			if (flags & CC_OBJECT && tmp) referer = tmp;
			pnt = FindOffScreenPoint(referer, flags, 0);
			if (pnt.IsZero()) {
				pnt = FindOffScreenPoint(referer, flags, 1);
			}
			break;
		case CC_OBJECT://use object + offset
			if (tmp) Sender=tmp;
			//fall through
		case CC_OFFSET://use sender + offset
			pnt = parameters->pointParameter + Sender->Pos;
			break;
		default: //absolute point, but -1,-1 means AtFeet
			pnt = parameters->pointParameter;
			if (!pnt.IsInvalid()) break;

			if (Sender->Type == ST_PROXIMITY || Sender->Type == ST_TRIGGER) {
				pnt = static_cast<InfoPoint*>(Sender)->TrapLaunch;
			} else {
				pnt = Sender->Pos;
			}
			break;
	}

	Map *map = Sender->GetCurrentArea();
	map->AddActor(ab, true);
	ab->SetPosition(pnt, flags & CC_CHECK_IMPASSABLE, 0, 0);
	ab->SetOrientation(ClampToOrientation(parameters->int0Parameter), false);

	// also set it as Sender's LastMarkedObject (fixes worg rider dismount killing players)
	if (Sender->Type == ST_ACTOR) {
		Actor *actor = static_cast<Actor*>(Sender);
		actor->LastMarked = ab->GetGlobalID();
	}

	//if string1 is animation, then we can't use it for a DV too
	if (flags & CC_PLAY_ANIM) {
		CreateVisualEffectCore(ab, ab->Pos, parameters->resref1Parameter, 1);
	} else if (!parameters->variable1Parameter.IsEmpty()) {
		//setting the deathvariable if it exists (iwd2)
		ab->SetScriptName(parameters->variable1Parameter);
	}

	if (flags & CC_COPY) {
		PolymorphCopyCore((const Actor *) tmp, ab);
	}
}

static ScriptedAnimation *GetVVCEffect(const ResRef& effect, int iterations)
{
	if (effect.IsEmpty()) {
		return nullptr;
	}

	ScriptedAnimation* vvc = gamedata->GetScriptedAnimation(effect, false);
	if (!vvc) {
		Log(ERROR, "GameScript", "Failed to create effect.");
		return nullptr;
	}
	if (iterations > 1) {
		vvc->SetDefaultDuration(vvc->GetSequenceDuration(core->Time.defaultTicksPerSec * iterations));
	} else {
		vvc->PlayOnce();
	}
	return vvc;
}

void CreateVisualEffectCore(Actor* target, const ResRef& effect, int iterations)
{
	ScriptedAnimation *vvc = GetVVCEffect(effect, iterations);
	if (vvc) {
		target->AddVVCell( vvc );
	}
}

void CreateVisualEffectCore(const Scriptable* Sender, const Point& position, const ResRef& effect, int iterations)
{
	Map *area = Sender->GetCurrentArea();
	if (!area) {
		Log(WARNING, "GSUtils", "Skipping visual effect positioning due to missing area!");
		return;
	}

	if (gamedata->Exists(effect, IE_VEF_CLASS_ID, true)) {
		VEFObject* vef = gamedata->GetVEFObject(effect, false);
		vef->Pos = position;
		area->AddVVCell(vef);
	} else {
		ScriptedAnimation* vvc = GetVVCEffect(effect, iterations);
		if (vvc) {
			vvc->Pos = position;
			area->AddVVCell(new VEFObject(vvc));
		}
	}
}

//this destroys the current actor and replaces it with another
void ChangeAnimationCore(Actor* src, const ResRef& replacement, bool effect)
{
	Actor* tar = gamedata->GetCreature(replacement);
	if (tar) {
		Map *map = src->GetCurrentArea();
		Point pos = src->Pos;
		// make sure to copy the HP, to avoid things like magically-healing trolls
		tar->BaseStats[IE_HITPOINTS] = src->BaseStats[IE_HITPOINTS];
		tar->SetOrientation(src->GetOrientation(), false);
		src->DestroySelf();
		// can't SetPosition while the old actor is taking the spot
		map->AddActor(tar, true);
		tar->SetPosition(pos, 1, 8*effect, 8*effect);
		if (effect) {
			CreateVisualEffectCore(tar, tar->Pos, "spsmpuff", 1);
		}
	}
}

void EscapeAreaCore(Scriptable* Sender, const Point &p, const ResRef& area, const Point &enter, int flags, int wait)
{
	if (Sender->CurrentActionTicks<100) {
		if (!p.IsInvalid() && PersonalDistance(p, Sender)>MAX_OPERATING_DISTANCE) {
			//MoveNearerTo will return 0, if the actor is in move
			//it will return 1 (the fourth parameter) if the target is unreachable
			if (!MoveNearerTo(Sender, p, MAX_OPERATING_DISTANCE,1) ) {
				if (!Sender->InMove()) Log(WARNING, "GSUtils", "At least it said so...");
				// ensure the action doesn't get interrupted
				// fixes Nalia starting a second dialog in the Coronet, if she gets a chance #253
				Sender->CurrentActionInterruptable = false;
				return;
			}
		}
	}

	std::string Tmp;
	if (flags &EA_DESTROY) {
		//this must be put into a non-const variable
		Tmp = "DestroySelf()";
	} else {
		// last parameter is 'face', which should be passed from relevant action parameter..
		Tmp = fmt::format("MoveBetweenAreas(\"{}\",[{}.{}],{})", area, enter.x, enter.y, 0);
	}
	Log(MESSAGE, "GSUtils", "Executing {} in EscapeAreaCore", Tmp);
	//drop this action, but add another (destroyself or movebetweenareas)
	//between the arrival and the final escape, there should be a wait time
	//that wait time could be handled here
	if (wait) {
		Log(WARNING, "GSUtils", "But wait a bit... ({})", wait);
		Sender->SetWait(wait);
	}
	Sender->ReleaseCurrentAction();
	Action * action = GenerateAction(std::move(Tmp));
	Sender->AddActionInFront( action );
}

static void GetTalkPositionFromScriptable(Scriptable* scr, Point &position)
{
	const InfoPoint* ip;
	switch (scr->Type) {
		case ST_AREA: case ST_GLOBAL:
			position = scr->Pos; //fake
			break;
		case ST_ACTOR:
			//if there are other moveables, put them here
			position = ((Movable *) scr)->GetMostLikelyPosition();
			break;
		case ST_TRIGGER: case ST_PROXIMITY: case ST_TRAVEL:
			ip = Scriptable::As<InfoPoint>(scr);
			if (ip->GetUsePoint()) {
				position = ip->UsePoint;
				break;
			}
			position = ip->TalkPos;
			break;
		case ST_DOOR: case ST_CONTAINER:
			position = static_cast<Highlightable*>(scr)->TrapLaunch;
			break;
	}
}

void GetPositionFromScriptable(const Scriptable *scr, Point &position, bool dest)
{
	if (!dest) {
		position = scr->Pos;
		return;
	}

	const InfoPoint* ip;
	switch (scr->Type) {
		case ST_AREA: case ST_GLOBAL:
			position = scr->Pos; //fake
			break;
		case ST_ACTOR:
		//if there are other moveables, put them here
			position = static_cast<const Movable*>(scr)->GetMostLikelyPosition();
			break;
		case ST_TRIGGER: case ST_PROXIMITY: case ST_TRAVEL:
			ip = Scriptable::As<InfoPoint>(scr);
			if (ip->GetUsePoint()) {
				position = ip->UsePoint;
				break;
			}
		// intentional fallthrough
		case ST_DOOR: case ST_CONTAINER:
			position = static_cast<const Highlightable*>(scr)->TrapLaunch;
	}
}

void BeginDialog(Scriptable* Sender, const Action* parameters, int Flags)
{
	Scriptable* tar = NULL, *scr = NULL;

	ScriptDebugLog(ID_VARIABLES, "BeginDialog core");

	tar = GetStoredActorFromObject(Sender, parameters->objects[1], GA_NO_DEAD);
	if (Flags & BD_OWN) {
		scr = tar;
	} else {
		scr = Sender;
	}
	if (!scr) {
		assert(Sender);
		Log(ERROR, "GameScript", "Speaker for dialog couldn't be found (Sender: {}, Type: {}) Flags:{}.",
			Sender->GetScriptName(), Sender->Type, Flags);
		Sender->ReleaseCurrentAction();
		return;
	}
	// do not allow disabled actors to start dialog
	if (!(scr->GetInternalFlag() & IF_VISIBLE)) {
		Sender->ReleaseCurrentAction();
		return;
	}

	if (!tar || tar->Type!=ST_ACTOR) {
		Log(ERROR, "GameScript", "Target for dialog couldn't be found (Sender: {}, Type: {}).",
			Sender->GetScriptName(), Sender->Type);
		if (Sender->Type == ST_ACTOR) {
			Sender->As<const Actor>()->dump();
		}
		std::string buffer("Target object: ");
		if (parameters->objects[1]) {
			buffer.append(parameters->objects[1]->dump(false));
		} else {
			buffer.append("<NULL>\n");
		}
		Log(ERROR, "GameScript", "{}", buffer);
		Sender->ReleaseCurrentAction();
		return;
	}

	const Actor *speaker = Scriptable::As<Actor>(scr);
	Actor *target = (Actor *) tar;
	bool swap = false;
	if (speaker) {
		if (speaker->GetStat(IE_STATE_ID)&STATE_DEAD) {
			Log(ERROR, "GameScript", "Speaker is dead, cannot start dialogue. Speaker and target are:");
			speaker->dump();
			target->dump();
			Sender->ReleaseCurrentAction();
			return;
		}
		//making sure speaker is the protagonist, player, actor
		const Actor *protagonist = core->GetGame()->GetPC(0, false);
		if (target == protagonist) swap = true;
		else if (speaker != protagonist && target->InParty) swap = true;
		//CHECKDIST works only for mobile scriptables
		if (Flags&BD_CHECKDIST) {
			//DialogueRange is set in IWD
			ieDword range = MAX_OPERATING_DISTANCE + speaker->GetBase(IE_DIALOGRANGE);
			if ( scr->GetCurrentArea()!=target->GetCurrentArea() ||
				PersonalDistance(scr, target)>range) {
				MoveNearerTo(Sender, target, MAX_OPERATING_DISTANCE);
				return;
			}
		}
	} else {
		//pst style dialog with trigger points
		swap=true;
		if (Flags&BD_CHECKDIST) {
			Point TalkPos;

			if (target->InMove()) {
				//waiting for target
				return;
			}
			GetTalkPositionFromScriptable(scr, TalkPos);
			if (PersonalDistance(TalkPos, target)>MAX_OPERATING_DISTANCE ) {
				//try to force the target to come closer???
				if(!MoveNearerTo(target, TalkPos, MAX_OPERATING_DISTANCE, 1))
					return;
			}
		}
	}

	GameControl* gc = core->GetGameControl();
	if (!gc) {
		Log(WARNING, "GameScript", "Dialog cannot be initiated because there is no GameControl.");
		Sender->ReleaseCurrentAction();
		return;
	}
	//can't initiate dialog, because it is already there
	if (gc->InDialog()) {
		if (Flags & BD_INTERRUPT) {
			//break the current dialog if possible
			gc->dialoghandler->EndDialog(true);
		}
		//check if we could manage to break it, not all dialogs are breakable!
		if (gc->InDialog()) {
			Log(WARNING, "GameScript", "Dialog cannot be initiated because there is already one.");
			Sender->ReleaseCurrentAction();
			return;
		}
	}

	// starting a dialog ends cutscenes!
	core->SetCutSceneMode(false);

	ResRef Dialog;
	AutoTable pdtable;

	switch (Flags & BD_LOCMASK) {
		case BD_STRING0:
			Dialog = parameters->string0Parameter;
			if (Flags & BD_SETDIALOG) {
				scr->SetDialog( Dialog );
			}
			break;
		case BD_SOURCE:
			// can't handle swap as BD_TARGET, since it just breaks dialog with PCs (eg. Maadeen in the gov. district)
			// freeing Minsc requires skipping the non-interruptible check for the second dialog to properly start
			if (speaker) {
				Dialog = speaker->GetDialog(swap ? GD_NORMAL : GD_FEEDBACK);
			} else {
				Dialog = scr->GetDialog();
			}
			break;
		case BD_TARGET:
			// Don't check for the target being non-interruptible if we swapped speakers
			// or if the speaker is the target, otherwise do (and request feedback on failure).
			if (swap || speaker==target) Dialog = scr->GetDialog();
			else Dialog = target->GetDialog(GD_FEEDBACK);
			break;
		case BD_RESERVED:
			//what if playerdialog was initiated from Player2?
			Dialog = "PLAYER1";
			break;
		case BD_INTERACT: //using the source for the dialog
			const Game *game = core->GetGame();
			if (game->BanterBlockFlag || game->BanterBlockTime) {
				Log(MESSAGE, "GameScript", "Banterblock disabled interaction.");
				Sender->ReleaseCurrentAction();
				return;
			}
			const ieVariable& scriptingname = scr->GetScriptName();

			/* banter dialogue */
			pdtable = gamedata->LoadTable("interdia");
			if (pdtable) {
				if (game->Expansion == GAME_TOB) {
					Dialog = pdtable->QueryField(scriptingname, "25FILE");
				} else {
					Dialog = pdtable->QueryField(scriptingname, "FILE");
				}
			}
			break;
	}

	// moved this here from InitDialog, because InitDialog doesn't know which side is which
	// post-swap (and non-actors always have IF_NOINT set) .. also added a check that it's
	// actually busy doing something, for the same reason
	const Action *curact = target->GetCurrentAction();
	if ((speaker != target) && (target->GetInternalFlag()&IF_NOINT) && \
	  (!curact && target->GetNextAction())) {
		core->GetTokenDictionary()["TARGET"] = target->GetName();
		displaymsg->DisplayConstantString(HCStrings::TargetBusy, GUIColors::RED);
		Sender->ReleaseCurrentAction();
		return;
	}

	if (speaker!=target) {
		if (swap) {
			Scriptable *tmp = tar;
			tar = scr;
			scr = tmp;
		} else if (!(Flags & BD_INTERRUPT)) {
			// added CurrentAction as part of blocking action fixes
			if (tar->GetCurrentAction() || tar->GetNextAction()) {
				core->GetTokenDictionary()["TARGET"] = target->GetName();
				displaymsg->DisplayConstantString(HCStrings::TargetBusy, GUIColors::RED);
				Sender->ReleaseCurrentAction();
				return;
			}
		}
	}

	// When dialog is initiated in IWD2 it directly clears the action queue of all party members.
	// Bubb: in this case, and only this case as far as I can tell, it specifically preserves spell actions.
	if (core->HasFeature(GFFlags::RULES_3ED)) {
		const Game* game = core->GetGame();
		for (int i = game->GetPartySize(false) - 1; i >= 0; --i) {
			Actor *pc = game->GetPC(i, false);
			pc->ClearActions(2);
		}
	}

	//don't clear target's actions, because a sequence like this will be broken:
	//StartDialog([PC]); SetGlobal("Talked","LOCALS",1);
	// Update orientation and potentially stance
	// sarevok's resurrection cutscene shows a need for this (cut206a)
	// however we do not want to affect lying actors (eg. Malla from tob)
	if (scr!=tar) {
		if (scr->Type==ST_ACTOR) {
			// might not be equal to speaker anymore due to swapping
			Actor *talker = (Actor *) scr;
			if (!talker->Immobile() && !(talker->GetStat(IE_STATE_ID) & STATE_SLEEP) && !(talker->AppearanceFlags&APP_NOTURN)) {
				talker->SetOrientation(tar->Pos, scr->Pos, true);
				if (talker->InParty) {
					talker->SetStance(IE_ANI_READY);
				}
			}
		}
		if (tar->Type==ST_ACTOR) {
			// might not be equal to target anymore due to swapping
			Actor *talkee = static_cast<Actor*>(tar);
			if (!talkee->Immobile() && !(talkee->GetStat(IE_STATE_ID) & STATE_SLEEP) && !(talkee->AppearanceFlags&APP_NOTURN)) {
				talkee->SetOrientation(scr->Pos, tar->Pos, true);
				if (talkee->InParty) {
					talkee->SetStance(IE_ANI_READY);
				}
				if (!core->InCutSceneMode()) {
					talkee->DialogInterrupt();
				}
			}
		}
	}

	//increasing NumTimesTalkedTo or NumTimesInteracted
	if (Flags & BD_TALKCOUNT) {
		gc->SetDialogueFlags(DF_TALKCOUNT, BitOp::OR);
	} else if ((Flags & BD_LOCMASK) == BD_INTERACT) {
		gc->SetDialogueFlags(DF_INTERACT, BitOp::OR);
	}

	core->GetDictionary()["DialogChoose"] = (ieDword) -1;
	if (!gc->dialoghandler->InitDialog(scr, tar, Dialog)) {
		if (!(Flags & BD_NOEMPTY)) {
			displaymsg->DisplayConstantStringName(HCStrings::NothingToSay, GUIColors::RED, tar);
		}
	}

	Sender->ReleaseCurrentAction();
}

static EffectRef fx_movetoarea_ref = { "MoveToArea", -1 };

bool CreateMovementEffect(Actor* actor, const ResRef& area, const Point &position, int face)
{
	if (actor->Area == area) return false; //no need of this for intra area movement

	Effect *fx = EffectQueue::CreateEffect(fx_movetoarea_ref, 0, face, FX_DURATION_INSTANT_PERMANENT);
	if (!fx) return false;
	fx->SetPosition(position);
	fx->Resource = area;
	core->ApplyEffect(fx, actor, actor);
	return true;
}

void MoveBetweenAreasCore(Actor* actor, const ResRef &area, const Point &position, int face, bool adjust)
{
	Log(MESSAGE, "GameScript", "MoveBetweenAreas: {} to {} [{}.{}] face: {}",
			fmt::WideToChar{actor->GetShortName()}, area, position.x, position.y, face);
	Map* map1 = actor->GetCurrentArea();
	Map* map2;
	Game* game = core->GetGame();
	bool newSong = false;
	if (!area.IsEmpty() && (!map1 || area != map1->GetScriptRef())) { //do we need to switch area?
		//we have to change the pathfinder
		//to the target area if adjust==true
		map2 = game->GetMap(area, false);
		if (map1) {
			map1->RemoveActor( actor );
		}
		map2->AddActor( actor, true );
		newSong = true;

		// update the worldmap if needed
		if (actor->InParty) {
			WorldMap *worldmap = core->GetWorldMap();
			unsigned int areaindex;
			WMPAreaEntry *entry = worldmap->GetArea(area, areaindex);
			// make sure the area is marked as revealed and visited
			if (entry && !(entry->GetAreaStatus() & WMP_ENTRY_VISITED)) {
				entry->SetAreaStatus(WMP_ENTRY_VISIBLE | WMP_ENTRY_VISITED, BitOp::OR);
			}
		}
	}
	actor->SetPosition(position, adjust);
	actor->SetStance(IE_ANI_READY);
	if (face !=-1) {
		actor->SetOrientation(ClampToOrientation(face), false);
	}
	// should this perhaps be a 'selected' check or similar instead?
	if (actor->InParty) {
		GameControl *gc=core->GetGameControl();
		gc->SetScreenFlags(SF_CENTERONACTOR, BitOp::OR);
		if (newSong) {
			game->ChangeSong(false, true);
		}
	}
}

//repeat movement, until goal isn't reached
//if int0parameter is !=0, then it will try only x times
void MoveToObjectCore(Scriptable *Sender, Action *parameters, ieDword flags, bool untilsee)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	const Scriptable *target = GetStoredActorFromObject(Sender, parameters->objects[1]);
	if (!target) {
		Sender->ReleaseCurrentAction();
		return;
	}

	Point dest = target->Pos;
	if (target->Type == ST_TRIGGER && static_cast<const InfoPoint*>(target)->GetUsePoint()) {
		dest = static_cast<const InfoPoint*>(target)->UsePoint;
	}
	if (untilsee && CanSee(actor, target, true, 0, true)) {
		Sender->LastSeen = target->GetGlobalID();
		Sender->ReleaseCurrentAction();
		actor->ClearPath(true);
		return;
	} else {
		if (PersonalDistance(actor, target)<MAX_OPERATING_DISTANCE) {
			if (flags & IF_NOINT) {
				actor->Interrupt();
			}
			Sender->ReleaseCurrentAction();
			return;
		}
	}
	if (!actor->InMove() || actor->Destination != dest) {
		actor->WalkTo(dest, flags);
	}

	//hopefully this hack will prevent lockups
	if (!actor->InMove()) {
		if (flags&IF_NOINT) {
			actor->Interrupt();
		}
		Sender->ReleaseCurrentAction();
		return;
	}

	// try just a few times (unused in original data)
	if (parameters->int0Parameter) {
		parameters->int0Parameter--;
		if (!parameters->int0Parameter) {
			actor->Interrupt();
			Sender->ReleaseCurrentAction();
		}
	}
}

bool CreateItemCore(CREItem *item, const ResRef &resref, int a, int b, int c)
{
	item->ItemResRef = resref;
	if (!core->ResolveRandomItem(item))
		return false;
	if (a==-1) {
		//use the default charge counts of the item
		const Item *origitem = gamedata->GetItem(item->ItemResRef);
		if (origitem) {
			for(int i=0;i<3;i++) {
				const ITMExtHeader *e = origitem->GetExtHeader(i);
				item->Usages[i]=e?e->Charges:0;
			}
			gamedata->FreeItem(origitem, item->ItemResRef, false);
		}
	} else {
		item->Usages[0]=(ieWord) a;
		item->Usages[1]=(ieWord) b;
		item->Usages[2]=(ieWord) c;
	}
	item->Flags=0;
	item->Expired=0;
	core->SanitizeItem(item);
	return true;
}

//It is possible to attack CONTAINERS/DOORS as well!!!
void AttackCore(Scriptable *Sender, Scriptable *target, int flags)
{
	Actor* attacker = Scriptable::As<Actor>(Sender);
	assert(attacker);
	assert(target);

	// if held or disabled, etc, then cannot start or continue attacking
	if (attacker->Immobile()) {
		attacker->roundTime = 0;
		Sender->ReleaseCurrentAction();
		return;
	}

	// mislead and projected images can't attack
	int puppet = attacker->GetStat(IE_PUPPETMASTERTYPE);
	if (puppet && puppet < 3) {
		Log(DEBUG, "AttackCore", "Tried attacking with an illusionary copy: {}!", fmt::WideToChar{attacker->GetName()});
		return;
	}

	const Actor* tar = Scriptable::As<Actor>(target);
	if (attacker == tar) {
		Sender->ReleaseCurrentAction();
		Log(WARNING, "AttackCore", "Tried attacking itself: {}!", fmt::WideToChar{tar->GetName()});
		return;
	}

	if (tar) {
		// release if target is invisible to sender (because of death or invisbility spell)
		if (tar->IsInvisibleTo(Sender) || (tar->GetSafeStat(IE_STATE_ID) & STATE_DEAD)){
			attacker->StopAttack();
			Sender->ReleaseCurrentAction();
			attacker->AddTrigger(TriggerEntry(trigger_targetunreachable, tar->GetGlobalID()));
			Log(WARNING, "AttackCore", "Tried attacking invisible/dead actor: {}!", fmt::WideToChar{tar->GetName()});
			return;
		}
	}

	bool leftOrRight = false;
	const ITMExtHeader* header = attacker->GetWeapon(leftOrRight);
	//will return false on any errors (eg, unusable weapon)
	if (!header) {
		attacker->StopAttack();
		Sender->ReleaseCurrentAction();
		assert(tar);
		attacker->AddTrigger(TriggerEntry(trigger_unusable, tar->GetGlobalID()));
		Log(WARNING, "AttackCore", "Weapon unusable: {}!", fmt::WideToChar{attacker->GetName()});
		return;
	}

	unsigned int weaponRange = attacker->GetWeaponRange(leftOrRight);
	if (target->Type == ST_DOOR || target->Type == ST_CONTAINER) {
		weaponRange += 10;
	}

	if (!(flags & AC_NO_SOUND) && !Sender->CurrentActionTicks && !core->GetGameControl()->InDialog()) {
		// play the battle cry
		// pick from all 5 possible verbal constants
		if (!attacker->PlayWarCry(5)) {
			// for monsters also try their 2da/ini file sounds
			if (!attacker->InParty) {
				ResRef sound;
				attacker->GetSoundFromFile(sound, 200U);
				core->GetAudioDrv()->Play(sound, SFX_CHAN_MONSTER, attacker->Pos);
			}
		}
		//display attack message
		if (target->GetGlobalID() != Sender->LastTarget) {
			displaymsg->DisplayConstantStringAction(HCStrings::ActionAttack, GUIColors::WHITE, Sender, target);
		}
	}

	double angle = AngleFromPoints(attacker->Pos, target->Pos);
	if (attacker->GetCurrentArea() != target->GetCurrentArea() ||
		!WithinPersonalRange(attacker, target, weaponRange) ||
		!attacker->GetCurrentArea()->IsVisibleLOS(attacker->Pos, target->Pos) ||
		!CanSee(attacker, target, true, 0)) {
		MoveNearerTo(attacker, target, Feet2Pixels(weaponRange, angle));
		return;
	} else if (target->Type == ST_DOOR) {
		//Forcing a lock does not launch the trap...
		Door* door = static_cast<Door*>(target);
		if(door->Flags & DOOR_LOCKED) {
			door->TryBashLock(attacker);
		}
		Sender->ReleaseCurrentAction();
		return;
	} else if (target->Type == ST_CONTAINER) {
		Container* cont = static_cast<Container*>(target);
		if(cont->Flags & CONT_LOCKED) {
			cont->TryBashLock(attacker);
		}
		Sender->ReleaseCurrentAction();
		return;
	}
	//action performed
	attacker->FaceTarget(target);

	Sender->LastTarget = target->GetGlobalID();
	Sender->LastTargetPersistent = Sender->LastTarget;
	attacker->PerformAttack(core->GetGame()->GameTime);
}

//we need this because some special characters like _ or * are also accepted
inline bool ismysymbol(const char letter)
{
	if (letter==']') return false;
	if (letter=='[') return false;
	if (letter==')') return false;
	if (letter=='(') return false;
	if (letter=='.') return false;
	if (letter==',') return false;
	return true;
}

//this function returns a value, symbol could be a numeric string or
//a symbol from idsname
static int GetIdsValue(const char *&symbol, const ResRef& idsname)
{
	char *newsymbol;
	int value = strtosigned<int>(symbol, &newsymbol);
	if (symbol!=newsymbol) {
		symbol=newsymbol;
		return value;
	}

	int idsfile = core->LoadSymbol(idsname);
	auto valHook = core->GetSymbol(idsfile);
	if (!valHook) {
		Log(ERROR, "GameScript", "Missing IDS file {} for symbol {}!", idsname, symbol);
		return -1;
	}

	char symbolname[64];
	int x;
	for (x=0;ismysymbol(*symbol) && x<(int) sizeof(symbolname)-1;x++) {
		symbolname[x]=*symbol;
		symbol++;
	}
	symbolname[x]=0;
	return valHook->GetValue(symbolname);
}

static int ParseIntParam(const char *&src, const char *&str)
{
	//going to the variable name
	while (*str != '*' && *str !=',' && *str != ')' ) {
		str++;
	}
	if (*str=='*') { //there may be an IDS table
		str++;
		ResRef idsTabName;
		char *cur = idsTabName.begin();
		const char *end = idsTabName.bufend();
		while (cur != end && *str != ',' && *str != ')') {
			*cur = *str;
			++cur;
			++str;
		}

		if (idsTabName[0]) {
			return GetIdsValue(src, idsTabName);
		}
	}
	//no IDS table
	return strtosigned<int>(src, (char **) &src);
}

static void ParseIdsTarget(const char *&src, Object *&object)
{
	for (int i=0;i<ObjectFieldsCount;i++) {
		object->objectFields[i]=GetIdsValue(src, ObjectIDSTableNames[i]);
		if (*src!='.') {
			break;
		}
		src++;
	}
	src++; //skipping ]
}

//this will skip to the next element in the prototype of an action/trigger
#define SKIP_ARGUMENT() while (*str && ( *str != ',' ) && ( *str != ')' )) str++

static void ParseObject(const char *&str,const char *&src, Object *&object)
{
	SKIP_ARGUMENT();
	object = new Object();
	switch (*src) {
	case ')':
		// missing parameter
		// (for example, StartDialogueNoSet() in aerie.d)
		Log(WARNING, "GSUtils", "ParseObject expected an object when parsing dialog");
		// replace with Myself
		object->objectFilters[0] = 1;
		break;
	case '"':
		//Scriptable Name
		src++;
		int i;
		for (i=0;i<(int) sizeof(object->objectName)-1 && *src && *src!='"';i++)
		{
			object->objectName[i] = *src;
			src++;
		}
		object->objectName[i] = 0;
		src++;
		break;
	case '[':
		src++; //skipping [
		ParseIdsTarget(src, object);
		break;
	default: //nested object filters
		int Nesting=0;

		while (Nesting<MaxObjectNesting) {
			memmove(object->objectFilters+1, object->objectFilters, (int) sizeof(int) *(MaxObjectNesting-1) );
			object->objectFilters[0]=GetIdsValue(src,"object");
			if (*src!='(') {
				break;
			}
			src++; //skipping (
			if (*src==')') {
				src++;
				break;
			}
			Nesting++;
		}
		if (*src=='[') {
			ParseIdsTarget(src, object);
		}
		src+=Nesting; //skipping )
	}
}

// some iwd2 dialogs use # instead of " for delimiting parameters (11phaen, 30gobpon, 11oswald)
// BUT at the same time, some bg2 mod prefixes use it too (eg. Tashia)
inline bool paramDelimiter(const char *src)
{
	return *src == '"' || (*src == '#' && (*(src-1) == '(' || *(src-1) == ',' || *(src+1) == ')'));
}

/* this function was lifted from GenerateAction, to make it clearer */
Action* GenerateActionCore(const char *src, const char *str, unsigned short actionID)
{
	Action *newAction = new Action(true);
	newAction->actionID = actionID;
	//this flag tells us to merge 2 consecutive strings together to get
	//a variable (context+variablename)
	int mergestrings = actionflags[newAction->actionID]&AF_MERGESTRINGS;
	int objectCount = (newAction->actionID == 1) ? 0 : 1; // only object 2 and 3 are used by actions, 1 being reserved for ActionOverride
	int stringsCount = 0;
	int intCount = 0;
	if (actionflags[newAction->actionID]&AF_DIRECT) {
		Object *tmp = new Object();
		tmp->objectFields[0] = -1;
		newAction->objects[objectCount++] = tmp;
	}
	//Here is the Action; Now we need to evaluate the parameters, if any
	if (*str!=')') while (*str) {
		if (*(str+1)!=':') {
			Log(WARNING, "GSUtils", "parser was sidetracked: {}", str);
		}
		switch (*str) {
			default:
				Log(WARNING, "GSUtils", "Invalid type: {}", str);
				delete newAction;
				return NULL;

			case 'p': //Point
				SKIP_ARGUMENT();
				src++; //Skip [
				newAction->pointParameter.x = strtosigned<int>( src, (char **) &src, 10);
				src++; //Skip .
				newAction->pointParameter.y = strtosigned<int>( src, (char **) &src, 10);
				src++; //Skip ]
				break;

			case 'i': //Integer
			{
				int value = ParseIntParam(src, str);
				if (!intCount) {
					newAction->int0Parameter = value;
				} else if (intCount == 1) {
					newAction->int1Parameter = value;
				} else {
					newAction->int2Parameter = value;
				}
				intCount++;
			}
			break;

			case 'a':
			// Action - only ActionOverride takes such a parameter
			{
				SKIP_ARGUMENT();
				char action[257];
				int i = 0;
				int openParenthesisCount = 0;
				while (true) {
					if (*src == ')') {
						if (!openParenthesisCount)
							break;
						openParenthesisCount--;
					} else {
						if (*src == '(') {
							openParenthesisCount++;
						} else {
							if (( *src == ',' ) &&
								!openParenthesisCount)
								break;
						}
					}
					action[i] = *src;
					i++;
					src++;
				}
				action[i] = 0;
				Action* act = GenerateAction( action);
				if (!act) {
					delete newAction;
					return NULL;
				}
				act->objects[0] = newAction->objects[0];
				newAction->objects[0] = NULL; //avoid freeing of object
				delete newAction; //freeing action
				newAction = act;
			}
			break;

			case 'o': //Object
				if (objectCount==3) {
					Log(ERROR, "GSUtils", "Invalid object count!");
					delete newAction;
					return NULL;
				}
				ParseObject(str, src, newAction->objects[objectCount++]);
				break;

			case 's': //String
			{
				SKIP_ARGUMENT();
				src++;
				int i;
				char* dst;
				if (!stringsCount) {
					dst = newAction->string0Parameter.begin();
				} else {
					dst = newAction->string1Parameter.begin();
				}
				//if there are 3 strings, the first 2 will be merged,
				//the last one will be left alone
				if (*str==')') {
					mergestrings = 0;
				}
				//skipping the context part, which
				//is to be readed later
				if (mergestrings) {
					for (i=0;i<6;i++) {
						*dst++='*';
					}
				}
				else {
					i=0;
				}
				//breaking on ',' in case of a monkey attack
				//fixes bg1:melicamp.dlg, bg1:sharte.dlg, bg2:udvith.dlg
				// NOTE: if strings ever need a , inside, this is will need to change
				while (*src != ',' && !paramDelimiter(src)) {
					if (*src == 0) {
						delete newAction;
						return NULL;
					}
					//sizeof(context+name) = 40
					if (i<40) {
						*dst++ = (char) tolower(*src);
						i++;
					}
					src++;
				}
				if (*src == '"' || *src == '#') {
					src++;
				}
				*dst = 0;
				//reading the context part
				if (mergestrings) {
					str++;
					if (*str!='s') {
						Log(ERROR, "GSUtils", "Invalid mergestrings: {}", str);
						delete newAction;
						return NULL;
					}
					SKIP_ARGUMENT();
					if (!stringsCount) {
						dst = newAction->string0Parameter.begin();
					} else {
						dst = newAction->string1Parameter.begin();
					}

					//this works only if there are no spaces
					if (*src++!=',' || *src++!='"') {
						break;
					}
					//reading the context string
					i=0;
					while (*src != '"') {
						if (*src == 0) {
							delete newAction;
							return NULL;
						}
						if (i++<6) {
							*dst++ = (char) tolower(*src);
						}
						src++;
					}
					src++; //skipping "
				}
				stringsCount++;
			}
			break;
		}
		str++;
		if (*src == ',' || *src==')')
			src++;
	}
	return newAction;
}

void MoveNearerTo(Scriptable *Sender, const Scriptable *target, int distance, int dont_release)
{
	Point p;

	Actor* mover = Scriptable::As<Actor>(Sender);
	if (!mover) {
		Log(ERROR, "GameScript", "MoveNearerTo only works with actors");
		Sender->ReleaseCurrentAction();
		return;
	}

	const Map *myarea = Sender->GetCurrentArea();
	const Map *hisarea = target->GetCurrentArea();
	if (hisarea && hisarea!=myarea) {
		target = myarea->GetTileMap()->GetTravelTo(hisarea->GetScriptRef());

		if (!target) {
			Log(WARNING, "GameScript", "MoveNearerTo failed to find an exit");
			Sender->ReleaseCurrentAction();
			return;
		}
		mover->UseExit(target->GetGlobalID());
	} else {
		mover->UseExit(0);
	}
	// we deliberately don't try GetLikelyPosition here for now,
	// maybe a future idea if we have a better implementation
	// (the old code used it - by passing true not 0 below - when target was a movable)
	GetPositionFromScriptable(target, p, false);

	// account for PersonalDistance (which caller uses, but pathfinder doesn't)
	if (distance) {
		distance += mover->CircleSize2Radius() * 4; // DistanceFactor
	}
	if (distance && target->Type == ST_ACTOR) {
		distance += static_cast<const Actor*>(target)->CircleSize2Radius() * 4;
	}

	MoveNearerTo(Sender, p, distance, dont_release);
}

//It is not always good to release the current action if target is unreachable
//we should also raise the trigger TargetUnreachable (if this is an Attack, at least)
//i hacked only this low level function, didn't need the higher ones so far
int MoveNearerTo(Scriptable *Sender, const Point &p, int distance, int dont_release)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Log(ERROR, "GameScript", "MoveNearerTo only works with actors");
		Sender->ReleaseCurrentAction();
		return 0;
	}

	// chasing is not unbreakable
	// would prevent smart ai from dropping a target that's running away
	//Sender->CurrentActionInterruptable = false;

	if (!actor->InMove() || actor->Destination != p) {
		ieDword flags = core->GetGameControl()->ShouldRun(actor) ? IF_RUNNING : 0;
		flags |= IF_NORETICLE;
		actor->WalkTo(p, flags, distance);
	}

	if (!actor->InMove()) {
		//didn't release
		if (dont_release) {
			return dont_release;
		}
		// we can't walk any nearer to destination, give up
		Sender->ReleaseCurrentAction();
	}
	return 0;
}

// checks the odd HasAdditionalRect / ADDITIONAL_RECT matching
// also returns true if the trigger is supposed to succeed
// formats supported by the originals:
// Normal (disabled) - If either the first or second parameter is negative.
// Point form: [x.y.range.unused] - If the fourth parameter is negative.
// Rect form: [left.top.right.bottom] (which we convert to a normal Region on load)
bool IsInObjectRect(const Point &pos, const Region &rect)
{
	if (!HasAdditionalRect) return true;
	if (rect.x < 0 || rect.y < 0) return true;
	if (rect.w <= 0) return true;

	// iwd2: testing shows the first point must be 0.0 for matching to work
	if (core->HasFeature(GFFlags::RULES_3ED) && !rect.origin.IsZero()) {
		return false;
	}

	// point or rect?
	if (rect.h <= 0) {
		unsigned int range = rect.w;
		return SquaredDistance(pos, rect.origin) <= range * range;
	} else {
		return rect.PointInside(pos);
	}
}

static Object *ObjectCopy(const Object *object)
{
	if (!object) return NULL;
	Object *newObject = new Object();
	memcpy(newObject->objectFields, object->objectFields, sizeof(newObject->objectFields));
	memcpy(newObject->objectFilters, object->objectFilters, sizeof(newObject->objectFilters));
	newObject->objectRect = object->objectRect;
	newObject->objectName = object->objectName;
	return newObject;
}

Action *ParamCopy(const Action *parameters)
{
	Action *newAction = new Action(true);
	newAction->actionID = parameters->actionID;
	newAction->int0Parameter = parameters->int0Parameter;
	newAction->int1Parameter = parameters->int1Parameter;
	newAction->int2Parameter = parameters->int2Parameter;
	newAction->pointParameter = parameters->pointParameter;
	newAction->string0Parameter = parameters->string0Parameter;
	newAction->string1Parameter = parameters->string1Parameter;
	for (int c=0;c<3;c++) {
		newAction->objects[c]= ObjectCopy( parameters->objects[c] );
	}
	return newAction;
}

Action *ParamCopyNoOverride(const Action *parameters)
{
	Action *newAction = new Action(true);
	newAction->actionID = parameters->actionID;
	newAction->int0Parameter = parameters->int0Parameter;
	newAction->int1Parameter = parameters->int1Parameter;
	newAction->int2Parameter = parameters->int2Parameter;
	newAction->pointParameter = parameters->pointParameter;
	newAction->string0Parameter = parameters->string0Parameter;
	newAction->string1Parameter = parameters->string1Parameter;
	newAction->objects[0]= NULL;
	newAction->objects[1]= ObjectCopy( parameters->objects[1] );
	newAction->objects[2]= ObjectCopy( parameters->objects[2] );
	return newAction;
}

Trigger *GenerateTriggerCore(const char *src, const char *str, int trIndex, int negate)
{
	Trigger *newTrigger = new Trigger();
	newTrigger->triggerID = (unsigned short) triggersTable->GetValueIndex( trIndex )&0x3fff;
	newTrigger->flags = (unsigned short) negate;
	int mergestrings = triggerflags[newTrigger->triggerID]&TF_MERGESTRINGS;
	int stringsCount = 0;
	int intCount = 0;
	//Here is the Trigger; Now we need to evaluate the parameters
	if (*str!=')') while (*str) {
		if (*(str+1)!=':') {
			Log(WARNING, "GSUtils", "parser was sidetracked: {}",str);
		}
		switch (*str) {
			default:
				Log(ERROR, "GSUtils", "Invalid type: {}", str);
				delete newTrigger;
				return NULL;

			case 'p': //Point
				SKIP_ARGUMENT();
				src++; //Skip [
				newTrigger->pointParameter.x = strtosigned<int>(src, (char **) &src, 10);
				src++; //Skip .
				newTrigger->pointParameter.y = strtosigned<int>(src, (char **) &src, 10);
				src++; //Skip ]
				break;

			case 'i': //Integer
			{
				int value = ParseIntParam(src, str);
				if (!intCount) {
					newTrigger->int0Parameter = value;
				} else if (intCount == 1) {
					newTrigger->int1Parameter = value;
				} else {
					newTrigger->int2Parameter = value;
				}
				intCount++;
			}
			break;

			case 'o': //Object
				ParseObject(str, src, newTrigger->objectParameter);
				break;

			case 's': //String
			{
				SKIP_ARGUMENT();
				src++;
				int i;
				char* dst;
				if (!stringsCount) {
					dst = newTrigger->string0Parameter.begin();
				} else {
					dst = newTrigger->string1Parameter.begin();
				}
				//skipping the context part, which
				//is to be readed later
				if (mergestrings) {
					for (i=0;i<6;i++) {
						*dst++='*';
					}
				}
				else {
					i=0;
				}
				// some iwd2 dialogs use # instead of " for delimiting parameters (11phaen)
				// BUT at the same time, some bg2 mod prefixes use it too (eg. Tashia)
				while (!paramDelimiter(src)) {
					if (*src == 0) {
						delete newTrigger;
						return NULL;
					}

					//sizeof(context+name) = 40
					if (i<40) {
						*dst++ = (char) tolower(*src);
						i++;
					}
					src++;
				}
				*dst = 0;
				//reading the context part
				if (mergestrings) {
					str++;
					if (*str!='s') {
						Log(ERROR, "GSUtils", "Invalid mergestrings 2: {}", str);
						delete newTrigger;
						return NULL;
					}
					SKIP_ARGUMENT();
					if (!stringsCount) {
						dst = newTrigger->string0Parameter.begin();
					} else {
						dst = newTrigger->string1Parameter.begin();
					}

					//this works only if there are no spaces
					if (*src++!='"' || *src++!=',' || *src++!='"') {
						break;
					}
					//reading the context string
					i=0;
					while (*src != '"' && (*src != '#' || (*(src-1) != '(' && *(src-1) != ','))) {
						if (*src == 0) {
							delete newTrigger;
							return NULL;
						}

						if (i++<6) {
							*dst++ = (char) tolower(*src);
						}
						src++;
					}
				}
				src++; //skipping "
				stringsCount++;
			}
			break;
		}
		str++;
		if (*src == ',' || *src==')')
			src++;
	}
	return newTrigger;
}

void SetVariable(Scriptable* Sender, const StringParam& VarName, ieDword value, VarContext context)
{
	ResRef key{VarName};

	auto SetLocalVariable = [=](ieVarsMap& vars, const ResRef& key, ieDword value) {
		auto lookup = vars.find(key);

		if (lookup != vars.cend()) {
			lookup->second = value;
		} else if (!NoCreate) {
			vars[key] = value;
		}
	};

	if (context.IsEmpty()) {
		const char* varName = &VarName[6];
		//some HoW triggers use a : to separate the scope from the variable name
		if (*varName == ':') {
			varName++;
		}
		context.Format("{:.6}", VarName);
		key = ResRef{varName};
	}
	ScriptDebugLog(ID_VARIABLES, "Setting variable(\"{}{}\", {})", context, VarName, value);

	if (context == "MYAREA") {
		SetLocalVariable(Sender->GetCurrentArea()->locals, key, value);
		return;
	}
	if (context == "LOCALS") {
		SetLocalVariable(Sender->locals, key, value);
		return;
	}
	Game *game = core->GetGame();
	if (HasKaputz && context == "KAPUTZ") {
		SetLocalVariable(game->kaputz, key, value);
		return;
	}
	if (context == "GLOBAL") {
		SetLocalVariable(game->locals, key, value);
	} else {
		// map name context, eg. AR1324
		Map* map = game->GetMap(game->FindMap(context));
		if (map) {
			SetLocalVariable(map->locals, key, value);
		} else if (core->InDebugMode(ID_VARIABLES)) {
			Log(WARNING, "GameScript", "Invalid variable {} {} in SetVariable", context, VarName);
		}
	}
}

void SetPointVariable(Scriptable *Sender, const StringParam& VarName, const Point &p, const VarContext& Context)
{
	SetVariable(Sender, VarName, ((p.y & 0xFFFF) << 16) | (p.x & 0xFFFF), Context);
}

ieDword CheckVariable(const Scriptable *Sender, const StringParam& VarName, VarContext context, bool *valid)
{
	ResRef key{VarName};

	auto GetLocalVariable = [](const ieVarsMap& vars, VarContext context, const ResRef& key) -> ieDword {
		auto lookup = vars.find(key);
		if (lookup != vars.cend()) {
			ScriptDebugLog(ID_VARIABLES, "CheckVariable {}{}: {}", context, key, lookup->second);
			return lookup->second;
		}

		return 0;
	};

	if (context.IsEmpty()) {
		const char* varName = &VarName[6];
		//some HoW triggers use a : to separate the scope from the variable name
		if (*varName == ':') {
			varName++;
		}
		context.Format("{:.6}", VarName);
		key = ResRef{varName};
	}
	
	if (context == "MYAREA") {
		return GetLocalVariable(Sender->GetCurrentArea()->locals, context, key);
	}
	
	if (context == "LOCALS") {
		return GetLocalVariable(Sender->locals, context, key);
	}
	
	const Game *game = core->GetGame();
	if (HasKaputz && context == "KAPUTZ") {
		return GetLocalVariable(game->kaputz, context, key);
	}
	
	if (context == "GLOBAL") {
		return GetLocalVariable(game->locals, context, key);
	} else {
		// map name context, eg. AR1324
		const Map* map = game->GetMap(game->FindMap(context));
		if (map) {
			return GetLocalVariable(map->locals, context, key);
		} else {
			if (valid) *valid = false;
			ScriptDebugLog(ID_VARIABLES, "Invalid variable {} {} in checkvariable", context, VarName);
		}
	}

	return 0;
}

Point CheckPointVariable(const Scriptable *Sender, const StringParam& VarName, const VarContext& Context, bool *valid)
{
	ieDword val = CheckVariable(Sender, VarName, Context, valid);
	return Point(val & 0xFFFF, val >> 16);
}

// checks if a variable exists in any context
bool VariableExists(const Scriptable *Sender, const StringParam& VarName, const VarContext& context)
{
	const Game *game = core->GetGame();

	auto hasLocalVariable = [](const ieVarsMap& vars, const StringParam& key) -> bool {
		return vars.find(key) != vars.cend();
	};

	if (hasLocalVariable(Sender->GetCurrentArea()->locals, VarName)) {
		return true;
	} else if (hasLocalVariable(Sender->locals, VarName)) {
		return true;
	} else if (HasKaputz && hasLocalVariable(game->kaputz, VarName)) {
		return true;
	} else if (hasLocalVariable(game->locals, VarName)) {
		return true;
	} else {
		const Map* map = game->GetMap(game->FindMap(context));
		if (map && hasLocalVariable(map->locals, VarName)) {
			return true;
		}
	}
	return false;
}

bool DiffCore(ieDword a, ieDword b, int diffMode)
{
	switch (diffMode) {
		case LESS_THAN:
			return a < b;
		case EQUALS:
			return a == b;
		case GREATER_THAN:
			return a > b;
		case GREATER_OR_EQUALS:
			return a >= b;
		case NOT_EQUALS:
			return a != b;
		case BINARY_LESS_OR_EQUALS:
			return (a & b) == a;
		case BINARY_MORE:
			return (a & b) != a;
		case BINARY_MORE_OR_EQUALS:
			return (a & b) == b;
		case BINARY_LESS:
			return (a & b) != b;
		case BINARY_INTERSECT:
			return static_cast<bool>(a & b);
		case BINARY_NOT_INTERSECT:
			return !(a & b);
		default: //less or equals
			return a <= b;
	}
}

int GetGroup(const Actor *actor)
{
	int type = 2; //neutral, has no enemies
	if (actor->GetStat(IE_EA) <= EA_GOODCUTOFF) {
		type = 1; //PC
	}
	else if (actor->GetStat(IE_EA) >= EA_EVILCUTOFF) {
		type = 0;
	}
	return type;
}

Actor *GetNearestEnemyOf(const Map *map, const Actor *origin, int whoseeswho)
{
	//determining the allegiance of the origin
	int type = GetGroup(origin);

	//neutral has no enemies
	if (type==2) {
		return NULL;
	}

	Targets *tgts = new Targets();

	int i = map->GetActorCount(true);
	Actor *ac;
	while (i--) {
		ac=map->GetActor(i,true);
		if (ac == origin) continue;

		if (whoseeswho&ENEMY_SEES_ORIGIN) {
			if (!CanSee(ac, origin, true, GA_NO_DEAD|GA_NO_UNSCHEDULED)) {
				continue;
			}
		}
		if (whoseeswho&ORIGIN_SEES_ENEMY) {
			if (!CanSee(ac, origin, true, GA_NO_DEAD|GA_NO_UNSCHEDULED)) {
				continue;
			}
		}

		int distance = Distance(ac, origin);
		if (type) { //origin is PC
			if (ac->GetStat(IE_EA) >= EA_EVILCUTOFF) {
				tgts->AddTarget(ac, distance, GA_NO_DEAD|GA_NO_UNSCHEDULED);
			}
		}
		else {
			if (ac->GetStat(IE_EA) <= EA_GOODCUTOFF) {
				tgts->AddTarget(ac, distance, GA_NO_DEAD|GA_NO_UNSCHEDULED);
			}
		}
	}
	ac = static_cast<Actor*>(tgts->GetTarget(0, ST_ACTOR));
	delete tgts;
	return ac;
}

Actor *GetNearestOf(const Map *map, const Actor *origin, int whoseeswho)
{
	Targets *tgts = new Targets();

	int i = map->GetActorCount(true);
	Actor *ac;
	while (i--) {
		ac=map->GetActor(i,true);
		if (ac == origin) continue;

		if (whoseeswho&ENEMY_SEES_ORIGIN) {
			if (!CanSee(ac, origin, true, GA_NO_DEAD|GA_NO_UNSCHEDULED)) {
				continue;
			}
		}
		if (whoseeswho&ORIGIN_SEES_ENEMY) {
			if (!CanSee(ac, origin, true, GA_NO_DEAD|GA_NO_UNSCHEDULED)) {
				continue;
			}
		}

		int distance = Distance(ac, origin);
		tgts->AddTarget(ac, distance, GA_NO_DEAD|GA_NO_UNSCHEDULED);
	}
	ac = static_cast<Actor*>(tgts->GetTarget(0, ST_ACTOR));
	delete tgts;
	return ac;
}

Point GetEntryPoint(const ResRef& areaname, const ResRef& entryname)
{
	AutoTable tab = gamedata->LoadTable("entries");
	if (!tab) {
		return {};
	}
	const char *tmpstr = tab->QueryField(areaname, entryname).c_str();
	Point p;
	sscanf(tmpstr, "%d.%d", &p.x, &p.y);
	return p;
}

/* returns a spell's casting distance, it depends on the caster (level), and targeting mode too
 the used header is calculated from the caster level */
unsigned int GetSpellDistance(const ResRef& spellRes, Scriptable* Sender, const Point& target)
{
	unsigned int dist;

	const Spell* spl = gamedata->GetSpell(spellRes);
	if (!spl) {
		Log(ERROR, "GameScript", "Spell couldn't be found: {}.", spellRes);
		return 0;
	}
	dist = spl->GetCastingDistance(Sender);
	gamedata->FreeSpell(spl, spellRes, false);

	//make possible special return values (like 0xffffffff means the spell doesn't need distance)
	//this is used with special targeting mode (3)
	if (dist>0xff000000) {
		return 0xffffffff;
	}

	if (!target.IsZero()) {
		double angle = AngleFromPoints(Sender->Pos, target);
		return Feet2Pixels(dist, angle);
	}

	// none of the callers not passing a target care about this
	return 0;
}

/* returns an item's casting distance, it depends on the used header, and targeting mode too
 the used header is explictly given */
unsigned int GetItemDistance(const ResRef& itemres, int header, double angle)
{
	const Item* itm = gamedata->GetItem(itemres);
	if (!itm) {
		Log(ERROR, "GameScript", "Item couldn't be found: {}.", itemres);
		return 0;
	}
	unsigned int dist = itm->GetCastingDistance(header);
	gamedata->FreeItem(itm, itemres, false);

	//make possible special return values (like 0xffffffff means the item doesn't need distance)
	//this is used with special targeting mode (3)
	if (dist>0xff000000) {
		return 0xffffffff;
	}

	return Feet2Pixels(dist, angle);
}

//read the wish 2da
void SetupWishCore(Scriptable *Sender, TableMgr::index_t column, int picks)
{
	// in the original, picks was at first the number of wish choices to set up,
	// but then it was hard coded to 5 (and SetupWishObject disused)
	if (picks == 1) picks = 5;

	AutoTable tm = gamedata->LoadTable("wish");
	if (!tm) {
		Log(ERROR, "GameScript", "Cannot find wish.2da.");
		return;
	}

	std::vector<int> selects(picks, -1);
	int count = tm->GetRowCount();
	// handle the unused SetupWishObject, which passes WIS instead of a column
	// just cutting the 1-25 range into four pieces (roughly how the djinn dialog works)
	TableMgr::index_t cols = tm->GetColumnCount();
	if (column > cols) {
		column = (column-1)/6;
		if (column == 4) column = RAND(0, 3);
	}

	ieVariable varname;
	for (int i = 0; i < 99; i++) {
		varname.Format("wishpower{:02d}", i);
		if(CheckVariable(Sender, varname, "GLOBAL") ) {
			SetVariable(Sender, varname, 0, "GLOBAL");
		}
	}

	if (count<picks) {
		for (int i = 0; i < count; i++) {
			selects[i]=i;
		}
	} else {
		for (int i = 0; i < picks; i++) {
			selects[i]=RAND(0, count-1);

			int j = 0;
			while (j < i) {
				if (selects[i] == selects[j]) {
					selects[i]++;
					j = 0; // retry from the start
					continue;
				}
				j++;
			}
		}
	}

	for (int i = 0; i < picks; i++) {
		if (selects[i] < 0) continue;
		std::string cell = tm->QueryField(selects[i], column - 1);
		if (cell == "*") continue;
		int spnum = atoi(cell.c_str());
		varname.Format("wishpower{:02d}", spnum);
		SetVariable(Sender, varname, 1, "GLOBAL");
	}
}

void AmbientActivateCore(const Scriptable *Sender, const Action *parameters, bool flag)
{
	AreaAnimation* anim = Sender->GetCurrentArea( )->GetAnimation(parameters->variable0Parameter);
	if (!anim) {
		anim = Sender->GetCurrentArea( )->GetAnimation( parameters->objects[1]->objectNameVar );
	}
	if (!anim) {
		// iwd2 expects this behaviour in ar6001 by (de)activating sound_portal
		AmbientMgr *ambientmgr = core->GetAudioDrv()->GetAmbientMgr();
		if (flag) {
			ambientmgr->Activate(parameters->objects[1]->objectName);
		} else {
			ambientmgr->Deactivate(parameters->objects[1]->objectName);
		}
		return;
	}

	BitOp op = flag ? BitOp::OR : BitOp::NAND;
	SetBits<ieDword>(anim->Flags, A_ANI_ACTIVE, op);
	for (size_t i = 0; i < anim->animation.size(); ++i) {
		SetBits<ieDword>(anim->animation[i].Flags, A_ANI_ACTIVE, op);
	}
}

#define MAX_ISLAND_POLYGONS  10

//read a polygon 2da
Gem_Polygon *GetPolygon2DA(ieDword index)
{
	ResRef resRef;

	if (index>=MAX_ISLAND_POLYGONS) {
		return NULL;
	}

	if (!polygons) {
		polygons = (Gem_Polygon **) calloc(MAX_ISLAND_POLYGONS, sizeof(Gem_Polygon *) );
	}
	if (polygons[index]) {
		return polygons[index];
	}
	resRef.Format("ISLAND{:02d}", index);
	AutoTable tm = gamedata->LoadTable(resRef);
	if (!tm) {
		return NULL;
	}
	TableMgr::index_t cnt = tm->GetRowCount();
	if (!cnt) {
		return NULL;
	}
	
	std::vector<Point> p(cnt);
	while(cnt--) {
		p[cnt].x = tm->QueryFieldSigned<int>(cnt, 0);
		p[cnt].y = tm->QueryFieldSigned<int>(cnt, 1);
	}

	polygons[index] = new Gem_Polygon(std::move(p), nullptr);
	return polygons[index];
}

static bool InterruptSpellcasting(Scriptable* Sender) {
	Actor* caster = Scriptable::As<Actor>(Sender);
	if (!caster) return false;

	// ouch, we got hit
	if (Sender->InterruptCasting) {
		if (caster->InParty) {
			displaymsg->DisplayConstantString(HCStrings::SpellDisrupted, GUIColors::WHITE, Sender);
		} else {
			displaymsg->DisplayConstantStringName(HCStrings::SpellFailed, GUIColors::WHITE, Sender);
		}
		DisplayStringCoreVC(Sender, VB_SPELL_DISRUPTED, DS_CONSOLE);
		return true;
	}

	// abort casting on invisible or dead targets
	// not all spells should be interrupted on death - some for chunking, some for raising the dead
	if (Sender->LastSpellTarget) {
		const Actor *target = core->GetGame()->GetActorByGlobalID(Sender->LastSpellTarget);
		if (!target) return false; // shouldn't happen, though perhaps it would be better to return true

		ieDword state = target->GetStat(IE_STATE_ID);
		if (!(state & STATE_DEAD) || (state & ~(STATE_PETRIFIED | STATE_FROZEN)) != state) {
			return false;
		}

		const Spell* spl = gamedata->GetSpell(Sender->SpellResRef, true);
		if (!spl) return false;

		const SPLExtHeader *seh = spl->GetExtHeader(0); // potentially wrong, but none of the existing spells is problematic
		bool invalidTarget = seh && seh->Target != TARGET_DEAD;
		gamedata->FreeSpell(spl, Sender->SpellResRef, false);
		if (!invalidTarget) return false;

		if (caster->InParty) {
			core->Autopause(AUTOPAUSE::NOTARGET, caster);
		}
		caster->SetStance(IE_ANI_READY);
		return true;
	}
	return false;
}

// shared spellcasting action code for casting on scriptables
void SpellCore(Scriptable *Sender, Action *parameters, int flags)
{
	ResRef spellResRef;
	int level = 0;
	static bool third = core->HasFeature(GFFlags::RULES_3ED);

	// handle iwd2 marked spell casting (MARKED_SPELL is 0)
	// NOTE: supposedly only casting via SpellWait checks this, so refactor if needed
	if (third && parameters->int0Parameter == 0 && parameters->resref0Parameter.IsEmpty()) {
		if (!Sender->LastMarkedSpell) {
			// otherwise we spam a lot
			Sender->ReleaseCurrentAction();
			return;
		}
		ResolveSpellName(spellResRef, Sender->LastMarkedSpell);
	}

	//resolve spellname
	if (spellResRef.IsEmpty() && !ResolveSpellName(spellResRef, parameters)) {
		Sender->ReleaseCurrentAction();
		return;
	} else {
		if (Sender->SpellResRef.IsEmpty() || Sender->SpellResRef != spellResRef) {
			if (Sender->CurrentActionTicks) {
				Log(WARNING, "GameScript", "SpellCore: Action ({}) lost spell somewhere!", parameters->actionID);
			}
			Sender->SetSpellResRef(spellResRef);
		}
	}
	if (!Sender->CurrentActionTicks) {
		parameters->int2Parameter = 1;
	}

	// use the passed level instead of the caster's casting level
	if (flags&SC_SETLEVEL) {
		if (!parameters->resref0Parameter.IsEmpty()) {
			level = parameters->int0Parameter;
		} else {
			level = parameters->int1Parameter;
		}
	}

	Actor* act = Scriptable::As<Actor>(Sender);
	// handle also held enemies, since their scripts still run
	if (act && act->Immobile()) {
		Sender->ReleaseCurrentAction();
		return;
	}

	//parse target
	int seeflag = 0;
	unsigned int dist = GetSpellDistance(spellResRef, Sender);
	if ((flags&SC_NO_DEAD) && dist != 0xffffffff) {
		seeflag = GA_NO_DEAD;
	}

	Scriptable* tar = GetStoredActorFromObject( Sender, parameters->objects[1], seeflag );
	if (!tar) {
		parameters->int2Parameter = 0;
		Sender->ReleaseCurrentAction();
		if (act) {
			act->SetStance(IE_ANI_READY);
		}
		return;
	}
	dist = GetSpellDistance(spellResRef, Sender, tar->Pos);

	if (act) {
		//move near to target
		if ((flags&SC_RANGE_CHECK) && dist != 0xffffffff) {
			if (PersonalDistance(tar, Sender) > dist) {
				MoveNearerTo(Sender, tar, dist);
				return;
			}
			if (!Sender->GetCurrentArea()->IsVisibleLOS(Sender->Pos, tar->Pos)) {
				const Spell *spl = gamedata->GetSpell(Sender->SpellResRef, true);
				if (!(spl->Flags&SF_NO_LOS)) {
					gamedata->FreeSpell(spl, Sender->SpellResRef, false);
					MoveNearerTo(Sender, tar, dist);
					return;
				}
				gamedata->FreeSpell(spl, Sender->SpellResRef, false);
			}

			// finish approach before continuing with casting
			if (act->InMove()) return;
		}

		//face target
		if (tar != Sender) {
			act->SetOrientation(tar->Pos, act->Pos, false);
		}

		//stop doing anything else
		act->SetModal(MS_NONE);
	}

	if ((flags&SC_AURA_CHECK) && parameters->int2Parameter && Sender->AuraPolluted()) {
		return;
	}

	// mark as uninterruptible in the action sense, so further script
	// updates don't remove the action before the casting is done
	// the originals or at least iwd2 even marked it as IF_NOINT,
	// so it also guarded against the ClearActions action
	// ... but just for the actual spellcasting part
	Sender->CurrentActionInterruptable = false;

	int duration;
	if (!parameters->int2Parameter) {
		duration = Sender->CurrentActionState--;
	} else {
		duration = Sender->CastSpell(tar, flags & SC_DEPLETE, flags & SC_INSTANT, flags & SC_NOINTERRUPT, level);
	}
	if (duration == -1) {
		// some kind of error
		parameters->int2Parameter = 0;
		Sender->ReleaseCurrentAction();
		return;
	} else if (duration > 0) {
		if (parameters->int2Parameter) {
			Sender->CurrentActionState = duration;
			parameters->int2Parameter = 0;
		}
		if (!(flags&SC_NOINTERRUPT) && InterruptSpellcasting(Sender)) {
			parameters->int2Parameter = 0;
			Sender->ReleaseCurrentAction();
		}
		return;
	}
	if (!(flags&SC_NOINTERRUPT) && InterruptSpellcasting(Sender)) {
		parameters->int2Parameter = 0;
		Sender->ReleaseCurrentAction();
		return;
	}

	if (Sender->LastSpellTarget) {
		//if target was set, fire spell
		Sender->CastSpellEnd(level, flags&SC_INSTANT);
	} else if(!Sender->LastTargetPos.IsInvalid()) {
		//the target was converted to a point
		Sender->CastSpellPointEnd(level, flags&SC_INSTANT);
	} else {
		Log(ERROR, "GameScript", "SpellCore: Action ({}) lost target somewhere!", parameters->actionID);
	}
	parameters->int2Parameter = 0;
	Sender->ReleaseCurrentAction();
}


// shared spellcasting action code for casting on the ground
void SpellPointCore(Scriptable *Sender, Action *parameters, int flags)
{
	ResRef spellResRef;
	int level = 0;

	//resolve spellname
	if (!ResolveSpellName(spellResRef, parameters)) {
		Sender->ReleaseCurrentAction();
		return;
	} else {
		if (Sender->SpellResRef.IsEmpty() || Sender->SpellResRef != spellResRef) {
			if (Sender->CurrentActionTicks) {
				Log(WARNING, "GameScript", "SpellPointCore: Action ({}) lost spell somewhere!", parameters->actionID);
			}
			Sender->SetSpellResRef(spellResRef);
		}
	}
	if (!Sender->CurrentActionTicks) {
		parameters->int2Parameter = 1;
	}

	// use the passed level instead of the caster's casting level
	if (flags&SC_SETLEVEL) {
		if (!parameters->resref0Parameter.IsEmpty()) {
			level = parameters->int0Parameter;
		} else {
			level = parameters->int1Parameter;
		}
	}

	Actor* act = Scriptable::As<Actor>(Sender);
	if (act) {
		// handle also held enemies, since their scripts still run
		if (act->Immobile()) {
			Sender->ReleaseCurrentAction();
			return;
		}

		//move near to target
		if (flags&SC_RANGE_CHECK) {
			unsigned int dist = GetSpellDistance(spellResRef, Sender, parameters->pointParameter);
			if (PersonalDistance(parameters->pointParameter, Sender) > dist) {
				MoveNearerTo(Sender, parameters->pointParameter, dist, 0);
				return;
			}
			if (!Sender->GetCurrentArea()->IsVisibleLOS(Sender->Pos, parameters->pointParameter)) {
				const Spell *spl = gamedata->GetSpell(Sender->SpellResRef, true);
				if (!(spl->Flags&SF_NO_LOS)) {
					gamedata->FreeSpell(spl, Sender->SpellResRef, false);
					MoveNearerTo(Sender, parameters->pointParameter, dist, 0);
					return;
				}
				gamedata->FreeSpell(spl, Sender->SpellResRef, false);
			}

			// finish approach before continuing with casting
			if (act->InMove()) return;
		}

		//face target
		act->SetOrientation(parameters->pointParameter, act->Pos, false);
		//stop doing anything else
		act->SetModal(MS_NONE);
	}

	if ((flags&SC_AURA_CHECK) && parameters->int2Parameter && Sender->AuraPolluted()) {
		return;
	}

	// mark as uninterruptible in the action sense, so further script
	// updates don't remove the action before the casting is done
	// the originals or at least iwd2 even marked it as IF_NOINT,
	// so it also guarded against the ClearActions action
	// ... but just for the actual spellcasting part
	Sender->CurrentActionInterruptable = false;

	int duration;
	if (!parameters->int2Parameter) {
		duration = Sender->CurrentActionState--;
	} else {
		duration = Sender->CastSpellPoint(parameters->pointParameter, flags & SC_DEPLETE, flags & SC_INSTANT, flags & SC_NOINTERRUPT, level);
	}
	if (duration == -1) {
		// some kind of error
		Sender->ReleaseCurrentAction();
		return;
	} else if (duration > 0) {
		if (parameters->int2Parameter) {
			Sender->CurrentActionState = duration;
			parameters->int2Parameter = 0;
		}
		if (!(flags&SC_NOINTERRUPT) && InterruptSpellcasting(Sender)) {
			Sender->ReleaseCurrentAction();
		}
		return;
	}
	if (!(flags&SC_NOINTERRUPT) && InterruptSpellcasting(Sender)) {
		Sender->ReleaseCurrentAction();
		return;
	}

	if(!Sender->LastTargetPos.IsInvalid()) {
		//if target was set, fire spell
		Sender->CastSpellPointEnd(level, flags&SC_INSTANT);
	} else {
		Log(ERROR, "GameScript", "SpellPointCore: Action ({}) lost target somewhere!", parameters->actionID);
	}
	Sender->ReleaseCurrentAction();
}

void AddXPCore(const Action *parameters, bool divide)
{
	AutoTable xptable;

	if (core->HasFeature(GFFlags::HAS_EXPTABLE)) {
		xptable = gamedata->LoadTable("exptable");
	} else {
		xptable = gamedata->LoadTable("xplist");
	}

	if (parameters->int0Parameter > 0 && core->HasFeedback(FT_MISC)) {
		displaymsg->DisplayString(ieStrRef(parameters->int0Parameter), GUIColors::XPCHANGE, STRING_FLAGS::SOUND);
	}
	if (!xptable) {
		Log(ERROR, "GameScript", "Can't perform AddXP2DA/AddXPVar!");
		return;
	}
	const char *xpvalue = xptable->QueryField(parameters->string0Parameter, "0").c_str(); // level is unused

	if (divide) {
		// force divide party xp
		core->GetGame()->ShareXP(atoi(xpvalue), SX_DIVIDE);
	} else if (xpvalue[0] == 'P' && xpvalue[1] == '_') {
		// divide party xp
		core->GetGame()->ShareXP(atoi(xpvalue+2), SX_DIVIDE);
	} else {
		// give xp to everyone
		core->GetGame()->ShareXP(atoi(xpvalue), 0);
	}
	core->PlaySound(DS_GOTXP, SFX_CHAN_ACTIONS);
}

int NumItemsCore(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable* target = GetScriptableFromObject(Sender, parameters->objectParameter);
	if (!target) {
		return 0;
	}

	const Inventory *inventory = nullptr;
	if (target->Type == ST_ACTOR) {
		inventory = &(static_cast<const Actor*>(target)->inventory);
	} else if (target->Type == ST_CONTAINER) {
		inventory = &(static_cast<const Container*>(target)->inventory);
	}
	if (!inventory) {
		return 0;
	}

	return inventory->CountItems(parameters->resref0Parameter, true);
}

static EffectRef fx_level_bounce_ref = { "Bounce:SpellLevel", -1 };
static EffectRef fx_level_bounce_dec_ref = { "Bounce:SpellLevelDec", -1 };
unsigned int NumBouncingSpellLevelCore(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable* target = GetScriptableFromObject(Sender, parameters->objectParameter);
	const Actor* actor = Scriptable::As<Actor>(target);
	if (!actor) {
		return 0;
	}

	unsigned int bounceCount = 0;
	if (actor->fxqueue.HasEffectWithPower(fx_level_bounce_ref, parameters->int0Parameter)) {
		bounceCount = 0xFFFFFFFF;
	} else {
		const Effect *fx = actor->fxqueue.HasEffectWithPower(fx_level_bounce_dec_ref, parameters->int0Parameter);
		if (fx) {
			bounceCount = fx->Parameter1;
		}
	}

	return bounceCount;
}

static EffectRef fx_level_immunity_ref = { "Protection:Spelllevel", -1 };
static EffectRef fx_level_immunity_dec_ref = { "Protection:SpellLevelDec", -1 };
int NumImmuneToSpellLevelCore(Scriptable *Sender, const Trigger *parameters)
{
	const Scriptable* target = GetScriptableFromObject(Sender, parameters->objectParameter);
	const Actor* actor = Scriptable::As<Actor>(target);
	if (!actor) {
		return 0;
	}

	unsigned int bounceCount = 0;
	if (actor->fxqueue.HasEffectWithPower(fx_level_immunity_ref, parameters->int0Parameter)) {
		bounceCount = 0xFFFFFFFF;
	} else {
		const Effect *fx = actor->fxqueue.HasEffectWithPower(fx_level_immunity_dec_ref, parameters->int0Parameter);
		if (fx) {
			bounceCount = fx->Parameter1;
		}
	}

	return bounceCount;
}

void RunAwayFromCore(Scriptable* Sender, const Action* parameters, int flags)
{
	Actor* actor = Scriptable::As<Actor>(Sender);
	if (!actor) {
		Sender->ReleaseCurrentAction();
		return;
	}
	// Avenger: I believe being dead still interrupts RunAwayFromNoInterrupt
	if (Sender->GetInternalFlag() & IF_STOPATTACK) {
		Sender->ReleaseCurrentAction();
		return;
	}

	// pst RunAwayFromEx uses int1Parameter to denote whether to run away regardless of new threats
	// so basically RunAwayFromNoInterrupt when it's true, since pst lacked it (even though it's present in bg1)
	if (parameters->int1Parameter) {
		flags |= RunAwayFlags::NoInterrupt;
	}

	// already fleeing or just about to end?
	if (Sender->CurrentActionState > 0) {
		Sender->CurrentActionState--;
		return;
	} else if (Sender->CurrentActionTicks > 0) {
		if (flags & RunAwayFlags::NoInterrupt) {
			actor->Interrupt();
		}
		Sender->ReleaseCurrentAction();
		return;
	}

	// start fleeing
	Sender->CurrentActionState = parameters->int0Parameter;

	Point start = parameters->pointParameter;
	if (!(flags & RunAwayFlags::UsePoint)) {
		const Scriptable* tar = GetStoredActorFromObject(Sender, parameters->objects[1]);
		if (!tar) {
			Sender->ReleaseCurrentAction();
			return;
		}
		start = tar->Pos;
	}

	// estimate max distance with time and actor speed
	double speed = actor->GetSpeed();
	int maxDistance = parameters->int0Parameter;
	if (speed) {
		maxDistance = static_cast<int>(maxDistance * gamedata->GetStepTime() / speed);
	}

	if (flags & RunAwayFlags::NoInterrupt) { // should we just mark the action as CurrentActionInterruptable = false instead?
		actor->NoInterrupt();
	}

	//TODO: actor could use travel areas (if flags & RAF_LEAVE_AREA)
	actor->RunAwayFrom(start, maxDistance, false);

	// TODO: only reset morale when moving to a new area (like the original)
	if (flags & RunAwayFlags::LeaveArea && actor->ShouldModifyMorale()) {
		actor->NewBase(IE_MORALE, 20, MOD_ABSOLUTE);
	}
}

void MoveGlobalObjectCore(Scriptable* Sender, const Action* parameters, int flags)
{
	Scriptable* tar = GetScriptableFromObject(Sender, parameters->objects[1]);
	Actor* actor = Scriptable::As<Actor>(tar);
	if (!actor) {
		return;
	}
	const Scriptable* to = GetScriptableFromObject(Sender, parameters->objects[2]);
	if (!to) return;
	const Map* map = to->GetCurrentArea();
	if (!map) return;

	Point dest = to->Pos;
	if (flags & CC_OFFSCREEN) {
		dest = FindOffScreenPoint(to, CC_OBJECT, 0);
		if (dest.IsZero()) {
			dest = FindOffScreenPoint(to, CC_OBJECT, 1);
		}
	}

	if (actor->Persistent() || !CreateMovementEffect(actor, map->GetScriptRef(), dest, 0)) {
		MoveBetweenAreasCore(actor, map->GetScriptRef(), dest, -1, true);
	}
}

}
