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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/GameScript.cpp,v 1.153 2004/04/22 20:44:07 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "GameScript.h"
#include "Interface.h"

extern Interface* core;
#ifdef WIN32
extern HANDLE hConsole;
#endif

//this will skip to the next element in the prototype of an action/trigger
#define SKIP_ARGUMENT() while(*str && ( *str != ',' ) && ( *str != ')' )) str++

//later this could be separated from talking distance
#define MAX_OPERATING_DISTANCE      40

static int initialized = 0;
static SymbolMgr* triggersTable;
static SymbolMgr* actionsTable;
static SymbolMgr* objectsTable;
static TriggerFunction triggers[MAX_TRIGGERS];
static ActionFunction actions[MAX_ACTIONS];
static short actionflags[MAX_ACTIONS];
static short triggerflags[MAX_TRIGGERS];
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
	{"actionlistempty", GameScript::ActionListEmpty,0},
	{"acquired", GameScript::Acquired,0},
	{"alignment", GameScript::Alignment,0},
	{"allegiance", GameScript::Allegiance,0},
	{"areacheck", GameScript::AreaCheck,0},
	{"areacheckobject", GameScript::AreaCheck,0},
	{"areatype", GameScript::AreaType,0},
	{"areaflag", GameScript::AreaFlag,0},
	{"bitcheck", GameScript::BitCheck,TF_MERGESTRINGS},
	{"bitcheckexact", GameScript::BitCheckExact,TF_MERGESTRINGS},
	{"bitglobal", GameScript::BitGlobal_Trigger,TF_MERGESTRINGS},
	{"breakingpoint", GameScript::BreakingPoint,0},
	{"calledbyname", GameScript::CalledByName,0},
	{"checkstat", GameScript::CheckStat,0},
	{"checkstatgt", GameScript::CheckStatGT,0},
	{"checkstatlt", GameScript::CheckStatLT,0},
	{"class", GameScript::Class,0},
	{"classlevel", GameScript::ClassLevel,0},
	{"classlevelgt", GameScript::ClassLevelGT,0},
	{"classlevellt", GameScript::ClassLevelLT,0},
	{"clicked", GameScript::Clicked,0},
	{"combatcounter", GameScript::CombatCounter,0},
	{"combatcountergt", GameScript::CombatCounterGT,0},
	{"combatcounterlt", GameScript::CombatCounterLT,0},
	{"dead", GameScript::Dead,0},
	{"entered", GameScript::Entered,0},
	{"exists", GameScript::Exists,0},
	{"extraproficiency", GameScript::ExtraProficiency,0},
	{"extraproficiencygt", GameScript::ExtraProficiencyGT,0},
	{"extraproficiencylt", GameScript::ExtraProficiencyLT,0},
	{"faction", GameScript::Faction,0},
	{"false", GameScript::False}, {"gender", GameScript::Gender},
	{"general", GameScript::General,0},
	{"global", GameScript::Global,TF_MERGESTRINGS},
	{"globalandglobal", GameScript::GlobalAndGlobal_Trigger,TF_MERGESTRINGS},
	{"globalband", GameScript::BitCheck,AF_MERGESTRINGS},
	{"globalbandglobal", GameScript::GlobalBAndGlobal_Trigger,AF_MERGESTRINGS},
	{"globalbitglobal", GameScript::GlobalBitGlobal_Trigger,TF_MERGESTRINGS},
	{"globalequalsglobal", GameScript::GlobalsEqual,TF_MERGESTRINGS}, //this is the same
	{"globalgt", GameScript::GlobalGT,TF_MERGESTRINGS},
	{"globalgtglobal", GameScript::GlobalGTGlobal,TF_MERGESTRINGS},
	{"globallt", GameScript::GlobalLT,TF_MERGESTRINGS},
	{"globalltglobal", GameScript::GlobalLTGlobal,TF_MERGESTRINGS},
	{"globalsequal", GameScript::GlobalsEqual,TF_MERGESTRINGS},
	{"globaltimerexact", GameScript::GlobalTimerExact,TF_MERGESTRINGS},
	{"globaltimerexpired", GameScript::GlobalTimerExpired,TF_MERGESTRINGS},
	{"globaltimernotexpired", GameScript::GlobalTimerNotExpired,TF_MERGESTRINGS},
	{"happiness", GameScript::Happiness,0},
	{"happinessgt", GameScript::HappinessGT,0},
	{"happinesslt", GameScript::HappinessLT,0},
	{"harmlessentered", GameScript::Entered,0}, //this isn't sure the same
	{"hasitem", GameScript::HasItem,0},
	{"hasitemslot", GameScript::HasItemSlot,0},
	{"hasiteminslot", GameScript::HasItemSlot,0},
	{"haveanyspells", GameScript::HaveAnySpells,0},
	{"havespell", GameScript::HaveSpell,0},    //these must be the same
	{"havespellres", GameScript::HaveSpell,0}, //they share the same ID
	{"hotkey", GameScript::HotKey,0},
	{"hp", GameScript::HP,0},
	{"hpgt", GameScript::HPGT,0}, {"hplt", GameScript::HPLT,0},
	{"hppercent", GameScript::HPPercent,0},
	{"hppercentgt", GameScript::HPPercentGT,0},
	{"hppercentlt", GameScript::HPPercentLT,0},
	{"inactivearea", GameScript::InActiveArea,0},
	{"incutscenemode", GameScript::InCutSceneMode,0},
	{"inmyarea", GameScript::InMyArea,0},
	{"inparty", GameScript::InParty,0},
	{"inpartyallowdead", GameScript::InPartyAllowDead,0},
	{"inpartyslot", GameScript::InPartySlot,0},
	{"internal", GameScript::Internal,0},
	{"internalgt", GameScript::InternalGT,0},
	{"internallt", GameScript::InternalLT,0},
	{"interactingwith", GameScript::InteractingWith,0},
	{"isaclown", GameScript::IsAClown,0},
	{"islocked", GameScript::IsLocked,0},
	{"isvalidforpartydialog", GameScript::IsValidForPartyDialog,0},
	{"itemisidentified", GameScript::ItemIsIdentified,0},
	{"level", GameScript::Level,0},
	{"levelgt", GameScript::LevelGT,0},
	{"levellt", GameScript::LevelLT,0},
	{"levelparty", GameScript::LevelParty,0},
	{"levelpartygt", GameScript::LevelPartyGT,0},
	{"levelpartylt", GameScript::LevelPartyLT,0},
	{"los", GameScript::LOS,0},
	{"morale", GameScript::Morale,0},
	{"moralegt", GameScript::MoraleGT,0},
	{"moralelt", GameScript::MoraleLT,0},
	{"nearlocation", GameScript::NearLocation,0},
	{"notstatecheck", GameScript::NotStateCheck,0},
	{"nulldialog", GameScript::NullDialog,0},
	{"numcreature", GameScript::NumCreatures,0},
	{"numcreatureGT", GameScript::NumCreaturesGT,0},
	{"numcreatureLT", GameScript::NumCreaturesLT,0},
	{"numinparty", GameScript::PartyCountEQ,0},
	{"numinpartyalive", GameScript::PartyCountAliveEQ,0},
	{"numinpartyalivegt", GameScript::PartyCountAliveGT,0},
	{"numinpartyalivelt", GameScript::PartyCountAliveLT,0},
	{"numinpartygt", GameScript::PartyCountGT,0},
	{"numinpartylt", GameScript::PartyCountLT,0},
	{"numitems", GameScript::NumItems,0},
	{"numitemsgt", GameScript::NumItemsGT,0},
	{"numitemslt", GameScript::NumItemsLT,0},
	{"numitemsparty", GameScript::NumItemsParty,0},
	{"numitemspartygt", GameScript::NumItemsPartyGT,0},
	{"numitemspartylt", GameScript::NumItemsPartyLT,0},
	{"numtimestalkedto", GameScript::NumTimesTalkedTo,0},
	{"numtimestalkedtogt", GameScript::NumTimesTalkedToGT,0},
	{"numtimestalkedtolt", GameScript::NumTimesTalkedToLT,0},
	{"objectactionlistempty", GameScript::ObjectActionListEmpty,0}, //same function
	{"oncreation", GameScript::OnCreation,0},
	{"openstate", GameScript::OpenState,0},
	{"or", GameScript::Or,0},
	{"ownsfloatermessage", GameScript::OwnsFloaterMessage,0},
	{"partycounteq", GameScript::PartyCountEQ,0},
	{"partycountgt", GameScript::PartyCountGT,0},
	{"partycountlt", GameScript::PartyCountLT,0},
	{"partygold", GameScript::PartyGold,0},
	{"partygoldGT", GameScript::PartyGoldGT,0},
	{"partygoldLT", GameScript::PartyGoldLT,0},
	{"partyhasitem", GameScript::PartyHasItem,0},
	{"partyhasitemidentified", GameScript::PartyHasItemIdentified,0},
	{"proficiency", GameScript::Proficiency,0},
	{"proficiencygt", GameScript::ProficiencyGT,0},
	{"proficiencylt", GameScript::ProficiencyLT,0},
	{"race", GameScript::Race,0},
	{"randomnum", GameScript::RandomNum,0},
	{"randomnumgt", GameScript::RandomNumGT,0},
	{"randomnumlt", GameScript::RandomNumLT,0},
	{"range", GameScript::Range,0},
	{"reputation", GameScript::Reputation,0},
	{"reputationgt", GameScript::ReputationGT,0},
	{"reputationlt", GameScript::ReputationLT,0},
	{"see", GameScript::See,0},
	{"specifics", GameScript::Specifics,0},
	{"statecheck", GameScript::StateCheck,0},
	{"targetunreachable", GameScript::TargetUnreachable,0},
	{"team", GameScript::Team,0},
	{"time", GameScript::Time,0},
	{"timegt", GameScript::TimeGT,0},
	{"timelt", GameScript::TimeLT,0},
	{"traptriggered", GameScript::TrapTriggered,0},
	{"true", GameScript::True,0},
	{"unselectablevariable", GameScript::UnselectableVariable,0},
	{"unselectablevariablegt", GameScript::UnselectableVariableGT,0},
	{"unselectablevariablelt", GameScript::UnselectableVariableLT,0},
	{"xor", GameScript::Xor,TF_MERGESTRINGS},
	{"xp", GameScript::XP,0},
	{"xpgt", GameScript::XPGT,0},
	{"xplt", GameScript::XPLT,0}, { NULL,NULL,0},
};

//Make this an ordered list, so we could use bsearch!
static ActionLink actionnames[] = {
	{"actionoverride",NULL,0}, {"activate", GameScript::Activate,0},
	{"addareaflag", GameScript::AddAreaFlag,0},
	{"addareatype", GameScript::AddAreaType,0},
	{"addexperienceparty", GameScript::AddExperienceParty,0},
	{"addexperiencepartyglobal", GameScript::AddExperiencePartyGlobal,AF_MERGESTRINGS},
	{"addglobals", GameScript::AddGlobals,0},
	{"addwaypoint", GameScript::AddWayPoint,AF_BLOCKING},
	{"addxp2da", GameScript::AddXP2DA,0},
	{"addxpobject", GameScript::AddXPObject,0},
	{"addxpvar", GameScript::AddXP2DA,0},
	{"ally", GameScript::Ally,0},
	{"ambientactivate", GameScript::AmbientActivate,0},
	{"bashdoor", GameScript::OpenDoor,AF_BLOCKING}, //the same until we know better
	{"bitclear", GameScript::BitClear,AF_MERGESTRINGS},
	{"bitglobal", GameScript::BitGlobal,AF_MERGESTRINGS},
	{"bitset", GameScript::GlobalBOr,AF_MERGESTRINGS}, //probably the same
	{"changeaiscript", GameScript::ChangeAIScript,0},
	{"changealignment", GameScript::ChangeAlignment,0},
	{"changeallegiance", GameScript::ChangeAllegiance,0},
	{"changeclass", GameScript::ChangeClass,0},
	{"changedialog", GameScript::ChangeDialogue,0},
	{"changedialogue", GameScript::ChangeDialogue,0},
	{"changegender", GameScript::ChangeGender,0},
	{"changegeneral", GameScript::ChangeGeneral,0},
	{"changeenemyally", GameScript::ChangeAllegiance,0}, //this is the same
	{"changerace", GameScript::ChangeRace,0},
	{"changespecifics", GameScript::ChangeSpecifics,0},
	{"changestat", GameScript::ChangeStat,0},
	{"clearactions", GameScript::ClearActions,0},
	{"clearallactions", GameScript::ClearAllActions,0},
	{"closedoor", GameScript::CloseDoor,AF_BLOCKING},
	{"continue", GameScript::Continue,AF_INSTANT | AF_CONTINUE},
	{"createcreature", GameScript::CreateCreature,0}, //point is relative to Sender
	{"createcreatureatfeet", GameScript::CreateCreatureAtFeet,0}, 
	{"createcreatureobject", GameScript::CreateCreatureObjectOffset,0}, //the same
	{"createcreatureobjectoffset", GameScript::CreateCreatureObjectOffset,0}, //the same
	{"createpartygold", GameScript::CreatePartyGold,0},
	{"createvisualeffect", GameScript::CreateVisualEffect,0},
	{"createvisualeffectobject", GameScript::CreateVisualEffectObject,0},
	{"cutsceneid", GameScript::CutSceneID,AF_INSTANT},
	{"deactivate", GameScript::Deactivate,0},
	{"debug", GameScript::Debug,0},
	{"destroyalldestructableequipment", GameScript::DestroyAllDestructableEquipment,0},
	{"destroyallequipment", GameScript::DestroyAllEquipment,0},
	{"destroyitem", GameScript::DestroyItem,0},
	{"destroypartygold", GameScript::DestroyPartyGold,0},
	{"destroyself", GameScript::DestroySelf,0},
	{"dialogue", GameScript::Dialogue,AF_BLOCKING},
	{"dialogueforceinterrupt", GameScript::DialogueForceInterrupt,AF_BLOCKING},
	{"displaystring", GameScript::DisplayString,0},
	{"displaystringhead", GameScript::DisplayStringHead,0},
	{"displaystringnonamehead", GameScript::DisplayStringNoNameHead,0},
	{"displaystringwait", GameScript::DisplayStringWait,AF_BLOCKING},
	{"endcutscenemode", GameScript::EndCutSceneMode,0},
	{"enemy", GameScript::Enemy,0},
	{"erasejournalentry", GameScript::RemoveJournalEntry,0},
	{"face", GameScript::Face,AF_BLOCKING},
	{"faceobject", GameScript::FaceObject, AF_BLOCKING},
	{"fadefromblack", GameScript::FadeFromColor,0}, //probably the same
	{"fadefromcolor", GameScript::FadeFromColor,0},
	{"fadetoblack", GameScript::FadeToColor,0}, //probably the same
	{"fadetocolor", GameScript::FadeToColor,0},
	{"floatmessage", GameScript::DisplayStringHead,0}, //probably the same
	{"forceaiscript", GameScript::ForceAIScript,0},
	{"forcefacing", GameScript::ForceFacing,0},
	{"forcespell", GameScript::ForceSpell,0},
	{"giveexperience", GameScript::AddXPObject,0},
	{"givegoldforce", GameScript::CreatePartyGold,0}, //this is the same
	{"givepartygold", GameScript::GivePartyGold,0},
	{"givepartygoldglobal", GameScript::GivePartyGoldGlobal,AF_MERGESTRINGS},
	{"globaladdglobal", GameScript::GlobalAddGlobal,AF_MERGESTRINGS},
	{"globalandglobal", GameScript::GlobalAndGlobal,AF_MERGESTRINGS},
	{"globalband", GameScript::GlobalBAnd,AF_MERGESTRINGS},
	{"globalbandglobal", GameScript::GlobalBAndGlobal,AF_MERGESTRINGS},
	{"globalbitglobal", GameScript::GlobalBitGlobal, AF_MERGESTRINGS},
	{"globalbor", GameScript::GlobalBOr,AF_MERGESTRINGS},
	{"globalborglobal", GameScript::GlobalBOrGlobal,AF_MERGESTRINGS},
	{"globalmax", GameScript::GlobalMax,AF_MERGESTRINGS},
	{"globalmaxglobal", GameScript::GlobalMaxGlobal,AF_MERGESTRINGS},
	{"globalmin", GameScript::GlobalMin,AF_MERGESTRINGS},
	{"globalminglobal", GameScript::GlobalMinGlobal,AF_MERGESTRINGS},
	{"globalorglobal", GameScript::GlobalOrGlobal,AF_MERGESTRINGS},
	{"globalset", GameScript::SetGlobal,AF_MERGESTRINGS},
	{"globalsetglobal", GameScript::GlobalSetGlobal,AF_MERGESTRINGS},
	{"globalshl", GameScript::GlobalShL,AF_MERGESTRINGS},
	{"globalshlglobal", GameScript::GlobalShLGlobal,AF_MERGESTRINGS},
	{"globalshr", GameScript::GlobalShR,AF_MERGESTRINGS},
	{"globalshrglobal", GameScript::GlobalShRGlobal,AF_MERGESTRINGS},
	{"globalsubglobal", GameScript::GlobalSubGlobal,AF_MERGESTRINGS},
	{"globalxor", GameScript::GlobalXor,AF_MERGESTRINGS},
	{"globalxorglobal", GameScript::GlobalXorGlobal,AF_MERGESTRINGS},
	{"hidegui", GameScript::HideGUI,0},
	{"incinternal", GameScript::IncInternal,0},
	{"incmoraleai", GameScript::IncMoraleAI,0},
	{"incrementchapter", GameScript::IncrementChapter,0},
	{"incrementglobal", GameScript::IncrementGlobal,AF_MERGESTRINGS},
	{"incrementglobalonce", GameScript::IncrementGlobalOnce,AF_MERGESTRINGS},
	{"incrementextraproficiency", GameScript::IncrementExtraProficiency,0},
	{"incrementproficiency", GameScript::IncrementProficiency,0},
	{"interact", GameScript::Interact,0},
	{"joinparty", GameScript::JoinParty,0},
	{"addjournalentry", GameScript::AddJournalEntry,0},
	{"jumptoobject", GameScript::JumpToObject,0},
	{"jumptopoint", GameScript::JumpToPoint,0},
	{"jumptopointinstant", GameScript::JumpToPointInstant,0},
	{"kill", GameScript::Kill,0},
	{"leavearealua", GameScript::LeaveAreaLUA,0},
	{"leavearealuaentry", GameScript::LeaveAreaLUAEntry,0},
	{"leavearealuapanic", GameScript::LeaveAreaLUAPanic,0},
	{"leavearealuapanicentry", GameScript::LeaveAreaLUAPanicEntry,0},
	{"leaveparty", GameScript::LeaveParty,0},
	{"lock", GameScript::Lock,AF_BLOCKING},//key not checked at this time!
	{"log", GameScript::Debug,0}, //the same until we know better
	{"makeglobal", GameScript::MakeGlobal,0},
	{"makeunselectable", GameScript::MakeUnselectable,0},
	{"moraledec", GameScript::MoraleDec,0},
	{"moraleinc", GameScript::MoraleInc,0},
	{"moraleset", GameScript::MoraleSet,0},
	{"movebetweenareas", GameScript::MoveBetweenAreas,0},
	{"movebetweenareaseffect", GameScript::MoveBetweenAreas,0},
	{"moveglobal", GameScript::MoveGlobal,0}, 
	{"moveglobalobject", GameScript::MoveGlobalObject,0}, 
	{"moveglobalobjectoffscreen", GameScript::MoveGlobalObjectOffScreen,0},
	{"movetoobject", GameScript::MoveToObject,AF_BLOCKING},
	{"movetooffset", GameScript::MoveToOffset,AF_BLOCKING},
	{"movetopoint", GameScript::MoveToPoint,AF_BLOCKING},
	{"movetopointnorecticle", GameScript::MoveToPoint,AF_BLOCKING},//the same until we know better
	{"moveviewobject", GameScript::MoveViewPoint,0},
	{"moveviewpoint", GameScript::MoveViewPoint,0},
	{"nidspecial1", GameScript::NIDSpecial1,AF_BLOCKING},
	{"noaction", GameScript::NoAction,0},
	{"opendoor", GameScript::OpenDoor,AF_BLOCKING},
	{"permanentstatchange", GameScript::ChangeStat,0}, //probably the same
	{"picklock", GameScript::OpenDoor,AF_BLOCKING}, //the same until we know better
	{"playdead", GameScript::PlayDead,0},
	{"playerdialog", GameScript::PlayerDialogue,AF_BLOCKING},
	{"playerdialogue", GameScript::PlayerDialogue,AF_BLOCKING},
	{"playsound", GameScript::PlaySound,0},
	{"recoil", GameScript::Recoil,0},
	{"removeareatype", GameScript::RemoveAreaType,0},
	{"removeareaflag", GameScript::RemoveAreaFlag,0},
	{"removejournalentry", GameScript::RemoveJournalEntry,0},
	{"runawayfrom", GameScript::RunAwayFrom,AF_BLOCKING},
	{"runawayfromnointerrupt", GameScript::RunAwayFromNoInterrupt,AF_BLOCKING},
	{"runawayfrompoint", GameScript::RunAwayFromPoint,AF_BLOCKING},
	{"runtoobject", GameScript::MoveToObject,AF_BLOCKING}, //until we know better
	{"runtopoint", GameScript::MoveToPoint,AF_BLOCKING}, //until we know better
	{"runtopointnorecticle", GameScript::MoveToPoint,AF_BLOCKING},//until we know better
	{"savelocation", GameScript::SaveLocation,0},
	{"saveobjectlocation", GameScript::SaveObjectLocation,0},
	{"screenshake", GameScript::ScreenShake,AF_BLOCKING},
	{"setanimstate", GameScript::SetAnimState,AF_BLOCKING},
	{"setarearestflag", GameScript::SetAreaRestFlag,0},
	{"setapparentnamestrref", GameScript::SetApparentName,0},
	{"setleavepartydialogfile", GameScript::SetLeavePartyDialogFile,0},
	{"setregularnamestrref", GameScript::SetRegularName,0},
	{"setbeeninpartyflags", GameScript::SetBeenInPartyFlags,0},
	{"setdialog", GameScript::SetDialogue,AF_BLOCKING},
	{"setdialogue", GameScript::SetDialogue,AF_BLOCKING},
	{"setdoorlocked", GameScript::Lock,AF_BLOCKING},//key shouldn't be checked!
	{"setfaction", GameScript::SetFaction,0},
	{"setglobal", GameScript::SetGlobal,AF_MERGESTRINGS},
	{"setglobaltimer", GameScript::SetGlobalTimer,AF_MERGESTRINGS},
	{"sethp", GameScript::SetHP,0},
	{"setinternal", GameScript::SetInternal,0},
	{"setmoraleai", GameScript::SetMoraleAI,0},
	{"setnumtimestalkedto", GameScript::SetNumTimesTalkedTo,0},
	{"setplayersound", GameScript::SetPlayerSound,0},
	{"setquestdone", GameScript::SetQuestDone,0},
	{"setteam", GameScript::SetTeam,0},
	{"settextcolor", GameScript::SetTextColor,0},
	{"settokenglobal", GameScript::SetTokenGlobal,AF_MERGESTRINGS},
	{"setvisualrange", GameScript::SetVisualRange,0},
	{"sg", GameScript::SG,0},
	{"smallwait", GameScript::SmallWait,AF_BLOCKING},
	{"startcutscene", GameScript::StartCutScene,0},
	{"startcutscenemode", GameScript::StartCutSceneMode,0},
	{"startdialog", GameScript::StartDialogue,AF_BLOCKING},
	{"startdialogue", GameScript::StartDialogue,AF_BLOCKING},
	{"startdialoginterrupt", GameScript::StartDialogueInterrupt,AF_BLOCKING},
	{"startdialogueinterrupt", GameScript::StartDialogueInterrupt,AF_BLOCKING},
	{"startdialognoname", GameScript::StartDialogue,AF_BLOCKING},
	{"startdialoguenoname", GameScript::StartDialogue,AF_BLOCKING},
	{"startdialognoset", GameScript::StartDialogueNoSet,AF_BLOCKING},
	{"startdialoguenoset", GameScript::StartDialogueNoSet,AF_BLOCKING},
	{"startdialoguenosetinterrupt", GameScript::StartDialogueNoSetInterrupt,AF_BLOCKING},
	{"startdialogoverride", GameScript::StartDialogueOverride,AF_BLOCKING},
	{"startdialogueoverride", GameScript::StartDialogueOverride,AF_BLOCKING},
	{"startdialogoverrideinterrupt", GameScript::StartDialogueOverrideInterrupt,AF_BLOCKING},
	{"startdialogueoverrideinterrupt", GameScript::StartDialogueOverrideInterrupt,AF_BLOCKING},
	{"startmovie", GameScript::StartMovie,AF_BLOCKING},
	{"startsong", GameScript::StartSong,0},
	{"swing", GameScript::Swing,0},
	{"swingonce", GameScript::SwingOnce,0},
	{"takepartygold", GameScript::TakePartyGold,0},
	{"textscreen", GameScript::TextScreen,0},
	{"triggeractivation", GameScript::TriggerActivation,0},
	{"unhidegui", GameScript::UnhideGUI,0},
	{"unloadarea", GameScript::UnloadArea,0},
	{"unlock", GameScript::Unlock,0},
	{"unmakeglobal", GameScript::UnMakeGlobal,0}, //this is a GemRB extension
	{"verbalconstant", GameScript::VerbalConstant,0},
	{"wait", GameScript::Wait, AF_BLOCKING},
	{"waitrandom", GameScript::WaitRandom, AF_BLOCKING}, { NULL,NULL,0},
};

//Make this an ordered list, so we could use bsearch!
static ObjectLink objectnames[] = {
	{"bestac", GameScript::BestAC},
	{"eighthnearest", GameScript::EighthNearest},
	{"eighthnearestenemyof", GameScript::EighthNearestEnemyOf},
	{"fifthnearest", GameScript::FifthNearest},
	{"fifthnearestenemyof", GameScript::FifthNearestEnemyOf},
	{"fourthnearest", GameScript::FourthNearest},
	{"fourthnearestenemyof", GameScript::FourthNearestEnemyOf},
	{"lastheardby", GameScript::LastHeardBy},
	{"lasthitter", GameScript::LastHitter},
	{"lastseenby", GameScript::LastSeenBy},
	{"lastsummonerof", GameScript::LastSummonerOf},
	{"lasttalkedtoby", GameScript::LastTalkedToBy},
	{"lasttrigger", GameScript::LastTrigger},
	{"myself", GameScript::Myself},
	{"nearest", GameScript::Nearest},
	{"nearestenemyof", GameScript::NearestEnemyOf},
	{"ninthnearest", GameScript::NinthNearest},
	{"ninthnearestenemyof", GameScript::NinthNearestEnemyOf},
	{"nothing", GameScript::Nothing},
	{"player1", GameScript::Player1},
	{"player1fill", GameScript::Player1Fill},
	{"player2", GameScript::Player2},
	{"player2fill", GameScript::Player2Fill},
	{"player3", GameScript::Player3},
	{"player3fill", GameScript::Player3Fill},
	{"player4", GameScript::Player4},
	{"player4fill", GameScript::Player4Fill},
	{"player5", GameScript::Player5},
	{"player5fill", GameScript::Player5Fill},
	{"player6", GameScript::Player6},
	{"player6fill", GameScript::Player6Fill},
	{"player7", GameScript::Player7},
	{"player7fill", GameScript::Player7Fill},
	{"player8", GameScript::Player8},
	{"player8fill", GameScript::Player8Fill},
	{"protagonist", GameScript::Protagonist},
	{"secondnearest", GameScript::SecondNearest},
	{"secondnearestenemyof", GameScript::SecondNearestEnemyOf},
	{"selectedcharacter", GameScript::SelectedCharacter},
	{"seventhnearest", GameScript::SeventhNearest},
	{"seventhnearestenemyof", GameScript::SeventhNearestEnemyOf},
	{"sixthnearest", GameScript::SixthNearest},
	{"sixthnearestenemyof", GameScript::SixthNearestEnemyOf},
	{"strongestof", GameScript::StrongestOf},
	{"tenthnearest", GameScript::TenthNearest},
	{"tenthnearestenemyof", GameScript::TenthNearestEnemyOf},
	{"thirdnearest", GameScript::ThirdNearest},
	{"thirdnearestenemyof", GameScript::ThirdNearestEnemyOf},
	{"weakestof", GameScript::WeakestOf},
	{"worstac", GameScript::WorstAC},
	{ NULL,NULL},
};

static IDSLink idsnames[] = {
	{"align", GameScript::ID_Alignment},
	{"alignmen", GameScript::ID_Alignment},
	{"alignmnt", GameScript::ID_Alignment},
	{"avclass", GameScript::ID_AVClass},
	{"class", GameScript::ID_Class},
	{"classmsk", GameScript::ID_ClassMask},
	{"ea", GameScript::ID_Allegiance},
	{"faction", GameScript::ID_Faction},
	{"gender", GameScript::ID_Gender},
	{"general", GameScript::ID_General},
	{"race", GameScript::ID_Race},
	{"specific", GameScript::ID_Specific},
	{"subrace", GameScript::ID_Subrace},
	{"team", GameScript::ID_Team},
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
			if(!triggernames[i].Name[len]) {
				return triggernames + i;
			}
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
			if(!actionnames[i].Name[len]) {
				return actionnames + i;
			}
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
			if(!objectnames[i].Name[len]) {
				return objectnames + i;
			}
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

static int Distance(int X, int Y, Scriptable *b)
{
	long x = ( X - b->XPos );
	long y = ( Y - b->YPos );
	return (int) sqrt( ( double ) ( x* x + y* y ) );
}

static int Distance(Scriptable *a, Scriptable *b)
{
	long x = ( a->XPos - b->XPos );
	long y = ( a->YPos - b->YPos );
	return (int) sqrt( ( double ) ( x* x + y* y ) );
}

static void GoNearAndRetry(Scriptable *Sender, Point *p)
{
	Sender->AddActionInFront( Sender->CurrentAction );
	char Tmp[256];
	sprintf( Tmp, "MoveToPoint([%d.%d])", p->x, p->y );
	Sender->AddActionInFront( GameScript::GenerateAction( Tmp, true ) );
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
		HasAdditionalRect = ( atoi( objNameTable->QueryField( 2 ) ) != 0 );
		ExtraParametersCount = atoi( objNameTable->QueryField( 3 ) );
		ObjectFieldsCount = ObjectIDSCount - ExtraParametersCount;

		/* Initializing the Script Engine */

		memset( triggers, 0, sizeof( triggers ) );
		memset( actions, 0, sizeof( actions ) );
		memset( objects, 0, sizeof( objects ) );

		for (i = 0; i < MAX_TRIGGERS; i++) {
			char* triggername = triggersTable->GetValue( i );
			//maybe we should watch for this bit?
			if (!triggername) {
				triggername = triggersTable->GetValue( i | 0x4000 );
			}
			TriggerLink* poi = FindTrigger( triggername );
			if (poi == NULL) {
				triggers[i] = NULL;
				triggerflags[i] = 0;
			}
			else {
				if(InDebug) {
					printf("Found trigger:%04x %s\n",i,triggername);
				}
				triggers[i] = poi->Function;
				triggerflags[i] = poi->Flags;
			}
		}

		for (i = 0; i < MAX_ACTIONS; i++) {
			ActionLink* poi = FindAction( actionsTable->GetValue( i ) );
			if (poi == NULL) {
				actions[i] = NULL;
				actionflags[i] = 0;
			} else {
				actions[i] = poi->Function;
				actionflags[i] = poi->Flags;
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
	char line[10];

	if (!stream) {
		return NULL;
	}
	stream->ReadLine( line, 10 );
	if (strncmp( line, "SC", 2 ) != 0) {
		printf( "[IEScript]: Not a Compiled Script file\n" );
		delete( stream );
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
	newScript->AllocateBlocks( ( unsigned int ) rBv.size() );
	for (unsigned int i = 0; i < newScript->responseBlocksCount; i++) {
		newScript->responseBlocks[i] = rBv.at( i );
	}
	delete( stream );
	return newScript;
}

/* MYAREA is currently used in actors,
   it could be improved to use from area scripts */
void GameScript::ReplaceMyArea(Scriptable* Sender, char* newVarName)
{
	switch(Sender->Type) {
		case ST_ACTOR:
		{
			Actor *act = (Actor *) Sender;
			memcpy(newVarName, act->Area, 6);
			break;
		}
		default:
			memcpy(newVarName, "GLOBAL", 6);
			break;
	}
}

void GameScript::SetVariable(Scriptable* Sender, const char* VarName,
	const char* Context, int value)
{
	if(InDebug) {
		printf( "Setting variable(\"%s%s\", %d)\n", Context,
			VarName, value );
	}
	if (strnicmp( Context, "LOCALS", 6 ) == 0) {
		Sender->locals->SetAt( VarName, value );
		return;
	}
	//this is not a temporary storage, SetAt relies on a malloc-ed
	//string, but heh, apparently it is a temporary storage
	char* newVarName = ( char* ) malloc( 40 );
	strncpy( newVarName, Context, 6 );
	strncat( newVarName, VarName, 40 );
	if (strnicmp( newVarName, "MYAREA", 6 ) == 0) {
		ReplaceMyArea( Sender, newVarName );
	}
	core->GetGame()->globals->SetAt( newVarName, ( unsigned long ) value );
	free( newVarName);
}

void GameScript::SetVariable(Scriptable* Sender, const char* VarName,
	int value)
{
	if(InDebug) {
		printf( "Setting variable(\"%s\", %d)\n", VarName, value );
	}
	if (strnicmp( VarName, "LOCALS", 6 ) == 0) {
		Sender->locals->SetAt( &VarName[6], value );
		return;
	}
	char* newVarName = strndup( VarName, 40 );
	if (strnicmp( newVarName, "MYAREA", 6 ) == 0) {
		ReplaceMyArea( Sender, newVarName );
	}
	core->GetGame()->globals->SetAt( newVarName, ( unsigned long ) value );
	free(newVarName); //freeing up the temporary area
}

unsigned long GameScript::CheckVariable(Scriptable* Sender,
	const char* VarName)
{
	char newVarName[40];
	unsigned long value = 0;

	if (strnicmp( VarName, "LOCALS", 6 ) == 0) {
		Sender->locals->Lookup( &VarName[6], value );
		if(InDebug) {
			printf("CheckVariable %s: %ld\n",VarName, value);
		}
		return value;
	}
	strncpy( newVarName, VarName, 40 );
	if (strnicmp( VarName, "MYAREA", 6 ) == 0) {
		ReplaceMyArea( Sender, newVarName );
	}
	core->GetGame()->globals->Lookup( newVarName, value );
	if(InDebug) {
		printf("CheckVariable %s: %ld\n",VarName, value);
	}
	return value;
}

unsigned long GameScript::CheckVariable(Scriptable* Sender,
	const char* VarName, const char* Context)
{
	char newVarName[40];
	unsigned long value = 0;

	if (strnicmp( Context, "LOCALS", 6 ) == 0) {
		Sender->locals->Lookup( VarName, value );
		if(InDebug) {
			printf("CheckVariable %s%s: %ld\n",Context, VarName, value);
		}
		return value;
	}
	strncpy( newVarName, Context, 6 );
	strncat( newVarName, VarName, 40 );
	if (strnicmp( VarName, "MYAREA", 6 ) == 0) {
		ReplaceMyArea( Sender, newVarName );
	}
	core->GetGame()->globals->Lookup( VarName, value );
	if(InDebug) {
		printf("CheckVariable %s%s: %ld\n",Context, VarName, value);
	}
	return value;
}

void GameScript::Update()
{
	if (!MySelf || !MySelf->Active) {
		return;
	}
	unsigned long thisTime;
	GetTime( thisTime ); //this should be gametime too, pause holds it
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
			continueExecution = ( ExecuteResponseSet( this->MySelf,
									rB->responseSet ) != 0 );
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
	GetTime( thisTime ); //this should be gametime too, pause holds it
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
	char line[10];

	stream->ReadLine( line, 10 );
	if (strncmp( line, "CR", 2 ) != 0) {
		return NULL;
	}
	ResponseBlock* rB = new ResponseBlock();
	rB->condition = ReadCondition( stream );
	rB->responseSet = ReadResponseSet( stream );
	return rB;
}

Condition* GameScript::ReadCondition(DataStream* stream)
{
	char line[10];

	stream->ReadLine( line, 10 );
	if (strncmp( line, "CO", 2 ) != 0) {
		return NULL;
	}
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
	char line[10];

	stream->ReadLine( line, 10 );
	if (strncmp( line, "RS", 2 ) != 0) {
		return NULL;
	}
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
	char *poi;
	rE->weight = strtoul(line,&poi,10);
	std::vector< Action*> aCv;
	if(strncmp(poi,"AC",2)==0) while (true) {
		Action* aC = new Action(false);
		count = stream->ReadLine( line, 1024 );
		aC->actionID = strtoul(line, NULL,10);
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
		sscanf( line, "%hd %d %d %d %d [%d,%d] \"%[^\"]\" \"%[^\"]\" OB",
			&tR->triggerID, &tR->int0Parameter, &tR->flags,
			&tR->int1Parameter, &tR->int2Parameter, &tR->XpointParameter,
			&tR->YpointParameter, tR->string0Parameter, tR->string1Parameter );
	} else {
		sscanf( line, "%hd %d %d %d %d \"%[^\"]\" \"%[^\"]\" OB",
			&tR->triggerID, &tR->int0Parameter, &tR->flags,
			&tR->int1Parameter, &tR->int2Parameter, tR->string0Parameter,
			tR->string1Parameter );
	}
	tR->triggerID &= 0x3fff;
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
	unsigned int result = 0;
	bool subresult = true;

	RandomNumValue=rand();
	for (int i = 0; i < condition->triggersCount; i++) {
		Trigger* tR = condition->triggers[i];
		//do not evaluate triggers in an Or() block if one of them
		//was already True()
		if (!ORcount || !subresult) {
			result = EvaluateTrigger( Sender, tR );
		}
		if (result > 1) {
			//we started an Or() block
			if (ORcount) {
				printf( "[IEScript]: Unfinished OR block encountered!\n" );
			}
			ORcount = result;
			subresult = false;
			continue;
		}
		if (ORcount) {
			subresult |= ( result != 0 );
			if (--ORcount) {
				continue;
			}
			result = subresult;
		}
		if (!result) {
			return 0;
		}
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
		return !ret;
	}
	return ( ret != 0 );
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
	int randWeight;
	int maxWeight = 0;

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
		switch (actionflags[aC->actionID] & AF_MASK) {
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
	if(InDebug) {
		printf("Sender: %s\n",Sender->scriptName);
	}
	ActionFunction func = actions[aC->actionID];
	if (func) {
		Scriptable* scr = GetActorFromObject( Sender, aC->objects[0]);
		if(scr && scr!=Sender) {
			//this is an Action Override
			scr->AddAction( Sender->CurrentAction );
			Sender->CurrentAction = NULL;
			//maybe we should always release here???
			if (!(actionflags[aC->actionID] & AF_INSTANT) ) {
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
	if (!( actionflags[aC->actionID] & AF_BLOCKING )) {
		Sender->CurrentAction = NULL;
	}
	if (actionflags[aC->actionID] & AF_INSTANT) {
		return;
	}
	aC->Release();
}

/* returns actors that match the [x.y.z] expression */
Targets* GameScript::EvaluateObject(Object* oC)
{
	Targets *tgts=NULL;

	if (oC->objectName[0]) {
		//We want the object by its name... (doors/triggers don't play here!)
		Actor* aC = core->GetGame()->GetCurrentMap( )->GetActor( oC->objectName );
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
			int i = core->GetGame()->GetCurrentMap()->GetActorCount();
			tgts = new Targets();
			while(i--) {
				Actor *ac=core->GetGame()->GetCurrentMap()->GetActor(i);
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
		Scriptable * aC = core->GetGame()->GetCurrentMap( )->tm->GetDoor( oC->objectName );
		if (aC) {
			return aC;
		}
		//No... it was not a door... maybe an InfoPoint?
		aC = core->GetGame()->GetCurrentMap( )->tm->GetInfoPoint( oC->objectName );
		if (aC) {
			return aC;
		}

		//No... it was not an infopoint... maybe a Container?
		aC = core->GetGame()->GetCurrentMap( )->tm->GetContainer( oC->objectName );
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
		object=tgts->GetTarget(0);
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

//this function returns a value, symbol could be a numeric string or
//a symbol from idsname
static int GetIdsValue(const char *&symbol, const char *idsname)
{
	int idsfile=core->LoadSymbol(idsname);
	SymbolMgr *valHook = core->GetSymbol(idsfile);
	if(!valHook) {
		//FIXME:missing ids file!!!
		if(InDebug) {
			char Tmp[256];

			sprintf(Tmp,"Missing IDS file %s for symbol %s!\n",idsname, symbol);
			printMessage("IEScript",Tmp,LIGHT_RED);
		}
		return -1;
	}
	char *newsymbol;
	int value=strtol(symbol, &newsymbol, 0);
	if(symbol!=newsymbol) {
		symbol=newsymbol;
		return value;
	}
	char symbolname[64];
	int x;
	for(x=0;isalnum(*symbol) && x<(int) sizeof(symbolname)-1;x++) {
		symbolname[x]=*symbol;
		symbol++;
	}
	symbolname[x]=0;
	return valHook->GetValue(symbolname);
}

static void ParseIdsTarget(const char *&src, Object *&object)
{
	for(int i=0;i<ObjectFieldsCount;i++) {
			object->objectFields[i]=GetIdsValue(src, ObjectIDSTableNames[i]);
		if(*src!='.') {
			break;
		}
	}
	src++; //skipping ]
}

static void ParseObject(const char *&str,const char *&src, Object *&object)
{
	SKIP_ARGUMENT();
	object = new Object();
	switch (*src) {
	case '"':
		//Scriptable Name
		src++;
		int i;
		for(i=0;i<(int) sizeof(object->objectName)-1 && *src!='"';i++)
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
			memmove(object->objectFilters+1, object->objectFilters, (int) sizeof(int) *(MaxObjectNesting-1) );
			object->objectFilters[0]=GetIdsValue(src,"object");
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

/* this function was lifted from GenerateAction, to make it clearer */
Action *GameScript::GenerateActionCore(const char *src, const char *str, int acIndex, bool autoFree)
{
	Action *newAction = new Action(autoFree);
	newAction->actionID = (unsigned short) actionsTable->GetValueIndex( acIndex );
	//this flag tells us to merge 2 consecutive strings together to get
	//a variable (context+variablename)
	int mergestrings = actionflags[newAction->actionID]&AF_MERGESTRINGS;
	int objectCount = ( newAction->actionID == 1 ) ? 0 : 1;
	int stringsCount = 0;
	int intCount = 0;
	//Here is the Action; Now we need to evaluate the parameters, if any
	if(*str!=')') while (*str) {
		if(*(str+1)!=':') {
			printf("Warning, parser was sidetracked: %s\n",str);
		}
		switch (*str) {
			default:
				printf("Invalid type: %s\n",str);
				str++;
				break;

			case 'p': //Point
				SKIP_ARGUMENT();
				src++; //Skip [
				newAction->XpointParameter = strtol( src, (char **) &src, 10 );
				src++; //Skip .
				newAction->YpointParameter = strtol( src, (char **) &src, 10 );
				src++; //Skip ]
				break;

			case 'i': //Integer
			{
				//going to the variable name
				while (*str != '*' && *str !=',' && *str != ')' ) {
					str++;
				}
				int value;
				if(*str=='*') { //there may be an IDS table
					str++;
					char idsTabName[33];
					char* tmp = idsTabName;
					while (( *str != ',' ) && ( *str != ')' )) {
						*tmp = *str;
						tmp++;
						str++;
					}
					*tmp = 0;
					if(idsTabName[0]) {
						value = GetIdsValue(src, idsTabName);
					}
					else {
						value = strtol( src, (char **) &src, 0);
					}
				}
				else { //no IDS table
					value = strtol( src, (char **) &src, 0);
				}
				if (!intCount) {
					newAction->int0Parameter = value;
				} else if (intCount == 1) {
					newAction->int1Parameter = value;
				} else {
					newAction->int2Parameter = value;
				}
			}
			break;

			case 'a':
			//Action
			 {
				SKIP_ARGUMENT();
				char action[257];
				int i = 0;
				int openParenthesisCount = 0;
				while (true) {
					if (*src == ')') {
						if (!openParenthesisCount)
							break;
						openParenthesisCount--;
					} else {
						if (*src == '(') {
							openParenthesisCount++;
						} else {
							if (( *src == ',' ) &&
								!openParenthesisCount)
								break;
						}
					}
					action[i] = *src;
					i++;
					src++;
				}
				action[i] = 0;
				Action* act = GenerateAction( action, autoFree );
				act->objects[0] = newAction->objects[0];
				//act->objects[0]->IncRef(); //avoid freeing of object
				// newAction->Release(); //freeing action
				//the previous hack doesn't work anymore
				//because we create actions with autofree
				newAction->objects[0] = NULL; //avoid freeing of object
				delete newAction; //freeing action
				newAction = act;
			}
			break;

			case 'o': //Object
				if(objectCount==3) {
					printf("Invalid object count!\n");
					abort();
				}
				ParseObject(str, src, newAction->objects[objectCount++]);
				break;

			case 's': //String
			{
				SKIP_ARGUMENT();
				src++;
				int i;
				char* dst;
				if (!stringsCount) {
					dst = newAction->string0Parameter;
				} else {
					dst = newAction->string1Parameter;
				}
				//skipping the context part, which
				//is to be readed later
				if(mergestrings) {
					for(i=0;i<6;i++) {
						*dst++='*';
					}
				}
				else {
					i=0;
				}
				while (*src != '"') {
					//sizeof(context+name) = 40
					if(i<40) {
						*dst++ = *src;
						i++;
					}
					src++;
				}
				*dst = 0;
				//reading the context part
				if(mergestrings) {
					str++;
					if(*str!='s') {
						printf("Invalid mergestrings:%s\n",str);
						abort();
					}
					SKIP_ARGUMENT();
					if (!stringsCount) {
						dst = newAction->string0Parameter;
					} else {
						dst = newAction->string1Parameter;
					}

					//this works only if there are no spaces
					if(*src++!='"' || *src++!=',' || *src++!='"') {
						break;
					}
					//reading the context string
					i=0;
					while (*src != '"') {
						if(i++<6) {
							*dst++ = *src;
						}
						src++;
					}
				}
				src++; //skipping "
				stringsCount++;
			}
			break;
		}
		str++;
		if(*src == ',' || *src==')')
			src++;
	}
	return newAction;
}

Action* GameScript::GenerateAction(char* String, bool autoFree)
{
	strlwr( String );
	//if(InDebug) {
		printf("Compiling:%s\n",String);
	//}
	int len = strlench(String,'(')+1; //including (
	int i = actionsTable->FindString(String, len);
	if (i<0) {
		return NULL;
	}
	char *src = String+len;
	char *str = actionsTable->GetStringIndex( i )+len;
	//if(InDebug) {
		printf("Match: %s vs. %s\n",src,str);
	//}
	return GenerateActionCore( src, str, i, autoFree);
}

/* this function was lifted from GenerateAction, to make it clearer */
Trigger *GameScript::GenerateTriggerCore(const char *src, const char *str, int trIndex, int negate)
{
	Trigger *newTrigger = new Trigger();
	newTrigger->triggerID = (unsigned short) triggersTable->GetValueIndex( trIndex )&0x3fff;
	newTrigger->flags = (unsigned short) negate;
	int mergestrings = triggerflags[newTrigger->triggerID]&AF_MERGESTRINGS;
	int stringsCount = 0;
	int intCount = 0;
	//Here is the Trigger; Now we need to evaluate the parameters
	if(*str!=')') while (*str) {
		if(*(str+1)!=':') {
			printf("Warning, parser was sidetracked: %s\n",str);
		}
		switch (*str) {
			default:
				printf("Invalid type: %s\n",str);
				str++;
				break;

			case 'p': //Point
				SKIP_ARGUMENT();
				src++; //Skip [
				newTrigger->XpointParameter = strtol( src, (char **) &src, 10 );
				src++; //Skip .
				newTrigger->YpointParameter = strtol( src, (char **) &src, 10 );
				src++; //Skip ]
				break;

			case 'i': //Integer
			{
				//going to the variable name
				while (*str != '*' && *str !=',' && *str != ')' ) {
					str++;
				}
				int value;
				if(*str=='*') { //there may be an IDS table
					str++;
					char idsTabName[33];
					char* tmp = idsTabName;
					while (( *str != ',' ) && ( *str != ')' )) {
						*tmp = *str;
						tmp++;
						str++;
					}
					*tmp = 0;
					if(idsTabName[0]) {
						value = GetIdsValue(src, idsTabName);
					}
					else {
						value = strtol( src, (char **) &src, 0);
					}
				}
				else { //no IDS table
					value = strtol( src, (char **) &src, 0);
				}
				if (!intCount) {
					newTrigger->int0Parameter = value;
				} else if (intCount == 1) {
					newTrigger->int1Parameter = value;
				} else {
					newTrigger->int2Parameter = value;
				}
				intCount++;
			}
			break;

			case 'o': //Object
				ParseObject(str, src, newTrigger->objectParameter);
				break;

			case 's': //String
			{
				SKIP_ARGUMENT();
				src++;
				int i;
				char* dst;
				if (!stringsCount) {
					dst = newTrigger->string0Parameter;
				} else {
					dst = newTrigger->string1Parameter;
				}
				//skipping the context part, which
				//is to be readed later
				if(mergestrings) {
					for(i=0;i<6;i++) {
						*dst++='*';
					}
				}
				else {
					i=0;
				}
				while (*src != '"') {
					//sizeof(context+name) = 40
					if(i<40) {
						*dst++ = *src;
						i++;
					}
					src++;
				}
				*dst = 0;
				//reading the context part
				if(mergestrings) {
					str++;
					if(*str!='s') {
						printf("Invalid mergestrings:%s\n",str);
						abort();
					}
					SKIP_ARGUMENT();
					if (!stringsCount) {
						dst = newTrigger->string0Parameter;
					} else {
						dst = newTrigger->string1Parameter;
					}

					//this works only if there are no spaces
					if(*src++!='"' || *src++!=',' || *src++!='"') {
						break;
					}
					//reading the context string
					i=0;
					while (*src != '"') {
						if(i++<6) {
							*dst++ = *src;
						}
						src++;
					}
				}
				src++; //skipping "
				stringsCount++;
			}
			break;
		}
		str++;
		if(*src == ',' || *src==')')
			src++;
	}
	return newTrigger;
}

Trigger* GameScript::GenerateTrigger(char* String)
{
	strlwr( String );
	if(InDebug) {
		printf("Compiling:%s\n",String);
	}
	int negate = 0;
	if (*String == '!') {
		String++;
		negate = 1;
	}
	int len = strlench(String,'(')+1; //including (
	int i = triggersTable->FindString(String, len);
	if( i<0 ) {
		return NULL;
	}
	char *src = String+len;
	char *str = triggersTable->GetStringIndex( i )+len;
	if(InDebug) {
		printf("Match: %s vs. %s\n",src,str);
	}
	return GenerateTriggerCore(src, str, i, negate);
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
	parameters->AddTarget(core->GetGame()->FindPC(1));
	return parameters;
}

//last talker
Targets *GameScript::Gabber(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	GameControl* gc = core->GetGameControl();
	if (gc) {
		parameters->AddTarget(gc->speaker);
	}
	return parameters;
}

Targets *GameScript::LastTrigger(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	Scriptable *lt = Sender->LastTrigger;
	if (lt && lt->Type == ST_ACTOR) {
 		parameters->AddTarget((Actor *) lt);
	}
	return parameters;
}

Targets *GameScript::LastSeenBy(Scriptable *Sender, Targets *parameters)
{
	Actor *actor = parameters->GetTarget(0);
	if(!actor) {
		if(Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if(actor) {
		parameters->AddTarget(actor->LastSeen);
	}
	return parameters;
}

Targets *GameScript::LastHeardBy(Scriptable *Sender, Targets *parameters)
{
	Actor *actor = parameters->GetTarget(0);
	if(!actor) {
		if(Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if(actor) {
		parameters->AddTarget(actor->LastHeard);
	}
	return parameters;
}

Targets *GameScript::LastHitter(Scriptable *Sender, Targets *parameters)
{
	Actor *actor = parameters->GetTarget(0);
	if(!actor) {
		if(Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if(actor) {
		parameters->AddTarget(actor->LastHitter);
	}
	return parameters;
}

Targets *GameScript::LastTalkedToBy(Scriptable *Sender, Targets *parameters)
{
	Actor *actor = parameters->GetTarget(0);
	if(!actor) {
		if(Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if(actor) {
		parameters->AddTarget(actor->LastTalkedTo);
	}
	return parameters;
}

Targets *GameScript::LastSummonerOf(Scriptable *Sender, Targets *parameters)
{
	Actor *actor = parameters->GetTarget(0);
	if(!actor) {
		if(Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if(actor) {
		parameters->AddTarget(actor->LastSummoner);
	}
	return parameters;
}

Targets *GameScript::Player1(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(1));
	return parameters;
}

Targets *GameScript::Player1Fill(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(0));
	return parameters;
}

Targets *GameScript::Player2(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(2));
	return parameters;
}

Targets *GameScript::Player2Fill(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(1));
	return parameters;
}

Targets *GameScript::Player3(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(3));
	return parameters;
}

Targets *GameScript::Player3Fill(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(2));
	return parameters;
}

Targets *GameScript::Player4(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(4));
	return parameters;
}

Targets *GameScript::Player4Fill(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(3));
	return parameters;
}

Targets *GameScript::Player5(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(5));
	return parameters;
}

Targets *GameScript::Player5Fill(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(5));
	return parameters;
}

Targets *GameScript::Player6(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(6));
	return parameters;
}

Targets *GameScript::Player6Fill(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(6));
	return parameters;
}

Targets *GameScript::Player7(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(7));
	return parameters;
}

Targets *GameScript::Player7Fill(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(6));
	return parameters;
}

Targets *GameScript::Player8(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(8));
	return parameters;
}

Targets *GameScript::Player8Fill(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(7));
	return parameters;
}

Targets *GameScript::BestAC(Scriptable *Sender, Targets *parameters)
{
	int i=parameters->Count();
	if(!i) {
		return parameters;
	}
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
	if(!i) {
		return parameters;
	}
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
	if(!i) {
		return parameters;
	}
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
	if(!i) {
		return parameters;
	}
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

Targets *GameScript::MostDamagedOf(Scriptable *Sender, Targets *parameters)
{
	int i=parameters->Count();
	if(!i) {
		return parameters;
	}
	Actor *actor=parameters->GetTarget(--i);
	int worsthp=actor->GetStat(IE_MAXHITPOINTS)-actor->GetStat(IE_HITPOINTS);
	int pos=i;
	while(i--) {
		actor = parameters->GetTarget(pos);
		int ac=actor->GetStat(IE_MAXHITPOINTS)-actor->GetStat(IE_HITPOINTS);
		if(worsthp>ac) {
			worsthp=ac;
			pos=i;
		}
	}
	actor=parameters->GetTarget(pos);
	parameters->Clear();
	parameters->AddTarget(actor);
	return parameters;
}
Targets *GameScript::LeastDamagedOf(Scriptable *Sender, Targets *parameters)
{
	int i=parameters->Count();
	if(!i) {
		return parameters;
	}
	Actor *actor=parameters->GetTarget(--i);
	int worsthp=actor->GetStat(IE_MAXHITPOINTS)-actor->GetStat(IE_HITPOINTS);
	int pos=i;
	while(i--) {
		actor = parameters->GetTarget(pos);
		int ac=actor->GetStat(IE_MAXHITPOINTS)-actor->GetStat(IE_HITPOINTS);
		if(worsthp<ac) {
			worsthp=ac;
			pos=i;
		}
	}
	actor=parameters->GetTarget(pos);
	parameters->Clear();
	parameters->AddTarget(actor);
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
	int i = core->GetGame()->GetCurrentMap()->GetActorCount();
	Targets *tgts = new Targets();
	Actor *ac;
	while(i--) {
		ac=core->GetGame()->GetCurrentMap()->GetActor(i);
/*
		long x = ( ac->XPos - origin->XPos );
		long y = ( ac->YPos - origin->YPos );
		double distance = sqrt( ( double ) ( x* x + y* y ) );
*/
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
//	unsigned int distance=0;
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

Targets *GameScript::SelectedCharacter(Scriptable *Sender, Targets *parameters)
{
	Map *cm = core->GetGame()->GetCurrentMap();
	parameters->Clear();
	int i = cm->GetActorCount();
	while(i--) {
		Actor *ac=cm->GetActor(i);
		if(ac->IsSelected()) {
			parameters->AddTarget(ac);
		}
	}
	return parameters;
}

Targets *GameScript::Nothing(Scriptable* Sender, Targets* parameters)
{
	parameters->Clear();
	return parameters;
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
	int value=GetHappiness(Sender, core->GetGame()->Reputation );
	return value == parameters->int0Parameter;
}

int GameScript::HappinessGT(Scriptable* Sender, Trigger* parameters)
{
	int value=GetHappiness(Sender, core->GetGame()->Reputation );
	return value > parameters->int0Parameter;
}

int GameScript::HappinessLT(Scriptable* Sender, Trigger* parameters)
{
	int value=GetHappiness(Sender, core->GetGame()->Reputation );
	return value < parameters->int0Parameter;
}

int GameScript::Reputation(Scriptable* Sender, Trigger* parameters)
{
	return core->GetGame()->Reputation == (unsigned int) parameters->int0Parameter;
}

int GameScript::ReputationGT(Scriptable* Sender, Trigger* parameters)
{
	return core->GetGame()->Reputation > (unsigned int) parameters->int0Parameter;
}

int GameScript::ReputationLT(Scriptable* Sender, Trigger* parameters)
{
	return core->GetGame()->Reputation < (unsigned int) parameters->int0Parameter;
}

int GameScript::Alignment(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	return ID_Alignment( actor, parameters->int0Parameter);
}

int GameScript::Allegiance(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) scr;
	return ID_Allegiance( actor, parameters->int0Parameter);
}

int GameScript::Class(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = (Actor*)scr;
	return ID_Class( actor, parameters->int0Parameter);
}

int GameScript::Faction(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = (Actor*)scr;
	return ID_Faction( actor, parameters->int0Parameter);
}

int GameScript::Team(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = (Actor*)scr;
	return ID_Team( actor, parameters->int0Parameter);
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
	Map *map = core->GetGame()->GetCurrentMap();
	if (!map->IsVisible( Sender->XPos, Sender->YPos,
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
	//don't allow dead
	return (tar->GetStat(IE_STATE_ID)&STATE_DEAD)!=STATE_DEAD;
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

int GameScript::IsAClown(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type!=ST_ACTOR) {
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

int GameScript::Specifics(Scriptable* Sender, Trigger* parameters)
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
	return ( value& parameters->int0Parameter ) !=0;
}

int GameScript::BitCheckExact(Scriptable* Sender, Trigger* parameters)
{
	unsigned long value = CheckVariable(Sender, parameters->string0Parameter );
	return (value & parameters->int0Parameter ) ==(unsigned long) parameters->int0Parameter;
}

//BM_OR would make sense only if this trigger changes the value of the variable
//should I do that???
int GameScript::BitGlobal_Trigger(Scriptable* Sender, Trigger* parameters)
{
	unsigned long value = CheckVariable(Sender, parameters->string0Parameter );
	switch(parameters->int1Parameter) {
		case BM_AND:
			return ( value& parameters->int0Parameter ) !=0;
		case BM_OR:
			return ( value| parameters->int0Parameter ) !=0;
		case BM_XOR:
			return ( value^ parameters->int0Parameter ) !=0;
		case BM_NAND:
			return ( value& ~parameters->int0Parameter ) !=0;
	}
	return 0;
}

int GameScript::GlobalBAndGlobal_Trigger(Scriptable* Sender, Trigger* parameters)
{
	unsigned long value1 = CheckVariable(Sender, parameters->string0Parameter );
	unsigned long value2 = CheckVariable(Sender, parameters->string1Parameter );
	return ( value1& value2 ) !=0;
}

int GameScript::GlobalBitGlobal_Trigger(Scriptable* Sender, Trigger* parameters)
{
	unsigned long value1 = CheckVariable(Sender, parameters->string0Parameter );
	unsigned long value2 = CheckVariable(Sender, parameters->string1Parameter );
	switch(parameters->int0Parameter) {
		case BM_AND:
			return ( value1& value2 ) !=0;
		case BM_OR:
			return ( value1| value2 ) !=0;
		case BM_XOR:
			return ( value1^ value2 ) !=0;
		case BM_NAND:
			return ( value1& ~value2 ) !=0;
	}
	return 0;
}

//would this function also alter the variable?
int GameScript::Xor(Scriptable* Sender, Trigger* parameters)
{
	unsigned long value = CheckVariable(Sender, parameters->string0Parameter );
	return ( value ^ parameters->int0Parameter ) != 0;
}

int GameScript::Global(Scriptable* Sender, Trigger* parameters)
{
	long value = CheckVariable(Sender, parameters->string0Parameter );
	return ( value == parameters->int0Parameter );
}

int GameScript::GlobalLT(Scriptable* Sender, Trigger* parameters)
{
	long value = CheckVariable(Sender, parameters->string0Parameter );
	return ( value < parameters->int0Parameter );
}

int GameScript::GlobalLTGlobal(Scriptable* Sender, Trigger* parameters)
{
	long value1 = CheckVariable(Sender, parameters->string0Parameter );
	long value2 = CheckVariable(Sender, parameters->string1Parameter );
	return ( value1 < value2 );
}

int GameScript::GlobalGT(Scriptable* Sender, Trigger* parameters)
{
	long value = CheckVariable(Sender, parameters->string0Parameter );
	return ( value > parameters->int0Parameter );
}

int GameScript::GlobalGTGlobal(Scriptable* Sender, Trigger* parameters)
{
	long value1 = CheckVariable(Sender, parameters->string0Parameter );
	long value2 = CheckVariable(Sender, parameters->string1Parameter );
	return ( value1 > value2 );
}

int GameScript::GlobalsEqual(Scriptable* Sender, Trigger* parameters)
{
	long value1 = CheckVariable(Sender, parameters->string0Parameter );
	long value2 = CheckVariable(Sender, parameters->string1Parameter );
	return ( value1 == value2 );
}

int GameScript::GlobalTimerExact(Scriptable* Sender, Trigger* parameters)
{
	unsigned long value1 = CheckVariable(Sender, parameters->string0Parameter );
	return ( value1 == core->GetGame()->GameTime );
}

int GameScript::GlobalTimerExpired(Scriptable* Sender, Trigger* parameters)
{
	unsigned long value1 = CheckVariable(Sender, parameters->string0Parameter );
	return ( value1 < core->GetGame()->GameTime );
}

int GameScript::GlobalTimerNotExpired(Scriptable* Sender, Trigger* parameters)
{
	unsigned long value1 = CheckVariable(Sender, parameters->string0Parameter );
	return ( value1 > core->GetGame()->GameTime );
}

int GameScript::OnCreation(Scriptable* Sender, Trigger* parameters)
{
	return Sender->OnCreation; //hopefully this is always 1 or 0
/* oncreation is about the script, not the owner area, oncreation is
   working in ANY script */
/*
	Map* area = core->GetGame()->GetCurrentMap( );
	if (area->justCreated) {
		area->justCreated = false;
		return 1;
	}
	return 0;
*/
}

int GameScript::NumItemsParty(Scriptable* Sender, Trigger* parameters)
{
	int cnt = 0;
	Actor *actor;
	Game *game=core->GetGame();

	//there is an assignment here
	for(int i=0; (actor = game->GetPC(i)) ; i++) {
		cnt+=actor->inventory.CountItems(parameters->string0Parameter,1);
	}
	return cnt==parameters->int0Parameter;
}

int GameScript::NumItemsPartyGT(Scriptable* Sender, Trigger* parameters)
{
	int cnt = 0;
	Actor *actor;
	Game *game=core->GetGame();

	//there is an assignment here
	for(int i=0; (actor = game->GetPC(i)) ; i++) {
		cnt+=actor->inventory.CountItems(parameters->string0Parameter,1);
	}
	return cnt>parameters->int0Parameter;
}

int GameScript::NumItemsPartyLT(Scriptable* Sender, Trigger* parameters)
{
	int cnt = 0;
	Actor *actor;
	Game *game=core->GetGame();

	//there is an assignment here
	for(int i=0; (actor = game->GetPC(i)) ; i++) {
		cnt+=actor->inventory.CountItems(parameters->string0Parameter,1);
	}
	return cnt<parameters->int0Parameter;
}

int GameScript::NumItems(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if( !tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	int cnt = actor->inventory.CountItems(parameters->string0Parameter,1);
	return cnt==parameters->int0Parameter;
}

int GameScript::NumItemsGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if( !tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	int cnt = actor->inventory.CountItems(parameters->string0Parameter,1);
	return cnt>parameters->int0Parameter;
}

int GameScript::NumItemsLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if( !tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	int cnt = actor->inventory.CountItems(parameters->string0Parameter,1);
	return cnt<parameters->int0Parameter;
}

//the int0 parameter is an addition, normally it is 0
int GameScript::Contains(Scriptable* Sender, Trigger* parameters)
{
//actually this should be a container
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if( !tar || tar->Type!=ST_CONTAINER) {
		return 0;
	}
	Container *cnt = (Container *) tar;
	if (cnt->inventory.HasItem(parameters->string0Parameter, parameters->int0Parameter) ) {
		return 1;
	}
	return 0;
}

//the int0 parameter is an addition, normally it is 0
int GameScript::HasItem(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if( !scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) scr;
	if (actor->inventory.HasItem(parameters->string0Parameter, parameters->int0Parameter) ) {
		return 1;
	}
	return 0;
}

int GameScript::ItemIsIdentified(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if( !scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) scr;
	if (actor->inventory.HasItem(parameters->string0Parameter, IE_ITEM_IDENTIFIED) ) {
		return 1;
	}
	return 0;
}

/** if the string is zero, then it will return true if there is any item in the slot */
/** if the string is non-zero, it will return true, if the given item was in the slot */
int GameScript::HasItemSlot(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if( !scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) scr;
	//this might require a conversion of the slots
	if (actor->inventory.HasItemInSlot(parameters->string0Parameter, parameters->int0Parameter) ) {
		return !parameters->string0Parameter;
	}
	return !!parameters->string0Parameter;
}

int GameScript::HasItemEquipped(Scriptable * Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	Actor *actor = (Actor *) scr;
	if (actor->inventory.HasItem(parameters->string0Parameter, IE_ITEM_EQUIPPED) ) {
		return 1;
	}
	return 0;
}

int GameScript::Acquired(Scriptable * Sender, Trigger* parameters)
{
	if( Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) Sender;
	if (actor->inventory.HasItem(parameters->string0Parameter, IE_ITEM_ACQUIRED) ) {
		return 1;
	}
	return 0;
}

/** this trigger accepts a numeric parameter, this number could be: */
/** 0 - normal, 1 - equipped, 2 - identified, 3 - equipped&identified */
/** this is a GemRB extension */
int GameScript::PartyHasItem(Scriptable * /*Sender*/, Trigger* parameters)
{
	/*hacked to never have the item, this requires inventory!*/
	if (stricmp( parameters->string0Parameter, "MISC4G" ) == 0) {
		return 1;
	}
	/** */
	Actor *actor;
	Game *game=core->GetGame();

	//there is an assignment here
	for(int i=0; (actor = game->GetPC(i)) ; i++) {
		if (actor->inventory.HasItem(parameters->string0Parameter,parameters->int0Parameter) ) {
			return 1;
		}
	}
	return 0;
}

int GameScript::PartyHasItemIdentified(Scriptable * /*Sender*/, Trigger* parameters)
{
	Actor *actor;
	Game *game=core->GetGame();

	//there is an assignment here
	for(int i=0; (actor = game->GetPC(i)) ; i++) {
		if (actor->inventory.HasItem(parameters->string0Parameter, IE_ITEM_IDENTIFIED) ) {
			return 1;
		}
	}
	return 0;
}
//                               0      1      2      3    4   
static char spellnames[5][5]={"ITEM","SPPR","SPWI","SPIN","SPCL"};

#define CreateSpellName(spellname, data) sprintf(spellname,"%s%03d",spellnames[data/1000],data%1000)

int GameScript::HaveSpell(Scriptable *Sender, Trigger *parameters)
{
	if(Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) Sender;
	if(parameters->string0Parameter[0]) {
		return actor->spellbook.HaveSpell(parameters->string0Parameter, 1);
	}
	char tmpname[9];
	CreateSpellName(tmpname, parameters->int0Parameter);
	return actor->spellbook.HaveSpell(tmpname, 1);
}

int GameScript::HaveAnySpells(Scriptable *Sender, Trigger *parameters)
{
	if(Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) Sender;
	return actor->spellbook.HaveSpell("", 1);
}

int GameScript::HaveSpellParty(Scriptable *Sender, Trigger *parameters)
{
	Actor *actor;
	Game *game=core->GetGame();

	char tmpname[9];
	char *poi;

	if(parameters->string0Parameter[0]) {
		poi=parameters->string0Parameter;
	}
	else {
		CreateSpellName(tmpname, parameters->int0Parameter);
		poi=tmpname;
	}
	//there is an assignment here
	for(int i=0; (actor = game->GetPC(i)) ; i++) {
		if(actor->spellbook.HaveSpell(poi, 1) ) {
			return 1;
		}
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
	int distance = Distance(Sender, scr);
	if (distance <= ( parameters->int0Parameter * 20 )) {
		return 1;
	}
	return 0;
}

int GameScript::NearLocation(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	int distance = Distance(parameters->XpointParameter, parameters->YpointParameter, scr);
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
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (actor->GetStat( parameters->int0Parameter ) > parameters->int1Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::CheckStatLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (actor->GetStat( parameters->int0Parameter ) < parameters->int1Parameter) {
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
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	/* don't set LastSeen if this isn't an actor */
	if (!tar || tar->Type !=ST_ACTOR) {
		return 0;
	}
	long x = ( tar->XPos - Sender->XPos );
	long y = ( tar->YPos - Sender->YPos );
	double distance = sqrt( ( double ) ( x* x + y* y ) );
	if (distance > ( snd->Modified[IE_VISUALRANGE] * 20 )) {
		return 0;
	}
	Map *map = core->GetGame()->GetCurrentMap();
	if (map->IsVisible( Sender->XPos, Sender->YPos, tar->XPos, tar->YPos )) {
		if(justlos) {
			return 1;
		}
		//additional checks for invisibility?
		snd->LastSeen = (Actor *) tar;
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
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetStat(IE_MORALEBREAK) == parameters->int0Parameter;
}

int GameScript::MoraleGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetStat(IE_MORALEBREAK) > parameters->int0Parameter;
}

int GameScript::MoraleLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetStat(IE_MORALEBREAK) < parameters->int0Parameter;
}

int GameScript::StateCheck(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetStat(IE_STATE_ID) & parameters->int0Parameter;
}

int GameScript::NotStateCheck(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
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
			return (int) (door->Flags&1) == parameters->int0Parameter;
		}
		case ST_CONTAINER:
		{
			Container *cont = (Container *) tar;
			return cont->Locked == parameters->int0Parameter;
		}
		default:; //to remove a warning
	}
	printf("[IEScript]: couldn't find door/container:%s\n",parameters->string0Parameter);
	return 0;
}

int GameScript::IsLocked(Scriptable * Sender, Trigger *parameters)
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
			return !!(door->Flags&2);
		}
		case ST_CONTAINER:
		{
			Container *cont = (Container *) tar;
			return !!cont->Locked;
		}
		default:; //to remove a warning
	}
	printf("[IEScript]: couldn't find door/container:%s\n",parameters->string0Parameter);
	return 0;
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

//this is just a hack, actually multiclass should be available
int GameScript::ClassLevel(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if(actor->GetStat(IE_CLASS) != parameters->int0Parameter) {
		return 0;
	}
	return actor->GetStat(IE_LEVEL) == parameters->int1Parameter;
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

//this is just a hack, actually multiclass should be available
int GameScript::ClassLevelGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if(actor->GetStat(IE_CLASS) != parameters->int0Parameter) {
		return 0;
	}
	return actor->GetStat(IE_LEVEL) > parameters->int1Parameter;
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

int GameScript::ClassLevelLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if(actor->GetStat(IE_CLASS) != parameters->int0Parameter) {
		return 0;
	}
	return actor->GetStat(IE_LEVEL) < parameters->int1Parameter;
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
	if (!tar || tar->Type != ST_ACTOR) {
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

int GameScript::AreaCheckObject(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
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
	if (!tar || tar->Type != ST_ACTOR) {
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

int GameScript::TargetUnreachable(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 1; //well, if it doesn't exist it is unreachable
	}
	Map* map=core->GetGame()->GetCurrentMap();
	return map->TargetUnreachable( Sender->XPos, Sender->YPos, tar->XPos, tar->YPos);
}

int GameScript::PartyCountEQ(Scriptable* Sender, Trigger* parameters)
{
	return core->GetGame()->GetPartySize(0)<parameters->int0Parameter;
}

int GameScript::PartyCountLT(Scriptable* Sender, Trigger* parameters)
{
	return core->GetGame()->GetPartySize(0)<parameters->int0Parameter;
}

int GameScript::PartyCountGT(Scriptable* Sender, Trigger* parameters)
{
	return core->GetGame()->GetPartySize(0)>parameters->int0Parameter;
}

int GameScript::PartyCountAliveEQ(Scriptable* Sender, Trigger* parameters)
{
	return core->GetGame()->GetPartySize(1)<parameters->int0Parameter;
}

int GameScript::PartyCountAliveLT(Scriptable* Sender, Trigger* parameters)
{
	return core->GetGame()->GetPartySize(1)<parameters->int0Parameter;
}

int GameScript::PartyCountAliveGT(Scriptable* Sender, Trigger* parameters)
{
	return core->GetGame()->GetPartySize(1)>parameters->int0Parameter;
}

int GameScript::LevelParty(Scriptable* Sender, Trigger* parameters)
{
	return core->GetGame()->GetPartyLevel(1)<parameters->int0Parameter;
}

int GameScript::LevelPartyLT(Scriptable* Sender, Trigger* parameters)
{
	return core->GetGame()->GetPartyLevel(1)<parameters->int0Parameter;
}

int GameScript::LevelPartyGT(Scriptable* Sender, Trigger* parameters)
{
	return core->GetGame()->GetPartyLevel(1)>parameters->int0Parameter;
}

int GameScript::PartyGold(Scriptable* Sender, Trigger* parameters)
{
	return core->GetGame()->PartyGold==(unsigned long) parameters->int0Parameter;
}

int GameScript::PartyGoldGT(Scriptable* Sender, Trigger* parameters)
{
	return core->GetGame()->PartyGold>(unsigned long) parameters->int0Parameter;
}

int GameScript::PartyGoldLT(Scriptable* Sender, Trigger* parameters)
{
	return core->GetGame()->PartyGold<(unsigned long) parameters->int0Parameter;
}

int GameScript::OwnsFloaterMessage(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if(!tar) {
		return 0;
	}
	return tar->textDisplaying;
}

int GameScript::GlobalAndGlobal_Trigger(Scriptable* Sender, Trigger* parameters)
{
	unsigned long value1 = CheckVariable( Sender,
							parameters->string0Parameter );
	unsigned long value2 = CheckVariable( Sender,
							parameters->string1Parameter );
	return (value1 && value2)!=0; //should be 1 or 0!
}

int GameScript::InCutSceneMode(Scriptable* /*Sender*/, Trigger* /*parameters*/)
{
	return core->InCutSceneMode();
}

int GameScript::Proficiency(Scriptable* Sender, Trigger* parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>31) {
		return 0;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetStat(IE_PROFICIENCYBASTARDSWORD+idx) == parameters->int1Parameter;
}

int GameScript::ProficiencyGT(Scriptable* Sender, Trigger* parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>31) {
		return 0;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetStat(IE_PROFICIENCYBASTARDSWORD+idx) > parameters->int1Parameter;
}

int GameScript::ProficiencyLT(Scriptable* Sender, Trigger* parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>31) {
		return 0;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetStat(IE_PROFICIENCYBASTARDSWORD+idx) < parameters->int1Parameter;
}

int GameScript::ExtraProficiency(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetStat(IE_EXTRAPROFICIENCY1) == parameters->int0Parameter;
}

int GameScript::ExtraProficiencyGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetStat(IE_EXTRAPROFICIENCY1) > parameters->int0Parameter;
}

int GameScript::ExtraProficiencyLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetStat(IE_EXTRAPROFICIENCY1) < parameters->int0Parameter;
}

int GameScript::Internal(Scriptable* Sender, Trigger* parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>15) {
		return 0;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetStat(IE_INTERNAL_0+idx) == parameters->int1Parameter;
}

int GameScript::InternalGT(Scriptable* Sender, Trigger* parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>15) {
		return 0;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetStat(IE_INTERNAL_0+idx) > parameters->int1Parameter;
}

int GameScript::InternalLT(Scriptable* Sender, Trigger* parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>15) {
		return 0;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetStat(IE_INTERNAL_0+idx) < parameters->int1Parameter;
}

int GameScript::NullDialog(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	char *poi=actor->GetDialog();
	if(!poi[0]) {
		return 1;
	}
	return 0;
}

int GameScript::CalledByName(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	if(stricmp(actor->GetScriptName(), parameters->string0Parameter) ) {
		return 0;
	}
	return 1;
}

int GameScript::AnimState(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	return actor->AnimID == parameters->int0Parameter;
}

int GameScript::Time(Scriptable* Sender, Trigger* parameters)
{
	return core->GetGame()->GameTime==(unsigned long) parameters->int0Parameter;
}

int GameScript::TimeGT(Scriptable* Sender, Trigger* parameters)
{
	return core->GetGame()->GameTime>(unsigned long) parameters->int0Parameter;
}
int GameScript::TimeLT(Scriptable* Sender, Trigger* parameters)
{
	return core->GetGame()->GameTime<(unsigned long) parameters->int0Parameter;
}

int GameScript::HotKey(Scriptable* Sender, Trigger* parameters)
{
	return core->GetGameControl()->HotKey==parameters->int0Parameter;
}

int GameScript::CombatCounter(Scriptable* Sender, Trigger* parameters)
{
	return core->GetGame()->CombatCounter==(unsigned long) parameters->int0Parameter;
}

int GameScript::CombatCounterGT(Scriptable* Sender, Trigger* parameters)
{
	return core->GetGame()->CombatCounter>(unsigned long) parameters->int0Parameter;
}

int GameScript::CombatCounterLT(Scriptable* Sender, Trigger* parameters)
{
	return core->GetGame()->CombatCounter<(unsigned long) parameters->int0Parameter;
}

int GameScript::TrapTriggered(Scriptable* Sender, Trigger* parameters)
{
	if(Sender->Type!=ST_TRIGGER) {
		return 0;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	return (Sender->LastTrigger==tar);
}

int GameScript::InteractingWith(Scriptable* Sender, Trigger* parameters)
{
	if(Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	GameControl *gc = core->GetGameControl();
	if(Sender != gc->target && Sender!=gc->speaker) {
		return 0;
	}
	if(tar != gc->target && tar!=gc->speaker) {
		return 0;
	}
	return 1;
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
	SetVariable( Sender, parameters->string0Parameter, "GLOBAL",
		parameters->int0Parameter );
}

void GameScript::SetGlobal(Scriptable* Sender, Action* parameters)
{
	SetVariable( Sender, parameters->string0Parameter,
		parameters->int0Parameter );
}

void GameScript::SetGlobalTimer(Scriptable* Sender, Action* parameters)
{
	unsigned long mytime;

	//GetTime(mytime); //actually, this should be game time, not real time
	mytime=core->GetGame()->GameTime;
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

void GameScript::SetFaction(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->SetStat( IE_FACTION, parameters->int0Parameter );
}

void GameScript::SetHP(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->SetStat( IE_HITPOINTS, parameters->int0Parameter );
}

void GameScript::SetTeam(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->SetStat( IE_TEAM, parameters->int0Parameter );
}

void GameScript::TriggerActivation(Scriptable* Sender, Action* parameters)
{
	Scriptable* ip;

	if(!parameters->objects[1]->objectName[0]) {
		ip=Sender;
	} else {
		ip = core->GetGame()->GetCurrentMap( )->tm->GetInfoPoint(parameters->objects[1]->objectName);
	}
	if(!ip) {
		printf("Script error: No Trigger Named \"%s\"\n", parameters->objects[1]->objectName);
		return;
	}
	ip->Active = ( parameters->int0Parameter != 0 );
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
	Map *map = core->GetGame()->GetCurrentMap();
	ab->SetPosition( map, parameters->XpointParameter, parameters->YpointParameter, true );
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
	Map *map = core->GetGame()->GetCurrentMap();
	ab->SetPosition( map, parameters->XpointParameter, parameters->YpointParameter, true );
}

void GameScript::JumpToObject(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );

	if (!tar) {
		return;
	}
	char Area[9];

	if(tar->Type == ST_ACTOR) {
		Actor *ac = ( Actor* ) tar;
		memcpy(Area, ac->Area, 9);
	}
	else {
		Area[0]=0;
	}
	if(parameters->string0Parameter[0]) {
		CreateVisualEffectCore(Sender->XPos, Sender->YPos, parameters->string0Parameter);
	}
	MoveBetweenAreasCore( (Actor *) Sender, Area, tar->XPos, tar->YPos, -1, true);
}

void GameScript::MoveGlobalsTo(Scriptable* Sender, Action* parameters)
{
	Game *game = core->GetGame();
	int i = game->GetPartySize(false);
	while (i--) {
		Actor *tar = game->GetPC(i);
		//if the actor isn't in the area, we don't care
		if (strnicmp(tar->Area, parameters->string0Parameter,8) ) {
			continue;
		}
		MoveBetweenAreasCore( tar, parameters->string1Parameter, 
			parameters->XpointParameter,
			parameters->YpointParameter, -1, true);
	}
	i = game->GetNPCCount();
	while (i--) {
		Actor *tar = game->GetNPC(i);
		//if the actor isn't in the area, we don't care
		if (strnicmp(tar->Area, parameters->string0Parameter,8) ) {
			continue;
		}
		MoveBetweenAreasCore( tar, parameters->string1Parameter, 
			parameters->XpointParameter,
			parameters->YpointParameter, -1, true);
	}
}

void GameScript::MoveGlobal(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if(!tar || tar->Type != ST_ACTOR) {
		return;
	}
	MoveBetweenAreasCore( (Actor *) tar, parameters->string0Parameter,
		parameters->XpointParameter, parameters->YpointParameter, -1, true);
}

//we also allow moving to door, container
void GameScript::MoveGlobalObject(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if(!tar || tar->Type != ST_ACTOR) {
		return;
	}
	Scriptable* to = GetActorFromObject( Sender, parameters->objects[2] );
	if(!to) {
		return;
	}
	MoveBetweenAreasCore( (Actor *) tar, parameters->string0Parameter,
		to->XPos, to->YPos, -1, true);
}

void GameScript::MoveGlobalObjectOffScreen(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if(!tar || tar->Type != ST_ACTOR) {
		return;
	}
	Scriptable* to = GetActorFromObject( Sender, parameters->objects[2] );
	if(!to) {
		return;
	}
	MoveBetweenAreasCore( (Actor *) tar, parameters->string0Parameter,
		to->XPos, to->YPos, -1, false);
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
	return strtol(repvalue,NULL,0); //this one handles 0x values too!
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
	DataStream* ds = core->GetResourceMgr()->GetResource( parameters->string0Parameter, IE_CRE_CLASS_ID );
	aM->Open( ds, true );
	Actor* ab = aM->GetActor();
	core->FreeInterface( aM );
	int x,y;

	switch (flags & CC_MASK) {
		case CC_OFFSCREEN:
			x = 0;
			y = 0;
			break;
		case CC_OBJECT://use object + offset
			Sender = GetActorFromObject( Sender, parameters->objects[1] );
			//falling through
		case CC_OFFSET://use sender + offset
			x = parameters->XpointParameter+Sender->XPos;
			y = parameters->YpointParameter+Sender->YPos;
			break;
		default: //absolute point, but -1,-1 means AtFeet
			x = parameters->XpointParameter;
			y = parameters->YpointParameter;
			if(x==-1 && y==-1) {
				x = Sender->XPos;
				y = Sender->YPos;
			}
			break;
	}
	printf("CreateCreature: %s at [%d.%d] face:%d\n",parameters->string0Parameter, x,y,parameters->int0Parameter);
	Map* map;
	Game* game=core->GetGame();
	if(Sender->Type==ST_AREA) {
		map = game->GetMap(Sender->scriptName);
	}
	else {
		map = game->GetCurrentMap( );
	}
	ab->SetPosition(map, x, y, flags&CC_CHECK_IMPASSABLE );
	ab->AnimID = IE_ANI_AWAKE;
	ab->Orientation = parameters->int0Parameter;
	map->AddActor( ab );

	//setting the deathvariable if it exists (iwd2)
	if(parameters->string1Parameter[0]) {
		ab->SetScriptName(parameters->string1Parameter);
	}
}

//don't use offset from Sender
void GameScript::CreateCreature(Scriptable* Sender, Action* parameters)
{
	CreateCreatureCore( Sender, parameters, CC_CHECK_IMPASSABLE );
}

//don't use offset from Sender
void GameScript::CreateCreatureImpassable(Scriptable* Sender, Action* parameters)
{
	CreateCreatureCore( Sender, parameters, 0 );
}

//use offset from Sender
void GameScript::CreateCreatureAtFeet(Scriptable* Sender, Action* parameters)
{
	CreateCreatureCore( Sender, parameters, CC_OFFSET | CC_CHECK_IMPASSABLE );
}

void GameScript::CreateCreatureOffScreen(Scriptable* Sender, Action* parameters)
{
	CreateCreatureCore( Sender, parameters, 0 ); //don't check impassable
}

//this is the same, object + offset
//using this for simple createcreatureobject, (0 offsets)
void GameScript::CreateCreatureObjectOffset(Scriptable* Sender, Action* parameters)
{
	CreateCreatureCore( Sender, parameters, CC_OBJECT | CC_CHECK_IMPASSABLE );
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
		Map* map = core->GetGame()->GetCurrentMap( );
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
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) tar;
	actor->StrRefs[parameters->int0Parameter]=parameters->int1Parameter;
}

//this one works only on real actors, they got constants
void GameScript::VerbalConstantHead(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) tar;
	printf( "Displaying string on: %s\n", actor->scriptName );
	char *str=core->GetString( actor->StrRefs[parameters->int0Parameter], 2 );
	GameControl *gc=core->GetGameControl();
	if (gc) {
		gc->DisplayString( str);
	}
	//this will free the string, no need of freeing it up!
	actor->DisplayHeadText( str);
}

void GameScript::VerbalConstant(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) tar;
	printf( "Displaying string on: %s\n", actor->scriptName );
	GameControl *gc=core->GetGameControl();
	char *str = core->GetString( actor->StrRefs[parameters->int0Parameter], 2 );
	if (gc) {
		gc->DisplayString(str);
	}
	free(str);
}

void GameScript::SaveLocation(Scriptable* Sender, Action* parameters)
{
	unsigned int value;

	*((unsigned short *) &value) = parameters->XpointParameter;
	*(((unsigned short *) &value)+1) = (unsigned short) parameters->YpointParameter;
	printf("SaveLocation: %s\n",parameters->string0Parameter);
	SetVariable(Sender, parameters->string0Parameter, value);
}

void GameScript::SaveObjectLocation(Scriptable* Sender, Action* parameters)
{
	unsigned int value;

	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	*((unsigned short *) &value) = tar->XPos;
	*(((unsigned short *) &value)+1) = (unsigned short) tar->YPos;
	printf("SaveLocation: %s\n",parameters->string0Parameter);
	SetVariable(Sender, parameters->string0Parameter, value);
}

void GameScript::CreateCreatureAtLocation(Scriptable* Sender, Action* parameters)
{
	unsigned int value = CheckVariable(Sender, parameters->string0Parameter);
	parameters->XpointParameter = *(unsigned short *) value;
	parameters->XpointParameter = *(((unsigned short *) value)+1);
	CreateCreatureCore(Sender, parameters, CC_CHECK_IMPASSABLE);
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
	if (Sender) {
		printf( "Displaying string on: %s (without name)\n", Sender->scriptName );
		//no need of freeing this string up!!!
		Sender->DisplayHeadText( core->GetString( parameters->int0Parameter, 2 ) );
	}
}

void GameScript::DisplayStringHead(Scriptable* Sender, Action* parameters)
{
	if (Sender) {
		printf( "Displaying string on: %s\n", Sender->scriptName );
		//no need of freeing this string up!!!
		Sender->DisplayHeadText( core->GetString( parameters->int0Parameter, 2 ) );
	}
}

void GameScript::ForceFacing(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if(!tar) {
		Sender->CurrentAction = NULL;
		return;
	}
	Actor *actor = (Actor *) tar;
	actor->Orientation = parameters->int0Parameter;
}

void GameScript::Face(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->Orientation = parameters->int0Parameter;
	actor->resetAction = true;
	actor->SetWait( 1 );
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
	actor->Orientation = GetOrient( target->XPos, target->YPos,
					actor->XPos, actor->YPos );
	actor->resetAction = true;
	actor->SetWait( 1 );
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
		unsigned long counter = ( AI_UPDATE_TIME * len ) / 1000;
		if (counter != 0)
			actor->SetWait( counter );
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

void GameScript::CreateVisualEffectCore(int X, int Y, const char *effect)
{
	DataStream* ds = core->GetResourceMgr()->GetResource( effect, IE_VVC_CLASS_ID );
	ScriptedAnimation* vvc = new ScriptedAnimation( ds, true, X, Y );
	core->GetGame()->GetCurrentMap( )->AddVVCCell( vvc );
}

void GameScript::CreateVisualEffectObject(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	CreateVisualEffectCore(tar->XPos, tar->YPos, parameters->string0Parameter);
}

void GameScript::CreateVisualEffect(Scriptable* Sender, Action* parameters)
{
	CreateVisualEffectCore(parameters->XpointParameter, parameters->YpointParameter, parameters->string0Parameter);
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
	GameControl* gc = core->GetGameControl();
	if (gc) {
		gc->UnhideGUI();
	}
	EndCutSceneMode( Sender, parameters );
}

void GameScript::HideGUI(Scriptable* Sender, Action* parameters)
{
	GameControl* gc = core->GetGameControl();
	if (gc) {
		gc->HideGUI();
	}
}

static char PlayerDialogRes[9] = "PLAYERx\0";

void GameScript::BeginDialog(Scriptable* Sender, Action* parameters, int Flags)
{
	Scriptable* tar, *scr;

	if(InDebug) {
		printf("BeginDialog core\n");
	}
	if (Flags & BD_OWN) {
		scr = tar = GetActorFromObject( Sender, parameters->objects[1] );
	} else {
		if(Flags & BD_NUMERIC) {
			//the target was already set, this is a crude hack
			tar = core->GetGameControl()->target;
		}
		else {
			tar = GetActorFromObject( Sender, parameters->objects[1] );
		}
		scr = Sender;
	}
	if(!tar) {
		printf("[IEScript]: Target for dialog couldn't be found.\n");
		parameters->objects[1]->Dump();
		Sender->CurrentAction = NULL;
		return;
	}
	//source could be other than Actor, we need to handle this too!
	if (tar->Type != ST_ACTOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	if(Flags&BD_CHECKDIST) {
		if(Distance(Sender, tar)>40) {
			Point p={tar->XPos, tar->YPos};
			GoNearAndRetry(Sender, &p);
			Sender->CurrentAction = NULL;
			return;
		}
	}

	Actor* actor = ( Actor* ) scr;
	Actor* target = ( Actor* ) tar;

	GameControl* gc = core->GetGameControl();
	if (!gc) {
		printf( "[IEScript]: Dialog cannot be initiated because there is no GameControl.\n" );
		Sender->CurrentAction = NULL;
		return;
	}
	//can't initiate dialog, because it is already there
	if (gc->DialogueFlags&DF_IN_DIALOG) {
		if (Flags & BD_INTERRUPT) {
			//break the current dialog if possible
			gc->EndDialog(true);
		}
		//check if we could manage to break it, not all dialogs are breakable!
		if (gc->DialogueFlags&DF_IN_DIALOG) {
			printf( "[IEScript]: Dialog cannot be initiated because there is already one.\n" );
			Sender->CurrentAction = NULL;
			return;
		}
	}

	const char* Dialog = NULL;

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
		case BD_INTERACT: //using the source for the dialog
			int pdtable = core->LoadTable( "interdia" );
			char* scriptingname = actor->GetScriptName();
			Dialog = core->GetTable( pdtable )->QueryField( scriptingname, "FILE" );
			core->DelTable( pdtable );
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
		if (Flags & BD_TALKCOUNT) {
			gc->DialogueFlags|=DF_TALKCOUNT;
		}

		if(Flags & BD_TARGET) {
			gc->InitDialog( target, actor, Dialog );
		}
		else {
			gc->InitDialog( actor, target, Dialog );
		}
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
	//no need of freeing this string
	Sender->DisplayHeadText( core->GetString( parameters->int0Parameter) );
	Sender->textDisplaying = 0; //why?
	GameControl* gc = core->GetGameControl();
	if (gc) {
		gc->DisplayString( Sender );
	}
}

void GameScript::AmbientActivate(Scriptable* Sender, Action* parameters)
{
	Animation* anim = core->GetGame()->GetCurrentMap( )->GetAnimation( parameters->objects[1]->objectName );
	if (!anim) {
		printf( "Script error: No Animation Named \"%s\"\n",
			parameters->objects[1]->objectName );
		return;
	}
	anim->Active = ( parameters->int0Parameter != 0 );
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
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* target = ( Actor* ) Sender;
	target->SetDialog( parameters->string0Parameter );
}

void GameScript::ChangeDialogue(Scriptable* Sender, Action* parameters)
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

//start talking to oneself
void GameScript::PlayerDialogue(Scriptable* Sender, Action* parameters)
{
	BeginDialog( Sender, parameters, BD_RESERVED | BD_OWN );
}

//we hijack this action for the player initiated dialogue
void GameScript::NIDSpecial1(Scriptable* Sender, Action* parameters)
{
	BeginDialog( Sender, parameters, BD_TARGET | BD_NUMERIC | BD_TALKCOUNT | BD_CHECKDIST );
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

//no talkcount, using banter scripts
void GameScript::Interact(Scriptable* Sender, Action* parameters)
{
	BeginDialog( Sender, parameters, BD_INTERACT | BD_SOURCE );
}

static Point* FindNearPoint(Scriptable* Sender, Point* p1, Point* p2, double& distance)
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
	Scriptable* tar = core->GetGame()->GetCurrentMap( )->tm->GetDoor( parameters->objects[1]->objectName );
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
	Scriptable* tar = core->GetGame()->GetCurrentMap( )->tm->GetDoor( parameters->objects[1]->objectName );
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
	Scriptable* tar = core->GetGame()->GetCurrentMap( )->tm->GetDoor( parameters->objects[1]->objectName );
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
	double distance;
	Point* p = FindNearPoint( Sender, &door->toOpen[0], &door->toOpen[1],
				distance );
	if (distance <= 40) {
		if(door->Flags&2) {
			//playsound unsuccessful opening of door
			core->GetSoundMgr()->Play("AMB_D06");
		}
		else {
			door->SetDoorClosed( false, true );
		}
	} else {
		GoNearAndRetry(Sender, p);
	}
	Sender->CurrentAction = NULL;
}

void GameScript::CloseDoor(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = core->GetGame()->GetCurrentMap( )->tm->GetDoor( parameters->objects[1]->objectName );
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
	double distance;
	Point* p = FindNearPoint( Sender, &door->toOpen[0], &door->toOpen[1],
				distance );	
	if (distance <= 40) {
		door->SetDoorClosed( true, true );
	} else {
		GoNearAndRetry(Sender, p);
	}
	Sender->CurrentAction = NULL;
}

void GameScript::MoveBetweenAreasCore(Actor* actor, const char *area, int X, int Y, int face, bool adjust)
{
	printf("MoveBetweenAreas: %s to %s [%d.%d] face: %d\n", actor->GetName(0), area,X,Y, face);
	Map* map2;
	Game* game = core->GetGame();
	if(area[0]) { //do we need to switch area?
		Map* map1 = game->GetMap(actor->Area);
		//we have to change the pathfinder
		//to the target area if adjust==true
		map2 = game->GetMap(area);
		if( map1!=map2 ) {
			if(map1) {
				map1->RemoveActor( actor );
			}
		        map2->AddActor( actor );
		}
	}
	else {
		map2=game->GetMap(actor->Area);
	}
	actor->SetPosition(map2, X, Y, adjust);
	if (face !=-1) {
		actor->Orientation = face;
	}
}

void GameScript::MoveBetweenAreas(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	if(parameters->string1Parameter[0]) {
		CreateVisualEffectCore(Sender->XPos, Sender->YPos, parameters->string1Parameter);
	}
	MoveBetweenAreasCore((Actor *) Sender, parameters->string0Parameter,
		parameters->XpointParameter, parameters->YpointParameter,
		parameters->int0Parameter, true);
}

void GetPositionFromScriptable(Scriptable* scr, unsigned short& X,
	unsigned short& Y)
{
	switch (scr->Type) {
		case ST_AREA:
		case ST_GLOBAL:
			X = Y = 0;
			break;
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
	long gold = (long) CheckVariable( Sender, parameters->string0Parameter );
	act->NewStat(IE_GOLD, -gold, MOD_ADDITIVE);
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
	act->NewStat(IE_GOLD, -gold, MOD_ADDITIVE);
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
	act->NewStat(IE_GOLD, gold, MOD_ADDITIVE);
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
	actor->NewStat(IE_XP, parameters->int0Parameter, MOD_ADDITIVE);
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
		core->GetGame()->ShareXP(atoi(xpvalue+2) ); //no hex value
	}
	else {
		Actor* actor = ( Actor* ) core->GetGame()->GetPC(0);
		actor->NewStat(IE_XP, strtol(xpvalue,NULL,0), MOD_ADDITIVE);
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

void GameScript::SetMoraleAI(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* act = ( Actor* ) Sender;
	act->NewStat(IE_MORALEBREAK, parameters->int0Parameter, MOD_ABSOLUTE);
}

void GameScript::IncMoraleAI(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* act = ( Actor* ) Sender;
	act->NewStat(IE_MORALEBREAK, parameters->int0Parameter, MOD_ADDITIVE);
}

void GameScript::MoraleSet(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	if (tar->Type != ST_ACTOR) {
		return;
	}
	Actor* act = ( Actor* ) tar;
	act->NewStat(IE_MORALEBREAK, parameters->int0Parameter, MOD_ABSOLUTE);
}

void GameScript::MoraleInc(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	if (tar->Type != ST_ACTOR) {
		return;
	}
	Actor* act = ( Actor* ) tar;
	act->NewStat(IE_MORALEBREAK, parameters->int0Parameter, MOD_ADDITIVE);
}

void GameScript::MoraleDec(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	if (tar->Type != ST_ACTOR) {
		return;
	}
	Actor* act = ( Actor* ) tar;
	act->NewStat(IE_MORALEBREAK, -parameters->int0Parameter, MOD_ADDITIVE);
}

void GameScript::JoinParty(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	/* calling this, so it is simpler to change */
	SetBeenInPartyFlags(Sender, parameters);
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
	core->GetGUIScriptEngine()->RunFunction( "PopulatePortraitWindow" );

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
	core->GetGUIScriptEngine()->RunFunction( "PopulatePortraitWindow" );
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
	strncpy(core->GetGame()->LoadMos, parameters->string1Parameter,8);
	MoveBetweenAreasCore( actor, parameters->string0Parameter, parameters->XpointParameter, parameters->YpointParameter, parameters->int0Parameter, true);
}

void GameScript::LeaveAreaLUAEntry(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) Sender;
	Game *game = core->GetGame();
	strncpy(game->LoadMos, parameters->string1Parameter,8);
	//no need to change the pathfinder just for getting the entrance
	Map *map = game->GetMap(actor->Area);
	Entrance *ent = map->GetEntrance(parameters->string1Parameter);
	if (Distance(ent->XPos, ent->YPos, Sender) <= 40) {
		LeaveAreaLUA(Sender, parameters);
		return;
	}
	Sender->AddActionInFront( Sender->CurrentAction );
	char Tmp[256];
	sprintf( Tmp, "MoveToPoint([%d.%d])", ent->XPos, ent->YPos );
	Sender->AddActionInFront( GameScript::GenerateAction( Tmp, true ) );
}

void GameScript::LeaveAreaLUAPanic(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	strncpy(core->GetGame()->LoadMos, parameters->string1Parameter,8);
	MoveBetweenAreasCore( actor, parameters->string0Parameter, parameters->XpointParameter, parameters->YpointParameter, parameters->int0Parameter, true);
}

void GameScript::LeaveAreaLUAPanicEntry(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) Sender;
	Game *game = core->GetGame();
	strncpy(game->LoadMos, parameters->string1Parameter,8);
	//no need to change the pathfinder just for getting the entrance
	Map *map = game->GetMap( actor->Area );
	Entrance *ent = map->GetEntrance(parameters->string1Parameter);
	if (Distance(ent->XPos, ent->YPos, Sender) <= 40) {
		LeaveAreaLUAPanic(Sender, parameters);
		return;
	}
	Sender->AddActionInFront( Sender->CurrentAction );
	char Tmp[256];
	sprintf( Tmp, "MoveToPoint([%d.%d])", ent->XPos, ent->YPos );
	Sender->AddActionInFront( GameScript::GenerateAction( Tmp, true ) );
}

void GameScript::SetTokenGlobal(Scriptable* Sender, Action* parameters)
{
	long value = CheckVariable( Sender, parameters->string0Parameter );
	char varname[33]; //this is the Token Name
	strncpy( varname, parameters->string1Parameter, 32 );
	varname[32] = 0;
	char tmpstr[10];
	sprintf( tmpstr, "%ld", value );
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

/* this may not be correct, just a placeholder you can fix */
void GameScript::Swing(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->AnimID = IE_ANI_ATTACK;
	actor->SetWait( 1 );
}

/* this may not be correct, just a placeholder you can fix */
void GameScript::SwingOnce(Scriptable* Sender, Action* parameters)
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
	long value1 = CheckVariable( Sender, parameters->string0Parameter );
	if (value1 > parameters->int0Parameter) {
		SetVariable( Sender, parameters->string0Parameter, value1 );
	}
}

void GameScript::GlobalMin(Scriptable* Sender, Action* parameters)
{
	long value1 = CheckVariable( Sender, parameters->string0Parameter );
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

	for (int i = 0; i < game->GetPartySize(0); i++) {
		Actor* act = game->GetPC( i );
		if (act) {
			act->ClearActions();
		}
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

void GameScript::TextScreen(Scriptable* Sender, Action* parameters)
{
	int chapter = core->LoadTable( parameters->string0Parameter );
	if(chapter<0) {
//add some standard way of notifying the user
//		MissingResource(parameters->string0Parameter);
		return;
	}
	GameControl *gc=core->GetGameControl();
	if (gc) {
		char *strref = core->GetTable(chapter)->QueryField(0);
		char *str=core->GetString( strtol(strref,NULL,0) );
		gc->DisplayString(str);
		free(str);
		strref = core->GetTable(chapter)->QueryField(1);
		str=core->GetString( strtol(strref,NULL,0) );
		gc->DisplayString(str);
		free(str);
	}
}

void GameScript::IncrementChapter(Scriptable* Sender, Action* parameters)
{
	TextScreen(Sender, parameters);
	unsigned long value=0;

	core->GetGame()->globals->Lookup( "GLOBALCHAPTER", value );
	core->GetGame()->globals->SetAt( "GLOBALCHAPTER", value+1 );
}

void GameScript::SetBeenInPartyFlags(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	//i think it is bit 15 of the multi-class flags
	actor->SetStat(IE_MC_FLAGS,actor->GetStat(IE_MC_FLAGS)|32768);
}

void GameScript::SetTextColor(Scriptable* Sender, Action* parameters)
{
	GameControl *gc=core->GetGameControl();
	if(gc) {
		Color color;

		memcpy(&color,&parameters->int0Parameter,4);
		gc->SetInfoTextColor( color );
	}
}

void GameScript::BitGlobal(Scriptable* Sender, Action* parameters)
{
	unsigned long value = CheckVariable(Sender, parameters->string0Parameter );
	switch(parameters->int1Parameter) {
		case BM_AND:
			value = ( value& parameters->int0Parameter );
		case BM_OR:
			value = ( value| parameters->int0Parameter );
		case BM_XOR:
			value = ( value^ parameters->int0Parameter );
		case BM_NAND: //this is a GemRB extension
			value = ( value& ~parameters->int0Parameter );
	}
	SetVariable(Sender, parameters->string0Parameter, value);
}

void GameScript::GlobalBitGlobal(Scriptable* Sender, Action* parameters)
{
	unsigned long value1 = CheckVariable(Sender, parameters->string0Parameter );
	unsigned long value2 = CheckVariable(Sender, parameters->string1Parameter );
	switch(parameters->int1Parameter) {
		case BM_AND:
			value1 = ( value1& value2);
		case BM_OR:
			value1 = ( value1| value2);
		case BM_XOR:
			value1 = ( value1^ value2);
		case BM_NAND: //this is a GemRB extension
			value1 = ( value1& ~value2);
	}
	SetVariable(Sender, parameters->string0Parameter, value1);
}

void GameScript::SetVisualRange(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->SetStat(IE_VISUALRANGE,parameters->int0Parameter);
}

void GameScript::MakeUnselectable(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->SetStat(IE_UNSELECTABLE,parameters->int0Parameter);
}

void GameScript::Debug(Scriptable* Sender, Action* parameters)
{
	InDebug=1;
	printMessage("IEScript",parameters->string0Parameter,YELLOW);
}

void GameScript::IncrementProficiency(Scriptable* Sender, Action* parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>31) {
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
	//start of the proficiency stats
	target->NewStat(IE_PROFICIENCYBASTARDSWORD+idx,
		parameters->int1Parameter,MOD_ADDITIVE);
}

// i'm unsure why this exists
void GameScript::IncrementExtraProficiency(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	if (tar->Type != ST_ACTOR) {
		return;
	}
	Actor* target = ( Actor* ) tar;
	target->NewStat(IE_EXTRAPROFICIENCY1, parameters->int0Parameter,MOD_ADDITIVE);
}

//the third parameter is a GemRB extension
void GameScript::AddJournalEntry(Scriptable* Sender, Action* parameters)
{
	core->GetGame()->AddJournalEntry(parameters->int0Parameter, parameters->int1Parameter, parameters->int2Parameter);
}

void GameScript::SetQuestDone(Scriptable* Sender, Action* parameters)
{
	core->GetGame()->DeleteJournalEntry(parameters->int0Parameter);
	core->GetGame()->AddJournalEntry(parameters->int0Parameter, IE_GAM_QUEST_DONE, parameters->int2Parameter);

}

void GameScript::RemoveJournalEntry(Scriptable* Sender, Action* parameters)
{
	core->GetGame()->DeleteJournalEntry(parameters->int0Parameter);
}

void GameScript::SetInternal(Scriptable* Sender, Action* parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>15) {
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
	//start of the internal stats
	target->NewStat(IE_INTERNAL_0+idx, parameters->int1Parameter,MOD_ABSOLUTE);
}

void GameScript::IncInternal(Scriptable* Sender, Action* parameters)
{
	unsigned int idx = parameters->int0Parameter;
	if (idx>15) {
		return;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	Actor* target = ( Actor* ) tar;
	//start of the internal stats
	target->NewStat(IE_INTERNAL_0+idx, parameters->int1Parameter,MOD_ADDITIVE);
}

void GameScript::DestroyAllEquipment(Scriptable* Sender, Action* parameters)
{
	Inventory *inv=NULL;

 	switch (Sender->Type) {
		case ST_ACTOR:
			inv = &(((Actor *) Sender)->inventory);
			break;
		case ST_CONTAINER:
			inv = &(((Container *) Sender)->inventory);
			break;
		default:;
	}
	if(inv) {
		inv->DestroyItem(NULL,0);
	}
}

void GameScript::DestroyItem(Scriptable* Sender, Action* parameters)
{
	Inventory *inv=NULL;

 	switch (Sender->Type) {
		case ST_ACTOR:
			inv = &(((Actor *) Sender)->inventory);
			break;
		case ST_CONTAINER:
			inv = &(((Container *) Sender)->inventory);
			break;
		default:;
	}
	if(inv) {
		inv->DestroyItem(parameters->string0Parameter,0);
	}
}

void GameScript::DestroyAllDestructableEquipment(Scriptable* Sender, Action* parameters)
{
	Inventory *inv=NULL;

 	switch (Sender->Type) {
		case ST_ACTOR:
			inv = &(((Actor *) Sender)->inventory);
			break;
		case ST_CONTAINER:
			inv = &(((Container *) Sender)->inventory);
			break;
		default:;
	}
	if(inv) {
		inv->DestroyItem(NULL, IE_ITEM_DESTRUCTIBLE);
	}
}

void GameScript::SetApparentName(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	Actor* target = ( Actor* ) tar;
	target->SetText(parameters->int0Parameter,1);
}

void GameScript::SetRegularName(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	Actor* target = ( Actor* ) tar;
	target->SetText(parameters->int0Parameter,2);
}

/** this is a gemrb extension */
void GameScript::UnloadArea(Scriptable* Sender, Action* parameters)
{
	int map=core->GetGame()->FindMap(parameters->string0Parameter);
	if(map>=0) {
		core->GetGame()->DelMap(map, parameters->int0Parameter);
	}
}

void GameScript::Kill(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	Actor* target = ( Actor* ) tar;
	target->Die(false); //die, no XP
}

