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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/GameScript.h,v 1.18 2004/01/01 15:45:07 balrog994 Exp $
 *
 */

class GameScript;
struct Action;

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
	bool			EndReached;
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

typedef int (* TriggerFunction)(Scriptable*, Trigger*);
typedef void (* ActionFunction)(Scriptable*, Action*);

#define IE_SCRIPT_ALWAYS		0
#define IE_SCRIPT_AREA			1
#define IE_SCRIPT_TRIGGER		2

#define MAX_TRIGGERS			0xFF
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
	bool EvaluateCondition(Scriptable * Sender, Condition * condition);
	bool EvaluateTrigger(Scriptable * Sender, Trigger * trigger);
	void ExecuteResponseSet(Scriptable * Sender, ResponseSet * rS);
	void ExecuteResponse(Scriptable * Sender, Response * rE);
public:
	static void ExecuteAction(Scriptable * Sender, Action * aC);
private:
	Action * GenerateAction(char * String);
	Trigger * GenerateTrigger(char * String);
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
	void ExecuteString(char * String);
	bool EvaluateString(char * String);
private: //Script Functions
	//Triggers
	static int  Globals(Scriptable * Sender, Trigger * parameters);
	static int  OnCreation(Scriptable * Sender, Trigger * parameters);
	static int	True(Scriptable * Sender, Trigger * parameters);
	static int  False(Scriptable * Sender, Trigger * parameters);
	static int  Alignment(Scriptable * Sender, Trigger * parameters);
	static int  Allegiance(Scriptable * Sender, Trigger * parameters);
	static int  Class(Scriptable * Sender, Trigger * parameters);
	static int  Exists(Scriptable * Sender, Trigger * parameters);
	static int  General(Scriptable * Sender, Trigger * parameters);
	static int  Range(Scriptable * Sender, Trigger * parameters);
	static int  Clicked(Scriptable * Sender, Trigger * parameters);
	static int  Entered(Scriptable * Sender, Trigger * parameters);
public:
	//Actions
	static void SetGlobal(Scriptable * Sender, Action * parameters);
	static void SG(Scriptable * Sender, Action * parameters);
	static void TriggerActivation(Scriptable * Sender, Action * parameters);
	static void FadeToColor(Scriptable * Sender, Action * parameters);
	static void FadeFromColor(Scriptable * Sender, Action * parameters);
	static void CreateCreature(Scriptable * Sender, Action * parameters);
	static void Enemy(Scriptable * Sender, Action * parameters);
	static void Ally(Scriptable * Sender, Action * parameters);
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
};

#endif
