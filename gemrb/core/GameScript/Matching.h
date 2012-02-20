/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */
#ifndef MATCHING_H
#define MATCHING_H

#include "GameScript/GameScript.h"

#include "exports.h"

namespace GemRB {

class TileMap;

GEM_EXPORT Targets* GetAllObjects(Map *map, Scriptable* Sender, Object* oC, int ga_flags);
Targets* GetAllActors(Scriptable* Sender, int ga_flags);
Scriptable* GetActorFromObject(Scriptable* Sender, Object* oC, int ga_flags = 0);
Scriptable* GetStoredActorFromObject(Scriptable* Sender, Object* oC, int ga_flags = 0);
Scriptable *GetActorObject(TileMap *TMap, const char *name);

Targets *GetMyTarget(Scriptable *Sender, Actor *actor, Targets *parameters, int ga_flags);
Targets *XthNearestOf(Targets *parameters, int count, int ga_flags);
Targets *XthNearestDoor(Targets *parameters, unsigned int count);
Targets *XthNearestEnemyOf(Targets *parameters, int count, int ga_flags);
Targets *ClosestEnemySummoned(Scriptable *origin, Targets *parameters, int ga_flags);
Targets *XthNearestEnemyOfType(Scriptable *origin, Targets *parameters, unsigned int count, int ga_flags);
Targets *XthNearestMyGroupOfType(Scriptable *origin, Targets *parameters, unsigned int count, int ga_flags);

/* returns true if actor matches the object specs. */
bool MatchActor(Scriptable *Sender, ieDword ID, Object* oC);
/* returns the number of actors matching the IDS targeting */
int GetObjectCount(Scriptable* Sender, Object* oC);
int GetObjectLevelCount(Scriptable* Sender, Object* oC);

}

#endif
