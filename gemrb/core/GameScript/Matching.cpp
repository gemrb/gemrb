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

#include "GameScript/Matching.h"

#include "GameScript/GSUtils.h"

#include "Interface.h"
#include "Game.h"
#include "Map.h"
#include "TileMap.h"
#include "Scriptable/Container.h"
#include "Scriptable/Door.h"
#include "Scriptable/InfoPoint.h"

namespace GemRB {

/* return a Targets object with a single scriptable inside */
static inline Targets* ReturnScriptableAsTarget(Scriptable *sc)
{
	if (!sc) return NULL;
	Targets *tgts = new Targets();
	tgts->AddTarget(sc, 0, 0);
	return tgts;
}

/* do IDS filtering: [PC], [ENEMY], etc */
static inline bool DoObjectIDSCheck(Object *oC, Actor *ac, bool *filtered) {
	for (int j = 0; j < ObjectIDSCount; j++) {
		if (!oC->objectFields[j]) {
			continue;
		}
		*filtered = true;
		IDSFunction func = idtargets[j];
		if (!func) {
			Log(WARNING, "GameScript", "Unimplemented IDS targeting opcode: %d", j);
			continue;
		}
		if (!func( ac, oC->objectFields[j] ) ) {
			return false;
		}
	}
	return true;
}

/* do object filtering: Myself, LastAttackerOf(Player1), etc */
static inline Targets *DoObjectFiltering(Scriptable *Sender, Targets *tgts, Object *oC, int ga_flags) {
	for (int i = 0; i < MaxObjectNesting; i++) {
		int filterid = oC->objectFilters[i];
		if (!filterid) break;
		if (filterid < 0) continue;

		ObjectFunction func = objects[filterid];
		if (!func) {
			Log(WARNING, "GameScript", "Unknown object filter: %d %s",
				filterid, objectsTable->GetValue(filterid));
			continue;
		}

		tgts = func(Sender, tgts, ga_flags);
		if (!tgts->Count()) {
			delete tgts;
			return NULL;
		}
	}
	return tgts;
}

static EffectRef fx_protection_creature_ref = { "Protection:Creature", -1 };

static inline bool DoObjectChecks(Map *map, Scriptable *Sender, Actor *target, int &dist, bool ignoreinvis=false)
{
	dist = SquaredMapDistance(Sender, target);

	// TODO: what do we check for non-actors?
	// non-actors have a visual range (15), we should do visual range and LOS

	if (Sender->Type == ST_ACTOR) {
		Actor *source = (Actor *)Sender;

		// Detect() ignores invisibility completely
		if (!ignoreinvis && target->IsInvisibleTo(source)) {
			return false;
		}

		// visual range check
		int visualrange = source->Modified[IE_VISUALRANGE];
		if (dist > visualrange*visualrange) return false;

		// LOS check
		if (!map->IsVisible(Sender->Pos, target->Pos)) return false;

		// protection against creature
		if (target->fxqueue.HasEffect(fx_protection_creature_ref)) {
			// TODO: de-hardcode these (may not all be correct anyway)
			if (target->fxqueue.HasEffectWithParamPair(fx_protection_creature_ref, 2, source->Modified[IE_EA])) return false;
			if (target->fxqueue.HasEffectWithParamPair(fx_protection_creature_ref, 3, source->Modified[IE_GENERAL])) return false;
			if (target->fxqueue.HasEffectWithParamPair(fx_protection_creature_ref, 4, source->Modified[IE_RACE])) return false;
			if (target->fxqueue.HasEffectWithParamPair(fx_protection_creature_ref, 5, source->Modified[IE_CLASS])) return false;
			if (target->fxqueue.HasEffectWithParamPair(fx_protection_creature_ref, 6, source->Modified[IE_SPECIFIC])) return false;
			if (target->fxqueue.HasEffectWithParamPair(fx_protection_creature_ref, 7, source->Modified[IE_SEX])) return false;
			if (target->fxqueue.HasEffectWithParamPair(fx_protection_creature_ref, 8, source->Modified[IE_ALIGNMENT])) return false;
		}
	}
	return true;
}

/* returns actors that match the [x.y.z] expression */
static Targets* EvaluateObject(Map *map, Scriptable* Sender, Object* oC, int ga_flags)
{
	// if you ActionOverride a global actor, they might not have a map :(
	// TODO: don't allow this to happen?
	if (!map) {
		return NULL;
	}

	if (oC->objectName[0]) {
		//We want the object by its name...
		Scriptable* aC = map->GetActor( oC->objectName, ga_flags );

		/*if (!aC && (ga_flags&GA_GLOBAL) ) {
			aC = FindActorNearby(oC->objectName, map, ga_flags );
		}*/

		//This order is the same as in GetActorObject
		//TODO:merge them
		if (!aC) {
			aC = map->GetTileMap()->GetDoor(oC->objectName);
		}
		if (!aC) {
			aC = map->GetTileMap()->GetContainer(oC->objectName);
		}
		if (!aC) {
			aC = map->GetTileMap()->GetInfoPoint(oC->objectName);
		}

		//return here because object name/IDS targeting are mutually exclusive
		return ReturnScriptableAsTarget(aC);
	}

	if (oC->objectFields[0]==-1) {
		// this is an internal hack, allowing us to pass actor ids around as objects
		Actor* aC = map->GetActorByGlobalID( (ieDword) oC->objectFields[1] );
		if (aC) {
			if (!aC->ValidTarget(ga_flags)) {
				return NULL;
			}
			return ReturnScriptableAsTarget(aC);
		}
		Door *door = map->GetDoorByGlobalID( (ieDword) oC->objectFields[1]);
		if (door) {
			return ReturnScriptableAsTarget(door);
		}

		Container* cont = map->GetContainerByGlobalID((ieDword) oC->objectFields[1]);
		if (cont) {
			return ReturnScriptableAsTarget(cont);
		}

		InfoPoint* trap = map->GetInfoPointByGlobalID((ieDword) oC->objectFields[1]);
		if (trap) {
			return ReturnScriptableAsTarget(trap);
		}

		return NULL;
	}

	Targets *tgts = NULL;

	//we need to get a subset of actors from the large array
	//if this gets slow, we will need some index tables
	int i = map->GetActorCount(true);
	while (i--) {
		Actor *ac = map->GetActor(i, true);
		if (!ac) continue; // is this check really needed?
		// don't return Sender in IDS targeting!
		if (ac == Sender) continue;
		bool filtered = false;
		if (DoObjectIDSCheck(oC, ac, &filtered)) {
			// this is needed so eg. Range trigger gets a good object
			// HACK: our parsing of Attack([0]) is broken
			if (!filtered) {
				// if no filters were applied..
				assert(!tgts);
				return NULL;
			}
			int dist;
			if (DoObjectChecks(map, Sender, ac, dist, (ga_flags & GA_DETECT) != 0)) {
				if (!tgts) tgts = new Targets();
				tgts->AddTarget((Scriptable *) ac, dist, ga_flags);
			}
		}
	}

	return tgts;
}

Targets* GetAllObjects(Map *map, Scriptable* Sender, Object* oC, int ga_flags)
{
	if (!oC) {
		//return all objects
		return GetAllActors(Sender, ga_flags);
	}
	Targets* tgts = EvaluateObject(map, Sender, oC, ga_flags);
	//if we couldn't find an endpoint by name or object qualifiers
	//it is not an Actor, but could still be a Door or Container (scriptable)
	if (!tgts && oC->objectName[0]) {
		return NULL;
	}
	//now lets do the object filter stuff, we create Targets because
	//it is possible to start from blank sheets using endpoint filters
	//like (Myself, Protagonist etc)
	if (!tgts) {
		tgts = new Targets();
	}
	tgts = DoObjectFiltering(Sender, tgts, oC, ga_flags);
	return tgts;
}

Targets *GetAllActors(Scriptable *Sender, int ga_flags)
{
	Map *map = Sender->GetCurrentArea();

	int i = map->GetActorCount(true);
	Targets *tgts = new Targets();
	while (i--) {
		Actor *ac = map->GetActor(i,true);
		int dist = Distance(Sender->Pos, ac->Pos);
		tgts->AddTarget((Scriptable *) ac, dist, ga_flags);
	}
	return tgts;
}

/* get a non-actor object from a map, by name */
Scriptable *GetActorObject(TileMap *TMap, const char *name)
{
	Scriptable * aC = TMap->GetDoor( name );
	if (aC) {
		return aC;
	}

	//containers should have a precedence over infopoints because otherwise
	//AR1512 sanity test quest would fail
	//If this order couldn't be maintained, then 'Contains' should have a
	//unique call to get containers only

	//No... it was not an door... maybe a Container?
	aC = TMap->GetContainer( name );
	if (aC) {
		return aC;
	}

	//No... it was not a container ... maybe an InfoPoint?
	aC = TMap->GetInfoPoint( name );
	if (aC) {
		return aC;
	}
	return aC;
}

// blocking actions need to store some kinds of objects between ticks
Scriptable* GetStoredActorFromObject(Scriptable* Sender, Object* oC, int ga_flags)
{
	Scriptable *tar = NULL;
	// retrieve an existing target if it still exists and is valid
	if (Sender->CurrentActionTarget) {
		tar = core->GetGame()->GetActorByGlobalID(Sender->CurrentActionTarget);
		if (tar) {
			// always an actor, check if it satisfies flags
			if (((Actor *)tar)->ValidTarget(ga_flags)) {
				return tar;
			}
		}
		return NULL; // target invalid/gone
	}
	tar = GetActorFromObject(Sender, oC, ga_flags);
	// maybe store the target if it's an actor..
	if (tar && tar->Type == ST_ACTOR) {
		// .. but we only want objects created via objectFilters
		if (oC->objectFilters[0]) {
			Sender->CurrentActionTarget = tar->GetGlobalID();
		}
	}
	return tar;
}

Scriptable* GetActorFromObject(Scriptable* Sender, Object* oC, int ga_flags)
{
	Scriptable *aC = NULL;

	Game *game = core->GetGame();
	Targets *tgts = GetAllObjects(Sender->GetCurrentArea(), Sender, oC, ga_flags);
	if (tgts) {
		//now this could return other than actor objects
		aC = tgts->GetTarget(0,-1);
		delete tgts;
		if (aC || oC->objectFields[0]!=-1) {
			return aC;
		}

		//global actors are always found by object ID!
		return game->GetGlobalActorByGlobalID(oC->objectFields[1]);
	}

	if (!oC) {
		return NULL;
	}

	if (oC->objectName[0]) {
		// if you ActionOverride a global actor, they might not have a map :(
		// TODO: don't allow this to happen?
		if (Sender->GetCurrentArea()) {
			aC = GetActorObject(Sender->GetCurrentArea()->GetTileMap(), oC->objectName );
			if (aC) {
				return aC;
			}
		}

		//global actors are always found by scripting name!
		aC = game->FindPC(oC->objectName);
		if (aC) {
			return aC;
		}
		aC = game->FindNPC(oC->objectName);
		if (aC) {
			return aC;
		}
	}
	return NULL;
}

bool MatchActor(Scriptable *Sender, ieDword actorID, Object* oC)
{
	if (!Sender) {
		return false;
	}
	Actor *ac = Sender->GetCurrentArea()->GetActorByGlobalID(actorID);
	if (!ac) {
		return false;
	}

	// [0]/[ANYONE] can match all actors
	if (!oC) {
		return true;
	}

	bool filtered = false;

	// name matching
	if (oC->objectName[0]) {
		if (strnicmp(ac->GetScriptName(), oC->objectName, 32) != 0) {
			return false;
		}
		filtered = true;
	}

	// IDS targeting
	// (if we already matched by name, we don't do this)
	// TODO: check distance? area? visibility?
	if (!filtered && !DoObjectIDSCheck(oC, ac, &filtered)) return false;

	// globalID hack should never get here
	assert(oC->objectFilters[0] != -1);

	// object filters
	if (oC->objectFilters[0]) {
		// object filters insist on having a stupid targets list,
		// so we waste a lot of time here
		Targets *tgts = new Targets();
		int ga_flags = 0; // TODO: correct?

		// handle already-filtered vs not-yet-filtered cases
		// e.g. LastTalkedToBy(Myself) vs LastTalkedToBy
		if (filtered) tgts->AddTarget(ac, 0, ga_flags);

		tgts = DoObjectFiltering(Sender, tgts, oC, ga_flags);
		if (!tgts) return false;

		// and sometimes object filters are lazy and not only don't filter
		// what we give them, they clear it and return a list :(
		// so we have to search the whole list..
		bool ret = false;
		targetlist::iterator m;
		const targettype *tt = tgts->GetFirstTarget(m, ST_ACTOR);
		while (tt) {
			Actor *actor = (Actor *) tt->actor;
			if (actor->GetGlobalID() == actorID) {
				ret = true;
				break;
			}
			tt = tgts->GetNextTarget(m, ST_ACTOR);
		}
		delete tgts;
		if (!ret) return false;
	}
	return true;
}

int GetObjectCount(Scriptable* Sender, Object* oC)
{
	if (!oC) {
		return 0;
	}
	// EvaluateObject will return [PC]
	// GetAllObjects will also return Myself (evaluates object filters)
	// i believe we need the latter here
	Targets* tgts = GetAllObjects(Sender->GetCurrentArea(), Sender, oC, 0);
	int count = 0; // silly fallback to avoid potential crashes
	if (tgts) {
		count = tgts->Count();
		delete tgts;
	}
	return count;
}

//TODO:
//check numcreaturesatmylevel(myself, 1)
//when the actor is alone
//it should (obviously) return true if the trigger
//evaluates object filters
//also check numcreaturesgtmylevel(myself,0) with
//actor having at high level
int GetObjectLevelCount(Scriptable* Sender, Object* oC)
{
	if (!oC) {
		return 0;
	}
	// EvaluateObject will return [PC]
	// GetAllObjects will also return Myself (evaluates object filters)
	// i believe we need the latter here
	Targets* tgts = GetAllObjects(Sender->GetCurrentArea(), Sender, oC, 0);
	int count = 0;
	if (tgts) {
		targetlist::iterator m;
		const targettype *tt = tgts->GetFirstTarget(m, ST_ACTOR);
		while (tt) {
			count += ((Actor *) tt->actor)->GetXPLevel(true);
			tt = tgts->GetNextTarget(m, ST_ACTOR);
		}
	}
	delete tgts;
	return count;
}

Targets *GetMyTarget(Scriptable *Sender, Actor *actor, Targets *parameters, int ga_flags)
{
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastTarget);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets *XthNearestDoor(Targets *parameters, unsigned int count)
{
	//get the origin
	Scriptable *origin = parameters->GetTarget(0, -1);
	parameters->Clear();
	if (!origin) {
		return parameters;
	}
	//get the doors based on it
	Map *map = origin->GetCurrentArea();
	unsigned int i =(unsigned int) map->TMap->GetDoorCount();
	if (count>i) {
		return parameters;
	}
	while (i--) {
		Door *door = map->TMap->GetDoor(i);
		unsigned int dist = Distance(origin->Pos, door->Pos);
		parameters->AddTarget(door, dist, 0);
	}

	//now get the xth door
	origin = parameters->GetTarget(count, ST_DOOR);
	parameters->Clear();
	if (!origin) {
		return parameters;
	}
	parameters->AddTarget(origin, 0, 0);
	return parameters;
}

Targets *XthNearestOf(Targets *parameters, int count, int ga_flags)
{
	Scriptable *origin;

	if (count<0) {
		const targettype *t = parameters->GetLastTarget(ST_ACTOR);
		origin = t->actor;
	} else {
		origin = parameters->GetTarget(count, ST_ACTOR);
	}
	parameters->Clear();
	if (!origin) {
		return parameters;
	}
	parameters->AddTarget(origin, 0, ga_flags);
	return parameters;
}

//mygroup means the same specifics as origin
Targets *XthNearestMyGroupOfType(Scriptable *origin, Targets *parameters, unsigned int count, int ga_flags)
{
	if (origin->Type != ST_ACTOR) {
		parameters->Clear();
		return parameters;
	}

	targetlist::iterator m;
	const targettype *t = parameters->GetFirstTarget(m, ST_ACTOR);
	if (!t) {
		return parameters;
	}
	Actor *actor = (Actor *) origin;
	//determining the specifics of origin
	ieDword type = actor->GetStat(IE_SPECIFIC); //my group

	while ( t ) {
		if (t->actor->Type!=ST_ACTOR) {
			t=parameters->RemoveTargetAt(m);
			continue;
		}
		Actor *actor = (Actor *) (t->actor);
		if (actor->GetStat(IE_SPECIFIC) != type) {
			t=parameters->RemoveTargetAt(m);
			continue;
		}
		t = parameters->GetNextTarget(m, ST_ACTOR);
	}
	return XthNearestOf(parameters,count, ga_flags);
}

Targets *ClosestEnemySummoned(Scriptable *origin, Targets *parameters, int ga_flags)
{
	if (origin->Type != ST_ACTOR) {
		parameters->Clear();
		return parameters;
	}

	targetlist::iterator m;
	const targettype *t = parameters->GetFirstTarget(m, ST_ACTOR);
	if (!t) {
		return parameters;
	}
	Actor *actor = (Actor *) origin;
	//determining the allegiance of the origin
	int type = GetGroup(actor);

	if (type==2) {
		parameters->Clear();
		return parameters;
	}

	actor = NULL;
	while ( t ) {
		Actor *tmp = (Actor *) (t->actor);
		if (tmp->GetStat(IE_SEX) != SEX_SUMMON) {
			continue;
		}
		if (type) { //origin is PC
			if (tmp->GetStat(IE_EA) <= EA_GOODCUTOFF) {
				continue;
			}
		} else {
			if (tmp->GetStat(IE_EA) >= EA_EVILCUTOFF) {
				continue;
			}
		}
		actor = tmp;
		t = parameters->GetNextTarget(m, ST_ACTOR);
	}
	parameters->Clear();
	parameters->AddTarget(actor, 0, ga_flags);
	return parameters;
}

Targets *XthNearestEnemyOfType(Scriptable *origin, Targets *parameters, unsigned int count, int ga_flags)
{
	if (origin->Type != ST_ACTOR) {
		parameters->Clear();
		return parameters;
	}

	targetlist::iterator m;
	const targettype *t = parameters->GetFirstTarget(m, ST_ACTOR);
	if (!t) {
		return parameters;
	}
	Actor *actor = (Actor *) origin;
	//determining the allegiance of the origin
	int type = GetGroup(actor);

	if (type==2) {
		parameters->Clear();
		return parameters;
	}

	while ( t ) {
		if (t->actor->Type!=ST_ACTOR) {
			t=parameters->RemoveTargetAt(m);
			continue;
		}
		Actor *actor = (Actor *) (t->actor);
		// IDS targeting already did object checks (unless we need to override Detect?)
		if (type) { //origin is PC
			if (actor->GetStat(IE_EA) <= EA_GOODCUTOFF) {
				t=parameters->RemoveTargetAt(m);
				continue;
			}
		} else {
			if (actor->GetStat(IE_EA) >= EA_EVILCUTOFF) {
				t=parameters->RemoveTargetAt(m);
				continue;
			}
		}
		t = parameters->GetNextTarget(m, ST_ACTOR);
	}
	return XthNearestOf(parameters,count, ga_flags);
}

Targets *XthNearestEnemyOf(Targets *parameters, int count, int ga_flags)
{
	Actor *origin = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	parameters->Clear();
	if (!origin) {
		return parameters;
	}
	//determining the allegiance of the origin
	int type = GetGroup(origin);

	if (type==2) {
		return parameters;
	}
	Map *map = origin->GetCurrentArea();
	int i = map->GetActorCount(true);
	Actor *ac;
	while (i--) {
		ac=map->GetActor(i,true);
		int distance;
		//int distance = Distance(ac, origin);
		// TODO: if it turns out you need to check Sender here, beware you take the right distance!
		// (n the original games, this is only used for NearestEnemyOf(Player1) in obsgolem.bcs)
		if (!DoObjectChecks(map, origin, ac, distance)) continue;
		if (type) { //origin is PC
			if (ac->GetStat(IE_EA) >= EA_EVILCUTOFF) {
				parameters->AddTarget(ac, distance, ga_flags);
			}
		}
		else {
			if (ac->GetStat(IE_EA) <= EA_GOODCUTOFF) {
				parameters->AddTarget(ac, distance, ga_flags);
			}
		}
	}
	return XthNearestOf(parameters,count, ga_flags);
}


}
