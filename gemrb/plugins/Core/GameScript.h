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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/GameScript.h,v 1.42 2004/02/10 20:48:05 avenger_teambg Exp $
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
#include <list>

#define GSASSERT(f,c) \
  if(!(f))  \
  {  \
  printf("Assertion failed: %s [0x%08X] Line %d",#f, c, __LINE__); \
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

class GEM_EXPORT Object {
public:
	Object() {
		RefCount = 1;
		objectName[0] = 0;
		PositionMask[0] = 0;

		eaField = 0;
		factionField = 0;
		teamField = 0;
		generalField = 0;
		raceField = 0;
		classField = 0;
		specificField = 0;
		genderField = 0;
		alignmentField = 0;
		identifiersField = 0;
		XobjectPosition = 0;
		YobjectPosition = 0;
		WobjectPosition = 0;
		HobjectPosition = 0;

		canary = 0xdeadbeef;
	};
	~Object() {
	}
public:
	int				eaField;
	int				factionField;
	int				teamField;
	int				generalField;
	int				raceField;
	int				classField;
	int				specificField;
	int				genderField;
	int				alignmentField;
	int				identifiersField;
	int				XobjectPosition;
	int				YobjectPosition;
	int				WobjectPosition;
	int             HobjectPosition;
	char            PositionMask[65];
	char			objectName[65];
private:
	int				RefCount;
	volatile unsigned long canary;
public:
	void Release() {
		GSASSERT(canary == 0xdeadbeef, canary);
		if(!RefCount) {
			printf("WARNING!!! Double Freeing in %s: Line %d\n", __FILE__, __LINE__);
			abort();
		}
		RefCount--;
		if(!RefCount)
			delete this;
	}
	void IncRef() {
		GSASSERT(canary == 0xdeadbeef, canary);
		RefCount++;
		printf("IncRef(%d) called in Line %d\n", RefCount, __LINE__);
		if(RefCount>=4)
		{
			printf("Refcount increased to: %d in object\n",RefCount);
			abort();
		}
	}
};

class GEM_EXPORT Trigger {
public:
	Trigger() {
	        objectParameter = NULL;
		RefCount = 1;
		string0Parameter[0] = 0;
		string1Parameter[0] = 0;
		canary = 0xdeadbeef;
	};
	~Trigger() {
		if(objectParameter) {
			objectParameter->Release();
		}
	}
public:
	unsigned short	triggerID;
	int				int0Parameter;
	int				flags;
	int				int1Parameter;
	int				int2Parameter;
	int				XpointParameter;
	int				YpointParameter;
	char			string0Parameter[65];
	char			string1Parameter[65];
	Object*			objectParameter;
private:
	int				RefCount;
	volatile unsigned long canary;
public:
	void Release() {
		GSASSERT(canary == 0xdeadbeef, canary);
		if(!RefCount) {
			printf("WARNING!!! Double Freeing in %s: Line %d\n", __FILE__, __LINE__);
			abort();
		}
		RefCount--;
		if(!RefCount)
			delete this;
	}
	void IncRef() {
		GSASSERT(canary == 0xdeadbeef, canary);
		RefCount++;
		printf("IncRef(%d) called in Line %d\n", RefCount, __LINE__);
		if(RefCount>=4)
		{
			printf("Refcount increased to: %d in trigger\n",RefCount);
			abort();
		}
	}
};

class GEM_EXPORT Condition {
public:
	Condition() {
		RefCount = 1;
		triggers = NULL;
		triggersCount = 0;
		canary = 0xdeadbeef;
	};
	~Condition() {
		if(!triggers)
			return;
		for(int c = 0; c < triggersCount; c++) {
			if(triggers[c])
				triggers[c]->Release();
		}
		//delete[] triggers;
		delete triggers;
	}
public:
	unsigned short triggersCount;
	Trigger ** triggers;
private:
	int				RefCount;
	volatile unsigned long canary;
public:
	void Release() {
		GSASSERT(canary == 0xdeadbeef, canary);
		if(!RefCount) {
			printf("WARNING!!! Double Freeing in %s: Line %d\n", __FILE__, __LINE__);
			abort();
		}
		RefCount--;
		if(!RefCount)
			delete this;
	}
	void IncRef() {
		GSASSERT(canary == 0xdeadbeef, canary);
		RefCount++;
		printf("IncRef(%d) called in Line %d\n", RefCount, __LINE__);
		if(RefCount>=4)
		{
			printf("Refcount increased to: %d in condition\n",RefCount);
			abort();
		}
	}
};

class GEM_EXPORT Action {
public:
	Action() {
		actionID = 0;
		objects[0] = NULL;
		objects[1] = NULL;
		objects[2] = NULL;
		string0Parameter[0] = 0;
		string1Parameter[0] = 0;
		int0Parameter = 0;
		XpointParameter = 0;
		YpointParameter = 0;
		int1Parameter = 0;
		int2Parameter = 0;
		RefCount = 1;
		canary = 0xdeadbeef;
	};
	~Action() {
		for(int c = 0; c < 3; c++) {
			if(objects[c])
				objects[c]->Release();
		}
	}
public:
	unsigned short	actionID;
	Object*			objects[3];
	int				int0Parameter;
	int				XpointParameter;
	int				YpointParameter;
	int				int1Parameter;
	int				int2Parameter;
	char			string0Parameter[65];
	char			string1Parameter[65];
private:
	int				RefCount;
	volatile unsigned long canary;
public:
	void Release() {
		GSASSERT(canary == 0xdeadbeef, canary);
		if(!RefCount) {
			printf("WARNING!!! Double Freeing in %s: Line %d\n", __FILE__, __LINE__);
			abort();
		}
		RefCount--;
		if(!RefCount)
			delete this;
	}
	void IncRef() {
		GSASSERT(canary == 0xdeadbeef, canary);
		RefCount++;
		printf("IncRef(%d) called in Line %d\n", RefCount, __LINE__);
		if(RefCount>=4)
		{
			printf("Refcount increased to: %d in action %d\n",RefCount, actionID);
			abort();
		}
	}
};

class GEM_EXPORT Response {
public:
	Response() {
		RefCount = 1;
		actions = NULL;
		weight = 0;
		actionsCount = 0;
		canary = 0xdeadbeef;
	};
	~Response() {
		if(!actions)
			return;
		for(int c = 0; c < actionsCount; c++) {
			if(actions[c])
				actions[c]->Release();
		}
		//delete[] actions;
		delete actions;
	}
public:
	unsigned char weight;
	unsigned char actionsCount;
	Action ** actions;
private:
	int				RefCount;
	volatile unsigned long canary;
public:
	void Release() {
		GSASSERT(canary == 0xdeadbeef, canary);
		if(!RefCount) {
			printf("WARNING!!! Double Freeing in %s: Line %d\n", __FILE__, __LINE__);
			abort();
		}
		RefCount--;
		if(!RefCount)
			delete this;
	}
	void IncRef() {
		GSASSERT(canary == 0xdeadbeef, canary);
		RefCount++;
		printf("IncRef(%d) called in Line %d\n", RefCount, __LINE__);
		if(RefCount>=4)
		{
			printf("Refcount increased to: %d in response\n",RefCount);
			abort();
		}
	}
};

class GEM_EXPORT ResponseSet {
public:
	ResponseSet() {
		RefCount = 1;
		responses = NULL;
		responsesCount = 0;
		canary = 0xdeadbeef;
	};
	~ResponseSet() {
		if(!responses)
			return;
		for(int b = 0; b < responsesCount; b++) {
			Response * rP = responses[b];
			rP->Release();
		}
		//delete[] responses;
		delete responses;
	}
public:
	unsigned short responsesCount;
	Response ** responses;
private:
	int				RefCount;
	volatile unsigned long canary;
public:
	void Release() {
		GSASSERT(canary == 0xdeadbeef, canary);
		if(!RefCount) {
			printf("WARNING!!! Double Freeing in %s: Line %d\n", __FILE__, __LINE__);
			abort();
		}
		RefCount--;
		if(!RefCount)
			delete this;
	}
	void IncRef() {
		GSASSERT(canary == 0xdeadbeef, canary);
		RefCount++;
		printf("IncRef(%d) called in Line %d\n", RefCount, __LINE__);
		if(RefCount>=4)
		{
			printf("Refcount increased to: %d\n",RefCount);
			abort();
		}
	}
};

class GEM_EXPORT ResponseBlock {
public:
	ResponseBlock() {
		RefCount = 1;
		condition = NULL;
		responseSet = NULL;
		canary = 0xdeadbeef;
	};
	~ResponseBlock() {
		if(condition)
			condition->Release();
		if(responseSet)
			responseSet->Release();
	}
public:
	Condition * condition;
	ResponseSet * responseSet;
private:
	int				RefCount;
	volatile unsigned long canary;
public:
	void Release() {
		GSASSERT(canary == 0xdeadbeef, canary);
		if(!RefCount) {
			printf("WARNING!!! Double Freeing in %s: Line %d\n", __FILE__, __LINE__);
			abort();
		}
		RefCount--;
		if(!RefCount)
			delete this;
	}
	void IncRef() {
		GSASSERT(canary == 0xdeadbeef, canary);
		RefCount++;
		printf("IncRef(%d) called in Line %d\n", RefCount, __LINE__);
		if(RefCount>=4)
		{
			printf("Refcount increased to: %d\n",RefCount);
			abort();
		}
	}
};

class GEM_EXPORT Script {
public:
	Script(const char * ScriptName) {
		canary = 0xdeadbeef;
		RefCount = 1;
		responseBlocks = NULL;
		responseBlocksCount = 0;
		if(!Name) {
			this->Name[0] = 0;
			return;
		}
		strncpy(Name, ScriptName, 8);
		Name[8] = 0;
	};
	~Script() {
		FreeResponseBlocks();
		printf("Freeing Script: %s\n",GetName());
	}
	const char *GetName() {
		return this?Name:"<none>";
	}
	void AllocateBlocks(unsigned int count) {
		if(!count) {
			FreeResponseBlocks();
			responseBlocks = NULL;
			responseBlocksCount = 0;
		}
		responseBlocks = new ResponseBlock*[count];
		responseBlocksCount = count;
	}
private:
	void FreeResponseBlocks() {
		if(!responseBlocks)
			return;
		for(unsigned int i = 0; i < responseBlocksCount; i++) {
			if(responseBlocks[i])
				responseBlocks[i]->Release();
		}
		//delete[] responseBlocks;
		delete responseBlocks;
	}
public:
	unsigned int responseBlocksCount;
	ResponseBlock ** responseBlocks;
	char Name[9];
private:
	int				RefCount;
	volatile unsigned long canary;
public:
	void Release() {
		GSASSERT(canary == 0xdeadbeef, canary);
		if(!RefCount) {
			printf("WARNING!!! Double Freeing in %s: Line %d\n", __FILE__, __LINE__);
			abort();
		}
		RefCount--;
		if(!RefCount)
			delete this;
	}
	void IncRef() {
		GSASSERT(canary == 0xdeadbeef, canary);
		RefCount++;
		printf("IncRef(%d) called in Line %d\n", RefCount, __LINE__);
		if(RefCount>=4)
		{
			printf("Refcount increased to: %d\n",RefCount);
			abort();
		}
	}
};

typedef int (* TriggerFunction)(Scriptable*, Trigger*);
typedef void (* ActionFunction)(Scriptable*, Action*);
typedef void (* ObjectFunction)(); //not known yet

struct TriggerLink {
	const char *Name;
	TriggerFunction Function;
};

#define BD_STRING0   0
#define BD_TARGET    1
#define BD_SOURCE    2
#define BD_RESERVED  3
#define BD_LOCMASK   3  //where is the dialog resref
#define BD_INTERRUPT 4  //interrupts action
#define BD_TALKCOUNT 8  //increases talkcount
#define BD_SETDIALOG 16 //also sets dialog (for string0)
#define BD_CHECKDIST 32 //checks distance, if needs, walks up
#define BD_OWN       64 //source == target, works for player only

#define AF_NONE      0
#define AF_INSTANT   1
#define AF_CONTINUE  2
#define AF_MASK      3   //none, instant or continue
#define AF_BLOCKING  4

struct ActionLink {
	const char *Name;
	ActionFunction Function;
	int Flags;
};

struct ObjectLink {
	const char *Name;
	ObjectFunction Function;
};

#define IE_SCRIPT_ALWAYS		0
#define IE_SCRIPT_AREA			1
#define IE_SCRIPT_TRIGGER		2

#define MAX_TRIGGERS			0xFF
#define MAX_ACTIONS			400
#define MAX_OBJECTS			128

class GEM_EXPORT GameScript
{
public:
	Variables * locals;
	Script * script;
	Scriptable * MySelf;
	unsigned long scriptRunDelay;
	bool endReached;
	void Update();
	void EvaluateAllBlocks();
private: //Internal Functions
	Script* CacheScript(DataStream * stream, const char * Context);
	ResponseBlock * ReadResponseBlock(DataStream * stream);
	Condition * ReadCondition(DataStream * stream);
	ResponseSet * ReadResponseSet(DataStream * stream);
	Response * ReadResponse(DataStream * stream);
	Trigger * ReadTrigger(DataStream * stream);
	Object * DecodeObject(const char * line);
	bool EvaluateCondition(Scriptable * Sender, Condition * condition);
	static bool EvaluateTrigger(Scriptable * Sender, Trigger * trigger);
	int ExecuteResponseSet(Scriptable * Sender, ResponseSet * rS);
	int ExecuteResponse(Scriptable * Sender, Response * rE);
public:
	static void ExecuteAction(Scriptable * Sender, Action * aC);
	static Action* CreateAction(char *string, bool autoFree = true);
private:
	static Action * GenerateAction(char * String);
	static Trigger * GenerateTrigger(char * String);
	static Scriptable * GetActorFromObject(Scriptable * Sender, Object * oC);
	static void BeginDialog(Scriptable *Sender, Action *parameters, int flags);
	static unsigned char GetOrient(short sX, short sY, short dX, short dY);
private: //Internal variables
	unsigned long lastRunTime;
	unsigned long scriptType;
private: //Script Internal Variables
	bool continueExecution;
	std::list<Action*> programmedActions;
	char Name[9];
	bool freeLocals;
public:
	GameScript(const char * ResRef, unsigned char ScriptType, Variables * local = NULL);
	~GameScript();
	static void ReplaceMyArea(Scriptable *Sender, char *newVarName);
	static unsigned long CheckVariable(Scriptable *Sender, const char * VarName, const char * Context);
	static unsigned long CheckVariable(Scriptable *Sender, const char * VarName);

	static void SetVariable(Scriptable *Sender, const char * VarName, const char * Context, int value);
	static void SetVariable(Scriptable *Sender, const char * VarName, int value);
	static void ExecuteString(Scriptable * Sender, char * String);
	static bool EvaluateString(Scriptable * Sender, char * String);
public: //Script Functions
	//Triggers
	static int  Global(Scriptable * Sender, Trigger * parameters);
	static int  GlobalLT(Scriptable * Sender, Trigger * parameters);
	static int  GlobalGT(Scriptable * Sender, Trigger * parameters);
	static int  GlobalsEqual(Scriptable * Sender, Trigger * parameters);
	static int  OnCreation(Scriptable * Sender, Trigger * parameters);
	static int  PartyHasItem(Scriptable * Sender, Trigger * parameters);
	static int  True(Scriptable * Sender, Trigger * parameters);
	static int  ActionListEmpty(Scriptable * Sender, Trigger * parameters);
	static int  NumTimesTalkedTo(Scriptable * Sender, Trigger * parameters);
	static int  NumTimesTalkedToGT(Scriptable * Sender, Trigger * parameters);
	static int  NumTimesTalkedToLT(Scriptable * Sender, Trigger * parameters);
	static int  False(Scriptable * Sender, Trigger * parameters);
	static int  Alignment(Scriptable * Sender, Trigger * parameters);
	static int  Allegiance(Scriptable * Sender, Trigger * parameters);
	static int  Class(Scriptable * Sender, Trigger * parameters);
	static int  Exists(Scriptable * Sender, Trigger * parameters);
	static int  InParty(Scriptable * Sender, Trigger * parameters);
	static int  IsValidForPartyDialog(Scriptable * Sender, Trigger * parameters);
	static int  General(Scriptable * Sender, Trigger * parameters);
	static int  Range(Scriptable * Sender, Trigger * parameters);
	static int  Or(Scriptable * Sender, Trigger * parameters);
	static int  Clicked(Scriptable * Sender, Trigger * parameters);
	static int  Entered(Scriptable * Sender, Trigger * parameters);
	static int  Dead(Scriptable * Sender, Trigger * parameters);
	static int  See(Scriptable * Sender, Trigger * parameters);
	static int  BitCheck(Scriptable * Sender, Trigger * parameters);
	static int  CheckStat(Scriptable * Sender, Trigger * parameters);
	static int  CheckStatGT(Scriptable * Sender, Trigger * parameters);
	static int  CheckStatLT(Scriptable * Sender, Trigger * parameters);
public:
	//Actions
	static void NoAction(Scriptable * Sender, Action * parameters);
	static void SetGlobal(Scriptable * Sender, Action * parameters);
	static void SG(Scriptable * Sender, Action * parameters);
	static void TriggerActivation(Scriptable * Sender, Action * parameters);
	static void FadeToColor(Scriptable * Sender, Action * parameters);
	static void FadeFromColor(Scriptable * Sender, Action * parameters);
	static void CreateCreature(Scriptable * Sender, Action * parameters);
	static void Enemy(Scriptable * Sender, Action * parameters);
	static void Ally(Scriptable * Sender, Action * parameters);
	static void ChangeAIScript(Scriptable * Sender, Action * parameters);
	static void ChangeAllegiance(Scriptable * Sender, Action * parameters);
	static void ChangeGeneral(Scriptable * Sender, Action * parameters);
	static void ChangeRace(Scriptable * Sender, Action * parameters);
	static void ChangeClass(Scriptable * Sender, Action * parameters);
	static void ChangeSpecifics(Scriptable * Sender, Action * parameters);
	static void ChangeGender(Scriptable * Sender, Action * parameters);
	static void ChangeAlignment(Scriptable * Sender, Action * parameters);
	static void StartCutSceneMode(Scriptable * Sender, Action * parameters);
	static void EndCutSceneMode(Scriptable * Sender, Action * parameters);
	static void StartCutScene(Scriptable * Sender, Action * parameters);
	static void CutSceneID(Scriptable * Sender, Action * parameters);
	static void Wait(Scriptable * Sender, Action * parameters);
	static void SmallWait(Scriptable * Sender, Action * parameters);
	static void MoveViewPoint(Scriptable * Sender, Action * parameters);
	static void MoveViewObject(Scriptable * Sender, Action * parameters);
	static void MoveToPoint(Scriptable * Sender, Action * parameters);
	static void MoveToObject(Scriptable * Sender, Action * parameters);
	static void DisplayStringHead(Scriptable * Sender, Action * parameters);
	static void Face(Scriptable * Sender, Action * parameters);
	static void FaceObject(Scriptable * Sender, Action * parameters);
	static void DisplayStringWait(Scriptable * Sender, Action * parameters);
	static void DisplayString(Scriptable * Sender, Action * parameters);
	static void StartSong(Scriptable * Sender, Action * parameters);
	static void Continue(Scriptable * Sender, Action * parameters);
	static void PlayDead(Scriptable * Sender, Action * parameters);
	static void PlaySound(Scriptable * Sender, Action * parameters);
	static void CreateVisualEffectObject(Scriptable * Sender, Action * parameters);
	static void CreateVisualEffect(Scriptable * Sender, Action * parameters);
	static void DestroySelf(Scriptable * Sender, Action * parameters);
	static void ScreenShake(Scriptable * Sender, Action * parameters);
	static void HideGUI(Scriptable * Sender, Action * parameters);
	static void UnhideGUI(Scriptable * Sender, Action * parameters);
	static void Dialogue(Scriptable * Sender, Action * parameters);
	static void SetDialogue(Scriptable * Sender, Action * parameters);
	static void AmbientActivate(Scriptable * Sender, Action * parameters);
	static void StartDialogue(Scriptable * Sender, Action * parameters);
	static void OpenDoor(Scriptable * Sender, Action * parameters);
	static void CloseDoor(Scriptable * Sender, Action * parameters);
	static void MoveBetweenAreas(Scriptable * Sender, Action * parameters);
	static void ForceSpell(Scriptable * Sender, Action * parameters);
	static void Deactivate(Scriptable * Sender, Action * parameters);
	static void Activate(Scriptable * Sender, Action * parameters);
	static void LeaveAreaLUA(Scriptable * Sender, Action * parameters);
	static void LeaveAreaLUAPanic(Scriptable * Sender, Action * parameters);
	static void SetTokenGlobal(Scriptable * Sender, Action * parameters);
	static void GlobalSetGlobal(Scriptable * Sender, Action * parameters);
	static void GlobalAddGlobal(Scriptable * Sender, Action * parameters);
	static void IncrementGlobal(Scriptable * Sender, Action * parameters);
	static void IncrementGlobalOnce(Scriptable * Sender, Action * parameters);
	static void AddGlobals(Scriptable * Sender, Action * parameters);
	static void GlobalSubGlobal(Scriptable * Sender, Action * parameters);
	static void GlobalAndGlobal(Scriptable * Sender, Action * parameters);
	static void GlobalOrGlobal(Scriptable * Sender, Action * parameters);
	static void GlobalBAndGlobal(Scriptable * Sender, Action * parameters);
	static void GlobalBOrGlobal(Scriptable * Sender, Action * parameters);
	static void GlobalBAnd(Scriptable * Sender, Action * parameters);
	static void GlobalMaxGlobal(Scriptable * Sender, Action * parameters);
	static void GlobalMinGlobal(Scriptable * Sender, Action * parameters);
	static void GlobalMax(Scriptable * Sender, Action * parameters);
	static void GlobalMin(Scriptable * Sender, Action * parameters);
	static void GlobalBOr(Scriptable * Sender, Action * parameters);
	static void BitClear(Scriptable * Sender, Action * parameters);
	static void GlobalShLGlobal(Scriptable * Sender, Action * parameters);
	static void GlobalShRGlobal(Scriptable * Sender, Action * parameters);
	static void GlobalShL(Scriptable * Sender, Action * parameters);
	static void GlobalShR(Scriptable * Sender, Action * parameters);
	static void ClearAllActions(Scriptable * Sender, Action * parameters);
	static void ClearActions(Scriptable * Sender, Action * parameters);
	static void JoinParty(Scriptable * Sender, Action * parameters);
	static void LeaveParty(Scriptable * Sender, Action * parameters);
	static void MakeGlobal(Scriptable * Sender, Action * parameters);
	static void SetNumTimesTalkedTo(Scriptable * Sender, Action * parameters);
	static void StartMovie(Scriptable * Sender, Action * parameters);
	static void SetLeavePartyDialogFile(Scriptable * Sender, Action * parameters);
	static void DialogueForceInterrupt(Scriptable * Sender, Action * parameters);
	static void StartDialogueNoSet(Scriptable * Sender, Action * parameters);
	static void StartDialogueNoSetInterrupt(Scriptable * Sender, Action * parameters);
	static void StartDialogueInterrupt(Scriptable * Sender, Action * parameters);
	static void StartDialogueOverride(Scriptable * Sender, Action * parameters);
	static void StartDialogueOverrideInterrupt(Scriptable * Sender, Action * parameters);
	static void PlayerDialogue(Scriptable * Sender, Action * parameters);
};

#endif
