// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef GSUTILS_H
#define GSUTILS_H

#include "exports.h"

#include "Cache.h"
#include "SymbolMgr.h"

#include "GameScript/GameScript.h"
#include "GameScript/Matching.h"

namespace GemRB {

class Gem_Polygon;
class SrcVector;
struct MouseEvent;

using VarContext = ResRef;

//whoseeswho for GetNearestEnemy:
#define ENEMY_SEES_ORIGIN 1
#define ORIGIN_SEES_ENEMY 2

extern PluginHolder<SymbolMgr> triggersTable;
extern PluginHolder<SymbolMgr> actionsTable;
extern PluginHolder<SymbolMgr> overrideTriggersTable;
extern PluginHolder<SymbolMgr> overrideActionsTable;
extern PluginHolder<SymbolMgr> objectsTable;

extern std::array<TriggerFunction, MAX_TRIGGERS> triggers;
extern std::array<ActionFunction, MAX_ACTIONS> actions;
extern std::array<ObjectFunction, MAX_OBJECTS> objects;
extern std::array<IDSFunction, MAX_OBJECT_FIELDS> idtargets;
extern std::array<uint16_t, MAX_ACTIONS> actionflags;
extern std::array<short, MAX_TRIGGERS> triggerflags;
extern ResRefRCCache<Script> BcsCache; //cache for scripts
extern int ObjectIDSCount;
extern int DialogObjectIDSCount;
extern int MaxObjectNesting;
extern bool HasAdditionalRect;
extern bool HasTriggerPoint;
extern bool NoCreate;
extern bool HasKaputz;
extern std::vector<ResRef> ObjectIDSTableNames;
extern std::vector<int> DialogObjectIDSOrder;
extern std::vector<ResRef> DialogObjectIDSTableNames;
extern int ObjectFieldsCount;
extern int ExtraParametersCount;
extern Gem_Polygon** polygons;

// MoveItemCore rvs
enum class MIC {
	Invalid = -2,
	Full,
	NoItem,
	GotItem,
};

// MoveNearerTo flags
enum class MNT {
	None = 0,
	NoRelease = 1,
	FinalDistance = 2,
};

GEM_EXPORT int GetReaction(const Actor* target, const Scriptable* Sender);
GEM_EXPORT ieWordSigned GetHappiness(const Scriptable* Sender, int reputation);
int GetHPPercent(const Scriptable* Sender);
unsigned int StoreCountItems(const ResRef& storeName, const ResRef& itemName);
bool StoreHasItemCore(const ResRef& storename, const ResRef& itemname);
bool RemoveStoreItem(const ResRef& storeName, const ResRef& itemName, ieDword count = 0);
bool HasItemCore(const Inventory* inventory, const ResRef& itemname, ieDword flags, ieDword itemFlags = 0, int negate = 0);
void ClickCore(Scriptable* Sender, const MouseEvent& me, int speed);
void PlaySequenceCore(Scriptable* Sender, const Action* parameters, Animation::index_t value);
void TransformItemCore(Actor* actor, const Action* parameters, bool onlyone);
void CreateVisualEffectCore(Actor* target, const ResRef& effect, int iterations);
void CreateVisualEffectCore(const Scriptable* Sender, const Point& position, const ResRef& effect, int iterations);
void GetPositionFromScriptable(const Scriptable* scr, Point& position, bool trap);
void BeginDialog(Scriptable* Sender, const Action* parameters, int flags);
void ChangeAnimationCore(Actor* src, const ResRef& replacement, bool effect);
void PolymorphCopyCore(const Actor* src, Actor* tar);
void CreateCreatureCore(Scriptable* Sender, Action* parameters, int flags);
MIC MoveItemCore(Scriptable* Sender, Scriptable* target, const ResRef& resref, int flags, int setflag, int count = 0);
void MoveToObjectCore(Scriptable* Sender, Action* parameters, ieDword flags, bool untilsee);
GEM_EXPORT bool CreateItemCore(CREItem* item, const ResRef& resref, int a, int b, int c, ieWord expiry = 0);
void AttackCore(Scriptable* Sender, Scriptable* target, int flags);
void HandleBitMod(ieDword& value1, ieDword value2, BitOp opcode);
bool ResolveSpellName(ResRef& spellRes, const Action* parameter);
GEM_EXPORT void ResolveSpellName(ResRef& spellRes, ieDword number);
GEM_EXPORT ieDword ResolveSpellNumber(const ResRef& spellRef);
bool ResolveItemName(ResRef& itemres, const Actor* act, ieDword Slot);
void EscapeAreaCore(Scriptable* Sender, const Point& p, const ResRef& area, const Point& enter, EscapeArea flags, int wait);
void GoNear(Scriptable* Sender, const Point& p);
void MoveNearerTo(Scriptable* Sender, const Scriptable* target, int distance, MNT flags = MNT::None);
MNT MoveNearerTo(Scriptable* Sender, const Point& p, int distance, MNT flags = MNT::None);

GEM_EXPORT GroupType GetGroup(const Actor* actor);
GEM_EXPORT Actor* GetNearestOf(const Map* map, const Actor* origin, int whoseeswho);
GEM_EXPORT Actor* GetNearestEnemyOf(const Map* map, const Actor* origin, int whoseeswho);
GEM_EXPORT void FreeSrc(const SrcVector* poi, const ResRef& key);
GEM_EXPORT SrcVector* LoadSrc(const ResRef& resname);
bool IsInObjectRect(const Point& pos, const Region& rect);
Action* ParamCopy(const Action* parameters);
Action* ParamCopyNoOverride(const Action* parameters);
GEM_EXPORT void SetVariable(Scriptable* Sender, const StringParam& VarName, ieDword value, VarContext Context = {});
GEM_EXPORT void SetPointVariable(Scriptable* Sender, const StringParam& VarName, const Point& point, const VarContext& Context = {});
Point GetEntryPoint(const ResRef& areaname, const ResRef& entryname);
//these are used from other plugins
GEM_EXPORT int CanSee(const Scriptable* Sender, const Scriptable* target, bool range, int nodead, bool halveRange = false);
GEM_EXPORT int SeeCore(Scriptable* Sender, const Trigger* parameters, int extraFlags);
GEM_EXPORT bool DiffCore(ieDword a, ieDword b, DiffMode diffMode);
GEM_EXPORT void DisplayStringCoreVC(Scriptable* Sender, Verbal vc, int flags);
GEM_EXPORT void DisplayStringCore(Scriptable* Sender, ieStrRef str, int flags, const char* sound = nullptr);
bool CreateMovementEffect(Actor* actor, const ResRef& area, const Point& position, int face);
GEM_EXPORT void MoveBetweenAreasCore(Actor* actor, const ResRef& area, const Point& position, int face, bool adjust);
GEM_EXPORT ieDword CheckVariable(const Scriptable* Sender, const StringParam& VarName, VarContext Context = {}, bool* valid = nullptr);
GEM_EXPORT Point CheckPointVariable(const Scriptable* Sender, const StringParam& VarName, const VarContext& Context = {}, bool* valid = nullptr);
GEM_EXPORT bool VariableExists(const Scriptable* Sender, const StringParam& VarName, const VarContext& Context);
Action* GenerateActionCore(const char* src, const char* str, unsigned short actionID);
Trigger* GenerateTriggerCore(const char* src, const char* str, int trIndex, int negate);
GEM_EXPORT unsigned int GetSpellDistance(const ResRef& spellRes, Scriptable* Sender, const Point& target = Point());
unsigned int GetItemDistance(const ResRef& itemres, int header, float_t angle);
void SetupWishCore(Scriptable* Sender, TableMgr::index_t column, int picks);
void AmbientActivateCore(const Scriptable* Sender, const Action* parameters, bool flag);
void SpellCore(Scriptable* Sender, Action* parameters, int flags);
void SpellPointCore(Scriptable* Sender, Action* parameters, int flags);
Gem_Polygon* GetPolygon2DA(ieDword index);
void AddXPCore(const Action* parameters, bool divide = false);
int NumItemsCore(Scriptable* Sender, const Trigger* parameters);
unsigned int NumBouncingSpellLevelCore(Scriptable* Sender, const Trigger* parameters);
unsigned int NumImmuneToSpellLevelCore(Scriptable* Sender, const Trigger* parameters);
void RunAwayFromCore(Scriptable* Sender, const Action* parameters, int flags);
void MoveGlobalObjectCore(Scriptable* Sender, const Action* parameters, int flags);
int OverrideStatsIDS(int stat);

}

#endif
