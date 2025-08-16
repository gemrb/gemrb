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

#include "exports-core.h"
#include "ie_types.h"

namespace GemRB {

class Action;
class Actor;
class Map;
class Object;
class Scriptable;
class Targets;
class TileMap;
class Trigger;

enum class GroupType {
	Enemy,
	PC,
	Neutral, // has no enemies
};

GEM_EXPORT Targets* GetAllObjects(const Map* map, Scriptable* Sender, const Object* oC, int ga_flags, bool anyone = false);
GEM_EXPORT Targets* GetAllObjects(const Map* map, Scriptable* Sender, const Trigger* parameters, int gaFlags);
GEM_EXPORT Targets* GetAllObjects(const Map* map, Scriptable* Sender, const Action* parameters, int gaFlags);
Targets* GetAllActors(Scriptable* Sender, int ga_flags);
Scriptable* GetScriptableFromObject(Scriptable* Sender, const Trigger* parameters, int gaFlags = 0);
Scriptable* GetScriptableFromObject(Scriptable* Sender, const Action* parameters, int gaFlags = 0);
Scriptable* GetScriptableFromObject2(Scriptable* Sender, const Action* parameters, int gaFlags = 0);
Scriptable* GetScriptableFromObject(Scriptable* Sender, const Object* oC, int gaFlags = 0, bool anyone = false);
Scriptable* GetStoredActorFromObject(Scriptable* Sender, const Action* parameters, int gaFlags = 0);
Scriptable* GetStoredActorFromObject(Scriptable* Sender, const Object* oC, int ga_flags = 0, bool anyone = false);
Scriptable* GetActorObject(const TileMap* TMap, const ieVariable& name);

Targets* GetMyTarget(const Scriptable* Sender, const Actor* actor, Targets* parameters, int ga_flags);
Targets* XthNearestOf(Targets* parameters, int count, int ga_flags);
Targets* XthNearestDoor(Targets* parameters, unsigned int count);
Targets* XthNearestEnemyOf(Targets* parameters, int count, int gaFlags, bool farthest = false);
Targets* ClosestEnemySummoned(const Scriptable* origin, Targets* parameters, int ga_flags);
Targets* XthNearestEnemyOfType(const Scriptable* origin, Targets* parameters, unsigned int count, int ga_flags);
Targets* XthNearestMyGroupOfType(const Scriptable* origin, Targets* parameters, unsigned int count, int ga_flags);

/* returns true if actor matches the object specs. */
bool MatchActor(const Scriptable* Sender, ieDword ID, const Object* oC);
/* returns the number of actors matching the IDS targeting */
int GetObjectCount(Scriptable* Sender, const Trigger* parameters);
int GetObjectCount(Scriptable* Sender, const Object* oC, bool anyone = false);
int GetObjectLevelCount(Scriptable* Sender, const Trigger* parameters);

}

#endif
