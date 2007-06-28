/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Id$
 *
 */

#include "../../includes/win32def.h"
#include "../../includes/overlays.h"
#include "../Core/Actor.h"
#include "../Core/EffectQueue.h"
#include "../Core/Interface.h"
#include "../Core/Game.h"
#include "../Core/Spell.h"
#include "../Core/damages.h"
#include "../Core/GSUtils.h"  //needs for displaystringcore
#include "IWDOpc.h"

static ieResRef *iwd_spell_hits = NULL;

//resources for the seven eyes effect
#define EYE_MIND   0
#define EYE_SWORD  1
#define EYE_MAGE   2
#define EYE_VENOM  3
#define EYE_SPIRIT 4
#define EYE_FORT   5
#define EYE_STONE  6

static ieResRef SevenEyes[7]={"spin126","spin127","spin128","spin129","spin130","spin131","spin132"};

static bool enhanced_effects = false;
static int shcount = -1;

#define PI_CONFUSION    3
#define PI_PROTFROMEVIL 9
#define PI_FREEACTION   19
#define PI_BARKSKIN     20
#define PI_PANIC        36
#define PI_NAUSEA       43
#define PI_HOPELESSNESS 44
#define PI_RIGHTEOUS    67
#define PI_ELEMENTS     76
#define PI_FAITHARMOR   84
#define PI_BLEEDING     85
#define PI_HOLYPOWER    86
#define PI_DEATHWARD    87
#define PI_UNCONSCIOUS  88
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
#define PI_EMPTYBODY    145

#define SS_HOPELESSNESS 0
#define SS_PROTFROMEVIL 1
#define SS_ARMOROFFAITH 2
#define SS_NAUSEA       3
#define SS_ENFEEBLED    4
#define SS_FIRESHIELD   5
#define SS_ICESHIELD    6
#define SS_HELD         7
#define SS_DEATHWARD    8
#define SS_HOLYPOWER    9
#define SS_GOODCHANT    10
#define SS_BADCHANT     11
#define SS_GOODPRAYER   12
#define SS_BADPRAYER    13
#define SS_GOODRECIT    14
#define SS_BADRECIT     15
#define SS_RIGHTEOUS    16    //allied
#define SS_RIGHTEOUS2   17    //allied and same alignment
#define SS_STONESKIN    18
#define SS_IRONSKIN     19
#define SS_SANCTUARY    20
#define SS_RESILIENT    21  
#define SS_BLESS        22
#define SS_AID          23
#define SS_BARKSKIN     24
#define SS_HOLYMIGHT    25
#define SS_ENTANGLE     26
#define SS_WEB          27
#define SS_GREASE       28
#define SS_FREEACTION   29
#define SS_ENTROPY      30
#define SS_STORMSHELL   31
#define SS_ELEMPROT     32
#define SS_BERSERK      33
#define SS_BLOODRAGE    34
#define SS_NOHPINFO     35
#define SS_NOAWAKE      36
#define SS_AWAKE        37
#define SS_DEAF         38
#define SS_ANIMALRAGE   39
#define SS_NOBACKSTAB   40
#define SS_CHAOTICCMD   41
#define SS_MISCAST      42
#define SS_PAIN         43
#define SS_MALISON      44
//#define SS_CATSGRACE    45   //used explicitly
#define SS_MOLDTOUCH    46
#define SS_FLAMESHROUD  47
#define SS_EYEMIND      48
#define SS_EYESWORD     49
#define SS_EYEMAGE      50
#define SS_EYEVENOM     51
#define SS_EYESPIRIT    52
#define SS_EYEFORTITUDE 53
#define SS_EYESTONE     54
#define SS_AEGIS        55
#define SS_EXECUTIONER  56
#define SS_ENERGYDRAIN  57
#define SS_TORTOISE     58
#define SS_BLINK        59
#define SS_MINORGLOBE   60
#define SS_PROTFROMMISS 61
#define SS_GHOSTARMOR   62
#define SS_REFLECTION   63
#define SS_KAI          64
#define SS_CALLEDSHOT   65
#define SS_MIRRORIMAGE  66
#define SS_TURNED       67
#define SS_BLADEBARRIER 68
#define SS_POISONWEAPON 69
#define SS_STUNNINGBLOW 70
#define SS_QUIVERPALM   71
#define SS_DOMINATION   72
#define SS_MAJORGLOBE   73
#define SS_SHIELD       74
#define SS_ANTIMAGIC    75
#define SS_POWERATTACK  76
//more powerattack
#define SS_EXPERTISE    81
//more expertise
#define SS_ARTERIAL     86
#define SS_HAMSTRING    87
#define SS_RAPIDSHOT    88
#define SS_IRONBODY     89
#define SS_TENSER       90
#define SS_SMITEEVIL    91
#define SS_ALICORNLANCE 92
#define SS_LIGHTNING    93
#define SS_CHAMPIONS    94
#define SS_BONECIRCLE   95
#define SS_CLOAKOFFEAR  96
#define SS_PESTILENCE   97
#define SS_CONTAGION    98
#define SS_BANE         99
#define SS_DEFENSIVE    100
#define SS_DESTRUCTION  101
#define SS_DOLOROUS     102
#define SS_DOOM         103
#define SS_EXALTATION   104
#define SS_FAERIEFIRE   105
#define SS_FINDTRAPS    106
#define SS_GREATERLATH  107
#define SS_MAGICRESIST  108
#define SS_NPROTECTION  109
#define SS_PROTFROMFIRE 110
#define SS_PROTFROMLIGHTNING 111
#define SS_ELEMENTAL    112
#define SS_LATHANDER    113
#define SS_SLOWPOISON   114
#define SS_SPELLSHIELD  115
#define SS_STATICCHARGE 116
#define SS_LOWERRESIST  140

//tested for this, splstate is wrong or this entry has two uses
#define SS_DAYBLINDNESS 178   

static int fx_ac_vs_damage_type_modifier_iwd2 (Actor* Owner, Actor* target, Effect* fx);//0
static int fx_draw_upon_holy_might (Actor* Owner, Actor* target, Effect* fx);//84 (iwd2)
static int fx_ironskins (Actor* Owner, Actor* target, Effect* fx);//da (iwd2)
static int fx_fade_rgb (Actor* Owner, Actor* target, Effect* fx);//e8
static int fx_iwd_visual_spell_hit (Actor* Owner, Actor* target, Effect* fx);//e9
static int fx_cold_damage (Actor* Owner, Actor* target, Effect* fx);//ea
//int fx_iwd_casting_glow (Actor* Owner, Actor* target, Effect* fx);//eb
static int fx_chill_touch (Actor* Owner, Actor* target, Effect* fx);//ec (how)
static int fx_chill_touch_panic (Actor* Owner, Actor* target, Effect* fx);//ec (iwd2)
static int fx_crushing_damage (Actor* Owner, Actor* target, Effect* fx);//ed
static int fx_save_bonus (Actor* Owner, Actor* target, Effect* fx); //ee
static int fx_slow_poison (Actor* Owner, Actor* target, Effect* fx); //ef
static int fx_iwd_monster_summoning (Actor* Owner, Actor* target, Effect* fx); //f0
static int fx_vampiric_touch (Actor* Owner, Actor* target, Effect* fx); //f1
// int fx_overlay f2 (same as PST)
static int fx_animate_dead (Actor* Owner, Actor* target, Effect* fx);//f3
//int fx_prayer (Actor* Owner, Actor* target, Effect* fx); //f4 pst
//int fx_prayer_bad (Actor* Owner, Actor* target, Effect* fx); //f5 pst
static int fx_summon_monster2 (Actor* Owner, Actor* target, Effect* fx); //f6
static int fx_burning_blood (Actor* Owner, Actor* target, Effect* fx); //f7 iwd
static int fx_burning_blood2 (Actor* Owner, Actor* target, Effect* fx); //f7 how, iwd2
static int fx_summon_shadow_monster (Actor* Owner, Actor* target, Effect* fx); //f8
static int fx_recitation (Actor* Owner, Actor* target, Effect* fx); //f9
static int fx_recitation_bad (Actor* Owner, Actor* target, Effect* fx); //fa
static int fx_lich_touch (Actor* Owner, Actor* target, Effect* fx); //fb
static int fx_blinding_orb (Actor* Owner, Actor* target, Effect* fx); //fc
// ac vs damage //fd
static int fx_remove_effects (Actor* Owner, Actor* target, Effect* fx); //fe
static int fx_salamander_aura (Actor* Owner, Actor* target, Effect* fx); //ff
static int fx_umberhulk_gaze (Actor* Owner, Actor* target, Effect* fx); //100
static int fx_zombielord_aura (Actor* Owner, Actor* target, Effect* fx); //101
static int fx_resist_spell (Actor* Owner, Actor* target, Effect* fx); //102
static int fx_summon_creature2 (Actor* Owner, Actor* target, Effect* fx); //103
//int fx_avatar_removal (Actor* Owner, Actor* target, Effect* fx); //104
//int fx_immunity_effect2 (Actor* Owner, Actor* target, Effect* fx); //105
static int fx_summon_pomab (Actor* Owner, Actor* target, Effect* fx); //106
static int fx_control_undead (Actor* Owner, Actor* target, Effect* fx); //107
static int fx_static_charge (Actor* Owner, Actor* target, Effect* fx); //108
static int fx_cloak_of_fear (Actor* Owner, Actor* target, Effect* fx); //109
//int fx_movement_modifier (Actor* Owner, Actor* target, Effect* fx); //10a
static int fx_remove_confusion (Actor* Owner, Actor* target, Effect* fx);//10b
static int fx_eye_of_the_mind (Actor* Owner, Actor* target, Effect* fx);//10c
static int fx_eye_of_the_sword (Actor* Owner, Actor* target, Effect* fx);//10d
static int fx_eye_of_the_mage (Actor* Owner, Actor* target, Effect* fx);//10e
static int fx_eye_of_venom (Actor* Owner, Actor* target, Effect* fx);//10f
static int fx_eye_of_the_spirit (Actor* Owner, Actor* target, Effect* fx);//110
static int fx_eye_of_fortitude (Actor* Owner, Actor* target, Effect* fx);//111
static int fx_eye_of_stone (Actor* Owner, Actor* target, Effect* fx);//112
static int fx_remove_seven_eyes (Actor* Owner, Actor* target, Effect* fx);//113
static int fx_remove_effect (Actor* Owner, Actor* target, Effect* fx);//114
static int fx_soul_eater (Actor* Owner, Actor* target, Effect* fx);//115
static int fx_shroud_of_flame (Actor* Owner, Actor* target, Effect* fx);//116
static int fx_shroud_of_flame2 (Actor* Owner, Actor* target, Effect* fx);//116
static int fx_animal_rage (Actor* Owner, Actor* target, Effect* fx);//117
static int fx_turn_undead (Actor* Owner, Actor* target, Effect* fx);//118
static int fx_turn_undead2 (Actor* Owner, Actor* target, Effect* fx);//118
static int fx_vitriolic_sphere (Actor* Owner, Actor* target, Effect* fx);//119
static int fx_suppress_hp (Actor* Owner, Actor* target, Effect* fx);//11a
static int fx_floattext (Actor* Owner, Actor* target, Effect* fx);//11b
static int fx_mace_of_disruption (Actor* Owner, Actor* target, Effect* fx);//11c
//0x11d Sleep2 ??? power word sleep?
//0x11e Reveal:Tracks (same as bg2)
//0x11f Protection:Backstab (same as bg2)
static int fx_set_state (Actor* Owner, Actor* target, Effect* fx); //120
static int fx_cutscene (Actor* Owner, Actor* target, Effect* fx);//121
static int fx_resist_spell (Actor* Owner, Actor* target, Effect *fx);//ce
static int fx_resist_spell_and_message (Actor* Owner, Actor* target, Effect *fx);//122
static int fx_rod_of_smithing (Actor* Owner, Actor* target, Effect* fx); //123
//0x124 MagicalRest (same as bg2)
//0x125 BeholderDispelMagic (???)
static int fx_harpy_wail (Actor* Owner, Actor* target, Effect* fx); //126
static int fx_jackalwere_gaze (Actor* Owner, Actor* target, Effect* fx); //127
//0x128 ModifyGlobalVariable (same as bg2)
//0x129 HideInShadows (same as bg2)
static int fx_use_magic_device_modifier (Actor* Owner, Actor* target, Effect* fx);//12a

//iwd2 specific effects
static int fx_hopelessness (Actor* Owner, Actor* target, Effect* fx);//400
static int fx_protection_from_evil (Actor* Owner, Actor* target, Effect* fx);//401
static int fx_add_effects_list (Actor* Owner, Actor* target, Effect* fx);//402
static int fx_armor_of_faith (Actor* Owner, Actor* target, Effect* fx);//403
static int fx_nausea (Actor* Owner, Actor* target, Effect* fx); //404
static int fx_enfeeblement (Actor* Owner, Actor* target, Effect* fx); //405
static int fx_fireshield (Actor* Owner, Actor* target, Effect* fx); //406
static int fx_death_ward (Actor* Owner, Actor* target, Effect* fx); //407
static int fx_holy_power (Actor* Owner, Actor* target, Effect* fx); //408
static int fx_righteous_wrath (Actor* Owner, Actor* target, Effect* fx); //409
static int fx_summon_ally (Actor* Owner, Actor* target, Effect* fx); //410
static int fx_summon_enemy (Actor* Owner, Actor* target, Effect* fx); //411
static int fx_control (Actor* Owner, Actor* target, Effect* fx); //412
static int fx_visual_effect_iwd2 (Actor* Owner, Actor* target, Effect* fx); //413
static int fx_resilient_sphere (Actor* Owner, Actor* target, Effect* fx); //414
static int fx_barkskin (Actor* Owner, Actor* target, Effect* fx); //415
static int fx_bleeding_wounds (Actor* Owner, Actor* target, Effect* fx); //416
static int fx_area_effect  (Actor* Owner, Actor* target, Effect* fx); //417
static int fx_free_action_iwd2 (Actor* Owner, Actor* target, Effect* fx); //418
static int fx_unconsciousness (Actor* Owner, Actor* target, Effect* fx); //419
//420 Death2 (same as 0xd)
static int fx_entropy_shield (Actor* Owner, Actor* target, Effect* fx); //421
static int fx_storm_shell (Actor* Owner, Actor* target, Effect* fx); //422
static int fx_protection_from_elements (Actor* Owner, Actor* target, Effect* fx); //423
//424 HoldUndead (same as 0x6d)
//425 ControlUndead
static int fx_aegis (Actor* Owner, Actor* target, Effect* fx); //426
static int fx_executioner_eyes (Actor* Owner, Actor* target, Effect* fx); //427
//428 DeathMagic (same as 0xd)
//429
//430
static int fx_energy_drain (Actor* Owner, Actor* target, Effect* fx); //431
static int fx_tortoise_shell (Actor* Owner, Actor* target, Effect* fx); //432
static int fx_blink (Actor* Owner, Actor* target, Effect* fx); //433
static int fx_persistent_use_effect_list (Actor* Owner, Actor* target, Effect* fx); //434
static int fx_day_blindness (Actor* Owner, Actor* target, Effect* fx); //435
static int fx_damage_reduction (Actor* Owner, Actor* target, Effect* fx); //436
static int fx_disguise (Actor* Owner, Actor* target, Effect* fx); //437
static int fx_heroic_inspiration (Actor* Owner, Actor* target, Effect* fx); //438
//static int fx_prevent_ai_slowdown (Actor* Owner, Actor* target, Effect* fx); //439
static int fx_barbarian_rage (Actor* Owner, Actor* target, Effect* fx); //440
//441 MovementModifier
//442 unknown
static int fx_missile_damage_reduction (Actor* Owner, Actor* target, Effect* fx); //443
static int fx_tenser_transformation (Actor* Owner, Actor* target, Effect* fx); //444
//static int fx_445 (Actor* Owner, Actor* target, Effect* fx); //445 unused
static int fx_smite_evil (Actor* Owner, Actor* target, Effect* fx); //446
static int fx_restoration (Actor* Owner, Actor* target, Effect* fx); //447
static int fx_alicorn_lance (Actor* Owner, Actor* target, Effect* fx); //448
static int fx_call_lightning (Actor* Owner, Actor* target, Effect* fx); //449
static int fx_globe_invulnerability (Actor* Owner, Actor* target, Effect* fx); //450
static int fx_lower_resistance (Actor* Owner, Actor* target, Effect* fx); //451
static int fx_bane (Actor* Owner, Actor* target, Effect* fx); //452
static int fx_power_attack (Actor* Owner, Actor* target, Effect* fx); //453
static int fx_expertise (Actor* Owner, Actor* target, Effect* fx); //454
static int fx_arterial_strike (Actor* Owner, Actor* target, Effect* fx); //455
static int fx_hamstring (Actor* Owner, Actor* target, Effect* fx); //456
static int fx_rapid_shot (Actor* Owner, Actor* target, Effect* fx); //457

//No need to make these ordered, they will be ordered by EffectQueue
static EffectRef effectnames[] = {
	{ "ACVsDamageTypeModifierIWD2", fx_ac_vs_damage_type_modifier_iwd2, 0}, //0
	{ "DrawUponHolyMight", fx_draw_upon_holy_might, 0},//84 (iwd2)
	{ "IronSkins", fx_ironskins, 0}, //da (iwd2)
	{ "Color:FadeRGB", fx_fade_rgb, 0}, //e8
	{ "IWDVisualSpellHit", fx_iwd_visual_spell_hit, 0}, //e9
	{ "ColdDamage", fx_cold_damage, 0}, //ea
	{ "ChillTouch", fx_chill_touch, 0}, //ec (how)
	{ "ChillTouchPanic", fx_chill_touch_panic, 0}, //ec (iwd2)
	{ "CrushingDamage", fx_crushing_damage, 0}, //ed
	{ "SaveBonus", fx_save_bonus, 0}, //ee
	{ "SlowPoison", fx_slow_poison, 0}, //ef
	{ "IWDMonsterSummoning", fx_iwd_monster_summoning, 0}, //f0
	{ "VampiricTouch", fx_vampiric_touch, 0}, //f1
	{ "AnimateDead", fx_animate_dead, 0}, //f3
	{ "SummonMonster2", fx_summon_monster2, 0}, //f6
	{ "BurningBlood", fx_burning_blood, 0}, //f7
	{ "BurningBlood2", fx_burning_blood2, 0}, //f7
	{ "SummonShadowMonster", fx_summon_shadow_monster, 0}, //f8
	{ "Recitation", fx_recitation, 0}, //f9
	{ "RecitationBad", fx_recitation_bad, 0},//fa
	{ "LichTouch", fx_lich_touch, 0},//fb
	{ "BlindingOrb", fx_blinding_orb, 0}, //fc
	{ "RemoveEffects", fx_remove_effects, 0}, //fe
	{ "SalamanderAura", fx_salamander_aura, 0}, //ff
	{ "UmberHulkGaze", fx_umberhulk_gaze, 0}, //100
	{ "ZombieLordAura", fx_zombielord_aura, 0},//101
	{ "SummonCreature2", fx_summon_creature2, 0}, //103
	{ "SummonPomab", fx_summon_pomab, 0}, //106
	{ "ControlUndead", fx_control_undead, 0}, //107
	{ "StaticCharge", fx_static_charge, 0}, //108
	{ "CloakOfFear", fx_cloak_of_fear, 0}, //109
	{ "RemoveConfusion", fx_remove_confusion, 0},//10b
	{ "EyeOfTheMind", fx_eye_of_the_mind, 0}, //10c
	{ "EyeOfTheSword", fx_eye_of_the_sword, 0}, //10d
	{ "EyeOfTheMage", fx_eye_of_the_mage, 0}, //10e
	{ "EyeOfVenom", fx_eye_of_venom, 0}, //10f
	{ "EyeOfTheSpirit", fx_eye_of_the_spirit, 0}, //110
	{ "EyeOfFortitude", fx_eye_of_fortitude, 0}, //111
	{ "EyeOfStone", fx_eye_of_stone, 0}, //112
	{ "RemoveSevenEyes", fx_remove_seven_eyes, 0}, //113
	{ "RemoveEffect", fx_remove_effect, 0}, //114
	{ "SoulEater", fx_soul_eater, 0}, //115
	{ "ShroudOfFlame", fx_shroud_of_flame, 0},//116
	{ "ShroudOfFlame2", fx_shroud_of_flame2, 0},//116
	{ "AnimalRage", fx_animal_rage, 0}, //117 - berserk?
	{ "TurnUndead", fx_turn_undead, 0}, //118 how
	{ "TurnUndead2", fx_turn_undead2, 0}, //118 iwd2
	{ "VitriolicSphere", fx_vitriolic_sphere, 0}, //119
	{ "SuppressHP", fx_suppress_hp, 0}, //11a -- some stat???
	{ "FloatText", fx_floattext, 0}, //11b
	{ "MaceOfDisruption", fx_mace_of_disruption, 0}, //11c
	{ "State:Set", fx_set_state, 0}, //120
	{ "CutScene", fx_cutscene, 0}, //121
	{ "Protection:Spell2", fx_resist_spell, 0}, //ce
	{ "Protection:Spell3", fx_resist_spell_and_message, 0}, //122
	{ "RodOfSmithing", fx_rod_of_smithing, 0}, //123
	{ "HarpyWail", fx_harpy_wail, 0}, //126
	{ "JackalWereGaze", fx_jackalwere_gaze, 0}, //127
	{ "UseMagicDeviceModifier", fx_use_magic_device_modifier, 0}, //12a
	//iwd2 effects
	{ "Hopelessness", fx_hopelessness, 0}, //400
	{ "ProtectionFromEvil", fx_protection_from_evil, 0}, //401
	{ "AddEffectsList", fx_add_effects_list, 0}, //402
	{ "ArmorOfFaith", fx_armor_of_faith, 0}, //403
	{ "Nausea", fx_nausea, 0}, //404
	{ "Enfeeblement", fx_enfeeblement, 0}, //405
	{ "FireShield", fx_fireshield, 0}, //406
	{ "DeathWard", fx_death_ward, 0}, //407
	{ "HolyPower", fx_holy_power, 0}, //408
	{ "RighteousWrath", fx_righteous_wrath, 0}, //409
	{ "SummonAlly", fx_summon_ally, 0}, //410
	{ "SummonEnemy", fx_summon_enemy, 0}, //411
	{ "Control2", fx_control, 0}, //412
	{ "VisualEffectIWD2", fx_visual_effect_iwd2, 0}, //413
	{ "ResilientSphere", fx_resilient_sphere, 0}, //414
	{ "BarkSkin", fx_barkskin, 0}, //415
	{ "BleedingWounds", fx_bleeding_wounds, 0},//416
	{ "AreaEffect", fx_area_effect, 0}, //417
	{ "FreeAction2", fx_free_action_iwd2, 0}, //418
	{ "Unconsciousness", fx_unconsciousness, 0}, //419
	{ "EntropyShield", fx_entropy_shield, 0}, //421
	{ "StormShell", fx_storm_shell, 0}, //422
	{ "ProtectionFromElements", fx_protection_from_elements, 0}, //423
	{ "ControlUndead2", fx_control_undead, 0}, //425
	{ "Aegis", fx_aegis, 0}, //426
	{ "ExecutionerEyes", fx_executioner_eyes, 0}, //427
	{ "EnergyDrain", fx_energy_drain, 0}, //431
	{ "TortoiseShell", fx_tortoise_shell, 0}, //432
	{ "Blink", fx_blink, 0},//433
	{ "PersistentUseEffectList", fx_persistent_use_effect_list, 0}, //434
	{ "DayBlindness", fx_day_blindness, 0}, //435
	{ "DamageReduction", fx_damage_reduction, 0}, //436
	{ "Disguise", fx_disguise, 0}, //437
	{ "HeroicInspiration", fx_heroic_inspiration, 0},//438
	//{ "PreventAISlowDown", fx_prevent_ai_slowdown, 0}, //439 same as bg2
	{ "BarbarianRage", fx_barbarian_rage, 0}, //440
	{ "MissileDamageReduction", fx_missile_damage_reduction, 0}, //443
	{ "TensersTransformation", fx_tenser_transformation, 0}, //444
	{ "SmiteEvil", fx_smite_evil, 0}, //446
	{ "Restoration", fx_restoration, 0}, //447
	{ "AlicornLance", fx_alicorn_lance, 0}, //448
	{ "CallLightning", fx_call_lightning, 0}, //449
	{ "GlobeInvulnerability", fx_globe_invulnerability, 0}, //450
	{ "LowerResistance", fx_lower_resistance, 0}, //451
	{ "Bane", fx_bane, 0}, //452
	{ "PowerAttack", fx_power_attack, 0}, //453
	{ "Expertise", fx_expertise, 0}, //454
	{ "ArterialStrike", fx_arterial_strike, 0}, //455
	{ "HamString", fx_hamstring, 0}, //456
	{ "RapidShot", fx_rapid_shot, 0}, //457
	{ NULL, NULL, 0 },
};


IWDOpc::IWDOpc(void)
{
	core->RegisterOpcodes( sizeof( effectnames ) / sizeof( EffectRef ) - 1, effectnames );
	enhanced_effects=!!core->HasFeature(GF_ENHANCED_EFFECTS);
}

IWDOpc::~IWDOpc(void)
{
}

//iwd got a weird targeting system
//the opcode parameters are:
//param1 - optional value used only rarely
//param2 - a specific condition mostly based on target's stats
//this is partly superior, partly inferior to the bioware
//ids targeting.
//superior because it can handle other stats and conditions
//inferior because it is not moddable
//The hardcoded conditions are simulated via the IWDIDSEntry
//structure.
//stat is usually a stat, but for special conditions it is a
//function code (>=0x100).
//If value is -1, then GemRB will use Param1, otherwise it is
//compared to the target's stat using the relation function.
//The relation function is exactly the same as the extended
//diffmode for gemrb. (Thus scripts can use the very same relation
//functions).

typedef struct {
	ieDword value;
	ieWord stat;
	ieWord relation;
} IWDIDSEntry;

static int spellrescnt = -1;
static IWDIDSEntry *spellres = NULL;

static void ReadSpellProtTable(const ieResRef tablename)
{
	TableMgr * tab;
	if (spellres) {
		free(spellres);
	}
	spellres = NULL;
	spellrescnt = 0;
	int table=core->LoadTable( tablename );

	if (table<0) {
		return;
	}
	tab = core->GetTable( table );
	if (!tab) {
		core->DelTable(table);
		return;
	}
	spellrescnt=tab->GetRowCount();
	spellres = (IWDIDSEntry *) malloc(sizeof(IWDIDSEntry) * spellrescnt);
	if (!spellres) {
		core->DelTable(table);
		return;
	}
	for( int i=0;i<spellrescnt;i++) {
		spellres[i].stat = (ieWord) strtol(tab->QueryField(i,0),NULL,0 );
		spellres[i].value = (ieDword) strtol(tab->QueryField(i,1),NULL,0 );
		spellres[i].relation = (ieWord) strtol(tab->QueryField(i,2),NULL,0 );
	}
	core->DelTable(table);
	return;
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

//returns true if iwd ids targeting resists the spell
static int check_iwd_targeting(Actor* Owner, Actor* target, ieDword value, ieDword type)
{
	if (spellrescnt==-1) {
		ReadSpellProtTable("splprot");
	}
	if (type>=(ieDword) spellrescnt) {
		return 0; //not resisted
	}

	ieDword idx = spellres[type].stat;
	ieDword val = spellres[type].value;
	//if IDS value is 'anything' then the supplied value is in Parameter1
	if (val==0xffffffff) {
		val = value;
	}
	switch (idx) {
	case STI_EA:
		return DiffCore(EARelation(Owner, target), val, spellres[type].relation);
	case STI_DAYTIME:
		return (core->GetGame()->GameTime%7200/3600) != val;
	case STI_AREATYPE:
		return DiffCore((ieDword) target->GetCurrentArea()->AreaType, val, spellres[type].relation);
	case STI_MORAL_ALIGNMENT:
		return DiffCore(Owner->GetStat(IE_ALIGNMENT)&0x3,STAT_GET(IE_ALIGNMENT)&0x3, spellres[type].relation);
	case STI_TWO_ROWS:
		if (check_iwd_targeting(Owner, target, value, idx)) return 0;
		if (check_iwd_targeting(Owner, target, value, val)) return 0;
		return 1;
	case STI_NOT_TWO_ROWS:
		if (check_iwd_targeting(Owner, target, value, idx)) return 1;
		if (check_iwd_targeting(Owner, target, value, val)) return 1;
		return 0;
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
		return DiffCore((ieDword) target->GetAnims()->GetCircleSize(), val, spellres[type].relation);
	default:
		return DiffCore(STAT_GET(idx), val, spellres[type].relation);
	}
}

//iwd got a hardcoded 'fireshield' system
//this effect applies damage on ALL nearby actors, except the center
static EffectRef fx_damage_opcode_ref={"Damage",NULL,-1};

static void ApplyDamageNearby(Actor* Owner, Actor* target, Effect *fx, ieDword damagetype)
{
	Effect *newfx = EffectQueue::CreateEffect(fx_damage_opcode_ref, fx->Parameter1, damagetype,FX_DURATION_INSTANT_PERMANENT);
	newfx->Power = fx->Power;
	newfx->DiceThrown = fx->DiceThrown;
	newfx->DiceSides = fx->DiceSides;  
	memcpy(newfx->Resource, fx->Resource,sizeof(newfx->Resource) );
	//applyeffectcopy on everyone near us
	Map *area = target->GetCurrentArea();
	int i = area->GetActorCount(true);
	while(i--) {
		Actor *victim = area->GetActor(i,true);
		if (target!=victim) continue;
		if (PersonalDistance(target, victim)<20) {
			Effect *tmp = new Effect();
			memcpy(tmp, newfx, sizeof(Effect));
			//this function deletes tmp
			core->ApplyEffect(tmp, victim, Owner);
		}
	}
	//finally remove the master copy
	delete newfx;
}

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
int fx_ac_vs_damage_type_modifier_iwd2 (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_ac_vs_damage_type_modifier_iwd2 (%2d): AC Modif: %d ; Type: %d ; MinLevel: %d ; MaxLevel: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2, (int) fx->DiceSides, (int) fx->DiceThrown );
	//check level was pulled outside as a common functionality
	//CHECK_LEVEL();

	// it is a bitmask
	int type = fx->Parameter2;
	//the original engine did work with the combination of these bits
	//but since it crashed, we are not bound to the same rules
	switch(type)
	{
	case 0: //generic
		HandleBonus(target, IE_ARMORCLASS, fx->Parameter1, fx->TimingMode);
		break;
	case 1: //armor
	 	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
			if (BASE_GET( IE_ARMORCLASS) > fx->Parameter1) {
				BASE_SET( IE_ARMORCLASS, fx->Parameter1 );
			}
		} else {
			if (STAT_GET( IE_ARMORCLASS) > fx->Parameter1) {
				STAT_SET( IE_ARMORCLASS, fx->Parameter1 );
			}
		}
		return FX_INSERT;
	case 2: //deflection
		break;
	case 3: //shield
		break;
	case 4:
		HandleBonus(target, IE_ACCRUSHINGMOD, fx->Parameter1, fx->TimingMode);
		break;
	case 5:
		HandleBonus(target, IE_ACPIERCINGMOD, fx->Parameter1, fx->TimingMode);
		break;
	case 6:
		HandleBonus(target, IE_ACSLASHINGMOD, fx->Parameter1, fx->TimingMode);
		break;
	case 7:
		HandleBonus(target, IE_ACMISSILEMOD, fx->Parameter1, fx->TimingMode);
		break;
	}

	return FX_PERMANENT;
}

// 0x84 DrawUponHolyMight
// this effect differs from bg2 because it doesn't use the actor state field
// it uses the spell state field
int fx_draw_upon_holy_might (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_draw_upon_holy_might (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	if (target->SetSpellState( SS_HOLYMIGHT)) return FX_NOT_APPLIED;
	STAT_ADD( IE_STR, fx->Parameter1);
	STAT_ADD( IE_CON, fx->Parameter1);
	STAT_ADD( IE_DEX, fx->Parameter1);
	return FX_APPLIED;
}

//0xda IronSkins (iwd2)
int fx_ironskins (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	//todo: calculate buffered damage in param3 on first run
	//somehow deduct suffered damage from param3
	//abort effect when param3 goes below 0
	if (fx->Parameter2) {
		//ironskins
		if (target->SetSpellState( SS_IRONSKIN)) return FX_NOT_APPLIED;
		
		return FX_APPLIED;
	}
	//stoneskins (iwd2)
	if (target->SetSpellState( SS_STONESKIN)) return FX_NOT_APPLIED;
	target->SetGradient(14);
	return FX_APPLIED;
}

//0xe8 Colour:FadeRGB
int fx_fade_rgb (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_fade_rgb (%2d): \n", fx->Opcode  );

	int speed = (fx->Parameter2 >> 16) & 0xFF;
	target->SetColorMod(0xff, RGBModifier::ADD, speed,
				fx->Parameter1 >> 8, fx->Parameter1 >> 16,
				fx->Parameter1 >> 24, speed);

	return FX_NOT_APPLIED;
}

//0xe9 IWDVisualSpellHit
int fx_iwd_visual_spell_hit (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_iwd_visual_spell_hit (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	if (shcount<0) {
		shcount = core->ReadResRefTable("iwdshtab",iwd_spell_hits);
	}
	//delay apply until map is loaded
	Map *map = target->GetCurrentArea();
	if (!map) {
		return FX_APPLIED;
	}
	if (fx->Parameter2<(ieDword) shcount) {
		ScriptedAnimation *sca = core->GetScriptedAnimation(iwd_spell_hits[fx->Parameter2], false);
		if (!sca) {
			return FX_NOT_APPLIED;
		}
		if (fx->Parameter1) {
			sca->XPos+=target->Pos.x;
			sca->YPos+=target->Pos.y;
		} else {
			sca->XPos+=fx->PosX;
			sca->YPos+=fx->PosY;
		}
		if (fx->Parameter2<32) {
			int tmp = fx->Parameter2>>2;
			if (tmp) {
				sca->SetFullPalette(tmp);
			}
		}
		sca->SetBlend();
		sca->PlayOnce();
		map->AddVVCell(sca);
	}
	return FX_NOT_APPLIED;
}

//0xea ColdDamage (special)
int fx_cold_damage (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cold_damage (%2d): Damage %d\n", fx->Opcode, fx->Parameter1 );
	target->Damage(fx->Parameter1, DAMAGE_COLD, Owner);
	return FX_NOT_APPLIED;
}

//0xeb CastingGlow2 will be same as original casting glow

//0xec ChillTouch (how)
//this effect is to simulate the composite effects of chill touch
//it is the usual iwd/how style hack
int fx_chill_touch (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_chill_touch (%2d)\n", fx->Opcode);
	target->Damage(fx->Parameter1, DAMAGE_COLD, Owner);
	if (STAT_GET(IE_GENERAL)==GEN_UNDEAD) {
		target->Panic();
	}
	return FX_NOT_APPLIED;
}

//0xec ChillTouchPanic (iwd2)
//the undead check is made by IDS targeting as it should be
int fx_chill_touch_panic (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_chill_touch_panic (%2d)\n", fx->Opcode);
	STAT_SET(IE_MORALE, STAT_GET(IE_MORALEBREAK));
	if (enhanced_effects) {
		target->AddPortraitIcon(PI_PANIC);
	}
	return FX_APPLIED;
}

//0xed CrushingDamage (special)
int fx_crushing_damage (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_crushing_damage (%2d): Damage %d\n", fx->Opcode, fx->Parameter1 );
	target->Damage(fx->Parameter1, DAMAGE_CRUSHING, Owner);
	return FX_NOT_APPLIED;
}

//0xee SaveBonus
int fx_save_bonus (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_save_bonus (%2d): Bonus %d Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_SAVEVSDEATH );
	STAT_MOD( IE_SAVEVSWANDS );
	STAT_MOD( IE_SAVEVSPOLY );
	STAT_MOD( IE_SAVEVSBREATH );
	STAT_MOD( IE_SAVEVSSPELL );
	return FX_APPLIED;
}

//0xef SlowPoison
int fx_slow_poison (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_slow_poison (%2d): Damage %d\n", fx->Opcode, fx->Parameter1 );
	return FX_NOT_APPLIED;
}

#define IWD_MSC  13
ieResRef iwd_monster_2da[IWD_MSC]={"MSUMMO1","MSUMMO2","MSUMMO3","MSUMMO4",
 "MSUMMO5","MSUMMO6","MSUMMO7","ASUMMO1","ASUMMO2","ASUMMO3","CDOOM","GINSECT",
 "MSUMMOM"};

//0xf0 IWDMonsterSummoning
int fx_iwd_monster_summoning (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_iwd_monster_summoning (%2d): ResRef:%s Anim:%s Type: %d\n", fx->Opcode, fx->Resource, fx->Resource2, fx->Parameter2 );
	//check the summoning limit?

	ieResRef monster;
	ieResRef hit;
	ieResRef areahit;

	if (fx->Parameter2>=IWD_MSC) {
		fx->Parameter2=0;
	}
	core->GetResRefFrom2DA(iwd_monster_2da[fx->Parameter2], monster, hit, areahit);

	//the monster should appear near the effect position
	Point p(fx->PosX, fx->PosY);
	core->SummonCreature(monster, areahit, Owner, target, p, -1, fx->Parameter1);
	return FX_NOT_APPLIED;
}

//0xf1 VampiricTouch
int fx_vampiric_touch (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_vampiric_touch (%2d): ResRef:%s Type: %d\n", fx->Opcode, fx->Resource, fx->Parameter2 );
	if (Owner==target) {
		return FX_NOT_APPLIED;
	}

	Actor *receiver;
	Actor *donor;

	switch(fx->Parameter2) {
		case 0: receiver = target; donor = Owner; break;
		case 1: receiver = Owner; donor = target; break;
		default:
			return FX_NOT_APPLIED;
	}
	int damage = donor->Damage(fx->Parameter1, fx->Parameter2, Owner);
	receiver->SetStat( IE_HITPOINTS, STAT_GET( IE_HITPOINTS ) + ( damage ), 0 );
	return FX_NOT_APPLIED;
}

#define IWD_AD  2
ieResRef animate_dead_2da[IWD_AD]={"ADEAD","ADEADL"};

//0xf3 AnimateDead
int fx_animate_dead (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_animate_dead (%2d): ResRef:%s Type: %d\n", fx->Opcode, fx->Resource, fx->Parameter2 );
	//check the summoning limit?

	ieResRef monster;
	ieResRef hit;
	ieResRef areahit;

	if (fx->Parameter2>=IWD_AD) {
		fx->Parameter2=0;
	}
	core->GetResRefFrom2DA(animate_dead_2da[fx->Parameter2], monster, hit, areahit);

	//the monster should appear near the effect position
	Point p(fx->PosX, fx->PosY);
	core->SummonCreature(monster, areahit, Owner, target, p, -1, fx->Parameter1);
	return FX_NOT_APPLIED;
}
//f4,f5 (implemented in PST)

//0xf6 SummonMonster2
#define IWD_SM2 11
ieResRef summon_monster_2da[IWD_SM2]={"SLIZARD","STROLLS","SSHADOW","ISTALKE",
 "CFELEMW","CEELEMW","CWELEMW","CFELEMP","CEELEMP","CWELEMP","CEELEMM"};

int fx_summon_monster2 (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_summon_monster2 (%2d): ResRef:%s Type: %d\n", fx->Opcode, fx->Resource, fx->Parameter2 );
	//check the summoning limit?

	ieResRef monster;
	ieResRef hit;
	ieResRef areahit;

	if (fx->Parameter2>=IWD_SM2) {
		fx->Parameter2=0;
	}
	core->GetResRefFrom2DA(summon_monster_2da[fx->Parameter2], monster, hit, areahit);

	//the monster should appear near the effect position
	Point p(fx->PosX, fx->PosY);
	core->SummonCreature(monster, areahit, Owner, target, p, -1, fx->Parameter1);
	return FX_NOT_APPLIED;
}

//0xf7 BurningBlood (iwd)
int fx_burning_blood (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_burning_blood (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	//inflicts damage calculated by dice values+parameter1
	//creates damage opcode on everyone around. fx->Parameter2 - 0 fire, 1 - ice
	ieDword damage = DAMAGE_FIRE;

	if (fx->Parameter2==1) {
		damage = DAMAGE_COLD;
	}

	target->Damage(fx->Parameter1, DAMAGE_FIRE, Owner);
	STAT_SET(IE_CHECKFORBERSERK,1);
	return FX_NOT_APPLIED;
}
//0xf7 BurningBlood2 (how, iwd2)
int fx_burning_blood2 (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_burning_blood2 (%2d): Count: %d Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//timing
	if (core->GetGame()->GameTime%6) {
		return FX_APPLIED;
	}
	//inflicts damage calculated by dice values+parameter1
	if (!fx->Parameter1) {
		return FX_NOT_APPLIED;
	}
	fx->Parameter1--;

	ieDword damage = DAMAGE_FIRE;

	if (fx->Parameter2==1) {
		damage = DAMAGE_COLD;
	}

	//this effect doesn't use Parameter1 to modify damage, it is a counter instead
	target->Damage(DICE_ROLL(0), DAMAGE_FIRE, Owner);
	STAT_SET(IE_CHECKFORBERSERK,1);
	return FX_APPLIED;
}

//0xf8 SummonShadowMonster

#define IWD_SSM 3
ieResRef summon_shadow_monster_2da[IWD_SM2]={"SMONSTE","DSMONST","SHADES" };

int fx_summon_shadow_monster (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_summon_shadow_monster (%2d): ResRef:%s Type: %d\n", fx->Opcode, fx->Resource, fx->Parameter2 );
	//check the summoning limit?

	ieResRef monster;
	ieResRef hit;
	ieResRef areahit;

	if (fx->Parameter2>=IWD_SSM) {
		fx->Parameter2=0;
	}
	core->GetResRefFrom2DA(summon_shadow_monster_2da[fx->Parameter2], monster, hit, areahit);

	//the monster should appear near the effect position
	Point p(fx->PosX, fx->PosY);
	core->SummonCreature(monster, areahit, Owner, target, p, -1, fx->Parameter1);
	return FX_NOT_APPLIED;
}
//f9 Recitation
int fx_recitation (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_recitation (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	if (EXTSTATE_GET(EXTSTATE_RECITATION)) return FX_NOT_APPLIED;
	EXTSTATE_SET(EXTSTATE_RECITATION);
	return FX_APPLIED;
}
//fa RecitationBad
int fx_recitation_bad (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_recitation (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	if (EXTSTATE_GET(EXTSTATE_REC_BAD)) return FX_NOT_APPLIED;
	EXTSTATE_SET(EXTSTATE_REC_BAD);
	return FX_APPLIED;
}
//fb LichTouch (how)
static EffectRef fx_hold_creature_ref={"State:Hold",NULL,-1};

int fx_lich_touch (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_lich_touch (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	if (STAT_GET(IE_GENERAL)==GEN_UNDEAD) {
		return FX_NOT_APPLIED;
	}
	target->Damage(DICE_ROLL(0), DAMAGE_COLD, Owner);
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
//fc BlindingOrb (how)
static EffectRef fx_state_blind_ref={"State:Blind",NULL,-1};

int fx_blinding_orb (Actor* Owner, Actor* target, Effect* fx)
{
	ieDword damage = fx->Parameter1;
	if (STAT_GET(IE_GENERAL)==GEN_UNDEAD) {
		damage *= 2;
	}
	//check saving throw
	bool st = target->GetSavingThrow(4,0); //spell
	if (st) {
		target->Damage(damage/2, DAMAGE_FIRE, Owner);
		return FX_NOT_APPLIED;
	}
	target->Damage(damage, DAMAGE_FIRE, Owner);
	fx->Opcode = EffectQueue::ResolveEffect(fx_state_blind_ref);
	fx->Duration = core->Roll(1,6,0);
	fx->TimingMode = FX_DURATION_INSTANT_LIMITED;
	ieDword GameTime = core->GetGame()->GameTime;
	PrepareDuration(fx);
	return FX_APPLIED;
}
//0xfe RemoveEffects
int fx_remove_effects (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_remove_effects (%2d): ResRef:%s Type: %d\n", fx->Opcode, fx->Resource, fx->Parameter2 );

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
int fx_salamander_aura (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_salamander_aura (%2d): ResRef:%s Type: %d\n", fx->Opcode, fx->Resource, fx->Parameter2 );
	//inflicts damage calculated by dice values+parameter1
	//creates damage opcode on everyone around. fx->Parameter2 - 0 fire, 1 - ice
	ieDword damage = DAMAGE_FIRE;

	if (fx->Parameter2==1) {
		damage = DAMAGE_COLD;
	}

	ApplyDamageNearby(Owner, target, fx, damage);
	return FX_APPLIED;
}
//0x100 UmberHulkGaze (causes confusion or other stuff)

int fx_umberhulk_gaze (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_umberhulk_gaze (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_SET( STATE_CONFUSED );
	//timing
	if (core->GetGame()->GameTime%6) {
		return FX_APPLIED;
	}
	if (!fx->Parameter1) {
		return FX_NOT_APPLIED;
	}
	//random event???
	fx->Parameter1--;
	return FX_APPLIED;
}

//0x101 ZombieLordAura (causes fear?)
int fx_zombielord_aura (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_zombielord_aura (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_SET( STATE_CONFUSED );
	//timing
	if (core->GetGame()->GameTime%6) {
		return FX_APPLIED;
	}
	if (!fx->Parameter1) {
		return FX_NOT_APPLIED;
	}
	//random event???
	fx->Parameter1--;
	return FX_APPLIED;
}

//0x102 Protection:Spell (this is the same as in bg2?)

//0x103 SummonCreature2

static int eamods[]={EAM_DEFAULT,EAM_SOURCEALLY,EAM_SOURCEENEMY,EAM_NEUTRAL};

int fx_summon_creature2 (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_summon_creature2 (%2d): ResRef:%s Anim:%s Type: %d\n", fx->Opcode, fx->Resource, fx->Resource2, fx->Parameter2 );

	//summon creature (resource), play vvc (resource2)
	//creature's lastsummoner is Owner
	//creature's target is target
	//position of appearance is target's pos
	int eamod = EAM_DEFAULT;
	if (fx->Parameter2<4){
		eamod = eamods[fx->Parameter2];
	}
	core->SummonCreature(fx->Resource, fx->Resource2, Owner, target, target->Pos, eamod, 0);
	return FX_NOT_APPLIED;
}

//0x104 AvatarRemovalModifier (same as bg2)
//0x105 immunity to effect (same as bg2?)
//0x106 SummonPomab
int fx_summon_pomab (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_summon_pomab (%2d): ResRef:%s Anim:%s Type: %d\n", fx->Opcode, fx->Resource, fx->Resource2, fx->Parameter2 );
	if (!fx->Resource[0]) {
		strcpy(fx->Resource,"POMAB");
	}

	Point p(fx->PosX+core->Roll(1,20,0), fx->PosY+core->Roll(1,20,0) );
	core->SummonCreature(fx->Resource, fx->Resource2, Owner, target, p, -1,100);
	return FX_NOT_APPLIED;
}

//0x107 ControlUndead (like charm?)
//425 ControlUndead2
int fx_control_undead (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_control_undead (%2d): General: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	if (fx->Parameter1 && (STAT_GET(IE_GENERAL)!=fx->Parameter1)) {
		return FX_NOT_APPLIED;
	}
	bool enemyally = Owner->GetStat(IE_EA)>EA_GOODCUTOFF; //or evilcutoff?

	//do this only on first use
	switch (fx->Parameter2) {
	case 0: //charmed (target neutral after charm)
		core->DisplayConstantStringName(STR_CHARMED, 0xf0f0f0, target);
		break;
	case 1: //charmed (target hostile after charm)
		core->DisplayConstantStringName(STR_CHARMED, 0xf0f0f0, target);
		target->SetBase(IE_EA, EA_ENEMY);
		break;
	case 2: //controlled by cleric
		core->DisplayConstantStringName(STR_CONTROLLED, 0xf0f0f0, target);
		break;
	case 3: //controlled by cleric (hostile after charm)
		core->DisplayConstantStringName(STR_CONTROLLED, 0xf0f0f0, target);
		target->SetBase(IE_EA, EA_ENEMY);
		break;
	case 4: //turn undead
		core->DisplayConstantStringName(STR_CONTROLLED, 0xf0f0f0, target);
		target->SetBase(IE_EA, EA_ENEMY);
		target->SetStat(IE_MORALE, 0, 0);
		break;
	}

	STATE_SET( STATE_CHARMED );
	STAT_SET( IE_EA, enemyally?EA_ENEMY:EA_CHARMED );
	//don't stick if permanent
	return FX_PERMANENT;
}

//0x108 StaticCharge
int fx_static_charge(Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_static_charge (%2d): Count: %d \n", fx->Opcode, fx->Parameter1 );
	//timing
	if (core->GetGame()->GameTime%6) {
		return FX_APPLIED;
	}
	if (!fx->Parameter1) {
		return FX_NOT_APPLIED;
	}
	fx->Parameter1--;
	//
	target->Damage(fx->Parameter1, DAMAGE_ELECTRICITY, Owner);
	return FX_APPLIED;
}

//0x109
int fx_cloak_of_fear(Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_cloak_of_fear (%2d): Count: %d \n", fx->Opcode, fx->Parameter1 );
	//timing
	if (core->GetGame()->GameTime%6) {
		return FX_APPLIED;
	}

	if (!fx->Parameter1) {
		return FX_NOT_APPLIED;
	}
	fx->Parameter1--;
	//
	return FX_APPLIED;
}
//0x10a MovementRateModifier3 (Like bg2)
//0x10b RemoveConfusion
static EffectRef fx_confusion_ref={"State:Confused",NULL,-1};
static EffectRef fx_umberhulk_gaze_ref={"UmberHulkGaze",NULL,-1};
static EffectRef fx_display_portrait_icon_ref={"Icon:Display",NULL,-1};

int fx_remove_confusion (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_remove_confusion (%2d)\n", fx->Opcode );
	BASE_STATE_CURE(STATE_CONFUSED);
	target->fxqueue.RemoveAllEffects(fx_confusion_ref);
	target->fxqueue.RemoveAllEffects(fx_umberhulk_gaze_ref);
	target->fxqueue.RemoveAllEffectsWithParam(fx_display_portrait_icon_ref,PI_CONFUSION);
	return FX_APPLIED;
}
//0x10c EyeOfTheMind
int fx_eye_of_the_mind (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_eye_of_the_mind (%2d)\n", fx->Opcode );
	if (target->SetSpellState( SS_EYEMIND)) return FX_APPLIED;
	EXTSTATE_SET(EXTSTATE_EYE_MIND);
	//TODO: first run
	target->LearnSpell(SevenEyes[EYE_MIND], 0);
	return FX_APPLIED;
}
//0x10d EyeOfTheSword
int fx_eye_of_the_sword (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_eye_of_the_sword (%2d)\n", fx->Opcode );
	if (target->SetSpellState( SS_EYESWORD)) return FX_APPLIED;
	EXTSTATE_SET(EXTSTATE_EYE_SWORD);
	//TODO: first run
	target->LearnSpell(SevenEyes[EYE_SWORD], 0);
	return FX_APPLIED;
}

//0x10e EyeOfTheMage
int fx_eye_of_the_mage (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_eye_of_the_mage (%2d)\n", fx->Opcode );
	if (target->SetSpellState( SS_EYEMAGE)) return FX_APPLIED;
	EXTSTATE_SET(EXTSTATE_EYE_MAGE);
	//TODO: first run
	target->LearnSpell(SevenEyes[EYE_MAGE], 0);
	return FX_APPLIED;
}

//0x10f EyeOfVenom
int fx_eye_of_venom (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_eye_of_venom (%2d)\n", fx->Opcode );
	if (target->SetSpellState( SS_EYEVENOM)) return FX_APPLIED;
	EXTSTATE_SET(EXTSTATE_EYE_VENOM);
	//TODO: first run
	target->LearnSpell(SevenEyes[EYE_VENOM], 0);
	return FX_APPLIED;
}

//0x110 EyeOfTheSpirit
int fx_eye_of_the_spirit (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_eye_of_the_spirit (%2d)\n", fx->Opcode );
	if (target->SetSpellState( SS_EYESPIRIT)) return FX_APPLIED;
	EXTSTATE_SET(EXTSTATE_EYE_SPIRIT);
	//TODO: first run
	target->LearnSpell(SevenEyes[EYE_SPIRIT], 0);
	return FX_APPLIED;
}

//0x111 EyeOfFortitude
int fx_eye_of_fortitude (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_eye_of_fortitude (%2d)\n", fx->Opcode );
	if (target->SetSpellState( SS_EYEFORTITUDE)) return FX_APPLIED;
	EXTSTATE_SET(EXTSTATE_EYE_FORT);
	//TODO: first run
	target->LearnSpell(SevenEyes[EYE_FORT], 0);
	return FX_APPLIED;
}

//0x112 EyeOfStone
int fx_eye_of_stone (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_eye_of_stone (%2d)\n", fx->Opcode );
	if (target->SetSpellState( SS_EYESTONE)) return FX_APPLIED;
	EXTSTATE_SET(EXTSTATE_EYE_STONE);
	//TODO: first run
	target->LearnSpell(SevenEyes[EYE_STONE], 0);
	return FX_APPLIED;
}

//0x113 RemoveSevenEyes

int fx_remove_seven_eyes (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_remove_seven_eyes (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
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
int fx_remove_effect (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_remove_effect (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
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
int fx_soul_eater (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_soul_eater (%2d): Damage %d\n", fx->Opcode, fx->Parameter1 );
	target->Damage(fx->Parameter1, DAMAGE_SOULEATER, Owner);
	return FX_NOT_APPLIED;
}

//0x116 ShroudOfFlame (how)
//FIXME: maybe it is cheaper to port effsof1/2 to how than having an alternate effect
int fx_shroud_of_flame (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_shroud_of_flame (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	//timing
	if (core->GetGame()->GameTime%6) {
		return FX_APPLIED;
	}
	//inflicts damage calculated by dice values+parameter1
	//creates damage opcode on everyone around. fx->Parameter2 - 0 fire, 1 - ice
	ieDword damage = DAMAGE_FIRE;

	if (fx->Parameter2==1) {
		damage = DAMAGE_COLD;
	}

	target->Damage(fx->Parameter1, DAMAGE_FIRE, Owner);
	ApplyDamageNearby(Owner, target, fx, DAMAGE_FIRE);
	return FX_APPLIED;
}

//0x116 ShroudOfFlame (iwd2)
int fx_shroud_of_flame2 (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_shroud_of_flame2 (%2d)\n", fx->Opcode );
	if (target->SetSpellState( SS_FLAMESHROUD)) return FX_APPLIED;
	//timing
	if (core->GetGame()->GameTime%6) {
		return FX_APPLIED;
	}
	//apply effsof1 on target
	//apply effsof2 on nearby
	return FX_APPLIED;
}

//0x117 AnimalRage
int fx_animal_rage (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_animal_rage (%2d): Count %d\n", fx->Opcode, fx->Parameter1 );
	if (target->SetSpellState( SS_ANIMALRAGE)) return FX_APPLIED;
	//timing
	if (core->GetGame()->GameTime%6) {
		return FX_APPLIED;
	}
	if (!fx->Parameter1) {
		return FX_NOT_APPLIED;
	}
	fx->Parameter1--;
	STAT_SET( IE_CHECKFORBERSERK, 1 );
	return FX_APPLIED;
}

//0x118 TurnUndead how
int fx_turn_undead (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_turn_undead (%2d): Level %d\n", fx->Opcode, fx->Parameter1 );
	if (fx->Parameter1) {
		target->Turn(Owner, fx->Parameter1);
	} else {
		target->Turn(Owner, Owner->GetStat(IE_TURNUNDEADLEVEL));
	}
	return FX_APPLIED;
}

//0x118 TurnUndead2 iwd2
int fx_turn_undead2 (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_turn_undead2 (%2d): Level: %d  Type %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	switch (fx->Parameter2)
	{
	case 0: //command
		target->Panic();
		break;
	case 1://rebuke
		target->Panic();
		break;
	case 2://destroy
		target->Die(Owner);
		break;
	case 3://panic
		target->Panic();
		break;
	default://depends on caster
		if (fx->Parameter1) {
			target->Turn(Owner, fx->Parameter1);
		} else {
			target->Turn(Owner, Owner->GetStat(IE_TURNUNDEADLEVEL));
		}
		break;
	}
	return FX_APPLIED;
}

//0x119 VitriolicSphere
int fx_vitriolic_sphere (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_vitriolic_sphere (%2d): Damage %d\n", fx->Opcode, fx->Parameter1 );
	//timing
	if (core->GetGame()->GameTime%6) {
		return FX_APPLIED;
	}
	target->Damage(fx->Parameter1, DAMAGE_ACID, Owner);
	fx->DiceThrown-=2;
	if ((signed) fx->DiceThrown<1) {
		return FX_NOT_APPLIED;
	}
	//also damage people nearby?
//	ApplyDamageNearby(Owner, target, fx, DAMAGE_ACID);
	return FX_APPLIED;
}

//0x11a SuppressHP
int fx_suppress_hp (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_suppress_hp (%2d)\n", fx->Opcode);
	if (target->SetSpellState( SS_NOHPINFO)) return FX_APPLIED;
	EXTSTATE_SET(0x00001000);
	return FX_APPLIED;
}

//0x11b FloatText
int fx_floattext (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_floattext (%2d): StrRef:%d Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	switch (fx->Parameter2)
	{
	default:
		DisplayStringCore(target, fx->Parameter1, DS_HEAD);
		break;
	case 1:
		//in the original game this signified that a specific weapon is equipped
		EXTSTATE_SET(0x00008000);
		return FX_APPLIED;
	case 2: //gemrb extension, displays verbalconstant
		DisplayStringCore(target, fx->Parameter1, DS_CONST|DS_HEAD);
		break;
	}
	return FX_NOT_APPLIED;
}

//0x11c MaceOfDisruption
//death with chance based on race and level
static EffectRef fx_death_ref={"Death",NULL,-1};

int fx_mace_of_disruption (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_mace_of_disruption (%2d): ResRef:%s Anim:%s Type: %d\n", fx->Opcode, fx->Resource, fx->Resource2, fx->Parameter2 );
	ieDword race = STAT_GET(IE_RACE);
	//golem / outer planar gets hit
	int chance = 0;
	switch (race) {
		case 156: // outsider
			chance = 5;
			break;
		case 108: case 115: case 167:  //ghoul, skeleton, undead
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
	if (chance>core->Roll(1,100,0)) {
		return FX_NOT_APPLIED;
	}


	Effect *newfx = EffectQueue::CreateEffect(fx_death_ref, 0,
			8, FX_DURATION_INSTANT_PERMANENT);
	core->ApplyEffect(newfx, Owner, target);
	return FX_NOT_APPLIED;
}
//0x11d Sleep2 ??? power word sleep?
//0x11e Reveal:Tracks (same as bg2)
//0x11f Protection:Backstab (same as bg2)

//0x120 State:Set
int fx_set_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_state (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	//in IWD2 we have 176 states (original had 256)
	target->SetSpellState(fx->Parameter2);
	//in HoW this sets only the 10 last bits of extstate (until it runs out of bits)
	if (fx->Parameter2<11) {
		EXTSTATE_SET(0x40000<<fx->Parameter2);
	}
	return FX_APPLIED;
}

//0x121 Cutscene (this is a very ugly hack in iwd)
int fx_cutscene (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_cutscene (%2d)\n", fx->Opcode );
	Game *game = core->GetGame();
	game->locals->SetAt("GEM_ACTIVE", 1);
	return FX_NOT_APPLIED;
}

//0xce (same place as in bg2, but different)
int fx_resist_spell (Actor* Owner, Actor* target, Effect *fx)
{
	//check iwd ids targeting
	if (check_iwd_targeting(Owner, target, fx->Parameter1, fx->Parameter2) ) {
		return FX_NOT_APPLIED;
	}

	if (strnicmp(fx->Resource,fx->Source,sizeof(fx->Resource)) ) {
		return FX_APPLIED;
	}
	//this has effect only on first apply, it will stop applying the spell
	return FX_ABORT;
}

static EffectRef fx_resist_spell_ref={"ResistSpell2",NULL,-1};

//0x122 Protection:Spell3 ??? IWD ids targeting
// this is a variant of resist spell, used in iwd2
int fx_resist_spell_and_message (Actor* Owner, Actor* target, Effect *fx)
{
	//check iwd ids targeting
	if (check_iwd_targeting(Owner, target, fx->Parameter1, fx->Parameter2) ) {
		return FX_NOT_APPLIED;
	}

	fx->Opcode=EffectQueue::ResolveEffect(fx_resist_spell_ref);
	//display message too

	if (strnicmp(fx->Resource,fx->Source,sizeof(fx->Resource)) ) {
		return FX_APPLIED;
	}
	//this has effect only on first apply, it will stop applying the spell
	return FX_ABORT;
}

//0x123 RodOfSmithing
//if golem: 5% death or 1d8+3 damage
//if outsider: 5% 8d3 damage or nothing
//otherwise: nothing
int fx_rod_of_smithing (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_rod_of_smithing (%2d): ResRef:%s Anim:%s Type: %d\n", fx->Opcode, fx->Resource, fx->Resource2, fx->Parameter2 );
	ieDword race = STAT_GET(IE_RACE);
	//golem / outer planar gets hit
	int damage;

	switch (race) {
		case 144: // golem
			if (core->Roll(1,100,0)>5) {
				damage = core->Roll(1,8,3);
			} else {
				damage = -1;
			}
			break;
		case 156: //outsider
			if (core->Roll(1,100,0)>5) {
				return FX_NOT_APPLIED;
			}
			damage = core->Roll(8,3,0);
			break;
		default:;
			return FX_NOT_APPLIED;
	}

	Effect *newfx;
	if (damage<0) {
		//create death effect (chunked death)
		newfx = EffectQueue::CreateEffect(fx_death_ref, 0,
			8, FX_DURATION_INSTANT_PERMANENT);
	} else {
		//create damage effect (blunt)
		newfx = EffectQueue::CreateEffect(fx_damage_opcode_ref, (ieDword) damage,
			0, FX_DURATION_INSTANT_PERMANENT);
	}
	core->ApplyEffect(newfx, Owner, target);
	return FX_NOT_APPLIED;
}

//0x124 MagicalRest (same as bg2)
//0x125 BeholderDispelMagic (???)
//0x126 HarpyWail (???)
int fx_harpy_wail (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_harpy_wail (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	target->Panic();
	return FX_NOT_APPLIED;
}

//0x127 JackalWereGaze (Charm ?)
int fx_jackalwere_gaze (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_jackalwere_gaze (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	target->Panic();
	return FX_APPLIED;
}
//0x128 ModifyGlobalVariable (same as bg2)
//0x129 HideInShadows (same as bg2)

//0x12a UseMagicDevice
int fx_use_magic_device_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_use_magic_device_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_MAGICDEVICE );
	return FX_APPLIED;
}

//IWD2 effects

//400 Hopelessness
int fx_hopelessness (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_hopelessness (%2d)\n", fx->Opcode);
	if (target->SetSpellState( SS_HOPELESSNESS)) return FX_NOT_APPLIED;
	target->AddPortraitIcon(PI_HOPELESSNESS);
	STATE_SET(STATE_HELPLESS);
	return FX_APPLIED;
}

//401 ProtectionFromEvil
int fx_protection_from_evil (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_protection_from_evil (%2d)\n", fx->Opcode);
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
int fx_add_effects_list (Actor* Owner, Actor* target, Effect* fx)
{
	//after iwd2 style ids targeting, apply the spell named in the resource field
	if (check_iwd_targeting(Owner, target, fx->Parameter1, fx->Parameter2) ) {
		return FX_NOT_APPLIED;
	}
	core->ApplySpell(fx->Resource, Owner, target, fx->Power);
	return FX_NOT_APPLIED;
}

//403 ArmorOfFaith
static int fx_armor_of_faith (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_armor_of_faith (%2d)\n", fx->Opcode);
	if (target->SetSpellState( SS_ARMOROFFAITH)) return FX_APPLIED;
	//TODO: damage reduction (all types)
	target->AddPortraitIcon(PI_FAITHARMOR);
	return FX_APPLIED;
}

//404 Nausea
static EffectRef fx_unconscious_state_ref={"State:Helpless",NULL,-1};

int fx_nausea (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_nausea (%2d)\n", fx->Opcode);
	//FIXME: i'm not sure if this part is there
	//create the sleep effect only once?
	if (!fx->Parameter3) {
		Effect *newfx = EffectQueue::CreateEffect(fx_unconscious_state_ref,
			fx->Parameter1, 1, fx->Duration);
		core->ApplyEffect(newfx, Owner, target);
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
int fx_enfeeblement (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_enfeeblement (%2d)\n", fx->Opcode);
	if (target->SetSpellState( SS_ENFEEBLED)) return FX_APPLIED;
	target->AddPortraitIcon(PI_ENFEEBLEMENT);
	STAT_ADD(IE_STR, -15);
	return FX_APPLIED;
}

//406 FireShield
int fx_fireshield (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_fireshield (%2d) Type: %d\n", fx->Opcode, fx->Parameter2);
	if (fx->Parameter2) {
		if (target->SetSpellState( SS_ICESHIELD)) return FX_APPLIED;
		target->AddPortraitIcon(PI_ICESHIELD);
	} else {
		if (target->SetSpellState( SS_FIRESHIELD)) return FX_APPLIED;
		target->AddPortraitIcon(PI_FIRESHIELD);
	}
	//target->SetSpellOnHit(fx->Resource);
	return FX_APPLIED;
}

//407 DeathWard
int fx_death_ward (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_death_ward (%2d)\n", fx->Opcode);
	if (target->SetSpellState( SS_DEATHWARD)) return FX_APPLIED;
	target->AddPortraitIcon(PI_DEATHWARD); // is it ok?

	return FX_APPLIED;
}

//408 HolyPower
int fx_holy_power (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_holy_power (%2d)\n", fx->Opcode);
	if (target->SetSpellState( SS_HOLYPOWER)) return FX_APPLIED;

	if (enhanced_effects) {
		target->AddPortraitIcon(PI_HOLYPOWER);
		target->SetColorMod(0xff, RGBModifier::ADD, 20, 0x80, 0x80, 0x80);
	}
	return FX_APPLIED;
}

//409 RighteousWrath
int fx_righteous_wrath (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_righteous_wrath (%2d) Type: %d\n", fx->Opcode, fx->Parameter2);
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
int fx_summon_ally (Actor* Owner, Actor* target, Effect* fx)
{
	core->SummonCreature(fx->Resource, fx->Resource2, Owner, target, target->Pos, EAM_ALLY, 0);
	return FX_NOT_APPLIED;
}

//411 SummonEnemyIWD2
int fx_summon_enemy (Actor* Owner, Actor* target, Effect* fx)
{
	core->SummonCreature(fx->Resource, fx->Resource2, Owner, target, target->Pos, EAM_ENEMY, 0);
	return FX_NOT_APPLIED;
}

//412 Control2

static EffectRef fx_protection_from_evil_ref={"ProtectionFromEvil",NULL,-1};

int fx_control (Actor* Owner, Actor* target, Effect* fx)
{
	//prot from evil deflects it
	if (target->fxqueue.HasEffect(fx_protection_from_evil_ref)) return FX_NOT_APPLIED;

	if (0) printf( "fx_control (%2d)\n", fx->Opcode);
	bool enemyally = Owner->GetStat(IE_EA)>EA_GOODCUTOFF;
	switch(fx->Parameter2)
	{
	case 0:
		core->DisplayConstantStringName(STR_CHARMED, 0xf0f0f0, target);
		break;
	case 1:
		core->DisplayConstantStringName(STR_DIRECHARMED, 0xf0f0f0, target)
;
		break;
	default:
		core->DisplayConstantStringName(STR_CONTROLLED, 0xf0f0f0, target);

		break;
	}
	STATE_SET( STATE_CHARMED );
	STAT_SET( IE_EA, enemyally?EA_ENEMY:EA_CHARMED );
	return FX_APPLIED;
}

//413 VisualEffectIWD2
//there are 32 bits, so they will fit on IE_SANCTUARY stat
//i put them there because the first bit is sanctuary
int fx_visual_effect_iwd2 (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	//
	if (0) printf( "fx_visual_effect_iwd2 (%2d) Type: %d\n", fx->Opcode, fx->Parameter2);
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
		// some more
		}
		target->SetOverlay(type);
		//STAT_BIT_OR(IE_SANCTUARY, 1<<type);
		return FX_APPLIED;
	}  
	return FX_NOT_APPLIED;
}

//414 ResilientSphere
int fx_resilient_sphere (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_resilient_sphere (%2d)\n", fx->Opcode);
	target->SetSpellState(SS_HELD|SS_RESILIENT);
	STATE_SET(STATE_HELPLESS);
	if (enhanced_effects) {
		target->AddPortraitIcon(PI_RESILIENT);
		target->SetOverlay(OV_RESILIENT);
	}
	return FX_APPLIED;
}

//415 Barkskin
int fx_barkskin (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_barkskin (%2d)\n", fx->Opcode);
	if (target->SetSpellState( SS_BARKSKIN)) return FX_APPLIED;

	int bonus;
	int level = STAT_GET(IE_LEVELCLERIC);
	if (level>6) {
		if (level>12) {
			bonus=5;
		} else {
			bonus=4;
		}
	} else {
		bonus=3;
	}
	STAT_ADD(IE_ARMORCLASS,bonus);
	if (enhanced_effects) {
		target->AddPortraitIcon(PI_BARKSKIN);
		target->SetGradient(2);
	}
	return FX_APPLIED;
}

//regen/poison/disease types (works for bleeding wounds too)
#define RPD_PERCENT 1
#define RPD_SECONDS 2
#define RPD_POINTS  3

//416 BleedingWounds
int fx_bleeding_wounds (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_bleeding_wounds (%2d): Damage: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	//also this effect is executed every update
	ieDword damage;

	switch(fx->Parameter2) {
	case RPD_PERCENT:
		damage = STAT_GET(IE_MAXHITPOINTS) * fx->Parameter1 / 100;
		break;
	case RPD_SECONDS:
		damage = 1;
		if (fx->Parameter1 && (core->GetGame()->GameTime%fx->Parameter1)) {
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
	target->Damage(damage, DAMAGE_POISON, Owner);
	target->AddPortraitIcon(PI_BLEEDING);
	return FX_APPLIED;
}

//417 AreaEffect
int fx_area_effect (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_area_effect (%2d) Type: %d\n", fx->Opcode, fx->Parameter2);

	Point pos(fx->PosX, fx->PosY);

	Spell *spell = core->GetSpell(fx->Resource);
	if (!spell) {
		return FX_NOT_APPLIED;
	}

	EffectQueue *fxqueue = spell->GetEffectBlock(0);
	fxqueue->SetOwner(Owner);
	fxqueue->AffectAllInRange(target->GetCurrentArea(), pos, 0, 0,fx->Parameter1, target);

	delete fxqueue;

	if (fx->Parameter2&1) {
		 return FX_APPLIED;
	}
	return FX_NOT_APPLIED;
}

//418 FreeAction2
int fx_free_action_iwd2 (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_free_action_iwd2 (%2d)\n", fx->Opcode);
	if (target->SetSpellState( SS_FREEACTION)) return FX_APPLIED;

	// immunity to the following effects: 
	// 0x9a Overlay:Entangle, 
	// 0x9d Overlay:Web
	// 0x9e Overlay:Grease
	// 0x6d State:Hold3
	// 0x28 State:Slowed
	// 0xb0 MovementModifier
	if (enhanced_effects) {
		target->AddPortraitIcon(PI_FREEACTION);
		target->SetColorMod(0xff, RGBModifier::ADD, 30, 0x80, 0x60, 0x60);
	}
	return FX_APPLIED;
}

//419 Unconsciousness
int fx_unconsciousness (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_unconsciousness (%2d)\n", fx->Opcode);
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
int fx_entropy_shield (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_entropy_shield (%2d)\n", fx->Opcode);
	if (target->SetSpellState( SS_ENTROPY)) return FX_APPLIED;
	//immunity to certain projectiles?
	//
	//
	if (enhanced_effects) {
		target->AddPortraitIcon(PI_ENTROPY);
		//entropy shield overlay
		target->SetOverlay(OV_ENTROPY);
		target->SetColorMod(0xff, RGBModifier::ADD, 30, 0x40, 0xc0, 0x40);
	}
	return FX_APPLIED;
}

//422 StormShell
int fx_storm_shell (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_storm_shell (%2d)\n", fx->Opcode);
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
int fx_protection_from_elements (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_protection_from_elements (%2d)\n", fx->Opcode);
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
int fx_aegis (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_aegis (%2d)\n", fx->Opcode);
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

	if (enhanced_effects) {
		target->AddPortraitIcon(PI_AEGIS);
		target->SetColorMod(0xff, RGBModifier::ADD, 30, 0x80, 0x60, 0x60);
		target->SetGradient(14);
	}
	return FX_APPLIED;
}

//427 ExecutionerEyes
int fx_executioner_eyes (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_executioner_eyes (%2d)\n", fx->Opcode);
	if (target->SetSpellState( SS_EXECUTIONER)) return FX_APPLIED;

	STAT_ADD(IE_CRITICALHITBONUS, 4);
	STAT_ADD(IE_TOHIT, 4);

	if (enhanced_effects) {
		target->AddPortraitIcon(PI_EXECUTIONER);
		target->SetGradient(8);
	}
	return FX_APPLIED;
}

//428 Death3 (also a death effect)

//429 WhenStruckUseEffectList

//430 ProjectileUseEffectList

//431 EnergyDrain

static EffectRef fx_energy_drain_ref={"EnergyDrain",NULL,-1};

int fx_energy_drain (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_energy_drain (%2d) Type: %d\n", fx->Opcode, fx->Parameter2);
	Effect *oldfx = target->fxqueue.HasEffect(fx_energy_drain_ref);
	if (oldfx) {
		oldfx->Parameter2+=fx->Parameter2;
		return FX_NOT_APPLIED;
	}
	//if there is another energy drain effect, add it up
	STAT_SET(IE_LEVELDRAIN, fx->Parameter2);
	return FX_APPLIED;
}

//432 TortoiseShell
int fx_tortoise_shell (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_tortoise_shell (%2d) Type: %d\n", fx->Opcode, fx->Parameter2);
	if (target->SetSpellState( SS_TORTOISE)) return FX_NOT_APPLIED;
	if (enhanced_effects) {
		target->AddPortraitIcon(PI_TORTOISE);
		target->SetOverlay(OV_TORTOISE);
	}
	return FX_APPLIED;
}

//433 Blink
int fx_blink (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_blink (%2d) Type: %d\n", fx->Opcode, fx->Parameter2);
	if (target->SetSpellState( SS_BLINK)) return FX_APPLIED;
	if(fx->Parameter2) {
		target->AddPortraitIcon(PI_EMPTYBODY);
		return FX_APPLIED;
	}
	target->AddPortraitIcon(PI_BLINK);

	return FX_APPLIED;
}

//434 PersistentUseEffectList
int fx_persistent_use_effect_list (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_persistent_use_effect_list (%2d) Interval: %d Resource: %.8s\n", fx->Opcode, fx->Parameter1, fx->Resource);
	if (fx->Parameter3) {
		fx->Parameter3--;
	} else {
		core->ApplySpell(fx->Resource, Owner, target, fx->Power);
		fx->Parameter3=fx->Parameter1;
	}
	return FX_APPLIED;
}

//435 DayBlindness
int fx_day_blindness (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_day_blindness (%2d) Amount: %d\n", fx->Opcode, fx->Parameter2);
	if (target->SetSpellState(SS_DAYBLINDNESS)) return FX_NOT_APPLIED;
	// medium hack (better than original)
	// the original used explicit race/subrace values
	ieDword penalty;

	if (check_iwd_targeting(Owner, target, 0, 82)) penalty = 2; //dark elf
	else if (check_iwd_targeting(Owner, target, 0, 84)) penalty = 2; //duergar
	else penalty = 0;

 	STAT_ADD(IE_SAVEFORTITUDE, penalty);
	STAT_ADD(IE_SAVEREFLEX, penalty);
	STAT_ADD(IE_SAVEWILL, penalty);
	//for compatibility reasons
	STAT_ADD(IE_SAVEVSBREATH, penalty);
	STAT_ADD(IE_SAVEVSSPELL, penalty);

	//TODO: implement the rest
	return FX_APPLIED;
}

//436 DamageReduction
int fx_damage_reduction (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_damage_reduction (%2d) Amount: %d\n", fx->Opcode, fx->Parameter2);
 	STAT_SET(IE_RESISTSLASHING, fx->Parameter2*5);
	STAT_SET(IE_RESISTCRUSHING, fx->Parameter2*5);
	STAT_SET(IE_RESISTPIERCING, fx->Parameter2*5);
	return FX_APPLIED;
}

//437 Disguise
//modifies character animations to look like clerics of same gender/race
int fx_disguise (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_disguise (%2d) Amount: %d\n", fx->Opcode, fx->Parameter2);
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
int fx_heroic_inspiration (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_heroic_inspiration (%2d)\n", fx->Opcode);

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
int fx_barbarian_rage (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_barbarian_rage (%2d) Amount:%d\n", fx->Opcode, fx->Parameter1);
	//TODO: implement this
	return FX_APPLIED;
}

//441 MovementRateModifier4 (same as others)

//442 Unknown (needs research)

//443 MissileDamageReduction
int fx_missile_damage_reduction (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_missile_damage_reduction (%2d) Amount:%d\n", fx->Opcode, fx->Parameter1);
	STAT_SET(IE_RESISTMISSILE, fx->Parameter1);
	//TODO: didn't set the pluses
	return FX_APPLIED;
}

//444 TensersTransformation
int fx_tenser_transformation (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_tenser_transformation (%2d)\n", fx->Opcode);
	if (target->SetSpellState( SS_TENSER)) return FX_APPLIED;
	STAT_SUB(IE_ARMORCLASS, 2);
	
	if (enhanced_effects) {
		target->SetGradient(0x3e);
	}
	return FX_APPLIED;
}

//445 Unknown (empty function in iwd2)

//446 SmiteEvil
int fx_smite_evil (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_smite_evil (%2d)\n", fx->Opcode);
	target->SetSpellState(SS_SMITEEVIL);
	return FX_NOT_APPLIED;
}

//447 Restoration

static EffectRef fx_disease_ref={"Disease",NULL,-1};
static EffectRef fx_str_ref={"StrengthModifier",NULL,-1};
static EffectRef fx_int_ref={"IntelligenceModifier",NULL,-1};
static EffectRef fx_wis_ref={"WisdomModifier",NULL,-1};
static EffectRef fx_con_ref={"ConstitutionModifier",NULL,-1};
static EffectRef fx_dex_ref={"DexterityModifier",NULL,-1};
static EffectRef fx_cha_ref={"CharismaModifier",NULL,-1};

int fx_restoration (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_restoration (%2d)\n", fx->Opcode);
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
int fx_alicorn_lance (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_alicorn_lance (%2d)\n", fx->Opcode);
	if (target->SetSpellState( SS_ALICORNLANCE)) return FX_APPLIED;
	////target->AddPortraitIcon(PI_ALICORN); //no portrait icon
	STAT_SUB(IE_ARMORCLASS, 2);
	//color glow
	if (enhanced_effects) {
		target->SetColorMod(0xff, RGBModifier::ADD, 1, 0xb9, 0xb9, 0xb9);
	}
	return FX_APPLIED;
}

//449 CallLightning
int fx_call_lightning (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_call_lightning (%2d)\n", fx->Opcode);
	//TODO: implement
	return FX_NOT_APPLIED;
}

//450 GlobeInvulnerability
int fx_globe_invulnerability (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_globe_invulnerability (%2d)\n", fx->Opcode);
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
int fx_lower_resistance (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_lower_resistance (%2d)\n", fx->Opcode);
	if (target->SetSpellState( SS_LOWERRESIST)) return FX_APPLIED;

	STAT_SUB(IE_RESISTMAGIC, 15);
	return FX_APPLIED;
}
//452 Bane
static EffectRef fx_bless_ref={"Bless",NULL,-1};

int fx_bane (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_bane (%2d)\n", fx->Opcode);
	
	if (target->SetSpellState( SS_BANE)) return FX_APPLIED;
	//do this once
	target->fxqueue.RemoveAllEffects(fx_bless_ref);
	if (enhanced_effects) {
		target->SetColorMod(0xff, RGBModifier::ADD, 20, 0, 0, 0x80);
	}
	return FX_APPLIED;
}

//453 PowerAttack
int fx_power_attack (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_power_attack (%2d)\n", fx->Opcode);
	if (target->SetSpellState( SS_POWERATTACK)) return FX_APPLIED;
	//TODO: 
	return FX_APPLIED;
}

//454 Expertise
int fx_expertise (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_expertise (%2d)\n", fx->Opcode);
	if (target->SetSpellState( SS_EXPERTISE)) return FX_APPLIED;
	//TODO: 
	return FX_APPLIED;
}

//455 ArterialStrike
int fx_arterial_strike (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_arterial_strike (%2d)\n", fx->Opcode);
	if (target->SetSpellState( SS_ARTERIAL)) return FX_APPLIED;
	//TODO: 
	return FX_APPLIED;
}

//456 HamString
int fx_hamstring (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_hamstring (%2d)\n", fx->Opcode);
	if (target->SetSpellState( SS_HAMSTRING)) return FX_APPLIED;
	//TODO: 
	return FX_APPLIED;
}

//457 RapidShot
int fx_rapid_shot (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_rapid_shot (%2d)\n", fx->Opcode);
	if (target->SetSpellState( SS_RAPIDSHOT)) return FX_APPLIED;
	//TODO: 
	return FX_APPLIED;
}
