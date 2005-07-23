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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/GameScript.cpp,v 1.319 2005/07/23 19:49:25 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "GameScript.h"
#include "Interface.h"
#include "GSUtils.h"

//debug flags
// 1 - cache
// 2 - cutscene ID
// 4 - globals
// 8 - script/trigger execution

//Make this an ordered list, so we could use bsearch!
static TriggerLink triggernames[] = {
	{"actionlistempty", GameScript::ActionListEmpty, 0},
	{"acquired", GameScript::Acquired, 0},
	{"alignment", GameScript::Alignment, 0},
	{"allegiance", GameScript::Allegiance, 0},
	{"animstate", GameScript::AnimState, 0},
	{"anypconmap", GameScript::AnyPCOnMap, 0},
	{"areacheck", GameScript::AreaCheck, 0},
	{"areacheckobject", GameScript::AreaCheckObject, 0},
	{"areaflag", GameScript::AreaFlag, 0},
	{"areatype", GameScript::AreaType, 0},
	{"arearestdisabled", GameScript::AreaRestDisabled, 0},
	{"atlocation", GameScript::AtLocation, 0},
	{"attackedby", GameScript::AttackedBy, 0},
	{"bitcheck", GameScript::BitCheck,TF_MERGESTRINGS},
	{"bitcheckexact", GameScript::BitCheckExact,TF_MERGESTRINGS},
	{"bitglobal", GameScript::BitGlobal_Trigger,TF_MERGESTRINGS},
	{"breakingpoint", GameScript::BreakingPoint, 0},
	{"calledbyname", GameScript::CalledByName, 0}, //this is still a question
	{"chargecount", GameScript::ChargeCount, 0},
	{"charname", GameScript::CharName, 0}, //not scripting name
	{"checkdoorflags", GameScript::CheckDoorFlags, 0},
	{"checkpartyaveragelevel", GameScript::CheckPartyAverageLevel, 0},
	{"checkpartylevel", GameScript::CheckPartyLevel, 0},
	{"checkstat", GameScript::CheckStat, 0},
	{"checkstatgt", GameScript::CheckStatGT, 0},
	{"checkstatlt", GameScript::CheckStatLT, 0},
	{"class", GameScript::Class, 0},
	{"classex", GameScript::ClassEx, 0}, //will return true for multis
	{"classlevel", GameScript::ClassLevel, 0},
	{"classlevelgt", GameScript::ClassLevelGT, 0},
	{"classlevellt", GameScript::ClassLevelLT, 0},
	{"clicked", GameScript::Clicked, 0},
	{"combatcounter", GameScript::CombatCounter, 0},
	{"combatcountergt", GameScript::CombatCounterGT, 0},
	{"combatcounterlt", GameScript::CombatCounterLT, 0},
	{"contains", GameScript::Contains, 0},
	{"currentareais", GameScript::AreaCheck, 0},
	{"creatureinarea", GameScript::AreaCheck, 0}, //cannot check others
	{"damagetaken", GameScript::DamageTaken, 0},
	{"damagetakengt", GameScript::DamageTakenGT, 0},
	{"damagetakenlt", GameScript::DamageTakenLT, 0},
	{"dead", GameScript::Dead, 0},
	{"delay", GameScript::Delay, 0},
	{"detect", GameScript::See, 0}, //so far i see no difference
	{"die", GameScript::Die, 0},
	{"died", GameScript::Died, 0},
	{"difficulty", GameScript::Difficulty, 0},
	{"difficultygt", GameScript::DifficultyGT, 0},
	{"difficultylt", GameScript::DifficultyLT, 0},
	{"entered", GameScript::Entered, 0},
	{"entirepartyonmap", GameScript::EntirePartyOnMap, 0},
	{"exists", GameScript::Exists, 0},
	{"extraproficiency", GameScript::ExtraProficiency, 0},
	{"extraproficiencygt", GameScript::ExtraProficiencyGT, 0},
	{"extraproficiencylt", GameScript::ExtraProficiencyLT, 0},
	{"faction", GameScript::Faction, 0},
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
	{"globalband", GameScript::BitCheck,AF_MERGESTRINGS},
	{"globalbandglobal", GameScript::GlobalBAndGlobal_Trigger,AF_MERGESTRINGS},
	{"globalbandglobalexact", GameScript::GlobalBAndGlobalExact,AF_MERGESTRINGS},
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
	{"happiness", GameScript::Happiness, 0},
	{"happinessgt", GameScript::HappinessGT, 0},
	{"happinesslt", GameScript::HappinessLT, 0},
	{"harmlessentered", GameScript::IsOverMe, 0}, //pst, not sure
	{"hasinnateability", GameScript::HaveSpell, 0}, //these must be the same
	{"hasitem", GameScript::HasItem, 0},
	{"hasitemequiped", GameScript::HasItemEquipped, 0}, //typo in bg2
	{"hasitemequipped", GameScript::HasItemEquipped, 0},
	{"hasitemslot", GameScript::HasItemSlot, 0},
	{"hasiteminslot", GameScript::HasItemSlot, 0},
	{"hasweaponequipped", GameScript::HasWeaponEquipped, 0},
	{"haveanyspells", GameScript::HaveAnySpells, 0},
	{"havespell", GameScript::HaveSpell, 0}, //these must be the same
	{"havespellparty", GameScript::HaveSpellParty, 0}, 
	{"havespellres", GameScript::HaveSpell, 0}, //they share the same ID
	{"heard", GameScript::Heard, 0},
	{"help", GameScript::Help_Trigger, 0},
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
	{"inparty", GameScript::InParty, 0},
	{"inpartyallowdead", GameScript::InPartyAllowDead, 0},
	{"inpartyslot", GameScript::InPartySlot, 0},
	{"internal", GameScript::Internal, 0},
	{"internalgt", GameScript::InternalGT, 0},
	{"internallt", GameScript::InternalLT, 0},
	{"interactingwith", GameScript::InteractingWith, 0},
	{"inventoryfull", GameScript::InventoryFull, 0},
	{"inview", GameScript::LOS, 0}, //it seems the same, needs research
	{"inweaponrange", GameScript::InWeaponRange, 0},
	{"isaclown", GameScript::IsAClown, 0},
	{"isactive", GameScript::IsActive, 0},
	{"isanimationid", GameScript::AnimationID, 0},
	{"iscreatureareaflag", GameScript::IsCreatureAreaFlag, 0},
	{"isfacingobject", GameScript::IsFacingObject, 0},
	{"isfacingsavedrotation", GameScript::IsFacingSavedRotation, 0},
	{"isgabber", GameScript::IsGabber, 0},
	{"islocked", GameScript::IsLocked, 0},
	{"isextendednight", GameScript::IsExtendedNight, 0},
	{"isoverme", GameScript::IsOverMe, 0}, // same as harmlessentered?
	{"isplayernumber", GameScript::IsPlayerNumber, 0},
	{"isrotation", GameScript::IsRotation, 0},
	{"isscriptname", GameScript::CalledByName, 0}, //seems the same
	{"isteambiton", GameScript::IsTeamBitOn, 0},
	{"isvalidforpartydialog", GameScript::IsValidForPartyDialog, 0},
	{"isweather", GameScript::IsWeather, 0}, //gemrb extension
	{"itemisidentified", GameScript::ItemIsIdentified, 0},
	{"kit", GameScript::Kit, 0},
	{"lastmarkedobject", GameScript::LastMarkedObject_Trigger, 0},
	{"lastpersontalkedto", GameScript::LastPersonTalkedTo, 0}, //pst
	{"level", GameScript::Level, 0},
	{"levelgt", GameScript::LevelGT, 0},
	{"levelinclass", GameScript::ClassLevel, 0},
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
	{"nearlocation", GameScript::NearLocation, 0},
	{"nearsavedlocation", GameScript::NearSavedLocation, 0},
	{"notstatecheck", GameScript::NotStateCheck, 0},
	{"nulldialog", GameScript::NullDialog, 0},
	{"numcreature", GameScript::NumCreatures, 0},
	{"numcreaturegt", GameScript::NumCreaturesGT, 0},
	{"numcreaturelt", GameScript::NumCreaturesLT, 0},
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
	{"numtimesinteractedobject", GameScript::NumTimesInteractedObject, 0},    //gemrb
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
	{"openstate", GameScript::OpenState, 0},
	{"or", GameScript::Or, 0},
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
	{"pccanseepoint", GameScript::PCCanSeePoint, 0},
	{"pcinstore", GameScript::PCInStore, 0},
	{"personalspacedistance", GameScript::PersonalSpaceDistance, 0},
	{"proficiency", GameScript::Proficiency, 0},
	{"proficiencygt", GameScript::ProficiencyGT, 0},
	{"proficiencylt", GameScript::ProficiencyLT, 0},
	{"race", GameScript::Race, 0},
	{"randomnum", GameScript::RandomNum, 0},
	{"randomnumgt", GameScript::RandomNumGT, 0},
	{"randomnumlt", GameScript::RandomNumLT, 0},
	{"range", GameScript::Range, 0},
	{"reaction", GameScript::Reaction, 0},
	{"reactiongt", GameScript::ReactionGT, 0},
	{"reactionlt", GameScript::ReactionLT, 0},
	{"realglobaltimerexact", GameScript::RealGlobalTimerExact, 0},
	{"realglobaltimerexpired", GameScript::RealGlobalTimerExpired, 0},
	{"realglobaltimernotexpired", GameScript::RealGlobalTimerNotExpired,0},
	{"reputation", GameScript::Reputation, 0},
	{"reputationgt", GameScript::ReputationGT, 0},
	{"reputationlt", GameScript::ReputationLT, 0},
	{"see", GameScript::See, 0},
	{"specifics", GameScript::Specifics, 0},
	{"statecheck", GameScript::StateCheck, 0},
	{"stuffglobalrandom", GameScript::StuffGlobalRandom, 0},//hm, this is a trigger
	{"subrace", GameScript::SubRace, 0},
	{"targetunreachable", GameScript::TargetUnreachable, 0},
	{"team", GameScript::Team, 0},
	{"time", GameScript::Time, 0},
	{"timegt", GameScript::TimeGT, 0},
	{"timelt", GameScript::TimeLT, 0},
	{"tookdamage", GameScript::TookDamage, 0},
	{"traptriggered", GameScript::TrapTriggered, 0},
	{"triggerclick", GameScript::Clicked, 0}, //not sure
	{"true", GameScript::True, 0},
	{"unselectablevariable", GameScript::UnselectableVariable, 0},
	{"unselectablevariablegt", GameScript::UnselectableVariableGT, 0},
	{"unselectablevariablelt", GameScript::UnselectableVariableLT, 0},
	{"vacant",GameScript::Vacant, 0},
	{"xor", GameScript::Xor,TF_MERGESTRINGS},
	{"xp", GameScript::XP, 0},
	{"xpgt", GameScript::XPGT, 0},
	{"xplt", GameScript::XPLT, 0}, { NULL,NULL,0},
};

//Make this an ordered list, so we could use bsearch!
static ActionLink actionnames[] = {
	{"actionoverride",NULL, AF_INVALID}, //will this function ever be reached
	{"activate", GameScript::Activate, 0},
	{"addareaflag", GameScript::AddAreaFlag, 0},
	{"addareatype", GameScript::AddAreaType, 0},
	{"addexperienceparty", GameScript::AddExperienceParty, 0},
	{"addexperiencepartyglobal", GameScript::AddExperiencePartyGlobal, 0},
	{"addglobals", GameScript::AddGlobals, 0},
	{"addhp", GameScript::AddHP, 0},
	{"addjournalentry", GameScript::AddJournalEntry, 0},
	{"addmapnote", GameScript::AddMapnote, 0},
	{"addpartyexperience", GameScript::AddExperienceParty, 0},
	{"addspecialability", GameScript::AddSpecialAbility, 0},
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
	{"attack", GameScript::Attack,AF_BLOCKING},
	{"attackreevaluate", GameScript::AttackReevaluate,AF_BLOCKING},
	{"bashdoor", GameScript::OpenDoor,AF_BLOCKING}, //the same until we know better
	{"battlesong", GameScript::BattleSong, 0},
	{"berserk", GameScript::Berserk, 0}, 
	{"bitclear", GameScript::BitClear,AF_MERGESTRINGS},
	{"bitglobal", GameScript::BitGlobal,AF_MERGESTRINGS},
	{"bitset", GameScript::GlobalBOr,AF_MERGESTRINGS}, //probably the same
	{"calm", GameScript::Calm, 0}, 
	{"changeaiscript", GameScript::ChangeAIScript, 0},
	{"changealignment", GameScript::ChangeAlignment, 0},
	{"changeallegiance", GameScript::ChangeAllegiance, 0},
	{"changeanimation", GameScript::ChangeAnimation, 0},
	{"changeanimationnoeffect", GameScript::ChangeAnimationNoEffect, 0},
	{"changeclass", GameScript::ChangeClass, 0},
	{"changecurrentscript", GameScript::ChangeAIScript,AF_SCRIPTLEVEL},
	{"changedialog", GameScript::ChangeDialogue, 0},
	{"changedialogue", GameScript::ChangeDialogue, 0},
	{"changegender", GameScript::ChangeGender, 0},
	{"changegeneral", GameScript::ChangeGeneral, 0},
	{"changeenemyally", GameScript::ChangeAllegiance, 0}, //this is the same
	{"changerace", GameScript::ChangeRace, 0},
	{"changespecifics", GameScript::ChangeSpecifics, 0},
	{"changestat", GameScript::ChangeStat, 0},
	{"changestoremarkup", GameScript::ChangeStoreMarkup, 0},//iwd2
	{"chunkcreature", GameScript::Kill, 0}, //should be more graphical
	{"clearactions", GameScript::ClearActions, 0},
	{"clearallactions", GameScript::ClearAllActions, 0},
	{"closedoor", GameScript::CloseDoor,AF_BLOCKING},
	{"containerenable", GameScript::ContainerEnable, 0},
	{"continue", GameScript::Continue,AF_INSTANT | AF_CONTINUE},
	{"copygroundpilesto", GameScript::CopyGroundPilesTo, 0},
	{"createcreature", GameScript::CreateCreature, 0}, //point is relative to Sender
	{"createcreatureatfeet", GameScript::CreateCreatureAtFeet, 0}, 
	{"createcreatureatlocation", GameScript::CreateCreatureAtLocation, 0},
	{"createcreatureimpassable", GameScript::CreateCreatureImpassable, 0},
	{"createcreatureimpassableallowoverlap", GameScript::CreateCreatureImpassableAllowOverlap, 0},
	{"createcreatureobject", GameScript::CreateCreatureObjectOffset, 0}, //the same
	{"createcreatureobjectoffscreen", GameScript::CreateCreatureObjectOffScreen, 0}, //same as createcreature object, but starts looking for a place far away from the player
	{"createcreatureobjectoffset", GameScript::CreateCreatureObjectOffset, 0}, //the same
	{"createcreatureoffscreen", GameScript::CreateCreatureOffScreen, 0},
	{"createitem", GameScript::CreateItem, 0},
	{"createitemglobal", GameScript::CreateItemNumGlobal, 0},
	{"createitemnumglobal", GameScript::CreateItemNumGlobal, 0},
	{"createpartygold", GameScript::CreatePartyGold, 0},
	{"createvisualeffect", GameScript::CreateVisualEffect, 0},
	{"createvisualeffectobject", GameScript::CreateVisualEffectObject, 0},
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
	{"dialogue", GameScript::Dialogue,AF_BLOCKING},
	{"dialogueforceinterrupt", GameScript::DialogueForceInterrupt,AF_BLOCKING},
	{"displaymessage", GameScript::DisplayMessage, 0},
	{"displaystring", GameScript::DisplayString, 0},
	{"displaystringhead", GameScript::DisplayStringHead, 0},
	{"displaystringheadowner", GameScript::DisplayStringHeadOwner, 0},
	{"displaystringheaddead", GameScript::DisplayStringHead, 0}, //same?
	{"displaystringnoname", GameScript::DisplayStringNoName, 0},
	{"displaystringnonamehead", GameScript::DisplayStringNoNameHead, 0},
	{"displaystringwait", GameScript::DisplayStringWait,AF_BLOCKING},
	{"dropinventory", GameScript::DropInventory, 0},
	{"dropinventoryex", GameScript::DropInventoryEX, 0},
	{"dropitem", GameScript::DropItem, AF_BLOCKING},
	{"endcredits", GameScript::EndCredits, 0},//movie
	{"endcutscenemode", GameScript::EndCutSceneMode, 0},
	{"enemy", GameScript::Enemy, 0},
	{"equipitem", GameScript::EquipItem, AF_BLOCKING},
	{"erasejournalentry", GameScript::RemoveJournalEntry, 0},
	{"escapearea", GameScript::EscapeArea, 0},
	{"escapeareadestroy", GameScript::EscapeAreaDestroy, 0},
	{"escapeareaobject", GameScript::EscapeAreaObject, 0},
	{"escapeareaobjectnosee", GameScript::EscapeAreaObjectNoSee, 0},
	{"expansionendcredits", GameScript::QuitGame, 0},//ends game too
	{"explore", GameScript::Explore, 0},
	{"exploremapchunk", GameScript::ExploreMapChunk, 0},
	{"face", GameScript::Face,AF_BLOCKING},
	{"faceobject", GameScript::FaceObject, AF_BLOCKING},
	{"facesavedlocation", GameScript::FaceSavedLocation, AF_BLOCKING},
	{"fadefromblack", GameScript::FadeFromColor, 0}, //probably the same
	{"fadefromcolor", GameScript::FadeFromColor, 0},
	{"fadetoblack", GameScript::FadeToColor, 0}, //probably the same
	{"fadetocolor", GameScript::FadeToColor, 0},
	{"findtraps", GameScript::FindTraps, 0},
	{"floatmessage", GameScript::DisplayStringHead, 0},
	{"floatmessagefixed", GameScript::FloatMessageFixed, 0},
	{"floatmessagefixedrnd", GameScript::FloatMessageFixedRnd, 0},
	{"floatmessagernd", GameScript::FloatMessageRnd, 0},
	{"forceaiscript", GameScript::ForceAIScript, 0},
	{"forceattack", GameScript::ForceAttack, 0},
	{"forcefacing", GameScript::ForceFacing, 0},
	{"forceleavearealua", GameScript::ForceLeaveAreaLUA, 0},
	{"forcespell", GameScript::ForceSpell, 0},
	{"fullheal", GameScript::FullHeal, 0},
	{"getstat", GameScript::GetStat, 0}, //gemrb specific
	{"getitem", GameScript::GetItem, 0},
	{"giveexperience", GameScript::AddXPObject, 0},
	{"givegoldforce", GameScript::CreatePartyGold, 0}, //this is the same
	{"giveitem", GameScript::GiveItem, 0},
	{"giveitemcreate", GameScript::CreateItem, 0}, //actually this is a targeted createitem
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
	{"gotostartscreen", GameScript::QuitGame, 0},
	{"help", GameScript::Help, 0},
	{"hide", GameScript::Hide, 0},
	{"hideareaonmap", GameScript::HideAreaOnMap, 0},
	{"hidecreature", GameScript::HideCreature, 0},
	{"hidegui", GameScript::HideGUI, 0},
	{"incinternal", GameScript::IncInternal, 0}, //pst
	{"incrementinternal", GameScript::IncInternal, 0},//iwd
	{"incmoraleai", GameScript::IncMoraleAI, 0},
	{"incrementchapter", GameScript::IncrementChapter, 0},
	{"incrementglobal", GameScript::IncrementGlobal,AF_MERGESTRINGS},
	{"incrementglobalonce", GameScript::IncrementGlobalOnce,AF_MERGESTRINGS},
	{"incrementextraproficiency", GameScript::IncrementExtraProficiency, 0},
	{"incrementproficiency", GameScript::IncrementProficiency, 0},
	{"interact", GameScript::Interact, 0},
	{"joinparty", GameScript::JoinParty, 0},
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
	{"movebetweenareas", GameScript::MoveBetweenAreas, 0},
	{"movebetweenareaseffect", GameScript::MoveBetweenAreas, 0},
	{"moveglobal", GameScript::MoveGlobal, 0}, 
	{"moveglobalobject", GameScript::MoveGlobalObject, 0}, 
	{"moveglobalobjectoffscreen", GameScript::MoveGlobalObjectOffScreen, 0},
	{"moveglobalsto", GameScript::MoveGlobalsTo, 0}, 
	{"transferinventory", GameScript::MoveInventory, 0}, 
	{"movetocenterofscreen", GameScript::MoveToCenterOfScreen,AF_BLOCKING},
	{"movetoobject", GameScript::MoveToObject,AF_BLOCKING},
	{"movetoobjectfollow", GameScript::MoveToObjectFollow,AF_BLOCKING},
	{"movetoobjectnointerrupt", GameScript::MoveToObjectNoInterrupt,AF_BLOCKING},
	{"movetooffset", GameScript::MoveToOffset,AF_BLOCKING},
	{"movetopoint", GameScript::MoveToPoint,AF_BLOCKING},
	{"movetopointnointerrupt", GameScript::MoveToPointNoInterrupt,AF_BLOCKING},
	{"movetopointnorecticle", GameScript::MoveToPointNoRecticle,AF_BLOCKING},//the same until we know better
	{"movetosavedlocation", GameScript::MoveToSavedLocation,AF_BLOCKING},
	//take care of the typo in the original bg2 action.ids
	{"movetosavedlocationn", GameScript::MoveToSavedLocation,AF_BLOCKING},
	{"moveviewobject", GameScript::MoveViewPoint, 0},
	{"moveviewpoint", GameScript::MoveViewPoint, 0},
	{"nidspecial1", GameScript::NIDSpecial1,AF_BLOCKING},//we use this for dialogs, hack
	{"nidspecial2", GameScript::NIDSpecial2,AF_BLOCKING},//we use this for worldmap, another hack
	{"nidspecial3", GameScript::Attack,AF_BLOCKING},     //this hack is for attacking preset target
	{"noaction", GameScript::NoAction, 0},
	{"opendoor", GameScript::OpenDoor,AF_BLOCKING},
	{"panic", GameScript::Panic, 0},
	{"permanentstatchange", GameScript::ChangeStat, 0}, //probably the same
	{"picklock", GameScript::PickLock,AF_BLOCKING},
	{"pickpockets", GameScript::PickPockets, AF_BLOCKING},
	{"pickupitem", GameScript::PickUpItem, 0}, 
	{"playdead", GameScript::PlayDead,AF_BLOCKING},
	{"playdeadinterruptable", GameScript::PlayDeadInterruptable,AF_BLOCKING},
	{"playerdialog", GameScript::PlayerDialogue,AF_BLOCKING},
	{"playerdialogue", GameScript::PlayerDialogue,AF_BLOCKING},
	{"playsequence", GameScript::PlaySequence, 0},
	{"playsequencetimed", GameScript::PlaySequenceTimed, 0},//pst
	{"playsong", GameScript::StartSong, 0},
	{"playsound", GameScript::PlaySound, 0},
	{"playsoundnotranged", GameScript::PlaySoundNotRanged, 0},
	{"playsoundpoint", GameScript::PlaySoundPoint, 0},
	{"plunder", GameScript::Plunder,AF_BLOCKING},
	{"polymorphcopy", GameScript::PolymorphCopy, 0},
	{"quitgame", GameScript::QuitGame, 0},
	{"randomfly", GameScript::RandomFly, AF_BLOCKING},
	{"randomturn", GameScript::RandomTurn, AF_BLOCKING},
	{"randomwalk", GameScript::RandomWalk, AF_BLOCKING},
	{"randomwalkcontinuous", GameScript::RandomWalkContinuous, AF_BLOCKING},
	{"realsetglobaltimer", GameScript::RealSetGlobalTimer,AF_MERGESTRINGS},
	{"recoil", GameScript::Recoil, 0},
	{"regainpaladinhood", GameScript::RegainPaladinHood, 0},
	{"regainrangerhood", GameScript::RegainRangerHood, 0},
	{"removeareaflag", GameScript::RemoveAreaFlag, 0},
	{"removeareatype", GameScript::RemoveAreaType, 0},
	{"removejournalentry", GameScript::RemoveJournalEntry, 0},
	{"removemapnote", GameScript::RemoveMapnote, 0},
	{"removepaladinhood", GameScript::RemovePaladinHood, 0},
	{"removerangerhood", GameScript::RemoveRangerHood, 0},
	{"removespell", GameScript::RemoveSpell, 0},
	{"reputationinc", GameScript::ReputationInc, 0},
	{"reputationset", GameScript::ReputationSet, 0},
	{"resetfogofwar", GameScript::UndoExplore, 0}, //pst
	{"restorepartylocations", GameScript:: RestorePartyLocation, 0},
	//this is in iwd2, same as movetosavedlocation, with a default variable
	{"returntosavedlocation", GameScript::MoveToSavedLocation, AF_BLOCKING},
	{"returntosavedlocationdelete", GameScript::MoveToSavedLocationDelete, AF_BLOCKING},
	{"returntosavedplace", GameScript::MoveToSavedLocation, AF_BLOCKING},
	{"revealareaonmap", GameScript::RevealAreaOnMap, 0},
	{"runawayfrom", GameScript::RunAwayFrom,AF_BLOCKING},
	{"runawayfromnointerrupt", GameScript::RunAwayFromNoInterrupt,AF_BLOCKING},
	{"runawayfrompoint", GameScript::RunAwayFromPoint,AF_BLOCKING},
	{"runtoobject", GameScript::MoveToObject,AF_BLOCKING}, //until we know better
	{"runtopoint", GameScript::MoveToPoint,AF_BLOCKING}, //until we know better
	{"runtopointnorecticle", GameScript::MoveToPoint,AF_BLOCKING},//until we know better
	{"runtosavedlocation", GameScript::MoveToSavedLocation,AF_BLOCKING},//
	{"savegame", GameScript::SaveGame, 0},
	{"savelocation", GameScript::SaveLocation, 0},
	{"saveplace", GameScript::SaveLocation, 0},
	{"saveobjectlocation", GameScript::SaveObjectLocation, 0},
	{"screenshake", GameScript::ScreenShake,AF_BLOCKING},
	//{"sequence", GameScript::PlaySequence, 0}, //which engine has this
	{"setanimstate", GameScript::PlaySequence, 0},//pst
	{"setapparentnamestrref", GameScript::SetApparentName, 0},
	{"setareaflags", GameScript::SetAreaFlags, 0},
	{"setarearestflag", GameScript::SetAreaRestFlag, 0},
	{"setbeeninpartyflags", GameScript::SetBeenInPartyFlags, 0},
	{"setcorpseenabled", GameScript::AmbientActivate, 0},//another weird name
	{"setcreatureareaflags", GameScript::SetCreatureAreaFlags, 0},
	{"setdialog", GameScript::SetDialogue,AF_BLOCKING},
	{"setdialogue", GameScript::SetDialogue,AF_BLOCKING},
	{"setdialoguerange", GameScript::SetDialogueRange, 0},
	{"setdoorlocked", GameScript::SetDoorLocked,AF_BLOCKING},
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
	{"sethomelocation", GameScript::SetHomeLocation, 0},
	{"sethp", GameScript::SetHP, 0},
	{"setinternal", GameScript::SetInternal, 0},
	{"setleavepartydialogfile", GameScript::SetLeavePartyDialogFile, 0},
	{"setmasterarea", GameScript::SetMasterArea, 0},
	{"setmazeeasier", GameScript::SetMazeEasier, 0}, //pst specific crap
	{"setmazeharder", GameScript::SetMazeHarder, 0}, //pst specific crap
	{"setmoraleai", GameScript::SetMoraleAI, 0},
	{"setmusic", GameScript::SetMusic, 0},
	{"setname", GameScript::SetApparentName, 0},
	{"setnamelessclass", GameScript::SetNamelessClass, 0},
	{"setnamelessdisguise", GameScript::SetNamelessDisguise, 0},
	{"setnumtimestalkedto", GameScript::SetNumTimesTalkedTo, 0},
	{"setplayersound", GameScript::SetPlayerSound, 0},
	{"setquestdone", GameScript::SetQuestDone, 0},
	{"setregularnamestrref", GameScript::SetRegularName, 0},
	{"setrestencounterchance", GameScript::SetRestEncounterChance, 0},
	{"setrestencounterprobabilityday", GameScript::SetRestEncounterProbabilityDay, 0},
	{"setrestencounterprobabilitynight", GameScript::SetRestEncounterProbabilityNight, 0},
	{"setsavedlocation", GameScript::SaveObjectLocation, 0},
	{"setsavedlocationpoint", GameScript::SaveLocation, 0},
	{"setscriptname", GameScript::SetScriptName, 0},
	{"setsequence", GameScript::PlaySequence, 0}, //bg2 (only own)
	{"setteam", GameScript::SetTeam, 0},
	{"setteambit", GameScript::SetTeamBit, 0},
	{"settextcolor", GameScript::SetTextColor, 0},
	{"settoken", GameScript::SetToken, 0},
	{"settokenglobal", GameScript::SetTokenGlobal,AF_MERGESTRINGS},
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
	{"startcutscene", GameScript::StartCutScene, 0},
	{"startcutscenemode", GameScript::StartCutSceneMode, 0},
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
	{"startmusic", GameScript::StartMusic, 0},
	{"startrainnow", GameScript::StartRainNow, 0},
	{"startsong", GameScript::StartSong, 0},
	{"startstore", GameScript::StartStore, 0},
	{"staticpalette", GameScript::StaticPalette, 0},
	{"staticsequence", GameScript::PlaySequence, 0},//bg2 animation sequence
	{"staticstart", GameScript::StaticStart, 0},
	{"staticstop", GameScript::StaticStop, 0},
	{"stickysinisterpoof", GameScript::CreateVisualEffectObject, 0},
	{"stopmoving", GameScript::StopMoving, 0},
	{"storepartylocations", GameScript::StorePartyLocation, 0},
	{"swing", GameScript::Swing, 0},
	{"swingonce", GameScript::SwingOnce, 0},
	{"takeitemlist", GameScript::TakeItemList, 0},
	{"takeitemlistparty", GameScript::TakeItemListParty, 0},
	{"takeitemreplace", GameScript::TakeItemReplace, 0},
	{"takepartygold", GameScript::TakePartyGold, 0},
	{"takepartyitem", GameScript::TakePartyItem, 0},
	{"takepartyitemall", GameScript::TakePartyItemAll, 0},
	{"takepartyitemnum", GameScript::TakePartyItemNum, 0},
	{"takepartyitemrange", GameScript::TakePartyItemRange, 0},
	{"teleportparty", GameScript::TeleportParty, 0}, 
	{"textscreen", GameScript::TextScreen, AF_BLOCKING},
	{"tomsstringdisplayer", GameScript::DisplayMessage, 0},
	{"triggeractivation", GameScript::TriggerActivation, 0},
	{"triggerwalkto", GameScript::MoveToObject,AF_BLOCKING}, //something like this
	{"turn", GameScript::Turn, 0},
	{"turnamt", GameScript::TurnAMT, AF_BLOCKING}, //relative Face()
	{"undoexplore", GameScript::UndoExplore, 0},
	{"unhidegui", GameScript::UnhideGUI, 0},
	{"unloadarea", GameScript::UnloadArea, 0},
	{"unlock", GameScript::Unlock, 0},
	{"unlockscroll", GameScript::UnlockScroll, 0},
	{"unmakeglobal", GameScript::UnMakeGlobal, 0}, //this is a GemRB extension
	{"usecontainer", GameScript::UseContainer,AF_BLOCKING},
	{"vequip",GameScript::SetArmourLevel, 0},
	{"verbalconstant", GameScript::VerbalConstant, 0},
	{"verbalconstanthead", GameScript::VerbalConstantHead, 0},
	{"wait", GameScript::Wait, AF_BLOCKING},
	{"waitanimation", GameScript::PlaySequenceTimed,AF_BLOCKING},//iwd2
	{"waitrandom", GameScript::WaitRandom, AF_BLOCKING},
	{"weather", GameScript::Weather, 0},
	{"xequipitem", GameScript::XEquipItem, 0},
 	{ NULL,NULL, 0},
};

//Make this an ordered list, so we could use bsearch!
static ObjectLink objectnames[] = {
	{"bestac", GameScript::BestAC},
	{"eighthnearest", GameScript::EighthNearest},
	{"eighthnearestenemyof", GameScript::EighthNearestEnemyOf},
	{"eighthnearestenemyoftype", GameScript::EighthNearestEnemyOfType},
	{"eighthnearestmygroupoftype", GameScript::EighthNearestEnemyOfType},
	{"eigthnearestenemyof", GameScript::EighthNearestEnemyOf}, //typo in iwd
	{"eigthnearestenemyoftype", GameScript::EighthNearestEnemyOfType}, //bg2
	{"eigthnearestmygroupoftype", GameScript::EighthNearestEnemyOfType},//bg2
	{"farthest", GameScript::Farthest},
	{"fifthnearest", GameScript::FifthNearest},
	{"fifthnearestenemyof", GameScript::FifthNearestEnemyOf},
	{"fifthnearestenemyoftype", GameScript::FifthNearestEnemyOfType},
	{"fifthnearestmygroupoftype", GameScript::FifthNearestEnemyOfType},
	{"fourthnearest", GameScript::FourthNearest},
	{"fourthnearestenemyof", GameScript::FourthNearestEnemyOf},
	{"fourthnearestenemyoftype", GameScript::FourthNearestEnemyOfType},
	{"fourthnearestmygroupoftype", GameScript::FourthNearestEnemyOfType},
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
	{"nearestmygroupoftype", GameScript::NearestMyGroupOfType},
	{"nearestpc", GameScript::NearestPC},
	{"ninthnearest", GameScript::NinthNearest},
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
	{"protagonist", GameScript::Protagonist},
	{"secondnearest", GameScript::SecondNearest},
	{"secondnearestenemyof", GameScript::SecondNearestEnemyOf},
	{"secondnearestenemyoftype", GameScript::SecondNearestEnemyOfType},
	{"secondnearestmygroupoftype", GameScript::SecondNearestMyGroupOfType},
	{"selectedcharacter", GameScript::SelectedCharacter},
	{"seventhnearest", GameScript::SeventhNearest},
	{"seventhnearestenemyof", GameScript::SeventhNearestEnemyOf},
	{"seventhnearestenemyoftype", GameScript::SeventhNearestEnemyOfType},
	{"seventhnearestmygroupoftype", GameScript::SeventhNearestMyGroupOfType},
	{"sixthnearest", GameScript::SixthNearest},
	{"sixthnearestenemyof", GameScript::SixthNearestEnemyOf},
	{"sixthnearestenemyoftype", GameScript::SixthNearestEnemyOfType},
	{"sixthnearestmygroupoftype", GameScript::SixthNearestMyGroupOfType},
	{"strongestof", GameScript::StrongestOf},
	{"strongestofmale", GameScript::StrongestOfMale},
	{"tenthnearest", GameScript::TenthNearest},
	{"tenthnearestenemyof", GameScript::TenthNearestEnemyOf},
	{"tenthnearestenemyoftype", GameScript::TenthNearestEnemyOfType},
	{"tenthnearestmygroupoftype", GameScript::TenthNearestMyGroupOfType},
	{"thirdnearest", GameScript::ThirdNearest},
	{"thirdnearestenemyof", GameScript::ThirdNearestEnemyOf},
	{"thirdnearestenemyoftype", GameScript::ThirdNearestEnemyOfType},
	{"thirdnearestmygroupoftype", GameScript::ThirdNearestMyGroupOfType},
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
	Variables* local, int ScriptLevel)
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
			if (triggers[i]) {
				printMessage("GameScript"," ", WHITE);
				printf("%s is a synonym\n", triggersTable->GetStringIndex( j ));
				continue; //we already found an alternative
			}
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
			if (actions[i]) {
				printMessage("GameScript"," ", WHITE);
				printf("%s is a synonym\n", actionsTable->GetStringIndex( j ));
				continue; //we already found an alternative
			}
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
			if (objects[i]) {
				printMessage("GameScript"," ", WHITE);
				printf("%s is a synonym\n", objectsTable->GetStringIndex( j ));
				continue;
			}
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
		if (InDebug&ID_REFERENCE) {
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
	src++;
	return atoi( number );
}

static void ParseString(const char*& src, char* tmp)
{
	while (*src != '"') {
		*tmp = *src;
		tmp++;
		src++;
	}
	*tmp = 0;
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
	if (HasAdditionalRect) {
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
	strupr(tR->string0Parameter);
	strupr(tR->string1Parameter);
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
	RandomNumValue=rand();
	for (unsigned int a = 0; a < script->responseBlocksCount; a++) {
		ResponseBlock* rB = script->responseBlocks[a];
		MySelf->InitTriggers();
		if (EvaluateCondition( MySelf, rB->condition )) {
			continueExecution = ( ExecuteResponseSet( MySelf,
						rB->responseSet ) != 0 );
			endReached = false;
			//clear triggers after response executed
			MySelf->ClearTriggers();
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
//according to research, cutscenes don't evaluate conditions, and always
//runs the first response, this is a serious cutback on possible
//functionality, so i kept the gemrb specific code too.
#ifdef GEMRB_CUTSCENES
//this is the logical way of executing a cutscene
	for (unsigned int a = 0; a < script->responseBlocksCount; a++) {
		ResponseBlock* rB = script->responseBlocks[a];
		if (EvaluateCondition( MySelf, rB->condition )) {
			ExecuteResponseSet( MySelf, rB->responseSet );
			endReached = false;
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
	if (strncmp(poi,"AC",2)==0) while (true) {
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
		strupr(aC->string0Parameter);
		strupr(aC->string1Parameter);
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
	if (Sender->CurrentAction) {
		Sender->CurrentAction->Release();
	}
	Sender->CurrentAction = act;
	ExecuteAction( Sender, act );
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
	if (Sender->LastTrigger) {
		Actor *target = Sender->GetCurrentArea()->GetActorByGlobalID(Sender->LastTrigger);
 		parameters->AddTarget(target, 0);
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
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastSeen);
		if (target) {
			parameters->AddTarget(target, 0);
		}
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
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastHelp);
		if (target) {
			parameters->AddTarget(target, 0);
		}
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
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastHeard);
		if (target) {
			parameters->AddTarget(target, 0);
		}
	}
	return parameters;
}

/*this one is tough*/
Targets *GameScript::ProtectorOf(Scriptable *Sender, Targets *parameters)
{
	Actor *actor = parameters->GetTarget(0);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastProtected);
		if (target) {
			parameters->AddTarget(target, 0);
		}
	}
	return parameters;
}

Targets *GameScript::ProtectedBy(Scriptable *Sender, Targets *parameters)
{
	Actor *actor = parameters->GetTarget(0);
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastProtected);
		if (target) {
			parameters->AddTarget(target, 0);
		}
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
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastCommander);
		if (target) {
			parameters->AddTarget(target, 0);
		}
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
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastTarget);
		if (target) {
			parameters->AddTarget(target, 0);
		}
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
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastHitter);
		if (target) {
printf("Lastattecker of %s is %s\n", actor->GetScriptName(), target->GetScriptName());
			parameters->AddTarget(target, 0);
		}
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
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastHitter);
		if (target) {
			parameters->AddTarget(target, 0);
		}
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
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastTalkedTo);
		if (target) {
			parameters->AddTarget(target, 0);
		}
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
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastSummoner);
		if (target) {
			parameters->AddTarget(target, 0);
		}
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
	parameters->AddTarget(core->GetGame()->GetPC(0,false), 0);
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
	parameters->AddTarget(core->GetGame()->GetPC(1,false), 0);
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
	parameters->AddTarget(core->GetGame()->GetPC(2,false), 0);
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
	parameters->AddTarget(core->GetGame()->GetPC(3,false), 0);
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
	parameters->AddTarget(core->GetGame()->GetPC(4,false), 0);
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
	parameters->AddTarget(core->GetGame()->GetPC(5,false), 0);
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
	parameters->AddTarget(core->GetGame()->GetPC(6,false), 0);
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
	parameters->AddTarget(core->GetGame()->GetPC(7,false), 0);
	return parameters;
}

Targets *GameScript::BestAC(Scriptable* /*Sender*/, Targets *parameters)
{
	targetlist::iterator m;
	targettype *t = parameters->GetFirstTarget(m);
	if (!t) {
		return parameters;
	}
	Actor *actor=t->actor;
	int bestac=actor->GetStat(IE_ARMORCLASS);
	// assignment in while
	while ( (t = parameters->GetNextTarget(m) ) ) {
		int ac=t->actor->GetStat(IE_ARMORCLASS);
		if (bestac<ac) {
			bestac=ac;
			actor=t->actor;
		}
	}

	parameters->Clear();
	parameters->AddTarget(actor, 0);
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
	Actor *actor = NULL;
	//assignment intentional
	while ( (t = parameters->GetNextTarget(m) ) ) {
		if (t->actor->GetStat(IE_SEX)!=1) continue;
		int hp=t->actor->GetStat(IE_HITPOINTS);
		if ((pos==-1) || (worsthp<hp)) {
			worsthp=hp;
			actor=t->actor;
		}
	}
	parameters->Clear();
	if (actor) {
		parameters->AddTarget(actor, 0);
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
	Actor *actor=t->actor;
	int besthp=actor->GetStat(IE_HITPOINTS);
	// assignment in while
	while ( (t = parameters->GetNextTarget(m) ) ) {
		int hp=t->actor->GetStat(IE_HITPOINTS);
		if (besthp<hp) {
			besthp=hp;
			actor=t->actor;
		}
	}
	parameters->Clear();
	parameters->AddTarget(actor, 0);
	return parameters;
}

Targets *GameScript::WeakestOf(Scriptable* /*Sender*/, Targets *parameters)
{
	targetlist::iterator m;
	targettype *t = parameters->GetFirstTarget(m);
	if (!t) {
		return parameters;
	}
	Actor *actor=t->actor;
	int worsthp=actor->GetStat(IE_HITPOINTS);
	// assignment in while
	while ( (t = parameters->GetNextTarget(m) ) ) {
		int hp=t->actor->GetStat(IE_HITPOINTS);
		if (worsthp>hp) {
			worsthp=hp;
			actor=t->actor;
		}
	}
	parameters->Clear();
	parameters->AddTarget(actor, 0);
	return parameters;
}

Targets *GameScript::WorstAC(Scriptable* /*Sender*/, Targets *parameters)
{
	targetlist::iterator m;
	targettype *t = parameters->GetFirstTarget(m);
	if (!t) {
		return parameters;
	}
	Actor *actor=t->actor;
	int worstac=actor->GetStat(IE_ARMORCLASS);
	// assignment in while
	while ( (t = parameters->GetNextTarget(m) ) ) {
		int ac=t->actor->GetStat(IE_ARMORCLASS);
		if (worstac>ac) {
			worstac=ac;
			actor=t->actor;
		}
	}
	parameters->Clear();
	parameters->AddTarget(actor, 0);
	return parameters;
}

Targets *GameScript::MostDamagedOf(Scriptable* /*Sender*/, Targets *parameters)
{
	targetlist::iterator m;
	targettype *t = parameters->GetFirstTarget(m);
	if (!t) {
		return parameters;
	}
	Actor *actor=t->actor;
	int worsthp=actor->GetStat(IE_MAXHITPOINTS)-actor->GetStat(IE_HITPOINTS);
	// assignment in while
	while ( (t = parameters->GetNextTarget(m) ) ) {
		int hp=t->actor->GetStat(IE_MAXHITPOINTS)-t->actor->GetStat(IE_HITPOINTS);
		if (worsthp>hp) {
			worsthp=hp;
			actor=t->actor;
		}
	}
	parameters->Clear();
	parameters->AddTarget(actor, 0);
	return parameters;
}
Targets *GameScript::LeastDamagedOf(Scriptable* /*Sender*/, Targets *parameters)
{
	targetlist::iterator m;
	targettype *t = parameters->GetFirstTarget(m);
	if (!t) {
		return parameters;
	}
	Actor *actor=t->actor;
	int besthp=actor->GetStat(IE_MAXHITPOINTS)-actor->GetStat(IE_HITPOINTS);
	// assignment in while
	while ( (t = parameters->GetNextTarget(m) ) ) {
		int hp=t->actor->GetStat(IE_MAXHITPOINTS)-t->actor->GetStat(IE_HITPOINTS);
		if (besthp<hp) {
			besthp=hp;
			actor=t->actor;
		}
	}
	parameters->Clear();
	parameters->AddTarget(actor, 0);
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

Targets *GameScript::XthNearestMyGroupOfType(Scriptable *origin, Targets *parameters, int count)
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
	if (actor->GetStat(IE_EA) <= EA_GOODCUTOFF) {
		type = 0; //PC
	}
	if (actor->GetStat(IE_EA) >= EA_EVILCUTOFF) {
		type = 1;
	}
	if (type==2) {
		parameters->Clear();
		return parameters;
	}

	while ( t ) {
		if (type) { //origin is enemy, so we remove PC groups
			if (t->actor->GetStat(IE_EA) <= EA_GOODCUTOFF) {
				t=parameters->RemoveTargetAt(m);
				continue;
			}
		}
		else {
			if (t->actor->GetStat(IE_EA) >= EA_EVILCUTOFF) {
				t=parameters->RemoveTargetAt(m);
				continue;
			}
		}
		t = parameters->GetNextTarget(m);
	}
	return XthNearestOf(parameters,count);
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
	if (actor->GetStat(IE_EA) <= EA_GOODCUTOFF) {
		type = 1; //PC
	}
	if (actor->GetStat(IE_EA) >= EA_EVILCUTOFF) {
		type = 0;
	}
	if (type==2) {
		parameters->Clear();
		return parameters;
	}

	while ( t ) {
		if (type) { //origin is PC
			if (t->actor->GetStat(IE_EA) <= EA_GOODCUTOFF) {
				t=parameters->RemoveTargetAt(m);
				continue;
			}
		}
		else {
			if (t->actor->GetStat(IE_EA) >= EA_EVILCUTOFF) {
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
	if (origin->GetStat(IE_EA) <= EA_GOODCUTOFF) {
		type = 1; //PC
	}
	if (origin->GetStat(IE_EA) >= EA_EVILCUTOFF) {
		type = 0;
	}
	if (type==2) {
		return parameters;
	}
	Map *map = origin->GetCurrentArea();
	int i = map->GetActorCount(true);
	Actor *ac;
	while (i--) {
		ac=map->GetActor(i,true);
		int distance = Distance(ac, origin);
		if (type) { //origin is PC
			if (ac->GetStat(IE_EA) >= EA_EVILCUTOFF) {
				parameters->AddTarget(ac, distance);
			}
		}
		else {
			if (ac->GetStat(IE_EA) <= EA_GOODCUTOFF) {
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

Targets *GameScript::NearestMyGroupOfType(Scriptable* Sender, Targets *parameters)
{
	return XthNearestMyGroupOfType(Sender, parameters, 0);
}

Targets *GameScript::SecondNearestMyGroupOfType(Scriptable* Sender, Targets *parameters)
{
	return XthNearestMyGroupOfType(Sender, parameters, 1);
}

Targets *GameScript::ThirdNearestMyGroupOfType(Scriptable* Sender, Targets *parameters)
{
	return XthNearestMyGroupOfType(Sender, parameters, 2);
}

Targets *GameScript::FourthNearestMyGroupOfType(Scriptable* Sender, Targets *parameters)
{
	return XthNearestMyGroupOfType(Sender, parameters, 3);
}

Targets *GameScript::FifthNearestMyGroupOfType(Scriptable* Sender, Targets *parameters)
{
	return XthNearestMyGroupOfType(Sender, parameters, 4);
}

Targets *GameScript::SixthNearestMyGroupOfType(Scriptable* Sender, Targets *parameters)
{
	return XthNearestMyGroupOfType(Sender, parameters, 5);
}

Targets *GameScript::SeventhNearestMyGroupOfType(Scriptable* Sender, Targets *parameters)
{
	return XthNearestMyGroupOfType(Sender, parameters, 6);
}

Targets *GameScript::EighthNearestMyGroupOfType(Scriptable* Sender, Targets *parameters)
{
	return XthNearestMyGroupOfType(Sender, parameters, 7);
}

Targets *GameScript::NinthNearestMyGroupOfType(Scriptable* Sender, Targets *parameters)
{
	return XthNearestMyGroupOfType(Sender, parameters, 8);
}

Targets *GameScript::TenthNearestMyGroupOfType(Scriptable* Sender, Targets *parameters)
{
	return XthNearestMyGroupOfType(Sender, parameters, 9);
}

/* returns only living PC's? if not, alter getpartysize/getpc flag*/
Targets *GameScript::NearestPC(Scriptable* Sender, Targets *parameters)
{
	parameters->Clear();
	Map *map = Sender->GetCurrentArea();
	Game *game = core->GetGame();
	int i = game->GetPartySize(true);
	int mindist = -1;
	Actor *ac = NULL;
	while (i--) {
		Actor *newactor=game->GetPC(i,true);
		if (newactor->GetCurrentArea()!=map) {
			continue;
		}
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
	int i = cm->GetActorCount(true);
	while (i--) {
		Actor *ac=cm->GetActor(i,true);
		if (ac->GetCurrentArea()!=cm) {
			continue;
		}
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
	int ret = EvaluateTrigger( Sender, tri );
	tri->Release();
	return ret;
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
				if (Sender->Active&SCR_CUTSCENEID) {
					if (Sender->CutSceneId) {
						Sender->CutSceneId->AddAction( aC );
						//AddAction( Sender->CutSceneId, aC );
					} else {
						printf("Did not find cutscene object, action ignored!\n");
					}
				} else {
					if (Sender->CutSceneId) {
						printf("Stuck with cutscene ID!\n");
						abort();
					}
					//ogres in dltc need this
					Sender->AddAction( aC );
					//this was a mistake, nothing
					//requires it, so use the code above
					//AddAction( Sender, aC );
				}
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
	if (aC->objects[0]) {
		Scriptable *scr = GetActorFromObject(Sender, aC->objects[0]);
		if (scr) {
			if (InDebug&ID_ACTIONS) {
				printMessage("GameScript"," ",YELLOW);
				printf("Sender: %s-->override: %s\n",Sender->GetScriptName(), scr->GetScriptName() );
			}      
			scr->AddAction(ParamCopyNoOverride(Sender->CurrentAction));
		} else {
			printMessage("GameScript","Actionoverride failed for object: \n",LIGHT_RED);
			aC->objects[0]->Dump();
		}
		Sender->CurrentAction=NULL;
		aC->Release();
		return;
	}
	if (InDebug&ID_ACTIONS) {
		printMessage("GameScript"," ",YELLOW);
		printf("Sender: %s\n",Sender->GetScriptName() );
	}
	ActionFunction func = actions[aC->actionID];
	if (func) {
		//turning off interruptable flag
		//uninterruptable actions will set it back
		if (Sender->Type==ST_ACTOR) {
			Sender->Active|=SCR_ACTIVE;
			((Actor *)Sender)->InternalFlags&=~IF_NOINT;
		}
		func( Sender, aC );
	} else {
		actions[aC->actionID] = NoActionAtAll;
		printMessage("GameScript", " ", YELLOW);
		printf("Unhandled action code: %d %s\n", aC->actionID , actionsTable->GetValue(aC->actionID) );
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

Action* GenerateAction(char* String, bool autoFree)
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
	return GenerateActionCore( src, str, i, autoFree);
}

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
