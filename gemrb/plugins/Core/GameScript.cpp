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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/GameScript.cpp,v 1.105 2004/03/20 10:06:18 avenger_teambg Exp $
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
static SymbolMgr* triggersTable;
static SymbolMgr* actionsTable;
static SymbolMgr* objectsTable;
static TriggerFunction triggers[MAX_TRIGGERS];
static ActionFunction actions[MAX_ACTIONS];
static int flags[MAX_ACTIONS];
static ObjectFunction objects[MAX_OBJECTS];
static IDSFunction idtargets[MAX_OBJECT_FIELDS];

static int ObjectIDSCount = 7;
static int MaxObjectNesting = 5;
static bool HasAdditionalRect = false;
static std::vector< char*> ObjectIDSTableNames;
static int ObjectFieldsCount = 7;
static int ExtraParametersCount = 0;
static int RandomNumValue;
static int InDebug = 0;

//Make this an ordered list, so we could use bsearch!
static TriggerLink triggernames[] = {
	{"actionlistempty", GameScript::ActionListEmpty},
	{"alignment", GameScript::Alignment},
	{"allegiance", GameScript::Allegiance},
	{"areacheck", GameScript::AreaCheck},
	{"areatype", GameScript::AreaType},
	{"areaflag", GameScript::AreaFlag},
	{"bitcheck",GameScript::BitCheck},
	{"breakingpoint",GameScript::BreakingPoint},
	{"checkstat",GameScript::CheckStat},
	{"checkstatgt",GameScript::CheckStatGT},
	{"checkstatlt",GameScript::CheckStatLT}, {"class", GameScript::Class},
	{"clicked", GameScript::Clicked}, {"dead", GameScript::Dead},
	{"entered", GameScript::Entered}, {"exists", GameScript::Exists},
	{"false", GameScript::False}, {"gender", GameScript::Gender},
	{"general", GameScript::General}, {"global", GameScript::Global},
	{"globalgt", GameScript::GlobalGT}, {"globallt", GameScript::GlobalLT},
	{"globalsequal", GameScript::GlobalsEqual},
	{"globaltimerexact", GameScript::GlobalTimerExact},
	{"globaltimerexpired", GameScript::GlobalTimerExpired},
	{"globaltimernotexpired", GameScript::GlobalTimerNotExpired},
	{"happiness", GameScript::Happiness},
	{"happinessgt", GameScript::HappinessGT},
	{"happinesslt", GameScript::HappinessLT},
	{"harmlessentered", GameScript::Entered}, //this isn't sure the same
	{"hp", GameScript::HP},
	{"hpgt", GameScript::HPGT}, {"hplt", GameScript::HPLT},
	{"hppercent", GameScript::HPPercent},
	{"hppercentgt", GameScript::HPPercentGT},
	{"hppercentlt", GameScript::HPPercentLT},
	{"inactivearea", GameScript::InActiveArea},
	{"inmyarea", GameScript::InMyArea},
	{"inparty", GameScript::InParty},
	{"inpartyallowdead", GameScript::InPartyAllowDead},
	{"inpartyslot", GameScript::InPartySlot},
	{"isvalidforpartydialog", GameScript::IsValidForPartyDialog},
	{"level", GameScript::Level},
	{"levelgt", GameScript::LevelGT},
	{"levellt", GameScript::LevelLT},
	{"los", GameScript::LOS},
	{"morale", GameScript::Morale},
	{"moralegt", GameScript::MoraleGT},
	{"moralelt", GameScript::MoraleLT},
	{"notstatecheck", GameScript::NotStateCheck},
	{"numtimestalkedto", GameScript::NumTimesTalkedTo},
	{"numtimestalkedtogt", GameScript::NumTimesTalkedToGT},
	{"numtimestalkedtolt", GameScript::NumTimesTalkedToLT},
	{"objectactionlistempty", GameScript::ObjectActionListEmpty}, //same function
	{"oncreation", GameScript::OnCreation},
	{"openstate", GameScript::OpenState},
	{"or", GameScript::Or},
	{"partyhasitem", GameScript::PartyHasItem}, {"race", GameScript::Race},
	{"randomnum", GameScript::RandomNum},
	{"randomnumgt", GameScript::RandomNumGT},
	{"randomnumlt", GameScript::RandomNumLT},
	{"range", GameScript::Range}, {"see", GameScript::See},
	{"specific", GameScript::Specific},
	{"statecheck", GameScript::StateCheck},
	{"true", GameScript::True}, {"xp", GameScript::XP},
	{"unselectablevariable", GameScript::UnselectableVariable},
	{"unselectablevariablegt", GameScript::UnselectableVariableGT},
	{"unselectablevariablelt", GameScript::UnselectableVariableLT},
	{"xpgt", GameScript::XPGT}, {"xplt", GameScript::XPLT}, { NULL,NULL}, 
};

//Make this an ordered list, so we could use bsearch!
static ActionLink actionnames[] = {
	{"actionoverride",NULL}, {"activate",GameScript::Activate},
	{"addareatype", GameScript::AddAreaType},
	{"addareaflag", GameScript::AddAreaFlag},
	{"addexperienceparty",GameScript::AddExperienceParty},
	{"addexperiencepartyglobal",GameScript::AddExperiencePartyGlobal},
	{"addglobals",GameScript::AddGlobals},
	{"addwaypoint",GameScript::AddWayPoint,AF_BLOCKING},
	{"addxp2da", GameScript::AddXP2DA},
	{"addxpobject", GameScript::AddXPObject},
	{"addxpvar", GameScript::AddXP2DA},
	{"ally",GameScript::Ally},
	{"ambientactivate",GameScript::AmbientActivate},
	{"bashdoor",GameScript::OpenDoor,AF_BLOCKING}, //the same until we know better
	{"bitclear",GameScript::BitClear},
	{"bitset",GameScript::GlobalBOr}, //probably the same
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
	{"createcreatureatfeet",GameScript::CreateCreatureOffset}, //this is the same
	{"createcreatureoffset",GameScript::CreateCreatureOffset},
	{"createvisualeffect",GameScript::CreateVisualEffect},
	{"createvisualeffectobject",GameScript::CreateVisualEffectObject},
	{"cutsceneid",GameScript::CutSceneID,AF_INSTANT},
	{"deactivate",GameScript::Deactivate},
	{"destroypartygold",GameScript::DestroyPartyGold},
	{"destroyself",GameScript::DestroySelf},
	{"dialogue",GameScript::Dialogue,AF_BLOCKING},
	{"dialogueforceinterrupt",GameScript::DialogueForceInterrupt,AF_BLOCKING},
	{"displaystring",GameScript::DisplayString},
	{"displaystringhead",GameScript::DisplayStringHead},
	{"displaystringnonamehead",GameScript::DisplayStringNoNameHead},
	{"displaystringwait",GameScript::DisplayStringWait,AF_BLOCKING},
	{"endcutscenemode",GameScript::EndCutSceneMode},
	{"enemy",GameScript::Enemy}, {"face",GameScript::Face,AF_BLOCKING},
	{"faceobject",GameScript::FaceObject, AF_BLOCKING},
	{"fadefromblack",GameScript::FadeFromColor}, //probably the same
	{"fadefromcolor",GameScript::FadeFromColor},
	{"fadetoblack",GameScript::FadeToColor}, //probably the same
	{"fadetocolor",GameScript::FadeToColor},
	{"floatmessage",GameScript::DisplayStringHead}, //probably the same
	{"forceaiscript",GameScript::ForceAIScript},
	{"forcespell",GameScript::ForceSpell},
	{"giveexperience", GameScript::AddXPObject},
	{"givepartygold",GameScript::GivePartyGold},
	{"givepartygoldglobal",GameScript::GivePartyGoldGlobal},
	{"globalband",GameScript::GlobalBAnd},
	{"globalbandglobal",GameScript::GlobalBAndGlobal},
	{"globalbor",GameScript::GlobalBOr},
	{"globalborglobal",GameScript::GlobalBOrGlobal},
	{"globalmax",GameScript::GlobalMax},
	{"globalmaxglobal",GameScript::GlobalMaxGlobal},
	{"globalmin",GameScript::GlobalMin},
	{"globalminglobal",GameScript::GlobalMinGlobal},
	{"globalsetglobal",GameScript::GlobalSetGlobal},
	{"globalshl",GameScript::GlobalShL},
	{"globalshlglobal",GameScript::GlobalShLGlobal},
	{"globalshr",GameScript::GlobalShR},
	{"globalshrglobal",GameScript::GlobalShRGlobal},
	{"globalxor",GameScript::GlobalXor},
	{"globalxorglobal",GameScript::GlobalXorGlobal},
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
	{"moraledec",GameScript::MoraleDec},
	{"moraleinc",GameScript::MoraleInc},
	{"moraleset",GameScript::MoraleSet},
	{"movebetweenareas",GameScript::MoveBetweenAreas},
	{"movetoobject",GameScript::MoveToObject,AF_BLOCKING},
	{"movetooffset",GameScript::MoveToOffset,AF_BLOCKING},
	{"movetopoint",GameScript::MoveToPoint,AF_BLOCKING},
	{"movetopointnorecticle",GameScript::MoveToPoint,AF_BLOCKING},//the same until we know better
	{"moveviewobject",GameScript::MoveViewPoint},
	{"moveviewpoint",GameScript::MoveViewPoint},
	{"noaction",GameScript::NoAction},
	{"opendoor",GameScript::OpenDoor,AF_BLOCKING},
	{"permanentstatchange",GameScript::ChangeStat}, //probably the same
	{"picklock",GameScript::OpenDoor,AF_BLOCKING}, //the same until we know better
	{"playdead",GameScript::PlayDead}, {"playsound",GameScript::PlaySound},
	{"recoil",GameScript::Recoil},
	{"removeareatype", GameScript::RemoveAreaType},
	{"removeareaflag", GameScript::RemoveAreaFlag},
	{"runawayfrom",GameScript::RunAwayFrom,AF_BLOCKING},
	{"runawayfromnointerrupt",GameScript::RunAwayFromNoInterrupt,AF_BLOCKING},
	{"runawayfrompoint",GameScript::RunAwayFromPoint,AF_BLOCKING},
	{"runtoobject",GameScript::MoveToObject,AF_BLOCKING}, //until we know better
	{"runtopoint",GameScript::MoveToPoint,AF_BLOCKING}, //until we know better
	{"runtopointnorecticle",GameScript::MoveToPoint,AF_BLOCKING},//until we know better
	{"screenshake",GameScript::ScreenShake,AF_BLOCKING},
	{"setanimstate",GameScript::SetAnimState,AF_BLOCKING},
	{"setarearestflag", GameScript::SetAreaRestFlag},
	{"setdialogue",GameScript::SetDialogue,AF_BLOCKING},
	{"setglobal",GameScript::SetGlobal},
	{"setglobaltimer",GameScript::SetGlobalTimer},
	{"setnumtimestalkedto",GameScript::SetNumTimesTalkedTo},
	{"setplayersound",GameScript::SetPlayerSound},
	{"settokenglobal",GameScript::SetTokenGlobal}, {"sg",GameScript::SG},
	{"smallwait",GameScript::SmallWait,AF_BLOCKING},
	{"startcutscene",GameScript::StartCutScene},
	{"startcutscenemode",GameScript::StartCutSceneMode},
	{"startdialog",GameScript::StartDialogue,AF_BLOCKING},
	{"startdialogue",GameScript::StartDialogue,AF_BLOCKING},
	{"startdialoginterrupt",GameScript::StartDialogueInterrupt,AF_BLOCKING},
	{"startdialogueinterrupt",GameScript::StartDialogueInterrupt,AF_BLOCKING},
	{"startdialognoname",GameScript::StartDialogue,AF_BLOCKING},
	{"startdialoguenoname",GameScript::StartDialogue,AF_BLOCKING},
	{"startdialognoset",GameScript::StartDialogueNoSet,AF_BLOCKING},
	{"startdialoguenoset",GameScript::StartDialogueNoSet,AF_BLOCKING},
	{"startdialoguenosetinterrupt",GameScript::StartDialogueNoSetInterrupt,AF_BLOCKING},
	{"startdialogoverride",GameScript::StartDialogueOverride,AF_BLOCKING},
	{"startdialogueoverride",GameScript::StartDialogueOverride,AF_BLOCKING},
	{"startdialogoverrideinterrupt",GameScript::StartDialogueOverrideInterrupt,AF_BLOCKING},
	{"startdialogueoverrideinterrupt",GameScript::StartDialogueOverrideInterrupt,AF_BLOCKING},
	{"startmovie",GameScript::StartMovie},
	{"startsong",GameScript::StartSong},
	{"swing",GameScript::Swing},
	{"takepartygold",GameScript::TakePartyGold},
	{"triggeractivation",GameScript::TriggerActivation},
	{"unhidegui",GameScript::UnhideGUI},
	{"unlock",GameScript::Unlock},
	{"unmakeglobal",GameScript::UnMakeGlobal}, //this is a GemRB extension
	{"verbalconstant",GameScript::VerbalConstant},
	{"wait",GameScript::Wait, AF_BLOCKING},
	{"waitrandom",GameScript::WaitRandom, AF_BLOCKING}, { NULL,NULL}, 
};

//Make this an ordered list, so we could use bsearch!
static ObjectLink objectnames[] = {
	{"bestac",GameScript::BestAC},
	{"eighthnearest",GameScript::EighthNearest},
	{"eighthnearestenemyof",GameScript::EighthNearestEnemyOf},
	{"fifthnearest",GameScript::FifthNearest},
	{"fifthnearestenemyof",GameScript::FifthNearestEnemyOf},
	{"fourthnearest",GameScript::FourthNearest},
	{"fourthnearestenemyof",GameScript::FourthNearestEnemyOf},
	{"lastseenby",GameScript::LastSeenBy},
	{"lasttalkedtoby",GameScript::LastTalkedToBy},
	{"myself",GameScript::Myself},
	{"nearest",GameScript::Nearest},
	{"nearestenemyof",GameScript::NearestEnemyOf},
	{"ninthnearest",GameScript::NinthNearest},
	{"ninthnearestenemyof",GameScript::NinthNearestEnemyOf},
	{"player1",GameScript::Player1},
	{"player1fill",GameScript::Player1Fill},
	{"player2",GameScript::Player2},
	{"player2fill",GameScript::Player2Fill},
	{"player3",GameScript::Player3},
	{"player3fill",GameScript::Player3Fill},
	{"player4",GameScript::Player4},
	{"player4fill",GameScript::Player4Fill},
	{"player5",GameScript::Player5},
	{"player5fill",GameScript::Player5Fill},
	{"player6",GameScript::Player6},
	{"player6fill",GameScript::Player6Fill},
	{"player7",GameScript::Player7},
	{"player7fill",GameScript::Player7Fill},
	{"player8",GameScript::Player8},
	{"player8fill",GameScript::Player8Fill},
	{"protagonist",GameScript::Protagonist},
	{"secondnearest",GameScript::SecondNearest},
	{"secondnearestenemyof",GameScript::SecondNearestEnemyOf},
	{"seventhnearest",GameScript::SeventhNearest},
	{"seventhnearestenemyof",GameScript::SeventhNearestEnemyOf},
	{"sixthnearest",GameScript::SixthNearest},
	{"sixthnearestenemyof",GameScript::SixthNearestEnemyOf},
	{"strongestof",GameScript::StrongestOf},
	{"tenthnearest",GameScript::TenthNearest},
	{"tenthnearestenemyof",GameScript::TenthNearestEnemyOf},
	{"thirdnearest",GameScript::ThirdNearest},
	{"thirdnearestenemyof",GameScript::ThirdNearestEnemyOf},
	{"weakestof",GameScript::WeakestOf},
	{"worstac",GameScript::WorstAC},
	{ NULL,NULL}, 
};

static IDSLink idsnames[] = {
	{"align",GameScript::ID_Alignment},
	{"alignmen",GameScript::ID_Alignment},
	{"alignmnt",GameScript::ID_Alignment},
	{"avclass",GameScript::ID_AVClass},
	{"class",GameScript::ID_Class},
	{"classmsk",GameScript::ID_ClassMask},
	{"ea",GameScript::ID_Allegiance},
	{"faction",GameScript::ID_Faction},
	{"gender",GameScript::ID_Gender},
	{"general",GameScript::ID_General},
	{"race",GameScript::ID_Race},
	{"specific",GameScript::ID_Specific},
	{"subrace",GameScript::ID_Subrace},
	{"team",GameScript::ID_Team},
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

static IDSLink* FindIdentifier(const char* idsname)
{
	if (!idsname) {
		return NULL;
	}
	int len = strlen( idsname );
	for (int i = 0; idsnames[i].Name; i++) {
		if (!strnicmp( idsnames[i].Name, idsname, len )) {
			return idsnames + i;
		}
	}
	printf( "Warning: Couldn't assign ids target: %.*s\n", len, idsname );
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
		if(ObjectIDSCount<0 || ObjectIDSCount>MAX_OBJECT_FIELDS) {
			printf("[IEScript]: The IDS Count shouldn't be more than 10!\n");
			abort();
		}
		
		for (i = 0; i < ObjectIDSCount; i++) {
			char *idsname;
			idsname=objNameTable->QueryField( 0, i + 1 );
			IDSLink *poi=FindIdentifier( idsname );
			if(poi==NULL) {
				idtargets[i]=NULL;
			}
			else {
				idtargets[i]=poi->Function;
			}
			ObjectIDSTableNames.push_back( idsname );
		}
		MaxObjectNesting = atoi( objNameTable->QueryField( 1 ) );
		if(MaxObjectNesting<0 || MaxObjectNesting>MAX_NESTING) {
			printf("[IEScript]: The Object Nesting Count shouldn't be more than 5!\n");
			abort();
		}
		HasAdditionalRect = ( bool ) atoi( objNameTable->QueryField( 2 ) );
		ExtraParametersCount = atoi( objNameTable->QueryField( 3 ) );
		ObjectFieldsCount = ObjectIDSCount - ExtraParametersCount;

		/* Initializing the Script Engine */

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
	core->GetGame()->globals->SetAt( newVarName, ( unsigned long ) value );
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
	core->GetGame()->globals->SetAt( newVarName, ( unsigned long ) value );
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
	core->GetGame()->globals->Lookup( newVarName, value );
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
	core->GetGame()->globals->Lookup( VarName, value );
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
	while (isdigit(*src) || *src=='-') {
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
}

Object* GameScript::DecodeObject(const char* line)
{
	Object* oB = new Object();
	for (int i = 0; i < ObjectFieldsCount; i++) {
		oB->objectFields[i] = ParseInt( line );
	}
	for (int i = 0; i < MaxObjectNesting; i++) {
		oB->objectFilters[i] = ParseInt( line );
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
	return oB;
}

bool GameScript::EvaluateCondition(Scriptable* Sender, Condition* condition)
{
	int ORcount = 0;
	int result = 0;
	bool subresult = true;

	RandomNumValue=rand();
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
	const char *tmpstr=triggersTable->GetValue(trigger->triggerID);
	if(!tmpstr) {
		tmpstr=triggersTable->GetValue(trigger->triggerID|0x4000);
	}
	if (!func) {
		triggers[trigger->triggerID] = False;
		printf( "[IEScript]: Unhandled trigger code: 0x%04x %s\n",
			trigger->triggerID, tmpstr );
		return false;
	}
	if(InDebug) {
		printf( "[IEScript]: Executing trigger code: 0x%04x %s\n",
				trigger->triggerID, tmpstr );
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
	switch(rS->responsesCount) {
		case 0:
			return 0;
		case 1:
			return ExecuteResponse( Sender, rS->responses[0] );
	}
	/*default*/
	int maxWeight, randWeight;

	for (int i = 0; i < rS->responsesCount; i++) {
		maxWeight+=rS->responses[i]->weight;
	}
	if(maxWeight) {
		randWeight = rand() % maxWeight;
	}
	else {
		randWeight = 0;
	}

	for (int i = 0; i < rS->responsesCount; i++) {
		Response* rE = rS->responses[i];
		if (rE->weight > randWeight) {
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
		Scriptable* scr = GetActorFromObject( Sender, aC->objects[0]);
		if(scr && scr!=Sender) {
			//this is an Action Override
			scr->AddAction( Sender->CurrentAction );
			Sender->CurrentAction = NULL;
			//maybe we should always release here???
			if (!(flags[aC->actionID] & AF_INSTANT) ) {
				aC->Release();
			}
			return;
		}
		else {
			if(InDebug) {
				printf( "[IEScript]: Executing action code: %d %s\n", aC->actionID , actionsTable->GetValue(aC->actionID) );
			}
			func( Sender, aC );
		}
	}
	else {
		actions[aC->actionID] = NoAction;
		printf( "[IEScript]: Unhandled action code: %d %s\n", aC->actionID , actionsTable->GetValue(aC->actionID) );
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

/* returns actors that match the [x.y.z] expression */
Targets* GameScript::EvaluateObject(Object* oC)
{
	Targets *tgts=NULL;

	if (oC->objectName[0]) {
		//We want the object by its name... (doors/triggers don't play here!)
		Actor* aC = core->GetGame()->GetMap( 0 )->GetActor( oC->objectName );
		if(!aC) {
			return tgts;
		}
		//Ok :) we now have our Object. Let's create a Target struct and add the object to it
		tgts = new Targets();
		tgts->AddTarget( aC );
		//return here because object name/IDS targeting are mutually exclusive
		return tgts;
	}
	//else branch, IDS targeting
	for(int j = 0; j < ObjectIDSCount; j++) {
		if(!oC->objectFields[j]) {
			continue;
		}
		IDSFunction func = idtargets[j];
		if (!func) {
			printf("Unimplemented IDS targeting opcode!\n");
			continue;
		}
		if(tgts) {
			//we already got a subset of actors
			int i = tgts->Count();
			/*premature end, filtered everything*/
			if(!i) {
				break; //leaving the loop
			}
			while(i--) {
				if(!func(tgts->GetTarget(i), oC->objectFields[j] ) ) {
					tgts->RemoveTargetAt(i);
				}
			}
		}
		else {
			//we need to get a subset of actors from the large array
			//if this gets slow, we will need some index tables
			int i = core->GetActorCount();
			tgts = new Targets();
			while(i--) {
				Actor *ac=core->GetActor(i);
				if(ac && func(ac, oC->objectFields[j]) ) {
					tgts->AddTarget(ac);
				}
			}
		}
	}
	return tgts;
}

int GameScript::GetObjectCount(Scriptable* Sender, Object* oC)
{
	if (!oC) {
		return 0;
	}
	Targets* tgts = EvaluateObject(oC);
	int count = tgts->Count();
	delete tgts;
	return count;
}

Scriptable* GameScript::GetActorFromObject(Scriptable* Sender, Object* oC)
{
	if (!oC) {
		return NULL;
	}
	Targets* tgts = EvaluateObject(oC);
	if(!tgts && oC->objectName[0]) {
		//It was not an actor... maybe it is a door?
		Scriptable * aC = core->GetGame()->GetMap( 0 )->tm->GetDoor( oC->objectName );
		if (aC) {
			return aC;
		}
		//No... it was not a door... maybe an InfoPoint?
		aC = core->GetGame()->GetMap( 0 )->tm->GetInfoPoint( oC->objectName );
		if (aC) {
			return aC;
		}
		return NULL;
	}
	//now lets do the object filter stuff, we create Targets because
	//it is possible to start from blank sheets using endpoint filters
	//like (Myself, Protagonist etc)
	if(!tgts) {
		tgts = new Targets();
	}
	for (int i = 0; i < MaxObjectNesting; i++) {
		int filterid = oC->objectFilters[i];
		if(!filterid) {
			break;
		}
		ObjectFunction func = objects[filterid];
		if (func) {
			tgts = func( Sender, tgts);
		}
		else {
			printf("[IEScript]: Unknown object filter: %d %s\n",filterid, objectsTable->GetValue(filterid) );
		}
		if(!tgts->Count()) {
			delete tgts;
			return NULL;
		}
	}
	if(tgts) {
		Scriptable *object;
		if(tgts->Count()) {
			object=tgts->GetTarget(0);
		}
		else {
			object=NULL;
		}
		delete tgts;
		return object;
	}
	return NULL;
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

static int GetIdsValue(const char *symbol, const char *idsname)
{
	int idsfile=core->LoadSymbol(idsname);
	SymbolMgr *valHook = core->GetSymbol(idsfile);
	if(!valHook) {
		//FIXME:missing ids file!!!
		return -1;
	}
	return valHook->GetValue(symbol);
}

static void ParseIdsTarget(char *&src, Object *&object)
{
	for(int i=0;i<ObjectFieldsCount;i++) {
		if(isdigit(*src) || *src=='-') {
			object->objectFields[i]=strtol(src,&src,0);
		}
		else {
			int x;
			char symbol[64];
			for(x=0;isalnum(*src) && x<sizeof(symbol)-1;x++) {
				symbol[x]=*src;
				src++;
			}
			symbol[x]=0;
			object->objectFields[i]=GetIdsValue(symbol, ObjectIDSTableNames[i]);
		}
		if(*src!='.') {
			break;
		}
	}
	src++; //skipping ]
}

static void ParseObject(char *&str, char *&src, Object *&object)
{
	while (( *str != ',' ) && ( *str != ')' )) {
		str++;
	}
	object = new Object();
	switch (*src) {
	case '"':
		//Object Name
		src++;
		int i;
		for(i=0;i<sizeof(object->objectName)-1 && *src!='"';i++)
		{
			object->objectName[i] = *src;
			src++;
		}
		object->objectName[i] = 0;
		src++;
		break;
	case '[':
		src++; //skipping [
		ParseIdsTarget(src, object);
		break;
	default: //nested object filters
		int Nesting=0;
		
		while(Nesting++<MaxObjectNesting) {
			char filtername[64];
			int x;
			for(x=0;isalnum(*src) && x<sizeof(filtername)-1;x++) {
				filtername[x]=*src;
				src++;
			}
			filtername[x]=0;
			memmove(object->objectFilters+1, object->objectFilters, sizeof(int) *(MaxObjectNesting-1) );
			object->objectFilters[0]=GetIdsValue(filtername,"object");
			if(*src!='(') {
				break;
			}
		}
		if(*src=='[') {
			ParseIdsTarget(src, object);
		}
		src+=Nesting; //skipping )
	}
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
							while(( *str != '.' ) && ( *str != ')' ))
								str++;
							src++; //Skip [
							newAction->XpointParameter = atoi( src );
							while( *src!='.' && *src!=']')
								src++;
							src++; //Skip .
							newAction->YpointParameter = atoi( src );
							src++; //Skip ]
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
							ParseObject(str, src, newAction->objects[objectCount++]);
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
							ParseObject(str, src, newTrigger->objectParameter);
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
// Object Functions
//-------------------------------------------------------------

//in this implementation, Myself will drop the parameter array
//i think all object filters could be expected to do so
//they should remove unnecessary elements from the parameters
Targets *GameScript::Myself(Scriptable* Sender, Targets* parameters)
{
	parameters->Clear();
	if(Sender->Type==ST_ACTOR) {
		parameters->AddTarget((Actor *) Sender);
	}
	return parameters;
}

//same as player1 so far
Targets *GameScript::Protagonist(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(0));
	return parameters;
}

//last talker
Targets *GameScript::Gabber(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	GameControl* gc = ( GameControl* ) core->GetWindow( 0 )->GetControl( 0 );	
	parameters->AddTarget(gc->speaker);
	return parameters;
}

Targets *GameScript::LastSeenBy(Scriptable *Sender, Targets *parameters)
{
	Targets *tgts = new Targets();

	int i = parameters->Count();
	while(i--) {
		Actor *actor = parameters->GetTarget(i);
		if(!actor)
			continue;
		tgts->AddTarget(actor->LastSeen);
	}
	delete parameters;
	return tgts;
}

Targets *GameScript::LastTalkedToBy(Scriptable *Sender, Targets *parameters)
{
	Targets *tgts = new Targets();

	int i = parameters->Count();
	while(i--) {
		Actor *actor = parameters->GetTarget(i);
		if(!actor)
			continue;
		tgts->AddTarget(actor->LastTalkedTo);
	}
	delete parameters;
	return tgts;
}

Targets *GameScript::Player1(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(0));
	return parameters;
}

Targets *GameScript::Player1Fill(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(0));
	return parameters;
}

Targets *GameScript::Player2(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(1));
	return parameters;
}

Targets *GameScript::Player2Fill(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(1));
	return parameters;
}

Targets *GameScript::Player3(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(2));
	return parameters;
}

Targets *GameScript::Player3Fill(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(2));
	return parameters;
}

Targets *GameScript::Player4(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(3));
	return parameters;
}

Targets *GameScript::Player4Fill(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(3));
	return parameters;
}

Targets *GameScript::Player5(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(4));
	return parameters;
}

Targets *GameScript::Player5Fill(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(4));
	return parameters;
}

Targets *GameScript::Player6(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(5));
	return parameters;
}

Targets *GameScript::Player6Fill(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(5));
	return parameters;
}

Targets *GameScript::Player7(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(6));
	return parameters;
}

Targets *GameScript::Player7Fill(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(6));
	return parameters;
}

Targets *GameScript::Player8(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(7));
	return parameters;
}

Targets *GameScript::Player8Fill(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(7));
	return parameters;
}

Targets *GameScript::BestAC(Scriptable *Sender, Targets *parameters)
{
	int i=parameters->Count();
	int worstac=parameters->GetTarget(--i)->GetStat(IE_ARMORCLASS);
	int pos=i;
	while(i--) {
		int ac=parameters->GetTarget(pos)->GetStat(IE_ARMORCLASS);
		if(worstac>ac) {
			worstac=ac;
			pos=i;
		}
	}
	Actor *ac=parameters->GetTarget(pos);
	parameters->Clear();
	parameters->AddTarget(ac);
	return parameters;
}

Targets *GameScript::StrongestOf(Scriptable *Sender, Targets *parameters)
{
	int i=parameters->Count();
	int worsthp=parameters->GetTarget(--i)->GetStat(IE_ARMORCLASS);
	int pos=i;
	while(i--) {
		int hp=parameters->GetTarget(pos)->GetStat(IE_ARMORCLASS);
		if(worsthp<hp) {
			worsthp=hp;
			pos=i;
		}
	}
	Actor *ac=parameters->GetTarget(pos);
	parameters->Clear();
	parameters->AddTarget(ac);
	return parameters;
}

Targets *GameScript::WeakestOf(Scriptable *Sender, Targets *parameters)
{
	int i=parameters->Count();
	int worsthp=parameters->GetTarget(--i)->GetStat(IE_HITPOINTS);
	int pos=i;
	while(i--) {
		int ac=parameters->GetTarget(pos)->GetStat(IE_HITPOINTS);
		if(worsthp>ac) {
			worsthp=ac;
			pos=i;
		}
	}
	Actor *ac=parameters->GetTarget(pos);
	parameters->Clear();
	parameters->AddTarget(ac);
	return parameters;
}

Targets *GameScript::WorstAC(Scriptable *Sender, Targets *parameters)
{
	int i=parameters->Count();
	int worstac=parameters->GetTarget(--i)->GetStat(IE_ARMORCLASS);
	int pos=i;
	while(i--) {
		int ac=parameters->GetTarget(pos)->GetStat(IE_ARMORCLASS);
		if(worstac<ac) {
			worstac=ac;
			pos=i;
		}
	}
	Actor *ac=parameters->GetTarget(pos);
	parameters->Clear();
	parameters->AddTarget(ac);
	return parameters;
}

Targets *GameScript::XthNearestOf(Scriptable *Sender, Targets *parameters, int count)
{
	Actor *origin = parameters->GetTarget(count);
	parameters->Clear();
	if(!origin) {
		return parameters;
	}
	parameters->AddTarget(origin);
	return parameters;
}

Targets *GameScript::XthNearestEnemyOf(Scriptable *Sender, Targets *parameters, int count)
{
	Actor *origin = parameters->GetTarget(0);
	parameters->Clear();
	if(!origin) {
		return parameters;
	}
	//determining the allegiance of the origin
	int type = 2; //neutral, has no enemies
	if(origin->GetStat(IE_EA) <= GOODCUTOFF) {
		type=0; //PC
	}
	if(origin->GetStat(IE_EA) >= EVILCUTOFF) {
		type=1;
	}
	if(type==2) {
		return parameters;
	}
	int i = core->GetActorCount();
	Targets *tgts = new Targets();
	Actor *ac;
	while(i--) {
		ac=core->GetActor(i);
		long x = ( ac->XPos - origin->XPos );
		long y = ( ac->YPos - origin->YPos );
		double distance = sqrt( ( double ) ( x* x + y* y ) );
		if(type) { //origin is PC
			if(ac->GetStat(IE_EA) >= EVILCUTOFF) {
				tgts->AddTarget(ac);
			}
		}
		else {
			if(ac->GetStat(IE_EA) <= GOODCUTOFF) {
				tgts->AddTarget(ac);
			}
		}
	}
	if(tgts->Count()<=count) {
		delete tgts;
		return parameters;
	}
	unsigned int distance=0;
	//order by distance
	ac=tgts->GetTarget(count);
	parameters->AddTarget(ac);
	delete tgts;
	return parameters;
}

Targets *GameScript::NearestEnemyOf(Scriptable *Sender, Targets *parameters)
{
	return XthNearestEnemyOf(Sender, parameters, 0);
}

Targets *GameScript::SecondNearestEnemyOf(Scriptable *Sender, Targets *parameters)
{
	return XthNearestEnemyOf(Sender, parameters, 1);
}

Targets *GameScript::ThirdNearestEnemyOf(Scriptable *Sender, Targets *parameters)
{
	return XthNearestEnemyOf(Sender, parameters, 2);
}

Targets *GameScript::FourthNearestEnemyOf(Scriptable *Sender, Targets *parameters)
{
	return XthNearestEnemyOf(Sender, parameters, 3);
}

Targets *GameScript::FifthNearestEnemyOf(Scriptable *Sender, Targets *parameters)
{
	return XthNearestEnemyOf(Sender, parameters, 4);
}

Targets *GameScript::SixthNearestEnemyOf(Scriptable *Sender, Targets *parameters)
{
	return XthNearestEnemyOf(Sender, parameters, 5);
}

Targets *GameScript::SeventhNearestEnemyOf(Scriptable *Sender, Targets *parameters)
{
	return XthNearestEnemyOf(Sender, parameters, 6);
}

Targets *GameScript::EighthNearestEnemyOf(Scriptable *Sender, Targets *parameters)
{
	return XthNearestEnemyOf(Sender, parameters, 7);
}

Targets *GameScript::NinthNearestEnemyOf(Scriptable *Sender, Targets *parameters)
{
	return XthNearestEnemyOf(Sender, parameters, 8);
}

Targets *GameScript::TenthNearestEnemyOf(Scriptable *Sender, Targets *parameters)
{
	return XthNearestEnemyOf(Sender, parameters, 9);
}

Targets *GameScript::Nearest(Scriptable *Sender, Targets *parameters)
{
	return XthNearestOf(Sender, parameters, 0);
}

Targets *GameScript::SecondNearest(Scriptable *Sender, Targets *parameters)
{
	return XthNearestOf(Sender, parameters, 1);
}

Targets *GameScript::ThirdNearest(Scriptable *Sender, Targets *parameters)
{
	return XthNearestOf(Sender, parameters, 2);
}

Targets *GameScript::FourthNearest(Scriptable *Sender, Targets *parameters)
{
	return XthNearestOf(Sender, parameters, 3);
}

Targets *GameScript::FifthNearest(Scriptable *Sender, Targets *parameters)
{
	return XthNearestOf(Sender, parameters, 4);
}

Targets *GameScript::SixthNearest(Scriptable *Sender, Targets *parameters)
{
	return XthNearestOf(Sender, parameters, 5);
}

Targets *GameScript::SeventhNearest(Scriptable *Sender, Targets *parameters)
{
	return XthNearestOf(Sender, parameters, 6);
}

Targets *GameScript::EighthNearest(Scriptable *Sender, Targets *parameters)
{
	return XthNearestOf(Sender, parameters, 7);
}

Targets *GameScript::NinthNearest(Scriptable *Sender, Targets *parameters)
{
	return XthNearestOf(Sender, parameters, 8);
}

Targets *GameScript::TenthNearest(Scriptable *Sender, Targets *parameters)
{
	return XthNearestOf(Sender, parameters, 9);
}

//-------------------------------------------------------------
// IDS Functions
//-------------------------------------------------------------

int GameScript::ID_Alignment(Actor *actor, int parameter)
{
	int value = actor->GetStat( IE_ALIGNMENT );
	int a = parameter&15;
	if (a) {
		if (a != ( value & 15 )) {
			return 0;
		}
	}
	a = parameter & 240;
	if (a) {
		if (a != ( value & 240 )) {
			return 0;
		}
	}
	return 1;
}

int GameScript::ID_Allegiance(Actor *actor, int parameter)
{
	int value = actor->GetStat( IE_EA );
	switch (parameter) {
		case GOODCUTOFF:
			//goodcutoff
			return value <= GOODCUTOFF;

		case NOTGOOD:
			//notgood
			return value >= NOTGOOD;

		case NOTEVIL:
			//notevil
			return value <= NOTEVIL;
			break;

		case EVILCUTOFF:
			//evilcutoff
			return value >= EVILCUTOFF;

		case 0:
		case 126:
			//anything
			return true;
			break;

	}
	//default
	return parameter == value;
}

int GameScript::ID_Class(Actor *actor, int parameter)
{
	//TODO: if parameter >=202, it is of *_ALL type
	int value = actor->GetStat(IE_CLASS);
	return parameter==value;
}

int GameScript::ID_ClassMask(Actor *actor, int parameter)
{
	//TODO: if parameter >=202, it is of *_ALL type
	int value = actor->GetStat(IE_CLASS);
	return parameter==value;
}

int GameScript::ID_AVClass(Actor *actor, int parameter)
{
	//TODO: if parameter >=202, it is of *_ALL type
	int value = actor->GetStat(IE_CLASS);
	return parameter==value;
}

int GameScript::ID_Race(Actor *actor, int parameter)
{
	int value = actor->GetStat(IE_RACE);
	return parameter==value;
}

int GameScript::ID_Subrace(Actor *actor, int parameter)
{
	int value = actor->GetStat(IE_SUBRACE);
	return parameter==value;
}

int GameScript::ID_Faction(Actor *actor, int parameter)
{
	int value = actor->GetStat(IE_FACTION);
	return parameter==value;
}

int GameScript::ID_Team(Actor *actor, int parameter)
{
	int value = actor->GetStat(IE_TEAM);
	return parameter==value;
}

int GameScript::ID_Gender(Actor *actor, int parameter)
{
	int value = actor->GetStat(IE_SEX);
	return parameter==value;
}

int GameScript::ID_General(Actor *actor, int parameter)
{
	int value = actor->GetStat(IE_GENERAL);
	return parameter==value;
}

int GameScript::ID_Specific(Actor *actor, int parameter)
{
	int value = actor->GetStat(IE_SPECIFIC);
	return parameter==value;
}

//-------------------------------------------------------------
// Trigger Functions
//-------------------------------------------------------------
int GameScript::BreakingPoint(Scriptable* Sender, Trigger* parameters)
{
	int value=GetHappiness(Sender, core->GetGame()->Reputation );
	return value < -300;
}

int GameScript::Happiness(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	int value=GetHappiness(Sender, core->GetGame()->Reputation );
	return value == parameters->int0Parameter;
}

int GameScript::HappinessGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	int value=GetHappiness(Sender, core->GetGame()->Reputation );
	return value > parameters->int0Parameter;
}

int GameScript::HappinessLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	int value=GetHappiness(Sender, core->GetGame()->Reputation );
	return value < parameters->int0Parameter;
}

int GameScript::Reputation(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	return core->GetGame()->Reputation == parameters->int0Parameter;
}

int GameScript::ReputationGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	return core->GetGame()->Reputation > parameters->int0Parameter;
}

int GameScript::ReputationLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	return core->GetGame()->Reputation < parameters->int0Parameter;
}

int GameScript::Alignment(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	return ID_Alignment( actor, parameters->int0Parameter);
}

int GameScript::Allegiance(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return false;
	}
	Actor* actor = ( Actor* ) scr;
	return ID_Allegiance( actor, parameters->int0Parameter);
}

int GameScript::Class(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if(!scr) {
		scr = Sender;
	}
	if(scr->Type != ST_ACTOR)
		return 0;
	Actor * actor = (Actor*)scr;
	return ID_Class( actor, parameters->int0Parameter);
}

//atm this checks for InParty and See, it is unsure what is required
int GameScript::IsValidForPartyDialog(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	//return actor->InParty?1:0; //maybe ???
	if (!core->GetGame()->InParty( ( Actor * ) scr )) {
		return 0;
	}
	int range;
	if (Sender->Type != ST_ACTOR) {
		//non actors got no visual range, needs research
		range = 20 * 20;
	}
	else {
		Actor* snd = ( Actor* ) Sender;
		range = snd->Modified[IE_VISUALRANGE] * 20;
	}
	long x = ( scr->XPos - Sender->XPos );
	long y = ( scr->YPos - Sender->YPos );
	double distance = sqrt( ( double ) ( x* x + y* y ) );
	if (distance > range) {
		return 0;
	}
	if (!core->GetPathFinder()->IsVisible( Sender->XPos, Sender->YPos,
						scr->XPos, scr->YPos )) {
		return 0;
	}
	//further checks, is target alive and talkative
	return 1;
}

int GameScript::InParty(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor *tar = (Actor *) scr;
	if(core->GetGame()->InParty( tar ) <0) {
		return 0;
	}
	return (tar->GetStat(IE_STATE_ID)&STATE_DEAD)!=0;
}

int GameScript::InPartyAllowDead(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	return core->GetGame()->InParty( ( Actor * ) scr ) >= 0 ? 1 : 0;
}

int GameScript::InPartySlot(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	return actor->InParty == parameters->int0Parameter;
}

int GameScript::Exists(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	return 1;
}

int GameScript::General(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	if (actor == NULL) {
		return 0;
	}
	return ID_General(actor, parameters->int0Parameter);
}

int GameScript::Specific(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	if (actor == NULL) {
		return 0;
	}
	return ID_Specific(actor, parameters->int0Parameter);
}

int GameScript::BitCheck(Scriptable* Sender, Trigger* parameters)
{
	unsigned long value = CheckVariable(Sender, parameters->string0Parameter );
	int eval = ( value& parameters->int0Parameter ) ? 1 : 0;
	return eval;
}

int GameScript::Global(Scriptable* Sender, Trigger* parameters)
{
	unsigned long value = CheckVariable(Sender, parameters->string0Parameter );
	int eval = ( value == parameters->int0Parameter ) ? 1 : 0;
	return eval;
}

int GameScript::GlobalLT(Scriptable* Sender, Trigger* parameters)
{
	unsigned long value = CheckVariable(Sender, parameters->string0Parameter );
	int eval = ( value < parameters->int0Parameter ) ? 1 : 0;
	return eval;
}

int GameScript::GlobalGT(Scriptable* Sender, Trigger* parameters)
{
	unsigned long value = CheckVariable(Sender, parameters->string0Parameter );
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

int GameScript::GlobalTimerExact(Scriptable* Sender, Trigger* parameters)
{
	unsigned long value1 = CheckVariable( Sender,
							parameters->string0Parameter );
	unsigned long value2;

	GetTime(value2);
	int eval = ( value1 == value2 ) ? 1 : 0;
	return eval;
}

int GameScript::GlobalTimerExpired(Scriptable* Sender, Trigger* parameters)
{
	unsigned long value1 = CheckVariable( Sender,
							parameters->string0Parameter );
	unsigned long value2;

	GetTime(value2);
	int eval = ( value1 < value2 ) ? 1 : 0;
	return eval;
}

int GameScript::GlobalTimerNotExpired(Scriptable* Sender, Trigger* parameters)
{
	unsigned long value1 = CheckVariable( Sender,
							parameters->string0Parameter );
	unsigned long value2;

	GetTime(value2);
	int eval = ( value1 > value2 ) ? 1 : 0;
	return eval;
}

int GameScript::OnCreation(Scriptable* Sender, Trigger* parameters)
{
	return Sender->OnCreation;
/* oncreation is about the script, not the owner area, oncreation is
   working in ANY script */
/*
	Map* area = core->GetGame()->GetMap( 0 );
	if (area->justCreated) {
		area->justCreated = false;
		return 1;
	}
	return 0;
*/
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
		scr = Sender;
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
		scr = Sender;
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
		scr = Sender;
	}
	if (scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	return actor->TalkCount < parameters->int0Parameter ? 1 : 0;
}

/* this single function works for ActionListEmpty and ObjectActionListEmpty */
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

int GameScript::ObjectActionListEmpty(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	if (Sender->GetNextAction()) {
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
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	printf( "x1 = %d, y1 = %d\nx2 = %d, y2 = %d\n", scr->XPos,
		scr->YPos, Sender->XPos, Sender->YPos );
	long x = ( scr->XPos - Sender->XPos );
	long y = ( scr->YPos - Sender->YPos );
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
	if (parameters->objectParameter->objectFields[0] == 0) {
		return 1;
	Scriptable* target = GetActorFromObject( Sender,
							parameters->objectParameter );
	if (Sender == target)
		return 1;
	}
	return 0;
}

int GameScript::Entered(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_PROXIMITY) {
		return 0;
	}
	if (parameters->objectParameter->objectFields[0] == 0) {
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
	Scriptable* target = GetActorFromObject( Sender,
							parameters->objectParameter );
	if (!target) {
		return 0;
	}
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
	return ID_Race(actor, parameters->int0Parameter);
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
	return ID_Gender(actor, parameters->int0Parameter);
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

int GameScript::HPPercent(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if(!scr) {
		return 0;
	}
	if (GetHPPercent( scr ) == parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::HPPercentGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if(!scr) {
		return 0;
	}
	if (GetHPPercent( scr ) > parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::HPPercentLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if(!scr) {
		return 0;
	}
	if (GetHPPercent( scr ) < parameters->int0Parameter) {
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

int GameScript::SeeCore(Scriptable* Sender, Trigger* parameters, int justlos)
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
	/* don't set LastSeen if this isn't an actor */
	if (target->Type !=ST_ACTOR) {
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
		if(justlos) {
			return 1;
		}
		//additional checks for invisibility?
		snd->LastSeen = (Actor *) target;
		return 1;
	}
	return 0;
}

int GameScript::See(Scriptable* Sender, Trigger* parameters)
{
	return SeeCore(Sender, parameters, 0);
}

int GameScript::LOS(Scriptable* Sender, Trigger* parameters)
{
	int see=SeeCore(Sender, parameters, 1);
	if(!see) {
		return 0;
	}
	return Range(Sender, parameters); //same as range
}

int GameScript::NumCreatures(Scriptable* Sender, Trigger* parameters)
{
	int value = GetObjectCount(Sender, parameters->objectParameter);
	return value == parameters->int0Parameter;
}

int GameScript::NumCreaturesLT(Scriptable* Sender, Trigger* parameters)
{
	int value = GetObjectCount(Sender, parameters->objectParameter);
	return value < parameters->int0Parameter;
}

int GameScript::NumCreaturesGT(Scriptable* Sender, Trigger* parameters)
{
	int value = GetObjectCount(Sender, parameters->objectParameter);
	return value > parameters->int0Parameter;
}

int GameScript::Morale(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetStat(IE_MORALEBREAK) == parameters->int0Parameter;
}

int GameScript::MoraleGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetStat(IE_MORALEBREAK) > parameters->int0Parameter;
}

int GameScript::MoraleLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetStat(IE_MORALEBREAK) < parameters->int0Parameter;
}

int GameScript::StateCheck(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetStat(IE_STATE_ID) & parameters->int0Parameter;
}

int GameScript::NotStateCheck(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetStat(IE_STATE_ID) & ~parameters->int0Parameter;
}

int GameScript::RandomNum(Scriptable* Sender, Trigger* parameters)
{
	if(parameters->int0Parameter<0) {
		return 0;
	}
	if(parameters->int1Parameter<0) {
		return 0;
	}
	return parameters->int1Parameter-1 == RandomNumValue%parameters->int0Parameter;
}

int GameScript::RandomNumGT(Scriptable* Sender, Trigger* parameters)
{
	if(parameters->int0Parameter<0) {
		return 0;
	}
	if(parameters->int1Parameter<0) {
		return 0;
	}
	return parameters->int1Parameter-1 == RandomNumValue%parameters->int0Parameter;
}

int GameScript::RandomNumLT(Scriptable* Sender, Trigger* parameters)
{
	if(parameters->int0Parameter<0) {
		return 0;
	}
	if(parameters->int1Parameter<0) {
		return 0;
	}
	return parameters->int1Parameter-1 == RandomNumValue%parameters->int0Parameter;
}

int GameScript::OpenState(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if(!tar) {
		printf("[IEScript]: couldn't find door/container:%s\n",parameters->objectParameter->objectName);
		return 0;
	}
	switch(tar->Type) {
		case ST_DOOR:
		{
			Door *door =(Door *) tar;
			return (door->Flags&1) == parameters->int0Parameter;
		}
		case ST_CONTAINER:
		{
			Container *cont = (Container *) tar;
			return cont->Locked == parameters->int0Parameter;
		}
		default:
			printf("[IEScript]: couldn't find door/container:%s\n",parameters->string0Parameter);
			return 0;
	}
}

int GameScript::Level(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetStat(IE_LEVEL) == parameters->int0Parameter;
}

int GameScript::LevelGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetStat(IE_LEVEL) > parameters->int0Parameter;
}

int GameScript::LevelLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetStat(IE_LEVEL) < parameters->int0Parameter;
}

int GameScript::UnselectableVariable(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetStat(IE_UNSELECTABLE) == parameters->int0Parameter;
}

int GameScript::UnselectableVariableGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetStat(IE_UNSELECTABLE) > parameters->int0Parameter;
}

int GameScript::UnselectableVariableLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetStat(IE_UNSELECTABLE) < parameters->int0Parameter;
}

int GameScript::AreaCheck(Scriptable* Sender, Trigger* parameters)
{
	if(Sender->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) Sender;
	return strnicmp(actor->Area, parameters->string0Parameter, 8)==0;
}

int GameScript::InMyArea(Scriptable* Sender, Trigger* parameters)
{
	if(Sender->Type != ST_ACTOR) {
		return 0;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor1 = ( Actor* ) Sender;
	Actor* actor2 = ( Actor* ) tar;
	
	return strnicmp(actor1->Area, actor2->Area, 8)==0;
}

int GameScript::InActiveArea(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor2 = ( Actor* ) tar;
	return strnicmp(core->GetGame()->CurrentArea, actor2->Area, 8) ==0;
}

int GameScript::AreaType(Scriptable* Sender, Trigger* parameters)
{
	Map *map=core->GetGame()->GetCurrentMap();
	return (map->AreaType&parameters->int0Parameter)>0;
}

int GameScript::AreaFlag(Scriptable* Sender, Trigger* parameters)
{
	Map *map=core->GetGame()->GetCurrentMap();
	return (map->AreaFlags&parameters->int0Parameter)>0;
}

//-------------------------------------------------------------
// Action Functions
//-------------------------------------------------------------

void GameScript::SetAreaRestFlag(Scriptable* /*Sender*/, Action* parameters)
{
	Map *map=core->GetGame()->GetCurrentMap();
	//either whole number, or just the lowest bit, needs research
	map->AreaFlags=parameters->int0Parameter;
}

void GameScript::AddAreaFlag(Scriptable* /*Sender*/, Action* parameters)
{
	Map *map=core->GetGame()->GetCurrentMap();
	map->AreaFlags|=parameters->int0Parameter;
}

void GameScript::RemoveAreaFlag(Scriptable* /*Sender*/, Action* parameters)
{
	Map *map=core->GetGame()->GetCurrentMap();
	map->AreaFlags&=~parameters->int0Parameter;
}

void GameScript::AddAreaType(Scriptable* /*Sender*/, Action* parameters)
{
	Map *map=core->GetGame()->GetCurrentMap();
	map->AreaType|=parameters->int0Parameter;
}

void GameScript::RemoveAreaType(Scriptable* /*Sender*/, Action* parameters)
{
	Map *map=core->GetGame()->GetCurrentMap();
	map->AreaType&=~parameters->int0Parameter;
}

void GameScript::NoAction(Scriptable* /*Sender*/, Action* /*parameters*/)
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

void GameScript::SetGlobalTimer(Scriptable* Sender, Action* parameters)
{
	unsigned long mytime;

	printf( "SetGlobalTimer(\"%s\")\n", parameters->string0Parameter,
		parameters->int0Parameter );
	GetTime(mytime);
	SetVariable( Sender, parameters->string0Parameter,
		parameters->int0Parameter + mytime);
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
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->SetStat( IE_SPECIFIC, parameters->int0Parameter );
}

void GameScript::ChangeStat(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	if (tar->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) tar;
	actor->NewStat( parameters->int0Parameter, parameters->int1Parameter,
			parameters->int2Parameter );
}

void GameScript::ChangeGender(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->SetStat( IE_SEX, parameters->int0Parameter );
}

void GameScript::ChangeAlignment(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->SetStat( IE_ALIGNMENT, parameters->int0Parameter );
}

void GameScript::TriggerActivation(Scriptable* Sender, Action* parameters)
{
	Scriptable* ip;

	if(!parameters->objects[1]->objectName[0]) {
		ip=Sender;
	} else {
		ip = core->GetGame()->GetMap(0)->tm->GetInfoPoint(parameters->objects[1]->objectName);
	}
	if(!ip) {
		printf("Script error: No Trigger Named \"%s\"\n", parameters->objects[1]->objectName);
		return;
	}
	ip->Active = parameters->int0Parameter;
}

void GameScript::FadeToColor(Scriptable* Sender, Action* parameters)
{
	core->timer->SetFadeToColor( parameters->XpointParameter );
}

void GameScript::FadeFromColor(Scriptable* Sender, Action* parameters)
{
	core->timer->SetFadeFromColor( parameters->XpointParameter );
}

void GameScript::JumpToPoint(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* ab = ( Actor* ) Sender;
	ab->SetPosition( parameters->XpointParameter, parameters->YpointParameter );
}

void GameScript::JumpToPointInstant(Scriptable* Sender, Action* parameters)
{
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
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );

	Actor* ab = ( Actor* ) Sender;
	ab->SetPosition( tar->XPos, tar->YPos );
}

int GameScript::GetHappiness(Scriptable* Sender, int reputation)
{
	if(Sender->Type != ST_ACTOR) {
		return 0;
	}
	Actor* ab = ( Actor* ) Sender;
	int alignment = ab->GetStat(IE_ALIGNMENT)&3; //good, neutral, evil
	int hptable = core->LoadTable( "happy" );
	char * repvalue = core->GetTable( hptable )->QueryField( reputation/10, alignment );
	core->DelTable( hptable );
	return atoi(repvalue);
}

int GameScript::GetHPPercent(Scriptable* Sender)
{
	if(Sender->Type != ST_ACTOR) {
		return 0;
	}
	Actor* ab = ( Actor* ) Sender;
	int hp1 = ab->GetStat(IE_MAXHITPOINTS);
	if(hp1<1) {
		return 0;
	}
	int hp2 = ab->GetStat(IE_HITPOINTS);
	if(hp2<1) {
		return 0;
	}
	return hp2*100/hp1;
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
	switch (flags & CC_MASK) {
		case CC_OBJECT:
			Sender = GetActorFromObject( Sender, parameters->objects[1] );
			x += Sender->XPos;
			y += Sender->YPos;
		case CC_OFFSET:
			x += Sender->XPos;
			y += Sender->YPos;
	}
	ab->SetPosition( parameters->XpointParameter, parameters->YpointParameter );
	ab->AnimID = IE_ANI_AWAKE;
	ab->Orientation = parameters->int0Parameter;
	Map* map = core->GetGame()->GetMap( 0 );
	map->AddActor( ab );

	//setting the deathvariable if it exists (iwd2)
	if(parameters->string1Parameter[0]) {
		ab->SetScriptName(parameters->string1Parameter);
	}
	core->FreeInterface( aM );
}

void GameScript::CreateCreature(Scriptable* Sender, Action* parameters)
{
	CreateCreatureCore( Sender, parameters, 0 );
}

void GameScript::CreateCreatureOffset(Scriptable* Sender, Action* parameters)
{
	CreateCreatureCore( Sender, parameters, CC_OFFSET );
}

void GameScript::CreateCreatureObject(Scriptable* Sender, Action* parameters)
{
	CreateCreatureCore( Sender, parameters, CC_OBJECT );
}

//this is the same, object + offset
void GameScript::CreateCreatureObjectOffset(Scriptable* Sender, Action* parameters)
{
	CreateCreatureCore( Sender, parameters, CC_OBJECT );
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
	if(parameters->objects[1]->objectName[0]) {
		Map* map = core->GetGame()->GetMap( 0 );
		Sender->CutSceneId = map->GetActor( parameters->objects[1]->objectName );
	} else {
		Sender->CutSceneId = GetActorFromObject( Sender,
								parameters->objects[1] );
	}
}

void GameScript::Enemy(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->SetStat( IE_EA, 255 );
}

void GameScript::Ally(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->SetStat( IE_EA, 4 );
}

void GameScript::ChangeAIScript(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	//changeaiscript clears the queue, i believe
	//	actor->ClearActions();
	actor->SetScript( parameters->string0Parameter, parameters->int0Parameter );
}

void GameScript::ForceAIScript(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (tar->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) tar;
	//changeaiscript clears the queue, i believe
	//	actor->ClearActions();
	actor->SetScript( parameters->string0Parameter, parameters->int0Parameter );
}

void GameScript::SetPlayerSound(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (tar->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) tar;
	actor->StrRefs[parameters->int0Parameter]=parameters->int1Parameter;
}

void GameScript::VerbalConstant(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	if (actor) {
		printf( "Displaying string on: %s\n", actor->scriptName );
		actor->DisplayHeadText( core->GetString( actor->StrRefs[parameters->int0Parameter], 2 ) );
	}
}

void GameScript::SaveLocation(Scriptable* Sender, Action* parameters)
{
	unsigned int value;

	*((unsigned short *) &value) = parameters->XpointParameter;
	*(((unsigned short *) &value)+1) = (unsigned short) parameters->YpointParameter;
	SetVariable(Sender, parameters->string0Parameter, value);
}

void GameScript::SaveObjectLocation(Scriptable* Sender, Action* parameters)
{
	unsigned int value;

	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	*((unsigned short *) &value) = tar->XPos;
	*(((unsigned short *) &value)+1) = (unsigned short) tar->YPos;
	SetVariable(Sender, parameters->string0Parameter, value);
}

void GameScript::CreateCreatureAtLocation(Scriptable* Sender, Action* parameters)
{
	unsigned int value = CheckVariable(Sender, parameters->string0Parameter);
	parameters->XpointParameter = *(unsigned short *) value;
	parameters->XpointParameter = *(((unsigned short *) value)+1);
	CreateCreatureCore(Sender, parameters, CC_OFFSET);
}

void GameScript::WaitRandom(Scriptable* Sender, Action* parameters)
{
	int width = parameters->int1Parameter-parameters->int0Parameter;
	if(width<2) {
		width = parameters->int0Parameter;
	}
	else {
		width = rand() % width + parameters->int0Parameter;
	}
	Sender->SetWait( width * AI_UPDATE_TIME );
}

void GameScript::Wait(Scriptable* Sender, Action* parameters)
{
	Sender->SetWait( parameters->int0Parameter * AI_UPDATE_TIME );
}

void GameScript::SmallWait(Scriptable* Sender, Action* parameters)
{
	Sender->SetWait( parameters->int0Parameter );
}

void GameScript::MoveViewPoint(Scriptable* Sender, Action* parameters)
{
	core->GetVideoDriver()->MoveViewportTo( parameters->XpointParameter,
								parameters->YpointParameter );
}

void GameScript::MoveViewObject(Scriptable* Sender, Action* parameters)
{
	Scriptable * scr = GetActorFromObject( Sender, parameters->objects[1]);
	core->GetVideoDriver()->MoveViewportTo( scr->XPos, scr->YPos );
}

void GameScript::AddWayPoint(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->AddWayPoint( parameters->XpointParameter, parameters->YpointParameter );
}

void GameScript::MoveToPoint(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->WalkTo( parameters->XpointParameter, parameters->YpointParameter );
	//core->timer->SetMovingActor(actor);
}

void GameScript::MoveToObject(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	Scriptable* target = GetActorFromObject( Sender, parameters->objects[1] );
	if (!target) {
		Sender->CurrentAction = NULL;
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->WalkTo( target->XPos, target->YPos );
}

void GameScript::MoveToOffset(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->WalkTo( Sender->XPos+parameters->XpointParameter, Sender->YPos+parameters->YpointParameter );
}

void GameScript::RunAwayFrom(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	actor->RunAwayFrom( tar->XPos, tar->YPos, parameters->int0Parameter, false);
}

void GameScript::RunAwayFromNoInterrupt(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	//nointerrupt???
	actor->RunAwayFrom( tar->XPos, tar->YPos, parameters->int0Parameter, false);
}

void GameScript::RunAwayFromPoint(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->RunAwayFrom( parameters->XpointParameter, parameters->YpointParameter, parameters->int0Parameter, false);
}
void GameScript::DisplayStringNoNameHead(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	if (actor) {
		printf( "Displaying string on: %s (without name)\n", actor->scriptName );
		actor->DisplayHeadText( core->GetString( parameters->int0Parameter, 2 ) );
	}
}

void GameScript::DisplayStringHead(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	if (actor) {
		printf( "Displaying string on: %s\n", actor->scriptName );
		actor->DisplayHeadText( core->GetString( parameters->int0Parameter, 2 ) );
	}
}

void GameScript::Face(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	Actor* actor = ( Actor* ) Sender;
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
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Scriptable* target = GetActorFromObject( Sender, parameters->objects[1] );
	if (!target) {
		Sender->CurrentAction = NULL;
		return;
	}
	Actor* actor = ( Actor* ) Sender;
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
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
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
	printf( "PlaySound(%s)\n", parameters->string0Parameter );
	core->GetSoundMgr()->Play( parameters->string0Parameter, Sender->XPos,
							Sender->YPos );
}

void GameScript::CreateVisualEffectObject(Scriptable* Sender,
	Action* parameters)
{
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
	DataStream* ds = core->GetResourceMgr()->GetResource( parameters->string0Parameter,
												IE_VVC_CLASS_ID );
	ScriptedAnimation* vvc = new ScriptedAnimation( ds, true,
									parameters->XpointParameter,
									parameters->YpointParameter );
	core->GetGame()->GetMap( 0 )->AddVVCCell( vvc );
}

void GameScript::DestroySelf(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
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
	GameControl* gc = ( GameControl* ) core->GetWindow( 0 )->GetControl( 0 );	
	if (gc->ControlType == IE_GUI_GAMECONTROL) {
		gc->UnhideGUI();
	}
	EndCutSceneMode( Sender, parameters );
}

void GameScript::HideGUI(Scriptable* Sender, Action* parameters)
{
	GameControl* gc = ( GameControl* ) core->GetWindow( 0 )->GetControl( 0 );	
	if (gc->ControlType == IE_GUI_GAMECONTROL) {
		gc->HideGUI();
	}
}

static char PlayerDialogRes[9] = "PLAYERx\0";

void GameScript::BeginDialog(Scriptable* Sender, Action* parameters, int Flags)
{
	Scriptable* tar, *scr;

	printf("BeginDialog core\n");
	if (Flags & BD_OWN) {
		scr = tar = GetActorFromObject( Sender, parameters->objects[1] );
	} else {
		tar = GetActorFromObject( Sender, parameters->objects[1] );
		scr = Sender;
	}
	if(!tar) {
		printf("[IEScript]: Target for dialog couldn't be found.\n");
		Sender->CurrentAction = NULL;
		abort();
		return;
	}
	//source could be other than Actor, we need to handle this too!
	if (tar->Type != ST_ACTOR) {
		Sender->CurrentAction = NULL;
		return;
	}
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
			Dialog = actor->Dialog;
			break;
		case BD_TARGET:
			Dialog = target->Dialog;
			break;
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
	if (Sender->overHeadText) {
		free( Sender->overHeadText );
	}
	Sender->overHeadText = core->GetString( parameters->int0Parameter );
	GetTime( Sender->timeStartDisplaying );
	Sender->textDisplaying = 0;
	GameControl* gc = ( GameControl* ) core->GetWindow( 0 )->GetControl( 0 );	
	if (gc->ControlType == IE_GUI_GAMECONTROL) {
		gc->DisplayString( Sender );
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

void GameScript::PlaySequence(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* target = ( Actor* ) Sender;
	target->AnimID = parameters->int0Parameter;
}

void GameScript::SetAnimState(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	if (tar->Type != ST_ACTOR) {
		return;
	}
	Actor* target = ( Actor* ) tar;
	target->AnimID = parameters->int0Parameter;
}

void GameScript::SetDialogue(Scriptable* Sender, Action* parameters)
{
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
	BeginDialog( Sender, parameters, BD_TALKCOUNT | BD_SOURCE );
}

void GameScript::StartDialogueNoSetInterrupt(Scriptable* Sender,
	Action* parameters)
{
	BeginDialog( Sender, parameters, BD_TALKCOUNT | BD_SOURCE | BD_INTERRUPT );
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

void GameScript::Lock(Scriptable* Sender, Action* parameters)
{
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
	door->SetDoorLocked( false, false);
}

void GameScript::Unlock(Scriptable* Sender, Action* parameters)
{
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
	door->SetDoorLocked( false, true);
}

void GameScript::OpenDoor(Scriptable* Sender, Action* parameters)
{
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
	if(!(door->Flags&1) ) {
		//door is already open
		Sender->CurrentAction = NULL;
		return;
	}
	if (Sender->Type != ST_ACTOR) {
		//if not an actor opens, it don't play sound
		door->SetDoorClosed( false, false );		
		Sender->CurrentAction = NULL;
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	double distance;
	Point* p = FindNearPoint( actor, &door->toOpen[0], &door->toOpen[1],
				distance );
	if (distance <= 12) {
		if(door->Flags&2) {
			//playsound unsuccessful opening of door
		}
		else {
			door->SetDoorClosed( false, true );
		}
	} else {
		Sender->AddActionInFront( Sender->CurrentAction );
		char Tmp[256];
		sprintf( Tmp, "MoveToPoint([%d.%d])", p->x, p->y );
		actor->AddActionInFront( GameScript::CreateAction( Tmp, true ) );
	}
	Sender->CurrentAction = NULL;
}

void GameScript::CloseDoor(Scriptable* Sender, Action* parameters)
{
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
	if(door->Flags&1) {
		//door is already open
		Sender->CurrentAction = NULL;
		return;
	}
	if (Sender->Type != ST_ACTOR) {
		//if not an actor opens, it don't play sound
		door->SetDoorClosed( true, false );
		Sender->CurrentAction = NULL;
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	double distance;
	Point* p = FindNearPoint( actor, &door->toOpen[0], &door->toOpen[1],
				distance );	
	if (distance <= 12) {
		door->SetDoorClosed( true, true );
	} else {
		Sender->AddActionInFront( Sender->CurrentAction );
		char Tmp[256];
		sprintf( Tmp, "MoveToPoint([%d.%d])", p->x, p->y );
		actor->AddActionInFront( GameScript::CreateAction( Tmp, true ) );
	}
	Sender->CurrentAction = NULL;
}

void GameScript::MoveBetweenAreas(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
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
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	unsigned short sX,sY, dX,dY;
	GetPositionFromScriptable( Sender, sX, sY );
	GetPositionFromScriptable( tar, dX, dY );
	printf( "ForceSpell from [%d,%d] to [%d,%d]\n", sX, sY, dX, dY );
}

void GameScript::Deactivate(Scriptable* Sender, Action* parameters)
{
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
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* act = ( Actor* ) Sender;
	if (!core->GetGame()->InParty( act )) {
		core->GetGame()->AddNPC( act );
	}
}

void GameScript::UnMakeGlobal(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* act = ( Actor* ) Sender;
	int slot;
	slot = core->GetGame()->InStore( act );
	if (slot >= 0) {
		core->GetGame()->DelNPC( slot );
	}
}

void GameScript::GivePartyGoldGlobal(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* act = ( Actor* ) Sender;
	unsigned long gold = CheckVariable( Sender, parameters->string0Parameter );
	act->NewStat(IE_GOLD, -gold, 0);
	core->GetGame()->PartyGold+=gold;
}

void GameScript::CreatePartyGold(Scriptable* Sender, Action* parameters)
{
	core->GetGame()->PartyGold+=parameters->int0Parameter;
}

void GameScript::GivePartyGold(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* act = ( Actor* ) Sender;
	int gold = act->GetStat(IE_GOLD);
	if(gold>parameters->int0Parameter) {
		gold=parameters->int0Parameter;
	}
	act->NewStat(IE_GOLD, -gold, 0);
	core->GetGame()->PartyGold+=gold;
}

void GameScript::DestroyPartyGold(Scriptable* Sender, Action* parameters)
{
	int gold = core->GetGame()->PartyGold;
	if(gold>parameters->int0Parameter) {
		gold=parameters->int0Parameter;
	}
	core->GetGame()->PartyGold-=gold;
}

void GameScript::TakePartyGold(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* act = ( Actor* ) Sender;
	int gold = core->GetGame()->PartyGold;
	if(gold>parameters->int0Parameter) {
		gold=parameters->int0Parameter;
	}
	act->NewStat(IE_GOLD, gold, 0);
	core->GetGame()->PartyGold-=gold;
}

void GameScript::AddXPObject(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	if (tar->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) tar;
	actor->NewStat(IE_XP, parameters->int0Parameter, 0);
}

void GameScript::AddXP2DA(Scriptable* Sender, Action* parameters)
{
	int xptable;
	
	if(core->HasFeature(GF_HAS_EXPTABLE) ) {
		xptable = core->LoadTable("exptable");
	}
	else {
		xptable = core->LoadTable( "xplist" );
	}
	
	if(parameters->int0Parameter>0) {
		//display string
	}
	char * xpvalue = core->GetTable( xptable )->QueryField( parameters->string0Parameter, "0" ); //level is unused
	
	if( xpvalue[0]=='P' && xpvalue[1]=='_') {
		core->GetGame()->ShareXP(atoi(xpvalue+2) );
	}
	else {
		Actor* actor = ( Actor* ) core->GetGame()->GetPC(0);
		actor->NewStat(IE_XP, atoi(xpvalue), 0);
	}
	core->DelTable( xptable );
}

void GameScript::AddExperienceParty(Scriptable* Sender, Action* parameters)
{
	core->GetGame()->ShareXP(parameters->int0Parameter);
}

void GameScript::AddExperiencePartyGlobal(Scriptable* Sender, Action* parameters)
{
	unsigned long xp = CheckVariable( Sender, parameters->string0Parameter );
	core->GetGame()->ShareXP(xp);
}

void GameScript::MoraleSet(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* act = ( Actor* ) Sender;
	act->NewStat(IE_MORALEBREAK, parameters->int0Parameter, 1);
}

void GameScript::MoraleInc(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* act = ( Actor* ) Sender;
	act->NewStat(IE_MORALEBREAK, parameters->int0Parameter, 0);
}

void GameScript::MoraleDec(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* act = ( Actor* ) Sender;
	act->NewStat(IE_MORALEBREAK, -parameters->int0Parameter, 0);
}

void GameScript::JoinParty(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* act = ( Actor* ) Sender;
	core->GetGame()->JoinParty( act );
	act->SetStat( IE_EA, PC );
	if(core->HasFeature( GF_HAS_DPLAYER ))  {
		act->SetScript( "DPLAYER2", SCR_DEFAULT );
	}
	if(core->HasFeature( GF_HAS_PDIALOG )) {
		int pdtable = core->LoadTable( "pdialog" );
		char* scriptingname = act->GetScriptName();
		act->SetDialog( core->GetTable( pdtable )->QueryField( scriptingname,
				"JOIN_DIALOG_FILE" ) );
		core->DelTable( pdtable );
	}
}

void GameScript::LeaveParty(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* act = ( Actor* ) Sender;
	core->GetGame()->LeaveParty( act );
	act->SetStat( IE_EA, NEUTRAL );
	act->SetScript( "", SCR_DEFAULT );
	if(core->HasFeature( GF_HAS_PDIALOG )) {
		int pdtable = core->LoadTable( "pdialog" );
		char* scriptingname = act->GetScriptName();
		act->SetDialog( core->GetTable( pdtable )->QueryField( scriptingname,
				"POST_DIALOG_FILE" ) );
		core->DelTable( pdtable );
	}
}

void GameScript::Activate(Scriptable* Sender, Action* parameters)
{
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
	if (Sender->Type != ST_ACTOR) {
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
	if (Sender->Type != ST_ACTOR) {
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
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->AnimID = IE_ANI_DIE;
	//also set time for playdead!
	actor->SetWait( parameters->int0Parameter );
}

void GameScript::Swing(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->AnimID = IE_ANI_ATTACK;
	actor->SetWait( 1 );
}

void GameScript::Recoil(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->AnimID = IE_ANI_DAMAGE;
	actor->SetWait( 1 );
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

void GameScript::GlobalXorGlobal(Scriptable* Sender, Action* parameters)
{
	unsigned long value1 = CheckVariable( Sender,
							parameters->string0Parameter );
	unsigned long value2 = CheckVariable( Sender,
							parameters->string1Parameter );
	SetVariable( Sender, parameters->string0Parameter, value1 ^ value2 );
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

void GameScript::GlobalXor(Scriptable* Sender, Action* parameters)
{
	unsigned long value1 = CheckVariable( Sender,
							parameters->string0Parameter );
	SetVariable( Sender, parameters->string0Parameter,
		value1 ^ parameters->int0Parameter );
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
	Sender->ClearActions();
}

void GameScript::SetNumTimesTalkedTo(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->TalkCount = parameters->int0Parameter;
}

void GameScript::StartMovie(Scriptable* Sender, Action* parameters)
{
	core->PlayMovie( parameters->string0Parameter );
}

void GameScript::SetLeavePartyDialogFile(Scriptable* Sender,
	Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	int pdtable = core->LoadTable( "pdialog" );
	Actor* act = ( Actor* ) Sender;
	char* scriptingname = act->GetScriptName();
	act->SetDialog( core->GetTable( pdtable )->QueryField( scriptingname,
			"POST_DIALOG_FILE" ) );
	core->DelTable( pdtable );
}

