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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/GameScript.h,v 1.9 2003/12/17 20:18:03 balrog994 Exp $
 *
 */

class GameScript;

#ifndef GAMESCRIPT_H
#define GAMESCRIPT_H

#include "DataStream.h"
#include "Variables.h"
#include "ActorBlock.h"
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
	int				WobjectPosition;
	int             HobjectPosition;
	char            PositionMask[32];
	char*			objectName;
} Object;

typedef struct Trigger {
	unsigned short	triggerID;
	int				int0Parameter;
	int				flags;
	int				int1Parameter;
	int				int2Parameter;
	int				XpointParameter;
	int				YpointParameter;
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

class GameScript;

typedef int (* TriggerFunction)(GameScript*, Trigger*);
typedef void (* ActionFunction)(GameScript*, Action*);

#define IE_SCRIPT_ALWAYS		0
#define IE_SCRIPT_AREA			1
#define IE_SCRIPT_TRIGGER		2

#define MAX_TRIGGERS			0x40D5
#define MAX_ACTIONS				325

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT GameScript
{
public:
	Variables * locals;
	Script * script;
	Scriptable * MySelf;
	unsigned long scriptRunDelay;
	bool endReached;
	void Update();
private: //Internal Functions
	Script* CacheScript(DataStream * stream, const char * Context);
	void FreeScript(Script * script);
	ResponseBlock * ReadResponseBlock(DataStream * stream);
	Condition * ReadCondition(DataStream * stream);
	ResponseSet * ReadResponseSet(DataStream * stream);
	Response * ReadResponse(DataStream * stream);
	Trigger * ReadTrigger(DataStream * stream);
	Object * DecodeObject(const char * line);
	bool EvaluateCondition(GameScript * sender, Condition * condition);
	bool EvaluateTrigger(GameScript * sender, Trigger * trigger);
	void ExecuteResponseSet(GameScript * sender, ResponseSet * rS);
	void ExecuteResponse(GameScript * sender, Response * rE);
	void ExecuteAction(GameScript * sender, Action * aC);
	static Scriptable * GetActorFromObject(GameScript * Sender, Object * oC);
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
private: //Script Functions
	//Triggers
	static int  Globals(GameScript * Sender, Trigger * parameters);
	static int  OnCreation(GameScript * Sender, Trigger * parameters);
	static int	True(GameScript * Sender, Trigger * parameters);
	static int  False(GameScript * Sender, Trigger * parameters);
	static int  Alignment(GameScript * Sender, Trigger * parameters);
	static int  Allegiance(GameScript * Sender, Trigger * parameters);
	static int  Class(GameScript * Sender, Trigger * parameters);
	static int  Exists(GameScript * Sender, Trigger * parameters);
	static int  General(GameScript * Sender, Trigger * parameters);
	//Actions
	static void SetGlobal(GameScript * Sender, Action * parameters);
	static void SG(GameScript * Sender, Action * parameters);
	static void TriggerActivation(GameScript * Sender, Action * parameters);
	static void FadeToColor(GameScript * Sender, Action * parameters);
	static void FadeFromColor(GameScript * Sender, Action * parameters);
	static void CreateCreature(GameScript * Sender, Action * parameters);
	static void Enemy(GameScript * Sender, Action * parameters);
	static void Ally(GameScript * Sender, Action * parameters);
	static void ChangeAllegiance(GameScript * Sender, Action * parameters);
	static void ChangeGeneral(GameScript * Sender, Action * parameters);
	static void ChangeRace(GameScript * Sender, Action * parameters);
	static void ChangeClass(GameScript * Sender, Action * parameters);
	static void ChangeSpecifics(GameScript * Sender, Action * parameters);
	static void ChangeGender(GameScript * Sender, Action * parameters);
	static void ChangeAlignment(GameScript * Sender, Action * parameters);
	static void StartCutSceneMode(GameScript * Sender, Action * parameters);
	static void EndCutSceneMode(GameScript * Sender, Action * parameters);
	static void StartCutScene(GameScript * Sender, Action * parameters);
	static void CutSceneId(GameScript * Sender, Action * parameters);
	static void Wait(GameScript * Sender, Action * parameters);
	static void SmallWait(GameScript * Sender, Action * parameters);
	static void MoveViewPoint(GameScript * Sender, Action * parameters);
	static void MoveViewObject(GameScript * Sender, Action * parameters);
	static void MoveToPoint(GameScript * Sender, Action * parameters);
	static void MoveToObject(GameScript * Sender, Action * parameters);
	static void DisplayStringHead(GameScript * Sender, Action * parameters);
	static void Face(GameScript * Sender, Action * parameters);
	static void FaceObject(GameScript * Sender, Action * parameters);
	static void DisplayStringWait(GameScript * Sender, Action * parameters);
	static void StartSong(GameScript * Sender, Action * parameters);
	static void Continue(GameScript * Sender, Action * parameters);
	static void PlaySound(GameScript * Sender, Action * parameters);
	static void CreateVisualEffectObject(GameScript * Sender, Action * parameters);
	static void CreateVisualEffect(GameScript * Sender, Action * parameters);
	static void DestroySelf(GameScript * Sender, Action * parameters);
	static void ScreenShake(GameScript * Sender, Action * parameters);
};

#endif
