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
#include "Video.h"
#include "WorldMap.h"
#include "GUI/GameControl.h"
#include "RNG.h"
#include "Scriptable/Container.h"
#include "Scriptable/Door.h"
#include "Scriptable/InfoPoint.h"
#include "System/StringBuffer.h"

#include <cmath>
#include <cstdio>

namespace GemRB {

//these tables will get freed by Core
Holder<SymbolMgr> triggersTable;
Holder<SymbolMgr> actionsTable;
Holder<SymbolMgr> overrideTriggersTable;
Holder<SymbolMgr> overrideActionsTable;
Holder<SymbolMgr> objectsTable;
TriggerFunction triggers[MAX_TRIGGERS];
ActionFunction actions[MAX_ACTIONS];
short actionflags[MAX_ACTIONS];
short triggerflags[MAX_TRIGGERS];
ObjectFunction objects[MAX_OBJECTS];
IDSFunction idtargets[MAX_OBJECT_FIELDS];
Cache SrcCache; //cache for string resources (pst)
Cache BcsCache; //cache for scripts
int ObjectIDSCount = 7;
int MaxObjectNesting = 5;
bool HasAdditionalRect = false;
bool HasTriggerPoint = false;
//don't create new variables
bool NoCreate = false;
bool HasKaputz = false;
//released by ReleaseMemory
ieResRef *ObjectIDSTableNames;
int ObjectFieldsCount = 7;
int ExtraParametersCount = 0;
int InDebug = 0;
int RandomNumValue;
// reaction modifiers (by reputation and charisma)
#define MAX_REP_COLUMN 20
#define MAX_CHR_COLUMN 25
int rmodrep[MAX_REP_COLUMN];
int rmodchr[MAX_CHR_COLUMN];
int happiness[3][MAX_REP_COLUMN];
Gem_Polygon **polygons;

void InitScriptTables()
{
	//initializing the happiness table
	{
	AutoTable tab("happy");
	if (tab) {
		for (int alignment=0;alignment<3;alignment++) {
			for (int reputation=0;reputation<MAX_REP_COLUMN;reputation++) {
				happiness[alignment][reputation]=strtol(tab->QueryField(reputation,alignment), NULL, 0);
			}
		}
	}
	}

	//initializing the reaction mod. reputation table
	AutoTable rmr("rmodrep");
	if (rmr) {
		for (int reputation=0; reputation<MAX_REP_COLUMN; reputation++) {
			rmodrep[reputation] = strtol(rmr->QueryField(0, reputation), NULL, 0);
		}
	}

	//initializing the reaction mod. charisma table
	AutoTable rmc("rmodchr");
	if (rmc) {
		for (int charisma=0; charisma<MAX_CHR_COLUMN; charisma++) {
			rmodchr[charisma] = strtol(rmc->QueryField(0, charisma), NULL, 0);
		}
	}

	// see note in voodooconst.h
	if (core->HasFeature(GF_AREA_OVERRIDE)) {
		MAX_OPERATING_DISTANCE = 40*3;
	}
}

int GetReaction(const Actor *target, const Scriptable *Sender)
{
	int chr, rep, reaction;

	chr = target->GetStat(IE_CHR)-1;
	if (target->GetStat(IE_EA) == EA_PC) {
		rep = core->GetGame()->Reputation/10-1;
	} else {
		rep = target->GetStat(IE_REPUTATION)/10-1;
	}
	if (rep<0) rep = 0;
	else if (rep>=MAX_REP_COLUMN) rep=MAX_REP_COLUMN-1;
	if (chr<0) chr = 0;
	else if (chr>=MAX_CHR_COLUMN) chr=MAX_CHR_COLUMN-1;

	reaction = 10 + rmodrep[rep] + rmodchr[chr];

	// add -4 penalty when dealing with racial enemies
	if (Sender && target->GetRangerLevel() && (Sender->Type == ST_ACTOR) ) {
		reaction -= target->GetRacialEnemyBonus((const Actor *) Sender);
	}

	return reaction;
}

int GetHappiness(const Scriptable *Sender, int reputation)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *ab = (const Actor *) Sender;
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
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	const Actor *ab = (const Actor *) Sender;
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

void HandleBitMod(ieDword &value1, ieDword value2, int opcode)
{
	switch(opcode) {
		case OP_AND:
			value1 = ( value1& value2 );
			break;
		case OP_OR:
			value1 = ( value1| value2 );
			break;
		case OP_XOR:
			value1 = ( value1^ value2 );
			break;
		case OP_NAND: //this is a GemRB extension
			value1 = ( value1& ~value2 );
			break;
		case OP_SET: //this is a GemRB extension
			value1 = value2;
			break;
	}
}

// SPIT is not in the original engine spec, it is reserved for the
// enchantable items feature
//					0      1       2     3      4
static const char *spell_suffices[]={"SPIT","SPPR","SPWI","SPIN","SPCL"};

//this function handles the polymorphism of Spell[RES] actions
//it returns spellres
bool ResolveSpellName(ieResRef spellres, Action *parameters)
{
	if (parameters->string0Parameter[0]) {
		strnlwrcpy(spellres, parameters->string0Parameter, 8);
	} else {
		//resolve spell
		int type = parameters->int0Parameter/1000;
		int spellid = parameters->int0Parameter%1000;
		if (type>4) {
			return false;
		}
		sprintf(spellres, "%s%03d", spell_suffices[type], spellid);
	}
	return gamedata->Exists(spellres, IE_SPL_CLASS_ID);
}

void ResolveSpellName(ieResRef spellres, ieDword number)
{
	//resolve spell
	unsigned int type = number/1000;
	int spellid = number%1000;
	if (type>4) {
		type=0;
	}
	sprintf(spellres, "%s%03d", spell_suffices[type], spellid);
}

ieDword ResolveSpellNumber(const ieResRef spellres)
{
	int i;

	for(i=0;i<5;i++) {
		if(!strnicmp(spellres, spell_suffices[i], 4)) {
			int n = -1;
			sscanf(spellres+4,"%d", &n);
			if (n<0) {
				return 0xffffffff;
			}
			return i*1000+n;
		}
	}
	return 0xffffffff;
}

bool ResolveItemName(ieResRef itemres, Actor *act, ieDword Slot)
{
	CREItem *itm = act->inventory.GetSlotItem(Slot);
	if(itm) {
		strnlwrcpy(itemres, itm->ItemResRef, 8);
		return gamedata->Exists(itemres, IE_ITM_CLASS_ID);
	}
	return false;
}

bool StoreHasItemCore(const ieResRef storename, const ieResRef itemname)
{
	CREItem item;

	Store* store = gamedata->GetStore(storename);
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

static bool StoreGetItemCore(CREItem &item, const ieResRef storename, const ieResRef itemname, unsigned int count)
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

//don't pass this point by reference, it is subject to change
void ClickCore(Scriptable *Sender, Point point, int type, int speed)
{
	Map *map = Sender->GetCurrentArea();
	if (!map) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Point p=map->TMap->GetMapSize();
	if (!p.PointInside(point)) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Video *video = core->GetVideoDriver();
	GlobalTimer *timer = core->timer;
	timer->SetMoveViewPort( point.x, point.y, speed, true );
	timer->DoStep(0);
	if (timer->ViewportIsMoving()) {
		Sender->AddActionInFront( Sender->GetCurrentAction() );
		Sender->SetWait(1);
		Sender->ReleaseCurrentAction();
		return;
	}

	video->ConvertToScreen(point.x, point.y);
	GameControl *win = core->GetGameControl();

	point.x+=win->XPos;
	point.y+=win->YPos;
	video->MoveMouse(point.x, point.y);
	video->ClickMouse(type);
	Sender->ReleaseCurrentAction();
}

void PlaySequenceCore(Scriptable *Sender, Action *parameters, ieDword value)
{
	Scriptable* tar;

	if (parameters->objects[1]) {
		tar = GetActorFromObject( Sender, parameters->objects[1] );
		if (!tar) {
			//could be an animation
			AreaAnimation* anim = Sender->GetCurrentArea( )->GetAnimation( parameters->objects[1]->objectName);
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
	if (tar->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) tar;
	actor->SetStance( value );
}

void TransformItemCore(Actor *actor, Action *parameters, bool onlyone)
{
	int i = actor->inventory.GetSlotCount();
	while(i--) {
		CREItem *item = actor->inventory.GetSlotItem(i);
		if (!item) {
			continue;
		}
		if (strnicmp(item->ItemResRef, parameters->string0Parameter, 8) ) {
			continue;
		}
		actor->inventory.SetSlotItemRes(parameters->string1Parameter,i,parameters->int0Parameter,parameters->int1Parameter,parameters->int2Parameter);
		if (onlyone) {
			break;
		}
	}
}

//check if an inventory (container or actor) has item (could be recursive ?)
bool HasItemCore(const Inventory *inventory, const ieResRef itemname, ieDword flags)
{
	if (inventory->HasItem(itemname, flags)) {
		return true;
	}
	int i=inventory->GetSlotCount();
	while (i--) {
		//maybe we could speed this up if we mark bag items with a flags bit
		CREItem *itemslot = inventory->GetSlotItem(i);
		if (!itemslot)
			continue;
		Item *item = gamedata->GetItem(itemslot->ItemResRef);
		if (!item)
			continue;
		bool ret = false;
		if (core->CanUseItemType(SLOT_BAG,item,NULL) ) {
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

//finds and takes an item from a container in the given inventory
static bool GetItemContainer(CREItem &itemslot2, Inventory *inventory, const ieResRef itemname, int count)
{
	int i=inventory->GetSlotCount();
	while (i--) {
		//maybe we could speed this up if we mark bag items with a flags bit
		CREItem *itemslot = inventory->GetSlotItem(i);
		if (!itemslot)
			continue;
		Item *item = gamedata->GetItem(itemslot->ItemResRef);
		if (!item)
			continue;
		bool ret = core->CanUseItemType(SLOT_BAG,item,NULL);
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

void DisplayStringCore(Scriptable* const Sender, int Strref, int flags)
{
	//no one hears you when you are in the Limbo!
	if (!Sender->GetCurrentArea()) {
		return;
	}

	char Sound[_MAX_PATH] = "";
	ieResRef soundRef = {};
	unsigned int channel = SFX_CHAN_DIALOG;

	Log(MESSAGE, "GameScript", "Displaying string on: %s", Sender->GetScriptName() );
	if (flags & DS_CONST) {
		if (Sender->Type!=ST_ACTOR) {
			Log(ERROR, "GameScript", "Verbal constant not supported for non actors!");
			return;
		}
		Actor* actor = ( Actor* ) Sender;
		if ((ieDword) Strref>=VCONST_COUNT) {
			Log(ERROR, "GameScript", "Invalid verbal constant!");
			return;
		}

		int tmp=(int) actor->GetVerbalConstant(Strref);
		if (tmp <= 0 || (actor->GetStat(IE_MC_FLAGS) & MC_EXPORTABLE)) {
			//get soundset based string constant
			actor->ResolveStringConstant(soundRef, (unsigned int) Strref);
			if (actor->PCStats && actor->PCStats->SoundFolder[0]) {
				snprintf(Sound, _MAX_PATH, "%s/%s",
					actor->PCStats->SoundFolder, soundRef);
			} else {
				memcpy(Sound, soundRef, sizeof(ieResRef) );
			}
		}
		Strref = tmp;

		//display the verbal constants in the console
		ieDword charactersubtitles = 0;
		core->GetDictionary()->Lookup("Subtitles", charactersubtitles);
		if (charactersubtitles) {
			flags |= DS_CONSOLE;
		}

		if (actor->InParty > 0) {
			channel = SFX_CHAN_CHAR0 + actor->InParty - 1;
		} else if (actor->GetStat(IE_EA) >= EA_EVILCUTOFF) {
			channel = SFX_CHAN_MONSTER;
		}
	}

	if ((Strref != -1) && !soundRef[0]) {
		StringBlock sb = core->strings->GetStringBlock( Strref );
		memcpy(Sound, sb.Sound, sizeof(ieResRef) );
		if (sb.text) {
			if (flags & DS_CONSOLE) {
				//can't play the sound here, we have to delay action
				//and for that, we have to know how long the text takes
				if(flags&DS_NONAME) {
					displaymsg->DisplayString(*sb.text);
				} else {
					displaymsg->DisplayStringName( Strref, DMC_WHITE, Sender, 0);
				}
			}
			if (flags & (DS_HEAD | DS_AREA)) {
				Sender->SetOverheadText( *sb.text );
				if (flags & DS_AREA) {
					Sender->FixHeadTextPos();
				}
			}
		}
	}
	if (Sound[0] && !(flags&DS_SILENT) ) {
		ieDword speech = 0;
		Point pos(Sender->Pos.x, Sender->Pos.y);
		if (flags&DS_SPEECH) {
			speech = GEM_SND_SPEECH;
		}
		// disable position, but only for party
		if (Sender->Type != ST_ACTOR || reinterpret_cast<Actor*>(Sender)->InParty) {
			speech |= GEM_SND_RELATIVE;
			pos.x = pos.y = 0;
		}
		if (flags&DS_QUEUE) speech|=GEM_SND_QUEUE;
		unsigned int len = 0;
		core->GetAudioDrv()->Play(Sound, channel, pos.x, pos.y, speech, &len);
		ieDword counter = ( AI_UPDATE_TIME * len ) / 1000;

		if (Sender->Type == ST_ACTOR && len > 0 && flags & DS_CIRCLE) {
			reinterpret_cast<Actor*>(Sender)->SetAnimatedTalking(len);
		}

		if ((counter != 0) && (flags &DS_WAIT) )
			Sender->SetWait( counter );
	}
}

int CanSee(const Scriptable *Sender, const Scriptable *target, bool range, int seeflag)
{
	if (target->Type==ST_ACTOR) {
		const Actor *tar = (const Actor *) target;

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
			const Actor *snd = (const Actor *) Sender;
			dist = snd->Modified[IE_VISUALRANGE];
		} else {
			dist = VOODOO_VISUAL_RANGE;
			los = false;
		}

		if (Distance(target->Pos, Sender->Pos) > dist * VOODOO_CANSEE_F) {
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
	const Scriptable *tar = GetActorFromObject(Sender, parameters->objectParameter, flags);
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
			//TODO: maybe set the object references here too
			return 1;
		}
		// NOTE: Detect supposedly doesn't set LastMarked — disable on GA_DETECT if needed
		if (Sender->Type==ST_ACTOR && tar->Type==ST_ACTOR && Sender!=tar) {
			Actor* snd = ( Actor* ) Sender;
			//additional checks for invisibility?
			snd->LastSeen = tar->GetGlobalID();
			snd->LastMarked = tar->GetGlobalID();
		}
		return 1;
	}
	return 0;
}

//transfering item from Sender to target
//if target has no inventory, the item will be destructed
//if target can't get it, it will be dropped at its feet
int MoveItemCore(Scriptable *Sender, Scriptable *target, const char *resref, int flags, int setflag, int count)
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
	switch(Sender->Type) {
		case ST_ACTOR:
			myinv=&((Actor *) Sender)->inventory;
			if (((Actor *)Sender)->InParty) lostitem = true;
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
		if (!GetItemContainer(*item, myinv, resref, count)) {
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
			myinv=&((Actor *) target)->inventory;
			if (((Actor *) target)->InParty) gotitem = true;
			break;
		case ST_CONTAINER:
			myinv=&((Container *) target)->inventory;
			break;
		default:
			myinv = NULL;
			break;
	}
	if (lostitem&&!gotitem) displaymsg->DisplayConstantString(STR_LOSTITEM, DMC_BG2XPGREEN);

	if (!myinv) {
		delete item;
		return MIC_GOTITEM; // actually it was lost, not gained
	}
	if ( myinv->AddSlotItem(item, SLOT_ONLYINVENTORY) !=ASI_SUCCESS) {
		// drop it at my feet
		map->AddItemToLocation(target->Pos, item);
		if (gotitem) {
			if (target->Type == ST_ACTOR) {
				if (((Actor *) target)->InParty) {
					((Actor *) target)->VerbalConstant(VB_INVENTORY_FULL);
				}
			}
			displaymsg->DisplayConstantString(STR_INVFULL_ITEMDROP, DMC_BG2XPGREEN);
		}
		return MIC_FULL;
	}
	if (gotitem&&!lostitem) displaymsg->DisplayConstantString(STR_GOTITEM, DMC_BG2XPGREEN);
	return MIC_GOTITEM;
}

void PolymorphCopyCore(Actor *src, Actor *tar)
{
	tar->SetBase(IE_ANIMATION_ID, src->GetStat(IE_ANIMATION_ID) );

	tar->SetBase(IE_ARMOR_TYPE, src->GetStat(IE_ARMOR_TYPE) );
	for (int i=0;i<7;i++) {
		tar->SetBase(IE_COLORS+i, src->GetStat(IE_COLORS+i) );
	}

	tar->SetName(src->GetName(0),0);
	tar->SetName(src->GetName(1),1);
	//add more attribute copying
}

void CreateCreatureCore(Scriptable* Sender, Action* parameters, int flags)
{
	Scriptable *tmp = GetActorFromObject( Sender, parameters->objects[1] );
	//if there is nothing to copy, don't spawn anything
	if (flags & CC_COPY) {
		if (!tmp || tmp->Type != ST_ACTOR) {
			return;
		}
	}

	Actor* ab;
	if (flags & CC_STRING1) {
		ab = gamedata->GetCreature(parameters->string1Parameter);
	}
	else {
		ab = gamedata->GetCreature(parameters->string0Parameter);
	}

	if (!ab) {
		Log(ERROR, "GameScript", "Failed to create creature! (missing creature file %s?)",
			parameters->string0Parameter);
		// maybe this should abort()?
		return;
	}

	//iwd2 allows an optional scriptname to be set
	//but bg2 doesn't have this feature
	//this way it works for both games
	if ((flags & CC_SCRIPTNAME) && parameters->string1Parameter[0]) {
		ab->SetScriptName(parameters->string1Parameter);
	}

	int radius;
	Point pnt;

	radius=0;
	switch (flags & CC_MASK) {
		//creates creature just off the screen
		case CC_OFFSCREEN:
			{
			Region vp = core->GetVideoDriver()->GetViewport();
			radius=vp.w/2;
			//center of screen
			pnt.x = vp.x+radius;
			pnt.y = vp.y+vp.h/2;
			break;
			}
			//fall through
		case CC_OBJECT://use object + offset
			if (tmp) Sender=tmp;
			//fall through
		case CC_OFFSET://use sender + offset
			pnt.x = parameters->pointParameter.x+Sender->Pos.x;
			pnt.y = parameters->pointParameter.y+Sender->Pos.y;
			break;
		default: //absolute point, but -1,-1 means AtFeet
			pnt.x = parameters->pointParameter.x;
			pnt.y = parameters->pointParameter.y;
			if (pnt.isempty()) {
				if (Sender->Type == ST_PROXIMITY || Sender->Type == ST_TRIGGER) {
					pnt.x = ((InfoPoint *)Sender)->TrapLaunch.x;
					pnt.y = ((InfoPoint *)Sender)->TrapLaunch.y;
				} else {
					pnt.x = Sender->Pos.x;
					pnt.y = Sender->Pos.y;
				}
			}
			break;
	}

	Map *map = Sender->GetCurrentArea();
	map->AddActor( ab, true );
	//radius adjusted to tile size
	ab->SetPosition( pnt, flags&CC_CHECK_IMPASSABLE, radius/16, radius/12 );
	ab->SetOrientation(parameters->int0Parameter, false );

	// also set it as Sender's LastMarkedObject (fixes worg rider dismount killing players)
	if (Sender->Type == ST_ACTOR) {
		Actor *actor = (Actor *) Sender;
		actor->LastMarked = ab->GetGlobalID();
	}

	//if string1 is animation, then we can't use it for a DV too
	if (flags & CC_PLAY_ANIM) {
		CreateVisualEffectCore( ab, ab->Pos, parameters->string1Parameter, 1);
	} else {
		//setting the deathvariable if it exists (iwd2)
		if (parameters->string1Parameter[0]) {
			ab->SetScriptName(parameters->string1Parameter);
		}
	}

	if (flags & CC_COPY) {
		PolymorphCopyCore ( (Actor *) tmp, ab);
	}
}

static ScriptedAnimation *GetVVCEffect(const char *effect, int iterations)
{
	if (effect[0]) {
		ScriptedAnimation* vvc = gamedata->GetScriptedAnimation(effect, false);
		if (!vvc) {
			Log(ERROR, "GameScript", "Failed to create effect.");
			return NULL;
		}
		if (iterations > 1) {
			vvc->SetDefaultDuration( vvc->GetSequenceDuration(AI_UPDATE_TIME * iterations));
		} else {
			vvc->PlayOnce();
		}
		return vvc;
	}
	return NULL;
}

void CreateVisualEffectCore(Actor *target, const char *effect, int iterations)
{
	ScriptedAnimation *vvc = GetVVCEffect(effect, iterations);
	if (vvc) {
		target->AddVVCell( vvc );
	}
}

void CreateVisualEffectCore(Scriptable *Sender, const Point &position, const char *effect, int iterations)
{
	ScriptedAnimation *vvc = GetVVCEffect(effect, iterations);
	if (vvc) {
		vvc->XPos +=position.x;
		vvc->YPos +=position.y;
		Map *area = Sender->GetCurrentArea();
		if (area) {
			area->AddVVCell(new VEFObject(vvc));
		} else {
			Log(WARNING, "GSUtils", "Skipping visual effect positioning due to missing area!");
			delete vvc;
		}
	}
}

//this destroys the current actor and replaces it with another
void ChangeAnimationCore(Actor *src, const char *resref, bool effect)
{
	Actor *tar = gamedata->GetCreature(resref);
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

void EscapeAreaCore(Scriptable* Sender, const Point &p, const char* area, const Point &enter, int flags, int wait)
{
	char Tmp[256];

	if (Sender->CurrentActionTicks<100) {
		if ( !p.isempty() && PersonalDistance(p, Sender)>MAX_OPERATING_DISTANCE) {
			//MoveNearerTo will return 0, if the actor is in move
			//it will return 1 (the fourth parameter) if the target is unreachable
			if (!MoveNearerTo(Sender, p, MAX_OPERATING_DISTANCE,1) ) {
				if(!Sender->InMove()) print("At least it said so...");
				// ensure the action doesn't get interrupted
				// fixes Nalia starting a second dialog in the Coronet, if she gets a chance #253
				Sender->CurrentActionInterruptable = false;
				return;
			}
		}
	}

	if (flags &EA_DESTROY) {
		//this must be put into a non-const variable
		sprintf( Tmp, "DestroySelf()" );
	} else {
		// last parameter is 'face', which should be passed from relevant action parameter..
		sprintf( Tmp, "MoveBetweenAreas(\"%s\",[%hd.%hd],%d)", area, enter.x, enter.y, 0 );
	}
	Log(MESSAGE, "GSUtils", "Executing %s in EscapeAreaCore", Tmp);
	//drop this action, but add another (destroyself or movebetweenareas)
	//between the arrival and the final escape, there should be a wait time
	//that wait time could be handled here
	if (wait) {
		print("But wait a bit...(%d)", wait);
		Sender->SetWait(wait);
	}
	Sender->ReleaseCurrentAction();
	Action * action = GenerateAction( Tmp);
	Sender->AddActionInFront( action );
}

static void GetTalkPositionFromScriptable(Scriptable* scr, Point &position)
{
	switch (scr->Type) {
		case ST_AREA: case ST_GLOBAL:
			position = scr->Pos; //fake
			break;
		case ST_ACTOR:
			//if there are other moveables, put them here
			position = ((Movable *) scr)->GetMostLikelyPosition();
			break;
		case ST_TRIGGER: case ST_PROXIMITY: case ST_TRAVEL:
			if (((InfoPoint *) scr)->GetUsePoint() ) {
				position=((InfoPoint *) scr)->UsePoint;
				break;
			}
			position=((InfoPoint *) scr)->TalkPos;
			break;
		case ST_DOOR: case ST_CONTAINER:
			position=((Highlightable *) scr)->TrapLaunch;
			break;
	}
}

void GetPositionFromScriptable(const Scriptable *scr, Point &position, bool dest)
{
	if (!dest) {
		position = scr->Pos;
		return;
	}
	switch (scr->Type) {
		case ST_AREA: case ST_GLOBAL:
			position = scr->Pos; //fake
			break;
		case ST_ACTOR:
		//if there are other moveables, put them here
			position = ((const Movable *) scr)->GetMostLikelyPosition();
			break;
		case ST_TRIGGER: case ST_PROXIMITY: case ST_TRAVEL:
			if (((const InfoPoint *) scr)->GetUsePoint()) {
				position = ((const InfoPoint *) scr)->UsePoint;
				break;
			}
		// intentional fallthrough
		case ST_DOOR: case ST_CONTAINER:
			position = ((const Highlightable *) scr)->TrapLaunch;
	}
}

static ieResRef PlayerDialogRes = "PLAYERx\0";

void BeginDialog(Scriptable* Sender, Action* parameters, int Flags)
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
		Log(ERROR, "GameScript", "Speaker for dialog couldn't be found (Sender: %s, Type: %d) Flags:%d.",
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
		Log(ERROR, "GameScript", "Target for dialog couldn't be found (Sender: %s, Type: %d).",
			Sender->GetScriptName(), Sender->Type);
		if (Sender->Type == ST_ACTOR) {
			((Actor *) Sender)->dump();
		}
		StringBuffer buffer;
		buffer.append("Target object: ");
		if (parameters->objects[1]) {
			parameters->objects[1]->dump(buffer);
		} else {
			buffer.append("<NULL>\n");
		}
		Log(ERROR, "GameScript", buffer);
		Sender->ReleaseCurrentAction();
		return;
	}

	Actor *speaker, *target;

	speaker = NULL;
	target = (Actor *) tar;
	bool swap = false;
	if (scr->Type==ST_ACTOR) {
		speaker = (Actor *) scr;
		if (speaker->GetStat(IE_STATE_ID)&STATE_DEAD) {
			StringBuffer buffer;
			buffer.append("Speaker is dead, cannot start dialogue. Speaker and target are:\n");
			speaker->dump(buffer);
			target->dump(buffer);
			Log(ERROR, "GameScript", buffer);
			Sender->ReleaseCurrentAction();
			return;
		}
		//making sure speaker is the protagonist, player, actor
		Actor *protagonist = core->GetGame()->GetPC(0, false);
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
	if (gc->GetDialogueFlags()&DF_IN_DIALOG) {
		if (Flags & BD_INTERRUPT) {
			//break the current dialog if possible
			gc->dialoghandler->EndDialog(true);
		}
		//check if we could manage to break it, not all dialogs are breakable!
		if (gc->GetDialogueFlags()&DF_IN_DIALOG) {
			Log(WARNING, "GameScript", "Dialog cannot be initiated because there is already one.");
			Sender->ReleaseCurrentAction();
			return;
		}
	}

	// starting a dialog ends cutscenes!
	core->SetCutSceneMode(false);

	const char* Dialog = NULL;
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
			PlayerDialogRes[5] = '1';
			Dialog = ( const char * ) PlayerDialogRes;
			break;
		case BD_INTERACT: //using the source for the dialog
			Game *game = core->GetGame();
			if (game->BanterBlockFlag || game->BanterBlockTime) {
				Log(MESSAGE, "GameScript", "Banterblock disabled interaction.");
				Sender->ReleaseCurrentAction();
				return;
			}
			const char* scriptingname = scr->GetScriptName();

			/* banter dialogue */
			pdtable.load("interdia");
			//Dialog is a borrowed reference, and pdtable will be freed automagically
			if (pdtable) {
				//5 is the magic number for the ToB expansion
				if (game->Expansion==5) {
					Dialog = pdtable->QueryField( scriptingname, "25FILE" );
				} else {
					Dialog = pdtable->QueryField( scriptingname, "FILE" );
				}
			}
			break;
	}

	// moved this here from InitDialog, because InitDialog doesn't know which side is which
	// post-swap (and non-actors always have IF_NOINT set) .. also added a check that it's
	// actually busy doing something, for the same reason
	Action *curact = target->GetCurrentAction();
	if ((speaker != target) && (target->GetInternalFlag()&IF_NOINT) && \
	  (!curact && target->GetNextAction())) {
		core->GetTokenDictionary()->SetAtCopy("TARGET", target->GetName(1));
		displaymsg->DisplayConstantString(STR_TARGETBUSY, DMC_RED);
		Sender->ReleaseCurrentAction();
		return;
	}

	if (speaker!=target) {
		if (swap) {
			Scriptable *tmp = tar;
			tar = scr;
			scr = tmp;
		} else {
			if (!(Flags & BD_INTERRUPT)) {
				// added CurrentAction as part of blocking action fixes
				if (tar->GetCurrentAction() || tar->GetNextAction()) {
					core->GetTokenDictionary()->SetAtCopy("TARGET", target->GetName(1));
					displaymsg->DisplayConstantString(STR_TARGETBUSY, DMC_RED);
					Sender->ReleaseCurrentAction();
					return;
				}
			}
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
				talker->SetOrientation(GetOrient( tar->Pos, scr->Pos), true);
				if (talker->InParty) {
					talker->SetStance(IE_ANI_READY);
				}
			}
		}
		if (tar->Type==ST_ACTOR) {
			// might not be equal to target anymore due to swapping
			Actor *talkee = (Actor *) tar;
			if (!talkee->Immobile() && !(talkee->GetStat(IE_STATE_ID) & STATE_SLEEP) && !(talkee->AppearanceFlags&APP_NOTURN)) {
				talkee->SetOrientation(GetOrient( scr->Pos, tar->Pos), true);
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
		gc->SetDialogueFlags(DF_TALKCOUNT, OP_OR);
	} else if ((Flags & BD_LOCMASK) == BD_INTERACT) {
		gc->SetDialogueFlags(DF_INTERACT, OP_OR);
	}

	core->GetDictionary()->SetAt("DialogChoose",(ieDword) -1);
	if (!gc->dialoghandler->InitDialog(scr, tar, Dialog)) {
		if (!(Flags & BD_NOEMPTY)) {
			displaymsg->DisplayConstantStringName(STR_NOTHINGTOSAY, DMC_RED, tar);
		}
	}

	Sender->ReleaseCurrentAction();
}

static EffectRef fx_movetoarea_ref = { "MoveToArea", -1 };

bool CreateMovementEffect(Actor* actor, const char *area, const Point &position, int face)
{
	if (!strnicmp(area, actor->Area, 8) ) return false; //no need of this for intra area movement

	Effect *fx = EffectQueue::CreateEffect(fx_movetoarea_ref, 0, face, FX_DURATION_INSTANT_PERMANENT);
	if (!fx) return false;
	fx->SetPosition(position);
	strnuprcpy(fx->Resource, area, 8);
	core->ApplyEffect(fx, actor, actor);
	delete fx;
	return true;
}

void MoveBetweenAreasCore(Actor* actor, const char *area, const Point &position, int face, bool adjust)
{
	Log(MESSAGE, "GameScript", "MoveBetweenAreas: %s to %s [%d.%d] face: %d",
		actor->GetName(0), area,position.x,position.y, face);
	Map* map1 = actor->GetCurrentArea();
	Map* map2;
	Game* game = core->GetGame();
	bool newSong = false;
	if (area[0] && (!map1 || stricmp(area, map1->GetScriptName()))) { //do we need to switch area?
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
			if (entry) {
				// make sure the area is marked as revealed and visited
				if (!(entry->GetAreaStatus() & WMP_ENTRY_VISITED)) {
					entry->SetAreaStatus(WMP_ENTRY_VISIBLE|WMP_ENTRY_VISITED, OP_OR);
				}
			}
		}
	}
	actor->SetPosition(position, adjust);
	actor->SetStance(IE_ANI_READY);
	if (face !=-1) {
		actor->SetOrientation( face, false );
	}
	// should this perhaps be a 'selected' check or similar instead?
	if (actor->InParty) {
		GameControl *gc=core->GetGameControl();
		gc->SetScreenFlags(SF_CENTERONACTOR,OP_OR);
		if (newSong) {
			game->ChangeSong(false, true);
		}
	}
}

//repeat movement, until goal isn't reached
//if int0parameter is !=0, then it will try only x times
void MoveToObjectCore(Scriptable *Sender, Action *parameters, ieDword flags, bool untilsee)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	const Scriptable *target = GetStoredActorFromObject(Sender, parameters->objects[1]);
	if (!target) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	Point dest = target->Pos;
	if (target->Type == ST_TRIGGER && ((const InfoPoint *)target)->GetUsePoint()) {
		dest = ((const InfoPoint *)target)->UsePoint;
	}
	if (untilsee && CanSee(actor, target, true, 0) ) {
		Sender->LastSeen = target->GetGlobalID();
		Sender->ReleaseCurrentAction();
		return;
	} else {
		if (PersonalDistance(actor, target)<MAX_OPERATING_DISTANCE) {
			Sender->ReleaseCurrentAction();
			return;
		}
	}
	if (!actor->InMove() || actor->Destination != dest) {
		actor->WalkTo( dest, flags, 0 );
	}
	//hopefully this hack will prevent lockups
	if (!actor->InMove()) {
		if (flags&IF_NOINT) {
			actor->Interrupt();
		}
		Sender->ReleaseCurrentAction();
		return;
	}

	//repeat movement...
	Action *newaction = ParamCopyNoOverride(parameters);
	if (newaction->int0Parameter!=1) {
		if (newaction->int0Parameter) {
			newaction->int0Parameter--;
		}
		actor->AddActionInFront(newaction);
		actor->SetWait(1);
	} else {
		delete newaction;
		actor->Interrupt();
	}

	Sender->ReleaseCurrentAction();
}

bool CreateItemCore(CREItem *item, const char *resref, int a, int b, int c)
{
	//copy the whole resref, including the terminating zero
	strnuprcpy(item->ItemResRef, resref, 8);
	if (!core->ResolveRandomItem(item))
		return false;
	if (a==-1) {
		//use the default charge counts of the item
		Item *origitem = gamedata->GetItem(item->ItemResRef);
		if (origitem) {
			for(int i=0;i<3;i++) {
				ITMExtHeader *e = origitem->GetExtHeader(i);
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
	//this is a dangerous cast, make sure actor is Actor * !!!
	assert(Sender && Sender->Type == ST_ACTOR);
	Actor *actor = (Actor *) Sender;

	// if held or disabled, etc, then cannot start or continue attacking
	if (actor->Immobile()) {
		actor->roundTime = 0;
		Sender->ReleaseCurrentAction();
		return;
	}

	// mislead and projected images can't attack
	int puppet = actor->GetStat(IE_PUPPETMASTERTYPE);
	if (puppet && puppet < 3) {
		Log(DEBUG, "AttackCore", "Tried attacking with an illusionary copy: %s!", actor->GetName(1));
		return;
	}

	Actor *tar = NULL;
	if (target->Type==ST_ACTOR) {
		tar = (Actor *) target;

		// release if target is invisible to sender (because of death or invisbility spell)
		if (tar->IsInvisibleTo(Sender) || (tar->GetSafeStat(IE_STATE_ID) & STATE_DEAD)){
			actor->StopAttack();
			Sender->ReleaseCurrentAction();
			actor->AddTrigger(TriggerEntry(trigger_targetunreachable, tar->GetGlobalID()));
			Log(WARNING, "AttackCore", "Tried attacking invisible/dead actor: %s!", tar->GetName(1));
			return;
		}
	}

	if (actor == tar) {
		Sender->ReleaseCurrentAction();
		Log(WARNING, "AttackCore", "Tried attacking itself: %s!", tar->GetName(1));
		return;
	}

	WeaponInfo wi;
	bool leftorright = false;
	ITMExtHeader *header = actor->GetWeapon(wi, leftorright);
	//will return false on any errors (eg, unusable weapon)
	if (!header || !actor->WeaponIsUsable(leftorright, header)) {
		actor->StopAttack();
		Sender->ReleaseCurrentAction();
		assert(tar);
		actor->AddTrigger(TriggerEntry(trigger_unusable, tar->GetGlobalID()));
		Log(WARNING, "AttackCore", "Weapon unusable: %s!", actor->GetName(1));
		return;
	}

	unsigned int weaponrange = actor->GetWeaponRange(wi);

	if ( target->Type == ST_DOOR || target->Type == ST_CONTAINER) {
		weaponrange += 10;
	}
	if (!(flags&AC_NO_SOUND) ) {
		if (!Sender->CurrentActionTicks) {
			// play the battle cry
			// pick from all 5 possible verbal constants
			if (!actor->PlayWarCry(5)) {
				// for monsters also try their 2da/ini file sounds
				if (!actor->InParty) {
					ieResRef sound;
					actor->GetSoundFromFile(sound, 200);
					core->GetAudioDrv()->Play(sound, SFX_CHAN_MONSTER, actor->Pos.x, actor->Pos.y);
				}
			}
			//display attack message
			if (target->GetGlobalID() != Sender->LastTarget) {
				displaymsg->DisplayConstantStringAction(STR_ACTION_ATTACK, DMC_WHITE, Sender, target);
			}
		}
	}

	double angle = atan2(actor->Pos.y - target->Pos.y, actor->Pos.x - target->Pos.x);
	if ( Sender->GetCurrentArea()!=target->GetCurrentArea() ||
		!WithinPersonalRange(actor, target, weaponrange) ||
		(!Sender->GetCurrentArea()->IsVisibleLOS(Sender->Pos, target->Pos)) ||
		!CanSee(Sender, target, true, 0)) {
		MoveNearerTo(Sender, target, Feet2Pixels(weaponrange, angle));
		return;
	} else if (target->Type == ST_DOOR) {
		//Forcing a lock does not launch the trap...
		Door* door = (Door*) target;
		if(door->Flags & DOOR_LOCKED) {
			door->TryBashLock(actor);
		}
		Sender->ReleaseCurrentAction();
		return;
	} else if (target->Type == ST_CONTAINER) {
		Container* cont = (Container*) target;
		if(cont->Flags & CONT_LOCKED) {
			cont->TryBashLock(actor);
		}
		Sender->ReleaseCurrentAction();
		return;
	}
	//action performed
	actor->FaceTarget(target);

	Sender->LastTarget = target->GetGlobalID();
	Sender->LastTargetPersistent = Sender->LastTarget;
	actor->PerformAttack(core->GetGame()->GameTime);
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
static int GetIdsValue(const char *&symbol, const char *idsname)
{
	int idsfile=core->LoadSymbol(idsname);
	Holder<SymbolMgr> valHook = core->GetSymbol(idsfile);
	if (!valHook) {
		Log(ERROR, "GameScript", "Missing IDS file %s for symbol %s!", idsname, symbol);
		return -1;
	}
	char *newsymbol;
	int value=strtol(symbol, &newsymbol, 0);
	if (symbol!=newsymbol) {
		symbol=newsymbol;
		return value;
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

/* this function was lifted from GenerateAction, to make it clearer */
Action* GenerateActionCore(const char *src, const char *str, unsigned short actionID)
{
	Action *newAction = new Action(true);
	newAction->actionID = actionID;
	//this flag tells us to merge 2 consecutive strings together to get
	//a variable (context+variablename)
	int mergestrings = actionflags[newAction->actionID]&AF_MERGESTRINGS;
	int objectCount = ( newAction->actionID == 1 ) ? 0 : 1;
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
			Log(WARNING, "GSUtils", "parser was sidetracked: %s", str);
		}
		switch (*str) {
			default:
				Log(WARNING, "GSUtils", "Invalid type: %s", str);
				//str++;
				delete newAction;
				return NULL;

			case 'p': //Point
				SKIP_ARGUMENT();
				src++; //Skip [
				newAction->pointParameter.x = (short) strtol( src, (char **) &src, 10 );
				src++; //Skip .
				newAction->pointParameter.y = (short) strtol( src, (char **) &src, 10 );
				src++; //Skip ]
				break;

			case 'i': //Integer
			{
				//going to the variable name
				while (*str != '*' && *str !=',' && *str != ')' ) {
					str++;
				}
				int value;
				if (*str=='*') { //there may be an IDS table
					str++;
					ieVariable idsTabName;
					char* tmp = idsTabName;
					while (( *str != ',' ) && ( *str != ')' )) {
						*tmp = *str;
						tmp++;
						str++;
					}
					*tmp = 0;
					if (idsTabName[0]) {
						value = GetIdsValue(src, idsTabName);
					}
					else {
						value = strtol( src, (char **) &src, 0);
					}
				}
				else { //no IDS table
					value = strtol( src, (char **) &src, 0);
				}
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
			//Action
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
					//abort();
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
					dst = newAction->string0Parameter;
				} else {
					dst = newAction->string1Parameter;
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
				//if strings ever need a , inside, this is a FIXME
				while (*src != '"' && *src !=',') {
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
				if (*src == '"') {
					src++;
				}
				*dst = 0;
				//reading the context part
				if (mergestrings) {
					str++;
					if (*str!='s') {
						Log(ERROR, "GSUtils", "Invalid mergestrings:%s", str);
						//abort();
						delete newAction;
						return NULL;
					}
					SKIP_ARGUMENT();
					if (!stringsCount) {
						dst = newAction->string0Parameter;
					} else {
						dst = newAction->string1Parameter;
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

	if (Sender->Type != ST_ACTOR) {
		Log(ERROR, "GameScript", "MoveNearerTo only works with actors");
		Sender->ReleaseCurrentAction();
		return;
	}

	const Map *myarea = Sender->GetCurrentArea();
	const Map *hisarea = target->GetCurrentArea();
	if (hisarea && hisarea!=myarea) {
		target = myarea->GetTileMap()->GetTravelTo(hisarea->GetScriptName());

		if (!target) {
			Log(WARNING, "GameScript", "MoveNearerTo failed to find an exit");
			Sender->ReleaseCurrentAction();
			return;
		}
		((Actor *) Sender)->UseExit(target->GetGlobalID());
	} else {
		((Actor *) Sender)->UseExit(0);
	}
	// we deliberately don't try GetLikelyPosition here for now,
	// maybe a future idea if we have a better implementation
	// (the old code used it - by passing true not 0 below - when target was a movable)
	GetPositionFromScriptable(target, p, 0);

	// account for PersonalDistance (which caller uses, but pathfinder doesn't)
	if (distance && Sender->Type == ST_ACTOR) {
		distance += ((Actor *)Sender)->size*10;
	}
	if (distance && target->Type == ST_ACTOR) {
		distance += ((const Actor *) target)->size * 10;
	}

	MoveNearerTo(Sender, p, distance, dont_release);
}

//It is not always good to release the current action if target is unreachable
//we should also raise the trigger TargetUnreachable (if this is an Attack, at least)
//i hacked only this low level function, didn't need the higher ones so far
int MoveNearerTo(Scriptable *Sender, const Point &p, int distance, int dont_release)
{
	if (Sender->Type != ST_ACTOR) {
		Log(ERROR, "GameScript", "MoveNearerTo only works with actors");
		Sender->ReleaseCurrentAction();
		return 0;
	}

	// chasing is not unbreakable
	// would prevent smart ai from dropping a target that's running away
	//Sender->CurrentActionInterruptable = false;

	Actor *actor = (Actor *)Sender;

	if (!actor->InMove() || actor->Destination != p) {
		bool always_run = core->GetGameControl()->ShouldRun(actor);
		actor->WalkTo(p, always_run ? IF_RUNNING : 0, distance);
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

void FreeSrc(SrcVector *poi, const ieResRef key)
{
	int res = SrcCache.DecRef((void *) poi, key, true);
	if (res<0) {
		error("GameScript", "Corrupted Src cache encountered (reference count went below zero), Src name is: %.8s\n", key);
	}
	if (!res) {
		delete poi;
	}
}

SrcVector *LoadSrc(const ieResRef resname)
{
	SrcVector *src = (SrcVector *) SrcCache.GetResource(resname);
	if (src) {
		return src;
	}
	DataStream* str = gamedata->GetResource( resname, IE_SRC_CLASS_ID );
	if ( !str) {
		return NULL;
	}
	ieDword size=0;
	str->ReadDword(&size);
	src = new SrcVector(size);
	SrcCache.SetAt( resname, (void *) src );
	while (size--) {
		ieDword tmp;
		str->ReadDword(&tmp);
		src->at(size)=tmp;
		str->ReadDword(&tmp);
	}
	delete ( str );
	return src;
}

// checks the odd HasAdditionalRect / ADDITIONAL_RECT matching
// also returns true if the trigger is supposed to succeed
bool IsInObjectRect(const Point &pos, const Region &rect)
{
	if (!HasAdditionalRect) return true;
	if (rect.w <= 0 || rect.h <= 0) return true;

	// iwd2: testing shows the first point must be 0.0 for matching to work
	if (core->HasFeature(GF_3ED_RULES) && (rect.x != 0 || rect.y != 0)) {
		return false;
	}

	return rect.PointInside(pos);
}

#define MEMCPY(a,b) memcpy((a),(b),sizeof(a) )

static Object *ObjectCopy(Object *object)
{
	if (!object) return NULL;
	Object *newObject = new Object();
	MEMCPY( newObject->objectFields, object->objectFields );
	MEMCPY( newObject->objectFilters, object->objectFilters );
	newObject->objectRect = object->objectRect;
	MEMCPY( newObject->objectName, object->objectName );
	return newObject;
}

Action *ParamCopy(Action *parameters)
{
	Action *newAction = new Action(true);
	newAction->actionID = parameters->actionID;
	newAction->int0Parameter = parameters->int0Parameter;
	newAction->int1Parameter = parameters->int1Parameter;
	newAction->int2Parameter = parameters->int2Parameter;
	newAction->pointParameter = parameters->pointParameter;
	MEMCPY( newAction->string0Parameter, parameters->string0Parameter );
	MEMCPY( newAction->string1Parameter, parameters->string1Parameter );
	for (int c=0;c<3;c++) {
		newAction->objects[c]= ObjectCopy( parameters->objects[c] );
	}
	return newAction;
}

Action *ParamCopyNoOverride(Action *parameters)
{
	Action *newAction = new Action(true);
	newAction->actionID = parameters->actionID;
	newAction->int0Parameter = parameters->int0Parameter;
	newAction->int1Parameter = parameters->int1Parameter;
	newAction->int2Parameter = parameters->int2Parameter;
	newAction->pointParameter = parameters->pointParameter;
	MEMCPY( newAction->string0Parameter, parameters->string0Parameter );
	MEMCPY( newAction->string1Parameter, parameters->string1Parameter );
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
			Log(WARNING, "GSUtils", "parser was sidetracked: %s",str);
		}
		switch (*str) {
			default:
				Log(ERROR, "GSUtils", "Invalid type: %s", str);
				//str++;
				delete newTrigger;
				return NULL;

			case 'p': //Point
				SKIP_ARGUMENT();
				src++; //Skip [
				newTrigger->pointParameter.x = (short) strtol( src, (char **) &src, 10 );
				src++; //Skip .
				newTrigger->pointParameter.y = (short) strtol( src, (char **) &src, 10 );
				src++; //Skip ]
				break;

			case 'i': //Integer
			{
				//going to the variable name
				while (*str != '*' && *str !=',' && *str != ')' ) {
					str++;
				}
				int value;
				if (*str=='*') { //there may be an IDS table
					str++;
					ieVariable idsTabName;
					char* tmp = idsTabName;
					while (( *str != ',' ) && ( *str != ')' )) {
						*tmp = *str;
						tmp++;
						str++;
					}
					*tmp = 0;
					if (idsTabName[0]) {
						value = GetIdsValue(src, idsTabName);
					}
					else {
						value = strtol( src, (char **) &src, 0);
					}
				}
				else { //no IDS table
					value = strtol( src, (char **) &src, 0);
				}
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
					dst = newTrigger->string0Parameter;
				} else {
					dst = newTrigger->string1Parameter;
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
				while (*src != '"' && (*src != '#' || (*(src-1) != '(' && *(src-1) != ',' && *(src+1) != ')'))) {
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
						Log(ERROR, "GSUtils", "Invalid mergestrings:%s", str);
						//abort();
						delete newTrigger;
						return NULL;
					}
					SKIP_ARGUMENT();
					if (!stringsCount) {
						dst = newTrigger->string0Parameter;
					} else {
						dst = newTrigger->string1Parameter;
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

void SetVariable(Scriptable* Sender, const char* VarName, const char* Context, ieDword value)
{
	char newVarName[8+33];

	ScriptDebugLog(ID_VARIABLES, "Setting variable(\"%s%s\", %d)", Context, VarName, value);

	strlcpy( newVarName, Context, 7 );
	if (strnicmp( newVarName, "MYAREA", 6 ) == 0) {
		Sender->GetCurrentArea()->locals->SetAt( VarName, value, NoCreate );
		return;
	}
	if (strnicmp( newVarName, "LOCALS", 6 ) == 0) {
		Sender->locals->SetAt( VarName, value, NoCreate );
		return;
	}
	Game *game = core->GetGame();
	if (HasKaputz && !strnicmp(newVarName,"KAPUTZ",6) ) {
		game->kaputz->SetAt( VarName, value );
		return;
	}

	if (strnicmp(newVarName,"GLOBAL",6) ) {
		Map *map=game->GetMap(game->FindMap(newVarName));
		if (map) {
			map->locals->SetAt( VarName, value, NoCreate);
		} else if (InDebug & ID_VARIABLES) {
			Log(WARNING, "GameScript", "Invalid variable %s %s in setvariable",
				Context, VarName);
		}
	} else {
		game->locals->SetAt(VarName, value, NoCreate);
	}
}

void SetVariable(Scriptable* Sender, const char* VarName, ieDword value)
{
	char newVarName[8];
	const char *poi;

	poi = &VarName[6];
	//some HoW triggers use a : to separate the scope from the variable name
	if (*poi==':') {
		poi++;
	}

	ScriptDebugLog(ID_VARIABLES, "Setting variable(\"%s\", %d)", VarName, value);

	strlcpy( newVarName, VarName, 7 );
	if (stricmp( newVarName, "MYAREA" ) == 0) {
		Sender->GetCurrentArea()->locals->SetAt( poi, value, NoCreate );
		return;
	}
	if (stricmp( newVarName, "LOCALS" ) == 0) {
		Sender->locals->SetAt( poi, value, NoCreate );
		return;
	}
	Game *game = core->GetGame();
	if (HasKaputz && !stricmp(newVarName,"KAPUTZ") ) {
		game->kaputz->SetAt( poi, value, NoCreate );
		return;
	}
	if (stricmp(newVarName,"GLOBAL") ) {
		Map *map=game->GetMap(game->FindMap(newVarName));
		if (map) {
			map->locals->SetAt( poi, value, NoCreate);
		} else if (InDebug & ID_VARIABLES) {
			Log(WARNING, "GameScript", "Invalid variable %s in setvariable",
				VarName);
		}
	} else {
		game->locals->SetAt(poi, value, NoCreate);
	}
}

ieDword CheckVariable(const Scriptable *Sender, const char *VarName, bool *valid)
{
	char newVarName[8];
	const char *poi;
	ieDword value = 0;

	strlcpy( newVarName, VarName, 7 );
	poi = &VarName[6];
	//some HoW triggers use a : to separate the scope from the variable name
	if (*poi==':') {
		poi++;
	}

	if (stricmp( newVarName, "MYAREA" ) == 0) {
		Sender->GetCurrentArea()->locals->Lookup( poi, value );
		ScriptDebugLog(ID_VARIABLES, "CheckVariable %s: %d", VarName, value);
		return value;
	}
	if (stricmp( newVarName, "LOCALS" ) == 0) {
		Sender->locals->Lookup( poi, value );
		ScriptDebugLog(ID_VARIABLES, "CheckVariable %s: %d", VarName, value);
		return value;
	}
	Game *game = core->GetGame();
	if (HasKaputz && !stricmp(newVarName,"KAPUTZ") ) {
		game->kaputz->Lookup( poi, value );
		ScriptDebugLog(ID_VARIABLES, "CheckVariable %s: %d", VarName, value);
		return value;
	}
	if (stricmp(newVarName,"GLOBAL") ) {
		Map *map=game->GetMap(game->FindMap(newVarName));
		if (map) {
			map->locals->Lookup( poi, value);
		} else {
			if (valid) {
				*valid=false;
			}
			ScriptDebugLog(ID_VARIABLES, "Invalid variable %s in CheckVariable", VarName);
		}
	} else {
		game->locals->Lookup( poi, value );
	}
	ScriptDebugLog(ID_VARIABLES, "CheckVariable %s: %d", VarName, value);
	return value;
}

ieDword CheckVariable(const Scriptable *Sender, const char *VarName, const char *Context, bool *valid)
{
	char newVarName[8];
	ieDword value = 0;

	strlcpy(newVarName, Context, 7);
	if (stricmp( newVarName, "MYAREA" ) == 0) {
		Sender->GetCurrentArea()->locals->Lookup( VarName, value );
		ScriptDebugLog(ID_VARIABLES, "CheckVariable %s%s: %d", Context, VarName, value);
		return value;
	}
	if (stricmp( newVarName, "LOCALS" ) == 0) {
		if (!Sender->locals->Lookup(VarName, value)) {
			if (valid) {
				*valid = false;
			}
		}
		ScriptDebugLog(ID_VARIABLES, "CheckVariable %s%s: %d", Context, VarName, value);
		return value;
	}
	Game *game = core->GetGame();
	if (HasKaputz && !stricmp(newVarName,"KAPUTZ") ) {
		game->kaputz->Lookup( VarName, value );
		ScriptDebugLog(ID_VARIABLES, "CheckVariable %s%s: %d", Context, VarName, value);
		return value;
	}
	if (stricmp(newVarName,"GLOBAL") ) {
		Map *map=game->GetMap(game->FindMap(newVarName));
		if (map) {
			map->locals->Lookup( VarName, value);
		} else {
			if (valid) {
				*valid=false;
			}
			ScriptDebugLog(ID_VARIABLES, "Invalid variable %s %s in checkvariable", Context, VarName);
		}
	} else {
		game->locals->Lookup( VarName, value );
	}
	ScriptDebugLog(ID_VARIABLES, "CheckVariable %s%s: %d", Context, VarName, value);
	return value;
}

// checks if a variable exists in any context
bool VariableExists(Scriptable *Sender, const char *VarName, const char *Context)
{
	ieDword value = 0;
	char newVarName[8];
	strlcpy(newVarName, Context, 7);
	Game *game = core->GetGame();

	if (Sender->GetCurrentArea()->locals->Lookup(VarName, value)) {
		return true;
	} else if (Sender->locals->Lookup(VarName, value)) {
		return true;
	} else if (HasKaputz && game->kaputz->Lookup(VarName, value)) {
		return true;
	} else if (game->locals->Lookup(VarName, value)) {
		return true;
	} else {
		Map *map = game->GetMap(game->FindMap(newVarName));
		if (map) {
			if (map->locals->Lookup(VarName, value)) {
				return true;
			}
		}
	}
	return false;
}

int DiffCore(ieDword a, ieDword b, int diffmode)
{
	switch (diffmode) {
		case LESS_THAN:
			if (a<b) {
				return 1;
			}
			break;
		case EQUALS:
			if (a==b) {
				return 1;
			}
			break;
		case GREATER_THAN:
			if (a>b) {
				return 1;
			}
			break;
		case GREATER_OR_EQUALS:
			if (a>=b) {
				return 1;
			}
			break;
		case NOT_EQUALS:
			if (a!=b) {
				return 1;
			}
			break;
		case BINARY_LESS_OR_EQUALS:
			if ((a&b) == a) {
				return 1;
			}
			break;
		case BINARY_MORE:
			if ((a&b) != a) {
				return 1;
			}
			break;
		case BINARY_MORE_OR_EQUALS:
			if ((a&b) == b) {
				return 1;
			}
			break;
		case BINARY_LESS:
			if ((a&b) != b) {
				return 1;
			}
			break;
		case BINARY_INTERSECT:
			if (a&b) {
				return 1;
			}
			break;
		case BINARY_NOT_INTERSECT:
			if (!(a&b)) {
				return 1;
			}
			break;
		default: //less or equals
			if (a<=b) {
				return 1;
			}
			break;
	}
	return 0;
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
	ac = (Actor *) tgts->GetTarget(0, ST_ACTOR);
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
	ac = (Actor *) tgts->GetTarget(0, ST_ACTOR);
	delete tgts;
	return ac;
}

Point GetEntryPoint(const char *areaname, const char *entryname)
{
	Point p;

	AutoTable tab("entries");
	if (!tab) {
		return p;
	}
	const char *tmpstr = tab->QueryField(areaname, entryname);
	short x = -1;
	short y = -1;
	sscanf(tmpstr, "%hd.%hd", &x, &y);
	p.x = x;
	p.y = y;
	return p;
}

/* returns a spell's casting distance, it depends on the caster (level), and targeting mode too
 the used header is calculated from the caster level */
unsigned int GetSpellDistance(const ieResRef spellres, Scriptable *Sender)
{
	unsigned int dist;

	Spell* spl = gamedata->GetSpell( spellres );
	if (!spl) {
		Log(ERROR, "GameScript", "Spell couldn't be found:%.8s.", spellres);
		return 0;
	}
	dist = spl->GetCastingDistance(Sender);
	gamedata->FreeSpell(spl, spellres, false);

	//make possible special return values (like 0xffffffff means the spell doesn't need distance)
	//this is used with special targeting mode (3)
	if (dist>0xff000000) {
		return dist;
	}
	return dist * VOODOO_SPL_RANGE_F;
}

/* returns an item's casting distance, it depends on the used header, and targeting mode too
 the used header is explictly given */
unsigned int GetItemDistance(const ieResRef itemres, int header)
{
	unsigned int dist;

	Item* itm = gamedata->GetItem( itemres );
	if (!itm) {
		Log(ERROR, "GameScript", "Item couldn't be found:%.8s.", itemres);
		return 0;
	}
	dist=itm->GetCastingDistance(header);
	gamedata->FreeItem(itm, itemres, false);

	//make possible special return values (like 0xffffffff means the item doesn't need distance)
	//this is used with special targeting mode (3)
	if (dist>0xff000000) {
		return dist;
	}
	return dist*VOODOO_ITM_RANGE_F;
}

//read the wish 2da
void SetupWishCore(Scriptable *Sender, int column, int picks)
{
	int count;
	ieVariable varname;
	int *selects;
	int i,j;

	// in the original, picks was at first the number of wish choices to set up,
	// but then it was hard coded to 5 (and SetupWishObject disused)
	if (picks == 1) picks = 5;

	AutoTable tm("wish");
	if (!tm) {
		Log(ERROR, "GameScript", "Cannot find wish.2da.");
		return;
	}

	selects = (int *) malloc(picks*sizeof(int));
	count = tm->GetRowCount();
	// handle the unused SetupWishObject, which passes WIS instead of a column
	// just cutting the 1-25 range into four pieces (roughly how the djinn dialog works)
	int cols = tm->GetColumnCount();
	if (column > cols) {
		column = (column-1)/6;
		if (column == 4) column = RAND(0, 3);
	}

	for(i=0;i<99;i++) {
		snprintf(varname,32, "wishpower%02d", i);
		if(CheckVariable(Sender, varname, "GLOBAL") ) {
			SetVariable(Sender, varname, "GLOBAL", 0);
		}
	}

	if (count<picks) {
		for(i=0;i<count;i++) {
			selects[i]=i;
		}
		while(i++<picks) {
			selects[i]=-1;
		}
	} else {
		for(i=0;i<picks;i++) {
			selects[i]=RAND(0, count-1);
retry:
			for(j=0;j<i;j++) {
				if(selects[i]==selects[j]) {
					selects[i]++;
					goto retry;
				}
			}
		}
	}

	for (i = 0; i < picks; i++) {
		if (selects[i]<0)
			continue;
		int spnum = atoi( tm->QueryField( selects[i], column-1 ) );
		snprintf(varname,32,"wishpower%02d", spnum);
		SetVariable(Sender, varname, "GLOBAL",1);
	}
	free(selects);
}

void AmbientActivateCore(Scriptable *Sender, Action *parameters, int flag)
{
	AreaAnimation* anim = Sender->GetCurrentArea( )->GetAnimation( parameters->string0Parameter);
	if (!anim) {
		anim = Sender->GetCurrentArea( )->GetAnimation( parameters->objects[1]->objectName );
	}
	if (!anim) {
		// iwd2 expects this behaviour in ar6001 by (de)activating sound_portal
		AmbientMgr *ambientmgr = core->GetAudioDrv()->GetAmbientMgr();
		if (flag) {
			ambientmgr->activate(parameters->objects[1]->objectName);
		} else {
			ambientmgr->deactivate(parameters->objects[1]->objectName);
		}
		return;
	}
	int i;
	if (flag) {
		anim->Flags |= A_ANI_ACTIVE;
		for (i=0; i<anim->animcount; i++) {
			anim->animation[i]->Flags |= A_ANI_ACTIVE;
		}
	} else {
		anim->Flags &= ~A_ANI_ACTIVE;
		for (i=0; i<anim->animcount; i++) {
			anim->animation[i]->Flags &= ~A_ANI_ACTIVE;
		}
	}
}

#define MAX_ISLAND_POLYGONS  10

//read a polygon 2da
Gem_Polygon *GetPolygon2DA(ieDword index)
{
	ieResRef resref;

	if (index>=MAX_ISLAND_POLYGONS) {
		return NULL;
	}

	if (!polygons) {
		polygons = (Gem_Polygon **) calloc(MAX_ISLAND_POLYGONS, sizeof(Gem_Polygon *) );
	}
	if (polygons[index]) {
		return polygons[index];
	}
	snprintf(resref, sizeof(ieResRef), "ISLAND%02d", index);
	AutoTable tm(resref);
	if (!tm) {
		return NULL;
	}
	int cnt = tm->GetRowCount();
	if (!cnt) {
		return NULL;
	}
	Point *p = new Point[cnt];

	int i = cnt;
	while(i--) {
		p[i].x = atoi(tm->QueryField(i, 0));
		p[i].y = atoi(tm->QueryField(i, 1));
	}

	polygons[index] = new Gem_Polygon(p, cnt, NULL);
	delete [] p;
	return polygons[index];
}

static bool InterruptSpellcasting(Scriptable* Sender) {
	if (Sender->Type != ST_ACTOR) return false;
	Actor *caster = (Actor *) Sender;

	// ouch, we got hit
	if (Sender->InterruptCasting) {
		if (caster->InParty) {
			displaymsg->DisplayConstantString(STR_SPELLDISRUPT, DMC_WHITE, Sender);
		} else {
			displaymsg->DisplayConstantStringName(STR_SPELL_FAILED, DMC_WHITE, Sender);
		}
		DisplayStringCore(Sender, VB_SPELL_DISRUPTED, DS_CONSOLE|DS_CONST );
		return true;
	}

	// abort casting on invisible or dead targets
	// not all spells should be interrupted on death - some for chunking, some for raising the dead
	if (Sender->LastSpellTarget) {
		Actor *target = core->GetGame()->GetActorByGlobalID(Sender->LastSpellTarget);
		if (target) {
			ieDword state = target->GetStat(IE_STATE_ID);
			if (state & STATE_DEAD) {
				if (state & ~(STATE_PETRIFIED|STATE_FROZEN)) {
					Spell* spl = gamedata->GetSpell(Sender->SpellResRef, true);
					if (!spl) return false;
					SPLExtHeader *seh = spl->GetExtHeader(0); // potentially wrong, but none of the existing spells is problematic
					bool invalidTarget = seh && seh->Target != TARGET_DEAD;
					gamedata->FreeSpell(spl, Sender->SpellResRef, false);
					if (invalidTarget) {
						if (caster->InParty) {
							core->Autopause(AP_NOTARGET, caster);
						}
						caster->SetStance(IE_ANI_READY);
						return true;
					}
				}
			}
		}
	}
	return false;
}

// shared spellcasting action code for casting on scriptables
void SpellCore(Scriptable *Sender, Action *parameters, int flags)
{
	ieResRef spellres = {};
	int level = 0;
	static bool third = core->HasFeature(GF_3ED_RULES);

	// handle iwd2 marked spell casting (MARKED_SPELL is 0)
	// NOTE: supposedly only casting via SpellWait checks this, so refactor if needed
	if (third && parameters->int0Parameter == 0 && !parameters->string0Parameter[0]) {
		if (!Sender->LastMarkedSpell) {
			// otherwise we spam a lot
			Sender->ReleaseCurrentAction();
			return;
		}
		ResolveSpellName(spellres, Sender->LastMarkedSpell);
	}

	//resolve spellname
	if (!spellres[0] && !ResolveSpellName(spellres, parameters)) {
		Sender->ReleaseCurrentAction();
		return;
	} else {
		if (!Sender->SpellResRef[0] || stricmp(Sender->SpellResRef, spellres)) {
			if (Sender->CurrentActionTicks) {
				Log(WARNING, "GameScript", "SpellCore: Action (%d) lost spell somewhere!", parameters->actionID);
			}
			Sender->SetSpellResRef(spellres);
		}
	}
	if (!Sender->CurrentActionTicks) {
		parameters->int2Parameter = 1;
	}

	// use the passed level instead of the caster's casting level
	if (flags&SC_SETLEVEL) {
		if (parameters->string0Parameter[0]) {
			level = parameters->int0Parameter;
		} else {
			level = parameters->int1Parameter;
		}
	}

	Actor *act = NULL;
	if (Sender->Type==ST_ACTOR) {
		act = (Actor *) Sender;
	}

	//parse target
	int seeflag = 0;
	unsigned int dist = GetSpellDistance(spellres, Sender);
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

	if (act) {
		//move near to target
		if ((flags&SC_RANGE_CHECK) && dist != 0xffffffff) {
			if (PersonalDistance(tar, Sender) > dist) {
				MoveNearerTo(Sender, tar, dist);
				return;
			}
			if (!Sender->GetCurrentArea()->IsVisibleLOS(Sender->Pos, tar->Pos)) {
				Spell *spl = gamedata->GetSpell(Sender->SpellResRef, true);
				if (!(spl->Flags&SF_NO_LOS)) {
					gamedata->FreeSpell(spl, Sender->SpellResRef, false);
					MoveNearerTo(Sender, tar, dist);
					return;
				}
				gamedata->FreeSpell(spl, Sender->SpellResRef, false);
			}
		}

		//face target
		if (tar != Sender) {
			act->SetOrientation( GetOrient( tar->Pos, act->Pos ), false );
		}

		//stop doing anything else
		act->SetModal(MS_NONE);
	}

	if ((flags&SC_AURA_CHECK) && parameters->int2Parameter && Sender->AuraPolluted()) {
		return;
	}

	int duration;
	if (!parameters->int2Parameter) {
		duration = Sender->CurrentActionState--;
	} else {
		duration = Sender->CastSpell( tar, flags&SC_DEPLETE, flags&SC_INSTANT, flags&SC_NOINTERRUPT );
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
	} else if(!Sender->LastTargetPos.isempty()) {
		//the target was converted to a point
		Sender->CastSpellPointEnd(level, flags&SC_INSTANT);
	} else {
		Log(ERROR, "GameScript", "SpellCore: Action (%d) lost target somewhere!", parameters->actionID);
	}
	parameters->int2Parameter = 0;
	Sender->ReleaseCurrentAction();
}


// shared spellcasting action code for casting on the ground
void SpellPointCore(Scriptable *Sender, Action *parameters, int flags)
{
	ieResRef spellres;
	int level = 0;

	//resolve spellname
	if (!ResolveSpellName( spellres, parameters) ) {
		Sender->ReleaseCurrentAction();
		return;
	} else {
		if (!Sender->SpellResRef[0] || stricmp(Sender->SpellResRef, spellres)) {
			if (Sender->CurrentActionTicks) {
				Log(WARNING, "GameScript", "SpellPointCore: Action (%d) lost spell somewhere!", parameters->actionID);
			}
			Sender->SetSpellResRef(spellres);
		}
	}
	if (!Sender->CurrentActionTicks) {
		parameters->int2Parameter = 1;
	}

	// use the passed level instead of the caster's casting level
	if (flags&SC_SETLEVEL) {
		if (parameters->string0Parameter[0]) {
			level = parameters->int0Parameter;
		} else {
			level = parameters->int1Parameter;
		}
	}

	if(Sender->Type==ST_ACTOR) {
		unsigned int dist = GetSpellDistance(spellres, Sender);

		Actor *act = (Actor *) Sender;
		//move near to target
		if (flags&SC_RANGE_CHECK) {
			if (PersonalDistance(parameters->pointParameter, Sender) > dist) {
				MoveNearerTo(Sender, parameters->pointParameter, dist, 0);
				return;
			}
			if (!Sender->GetCurrentArea()->IsVisibleLOS(Sender->Pos, parameters->pointParameter)) {
				Spell *spl = gamedata->GetSpell(Sender->SpellResRef, true);
				if (!(spl->Flags&SF_NO_LOS)) {
					gamedata->FreeSpell(spl, Sender->SpellResRef, false);
					MoveNearerTo(Sender, parameters->pointParameter, dist, 0);
					return;
				}
				gamedata->FreeSpell(spl, Sender->SpellResRef, false);
			}
		}

		//face target
		act->SetOrientation( GetOrient( parameters->pointParameter, act->Pos ), false );
		//stop doing anything else
		act->SetModal(MS_NONE);
	}

	if ((flags&SC_AURA_CHECK) && parameters->int2Parameter && Sender->AuraPolluted()) {
		return;
	}

	int duration;
	if (!parameters->int2Parameter) {
		duration = Sender->CurrentActionState--;
	} else {
		duration = Sender->CastSpellPoint( parameters->pointParameter, flags&SC_DEPLETE, flags&SC_INSTANT, flags&SC_NOINTERRUPT );
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

	if(!Sender->LastTargetPos.isempty()) {
		//if target was set, fire spell
		Sender->CastSpellPointEnd(level, flags&SC_INSTANT);
	} else {
		Log(ERROR, "GameScript", "SpellPointCore: Action (%d) lost target somewhere!", parameters->actionID);
	}
	Sender->ReleaseCurrentAction();
}

void AddXPCore(Action *parameters, bool divide)
{
	AutoTable xptable;

	if (core->HasFeature(GF_HAS_EXPTABLE)) {
		xptable.load("exptable");
	} else {
		xptable.load("xplist");
	}

	if (parameters->int0Parameter > 0 && core->HasFeedback(FT_MISC)) {
		displaymsg->DisplayString(parameters->int0Parameter, DMC_BG2XPGREEN, IE_STR_SOUND);
	}
	if (!xptable) {
		Log(ERROR, "GameScript", "Can't perform AddXP2DA/AddXPVar!");
		return;
	}
	const char *xpvalue = xptable->QueryField(parameters->string0Parameter, "0"); // level is unused

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

}
