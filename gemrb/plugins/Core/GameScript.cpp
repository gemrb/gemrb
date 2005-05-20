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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/GameScript.cpp,v 1.273 2005/05/20 16:41:03 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "GameScript.h"
#include "Interface.h"
#include "../../includes/strrefs.h"
#include "../../includes/defsounds.h"

//this will skip to the next element in the prototype of an action/trigger
#define SKIP_ARGUMENT() while (*str && ( *str != ',' ) && ( *str != ')' )) str++

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
static Cache SrcCache; //cache for string resources (pst)
static Cache BcsCache; //cache for scripts
static int happiness[3][20];

static int ObjectIDSCount = 7;
static int MaxObjectNesting = 5;
static bool HasAdditionalRect = false;
static ieResRef *ObjectIDSTableNames;
static int ObjectFieldsCount = 7;
static int ExtraParametersCount = 0;
static int RandomNumValue;

//debug flags
// 1 - cache
// 2 - cutscene ID
// 4 - globals
// 8 - script/trigger execution

static int InDebug = 0;

#define MIC_INVALID -2
#define MIC_FULL -1
#define MIC_NOITEM 0
#define MIC_GOTITEM 1

//Make this an ordered list, so we could use bsearch!
static TriggerLink triggernames[] = {
	{"actionlistempty", GameScript::ActionListEmpty,0},
	{"acquired", GameScript::Acquired,0},
	{"alignment", GameScript::Alignment,0},
	{"allegiance", GameScript::Allegiance,0},
	{"animstate", GameScript::AnimState,0},
	{"anypconmap", GameScript::AnyPCOnMap,0},
	{"areacheck", GameScript::AreaCheck,0},
	{"areacheckobject", GameScript::AreaCheckObject,0},
	{"areaflag", GameScript::AreaFlag,0},
	{"areatype", GameScript::AreaType,0},
	{"bitcheck", GameScript::BitCheck,TF_MERGESTRINGS},
	{"bitcheckexact", GameScript::BitCheckExact,TF_MERGESTRINGS},
	{"bitglobal", GameScript::BitGlobal_Trigger,TF_MERGESTRINGS},
	{"breakingpoint", GameScript::BreakingPoint,0},
	{"calledbyname", GameScript::CalledByName,0}, //this is still a question
	{"charname", GameScript::CharName,0}, //not scripting name
	{"checkstat", GameScript::CheckStat,0},
	{"checkstatgt", GameScript::CheckStatGT,0},
	{"checkstatlt", GameScript::CheckStatLT,0},
	{"class", GameScript::Class,0},
	{"classex", GameScript::ClassEx,0}, //will return true for multis
	{"classlevel", GameScript::ClassLevel,0},
	{"classlevelgt", GameScript::ClassLevelGT,0},
	{"classlevellt", GameScript::ClassLevelLT,0},
	{"clicked", GameScript::Clicked,0},
	{"combatcounter", GameScript::CombatCounter,0},
	{"combatcountergt", GameScript::CombatCounterGT,0},
	{"combatcounterlt", GameScript::CombatCounterLT,0},
	{"contains", GameScript::Contains,0},
	{"creatureinarea", GameScript::AreaCheck,0}, //cannot check others
	{"damagetaken", GameScript::DamageTaken,0},
	{"damagetakengt", GameScript::DamageTakenGT,0},
	{"damagetakenlt", GameScript::DamageTakenLT,0},
	{"dead", GameScript::Dead,0},
	{"die", GameScript::Die,0},
	{"difficulty", GameScript::Difficulty,0},
	{"difficultygt", GameScript::DifficultyGT,0},
	{"difficultylt", GameScript::DifficultyLT,0},
	{"entered", GameScript::Entered,0},
	{"entirepartyonmap", GameScript::EntirePartyOnMap,0},
	{"exists", GameScript::Exists,0},
	{"extraproficiency", GameScript::ExtraProficiency,0},
	{"extraproficiencygt", GameScript::ExtraProficiencyGT,0},
	{"extraproficiencylt", GameScript::ExtraProficiencyLT,0},
	{"faction", GameScript::Faction,0},
	{"fallenpaladin", GameScript::FallenPaladin,0},
	{"fallenranger", GameScript::FallenRanger,0},
	{"false", GameScript::False,0},
	{"g", GameScript::G_Trigger,0},
	{"gender", GameScript::Gender,0},
	{"general", GameScript::General,0},
	{"ggt", GameScript::GGT_Trigger,0},
	{"glt", GameScript::GLT_Trigger,0},
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
	{"globalorglobal", GameScript::GlobalOrGlobal_Trigger,TF_MERGESTRINGS},
	{"globalsequal", GameScript::GlobalsEqual,TF_MERGESTRINGS},
	{"globalsgt", GameScript::GlobalsGT,TF_MERGESTRINGS},
	{"globalslt", GameScript::GlobalsLT,TF_MERGESTRINGS},
	{"globaltimerexact", GameScript::GlobalTimerExact,TF_MERGESTRINGS},
	{"globaltimerexpired", GameScript::GlobalTimerExpired,TF_MERGESTRINGS},
	{"globaltimernotexpired", GameScript::GlobalTimerNotExpired,TF_MERGESTRINGS},
	{"happiness", GameScript::Happiness,0},
	{"happinessgt", GameScript::HappinessGT,0},
	{"happinesslt", GameScript::HappinessLT,0},
	{"harmlessentered", GameScript::Entered,0}, //this isn't sure the same
	{"hasitem", GameScript::HasItem,0},
	{"hasitemequiped", GameScript::HasItemEquipped,0}, //typo in bg2
	{"hasitemequipped", GameScript::HasItemEquipped,0},
	{"hasitemslot", GameScript::HasItemSlot,0},
	{"hasiteminslot", GameScript::HasItemSlot,0},
	{"hasweaponequipped", GameScript::HasWeaponEquipped,0},
	{"haveanyspells", GameScript::HaveAnySpells,0},
	{"havespell", GameScript::HaveSpell,0}, //these must be the same
	{"havespellparty", GameScript::HaveSpellParty,0}, 
	{"havespellres", GameScript::HaveSpell,0}, //they share the same ID
	{"heard", GameScript::Heard,0},
	{"help", GameScript::Help_Trigger,0},
	{"hitby", GameScript::HitBy,0},
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
	{"inventoryfull", GameScript::InventoryFull,0},
	{"inweaponrange", GameScript::InWeaponRange,0},
	{"isaclown", GameScript::IsAClown,0},
	{"isactive", GameScript::IsActive,0},
	{"isanimationid", GameScript::AnimationID,0},
	{"isgabber", GameScript::IsGabber,0},
	{"islocked", GameScript::IsLocked,0},
	{"isextendednight", GameScript::IsExtendedNight,0},
	{"isscriptname", GameScript::CalledByName,0}, //seems the same
	{"isvalidforpartydialog", GameScript::IsValidForPartyDialog,0},
	{"itemisidentified", GameScript::ItemIsIdentified,0},
	{"kit", GameScript::Kit,0},
	{"lastmarkedobject", GameScript::LastMarkedObject_Trigger,0},
	{"level", GameScript::Level,0},
	{"levelgt", GameScript::LevelGT,0},
	{"levellt", GameScript::LevelLT,0},
	{"levelparty", GameScript::LevelParty,0},
	{"levelpartygt", GameScript::LevelPartyGT,0},
	{"levelpartylt", GameScript::LevelPartyLT,0},
	{"localsequal", GameScript::LocalsEqual,TF_MERGESTRINGS},
	{"localsgt", GameScript::LocalsGT,TF_MERGESTRINGS},
	{"localslt", GameScript::LocalsLT,TF_MERGESTRINGS},
	{"los", GameScript::LOS,0},
	{"morale", GameScript::Morale,0},
	{"moralegt", GameScript::MoraleGT,0},
	{"moralelt", GameScript::MoraleLT,0},
	{"name", GameScript::CalledByName,0}, //this is the same too?
	{"namelessbitthedust", GameScript::NamelessBitTheDust,0},
	{"nearbydialog", GameScript::NearbyDialog,0},
	{"nearlocation", GameScript::NearLocation,0},
	{"nearsavedlocation", GameScript::NearSavedLocation,0},
	{"notstatecheck", GameScript::NotStateCheck,0},
	{"nulldialog", GameScript::NullDialog,0},
	{"numcreature", GameScript::NumCreatures,0},
	{"numcreaturegt", GameScript::NumCreaturesGT,0},
	{"numcreaturelt", GameScript::NumCreaturesLT,0},
	{"numcreaturevsparty", GameScript::NumCreatureVsParty,0},
	{"numcreaturevspartygt", GameScript::NumCreatureVsPartyGT,0},
	{"numcreaturevspartylt", GameScript::NumCreatureVsPartyLT,0},
	{"numdead", GameScript::NumDead,0},
	{"numdeadgt", GameScript::NumDeadGT,0},
	{"numdeadlt", GameScript::NumDeadLT,0},
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
	{"onscreen", GameScript::OnScreen,0},
	{"openstate", GameScript::OpenState,0},
	{"or", GameScript::Or,0},
	{"ownsfloatermessage", GameScript::OwnsFloaterMessage,0},
	{"partycounteq", GameScript::PartyCountEQ,0},
	{"partycountgt", GameScript::PartyCountGT,0},
	{"partycountlt", GameScript::PartyCountLT,0},
	{"partygold", GameScript::PartyGold,0},
	{"partygoldgt", GameScript::PartyGoldGT,0},
	{"partygoldlt", GameScript::PartyGoldLT,0},
	{"partyhasitem", GameScript::PartyHasItem,0},
	{"partyhasitemidentified", GameScript::PartyHasItemIdentified,0},
	{"partymemberdied", GameScript::PartyMemberDied,0},
	{"pcinstore", GameScript::PCInStore,0},
	{"proficiency", GameScript::Proficiency,0},
	{"proficiencygt", GameScript::ProficiencyGT,0},
	{"proficiencylt", GameScript::ProficiencyLT,0},
	{"race", GameScript::Race,0},
	{"randomnum", GameScript::RandomNum,0},
	{"randomnumgt", GameScript::RandomNumGT,0},
	{"randomnumlt", GameScript::RandomNumLT,0},
	{"range", GameScript::Range,0},
	{"realglobaltimerexact", GameScript::RealGlobalTimerExact,TF_MERGESTRINGS},
	{"realglobaltimerexpired", GameScript::RealGlobalTimerExpired,TF_MERGESTRINGS},
	{"realglobaltimernotexpired", GameScript::RealGlobalTimerNotExpired,TF_MERGESTRINGS},
	{"reputation", GameScript::Reputation,0},
	{"reputationgt", GameScript::ReputationGT,0},
	{"reputationlt", GameScript::ReputationLT,0},
	{"isrotation", GameScript::IsRotation,0},
	{"see", GameScript::See,0},
	{"specifics", GameScript::Specifics,0},
	{"statecheck", GameScript::StateCheck,0},
	{"targetunreachable", GameScript::TargetUnreachable,0},
	{"team", GameScript::Team,0},
	{"time", GameScript::Time,0},
	{"timegt", GameScript::TimeGT,0},
	{"timelt", GameScript::TimeLT,0},
	{"tookdamage", GameScript::TookDamage,0},
	{"traptriggered", GameScript::TrapTriggered,0},
	{"triggerclick", GameScript::Clicked,0}, //not sure
	{"true", GameScript::True,0},
	{"unselectablevariable", GameScript::UnselectableVariable,0},
	{"unselectablevariablegt", GameScript::UnselectableVariableGT,0},
	{"unselectablevariablelt", GameScript::UnselectableVariableLT,0},
	{"vacant",GameScript::Vacant,0},
	{"xor", GameScript::Xor,TF_MERGESTRINGS},
	{"xp", GameScript::XP,0},
	{"xpgt", GameScript::XPGT,0},
	{"xplt", GameScript::XPLT,0}, { NULL,NULL,0},
};

//Make this an ordered list, so we could use bsearch!
static ActionLink actionnames[] = {
	{"actionoverride",NULL,0},
	{"activate", GameScript::Activate,0},
	{"addareaflag", GameScript::AddAreaFlag,0},
	{"addareatype", GameScript::AddAreaType,0},
	{"addexperienceparty", GameScript::AddExperienceParty,0},
	{"addexperiencepartyglobal", GameScript::AddExperiencePartyGlobal,AF_MERGESTRINGS},
	{"addglobals", GameScript::AddGlobals,0},
	{"addjournalentry", GameScript::AddJournalEntry,0},
	{"addmapnote", GameScript::AddMapnote,0},
	{"addspecialability", GameScript::AddSpecialAbility,0},
	{"addwaypoint", GameScript::AddWayPoint,AF_BLOCKING},
	{"addxp2da", GameScript::AddXP2DA,0},
	{"addxpobject", GameScript::AddXPObject,0},
	{"addxpvar", GameScript::AddXP2DA,0},
	{"advancetime", GameScript::AdvanceTime,0},
	{"allowarearesting", GameScript::SetAreaRestFlag,0},
	{"ally", GameScript::Ally,0},
	{"ambientactivate", GameScript::AmbientActivate,0},
	{"applydamage", GameScript::ApplyDamage,0},
	{"applydamagepercent", GameScript::ApplyDamagePercent,0},
	{"attack", GameScript::Attack,AF_BLOCKING},
	{"attackreevaluate", GameScript::AttackReevaluate,AF_BLOCKING},
	{"bashdoor", GameScript::OpenDoor,AF_BLOCKING}, //the same until we know better
	{"battlesong", GameScript::BattleSong,0},
	{"berserk", GameScript::Berserk,0}, 
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
	{"chunkcreature", GameScript::Kill,0}, //should be more graphical
	{"clearactions", GameScript::ClearActions,0},
	{"clearallactions", GameScript::ClearAllActions,0},
	{"closedoor", GameScript::CloseDoor,AF_BLOCKING},
	{"containerenable", GameScript::ContainerEnable,0},
	{"continue", GameScript::Continue,AF_INSTANT | AF_CONTINUE},
	{"createcreature", GameScript::CreateCreature,0}, //point is relative to Sender
	{"createcreatureatfeet", GameScript::CreateCreatureAtFeet,0}, 
	{"createcreatureatlocation", GameScript::CreateCreatureAtLocation,0},
	{"createcreatureimpassable", GameScript::CreateCreatureImpassable,0},
	{"createcreatureobject", GameScript::CreateCreatureObjectOffset,0}, //the same
	{"createcreatureobjectoffscreen", GameScript::CreateCreatureObjectOffScreen,0}, //same as createcreature object, but starts looking for a place far away from the player
	{"createcreatureobjectoffset", GameScript::CreateCreatureObjectOffset,0}, //the same
	{"createcreatureoffscreen", GameScript::CreateCreatureOffScreen,0},
	{"createitem", GameScript::CreateItem,0},
	{"createitemglobal", GameScript::CreateItemNumGlobal,0},
	{"createitemnumglobal", GameScript::CreateItemNumGlobal,0},
	{"createpartygold", GameScript::CreatePartyGold,0},
	{"createvisualeffect", GameScript::CreateVisualEffect,0},
	{"createvisualeffectobject", GameScript::CreateVisualEffectObject,0},
	{"cutsceneid", GameScript::CutSceneID,AF_INSTANT},
	{"damage", GameScript::Damage,0},
	{"deactivate", GameScript::Deactivate,0},
	{"debug", GameScript::Debug,0},
	{"debugoutput", GameScript::Debug,0},
	{"deletejournalentry", GameScript::RemoveJournalEntry,0},
	{"demoend", GameScript::QuitGame, 0}, //same for now
	{"destroyalldestructableequipment", GameScript::DestroyAllDestructableEquipment,0},
	{"destroyallequipment", GameScript::DestroyAllEquipment,0},
	{"destroygold", GameScript::DestroyGold,0},
	{"destroyitem", GameScript::DestroyItem,0},
	{"destroypartygold", GameScript::DestroyPartyGold,0},
	{"destroypartyitem", GameScript::DestroyPartyItem,0},
	{"destroyself", GameScript::DestroySelf,0},
	{"dialogue", GameScript::Dialogue,AF_BLOCKING},
	{"dialogueforceinterrupt", GameScript::DialogueForceInterrupt,AF_BLOCKING},
	{"displaymessage", GameScript::DisplayMessage,0},
	{"displaystring", GameScript::DisplayString,0},
	{"displaystringhead", GameScript::DisplayStringHead,0},
	{"displaystringheadowner", GameScript::DisplayStringHeadOwner,0},
	{"displaystringheaddead", GameScript::DisplayStringHead,0}, //same?
	{"displaystringnoname", GameScript::DisplayStringNoName,0},
	{"displaystringnonamehead", GameScript::DisplayStringNoNameHead,0},
	{"displaystringwait", GameScript::DisplayStringWait,AF_BLOCKING},
	{"dropinventory", GameScript::DropInventory, 0},
	{"dropitem", GameScript::DropItem, AF_BLOCKING},
	{"endcredits", GameScript::EndCredits, 0},//movie
	{"endcutscenemode", GameScript::EndCutSceneMode,0},
	{"enemy", GameScript::Enemy,0},
	{"equipitem", GameScript::EquipItem, AF_BLOCKING},
	{"erasejournalentry", GameScript::RemoveJournalEntry,0},
	{"expansionendcredits", GameScript::QuitGame, 0},//ends game too
	{"explore", GameScript::Explore,0},
	{"exploremapchunk", GameScript::ExploreMapChunk,0},
	{"face", GameScript::Face,AF_BLOCKING},
	{"faceobject", GameScript::FaceObject, AF_BLOCKING},
	{"facesavedlocation", GameScript::FaceSavedLocation, AF_BLOCKING},
	{"fadefromblack", GameScript::FadeFromColor,0}, //probably the same
	{"fadefromcolor", GameScript::FadeFromColor,0},
	{"fadetoblack", GameScript::FadeToColor,0}, //probably the same
	{"fadetocolor", GameScript::FadeToColor,0},
	{"floatmessage", GameScript::DisplayStringHead,0},
	{"floatmessagefixed", GameScript::FloatMessageFixed,0},
	{"floatmessagefixedrnd", GameScript::FloatMessageFixedRnd,0},
	{"floatmessagernd", GameScript::FloatMessageRnd,0},
	{"forceaiscript", GameScript::ForceAIScript,0},
	{"forceattack", GameScript::ForceAttack,0},
	{"forcefacing", GameScript::ForceFacing,0},
	{"forceleavearealua", GameScript::ForceLeaveAreaLUA,0},
	{"forcespell", GameScript::ForceSpell,0},
	{"fullheal", GameScript::FullHeal,0},
	{"getitem", GameScript::GetItem,0},
	{"giveexperience", GameScript::AddXPObject,0},
	{"givegoldforce", GameScript::CreatePartyGold,0}, //this is the same
	{"giveitem", GameScript::GiveItem,0},
	{"giveitemcreate", GameScript::CreateItem,0}, //actually this is a targeted createitem
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
	{"globalshout", GameScript::GlobalShout,0},
	{"globalshr", GameScript::GlobalShR,AF_MERGESTRINGS},
	{"globalshrglobal", GameScript::GlobalShRGlobal,AF_MERGESTRINGS},
	{"globalsubglobal", GameScript::GlobalSubGlobal,AF_MERGESTRINGS},
	{"globalxor", GameScript::GlobalXor,AF_MERGESTRINGS},
	{"globalxorglobal", GameScript::GlobalXorGlobal,AF_MERGESTRINGS},
	{"gotostartscreen", GameScript::QuitGame, 0},
	{"help", GameScript::Help,0},
	{"hideareaonmap", GameScript::HideAreaOnMap,0},
	{"hidecreature", GameScript::HideCreature,0},
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
	{"journalentrydone", GameScript::SetQuestDone,0},
	{"jumptoobject", GameScript::JumpToObject,0},
	{"jumptopoint", GameScript::JumpToPoint,0},
	{"jumptopointinstant", GameScript::JumpToPointInstant,0},
	{"jumptosavedlocation", GameScript::JumpToSavedLocation,0},
	{"kill", GameScript::Kill,0},
	{"killfloatmessage", GameScript::KillFloatMessage,0},
	{"leavearea", GameScript::LeaveAreaLUA,0}, //so far the same
	{"leavearealua", GameScript::LeaveAreaLUA,0},
	{"leavearealuaentry", GameScript::LeaveAreaLUAEntry,AF_BLOCKING},
	{"leavearealuapanic", GameScript::LeaveAreaLUAPanic,0},
	{"leavearealuapanicentry", GameScript::LeaveAreaLUAPanicEntry,AF_BLOCKING},
	{"leaveparty", GameScript::LeaveParty,0},
	{"lock", GameScript::Lock,AF_BLOCKING},//key not checked at this time!
	{"lockscroll", GameScript::LockScroll,0},
	{"log", GameScript::Debug,0}, //the same until we know better
	{"makeglobal", GameScript::MakeGlobal,0},
	{"makeunselectable", GameScript::MakeUnselectable,0},
	{"markobject", GameScript::MarkObject,0},
	{"moraledec", GameScript::MoraleDec,0},
	{"moraleinc", GameScript::MoraleInc,0},
	{"moraleset", GameScript::MoraleSet,0},
	{"movebetweenareas", GameScript::MoveBetweenAreas,0},
	{"movebetweenareaseffect", GameScript::MoveBetweenAreas,0},
	{"moveglobal", GameScript::MoveGlobal,0}, 
	{"moveglobalobject", GameScript::MoveGlobalObject,0}, 
	{"moveglobalobjectoffscreen", GameScript::MoveGlobalObjectOffScreen,0},
	{"moveglobalsto", GameScript::MoveGlobalsTo,0}, 
	{"movetocenterofscreen", GameScript::MoveToCenterOfScreen,AF_BLOCKING},
	{"movetoobject", GameScript::MoveToObject,AF_BLOCKING},
	{"movetoobjectnointerrupt", GameScript::MoveToObjectNoInterrupt,AF_BLOCKING},
	{"movetooffset", GameScript::MoveToOffset,AF_BLOCKING},
	{"movetopoint", GameScript::MoveToPoint,AF_BLOCKING},
	{"movetopointnointerrupt", GameScript::MoveToPointNoInterrupt,AF_BLOCKING},
	{"movetopointnorecticle", GameScript::MoveToPointNoRecticle,AF_BLOCKING},//the same until we know better
	{"movetosavedlocation", GameScript::MoveToSavedLocation,AF_BLOCKING},
	//take care of the typo in the original bg2 action.ids
	{"movetosavedlocationn", GameScript::MoveToSavedLocation,AF_BLOCKING},
	{"moveviewobject", GameScript::MoveViewPoint,0},
	{"moveviewpoint", GameScript::MoveViewPoint,0},
	{"nidspecial1", GameScript::NIDSpecial1,AF_BLOCKING},//we use this for dialogs, hack
	{"nidspecial2", GameScript::NIDSpecial2,AF_BLOCKING},//we use this for worldmap, another hack
	{"noaction", GameScript::NoAction,0},
	{"opendoor", GameScript::OpenDoor,AF_BLOCKING},
	{"panic", GameScript::Panic,0},
	{"permanentstatchange", GameScript::ChangeStat,0}, //probably the same
	{"picklock", GameScript::OpenDoor,AF_BLOCKING}, //the same until we know better
	{"pickpockets", GameScript::PickPockets, AF_BLOCKING},
	{"playdead", GameScript::PlayDead,AF_BLOCKING},
	{"playdeadinterruptable", GameScript::PlayDeadInterruptable,AF_BLOCKING},
	{"playerdialog", GameScript::PlayerDialogue,AF_BLOCKING},
	{"playerdialogue", GameScript::PlayerDialogue,AF_BLOCKING},
	{"playsequence", GameScript::PlaySequence,0},
	{"playsong", GameScript::StartSong,0},
	{"playsound", GameScript::PlaySound,0},
	{"playsoundnotranged", GameScript::PlaySoundNotRanged,0},
	{"playsoundpoint", GameScript::PlaySoundPoint,0},
	{"plunder", GameScript::Plunder,AF_BLOCKING},
	{"quitgame", GameScript::QuitGame, 0},
	{"randomfly", GameScript::RandomFly, AF_BLOCKING},
	{"randomwalk", GameScript::RandomWalk, AF_BLOCKING},
	{"realsetglobaltimer", GameScript::RealSetGlobalTimer,AF_MERGESTRINGS},
	{"recoil", GameScript::Recoil,0},
	{"regainpaladinhood", GameScript::RegainPaladinHood,0},
	{"regainrangerhood", GameScript::RegainRangerHood,0},
	{"removeareaflag", GameScript::RemoveAreaFlag,0},
	{"removeareatype", GameScript::RemoveAreaType,0},
	{"removejournalentry", GameScript::RemoveJournalEntry,0},
	{"removemapnote", GameScript::RemoveMapnote,0},
	{"removepaladinhood", GameScript::RemovePaladinHood,0},
	{"removerangerhood", GameScript::RemoveRangerHood,0},
	{"removespell", GameScript::RemoveSpell,0},
	{"reputationinc", GameScript::ReputationInc,0},
	{"reputationset", GameScript::ReputationSet,0},
	{"resetfogofwar", GameScript::UndoExplore,0}, //pst
	{"restorepartylocations", GameScript:: RestorePartyLocation,0},
	//this is in iwd2, same as movetosavedlocation, with a default variable
	{"returntosavedlocation", GameScript::MoveToSavedLocation, AF_BLOCKING},
	{"returntosavedlocationdelete", GameScript::MoveToSavedLocationDelete, AF_BLOCKING},
	{"returntosavedplace", GameScript::MoveToSavedLocation, AF_BLOCKING},
	{"revealareaonmap", GameScript::RevealAreaOnMap,0},
	{"runawayfrom", GameScript::RunAwayFrom,AF_BLOCKING},
	{"runawayfromnointerrupt", GameScript::RunAwayFromNoInterrupt,AF_BLOCKING},
	{"runawayfrompoint", GameScript::RunAwayFromPoint,AF_BLOCKING},
	{"runtoobject", GameScript::MoveToObject,AF_BLOCKING}, //until we know better
	{"runtopoint", GameScript::MoveToPoint,AF_BLOCKING}, //until we know better
	{"runtopointnorecticle", GameScript::MoveToPoint,AF_BLOCKING},//until we know better
	{"runtosavedlocation", GameScript::MoveToSavedLocation,AF_BLOCKING},//
	{"savelocation", GameScript::SaveLocation,0},
	{"saveplace", GameScript::SaveLocation,0},
	{"saveobjectlocation", GameScript::SaveObjectLocation,0},
	{"screenshake", GameScript::ScreenShake,AF_BLOCKING},
	{"setanimstate", GameScript::SetAnimState,AF_BLOCKING},
	{"setapparentnamestrref", GameScript::SetApparentName,0},
	{"setareaflags", GameScript::SetAreaFlags,0},
	{"setarearestflag", GameScript::SetAreaRestFlag,0},
	{"setbeeninpartyflags", GameScript::SetBeenInPartyFlags,0},
	{"setcorpseenabled", GameScript::SetCorpseEnabled,0},
	{"setcreatureareaflags", GameScript::SetCreatureAreaFlags,0},
	{"setdialog", GameScript::SetDialogue,AF_BLOCKING},
	{"setdialogue", GameScript::SetDialogue,AF_BLOCKING},
	{"setdialoguerange", GameScript::SetDialogueRange,0},
	{"setdoorlocked", GameScript::SetDoorLocked,AF_BLOCKING},
	{"setextendednight", GameScript::SetExtendedNight,0},
	{"setfaction", GameScript::SetFaction,0},
	{"setgabber", GameScript::SetGabber,0},
	{"setglobal", GameScript::SetGlobal,AF_MERGESTRINGS},
	{"setglobaltimer", GameScript::SetGlobalTimer,AF_MERGESTRINGS},
	{"setglobaltint", GameScript::SetGlobalTint,0},
	{"sethomelocation", GameScript::SetHomeLocation,0},
	{"sethp", GameScript::SetHP,0},
	{"setinternal", GameScript::SetInternal,0},
	{"setleavepartydialogfile", GameScript::SetLeavePartyDialogFile,0},
	{"setmasterarea", GameScript::SetMasterArea,0},
	{"setmoraleai", GameScript::SetMoraleAI,0},
	{"setmusic", GameScript::SetMusic,0},
	{"setname", GameScript::SetApparentName,0},
	{"setnamelessclass", GameScript::SetNamelessClass,0},
	{"setnamelessdisguise", GameScript::SetNamelessDisguise,0},
	{"setnumtimestalkedto", GameScript::SetNumTimesTalkedTo,0},
	{"setplayersound", GameScript::SetPlayerSound,0},
	{"setquestdone", GameScript::SetQuestDone,0},
	{"setregularnamestrref", GameScript::SetRegularName,0},
	{"setrestencounterchance", GameScript::SetRestEncounterChance,0},
	{"setrestencounterprobabilityday", GameScript::SetRestEncounterProbabilityDay,0},
	{"setrestencounterprobabilitynight", GameScript::SetRestEncounterProbabilityNight,0},
	{"setsavedlocation", GameScript::SaveObjectLocation, 0},
	{"setsavedlocationpoint", GameScript::SaveLocation, 0},
	{"setscriptname", GameScript::SetScriptName,0},
	{"setsequence", GameScript::PlaySequence,0},
	{"setteam", GameScript::SetTeam,0},
	{"settextcolor", GameScript::SetTextColor,0},
	{"settoken", GameScript::SetToken,0},
	{"settokenglobal", GameScript::SetTokenGlobal,AF_MERGESTRINGS},
	{"setvisualrange", GameScript::SetVisualRange,0},
	{"sg", GameScript::SG,0},
	{"shout", GameScript::Shout,0},
	{"sinisterpoof", GameScript::CreateVisualEffect,0},
	{"smallwait", GameScript::SmallWait,AF_BLOCKING},
	{"smallwaitrandom", GameScript::SmallWaitRandom,AF_BLOCKING},
	{"soundactivate", GameScript::SoundActivate,0},
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
	{"startmusic", GameScript::StartMusic,0},
	{"startsong", GameScript::StartSong,0},
	{"startstore", GameScript::StartStore,0},
	{"staticpalette", GameScript::StaticPalette,0},
	{"staticstart", GameScript::StaticStart,0},
	{"staticstop", GameScript::StaticStop,0},
	{"stickysinisterpoof", GameScript::CreateVisualEffectObject,0},
	{"stopmoving", GameScript::StopMoving,0},
	{"storepartylocations", GameScript::StorePartyLocation,0},
	{"stuffglobalrandom", GameScript::SetGlobalRandom,0},
	{"swing", GameScript::Swing,0},
	{"swingonce", GameScript::SwingOnce,0},
	{"takeitemlist", GameScript::TakeItemList,0},
	{"takeitemreplace", GameScript::TakeItemReplace,0},
	{"takepartygold", GameScript::TakePartyGold,0},
	{"takepartyitem", GameScript::TakePartyItem,0},
	{"takepartyitemall", GameScript::TakePartyItemAll,0},
	{"takepartyitemnum", GameScript::TakePartyItemNum,0},
	{"teleportparty", GameScript::TeleportParty,0}, 
	{"textscreen", GameScript::TextScreen,0},
	{"tomsstringdisplayer", GameScript::DisplayMessage,0},
	{"triggeractivation", GameScript::TriggerActivation,0},
	{"undoexplore", GameScript::UndoExplore,0},
	{"unhidegui", GameScript::UnhideGUI,0},
	{"unloadarea", GameScript::UnloadArea,0},
	{"unlock", GameScript::Unlock,0},
	{"unlockscroll", GameScript::UnlockScroll,0},
	{"unmakeglobal", GameScript::UnMakeGlobal,0}, //this is a GemRB extension
	{"usecontainer", GameScript::UseContainer,AF_BLOCKING},
	{"vequip",GameScript::SetArmourLevel,0},
	{"verbalconstant", GameScript::VerbalConstant,0},
	{"verbalconstanthead", GameScript::VerbalConstantHead,0},
	{"wait", GameScript::Wait, AF_BLOCKING},
	{"waitanimation", GameScript::WaitAnimation,AF_BLOCKING},
	{"waitrandom", GameScript::WaitRandom, AF_BLOCKING}, { NULL,NULL,0},
};

//Make this an ordered list, so we could use bsearch!
static ObjectLink objectnames[] = {
	{"bestac", GameScript::BestAC},
	{"eighthnearest", GameScript::EighthNearest},
	{"eighthnearestenemyof", GameScript::EighthNearestEnemyOf},
	{"eighthnearestenemyoftype", GameScript::EighthNearestEnemyOfType},
	{"eigthnearestenemyof", GameScript::EighthNearestEnemyOf}, //typo in iwd
	{"farthest", GameScript::Farthest},
	{"fifthnearest", GameScript::FifthNearest},
	{"fifthnearestenemyof", GameScript::FifthNearestEnemyOf},
	{"fifthnearestenemyoftype", GameScript::FifthNearestEnemyOfType},
	{"fourthnearest", GameScript::FourthNearest},
	{"fourthnearestenemyof", GameScript::FourthNearestEnemyOf},
	{"fourthnearestenemyoftype", GameScript::FourthNearestEnemyOfType},
	{"lastattackerof", GameScript::LastAttackerOf},
	{"lastcommandedby", GameScript::LastCommandedBy},
	{"lastheardby", GameScript::LastHeardBy},
	{"lasthelp", GameScript::LastHelp},
	{"lasthitter", GameScript::LastHitter},
	{"lastmarkedobject", GameScript::LastSeenBy},
	{"lastseenby", GameScript::LastSeenBy},
	{"lastsummonerof", GameScript::LastSummonerOf},
	{"lasttalkedtoby", GameScript::LastTalkedToBy},
	{"lasttargetedby", GameScript::LastTargetedBy},
	{"lasttrigger", GameScript::LastTrigger},
	{"leastdamagedof", GameScript::LeastDamagedOf},
	{"mostdamagedof", GameScript::MostDamagedOf},
	{"myself", GameScript::Myself},
	{"nearest", GameScript::Nearest}, //actually this seems broken in IE and resolve as Myself
	{"nearestenemyof", GameScript::NearestEnemyOf},
	{"nearestenemyoftype", GameScript::NearestEnemyOfType},
	{"nearestpc", GameScript::NearestPC},
	{"ninthnearest", GameScript::NinthNearest},
	{"ninthnearestenemyof", GameScript::NinthNearestEnemyOf},
	{"ninthnearestenemyoftype", GameScript::NinthNearestEnemyOfType},
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
	{"secondnearestenemyoftype", GameScript::SecondNearestEnemyOfType},
	{"selectedcharacter", GameScript::SelectedCharacter},
	{"seventhnearest", GameScript::SeventhNearest},
	{"seventhnearestenemyof", GameScript::SeventhNearestEnemyOf},
	{"seventhnearestenemyoftype", GameScript::SeventhNearestEnemyOfType},
	{"sixthnearest", GameScript::SixthNearest},
	{"sixthnearestenemyof", GameScript::SixthNearestEnemyOf},
	{"sixthnearestenemyoftype", GameScript::SixthNearestEnemyOfType},
	{"strongestof", GameScript::StrongestOf},
	{"strongestofmale", GameScript::StrongestOfMale},
	{"tenthnearest", GameScript::TenthNearest},
	{"tenthnearestenemyof", GameScript::TenthNearestEnemyOf},
	{"tenthnearestenemyoftype", GameScript::TenthNearestEnemyOfType},
	{"thirdnearest", GameScript::ThirdNearest},
	{"thirdnearestenemyof", GameScript::ThirdNearestEnemyOf},
	{"thirdnearestenemyoftype", GameScript::ThirdNearestEnemyOfType},
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
			if (!triggernames[i].Name[len]) {
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
			if (!actionnames[i].Name[len]) {
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
			if (!objectnames[i].Name[len]) {
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
	int len = (int)strlen( idsname );
	for (int i = 0; idsnames[i].Name; i++) {
		if (!strnicmp( idsnames[i].Name, idsname, len )) {
			return idsnames + i;
		}
	}
	printf( "Warning: Couldn't assign ids target: %.*s\n", len, idsname );
	return NULL;
}

void SetScriptDebugMode(int arg)
{
	InDebug=arg;
}

static void GoNearAndRetry(Scriptable *Sender, Scriptable *target)
{
	Sender->AddActionInFront( Sender->CurrentAction );
	char Tmp[256];
	sprintf( Tmp, "MoveToPoint([%hd.%hd])", target->Pos.x, target->Pos.y );
	Sender->AddActionInFront( GameScript::GenerateAction( Tmp, true ) );
}

static void GoNearAndRetry(Scriptable *Sender, Point &p)
{
	Sender->AddActionInFront( Sender->CurrentAction );
	char Tmp[256];
	sprintf( Tmp, "MoveToPoint([%hd.%hd])", p.x, p.y );
	Sender->AddActionInFront( GameScript::GenerateAction( Tmp, true ) );
}

#define MEMCPY(a,b) memcpy((a),(b),sizeof(a) )

static Object *ObjectCopy(Object *object)
{
	if (!object) return NULL;
	Object *newObject = new Object();
	MEMCPY( newObject->objectFields, object->objectFields );
	MEMCPY( newObject->objectFilters, object->objectFilters );
	MEMCPY( newObject->objectRect, object->objectRect );
	MEMCPY( newObject->objectName, object->objectName );
	return newObject;
}

static Action *ParamCopy(Action *parameters)
{
	Action *newAction = new Action(true);
	newAction->actionID = parameters->actionID;
	newAction->int0Parameter = parameters->int0Parameter;
	newAction->int1Parameter = parameters->int1Parameter;
	newAction->int2Parameter = parameters->int2Parameter;
	newAction->pointParameter = parameters->pointParameter;
	MEMCPY( newAction->string0Parameter, parameters->string0Parameter );
	MEMCPY( newAction->string1Parameter, parameters->string1Parameter );
	for (int c=0;c<3;c++) {
		newAction->objects[c]= ObjectCopy( parameters->objects[c] );
	}
	return newAction;
}

static void HandleBitMod(ieDword &value1, ieDword value2, int opcode)
{
	switch(opcode) {
		case BM_AND:
			value1 = ( value1& value2 );
			break;
		case BM_OR:
			value1 = ( value1| value2 );
			break;
		case BM_XOR:
			value1 = ( value1^ value2 );
			break;
		case BM_NAND: //this is a GemRB extension
			value1 = ( value1& ~value2 );
			break;
		case BM_SET: //this is a GemRB extension
			value1 = value2;
			break;
	}
}

static void FreeSrc(SrcVector *poi, ieResRef key)
{
	int res = SrcCache.DecRef((void *) poi, key, true);
	if (res<0) {
		printMessage( "GameScript", "Corrupted Src cache encountered (reference count went below zero), ", LIGHT_RED );
		printf( "Src name is: %.8s\n", key);
		abort();
	}
	if (!res) {
		delete poi;	
	}
}

static SrcVector *LoadSrc(ieResRef resname)
{
	SrcVector *src = (SrcVector *) SrcCache.GetResource(resname);
	if (src) {
		return src;
	}
	DataStream* str = core->GetResourceMgr()->GetResource( resname, IE_SRC_CLASS_ID );
	if ( !str) {
		return NULL;
	}
	ieDword size=0;
	str->ReadDword(&size);
	src = new SrcVector(size);
	SrcCache.SetAt( resname, (void *) src );
	while (size--) {
		ieDword tmp;
		str->ReadDword(&tmp);
		src->at(size)=tmp;
		str->ReadDword(&tmp);
	}
	delete ( str );
	return src;
}

static void InitScriptTables()
{
	//initializing the happiness table
	int hptable = core->LoadTable( "happy" );
	TableMgr *tab = core->GetTable( hptable );
	for (int alignment=0;alignment<3;alignment++) {
		for (int reputation=0;reputation<20;reputation++) {
			happiness[alignment][reputation]=strtol(tab->QueryField(reputation,alignment), NULL, 0);
		}
	}
	core->DelTable( hptable );

	//
}

/********************** Targets **********************************/

int Targets::Count() const
{
	return (int)objects.size();
}
 
targettype *Targets::RemoveTargetAt(targetlist::iterator &m)
{
	m=objects.erase(m);
	if (m!=objects.end() ) {
		return &(*m);
	}
	return NULL;
}

targettype *Targets::GetNextTarget(targetlist::iterator &m)
{
	m++;
	if (m!=objects.end() ) {
		return &(*m);
	}
	return NULL;
}

targettype *Targets::GetLastTarget()
{
	targetlist::const_iterator m = objects.end();
	if (m!=objects.begin() ) {
		return (targettype *) &(*(--m));
	}
	return NULL;
}

targettype *Targets::GetFirstTarget(targetlist::iterator &m)
{
	m = objects.begin();
	if (m!=objects.end() ) {
		return &(*m);
	}
	return NULL;
}

Actor *Targets::GetTarget(unsigned int index)
{
	targetlist::iterator m = objects.begin();
	while(m!=objects.end() ) {
		if (!index) {
			return (*m).actor;
		}
		index--;
	}
	return NULL;
}

void Targets::AddTarget(Actor* actor, unsigned int distance)
{
	//i don't know if unselectable actors are targetable by script
	//if yes, then remove GA_SELECT
	if (actor && actor->ValidTarget(GA_SELECT|GA_NO_DEAD) ) {
		targettype Target = {actor, distance};
		targetlist::iterator m;
		for (m = objects.begin(); m != objects.end(); ++m) {
			if ( (*m).distance>distance) {
				objects.insert( m, Target);
				return;
			}
		}
		objects.push_back( Target );
	}
}

void Targets::Clear()
{
	objects.clear();
}

/********************** GameScript *******************************/
GameScript::GameScript(ieResRef ResRef, unsigned char ScriptType,
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
		InitScriptTables();
		int tT = core->LoadSymbol( "TRIGGER" );
		int aT = core->LoadSymbol( "ACTION" );
		int oT = core->LoadSymbol( "OBJECT" );
		int iT = core->LoadTable( "SCRIPT" );
		if (tT < 0 || aT < 0 || oT < 0 || iT < 0) {
			printMessage( "GameScript","A critical scripting file is missing!\n",LIGHT_RED );
			abort();
		}
		triggersTable = core->GetSymbol( tT );
		actionsTable = core->GetSymbol( aT );
		objectsTable = core->GetSymbol( oT );
		TableMgr* objNameTable = core->GetTable( iT );
		if (!triggersTable || !actionsTable || !objectsTable || !objNameTable) {
			printMessage( "GameScript","A critical scripting file is damaged!\n",LIGHT_RED );
			abort();
		}

		int i;

		/* Loading Script Configuration Parameters */

		ObjectIDSCount = atoi( objNameTable->QueryField() );
		if (ObjectIDSCount<0 || ObjectIDSCount>MAX_OBJECT_FIELDS) {
			printMessage("GameScript","The IDS Count shouldn't be more than 10!\n",LIGHT_RED);
			abort();
		}
		
		ObjectIDSTableNames = (ieResRef *) malloc( sizeof(ieResRef) * ObjectIDSCount );
		for (i = 0; i < ObjectIDSCount; i++) {
			char *idsname;
			idsname=objNameTable->QueryField( 0, i + 1 );
			IDSLink *poi=FindIdentifier( idsname );
			if (poi==NULL) {
				idtargets[i]=NULL;
			}
			else {
				idtargets[i]=poi->Function;
			}
			strnuprcpy(ObjectIDSTableNames[i], idsname, 8 );
			//ObjectIDSTableNames.push_back( idsname );
		}
		MaxObjectNesting = atoi( objNameTable->QueryField( 1 ) );
		if (MaxObjectNesting<0 || MaxObjectNesting>MAX_NESTING) {
			printMessage("GameScript","The Object Nesting Count shouldn't be more than 5!\n", LIGHT_RED);
			abort();
		}
		HasAdditionalRect = ( atoi( objNameTable->QueryField( 2 ) ) != 0 );
		ExtraParametersCount = atoi( objNameTable->QueryField( 3 ) );
		ObjectFieldsCount = ObjectIDSCount - ExtraParametersCount;

		/* Initializing the Script Engine */

		memset( triggers, 0, sizeof( triggers ) );
		memset( triggerflags, 0, sizeof( triggerflags ) );
		memset( actions, 0, sizeof( actions ) );
		memset( actionflags, 0, sizeof( actionflags ) );
		memset( objects, 0, sizeof( objects ) );

		int j;

		j = triggersTable->GetSize();
		while (j--) {
			i = triggersTable->GetValueIndex( j );
			//maybe we should watch for this bit?
			//bool triggerflag = i & 0x4000;
			i &= 0x3fff;
			if (triggers[i]) continue; //we already found an alternative
			TriggerLink* poi = FindTrigger(triggersTable->GetStringIndex( j ) );
			if (poi == NULL) {
				triggers[i] = NULL;
				triggerflags[i] = 0;
			}
			else {
				triggers[i] = poi->Function;
				triggerflags[i] = poi->Flags;
			}
		}

		j = actionsTable->GetSize();
		while (j--) {
			i = actionsTable->GetValueIndex( j );
			if (actions[i]) continue; //we already found an alternative
			ActionLink* poi = FindAction( actionsTable->GetStringIndex( j ) );
			if (poi == NULL) {
				actions[i] = NULL;
				actionflags[i] = 0;
			} else {
				actions[i] = poi->Function;
				actionflags[i] = poi->Flags;
			}
		}
		j = objectsTable->GetSize();
		while (j--) {
			i = objectsTable->GetValueIndex( j );
			if (objects[i]) continue;
			ObjectLink* poi = FindObject( objectsTable->GetStringIndex( j ) );
			if (poi == NULL) {
				objects[i] = NULL;
			} else {
				objects[i] = poi->Function;
			}
		}
		initialized = 2;
	}
	continueExecution = false;
	script = CacheScript( ResRef );
	MySelf = NULL;
	scriptRunDelay = 1000;
	scriptType = ScriptType;
	lastRunTime = 0;
	endReached = false;
	strcpy( Name, ResRef );
}

GameScript::~GameScript(void)
{
	if (freeLocals) {
		if (locals) {
			delete( locals );
		}
	}
	if (script) {
		//set 3. parameter to true if you want instant free
		//and possible death
		if (InDebug&1) {
			printf("One instance of %s is dropped from %d.\n", Name, BcsCache.RefCount(Name) );
		}
		int res = BcsCache.DecRef(script, Name, true);
		
		if (res<0) {
			printMessage( "GameScript", "Corrupted Script cache encountered (reference count went below zero), ", LIGHT_RED );
			printf( "Script name is: %.8s\n", Name);
			abort();
		}
		if (!res) {
			printf("Freeing script %s because its refcount has reached 0.\n", Name);
			script->Release();
		}
		script = NULL;
	}
}

Script* GameScript::CacheScript(ieResRef ResRef)
{
	char line[10];

	Script *newScript = (Script *) BcsCache.GetResource(ResRef);
	if ( newScript ) {
		if (InDebug&1) {
			printf("Caching %s for the %d. time\n", ResRef, BcsCache.RefCount(ResRef) );
		}
		return newScript;
}

	DataStream* stream = core->GetResourceMgr()->GetResource( ResRef,
		IE_BCS_CLASS_ID );
	if (!stream) {
		return NULL;
	}
	stream->ReadLine( line, 10 );
	if (strncmp( line, "SC", 2 ) != 0) {
		printMessage( "GameScript","Not a Compiled Script file\n", YELLOW );
		delete( stream );
		return NULL;
	}
	newScript = new Script( );
	BcsCache.SetAt( ResRef, (void *) newScript );
	if (InDebug&1) {
		printf("Caching %s for the %d. time\n", ResRef, BcsCache.RefCount(ResRef) );
	}
	
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

void GameScript::SetVariable(Scriptable* Sender, const char* VarName,
	const char* Context, ieDword value)
{
	char newVarName[8+33];

	if (InDebug&4) {
		printf( "Setting variable(\"%s%s\", %d)\n", Context,
			VarName, value );
	}
	strncpy( newVarName, Context, 6 );
	newVarName[6]=0;
	if (strnicmp( newVarName, "MYAREA", 6 ) == 0) {
		Sender->GetCurrentArea()->locals->SetAt( VarName, value );
		return;
	}
	if (strnicmp( newVarName, "LOCALS", 6 ) == 0) {
		Sender->locals->SetAt( VarName, value );
		return;
	}
	Game *game = core->GetGame();
	if (!strnicmp(newVarName,"KAPUTZ",6) && core->HasFeature(GF_HAS_KAPUTZ) ) {
		game->kaputz->SetAt( VarName, value );
		return;
	}

	if (strnicmp(newVarName,"GLOBAL",6) ) {
		Map *map=game->GetMap(game->FindMap(newVarName));
		if (map) {
			map->vars->SetAt( VarName, value);
		}
		else if (InDebug&4) {
			printMessage("GameScript"," ",YELLOW);
			printf("Invalid variable %s %s in checkvariable\n",Context, VarName);
		}
	}
	else {
		game->globals->SetAt( VarName, ( ieDword ) value );
	}
}

void GameScript::SetVariable(Scriptable* Sender, const char* VarName, ieDword value)
{
	char newVarName[8];

	if (InDebug&4) {
		printf( "Setting variable(\"%s\", %d)\n", VarName, value );
	}
	strncpy( newVarName, VarName, 6 );
	newVarName[6]=0;
	if (strnicmp( newVarName, "MYAREA", 6 ) == 0) {
		Sender->GetCurrentArea()->locals->SetAt( &VarName[6], value );
		return;
	}
	if (strnicmp( newVarName, "LOCALS", 6 ) == 0) {
		Sender->locals->SetAt( &VarName[6], value );
		return;
	}
	Game *game = core->GetGame();
	if (!strnicmp(newVarName,"KAPUTZ",6) && core->HasFeature(GF_HAS_KAPUTZ) ) {
		game->kaputz->SetAt( &VarName[6], value );
		return;
	}
	if (strnicmp(newVarName,"GLOBAL",6) ) {
		Map *map=game->GetMap(game->FindMap(newVarName));
		if (map) {
			map->vars->SetAt( &VarName[6], value);
		}
		else if (InDebug&4) {
			printMessage("GameScript"," ",YELLOW);
			printf("Invalid variable %s in setvariable\n",VarName);
		}
	}
	else {
		game->globals->SetAt( &VarName[6], ( ieDword ) value );
	}
}

ieDword GameScript::CheckVariable(Scriptable* Sender, const char* VarName)
{
	char newVarName[8];
	ieDword value = 0;

	strncpy( newVarName, VarName, 6 );
	newVarName[6]=0;
	if (strnicmp( newVarName, "MYAREA", 6 ) == 0) {
		Sender->GetCurrentArea()->locals->SetAt( VarName, value );
		if (InDebug&4) {
			printf("CheckVariable %s: %d\n",VarName, value);
		}
		return value;
	}
	if (strnicmp( newVarName, "LOCALS", 6 ) == 0) {
		Sender->locals->Lookup( &VarName[6], value );
		if (InDebug&4) {
			printf("CheckVariable %s: %d\n",VarName, value);
		}
		return value;
	}
	Game *game = core->GetGame();
	if (!strnicmp(newVarName,"KAPUTZ",6) && core->HasFeature(GF_HAS_KAPUTZ) ) {
		game->kaputz->Lookup( &VarName[6], value );
		if (InDebug&4) {
			printf("CheckVariable %s: %d\n",VarName, value);
		}
		return value;
	}
	if (strnicmp(newVarName,"GLOBAL",6) ) {
		Map *map=game->GetMap(game->FindMap(newVarName));
		if (map) {
			map->vars->Lookup( &VarName[6], value);
		}
		else if (InDebug&4) {
			printMessage("GameScript"," ",YELLOW);
			printf("Invalid variable %s in checkvariable\n",VarName);
		}
	}
	else {
		game->globals->Lookup( &VarName[6], value );
	}
	if (InDebug&4) {
		printf("CheckVariable %s: %d\n",VarName, value);
	}
	return value;
}

ieDword GameScript::CheckVariable(Scriptable* Sender, const char* VarName, const char* Context)
{
	char newVarName[8];
	ieDword value = 0;

	strncpy(newVarName, Context, 6);
	newVarName[6]=0;
	if (strnicmp( newVarName, "MYAREA", 6 ) == 0) {
		Sender->GetCurrentArea()->locals->SetAt( VarName, value );
		if (InDebug&4) {
			printf("CheckVariable %s%s: %d\n",Context, VarName, value);
		}
		return value;
	}
	if (strnicmp( newVarName, "LOCALS", 6 ) == 0) {
		Sender->locals->Lookup( VarName, value );
		if (InDebug&4) {
			printf("CheckVariable %s%s: %d\n",Context, VarName, value);
		}
		return value;
	}
	Game *game = core->GetGame();
	if (!strnicmp(newVarName,"KAPUTZ",6) && core->HasFeature(GF_HAS_KAPUTZ) ) {
		game->kaputz->Lookup( VarName, value );
		if (InDebug&4) {
			printf("CheckVariable %s%s: %d\n",Context, VarName, value);
		}
		return value;
	}
	if (strnicmp(newVarName,"GLOBAL",6) ) {
		Map *map=game->GetMap(game->FindMap(newVarName));
		if (map) {
			map->vars->Lookup( VarName, value);
		}
		else if (InDebug&4) {
			printMessage("GameScript"," ",YELLOW);
			printf("Invalid variable %s %s in checkvariable\n",Context, VarName);
		}
	} else {
		game->globals->Lookup( VarName, value );
	}
	if (InDebug&4) {
		printf("CheckVariable %s%s: %d\n",Context, VarName, value);
	}
	return value;
}

void GameScript::Update()
{
	if (!MySelf || !(MySelf->Active&SCR_ACTIVE) ) {
		return;
	}
	ieDword thisTime;
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
	if (!MySelf || !(MySelf->Active&SCR_ACTIVE) ) {
		return;
	}
	ieDword thisTime;
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
	rE->weight = (unsigned char)strtoul(line,&poi,10);
	std::vector< Action*> aCv;
	if (strncmp(poi,"AC",2)==0) while (true) {
		Action* aC = new Action(false);
		count = stream->ReadLine( line, 1024 );
		aC->actionID = (unsigned short)strtoul(line, NULL,10);
		if (aC->actionID>=MAX_ACTIONS) {
			aC->actionID=0;
			printMessage("GameScript","Invalid script action ID!",LIGHT_RED);
		}
		for (int i = 0; i < 3; i++) {
			stream->ReadLine( line, 1024 );
			Object* oB = DecodeObject( line );
			aC->objects[i] = oB;
			if (i != 2)
				stream->ReadLine( line, 1024 );
		}
		stream->ReadLine( line, 1024 );
		sscanf( line, "%d %hd %hd %d %d\"%[^\"]\" \"%[^\"]\" AC",
			&aC->int0Parameter, &aC->pointParameter.x, &aC->pointParameter.y,
			&aC->int1Parameter, &aC->int2Parameter, aC->string0Parameter,
			aC->string1Parameter );
		strupr(aC->string0Parameter);
		strupr(aC->string1Parameter);
		aCv.push_back( aC );
		stream->ReadLine( line, 1024 );
		if (strncmp( line, "RE", 2 ) == 0)
			break;
	}
	free( line );
	rE->actionsCount = ( unsigned char ) aCv.size();
	rE->actions = new Action* [rE->actionsCount];
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
		sscanf( line, "%hu %d %d %d %d [%hd,%hd] \"%[^\"]\" \"%[^\"]\" OB",
			&tR->triggerID, &tR->int0Parameter, &tR->flags,
			&tR->int1Parameter, &tR->int2Parameter, &tR->pointParameter.x,
			&tR->pointParameter.y, tR->string0Parameter, tR->string1Parameter );
	} else {
		sscanf( line, "%hu %d %d %d %d \"%[^\"]\" \"%[^\"]\" OB",
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
	int i;

	Object* oB = new Object();
	for (i = 0; i < ObjectFieldsCount; i++) {
		oB->objectFields[i] = ParseInt( line );
	}
	for (i = 0; i < MaxObjectNesting; i++) {
		oB->objectFilters[i] = ParseInt( line );
	}
	if (HasAdditionalRect) {
		line++; //Skip [
		for (i = 0; i < 4; i++) {
			oB->objectRect[i] = ParseInt( line );
		}
		line++; //Skip ] (not really... it skips a ' ' since the ] was skipped by the ParseInt function
	}
	line++; //Skip "
	ParseString( line, oB->objectName );
	line++; //Skip " (the same as above)
	for (i = 0; i < ExtraParametersCount; i++) {
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
				printMessage( "GameScript","Unfinished OR block encountered!\n",YELLOW );
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
		printMessage( "GameScript","Unfinished OR block encountered!\n",YELLOW );
	}
	return 1;
}

/* this may return more than a boolean, in case of Or(x) */
int GameScript::EvaluateTrigger(Scriptable* Sender, Trigger* trigger)
{
	if (!trigger) {
		printMessage( "GameScript","Trigger evaluation fails due to NULL trigger.\n",YELLOW );
		return 0;
	}
	TriggerFunction func = triggers[trigger->triggerID];
	const char *tmpstr=triggersTable->GetValue(trigger->triggerID);
	if (!tmpstr) {
		tmpstr=triggersTable->GetValue(trigger->triggerID|0x4000);
	}
	if (!func) {
		triggers[trigger->triggerID] = False;
		//hope this is enough, snprintf will prevent buffer overflow
		char Tmp[256]; 
		snprintf(Tmp,sizeof(Tmp),"Unhandled trigger code: 0x%04x %s\n",
			trigger->triggerID, tmpstr );
		printMessage( "GameScript",Tmp,YELLOW);
		return 0;
	}
	if (InDebug&16) {
		printf( "[GameScript]: Executing trigger code: 0x%04x %s\n",
				trigger->triggerID, tmpstr );
	}
	int ret = func( Sender, trigger );
	if (trigger->flags & 1) {
		return !ret;
	}
	return ret;
}

int GameScript::ExecuteResponseSet(Scriptable* Sender, ResponseSet* rS)
{
	int i;

	switch(rS->responsesCount) {
		case 0:
			return 0;
		case 1:
			return ExecuteResponse( Sender, rS->responses[0] );
	}
	/*default*/
	int randWeight;
	int maxWeight = 0;

	for (i = 0; i < rS->responsesCount; i++) {
		maxWeight+=rS->responses[i]->weight;
	}
	if (maxWeight) {
		randWeight = rand() % maxWeight;
	}
	else {
		randWeight = 0;
	}

	for (i = 0; i < rS->responsesCount; i++) {
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
	int ret = 0; // continue or not
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
	if (InDebug&8) {
		printf("Sender: %s\n",Sender->GetScriptName() );
	}
	ActionFunction func = actions[aC->actionID];
	if (func) {
		Scriptable* scr = GetActorFromObject( Sender, aC->objects[0]);
		if (scr && scr!=Sender) {
			//this is an Action* Override
			scr->AddAction( Sender->CurrentAction );
			Sender->CurrentAction = NULL;
			//maybe we should always release here???
			if (!(actionflags[aC->actionID] & AF_INSTANT) ) {
				aC->Release();
			}
			return;
		}
		else {
			if (InDebug&8) {
				printf( "[GameScript]: Executing action code: %d %s\n", aC->actionID , actionsTable->GetValue(aC->actionID) );
			}
			//turning off interruptable flag
			//uninterruptable actions will set it back
			if (Sender->Type==ST_ACTOR) {
				Sender->Active|=SCR_ACTIVE;
				((Actor *)Sender)->InternalFlags&=~IF_NOINT;
			}
			func( Sender, aC );
		}
	}
	else {
		actions[aC->actionID] = NoActionAtAll;
		char Tmp[256]; 
		snprintf(Tmp, sizeof(Tmp), "Unhandled action code: %d %s\n", aC->actionID , actionsTable->GetValue(aC->actionID) );
		printMessage("GameScript", Tmp, YELLOW);
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
Targets* GameScript::EvaluateObject(Scriptable* Sender, Object* oC)
{
	Map *map=Sender->GetCurrentArea();
	Targets *tgts=NULL;

	if (oC->objectName[0]) {
		//We want the object by its name... (doors/triggers don't play here!)
		Actor* aC = map->GetActor( oC->objectName );
		if (!aC) {
			return tgts;
		}
		//Ok :) we now have our Object. Let's create a Target struct and add the object to it
		tgts = new Targets();
		tgts->AddTarget( aC, 0 );
		//return here because object name/IDS targeting are mutually exclusive
		return tgts;
	}
	//else branch, IDS targeting
	for (int j = 0; j < ObjectIDSCount; j++) {
		if (!oC->objectFields[j]) {
			continue;
		}
		IDSFunction func = idtargets[j];
		if (!func) {
			printf("Unimplemented IDS targeting opcode!\n");
			continue;
		}
		if (tgts) {
			//we already got a subset of actors
			int i = tgts->Count();
			/*premature end, filtered everything*/
			if (!i) {
				break; //leaving the loop
			}
			targetlist::iterator m;
			targettype *t = tgts->GetFirstTarget(m);
			while (t) {
				if (!func(t->actor, oC->objectFields[j] ) ) {
					t = tgts->RemoveTargetAt(m);
				} else {
					t = tgts->GetNextTarget(m);
				}
			}
		}
		else {
			//we need to get a subset of actors from the large array
			//if this gets slow, we will need some index tables
			int i = map->GetActorCount();
			tgts = new Targets();
			while (i--) {
				Actor *ac=map->GetActor(i);
				int dist = Distance(Sender->Pos, ac->Pos);
				if (ac && func(ac, oC->objectFields[j]) ) {
					tgts->AddTarget(ac, dist);
				}
			}
		}
	}
	return tgts;
}

bool GameScript::MatchActor(Scriptable *Sender, Actor* actor, Object* oC)
{
	if (!actor) {
		return false;
	}
	if (oC->objectName[0]) {
		return (stricmp( actor->GetScriptName(), oC->objectName ) == 0);
	}
	Targets *tgts = new Targets();
	for (int i = 0; i < MaxObjectNesting; i++) {
		int filterid = oC->objectFilters[i];
		if (!filterid) {
			break;
		}
		ObjectFunction func = objects[filterid];
		if (func) {
			tgts = func( Sender, tgts);
		}
		else {
			printf("[GameScript]: Unknown object filter: %d %s\n",filterid, objectsTable->GetValue(filterid) );
		}
		if (!tgts->Count()) {
			delete tgts;
			return false;
		}
	}
	bool ret = false;

	if (tgts) {
		targetlist::iterator m;
		targettype *tt = tgts->GetFirstTarget(m);
		while (tt) {
			if (tt->actor == actor) {
				ret = true;
			}
			tt = tgts->GetNextTarget(m);
		}
	}
	delete tgts;
	return ret;
}

int GameScript::GetObjectCount(Scriptable* Sender, Object* oC)
{
	if (!oC) {
		return 0;
	}
	Targets* tgts = EvaluateObject(Sender, oC);
	int count = tgts->Count();
	delete tgts;
	return count;
}

Scriptable* GameScript::GetActorFromObject(Scriptable* Sender, Object* oC)
{
	if (!oC) {
		return NULL;
	}
	Targets* tgts = EvaluateObject(Sender, oC);
	if (!tgts && oC->objectName[0]) {
		//It was not an actor... maybe it is a door?
		Scriptable * aC = Sender->GetCurrentArea()->TMap->GetDoor( oC->objectName );
		if (aC) {
			return aC;
		}
		//No... it was not a door... maybe an InfoPoint?
		aC = Sender->GetCurrentArea()->TMap->GetInfoPoint( oC->objectName );
		if (aC) {
			return aC;
		}

		//No... it was not an infopoint... maybe a Container?
		aC = Sender->GetCurrentArea()->TMap->GetContainer( oC->objectName );
		if (aC) {
			return aC;
		}
		return NULL;
	}
	//now lets do the object filter stuff, we create Targets because
	//it is possible to start from blank sheets using endpoint filters
	//like (Myself, Protagonist etc)
	if (!tgts) {
		tgts = new Targets();
	}
	for (int i = 0; i < MaxObjectNesting; i++) {
		int filterid = oC->objectFilters[i];
		if (!filterid) {
			break;
		}
		ObjectFunction func = objects[filterid];
		if (func) {
			tgts = func( Sender, tgts);
		}
		else {
			printf("[GameScript]: Unknown object filter: %d %s\n",filterid, objectsTable->GetValue(filterid) );
		}
		if (!tgts->Count()) {
			delete tgts;
			return NULL;
		}
	}
	if (tgts) {
		Scriptable *object;
		object= (Scriptable *) tgts->GetTarget(0);
		delete tgts;
		return object;
	}
	return NULL;
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

//This must return integer because Or(3) returns 3
int GameScript::EvaluateString(Scriptable* Sender, char* String)
{
	if (String[0] == 0) {
		return 0;
	}
	Trigger* tri = GenerateTrigger( String );
	int ret = EvaluateTrigger( Sender, tri );
	tri->Release();
	return ret;
}

//this function returns a value, symbol could be a numeric string or
//a symbol from idsname
static int GetIdsValue(const char *&symbol, const char *idsname)
{
	int idsfile=core->LoadSymbol(idsname);
	SymbolMgr *valHook = core->GetSymbol(idsfile);
	if (!valHook) {
		//FIXME:missing ids file!!!
		if (InDebug&16) {
			char Tmp[256];

			sprintf(Tmp,"Missing IDS file %s for symbol %s!\n",idsname, symbol);
			printMessage("GameScript",Tmp,LIGHT_RED);
		}
		return -1;
	}
	char *newsymbol;
	int value=strtol(symbol, &newsymbol, 0);
	if (symbol!=newsymbol) {
		symbol=newsymbol;
		return value;
	}
	char symbolname[64];
	int x;
	for (x=0;isalnum(*symbol) && x<(int) sizeof(symbolname)-1;x++) {
		symbolname[x]=*symbol;
		symbol++;
	}
	symbolname[x]=0;
	return valHook->GetValue(symbolname);
}

static void ParseIdsTarget(const char *&src, Object *&object)
{
	for (int i=0;i<ObjectFieldsCount;i++) {
			object->objectFields[i]=GetIdsValue(src, ObjectIDSTableNames[i]);
		if (*src!='.') {
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
		for (i=0;i<(int) sizeof(object->objectName)-1 && *src!='"';i++)
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
		
		while (Nesting<MaxObjectNesting) {
			memmove(object->objectFilters+1, object->objectFilters, (int) sizeof(int) *(MaxObjectNesting-1) );
			object->objectFilters[0]=GetIdsValue(src,"object");
			if (*src!='(') {
				break;
			}
			src++; //skipping (
			Nesting++;
		}
		if (*src=='[') {
			ParseIdsTarget(src, object);
		}
		src+=Nesting; //skipping )
	}
}

void GameScript::DisplayStringCore(Scriptable* Sender, int Strref, int flags)
{
	printf( "Displaying string on: %s\n", Sender->GetScriptName() );
	if (flags & DS_CONST ) {
		Actor* actor = ( Actor* ) Sender;
		Strref=actor->StrRefs[Strref];
	}
	StringBlock sb = core->strings->GetStringBlock( Strref );
	if (flags & DS_HEAD) {
		Sender->DisplayHeadText( sb.text );
	}
	if (flags & DS_CONSOLE) {
			core->DisplayString( sb.text );
	}
	if (sb.Sound[0] ) {
		ieDword len = core->GetSoundMgr()->Play( sb.Sound );
		ieDword counter = ( AI_UPDATE_TIME * len ) / 1000;
		if ((counter != 0) && (flags &DS_WAIT) )
			Sender->SetWait( counter );
	}
}
/* this function was lifted from GenerateAction, to make it clearer */
Action*GameScript::GenerateActionCore(const char *src, const char *str, int acIndex, bool autoFree)
{
	Action*newAction = new Action(autoFree);
	newAction->actionID = (unsigned short) actionsTable->GetValueIndex( acIndex );
	//this flag tells us to merge 2 consecutive strings together to get
	//a variable (context+variablename)
	int mergestrings = actionflags[newAction->actionID]&AF_MERGESTRINGS;
	int objectCount = ( newAction->actionID == 1 ) ? 0 : 1;
	int stringsCount = 0;
	int intCount = 0;
	//Here is the Action; Now we need to evaluate the parameters, if any
	if (*str!=')') while (*str) {
		if (*(str+1)!=':') {
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
				newAction->pointParameter.x = (short) strtol( src, (char **) &src, 10 );
				src++; //Skip .
				newAction->pointParameter.y = (short) strtol( src, (char **) &src, 10 );
				src++; //Skip ]
				break;

			case 'i': //Integer
			{
				//going to the variable name
				while (*str != '*' && *str !=',' && *str != ')' ) {
					str++;
				}
				int value;
				if (*str=='*') { //there may be an IDS table
					str++;
					char idsTabName[33];
					char* tmp = idsTabName;
					while (( *str != ',' ) && ( *str != ')' )) {
						*tmp = *str;
						tmp++;
						str++;
					}
					*tmp = 0;
					if (idsTabName[0]) {
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
				newAction->objects[0] = NULL; //avoid freeing of object
				delete newAction; //freeing action
				newAction = act;
			}
			break;

			case 'o': //Object
				if (objectCount==3) {
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
				if (mergestrings) {
					for (i=0;i<6;i++) {
						*dst++='*';
					}
				}
				else {
					i=0;
				}
				while (*src != '"') {
					//sizeof(context+name) = 40
					if (i<40) {
						*dst++ = toupper(*src);
						i++;
					}
					src++;
				}
				*dst = 0;
				//reading the context part
				if (mergestrings) {
					str++;
					if (*str!='s') {
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
					if (*src++!='"' || *src++!=',' || *src++!='"') {
						break;
					}
					//reading the context string
					i=0;
					while (*src != '"') {
						if (i++<6) {
							*dst++ = toupper(*src);
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
		if (*src == ',' || *src==')')
			src++;
	}
	return newAction;
}

Action* GameScript::GenerateAction(char* String, bool autoFree)
{
	strlwr( String );
	if (InDebug&8) {
		printf("Compiling:%s\n",String);
	}
	int len = strlench(String,'(')+1; //including (
	int i = actionsTable->FindString(String, len);
	if (i<0) {
		printMessage("GameScript"," ",YELLOW);
		printf("Invalid scripting action: %s\n", String);
		return NULL;
	}
	char *src = String+len;
	char *str = actionsTable->GetStringIndex( i )+len;
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
	if (*str!=')') while (*str) {
		if (*(str+1)!=':') {
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
				newTrigger->pointParameter.x = (short) strtol( src, (char **) &src, 10 );
				src++; //Skip .
				newTrigger->pointParameter.y = (short) strtol( src, (char **) &src, 10 );
				src++; //Skip ]
				break;

			case 'i': //Integer
			{
				//going to the variable name
				while (*str != '*' && *str !=',' && *str != ')' ) {
					str++;
				}
				int value;
				if (*str=='*') { //there may be an IDS table
					str++;
					char idsTabName[33];
					char* tmp = idsTabName;
					while (( *str != ',' ) && ( *str != ')' )) {
						*tmp = *str;
						tmp++;
						str++;
					}
					*tmp = 0;
					if (idsTabName[0]) {
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
				if (mergestrings) {
					for (i=0;i<6;i++) {
						*dst++='*';
					}
				}
				else {
					i=0;
				}
				while (*src != '"') {
					//sizeof(context+name) = 40
					if (i<40) {
						*dst++ = *src;
						i++;
					}
					src++;
				}
				*dst = 0;
				//reading the context part
				if (mergestrings) {
					str++;
					if (*str!='s') {
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
					if (*src++!='"' || *src++!=',' || *src++!='"') {
						break;
					}
					//reading the context string
					i=0;
					while (*src != '"') {
						if (i++<6) {
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
		if (*src == ',' || *src==')')
			src++;
	}
	return newTrigger;
}

Trigger* GameScript::GenerateTrigger(char* String)
{
	strlwr( String );
	if (InDebug&16) {
		printf("Compiling:%s\n",String);
	}
	int negate = 0;
	if (*String == '!') {
		String++;
		negate = 1;
	}
	int len = strlench(String,'(')+1; //including (
	int i = triggersTable->FindString(String, len);
	if (i<0) {
		return NULL;
	}
	char *src = String+len;
	char *str = triggersTable->GetStringIndex( i )+len;
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
	if (Sender->Type==ST_ACTOR) {
		parameters->AddTarget((Actor *) Sender, 0);
	}
	return parameters;
}

//same as player1 so far
Targets *GameScript::Protagonist(Scriptable* /*Sender*/, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(1), 0);
	return parameters;
}

//last talker
Targets *GameScript::Gabber(Scriptable* /*Sender*/, Targets *parameters)
{
	parameters->Clear();
	GameControl* gc = core->GetGameControl();
	if (gc) {
		parameters->AddTarget(gc->speaker, 0);
	}
	return parameters;
}

Targets *GameScript::LastTrigger(Scriptable *Sender, Targets *parameters)
{
	parameters->Clear();
	Scriptable *lt = Sender->LastTrigger;
	if (lt && lt->Type == ST_ACTOR) {
 		parameters->AddTarget((Actor *) lt, 0);
	}
	return parameters;
}

Targets *GameScript::LastSeenBy(Scriptable *Sender, Targets *parameters)
{
	Actor *actor = parameters->GetTarget(0);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		parameters->AddTarget(actor->LastSeen, 0);
	}
	return parameters;
}

Targets *GameScript::LastHelp(Scriptable *Sender, Targets *parameters)
{
	Actor *actor = parameters->GetTarget(0);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		parameters->AddTarget(actor->LastHelp, 0);
	}
	return parameters;
}

Targets *GameScript::LastHeardBy(Scriptable *Sender, Targets *parameters)
{
	Actor *actor = parameters->GetTarget(0);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		parameters->AddTarget(actor->LastHeard, 0);
	}
	return parameters;
}

Targets *GameScript::LastCommandedBy(Scriptable *Sender, Targets *parameters)
{
	Actor *actor = parameters->GetTarget(0);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		parameters->AddTarget(actor->LastCommander, 0);
	}
	return parameters;
}

Targets *GameScript::LastTargetedBy(Scriptable *Sender, Targets *parameters)
{
	Actor *actor = parameters->GetTarget(0);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		parameters->AddTarget(actor->LastTarget, 0);
	}
	return parameters;
}
Targets *GameScript::LastAttackerOf(Scriptable *Sender, Targets *parameters)
{
	Actor *actor = parameters->GetTarget(0);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		parameters->AddTarget(actor->LastAttacker, 0);
	}
	return parameters;
}

Targets *GameScript::LastHitter(Scriptable *Sender, Targets *parameters)
{
	Actor *actor = parameters->GetTarget(0);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		parameters->AddTarget(actor->LastHitter, 0);
	}
	return parameters;
}

Targets *GameScript::LastTalkedToBy(Scriptable *Sender, Targets *parameters)
{
	Actor *actor = parameters->GetTarget(0);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		parameters->AddTarget(actor->LastTalkedTo, 0);
	}
	return parameters;
}

Targets *GameScript::LastSummonerOf(Scriptable* Sender, Targets *parameters)
{
	Actor *actor = parameters->GetTarget(0);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		parameters->AddTarget(actor->LastSummoner, 0);
	}
	return parameters;
}

Targets *GameScript::Player1(Scriptable* /*Sender*/, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(1), 0);
	return parameters;
}

Targets *GameScript::Player1Fill(Scriptable* /*Sender*/, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(0), 0);
	return parameters;
}

Targets *GameScript::Player2(Scriptable* /*Sender*/, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(2), 0);
	return parameters;
}

Targets *GameScript::Player2Fill(Scriptable* /*Sender*/, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(1), 0);
	return parameters;
}

Targets *GameScript::Player3(Scriptable* /*Sender*/, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(3), 0);
	return parameters;
}

Targets *GameScript::Player3Fill(Scriptable* /*Sender*/, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(2), 0);
	return parameters;
}

Targets *GameScript::Player4(Scriptable* /*Sender*/, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(4), 0);
	return parameters;
}

Targets *GameScript::Player4Fill(Scriptable* /*Sender*/, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(3), 0);
	return parameters;
}

Targets *GameScript::Player5(Scriptable* /*Sender*/, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(5), 0);
	return parameters;
}

Targets *GameScript::Player5Fill(Scriptable* /*Sender*/, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(4), 0);
	return parameters;
}

Targets *GameScript::Player6(Scriptable* /*Sender*/, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(6), 0);
	return parameters;
}

Targets *GameScript::Player6Fill(Scriptable* /*Sender*/, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(5), 0);
	return parameters;
}

Targets *GameScript::Player7(Scriptable* /*Sender*/, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(7), 0);
	return parameters;
}

Targets *GameScript::Player7Fill(Scriptable* /*Sender*/, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(6), 0);
	return parameters;
}

Targets *GameScript::Player8(Scriptable* /*Sender*/, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(8), 0);
	return parameters;
}

Targets *GameScript::Player8Fill(Scriptable* /*Sender*/, Targets *parameters)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(7), 0);
	return parameters;
}

Targets *GameScript::BestAC(Scriptable* /*Sender*/, Targets *parameters)
{
	targetlist::iterator m;
	targettype *t = parameters->GetFirstTarget(m);
	if (!t) {
		return parameters;
	}
	int bestac=t->actor->GetStat(IE_ARMORCLASS);
	targettype *select = t;
	// assignment in while
	while ( (t = parameters->GetNextTarget(m) ) ) {
		int ac=t->actor->GetStat(IE_ARMORCLASS);
		if (bestac<ac) {
			bestac=ac;
			select=t;
		}
	}

	parameters->Clear();
	parameters->AddTarget(select->actor, 0);
	return parameters;
}

/*no idea why this object exists since the gender could be filtered easier*/
Targets *GameScript::StrongestOfMale(Scriptable* /*Sender*/, Targets *parameters)
{
	targetlist::iterator m;
	targettype *t = parameters->GetFirstTarget(m);
	if (!t) {
		return parameters;
	}
	int pos=-1;
	int worsthp=-1;
	targettype *select = NULL;
	while ( (t = parameters->GetNextTarget(m) ) ) {
		if (t->actor->GetStat(IE_SEX)!=1) continue;
		int hp=t->actor->GetStat(IE_HITPOINTS);
		if ((pos==-1) || (worsthp<hp)) {
			worsthp=hp;
			select=t;
		}
	}
	parameters->Clear();
	if (select) {
		parameters->AddTarget(select->actor, 0);
	}
	return parameters;
}

Targets *GameScript::StrongestOf(Scriptable* /*Sender*/, Targets *parameters)
{
	targetlist::iterator m;
	targettype *t = parameters->GetFirstTarget(m);
	if (!t) {
		return parameters;
	}
	int besthp=t->actor->GetStat(IE_HITPOINTS);
	targettype *select = t;
	// assignment in while
	while ( (t = parameters->GetNextTarget(m) ) ) {
		int hp=t->actor->GetStat(IE_HITPOINTS);
		if (besthp<hp) {
			besthp=hp;
			select=t;
		}
	}
	parameters->Clear();
	parameters->AddTarget(select->actor, 0);
	return parameters;
}

Targets *GameScript::WeakestOf(Scriptable* /*Sender*/, Targets *parameters)
{
	targetlist::iterator m;
	targettype *t = parameters->GetFirstTarget(m);
	if (!t) {
		return parameters;
	}
	int worsthp=t->actor->GetStat(IE_HITPOINTS);
	targettype *select = t;
	// assignment in while
	while ( (t = parameters->GetNextTarget(m) ) ) {
		int hp=t->actor->GetStat(IE_HITPOINTS);
		if (worsthp>hp) {
			worsthp=hp;
			select=t;
		}
	}
	parameters->Clear();
	parameters->AddTarget(select->actor, 0);
	return parameters;
}

Targets *GameScript::WorstAC(Scriptable* /*Sender*/, Targets *parameters)
{
	targetlist::iterator m;
	targettype *t = parameters->GetFirstTarget(m);
	if (!t) {
		return parameters;
	}
	int worstac=t->actor->GetStat(IE_ARMORCLASS);
	targettype *select = t;
	// assignment in while
	while ( (t = parameters->GetNextTarget(m) ) ) {
		int ac=t->actor->GetStat(IE_ARMORCLASS);
		if (worstac>ac) {
			worstac=ac;
			select=t;
		}
	}
	parameters->Clear();
	parameters->AddTarget(select->actor, 0);
	return parameters;
}

Targets *GameScript::MostDamagedOf(Scriptable* /*Sender*/, Targets *parameters)
{
	targetlist::iterator m;
	targettype *t = parameters->GetFirstTarget(m);
	if (!t) {
		return parameters;
	}
	int worsthp=t->actor->GetStat(IE_MAXHITPOINTS)-t->actor->GetStat(IE_HITPOINTS);
	targettype *select = t;
	// assignment in while
	while ( (t = parameters->GetNextTarget(m) ) ) {
		int hp=t->actor->GetStat(IE_MAXHITPOINTS)-t->actor->GetStat(IE_HITPOINTS);
		if (worsthp>hp) {
			worsthp=hp;
			select=t;
		}
	}
	parameters->Clear();
	parameters->AddTarget(select->actor, 0);
	return parameters;
}
Targets *GameScript::LeastDamagedOf(Scriptable* /*Sender*/, Targets *parameters)
{
	targetlist::iterator m;
	targettype *t = parameters->GetFirstTarget(m);
	if (!t) {
		return parameters;
	}
	int besthp=t->actor->GetStat(IE_MAXHITPOINTS)-t->actor->GetStat(IE_HITPOINTS);
	targettype *select = t;
	// assignment in while
	while ( (t = parameters->GetNextTarget(m) ) ) {
		int hp=t->actor->GetStat(IE_MAXHITPOINTS)-t->actor->GetStat(IE_HITPOINTS);
		if (besthp<hp) {
			besthp=hp;
			select=t;
		}
	}
	parameters->Clear();
	parameters->AddTarget(select->actor, 0);
	return parameters;
}

Targets *GameScript::XthNearestOf(Targets *parameters, int count)
{
	Actor *origin = parameters->GetTarget(count);
	parameters->Clear();
	if (!origin) {
		return parameters;
	}
	parameters->AddTarget(origin, 0);
	return parameters;
}

Targets *GameScript::XthNearestEnemyOfType(Scriptable *origin, Targets *parameters, int count)
{
	if (origin->Type != ST_ACTOR) {
		parameters->Clear();
		return parameters;
	}

	targetlist::iterator m;
	targettype *t = parameters->GetFirstTarget(m);
	if (!t) {
		return parameters;
	}
	Actor *actor = (Actor *) origin;
	//determining the allegiance of the origin
	int type = 2; //neutral, has no enemies
	if (actor->GetStat(IE_EA) <= GOODCUTOFF) {
		type = 1; //PC
	}
	if (actor->GetStat(IE_EA) >= EVILCUTOFF) {
		type = 0;
	}
	if (type==2) {
		parameters->Clear();
		return parameters;
	}

	while ( t ) {
		if (type) { //origin is PC
			if (t->actor->GetStat(IE_EA) <= GOODCUTOFF) {
				t=parameters->RemoveTargetAt(m);
				continue;
			}
		}
		else {
			if (t->actor->GetStat(IE_EA) >= EVILCUTOFF) {
				t=parameters->RemoveTargetAt(m);
				continue;
			}
		}
		t = parameters->GetNextTarget(m);
	}
	return XthNearestOf(parameters,count);
}

Targets *GameScript::XthNearestEnemyOf(Targets *parameters, int count)
{
	Actor *origin = parameters->GetTarget(0);
	parameters->Clear();
	if (!origin) {
		return parameters;
	}
	//determining the allegiance of the origin
	int type = 2; //neutral, has no enemies
	if (origin->GetStat(IE_EA) <= GOODCUTOFF) {
		type = 1; //PC
	}
	if (origin->GetStat(IE_EA) >= EVILCUTOFF) {
		type = 0;
	}
	if (type==2) {
		return parameters;
	}
	Map *map = origin->GetCurrentArea();
	int i = map->GetActorCount();
	Actor *ac;
	while (i--) {
		ac=map->GetActor(i);
		int distance = Distance(ac, origin);
		if (type) { //origin is PC
			if (ac->GetStat(IE_EA) >= EVILCUTOFF) {
				parameters->AddTarget(ac, distance);
			}
		}
		else {
			if (ac->GetStat(IE_EA) <= GOODCUTOFF) {
				parameters->AddTarget(ac, distance);
			}
		}
	}
	return XthNearestOf(parameters,count);
}

Targets *GameScript::Farthest(Scriptable* /*Sender*/, Targets *parameters)
{
	targettype *t = parameters->GetLastTarget();
	parameters->Clear();
	if (t) {
		parameters->AddTarget(t->actor, 0);
	}
	return parameters;
}

Targets *GameScript::NearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters)
{
	return XthNearestEnemyOf(parameters, 0);
}

Targets *GameScript::SecondNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters)
{
	return XthNearestEnemyOf(parameters, 1);
}

Targets *GameScript::ThirdNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters)
{
	return XthNearestEnemyOf(parameters, 2);
}

Targets *GameScript::FourthNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters)
{
	return XthNearestEnemyOf(parameters, 3);
}

Targets *GameScript::FifthNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters)
{
	return XthNearestEnemyOf(parameters, 4);
}

Targets *GameScript::SixthNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters)
{
	return XthNearestEnemyOf(parameters, 5);
}

Targets *GameScript::SeventhNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters)
{
	return XthNearestEnemyOf(parameters, 6);
}

Targets *GameScript::EighthNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters)
{
	return XthNearestEnemyOf(parameters, 7);
}

Targets *GameScript::NinthNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters)
{
	return XthNearestEnemyOf(parameters, 8);
}

Targets *GameScript::TenthNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters)
{
	return XthNearestEnemyOf(parameters, 9);
}

Targets *GameScript::NearestEnemyOfType(Scriptable* Sender, Targets *parameters)
{
	return XthNearestEnemyOfType(Sender, parameters, 0);
}

Targets *GameScript::SecondNearestEnemyOfType(Scriptable* Sender, Targets *parameters)
{
	return XthNearestEnemyOfType(Sender, parameters, 1);
}

Targets *GameScript::ThirdNearestEnemyOfType(Scriptable* Sender, Targets *parameters)
{
	return XthNearestEnemyOfType(Sender, parameters, 2);
}

Targets *GameScript::FourthNearestEnemyOfType(Scriptable* Sender, Targets *parameters)
{
	return XthNearestEnemyOfType(Sender, parameters, 3);
}

Targets *GameScript::FifthNearestEnemyOfType(Scriptable* Sender, Targets *parameters)
{
	return XthNearestEnemyOfType(Sender, parameters, 4);
}

Targets *GameScript::SixthNearestEnemyOfType(Scriptable* Sender, Targets *parameters)
{
	return XthNearestEnemyOfType(Sender, parameters, 5);
}

Targets *GameScript::SeventhNearestEnemyOfType(Scriptable* Sender, Targets *parameters)
{
	return XthNearestEnemyOfType(Sender, parameters, 6);
}

Targets *GameScript::EighthNearestEnemyOfType(Scriptable* Sender, Targets *parameters)
{
	return XthNearestEnemyOfType(Sender, parameters, 7);
}

Targets *GameScript::NinthNearestEnemyOfType(Scriptable* Sender, Targets *parameters)
{
	return XthNearestEnemyOfType(Sender, parameters, 8);
}

Targets *GameScript::TenthNearestEnemyOfType(Scriptable* Sender, Targets *parameters)
{
	return XthNearestEnemyOfType(Sender, parameters, 9);
}

Targets *GameScript::NearestPC(Scriptable* Sender, Targets *parameters)
{
	parameters->Clear();
	Map *map = Sender->GetCurrentArea();
	int i = map->GetActorCount();
	int mindist = -1;
	Actor *ac = NULL;
	while (i--) {
		Actor *newactor=map->GetActor(i);
		int dist = Distance(Sender, ac);
		if (ac->InParty) {
			if ( (mindist == -1) || (dist<mindist) ) {
				ac = newactor;
				mindist = dist;
			}
		}
	}
	if (ac) {
		parameters->AddTarget(ac, 0);
	}
	return parameters;
}

Targets *GameScript::Nearest(Scriptable* /*Sender*/, Targets *parameters)
{
	return XthNearestOf(parameters, 0);
}

Targets *GameScript::SecondNearest(Scriptable* /*Sender*/, Targets *parameters)
{
	return XthNearestOf(parameters, 1);
}

Targets *GameScript::ThirdNearest(Scriptable* /*Sender*/, Targets *parameters)
{
	return XthNearestOf(parameters, 2);
}

Targets *GameScript::FourthNearest(Scriptable* /*Sender*/, Targets *parameters)
{
	return XthNearestOf(parameters, 3);
}

Targets *GameScript::FifthNearest(Scriptable* /*Sender*/, Targets *parameters)
{
	return XthNearestOf(parameters, 4);
}

Targets *GameScript::SixthNearest(Scriptable* /*Sender*/, Targets *parameters)
{
	return XthNearestOf(parameters, 5);
}

Targets *GameScript::SeventhNearest(Scriptable* /*Sender*/, Targets *parameters)
{
	return XthNearestOf(parameters, 6);
}

Targets *GameScript::EighthNearest(Scriptable* /*Sender*/, Targets *parameters)
{
	return XthNearestOf(parameters, 7);
}

Targets *GameScript::NinthNearest(Scriptable* /*Sender*/, Targets *parameters)
{
	return XthNearestOf(parameters, 8);
}

Targets *GameScript::TenthNearest(Scriptable* /*Sender*/, Targets *parameters)
{
	return XthNearestOf(parameters, 9);
}

Targets *GameScript::SelectedCharacter(Scriptable* Sender, Targets* parameters)
{
	Map *cm = Sender->GetCurrentArea();
	parameters->Clear();
	int i = cm->GetActorCount();
	while (i--) {
		Actor *ac=cm->GetActor(i);
		if (ac->IsSelected()) {
			parameters->AddTarget(ac, Distance(Sender, ac) );
		}
	}
	return parameters;
}

Targets *GameScript::Nothing(Scriptable* /*Sender*/, Targets* parameters)
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
	int value = actor->GetStat(IE_CLASS);
	if (parameter==value) return 1;
	//check on multiclass
	return 0;
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
int GameScript::BreakingPoint(Scriptable* Sender, Trigger* /*parameters*/)
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

int GameScript::Reputation(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->Reputation/10 == (ieDword) parameters->int0Parameter;
}

int GameScript::ReputationGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->Reputation/10 > (ieDword) parameters->int0Parameter;
}

int GameScript::ReputationLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->Reputation/10 < (ieDword) parameters->int0Parameter;
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

//should return *_ALL stuff
int GameScript::Class(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = (Actor*)scr;
	return ID_Class( actor, parameters->int0Parameter);
}

//should not handle >200, but should check on multi-class
//this is most likely ClassMask
int GameScript::ClassEx(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = (Actor*)scr;
	return ID_ClassMask( actor, parameters->int0Parameter);
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

static int CanSee(Scriptable* Sender, Scriptable* target)
{
	Map *map;
	unsigned int range;

	if (Sender->Type == ST_ACTOR) {
		Actor* snd = ( Actor* ) Sender;
		range = snd->Modified[IE_VISUALRANGE] * 20;
		map = Sender->GetCurrentArea();
		//huh, lets hope it won't crash often
		if (!map) {
			abort();
		}
	}
	else { 
		map = Sender->GetCurrentArea();
		range = 20 * 20;
	}
	if ( target->GetCurrentArea()!=map ) {
		return 0;
	}

	if (Distance(target->Pos, Sender->Pos) > range) {
		return 0;
	}
	return map->IsVisible(target->Pos, Sender->Pos);
}

int GameScript::ValidForDialogCore(Scriptable* Sender, Actor *target)
{
	if (!CanSee(Sender, target) ) {
		return 0;
	}
	
	//we should rather use STATE_SPEECHLESS_MASK
	if (target->GetStat(IE_STATE_ID)&STATE_DEAD) {
		return 0;
	}
	return 1;
}

int GameScript::NearbyDialog(Scriptable* Sender, Trigger* parameters)
{
	Actor *target = Sender->GetCurrentArea()->GetActorByDialog(parameters->string0Parameter);
	if ( !target ) {
		return 0;
	}
	return ValidForDialogCore( Sender, target );
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
	Actor *target = (Actor *) scr;
	if (!core->GetGame()->InParty( target )) {
		return 0;
	}
	return ValidForDialogCore( Sender, target );
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
	if (core->GetGame()->InParty( tar ) <0) {
		return 0;
	}
	//don't allow dead
	return tar->ValidTarget(GA_NO_DEAD);
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
	Actor *actor = core->GetGame()->GetPC(parameters->int0Parameter);
	return MatchActor(Sender, actor, parameters->objectParameter);
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

int GameScript::IsGabber(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	if ((Actor *) scr == core->GetGameControl()->speaker)
		return 1;
	return 0;
}

int GameScript::IsActive(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	if (scr->Active&SCR_ACTIVE) {
		return 1;
	}
	return 0;
}

int GameScript::Kit(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = (Actor *) scr;
	//only the low 2 bytes count
	if ( (ieWord) actor->GetStat(IE_KIT) == (ieWord) parameters->int0Parameter) {
		return 1;
	}
	return 0;
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
	ieDword value = CheckVariable(Sender, parameters->string0Parameter );
	return ( value& parameters->int0Parameter ) !=0;
}

int GameScript::BitCheckExact(Scriptable* Sender, Trigger* parameters)
{
	ieDword value = CheckVariable(Sender, parameters->string0Parameter );
	return (value & parameters->int0Parameter ) == (ieDword) parameters->int0Parameter;
}

//BM_OR would make sense only if this trigger changes the value of the variable
//should I do that???
int GameScript::BitGlobal_Trigger(Scriptable* Sender, Trigger* parameters)
{
	ieDword value = CheckVariable(Sender, parameters->string0Parameter );
	HandleBitMod(value, parameters->int0Parameter, parameters->int1Parameter);
	//set the variable here if BitGlobal_Trigger really does that
	//SetVariable(Sender, parameters->string0Parameter, value);
	return value!=0;
}

int GameScript::GlobalOrGlobal_Trigger(Scriptable* Sender, Trigger* parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter );
	if ( value1 ) return 1;
	ieDword value2 = CheckVariable(Sender, parameters->string1Parameter );
	if ( value2 ) return 1;
	return 0;
}

int GameScript::GlobalBAndGlobal_Trigger(Scriptable* Sender, Trigger* parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter );
	ieDword value2 = CheckVariable(Sender, parameters->string1Parameter );
	return ( value1& value2 ) != 0;
}

int GameScript::GlobalBitGlobal_Trigger(Scriptable* Sender, Trigger* parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter );
	ieDword value2 = CheckVariable(Sender, parameters->string1Parameter );
	HandleBitMod( value1, value2, parameters->int1Parameter);
	return value1!=0;
}

//would this function also alter the variable?
int GameScript::Xor(Scriptable* Sender, Trigger* parameters)
{
	ieDword value = CheckVariable(Sender, parameters->string0Parameter );
	return ( value ^ parameters->int0Parameter ) != 0;
}

int GameScript::NumDead(Scriptable* Sender, Trigger* parameters)
{
	long value;

	if (core->HasFeature(GF_HAS_KAPUTZ) ) {
		value = CheckVariable(Sender, parameters->string0Parameter, "KAPUTZ");
	}
	else {
		char VariableName[33];
		snprintf(VariableName,32, "SPRITE_IS_DEAD%s",parameters->string0Parameter);
		value = CheckVariable(Sender, VariableName, "GLOBAL" );
	}
	return ( value == parameters->int0Parameter );
}

int GameScript::NumDeadGT(Scriptable* Sender, Trigger* parameters)
{
	long value;

	if (core->HasFeature(GF_HAS_KAPUTZ) ) {
		value = CheckVariable(Sender, parameters->string0Parameter, "KAPUTZ");
	}
	else {
		char VariableName[33];
		snprintf(VariableName,32, "SPRITE_IS_DEAD%s",parameters->string0Parameter);
		value = CheckVariable(Sender, VariableName, "GLOBAL" );
	}
	return ( value > parameters->int0Parameter );
}

int GameScript::NumDeadLT(Scriptable* Sender, Trigger* parameters)
{
	long value;

	if (core->HasFeature(GF_HAS_KAPUTZ) ) {
		value = CheckVariable(Sender, parameters->string0Parameter, "KAPUTZ");
	}
	else {
		char VariableName[33];
		snprintf(VariableName,32, "SPRITE_IS_DEAD%s",parameters->string0Parameter);
		value = CheckVariable(Sender, VariableName, "GLOBAL" );
	}
	return ( value < parameters->int0Parameter );
}

int GameScript::G_Trigger(Scriptable* Sender, Trigger* parameters)
{
	long value = CheckVariable(Sender, parameters->string0Parameter, "GLOBAL" );
	return ( value == parameters->int0Parameter );
}

int GameScript::Global(Scriptable* Sender, Trigger* parameters)
{
	long value = CheckVariable(Sender, parameters->string0Parameter );
	return ( value == parameters->int0Parameter );
}

int GameScript::GLT_Trigger(Scriptable* Sender, Trigger* parameters)
{
	long value = CheckVariable(Sender, parameters->string0Parameter,"GLOBAL" );
	return ( value < parameters->int0Parameter );
}

int GameScript::GlobalLT(Scriptable* Sender, Trigger* parameters)
{
	long value = CheckVariable(Sender, parameters->string0Parameter );
	return ( value < parameters->int0Parameter );
}

int GameScript::GGT_Trigger(Scriptable* Sender, Trigger* parameters)
{
	long value = CheckVariable(Sender, parameters->string0Parameter, "GLOBAL" );
	return ( value > parameters->int0Parameter );
}

int GameScript::GlobalGT(Scriptable* Sender, Trigger* parameters)
{
	long value = CheckVariable(Sender, parameters->string0Parameter );
	return ( value > parameters->int0Parameter );
}

int GameScript::GlobalLTGlobal(Scriptable* Sender, Trigger* parameters)
{
	long value1 = CheckVariable(Sender, parameters->string0Parameter );
	long value2 = CheckVariable(Sender, parameters->string1Parameter );
	return ( value1 < value2 );
}

int GameScript::GlobalGTGlobal(Scriptable* Sender, Trigger* parameters)
{
	long value1 = CheckVariable(Sender, parameters->string0Parameter );
	long value2 = CheckVariable(Sender, parameters->string1Parameter );
	return ( value1 > value2 );
}

int GameScript::GlobalsEqual(Scriptable* Sender, Trigger* parameters)
{
	long value1 = CheckVariable(Sender, parameters->string0Parameter, "GLOBAL" );
	long value2 = CheckVariable(Sender, parameters->string1Parameter, "GLOBAL" );
	return ( value1 == value2 );
}

int GameScript::GlobalsGT(Scriptable* Sender, Trigger* parameters)
{
	long value1 = CheckVariable(Sender, parameters->string0Parameter, "GLOBAL" );
	long value2 = CheckVariable(Sender, parameters->string1Parameter, "GLOBAL" );
	return ( value1 > value2 );
}

int GameScript::GlobalsLT(Scriptable* Sender, Trigger* parameters)
{
	long value1 = CheckVariable(Sender, parameters->string0Parameter, "GLOBAL" );
	long value2 = CheckVariable(Sender, parameters->string1Parameter, "GLOBAL" );
	return ( value1 < value2 );
}

int GameScript::LocalsEqual(Scriptable* Sender, Trigger* parameters)
{
	long value1 = CheckVariable(Sender, parameters->string0Parameter, "LOCALS" );
	long value2 = CheckVariable(Sender, parameters->string1Parameter, "LOCALS" );
	return ( value1 == value2 );
}

int GameScript::LocalsGT(Scriptable* Sender, Trigger* parameters)
{
	long value1 = CheckVariable(Sender, parameters->string0Parameter, "LOCALS" );
	long value2 = CheckVariable(Sender, parameters->string1Parameter, "LOCALS" );
	return ( value1 > value2 );
}

int GameScript::LocalsLT(Scriptable* Sender, Trigger* parameters)
{
	long value1 = CheckVariable(Sender, parameters->string0Parameter, "LOCALS" );
	long value2 = CheckVariable(Sender, parameters->string1Parameter, "LOCALS" );
	return ( value1 < value2 );
}

int GameScript::RealGlobalTimerExact(Scriptable* Sender, Trigger* parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, parameters->string1Parameter );
	if (!value1) return 0;
	ieDword value2;
	GetTime(value2);
	return ( value1 == value2 );
}

int GameScript::RealGlobalTimerExpired(Scriptable* Sender, Trigger* parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, parameters->string1Parameter );
	if (!value1) return 0;
	ieDword value2;
	GetTime(value2);
	return ( value1 < value2 );
}

int GameScript::RealGlobalTimerNotExpired(Scriptable* Sender, Trigger* parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, parameters->string1Parameter );
	if (!value1) return 0;
	ieDword value2;
	GetTime(value2);
	return ( value1 > value2 );
}

int GameScript::GlobalTimerExact(Scriptable* Sender, Trigger* parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, parameters->string1Parameter );
	if (!value1) return 0;
	return ( value1 == core->GetGame()->GameTime );
}

int GameScript::GlobalTimerExpired(Scriptable* Sender, Trigger* parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, parameters->string1Parameter );
	if (!value1) return 0;
	return ( value1 < core->GetGame()->GameTime );
}

int GameScript::GlobalTimerNotExpired(Scriptable* Sender, Trigger* parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter, parameters->string1Parameter );
	if (!value1) return 0;
	return ( value1 > core->GetGame()->GameTime );
}

int GameScript::OnCreation(Scriptable* Sender, Trigger* /*parameters*/)
{
	return Sender->OnCreation; //hopefully this is always 1 or 0
}

int GameScript::NumItemsParty(Scriptable* /*Sender*/, Trigger* parameters)
{
	int cnt = 0;
	Actor *actor;
	Game *game=core->GetGame();

	//there is an assignment here
	for (int i=0; (actor = game->GetPC(i)) ; i++) {
		cnt+=actor->inventory.CountItems(parameters->string0Parameter,1);
	}
	return cnt==parameters->int0Parameter;
}

int GameScript::NumItemsPartyGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	int cnt = 0;
	Actor *actor;
	Game *game=core->GetGame();

	//there is an assignment here
	for (int i=0; (actor = game->GetPC(i)) ; i++) {
		cnt+=actor->inventory.CountItems(parameters->string0Parameter,1);
	}
	return cnt>parameters->int0Parameter;
}

int GameScript::NumItemsPartyLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	int cnt = 0;
	Actor *actor;
	Game *game=core->GetGame();

	//there is an assignment here
	for (int i=0; (actor = game->GetPC(i)) ; i++) {
		cnt+=actor->inventory.CountItems(parameters->string0Parameter,1);
	}
	return cnt<parameters->int0Parameter;
}

int GameScript::NumItems(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if ( !tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	int cnt = actor->inventory.CountItems(parameters->string0Parameter,1);
	return cnt==parameters->int0Parameter;
}

int GameScript::NumItemsGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if ( !tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	int cnt = actor->inventory.CountItems(parameters->string0Parameter,1);
	return cnt>parameters->int0Parameter;
}

int GameScript::NumItemsLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if ( !tar || tar->Type!=ST_ACTOR) {
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
	if ( !tar || tar->Type!=ST_CONTAINER) {
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
	if ( !scr || scr->Type!=ST_ACTOR) {
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
	if ( !scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) scr;
	if (actor->inventory.HasItem(parameters->string0Parameter, IE_INV_ITEM_IDENTIFIED) ) {
		return 1;
	}
	return 0;
}

/** if the string is zero, then it will return true if there is any item in the slot */
/** if the string is non-zero, it will return true, if the given item was in the slot */
int GameScript::HasItemSlot(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if ( !scr || scr->Type!=ST_ACTOR) {
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
	if (actor->inventory.HasItem(parameters->string0Parameter, IE_INV_ITEM_EQUIPPED) ) {
		return 1;
	}
	return 0;
}

int GameScript::Acquired(Scriptable * Sender, Trigger* parameters)
{
	if ( Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) Sender;
	if (actor->inventory.HasItem(parameters->string0Parameter, IE_INV_ITEM_ACQUIRED) ) {
		return 1;
	}
	return 0;
}

/** this trigger accepts a numeric parameter, this number could be: */
/** 0 - normal, 1 - equipped, 2 - identified, 3 - equipped&identified */
/** this is a GemRB extension */
int GameScript::PartyHasItem(Scriptable * /*Sender*/, Trigger* parameters)
{
	Actor *actor;
	Game *game=core->GetGame();

	//there is an assignment here
	for (int i=0; (actor = game->GetPC(i)) ; i++) {
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
	for (int i=0; (actor = game->GetPC(i)) ; i++) {
		if (actor->inventory.HasItem(parameters->string0Parameter, IE_INV_ITEM_IDENTIFIED) ) {
			return 1;
		}
	}
	return 0;
}

int GameScript::InventoryFull( Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	if (actor->inventory.FindCandidateSlot( SLOT_INVENTORY, 0 )==-1) {
		return 1;
	}
	return 0;
}

int GameScript::HaveSpell(Scriptable *Sender, Trigger *parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) Sender;
	if (parameters->string0Parameter[0]) {
		return actor->spellbook.HaveSpell(parameters->string0Parameter, 0);
	}
	return actor->spellbook.HaveSpell(parameters->int0Parameter, 0);
/*
	ieResRef tmpname;
	CreateSpellName(tmpname, parameters->int0Parameter);
	return actor->spellbook.HaveSpell(tmpname, 0);
*/
}

int GameScript::HaveAnySpells(Scriptable* Sender, Trigger* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) Sender;
	return actor->spellbook.HaveSpell("", 0);
}

int GameScript::HaveSpellParty(Scriptable* /*Sender*/, Trigger *parameters)
{
	Actor *actor;
	//char *poi;
	//ieResRef tmpname;
	Game *game=core->GetGame();

	if (parameters->string0Parameter[0]) {
		//poi=parameters->string0Parameter;
		//there is an assignment here
		for (int i=0; (actor = game->GetPC(i)) ; i++) {
			if (actor->spellbook.HaveSpell(parameters->string0Parameter, 0) ) {
				return 1;
			}
		}
	}
	else {
		for (int i=0; (actor = game->GetPC(i)) ; i++) {
			if (actor->spellbook.HaveSpell(parameters->int0Parameter, 0) ) {
				return 1;
			}
		}
		//CreateSpellName(tmpname, parameters->int0Parameter);
		//poi=tmpname;
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
	return actor->TalkCount == (ieDword) parameters->int0Parameter ? 1 : 0;
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
	return actor->TalkCount > (ieDword) parameters->int0Parameter ? 1 : 0;
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
	return actor->TalkCount < (ieDword) parameters->int0Parameter ? 1 : 0;
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

int GameScript::ObjectActionListEmpty(Scriptable* Sender, Trigger* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	if (Sender->GetNextAction()) {
		return 0;
	}
	return 1;
}

int GameScript::False(Scriptable* /*Sender*/, Trigger* /*parameters*/)
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
	int distance = Distance(parameters->pointParameter, scr);
	if (distance <= ( parameters->int0Parameter * 20 )) {
		return 1;
	}
	return 0;
}

int GameScript::NearSavedLocation(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
		return 0;
	}
	ieDword value;
	if (!parameters->string0Parameter[0]) {
		strcpy(parameters->string0Parameter,"LOCALSsavedlocation");
	}
	value = (ieDword) CheckVariable( scr, parameters->string0Parameter );
	Point p = { *(unsigned short *) &value, *(((unsigned short *) &value)+1)};
	int distance = Distance(p, scr);
	if (distance <= ( parameters->int0Parameter * 20 )) {
		return 1;
	}
	return 0;
}

int GameScript::Or(Scriptable* /*Sender*/, Trigger* parameters)
{
	return parameters->int0Parameter;
}

int GameScript::Clicked(Scriptable* Sender, Trigger* parameters)
{
	if (parameters->objectParameter->objectFields[0] == 0) {
		if (Sender->LastTrigger) {
			return 1;
		}
		return 0;
	}
	Scriptable* target = GetActorFromObject( Sender, parameters->objectParameter );
	if (Sender->LastTrigger == target) {
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
	Scriptable* target = GetActorFromObject( Sender, parameters->objectParameter );
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
	Scriptable* target = GetActorFromObject( Sender, parameters->objectParameter );
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

int GameScript::Die(Scriptable* Sender, Trigger* /*parameters*/)
{
	if (!Sender || Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor *act=(Actor *) Sender;
	if (act->InternalFlags&IF_JUSTDIED) {
		return 1;
	}
	return 0;
}

int GameScript::PartyMemberDied(Scriptable* /*Sender*/, Trigger* /*parameters*/)
{
	return core->GetGame()->PartyMemberDied();
}

int GameScript::NamelessBitTheDust(Scriptable* /*Sender*/, Trigger* /*parameters*/)
{
	Actor* actor = core->GetGame()->FindPC(1);
	if (actor->InternalFlags&IF_JUSTDIED) {
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
	if ((signed) actor->GetStat( IE_HITPOINTS ) == parameters->int0Parameter) {
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
	if ( (signed) actor->GetStat( IE_HITPOINTS ) > parameters->int0Parameter) {
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
	if ( (signed) actor->GetStat( IE_HITPOINTS ) < parameters->int0Parameter) {
		return 1;
	}
	return 0;
}

int GameScript::HPPercent(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr) {
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
	if (!scr) {
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
	if (!scr) {
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
	if (actor->GetStat( IE_XP ) == (unsigned) parameters->int0Parameter) {
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
	if (actor->GetStat( IE_XP ) > (unsigned) parameters->int0Parameter) {
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
	if (actor->GetStat( IE_XP ) < (unsigned) parameters->int0Parameter) {
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
	if ((signed) actor->GetStat( parameters->int0Parameter ) == parameters->int1Parameter) {
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
	if ((signed) actor->GetStat( parameters->int0Parameter ) > parameters->int1Parameter) {
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
	if ((signed) actor->GetStat( parameters->int0Parameter ) < parameters->int1Parameter) {
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
	//both are actors
	if (CanSee(Sender, tar) ) {
		if (justlos) {
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
	if (!see) {
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

int GameScript::NumCreatureVsParty(Scriptable* Sender, Trigger* parameters)
{
	parameters->objectParameter->objectFields[0]=EVILCUTOFF;
	int value = GetObjectCount(Sender, parameters->objectParameter);
	return value == parameters->int0Parameter;
}

int GameScript::NumCreatureVsPartyGT(Scriptable* Sender, Trigger* parameters)
{
	parameters->objectParameter->objectFields[0]=EVILCUTOFF;
	int value = GetObjectCount(Sender, parameters->objectParameter);
	return value > parameters->int0Parameter;
}

int GameScript::NumCreatureVsPartyLT(Scriptable* Sender, Trigger* parameters)
{
	parameters->objectParameter->objectFields[0]=EVILCUTOFF;
	int value = GetObjectCount(Sender, parameters->objectParameter);
	return value < parameters->int0Parameter;
}

int GameScript::Morale(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return (signed) actor->GetStat(IE_MORALEBREAK) == parameters->int0Parameter;
}

int GameScript::MoraleGT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return (signed) actor->GetStat(IE_MORALEBREAK) > parameters->int0Parameter;
}

int GameScript::MoraleLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return (signed) actor->GetStat(IE_MORALEBREAK) < parameters->int0Parameter;
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

int GameScript::RandomNum(Scriptable* /*Sender*/, Trigger* parameters)
{
	if (parameters->int0Parameter<0) {
		return 0;
	}
	if (parameters->int1Parameter<0) {
		return 0;
	}
	return parameters->int1Parameter-1 == RandomNumValue%parameters->int0Parameter;
}

int GameScript::RandomNumGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	if (parameters->int0Parameter<0) {
		return 0;
	}
	if (parameters->int1Parameter<0) {
		return 0;
	}
	return parameters->int1Parameter-1 == RandomNumValue%parameters->int0Parameter;
}

int GameScript::RandomNumLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	if (parameters->int0Parameter<0) {
		return 0;
	}
	if (parameters->int1Parameter<0) {
		return 0;
	}
	return parameters->int1Parameter-1 == RandomNumValue%parameters->int0Parameter;
}

int GameScript::OpenState(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		printf("[GameScript]: couldn't find door/container:%s\n",parameters->objectParameter->objectName);
		return 0;
	}
	switch(tar->Type) {
		case ST_DOOR:
		{
			Door *door =(Door *) tar;
			return !door->IsOpen() == !parameters->int0Parameter;
		}
		case ST_CONTAINER:
		{
			Container *cont = (Container *) tar;
			return !(cont->Flags&CONT_LOCKED) == !parameters->int0Parameter;
		}
		default:; //to remove a warning
	}
	printf("[GameScript]: couldn't find door/container:%s\n",parameters->string0Parameter);
	return 0;
}

int GameScript::IsLocked(Scriptable * Sender, Trigger *parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		printf("[GameScript]: couldn't find door/container:%s\n",parameters->objectParameter->objectName);
		return 0;
	}
	switch(tar->Type) {
		case ST_DOOR:
		{
			Door *door =(Door *) tar;
			return !!(door->Flags&DOOR_LOCKED);
		}
		case ST_CONTAINER:
		{
			Container *cont = (Container *) tar;
			return !!(cont->Flags&CONT_LOCKED);
		}
		default:; //to remove a warning
	}
	printf("[GameScript]: couldn't find door/container:%s\n",parameters->string0Parameter);
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
	return actor->GetStat(IE_LEVEL) == (unsigned) parameters->int0Parameter;
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
	
	if (!ID_ClassMask( actor, parameters->int0Parameter) )
		return 0;
	return actor->GetStat(IE_LEVEL) == (unsigned) parameters->int1Parameter;
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
	return actor->GetStat(IE_LEVEL) > (unsigned) parameters->int0Parameter;
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
	if (!ID_ClassMask( actor, parameters->int0Parameter) )
		return 0;
	return actor->GetStat(IE_LEVEL) > (unsigned) parameters->int1Parameter;
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
	return actor->GetStat(IE_LEVEL) < (unsigned) parameters->int0Parameter;
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
	if (!ID_ClassMask( actor, parameters->int0Parameter) )
		return 0;
	return actor->GetStat(IE_LEVEL) < (unsigned) parameters->int1Parameter;
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
	return actor->GetStat(IE_UNSELECTABLE) == (unsigned) parameters->int0Parameter;
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
	return actor->GetStat(IE_UNSELECTABLE) > (unsigned) parameters->int0Parameter;
}

int GameScript::UnselectableVariableLT(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	return actor->GetStat(IE_UNSELECTABLE) < (unsigned) parameters->int0Parameter;
}

int GameScript::AreaCheck(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_ACTOR) {
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

int GameScript::EntirePartyOnMap(Scriptable* /*Sender*/, Trigger* /*parameters*/)
{
	Game *game=core->GetGame();
	int i=game->GetPartySize(false);
	while (i--) {
		Actor *actor=game->GetPC(i);
		if (strnicmp(game->CurrentArea, actor->Area, 8) ) return 0;
	}
	return 1;
}

int GameScript::AnyPCOnMap(Scriptable* /*Sender*/, Trigger* /*parameters*/)
{
	Game *game=core->GetGame();
	int i=game->GetPartySize(false);
	while (i--) {
		Actor *actor=game->GetPC(i);
		if (!strnicmp(game->CurrentArea, actor->Area, 8) ) return 1;
	}
	return 0;
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

int GameScript::InMyArea(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type != ST_ACTOR) {
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

int GameScript::AreaType(Scriptable* Sender, Trigger* parameters)
{
	Map *map=Sender->GetCurrentArea();
	return (map->AreaType&parameters->int0Parameter)>0;
}

int GameScript::IsExtendedNight( Scriptable* Sender, Trigger* /*parameters*/)
{
	Map *map=Sender->GetCurrentArea();
	if (map->AreaType&AT_EXTENDED_NIGHT) {
		return 1;
	}
	return 0;
}

int GameScript::AreaFlag(Scriptable* Sender, Trigger* parameters)
{
	Map *map=Sender->GetCurrentArea();
	return (map->AreaFlags&parameters->int0Parameter)>0;
}

int GameScript::AreaRestDisabled(Scriptable* Sender, Trigger* /*parameters*/)
{
	Map *map=Sender->GetCurrentArea();
	if (map->AreaFlags&2) {
		return 1;
	}
	return 0;
}

int GameScript::TargetUnreachable(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 1; //well, if it doesn't exist it is unreachable
	}
	Map *map=Sender->GetCurrentArea();
	return map->TargetUnreachable( Sender->Pos, tar->Pos );
}

int GameScript::PartyCountEQ(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->GetPartySize(0)<parameters->int0Parameter;
}

int GameScript::PartyCountLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->GetPartySize(0)<parameters->int0Parameter;
}

int GameScript::PartyCountGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->GetPartySize(0)>parameters->int0Parameter;
}

int GameScript::PartyCountAliveEQ(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->GetPartySize(1)<parameters->int0Parameter;
}

int GameScript::PartyCountAliveLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->GetPartySize(1)<parameters->int0Parameter;
}

int GameScript::PartyCountAliveGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->GetPartySize(1)>parameters->int0Parameter;
}

int GameScript::LevelParty(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->GetPartyLevel(1)<parameters->int0Parameter;
}

int GameScript::LevelPartyLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->GetPartyLevel(1)<parameters->int0Parameter;
}

int GameScript::LevelPartyGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->GetPartyLevel(1)>parameters->int0Parameter;
}

int GameScript::PartyGold(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->PartyGold == (ieDword) parameters->int0Parameter;
}

int GameScript::PartyGoldGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->PartyGold > (ieDword) parameters->int0Parameter;
}

int GameScript::PartyGoldLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->PartyGold < (ieDword) parameters->int0Parameter;
}

int GameScript::OwnsFloaterMessage(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	return tar->textDisplaying;
}

int GameScript::GlobalAndGlobal_Trigger(Scriptable* Sender, Trigger* parameters)
{
	ieDword value1 = CheckVariable( Sender, parameters->string0Parameter );
	ieDword value2 = CheckVariable( Sender, parameters->string1Parameter );
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
	return (signed) actor->GetStat(IE_PROFICIENCYBASTARDSWORD+idx) == parameters->int1Parameter;
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
	return (signed) actor->GetStat(IE_PROFICIENCYBASTARDSWORD+idx) > parameters->int1Parameter;
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
	return (signed) actor->GetStat(IE_PROFICIENCYBASTARDSWORD+idx) < parameters->int1Parameter;
}

//this is a PST specific stat, shows how many free proficiency slots we got
//we use an unused stat for it
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
	return (signed) actor->GetStat(IE_FREESLOTS) == parameters->int0Parameter;
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
	return (signed) actor->GetStat(IE_FREESLOTS) > parameters->int0Parameter;
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
	return (signed) actor->GetStat(IE_FREESLOTS) < parameters->int0Parameter;
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
	return (signed) actor->GetStat(IE_INTERNAL_0+idx) == parameters->int1Parameter;
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
	return (signed) actor->GetStat(IE_INTERNAL_0+idx) > parameters->int1Parameter;
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
	return (signed) actor->GetStat(IE_INTERNAL_0+idx) < parameters->int1Parameter;
}

//we check if target is currently in dialog or not
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
	GameControl *gc = core->GetGameControl();
	if ( (actor != gc->target) && (actor != gc->speaker) ) {
		return 1;
	}
	return 0;
}

//this one checks scriptname (deathvar), i hope it is right
//IsScriptName depends on this too
//Name is another (similar function)
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
	if (stricmp(actor->GetScriptName(), parameters->string0Parameter) ) {
		return 0;
	}
	return 1;
}

//This is checking on the character's name as it was typed in
int GameScript::CharName(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objectParameter );
	if (!scr || scr->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = (Actor *) scr;
	if (!strnicmp(actor->ShortName, parameters->string0Parameter, 32) ) {
		return 1;
	}
	return 0;
}

int GameScript::AnimationID(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	if (tar->Type != ST_ACTOR) {
		return 0;
	}
	Actor *actor = (Actor *) tar;
	if ((ieWord) actor->GetStat(IE_ANIMATION_ID) == (ieWord) parameters->int0Parameter) {
		return 1;
	}
	return 0;
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
	return actor->GetStance() == parameters->int0Parameter;
}

int GameScript::Time(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->GameTime == (ieDword) parameters->int0Parameter;
}

int GameScript::TimeGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->GameTime > (ieDword) parameters->int0Parameter;
}
int GameScript::TimeLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->GameTime < (ieDword) parameters->int0Parameter;
}

int GameScript::HotKey(Scriptable* /*Sender*/, Trigger* parameters)
{
	int ret=core->GetGameControl()->HotKey==parameters->int0Parameter;
	//probably we need to implement a trigger mechanism, clear
	//the hotkey only when the triggerblock was evaluated as true
	if (ret) {
		core->GetGameControl()->HotKey=0;
	}
	return ret;
}

int GameScript::CombatCounter(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->CombatCounter == (ieDword) parameters->int0Parameter;
}

int GameScript::CombatCounterGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->CombatCounter > (ieDword) parameters->int0Parameter;
}

int GameScript::CombatCounterLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	return core->GetGame()->CombatCounter < (ieDword) parameters->int0Parameter;
}

int GameScript::TrapTriggered(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_TRIGGER) {
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
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type != ST_ACTOR) {
		return 0;
	}
	GameControl *gc = core->GetGameControl();
	if (Sender != gc->target && Sender!=gc->speaker) {
		return 0;
	}
	if (tar != gc->target && tar!=gc->speaker) {
		return 0;
	}
	return 1;
}

int GameScript::IsRotation(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if ( actor->GetOrientation() == parameters->int0Parameter ) {
		return 1;
	}
	return 0;
}

int GameScript::IsFacingSavedRotation(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	ieDword value;
	if (!parameters->string0Parameter[0]) {
		strcpy(parameters->string0Parameter,"LOCALSsavedlocation");
	}
	value = (ieDword) CheckVariable( tar, parameters->string0Parameter );
	Point p = { *(unsigned short *) &value, *(((unsigned short *) &value)+1) };
	if (actor->GetOrientation() == GetOrient( p, actor->Pos ) ) {
		return 1;
	}
	return 0;
}

int GameScript::TookDamage(Scriptable* Sender, Trigger* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) Sender;
	if (actor->LastDamage) {
//mark actor to clear this trigger?
		return 1;
	}
	return 0;
}

int GameScript::DamageTaken(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) Sender;
	if (actor->LastDamage==parameters->int0Parameter) {
//mark actor to clear this trigger?
		return 1;
	}
	return 0;
}

int GameScript::DamageTakenGT(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) Sender;
	if (actor->LastDamage>parameters->int0Parameter) {
//mark actor to clear this trigger?
		return 1;
	}
	return 0;
}

int GameScript::DamageTakenLT(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) Sender;
	if (actor->LastDamage<parameters->int0Parameter) {
//mark actor to clear this trigger?
		return 1;
	}
	return 0;
}

int GameScript::HitBy(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) Sender;
	if (parameters->int0Parameter) {
		if (!(parameters->int0Parameter&actor->LastDamageType) ) {
			return 0;
		}
	}
	return MatchActor(Sender, actor->LastHitter, parameters->objectParameter);
}

int GameScript::Heard(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) Sender;
	if (parameters->int0Parameter) {
		if (!(parameters->int0Parameter&actor->LastShout) ) {
			return 0;
		}
	}
	return MatchActor(Sender, actor->LastHeard, parameters->objectParameter);
}

int GameScript::LastMarkedObject_Trigger(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) Sender;
	return MatchActor(Sender, actor->LastSeen, parameters->objectParameter);
}

int GameScript::Help_Trigger(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) Sender;
	return MatchActor(Sender, actor->LastHelp, parameters->objectParameter);
}

int GameScript::FallenPaladin(Scriptable* Sender, Trigger* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* act = ( Actor* ) Sender;
	return (act->GetStat(IE_MC_FLAGS) & MC_FALLEN_PALADIN)!=0;
}

int GameScript::FallenRanger(Scriptable* Sender, Trigger* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* act = ( Actor* ) Sender;
	return (act->GetStat(IE_MC_FLAGS) & MC_FALLEN_RANGER)!=0;
}

int GameScript::Difficulty(Scriptable* /*Sender*/, Trigger* parameters)
{
	ieDword diff;

	core->GetDictionary()->Lookup("Difficulty Level", diff);
	return diff==(ieDword) parameters->int0Parameter;
}

int GameScript::DifficultyGT(Scriptable* /*Sender*/, Trigger* parameters)
{
	ieDword diff;

	core->GetDictionary()->Lookup("Difficulty Level", diff);
	return diff>(ieDword) parameters->int0Parameter;
}

int GameScript::DifficultyLT(Scriptable* /*Sender*/, Trigger* parameters)
{
	ieDword diff;

	core->GetDictionary()->Lookup("Difficulty Level", diff);
	return diff<(ieDword) parameters->int0Parameter;
}

int GameScript::Vacant(Scriptable* Sender, Trigger* /*parameters*/)
{
	if (Sender->Type!=ST_AREA) {
		return 0;
	}
	Map *map = (Map *) Sender;
	if ( map->CanFree() ) return 1;
	return 0;
}

int GameScript::InWeaponRange(Scriptable* Sender, Trigger* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return 0;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar) {
		return 0;
	}
	Actor *actor = (Actor *) Sender;
	unsigned int wrange = actor->GetWeaponRange() * 10;
	if ( Distance( Sender, tar ) <= wrange ) {
		return 1;
	}
	return 0;
}

int GameScript::HasWeaponEquipped(Scriptable* Sender, Trigger* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objectParameter );
	if (!tar || tar->Type!=ST_ACTOR) {
		return 0;
	}
	Actor* actor = ( Actor* ) tar;
	if (actor->inventory.GetEquippedSlot() == IW_NO_EQUIPPED) {
		return 0;
	}
	return 1;
}

int GameScript::PCInStore( Scriptable* /*Sender*/, Trigger* /*parameters*/)
{
	if (core->GetCurrentStore()) {
		return 1;
	}
	return 0;
}

//this checks if the launch point is onscreen, a more elaborate check
//would see if any piece of the Scriptable is onscreen, what is the original
//behaviour?
int GameScript::OnScreen( Scriptable* Sender, Trigger* /*parameters*/)
{
	Region vp = core->GetVideoDriver()->GetViewport();
	if (vp.PointInside(Sender->Pos) ) {
		return 1;
	}
	return 0;
}

//-------------------------------------------------------------
// Action Functions
//-------------------------------------------------------------

void GameScript::SetExtendedNight(Scriptable* Sender, Action* parameters)
{
	Map *map=Sender->GetCurrentArea();
	//sets the 'can rest other' bit
	if (parameters->int0Parameter) {
		map->AreaType|=AT_EXTENDED_NIGHT;
	}
	else {
		map->AreaType&=~AT_EXTENDED_NIGHT;
	}
}

void GameScript::SetAreaRestFlag(Scriptable* Sender, Action* parameters)
{
	Map *map=Sender->GetCurrentArea();
	//sets the 'can rest other' bit
	if (parameters->int0Parameter) {
		map->AreaType|=AT_CAN_REST;
	}
	else {
		map->AreaType&=~AT_CAN_REST;
	}
}

void GameScript::AddAreaFlag(Scriptable* Sender, Action* parameters)
{
	Map *map=Sender->GetCurrentArea();
	map->AreaFlags|=parameters->int0Parameter;
}

void GameScript::RemoveAreaFlag(Scriptable* Sender, Action* parameters)
{
	Map *map=Sender->GetCurrentArea();
	map->AreaFlags&=~parameters->int0Parameter;
}

void GameScript::SetAreaFlags(Scriptable* Sender, Action* parameters)
{
	Map *map=Sender->GetCurrentArea();
	ieDword value = map->AreaFlags;
	HandleBitMod( value, parameters->int0Parameter, parameters->int1Parameter);
	map->AreaFlags=value;
}

void GameScript::AddAreaType(Scriptable* Sender, Action* parameters)
{
	Map *map=Sender->GetCurrentArea();
	map->AreaType|=parameters->int0Parameter;
}

void GameScript::RemoveAreaType(Scriptable* Sender, Action* parameters)
{
	Map *map=Sender->GetCurrentArea();
	map->AreaType&=~parameters->int0Parameter;
}

void GameScript::NoActionAtAll(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	//thats all :)
}

void GameScript::NoAction(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	//thats all :)
}

void GameScript::SG(Scriptable* Sender, Action* parameters)
{
	SetVariable( Sender, parameters->string0Parameter, "GLOBAL", parameters->int0Parameter );
}

void GameScript::SetGlobal(Scriptable* Sender, Action* parameters)
{
	SetVariable( Sender, parameters->string0Parameter, parameters->int0Parameter );
}

void GameScript::SetGlobalRandom(Scriptable* Sender, Action* parameters)
{
	unsigned int max=parameters->int0Parameter+1;
	if (max) {
		SetVariable( Sender, parameters->string0Parameter,
			RandomNumValue%max );
	} else {
		SetVariable( Sender, parameters->string0Parameter,
			RandomNumValue );
	}
}

void GameScript::SetGlobalTimer(Scriptable* Sender, Action* parameters)
{
	ieDword mytime;

	mytime=core->GetGame()->GameTime; //gametime (should increase it)
	SetVariable( Sender, parameters->string0Parameter,
		parameters->int0Parameter + mytime);
}

void GameScript::RealSetGlobalTimer(Scriptable* Sender, Action* parameters)
{
	ieDword mytime;

	GetTime(mytime); //this is real time
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
	if (!scr || scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->SetStat( IE_RACE, parameters->int0Parameter );
}

void GameScript::ChangeClass(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[1] );
	if (!scr || scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->SetStat( IE_CLASS, parameters->int0Parameter );
}

void GameScript::SetNamelessClass(Scriptable* /*Sender*/, Action* parameters)
{
	//same as Protagonist
	Actor* actor = core->GetGame()->FindPC(1);
	actor->SetStat( IE_CLASS, parameters->int0Parameter );
}

void GameScript::SetNamelessDisguise(Scriptable* Sender, Action* parameters)
{
	SetVariable(Sender, "APPEARANCE", "GLOBAL", parameters->int0Parameter);
//maybe add a guiscript call here ?
	if (parameters->int0Parameter) {
	}
	else {
		
	}
}

void GameScript::ChangeSpecifics(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[1] );
	if (!scr || scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
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
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[1] );
	if (!scr || scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->SetStat( IE_SEX, parameters->int0Parameter );
}

void GameScript::ChangeAlignment(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[1] );
	if (!scr || scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->SetStat( IE_ALIGNMENT, parameters->int0Parameter );
}

void GameScript::SetFaction(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[1] );
	if (!scr || scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->SetStat( IE_FACTION, parameters->int0Parameter );
}

void GameScript::SetHP(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[1] );
	if (!scr || scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->SetStat( IE_HITPOINTS, parameters->int0Parameter );
}

void GameScript::SetTeam(Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[1] );
	if (!scr || scr->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) scr;
	actor->SetStat( IE_TEAM, parameters->int0Parameter );
}

void GameScript::TriggerActivation(Scriptable* Sender, Action* parameters)
{
	Scriptable* ip;

	if (!parameters->objects[1]->objectName[0]) {
		ip=Sender;
	} else {
		ip = Sender->GetCurrentArea()->TMap->GetInfoPoint(parameters->objects[1]->objectName);
	}
	if (!ip) {
		printf("Script error: No Trigger Named \"%s\"\n", parameters->objects[1]->objectName);
		return;
	}
	if ( parameters->int0Parameter != 0 ) {
		ip->Active |= SCR_ACTIVE;
	} else {
		ip->Active &=~SCR_ACTIVE;
	}
}

void GameScript::FadeToColor(Scriptable* /*Sender*/, Action* parameters)
{
	core->timer->SetFadeToColor( parameters->pointParameter.x );
}

void GameScript::FadeFromColor(Scriptable* /*Sender*/, Action* parameters)
{
	core->timer->SetFadeFromColor( parameters->pointParameter.x );
}

void GameScript::JumpToPoint(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* ab = ( Actor* ) Sender;
	Map *map = Sender->GetCurrentArea();
	ab->SetPosition( map, parameters->pointParameter, true );
}

void GameScript::JumpToPointInstant(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	Actor* ab = ( Actor* ) tar;
	Map *map = Sender->GetCurrentArea();
	ab->SetPosition( map, parameters->pointParameter, true );
}

/** instant jump to location saved in variable, default: savedlocation */
/** default subject is the current actor */
void GameScript::JumpToSavedLocation(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		tar = Sender;
	}
	if (tar->Type != ST_ACTOR) {
		return;
	}
	if (!parameters->string0Parameter[0]) {
		strcpy(parameters->string0Parameter,"LOCALSsavedlocation");
	}
	ieDword value = (ieDword) CheckVariable( Sender, parameters->string0Parameter );
	parameters->pointParameter.x = *(unsigned short *) &value;
	parameters->pointParameter.y = *(((unsigned short *) &value)+1);
	Actor* ab = ( Actor* ) tar;
	Map *map = Sender->GetCurrentArea();
	ab->SetPosition( map, parameters->pointParameter, true );
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
	ieResRef Area;

	if (tar->Type == ST_ACTOR) {
		Actor *ac = ( Actor* ) tar;
		memcpy(Area, ac->Area, 9);
	}
	else {
		Area[0]=0;
	}
	if (parameters->string0Parameter[0]) {
		CreateVisualEffectCore(Sender, Sender->Pos, parameters->string0Parameter);
	}
	MoveBetweenAreasCore( (Actor *) Sender, Area, tar->Pos, -1, true);
}

void GameScript::TeleportParty(Scriptable* /*Sender*/, Action* parameters)
{
	Game *game = core->GetGame();
	int i = game->GetPartySize(false);
	while (i--) {
		Actor *tar = game->GetPC(i);
		MoveBetweenAreasCore( tar, parameters->string1Parameter, 
			parameters->pointParameter, -1, true);
	}
}

void GameScript::MoveGlobalsTo(Scriptable* /*Sender*/, Action* parameters)
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
			parameters->pointParameter, -1, true);
	}
	i = game->GetNPCCount();
	while (i--) {
		Actor *tar = game->GetNPC(i);
		//if the actor isn't in the area, we don't care
		if (strnicmp(tar->Area, parameters->string0Parameter,8) ) {
			continue;
		}
		MoveBetweenAreasCore( tar, parameters->string1Parameter, 
			parameters->pointParameter, -1, true);
	}
}

void GameScript::MoveGlobal(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	MoveBetweenAreasCore( (Actor *) tar, parameters->string0Parameter,
		parameters->pointParameter, -1, true);
}

//we also allow moving to door, container
void GameScript::MoveGlobalObject(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	Scriptable* to = GetActorFromObject( Sender, parameters->objects[2] );
	if (!to) {
		return;
	}
	MoveBetweenAreasCore( (Actor *) tar, parameters->string0Parameter,
		to->Pos, -1, true);
}

void GameScript::MoveGlobalObjectOffScreen(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	Scriptable* to = GetActorFromObject( Sender, parameters->objects[2] );
	if (!to) {
		return;
	}
	MoveBetweenAreasCore( (Actor *) tar, parameters->string0Parameter,
		to->Pos, -1, false);
}

int GameScript::GetHappiness(Scriptable* Sender, int reputation)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	Actor* ab = ( Actor* ) Sender;
	int alignment = ab->GetStat(IE_ALIGNMENT)&3; //good, neutral, evil
	if (reputation>19) {
		reputation=19;
	}
	return happiness[alignment][reputation/10];
}

int GameScript::GetHPPercent(Scriptable* Sender)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	Actor* ab = ( Actor* ) Sender;
	int hp1 = ab->GetStat(IE_MAXHITPOINTS);
	if (hp1<1) {
		return 0;
	}
	int hp2 = ab->GetStat(IE_HITPOINTS);
	if (hp2<1) {
		return 0;
	}
	return hp2*100/hp1;
}

void GameScript::CreateCreatureCore(Scriptable* Sender, Action* parameters,
	int flags)
{
	ActorMgr* aM = ( ActorMgr* ) core->GetInterface( IE_CRE_CLASS_ID );
	DataStream* ds;

	if (flags & CC_STRING1) {
		ds = core->GetResourceMgr()->GetResource( parameters->string1Parameter, IE_CRE_CLASS_ID );
	}
	else {
		ds = core->GetResourceMgr()->GetResource( parameters->string0Parameter, IE_CRE_CLASS_ID );
	}
	aM->Open( ds, true );
	Actor* ab = aM->GetActor();
	core->FreeInterface( aM );
	int radius;
	Point pnt;

	radius=0;
	switch (flags & CC_MASK) {
		case CC_OFFSCREEN:
			{
			Region vp = core->GetVideoDriver()->GetViewport();
			radius=vp.w/2; //actually it must be further divided by the tile size, hmm 16?
			}
			//falling through
		case CC_OBJECT://use object + offset
			{
			Scriptable *tmp = GetActorFromObject( Sender, parameters->objects[1] );
			if (tmp) Sender=tmp;
			}
			//falling through
		case CC_OFFSET://use sender + offset
			pnt.x = parameters->pointParameter.x+Sender->Pos.x;
			pnt.y = parameters->pointParameter.y+Sender->Pos.y;
			break;
		default: //absolute point, but -1,-1 means AtFeet
			pnt.x = parameters->pointParameter.x;
			pnt.y = parameters->pointParameter.y;
			if ((pnt.x==-1) && (pnt.y==-1)) {
				pnt.x = Sender->Pos.x;
				pnt.y = Sender->Pos.y;
			}
			break;
	}
	printf("CreateCreature: %s at [%d.%d] face:%d\n",parameters->string0Parameter, pnt.x,pnt.y,parameters->int0Parameter);
	Map *map = Sender->GetCurrentArea();
	ab->SetPosition(map, pnt, flags&CC_CHECK_IMPASSABLE, radius );
	//i think this isn't needed, the creature's stance should be set in
	//the creature, GetActor sets it correctly
	//ab->SetStance( IE_ANI_AWAKE );
	ab->SetOrientation(parameters->int0Parameter, 0 );
	map->AddActor( ab );

	//setting the deathvariable if it exists (iwd2)
	if (parameters->string1Parameter[0]) {
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

void GameScript::CreateCreatureObjectOffScreen(Scriptable* Sender, Action* parameters)
{
	CreateCreatureCore( Sender, parameters, CC_OFFSCREEN | CC_OBJECT | CC_CHECK_IMPASSABLE );
}

void GameScript::StartCutSceneMode(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	core->SetCutSceneMode( true );
}

void GameScript::EndCutSceneMode(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	core->SetCutSceneMode( false );
}

void GameScript::StartCutScene(Scriptable* Sender, Action* parameters)
{
	GameScript* gs = new GameScript( parameters->string0Parameter, ST_GLOBAL );
	gs->MySelf = Sender;
	gs->EvaluateAllBlocks();
	delete( gs );
}

void GameScript::CutSceneID(Scriptable* Sender, Action* parameters)
{
	if (parameters->objects[1]->objectName[0]) {
		Map *map = Sender->GetCurrentArea();
		Sender->CutSceneId = map->GetActor( parameters->objects[1]->objectName );
	} else {
		Sender->CutSceneId = GetActorFromObject( Sender, parameters->objects[1] );
	}
	if (InDebug&2) {
		if (!Sender->CutSceneId) {
			printMessage("GameScript","Failed to set CutSceneID!\n",YELLOW);
		}
	}
}

void GameScript::Enemy(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->SetStat( IE_EA, 255 );
}

void GameScript::Ally(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->SetStat( IE_EA, 4 );
}

/** GemRB extension: you can replace baldur.bcs */
void GameScript::ChangeAIScript(Scriptable* Sender, Action* parameters)
{
	if (parameters->int0Parameter>7) {
		return;
	}
	Sender->SetScript( parameters->string0Parameter, parameters->int0Parameter );
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
	DisplayStringCore( tar, parameters->int0Parameter, DS_HEAD|DS_CONSOLE|DS_CONST);
}

void GameScript::VerbalConstant(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	DisplayStringCore( tar, parameters->int0Parameter, DS_CONSOLE|DS_CONST);
}

/** in IWD2 this has no string parameter, saves the location into a local */
void GameScript::SaveLocation(Scriptable* Sender, Action* parameters)
{
	unsigned int value;

	*((unsigned short *) &value) = parameters->pointParameter.x;
	*(((unsigned short *) &value)+1) = (unsigned short) parameters->pointParameter.y;
	if (!parameters->string0Parameter[0]) {
		strcpy(parameters->string0Parameter,"LOCALSsavedlocation");
	}
	printf("SaveLocation: %s\n",parameters->string0Parameter);
	SetVariable(Sender, parameters->string0Parameter, value);
}

/** in IWD2 this has no string parameter, saves the location into a local */
void GameScript::SaveObjectLocation(Scriptable* Sender, Action* parameters)
{
	unsigned int value;

	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	*((unsigned short *) &value) = tar->Pos.x;
	*(((unsigned short *) &value)+1) = (unsigned short) tar->Pos.y;
	if (!parameters->string0Parameter[0]) {
		strcpy(parameters->string0Parameter,"LOCALSsavedlocation");
	}
	printf("SaveLocation: %s\n",parameters->string0Parameter);
	SetVariable(Sender, parameters->string0Parameter, value);
}

/** you may omit the string0parameter, in this case this will be a */
/** CreateCreatureAtSavedLocation */
void GameScript::CreateCreatureAtLocation(Scriptable* Sender, Action* parameters)
{
	if (!parameters->string0Parameter[0]) {
		strcpy(parameters->string0Parameter,"LOCALSsavedlocation");
	}
	unsigned int value = CheckVariable(Sender, parameters->string0Parameter);
	parameters->pointParameter.y = value & 0xffff;
	parameters->pointParameter.x = value >> 16;
	CreateCreatureCore(Sender, parameters, CC_CHECK_IMPASSABLE|CC_STRING1);
}

void GameScript::WaitRandom(Scriptable* Sender, Action* parameters)
{
	int width = parameters->int1Parameter-parameters->int0Parameter;
	if (width<2) {
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

void GameScript::SmallWaitRandom(Scriptable* Sender, Action* parameters)
{
	int random = parameters->int1Parameter - parameters->int0Parameter;
	if (random<1) {
		random = 1;
	}
	Sender->SetWait( rand()%random + parameters->int0Parameter );
}

void GameScript::MoveViewPoint(Scriptable* /*Sender*/, Action* parameters)
{
	core->MoveViewportTo( parameters->pointParameter.x, parameters->pointParameter.y, true );
}

void GameScript::MoveViewObject(Scriptable* Sender, Action* parameters)
{
	Scriptable * scr = GetActorFromObject( Sender, parameters->objects[1]);
	core->MoveViewportTo( scr->Pos.x, scr->Pos.y, true );
}

void GameScript::AddWayPoint(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->AddWayPoint( parameters->pointParameter );
}

void GameScript::MoveToPointNoRecticle(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->InternalFlags|=IF_NORECTICLE;
	actor->WalkTo( parameters->pointParameter );
}

void GameScript::MoveToPointNoInterrupt(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->InternalFlags|=IF_NOINT;
	actor->WalkTo( parameters->pointParameter );
}

void GameScript::MoveToPoint(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->WalkTo( parameters->pointParameter );
}
/** this function extends bg2: movetosavedlocationn (sic), */
/** iwd2 returntosavedlocation (with default variable) */
/** use Sender as default subject */
void GameScript::MoveToSavedLocation(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		tar = Sender;
	}
	if (tar->Type != ST_ACTOR) {
		return;
	}
	
	if (!parameters->string0Parameter[0]) {
		strcpy(parameters->string0Parameter,"LOCALSsavedlocation");
	}
	ieDword value = (ieDword) CheckVariable( Sender, parameters->string0Parameter );
	parameters->pointParameter.x = *(unsigned short *) &value;
	parameters->pointParameter.y = *(((unsigned short *) &value)+1);
	Actor* actor = ( Actor* ) tar;
	actor->WalkTo( parameters->pointParameter );
}

void GameScript::MoveToSavedLocationDelete(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		tar = Sender;
	}
	if (tar->Type != ST_ACTOR) {
		return;
	}
	
	if (!parameters->string0Parameter[0]) {
		strcpy(parameters->string0Parameter,"LOCALSsavedlocation");
	}
	ieDword value = (ieDword) CheckVariable( Sender, parameters->string0Parameter );
	SetVariable( Sender, parameters->string0Parameter, 0);
	parameters->pointParameter.x = *(unsigned short *) &value;
	parameters->pointParameter.y = *(((unsigned short *) &value)+1);
	Actor* actor = ( Actor* ) tar;
	actor->WalkTo( parameters->pointParameter );
}

void GameScript::MoveToObjectNoInterrupt(Scriptable* Sender, Action* parameters)
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
	actor->InternalFlags|=IF_NOINT;
	actor->WalkTo( target->Pos );
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
	actor->WalkTo( target->Pos );
}

void GameScript::StorePartyLocation(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	Game *game = core->GetGame();
	for (int i = 0; i < game->GetPartySize(0); i++) {
		Actor* act = game->GetPC( i );
		if (act) {
			ieDword value;
			*((unsigned short *) &value) = act->Pos.x;
			*(((unsigned short *) &value)+1) = act->Pos.y;
			SetVariable( act, "LOCALSsavedlocation", value);
		}
	}

}

void GameScript::RestorePartyLocation(Scriptable* Sender, Action* /*parameters*/)
{
	Game *game = core->GetGame();
	for (int i = 0; i < game->GetPartySize(0); i++) {
		Actor* act = game->GetPC( i );
		if (act) {
			ieDword value=CheckVariable( act, "LOCALSsavedlocation");
			Map *map = Sender->GetCurrentArea();
			//setting position, don't put actor on another actor
			Point p = { *((unsigned short *) &value),
				*(((unsigned short *) &value)+1) };
			act->SetPosition( map, p , -1 );
		}
	}

}

void GameScript::MoveToCenterOfScreen(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	Region vp = core->GetVideoDriver()->GetViewport();
	Actor* actor = ( Actor* ) Sender;
	Point p = {vp.x+vp.w/2, vp.y+vp.h/2};
	actor->WalkTo( p );
}

void GameScript::MoveToOffset(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	Point p = {Sender->Pos.x+parameters->pointParameter.x, Sender->Pos.y+parameters->pointParameter.y};
	actor->WalkTo( p );
}

void GameScript::RunAwayFrom(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	actor->RunAwayFrom( tar->Pos, parameters->int0Parameter, false);
}

void GameScript::RunAwayFromNoInterrupt(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	actor->InternalFlags|=IF_NOINT;
	actor->RunAwayFrom( tar->Pos, parameters->int0Parameter, false);
}

void GameScript::RunAwayFromPoint(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->RunAwayFrom( parameters->pointParameter, parameters->int0Parameter, false);
}

void GameScript::DisplayStringNoName(Scriptable* Sender, Action* parameters)
{
	DisplayStringCore( Sender, parameters->int0Parameter, DS_CONSOLE|DS_NONAME);
}

void GameScript::DisplayStringNoNameHead(Scriptable* Sender, Action* parameters)
{
	DisplayStringCore( Sender, parameters->int0Parameter, DS_HEAD|DS_CONSOLE|DS_NONAME);
}

//display message over current script owner
void GameScript::DisplayMessage(Scriptable* Sender, Action* parameters)
{
	DisplayStringCore(Sender, parameters->int0Parameter, DS_CONSOLE );
}

//float message over target
void GameScript::DisplayStringHead(Scriptable* Sender, Action* parameters)
{
	Scriptable* target = GetActorFromObject( Sender, parameters->objects[1] );
	if (!target) {
		target=Sender;
		printf("DisplayStringHead/FloatMessage got no target, assuming Sender!\n");
	}

	DisplayStringCore(target, parameters->int0Parameter, DS_CONSOLE|DS_HEAD );
}

void GameScript::KillFloatMessage(Scriptable* Sender, Action* parameters)
{
	Scriptable* target = GetActorFromObject( Sender, parameters->objects[1] );
	if (!target) {
		target=Sender;
	}
	target->DisplayHeadText(NULL);
}

void GameScript::DisplayStringHeadOwner(Scriptable* /*Sender*/, Action* parameters)
{
	Actor *actor;
	Game *game=core->GetGame();

	//there is an assignment here
	for (int i=0; (actor = game->GetPC(i)) ; i++) {
		if (actor->inventory.HasItem(parameters->string0Parameter,parameters->int0Parameter) ) {
			DisplayStringCore(actor, parameters->int0Parameter, DS_CONSOLE|DS_HEAD );
		}
	}
}

void GameScript::FloatMessageFixed(Scriptable* Sender, Action* parameters)
{
	Scriptable* target = GetActorFromObject( Sender, parameters->objects[1] );
	if (!target) {
		target=Sender;
		printf("DisplayStringHead/FloatMessage got no target, assuming Sender!\n");
	}

	DisplayStringCore(target, parameters->int0Parameter, DS_CONSOLE|DS_HEAD);
}

void GameScript::FloatMessageFixedRnd(Scriptable* Sender, Action* parameters)
{
	Scriptable* target = GetActorFromObject( Sender, parameters->objects[1] );
	if (!target) {
		target=Sender;
		printf("DisplayStringHead/FloatMessage got no target, assuming Sender!\n");
	}

	SrcVector *rndstr=LoadSrc(parameters->string0Parameter);
	if (!rndstr) {
		printMessage("GameScript","Cannot display resource!",LIGHT_RED);
		return;
	}
	DisplayStringCore(target, rndstr->at(rand()%rndstr->size()), DS_CONSOLE|DS_HEAD);
	FreeSrc(rndstr, parameters->string0Parameter);
}

void GameScript::FloatMessageRnd(Scriptable* Sender, Action* parameters)
{
	Scriptable* target = GetActorFromObject( Sender, parameters->objects[1] );
	if (!target) {
		target=Sender;
		printf("DisplayStringHead/FloatMessage got no target, assuming Sender!\n");
	}

	SrcVector *rndstr=LoadSrc(parameters->string0Parameter);
	if (!rndstr) {
		printMessage("GameScript","Cannot display resource!",LIGHT_RED);
		return;
	}
	DisplayStringCore(target, rndstr->at(rand()%rndstr->size()), DS_CONSOLE|DS_HEAD);
	FreeSrc(rndstr, parameters->string0Parameter);
}

void GameScript::DisplayString(Scriptable* Sender, Action* parameters)
{
	DisplayStringCore( Sender, parameters->int0Parameter, DS_CONSOLE);
}

void GameScript::DisplayStringWait(Scriptable* Sender, Action* parameters)
{
	DisplayStringCore( Sender, parameters->int0Parameter, DS_HEAD|DS_WAIT);
}

void GameScript::ForceFacing(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		Sender->CurrentAction = NULL;
		return;
	}
	Actor *actor = (Actor *) tar;
	actor->SetOrientation(parameters->int0Parameter,0 );
}

/* A -1 means random facing? */
void GameScript::Face(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	if (parameters->int0Parameter==-1) {
		actor->SetOrientation(core->Roll(1,MAX_ORIENT,-1),0 );
	} else {
		actor->SetOrientation(parameters->int0Parameter,0);
	}
	actor->resetAction = true;
	actor->SetWait( 1 );
}

void GameScript::FaceObject(Scriptable* Sender, Action* parameters)
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
	actor->SetOrientation( GetOrient( target->Pos, actor->Pos ),0 );
	actor->resetAction = true;
	actor->SetWait( 1 );
}

void GameScript::FaceSavedLocation(Scriptable* Sender, Action* parameters)
{
	Scriptable* target = GetActorFromObject( Sender, parameters->objects[1] );
	if (!target) {
		Sender->CurrentAction = NULL;
		return;
	}
	Actor* actor = ( Actor* ) target;
	ieDword value;
	if (!parameters->string0Parameter[0]) {
		strcpy(parameters->string0Parameter,"LOCALSsavedlocation");
	}
	value = (ieDword) CheckVariable( target, parameters->string0Parameter );
	Point p = { *(unsigned short *) &value, *(((unsigned short *) &value)+1)};
	actor->SetOrientation ( GetOrient( p, actor->Pos ),0 );
	actor->resetAction = true;
	actor->SetWait( 1 );
}

/*pst and bg2 can play a song designated by index*/
/*actually pst has some extra params not currently implemented*/
/*switchplaylist could implement fade */
void GameScript::StartSong(Scriptable* /*Sender*/, Action* parameters)
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

void GameScript::StartMusic(Scriptable* Sender, Action* parameters)
{
	Map *map = Sender->GetCurrentArea();
	map->PlayAreaSong(parameters->int0Parameter);
}

void GameScript::BattleSong(Scriptable* Sender, Action* /*parameters*/)
{
	Map *map = Sender->GetCurrentArea();
	map->PlayAreaSong(3); //battlesong is in slot 3
}

/*iwd2 can set an areasong slot*/
void GameScript::SetMusic(Scriptable* Sender, Action* parameters)
{
	//iwd2 seems to have 10 slots, dunno if it is important
	if (parameters->int0Parameter>4) return;
	Map *map = Sender->GetCurrentArea();
	map->SongHeader.SongList[parameters->int0Parameter]=parameters->int1Parameter;
}

//optional integer parameter (isSpeech)
void GameScript::PlaySound(Scriptable* Sender, Action* parameters)
{
	printf( "PlaySound(%s)\n", parameters->string0Parameter );
	core->GetSoundMgr()->Play( parameters->string0Parameter, Sender->Pos.x,
				Sender->Pos.y, parameters->int0Parameter );
}

void GameScript::PlaySoundPoint(Scriptable* /*Sender*/, Action* parameters)
{
	printf( "PlaySound(%s)\n", parameters->string0Parameter );
	core->GetSoundMgr()->Play( parameters->string0Parameter, parameters->pointParameter.x, parameters->pointParameter.y );
}

void GameScript::PlaySoundNotRanged(Scriptable* /*Sender*/, Action* parameters)
{
	printf( "PlaySound(%s)\n", parameters->string0Parameter );
	core->GetSoundMgr()->Play( parameters->string0Parameter, 0, 0, 0);
}

void GameScript::Continue(Scriptable* /*Sender*/, Action* /*parameters*/)
{
}

void GameScript::CreateVisualEffectCore(Scriptable *Sender, Point &position, const char *effect)
{
//TODO: add engine specific VVC replacement methods
//stick to object flag, sounds, iterations etc.
	ScriptedAnimation* vvc = core->GetScriptedAnimation(effect, position);
	Sender->GetCurrentArea( )->AddVVCCell( vvc );
}

void GameScript::CreateVisualEffectObject(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	CreateVisualEffectCore(tar, tar->Pos, parameters->string0Parameter);
}

void GameScript::CreateVisualEffect(Scriptable* Sender, Action* parameters)
{
	CreateVisualEffectCore(Sender, parameters->pointParameter, parameters->string0Parameter);
}

void GameScript::DestroySelf(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Sender->ClearActions();
	Actor* actor = ( Actor* ) Sender;
	actor->InternalFlags |= IF_CLEANUP;
}

void GameScript::ScreenShake(Scriptable* Sender, Action* parameters)
{
	if (parameters->int1Parameter) { //IWD2 has a different profile
		core->timer->SetScreenShake( parameters->int1Parameter,
			parameters->int2Parameter, parameters->int0Parameter );
	}
	else {
		core->timer->SetScreenShake( parameters->pointParameter.x,
			parameters->pointParameter.y, parameters->int0Parameter );
	}
	Sender->SetWait( parameters->int0Parameter );
}

void GameScript::UnhideGUI(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	Game* game = core->GetGame();
	game->SetControlStatus(CS_HIDEGUI, BM_NAND);
	//core->SetCutSceneMode( false );
}

void GameScript::HideGUI(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	Game* game = core->GetGame();
	game->SetControlStatus(CS_HIDEGUI, BM_OR);
}

void GameScript::LockScroll(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	GameControl* gc = core->GetGameControl();
	if (gc) {
		gc->SetScreenFlags(SF_LOCKSCROLL, BM_OR);
	}
}

void GameScript::UnlockScroll(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	GameControl* gc = core->GetGameControl();
	if (gc) {
		gc->SetScreenFlags(SF_LOCKSCROLL, BM_NAND);
	}
}

static const char *GetDialog(Scriptable* scr)
{
	switch(scr->Type) {
		case ST_CONTAINER: case ST_DOOR:
		case ST_PROXIMITY: case ST_TRAVEL: case ST_TRIGGER:
			return ((Highlightable *) scr)->GetDialog();
		case ST_ACTOR:
			return ((Actor *) scr)->GetDialog(true);
		case ST_GLOBAL:case ST_AREA:
			return NULL;
	}
	return NULL;
}

static ieResRef PlayerDialogRes = "PLAYERx\0";

void GameScript::BeginDialog(Scriptable* Sender, Action* parameters, int Flags)
{
	Scriptable* tar, *scr;

	if (InDebug&4) {
		printf("BeginDialog core\n");
	}
	if (Flags & BD_OWN) {
		scr = tar = GetActorFromObject( Sender, parameters->objects[1] );
	} else {
		if (Flags & BD_NUMERIC) {
			//the target was already set, this is a crude hack
			tar = core->GetGameControl()->target;
		}
		else {
			tar = GetActorFromObject( Sender, parameters->objects[1] );
		}
		scr = Sender;
	}
	if (!tar) {
		printf("[GameScript]: Target for dialog couldn't be found (Sender: %s, Type: %d).\n", Sender->GetScriptName(), Sender->Type);
		parameters->Dump();
		if (Sender->Type == ST_ACTOR) {
			((Actor *) Sender)->DebugDump();
		}
		parameters->objects[1]->Dump();
		Sender->CurrentAction = NULL;
		return;
	}

	//target could be other than Actor, we need to handle this too!
	//if (scr->Type != ST_ACTOR) {
	//	Sender->CurrentAction = NULL;
	//	return;
	//}
	//CHECKDIST works only for mobile scriptables
	if ((Flags&BD_CHECKDIST) && (Sender->Type==ST_ACTOR) ) {
		Actor *actor = (Actor *) Sender;
		if (Distance(Sender, tar)>actor->GetStat(IE_DIALOGRANGE)*20 ) {
			GoNearAndRetry(Sender, tar);
			Sender->CurrentAction = NULL;
			return;
		}
	}
	//making sure speaker is the protagonist, player, actor
	bool swap = false;
	if (scr->Type != ST_ACTOR) swap = true;
	else if (tar->Type == ST_ACTOR) {
		if ( ((Actor *) tar)->InParty == 1) swap = true;
		else if ( (((Actor *) scr)->InParty !=1) && ((Actor *) tar)->InParty) swap = true;
	}

	GameControl* gc = core->GetGameControl();
	if (!gc) {
		printMessage( "GameScript","Dialog cannot be initiated because there is no GameControl.", YELLOW );
		Sender->CurrentAction = NULL;
		return;
	}
	//can't initiate dialog, because it is already there
	if (gc->GetDialogueFlags()&DF_IN_DIALOG) {
		if (Flags & BD_INTERRUPT) {
			//break the current dialog if possible
			gc->EndDialog(true);
		}
		//check if we could manage to break it, not all dialogs are breakable!
		if (gc->GetDialogueFlags()&DF_IN_DIALOG) {
			printMessage( "GameScript","Dialog cannot be initiated because there is already one.", YELLOW );
			Sender->CurrentAction = NULL;
			return;
		}
	}

	const char* Dialog = NULL;
	int pdtable = -1;

	switch (Flags & BD_LOCMASK) {
		case BD_STRING0:
			Dialog = parameters->string0Parameter;
			if (Flags & BD_SETDIALOG) {
				if ( scr->Type == ST_ACTOR) {
					Actor* actor = ( Actor* ) scr;
					actor->SetDialog( Dialog );
				}
			}
			break;
		case BD_SOURCE:
//			Dialog = GetDialog(scr); //actor->Dialog;
//			break;
		case BD_TARGET:
			if (swap) Dialog = GetDialog(scr);
			else Dialog = GetDialog(tar);//target->Dialog;
			break;
		case BD_RESERVED:
			//what if playerdialog was initiated from Player2?
			PlayerDialogRes[5] = '1';
			Dialog = ( const char * ) PlayerDialogRes;
			break;
		case BD_INTERACT: //using the source for the dialog
			if ( scr->Type == ST_ACTOR) {
				pdtable = core->LoadTable( "interdia" );
				const char* scriptingname = ((Actor *) scr)->GetScriptName();
				//Dialog is a borrowed reference, we cannot free pdtable while it is being used
				Dialog = core->GetTable( pdtable )->QueryField( scriptingname, "FILE" );
			}
			break;
	}

	//maybe we should remove the action queue, but i'm unsure
	Sender->CurrentAction = NULL;

	//dialog is not meaningful
	if (!Dialog || Dialog[0]=='*') {
		goto end_of_quest;
	}

	//we also need to freeze active scripts during a dialog!
	if (scr!=tar) {
		if (swap) {
			Scriptable *tmp = tar;
			tar = scr;
			scr = tmp;
		}
		if (Sender!=tar) {
			if (Flags & BD_INTERRUPT) {
				scr->ClearActions();
			} else {
				if (scr->GetNextAction()) {
					core->DisplayConstantString(STR_TARGETBUSY,0xff0000);
					goto end_of_quest;
				}
			}
		}
		
	}

	if (scr->Type==ST_ACTOR) {
		((Actor *)scr)->SetOrientation(GetOrient( tar->Pos, scr->Pos),1);
		//scr->resetAction = true; //im not sure this is needed
	}
	if (tar->Type==ST_ACTOR) {
		((Actor *)tar)->SetOrientation(GetOrient( scr->Pos, tar->Pos),1);
		//tar->resetAction = true;//nor this
	}

	if (Dialog[0]) {
		//increasing NumTimesTalkedTo
		if (Flags & BD_TALKCOUNT) {
			gc->SetDialogueFlags(DF_TALKCOUNT, BM_OR);
		}

		gc->InitDialog( (Actor *) scr, tar, Dialog );
	}
//if pdtable was allocated, free it now, it will release Dialog
end_of_quest:
	if (pdtable!=-1) {
		core->DelTable( pdtable );
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

// not in IESDP but this one should affect ambients
void GameScript::SoundActivate(Scriptable* /*Sender*/, Action* parameters)
{
	AmbientMgr * ambientmgr = core->GetSoundMgr()->GetAmbientMgr();
	if (parameters->int0Parameter) {
		ambientmgr->activate(parameters->objects[1]->objectName);
	} else {
		ambientmgr->deactivate(parameters->objects[1]->objectName);
	}
}

// according to IESDP this action is about animations
void GameScript::AmbientActivate(Scriptable* Sender, Action* parameters)
{
	Animation* anim = Sender->GetCurrentArea( )->GetAnimation( parameters->objects[1]->objectName );
	if (!anim) {
		printf( "Script error: No Animation Named \"%s\"\n",
			parameters->objects[1]->objectName );
		return;
	}
	if (parameters->int0Parameter) {
		anim->Flags |= A_ANI_ACTIVE;
	} else {
		anim->Flags &= ~A_ANI_ACTIVE;
	}
}

void GameScript::StaticStart(Scriptable* Sender, Action* parameters)
{
	Animation *anim = Sender->GetCurrentArea()->GetAnimation(parameters->objects[1]->objectName);
	if (!anim) {
		printf( "Script error: No Animation Named \"%s\"\n",
			parameters->objects[1]->objectName );
		return;
	}
	anim->Flags &=~A_ANI_PLAYONCE;
}

void GameScript::StaticStop(Scriptable* Sender, Action* parameters)
{
	Animation *anim = Sender->GetCurrentArea()->GetAnimation(parameters->objects[1]->objectName);
	if (!anim) {
		printf( "Script error: No Animation Named \"%s\"\n",
			parameters->objects[1]->objectName );
		return;
	}
	anim->Flags |= A_ANI_PLAYONCE;
}

void GameScript::StaticPalette(Scriptable* Sender, Action* parameters)
{
	Animation *anim = Sender->GetCurrentArea()->GetAnimation(parameters->objects[1]->objectName);
	if (!anim) {
		printf( "Script error: No Animation Named \"%s\"\n",
			parameters->objects[1]->objectName );
		return;
	}
	ImageMgr *bmp = (ImageMgr *) core->GetInterface( IE_BMP_CLASS_ID);
	if (!bmp) {
		return;
	}
	DataStream* s = core->GetResourceMgr()->GetResource( parameters->string0Parameter, IE_BMP_CLASS_ID );
	bmp->Open( s, true );
	Color *pal = (Color *) malloc( sizeof(Color) * 256 );
	bmp->GetPalette( 0, 256, pal );
	core->FreeInterface( bmp );
	anim->SetPalette( pal, true );
	free (pal);
}

void GameScript::WaitAnimation(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->SetStance( parameters->int0Parameter );
	actor->SetWait( 1 );
}

/* sender itself */
void GameScript::PlaySequence(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->SetStance( parameters->int0Parameter );
}

/* another object */
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
	target->SetStance( parameters->int0Parameter );
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
	BeginDialog( Sender, parameters, BD_INTERRUPT | BD_TARGET | BD_NUMERIC | BD_TALKCOUNT | BD_CHECKDIST );
}

void GameScript::NIDSpecial2(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	Game *game=core->GetGame();
	if (!game->EveryoneStopped() ) {
		Sender->AddActionInFront( Sender->CurrentAction );
		//wait for a while
		Sender->SetWait( 1 * AI_UPDATE_TIME );
		return;
	}
	Actor *actor = (Actor *) Sender;
	if (!game->EveryoneNearPoint(actor->Area, actor->Pos, true) ) {
		//we abort the command, everyone should be here
		Sender->CurrentAction = NULL;
		return;
	}
	//travel direction passed to guiscript
	int direction = Sender->GetCurrentArea()->WhichEdge(actor->Pos);
	printf("Travel direction returned: %d\n", direction);
	if (direction==-1) {
		Sender->CurrentAction = NULL;
		return;
	}
	core->GetDictionary()->SetAt("Travel", (ieDword) direction);
	core->GetGUIScriptEngine()->RunFunction( "OpenWorldMapWindow" );
	//sorry, i have absolutely no idea when i should do this :)
	Sender->CurrentAction = NULL;
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
	BeginDialog( Sender, parameters, BD_INTERACT );
}

static Point &FindNearPoint(Scriptable* Sender, Point &p1, Point &p2, double& distance)
{
	int distance1 = Distance(p1, Sender);
	int distance2 = Distance(p2, Sender);
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
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		Sender->CurrentAction = NULL;
		return;
	}
	if (tar->Type != ST_DOOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	Door* door = ( Door* ) tar;
	door->SetDoorLocked( true, true);
}

void GameScript::Unlock(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
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

void GameScript::SetDoorLocked(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		Sender->CurrentAction = NULL;
		return;
	}
	if (tar->Type != ST_DOOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	Door* door = ( Door* ) tar;
	door->SetDoorLocked( parameters->int0Parameter!=0, false);
}

void GameScript::OpenDoor(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		Sender->CurrentAction = NULL;
		return;
	}
	if (tar->Type != ST_DOOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	Door* door = ( Door* ) tar;
	if (door->IsOpen()) {
		//door is already open
		Sender->CurrentAction = NULL;
		return;
	}
	if (Sender->Type != ST_ACTOR) {
		//if not an actor opens, it don't play sound
		door->SetDoorOpen( true, false );		
		Sender->CurrentAction = NULL;
		return;
	}
	double distance;
	Point &p = FindNearPoint( Sender, door->toOpen[0], door->toOpen[1],
				distance );
	if (distance <= MAX_OPERATING_DISTANCE) {
		if (door->Flags&DOOR_LOCKED) {
			const char *Key = door->GetKey();
			Actor *actor = (Actor *) Sender;
			if (!Key || !actor->inventory.HasItem(Key,0) ) {
				//playsound unsuccessful opening of door
				core->PlaySound(DS_OPEN_FAIL);
				return;
			}

			//remove the key
			if (door->Flags&DOOR_KEY) {
				CREItem *item = NULL;
				actor->inventory.RemoveItem(Key,0,&item);
				//the item should always be existing!!!
				if (item) {
					delete item;
				}
			}
		}
		door->SetDoorOpen( true, true );
	} else {
		GoNearAndRetry(Sender, p);
	}
	Sender->CurrentAction = NULL;
}

void GameScript::CloseDoor(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		Sender->CurrentAction = NULL;
		return;
	}
	if (tar->Type != ST_DOOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	Door* door = ( Door* ) tar;
	if (!door->IsOpen() ) {
		//door is already closed 
		Sender->CurrentAction = NULL;
		return;
	}
	if (Sender->Type != ST_ACTOR) {
		//if not an actor opens, it don't play sound
		door->SetDoorOpen( false, false );
		Sender->CurrentAction = NULL;
		return;
	}
	double distance;
	Point &p = FindNearPoint( Sender, door->toOpen[0], door->toOpen[1],
				distance );	
	if (distance <= MAX_OPERATING_DISTANCE) {
		//actually if we got the key, we could still open it
		//we need a more sophisticated check here
		//doors could be locked but open, and unable to close
		if (door->Flags&DOOR_LOCKED) {
			//playsound unsuccessful closing of door
			core->PlaySound(DS_CLOSE_FAIL);
		}
		else {
			door->SetDoorOpen( false, true );
		}
	} else {
		GoNearAndRetry(Sender, p);
	}
	Sender->CurrentAction = NULL;
}

void GameScript::ContainerEnable(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_CONTAINER) {
		return;
	}
	Container *cnt = (Container *) tar;
	if (parameters->int0Parameter) {
		cnt->Flags&=~CONT_DISABLED;
	}
	else {
		cnt->Flags|=CONT_DISABLED;
	}
}

void GameScript::MoveBetweenAreasCore(Actor* actor, const char *area, Point &position, int face, bool adjust)
{
	printf("MoveBetweenAreas: %s to %s [%d.%d] face: %d\n", actor->GetName(0), area,position.x,position.y, face);
	Map* map2;
	Game* game = core->GetGame();
	if (area[0]) { //do we need to switch area?
		Map* map1 = actor->GetCurrentArea();
		//we have to change the pathfinder
		//to the target area if adjust==true
		map2 = game->GetMap(area, false);
		if ( map1!=map2 ) {
			if (map1) {
				map1->RemoveActor( actor );
			}
			map2->AddActor( actor );
		}
	}
	else {
		map2=actor->GetCurrentArea();
	}
	actor->SetPosition(map2, position, adjust);
	if (face !=-1) {
		actor->SetOrientation( face,0 );
	}
	GameControl *gc=core->GetGameControl();
	gc->SetScreenFlags(SF_CENTERONACTOR,BM_OR);
}

void GameScript::MoveBetweenAreas(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	if (parameters->string1Parameter[0]) {
		CreateVisualEffectCore(Sender, Sender->Pos, parameters->string1Parameter);
	}
	MoveBetweenAreasCore((Actor *) Sender, parameters->string0Parameter,
		parameters->pointParameter, parameters->int0Parameter, true);
}

void GetPositionFromScriptable(Scriptable* scr, Point &position, bool trap)
{
	if (!trap) {
		position = scr->Pos;
		return;
	}
	switch (scr->Type) {
		case ST_AREA: case ST_GLOBAL: case ST_ACTOR:
			position = scr->Pos;
			break;
		case ST_TRIGGER: case ST_PROXIMITY: case ST_TRAVEL:
		case ST_DOOR: case ST_CONTAINER:
			position=((Highlightable *) scr)->TrapLaunch;
	}
}

void GameScript::ForceSpell(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar) {
		return;
	}
	if (Sender->Type == ST_ACTOR) {
		Actor *actor = (Actor *) Sender;
		actor->SetStance (IE_ANI_CAST);
	}
	Point s,d;
	GetPositionFromScriptable( Sender, s, true );
	GetPositionFromScriptable( tar, d, true );
	printf( "ForceSpell from [%d,%d] to [%d,%d]\n", s.x, s.y, d.x, d.y );
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
	tar->Active &=~SCR_ACTIVE;
}

void GameScript::MakeGlobal(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* act = ( Actor* ) Sender;
	core->GetGame()->AddNPC( act );
}

void GameScript::UnMakeGlobal(Scriptable* Sender, Action* /*parameters*/)
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

//this apparently doesn't check the gold, thus could be used from non actors
void GameScript::GivePartyGoldGlobal(Scriptable* Sender, Action* parameters)
{
/*
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* act = ( Actor* ) Sender;
*/
	ieDword gold = (unsigned long) CheckVariable( Sender, parameters->string0Parameter );
	//act->NewStat(IE_GOLD, -gold, MOD_ADDITIVE);
	core->GetGame()->PartyGold += gold;
}

void GameScript::CreatePartyGold(Scriptable* /*Sender*/, Action* parameters)
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
	if (gold>parameters->int0Parameter) {
		gold=parameters->int0Parameter;
	}
	act->NewStat(IE_GOLD, -gold, MOD_ADDITIVE);
	core->GetGame()->PartyGold+=gold;
}

void GameScript::DestroyPartyGold(Scriptable* /*Sender*/, Action* parameters)
{
	int gold = core->GetGame()->PartyGold;
	if (gold>parameters->int0Parameter) {
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
	if (gold>parameters->int0Parameter) {
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

void GameScript::AddXP2DA(Scriptable* /*Sender*/, Action* parameters)
{
	int xptable;
	
	if (core->HasFeature(GF_HAS_EXPTABLE) ) {
		xptable = core->LoadTable("exptable");
	}
	else {
		xptable = core->LoadTable( "xplist" );
	}
	
	if (parameters->int0Parameter>0) {
		//display string
	}
	if (xptable<0) {
		printMessage("GameScript","Can't perform ADDXP2DA",LIGHT_RED);
		return;
	}
	char * xpvalue = core->GetTable( xptable )->QueryField( parameters->string0Parameter, "0" ); //level is unused
	
	if ( xpvalue[0]=='P' && xpvalue[1]=='_') {
		//divide party xp
		core->GetGame()->ShareXP(atoi(xpvalue+2), true );
	}
	else {
		//give xp everyone
		core->GetGame()->ShareXP(atoi(xpvalue), false );
	}
	core->DelTable( xptable );
}

void GameScript::AddExperienceParty(Scriptable* /*Sender*/, Action* parameters)
{
	core->GetGame()->ShareXP(parameters->int0Parameter, true);
}

void GameScript::AddExperiencePartyGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword xp = CheckVariable( Sender, parameters->string0Parameter );
	core->GetGame()->ShareXP(xp, true);
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
	/* i'm not sure this is required here at all */
	SetBeenInPartyFlags(Sender, parameters);
	Actor* act = ( Actor* ) Sender;
	act->SetStat( IE_EA, PC );
	if (core->HasFeature( GF_HAS_DPLAYER )) {
		act->SetScript( "DPLAYER2", SCR_DEFAULT );
	}
	int pdtable = core->LoadTable( "pdialog" );
	if ( pdtable >= 0 ) {
		const char* scriptname = act->GetScriptName();
		ieResRef resref;
		TableMgr *tab=core->GetTable( pdtable );
		//set dialog only if we got a row
		if (tab->GetRowIndex( scriptname ) != -1) {
			strnuprcpy(resref, tab->QueryField( scriptname, "JOIN_DIALOG_FILE"),8);
			act->SetDialog( resref );
		}
		core->DelTable( pdtable );
	}
	core->GetGame()->JoinParty( act );
	core->GetGUIScriptEngine()->RunFunction( "UpdatePortraitWindow" );
}

void GameScript::LeaveParty(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* act = ( Actor* ) Sender;
	act->SetStat( IE_EA, NEUTRAL );
	core->GetGame()->LeaveParty( act );
	if (core->HasFeature( GF_HAS_DPLAYER )) {
		act->SetScript( "", SCR_DEFAULT );
	}
/* apparently this is handled by script in dplayer3 (or2)
	if (core->HasFeature( GF_HAS_PDIALOG )) {
		int pdtable = core->LoadTable( "pdialog" );
		char* scriptingname = act->GetScriptName();
		act->SetDialog( core->GetTable( pdtable )->QueryField( scriptingname,
				"POST_DIALOG_FILE" ) );
		core->DelTable( pdtable );
	}
*/
	core->GetGUIScriptEngine()->RunFunction( "UpdatePortraitWindow" );
}

//no idea why we would need this if we have activate/deactivate
void GameScript::HideCreature(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	if ( parameters->int0Parameter != 0 ) {
		tar->Active |= SCR_ACTIVE;
	} else {
		tar->Active &=~SCR_ACTIVE;
	}
}

void GameScript::Activate(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	tar->Active |= SCR_ACTIVE;
}

void GameScript::ForceLeaveAreaLUA(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) tar;
	//the LoadMos ResRef may be empty
	strncpy(core->GetGame()->LoadMos, parameters->string1Parameter,8);
	MoveBetweenAreasCore( actor, parameters->string0Parameter, parameters->pointParameter, parameters->int0Parameter, true);
}

void GameScript::LeaveAreaLUA(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	//the LoadMos ResRef may be empty
	strncpy(core->GetGame()->LoadMos, parameters->string1Parameter,8);
	MoveBetweenAreasCore( actor, parameters->string0Parameter, parameters->pointParameter, parameters->int0Parameter, true);
}

//this is a blocking action, because we have to move to the Entry
void GameScript::LeaveAreaLUAEntry(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	Actor *actor = (Actor *) Sender;
	Game *game = core->GetGame();
	strncpy(game->LoadMos, parameters->string1Parameter,8);
	//no need to change the pathfinder just for getting the entrance
	Map *map = game->GetMap(actor->Area, false);
	Entrance *ent = map->GetEntrance(parameters->string1Parameter);
	if (Distance(ent->Pos, Sender) <= MAX_OPERATING_DISTANCE) {
		LeaveAreaLUA(Sender, parameters);
		Sender->CurrentAction = NULL;
		return;
	}
	GoNearAndRetry( Sender, ent->Pos);
	Sender->CurrentAction = NULL;
}

void GameScript::LeaveAreaLUAPanic(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	strncpy(core->GetGame()->LoadMos, parameters->string1Parameter,8);
	MoveBetweenAreasCore( actor, parameters->string0Parameter, parameters->pointParameter, parameters->int0Parameter, true);
}

//this is a blocking action, because we have to move to the Entry
void GameScript::LeaveAreaLUAPanicEntry(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	Actor *actor = (Actor *) Sender;
	Game *game = core->GetGame();
	strncpy(game->LoadMos, parameters->string1Parameter,8);
	//no need to change the pathfinder just for getting the entrance
	Map *map = game->GetMap( actor->Area, false);
	Entrance *ent = map->GetEntrance(parameters->string1Parameter);
	if (Distance(ent->Pos, Sender) <= MAX_OPERATING_DISTANCE) {
		LeaveAreaLUAPanic(Sender, parameters);
		Sender->CurrentAction = NULL;
		return;
	}
	GoNearAndRetry( Sender, ent->Pos);
	Sender->CurrentAction = NULL;
}

void GameScript::SetToken(Scriptable* /*Sender*/, Action* parameters)
{
	//SetAt takes a newly created reference (no need of free/copy)
	core->GetTokenDictionary()->SetAt( parameters->string1Parameter, core->GetString( parameters->int0Parameter) );
}

void GameScript::SetTokenGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value = CheckVariable( Sender, parameters->string0Parameter );
	//using SetAtCopy because we need a copy of the value
	char tmpstr[10];
	sprintf( tmpstr, "%d", value );
	core->GetTokenDictionary()->SetAtCopy( parameters->string1Parameter, tmpstr );
}

void GameScript::PlayDead(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->SetStance( IE_ANI_DIE );
	actor->InternalFlags|=IF_NOINT;
	actor->playDeadCounter = parameters->int0Parameter;
	actor->SetWait( parameters->int0Parameter );
}

/** no difference at this moment, but this action should be interruptable */
/** probably that means, we don't have to issue the SetWait, but this needs */
/** further research */
void GameScript::PlayDeadInterruptable(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->SetStance( IE_ANI_DIE );
	//also set time for playdead!
	actor->playDeadCounter = parameters->int0Parameter;
	actor->SetWait( parameters->int0Parameter );
}

/* this may not be correct, just a placeholder you can fix */
void GameScript::Swing(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->SetStance( IE_ANI_ATTACK );
	actor->SetWait( 1 );
}

/* this may not be correct, just a placeholder you can fix */
void GameScript::SwingOnce(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->SetStance( IE_ANI_ATTACK );
	actor->SetWait( 1 );
}

void GameScript::Recoil(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->SetStance( IE_ANI_DAMAGE );
	actor->SetWait( 1 );
}

void GameScript::GlobalSetGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value = CheckVariable( Sender, parameters->string0Parameter );
	SetVariable( Sender, parameters->string1Parameter, value );
}

/* adding the second variable to the first, they must be GLOBAL */
void GameScript::AddGlobals(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender, "GLOBAL",
		parameters->string0Parameter );
	ieDword value2 = CheckVariable( Sender, "GLOBAL",
		parameters->string1Parameter );
	SetVariable( Sender, "GLOBAL", parameters->string0Parameter,
		value1 + value2 );
}

/* adding the second variable to the first, they could be area or locals */
void GameScript::GlobalAddGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender,
		parameters->string0Parameter );
	ieDword value2 = CheckVariable( Sender,
		parameters->string1Parameter );
	SetVariable( Sender, parameters->string0Parameter, value1 + value2 );
}

/* adding the number to the global, they could be area or locals */
void GameScript::IncrementGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value = CheckVariable( Sender, parameters->string0Parameter );
	SetVariable( Sender, parameters->string0Parameter,
		value + parameters->int0Parameter );
}

/* adding the number to the global ONLY if the first global is zero */
void GameScript::IncrementGlobalOnce(Scriptable* Sender, Action* parameters)
{
	ieDword value = CheckVariable( Sender, parameters->string0Parameter );
	if (value != 0) {
		return;
	}
	value = CheckVariable( Sender, parameters->string1Parameter );
	SetVariable( Sender, parameters->string1Parameter,
		value + parameters->int0Parameter );
}

void GameScript::GlobalSubGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender,
		parameters->string0Parameter );
	ieDword value2 = CheckVariable( Sender,
		parameters->string1Parameter );
	SetVariable( Sender, parameters->string0Parameter, value1 - value2 );
}

void GameScript::GlobalAndGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender,
		parameters->string0Parameter );
	ieDword value2 = CheckVariable( Sender,
		parameters->string1Parameter );
	SetVariable( Sender, parameters->string0Parameter, value1 && value2 );
}

void GameScript::GlobalOrGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender,
		parameters->string0Parameter );
	ieDword value2 = CheckVariable( Sender,
		parameters->string1Parameter );
	SetVariable( Sender, parameters->string0Parameter, value1 || value2 );
}

void GameScript::GlobalBOrGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender,
		parameters->string0Parameter );
	ieDword value2 = CheckVariable( Sender,
		parameters->string1Parameter );
	SetVariable( Sender, parameters->string0Parameter, value1 | value2 );
}

void GameScript::GlobalBAndGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender,
		parameters->string0Parameter );
	ieDword value2 = CheckVariable( Sender,
		parameters->string1Parameter );
	SetVariable( Sender, parameters->string0Parameter, value1 & value2 );
}

void GameScript::GlobalXorGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender,
		parameters->string0Parameter );
	ieDword value2 = CheckVariable( Sender,
		parameters->string1Parameter );
	SetVariable( Sender, parameters->string0Parameter, value1 ^ value2 );
}

void GameScript::GlobalBOr(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender,
		parameters->string0Parameter );
	SetVariable( Sender, parameters->string0Parameter,
		value1 | parameters->int0Parameter );
}

void GameScript::GlobalBAnd(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender,
		parameters->string0Parameter );
	SetVariable( Sender, parameters->string0Parameter,
		value1 & parameters->int0Parameter );
}

void GameScript::GlobalXor(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender,
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
	ieDword value1 = CheckVariable( Sender,
		parameters->string0Parameter );
	SetVariable( Sender, parameters->string0Parameter,
		value1 & ~parameters->int0Parameter );
}

void GameScript::GlobalShL(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender,
		parameters->string0Parameter );
	ieDword value2 = parameters->int0Parameter;
	if (value2 > 31) {
		value1 = 0;
	} else {
		value1 <<= value2;
	}
	SetVariable( Sender, parameters->string0Parameter, value1 );
}

void GameScript::GlobalShR(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender,
		parameters->string0Parameter );
	ieDword value2 = parameters->int0Parameter;
	if (value2 > 31) {
		value1 = 0;
	} else {
		value1 >>= value2;
	}
	SetVariable( Sender, parameters->string0Parameter, value1 );
}

void GameScript::GlobalMaxGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender,
		parameters->string0Parameter );
	ieDword value2 = CheckVariable( Sender,
		parameters->string1Parameter );
	if (value1 < value2) {
		SetVariable( Sender, parameters->string0Parameter, value1 );
	}
}

void GameScript::GlobalMinGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender, parameters->string0Parameter );
	ieDword value2 = CheckVariable( Sender, parameters->string1Parameter );
	if (value1 < value2) {
		SetVariable( Sender, parameters->string0Parameter, value1 );
	}
}

void GameScript::GlobalShLGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender, parameters->string0Parameter );
	ieDword value2 = CheckVariable( Sender, parameters->string1Parameter );
	if (value2 > 31) {
		value1 = 0;
	} else {
		value1 <<= value2;
	}
	SetVariable( Sender, parameters->string0Parameter, value1 );
}
void GameScript::GlobalShRGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable( Sender, parameters->string0Parameter );
	ieDword value2 = CheckVariable( Sender, parameters->string1Parameter );
	if (value2 > 31) {
		value1 = 0;
	} else {
		value1 >>= value2;
	}
	SetVariable( Sender, parameters->string0Parameter, value1 );
}

void GameScript::ClearAllActions(Scriptable* /*Sender*/, Action* /*parameters*/)
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

void GameScript::ClearActions(Scriptable* Sender, Action* /*parameters*/)
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

void GameScript::StartMovie(Scriptable* /*Sender*/, Action* parameters)
{
	core->PlayMovie( parameters->string0Parameter );
}

void GameScript::SetLeavePartyDialogFile(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	int pdtable = core->LoadTable( "pdialog" );
	Actor* act = ( Actor* ) Sender;
	const char* scriptingname = act->GetScriptName();
	act->SetDialog( core->GetTable( pdtable )->QueryField( scriptingname,
			"POST_DIALOG_FILE" ) );
	core->DelTable( pdtable );
}

void GameScript::TextScreen(Scriptable* /*Sender*/, Action* parameters)
{
	int chapter;
	ieDword line;
	bool iwd=false;

	if (parameters->string0Parameter[0]) {
		chapter = core->LoadTable( parameters->string0Parameter );
		line = 1;
	}
	else {//iwd/iwd2 has a single table named chapters
		chapter = core->LoadTable ( "CHAPTERS" );
		core->GetGame()->globals->Lookup( "CHAPTER", line );
		iwd = true;
	}

	if (chapter<0) {
//add some standard way of notifying the user
//		MissingResource(parameters->string0Parameter);
		return;
	}

	TableMgr *table = core->GetTable(chapter);
	strnuprcpy(core->GetGame()->LoadMos, table->QueryField(0xffffffff),8);
	GameControl *gc=core->GetGameControl();
	if (gc) {
		char *strref = table->QueryField(line, 0);
		char *str=core->GetString( strtol(strref,NULL,0) );
		core->DisplayString(str);
		free(str);
		strref = table->QueryField(line, 1);
		str=core->GetString( strtol(strref,NULL,0) );
		core->DisplayString(str);
		free(str);
	}
	core->DelTable(chapter);
	core->GetGUIScriptEngine()->RunFunction( "StartTextScreen" );
}

void GameScript::IncrementChapter(Scriptable* Sender, Action* parameters)
{
	TextScreen(Sender, parameters);
	core->GetGame()->IncrementChapter();
}

void GameScript::SetBeenInPartyFlags(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	//it is bit 15 of the multi-class flags (confirmed)
	actor->SetStat(IE_MC_FLAGS,actor->GetStat(IE_MC_FLAGS)|MC_BEENINPARTY);
}

/*pst sets the corpse enabled flags */
void GameScript::SetCorpseEnabled(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor* act = ( Actor* ) Sender;
	int value = act->GetStat(IE_MC_FLAGS);
	if ( parameters->int0Parameter) {
		value |= MC_KEEP_CORPSE;
		value &= ~MC_REMOVE_CORPSE;
	}
	else {
		value |= MC_REMOVE_CORPSE;
		value &= ~MC_KEEP_CORPSE;
	}
	act->SetStat(IE_MC_FLAGS, value);
}

/*iwd2 sets the high MC bits this way*/
void GameScript::SetCreatureAreaFlags(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	//confirmed with the invulnerability flag (0x20000)
	ieDword value=actor->GetStat(IE_MC_FLAGS);
	HandleBitMod(value, parameters->int0Parameter, parameters->int1Parameter);
	actor->SetStat(IE_MC_FLAGS,value);
}

void GameScript::SetTextColor(Scriptable* /*Sender*/, Action* parameters)
{
	GameControl *gc=core->GetGameControl();
	if (gc) {
		Color color;

		memcpy(&color,&parameters->int0Parameter,4);
		gc->SetInfoTextColor( color );
	}
}

void GameScript::BitGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value = CheckVariable(Sender, parameters->string0Parameter );
	HandleBitMod( value, parameters->int0Parameter, parameters->int1Parameter);
	SetVariable(Sender, parameters->string0Parameter, value);
}

void GameScript::GlobalBitGlobal(Scriptable* Sender, Action* parameters)
{
	ieDword value1 = CheckVariable(Sender, parameters->string0Parameter );
	ieDword value2 = CheckVariable(Sender, parameters->string1Parameter );
	HandleBitMod( value1, value2, parameters->int1Parameter);
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

void GameScript::Debug(Scriptable* /*Sender*/, Action* parameters)
{
	InDebug=parameters->int0Parameter;
	printMessage("GameScript",parameters->string0Parameter,YELLOW);
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
	target->NewStat(IE_FREESLOTS, parameters->int0Parameter,MOD_ADDITIVE);
}

//the third parameter is a GemRB extension
void GameScript::AddJournalEntry(Scriptable* /*Sender*/, Action* parameters)
{
	core->GetGame()->AddJournalEntry(parameters->int0Parameter, parameters->int1Parameter, parameters->int2Parameter);
}

void GameScript::SetQuestDone(Scriptable* /*Sender*/, Action* parameters)
{
	core->GetGame()->DeleteJournalEntry(parameters->int0Parameter);
	core->GetGame()->AddJournalEntry(parameters->int0Parameter, IE_GAM_QUEST_DONE, parameters->int2Parameter);

}

void GameScript::RemoveJournalEntry(Scriptable* /*Sender*/, Action* parameters)
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

void GameScript::DestroyAllEquipment(Scriptable* Sender, Action* /*parameters*/)
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
	if (inv) {
		inv->DestroyItem("",0,~0); //destroy any and all
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
	if (inv) { 
		inv->DestroyItem(parameters->string0Parameter,0,1); //destroy one (even indestructible?)
	}
}

void GameScript::DestroyGold(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR)
		return;
	Actor *act=(Actor *) Sender;
	int max=act->GetStat(IE_GOLD);
	if (max>parameters->int0Parameter) {
		max=parameters->int0Parameter;
	}
	act->NewStat(IE_GOLD, -max, MOD_ADDITIVE);
}

void GameScript::DestroyPartyItem(Scriptable* /*Sender*/, Action* parameters)
{
	Game *game = core->GetGame();
	int i = game->GetPartySize(false);
	ieDword count;
	if (parameters->int0Parameter) count=~0;
	else count=1;
	while (i--) {
		Inventory *inv = &(game->GetPC(i)->inventory);
		int res=inv->DestroyItem(parameters->string0Parameter,0,count);
		if ( (count == 1) && res) {
			break;
		}
	}
}

/* this is a gemrb extension */
void GameScript::DestroyPartyItemNum(Scriptable* /*Sender*/, Action* parameters)
{
	Game *game = core->GetGame();
	int i = game->GetPartySize(false);
	ieDword count;
	count = parameters->int0Parameter;
	while (i--) {
		Inventory *inv = &(game->GetPC(i)->inventory);
		count -= inv->DestroyItem(parameters->string0Parameter,0,count);
		if (!count ) {
			break;
		}
	}
}

void GameScript::DestroyAllDestructableEquipment(Scriptable* Sender, Action* /*parameters*/)
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
	if (inv) {
		inv->DestroyItem("", IE_INV_ITEM_DESTRUCTIBLE, ~0);
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
void GameScript::UnloadArea(Scriptable* /*Sender*/, Action* parameters)
{
	int map=core->GetGame()->FindMap(parameters->string0Parameter);
	if (map>=0) {
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
	target->SetStat(IE_HITPOINTS,0); //probably this is the proper way
}

void GameScript::SetGabber(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	GameControl* gc = core->GetGameControl();
	if (gc->GetDialogueFlags()&DF_IN_DIALOG) {
		gc->speaker = (Actor *) tar;
	}
	else {
		printMessage("GameScript","Can't set gabber!",YELLOW);
	}
}

void GameScript::ReputationSet(Scriptable* /*Sender*/, Action* parameters)
{
	core->GetGame()->SetReputation(parameters->int0Parameter);
}

void GameScript::ReputationInc(Scriptable* /*Sender*/, Action* parameters)
{
	Game *game = core->GetGame();
	game->SetReputation( (int) game->Reputation + parameters->int0Parameter);
}

void GameScript::FullHeal(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	Actor *scr = (Actor *) tar;
	scr->SetStat(IE_HITPOINTS, scr->GetStat(IE_MAXHITPOINTS) );
}

void GameScript::RemovePaladinHood(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *act = (Actor *) Sender;
	act->SetStat(IE_MC_FLAGS, act->GetStat(IE_MC_FLAGS) | MC_FALLEN_PALADIN);
}

void GameScript::RemoveRangerHood(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *act = (Actor *) Sender;
	act->SetStat(IE_MC_FLAGS, act->GetStat(IE_MC_FLAGS) | MC_FALLEN_RANGER);
}

void GameScript::RegainPaladinHood(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *act = (Actor *) Sender;
	act->SetStat(IE_MC_FLAGS, act->GetStat(IE_MC_FLAGS) & ~MC_FALLEN_PALADIN);
}

void GameScript::RegainRangerHood(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *act = (Actor *) Sender;
	act->SetStat(IE_MC_FLAGS, act->GetStat(IE_MC_FLAGS) & ~MC_FALLEN_RANGER);
}

//transfering item from Sender to target, target must be an actor
//if target can't get it, it will be dropped at its feet
int GameScript::MoveItemCore(Scriptable *Sender, Scriptable *target, const char *resref, int flags)
{
	Inventory *myinv;
	Map *map;

	if (!target || target->Type!=ST_ACTOR) {
		return MIC_INVALID;
	}
	map=Sender->GetCurrentArea();
	switch(Sender->Type) {
		case ST_ACTOR:
			myinv=&((Actor *) Sender)->inventory;
			break;
		case ST_CONTAINER:
			myinv=&((Container *) Sender)->inventory;
			break;
		default:
			return MIC_INVALID;
	}
	Actor *scr = (Actor *) target;
	CREItem *item;
	scr->inventory.RemoveItem(resref, flags, &item);
	if (!item)
		return MIC_NOITEM;
	if ( 2 != myinv->AddSlotItem(item, -1)) {
		// drop it at my feet
		map->TMap->AddItemToLocation(Sender->Pos, item);
		return MIC_FULL;
	}
	return MIC_GOTITEM;
}

//a container or an actor can take an item from someone
void GameScript::GetItem(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	MoveItemCore(tar, Sender, parameters->string0Parameter,0);
}

//getting one single item
void GameScript::TakePartyItem(Scriptable* Sender, Action* parameters)
{
	Game *game=core->GetGame();
	int i=game->GetPartySize(false);
	while (i--) {
		int res=MoveItemCore(game->GetPC(i), Sender, parameters->string0Parameter,0);
		if (res!=MIC_NOITEM) return;
	}
}

//getting x single item
void GameScript::TakePartyItemNum(Scriptable* Sender, Action* parameters)
{
	int count = parameters->int0Parameter;
	Game *game=core->GetGame();
	int i=game->GetPartySize(false);
	while (i--) {
		int res=MoveItemCore(game->GetPC(i), Sender, parameters->string0Parameter,0);
		if (res == MIC_GOTITEM) {
			i++;
			count--;
		}
		if (res!=MIC_NOITEM || !count) return;
	}
}

void GameScript::TakePartyItemAll(Scriptable* Sender, Action* parameters)
{
	Game *game=core->GetGame();
	int i=game->GetPartySize(false);
	while (i--) {
		while (MoveItemCore(game->GetPC(i), Sender, parameters->string0Parameter,0)==MIC_GOTITEM);
	}
}

//an actor can 'give' an item to a container or another actor
void GameScript::GiveItem(Scriptable *Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	MoveItemCore(Sender, tar, parameters->string0Parameter,0);
}

void CreateItemCore(CREItem *item, const char *resref, int a, int b, int c)
{
	strncpy(item->ItemResRef, resref, 8);
	if (a==-1) {
		//get the usages right
	}
	else {
		item->Usages[0]=a;
		item->Usages[1]=b;
		item->Usages[2]=c;
	}
	//no need, when Inventory receives it, it will be set
	//item->Flags=IE_INV_ITEM_ACQUIRED; //get the flags right
}

//this action creates an item in a container or a creature
//if there is an object it works as GiveItemCreate
//otherwise it creates the item on the Sender
void GameScript::CreateItem(Scriptable *Sender, Action* parameters)
{
	Scriptable* tar;
	if (parameters->objects[1]) {
		tar = GetActorFromObject( Sender, parameters->objects[1] );
	}
	else {
		tar = Sender;
	}
	if (!tar)
		return;
	Inventory *myinv;

	switch(tar->Type) {
		case ST_ACTOR:
			myinv = &((Actor *) tar)->inventory;
			break;
		case ST_CONTAINER:
			myinv = &((Container *) tar)->inventory;
			break;
		default:
			return;
	}
	
	CREItem *item = new CREItem();
	CreateItemCore(item, parameters->string0Parameter, parameters->int0Parameter, parameters->int1Parameter, parameters->int2Parameter);
	if (tar->Type==ST_CONTAINER) {
		myinv->AddItem(item);
	}
	else {
		if ( 2 != myinv->AddSlotItem(item, -1)) {
			Map *map=Sender->GetCurrentArea();
			// drop it at my feet
			map->TMap->AddItemToLocation(Sender->Pos, item);
		}
	}
}

void GameScript::CreateItemNumGlobal(Scriptable *Sender, Action* parameters)
{
	Inventory *myinv;

	switch(Sender->Type) {
		case ST_ACTOR:
			myinv = &((Actor *) Sender)->inventory;
			break;
		case ST_CONTAINER:
			myinv = &((Container *) Sender)->inventory;
			break;
		default:
			return;
	}
	int value = CheckVariable( Sender, parameters->string0Parameter );
	CREItem *item = new CREItem();
	CreateItemCore(item, parameters->string1Parameter, value, 0, 0);
	if (Sender->Type==ST_CONTAINER) {
		myinv->AddItem(item);
	}
	else {
		if ( 2 != myinv->AddSlotItem(item, -1)) {
			Map *map=Sender->GetCurrentArea();
			// drop it at my feet
			map->TMap->AddItemToLocation(Sender->Pos, item);
		}
	}
}

void GameScript::TakeItemReplace(Scriptable *Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type != ST_ACTOR) {
		return;
	}
	
	Actor *scr = (Actor *) tar;
	CREItem *item;
	int slot = scr->inventory.RemoveItem(parameters->string1Parameter, false, &item);
	if (!item) {
		item = new CREItem();
	}
	CreateItemCore(item, parameters->string0Parameter, -1, 0, 0);
	if (2 != scr->inventory.AddSlotItem(item,slot)) {
		Map *map = scr->GetCurrentArea();
		map->TMap->AddItemToLocation(Sender->Pos, item);
	}
}

//same as equipitem, but with additional slots parameter, and object to perform action
void GameScript::XEquipItem(Scriptable *Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );

	if (tar->Type!=ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) tar;
	int slot = actor->inventory.FindItem(parameters->string0Parameter, 0);
	if (slot<0) {
		return;
	}
}

void GameScript::EquipItem(Scriptable *Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) Sender;
	int slot = actor->inventory.FindItem(parameters->string0Parameter, 0);
	if (slot<0) {
		return;
	}
	if (parameters->int0Parameter==0) { //unequip
		//move item to inventory if possible
	}
	else { //equip
		//equip item if possible
	}
}

void GameScript::DropItem(Scriptable *Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	if (Distance(parameters->pointParameter, Sender) > 10) {
		GoNearAndRetry(Sender, parameters->pointParameter);
		Sender->CurrentAction = NULL;
		return;
	}
	Actor *scr = (Actor *) Sender;
	Map *map = Sender->GetCurrentArea();
	//dropping location isn't exactly our place, this is why i didn't use
	scr->inventory.DropItemAtLocation(parameters->string0Parameter, 0, map,
		parameters->pointParameter);
	Sender->CurrentAction = NULL;
}

void GameScript::DropInventory(Scriptable *Sender, Action* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *scr = (Actor *) Sender;
	scr->DropItem("",0);
}

void GameScript::Plunder(Scriptable *Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	Actor *scr = (Actor *) tar;
	//can plunder only dead actors
	if (! (scr->GetStat(IE_STATE_ID)&STATE_DEAD) ) {
		Sender->CurrentAction = NULL;
		return;
	}
	if (Distance(Sender, tar)>MAX_OPERATING_DISTANCE*20 ) {
		GoNearAndRetry(Sender, tar);
		Sender->CurrentAction = NULL;
		return;
	}
	//move all movable item from the target to the Sender
	//the rest will be dropped at the feet of Sender
	while(MoveItemCore(tar, Sender, "",0)!=MIC_NOITEM);
}

void GameScript::PickPockets(Scriptable *Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	Actor *snd = (Actor *) Sender;
	Actor *scr = (Actor *) tar;
	//for PP one must go REALLY close
	if (Distance(Sender, tar)>10 ) {
		GoNearAndRetry(Sender, tar);
		Sender->CurrentAction = NULL;
		return;
	}
	//find a candidate item for stealing (unstealable items are noticed)
	Actor *actor = (Actor *) tar;
	int slot = actor->inventory.FindItem("", IE_INV_ITEM_UNSTEALABLE | IE_INV_ITEM_EQUIPPED);
	int money=0;
	if (slot<0) {
		//go for money too
		if (scr->GetStat(IE_GOLD)>0)
		{
			money=RandomNumValue%(scr->GetStat(IE_GOLD)+1);
		}
		if (!money) {
			//no stuff to steal
			return;
		}
	}
	else {
		
	}
	//check for success, failure sends an attackedby trigger and a
	//pickpocket failed trigger sent to the target and sender respectively
	//slot == -1 here means money
	if (slot==-1) { 
		scr->NewStat(IE_GOLD,-money,MOD_ADDITIVE);
		snd->NewStat(IE_GOLD,money,MOD_ADDITIVE);
		return;
	}
	// now this is a kind of giveitem
//	MoveItemCore(tar, Sender, MOVABLE, slot);
}

void GameScript::TakeItemList(Scriptable * Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	int table = core->LoadTable(parameters->string0Parameter);
	if (table<0) {
		return;
	}
	TableMgr *tab=core->GetTable( table );
	if (tab) {
		int rows = tab->GetRowCount();
		for (int i=0;i<rows;i++) {
			MoveItemCore(tar, Sender, tab->QueryField(i,0), 0);
		}
	}
	core->DelTable(table);
}

//bg2
void GameScript::SetRestEncounterProbabilityDay(Scriptable* Sender, Action* parameters)
{
	Map *map=Sender->GetCurrentArea();
	map->RestHeader.DayChance = parameters->int0Parameter;
}

void GameScript::SetRestEncounterProbabilityNight(Scriptable* Sender, Action* parameters)
{
	Map *map=Sender->GetCurrentArea();
	map->RestHeader.NightChance = parameters->int0Parameter;
}

//iwd
void GameScript::SetRestEncounterChance(Scriptable * Sender, Action* parameters)
{
	Map *map=Sender->GetCurrentArea();
	map->RestHeader.DayChance = parameters->int0Parameter;
	map->RestHeader.NightChance = parameters->int1Parameter;
}

void GameScript::EndCredits(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	core->PlayMovie("credits");
}

void GameScript::ExpansionEndCredits(Scriptable* /*Sender*/, Action* /*parameters*/)
{
	core->PlayMovie("ecredit");
}

void GameScript::QuitGame(Scriptable* Sender, Action* parameters)
{
	ClearAllActions(Sender, parameters);
	core->quitflag = 1;
}

void GameScript::StopMoving(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) Sender;
	actor->ClearPath();
}

void GameScript::ApplyDamage(Scriptable* Sender, Action* parameters)
{
	Actor *damagee;
	Actor *damager;
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	damagee = (Actor *) tar;
	if (Sender->Type==ST_ACTOR) {
		damager=(Actor *) Sender;
	}
	else {
		damager=damagee;
	}
	damagee->Damage(parameters->int0Parameter, parameters->int1Parameter, damager);
}

void GameScript::ApplyDamagePercent(Scriptable* Sender, Action* parameters)
{
	Actor *damagee;
	Actor *damager;
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	damagee = (Actor *) tar;
	if (Sender->Type==ST_ACTOR) {
		damager=(Actor *) Sender;
	}
	else {
		damager=damagee;
	}
	damagee->Damage(damagee->GetStat(IE_HITPOINTS)*parameters->int0Parameter/100, parameters->int1Parameter, damager);
}

void GameScript::Damage(Scriptable* Sender, Action* parameters)
{
	Actor *damagee;
	Actor *damager;
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	damagee = (Actor *) tar;
	if (Sender->Type==ST_ACTOR) {
		damager=(Actor *) Sender;
	}
	else {
		damager=damagee;
	}
	int damage = core->Roll( (parameters->int1Parameter>>12)&15, (parameters->int1Parameter>>4)&255, parameters->int1Parameter&15 );
	int type=MOD_ADDITIVE;
	switch(parameters->int0Parameter) {
	case 2: //raise
		damage=-damage;
		break;
	case 3: //set
		type=MOD_ABSOLUTE;
		break;
	case 4: //
		type=MOD_PERCENT;
		break;
	}
	damagee->Damage( damage, type, damager );
}

void GameScript::SetHomeLocation(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	Moveble *movable = (Moveble *) tar; //not actor, though it is the only moveable
	movable->Destination = parameters->pointParameter;
	//no movement should be started here, i think
}

void GameScript::SetMasterArea(Scriptable* /*Sender*/, Action* parameters)
{
	core->GetGame()->SetMasterArea(parameters->string0Parameter);
}

void GameScript::Berserk(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *act = (Actor *) Sender;
	act->NewStat(IE_STATE_ID, act->GetStat(IE_STATE_ID)|STATE_BERSERK, MOD_ABSOLUTE);
}

void GameScript::Panic(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *act = (Actor *) Sender;
	act->Panic();
}

void GameScript::RevealAreaOnMap(Scriptable* /*Sender*/, Action* parameters)
{
	WorldMap *worldmap = core->GetWorldMap();
	worldmap->SetAreaStatus(parameters->string0Parameter, WMP_ENTRY_VISIBLE, BM_OR);
}

void GameScript::HideAreaOnMap( Scriptable* /*Sender*/, Action* parameters)
{
	WorldMap *worldmap = core->GetWorldMap();
	worldmap->SetAreaStatus(parameters->string0Parameter, WMP_ENTRY_VISIBLE, BM_NAND);
}

void GameScript::Shout( Scriptable* Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Map *map=Sender->GetCurrentArea();
	//max. shouting distance
	map->Shout(Sender, parameters->int0Parameter, 40);
}

void GameScript::GlobalShout( Scriptable* Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Map *map=Sender->GetCurrentArea();
	map->Shout(Sender, parameters->int0Parameter, 0);
}

void GameScript::Help( Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Map *map=Sender->GetCurrentArea();
	map->Shout(Sender, 0, 40);
}

void GameScript::AddMapnote( Scriptable* Sender, Action* parameters)
{
	Map *map=Sender->GetCurrentArea();
	char *str = core->GetString( parameters->int0Parameter, 0);
	map->AddMapNote(parameters->pointParameter, parameters->int1Parameter, str);
}

void GameScript::RemoveMapnote( Scriptable* Sender, Action* parameters)
{
	Map *map=Sender->GetCurrentArea();
	map->RemoveMapNote(parameters->pointParameter);
}

//It is possible to attack CONTAINERS/DOORS as well!!!
void GameScript::AttackCore(Scriptable *Sender, Scriptable *target, Action *parameters, int flags)
{
	//this is a dangerous cast, make sure actor is Actor * !!!
	Actor *actor = (Actor *) Sender;
	unsigned int wrange = actor->GetWeaponRange() * 10;
	if ( wrange == 0) {
		printMessage("[GameScript]","Zero weapon range!\n",LIGHT_RED);
		if (flags&AC_REEVALUATE) {
			delete parameters;
		}
		return;
	}
	if ( Distance(Sender, target) > wrange ) {
		GoNearAndRetry(Sender, target);
		if (flags&AC_REEVALUATE) {
			delete parameters;
		}
		return;
	}
	//TODO:
	//send Attack trigger to attacked
	//calculate attack/damage
	actor->SetStance( IE_ANI_ATTACK );
	actor->SetWait( 1 );
	//attackreevaluate
	if ( (flags&AC_REEVALUATE) && parameters->int0Parameter) {
		parameters->int0Parameter--;
		Sender->AddAction( parameters );
	}
}

void GameScript::Attack( Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || (tar->Type != ST_ACTOR && tar->Type !=ST_DOOR && tar->Type !=ST_CONTAINER) ) {
		return;
	}
	AttackCore(Sender, tar, NULL, 0);
}

void GameScript::ForceAttack( Scriptable* Sender, Action* parameters)
{
	Scriptable* scr = GetActorFromObject( Sender, parameters->objects[1] );
	if (!scr || scr->Type != ST_ACTOR) {
		return;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[2] );
	if (!tar || (tar->Type != ST_ACTOR && tar->Type !=ST_DOOR && tar->Type !=ST_CONTAINER) ) {
		return;
	}
	AttackCore(scr, tar, NULL, 0);
}

void GameScript::AttackReevaluate( Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || (tar->Type != ST_ACTOR && tar->Type !=ST_DOOR && tar->Type !=ST_CONTAINER) ) {
		return;
	}
	//pumping parameters back for AttackReevaluate
	//FIXME: we should make a copy of parameters here
	Action *newAction = ParamCopy( parameters );
	AttackCore(Sender, tar, newAction, AC_REEVALUATE);
}

void GameScript::Explore( Scriptable* Sender, Action* /*parameters*/)
{
	Sender->GetCurrentArea( )->Explore(0);
}

void GameScript::UndoExplore( Scriptable* Sender, Action* /*parameters*/)
{
	Sender->GetCurrentArea( )->Explore(-1);
}

void GameScript::ExploreMapChunk( Scriptable* Sender, Action* parameters)
{
	Map *map = Sender->GetCurrentArea();
	/*
	There is a mode flag in int1Parameter, but i don't know what is it,
	our implementation uses it for LOS=1, or no LOS=0
	ExploreMapChunk will reveal both visibility/explored map, but the
	visibility will fade in the next update cycle (which is quite frequent)
	*/
	map->ExploreMapChunk(parameters->pointParameter, parameters->int0Parameter, parameters->int1Parameter);
}

void GameScript::StartStore( Scriptable* Sender, Action* parameters)
{
	if (core->GetCurrentStore() ) {
		return;
	}
	core->SetCurrentStore( parameters->string0Parameter );
	core->GetGUIScriptEngine()->RunFunction( "OpenStoreWindow" );
	//sorry, i have absolutely no idea when i should do this :)
	Sender->CurrentAction = NULL;
}

//The integer parameter is a GemRB extension, if set to 1, the player
//gains experience for learning the spell
void GameScript::AddSpecialAbility( Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) Sender;
	actor->LearnSpell (parameters->string0Parameter, parameters->int0Parameter);
}

void GameScript::RemoveSpell( Scriptable* Sender, Action* parameters)
{
	if (Sender->Type!=ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) Sender;
	if (parameters->string0Parameter[0]) {
		actor->spellbook.HaveSpell(parameters->string0Parameter, HS_DEPLETE);
		return;
	}
	actor->spellbook.HaveSpell(parameters->int0Parameter, HS_DEPLETE);
/*
	ieResRef tmpname;
	CreateSpellName(tmpname, parameters->int0Parameter);
	actor->spellbook.HaveSpell(tmpname, HS_DEPLETE);
*/
}

void GameScript::SetScriptName( Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	tar->SetScriptName(parameters->string0Parameter);
}

void GameScript::AdvanceTime( Scriptable* /*Sender*/, Action* parameters)
{
	core->GetGame()->GameTime += parameters->int0Parameter;
}

//IWD2 special, can mark only actors, hope it is enough
void GameScript::MarkObject(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) Sender;
	actor->LastSeen = (Actor *) tar;
}

void GameScript::SetDialogueRange(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) Sender;
	actor->SetStat( IE_DIALOGRANGE, parameters->int0Parameter );
}

void GameScript::SetGlobalTint(Scriptable* /*Sender*/, Action* parameters)
{
	core->GetVideoDriver()->SetFadeColor(parameters->int0Parameter, parameters->int1Parameter, parameters->int2Parameter);
}

void GameScript::SetArmourLevel(Scriptable* Sender, Action* parameters)
{
	Scriptable* tar = GetActorFromObject( Sender, parameters->objects[1] );
	if (!tar || tar->Type!=ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) Sender;
	actor->NewStat( IE_ARMOR_TYPE, parameters->int0Parameter, MOD_ABSOLUTE );
}

void GameScript::RandomWalk(Scriptable* Sender, Action* /*parameters*/)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->RandomWalk( );
}

void GameScript::RandomFly(Scriptable* Sender, Action* parameters)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	int x = rand()&31;
	if (x<10) {
		actor->SetOrientation(actor->GetOrientation()-1, 0);
	} else if (x>20) {
		actor->SetOrientation(actor->GetOrientation()+1, 0);
	}
	//fly in this direction for 2 steps
	actor->MoveLine(2, GL_PASS);
	Sender->AddAction( parameters );
}

//UseContainer uses the predefined target (like Nidspecial1 dialog hack)
void GameScript::UseContainer(Scriptable* Sender, Action* /*parameters*/)
{
	GameControl* gc = core->GetGameControl();
	if (!gc || !gc->target || (gc->target->Type!=ST_CONTAINER) ) {
		Sender->CurrentAction = NULL;
		return;
	}
	Container *container = (Container *) gc->target;
	if (Sender->Type != ST_ACTOR) {
		Sender->CurrentAction = NULL;
		return;
	}
	double distance = Distance(Sender, gc->target);
	double needed = MAX_OPERATING_DISTANCE;
	if (container->Type==IE_CONTAINER_PILE) {
		needed = 15; // less than a search square (width)
	}
	if (distance<=needed)
	{
		//check if the container is unlocked
		if (container->Flags & CONT_LOCKED) {
			//playsound can't open container
			//display string, etc
			core->DisplayConstantString(STR_CONTLOCKED,0xff0000);
			Sender->CurrentAction = NULL;
			return;
		}
		core->SetCurrentContainer((Actor *) Sender, container);
		Sender->CurrentAction = NULL;
		return;
	}
	GoNearAndRetry(Sender, gc->target);
	Sender->CurrentAction = NULL;
}
