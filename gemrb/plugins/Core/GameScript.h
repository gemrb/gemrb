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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/GameScript.h,v 1.32 2004/01/17 21:33:38 balrog994 Exp $
 *
 */

class GameScript;
class Action;

#ifndef GAMESCRIPT_H
#define GAMESCRIPT_H

#include "DataStream.h"
#include "Variables.h"
#include "ActorBlock.h"
#include "SymbolMgr.h"
#include <list>

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
	};
	~Object() {
		printf("Freeing Object\n");
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
public:
	void Release() {
		if(!RefCount) {
			printf("WARNING!!! Double Freeing in %s: Line %d\n", __FILE__, __LINE__);
			abort();
		}
		RefCount--;
		if(!RefCount)
			delete this;
	}
	void IncRef() {
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
	};
	~Trigger() {
		if(objectParameter) {
			objectParameter->Release();
		}
		printf("Freeing Trigger\n");
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
public:
	void Release() {
		if(!RefCount) {
			printf("WARNING!!! Double Freeing in %s: Line %d\n", __FILE__, __LINE__);
			abort();
		}
		RefCount--;
		printf("Refcount decreased to: %d\n",RefCount);
		if(!RefCount)
			delete this;
	}
	void IncRef() {
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
		printf("Freeing Condition\n");
	}
public:
	unsigned short triggersCount;
	Trigger ** triggers;
private:
	int				RefCount;
public:
	void Release() {
		if(!RefCount) {
			printf("WARNING!!! Double Freeing in %s: Line %d\n", __FILE__, __LINE__);
			abort();
		}
		RefCount--;
		printf("Refcount decreased to: %d\n",RefCount);
		if(!RefCount)
			delete this;
	}
	void IncRef() {
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
	};
	~Action() {
		for(int c = 0; c < 3; c++) {
			if(objects[c])
				objects[c]->Release();
		}
		printf("Freeing Action\n");
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
public:
	void Release() {
		if(!RefCount) {
			printf("WARNING!!! Double Freeing in %s: Line %d\n", __FILE__, __LINE__);
			abort();
		}
		RefCount--;
		printf("Refcount decreased to: %d\n",RefCount);
		if(!RefCount)
			delete this;
	}
	void IncRef() {
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
		printf("Freeing Response\n");
	}
public:
	unsigned char weight;
	unsigned char actionsCount;
	Action ** actions;
private:
	int				RefCount;
public:
	void Release() {
		if(!RefCount) {
			printf("WARNING!!! Double Freeing in %s: Line %d\n", __FILE__, __LINE__);
			abort();
		}
		RefCount--;
		printf("Refcount decreased to: %d\n",RefCount);
		if(!RefCount)
			delete this;
	}
	void IncRef() {
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
		printf("Freeing ResponseSet\n");
	}
public:
	unsigned short responsesCount;
	Response ** responses;
private:
	int				RefCount;
public:
	void Release() {
		if(!RefCount) {
			printf("WARNING!!! Double Freeing in %s: Line %d\n", __FILE__, __LINE__);
			abort();
		}
		RefCount--;
		printf("Refcount decreased to: %d\n",RefCount);
		if(!RefCount)
			delete this;
	}
	void IncRef() {
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
	};
	~ResponseBlock() {
		if(condition)
			condition->Release();
		if(responseSet)
			responseSet->Release();
		printf("Freeing ResponseBlock\n");
	}
public:
	Condition * condition;
	ResponseSet * responseSet;
private:
	int				RefCount;
public:
	void Release() {
		if(!RefCount) {
			printf("WARNING!!! Double Freeing in %s: Line %d\n", __FILE__, __LINE__);
			abort();
		}
		RefCount--;
		printf("Refcount decreased to: %d\n",RefCount);
		if(!RefCount)
			delete this;
	}
	void IncRef() {
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
	Script(const char * Name) {
		RefCount = 1;
		responseBlocks = NULL;
		responseBlocksCount = 0;
		if(!Name) {
			this->Name[0] = 0;
			return;
		}
		strncpy(this->Name, Name, 8);
		this->Name[8] = 0;
	};
	~Script() {
		FreeResponseBlocks();
		printf("Freeing Script\n");
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
public:
	void Release() {
		if(!RefCount) {
			printf("WARNING!!! Double Freeing in %s: Line %d\n", __FILE__, __LINE__);
			abort();
		}
		RefCount--;
		printf("Refcount decreased to: %d\n",RefCount);
		if(!RefCount)
			delete this;
	}
	void IncRef() {
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

#define IE_SCRIPT_ALWAYS		0
#define IE_SCRIPT_AREA			1
#define IE_SCRIPT_TRIGGER		2

#define MAX_TRIGGERS			0xFF
#define MAX_ACTIONS				400

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
//	void FreeScript(Script * script);
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
	void SetVariable(const char * VarName, const char * Context, int value);
	static void ExecuteString(Scriptable * Sender, char * String);
	static bool EvaluateString(Scriptable * Sender, char * String);
private: //Script Functions
	//Triggers
	static int  Globals(Scriptable * Sender, Trigger * parameters);
	static int  OnCreation(Scriptable * Sender, Trigger * parameters);
	static int  PartyHasItem(Scriptable * Sender, Trigger * parameters);
	static int  True(Scriptable * Sender, Trigger * parameters);
	static int  ActionListEmpty(Scriptable * Sender, Trigger * parameters);
	static int  False(Scriptable * Sender, Trigger * parameters);
	static int  Alignment(Scriptable * Sender, Trigger * parameters);
	static int  Allegiance(Scriptable * Sender, Trigger * parameters);
	static int  Class(Scriptable * Sender, Trigger * parameters);
	static int  Exists(Scriptable * Sender, Trigger * parameters);
	static int  General(Scriptable * Sender, Trigger * parameters);
	static int  Range(Scriptable * Sender, Trigger * parameters);
	static int  Or(Scriptable * Sender, Trigger * parameters);
	static int  Clicked(Scriptable * Sender, Trigger * parameters);
	static int  Entered(Scriptable * Sender, Trigger * parameters);
	static int  Dead(Scriptable * Sender, Trigger * parameters);
private:
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
	static void CutSceneId(Scriptable * Sender, Action * parameters);
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
	static void GlobalShlGlobal(Scriptable * Sender, Action * parameters);
	static void GlobalShrGlobal(Scriptable * Sender, Action * parameters);
	static void GlobalShl(Scriptable * Sender, Action * parameters);
	static void GlobalShr(Scriptable * Sender, Action * parameters);
	static void ClearAllActions(Scriptable * Sender, Action * parameters);
	static void ClearActions(Scriptable * Sender, Action * parameters);
};

#endif
