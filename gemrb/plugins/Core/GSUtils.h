#ifndef GSUTILS_H
#define GSUTILS_H
#include "Interface.h"
#include "../../includes/strrefs.h"
#include "../../includes/defsounds.h"
//indebug flags
#define ID_REFERENCE 1
#define ID_CUTSCENE  2
#define ID_VARIABLES 4
#define ID_ACTIONS   8
#define ID_TRIGGERS  16

extern int initialized;
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
extern ieResRef *ObjectIDSTableNames;
extern int ObjectFieldsCount;
extern int ExtraParametersCount;
extern int InDebug;

#define MIC_INVALID -2
#define MIC_FULL -1
#define MIC_NOITEM 0
#define MIC_GOTITEM 1

int GetReaction(Scriptable *Sender);
int GetHappiness(Scriptable *Sender, int reputation);
int GetHPPercent(Scriptable *Sender);
void CreateVisualEffectCore(Scriptable *Sender, Point &position, const char *effect);
void GetPositionFromScriptable(Scriptable* scr, Point &position, bool trap);
int CanSee(Scriptable* Sender, Scriptable* target);
int ValidForDialogCore(Scriptable* Sender, Actor *target);
void BeginDialog(Scriptable* Sender, Action* parameters, int flags);
void ChangeAnimationCore(Actor *src, const char *resref, bool effect);
void PolymorphCopyCore(Actor *src, Actor *tar, bool base);
void CreateCreatureCore(Scriptable* Sender, Action* parameters, int flags);
Targets* GetAllObjects(Scriptable* Sender, Object* oC);
Scriptable* GetActorFromObject(Scriptable* Sender, Object* oC);
int MoveItemCore(Scriptable *Sender, Scriptable *target, const char *resref, int flags);
void AttackCore(Scriptable *Sender, Scriptable *target, Action *parameters, int flags);
void InitScriptTables();
void HandleBitMod(ieDword &value1, ieDword value2, int opcode);
void DisplayStringCore(Scriptable* Sender, int Strref, int flags);
void EscapeAreaCore(Actor *src, const char *resref, Point &enter, Point &exit, int flags);
void GoNearAndRetry(Scriptable *Sender, Scriptable *target, bool destination);
void GoNearAndRetry(Scriptable *Sender, Point &p);
void FreeSrc(SrcVector *poi, const ieResRef key);
SrcVector *LoadSrc(const ieResRef resname);
Action *ParamCopy(Action *parameters);
Action *ParamCopyNoOverride(Action *parameters);
void SetVariable(Scriptable* Sender, const char* VarName, const char* Context, ieDword value);
void SetVariable(Scriptable* Sender, const char* VarName, ieDword value);
//these are used from other plugins
GEM_EXPORT void MoveBetweenAreasCore(Actor* actor, const char *area, Point &position, int face, bool adjust);
GEM_EXPORT ieDword CheckVariable(Scriptable* Sender, const char* VarName);
GEM_EXPORT ieDword CheckVariable(Scriptable* Sender, const char* VarName, const char* Context);
Action* GenerateActionCore(const char *src, const char *str, int acIndex, bool autoFree);
Trigger *GenerateTriggerCore(const char *src, const char *str, int trIndex, int negate);

#endif
