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
#ifndef GSUTILS_H
#define GSUTILS_H

#include "GameScript/GameScript.h"

#include "defsounds.h"
#include "exports.h"
#include "strrefs.h"

#include "Interface.h"

namespace GemRB {

using VarContext = ResRef;

//whoseeswho for GetNearestEnemy:
#define ENEMY_SEES_ORIGIN 1
#define ORIGIN_SEES_ENEMY 2

extern std::shared_ptr<SymbolMgr> triggersTable;
extern std::shared_ptr<SymbolMgr> actionsTable;
extern std::shared_ptr<SymbolMgr> overrideTriggersTable;
extern std::shared_ptr<SymbolMgr> overrideActionsTable;
extern std::shared_ptr<SymbolMgr> objectsTable;
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
extern bool NoCreate;
extern bool HasKaputz;
extern std::vector<ResRef> ObjectIDSTableNames;
extern int ObjectFieldsCount;
extern int ExtraParametersCount;
extern Gem_Polygon **polygons;

#define MIC_INVALID -2
#define MIC_FULL -1
#define MIC_NOITEM 0
#define MIC_GOTITEM 1

GEM_EXPORT int GetReaction(const Actor *target, const Scriptable *Sender);
GEM_EXPORT int GetHappiness(const Scriptable *Sender, int reputation);
int GetHPPercent(const Scriptable *Sender);
bool StoreHasItemCore(const ResRef& storename, const ResRef& itemname);
bool RemoveStoreItem(const ResRef& storename, const ResRef& itemname);
bool HasItemCore(const Inventory *inventory, const ResRef& itemname, ieDword flags);
void ClickCore(Scriptable *Sender, const MouseEvent& me, int speed);
void PlaySequenceCore(Scriptable *Sender, const Action *parameters, Animation::index_t value);
void TransformItemCore(Actor *actor, const Action *parameters, bool onlyone);
void CreateVisualEffectCore(Actor* target, const ResRef& effect, int iterations);
void CreateVisualEffectCore(const Scriptable* Sender, const Point& position, const ResRef& effect, int iterations);
void GetPositionFromScriptable(const Scriptable *scr, Point &position, bool trap);
void BeginDialog(Scriptable* Sender, const Action* parameters, int flags);
void ChangeAnimationCore(Actor* src, const ResRef& replacement, bool effect);
void PolymorphCopyCore(const Actor *src, Actor *tar);
void CreateCreatureCore(Scriptable* Sender, Action* parameters, int flags);
int MoveItemCore(Scriptable *Sender, Scriptable *target, const ResRef& resref, int flags, int setflag, int count = 0);
void MoveToObjectCore(Scriptable *Sender, Action *parameters, ieDword flags, bool untilsee);
GEM_EXPORT bool CreateItemCore(CREItem *item, const ResRef &resref, int a, int b, int c);
void AttackCore(Scriptable *Sender, Scriptable *target, int flags);
void InitScriptTables();
void HandleBitMod(ieDword &value1, ieDword value2, BitOp opcode);
bool ResolveSpellName(ResRef& spellRes, const Action *parameter);
GEM_EXPORT void ResolveSpellName(ResRef& spellRes, ieDword number);
GEM_EXPORT ieDword ResolveSpellNumber(const ResRef& spellRef);
bool ResolveItemName(ResRef& itemres, const Actor *act, ieDword Slot);
void EscapeAreaCore(Scriptable *Sender, const Point &p, const ResRef& area, const Point &enter, int flags, int wait);
void GoNear(Scriptable *Sender, const Point &p);
void MoveNearerTo(Scriptable *Sender, const Scriptable *target, int distance, int dont_release = 0);
int MoveNearerTo(Scriptable *Sender, const Point &p, int distance, int no_release);

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

GEM_EXPORT int GetGroup(const Actor *actor);
GEM_EXPORT Actor *GetNearestOf(const Map *map, const Actor *origin, int whoseeswho);
GEM_EXPORT Actor *GetNearestEnemyOf(const Map *map, const Actor *origin, int whoseeswho);
GEM_EXPORT void FreeSrc(const SrcVector *poi, const ResRef& key);
GEM_EXPORT SrcVector *LoadSrc(const ResRef& resname);
bool IsInObjectRect(const Point &pos, const Region &rect);
Action *ParamCopy(const Action *parameters);
Action *ParamCopyNoOverride(const Action *parameters);
GEM_EXPORT void SetVariable(Scriptable* Sender, const StringParam& VarName, ieDword value, VarContext Context = {});
GEM_EXPORT void SetPointVariable(Scriptable* Sender, const StringParam& VarName, const Point &point, const VarContext& Context = {});
Point GetEntryPoint(const ResRef& areaname, const ResRef& entryname);
//these are used from other plugins
GEM_EXPORT int CanSee(const Scriptable *Sender, const Scriptable *target, bool range, int nodead, bool halveRange = false);
GEM_EXPORT int SeeCore(Scriptable *Sender, const Trigger *parameters, int justlos);
GEM_EXPORT bool DiffCore(ieDword a, ieDword b, int diffMode);
GEM_EXPORT void DisplayStringCoreVC(Scriptable* Sender, size_t vc, int flags);
GEM_EXPORT void DisplayStringCore(Scriptable* Sender, ieStrRef str, int flags, const char* sound = nullptr);
bool CreateMovementEffect(Actor* actor, const ResRef& area, const Point &position, int face);
GEM_EXPORT void MoveBetweenAreasCore(Actor* actor, const ResRef &area, const Point &position, int face, bool adjust);
GEM_EXPORT ieDword CheckVariable(const Scriptable *Sender, const StringParam& VarName, VarContext Context = {}, bool *valid = nullptr);
GEM_EXPORT Point CheckPointVariable(const Scriptable *Sender, const StringParam& VarName, const VarContext& Context = {}, bool *valid = nullptr);
GEM_EXPORT bool VariableExists(const Scriptable *Sender, const StringParam& VarName, const VarContext& Context);
Action* GenerateActionCore(const char *src, const char *str, unsigned short actionID);
Trigger *GenerateTriggerCore(const char *src, const char *str, int trIndex, int negate);
GEM_EXPORT unsigned int GetSpellDistance(const ResRef& spellRes, Scriptable* Sender, const Point& target = Point());
unsigned int GetItemDistance(const ResRef& itemres, int header, double angle);
void SetupWishCore(Scriptable *Sender, TableMgr::index_t column, int picks);
void AmbientActivateCore(const Scriptable *Sender, const Action *parameters, bool flag);
void SpellCore(Scriptable *Sender, Action *parameters, int flags);
void SpellPointCore(Scriptable *Sender, Action *parameters, int flags);
Gem_Polygon *GetPolygon2DA(ieDword index);
void AddXPCore(const Action *parameters, bool divide = false);
int NumItemsCore(Scriptable *Sender, const Trigger *parameters);
unsigned int NumBouncingSpellLevelCore(Scriptable *Sender, const Trigger *parameters);
int NumImmuneToSpellLevelCore(Scriptable *Sender, const Trigger *parameters);
void RunAwayFromCore(Scriptable* Sender, const Action* parameters, int flags);
void MoveGlobalObjectCore(Scriptable* Sender, const Action* parameters, int flags);

inline int Bones(ieDword value)
{
	return core->Roll((value&0xf000)>>12, (value&0xff0)>>8, value&15);
}

}

#endif
