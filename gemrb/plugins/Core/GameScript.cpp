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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/GameScript.cpp,v 1.72 2004/02/26 16:51:04 balrog994 Exp $
 *
 */

#include "../../includes/win32def.h"
#include "GameScript.h"
#include "Interface.h"
//#include "DialogMgr.h"

extern Interface* core;
#ifdef WIN32
extern HANDLE hConsole;
#endif

static int initialized = 0;
static Variables* globals;
static SymbolMgr* triggersTable;
static SymbolMgr* actionsTable;
static SymbolMgr* objectsTable;
static TriggerFunction triggers[MAX_TRIGGERS];
static ActionFunction actions[MAX_ACTIONS];
static ObjectFunction objects[MAX_ACTIONS];
static int flags[MAX_ACTIONS];

static int ObjectIDSCount = 7;
static int MaxObjectNesting = 5;
static bool HasAdditionalRect = false;
static std::vector< char*> ObjectIDSTableNames;
static int ObjectFieldsCount = 7;
static int ExtraParametersCount = 0;

//Make this an ordered list, so we could use bsearch!
static TriggerLink triggernames[] = {
	{"actionlistempty", GameScript::ActionListEmpty},
	{"alignment", GameScript::Alignment},
	{"allegiance", GameScript::Allegiance}, {"bitcheck",GameScript::BitCheck},
	{"checkstat",GameScript::CheckStat},
	{"checkstatgt",GameScript::CheckStatGT},
	{"checkstatlt",GameScript::CheckStatLT}, {"class", GameScript::Class},
	{"clicked", GameScript::Clicked}, {"dead", GameScript::Dead},
	{"entered", GameScript::Entered}, {"exists", GameScript::Exists},
	{"false", GameScript::False}, {"gender", GameScript::Gender},
	{"general", GameScript::General}, {"global", GameScript::Global},
	{"globalgt", GameScript::GlobalGT}, {"globallt", GameScript::GlobalLT},
	{"globalsequal", GameScript::GlobalsEqual}, {"hp", GameScript::HP},
	{"hpgt", GameScript::HPGT}, {"hplt", GameScript::HPLT},
	{"inparty", GameScript::InParty},
	{"isvalidforpartydialog", GameScript::IsValidForPartyDialog},
	{"numtimestalkedto", GameScript::NumTimesTalkedTo},
	{"numtimestalkedtogt", GameScript::NumTimesTalkedToGT},
	{"numtimestalkedtolt", GameScript::NumTimesTalkedToLT},
	{"oncreation", GameScript::OnCreation}, {"or", GameScript::Or},
	{"partyhasitem", GameScript::PartyHasItem}, {"race", GameScript::Race},
	{"range", GameScript::Range}, {"see", GameScript::See},
	{"true", GameScript::True}, {"xp", GameScript::XP},
	{"xpgt", GameScript::XPGT}, {"xplt", GameScript::XPLT}, { NULL,NULL}, 
};

//Make this an ordered list, so we could use bsearch!
static ActionLink actionnames[] = {
	{"actionoverride",NULL}, {"activate",GameScript::Activate},
	{"addglobals",GameScript::AddGlobals}, {"ally",GameScript::Ally},
	{"ambientactivate",GameScript::AmbientActivate},
	{"bitclear",GameScript::BitClear}, {"bitset",GameScript::GlobalBOr}, //probably the same
	{"changeaiscript",GameScript::ChangeAIScript},
	{"changealignment",GameScript::ChangeAlignment},
	{"changeallegiance",GameScript::ChangeAllegiance},
	{"changeclass",GameScript::ChangeClass},
	{"changegender",GameScript::ChangeGender},
	{"changegeneral",GameScript::ChangeGeneral},
	{"changeenemyally",GameScript::ChangeAllegiance}, //this is the same
	{"changerace",GameScript::ChangeRace},
	{"changespecifics",GameScript::ChangeSpecifics},
	{"changestat",GameScript::ChangeStat},
	{"clearactions",GameScript::ClearActions},
	{"clearallactions",GameScript::ClearAllActions},
	{"closedoor",GameScript::CloseDoor,AF_BLOCKING},
	{"continue",GameScript::Continue,AF_INSTANT | AF_CONTINUE},
	{"createcreature",GameScript::CreateCreature},
	{"createvisualeffect",GameScript::CreateVisualEffect},
	{"createvisualeffectobject",GameScript::CreateVisualEffectObject},
	{"cutsceneid",GameScript::CutSceneID,AF_INSTANT},
	{"deactivate",GameScript::Deactivate},
	{"destroyself",GameScript::DestroySelf},
	{"dialogue",GameScript::Dialogue,AF_BLOCKING},
	{"dialogueforceinterrupt",GameScript::DialogueForceInterrupt,AF_BLOCKING},
	{"displaystring",GameScript::DisplayString},
	{"displaystringhead",GameScript::DisplayStringHead},
	{"displaystringwait",GameScript::DisplayStringWait,AF_BLOCKING},
	{"endcutscenemode",GameScript::EndCutSceneMode},
	{"enemy",GameScript::Enemy}, {"face",GameScript::Face,AF_BLOCKING},
	{"faceobject",GameScript::FaceObject, AF_BLOCKING},
	{"fadefromcolor",GameScript::FadeFromColor},
	{"fadetocolor",GameScript::FadeToColor},
	{"floatmessage",GameScript::DisplayStringHead}, //probably the same
	{"forceaiscript",GameScript::ChangeAIScript}, //probably the same
	{"forcespell",GameScript::ForceSpell},
	{"globalband",GameScript::GlobalBAnd},
	{"globalbandglobal",GameScript::GlobalBAndGlobal},
	{"globalbor",GameScript::GlobalBOr},
	{"globalborglobal",GameScript::GlobalBOrGlobal},
	{"globalmax",GameScript::GlobalMax},
	{"globalmaxglobal",GameScript::GlobalMaxGlobal},
	{"globalmin",GameScript::GlobalMin},
	{"globalminglobal",GameScript::GlobalMinGlobal},
	{"globalshl",GameScript::GlobalShL},
	{"globalshlglobal",GameScript::GlobalShLGlobal},
	{"globalshr",GameScript::GlobalShR},
	{"globalshrglobal",GameScript::GlobalShRGlobal},
	{"hidegui",GameScript::HideGUI},
	{"incrementglobal",GameScript::IncrementGlobal},
	{"incrementglobalonce",GameScript::IncrementGlobalOnce},
	{"joinparty",GameScript::JoinParty},
	{"jumptoobject",GameScript::JumpToObject},
	{"jumptopoint",GameScript::JumpToPoint},
	{"jumptopointinstant",GameScript::JumpToPointInstant},
	{"leavearealua",GameScript::LeaveAreaLUA},
	{"leavearealuapanic",GameScript::LeaveAreaLUAPanic},
	{"makeglobal",GameScript::MakeGlobal},
	{"movebetweenareas",GameScript::MoveBetweenAreas},
	{"movetoobject",GameScript::MoveToObject,AF_BLOCKING},
	{"movetopoint",GameScript::MoveToPoint,AF_BLOCKING},
	{"moveviewobject",GameScript::MoveViewPoint},
	{"moveviewpoint",GameScript::MoveViewPoint},
	{"noaction",GameScript::NoAction},
	{"opendoor",GameScript::OpenDoor,AF_BLOCKING},
	{"playdead",GameScript::PlayDead}, {"playsound",GameScript::PlaySound},
	{"screenshake",GameScript::ScreenShake,AF_BLOCKING},
	{"setdialogue",GameScript::SetDialogue,AF_BLOCKING},
	{"setglobal",GameScript::SetGlobal},
	{"setnumtimestalkedto",GameScript::SetNumTimesTalkedTo},
	{"settokenglobal",GameScript::SetTokenGlobal}, {"sg",GameScript::SG},
	{"smallwait",GameScript::SmallWait,AF_BLOCKING},
	{"startcutscene",GameScript::StartCutScene},
	{"startcutscenemode",GameScript::StartCutSceneMode},
	{"startdialogue",GameScript::StartDialogue,AF_BLOCKING},
	{"startdialogueinterrupt",GameScript::StartDialogueInterrupt,AF_BLOCKING},
	{"startdialoguenoset",GameScript::StartDialogueNoSet,AF_BLOCKING},
	{"startdialoguenosetinterrupt",GameScript::StartDialogueNoSetInterrupt,AF_BLOCKING},
	{"startdialogueoverride",GameScript::StartDialogueOverride,AF_BLOCKING},
	{"startdialogueoverrideinterrupt",GameScript::StartDialogueOverrideInterrupt,AF_BLOCKING},
	{"startmovie",GameScript::StartMovie},
	{"startsong",GameScript::StartSong},
	{"triggeractivation",GameScript::TriggerActivation},
	{"unhidegui",GameScript::UnhideGUI},
	{"unmakeglobal",GameScript::UnMakeGlobal}, //this is a GemRB extension
	{"wait",GameScript::Wait, AF_BLOCKING}, { NULL,NULL}, 
};

//Make this an ordered list, so we could use bsearch!
static ObjectLink objectnames[] = {
	{ NULL,NULL}, 
};

static TriggerLink* FindTrigger(const char* triggername)
{
	if (!triggername) {
		return NULL;
	}
	int len = strlench( triggername, '(' );
	for (int i = 0; triggernames[i].Name; i++) {
		if (!strnicmp( triggernames[i].Name, triggername, len )) {
			return triggernames + i;
		}
	}
	printf( "Warning: Couldn't assign trigger: %.*s\n", len, triggername );
	return NULL;
}

static ActionLink* FindAction(const char* actionname)
{
	if (!actionname) {
		return NULL;
	}
	int len = strlench( actionname, '(' );
	for (int i = 0; actionnames[i].Name; i++) {
		if (!strnicmp( actionnames[i].Name, actionname, len )) {
			return actionnames + i;
		}
	}
	printf( "Warning: Couldn't assign action: %.*s\n", len, actionname );
	return NULL;
}

static ObjectLink* FindObject(const char* objectname)
{
	if (!objectname) {
		return NULL;
	}
	int len = strlench( objectname, '(' );
	for (int i = 0; objectnames[i].Name; i++) {
		if (!strnicmp( objectnames[i].Name, objectname, len )) {
			return objectnames + i;
		}
	}
	printf( "Warning: Couldn't assign object: %.*s\n", len, objectname );
	return NULL;
}

GameScript::GameScript(const char* ResRef, unsigned char ScriptType,
	Variables* local)
{
	if (local) {
		locals = local;
		freeLocals = false;
	} else {
		locals = new Variables();
		locals->SetType( GEM_VARIABLES_INT );
		freeLocals = true;
	}
	if (!initialized) {
		initialized = 1;
		int tT = core->LoadSymbol( "TRIGGER" );
		int aT = core->LoadSymbol( "ACTION" );
		int oT = core->LoadSymbol( "OBJECT" );
		int iT = core->LoadTable( "SCRIPT" );
		if (tT < 0 || aT < 0 || oT < 0 || iT < 0) {
			printf( "[IEScript]: A critical scripting file is missing!\n" );
			abort();
		}
		triggersTable = core->GetSymbol( tT );
		actionsTable = core->GetSymbol( aT );
		objectsTable = core->GetSymbol( oT );
		TableMgr* objNameTable = core->GetTable( iT );
		if (!triggersTable || !actionsTable || !objectsTable || !objNameTable) {
			printf( "[IEScript]: A critical scripting file is damaged!\n" );
			abort();
		}

		int i;

		/* Loading Script Configuration Parameters */

		ObjectIDSCount = atoi( objNameTable->QueryField() );
		for (i = 0; i < ObjectIDSCount; i++) {
			ObjectIDSTableNames.push_back( objNameTable->QueryField( 0, i + 1 ) );
		}
		MaxObjectNesting = atoi( objNameTable->QueryField( 1 ) );
		HasAdditionalRect = ( bool ) atoi( objNameTable->QueryField( 2 ) );
		ExtraParametersCount = atoi( objNameTable->QueryField( 3 ) );
		ObjectFieldsCount = ObjectIDSCount - ExtraParametersCount;

		/* Initializing the Script Engine */

		globals = new Variables();
		globals->SetType( GEM_VARIABLES_INT );
		memset( triggers, 0, sizeof( triggers ) );
		memset( actions, 0, sizeof( actions ) );
		memset( objects, 0, sizeof( objects ) );

		for (i = 0; i < MAX_TRIGGERS; i++) {
			char* triggername = triggersTable->GetValue( i );
			//maybe we should watch for this bit?
			if (!triggername)
				triggername = triggersTable->GetValue( i | 0x4000 );
			TriggerLink* poi = FindTrigger( triggername );
			if (poi == NULL)
				triggers[i] = NULL;
			else
				triggers[i] = poi->Function;
		}

		for (i = 0; i < MAX_ACTIONS; i++) {
			ActionLink* poi = FindAction( actionsTable->GetValue( i ) );
			if (poi == NULL) {
				actions[i] = NULL;
				flags[i] = 0;
			} else {
				actions[i] = poi->Function;
				flags[i] = poi->Flags;
			}
		}
		for (i = 0; i < MAX_OBJECTS; i++) {
			ObjectLink* poi = FindObject( objectsTable->GetValue( i ) );
			if (poi == NULL) {
				objects[i] = NULL;
			} else {
				objects[i] = poi->Function;
			}
		}
		initialized = 2;
	}
	continueExecution = false;
	DataStream* ds = core->GetResourceMgr()->GetResource( ResRef,
												IE_BCS_CLASS_ID );
	script = CacheScript( ds, ResRef );
	MySelf = NULL;
	scriptRunDelay = 1000;
	scriptType = ScriptType;
	lastRunTime = 0;
	endReached = false;
	strcpy( Name, ResRef );
}

GameScript::~GameScript(void)
{
	if (script) {
		script->Release();
	}
	if (freeLocals) {
		if (locals) {
			delete( locals );
		}
	}
}

Script* GameScript::CacheScript(DataStream* stream, const char* Context)
{
	if (!stream) {
		return NULL;
	}
	char* line = ( char* ) malloc( 10 );
	stream->ReadLine( line, 10 );
	if (strncmp( line, "SC", 2 ) != 0) {
		printf( "[IEScript]: Not a Compiled Script file\n" );
		delete( stream );
		free( line );
		return NULL;
	}
	Script* newScript = new Script( Context );
	std::vector< ResponseBlock*> rBv;
	while (true) {
		ResponseBlock* rB = ReadResponseBlock( stream );
		if (!rB)
			break;
		rBv.push_back( rB );
		stream->ReadLine( line, 10 );
	}
	free( line );
	newScript->AllocateBlocks( ( unsigned int ) rBv.size() );
	for (unsigned int i = 0; i < newScript->responseBlocksCount; i++) {
		newScript->responseBlocks[i] = rBv.at( i );
	}
	delete( stream );
	return newScript;
}

void GameScript::ReplaceMyArea(Scriptable* Sender, char* newVarName)
{
}

void GameScript::SetVariable(Scriptable* Sender, const char* VarName,
	const char* Context, int value)
{
	if (strnicmp( VarName, "LOCALS", 6 ) == 0) {
		Sender->locals->SetAt( VarName, value );
		return;
	}
	char* newVarName = ( char* ) malloc( 40 );
	strncpy( newVarName, Context, 6 );
	strncat( newVarName, VarName, 40 );
	if (strnicmp( newVarName, "MYAREA", 6 ) == 0) {
		ReplaceMyArea( Sender, newVarName );
	}
	globals->SetAt( newVarName, ( unsigned long ) value );
}

void GameScript::SetVariable(Scriptable* Sender, const char* VarName,
	int value)
{
	if (strnicmp( VarName, "LOCALS", 6 ) == 0) {
		Sender->locals->SetAt( &VarName[6], value );
		return;
	}
	char* newVarName = strndup( VarName, 40 );
	if (strnicmp( newVarName, "MYAREA", 6 ) == 0) {
		ReplaceMyArea( Sender, newVarName );
	}
	globals->SetAt( newVarName, ( unsigned long ) value );
}

unsigned long GameScript::CheckVariable(Scriptable* Sender,
	const char* VarName)
{
	char newVarName[40];
	unsigned long value = 0;

	if (strnicmp( VarName, "LOCALS", 6 ) == 0) {
		Sender->locals->Lookup( &VarName[6], value );
		return value;
	}
	strncpy( newVarName, VarName, 40 );
	if (strnicmp( VarName, "MYAREA", 6 ) == 0) {
		ReplaceMyArea( Sender, newVarName );
	}
	globals->Lookup( newVarName, value );
	return value;
}

unsigned long GameScript::CheckVariable(Scriptable* Sender,
	const char* VarName, const char* Context)
{
	char newVarName[40];
	unsigned long value = 0;

	if (strnicmp( Context, "LOCALS", 6 ) == 0) {
		Sender->locals->Lookup( VarName, value );
		return value;
	}
	strncpy( newVarName, Context, 6 );
	strncat( newVarName, VarName, 40 );
	if (strnicmp( VarName, "MYAREA", 6 ) == 0) {
		ReplaceMyArea( Sender, newVarName );
	}
	globals->Lookup( VarName, value );
	return value;
}

void GameScript::Update()
{
	if (!MySelf || !MySelf->Active) {
		return;
	}
	unsigned long thisTime;
	GetTime( thisTime );
	if (( thisTime - lastRunTime ) < scriptRunDelay) {
		return;
	}
	lastRunTime = thisTime;
	if (!script) {
		return;
	}
	for (unsigned int a = 0; a < script->responseBlocksCount; a++) {
		ResponseBlock* rB = script->responseBlocks[a];
		if (EvaluateCondition( this->MySelf, rB->condition )) {
			continueExecution = ExecuteResponseSet( this->MySelf,
									rB->responseSet );
			endReached = false;
			if (!continueExecution)
				break;
			continueExecution = false;
		}
	}
}

void GameScript::EvaluateAllBlocks()
{
	if (!MySelf || !MySelf->Active) {
		return;
	}
	unsigned long thisTime;
	GetTime( thisTime );
	if (( thisTime - lastRunTime ) < scriptRunDelay) {
		return;
	}
	lastRunTime = thisTime;
	if (!script) {
		return;
	}
	for (unsigned int a = 0; a < script->responseBlocksCount; a++) {
		ResponseBlock* rB = script->responseBlocks[a];
		if (EvaluateCondition( this->MySelf, rB->condition )) {
			ExecuteResponseSet( this->MySelf, rB->responseSet );
			endReached = false;
		}
	}
}

ResponseBlock* GameScript::ReadResponseBlock(DataStream* stream)
{
	char* line = ( char* ) malloc( 10 );
	stream->ReadLine( line, 10 );
	if (strncmp( line, "CR", 2 ) != 0) {
		free( line );
		return NULL;
	}
	free( line );
	ResponseBlock* rB = new ResponseBlock();
	rB->condition = ReadCondition( stream );
	rB->responseSet = ReadResponseSet( stream );
	return rB;
}

Condition* GameScript::ReadCondition(DataStream* stream)
{
	char* line = ( char* ) malloc( 10 );
	stream->ReadLine( line, 10 );
	if (strncmp( line, "CO", 2 ) != 0) {
		free( line );
		return NULL;
	}
	free( line );
	Condition* cO = new Condition();
	std::vector< Trigger*> tRv;
	while (true) {
		Trigger* tR = ReadTrigger( stream );
		if (!tR)
			break;
		tRv.push_back( tR );
	}
	cO->triggersCount = ( unsigned short ) tRv.size();
	cO->triggers = new Trigger * [cO->triggersCount];
	for (int i = 0; i < cO->triggersCount; i++) {
		cO->triggers[i] = tRv.at( i );
	}
	return cO;
}

ResponseSet* GameScript::ReadResponseSet(DataStream* stream)
{
	char* line = ( char* ) malloc( 10 );
	stream->ReadLine( line, 10 );
	if (strncmp( line, "RS", 2 ) != 0) {
		free( line );
		return NULL;
	}
	free( line );
	ResponseSet* rS = new ResponseSet();
	std::vector< Response*> rEv;
	while (true) {
		Response* rE = ReadResponse( stream );
		if (!rE)
			break;
		rEv.push_back( rE );
	}
	rS->responsesCount = ( unsigned short ) rEv.size();
	rS->responses = new Response * [rS->responsesCount];
	for (int i = 0; i < rS->responsesCount; i++) {
		rS->responses[i] = rEv.at( i );
	}
	return rS;
}

Response* GameScript::ReadResponse(DataStream* stream)
{
	char* line = ( char* ) malloc( 1024 );
	stream->ReadLine( line, 1024 );
	if (strncmp( line, "RE", 2 ) != 0) {
		free( line );
		return NULL;
	}
	Response* rE = new Response();
	rE->weight = 0;
	int count = stream->ReadLine( line, 1024 );
	for (int i = 0; i < count; i++) {
		if (( line[i] >= '0' ) && ( line[i] <= '9' )) {
			rE->weight *= 10;
			rE->weight += ( line[i] - '0' );
			continue;
		}
		break;
	}
	std::vector< Action*> aCv;
	while (true) {
		Action* aC = new Action();
		count = stream->ReadLine( line, 1024 );
		for (int i = 0; i < count; i++) {
			if (( line[i] >= '0' ) && ( line[i] <= '9' )) {
				aC->actionID *= 10;
				aC->actionID += ( line[i] - '0' );
				continue;
			}
			break;
		}
		for (int i = 0; i < 3; i++) {
			stream->ReadLine( line, 1024 );
			Object* oB = DecodeObject( line );
			aC->objects[i] = oB;
			if (i != 2)
				stream->ReadLine( line, 1024 );
		}
		stream->ReadLine( line, 1024 );
		sscanf( line, "%d %d %d %d %d\"%[^\"]\" \"%[^\"]\" AC",
			&aC->int0Parameter, &aC->XpointParameter, &aC->YpointParameter,
			&aC->int1Parameter, &aC->int2Parameter, aC->string0Parameter,
			aC->string1Parameter );
		aCv.push_back( aC );
		stream->ReadLine( line, 1024 );
		if (strncmp( line, "RE", 2 ) == 0)
			break;
	}
	free( line );
	rE->actionsCount = ( unsigned char ) aCv.size();
	rE->actions = new Action * [rE->actionsCount];
	for (int i = 0; i < rE->actionsCount; i++) {
		rE->actions[i] = aCv.at( i );
	}
	return rE;
}

Trigger* GameScript::ReadTrigger(DataStream* stream)
{
	char* line = ( char* ) malloc( 1024 );
	stream->ReadLine( line, 1024 );
	if (strncmp( line, "TR", 2 ) != 0) {
		free( line );
		return NULL;
	}
	stream->ReadLine( line, 1024 );
	Trigger* tR = new Trigger();
	if (strcmp( core->GameType, "pst" ) == 0) {
		sscanf( line, "%d %d %d %d %d [%d,%d] \"%[^\"]\" \"%[^\"]\" OB",
			&tR->triggerID, &tR->int0Parameter, &tR->flags,
			&tR->int1Parameter, &tR->int2Parameter, &tR->XpointParameter,
			&tR->YpointParameter, tR->string0Parameter, tR->string1Parameter );
	} else {
		sscanf( line, "%d %d %d %d %d \"%[^\"]\" \"%[^\"]\" OB",
			&tR->triggerID, &tR->int0Parameter, &tR->flags,
			&tR->int1Parameter, &tR->int2Parameter, tR->string0Parameter,
			tR->string1Parameter );
	}
	tR->triggerID &= 0xFF;
	stream->ReadLine( line, 1024 );
	tR->objectParameter = DecodeObject( line );
	stream->ReadLine( line, 1024 );
	free( line );
	return tR;
}

int GameScript::ParseInt(const char*& src)
{
	char number[33];
	char* tmp = number;
	while (*src != ' ') {
		*tmp = *src;
		tmp++;
		src++;
	}
	*tmp = 0;
	src++;
	return atoi( number );
}

void GameScript::ParseString(const char*& src, char* tmp)
{
	while (*src != '"') {
		*tmp = *src;
		tmp++;
		src++;
	}
	*tmp = 0;
	src++;
	return;
}

Object* GameScript::DecodeObject(const char* line)
{
	Object* oB = new Object();
	for (int i = 0; i < ObjectFieldsCount; i++) {
		oB->objectFields[i] = ParseInt( line );
	}
	for (int i = 0; i < MaxObjectNesting; i++) {
		oB->objectIdentifiers[i] = ParseInt( line );
	}
	if (HasAdditionalRect) {
		line++; //Skip [
		for (int i = 0; i < 4; i++) {
			oB->objectRect[i] = ParseInt( line );
		}
		line++; //Skip ] (not really... it skips a ' ' since the ] was skipped by the ParseInt function
	}
	line++; //Skip "
	ParseString( line, oB->objectName );
	line++; //Skip " (the same as above)
	for (int i = 0; i < ExtraParametersCount; i++) {
		oB->objectFields[i + ObjectFieldsCount] = ParseInt( line );
	}
	/*if (strcmp( core->GameType, "pst" ) == 0) {
		sscanf( line,
			"%d %d %d %d %d %d %d %d %d %d %d %d %d %d [%[^]]] \"%[^\"]\"OB",
			&oB->eaField, &oB->factionField, &oB->teamField,
			&oB->generalField, &oB->raceField, &oB->classField,
			&oB->specificField, &oB->genderField, &oB->alignmentField,
			&oB->identifiersField, &oB->XobjectPosition, &oB->YobjectPosition,
			&oB->WobjectPosition, &oB->HobjectPosition, oB->PositionMask,
			oB->objectName );
	} else {
		sscanf( line, "%d %d %d %d %d %d %d %d %d %d %d %d \"%[^\"]\"OB",
			&oB->eaField, &oB->factionField, &oB->teamField,
			&oB->generalField, &oB->raceField, &oB->classField,
			&oB->specificField, &oB->genderField, &oB->alignmentField,
			&oB->identifiersField, &oB->XobjectPosition, &oB->YobjectPosition,
			oB->objectName );
	}*/
	return oB;
}

bool GameScript::EvaluateCondition(Scriptable* Sender, Condition* condition)
{
	int ORcount = 0;
	int result = 0;
	bool subresult = true;

	for (int i = 0; i < condition->triggersCount; i++) {
		Trigger* tR = condition->triggers[i];
		//do not evaluate triggers in an Or() block if one of them
		//was already True()
		if (!ORcount || !subresult)
			result = EvaluateTrigger( Sender, tR );
		if (result > 1) {
			//we started an Or() block
			if (ORcount)
				printf( "[IEScript]: Unfinished OR block encountered!\n" );
			ORcount = result;
			subresult = false;
			continue;
		}
		if (ORcount) {
			subresult |= result;
			if (--ORcount)
				continue;
			result = subresult;
		}
		if (!result)
			return 0;
	}
	if (ORcount) {
		printf( "[IEScript]: Unfinished OR block encountered!\n" );
	}
	return 1;
}

bool GameScript::EvaluateTrigger(Scriptable* Sender, Trigger* trigger)
{
	if (!trigger) {
		printf( "[IEScript]: Trigger evaluation fails due to NULL trigger.\n" );
		return false;
	}
	TriggerFunction func = triggers[trigger->triggerID];
	if (!func) {
		triggers[trigger->triggerID] = False;
		printf( "[IEScript]: Unhandled trigger code: 0x%04x\n",
			trigger->triggerID );
		return false;
	}
	int ret = func( Sender, trigger );
	if (trigger->flags & 1) {
		if (ret) {
			ret = 0;
		} else {
			ret = 1;
		}
	}
	return ret;
}

int GameScript::ExecuteResponseSet(Scriptable* Sender, ResponseSet* rS)
{
	for (int i = 0; i < rS->responsesCount; i++) {
		Response* rE = rS->responses[i];
		int randWeight = ( rand() % 100 ) + 1;
		if (rE->weight >= randWeight) {
			return ExecuteResponse( Sender, rE );
			/* this break is only symbolic */
			break;
		}
	}
	return 0;
}

int GameScript::ExecuteResponse(Scriptable* Sender, Response* rE)
{
	int ret = 0;  // continue or not
	for (int i = 0; i < rE->actionsCount; i++) {
		Action* aC = rE->actions[i];
		switch (flags[aC->actionID] & AF_MASK) {
			case AF_INSTANT:
				ExecuteAction( Sender, aC );
				break;
			case AF_NONE:
				if (Sender->CutSceneId)
					Sender->CutSceneId->AddAction( aC );
				else
					Sender->AddAction( aC );
				break;
			case AF_CONTINUE:
			case AF_MASK:
				ret = 1;
				break;
		}
	}
	return ret;
}

void GameScript::ExecuteAction(Scriptable* Sender, Action* aC)
{
	ActionFunction func = actions[aC->actionID];
	if (func) {
		printf( "[IEScript]: %s\n", actionsTable->GetValue( aC->actionID ) );
		func( Sender, aC );
	} else {
		actions[aC->actionID] = NoAction;
		printf( "[IEScript]: Unhandled action code: %d\n", aC->actionID );
		Sender->CurrentAction = NULL;
		aC->Release();
		return;
	}
	if (!( flags[aC->actionID] & AF_BLOCKING )) {
		Sender->CurrentAction = NULL;
	}
	if (flags[aC->actionID] & AF_INSTANT) {
		return;
	}
	aC->Release();
}

Action* GameScript::CreateAction(char* string, bool autoFree)
{
	Action* aC = GenerateAction( string );
	return aC;
}

Targets* GameScript::EvaluateObject(Scriptable* Sender, Object* oC,
	int IDIndex, Targets* parm)
{
	if (IDIndex < 0) {
		if (oC->objectName[0]) {
			//We want the object by its name...
			Scriptable* aC = core->GetGame()->GetMap( 0 )->GetActor( oC->objectName );
			if (!aC) {
				//It was not an actor... maybe it is a door?
				aC = core->GetGame()->GetMap( 0 )->tm->GetDoor( oC->objectName );
				if (!aC) {
					//No... it was not a door... maybe an InfoPoint?
					aC = core->GetGame()->GetMap( 0 )->tm->GetInfoPoint( oC->objectName );
					if (!aC) //No... ok probably this object does not exist... just return NULL
						return NULL;
				}
			}
			//Ok :) we now have our Object. Let's create a Target struct and add the object to it
			Targets* tgts = new Targets();
			tgts->AddTarget( aC );
			return tgts;
		} else {
			//Here is the hard part...
			bool Anything = true;
			for (int i = 0; i < ObjectFieldsCount; i++) {
				//Just check the Fields
				if (oC->objectFields[i]) {
					//If the field is set we need to evaluate it
					Anything = false;
					ObjectFunction func = fieldFunctions[i][oC->objectFields[i]];
					if (func) {
						//If we support that function, just _use_ it :)
						/* This function will evaluate the parameter */
						parm = func( parm );
					} else {
						//ARGH!!!! We cannot handle that function.... it's better to return NULL
						if (parm)
							delete( parm );
						return NULL;
					}
				}
			}
			if (Anything && !parm) {
				//If we need to operate on everything we just return an empty Targets struct
				return new Targets();
			}
			//Ok, now we just need to return parm
			return parm;
		}
	} else {
		//At this point we only need to call the Object Functions on the actual Targets parameter
		ObjectFunction func = objects[oC->objectIdentifiers[IDIndex]];
		if (!func) {
			//ARGH!!!! We need to implement this object opcode... I think I'll return NULL....
			if (parm)
				delete( parm );
			return NULL;
		}
		//Good! We can now call the function
		parm = func( parm );
	}
}

Targets* GameScript::GetActorFromObject(Scriptable* Sender, Object* oC)
{
	if (!oC) {
		return NULL;
	}
	Targets* tgts = NULL;
	for (int i = -1; i < 5; i++) {
		tgts = EvaluateObject( Sender, oC, i, tgts );
		if (!tgts) {
			//ARGH!!!! An Error occourred while processing this object... better return NULL
			return NULL;
		}
	}
	return tgts;
}

unsigned char GameScript::GetOrient(short sX, short sY, short dX, short dY)
{
	short deltaX = ( dX- sX), deltaY = ( dY - sY );
	if (deltaX > 0) {
		if (deltaY > 0) {
			return 6;
		} else if (deltaY == 0) {
			return 4;
		} else {
			return 2;
		}
	} else if (deltaX == 0) {
		if (deltaY > 0) {
			return 8;
		} else {
			return 0;
		}
	} else {
		if (deltaY > 0) {
			return 10;
		} else if (deltaY == 0) {
			return 12;
		} else {
			return 14;
		}
	}
	return 0;
}

void GameScript::ExecuteString(Scriptable* Sender, char* String)
{
	if (String[0] == 0) {
		return;
	}
	Action* act = GenerateAction( String );
	if (!act) {
		return;
	}
	if (Sender->CurrentAction) {
		Sender->CurrentAction->Release();
	}
	Sender->CurrentAction = act;
	ExecuteAction( Sender, act );
	return;
}

bool GameScript::EvaluateString(Scriptable* Sender, char* String)
{
	if (String[0] == 0) {
		return false;
	}
	Trigger* tri = GenerateTrigger( String );
	bool ret = EvaluateTrigger( Sender, tri );
	tri->Release();
	return ret;
}

Action* GameScript::GenerateAction(char* String)
{
	strlwr( String );
	Action* newAction = NULL;
	int i = 0;
	while (true) {
		char* src = String;
		char* str = actionsTable->GetStringIndex( i );
		if (!str)
			return newAction;
		while (*str) {
			if (*str != *src)
				break;
			if (*str == '(') {
				newAction = new Action();
				newAction->actionID = ( unsigned short )
					actionsTable->GetValueIndex( i );
				int objectCount = ( newAction->actionID == 1 ) ? 0 : 1;
				int stringsCount = 0;
				int intCount = 0;
				//Here is the Trigger; Now we need to evaluate the parameters
				str++;
				src++;
				while (*str) {
					switch (*str) {
						default:
							str++;
							break;

						case 'p':
							//Point
							 {
								while (( *str != ',' ) && ( *str != ')' ))
									str++;
								src++; //Skip [
								char symbol[33];
								char* tmp = symbol;
								while (( ( *src >= '0' ) && ( *src <= '9' ) ) ||
									( *src == '-' )) {
									*tmp = *src;
									tmp++;
									src++;
								}
								*tmp = 0;
								newAction->XpointParameter = atoi( symbol );
								src++; //Skip .
								tmp = symbol;
								while (( ( *src >= '0' ) && ( *src <= '9' ) ) ||
									( *src == '-' )) {
									*tmp = *src;
									tmp++;
									src++;
								}
								*tmp = 0;
								newAction->YpointParameter = atoi( symbol );
								src++; //Skip ]
							}
							break;

						case 'i':
							//Integer
							 {
								while (*str != '*')
									str++;
								str++;
								SymbolMgr* valHook = NULL;
								if (( *str != ',' ) && ( *str != ')' )) {
									char idsTabName[33];
									char* tmp = idsTabName;
									while (( *str != ',' ) && ( *str != ')' )) {
										*tmp = *str;
										tmp++;
										str++;
									}
									*tmp = 0;
									int i = core->LoadSymbol( idsTabName );
									valHook = core->GetSymbol( i );
								}
								if (!valHook) {
									char symbol[33];
									char* tmp = symbol;
									while (( ( *src >= '0' ) &&
										( *src <= '9' ) ) ||
										( *src == '-' )) {
										*tmp = *src;
										tmp++;
										src++;
									}
									*tmp = 0;
									if (!intCount) {
										newAction->int0Parameter = atoi( symbol );
									} else if (intCount == 1) {
										newAction->int1Parameter = atoi( symbol );
									} else {
										newAction->int2Parameter = atoi( symbol );
									}
								} else {
									char symbol[33];
									char* tmp = symbol;
									while (( *src != ',' ) && ( *src != ')' )) {
										*tmp = *src;
										tmp++;
										src++;
									}
									*tmp = 0;
									if (!intCount) {
										newAction->int0Parameter = valHook->GetValue( symbol );
									} else if (intCount == 1) {
										newAction->int1Parameter = valHook->GetValue( symbol );
									} else {
										newAction->int2Parameter = valHook->GetValue( symbol );
									}
								}
							}
							break;

						case 'a':
							//Action
							 {
								while (( *str != ',' ) && ( *str != ')' ))
									str++;
								char action[257];
								int i = 0;
								int openParentesisCount = 0;
								while (true) {
									if (*src == ')') {
										if (!openParentesisCount)
											break;
										openParentesisCount--;
									} else {
										if (*src == '(') {
											openParentesisCount++;
										} else {
											if (( *src == ',' ) &&
												!openParentesisCount)
												break;
										}
									}
									action[i] = *src;
									i++;
									src++;
								}
								action[i] = 0;
								Action* act = GenerateAction( action );
								act->objects[0] = newAction->objects[0];
								act->objects[0]->IncRef();
								newAction->Release();
								newAction = act;
							}
							break;

						case 'o':
							//Object
							 {
								while (( *str != ',' ) && ( *str != ')' ))
									str++;
								if (*src == '"') {
									//Object Name
									src++;
									newAction->objects[objectCount] = new Object();
									char* dst = newAction->objects[objectCount]->objectName;
									while (*src != '"') {
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

						case 's':
							//String
							 {
								while (( *str != ',' ) && ( *str != ')' ))
									str++;
								src++;
								char* dst;
								if (!stringsCount) {
									dst = newAction->string0Parameter;
								} else {
									dst = newAction->string1Parameter;
								}
								while (*src != '"') {
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
					while (( *src == ',' ) || ( *src == ' ' ))
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

Trigger* GameScript::GenerateTrigger(char* String)
{
	strlwr( String );
	Trigger* newTrigger = NULL;
	bool negate = false;
	if (*String == '!') {
		String++;
		negate = true;
	}
	int i = 0;
	while (true) {
		char* src = String;
		char* str = triggersTable->GetStringIndex( i );
		if (!str)
			return newTrigger;
		while (*str) {
			if (*str != *src)
				break;
			if (*str == '(') {
				newTrigger = new Trigger();
				newTrigger->triggerID = ( unsigned short )
					( triggersTable->GetValueIndex( i ) & 0xff );
				newTrigger->flags = ( negate ) ?
					( unsigned short ) 1 :
					( unsigned short ) 0;
				int stringsCount = 0;
				int intCount = 0;
				//Here is the Trigger; Now we need to evaluate the parameters
				str++;
				src++;
				while (*str) {
					switch (*str) {
						default:
							str++;
							break;

						case 'p':
							//Point
							 {
								while (( *str != ',' ) && ( *str != ')' ))
									str++;
								src++; //Skip [
								char symbol[33];
								char* tmp = symbol;
								while (( *src >= '0' ) && ( *src <= '9' )) {
									*tmp = *src;
									tmp++;
									src++;
								}
								*tmp = 0;
								newTrigger->XpointParameter = atoi( symbol );
								src++; //Skip .
								tmp = symbol;
								while (( *src >= '0' ) && ( *src <= '9' )) {
									*tmp = *src;
									tmp++;
									src++;
								}
								*tmp = 0;
								newTrigger->YpointParameter = atoi( symbol );
								src++; //Skip ]
							}
							break;

						case 'i':
							//Integer
							 {
								while (*str != '*')
									str++;
								str++;
								SymbolMgr* valHook = NULL;
								if (( *str != ',' ) && ( *str != ')' )) {
									char idsTabName[33];
									char* tmp = idsTabName;
									while (( *str != ',' ) && ( *str != ')' )) {
										*tmp = *str;
										tmp++;
										str++;
									}
									*tmp = 0;
									int i = core->LoadSymbol( idsTabName );
									valHook = core->GetSymbol( i );
								}
								if (!valHook) {
									char symbol[33];
									char* tmp = symbol;
									while (( *src >= '0' ) && ( *src <= '9' )) {
										*tmp = *src;
										tmp++;
										src++;
									}
									*tmp = 0;
									if (!intCount) {
										newTrigger->int0Parameter = atoi( symbol );
									} else if (intCount == 1) {
										newTrigger->int1Parameter = atoi( symbol );
									} else {
										newTrigger->int2Parameter = atoi( symbol );
									}
								} else {
									char symbol[33];
									char* tmp = symbol;
									while (( *src != ',' ) && ( *src != ')' )) {
										*tmp = *src;
										tmp++;
										src++;
									}
									*tmp = 0;
									if (!intCount) {
										newTrigger->int0Parameter = valHook->GetValue( symbol );
									} else if (intCount == 1) {
										newTrigger->int1Parameter = valHook->GetValue( symbol );
									} else {
										newTrigger->int2Parameter = valHook->GetValue( symbol );
									}
								}
								intCount++;
							}
							break;

						case 'o':
							//Object
							 {
								while (( *str != ',' ) && ( *str != ')' ))
									str++;
								if (*src == '"') {
									//Object Name
									src++;
									newTrigger->objectParameter = new Object();
									char* dst = newTrigger->objectParameter->objectName;
									while (*src != '"') {
										*dst = *src;
										dst++;
										src++;
									}
									*dst = 0;
									src++;
								} else {
									textcolor( LIGHT_RED );
									printf( "[GenerateTrigger]: OBJECT TYPE NOT SUPPORTED\n" );
									textcolor( WHITE );
								}
							}
							break;

						case 's':
							//String
							 {
								while (( *str != ',' ) && ( *str != ')' ))
									str++;
								src++;
								char* dst;
								if (!stringsCount) {
									dst = newTrigger->string0Parameter;
								} else {
									dst = newTrigger->string1Parameter;
								}
								while (*src != '"') {
									*dst = *src;
									dst++;
									src++;
								}
								src++;
								stringsCount++;
							}
							break;
					}
					while (( *src == ',' ) || ( *src == ' ' ))
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

int GameScript::Alignment(Scriptable* Sender, Trigger* parameters)
{
	Targets* targets = GetActorFromObject( Sender,
						parameters->objectParameter );
	if (!targets) {
		return;
	}
	if (!targets->Count()) {
		delete( targets );
		return;
	}
	for (int i = 0; i < targets->Count(); i++) {
		Scriptable* scr = targets->GetTarget( i );
		if (scr->Type != ST_ACTOR) {
			continue;
		}
		Actor* actor = ( Actor* ) scr;
		int value = actor->GetStat( IE_ALIGNMENT );
		int a = parameters->int0Parameter&15;
		if (a) {
			if (a != ( value & 15 )) {
				delete( targets );
				return 0;
			}
		}

		a = parameters->int0Parameter & 240;
		if (a) {
			if (a != ( value & 240 )) {
				delete( targets );
				return 0;
			}
		}
	}
	return 1;
	/*Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	int value = actor->GetStat( IE_ALIGNMENT );
	int a = parameters->int0Parameter&15;
	if (a) {
		if (a != ( value & 15 )) {
			return 0;
		}
	}
	a = parameters->int0Parameter & 240;
	if (a) {
		if (a != ( value & 240 )) {
			return 0;
		}
	}
	return 1;*/
}

int GameScript::Allegiance(Scriptable* Sender, Trigger* parameters)
{
	Targets* targets = GetActorFromObject( Sender,
						parameters->objectParameter );
	if (!targets) {
		return false;
	}
	if (!targets->Count()) {
		delete( targets );
		return false;
	}
	bool ret = true;
	for (int i = 0; i < targets->Count(); i++) {
		Scriptable* scr = targets->GetTarget( i );
		if (scr->Type != ST_ACTOR) {
			continue;
		}
		Actor* actor = ( Actor* ) scr;
		int value = actor->GetStat( IE_EA );
		switch (parameters->int0Parameter) {
			case 30:
				//goodcutoff
				ret &= value <= 30;
				break;

			case 31:
				//notgood
				ret &= value >= 31;
				break;

			case 199:
				//notevil
				ret &= value <= 199;
				break;

			case 200:
				//evilcutoff
				ret &= value >= 200;
				break;

			case 0:
			case 126:
				//anything
				ret &= true;
				break;

			default:
				ret &= parameters->int0Parameter == value;
				break;
		}
		if (!ret) {
			delete( targets );
			return false;
		}
	}
	delete( targets );
	return true;
}

int GameScript::Class(Scriptable* Sender, Trigger* parameters)
{
	Targets* targets = GetActorFromObject( Sender,
						parameters->objectParameter );
	if (!targets) {
		return false;
	}
	if (!targets->Count()) {
		delete( targets );
		return false;
	}
	bool ret = true;
	for (int i = 0; i < targets->Count(); i++) {
		Scriptable* scr = targets->GetTarget( i );
		if (scr->Type != ST_ACTOR) {
			continue;
		}
		Actor* actor = ( Actor* ) scr;
		//TODO: if parameter >=202, it is of *_ALL type
		ret &= ( parameters->int0Parameter == actor->GetStat( IE_CLASS ) );
		if (!ret) {
			delete( targets );
			return false;
		}
	}
	delete( targets );
	return true;
}

//atm this checks for InParty and See, it is unsure what is required
int GameScript::IsValidForPartyDialog(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* actor = GetActorFromObject( Sender,
							parameters->objectParameter );
	if (actor == NULL) {
		return 0;
	}
	//non actors got no visual range, needs research
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	Actor* snd = ( Actor* ) Sender;
	if (actor->Type != ST_ACTOR) {
		return 0;
	}
	//return actor->InParty?1:0; //maybe ???
	if (!core->GetGame()->InParty( ( Actor * ) actor )) {
		return 0;
	}
	long x = ( actor->XPos - Sender->XPos );
	long y = ( actor->YPos - Sender->YPos );
	double distance = sqrt( ( double ) ( x* x + y* y ) );
	if (distance > ( snd->Modified[IE_VISUALRANGE] * 20 )) {
		return 0;
	}
	if (!core->GetPathFinder()->IsVisible( Sender->XPos, Sender->YPos,
									actor->XPos, actor->YPos )) {
		return 0;
	}
	//further checks, is target alive and talkative
	return 1;
}

int GameScript::InParty(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* actor = GetActorFromObject( Sender,
							parameters->objectParameter );
	if (actor == NULL) {
		return 0;
	}
	if (actor->Type != ST_ACTOR) {
		return 0;
	}
	//return actor->InParty?1:0; //maybe ???
	return core->GetGame()->InParty( ( Actor * ) actor ) >= 0 ? 1 : 0;
}

int GameScript::Exists(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* actor = GetActorFromObject( Sender,
							parameters->objectParameter );
	if (actor == NULL) {
		return 0;
	}
	return 1;
}

int GameScript::General(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	if (actor == NULL) {
		return 0;
	}
	return parameters->int0Parameter == actor->GetStat( IE_GENERAL );
}

int GameScript::BitCheck(Scriptable* Sender, Trigger* parameters)
{
	unsigned long value = CheckVariable( Sender, parameters->string0Parameter );
	int eval = ( value& parameters->int0Parameter ) ? 1 : 0;
	return eval;
}

int GameScript::Global(Scriptable* Sender, Trigger* parameters)
{
	unsigned long value = CheckVariable( Sender, parameters->string0Parameter );
	int eval = ( value == parameters->int0Parameter ) ? 1 : 0;
	return eval;
}

int GameScript::GlobalLT(Scriptable* Sender, Trigger* parameters)
{
	unsigned long value = CheckVariable( Sender, parameters->string0Parameter );
	int eval = ( value < parameters->int0Parameter ) ? 1 : 0;
	return eval;
}

int GameScript::GlobalGT(Scriptable* Sender, Trigger* parameters)
{
	unsigned long value = CheckVariable( Sender, parameters->string0Parameter );
	int eval = ( value > parameters->int0Parameter ) ? 1 : 0;
	return eval;
}

int GameScript::GlobalsEqual(Scriptable* Sender, Trigger* parameters)
{
	unsigned long value1 = CheckVariable( Sender,
							parameters->string0Parameter );
	unsigned long value2 = CheckVariable( Sender,
							parameters->string1Parameter );
	int eval = ( value1 == value2 ) ? 1 : 0;
	return eval;
}

int GameScript::OnCreation(Scriptable* Sender, Trigger* parameters)
{
	Map* area = core->GetGame()->GetMap( 0 );
	if (area->justCreated) {
		area->justCreated = false;
		return 1;
	}
	return 0;
}

int GameScript::PartyHasItem(Scriptable * /*Sender*/, Trigger* parameters)
{
	/*hacked to never have the item, this requires inventory!*/
	if (stricmp( parameters->string0Parameter, "MISC4G" ) == 0) {
		return 1;
	}
	return 0;
}

int GameScript::True(Scriptable * /* Sender*/, Trigger * /*parameters*/)
{
	return 1;
}

//in fact this could be used only on Sender, but we want to enhance these
//triggers and actions to accept an object argument whenever possible.
//0 defaults to Myself (Sender)
int GameScript::NumTimesTalkedTo(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	return actor->TalkCount == parameters->int0Parameter ? 1 : 0;
}

int GameScript::NumTimesTalkedToGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	return actor->TalkCount > parameters->int0Parameter ? 1 : 0;
}

int GameScript::NumTimesTalkedToLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	return actor->TalkCount < parameters->int0Parameter ? 1 : 0;
}

int GameScript::ActionListEmpty(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	if (scr->GetNextAction()) {
		return 0;
	}
	return 1;
}

int GameScript::False(Scriptable * /*Sender*/, Trigger * /*parameters*/)
{
	return 0;
}

int GameScript::Range(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* target = GetActorFromObject( Sender,
							parameters->objectParameter );
	if (!target) {
		return 0;
	}
	printf( "x1 = %d, y1 = %d\nx2 = %d, y2 = %d\n", target->XPos,
		target->YPos, Sender->XPos, Sender->YPos );
	long x = ( target->XPos - Sender->XPos );
	long y = ( target->YPos - Sender->YPos );
	double distance = sqrt( ( double ) ( x* x + y* y ) );
	printf( "Distance = %.3f\n", distance );
	if (distance <= ( parameters->int0Parameter * 20 )) {
		return 1;
	}
	return 0;
}

int GameScript::Or(Scriptable* Sender, Trigger* parameters)
{
	return parameters->int0Parameter;
}

int GameScript::Clicked(Scriptable* Sender, Trigger* parameters)
{
	/*
	if (parameters->objectParameter->eaField == 0) {
		return 1;
	Scriptable* target = GetActorFromObject( Sender,
							parameters->objectParameter );
	if (Sender == target)
		return 1;
	}*/
	Targets* targets = GetActorFromObject( Sender,
						parameters->objectParameter );
	if (!targets) {
		return 0;
	}
	if (!targets->Count()) {
		//Anyone
		delete( targets );
		return 1;
	}
	//Let's check if the clicker is in the targets list
	if (targets->Contains( Sender )) {
		delete( targets );
		return 1;
	}

	delete( targets );
	return 0;
}

int GameScript::Entered(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_PROXIMITY) {
		return 0;
	}
	/*if (parameters->objectParameter->eaField == 0) {
		if (Sender->LastEntered) {
			return 1;
		} else {
			return 0;
		}
	}
	Scriptable* target = GetActorFromObject( Sender,
							parameters->objectParameter );
	if (Sender->LastEntered == target) {
		return 1;
	}
	return 0;*/
	Targets* targets = GetActorFromObject( Sender,
						parameters->objectParameter );
	if (!targets) {
		return 0;
	}
	if (!targets->Count()) {
		delete( targets );
		if (Sender->LastEntered) {
			return 1;
		} else {
			return 0;
		}
	}
	if (targets->Contains( Sender->LastEntered )) {
		delete( targets );
		return 1;
	}
	delete( targets );
	return 0;
}

//this function is different in every engines, if you use a string0parameter
//then it will be considered as a variable check
//you can also use an object parameter (like in iwd)
int GameScript::Dead(Scriptable* Sender, Trigger* parameters)
{
	if (parameters->string0Parameter) {
		char Variable[40];
		if (core->HasFeature( GF_HAS_KAPUTZ )) {
			sprintf( Variable, "KAPUTZ%32.32s", parameters->string0Parameter );
		} else {
			sprintf( Variable, "GLOBALSPRITE_IS_DEAD%18.18s",
				parameters->string0Parameter );
		}
		return CheckVariable( Sender, Variable ) > 0 ? 1 : 0;
	}
	Targets* targets = GetActorFromObject( Sender,
						parameters->objectParameter );
	if (!targets) {
		return 0;
	}
	if (!targets->Count()) {
		delete( targets );
		return 0;
	}
	Scriptable* target = targets->GetTarget( 0 );
	/*Scriptable* target = GetActorFromObject( Sender,
							parameters->objectParameter );
	if (!target) {
		return 0;
	}*/
	if (target->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) target;
	if (actor->GetStat( IE_STATE_ID ) & STATE_DEAD) {
		return 1;
	}
	return 0;
}

int GameScript::Race(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	if (actor->GetStat( IE_RACE ) == parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::Gender(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	if (actor->GetStat( IE_SEX ) == parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::HP(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	if (actor->GetStat( IE_HITPOINTS ) == parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::HPGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	if (actor->GetStat( IE_HITPOINTS ) > parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::HPLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	if (actor->GetStat( IE_HITPOINTS ) < parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::XP(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	if (actor->GetStat( IE_XP ) == parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::XPGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	if (actor->GetStat( IE_XP ) > parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::XPLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	if (actor->GetStat( IE_XP ) < parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::CheckStat(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* target = GetActorFromObject( Sender,
							parameters->objectParameter );
	if (!target) {
		return 0;
	}
	if (target->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) target;
	if (actor->GetStat( parameters->int0Parameter ) ==
		parameters->int1Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::CheckStatGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* target = GetActorFromObject( Sender,
							parameters->objectParameter );
	if (!target) {
		return 0;
	}
	if (target->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) target;
	if (actor->GetStat( parameters->int0Parameter ) >
		parameters->int1Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::CheckStatLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* target = GetActorFromObject( Sender,
							parameters->objectParameter );
	if (!target) {
		return 0;
	}
	if (target->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) target;
	if (actor->GetStat( parameters->int0Parameter ) <
		parameters->int1Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::See(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	Actor* snd = ( Actor* ) Sender;
	Scriptable* target = GetActorFromObject( Sender,
							parameters->objectParameter );
	if (!target) {
		return 0;
	}
	long x = ( target->XPos - Sender->XPos );
	long y = ( target->YPos - Sender->YPos );
	double distance = sqrt( ( double ) ( x* x + y* y ) );
	if (distance > ( snd->Modified[IE_VISUALRANGE] * 20 )) {
		return 0;
	}
	if (core->GetPathFinder()->IsVisible( Sender->XPos, Sender->YPos,
								target->XPos, target->YPos )) {
		return 1;
	}
	return 0;
}

//-------------------------------------------------------------
// Action Functions
//-------------------------------------------------------------

void GameScript::NoAction(Scriptable */*Sender*/, Action */*parameters*/)
{
	//thats all :)
}

void GameScript::SG(Scriptable* Sender, Action* parameters)
{
	SetVariable( Sender, "GLOBAL", parameters->string0Parameter,
		parameters->int0Parameter );
}

void GameScript::SetGlobal(Scriptable* Sender, Action* parameters)
{
	printf( "SetGlobal(\"%s\", %d)\n", parameters->string0Parameter,
		parameters->int0Parameter );
	SetVariable( Sender, parameters->string0Parameter,
		parameters->int0Parameter );
}

void GameScript::ChangeAllegiance(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[1] );
	if (!scr) {
		return;
	}
	if (scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->SetStat( IE_EA, parameters->int0Parameter );
}

void GameScript::ChangeGeneral(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[1] );
	if (!scr) {
		return;
	}
	if (scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->SetStat( IE_GENERAL, parameters->int0Parameter );
}

void GameScript::ChangeRace(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[1] );
	if (!scr) {
		return;
	}
	if (scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->SetStat( IE_RACE, parameters->int0Parameter );
}

void GameScript::ChangeClass(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[1] );
	if (!scr) {
		return;
	}
	if (scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->SetStat( IE_CLASS, parameters->int0Parameter );
}

void GameScript::ChangeSpecifics(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[1] );
	if (!scr) {
		return;
	}
	if (scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->SetStat( IE_SPECIFIC, parameters->int0Parameter );
}

void GameScript::ChangeStat(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[1] );
	if (!scr) {
		return;
	}
	if (scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->NewStat( parameters->int0Parameter, parameters->int1Parameter,
			parameters->int2Parameter );
}

void GameScript::ChangeGender(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[1] );
	if (!scr) {
		return;
	}
	if (scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->SetStat( IE_SEX, parameters->int0Parameter );
}

void GameScript::ChangeAlignment(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[1] );
	if (!scr) {
		return;
	}
	if (scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->SetStat( IE_ALIGNMENT, parameters->int0Parameter );
}

void GameScript::TriggerActivation(Scriptable* Sender, Action* parameters)
{
	Targets* targets = GetActorFromObject( Sender, parameters->objects[1] );
	if (!targets) {
		return;
	}
	if (!targets->Count()) {
		delete( targets );
		return;
	}
	Scriptable* ip = targets->GetTarget( 0 );
	delete( targets );
	/*if (parameters->objects[1]->genderField != 0) {
		switch (parameters->objects[1]->genderField) {
			case 1:
				ip = Sender;
				break;
		}
	} else {
		ip = core->GetGame()->GetMap( 0 )->tm->GetInfoPoint( parameters->objects[1]->objectName );
	}*/
	if (!ip) {
		printf( "Script error: No Trigger Named \"%s\"\n",
			parameters->objects[1]->objectName );
		return;
	}
	ip->Active = parameters->int0Parameter;
}

void GameScript::FadeToColor(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		return;
	}
	core->timer->SetFadeToColor( parameters->XpointParameter );
	//Sender->SetWait(parameters->XpointParameter);
}

void GameScript::FadeFromColor(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		return;
	}
	core->timer->SetFadeFromColor( parameters->XpointParameter );
}

void GameScript::JumpToPoint(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		return;
	}
	if (scr->Type != ST_ACTOR) {
		return;
	}
	Actor* ab = ( Actor* ) scr;
	ab->SetPosition( parameters->XpointParameter, parameters->YpointParameter );
}

void GameScript::JumpToPointInstant(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		return;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	if (tar->Type != ST_ACTOR) {
		return;
	}
	Actor* ab = ( Actor* ) tar;
	ab->SetPosition( parameters->XpointParameter, parameters->YpointParameter );
}

void GameScript::JumpToObject(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		return;
	}
	if (scr->Type != ST_ACTOR) {
		return;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );

	Actor* ab = ( Actor* ) scr;
	ab->SetPosition( tar->XPos, tar->YPos );
}

void GameScript::CreateCreatureCore(Scriptable* Sender, Action* parameters,
	int flags)
{
	ActorMgr* aM = ( ActorMgr* ) core->GetInterface( IE_CRE_CLASS_ID );
	DataStream* ds = core->GetResourceMgr()->GetResource( parameters->string0Parameter,
												IE_CRE_CLASS_ID );
	aM->Open( ds, true );
	Actor* ab = aM->GetActor();
	int x = parameters->XpointParameter;
	int y = parameters->YpointParameter;
	if (flags & CC_OFFSET) {
		x += Sender->XPos;
		y += Sender->YPos;
	}
	ab->SetPosition( parameters->XpointParameter, parameters->YpointParameter );
	ab->AnimID = IE_ANI_AWAKE;
	ab->Orientation = parameters->int0Parameter;
	Map* map = core->GetGame()->GetMap( 0 );
	map->AddActor( ab );
	core->FreeInterface( aM );
}

void GameScript::CreateCreature(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		return;
	}
	CreateCreatureCore( Sender, parameters, 0 );
}

void GameScript::CreateCreatureObject(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		return;
	}
	CreateCreatureCore( Sender, parameters, CC_OFFSET );
}

void GameScript::StartCutSceneMode(Scriptable* Sender, Action* parameters)
{
	core->SetCutSceneMode( true );
}

void GameScript::EndCutSceneMode(Scriptable* Sender, Action* parameters)
{
	core->SetCutSceneMode( false );
}

void GameScript::StartCutScene(Scriptable* Sender, Action* parameters)
{
	GameScript* gs = new GameScript( parameters->string0Parameter,
							IE_SCRIPT_ALWAYS );
	gs->MySelf = Sender;
	gs->EvaluateAllBlocks();
	delete( gs );
}

void GameScript::CutSceneID(Scriptable* Sender, Action* parameters)
{
	Targets* targets = GetActorFromObject( Sender, parameters->objects[1] );
	if (!targets) {
		return;
	}
	if (!targets->Count()) {
		delete( targets );
		return;
	}
	Sender->CutSceneId = targets->GetTarget( 0 );
	delete( targets );
	/*if (parameters->objects[1]->genderField != 0) {
		Sender->CutSceneId = GetActorFromObject( Sender,
								parameters->objects[1] );
	} else {
		Map* map = core->GetGame()->GetMap( 0 );
		Sender->CutSceneId = map->GetActor( parameters->objects[1]->objectName );
	}*/
}

void GameScript::Enemy(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr->Type != ST_ACTOR) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		//Sender->CurrentAction = NULL;
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->SetStat( IE_EA, 255 );
}

void GameScript::Ally(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr->Type != ST_ACTOR) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->SetStat( IE_EA, 4 );
}

void GameScript::ChangeAIScript(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	//changeaiscript clears the queue, i believe
	//	actor->ClearActions();
	actor->SetScript( parameters->string0Parameter, parameters->int0Parameter );
}

void GameScript::Wait(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		/* 
								We need to NULL the CurrentAction because this is a blocking
								OpCode.
							*/
		Sender->CurrentAction = NULL;
		return;
	}
	Sender->SetWait( parameters->int0Parameter * AI_UPDATE_TIME );
}

void GameScript::SmallWait(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		/* 
								We need to NULL the CurrentAction because this is a blocking
								OpCode.
							*/
		Sender->CurrentAction = NULL;
		return;
	}
	Sender->SetWait( parameters->int0Parameter );
}

void GameScript::MoveViewPoint(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		return;
	}
	core->GetVideoDriver()->MoveViewportTo( parameters->XpointParameter,
								parameters->YpointParameter );
}

void GameScript::MoveViewObject(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[1] );
	if (!scr) {
		return;
	}
	core->GetVideoDriver()->MoveViewportTo( scr->XPos, scr->YPos );
}

void GameScript::MoveToPoint(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr->Type != ST_ACTOR) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		/* 
								We need to NULL the CurrentAction because this is a blocking
								OpCode.
							*/
		Sender->CurrentAction = NULL;
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->WalkTo( parameters->XpointParameter, parameters->YpointParameter );
	//core->timer->SetMovingActor(actor);
}

void GameScript::MoveToObject(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		Sender->CurrentAction = NULL;
		return;
	}
	Scriptable* target = GetActorFromObject( Sender, parameters->objects[1] );
	if (!target) {
		Sender->CurrentAction = NULL;
		return;
	}
	if (scr->Type != ST_ACTOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		/* 
								We need to NULL the CurrentAction because this is a blocking
								OpCode.
							*/
		Sender->CurrentAction = NULL;
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->WalkTo( target->XPos, target->YPos );
}

void GameScript::DisplayStringHead(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr->Type != ST_ACTOR) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		return;
	}
	Actor* actor = ( Actor* ) scr;
	if (actor) {
		printf( "Displaying string on: %s\n", actor->scriptName );
		actor->DisplayHeadText( core->GetString( parameters->int0Parameter, 2 ) );
	}
}

void GameScript::Face(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		Sender->CurrentAction = NULL;
		return;
	}
	if (scr->Type != ST_ACTOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		/* 
								We need to NULL the CurrentAction because this is a blocking
								OpCode.
							*/
		Sender->CurrentAction = NULL;
		return;
	}
	Actor* actor = ( Actor* ) scr;
	if (actor) {
		actor->Orientation = parameters->int0Parameter;
		actor->resetAction = true;
		actor->SetWait( 1 );
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

void GameScript::FaceObject(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		Sender->CurrentAction = NULL;
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		Sender->CurrentAction = NULL;
		return;
	}
	Scriptable* target = GetActorFromObject( Sender, parameters->objects[1] );
	if (!target) {
		return;
	}
	if (scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	if (actor) {
		actor->Orientation = GetOrient( target->XPos, target->YPos,
								actor->XPos, actor->YPos );
		actor->resetAction = true;
		actor->SetWait( 1 );
	} else {
		Sender->CurrentAction = NULL;
	}
}

void GameScript::DisplayStringWait(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr->Type != ST_ACTOR) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		/* 
								We need to NULL the CurrentAction because this is a blocking
								OpCode.
							*/
		Sender->CurrentAction = NULL;
		return;
	}
	Actor* actor = ( Actor* ) scr;
	printf( "Displaying string on: %s\n", actor->scriptName );
	StringBlock sb = core->strings->GetStringBlock( parameters->int0Parameter );
	actor->DisplayHeadText( sb.text );
	if (sb.Sound[0]) {
		unsigned long len = core->GetSoundMgr()->Play( sb.Sound );
		if (len != 0)
			actor->SetWait( ( AI_UPDATE_TIME * len ) / 1000 );
	}
}

void GameScript::StartSong(Scriptable* Sender, Action* parameters)
{
	int MusicTable = core->LoadTable( "music" );
	if (MusicTable >= 0) {
		TableMgr* music = core->GetTable( MusicTable );
		char* string = music->QueryField( parameters->int0Parameter, 0 );
		if (string[0] == '*') {
			core->GetMusicMgr()->HardEnd();
		} else {
			core->GetMusicMgr()->SwitchPlayList( string, true );
		}
	}
}

void GameScript::Continue(Scriptable* Sender, Action* parameters)
{
}

void GameScript::PlaySound(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		return;
	}
	printf( "PlaySound(%s)\n", parameters->string0Parameter );
	core->GetSoundMgr()->Play( parameters->string0Parameter, scr->XPos,
							scr->YPos );
}

void GameScript::CreateVisualEffectObject(Scriptable* Sender,
	Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		return;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	DataStream* ds = core->GetResourceMgr()->GetResource( parameters->string0Parameter,
												IE_VVC_CLASS_ID );
	ScriptedAnimation* vvc = new ScriptedAnimation( ds, true, tar->XPos,
									tar->YPos );
	core->GetGame()->GetMap( 0 )->AddVVCCell( vvc );
}

void GameScript::CreateVisualEffect(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		return;
	}
	DataStream* ds = core->GetResourceMgr()->GetResource( parameters->string0Parameter,
												IE_VVC_CLASS_ID );
	ScriptedAnimation* vvc = new ScriptedAnimation( ds, true,
									parameters->XpointParameter,
									parameters->YpointParameter );
	core->GetGame()->GetMap( 0 )->AddVVCCell( vvc );
}

void GameScript::DestroySelf(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr->Type != ST_ACTOR) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->DeleteMe = true;
}

void GameScript::ScreenShake(Scriptable* Sender, Action* parameters)
{
	core->timer->SetScreenShake( parameters->XpointParameter,
					parameters->YpointParameter, parameters->int0Parameter );
	Sender->SetWait( parameters->int0Parameter );
}

void GameScript::UnhideGUI(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		return;
	}
	GameControl* gc = ( GameControl* ) core->GetWindow( 0 )->GetControl( 0 );	
	if (gc->ControlType == IE_GUI_GAMECONTROL) {
		gc->UnhideGUI();
	}
	EndCutSceneMode( Sender, parameters );
}

void GameScript::HideGUI(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		return;
	}
	GameControl* gc = ( GameControl* ) core->GetWindow( 0 )->GetControl( 0 );	
	if (gc->ControlType == IE_GUI_GAMECONTROL) {
		gc->HideGUI();
	}
}

static char PlayerDialogRes[9] = "PLAYERx\0";

void GameScript::BeginDialog(Scriptable* Sender, Action* parameters, int Flags)
{
	Scriptable* tar, * scr;

	if (Flags & BD_OWN) {
		Targets* targets = GetActorFromObject( Sender, parameters->objects[1] );
		if (!targets)
			return;
		if (!targets->Count()) {
			delete( targets );
			return;
		}
		scr = tar = targets->GetTarget( 0 );
		delete( targets );
		//scr = tar = GetActorFromObject( Sender, parameters->objects[1] );
	} else {
		Targets* targets = GetActorFromObject( Sender, parameters->objects[0] );
		if (!targets)
			return;
		if (!targets->Count()) {
			delete( targets );
			return;
		}
		scr = targets->GetTarget( 0 );
		delete( targets );
		//scr = GetActorFromObject( Sender, parameters->objects[0] );
		//if (!scr)
		//	return;
		if (scr != Sender) {
			//this is an Action Override
			scr->AddAction( Sender->CurrentAction );
			Sender->CurrentAction = NULL;
			return;
		}
		targets = GetActorFromObject( Sender, parameters->objects[0] );
		if (!targets)
			return;
		if (!targets->Count()) {
			delete( targets );
			return;
		}
		tar = targets->GetTarget( 0 );
		//tar = GetActorFromObject( Sender, parameters->objects[1] );
	}
	//source could be other than Actor, we need to handle this too!
	if (scr->Type != ST_ACTOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	//no need to check these for NULL
	Actor* actor = ( Actor* ) scr;
	Actor* target = ( Actor* ) tar;

	GameControl* gc = ( GameControl* ) core->GetWindow( 0 )->GetControl( 0 );	
	if (gc->ControlType != IE_GUI_GAMECONTROL) {
		printf( "[IEScript]: Dialog cannot be initiated because there is no GameControl.\n" );
		Sender->CurrentAction = NULL;
		return;
	}
	//can't initiate dialog, because it is already there
	if (gc->Dialogue) {
		if (Flags & BD_INTERRUPT) {
			//break the current dialog if possible
			gc->EndDialog();
		}
		//check if we could manage to break it, not all dialogs are breakable!
		if (gc->Dialogue) {
			printf( "[IEScript]: Dialog cannot be initiated because there is already one.\n" );
			Sender->CurrentAction = NULL;
			return;
		}
	}

	const char* Dialog;
	switch (Flags & BD_LOCMASK) {
		case BD_STRING0:
			Dialog = parameters->string0Parameter;
			if (Flags & BD_SETDIALOG)
				actor->SetDialog( Dialog );
			break;
		case BD_SOURCE:
			Dialog = actor->Dialog; break;
		case BD_TARGET:
			Dialog = target->Dialog; break;
		case BD_RESERVED:
			PlayerDialogRes[5] = '1';
			Dialog = ( const char * ) PlayerDialogRes;
			break;
	}
	if (!Dialog) {
		Sender->CurrentAction = NULL;
		return;
	}

	//we also need to freeze active scripts during a dialog!
	if (( Flags & BD_INTERRUPT )) {
		target->ClearActions();
	} else {
		if (target->GetNextAction()) {
			printf( "[IEScript]: Target appears busy!\n" );
			Sender->CurrentAction = NULL;
			return;
		}
	}

	actor->Orientation = GetOrient( target->XPos, target->YPos, actor->XPos,
							actor->YPos );
	actor->resetAction = true; //im not sure this is needed
	target->Orientation = GetOrient( actor->XPos, actor->YPos, target->XPos,
							target->YPos );
	target->resetAction = true;//nor this

	if (Dialog[0]) {
		//increasing NumTimesTalkedTo
		if (Flags & BD_TALKCOUNT)
			actor->TalkCount++;

		gc->InitDialog( actor, target, Dialog );
	}
}

//no string, increase talkcount, no interrupt
void GameScript::Dialogue(Scriptable* Sender, Action* parameters)
{
	BeginDialog( Sender, parameters, BD_SOURCE | BD_TALKCOUNT | BD_CHECKDIST );
}

void GameScript::DialogueForceInterrupt(Scriptable* Sender, Action* parameters)
{
	BeginDialog( Sender, parameters, BD_SOURCE | BD_TALKCOUNT | BD_INTERRUPT );
}

void GameScript::DisplayString(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[1] );
	if (!scr) {
		return;
	}
	if (scr->overHeadText) {
		free( scr->overHeadText );
	}
	scr->overHeadText = core->GetString( parameters->int0Parameter );
	GetTime( scr->timeStartDisplaying );
	scr->textDisplaying = 0;
	GameControl* gc = ( GameControl* ) core->GetWindow( 0 )->GetControl( 0 );	
	if (gc->ControlType == IE_GUI_GAMECONTROL) {
		gc->DisplayString( scr );
	}
}

void GameScript::AmbientActivate(Scriptable* Sender, Action* parameters)
{
	Animation* anim = core->GetGame()->GetMap( 0 )->GetAnimation( parameters->objects[1]->objectName );
	if (!anim) {
		printf( "Script error: No Animation Named \"%s\"\n",
			parameters->objects[1]->objectName );
		return;
	}
	anim->Active = parameters->int0Parameter;
}

void GameScript::SetDialogue(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		return;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	if (tar->Type != ST_ACTOR) {
		return;
	}
	Actor* target = ( Actor* ) tar;
	target->SetDialog( parameters->string0Parameter );
}

//string0, no interrupt, talkcount increased
void GameScript::StartDialogue(Scriptable* Sender, Action* parameters)
{
	BeginDialog( Sender, parameters, BD_STRING0 | BD_TALKCOUNT | BD_SETDIALOG );
}

//string0, no interrupt, talkcount increased, don't set default
void GameScript::StartDialogueOverride(Scriptable* Sender, Action* parameters)
{
	BeginDialog( Sender, parameters, BD_STRING0 | BD_TALKCOUNT );
}

//string0, no interrupt, talkcount increased, don't set default
void GameScript::StartDialogueOverrideInterrupt(Scriptable* Sender,
	Action* parameters)
{
	BeginDialog( Sender, parameters, BD_STRING0 | BD_TALKCOUNT | BD_INTERRUPT );
}

//no string, no interrupt, talkcount increased
void GameScript::PlayerDialogue(Scriptable* Sender, Action* parameters)
{
	//i think playerdialog is when a player initiates dialog with the
	//target, in this case, the dialog is the target's dialog
	BeginDialog( Sender, parameters, BD_RESERVED | BD_OWN );
}

void GameScript::StartDialogueInterrupt(Scriptable* Sender, Action* parameters)
{
	BeginDialog( Sender, parameters,
		BD_STRING0 | BD_INTERRUPT | BD_TALKCOUNT | BD_SETDIALOG );
}

//No string, flags:0
void GameScript::StartDialogueNoSet(Scriptable* Sender, Action* parameters)
{
	BeginDialog( Sender, parameters, BD_SOURCE );
}

void GameScript::StartDialogueNoSetInterrupt(Scriptable* Sender,
	Action* parameters)
{
	BeginDialog( Sender, parameters, BD_SOURCE | BD_INTERRUPT );
}

Point* FindNearPoint(Actor* Sender, Point* p1, Point* p2, double& distance)
{
	long x1 = ( Sender->XPos - p1->x );
	long y1 = ( Sender->YPos - p1->y );
	double distance1 = sqrt( ( double ) ( x1* x1 + y1* y1 ) );
	long x2 = ( Sender->XPos - p2->x );
	long y2 = ( Sender->YPos - p2->y );
	double distance2 = sqrt( ( double ) ( x2* x2 + y2* y2 ) );
	if (distance1 < distance2) {
		distance = distance1;
		return p1;
	} else {
		distance = distance2;
		return p2;
	}
}

void GameScript::OpenDoor(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		Sender->CurrentAction = NULL;
		return;
	}
	Scriptable* tar = core->GetGame()->GetMap( 0 )->tm->GetDoor( parameters->objects[1]->objectName );
	if (!tar) {
		Sender->CurrentAction = NULL;
		return;
	}
	if (tar->Type != ST_DOOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	Door* door = ( Door* ) tar;
	if (scr->Type != ST_ACTOR) {
		door->SetDoorClosed( false, true );		
		Sender->CurrentAction = NULL;
		return;
	}
	Actor* actor = ( Actor* ) scr;
	double distance;
	Point* p = FindNearPoint( actor, &door->toOpen[0], &door->toOpen[1],
				distance );	
	if (distance <= 12) {
		door->SetDoorClosed( false, true );
	} else {
		Sender->AddActionInFront( Sender->CurrentAction );
		char Tmp[256];
		sprintf( Tmp, "MoveToPoint([%d,%d])", p->x, p->y );
		actor->AddActionInFront( GameScript::CreateAction( Tmp, true ) );
	}
	Sender->CurrentAction = NULL;
}

void GameScript::CloseDoor(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		Sender->CurrentAction = NULL;
		return;
	}
	Scriptable* tar = core->GetGame()->GetMap( 0 )->tm->GetDoor( parameters->objects[1]->objectName );
	if (!tar) {
		Sender->CurrentAction = NULL;
		return;
	}
	if (tar->Type != ST_DOOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	Door* door = ( Door* ) tar;
	if (scr->Type != ST_ACTOR) {
		door->SetDoorClosed( true, true );
		Sender->CurrentAction = NULL;
		return;
	}
	Actor* actor = ( Actor* ) scr;
	double distance;
	Point* p = FindNearPoint( actor, &door->toOpen[0], &door->toOpen[1],
				distance );	
	if (distance <= 12) {
		door->SetDoorClosed( true, true );
	} else {
		Sender->AddActionInFront( Sender->CurrentAction );
		char Tmp[256];
		sprintf( Tmp, "MoveToPoint([%d,%d])", p->x, p->y );
		actor->AddActionInFront( GameScript::CreateAction( Tmp, true ) );
	}
	Sender->CurrentAction = NULL;
}

void GameScript::MoveBetweenAreas(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr->Type != ST_ACTOR) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	strcpy( actor->Area, parameters->string0Parameter );
	if (!actor->FromGame) {
		actor->FromGame = true;
		if (actor->InParty)
			core->GetGame()->SetPC( actor );
		else
			core->GetGame()->AddNPC( actor );
		core->GetGame()->GetMap( 0 )->RemoveActor( actor );
		actor->XPos = parameters->XpointParameter;
		actor->YPos = parameters->YpointParameter;
		actor->Orientation = parameters->int0Parameter;
	}
}

void GetPositionFromScriptable(Scriptable* scr, unsigned short& X,
	unsigned short& Y)
{
	switch (scr->Type) {
		case ST_TRIGGER:
		case ST_PROXIMITY:
		case ST_TRAVEL:
			 {
				InfoPoint* ip = ( InfoPoint* ) scr;
				X = ip->TrapLaunchX;
				Y = ip->TrapLaunchY;
			}
			break;

		case ST_ACTOR:
			 {
				Actor* ac = ( Actor* ) scr;
				X = ac->XPos;
				Y = ac->YPos;
			}
			break;

		case ST_DOOR:
			 {
				Door* door = ( Door* ) scr;
				X = door->XPos;
				Y = door->YPos;
			}
			break;

		case ST_CONTAINER:
			 {
				Container* cont = ( Container* ) scr;
				X = cont->trapTarget.x;
				Y = cont->trapTarget.y;
			}
			break;
	}
}

void GameScript::ForceSpell(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		return;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	unsigned short sX,sY, dX,dY;
	GetPositionFromScriptable( scr, sX, sY );
	GetPositionFromScriptable( tar, dX, dY );
	printf( "ForceSpell from [%d,%d] to [%d,%d]\n", sX, sY, dX, dY );
}

void GameScript::Deactivate(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		return;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	if (tar->Type != ST_ACTOR) {
		return;
	}
	tar->Active = false;
}

void GameScript::MakeGlobal(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (scr->Type != ST_ACTOR) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		return;
	}
	Actor* act = ( Actor* ) scr;
	if (!core->GetGame()->InParty( act )) {
		core->GetGame()->AddNPC( act );
	}
}

void GameScript::UnMakeGlobal(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (scr->Type != ST_ACTOR) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		return;
	}
	Actor* act = ( Actor* ) scr;
	int slot;
	slot = core->GetGame()->InStore( act );
	if (slot >= 0) {
		core->GetGame()->DelNPC( slot );
	}
}

void GameScript::JoinParty(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (scr->Type != ST_ACTOR) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		return;
	}
	Actor* act = ( Actor* ) scr;
	core->GetGame()->JoinParty( act );
	act->SetScript( "DPLAYER2", SCR_DEFAULT );
}

void GameScript::LeaveParty(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (scr->Type != ST_ACTOR) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		return;
	}
	Actor* act = ( Actor* ) scr;
	core->GetGame()->LeaveParty( act );
}

void GameScript::Activate(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		return;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	if (tar->Type != ST_ACTOR) {
		return;
	}
	tar->Active = true;
}

void GameScript::LeaveAreaLUA(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr->Type != ST_ACTOR) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	strcpy( actor->Area, parameters->string0Parameter );
	if (!actor->FromGame) {
		actor->FromGame = true;
		if (actor->InParty)
			core->GetGame()->SetPC( actor );
		else
			core->GetGame()->AddNPC( actor );
	}
	core->GetGame()->GetMap( 0 )->RemoveActor( actor );
	if (parameters->XpointParameter >= 0) {
		actor->XPos = parameters->XpointParameter;
	}
	if (parameters->YpointParameter >= 0) {
		actor->YPos = parameters->YpointParameter;
	}
	if (parameters->int0Parameter >= 0) {
		actor->Orientation = parameters->int0Parameter;
	}
}

void GameScript::LeaveAreaLUAPanic(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr->Type != ST_ACTOR) {
		return;
	}
	if (scr != Sender) {
		//this is an Action Override
		scr->AddAction( Sender->CurrentAction );
		return;
	}
	LeaveAreaLUA( Sender, parameters );
}

void GameScript::SetTokenGlobal(Scriptable* Sender, Action* parameters)
{
	unsigned long value = CheckVariable( Sender, parameters->string0Parameter );
	char varname[33]; //this is the Token Name
	strncpy( varname, parameters->string1Parameter, 32 );
	varname[32] = 0;
	printf( "SetTokenGlobal: %d -> %s\n", value, varname );
	char tmpstr[10];
	sprintf( tmpstr, "%d", value );
	char* newvalue = ( char* ) malloc( strlen( tmpstr ) + 1 );
	strcpy( newvalue, tmpstr );
	core->GetTokenDictionary()->SetAt( varname, newvalue );
}

void GameScript::PlayDead(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->AnimID = IE_ANI_DIE;
	//also set time for playdead!
	actor->SetWait( parameters->int0Parameter );
}

void GameScript::GlobalSetGlobal(Scriptable* Sender, Action* parameters)
{
	unsigned long value = CheckVariable( Sender, parameters->string0Parameter );
	SetVariable( Sender, parameters->string1Parameter, value );
}

/* adding the second variable to the first, they must be GLOBAL */
void GameScript::AddGlobals(Scriptable* Sender, Action* parameters)
{
	unsigned long value1 = CheckVariable( Sender, "GLOBAL",
							parameters->string0Parameter );
	unsigned long value2 = CheckVariable( Sender, "GLOBAL",
							parameters->string1Parameter );
	SetVariable( Sender, "GLOBAL", parameters->string0Parameter,
		value1 + value2 );
}

/* adding the second variable to the first, they could be area or locals */
void GameScript::GlobalAddGlobal(Scriptable* Sender, Action* parameters)
{
	unsigned long value1 = CheckVariable( Sender,
							parameters->string0Parameter );
	unsigned long value2 = CheckVariable( Sender,
							parameters->string1Parameter );
	SetVariable( Sender, parameters->string0Parameter, value1 + value2 );
}

/* adding the number to the global, they could be area or locals */
void GameScript::IncrementGlobal(Scriptable* Sender, Action* parameters)
{
	unsigned long value = CheckVariable( Sender, parameters->string0Parameter );
	SetVariable( Sender, parameters->string0Parameter,
		value + parameters->int0Parameter );
}

/* adding the number to the global ONLY if the first global is zero */
void GameScript::IncrementGlobalOnce(Scriptable* Sender, Action* parameters)
{
	unsigned long value = CheckVariable( Sender, parameters->string0Parameter );
	if (value != 0) {
		return;
	}
	value = CheckVariable( Sender, parameters->string1Parameter );
	SetVariable( Sender, parameters->string1Parameter,
		value + parameters->int0Parameter );
}

void GameScript::GlobalSubGlobal(Scriptable* Sender, Action* parameters)
{
	unsigned long value1 = CheckVariable( Sender,
							parameters->string0Parameter );
	unsigned long value2 = CheckVariable( Sender,
							parameters->string1Parameter );
	SetVariable( Sender, parameters->string0Parameter, value1 - value2 );
}

void GameScript::GlobalAndGlobal(Scriptable* Sender, Action* parameters)
{
	unsigned long value1 = CheckVariable( Sender,
							parameters->string0Parameter );
	unsigned long value2 = CheckVariable( Sender,
							parameters->string1Parameter );
	SetVariable( Sender, parameters->string0Parameter, value1 && value2 );
}

void GameScript::GlobalOrGlobal(Scriptable* Sender, Action* parameters)
{
	unsigned long value1 = CheckVariable( Sender,
							parameters->string0Parameter );
	unsigned long value2 = CheckVariable( Sender,
							parameters->string1Parameter );
	SetVariable( Sender, parameters->string0Parameter, value1 || value2 );
}

void GameScript::GlobalBOrGlobal(Scriptable* Sender, Action* parameters)
{
	unsigned long value1 = CheckVariable( Sender,
							parameters->string0Parameter );
	unsigned long value2 = CheckVariable( Sender,
							parameters->string1Parameter );
	SetVariable( Sender, parameters->string0Parameter, value1 | value2 );
}

void GameScript::GlobalBAndGlobal(Scriptable* Sender, Action* parameters)
{
	unsigned long value1 = CheckVariable( Sender,
							parameters->string0Parameter );
	unsigned long value2 = CheckVariable( Sender,
							parameters->string1Parameter );
	SetVariable( Sender, parameters->string0Parameter, value1 & value2 );
}

void GameScript::GlobalBOr(Scriptable* Sender, Action* parameters)
{
	unsigned long value1 = CheckVariable( Sender,
							parameters->string0Parameter );
	SetVariable( Sender, parameters->string0Parameter,
		value1 | parameters->int0Parameter );
}

void GameScript::GlobalBAnd(Scriptable* Sender, Action* parameters)
{
	unsigned long value1 = CheckVariable( Sender,
							parameters->string0Parameter );
	SetVariable( Sender, parameters->string0Parameter,
		value1 & parameters->int0Parameter );
}

void GameScript::GlobalMax(Scriptable* Sender, Action* parameters)
{
	unsigned long value1 = CheckVariable( Sender,
							parameters->string0Parameter );
	if (value1 > parameters->int0Parameter) {
		SetVariable( Sender, parameters->string0Parameter, value1 );
	}
}

void GameScript::GlobalMin(Scriptable* Sender, Action* parameters)
{
	unsigned long value1 = CheckVariable( Sender,
							parameters->string0Parameter );
	if (value1 < parameters->int0Parameter) {
		SetVariable( Sender, parameters->string0Parameter, value1 );
	}
}

void GameScript::BitClear(Scriptable* Sender, Action* parameters)
{
	unsigned long value1 = CheckVariable( Sender,
							parameters->string0Parameter );
	SetVariable( Sender, parameters->string0Parameter,
		value1 & ~parameters->int0Parameter );
}

void GameScript::GlobalShL(Scriptable* Sender, Action* parameters)
{
	unsigned long value1 = CheckVariable( Sender,
							parameters->string0Parameter );
	unsigned long value2 = parameters->int0Parameter;
	if (value2 > 31) {
		value1 = 0;
	} else {
		value1 <<= value2;
	}
	SetVariable( Sender, parameters->string0Parameter, value1 );
}

void GameScript::GlobalShR(Scriptable* Sender, Action* parameters)
{
	unsigned long value1 = CheckVariable( Sender,
							parameters->string0Parameter );
	unsigned long value2 = parameters->int0Parameter;
	if (value2 > 31) {
		value1 = 0;
	} else {
		value1 >>= value2;
	}
	SetVariable( Sender, parameters->string0Parameter, value1 );
}

void GameScript::GlobalMaxGlobal(Scriptable* Sender, Action* parameters)
{
	unsigned long value1 = CheckVariable( Sender,
							parameters->string0Parameter );
	unsigned long value2 = CheckVariable( Sender,
							parameters->string1Parameter );
	if (value1 < value2) {
		SetVariable( Sender, parameters->string0Parameter, value1 );
	}
}

void GameScript::GlobalMinGlobal(Scriptable* Sender, Action* parameters)
{
	unsigned long value1 = CheckVariable( Sender,
							parameters->string0Parameter );
	unsigned long value2 = CheckVariable( Sender,
							parameters->string1Parameter );
	if (value1 < value2) {
		SetVariable( Sender, parameters->string0Parameter, value1 );
	}
}

void GameScript::GlobalShLGlobal(Scriptable* Sender, Action* parameters)
{
	unsigned long value1 = CheckVariable( Sender,
							parameters->string0Parameter );
	unsigned long value2 = CheckVariable( Sender,
							parameters->string1Parameter );
	if (value2 > 31) {
		value1 = 0;
	} else {
		value1 <<= value2;
	}
	SetVariable( Sender, parameters->string0Parameter, value1 );
}
void GameScript::GlobalShRGlobal(Scriptable* Sender, Action* parameters)
{
	unsigned long value1 = CheckVariable( Sender,
							parameters->string0Parameter );
	unsigned long value2 = CheckVariable( Sender,
							parameters->string1Parameter );
	if (value2 > 31) {
		value1 = 0;
	} else {
		value1 >>= value2;
	}
	SetVariable( Sender, parameters->string0Parameter, value1 );
}

void GameScript::ClearAllActions(Scriptable* Sender, Action* parameters)
{
	//just a hack
	Game* game = core->GetGame();

	for (int i = 0; i < game->PartySize; i++) {
		Actor* act = game->GetPC( i );
		if (act)
			act->ClearActions();
	}
}

void GameScript::ClearActions(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (scr) {
		scr->ClearActions();
	}
}

void GameScript::SetNumTimesTalkedTo(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->TalkCount = parameters->int0Parameter;
}

void GameScript::StartMovie(Scriptable* Sender, Action* parameters)
{
	core->PlayMovie( parameters->string0Parameter );
}

void GameScript::SetLeavePartyDialogFile(Scriptable* Sender,
	Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[0] );
	if (!scr) {
		return;
	}
	if (scr->Type != ST_ACTOR) {
		return;
	}
	int pdtable = core->LoadTable( "pdialog" );
	Actor* actor = ( Actor* ) scr;
	char* scriptingname = actor->GetScriptName();
	actor->SetDialog( core->GetTable( pdtable )->QueryField( scriptingname,
													"POST_DIALOG_FILE" ) );
	core->DelTable( pdtable );
}

