/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "ie_feats.h" //cannot avoid declaring these
#include "opcode_params.h"
#include "overlays.h"
#include "win32def.h"

#include "Audio.h"  //needs for a playsound call
#include "DisplayMessage.h"
#include "EffectQueue.h"
#include "Game.h"
#include "GameData.h"
#include "Interface.h"
#include "ProjectileServer.h" //needs for alter_animation
#include "Spell.h"
#include "damages.h"
#include "Scriptable/Actor.h"
#include "GameScript/GSUtils.h" //needs for displaystringcore

using namespace GemRB;

static const ieResRef SevenEyes[7]={"spin126","spin127","spin128","spin129","spin130","spin131","spin132"};

static bool enhanced_effects = false;
//a scripting object for enemy (used for enemy in line of sight check)
static Trigger *Enemy = NULL;

#define PI_PROTFROMEVIL 9
#define PI_FREEACTION   19
#define PI_BARKSKIN     20
#define PI_BANE         35
#define PI_PANIC        36
#define PI_NAUSEA       43
#define PI_HOPELESSNESS 44
#define PI_STONESKIN    46
#define PI_TENSER       55
#define PI_RIGHTEOUS    67
#define PI_ELEMENTS     76
#define PI_FAITHARMOR   84
#define PI_BLEEDING     85
#define PI_HOLYPOWER    86
#define PI_DEATHWARD    87
#define PI_UNCONSCIOUS  88
#define PI_IRONSKIN     89
#define PI_ENFEEBLEMENT 90
#define PI_ELEMPROT     93
#define PI_MINORGLOBE   96
#define PI_MAJORGLOBE   97
#define PI_SHROUD       98
#define PI_ANTIMAGIC    99
#define PI_RESILIENT    100
#define PI_MINDFLAYER   101
#define PI_CLOAKOFFEAR  102
#define PI_ENTROPY      103
#define PI_INSECT       104
#define PI_STORMSHELL   105
//#define PI_LOWERRESIST  106 //this is different in iwd2 and bg2

#define PI_AEGIS        119
#define PI_EXECUTIONER  120
#define PI_FIRESHIELD   121
#define PI_ICESHIELD    122

#define PI_TORTOISE     125

#define PI_BLINK        130

#define PI_DAYBLINDNESS 137
#define PI_HEROIC       138

#define PI_EMPTYBODY    145

//These settings are saved in the V2.2 chr struct in IWD2
#define ES_EXPERTISE    3
#define ES_POWERATTACK  4
#define ES_ARTERIAL     5
#define ES_HAMSTRING    6
#define ES_RAPIDSHOT    7

static int fx_ac_vs_damage_type_modifier_iwd2 (Scriptable* Owner, Actor* target, Effect* fx);//0
static int fx_damage_bonus_modifier (Scriptable* Owner, Actor* target, Effect* fx);//49
static int fx_draw_upon_holy_might (Scriptable* Owner, Actor* target, Effect* fx);//84 (iwd2)
static int fx_ironskins (Scriptable* Owner, Actor* target, Effect* fx);//da (iwd2)
static int fx_fade_rgb (Scriptable* Owner, Actor* target, Effect* fx);//e8
static int fx_iwd_visual_spell_hit (Scriptable* Owner, Actor* target, Effect* fx);//e9
static int fx_cold_damage (Scriptable* Owner, Actor* target, Effect* fx);//ea
//int fx_iwd_casting_glow (Scriptable* Owner, Actor* target, Effect* fx);//eb
static int fx_chill_touch (Scriptable* Owner, Actor* target, Effect* fx);//ec (how)
static int fx_chill_touch_panic (Scriptable* Owner, Actor* target, Effect* fx);//ec (iwd2)
static int fx_crushing_damage (Scriptable* Owner, Actor* target, Effect* fx);//ed
static int fx_save_bonus (Scriptable* Owner, Actor* target, Effect* fx); //ee
static int fx_slow_poison (Scriptable* Owner, Actor* target, Effect* fx); //ef
static int fx_iwd_monster_summoning (Scriptable* Owner, Actor* target, Effect* fx); //f0
static int fx_vampiric_touch (Scriptable* Owner, Actor* target, Effect* fx); //f1
// int fx_overlay f2 (same as PST)
static int fx_animate_dead (Scriptable* Owner, Actor* target, Effect* fx);//f3
static int fx_prayer (Scriptable* Owner, Actor* target, Effect* fx); //f4
static int fx_curse (Scriptable* Owner, Actor* target, Effect* fx); //f5
static int fx_summon_monster2 (Scriptable* Owner, Actor* target, Effect* fx); //f6
static int fx_burning_blood (Scriptable* Owner, Actor* target, Effect* fx); //f7 iwd
static int fx_burning_blood2 (Scriptable* Owner, Actor* target, Effect* fx); //f7 how, iwd2
static int fx_summon_shadow_monster (Scriptable* Owner, Actor* target, Effect* fx); //f8
static int fx_recitation (Scriptable* Owner, Actor* target, Effect* fx); //f9
static int fx_recitation_bad (Scriptable* Owner, Actor* target, Effect* fx); //fa
static int fx_lich_touch (Scriptable* Owner, Actor* target, Effect* fx); //fb
static int fx_blinding_orb (Scriptable* Owner, Actor* target, Effect* fx); //fc
// ac vs damage //fd
static int fx_remove_effects (Scriptable* Owner, Actor* target, Effect* fx); //fe
static int fx_salamander_aura (Scriptable* Owner, Actor* target, Effect* fx); //ff
static int fx_umberhulk_gaze (Scriptable* Owner, Actor* target, Effect* fx); //100
static int fx_zombielord_aura (Scriptable* Owner, Actor* target, Effect* fx); //101, duff in iwd2
static int fx_resist_spell (Scriptable* Owner, Actor* target, Effect* fx); //102
static int fx_summon_creature2 (Scriptable* Owner, Actor* target, Effect* fx); //103
static int fx_avatar_removal (Scriptable* Owner, Actor* target, Effect* fx); //104
//int fx_immunity_effect2 (Scriptable* Owner, Actor* target, Effect* fx); //105
static int fx_summon_pomab (Scriptable* Owner, Actor* target, Effect* fx); //106
static int fx_control_undead (Scriptable* Owner, Actor* target, Effect* fx); //107
static int fx_static_charge (Scriptable* Owner, Actor* target, Effect* fx); //108
static int fx_cloak_of_fear (Scriptable* Owner, Actor* target, Effect* fx); //109
//int fx_movement_modifier (Scriptable* Owner, Actor* target, Effect* fx); //10a
//int fx_remove_confusion (Scriptable* Owner, Actor* target, Effect* fx);//10b
static int fx_eye_of_the_mind (Scriptable* Owner, Actor* target, Effect* fx);//10c
static int fx_eye_of_the_sword (Scriptable* Owner, Actor* target, Effect* fx);//10d
static int fx_eye_of_the_mage (Scriptable* Owner, Actor* target, Effect* fx);//10e
static int fx_eye_of_venom (Scriptable* Owner, Actor* target, Effect* fx);//10f
static int fx_eye_of_the_spirit (Scriptable* Owner, Actor* target, Effect* fx);//110
static int fx_eye_of_fortitude (Scriptable* Owner, Actor* target, Effect* fx);//111
static int fx_eye_of_stone (Scriptable* Owner, Actor* target, Effect* fx);//112
static int fx_remove_seven_eyes (Scriptable* Owner, Actor* target, Effect* fx);//113
static int fx_remove_effect (Scriptable* Owner, Actor* target, Effect* fx);//114
static int fx_soul_eater (Scriptable* Owner, Actor* target, Effect* fx);//115
static int fx_shroud_of_flame (Scriptable* Owner, Actor* target, Effect* fx);//116
static int fx_shroud_of_flame2 (Scriptable* Owner, Actor* target, Effect* fx);//116
static int fx_animal_rage (Scriptable* Owner, Actor* target, Effect* fx);//117
static int fx_turn_undead2 (Scriptable* Owner, Actor* target, Effect* fx);//118
static int fx_vitriolic_sphere (Scriptable* Owner, Actor* target, Effect* fx);//119
static int fx_suppress_hp (Scriptable* Owner, Actor* target, Effect* fx);//11a
static int fx_floattext (Scriptable* Owner, Actor* target, Effect* fx);//11b
static int fx_mace_of_disruption (Scriptable* Owner, Actor* target, Effect* fx);//11c
//0x11d Sleep2 ??? power word sleep?
//0x11e Reveal:Tracks (same as bg2)
//0x11f Protection:Backstab (same as bg2)
static int fx_set_state (Scriptable* Owner, Actor* target, Effect* fx); //120
static int fx_cutscene (Scriptable* Owner, Actor* target, Effect* fx);//121
static int fx_resist_spell (Scriptable* Owner, Actor* target, Effect *fx);//ce
static int fx_resist_spell_and_message (Scriptable* Owner, Actor* target, Effect *fx);//122
static int fx_rod_of_smithing (Scriptable* Owner, Actor* target, Effect* fx); //123
//0x124 MagicalRest (same as bg2)
static int fx_beholder_dispel_magic (Scriptable* Owner, Actor* target, Effect* fx); //125
static int fx_harpy_wail (Scriptable* Owner, Actor* target, Effect* fx); //126
static int fx_jackalwere_gaze (Scriptable* Owner, Actor* target, Effect* fx); //127
//0x128 ModifyGlobalVariable (same as bg2)
//0x129 HideInShadows (same as bg2)
static int fx_use_magic_device_modifier (Scriptable* Owner, Actor* target, Effect* fx);//12a

//iwd2 related, gemrb specific effects (IE. unhardcoded hacks)
static int fx_animal_empathy_modifier (Scriptable* Owner, Actor* target, Effect* fx);//12b
static int fx_bluff_modifier (Scriptable* Owner, Actor* target, Effect* fx);//12c
static int fx_concentration_modifier (Scriptable* Owner, Actor* target, Effect* fx);//12d
static int fx_diplomacy_modifier (Scriptable* Owner, Actor* target, Effect* fx);//12e
static int fx_intimidate_modifier (Scriptable* Owner, Actor* target, Effect* fx);//12f
static int fx_search_modifier (Scriptable* Owner, Actor* target, Effect* fx);//130
static int fx_spellcraft_modifier (Scriptable* Owner, Actor* target, Effect* fx);//131
//0x132 CriticalHitModifier (same as bg2, needed for improved criticals)
static int fx_turnlevel_modifier (Scriptable* Owner, Actor* target, Effect* fx); //133

//iwd related, gemrb specific effects (IE. unhardcoded hacks)
static int fx_alter_animation (Scriptable* Owner, Actor *target, Effect* fx); //399

//iwd2 specific effects
static int fx_hopelessness (Scriptable* Owner, Actor* target, Effect* fx);//400
static int fx_protection_from_evil (Scriptable* Owner, Actor* target, Effect* fx);//401
static int fx_add_effects_list (Scriptable* Owner, Actor* target, Effect* fx);//402
static int fx_armor_of_faith (Scriptable* Owner, Actor* target, Effect* fx);//403
static int fx_nausea (Scriptable* Owner, Actor* target, Effect* fx); //404
static int fx_enfeeblement (Scriptable* Owner, Actor* target, Effect* fx); //405
static int fx_fireshield (Scriptable* Owner, Actor* target, Effect* fx); //406
static int fx_death_ward (Scriptable* Owner, Actor* target, Effect* fx); //407
static int fx_holy_power (Scriptable* Owner, Actor* target, Effect* fx); //408
static int fx_righteous_wrath (Scriptable* Owner, Actor* target, Effect* fx); //409
static int fx_summon_ally (Scriptable* Owner, Actor* target, Effect* fx); //410
static int fx_summon_enemy (Scriptable* Owner, Actor* target, Effect* fx); //411
static int fx_control (Scriptable* Owner, Actor* target, Effect* fx); //412
static int fx_visual_effect_iwd2 (Scriptable* Owner, Actor* target, Effect* fx); //413
static int fx_resilient_sphere (Scriptable* Owner, Actor* target, Effect* fx); //414
static int fx_barkskin (Scriptable* Owner, Actor* target, Effect* fx); //415
static int fx_bleeding_wounds (Scriptable* Owner, Actor* target, Effect* fx); //416
static int fx_area_effect (Scriptable* Owner, Actor* target, Effect* fx); //417
static int fx_free_action_iwd2 (Scriptable* Owner, Actor* target, Effect* fx); //418
static int fx_unconsciousness (Scriptable* Owner, Actor* target, Effect* fx); //419
//420 Death2 (same as 0xd)
static int fx_entropy_shield (Scriptable* Owner, Actor* target, Effect* fx); //421
static int fx_storm_shell (Scriptable* Owner, Actor* target, Effect* fx); //422
static int fx_protection_from_elements (Scriptable* Owner, Actor* target, Effect* fx); //423
//424 HoldUndead (same as 0x6d)
//425 ControlUndead
static int fx_aegis (Scriptable* Owner, Actor* target, Effect* fx); //426
static int fx_executioner_eyes (Scriptable* Owner, Actor* target, Effect* fx); //427
//428 DeathMagic (same as 0xd)
static int fx_effects_on_struck (Scriptable* Owner, Actor* target, Effect *fx);//429 (similar to 0xe8)
static int fx_projectile_use_effect_list (Scriptable* Owner, Actor* target, Effect* fx); //430
static int fx_energy_drain (Scriptable* Owner, Actor* target, Effect* fx); //431
static int fx_tortoise_shell (Scriptable* Owner, Actor* target, Effect* fx); //432
static int fx_blink (Scriptable* Owner, Actor* target, Effect* fx); //433
static int fx_persistent_use_effect_list (Scriptable* Owner, Actor* target, Effect* fx); //434
static int fx_day_blindness (Scriptable* Owner, Actor* target, Effect* fx); //435
static int fx_damage_reduction (Scriptable* Owner, Actor* target, Effect* fx); //436
static int fx_disguise (Scriptable* Owner, Actor* target, Effect* fx); //437
static int fx_heroic_inspiration (Scriptable* Owner, Actor* target, Effect* fx); //438
//static int fx_prevent_ai_slowdown (Scriptable* Owner, Actor* target, Effect* fx); //439
static int fx_barbarian_rage (Scriptable* Owner, Actor* target, Effect* fx); //440
//441 MovementRateModifier4
static int fx_cleave (Scriptable* Owner, Actor* target, Effect* fx); //442
static int fx_missile_damage_reduction (Scriptable* Owner, Actor* target, Effect* fx); //443
static int fx_tenser_transformation (Scriptable* Owner, Actor* target, Effect* fx); //444
static int fx_slippery_mind (Scriptable* Owner, Actor* target, Effect* fx); //445
static int fx_smite_evil (Scriptable* Owner, Actor* target, Effect* fx); //446
static int fx_restoration (Scriptable* Owner, Actor* target, Effect* fx); //447
static int fx_alicorn_lance (Scriptable* Owner, Actor* target, Effect* fx); //448
static int fx_call_lightning (Scriptable* Owner, Actor* target, Effect* fx); //449
static int fx_globe_invulnerability (Scriptable* Owner, Actor* target, Effect* fx); //450
static int fx_lower_resistance (Scriptable* Owner, Actor* target, Effect* fx); //451
static int fx_bane (Scriptable* Owner, Actor* target, Effect* fx); //452
static int fx_power_attack (Scriptable* Owner, Actor* target, Effect* fx); //453
static int fx_expertise (Scriptable* Owner, Actor* target, Effect* fx); //454
static int fx_arterial_strike (Scriptable* Owner, Actor* target, Effect* fx); //455
static int fx_hamstring (Scriptable* Owner, Actor* target, Effect* fx); //456
static int fx_rapid_shot (Scriptable* Owner, Actor* target, Effect* fx); //457

//No need to make these ordered, they will be ordered by EffectQueue
static EffectDesc effectnames[] = {
	{ "ACVsDamageTypeModifierIWD2", fx_ac_vs_damage_type_modifier_iwd2, 0, -1 }, //0
	{ "DamageBonusModifier2", fx_damage_bonus_modifier, 0, -1 }, //49
	{ "DrawUponHolyMight", fx_draw_upon_holy_might, 0, -1 },//84 (iwd2)
	{ "IronSkins", fx_ironskins, 0, -1 }, //da (iwd2)
	{ "Color:FadeRGB", fx_fade_rgb, 0, -1 }, //e8
	{ "IWDVisualSpellHit", fx_iwd_visual_spell_hit, 0, -1 }, //e9
	{ "ColdDamage", fx_cold_damage, EFFECT_DICED, -1 }, //ea
	{ "ChillTouch", fx_chill_touch, 0, -1 }, //ec (how)
	{ "ChillTouchPanic", fx_chill_touch_panic, 0, -1 }, //ec (iwd2)
	{ "CrushingDamage", fx_crushing_damage, EFFECT_DICED, -1 }, //ed
	{ "SaveBonus", fx_save_bonus, 0, -1 }, //ee
	{ "SlowPoison", fx_slow_poison, 0, -1 }, //ef
	{ "IWDMonsterSummoning", fx_iwd_monster_summoning, EFFECT_NO_ACTOR, -1 }, //f0
	{ "VampiricTouch", fx_vampiric_touch, EFFECT_DICED, -1 }, //f1
	{ "AnimateDead", fx_animate_dead, 0, -1 }, //f3
	{ "Prayer2", fx_prayer, 0, -1 }, //f4
	{ "Curse2", fx_curse, 0, -1 }, //f5
	{ "SummonMonster2", fx_summon_monster2, EFFECT_NO_ACTOR, -1 }, //f6
	{ "BurningBlood", fx_burning_blood, EFFECT_DICED, -1 }, //f7
	{ "BurningBlood2", fx_burning_blood2, EFFECT_NO_LEVEL_CHECK, -1 }, //f7
	{ "SummonShadowMonster", fx_summon_shadow_monster, EFFECT_NO_ACTOR, -1 }, //f8
	{ "Recitation", fx_recitation, 0, -1 }, //f9
	{ "RecitationBad", fx_recitation_bad, 0, -1 },//fa
	{ "LichTouch", fx_lich_touch, EFFECT_NO_LEVEL_CHECK, -1 },//fb
	{ "BlindingOrb", fx_blinding_orb, 0, -1 }, //fc
	{ "RemoveEffects", fx_remove_effects, 0, -1 }, //fe
	{ "SalamanderAura", fx_salamander_aura, 0, -1 }, //ff
	{ "UmberHulkGaze", fx_umberhulk_gaze, 0, -1 }, //100
	{ "ZombieLordAura", fx_zombielord_aura, 0, -1 },//101, duff in iwd2
	{ "SummonCreature2", fx_summon_creature2, 0, -1 }, //103
	{ "AvatarRemoval", fx_avatar_removal, 0, -1 }, //104
	{ "SummonPomab", fx_summon_pomab, 0, -1 }, //106
	{ "ControlUndead", fx_control_undead, 0, -1 }, //107
	{ "StaticCharge", fx_static_charge, EFFECT_NO_LEVEL_CHECK, -1 }, //108
	{ "CloakOfFear", fx_cloak_of_fear, 0, -1 }, //109 how/iwd2
	{ "EyeOfTheMind", fx_eye_of_the_mind, 0, -1 }, //10c
	{ "EyeOfTheSword", fx_eye_of_the_sword, 0, -1 }, //10d
	{ "EyeOfTheMage", fx_eye_of_the_mage, 0, -1 }, //10e
	{ "EyeOfVenom", fx_eye_of_venom, 0, -1 }, //10f
	{ "EyeOfTheSpirit", fx_eye_of_the_spirit, 0, -1 }, //110
	{ "EyeOfFortitude", fx_eye_of_fortitude, 0, -1 }, //111
	{ "EyeOfStone", fx_eye_of_stone, 0, -1 }, //112
	{ "RemoveSevenEyes", fx_remove_seven_eyes, 0, -1 }, //113
	{ "RemoveEffect", fx_remove_effect, 0, -1 }, //114
	{ "SoulEater", fx_soul_eater, EFFECT_NO_LEVEL_CHECK, -1 }, //115
	{ "ShroudOfFlame", fx_shroud_of_flame, 0, -1 },//116
	{ "ShroudOfFlame2", fx_shroud_of_flame2, 0, -1 },//116
	{ "AnimalRage", fx_animal_rage, 0, -1 }, //117 - berserk?
	{ "TurnUndead2", fx_turn_undead2, 0, -1 }, //118 iwd2
	{ "VitriolicSphere", fx_vitriolic_sphere, EFFECT_DICED, -1 }, //119
	{ "SuppressHP", fx_suppress_hp, 0, -1 }, //11a -- some stat???
	{ "FloatText", fx_floattext, 0, -1 }, //11b
	{ "MaceOfDisruption", fx_mace_of_disruption, 0, -1 }, //11c
	{ "State:Set", fx_set_state, 0, -1 }, //120
	{ "CutScene", fx_cutscene, EFFECT_NO_ACTOR, -1 }, //121
	{ "Protection:Spell2", fx_resist_spell, 0, -1 }, //ce
	{ "Protection:Spell3", fx_resist_spell_and_message, 0, -1 }, //122
	{ "RodOfSmithing", fx_rod_of_smithing, 0, -1 }, //123
	{ "BeholderDispelMagic", fx_beholder_dispel_magic, 0, -1 },//125
	{ "HarpyWail", fx_harpy_wail, 0, -1 }, //126
	{ "JackalWereGaze", fx_jackalwere_gaze, 0, -1 }, //127
	{ "UseMagicDeviceModifier", fx_use_magic_device_modifier, 0, -1 }, //12a
	//unhardcoded hacks for IWD2
	{ "AnimalEmpathyModifier",  fx_animal_empathy_modifier, 0, -1 },//12b
	{ "BluffModifier", fx_bluff_modifier, 0, -1 },//12c
	{ "ConcentrationModifier", fx_concentration_modifier, 0, -1 },//12d
	{ "DiplomacyModifier", fx_diplomacy_modifier, 0, -1 },//12e
	{ "IntimidateModifier", fx_intimidate_modifier, 0, -1 },//12f
	{ "SearchModifier", fx_search_modifier, 0, -1 },//130
	{ "SpellcraftModifier", fx_spellcraft_modifier, 0, -1 },//131
	{ "TurnLevelModifier", fx_turnlevel_modifier, 0, -1 },//133
	//unhardcoded hacks for IWD
	{ "AlterAnimation", fx_alter_animation, EFFECT_NO_ACTOR, -1 }, //399
	//iwd2 effects
	{ "Hopelessness", fx_hopelessness, 0, -1 }, //400
	{ "ProtectionFromEvil", fx_protection_from_evil, 0, -1 }, //401
	{ "AddEffectsList", fx_add_effects_list, 0, -1 }, //402
	{ "ArmorOfFaith", fx_armor_of_faith, 0, -1 }, //403
	{ "Nausea", fx_nausea, 0, -1 }, //404
	{ "Enfeeblement", fx_enfeeblement, 0, -1 }, //405
	{ "FireShield", fx_fireshield, 0, -1 }, //406
	{ "DeathWard", fx_death_ward, 0, -1 }, //407
	{ "HolyPower", fx_holy_power, 0, -1 }, //408
	{ "RighteousWrath", fx_righteous_wrath, 0, -1 }, //409
	{ "SummonAlly", fx_summon_ally, EFFECT_NO_ACTOR, -1 }, //410
	{ "SummonEnemy", fx_summon_enemy, EFFECT_NO_ACTOR, -1 }, //411
	{ "Control2", fx_control, 0, -1 }, //412
	{ "VisualEffectIWD2", fx_visual_effect_iwd2, 0, -1 }, //413
	{ "ResilientSphere", fx_resilient_sphere, 0, -1 }, //414
	{ "BarkSkin", fx_barkskin, 0, -1 }, //415
	{ "BleedingWounds", fx_bleeding_wounds, 0, -1 },//416
	{ "AreaEffect", fx_area_effect, EFFECT_NO_ACTOR, -1 }, //417
	{ "FreeAction2", fx_free_action_iwd2, 0, -1 }, //418
	{ "Unconsciousness", fx_unconsciousness, 0, -1 }, //419
	{ "EntropyShield", fx_entropy_shield, 0, -1 }, //421
	{ "StormShell", fx_storm_shell, 0, -1 }, //422
	{ "ProtectionFromElements", fx_protection_from_elements, 0, -1 }, //423
	{ "ControlUndead2", fx_control_undead, 0, -1 }, //425
	{ "Aegis", fx_aegis, 0, -1 }, //426
	{ "ExecutionerEyes", fx_executioner_eyes, 0, -1 }, //427
	{ "EffectsOnStruck", fx_effects_on_struck, 0, -1 }, //429
	{ "ProjectileUseEffectList", fx_projectile_use_effect_list, 0, -1 }, //430
	{ "EnergyDrain", fx_energy_drain, 0, -1 }, //431
	{ "TortoiseShell", fx_tortoise_shell, 0, -1 }, //432
	{ "Blink", fx_blink, 0, -1 },//433
	{ "PersistentUseEffectList", fx_persistent_use_effect_list, 0, -1 }, //434
	{ "DayBlindness", fx_day_blindness, 0, -1 }, //435
	{ "DamageReduction", fx_damage_reduction, 0, -1 }, //436
	{ "Disguise", fx_disguise, 0, -1 }, //437
	{ "HeroicInspiration", fx_heroic_inspiration, 0, -1 },//438
	//{ "PreventAISlowDown", fx_prevent_ai_slowdown, 0, -1 }, //439 same as bg2
	{ "BarbarianRage", fx_barbarian_rage, 0, -1 }, //440
	{ "Cleave", fx_cleave, 0, -1 }, //442
	{ "MissileDamageReduction", fx_missile_damage_reduction, 0, -1 }, //443
	{ "TensersTransformation", fx_tenser_transformation, 0, -1 }, //444
	{ "SlipperyMind", fx_slippery_mind, 0, -1 }, //445
	{ "SmiteEvil", fx_smite_evil, 0, -1 }, //446
	{ "Restoration", fx_restoration, 0, -1 }, //447
	{ "AlicornLance", fx_alicorn_lance, 0, -1 }, //448
	{ "CallLightning", fx_call_lightning, 0, -1 }, //449
	{ "GlobeInvulnerability", fx_globe_invulnerability, 0, -1 }, //450
	{ "LowerResistance", fx_lower_resistance, 0, -1 }, //451
	{ "Bane", fx_bane, 0, -1 }, //452
	{ "PowerAttack", fx_power_attack, 0, -1 }, //453
	{ "Expertise", fx_expertise, 0, -1 }, //454
	{ "ArterialStrike", fx_arterial_strike, 0, -1 }, //455
	{ "HamString", fx_hamstring, 0, -1 }, //456
	{ "RapidShot", fx_rapid_shot, 0, -1 }, //457
	{ NULL, NULL, 0, 0 },
};

//effect refs used in this module
static EffectRef fx_charm_ref = { "Charm", -1 }; //5
static EffectRef fx_cha_ref = { "CharismaModifier", -1 }; //0x6
static EffectRef fx_con_ref = { "ConstitutionModifier", -1 }; //0xa
static EffectRef fx_damage_opcode_ref = { "Damage", -1 }; //0xc
static EffectRef fx_death_ref = { "Death", -1 }; //0xd
static EffectRef fx_dex_ref = { "DexterityModifier", -1 }; //0xf
static EffectRef fx_int_ref = { "IntelligenceModifier", -1 }; //0x13
static EffectRef fx_poison_ref = { "Poison", -1 }; //0x19
static EffectRef fx_unconscious_state_ref = { "State:Helpless", -1 }; //0x27
static EffectRef fx_str_ref = { "StrengthModifier", -1 }; //0x2c
static EffectRef fx_wis_ref = { "WisdomModifier", -1 }; //0x31
static EffectRef fx_state_blind_ref = { "State:Blind", -1 }; //0x4a
static EffectRef fx_disease_ref = { "State:Diseased", -1 }; //0x4e
static EffectRef fx_confusion_ref = { "State:Confused", -1 }; //0x80
static EffectRef fx_bless_ref = { "BlessNonCumulative", -1 }; //0x82
static EffectRef fx_fear_ref = { "State:Panic", -1 }; //0xa1
static EffectRef fx_hold_creature_ref = { "State:Hold", -1 }; //0xaf
static EffectRef fx_resist_spell_ref = { "Protection:Spell2", -1 }; //0xce (IWD2)
static EffectRef fx_iwd_visual_spell_hit_ref = { "IWDVisualSpellHit", -1 }; //0xe9
static EffectRef fx_umberhulk_gaze_ref = { "UmberHulkGaze", -1 }; //0x100
static EffectRef fx_protection_from_evil_ref = { "ProtectionFromEvil", -1 }; //401
static EffectRef fx_wound_ref = { "BleedingWounds", -1 }; //416


struct IWDIDSEntry {
	ieDword value;
	ieWord stat;
	ieWord relation;
};

static int spellrescnt = -1;
static IWDIDSEntry *spellres = NULL;
static Variables tables;

static void Cleanup()
{
	if (Enemy) {
		delete Enemy;
	}
	Enemy=NULL;

	if (spellres) {
		free (spellres);
	}
}

void RegisterIWDOpcodes()
{
	core->RegisterOpcodes( sizeof( effectnames ) / sizeof( EffectDesc ) - 1, effectnames );
	enhanced_effects=!!core->HasFeature(GF_ENHANCED_EFFECTS);
	//create enemy trigger object for enemy in line of sight check
	if (!Enemy) {
		Enemy = new Trigger;
		Object *o = new Object;
		Enemy->objectParameter = o;
		o->objectFields[0]=EA_ENEMY;
	}
}

//iwd got a weird targeting system
//the opcode parameters are:
//param1 - optional value used only rarely
//param2 - a specific condition mostly based on target's stats
//this is partly superior, partly inferior to the bioware
//ids targeting.
//superior because it can handle other stats and conditions
//inferior because it is not so readily moddable
//The hardcoded conditions are simulated via the IWDIDSEntry
//structure.
//stat is usually a stat, but for special conditions it is a
//function code (>=0x100).
//If value is -1, then GemRB will use Param1, otherwise it is
//compared to the target's stat using the relation function.
//The relation function is exactly the same as the extended
//diffmode for gemrb. (Thus scripts can use the very same relation
//functions).

static void ReadSpellProtTable(const ieResRef tablename)
{
	if (spellres) {
		free(spellres);
	}
	spellres = NULL;
	spellrescnt = 0;
	AutoTable tab(tablename);
	if (!tab) {
		return;
	}
	spellrescnt=tab->GetRowCount();
	spellres = (IWDIDSEntry *) malloc(sizeof(IWDIDSEntry) * spellrescnt);
	if (!spellres) {
		return;
	}
	for( int i=0;i<spellrescnt;i++) {
		ieDword stat = core->TranslateStat(tab->QueryField(i,0) );
		spellres[i].stat = (ieWord) stat;

		spellres[i].value = (ieDword) strtol(tab->QueryField(i,1),NULL,0 );
		spellres[i].relation = (ieWord) strtol(tab->QueryField(i,2),NULL,0 );
	}
}

//unusual types which need hacking (fake stats)
#define STI_SOURCE_TARGET     0x100
#define STI_SOURCE_NOT_TARGET 0x101
#define STI_CIRCLESIZE        0x102
#define STI_TWO_ROWS          0x103
#define STI_NOT_TWO_ROWS      0x104
#define STI_MORAL_ALIGNMENT   0x105
#define STI_AREATYPE          0x106
#define STI_DAYTIME           0x107
#define STI_EA                0x108
#define STI_EVASION           0x109
#define STI_INVALID           0xffff

//returns true if iwd ids targeting resists the spell
//FIXME: the constant return values do not match the rest
static int check_iwd_targeting(Scriptable* Owner, Actor* target, ieDword value, ieDword type)
{
	if (spellrescnt==-1) {
		ReadSpellProtTable("splprot");
	}
	if (type>=(ieDword) spellrescnt) {
		return 0; //not resisted
	}

	ieDword idx = spellres[type].stat;
	ieDword val = spellres[type].value;
	ieDword rel = spellres[type].relation;
	//if IDS value is 'anything' then the supplied value is in Parameter1
	if (val==0xffffffff) {
		val = value;
	}
	switch (idx) {
	case STI_INVALID:
		return 0;
	case STI_EA:
		return DiffCore(EARelation(Owner, target), val, rel);
	case STI_DAYTIME:
		{
			ieDword timeofday = core->GetGame()->GameTime%7200/3600;
			return timeofday>= val && timeofday<= rel;
		}
	case STI_AREATYPE:
		return DiffCore((ieDword) target->GetCurrentArea()->AreaType, val, rel);
	case STI_MORAL_ALIGNMENT:
		if(Owner && Owner->Type==ST_ACTOR) {
			return DiffCore( ((Actor *) Owner)->GetStat(IE_ALIGNMENT)&AL_GE_MASK,STAT_GET(IE_ALIGNMENT)&AL_GE_MASK, rel);
		} else {
			return DiffCore(AL_TRUE_NEUTRAL,STAT_GET(IE_ALIGNMENT)&AL_GE_MASK, rel);
		}
	case STI_TWO_ROWS:
		//used in checks where any of two matches are ok (golem or undead etc)
		if (check_iwd_targeting(Owner, target, value, idx)) return 1;
		if (check_iwd_targeting(Owner, target, value, val)) return 1;
		return 0;
	case STI_NOT_TWO_ROWS:
		//this should be the opposite as above
		if (check_iwd_targeting(Owner, target, value, idx)) return 0;
		if (check_iwd_targeting(Owner, target, value, val)) return 0;
		return 1;
	case STI_SOURCE_TARGET:
		if (Owner==target) {
			return 0;
		}
		return 1;
	case STI_SOURCE_NOT_TARGET:
		if (Owner!=target) {
			return 0;
		}
		return 1;
	case STI_CIRCLESIZE:
		return DiffCore((ieDword) target->GetAnims()->GetCircleSize(), val, rel);
	case STI_EVASION:
		if (target->GetThiefLevel() < 7 ) {
			return 0;
		}
		val = target->GetSavingThrow(1,0); //breath
		if (val) {
			return 1;
		}
		return 0;
	default:
		{
			//subraces are not stand alone stats, actually, this hack should affect the CheckStat action too
			ieDword stat = STAT_GET(idx);
			if (idx==IE_SUBRACE) {
				stat |= STAT_GET(IE_RACE)<<16;
			}
			return DiffCore(stat, val, rel);
		}
	}
}

//iwd got a hardcoded 'fireshield' system
//this effect applies damage on ALL nearby actors, except the center
//also in IWD2, a dragon with this aura
//would probably not hurt anyone, because it is not using personaldistance
//but a short range area projectile

static void ApplyDamageNearby(Scriptable* Owner, Actor* target, Effect *fx, ieDword damagetype)
{
	Effect *newfx = EffectQueue::CreateEffect(fx_damage_opcode_ref, fx->Parameter1, damagetype<<16, FX_DURATION_INSTANT_PERMANENT);
	newfx->Target = FX_TARGET_PRESET;
	newfx->Power = fx->Power;
	newfx->DiceThrown = fx->DiceThrown;
	newfx->DiceSides = fx->DiceSides;
	memcpy(newfx->Resource, fx->Resource,sizeof(newfx->Resource) );
	//applyeffectcopy on everyone near us
	Map *area = target->GetCurrentArea();
	int i = area->GetActorCount(true);
	while(i--) {
		Actor *victim = area->GetActor(i,true);
		//not sure if this is needed
		if (target==victim) continue;
		if (PersonalDistance(target, victim)<20) {
			//this function deletes newfx (not anymore)
			core->ApplyEffect(newfx, victim, Owner);
		}
	}
	//finally remove the master copy
	delete newfx;
}

//this function implements AC bonus handling
//ReverseToHit is the 2nd ed way of AC (lower is better)
static inline void HandleBonus(Actor *target, int stat, int mod, int mode)
{
	if (mode==FX_DURATION_INSTANT_PERMANENT) {
		if (target->IsReverseToHit()) {
			BASE_SUB( stat, mod );
		} else {
			BASE_ADD( stat, mod );
		}
		return;
	}
	if (target->IsReverseToHit()) {
		STAT_SUB( stat, mod );
	} else {
		STAT_ADD( stat, mod );
	}
}

// fx_ac_vs_damage_type_modifier_iwd2
// the major difference from bg2 are the different type values and more AC types
int fx_ac_vs_damage_type_modifier_iwd2 (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_ac_vs_damage_type_modifier_iwd2(%2d): AC Modif: %d ; Type: %d ; MinLevel: %d ; MaxLevel: %d", fx->Opcode, fx->Parameter1, fx->Parameter2,(int) fx->DiceSides,(int) fx->DiceThrown);

	// it is a bitmask
	int type = fx->Parameter2;
	//the original engine did work with the combination of these bits
	//but since it crashed, we are not bound to the same rules
	switch(type)
	{
	case 0: //generic
		target->AC.HandleFxBonus(fx->Parameter1, fx->TimingMode==FX_DURATION_INSTANT_PERMANENT);
		break;
	case 1: //armor
		target->AC.SetArmorBonus(fx->Parameter1, 0);
		break;
	case 2: //deflection
		target->AC.SetDeflectionBonus(fx->Parameter1, 0);
		break;
	case 3: //shield
		target->AC.SetShieldBonus(fx->Parameter1, 0);
		break;
	case 4: // crushing
		HandleBonus(target, IE_ACCRUSHINGMOD, fx->Parameter1, fx->TimingMode);
		break;
	case 5: // piercing
		HandleBonus(target, IE_ACPIERCINGMOD, fx->Parameter1, fx->TimingMode);
		break;
	case 6: // slashing
		HandleBonus(target, IE_ACSLASHINGMOD, fx->Parameter1, fx->TimingMode);
		break;
	case 7: // missile
		HandleBonus(target, IE_ACMISSILEMOD, fx->Parameter1, fx->TimingMode);
		break;
	}

	return FX_PERMANENT;
}

// 0x49 DamageBonusModifier
// iwd/iwd2 supports different damage types, but only flat and percentage boni
// only the special type of 0 means a flat bonus
int fx_damage_bonus_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_damage_bonus_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	switch (fx->Parameter2) {
		case 0:
			STAT_MOD(IE_DAMAGEBONUS);
			break;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			// no stat to save to, so we handle it when dealing damage
			break;
		default:
			return FX_NOT_APPLIED;
	}
	return FX_APPLIED;
}

// 0x84 DrawUponHolyMight
// this effect differs from bg2 because it doesn't use the actor state field
// it uses the spell state field
// in bg2 the effect is called: HolyNonCumulative
int fx_draw_upon_holy_might (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_draw_upon_holy_might(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (target->SetSpellState( SS_HOLYMIGHT)) return FX_NOT_APPLIED;
	STAT_ADD( IE_STR, fx->Parameter1);
	STAT_ADD( IE_CON, fx->Parameter1);
	STAT_ADD( IE_DEX, fx->Parameter1);
	return FX_APPLIED;
}

//0xda IronSkins (iwd2)
//This is about damage reduction, not full stoneskin like in bg2
//this effect has no level check in original, but I think, it is a bug
int fx_ironskins (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	ieDword tmp;

	if (fx->Parameter2) {
		//ironskins
		tmp = STAT_GET(IE_STONESKINS);
		if (fx->Parameter1>tmp) {
			STAT_SET(IE_STONESKINS, fx->Parameter1);
		}
		target->SetSpellState( SS_IRONSKIN);
		target->AddPortraitIcon(PI_IRONSKIN);
		return FX_APPLIED;
	}

	//stoneskins (iwd2)
	if (fx->FirstApply) {
		tmp=fx->CasterLevel*10;
		if (tmp>150) tmp=150;
		fx->Parameter3=tmp;
	}
	if (!fx->Parameter3) {
		return FX_NOT_APPLIED;
	}

	if (target->SetSpellState( SS_STONESKIN)) return FX_NOT_APPLIED;
	target->SetGradient(14);
	target->AddPortraitIcon(PI_STONESKIN);
	return FX_APPLIED;
}

//0xe8 Colour:FadeRGB
int fx_fade_rgb (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_fade_rgb(%2d): RGB:%x", fx->Opcode, fx->Parameter1);

	int speed = (fx->Parameter2 >> 16) & 0xFF;
	target->SetColorMod(0xff, RGBModifier::ADD, speed,
				fx->Parameter1 >> 8, fx->Parameter1 >> 16,
				fx->Parameter1 >> 24, speed);

	return FX_NOT_APPLIED;
}

//0xe9 IWDVisualSpellHit
int fx_iwd_visual_spell_hit (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_iwd_visual_spell_hit(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	if (!Owner) {
		return FX_NOT_APPLIED;
	}
	//remove effect if there is no current area
	Map *map = Owner->GetCurrentArea();
	if (!map) {
		return FX_NOT_APPLIED;
	}
	Point pos(fx->PosX,fx->PosY);
	Projectile *pro = core->GetProjectileServer()->GetProjectileByIndex(0x1001+fx->Parameter2);
	pro->SetCaster(fx->CasterID, fx->CasterLevel);
	if (target) {
		//i believe the spell hit projectiles don't follow anyone
		map->AddProjectile( pro, pos, target->GetGlobalID(), true);
	} else {
		map->AddProjectile( pro, pos, pos);
	}
	return FX_NOT_APPLIED;
}

//0xea ColdDamage (how)
int fx_cold_damage (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_cold_damage(%2d): Damage %d", fx->Opcode, fx->Parameter1);
	target->Damage(fx->Parameter1, DAMAGE_COLD, Owner, fx->IsVariable, fx->SavingThrowType);
	return FX_NOT_APPLIED;
}

//0xeb CastingGlow2 will be same as original casting glow (iwd2 does the same)

//0xec ChillTouch (how)
//this effect is to simulate the composite effects of chill touch
//it is the usual iwd/how style hack
int fx_chill_touch (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_chill_touch(%2d)", fx->Opcode);
	target->Damage(fx->Parameter1, DAMAGE_COLD, Owner, fx->IsVariable, fx->SavingThrowType);
	if (STAT_GET(IE_GENERAL)==GEN_UNDEAD) {
		target->Panic(Owner, PANIC_RUNAWAY);
	}
	return FX_NOT_APPLIED;
}

//0xec ChillTouchPanic (iwd2)
//the undead check is made by IDS targeting as it should be
int fx_chill_touch_panic (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_chill_touch_panic(%2d)", fx->Opcode);
	ieDword state;

	if (fx->Parameter2) {
		state = STATE_HELPLESS|STATE_STUNNED;
	}
	else {
		state = STATE_PANIC;
	}
	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_STATE_SET(state);
	} else {
		STATE_SET(state);
	}
	if (enhanced_effects) {
		target->AddPortraitIcon(PI_PANIC);
	}
	return FX_PERMANENT;
}

//0xed CrushingDamage (how)
int fx_crushing_damage (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_crushing_damage(%2d): Damage %d", fx->Opcode, fx->Parameter1);
	target->Damage(fx->Parameter1, DAMAGE_CRUSHING, Owner, fx->IsVariable, fx->SavingThrowType);
	return FX_NOT_APPLIED;
}

//0xee SaveBonus
int fx_save_bonus (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_save_bonus(%2d): Bonus %d Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	STAT_MOD( IE_SAVEVSDEATH );
	STAT_MOD( IE_SAVEVSWANDS );
	STAT_MOD( IE_SAVEVSPOLY );
	STAT_MOD( IE_SAVEVSBREATH );
	STAT_MOD( IE_SAVEVSSPELL );
	return FX_APPLIED;
}

//0xef SlowPoison
//gemrb extension: can slow bleeding wounds (like bandage)

int fx_slow_poison (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	ieDword my_opcode;

	if (fx->Parameter2) my_opcode = EffectQueue::ResolveEffect(fx_wound_ref);
	else my_opcode = EffectQueue::ResolveEffect(fx_poison_ref);
	if(0) print("fx_slow_poison(%2d): Damage %d", fx->Opcode, fx->Parameter1);
	std::list< Effect* >::const_iterator f=target->fxqueue.GetFirstEffect();
	Effect *poison;
	//this is intentionally an assignment
	while( (poison = target->fxqueue.GetNextEffect(f)) ) {
		if (poison->Opcode!=my_opcode) continue;
		switch (poison->Parameter2) {
		case RPD_SECONDS:
			poison->Parameter2=RPD_ROUNDS;
			break;
		case RPD_POINTS:
			//i'm not sure if this is ok
			//the hardcoded formula is supposed to be this:
			//duration = (duration - gametime)*7+gametime;
			//duration = duration*7 - gametime*6;
			//but in fact it is like this.
			//it is (duration - gametime)*8+gametime;
			//like the damage is spread for a longer time than
			//it should
			poison->Duration=poison->Duration*8-core->GetGame()->GameTime*7;
			poison->Parameter1*=7;
			break;
		case RPD_ROUNDS:
			poison->Parameter2=RPD_TURNS;
			break;
		}
	}
	return FX_NOT_APPLIED;
}

#define IWD_MSC 13

//this requires the FXOpcode package
ieResRef iwd_monster_2da[IWD_MSC]={"MSUMMO1","MSUMMO2","MSUMMO3","MSUMMO4",
 "MSUMMO5","MSUMMO6","MSUMMO7","ASUMMO1","ASUMMO2","ASUMMO3","CDOOM","GINSECT",
 "MSUMMOM"};

//0xf0 IWDMonsterSummoning
int fx_iwd_monster_summoning (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_iwd_monster_summoning(%2d): ResRef:%s Anim:%s Type: %d", fx->Opcode, fx->Resource, fx->Resource2, fx->Parameter2);

	//check the summoning limit?

	ieResRef monster;
	ieResRef hit;
	ieResRef areahit;

	if (fx->Parameter2>=IWD_MSC) {
		fx->Parameter2 = 0;
	}
	core->GetResRefFrom2DA(iwd_monster_2da[fx->Parameter2], monster, hit, areahit);

	//the monster should appear near the effect position
	Point p(fx->PosX, fx->PosY);
	Effect *newfx = EffectQueue::CreateUnsummonEffect(fx);
	core->SummonCreature(monster, areahit, Owner, target, p, EAM_SOURCEALLY, fx->Parameter1, newfx);
	delete newfx;
	return FX_NOT_APPLIED;
}

//0xf1 VampiricTouch
int fx_vampiric_touch (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_vampiric_touch(%2d): ResRef:%s Type: %d", fx->Opcode, fx->Resource, fx->Parameter2);
	if (Owner->Type!=ST_ACTOR) {
		return FX_NOT_APPLIED;
	}

	Actor *owner = (Actor *) Owner;

	if (owner==target) {
		return FX_NOT_APPLIED;
	}

	Actor *receiver;
	Actor *donor;

	switch(fx->Parameter2) {
		case 0: receiver = target; donor = owner; break;
		case 1: receiver = owner; donor = target; break;
		default:
			return FX_NOT_APPLIED;
	}
	int damage = donor->Damage(fx->Parameter1, fx->Parameter2, owner, fx->IsVariable, fx->SavingThrowType);
	receiver->SetBase( IE_HITPOINTS, BASE_GET( IE_HITPOINTS ) + ( damage ) );
	return FX_NOT_APPLIED;
}

#define IWD_AD 2
ieResRef animate_dead_2da[IWD_AD]={"ADEAD","ADEADL"};

//0xf3 AnimateDead
int fx_animate_dead (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_animate_dead(%2d): ResRef:%s Type: %d", fx->Opcode, fx->Resource, fx->Parameter2);
	//check the summoning limit?
	if (!target) {
		return FX_NOT_APPLIED;
	}

	if (!target->GetCurrentArea()) {
		return FX_APPLIED;
	}

	ieResRef monster;
	ieResRef hit;
	ieResRef areahit;

	if (fx->Parameter2>=IWD_AD) {
		fx->Parameter2 = 0;
	}
	core->GetResRefFrom2DA(animate_dead_2da[fx->Parameter2], monster, hit, areahit);

	//the monster should appear near the effect position
	Point p(fx->PosX, fx->PosY);
	Effect *newfx = EffectQueue::CreateUnsummonEffect(fx);
	core->SummonCreature(monster, areahit, Owner, target, p, EAM_SOURCEALLY, fx->Parameter1, newfx);
	delete newfx;
	return FX_NOT_APPLIED;
}
//f4 Prayer
int fx_prayer (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_prayer(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	ieDword value;

	if (fx->Parameter2)
	{
		if (target->SetSpellState(SS_BADPRAYER)) return FX_NOT_APPLIED;
		EXTSTATE_SET(EXTSTATE_PRAYER_BAD);
		value = (ieDword) -1;
	}
	else
	{
		if (target->SetSpellState(SS_GOODPRAYER)) return FX_NOT_APPLIED;
		EXTSTATE_SET(EXTSTATE_PRAYER);
		value = 1;
	}

	target->ToHit.HandleFxBonus(value, fx->TimingMode==FX_DURATION_INSTANT_PERMANENT);
	STAT_ADD( IE_SAVEFORTITUDE, value);
	STAT_ADD( IE_SAVEREFLEX, value);
	STAT_ADD( IE_SAVEWILL, value);
	//make it compatible with 2nd edition
	STAT_ADD( IE_SAVEVSBREATH, value);
	STAT_ADD( IE_SAVEVSSPELL, value);
	return FX_APPLIED;
}
//0xf5
int fx_curse (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_curse(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	if (target->SetSpellState(SS_BADPRAYER)) return FX_NOT_APPLIED;
	EXTSTATE_SET(EXTSTATE_PRAYER_BAD);
	target->ToHit.HandleFxBonus(-1, fx->TimingMode==FX_DURATION_INSTANT_PERMANENT);
	STAT_ADD( IE_SAVEFORTITUDE, -1);
	STAT_ADD( IE_SAVEREFLEX, -1);
	STAT_ADD( IE_SAVEWILL, -1);
	//make it compatible with 2nd edition
	STAT_ADD( IE_SAVEVSBREATH, -1);
	STAT_ADD( IE_SAVEVSSPELL, -1);
	return FX_APPLIED;
}

//0xf6 SummonMonster2
#define IWD_SM2 11
ieResRef summon_monster_2da[IWD_SM2]={"SLIZARD","STROLLS","SSHADOW","ISTALKE",
 "CFELEMW","CEELEMW","CWELEMW","CFELEMP","CEELEMP","CWELEMP","CEELEMM"};

int fx_summon_monster2 (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_summon_monster2(%2d): ResRef:%s Type: %d", fx->Opcode, fx->Resource, fx->Parameter2);

	ieResRef monster;
	ieResRef hit;
	ieResRef areahit;

	if (fx->Parameter2>=IWD_SM2) {
		fx->Parameter2 = 0;
	}
	core->GetResRefFrom2DA(summon_monster_2da[fx->Parameter2], monster, hit, areahit);

	//the monster should appear near the effect position
	Point p(fx->PosX, fx->PosY);
	Effect *newfx = EffectQueue::CreateUnsummonEffect(fx);
	core->SummonCreature(monster, areahit, Owner, target, p, EAM_SOURCEALLY, fx->Parameter1, newfx);
	delete newfx;
	return FX_NOT_APPLIED;
}

//0xf7 BurningBlood (iwd)
int fx_burning_blood (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_burning_blood(%2d): Type: %d", fx->Opcode, fx->Parameter2);

	//if the target is dead, this effect ceases to exist
	if (STATE_GET(STATE_DEAD|STATE_PETRIFIED|STATE_FROZEN) ) {
		return FX_NOT_APPLIED;
	}

	//inflicts damage calculated by dice values+parameter1
	//creates damage opcode on everyone around. fx->Parameter2 - 0 fire, 1 - ice
	ieDword damage = DAMAGE_FIRE;

	if (fx->Parameter2==1) {
		damage = DAMAGE_COLD;
	}

	target->Damage(fx->Parameter1, damage, Owner, fx->IsVariable, fx->SavingThrowType);
	STAT_SET(IE_CHECKFORBERSERK,1);
	return FX_NOT_APPLIED;
}
//0xf7 BurningBlood2 (how, iwd2)
int fx_burning_blood2 (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_burning_blood2(%2d): Count: %d Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	//if the target is dead, this effect ceases to exist
	if (STATE_GET(STATE_DEAD|STATE_PETRIFIED|STATE_FROZEN) ) {
		return FX_NOT_APPLIED;
	}

	//timing
	if (core->GetGame()->GameTime%6) {
		return FX_APPLIED;
	}

	if (!fx->Parameter1) {
		return FX_NOT_APPLIED;
	}
	fx->Parameter1--;

	ieDword damage = DAMAGE_FIRE;

	if (fx->Parameter2==1) {
		damage = DAMAGE_COLD;
	}

	//this effect doesn't use Parameter1 to modify damage, it is a counter instead
	target->Damage(DICE_ROLL(0), damage, Owner, fx->IsVariable, fx->SavingThrowType);
	STAT_SET(IE_CHECKFORBERSERK,1);
	return FX_APPLIED;
}

//0xf8 SummonShadowMonster

#define IWD_SSM 3
ieResRef summon_shadow_monster_2da[IWD_SM2]={"SMONSTE","DSMONST","SHADES" };

int fx_summon_shadow_monster (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_summon_shadow_monster(%2d): ResRef:%s Type: %d", fx->Opcode, fx->Resource, fx->Parameter2);

	ieResRef monster;
	ieResRef hit;
	ieResRef areahit;

	if (fx->Parameter2>=IWD_SSM) {
		fx->Parameter2 = 0;
	}
	core->GetResRefFrom2DA(summon_shadow_monster_2da[fx->Parameter2], monster, hit, areahit);

	//the monster should appear near the effect position
	Point p(fx->PosX, fx->PosY);
	Effect *newfx = EffectQueue::CreateUnsummonEffect(fx);
	core->SummonCreature(monster, areahit, Owner, target, p, EAM_SOURCEALLY, fx->Parameter1, newfx);
	delete newfx;
	return FX_NOT_APPLIED;
}
//0xf9 Recitation
int fx_recitation (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_recitation(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	ieDword value;

	if (fx->Parameter2)
	{
		if (target->SetSpellState(SS_BADRECIT)) return FX_NOT_APPLIED;
		EXTSTATE_SET(EXTSTATE_REC_BAD);
		value = (ieDword) -2;
	}
	else
	{
		if (target->SetSpellState(SS_GOODRECIT)) return FX_NOT_APPLIED;
		EXTSTATE_SET(EXTSTATE_RECITATION);
		value = 2;
	}

	target->ToHit.HandleFxBonus(value, fx->TimingMode==FX_DURATION_INSTANT_PERMANENT);
	STAT_ADD( IE_SAVEFORTITUDE, value);
	STAT_ADD( IE_SAVEREFLEX, value);
	STAT_ADD( IE_SAVEWILL, value);
	//make it compatible with 2nd edition
	STAT_ADD( IE_SAVEVSBREATH, value);
	STAT_ADD( IE_SAVEVSSPELL, value);
	return FX_APPLIED;
}
//0xfa RecitationBad
int fx_recitation_bad (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_recitation(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	if (target->SetSpellState(SS_BADRECIT)) return FX_NOT_APPLIED;
	EXTSTATE_SET(EXTSTATE_REC_BAD);
	target->ToHit.HandleFxBonus(-2, fx->TimingMode==FX_DURATION_INSTANT_PERMANENT);
	STAT_ADD( IE_SAVEFORTITUDE, -2);
	STAT_ADD( IE_SAVEREFLEX, -2);
	STAT_ADD( IE_SAVEWILL, -2);
	//make it compatible with 2nd edition
	STAT_ADD( IE_SAVEVSBREATH, -2);
	STAT_ADD( IE_SAVEVSSPELL, -2);
	return FX_APPLIED;
}
//0xfb LichTouch (how)
//0xfb State:Hold4 (iwd2)

int fx_lich_touch (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_lich_touch(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	if (STAT_GET(IE_GENERAL)==GEN_UNDEAD) {
		return FX_NOT_APPLIED;
	}
	target->Damage(DICE_ROLL(0), DAMAGE_COLD, Owner, fx->IsVariable, fx->SavingThrowType );
	///convert to hold creature
	///shall we check for immunity vs. #175?
	///if yes, then probably it is easier to apply the hold effect instead of converting to it
	fx->Opcode = EffectQueue::ResolveEffect(fx_hold_creature_ref);
	fx->Duration = fx->Parameter1;
	fx->TimingMode = FX_DURATION_INSTANT_LIMITED;
	ieDword GameTime = core->GetGame()->GameTime;
	PrepareDuration(fx);
	return FX_APPLIED;
}

//0xfc BlindingOrb (how)

int fx_blinding_orb (Scriptable* Owner, Actor* target, Effect* fx)
{
	ieDword damage = fx->Parameter1;

	//original code checks race: 0x6c, 0x73, 0xa7
	if (STAT_GET(IE_GENERAL)==GEN_UNDEAD) {
		damage *= 2;
	}
	//check saving throw
	bool st = target->GetSavingThrow(0,0); //spell
	if (st) {
		target->Damage(damage/2, DAMAGE_FIRE, Owner, fx->IsVariable, fx->SavingThrowType);
		return FX_NOT_APPLIED;
	}
	target->Damage(damage, DAMAGE_FIRE, Owner, fx->IsVariable, fx->SavingThrowType);

	//convert effect to a blind effect.
	fx->Opcode = EffectQueue::ResolveEffect(fx_state_blind_ref);
	fx->Duration = core->Roll(1,6,0);
	fx->TimingMode = FX_DURATION_INSTANT_LIMITED;
	ieDword GameTime = core->GetGame()->GameTime;
	PrepareDuration(fx);
	return FX_APPLIED;
}

//0xfe RemoveEffects
int fx_remove_effects (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_remove_effects(%2d): ResRef:%s Type: %d", fx->Opcode, fx->Resource, fx->Parameter2);

	switch(fx->Parameter2) {
		case 1:
			target->fxqueue.RemoveAllEffects(fx->Resource, FX_DURATION_INSTANT_WHILE_EQUIPPED);
			break;
		case 2:
			target->fxqueue.RemoveAllEffects(fx->Resource, FX_DURATION_INSTANT_LIMITED);
			break;
		default:
			target->fxqueue.RemoveAllEffects(fx->Resource);
	}
	return FX_NOT_APPLIED;
}

//0xff SalamanderAura
int fx_salamander_aura (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_salamander_aura(%2d): ResRef:%s Type: %d", fx->Opcode, fx->Resource, fx->Parameter2);
	//inflicts damage calculated by dice values+parameter1
	//creates damage opcode on everyone around. fx->Parameter2 - 0 fire, 1 - ice,
	//Param2 = 2/3 are gemrb specific, i couldn't resist
	//doesn't affect targets with 100% resistance, but could affect self

	//if the target is dead, this effect ceases to exist
	if (STATE_GET(STATE_DEAD|STATE_PETRIFIED|STATE_FROZEN) ) {
		return FX_NOT_APPLIED;
	}

	//timing
	ieDword time = core->GetGame()->GameTime;
	if ((fx->Parameter4==time) || (time%core->Time.round_size) ) {
		return FX_APPLIED;
	}
	fx->Parameter4=time;

	ieDword damage, mystat;
	switch(fx->Parameter2) {
	case 0:default:
		damage = DAMAGE_FIRE;
		mystat = IE_RESISTFIRE;
		break;
	case 1:
		damage = DAMAGE_COLD;
		mystat = IE_RESISTCOLD;
		break;
	case 2:
		damage = DAMAGE_ELECTRICITY;
		mystat = IE_RESISTELECTRICITY;
		break;
	case 3:
		damage = DAMAGE_ACID;
		mystat = IE_RESISTACID;
		break;
	}

	Effect *newfx = EffectQueue::CreateEffect(fx_damage_opcode_ref, fx->Parameter1, damage<<16, FX_DURATION_INSTANT_PERMANENT);
	newfx->Target = FX_TARGET_PRESET;
	newfx->Power = fx->Power;
	newfx->DiceThrown = fx->DiceThrown;
	newfx->DiceSides = fx->DiceSides;
	memcpy(newfx->Resource, fx->Resource,sizeof(newfx->Resource) );

	Map *area = target->GetCurrentArea();
	int i = area->GetActorCount(true);
	while(i--) {
		Actor *victim = area->GetActor(i,true);
		if (PersonalDistance(target, victim)>20) continue;
		if (victim->GetSafeStat(mystat)>=100) continue;
		//apply the damage opcode
		core->ApplyEffect(newfx, victim, Owner);
	}
	delete newfx;
	return FX_APPLIED;
}

//0x100 UmberHulkGaze (causes confusion)
//it is a specially hacked effect to ignore certain races
//from the confusion effect

int fx_umberhulk_gaze (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_umberhulk_gaze(%2d): Duration: %d", fx->Opcode, fx->Parameter1);

	//if the target is dead, this effect ceases to exist
	if (STATE_GET(STATE_DEAD|STATE_PETRIFIED|STATE_FROZEN) ) {
		return FX_NOT_APPLIED;
	}
	fx->TimingMode=FX_DURATION_AFTER_EXPIRES;
	fx->Duration=core->GetGame()->GameTime+7*AI_UPDATE_TIME;

	//build effects to apply
	Effect * newfx1, *newfx2;

	newfx1 = EffectQueue::CreateEffectCopy(fx, fx_confusion_ref, 0, 0);
	newfx1->TimingMode = FX_DURATION_INSTANT_LIMITED;
	newfx1->Duration = fx->Parameter1;

	newfx2 = EffectQueue::CreateEffectCopy(fx, fx_resist_spell_ref, 0, 0);
	newfx2->TimingMode = FX_DURATION_INSTANT_LIMITED;
	newfx2->Duration = fx->Parameter1;
	memcpy(newfx2->Resource, fx->Source, sizeof(newfx2->Resource) );

	//collect targets and apply effect on targets
	Map *area = target->GetCurrentArea();
	int i = area->GetActorCount(true);
	while(i--) {
		Actor *victim = area->GetActor(i,true);
		if (target==victim) continue;
		if (PersonalDistance(target, victim)>300) continue;

		//check if target is golem/umber hulk/minotaur, the effect is not working
		if (check_iwd_targeting(Owner, victim, 0, 17)) { //umber hulk
			continue;
		}
		if (check_iwd_targeting(Owner, victim, 0, 27)) { //golem
			continue;
		}
		if (check_iwd_targeting(Owner, victim, 0, 29)) { //minotaur
			continue;
		}
		if (check_iwd_targeting(Owner, victim, 0, 23)) { //blind
			continue;
		}

		//apply a confusion opcode on target (0x80)
		core->ApplyEffect(newfx1, victim, Owner);

		//apply a resource resistance against this spell to block flood
		core->ApplyEffect(newfx2, victim, Owner);
	}
	delete newfx1;
	delete newfx2;

	return FX_APPLIED;
}

//0x101 ZombieLordAura (causes Panic) unused in all games

int fx_zombielord_aura (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_zombie_lord_aura(%2d): Duration: %d", fx->Opcode, fx->Parameter1);

	//if the target is dead, this effect ceases to exist
	if (STATE_GET(STATE_DEAD|STATE_PETRIFIED|STATE_FROZEN) ) {
		return FX_NOT_APPLIED;
	}
	fx->TimingMode=FX_DURATION_AFTER_EXPIRES;
	fx->Duration=core->GetGame()->GameTime+7*AI_UPDATE_TIME;

	//build effects to apply
	Effect * newfx1, *newfx2;

	newfx1 = EffectQueue::CreateEffectCopy(fx, fx_fear_ref, 0, 0);
	newfx1->TimingMode = FX_DURATION_INSTANT_LIMITED;
	newfx1->Duration = fx->Parameter1;

	newfx2 = EffectQueue::CreateEffectCopy(fx, fx_resist_spell_ref, 0, 0);
	newfx2->TimingMode = FX_DURATION_INSTANT_LIMITED;
	newfx2->Duration = fx->Parameter1;
	memcpy(newfx2->Resource, fx->Source, sizeof(newfx2->Resource) );

	//collect targets and apply effect on targets
	Map *area = target->GetCurrentArea();
	int i = area->GetActorCount(true);
	while(i--) {
		Actor *victim = area->GetActor(i,true);
		if (target==victim) continue;
		if (PersonalDistance(target, victim)>20) continue;

		//check if target is golem/umber hulk/minotaur, the effect is not working
		if (check_iwd_targeting(Owner, victim, 0, 27)) { //golem
			continue;
		}
		if (check_iwd_targeting(Owner, victim, 0, 1)) { //undead
			continue;
		}

		//apply a panic opcode on target (0x18)
		core->ApplyEffect(newfx1, victim, Owner);

		//apply a resource resistance against this spell to block flood
		core->ApplyEffect(newfx2, victim, Owner);
	}
	delete newfx1;
	delete newfx2;

	return FX_APPLIED;
}
//0x102 Protection:Spell (this is the same as in bg2?)

//0x103 SummonCreature2

static int eamods[]={EAM_DEFAULT,EAM_SOURCEALLY,EAM_SOURCEENEMY};

int fx_summon_creature2 (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_summon_creature2(%2d): ResRef:%s Anim:%s Type: %d", fx->Opcode, fx->Resource, fx->Resource2, fx->Parameter2);

	if (!target) {
		return FX_NOT_APPLIED;
	}

	if (!target->GetCurrentArea()) {
		return FX_APPLIED;
	}
	//summon creature (resource), play vvc (resource2)
	//creature's lastsummoner is Owner
	//creature's target is target
	//position of appearance is target's pos (not sure!!!)
	int eamod = EAM_DEFAULT;
	if (fx->Parameter2<3){
		eamod = eamods[fx->Parameter2];
	}
	Effect *newfx = EffectQueue::CreateUnsummonEffect(fx);
	if (fx->Parameter2 == 3) { // summon at source
		core->SummonCreature(fx->Resource, fx->Resource2, Owner, target, Owner->Pos, eamod, 0, newfx);
	} else {
		core->SummonCreature(fx->Resource, fx->Resource2, Owner, target, target->Pos, eamod, 0, newfx);
	}
	delete newfx;
	return FX_NOT_APPLIED;
}

//0x104 AvatarRemoval
int fx_avatar_removal (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	BASE_SET(IE_AVATARREMOVAL, 1);
	return FX_NOT_APPLIED;
}

//0x105 immunity to effect (same as bg2?)
//0x106 SummonPomab

int fx_summon_pomab (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_summon_pomab(%2d): ResRef:%s Anim:%s Type: %d", fx->Opcode, fx->Resource, fx->Resource2, fx->Parameter2);

	if (!target) {
		return FX_NOT_APPLIED;
	}

	if (!target->GetCurrentArea()) {
		return FX_APPLIED;
	}

	ieResRef tableResRef;

	if (fx->Resource[0]) {
		strnlwrcpy(tableResRef, fx->Resource, 8);
	} else {
		memcpy(tableResRef,"pomab",6);
	}

	AutoTable tab(tableResRef);
	if (!tab) {
		return FX_NOT_APPLIED;
	}

	int cnt = tab->GetRowCount()-1;
	if (cnt<2) {
		return FX_NOT_APPLIED;
	}

	int real = core->Roll(1,cnt,-1);
	const char *resrefs[2]={tab->QueryField((unsigned int) 0,0), tab->QueryField((int) 0,1) };

	for (int i=0;i<cnt;i++) {
		Point p(strtol(tab->QueryField(i+1,0),NULL,0), strtol(tab->QueryField(i+1,1), NULL, 0));
		core->SummonCreature(resrefs[real!=i], fx->Resource2, Owner,
			target, p, EAM_DEFAULT, 0, NULL, 0);
	}
	return FX_NOT_APPLIED;
}

//This is the IWD2 charm effect
//0x107 ControlUndead (like charm)
//425 ControlUndead2
int fx_control_undead (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_control_undead(%2d): General: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	//blood rage berserking gives immunity to charm (in iwd2)
	if (target->HasSpellState(SS_BLOODRAGE)) {
		return FX_NOT_APPLIED;
	}

	//protection from evil gives immunity to charm (in iwd2)
	if (target->HasSpellState(SS_PROTFROMEVIL)) {
		return FX_NOT_APPLIED;
	}

	if (fx->Parameter1 && (STAT_GET(IE_GENERAL)!=fx->Parameter1)) {
		return FX_NOT_APPLIED;
	}
	bool enemyally = true;
	Scriptable *caster = target->GetCurrentArea()->GetActorByGlobalID(fx->CasterID);
	if (caster && caster->Type==ST_ACTOR) {
		enemyally = ((Actor *) caster)->GetStat(IE_EA) > EA_GOODCUTOFF; //or evilcutoff?
	}

	//do this only on first use
	if (fx->FirstApply) {
		if (Owner->Type == ST_ACTOR) {
			fx->CasterID = Owner->GetGlobalID();
			enemyally = ((Actor *) Owner)->GetStat(IE_EA) > EA_GOODCUTOFF; //or evilcutoff?
		}
		switch (fx->Parameter2) {
		case 0: //charmed (target neutral after charm)
			displaymsg->DisplayConstantStringName(STR_CHARMED, DMC_WHITE, target);
			break;
		case 1: //charmed (target hostile after charm)
			displaymsg->DisplayConstantStringName(STR_CHARMED, DMC_WHITE, target);
			target->SetBase(IE_EA, EA_ENEMY);
			break;
		case 2: //controlled by cleric
			displaymsg->DisplayConstantStringName(STR_CONTROLLED, DMC_WHITE, target);
			target->SetSpellState(SS_DOMINATION);
			break;
		case 3: //controlled by cleric (hostile after charm)
			displaymsg->DisplayConstantStringName(STR_CONTROLLED, DMC_WHITE, target);
			target->SetBase(IE_EA, EA_ENEMY);
			target->SetSpellState(SS_DOMINATION);
			break;
		case 4: //turn undead
			displaymsg->DisplayConstantStringName(STR_CONTROLLED, DMC_WHITE, target);
			target->SetBase(IE_EA, EA_ENEMY);
			target->SetStat(IE_MORALE, 0, 0);
			target->SetSpellState(SS_DOMINATION);
			break;
		}
	}

	STATE_SET( STATE_CHARMED );
	STAT_SET_PCF( IE_EA, enemyally?EA_ENEMY:EA_CHARMED );
	//don't stick if permanent
	return FX_PERMANENT;
}

//0x108 StaticCharge
int fx_static_charge(Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_static_charge(%2d): Count: %d ", fx->Opcode, fx->Parameter1);

	//if the target is dead, this effect ceases to exist
	if (STATE_GET(STATE_DEAD|STATE_PETRIFIED|STATE_FROZEN) ) {
		return FX_NOT_APPLIED;
	}

	int ret = FX_APPLIED;

	if (fx->Parameter1<=1) {
		ret = FX_NOT_APPLIED;
	}

	//timing
	fx->TimingMode=FX_DURATION_DELAY_PERMANENT;
	fx->Duration=core->GetGame()->GameTime+70*AI_UPDATE_TIME;
	fx->Parameter1--;

	//iwd2 style
	if (fx->Resource[0]) {
		core->ApplySpell(fx->Resource, target, Owner, fx->Power);
		return ret;
	}

	//how style
	target->Damage(DICE_ROLL(0), DAMAGE_ELECTRICITY, Owner, fx->IsVariable, fx->SavingThrowType);
	return ret;
}

//0x109 CloakOfFear (HoW/IWD2)
//if the resource is not specified, it will work like in HoW
int fx_cloak_of_fear(Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_cloak_of_fear(%2d): Count: %d ", fx->Opcode, fx->Parameter1);

	//if the target is dead, this effect ceases to exist
	if (STATE_GET(STATE_DEAD|STATE_PETRIFIED|STATE_FROZEN) ) {
		return FX_NOT_APPLIED;
	}

	if (!fx->Parameter1) {
		return FX_NOT_APPLIED;
	}

	//timing (set up next fire)
	fx->TimingMode=FX_DURATION_DELAY_PERMANENT;
	fx->Duration=core->GetGame()->GameTime+3*AI_UPDATE_TIME;
	fx->Parameter1--;

	//iwd2 style
	if (fx->Resource[0]) {
		core->ApplySpell(fx->Resource, target, Owner, fx->Power);
		return FX_APPLIED;
	}

	//how style (probably better would be to provide effcof.spl)
	Effect *newfx = EffectQueue::CreateEffect(fx_umberhulk_gaze_ref, 0,
		8, FX_DURATION_INSTANT_PERMANENT);
	newfx->Power = fx->Power;

	//collect targets and apply effect on targets
	Map *area = target->GetCurrentArea();
	int i = area->GetActorCount(true);
	while(i--) {
		Actor *victim = area->GetActor(i,true);
		if (target==victim) continue;
		if (PersonalDistance(target, victim)<20) {
			core->ApplyEffect(newfx, target, Owner);
		}
	}
	delete newfx;

	return FX_APPLIED;
}

//0x10a MovementRateModifier3 (Like bg2)
//0x10b Cure:Confusion (Like bg2)

//0x10c EyeOfTheMind
int fx_eye_of_the_mind (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_eye_of_the_mind(%2d)", fx->Opcode);
	if (target->SetSpellState( SS_EYEMIND)) return FX_APPLIED;
	EXTSTATE_SET(EXTSTATE_EYE_MIND);

	if (fx->FirstApply) {
		target->LearnSpell(SevenEyes[EYE_MIND], LS_MEMO);
	}
	return FX_APPLIED;
}
//0x10d EyeOfTheSword
int fx_eye_of_the_sword (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_eye_of_the_sword(%2d)", fx->Opcode);
	if (target->SetSpellState( SS_EYESWORD)) return FX_APPLIED;
	EXTSTATE_SET(EXTSTATE_EYE_SWORD);

	if (fx->FirstApply) {
		target->LearnSpell(SevenEyes[EYE_SWORD], LS_MEMO);
	}
	return FX_APPLIED;
}

//0x10e EyeOfTheMage
int fx_eye_of_the_mage (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_eye_of_the_mage(%2d)", fx->Opcode);
	if (target->SetSpellState( SS_EYEMAGE)) return FX_APPLIED;
	EXTSTATE_SET(EXTSTATE_EYE_MAGE);

	if (fx->FirstApply) {
		target->LearnSpell(SevenEyes[EYE_MAGE], LS_MEMO);
	}
	return FX_APPLIED;
}

//0x10f EyeOfVenom
int fx_eye_of_venom (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_eye_of_venom(%2d)", fx->Opcode);
	if (target->SetSpellState( SS_EYEVENOM)) return FX_APPLIED;
	EXTSTATE_SET(EXTSTATE_EYE_VENOM);

	if (fx->FirstApply) {
		target->LearnSpell(SevenEyes[EYE_VENOM], LS_MEMO);
	}
	return FX_APPLIED;
}

//0x110 EyeOfTheSpirit
int fx_eye_of_the_spirit (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_eye_of_the_spirit(%2d)", fx->Opcode);
	if (target->SetSpellState( SS_EYESPIRIT)) return FX_APPLIED;
	EXTSTATE_SET(EXTSTATE_EYE_SPIRIT);

	if (fx->FirstApply) {
		target->LearnSpell(SevenEyes[EYE_SPIRIT], LS_MEMO);
	}
	return FX_APPLIED;
}

//0x111 EyeOfFortitude
int fx_eye_of_fortitude (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_eye_of_fortitude(%2d)", fx->Opcode);
	if (target->SetSpellState( SS_EYEFORTITUDE)) return FX_APPLIED;
	EXTSTATE_SET(EXTSTATE_EYE_FORT);

	if (fx->FirstApply) {
		target->LearnSpell(SevenEyes[EYE_FORT], LS_MEMO);
	}
	return FX_APPLIED;
}

//0x112 EyeOfStone
int fx_eye_of_stone (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_eye_of_stone(%2d)", fx->Opcode);
	if (target->SetSpellState( SS_EYESTONE)) return FX_APPLIED;
	EXTSTATE_SET(EXTSTATE_EYE_STONE);

	if (fx->FirstApply) {
		target->LearnSpell(SevenEyes[EYE_STONE], LS_MEMO);
	}
	return FX_APPLIED;
}

//0x113 RemoveSevenEyes

int fx_remove_seven_eyes (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_remove_seven_eyes(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	target->spellbook.RemoveSpell(SevenEyes[EYE_MIND]);
	target->spellbook.RemoveSpell(SevenEyes[EYE_SWORD]);
	target->spellbook.RemoveSpell(SevenEyes[EYE_MAGE]);
	target->spellbook.RemoveSpell(SevenEyes[EYE_VENOM]);
	target->spellbook.RemoveSpell(SevenEyes[EYE_SPIRIT]);
	target->spellbook.RemoveSpell(SevenEyes[EYE_FORT]);
	target->spellbook.RemoveSpell(SevenEyes[EYE_STONE]);
	return FX_NOT_APPLIED;
}

//0x114 RemoveEffect
int fx_remove_effect (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_remove_effect(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	if (fx->Resource[0])
	{
		target->fxqueue.RemoveAllEffectsWithResource(fx->Parameter2, fx->Resource);
	}
	else
	{
		target->fxqueue.RemoveAllEffects(fx->Parameter2);
	}
	return FX_NOT_APPLIED;
}

//0x115 SoulEater
int fx_soul_eater (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_soul_eater(%2d): Damage %d", fx->Opcode, fx->Parameter1);
	// Soul Eater has no effect on undead, constructs, and elemental creatures,
	// but this is handled in the spells via fx_resist_spell_and_message
	int damage = fx->Parameter1;
	// the how spell has it set to 0, so use the damage from the description
	if (!damage) {
		damage = core->Roll(3, 8, 0);
	}

	target->Damage(damage, DAMAGE_SOULEATER, Owner, fx->IsVariable, fx->SavingThrowType);
	//the state is not set soon enough!
	//if (STATE_GET(STATE_DEAD) ) {
	if (target->GetInternalFlag() & IF_REALLYDIED) {
		ieResRef monster;
		ieResRef hit;
		ieResRef areahit;

		core->GetResRefFrom2DA("souleatr", monster, hit, areahit);
		//the monster should appear near the effect position
		Point p(fx->PosX, fx->PosY);
		Effect *newfx = EffectQueue::CreateUnsummonEffect(fx);
		core->SummonCreature(monster, areahit, Owner, target, p, EAM_SOURCEALLY, fx->Parameter1, newfx);

		// for each kill the caster receives a +1 bonus to Str, Dex and Con for 1 turn
		if (Owner->Type == ST_ACTOR) {
			newfx = EffectQueue::CreateEffect(fx_str_ref, 1, MOD_ADDITIVE, FX_DURATION_INSTANT_LIMITED);
			newfx->Duration = core->Time.turn_sec;
			core->ApplyEffect(newfx, (Actor *)Owner, Owner);
			newfx = EffectQueue::CreateEffect(fx_dex_ref, 1, MOD_ADDITIVE, FX_DURATION_INSTANT_LIMITED);
			newfx->Duration = core->Time.turn_sec;
			core->ApplyEffect(newfx, (Actor *)Owner, Owner);
			newfx = EffectQueue::CreateEffect(fx_con_ref, 1, MOD_ADDITIVE, FX_DURATION_INSTANT_LIMITED);
			newfx->Duration = core->Time.turn_sec;
			core->ApplyEffect(newfx, (Actor *)Owner, Owner);
		}
		delete newfx;
	}
	return FX_NOT_APPLIED;
}

//0x116 ShroudOfFlame (how)
//FIXME: maybe it is cheaper to port effsof1/2 to how than having an alternate effect
int fx_shroud_of_flame (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_shroud_of_flame(%2d): Type: %d", fx->Opcode, fx->Parameter2);

	//if the target is dead, this effect ceases to exist
	if (STATE_GET(STATE_DEAD|STATE_PETRIFIED|STATE_FROZEN) ) {
		return FX_NOT_APPLIED;
	}

	//don't apply it twice in a round
	if (EXTSTATE_GET(EXTSTATE_SHROUD)) {
		return FX_APPLIED;
	}
	EXTSTATE_SET(EXTSTATE_SHROUD);
	//directly modifying the color of the target
	if (fx->Parameter2==1) {
		target->SetColorMod(0xff, RGBModifier::ADD, -1, 0, 0, 0x96);
	}
	else {
		target->SetColorMod(0xff, RGBModifier::ADD, -1, 0x96, 0, 0);
	}

	//timing
	ieDword time = core->GetGame()->GameTime;
	if ((fx->Parameter4==time) || (time%6) ) {
		return FX_APPLIED;
	}
	fx->Parameter4=time;

	//inflicts damage calculated by dice values+parameter1
	//creates damage opcode on everyone around. fx->Parameter2 - 0 fire, 1 - ice
	ieDword damagetype = DAMAGE_FIRE;

	if (fx->Parameter2==1) {
		damagetype = DAMAGE_COLD;
	}

	target->Damage(fx->Parameter1, damagetype, Owner, fx->IsVariable, fx->SavingThrowType);
	ApplyDamageNearby(Owner, target, fx, damagetype);
	return FX_APPLIED;
}

static ieResRef resref_sof1={"effsof1"};
static ieResRef resref_sof2={"effsof2"};

//0x116 ShroudOfFlame (iwd2)
int fx_shroud_of_flame2 (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_shroud_of_flame2(%2d)", fx->Opcode);

	//if the target is dead, this effect ceases to exist
	if (STATE_GET(STATE_DEAD|STATE_PETRIFIED|STATE_FROZEN) ) {
		return FX_NOT_APPLIED;
	}

	if (target->SetSpellState( SS_FLAMESHROUD)) return FX_APPLIED;
	EXTSTATE_SET(EXTSTATE_SHROUD); //just for compatibility

	if(enhanced_effects) {
		target->SetColorMod(0xff, RGBModifier::ADD, 1, 0xa0, 0, 0);
	}

	//apply resource on hitter
	//actually, this should be a list of triggers
	if (fx->Resource[0]) {
		memcpy(target->applyWhenBeingHit,fx->Resource,sizeof(ieResRef));
	} else {
		memcpy(target->applyWhenBeingHit,resref_sof1,sizeof(ieResRef));
	}

	//timing
	ieDword time = core->GetGame()->GameTime;
	if ((fx->Parameter4==time) || (time%6) ) {
		return FX_APPLIED;
	}
	fx->Parameter4=time;

	if (fx->Resource2[0]) {
		core->ApplySpell(fx->Resource2, target, Owner, fx->Power);
	} else {
		core->ApplySpell(resref_sof2, target, Owner, fx->Power);
	}
	return FX_APPLIED;
}

//0x117 AnimalRage
int fx_animal_rage (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_animal_rage(%2d): Mode: %d", fx->Opcode, fx->Parameter2);

	//param2==1 sets only the spell state
	if (fx->Parameter2) {
		target->SetSpellState( SS_ANIMALRAGE);
		return FX_APPLIED;
	}

	//the state check is done after the parameter differentiation
	//it might be a bug but i cannot decide (going with the original)
	if (STATE_GET(STATE_DEAD|STATE_PETRIFIED|STATE_FROZEN) ) {
		return FX_NOT_APPLIED;
	}

	//param2==0 doesn't set the spell state

	//don't do anything if already berserking
	//FIXME: is it berserkstage2 or 1?
	if (STAT_GET(IE_BERSERKSTAGE1)) {
		return FX_APPLIED;
	}

	//it has 5% of going berserk
	//FIXME: how much is the original bg berserking chance
	//if it is different, use checkforberserk as a percentile chance
	STAT_SET( IE_CHECKFORBERSERK, 1 );

	//and attacks the first enemy in sight
	//timing
	if (core->GetGame()->GameTime%6) {
		return FX_APPLIED;
	}
	//if enemy is in sight
	//attack them
	//FIXME: would the circle color change?
	if (!target->LastTarget) {
		//depends on whom it considers enemy
		if (STAT_GET(IE_EA)<EA_EVILCUTOFF) {
			Enemy->objectParameter->objectFilters[0]=EA_ENEMY;
		} else {
			Enemy->objectParameter->objectFilters[0]=EA_ALLY;
		}
		//see the nearest enemy
		if (SeeCore(target, Enemy, false)) {
			target->FaceTarget(target->GetCurrentArea()->GetActorByGlobalID(target->LastSeen));
			//this is highly unsure
			//fx->Parameter1=1;
		}
	}
	return FX_APPLIED;
}

//0x118 TurnUndead2 iwd2
int fx_turn_undead2 (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_turn_undead2(%2d): Level: %d Type %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	switch (fx->Parameter2)
	{
	case 0: //command
		target->AddTrigger(TriggerEntry(trigger_turnedby, Owner->GetGlobalID()));
		target->Panic(Owner, PANIC_RUNAWAY);
		break;
	case 1://rebuke
		target->AddTrigger(TriggerEntry(trigger_turnedby, Owner->GetGlobalID()));
		if (target->SetSpellState(SS_REBUKED)) {
			//display string rebuked
		}
		target->AC.HandleFxBonus(-4, fx->TimingMode==FX_DURATION_INSTANT_PERMANENT);
		break;
	case 2://destroy
		target->AddTrigger(TriggerEntry(trigger_turnedby, Owner->GetGlobalID()));
		target->Die(Owner);
		break;
	case 3://panic
		target->AddTrigger(TriggerEntry(trigger_turnedby, Owner->GetGlobalID()));
		target->Panic(Owner, PANIC_RUNAWAY);
		break;
	default://depends on caster
		if (fx->Parameter1) {
			target->Turn(Owner, fx->Parameter1);
		} else {
			if (Owner->Type!=ST_ACTOR) {
				return FX_NOT_APPLIED;
			}
			target->Turn(Owner, ((Actor *) Owner)->GetStat(IE_TURNUNDEADLEVEL));
		}
		break;
	}
	return FX_APPLIED;
}

//0x119 VitriolicSphere
int fx_vitriolic_sphere (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_vitriolic_sphere(%2d): Damage %d", fx->Opcode, fx->Parameter1);
	//timing
	if (core->GetGame()->GameTime%6) {
		return FX_APPLIED;
	}
	target->Damage(fx->Parameter1, DAMAGE_ACID, Owner, fx->IsVariable, fx->SavingThrowType);
	fx->DiceThrown-=2;
	if ((signed) fx->DiceThrown<1) {
		return FX_NOT_APPLIED;
	}
	//also damage people nearby?
	ApplyDamageNearby(Owner, target, fx, DAMAGE_ACID);
	return FX_APPLIED;
}

//0x11a SuppressHP
int fx_suppress_hp (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_suppress_hp(%2d)", fx->Opcode);
	if (target->SetSpellState( SS_NOHPINFO)) return FX_APPLIED;
	EXTSTATE_SET(EXTSTATE_NO_HP);
	return FX_APPLIED;
}

//0x11b FloatText
int fx_floattext (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_floattext(%2d): StrRef:%d Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	switch (fx->Parameter2)
	{
	case 1:
		//in the original game this signified that a specific weapon is equipped
		if (EXTSTATE_GET(EXTSTATE_FLOATTEXTS))
			return FX_APPLIED;

		EXTSTATE_SET(EXTSTATE_FLOATTEXTS);
		if (!fx->Resource[0]) {
			strnuprcpy(fx->Resource,"cynicism",sizeof(ieResRef)-1);
		}
		if (fx->Parameter1) {
			fx->Parameter1--;
			return FX_APPLIED;
		} else {
			fx->Parameter1=core->Roll(1,500,500);
		}
	case 2:
		if (EXTSTATE_GET(EXTSTATE_FLOATTEXTS)) {
			ieDword *CynicismList = core->GetListFrom2DA(fx->Resource);
			ieDword i = CynicismList[0];
			if (i) {
				DisplayStringCore(target, CynicismList[core->Roll(1,i,0)], DS_HEAD);
			}
		}
		return FX_APPLIED;
	default:
		DisplayStringCore(target, fx->Parameter1, DS_HEAD);
		break;
	case 3: //gemrb extension, displays verbalconstant
		DisplayStringCore(target, fx->Parameter1, DS_CONST|DS_HEAD);
		break;
	}
	return FX_NOT_APPLIED;
}

//0x11c MaceOfDisruption
//death with chance based on race and level

int fx_mace_of_disruption (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_mace_of_disruption(%2d): ResRef:%s Anim:%s Type: %d", fx->Opcode, fx->Resource, fx->Resource2, fx->Parameter2);
	ieDword race = STAT_GET(IE_RACE);
	//golem / outer planar gets hit
	int chance = 0;
	switch (race) {
		case 156: // outsider
			chance = 5;
			break;
		case 108: case 115: case 167: //ghoul, skeleton, undead
			switch (STAT_GET(IE_LEVEL)) {
			case 1: case 2: case 3: case 4:
				chance = 100;
				break;
			case 5:
				chance = 95;
				break;
			case 6:
				chance = 80;
				break;
			case 7:
				chance = 65;
				break;
			case 8: case 9:
				chance = 50;
				break;
			case 10:
				chance = 35;
				break;
			default:
				chance = 20;
				break;
			}
			break;
		default:;
	}
	if (chance < core->Roll(1,100,0)) {
		return FX_NOT_APPLIED;
	}

	Effect *newfx = EffectQueue::CreateEffect(fx_iwd_visual_spell_hit_ref, 0,
			8, FX_DURATION_INSTANT_PERMANENT);
	newfx->Target=FX_TARGET_PRESET;
	newfx->Power=fx->Power;
	core->ApplyEffect(newfx, target, Owner);

	newfx = EffectQueue::CreateEffect(fx_death_ref, 0,
			8, FX_DURATION_INSTANT_PERMANENT);
	newfx->Target=FX_TARGET_PRESET;
	newfx->Power=fx->Power;
	core->ApplyEffect(newfx, target, Owner);

	delete newfx;

	return FX_NOT_APPLIED;
}
//0x11d Sleep2 ??? power word sleep?
//0x11e Reveal:Tracks (same as bg2)
//0x11f Protection:Backstab (same as bg2)

//0x120 State:Set
int fx_set_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_set_state(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	//in IWD2 we have 176 states (original had 256)
	target->SetSpellState(fx->Parameter2);
	//in HoW this sets only the 10 last bits of extstate (until it runs out of bits)
	if (fx->Parameter2<11) {
		EXTSTATE_SET(0x40000<<fx->Parameter2);
	}
	//maximized attacks active
	if (fx->Parameter2==SS_KAI) {
		target->Modified[IE_DAMAGELUCK]=255;
	}
	return FX_APPLIED;
}

//0x121 Cutscene (this is a very ugly hack in iwd)
//It doesn't really start a cutscene, just sets a variable
//The script system itself will detect that variable and activate the cutscene
//ToB has an effect which actually runs a hardcoded cutscene
int fx_cutscene (Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if(0) print("fx_cutscene(%2d)", fx->Opcode);
	Game *game = core->GetGame();
	game->locals->SetAt("GEM_ACTIVE", 1);
	return FX_NOT_APPLIED;
}

//0xce (same place as in bg2, but different targeting)
int fx_resist_spell (Scriptable* Owner, Actor* target, Effect *fx)
{
	//check iwd ids targeting
	//changed this to the opposite (cure light wounds resisted by undead)
	if (!check_iwd_targeting(Owner, target, fx->Parameter1, fx->Parameter2) ) {
		return FX_NOT_APPLIED;
	}

	if (strnicmp(fx->Resource,fx->Source,sizeof(fx->Resource)) ) {
		return FX_APPLIED;
	}
	//this has effect only on first apply, it will stop applying the spell
	return FX_ABORT;
}

//0x122 Protection:Spell3 ??? IWD ids targeting
// this is a variant of resist spell, used in iwd2
int fx_resist_spell_and_message (Scriptable* Owner, Actor* target, Effect *fx)
{
	//check iwd ids targeting
	//changed this to the opposite (cure light wounds resisted by undead)
	if (!check_iwd_targeting(Owner, target, fx->Parameter1, fx->Parameter2) ) {
		return FX_NOT_APPLIED;
	}

	//convert effect to the normal resist spell effect (without text)
	//in case it lingers
	fx->Opcode=EffectQueue::ResolveEffect(fx_resist_spell_ref);

	if (strnicmp(fx->Resource,fx->Source,sizeof(fx->Resource)) ) {
		return FX_APPLIED;
	}
	//display message too
	int spellname = -1;

	if(gamedata->Exists(fx->Resource, IE_ITM_CLASS_ID) ) {
		Item *poi = gamedata->GetItem(fx->Resource);
		spellname = poi->ItemName;
		gamedata->FreeItem(poi, fx->Resource, 0);
	} else if (gamedata->Exists(fx->Resource, IE_SPL_CLASS_ID) ) {
		Spell *poi = gamedata->GetSpell(fx->Resource, true);
		spellname = poi->SpellName;
		gamedata->FreeSpell(poi, fx->Resource, 0);
	}

	if (spellname>=0) {
		char *tmpstr = core->GetString(spellname, 0);
		core->GetTokenDictionary()->SetAtCopy("RESOURCE", tmpstr);
		core->FreeString(tmpstr);
		displaymsg->DisplayConstantStringName(STR_RES_RESISTED, DMC_WHITE, target);
	}
	//this has effect only on first apply, it will stop applying the spell
	return FX_ABORT;
}

//0x123 RodOfSmithing
//if golem: 5% death or 1d8+3 damage
//if outsider: 5% 8d3 damage or nothing
//otherwise: nothing
int fx_rod_of_smithing (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_rod_of_smithing(%2d): ResRef:%s Anim:%s Type: %d", fx->Opcode, fx->Resource, fx->Resource2, fx->Parameter2);
	int damage = 0;
	int five_percent = core->Roll(1,100,0)<5;

	if (check_iwd_targeting(Owner, target, 0, 27)) { //golem
			if(five_percent) {
				//instant death
				damage = -1;
			} else {
				damage = core->Roll(1,8,3);
			}
	} else if (check_iwd_targeting(Owner, target, 0, 92)) { //outsider
			if (five_percent) {
				damage = core->Roll(8,3,0);
			}
	}
	if (damage) {
		Effect *newfx;
		if (damage<0) {
			//create death effect (chunked death)
			newfx = EffectQueue::CreateEffect(fx_death_ref, 0, 8, FX_DURATION_INSTANT_PERMANENT);
		} else {
			//create damage effect (blunt)
			newfx = EffectQueue::CreateEffect(fx_damage_opcode_ref, (ieDword) damage,
				0, FX_DURATION_INSTANT_PERMANENT);
		}
		core->ApplyEffect(newfx, target, Owner);
		delete newfx;
	}

	return FX_NOT_APPLIED;
}

//0x124 MagicalRest (same as bg2)

//0x125 BeholderDispelMagic (applies resource on nearby actors)
//TODO: range, affected actors
int fx_beholder_dispel_magic (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_beholder_dispel_magic(%2d): Spell: %s", fx->Opcode, fx->Resource);
	if (!fx->Resource[0]) {
		strcpy(fx->Resource,"SPIN164");
	}

	//if the target is dead, this effect ceases to exist
	if (STATE_GET(STATE_DEAD|STATE_PETRIFIED|STATE_FROZEN) ) {
		return FX_NOT_APPLIED;
	}

	Map *area = target->GetCurrentArea();
	int i = area->GetActorCount(true);
	while(i--) {
		Actor *victim = area->GetActor(i,true);
		if (target==victim) continue;
		if (PersonalDistance(target, victim)<300) {
			//this function deletes tmp
			core->ApplySpell(fx->Resource, victim, Owner, fx->Power);
		}
	}

	return FX_NOT_APPLIED;
}

//0x126 HarpyWail (applies resource on nearby actors)
//TODO: range, affected actors, sound effect
int fx_harpy_wail (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_harpy_wail(%2d): Spell: %s", fx->Opcode, fx->Resource);
	if (!fx->Resource[0]) {
		strcpy(fx->Resource,"SPIN166");
	}
	if (!fx->Resource2[0]) {
		strcpy(fx->Resource2,"EFF_P111");
	}

	//if the target is dead, this effect ceases to exist
	if (STATE_GET(STATE_DEAD|STATE_PETRIFIED|STATE_FROZEN) ) {
		return FX_NOT_APPLIED;
	}
	core->GetAudioDrv()->Play(fx->Resource2, target->Pos.x, target->Pos.y);

	Map *area = target->GetCurrentArea();
	int i = area->GetActorCount(true);
	while(i--) {
		Actor *victim = area->GetActor(i,true);
		if (target==victim) continue;
		if (PersonalDistance(target, victim)<300) {
			//this function deletes tmp
			core->ApplySpell(fx->Resource, victim, Owner, fx->Power);
		}
	}

	return FX_NOT_APPLIED;
}

//0x127 JackalWereGaze (applies resource on nearby actors)
//TODO: range, affected actors
int fx_jackalwere_gaze (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_jackalwere_gaze(%2d): Spell: %s", fx->Opcode, fx->Resource);
	if (!fx->Resource[0]) {
		strcpy(fx->Resource,"SPIN179");
	}

	//if the target is dead, this effect ceases to exist
	if (STATE_GET(STATE_DEAD|STATE_PETRIFIED|STATE_FROZEN) ) {
		return FX_NOT_APPLIED;
	}

	Map *area = target->GetCurrentArea();
	int i = area->GetActorCount(true);
	while(i--) {
		Actor *victim = area->GetActor(i,true);
		if (target==victim) continue;
		if (PersonalDistance(target, victim)<300) {
			//this function deletes tmp
			core->ApplySpell(fx->Resource, victim, Owner, fx->Power);
		}
	}

	return FX_APPLIED;
}
//0x128 ModifyGlobalVariable (same as bg2)
//0x129 HideInShadows (same as bg2)

//0x12a UseMagicDevice
int fx_use_magic_device_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_use_magic_device_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	STAT_MOD( IE_MAGICDEVICE );
	return FX_APPLIED;
}

//GemRB specific IWD related effects

//This code is needed for the ending cutscene in IWD (found in a projectile)
//The effect will alter the target animation's cycle by a xor value
//Parameter1: the value to binary xor on the initial cycle numbers of the animation(s)
//Parameter2: an optional projectile (IWD spell hit projectiles start at 0x1001)
//Resource: the animation's name

//Useful applications other than the HoW cutscene:
//A fireball could affect environment by applying the effect on certain animations
//all you need to do:
//Name the area animation as 'burnable'.
//Create alternate cycles for the altered area object
//Create a spell hit animation (optionally)
//Create the effect which will contain the spell hit projectile and the cycle change command
//0x18f AlterAnimation
int fx_alter_animation (Scriptable* Owner, Actor* /*target*/, Effect* fx)
{
	if(0) print("fx_alter_animation(%2d) Parameter: %d  Projectile: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	Map *map = Owner->GetCurrentArea();
	if (!map) {
		return FX_NOT_APPLIED;
	}

	aniIterator iter = map->GetFirstAnimation();
	while(AreaAnimation *an = map->GetNextAnimation(iter) ) {
		//Only animations with 8 letters could be used, no problem, iwd uses 8 letters
		if (!strnicmp (an->Name, fx->Resource, 8) ) {
			//play spell hit animation
			Projectile *pro=core->GetProjectileServer()->GetProjectileByIndex(fx->Parameter2);
			pro->SetCaster(fx->CasterID, fx->CasterLevel);
			map->AddProjectile(pro, an->Pos, an->Pos);
			//alter animation, we need only this for the original, but in the
			//spirit of unhardcoding, i provided the standard modifier codeset
			//0->4, 1->5, 2->6, 3->7
			//4->0, 5->1, 6->2, 7->3
			ieWord value = fx->Parameter1>>16;
			switch(fx->Parameter1&0xffff) {
				case BM_SET:
					an->sequence=value;
					break;
				case BM_AND:
					an->sequence&=value;
					break;
				case BM_OR:
					an->sequence|=value;
					break;
				case BM_NAND:
					an->sequence&=~value;
					break;
				case BM_XOR:
					an->sequence^=value;
					break;
			}
			an->frame = 0;
			an->InitAnimation();
		}
	}
	return FX_NOT_APPLIED;
}

//0x12b AnimalEmpathy (gemrb extension for iwd2)
int fx_animal_empathy_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_animal_empathy_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	STAT_MOD( IE_ANIMALS );
	return FX_APPLIED;
}

//0x12c Bluff (gemrb extension for iwd2)
int fx_bluff_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_bluff_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	STAT_MOD( IE_BLUFF );
	return FX_APPLIED;
}

//0x12d Concentration (gemrb extension for iwd2)
int fx_concentration_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_concentration_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	STAT_MOD( IE_CONCENTRATION );
	return FX_APPLIED;
}

//0x12e Diplomacy (gemrb extension for iwd2)
int fx_diplomacy_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_diplomacy_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	STAT_MOD( IE_DIPLOMACY );
	return FX_APPLIED;
}

//0x12f Intimidate (gemrb extension for iwd2)
int fx_intimidate_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_intimidate_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	STAT_MOD( IE_INTIMIDATE );
	return FX_APPLIED;
}

//0x130 Search (gemrb extension for iwd2)
int fx_search_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_search_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	STAT_MOD( IE_SEARCH );
	return FX_APPLIED;
}

//0x131 Spellcraft (gemrb extension for iwd2)
int fx_spellcraft_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_spellcraft_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	STAT_MOD( IE_SPELLCRAFT );
	return FX_APPLIED;
}

//0x132 DamageLuck (gemrb extension for iwd2, implemented in base opcodes)

//0x133 TurnLevel (gemrb extension for iwd2)
int fx_turnlevel_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_turnlevel_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	STAT_MOD( IE_TURNUNDEADLEVEL );
	return FX_APPLIED;
}

//IWD2 effects

//400 Hopelessness
int fx_hopelessness (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_hopelessness(%2d)", fx->Opcode);

	if (target->HasSpellState(SS_BLOODRAGE)) {
		return FX_NOT_APPLIED;
	}

	if (target->SetSpellState( SS_HOPELESSNESS)) return FX_NOT_APPLIED;
	target->AddPortraitIcon(PI_HOPELESSNESS);
	STATE_SET(STATE_HELPLESS);
	return FX_APPLIED;
}

//401 ProtectionFromEvil
int fx_protection_from_evil (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_protection_from_evil(%2d)", fx->Opcode);
	//
	if (target->SetSpellState( SS_PROTFROMEVIL)) return FX_APPLIED;
	target->AddPortraitIcon(PI_PROTFROMEVIL);
	//+2 to all saving throws
	STAT_ADD( IE_SAVEFORTITUDE, 2);
	STAT_ADD( IE_SAVEREFLEX, 2);
	STAT_ADD( IE_SAVEWILL, 2);
	//make it compatible with 2nd edition
	STAT_ADD( IE_SAVEVSBREATH, 2);
	STAT_ADD( IE_SAVEVSSPELL, 2);
	//immune to control
	return FX_APPLIED;
}

//402 AddEffectsList
int fx_add_effects_list (Scriptable* Owner, Actor* target, Effect* fx)
{
	//after iwd2 style ids targeting, apply the spell named in the resource field
	if (!check_iwd_targeting(Owner, target, fx->Parameter1, fx->Parameter2) ) {
		return FX_NOT_APPLIED;
	}
	core->ApplySpell(fx->Resource, target, Owner, fx->Power);
	return FX_NOT_APPLIED;
}

//403 ArmorOfFaith
static int fx_armor_of_faith (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_armor_of_faith(%2d) Amount: %d", fx->Opcode, fx->Parameter1);
	if (target->SetSpellState( SS_ARMOROFFAITH)) return FX_APPLIED;
	if (!fx->Parameter1) {
		fx->Parameter1=1;
	}
	//TODO: damage reduction (all types)
	STAT_ADD(IE_RESISTFIRE,fx->Parameter1 );
	STAT_ADD(IE_RESISTCOLD,fx->Parameter1 );
	STAT_ADD(IE_RESISTELECTRICITY,fx->Parameter1 );
	STAT_ADD(IE_RESISTACID,fx->Parameter1 );
	STAT_ADD(IE_RESISTMAGIC,fx->Parameter1 );
	STAT_ADD(IE_RESISTSLASHING,fx->Parameter1 );
	STAT_ADD(IE_RESISTCRUSHING,fx->Parameter1 );
	STAT_ADD(IE_RESISTPIERCING,fx->Parameter1 );
	STAT_ADD(IE_RESISTMISSILE,fx->Parameter1 );
	if (enhanced_effects) {
		target->AddPortraitIcon(PI_FAITHARMOR);
	}
	return FX_APPLIED;
}

//404 Nausea

int fx_nausea (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_nausea(%2d)", fx->Opcode);
	//FIXME: i'm not sure if this part is there
	//create the sleep effect only once?
	if (!fx->Parameter3 && Owner) {
		Effect *newfx = EffectQueue::CreateEffect(fx_unconscious_state_ref,
			fx->Parameter1, 1, fx->TimingMode);
		newfx->Power = fx->Power;
		core->ApplyEffect(newfx, target, Owner);

		delete newfx;
		fx->Parameter3=1;
	}
	//end of unsure part
	if (target->SetSpellState( SS_NAUSEA)) return FX_APPLIED;
	target->AddPortraitIcon(PI_NAUSEA);
	STATE_SET(STATE_HELPLESS|STATE_SLEEP);
	return FX_APPLIED;
}

//405 Enfeeblement
//minimum stats in 3rd ed are 1, so this effect won't kill the target
int fx_enfeeblement (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_enfeeblement(%2d)", fx->Opcode);
	if (target->SetSpellState( SS_ENFEEBLED)) return FX_APPLIED;
	target->AddPortraitIcon(PI_ENFEEBLEMENT);
	STAT_ADD(IE_STR, -15);
	return FX_APPLIED;
}

//406 FireShield
int fx_fireshield (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_fireshield(%2d) Type: %d", fx->Opcode, fx->Parameter2);
	if (fx->Parameter2) {
		if (target->SetSpellState( SS_ICESHIELD)) return FX_APPLIED;
		target->AddPortraitIcon(PI_ICESHIELD);
	} else {
		if (target->SetSpellState( SS_FIRESHIELD)) return FX_APPLIED;
		target->AddPortraitIcon(PI_FIRESHIELD);
	}
	memcpy(target->applyWhenBeingHit,fx->Resource,sizeof(ieResRef));
	return FX_APPLIED;
}

//407 DeathWard
int fx_death_ward (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_death_ward(%2d)", fx->Opcode);
	if (target->SetSpellState( SS_DEATHWARD)) return FX_APPLIED;
	target->AddPortraitIcon(PI_DEATHWARD); // is it ok?

	return FX_APPLIED;
}

//408 HolyPower
int fx_holy_power (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_holy_power(%2d)", fx->Opcode);
	if (target->SetSpellState( SS_HOLYPOWER)) return FX_APPLIED;

	if (enhanced_effects) {
		target->AddPortraitIcon(PI_HOLYPOWER);
		target->SetColorMod(0xff, RGBModifier::ADD, 20, 0x80, 0x80, 0x80);
	}
	STAT_ADD(IE_DAMAGEBONUS, 4);
	return FX_APPLIED;
}

//409 RighteousWrath
int fx_righteous_wrath (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_righteous_wrath(%2d) Type: %d", fx->Opcode, fx->Parameter2);
	if (fx->Parameter2)
	{
		if (target->SetSpellState( SS_RIGHTEOUS2)) return FX_APPLIED;
		//
	}
	else
	{
		if (target->SetSpellState( SS_RIGHTEOUS)) return FX_APPLIED;
		//
	}
	if (enhanced_effects) {
		target->AddPortraitIcon(PI_RIGHTEOUS);
		target->SetColorMod(0xff, RGBModifier::ADD, 30, 0xd7, 0xb6, 0 );
	}
	return FX_APPLIED;
}

//410 SummonAllyIWD2
int fx_summon_ally (Scriptable* Owner, Actor* target, Effect* fx)
{
	Point p(fx->PosX, fx->PosY);
	Effect *newfx = EffectQueue::CreateUnsummonEffect(fx);
	core->SummonCreature(fx->Resource, fx->Resource2, Owner, target, p, EAM_ALLY, 0, newfx);
	delete newfx;
	return FX_NOT_APPLIED;
}

//411 SummonEnemyIWD2
int fx_summon_enemy (Scriptable* Owner, Actor* target, Effect* fx)
{
	Point p(fx->PosX, fx->PosY);
	Effect *newfx = EffectQueue::CreateUnsummonEffect(fx);
	core->SummonCreature(fx->Resource, fx->Resource2, Owner, target, p, EAM_ENEMY, 0, newfx);
	delete newfx;
	return FX_NOT_APPLIED;
}

//412 Control2

int fx_control (Scriptable* Owner, Actor* target, Effect* fx)
{
	//prot from evil deflects it
	if (target->fxqueue.HasEffect(fx_protection_from_evil_ref)) return FX_NOT_APPLIED;

	//check for slippery mind feat success
	Game *game = core->GetGame();
	if (fx->FirstApply && target->HasFeat(FEAT_SLIPPERY_MIND)) {
		fx->Parameter3 = 1;
		fx->Parameter4 = game->GameTime+core->Time.round_size;
	}

	if (fx->Parameter3 && fx->Parameter4<game->GameTime) {
		fx->Parameter3 = 0;
		if (target->GetSavingThrow(IE_SAVEVSSPELL,0) ) return FX_NOT_APPLIED;
	}
	if(0) print("fx_control(%2d)", fx->Opcode);
	bool enemyally = true;
	if (Owner->Type==ST_ACTOR) {
		enemyally = ((Actor *) Owner)->GetStat(IE_EA)>EA_GOODCUTOFF; //or evilcutoff?
	}

	switch(fx->Parameter2)
	{
	case 0:
		displaymsg->DisplayConstantStringName(STR_CHARMED, DMC_WHITE, target);
		break;
	case 1:
		displaymsg->DisplayConstantStringName(STR_DIRECHARMED, DMC_WHITE, target);
		break;
	default:
		displaymsg->DisplayConstantStringName(STR_CONTROLLED, DMC_WHITE, target);

		break;
	}
	STATE_SET( STATE_CHARMED );
	STAT_SET( IE_EA, enemyally?EA_ENEMY:EA_CHARMED );
	return FX_APPLIED;
}

//413 VisualEffectIWD2
//there are 32 bits, so they will fit on IE_SANCTUARY stat
//i put them there because the first bit is sanctuary
int fx_visual_effect_iwd2 (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_visual_effect_iwd2(%2d) Type: %d", fx->Opcode, fx->Parameter2);
	unsigned int type = fx->Parameter2;
	if (type<32) {
		switch(type) {
		case OV_ENTANGLE:
			STAT_BIT_OR(IE_ENTANGLE, 1);
			break;
		case OV_SHIELDGLOBE:
			STAT_BIT_OR(IE_SHIELDGLOBE, 1);
			break;
		case OV_GREASE:
			STAT_BIT_OR(IE_GREASE, 1);
			break;
		case OV_WEB:
			STAT_BIT_OR(IE_WEB, 1);
			break;
		case OV_MINORGLOBE: case OV_GLOBE:
			STAT_BIT_OR(IE_MINORGLOBE, 1);
			break;
		case OV_SEVENEYES:
			target->SetOverlay(OV_SEVENEYES2);
			break;
		case OV_LATH1:
			target->SetOverlay(OV_LATH2);
			break;
		case OV_GLATH1:
			target->SetOverlay(OV_GLATH2);
			break;
		case OV_FIRESHIELD1:
			target->SetOverlay(OV_FIRESHIELD2);
			break;
		case OV_ICESHIELD1:
			target->SetOverlay(OV_ICESHIELD2);
			break;
		}
		//the sanctuary stat is handled in SetOverlay
		target->SetOverlay(type);
		return FX_APPLIED;
	}
	return FX_NOT_APPLIED;
}

//414 ResilientSphere
int fx_resilient_sphere (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_resilient_sphere(%2d)", fx->Opcode);
	target->SetSpellState(SS_HELD|SS_RESILIENT);
	STATE_SET(STATE_HELPLESS);
	if (enhanced_effects) {
		target->AddPortraitIcon(PI_RESILIENT);
		target->SetOverlay(OV_RESILIENT);
	}
	return FX_APPLIED;
}

//415 Barkskin
int fx_barkskin (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_barkskin(%2d)", fx->Opcode);
	if (target->SetSpellState( SS_BARKSKIN)) return FX_APPLIED;

	int bonus;
	if (fx->CasterLevel>6) {
		if (fx->CasterLevel>12) {
			bonus=5;
		} else {
			bonus=4;
		}
	} else {
		bonus=3;
	}
	target->AC.HandleFxBonus(bonus, fx->TimingMode==FX_DURATION_INSTANT_PERMANENT);

	if (enhanced_effects) {
		target->AddPortraitIcon(PI_BARKSKIN);
		target->SetGradient(2);
	}
	return FX_APPLIED;
}

//416 BleedingWounds
int fx_bleeding_wounds (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_bleeding_wounds(%2d): Damage: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	//also this effect is executed every update
	ieDword damage;
	int tmp = fx->Parameter1;

	switch(fx->Parameter2) {
	case RPD_PERCENT:
		damage = STAT_GET(IE_MAXHITPOINTS) * fx->Parameter1 / 100;
		break;
	case RPD_ROUNDS:
		tmp *= 6;
		goto seconds;
	case RPD_TURNS:
		tmp *= 30;
	case RPD_SECONDS:
seconds:
		damage = 1;
		if (tmp && (core->GetGame()->GameTime%tmp)) {
			return FX_APPLIED;
		}
		break;
	case RPD_POINTS:
		damage = fx->Parameter1;
		break;
	default:
		damage = 1;
		break;
	}
	//percent
	target->Damage(damage, DAMAGE_POISON, Owner, fx->IsVariable, fx->SavingThrowType);
	target->AddPortraitIcon(PI_BLEEDING);
	return FX_APPLIED;
}

//417 AreaEffect
//move these flags to a header file if used elsewhere
#define AE_REPEAT     1
#define AE_TARGETEXCL 2

int fx_area_effect (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_area_effect(%2d) Radius: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	//this effect ceases to affect dead targets (probably on frozen and stoned too)
	Game *game = core->GetGame();
	Map *map = NULL;

	if (target) {
		if (STATE_GET(STATE_DEAD) ) {
			return FX_NOT_APPLIED;
		}
		map = target->GetCurrentArea();
	} else {
		map = game->GetCurrentArea();
	}

	if (fx->FirstApply) {
		if (!fx->Parameter3) {
			fx->Parameter3=AI_UPDATE_TIME;
		} else {
			fx->Parameter3*=AI_UPDATE_TIME;
		}
		fx->Parameter4 = 0;
	}

	if (fx->Parameter4>=game->GameTime) {
		return FX_APPLIED;
	}

	fx->Parameter4 = game->GameTime+fx->Parameter3;
	Point pos(fx->PosX, fx->PosY);

	Spell *spell = gamedata->GetSpell(fx->Resource);
	if (!spell) {
		return FX_NOT_APPLIED;
	}

	EffectQueue *fxqueue = spell->GetEffectBlock(Owner, pos, 0, fx->CasterLevel);
	fxqueue->SetOwner(Owner);
	//bit 2 original target is excluded or not excluded
	fxqueue->AffectAllInRange(map, pos, 0, 0,fx->Parameter1, fx->Parameter2&AE_TARGETEXCL?target:NULL);
	delete fxqueue;

	//bit 1 repeat or only once
	if (fx->Parameter2&AE_REPEAT) {
		return FX_APPLIED;
	}
	return FX_NOT_APPLIED;
}

//418 FreeAction2
int fx_free_action_iwd2 (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_free_action_iwd2(%2d)", fx->Opcode);
	if (target->SetSpellState( SS_FREEACTION)) return FX_APPLIED;

	// immunity to the following effects, coded in the effects:
	// 0x9a Overlay:Entangle,       ok
	// 0x9d Overlay:Web             ok
	// 0x9e Overlay:Grease          ok
	// 0x6d State:Hold3             ok
	// 0x28 State:Slowed            ok
	// 0xb0 MovementRateModifier2   ok
	if (enhanced_effects) {
		target->AddPortraitIcon(PI_FREEACTION);
		target->SetColorMod(0xff, RGBModifier::ADD, 30, 0x80, 0x60, 0x60);
	}
	return FX_APPLIED;
}

//419 Unconsciousness
//same as the sleep effect, but different icon
int fx_unconsciousness (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_unconsciousness(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	STATE_SET(STATE_HELPLESS|STATE_SLEEP);
	if (fx->Parameter2) {
		target->SetSpellState(SS_NOAWAKE);
	}
	//
	if (enhanced_effects) {
		target->AddPortraitIcon(PI_UNCONSCIOUS);
	}
	return FX_APPLIED;
}

//420 Death2 (see in core effects)

//421 EntropyShield
int fx_entropy_shield (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_entropy_shield(%2d)", fx->Opcode);
	if (target->SetSpellState( SS_ENTROPY)) return FX_APPLIED;
	if (!fx->Resource[0]) {
		strnuprcpy(fx->Resource, "entropy", sizeof(ieResRef)-1);
	}
	//immunity to certain projectiles
	ieDword *EntropyProjectileList = core->GetListFrom2DA(fx->Resource);
	ieDword i = EntropyProjectileList[0];
	//the index is handled differently because
	//the list's first element is the element count
	while(i) {
		target->AddProjectileImmunity(EntropyProjectileList[i--]);
	}
	if (enhanced_effects) {
		target->AddPortraitIcon(PI_ENTROPY);
		//entropy shield overlay
		target->SetOverlay(OV_ENTROPY);
		target->SetColorMod(0xff, RGBModifier::ADD, 30, 0x40, 0xc0, 0x40);
	}
	return FX_APPLIED;
}

//422 StormShell
int fx_storm_shell (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_storm_shell(%2d)", fx->Opcode);
	if (target->SetSpellState(SS_STORMSHELL)) return FX_APPLIED;
	STAT_ADD(IE_RESISTFIRE, 15);
	STAT_ADD(IE_RESISTCOLD, 15);
	STAT_ADD(IE_RESISTELECTRICITY, 15);

	if (enhanced_effects) {
		target->SetOverlay(OV_STORMSHELL);
	}
	return FX_APPLIED;
}

//423 ProtectionFromElements
int fx_protection_from_elements (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_protection_from_elements(%2d)", fx->Opcode);
	if (target->SetSpellState( SS_ELEMPROT)) return FX_APPLIED;
	target->AddPortraitIcon(PI_ELEMPROT);
	STAT_ADD(IE_RESISTFIRE, 15);
	STAT_ADD(IE_RESISTCOLD, 15);
	STAT_ADD(IE_RESISTACID, 15);
	STAT_ADD(IE_RESISTELECTRICITY, 15);
	//compatible with 2nd ed
	STAT_ADD(IE_RESISTMAGICFIRE, 15);
	STAT_ADD(IE_RESISTMAGICCOLD, 15);

	if (enhanced_effects) {
		target->SetColorMod(0xff, RGBModifier::ADD, 0x4f, 0, 0, 0xc0);
	}
	return FX_APPLIED;
}

//424 HoldUndead (see in core effects, 0x6d)
//425 ControlUndead2 (see above)
//426 Aegis
int fx_aegis (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	ieDword tmp;

	if(0) print("fx_aegis(%2d)", fx->Opcode);
	//gives immunity against:
	//0xda stoneskin
	//0x9a entangle
	//0x9e grease
	//0x9d web
	//0x6d hold
	//0x28 slow

	if (target->SetSpellState( SS_AEGIS)) return FX_APPLIED;
	//deflection AC bonus
	//
	//physical damage reduction
	STAT_ADD(IE_RESISTSLASHING, 10);
	STAT_ADD(IE_RESISTCRUSHING, 10);
	STAT_ADD(IE_RESISTPIERCING, 10);

	//elemental damage reduction
	STAT_ADD(IE_RESISTFIRE, 15);
	STAT_ADD(IE_RESISTCOLD, 15);
	STAT_ADD(IE_RESISTELECTRICITY, 15);
	STAT_ADD(IE_RESISTACID, 15);

	//magic resistance
	STAT_ADD(IE_RESISTMAGIC, 3);

	//saving throws
	STAT_ADD(IE_SAVEFORTITUDE, 2);
	STAT_ADD(IE_SAVEWILL, 2);
	STAT_ADD(IE_SAVEREFLEX, 2);

	if (fx->FirstApply) {
		fx->Parameter1=8;
	}
	tmp = STAT_GET(IE_STONESKINS);
	if (fx->Parameter1>tmp) {
		STAT_SET(IE_STONESKINS, fx->Parameter1);
	}

	if (enhanced_effects) {
		target->AddPortraitIcon(PI_AEGIS);
		target->SetColorMod(0xff, RGBModifier::ADD, 30, 0x80, 0x60, 0x60);
		target->SetGradient(14);
	}

	return FX_APPLIED;
}

//427 ExecutionerEyes
int fx_executioner_eyes (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_executioner_eyes(%2d)", fx->Opcode);
	if (target->SetSpellState( SS_EXECUTIONER)) return FX_APPLIED;

	STAT_ADD(IE_CRITICALHITBONUS, 4);
	target->ToHit.HandleFxBonus(4, fx->TimingMode==FX_DURATION_INSTANT_PERMANENT);

	if (enhanced_effects) {
		target->AddPortraitIcon(PI_EXECUTIONER);
		target->SetGradient(8);
	}
	return FX_APPLIED;
}

//428 Death3 (also a death effect)

//429 EffectsOnStruck (a much simpler version of CastSpellOnCondition)
int fx_effects_on_struck (Scriptable* Owner, Actor* target, Effect* fx)
{
	Map *map = target->GetCurrentArea();
	if (!map) return FX_APPLIED;

	Actor *actor = map->GetActorByGlobalID(target->LastHitter);
	if (!actor) {
		return FX_APPLIED;
	}

	const TriggerEntry *entry = target->GetMatchingTrigger(trigger_hitby, TEF_PROCESSED_EFFECTS);
	if (entry!=NULL) {
		ieDword dist = GetSpellDistance(fx->Resource, target);
		if (!dist) return FX_APPLIED;
		if (PersonalDistance(target, actor) > dist) return FX_APPLIED;
		core->ApplySpell(fx->Resource, actor, Owner, fx->Power);
	}
	return FX_APPLIED;
}

//430 ProjectileUseEffectList
int fx_projectile_use_effect_list (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_projectile_use_effect_list(%2d) Type: %d Spell:%s", fx->Opcode, fx->Parameter2, fx->Resource);

	if (!Owner) {
		return FX_NOT_APPLIED;
	}
	Map *map = Owner->GetCurrentArea();
	if (!map) {
		return FX_NOT_APPLIED;
	}
	Spell* spl = gamedata->GetSpell( fx->Resource );
	//create projectile from known spellheader
	//cannot get the projectile from the spell
	Projectile *pro = core->GetProjectileServer()->GetProjectileByIndex(fx->Parameter2);

	if (pro) {
		Point p(fx->PosX, fx->PosY);

		pro->SetEffects(spl->GetEffectBlock(Owner, p, 0, fx->CasterLevel, fx->Parameter2));
		Point origin(fx->PosX, fx->PosY);
		pro->SetCaster(fx->CasterID, fx->CasterLevel);
		if (target) {
			map->AddProjectile( pro, origin, target->GetGlobalID(), false);
		} else {
			map->AddProjectile( pro, origin, origin);
		}
	}
	return FX_NOT_APPLIED;
}

//431 EnergyDrain

int fx_energy_drain (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_energy_drain(%2d) Type: %d", fx->Opcode, fx->Parameter1);
	if (!fx->Parameter1) {
		return FX_NOT_APPLIED;
	}
	if (fx->FirstApply) {
		//display string?

		//hitpoints don't have base/modified, only current
		BASE_SUB(IE_HITPOINTS, fx->Parameter1*5);
	}
	//if there is another energy drain effect (level drain), add them up
	STAT_ADD(IE_LEVELDRAIN, fx->Parameter1);
	STAT_SUB(IE_SAVEFORTITUDE, fx->Parameter1);
	STAT_SUB(IE_SAVEREFLEX, fx->Parameter1);
	STAT_SUB(IE_SAVEWILL, fx->Parameter1);
	//these saving throws don't exist in 2nd edition, but to make it portable, we should affect ALL saving throws
	STAT_SUB(IE_SAVEVSBREATH, fx->Parameter1);
	STAT_SUB(IE_SAVEVSSPELL, fx->Parameter1);
	STAT_SUB(IE_MAXHITPOINTS, fx->Parameter1*5);
	return FX_APPLIED;
}

//432 TortoiseShell
int fx_tortoise_shell (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_tortoise_shell(%2d) Hits: %d", fx->Opcode, fx->Parameter1);
	if (!fx->Parameter1) {
		return FX_NOT_APPLIED;
	}

	if (target->SetSpellState( SS_TORTOISE)) return FX_NOT_APPLIED;
	if (enhanced_effects) {
		target->AddPortraitIcon(PI_TORTOISE);
		target->SetOverlay(OV_TORTOISE);
	}
	target->SetSpellState(SS_HELD);
	STATE_SET(STATE_HELPLESS);
	return FX_APPLIED;
}

//433 Blink
int fx_blink (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_blink(%2d) Type: %d", fx->Opcode, fx->Parameter2);

	if (target->SetSpellState( SS_BLINK)) return FX_APPLIED;

	//pulsating translucence (like with invisibility)
	ieDword Trans = fx->Parameter4;
	if (fx->Parameter3) {
		if (Trans>=240) {
			fx->Parameter3 = 0;
		} else {
			Trans+=16;
		}
	} else {
		if (Trans<=32) {
			fx->Parameter3=1;
		} else {
			Trans-=16;
		}
	}
	fx->Parameter4=Trans;
	STAT_SET( IE_TRANSLUCENT, Trans);

	if(fx->Parameter2) {
		target->AddPortraitIcon(PI_EMPTYBODY);
		return FX_APPLIED;
	}

	target->AddPortraitIcon(PI_BLINK);
	return FX_APPLIED;
}

//434 PersistentUseEffectList
int fx_persistent_use_effect_list (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_persistent_use_effect_list(%2d) Interval: %d Resource: %.8s", fx->Opcode, fx->Parameter1, fx->Resource);
	if (fx->Parameter3) {
		fx->Parameter3--;
	} else {
		core->ApplySpell(fx->Resource, target, Owner, fx->Power);
		fx->Parameter3=fx->Parameter1;
	}
	return FX_APPLIED;
}

//435 DayBlindness
//for this effect to work, apply it repeatedly on targets
int fx_day_blindness (Scriptable* Owner, Actor* target, Effect* fx)
{
	if(0) print("fx_day_blindness(%2d) Amount: %d", fx->Opcode, fx->Parameter2);
	Map *map = target->GetCurrentArea();
	if (!map) {
		return FX_NOT_APPLIED;
	}

	//extended night IS needed, even though IWD2's extended night isn't the same as in BG2 (no separate tileset)
	if ((map->AreaType&(AT_OUTDOOR|AT_DAYNIGHT|AT_EXTENDED_NIGHT)) == AT_EXTENDED_NIGHT) {
		return FX_NOT_APPLIED;
	}
	//drop the effect when day came
	if (!core->GetGame()->IsDay()) {
		return FX_NOT_APPLIED;
	}

	//don't let it work twice
	if (target->SetSpellState(SS_DAYBLINDNESS)) {
		return FX_NOT_APPLIED;
	}

	// medium hack (better than original)
	// the original used explicit race/subrace values
	ieDword penalty;

	//the original engine let the effect stay on non affected races, doing the same so the spell state sticks
	if (check_iwd_targeting(Owner, target, 0, 82)) penalty = 1; //dark elf
	else if (check_iwd_targeting(Owner, target, 0, 84)) penalty = 2; //duergar
	else return FX_APPLIED;

	target->AddPortraitIcon(PI_DAYBLINDNESS);

	//saving throw penalty (bigger is better in iwd2)
	STAT_SUB(IE_SAVEFORTITUDE, penalty);
	STAT_SUB(IE_SAVEREFLEX, penalty);
	STAT_SUB(IE_SAVEWILL, penalty);
	//for compatibility reasons
	STAT_SUB(IE_SAVEVSBREATH, penalty);
	STAT_SUB(IE_SAVEVSSPELL, penalty);
	//bigger is better in iwd2
	target->ToHit.HandleFxBonus(-penalty, fx->TimingMode==FX_DURATION_INSTANT_PERMANENT);

	//decrease all skills by 1
	for(int i=0;i<32;i++) {
		int stat = target->GetSkillStat(i);
		if (stat<0) break;
		STAT_SUB(stat, 1);
	}
	return FX_APPLIED;
}

//436 DamageReduction
int fx_damage_reduction (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_damage_reduction(%2d) Hits: %d  Strength: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (!fx->Parameter1) return FX_NOT_APPLIED;
	//TODO: didn't set the pluses stored in Parameter2
	STAT_SET(IE_RESISTSLASHING, fx->Parameter1);
	STAT_SET(IE_RESISTCRUSHING, fx->Parameter1);
	STAT_SET(IE_RESISTPIERCING, fx->Parameter1);
	return FX_APPLIED;
}

//437 Disguise
//modifies character animations to look like clerics of same gender/race
int fx_disguise (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_disguise(%2d) Amount: %d", fx->Opcode, fx->Parameter2);
	if (fx->Parameter1) {
		//
		if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
			BASE_SET(IE_ANIMATION_ID, fx->Parameter1);
		} else {
			STAT_SET(IE_ANIMATION_ID, fx->Parameter1);
		}
		return FX_PERMANENT;
	}

	ieDword anim = BASE_GET(IE_ANIMATION_ID);
	if (anim>=0x6000 || anim<=0x6fff) {
		STAT_SET(IE_ANIMATION_ID, anim&0x600f);
		return FX_APPLIED;
	}

	if (anim>=0x5000 || anim<=0x5fff) {
		STAT_SET(IE_ANIMATION_ID, anim&0x500f);
		return FX_APPLIED;
	}
	//set avatar anim?
	return FX_NOT_APPLIED;
}

//438 HeroicInspiration
int fx_heroic_inspiration (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_heroic_inspiration(%2d)", fx->Opcode);

	if (target->GetSafeStat(IE_HITPOINTS)*2>=target->GetSafeStat(IE_MAXHITPOINTS)) return FX_APPLIED;

	target->AddPortraitIcon(PI_HEROIC);
	//+1 bab and damage
	STAT_ADD( IE_DAMAGEBONUS, 1);
	STAT_ADD( IE_HITBONUS, 1);
	//+1 to all saves
	STAT_ADD( IE_SAVEFORTITUDE, 1);
	STAT_ADD( IE_SAVEREFLEX, 1);
	STAT_ADD( IE_SAVEWILL, 1);
	//make it compatible with 2nd edition
	STAT_ADD( IE_SAVEVSBREATH, 1);
	STAT_ADD( IE_SAVEVSSPELL, 1);

	return FX_APPLIED;
}
//439 PreventAISlowDown
//same as BG2 OffscreenAIModifier

//440 BarbarianRage
int fx_barbarian_rage (Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if(0) print("fx_barbarian_rage(%2d) Amount:%d", fx->Opcode, fx->Parameter1);
	//TODO: implement this
	return FX_APPLIED;
}

//441 MovementRateModifier4 (same as others)

//442 Cleave
int fx_cleave (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_cleave(%2d) Amount:%d", fx->Opcode, fx->Parameter1);
	//just remain dormant after first apply for the remaining duration (possibly disabling more cleaves)
	if (!fx->FirstApply) return FX_APPLIED;
	Map *map = target->GetCurrentArea();
	if (!map) return FX_NOT_APPLIED;

	//reset attackcount to a previous number and hack the current opponent to another enemy nearby
	//SeeCore returns the closest living enemy
	//FIXME:the previous opponent must be dead by now, or this code won't work
	if (SeeCore(target, Enemy, false) ) {
		Actor *enemy = map->GetActorByGlobalID(target->LastSeen);
		//50 is more like our current weapon range
		if (enemy && (PersonalDistance(enemy, target)<50) && target->LastSeen!=target->LastTarget) {
			displaymsg->DisplayConstantStringNameValue(STR_CLEAVE, DMC_WHITE, target, fx->Parameter1);
			target->attackcount=fx->Parameter1;
			target->FaceTarget(enemy);
			target->LastTarget=target->LastSeen;
			//linger around for more
			return FX_APPLIED;
		}
	}
	//no opponent found, nothing to do
	return FX_NOT_APPLIED;
}

//443 MissileDamageReduction
int fx_missile_damage_reduction (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_missile_damage_reduction(%2d) Amount:%d", fx->Opcode, fx->Parameter1);
	if (!fx->Parameter1) fx->Parameter1=10;
	STAT_SET(IE_RESISTMISSILE, fx->Parameter1);
	//TODO: didn't set the pluses stored in Parameter2
	return FX_APPLIED;
}

//444 TensersTransformation
int fx_tenser_transformation (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_tenser_transformation(%2d)", fx->Opcode);
	if (target->SetSpellState( SS_TENSER)) return FX_APPLIED;

	if (fx->FirstApply) {
		fx->Parameter3=core->Roll(fx->CasterLevel, 6, 0);
		fx->Parameter4=core->Roll(2,4,0);
		fx->Parameter5=core->Roll(2,4,0);
		BASE_ADD( IE_HITPOINTS, fx->Parameter3);
	}

	target->AC.HandleFxBonus(4, fx->TimingMode==FX_DURATION_INSTANT_PERMANENT);
	STAT_ADD(IE_SAVEFORTITUDE, 5); //FIXME: this is a bonus
	STAT_ADD(IE_MAXHITPOINTS, fx->Parameter3);
	STAT_ADD(IE_STR, fx->Parameter4);
	STAT_ADD(IE_CON, fx->Parameter5);

	if (enhanced_effects) {
		target->AddPortraitIcon(PI_TENSER);
		target->SetGradient(0x3e);
	}

	return FX_APPLIED;
}

//445 SlipperyMind (the original removed charm when the effect itself was about to be removed)

int fx_slippery_mind (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_slippery_mind(%2d)", fx->Opcode);
	target->fxqueue.RemoveAllEffects(fx_charm_ref);
	return FX_NOT_APPLIED;
}

//446 SmiteEvil
int fx_smite_evil (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_smite_evil(%2d)", fx->Opcode);
	target->SetSpellState(SS_SMITEEVIL);
	return FX_NOT_APPLIED;
}

//447 Restoration
int fx_restoration (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_restoration(%2d)", fx->Opcode);
	target->fxqueue.RemoveAllEffectsWithParam(fx_disease_ref, 4);
	target->fxqueue.RemoveAllEffectsWithParam(fx_disease_ref, 5);
	target->fxqueue.RemoveAllEffectsWithParam(fx_disease_ref, 6);
	target->fxqueue.RemoveAllEffectsWithParam(fx_disease_ref, 7);
	target->fxqueue.RemoveAllEffectsWithParam(fx_disease_ref, 8);
	target->fxqueue.RemoveAllEffectsWithParam(fx_disease_ref, 9);
	target->fxqueue.RemoveAllEffectsWithParam(fx_disease_ref, 13);
	target->fxqueue.RemoveAllEffectsWithParam(fx_disease_ref, 14);
	target->fxqueue.RemoveAllEffectsWithParam(fx_disease_ref, 15);

	target->fxqueue.RemoveAllDetrimentalEffects(fx_str_ref, BASE_GET(IE_STR));
	target->fxqueue.RemoveAllDetrimentalEffects(fx_int_ref, BASE_GET(IE_INT));
	target->fxqueue.RemoveAllDetrimentalEffects(fx_wis_ref, BASE_GET(IE_WIS));
	target->fxqueue.RemoveAllDetrimentalEffects(fx_con_ref, BASE_GET(IE_CON));
	target->fxqueue.RemoveAllDetrimentalEffects(fx_dex_ref, BASE_GET(IE_DEX));
	target->fxqueue.RemoveAllDetrimentalEffects(fx_cha_ref, BASE_GET(IE_CHR));
	return FX_NOT_APPLIED;
}

//448 AlicornLance
int fx_alicorn_lance (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_alicorn_lance(%2d)", fx->Opcode);
	if (target->SetSpellState( SS_ALICORNLANCE)) return FX_APPLIED;
	////target->AddPortraitIcon(PI_ALICORN); //no portrait icon
	target->AC.HandleFxBonus(-2, fx->TimingMode==FX_DURATION_INSTANT_PERMANENT);
	//color glow
	if (enhanced_effects) {
		target->SetColorMod(0xff, RGBModifier::ADD, 1, 0xb9, 0xb9, 0xb9);
	}
	return FX_APPLIED;
}

//449 CallLightning
Actor *GetRandomEnemySeen(Map *map, Actor *origin)
{
	int type = GetGroup(origin);

	if (type==2) {
		return NULL; //no enemies
	}
	int i = map->GetActorCount(true);
	//see a random enemy
	int pos = core->Roll(1,i,-1);
	i -= pos;
	while(i--) {
		Actor *ac = map->GetActor(i,true);
		if (!CanSee(origin, ac, true, GA_NO_DEAD|GA_NO_HIDDEN)) continue;
		if (type) { //origin is PC
			if (ac->GetStat(IE_EA) >= EA_EVILCUTOFF) {
				return ac;
			}
		}
		else {
			if (ac->GetStat(IE_EA) <= EA_GOODCUTOFF) {
				return ac;
			}
		}
	}

	i=map->GetActorCount(true);
	while(i--!=pos) {
		Actor *ac = map->GetActor(i,true);
		if (!CanSee(origin, ac, true, GA_NO_DEAD|GA_NO_HIDDEN)) continue;
		if (type) { //origin is PC
			if (ac->GetStat(IE_EA) >= EA_EVILCUTOFF) {
				return ac;
			}
		}
		else {
			if (ac->GetStat(IE_EA) <= EA_GOODCUTOFF) {
				return ac;
			}
		}
	}

	return NULL;
}

int fx_call_lightning (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_call_lightning(%2d)", fx->Opcode);

	//this effect ceases to affect dead targets (probably on frozen and stoned too)
	if (STATE_GET(STATE_DEAD)) {
		return FX_NOT_APPLIED;
	}
	int ret = FX_APPLIED;

	Map *map = target->GetCurrentArea();
	if (!map) return ret;

	if (fx->Parameter1<=1) {
		ret = FX_NOT_APPLIED;
	}

	//timing
	fx->TimingMode=FX_DURATION_DELAY_PERMANENT;
	fx->Duration=core->GetGame()->GameTime+70*AI_UPDATE_TIME;
	fx->Parameter1--;

	//calculate victim (an opponent of target)
	Actor *victim = GetRandomEnemySeen(map, target);
	if (!victim) {
		displaymsg->DisplayConstantStringName(STR_LIGHTNING_DISS, DMC_WHITE, target);
		return ret;
	}

	//iwd2 style
	if (fx->Resource[0]) {
		core->ApplySpell(fx->Resource, victim, target, fx->Power);
		return ret;
	}

	//how style
	victim->Damage(DICE_ROLL(0), DAMAGE_ELECTRICITY, target, fx->IsVariable, fx->SavingThrowType);
	return ret;
}

//450 GlobeInvulnerability
int fx_globe_invulnerability (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_globe_invulnerability(%2d)", fx->Opcode);
	int state;
	int icon;
	int value;
	int overlay;

	if (fx->Parameter2) {
		state = SS_MAJORGLOBE;
		icon = PI_MAJORGLOBE;
		value = 30; //if globe is needed, use 31
		overlay = OV_GLOBE;
	} else {
		state = SS_MINORGLOBE;
		icon = PI_MINORGLOBE;
		value = 14; //if globe is needed use 15
		overlay = OV_MINORGLOBE;
	}
	if (target->SetSpellState( state)) return FX_APPLIED;

	STAT_BIT_OR(IE_MINORGLOBE, value);
	if (enhanced_effects) {
		target->AddPortraitIcon(icon);
		target->SetOverlay(overlay);
	}
	return FX_APPLIED;
}

//451 LowerResistance
//the original effect has two unwanted quirks
//1. strength is always caster level * 2
//2. non cumulative, which causes stronger effects canceled out
int fx_lower_resistance (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_lower_resistance(%2d)", fx->Opcode);
	int modifier;

	switch(fx->Parameter2) {
	case 0: default:
		//original IWD2 style
		if (target->SetSpellState( SS_LOWERRESIST)) return FX_APPLIED;
		modifier = fx->CasterLevel * 2;
		if (modifier>50) modifier = 50;
		break;
	case 1:
		//like IWD2, but stronger effects are not canceled out
		target->SetSpellState( SS_LOWERRESIST);
		modifier = fx->CasterLevel * 2;
		if (modifier>50) modifier = 50;
		break;
	case 2:
		//GemRB style, non cumulative
		if (target->SetSpellState( SS_LOWERRESIST)) return FX_APPLIED;
		modifier = fx->Parameter1;
		break;
	case 3:
		//GemRB style, cumulative
		target->SetSpellState( SS_LOWERRESIST);
		modifier = fx->Parameter1;
		break;
	}
	//FIXME: the original never goes below zero, but i think it is prone to bugs that way
	//it is easier to handle this by simply not printing negative values
	STAT_SUB(IE_RESISTMAGIC, modifier);
	return FX_APPLIED;
}

//452 Bane

int fx_bane (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_bane(%2d)", fx->Opcode);

	if (target->SetSpellState( SS_BANE)) return FX_NOT_APPLIED;
	//do this once
	if (fx->FirstApply)
		target->fxqueue.RemoveAllEffects(fx_bless_ref);
	if (enhanced_effects) {
		target->AddPortraitIcon(PI_BANE);
		target->SetColorMod(0xff, RGBModifier::ADD, 20, 0, 0, 0x80);
	}
	target->ToHit.HandleFxBonus(-fx->Parameter1, fx->TimingMode==FX_DURATION_INSTANT_PERMANENT);
	STAT_ADD( IE_MORALEBREAK, -fx->Parameter1);
	return FX_APPLIED;
}

//453 PowerAttack
int fx_power_attack (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_power_attack(%2d)", fx->Opcode);

	if (!target->HasFeat(FEAT_POWER_ATTACK)) return FX_NOT_APPLIED;
	if (!target->PCStats) return FX_NOT_APPLIED;

	ieDword x=target->PCStats->ExtraSettings[ES_POWERATTACK];
	if (x) {
		if (target->SetSpellState(SS_POWERATTACK+x)) return FX_NOT_APPLIED;
		if (fx->FirstApply) {
			//disable mutually exclusive feats
			target->PCStats->ExtraSettings[ES_EXPERTISE] = 0;

			//set new modal feat
			displaymsg->DisplayConstantStringNameString(STR_USING_FEAT, DMC_WHITE, STR_POWERATTACK, target);
		}
	}

	displaymsg->DisplayConstantStringNameString(STR_STOPPED_FEAT, DMC_WHITE, STR_POWERATTACK, target);
	return FX_NOT_APPLIED;
}

//454 Expertise
int fx_expertise (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
//expertise feat:
//convert positive base attack bonus into AC (dodge bonus)
//up to feat_expertise count (player's choice)
	if(0) print("fx_expertise(%2d)", fx->Opcode);

	if (!target->HasFeat(FEAT_EXPERTISE)) return FX_NOT_APPLIED;
	if (!target->PCStats) return FX_NOT_APPLIED;

	ieDword x=target->PCStats->ExtraSettings[ES_EXPERTISE];
	if (x) {
		if (target->SetSpellState(SS_EXPERTISE+x)) return FX_NOT_APPLIED;
		if (fx->FirstApply) {
			//disable mutually exclusive feats
			target->PCStats->ExtraSettings[ES_POWERATTACK] = 0;

			//set new modal feat
			displaymsg->DisplayConstantStringNameString(STR_USING_FEAT, DMC_WHITE, STR_EXPERTISE, target);
		}
	}

	displaymsg->DisplayConstantStringNameString(STR_STOPPED_FEAT, DMC_WHITE, STR_EXPERTISE, target);
	return FX_NOT_APPLIED;
}

//455 ArterialStrike
//apply arterial strike spell on backstab, this is by default the same as in iwd2
int fx_arterial_strike (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_arterial_strike(%2d)", fx->Opcode);
	//arterial strike doesn't work for npcs?
	if (!target->HasFeat(FEAT_ARTERIAL_STRIKE)) return FX_NOT_APPLIED;
	if (!target->PCStats) return FX_NOT_APPLIED;

	if (target->PCStats->ExtraSettings[ES_ARTERIAL]) {
		if (target->SetSpellState( SS_ARTERIAL)) return FX_NOT_APPLIED; //don't apply it twice

		if (fx->FirstApply) {
			if (!fx->Resource[0]) {
				strnuprcpy(fx->Resource, "artstr", sizeof(ieResRef)-1);
			}
			//disable mutually exclusive feats
			target->PCStats->ExtraSettings[ES_HAMSTRING] = 0;

			//set new modal feat
			displaymsg->DisplayConstantStringNameString(STR_USING_FEAT, DMC_WHITE, STR_ARTERIAL, target);
		}
		if (target->BackstabResRef[0]=='*') {
			memcpy(target->BackstabResRef, fx->Resource, sizeof(ieResRef));
		}
		return FX_APPLIED;
	}

	//stop arterial
	displaymsg->DisplayConstantStringNameString(STR_STOPPED_FEAT, DMC_WHITE, STR_ARTERIAL, target);
	return FX_NOT_APPLIED;
}

//456 HamString
//apply hamstring spell on backstab, this is by default the same as in iwd2
int fx_hamstring (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_hamstring(%2d)", fx->Opcode);
	//hamstring doesn't work for npcs?
	if (!target->HasFeat(FEAT_HAMSTRING)) return FX_NOT_APPLIED;
	if (!target->PCStats) return FX_NOT_APPLIED;

	if (target->PCStats->ExtraSettings[ES_HAMSTRING]) {
		if (target->SetSpellState( SS_HAMSTRING)) return FX_NOT_APPLIED; //don't apply it twice

		if (fx->FirstApply) {
			if (!fx->Resource[0]) {
				strnuprcpy(fx->Resource, "hamstr", sizeof(ieResRef)-1);
			}
			//disable mutually exclusive feats
			target->PCStats->ExtraSettings[ES_ARTERIAL] = 0;

			//set new modal feat
			displaymsg->DisplayConstantStringNameString(STR_USING_FEAT, DMC_WHITE, STR_HAMSTRING, target);
		}
		if (target->BackstabResRef[0]=='*') {
			memcpy(target->BackstabResRef, fx->Resource, sizeof(ieResRef));
		}
		return FX_APPLIED;
	}

	//stop hamstring
	displaymsg->DisplayConstantStringNameString(STR_STOPPED_FEAT, DMC_WHITE, STR_HAMSTRING, target);
	return FX_NOT_APPLIED;
}

//457 RapidShot
int fx_rapid_shot (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if(0) print("fx_rapid_shot(%2d)", fx->Opcode);
	//rapid shot doesn't work for npcs?
	if (!target->HasFeat(FEAT_RAPID_SHOT)) return FX_NOT_APPLIED;
	if (!target->PCStats) return FX_NOT_APPLIED;

	if (target->PCStats->ExtraSettings[ES_RAPIDSHOT]) {
		if (target->SetSpellState( SS_RAPIDSHOT)) return FX_NOT_APPLIED; //don't apply it twice

		target->ToHit.HandleFxBonus(-2, fx->TimingMode==FX_DURATION_INSTANT_PERMANENT);
		if (fx->FirstApply) {
			//disable mutually exclusive feats
			//none i know of

			//set new modal feat
			displaymsg->DisplayConstantStringNameString(STR_USING_FEAT, DMC_WHITE, STR_RAPIDSHOT, target);
		}

		return FX_APPLIED;
	}

	//stop rapidshot
	displaymsg->DisplayConstantStringNameString(STR_STOPPED_FEAT, DMC_WHITE, STR_RAPIDSHOT, target);
	return FX_NOT_APPLIED;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x4F172B2, "Effect opcodes for the icewind branch of the games")
PLUGIN_INITIALIZER(RegisterIWDOpcodes)
PLUGIN_CLEANUP(Cleanup)
END_PLUGIN()
