/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/GameScript.h,v 1.194 2005/06/19 22:59:34 avenger_teambg Exp $
 *
 */

class GameScript;
class Action;

#ifndef GAMESCRIPT_H
#define GAMESCRIPT_H

#include "DataStream.h"
#include "Variables.h"
#include "SymbolMgr.h"
#include "Actor.h"

//escapearea flags
#define EA_DESTROY 1

//displaystring flags
#define DS_WAIT    1
#define DS_HEAD    2
#define DS_CONSOLE 4
#define DS_CONST   8
#define DS_NONAME  16

//diffmode (iwd2)
#define DM_EQUAL   1
#define DM_LESS    2
#define DM_GREATER 3

//attack core flags
#define AC_REEVALUATE 1

//trigger flags stored in triggers in .bcs files
#define NEGATE_TRIGGER 1

#define MAX_OBJECT_FIELDS	10
#define MAX_NESTING		5

#define GSASSERT(f,c) \
 if(!(f))  \
 {  \
 printf("Assertion failed: %s [0x%08lX] Line %d",#f, c, __LINE__); \
	abort(); \
 }

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

typedef std::vector<ieDword> SrcVector;

typedef struct targettype {
	Actor *actor;
	unsigned int distance;
} targettype;

typedef std::list<targettype> targetlist;

class GEM_EXPORT Targets {
public:
	Targets()
	{
	};
	~Targets()
	{
		Clear();
	};
private:
	std::list<targettype> objects;
public:
	int Count() const;
	targettype *RemoveTargetAt(targetlist::iterator &m);
	targettype *GetNextTarget(targetlist::iterator &m);
	targettype *GetLastTarget();
	targettype *GetFirstTarget(targetlist::iterator &m);
	Actor *GetTarget(unsigned int index);
	void AddTarget(Actor* actor, unsigned int distance);
	void Clear();
};

class GEM_EXPORT Object {
public:
	Object()
	{
		objectName[0] = 0;

		memset( objectFields, 0, MAX_OBJECT_FIELDS * sizeof( int ) );
		memset( objectFilters, 0, MAX_NESTING * sizeof( int ) );
		memset( objectRect, 0, 4 * sizeof( int ) );

		canary = (unsigned long) 0xdeadbeef;
	};
	~Object()
	{
	}
public:
	int objectFields[MAX_OBJECT_FIELDS];
	int objectFilters[MAX_NESTING];
	int objectRect[4];
	char objectName[65];
private:
	volatile unsigned long canary;
public:
	void Dump()
	{
		int i;

		GSASSERT( canary == (unsigned long) 0xdeadbeef, canary );
		if(objectName[0]) {
			printf("Object: %s\n",objectName);
			return;
		}
		printf("IDS Targeting: ");
		for(i=0;i<MAX_OBJECT_FIELDS;i++) {
			printf("%d ",objectFields[i]);
		}
		printf("\n");
		printf("Filters: ");
		for(i=0;i<MAX_NESTING;i++) {
			printf("%d ",objectFilters[i]);
		}
		printf("\n");
	}

	void Release()
	{
		GSASSERT( canary == (unsigned long) 0xdeadbeef, canary );
		canary = 0xdddddddd;
		delete this;
	}
};

class GEM_EXPORT Trigger {
public:
	Trigger()
	{
		objectParameter = NULL;
		RefCount = 1;
		string0Parameter[0] = 0;
		string1Parameter[0] = 0;
		pointParameter.x = 0;
		pointParameter.y = 0;
		canary = (unsigned long) 0xdeadbeef;
	};
	~Trigger()
	{
		if (objectParameter) {
			objectParameter->Release();
			objectParameter = NULL;
		}
	}
public:
	unsigned short triggerID;
	int int0Parameter;
	int flags;
	int int1Parameter;
	int int2Parameter;
	Point pointParameter;
	char string0Parameter[65];
	char string1Parameter[65];
	Object* objectParameter;
private:
	int RefCount;
	volatile unsigned long canary;
public:
	void Release()
	{
		GSASSERT( canary == (unsigned long) 0xdeadbeef, canary );
		if (!RefCount) {
			printf( "WARNING!!! Double Freeing in %s: Line %d\n", __FILE__,
				__LINE__ );
			abort();
		}
		RefCount--;
		if (!RefCount) {
			canary = 0xdddddddd;
			delete this;
		}
	}
	void IncRef()
	{
		GSASSERT( canary == (unsigned long) 0xdeadbeef, canary );
		RefCount++;
		if (RefCount >= 4000) {
			printf( "Refcount increased to: %d in trigger\n", RefCount );
			abort();
		}
	}
};

class GEM_EXPORT Condition {
public:
	Condition()
	{
		RefCount = 1;
		triggers = NULL;
		triggersCount = 0;
		canary = (unsigned long) 0xdeadbeef;
	};
	~Condition()
	{
		if (!triggers) {
			return;
		}
		for (int c = 0; c < triggersCount; c++) {
			if (triggers[c]) {
				triggers[c]->Release();
				triggers[c] = NULL;
			}
		}
		delete[] triggers;
	}
public:
	unsigned short triggersCount;
	Trigger** triggers;
private:
	int RefCount;
	volatile unsigned long canary;
public:
	void Release()
	{
		GSASSERT( canary == (unsigned long) 0xdeadbeef, canary );
		if (!RefCount) {
			printf( "WARNING!!! Double Freeing in %s: Line %d\n", __FILE__,
				__LINE__ );
			abort();
		}
		RefCount--;
		if (!RefCount) {
			canary = 0xdddddddd;
			delete this;
		}
	}
	void IncRef()
	{
		GSASSERT( canary == (unsigned long) 0xdeadbeef, canary );
		RefCount++;
		if (RefCount >= 4000) {
			printf( "Refcount increased to: %d in condition\n", RefCount );
			abort();
		}
	}
};

class GEM_EXPORT Action {
public:
	Action(bool autoFree)
	{
		actionID = 0;
		objects[0] = NULL;
		objects[1] = NULL;
		objects[2] = NULL;
		string0Parameter[0] = 0;
		string1Parameter[0] = 0;
		int0Parameter = 0;
		pointParameter.x = 0;
		pointParameter.y = 0;
		int1Parameter = 0;
		int2Parameter = 0;
		if(autoFree) {
			RefCount = 0;
		}
		else {
			RefCount = 1;
		}
		canary = (unsigned long) 0xdeadbeef;
	};
	~Action()
	{
		for (int c = 0; c < 3; c++) {
			if (objects[c]) {
				objects[c]->Release();
				objects[c] = NULL;
			}
		}
	}
public:
	unsigned short actionID;
	Object* objects[3];
	int int0Parameter;
	Point pointParameter;
	int int1Parameter;
	int int2Parameter;
	char string0Parameter[65];
	char string1Parameter[65];
private:
	int RefCount;
	volatile unsigned long canary;
public:
	void Dump()
	{
		int i;

		GSASSERT( canary == (unsigned long) 0xdeadbeef, canary );
		printf("String1: %s, String2: %s\n", string0Parameter?string0Parameter:"<NULL>", string1Parameter?string1Parameter:"<NULL>");
		for (i=0;i<3;i++) {
			if (objects[i]) {
				printf( "%d. ",i+1);
				objects[i]->Dump();
			} else {
				printf( "%d. Object - NULL\n",i+1);
			}
		}
	}

	void Release()
	{
		GSASSERT( canary == (unsigned long) 0xdeadbeef, canary );
		if (!RefCount) {
			printf( "WARNING!!! Double Freeing in %s: Line %d\n", __FILE__,
				__LINE__ );
			abort();
		}
		RefCount--;
		if (!RefCount) {
			canary = 0xdddddddd;
			delete this;
		}
	}
	void IncRef()
	{
		GSASSERT( canary == (unsigned long) 0xdeadbeef, canary );
		RefCount++;
		if (RefCount >= 4000) {
			printf( "Refcount increased to: %d in action %d\n", RefCount,
				actionID );
			abort();
		}
	}
};

class GEM_EXPORT Response {
public:
	Response()
	{
		RefCount = 1;
		actions = NULL;
		weight = 0;
		actionsCount = 0;
		canary = (unsigned long) 0xdeadbeef;
	};
	~Response()
	{
		if (!actions) {
			return;
		}
		for (int c = 0; c < actionsCount; c++) {
			if (actions[c]) {
				actions[c]->Release();
				actions[c] = NULL;
			}
		}
		delete[] actions;
	}
public:
	unsigned char weight;
	unsigned char actionsCount;
	Action** actions;
private:
	int RefCount;
	volatile unsigned long canary;
public:
	void Release()
	{
		GSASSERT( canary == (unsigned long) 0xdeadbeef, canary );
		if (!RefCount) {
			printf( "WARNING!!! Double Freeing in %s: Line %d\n", __FILE__,
				__LINE__ );
			abort();
		}
		RefCount--;
		if (!RefCount) {
			canary = 0xdddddddd;
			delete this;
		}
	}
	void IncRef()
	{
		GSASSERT( canary == (unsigned long) 0xdeadbeef, canary );
		RefCount++;
		if (RefCount >= 4000) {
			printf( "Refcount increased to: %d in response\n", RefCount );
			abort();
		}
	}
};

class GEM_EXPORT ResponseSet {
public:
	ResponseSet()
	{
		RefCount = 1;
		responses = NULL;
		responsesCount = 0;
		canary = (unsigned long) 0xdeadbeef;
	};
	~ResponseSet()
	{
		if (!responses) {
			return;
		}
		for (int b = 0; b < responsesCount; b++) {
			responses[b] -> Release();
			responses[b] = NULL;
		}
		delete[] responses;
	}
public:
	unsigned short responsesCount;
	Response** responses;
private:
	int RefCount;
	volatile unsigned long canary;
public:
	void Release()
	{
		GSASSERT( canary == (unsigned long) 0xdeadbeef, canary );
		if (!RefCount) {
			printf( "WARNING!!! Double Freeing in %s: Line %d\n", __FILE__,
				__LINE__ );
			abort();
		}
		RefCount--;
		if (!RefCount) {
			canary = 0xdddddddd;
			delete this;
		}
	}
	void IncRef()
	{
		GSASSERT( canary == (unsigned long) 0xdeadbeef, canary );
		RefCount++;
		if (RefCount >= 4000) {
			printf( "Refcount increased to: %d\n", RefCount );
			abort();
		}
	}
};

class GEM_EXPORT ResponseBlock {
public:
	ResponseBlock()
	{
		condition = NULL;
		responseSet = NULL;
		canary = (unsigned long) 0xdeadbeef;
	};
	~ResponseBlock()
	{
		if (condition) {
			condition->Release();
			condition = NULL;
		}
		if (responseSet) {
			responseSet->Release();
			responseSet = NULL;
		}
	}
public:
	Condition* condition;
	ResponseSet* responseSet;
private:
	volatile unsigned long canary;
public:
	void Release()
	{
		GSASSERT( canary == (unsigned long) 0xdeadbeef, canary );
		canary = 0xdddddddd;
		delete this;
	}
};

class GEM_EXPORT Script {
public:
	Script()
	{
		canary = (unsigned long) 0xdeadbeef;
		responseBlocks = NULL;
		responseBlocksCount = 0;
	};
	~Script()
	{
		FreeResponseBlocks();
	}
	void AllocateBlocks(unsigned int count)
	{
		if (!count) {
			FreeResponseBlocks();
			responseBlocks = NULL;
			responseBlocksCount = 0;
		}
		responseBlocks = new ResponseBlock * [count];
		responseBlocksCount = count;
	}
private:
	void FreeResponseBlocks()
	{
		if (!responseBlocks) {
			return;
		}
		for (unsigned int i = 0; i < responseBlocksCount; i++) {
			if (responseBlocks[i]) {
				responseBlocks[i]->Release();
				responseBlocks[i] = NULL;
			}
		}
		delete[] responseBlocks;
	}
public:
	unsigned int responseBlocksCount;
	ResponseBlock** responseBlocks;
private:
	volatile unsigned long canary;
public:
	void Release()
	{
		GSASSERT( canary == (unsigned long) 0xdeadbeef, canary );
		canary = 0xdddddddd;
		delete this;
	}
};

typedef int (* TriggerFunction)(Scriptable*, Trigger*);
typedef void (* ActionFunction)(Scriptable*, Action*);
typedef Targets* (* ObjectFunction)(Scriptable *, Targets*);
typedef int (* IDSFunction)(Actor *, int parameter);

#define TF_NONE 	 0
#define TF_CONDITION     1  //this isn't a trigger, just a condition (0x4000)
#define TF_MERGESTRINGS  8  //same value as actions' mergestring

struct TriggerLink {
	const char* Name;
	TriggerFunction Function;
	short Flags;
};

//createcreature flags
#define CC_OFFSET    1 
#define CC_OBJECT    2  
#define CC_OFFSCREEN 3
#define CC_MASK      3
#define CC_CHECK_IMPASSABLE  4  //adjust position (searchmap)
#define CC_PLAY_ANIM 8          //play animation
#define CC_STRING1   16         //resref is in second string
#define CC_CHECK_OVERLAP 32     //other actors

//begindialog flags
#define BD_STRING0   0
#define BD_TARGET    1
#define BD_SOURCE    2
#define BD_RESERVED  3  //playerX resref
#define BD_INTERACT  4  //banter dialogs
#define BD_LOCMASK   7  //where is the dialog resref
#define BD_TALKCOUNT 8  //increases talkcount
#define BD_SETDIALOG 16 //also sets dialog (for string0)
#define BD_CHECKDIST 32 //checks distance, if needs, walks up
#define BD_OWN  	 64 //source == target, works for player only
#define BD_INTERRUPT 128  //interrupts action
#define BD_NUMERIC   256  //target is numeric

#define AF_NONE 	 0
#define AF_INSTANT       1
#define AF_CONTINUE      2
#define AF_MASK 	 3   //none, instant or continue
#define AF_BLOCKING      4
#define AF_MERGESTRINGS  8
//we could use this flag to restrict player scripts from using dangerous
//opcodes, it would be a very useful and easy to implement feature!
#define AF_RESTRICTED    16 
//#define AF_RESTRICTED_LEVEL2  32 //maybe we could use 2 bits for this???
#define AF_SCRIPTLEVEL   64  //this hack will transfer scriptlevel to int0parameter at runtime (changecurrentscript relies on it)

struct ActionLink {
	const char* Name;
	ActionFunction Function;
	short Flags;
};

struct ObjectLink {
	const char* Name;
	ObjectFunction Function;
};

struct IDSLink {
	const char* Name;
	IDSFunction Function;
};

#define MAX_TRIGGERS			0xFF
#define MAX_ACTIONS			400
#define MAX_OBJECTS			128

extern void SetScriptDebugMode(int arg);
extern int RandomNumValue;

class GEM_EXPORT GameScript {
public:
	Variables* locals;
	Script* script;
	Scriptable* MySelf;
	unsigned long scriptRunDelay;
	bool endReached;
	int scriptlevel;
	void Update();
	void EvaluateAllBlocks();
private: //Internal Functions
	Script* CacheScript(ieResRef ResRef);
	ResponseBlock* ReadResponseBlock(DataStream* stream);
	ResponseSet* ReadResponseSet(DataStream* stream);
	Response* ReadResponse(DataStream* stream);
	Trigger* ReadTrigger(DataStream* stream);
	//Object* DecodeObject(const char* line);
	bool EvaluateCondition(Scriptable* Sender, Condition* condition);
	static int EvaluateTrigger(Scriptable* Sender, Trigger* trigger);
	int ExecuteResponseSet(Scriptable* Sender, ResponseSet* rS);
	int ExecuteResponse(Scriptable* Sender, Response* rE);
	//static Targets* EvaluateObject(Scriptable* Sender, Object* oC);
	static int ParseInt(const char*& src);
	static void ParseString(const char*& src, char* tmp);
	//static void DisplayStringCore(Scriptable* Sender, int Strref, int flags);
	//static int ValidForDialogCore(Scriptable* Sender, Actor* target);
private:
	//static void CreateVisualEffectCore(Scriptable *Sender, Point &position, const char *effect);
	//static int SeeCore(Scriptable* Sender, Trigger* parameters, int flags);
	//static void BeginDialog(Scriptable* Sender, Action* parameters, int flags);
	//static void CreateCreatureCore(Scriptable* Sender, Action* parameters, int flags);
	//static int MoveItemCore(Scriptable *Sender, Scriptable *Target, const char *resref, int flags);
	//static void AttackCore(Scriptable *actor, Scriptable *target, Action *parameters, int flags);
	//static Action *GenerateActionCore(const char *src, const char *str, int acIndex, bool autoFree);
	//static Trigger *GenerateTriggerCore(const char *src, const char *str, int trIndex, int negate);
	//static Trigger* GenerateTrigger(char* String);
	/* returns true if actor matches the object specs. */
	static bool MatchActor(Scriptable *Sender, Actor* actor, Object* oC);
	/* returns the number of actors matching the IDS targeting */
	static int GetObjectCount(Scriptable* Sender, Object* oC);
	
	static int GetHappiness(Scriptable* Sender, int reputation);
	static int GetHPPercent(Scriptable* Sender);
	static Targets *XthNearestOf(Targets *parameters, int count);
	static Targets *XthNearestEnemyOf(Targets *parameters, int count);
	static Targets *XthNearestEnemyOfType(Scriptable *origin, Targets *parameters, int count);
	static Targets *XthNearestMyGroupOfType(Scriptable *origin, Targets *parameters, int count);

	//static unsigned char GetOrient(Point &s, Point &d);
private: //Internal variables
	unsigned long lastRunTime;
	unsigned long scriptType;
private: //Script Internal Variables
	ieResRef Name;
	bool continueExecution;
	bool freeLocals;
public:
	GameScript(ieResRef ResRef, unsigned char ScriptType,
		Variables* local = NULL, int ScriptLevel = 0);
	~GameScript();
	const char *GetName() { return this?Name:"NONE\0\0\0\0"; }
	//static void ReplaceMyArea(Scriptable* Sender, char* newVarName);
	//static ieDword CheckVariable(Scriptable* Sender, const char* VarName, const char* Context);
	//static ieDword CheckVariable(Scriptable* Sender, const char* VarName);

	//static void SetVariable(Scriptable* Sender, const char* VarName,
	//	const char* Context, ieDword value);
	//static void SetVariable(Scriptable* Sender, const char* VarName, ieDword value);
	static void ExecuteString(Scriptable* Sender, char* String);
	static int EvaluateString(Scriptable* Sender, char* String);
	static void ExecuteAction(Scriptable* Sender, Action* aC);
	//static Action* GenerateAction(char* String, bool autoFree=false);
	static void MoveBetweenAreasCore(Actor* actor, const char *area, Point &position, int face, bool adjust);
public: //Script Functions
	static int ID_Alignment(Actor *actor, int parameter);
	static int ID_Allegiance(Actor *actor, int parameter);
	static int ID_AVClass(Actor *actor, int parameter);
	static int ID_Class(Actor *actor, int parameter);
	static int ID_ClassMask(Actor *actor, int parameter);
	static int ID_Faction(Actor *actor, int parameter);
	static int ID_Gender(Actor *actor, int parameter);
	static int ID_General(Actor *actor, int parameter);
	static int ID_Race(Actor *actor, int parameter);
	static int ID_Specific(Actor *actor, int parameter);
	static int ID_Subrace(Actor *actor, int parameter);
	static int ID_Team(Actor *actor, int parameter);

	//Triggers
	static int ActionListEmpty(Scriptable* Sender, Trigger* parameters);
	static int Acquired(Scriptable* Sender, Trigger* parameters);
	static int Alignment(Scriptable* Sender, Trigger* parameters);
	static int Allegiance(Scriptable* Sender, Trigger* parameters);
	static int AnimationID(Scriptable* Sender, Trigger* parameters);
	static int AnimState(Scriptable* Sender, Trigger* parameters);
	static int AnyPCOnMap(Scriptable* Sender, Trigger* parameters);
	static int AreaCheck(Scriptable* Sender, Trigger* parameter);
	static int AreaCheckObject(Scriptable* Sender, Trigger* parameter);
	static int AreaFlag(Scriptable* Sender, Trigger* parameter);
	static int AreaRestDisabled(Scriptable* Sender, Trigger* parameter);
	static int AreaType(Scriptable* Sender, Trigger* parameter);
	static int AtLocation(Scriptable* Sender, Trigger* parameter);
	static int BitCheck(Scriptable* Sender, Trigger* parameters);
	static int BitCheckExact(Scriptable* Sender, Trigger* parameters);
	static int BitGlobal_Trigger(Scriptable* Sender, Trigger* parameters);
	static int BreakingPoint(Scriptable* Sender, Trigger* parameters);
	static int CalledByName(Scriptable* Sender, Trigger* parameters);
	static int CharName(Scriptable* Sender, Trigger* parameters);
	static int ChargeCount(Scriptable* Sender, Trigger* parameters);
	static int CheckDoorFlags(Scriptable* Sender, Trigger* parameters);
	static int CheckPartyAverageLevel(Scriptable* Sender, Trigger* parameters);
	static int CheckPartyLevel(Scriptable* Sender, Trigger* parameters);
	static int CheckStat(Scriptable* Sender, Trigger* parameters);
	static int CheckStatGT(Scriptable* Sender, Trigger* parameters);
	static int CheckStatLT(Scriptable* Sender, Trigger* parameters);
	static int Class(Scriptable* Sender, Trigger* parameters);
	static int ClassEx(Scriptable* Sender, Trigger* parameters);
	static int ClassLevel(Scriptable* Sender, Trigger* parameters);
	static int ClassLevelGT(Scriptable* Sender, Trigger* parameters);
	static int ClassLevelLT(Scriptable* Sender, Trigger* parameters);
	static int Clicked(Scriptable* Sender, Trigger* parameters);
	static int CombatCounter(Scriptable* Sender, Trigger* parameters);
	static int CombatCounterGT(Scriptable* Sender, Trigger* parameters);
	static int CombatCounterLT(Scriptable* Sender, Trigger* parameters);
	static int Contains(Scriptable* Sender, Trigger* parameters);
	static int DamageTaken(Scriptable* Sender, Trigger* parameters);
	static int DamageTakenGT(Scriptable* Sender, Trigger* parameters);
	static int DamageTakenLT(Scriptable* Sender, Trigger* parameters);
	static int Dead(Scriptable* Sender, Trigger* parameters);
	static int Die(Scriptable* Sender, Trigger* parameters);
	static int Difficulty(Scriptable* Sender, Trigger* parameters);
	static int DifficultyGT(Scriptable* Sender, Trigger* parameters);
	static int DifficultyLT(Scriptable* Sender, Trigger* parameters);
	static int Entered(Scriptable* Sender, Trigger* parameters);
	static int EntirePartyOnMap(Scriptable* Sender, Trigger* parameters);
	static int Exists(Scriptable* Sender, Trigger* parameters);
	static int ExtraProficiency(Scriptable* Sender, Trigger* parameters);
	static int ExtraProficiencyGT(Scriptable* Sender, Trigger* parameters);
	static int ExtraProficiencyLT(Scriptable* Sender, Trigger* parameters);
	static int Faction(Scriptable* Sender, Trigger* parameters);
	static int FallenPaladin(Scriptable* Sender, Trigger* parameters);
	static int FallenRanger(Scriptable* Sender, Trigger* parameters);
	static int False(Scriptable* Sender, Trigger* parameters);
	static int Frame(Scriptable* Sender, Trigger* parameters);
	static int Gender(Scriptable* Sender, Trigger* parameters);
	static int General(Scriptable* Sender, Trigger* parameters);
	static int G_Trigger(Scriptable* Sender, Trigger* parameters);
	static int Global(Scriptable* Sender, Trigger* parameters);
	static int GlobalAndGlobal_Trigger(Scriptable* Sender, Trigger* parameters);
	static int GlobalBAndGlobal_Trigger(Scriptable* Sender, Trigger* parameters);
	static int GlobalBAndGlobalExact(Scriptable* Sender, Trigger* parameters);
	static int GlobalBitGlobal_Trigger(Scriptable* Sender, Trigger* parameters);
	static int GlobalGT(Scriptable* Sender, Trigger* parameters);
	static int GlobalGTGlobal(Scriptable* Sender, Trigger* parameters);
	static int GlobalLT(Scriptable* Sender, Trigger* parameters);
	static int GlobalLTGlobal(Scriptable* Sender, Trigger* parameters);
	static int GlobalOrGlobal_Trigger(Scriptable* Sender, Trigger* parameters);
	static int GlobalsEqual(Scriptable* Sender, Trigger* parameters);
	static int GlobalsGT(Scriptable* Sender, Trigger* parameters);
	static int GlobalsLT(Scriptable* Sender, Trigger* parameters);
	static int GlobalTimerExact(Scriptable* Sender, Trigger* parameters);
	static int GlobalTimerExpired(Scriptable* Sender, Trigger* parameters);
	static int GlobalTimerNotExpired(Scriptable* Sender, Trigger* parameters);
	static int GGT_Trigger(Scriptable* Sender, Trigger* parameters);
	static int GLT_Trigger(Scriptable* Sender, Trigger* parameters);
	static int Happiness(Scriptable* Sender, Trigger* parameters);
	static int HappinessGT(Scriptable* Sender, Trigger* parameters);
	static int HappinessLT(Scriptable* Sender, Trigger* parameters);
	static int HasInnateAbility(Scriptable* Sender, Trigger* parameters);
	static int HasItem(Scriptable* Sender, Trigger* parameters);
	static int HasItemEquipped(Scriptable* Sender, Trigger* parameters);
	static int HasItemSlot(Scriptable* Sender, Trigger* parameters);
	static int HasWeaponEquipped(Scriptable* Sender, Trigger* parameters);
	static int HaveAnySpells(Scriptable* Sender, Trigger* parameters);
	static int HaveSpellParty(Scriptable* Sender, Trigger* parameters);
	static int HaveSpell(Scriptable* Sender, Trigger* parameters);
	static int Heard(Scriptable* Sender, Trigger* parameters);
	static int Help_Trigger(Scriptable* Sender, Trigger* parameters);
	static int HitBy(Scriptable* Sender, Trigger* parameters);
	static int HotKey(Scriptable* Sender, Trigger* parameters);
	static int HP(Scriptable* Sender, Trigger* parameters);
	static int HPGT(Scriptable* Sender, Trigger* parameters);
	static int HPLost(Scriptable* Sender, Trigger* parameters);
	static int HPLostGT(Scriptable* Sender, Trigger* parameters);
	static int HPLostLT(Scriptable* Sender, Trigger* parameters);
	static int HPLT(Scriptable* Sender, Trigger* parameters);
	static int HPPercent(Scriptable* Sender, Trigger* parameters);
	static int HPPercentGT(Scriptable* Sender, Trigger* parameters);
	static int HPPercentLT(Scriptable* Sender, Trigger* parameters);
	static int InActiveArea(Scriptable* Sender, Trigger* parameter);
	static int InCutSceneMode(Scriptable *Sender, Trigger* parameter);
	static int InMyArea(Scriptable* Sender, Trigger* parameter);
	static int InParty(Scriptable* Sender, Trigger* parameters);
	static int InPartyAllowDead(Scriptable* Sender, Trigger* parameters);
	static int InPartySlot(Scriptable* Sender, Trigger* parameters);
	static int InteractingWith(Scriptable* Sender, Trigger* parameters);
	static int Internal(Scriptable* Sender, Trigger* parameters);
	static int InternalGT(Scriptable* Sender, Trigger* parameters);
	static int InternalLT(Scriptable* Sender, Trigger* parameters);
	static int InventoryFull(Scriptable* Sender, Trigger* parameter);
	static int InWeaponRange(Scriptable* Sender, Trigger* parameter);
	static int IsAClown(Scriptable* Sender, Trigger* parameters);
	static int IsActive(Scriptable* Sender, Trigger* parameters);
	static int IsCreatureAreaFlag( Scriptable* Sender, Trigger* parameters);
	static int IsGabber(Scriptable* Sender, Trigger* parameters);
	static int IsExtendedNight(Scriptable* Sender, Trigger* parameters);
	static int IsFacingObject(Scriptable* Sender, Trigger* parameters);
	static int IsFacingSavedRotation(Scriptable* Sender, Trigger* parameters);
	static int IsLocked(Scriptable* Sender, Trigger* parameters);
	static int IsPlayerNumber( Scriptable* Sender, Trigger* parameters);
	static int IsRotation(Scriptable* Sender, Trigger* parameters);
	static int IsTeamBitOn(Scriptable* Sender, Trigger* parameters);
	static int IsValidForPartyDialog(Scriptable* Sender, Trigger* parameters);
	static int ItemIsIdentified(Scriptable* Sender, Trigger* parameters);
	static int Kit(Scriptable* Sender, Trigger* parameters);
	static int LastMarkedObject_Trigger(Scriptable* Sender, Trigger* parameters);
	static int Level(Scriptable* Sender, Trigger* parameters);
	static int LevelGT(Scriptable* Sender, Trigger* parameters);
	static int LevelLT(Scriptable* Sender, Trigger* parameters);
	static int LevelParty(Scriptable* Sender, Trigger* parameters);
	static int LevelPartyGT(Scriptable* Sender, Trigger* parameters);
	static int LevelPartyLT(Scriptable* Sender, Trigger* parameters);
	static int LocalsEqual(Scriptable* Sender, Trigger* parameters);
	static int LocalsGT(Scriptable* Sender, Trigger* parameters);
	static int LocalsLT(Scriptable* Sender, Trigger* parameters);
	static int LOS(Scriptable* Sender, Trigger* parameters);
	static int ModalState(Scriptable* Sender, Trigger* parameters);
	static int Morale(Scriptable* Sender, Trigger* parameters);
	static int MoraleGT(Scriptable* Sender, Trigger* parameters);
	static int MoraleLT(Scriptable* Sender, Trigger* parameters);
	static int NamelessBitTheDust(Scriptable* Sender, Trigger* parameters);
	static int NearbyDialog(Scriptable* Sender, Trigger* parameters);
	static int NearLocation(Scriptable* Sender, Trigger* parameters);
	static int NearSavedLocation(Scriptable* Sender, Trigger* parameters);
	static int NotStateCheck(Scriptable* Sender, Trigger* parameters);
	static int NullDialog(Scriptable* Sender, Trigger* parameters);
	static int NumCreatures(Scriptable* Sender, Trigger* parameters);
	static int NumCreaturesGT(Scriptable* Sender, Trigger* parameters);
	static int NumCreaturesLT(Scriptable* Sender, Trigger* parameters);
	static int NumCreatureVsParty(Scriptable* Sender, Trigger* parameters);
	static int NumCreatureVsPartyGT(Scriptable* Sender, Trigger* parameters);
	static int NumCreatureVsPartyLT(Scriptable* Sender, Trigger* parameters);
	static int NumDead(Scriptable* Sender, Trigger* parameters);
	static int NumDeadGT(Scriptable* Sender, Trigger* parameters);
	static int NumDeadLT(Scriptable* Sender, Trigger* parameters);
	static int NumItems(Scriptable* Sender, Trigger* parameters);
	static int NumItemsGT(Scriptable* Sender, Trigger* parameters);
	static int NumItemsLT(Scriptable* Sender, Trigger* parameters);
	static int NumItemsParty(Scriptable* Sender, Trigger* parameters);
	static int NumItemsPartyGT(Scriptable* Sender, Trigger* parameters);
	static int NumItemsPartyLT(Scriptable* Sender, Trigger* parameters);
	static int NumTimesTalkedTo(Scriptable* Sender, Trigger* parameters);
	static int NumTimesTalkedToGT(Scriptable* Sender, Trigger* parameters);
	static int NumTimesTalkedToLT(Scriptable* Sender, Trigger* parameters);
	static int ObjectActionListEmpty(Scriptable* Sender, Trigger* parameters);
	static int OnCreation(Scriptable* Sender, Trigger* parameters);
	static int OnScreen(Scriptable* Sender, Trigger* parameters);
	static int OpenState(Scriptable* Sender, Trigger* parameters);
	static int Or(Scriptable* Sender, Trigger* parameters);
	static int OwnsFloaterMessage(Scriptable* Sender, Trigger* parameters);
	static int PartyCountEQ(Scriptable* Sender, Trigger* parameters);
	static int PartyCountGT(Scriptable* Sender, Trigger* parameters);
	static int PartyCountLT(Scriptable* Sender, Trigger* parameters);
	static int PartyCountAliveEQ(Scriptable* Sender, Trigger* parameters);
	static int PartyCountAliveGT(Scriptable* Sender, Trigger* parameters);
	static int PartyCountAliveLT(Scriptable* Sender, Trigger* parameters);
	static int PartyGold(Scriptable* Sender, Trigger* parameters);
	static int PartyGoldGT(Scriptable* Sender, Trigger* parameters);
	static int PartyGoldLT(Scriptable* Sender, Trigger* parameters);
	static int PartyHasItem(Scriptable* Sender, Trigger* parameters);
	static int PartyHasItemIdentified(Scriptable* Sender, Trigger* parameters);
	static int PartyMemberDied(Scriptable* Sender, Trigger* parameters);
	static int PCCanSeePoint(Scriptable* Sender, Trigger* parameters);
	static int PCInStore(Scriptable* Sender, Trigger* parameters);
	static int PersonalSpaceDistance(Scriptable* Sender, Trigger* parameters);
	static int Proficiency(Scriptable* Sender, Trigger* parameters);
	static int ProficiencyGT(Scriptable* Sender, Trigger* parameters);
	static int ProficiencyLT(Scriptable* Sender, Trigger* parameters);
	static int Race(Scriptable* Sender, Trigger* parameters);
	static int RandomNum(Scriptable* Sender, Trigger* parameters);
	static int RandomNumGT(Scriptable* Sender, Trigger* parameters);
	static int RandomNumLT(Scriptable* Sender, Trigger* parameters);
	static int Range(Scriptable* Sender, Trigger* parameters);
	static int RealGlobalTimerExact(Scriptable* Sender, Trigger* parameters);
	static int RealGlobalTimerExpired(Scriptable* Sender, Trigger* parameters);
	static int RealGlobalTimerNotExpired(Scriptable* Sender, Trigger* parameters);
	static int Reputation(Scriptable* Sender, Trigger* parameters);
	static int ReputationGT(Scriptable* Sender, Trigger* parameters);
	static int ReputationLT(Scriptable* Sender, Trigger* parameters);
	static int See(Scriptable* Sender, Trigger* parameters);
	static int Specifics(Scriptable* Sender, Trigger* parameters);
	static int StateCheck(Scriptable* Sender, Trigger* parameters);
	static int StuffGlobalRandom(Scriptable* Sender, Trigger* parameters);
	static int SubRace(Scriptable* Sender, Trigger* parameters);
	static int TargetUnreachable(Scriptable* Sender, Trigger* parameters);
	static int Team(Scriptable* Sender, Trigger* parameters);
	static int Time(Scriptable* Sender, Trigger* parameters);
	static int TimeGT(Scriptable* Sender, Trigger* parameters);
	static int TimeLT(Scriptable* Sender, Trigger* parameters);
	static int TookDamage(Scriptable* Sender, Trigger* parameters);
	static int TotalItemCnt(Scriptable* Sender, Trigger* parameters);
	static int TotalItemCntGT(Scriptable* Sender, Trigger* parameters);
	static int TotalItemCntLT(Scriptable* Sender, Trigger* parameters);
	static int True(Scriptable* Sender, Trigger* parameters);
	static int TrapTriggered(Scriptable* Sender, Trigger* parameters);
	static int UnselectableVariable(Scriptable* Sender, Trigger* parameters);
	static int UnselectableVariableGT(Scriptable* Sender, Trigger* parameters);
	static int UnselectableVariableLT(Scriptable* Sender, Trigger* parameters);
	static int Vacant(Scriptable* Sender, Trigger* parameters);
	static int IsWeather(Scriptable* Sender, Trigger* parameters);
	static int Xor(Scriptable* Sender, Trigger* parameters);
	static int XP(Scriptable* Sender, Trigger* parameters);
	static int XPGT(Scriptable* Sender, Trigger* parameters);
	static int XPLT(Scriptable* Sender, Trigger* parameters);
public:
	//Actions
	static void Activate(Scriptable* Sender, Action* parameters);
	static void AddAreaFlag(Scriptable* Sender, Action* parameters);
	static void AddAreaType(Scriptable* Sender, Action* parameters);
	static void AddExperienceParty(Scriptable *Sender, Action* parameters);
	static void AddExperiencePartyGlobal(Scriptable *Sender, Action* parameters);
	static void AddGlobals(Scriptable* Sender, Action* parameters);
	static void AddJournalEntry(Scriptable* Sender, Action* parameters);
	static void AddMapnote(Scriptable* Sender, Action* parameters);
	static void AddSpecialAbility(Scriptable* Sender, Action* parameters);
	static void AddWayPoint(Scriptable* Sender, Action* parameters);
	static void AddXP2DA(Scriptable *Sender, Action* parameters);
	static void AddXPObject(Scriptable *Sender, Action* parameters);
	static void AdvanceTime(Scriptable *Sender, Action* parameters);
	static void Ally(Scriptable* Sender, Action* parameters);
	static void AmbientActivate(Scriptable* Sender, Action* parameters);
	static void ApplyDamage(Scriptable* Sender, Action* parameters);
	static void ApplyDamagePercent(Scriptable* Sender, Action* parameters);
	static void AttachTransitionToDoor(Scriptable* Sender, Action* parameters);
	static void Attack(Scriptable* Sender, Action* parameters);
	static void AttackReevaluate(Scriptable* Sender, Action* parameters);
	static void BattleSong(Scriptable* Sender, Action* parameters);
	static void Berserk(Scriptable* Sender, Action* parameters);
	static void BitClear(Scriptable* Sender, Action* parameters);
	static void BitGlobal(Scriptable* Sender, Action* parameters);
	static void Calm(Scriptable* Sender, Action* parameters);
	static void ChangeAIScript(Scriptable* Sender, Action* parameters);
	static void ChangeAlignment(Scriptable* Sender, Action* parameters);
	static void ChangeAllegiance(Scriptable* Sender, Action* parameters);
	static void ChangeAnimation(Scriptable* Sender, Action* parameters);
	static void ChangeAnimationNoEffect(Scriptable* Sender, Action* parameters);
	static void ChangeClass(Scriptable* Sender, Action* parameters);
	static void ChangeCurrentScript(Scriptable* Sender, Action* parameters);
	static void ChangeDialogue(Scriptable* Sender, Action* parameters);
	static void ChangeGender(Scriptable* Sender, Action* parameters);
	static void ChangeGeneral(Scriptable* Sender, Action* parameters);
	static void ChangeRace(Scriptable* Sender, Action* parameters);
	static void ChangeSpecifics(Scriptable* Sender, Action* parameters);
	static void ChangeStat(Scriptable* Sender, Action* parameters);
	static void ClearActions(Scriptable* Sender, Action* parameters);
	static void ClearAllActions(Scriptable* Sender, Action* parameters);
	static void CloseDoor(Scriptable* Sender, Action* parameters);
	static void ContainerEnable(Scriptable* Sender, Action* parameters);
	static void Continue(Scriptable* Sender, Action* parameters);
	static void CopyGroundPilesTo(Scriptable* Sender, Action* parameters);
	static void CreateCreature(Scriptable* Sender, Action* parameters);
	static void CreateCreatureAtLocation(Scriptable* Sender, Action* parameters);
	static void CreateCreatureAtFeet(Scriptable* Sender, Action* parameters);
	static void CreateCreatureImpassable(Scriptable* Sender, Action* parameters);
	static void CreateCreatureImpassableAllowOverlap(Scriptable* Sender, Action* parameters);
	static void CreateCreatureObject(Scriptable* Sender, Action* parameters);
	static void CreateCreatureObjectOffset(Scriptable* Sender, Action* parameters);
	static void CreateCreatureObjectOffScreen(Scriptable* Sender, Action* parameters);
	static void CreateCreatureOffScreen(Scriptable* Sender, Action* parameters);
	static void CreateItem(Scriptable* Sender, Action* parameters);
	static void CreateItemNumGlobal(Scriptable* Sender, Action* parameters);
	static void CreatePartyGold(Scriptable *Sender, Action *parameters);
	static void CreateVisualEffect(Scriptable* Sender, Action* parameters);
	static void CreateVisualEffectObject(Scriptable* Sender,
		Action* parameters);
	static void CutSceneID(Scriptable* Sender, Action* parameters);
	static void Damage(Scriptable* Sender, Action* parameters);
	static void Deactivate(Scriptable* Sender, Action* parameters);
	static void Debug(Scriptable* Sender, Action* parameters);
	static void DestroyAllDestructableEquipment(Scriptable* Sender, Action* parameters);
	static void DestroyAllEquipment(Scriptable* Sender, Action* parameters);
	static void DestroyGold(Scriptable* Sender, Action* parameters);
	static void DestroyItem(Scriptable* Sender, Action* parameters);
	static void DestroyPartyGold(Scriptable* Sender, Action* parameters);
	static void DestroyPartyItem(Scriptable* Sender, Action* parameters);
	static void DestroyPartyItemNum(Scriptable* Sender, Action* parameters);
	static void DestroySelf(Scriptable* Sender, Action* parameters);
	static void DetectSecretDoor(Scriptable* Sender, Action* parameters);
	static void Dialogue(Scriptable* Sender, Action* parameters);
	static void DialogueForceInterrupt(Scriptable* Sender, Action* parameters);
	static void DisplayMessage(Scriptable* Sender, Action* parameters);
	static void DisplayString(Scriptable* Sender, Action* parameters);
	static void DisplayStringHead(Scriptable* Sender, Action* parameters);
	static void DisplayStringHeadOwner(Scriptable* Sender, Action* parameters);
	static void DisplayStringNoName(Scriptable* Sender, Action* parameters);
	static void DisplayStringNoNameHead(Scriptable* Sender, Action* parameters);
	static void DisplayStringWait(Scriptable* Sender, Action* parameters);
	static void DropInventory(Scriptable* Sender, Action* parameters);
	static void DropInventoryEX(Scriptable* Sender, Action* parameters);
	static void DropItem(Scriptable* Sender, Action* parameters);
	static void EndCredits(Scriptable* Sender, Action* parameters);
	static void EndCutSceneMode(Scriptable* Sender, Action* parameters);
	static void Enemy(Scriptable* Sender, Action* parameters);
	static void EscapeArea(Scriptable* Sender, Action* parameters);
	static void EscapeAreaDestroy(Scriptable* Sender, Action* parameters);
	static void EscapeAreaObject(Scriptable* Sender, Action* parameters);
	static void EscapeAreaObjectNoSee(Scriptable* Sender, Action* parameters);
	static void EquipItem(Scriptable *Sender, Action *parameters);
	static void ExpansionEndCredits(Scriptable* Sender, Action* parameters);
	static void Explore(Scriptable *Sender, Action *parameters);
	static void ExploreMapChunk(Scriptable *Sender, Action *parameters);
	static void Face(Scriptable* Sender, Action* parameters);
	static void FaceObject(Scriptable* Sender, Action* parameters);
	static void FaceSavedLocation(Scriptable* Sender, Action* parameters);
	static void FadeFromColor(Scriptable* Sender, Action* parameters);
	static void FadeToColor(Scriptable* Sender, Action* parameters);
	static void FindTraps(Scriptable* Sender, Action* parameters);
	static void FloatMessageFixed(Scriptable* Sender, Action* parameters);
	static void FloatMessageFixedRnd(Scriptable* Sender, Action* parameters);
	static void FloatMessageRnd(Scriptable* Sender, Action* parameters);
	static void ForceAIScript(Scriptable* Sender, Action* parameters);
	static void ForceAttack(Scriptable* Sender, Action* parameters);
	static void ForceFacing(Scriptable* Sender, Action* parameters);
	static void ForceLeaveAreaLUA(Scriptable* Sender, Action* parameters);
	static void ForceSpell(Scriptable* Sender, Action* parameters);
	static void FullHeal(Scriptable* Sender, Action* parameters);
	static void GetItem(Scriptable* Sender, Action* parameters);
	static void GiveItem(Scriptable* Sender, Action* parameters);
	static void GivePartyAllEquipment(Scriptable* Sender, Action* parameters);
	static void GivePartyGold(Scriptable* Sender, Action* parameters);
	static void GivePartyGoldGlobal(Scriptable* Sender, Action* parameters);
	static void GlobalAddGlobal(Scriptable* Sender, Action* parameters);
	static void GlobalAndGlobal(Scriptable* Sender, Action* parameters);
	static void GlobalBAnd(Scriptable* Sender, Action* parameters);
	static void GlobalBAndGlobal(Scriptable* Sender, Action* parameters);
	static void GlobalBitGlobal(Scriptable* Sender, Action* parameters);
	static void GlobalBOr(Scriptable* Sender, Action* parameters);
	static void GlobalBOrGlobal(Scriptable* Sender, Action* parameters);
	static void GlobalMax(Scriptable* Sender, Action* parameters);
	static void GlobalMaxGlobal(Scriptable* Sender, Action* parameters);
	static void GlobalMin(Scriptable* Sender, Action* parameters);
	static void GlobalMinGlobal(Scriptable* Sender, Action* parameters);
	static void GlobalOrGlobal(Scriptable* Sender, Action* parameters);
	static void GlobalSetGlobal(Scriptable* Sender, Action* parameters);
	static void GlobalShL(Scriptable* Sender, Action* parameters);
	static void GlobalShLGlobal(Scriptable* Sender, Action* parameters);
	static void GlobalShout(Scriptable* Sender, Action* parameters);
	static void GlobalShR(Scriptable* Sender, Action* parameters);
	static void GlobalShRGlobal(Scriptable* Sender, Action* parameters);
	static void GlobalSubGlobal(Scriptable* Sender, Action* parameters);
	static void GlobalXor(Scriptable* Sender, Action* parameters);
	static void GlobalXorGlobal(Scriptable* Sender, Action* parameters);
	static void Help(Scriptable* Sender, Action* parameters);
	static void Hide(Scriptable* Sender, Action* parameters);
	static void HideAreaOnMap(Scriptable* Sender, Action* parameters);
	static void HideCreature(Scriptable* Sender, Action* parameters);
	static void HideGUI(Scriptable* Sender, Action* parameters);
	static void IncInternal(Scriptable* Sender, Action* parameters);
	static void IncMoraleAI(Scriptable* Sender, Action* parameters);
	static void IncrementChapter(Scriptable* Sender, Action* parameters);
	static void IncrementGlobal(Scriptable* Sender, Action* parameters);
	static void IncrementGlobalOnce(Scriptable* Sender, Action* parameters);
	static void IncrementExtraProficiency(Scriptable* Sender, Action* parameters);
	static void IncrementProficiency(Scriptable* Sender, Action* parameters);
	static void Interact(Scriptable* Sender, Action* parameters);
	static void JoinParty(Scriptable* Sender, Action* parameters);
	static void JumpToPoint(Scriptable* Sender, Action* parameters);
	static void JumpToPointInstant(Scriptable* Sender, Action* parameters);
	static void JumpToObject(Scriptable* Sender, Action* parameters);
	static void JumpToSavedLocation(Scriptable* Sender, Action* parameters);
	static void Kill(Scriptable* Sender, Action* parameters);
	static void KillFloatMessage(Scriptable* Sender, Action* parameters);
	static void LeaveArea(Scriptable* Sender, Action* parameters);
	static void LeaveAreaLUA(Scriptable* Sender, Action* parameters);
	static void LeaveAreaLUAEntry(Scriptable* Sender, Action* parameters);
	static void LeaveAreaLUAPanic(Scriptable* Sender, Action* parameters);
	static void LeaveAreaLUAPanicEntry(Scriptable* Sender, Action* parameters);
	static void LeaveParty(Scriptable* Sender, Action* parameters);
	static void Lock(Scriptable* Sender, Action* parameters);
	static void LockScroll(Scriptable* Sender, Action* parameters);
	static void MarkObject(Scriptable* Sender, Action* parameters);
	static void MakeGlobal(Scriptable* Sender, Action* parameters);
	static void MakeUnselectable(Scriptable* Sender, Action* parameters);
	static void MoraleDec(Scriptable* Sender, Action* parameters);
	static void MoraleInc(Scriptable* Sender, Action* parameters);
	static void MoraleSet(Scriptable* Sender, Action* parameters);
	static void MoveBetweenAreas(Scriptable* Sender, Action* parameters);
	static void MoveBetweenAreasEffect(Scriptable* Sender, Action* parameters);
	static void MoveGlobal(Scriptable* Sender, Action* parameters);
	static void MoveGlobalObject(Scriptable* Sender, Action* parameters);
	static void MoveGlobalObjectOffScreen(Scriptable* Sender, Action* parameters);
	static void MoveGlobalsTo(Scriptable* Sender, Action* parameters);
	static void MoveInventory(Scriptable *Sender, Action* parameters);
	static void MoveToCenterOfScreen(Scriptable* Sender, Action* parameters);
	static void MoveToPoint(Scriptable* Sender, Action* parameters);
	static void MoveToPointNoInterrupt(Scriptable* Sender, Action* parameters);
	static void MoveToPointNoRecticle(Scriptable* Sender, Action* parameters);
	static void MoveToObject(Scriptable* Sender, Action* parameters);
	static void MoveToObjectNoInterrupt(Scriptable* Sender, Action* parameters);
	static void MoveToOffset(Scriptable* Sender, Action* parameters);
	static void MoveToSavedLocation(Scriptable* Sender, Action* parameters);
	static void MoveToSavedLocationDelete(Scriptable* Sender, Action* parameters);
	static void MoveViewPoint(Scriptable* Sender, Action* parameters);
	static void MoveViewObject(Scriptable* Sender, Action* parameters);
	static void NIDSpecial1(Scriptable* Sender, Action* parameters);
	static void NIDSpecial2(Scriptable* Sender, Action* parameters);
	static void NoAction(Scriptable* Sender, Action* parameters);
	static void NoActionAtAll(Scriptable* Sender, Action* parameters);
	static void OpenDoor(Scriptable* Sender, Action* parameters);
	static void Panic(Scriptable* Sender, Action* parameters);
	static void PickPockets(Scriptable* Sender, Action* parameters);
	static void PlayBardSong(Scriptable* Sender, Action* parameters);
	static void PlayDead(Scriptable* Sender, Action* parameters);
	static void PlayDeadInterruptable(Scriptable* Sender, Action* parameters);
	static void PlayerDialogue(Scriptable* Sender, Action* parameters);
	static void PlaySequence(Scriptable* Sender, Action* parameters);
	static void PlaySound(Scriptable* Sender, Action* parameters);
	static void PlaySoundNotRanged(Scriptable* Sender, Action* parameters);
	static void PlaySoundPoint(Scriptable* Sender, Action* parameters);
	static void Plunder(Scriptable* Sender, Action* parameters);
	static void Polymorph(Scriptable* Sender, Action* parameters);
	static void PolymorphCopy(Scriptable* Sender, Action* parameters);
	static void PolymorphCopyBase(Scriptable* Sender, Action* parameters);
	static void Protect(Scriptable* Sender, Action* parameters);
	static void ProtectPoint(Scriptable* Sender, Action* parameters);
	static void QuitGame(Scriptable* Sender, Action* parameters);
	static void RandomFly(Scriptable* Sender, Action* parameters);
	static void RandomWalk(Scriptable* Sender, Action* parameters);
	static void RandomWalkContinuous(Scriptable* Sender, Action* parameters);
	static void RealSetGlobalTimer(Scriptable* Sender, Action* parameters);
	static void Recoil(Scriptable* Sender, Action* parameters);
	static void RegainPaladinHood(Scriptable* Sender, Action* parameters);
	static void RegainRangerHood(Scriptable* Sender, Action* parameters);
	static void RemoveAreaFlag(Scriptable* Sender, Action* parameters);
	static void RemoveAreaType(Scriptable* Sender, Action* parameters);
	static void RemoveJournalEntry(Scriptable* Sender, Action* parameters);
	static void RemoveMapnote(Scriptable* Sender, Action* parameters);
	static void RemovePaladinHood(Scriptable* Sender, Action* parameters);
	static void RemoveRangerHood(Scriptable* Sender, Action* parameters);
	static void RemoveSpell(Scriptable* Sender, Action* parameters);
	static void ReputationInc(Scriptable* Sender, Action* parameters);
	static void ReputationSet(Scriptable* Sender, Action* parameters);
	static void RestorePartyLocation(Scriptable *Sender, Action* parameters);
	static void RevealAreaOnMap(Scriptable* Sender, Action* parameters);
	static void RunAwayFrom(Scriptable* Sender, Action* parameters);
	static void RunAwayFromNoInterrupt(Scriptable* Sender, Action* parameters);
	static void SaveGame(Scriptable* Sender, Action* parameters);
	static void SaveLocation(Scriptable* Sender, Action* parameters);
	static void SaveObjectLocation(Scriptable* Sender, Action* parameters);
	static void ScreenShake(Scriptable* Sender, Action* parameters);
	static void SetAnimState(Scriptable* Sender, Action* parameters);
	static void SetApparentName(Scriptable* Sender, Action* parameters);
	static void SetAreaFlags(Scriptable* Sender, Action* parameters);
	static void SetAreaRestFlag(Scriptable* Sender, Action* parameters);
	static void SetArmourLevel(Scriptable* Sender, Action* parameters);
	static void SetBeenInPartyFlags(Scriptable* Sender, Action* parameters);
	static void SetCreatureAreaFlags(Scriptable* Sender, Action* parameters);
	static void SetDialogue(Scriptable* Sender, Action* parameters);
	static void SetDialogueRange(Scriptable* Sender, Action* parameters);
	static void SetDoorLocked(Scriptable* Sender, Action* parameters);
	static void SetExtendedNight(Scriptable* Sender, Action* parameters);
	static void SetFaction(Scriptable* Sender, Action* parameters);
	static void SetGabber(Scriptable* Sender, Action* parameters);
	static void SetGlobal(Scriptable* Sender, Action* parameters);
	static void SetGlobalRandom(Scriptable* Sender, Action* parameters);
	static void SetGlobalTimer(Scriptable* Sender, Action* parameters);
	static void SetGlobalTimerOnce(Scriptable* Sender, Action* parameters);
	static void SetGlobalTint(Scriptable* Sender, Action* parameters);
	static void SetHomeLocation(Scriptable* Sender, Action* parameters);
	static void SetHP(Scriptable* Sender, Action* parameters);
	static void SetInternal(Scriptable* Sender, Action* parameters);
	static void SetLeavePartyDialogFile(Scriptable* Sender, Action* parameters);
	static void SetMasterArea(Scriptable* Sender, Action* parameters);
	static void SetMazeEasier(Scriptable* Sender, Action* parameters);
	static void SetMazeHarder(Scriptable* Sender, Action* parameters);
	static void SetMoraleAI(Scriptable* Sender, Action* parameters);
	static void SetMusic(Scriptable* Sender, Action* parameters);
	static void SetNamelessClass(Scriptable* Sender, Action* parameters);
	static void SetNamelessDisguise(Scriptable* Sender, Action* parameters);
	static void SetNumTimesTalkedTo(Scriptable* Sender, Action* parameters);
	static void SetPlayerSound(Scriptable* Sender, Action* parameters);
	static void SetQuestDone(Scriptable* Sender, Action* parameters);
	static void SetRegularName(Scriptable* Sender, Action* parameters);
	static void SetRestEncounterChance(Scriptable* Sender, Action* parameters);
	static void SetRestEncounterProbabilityDay(Scriptable* Sender, Action* parameters);
	static void SetRestEncounterProbabilityNight(Scriptable* Sender, Action* parameters);
	static void SetScriptName(Scriptable* Sender, Action* parameters);
	static void SetTeam(Scriptable* Sender, Action* parameters);
	static void SetTeamBit(Scriptable* Sender, Action* parameters);
	static void SetTextColor(Scriptable* Sender, Action* parameters);
	static void SetToken(Scriptable* Sender, Action* parameters);
	static void SetTokenGlobal(Scriptable* Sender, Action* parameters);
	static void SetVisualRange(Scriptable* Sender, Action* parameters);
	static void SG(Scriptable* Sender, Action* parameters);
	static void Shout(Scriptable* Sender, Action* parameters);
	static void SmallWait(Scriptable* Sender, Action* parameters);
	static void SmallWaitRandom(Scriptable* Sender, Action* parameters);
	static void SoundActivate(Scriptable* Sender, Action* parameters);
	static void StartCutScene(Scriptable* Sender, Action* parameters);
	static void StartCutSceneMode(Scriptable* Sender, Action* parameters);
	static void StartDialogue(Scriptable* Sender, Action* parameters);
	static void StartDialogueInterrupt(Scriptable* Sender, Action* parameters);
	static void StartDialogueNoSet(Scriptable* Sender, Action* parameters);
	static void StartDialogueNoSetInterrupt(Scriptable* Sender,
		Action* parameters);
	static void StartDialogueOverride(Scriptable* Sender, Action* parameters);
	static void StartDialogueOverrideInterrupt(Scriptable* Sender,
		Action* parameters);
	static void StartMovie(Scriptable* Sender, Action* parameters);
	static void StartMusic(Scriptable* Sender, Action* parameters);
	static void StartRainNow(Scriptable* Sender, Action* parameters);
	static void StartSong(Scriptable* Sender, Action* parameters);
	static void StartStore(Scriptable* Sender, Action* parameters);
	static void StaticPalette(Scriptable* Sender, Action* parameters);
	static void StaticStart(Scriptable* Sender, Action* parameters);
	static void StaticStop(Scriptable* Sender, Action* parameters);
	static void StopMoving(Scriptable* Sender, Action* parameters);
	static void StorePartyLocation(Scriptable *Sender, Action* parameters);
	static void Swing(Scriptable* Sender, Action* parameters);
	static void SwingOnce(Scriptable* Sender, Action* parameters);
	static void TakeItemList(Scriptable* Sender, Action* parameters);
	static void TakeItemReplace(Scriptable* Sender, Action* parameters);
	static void TakePartyGold(Scriptable* Sender, Action* parameters);
	static void TakePartyItem(Scriptable* Sender, Action* parameters);
	static void TakePartyItemAll(Scriptable* Sender, Action* parameters);
	static void TakePartyItemNum(Scriptable* Sender, Action* parameters);
	static void TakePartyItemRange(Scriptable* Sender, Action* parameters);
	static void TeleportParty(Scriptable* Sender, Action* parameters);
	static void TextScreen(Scriptable* Sender, Action* parameters);
	static void TriggerActivation(Scriptable* Sender, Action* parameters);
	static void Turn(Scriptable* Sender, Action* parameters);
	static void TurnAMT(Scriptable* Sender, Action* parameters);
	static void UndoExplore(Scriptable *Sender, Action *parameters);
	static void UnhideGUI(Scriptable* Sender, Action* parameters);
	static void Unlock(Scriptable* Sender, Action* parameters);
	static void UnlockScroll(Scriptable* Sender, Action* parameters);
	static void UseContainer(Scriptable* Sender, Action* parameters);
	static void VerbalConstant(Scriptable* Sender, Action* parameters);
	static void VerbalConstantHead(Scriptable* Sender, Action* parameters);
	static void Wait(Scriptable* Sender, Action* parameters);
	static void WaitAnimation(Scriptable* Sender, Action* parameters);
	static void WaitRandom(Scriptable* Sender, Action* parameters);
	static void Weather(Scriptable* Sender, Action* parameters);
	static void XEquipItem(Scriptable *Sender, Action *parameters);
public:
	//Objects
	static Targets *BestAC(Scriptable *Sender, Targets *parameters);
	static Targets *EighthNearest(Scriptable *Sender, Targets *parameters);
	static Targets *EighthNearestEnemyOf(Scriptable *Sender, Targets *parameters);
	static Targets *EighthNearestEnemyOfType(Scriptable *Sender, Targets *parameters);
	static Targets *EighthNearestMyGroupOfType(Scriptable *Sender, Targets *parameters);
	static Targets *Farthest(Scriptable *Sender, Targets *parameters);
	static Targets *FifthNearest(Scriptable *Sender, Targets *parameters);
	static Targets *FifthNearestEnemyOf(Scriptable *Sender, Targets *parameters);
	static Targets *FifthNearestEnemyOfType(Scriptable *Sender, Targets *parameters);
	static Targets *FifthNearestMyGroupOfType(Scriptable *Sender, Targets *parameters);
	static Targets *FourthNearest(Scriptable *Sender, Targets *parameters);
	static Targets *FourthNearestEnemyOf(Scriptable *Sender, Targets *parameters);
	static Targets *FourthNearestEnemyOfType(Scriptable *Sender, Targets *parameters);
	static Targets *FourthNearestMyGroupOfType(Scriptable *Sender, Targets *parameters);
	static Targets *Gabber(Scriptable *Sender, Targets *parameters);
	static Targets *LastAttackerOf(Scriptable *Sender, Targets *parameters);
	static Targets *LastCommandedBy(Scriptable *Sender, Targets *parameters);
	static Targets *LastHeardBy(Scriptable *Sender, Targets *parameters);
	static Targets *LastHelp(Scriptable *Sender, Targets *parameters);
	static Targets *LastHitter(Scriptable *Sender, Targets *parameters);
	static Targets *LastSeenBy(Scriptable *Sender, Targets *parameters);
	static Targets *LastSummonerOf(Scriptable *Sender, Targets *parameters);
	static Targets *LastTalkedToBy(Scriptable *Sender, Targets *parameters);
	static Targets *LastTargetedBy(Scriptable *Sender, Targets *parameters);
	static Targets *LastTrigger(Scriptable *Sender, Targets *parameters);
	static Targets *LeastDamagedOf(Scriptable *Sender, Targets *parameters);
	static Targets *MostDamagedOf(Scriptable *Sender, Targets *parameters);
	static Targets *Myself(Scriptable *Sender, Targets *parameters);
	static Targets *Nearest(Scriptable *Sender, Targets *parameters);
	static Targets *NearestEnemyOf(Scriptable *Sender, Targets *parameters);
	static Targets *NearestEnemyOfType(Scriptable *Sender, Targets *parameters);
	static Targets *NearestMyGroupOfType(Scriptable *Sender, Targets *parameters);
	static Targets *NearestPC(Scriptable *Sender, Targets *parameters);
	static Targets *NinthNearest(Scriptable *Sender, Targets *parameters);
	static Targets *NinthNearestEnemyOf(Scriptable *Sender, Targets *parameters);
	static Targets *NinthNearestEnemyOfType(Scriptable *Sender, Targets *parameters);
	static Targets *NinthNearestMyGroupOfType(Scriptable *Sender, Targets *parameters);
	static Targets *Nothing(Scriptable *Sender, Targets *parameters);
	static Targets *Player1(Scriptable *Sender, Targets *parameters);
	static Targets *Player1Fill(Scriptable *Sender, Targets *parameters);
	static Targets *Player2(Scriptable *Sender, Targets *parameters);
	static Targets *Player2Fill(Scriptable *Sender, Targets *parameters);
	static Targets *Player3(Scriptable *Sender, Targets *parameters);
	static Targets *Player3Fill(Scriptable *Sender, Targets *parameters);
	static Targets *Player4(Scriptable *Sender, Targets *parameters);
	static Targets *Player4Fill(Scriptable *Sender, Targets *parameters);
	static Targets *Player5(Scriptable *Sender, Targets *parameters);
	static Targets *Player5Fill(Scriptable *Sender, Targets *parameters);
	static Targets *Player6(Scriptable *Sender, Targets *parameters);
	static Targets *Player6Fill(Scriptable *Sender, Targets *parameters);
	static Targets *Protagonist(Scriptable *Sender, Targets *parameters);
	static Targets *ProtectedBy(Scriptable *Sender, Targets *parameters);
	static Targets *ProtectorOf(Scriptable *Sender, Targets *parameters);
	static Targets *SecondNearest(Scriptable *Sender, Targets *parameters);
	static Targets *SecondNearestEnemyOf(Scriptable *Sender, Targets *parameters);
	static Targets *SecondNearestEnemyOfType(Scriptable *Sender, Targets *parameters);
	static Targets *SecondNearestMyGroupOfType(Scriptable *Sender, Targets *parameters);
	static Targets *SelectedCharacter(Scriptable *Sender, Targets *parameters);
	static Targets *SeventhNearest(Scriptable *Sender, Targets *parameters);
	static Targets *SeventhNearestEnemyOf(Scriptable *Sender, Targets *parameters);
	static Targets *SeventhNearestEnemyOfType(Scriptable *Sender, Targets *parameters);
	static Targets *SeventhNearestMyGroupOfType(Scriptable *Sender, Targets *parameters);
	static Targets *SixthNearest(Scriptable *Sender, Targets *parameters);
	static Targets *SixthNearestEnemyOf(Scriptable *Sender, Targets *parameters);
	static Targets *SixthNearestEnemyOfType(Scriptable *Sender, Targets *parameters);
	static Targets *SixthNearestMyGroupOfType(Scriptable *Sender, Targets *parameters);
	static Targets *StrongestOf(Scriptable *Sender, Targets *parameters);
	static Targets *StrongestOfMale(Scriptable *Sender, Targets *parameters);
	static Targets *TenthNearest(Scriptable *Sender, Targets *parameters);
	static Targets *TenthNearestEnemyOf(Scriptable *Sender, Targets *parameters);
	static Targets *TenthNearestEnemyOfType(Scriptable *Sender, Targets *parameters);
	static Targets *TenthNearestMyGroupOfType(Scriptable *Sender, Targets *parameters);
	static Targets *ThirdNearest(Scriptable *Sender, Targets *parameters);
	static Targets *ThirdNearestEnemyOf(Scriptable *Sender, Targets *parameters);
	static Targets *ThirdNearestEnemyOfType(Scriptable *Sender, Targets *parameters);
	static Targets *ThirdNearestMyGroupOfType(Scriptable *Sender, Targets *parameters);
	static Targets *WeakestOf(Scriptable *Sender, Targets *parameters);
	static Targets *WorstAC(Scriptable *Sender, Targets *parameters);

public:
	/*GemRB extensions/actions*/
	static void RunAwayFromPoint(Scriptable* Sender, Action* parameters);
	static void UnMakeGlobal(Scriptable* Sender, Action* parameters);
	static void UnloadArea(Scriptable* Sender, Action* parameters);

	/*GemRB extensions/objects*/
	static Targets *Player7(Scriptable *Sender, Targets *parameters);
	static Targets *Player7Fill(Scriptable *Sender, Targets *parameters);
	static Targets *Player8(Scriptable *Sender, Targets *parameters);
	static Targets *Player8Fill(Scriptable *Sender, Targets *parameters);
};

Action* GenerateAction(char* String, bool autoFree=true);
Trigger* GenerateTrigger(char* String);

#endif
