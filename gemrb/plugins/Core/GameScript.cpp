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
 * $Id$
 *
 */

#include "../../includes/win32def.h"
#include "GameScript.h"
#include "Interface.h"
#include "GSUtils.h"
#include "ResourceMgr.h"
#include "Game.h"
#include "GameControl.h"

//debug flags
// 1 - cache
// 2 - cutscene ID
// 4 - globals
// 8 - action execution
//16 - trigger evaluation

static bool charnameisgabber = false;

//Make this an ordered list, so we could use bsearch!
static const TriggerLink triggernames[] = {
	{"actionlistempty", GameScript::ActionListEmpty, 0},
	{"actuallyincombat", GameScript::ActuallyInCombat, 0},
	{"acquired", GameScript::Acquired, 0},
	{"alignment", GameScript::Alignment, 0},
	{"allegiance", GameScript::Allegiance, 0},
	{"animstate", GameScript::AnimState, 0},
	{"anypconmap", GameScript::AnyPCOnMap, 0},
	{"anypcseesenemy", GameScript::AnyPCSeesEnemy, 0},
	{"areacheck", GameScript::AreaCheck, 0},
	{"areacheckobject", GameScript::AreaCheckObject, 0},
	{"areaflag", GameScript::AreaFlag, 0},
	{"arearestdisabled", GameScript::AreaRestDisabled, 0},
	{"areatype", GameScript::AreaType, 0},
	{"atlocation", GameScript::AtLocation, 0},
	{"assaltedby", GameScript::AttackedBy, 0},//pst
	{"attackedby", GameScript::AttackedBy, 0},
	{"becamevisible", GameScript::BecameVisible, 0},
	{"bitcheck", GameScript::BitCheck,TF_MERGESTRINGS},
	{"bitcheckexact", GameScript::BitCheckExact,TF_MERGESTRINGS},
	{"bitglobal", GameScript::BitGlobal_Trigger,TF_MERGESTRINGS},
	{"breakingpoint", GameScript::BreakingPoint, 0},
	{"calanderday", GameScript::CalendarDay, 0}, //illiterate developers O_o
	{"calendarday", GameScript::CalendarDay, 0},
	{"calanderdaygt", GameScript::CalendarDayGT, 0},
	{"calendardaygt", GameScript::CalendarDayGT, 0},
	{"calanderdaylt", GameScript::CalendarDayLT, 0},
	{"calendardaylt", GameScript::CalendarDayLT, 0},
	{"calledbyname", GameScript::CalledByName, 0}, //this is still a question
	{"chargecount", GameScript::ChargeCount, 0},
	{"charname", GameScript::CharName, 0}, //not scripting name
	{"checkareadifflevel", GameScript::DifficultyLT, 0},//iwd2 guess
	{"checkdoorflags", GameScript::CheckDoorFlags, 0},
	{"checkpartyaveragelevel", GameScript::CheckPartyAverageLevel, 0},
	{"checkpartylevel", GameScript::CheckPartyLevel, 0},
	{"checkskill", GameScript::CheckSkill, 0},
	{"checkskillgt", GameScript::CheckSkillGT, 0},
	{"checkskilllt", GameScript::CheckSkillLT, 0},
	{"checkspellstate", GameScript::CheckSpellState, 0},
	{"checkstat", GameScript::CheckStat, 0},
	{"checkstatgt", GameScript::CheckStatGT, 0},
	{"checkstatlt", GameScript::CheckStatLT, 0},
	{"class", GameScript::Class, 0},
	{"classex", GameScript::ClassEx, 0}, //will return true for multis
	{"classlevel", GameScript::ClassLevel, 0}, //pst
	{"classlevelgt", GameScript::ClassLevelGT, 0},
	{"classlevellt", GameScript::ClassLevelLT, 0},
	{"clicked", GameScript::Clicked, 0},
	{"closed", GameScript::Closed, 0},
	{"combatcounter", GameScript::CombatCounter, 0},
	{"combatcountergt", GameScript::CombatCounterGT, 0},
	{"combatcounterlt", GameScript::CombatCounterLT, 0},
	{"contains", GameScript::Contains, 0},
	{"currentareais", GameScript::CurrentAreaIs, 0},//checks object
	{"creaturehidden", GameScript::CreatureHidden, 0},//this is the engine level hiding feature, not the skill
	{"creatureinarea", GameScript::AreaCheck, 0}, //pst, checks this object
	{"damagetaken", GameScript::HPLost, 0},
	{"damagetakengt", GameScript::HPLostGT, 0},
	{"damagetakenlt", GameScript::HPLostLT, 0},
	{"dead", GameScript::Dead, 0},
	{"delay", GameScript::Delay, 0},
	{"detect", GameScript::Detect, 0}, //so far i see no difference
	{"die", GameScript::Die, 0},
	{"died", GameScript::Died, 0},
	{"difficulty", GameScript::Difficulty, 0},
	{"difficultygt", GameScript::DifficultyGT, 0},
	{"difficultylt", GameScript::DifficultyLT, 0},
	{"disarmed", GameScript::Disarmed, 0},
	{"disarmfailed", GameScript::DisarmFailed, 0},
	{"entered", GameScript::Entered, 0},
	{"entirepartyonmap", GameScript::EntirePartyOnMap, 0},
	{"exists", GameScript::Exists, 0},
	{"extendedstatecheck", GameScript::ExtendedStateCheck, 0},
	{"extraproficiency", GameScript::ExtraProficiency, 0},
	{"extraproficiencygt", GameScript::ExtraProficiencyGT, 0},
	{"extraproficiencylt", GameScript::ExtraProficiencyLT, 0},
	{"faction", GameScript::Faction, 0},
	{"failedtoopen", GameScript::OpenFailed, 0},
	{"fallenpaladin", GameScript::FallenPaladin, 0},
	{"fallenranger", GameScript::FallenRanger, 0},
	{"false", GameScript::False, 0},
	{"frame", GameScript::Frame, 0},
	{"g", GameScript::G_Trigger, 0},
	{"gender", GameScript::Gender, 0},
	{"general", GameScript::General, 0},
	{"ggt", GameScript::GGT_Trigger, 0},
	{"glt", GameScript::GLT_Trigger, 0},
	{"global", GameScript::Global,TF_MERGESTRINGS},
	{"globalandglobal", GameScript::GlobalAndGlobal_Trigger,TF_MERGESTRINGS},
	{"globalband", GameScript::BitCheck,TF_MERGESTRINGS},
	{"globalbandglobal", GameScript::GlobalBAndGlobal_Trigger,TF_MERGESTRINGS},
	{"globalbandglobalexact", GameScript::GlobalBAndGlobalExact,TF_MERGESTRINGS},
	{"globalbitglobal", GameScript::GlobalBitGlobal_Trigger,TF_MERGESTRINGS},
	{"globalequalsglobal", GameScript::GlobalsEqual,TF_MERGESTRINGS}, //this is the same
	{"globalgt", GameScript::GlobalGT,TF_MERGESTRINGS},
	{"globalgtglobal", GameScript::GlobalGTGlobal,TF_MERGESTRINGS},
	{"globallt", GameScript::GlobalLT,TF_MERGESTRINGS},
	{"globalltglobal", GameScript::GlobalLTGlobal,TF_MERGESTRINGS},
	{"globalorglobal", GameScript::GlobalOrGlobal_Trigger,TF_MERGESTRINGS},
	{"globalsequal", GameScript::GlobalsEqual, 0},
	{"globalsgt", GameScript::GlobalsGT, 0},
	{"globalslt", GameScript::GlobalsLT, 0},
	{"globaltimerexact", GameScript::GlobalTimerExact, 0},
	{"globaltimerexpired", GameScript::GlobalTimerExpired, 0},
	{"globaltimernotexpired", GameScript::GlobalTimerNotExpired, 0},
	{"globaltimerstarted", GameScript::GlobalTimerStarted, 0},
	{"happiness", GameScript::Happiness, 0},
	{"happinessgt", GameScript::HappinessGT, 0},
	{"happinesslt", GameScript::HappinessLT, 0},
	{"harmlessclosed", GameScript::Closed, 0}, //pst, not sure
	{"harmlessentered", GameScript::HarmlessEntered, 0}, //???
	{"harmlessopened", GameScript::Opened, 0}, //pst, not sure
	{"hasbounceeffects", GameScript::HasBounceEffects, 0},
	{"hasimmunityeffects", GameScript::HasImmunityEffects, 0},
	{"hasinnateability", GameScript::HaveSpell, 0}, //these must be the same
	{"hasitem", GameScript::HasItem, 0},
	{"hasitemequiped", GameScript::HasItemEquipped, 0}, //typo in bg2
	{"hasitemequipedreal", GameScript::HasItemEquipped, 0}, //not sure
	{"hasitemequipped", GameScript::HasItemEquipped, 0},
	{"hasitemequippedreal", GameScript::HasItemEquipped, 0}, //not sure
	{"hasiteminslot", GameScript::HasItemSlot, 0},
	{"hasitemslot", GameScript::HasItemSlot, 0},
	{"hasitemtypeslot", GameScript::HasItemTypeSlot, 0},//gemrb extension
	{"hasweaponequiped", GameScript::HasWeaponEquipped, 0},//a typo again
	{"hasweaponequipped", GameScript::HasWeaponEquipped, 0},
	{"haveanyspells", GameScript::HaveAnySpells, 0},
	{"havespell", GameScript::HaveSpell, 0}, //these must be the same
	{"havespellparty", GameScript::HaveSpellParty, 0},
	{"havespellres", GameScript::HaveSpell, 0}, //they share the same ID
	{"haveusableweaponequipped", GameScript::HaveUsableWeaponEquipped, 0},
	{"heard", GameScript::Heard, 0},
	{"help", GameScript::Help_Trigger, 0},
	{"helpex", GameScript::HelpEX, 0},
	{"hitby", GameScript::HitBy, 0},
	{"hotkey", GameScript::HotKey, 0},
	{"hp", GameScript::HP, 0},
	{"hpgt", GameScript::HPGT, 0},
	{"hplost", GameScript::HPLost, 0},
	{"hplostgt", GameScript::HPLostGT, 0},
	{"hplostlt", GameScript::HPLostLT, 0},
	{"hplt", GameScript::HPLT, 0},
	{"hppercent", GameScript::HPPercent, 0},
	{"hppercentgt", GameScript::HPPercentGT, 0},
	{"hppercentlt", GameScript::HPPercentLT, 0},
	{"inactivearea", GameScript::InActiveArea, 0},
	{"incutscenemode", GameScript::InCutSceneMode, 0},
	{"inmyarea", GameScript::InMyArea, 0},
	{"inmygroup", GameScript::InMyGroup, 0},
	{"inparty", GameScript::InParty, 0},
	{"inpartyallowdead", GameScript::InPartyAllowDead, 0},
	{"inpartyslot", GameScript::InPartySlot, 0},
	{"internal", GameScript::Internal, 0},
	{"internalgt", GameScript::InternalGT, 0},
	{"internallt", GameScript::InternalLT, 0},
	{"interactingwith", GameScript::InteractingWith, 0},
	{"intrap", GameScript::InTrap, 0},
	{"inventoryfull", GameScript::InventoryFull, 0},
	{"inview", GameScript::LOS, 0}, //it seems the same, needs research
	{"inwatcherskeep", GameScript::AreaStartsWith, 0},
	{"inweaponrange", GameScript::InWeaponRange, 0},
	{"isaclown", GameScript::IsAClown, 0},
	{"isactive", GameScript::IsActive, 0},
	{"isanimationid", GameScript::AnimationID, 0},
	{"iscreatureareaflag", GameScript::IsCreatureAreaFlag, 0},
	{"isfacingobject", GameScript::IsFacingObject, 0},
	{"isfacingsavedrotation", GameScript::IsFacingSavedRotation, 0},
	{"isgabber", GameScript::IsGabber, 0},
	{"isheartoffurymodeon", GameScript::NightmareModeOn, 0},
	{"islocked", GameScript::IsLocked, 0},
	{"isextendednight", GameScript::IsExtendedNight, 0},
	{"isoverme", GameScript::IsOverMe, 0},
	{"isplayernumber", GameScript::IsPlayerNumber, 0},
	{"isrotation", GameScript::IsRotation, 0},
	{"isscriptname", GameScript::CalledByName, 0}, //seems the same
	{"isteambiton", GameScript::IsTeamBitOn, 0},
	{"isvalidforpartydialog", GameScript::IsValidForPartyDialog, 0},
	{"isvalidforpartydialogue", GameScript::IsValidForPartyDialog, 0},
	{"isweaponranged", GameScript::IsWeaponRanged, 0},
	{"isweather", GameScript::IsWeather, 0}, //gemrb extension
	{"itemisidentified", GameScript::ItemIsIdentified, 0},
	{"kit", GameScript::Kit, 0},
	{"knowspell", GameScript::KnowSpell, 0}, //gemrb specific
	{"lastmarkedobject", GameScript::LastMarkedObject_Trigger, 0},
	{"lastpersontalkedto", GameScript::LastPersonTalkedTo, 0}, //pst
	{"level", GameScript::Level, 0},
	{"levelgt", GameScript::LevelGT, 0},
	{"levelinclass", GameScript::ClassLevel, 0}, //iwd2
	{"levelinclassgt", GameScript::ClassLevelGT, 0},
	{"levelinclasslt", GameScript::ClassLevelLT, 0},
	{"levellt", GameScript::LevelLT, 0},
	{"levelparty", GameScript::LevelParty, 0},
	{"levelpartygt", GameScript::LevelPartyGT, 0},
	{"levelpartylt", GameScript::LevelPartyLT, 0},
	{"localsequal", GameScript::LocalsEqual, 0},
	{"localsgt", GameScript::LocalsGT, 0},
	{"localslt", GameScript::LocalsLT, 0},
	{"los", GameScript::LOS, 0},
	{"modalstate", GameScript::ModalState, 0},
	{"morale", GameScript::Morale, 0},
	{"moralegt", GameScript::MoraleGT, 0},
	{"moralelt", GameScript::MoraleLT, 0},
	{"name", GameScript::CalledByName, 0}, //this is the same too?
	{"namelessbitthedust", GameScript::NamelessBitTheDust, 0},
	{"nearbydialog", GameScript::NearbyDialog, 0},
	{"nearbydialogue", GameScript::NearbyDialog, 0},
	{"nearlocation", GameScript::NearLocation, 0},
	{"nearsavedlocation", GameScript::NearSavedLocation, 0},
	{"nightmaremodeon", GameScript::NightmareModeOn, 0},
	{"notstatecheck", GameScript::NotStateCheck, 0},
	{"nulldialog", GameScript::NullDialog, 0},
	{"nulldialogue", GameScript::NullDialog, 0},
	{"numcreature", GameScript::NumCreatures, 0},
	{"numcreaturegt", GameScript::NumCreaturesGT, 0},
	{"numcreaturelt", GameScript::NumCreaturesLT, 0},
	{"numcreaturesatmylevel", GameScript::NumCreaturesAtMyLevel, 0},
	{"numcreaturesgtmylevel", GameScript::NumCreaturesGTMyLevel, 0},
	{"numcreaturesltmylevel", GameScript::NumCreaturesLTMyLevel, 0},
	{"numcreaturevsparty", GameScript::NumCreatureVsParty, 0},
	{"numcreaturevspartygt", GameScript::NumCreatureVsPartyGT, 0},
	{"numcreaturevspartylt", GameScript::NumCreatureVsPartyLT, 0},
	{"numdead", GameScript::NumDead, 0},
	{"numdeadgt", GameScript::NumDeadGT, 0},
	{"numdeadlt", GameScript::NumDeadLT, 0},
	{"numinparty", GameScript::PartyCountEQ, 0},
	{"numinpartyalive", GameScript::PartyCountAliveEQ, 0},
	{"numinpartyalivegt", GameScript::PartyCountAliveGT, 0},
	{"numinpartyalivelt", GameScript::PartyCountAliveLT, 0},
	{"numinpartygt", GameScript::PartyCountGT, 0},
	{"numinpartylt", GameScript::PartyCountLT, 0},
	{"numitems", GameScript::NumItems, 0},
	{"numitemsgt", GameScript::NumItemsGT, 0},
	{"numitemslt", GameScript::NumItemsLT, 0},
	{"numitemsparty", GameScript::NumItemsParty, 0},
	{"numitemspartygt", GameScript::NumItemsPartyGT, 0},
	{"numitemspartylt", GameScript::NumItemsPartyLT, 0},
	{"numtimesinteracted", GameScript::NumTimesInteracted, 0},
	{"numtimesinteractedgt", GameScript::NumTimesInteractedGT, 0},
	{"numtimesinteractedlt", GameScript::NumTimesInteractedLT, 0},
	{"numtimesinteractedobject", GameScript::NumTimesInteractedObject, 0},//gemrb
	{"numtimesinteractedobjectgt", GameScript::NumTimesInteractedObjectGT, 0},//gemrb
	{"numtimesinteractedobjectlt", GameScript::NumTimesInteractedObjectLT, 0},//gemrb
	{"numtimestalkedto", GameScript::NumTimesTalkedTo, 0},
	{"numtimestalkedtogt", GameScript::NumTimesTalkedToGT, 0},
	{"numtimestalkedtolt", GameScript::NumTimesTalkedToLT, 0},
	{"objectactionlistempty", GameScript::ObjectActionListEmpty, 0}, //same function
	{"objitemcounteq", GameScript::NumItems, 0},
	{"objitemcountgt", GameScript::NumItemsGT, 0},
	{"objitemcountlt", GameScript::NumItemsLT, 0},
	{"oncreation", GameScript::OnCreation, 0},
	{"onscreen", GameScript::OnScreen, 0},
	{"opened", GameScript::Opened, 0},
	{"openfailed", GameScript::OpenFailed, 0},
	{"openstate", GameScript::OpenState, 0},
	{"or", GameScript::Or, 0},
	{"outofammo", GameScript::OutOfAmmo, 0},
	{"ownsfloatermessage", GameScript::OwnsFloaterMessage, 0},
	{"partycounteq", GameScript::PartyCountEQ, 0},
	{"partycountgt", GameScript::PartyCountGT, 0},
	{"partycountlt", GameScript::PartyCountLT, 0},
	{"partygold", GameScript::PartyGold, 0},
	{"partygoldgt", GameScript::PartyGoldGT, 0},
	{"partygoldlt", GameScript::PartyGoldLT, 0},
	{"partyhasitem", GameScript::PartyHasItem, 0},
	{"partyhasitemidentified", GameScript::PartyHasItemIdentified, 0},
	{"partyitemcounteq", GameScript::NumItemsParty, 0},
	{"partyitemcountgt", GameScript::NumItemsPartyGT, 0},
	{"partyitemcountlt", GameScript::NumItemsPartyLT, 0},
	{"partymemberdied", GameScript::PartyMemberDied, 0},
	{"partyrested", GameScript::PartyRested, 0},
	{"pccanseepoint", GameScript::PCCanSeePoint, 0},
	{"pcinstore", GameScript::PCInStore, 0},
	{"personalspacedistance", GameScript::PersonalSpaceDistance, 0},
	{"picklockfailed", GameScript::PickLockFailed, 0},
	{"pickpocketfailed", GameScript::PickpocketFailed, 0},
	{"proficiency", GameScript::Proficiency, 0},
	{"proficiencygt", GameScript::ProficiencyGT, 0},
	{"proficiencylt", GameScript::ProficiencyLT, 0},
	{"race", GameScript::Race, 0},
	{"randomnum", GameScript::RandomNum, 0},
	{"randomnumgt", GameScript::RandomNumGT, 0},
	{"randomnumlt", GameScript::RandomNumLT, 0},
	{"randomstatcheck", GameScript::RandomStatCheck, 0},
	{"range", GameScript::Range, 0},
	{"reaction", GameScript::Reaction, 0},
	{"reactiongt", GameScript::ReactionGT, 0},
	{"reactionlt", GameScript::ReactionLT, 0},
	{"realglobaltimerexact", GameScript::RealGlobalTimerExact, 0},
	{"realglobaltimerexpired", GameScript::RealGlobalTimerExpired, 0},
	{"realglobaltimernotexpired", GameScript::RealGlobalTimerNotExpired, 0},
	{"receivedorder", GameScript::ReceivedOrder, 0},
	{"reputation", GameScript::Reputation, 0},
	{"reputationgt", GameScript::ReputationGT, 0},
	{"reputationlt", GameScript::ReputationLT, 0},
	{"school", GameScript::School, 0}, //similar to kit
	{"see", GameScript::See, 0},
	{"sequence", GameScript::Sequence, 0},
	{"setlastmarkedobject", GameScript::SetLastMarkedObject, 0},
	{"specifics", GameScript::Specifics, 0},
	{"spellcast", GameScript::SpellCast, 0},
	{"spellcastinnate", GameScript::SpellCastInnate, 0},
	{"spellcastonme", GameScript::SpellCastOnMe, 0},
	{"spellcastpriest", GameScript::SpellCastPriest, 0},
	{"statecheck", GameScript::StateCheck, 0},
	{"stealfailed", GameScript::StealFailed, 0},
	{"storehasitem", GameScript::StoreHasItem, 0},
	{"stuffglobalrandom", GameScript::StuffGlobalRandom, 0},//hm, this is a trigger
	{"subrace", GameScript::SubRace, 0},
	{"systemvariable", GameScript::SystemVariable_Trigger, 0}, //gemrb
	{"targetunreachable", GameScript::TargetUnreachable, 0},
	{"team", GameScript::Team, 0},
	{"time", GameScript::Time, 0},
	{"timegt", GameScript::TimeGT, 0},
	{"timelt", GameScript::TimeLT, 0},
	{"timeofday", GameScript::TimeOfDay, 0},
	{"timeractive", GameScript::TimerActive, 0},
	{"timerexpired", GameScript::TimerExpired, 0},
	{"tookdamage", GameScript::TookDamage, 0},
	{"totalitemcnt", GameScript::TotalItemCnt, 0}, //iwd2
	{"totalitemcntexclude", GameScript::TotalItemCntExclude, 0}, //iwd2
	{"totalitemcntexcludegt", GameScript::TotalItemCntExcludeGT, 0}, //iwd2
	{"totalitemcntexcludelt", GameScript::TotalItemCntExcludeLT, 0}, //iwd2
	{"totalitemcntgt", GameScript::TotalItemCntGT, 0}, //iwd2
	{"totalitemcntlt", GameScript::TotalItemCntLT, 0}, //iwd2
	{"traptriggered", GameScript::TrapTriggered, 0},
	{"trigger", GameScript::TriggerTrigger, 0},
	{"triggerclick", GameScript::Clicked, 0}, //not sure
	{"triggersetglobal", GameScript::TriggerSetGlobal,0}, //iwd2, but never used
	{"true", GameScript::True, 0},
	{"unlocked", GameScript::Unlocked, 0},
	{"unselectablevariable", GameScript::UnselectableVariable, 0},
	{"unselectablevariablegt", GameScript::UnselectableVariableGT, 0},
	{"unselectablevariablelt", GameScript::UnselectableVariableLT, 0},
	{"unusable",GameScript::Unusable, 0},
	{"vacant",GameScript::Vacant, 0},
	{"walkedtotrigger", GameScript::WalkedToTrigger, 0},
	{"wasindialog", GameScript::WasInDialog, 0},
	{"xor", GameScript::Xor,TF_MERGESTRINGS},
	{"xp", GameScript::XP, 0},
	{"xpgt", GameScript::XPGT, 0},
	{"xplt", GameScript::XPLT, 0},
	{ NULL,NULL,0}
};

//Make this an ordered list, so we could use bsearch!
static const ActionLink actionnames[] = {
	{"actionoverride",NULL, AF_INVALID}, //will this function ever be reached
	{"activate", GameScript::Activate, 0},
	{"activateportalcursor", GameScript::ActivatePortalCursor, 0},
	{"addareaflag", GameScript::AddAreaFlag, 0},
	{"addareatype", GameScript::AddAreaType, 0},
	{"addexperienceparty", GameScript::AddExperienceParty, 0},
	{"addexperiencepartycr", GameScript::AddExperiencePartyCR, 0},
	{"addexperiencepartyglobal", GameScript::AddExperiencePartyGlobal, 0},
	{"addfeat", GameScript::AddFeat, 0},
	{"addglobals", GameScript::AddGlobals, 0},
	{"addhp", GameScript::AddHP, 0},
	{"addjournalentry", GameScript::AddJournalEntry, 0},
	{"addkit", GameScript::AddKit, 0},
	{"addmapnote", GameScript::AddMapnote, 0},
	{"addpartyexperience", GameScript::AddExperienceParty, 0},
	{"addspecialability", GameScript::AddSpecialAbility, 0},
	{"addsuperkit", GameScript::AddSuperKit, 0},
	{"addwaypoint", GameScript::AddWayPoint,AF_BLOCKING},
	{"addxp2da", GameScript::AddXP2DA, 0},
	{"addxpobject", GameScript::AddXPObject, 0},
	{"addxpvar", GameScript::AddXP2DA, 0},
	{"advancetime", GameScript::AdvanceTime, 0},
	{"allowarearesting", GameScript::SetAreaRestFlag, 0},//iwd2
	{"ally", GameScript::Ally, 0},
	{"ambientactivate", GameScript::AmbientActivate, 0},
	{"applydamage", GameScript::ApplyDamage, 0},
	{"applydamagepercent", GameScript::ApplyDamagePercent, 0},
	{"applyspell", GameScript::ApplySpell, 0},
	{"applyspellpoint", GameScript::ApplySpellPoint, 0}, //gemrb extension
	{"attachtransitiontodoor", GameScript::AttachTransitionToDoor, 0},
	{"attack", GameScript::Attack,AF_BLOCKING|AF_ALIVE},
	{"attacknosound", GameScript::AttackNoSound,AF_BLOCKING|AF_ALIVE}, //no sound yet anyway
	{"attackoneround", GameScript::AttackOneRound,AF_BLOCKING|AF_ALIVE},
	{"attackreevaluate", GameScript::AttackReevaluate,AF_BLOCKING|AF_ALIVE},
	{"backstab", GameScript::Attack,AF_BLOCKING|AF_ALIVE},//actually hide+attack
	{"banterblockflag", GameScript::BanterBlockFlag,0},
	{"banterblocktime", GameScript::BanterBlockTime,0},
	{"bashdoor", GameScript::BashDoor,AF_BLOCKING|AF_ALIVE}, //the same until we know better
	{"battlesong", GameScript::BattleSong, AF_ALIVE},
	{"berserk", GameScript::Berserk, AF_ALIVE},
	{"bitclear", GameScript::BitClear,AF_MERGESTRINGS},
	{"bitglobal", GameScript::BitGlobal,AF_MERGESTRINGS},
	{"bitset", GameScript::GlobalBOr,AF_MERGESTRINGS}, //probably the same
	{"breakinstants", GameScript::BreakInstants, AF_BLOCKING},//delay execution of instants to the next AI cycle???
	{"calllightning", GameScript::Kill, 0}, //TODO: call lightning projectile
	{"calm", GameScript::Calm, 0},
	{"changeaiscript", GameScript::ChangeAIScript, 0},
	{"changeaitype", GameScript::ChangeAIType, 0},
	{"changealignment", GameScript::ChangeAlignment, 0},
	{"changeallegiance", GameScript::ChangeAllegiance, 0},
	{"changeanimation", GameScript::ChangeAnimation, 0},
	{"changeanimationnoeffect", GameScript::ChangeAnimationNoEffect, 0},
	{"changeclass", GameScript::ChangeClass, 0},
	{"changecolor", GameScript::ChangeColor, 0},
	{"changecurrentscript", GameScript::ChangeAIScript,AF_SCRIPTLEVEL},
	{"changedestination", GameScript::ChangeDestination,0}, //gemrb extension (iwd hack)
	{"changedialog", GameScript::ChangeDialogue, 0},
	{"changedialogue", GameScript::ChangeDialogue, 0},
	{"changegender", GameScript::ChangeGender, 0},
	{"changegeneral", GameScript::ChangeGeneral, 0},
	{"changeenemyally", GameScript::ChangeAllegiance, 0}, //this is the same
	{"changefaction", GameScript::SetFaction, 0}, //pst
	{"changerace", GameScript::ChangeRace, 0},
	{"changespecifics", GameScript::ChangeSpecifics, 0},
	{"changestat", GameScript::ChangeStat, 0},
	{"changestatglobal", GameScript::ChangeStatGlobal, 0},
	{"changestoremarkup", GameScript::ChangeStoreMarkup, 0},//iwd2
	{"changeteam", GameScript::SetTeam, 0}, //pst
	{"changetilestate", GameScript::ChangeTileState, 0}, //bg2
	{"chunkcreature", GameScript::Kill, 0}, //should be more graphical
	{"clearactions", GameScript::ClearActions, AF_INSTANT},
	{"clearallactions", GameScript::ClearAllActions, 0},
	{"clearpartyeffects", GameScript::ClearPartyEffects, 0},
	{"clearspriteeffects", GameScript::ClearSpriteEffects, 0},
	{"clicklbuttonobject", GameScript::ClickLButtonObject, AF_BLOCKING},
	{"clicklbuttonpoint", GameScript::ClickLButtonPoint, AF_BLOCKING},
	{"clickrbuttonobject", GameScript::ClickLButtonObject, AF_BLOCKING},
	{"clickrbuttonpoint", GameScript::ClickLButtonPoint, AF_BLOCKING},
	{"closedoor", GameScript::CloseDoor,0},
	{"containerenable", GameScript::ContainerEnable, 0},
	{"continue", GameScript::Continue,AF_INSTANT | AF_CONTINUE},
	{"copygroundpilesto", GameScript::CopyGroundPilesTo, 0},
	{"createcreature", GameScript::CreateCreature, 0}, //point is relative to Sender
	{"createcreaturecopypoint", GameScript::CreateCreatureCopyPoint, 0}, //point is relative to Sender
	{"createcreaturedoor", GameScript::CreateCreatureDoor, 0},
	{"createcreatureatfeet", GameScript::CreateCreatureAtFeet, 0},
	{"createcreatureatlocation", GameScript::CreateCreatureAtLocation, 0},
	{"createcreatureimpassable", GameScript::CreateCreatureImpassable, 0},
	{"createcreatureimpassableallowoverlap", GameScript::CreateCreatureImpassableAllowOverlap, 0},
	{"createcreatureobject", GameScript::CreateCreatureObjectOffset, 0}, //the same
	{"createcreatureobjectcopy", GameScript::CreateCreatureObjectCopy, 0},
	{"createcreatureobjectcopyeffect", GameScript::CreateCreatureObjectCopy, 0}, //the same
	{"createcreatureobjectdoor", GameScript::CreateCreatureObjectDoor, 0},//same as createcreatureobject, but with dimension door animation
	{"createcreatureobjectoffscreen", GameScript::CreateCreatureObjectOffScreen, 0}, //same as createcreature object, but starts looking for a place far away from the player
	{"createcreatureobjectoffset", GameScript::CreateCreatureObjectOffset, 0}, //the same
	{"createcreatureoffscreen", GameScript::CreateCreatureOffScreen, 0},
	{"createitem", GameScript::CreateItem, 0},
	{"createitemglobal", GameScript::CreateItemNumGlobal, 0},
	{"createitemnumglobal", GameScript::CreateItemNumGlobal, 0},
	{"createpartygold", GameScript::CreatePartyGold, 0},
	{"createvisualeffect", GameScript::CreateVisualEffect, 0},
	{"createvisualeffectobject", GameScript::CreateVisualEffectObject, 0},
	{"createvisualeffectobjectSticky", GameScript::CreateVisualEffectObjectSticky, 0},
	{"cutsceneid", GameScript::CutSceneID,AF_INSTANT},
	{"damage", GameScript::Damage, 0},
	{"daynight", GameScript::DayNight, 0},
	{"deactivate", GameScript::Deactivate, 0},
	{"debug", GameScript::Debug, 0},
	{"debugoutput", GameScript::Debug, 0},
	{"deletejournalentry", GameScript::RemoveJournalEntry, 0},
	{"demoend", GameScript::QuitGame, 0}, //same for now
	{"destroyalldestructableequipment", GameScript::DestroyAllDestructableEquipment, 0},
	{"destroyallequipment", GameScript::DestroyAllEquipment, 0},
	{"destroygold", GameScript::DestroyGold, 0},
	{"destroyitem", GameScript::DestroyItem, 0},
	{"destroypartygold", GameScript::DestroyPartyGold, 0},
	{"destroypartyitem", GameScript::DestroyPartyItem, 0},
	{"destroyself", GameScript::DestroySelf, 0},
	{"detectsecretdoor", GameScript::DetectSecretDoor, 0},
	{"dialog", GameScript::Dialogue,AF_BLOCKING},
	{"dialogforceinterrupt", GameScript::DialogueForceInterrupt,AF_BLOCKING},
	{"dialoginterrupt", GameScript::DialogueInterrupt,0},
	{"dialogue", GameScript::Dialogue,AF_BLOCKING},
	{"dialogueforceinterrupt", GameScript::DialogueForceInterrupt,AF_BLOCKING},
	{"dialogueinterrupt", GameScript::DialogueInterrupt,0},
	{"disablefogdither", GameScript::DisableFogDither, 0},
	{"disablespritedither", GameScript::DisableSpriteDither, 0},
	{"displaymessage", GameScript::DisplayMessage, 0},
	{"displaystring", GameScript::DisplayString, 0},
	{"displaystringhead", GameScript::DisplayStringHead, 0},
	{"displaystringheadowner", GameScript::DisplayStringHeadOwner, 0},
	{"displaystringheaddead", GameScript::DisplayStringHead, 0}, //same?
	{"displaystringnoname", GameScript::DisplayStringNoName, 0},
	{"displaystringnonamehead", GameScript::DisplayStringNoNameHead, 0},
	{"displaystringwait", GameScript::DisplayStringWait,AF_BLOCKING},
	{"doubleclicklbuttonobject", GameScript::DoubleClickLButtonObject, AF_BLOCKING},
	{"doubleclicklbuttonpoint", GameScript::DoubleClickLButtonPoint, AF_BLOCKING},
	{"doubleclickrbuttonobject", GameScript::DoubleClickLButtonObject, AF_BLOCKING},
	{"doubleclickrbuttonpoint", GameScript::DoubleClickLButtonPoint, AF_BLOCKING},
	{"dropinventory", GameScript::DropInventory, 0},
	{"dropinventoryex", GameScript::DropInventoryEX, 0},
	{"dropinventoryexexclude", GameScript::DropInventoryEX, 0}, //same
	{"dropitem", GameScript::DropItem, AF_BLOCKING},
	{"enablefogdither", GameScript::EnableFogDither, 0},
	{"enableportaltravel", GameScript::EnablePortalTravel, 0},
	{"enablespritedither", GameScript::EnableSpriteDither, 0},
	{"endcredits", GameScript::EndCredits, 0},//movie
	{"endcutscenemode", GameScript::EndCutSceneMode, 0},
	{"endgame", GameScript::QuitGame, 0}, //ending in iwd2
	{"enemy", GameScript::Enemy, 0},
	{"equipitem", GameScript::EquipItem, 0},
	{"equipmostdamagingmelee",GameScript::EquipMostDamagingMelee,0},
	{"equipranged", GameScript::EquipRanged,0},
	{"equipweapon", GameScript::EquipWeapon,0},
	{"erasejournalentry", GameScript::RemoveJournalEntry, 0},
	{"escapearea", GameScript::EscapeArea, AF_BLOCKING},
	{"escapeareadestroy", GameScript::EscapeAreaDestroy, AF_BLOCKING},
	{"escapeareaobject", GameScript::EscapeAreaObject, AF_BLOCKING},
	{"escapeareaobjectnosee", GameScript::EscapeAreaObjectNoSee, AF_BLOCKING},
	{"exitpocketplane", GameScript::ExitPocketPlane, 0},
	{"expansionendcredits", GameScript::QuitGame, 0},//ends game too
	{"explore", GameScript::Explore, 0},
	{"exploremapchunk", GameScript::ExploreMapChunk, 0},
	{"face", GameScript::Face,AF_BLOCKING},
	{"faceobject", GameScript::FaceObject, AF_BLOCKING},
	{"facesavedlocation", GameScript::FaceSavedLocation, AF_BLOCKING},
	{"fadefromblack", GameScript::FadeFromColor, AF_BLOCKING}, //probably the same
	{"fadefromcolor", GameScript::FadeFromColor, AF_BLOCKING},
	{"fadetoandfromcolor", GameScript::FadeToAndFromColor, AF_BLOCKING},
	{"fadetoblack", GameScript::FadeToColor, AF_BLOCKING}, //probably the same
	{"fadetocolor", GameScript::FadeToColor, AF_BLOCKING},
	{"fakeeffectexpirycheck", GameScript::FakeEffectExpiryCheck, 0},
	{"finalsave", GameScript::SaveGame, 0}, //synonym
	{"findtraps", GameScript::FindTraps, 0},
	{"floatmessage", GameScript::DisplayStringHead, 0},
	{"floatmessagefixed", GameScript::FloatMessageFixed, 0},
	{"floatmessagefixedrnd", GameScript::FloatMessageFixedRnd, 0},
	{"floatmessagernd", GameScript::FloatMessageRnd, 0},
	{"floatrebus", GameScript::FloatRebus, 0},
	{"follow", GameScript::Follow, AF_ALIVE},
	{"followcreature", GameScript::FollowCreature, AF_BLOCKING|AF_ALIVE}, //pst
	{"followobjectformation", GameScript::FollowObjectFormation, AF_BLOCKING|AF_ALIVE},
	{"forceaiscript", GameScript::ForceAIScript, 0},
	{"forceattack", GameScript::ForceAttack, 0},
	{"forcefacing", GameScript::ForceFacing, 0},
	{"forcehide", GameScript::ForceHide, 0},
	{"forceleavearealua", GameScript::ForceLeaveAreaLUA, 0},
	{"forcespell", GameScript::ForceSpell, AF_BLOCKING},
	{"forcespellpoint", GameScript::ForceSpellPoint, AF_BLOCKING},
	{"forceusecontainer", GameScript::ForceUseContainer,AF_BLOCKING},
	{"formation", GameScript::Formation, AF_BLOCKING},
	{"fullheal", GameScript::FullHeal, 0},
	{"fullhealex", GameScript::FullHeal, 0}, //pst, not sure what's different
	{"generatepartymember", GameScript::GeneratePartyMember, 0},
	{"getitem", GameScript::GetItem, 0},
	{"getstat", GameScript::GetStat, 0}, //gemrb specific
	{"giveexperience", GameScript::AddXPObject, 0},
	{"givegoldforce", GameScript::CreatePartyGold, 0}, //this is the same
	{"giveitem", GameScript::GiveItem, 0},
	{"giveitemcreate", GameScript::CreateItem, 0}, //actually this is a targeted createitem
	{"giveorder", GameScript::GiveOrder, 0},
	{"givepartyallequipment", GameScript::GivePartyAllEquipment, 0},
	{"givepartygold", GameScript::GivePartyGold, 0},
	{"givepartygoldglobal", GameScript::GivePartyGoldGlobal,0},//no mergestrings!
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
	{"globalshout", GameScript::GlobalShout, 0},
	{"globalshr", GameScript::GlobalShR,AF_MERGESTRINGS},
	{"globalshrglobal", GameScript::GlobalShRGlobal,AF_MERGESTRINGS},
	{"globalsubglobal", GameScript::GlobalSubGlobal,AF_MERGESTRINGS},
	{"globalxor", GameScript::GlobalXor,AF_MERGESTRINGS},
	{"globalxorglobal", GameScript::GlobalXorGlobal,AF_MERGESTRINGS},
	{"gotostartscreen", GameScript::QuitGame, 0},//ending
	{"help", GameScript::Help, 0},
	{"hide", GameScript::Hide, 0},
	{"hideareaonmap", GameScript::HideAreaOnMap, 0},
	{"hidecreature", GameScript::HideCreature, 0},
	{"hidegui", GameScript::HideGUI, 0},
	{"incinternal", GameScript::IncInternal, 0}, //pst
	{"incrementinternal", GameScript::IncInternal, 0},//iwd
	{"incmoraleai", GameScript::IncMoraleAI, 0},
	{"incrementchapter", GameScript::IncrementChapter, AF_BLOCKING},
	{"incrementextraproficiency", GameScript::IncrementExtraProficiency, 0},
	{"incrementglobal", GameScript::IncrementGlobal,AF_MERGESTRINGS},
	{"incrementglobalonce", GameScript::IncrementGlobalOnce,AF_MERGESTRINGS},
	{"incrementkillstat", GameScript::IncrementKillStat, 0},
	{"incrementproficiency", GameScript::IncrementProficiency, 0},
	{"interact", GameScript::Interact, 0},
	{"joinparty", GameScript::JoinParty, 0}, //this action appears to be blocking in bg2
	{"journalentrydone", GameScript::SetQuestDone, 0},
	{"jumptoobject", GameScript::JumpToObject, 0},
	{"jumptopoint", GameScript::JumpToPoint, 0},
	{"jumptopointinstant", GameScript::JumpToPointInstant, 0},
	{"jumptosavedlocation", GameScript::JumpToSavedLocation, 0},
	{"kill", GameScript::Kill, 0},
	{"killfloatmessage", GameScript::KillFloatMessage, 0},
	{"leavearea", GameScript::LeaveAreaLUA, 0}, //so far the same
	{"leavearealua", GameScript::LeaveAreaLUA, 0},
	{"leavearealuaentry", GameScript::LeaveAreaLUAEntry,AF_BLOCKING},
	{"leavearealuapanic", GameScript::LeaveAreaLUAPanic, 0},
	{"leavearealuapanicentry", GameScript::LeaveAreaLUAPanicEntry,AF_BLOCKING},
	{"leaveparty", GameScript::LeaveParty, 0},
	{"lock", GameScript::Lock, 0},//key not checked at this time!
	{"lockscroll", GameScript::LockScroll, 0},
	{"log", GameScript::Debug, 0}, //the same until we know better
	{"makeglobal", GameScript::MakeGlobal, 0},
	{"makeunselectable", GameScript::MakeUnselectable, 0},
	{"markobject", GameScript::MarkObject, 0},
	{"moraledec", GameScript::MoraleDec, 0},
	{"moraleinc", GameScript::MoraleInc, 0},
	{"moraleset", GameScript::MoraleSet, 0},
	{"matchhp", GameScript::MatchHP, 0},
	{"movebetweenareas", GameScript::MoveBetweenAreas, 0},
	{"movebetweenareaseffect", GameScript::MoveBetweenAreas, 0},
	{"movecursorpoint", GameScript::MoveCursorPoint, 0},//immediate move
	{"moveglobal", GameScript::MoveGlobal, 0},
	{"moveglobalobject", GameScript::MoveGlobalObject, 0},
	{"moveglobalobjectoffscreen", GameScript::MoveGlobalObjectOffScreen, 0},
	{"moveglobalsto", GameScript::MoveGlobalsTo, 0},
	{"transferinventory", GameScript::MoveInventory, 0},
	{"movetocenterofscreen", GameScript::MoveToCenterOfScreen,AF_BLOCKING},
	{"movetoexpansion", GameScript::MoveToExpansion,AF_BLOCKING},
	{"movetoobject", GameScript::MoveToObject,AF_BLOCKING|AF_ALIVE},
	{"movetoobjectfollow", GameScript::MoveToObjectFollow,AF_BLOCKING|AF_ALIVE},
	{"movetoobjectnointerrupt", GameScript::MoveToObjectNoInterrupt,AF_BLOCKING|AF_ALIVE},
	{"movetoobjectuntilsee", GameScript::MoveToObjectUntilSee,AF_BLOCKING|AF_ALIVE},
	{"movetooffset", GameScript::MoveToOffset,AF_BLOCKING|AF_ALIVE},
	{"movetopoint", GameScript::MoveToPoint,AF_BLOCKING|AF_ALIVE},
	{"movetopointnointerrupt", GameScript::MoveToPointNoInterrupt,AF_BLOCKING|AF_ALIVE},
	{"movetopointnorecticle", GameScript::MoveToPointNoRecticle,AF_BLOCKING|AF_ALIVE},//the same until we know better
	{"movetosavedlocation", GameScript::MoveToSavedLocation,AF_MERGESTRINGS|AF_BLOCKING},
	//take care of the typo in the original bg2 action.ids
	//FIXME: why doesn't this have MERGESTRINGS like the above entry?
	{"movetosavedlocationn", GameScript::MoveToSavedLocation,AF_BLOCKING},
	{"moveviewobject", GameScript::MoveViewObject, AF_BLOCKING},
	{"moveviewpoint", GameScript::MoveViewPoint, AF_BLOCKING},
	{"moveviewpointuntildone", GameScript::MoveViewPoint, 0},
	{"nidspecial1", GameScript::NIDSpecial1,AF_BLOCKING|AF_DIRECT|AF_ALIVE},//we use this for dialogs, hack
	{"nidspecial2", GameScript::NIDSpecial2,AF_BLOCKING},//we use this for worldmap, another hack
	{"nidspecial3", GameScript::Attack,AF_BLOCKING|AF_DIRECT|AF_ALIVE},//this hack is for attacking preset target
	{"nidspecial4", GameScript::ProtectObject,AF_BLOCKING|AF_DIRECT|AF_ALIVE},
	{"nidspecial5", GameScript::UseItem, AF_BLOCKING|AF_DIRECT|AF_ALIVE},
	{"nidspecial6", GameScript::Spell, AF_BLOCKING|AF_DIRECT|AF_ALIVE},
	{"nidspecial7", GameScript::UseItemPoint, AF_BLOCKING|AF_ALIVE},
	{"nidspecial8", GameScript::SpellPoint, AF_BLOCKING|AF_ALIVE},
	{"nidspecial9", GameScript::ToggleDoor, AF_BLOCKING},//another internal hack, maybe we should use UseDoor instead
	{"noaction", GameScript::NoAction, 0},
	{"opendoor", GameScript::OpenDoor,0},
	{"panic", GameScript::Panic, AF_ALIVE},
	{"permanentstatchange", GameScript::PermanentStatChange, 0}, //pst
	{"pausegame", GameScript::PauseGame, AF_BLOCKING}, //this is almost surely blocking
	{"picklock", GameScript::PickLock,AF_BLOCKING},
	{"pickpockets", GameScript::PickPockets, AF_BLOCKING},
	{"pickupitem", GameScript::PickUpItem, 0},
	{"playdead", GameScript::PlayDead,AF_BLOCKING|AF_ALIVE},
	{"playdeadinterruptable", GameScript::PlayDeadInterruptable,AF_BLOCKING|AF_ALIVE},
	{"playerdialog", GameScript::PlayerDialogue,AF_BLOCKING},
	{"playerdialogue", GameScript::PlayerDialogue,AF_BLOCKING},
	{"playsequence", GameScript::PlaySequence, 0},
	{"playsequencetimed", GameScript::PlaySequenceTimed, 0},//pst
	{"playsong", GameScript::StartSong, 0},
	{"playsound", GameScript::PlaySound, 0},
	{"playsoundnotranged", GameScript::PlaySoundNotRanged, 0},
	{"playsoundpoint", GameScript::PlaySoundPoint, 0},
	{"plunder", GameScript::Plunder,AF_BLOCKING|AF_ALIVE},
	{"polymorph", GameScript::Polymorph, 0},
	{"polymorphcopy", GameScript::PolymorphCopy, 0},
	{"polymorphcopybase", GameScript::PolymorphCopyBase, 0},
	{"protectobject", GameScript::ProtectObject, 0},
	{"protectpoint", GameScript::ProtectPoint, AF_BLOCKING},
	{"quitgame", GameScript::QuitGame, 0},
	{"randomfly", GameScript::RandomFly, AF_BLOCKING|AF_ALIVE},
	{"randomrun", GameScript::RandomRun, AF_BLOCKING|AF_ALIVE},
	{"randomturn", GameScript::RandomTurn, AF_BLOCKING},
	{"randomwalk", GameScript::RandomWalk, AF_BLOCKING|AF_ALIVE},
	{"randomwalkcontinuous", GameScript::RandomWalkContinuous, AF_BLOCKING|AF_ALIVE},
	{"realsetglobaltimer", GameScript::RealSetGlobalTimer,AF_MERGESTRINGS},
	{"reallyforcespell", GameScript::ReallyForceSpell, AF_BLOCKING},
	{"reallyforcespelldead", GameScript::ReallyForceSpellDead, AF_BLOCKING},
	{"recoil", GameScript::Recoil, AF_ALIVE},
	{"regainpaladinhood", GameScript::RegainPaladinHood, 0},
	{"regainrangerhood", GameScript::RegainRangerHood, 0},
	{"removeareaflag", GameScript::RemoveAreaFlag, 0},
	{"removeareatype", GameScript::RemoveAreaType, 0},
	{"removejournalentry", GameScript::RemoveJournalEntry, 0},
	{"removemapnote", GameScript::RemoveMapnote, 0},
	{"removepaladinhood", GameScript::RemovePaladinHood, 0},
	{"removerangerhood", GameScript::RemoveRangerHood, 0},
	{"removespell", GameScript::RemoveSpell, 0},
	{"removetraps", GameScript::RemoveTraps, AF_BLOCKING},
	{"reputationinc", GameScript::ReputationInc, 0},
	{"reputationset", GameScript::ReputationSet, 0},
	{"resetfogofwar", GameScript::UndoExplore, 0}, //pst
	{"rest", GameScript::Rest, AF_ALIVE},
	{"restnospells", GameScript::RestNoSpells, 0},
	{"restorepartylocations", GameScript:: RestorePartyLocation, 0},
	{"restparty", GameScript::RestParty, 0},
	{"restuntilhealed", GameScript::RestUntilHealed, 0},
	//this is in iwd2, same as movetosavedlocation, but with stats
	{"returntosavedlocation", GameScript::ReturnToSavedLocation, AF_BLOCKING|AF_ALIVE},
	{"returntosavedlocationdelete", GameScript::ReturnToSavedLocationDelete, AF_BLOCKING|AF_ALIVE},
	{"returntosavedplace", GameScript::ReturnToSavedLocation, AF_BLOCKING|AF_ALIVE},
	{"revealareaonmap", GameScript::RevealAreaOnMap, 0},
	{"runawayfrom", GameScript::RunAwayFrom,AF_BLOCKING|AF_ALIVE},
	{"runawayfromnointerrupt", GameScript::RunAwayFromNoInterrupt,AF_BLOCKING|AF_ALIVE},
	{"runawayfromnoleavearea", GameScript::RunAwayFromNoLeaveArea,AF_BLOCKING|AF_ALIVE},
	{"runawayfrompoint", GameScript::RunAwayFromPoint,AF_BLOCKING|AF_ALIVE},
	{"runfollow", GameScript::RunAwayFrom,AF_BLOCKING|AF_ALIVE},
	{"runningattack", GameScript::RunningAttack,AF_BLOCKING|AF_ALIVE},
	{"runningattacknosound", GameScript::RunningAttackNoSound,AF_BLOCKING|AF_ALIVE},
	{"runtoobject", GameScript::RunToObject,AF_BLOCKING|AF_ALIVE},
	{"runtopoint", GameScript::RunToPoint,AF_BLOCKING},
	{"runtopointnorecticle", GameScript::RunToPointNoRecticle,AF_BLOCKING|AF_ALIVE},
	{"runtosavedlocation", GameScript::RunToSavedLocation,AF_BLOCKING|AF_ALIVE},
	{"savegame", GameScript::SaveGame, 0},
	{"savelocation", GameScript::SaveLocation, 0},
	{"saveplace", GameScript::SaveLocation, 0},
	{"saveobjectlocation", GameScript::SaveObjectLocation, 0},
	{"screenshake", GameScript::ScreenShake,AF_BLOCKING},
	{"selectweaponability", GameScript::SelectWeaponAbility, 0},
	{"sendtrigger", GameScript::SendTrigger, 0},
	{"setanimstate", GameScript::PlaySequence, AF_ALIVE},//pst
	{"setapparentnamestrref", GameScript::SetApparentName, 0},
	{"setareaflags", GameScript::SetAreaFlags, 0},
	{"setarearestflag", GameScript::SetAreaRestFlag, 0},
	{"setbeeninpartyflags", GameScript::SetBeenInPartyFlags, 0},
	{"setbestweapon", GameScript::SetBestWeapon, 0},
	{"setcorpseenabled", GameScript::AmbientActivate, 0},//another weird name
	{"setcreatureareaflag", GameScript::SetCreatureAreaFlag, 0},
	{"setcriticalpathobject", GameScript::SetCriticalPathObject, 0},
	{"setdialog", GameScript::SetDialogue,0},
	{"setdialogrange", GameScript::SetDialogueRange, 0},
	{"setdialogue", GameScript::SetDialogue,0},
	{"setdialoguerange", GameScript::SetDialogueRange, 0},
	{"setdoorflag", GameScript::SetDoorFlag,0},
	{"setdoorlocked", GameScript::SetDoorLocked,0},
	{"setencounterprobability", GameScript::SetEncounterProbability,0},
	{"setextendednight", GameScript::SetExtendedNight, 0},
	{"setfaction", GameScript::SetFaction, 0},
	{"setgabber", GameScript::SetGabber, 0},
	{"setglobal", GameScript::SetGlobal,AF_MERGESTRINGS},
	{"setglobalrandom", GameScript::SetGlobalRandom, AF_MERGESTRINGS},
	{"setglobaltimer", GameScript::SetGlobalTimer,AF_MERGESTRINGS},
	{"setglobaltimeronce", GameScript::SetGlobalTimerOnce,AF_MERGESTRINGS},
	{"setglobaltimerrandom", GameScript::SetGlobalTimerRandom,AF_MERGESTRINGS},
	{"setglobaltint", GameScript::SetGlobalTint, 0},
	{"sethomelocation", GameScript::SetSavedLocation, 0}, //bg2
	{"sethp", GameScript::SetHP, 0},
	{"sethppercent", GameScript::SetHPPercent, 0},
	{"setinternal", GameScript::SetInternal, 0},
	{"setinterrupt", GameScript::SetInterrupt, 0},
	{"setleavepartydialogfile", GameScript::SetLeavePartyDialogFile, 0},
	{"setleavepartydialoguefile", GameScript::SetLeavePartyDialogFile, 0},
	{"setmasterarea", GameScript::SetMasterArea, 0},
	{"setmazeeasier", GameScript::SetMazeEasier, 0}, //pst specific crap
	{"setmazeharder", GameScript::SetMazeHarder, 0}, //pst specific crap
	{"setmoraleai", GameScript::SetMoraleAI, 0},
	{"setmusic", GameScript::SetMusic, 0},
	{"setname", GameScript::SetApparentName, 0},
	{"setnamelessclass", GameScript::SetNamelessClass, 0},
	{"setnamelessdisguise", GameScript::SetNamelessDisguise, 0},
	{"setnooneontrigger", GameScript::SetNoOneOnTrigger, 0},
	{"setnumtimestalkedto", GameScript::SetNumTimesTalkedTo, 0},
	{"setplayersound", GameScript::SetPlayerSound, 0},
	{"setquestdone", GameScript::SetQuestDone, 0},
	{"setregularnamestrref", GameScript::SetRegularName, 0},
	{"setrestencounterchance", GameScript::SetRestEncounterChance, 0},
	{"setrestencounterprobabilityday", GameScript::SetRestEncounterProbabilityDay, 0},
	{"setrestencounterprobabilitynight", GameScript::SetRestEncounterProbabilityNight, 0},
	{"setsavedlocation", GameScript::SetSavedLocation, 0},
	{"setsavedlocationpoint", GameScript::SetSavedLocationPoint, 0},
	{"setscriptname", GameScript::SetScriptName, 0},
	{"setselection", GameScript::SetSelection, 0},
	{"setsequence", GameScript::PlaySequence, 0}, //bg2 (only own)
	{"setstartpos", GameScript::SetStartPos, 0},
	{"setteam", GameScript::SetTeam, 0},
	{"setteambit", GameScript::SetTeamBit, 0},
	{"settextcolor", GameScript::SetTextColor, 0},
	{"settrackstring", GameScript::SetTrackString, 0},
	{"settoken", GameScript::SetToken, 0},
	{"settoken2da", GameScript::SetToken2DA, 0}, //GemRB specific
	{"settokenglobal", GameScript::SetTokenGlobal,AF_MERGESTRINGS},
	{"settokenobject", GameScript::SetTokenObject,0},
	{"setupwish", GameScript::SetupWish, 0},
        {"setupwishobject", GameScript::SetupWishObject, 0},
	{"setvisualrange", GameScript::SetVisualRange, 0},
	{"sg", GameScript::SG, 0},
	{"shout", GameScript::Shout, 0},
	{"sinisterpoof", GameScript::CreateVisualEffect, 0},
	{"smallwait", GameScript::SmallWait,AF_BLOCKING},
	{"smallwaitrandom", GameScript::SmallWaitRandom,AF_BLOCKING},
	{"soundactivate", GameScript::SoundActivate, 0},
	{"spawnptactivate", GameScript::SpawnPtActivate, 0},
	{"spawnptdeactivate", GameScript::SpawnPtDeactivate, 0},
	{"spawnptspawn", GameScript::SpawnPtSpawn, 0},
	{"spell", GameScript::Spell, AF_BLOCKING|AF_ALIVE},
	{"spellhiteffectpoint", GameScript::SpellHitEffectPoint, 0},
	{"spellhiteffectsprite", GameScript::SpellHitEffectSprite, 0},
	{"spellnodec", GameScript::SpellNoDec, AF_BLOCKING|AF_ALIVE},
	{"spellpoint", GameScript::SpellPoint, AF_BLOCKING|AF_ALIVE},
	{"spellpointnodec", GameScript::SpellPointNoDec, AF_BLOCKING|AF_ALIVE},
	{"startcutscene", GameScript::StartCutScene, 0},
	{"startcutsceneex", GameScript::StartCutScene, 0}, //pst (unknown)
	{"startcutscenemode", GameScript::StartCutSceneMode, 0},
	{"startdialog", GameScript::StartDialogue,AF_BLOCKING},
	{"startdialoginterrupt", GameScript::StartDialogueInterrupt,AF_BLOCKING},
	{"startdialogue", GameScript::StartDialogue,AF_BLOCKING},
	{"startdialogueinterrupt", GameScript::StartDialogueInterrupt,AF_BLOCKING},
	{"startdialognoname", GameScript::StartDialogue,AF_BLOCKING},
	{"startdialognoset", GameScript::StartDialogueNoSet,AF_BLOCKING},
	{"startdialognosetinterrupt", GameScript::StartDialogueNoSetInterrupt,AF_BLOCKING},
	{"startdialogoverride", GameScript::StartDialogueOverride,AF_BLOCKING},
	{"startdialogoverrideinterrupt", GameScript::StartDialogueOverrideInterrupt,AF_BLOCKING},
	{"startdialoguenoname", GameScript::StartDialogue,AF_BLOCKING},
	{"startdialoguenoset", GameScript::StartDialogueNoSet,AF_BLOCKING},
	{"startdialoguenosetinterrupt", GameScript::StartDialogueNoSetInterrupt,AF_BLOCKING},
	{"startdialogueoverride", GameScript::StartDialogueOverride,AF_BLOCKING},
	{"startdialogueoverrideinterrupt", GameScript::StartDialogueOverrideInterrupt,AF_BLOCKING},
	{"startmovie", GameScript::StartMovie,AF_BLOCKING},
	{"startmusic", GameScript::StartMusic, 0},
	{"startrainnow", GameScript::StartRainNow, 0},
	{"startrandomtimer", GameScript::StartRandomTimer, 0},
	{"startsong", GameScript::StartSong, 0},
	{"startstore", GameScript::StartStore, 0},
	{"starttimer", GameScript::StartTimer, 0},
	{"stateoverrideflag", GameScript::StateOverrideFlag, 0},
	{"stateoverridetime", GameScript::StateOverrideTime, 0},
	{"staticpalette", GameScript::StaticPalette, 0},
	{"staticsequence", GameScript::PlaySequence, 0},//bg2 animation sequence
	{"staticstart", GameScript::StaticStart, 0},
	{"staticstop", GameScript::StaticStop, 0},
	{"stickysinisterpoof", GameScript::CreateVisualEffectObjectSticky, 0},
	{"stopmoving", GameScript::StopMoving, 0},
	{"storepartylocations", GameScript::StorePartyLocation, 0},
	{"swing", GameScript::Swing, AF_ALIVE},
	{"swingonce", GameScript::SwingOnce, AF_ALIVE},
	{"takeitemlist", GameScript::TakeItemList, 0},
	{"takeitemlistparty", GameScript::TakeItemListParty, 0},
	{"takeitemlistpartynum", GameScript::TakeItemListPartyNum, 0},
	{"takeitemreplace", GameScript::TakeItemReplace, 0},
	{"takepartygold", GameScript::TakePartyGold, 0},
	{"takepartyitem", GameScript::TakePartyItem, 0},
	{"takepartyitemall", GameScript::TakePartyItemAll, 0},
	{"takepartyitemnum", GameScript::TakePartyItemNum, 0},
	{"takepartyitemrange", GameScript::TakePartyItemRange, 0},
	{"teleportparty", GameScript::TeleportParty, 0},
	{"textscreen", GameScript::TextScreen, AF_BLOCKING},
	{"timedmovetopoint", GameScript::TimedMoveToPoint,AF_BLOCKING|AF_ALIVE},
	{"tomsstringdisplayer", GameScript::DisplayMessage, 0},
	{"transformitem", GameScript::TransformItem, 0},
	{"transformitemall", GameScript::TransformItemAll, 0},
	{"transformpartyitem", GameScript::TransformPartyItem, 0},
	{"transformpartyitemall", GameScript::TransformPartyItemAll, 0},
	{"triggeractivation", GameScript::TriggerActivation, 0},
	{"triggerwalkto", GameScript::MoveToObject,AF_BLOCKING|AF_ALIVE}, //something like this
	{"turn", GameScript::Turn, 0},
	{"turnamt", GameScript::TurnAMT, AF_BLOCKING}, //relative Face()
	{"undoexplore", GameScript::UndoExplore, 0},
	{"unhidegui", GameScript::UnhideGUI, 0},
	{"unloadarea", GameScript::UnloadArea, 0},
	{"unlock", GameScript::Unlock, 0},
	{"unlockscroll", GameScript::UnlockScroll, 0},
	{"unmakeglobal", GameScript::UnMakeGlobal, 0}, //this is a GemRB extension
	{"usecontainer", GameScript::UseContainer,AF_BLOCKING},
	{"usedoor", GameScript::UseDoor,AF_BLOCKING},
	{"useitem", GameScript::UseItem,AF_BLOCKING},
	{"useitempoint", GameScript::UseItemPoint,AF_BLOCKING},
	{"useitempointslot", GameScript::UseItemPoint,AF_BLOCKING},
	{"useitemslot", GameScript::UseItem,AF_BLOCKING},
	{"vequip",GameScript::SetArmourLevel, 0},
	{"verbalconstant", GameScript::VerbalConstant, 0},
	{"verbalconstanthead", GameScript::VerbalConstantHead, 0},
	{"wait", GameScript::Wait, AF_BLOCKING},
	{"waitanimation", GameScript::WaitAnimation,AF_BLOCKING},//iwd2
	{"waitrandom", GameScript::WaitRandom, AF_BLOCKING},
	{"weather", GameScript::Weather, 0},
	{"xequipitem", GameScript::XEquipItem, 0},
	{ NULL,NULL, 0}
};

//Make this an ordered list, so we could use bsearch!
static const ObjectLink objectnames[] = {
	{"bestac", GameScript::BestAC},
	{"eighthnearest", GameScript::EighthNearest},
	{"eighthnearestdoor", GameScript::EighthNearestDoor},
	{"eighthnearestenemyof", GameScript::EighthNearestEnemyOf},
	{"eighthnearestenemyoftype", GameScript::EighthNearestEnemyOfType},
	{"eighthnearestmygroupoftype", GameScript::EighthNearestEnemyOfType},
	{"eigthnearestenemyof", GameScript::EighthNearestEnemyOf}, //typo in iwd
	{"eigthnearestenemyoftype", GameScript::EighthNearestEnemyOfType}, //bg2
	{"eigthnearestmygroupoftype", GameScript::EighthNearestEnemyOfType},//bg2
	{"farthest", GameScript::Farthest},
	{"farthestenemyof", GameScript::FarthestEnemyOf},
	{"fifthnearest", GameScript::FifthNearest},
	{"fifthnearestdoor", GameScript::FifthNearestDoor},
	{"fifthnearestenemyof", GameScript::FifthNearestEnemyOf},
	{"fifthnearestenemyoftype", GameScript::FifthNearestEnemyOfType},
	{"fifthnearestmygroupoftype", GameScript::FifthNearestEnemyOfType},
	{"fourthnearest", GameScript::FourthNearest},
	{"fourthnearestdoor", GameScript::FourthNearestDoor},
	{"fourthnearestenemyof", GameScript::FourthNearestEnemyOf},
	{"fourthnearestenemyoftype", GameScript::FourthNearestEnemyOfType},
	{"fourthnearestmygroupoftype", GameScript::FourthNearestEnemyOfType},
	{"gabber", GameScript::Gabber},
	{"groupof", GameScript::GroupOf},
	{"lastattackerof", GameScript::LastAttackerOf},
	{"lastcommandedby", GameScript::LastCommandedBy},
	{"lastheardby", GameScript::LastHeardBy},
	{"lasthelp", GameScript::LastHelp},
	{"lasthitter", GameScript::LastHitter},
	{"lastmarkedobject", GameScript::LastMarkedObject},
	{"lastseenby", GameScript::LastSeenBy},
	{"lastsummonerof", GameScript::LastSummonerOf},
	{"lasttalkedtoby", GameScript::LastTalkedToBy},
	{"lasttargetedby", GameScript::LastTargetedBy},
	{"lasttrigger", GameScript::LastTrigger},
	{"leaderof", GameScript::LeaderOf},
	{"leastdamagedof", GameScript::LeastDamagedOf},
	{"marked", GameScript::LastMarkedObject}, //pst
	{"mostdamagedof", GameScript::MostDamagedOf},
	{"myself", GameScript::Myself},
	{"mytarget", GameScript::MyTarget},//see lasttargetedby(myself)
	{"nearest", GameScript::Nearest}, //actually this seems broken in IE and resolve as Myself
	{"nearestdoor", GameScript::NearestDoor},
	{"nearestenemyof", GameScript::NearestEnemyOf},
	{"nearestenemyoftype", GameScript::NearestEnemyOfType},
	{"nearestenemysummoned", GameScript::NearestEnemySummoned},
	{"nearestmygroupoftype", GameScript::NearestMyGroupOfType},
	{"nearestpc", GameScript::NearestPC},
	{"ninthnearest", GameScript::NinthNearest},
	{"ninthnearestdoor", GameScript::NinthNearestDoor},
	{"ninthnearestenemyof", GameScript::NinthNearestEnemyOf},
	{"ninthnearestenemyoftype", GameScript::NinthNearestEnemyOfType},
	{"ninthnearestmygroupoftype", GameScript::NinthNearestMyGroupOfType},
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
	{"protectedby", GameScript::ProtectedBy},
	{"protectorof", GameScript::ProtectorOf},
	{"protagonist", GameScript::Protagonist},
	{"secondnearest", GameScript::SecondNearest},
	{"secondnearestdoor", GameScript::SecondNearestDoor},
	{"secondnearestenemyof", GameScript::SecondNearestEnemyOf},
	{"secondnearestenemyoftype", GameScript::SecondNearestEnemyOfType},
	{"secondnearestmygroupoftype", GameScript::SecondNearestMyGroupOfType},
	{"selectedcharacter", GameScript::SelectedCharacter},
	{"seventhnearest", GameScript::SeventhNearest},
	{"seventhnearestdoor", GameScript::SeventhNearestDoor},
	{"seventhnearestenemyof", GameScript::SeventhNearestEnemyOf},
	{"seventhnearestenemyoftype", GameScript::SeventhNearestEnemyOfType},
	{"seventhnearestmygroupoftype", GameScript::SeventhNearestMyGroupOfType},
	{"sixthnearest", GameScript::SixthNearest},
	{"sixthnearestdoor", GameScript::SixthNearestDoor},
	{"sixthnearestenemyof", GameScript::SixthNearestEnemyOf},
	{"sixthnearestenemyoftype", GameScript::SixthNearestEnemyOfType},
	{"sixthnearestmygroupoftype", GameScript::SixthNearestMyGroupOfType},
	{"strongestof", GameScript::StrongestOf},
	{"strongestofmale", GameScript::StrongestOfMale},
	{"tenthnearest", GameScript::TenthNearest},
	{"tenthnearestdoor", GameScript::TenthNearestDoor},
	{"tenthnearestenemyof", GameScript::TenthNearestEnemyOf},
	{"tenthnearestenemyoftype", GameScript::TenthNearestEnemyOfType},
	{"tenthnearestmygroupoftype", GameScript::TenthNearestMyGroupOfType},
	{"thirdnearest", GameScript::ThirdNearest},
	{"thirdnearestdoor", GameScript::ThirdNearestDoor},
	{"thirdnearestenemyof", GameScript::ThirdNearestEnemyOf},
	{"thirdnearestenemyoftype", GameScript::ThirdNearestEnemyOfType},
	{"thirdnearestmygroupoftype", GameScript::ThirdNearestMyGroupOfType},
	{"weakestof", GameScript::WeakestOf},
	{"worstac", GameScript::WorstAC},
	{ NULL,NULL}
};

static const IDSLink idsnames[] = {
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
	{ NULL,NULL}
};

static const TriggerLink* FindTrigger(const char* triggername, int index)
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
	printf( "Warning: Couldn't assign trigger: 0x%x %.*s\n", index, len, triggername );
	return NULL;
}

static const ActionLink* FindAction(const char* actionname, int index)
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
	printf( "Warning: Couldn't assign action: %d %.*s\n", index, len, actionname );
	return NULL;
}

static const ObjectLink* FindObject(const char* objectname, int index)
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
	printf( "Warning: Couldn't assign object: %d %.*s\n", index, len, objectname );
	return NULL;
}

static const IDSLink* FindIdentifier(const char* idsname)
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

const targettype *Targets::GetLastTarget(int Type)
{
	targetlist::const_iterator m = objects.end();
	while (m--!=objects.begin() ) {
		if ( (Type==-1) || ((*m).actor->Type==Type) ) {
			return &(*(m));
		}
	}
	return NULL;
}

const targettype *Targets::GetFirstTarget(targetlist::iterator &m, int Type)
{
	m=objects.begin();
	while (m!=objects.end() ) {
		if ( (Type!=-1) && ( (*m).actor->Type!=Type)) {
			m++;
			continue;
		}
		return &(*m);
	}
	return NULL;
}

const targettype *Targets::GetNextTarget(targetlist::iterator &m, int Type)
{
	m++;
	while (m!=objects.end() ) {
		if ( (Type!=-1) && ( (*m).actor->Type!=Type)) {
			m++;
			continue;
		}
		return &(*m);
	}
	return NULL;
}

Scriptable *Targets::GetTarget(unsigned int index, int Type)
{
	targetlist::iterator m = objects.begin();
	while(m!=objects.end() ) {
		if ( (Type==-1) || ((*m).actor->Type==Type)) {
			if (!index) {
				return (*m).actor;
			}
			index--;
		}
		m++;
	}
	return NULL;
}

//this stuff should be refined, dead actors are sometimes targetable by script?
void Targets::AddTarget(Scriptable* target, unsigned int distance, int ga_flags)
{
	if (!target) {
		return;
	}

	switch (target->Type) {
	case ST_ACTOR:
		//i don't know if unselectable actors are targetable by script
		//if yes, then remove GA_SELECT
		if (ga_flags) {
			if (!((Actor *) target)->ValidTarget(ga_flags) ) {
				return;
			}
		}
		break;
	case ST_GLOBAL:
		// this doesn't seem a good idea to allow
		return;
	default:
		break;
	}
	targettype Target = {target, distance};
	targetlist::iterator m;
	for (m = objects.begin(); m != objects.end(); ++m) {
		if ( (*m).distance>distance) {
			objects.insert( m, Target);
			return;
		}
	}
	objects.push_back( Target );
}

void Targets::Clear()
{
	objects.clear();
}

/********************** GameScript *******************************/
GameScript::GameScript(const ieResRef ResRef, ScriptableType ScriptType,
	Variables* local, int ScriptLevel, bool AIScript)
{
	if (local) {
		locals = local;
		freeLocals = false;
	} else {
		locals = new Variables();
		locals->SetType( GEM_VARIABLES_INT );
		freeLocals = true;
	}
	scriptlevel = ScriptLevel;
	lastAction = (unsigned int) ~0;

	if (!initialized) {
		initialized = 1;
		ieDword tmp = 0;

		//display the verbal constants in the console
		core->GetDictionary()->Lookup("Subtitles", tmp);
		if (tmp) {
			charactersubtitles = true;
		} else {
			charactersubtitles = false;
		}

		if (core->HasFeature(GF_CHARNAMEISGABBER)) {
			charnameisgabber=true;
		} else {
			charnameisgabber=false;
		}

		InitScriptTables();
		int tT = core->LoadSymbol( "trigger" );
		int aT = core->LoadSymbol( "action" );
		int oT = core->LoadSymbol( "object" );
		int gaT = core->LoadSymbol( "gemact" );
		AutoTable objNameTable("script");
		if (tT < 0 || aT < 0 || oT < 0 || !objNameTable) {
			printMessage( "GameScript","A critical scripting file is missing!\n",LIGHT_RED );
			abort();
		}
		triggersTable = core->GetSymbol( tT );
		actionsTable = core->GetSymbol( aT );
		objectsTable = core->GetSymbol( oT );
		SymbolMgr *overrideActionsTable = core->GetSymbol( gaT );
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
			const char *idsname;
			idsname=objNameTable->QueryField( 0, i + 1 );
			const IDSLink *poi=FindIdentifier( idsname );
			if (poi==NULL) {
				idtargets[i]=NULL;
			}
			else {
				idtargets[i]=poi->Function;
			}
			strnlwrcpy(ObjectIDSTableNames[i], idsname, 8 );
		}
		MaxObjectNesting = atoi( objNameTable->QueryField( 1 ) );
		if (MaxObjectNesting<0 || MaxObjectNesting>MAX_NESTING) {
			printMessage("GameScript","The Object Nesting Count shouldn't be more than 5!\n", LIGHT_RED);
			abort();
		}
		HasAdditionalRect = ( atoi( objNameTable->QueryField( 2 ) ) != 0 );
		ExtraParametersCount = atoi( objNameTable->QueryField( 3 ) );
		HasTriggerPoint = ( atoi( objNameTable->QueryField( 4 ) ) != 0 );
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
			const TriggerLink* poi = FindTrigger(triggersTable->GetStringIndex( j ), i );
			//bool triggerflag = i & 0x4000;
			i &= 0x3fff;
			if (i >= MAX_TRIGGERS) {
				printMessage("GameScript"," ", RED);
				printf("trigger %d (%s) is too high, ignoring\n", i, triggersTable->GetStringIndex( j ) );
				continue;
			}
			if (triggers[i]) {
				if (poi && triggers[i]!=poi->Function) {
					printMessage("GameScript"," ", YELLOW);
					printf("%s is in collision with %s\n", triggersTable->GetStringIndex( j ), triggersTable->GetStringIndex(triggersTable->FindValue(triggersTable->GetValueIndex( j )) ));
				} else {
					printMessage("GameScript"," ", WHITE);
					printf("%s is a synonym of %s\n", triggersTable->GetStringIndex( j ), triggersTable->GetStringIndex(triggersTable->FindValue(triggersTable->GetValueIndex( j )) ) );
				}
				continue; //we already found an alternative
			}
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
			if (i >= MAX_ACTIONS) {
				printMessage("GameScript"," ", RED);
				printf("action %d (%s) is too high, ignoring\n", i, actionsTable->GetStringIndex( j ) );
				continue;
			}
			const ActionLink* poi = FindAction( actionsTable->GetStringIndex( j ), i );
			if (actions[i]) {
				if (poi && actions[i]!=poi->Function) {
					printMessage("GameScript"," ", YELLOW);
					printf("%s is in collision with %s\n", actionsTable->GetStringIndex( j ), actionsTable->GetStringIndex(actionsTable->FindValue(actionsTable->GetValueIndex( j )) ) );
				} else {
					printMessage("GameScript"," ", WHITE);
					printf("%s is a synonym of %s\n", actionsTable->GetStringIndex( j ), actionsTable->GetStringIndex(actionsTable->FindValue(actionsTable->GetValueIndex( j )) ) );
				}
				continue; //we already found an alternative
			}
			if (poi == NULL) {
				actions[i] = NULL;
				actionflags[i] = 0;
				continue;
			}
			actions[i] = poi->Function;
			actionflags[i] = poi->Flags;
		}

		if (overrideActionsTable) {
			/*
			 * we add/replace some actions from gemact.ids
			 * right now you can't print or generate these actions!
			 */
			j = overrideActionsTable->GetSize();
			while (j--) {
				i = overrideActionsTable->GetValueIndex( j );
				if (i >= MAX_ACTIONS) {
					printMessage("GameScript"," ", RED);
					printf("action %d (%s) is too high, ignoring\n", i, overrideActionsTable->GetStringIndex( j ) );
					continue;
				}
				const ActionLink *poi = FindAction( overrideActionsTable->GetStringIndex( j ), i );
				if (!poi) {
					continue;
				}
				if (actions[i]) {
					printMessage("GameScript"," ", WHITE);
					printf("%s overrides existing action %s\n", overrideActionsTable->GetStringIndex( j ), actionsTable->GetStringIndex(actionsTable->FindValue(overrideActionsTable->GetValueIndex( j )) ) );
				}
				actions[i] = poi->Function;
				actionflags[i] = poi->Flags;
			}
		}

		j = objectsTable->GetSize();
		while (j--) {
			i = objectsTable->GetValueIndex( j );
			if (i >= MAX_OBJECTS) {
				printMessage("GameScript"," ", RED);
				printf("object %d (%s) is too high, ignoring\n", i, objectsTable->GetStringIndex( j ) );
				continue;
			}
			const ObjectLink* poi = FindObject( objectsTable->GetStringIndex( j ), i );
			if (objects[i]) {
				if (poi && objects[i]!=poi->Function) {
					printMessage("GameScript"," ", YELLOW);
					printf("%s is in collision with %s\n", objectsTable->GetStringIndex( j ), objectsTable->GetStringIndex(objectsTable->FindValue(objectsTable->GetValueIndex( j )) ) );
				} else {
					printMessage("GameScript"," ", WHITE);
					printf("%s is a synonym of %s\n", objectsTable->GetStringIndex( j ), objectsTable->GetStringIndex(objectsTable->FindValue(objectsTable->GetValueIndex( j )) ) );
				}
				continue;
			}
			if (poi == NULL) {
				objects[i] = NULL;
			} else {
				objects[i] = poi->Function;
			}
		}
		initialized = 2;
	}
	strnlwrcpy( Name, ResRef, 8 );

	script = CacheScript( Name, AIScript?IE_BS_CLASS_ID:IE_BCS_CLASS_ID);
	MySelf = NULL;
	scriptType = ScriptType;
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
		if (InDebug&ID_REFERENCE) {
			printf("One instance of %s is dropped from %d.\n", Name, BcsCache.RefCount(Name) );
		}
		int res = BcsCache.DecRef(script, Name, true);

		if (res<0) {
			printMessage( "GameScript", "Corrupted Script cache encountered (reference count went below zero), ", LIGHT_RED );
			printf( "Script name is: %.8s\n", Name);
			abort();
		}
		if (!res) {
			//printf("Freeing script %s because its refcount has reached 0.\n", Name);
			script->Release();
		}
		script = NULL;
	}
}

/** releasing global memory */
void GameScript::ReleaseMemory()
{
	if (SkillStats)
		free(SkillStats);
	SkillStats = NULL;
	SkillCount = -1;
	if (ObjectIDSTableNames)
		free(ObjectIDSTableNames);
	ObjectIDSTableNames = NULL;
	initialized = 0;
}

Script* GameScript::CacheScript(ieResRef ResRef, SClass_ID type)
{
	char line[10];

	Script *newScript = (Script *) BcsCache.GetResource(ResRef);
	if ( newScript ) {
		if (InDebug&ID_REFERENCE) {
			printf("Caching %s for the %d. time\n", ResRef, BcsCache.RefCount(ResRef) );
		}
		return newScript;
	}

	DataStream* stream = core->GetResourceMgr()->GetResource( ResRef, type );
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
	if (InDebug&ID_REFERENCE) {
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

static int ParseInt(const char*& src)
{
	char number[33];

	char* tmp = number;
	while (isdigit(*src) || *src=='-') {
		*tmp = *src;
		tmp++;
		src++;
	}
	*tmp = 0;
	if (*src)
		src++;
	return atoi( number );
}

static void ParseString(const char*& src, char* tmp)
{
	while (*src != '"' && *src) {
		*tmp = *src;
		tmp++;
		src++;
	}
	*tmp = 0;
	if (*src)
		src++;
}

static Object* DecodeObject(const char* line)
{
	int i;

	Object* oB = new Object();
	for (i = 0; i < ObjectFieldsCount; i++) {
		oB->objectFields[i] = ParseInt( line );
	}
	for (i = 0; i < MaxObjectNesting; i++) {
		oB->objectFilters[i] = ParseInt( line );
	}
	//iwd tolerates the missing rectangle, so we do so too
	if (HasAdditionalRect && (*line=='[') ) {
		line++; //Skip [
		for (i = 0; i < 4; i++) {
			oB->objectRect[i] = ParseInt( line );
		}
		if (*line)
			line++; //Skip ] (not really... it skips a ' ' since the ] was skipped by the ParseInt function
	}
	if (*line)
		line++; //Skip "
	ParseString( line, oB->objectName );
	if (*line)
		line++; //Skip " (the same as above)
	//this seems to be needed too
	if (ExtraParametersCount && *line) {
		line++;
	}
	for (i = 0; i < ExtraParametersCount; i++) {
		oB->objectFields[i + ObjectFieldsCount] = ParseInt( line );
	}
	//let the object realize it has no future (in case of null objects)
	if (oB->ReadyToDie()) {
		oB = NULL;
	}
	return oB;
}

static Trigger* ReadTrigger(DataStream* stream)
{
	char* line = ( char* ) malloc( 1024 );
	stream->ReadLine( line, 1024 );
	if (strncmp( line, "TR", 2 ) != 0) {
		free( line );
		return NULL;
	}
	stream->ReadLine( line, 1024 );
	Trigger* tR = new Trigger();
	//this exists only in PST?
	if (HasTriggerPoint) {
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
	strlwr(tR->string0Parameter);
	strlwr(tR->string1Parameter);
	tR->triggerID &= 0x3fff;
	stream->ReadLine( line, 1024 );
	tR->objectParameter = DecodeObject( line );
	stream->ReadLine( line, 1024 );
	free( line );
	return tR;
}

static Condition* ReadCondition(DataStream* stream)
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

//call this whenever a script was triggered by an event
//(death, trap entered)
/*
void GameScript::RunNow()
{
	lastRunTime = 0;
}
*/

/*
 * if you pass non-NULL parameters, continuing is set to whether we Continue()ed
 * (should start false and be passed to next script's Update),
 * and done is set to whether we processed a block without Continue()
 */
bool GameScript::Update(bool *continuing, bool *done)
{
	if (!MySelf)
		return false;

	if (!script)
		return false;

	//ieDword thisTime = core->GetGame()->Ticks;
	//if (( thisTime - lastRunTime ) < scriptRunDelay) {
	//	return false;
	//}

	//lastRunTime = thisTime;

	if(!(MySelf->GetInternalFlag()&IF_ACTIVE) ) {
		return true;
	}

	bool continueExecution = false;
	if (continuing) continueExecution = *continuing;

	RandomNumValue=rand();
	for (unsigned int a = 0; a < script->responseBlocksCount; a++) {
		ResponseBlock* rB = script->responseBlocks[a];
		if (EvaluateCondition( MySelf, rB->condition )) {
			//if this isn't a continue-d block, we have to clear the queue
			//we cannot clear the queue and cannot execute the new block
			//if we already have stuff on the queue!
			if (!continueExecution) {
				if (MySelf->GetCurrentAction() || MySelf->GetNextAction()) {
					if (MySelf->GetInternalFlag()&IF_NOINT) {
						// we presumably don't want any further execution?
						if (done) *done = true;
						return true;
					}

					if (lastAction==a) {
						// we presumably don't want any further execution?
						// this one is a bit more complicated, due to possible
						// interactions with Continue() (lastAction here is always
						// the first block encountered), needs more testing
						//if (done) *done = true;
						return true;
					}

					//movetoobjectfollow would break if this isn't called
					//(what is broken if it is here?)
					MySelf->ClearActions();
					//IE even clears the path, shall we?
					//yes we must :)
					if (MySelf->Type == ST_ACTOR) {
						((Movable *)MySelf)->ClearPath();
					}
				}
				lastAction=a;
			}
			continueExecution = ( ExecuteResponseSet( MySelf,
				rB->responseSet ) != 0 );
			if (continuing) *continuing = continueExecution;
			//clear triggers after response executed
			//MySelf->ClearTriggers();
			if (!continueExecution) {
				if (done) *done = true;
				break;
			}
			//continueExecution = false;
		}
	}
	return true;
}

void GameScript::EvaluateAllBlocks()
{
	if (!MySelf || !(MySelf->GetInternalFlag()&IF_ACTIVE) ) {
		return;
	}

	if (!script) {
		return;
	}
//according to research, cutscenes don't evaluate conditions, and always
//run the first response, this is a serious cutback on possible
//functionality, so i kept the gemrb specific code too.
#ifdef GEMRB_CUTSCENES
//this is the logical way of executing a cutscene
	for (unsigned int a = 0; a < script->responseBlocksCount; a++) {
		ResponseBlock* rB = script->responseBlocks[a];
		if (EvaluateCondition( MySelf, rB->condition )) {
			ExecuteResponseSet( MySelf, rB->responseSet );
		}
	}
#else
//this is the apparent IE behaviour
	for (unsigned int a = 0; a < script->responseBlocksCount; a++) {
		ResponseBlock* rB = script->responseBlocks[a];
		ResponseSet * rS = rB->responseSet;
		if (rS->responsesCount) {
			ExecuteResponse( MySelf, rS->responses[0] );
		}
	}
#endif
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

//this is the border of the GameScript object (all subsequent functions are library functions)
//we can't make this a library function, because scriptlevel is set here
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
	if (strncmp(poi,"AC",2)==0)
	while (true) {
		//not autofreed, because it is referenced by the Script
		Action* aC = new Action(false);
		count = stream->ReadLine( line, 1024 );
		aC->actionID = (unsigned short)strtoul(line, NULL,10);
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
		strlwr(aC->string0Parameter);
		strlwr(aC->string1Parameter);
		if (aC->actionID>=MAX_ACTIONS) {
			aC->actionID=0;
			printMessage("GameScript","Invalid script action ID!",LIGHT_RED);
		} else {
			if (actionflags[aC->actionID] & AF_SCRIPTLEVEL) {
				aC->int0Parameter = scriptlevel;
			}
		}
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

void GameScript::ExecuteString(Scriptable* Sender, char* String)
{
	if (String[0] == 0) {
		return;
	}
	Action* act = GenerateAction( String );
	if (!act) {
		return;
	}
	Sender->AddActionInFront(act);
}

//-------------------------------------------------------------
// Object Functions
//-------------------------------------------------------------

//in this implementation, Myself will drop the parameter array
//i think all object filters could be expected to do so
//they should remove unnecessary elements from the parameters
Targets *GameScript::Myself(Scriptable* Sender, Targets* parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(Sender, 0, ga_flags);
	return parameters;
}

Targets *GameScript::NearestDoor(Scriptable* /*Sender*/, Targets *parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 0);
}

Targets *GameScript::SecondNearestDoor(Scriptable* /*Sender*/, Targets *parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 1);
}

Targets *GameScript::ThirdNearestDoor(Scriptable* /*Sender*/, Targets *parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 2);
}

Targets *GameScript::FourthNearestDoor(Scriptable* /*Sender*/, Targets *parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 3);
}

Targets *GameScript::FifthNearestDoor(Scriptable* /*Sender*/, Targets *parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 4);
}

Targets *GameScript::SixthNearestDoor(Scriptable* /*Sender*/, Targets *parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 5);
}

Targets *GameScript::SeventhNearestDoor(Scriptable* /*Sender*/, Targets *parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 6);
}

Targets *GameScript::EighthNearestDoor(Scriptable* /*Sender*/, Targets *parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 7);
}

Targets *GameScript::NinthNearestDoor(Scriptable* /*Sender*/, Targets *parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 8);
}

Targets *GameScript::TenthNearestDoor(Scriptable* /*Sender*/, Targets *parameters, int /*ga_flags*/)
{
	return XthNearestDoor(parameters, 9);
}

//in bg2 it is same as player1 so far
//in iwd2 this is the Gabber!!!
//but also, if there is no gabber, it is the first PC
//probably it is simply the nearest exportable character...
Targets *GameScript::Protagonist(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	//this sucks but IWD2 is like that...
	if (charnameisgabber) {
		GameControl* gc = core->GetGameControl();
		if (gc) {
			parameters->AddTarget(gc->GetSpeaker(), 0, ga_flags);
		}
		if (parameters->Count()) {
			return parameters;
		}
		//ok, this will return the nearest PC in the first slot
		Game *game = core->GetGame();
		int i = game->GetPartySize(false);
		while(i--) {
			Actor *target = game->GetPC(i,false);
			parameters->AddTarget(target, Distance(Sender, target), ga_flags);
		}
		return parameters;
	}
	parameters->AddTarget(core->GetGame()->GetPC(0, false), 0, ga_flags);
	return parameters;
}

//last talker
Targets *GameScript::Gabber(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	GameControl* gc = core->GetGameControl();
	if (gc) {
		parameters->AddTarget(gc->GetSpeaker(), 0, ga_flags);
	}
	return parameters;
}

Targets *GameScript::LastTrigger(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	if (Sender->LastTriggerObject) {
		Actor *target = Sender->GetCurrentArea()->GetActorByGlobalID(Sender->LastTriggerObject);
		parameters->AddTarget(target, 0, ga_flags);
	}
	return parameters;
}

Targets *GameScript::LastMarkedObject(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastMarked);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

//actions should always use LastMarkedObject, because LastSeen could be deleted
Targets *GameScript::LastSeenBy(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastSeen);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets *GameScript::LastHelp(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastHelp);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets *GameScript::LastHeardBy(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastHeard);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

//i was told that Group means the same specifics, so this is just an
//object selector for everyone with the same specifics as the current object
Targets *GameScript::GroupOf(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		ieDword tmp = actor->GetStat(IE_SPECIFIC);
		Map *cm = Sender->GetCurrentArea();
		int i = cm->GetActorCount(true);
		while (i--) {
			Actor *target=cm->GetActor(i,true);
			if (target && (target->GetStat(IE_SPECIFIC)==tmp) ) {
				parameters->AddTarget(target, 0, ga_flags);
			}
		}
	}
	return parameters;
}

/*this one is tough, but done */
Targets *GameScript::ProtectorOf(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		ieWord tmp = actor->LastProtected;
		Map *cm = Sender->GetCurrentArea();
		int i = cm->GetActorCount(true);
		while (i--) {
			Actor *target=cm->GetActor(i,true);
			if (target && (target->LastProtected ==tmp) ) {
				parameters->AddTarget(target, 0, ga_flags);
			}
		}
	}
	return parameters;
}

Targets *GameScript::ProtectedBy(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastProtected);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets *GameScript::LastCommandedBy(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastCommander);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

// this is essentially a LastTargetedBy(0) - or MySelf
// but IWD2 defines it
Targets *GameScript::MyTarget(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	return GetMyTarget(Sender, NULL, parameters, ga_flags);
}

Targets *GameScript::LastTargetedBy(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	return GetMyTarget(Sender, actor, parameters, ga_flags);
}

Targets *GameScript::LastAttackerOf(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastHitter);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets *GameScript::LastHitter(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastHitter);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets *GameScript::LeaderOf(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastFollowed);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets *GameScript::LastTalkedToBy(Scriptable *Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastTalkedTo);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets *GameScript::LastSummonerOf(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	Actor *actor = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastSummoner);
		if (target) {
			parameters->AddTarget(target, 0, ga_flags);
		}
	}
	return parameters;
}

Targets *GameScript::Player1(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(0,false), 0, ga_flags);
	return parameters;
}

Targets *GameScript::Player1Fill(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(1), 0, ga_flags);
	return parameters;
}

Targets *GameScript::Player2(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(1,false), 0, ga_flags);
	return parameters;
}

Targets *GameScript::Player2Fill(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(2), 0, ga_flags);
	return parameters;
}

Targets *GameScript::Player3(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(2,false), 0, ga_flags);
	return parameters;
}

Targets *GameScript::Player3Fill(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(3), 0, ga_flags);
	return parameters;
}

Targets *GameScript::Player4(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(3,false), 0, ga_flags);
	return parameters;
}

Targets *GameScript::Player4Fill(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(4), 0, ga_flags);
	return parameters;
}

Targets *GameScript::Player5(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(4,false), 0, ga_flags);
	return parameters;
}

Targets *GameScript::Player5Fill(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(5), 0, ga_flags);
	return parameters;
}

Targets *GameScript::Player6(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(5,false), 0, ga_flags);
	return parameters;
}

Targets *GameScript::Player6Fill(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(6), 0, ga_flags);
	return parameters;
}

Targets *GameScript::Player7(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(6,false), 0, ga_flags);
	return parameters;
}

Targets *GameScript::Player7Fill(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(7), 0, ga_flags);
	return parameters;
}

Targets *GameScript::Player8(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->GetPC(7,false), 0, ga_flags);
	return parameters;
}

Targets *GameScript::Player8Fill(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	parameters->AddTarget(core->GetGame()->FindPC(8), 0, ga_flags);
	return parameters;
}

Targets *GameScript::BestAC(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	targetlist::iterator m;
	const targettype *t = parameters->GetFirstTarget(m, ST_ACTOR);
	if (!t) {
		return parameters;
	}
	Scriptable *scr=t->actor;
	Actor *actor=(Actor *) scr;
	int bestac=actor->GetStat(IE_ARMORCLASS);
	// assignment in while
	while ( (t = parameters->GetNextTarget(m, ST_ACTOR) ) ) {
		actor = (Actor *) t->actor;
		int ac=actor->GetStat(IE_ARMORCLASS);
		if (bestac<ac) {
			bestac=ac;
			scr=t->actor;
		}
	}

	parameters->Clear();
	parameters->AddTarget(scr, 0, ga_flags);
	return parameters;
}

/*no idea why this object exists since the gender could be filtered easier*/
Targets *GameScript::StrongestOfMale(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	targetlist::iterator m;
	const targettype *t = parameters->GetFirstTarget(m, ST_ACTOR);
	if (!t) {
		return parameters;
	}
	int pos=-1;
	int worsthp=-1;
	Scriptable *scr = NULL;
	//assignment intentional
	while ( (t = parameters->GetNextTarget(m, ST_ACTOR) ) ) {
		Actor *actor = (Actor *) t->actor;
		if (actor->GetStat(IE_SEX)!=SEX_MALE) continue;
		int hp=actor->GetStat(IE_HITPOINTS);
		if ((pos==-1) || (worsthp<hp)) {
			worsthp=hp;
			scr=t->actor;
		}
	}
	parameters->Clear();
	if (scr) {
		parameters->AddTarget(scr, 0, ga_flags);
	}
	return parameters;
}

Targets *GameScript::StrongestOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	targetlist::iterator m;
	const targettype *t = parameters->GetFirstTarget(m, ST_ACTOR);
	if (!t) {
		return parameters;
	}
	Scriptable *scr=t->actor;
	Actor *actor=(Actor *) scr;
	int besthp=actor->GetStat(IE_HITPOINTS);
	// assignment in while
	while ( (t = parameters->GetNextTarget(m, ST_ACTOR) ) ) {
		actor = (Actor *) t->actor;
		int hp=actor->GetStat(IE_HITPOINTS);
		if (besthp<hp) {
			besthp=hp;
			scr=t->actor;
		}
	}
	parameters->Clear();
	parameters->AddTarget(scr, 0, ga_flags);
	return parameters;
}

Targets *GameScript::WeakestOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	targetlist::iterator m;
	const targettype *t = parameters->GetFirstTarget(m, ST_ACTOR);
	if (!t) {
		return parameters;
	}
	Scriptable *scr=t->actor;
	Actor *actor=(Actor *) scr;
	int worsthp=actor->GetStat(IE_HITPOINTS);
	// assignment in while
	while ( (t = parameters->GetNextTarget(m, ST_ACTOR) ) ) {
		actor = (Actor *) t->actor;
		int hp=actor->GetStat(IE_HITPOINTS);
		if (worsthp>hp) {
			worsthp=hp;
			scr=t->actor;
		}
	}
	parameters->Clear();
	parameters->AddTarget(scr, 0, ga_flags);
	return parameters;
}

Targets *GameScript::WorstAC(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	targetlist::iterator m;
	const targettype *t = parameters->GetFirstTarget(m, ST_ACTOR);
	if (!t) {
		return parameters;
	}
	Scriptable *scr=t->actor;
	Actor *actor=(Actor *) scr;
	int worstac=actor->GetStat(IE_ARMORCLASS);
	// assignment in while
	while ( (t = parameters->GetNextTarget(m, ST_ACTOR) ) ) {
		actor = (Actor *) t->actor;
		int ac=actor->GetStat(IE_ARMORCLASS);
		if (worstac>ac) {
			worstac=ac;
			scr=t->actor;
		}
	}
	parameters->Clear();
	parameters->AddTarget(scr, 0, ga_flags);
	return parameters;
}

Targets *GameScript::MostDamagedOf(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	//Original engines restrict this to the PCs...
	/*targetlist::iterator m;
	const targettype *t = parameters->GetFirstTarget(m, ST_ACTOR);
	if (!t) {
		return parameters;
	}
	Scriptable *scr = t->actor;
	Actor *actor=(Actor *) scr;
	int worsthp=actor->GetStat(IE_MAXHITPOINTS)-actor->GetBase(IE_HITPOINTS);
	// assignment in while
	while ( (t = parameters->GetNextTarget(m, ST_ACTOR) ) ) {
		actor = (Actor *) t->actor;
		int hp=actor->GetStat(IE_MAXHITPOINTS)-actor->GetBase(IE_HITPOINTS);
		if (worsthp>hp) {
			worsthp=hp;
			scr=t->actor;
		}
	}
	parameters->Clear();
	parameters->AddTarget(scr, 0, ga_flags);
	return parameters;*/
	Map* area = Sender->GetCurrentArea() ;
	Game *game = core->GetGame();
	Scriptable* scr = NULL ;
	int worsthp = 0xffff ;
	int i = game->GetPartySize(false);
	while (i--) {
		Actor *actor = game->GetPC(i, false);
		if(actor->GetCurrentArea() == area) {
			int hp=actor->GetStat(IE_MAXHITPOINTS)-actor->GetBase(IE_HITPOINTS);
			if (worsthp>hp) {
				worsthp=hp;
				scr=actor;
			}
		}
	}
	parameters->Clear();
	parameters->AddTarget(scr, 0, ga_flags);
	return parameters;
}
Targets *GameScript::LeastDamagedOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	targetlist::iterator m;
	const targettype *t = parameters->GetFirstTarget(m, ST_ACTOR);
	if (!t) {
		return parameters;
	}
	Scriptable *scr = t->actor;
	Actor *actor = (Actor *) scr;
	int besthp=actor->GetStat(IE_MAXHITPOINTS)-actor->GetBase(IE_HITPOINTS);
	// assignment in while
	while ( (t = parameters->GetNextTarget(m, ST_ACTOR) ) ) {
		actor = (Actor *) t->actor;
		int hp=actor->GetStat(IE_MAXHITPOINTS)-actor->GetBase(IE_HITPOINTS);
		if (besthp<hp) {
			besthp=hp;
			scr=t->actor;
		}
	}
	parameters->Clear();
	parameters->AddTarget(scr, 0, ga_flags);
	return parameters;
}

Targets *GameScript::Farthest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	const targettype *t = parameters->GetLastTarget(ST_ACTOR);
	parameters->Clear();
	if (t) {
		parameters->AddTarget(t->actor, 0, ga_flags);
	}
	return parameters;
}

Targets *GameScript::FarthestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, -1, ga_flags);
}

Targets *GameScript::NearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 0, ga_flags);
}

Targets *GameScript::SecondNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 1, ga_flags);
}

Targets *GameScript::ThirdNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 2, ga_flags);
}

Targets *GameScript::FourthNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 3, ga_flags);
}

Targets *GameScript::FifthNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 4, ga_flags);
}

Targets *GameScript::SixthNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 5, ga_flags);
}

Targets *GameScript::SeventhNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 6, ga_flags);
}

Targets *GameScript::EighthNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 7, ga_flags);
}

Targets *GameScript::NinthNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 8, ga_flags);
}

Targets *GameScript::TenthNearestEnemyOf(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOf(parameters, 9, ga_flags);
}

Targets *GameScript::NearestEnemySummoned(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return ClosestEnemySummoned(Sender, parameters, ga_flags);
}

Targets *GameScript::NearestEnemyOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 0, ga_flags);
}

Targets *GameScript::SecondNearestEnemyOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 1, ga_flags);
}

Targets *GameScript::ThirdNearestEnemyOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 2, ga_flags);
}

Targets *GameScript::FourthNearestEnemyOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 3, ga_flags);
}

Targets *GameScript::FifthNearestEnemyOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 4, ga_flags);
}

Targets *GameScript::SixthNearestEnemyOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 5, ga_flags);
}

Targets *GameScript::SeventhNearestEnemyOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 6, ga_flags);
}

Targets *GameScript::EighthNearestEnemyOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 7, ga_flags);
}

Targets *GameScript::NinthNearestEnemyOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 8, ga_flags);
}

Targets *GameScript::TenthNearestEnemyOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestEnemyOfType(Sender, parameters, 9, ga_flags);
}

Targets *GameScript::NearestMyGroupOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 0, ga_flags);
}

Targets *GameScript::SecondNearestMyGroupOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 1, ga_flags);
}

Targets *GameScript::ThirdNearestMyGroupOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 2, ga_flags);
}

Targets *GameScript::FourthNearestMyGroupOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 3, ga_flags);
}

Targets *GameScript::FifthNearestMyGroupOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 4, ga_flags);
}

Targets *GameScript::SixthNearestMyGroupOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 5, ga_flags);
}

Targets *GameScript::SeventhNearestMyGroupOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 6, ga_flags);
}

Targets *GameScript::EighthNearestMyGroupOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 7, ga_flags);
}

Targets *GameScript::NinthNearestMyGroupOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 8, ga_flags);
}

Targets *GameScript::TenthNearestMyGroupOfType(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	return XthNearestMyGroupOfType(Sender, parameters, 9, ga_flags);
}

/* returns only living PC's? if not, alter getpartysize/getpc flag*/
Targets *GameScript::NearestPC(Scriptable* Sender, Targets *parameters, int ga_flags)
{
	parameters->Clear();
	Map *map = Sender->GetCurrentArea();
	Game *game = core->GetGame();
	int i = game->GetPartySize(true);
	int mindist = -1;
	Actor *ac = NULL;
	while (i--) {
		Actor *newactor=game->GetPC(i,true);
		//NearestPC for PC's will not give themselves as a result
		//this might be different from the original engine
		if ((Sender->Type==ST_ACTOR) && (newactor == (Actor *) Sender)) {
			continue;
		}
		if (newactor->GetCurrentArea()!=map) {
			continue;
		}
		int dist = Distance(Sender, newactor);
		if ( (mindist == -1) || (dist<mindist) ) {
			ac = newactor;
			mindist = dist;
		}
	}
	if (ac) {
		parameters->AddTarget(ac, 0, ga_flags);
	}
	return parameters;
}

Targets *GameScript::Nearest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestOf(parameters, 0, ga_flags);
}

Targets *GameScript::SecondNearest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestOf(parameters, 1, ga_flags);
}

Targets *GameScript::ThirdNearest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestOf(parameters, 2, ga_flags);
}

Targets *GameScript::FourthNearest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestOf(parameters, 3, ga_flags);
}

Targets *GameScript::FifthNearest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestOf(parameters, 4, ga_flags);
}

Targets *GameScript::SixthNearest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestOf(parameters, 5, ga_flags);
}

Targets *GameScript::SeventhNearest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestOf(parameters, 6, ga_flags);
}

Targets *GameScript::EighthNearest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestOf(parameters, 7, ga_flags);
}

Targets *GameScript::NinthNearest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestOf(parameters, 8, ga_flags);
}

Targets *GameScript::TenthNearest(Scriptable* /*Sender*/, Targets *parameters, int ga_flags)
{
	return XthNearestOf(parameters, 9, ga_flags);
}

Targets *GameScript::SelectedCharacter(Scriptable* Sender, Targets* parameters, int ga_flags)
{
	Map *cm = Sender->GetCurrentArea();
	parameters->Clear();
	int i = cm->GetActorCount(true);
	while (i--) {
		Actor *ac=cm->GetActor(i,true);
		if (ac->GetCurrentArea()!=cm) {
			continue;
		}
		if (ac->IsSelected()) {
			parameters->AddTarget(ac, Distance(Sender, ac), ga_flags );
		}
	}
	return parameters;
}

Targets *GameScript::Nothing(Scriptable* /*Sender*/, Targets* parameters, int /*ga_flags*/)
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
		case EA_GOODCUTOFF:
			//goodcutoff
			return value <= EA_GOODCUTOFF;

		case EA_NOTGOOD:
			//notgood
			return value >= EA_NOTGOOD;

		case EA_NOTEVIL:
			//notevil
			return value <= EA_NOTEVIL;

		case EA_EVILCUTOFF:
			//evilcutoff
			return value >= EA_EVILCUTOFF;

		case 0:
		case 126:
			//anything
			return true;

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

//This must return integer because Or(3) returns 3
int GameScript::EvaluateString(Scriptable* Sender, char* String)
{
	if (String[0] == 0) {
		return 0;
	}
	Trigger* tri = GenerateTrigger( String );
	if (tri) {
		int ret = EvaluateTrigger( Sender, tri );
		tri->Release();
		return ret;
	}
	return 0;
}

bool GameScript::EvaluateCondition(Scriptable* Sender, Condition* condition)
{
	int ORcount = 0;
	unsigned int result = 0;
	bool subresult = true;

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
		printMessage( "GameScript","Trigger evaluation fails due to NULL trigger.\n",LIGHT_RED );
		return 0;
	}
	TriggerFunction func = triggers[trigger->triggerID];
	const char *tmpstr=triggersTable->GetValue(trigger->triggerID);
	if (!tmpstr) {
		tmpstr=triggersTable->GetValue(trigger->triggerID|0x4000);
	}
	if (!func) {
		triggers[trigger->triggerID] = False;
		printMessage("GameScript"," ",YELLOW);
		printf("Unhandled trigger code: 0x%04x %s\n",
			trigger->triggerID, tmpstr );
		return 0;
	}
	if (InDebug&ID_TRIGGERS) {
		printMessage("GameScript"," ",YELLOW);
		printf( "Executing trigger code: 0x%04x %s\n",
				trigger->triggerID, tmpstr );
	}
	int ret = func( Sender, trigger );
	if (trigger->flags & NEGATE_TRIGGER) {
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
		randWeight-=rE->weight;
	}
	return 0;
}

//continue is effective only as the last action in the block
int GameScript::ExecuteResponse(Scriptable* Sender, Response* rE)
{
	int ret = 0; // continue or not
	for (int i = 0; i < rE->actionsCount; i++) {
		Action* aC = rE->actions[i];
		switch (actionflags[aC->actionID] & AF_MASK) {
			case AF_INSTANT:
				ExecuteAction( Sender, aC );
				ret = 0;
				break;
			case AF_NONE:
				if (Sender->GetInternalFlag()&IF_CUTSCENEID) {
					Scriptable *cs = Sender->GetCutsceneID();
					if (cs) {
						// maybe this belongs somewhere else, but it certainly
						// happens at the start of the cutscene (note that the
						// queue is NOT cleared, so you can still have a pending
						// action block your cutscene!)
						cs->ReleaseCurrentAction();

						cs->AddAction( aC );
					} else {
						//this can happen if a script refers to a wrong cutsceneid
						if (InDebug&ID_CUTSCENE) {
							printMessage("GameScript","Did not find cutscene object, action ignored!\n",YELLOW);
						}
					}
				} else {
					//this shouldn't happen, i think
					if (Sender->GetCutsceneID()) {
						printf("Stuck with cutscene ID!\n");
						abort();
					}
					//ogres in dltc need this
					Sender->AddAction( aC );
					//this was a mistake, nothing
					//requires it, so use the code above
					//AddAction( Sender, aC );
				}
				ret = 0;
				break;
			case AF_CONTINUE:
			case AF_MASK:
				ret = 1;
				break;
		}
	}
	return ret;
}

void PrintAction(int actionID)
{
	printf("Action: %d %s\n", actionID , actionsTable->GetValue(actionID) );
}

void GameScript::ExecuteAction(Scriptable* Sender, Action* aC)
{
	int actionID = aC->actionID;

	if (aC->objects[0]) {
		Scriptable *scr = GetActorFromObject(Sender, aC->objects[0]);
		if (scr) {
			if (InDebug&ID_ACTIONS) {
				printMessage("GameScript"," ",YELLOW);
				printf("Sender: %s-->override: %s\n",Sender->GetScriptName(), scr->GetScriptName() );
			}
			//Sender->CurrentAction
			scr->AddAction(ParamCopyNoOverride(aC));
			scr->ReleaseCurrentAction(); // goes after AddAction, otherwise we might delete aC!
			// there are plenty of places where it's vital that ActionOverride is not interrupted and if
			// there are actions left on the queue after the release above, we can't instant-execute,
			// so this is my best guess for now..
			scr->CurrentActionInterruptable = false;
		} else {
			printMessage("GameScript","Actionoverride failed for object: \n",LIGHT_RED);
			aC->objects[0]->Dump();
		}
		Sender->ReleaseCurrentAction();
		return;
	}
	if (InDebug&ID_ACTIONS) {
		printMessage("GameScript"," ",YELLOW);
		PrintAction(actionID);
		printf("Sender: %s\n",Sender->GetScriptName() );
	}
	ActionFunction func = actions[actionID];
	if (func) {
		//turning off interruptable flag
		//uninterruptable actions will set it back
		if (Sender->Type==ST_ACTOR) {
			Sender->Activate();
			if (actionflags[actionID]&AF_ALIVE) {
				if (Sender->GetInternalFlag()&IF_STOPATTACK) {
					printMessage("GameScript", "Aborted action due to death\n", YELLOW);
					Sender->ReleaseCurrentAction();
					return;
				}
			}
		}
		func( Sender, aC );
	} else {
		actions[actionID] = NoActionAtAll;
		printMessage("GameScript", "Unknown ", YELLOW);
		textcolor(YELLOW);
		PrintAction(actionID);
		Sender->ReleaseCurrentAction();
		textcolor(WHITE);
		return;
	}

	//don't bother with special flow control actions
	if (actionflags[actionID] & AF_INSTANT) {
		//this action never entered the action queue, therefore shouldn't be freed
		if (aC->GetRef()!=1) {
			printf("Instant action got queued!\n");
			PrintAction(actionID);
			abort();
		}
		return;
	}

	//Releasing nonblocking actions, blocking actions will release themselves
	if (!( actionflags[actionID] & AF_BLOCKING )) {
		Sender->ReleaseCurrentAction();
		//aC is invalid beyond this point, so we return!
		return;
	}
}

Trigger* GenerateTrigger(char* String)
{
	strlwr( String );
	if (InDebug&ID_TRIGGERS) {
		printMessage("GameScript"," ",YELLOW);
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

Action* GenerateAction(char* String)
{
	strlwr( String );
	if (InDebug&ID_ACTIONS) {
		printMessage("GameScript"," ",YELLOW);
		printf("Compiling:%s\n",String);
	}
	int len = strlench(String,'(')+1; //including (
	int i = actionsTable->FindString(String, len);
	if (i<0) {
		printMessage("GameScript"," ",LIGHT_RED);
		printf("Invalid scripting action: %s\n", String);
		return NULL;
	}
	char *src = String+len;
	char *str = actionsTable->GetStringIndex( i )+len;
	return GenerateActionCore( src, str, i);
}

Action* GenerateActionDirect(char *String, Actor *object)
{
	Action* action = GenerateAction(String);
	Object *tmp = action->objects[1];
	if (tmp && tmp->objectFields[0]==-1) {
		tmp->objectFields[1] = object->globalID;
	}
	action->pointParameter.empty();
	return action;
}

/** Self-destructing object if it is empty */
bool Object::ReadyToDie()
{
	if (objectName[0]!=0) {
		return false;
	}
	if (objectFilters[0]) {
		return false;
	}
	for (int i=0;i<ObjectFieldsCount;i++) {
		if (objectFields[i]) {
			return false;
		}
	}
	//commit suicide
	Release();
	return true;
}

