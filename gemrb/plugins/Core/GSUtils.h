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
 * $Id$
 *
 */
#ifndef GSUTILS_H
#define GSUTILS_H
#include "Interface.h"
#include "GameScript.h"
#include "../../includes/strrefs.h"
#include "../../includes/defsounds.h"
//indebug flags
#define ID_REFERENCE 1
#define ID_CUTSCENE  2
#define ID_VARIABLES 4
#define ID_ACTIONS   8
#define ID_TRIGGERS  16

extern int initialized;
extern bool charactersubtitles;
extern SymbolMgr* triggersTable;
extern SymbolMgr* actionsTable;
extern SymbolMgr* objectsTable;
extern TriggerFunction triggers[MAX_TRIGGERS];
extern ActionFunction actions[MAX_ACTIONS];
extern short actionflags[MAX_ACTIONS];
extern short triggerflags[MAX_TRIGGERS];
extern ObjectFunction objects[MAX_OBJECTS];
extern IDSFunction idtargets[MAX_OBJECT_FIELDS];
extern Cache SrcCache; //cache for string resources (pst)
extern Cache BcsCache; //cache for scripts
extern int ObjectIDSCount;
extern int MaxObjectNesting;
extern bool HasAdditionalRect;
extern bool HasTriggerPoint;
extern ieResRef *ObjectIDSTableNames;
extern int ObjectFieldsCount;
extern int ExtraParametersCount;
extern int InDebug;
extern int *SkillStats;
extern int SkillCount;

#define MIC_INVALID -2
#define MIC_FULL -1
#define MIC_NOITEM 0
#define MIC_GOTITEM 1

int GetReaction(Scriptable *Sender);
int GetHappiness(Scriptable *Sender, int reputation);
int GetHPPercent(Scriptable *Sender);
bool StoreHasItemCore(const ieResRef storename, const ieResRef itemname);
bool HasItemCore(Inventory *inventory, const ieResRef itemname, ieDword flags);
void ClickCore(Scriptable *Sender, Point point, int type, int speed);
void TransformItemCore(Actor *actor, Action *parameters, bool onlyone);
void CreateVisualEffectCore(Actor *target, const char *effect, int iterations);
void CreateVisualEffectCore(Scriptable *Sender, Point &position, const char *effect, int iterations);
void GetPositionFromScriptable(Scriptable* scr, Point &position, bool trap);
void BeginDialog(Scriptable* Sender, Action* parameters, int flags);
void ChangeAnimationCore(Actor *src, const char *resref, bool effect);
void PolymorphCopyCore(Actor *src, Actor *tar, bool base);
void CreateCreatureCore(Scriptable* Sender, Action* parameters, int flags);
Targets* GetAllObjects(Scriptable* Sender, Object* oC, int ga_flags);
Scriptable* GetActorFromObject(Scriptable* Sender, Object* oC, int ga_flags = 0);
int MoveItemCore(Scriptable *Sender, Scriptable *target, const char *resref, int flags, int setflag);
void MoveToObjectCore(Scriptable *Sender, Action *parameters, ieDword flags, bool untilsee);
void CreateItemCore(CREItem *item, const char *resref, int a, int b, int c);
void AttackCore(Scriptable *Sender, Scriptable *target, Action *parameters, int flags);
void InitScriptTables();
void HandleBitMod(ieDword &value1, ieDword value2, int opcode);
bool ResolveSpellName(ieResRef spellres, Action *parameter);
bool ResolveItemName(ieResRef itemres, Actor *act, ieDword Slot);
void EscapeAreaCore(Actor *src, const char *resref, Point &enter, Point &exit, int flags);
void GoNear(Scriptable *Sender, Point &p);
void GoNearAndRetry(Scriptable *Sender, Scriptable *target, bool destination, int distance);
void GoNearAndRetry(Scriptable *Sender, Point &p, int distance);

#define NO_OPERATION -1
#define LESS_OR_EQUALS 0
//iwd2 diffmode with gemrb enhancements
#define EQUALS 1
#define LESS_THAN 2
#define GREATER_THAN 3
#define GREATER_OR_EQUALS 4
#define NOT_EQUALS 5
#define BINARY_LESS_OR_EQUALS 6 //(left has only bits in right)
#define BINARY_MORE_OR_EQUALS 7 //(left has equal or more bits than right)
#define BINARY_INTERSECT 8      //(left and right has at least one common bit)
#define BINARY_NOT_INTERSECT 9  //(no common bits)
#define BINARY_MORE 10          //left has more bits than right
#define BINARY_LESS 11          //left has less bits than right

Targets *GetMyTarget(Scriptable *Sender, Actor *actor, Targets *parameters, int ga_flags);
Targets *XthNearestOf(Targets *parameters, int count, int ga_flags);
Targets *XthNearestDoor(Targets *parameters, unsigned int count);
Targets *XthNearestEnemyOf(Targets *parameters, int count, int ga_flags);
Targets *ClosestEnemySummoned(Scriptable *origin, Targets *parameters, int ga_flags);
Targets *XthNearestEnemyOfType(Scriptable *origin, Targets *parameters, unsigned int count, int ga_flags);
Targets *XthNearestMyGroupOfType(Scriptable *origin, Targets *parameters, unsigned int count, int ga_flags);

void FreeSrc(SrcVector *poi, const ieResRef key);
SrcVector *LoadSrc(const ieResRef resname);
Action *ParamCopy(Action *parameters);
Action *ParamCopyNoOverride(Action *parameters);
/* returns true if actor matches the object specs. */
bool MatchActor(Scriptable *Sender, ieDword ID, Object* oC);
/* returns the number of actors matching the IDS targeting */
int GetObjectCount(Scriptable* Sender, Object* oC);
int GetObjectLevelCount(Scriptable* Sender, Object* oC);
void SetVariable(Scriptable* Sender, const char* VarName, ieDword value);
Point GetEntryPoint(const char *areaname, const char *entryname);
//these are used from other plugins
GEM_EXPORT int CanSee(Scriptable* Sender, Scriptable* target, bool range, int nodead);
GEM_EXPORT int SeeCore(Scriptable* Sender, Trigger* parameters, int justlos);
GEM_EXPORT int DiffCore(ieDword a, ieDword b, int diffmode);
GEM_EXPORT void DisplayStringCore(Scriptable* Sender, int Strref, int flags);
GEM_EXPORT void SetVariable(Scriptable* Sender, const char* VarName, const char* Context, ieDword value);
GEM_EXPORT void MoveBetweenAreasCore(Actor* actor, const char *area, Point &position, int face, bool adjust);
GEM_EXPORT ieDword CheckVariable(Scriptable* Sender, const char* VarName, bool *valid = NULL);
GEM_EXPORT ieDword CheckVariable(Scriptable* Sender, const char* VarName, const char* Context, bool *valid = NULL);
Action* GenerateActionCore(const char *src, const char *str, int acIndex);
Trigger *GenerateTriggerCore(const char *src, const char *str, int trIndex, int negate);
unsigned int GetSpellDistance(ieResRef spellres, Actor *actor);
unsigned int GetItemDistance(ieResRef itemres, int header);

inline int Bones(ieDword value)
{
	return core->Roll((value&0xf000)>>12, (value&0xff0)>>8, value&15);
}

#endif
