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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/GameScript.cpp,v 1.46 2004/01/16 22:56:35 balrog994 Exp $
 *
 */

#include "GameScript.h"
#include "Interface.h"
#include "DialogMgr.h"

extern Interface * core;

static int initialized = 0;
static Variables * globals;
static SymbolMgr* triggersTable;
static SymbolMgr* actionsTable;
static TriggerFunction triggers[MAX_TRIGGERS];
static ActionFunction actions[MAX_ACTIONS];
static bool blocking[MAX_ACTIONS];
static bool instant[MAX_ACTIONS];

GameScript::GameScript(const char * ResRef, unsigned char ScriptType, Variables * local)
{
	if(local) {
		locals = local;
		freeLocals = false;
	}
	else {
		locals = new Variables();
		locals->SetType(GEM_VARIABLES_INT);
		freeLocals = true;
	}
	if(!initialized) {
		int tT = core->LoadSymbol("TRIGGER");
		int aT = core->LoadSymbol("ACTION");
		triggersTable = core->GetSymbol(tT);
		actionsTable = core->GetSymbol(aT);
		globals = new Variables();
		globals->SetType(GEM_VARIABLES_INT);
		memset(triggers, 0, MAX_TRIGGERS*sizeof(TriggerFunction));
		memset(actions, 0, MAX_ACTIONS*sizeof(ActionFunction));
		memset(blocking, 0, MAX_ACTIONS*sizeof(bool));
		triggers[0x0a] = Alignment;
		triggers[0x0b] = Allegiance;
		triggers[0x0c] = Class;
		triggers[0x0d] = Exists;
		triggers[0x0e] = General;
		triggers[0x0f] = Globals;
		triggers[0x18] = Range;
		triggers[0x23] = True;
		triggers[0x30] = False;
		triggers[0x36] = OnCreation;
		triggers[0x42] = PartyHasItem;
		triggers[0x4C] = Entered;
		triggers[0x51] = Dead;
		triggers[0x70] = Clicked;

		actions[0] = NoAction;
		actions[7] = CreateCreature;
		//instant[7] = true;
		actions[8] = Dialogue;
		blocking[8] = true;
		actions[10] = Enemy;
		instant[10] = true;
		actions[22] = MoveToObject;
		blocking[22] = true;
		actions[23] = MoveToPoint;
		blocking[23] = true;
		actions[26] = PlaySound;
		actions[30] = SetGlobal;
		//instant[30] = true;
		actions[36] = Continue;
		//instant[36] = true;
		actions[40] = PlayDead;
		actions[49] = MoveViewPoint;
		actions[50] = MoveViewObject;
		actions[60] = ChangeAIScript;
		actions[63] = Wait;
		blocking[63] = true;
		actions[83] = SmallWait;
		blocking[83] = true;
		actions[84] = Face;
		blocking[84] = true;
		actions[109] = IncrementGlobal;
		actions[110] = LeaveAreaLUA;
		actions[111] = DestroySelf;
		//instant[111] = true;
		actions[113] = ForceSpell;
		actions[120] = StartCutScene;
		//instant[120] = true;
		actions[121] = StartCutSceneMode;
		//instant[121] = true;
		actions[122] = EndCutSceneMode;
		//instant[122] = true;
		actions[123] = ClearAllActions;
		//instant[123] = true;
		actions[125] = Deactivate;
		//instant[125] = true;
		actions[126] = Activate;
		//instant[126] = true;
		actions[127] = CutSceneId;
		instant[127] = true;
		actions[133] = ClearActions;
		//instant[133] = true;
		actions[137] = StartDialogue;
		actions[143] = OpenDoor;
		blocking[143] = true;
		actions[144] = CloseDoor;
		blocking[144] = true;
		actions[151] = DisplayString;
		actions[153] = ChangeAllegiance;
		actions[154] = ChangeGeneral;
		actions[155] = ChangeRace;
		actions[156] = ChangeClass;
		actions[157] = ChangeSpecifics;
		actions[158] = ChangeGender;
		actions[159] = ChangeAlignment;
		actions[177] = TriggerActivation;
		actions[198] = Dialogue;
		if(strcmp(core->GameType, "pst") == 0) {
			actions[215] = FaceObject;
			actions[227] = GlobalBAnd;
			instant[227] = true;
			actions[228] = GlobalBOr;
			instant[228] = true;
			actions[229] = GlobalShr;
			instant[229] = true;
			actions[230] = GlobalShl;
			instant[230] = true;
			actions[231] = GlobalMax;
			instant[231] = true;
			actions[232] = GlobalMin;
			instant[232] = true;
			actions[233] = GlobalSetGlobal;
			instant[233] = true;
			actions[234] = GlobalAddGlobal;
			instant[234] = true;
			actions[235] = GlobalSubGlobal;
			instant[235] = true;
			actions[236] = GlobalAndGlobal;
			instant[236] = true;
			actions[237] = GlobalOrGlobal;
			instant[237] = true;
			actions[238] = GlobalBAndGlobal;
			instant[238] = true;
			actions[239] = GlobalBOrGlobal;
			instant[239] = true;
			actions[240] = GlobalShrGlobal;
			instant[240] = true;
			actions[241] = GlobalShlGlobal;
			instant[241] = true;
			actions[242] = GlobalMaxGlobal;
			instant[242] = true;
			actions[243] = GlobalMinGlobal;
			instant[243] = true;
			actions[244] = GlobalBOr; //BitSet
			instant[244] = true;
			actions[245] = BitClear; 
			instant[245] = true;
			actions[267] = StartSong;
			instant[267] = true;
		}
		else
		{
			actions[202] = FadeToColor;
			//blocking[202] = true;
			actions[203] = FadeFromColor;
			//blocking[203] = true;
			actions[225] = MoveBetweenAreas;
			actions[229] = FaceObject;
			//IWD and SoA are different from #231
			if(strcmp(core->GameType, "bg2") == 0) {
				actions[242] = Ally;
				actions[254] = ScreenShake;
				blocking[254] = true;
				actions[255] = AddGlobals;
				actions[269] = DisplayStringHead;
				actions[272] = CreateVisualEffect;
				actions[273] = CreateVisualEffectObject;
				actions[286] = HideGUI;
				actions[287] = UnhideGUI;
				actions[301] = AmbientActivate;
				actions[307] = SG;
				actions[311] = DisplayStringWait;
				blocking[311] = true;
				actions[335] = SetTokenGlobal;
			}
			else { //iwd and iwd2
				actions[241] = DisplayStringHead; //FloatMessage
				actions[273] = HideGUI;
				actions[274] = UnhideGUI;
				actions[297] = ScreenShake;
			}
		}
		initialized = 1;
	}
	continueExecution = false;
	DataStream * ds = core->GetResourceMgr()->GetResource(ResRef, IE_BCS_CLASS_ID);
	script = CacheScript(ds, ResRef);
	MySelf = NULL;
	scriptRunDelay = 1000;
	scriptType = ScriptType;
	lastRunTime = 0;
	endReached = false;
	strcpy(Name, ResRef);
}

GameScript::~GameScript(void)
{
	if(script) {
		printf("Releasing Script [0x%08X] in %s Line: %d\n", script, __FILE__, __LINE__);
		script->Release();
	}
	if(freeLocals) {
		if(locals)
			delete(locals);
	}
}

void GameScript::FreeScript(Script * script)
{
	printf("Releasing Script [0x%08X] in %s Line: %d\n", script, __FILE__, __LINE__);
	script->Release();
}

Script* GameScript::CacheScript(DataStream * stream, const char * Context)
{
	if(!stream)
		return NULL;
	char * line = (char*)malloc(10);
	stream->ReadLine(line, 10);
	if(strncmp(line, "SC", 2) != 0) {
		printf("[IEScript]: Not a Compiled Script file\n");
		delete(stream);
		free(line);
		return NULL;
	}
	Script * newScript = new Script(Context);
	std::vector<ResponseBlock*> rBv;
	while(true) {
		ResponseBlock * rB = ReadResponseBlock(stream);
		if(!rB)
			break;
		rBv.push_back(rB);
		stream->ReadLine(line, 10);
	}
	free(line);
	newScript->AllocateBlocks((unsigned int)rBv.size());
	for(unsigned int i = 0; i < newScript->responseBlocksCount; i++) {
		newScript->responseBlocks[i] = rBv.at(i);
	}
	delete(stream);
	return newScript;
}

void GameScript::SetVariable(const char * VarName, const char * Context, int value)
{
	char * newVarName = (char*)malloc(strlen(VarName)+strlen(Context)+1);
	strcpy(newVarName, Context);
	strcat(newVarName, VarName);
	globals->SetAt(newVarName, (unsigned long)value);
	free(newVarName);
}

void GameScript::Update()
{
	if(!MySelf || !MySelf->Active)
		return;
	unsigned long thisTime;
	GetTime(thisTime);
	if((thisTime - lastRunTime) < scriptRunDelay)
		return;
	lastRunTime = thisTime;
	if(!script)
		return;
	for(unsigned int a = 0; a < script->responseBlocksCount; a++) {
		ResponseBlock * rB = script->responseBlocks[a];
		if(EvaluateCondition(this->MySelf, rB->condition)) {
			ExecuteResponseSet(this->MySelf, rB->responseSet);
			endReached = false;
			if(!continueExecution)
				break;
			continueExecution = false;	
		}
	}
}

void GameScript::EvaluateAllBlocks()
{
	if(!MySelf || !MySelf->Active)
		return;
	unsigned long thisTime;
	GetTime(thisTime);
	if((thisTime - lastRunTime) < scriptRunDelay)
		return;
	lastRunTime = thisTime;
	if(!script)
		return;
	for(unsigned int a = 0; a < script->responseBlocksCount; a++) {
		ResponseBlock * rB = script->responseBlocks[a];
		if(EvaluateCondition(this->MySelf, rB->condition)) {
			ExecuteResponseSet(this->MySelf, rB->responseSet);
			endReached = false;
		}
	}
}

ResponseBlock * GameScript::ReadResponseBlock(DataStream * stream)
{
	char * line = (char*)malloc(10);
	stream->ReadLine(line, 10);
	if(strncmp(line, "CR", 2) != 0) {
		free(line);
		return NULL;
	}
	free(line);
	ResponseBlock * rB = new ResponseBlock();
	rB->condition = ReadCondition(stream);
	rB->responseSet = ReadResponseSet(stream);
	return rB;
}

Condition * GameScript::ReadCondition(DataStream * stream)
{
	char * line = (char*)malloc(10);
	stream->ReadLine(line, 10);
	if(strncmp(line, "CO", 2) != 0) {
		free(line);
		return NULL;
	}
	free(line);
	Condition * cO = new Condition();
	std::vector<Trigger*> tRv;
	while(true) {
		Trigger * tR = ReadTrigger(stream);
		if(!tR)
			break;
		tRv.push_back(tR);
	}
	cO->triggersCount = (unsigned short)tRv.size();
	cO->triggers = new Trigger*[cO->triggersCount];
	for(int i = 0; i < cO->triggersCount; i++) {
		cO->triggers[i] = tRv.at(i);
	}
	return cO;
}

ResponseSet * GameScript::ReadResponseSet(DataStream * stream)
{
	char * line = (char*)malloc(10);
	stream->ReadLine(line, 10);
	if(strncmp(line, "RS", 2) != 0) {
		free(line);
		return NULL;
	}
	free(line);
	ResponseSet * rS = new ResponseSet();
	std::vector<Response*> rEv;
	while(true) {
		Response * rE = ReadResponse(stream);
		if(!rE)
			break;
		rEv.push_back(rE);
	}
	rS->responsesCount = (unsigned short)rEv.size();
	rS->responses = new Response*[rS->responsesCount];
	for(int i = 0; i < rS->responsesCount; i++) {
		rS->responses[i] = rEv.at(i);
	}
	return rS;
}

Response * GameScript::ReadResponse(DataStream * stream)
{
	char * line = (char*)malloc(1024);
	stream->ReadLine(line, 1024);
	if(strncmp(line, "RE", 2) != 0) {
		free(line);
		return NULL;
	}
	Response * rE = new Response();
	rE->weight = 0;
	int count = stream->ReadLine(line, 1024);
	for(int i = 0; i < count; i++) {
		if((line[i] >= '0') && (line[i] <= '9')) {
			rE->weight *= 10;
			rE->weight += (line[i]-'0');
			continue;
		}
		break;
	}
	std::vector<Action*> aCv;
	while(true) {
		Action * aC = new Action();
		count = stream->ReadLine(line, 1024);
		for(int i = 0; i < count; i++) {
			if((line[i] >= '0') && (line[i] <= '9')) {
				aC->actionID *= 10;
				aC->actionID += (line[i]-'0');
				continue;
			}
			break;
		}
		for(int i = 0; i < 3; i++) {
			stream->ReadLine(line, 1024);
			Object * oB = DecodeObject(line);
			aC->objects[i] = oB;
			if(i != 2)
				stream->ReadLine(line, 1024);
		}
		stream->ReadLine(line, 1024);
		sscanf(line, "%d %d %d %d %d\"%[^\"]\" \"%[^\"]\" AC", &aC->int0Parameter, &aC->XpointParameter, &aC->YpointParameter, &aC->int1Parameter, &aC->int2Parameter, aC->string0Parameter, aC->string1Parameter);
		aCv.push_back(aC);
		stream->ReadLine(line, 1024);
		if(strncmp(line, "RE", 2) == 0)
			break;
	}
	free(line);
	rE->actionsCount = (unsigned char)aCv.size();
	rE->actions = new Action*[rE->actionsCount];
	for(int i = 0; i < rE->actionsCount; i++) {
		rE->actions[i] = aCv.at(i);
	}
	return rE;
}

Trigger * GameScript::ReadTrigger(DataStream * stream)
{
	char * line = (char*)malloc(1024);
	stream->ReadLine(line, 1024);
	if(strncmp(line, "TR", 2) != 0) {
		free(line);
		return NULL;
	}
	stream->ReadLine(line, 1024);
	Trigger * tR = new Trigger();
	if(strcmp(core->GameType, "pst") == 0)
		sscanf(line, "%d %d %d %d %d [%d,%d] \"%[^\"]\" \"%[^\"]\" OB", &tR->triggerID, &tR->int0Parameter, &tR->flags, &tR->int1Parameter, &tR->int2Parameter, &tR->XpointParameter, &tR->YpointParameter, tR->string0Parameter, tR->string1Parameter);
	else
		sscanf(line, "%d %d %d %d %d \"%[^\"]\" \"%[^\"]\" OB", &tR->triggerID, &tR->int0Parameter, &tR->flags, &tR->int1Parameter, &tR->int2Parameter, tR->string0Parameter, tR->string1Parameter);
	tR->triggerID &= 0xFF;
	stream->ReadLine(line, 1024);
	tR->objectParameter = DecodeObject(line);
	stream->ReadLine(line, 1024);
	free(line);
	return tR;
}

Object * GameScript::DecodeObject(const char * line)
{
	Object * oB = new Object();
	if(strcmp(core->GameType, "pst") == 0)
		sscanf(line, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d [%[^]]] \"%[^\"]\"OB", &oB->eaField, &oB->factionField, &oB->teamField, &oB->generalField, &oB->raceField, &oB->classField, &oB->specificField, &oB->genderField, &oB->alignmentField, &oB->identifiersField, &oB->XobjectPosition, &oB->YobjectPosition, &oB->WobjectPosition, &oB->HobjectPosition, oB->PositionMask, oB->objectName);
	else
		sscanf(line, "%d %d %d %d %d %d %d %d %d %d %d %d \"%[^\"]\"OB", &oB->eaField, &oB->factionField, &oB->teamField, &oB->generalField, &oB->raceField, &oB->classField, &oB->specificField, &oB->genderField, &oB->alignmentField, &oB->identifiersField, &oB->XobjectPosition, &oB->YobjectPosition, oB->objectName);
	return oB;
}

bool GameScript::EvaluateCondition(Scriptable * Sender, Condition * condition)
{
	bool ORing = false;
	int ORcount = 0;
	bool ret = true;
	for(int i = 0; i < condition->triggersCount; i++) {
		Trigger * tR = condition->triggers[i];
		ret &= EvaluateTrigger(Sender, tR);
		if(!ret)
			return ret;
	}
	return ret;
}

bool GameScript::EvaluateTrigger(Scriptable * Sender, Trigger * trigger)
{
	if(!trigger)
		return false;
	TriggerFunction func = triggers[trigger->triggerID];
	if(!func) {
		triggers[trigger->triggerID]=False;
		printf("Unhandled trigger code: %x\n",trigger->triggerID);
		return false;
	}
	int ret = func(Sender, trigger);
	if(trigger->flags&1) {
		if(ret)
			ret = 0;
		else
			ret = 1;
	}
	return ret;
}

void GameScript::ExecuteResponseSet(Scriptable * Sender, ResponseSet * rS)
{
	for(int i = 0; i < rS->responsesCount; i++) {
		Response * rE = rS->responses[i];
		int randWeight = (rand()%100)+1;
		if(rE->weight >= randWeight) {
			ExecuteResponse(Sender, rE);
			break;
		}
	}
}

void GameScript::ExecuteResponse(Scriptable * Sender, Response * rE)
{
	for(int i = 0; i < rE->actionsCount; i++) {
		Action * aC = rE->actions[i];
		if(instant[aC->actionID])
			ExecuteAction(Sender, aC);
		else {
			if(Sender->CutSceneId)
				Sender->CutSceneId->AddAction(aC);
			else
				Sender->AddAction(aC);
		}
	}
}

void GameScript::ExecuteAction(Scriptable * Sender, Action * aC)
{
	ActionFunction func = actions[aC->actionID];
	if(func) {
		printf("Executing action code; %d\n",aC->actionID);
		func(Sender, aC);
	}
	else {
		actions[aC->actionID]=NoAction;
		printf("Unhandled action code: %d\n",aC->actionID);
	}
	if(!blocking[aC->actionID]) {
		Sender->CurrentAction = NULL;	
	}
	if(instant[aC->actionID])
		return;
	printf("Releasing Action %d [0x%08X] in %s Line: %d\n", aC->actionID, aC, __FILE__, __LINE__);
	aC->Release();
}

Action* GameScript::CreateAction(char *string, bool autoFree)
{
	Action * aC = GenerateAction(string);
	return aC;
}

Scriptable * GameScript::GetActorFromObject(Scriptable * Sender, Object * oC)
{
	//TODO: Implement Object Retieval
	if(oC) {
		if(oC->objectName[0] != 0) {
			//printf("ActionOverride on %s\n", oC->objectName);
			Map * map = core->GetGame()->GetMap(0);
			return map->GetActor(oC->objectName);
		}
		else {
			if(oC->eaField != 0) {
				switch(oC->eaField) {
					case 2:
						return core->GetGame()->GetPC(0);
					break;
				}
			}
			else if(oC->genderField != 0) {
				switch(oC->genderField) {
					case 1:
						if(Sender->CutSceneId)
							return Sender->CutSceneId;
						return Sender;
					break;

					case 21:
						return core->GetGame()->GetPC(0);
					break;
					
					case 17:
						return Sender->LastTrigger;
					break;
				}
				return NULL;
			}
		}
	}
	if(Sender->CutSceneId)
		return Sender->CutSceneId;
	return Sender;
}

unsigned char GameScript::GetOrient(short sX, short sY, short dX, short dY)
{
	short deltaX = (dX-sX), deltaY = (dY-sY);
	if(deltaX > 0) {
		if(deltaY > 0) {
			return 6;
		} else if(deltaY == 0) {
			return 4;
		} else {
			return 2;
		}
	}
	else if(deltaX == 0) {
		if(deltaY > 0) {
			return 8;
		} else {
			return 0;
		}
	}
	else {
		if(deltaY > 0) {
			return 10;
		} else if(deltaY == 0) {
			return 12;
		} else {
			return 14;
		}
	}
	return 0;
}

void GameScript::ExecuteString(Scriptable * Sender, char * String)
{
	if(String[0] == 0)
		return;
	Action * act = GenerateAction(String);
	if(!act)
		return;
	if(Sender->CurrentAction) {
		printf("Releasing Action %d [0x%08X] in %s Line: %d\n", Sender->CurrentAction->actionID, Sender->CurrentAction, __FILE__, __LINE__);
		Sender->CurrentAction->Release();
	}
	Sender->CurrentAction = act;
	ExecuteAction(Sender, act);
	return;
}

bool GameScript::EvaluateString(Scriptable * Sender, char * String)
{
	if(String[0] == 0)
		return false;
	Trigger * tri = GenerateTrigger(String);
	bool ret = EvaluateTrigger(Sender, tri);
	printf("Releasing Trigger %d [0x%08X] in %s Line: %d\n", tri->triggerID, tri, __FILE__, __LINE__);
	tri->Release();
	return ret;
}

Action * GameScript::GenerateAction(char * String)
{
	Action * newAction = NULL;
	int i = 0;
	while(true) {
		char * src = String;
		char * str = actionsTable->GetStringIndex(i);
		if(!str)
			return newAction;
		while(*str) {
			if(*str != *src)
				break;
			if(*str == '(') {
				newAction = new Action();
				newAction->actionID = (unsigned short)actionsTable->GetValueIndex(i);
				int objectCount = (newAction->actionID == 1) ? 0 : 1;
				int stringsCount = 0;
				int intCount = 0;
				//Here is the Trigger; Now we need to evaluate the parameters
				str++;
				src++;
				while(*str) {
					switch(*str) {
						default:
							str++;
						break;

						case 'P': //Point
							{
							while((*str != ',') && (*str != ')')) str++;
							src++; //Skip [
							char * symbol = (char*)malloc(32);
							char * tmp = symbol;
							while(((*src >= '0') && (*src <= '9')) || (*src == '-')) {
								*tmp = *src;
								tmp++;
								src++;
							}
							*tmp = 0;
							newAction->XpointParameter = atoi(symbol);
							src++; //Skip .
							tmp = symbol;
							while(((*src >= '0') && (*src <= '9')) || (*src == '-')) {
								*tmp = *src;
								tmp++;
								src++;
							}
							*tmp = 0;
							newAction->YpointParameter = atoi(symbol);
							free(symbol);
							src++; //Skip ]
							}
						break;

						case 'I': //Integer
							{
								while(*str != '*') str++;
								str++;
								SymbolMgr * valHook = NULL;
								if((*str != ',') && (*str != ')')) {
									char * idsTabName = (char*)malloc(32);
									char * tmp = idsTabName;
									while((*str != ',') && (*str != ')')) {
										*tmp = *str;
										tmp++;
										str++;
									}
									*tmp = 0;
									int i = core->LoadSymbol(idsTabName);
									valHook = core->GetSymbol(i);
									free(idsTabName);
								}
								if(!valHook) {
									char * symbol = (char*)malloc(32);
									char * tmp = symbol;
									while(((*src >= '0') && (*src <= '9')) || (*src == '-')) {
										*tmp = *src;
										tmp++;
										src++;
									}
									*tmp = 0;
									if(!intCount) {
										newAction->int0Parameter = atoi(symbol);
									} else if(intCount == 1) {
										newAction->int1Parameter = atoi(symbol);
									} else {
										newAction->int2Parameter = atoi(symbol);
									}
									free(symbol);
								} else {
									char * symbol = (char*)malloc(32);
									char * tmp = symbol;
									while((*src != ',') && (*src != ')')) {
										*tmp = *src;
										tmp++;
										src++;
									}
									*tmp = 0;
									if(!intCount) {
										newAction->int0Parameter = valHook->GetValue(symbol);
									} else if(intCount == 1) {
										newAction->int1Parameter = valHook->GetValue(symbol);
									} else {
										newAction->int2Parameter = valHook->GetValue(symbol);
									}
									free(symbol);
								}
							}
						break;

						case 'A': //Action
							{
								while((*str != ',') && (*str != ')')) str++;
								char *action = (char*)malloc(128);
								char *dst = action;
								int openParentesisCount = 0;
								while(true) {
									if(*src == ')') {
										if(!openParentesisCount)
											break;
										openParentesisCount--;
									} else {
										if(*src == '(') {
											openParentesisCount++;
										}
										else {
											if((*src == ',') && !openParentesisCount)
												break;
										}
									}
									*dst = *src;
									dst++;
									src++;
								}
								*dst = 0;
								Action * act = GenerateAction(action);
								free(action);
								act->objects[0] = newAction->objects[0];
								act->objects[0]->IncRef();
								printf("Releasing Action %d [0x%08X] in %s Line: %d\n", newAction->actionID, newAction, __FILE__, __LINE__);
								newAction->Release();
								newAction = act;
							}
						break;

						case 'O': //Object
							{
								while((*str != ',') && (*str != ')')) str++;
								if(*src == '"') { //Object Name
									src++;
									newAction->objects[objectCount] = new Object();
									char *dst = newAction->objects[objectCount]->objectName;
									while(*src != '"') {
										*dst = *src;
										dst++;
										src++;
									}
									*dst = 0;
									src++;
								} else {
									
								}
								objectCount++;
							}
						break;

						case 'S': //String
							{
								while((*str != ',') && (*str != ')')) str++;
								src++;
								char * dst;
								if(!stringsCount) {
									//newAction->string0Parameter = (char*)malloc(128);
									dst = newAction->string0Parameter;
								} else {
									//newAction->string1Parameter = (char*)malloc(128);
									dst = newAction->string1Parameter;
								}
								while(*src != '"') {
									*dst = *src;
									dst++;
									src++;
								}
								*dst = 0;
								src++;
								stringsCount++;
							}
						break;
					}
					while((*src == ',') || (*src == ' '))
						src++;
				}
				return newAction;
			}
			src++;
			str++;
		}
		i++;
	}
	return newAction;
}

Trigger * GameScript::GenerateTrigger(char * String)
{
	Trigger * newTrigger = NULL;
	bool negate = false;
	if(*String == '!') {
		String++;
		negate = true;
	}
	int i = 0;
	while(true) {
		char * src = String;
		char * str = triggersTable->GetStringIndex(i);
		if(!str)
			return newTrigger;
		while(*str) {
			if(*str != *src)
				break;
			if(*str == '(') {
				newTrigger = new Trigger();
				newTrigger->triggerID = (unsigned short)(triggersTable->GetValueIndex(i)&0xff);
				newTrigger->flags = (negate) ? (unsigned short)1 : (unsigned short)0;
				int stringsCount = 0;
				int intCount = 0;
				//Here is the Trigger; Now we need to evaluate the parameters
				str++;
				src++;
				while(*str) {
					switch(*str) {
						default:
							str++;
						break;

						case 'P': //Point
							{
							while((*str != ',') && (*str != ')')) str++;
							src++; //Skip [
							char * symbol = (char*)malloc(32);
							char * tmp = symbol;
							while((*src >= '0') && (*src <= '9')) {
								*tmp = *src;
								tmp++;
								src++;
							}
							*tmp = 0;
							newTrigger->XpointParameter = atoi(symbol);
							src++; //Skip .
							tmp = symbol;
							while((*src >= '0') && (*src <= '9')) {
								*tmp = *src;
								tmp++;
								src++;
							}
							*tmp = 0;
							newTrigger->YpointParameter = atoi(symbol);
							src++; //Skip ]
							}
						break;

						case 'I': //Integer
							{
								while(*str != '*') str++;
								str++;
								SymbolMgr * valHook = NULL;
								if((*str != ',') && (*str != ')')) {
									char * idsTabName = (char*)malloc(32);
									char * tmp = idsTabName;
									while((*str != ',') && (*str != ')')) {
										*tmp = *str;
										tmp++;
										str++;
									}
									*tmp = 0;
									int i = core->LoadSymbol(idsTabName);
									valHook = core->GetSymbol(i);
									free(idsTabName);
								}
								if(!valHook) {
									char * symbol = (char*)malloc(32);
									char * tmp = symbol;
									while((*src >= '0') && (*src <= '9')) {
										*tmp = *src;
										tmp++;
										src++;
									}
									*tmp = 0;
									if(!intCount) {
										newTrigger->int0Parameter = atoi(symbol);
									} else if(intCount == 1) {
										newTrigger->int1Parameter = atoi(symbol);
									} else {
										newTrigger->int2Parameter = atoi(symbol);
									}
									free(symbol);
								} else {
									char * symbol = (char*)malloc(32);
									char * tmp = symbol;
									while((*src != ',') && (*src != ')')) {
										*tmp = *src;
										tmp++;
										src++;
									}
									*tmp = 0;
									if(!intCount) {
										newTrigger->int0Parameter = valHook->GetValue(symbol);
									} else if(intCount == 1) {
										newTrigger->int1Parameter = valHook->GetValue(symbol);
									} else {
										newTrigger->int2Parameter = valHook->GetValue(symbol);
									}
									free(symbol);
								}
								intCount++;
							}
						break;

						case 'O': //Object
							{
								while((*str != ',') && (*str != ')')) str++;
								if(*src == '"') { //Object Name
									src++;
									newTrigger->objectParameter = new Object();
									char *dst = newTrigger->objectParameter->objectName;
									while(*src != '"') {
										*dst = *src;
										dst++;
										src++;
									}
									*dst = 0;
									src++;
								} else {
									
								}
							}
						break;

						case 'S': //String
							{
								while((*str != ',') && (*str != ')')) str++;
								src++;
								char * dst;
								if(!stringsCount) {
									dst = newTrigger->string0Parameter;
								} else {
									dst = newTrigger->string1Parameter;
								}
								while(*src != '"') {
									*dst = *src;
									dst++;
									src++;
								}
								src++;
								stringsCount++;
							}
						break;
					}
					while((*src == ',') || (*src == ' '))
						src++;
				}
				return newTrigger;
			}
			src++;
			str++;
		}
		i++;
	}
	return newTrigger;
}

//-------------------------------------------------------------
// Trigger Functions
//-------------------------------------------------------------

int GameScript::Alignment(Scriptable * Sender, Trigger * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objectParameter);
	if(!scr)
		return 0;
	if(scr->Type != ST_ACTOR)
		return 0;
	Actor * actor = (Actor*)scr;
	int value = actor->GetStat(IE_ALIGNMENT);
	int a = parameters->int0Parameter&15;
	if(a) {
		if(a!=(value&15)) return 0;
	}

	a = parameters->int0Parameter&240;
	if(a) {
		if(a!=(value&240)) return 0;
	}
	return 1;
}

int GameScript::Allegiance(Scriptable * Sender, Trigger * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objectParameter);
	if(!scr)
		return 0;
	if(scr->Type != ST_ACTOR)
		return 0;
	Actor * actor = (Actor*)scr;
	int value = actor->GetStat(IE_EA);
	switch(parameters->int0Parameter)
	{
	case 30: //goodcutoff
		return value<=30;
	case 31: //notgood
		return value>=31;
	case 199: //notevil
		return value<=199;
	case 200: //evilcutoff
		return value>=200;
	case 0: case 126: //anything
		return true;
	}
	return parameters->int0Parameter==value;
}

int GameScript::Class(Scriptable * Sender, Trigger * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objectParameter);
	if(!scr)
		return 0;
	if(scr->Type != ST_ACTOR)
		return 0;
	Actor * actor = (Actor*)scr;
	//TODO: if parameter >=202, it is of *_ALL type
	int value = actor->GetStat(IE_CLASS);
	return parameters->int0Parameter==value;
}

int GameScript::Exists(Scriptable * Sender, Trigger * parameters)
{
	Scriptable * actor = GetActorFromObject(Sender, parameters->objectParameter);
	if(actor==NULL) return 0;
	return 1;
}

int GameScript::General(Scriptable * Sender, Trigger * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objectParameter);
	if(!scr)
		return 0;
	if(scr->Type != ST_ACTOR)
		return 0;
	Actor * actor = (Actor*)scr;
	if(actor==NULL) return 0;
	return parameters->int0Parameter==actor->GetStat(IE_GENERAL);
}

int GameScript::Globals(Scriptable * Sender, Trigger * parameters)
{
	unsigned long value=0;
	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0) {
		globals->Lookup(&parameters->string0Parameter[6], value);
	}
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0) {
		Sender->locals->Lookup(&parameters->string0Parameter[6], value);
	}
	else {
		globals->Lookup(parameters->string0Parameter, value);
	}
	int eval = (value == parameters->int0Parameter) ? 1 : 0;
	return eval;
}

int  GameScript::OnCreation(Scriptable * Sender, Trigger * parameters)
{
	Map * area = core->GetGame()->GetMap(0);
	if(area->justCreated) {
		area->justCreated = false;
		return 1;
	}
	return 0;
}

int GameScript::PartyHasItem(Scriptable * /*Sender*/, Trigger *parameters)
{
/*hacked to never have the item, this requires inventory!*/
	if(stricmp(parameters->string0Parameter, "MISC4G") == 0)
		return 1;
	return 0;
}

int GameScript::True(Scriptable * /* Sender*/, Trigger * /*parameters*/)
{
	return 1;
}

int GameScript::False(Scriptable * /*Sender*/, Trigger * /*parameters*/)
{
	return 0;
}

int GameScript::Range(Scriptable * Sender, Trigger * parameters)
{
	Scriptable * target = GetActorFromObject(Sender, parameters->objectParameter);
	if(!target)
		return 0;
	printf("x1 = %d, y1 = %d\nx2 = %d, y2 = %d\n", target->XPos, target->YPos, Sender->XPos, Sender->YPos);
	long x = (target->XPos - Sender->XPos);
	long y = (target->YPos - Sender->YPos);
	double distance = sqrt((double)(x*x+y*y));
	printf("Distance = %.3f\n", distance);
	if(distance <= (parameters->int0Parameter*20)) {
		return 1;
	}
	return 0;
}

int GameScript::Clicked(Scriptable * Sender, Trigger * parameters)
{
	if(parameters->objectParameter->eaField == 0) {
		return 1;
	}
	Scriptable * target = GetActorFromObject(Sender, parameters->objectParameter);
	if(Sender == target)
		return 1;
	return 0;
}

int GameScript::Entered(Scriptable * Sender, Trigger * parameters)
{
	if(Sender->Type != ST_PROXIMITY)
		return 0;
	if(parameters->objectParameter->eaField == 0) {
		if(Sender->LastEntered)
			return 1;
		else
			return 0;
	}
	Scriptable * target = GetActorFromObject(Sender, parameters->objectParameter);
	if(Sender->LastEntered == target)
		return 1;
	return 0;
}

int GameScript::Dead(Scriptable * Sender, Trigger * parameters)
{
	return 0;
}

//-------------------------------------------------------------
// Action Functions
//-------------------------------------------------------------

void GameScript::NoAction(Scriptable */*Sender*/, Action */*parameters*/)
{
	//thats all :)
}

void GameScript::SG(Scriptable * Sender, Action * parameters)
{
	globals->SetAt(&parameters->string0Parameter[0], parameters->int0Parameter);
}

void GameScript::SetGlobal(Scriptable * Sender, Action * parameters)
{
	printf("SetGlobal(\"%s\", %d)\n", parameters->string0Parameter, parameters->int0Parameter);
	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0)
		globals->SetAt(&parameters->string0Parameter[6], parameters->int0Parameter);
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0)
		Sender->locals->SetAt(&parameters->string0Parameter[6], parameters->int0Parameter);
	else {
		globals->SetAt(parameters->string0Parameter, parameters->int0Parameter);
	}
}

void GameScript::ChangeAllegiance(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[1]);
	if(!scr)
		return;
	if(scr->Type != ST_ACTOR)
		return;
	Actor * actor = (Actor*)scr;
	actor->SetStat(IE_EA,parameters->int0Parameter);
}

void GameScript::ChangeGeneral(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[1]);
	if(!scr)
		return;
	if(scr->Type != ST_ACTOR)
		return;
	Actor * actor = (Actor*)scr;
	actor->SetStat(IE_GENERAL,parameters->int0Parameter);
}

void GameScript::ChangeRace(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[1]);
	if(!scr)
		return;
	if(scr->Type != ST_ACTOR)
		return;
	Actor * actor = (Actor*)scr;
	actor->SetStat(IE_RACE,parameters->int0Parameter);
}

void GameScript::ChangeClass(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[1]);
	if(!scr)
		return;
	if(scr->Type != ST_ACTOR)
		return;
	Actor * actor = (Actor*)scr;
	actor->SetStat(IE_CLASS,parameters->int0Parameter);
}

void GameScript::ChangeSpecifics(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[1]);
	if(!scr)
		return;
	if(scr->Type != ST_ACTOR)
		return;
	Actor * actor = (Actor*)scr;
	actor->SetStat(IE_SPECIFIC,parameters->int0Parameter);
}

void GameScript::ChangeGender(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[1]);
	if(!scr)
		return;
	if(scr->Type != ST_ACTOR)
		return;
	Actor * actor = (Actor*)scr;
	actor->SetStat(IE_SEX,parameters->int0Parameter);
}

void GameScript::ChangeAlignment(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[1]);
	if(!scr)
		return;
	if(scr->Type != ST_ACTOR)
		return;
	Actor * actor = (Actor*)scr;
	actor->SetStat(IE_ALIGNMENT,parameters->int0Parameter);
}

void GameScript::TriggerActivation(Scriptable * Sender, Action * parameters)
{
	Scriptable * ip;
	if(parameters->objects[1]->genderField != 0) {
		switch(parameters->objects[1]->genderField) {
			case 1:
				ip = Sender;
			break;
		}
	}
	else
		ip = core->GetGame()->GetMap(0)->tm->GetInfoPoint(parameters->objects[1]->objectName);
	if(!ip) {
		printf("Script error: No Trigger Named \"%s\"\n", parameters->objects[1]->objectName);
		return;
	}
	ip->Active = parameters->int0Parameter;
}

void GameScript::FadeToColor(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
	if(scr != Sender) { //this is an Action Override
		scr->AddAction(Sender->CurrentAction);
		return;
	}
	core->timer->SetFadeToColor(parameters->XpointParameter);
	//Sender->SetWait(parameters->XpointParameter);
}

void GameScript::FadeFromColor(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
	if(scr != Sender) { //this is an Action Override
		scr->AddAction(Sender->CurrentAction);
		return;
	}
	core->timer->SetFadeFromColor(parameters->XpointParameter);
	//Sender->SetWait(parameters->XpointParameter);
}

void GameScript::CreateCreature(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
	if(scr != Sender) { //this is an Action Override
		scr->AddAction(Sender->CurrentAction);
		return;
	}
	ActorMgr * aM = (ActorMgr*)core->GetInterface(IE_CRE_CLASS_ID);
	DataStream * ds = core->GetResourceMgr()->GetResource(parameters->string0Parameter, IE_CRE_CLASS_ID);
	aM->Open(ds, true);
	Actor *ab = aM->GetActor();
	ab->MoveTo(parameters->XpointParameter, parameters->YpointParameter);
	ab->AnimID = IE_ANI_AWAKE;
	ab->Orientation = parameters->int0Parameter;
	Map * map = core->GetGame()->GetMap(0);
	map->AddActor(ab);
	core->FreeInterface(aM);
}

void GameScript::StartCutSceneMode(Scriptable * Sender, Action * parameters)
{
	core->SetCutSceneMode(true);
}

void GameScript::EndCutSceneMode(Scriptable * Sender, Action * parameters)
{
	core->SetCutSceneMode(false);
}

void GameScript::StartCutScene(Scriptable * Sender, Action * parameters)
{
	GameScript * gs = new GameScript(parameters->string0Parameter, IE_SCRIPT_ALWAYS);
	gs->MySelf = Sender;
	gs->EvaluateAllBlocks();
	delete(gs);
}

void GameScript::CutSceneId(Scriptable * Sender, Action * parameters)
{
	if(parameters->objects[1]->genderField != 0) {
		Sender->CutSceneId = GetActorFromObject(Sender, parameters->objects[1]);
	} else {
		Map * map = core->GetGame()->GetMap(0);
		Sender->CutSceneId = map->GetActor(parameters->objects[1]->objectName);
	}
}

void GameScript::Enemy(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
	if(scr->Type != ST_ACTOR)
		return;
	if(scr != Sender) { //this is an Action Override
		scr->AddAction(Sender->CurrentAction);
		Sender->CurrentAction = NULL;
		return;
	}
	Actor * actor = (Actor*)scr;
	actor->SetStat(IE_EA,255);
}

void GameScript::Ally(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
	if(scr->Type != ST_ACTOR)
		return;
	if(scr != Sender) { //this is an Action Override
		scr->AddAction(Sender->CurrentAction);
		return;
	}
	Actor * actor = (Actor*)scr;
	actor->SetStat(IE_EA,4);
}

void GameScript::ChangeAIScript(Scriptable * Sender, Action *parameters)
{
printf("ChangeAIScript\n");
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
printf("A\n");
	if(scr->Type != ST_ACTOR)
		return;
printf("B\n");
	Actor * actor = (Actor*)scr;
printf("E\n");
	actor->SetScript(parameters->string0Parameter, parameters->int0Parameter);
}

void GameScript::Wait(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
	if(scr != Sender) { //this is an Action Override
		scr->AddAction(Sender->CurrentAction);
		/* 
			We need to NULL the CurrentAction because this is a blocking
			OpCode.
		*/
		Sender->CurrentAction = NULL;
		return;
	}
	Sender->SetWait(parameters->int0Parameter*AI_UPDATE_TIME);
}

void GameScript::SmallWait(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
	if(scr != Sender) { //this is an Action Override
		scr->AddAction(Sender->CurrentAction);
		/* 
			We need to NULL the CurrentAction because this is a blocking
			OpCode.
		*/
		Sender->CurrentAction = NULL;
		return;
	}
	Sender->SetWait(parameters->int0Parameter);
}

void GameScript::MoveViewPoint(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
	if(scr != Sender) { //this is an Action Override
		scr->AddAction(Sender->CurrentAction);
		return;
	}
	core->GetVideoDriver()->MoveViewportTo(parameters->XpointParameter, parameters->YpointParameter);
}

void GameScript::MoveViewObject(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[1]);
	if(!scr)
		return;
	core->GetVideoDriver()->MoveViewportTo(scr->XPos, scr->YPos);
}

void GameScript::MoveToPoint(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
	if(scr->Type != ST_ACTOR)
		return;
	if(scr != Sender) { //this is an Action Override
		scr->AddAction(Sender->CurrentAction);
		/* 
			We need to NULL the CurrentAction because this is a blocking
			OpCode.
		*/
		Sender->CurrentAction = NULL;
		return;
	}
	Actor * actor = (Actor*)scr;
	actor->WalkTo(parameters->XpointParameter, parameters->YpointParameter);
	//core->timer->SetMovingActor(actor);
}

void GameScript::MoveToObject(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
	Scriptable * target = GetActorFromObject(Sender, parameters->objects[1]);
	if(!target)
		return;
	if(scr->Type != ST_ACTOR)
		return;
	if(scr != Sender) { //this is an Action Override
		scr->AddAction(Sender->CurrentAction);
		/* 
			We need to NULL the CurrentAction because this is a blocking
			OpCode.
		*/
		Sender->CurrentAction = NULL;
		return;
	}
	Actor * actor = (Actor*)scr;
	actor->WalkTo(target->XPos, target->YPos);
	//core->timer->SetMovingActor(actor);
}

void GameScript::DisplayStringHead(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
	if(scr->Type != ST_ACTOR)
		return;
	if(scr != Sender) { //this is an Action Override
		scr->AddAction(Sender->CurrentAction);
		return;
	}
	Actor * actor = (Actor*)scr;
	if(actor) {
		printf("Displaying string on: %s\n", actor->scriptName);
		actor->DisplayHeadText(core->GetString(parameters->int0Parameter,2));
	}
}

void GameScript::Face(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
	if(scr->Type != ST_ACTOR)
		return;
	if(scr != Sender) { //this is an Action Override
		scr->AddAction(Sender->CurrentAction);
		/* 
			We need to NULL the CurrentAction because this is a blocking
			OpCode.
		*/
		Sender->CurrentAction = NULL;
		return;
	}
	Actor * actor = (Actor*)scr;
	if(actor) {
		actor->Orientation = parameters->int0Parameter;
		actor->resetAction = true;
		actor->SetWait(1);
	} else {
		/* 
			This action is a fast Ending OpCode. This means that this OpCode
			is executed and finished immediately, but since we need to 
			redraw the Screen to see the change, we consider it as a Blocking
			Action. We need to NULL the CurrentAction to prevent an infinite loop
			waiting for this 'blocking' action to terminate.
		*/
		Sender->CurrentAction = NULL;
	}
}

void GameScript::FaceObject(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
	if(scr != Sender) { //this is an Action Override
		scr->AddAction(Sender->CurrentAction);
		return;
	}
	Scriptable * target = GetActorFromObject(Sender, parameters->objects[1]);
	if(!target)
		return;
	if(scr->Type != ST_ACTOR)
		return;
	Actor * actor = (Actor*)scr;
	actor->Orientation = GetOrient(target->XPos, target->YPos, actor->XPos, actor->YPos);
}

void GameScript::DisplayStringWait(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
	if(scr->Type != ST_ACTOR)
		return;
	if(scr != Sender) { //this is an Action Override
		scr->AddAction(Sender->CurrentAction);
		/* 
			We need to NULL the CurrentAction because this is a blocking
			OpCode.
		*/
		Sender->CurrentAction = NULL;
		return;
	}
	Actor * actor = (Actor*)scr;
	printf("Displaying string on: %s\n", actor->scriptName);
	StringBlock sb = core->strings->GetStringBlock(parameters->int0Parameter);
	actor->DisplayHeadText(sb.text);
	if(sb.Sound[0]) {
		unsigned long len = core->GetSoundMgr()->Play(sb.Sound);
		if(len != 0)
			actor->SetWait((AI_UPDATE_TIME*len)/1000);
	}
}

void GameScript::StartSong(Scriptable * Sender, Action * parameters)
{
	int MusicTable = core->LoadTable("music");
	if(MusicTable>=0) {
		TableMgr * music = core->GetTable(MusicTable);
		char * string = music->QueryField(parameters->int0Parameter, 0);
		if(string[0] == '*') {
			core->GetMusicMgr()->HardEnd();
		}
		else {
			core->GetMusicMgr()->SwitchPlayList(string, true);
		}
	}
}

void GameScript::Continue(Scriptable * Sender, Action * parameters)
{
	//Sender->continueExecution = true;
}

void GameScript::PlaySound(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
	if(scr != Sender) { //this is an Action Override
		scr->AddAction(Sender->CurrentAction);
		return;
	}
	printf("PlaySound(%s)\n", parameters->string0Parameter);
	core->GetSoundMgr()->Play(parameters->string0Parameter, scr->XPos, scr->YPos);
}

void GameScript::CreateVisualEffectObject(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
	if(scr != Sender) { //this is an Action Override
		scr->AddAction(Sender->CurrentAction);
		return;
	}
	Scriptable * tar = GetActorFromObject(Sender, parameters->objects[1]);
	if(!tar)
		return;
	DataStream * ds = core->GetResourceMgr()->GetResource(parameters->string0Parameter, IE_VVC_CLASS_ID);
	ScriptedAnimation * vvc = new ScriptedAnimation(ds, true, tar->XPos, tar->YPos);
	core->GetGame()->GetMap(0)->AddVVCCell(vvc);
}

void GameScript::CreateVisualEffect(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
	if(scr != Sender) { //this is an Action Override
		scr->AddAction(Sender->CurrentAction);
		return;
	}
	DataStream * ds = core->GetResourceMgr()->GetResource(parameters->string0Parameter, IE_VVC_CLASS_ID);
	ScriptedAnimation * vvc = new ScriptedAnimation(ds, true, parameters->XpointParameter, parameters->YpointParameter);
	core->GetGame()->GetMap(0)->AddVVCCell(vvc);
}

void GameScript::DestroySelf(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
	if(scr->Type != ST_ACTOR)
		return;
	if(scr != Sender) { //this is an Action Override
		scr->AddAction(Sender->CurrentAction);
		return;
	}
	Actor * actor = (Actor*)scr;
	actor->DeleteMe = true;
}

void GameScript::ScreenShake(Scriptable * Sender, Action * parameters)
{
	core->timer->SetScreenShake(parameters->XpointParameter, parameters->YpointParameter, parameters->int0Parameter);
	Sender->SetWait(parameters->int0Parameter);
}

void GameScript::UnhideGUI(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
	if(scr != Sender) { //this is an Action Override
		scr->AddAction(Sender->CurrentAction);
		return;
	}
	GameControl * gc = (GameControl*)core->GetWindow(0)->GetControl(0);	
	if(gc->ControlType == IE_GUI_GAMECONTROL)
		gc->UnhideGUI();
	EndCutSceneMode(Sender, parameters);
}

void GameScript::HideGUI(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
	if(scr != Sender) { //this is an Action Override
		scr->AddAction(Sender->CurrentAction);
		return;
	}
	GameControl * gc = (GameControl*)core->GetWindow(0)->GetControl(0);	
	if(gc->ControlType == IE_GUI_GAMECONTROL)
		gc->HideGUI();
}

void GameScript::Dialogue(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
	if(scr != Sender) { //this is an Action Override
		scr->AddAction(Sender->CurrentAction);
		Sender->CurrentAction = NULL;
		return;
	}
	Scriptable * tar = GetActorFromObject(Sender, parameters->objects[1]);
	if(!tar)
		return;
	if(scr->Type != ST_ACTOR)
		return;
	Actor * actor = (Actor*)scr;
	Actor * target = (Actor*)tar;
	if(actor->Dialog[0] != 0) {
		DialogMgr * dm = (DialogMgr*)core->GetInterface(IE_DLG_CLASS_ID);
		dm->Open(core->GetResourceMgr()->GetResource(actor->Dialog, IE_DLG_CLASS_ID), true);
		GameControl * gc = (GameControl*)core->GetWindow(0)->GetControl(0);	
		if(gc->ControlType == IE_GUI_GAMECONTROL)
			gc->InitDialog(actor, target, dm->GetDialog());
		core->FreeInterface(dm);
	}
}

void GameScript::DisplayString(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[1]);
	if(!scr)
		return;
	if(scr->overHeadText)
		free(scr->overHeadText);
	scr->overHeadText = core->GetString(parameters->int0Parameter);
	GetTime(scr->timeStartDisplaying);
	scr->textDisplaying = 0;
	GameControl * gc = (GameControl*)core->GetWindow(0)->GetControl(0);	
	if(gc->ControlType == IE_GUI_GAMECONTROL)
		gc->DisplayString(scr);
}

void GameScript::AmbientActivate(Scriptable * Sender, Action * parameters)
{
	Animation * anim = core->GetGame()->GetMap(0)->GetAnimation(parameters->objects[1]->objectName);
	if(!anim) {
		printf("Script error: No Animation Named \"%s\"\n", parameters->objects[1]->objectName);
		return;
	}
	anim->Active = parameters->int0Parameter;
}

void GameScript::StartDialogue(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
	if(scr != Sender) { //this is an Action Override
		scr->AddAction(Sender->CurrentAction);
		return;
	}
	Scriptable * tar = GetActorFromObject(Sender, parameters->objects[1]);
	if(!tar)
		return;
	if(scr->Type != ST_ACTOR)
		return;
	Actor * actor = (Actor*)scr;
	Actor * target = (Actor*)tar;
	if(parameters->string0Parameter[0] != 0) {
		DialogMgr * dm = (DialogMgr*)core->GetInterface(IE_DLG_CLASS_ID);
		dm->Open(core->GetResourceMgr()->GetResource(parameters->string0Parameter, IE_DLG_CLASS_ID), true);
		GameControl * gc = (GameControl*)core->GetWindow(0)->GetControl(0);	
		if(gc->ControlType == IE_GUI_GAMECONTROL)
			gc->InitDialog(actor, target, dm->GetDialog());
		core->FreeInterface(dm);
	}
}

Point* FindNearPoint(Actor* Sender, Point *p1, Point *p2, double &distance)
{
	long x1 = (Sender->XPos - p1->x);
	long y1 = (Sender->YPos - p1->y);
	double distance1 = sqrt((double)(x1*x1+y1*y1));
	long x2 = (Sender->XPos - p2->x);
	long y2 = (Sender->YPos - p2->y);
	double distance2 = sqrt((double)(x2*x2+y2*y2));
	if(distance1 < distance2) {
		distance = distance1;
		return p1;
	}
	else {
		distance = distance2;
		return p2;
	}
}

void GameScript::OpenDoor(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
	if(scr->Type != ST_ACTOR)
		return;
	if(scr != Sender) { //this is an Action Override
		scr->AddAction(Sender->CurrentAction);
		Sender->CurrentAction = NULL;
		return;
	}
	Scriptable * tar = core->GetGame()->GetMap(0)->tm->GetDoor(parameters->objects[1]->objectName);
	if(!tar)
		return;
	if(tar->Type != ST_DOOR)
		return;
	Door * door = (Door*)tar;
	Actor * actor = (Actor*)scr;
	double distance;
	Point * p = FindNearPoint(actor, &door->toOpen[0], &door->toOpen[1], distance);	
	if(distance <= 12) {
		door->SetDoorClosed(false, true);		
	} else {
		Sender->AddActionInFront(Sender->CurrentAction);
		char Tmp[256];
		sprintf(Tmp, "MoveToPoint([%d,%d])", p->x, p->y);
		actor->AddActionInFront(GameScript::CreateAction(Tmp, true));
	}
	Sender->CurrentAction = NULL;
}

void GameScript::CloseDoor(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
	if(scr->Type != ST_ACTOR)
		return;
	if(scr != Sender) { //this is an Action Override
		scr->AddAction(Sender->CurrentAction);
		Sender->CurrentAction = NULL;
		return;
	}
	Scriptable * tar = core->GetGame()->GetMap(0)->tm->GetDoor(parameters->objects[1]->objectName);
	if(!tar)
		return;
	if(tar->Type != ST_DOOR)
		return;
	Door * door = (Door*)tar;
	Actor * actor = (Actor*)scr;
	double distance;
	Point * p = FindNearPoint(actor, &door->toOpen[0], &door->toOpen[1], distance);	
	if(distance <= 12) {
		door->SetDoorClosed(true, true);
	} else {
		Sender->AddActionInFront(Sender->CurrentAction);
		char Tmp[256];
		sprintf(Tmp, "MoveToPoint([%d,%d])", p->x, p->y);
		actor->AddActionInFront(GameScript::CreateAction(Tmp, true));
	}
	Sender->CurrentAction = NULL;
}

void GameScript::MoveBetweenAreas(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
	if(scr->Type != ST_ACTOR)
		return;
	if(scr != Sender) { //this is an Action Override
		scr->AddAction(Sender->CurrentAction);
		return;
	}
	Actor * actor = (Actor*)Sender;
	strcpy(actor->Area, parameters->string0Parameter);
	if(!actor->FromGame) {
		actor->FromGame = true;
		if(actor->InParty)
			core->GetGame()->SetPC(actor);
		else
			core->GetGame()->AddNPC(actor);
		core->GetGame()->GetMap(0)->RemoveActor(actor);
		actor->XPos = parameters->XpointParameter;
		actor->YPos = parameters->YpointParameter;
		actor->Orientation = parameters->int0Parameter;
	}
}

void GetPositionFromScriptable(Scriptable * scr, unsigned short &X, unsigned short &Y)
{
	switch(scr->Type) {
		case ST_TRIGGER:
		case ST_PROXIMITY:
		case ST_TRAVEL:
			{
				InfoPoint * ip = (InfoPoint*)scr;
				X = ip->TrapLaunchX;
				Y = ip->TrapLaunchY;
			}
		break;

		case ST_ACTOR:
			{
				Actor * ac = (Actor*)scr;
				X = ac->XPos;
				Y = ac->YPos;
			}
		break;

		case ST_DOOR:
			{
				Door * door = (Door*)scr;
				X = door->XPos;
				Y = door->YPos;
			}
		break;

		case ST_CONTAINER:
			{
				Container * cont = (Container*)scr;
				X = cont->trapTarget.x;
				Y = cont->trapTarget.y;
			}
		break;
	}
}

void GameScript::ForceSpell(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
	if(scr != Sender) { //this is an Action Override
		scr->AddAction(Sender->CurrentAction);
		return;
	}
	Scriptable * tar = GetActorFromObject(Sender, parameters->objects[1]);
	if(!tar)
		return;
	unsigned short sX,sY, dX,dY;
	GetPositionFromScriptable(scr, sX, sY);
	GetPositionFromScriptable(tar, dX, dY);
	printf("ForceSpell from [%d,%d] to [%d,%d]\n", sX, sY, dX, dY);
}

void GameScript::Deactivate(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
	if(scr != Sender) { //this is an Action Override
		scr->AddAction(Sender->CurrentAction);
		return;
	}
	Scriptable * tar = GetActorFromObject(Sender, parameters->objects[1]);
	if(!tar)
		return;
	if(tar->Type != ST_ACTOR)
		return;
	tar->Active = false;
}

void GameScript::Activate(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
	if(scr != Sender) { //this is an Action Override
		scr->AddAction(Sender->CurrentAction);
		return;
	}
	Scriptable * tar = GetActorFromObject(Sender, parameters->objects[1]);
	if(!tar)
		return;
	if(tar->Type != ST_ACTOR)
		return;
	tar->Active = true;
}

void GameScript::LeaveAreaLUA(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
	if(scr->Type != ST_ACTOR)
		return;
	if(scr != Sender) { //this is an Action Override
		scr->AddAction(Sender->CurrentAction);
		return;
	}
	Actor * actor = (Actor*)Sender;
	strcpy(actor->Area, parameters->string0Parameter);
	if(!actor->FromGame) {
		actor->FromGame = true;
		if(actor->InParty)
			core->GetGame()->SetPC(actor);
		else
			core->GetGame()->AddNPC(actor);
	}
	core->GetGame()->GetMap(0)->RemoveActor(actor);
	if(parameters->XpointParameter >= 0)
		actor->XPos = parameters->XpointParameter;
	if(parameters->YpointParameter >= 0)
		actor->YPos = parameters->YpointParameter;
	if(parameters->int0Parameter >= 0)
		actor->Orientation = parameters->int0Parameter;
}

void GameScript::LeaveAreaLUAPanic(Scriptable * Sender, Action * parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
	if(scr->Type != ST_ACTOR)
		return;
	if(scr != Sender) { //this is an Action Override
		scr->AddAction(Sender->CurrentAction);
		return;
	}
	LeaveAreaLUA(Sender, parameters);
}

void GameScript::SetTokenGlobal(Scriptable *Sender, Action *parameters)
{
	unsigned long value=0;
	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0) {
		globals->Lookup(&parameters->string0Parameter[6], value);
	}
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0) {
		Sender->locals->Lookup(&parameters->string0Parameter[6], value);
	}
	else {
		globals->Lookup(parameters->string0Parameter, value);
	}
	char varname[33];
	strncpy(varname,parameters->string1Parameter,32);
	varname[32]=0;
	printf("SetTokenGlobal: %d -> %s\n",value, varname);
	char tmpstr[10];
	sprintf(tmpstr,"%d",value);
	core->GetTokenDictionary()->SetAt(varname, tmpstr);
}

void GameScript::PlayDead(Scriptable *Sender, Action *parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(!scr)
		return;
	if(scr->Type != ST_ACTOR)
		return;
	Actor * actor = (Actor*)scr;
	actor->AnimID=IE_ANI_DIE;
	//also set time for playdead!
}

void GameScript::GlobalSetGlobal(Scriptable *Sender, Action *parameters)
{
	unsigned long value=0;
	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0) {
		globals->Lookup(&parameters->string0Parameter[6], value);
	}
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0) {
		Sender->locals->Lookup(&parameters->string0Parameter[6], value);
	}
	else {
		globals->Lookup(parameters->string0Parameter, value);
	}

	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0)
		globals->SetAt(&parameters->string0Parameter[6], value);
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0)
		Sender->locals->SetAt(&parameters->string0Parameter[6], value);
	else {
		globals->SetAt(parameters->string0Parameter, value);
	}
}

/* adding the second variable to the first, they must be GLOBAL */
void GameScript::AddGlobals(Scriptable *Sender, Action *parameters)
{
	unsigned long value1=0;
	unsigned long value2=0;
        char Variable0[6+33], Variable1[6+33];

	memcpy(Variable0,"GLOBAL",6);
	memcpy(Variable1,"GLOBAL",6);
	strncpy(Variable0+6,parameters->string0Parameter,32);
	strncpy(Variable1+6,parameters->string0Parameter,32);
	globals->Lookup(Variable0, value1);
	globals->Lookup(Variable1, value2);
	globals->SetAt(Variable0, value1+value2);
}

/* adding the second variable to the first, they could be area or locals */
void GameScript::GlobalAddGlobal(Scriptable *Sender, Action *parameters)
{
	unsigned long value1=0;
	unsigned long value2=0;
	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0) {
		globals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0) {
		Sender->locals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else {
		globals->Lookup(parameters->string0Parameter, value1);
	}

	if(memcmp(parameters->string1Parameter, "GLOBAL", 6) == 0) {
		globals->Lookup(&parameters->string1Parameter[6], value2);
	}
	else if(memcmp(parameters->string1Parameter, "LOCALS", 6) == 0) {
		Sender->locals->Lookup(&parameters->string1Parameter[6], value2);
	}
	else {
		globals->Lookup(parameters->string1Parameter, value2);
	}

	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0)
		globals->SetAt(&parameters->string0Parameter[6], value1+value2);
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0)
		Sender->locals->SetAt(&parameters->string0Parameter[6], value1+value2);
	else {
		globals->SetAt(parameters->string0Parameter, value1+value2);
	}
}

/* adding the number to the global, they could be area or locals */
void GameScript::IncrementGlobal(Scriptable *Sender, Action *parameters)
{
	unsigned long value1=0;
	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0) {
		globals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0) {
		Sender->locals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else {
		globals->Lookup(parameters->string0Parameter, value1);
	}

	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0)
		globals->SetAt(&parameters->string0Parameter[6], value1+parameters->int0Parameter);
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0)
		Sender->locals->SetAt(&parameters->string0Parameter[6], value1+parameters->int0Parameter);
	else {
		globals->SetAt(parameters->string0Parameter, value1+parameters->int0Parameter);
	}
}

void GameScript::GlobalSubGlobal(Scriptable *Sender, Action *parameters)
{
	unsigned long value1=0;
	unsigned long value2=0;
	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0) {
		globals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0) {
		Sender->locals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else {
		globals->Lookup(parameters->string0Parameter, value1);
	}

	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0) {
		globals->Lookup(&parameters->string0Parameter[6], value2);
	}
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0) {
		Sender->locals->Lookup(&parameters->string0Parameter[6], value2);
	}
	else {
		globals->Lookup(parameters->string1Parameter, value2);
	}

	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0)
		globals->SetAt(&parameters->string0Parameter[6], value1-value2);
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0)
		Sender->locals->SetAt(&parameters->string0Parameter[6], value1-value2);
	else {
		globals->SetAt(parameters->string0Parameter, value1-value2);
	}
}

void GameScript::GlobalAndGlobal(Scriptable *Sender, Action *parameters)
{
	unsigned long value1=0;
	unsigned long value2=0;
	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0) {
		globals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0) {
		Sender->locals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else {
		globals->Lookup(parameters->string0Parameter, value1);
	}

	if(memcmp(parameters->string1Parameter, "GLOBAL", 6) == 0) {
		globals->Lookup(&parameters->string1Parameter[6], value2);
	}
	else if(memcmp(parameters->string1Parameter, "LOCALS", 6) == 0) {
		Sender->locals->Lookup(&parameters->string1Parameter[6], value2);
	}
	else {
		globals->Lookup(parameters->string1Parameter, value2);
	}

	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0)
		globals->SetAt(&parameters->string0Parameter[6], value1&&value2);
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0)
		Sender->locals->SetAt(&parameters->string0Parameter[6], value1&&value2);
	else {
		globals->SetAt(parameters->string0Parameter, value1&&value2);
	}
}

void GameScript::GlobalOrGlobal(Scriptable *Sender, Action *parameters)
{
	unsigned long value1=0;
	unsigned long value2=0;
	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0) {
		globals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0) {
		Sender->locals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else {
		globals->Lookup(parameters->string0Parameter, value1);
	}

	if(memcmp(parameters->string1Parameter, "GLOBAL", 6) == 0) {
		globals->Lookup(&parameters->string1Parameter[6], value2);
	}
	else if(memcmp(parameters->string1Parameter, "LOCALS", 6) == 0) {
		Sender->locals->Lookup(&parameters->string1Parameter[6], value2);
	}
	else {
		globals->Lookup(parameters->string1Parameter, value2);
	}

	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0)
		globals->SetAt(&parameters->string0Parameter[6], value1||value2);
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0)
		Sender->locals->SetAt(&parameters->string0Parameter[6], value1||value2);
	else {
		globals->SetAt(parameters->string0Parameter, value1||value2);
	}
}

void GameScript::GlobalBOrGlobal(Scriptable *Sender, Action *parameters)
{
	unsigned long value1=0;
	unsigned long value2=0;
	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0) {
		globals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0) {
		Sender->locals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else {
		globals->Lookup(parameters->string0Parameter, value1);
	}

	if(memcmp(parameters->string1Parameter, "GLOBAL", 6) == 0) {
		globals->Lookup(&parameters->string1Parameter[6], value2);
	}
	else if(memcmp(parameters->string1Parameter, "LOCALS", 6) == 0) {
		Sender->locals->Lookup(&parameters->string1Parameter[6], value2);
	}
	else {
		globals->Lookup(parameters->string1Parameter, value2);
	}

	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0)
		globals->SetAt(&parameters->string0Parameter[6], value1|value2);
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0)
		Sender->locals->SetAt(&parameters->string0Parameter[6], value1|value2);
	else {
		globals->SetAt(parameters->string0Parameter, value1|value2);
	}
}

void GameScript::GlobalBAndGlobal(Scriptable *Sender, Action *parameters)
{
	unsigned long value1=0;
	unsigned long value2=0;
	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0) {
		globals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0) {
		Sender->locals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else {
		globals->Lookup(parameters->string0Parameter, value1);
	}

	if(memcmp(parameters->string1Parameter, "GLOBAL", 6) == 0) {
		globals->Lookup(&parameters->string1Parameter[6], value2);
	}
	else if(memcmp(parameters->string1Parameter, "LOCALS", 6) == 0) {
		Sender->locals->Lookup(&parameters->string1Parameter[6], value2);
	}
	else {
		globals->Lookup(parameters->string1Parameter, value2);
	}

	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0)
		globals->SetAt(&parameters->string0Parameter[6], value1&value2);
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0)
		Sender->locals->SetAt(&parameters->string0Parameter[6], value1&value2);
	else {
		globals->SetAt(parameters->string0Parameter, value1&value2);
	}
}

void GameScript::GlobalBOr(Scriptable *Sender, Action *parameters)
{
	unsigned long value1=0;
	unsigned long value2=parameters->int0Parameter;
	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0) {
		globals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0) {
		Sender->locals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else {
		globals->Lookup(parameters->string0Parameter, value1);
	}

	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0)
		globals->SetAt(&parameters->string0Parameter[6], value1|value2);
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0)
		Sender->locals->SetAt(&parameters->string0Parameter[6], value1|value2);
	else {
		globals->SetAt(parameters->string0Parameter, value1|value2);
	}
}

void GameScript::GlobalBAnd(Scriptable *Sender, Action *parameters)
{
	unsigned long value1=0;
	unsigned long value2=parameters->int0Parameter;
	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0) {
		globals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0) {
		Sender->locals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else {
		globals->Lookup(parameters->string0Parameter, value1);
	}

	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0)
		globals->SetAt(&parameters->string0Parameter[6], value1&value2);
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0)
		Sender->locals->SetAt(&parameters->string0Parameter[6], value1&value2);
	else {
		globals->SetAt(parameters->string0Parameter, value1&value2);
	}
}

void GameScript::GlobalMax(Scriptable *Sender, Action *parameters)
{
	unsigned long value1=0;
	unsigned long value2=parameters->int0Parameter;
	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0) {
		globals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0) {
		Sender->locals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else {
		globals->Lookup(parameters->string0Parameter, value1);
	}

	if(value1>=value2) return;
	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0)
		globals->SetAt(&parameters->string0Parameter[6], value2);
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0)
		Sender->locals->SetAt(&parameters->string0Parameter[6], value2);
	else {
		globals->SetAt(parameters->string0Parameter, value2);
	}
}

void GameScript::GlobalMin(Scriptable *Sender, Action *parameters)
{
	unsigned long value1=0;
	unsigned long value2=parameters->int0Parameter;
	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0) {
		globals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0) {
		Sender->locals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else {
		globals->Lookup(parameters->string0Parameter, value1);
	}

	if(value1<=value2) return;
	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0)
		globals->SetAt(&parameters->string0Parameter[6], value2);
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0)
		Sender->locals->SetAt(&parameters->string0Parameter[6], value2);
	else {
		globals->SetAt(parameters->string0Parameter, value2);
	}
}

void GameScript::BitClear(Scriptable *Sender, Action *parameters)
{
	unsigned long value1=0;
	unsigned long value2=parameters->int0Parameter;
	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0) {
		globals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0) {
		Sender->locals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else {
		globals->Lookup(parameters->string0Parameter, value1);
	}

	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0)
		globals->SetAt(&parameters->string0Parameter[6], value1&~value2);
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0)
		Sender->locals->SetAt(&parameters->string0Parameter[6], value1&~value2);
	else {
		globals->SetAt(parameters->string0Parameter, value1&~value2);
	}
}

void GameScript::GlobalShl(Scriptable *Sender, Action *parameters)
{
	unsigned long value1=0;
	unsigned long value2=parameters->int0Parameter;
	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0) {
		globals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0) {
		Sender->locals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else {
		globals->Lookup(parameters->string0Parameter, value1);
	}

	if(value2>31) value1=0;
	else value1<<=value2;

	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0)
		globals->SetAt(&parameters->string0Parameter[6], value1);
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0)
		Sender->locals->SetAt(&parameters->string0Parameter[6], value1);
	else {
		globals->SetAt(parameters->string0Parameter, value1);
	}
}

void GameScript::GlobalShr(Scriptable *Sender, Action *parameters)
{
	unsigned long value1=0;
	unsigned long value2=parameters->int0Parameter;
	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0) {
		globals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0) {
		Sender->locals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else {
		globals->Lookup(parameters->string0Parameter, value1);
	}

	if(value2>31) value1=0;
	else value1>>=value2;

	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0)
		globals->SetAt(&parameters->string0Parameter[6], value1);
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0)
		Sender->locals->SetAt(&parameters->string0Parameter[6], value1);
	else {
		globals->SetAt(parameters->string0Parameter, value1);
	}
}

void GameScript::GlobalMaxGlobal(Scriptable *Sender, Action *parameters)
{
	unsigned long value1=0;
	unsigned long value2=0;
	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0) {
		globals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0) {
		Sender->locals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else {
		globals->Lookup(parameters->string0Parameter, value1);
	}

	if(memcmp(parameters->string1Parameter, "GLOBAL", 6) == 0) {
		globals->Lookup(&parameters->string1Parameter[6], value2);
	}
	else if(memcmp(parameters->string1Parameter, "LOCALS", 6) == 0) {
		Sender->locals->Lookup(&parameters->string1Parameter[6], value2);
	}
	else {
		globals->Lookup(parameters->string1Parameter, value2);
	}

	if(value1>=value2) return;
	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0)
		globals->SetAt(&parameters->string0Parameter[6], value2);
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0)
		Sender->locals->SetAt(&parameters->string0Parameter[6], value2);
	else {
		globals->SetAt(parameters->string0Parameter, value2);
	}
}

void GameScript::GlobalMinGlobal(Scriptable *Sender, Action *parameters)
{
	unsigned long value1=0;
	unsigned long value2=0;
	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0) {
		globals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0) {
		Sender->locals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else {
		globals->Lookup(parameters->string0Parameter, value1);
	}

	if(memcmp(parameters->string1Parameter, "GLOBAL", 6) == 0) {
		globals->Lookup(&parameters->string1Parameter[6], value2);
	}
	else if(memcmp(parameters->string1Parameter, "LOCALS", 6) == 0) {
		Sender->locals->Lookup(&parameters->string1Parameter[6], value2);
	}
	else {
		globals->Lookup(parameters->string1Parameter, value2);
	}

	if(value1<=value2) return;
	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0)
		globals->SetAt(&parameters->string0Parameter[6], value2);
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0)
		Sender->locals->SetAt(&parameters->string0Parameter[6], value2);
	else {
		globals->SetAt(parameters->string0Parameter, value2);
	}
}

void GameScript::GlobalShlGlobal(Scriptable *Sender, Action *parameters)
{
	unsigned long value1=0;
	unsigned long value2=parameters->int0Parameter;
	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0) {
		globals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0) {
		Sender->locals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else {
		globals->Lookup(parameters->string0Parameter, value1);
	}

	if(value2>31) value1=0;
	else value1<<=value2;

	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0)
		globals->SetAt(&parameters->string0Parameter[6], value1);
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0)
		Sender->locals->SetAt(&parameters->string0Parameter[6], value1);
	else {
		globals->SetAt(parameters->string0Parameter, value1);
	}
}
void GameScript::GlobalShrGlobal(Scriptable *Sender, Action *parameters)
{
	unsigned long value1=0;
	unsigned long value2=parameters->int0Parameter;
	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0) {
		globals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0) {
		Sender->locals->Lookup(&parameters->string0Parameter[6], value1);
	}
	else {
		globals->Lookup(parameters->string0Parameter, value1);
	}

	if(value2>31) value1=0;
	else value1>>=value2;

	if(memcmp(parameters->string0Parameter, "GLOBAL", 6) == 0)
		globals->SetAt(&parameters->string0Parameter[6], value1);
	else if(memcmp(parameters->string0Parameter, "LOCALS", 6) == 0)
		Sender->locals->SetAt(&parameters->string0Parameter[6], value1);
	else {
		globals->SetAt(parameters->string0Parameter, value1);
	}
}

void GameScript::ClearAllActions(Scriptable *Sender, Action *parameters)
{
//just a hack
	for(int i=0;i<6;i++) {
		Scriptable * scr = core->GetGame()->GetPC(i);
		if(scr)
			scr->ClearActions();
	}
}

void GameScript::ClearActions(Scriptable *Sender, Action *parameters)
{
	Scriptable * scr = GetActorFromObject(Sender, parameters->objects[0]);
	if(scr)
		scr->ClearActions();
}
