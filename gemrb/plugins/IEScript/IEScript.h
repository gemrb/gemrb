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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/IEScript/Attic/IEScript.h,v 1.4 2003/12/06 17:32:48 balrog994 Exp $
 *
 */

#ifndef IESCRIPT_H
#define IESCRIPT_H

#define MAX_TRIGGERS	0x40D5
#define MAX_ACTIONS		325

#include "../Core/Interface.h"
#include "../Core/GameScript.h"
#include "../Core/Variables.h"
#include <vector>
#include <list>

typedef struct Object {
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
	char*			objectName;
} Object;

typedef struct Trigger {
	unsigned short	triggerID;
	int				int0Parameter;
	int				flags;
	int				int1Parameter;
	int				int2Parameter;
	char*			string0Parameter;
	char*			string1Parameter;
	Object*			objectParameter;
} Trigger;

typedef struct Condition {
	unsigned short triggersCount;
	Trigger ** triggers;
} Condition;

typedef struct Action {
	unsigned short	actionID;
	Object*			objects[3];
	int				int0Parameter;
	int				XpointParameter;
	int				YpointParameter;
	int				int1Parameter;
	int				int2Parameter;
	char*			string0Parameter;
	char*			string1Parameter;
} Action;

typedef struct Response {
	unsigned char weight;
	unsigned char actionsCount;
	Action ** actions;
} Response;

typedef struct ResponseSet {
	unsigned short responsesCount;
	Response ** responses;
} ResponseSet;

typedef struct ResponseBlock {
	Condition * condition;
	ResponseSet * responseSet;
} ResponseBlock;

typedef struct Script {
	unsigned char responseBlocksCount;
	ResponseBlock ** responseBlocks;
	char * Name;
} Script;

typedef int (* TriggerFunction)(Script*, Trigger*);
typedef void (* ActionFunction)(Script*, Action*);

class IEScript : public GameScript
{
private:
	int triggersTable;
	int actionsTable;
	bool Continue;
	std::vector<Script*> scripts;
	std::list<Action*> programmedActions;
public:
	IEScript(void);
	~IEScript(void);
	int CacheScript(DataStream * stream, const char * Context);
	void SetVariable(const char * VarName, const char * Context, int value);
	void Update();
public:
	void release(void)
	{
		delete this;
	}
private: //Utility Functions
	void FreeScript(Script * script);
	ResponseBlock * ReadResponseBlock(DataStream * stream);
	Condition * ReadCondition(DataStream * stream);
	ResponseSet * ReadResponseSet(DataStream * stream);
	Response * ReadResponse(DataStream * stream);
	Trigger * ReadTrigger(DataStream * stream);
	Object * DecodeObject(const char * line);
	bool EvaluateCondition(Script * sender, Condition * condition);
	bool EvaluateTrigger(Script * sender, Trigger * trigger);
	void ExecuteResponseSet(Script * sender, ResponseSet * rS);
	void ExecuteResponse(Script * sender, Response * rE);
	void ExecuteAction(Script * sender, Action * aC);
private:
	//Triggers Array
	TriggerFunction triggers[MAX_TRIGGERS];
	//Actions Array
	ActionFunction actions[MAX_ACTIONS];
	bool blocking[MAX_ACTIONS];
	static ActorBlock * GetActorFromObject(Object * oC);
private:
	//Triggers
	static int  Globals(Script * Sender, Trigger * parameters);
	static int  OnCreation(Script * Sender, Trigger * parameters);
	static int	True(Script * Sender, Trigger * parameters);
	//Actions
	static void SetGlobal(Script * Sender, Action * parameters);
	static void TriggerActivation(Script * Sender, Action * parameters);
	static void FadeToColor(Script * Sender, Action * parameters);
	static void FadeFromColor(Script * Sender, Action * parameters);
	static void CreateCreature(Script * Sender, Action * parameters);
	static void Enemy(Script * Sender, Action * parameters);
	static void ChangeAllegiance(Script * Sender, Action * parameters);
	static void ChangeGeneral(Script * Sender, Action * parameters);
	static void ChangeRace(Script * Sender, Action * parameters);
	static void ChangeClass(Script * Sender, Action * parameters);
	static void ChangeSpecifics(Script * Sender, Action * parameters);
	static void ChangeGender(Script * Sender, Action * parameters);
	static void ChangeAlignment(Script * Sender, Action * parameters);
	static void StartCutSceneMode(Script * Sender, Action * parameters);
	static void StartCutScene(Script * Sender, Action * parameters);
	static void CutSceneId(Script * Sender, Action * parameters);
	static void Wait(Script * Sender, Action * parameters);
	static void SmallWait(Script * Sender, Action * parameters);
	static void MoveViewPoint(Script * Sender, Action * parameters);
	static void MoveToPoint(Script * Sender, Action * parameters);
	static void DisplayStringHead(Script * Sender, Action * parameters);
	static void Face(Script * Sender, Action * parameters);
	static void DisplayStringWait(Script * Sender, Action * parameters);
};

#endif
