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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

// This class implements IEScript, the scripting language used in various scripts (compiled) and dialogs (plain text).
//
// Scriptable objects in GemRB execute script actions one-by-one from a queue,
// this is done in Scriptable::ProcessActions. If a blocking action is encountered
// (AF_BLOCKING in the actionnames table in GameScript.cpp) and CurrentAction is
// left set at the end of execution (ie, the action doesn't call
// ReleaseCurrentAction() on the Sender) then execution ends for that frame, and
// the same action is repeated the next time around. This behaviour is important
// because some state (eg, marked objects) is reset at the beginning of each action
// but must remain unchanged while the action is being executed.
//
// Scriptable::ExecuteScript is one way for the action queue to be changed; the
// triggers of all blocks are checked (note that some triggers have side-effects,
// for example See() changes LastSeen) until all triggers for a block evaluate as
// true, and then that block is executed. If actions from that block are already
// queued (from a previous round of checks) then nothing happens, so a block is not
// restarted until it is finished. This becomes a bit more complicated when
// Continue() is involved, be warned and make sure to check any behaviour changes
// against the original engine. Note that AF_INSTANT actions are executed
// instantly, rather than added to the queue.

#include "GameScript/GameScript.h"

#include "Game.h"
#include "GameData.h"
#include "Interface.h"
#include "PluginMgr.h"
#include "RNG.h"
#include "TableMgr.h"

#include "GUI/GameControl.h"
#include "GameScript/GSUtils.h"
#include "GameScript/Matching.h"

#include <cstdarg>

namespace GemRB {

//debug flags
// 1 - cache
// 2 - cutscene ID
// 4 - globals
// 8 - action execution
//16 - trigger evaluation

// test cases: tob pp summoning spirit, pst portals, pst AR0405, AR0508, ar0500 (guards through gates)
// it's about 3 times bigger in pst, perhaps related to the bigger sprite sizes and we modify it in Scriptable
// The distance of operating a trigger, container, dialog buffer etc.
unsigned int MAX_OPERATING_DISTANCE = 40; // a search square is 16x12 (diagonal of 20), so that's about two

//Make this an ordered list, so we could use bsearch!
static const TriggerLink triggernames[] = {
	{ "actionlistempty", GameScript::ActionListEmpty, 0 },
	{ "actuallyincombat", GameScript::ActuallyInCombat, 0 },
	{ "acquired", GameScript::Acquired, 0 },
	{ "alignment", GameScript::Alignment, 0 },
	{ "allegiance", GameScript::Allegiance, 0 },
	{ "animstate", GameScript::AnimState, 0 },
	{ "anypconmap", GameScript::AnyPCOnMap, 0 },
	{ "anypcseesenemy", GameScript::AnyPCSeesEnemy, 0 },
	{ "areacheck", GameScript::AreaCheck, 0 },
	{ "areacheckobject", GameScript::AreaCheckObject, 0 },
	{ "areacheckallegiance", GameScript::AreaCheckAllegiance, 0 },
	{ "areaflag", GameScript::AreaFlag, 0 },
	{ "arearestdisabled", GameScript::AreaRestDisabled, 0 },
	{ "areatype", GameScript::AreaType, 0 },
	{ "assaltedby", GameScript::AttackedBy, 0 }, //pst
	{ "assign", GameScript::Assign, 0 },
	{ "atlocation", GameScript::AtLocation, 0 },
	{ "attackedby", GameScript::AttackedBy, 0 },
	{ "becamevisible", GameScript::BecameVisible, 0 },
	{ "beeninparty", GameScript::BeenInParty, 0 },
	{ "bitcheck", GameScript::BitCheck, TF_MERGESTRINGS },
	{ "bitcheckexact", GameScript::BitCheckExact, TF_MERGESTRINGS },
	{ "bitglobal", GameScript::BitGlobal_Trigger, TF_MERGESTRINGS },
	{ "bouncingspelllevel", GameScript::BouncingSpellLevel, 0 },
	{ "breakingpoint", GameScript::BreakingPoint, 0 },
	{ "buttondisabled", GameScript::ButtonDisabled, 0 },
	{ "calanderday", GameScript::CalendarDay, 0 }, //illiterate developers O_o
	{ "calendarday", GameScript::CalendarDay, 0 },
	{ "calanderdaygt", GameScript::CalendarDayGT, 0 },
	{ "calendardaygt", GameScript::CalendarDayGT, 0 },
	{ "calanderdaylt", GameScript::CalendarDayLT, 0 },
	{ "calendardaylt", GameScript::CalendarDayLT, 0 },
	{ "calledbyname", GameScript::CalledByName, 0 }, //this is still a question
	{ "canequipranged", GameScript::CanEquipRanged, 0 },
	{ "canturn", GameScript::CanTurn, 0 },
	{ "chargecount", GameScript::ChargeCount, 0 },
	{ "charname", GameScript::CharName, 0 }, //not scripting name
	{ "checkareadifflevel", GameScript::CheckAreaDiffLevel, 0 }, //iwd2
	{ "checkdoorflags", GameScript::CheckDoorFlags, 0 },
	{ "checkitemslot", GameScript::HasItemSlot, 0 },
	{ "checkpartyaveragelevel", GameScript::CheckPartyAverageLevel, 0 },
	{ "checkpartylevel", GameScript::CheckPartyLevel, 0 },
	{ "checkskill", GameScript::CheckSkill, 0 },
	{ "checkskillgt", GameScript::CheckSkillGT, 0 },
	{ "checkskilllt", GameScript::CheckSkillLT, 0 },
	{ "checkspellstate", GameScript::CheckSpellState, 0 },
	{ "checkstat", GameScript::CheckStat, 0 },
	{ "checkstatgt", GameScript::CheckStatGT, 0 },
	{ "checkstatlt", GameScript::CheckStatLT, 0 },
	{ "class", GameScript::Class, 0 },
	{ "classex", GameScript::ClassEx, 0 }, //will return true for multis
	{ "classlevel", GameScript::ClassLevel, 0 }, //pst
	{ "classlevelgt", GameScript::ClassLevelGT, 0 },
	{ "classlevellt", GameScript::ClassLevelLT, 0 },
	{ "clicked", GameScript::Clicked, 0 },
	{ "closed", GameScript::Closed, 0 },
	{ "combatcounter", GameScript::CombatCounter, 0 },
	{ "combatcountergt", GameScript::CombatCounterGT, 0 },
	{ "combatcounterlt", GameScript::CombatCounterLT, 0 },
	{ "contains", GameScript::Contains, 0 },
	{ "currentammo", GameScript::CurrentAmmo, 0 },
	{ "currentareais", GameScript::CurrentAreaIs, 0 }, //checks object
	{ "cutscenebroken", GameScript::CutSceneBroken, 0 },
	{ "creaturehidden", GameScript::CreatureHidden, 0 }, //this is the engine level hiding feature, not the skill
	{ "creatureinarea", GameScript::AreaCheck, 0 }, //pst, checks this object
	{ "damagetaken", GameScript::DamageTaken, 0 },
	{ "damagetakengt", GameScript::DamageTakenGT, 0 },
	{ "damagetakenlt", GameScript::DamageTakenLT, 0 },
	{ "dead", GameScript::Dead, 0 },
	{ "delay", GameScript::Delay, 0 },
	{ "detect", GameScript::Detect, 0 }, //so far i see no difference
	{ "detected", GameScript::Detected, 0 }, //trap or secret door detected
	{ "die", GameScript::Die, 0 },
	{ "died", GameScript::Died, 0 },
	{ "difficulty", GameScript::Difficulty, 0 },
	{ "difficultygt", GameScript::DifficultyGT, 0 },
	{ "difficultylt", GameScript::DifficultyLT, 0 },
	{ "disarmed", GameScript::Disarmed, 0 },
	{ "disarmfailed", GameScript::DisarmFailed, 0 },
	{ "e", GameScript::E, 0 },
	{ "entered", GameScript::Entered, 0 },
	{ "entirepartyonmap", GameScript::EntirePartyOnMap, 0 },
	{ "exists", GameScript::Exists, 0 },
	{ "extendedstatecheck", GameScript::ExtendedStateCheck, 0 },
	{ "extraproficiency", GameScript::ExtraProficiency, 0 },
	{ "extraproficiencygt", GameScript::ExtraProficiencyGT, 0 },
	{ "extraproficiencylt", GameScript::ExtraProficiencyLT, 0 },
	{ "eval", GameScript::Eval, 0 },
	{ "faction", GameScript::Faction, 0 },
	{ "failedtoopen", GameScript::OpenFailed, 0 },
	{ "fallenpaladin", GameScript::FallenPaladin, 0 },
	{ "fallenranger", GameScript::FallenRanger, 0 },
	{ "false", GameScript::False, 0 },
	{ "forcemarkedspell", GameScript::ForceMarkedSpell_Trigger, 0 },
	{ "frame", GameScript::Frame, 0 },
	{ "g", GameScript::G_Trigger, 0 },
	{ "gender", GameScript::Gender, 0 },
	{ "general", GameScript::General, 0 },
	{ "ggt", GameScript::GGT_Trigger, 0 },
	{ "glt", GameScript::GLT_Trigger, 0 },
	{ "global", GameScript::Global, TF_MERGESTRINGS },
	{ "globalandglobal", GameScript::GlobalAndGlobal_Trigger, TF_MERGESTRINGS },
	{ "globalband", GameScript::BitCheck, TF_MERGESTRINGS },
	{ "globalbandglobal", GameScript::GlobalBAndGlobal_Trigger, TF_MERGESTRINGS },
	{ "globalbandglobalexact", GameScript::GlobalBAndGlobalExact, TF_MERGESTRINGS },
	{ "globalbitglobal", GameScript::GlobalBitGlobal_Trigger, TF_MERGESTRINGS },
	{ "globalequalsglobal", GameScript::GlobalsEqual, TF_MERGESTRINGS }, //this is the same
	{ "globalgt", GameScript::GlobalGT, TF_MERGESTRINGS },
	{ "globalgtglobal", GameScript::GlobalGTGlobal, TF_MERGESTRINGS },
	{ "globallt", GameScript::GlobalLT, TF_MERGESTRINGS },
	{ "globalltglobal", GameScript::GlobalLTGlobal, TF_MERGESTRINGS },
	{ "globalorglobal", GameScript::GlobalOrGlobal_Trigger, TF_MERGESTRINGS },
	{ "globalsequal", GameScript::GlobalsEqual, 0 },
	{ "globalsgt", GameScript::GlobalsGT, 0 },
	{ "globalslt", GameScript::GlobalsLT, 0 },
	{ "globaltimerexact", GameScript::GlobalTimerExact, 0 },
	{ "globaltimerexpired", GameScript::GlobalTimerExpired, 0 },
	{ "globaltimernotexpired", GameScript::GlobalTimerNotExpired, 0 },
	{ "globaltimerstarted", GameScript::GlobalTimerStarted, 0 },
	{ "gt", GameScript::GT, 0 },
	{ "happiness", GameScript::Happiness, 0 },
	{ "happinessgt", GameScript::HappinessGT, 0 },
	{ "happinesslt", GameScript::HappinessLT, 0 },
	{ "harmlessclosed", GameScript::HarmlessClosed, 0 }, //pst
	{ "harmlessentered", GameScript::HarmlessEntered, 0 }, //pst
	{ "harmlessopened", GameScript::HarmlessOpened, 0 }, //pst
	{ "hasbounceeffects", GameScript::HasBounceEffects, 0 },
	{ "hasdlc", GameScript::HasDLC, 0 },
	{ "hasimmunityeffects", GameScript::HasImmunityEffects, 0 },
	{ "hasinnateability", GameScript::HaveSpell, 0 }, //these must be the same
	{ "hasitem", GameScript::HasItem, 0 },
	{ "hasitemcategory", GameScript::HasItemCategory, 0 },
	{ "hasitemequiped", GameScript::HasItemEquipped, 0 }, //typo in bg2
	{ "hasitemequipedreal", GameScript::HasItemEquippedReal, 0 },
	{ "hasitemequipped", GameScript::HasItemEquipped, 0 },
	{ "hasitemequippedreal", GameScript::HasItemEquippedReal, 0 },
	{ "hasiteminslot", GameScript::HasItemSlot, 0 },
	{ "hasitemslot", GameScript::HasItemSlot, 0 },
	{ "hasitemtypeslot", GameScript::HasItemTypeSlot, 0 }, //gemrb extension
	{ "hasweaponequiped", GameScript::HasWeaponEquipped, 0 }, //a typo again
	{ "hasweaponequipped", GameScript::HasWeaponEquipped, 0 },
	{ "haveanyspells", GameScript::HaveAnySpells, 0 },
	{ "haveknownspell", GameScript::KnowSpell, 0 },
	{ "haveknownspellres", GameScript::KnowSpell, 0 },
	{ "havespell", GameScript::HaveSpell, 0 }, //these must be the same
	{ "havespellparty", GameScript::HaveSpellParty, 0 },
	{ "havespellres", GameScript::HaveSpell, 0 }, //they share the same ID
	{ "haveusableweaponequipped", GameScript::HaveUsableWeaponEquipped, 0 },
	{ "heard", GameScript::Heard, 0 },
	{ "help", GameScript::Help_Trigger, 0 },
	{ "helpex", GameScript::HelpEX, 0 },
	{ "hitby", GameScript::HitBy, 0 },
	{ "hotkey", GameScript::HotKey, 0 },
	{ "hp", GameScript::HP, 0 },
	{ "hpgt", GameScript::HPGT, 0 },
	{ "hplost", GameScript::HPLost, 0 },
	{ "hplostgt", GameScript::HPLostGT, 0 },
	{ "hplostlt", GameScript::HPLostLT, 0 },
	{ "hplt", GameScript::HPLT, 0 },
	{ "hppercent", GameScript::HPPercent, 0 },
	{ "hppercentgt", GameScript::HPPercentGT, 0 },
	{ "hppercentlt", GameScript::HPPercentLT, 0 },
	{ "ifvalidforpartydialog", GameScript::IsValidForPartyDialog, 0 },
	{ "ifvalidforpartydialogue", GameScript::IsValidForPartyDialog, 0 },
	{ "immunetospelllevel", GameScript::ImmuneToSpellLevel, 0 },
	{ "inactivearea", GameScript::InActiveArea, 0 },
	{ "incutscenemode", GameScript::InCutSceneMode, 0 },
	{ "ini", GameScript::INI, 0 },
	{ "inline", GameScript::InLine, 0 },
	{ "inmyarea", GameScript::InMyArea, 0 },
	{ "inmygroup", GameScript::InMyGroup, 0 },
	{ "inparty", GameScript::InParty, 0 },
	{ "inpartyallowdead", GameScript::InPartyAllowDead, 0 },
	{ "inpartyslot", GameScript::InPartySlot, 0 },
	{ "internal", GameScript::Internal, 0 },
	{ "internalgt", GameScript::InternalGT, 0 },
	{ "internallt", GameScript::InternalLT, 0 },
	{ "interactingwith", GameScript::InteractingWith, 0 },
	{ "intrap", GameScript::InTrap, 0 },
	{ "inventoryfull", GameScript::InventoryFull, 0 },
	{ "inview", GameScript::LOS, 0 }, //it seems the same, needs research; uses eyesight.ids for int param suggesting angular checks, but is unused
	{ "inwatcherskeep", GameScript::AreaStartsWith, 0 },
	{ "inweaponrange", GameScript::InWeaponRange, 0 },
	{ "isaclown", GameScript::IsAClown, 0 },
	{ "isactive", GameScript::IsActive, 0 },
	{ "isanimationid", GameScript::AnimationID, 0 },
	{ "iscreatureareaflag", GameScript::IsCreatureAreaFlag, 0 },
	{ "iscreaturehiddeninshadows", GameScript::IsCreatureHiddenInShadows, 0 },
	{ "isfacingobject", GameScript::IsFacingObject, 0 },
	{ "isfacingsavedrotation", GameScript::IsFacingSavedRotation, 0 },
	{ "isforcedrandomencounteractive", GameScript::IsForcedRandomEncounterActive, 0 },
	{ "isgabber", GameScript::IsGabber, 0 },
	{ "isheartoffurymodeon", GameScript::NightmareModeOn, 0 },
	{ "isinguardianmantle", GameScript::IsInGuardianMantle, 0 },
	{ "islocked", GameScript::IsLocked, 0 },
	{ "isextendednight", GameScript::IsExtendedNight, 0 },
	{ "ismarkedspell", GameScript::IsMarkedSpell, 0 },
	{ "isoverme", GameScript::IsOverMe, 0 },
	{ "ispathcriticalobject", GameScript::IsPathCriticalObject, 0 },
	{ "isplayernumber", GameScript::IsPlayerNumber, 0 },
	{ "isrotation", GameScript::IsRotation, 0 },
	{ "isscriptname", GameScript::CalledByName, 0 }, //seems the same
	{ "isspelltargetvalid", GameScript::IsSpellTargetValid, 0 },
	{ "isteambiton", GameScript::IsTeamBitOn, 0 },
	{ "istouchgui", GameScript::IsTouchGUI, 0 },
	{ "isvalidforpartydialog", GameScript::IsValidForPartyDialog, 0 },
	{ "isvalidforpartydialogue", GameScript::IsValidForPartyDialog, 0 },
	{ "isweaponranged", GameScript::IsWeaponRanged, 0 },
	{ "isweather", GameScript::IsWeather, 0 }, //gemrb extension
	{ "itemisidentified", GameScript::ItemIsIdentified, 0 },
	{ "joins", GameScript::Joins, 0 },
	{ "killed", GameScript::Killed, 0 },
	{ "kit", GameScript::Kit, 0 },
	{ "knowspell", GameScript::KnowSpell, 0 }, // gemrb specific, but also reused for ees
	{ "lastmarkedobject", GameScript::LastMarkedObject_Trigger, 0 },
	{ "lastpersontalkedto", GameScript::LastPersonTalkedTo, 0 }, //pst
	{ "leaves", GameScript::Leaves, 0 },
	{ "level", GameScript::Level, 0 },
	{ "levelgt", GameScript::LevelGT, 0 },
	{ "levelinclass", GameScript::LevelInClass, 0 }, //iwd2
	{ "levelinclassgt", GameScript::LevelInClassGT, 0 },
	{ "levelinclasslt", GameScript::LevelInClassLT, 0 },
	{ "levellt", GameScript::LevelLT, 0 },
	{ "levelparty", GameScript::LevelParty, 0 },
	{ "levelpartygt", GameScript::LevelPartyGT, 0 },
	{ "levelpartylt", GameScript::LevelPartyLT, 0 },
	{ "localsequal", GameScript::LocalsEqual, 0 },
	{ "localsgt", GameScript::LocalsGT, 0 },
	{ "localslt", GameScript::LocalsLT, 0 },
	{ "los", GameScript::LOS, 0 },
	{ "lt", GameScript::LT, 0 },
	{ "modalstate", GameScript::ModalState, 0 },
	{ "modalstateobject", GameScript::ModalState, 0 },
	{ "morale", GameScript::Morale, 0 },
	{ "moralegt", GameScript::MoraleGT, 0 },
	{ "moralelt", GameScript::MoraleLT, 0 },
	{ "movementrate", GameScript::MovementRate, 0 },
	{ "movementrategt", GameScript::MovementRateGT, 0 },
	{ "movementratelt", GameScript::MovementRateLT, 0 },
	{ "name", GameScript::CalledByName, 0 }, //this is the same too?
	{ "namelessbitthedust", GameScript::NamelessBitTheDust, 0 },
	{ "nearbydialog", GameScript::NearbyDialog, 0 },
	{ "nearbydialogue", GameScript::NearbyDialog, 0 },
	{ "nearlocation", GameScript::NearLocation, 0 },
	{ "nearsavedlocation", GameScript::NearSavedLocation, 0 },
	{ "nexttriggerobject", NULL, 0 }, // handled inline
	{ "nightmaremodeon", GameScript::NightmareModeOn, 0 },
	{ "notstatecheck", GameScript::NotStateCheck, 0 },
	{ "nulldialog", GameScript::NullDialog, 0 },
	{ "nulldialogue", GameScript::NullDialog, 0 },
	{ "numbouncingspelllevel", GameScript::NumBouncingSpellLevel, 0 },
	{ "numbouncingspelllevelgt", GameScript::NumBouncingSpellLevelGT, 0 },
	{ "numbouncingspelllevellt", GameScript::NumBouncingSpellLevelLT, 0 },
	{ "numcreature", GameScript::NumCreatures, 0 },
	{ "numcreaturegt", GameScript::NumCreaturesGT, 0 },
	{ "numcreaturelt", GameScript::NumCreaturesLT, 0 },
	{ "numcreaturesatmylevel", GameScript::NumCreaturesAtMyLevel, 0 },
	{ "numcreaturesgtmylevel", GameScript::NumCreaturesGTMyLevel, 0 },
	{ "numcreaturesltmylevel", GameScript::NumCreaturesLTMyLevel, 0 },
	{ "numcreaturevsparty", GameScript::NumCreatureVsParty, 0 },
	{ "numcreaturevspartygt", GameScript::NumCreatureVsPartyGT, 0 },
	{ "numcreaturevspartylt", GameScript::NumCreatureVsPartyLT, 0 },
	{ "numdead", GameScript::NumDead, 0 },
	{ "numdeadgt", GameScript::NumDeadGT, 0 },
	{ "numdeadlt", GameScript::NumDeadLT, 0 },
	{ "numimmunetospelllevel", GameScript::NumImmuneToSpellLevel, 0 },
	{ "numimmunetospelllevelgt", GameScript::NumImmuneToSpellLevelGT, 0 },
	{ "numimmunetospelllevellt", GameScript::NumImmuneToSpellLevelLT, 0 },
	{ "numinparty", GameScript::PartyCountEQ, 0 },
	{ "numinpartyalive", GameScript::PartyCountAliveEQ, 0 },
	{ "numinpartyalivegt", GameScript::PartyCountAliveGT, 0 },
	{ "numinpartyalivelt", GameScript::PartyCountAliveLT, 0 },
	{ "numinpartygt", GameScript::PartyCountGT, 0 },
	{ "numinpartylt", GameScript::PartyCountLT, 0 },
	{ "numitems", GameScript::NumItems, 0 },
	{ "numitemsgt", GameScript::NumItemsGT, 0 },
	{ "numitemslt", GameScript::NumItemsLT, 0 },
	{ "numitemsparty", GameScript::NumItemsParty, 0 },
	{ "numitemspartygt", GameScript::NumItemsPartyGT, 0 },
	{ "numitemspartylt", GameScript::NumItemsPartyLT, 0 },
	{ "numkilledbyparty", GameScript::NumKilledByParty, 0 },
	{ "numkilledbypartygt", GameScript::NumKilledByPartyGT, 0 },
	{ "numkilledbypartylt", GameScript::NumKilledByPartyLT, 0 },
	{ "nummirrorimages", GameScript::NumMirrorImages, 0 },
	{ "nummirrorimagesgt", GameScript::NumMirrorImagesGT, 0 },
	{ "nummirrorimageslt", GameScript::NumMirrorImagesLT, 0 },
	{ "numtimesinteracted", GameScript::NumTimesInteracted, 0 },
	{ "numtimesinteractedgt", GameScript::NumTimesInteractedGT, 0 },
	{ "numtimesinteractedlt", GameScript::NumTimesInteractedLT, 0 },
	{ "numtimesinteractedobject", GameScript::NumTimesInteractedObject, 0 }, //gemrb
	{ "numtimesinteractedobjectgt", GameScript::NumTimesInteractedObjectGT, 0 }, //gemrb
	{ "numtimesinteractedobjectlt", GameScript::NumTimesInteractedObjectLT, 0 }, //gemrb
	{ "numtimestalkedto", GameScript::NumTimesTalkedTo, 0 },
	{ "numtimestalkedtogt", GameScript::NumTimesTalkedToGT, 0 },
	{ "numtimestalkedtolt", GameScript::NumTimesTalkedToLT, 0 },
	{ "numtrappingspelllevel", GameScript::NumTrappingSpellLevel, 0 },
	{ "numtrappingspelllevelgt", GameScript::NumTrappingSpellLevelGT, 0 },
	{ "numtrappingspelllevellt", GameScript::NumTrappingSpellLevelLT, 0 },
	{ "objectactionlistempty", GameScript::ObjectActionListEmpty, 0 }, //same function
	{ "objitemcounteq", GameScript::NumItems, 0 },
	{ "objitemcountgt", GameScript::NumItemsGT, 0 },
	{ "objitemcountlt", GameScript::NumItemsLT, 0 },
	{ "oncreation", GameScript::OnCreation, 0 },
	{ "onisland", GameScript::OnIsland, 0 },
	{ "onscreen", GameScript::OnScreen, 0 },
	{ "opened", GameScript::Opened, 0 },
	{ "openfailed", GameScript::OpenFailed, 0 },
	{ "openstate", GameScript::OpenState, 0 },
	{ "or", GameScript::Or, 0 },
	{ "originalclass", GameScript::OriginalClass, 0 },
	{ "outofammo", GameScript::OutOfAmmo, 0 },
	{ "ownsfloatermessage", GameScript::OwnsFloaterMessage, 0 },
	{ "partycounteq", GameScript::PartyCountEQ, 0 },
	{ "partycountgt", GameScript::PartyCountGT, 0 },
	{ "partycountlt", GameScript::PartyCountLT, 0 },
	{ "partygold", GameScript::PartyGold, 0 },
	{ "partygoldgt", GameScript::PartyGoldGT, 0 },
	{ "partygoldlt", GameScript::PartyGoldLT, 0 },
	{ "partyhasitem", GameScript::PartyHasItem, 0 },
	{ "partyhasitemidentified", GameScript::PartyHasItemIdentified, 0 },
	{ "partyitemcounteq", GameScript::NumItemsParty, 0 },
	{ "partyitemcountgt", GameScript::NumItemsPartyGT, 0 },
	{ "partyitemcountlt", GameScript::NumItemsPartyLT, 0 },
	{ "partylevelvs", GameScript::NumCreatureVsParty, 0 },
	{ "partylevelvsgt", GameScript::NumCreatureVsPartyGT, 0 },
	{ "partylevelvslt", GameScript::NumCreatureVsPartyLT, 0 },
	{ "partymemberdied", GameScript::PartyMemberDied, 0 },
	{ "partyrested", GameScript::PartyRested, 0 },
	{ "pccanseepoint", GameScript::PCCanSeePoint, 0 },
	{ "pcinstore", GameScript::PCInStore, 0 },
	{ "personalspacedistance", GameScript::PersonalSpaceDistance, 0 },
	{ "picklockfailed", GameScript::PickLockFailed, 0 },
	{ "pickpocketfailed", GameScript::PickpocketFailed, 0 },
	{ "proficiency", GameScript::Proficiency, 0 },
	{ "proficiencygt", GameScript::ProficiencyGT, 0 },
	{ "proficiencylt", GameScript::ProficiencyLT, 0 },
	{ "race", GameScript::Race, 0 },
	{ "randomnum", GameScript::RandomNum, 0 },
	{ "randomnumgt", GameScript::RandomNumGT, 0 },
	{ "randomnumlt", GameScript::RandomNumLT, 0 },
	{ "randomstatcheck", GameScript::RandomStatCheck, 0 },
	{ "range", GameScript::Range, 0 },
	{ "reaction", GameScript::Reaction, 0 },
	{ "reactiongt", GameScript::ReactionGT, 0 },
	{ "reactionlt", GameScript::ReactionLT, 0 },
	{ "realglobaltimerexact", GameScript::RealGlobalTimerExact, 0 },
	{ "realglobaltimerexpired", GameScript::RealGlobalTimerExpired, 0 },
	{ "realglobaltimernotexpired", GameScript::RealGlobalTimerNotExpired, 0 },
	{ "receivedorder", GameScript::ReceivedOrder, 0 },
	{ "reputation", GameScript::Reputation, 0 },
	{ "reputationgt", GameScript::ReputationGT, 0 },
	{ "reputationlt", GameScript::ReputationLT, 0 },
	{ "reserved", nullptr, 0 },
	{ "reserved1", nullptr, 0 },
	{ "reserved2", nullptr, 0 },
	{ "reserved3", nullptr, 0 },
	{ "reset", GameScript::Reset, 0 },
	{ "said", GameScript::False, 0 },
	{ "school", GameScript::School, 0 }, //similar to kit
	{ "secretdoordetected", GameScript::SecretDoorDetected, 0 },
	{ "see", GameScript::See, 0 },
	{ "sequence", GameScript::Sequence, 0 },
	{ "setlastmarkedobject", GameScript::SetLastMarkedObject, 0 },
	{ "setmarkedspell", GameScript::SetMarkedSpell_Trigger, 0 },
	{ "setspelltarget", GameScript::SetSpellTarget, 0 },
	{ "specifics", GameScript::Specifics, 0 },
	{ "spellcast", GameScript::SpellCast, 0 },
	{ "spellcastinnate", GameScript::SpellCastInnate, 0 },
	{ "spellcastonme", GameScript::SpellCastOnMe, 0 },
	{ "spellcastpriest", GameScript::SpellCastPriest, 0 },
	{ "statecheck", GameScript::StateCheck, 0 },
	{ "stealfailed", GameScript::StealFailed, 0 },
	{ "storehasitem", GameScript::StoreHasItem, 0 },
	{ "storymodeon", GameScript::StoryModeOn, 0 },
	{ "stuffglobalrandom", GameScript::StuffGlobalRandom, 0 }, //hm, this is a trigger
	{ "subrace", GameScript::SubRace, 0 },
	{ "summoned", GameScript::Summoned, 0 },
	{ "summoninglimit", GameScript::SummoningLimit, 0 },
	{ "summoninglimitgt", GameScript::SummoningLimitGT, 0 },
	{ "summoninglimitlt", GameScript::SummoningLimitLT, 0 },
	{ "switch", GameScript::Switch, 0 },
	{ "systemvariable", GameScript::SystemVariable_Trigger, 0 }, //gemrb
	{ "targetunreachable", GameScript::TargetUnreachable, 0 },
	{ "team", GameScript::Team, 0 },
	{ "time", GameScript::Time, 0 },
	{ "timegt", GameScript::TimeGT, 0 },
	{ "timelt", GameScript::TimeLT, 0 },
	{ "timeofday", GameScript::TimeOfDay, 0 },
	{ "timeractive", GameScript::TimerActive, 0 },
	{ "timerexpired", GameScript::TimerExpired, 0 },
	{ "timestopcounter", GameScript::TimeStopCounter, 0 },
	{ "timestopcountergt", GameScript::TimeStopCounterGT, 0 },
	{ "timestopcounterlt", GameScript::TimeStopCounterLT, 0 },
	{ "timestopobject", GameScript::TimeStopObject, 0 },
	{ "tookdamage", GameScript::TookDamage, 0 },
	{ "totalitemcnt", GameScript::TotalItemCnt, 0 }, //iwd2
	{ "totalitemcntexclude", GameScript::TotalItemCntExclude, 0 }, //iwd2
	{ "totalitemcntexcludegt", GameScript::TotalItemCntExcludeGT, 0 }, //iwd2
	{ "totalitemcntexcludelt", GameScript::TotalItemCntExcludeLT, 0 }, //iwd2
	{ "totalitemcntgt", GameScript::TotalItemCntGT, 0 }, //iwd2
	{ "totalitemcntlt", GameScript::TotalItemCntLT, 0 }, //iwd2
	{ "traptriggered", GameScript::TrapTriggered, 0 },
	{ "trigger", GameScript::TriggerTrigger, 0 },
	{ "triggerclick", GameScript::Clicked, 0 }, //not sure
	{ "triggersetglobal", GameScript::TriggerSetGlobal, 0 }, //iwd2, but never used
	{ "true", GameScript::True, 0 },
	{ "turnedby", GameScript::TurnedBy, 0 },
	{ "unlocked", GameScript::Unlocked, 0 },
	{ "unselectablevariable", GameScript::UnselectableVariable, 0 },
	{ "unselectablevariablegt", GameScript::UnselectableVariableGT, 0 },
	{ "unselectablevariablelt", GameScript::UnselectableVariableLT, 0 },
	{ "unusable", GameScript::Unusable, 0 },
	{ "usedexit", GameScript::UsedExit, 0 }, //pst unhardcoded trigger for protagonist teleport
	{ "vacant", GameScript::Vacant, 0 },
	{ "walkedtotrigger", GameScript::WalkedToTrigger, 0 },
	{ "wasindialog", GameScript::WasInDialog, 0 },
	{ "weaponcandamage", GameScript::WeaponCanDamage, 0 },
	{ "weaponeffectivevs", GameScript::WeaponEffectiveVs, 0 },
	{ "xor", GameScript::Xor, TF_MERGESTRINGS },
	{ "xp", GameScript::XP, 0 },
	{ "xpgt", GameScript::XPGT, 0 },
	{ "xplt", GameScript::XPLT, 0 },
	{ NULL, NULL, 0 }
};

//Make this an ordered list, so we could use bsearch!
static const ActionLink actionnames[] = {
	{ "actionoverride", GameScript::NoAction, AF_INVALID }, // will never be reached, but this sooths other references
	{ "activate", GameScript::Activate, 0 },
	{ "activateportalcursor", GameScript::ActivatePortalCursor, 0 },
	{ "addareaflag", GameScript::AddAreaFlag, 0 },
	{ "addareatype", GameScript::AddAreaType, 0 },
	{ "addexperienceparty", GameScript::AddExperienceParty, 0 },
	{ "addexperiencepartycr", GameScript::AddExperiencePartyCR, 0 },
	{ "addexperiencepartyglobal", GameScript::AddExperiencePartyGlobal, 0 },
	{ "addfamiliar", GameScript::AddFamiliar, 0 },
	{ "addfeat", GameScript::AddFeat, 0 },
	{ "addglobals", GameScript::AddGlobals, 0 },
	{ "addhp", GameScript::AddHP, 0 },
	{ "addjournalentry", GameScript::AddJournalEntry, 0 },
	{ "addkit", GameScript::AddKit, 0 },
	{ "addmapnote", GameScript::AddMapnote, 0 },
	{ "addmapnotecolor", GameScript::AddMapnote, 0 },
	{ "addpartyexperience", GameScript::AddExperienceParty, 0 },
	{ "addspecialability", GameScript::AddSpecialAbility, 0 },
	{ "addsuperkit", GameScript::AddSuperKit, 0 },
	{ "addstoreitem", GameScript::AddStoreItem, 0 },
	{ "addwaypoint", GameScript::AddWayPoint, AF_BLOCKING },
	{ "addworldmapareaflag", GameScript::AddWorldmapAreaFlag, 0 },
	{ "addxp2da", GameScript::AddXP2DA, 0 },
	{ "addxpobject", GameScript::AddXPObject, 0 },
	{ "addxpvar", GameScript::AddXPVar, AF_INSTANT },
	{ "addxpworth", GameScript::AddXPWorth, 0 },
	{ "addxpworthonce", GameScript::AddXPWorth, 0 },
	{ "advancetime", GameScript::AdvanceTime, 0 },
	{ "allowarearesting", GameScript::SetAreaRestFlag, 0 }, //iwd2
	{ "ally", GameScript::Ally, 0 },
	{ "ambientactivate", GameScript::AmbientActivate, 0 },
	{ "ankhegemerge", GameScript::AnkhegEmerge, AF_ALIVE },
	{ "ankheghide", GameScript::AnkhegHide, AF_ALIVE },
	{ "applydamage", GameScript::ApplyDamage, 0 },
	{ "applydamagepercent", GameScript::ApplyDamagePercent, 0 },
	{ "applyspell", GameScript::ApplySpell, AF_IWD2_OVERRIDE },
	{ "applyspellpoint", GameScript::ApplySpellPoint, 0 }, //gemrb extension
	{ "attachtransitiontodoor", GameScript::AttachTransitionToDoor, 0 },
	{ "attack", GameScript::Attack, AF_BLOCKING | AF_ALIVE },
	{ "attacknosound", GameScript::AttackNoSound, AF_BLOCKING | AF_ALIVE }, //no sound yet anyway
	{ "attackoneround", GameScript::AttackOneRound, AF_BLOCKING | AF_ALIVE },
	{ "attackreevaluate", GameScript::AttackReevaluate, AF_BLOCKING | AF_ALIVE },
	{ "backstab", GameScript::Attack, AF_BLOCKING | AF_ALIVE }, //actually hide+attack
	{ "banterblockflag", GameScript::BanterBlockFlag, 0 },
	{ "banterblocktime", GameScript::BanterBlockTime, 0 },
	{ "bashdoor", GameScript::BashDoor, AF_BLOCKING | AF_ALIVE }, //the same until we know better
	{ "battlesong", GameScript::BattleSong, AF_ALIVE },
	{ "berserk", GameScript::Berserk, AF_ALIVE },
	{ "bitclear", GameScript::BitClear, AF_MERGESTRINGS },
	{ "bitglobal", GameScript::BitGlobal, AF_MERGESTRINGS },
	{ "bitset", GameScript::GlobalBOr, AF_MERGESTRINGS }, //probably the same
	{ "breakinstants", GameScript::BreakInstants, AF_BLOCKING }, //delay execution of instants to the next AI cycle???
	{ "buystuff", GameScript::NoAction, 0 },
	{ "calllightning", GameScript::Kill, 0 }, // just an instant death with Param2 = 0x100
	{ "calm", GameScript::Calm, 0 },
	{ "changeaiscript", GameScript::ChangeAIScript, 0 },
	{ "changeaitype", GameScript::ChangeAIType, 0 },
	{ "changealignment", GameScript::ChangeAlignment, 0 },
	{ "changeallegiance", GameScript::ChangeEnemyAlly, 0 },
	{ "changeanimation", GameScript::ChangeAnimation, 0 },
	{ "changeanimationnoeffect", GameScript::ChangeAnimationNoEffect, 0 },
	{ "changeclass", GameScript::ChangeClass, 0 },
	{ "changecolor", GameScript::ChangeColor, 0 },
	{ "changecurrentscript", GameScript::ChangeAIScript, AF_SCRIPTLEVEL },
	{ "changedestination", GameScript::ChangeDestination, 0 }, //gemrb extension (iwd hack)
	{ "changedialog", GameScript::ChangeDialogue, 0 },
	{ "changedialogue", GameScript::ChangeDialogue, 0 },
	{ "changegender", GameScript::ChangeGender, 0 },
	{ "changegeneral", GameScript::ChangeGeneral, 0 },
	{ "changeenemyally", GameScript::ChangeEnemyAlly, 0 },
	{ "changefaction", GameScript::SetFaction, 0 }, //pst
	{ "changerace", GameScript::ChangeRace, 0 },
	{ "changespecifics", GameScript::ChangeSpecifics, 0 },
	{ "changestat", GameScript::ChangeStat, 0 },
	{ "changestatglobal", GameScript::ChangeStatGlobal, 0 },
	{ "changestoremarkup", GameScript::ChangeStoreMarkup, 0 }, //iwd2
	{ "changeteam", GameScript::SetTeam, 0 }, //pst
	{ "changetilestate", GameScript::ChangeTileState, 0 }, //bg2
	{ "chunkcreature", GameScript::ChunkCreature, 0 }, //should be more graphical
	{ "clearactions", GameScript::ClearActions, 0 },
	{ "clearallactions", GameScript::ClearAllActions, 0 },
	{ "clearpartyeffects", GameScript::ClearPartyEffects, 0 },
	{ "clearspriteeffects", GameScript::ClearSpriteEffects, 0 },
	{ "clicklbuttonobject", GameScript::ClickLButtonObject, AF_BLOCKING },
	{ "clicklbuttonpoint", GameScript::ClickLButtonPoint, AF_BLOCKING },
	{ "clickrbuttonobject", GameScript::ClickLButtonObject, AF_BLOCKING },
	{ "clickrbuttonpoint", GameScript::ClickLButtonPoint, AF_BLOCKING },
	{ "closedoor", GameScript::CloseDoor, 0 },
	{ "containerenable", GameScript::ContainerEnable, 0 },
	{ "continue", GameScript::Continue, AF_IMMEDIATE | AF_CONTINUE },
	{ "copygroundpilesto", GameScript::CopyGroundPilesTo, 0 },
	{ "createcreature", GameScript::CreateCreature, 0 }, //point is relative to Sender
	{ "createcreaturecopypoint", GameScript::CreateCreatureCopyPoint, 0 }, //point is relative to Sender
	{ "createcreaturedoor", GameScript::CreateCreatureDoor, 0 },
	{ "createcreatureatfeet", GameScript::CreateCreatureAtFeet, 0 },
	{ "createcreatureatlocation", GameScript::CreateCreatureAtLocation, 0 },
	{ "createcreatureimpassable", GameScript::CreateCreatureImpassable, 0 },
	{ "createcreatureimpassableallowoverlap", GameScript::CreateCreatureImpassableAllowOverlap, 0 },
	{ "createcreatureobject", GameScript::CreateCreatureObjectOffset, 0 }, //the same
	{ "createcreatureobjectcopy", GameScript::CreateCreatureObjectCopy, 0 },
	{ "createcreatureobjectcopyeffect", GameScript::CreateCreatureObjectCopy, 0 }, //the same
	{ "createcreatureobjectdoor", GameScript::CreateCreatureObjectDoor, 0 }, //same as createcreatureobject, but with dimension door animation
	{ "createcreatureobjectoffscreen", GameScript::CreateCreatureObjectOffScreen, 0 }, //same as createcreature object, but starts looking for a place far away from the player
	{ "createcreatureobjectoffset", GameScript::CreateCreatureObjectOffset, 0 }, //the same
	{ "createcreatureoffscreen", GameScript::CreateCreatureOffScreen, 0 },
	{ "createitem", GameScript::CreateItem, 0 },
	{ "createitemglobal", GameScript::CreateItemNumGlobal, 0 },
	{ "createitemnumglobal", GameScript::CreateItemNumGlobal, 0 },
	{ "createpartygold", GameScript::CreatePartyGold, 0 },
	{ "createvisualeffect", GameScript::CreateVisualEffect, 0 },
	{ "createvisualeffectobject", GameScript::CreateVisualEffectObject, 0 },
	{ "createvisualeffectobjectSticky", GameScript::CreateVisualEffectObjectSticky, 0 },
	{ "cutsceneid", GameScript::CutSceneID, 0 },
	{ "damage", GameScript::Damage, 0 },
	{ "daynight", GameScript::DayNight, 0 },
	{ "deactivate", GameScript::Deactivate, 0 },
	{ "deathmatchpositionarea", nullptr, 0 },
	{ "deathmatchpositionglobal", nullptr, 0 },
	{ "deathmatchpositionlocal", nullptr, 0 },
	{ "debug", GameScript::Debug, 0 },
	{ "debugoutput", GameScript::Debug, 0 },
	{ "deletejournalentry", GameScript::RemoveJournalEntry, 0 },
	{ "demoend", GameScript::DemoEnd, 0 }, //same for now
	{ "destroyalldestructableequipment", GameScript::DestroyAllDestructableEquipment, 0 },
	{ "destroyallequipment", GameScript::DestroyAllEquipment, 0 },
	{ "destroyallfragileequipment", GameScript::DestroyAllFragileEquipment, 0 },
	{ "destroygold", GameScript::DestroyGold, 0 },
	{ "destroygroundpiles", GameScript::DestroyGroundPiles, 0 },
	{ "destroyitem", GameScript::DestroyItem, AF_DLG_INSTANT }, //Cespenar won't work without this hack So, do we really need instant.ids?
	{ "destroypartygold", GameScript::DestroyPartyGold, 0 },
	{ "destroypartyitem", GameScript::DestroyPartyItem, 0 },
	{ "destroyself", GameScript::DestroySelf, 0 },
	{ "detectsecretdoor", GameScript::DetectSecretDoor, 0 },
	{ "dialog", GameScript::Dialogue, AF_BLOCKING | AF_DIALOG },
	{ "dialogforceinterrupt", GameScript::DialogueForceInterrupt, AF_BLOCKING | AF_DIALOG },
	{ "dialoginterrupt", GameScript::DialogueInterrupt, AF_DIALOG },
	{ "dialogue", GameScript::Dialogue, AF_BLOCKING | AF_DIALOG },
	{ "dialogueforceinterrupt", GameScript::DialogueForceInterrupt, AF_BLOCKING | AF_DIALOG },
	{ "dialogueinterrupt", GameScript::DialogueInterrupt, AF_DIALOG },
	{ "disablefogdither", GameScript::DisableFogDither, 0 },
	{ "disablespritedither", GameScript::DisableSpriteDither, 0 },
	{ "displaymessage", GameScript::DisplayMessage, 0 },
	{ "displaystring", GameScript::DisplayString, 0 },
	{ "displaystringhead", GameScript::DisplayStringHead, 0 },
	{ "displaystringheadnolog", GameScript::DisplayStringHeadNoLog, 0 },
	{ "displaystringheadowner", GameScript::DisplayStringHeadOwner, 0 },
	{ "displaystringheaddead", GameScript::DisplayStringHead, 0 }, //same?
	{ "displaystringnoname", GameScript::DisplayStringNoName, 0 },
	{ "displaystringnonamedlg", GameScript::DisplayStringNoName, 0 },
	{ "displaystringnonamehead", GameScript::DisplayStringNoNameHead, 0 },
	{ "displaystringpoint", GameScript::FloatMessageFixed, 0 }, // can customize color, maybe a different font, otherwise the same
	{ "displaystringpointlog", GameScript::FloatMessageFixed, 0 }, // same, but force also printing to message window
	{ "displaystringwait", GameScript::DisplayStringWait, AF_BLOCKING },
	{ "doubleclicklbuttonobject", GameScript::DoubleClickLButtonObject, AF_BLOCKING },
	{ "doubleclicklbuttonpoint", GameScript::DoubleClickLButtonPoint, AF_BLOCKING },
	{ "doubleclickrbuttonobject", GameScript::DoubleClickLButtonObject, AF_BLOCKING },
	{ "doubleclickrbuttonpoint", GameScript::DoubleClickLButtonPoint, AF_BLOCKING },
	{ "dropinventory", GameScript::DropInventory, 0 },
	{ "dropinventoryex", GameScript::DropInventoryEX, 0 },
	{ "dropinventoryexexclude", GameScript::DropInventoryEX, 0 }, //same
	{ "dropitem", GameScript::DropItem, AF_BLOCKING },
	{ "enablefogdither", GameScript::EnableFogDither, 0 },
	{ "enableportaltravel", GameScript::EnablePortalTravel, 0 },
	{ "enablespritedither", GameScript::EnableSpriteDither, 0 },
	{ "endcredits", GameScript::EndCredits, 0 }, //movie
	{ "endcutscenemode", GameScript::EndCutSceneMode, 0 },
	{ "endgame", GameScript::QuitGame, 0 }, //ending in iwd2
	{ "enemy", GameScript::Enemy, 0 },
	{ "equipitem", GameScript::EquipItem, 0 },
	{ "equipmostdamagingmelee", GameScript::EquipMostDamagingMelee, 0 },
	{ "equipranged", GameScript::EquipRanged, 0 },
	{ "equipweapon", GameScript::EquipWeapon, 0 },
	{ "erasejournalentry", GameScript::RemoveJournalEntry, 0 },
	{ "escapearea", GameScript::EscapeArea, AF_BLOCKING },
	{ "escapeareadestroy", GameScript::EscapeAreaDestroy, AF_BLOCKING },
	{ "escapeareamove", GameScript::EscapeArea, AF_BLOCKING },
	{ "escapeareanosee", GameScript::EscapeAreaNoSee, AF_BLOCKING },
	{ "escapeareaobject", GameScript::EscapeAreaObject, AF_BLOCKING },
	{ "escapeareaobjectnosee", GameScript::EscapeAreaObjectNoSee, AF_BLOCKING },
	{ "exitpocketplane", GameScript::ExitPocketPlane, 0 },
	{ "expansionendcredits", GameScript::ExpansionEndCredits, 0 }, //ends game too
	{ "explore", GameScript::Explore, 0 },
	{ "exploremapchunk", GameScript::ExploreMapChunk, 0 },
	{ "exportparty", GameScript::ExportParty, 0 },
	{ "face", GameScript::Face, AF_BLOCKING | AF_ALIVE },
	{ "faceobject", GameScript::FaceObject, AF_BLOCKING },
	{ "facesavedlocation", GameScript::FaceSavedLocation, AF_BLOCKING },
	{ "fadefromblack", GameScript::FadeFromColor, 0 }, //probably the same
	{ "fadefromcolor", GameScript::FadeFromColor, 0 },
	{ "fadetoandfromcolor", GameScript::FadeToAndFromColor, 0 },
	{ "fadetoblack", GameScript::FadeToColor, 0 }, //probably the same
	{ "fadetocolor", GameScript::FadeToColor, 0 },
	{ "fakeeffectexpirycheck", GameScript::FakeEffectExpiryCheck, 0 },
	{ "fillslot", GameScript::FillSlot, 0 },
	{ "finalsave", GameScript::SaveGame, 0 }, //synonym
	{ "findtraps", GameScript::FindTraps, 0 },
	{ "fixengineroom", GameScript::FixEngineRoom, 0 },
	{ "floatmessage", GameScript::DisplayStringHead, 0 },
	{ "floatmessagefixed", GameScript::FloatMessageFixed, 0 },
	{ "floatmessagefixedrnd", GameScript::FloatMessageFixedRnd, 0 },
	{ "floatmessagernd", GameScript::FloatMessageRnd, 0 },
	{ "floatrebus", GameScript::FloatRebus, 0 },
	{ "follow", GameScript::Follow, AF_ALIVE },
	{ "followcreature", GameScript::FollowCreature, AF_BLOCKING | AF_ALIVE }, //pst
	{ "followobjectformation", GameScript::FollowObjectFormation, AF_BLOCKING | AF_ALIVE },
	{ "forceaiscript", GameScript::ForceAIScript, 0 },
	{ "forceattack", GameScript::ForceAttack, 0 },
	{ "forcefacing", GameScript::ForceFacing, 0 },
	{ "forcehide", GameScript::ForceHide, 0 },
	{ "forceleavearealua", GameScript::ForceLeaveAreaLUA, 0 },
	{ "forcemarkedspell", GameScript::ForceMarkedSpell, 0 },
	{ "forcerandomencounter", GameScript::ForceRandomEncounter, 0 },
	{ "forcerandomencounterentry", GameScript::ForceRandomEncounter, 0 },
	{ "forcespell", GameScript::ForceSpell, AF_BLOCKING | AF_ALIVE | AF_IWD2_OVERRIDE },
	{ "forcespellres", GameScript::ForceSpell, AF_BLOCKING | AF_ALIVE | AF_IWD2_OVERRIDE },
	{ "forcespellresnofeedback", GameScript::ForceSpell, AF_BLOCKING | AF_ALIVE | AF_IWD2_OVERRIDE },
	{ "forcespellpoint", GameScript::ForceSpellPoint, AF_BLOCKING | AF_ALIVE | AF_IWD2_OVERRIDE },
	{ "forcespellpointres", GameScript::ForceSpellPoint, AF_BLOCKING | AF_ALIVE | AF_IWD2_OVERRIDE },
	{ "forcespellpointrange", GameScript::ForceSpellPointRange, AF_BLOCKING | AF_ALIVE },
	{ "forcespellpointrangeres", GameScript::ForceSpellPointRange, AF_BLOCKING | AF_ALIVE },
	{ "forcespellrange", GameScript::ForceSpellRange, AF_BLOCKING | AF_ALIVE },
	{ "forcespellrangeres", GameScript::ForceSpellRange, AF_BLOCKING | AF_ALIVE },
	{ "forceusecontainer", GameScript::ForceUseContainer, AF_BLOCKING },
	{ "formation", GameScript::Formation, AF_BLOCKING },
	{ "fullheal", GameScript::FullHeal, 0 },
	{ "fullhealex", GameScript::FullHeal, 0 }, //pst, not sure what's different
	{ "gameover", GameScript::QuitGame, 0 },
	{ "generatemodronmaze", GameScript::GenerateMaze, 0 },
	{ "generatepartymember", GameScript::GeneratePartyMember, 0 },
	{ "getitem", GameScript::GetItem, 0 },
	{ "getstat", GameScript::GetStat, 0 }, //gemrb specific
	{ "giveexperience", GameScript::AddXPObject, 0 },
	{ "givegoldforce", GameScript::CreatePartyGold, 0 }, //this is the same
	{ "giveitem", GameScript::GiveItem, 0 },
	{ "giveitemcreate", GameScript::CreateItem, 0 }, //actually this is a targeted createitem
	{ "giveobjectgoldglobal", GameScript::GiveObjectGoldGlobal, 0 },
	{ "giveorder", GameScript::GiveOrder, 0 },
	{ "givepartyallequipment", GameScript::GivePartyAllEquipment, 0 },
	{ "givepartygold", GameScript::GivePartyGold, 0 },
	{ "givepartygoldglobal", GameScript::GivePartyGoldGlobal, 0 }, //no mergestrings!
	{ "globaladdglobal", GameScript::GlobalAddGlobal, AF_MERGESTRINGS },
	{ "globalandglobal", GameScript::GlobalAndGlobal, AF_MERGESTRINGS },
	{ "globalband", GameScript::GlobalBAnd, AF_MERGESTRINGS },
	{ "globalbandglobal", GameScript::GlobalBAndGlobal, AF_MERGESTRINGS },
	{ "globalbitglobal", GameScript::GlobalBitGlobal, AF_MERGESTRINGS },
	{ "globalbor", GameScript::GlobalBOr, AF_MERGESTRINGS },
	{ "globalborglobal", GameScript::GlobalBOrGlobal, AF_MERGESTRINGS },
	{ "globalmax", GameScript::GlobalMax, AF_MERGESTRINGS },
	{ "globalmaxglobal", GameScript::GlobalMaxGlobal, AF_MERGESTRINGS },
	{ "globalmin", GameScript::GlobalMin, AF_MERGESTRINGS },
	{ "globalminglobal", GameScript::GlobalMinGlobal, AF_MERGESTRINGS },
	{ "globalorglobal", GameScript::GlobalOrGlobal, AF_MERGESTRINGS },
	{ "globalset", GameScript::SetGlobal, AF_MERGESTRINGS },
	{ "globalsetglobal", GameScript::GlobalSetGlobal, AF_MERGESTRINGS },
	{ "globalshl", GameScript::GlobalShL, AF_MERGESTRINGS },
	{ "globalshlglobal", GameScript::GlobalShLGlobal, AF_MERGESTRINGS },
	{ "globalshout", GameScript::GlobalShout, 0 },
	{ "globalshr", GameScript::GlobalShR, AF_MERGESTRINGS },
	{ "globalshrglobal", GameScript::GlobalShRGlobal, AF_MERGESTRINGS },
	{ "globalsubglobal", GameScript::GlobalSubGlobal, AF_MERGESTRINGS },
	{ "globalxor", GameScript::GlobalXor, AF_MERGESTRINGS },
	{ "globalxorglobal", GameScript::GlobalXorGlobal, AF_MERGESTRINGS },
	{ "gotostartscreen", GameScript::QuitGame, 0 }, //ending
	{ "groupattack", GameScript::GroupAttack, 0 },
	{ "help", GameScript::Help, 0 },
	{ "hide", GameScript::Hide, 0 },
	{ "hideareaonmap", GameScript::HideAreaOnMap, 0 },
	{ "hidecreature", GameScript::HideCreature, 0 },
	{ "hidegui", GameScript::HideGUI, 0 },
	{ "incinternal", GameScript::IncInternal, 0 }, //pst
	{ "incrementinternal", GameScript::IncInternal, 0 }, //iwd
	{ "incmoraleai", GameScript::IncMoraleAI, 0 },
	{ "incrementchapter", GameScript::IncrementChapter, 0 },
	{ "incrementextraproficiency", GameScript::IncrementExtraProficiency, 0 },
	{ "incrementglobal", GameScript::IncrementGlobal, AF_MERGESTRINGS },
	{ "incrementglobalonce", GameScript::IncrementGlobalOnce, AF_MERGESTRINGS },
	{ "incrementglobalonceex", GameScript::IncrementGlobalOnce, 0 },
	{ "incrementkillstat", GameScript::IncrementKillStat, 0 },
	{ "incrementproficiency", GameScript::IncrementProficiency, 0 },
	{ "interact", GameScript::Interact, 0 },
	{ "joinparty", GameScript::JoinParty, 0 }, //this action appears to be blocking in bg2
	{ "joinpartyoverride", GameScript::JoinParty, 0 },
	{ "journalentrydone", GameScript::SetQuestDone, 0 },
	{ "jumptoobject", GameScript::JumpToObject, 0 },
	{ "jumptopoint", GameScript::JumpToPoint, 0 },
	{ "jumptopointinstant", GameScript::JumpToPointInstant, 0 },
	{ "jumptosavedlocation", GameScript::JumpToSavedLocation, 0 },
	{ "kill", GameScript::Kill, 0 },
	{ "killfloatmessage", GameScript::KillFloatMessage, 0 },
	{ "layhands", GameScript::NoAction, 0 }, // broken in released engines; likely on purpose, since a spell is cleaner
	{ "leader", GameScript::Leader, AF_ALIVE },
	{ "leavearea", GameScript::LeaveAreaLUA, 0 }, //so far the same
	{ "leavearealua", GameScript::LeaveAreaLUA, 0 },
	{ "leavearealuaentry", GameScript::LeaveAreaLUAEntry, AF_BLOCKING },
	{ "leavearealuapanic", GameScript::LeaveAreaLUAPanic, 0 },
	{ "leavearealuapanicentry", GameScript::LeaveAreaLUAPanicEntry, 0 },
	{ "leaveparty", GameScript::LeaveParty, 0 },
	{ "lock", GameScript::Lock, 0 }, //key not checked at this time!
	{ "lockscroll", GameScript::LockScroll, 0 },
	{ "log", GameScript::Debug, 0 }, //the same until we know better
	{ "losegame", GameScript::QuitGame, 0 }, // tobex
	{ "makeglobal", GameScript::MakeGlobal, 0 },
	{ "makeglobaloverride", GameScript::MakeGlobalOverride, 0 },
	{ "makeunselectable", GameScript::MakeUnselectable, 0 },
	{ "markobject", GameScript::MarkObject, 0 },
	{ "markspellandobject", GameScript::MarkSpellAndObject, 0 },
	{ "moraledec", GameScript::MoraleDec, 0 },
	{ "moraleinc", GameScript::MoraleInc, 0 },
	{ "moraleset", GameScript::MoraleSet, 0 },
	{ "matchhp", GameScript::MatchHP, 0 },
	{ "movebetweenareas", GameScript::MoveBetweenAreas, 0 },
	{ "movebetweenareaseffect", GameScript::MoveBetweenAreas, 0 },
	{ "movecontainercontents", GameScript::MoveContainerContents, 0 },
	{ "movecursorpoint", GameScript::MoveCursorPoint, 0 }, //immediate move
	{ "moveglobal", GameScript::MoveGlobal, 0 },
	{ "moveglobalobject", GameScript::MoveGlobalObject, 0 },
	{ "moveglobalobjectoffscreen", GameScript::MoveGlobalObjectOffScreen, 0 },
	{ "moveglobalsto", GameScript::MoveGlobalsTo, 0 },
	{ "transferinventory", GameScript::MoveInventory, 0 },
	{ "movetocenterofscreen", GameScript::MoveToCenterOfScreen, AF_BLOCKING },
	{ "movetocampaign", GameScript::MoveToCampaign, AF_BLOCKING },
	{ "movetoexpansion", GameScript::MoveToExpansion, AF_BLOCKING },
	{ "movetoobject", GameScript::MoveToObject, AF_BLOCKING | AF_ALIVE },
	{ "movetoobjectfollow", GameScript::MoveToObjectFollow, AF_BLOCKING | AF_ALIVE },
	{ "movetoobjectnointerrupt", GameScript::MoveToObjectNoInterrupt, AF_BLOCKING | AF_ALIVE },
	{ "movetoobjectoffset", GameScript::MoveToObject, AF_BLOCKING | AF_ALIVE },
	{ "movetoobjectuntilsee", GameScript::MoveToObjectUntilSee, AF_BLOCKING | AF_ALIVE },
	{ "movetooffset", GameScript::MoveToOffset, AF_BLOCKING | AF_ALIVE },
	{ "movetopoint", GameScript::MoveToPoint, AF_BLOCKING | AF_ALIVE },
	{ "movetopointnointerrupt", GameScript::MoveToPointNoInterrupt, AF_BLOCKING | AF_ALIVE },
	{ "movetopointnorecticle", GameScript::MoveToPointNoRecticle, AF_BLOCKING | AF_ALIVE }, //the same until we know better
	{ "movetosavedlocation", GameScript::MoveToSavedLocation, AF_MERGESTRINGS | AF_BLOCKING },
	//take care of the typo in the original bg2 action.ids
	{ "movetosavedlocationn", GameScript::MoveToSavedLocation, AF_MERGESTRINGS | AF_BLOCKING },
	{ "moveviewobject", GameScript::MoveViewObject, 0 },
	{ "moveviewpoint", GameScript::MoveViewPoint, 0 },
	{ "moveviewpointuntildone", GameScript::MoveViewPointUntilDone, AF_BLOCKING },
	{ "multiplayersync", GameScript::MultiPlayerSync, 0 },
	{ "nidspecial1", GameScript::NIDSpecial1, AF_BLOCKING | AF_DIRECT | AF_ALIVE }, //we use this for dialogs, hack
	{ "nidspecial2", GameScript::NIDSpecial2, AF_BLOCKING }, //we use this for worldmap, another hack
	{ "nidspecial3", GameScript::Attack, AF_BLOCKING | AF_DIRECT | AF_ALIVE }, //this hack is for attacking preset target
	{ "nidspecial4", GameScript::ProtectObject, AF_BLOCKING | AF_DIRECT | AF_ALIVE },
	{ "nidspecial5", GameScript::UseItem, AF_BLOCKING | AF_DIRECT | AF_ALIVE },
	{ "nidspecial6", GameScript::Spell, AF_BLOCKING | AF_DIRECT | AF_ALIVE },
	{ "nidspecial7", GameScript::SpellNoDec, AF_BLOCKING | AF_DIRECT | AF_ALIVE },
	{ "nidspecial8", GameScript::SpellPoint, AF_BLOCKING | AF_ALIVE }, // not needed, but avoids warning
	{ "nidspecial9", GameScript::ToggleDoor, AF_BLOCKING }, //another internal hack, maybe we should use UseDoor instead
	{ "nidspecial10", GameScript::NoAction, 0 },
	{ "nidspecial11", GameScript::NoAction, 0 },
	{ "nidspecial12", GameScript::NoAction, 0 },
	{ "noaction", GameScript::NoAction, 0 },
	{ "opendoor", GameScript::OpenDoor, 0 },
	{ "overrideareadifficulty", GameScript::OverrideAreaDifficulty, 0 },
	{ "panic", GameScript::Panic, AF_ALIVE },
	{ "permanentstatchange", GameScript::PermanentStatChange, 0 }, //pst
	{ "pausegame", GameScript::PauseGame, AF_BLOCKING }, //this is almost surely blocking
	{ "picklock", GameScript::PickLock, AF_BLOCKING },
	{ "pickpockets", GameScript::PickPockets, AF_BLOCKING },
	{ "pickupitem", GameScript::PickUpItem, 0 },
	{ "playbardsong", GameScript::PlayBardSong, AF_ALIVE },
	{ "playdead", GameScript::PlayDead, AF_BLOCKING | AF_ALIVE },
	{ "playdeadinterruptable", GameScript::PlayDeadInterruptible, AF_BLOCKING | AF_ALIVE },
	{ "playerdialog", GameScript::PlayerDialogue, AF_BLOCKING | AF_DIALOG },
	{ "playerdialogue", GameScript::PlayerDialogue, AF_BLOCKING | AF_DIALOG },
	{ "playsequence", GameScript::PlaySequence, 0 },
	{ "playsequenceglobal", GameScript::PlaySequenceGlobal, 0 }, //pst
	{ "playsequencetimed", GameScript::PlaySequenceTimed, 0 }, //pst
	{ "playsong", GameScript::StartSong, 0 },
	{ "playsound", GameScript::PlaySound, 0 },
	{ "playsoundnotranged", GameScript::PlaySoundNotRanged, 0 },
	{ "playsoundpoint", GameScript::PlaySoundPoint, 0 },
	{ "plunder", GameScript::Plunder, AF_BLOCKING | AF_ALIVE },
	{ "polymorph", GameScript::Polymorph, 0 },
	{ "polymorphcopy", GameScript::PolymorphCopy, 0 },
	{ "polymorphcopybase", GameScript::PolymorphCopyBase, 0 },
	{ "polymorphex", GameScript::Polymorph, 0 }, //pst FIXME: has RangeModes int1 that we ignore
	{ "protectobject", GameScript::ProtectObject, 0 },
	{ "protectpoint", GameScript::ProtectPoint, AF_BLOCKING },
	{ "quitgame", GameScript::QuitGame, 0 },
	{ "randomfly", GameScript::RandomFly, AF_BLOCKING | AF_ALIVE },
	{ "randomrun", GameScript::RandomRun, AF_BLOCKING | AF_ALIVE },
	{ "randomturn", GameScript::RandomTurn, AF_BLOCKING },
	{ "randomwalk", GameScript::RandomWalk, AF_BLOCKING | AF_ALIVE },
	{ "randomwalktime", GameScript::RandomWalk, AF_BLOCKING | AF_ALIVE },
	{ "randomwalkcontinuous", GameScript::RandomWalkContinuous, AF_BLOCKING | AF_ALIVE },
	{ "randomwalkcontinuoustime", GameScript::RandomWalkContinuous, AF_BLOCKING | AF_ALIVE },
	{ "realsetglobaltimer", GameScript::RealSetGlobalTimer, AF_MERGESTRINGS },
	{ "reallyforcespell", GameScript::ReallyForceSpell, AF_BLOCKING | AF_ALIVE | AF_IWD2_OVERRIDE },
	{ "reallyforcespellres", GameScript::ReallyForceSpell, AF_BLOCKING | AF_ALIVE | AF_IWD2_OVERRIDE },
	{ "reallyforcespelldead", GameScript::ReallyForceSpellDead, AF_BLOCKING },
	{ "reallyforcespelldeadres", GameScript::ReallyForceSpellDead, AF_BLOCKING },
	{ "reallyforcespelllevel", GameScript::ReallyForceSpell, AF_BLOCKING | AF_ALIVE }, //this is the same action
	{ "reallyforcespellpoint", GameScript::ReallyForceSpellPoint, AF_BLOCKING | AF_ALIVE },
	{ "reallyforcespellpointres", GameScript::ReallyForceSpellPoint, AF_BLOCKING | AF_ALIVE },
	{ "recoil", GameScript::Recoil, AF_ALIVE },
	{ "regainpaladinhood", GameScript::RegainPaladinHood, 0 },
	{ "regainrangerhood", GameScript::RegainRangerHood, 0 },
	{ "removeareaflag", GameScript::RemoveAreaFlag, 0 },
	{ "removeareatype", GameScript::RemoveAreaType, 0 },
	{ "removefamiliar", GameScript::RemoveFamiliar, 0 },
	{ "removejournalentry", GameScript::RemoveJournalEntry, 0 },
	{ "removemapnote", GameScript::RemoveMapnote, 0 },
	{ "removepaladinhood", GameScript::RemovePaladinHood, 0 },
	{ "removerangerhood", GameScript::RemoveRangerHood, 0 },
	{ "removespell", GameScript::RemoveSpell, 0 },
	{ "removestoreitem", GameScript::RemoveStoreItem, 0 },
	{ "removetraps", GameScript::RemoveTraps, AF_BLOCKING },
	{ "removeworldmapareaflag", GameScript::RemoveWorldmapAreaFlag, 0 },
	{ "reputationinc", GameScript::ReputationInc, 0 },
	{ "reputationset", GameScript::ReputationSet, 0 },
	{ "reserved", nullptr, 0 },
	{ "resetfogofwar", GameScript::UndoExplore, 0 }, //pst
	{ "resetjoinrequests", nullptr, 0 },
	{ "resetmorale", GameScript::ResetMorale, 0 },
	{ "resetplayerai", GameScript::ResetPlayerAI, 0 },
	{ "rest", GameScript::Rest, AF_ALIVE },
	{ "restnospells", GameScript::RestNoSpells, 0 },
	{ "restorepartylocations", GameScript::RestorePartyLocation, 0 },
	{ "restparty", GameScript::RestParty, 0 },
	{ "restuntilhealed", GameScript::RestUntilHealed, 0 },
	//this is in iwd2, same as movetosavedlocation, but with stats
	{ "returntosavedlocation", GameScript::ReturnToSavedLocation, AF_BLOCKING | AF_ALIVE },
	{ "returntosavedlocationdelete", GameScript::ReturnToSavedLocationDelete, AF_BLOCKING | AF_ALIVE },
	{ "returntosavedplace", GameScript::ReturnToSavedLocation, AF_BLOCKING | AF_ALIVE },
	{ "returntostartloc", GameScript::ReturnToStartLocation, AF_BLOCKING | AF_ALIVE }, // iwd2
	{ "revealareaonmap", GameScript::RevealAreaOnMap, 0 },
	{ "runawayfrom", GameScript::RunAwayFrom, AF_BLOCKING | AF_ALIVE },
	{ "runawayfromex", GameScript::RunAwayFrom, AF_BLOCKING | AF_ALIVE },
	{ "runawayfromnointerrupt", GameScript::RunAwayFromNoInterrupt, AF_BLOCKING | AF_ALIVE },
	{ "runawayfromnointerruptnoleavearea", GameScript::RunAwayFromNoInterruptNoLeaveArea, AF_BLOCKING | AF_ALIVE },
	{ "runawayfromnoleavearea", GameScript::RunAwayFromNoLeaveArea, AF_BLOCKING | AF_ALIVE },
	{ "runawayfrompoint", GameScript::RunAwayFromPoint, AF_BLOCKING | AF_ALIVE },
	{ "runfollow", GameScript::RunAwayFrom, AF_BLOCKING | AF_ALIVE },
	{ "runningattack", GameScript::RunningAttack, AF_BLOCKING | AF_ALIVE },
	{ "runningattacknosound", GameScript::RunningAttackNoSound, AF_BLOCKING | AF_ALIVE },
	{ "runtoobject", GameScript::RunToObject, AF_BLOCKING | AF_ALIVE },
	{ "runtopoint", GameScript::RunToPoint, AF_BLOCKING },
	{ "runtopointnorecticle", GameScript::RunToPointNoRecticle, AF_BLOCKING | AF_ALIVE },
	{ "runtosavedlocation", GameScript::RunToSavedLocation, AF_BLOCKING | AF_ALIVE },
	{ "savegame", GameScript::SaveGame, 0 },
	{ "savelocation", GameScript::SaveLocation, 0 },
	{ "saveplace", GameScript::SaveLocation, 0 },
	{ "saveobjectlocation", GameScript::SaveObjectLocation, 0 },
	{ "screenshake", GameScript::ScreenShake, 0 },
	{ "selectweaponability", GameScript::SelectWeaponAbility, 0 },
	{ "sendtrigger", GameScript::SendTrigger, 0 },
	{ "setanimstate", GameScript::PlaySequence, AF_ALIVE }, //pst
	{ "setapparentnamestrref", GameScript::SetApparentName, 0 },
	{ "setareaflags", GameScript::SetAreaFlags, 0 },
	{ "setarearestflag", GameScript::SetAreaRestFlag, 0 },
	{ "setareascript", GameScript::SetAreaScript, 0 },
	{ "setbeeninpartyflags", GameScript::SetBeenInPartyFlags, 0 },
	{ "setbestweapon", GameScript::SetBestWeapon, 0 },
	{ "setcorpseenabled", GameScript::AmbientActivate, 0 }, //another weird name
	{ "setcutscenelite", GameScript::SetCursorState, 0 }, //same as next
	{ "setcursorstate", GameScript::SetCursorState, 0 },
	{ "setcreatureareaflag", GameScript::SetCreatureAreaFlag, 0 },
	{ "setcriticalpathobject", GameScript::SetCriticalPathObject, 0 },
	{ "setcutscenebreakable", GameScript::SetCutSceneBreakable, 0 },
	{ "setdialog", GameScript::SetDialogue, 0 },
	{ "setdialogrange", GameScript::SetDialogueRange, 0 },
	{ "setdialogue", GameScript::SetDialogue, 0 },
	{ "setdialoguerange", GameScript::SetDialogueRange, 0 },
	{ "setdoorflag", GameScript::SetDoorFlag, 0 },
	{ "setdoorlocked", GameScript::SetDoorLocked, 0 },
	{ "setencounterprobability", GameScript::SetEncounterProbability, 0 },
	{ "setextendednight", GameScript::SetExtendedNight, 0 },
	{ "setfaction", GameScript::SetFaction, 0 },
	{ "setfavouritestokens", GameScript::SetPCStatsTokens, 0 },
	{ "setgabber", GameScript::SetGabber, 0 },
	{ "setglobal", GameScript::SetGlobal, AF_MERGESTRINGS },
	{ "setglobalrandom", GameScript::SetGlobalRandom, AF_MERGESTRINGS },
	{ "setglobalrandomplus", GameScript::SetGlobalRandom, AF_MERGESTRINGS },
	{ "setglobaltimer", GameScript::SetGlobalTimer, AF_MERGESTRINGS },
	{ "setglobaltimeronce", GameScript::SetGlobalTimerOnce, AF_MERGESTRINGS },
	{ "setglobaltimerrandom", GameScript::SetGlobalTimerRandom, AF_MERGESTRINGS },
	{ "setglobaltint", GameScript::SetGlobalTint, 0 },
	{ "sethomelocation", GameScript::SetHomeLocation, 0 }, //bg2
	{ "sethp", GameScript::SetHP, 0 },
	{ "sethppercent", GameScript::SetHPPercent, 0 },
	{ "setinternal", GameScript::SetInternal, 0 },
	{ "setinterrupt", GameScript::SetInterrupt, AF_INSTANT },
	{ "setitemflags", GameScript::SetItemFlags, 0 },
	{ "setleavepartydialogfile", GameScript::SetLeavePartyDialogFile, 0 },
	{ "setleavepartydialoguefile", GameScript::SetLeavePartyDialogFile, 0 },
	{ "setmarkedspell", GameScript::SetMarkedSpell, 0 },
	{ "setmasterarea", GameScript::SetMasterArea, 0 },
	{ "setmazeeasier", GameScript::SetMazeEasier, 0 }, //pst specific crap
	{ "setmazeharder", GameScript::SetMazeHarder, 0 }, //pst specific crap
	{ "setmoraleai", GameScript::SetMoraleAI, 0 },
	{ "setmusic", GameScript::SetMusic, 0 },
	{ "setmytarget", GameScript::SetMyTarget, 0 },
	{ "setname", GameScript::SetApparentName, 0 },
	{ "setnamelessclass", GameScript::SetNamelessClass, 0 },
	{ "setnamelessdeath", GameScript::SetNamelessDeath, 0 },
	{ "setnamelessdeathparty", GameScript::SetNamelessDeathParty, 0 },
	{ "setnamelessdisguise", GameScript::SetNamelessDisguise, 0 },
	{ "setnooneontrigger", GameScript::SetNoOneOnTrigger, 0 },
	{ "setnumtimestalkedto", GameScript::SetNumTimesTalkedTo, 0 },
	{ "setoriginalclass", GameScript::SetOriginalClass, 0 },
	{ "setplayersound", GameScript::SetPlayerSound, 0 },
	{ "setquestdone", GameScript::SetQuestDone, 0 },
	{ "setregularnamestrref", GameScript::SetRegularName, 0 },
	{ "setrestencounterchance", GameScript::SetRestEncounterChance, 0 },
	{ "setrestencounterprobabilityday", GameScript::SetRestEncounterProbabilityDay, 0 },
	{ "setrestencounterprobabilitynight", GameScript::SetRestEncounterProbabilityNight, 0 },
	{ "setsavedlocation", GameScript::SetSavedLocation, 0 },
	{ "setsavedlocationpoint", GameScript::SetSavedLocationPoint, 0 },
	{ "setscriptname", GameScript::SetScriptName, 0 },
	{ "setselection", GameScript::SetSelection, 0 },
	{ "setsequence", GameScript::PlaySequence, 0 }, //bg2 (only own)
	{ "setstartpos", GameScript::SetStartPos, 0 },
	{ "setteam", GameScript::SetTeam, 0 },
	{ "setteambit", GameScript::SetTeamBit, 0 },
	{ "settextcolor", GameScript::SetTextColor, 0 },
	{ "settrackstring", GameScript::SetTrackString, 0 },
	{ "settoken", GameScript::SetToken, 0 },
	{ "settoken2da", GameScript::SetToken2DA, 0 }, //GemRB specific
	{ "settokenglobal", GameScript::SetTokenGlobal, AF_MERGESTRINGS },
	{ "settokenobject", GameScript::SetTokenObject, 0 },
	{ "setupwish", GameScript::SetupWish, 0 },
	{ "setupwishobject", GameScript::SetupWishObject, 0 },
	{ "setvisualrange", GameScript::SetVisualRange, 0 },
	{ "setworldmap", GameScript::SetWorldmap, 0 },
	{ "sg", GameScript::SG, 0 },
	{ "shout", GameScript::Shout, 0 },
	{ "sinisterpoof", GameScript::CreateVisualEffect, 0 },
	{ "smallwait", GameScript::SmallWait, AF_BLOCKING },
	{ "smallwaitrandom", GameScript::SmallWaitRandom, AF_BLOCKING | AF_ALIVE },
	{ "soundactivate", GameScript::SoundActivate, 0 },
	{ "spawnptactivate", GameScript::SpawnPtActivate, 0 },
	{ "spawnptdeactivate", GameScript::SpawnPtDeactivate, 0 },
	{ "spawnptspawn", GameScript::SpawnPtSpawn, 0 },
	{ "spell", GameScript::Spell, AF_BLOCKING | AF_ALIVE | AF_IWD2_OVERRIDE },
	{ "spellcasteffect", GameScript::SpellCastEffect, 0 },
	{ "spellhiteffectpoint", GameScript::SpellHitEffectPoint, 0 },
	{ "spellhiteffectsprite", GameScript::SpellHitEffectSprite, 0 },
	{ "spellnodec", GameScript::SpellNoDec, AF_BLOCKING | AF_ALIVE | AF_IWD2_OVERRIDE },
	{ "spellpoint", GameScript::SpellPoint, AF_BLOCKING | AF_ALIVE | AF_IWD2_OVERRIDE },
	{ "spellpointnodec", GameScript::SpellPointNoDec, AF_BLOCKING | AF_ALIVE | AF_IWD2_OVERRIDE },
	{ "spellwait", GameScript::Spell, AF_BLOCKING | AF_ALIVE | AF_IWD2_OVERRIDE },
	{ "startcombatcounter", GameScript::StartCombatCounter, 0 },
	{ "startcutscene", GameScript::StartCutScene, 0 },
	{ "startcutsceneex", GameScript::StartCutSceneEx, 0 }, //pst (unknown), EE perhaps as of PST:EE
	{ "startcutscenemode", GameScript::StartCutSceneMode, 0 },
	{ "startdialog", GameScript::StartDialogue, AF_BLOCKING | AF_DIALOG },
	{ "startdialoginterrupt", GameScript::StartDialogueInterrupt, AF_BLOCKING | AF_DIALOG },
	{ "startdialogue", GameScript::StartDialogue, AF_BLOCKING | AF_DIALOG },
	{ "startdialogueinterrupt", GameScript::StartDialogueInterrupt, AF_BLOCKING | AF_DIALOG },
	{ "startdialognoname", GameScript::StartDialogue, AF_BLOCKING | AF_DIALOG },
	{ "startdialognoset", GameScript::StartDialogueNoSet, AF_BLOCKING | AF_DIALOG },
	{ "startdialognosetinterrupt", GameScript::StartDialogueNoSetInterrupt, AF_BLOCKING | AF_DIALOG },
	{ "startdialogoverride", GameScript::StartDialogueOverride, AF_BLOCKING | AF_DIALOG },
	{ "startdialogoverrideinterrupt", GameScript::StartDialogueOverrideInterrupt, AF_BLOCKING | AF_DIALOG },
	{ "startdialoguenoname", GameScript::StartDialogue, AF_BLOCKING | AF_DIALOG },
	{ "startdialoguenoset", GameScript::StartDialogueNoSet, AF_BLOCKING | AF_DIALOG },
	{ "startdialoguenosetinterrupt", GameScript::StartDialogueNoSetInterrupt, AF_BLOCKING | AF_DIALOG },
	{ "startdialogueoverride", GameScript::StartDialogueOverride, AF_BLOCKING | AF_DIALOG },
	{ "startdialogueoverrideinterrupt", GameScript::StartDialogueOverrideInterrupt, AF_BLOCKING | AF_DIALOG },
	{ "startmovie", GameScript::StartMovie, AF_BLOCKING },
	{ "startmusic", GameScript::StartMusic, 0 },
	{ "startrainnow", GameScript::StartRainNow, 0 },
	{ "startrandomtimer", GameScript::StartRandomTimer, 0 },
	{ "startsong", GameScript::StartSong, 0 },
	{ "startstore", GameScript::StartStore, 0 },
	{ "starttimer", GameScript::StartTimer, 0 },
	{ "stateoverrideflag", GameScript::StateOverrideFlag, 0 },
	{ "stateoverridetime", GameScript::StateOverrideTime, 0 },
	{ "staticpalette", GameScript::StaticPalette, 0 },
	{ "staticsequence", GameScript::PlaySequence, 0 }, //bg2 animation sequence
	{ "staticstart", GameScript::StaticStart, 0 },
	{ "staticstop", GameScript::StaticStop, 0 },
	{ "stopjoinrequests", GameScript::NoAction, 0 },
	{ "stickysinisterpoof", GameScript::CreateVisualEffectObjectSticky, 0 },
	{ "stopmoving", GameScript::StopMoving, 0 },
	{ "storepartylocations", GameScript::StorePartyLocation, 0 },
	{ "swing", GameScript::Swing, AF_ALIVE },
	{ "swingonce", GameScript::SwingOnce, AF_ALIVE },
	{ "takecreatureitems", GameScript::TakeCreatureItems, 0 },
	{ "takeitemlist", GameScript::TakeItemList, 0 },
	{ "takeitemlistparty", GameScript::TakeItemListParty, 0 },
	{ "takeitemlistpartynum", GameScript::TakeItemListPartyNum, 0 },
	{ "takeitemreplace", GameScript::TakeItemReplace, 0 },
	{ "takeobjectgoldglobal", GameScript::TakeObjectGoldGlobal, 0 },
	{ "takepartygold", GameScript::TakePartyGold, 0 },
	{ "takepartyitem", GameScript::TakePartyItem, 0 },
	{ "takepartyitemall", GameScript::TakePartyItemAll, 0 },
	{ "takepartyitemnum", GameScript::TakePartyItemNum, 0 },
	{ "takepartyitemrange", GameScript::TakePartyItemRange, 0 },
	{ "teleportparty", GameScript::TeleportParty, 0 },
	{ "textscreen", GameScript::TextScreen, 0 },
	{ "timedmovetopoint", GameScript::TimedMoveToPoint, AF_BLOCKING | AF_ALIVE },
	{ "tomsstringdisplayer", GameScript::DisplayMessage, 0 },
	{ "transformitem", GameScript::TransformItem, 0 },
	{ "transformitemall", GameScript::TransformItemAll, 0 },
	{ "transformpartyitem", GameScript::TransformPartyItem, 0 },
	{ "transformpartyitemall", GameScript::TransformPartyItemAll, 0 },
	{ "triggeractivation", GameScript::TriggerActivation, 0 },
	{ "triggerwalkto", GameScript::TriggerWalkTo, AF_BLOCKING | AF_ALIVE }, //something like this
	{ "turn", GameScript::Turn, 0 },
	{ "turnamt", GameScript::TurnAMT, AF_BLOCKING }, //relative Face()
	{ "undoexplore", GameScript::UndoExplore, 0 },
	{ "unhide", GameScript::Unhide, 0 },
	{ "unhidegui", GameScript::UnhideGUI, 0 },
	{ "unloadarea", GameScript::UnloadArea, 0 },
	{ "unlock", GameScript::Unlock, 0 },
	{ "unlockscroll", GameScript::UnlockScroll, 0 },
	{ "unmakeglobal", GameScript::UnMakeGlobal, 0 }, //this is a GemRB extension
	{ "usecontainer", GameScript::UseContainer, AF_BLOCKING },
	{ "usedoor", GameScript::UseDoor, AF_BLOCKING },
	{ "useitem", GameScript::UseItem, AF_BLOCKING },
	{ "useitemability", GameScript::UseItem, AF_BLOCKING },
	{ "useitempoint", GameScript::UseItemPoint, AF_BLOCKING },
	{ "useitempointslot", GameScript::UseItemPoint, AF_BLOCKING },
	{ "useitemslot", GameScript::UseItem, AF_BLOCKING },
	{ "useitemslotability", GameScript::UseItem, AF_BLOCKING },
	{ "vequip", GameScript::SetArmourLevel, 0 },
	{ "verbalconstant", GameScript::VerbalConstant, 0 },
	{ "verbalconstanthead", GameScript::VerbalConstantHead, 0 },
	{ "wait", GameScript::Wait, AF_BLOCKING },
	{ "waitsync", GameScript::Wait, AF_BLOCKING },
	{ "waitanimation", GameScript::WaitAnimation, AF_BLOCKING }, //iwd2
	{ "waitrandom", GameScript::WaitRandom, AF_BLOCKING | AF_ALIVE },
	{ "weather", GameScript::Weather, 0 },
	{ "xequipitem", GameScript::XEquipItem, 0 },
	{ NULL, NULL, 0 }
};

//Make this an ordered list, so we could use bsearch!
static const ObjectLink objectnames[] = {
	{ "bestac", GameScript::BestAC },
	{ "eighthfarthestenemyof", GameScript::EighthFarthestEnemyOf },
	{ "eighthnearest", GameScript::EighthNearest },
	{ "eighthnearestallyof", GameScript::EighthNearestAllyOf },
	{ "eighthnearestdoor", GameScript::EighthNearestDoor },
	{ "eighthnearestenemyof", GameScript::EighthNearestEnemyOf },
	{ "eighthnearestenemyoftype", GameScript::EighthNearestEnemyOfType },
	{ "eighthnearestmygroupoftype", GameScript::EighthNearestEnemyOfType },
	{ "eigthnearestenemyof", GameScript::EighthNearestEnemyOf }, //typo in iwd
	{ "eigthnearestenemyoftype", GameScript::EighthNearestEnemyOfType }, //bg2
	{ "eigthnearestmygroupoftype", GameScript::EighthNearestEnemyOfType }, //bg2
	{ "familiar", GameScript::Familiar },
	{ "familiarsummoner", GameScript::FamiliarSummoner },
	{ "farthest", GameScript::Farthest },
	{ "farthestenemyof", GameScript::FarthestEnemyOf },
	{ "fifthfarthestenemyof", GameScript::FifthFarthestEnemyOf },
	{ "fifthnearest", GameScript::FifthNearest },
	{ "fifthnearestallyof", GameScript::FifthNearestAllyOf },
	{ "fifthnearestdoor", GameScript::FifthNearestDoor },
	{ "fifthnearestenemyof", GameScript::FifthNearestEnemyOf },
	{ "fifthnearestenemyoftype", GameScript::FifthNearestEnemyOfType },
	{ "fifthnearestmygroupoftype", GameScript::FifthNearestEnemyOfType },
	{ "fourthfarthestenemyof", GameScript::FourthFarthestEnemyOf },
	{ "fourthnearest", GameScript::FourthNearest },
	{ "fourthnearestallyof", GameScript::FourthNearestAllyOf },
	{ "fourthnearestdoor", GameScript::FourthNearestDoor },
	{ "fourthnearestenemyof", GameScript::FourthNearestEnemyOf },
	{ "fourthnearestenemyoftype", GameScript::FourthNearestEnemyOfType },
	{ "fourthnearestmygroupoftype", GameScript::FourthNearestEnemyOfType },
	{ "gabber", GameScript::Gabber },
	{ "groupof", GameScript::GroupOf },
	{ "lastattackerof", GameScript::LastAttackerOf },
	{ "lastcommandedby", GameScript::LastCommandedBy },
	{ "lastheardby", GameScript::LastHeardBy },
	{ "lasthelp", GameScript::LastHelp },
	{ "lasthitter", GameScript::LastHitter },
	{ "lastkilled", GameScript::LastKilled },
	{ "lastmarkedobject", GameScript::LastMarkedObject },
	{ "lastseenby", GameScript::LastSeenBy },
	{ "lastsummonerof", GameScript::LastSummonerOf },
	{ "lasttalkedtoby", GameScript::LastTalkedToBy },
	{ "lasttargetedby", GameScript::LastTargetedBy },
	{ "lasttrigger", GameScript::LastTrigger },
	{ "leaderof", GameScript::LeaderOf },
	{ "leastdamagedof", GameScript::LeastDamagedOf },
	{ "marked", GameScript::LastMarkedObject }, //pst
	{ "mostdamagedof", GameScript::MostDamagedOf },
	{ "myself", GameScript::Myself },
	{ "mytarget", GameScript::MyTarget }, //see lasttargetedby(myself)
	{ "nearest", GameScript::Nearest }, //actually this seems broken in IE and resolve as Myself
	{ "nearestallyof", GameScript::NearestAllyOf },
	{ "nearestdoor", GameScript::NearestDoor },
	{ "nearestenemyof", GameScript::NearestEnemyOf },
	{ "nearestenemyoftype", GameScript::NearestEnemyOfType },
	{ "nearestenemysummoned", GameScript::NearestEnemySummoned },
	{ "nearestmygroupoftype", GameScript::NearestMyGroupOfType },
	{ "nearestpc", GameScript::NearestPC },
	{ "ninthfarthestenemyof", GameScript::NinthFarthestEnemyOf },
	{ "ninthnearest", GameScript::NinthNearest },
	{ "ninthnearestallyof", GameScript::NinthNearestAllyOf },
	{ "ninthnearestdoor", GameScript::NinthNearestDoor },
	{ "ninthnearestenemyof", GameScript::NinthNearestEnemyOf },
	{ "ninthnearestenemyoftype", GameScript::NinthNearestEnemyOfType },
	{ "ninthnearestmygroupoftype", GameScript::NinthNearestMyGroupOfType },
	{ "nothing", GameScript::Nothing },
	{ "partyslot1", GameScript::PartySlot1 },
	{ "partyslot2", GameScript::PartySlot2 },
	{ "partyslot3", GameScript::PartySlot3 },
	{ "partyslot4", GameScript::PartySlot4 },
	{ "partyslot5", GameScript::PartySlot5 },
	{ "partyslot6", GameScript::PartySlot6 },
	{ "partyslot7", GameScript::PartySlot7 },
	{ "partyslot8", GameScript::PartySlot8 },
	{ "partyslot9", GameScript::PartySlot9 },
	{ "partyslot10", GameScript::PartySlot10 },
	{ "player1", GameScript::Player1 },
	{ "player1fill", GameScript::Player1Fill },
	{ "player2", GameScript::Player2 },
	{ "player2fill", GameScript::Player2Fill },
	{ "player3", GameScript::Player3 },
	{ "player3fill", GameScript::Player3Fill },
	{ "player4", GameScript::Player4 },
	{ "player4fill", GameScript::Player4Fill },
	{ "player5", GameScript::Player5 },
	{ "player5fill", GameScript::Player5Fill },
	{ "player6", GameScript::Player6 },
	{ "player6fill", GameScript::Player6Fill },
	{ "player7", GameScript::Player7 },
	{ "player7fill", GameScript::Player7Fill },
	{ "player8", GameScript::Player8 },
	{ "player8fill", GameScript::Player8Fill },
	{ "player9", GameScript::Player9 },
	{ "player9fill", GameScript::Player9Fill },
	{ "player10", GameScript::Player10 },
	{ "player10fill", GameScript::Player10Fill },
	{ "protectedby", GameScript::ProtectedBy },
	{ "protectorof", GameScript::ProtectorOf },
	{ "protagonist", GameScript::Protagonist },
	{ "secondfarthestenemyof", GameScript::SecondFarthestEnemyOf },
	{ "secondnearest", GameScript::SecondNearest },
	{ "secondnearestallyof", GameScript::SecondNearestAllyOf },
	{ "secondnearestdoor", GameScript::SecondNearestDoor },
	{ "secondnearestenemyof", GameScript::SecondNearestEnemyOf },
	{ "secondnearestenemyoftype", GameScript::SecondNearestEnemyOfType },
	{ "secondnearestmygroupoftype", GameScript::SecondNearestMyGroupOfType },
	{ "selectedcharacter", GameScript::SelectedCharacter },
	{ "seventhfarthestenemyof", GameScript::SeventhFarthestEnemyOf },
	{ "seventhnearest", GameScript::SeventhNearest },
	{ "seventhnearestallyof", GameScript::SeventhNearestAllyOf },
	{ "seventhnearestdoor", GameScript::SeventhNearestDoor },
	{ "seventhnearestenemyof", GameScript::SeventhNearestEnemyOf },
	{ "seventhnearestenemyoftype", GameScript::SeventhNearestEnemyOfType },
	{ "seventhnearestmygroupoftype", GameScript::SeventhNearestMyGroupOfType },
	{ "sixthfarthestenemyof", GameScript::SixthFarthestEnemyOf },
	{ "sixthnearest", GameScript::SixthNearest },
	{ "sixthnearestallyof", GameScript::SixthNearestAllyOf },
	{ "sixthnearestdoor", GameScript::SixthNearestDoor },
	{ "sixthnearestenemyof", GameScript::SixthNearestEnemyOf },
	{ "sixthnearestenemyoftype", GameScript::SixthNearestEnemyOfType },
	{ "sixthnearestmygroupoftype", GameScript::SixthNearestMyGroupOfType },
	{ "spelltarget", GameScript::SpellTarget },
	{ "strongestof", GameScript::StrongestOf },
	{ "strongestofmale", GameScript::StrongestOfMale },
	{ "tenthfarthestenemyof", GameScript::TenthFarthestEnemyOf },
	{ "tenthnearest", GameScript::TenthNearest },
	{ "tenthnearestallyof", GameScript::TenthNearestAllyOf },
	{ "tenthnearestdoor", GameScript::TenthNearestDoor },
	{ "tenthnearestenemyof", GameScript::TenthNearestEnemyOf },
	{ "tenthnearestenemyoftype", GameScript::TenthNearestEnemyOfType },
	{ "tenthnearestmygroupoftype", GameScript::TenthNearestMyGroupOfType },
	{ "thirdfarthestenemyof", GameScript::ThirdFarthestEnemyOf },
	{ "thirdnearest", GameScript::ThirdNearest },
	{ "thirdnearestallyof", GameScript::ThirdNearestAllyOf },
	{ "thirdnearestdoor", GameScript::ThirdNearestDoor },
	{ "thirdnearestenemyof", GameScript::ThirdNearestEnemyOf },
	{ "thirdnearestenemyoftype", GameScript::ThirdNearestEnemyOfType },
	{ "thirdnearestmygroupoftype", GameScript::ThirdNearestMyGroupOfType },
	{ "weakestof", GameScript::WeakestOf },
	{ "worstac", GameScript::WorstAC },
	{ NULL, NULL }
};

static const IDSLink idsnames[] = {
	{ "align", GameScript::ID_Alignment },
	{ "alignmen", GameScript::ID_Alignment },
	{ "alignmnt", GameScript::ID_Alignment },
	{ "avclass", GameScript::ID_AVClass },
	{ "class", GameScript::ID_Class },
	{ "classmsk", GameScript::ID_ClassMask },
	{ "ea", GameScript::ID_Allegiance },
	{ "faction", GameScript::ID_Faction },
	{ "gender", GameScript::ID_Gender },
	{ "general", GameScript::ID_General },
	{ "race", GameScript::ID_Race },
	{ "specific", GameScript::ID_Specific },
	{ "subrace", GameScript::ID_Subrace },
	{ "team", GameScript::ID_Team },
	{ NULL, NULL }
};

template<typename L, typename L2>
static const L* FindLink(StringView name, const L2& names)
{
	if (name.empty()) {
		return nullptr;
	}

	auto len = std::min(FindFirstOf(name, "("), name.length());
	for (int i = 0; names[i].Name; i++) {
		if (!strnicmp(names[i].Name, name.c_str(), len)) {
			return &names[i];
		}
	}
	return nullptr;
}

static const IDSLink* FindIdentifier(const std::string& idsname)
{
	int len = static_cast<int>(idsname.size() + 1);
	for (int i = 0; idsnames[i].Name; i++) {
		if (!strnicmp(idsnames[i].Name, idsname.c_str(), len)) {
			return idsnames + i;
		}
	}

	Log(WARNING, "GameScript", "Couldn't assign ids target: {}", idsname);
	return nullptr;
}

/** releasing global memory */
static void CleanupIEScript()
{
	triggersTable.reset();
	actionsTable.reset();
	objectsTable.reset();
	overrideActionsTable.reset();
	overrideTriggersTable.reset();
}

static void printFunction(std::string& buffer, const std::string& str, int value)
{
	size_t len = str.find_first_of('(');
	if (len == std::string::npos) {
		AppendFormat(buffer, "{} {}", value, str);
	} else {
		AppendFormat(buffer, "{} {:*^{}}", value, str, len);
	}
}

static void printFunction(std::string& buffer, const PluginHolder<SymbolMgr>& table, int index)
{
	const auto& str = table->GetStringIndex(index);
	int value = table->GetValueIndex(index);

	return printFunction(buffer, str, value);
}

static void LoadActionFlags(const ResRef& tableName, int flag, bool critical)
{
	int tableIndex = core->LoadSymbol(tableName);
	if (tableIndex < 0) {
		if (critical) {
			error("GameScript", "Couldn't find {} symbols!", tableName);
		} else {
			return;
		}
	}
	auto table = core->GetSymbol(tableIndex);
	if (!table) {
		error("GameScript", "Couldn't load {} symbols!", tableName);
	}
	size_t j = table->GetSize();
	while (j--) {
		int i = table->GetValueIndex(j);
		if (i >= MAX_ACTIONS) {
			Log(ERROR, "GameScript", "{} action {} ({}) is too high, ignoring",
			    tableName, i, table->GetStringIndex(j));
			continue;
		}
		if (!actions[i]) {
			Log(WARNING, "GameScript", "{} action {} ({}) doesn't exist, ignoring",
			    tableName, i, table->GetStringIndex(j));
			continue;
		}
		actionflags[i] |= flag;
	}
}

template<typename L, typename F>
static void CheckSynonym(const L& names, const F& func, const std::string& idx)
{
	for (int i = 0; names[i].Name; i++) {
		if (func == names[i].Function) {
			Log(MESSAGE, "GameScript", "{} is a synonym of {}", idx, names[i].Name);
			break;
		}
	}
}

static void SetupTriggers()
{
	std::list<int> missingTriggers;
	size_t max = triggersTable->GetSize();
	for (size_t j = 0; j < max; j++) {
		int i = triggersTable->GetValueIndex(j);
		const std::string& name = triggersTable->GetStringIndex(j);
		const TriggerLink* poi = FindLink<TriggerLink>(name, triggernames);

		bool wasCondition = (i & 0x4000);
		i &= 0x3fff;
		if (i >= MAX_TRIGGERS) {
			Log(ERROR, "GameScript", "Trigger {} ({}) is too high, ignoring", i, name);
			continue;
		}

		if (triggers[i]) {
			if (poi && triggers[i] != poi->Function) {
				std::string buffer = fmt::format("{} is in collision with ", name);
				printFunction(buffer, poi->Name, i);
				Log(WARNING, "GameScript", "{}", buffer);
			} else if (InDebugMode(DebugMode::TRIGGERS)) {
				std::string buffer = fmt::format("{} is a synonym of ", name);
				printFunction(buffer, name, i);
				Log(DEBUG, "GameScript", "{}", buffer);
			}
			continue; // we already found an alternative
		}

		if (poi == nullptr) {
			// missing trigger which might be resolved later
			triggers[i] = nullptr;
			triggerflags[i] = 0;
			missingTriggers.push_back(static_cast<int>(j));
			continue;
		}
		triggers[i] = poi->Function;
		triggerflags[i] = poi->Flags;
		if (wasCondition) {
			triggerflags[i] |= TF_CONDITION;
		}
		if (name.find("o:") != std::string::npos) triggerflags[i] |= TF_HAS_OBJECT;
	}

	// retry resolving previously missing triggers
	for (auto l = missingTriggers.begin(); l != missingTriggers.end(); ++l) {
		int j = *l;
		// found later as a different name
		int ii = triggersTable->GetValueIndex(j) & 0x3fff;
		if (ii >= MAX_TRIGGERS) {
			continue;
		}

		TriggerFunction f = triggers[ii];
		if (f) {
			CheckSynonym(triggernames, f, triggersTable->GetStringIndex(j));
			continue;
		}

		std::string buffer("Couldn't assign function to trigger: ");
		printFunction(buffer, triggersTable, j);
		Log(WARNING, "GameScript", "{}", buffer);
	}
}

static void SetupActions()
{
	std::list<int> missingActions;
	size_t max = actionsTable->GetSize();
	for (size_t j = 0; j < max; j++) {
		int i = actionsTable->GetValueIndex(j);
		const std::string& name = actionsTable->GetStringIndex(j);
		if (i >= MAX_ACTIONS) {
			Log(ERROR, "GameScript", "action {} ({}) is too high, ignoring", i, name);
			continue;
		}
		const ActionLink* poi = FindLink<ActionLink>(name, actionnames);
		if (actions[i]) {
			if (poi && actions[i] != poi->Function) {
				std::string buffer = fmt::format("{} is in collision with ", name);
				printFunction(buffer, name, i);
				Log(WARNING, "GameScript", "{}", buffer);
			} else if (InDebugMode(DebugMode::ACTIONS)) {
				std::string buffer = fmt::format("{} is a synonym of ", name);
				printFunction(buffer, name, i);
				Log(DEBUG, "GameScript", "{}", buffer);
			}
			continue; // we already found an alternative
		}
		if (poi == nullptr) {
			actions[i] = nullptr;
			actionflags[i] = 0;
			missingActions.push_back(static_cast<int>(j));
			continue;
		}
		actions[i] = poi->Function;
		actionflags[i] = poi->Flags;
		if (name.find("o:") != std::string::npos) actionflags[i] |= AF_HAS_OBJECT;
	}

	// retry resolving previously missing actions
	for (auto l = missingActions.begin(); l != missingActions.end(); ++l) {
		int j = *l;
		// found later as a different name
		int ii = actionsTable->GetValueIndex(j);
		if (ii >= MAX_ACTIONS) {
			continue;
		}

		ActionFunction f = actions[ii];
		if (f) {
			CheckSynonym(actionnames, f, actionsTable->GetStringIndex(j));
			continue;
		}
		std::string buffer("Couldn't assign function to action: ");
		printFunction(buffer, actionsTable, j);
		Log(WARNING, "GameScript", "{}", buffer);
	}
}

static void SetupObjects()
{
	std::list<int> missingObjects;
	size_t j = objectsTable->GetSize();
	while (j--) {
		int i = objectsTable->GetValueIndex(j);
		const std::string& name = objectsTable->GetStringIndex(j);
		if (i >= MAX_OBJECTS) {
			Log(ERROR, "GameScript", "object {} ({}) is too high, ignoring", i, name);
			continue;
		}
		const ObjectLink* poi = FindLink<ObjectLink>(name, objectnames);
		if (objects[i]) {
			if (poi && objects[i] != poi->Function) {
				std::string buffer = fmt::format("{} is in collision with ", name);
				printFunction(buffer, name, i);
				Log(WARNING, "GameScript", "{}", buffer);
			} else {
				std::string buffer = fmt::format("{} is a synonym of ", name);
				printFunction(buffer, name, i);
				Log(DEBUG, "GameScript", "{}", buffer);
			}
			continue;
		}
		if (poi == nullptr) {
			objects[i] = nullptr;
			missingObjects.push_back(static_cast<int>(j));
		} else {
			objects[i] = poi->Function;
		}
	}

	// retry resolving previously missing objects
	for (auto l = missingObjects.begin(); l != missingObjects.end(); ++l) {
		j = *l;
		// found later as a different name
		int ii = objectsTable->GetValueIndex(j);
		if (ii >= MAX_OBJECTS) {
			continue;
		}

		ObjectFunction f = objects[ii];
		if (f) {
			CheckSynonym(objectnames, f, objectsTable->GetStringIndex(j));
			continue;
		}
		std::string buffer("Couldn't assign function to object: ");
		printFunction(buffer, objectsTable, static_cast<int>(j));
		Log(WARNING, "GameScript", "{}", buffer);
	}
}

static void SetupOverrideActions()
{
	size_t max = overrideActionsTable->GetSize();
	for (size_t j = 0; j < max; j++) {
		int i = overrideActionsTable->GetValueIndex(j);
		const std::string& name = overrideActionsTable->GetStringIndex(j);
		if (i >= MAX_ACTIONS) {
			Log(ERROR, "GameScript", "action {} ({}) is too high, ignoring", i, name);
			continue;
		}
		const ActionLink* poi = FindLink<ActionLink>(name, actionnames);
		if (!poi) {
			std::string buffer("Couldn't assign function to override action: ");
			printFunction(buffer, name, i);
			continue;
		}
		if (actions[i] && (actions[i] != poi->Function || (actionflags[i] & AF_FILE_MASK) != (poi->Flags & AF_FILE_MASK))) {
			std::string buffer = fmt::format("{} overrides existing action ", name);
			int x = actionsTable->FindValue(i);
			if (x >= 0) {
				printFunction(buffer, actionsTable, actionsTable->FindValue(i));
			} else {
				printFunction(buffer, name, i);
			}
			Log(MESSAGE, "GameScript", "{}", buffer);
		}
		actions[i] = poi->Function;
		actionflags[i] = poi->Flags;
		if (name.find("o:") != std::string::npos) actionflags[i] |= AF_HAS_OBJECT;
	}
}

static void SetupOverrideTriggers()
{
	NextTriggerObjectID = 0;
	size_t max = overrideTriggersTable->GetSize();
	for (size_t j = 0; j < max; j++) {
		int i = overrideTriggersTable->GetValueIndex(j);
		bool wasCondition = (i & 0x4000);
		i &= 0x3fff;
		const auto& trName = overrideTriggersTable->GetStringIndex(j);
		if (i >= MAX_TRIGGERS) {
			Log(ERROR, "GameScript", "Trigger {} ({}) is too high, ignoring", i, trName);
			continue;
		}
		if (!NextTriggerObjectID && !stricmp(trName.c_str(), "NextTriggerObject(O:Object*)")) {
			NextTriggerObjectID = i;
		}
		const TriggerLink* poi = FindLink<TriggerLink>(trName, triggernames);
		if (!poi) {
			std::string buffer("Couldn't assign function to override trigger: ");
			printFunction(buffer, trName, i);
			continue;
		}

		short tf = poi->Flags | (wasCondition ? TF_CONDITION : 0);
		if (triggers[i] && (triggers[i] != poi->Function || triggerflags[i] != tf)) {
			std::string buffer = fmt::format("{} overrides existing trigger ", trName);
			int x = triggersTable->FindValue(i);
			if (x < 0) x = triggersTable->FindValue(i | 0x4000);
			if (x >= 0) {
				printFunction(buffer, triggersTable, x);
			} else {
				x = overrideTriggersTable->FindValue(i);
				if (x < 0 || size_t(x) >= j) x = overrideTriggersTable->FindValue(i | 0x4000);
				printFunction(buffer, overrideTriggersTable, x);
			}
			Log(MESSAGE, "GameScript", "{}", buffer);
		}
		triggers[i] = poi->Function;
		triggerflags[i] = tf;
		if (trName.find("o:") != std::string::npos) triggerflags[i] |= TF_HAS_OBJECT;
	}
}

static void SetupSavedTriggers()
{
	int savedTriggersIndex = core->LoadSymbol("svtriobj");
	if (savedTriggersIndex < 0) {
		// leaving this as not strictly necessary, for now
		Log(WARNING, "GameScript", "Couldn't find saved trigger symbols!");
	} else {
		auto savedTriggersTable = core->GetSymbol(savedTriggersIndex);
		if (!savedTriggersTable) {
			error("GameScript", "Couldn't load saved trigger symbols!");
		}
		size_t j = savedTriggersTable->GetSize();
		while (j--) {
			int i = savedTriggersTable->GetValueIndex(j);
			const std::string& trName = savedTriggersTable->GetStringIndex(j);
			i &= 0x3fff;
			if (i >= MAX_TRIGGERS) {
				Log(ERROR, "GameScript", "saved trigger {} ({}) is too high, ignoring", i, trName);
				continue;
			}
			if (!triggers[i]) {
				Log(WARNING, "GameScript", "saved trigger {} ({}) doesn't exist, ignoring", i, trName);
				continue;
			}
			triggerflags[i] |= TF_SAVED;
		}
	}
}

static void InitializeObjectIDS(const AutoTable& objNameTable)
{
	TableMgr::index_t idsRow = objNameTable->GetRowIndex("OBJECT_IDS_COUNT");

	ObjectIDSCount = objNameTable->QueryFieldSigned<int>(idsRow, 0);
	if (ObjectIDSCount < 0 || ObjectIDSCount > MAX_OBJECT_FIELDS) {
		error("GameScript", "The IDS Count shouldn't be more than 10!");
	}

	ObjectIDSTableNames.resize(ObjectIDSCount);
	for (int i = 0; i < ObjectIDSCount; i++) {
		const std::string& idsName = objNameTable->QueryField(idsRow, i + 1);
		const IDSLink* poi = FindIdentifier(idsName.c_str());
		if (poi == nullptr) {
			idtargets[i] = nullptr;
		} else {
			idtargets[i] = poi->Function;
		}
		ObjectIDSTableNames[i] = ResRef(idsName);
	}

	MaxObjectNesting = objNameTable->QueryFieldSigned<int>("MAX_OBJECT_NESTING", "DATA");
	if (MaxObjectNesting < 0 || MaxObjectNesting > MAX_NESTING) {
		error("GameScript", "The Object Nesting Count shouldn't be more than 5!");
	}

	HasAdditionalRect = objNameTable->QueryFieldSigned<int>("ADDITIONAL_RECT", "DATA") != 0;
	ExtraParametersCount = objNameTable->QueryFieldSigned<int>("EXTRA_PARAMETERS_COUNT", "DATA");
	HasTriggerPoint = objNameTable->QueryFieldSigned<int>("TRIGGER_POINT", "DATA") != 0;
	ObjectFieldsCount = ObjectIDSCount - ExtraParametersCount;
}

// IWD2 parses object selectors in differing orders depending on whether it's working with .DLG resources (text) or .BCS resources (compiled)!
//   Dialog: [EA.GENERAL.RACE.SUBRACE.CLASS.SPECIFIC.GENDER.ALIGN.AVCLASS.CLASSMSK]
//   BCS: EA GENERAL RACE CLASS SPECIFIC GENDER ALIGNMNT SUBRACE filter1 filter2 filter3 filter4 filter5 [rect1.rect2.rect3.rect4] "scriptname" AVCLASS CLASSMSK
static void InitializeDialogObjectIDS(AutoTable& objNameTable)
{
	TableMgr::index_t idsRow = objNameTable->GetRowIndex("DIALOG_IDS_COUNT");

	if (idsRow != TableMgr::npos) {
		DialogObjectIDSCount = objNameTable->QueryFieldSigned<int>(idsRow, 0);
		DialogObjectIDSTableNames.resize(DialogObjectIDSCount);

		for (int i = 0; i < DialogObjectIDSCount; i++) {
			const std::string& idsName = objNameTable->QueryField(idsRow, i + 1);
			DialogObjectIDSTableNames[i] = ResRef(idsName);
		}
	} else {
		DialogObjectIDSCount = ObjectIDSCount;
		DialogObjectIDSTableNames.resize(DialogObjectIDSCount);

		for (int i = 0; i < DialogObjectIDSCount; i++) {
			DialogObjectIDSTableNames[i] = ObjectIDSTableNames[i];
		}
	}

	TableMgr::index_t orderRow = objNameTable->GetRowIndex("DIALOG_IDS_ORDER");
	DialogObjectIDSOrder.resize(DialogObjectIDSCount);

	if (orderRow != TableMgr::npos) {
		for (int i = 0; i < DialogObjectIDSCount; i++) {
			DialogObjectIDSOrder[i] = objNameTable->QueryFieldSigned<int>(orderRow, i + 1) - 1;
		}
	} else {
		for (int i = 0; i < DialogObjectIDSCount; i++) {
			DialogObjectIDSOrder[i] = i;
		}
	}
}

void InitializeIEScript()
{
	PluginMgr::Get()->RegisterCleanup(CleanupIEScript);

	NoCreate = core->HasFeature(GFFlags::NO_NEW_VARIABLES);
	HasKaputz = core->HasFeature(GFFlags::HAS_KAPUTZ);

	if (core->HasFeature(GFFlags::AREA_OVERRIDE)) {
		MAX_OPERATING_DISTANCE = 40 * 3;
	}

	int tT = core->LoadSymbol("trigger");
	int aT = core->LoadSymbol("action");
	int oT = core->LoadSymbol("object");
	int gaT = core->LoadSymbol("gemact");
	int gtT = core->LoadSymbol("gemtrig");
	AutoTable objNameTable = gamedata->LoadTable("script");
	if (tT < 0 || aT < 0 || oT < 0 || !objNameTable) {
		error("GameScript", "A critical scripting file is missing!");
	}
	triggersTable = core->GetSymbol(tT);
	actionsTable = core->GetSymbol(aT);
	objectsTable = core->GetSymbol(oT);
	overrideActionsTable = core->GetSymbol(gaT);
	overrideTriggersTable = core->GetSymbol(gtT);
	if (!triggersTable || !actionsTable || !objectsTable || !objNameTable) {
		error("GameScript", "A critical scripting file is damaged!");
	}

	/* Loading Script Configuration Parameters */
	InitializeObjectIDS(objNameTable);
	InitializeDialogObjectIDS(objNameTable);

	/* Initializing the Script Engine */
	SetupTriggers();
	SetupActions();

	if (overrideActionsTable) {
		/*
		 * we add/replace some actions from gemact.ids
		 * right now you can't print or generate these actions!
		 */
		SetupOverrideActions();
	}

	if (overrideTriggersTable) {
		/*
		 * we add/replace some triggers from gemtrig.ids
		 * right now you can't print or generate these actions!
		 */
		SetupOverrideTriggers();
	}

	SetupObjects();

	if (gamedata->Exists("dlginst", IE_IDS_CLASS_ID, true)) {
		LoadActionFlags("dlginst", AF_DLG_INSTANT, true);
		LoadActionFlags("scrinst", AF_SCR_INSTANT, true);
	} else {
		LoadActionFlags("instant", AF_INSTANT, true);
	}
	LoadActionFlags("actsleep", AF_SLEEP, false);
	LoadActionFlags("chase", AF_CHASE, false);

	SetupSavedTriggers();
}

/********************** GameScript *******************************/
GameScript::GameScript(const ResRef& resref, Scriptable* MySelf,
		       int ScriptLevel, bool AIScript)
	: MySelf(MySelf), Name(resref), scriptlevel(ScriptLevel)
{
	script = CacheScript(Name, AIScript);
}

GameScript::~GameScript(void)
{
	BcsCache.DecRef(Name, true);
}

Script* GameScript::CacheScript(const ResRef& resRef, bool AIScript)
{
	SClass_ID type = AIScript ? IE_BS_CLASS_ID : IE_BCS_CLASS_ID;

	Script* cachedScript = BcsCache.GetResource(resRef);
	if (cachedScript) {
		return cachedScript;
	}

	DataStream* stream = gamedata->GetResourceStream(resRef, type);
	if (!stream) {
		return NULL;
	}

	std::string line;
	stream->ReadLine(line, 10);
	if (line.compare(0, 2, "SC") != 0) {
		Log(WARNING, "GameScript", "Not a Compiled Script file");
		delete stream;
		return nullptr;
	}

	auto newScript = BcsCache.SetAt(resRef).first;

	while (true) {
		ResponseBlock* rB = ReadResponseBlock(stream);
		if (!rB)
			break;
		newScript->responseBlocks.push_back(rB);
		stream->ReadLine(line, 10);
	}
	delete stream;
	return newScript;
}

/*
 * if you pass non-NULL parameters, continuing is set to whether we Continue()ed
 * (should start false and be passed to next script's Update),
 * and done is set to whether we processed a block without Continue()
 *
 * NOTE: After calling, callers should deallocate this object if `dead==true`.
 *  Scripts can replace themselves while running but it's up to the caller to clean up.
 */
bool GameScript::Update(bool* continuing, bool* done)
{
	if (!MySelf)
		return false;

	if (!script)
		return false;

	if (!(MySelf->GetInternalFlag() & IF_ACTIVE)) {
		return false;
	}

	bool continueExecution = false;
	if (continuing) continueExecution = *continuing;

	RandomNumValue = RAND<int>();
	for (size_t a = 0; a < script->responseBlocks.size(); a++) {
		ResponseBlock* rB = script->responseBlocks[a];
		if (!rB->condition->Evaluate(MySelf)) {
			continue;
		}

		// if this isn't a continue-d block, we have to clear the queue
		// we cannot clear the queue and cannot execute the new block
		// if we already have stuff on the queue!
		if (!continueExecution) {
			if (MySelf->GetCurrentAction() || MySelf->GetNextAction()) {
				if (MySelf->GetInternalFlag() & IF_NOINT) {
					// we presumably don't want any further execution?
					if (done) *done = true;
					return false;
				}

				if (lastResponseBlock == a) {
					// don't interrupt if the already running block still matches
					// eg. a Wait in the middle of the response would eventually cause a recheck of conditions
					// we presumably don't want any further execution?
					// this one is a bit more complicated, due to possible
					// interactions with Continue() (lastResponseBlock here is always
					// the first block encountered), needs more testing
					// we've had a colorful history with this block (look at GF_SKIPUPDATE_HACK), but:
					// - bg2 *does* need this (ar1516 irenicus fight doesn't work properly otherwise); spirit trolls (trollsp01 in ar1506) work regardless
					// - iwd2 targos goblins misbehave without it; see https://github.com/gemrb/gemrb/issues/344 for the gory details
					// - previously we thought iwd:totlm needed this bit (ar9708 djinni never becoming visible, but that's also fine now)
					if (done) {
						*done = true;
					}
					return false;
				}

				// movetoobjectfollow would break if this isn't called
				// (what is broken if it is here?)
				// IE even clears the path, shall we?
				// yes we must :)
				MySelf->Stop();
			}
			lastResponseBlock = a;
		}
		running = true;
		continueExecution = rB->responseSet->Execute(MySelf) != 0;
		running = false;
		if (continuing) *continuing = continueExecution;
		if (!continueExecution) {
			if (done) *done = true;
			return true;
		}
	}
	return continueExecution;
}

//IE simply takes the first action's object for cutscene object
//then adds these actions to its queue:
// SetInterrupt(false), <actions>, SetInterrupt(true)
/*
 * NOTE: After calling, callers should deallocate this object if `dead==true`.
 *  Scripts can replace themselves while running but it's up to the caller to clean up.
 */
void GameScript::EvaluateAllBlocks(bool testConditions)
{
	if (!MySelf || !(MySelf->GetInternalFlag() & IF_ACTIVE) || !script) {
		return;
	}

	// pst and ee StartCutSceneEx don't skip trigger evaluation when the second param is set
	// not needed in vanilla pst, since the only two uses point to cutscenes with just True conditions
	if (testConditions) {
		// this is the more logical way of executing a cutscene, which
		// evaluates conditions and doesn't just use the first response
		for (const auto& rB : script->responseBlocks) {
			if (!rB->condition || !rB->responseSet) continue;
			if (rB->condition->Evaluate(MySelf)) {
				rB->responseSet->Execute(MySelf);
			}
		}
		return;
	}

	// this is the original IE behaviour:
	// cutscenes don't evaluate conditions - they just choose the
	// first response, take the object from the first action,
	// and then add the actions to that object's queue.
	for (const auto& rB : script->responseBlocks) {
		const ResponseSet* rS = rB->responseSet;
		if (rS->responses.empty()) continue;

		Response* response = rS->responses[0];
		if (response->actions.empty()) continue;

		const Action* action = response->actions[0];
		Scriptable* target = GetScriptableFromObject(MySelf, action->objects[1]);
		if (!target) {
			Log(ERROR, "GameScript", "Failed to find CutSceneID target!");
			if (InDebugMode(DebugMode::CUTSCENE) && action->objects[1]) {
				action->objects[1]->dump();
			}
			continue;
		}

		// save the target in case it selfdestructs and we need to manually exit the cutscene
		core->SetCutSceneRunner(target);

		// the original first queued them all similarly to DialogHandler::DialogChoose
		if (target->Type != ST_ACTOR) {
			Action* interrupt = GenerateAction("SetInterrupt(FALSE)");
			interrupt->IncRef(); // SetInterrupt is an instant, so we need to ensure StartCutScene can safely delete it
			response->actions.insert(response->actions.begin(), interrupt);
			interrupt = GenerateAction("SetInterrupt(TRUE)");
			interrupt->IncRef();
			response->actions.push_back(interrupt);
		}
		response->Execute(target);

		// NOTE: this will break blocking instants, if there are any
		target->ReleaseCurrentAction();
	}
}

void GameScript::ExecuteString(Scriptable* Sender, std::string string)
{
	if (string.empty()) {
		return;
	}
	Action* act = GenerateAction(std::move(string));
	if (!act) {
		return;
	}
	Sender->AddActionInFront(act);
}

//This must return integer because Or(3) returns 3
int GameScript::EvaluateString(Scriptable* Sender, const char* String)
{
	if (String[0] == 0) {
		return 0;
	}
	Trigger* tri = GenerateTrigger(String);
	if (tri) {
		int ret = tri->Evaluate(Sender);
		tri->Release();
		return ret;
	}
	return 0;
}

bool Condition::Evaluate(Scriptable* Sender) const
{
	int ORcount = 0;
	unsigned int result = 0;
	bool subresult = true;

	if (triggers.empty()) {
		return true;
	}

	for (const Trigger* tR : triggers) {
		//do not evaluate triggers in an Or() block if one of them
		//was already True() ... but this sane approach was only used in iwd2!
		if (!core->HasFeature(GFFlags::EFFICIENT_OR) || !ORcount || !subresult) {
			result = tR->Evaluate(Sender);
		}
		if (result > 1) {
			//we started an Or() block
			if (ORcount) {
				Log(WARNING, "GameScript", "Unfinished OR block encountered! 1");
				if (!subresult) {
					return false;
				}
			}
			ORcount = result;
			subresult = false;
			continue;
		}
		if (ORcount) {
			subresult |= (result != 0);
			if (--ORcount) {
				continue;
			}
			result = subresult;
		}
		if (!result) {
			return false;
		}
	}
	if (ORcount) {
		Log(WARNING, "GameScript", "Unfinished OR block encountered! 2");
		return subresult;
	}
	return true;
}

/* this may return more than a boolean, in case of Or(x) */
int Trigger::Evaluate(Scriptable* Sender) const
{
	if (triggerID >= MAX_TRIGGERS) {
		Log(ERROR, "GameScript", "Corrupted (too high) trigger code: {}", triggerID);
		return 0;
	}
	TriggerFunction func = triggers[triggerID];
	StringView tmpstr = triggersTable->GetValue(triggerID);
	if (tmpstr.empty()) {
		tmpstr = triggersTable->GetValue(triggerID | 0x4000);
	}
	if (!func) {
		triggers[triggerID] = GameScript::False;
		Log(WARNING, "GameScript", "Unhandled trigger code: {:#x} {}",
		    triggerID, tmpstr);
		return 0;
	}
	ScriptDebugLog(DebugMode::TRIGGERS, "Executing trigger code: {:#x} {} (Sender: {} / {})", triggerID, tmpstr, Sender->GetScriptName(), fmt::WideToChar { Sender->GetName() });

	int ret = func(Sender, this);
	if (flags & TF_NEGATE) {
		return !ret;
	}
	// ideally we'd set LastTrigger here, but we need the resolved target object

	return ret;
}

int ResponseSet::Execute(Scriptable* Sender)
{
	switch (responses.size()) {
		case 0:
			return 0;
		case 1:
			return responses[0]->Execute(Sender);
		default:
			break;
	}

	// ees added a switch-case mode with the Switch trigger
	if (Sender->weightsAsCases) {
		for (Response* response : responses) {
			if (response->weight == Sender->weightsAsCases) {
				Sender->weightsAsCases = 0;
				return response->Execute(Sender);
			}
		}

		Sender->weightsAsCases = 0;
	} else {
		int randWeight = 0;
		int maxWeight = 0;

		for (const Response* response : responses) {
			maxWeight += response->weight;
		}
		if (maxWeight) {
			randWeight = RAND(0, maxWeight - 1);
		}

		for (Response* response : responses) {
			if (response->weight > randWeight) {
				return response->Execute(Sender);
			}
			randWeight -= response->weight;
		}
	}
	return 0;
}

//continue is effective only as the last action in the block
// in iwd2, unlike the rest, instants do get immediately executed
// (if continue will be used, this happens regardless of the queue)
// we know in bg2 this is not the case (c829de45), hence people getting confused why
// SetGlobal calls don't affect conditions after their block was continued from
int Response::Execute(Scriptable* Sender)
{
	static bool startActive = core->HasFeature(GFFlags::START_ACTIVE);
	int ret = 0; // continue or not
	if (actions.empty()) return ret;

	bool iwd2 = core->HasFeature(GFFlags::EFFICIENT_OR);
	const Action* last = actions.back();
	bool hasContinue = false;
	if (iwd2 && actionflags[last->actionID] & AF_CONTINUE) {
		hasContinue = true;
	}

	for (size_t i = 0; i < actions.size(); i++) {
		Action* aC = actions[i];
		bool iwd2Instant = hasContinue && actionflags[aC->actionID] & AF_SCR_INSTANT;
		if ((actionflags[aC->actionID] & AF_MASK) == AF_IMMEDIATE || iwd2Instant) {
			// mimicking AddAction
			Sender->SetInternalFlag(IF_ACTIVE, BitOp::OR);
			if (startActive) Sender->SetInternalFlag(IF_IDLE, BitOp::NAND);
			GameScript::ExecuteAction(Sender, aC);
			ret = 0;
		} else if ((actionflags[aC->actionID] & AF_MASK) == AF_NONE) {
			Sender->AddAction(aC);
			ret = 0;
		} else if ((actionflags[aC->actionID] & AF_MASK) == AF_MASK) {
			// covers also AF_CONTINUE, since Continue, the only user, also has AF_IMMEDIATE
			ret = 1;
		}
	}
	return ret;
}

static void PrintAction(std::string& buffer, int actionID)
{
	AppendFormat(buffer, "Action: {} {}\n", actionID, actionsTable->GetValue(actionID));
}

static void HandleActionOverride(Scriptable* target, const Action* aC)
{
	Action* newAction = ParamCopyNoOverride(aC);
	// mark the target action, so other ActionOverrides don't clear it
	// only happened for queued actions, but since instants are gone immediately,
	// it shouldn't matter that we set it on all
	newAction->flags |= ACF_OVERRIDE;

	if (core->HasFeature(GFFlags::CLEARING_ACTIONOVERRIDE)) {
		// bg2, but not iwd2, clears the previous non-actionoverriden actions in the queue
		target->ClearActions(1);
	} else if (core->HasFeature(GFFlags::RULES_3ED)) { // iwd2
		// it was more complicated, always releasing if the game was paused — not something to replicate
		if (target->CurrentActionInterruptible) {
			target->ReleaseCurrentAction();
		}
	} else { // for all the games we don't understand fully yet
		target->ReleaseCurrentAction();
	}

	target->AddAction(newAction);
	if (!(actionflags[aC->actionID] & AF_INSTANT)) {
		assert(target->GetNextAction());
		// there are plenty of places where it's vital that ActionOverride is not interrupted,
		// so make double sure it doesn't happen
		target->CurrentActionInterruptible = false;
	}
}

static bool CheckSleepException(const Scriptable* sender, int actionID)
{
	const Actor* actor = Scriptable::As<Actor>(sender);
	if (!actor) return false;
	if (actionflags[actionID] & AF_SLEEP) return false;

	// action is not in actsleep.ids, so better be awake
	if (actor->GetStat(IE_STATE_ID) & STATE_SLEEP) {
		return true;
	}
	return false;
}

// allow only instants to run on dead actors
static bool CheckDeadException(const Scriptable* sender, int actionID)
{
	const Actor* actor = Scriptable::As<Actor>(sender);
	if (!actor) return false;
	if (!(actor->GetStat(IE_STATE_ID) & STATE_DEAD)) return false;

	// original also required AF_SLEEP, but we're probably fine without
	if (actionflags[actionID] & AF_INSTANT) return false;
	return true;
}

void GameScript::ExecuteAction(Scriptable* Sender, Action* aC)
{
	int actionID = aC->actionID;

	// reallow area scripts after us, if they were disabled
	if (aC->flags & ACF_REALLOW_SCRIPTS) {
		core->GetGameControl()->SetDialogueFlags(DF_POSTPONE_SCRIPTS, BitOp::NAND);
	}

	// check for ActionOverride
	// actions use the second and third object, so this is only set when overridden (see GenerateActionCore)
	const Object* overrider = aC->objects[0];
	if (overrider) {
		Scriptable* scr = GetScriptableFromObject(Sender, overrider);
		if (CheckDeadException(scr, actionID)) {
			scr = GetScriptableFromObject(Sender, overrider, GA_NO_DEAD);
		}
		aC->IncRef(); // if aC is us, we don't want it deleted!
		Sender->ReleaseCurrentAction();

		if (scr) {
			if (CheckSleepException(scr, actionID)) {
				ScriptDebugLog(DebugMode::ACTIONS, "Sender {} tried to run ActionOverride on a sleeping {} with incompatible action {}", Sender->GetScriptName(), scr->GetScriptName(), actionID);
				return;
			}
			ScriptDebugLog(DebugMode::ACTIONS, "Sender {} ran ActionOverride on {}", Sender->GetScriptName(), scr->GetScriptName());
			HandleActionOverride(scr, aC);
		} else {
			// skip showing errors when party size is lower than the (original) max
			bool pc = overrider->objectFilters[0] >= 21 && overrider->objectFilters[0] < 27; // Player2-Player6
			if (!pc) {
				Log(ERROR, "GameScript", "ActionOverride failed for object and action: ");
				overrider->dump();
				aC->dump();
			}
		}
		aC->Release();
		return;
	}
	if (CheckSleepException(Sender, actionID)) {
		ScriptDebugLog(DebugMode::ACTIONS, "Sleeping sender {} tried to run non-compatible action {}", Sender->GetScriptName(), actionID);
		Sender->ReleaseCurrentAction();
		return;
	}
	if (InDebugMode(DebugMode::ACTIONS)) {
		std::string buffer;
		PrintAction(buffer, actionID);
		AppendFormat(buffer, "Sender: {}\n", Sender->GetScriptName());
		Log(WARNING, "GameScript", "{}", buffer);
	}

	ActionFunction func = actions[actionID];
	if (!func) {
		actions[actionID] = NoAction;
		std::string buffer("Unknown ");
		PrintAction(buffer, actionID);
		Log(WARNING, "GameScript", "{}", buffer);
		Sender->ReleaseCurrentAction();
		return;
	}

	// turning off interruptible flag
	// uninterruptible actions will set it back
	if (Sender->Type == ST_ACTOR) {
		Sender->Activate();
		if (actionflags[actionID] & AF_ALIVE && Sender->GetInternalFlag() & IF_STOPATTACK) {
			Log(WARNING, "GameScript", "{}", "Aborted action due to death!");
			Sender->ReleaseCurrentAction();
			return;
		}
	}
	func(Sender, aC);

	//don't bother with special flow control actions
	if (actionflags[actionID] & AF_IMMEDIATE) {
		//this action never entered the action queue, therefore shouldn't be freed
		if (aC->GetRef() != 1) {
			std::string buffer("Immediate action got queued!\n");
			PrintAction(buffer, actionID);
			Log(ERROR, "GameScript", "{}", buffer);
			error("GameScript", "aborting...");
		}
		return;
	}

	//Releasing nonblocking actions, blocking actions will release themselves
	if (!(actionflags[actionID] & AF_BLOCKING)) {
		Sender->ReleaseCurrentAction();
		//aC is invalid beyond this point, so we return!
		return;
	}
}

std::string Object::dump(bool print) const
{
	AssertCanary(__func__);
	std::string buffer;
	if (objectName[0]) {
		AppendFormat(buffer, "Object: {}\n", objectName);
		return buffer;
	}
	AppendFormat(buffer, "IDS Targeting: ");
	for (auto objectField : objectFields) {
		AppendFormat(buffer, "{} ", objectField);
	}
	buffer.append("\n");
	buffer.append("Filters: ");
	for (auto objectFilter : objectFilters) {
		AppendFormat(buffer, "{} ", objectFilter);
	}
	buffer.append("\n");

	if (print) Log(DEBUG, "GameScript", "{}", buffer);
	return buffer;
}

/** Return true if object is null */
bool Object::isNull() const
{
	if (objectName[0] != 0) {
		return false;
	}
	if (objectFilters[0]) {
		return false;
	}
	for (int i = 0; i < ObjectFieldsCount; i++) {
		if (objectFields[i]) {
			return false;
		}
	}
	return true;
}

std::string Trigger::dump() const
{
	AssertCanary(__func__);
	std::string buffer;
	AppendFormat(buffer, "Trigger: {}\n", triggerID);
	AppendFormat(buffer, "Int parameters: {} {} {}\n", int0Parameter, int1Parameter, int2Parameter);
	AppendFormat(buffer, "Point: {}\n", pointParameter);
	AppendFormat(buffer, "String0: {}\n", string0Parameter);
	AppendFormat(buffer, "String1: {}\n", string1Parameter);
	if (objectParameter) {
		buffer.append(objectParameter->dump(false));
	} else {
		AppendFormat(buffer, "No object\n");
	}
	AppendFormat(buffer, "\n");
	Log(DEBUG, "GameScript", "{}", buffer);
	return buffer;
}

std::string Action::dump() const
{
	AssertCanary(__func__);
	std::string buffer;
	AppendFormat(buffer, "Int0: {}, Int1: {}, Int2: {}\n", int0Parameter, int1Parameter, int2Parameter);
	AppendFormat(buffer, "String0: {}, String1: {}\n", string0Parameter, string1Parameter);
	AppendFormat(buffer, "Point: {}\n", pointParameter);
	for (int i = 0; i < 3; i++) {
		if (objects[i]) {
			AppendFormat(buffer, "{}. ", i + 1);
			buffer.append(objects[i]->dump(false));
		} else {
			AppendFormat(buffer, "{}. Object - NULL\n", i + 1);
		}
	}

	AppendFormat(buffer, "RefCount: {}\tactionID: {}\n", RefCount, actionID);
	Log(DEBUG, "GameScript", "{}", buffer);
	return buffer;
}

}
