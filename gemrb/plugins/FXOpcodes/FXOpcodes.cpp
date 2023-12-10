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
#include "strrefs.h"
#include "voodooconst.h"

#include "Audio.h"
#include "DisplayMessage.h"
#include "EffectQueue.h"
#include "Game.h"
#include "GameData.h"
#include "GlobalTimer.h"
#include "Interface.h"
#include "PolymorphCache.h" // fx_polymorph
#include "Projectile.h" //needs for clearair
#include "ProjectileServer.h"
#include "RNG.h"
#include "ScriptedAnimation.h"
#include "ScriptEngine.h"
#include "Spell.h" //needed for fx_cast_spell feedback
#include "TileMap.h" //needs for knock!
#include "VEFObject.h"
#include "damages.h"
#include "GameScript/GSUtils.h" //needs for MoveBetweenAreasCore
#include "GameScript/Matching.h" //needs for GetAllObjects
#include "GUI/GameControl.h"
#include "Scriptable/Actor.h"
#include "Scriptable/Container.h"
#include "Scriptable/Door.h"
#include "Scriptable/InfoPoint.h"
#include "Scriptable/PCStatStruct.h" //fx_polymorph (action definitions)

using namespace GemRB;

#define PI_RIGID     2
#define PI_CONFUSED  3
#define PI_BERSERK   4
#define PI_DRUNK     5
#define PI_POISONED  6
#define PI_DISEASED  7
#define PI_BLIND     8
#define PI_HELD     13
#define PI_SLEEP    14
#define PI_BLESS    17
#define PI_PANIC        36
#define PI_HASTED   38
#define PI_FATIGUE  39
#define PI_SLOWED   41
#define PI_HOPELESS 44
#define PI_LEVELDRAIN 53
#define PI_FEEBLEMIND 54
#define PI_STUN     55 //bg1+bg2
#define PI_STUN_IWD 44 //iwd1+iwd2
#define PI_AID      57
#define PI_HOLY     59
#define PI_BOUNCE   65
#define PI_BOUNCE2  67

#define PI_CONTINGENCY 75
#define PI_BLOODRAGE 76 //iwd2
#define PI_PROJIMAGE 77
#define PI_MAZE     78
#define PI_PRISON   79
#define PI_STONESKIN 80
#define PI_DEAFNESS  83 //iwd2
#define PI_SEQUENCER 92
#define PI_BLUR      109
#define PI_IMPROVEDHASTE 110
#define PI_SPELLTRAP 117
#define PI_CSHIELD  162
#define PI_CSHIELD2 163

//the original engine stores the colors in sprklclr.2da in a different order

static ScriptedAnimation default_spell_hit;

int fx_ac_vs_damage_type_modifier (Scriptable* Owner, Actor* target, Effect* fx);//00
int fx_attacks_per_round_modifier (Scriptable* Owner, Actor* target, Effect* fx);//01
int fx_cure_sleep_state (Scriptable* Owner, Actor* target, Effect* fx);//02
int fx_set_berserk_state (Scriptable* Owner, Actor* target, Effect* fx);//03
int fx_cure_berserk_state (Scriptable* Owner, Actor* target, Effect* fx);//04
int fx_set_charmed_state (Scriptable* Owner, Actor* target, Effect* fx);//05
int fx_charisma_modifier (Scriptable* Owner, Actor* target, Effect* fx);//06
int fx_set_color_gradient (Scriptable* Owner, Actor* target, Effect* fx);//07
int fx_set_color_rgb (Scriptable* Owner, Actor* target, Effect* fx);//08
int fx_set_color_rgb_global (Scriptable* Owner, Actor* target, Effect* fx);//08
int fx_set_color_pulse_rgb (Scriptable* Owner, Actor* target, Effect* fx);//09
int fx_set_color_pulse_rgb_global (Scriptable* Owner, Actor* target, Effect* fx);//09
int fx_constitution_modifier (Scriptable* Owner, Actor* target, Effect* fx);//0a
int fx_cure_poisoned_state (Scriptable* Owner, Actor* target, Effect* fx);//0b
int fx_damage (Scriptable* Owner, Actor* target, Effect* fx);//0c
int fx_death (Scriptable* Owner, Actor* target, Effect* fx);//0d
int fx_cure_frozen_state (Scriptable* Owner, Actor* target, Effect* fx);//0e
int fx_dexterity_modifier (Scriptable* Owner, Actor* target, Effect* fx);//0f
int fx_set_hasted_state (Scriptable* Owner, Actor* target, Effect* fx);//10
int fx_current_hp_modifier (Scriptable* Owner, Actor* target, Effect* fx);//11
int fx_maximum_hp_modifier (Scriptable* Owner, Actor* target, Effect* fx);//12
int fx_intelligence_modifier (Scriptable* Owner, Actor* target, Effect* fx);//13
int fx_set_invisible_state (Scriptable* Owner, Actor* target, Effect* fx);//14
int fx_lore_modifier (Scriptable* Owner, Actor* target, Effect* fx);//15
int fx_luck_modifier (Scriptable* Owner, Actor* target, Effect* fx);//16
int fx_morale_modifier (Scriptable* Owner, Actor* target, Effect* fx);//17
int fx_set_panic_state (Scriptable* Owner, Actor* target, Effect* fx);//18
int fx_set_poisoned_state (Scriptable* Owner, Actor* target, Effect* fx);//19
int fx_remove_curse (Scriptable* Owner, Actor* target, Effect* fx);//1a
int fx_acid_resistance_modifier (Scriptable* Owner, Actor* target, Effect* fx);//1b
int fx_cold_resistance_modifier (Scriptable* Owner, Actor* target, Effect* fx);//1c
int fx_electricity_resistance_modifier (Scriptable* Owner, Actor* target, Effect* fx);//1d
int fx_fire_resistance_modifier (Scriptable* Owner, Actor* target, Effect* fx);//1e
int fx_magic_damage_resistance_modifier (Scriptable* Owner, Actor* target, Effect* fx);//1f
int fx_cure_dead_state (Scriptable* Owner, Actor* target, Effect* fx);//20
int fx_save_vs_death_modifier (Scriptable* Owner, Actor* target, Effect* fx);//21
int fx_save_vs_wands_modifier (Scriptable* Owner, Actor* target, Effect* fx);//22
int fx_save_vs_poly_modifier (Scriptable* Owner, Actor* target, Effect* fx);//23
int fx_save_vs_breath_modifier (Scriptable* Owner, Actor* target, Effect* fx);//24
int fx_save_vs_spell_modifier (Scriptable* Owner, Actor* target, Effect* fx);//25
int fx_set_silenced_state (Scriptable* Owner, Actor* target, Effect* fx);//26
int fx_set_unconscious_state (Scriptable* Owner, Actor* target, Effect* fx);//27
int fx_set_slowed_state (Scriptable* Owner, Actor* target, Effect* fx);//28
int fx_sparkle(Scriptable* Owner, Actor* target, Effect* fx);//29
int fx_bonus_wizard_spells (Scriptable* Owner, Actor* target, Effect* fx);//2a
int fx_cure_petrified_state (Scriptable* Owner, Actor* target, Effect* fx);//2b
int fx_strength_modifier (Scriptable* Owner, Actor* target, Effect* fx);//2c
int fx_set_stun_state (Scriptable* Owner, Actor* target, Effect* fx);//2d
int fx_cure_stun_state (Scriptable* Owner, Actor* target, Effect* fx);//2e
int fx_cure_invisible_state (Scriptable* Owner, Actor* target, Effect* fx);//2f
int fx_cure_silenced_state (Scriptable* Owner, Actor* target, Effect* fx);//30
int fx_wisdom_modifier (Scriptable* Owner, Actor* target, Effect* fx);//31
int fx_brief_rgb (Scriptable* Owner, Actor* target, Effect* fx);//32
int fx_darken_rgb (Scriptable* Owner, Actor* target, Effect* fx);//33
int fx_glow_rgb (Scriptable* Owner, Actor* target, Effect* fx);//34
int fx_animation_id_modifier (Scriptable* Owner, Actor* target, Effect* fx);//35
int fx_to_hit_modifier (Scriptable* Owner, Actor* target, Effect* fx);//36
int fx_kill_creature_type (Scriptable* Owner, Actor* target, Effect* fx);//37
int fx_alignment_invert (Scriptable* Owner, Actor* target, Effect* fx);//38
int fx_alignment_change (Scriptable* Owner, Actor* target, Effect* fx);//39
int fx_dispel_effects (Scriptable* Owner, Actor* target, Effect* fx);//3a
int fx_stealth_modifier (Scriptable* Owner, Actor* target, Effect* fx);//3b
int fx_miscast_magic_modifier (Scriptable* Owner, Actor* target, Effect* fx);//3c
int fx_alchemy_modifier (Scriptable* Owner, Actor* target, Effect* fx);//3d
int fx_bonus_priest_spells (Scriptable* Owner, Actor* target, Effect* fx);//3e
int fx_set_infravision_state (Scriptable* Owner, Actor* target, Effect* fx);//3f
int fx_cure_infravision_state (Scriptable* Owner, Actor* target, Effect* fx);//40
int fx_set_blur_state (Scriptable* Owner, Actor* target, Effect* fx);//41
int fx_transparency_modifier (Scriptable* Owner, Actor* target, Effect* fx);//42
int fx_summon_creature (Scriptable* Owner, Actor* target, Effect* fx);//43
int fx_unsummon_creature (Scriptable* Owner, Actor* target, Effect* fx);//44
int fx_set_nondetection_state (Scriptable* Owner, Actor* target, Effect* fx);//45
int fx_cure_nondetection_state (Scriptable* Owner, Actor* target, Effect* fx);//46
int fx_sex_modifier (Scriptable* Owner, Actor* target, Effect* fx);//47
int fx_ids_modifier (Scriptable* Owner, Actor* target, Effect* fx);//48
int fx_damage_bonus_modifier (Scriptable* Owner, Actor* target, Effect* fx);//49
int fx_damage_bonus_modifier2 (Scriptable* Owner, Actor* target, Effect* fx);//49 (iwd, ee)
int fx_set_blind_state (Scriptable* Owner, Actor* target, Effect* fx);//4a
int fx_cure_blind_state (Scriptable* Owner, Actor* target, Effect* fx);//4b
int fx_set_feebleminded_state (Scriptable* Owner, Actor* target, Effect* fx);//4c
int fx_cure_feebleminded_state (Scriptable* Owner, Actor* target, Effect* fx);//4d
int fx_set_diseased_state (Scriptable* Owner, Actor* target, Effect*fx);//4e
int fx_cure_diseased_state (Scriptable* Owner, Actor* target, Effect* fx);//4f
int fx_set_deaf_state (Scriptable* Owner, Actor* target, Effect* fx); //50
int fx_cure_deaf_state (Scriptable* Owner, Actor* target, Effect* fx);//51
int fx_set_ai_script (Scriptable* Owner, Actor* target, Effect*fx);//52
int fx_protection_from_projectile (Scriptable* Owner, Actor* target, Effect*fx);//53
int fx_magical_fire_resistance_modifier (Scriptable* Owner, Actor* target, Effect* fx);//54
int fx_magical_cold_resistance_modifier (Scriptable* Owner, Actor* target, Effect* fx);//55
int fx_slashing_resistance_modifier (Scriptable* Owner, Actor* target, Effect* fx);//56
int fx_crushing_resistance_modifier (Scriptable* Owner, Actor* target, Effect* fx);//57
int fx_piercing_resistance_modifier (Scriptable* Owner, Actor* target, Effect* fx);//58
int fx_missiles_resistance_modifier (Scriptable* Owner, Actor* target, Effect* fx);//59
int fx_open_locks_modifier (Scriptable* Owner, Actor* target, Effect* fx);//5a
int fx_find_traps_modifier (Scriptable* Owner, Actor* target, Effect* fx);//5b
int fx_pick_pockets_modifier (Scriptable* Owner, Actor* target, Effect* fx);//5c
int fx_fatigue_modifier (Scriptable* Owner, Actor* target, Effect* fx);//5d
int fx_intoxication_modifier (Scriptable* Owner, Actor* target, Effect* fx);//5e
int fx_tracking_modifier (Scriptable* Owner, Actor* target, Effect* fx);//5f
int fx_level_modifier (Scriptable* Owner, Actor* target, Effect* fx);//60
int fx_strength_bonus_modifier (Scriptable* Owner, Actor* target, Effect* fx);//61
int fx_set_regenerating_state (Scriptable* Owner, Actor* target, Effect* fx);//62
int fx_spell_duration_modifier (Scriptable* Owner, Actor* target, Effect* fx);///63
int fx_generic_effect (Scriptable* Owner, Actor* target, Effect* fx);//64 protection from creature is a generic effect
int fx_protection_opcode(Scriptable* Owner, Actor* target, Effect* fx); //65
int fx_protection_spelllevel (Scriptable* Owner, Actor* target, Effect* fx); //66
int fx_change_name (Scriptable* Owner, Actor* target, Effect* fx);//67
int fx_experience_modifier (Scriptable* Owner, Actor* target, Effect* fx);//68
int fx_gold_modifier (Scriptable* Owner, Actor* target, Effect* fx);//69
int fx_morale_break_modifier (Scriptable* Owner, Actor* target, Effect* fx);//6a
int fx_portrait_change (Scriptable* Owner, Actor* target, Effect* fx);//6b
int fx_reputation_modifier (Scriptable* Owner, Actor* target, Effect* fx);//6c
int fx_hold_creature_no_icon (Scriptable* Owner, Actor* target, Effect* fx);//6d
int fx_turn_undead (Scriptable* Owner, Actor* target, Effect* fx);//6e reused
int fx_create_magic_item (Scriptable* Owner, Actor* target, Effect* fx);//6f
int fx_remove_item (Scriptable* Owner, Actor* target, Effect* fx);//70
int fx_equip_item (Scriptable* Owner, Actor* target, Effect* fx);//71
int fx_dither (Scriptable* Owner, Actor* target, Effect* fx);//72
int fx_detect_alignment (Scriptable* Owner, Actor* target, Effect* fx);//73
//int fx_cure_improved_invisible_state (Scriptable* Owner, Actor* target, Effect* fx);//74 (2f)
int fx_reveal_area (Scriptable* Owner, Actor* target, Effect* fx);//75
int fx_reveal_creatures (Scriptable* Owner, Actor* target, Effect* fx);//76
int fx_mirror_image (Scriptable* Owner, Actor* target, Effect* fx);//77
int fx_immune_to_weapon (Scriptable* Owner, Actor* target, Effect* fx);//78
int fx_visual_animation_effect (Scriptable* Owner, Actor* target, Effect* fx);//79 unknown
int fx_create_inventory_item (Scriptable* Owner, Actor* target, Effect* fx);//7a
int fx_remove_inventory_item (Scriptable* Owner, Actor* target, Effect* fx);//7b
int fx_dimension_door (Scriptable* Owner, Actor* target, Effect* fx);//7c
int fx_knock (Scriptable* Owner, Actor* target, Effect* fx);//7d
int fx_movement_modifier (Scriptable* Owner, Actor* target, Effect* fx);//7e
int fx_monster_summoning (Scriptable* Owner, Actor* target, Effect* fx);//7f
int fx_set_confused_state (Scriptable* Owner, Actor* target, Effect* fx);//80
int fx_set_aid_state (Scriptable* Owner, Actor* target, Effect* fx);//81
int fx_set_bless_state (Scriptable* Owner, Actor* target, Effect* fx);//82
int fx_set_chant_state (Scriptable* Owner, Actor* target, Effect* fx);//83
int fx_set_holy_state (Scriptable* Owner, Actor* target, Effect* fx);//84
int fx_luck_non_cumulative (Scriptable* Owner, Actor* target, Effect* fx);//85
int fx_luck_cumulative (Scriptable* Owner, Actor* target, Effect* fx);//85
int fx_set_petrified_state (Scriptable* Owner, Actor* target, Effect* fx);//86
int fx_polymorph (Scriptable* Owner, Actor* target, Effect* fx);//87
int fx_force_visible (Scriptable* Owner, Actor* target, Effect* fx);//88
int fx_set_chantbad_state (Scriptable* Owner, Actor* target, Effect* fx);//89
int fx_animation_stance (Scriptable* Owner, Actor* target, Effect* fx);//8a
int fx_display_string (Scriptable* Owner, Actor* target, Effect* fx);//8b
int fx_casting_glow (Scriptable* Owner, Actor* target, Effect* fx);//8c
int fx_visual_spell_hit (Scriptable* Owner, Actor* target, Effect* fx);//8d
int fx_display_portrait_icon (Scriptable* Owner, Actor* target, Effect* fx);//8e
int fx_create_item_in_slot (Scriptable* Owner, Actor* target, Effect* fx);//8f
int fx_disable_button (Scriptable* Owner, Actor* target, Effect* fx);//90
int fx_disable_spellcasting (Scriptable* Owner, Actor* target, Effect* fx);//91
int fx_cast_spell (Scriptable* Owner, Actor* target, Effect *fx);//92
int fx_learn_spell (Scriptable* Owner, Actor* target, Effect *fx);//93
int fx_cast_spell_point (Scriptable* Owner, Actor* target, Effect *fx);//94
int fx_identify (Scriptable* Owner, Actor* target, Effect *fx);//95
int fx_find_traps (Scriptable* Owner, Actor* target, Effect *fx);//96
int fx_replace_creature (Scriptable* Owner, Actor* target, Effect *fx);//97
int fx_play_movie (Scriptable* Owner, Actor* target, Effect* fx);//98
int fx_set_sanctuary_state (Scriptable* Owner, Actor* target, Effect* fx);//99
int fx_set_entangle_state (Scriptable* Owner, Actor* target, Effect* fx);//9a
int fx_set_minorglobe_state (Scriptable* Owner, Actor* target, Effect* fx);//9b
int fx_set_shieldglobe_state (Scriptable* Owner, Actor* target, Effect* fx);//9c
int fx_set_web_state (Scriptable* Owner, Actor* target, Effect* fx);//9d
int fx_set_grease_state (Scriptable* Owner, Actor* target, Effect* fx);//9e
int fx_mirror_image_modifier (Scriptable* Owner, Actor* target, Effect* fx);//9f
int fx_cure_sanctuary_state (Scriptable* Owner, Actor* target, Effect* fx);//a0
int fx_cure_panic_state (Scriptable* Owner, Actor* target, Effect* fx);//a1
int fx_cure_hold_state (Scriptable* Owner, Actor* target, Effect* fx);//a2 //cures 175
int fx_cure_slow_state (Scriptable* Owner, Actor* target, Effect* fx);//a3
int fx_cure_intoxication (Scriptable* Owner, Actor* target, Effect* fx);//a4
int fx_pause_target (Scriptable* Owner, Actor* target, Effect* fx);//a5
int fx_magic_resistance_modifier (Scriptable* Owner, Actor* target, Effect* fx);//a6
int fx_missile_to_hit_modifier (Scriptable* Owner, Actor* target, Effect* fx);//a7
int fx_remove_creature (Scriptable* Owner, Actor* target, Effect* fx);//a8
int fx_disable_portrait_icon (Scriptable* Owner, Actor* target, Effect* fx);//a9
int fx_damage_animation (Scriptable* Owner, Actor* target, Effect* fx);//aa
int fx_add_innate (Scriptable* Owner, Actor* target, Effect* fx);//ab
int fx_remove_spell (Scriptable* Owner, Actor* target, Effect* fx);//ac
int fx_poison_resistance_modifier (Scriptable* Owner, Actor* target, Effect* fx);//ad
int fx_playsound (Scriptable* Owner, Actor* target, Effect* fx);//ae
int fx_hold_creature (Scriptable* Owner, Actor* target, Effect* fx);//af
// this function is exactly the same as 0x7e fx_movement_modifier (in bg2 at least)//b0
int fx_apply_effect (Scriptable* Owner, Actor* target, Effect* fx);//b1
//b2 //hitbonus against creature (generic_effect)
//b3 //damagebonus against creature (generic effect)
//b4 //restrict item (generic effect)
//b5 //restrict itemtype (generic effect)
int fx_apply_effect_item (Scriptable* Owner, Actor* target, Effect* fx);//b6
int fx_apply_effect_item_type (Scriptable* Owner, Actor* target, Effect* fx);//b7
int fx_dontjump_modifier (Scriptable* Owner, Actor* target, Effect* fx);//b8
// this function is exactly the same as 0xaf hold_creature (in bg2 at least) //b9
int fx_move_to_area (Scriptable* Owner, Actor* target, Effect* fx);//ba
int fx_local_variable (Scriptable* Owner, Actor* target, Effect* fx);//bb
int fx_auracleansing_modifier (Scriptable* Owner, Actor* target, Effect* fx);//bc
int fx_castingspeed_modifier (Scriptable* Owner, Actor* target, Effect* fx);//bd
int fx_attackspeed_modifier (Scriptable* Owner, Actor* target, Effect* fx);//be
int fx_castinglevel_modifier (Scriptable* Owner, Actor* target, Effect* fx);//bf
int fx_find_familiar (Scriptable* Owner, Actor* target, Effect* fx);//c0
int fx_see_invisible_modifier (Scriptable* Owner, Actor* target, Effect* fx);//c1
int fx_ignore_dialogpause_modifier (Scriptable* Owner, Actor* target, Effect* fx);//c2
int fx_familiar_constitution_loss (Scriptable* Owner, Actor* target, Effect* fx);//c3
int fx_familiar_marker (Scriptable* Owner, Actor* target, Effect* fx);//c4
int fx_bounce_projectile (Scriptable* Owner, Actor* target, Effect* fx);//c5
int fx_bounce_opcode (Scriptable* Owner, Actor* target, Effect* fx);//c6
int fx_bounce_spelllevel (Scriptable* Owner, Actor* target, Effect* fx);//c7
int fx_bounce_spelllevel_dec (Scriptable* Owner, Actor* target, Effect* fx);//c8
int fx_protection_spelllevel_dec (Scriptable* Owner, Actor* target, Effect* fx);//c9
int fx_bounce_school (Scriptable* Owner, Actor* target, Effect* fx);//ca
int fx_bounce_secondary_type (Scriptable* Owner, Actor* target, Effect* fx);//cb
int fx_protection_school (Scriptable* Owner, Actor* target, Effect* fx); //cc
int fx_protection_secondary_type (Scriptable* Owner, Actor* target, Effect* fx); //cd
int fx_resist_spell (Scriptable* Owner, Actor* target, Effect* fx);//ce
int fx_resist_spell2(Scriptable* Owner, Actor* target, Effect* fx); // ce (iwd2, ee), remapped to 0x200 in bgs to be also available there
int fx_resist_spell_dec (Scriptable* Owner, Actor* target, Effect* fx);//??
int fx_bounce_spell (Scriptable* Owner, Actor* target, Effect* fx);//cf
int fx_bounce_spell_dec (Scriptable* Owner, Actor* target, Effect* fx);//??
int fx_minimum_hp_modifier (Scriptable* Owner, Actor* target, Effect* fx);//d0
int fx_power_word_kill (Scriptable* Owner, Actor* target, Effect* fx);//d1
int fx_power_word_stun (Scriptable* Owner, Actor* target, Effect* fx);//d2
int fx_imprisonment (Scriptable* Owner, Actor* target, Effect* fx);//d3
int fx_freedom (Scriptable* Owner, Actor* target, Effect* fx);//d4
int fx_maze (Scriptable* Owner, Actor* target, Effect* fx);//d5
int fx_select_spell (Scriptable* Owner, Actor* target, Effect* fx);//d6
int fx_play_visual_effect (Scriptable* Owner, Actor* target, Effect* fx); //d7
int fx_leveldrain_modifier (Scriptable* Owner, Actor* target, Effect* fx);//d8
int fx_power_word_sleep (Scriptable* Owner, Actor* target, Effect* fx);//d9
int fx_stoneskin_modifier (Scriptable* Owner, Actor* target, Effect* fx);//da
//db ac vs creature type (general effect)
int fx_dispel_school (Scriptable* Owner, Actor* target, Effect* fx);//dc
int fx_dispel_secondary_type (Scriptable* Owner, Actor* target, Effect* fx);//dd
int fx_teleport_field (Scriptable* Owner, Actor* target, Effect* fx);//de
int fx_protection_school_dec (Scriptable* Owner, Actor* target, Effect* fx);//df
int fx_cure_leveldrain (Scriptable* Owner, Actor* target, Effect* fx);//e0
int fx_reveal_magic (Scriptable* Owner, Actor* target, Effect* fx);//e1
int fx_protection_secondary_type_dec (Scriptable* Owner, Actor* target, Effect* fx);//e2
int fx_bounce_school_dec (Scriptable* Owner, Actor* target, Effect* fx);//e3
int fx_bounce_secondary_type_dec (Scriptable* Owner, Actor* target, Effect* fx);//e4
int fx_dispel_school_one (Scriptable* Owner, Actor* target, Effect* fx);//e5
int fx_dispel_secondary_type_one (Scriptable* Owner, Actor* target, Effect* fx);//e6
int fx_timestop (Scriptable* Owner, Actor* target, Effect* fx);//e7
int fx_cast_spell_on_condition (Scriptable* Owner, Actor* target, Effect* fx);//e8
int fx_proficiency (Scriptable* Owner, Actor* target, Effect* fx);//e9
int fx_create_contingency (Scriptable* Owner, Actor* target, Effect* fx);//ea
int fx_wing_buffet (Scriptable* Owner, Actor* target, Effect* fx);//eb
int fx_puppet_master (Scriptable* Owner, Actor* target, Effect* fx);//ec
int fx_puppet_marker (Scriptable* Owner, Actor* target, Effect* fx);//ed
int fx_disintegrate (Scriptable* Owner, Actor* target, Effect* fx);//ee
int fx_farsee (Scriptable* Owner, Actor* target, Effect* fx);//ef
int fx_remove_portrait_icon (Scriptable* Owner, Actor* target, Effect* fx);//f0
//f1 control creature (see charm)
int fx_cure_confused_state (Scriptable* Owner, Actor* target, Effect* fx);//f2
int fx_drain_items (Scriptable* Owner, Actor* target, Effect* fx);//f3
int fx_drain_spells (Scriptable* Owner, Actor* target, Effect* fx);//f4
int fx_checkforberserk_modifier (Scriptable* Owner, Actor* target, Effect* fx);//f5
int fx_berserkstage1_modifier (Scriptable* Owner, Actor* target, Effect* fx);//f6
int fx_berserkstage2_modifier (Scriptable* Owner, Actor* target, Effect* fx);//f7
//int fx_melee_effect (Scriptable* Owner, Actor* target, Effect* fx);//f8
//int fx_ranged_effect (Scriptable* Owner, Actor* target, Effect* fx);//f9
int fx_damageluck_modifier (Scriptable* Owner, Actor* target, Effect* fx);//fa
int fx_change_bardsong (Scriptable* Owner, Actor* target, Effect* fx);//fb
int fx_set_area_effect (Scriptable* Owner, Actor* target, Effect* fx);//fc (set trap)
int fx_set_map_note (Scriptable* Owner, Actor* target, Effect* fx);//fd
int fx_remove_map_note (Scriptable* Owner, Actor* target, Effect* fx);//fe
int fx_create_item_days (Scriptable* Owner, Actor* target, Effect* fx);//ff
int fx_store_spell_sequencer (Scriptable* Owner, Actor* target, Effect* fx);//0x100
int fx_create_spell_sequencer (Scriptable* Owner, Actor* target, Effect* fx);//101
int fx_activate_spell_sequencer (Scriptable* Owner, Actor* target, Effect* fx);//102
int fx_spelltrap (Scriptable* Owner, Actor* target, Effect* fx);//103
int fx_crash (Scriptable* Owner, Actor* target, Effect* fx);//104, disabled
int fx_restore_spell_level (Scriptable* Owner, Actor* target, Effect* fx);//105
int fx_visual_range_modifier (Scriptable* Owner, Actor* target, Effect* fx);//106
int fx_backstab_modifier (Scriptable* Owner, Actor* target, Effect* fx);//107
int fx_drop_weapon (Scriptable* Owner, Actor* target, Effect* fx);//108
int fx_modify_global_variable (Scriptable* Owner, Actor* target, Effect* fx);//109
int fx_remove_immunity (Scriptable* Owner, Actor* target, Effect* fx);//10a
int fx_protection_from_string (Scriptable* Owner, Actor* target, Effect* fx);//10b
int fx_explore_modifier (Scriptable* Owner, Actor* target, Effect* fx);//10c
int fx_screenshake (Scriptable* Owner, Actor* target, Effect* fx);//10d
int fx_unpause_caster (Scriptable* Owner, Actor* target, Effect* fx);//10e
int fx_summon_disable (Scriptable* Owner, Actor* target, Effect* fx);//10f
int fx_apply_effect_repeat (Scriptable* Owner, Actor* target, Effect* fx);//110
int fx_remove_projectile (Scriptable* Owner, Actor* target, Effect* fx);//111
int fx_teleport_to_target (Scriptable* Owner, Actor* target, Effect* fx);//112
int fx_hide_in_shadows_modifier (Scriptable* Owner, Actor* target, Effect* fx);//113
int fx_detect_illusion_modifier (Scriptable* Owner, Actor* target, Effect* fx);//114
int fx_set_traps_modifier (Scriptable* Owner, Actor* target, Effect* fx);//115
int fx_to_hit_bonus_modifier (Scriptable* Owner, Actor* target, Effect* fx);//116
int fx_renable_button (Scriptable* Owner, Actor* target, Effect* fx);//117
int fx_force_surge_modifier (Scriptable* Owner, Actor* target, Effect* fx);//118
int fx_wild_surge_modifier (Scriptable* Owner, Actor* target, Effect* fx);//119
int fx_scripting_state (Scriptable* Owner, Actor* target, Effect* fx);//11a
int fx_apply_effect_curse (Scriptable* Owner, Actor* target, Effect* fx);//11b
int fx_melee_to_hit_modifier (Scriptable* Owner, Actor* target, Effect* fx);//11c
int fx_melee_damage_modifier (Scriptable* Owner, Actor* target, Effect* fx);//11d
int fx_missile_damage_modifier (Scriptable* Owner, Actor* target, Effect* fx);//11e
int fx_no_circle_state (Scriptable* Owner, Actor* target, Effect* fx);//11f
int fx_fist_to_hit_modifier (Scriptable* Owner, Actor* target, Effect* fx);//120
int fx_fist_damage_modifier (Scriptable* Owner, Actor* target, Effect* fx);//121
int fx_title_modifier (Scriptable* Owner, Actor* target, Effect* fx);//122
int fx_disable_overlay_modifier (Scriptable* Owner, Actor* target, Effect* fx);//123
int fx_no_backstab_modifier (Scriptable* Owner, Actor* target, Effect* fx);//124
int fx_offscreenai_modifier (Scriptable* Owner, Actor* target, Effect* fx);//125
int fx_existence_delay_modifier (Scriptable* Owner, Actor* target, Effect* fx);//126
int fx_disable_chunk_modifier (Scriptable* Owner, Actor* target, Effect* fx);//127
// fx_protection_from_animation implements as generic effect, 128
int fx_protection_from_turn (Scriptable* Owner, Actor* target, Effect* fx);//129
int fx_cutscene2 (Scriptable* Owner, Actor* target, Effect* fx);//12a
int fx_chaos_shield_modifier (Scriptable* Owner, Actor* target, Effect* fx);//12b
int fx_npc_bump (Scriptable* Owner, Actor* target, Effect* fx);//12c
int fx_critical_hit_modifier (Scriptable* Owner, Actor* target, Effect* fx);//12d
int fx_can_use_any_item_modifier (Scriptable* Owner, Actor* target, Effect* fx);//12e
int fx_always_backstab_modifier (Scriptable* Owner, Actor* target, Effect* fx);//12f
int fx_mass_raise_dead (Scriptable* Owner, Actor* target, Effect* fx);//130
int fx_left_to_hit_modifier (Scriptable* Owner, Actor* target, Effect* fx);//131
int fx_right_to_hit_modifier (Scriptable* Owner, Actor* target, Effect* fx);//132
int fx_reveal_tracks (Scriptable* Owner, Actor* target, Effect* fx);//133
int fx_protection_from_tracking (Scriptable* Owner, Actor* target, Effect* fx);//134
int fx_modify_local_variable (Scriptable* Owner, Actor* target, Effect* fx);//135
int fx_timeless_modifier (Scriptable* Owner, Actor* target, Effect* fx);//136
int fx_generate_wish (Scriptable* Owner, Actor* target, Effect* fx);//137
//138 see fx_crash
//139 HLA generic effect
int fx_golem_stoneskin_modifier (Scriptable* Owner, Actor* target, Effect* fx);//13a
int fx_avatar_removal_modifier (Scriptable* Owner, Actor* target, Effect* fx);//13b
int fx_magical_rest (Scriptable* Owner, Actor* target, Effect* fx);//13c
//int fx_improved_haste_state (Scriptable* Owner, Actor* target, Effect* fx);//13d same as haste
int fx_set_stat(Scriptable* Owner, Actor* target, Effect* fx); // 13e (tobex only), ees have fx_resist_spell2 here
int fx_item_usability(Scriptable* /*Owner*/, Actor* target, Effect* fx); // 13f (tobex and ee)
int fx_change_weather (Scriptable* Owner, Actor* target, Effect* fx);//140 ChangeWeather
int fx_remove_effects(Scriptable* Owner, Actor* target, Effect* fx); // 0x141 - 321
// 0x142 unused in ees
int fx_turnlevel_modifier(Scriptable* Owner, Actor* target, Effect* fx);
int fx_resist_spell_and_message(Scriptable* Owner, Actor* target, Effect *fx); // 0x144 (0x122) - 324
int fx_save_bonus(Scriptable* Owner, Actor* target, Effect* fx); // 0x145 in ees, ee in iwds
int fx_add_effects_list(Scriptable* Owner, Actor* target, Effect* fx); // 402 in iwd2, 0x146 in ees
int fx_iwd_visual_spell_hit(Scriptable* Owner, Actor* target, Effect* fx);
int fx_set_state(Scriptable* Owner, Actor* target, Effect* fx);
int fx_slow_poison(Scriptable* Owner, Actor* target, Effect* fx);
int fx_floattext(Scriptable* Owner, Actor* target, Effect* fx);
int fx_iwdee_monster_summoning(Scriptable* Owner, Actor* target, Effect* fx); // 0x14b in ees
// remapped fx_damage_bonus_modifier2
int fx_static_charge(Scriptable* Owner, Actor* target, Effect* fx); // 0x14d in ees
// remapped fx_turn_undead
int fx_seven_eyes(Scriptable* /*Owner*/, Actor* target, Effect* fx); // 0x14f in ees
// seven eyes displayer opcode
int fx_remove_effect(Scriptable* Owner, Actor* target, Effect* fx);
// disable rest is a generic effect
int fx_alter_animation(Scriptable* Owner, Actor *target, Effect* fx); // 0x153 in ees
int fx_change_backstab(Scriptable* /*Owner*/, Actor* target, Effect* fx); // 0x154 in ees
// generic effect x2
int fx_swap_hp(Scriptable* /*Owner*/, Actor* target, Effect* fx); // 0x157 in ees


int fx_set_concealment (Scriptable* Owner, Actor* target, Effect* fx); // 1ca - 458
int fx_uncanny_dodge (Scriptable* Owner, Actor* target, Effect* fx); // 1cb - 459

int fx_unknown (Scriptable* Owner, Actor* target, Effect* fx);//???

static EffectDesc effectnames[] = {
	EffectDesc("*Crash*", fx_crash, EFFECT_NO_ACTOR, -1 ),
	EffectDesc("AcidResistanceModifier", fx_acid_resistance_modifier, EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("ACVsCreatureType", fx_generic_effect, 0, -1 ), //0xdb
	EffectDesc("ACVsDamageTypeModifier", fx_ac_vs_damage_type_modifier, 0, -1 ),
	EffectDesc("ACVsDamageTypeModifier2", fx_ac_vs_damage_type_modifier, 0, -1 ), // used in IWD
	EffectDesc("AidNonCumulative", fx_set_aid_state, 0, -1 ),
	EffectDesc("AIIdentifierModifier", fx_ids_modifier, 0, -1 ),
	EffectDesc("AlchemyModifier", fx_alchemy_modifier, 0, -1 ),
	EffectDesc("Alignment:Change", fx_alignment_change, 0, -1 ),
	EffectDesc("Alignment:Invert", fx_alignment_invert, 0, -1 ),
	EffectDesc("AlterAnimation", fx_alter_animation, EFFECT_NO_ACTOR, -1),
	EffectDesc("AlwaysBackstab", fx_always_backstab_modifier, 0, -1 ),
	EffectDesc("AnimationIDModifier", fx_animation_id_modifier, 0, -1 ),
	EffectDesc("AnimationStateChange", fx_animation_stance, 0, -1 ),
	EffectDesc("AnimationOverrideData", fx_generic_effect, 0, -1),
	EffectDesc("ApplyEffect", fx_apply_effect, EFFECT_NO_ACTOR, -1 ),
	EffectDesc("ApplyEffectCurse", fx_apply_effect_curse, 0, -1 ),
	EffectDesc("ApplyEffectItem", fx_apply_effect_item, 0, -1 ),
	EffectDesc("ApplyEffectItemType", fx_apply_effect_item_type, 0, -1 ),
	EffectDesc("ApplyEffectsList", fx_add_effects_list, 0, -1),
	EffectDesc("ApplyEffectRepeat", fx_apply_effect_repeat, 0, -1 ),
	EffectDesc("CutScene2", fx_cutscene2, EFFECT_NO_ACTOR, -1 ),
	EffectDesc("AttackSpeedModifier", fx_attackspeed_modifier, 0, -1 ),
	EffectDesc("AttacksPerRoundModifier", fx_attacks_per_round_modifier, 0, -1 ),
	EffectDesc("AuraCleansingModifier", fx_auracleansing_modifier, 0, -1 ),
	EffectDesc("SummonDisable", fx_summon_disable, 0, -1 ), //unknown
	EffectDesc("AvatarRemovalModifier", fx_avatar_removal_modifier, 0, -1 ),
	EffectDesc("BackstabModifier", fx_backstab_modifier, 0, -1 ),
	EffectDesc("BerserkStage1Modifier", fx_berserkstage1_modifier, 0, -1 ),
	EffectDesc("BerserkStage2Modifier", fx_berserkstage2_modifier, 0, -1 ),
	EffectDesc("BlessNonCumulative", fx_set_bless_state, 0, -1 ),
	EffectDesc("Bounce:School", fx_bounce_school, 0, -1 ),
	EffectDesc("Bounce:SchoolDec", fx_bounce_school_dec, 0, -1 ),
	EffectDesc("Bounce:SecondaryType", fx_bounce_secondary_type, 0, -1 ),
	EffectDesc("Bounce:SecondaryTypeDec", fx_bounce_secondary_type_dec, 0, -1 ),
	EffectDesc("Bounce:Spell", fx_bounce_spell, 0, -1 ),
	EffectDesc("Bounce:SpellDec", fx_bounce_spell_dec, 0, -1 ),
	EffectDesc("Bounce:SpellLevel", fx_bounce_spelllevel, 0, -1 ),
	EffectDesc("Bounce:SpellLevelDec", fx_bounce_spelllevel_dec, 0, -1 ),
	EffectDesc("Bounce:Opcode", fx_bounce_opcode, 0, -1 ),
	EffectDesc("Bounce:Projectile", fx_bounce_projectile, 0, -1 ),
	EffectDesc("CantUseItem", fx_generic_effect, EFFECT_NO_ACTOR, -1 ),
	EffectDesc("CantUseItemType", fx_generic_effect, 0, -1 ),
	EffectDesc("CanUseAnyItem", fx_can_use_any_item_modifier, 0, -1 ),
	EffectDesc("CastFromList", fx_select_spell, 0, -1 ),
	EffectDesc("CastingGlow", fx_casting_glow, 0, -1 ),
	EffectDesc("CastingGlow2", fx_casting_glow, 0, -1 ), //used in iwd
	EffectDesc("CastingLevelModifier", fx_castinglevel_modifier, 0, -1 ),
	EffectDesc("CastingSpeedModifier", fx_castingspeed_modifier, 0, -1 ),
	EffectDesc("CastSpellOnCondition", fx_cast_spell_on_condition, 0, -1 ),
	EffectDesc("CastSpellOnCriticalHit", fx_generic_effect, 0, -1), // aka ChangeCritical
	EffectDesc("CastSpellOnCriticalMiss", fx_generic_effect, 0, -1),
	EffectDesc("ChangeBackstab", fx_change_backstab, 0, -1),
	EffectDesc("ChangeBardSong", fx_change_bardsong, 0, -1 ),
	EffectDesc("ChangeCritical", fx_generic_effect, 0, -1),
	EffectDesc("ChangeName", fx_change_name, 0, -1 ),
	EffectDesc("ChangeWeather", fx_change_weather, EFFECT_NO_ACTOR, -1 ),
	EffectDesc("ChantBadNonCumulative", fx_set_chantbad_state, 0, -1 ),
	EffectDesc("ChantNonCumulative", fx_set_chant_state, 0, -1 ),
	EffectDesc("ChaosShieldModifier", fx_chaos_shield_modifier, 0, -1 ),
	EffectDesc("CharismaModifier", fx_charisma_modifier, EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("CheckForBerserkModifier", fx_checkforberserk_modifier, 0, -1 ),
	EffectDesc("ColdResistanceModifier", fx_cold_resistance_modifier, EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("Color:BriefRGB", fx_brief_rgb, 0, -1 ),
	EffectDesc("Color:GlowRGB", fx_glow_rgb, 0, -1 ),
	EffectDesc("Color:DarkenRGB", fx_darken_rgb, 0, -1 ),
	EffectDesc("Color:SetPalette", fx_set_color_gradient, 0, -1 ),
	EffectDesc("Color:SetRGB", fx_set_color_rgb, 0, -1 ),
	EffectDesc("Color:SetRGBGlobal", fx_set_color_rgb_global, 0, -1 ), //08
	EffectDesc("Color:PulseRGB", fx_set_color_pulse_rgb, 0, -1 ), //9
	EffectDesc("Color:PulseRGBGlobal", fx_set_color_pulse_rgb_global, 0, -1 ), //9
	EffectDesc("ConstitutionModifier", fx_constitution_modifier, EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("ControlCreature", fx_set_charmed_state, 0, -1 ), //0xf1 same as charm
	EffectDesc("CreateContingency", fx_create_contingency, 0, -1 ),
	EffectDesc("CriticalHitModifier", fx_critical_hit_modifier, 0, -1 ),
	EffectDesc("CriticalMissModifier", fx_generic_effect, 0, -1),
	EffectDesc("CrushingResistanceModifier", fx_crushing_resistance_modifier, EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("Cure:Berserk", fx_cure_berserk_state, 0, -1 ),
	EffectDesc("Cure:Blind", fx_cure_blind_state, 0, -1 ),
	EffectDesc("Cure:CasterHold", fx_unpause_caster, 0, -1 ),
	EffectDesc("Cure:Confusion", fx_cure_confused_state, 0, -1 ),
	EffectDesc("Cure:Deafness", fx_cure_deaf_state, 0, -1 ),
	EffectDesc("Cure:Death", fx_cure_dead_state, 0, -1 ),
	EffectDesc("Cure:Defrost", fx_cure_frozen_state, 0, -1 ),
	EffectDesc("Cure:Disease", fx_cure_diseased_state, 0, -1 ),
	EffectDesc("Cure:Feeblemind", fx_cure_feebleminded_state, 0, -1 ),
	EffectDesc("Cure:Hold", fx_cure_hold_state, 0, -1 ),
	EffectDesc("Cure:Imprisonment", fx_freedom, 0, -1 ),
	EffectDesc("Cure:Infravision", fx_cure_infravision_state, 0, -1 ),
	EffectDesc("Cure:Intoxication", fx_cure_intoxication, 0, -1 ), //0xa4 (iwd2 has this working)
	EffectDesc("Cure:Invisible", fx_cure_invisible_state, 0, -1 ), //0x2f
	EffectDesc("Cure:Invisible2", fx_cure_invisible_state, 0, -1 ), //0x74
	//EffectDesc("Cure:ImprovedInvisible", fx_cure_improved_invisible_state, 0, -1 ),
	EffectDesc("Cure:LevelDrain", fx_cure_leveldrain, 0, -1 ), //restoration
	EffectDesc("Cure:Nondetection", fx_cure_nondetection_state, 0, -1 ),
	EffectDesc("Cure:Panic", fx_cure_panic_state, 0, -1 ),
	EffectDesc("Cure:Petrification", fx_cure_petrified_state, 0, -1 ),
	EffectDesc("Cure:Poison", fx_cure_poisoned_state, 0, -1 ),
	EffectDesc("Cure:Sanctuary", fx_cure_sanctuary_state, 0, -1 ),
	EffectDesc("Cure:Silence", fx_cure_silenced_state, 0, -1 ),
	EffectDesc("Cure:Sleep", fx_cure_sleep_state, 0, -1 ),
	EffectDesc("Cure:Stun", fx_cure_stun_state, 0, -1 ),
	EffectDesc("CurrentHPModifier", fx_current_hp_modifier, EFFECT_DICED, -1 ),
	EffectDesc("Damage", fx_damage, EFFECT_DICED, -1 ),
	EffectDesc("DamageAnimation", fx_damage_animation, 0, -1 ),
	EffectDesc("DamageBonusModifier", fx_damage_bonus_modifier, 0, -1 ),
	EffectDesc("DamageBonusModifier2", fx_damage_bonus_modifier, 0, -1), //49 (iwd, ee)
	EffectDesc("DamageLuckModifier", fx_damageluck_modifier, 0, -1 ),
	EffectDesc("DamageVsCreature", fx_generic_effect, 0, -1 ),
	EffectDesc("Death", fx_death, 0, -1 ),
	EffectDesc("Death2", fx_death, 0, -1 ), //(iwd2 effect)
	EffectDesc("Death3", fx_death, 0, -1 ), //(iwd2 effect too, Banish)
	EffectDesc("DetectAlignment", fx_detect_alignment, 0, -1 ),
	EffectDesc("DetectIllusionsModifier", fx_detect_illusion_modifier, 0, -1 ),
	EffectDesc("DexterityModifier", fx_dexterity_modifier, EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("DimensionDoor", fx_dimension_door, 0, -1 ),
	EffectDesc("DisableButton", fx_disable_button, 0, -1 ), //sets disable button flag
	EffectDesc("DisableChunk", fx_disable_chunk_modifier, 0, -1 ),
	EffectDesc("DisableOverlay", fx_disable_overlay_modifier, 0, -1 ),
	EffectDesc("DisableCasting", fx_disable_spellcasting, 0, -1 ),
	EffectDesc("DisableRest", fx_generic_effect, 0, -1),
	EffectDesc("Disintegrate", fx_disintegrate, 0, -1 ),
	EffectDesc("DispelEffects", fx_dispel_effects, 0, -1 ),
	EffectDesc("DispelSchool", fx_dispel_school, 0, -1 ),
	EffectDesc("DispelSchoolOne", fx_dispel_school_one, 0, -1 ),
	EffectDesc("DispelSecondaryType", fx_dispel_secondary_type, 0, -1 ),
	EffectDesc("DispelSecondaryTypeOne", fx_dispel_secondary_type_one, 0, -1 ),
	EffectDesc("DisplayString", fx_display_string, 0, -1 ),
	EffectDesc("Dither", fx_dither, 0, -1 ),
	EffectDesc("DontJumpModifier", fx_dontjump_modifier, 0, -1 ),
	EffectDesc("DrainItems", fx_drain_items, 0, -1 ),
	EffectDesc("DrainSpells", fx_drain_spells, 0, -1 ),
	EffectDesc("DropWeapon", fx_drop_weapon, 0, -1 ),
	EffectDesc("ElectricityResistanceModifier", fx_electricity_resistance_modifier, EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("EnchantmentBonus", fx_generic_effect, 0, -1),
	EffectDesc("EnchantmentVsCreatureType", fx_generic_effect, 0, -1),
	EffectDesc("ExistanceDelayModifier", fx_existence_delay_modifier , 0, -1 ),
	EffectDesc("ExperienceModifier", fx_experience_modifier, 0, -1 ),
	EffectDesc("ExploreModifier", fx_explore_modifier, 0, -1 ),
	EffectDesc("FamiliarBond", fx_familiar_constitution_loss, 0, -1 ),
	EffectDesc("FamiliarMarker", fx_familiar_marker, 0, -1 ),
	EffectDesc("Farsee", fx_farsee, 0, -1 ),
	EffectDesc("FatigueModifier", fx_fatigue_modifier, EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("FindFamiliar", fx_find_familiar, 0, -1 ),
	EffectDesc("FindTraps", fx_find_traps, 0, -1 ),
	EffectDesc("FindTrapsModifier", fx_find_traps_modifier, EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("FireResistanceModifier", fx_fire_resistance_modifier, EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("FistDamageModifier", fx_fist_damage_modifier, 0, -1 ),
	EffectDesc("FistHitModifier", fx_fist_to_hit_modifier, 0, -1 ),
	EffectDesc("FloatText", fx_floattext, 0, -1),
	EffectDesc("ForceSurgeModifier", fx_force_surge_modifier, 0, -1 ),
	EffectDesc("ForceVisible", fx_force_visible, 0, -1 ), //not invisible but improved invisible
	EffectDesc("FreeAction", fx_cure_slow_state, 0, -1 ),
	EffectDesc("GenerateWish", fx_generate_wish, 0, -1 ),
	EffectDesc("GoldModifier", fx_gold_modifier, 0, -1 ),
	EffectDesc("HideInShadowsModifier", fx_hide_in_shadows_modifier, 0, -1 ),
	EffectDesc("HLA", fx_generic_effect, 0, -1 ),
	EffectDesc("HolyNonCumulative", fx_set_holy_state, 0, -1 ),
	EffectDesc("Icon:Disable", fx_disable_portrait_icon, 0, -1 ),
	EffectDesc("Icon:Display", fx_display_portrait_icon, 0, -1 ),
	EffectDesc("Icon:Remove", fx_remove_portrait_icon, 0, -1 ),
	EffectDesc("Identify", fx_identify, 0, -1 ),
	EffectDesc("IgnoreDialogPause", fx_ignore_dialogpause_modifier, 0, -1 ),
	EffectDesc("IgnoreReputationBreakingPoint", fx_generic_effect, 0, -1),
	EffectDesc("IntelligenceModifier", fx_intelligence_modifier, EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("IntoxicationModifier", fx_intoxication_modifier, EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("InvisibleDetection", fx_see_invisible_modifier, 0, -1 ),
	EffectDesc("Item:CreateDays", fx_create_item_days, 0, -1 ),
	EffectDesc("Item:CreateInSlot", fx_create_item_in_slot, 0, -1 ),
	EffectDesc("Item:CreateInventory", fx_create_inventory_item, 0, -1 ),
	EffectDesc("Item:CreateMagic", fx_create_magic_item, 0, -1 ),
	EffectDesc("Item:Equip", fx_equip_item, 0, -1 ), //71
	EffectDesc("Item:Remove", fx_remove_item, 0, -1 ), //70
	EffectDesc("Item:RemoveInventory", fx_remove_inventory_item, 0, -1 ),
	EffectDesc("IWDEEMonsterSummoning", fx_iwdee_monster_summoning, EFFECT_NO_ACTOR, -1),
	EffectDesc("IWDVisualSpellHit", fx_iwd_visual_spell_hit, EFFECT_NO_ACTOR, -1),
	EffectDesc("KillCreatureType", fx_kill_creature_type, 0, -1 ),
	EffectDesc("LevelModifier", fx_level_modifier, 0, -1 ),
	EffectDesc("LevelDrainModifier", fx_leveldrain_modifier, 0, -1 ),
	EffectDesc("LoreModifier", fx_lore_modifier, EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("LuckModifier", fx_luck_modifier, EFFECT_NO_LEVEL_CHECK|EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("LuckCumulative", fx_luck_cumulative, 0, -1 ),
	EffectDesc("LuckNonCumulative", fx_luck_non_cumulative, 0, -1 ),
	EffectDesc("MagicalColdResistanceModifier", fx_magical_cold_resistance_modifier, EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("MagicalFireResistanceModifier", fx_magical_fire_resistance_modifier, EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("MagicalRest", fx_magical_rest, 0, -1 ),
	EffectDesc("MagicDamageResistanceModifier", fx_magic_damage_resistance_modifier, 0, -1 ),
	EffectDesc("MagicResistanceModifier", fx_magic_resistance_modifier, 0, -1 ),
	EffectDesc("MassRaiseDead", fx_mass_raise_dead, EFFECT_NO_ACTOR, -1 ),
	EffectDesc("MaximumHPModifier", fx_maximum_hp_modifier, EFFECT_DICED|EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("Maze", fx_maze, 0, -1 ),
	EffectDesc("MeleeDamageModifier", fx_melee_damage_modifier, 0, -1 ),
	EffectDesc("MeleeHitModifier", fx_melee_to_hit_modifier, 0, -1 ),
	EffectDesc("MinimumBaseStats", fx_generic_effect, 0, -1),
	EffectDesc("MinimumHPModifier", fx_minimum_hp_modifier, 0, -1 ),
	EffectDesc("MiscastMagicModifier", fx_miscast_magic_modifier, 0, -1 ),
	EffectDesc("MissileDamageModifier", fx_missile_damage_modifier, 0, -1 ),
	EffectDesc("MissileHitModifier", fx_missile_to_hit_modifier, 0, -1 ),
	EffectDesc("MissilesResistanceModifier", fx_missiles_resistance_modifier, EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("MirrorImage", fx_mirror_image, 0, -1 ),
	EffectDesc("MirrorImageModifier", fx_mirror_image_modifier, 0, -1 ),
	EffectDesc("ModifyGlobalVariable", fx_modify_global_variable, EFFECT_NO_ACTOR, -1 ),
	EffectDesc("ModifyLocalVariable", fx_modify_local_variable, 0, -1 ),
	EffectDesc("MonsterSummoning", fx_monster_summoning, EFFECT_NO_ACTOR, -1 ),
	EffectDesc("MoraleBreakModifier", fx_morale_break_modifier, EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("MoraleModifier", fx_morale_modifier, 0, -1 ),
	EffectDesc("MovementRateModifier", fx_movement_modifier, 0, -1 ), //fast (7e)
	EffectDesc("MovementRateModifier2", fx_movement_modifier, 0, -1 ),//slow (b0)
	EffectDesc("MovementRateModifier3", fx_movement_modifier, 0, -1 ),//forced (IWD - 10a)
	EffectDesc("MovementRateModifier4", fx_movement_modifier, 0, -1 ),//slow (IWD2 - 1b9)
	EffectDesc("MoveToArea", fx_move_to_area, EFFECT_REINIT_ON_LOAD, -1 ), //0xba
	EffectDesc("NoCircleState", fx_no_circle_state, 0, -1 ),
	EffectDesc("NPCBump", fx_npc_bump, 0, -1 ),
	EffectDesc("OffscreenAIModifier", fx_offscreenai_modifier, 0, -1 ),
	EffectDesc("OffhandHitModifier", fx_left_to_hit_modifier, 0, -1 ),
	EffectDesc("OpenLocksModifier", fx_open_locks_modifier, EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("Overlay:Entangle", fx_set_entangle_state, 0, -1 ),
	EffectDesc("Overlay:Grease", fx_set_grease_state, 0, -1 ),
	EffectDesc("Overlay:MinorGlobe", fx_set_minorglobe_state, 0, -1 ),
	EffectDesc("Overlay:Sanctuary", fx_set_sanctuary_state, 0, -1 ),
	EffectDesc("Overlay:ShieldGlobe", fx_set_shieldglobe_state, 0, -1 ),
	EffectDesc("Overlay:Web", fx_set_web_state, 0, -1 ),
	EffectDesc("PauseTarget", fx_pause_target, 0, -1 ), //also known as casterhold
	EffectDesc("PickPocketsModifier", fx_pick_pockets_modifier, EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("PiercingResistanceModifier", fx_piercing_resistance_modifier, EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("PlayMovie", fx_play_movie, EFFECT_NO_ACTOR, -1 ),
	EffectDesc("PlaySound", fx_playsound, EFFECT_NO_ACTOR, -1 ),
	EffectDesc("PlayVisualEffect", fx_play_visual_effect, EFFECT_REINIT_ON_LOAD, -1 ),
	EffectDesc("PoisonResistanceModifier", fx_poison_resistance_modifier, 0, -1 ),
	EffectDesc("Polymorph", fx_polymorph, 0, -1 ),
	EffectDesc("PortraitChange", fx_portrait_change, 0, -1 ),
	EffectDesc("PowerWordKill", fx_power_word_kill, 0, -1 ),
	EffectDesc("PowerWordSleep", fx_power_word_sleep, 0, -1 ),
	EffectDesc("PowerWordStun", fx_power_word_stun, 0, -1 ),
	EffectDesc("PriestSpellSlotsModifier", fx_bonus_priest_spells, 0, -1 ),
	EffectDesc("Proficiency", fx_proficiency, 0, -1 ),
	EffectDesc("Protection:Animation", fx_generic_effect, 0, -1 ),
	EffectDesc("Protection:Backstab", fx_no_backstab_modifier, 0, -1 ),
	EffectDesc("Protection:Creature", fx_generic_effect, 0, -1 ),
	EffectDesc("Protection:Opcode", fx_protection_opcode, 0, -1 ),
	EffectDesc("Protection:Opcode2", fx_protection_opcode, 0, -1 ),
	EffectDesc("Protection:Projectile",fx_protection_from_projectile, 0, -1 ),
	EffectDesc("Protection:School",fx_protection_school, 0, -1 ),//overlay?
	EffectDesc("Protection:SchoolDec",fx_protection_school_dec, 0, -1 ),//overlay?
	EffectDesc("Protection:SecondaryType",fx_protection_secondary_type, 0, -1 ),//overlay?
	EffectDesc("Protection:SecondaryTypeDec",fx_protection_secondary_type_dec, 0, -1 ),//overlay?
	EffectDesc("Protection:Spell",fx_resist_spell, 0, -1 ),//overlay?
	EffectDesc("Protection:Spell2", fx_resist_spell2, 0, -1),
	EffectDesc("Protection:Spell3", fx_resist_spell_and_message, 0, -1),
	EffectDesc("Protection:SpellDec",fx_resist_spell_dec, 0, -1 ),//overlay?
	EffectDesc("Protection:SpellLevel",fx_protection_spelllevel, 0, -1 ),//overlay?
	EffectDesc("Protection:SpellLevelDec",fx_protection_spelllevel_dec, 0, -1 ),//overlay?
	EffectDesc("Protection:String", fx_protection_from_string, 0, -1),
	EffectDesc("Protection:Tracking", fx_protection_from_tracking, 0, -1 ),
	EffectDesc("Protection:Turn", fx_protection_from_turn, 0, -1 ),
	EffectDesc("Protection:Weapons", fx_immune_to_weapon, EFFECT_NO_ACTOR|EFFECT_REINIT_ON_LOAD, -1 ),
	EffectDesc("PuppetMarker", fx_puppet_marker, 0, -1 ),
	EffectDesc("ProjectImage", fx_puppet_master, 0, -1 ),
	EffectDesc("Reveal:Area", fx_reveal_area, EFFECT_NO_ACTOR, -1 ),
	EffectDesc("Reveal:Creatures", fx_reveal_creatures, 0, -1 ),
	EffectDesc("Reveal:Magic", fx_reveal_magic, 0, -1 ),
	EffectDesc("Reveal:Tracks", fx_reveal_tracks, 0, -1 ),
	EffectDesc("RemoveCreature", fx_remove_creature, EFFECT_NO_ACTOR, -1),
	EffectDesc("RemoveCurse", fx_remove_curse, 0, -1 ),
	EffectDesc("RemoveEffect", fx_remove_effect, 0, -1),
	EffectDesc("RemoveEffectsByResource", fx_remove_effects, 0, -1),
	EffectDesc("RemoveImmunity", fx_remove_immunity, 0, -1 ),
	EffectDesc("RemoveMapNote", fx_remove_map_note, EFFECT_NO_ACTOR, -1 ),
	EffectDesc("RemoveProjectile", fx_remove_projectile, 0, -1 ), //removes effects from actor and area
	EffectDesc("RenableButton", fx_renable_button, 0, -1 ), //removes disable button flag
	EffectDesc("ReplaceCreature", fx_replace_creature, 0, -1 ),
	EffectDesc("ReputationModifier", fx_reputation_modifier, 0, -1 ),
	EffectDesc("RestoreSpells", fx_restore_spell_level, 0, -1 ),
	EffectDesc("RetreatFrom2", fx_turn_undead, 0, -1 ),
	EffectDesc("RightHitModifier", fx_right_to_hit_modifier, 0, -1 ),
	EffectDesc("SaveBonus", fx_save_bonus, 0, -1),
	EffectDesc("SaveVsBreathModifier", fx_save_vs_breath_modifier, EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("SaveVsDeathModifier", fx_save_vs_death_modifier, EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("SaveVsPolyModifier", fx_save_vs_poly_modifier, EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("SaveVsSchoolModifier", fx_generic_effect, 0, -1),
	EffectDesc("SaveVsSpellsModifier", fx_save_vs_spell_modifier, EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("SaveVsWandsModifier", fx_save_vs_wands_modifier, EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("ScreenShake", fx_screenshake, EFFECT_NO_ACTOR, -1 ),
	EffectDesc("ScriptingState", fx_scripting_state, 0, -1 ),
	EffectDesc("Sequencer:Activate", fx_activate_spell_sequencer, EFFECT_PRESET_TARGET, -1 ),
	EffectDesc("Sequencer:Create", fx_create_spell_sequencer, 0, -1 ),
	EffectDesc("Sequencer:Store", fx_store_spell_sequencer, 0, -1 ),
	EffectDesc("SetAIScript", fx_set_ai_script, 0, -1 ),
	EffectDesc("SetConcealment", fx_set_concealment, 0, -1 ),
	EffectDesc("SetMapNote", fx_set_map_note, EFFECT_NO_ACTOR, -1 ),
	EffectDesc("SetMeleeEffect", fx_generic_effect, 0, -1 ),
	EffectDesc("SetRangedEffect", fx_generic_effect, 0, -1 ),
	EffectDesc("SetTrap", fx_set_area_effect, 0, -1 ),
	EffectDesc("SetTrapsModifier", fx_set_traps_modifier, 0, -1 ),
	EffectDesc("SevenEyes", fx_seven_eyes, 0, -1),
	EffectDesc("SexModifier", fx_sex_modifier, 0, -1 ),
	EffectDesc("SlashingResistanceModifier", fx_slashing_resistance_modifier, EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("SlowPoison", fx_slow_poison, 0, -1),
	EffectDesc("Sparkle", fx_sparkle, 0, -1 ),
	EffectDesc("SpellDurationModifier", fx_spell_duration_modifier, 0, -1 ),
	EffectDesc("Spell:Add", fx_add_innate, 0, -1 ),
	EffectDesc("Spell:Cast", fx_cast_spell, 0, -1 ),
	EffectDesc("Spell:CastPoint", fx_cast_spell_point, 0, -1 ),
	EffectDesc("Spell:Learn", fx_learn_spell, 0, -1 ),
	EffectDesc("Spell:Remove", fx_remove_spell, 0, -1 ),
	EffectDesc("SpellFocus",fx_generic_effect , 0, -1 ), //to implement school specific saving throw penalty to opponent
	EffectDesc("SpellResistance",fx_generic_effect , 0, -1 ), //to implement school specific saving throw bonus
	EffectDesc("Spelltrap",fx_spelltrap , 0, -1 ), //overlay: spmagglo
	EffectDesc("Stat:SetStat", fx_set_stat, 0, -1),
	EffectDesc("State:Berserk", fx_set_berserk_state, 0, -1 ),
	EffectDesc("State:Blind", fx_set_blind_state, 0, -1 ),
	EffectDesc("State:Blur", fx_set_blur_state, 0, -1 ),
	EffectDesc("State:Charmed", fx_set_charmed_state, EFFECT_NO_LEVEL_CHECK, -1 ), //0x05
	EffectDesc("State:Confused", fx_set_confused_state, 0, -1 ),
	EffectDesc("State:Deafness", fx_set_deaf_state, 0, -1 ),
	EffectDesc("State:Diseased", fx_set_diseased_state, 0, -1 ),
	EffectDesc("State:Feeblemind", fx_set_feebleminded_state, 0, -1 ),
	EffectDesc("State:Hasted", fx_set_hasted_state, 0, -1 ),
	EffectDesc("State:Haste2", fx_set_hasted_state, 0, -1 ),
	EffectDesc("State:Hold", fx_hold_creature, 0, -1 ), //175 (doesn't work in original iwd2)
	EffectDesc("State:Hold2", fx_hold_creature, 0, -1 ),//185 (doesn't work in original iwd2)
	EffectDesc("State:Hold3", fx_hold_creature, 0, -1 ),//109 iwd2
	EffectDesc("State:HoldNoIcon", fx_hold_creature_no_icon, 0, -1 ), //109 (bg2) 0x6d
	EffectDesc("State:HoldNoIcon2", fx_hold_creature_no_icon, 0, -1 ), //0xfb (iwd/iwd2)
	EffectDesc("State:HoldNoIcon3", fx_hold_creature_no_icon, 0, -1 ), //0x1a8 (iwd2)
	EffectDesc("State:Imprisonment", fx_imprisonment, 0, -1 ),
	EffectDesc("State:Infravision", fx_set_infravision_state, 0, -1 ),
	EffectDesc("State:Invisible", fx_set_invisible_state, 0, -1 ), //both invis or improved invis
	EffectDesc("State:Nondetection", fx_set_nondetection_state, 0, -1 ),
	EffectDesc("State:Panic", fx_set_panic_state, 0, -1 ),
	EffectDesc("State:Petrification", fx_set_petrified_state, 0, -1 ),
	EffectDesc("State:Poisoned", fx_set_poisoned_state, 0, -1 ),
	EffectDesc("State:Regenerating", fx_set_regenerating_state, 0, -1 ),
	EffectDesc("State:Set", fx_set_state, 0, -1),
	EffectDesc("State:Silenced", fx_set_silenced_state, 0, -1 ),
	EffectDesc("State:Helpless", fx_set_unconscious_state, 0, -1 ),
	EffectDesc("State:Sleep", fx_set_unconscious_state, 0, -1 ),
	EffectDesc("State:Slowed", fx_set_slowed_state, 0, -1 ),
	EffectDesc("State:Stun", fx_set_stun_state, 0, -1 ),
	EffectDesc("StaticCharge", fx_static_charge, EFFECT_NO_LEVEL_CHECK, -1),
	EffectDesc("StealthModifier", fx_stealth_modifier, 0, -1 ),
	EffectDesc("StoneSkinModifier", fx_stoneskin_modifier, 0, -1 ),
	EffectDesc("StoneSkin2Modifier", fx_golem_stoneskin_modifier, 0, -1 ),
	EffectDesc("StrengthModifier", fx_strength_modifier, EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("StrengthBonusModifier", fx_strength_bonus_modifier, 0, -1 ),
	EffectDesc("SummonCreature", fx_summon_creature, EFFECT_NO_ACTOR, -1 ),
	EffectDesc("SwapHP", fx_swap_hp, 0, -1),
	EffectDesc("RandomTeleport", fx_teleport_field, 0, -1 ),
	EffectDesc("TeleportToTarget", fx_teleport_to_target, 0, -1 ),
	EffectDesc("TimelessState", fx_timeless_modifier, 0, -1 ),
	EffectDesc("Timestop", fx_timestop, 0, -1 ),
	EffectDesc("TitleModifier", fx_title_modifier, 0, -1 ),
	EffectDesc("ToHitModifier", fx_to_hit_modifier, EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("ToHitBonusModifier", fx_to_hit_bonus_modifier, EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("ToHitVsCreature", fx_generic_effect, 0, -1 ),
	EffectDesc("TrackingModifier", fx_tracking_modifier, EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("TransparencyModifier", fx_transparency_modifier, 0, -1 ),
	EffectDesc("TurnUndead", fx_turn_undead, 0, -1 ),
	EffectDesc("TurnLevelModifier", fx_turnlevel_modifier, 0, -1),
	EffectDesc("UncannyDodge", fx_uncanny_dodge, 0, -1 ),
	EffectDesc("Unknown", fx_unknown, EFFECT_NO_ACTOR, -1 ),
	EffectDesc("Unlock", fx_knock, EFFECT_NO_ACTOR, -1 ), //open doors/containers
	EffectDesc("UnsummonCreature", fx_unsummon_creature, EFFECT_NO_LEVEL_CHECK, -1 ),
	EffectDesc("Usability:ItemUsability", fx_item_usability, EFFECT_NO_LEVEL_CHECK, -1 ),
	EffectDesc("Variable:StoreLocalVariable", fx_local_variable, 0, -1 ),
	EffectDesc("VisualAnimationEffect", fx_visual_animation_effect, 0, -1 ), //unknown
	EffectDesc("VisualRangeModifier", fx_visual_range_modifier, 0, -1 ),
	EffectDesc("VisualSpellHit", fx_visual_spell_hit, 0, -1 ),
	EffectDesc("WildSurgeModifier", fx_wild_surge_modifier, 0, -1 ),
	EffectDesc("WingBuffet", fx_wing_buffet, 0, -1 ),
	EffectDesc("WisdomModifier", fx_wisdom_modifier, EFFECT_SPECIAL_UNDO, -1 ),
	EffectDesc("WizardSpellSlotsModifier", fx_bonus_wizard_spells, 0, -1 ),
	EffectDesc(NULL, NULL, 0, 0 ),
};

static EffectRef fx_set_berserk_state_ref = { "State:Berserk", -1 };//0x3
static EffectRef fx_set_charmed_state_ref = { "State:Charmed", -1 }; // 0x5
static EffectRef fx_constitution_modifier_ref = { "ConstitutionModifier", -1 }; //0xa
static EffectRef fx_damage_opcode_ref = { "Damage", -1 };  //0xc
static EffectRef fx_death_ref = { "Death", -1 }; //0xd
static EffectRef fx_set_haste_state_ref = { "State:Hasted", -1 }; //0x10
static EffectRef fx_maximum_hp_modifier_ref = { "MaximumHPModifier", -1 }; //0x12
static EffectRef fx_set_invisible_state_ref = { "State:Invisible", -1 }; //0x14
static EffectRef fx_set_panic_state_ref = { "State:Panic", -1 }; //0x18
static EffectRef fx_poisoned_state_ref = { "State:Poisoned", -1 }; //0x19
static EffectRef fx_set_silenced_state_ref = { "State:Silenced", -1 }; //0x26
static EffectRef fx_set_sleep_state_ref = { "State:Helpless", -1 }; //0x27
static EffectRef fx_set_slow_state_ref = { "State:Slowed", -1 }; //0x28
static EffectRef fx_sparkle_ref = { "Sparkle", -1 }; //0x29
static EffectRef fx_set_stun_state_ref = { "State:Stun", -1 }; //0x2d
static EffectRef fx_animation_id_modifier_ref = { "AnimationIDModifier", -1 }; //0x35
static EffectRef fx_set_infravision_state_ref = { "State:Infravision", -1 }; //0x3f
static EffectRef fx_set_nondetection_state_ref = { "State:Nondetection", -1 }; //0x45
static EffectRef fx_set_blind_state_ref = { "State:Blind", -1 }; //0x4a
static EffectRef fx_set_feebleminded_state_ref = { "State:Feeblemind", -1 }; //0x4c
static EffectRef fx_diseased_state_ref = { "State:Diseased", -1 }; //0x4e
static EffectRef fx_deaf_state_ref = { "State:Deafness", -1 }; //0x50
static EffectRef fx_fatigue_ref = { "FatigueModifier", -1 }; //0x5d
static EffectRef fx_intoxication_ref = { "IntoxicationModifier", -1 }; //0x5e
static EffectRef fx_hold_creature_no_icon_ref = { "State:HoldNoIcon", -1 }; //0x6d
static EffectRef fx_remove_item_ref = { "Item:Remove", -1 }; //0x70
static EffectRef fx_remove_inventory_item_ref = { "Item:RemoveInventory", -1 }; //0x7b
static EffectRef fx_confused_state_ref = { "State:Confused", -1 }; //0x80
static EffectRef fx_polymorph_ref = { "Polymorph", -1 }; //0x87
static EffectRef fx_animation_stance_ref = { "AnimationStateChange", -1 }; //0x8a
static EffectRef fx_display_portrait_icon_ref = { "Icon:Display", -1 };//0x8e
static EffectRef fx_disable_button_ref = { "DisableButton", -1 }; //0x90
static EffectRef fx_sanctuary_state_ref = { "Overlay:Sanctuary", -1 }; //0x99
static EffectRef fx_mirror_image_modifier_ref = { "MirrorImageModifier", -1 }; //0x9f
static EffectRef fx_pause_caster_modifier_ref = { "PauseTarget", -1 }; //0xa5
static EffectRef fx_hold_creature_ref = { "State:Hold", -1 }; //0xaf
static EffectRef fx_movement_modifier_ref = { "MovementRateModifier2", -1 }; //0xb0
static EffectRef fx_familiar_constitution_loss_ref = { "FamiliarBond", -1 }; //0xc3
static EffectRef fx_familiar_marker_ref = { "FamiliarMarker", -1 }; //0xc4
static EffectRef fx_immunity_effect_ref = { "Protection:Spell", -1 }; //0xce
static EffectRef fx_resist_spell2_ref = { "Protection:Spell2", -1 }; //0xce (IWD2, ee), 0x200 bgs
static EffectRef fx_imprisonment_ref = { "State:Imprisonment", -1 }; //0xd3
static EffectRef fx_maze_ref = { "Maze", -1 }; //0xd5
static EffectRef fx_leveldrain_ref = { "LevelDrainModifier", -1 }; //0xd8
static EffectRef fx_contingency_ref = { "CastSpellOnCondition", -1 }; //0xe8
static EffectRef fx_puppetmarker_ref = { "PuppetMarker", -1 }; //0xed
static EffectRef fx_spell_sequencer_active_ref = { "Sequencer:Store", -1 }; //0x100
static EffectRef fx_protection_from_display_string_ref = { "Protection:String", -1 }; //0x10b
static EffectRef fx_apply_effect_repeat_ref = { "ApplyEffectRepeat", -1 }; //0x110
static EffectRef fx_death_ward_ref = { "DeathWard", -1 };  //iwd2 specific
static EffectRef fx_death_magic_ref = { "Death2", -1 };    //iwd2 specific
static EffectRef fx_apply_effect_curse_ref = { "ApplyEffectCurse", -1 }; //0x11b
static EffectRef fx_pst_jumble_curse_ref = { "JumbleCurse", -1 };  //PST specific
static EffectRef fx_bane_ref = { "Bane", -1 }; //iwd2
static EffectRef fx_protection_from_animation_ref = { "Protection:Animation", -1 }; //0x128
static EffectRef fx_change_bardsong_ref = { "ChangeBardSong", -1 };
static EffectRef fx_eye_stone_ref = { "EyeOfStone", -1 };
static EffectRef fx_eye_spirit_ref = { "EyeOfTheSpirit", -1 };
static EffectRef fx_eye_venom_ref = { "EyeOfVenom", -1 };
static EffectRef fx_eye_mind_ref = { "EyeOfTheMind", -1 };
static EffectRef fx_eye_fortitude_ref = { "EyeOfFortitude", -1 };
static EffectRef fx_remove_effects_ref = { "RemoveEffectsByResource", -1 };
static EffectRef fx_wound_ref = { "BleedingWounds", -1 }; // 416

static EffectRef fx_str_ref = { "StrengthModifier", -1 };
static EffectRef fx_int_ref = { "IntelligenceModifier", -1 };
static EffectRef fx_wis_ref = { "WisdomModifier", -1 };
static EffectRef fx_dex_ref = { "DexterityModifier", -1 };
static EffectRef fx_con_ref = { "ConstitutionModifier", -1 };
static EffectRef fx_chr_ref = { "CharismaModifier", -1 };

static void RegisterCoreOpcodes(const CoreSettings&)
{
	core->RegisterOpcodes( sizeof( effectnames ) / sizeof( EffectDesc ) - 1, effectnames );
	default_spell_hit.SequenceFlags|=IE_VVC_BAM;
}


#define STONE_GRADIENT 14 // for stoneskin, not petrification
#define ICE_GRADIENT 71
static const ieDword fullstone[7]= { STONE_GRADIENT, STONE_GRADIENT, STONE_GRADIENT, STONE_GRADIENT, STONE_GRADIENT, STONE_GRADIENT, STONE_GRADIENT };

static inline void SetGradient(Actor *target,const ieDword *gradients)
{
	for(int i=0;i<7;i++) {
		int gradient = gradients[i];
		gradient |= (gradient <<16);
		gradient |= (gradient <<8);

		STAT_SET(IE_COLORS+i, gradient);
	}
	target->SetLockedPalette(gradients);
}

static inline void HandleBonus(Actor *target, int stat, int mod, int mode)
{
	if (mode==FX_DURATION_INSTANT_PERMANENT) {
		if (Actor::IsReverseToHit()) {
			BASE_SUB( stat, mod );
		} else {
			BASE_ADD( stat, mod );
		}
		return;
	}
	if (Actor::IsReverseToHit()) {
		STAT_SUB( stat, mod );
	} else {
		STAT_ADD( stat, mod );
	}
}

// handle 3ed's effect exclusion for main stats, where only the highest bonus counts
// NOTE: handles only additive bonuses, since others aren't present and would only muddy matters
static inline void HandleMainStatBonus(const Actor *target, int stat, Effect *fx)
{
	// ignoring strextra, since it won't be used
	EffectRef mainStatRefs[] = { fx_str_ref, fx_str_ref, fx_int_ref, fx_wis_ref, fx_dex_ref, fx_con_ref, fx_chr_ref };

	int bonus = fx->Parameter1;
	if (!core->HasFeature(GFFlags::RULES_3ED) || fx->Parameter2 != MOD_ADDITIVE) return;

	if (fx->TimingMode == FX_DURATION_INSTANT_PERMANENT) {
		// don't touch it, so eg. potion of holy transference still works 00POTN89
		return;
	}

	// restore initial value in case we annulled it the previous tick
	if (!bonus && fx->Parameter3 != 0) {
		bonus = fx->Parameter3;
		fx->Parameter3 = 0;
	}
	if (!bonus) return;

	// get lowest and highest current bonus, if any
	EffectRef &eff_ref = mainStatRefs[stat-IE_STR];

	// maybe it's the only effect
	if (target->fxqueue.CountEffects(eff_ref, fx->Parameter1, fx->Parameter2) == 1) {
		return;
	}

	int maxMalus = target->fxqueue.MaxParam1(eff_ref, false);
	int maxBonus = target->fxqueue.MaxParam1(eff_ref, true);
	if (bonus > 0 && bonus > maxBonus) {
		return;
	} else if (bonus < 0 && bonus < maxMalus) {
		return;
	}
	fx->Parameter1 = 0;
	fx->Parameter3 = bonus;
}

static inline void PlayRemoveEffect(const Actor *target, const Effect* fx, StringView defsound = StringView())
{
	core->GetAudioDrv()->Play(fx->Resource.IsEmpty() ? defsound : StringView(fx->Resource),
			SFX_CHAN_ACTIONS, target->Pos);
}

//resurrect code used in many places
static void Resurrect(const Scriptable *Owner, Actor *target, const Effect *fx, const Point &p)
{
	const Scriptable *caster = GetCasterObject();
	if (!caster) {
		caster = Owner;
	}
	const Map *area = caster->GetCurrentArea();

	if (area && target->GetCurrentArea()!=area) {
		MoveBetweenAreasCore(target, area->GetScriptRef(), p, fx->Parameter2, true);
	}
	target->Resurrect(p);
}


// handles the percentage damage spread over time by converting it to absolute damage
inline int HandlePercentageDamage(Effect* fx, const Actor* target) {
	int damage = 0;
	if (fx->Parameter2 == RPD_PERCENT && fx->FirstApply) {
		// distribute the damage to one second intervals
		int seconds = (fx->Duration - core->GetGame()->GameTime) / core->Time.defaultTicksPerSec;
		damage = target->GetStat(IE_MAXHITPOINTS) * fx->Parameter1 / 100;
		fx->Parameter1 = static_cast<ieDword>(damage / seconds);
	}
	return damage;
}
// Effect opcodes

// 0x00 ACVsDamageTypeModifier
// Known values for Parameter2 are:
// 0   All
// 1   Crushing
// 2   Missile
// 4   Piercing
// 8   Slashing
// 16  Base AC setting (sets the targets AC to the value specifiedg)
// since the implemented ad&d doesn't have separate stats for all, we use them as the generic bonus
int fx_ac_vs_damage_type_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_ac_vs_damage_type_modifier(%2d): AC Modif: %d ; Type: %d ; MinLevel: %d ; MaxLevel: %d", fx->Opcode, fx->Parameter1, fx->Parameter2,(int) fx->DiceSides,(int) fx->DiceThrown);

	if (fx->IsVariable) {
		//has a second weapon or shield, cannot deflect arrows
		int slot = target->inventory.GetShieldSlot();
		if (slot > 0 && !target->inventory.IsSlotEmpty(slot)) return FX_APPLIED;

		//has a twohanded weapon equipped
		slot = Inventory::GetWeaponSlot();
		if (slot > 0) {
			const CREItem* item = target->inventory.GetSlotItem(slot);
			if (item && item->Flags & IE_INV_ITEM_TWOHANDED) return FX_APPLIED;
		}
	}

	// it is a bitmask
	int type = fx->Parameter2;
	if (type == 0) { // generic bonus
		target->AC.HandleFxBonus(fx->Parameter1, fx->TimingMode==FX_DURATION_INSTANT_PERMANENT);
		return FX_PERMANENT;
	}

	//convert to signed so -1 doesn't turn to an astronomical number
	if (type == 16) { // natural AC
		if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
			if (target->AC.GetNatural() > (signed) fx->Parameter1) {
				target->AC.SetNatural(fx->Parameter1);
			}
		} else {
			if (target->AC.GetTotal() > (signed) fx->Parameter1) {
				// previously we were overriding the whole stat, but now we can be finegrained
				// and reuse the deflection bonus, since iwd2 has its own version of this effect
				target->AC.SetDeflectionBonus((signed) fx->Parameter1 - target->AC.GetNatural());
			}
		}
		return FX_INSERT;
	}

	//the original engine did work with the combination of these bits
	//but since it crashed, we are not bound to the same rules
	if (type & 1) {
		HandleBonus(target, IE_ACCRUSHINGMOD, fx->Parameter1, fx->TimingMode);
	}
	if (type & 2) {
		HandleBonus(target, IE_ACMISSILEMOD, fx->Parameter1, fx->TimingMode);
	}
	if (type & 4) {
		HandleBonus(target, IE_ACPIERCINGMOD, fx->Parameter1, fx->TimingMode);
	}
	if (type & 8) {
		HandleBonus(target, IE_ACSLASHINGMOD, fx->Parameter1, fx->TimingMode);
	}

	return FX_PERMANENT;
}

// 0x01 AttacksPerRoundModifier
int fx_attacks_per_round_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_attacks_per_round_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	int tmp = (signed) fx->Parameter1;
	if (fx->Parameter2 != MOD_PERCENT) {
		if (tmp>10) tmp=10;
		else if (tmp<-10) tmp=-10;
		tmp <<= 1;
		if (tmp>10) tmp-=11;
		else if (tmp<-10) tmp+=11;
	}

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD_VAR(IE_NUMBEROFATTACKS, tmp);
	} else {
		STAT_MOD_VAR(IE_NUMBEROFATTACKS, tmp);
	}
	return FX_PERMANENT;
}

// 0x02 Cure:Sleep (Awaken)
// this effect clears the STATE_SLEEP (1) bit, but clearing it alone wouldn't remove the

int fx_cure_sleep_state (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_cure_sleep_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	BASE_STATE_CURE( STATE_SLEEP );
	target->fxqueue.RemoveAllEffects(fx_set_sleep_state_ref);
	target->fxqueue.RemoveAllEffectsWithParam(fx_display_portrait_icon_ref, PI_SLEEP);
	target->SetStance(IE_ANI_GET_UP);
	return FX_NOT_APPLIED;
}

// 0x03 State:Berserk
// this effect sets the STATE_BERSERK bit, but bg2 actually ignores the bit
int fx_set_berserk_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_set_berserk_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	// atleast how and bg2 allow this to only work on pcs
	if (!core->HasFeature(GFFlags::RULES_3ED) && !target->InParty) {
		return FX_NOT_APPLIED;
	}

	if (fx->FirstApply) {
		target->inventory.EquipBestWeapon(EQUIP_MELEE);
	}

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_STATE_SET( STATE_BERSERK );
	} else {
		STATE_SET( STATE_BERSERK );
	}

	switch(fx->Parameter2) {
	case 1: //always berserk
		target->SetSpellState(SS_BERSERK);
		STAT_SET(IE_BERSERKSTAGE2, 1);
		// intentional fallthrough
	default:
		target->AddPortraitIcon(PI_BERSERK);
		break;
	case 2: //blood rage
		target->SetSpellState(SS_BERSERK);
		//immunity to effects:
		//5 charm
		//0x11 heal
		//0x18 panic
		//0x27 sleep
		//0x2d stun
		//0x6d hold
		//0x80 confusion
		//400 hopelessness
		//
		target->SetSpellState(SS_BLOODRAGE);
		target->SetSpellState(SS_NOHPINFO);
		target->SetColorMod(0xff, RGBModifier::ADD, 15, Color(128, 0, 0, 0));
		target->AddPortraitIcon(PI_BLOODRAGE);
		break;
	}
	return FX_PERMANENT;
}

// 0x04 Cure:Berserk
// this effect clears the STATE_BERSERK (2) bit, but bg2 actually ignores the bit
// it also removes effect 04

int fx_cure_berserk_state (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_cure_berserk_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	BASE_STATE_CURE( STATE_BERSERK );
	target->fxqueue.RemoveAllEffects(fx_set_berserk_state_ref);
	return FX_NOT_APPLIED;
}

// 0x05 State:Charmed
// 0xf1 ControlCreature (iwd2)
int fx_set_charmed_state (Scriptable* Owner, Actor* target, Effect* fx)
{
	// print("fx_set_charmed_state(%2d): General: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

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

	if (target->GetStat(IE_EXTSTATE_ID) & EXTSTATE_EYE_MIND) {
		target->fxqueue.RemoveAllEffects(fx_eye_mind_ref);
		target->spellbook.RemoveSpell(SevenEyes[EYE_MIND]);
		target->SetBaseBit(IE_EXTSTATE_ID, EXTSTATE_EYE_MIND, false);
		return FX_ABORT;
	}

	// if there are several effects on the queue, suppress all but the newest
	unsigned int count = target->fxqueue.CountEffects(fx_set_charmed_state_ref, -1, -1);
	if (count > 1 && target->fxqueue.GetEffectOrder(fx_set_charmed_state_ref, fx) < count) {
		return FX_PERMANENT;
	}

	bool playercharmed;
	bool casterenemy;
	if (fx->FirstApply) {
		//when charmed, the target forgets its current action
		target->ClearActions();

		Scriptable *caster = GetCasterObject();
		if (!caster) caster = Owner;
		const Actor* casterActor = Scriptable::As<const Actor>(caster);
		if (casterActor) {
			casterenemy = casterActor->GetStat(IE_EA) > EA_GOODCUTOFF; //or evilcutoff?
		} else {
			casterenemy = true;
		}
		if (!fx->DiceThrown) fx->DiceThrown = casterenemy;

		playercharmed = target->InParty;
		fx->DiceSides = playercharmed;
	} else {
		casterenemy = fx->DiceThrown;
		playercharmed = fx->DiceSides;
	}


	switch (fx->Parameter2) {
	case 0: //charmed (target neutral after charm)
		if (fx->FirstApply) {
			displaymsg->DisplayConstantStringName(HCStrings::Charmed, GUIColors::WHITE, target);
		}
		// intentional fallthrough
	case 1000:
		break;
	case 1: //charmed (target hostile after charm)
		if (fx->FirstApply) {
			displaymsg->DisplayConstantStringName(HCStrings::Charmed, GUIColors::WHITE, target);
		}
		// intentional fallthrough
	case 1001:
		if (!target->InParty) {
			target->SetBaseNoPCF(IE_EA, EA_ENEMY);
		}
		break;
	case 2: //dire charmed (target neutral after charm)
		if (fx->FirstApply) {
			displaymsg->DisplayConstantStringName(HCStrings::DireCharmed, GUIColors::WHITE, target);
		}
		// intentional fallthrough
	case 1002:
		break;
	case 3: //dire charmed (target hostile after charm)
		if (fx->FirstApply) {
			displaymsg->DisplayConstantStringName(HCStrings::DireCharmed, GUIColors::WHITE, target);
		}
		// intentional fallthrough
	case 1003:
		if (!target->InParty) {
			target->SetBaseNoPCF(IE_EA, EA_ENEMY);
		}
		break;
	case 4: //controlled by cleric
		if (fx->FirstApply) {
			displaymsg->DisplayConstantStringName(HCStrings::Controlled, GUIColors::WHITE, target);
		}
		// intentional fallthrough
	case 1004:
		if (!target->InParty) {
			target->SetBaseNoPCF(IE_EA, EA_ENEMY);
		}
		target->SetSpellState(SS_DOMINATION);
		break;
	case 5: //thrall (typo comes from original engine doc)
		if (fx->FirstApply) {
			displaymsg->DisplayConstantStringName(HCStrings::Charmed, GUIColors::WHITE, target);
		}
		// intentional fallthrough
	case 1005:
		STAT_SET(IE_EA, EA_ENEMY );
		STAT_SET(IE_THRULLCHARM, 1);
		return FX_PERMANENT;
	}

	STATE_SET( STATE_CHARMED );
	if (playercharmed) {
		STAT_SET_PCF( IE_EA, casterenemy?EA_CHARMEDPC:EA_CHARMED );
	} else {
		STAT_SET_PCF( IE_EA, casterenemy?EA_ENEMY:EA_CHARMED );
	}
	//don't stick if permanent
	return FX_PERMANENT;
}

// 0x06 CharismaModifier
int fx_charisma_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_charisma_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	// ensure we don't stack in pst
	if (core->HasFeature(GFFlags::PST_STATE_FLAGS)) {
		// don't be cumulative — luckily the opcode is the first on the effect list of spwi114
		// we remove the old spell, so the player benefits from renewed duration
		ResRef tmp = fx->SourceRef;
		fx->SourceRef = "";
		target->fxqueue.RemoveAllEffects(tmp);
		fx->SourceRef = tmp;
	}

	// in pst (only) this is a diced effect (eg. Friends)
	if (fx->FirstApply == 1 && fx->Parameter1 == 0 && fx->Parameter2 == 0) {
		fx->Parameter1 = DICE_ROLL(0);
	}

	HandleMainStatBonus(target, IE_CHR, fx);

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD( IE_CHR );
	} else {
		STAT_MOD( IE_CHR );
	}
	return FX_PERMANENT;
}

// 0x07 Color:SetPalette
// this effect might not work in pst, they don't have separate weapon slots
int fx_set_color_gradient (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_set_color_gradient(%2d): Gradient: %d, Location: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	EffectQueue::HackColorEffects(target, fx);
	target->SetColor( fx->Parameter2, fx->Parameter1 );
	return FX_APPLIED;
}

// 08 Color:SetRGB
int fx_set_color_rgb (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_set_color_rgb(%2d): RGB: %x, Location: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	EffectQueue::HackColorEffects(target, fx);
	ieDword location = fx->Parameter2 & 0xff;
	target->SetColorMod(location, RGBModifier::ADD, -1, Color::FromBGRA(fx->Parameter1));

	return FX_APPLIED;
}
// 08 Color:SetRGBGlobal
int fx_set_color_rgb_global (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_set_color_rgb_global(%2d): RGB: %x, Location: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	target->SetColorMod(0xff, RGBModifier::ADD, -1, Color::FromBGRA(fx->Parameter1));

	return FX_APPLIED;
}

// 09 Color:PulseRGB
int fx_set_color_pulse_rgb (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_set_color_pulse_rgb(%2d): RGB: %x, Location: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	EffectQueue::HackColorEffects(target, fx);
	ieDword location = fx->Parameter2 & 0xff;
	int speed = (fx->Parameter2 >> 16) & 0xFF;
	target->SetColorMod(location, RGBModifier::ADD, speed, Color::FromBGRA(fx->Parameter1));

	return FX_APPLIED;
}

// 09 Color:PulseRGBGlobal (pst variant)
int fx_set_color_pulse_rgb_global (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_set_color_pulse_rgb_global(%2d): RGB: %x, Location: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	int speed = (fx->Parameter2 >> 16) & 0xFF;
	target->SetColorMod(0xff, RGBModifier::ADD, speed, Color::FromBGRA(fx->Parameter1));

	return FX_APPLIED;
}

// 0x0A ConstitutionModifier
int fx_constitution_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_constitution_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	HandleMainStatBonus(target, IE_CON, fx);

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD( IE_CON );
	} else {
		STAT_MOD( IE_CON );
	}
	return FX_PERMANENT;
}

// 0x0B Cure:Poison

int fx_cure_poisoned_state (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_cure_poisoned_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	//all three steps are present in bg2 and iwd2
	BASE_STATE_CURE( STATE_POISONED );
	target->fxqueue.RemoveAllEffects( fx_poisoned_state_ref );
	target->fxqueue.RemoveAllEffectsWithParam(fx_display_portrait_icon_ref, PI_POISONED);
	return FX_NOT_APPLIED;
}

// 0x0c Damage
// this is a very important effect
int fx_damage (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_damage(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	//save for half damage type
	ieDword damagetype = fx->Parameter2>>16;
	ieDword modtype = fx->Parameter2&3;
	if (modtype == 3) {
		// too late to set DamageFlags::SaveForHalf here
		modtype &= ~3;
	}
	Scriptable *caster = GetCasterObject();
	const Actor* source = Scriptable::As<Actor>(caster);

	// immediately returns without doing anything if the source doesn't have SLOT_FIST selected
	// it stays on the creature if the timing mode allows, waiting for SLOT_FIST to be selected
	// if the effect is waiting for SLOT_FIST, all effects after the op12 in the list fail to
	// be evaluated, only (potentially) being evaluated a single time on being added to the actor
	// NOTE: add a soft FX_ABORT-like return type if this queueing is really needed
	if (fx->IsVariable & DamageFlags::FistOnly && source && source->inventory.IsSlotEmpty(Inventory::GetFistSlot())) {
		return FX_ABORT;
	}

	// gemrb extension
	if (fx->Parameter3) {
		if(caster && caster->Type==ST_ACTOR) {
			target->AddTrigger(TriggerEntry(trigger_hitby, caster->GetGlobalID()));
			target->objects.LastHitter = caster->GetGlobalID();
		} else {
			//Maybe it should be something impossible like 0xffff, and use 'Someone'
			Log(ERROR, "Actor", "LastHitter (type {}) falling back to target: {}.", caster ? caster->Type : -1, fmt::WideToChar{target->GetName()});
			target->objects.LastHitter = target->GetGlobalID();
		}
	}

	if (core->HasFeature(GFFlags::RULES_3ED) && target->GetStat(IE_MC_FLAGS) & MC_INVULNERABLE) {
		Log(DEBUG, "fx_damage", "Attacking invulnerable target, skipping!");
		return FX_NOT_APPLIED;
	}

	target->Damage(fx->Parameter1, damagetype, caster, modtype, fx->IsVariable, fx->SavingThrowType, fx->IsVariable);
	//this effect doesn't stick
	return FX_NOT_APPLIED;
}

// 0x0d Death

int fx_death (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_death(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (target->GetStat(IE_EXTSTATE_ID) & EXTSTATE_EYE_SPIRIT) {
		target->fxqueue.RemoveAllEffects(fx_eye_spirit_ref);
		target->spellbook.RemoveSpell(SevenEyes[EYE_SPIRIT]);
		target->SetBaseBit(IE_EXTSTATE_ID, EXTSTATE_EYE_SPIRIT, false);
		return FX_ABORT;
	}

	//if the opcode of this effect is associated with Death2 (iwd2's death magic opcode) and
	//there is an active death ward effect, ignore this opcode
	if (target->fxqueue.HasEffect(fx_death_ward_ref) ) {
		//find the opcode for death magic (should be 420 in original IWD2)
		EffectQueue::ResolveEffect(fx_death_magic_ref);
		if (fx->Opcode==(ieDword) fx_death_magic_ref.opcode) return FX_NOT_APPLIED;
	}

	ieDword damagetype = 0;
	switch (fx->Parameter2) {
	case 1:
		if (!target->GetStat(IE_DISABLECHUNKING)) BASE_STATE_SET(STATE_FLAME); //not sure, should be charred
		damagetype = DAMAGE_FIRE;
		break;
	case 2:
		damagetype = DAMAGE_CRUSHING;
		break;
	case 4: // "normal" death
		damagetype = DAMAGE_CRUSHING;
		break;
	case 8:
		// Actor::CheckOnDeath handles the actual chunking
		damagetype = DAMAGE_CRUSHING|DAMAGE_CHUNKING;
		// bg1 & iwds have this file, bg2 & pst none
		core->GetAudioDrv()->Play("GORE", SFX_CHAN_HITS, target->Pos);
		break;
	case 16:
		BASE_STATE_SET(STATE_PETRIFIED);
		damagetype = DAMAGE_CRUSHING;
		break;
	case 32:
		BASE_STATE_SET(STATE_FROZEN);
		damagetype = DAMAGE_COLD;
		break;
	case 64:
		if (!target->GetStat(IE_DISABLECHUNKING)) BASE_STATE_SET(STATE_PETRIFIED);
		damagetype = DAMAGE_CRUSHING|DAMAGE_CHUNKING;
		// file only in iwds
		core->GetAudioDrv()->Play("GORE2", SFX_CHAN_HITS, target->Pos);
		break;
	case 128:
		if (!target->GetStat(IE_DISABLECHUNKING)) BASE_STATE_SET(STATE_FROZEN);
		damagetype = DAMAGE_COLD|DAMAGE_CHUNKING;
		core->GetAudioDrv()->Play("GORE2", SFX_CHAN_HITS, target->Pos);
		break;
	case 256:
		damagetype = DAMAGE_ELECTRICITY;
		break;
	case 512: //disintegration
		damagetype = DAMAGE_DISINTEGRATE;
		break;
	case 1024: // destruction, iwd2: maybe internally used for smiting and/or disruption (separate opcodes — see IWDOpcodes)
	default:
		damagetype = DAMAGE_ACID;
	}

	if (target->GetStat(IE_DISABLECHUNKING)) {
		damagetype &= ~DAMAGE_CHUNKING;
	}

	if (damagetype!=DAMAGE_COLD) {
		//these two bits are turned off on death
		BASE_STATE_CURE(STATE_FROZEN|STATE_PETRIFIED);
	}
	if (target->ShouldModifyMorale()) {
		BASE_SET(IE_MORALE, 10);
	}

	// don't give xp in cutscenes
	bool giveXP = true;
	if (core->InCutSceneMode()) {
		giveXP = false;
	}

	Scriptable *killer = GetCasterObject();
	target->Damage(0, damagetype, killer);
	//death has damage type too
	target->Die(killer, giveXP);
	//this effect doesn't stick
	return FX_NOT_APPLIED;
}

// 0xE Cure:Defrost
int fx_cure_frozen_state (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_cure_frozen_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	if (STATE_GET(STATE_FROZEN) && (STAT_GET(IE_HITPOINTS)<1) ) {
		BASE_SET(IE_HITPOINTS,1);
	}
	BASE_STATE_CURE( STATE_FROZEN );
	return FX_NOT_APPLIED;
}

// 0x0F DexterityModifier
int fx_dexterity_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_dexterity_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	////how cat's grace: value is based on class
	if (fx->Parameter2==3) {
		fx->Parameter1 = core->Roll(1, gamedata->GetSpellAbilityDie(target, 0), 0);
		fx->Parameter2 = 0;
	}

	HandleMainStatBonus(target, IE_DEX, fx);

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD( IE_DEX );
	} else {
		STAT_MOD( IE_DEX );
	}
	return FX_PERMANENT;
}

// 0x10 State:Hasted
// this function removes slowed state, or sets hasted state
int fx_set_hasted_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_set_hasted_state(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	target->fxqueue.RemoveAllEffects(fx_set_slow_state_ref);
	target->fxqueue.RemoveAllEffectsWithParam( fx_display_portrait_icon_ref, PI_SLOWED );
	int old_type = -2;
	int new_type = -2; // lowest priority by default
	if (target->GetStat(IE_STATE_ID) & STATE_HASTED) {
		old_type = (signed) target->GetStat(IE_IMPROVEDHASTE); // we cramp all three types into this stat now
	}
	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_STATE_CURE( STATE_SLOWED );
		BASE_STATE_SET( STATE_HASTED );
	} else {
		STATE_CURE( STATE_SLOWED );
		STATE_SET( STATE_HASTED );
	}
	target->NewStat(IE_MOVEMENTRATE, 200, MOD_PERCENT);
	// note that the number of attacks is handled separately, based on below stats, after effects are applied
	switch (fx->Parameter2) {
	case 0: //normal haste
		target->AddPortraitIcon(PI_HASTED);
		new_type = 0;
		// -2 initiative bonus
		STAT_ADD(IE_PHYSICALSPEED, 2);
		break;
	case 1://improved haste
		target->AddPortraitIcon(PI_IMPROVEDHASTE);
		new_type = 1;
		// -2 initiative bonus
		STAT_ADD(IE_PHYSICALSPEED, 2);
		break;
	case 2://speed haste only
		target->AddPortraitIcon(PI_HASTED);
		new_type = -1; // SCS detects imp. haste by checking if stat is > 0, so this correctly fails
		break;
	}
	if (new_type > old_type) { // improved > normal > weak > none
		STAT_SET(IE_IMPROVEDHASTE, new_type);
	}
	return FX_PERMANENT;
}

// 0x11 CurrentHPModifier
static int GetSpecialHealAmount(int type, const Scriptable *caster)
{
	if (!caster) return 0;
	const Actor* actor = caster->As<const Actor>();

	switch(type) {
		case 3: //paladin's lay on hands, the amount is already calculated in a compatible way
			return actor->GetSafeStat(IE_LAYONHANDSAMOUNT);
		case 4: //monk wholeness of body
			return actor->GetSafeStat(IE_LEVELMONK)*2; //ignore level 7 restriction, since the spell won't be granted under level 7
		case 5: //lathander's renewal
			return actor->GetSafeStat(IE_LEVELCLERIC)*2; //ignore kit restriction, since the spell won't be granted to non lathander
		default:
			return 0;
	}
}

int fx_current_hp_modifier (Scriptable* Owner, Actor* target, Effect* fx)
{
	// print("fx_current_hp_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (fx->Parameter2&0x10000) {
		Resurrect(Owner, target, fx, fx->Pos);
	}
	if (fx->Parameter2&0x20000) {
		target->fxqueue.RemoveAllNonPermanentEffects();
	}
	//Cannot heal bloodrage
	if (target->HasSpellState(SS_BLOODRAGE)) {
		return FX_NOT_APPLIED;
	}

	//current hp percent is relative to modified max hp
	int type = fx->Parameter2&0xffff;
	switch(type) {
	case MOD_ADDITIVE:
	case MOD_ABSOLUTE:
		target->NewBase( IE_HITPOINTS, fx->Parameter1, type);
		break;
	case MOD_PERCENT:
		target->NewBase( IE_HITPOINTS, target->GetSafeStat(IE_MAXHITPOINTS)*fx->Parameter1/100, MOD_ABSOLUTE);
		break;
	default: // lay on hands amount, wholeness of body, lathander's renewal
		target->NewBase( IE_HITPOINTS, GetSpecialHealAmount(type, GetCasterObject()), MOD_ADDITIVE);
		break;
	}
	//never stay permanent
	return FX_NOT_APPLIED;
}

// 0x12 MaximumHPModifier
// 0 and 3 differ in that 3 doesn't modify current hitpoints
// 1,4 and 2,5 are analogous to them, but with different modifiers
int fx_maximum_hp_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_maximum_hp_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	//state_exploding is different in PST, probably not needed anyway
	if (STATE_GET(STATE_DEAD|STATE_PETRIFIED|STATE_FROZEN|STATE_ACID|STATE_FLAME) ) {
		return FX_NOT_APPLIED;
	}

	if (BASE_GET(IE_HITPOINTS)<=0 ) {
		return FX_NOT_APPLIED;
	}

	bool base = fx->TimingMode==FX_DURATION_INSTANT_PERMANENT;

	switch (fx->Parameter2) {
	case 0:
		// random value Parameter1 is set by level_check in EffectQueue
		if (base) {
			BASE_ADD( IE_MAXHITPOINTS, fx->Parameter1 );
			BASE_ADD( IE_HITPOINTS, fx->Parameter1 );
		} else {
			STAT_ADD( IE_MAXHITPOINTS, fx->Parameter1 );
			if (fx->FirstApply) {
				BASE_ADD( IE_HITPOINTS, fx->Parameter1 );
			}
		}
		break;
	case 3: // no current hp bonus
		// random value Parameter1 is set by level_check in EffectQueue
		if (base) {
			BASE_ADD( IE_MAXHITPOINTS, fx->Parameter1 );
		} else {
			STAT_ADD( IE_MAXHITPOINTS, fx->Parameter1 );
		}
		break;
	case 1: // with current hp bonus, but unimplemented in the original
	case 4:
		if (base) {
			BASE_SET( IE_MAXHITPOINTS, fx->Parameter1 );
		} else {
			STAT_SET( IE_MAXHITPOINTS, fx->Parameter1 );
		}
		break;
	case 2:
		if (base) {
			BASE_MUL(IE_MAXHITPOINTS, fx->Parameter1 );
			BASE_MUL(IE_HITPOINTS, fx->Parameter1 );
		} else {
			target->NewStat( IE_MAXHITPOINTS, target->GetStat(IE_MAXHITPOINTS)*fx->Parameter1/100, MOD_ABSOLUTE);
			if (fx->FirstApply) {
				target->NewBase( IE_HITPOINTS, target->GetSafeStat(IE_HITPOINTS)*fx->Parameter1/100, MOD_ABSOLUTE);
			}
		}
		break;
	case 5: // no current hp bonus
		if (base) {
			BASE_MUL( IE_MAXHITPOINTS, fx->Parameter1 );
		} else {
			STAT_MUL( IE_MAXHITPOINTS, fx->Parameter1 );
		}
		break;
	}
	return FX_PERMANENT;
}

// 0x13 IntelligenceModifier
int fx_intelligence_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_intelligence_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	HandleMainStatBonus(target, IE_INT, fx);

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD( IE_INT );
	} else {
		STAT_MOD( IE_INT );
	}
	return FX_PERMANENT;
}

// 0x14 State:Invisible
// this is more complex, there is a half-invisibility state
// and there is a hidden state
int fx_set_invisible_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	switch (fx->Parameter2) {
	case 1:
			STATE_SET(STATE_INVIS2); // TODO: actually use STATE_INVIS2 in code
			// intentional fallthrough
	case 0:
		if (core->HasFeature(GFFlags::PST_STATE_FLAGS)) {
			STATE_SET( STATE_PST_INVIS );
		} else {
			STATE_SET( STATE_INVISIBLE );
		}
		if (fx->FirstApply || fx->TimingMode != FX_DURATION_INSTANT_PERMANENT) {
			target->ToHit.HandleFxBonus(4, fx->TimingMode==FX_DURATION_INSTANT_PERMANENT);
		}
		break;
	case 2:// EE: weak invisibility, like improved after being revealed (no backstabbing)
		STATE_SET(STATE_INVIS2);
	default:
		break;
	}
	ieDword Trans = fx->Parameter4;
	if (fx->Parameter3) {
		if (Trans >= 240) {
			fx->Parameter3 = 0;
		} else {
			Trans += 4;
		}
	} else {
		if (Trans <= 160) {
			fx->Parameter3 = 1;
		} else {
			Trans -= 4;
		}
	}
	fx->Parameter4 = Trans;
	STAT_SET( IE_TRANSLUCENT, Trans);
	//FIXME: probably FX_PERMANENT, but TRANSLUCENT has no saved base stat
	// we work around it by only applying the tohit/ac bonus once
	return FX_APPLIED;
}

// 0x15 LoreModifier
int fx_lore_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_lore_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	ieDword mode = fx->Parameter2, value = fx->Parameter1;
	if (mode == 2) {
		//guaranteed identification
		mode = MOD_ABSOLUTE;
		value = 100;
	}
	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		target->NewBase(IE_LORE, value, mode);
	} else {
		target->NewStat(IE_LORE, value, mode);
	}
	return FX_PERMANENT;
}

// 0x16 LuckModifier
int fx_luck_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_luck_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	// in pst (only) this is a diced effect (eg. Luck)
	if (fx->FirstApply == 1 && fx->Parameter1 == 0 && fx->Parameter2 == 0) {
		fx->Parameter1 = DICE_ROLL(0);
	}
	// iwd2 supposedly only supported MOD_ADDITIVE, with 1 being Lucky Streak and 2 Fortunes Favorite (perfect rolls)
	// there are no users of that, so we don't bother

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD( IE_LUCK );
	} else {
		STAT_MOD( IE_LUCK );
		//I checked, it doesn't modify damage luck stat in bg2 (Avenger)
		//STAT_MOD( IE_DAMAGELUCK );
	}
	return FX_PERMANENT;
}

// 0x17 MoraleModifier
int fx_morale_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_morale_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (STATE_GET(STATE_BERSERK)) {
		return FX_NOT_APPLIED;
	}

	// EEs toggle this with fx->Special, but 0 stands for bg2, so we can't use it
	// they will just use the same game flag instead
	if (core->HasFeature(GFFlags::FIXED_MORALE_OPCODE)) {
		BASE_SET(IE_MORALE, 10);
		return FX_NOT_APPLIED;
	}

	if (target->ShouldModifyMorale()) {
		STAT_MOD(IE_MORALE);
	}
	return FX_APPLIED;
}

// 0x18 State:Panic / State: Horror
// TODO: Non-zero param2 -> Bypass opcode #101 (Immunity to effect); iwd thing, but no users
int fx_set_panic_state(Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_set_panic_state(%2d)", fx->Opcode);

	if (target->HasSpellState(SS_BLOODRAGE)) {
		return FX_NOT_APPLIED;
	}

	if (target->GetStat(IE_EXTSTATE_ID) & EXTSTATE_EYE_MIND) {
		target->fxqueue.RemoveAllEffects(fx_eye_mind_ref);
		target->spellbook.RemoveSpell(SevenEyes[EYE_MIND]);
		target->SetBaseBit(IE_EXTSTATE_ID, EXTSTATE_EYE_MIND, false);
		return FX_ABORT;
	}

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_STATE_SET( STATE_PANIC );
	} else {
		STATE_SET( STATE_PANIC );
	}

	// run away and reevaluate every round, so expiry can take effect
	// a simplified Actor::Panic that does too much and too little for reuse here
	if (fx->FirstApply || target->Ticks % core->Time.round_size == 0) {
		if (target->InParty) core->GetGame()->SelectActor(target, false, SELECT_NORMAL);
		target->VerbalConstant(Verbal::Panic, gamedata->GetVBData("SPECIAL_COUNT"));

		Action* action;
		const Actor* caster = GetCasterObject();
		if (caster) {
			if (core->HasFeature(GFFlags::IWD_MAP_DIMENSIONS)) { // iwd troll scripts are incompatible with full panic
				action = GenerateActionDirect("RunAwayFrom([-1],300)", caster);
			} else {
				action = GenerateActionDirect("RunAwayFromNoInterrupt([-1],300)", caster);
			}
		} else {
			action = GenerateAction("RandomWalk()");
		}
		assert(action);
		action->int0Parameter = core->Time.round_size;
		action->int2Parameter = 1; // mark as our own
		const Action* current = target->GetCurrentAction();
		if (current && current->int2Parameter == 1) target->ReleaseCurrentAction();
		target->AddActionInFront(action);
	}
	if (core->HasFeature(GFFlags::ENHANCED_EFFECTS)) {
		target->AddPortraitIcon(PI_PANIC);
	}
	target->SetCircleSize();
	return FX_PERMANENT;
}

// 0x19 State:Poisoned
int fx_set_poisoned_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_set_poisoned_state(%2d): Damage: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	if (STATE_GET(STATE_DEAD) ) {
		return FX_NOT_APPLIED;
	}

	if (target->GetStat(IE_EXTSTATE_ID) & EXTSTATE_EYE_VENOM) {
		target->fxqueue.RemoveAllEffects(fx_eye_venom_ref);
		target->spellbook.RemoveSpell(SevenEyes[EYE_VENOM]);
		target->SetBaseBit(IE_EXTSTATE_ID, EXTSTATE_EYE_VENOM, false);
		return FX_ABORT;
	}

	// don't stack, only run one of the same at a time
	ieDword count = target->fxqueue.CountEffects(fx_poisoned_state_ref, fx->Parameter1, fx->Parameter2, fx->Resource);
	if (count > 1 && target->fxqueue.GetEffectOrder(fx_poisoned_state_ref, fx) < count) {
		return FX_APPLIED;
	}

	STATE_SET( STATE_POISONED );
	if (fx->IsVariable) {
		target->AddPortraitIcon(static_cast<ieByte>(fx->IsVariable));
	} else {
		target->AddPortraitIcon(PI_POISONED);
	}

	ieDword damage = 0;
	tick_t tmp = fx->Parameter1;
	// fx->Parameter4 is an optional frequency multiplier
	ieDword aRound = (fx->Parameter4 ? fx->Parameter4 : 1) * core->Time.defaultTicksPerSec;
	tick_t timeStep = target->GetAdjustedTime(aRound);

	int totalDamage = HandlePercentageDamage(fx, target);
	// ensure we deal at least 1 point of damage, but shorten the duration when rounding up
	if (fx->Parameter2 == RPD_PERCENT) {
		if (fx->FirstApply) {
			fx->Parameter5 = totalDamage;
			if (fx->Parameter1 == 0) fx->Parameter1++;
		} else if (core->GetGame()->GameTime % timeStep == 0) { // only if we're dealing damage in this run
			if (signed(fx->Parameter5) <= 0) return FX_ABORT;
			fx->Parameter5 -= fx->Parameter1;
		}
	}

	Scriptable *caster = GetCasterObject();

	switch(fx->Parameter2) {
	case RPD_ROUNDS:
		tmp = core->Time.round_sec;
		damage = core->HasFeature(GFFlags::HAS_EE_EFFECTS) ? fx->Parameter3 : fx->Parameter1;
		break;
	case RPD_TURNS:
		tmp = core->Time.turn_sec;
		damage = fx->Parameter1;
		break;
	case RPD_SECONDS:
		tmp *= core->Time.round_sec;
		damage = 1;
		break;
	case RPD_PERCENT: // handled in HandlePercentageDamage
	case RPD_POINTS:
		tmp = 1; // hit points per second
		damage = fx->Parameter1;
		break;
	case RPD_SNAKE: //iwd2 snakebite (a poison causing paralysis)
		STAT_SET(IE_HELD, 1);
		target->AddPortraitIcon(PI_HELD);
		target->SetSpellState(SS_HELD);
		STATE_SET(STATE_HELPLESS);
		if (fx->FirstApply) {
			displaymsg->DisplayConstantStringName(HCStrings::StateHeld, GUIColors::WHITE, target);
		}
		break;
	case RPD_7:
		damage = fx->Parameter1;
		tmp = fx->Parameter3;
		break;
	case RPD_ENVENOM:
		//call the constitution effect like it was this one (don't add it to the effect queue)
		//can't simply convert this effect to the constitution effect, because a poison effect
		//could be removed and there is a portrait icon
		Effect *newfx;
		newfx = EffectQueue::CreateEffectCopy(fx, fx_constitution_modifier_ref, fx->Parameter1, 0);
		target->fxqueue.ApplyEffect(target, newfx, fx->FirstApply, 0);
		delete newfx;
		damage = 0;
		tmp = 1;
		break;
	default:
		tmp = 1;
		damage = 1;
		break;
	}

	// all damage is at most per-second
	tmp *= timeStep;
	if (tmp && (core->GetGame()->GameTime%tmp)) {
		return FX_APPLIED;
	}

	if (damage) {
		target->Damage(damage, DAMAGE_POISON, caster);
	}
	return FX_APPLIED;
}

// 0x1a RemoveCurse

// gemrb extension: if the resource field is filled, it will remove curse only from the specified item
int fx_remove_curse (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (fx->Parameter2 == 1) {
		// this is pst specific, other games don't use parameters
		target->fxqueue.RemoveAllEffects(fx_pst_jumble_curse_ref);
		return FX_NOT_APPLIED;
	}

	Inventory* inv = &target->inventory;
	int i = target->inventory.GetSlotCount();
	while (i--) {
		// does this slot need unequipping
		if (!core->QuerySlotEffects(i)) continue;
		if (!fx->Resource.IsEmpty() && inv->GetSlotItem(i)->ItemResRef != fx->Resource) {
			continue;
		}
		if (!(inv->GetItemFlag(i) & IE_INV_ITEM_CURSED)) {
			continue;
		}

		if (inv->UnEquipItem(i, true)) {
			CREItem* tmp = inv->RemoveItem(i);
			if (inv->AddSlotItem(tmp, -3) != ASI_SUCCESS) {
				// if the item couldn't be placed in the inventory, then put it back to the original slot
				inv->SetSlotItem(tmp, i);
				// and drop it in the area. (If there is no area, then the item will stay in the inventory)
				target->DropItem(i, 0);
			}
		}
	}
	target->fxqueue.RemoveAllEffects(fx_apply_effect_curse_ref);

	//this is an instant effect
	return FX_NOT_APPLIED;
}

// 0x1b AcidResistanceModifier
int fx_acid_resistance_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_acid_resistance_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD( IE_RESISTACID );
	} else {
		STAT_MOD( IE_RESISTACID );
	}
	return FX_PERMANENT;
}

// 0x1c ColdResistanceModifier
int fx_cold_resistance_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_cold_resistance_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD( IE_RESISTCOLD );
	} else {
		STAT_MOD( IE_RESISTCOLD );
	}
	return FX_PERMANENT;
}

// 0x1d ElectricityResistanceModifier
int fx_electricity_resistance_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_electricity_resistance_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD( IE_RESISTELECTRICITY );
	} else {
		STAT_MOD( IE_RESISTELECTRICITY );
	}
	return FX_PERMANENT;
}

// 0x1e FireResistanceModifier
int fx_fire_resistance_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_fire_resistance_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD( IE_RESISTFIRE );
	} else {
		STAT_MOD( IE_RESISTFIRE );
	}
	return FX_PERMANENT;
}

// 0x1f MagicDamageResistanceModifier
int fx_magic_damage_resistance_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_magic_damage_resistance_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	//This stat has no saved basestat variant, so this effect is always stored (not FX_PERMANENT)
	STAT_MOD( IE_MAGICDAMAGERESISTANCE );
	return FX_APPLIED;
}

// 0x20 Cure:Death
int fx_cure_dead_state (Scriptable* Owner, Actor* target, Effect* fx)
{
	// print("fx_cure_dead_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	//call this only if the target is dead, otherwise some variables can get wrong
	if (STATE_GET(STATE_DEAD) ) {
		Resurrect(Owner, target, fx, fx->Pos);
	}
	return FX_NOT_APPLIED;
}

// 0x21 SaveVsDeathModifier
int fx_save_vs_death_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_save_vs_death_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	HandleBonus( target, IE_SAVEVSDEATH, fx->Parameter1, fx->TimingMode );
	return FX_PERMANENT;
}

// 0x22 SaveVsWandsModifier
int fx_save_vs_wands_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_save_vs_wands_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	HandleBonus( target, IE_SAVEVSWANDS, fx->Parameter1, fx->TimingMode );
	return FX_PERMANENT;
}

// 0x23 SaveVsPolyModifier
int fx_save_vs_poly_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_save_vs_poly_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	HandleBonus( target, IE_SAVEVSPOLY, fx->Parameter1, fx->TimingMode );
	return FX_PERMANENT;
}

// 0x24 SaveVsBreathModifier
int fx_save_vs_breath_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_save_vs_breath_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	HandleBonus( target, IE_SAVEVSBREATH, fx->Parameter1, fx->TimingMode );
	return FX_PERMANENT;
}

// 0x25 SaveVsSpellsModifier
int fx_save_vs_spell_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_save_vs_spell_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	HandleBonus( target, IE_SAVEVSSPELL, fx->Parameter1, fx->TimingMode );
	return FX_PERMANENT;
}

// 0x26 State:Silenced
int fx_set_silenced_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_set_silenced_state(%2d)", fx->Opcode);

	if (target->GetStat(IE_EXTSTATE_ID) & EXTSTATE_EYE_FORT) {
		target->fxqueue.RemoveAllEffects(fx_eye_fortitude_ref);
		target->spellbook.RemoveSpell(SevenEyes[EYE_FORT]);
		target->SetBaseBit(IE_EXTSTATE_ID, EXTSTATE_EYE_FORT, false);
		return FX_ABORT;
	}

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_STATE_SET( STATE_SILENCED );
	} else {
		STATE_SET( STATE_SILENCED );
	}
	return FX_PERMANENT;
}

// 0x27 State:Helpless, State:Sleep
// this effect sets both bits, but 'awaken' only removes the sleep bit
int fx_set_unconscious_state (Scriptable* Owner, Actor* target, Effect* fx)
{
	if (target->HasSpellState(SS_BLOODRAGE)) {
		return FX_NOT_APPLIED;
	}

	if (fx->FirstApply) {
		target->ApplyEffectCopy(fx, fx_animation_stance_ref, Owner, 0, IE_ANI_SLEEP);
		Effect* standUp = EffectQueue::CreateEffect(fx_animation_stance_ref, 0, IE_ANI_GET_UP, FX_DURATION_DELAY_LIMITED);
		standUp->Duration = (fx->Duration - core->GetGame()->GameTime) / core->Time.defaultTicksPerSec;
		core->ApplyEffect(standUp, target, target);
	}

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_STATE_SET( STATE_HELPLESS | STATE_SLEEP ); //don't awaken on damage
	} else {
		STATE_SET( STATE_HELPLESS | STATE_SLEEP ); //don't awaken on damage
		if (fx->Parameter2 || !core->HasFeature(GFFlags::HAS_EE_EFFECTS)) {
			target->SetSpellState(SS_NOAWAKE);
		}
		if (fx->IsVariable) {
			target->SetSpellState(SS_PRONE);
		}
		// else make the creature untargettable (backlisted); an original hack to avoid stunning damage
		// knockout then death by some other (eg. script targeting or call lightning)

		target->AddPortraitIcon(PI_SLEEP);
	}
	target->InterruptCasting = true;
	return FX_PERMANENT;
}

// 0x28 State:Slowed
// this function removes hasted state, or sets slowed state

int fx_set_slowed_state (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_set_slowed_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	//iwd2 free action or aegis disables this effect
	if (target->HasSpellState(SS_FREEACTION)) return FX_NOT_APPLIED;
	if (target->HasSpellState(SS_AEGIS)) return FX_NOT_APPLIED;

	if (STATE_GET(STATE_HASTED) ) {
		BASE_STATE_CURE( STATE_HASTED );
		target->fxqueue.RemoveAllEffects( fx_set_haste_state_ref );
		target->fxqueue.RemoveAllEffectsWithParam( fx_display_portrait_icon_ref, PI_HASTED );
	} else if (STATE_GET(STATE_SLOWED)) {
		// already slowed
		return FX_NOT_APPLIED;
	} else {
		STATE_SET( STATE_SLOWED );
		target->AddPortraitIcon(PI_SLOWED);
		// halve apr and speed
		STAT_MUL(IE_NUMBEROFATTACKS, 50);
		STAT_MUL(IE_MOVEMENTRATE, 50);
		STAT_SUB(IE_MENTALSPEED, 2);
	}
	return FX_PERMANENT;
}

// 0x29 Sparkle
int fx_sparkle (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_sparkle(%2d): Sparkle colour: %d ; Sparkle type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	if (!target) {
		return FX_NOT_APPLIED;
	}

	Map *map = target->GetCurrentArea();
	if (!map) {
		return FX_APPLIED;
	}
	// TODO: ee, pstee (if not pst) can use fx->Resource for bitmap mode
	map->Sparkle( fx->Duration, fx->Parameter1, fx->Parameter2, fx->Pos, fx->Parameter3);
	return FX_NOT_APPLIED;
}

// 0x2A WizardSpellSlotsModifier
int fx_bonus_wizard_spells (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_bonus_wizard_spells(%2d): Spell Add: %d ; Spell Level: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	int i=1;
	//if param2 is 0, then double spells up to param1
	if(!fx->Parameter2) {
		for (unsigned int j=0;j<fx->Parameter1 && j<MAX_SPELL_LEVEL;j++) {
			target->spellbook.SetMemorizableSpellsCount(0, IE_SPELL_TYPE_WIZARD, j, true);
		}
		return FX_APPLIED;
	}
	//HoW specific
	//if param2 is 0x200, then double spells at param1
	if (fx->Parameter2==0x200) {
		unsigned int j = fx->Parameter1-1;
		if (j<MAX_SPELL_LEVEL) {
			target->spellbook.SetMemorizableSpellsCount(0, IE_SPELL_TYPE_WIZARD, j, true);
		}
	}

	for(unsigned int j=0;j<MAX_SPELL_LEVEL;j++) {
		if (fx->Parameter2&i) {
			target->spellbook.SetMemorizableSpellsCount(fx->Parameter1, IE_SPELL_TYPE_WIZARD, j, true);
		}
		i<<=1;
	}
	return FX_APPLIED;
}

// 0x2B Cure:Petrification
int fx_cure_petrified_state (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_cure_petrified_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	BASE_STATE_CURE( STATE_PETRIFIED );
	return FX_NOT_APPLIED;
}

// 0x2C StrengthModifier
int fx_strength_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_strength_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	////how strength: value is based on class
	////pst power of one also depends on this!
	if (fx->Parameter2==3) {
		fx->Parameter1 = core->Roll(1, gamedata->GetSpellAbilityDie(target, 1), 0);
		fx->Parameter2 = MOD_ADDITIVE;
	}
	if (fx->Parameter2 == 3 && core->HasFeature(GFFlags::IWD_MAP_DIMENSIONS)) {
		// how had a max of 18/00, so clamp
		if (fx->TimingMode == FX_DURATION_INSTANT_PERMANENT) {
			if (target->GetBase(IE_STR) + fx->Parameter1 > 18) fx->Parameter1 = 18 - target->GetBase(IE_STR);
		} else {
			if (target->GetStat(IE_STR) + fx->Parameter1 > 18) fx->Parameter1 = 18 - target->GetStat(IE_STR);
		}
}

	HandleMainStatBonus(target, IE_STR, fx);

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD( IE_STR );
	} else {
		STAT_MOD( IE_STR );
	}

	// handle IE_STREXTRA (how, tobex, likely pst)
	if (fx->Parameter2 == 3 && target->GetStat(IE_STR) == 18) {
		int maxStrExtra = Clamp(gamedata->GetSpellAbilityDie(target, 2), 0, 100);
		target->SetStat(IE_STREXTRA, maxStrExtra, 0);
	}
	return FX_PERMANENT;
}

// 0x2D State:Stun
static int power_word_stun_iwd2(Actor *target, Effect *fx)
{
	int hp = BASE_GET(IE_HITPOINTS);
	if (hp > 150) return FX_NOT_APPLIED;
	int stuntime;
	if (hp > 100) {
		stuntime = core->Roll(1, 4, 0);
	} else if (hp > 50) {
		stuntime = core->Roll(2, 4, 0);
	} else {
		stuntime = core->Roll(4, 4, 0);
	}
	fx->Parameter2 = 0;
	fx->TimingMode = FX_DURATION_ABSOLUTE;
	fx->Duration = stuntime * core->Time.round_size + core->GetGame()->GameTime;
	STATE_SET(STATE_STUNNED);
	STAT_SET(IE_HELD, 1);
	target->AddPortraitIcon(PI_STUN_IWD);
	return FX_APPLIED;
}

int fx_set_stun_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_set_stun_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	//actually the original engine just skips this effect if the target is dead
	if (STATE_GET(STATE_DEAD) || core->InCutSceneMode()) {
		return FX_NOT_APPLIED;
	}

	//this is an IWD extension
	if (target->HasSpellState(SS_BLOODRAGE)) {
		return FX_NOT_APPLIED;
	}

	if (target->GetStat(IE_EXTSTATE_ID) & EXTSTATE_EYE_FORT) {
		target->fxqueue.RemoveAllEffects(fx_eye_fortitude_ref);
		target->spellbook.RemoveSpell(SevenEyes[EYE_FORT]);
		target->SetBaseBit(IE_EXTSTATE_ID, EXTSTATE_EYE_FORT, false);
		return FX_ABORT;
	}

	if (fx->Parameter2==2) {
		//don't reroll the duration next time we get here
		if (fx->FirstApply) {
			return power_word_stun_iwd2(target, fx);
		}
	}
	STATE_SET( STATE_STUNNED );
	if (core->HasFeature(GFFlags::IWD2_SCRIPTNAME)) { // all iwds
		target->AddPortraitIcon(PI_STUN_IWD);
	} else {
		target->AddPortraitIcon(PI_STUN);
	}
	if (fx->Parameter2==1) {
		target->SetSpellState(SS_AWAKE);
	}
	return FX_APPLIED;
}

// 0x2E Cure:Stun
int fx_cure_stun_state (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_cure_stun_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	BASE_STATE_CURE( STATE_STUNNED );
	target->fxqueue.RemoveAllEffects(fx_set_stun_state_ref);
	target->fxqueue.RemoveAllEffects(fx_hold_creature_no_icon_ref);
	target->fxqueue.RemoveAllEffectsWithParam(fx_display_portrait_icon_ref, PI_HELD);
	target->fxqueue.RemoveAllEffectsWithParam(fx_display_portrait_icon_ref, PI_HOPELESS);
	return FX_NOT_APPLIED;
}

// 0x2F Cure:Invisible
// 0x74 Cure:Invisible2
int fx_cure_invisible_state (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_cure_invisible_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	const Game *game = core->GetGame();
	if (!STATE_GET(STATE_NONDET) && !game->StateOverrideFlag && !game->StateOverrideTime) {
		if (core->HasFeature(GFFlags::PST_STATE_FLAGS)) {
			BASE_STATE_CURE( STATE_PST_INVIS );
		} else {
			BASE_STATE_CURE( STATE_INVISIBLE | STATE_INVIS2 );
		}
		target->fxqueue.RemoveAllEffects(fx_set_invisible_state_ref);
	}
	return FX_NOT_APPLIED;
}

// 0x30 Cure:Silence

int fx_cure_silenced_state (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_cure_silenced_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	BASE_STATE_CURE( STATE_SILENCED );
	target->fxqueue.RemoveAllEffects(fx_set_silenced_state_ref);
	return FX_NOT_APPLIED;
}

// 0x31 WisdomModifier
int fx_wisdom_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_wisdom_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	HandleMainStatBonus(target, IE_WIS, fx);

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD( IE_WIS );
	} else {
		STAT_MOD( IE_WIS );
	}
	return FX_PERMANENT;
}

// 0x32 Color:BriefRGB
int fx_brief_rgb (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_brief_rgb(%2d): RGB: %d, Location and speed: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	int speed = (fx->Parameter2 >> 16) & 0xff;
	target->SetColorMod(0xff, RGBModifier::ADD, speed,
						Color::FromBGRA(fx->Parameter1), 0);

	return FX_NOT_APPLIED;
}

// 0x33 Color:DarkenRGB
int fx_darken_rgb (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_darken_rgb(%2d): RGB: %d, Location and speed: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	EffectQueue::HackColorEffects(target, fx);
	ieDword location = fx->Parameter2 & 0xff;
	target->SetColorMod(location, RGBModifier::TINT, -1, Color::FromBGRA(fx->Parameter1));
	return FX_APPLIED;
}

// 0x34 Color:GlowRGB
int fx_glow_rgb (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_glow_rgb(%2d): RGB: %d, Location and speed: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	EffectQueue::HackColorEffects(target, fx);
	ieDword location = fx->Parameter2 & 0xff;
	target->SetColorMod(location, RGBModifier::BRIGHTEN, -1,
						Color::FromBGRA(fx->Parameter1));

	return FX_APPLIED;
}

// 0x35 AnimationIDModifier
int fx_animation_id_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_animation_id_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	switch (fx->Parameter2) {
	case 0: //non permanent animation change
	default:
		STAT_SET_PCF( IE_ANIMATION_ID, fx->Parameter1 );
		return FX_APPLIED;
	case 1: //remove any non permanent change
		target->fxqueue.RemoveAllEffects(fx_animation_id_modifier_ref);
		return FX_NOT_APPLIED;
	case 2: //permanent animation id change
		// also removes previous changes
		if (fx->Parameter1) {
			target->SetBase(IE_ANIMATION_ID, fx->Parameter1);
		} else {
			// avoid crashing
			target->SetBase(IE_AVATARREMOVAL, 1);
		}
		target->fxqueue.RemoveAllEffects(fx_animation_id_modifier_ref);
		return FX_NOT_APPLIED;
	}
}

// 0x36 ToHitModifier
int fx_to_hit_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_to_hit_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	int percentage;
	switch (fx->Parameter2) {
	case MOD_ADDITIVE:
	default:
		target->ToHit.HandleFxBonus(fx->Parameter1, fx->TimingMode==FX_DURATION_INSTANT_PERMANENT);
		break;
	case MOD_ABSOLUTE:
		if (fx->TimingMode == FX_DURATION_INSTANT_PERMANENT) {
			target->ToHit.SetBase(fx->Parameter1);
		} else {
			//FIXME: two such effects probably should not stack, but they do for now
			target->ToHit.SetFxBonus(fx->Parameter1 - target->ToHit.GetBase(), MOD_ADDITIVE);
		}
		break;
	case MOD_PERCENT:
		percentage = target->ToHit.GetBase() * fx->Parameter1 / 100;
		if (fx->TimingMode == FX_DURATION_INSTANT_PERMANENT) {
			target->ToHit.SetBase(percentage);
		} else {
			target->ToHit.SetFxBonus(percentage - target->ToHit.GetBase(), MOD_ADDITIVE);
		}
		break;
	}
	return FX_PERMANENT;
}

// 0x37 KillCreatureType

int fx_kill_creature_type (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_kill_creature_type(%2d): Value: %d, IDS: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	if (EffectQueue::match_ids( target, fx->Parameter2, fx->Parameter1) ) {
		//convert it to a death opcode or apply the new effect?
		fx->Opcode = EffectQueue::ResolveEffect(fx_death_ref);
		fx->TimingMode = FX_DURATION_INSTANT_PERMANENT;
		fx->Parameter1 = 0;
		fx->Parameter2 = 4;
		return FX_APPLIED;
	}
	//doesn't stick
	return FX_NOT_APPLIED;
}

// 0x38 Alignment:Invert
//switch good to evil and evil to good (neutral unchanged)
//also switch chaotic to lawful and vice versa (neutral unchanged)
//gemrb extension: param2 actually controls which parts should be reversed
// 0 - switch both (as original)
// 1 - switch good and evil
// 2 - switch lawful and chaotic

int fx_alignment_invert (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	static const int al_switch_both[12] = { 0, 0x33, 0x32, 0x31, 0, 0x23, 0x22, 0x21, 0, 0x13, 0x12, 0x11 };
	static const int al_switch_law[12] = { 0, 0x31, 0x32, 0x33, 0, 0x21, 0x22, 0x23, 0, 0x11, 0x12, 0x13 };
	static const int al_switch_good[12] = { 0, 0x13, 0x12, 0x11, 0, 0x23, 0x22, 0x21, 0, 0x33, 0x32, 0x31 };

	// print("fx_alignment_invert(%2d)", fx->Opcode);
	ieDword newalign = target->GetStat( IE_ALIGNMENT );
	if (!newalign) {
		// unset, so just do nothing;
		return FX_APPLIED;
	}
	//compress the values. GNE is the first 2 bits originally
	//LNC is the 4/5. bits.
	newalign = (newalign & AL_GE_MASK) | (((newalign & AL_LC_MASK)-0x10)>>2);
	switch (fx->Parameter2) {
	default:
		newalign = al_switch_both[newalign];
		break;
	case 1: //switch good/evil
		newalign = al_switch_good[newalign];
		break;
	case 2:
		newalign = al_switch_law[newalign];
		break;
	}
	STAT_SET( IE_ALIGNMENT, newalign );
	return FX_APPLIED;
}

// 0x39 Alignment:Change
int fx_alignment_change (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_alignment_change(%2d): Value: %d", fx->Opcode, fx->Parameter2);
	STAT_SET( IE_ALIGNMENT, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x3a DispelEffects
int fx_dispel_effects (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// ees added an upper word to tweak targeting of weapons in SLOT_MAGIC
	int slot = Inventory::GetMagicSlot();
	ieDword itemLevel;
	if (fx->Parameter2 > 2 && !target->inventory.IsSlotEmpty(slot)) {
		switch (fx->Parameter2 >> 16) {
			case 0:
			default: // always dispel, ignore undispellable
				if (!(target->inventory.GetItemFlag(slot) & IE_INV_ITEM_NO_DISPEL)) target->inventory.RemoveItem(slot);
				break;
			case 1: // never dispel, regardless of flags
				break;
			case 2: // use caster level, ignore undispellable
				if (target->inventory.GetItemFlag(slot) & IE_INV_ITEM_NO_DISPEL) break;
				itemLevel = std::max(1U, target->GetAnyActiveCasterLevel());
				if (EffectQueue::RollDispelChance(fx->CasterLevel, itemLevel)) target->inventory.RemoveItem(slot);
				break;
		}
	}

	switch (fx->Parameter2 & 3) {
	case 0:
	default:
		// dispel everything
		target->fxqueue.RemoveLevelEffects(0xffffffff, RL_DISPELLABLE, 0, target);
		break;
	case 1:
		//same level: 50% success, positive level diff modifies it by 5%, negative by -10%
		target->fxqueue.DispelEffects(fx, fx->CasterLevel);
		break;
	case 2:
		//same level: 50% success, positive level diff modifies it by 5%, negative by -10%
		target->fxqueue.DispelEffects(fx, fx->Parameter1);
		break;
	}

	return FX_NOT_APPLIED;
}

// 0x3B StealthModifier
int fx_stealth_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_stealth_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	STAT_MOD( IE_STEALTH );
	return FX_APPLIED;
}

// 0x3C MiscastMagicModifier
int fx_miscast_magic_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_miscast_magic_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	switch (fx->Parameter2) {
	case 3:
		STAT_SET( IE_DEADMAGIC, 1);
		// intentional fallthrough
	case 0:
		STAT_SET( IE_SPELLFAILUREMAGE, fx->Parameter1);
		break;
	case 4:
		STAT_SET( IE_DEADMAGIC, 1);
		// intentional fallthrough
	case 1:
		STAT_SET( IE_SPELLFAILUREPRIEST, fx->Parameter1);
		break;
	case 5:
		STAT_SET( IE_DEADMAGIC, 1);
		// intentional fallthrough
	case 2:
		STAT_SET( IE_SPELLFAILUREINNATE, fx->Parameter1);
		break;
	default:
		return FX_NOT_APPLIED;
	}
	return FX_APPLIED;
}

// 0x3D AlchemyModifier
// this crashes in bg2 due to assertion failure (disabled intentionally)
// and in iwd it doesn't really follow the stat_mod convention (quite lame)
// TODO: ee, Creature RGB color fade
int fx_alchemy_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_alchemy_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	switch(fx->Parameter2) {
	case 0:
		STAT_ADD( IE_ALCHEMY, fx->Parameter1 );
		break;
	case 1:
		STAT_SET( IE_ALCHEMY, fx->Parameter1 );
		break;
	case 2:
		STAT_SET( IE_ALCHEMY, 100 );
		break;
	}
	return FX_APPLIED;
}

// 0x3E PriestSpellSlotsModifier
int fx_bonus_priest_spells (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_bonus_priest_spells(%2d): Spell Add: %d ; Spell Level: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	int i=1;
	//if param2 is 0, then double spells up to param1
	if(!fx->Parameter2) {
		for (unsigned int j=0;j<fx->Parameter1 && j<MAX_SPELL_LEVEL;j++) {
			target->spellbook.SetMemorizableSpellsCount(0, IE_SPELL_TYPE_PRIEST, j, true);
		}
		return FX_APPLIED;
	}

	//HoW specific
	//if param2 is 0x200, then double spells at param1
	if (fx->Parameter2==0x200) {
		unsigned int j = fx->Parameter1-1;
		target->spellbook.SetMemorizableSpellsCount(fx->Parameter1, IE_SPELL_TYPE_PRIEST, j, true);
		return FX_APPLIED;
	}

	for(unsigned int j=0;j<MAX_SPELL_LEVEL;j++) {
		if (fx->Parameter2&i) {
			target->spellbook.SetMemorizableSpellsCount(fx->Parameter1, IE_SPELL_TYPE_PRIEST, j, true);
		}
		i<<=1;
	}
	return FX_APPLIED;
}

// 0x3F State:Infravision
int fx_set_infravision_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_set_infravision_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_STATE_SET( STATE_INFRA );
	} else {
		STATE_SET( STATE_INFRA );
	}
	return FX_PERMANENT;
}

// 0x40 Cure:Infravision

int fx_cure_infravision_state (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_cure_infravision_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	BASE_STATE_CURE( STATE_INFRA );
	target->fxqueue.RemoveAllEffects(fx_set_infravision_state_ref);
	return FX_NOT_APPLIED;
}

// 0x41 State:Blur
int fx_set_blur_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_set_blur_state(%2d)", fx->Opcode);
	//death stops this effect
	if (STATE_GET( STATE_DEAD) ) {
		return FX_NOT_APPLIED;
	}
	// ensure we don't stack in pst
	if (core->HasFeature(GFFlags::PST_STATE_FLAGS) && STATE_GET(STATE_BLUR)) {
		// don't be cumulative — luckily the blur opcode is the first on the effect list of spwi216
		// we remove the old spell, so the player benefits from renewed duration
		ResRef tmp = fx->SourceRef;
		fx->SourceRef = "";
		target->fxqueue.RemoveAllEffects(tmp);
		fx->SourceRef = tmp;
	}
	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_STATE_SET( STATE_BLUR );
	} else {
		STATE_SET( STATE_BLUR );
	}
	//iwd2 specific
	if (core->HasFeature(GFFlags::ENHANCED_EFFECTS)) {
		target->AddPortraitIcon(PI_BLUR);
	}
	return FX_PERMANENT;
}

// 0x42 TransparencyModifier
int fx_transparency_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_transparency_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	bool permanent = fx->TimingMode == FX_DURATION_INSTANT_PERMANENT;
	bool done = true;
	ieDword transp;
	if (fx->Parameter2 == 1 || fx->Parameter2 == 2) {
		if (permanent) {
			transp = target->GetBase(IE_TRANSLUCENT);
		} else {
			transp = target->GetStat(IE_TRANSLUCENT);
		}
		
		if (fx->Parameter2 == 1) { // fade in
			// the stat setting functions don't handle minimum values so we need to do it ourselves
			transp -= std::min(std::max(fx->Parameter1, 1u), transp);
			done = transp <= 0;
		} else { // fade out
			transp += std::max(fx->Parameter1 ,1u);
			done = transp >= 255;
		}
	} else {
		transp = fx->Parameter1;
	}

	if (permanent) {
		target->SetBase(IE_TRANSLUCENT, transp);
		if (done) {
			return FX_PERMANENT;
		}
	} else {
		target->SetStat(IE_TRANSLUCENT, transp, 1);
	}

	return FX_APPLIED;
}

// 0x43 SummonCreature
int fx_summon_creature (Scriptable* Owner, Actor* target, Effect* fx)
{
	// print("fx_summon_creature(%2d): ResRef:%s Anim:%s Type: %d", fx->Opcode, fx->Resource, fx->Resource2, fx->Parameter2);

	static const int eamods[] = { EAM_ALLY, EAM_ALLY, EAM_DEFAULT, EAM_ALLY, EAM_DEFAULT, EAM_ENEMY, EAM_ALLY };

	//summon creature (resource), play vvc (resource2)
	//creature's lastsummoner is Owner
	//creature's target is target
	//position of appearance is target's pos
	int eamod = -1;
	if (fx->Parameter2<6){
		// NOTE: IESDP suggests eamods might have some wrong values
		// keep it in mind if anything goes wrong
		eamod = eamods[fx->Parameter2];
	}

	//the monster should appear near the effect position
	Effect *newfx = EffectQueue::CreateUnsummonEffect(fx);
	core->SummonCreature(fx->Resource, fx->Resource2, Owner, target, fx->Pos, eamod, 0, newfx);
	return FX_NOT_APPLIED;
}

// 0x44 UnsummonCreature
int fx_unsummon_creature (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_unsummon_creature(%2d)", fx->Opcode);

	//to be compatible with the original engine, unsummon doesn't work with PC's
	//but it works on anything else
	Map *area = target->GetCurrentArea();
	if (!target->InParty && area) {
		//play the vanish animation
		ScriptedAnimation* sca = gamedata->GetScriptedAnimation(fx->Resource, false);
		if (sca) {
			sca->Pos = target->Pos;
			area->AddVVCell(sca);
		}
		//remove the creature
		target->DestroySelf();
		return FX_NOT_APPLIED;
	}

	//the original keeps the effect around on partymembers or
	//on those who don't have an area and executes it when the conditions apply.
	return FX_APPLIED;
}

// 0x45 State:Nondetection
int fx_set_nondetection_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_set_nondetection_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_STATE_SET( STATE_NONDET );
	} else {
		STATE_SET( STATE_NONDET );
	}
	return FX_PERMANENT;
}

// 0x46 Cure:Nondetection

int fx_cure_nondetection_state (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_cure_nondetection_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	BASE_STATE_CURE( STATE_NONDET );
	target->fxqueue.RemoveAllEffects(fx_set_nondetection_state_ref);
	return FX_NOT_APPLIED;
}

// 0x47 SexModifier
int fx_sex_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_sex_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	ieDword value;
	if (fx->Parameter2) {
		value = fx->Parameter1;
	} else {
		if (STAT_GET(IE_SEX_CHANGED)) {
			return FX_NOT_APPLIED;
		}
		STAT_SET( IE_SEX_CHANGED, 1);
		value = STAT_GET(IE_SEX);
		if (value==SEX_MALE) {
			value = SEX_FEMALE;
		} else {
			value = SEX_MALE;
		}
	}
	STAT_SET( IE_SEX, value );
	return FX_APPLIED;
}

// 0x48 AIIdentifierModifier
int fx_ids_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_ids_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	ieDword stat = 0;
	switch (fx->Parameter2) {
	case 0:
		stat = IE_EA;
		break;
	case 1:
		stat = IE_GENERAL;
		break;
	case 2:
		stat = IE_RACE;
		break;
	case 3:
		stat = IE_CLASS;
		break;
	case 4:
		stat = IE_SPECIFIC;
		break;
	case 5:
		stat = IE_SEX;
		break;
	case 6:
		stat = IE_ALIGNMENT;
		break;
	default:
		return FX_NOT_APPLIED;
	}
	if (fx->TimingMode == FX_DURATION_INSTANT_PERMANENT) {
		BASE_SET(stat, fx->Parameter1);
	} else {
		STAT_SET(stat, fx->Parameter1);
	}
	return FX_PERMANENT;
}

// 0x49 DamageBonusModifier
int fx_damage_bonus_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_damage_bonus_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	STAT_MOD( IE_DAMAGEBONUS );
	//the basestat is not saved, so no FX_PERMANENT
	return FX_APPLIED;
}

// 0x49 DamageBonusModifier(2)
// iwd/iwd2 supports different damage types, but only flat and percentage boni
// only the special type of 0 means a flat bonus
int fx_damage_bonus_modifier2 (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_damage_bonus_modifier2(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	switch (fx->Parameter2) {
		case 0:
			STAT_MOD(IE_DAMAGEBONUS);
			break;
		case 1: // fire
		case 2: // cold
		case 3: // electricity
		case 4: // acid
		case 5: // magic
		case 6: // poison
		case 7: // slashing
		case 8: // piercing
		case 9: // crushing
		case 10: // missile
			// no stat to save to, so we handle it when dealing damage
			break;
		// gemrb extensions for tobex
		case 11: // magic fire
		case 12: // magic cold
		case 13: // stunning
			break;
		default:
			return FX_NOT_APPLIED;
	}
	return FX_APPLIED;
}

// 0x4a State:Blind
int fx_set_blind_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_set_blind_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (target->GetStat(IE_EXTSTATE_ID) & EXTSTATE_EYE_FORT) {
		target->fxqueue.RemoveAllEffects(fx_eye_fortitude_ref);
		target->spellbook.RemoveSpell(SevenEyes[EYE_FORT]);
		target->SetBaseBit(IE_EXTSTATE_ID, EXTSTATE_EYE_FORT, false);
		return FX_ABORT;
	}

	//pst power word blind projectile support
	if (fx->Parameter2==1) {
		fx->Parameter2 = 0;
		int stat = target->GetSafeStat(IE_HITPOINTS);
		if (stat<25) {
			stat = core->Roll(1,240,150);
		} else if (stat<50) {
			stat = core->Roll(1,120,70);
		} else if (stat<100) {
			stat = core->Roll(1,30,15);
		} else stat = 0;
		fx->Duration = core->GetGame()->GameTime+stat;
	}

	//don't do this effect twice (bug exists in BG2, but fixed in IWD2)
	static bool reverse = core->HasFeature(GFFlags::REVERSE_TOHIT);
	if (!STATE_GET(STATE_BLIND)) {
		STATE_SET( STATE_BLIND );
		//the feat normally exists only in IWD2, but won't hurt
		if (!target->GetFeat(FEAT_BLIND_FIGHT)) {
			target->AddPortraitIcon(PI_BLIND);
			if (reverse) {
				//BG2
				target->AC.HandleFxBonus(-4, fx->TimingMode==FX_DURATION_INSTANT_PERMANENT);
				target->ToHit.HandleFxBonus(-4, fx->TimingMode==FX_DURATION_INSTANT_PERMANENT);
			} else {
				//IWD2
				target->AC.HandleFxBonus(-2, fx->TimingMode==FX_DURATION_INSTANT_PERMANENT);
				// no dexterity bonus to AC (caught flatfooted) is handled in core
			}
		}
	}
	// this part is unaffected by blind fighting feat
	if (!reverse) {
		// 50% inherent miss chance (full concealment)
		STAT_ADD(IE_ETHEREALNESS, 50<<8);
	}
	//this should be FX_PERMANENT, but the current code is a mess here. Review after cleaned up
	return FX_APPLIED;
}

// 0x4b Cure:Blind

int fx_cure_blind_state (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_cure_blind_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	BASE_STATE_CURE( STATE_BLIND );
	target->fxqueue.RemoveAllEffects(fx_set_blind_state_ref);
	return FX_NOT_APPLIED;
}

// 0x4c State:Feeblemind
int fx_set_feebleminded_state (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_set_feebleminded_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	STATE_SET( STATE_FEEBLE );
	STAT_SET( IE_INT, 3);
	if (core->HasFeature(GFFlags::ENHANCED_EFFECTS)) {
		target->AddPortraitIcon(PI_FEEBLEMIND);
	}
	//This state is better off with always stored, because of the portrait icon and the int stat
	//it wouldn't be easily cured if it would go away after irrevocably altering another stat
	return FX_APPLIED;
}

// 0x4d Cure:Feeblemind

int fx_cure_feebleminded_state (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_cure_feebleminded_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	BASE_STATE_CURE( STATE_FEEBLE );
	target->fxqueue.RemoveAllEffects(fx_set_feebleminded_state_ref);
	target->fxqueue.RemoveAllEffectsWithParam(fx_display_portrait_icon_ref, PI_FEEBLEMIND);
	return FX_NOT_APPLIED;
}

// 0x4e State:Diseased
int fx_set_diseased_state(Scriptable* Owner, Actor* target, Effect* fx)
{
	// print("fx_set_diseased_state(%2d): Damage: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	if (STATE_GET(STATE_DEAD|STATE_PETRIFIED|STATE_FROZEN) ) {
		return FX_NOT_APPLIED;
	}

	int count = target->fxqueue.CountEffects(fx_diseased_state_ref, fx->Parameter1, fx->Parameter2, fx->Resource);
	if (count > 1) {
		return FX_APPLIED;
	}

	//setting damage to 0 because not all types do damage
	ieDword damage = 0;
	ieDword damageType = DAMAGE_POISON;
	// fx->Parameter4 is an optional frequency multiplier
	ieDword aRound = (fx->Parameter4 ? fx->Parameter4 : 1) * core->Time.defaultTicksPerSec;

	HandlePercentageDamage(fx, target);

	switch(fx->Parameter2) {
	case RPD_SECONDS:
		damage = 1;
		if (fx->Parameter1 && (core->GetGame()->GameTime % target->GetAdjustedTime(fx->Parameter1 * aRound))) {
			return FX_APPLIED;
		}
		break;
	case RPD_PERCENT: // handled in HandlePercentageDamage
	case RPD_POINTS:
		damage = fx->Parameter1;
		// per second
		if (core->GetGame()->GameTime % target->GetAdjustedTime(aRound)) {
			return FX_APPLIED;
		}
		break;
	case RPD_STR: //strength
		STAT_SUB(IE_STR, fx->Parameter1);
		break;
	case RPD_DEX: //dex
		STAT_SUB(IE_DEX, fx->Parameter1);
		break;
	case RPD_CON: //con
		STAT_SUB(IE_CON, fx->Parameter1);
		break;
	case RPD_INT: //int
		STAT_SUB(IE_INT, fx->Parameter1);
		break;
	case RPD_WIS: //wis
		STAT_SUB(IE_WIS, fx->Parameter1);
		break;
	case RPD_CHA: //cha
		STAT_SUB(IE_CHR, fx->Parameter1);
		break;
	case RPD_CONTAGION: //contagion (iwd2) - an aggregate of STR,DEX,CHR,SLOW diseases
		STAT_SUB(IE_STR, fx->Parameter1 ? fx->Parameter1 : 2);
		STAT_SUB(IE_DEX, fx->Parameter1 ? fx->Parameter1 : 2);
		STAT_SUB(IE_CHR, fx->Parameter1 ? fx->Parameter1 : 2);
		//fall through
	case RPD_SLOW: //slow
		if (core->HasFeature(GFFlags::RULES_3ED)) {
			if (target->HasSpellState(SS_FREEACTION)) return FX_NOT_APPLIED;
			if (target->HasSpellState(SS_AEGIS)) return FX_NOT_APPLIED;
			STAT_SUB(IE_ARMORCLASS, 2);
			STAT_SUB(IE_TOHIT, 2);
			STAT_SUB(IE_DAMAGEBONUS, 2);
			STAT_SUB(IE_SAVEREFLEX, 2);
			STAT_MUL(IE_MOVEMENTRATE, 50);
		} else {
			// in bg2 normal slow
			fx_set_slowed_state(Owner, target, fx);
		}
		target->AddPortraitIcon(PI_SLOWED);
		break;
	case RPD_MOLD2:
	case RPD_MOLD: //mold touch (how)
		EXTSTATE_SET(EXTSTATE_MOLD);
		target->SetSpellState(SS_MOLDTOUCH);
		if (core->GetGame()->GameTime % target->GetAdjustedTime(aRound)) {
			return FX_APPLIED;
		}
		if (fx->Parameter1<1) {
			return FX_NOT_APPLIED;
		}
		damage = core->Roll(fx->Parameter1--, 6, 0);
		damageType = DAMAGE_MAGIC;
		//TODO: spread to nearest (range 10) non-affected (use spell state?)
		break;
	case RPD_PEST:     //cloud of pestilence (iwd2)
		break;
	case RPD_DOLOR:     //dolorous decay (iwd2)
		break;
	default:
		damage = 1;
		break;
	}
	//percent
	Scriptable *caster = GetCasterObject();
	if (damage) {
		target->Damage(damage, damageType, caster);
	}
	if (fx->IsVariable) target->AddPortraitIcon(static_cast<ieByte>(fx->IsVariable));
	target->SetSpellState(SS_DISEASED);

	return FX_APPLIED;
}


// 0x4f Cure:Disease
int fx_cure_diseased_state (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_cure_diseased_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	//STATE_CURE( STATE_DISEASED ); //the bit flagged as disease is actually the active state. so this is even more unlikely to be used as advertised
	target->fxqueue.RemoveAllEffects( fx_diseased_state_ref ); //this is what actually happens in bg2
	// iwd also does this, as its mummies have permanent timing diseases
	target->fxqueue.RemoveAllEffectsWithParam(fx_display_portrait_icon_ref, PI_DISEASED);
	// also cures feeblemind, duplicating fx_cure_feebleminded_state
	BASE_STATE_CURE(STATE_FEEBLE);
	target->fxqueue.RemoveAllEffects(fx_set_feebleminded_state_ref);
	target->fxqueue.RemoveAllEffectsWithParam(fx_display_portrait_icon_ref, PI_FEEBLEMIND);
	return FX_NOT_APPLIED;
}

// 0x50 State:Deafness
int fx_set_deaf_state (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_set_deaf_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (target->GetStat(IE_EXTSTATE_ID) & EXTSTATE_EYE_FORT) {
		target->fxqueue.RemoveAllEffects(fx_eye_fortitude_ref);
		target->spellbook.RemoveSpell(SevenEyes[EYE_FORT]);
		target->SetBaseBit(IE_EXTSTATE_ID, EXTSTATE_EYE_FORT, false);
		return FX_ABORT;
	}

	//adopted IWD2 method, spellfailure will be handled internally based on the spell state
	if (target->SetSpellState(SS_DEAF)) return FX_APPLIED;

	EXTSTATE_SET(EXTSTATE_DEAF); //iwd1/how needs this
	if (core->HasFeature(GFFlags::ENHANCED_EFFECTS)) {
		target->AddPortraitIcon(PI_DEAFNESS);
	}
	return FX_APPLIED;
}

// 0x51 Cure:Deafness
//removes the deafness effect
int fx_cure_deaf_state (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_cure_deaf_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	target->fxqueue.RemoveAllEffects(fx_deaf_state_ref);
	return FX_NOT_APPLIED;
}

// 0x52 SetAIScript
int fx_set_ai_script (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// spin101 litany is supposed to sick enemies at Morte, but they didn't set the field ...
	if (fx->Resource.IsEmpty() && fx->SourceRef == "spin101") {
		fx->Resource = fx->SourceRef;
	}
	target->SetScript (fx->Resource, fx->Parameter2);
	return FX_NOT_APPLIED;
}

// 0x53 Protection:Projectile
int fx_protection_from_projectile (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_protection_from_projectile(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	STAT_BIT_OR( IE_IMMUNITY, IMM_PROJECTILE);
	return FX_APPLIED;
}

// 0x54 MagicalFireResistanceModifier
int fx_magical_fire_resistance_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_magical_fire_resistance_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD( IE_RESISTMAGICFIRE );
	} else {
		STAT_MOD( IE_RESISTMAGICFIRE );
	}
	return FX_PERMANENT;
}

// 0x55 MagicalColdResistanceModifier
int fx_magical_cold_resistance_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_magical_cold_resistance_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD( IE_RESISTMAGICCOLD );
	} else {
		STAT_MOD( IE_RESISTMAGICCOLD );
	}
	return FX_PERMANENT;
}

// 0x56 SlashingResistanceModifier
int fx_slashing_resistance_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_slashing_resistance_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD( IE_RESISTSLASHING );
	} else {
		STAT_MOD( IE_RESISTSLASHING );
	}
	return FX_PERMANENT;
}

// 0x57 CrushingResistanceModifier
int fx_crushing_resistance_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_crushing_resistance_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD( IE_RESISTCRUSHING );
	} else {
		STAT_MOD( IE_RESISTCRUSHING );
	}
	return FX_PERMANENT;
}

// 0x58 PiercingResistanceModifier
int fx_piercing_resistance_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_piercing_resistance_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD( IE_RESISTPIERCING );
	} else {
		STAT_MOD( IE_RESISTPIERCING );
	}
	return FX_PERMANENT;
}

// 0x59 MissilesResistanceModifier
int fx_missiles_resistance_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_missiles_resistance_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD( IE_RESISTMISSILE );
	} else {
		STAT_MOD( IE_RESISTMISSILE );
	}
	return FX_PERMANENT;
}

// 0x5A OpenLocksModifier
int fx_open_locks_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_open_locks_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD( IE_LOCKPICKING );
	} else {
		STAT_MOD( IE_LOCKPICKING );
	}
	return FX_PERMANENT;
}

// 0x5B FindTrapsModifier
int fx_find_traps_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_find_traps_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD( IE_TRAPS );
	} else {
		STAT_MOD( IE_TRAPS );
	}
	return FX_PERMANENT;
}

// 0x5C PickPocketsModifier
int fx_pick_pockets_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_pick_pockets_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD( IE_PICKPOCKET );
	} else {
		STAT_MOD( IE_PICKPOCKET );
	}
	return FX_PERMANENT;
}

// 0x5D FatigueModifier
int fx_fatigue_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_fatigue_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD( IE_FATIGUE );
	} else {
		STAT_MOD( IE_FATIGUE );
	}
	return FX_PERMANENT;
}

// 0x5E IntoxicationModifier
int fx_intoxication_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_intoxication_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD( IE_INTOXICATION );
	} else {
		STAT_MOD( IE_INTOXICATION );
	}
	return FX_PERMANENT;
}

// 0x5F TrackingModifier
int fx_tracking_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_tracking_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD( IE_TRACKING );
	} else {
		STAT_MOD( IE_TRACKING );
	}
	return FX_PERMANENT;
}

// 0x60 LevelModifier
int fx_level_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_level_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	STAT_MOD( IE_LEVEL );
	//While the original can, i would rather not modify the base stat here...
	return FX_APPLIED;
}

// 0x61 StrengthBonusModifier
int fx_strength_bonus_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_strength_bonus_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD( IE_STREXTRA );
	} else {
		STAT_MOD( IE_STREXTRA );
	}
	return FX_APPLIED;
}

// 0x62 State:Regenerating
int fx_set_regenerating_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_set_regenerating_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	int damage;
	int tmp = fx->Parameter1;
	ieDword gameTime = core->GetGame()->GameTime;
	// fx->Parameter4 is an optional frequency multiplier
	ieDword aRound = (fx->Parameter4 ? fx->Parameter4 : 1) * core->Time.defaultTicksPerSec;
	tick_t timeStep = target->GetAdjustedTime(aRound);

	if (fx->FirstApply) {
		// ensure we prepare Parameter5 now
	} else {
		//we can have multiple calls at the same gameTime, so we
		//just go to gameTime+1 to ensure one call
		ieDword nextHeal = fx->Parameter5;
		if (nextHeal>=gameTime) return FX_APPLIED;
	}

	HandlePercentageDamage(fx, target);

	switch(fx->Parameter2) {
	case RPD_TURNS:		//restore param3 hp every param1 turns
		tmp *= core->Time.rounds_per_turn;
		// fall through
	case RPD_ROUNDS:	//restore param3 hp every param1 rounds
		tmp *= core->Time.round_sec;
		// fall through
	case RPD_SECONDS:	//restore param3 hp every param1 seconds
		fx->Parameter5 = gameTime + tmp * static_cast<ieDword>(timeStep);
		damage = fx->Parameter3 ? fx->Parameter3 : 1;
		break;
	case RPD_PERCENT: // handled in HandlePercentageDamage
	case RPD_POINTS:	//restore param1 hp every second? that's crazy!
		damage = fx->Parameter1;
		fx->Parameter5 = gameTime + static_cast<ieDword>(timeStep);
		break;
	default:
		fx->Parameter5 = gameTime + static_cast<ieDword>(timeStep);
		damage = fx->Parameter3 ? fx->Parameter3 : 1;
		break;
	}

	// different in iwd2 only?
	// x hp per 1 round
	if (fx->Parameter2 == RPD_ROUNDS && core->HasFeature(GFFlags::ENHANCED_EFFECTS)) {
		damage = fx->Parameter1;
		fx->Parameter5 = gameTime + core->Time.round_sec * static_cast<ieDword>(timeStep);
	}

	if (fx->FirstApply) {
		//don't add hp in the first occasion, so it cannot be used for cheat heals
		return FX_APPLIED;
	}

	target->NewBase(IE_HITPOINTS, damage, MOD_ADDITIVE);
	if (fx->IsVariable) target->AddPortraitIcon(static_cast<ieByte>(fx->IsVariable));
	return FX_APPLIED;
}

// 0x63 SpellDurationModifier
int fx_spell_duration_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_spell_duration_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	switch (fx->Parameter2) {
		case 0:
			STAT_SET( IE_SPELLDURATIONMODMAGE, fx->Parameter1);
			break;
		case 1:
			STAT_SET( IE_SPELLDURATIONMODPRIEST, fx->Parameter1);
			break;
		default:
			return FX_NOT_APPLIED;
	}
	return FX_APPLIED;
}
// 0x64 Protection:Creature
int fx_generic_effect (Scriptable* /*Owner*/, Actor* /*target*/, Effect* /*fx*/)
{
	// print("fx_generic_effect(%2d): Param1: %d, Param2: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	return FX_APPLIED;
}

// 0x65 Protection:Opcode
int fx_protection_opcode (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_protection_opcode(%2d): Opcode: %d", fx->Opcode, fx->Parameter2);
	STAT_BIT_OR(IE_IMMUNITY, IMM_OPCODE);
	return FX_APPLIED;
}

// 0x66 Protection:SpellLevel
int fx_protection_spelllevel (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_protection_spelllevel(%2d) Level: %d", fx->Opcode, fx->Parameter1);

	int value = fx->Parameter1;
	if (value <= 9) {
		STAT_BIT_OR(IE_MINORGLOBE, 1<<value);
		STAT_BIT_OR(IE_IMMUNITY, IMM_LEVEL);
		return FX_APPLIED;
	}
	return FX_NOT_APPLIED;
}

// 0x67 ChangeName
int fx_change_name (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_change_name_modifier(%2d): StrRef: %d", fx->Opcode, fx->Parameter1);
	//this also changes the base stat
	target->SetName(ieStrRef(fx->Parameter1), 0);
	return FX_NOT_APPLIED;
}

// 0x68 ExperienceModifier
int fx_experience_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_experience_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	BASE_MOD(IE_XP);
	return FX_NOT_APPLIED;
}

// 0x69 GoldModifier
//in BG2 this effect subtracts gold when type is MOD_ADDITIVE
//no one uses it, though. To keep the function, the default branch will do the subtraction
int fx_gold_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_gold_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	if (!target->InParty) {
		STAT_MOD( IE_GOLD );
		return FX_NOT_APPLIED;
	}
	int gold;
	Game *game = core->GetGame();
	//for party members, the gold is stored in the game object
	switch( fx->Parameter2) {
		case MOD_ADDITIVE:
			if (core->HasFeature(GFFlags::FIXED_MORALE_OPCODE)) {
				gold = - signed(fx->Parameter1);
			} else {
				gold = fx->Parameter1;
			}
			break;
		case MOD_ABSOLUTE:
			gold = fx->Parameter1-game->PartyGold;
			break;
		case MOD_PERCENT:
			gold = game->PartyGold*fx->Parameter1/100-game->PartyGold;
			break;
		default:
			gold = - signed(fx->Parameter1);
			break;
	}
	game->AddGold (gold);
	return FX_NOT_APPLIED;
}

// 0x6a MoraleBreakModifier
int fx_morale_break_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_morale_break_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD(IE_MORALEBREAK);
	} else {
		STAT_MOD(IE_MORALEBREAK);
	}
	return FX_PERMANENT; //permanent morale break doesn't stick
}

// 0x6b PortraitChange
// 0 - small
// 1 - large
// 2 - both
int fx_portrait_change (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_portrait_change(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	//in gemrb this is 0 both, 1 Large, 2 Small so we have to swap 2 and 0
	target->SetPortrait( fx->Resource, 2-fx->Parameter2);
	return FX_NOT_APPLIED;
}

// 0x6c ReputationModifier
int fx_reputation_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (fx->Parameter2 < 3) {
		STAT_MOD(IE_REPUTATION);
	} else {
		// EEs: modify party reputation instead, with a lower limit of 100
		// should it also affect individuals?
		Game* game = core->GetGame();
		if (fx->Parameter2 == 3) { // cumulative
			game->SetReputation(game->Reputation + fx->Parameter1 * 10, 100);
		} else if (fx->Parameter2 == 4) { // flat
			game->SetReputation(fx->Parameter1 * 10, 100);
		} else { // percentage
			game->SetReputation(game->Reputation * fx->Parameter1 / 100, 100);
		}
	}
	//needs testing and thinking, the original engine can permanently modify the basestat
	//but it is unclear how a timed reputation change alters the global reputation stored in game
	//our current solution immediately transfers the reputation to game (in case of pc)
	return FX_NOT_APPLIED;
}

// 0x6d --> see later

//0x6e works only in PST, reused for turning undead in bg
//0x118 TurnUndead how
int fx_turn_undead (Scriptable* Owner, Actor* target, Effect* fx)
{
	// print("fx_turn_undead(%2d): Level %d", fx->Opcode, fx->Parameter1);

	if (target->GetStat(IE_NOTURNABLE) || Owner == target) {
		return FX_NOT_APPLIED;
	}
	if (fx->Parameter1) {
		target->Turn(Owner, fx->Parameter1);
	} else {
		const Actor* actor = Scriptable::As<const Actor>(Owner);
		if (!actor) {
			return FX_NOT_APPLIED;
		}
		target->Turn(Owner, actor->GetStat(IE_TURNUNDEADLEVEL));
	}
	return FX_APPLIED;
}

static int MaybeTransformTo(EffectRef& ref, Effect* fx)
{
	if ((fx->TimingMode & 0xff) == FX_DURATION_INSTANT_LIMITED) {
		// if this effect has expiration, then it will remain as a remove_item or remove_inventory_item
		// on the effect queue, inheriting all the parameters
		fx->Opcode = EffectQueue::ResolveEffect(ref);
		fx->TimingMode = FX_DURATION_DELAY_PERMANENT;
		return FX_APPLIED;
	}
	return FX_NOT_APPLIED;
}

// 0x6f Item:CreateMagic
int fx_create_magic_item (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	//charge count is the same for all slots by default
	if (!fx->Parameter3) fx->Parameter3 = fx->Parameter1;
	if (!fx->Parameter4) fx->Parameter4 = fx->Parameter1;
	int slot = Inventory::GetMagicSlot();
	target->inventory.SetSlotItemRes(fx->Resource, slot, fx->Parameter1, fx->Parameter3, fx->Parameter4);
	//IWD doesn't let you create two handed weapons (actually only decastave) if shield slot is filled
	//modders can still force two handed weapons with Parameter2
	if (!fx->Parameter2) {
		if (target->inventory.GetItemFlag(slot) & IE_INV_ITEM_TWOHANDED) {
			if (!target->inventory.IsSlotEmpty(target->inventory.GetShieldSlot())) {
				target->inventory.RemoveItem(slot);
				displaymsg->DisplayConstantStringNameString(HCStrings::SpellFailed, GUIColors::WHITE, HCStrings::OffhandUsed, target);
				return FX_NOT_APPLIED;
			}
		}
	}

	//equip the weapon
	// but don't add new effects if there are none, which is an ugly workaround
	// fixes infinite loop with wm_sqrl spell from "wild mage additions" mod
	const Item *itm = gamedata->GetItem(fx->Resource, true);
	if (!itm) return FX_NOT_APPLIED;
	target->inventory.SetEquippedSlot(slot - Inventory::GetWeaponSlot(), 0, itm->EquippingFeatureCount == 0);
	gamedata->FreeItem(itm, fx->Resource);
	return MaybeTransformTo(fx_remove_item_ref, fx);
}

// 0x70 Item:Remove
int fx_remove_item (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	//will destroy the first item
	if (target->inventory.DestroyItem(fx->Resource,0,1)) {
		target->ReinitQuickSlots();

		// "EFF_M02" was hardcoded in the originals, but EEs added extra options
		if (fx->Parameter1 == 0) {
			core->PlaySound(DS_ITEM_GONE, SFX_CHAN_GUI);
		} else if (fx->Parameter1 == 1) {
			core->GetAudioDrv()->PlayRelative("AMB_D02B", SFX_CHAN_GUI);
		} else if (fx->Parameter1 == 2) {
			core->GetAudioDrv()->PlayRelative(fx->Resource2, SFX_CHAN_GUI);
		}
	}

	return FX_NOT_APPLIED;
}

// 0x71 Item:Equip
int fx_equip_item (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	int eff = core->QuerySlotEffects( fx->Parameter2 );
	switch(eff) {
	case SLOT_EFFECT_NONE:
	case SLOT_EFFECT_MELEE:
		target->inventory.SetEquippedSlot( fx->Parameter2, fx->Parameter1 );
		break;
	default:
		target->inventory.EquipItem( fx->Parameter2 );
		break;
	}
	target->ReinitQuickSlots();
	return FX_NOT_APPLIED;
}

// 0x72 Dither
int fx_dither (Scriptable* /*Owner*/, Actor* /*target*/, Effect* /*fx*/)
{
	// print("fx_dither(%2d): Value: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	//this effect doesn't work in any original engine versions
	return FX_NOT_APPLIED;
}

// 0x73 DetectAlignment
//gemrb extension: chaotic/lawful detection
int fx_detect_alignment (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	ieDword msk;
	ieDword stat;
	static int ge[] = {AL_EVIL, AL_GE_NEUTRAL, AL_GOOD, AL_CHAOTIC, AL_LC_NEUTRAL, AL_LAWFUL};

	msk = ge[fx->Parameter2];
	if (fx->Parameter2<3) {
		//0,1,2 -> 3,2,1
		stat = target->GetStat(IE_ALIGNMENT)&AL_GE_MASK;
	}
	else {
		//3,4,5 -> 0x30, 0x20, 0x10
		stat = target->GetStat(IE_ALIGNMENT)&AL_LC_MASK;
	}
	if (stat != msk) return FX_NOT_APPLIED;

	Color color = Color(fx->Parameter1);
	color.a = 0xff;
	switch (msk) {
	case AL_EVIL:
		if (color == ColorBlack) color = ColorRed;
		displaymsg->DisplayConstantStringName(HCStrings::Evil, color, target);
		//glow red
		target->SetColorMod(0xff, RGBModifier::ADD, 30, Color(0xff, 0, 0, 0), 0);
		break;
	case AL_GOOD:
		if (color == ColorBlack) color = ColorGreen;
		displaymsg->DisplayConstantStringName(HCStrings::Good, color, target);
		//glow green
		target->SetColorMod(0xff, RGBModifier::ADD, 30, Color(0, 0xff, 0, 0), 0);
		break;
	case AL_GE_NEUTRAL:
		if (color == ColorBlack) color = ColorBlue;
		displaymsg->DisplayConstantStringName(HCStrings::GENeutral, color, target);
		//glow blue
		target->SetColorMod(0xff, RGBModifier::ADD, 30, Color(0, 0, 0xff, 0), 0);
		break;
	case AL_CHAOTIC:
		if (color == ColorBlack) color = ColorMagenta;
		displaymsg->DisplayConstantStringName(HCStrings::Chaotic, color, target);
		//glow purple
		target->SetColorMod(0xff, RGBModifier::ADD, 30, Color(0xff, 0, 0xff, 0), 0);
		break;
	case AL_LAWFUL:
		if (color == ColorBlack) color = ColorWhite;
		displaymsg->DisplayConstantStringName(HCStrings::Lawful, color, target);
		//glow white
		target->SetColorMod(0xff, RGBModifier::ADD, 30, Color(0xff, 0xff, 0xff, 0), 0);
		break;
	case AL_LC_NEUTRAL:
		if (color == ColorBlack) color = ColorBlue;
		displaymsg->DisplayConstantStringName(HCStrings::LCNeutral, color, target);
		//glow blue
		target->SetColorMod(0xff, RGBModifier::ADD, 30, Color(0, 0, 0xff, 0), 0);
		break;
	}
	return FX_NOT_APPLIED;
}

// 0x74 Cure:Invisible2 (see 0x2f)

// 0x75 Reveal:Area
// 0 reveal whole area
// 1 reveal area in pattern
int fx_reveal_area (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_reveal_area(%2d): Value: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	Map *map = nullptr;

	if (target) {
		map = target->GetCurrentArea();
	} else {
		map = core->GetGame()->GetCurrentArea();
	}
	if (!map) {
		return FX_APPLIED;
	}

	if (fx->Parameter2) {
		// GemRB extension
		map->FillExplored(fx->Parameter1);
	} else {
		map->FillExplored(true);
	}
	return FX_NOT_APPLIED;
}

// 0x76 Reveal:Creatures
int fx_reveal_creatures (Scriptable* /*Owner*/, Actor* /*target*/, Effect* /*fx*/)
{
	// print("fx_reveal_creatures(%2d): Value: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	//this effect doesn't work in any original engine versions
	return FX_NOT_APPLIED;
}

// 0x77 MirrorImage
int fx_mirror_image (Scriptable* Owner, Actor* target, Effect* fx)
{
	// print("fx_mirror_image(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	ieDword images;

	if (fx->Parameter2) {
		images = 1; //reflection
	} else {
		// the original uses only IE_LEVEL, but that can be awfully bad in
		// the case of dual- and multiclasses
		unsigned int level = target->GetCasterLevel(IE_SPL_WIZARD);
		if (!level) level = target->GetAnyActiveCasterLevel();
		// 2-8 mirror images
		images = level/3 + 2;
		if (images > 8) images = 8;
	}

	Effect *fx2 = target->fxqueue.HasEffect(fx_mirror_image_modifier_ref);
	if (fx2) {
		//update old effect with our numbers if our numbers are more
		if (fx2->Parameter1<images) {
			fx2->Parameter1=images;
		}
		if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
			fx2->TimingMode = FX_DURATION_INSTANT_PERMANENT;
		}
		return FX_NOT_APPLIED;
	}
	fx->Opcode = EffectQueue::ResolveEffect(fx_mirror_image_modifier_ref);
	fx->Parameter1=images;
	//parameter2 could be 0 or 1 (mirror image or reflection)
	//execute the translated effect
	return fx_mirror_image_modifier(Owner, target, fx);
}

// 0x78 Protection:Weapons
// gemrb extension: modes 12 and 13 - arbitrary checks
int fx_immune_to_weapon (Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (!fx->FirstApply) return FX_APPLIED;

	int level = -1;
	ieDword mask = 0;
	ieDword value = 0;
	switch(fx->Parameter2) {
	case 0: //enchantment level
		level = fx->Parameter1;
		break;
	case 1: //all magical weapons
		value = IE_INV_ITEM_MAGICAL;
		//fallthrough
	case 2: //all nonmagical weapons
		mask = IE_INV_ITEM_MAGICAL;
		break;
	case 3: //all silver weapons
		value = IE_INV_ITEM_SILVER;
		//fallthrough
	case 4: //all non silver weapons
		mask = IE_INV_ITEM_SILVER;
		break;
	case 5:
		value = IE_INV_ITEM_SILVER;
		mask = IE_INV_ITEM_SILVER;
		level = 0;
		break;
	case 6: //all twohanded
		value = IE_INV_ITEM_TWOHANDED;
		//fallthrough
	case 7: //all not twohanded
		mask = IE_INV_ITEM_TWOHANDED;
		break;
	case 8: //all cursed
		value = IE_INV_ITEM_CURSED;
		//fallthrough
	case 9: //all non-cursed
		mask = IE_INV_ITEM_CURSED;
		break;
	case 10: //all cold-iron
		value = IE_INV_ITEM_COLDIRON;
		//fallthrough
	case 11: //all non cold-iron
		mask = IE_INV_ITEM_COLDIRON;
		break;
	case 12:
		mask = fx->Parameter1;
		//fallthrough
	case 13:
		value = fx->Parameter1;
		break;
	default:;
	}

	fx->Parameter1 = (ieDword) level; //putting the corrected value back
	fx->Parameter3 = mask;
	fx->Parameter4 = value;
	return FX_APPLIED;
}

// 0x79 VisualAnimationEffect (unknown)
int fx_visual_animation_effect (Scriptable* /*Owner*/, Actor* /*target*/, Effect* /*fx*/)
{
	//this effect doesn't work in any original engine versions
	// print("fx_visual_animation_effect(%2d)", fx->Opcode);
	return FX_NOT_APPLIED;
}

// 0x7a Item:CreateInventory
int fx_create_inventory_item (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_create_inventory_item(%2d)", fx->Opcode);
	// EEs added randomness that can't hurt elsewhere
	ResRef *refs[] = { &fx->Resource, &fx->Resource2, &fx->Resource3 };
	char count = 1;
	if (!fx->Resource2.IsEmpty()) count++;
	if (!fx->Resource3.IsEmpty()) count++;
	int choice = RAND(0, count - 1);

	Actor* receiver = target;
	if (target->GetBase(IE_EA) == EA_FAMILIAR) {
		receiver = core->GetGame()->FindPC(1);
	}
	receiver->inventory.AddSlotItemRes(*refs[choice], SLOT_ONLYINVENTORY, fx->Parameter1, fx->Parameter3, fx->Parameter4);

	int ret = MaybeTransformTo(fx_remove_inventory_item_ref, fx);
	if (ret == FX_APPLIED) fx->Resource = *refs[choice];
	return ret;
}

// 0x7b Item:RemoveInventory
int fx_remove_inventory_item (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// changed to work on any items - book of infinite spells relies on this
	// gemrb extension: selective destruction is available if you set Parameter2 to nonzero
	// and Parameter1 to the bitfield
	target->inventory.DestroyItem(fx->Resource, fx->Parameter2 ? fx->Parameter1 : 0, 0);
	return FX_NOT_APPLIED;
}

// 0x7c DimensionDoor
// iwd2 has several options
int fx_dimension_door (Scriptable* Owner, Actor* target, Effect* fx)
{
	// print("fx_dimension_door(%2d) Type:%d", fx->Opcode, fx->Parameter2);
	Point p;

	switch(fx->Parameter2)
	{
	case 0: //target to point
		p = fx->Pos;
		break;
	case 1: //owner to target
		if (Owner->Type!=ST_ACTOR) {
			return FX_NOT_APPLIED;
		}
		p=target->Pos;
		target = (Actor *) Owner;
		break;
	case 2: //target to saved location
		p.x=STAT_GET(IE_SAVEDXPOS);
		p.y=STAT_GET(IE_SAVEDYPOS);
		target->SetOrientation(ClampToOrientation(STAT_GET(IE_SAVEDFACE)), false);
		break;
	case 3: //owner swapped with target
		if (Owner->Type!=ST_ACTOR) {
			return FX_NOT_APPLIED;
		}
		p=target->Pos;
		target->SetPosition(Owner->Pos, true);
		target = (Actor *) Owner;
		break;
	}
	target->SetPosition(p, true);
	return FX_NOT_APPLIED;
}

// 0x7d Unlock
int fx_knock (Scriptable* Owner, Actor* /*target*/, Effect* fx)
{
	// print("fx_knock(%2d) [%d.%d]", fx->Opcode, fx->PosX, fx->PosY);
	const Map *map = Owner->GetCurrentArea();
	if (!map) {
		return FX_NOT_APPLIED;
	}

	Door *door = map->TMap->GetDoorByPosition(fx->Pos);
	if (door) {
		if (door->LockDifficulty<100) {
			door->SetDoorLocked(false, true);
		}
		return FX_NOT_APPLIED;
	}
	Container *container = map->TMap->GetContainerByPosition(fx->Pos);
	if (container) {
		if(container->LockDifficulty<100) {
			container->SetContainerLocked(false);
		}
		return FX_NOT_APPLIED;
	}
	return FX_NOT_APPLIED;
}

// 0x7e MovementRateModifier
// 0xb0 MovementRateModifier2
int fx_movement_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_movement_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	//iwd2 freeaction disables only 0xb0, who cares
	if (target->HasSpellState(SS_FREEACTION)) return FX_NOT_APPLIED;
	//iwd2 aegis doesn't protect against grease/acid fog slowness, but that is
	//definitely a bug
	if (target->HasSpellState(SS_AEGIS)) return FX_NOT_APPLIED;
	if (core->InCutSceneMode()) return FX_APPLIED;

	// bg1 crashes on 13+, 9&10 are equal to 8 and 11 (boots of speed) equals to roughly 15.6 #129
	if (core->HasFeature(GFFlags::BREAKABLE_WEAPONS) && fx->Parameter2 == MOD_ABSOLUTE) {
		switch (fx->Parameter1) {
			case 9:
			case 10:
				fx->Parameter1 = 8;
				break;
			case 11:
			case 30:
				fx->Parameter1 = 15;
				break;
			default:
				break;
		}
	}

	ieDword value = target->GetStat(IE_MOVEMENTRATE);
	STAT_MOD(IE_MOVEMENTRATE);
	if (value < target->GetStat(IE_MOVEMENTRATE)) {
		target->AddPortraitIcon(PI_HASTED);
	}
	return FX_APPLIED;
}

// 0x7f MonsterSummoning
int fx_monster_summoning (Scriptable* Owner, Actor* target, Effect* fx)
{
	// print("fx_monster_summoning(%2d): Number: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	//check the summoning limit?
	if (!Owner) {
		return FX_NOT_APPLIED;
	}

	if (!Owner->GetCurrentArea()) {
		return FX_APPLIED;
	}

	//get monster resref from 2da determined by fx->Resource or fx->Parameter2
	ResRef monster;
	ResRef hit;
	ResRef areahit;
	ResRef table;
	int level = fx->Parameter1;
	static const std::array<ResRef, 10> monster_summoning_2da = { "MONSUM01", "MONSUM02", "MONSUM03",
		"ANISUM01", "ANISUM02", "MONSUM01", "MONSUM02", "MONSUM03", "ANISUM01", "ANISUM02" };

	if (!fx->Resource.IsEmpty()) {
		table = fx->Resource;
	} else {
		if (fx->Parameter2 >= monster_summoning_2da.size()) {
			table = "ANISUM03";
		} else {
			table = monster_summoning_2da[fx->Parameter2];
		}
	}
	core->GetResRefFrom2DA(table, monster, hit, areahit);

	if (hit.IsEmpty()) {
		hit = fx->Resource2;
	}

	Effect *newfx = EffectQueue::CreateUnsummonEffect(fx);
	//The hostile flag should cover these cases, all else is arbitrary
	//0,1,2,3,4 - friendly to target
	//5,6,7,8,9 - hostile to target
	//10        - friendly to target

	int eamod;
	if (fx->Parameter2>=5 && fx->Parameter2<=9) {
		eamod = EAM_ENEMY;
	}
	else {
		eamod = EAM_ALLY;
	}

	//caster may be important here (Source), even if currently the EA modifiers
	//don't use it
	Scriptable *caster = GetCasterObject();
	//the monster should appear near the effect position
	core->SummonCreature(monster, hit, caster, target, fx->Pos, eamod, level, newfx);
	return FX_NOT_APPLIED;
}

// 0x80 State:Confused
int fx_set_confused_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_set_confused_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (target->HasSpellState(SS_BLOODRAGE)) {
		return FX_NOT_APPLIED;
	}

	if (fx->TimingMode==FX_DURATION_DELAY_PERMANENT) {
		BASE_STATE_SET( STATE_CONFUSED );
	} else {
		STATE_SET( STATE_CONFUSED );
	}
	//NOTE: iwd2 is also unable to display the portrait icon
	//for permanent confusion
	//the portrait icon cannot be made common because rigid thinking uses a different icon
	//in bg2/how
	if (core->HasFeature(GFFlags::ENHANCED_EFFECTS)) {
		target->AddPortraitIcon(PI_CONFUSED);
	}
	return FX_PERMANENT;
}

// 0x81 AidNonCumulative
int fx_set_aid_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_set_aid_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	if (!fx->Parameter2) {
		fx->Parameter2=core->Roll(fx->Parameter1,8,0);
	}
	if (STATE_GET (STATE_AID) ) //aid is non cumulative
		return FX_NOT_APPLIED;
	STATE_SET( STATE_AID );
	target->SetSpellState(SS_AID);
	STAT_ADD( IE_MAXHITPOINTS, fx->Parameter2);
	//This better happens after increasing maxhitpoints
	if (fx->FirstApply) {
		BASE_ADD(IE_HITPOINTS, fx->Parameter2);
	}
	HandleBonus(target, IE_SAVEVSDEATH, fx->Parameter1, fx->TimingMode);
	HandleBonus(target, IE_SAVEVSWANDS, fx->Parameter1, fx->TimingMode);
	HandleBonus(target, IE_SAVEVSPOLY, fx->Parameter1, fx->TimingMode);
	HandleBonus(target, IE_SAVEVSBREATH, fx->Parameter1, fx->TimingMode);
	HandleBonus(target, IE_SAVEVSSPELL, fx->Parameter1, fx->TimingMode);

	//bless effect too?
	target->ToHit.HandleFxBonus(fx->Parameter1, fx->TimingMode==FX_DURATION_INSTANT_PERMANENT);
	STAT_ADD( IE_DAMAGEBONUS, fx->Parameter1);
	if (core->HasFeature(GFFlags::ENHANCED_EFFECTS)) {
		target->AddPortraitIcon(PI_AID);
		target->SetColorMod(0xff, RGBModifier::ADD, 30, Color(50, 50, 50, 0));
	}
	return FX_APPLIED;
}

// 0x82 BlessNonCumulative
int fx_set_bless_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_set_bless_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (STATE_GET (STATE_BLESS) ) //bless is non cumulative
		return FX_NOT_APPLIED;

	//do this once
	if (fx->FirstApply) {
		target->fxqueue.RemoveAllEffects(fx_bane_ref);
	}

	STATE_SET( STATE_BLESS );
	target->SetSpellState(SS_BLESS);
	target->ToHit.HandleFxBonus(fx->Parameter1, fx->TimingMode==FX_DURATION_INSTANT_PERMANENT);
	STAT_ADD( IE_DAMAGEBONUS, fx->Parameter1);
	if (target->ShouldModifyMorale()) STAT_ADD(IE_MORALE, fx->Parameter1);
	if (core->HasFeature(GFFlags::ENHANCED_EFFECTS)) {
		target->AddPortraitIcon(PI_BLESS);
		target->SetColorMod(0xff, RGBModifier::ADD, 30, Color(0xc0, 0x80, 0, 0));
	}
	return FX_APPLIED;
}
// 0x83 ChantNonCumulative
int fx_set_chant_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_set_chant_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (STATE_GET (STATE_CHANT) ) //chant is non cumulative
		return FX_NOT_APPLIED;
	STATE_SET( STATE_CHANT );
	target->SetSpellState(SS_GOODCHANT);
	STAT_ADD( IE_LUCK, fx->Parameter1 );
	return FX_APPLIED;
}

// 0x84 HolyNonCumulative
int fx_set_holy_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_set_holy_state(%2d): Modifier: %d", fx->Opcode, fx->Parameter1);

	if (STATE_GET (STATE_HOLY) ) //holy power is non cumulative
		return FX_NOT_APPLIED;
	STATE_SET( STATE_HOLY );
	//setting the spell state to be compatible with iwd2
	target->SetSpellState(SS_HOLYMIGHT);
	STAT_ADD( IE_STR, fx->Parameter1);
	STAT_ADD( IE_CON, fx->Parameter1);
	STAT_ADD( IE_DEX, fx->Parameter1);
	if (core->HasFeature(GFFlags::ENHANCED_EFFECTS)) {
		target->AddPortraitIcon(PI_HOLY);
		target->SetColorMod(0xff, RGBModifier::ADD, 30, Color(0x80, 0x80, 0x80, 0));
	}
	return FX_APPLIED;
}

// 0x85 LuckNonCumulative
int fx_luck_non_cumulative (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_luck_non_cumulative(%2d): Modifier: %d", fx->Opcode, fx->Parameter1);

	if (STATE_GET (STATE_LUCK) ) //this luck is non cumulative
		return FX_NOT_APPLIED;
	STATE_SET( STATE_LUCK );
	target->SetSpellState(SS_LUCK);
	STAT_ADD( IE_LUCK, fx->Parameter1 );
	//no, this isn't in BG2, this modifies only the state bitfield and the stat
	//STAT_ADD( IE_DAMAGELUCK, fx->Parameter1 );
	return FX_APPLIED;
}

// 0x85 LuckCumulative (iwd2)
int fx_luck_cumulative (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_luck_cumulative(%2d): Modifier: %d", fx->Opcode, fx->Parameter1);

	target->SetSpellState(SS_LUCK);
	STAT_ADD( IE_LUCK, fx->Parameter1 );
	//TODO:check this in IWD2
	STAT_ADD( IE_DAMAGELUCK, fx->Parameter1 );
	return FX_APPLIED;
}

// 0x86 State:Petrification
int fx_set_petrified_state (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_set_petrified_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (target->GetStat(IE_EXTSTATE_ID) & EXTSTATE_EYE_STONE) {
		target->fxqueue.RemoveAllEffects(fx_eye_stone_ref);
		target->spellbook.RemoveSpell(SevenEyes[EYE_STONE]);
		target->SetBaseBit(IE_EXTSTATE_ID, EXTSTATE_EYE_STONE, false);
		return FX_ABORT;
	}

	BASE_STATE_SET( STATE_PETRIFIED );
	if (target->InParty) core->GetGame()->LeaveParty(target);
	target->SendDiedTrigger();

	// end the game if everyone in the party gets petrified
	const Game *game = core->GetGame();
	int partySize = game->GetPartySize(true);
	int stoned = 0;
	for (int j=0; j<partySize; j++) {
		const Actor *pc = game->GetPC(j, true);
		if (pc->GetStat(IE_STATE_ID) & STATE_PETRIFIED) stoned++;
	}
	if (stoned == partySize) {
		core->GetGUIScriptEngine()->RunFunction("GUIWORLD", "DeathWindowPlot", false);
	}

	//always permanent effect, in fact in the original it is a death opcode (Avenger)
	//i just would like this to be less difficult to use in mods (don't destroy petrified creatures)
	return FX_NOT_APPLIED;
}

class PolymorphStats {
	PolymorphStats() {
		AutoTable tab = gamedata->LoadTable("polystat");
		if (!tab) {
			return;
		}
		data.resize(tab->GetRowCount());
		TableMgr::index_t rowCount = static_cast<TableMgr::index_t>(data.size());
		for (TableMgr::index_t i = 0; i < rowCount; ++i) {
			data[i] = core->TranslateStat(tab->QueryField(i, 0));
		}
	}
	
public:
	static const PolymorphStats& Get() {
		static PolymorphStats stats;
		return stats;
	}
	
	std::vector<int> data;
};

// 0x87 Polymorph
static void CopyPolymorphStats(Actor *source, Actor *target)
{
	assert(target->polymorphCache);
	
	const auto& polystats = PolymorphStats::Get().data;

	if (target->polymorphCache->stats.empty()) {
		target->polymorphCache->stats.resize(polystats.size());
	}

	for (size_t i = 0; i < polystats.size(); ++i) {
		target->polymorphCache->stats[i] = source->Modified[polystats[i]];
	}
}

int fx_polymorph (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_set_polymorph_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (!gamedata->Exists(fx->Resource,IE_CRE_CLASS_ID)) {
		//kill all polymorph effects
		target->fxqueue.RemoveAllEffectsWithParam(fx_polymorph_ref, fx->Parameter2);
		//destroy the magic item slot
		target->inventory.RemoveItem(Inventory::GetMagicSlot());
		return FX_NOT_APPLIED;
	}

	// kill all other polymorph effects
	if (fx->FirstApply) {
		target->fxqueue.RemoveAllEffects(fx_polymorph_ref);
	}

	// to avoid repeatedly loading the file or keeping all the data around
	// wasting memory, we keep a PolymorphCache object around, with only
	// the data we need from the polymorphed creature
	bool cached = true;
	if (!target->polymorphCache) {
		cached = false;
		target->polymorphCache = new PolymorphCache();
	}
	if (!cached || fx->Resource != target->polymorphCache->Resource) {
		Actor *newCreature = gamedata->GetCreature(fx->Resource,0);

		// I don't know how could this happen, existence of the resource was already checked
		if (!newCreature) {
			return FX_NOT_APPLIED;
		}

		target->polymorphCache->Resource = fx->Resource;
		CopyPolymorphStats(newCreature, target);

		delete newCreature;
	}

	//copy all polymorphed stats
	if(!fx->Parameter2) {
		STAT_SET( IE_POLYMORPHED, 1 );
		//disable mage and cleric spells (see IE_CASTING doc above)
		STAT_BIT_OR(IE_CASTING, 6);
		STAT_BIT_OR(IE_DISABLEDBUTTON, (1<<ACT_CAST)|(1<<ACT_QSPELL1)|(1<<ACT_QSPELL2)|(1<<ACT_QSPELL3) );
	}

	if (fx->Parameter2) {
		// copy only the animation ID (line 23 in polystat.2da)
		target->SetStat(IE_ANIMATION_ID, target->polymorphCache->stats[23], 1);
	} else {
		const auto& polystats = PolymorphStats::Get().data;
		for (size_t i = 0; i < polystats.size(); ++i) {
			target->SetStat(polystats[i], target->polymorphCache->stats[i], 1);
		}
	}

	return FX_APPLIED;
}

// 0x88 ForceVisible
int fx_force_visible (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_force_visible(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (core->HasFeature(GFFlags::PST_STATE_FLAGS)) {
		BASE_STATE_CURE(STATE_PST_INVIS);
	} else {
		BASE_STATE_CURE(STATE_INVISIBLE);
	}
	target->fxqueue.RemoveAllEffectsWithParam(fx_set_invisible_state_ref,0);
	target->fxqueue.RemoveAllEffectsWithParam(fx_set_invisible_state_ref,2);

	//fix the hiding puppet while mislead bug, by
	if (target->GetSafeStat(IE_PUPPETTYPE)==1) {
		//hack target to use the plain type
		target->Modified[IE_PUPPETTYPE]=0;

		//go after the original puppetmarker in the puppet too
		Actor *puppet = core->GetGame()->GetActorByGlobalID(target->GetSafeStat(IE_PUPPETID) );
		if (puppet) {
			Effect *puppetmarker = puppet->fxqueue.HasEffect(fx_puppetmarker_ref);

			//hack the puppet type to normal, where it doesn't grant invisibility
			if (puppetmarker) {
				puppetmarker->Parameter2 = 0;
			}
		}
	}
	return FX_NOT_APPLIED;
}

// 0x89 ChantBadNonCumulative
int fx_set_chantbad_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_set_chantbad_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (STATE_GET (STATE_CHANTBAD) ) //chant is non cumulative
		return FX_NOT_APPLIED;
	STATE_SET( STATE_CHANTBAD );
	target->SetSpellState(SS_BADCHANT);
	STAT_SUB( IE_LUCK, fx->Parameter1 );
	return FX_APPLIED;
}

// 0x8A AnimationStateChange
int fx_animation_stance (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_animation_stance(%2d): Stance: %d", fx->Opcode, fx->Parameter2);

	//this effect works only on living actors
	if ( !STATE_GET(STATE_DEAD) ) {
		target->SetStance(fx->Parameter2);
	}
	return FX_NOT_APPLIED;
}

// 0x8B DisplayString
// gemrb extension: resource may be an strref list (src or 2da)

int fx_display_string (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_display_string(%2d): StrRef: %d", fx->Opcode, fx->Parameter1);
	if (STATE_GET(STATE_DEAD) ) {
		return FX_NOT_APPLIED;
	}

	if (!fx->Resource.IsEmpty()) {
		const SrcVector* strList = gamedata->SrcManager.GetSrc(fx->Resource);
		if (!strList->IsEmpty()) {
			ieStrRef str = strList->RandomRef();
			// mrttaunt.src has placeholder text, but valid audio, so enable just sound
			if (fx->IsVariable) {
				StringBlock sb = core->strings->GetStringBlock(str);
				core->GetAudioDrv()->Play(StringView(sb.Sound), SFX_CHAN_ACTIONS, target->Pos);
			} else {
				fx->Parameter1 = ieDword(str);
				DisplayStringCore(target, str, DS_HEAD);
				target->overColor = Color(fx->Parameter2);
			}
			return FX_NOT_APPLIED;
		}

		//random text for other games
		const auto rndstr2 = core->GetListFrom2DA(fx->Resource);
		if (!rndstr2->empty()) {
			fx->Parameter1 = rndstr2->at(RAND<size_t>(0, rndstr2->size() - 1));
		}
	}

	if (!target->fxqueue.HasEffectWithParamPair(fx_protection_from_display_string_ref, fx->Parameter1, 0) ) {
		displaymsg->DisplayStringName(ieStrRef(fx->Parameter1), GUIColors::WHITE, target, STRING_FLAGS::SOUND | STRING_FLAGS::SPEECH);
	}
	return FX_NOT_APPLIED;
}

// 0x8c CastingGlow
// the originals actually spawned a projectile to do the work
// it was unhardcoded in bg2ee or iwdee
int fx_casting_glow (Scriptable* Owner, Actor* target, Effect* fx)
{
	if (fx->Parameter2 < gamedata->castingGlows.size()) {
		// check if we're in SpellCastEffect mode for iwd2
		ResRef& animRef = gamedata->castingGlows[fx->Parameter2];
		if (fx->Parameter4) {
			animRef = gamedata->castingHits[fx->Parameter2];
		}

		ScriptedAnimation* sca = gamedata->GetScriptedAnimation(animRef, false);
		//remove effect if animation doesn't exist
		if (!sca) {
			return FX_NOT_APPLIED;
		}
		// as per the original bg2 code, should we externalize?
		// only dragons got a different x, y, z offset (x and y being handled in the projectile)
		int heightmod = core->HasFeature(GFFlags::PST_STATE_FLAGS) ? ProHeights::None : ProHeights::Normal;
		if (target->ValidTarget(GA_BIGBAD)) {
			heightmod = ProHeights::Dragon;
		}
		Point offset = Projectile::GetStartOffset(target);
		sca->XOffset += offset.x;
		sca->YOffset += offset.y;
		sca->ZOffset = heightmod;
		sca->SetBlend();
		if (fx->Duration) {
			sca->SetDefaultDuration(fx->Duration-core->GetGame()->GameTime);
		} else {
			sca->SetDefaultDuration(10000);
		}
		
		sca->SequenceFlags |= IE_VVC_STATIC;
		target->AddVVCell(sca);
	} else {
		//simulate sparkle casting glows
		target->ApplyEffectCopy(fx, fx_sparkle_ref, Owner, fx->Parameter2, 3);
	}
	// TODO: this opcode is also affected by slow/haste
	return FX_NOT_APPLIED;
}

//0x8d VisualSpellHit
int fx_visual_spell_hit (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_visual_spell_hit(%2d): Target: %d Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	if (gamedata->spellHits.empty()) {
		gamedata->ReadResRefTable(ResRef("shtable"), gamedata->spellHits);
	}
	//remove effect if map is not loaded
	Map *map = target->GetCurrentArea();
	if (!map) {
		return FX_NOT_APPLIED;
	}
	if (fx->Parameter2 < gamedata->spellHits.size()) {
		ScriptedAnimation *sca = gamedata->GetScriptedAnimation(gamedata->spellHits[fx->Parameter2], false);
		//remove effect if animation doesn't exist
		if (!sca) {
			return FX_NOT_APPLIED;
		}
		if (fx->Parameter1) {
			sca->Pos = target->Pos;
		} else {
			sca->Pos = fx->Pos;
		}
		sca->ZOffset += 45; // roughly half the target height; empirical value to match original
		if (fx->Parameter2<32) {
			int tmp = fx->Parameter2>>2;
			if (tmp) {
				sca->SetFullPalette(tmp);
			}
		}
		sca->SetBlend();
		sca->PlayOnce();
		map->AddVVCell(sca);
	} else {
		Log(ERROR, "FXOpcodes", "fx_visual_spell_hit: Unhandled Type: {}", fx->Parameter2);
	}
	return FX_NOT_APPLIED;
}

//0x8e Icon:Display
int fx_display_portrait_icon (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	static EffectRef fx_disable_portrait_icon_ref = { "Icon:Disable", -1 };
	if (!target->fxqueue.HasEffectWithParam(fx_disable_portrait_icon_ref, fx->Parameter2)) {
		target->AddPortraitIcon(fx->Parameter2);
	}
	return FX_APPLIED;
}

//0x8f Item:CreateInSlot
int fx_create_item_in_slot (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_create_item_in_slot(%2d): Button: %d", fx->Opcode, fx->Parameter2);
	//create item and set it in target's slot
	target->inventory.SetSlotItemRes( fx->Resource, core->QuerySlot(fx->Parameter2), fx->Parameter1, fx->Parameter3, fx->Parameter4 );
	return MaybeTransformTo(fx_remove_item_ref, fx);
}

// 0x90 DisableButton
// different in iwd2 and the rest (maybe also in how: 0-7?)
int fx_disable_button (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_disable_button(%2d): Button: %d", fx->Opcode, fx->Parameter2);

	// iwd2 has a flexible action bar, so there are more possible parameter values
	// only values 0-5 match the bg2 constants (which map to ACT_*)
	// FIXME: support disabling all iwd2 buttons
	if (target->spellbook.IsIWDSpellBook()) {
		if (fx->Parameter2 < 6) STAT_BIT_OR( IE_DISABLEDBUTTON, 1<<fx->Parameter2 );
	} else {
		STAT_BIT_OR( IE_DISABLEDBUTTON, 1<<fx->Parameter2 );
	}
	if (fx->FirstApply && target->GetStat(IE_EA) < EA_CONTROLLABLE) {
		core->SetEventFlag(EF_ACTION);
	}
	return FX_APPLIED;
}

//0x91 DisableSpellCasting
//bg2:  (-1 item), 0 - mage, 1 - cleric, 2 - innate, 3 - class
//iwd2: (-1 item), 0 - all, 1 - mage+cleric, 2 - mage, 3 - cleric , 4 - innate,( 5 - class)
// class spells here are only 'magical' abilities (flags bit SF_HLA "Ignore dead/wild magic" unset)

/*internal representation of disabled spells in IE_CASTING (bitfield):
1 - items (SPIT)
2 - cleric (SPPR)
4 - mage  (SPWI)
8 - innate (SPIN)
16 - class (SPCL)
*/

int fx_disable_spellcasting (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_disable_spellcasting(%2d): Button: %d", fx->Opcode, fx->Parameter2);

	static const ieDword dsc_bits_iwd2[7] = { 1, 14, 6, 2, 4, 8, 16 };
	static const ieDword dsc_bits_bg2[7] = { 1, 4, 2, 8, 16, 14, 6 };
	static const int mageBooks = 1 << IE_IWD2_SPELL_BARD | 1 << IE_IWD2_SPELL_SORCERER | 1 << IE_IWD2_SPELL_WIZARD;
	bool displayWarning = false;
	ieDword tmp = fx->Parameter2+1;

	//IWD2 Style spellbook
	if (target->spellbook.IsIWDSpellBook()) {
		int bookMask = target->GetBookMask();
		 // is there a potential mage spellbook involved?
		if (fx->Parameter2 <= 2 && bookMask & mageBooks) {
			displayWarning = true;
		}
		if (tmp<7) {
			STAT_BIT_OR(IE_CASTING, dsc_bits_iwd2[tmp] );
		}
	} else { // bg2
		if (fx->Parameter2 == 0 && target->spellbook.GetKnownSpellsCount(IE_SPELL_TYPE_WIZARD, 0)) {
			displayWarning = true;
		}
		//-1->  1 (item)
		//0 ->  4 (mage)
		//1 ->  2 (cleric)
		//2 ->  8 (innate)
		//3 ->  16 (class)
		if (tmp<7) {
			STAT_BIT_OR(IE_CASTING, dsc_bits_bg2[tmp] );
		}
	}
	if (fx->FirstApply && target->GetStat(IE_EA) < EA_CONTROLLABLE) {
		if (displayWarning) displaymsg->DisplayConstantStringName(HCStrings::DisabledMageSpells, GUIColors::RED, target);
		core->SetEventFlag(EF_ACTION);
	}
	return FX_APPLIED;
}

//0x92 Spell:Cast
int fx_cast_spell (Scriptable* Owner, Actor* target, Effect* fx)
{
	// print("fx_cast_spell(%2d): Resource:%s Mode: %d", fx->Opcode, fx->Resource, fx->Parameter2);
	if (Owner->Type == ST_ACTOR) {
		const Actor *owner = (const Actor *) Owner;
		// prevent eg. True Sight continuing after death
		if (!owner->ValidTarget(GA_NO_DEAD)) {
			return FX_NOT_APPLIED;
		}
	}

	if (fx->Parameter2 == 0 || target->Type == ST_CONTAINER) {
		// no deplete, no interrupt, caster or provided level
		std::string tmp = fmt::format("ForceSpellRES(\"{}\",[-1],{})", fx->Resource, fx->Parameter1);
		Action* forceSpellAction = GenerateActionDirect(std::move(tmp), target);
		Owner->AddActionInFront(forceSpellAction);
		Owner->ImmediateEvent();
	} else if (fx->Parameter2 == 1) {
		// no deplete, instant, no interrupt, caster level
		ResRef OldSpellResRef(Owner->SpellResRef);
		Owner->DirectlyCastSpell(target, fx->Resource, fx->CasterLevel, true, false);
		Owner->SetSpellResRef(OldSpellResRef);
	} else { // ees introduce 2
		// no deplete, instant, no interrupt, provided level
		ResRef OldSpellResRef(Owner->SpellResRef);
		Owner->DirectlyCastSpell(target, fx->Resource, fx->Parameter1, true, false);
		Owner->SetSpellResRef(OldSpellResRef);
	}

	return FX_NOT_APPLIED;
}

// 0x93 Spell:Learn
int fx_learn_spell (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	//parameter1 is unused, gemrb lets you to make it not give XP
	//probably we should also let this via a game flag if we want
	//full compatibility with bg1
	//parameter2 is used in bg1 and pst to specify the spell type; bg2 and iwd2 figure it out from the resource
	//LS_STATS is on by default (int check)
	target->LearnSpell(fx->Resource, fx->Parameter1 ^ LS_STATS);
	return FX_NOT_APPLIED;
}
// 0x94 Spell:CastSpellPoint
int fx_cast_spell_point (Scriptable* Owner, Actor* /*target*/, Effect* fx)
{
	if (fx->Parameter2 == 0) {
		// no deplete, no interrupt, caster or provided level
		std::string tmp = fmt::format("ForceSpellPointRES(\"{}\",[{}.{}],{})", fx->Resource, fx->Pos.x, fx->Pos.y, fx->Parameter1);
		Action* forceSpellAction = GenerateAction(std::move(tmp));
		Owner->AddActionInFront(forceSpellAction);
		Owner->ImmediateEvent();
	} else if (fx->Parameter2 == 1) {
		// no deplete, instant, no interrupt, caster level
		ResRef OldSpellResRef(Owner->SpellResRef);
		Owner->DirectlyCastSpellPoint(fx->Pos, fx->Resource, fx->CasterLevel, true, false);
		Owner->SetSpellResRef(OldSpellResRef);
	} else { // gemrb extension to mirror fx_cast_spell
		// no deplete, instant, no interrupt, provided level
		ResRef OldSpellResRef(Owner->SpellResRef);
		Owner->DirectlyCastSpellPoint(fx->Pos, fx->Resource, fx->Parameter1, true, false);
		Owner->SetSpellResRef(OldSpellResRef);
	}

	return FX_NOT_APPLIED;
}

// 0x95 Identify
int fx_identify (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_identify(%2d): Resource:%s Mode: %d", fx->Opcode, fx->Resource, fx->Parameter2);
	if (target->InParty) {
		BASE_SET (IE_IDENTIFYMODE, 1);
		core->SetEventFlag(EF_IDENTIFY);
	}
	return FX_NOT_APPLIED;
}
// 0x96 FindTraps
// (actually, in bg2 the effect targets area objects and the range is implemented
// by the inareans projectile) - inanimate, area, no sprite
// TODO: effects should target inanimates using different code
// 0 - detect traps automatically
// 1 - detect traps by skill
// 2 - detect secret doors automatically
// 3 - detect secret doors by luck
int fx_find_traps (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_find_traps(%2d)", fx->Opcode);
	//reveal trapped containers, doors, triggers that are in the visible range
	ieDword id = target->GetGlobalID();
	ieDword range = target->GetStat(IE_VISUALRANGE) / 2;
	ieDword skill;
	bool detecttraps = true;

	switch(fx->Parameter2) {
		case 1:
			//find traps
			skill = target->GetStat(IE_TRAPS);
			break;
		case 3:
			//detect secret doors
			//FIXME: is 1d100 the real base value?
			skill = target->LuckyRoll(1, 100, 0, 0)+core->ResolveStatBonus(target, "dstable");
			detecttraps = false;
			break;
		case 2:
			//automatic secret door detection
			detecttraps = false;
			//intentional fallthrough
		default:
			//automatic find traps
			skill = 256;
			break;
	}

	const TileMap *TMap = target->GetCurrentArea()->TMap;

	size_t Count = 0;
	while (true) {
		Door* door = TMap->GetDoor( Count++ );
		if (!door)
			break;
		if (WithinRange(target, door->Pos, range)) {
			door->TryDetectSecret(skill, id);
			if (detecttraps && door->Visible()) {
			//when was door trap noticed
				door->DetectTrap(skill, id);
			}
		}
	}

	if (!detecttraps) {
		return FX_NOT_APPLIED;
	}

	Count = 0;
	while (true) {
		Container* container = TMap->GetContainer( Count++ );
		if (!container)
			break;
		if (WithinRange(target, container->Pos, range)) {
			//when was door trap noticed
			container->DetectTrap(skill, id);
		}
	}


	Count = 0;
	while (true) {
		InfoPoint* trap = TMap->GetInfoPoint( Count++ );
		if (!trap)
			break;
		if (WithinRange(target, trap->Pos, range)) {
			//when was door trap noticed
			trap->DetectTrap(skill, id);
		}
	}

	return FX_NOT_APPLIED;
}

// 0x97 ReplaceCreature
int fx_replace_creature (Scriptable* Owner, Actor* target, Effect *fx)
{
	// print("fx_replace_creature(%2d): Resource: %s", fx->Opcode, fx->Resource);

	//this safeguard exists in the original engine too
	if (!gamedata->Exists(fx->Resource,IE_CRE_CLASS_ID)) {
		return FX_NOT_APPLIED;
	}

	//remove old creature
	switch(fx->Parameter2) {
	case 0: //remove silently
		target->DestroySelf();
		break;
	case 1: //chunky death
		target->LastDamageType |= DAMAGE_CHUNKING;
		target->NewBase(IE_HITPOINTS, (Actor::stat_t) -100, MOD_ABSOLUTE);
		target->Die(Owner);
		// we also have to remove any party members or their corpses will stay around
		if (target->InParty) {
			int slot = core->GetGame()->LeaveParty(target);
			core->GetGame()->DelNPC(slot);
			target->SetPersistent(-1);
		}
		target->SetBase(IE_MC_FLAGS, target->GetBase(IE_MC_FLAGS) & ~MC_KEEP_CORPSE);
		break;
	case 2: //normal death
		target->Die(Owner);
		break;
	default:;
	}
	// don't unsummon replacement creatures
	// the monster should appear near the effect position
	core->SummonCreature(fx->Resource, fx->Resource2, Owner, nullptr, fx->Pos, EAM_DEFAULT, -1, nullptr, false);
	return FX_NOT_APPLIED;
}

// 0x98 PlayMovie
int fx_play_movie (Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	// print("fx_play_movie(%2d): Resource: %s", fx->Opcode, fx->Resource);
	core->PlayMovie(fx->Resource);
	return FX_NOT_APPLIED;
}
// 0x99 Overlay:Sanctuary
// iwd and bg are a bit different, but we solve the whole stuff in a single opcode
int fx_set_sanctuary_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_set_sanctuary_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	static const ieDword fullwhite[7] = { ICE_GRADIENT, ICE_GRADIENT, ICE_GRADIENT, ICE_GRADIENT, ICE_GRADIENT, ICE_GRADIENT, ICE_GRADIENT };

	// don't set the state twice
	// SetSpellState will also check if it is already set first
	if (target->SetSpellState(SS_SANCTUARY)) return FX_NOT_APPLIED;

	if (!fx->Parameter2) {
		fx->Parameter2=1;
	}
	// this effect needs the pcf to be ran immediately, but we do it manually
	STAT_SET(IE_SANCTUARY, fx->Parameter2);
	//a rare event, but this effect gives more in bg2 than in iwd2
	//so we use this flag
	if (!core->HasFeature(GFFlags::ENHANCED_EFFECTS)) {
		target->SetLockedPalette(fullwhite);
	}
	return FX_APPLIED;
}

// 0x9a Overlay:Entangle
int fx_set_entangle_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_set_entangle_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	//iwd2 effects that disable entangle
	if (target->HasSpellState(SS_FREEACTION)) return FX_NOT_APPLIED;
	if (target->HasSpellState(SS_AEGIS)) return FX_NOT_APPLIED;

	if (!fx->Parameter2) {
		fx->Parameter2=1;
	}
	STAT_SET_PCF( IE_ENTANGLE, fx->Parameter2);
	return FX_APPLIED;
}

// 0x9b Overlay:MinorGlobe
int fx_set_minorglobe_state (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_set_minorglobe_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	//the globe vanishes on death
	if (STATE_GET(STATE_DEAD) ) {
		return FX_NOT_APPLIED;
	}
	//the resisted levels are stored in minor globe (bit 2-)
	//the globe effect is stored in the first bit
	STAT_BIT_OR_PCF( IE_MINORGLOBE, 1);
	return FX_APPLIED;
}

// 0x9c Overlay:ShieldGlobe
int fx_set_shieldglobe_state (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_set_shieldglobe_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	//the shield vanishes on dead
	if (STATE_GET(STATE_DEAD) ) {
		return FX_NOT_APPLIED;
	}
	STAT_SET_PCF( IE_SHIELDGLOBE, 1);
	return FX_APPLIED;
}

// 0x9d Overlay:Web
int fx_set_web_state (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_set_web_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	//iwd2 effects that disable web
	if (target->HasSpellState(SS_FREEACTION)) return FX_NOT_APPLIED;
	if (target->HasSpellState(SS_AEGIS)) return FX_NOT_APPLIED;

	target->SetSpellState(SS_WEB);
	//attack penalty in IWD2
	STAT_SET_PCF( IE_WEB, 1);
	STAT_SET(IE_HELD, 1);
	STATE_SET(STATE_HELPLESS);
	return FX_APPLIED;
}

// 0x9e Overlay:Grease
int fx_set_grease_state (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_set_grease_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	//iwd2 effects that disable grease
	if (target->HasSpellState(SS_FREEACTION)) return FX_NOT_APPLIED;
	if (target->HasSpellState(SS_AEGIS)) return FX_NOT_APPLIED;

	target->SetSpellState(SS_GREASE);
	STAT_SET_PCF( IE_GREASE, 1);
	//the movement rate is set by separate opcodes in all engines
	return FX_APPLIED;
}

// 0x9f MirrorImageModifier
int fx_mirror_image_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_mirror_image_modifier(%2d): Mod: %d", fx->Opcode, fx->Parameter1);
	if (STATE_GET(STATE_DEAD) ) {
		return FX_NOT_APPLIED;
	}
	if (!fx->Parameter1) {
		return FX_NOT_APPLIED;
	}
	if (core->HasFeature(GFFlags::PST_STATE_FLAGS)) {
		STATE_SET( STATE_PST_MIRROR );
	}
	else {
		STATE_SET( STATE_MIRROR );
	}
	if (fx->Parameter2) {
		target->SetSpellState(SS_REFLECTION);
	} else {
		target->SetSpellState(SS_MIRRORIMAGE);
	}
	//actually, there is no such stat in the original IE
	STAT_SET( IE_MIRRORIMAGES, fx->Parameter1);
	return FX_APPLIED;
}

// 0xa0 Cure:Sanctuary

int fx_cure_sanctuary_state (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_cure_sanctuary_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	STAT_SET( IE_SANCTUARY, 0);
	target->fxqueue.RemoveAllEffects(fx_sanctuary_state_ref);
	return FX_NOT_APPLIED;
}

// 0xa1 Cure:Panic

int fx_cure_panic_state (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_cure_panic_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	BASE_STATE_CURE( STATE_PANIC );
	target->fxqueue.RemoveAllEffects(fx_set_panic_state_ref);
	return FX_NOT_APPLIED;
}

// 0xa2 Cure:Hold
 int fx_cure_hold_state (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_cure_hold_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	//note that this effect doesn't remove 185 (another hold effect)
	target->fxqueue.RemoveAllEffects( fx_hold_creature_ref );
	target->fxqueue.RemoveAllEffects(fx_hold_creature_no_icon_ref);
	target->fxqueue.RemoveAllEffectsWithParam(fx_display_portrait_icon_ref, PI_HELD);
	return FX_NOT_APPLIED;
}

// 0xa3 FreeAction
// if needed, make this match bg2 behaviour more closely: it removed 0x7e (126) effects with duration/permanent
// and param2 == MOD_ABSOLUTE IF the modified movement rate is lower than the original. This is, of course,
// crap. It should remove individual 0x7e effects if they lower the movement rate (or remove all of them, which
// is what we do now).
// NOTE: it didn't remove 0xb0 effects, so perhaps fx_movement_modifier_ref needs to be changed to refer to 0x7e
int fx_cure_slow_state (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_cure_slow_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	target->fxqueue.RemoveAllEffects( fx_movement_modifier_ref );
//	STATE_CURE( STATE_SLOWED );
	return FX_NOT_APPLIED;
}

// 0xa4 Cure:Intoxication
int fx_cure_intoxication (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_cure_intoxication(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	target->fxqueue.RemoveAllEffects( fx_intoxication_ref );
	BASE_SET(IE_INTOXICATION,0);
	return FX_NOT_APPLIED;
}

// 0xa5 PauseTarget
int fx_pause_target (Scriptable* /*Owner*/, Actor * target, Effect* fx)
{
	// print("fx_pause_target(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	// the parameters are not set (bg2), so we can't use STAT_MOD alone
	if (!fx->Parameter1) {
		fx->Parameter1 = 1;
	}

	STAT_MOD(IE_CASTERHOLD); // the actual pausing is done elsewhere
	return FX_PERMANENT;
}

// 0xa6 MagicResistanceModifier
int fx_magic_resistance_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_magic_resistance_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	// iwd2 monk's Diamond soul
	// for some silly reason the +1/level was not handled as a clab
	// our clab adds the 10 and sets Special to the rate per level
	if (core->HasFeature(GFFlags::RULES_3ED) && fx->FirstApply) {
		fx->Parameter1 += target->GetMonkLevel() * fx->IsVariable;
	}

	STAT_MOD( IE_RESISTMAGIC );
	return FX_APPLIED;
}

// 0xa7 MissileHitModifier
int fx_missile_to_hit_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_missile_to_hit_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	STAT_MOD( IE_MISSILEHITBONUS );
	return FX_APPLIED;
}

// 0xa8 RemoveCreature
// removes targeted creature
// removes creature specified by resource key (gemrb extension)
int fx_remove_creature (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_remove_creature(%2d)", fx->Opcode);

	const Map *map;

	if (target) {
		map = target->GetCurrentArea();
	} else {
		map = core->GetGame()->GetCurrentArea();
	}
	Actor *actor = target;

	if (!fx->Resource.IsEmpty()) {
		if (map) {
			actor = map->GetActorByResource(fx->Resource);
		} else {
			actor = NULL;
		}
	}

	if (actor) {
		//leaveparty will be handled automagically
		//plot critical items are not handled, shall we?
		actor->DestroySelf();
	}
	return FX_NOT_APPLIED;
}

// 0xa9 Icon:Disable
int fx_disable_portrait_icon (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_disable_portrait_icon(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	target->DisablePortraitIcon(fx->Parameter2);
	return FX_APPLIED;
}

// 0xaa DamageAnimation
int fx_damage_animation (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_damage_animation(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	//Parameter1 is a gemrb extension
	//Parameter2's high byte has effect on critical damage animations (PST compatibility hack)
	target->PlayDamageAnimation(fx->Parameter2, !fx->Parameter1);
	return FX_NOT_APPLIED;
}

// 0xab Spell:Add
// param2 handling is a gemrb extension
int fx_add_innate (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_add_innate(%2d): Resource: %s Mode: %d", fx->Opcode, fx->Resource, fx->Parameter2);
	target->LearnSpell(fx->Resource, fx->Parameter2|LS_MEMO);
	//this is an instant, so it shouldn't stick
	return FX_NOT_APPLIED;
}

// 0xac Spell:Remove
//gemrb extension: deplete spell by resref
int fx_remove_spell (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_remove_spell(%2d): Resource: %s Type:%d", fx->Opcode, fx->Resource, fx->Parameter2);

	bool onlyknown;

	switch (fx->Parameter2) {
	default:
		// in yet another poor IE design decision ...
		onlyknown = fx->Resource.length() == 8;
		target->spellbook.RemoveSpell(fx->Resource, onlyknown);
		break;
	case 1: //forget all spells of Resource
		do {} while(target->spellbook.HaveSpell( fx->Resource, HS_DEPLETE ));
		break;
	case 2: //forget x spells of resource
		while( fx->Parameter1--) {
			target->spellbook.HaveSpell( fx->Resource, HS_DEPLETE );
		}
		break;
	}
	//this is an instant, so it shouldn't stick
	return FX_NOT_APPLIED;
}

// 0xad PoisonResistanceModifier
int fx_poison_resistance_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// this is a gemrb extension, the original only supported flat setting of this resistance
	// ... up until EE version 2.5, where it started always incrementing
	STAT_MOD( IE_RESISTPOISON );
	return FX_APPLIED;
}

//0xae PlaySound
int fx_playsound (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print( "fx_playsound (%s)", fx->Resource );

	if (target && STATE_GET(STATE_DEAD)) {
		return FX_NOT_APPLIED;
	}

	//this is probably inaccurate
	if (target) {
		core->GetAudioDrv()->Play(fx->Resource, SFX_CHAN_HITS, target->Pos);
	} else {
		core->GetAudioDrv()->PlayRelative(fx->Resource, SFX_CHAN_HITS);
	}
	//this is an instant, it shouldn't stick
	return FX_NOT_APPLIED;
}

//0x6d State:Hold3
//0xfb State:Hold4
int fx_hold_creature_no_icon (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_hold_creature_no_icon(%2d): Value: %d, IDS: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	//actually the original engine just skips this effect if the target is dead
	if ( STATE_GET(STATE_DEAD) ) {
		return FX_NOT_APPLIED;
	}

	if (!EffectQueue::match_ids( target, fx->Parameter2, fx->Parameter1) ) {
		//if the ids don't match, the effect doesn't stick
		return FX_NOT_APPLIED;
	}
	target->SetSpellState(SS_HELD);
	STATE_SET(STATE_HELPLESS);
	STAT_SET( IE_HELD, 1);
	return FX_APPLIED;
}

//0xaf State:Hold
//0xb9 State:Hold2
//(0x6d/0x1a8 for iwd2)
int fx_hold_creature (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_hold_creature(%2d): Value: %d, IDS: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	//actually the original engine just skips this effect if the target is dead
	if ( STATE_GET(STATE_DEAD) ) {
		return FX_NOT_APPLIED;
	}

	//iwd2 free action or blood rage disables this effect
	if (target->HasSpellState(SS_FREEACTION)) return FX_NOT_APPLIED;
	if (target->HasSpellState(SS_BLOODRAGE)) return FX_NOT_APPLIED;
	if (target->HasSpellState(SS_AEGIS)) return FX_NOT_APPLIED;

	if (!EffectQueue::match_ids( target, fx->Parameter2, fx->Parameter1) ) {
		//if the ids don't match, the effect doesn't stick
		return FX_NOT_APPLIED;
	}
	target->SetSpellState(SS_HELD);
	STAT_SET( IE_HELD, 1);
	target->AddPortraitIcon(PI_HELD);
	return FX_APPLIED;
}
//0xb0 see: fx_movement_modifier

//0xb1 ApplyEffect
int fx_apply_effect (Scriptable* Owner, Actor* target, Effect* fx)
{
	// print( "fx_apply_effect (%2d) %s", fx->Opcode, fx->Resource );

	//this effect executes a file effect in place of this effect
	//the file effect inherits the target and the timingmode, but gets
	//a new chance to roll percents
	if (target && !EffectQueue::match_ids( target, fx->Parameter2, fx->Parameter1) ) {
		return FX_NOT_APPLIED;
	}

	//apply effect, if the effect is a goner, then kill
	//this effect too
	Effect *myfx = core->GetEffect(fx->Resource, fx->Power, fx->Pos);
	if (!myfx)
		return FX_NOT_APPLIED;

	myfx->RandomValue = core->Roll(1, 100, -1);
	myfx->Target = FX_TARGET_PRESET;
	myfx->TimingMode = fx->TimingMode;
	myfx->Duration = fx->Duration;
	myfx->CasterID = fx->CasterID;

	int ret;
	if (target) {
		if (fx->FirstApply && (fx->IsVariable || fx->TimingMode == FX_DURATION_INSTANT_PERMANENT_AFTER_BONUSES)) {
			// FIXME: should this happen for all effects?
			//hack to entirely replace this effect with the applied effect, this is required for some generic effects
			//that must be put directly in the effect queue to have any impact (to be counted by BonusAgainstCreature, etc)
			myfx->Source = fx->Source; // more?
			target->fxqueue.AddEffect(myfx);
			return FX_NOT_APPLIED;
		}
		ret = target->fxqueue.ApplyEffect(target, myfx, fx->FirstApply, !fx->Parameter3);
	} else {
		EffectQueue fxqueue;
		fxqueue.SetOwner(Owner);
		ret = fxqueue.ApplyEffect(NULL, myfx, fx->FirstApply, !fx->Parameter3);
	}

	fx->Parameter3 = 1;
	delete myfx;
	return ret;
}

//0xb2 hitbonus generic effect ToHitVsCreature
//0xb3 damagebonus generic effect DamageVsCreature
// b4 can't use item (resource) generic effect CantUseItem
// b5 can't use itemtype (resource) generic effect CantUseItemType

// b6 generic effect ApplyEffectItem
// didn't work in the originals or ees
// didn't check immunities
int fx_apply_effect_item (Scriptable* Owner, Actor* target, Effect* fx)
{
	// print("fx_apply_effect_item(%2d)(%.8s)", fx->Opcode, fx->Resource);
	if (target->inventory.HasItem(fx->Resource, 0) ) {
		core->ApplySpell(fx->Resource2, target, Owner, fx->Parameter1);
		return FX_NOT_APPLIED;
	}
	return FX_APPLIED;
}

// b7 generic effect ApplyEffectItemType
// didn't check immunities
int fx_apply_effect_item_type (Scriptable* Owner, Actor* target, Effect* fx)
{
	// print("fx_apply_effect_item(%2d), Type: %d", fx->Opcode, fx->Parameter2);
	if (target->inventory.HasItemType(fx->Parameter2) ) {
		core->ApplySpell(fx->Resource, target, Owner, fx->Parameter1);
		return FX_NOT_APPLIED;
	}
	return FX_APPLIED;
}

// b8 DontJumpModifier
int fx_dontjump_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_dontjump_modifier(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	STAT_SET( IE_DONOTJUMP, fx->Parameter2 );
	return FX_APPLIED;
}

//0xb9 see above: fx_hold_creature

//0xba MoveToArea
int fx_move_to_area (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print( "fx_move_to_area (%2d) %s", fx->Opcode, fx->Resource );

	Game *game = core->GetGame();
	//remove actor from current map, and set destination map
	if (fx->FirstApply) {
		//if current area is different from target area
		if (game->CurrentArea != fx->Resource) {
			//make global
			game->AddNPC( target );
			//remove from current area
			Map *map = target->GetCurrentArea();
			if (map) {
				map->RemoveActor( target );
			}
			//set the destination area
			target->Area = fx->Resource;
			return FX_APPLIED;
		}
	}

	if (game->CurrentArea == fx->Resource) {
		//UnMakeGlobal only if it was not in the party
		int slot = core->GetGame()->InStore( target );
		if (slot >= 0) {
			game->DelNPC( slot );
			if (!target->InParty) {
				target->SetPersistent(-1);
			}
		}
		//move to area
		Point& targetPos = fx->Pos;
		if (targetPos.IsZero() || targetPos.IsInvalid()) targetPos = fx->Source;
		MoveBetweenAreasCore(target, fx->Resource, targetPos, fx->Parameter2, true);
		//remove the effect now
		return FX_NOT_APPLIED;
	}
	//stick around, waiting for the time
	return FX_APPLIED;
}

// 0xbb Variable:StoreLocalVariable
int fx_local_variable (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	//this is a hack, the variable name spreads across the resources
	// print( "fx_local_variable (%2d) %s=%d", fx->Opcode, fx->Resource, fx->Parameter1 );
	target->locals[fx->VariableName] = fx->Parameter1;
	//local variable effects are not applied, they will be resaved though
	return FX_NOT_APPLIED;
}

// 0xbc AuraCleansingModifier
int fx_auracleansing_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_auracleansing_modifier(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	STAT_SET( IE_AURACLEANSING, fx->Parameter2 );
	return FX_APPLIED;
}

// 0xbd CastingSpeedModifier
int fx_castingspeed_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_castingspeed_modifier(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	STAT_MOD( IE_MENTALSPEED );
	// BGEE2 has param2==2: Set casting time of spells with casting
	// time higher than param1 to param1 ... which we handle in the user
	return FX_APPLIED;
}

// 0xbe PhysicalSpeedModifier
int fx_attackspeed_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_attackspeed_modifier(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	STAT_MOD( IE_PHYSICALSPEED );
	return FX_APPLIED;
}

// 0xbf CastingLevelModifier
// gemrb extension: if the resource key is set, apply param1 as a percentual modifier
int fx_castinglevel_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print( "fx_castinglevel_modifier (%2d) Value:%d Type:%d", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	switch (fx->Parameter2) {
	case 0:
		if (!fx->Resource.IsEmpty()) {
			STAT_MUL( IE_CASTINGLEVELBONUSMAGE, fx->Parameter1 );
		} else {
			STAT_SET( IE_CASTINGLEVELBONUSMAGE, fx->Parameter1 );
		}
		break;
	case 1:
		if (!fx->Resource.IsEmpty()) {
			STAT_MUL( IE_CASTINGLEVELBONUSCLERIC, fx->Parameter1 );
		} else {
			STAT_SET( IE_CASTINGLEVELBONUSCLERIC, fx->Parameter1 );
		}
		break;
	default:
		return FX_NOT_APPLIED;
	}
	return FX_APPLIED;
}

//returns the familiar if there was no error
static Actor *GetFamiliar(Scriptable *Owner, const Actor *target, const Effect *fx, const ResRef& resource)
{
	//summon familiar
	Actor *fam = gamedata->GetCreature(resource);
	if (!fam) {
		return NULL;
	}
	fam->SetBase(IE_EA, EA_FAMILIAR);

	//when upgrading, there is no need for the script to be triggered again, so this isn't a problem
	if (Owner) {
		fam->objects.LastSummoner = Owner->GetGlobalID();
	}

	Map *map = target->GetCurrentArea();
	if (!map) return NULL;

	map->AddActor(fam, true);
	fam->SetPosition(fx->Pos, true);
	fam->RefreshEffects();
	//Make the familiar an NPC (MoveGlobal needs this)
	Game *game = core->GetGame();
	game->AddNPC(fam);

	//Add some essential effects
	Effect *newfx = EffectQueue::CreateEffect(fx_familiar_constitution_loss_ref, fam->GetBase(IE_HITPOINTS)/2, 0, FX_DURATION_INSTANT_PERMANENT);
	core->ApplyEffect(newfx, fam, fam);

	//the familiar marker needs to be set to 2 in case of ToB
	ieDword fm = 0;
	if (game->Expansion == GAME_TOB) {
		fm = 2;
	}
	newfx = EffectQueue::CreateEffect(fx_familiar_marker_ref, fm, 0, FX_DURATION_INSTANT_PERMANENT);
	core->ApplyEffect(newfx, fam, fam);

	//maximum hp bonus of half the familiar's hp, there is no hp new bonus upgrade when upgrading familiar
	//this is a bug even in the original engine, so I don't care
	if (Owner) {
		newfx = EffectQueue::CreateEffect(fx_maximum_hp_modifier_ref, fam->GetBase(IE_HITPOINTS)/2, MOD_ADDITIVE, FX_DURATION_INSTANT_PERMANENT);
		core->ApplyEffect(newfx, (Actor *) Owner, Owner);
	}

	if (!fx->Resource2.IsEmpty()) {
		ScriptedAnimation* vvc = gamedata->GetScriptedAnimation(fx->Resource2, false);
		if (vvc) {
			//This is the final position of the summoned creature
			//not the original target point
			vvc->Pos = fam->Pos;
			//force vvc to play only once
			vvc->PlayOnce();
			map->AddVVCell(vvc);
		}
	}

	return fam;
}

// 0xc0 FindFamiliar
// param2 = 0 normal
// param2 = 1 alignment is in param1
// param2 = 2 resource used
int fx_find_familiar (Scriptable* Owner, Actor* target, Effect* fx)
{
	if (!target || !Owner) {
		return FX_NOT_APPLIED;
	}

	if (!target->GetCurrentArea()) {
		//this will delay casting until we get an area
		return FX_APPLIED;
	}

	const Game *game = core->GetGame();
	//FIXME: the familiar block field is not saved in the game and not set when the
	//familiar is itemized, so a game reload will clear it (see how this is done in original)
	if (game->familiarBlock) {
		displaymsg->DisplayConstantStringName(HCStrings::FamiliarBlock, GUIColors::RED, target);
		return FX_NOT_APPLIED;
	}

	//The protagonist is ALWAYS in the first slot
	if (game->GetPC(0, false)!=target) {
		displaymsg->DisplayConstantStringName(HCStrings::FamiliarProtagonistOnly, GUIColors::RED, target);
		return FX_NOT_APPLIED;
	}

	if (fx->Parameter2 != 2) {
		ieDword alignment;

		if (fx->Parameter2 == 1) {
			alignment = fx->Parameter1;
		} else {
			alignment = target->GetStat(IE_ALIGNMENT);
			alignment = ((alignment&AL_LC_MASK)>>4)*3+(alignment&AL_GE_MASK)-4;
		}
		if (alignment>8) {
			return FX_NOT_APPLIED;
		}

		//ToB familiars
		if (game->Expansion == GAME_TOB) {
			// just appending 25 breaks the quasit, fairy dragon and dust mephit upgrade
			fx->Resource.Format("{:.6}25", game->GetFamiliar(alignment));
		} else {
			fx->Resource = game->GetFamiliar(alignment);
		}
		fx->Parameter2 = 2;
	}

	GetFamiliar(Owner, target, fx, fx->Resource);
	return FX_NOT_APPLIED;
}

// 0xc1 InvisibleDetection
int fx_see_invisible_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_see_invisible_modifier(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	STAT_SET( IE_SEEINVISIBLE, fx->Parameter2 );
	return FX_APPLIED;
}

// 0xc2 IgnoreDialogPause
int fx_ignore_dialogpause_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_ignore_dialogpause_modifier(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	STAT_SET( IE_IGNOREDIALOGPAUSE, fx->Parameter2 );
	return FX_APPLIED;
}

//0xc3 FamiliarBond
//when this effect's target dies it should incur damage on protagonist
int fx_familiar_constitution_loss (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_familiar_constitution_loss(%2d): Loss: %d", fx->Opcode,(signed) fx->Parameter1);
	if (!STATE_GET(STATE_NOSAVE)) {
		return FX_APPLIED;
	}
	Effect *newfx;
	//familiar died
	Actor *master = core->GetGame()->FindPC(1);
	if (!master) return FX_NOT_APPLIED;

	//lose 1 point of constitution
	newfx = EffectQueue::CreateEffect(fx_constitution_modifier_ref, (ieDword) -1, MOD_ADDITIVE, FX_DURATION_INSTANT_PERMANENT);
	core->ApplyEffect(newfx, master, master);

	//remove the maximum hp bonus
	newfx = EffectQueue::CreateEffect(fx_maximum_hp_modifier_ref, (ieDword) (-(signed) (fx->Parameter1)), 3, FX_DURATION_INSTANT_PERMANENT);
	core->ApplyEffect(newfx, master, master);

	//damage for half of the familiar's hitpoints
	newfx = EffectQueue::CreateEffect(fx_damage_opcode_ref, fx->Parameter1, DAMAGE_CRUSHING<<16, FX_DURATION_INSTANT_PERMANENT);
	core->ApplyEffect(newfx, master, master);

	return FX_NOT_APPLIED;
}

//0xc4 FamiliarMarker
int fx_familiar_marker(Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (!target) {
		return FX_NOT_APPLIED;
	}

	Game *game = core->GetGame();

	//upgrade familiar to ToB version
	if (fx->Parameter1 != 2 && game->Expansion == GAME_TOB) {
		ResRef resource;
		resource.Format("{:.6}25", target->GetScriptName());
		//set this field, so the upgrade is triggered only once
		fx->Parameter1 = 2;

		//the NULL here is probably fine when upgrading, Owner (Original summoner) is not needed.
		const Actor *fam = GetFamiliar(nullptr, target, fx, resource);

		if (fam) {
			//upgrade successful
			//TODO: copy stuff from old familiar if needed
			target->DestroySelf();
			return FX_NOT_APPLIED;
		}
	}

	if (!STATE_GET(STATE_NOSAVE)) {
		game->familiarBlock=true;
		if (fx->FirstApply) {
			const Actor* master = Scriptable::As<Actor>(GetCasterObject());
			if (master && master->InParty) game->FamiliarOwner = master->InParty - 1;
		}
		return FX_APPLIED;
	}
	game->familiarBlock=false;
	return FX_NOT_APPLIED;
}

// 0xc5 Bounce:Projectile
int fx_bounce_projectile (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_bounce_projectile(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	STAT_BIT_OR_PCF( IE_BOUNCE, BNC_PROJECTILE );
	return FX_APPLIED;
}

// 0xc6 Bounce:Opcode
int fx_bounce_opcode (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_bounce_opcode(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	STAT_BIT_OR_PCF( IE_BOUNCE, BNC_OPCODE );
	target->AddPortraitIcon(PI_BOUNCE2);
	return FX_APPLIED;
}

// 0xc7 Bounce:SpellLevel
int fx_bounce_spelllevel (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_bounce_spellevel(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	STAT_BIT_OR_PCF( IE_BOUNCE, BNC_LEVEL );
	target->AddPortraitIcon(PI_BOUNCE2);
	return FX_APPLIED;
}

// 0xc8 Bounce:SpellLevelDec
int fx_bounce_spelllevel_dec (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_bounce_spellevel_dec(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	if (fx->Parameter1 < 1 || STATE_GET(STATE_DEAD)) {
		PlayRemoveEffect(target, fx, "EFF_E02");
		return FX_NOT_APPLIED;
	}

	STAT_BIT_OR_PCF( IE_BOUNCE, BNC_LEVEL_DEC );
	target->AddPortraitIcon(PI_BOUNCE);
	return FX_APPLIED;
}

//0xc9 Protection:SpellLevelDec
int fx_protection_spelllevel_dec (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_protection_spelllevel_dec(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	if (fx->Parameter1<1) {
		PlayRemoveEffect(target, fx, "EFF_E02");
		return FX_NOT_APPLIED;
	}
	STAT_BIT_OR( IE_IMMUNITY, IMM_LEVEL_DEC );
	target->AddPortraitIcon(PI_BOUNCE2);
	target->SetOverlay(OV_SPELLTRAP);
	return FX_APPLIED;
}

//0xca Bounce:School
int fx_bounce_school (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_bounce_school(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	STAT_BIT_OR_PCF( IE_BOUNCE, BNC_SCHOOL );
	target->AddPortraitIcon(PI_BOUNCE2);
	return FX_APPLIED;
}

// 0xcb Bounce:SecondaryType
int fx_bounce_secondary_type (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_bounce_secondary_type(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	STAT_BIT_OR_PCF( IE_BOUNCE, BNC_SECTYPE );
	target->AddPortraitIcon(PI_BOUNCE2);
	return FX_APPLIED;
}

// 0xcc //resist school
int fx_protection_school (Scriptable* /*Owner*/, Actor* target, Effect */*fx*/)
{
	// print("fx_protection_school(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	STAT_BIT_OR( IE_IMMUNITY, IMM_SCHOOL);
	target->SetOverlay(OV_SPELLTRAP);
	return FX_APPLIED;
}

// 0xcd //resist sectype
int fx_protection_secondary_type (Scriptable* /*Owner*/, Actor* target, Effect */*fx*/)
{
	// print("fx_protection_secondary_type(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	STAT_BIT_OR( IE_IMMUNITY, IMM_SECTYPE);
	target->SetOverlay(OV_SPELLTRAP);
	return FX_APPLIED;
}

//0xce Protection:Spell
int fx_resist_spell (Scriptable* /*Owner*/, Actor* target, Effect *fx)
{
	if (fx->Resource != fx->SourceRef) {
		STAT_BIT_OR( IE_IMMUNITY, IMM_RESOURCE);
		return FX_APPLIED;
	}
	//this has effect only on first apply, it will stop applying the spell
	return FX_ABORT;
}

//0xce (same place as in bg2, but different targeting)
int fx_resist_spell2(Scriptable* Owner, Actor* target, Effect *fx)
{
	if (!EffectQueue::CheckIWDTargeting(Owner, target, fx->Parameter1, fx->Parameter2, fx)) {
		return FX_NOT_APPLIED;
	}

	if (fx->Resource != fx->SourceRef) {
		return FX_APPLIED;
	}
	//this has effect only on first apply, it will stop applying the spell
	return FX_ABORT;
}

// ??? Protection:SpellDec
// This is a fictional opcode, it isn't implemented in the original engine
int fx_resist_spell_dec (Scriptable* /*Owner*/, Actor* target, Effect *fx)
{
	// print("fx_resist_spell_dec(%2d): Resource: %s", fx->Opcode, fx->Resource);

	if (fx->Parameter1<1) {
		PlayRemoveEffect(target, fx, "EFF_E02");
		return FX_NOT_APPLIED;
	}

	if (fx->Resource != fx->SourceRef) {
		STAT_BIT_OR( IE_IMMUNITY, IMM_RESOURCE_DEC);
		return FX_APPLIED;
	}
	//this has effect only on first apply, it will stop applying the spell
	return FX_ABORT;
}

// 0xcf Bounce:Spell
int fx_bounce_spell (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_bounce_spell(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	STAT_BIT_OR_PCF( IE_BOUNCE, BNC_RESOURCE );
	return FX_APPLIED;
}

// ??? Bounce:SpellDec
// This is a fictional opcode, it isn't implemented in the original engine
int fx_bounce_spell_dec (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_bounce_spell(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	if (fx->Parameter1<1) {
		PlayRemoveEffect(target, fx, "EFF_E02");
		return FX_NOT_APPLIED;
	}
	STAT_BIT_OR_PCF( IE_BOUNCE, BNC_RESOURCE_DEC );
	return FX_APPLIED;
}

// 0xd0 MinimumHPModifier
// the original engine didn't allow modifying of this stat
// it allowed only setting it, and only by one instance
int fx_minimum_hp_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_minimum_hp_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	STAT_MOD( IE_MINHITPOINTS );
	return FX_APPLIED;
}

//0xd1 PowerWordKill
int fx_power_word_kill (Scriptable* Owner, Actor* target, Effect* fx)
{
	// print("fx_power_word_kill(%2d): HP: %d Stat: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	ieDword limit = 60;

	if (target->GetStat(IE_EXTSTATE_ID) & EXTSTATE_EYE_SPIRIT) {
		target->fxqueue.RemoveAllEffects(fx_eye_spirit_ref);
		target->spellbook.RemoveSpell(SevenEyes[EYE_SPIRIT]);
		target->SetBaseBit(IE_EXTSTATE_ID, EXTSTATE_EYE_SPIRIT, false);
		return FX_ABORT;
	}

	if (fx->Parameter1) {
		limit = fx->Parameter1;
	}
	//normally this would work only with hitpoints
	//but why not add some extra features
	ieDword stat = target->GetStat (fx->Parameter2&0xffff);

	if (stat < limit) {
		target->Die( Owner );
	}
	return FX_NOT_APPLIED;
}

//0xd2 PowerWordStun
int fx_power_word_stun (Scriptable* Owner, Actor* target, Effect* fx)
{
	// print("fx_power_word_stun(%2d): HP: %d Stat: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	ieDword limit = 90;

	if (fx->Parameter1) {
		limit = fx->Parameter1;
	}
	//normally this would work only with hitpoints
	//but why not add some extra features
	ieDword stat = target->GetStat (fx->Parameter2&0xffff);
	ieDword x = fx->Parameter2>>16; //dice sides

	if (stat > limit) {
		return FX_NOT_APPLIED;
	}
	//recalculate delay
	stat = (stat * 3 + limit - 1) / limit;
	//delay will be calculated as 1dx/2dx/3dx
	//depending on the current hitpoints (or the stat in param2)
	stat = core->Roll(stat,x?x:4,0) * core->Time.round_size;
	fx->Duration = core->GetGame()->GameTime+stat;
	fx->TimingMode = FX_DURATION_ABSOLUTE;
	fx->Opcode = EffectQueue::ResolveEffect(fx_set_stun_state_ref);
	return fx_set_stun_state(Owner,target,fx);
}

//0xd3 State:Imprisonment (avatar removal plus portrait icon)
int fx_imprisonment(Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// a bunch of odd special-casing for (fake) familiars
	if (target->GetBase(IE_EA) == EA_FAMILIAR) {
		if (fx->IsVariable == 1) {
			target->GetCurrentArea()->RemoveActor(target);
			return FX_NOT_APPLIED;
		} else if (fx->IsVariable == 2) {
			BASE_STATE_SET(STATE_DEAD);
			target->SetBase(IE_EA, EA_NEUTRAL);
			target->SetPersistent(-1);
			return FX_NOT_APPLIED;
		} else if (fx->IsVariable == 3) {
			target->SetBase(IE_EA, EA_NEUTRAL);
			target->SetPersistent(-1);
		} else {
			core->GetGame()->familiarBlock = false;
			core->GetGame()->FamiliarOwner = 0;
			target->GetCurrentArea()->RemoveActor(target);
			return FX_NOT_APPLIED;
		}
	}
	STAT_SET(IE_AVATARREMOVAL, 1);
	target->AddPortraitIcon(PI_PRISON);
	target->SendDiedTrigger();
	target->Stop();
	if (target->InParty) core->GetGame()->LeaveParty(target);
	return FX_APPLIED;
}

//0xd4 Cure:Imprisonment
int fx_freedom (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_freedom(%2d)", fx->Opcode);
	target->fxqueue.RemoveAllEffects( fx_imprisonment_ref );
	target->fxqueue.RemoveAllEffects( fx_maze_ref );
	return FX_NOT_APPLIED;
}

//0xd5 Maze
int fx_maze (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_maze(%2d)", fx->Opcode);
	const Game *game = core->GetGame();
	if (fx->Parameter2) {
		//this version of maze works only in combat
		if (!fx->FirstApply && !game->CombatCounter) {
			return FX_NOT_APPLIED;
		}
	} else {
		if (fx->FirstApply) {
			//get the maze dice number (column 3)
			int stat = target->GetSafeStat(IE_INT);
			int size = core->GetIntelligenceBonus(3, stat);
			int dice = core->GetIntelligenceBonus(4, stat);
			fx->Duration = game->GameTime + target->LuckyRoll(dice, size, 0, LR_NEGATIVE) * core->Time.round_size;

			// fix the effect from being FX_DURATION_DELAY_PERMANENT to a non-oneshot
			// needed for the bg2 version of the maze spell (spwi813)
			fx->TimingMode = FX_DURATION_INSTANT_LIMITED;
		}
	}

	if (core->InCutSceneMode()) return FX_APPLIED;

	STAT_SET(IE_AVATARREMOVAL, 1);
	target->AddPortraitIcon(PI_MAZE);
	target->Stop();
	return FX_APPLIED;
}

//0xd6 CastFromList
//GemRB extension: if fx->Parameter1 is set, it is the bitfield of spell types (could be priest spells)
int fx_select_spell (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	auto& vars = core->GetDictionary();
	// print("fx_select_spell(%2d) %d", fx->Opcode, fx->Parameter2);
	Spellbook *sb = &target->spellbook;
	if(fx->Parameter2) {
		//all known spells, no need to memorize
		// the details are all handled by the Spellbook guiscript
		vars.Set("ActionLevel", 5);
	} else {
		//all spells listed in 2da
		// (ees) differentiate between 1 and 2 with some minor extra filtering, but we do that elsewhere
		std::vector<ResRef> data;
		gamedata->ReadResRefTable(fx->Resource, data);
		sb->SetCustomSpellInfo(data, fx->SourceRef, 0);

		vars.Set("ActionLevel", 11);
	}
	// force a redraw of the action bar
	//this is required, because not all of these opcodes are firing right at casting
	vars.Set("Type", -1);
	core->SetEventFlag(EF_ACTION);
	return FX_NOT_APPLIED;
}

// 0xd7 PlayVisualEffect
// Known values for Parameter2 are:
// 0 Play on target (not attached)
// 1 Play on target (attached)
// 2 Play on point
int fx_play_visual_effect (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_play_visual_effect(%2d): Resource: %s Type: %d", fx->Opcode, fx->Resource, fx->Parameter2);

	//this is in the original engine (dead actors lose this effect)
	if (!target || STATE_GET(STATE_DEAD)) {
		return FX_NOT_APPLIED;
	}

	//delay action until area is loaded to avoid crash
	Map *map = target->GetCurrentArea();
	if (!map) return FX_APPLIED;

	if (target->fxqueue.HasEffectWithResource(fx_protection_from_animation_ref, fx->Resource)) {
		// effect suppressed by opcode 0x128
		return FX_APPLIED;
	}

	//if it is sticky, don't add it if it is already played
	if (fx->Parameter2) {
		auto range = target->GetVVCCells(fx->Resource);
		if (range.first != range.second) {
			for (; range.first != range.second; ++range.first) {
				range.first->second->active = true;
			}
			return FX_APPLIED;
		}
		if (! fx->FirstApply) return FX_NOT_APPLIED;
	}

	ScriptedAnimation* sca = gamedata->GetScriptedAnimation(fx->Resource, false);

	//don't crash on nonexistent resources
	if (!sca) {
		return FX_NOT_APPLIED;
	}

	if (fx->TimingMode!=FX_DURATION_INSTANT_PERMANENT) {
		sca->SetDefaultDuration(fx->Duration-core->GetGame()->GameTime);
		if (!(sca->SequenceFlags & IE_VVC_LOOP) && sca->anims[P_HOLD * MAX_ORIENT]) {
			// shorten effect duration to match vvc; Duration is sensible only for looping or frozen looping vvcs
			fx->Duration = sca->anims[P_HOLD * MAX_ORIENT]->GetFrameCount() + core->GetGame()->GameTime;
		}
	}
	if (fx->Parameter2 == 1) {
		//play over target (sticky)
		sca->SetEffectOwned(true);
		target->AddVVCell( sca );
		return FX_APPLIED;
	}

	//not sticky
	if (fx->Parameter2 == 2) {
		// if source is set use that instead
		// used for example in Horror, where you want only one screaming face at the target location, not all secondaries #198
		// (both the main and secondary projectiles carry this effect with the same resource)
		// BUT child pros shouldn't actually draw this, since their timing is not in sync and the last few frames will then flicker, as each copy ends
		if (!fx->Source.IsZero()) {
			if (map->HasVVCCell(fx->Resource, fx->Source)) {
				delete sca;
				return FX_NOT_APPLIED;
			}
			sca->Pos = fx->Source;
		} else {
			sca->Pos = fx->Pos;
		}
	} else {
		sca->Pos = target->Pos;
	}
	sca->PlayOnce();
	map->AddVVCell(sca);
	return FX_NOT_APPLIED;
}

//0xd8 LevelDrainModifier
// BG2 level drain internally uses parameter3 to decrease the MaxHp (current hp?), and parameter4 to decrease level (max hp?). (unset)
int fx_leveldrain_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (STATE_GET(STATE_DEAD)) {
		return FX_NOT_APPLIED;
	}

	//never subtract more than the maximum hitpoints
	ieDword x = STAT_GET(IE_MAXHITPOINTS)-1;
	int mod = signed(fx->Parameter1);
	if (mod * 4 < signed(x)) {
		x = mod * 4;
	}
	STAT_ADD(IE_LEVELDRAIN, mod);
	STAT_SUB(IE_MAXHITPOINTS, x);
	HandleBonus(target, IE_SAVEVSDEATH, -mod, fx->TimingMode);
	HandleBonus(target, IE_SAVEVSWANDS, -mod, fx->TimingMode);
	HandleBonus(target, IE_SAVEVSPOLY, -mod, fx->TimingMode);
	HandleBonus(target, IE_SAVEVSBREATH, -mod, fx->TimingMode);
	HandleBonus(target, IE_SAVEVSSPELL, -mod, fx->TimingMode);

	// to-hit gets recalculated based on class, but we just estimate it
	STAT_SUB(IE_TOHIT, mod / 2);
	// same for skills
	// all are adjusted by the average amount of skill points they receive per level, capped to [0,255] before racial and/or dexterity adjustments
	static ieDword skillStats[] = { IE_STEALTH, IE_TRAPS, IE_PICKPOCKET, IE_HIDEINSHADOWS, IE_DETECTILLUSIONS, IE_SETTRAPS };
	for (const auto& stat : skillStats) {
		STAT_SUB(stat, 20 * mod);
	}
	STAT_SUB(IE_LORE, mod);

	target->AddPortraitIcon(PI_LEVELDRAIN);
	//decrease current hitpoints on first apply
	if (fx->FirstApply) {
		//current hitpoints don't have base/modified, only current
		BASE_SUB(IE_HITPOINTS, x);
	}

	return FX_APPLIED;
}

//d9 PowerWordSleep

int fx_power_word_sleep (Scriptable* Owner, Actor* target, Effect* fx)
{
	// gemrb extension: pass a stat to check instead of hp via parameter1
	ieDword limit = 20;

	if (fx->Parameter1) {
		limit = fx->Parameter1;
	}

	ieDword stat = target->GetStat (fx->Parameter2&0xffff);
	ieDword x = fx->Parameter2>>16; //rounds
	if (!x) x = 5;

	if (stat>limit) {
		return FX_NOT_APPLIED;
	}
	//translate this effect to a normal sleep effect
	//recalculate delay
	fx->Duration = core->GetGame()->GameTime+x*core->Time.round_size;
	fx->TimingMode = FX_DURATION_ABSOLUTE;
	fx->Opcode = EffectQueue::ResolveEffect(fx_set_sleep_state_ref);
	if (!core->HasFeature(GFFlags::HAS_EE_EFFECTS)) fx->Parameter2 = 0;
	return fx_set_unconscious_state(Owner,target,fx);
}

// 0xda StoneSkinModifier
int fx_stoneskin_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_stoneskin_modifier(%2d): Mod: %d", fx->Opcode, fx->Parameter1);
	if (!fx->Parameter1) {
		PlayRemoveEffect(target, fx, "EFF_E02");
		return FX_NOT_APPLIED;
	}

	//dead actors lose this effect
	if (STATE_GET( STATE_DEAD) ) {
		return FX_NOT_APPLIED;
	}

	//this is the bg2 style stoneskin, not normally using spell states
	//but this way we can support hybrid games
	if (core->HasFeature(GFFlags::HAS_EE_EFFECTS)) {
		if (fx->Parameter2) {
			fx->Parameter1 = DICE_ROLL((signed) fx->Parameter1);
		}
		target->SetSpellState(SS_STONESKIN);
		SetGradient(target, fullstone);
	} else {
		// iwds actually differentiate iron skins
		if (fx->Parameter2) {
			target->SetSpellState(SS_IRONSKIN);
			// gradient for iron skins?
		} else {
			target->SetSpellState(SS_STONESKIN);
			SetGradient(target, fullstone);
		}
	}

	STAT_SET(IE_STONESKINS, fx->Parameter1);
	target->AddPortraitIcon(PI_STONESKIN);
	return FX_APPLIED;
}

//0xdb ac vs creature type (general effect)
//0xdc DispelSchool
int fx_dispel_school (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	target->fxqueue.RemoveLevelEffects(fx->Parameter1, RL_MATCHSCHOOL, fx->Parameter2, target);
	return FX_NOT_APPLIED;
}
//0xdd DispelSecondaryType
int fx_dispel_secondary_type (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	target->fxqueue.RemoveLevelEffects(fx->Parameter1, RL_MATCHSECTYPE, fx->Parameter2, target);
	return FX_NOT_APPLIED;
}

//0xde RandomTeleport
int fx_teleport_field (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_teleport_field(%2d): Distance: %d", fx->Opcode, fx->Parameter1);

	const Map *map = target->GetCurrentArea();
	if (!map) {
		return FX_NOT_APPLIED;
	}
	//the origin is the effect's target point
	Point p = Point(core->Roll(1,fx->Parameter1*2,-(signed) (fx->Parameter1)),
					core->Roll(1,fx->Parameter1*2,-(signed) (fx->Parameter1)) ) + fx->Pos;

	target->SetPosition(p, true);
	return FX_NOT_APPLIED;
}

//0xdf //Protection:SchoolDec
int fx_protection_school_dec (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_protection_school_dec(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	if (fx->Parameter1<1) {
		//The original doesn't have anything here
		PlayRemoveEffect(target, fx);
		return FX_NOT_APPLIED;
	}

	STAT_BIT_OR( IE_IMMUNITY, IMM_SCHOOL_DEC );
	target->SetOverlay(OV_SPELLTRAP);
	return FX_APPLIED;
}

//0xe0 Cure:LevelDrain

int fx_cure_leveldrain (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_cure_leveldrain(%2d)", fx->Opcode);
	//all level drain removed at once???
	//if not, then find old effect, remove a number
	target->fxqueue.RemoveAllEffects( fx_leveldrain_ref );
	return FX_NOT_APPLIED;
}

//0xe1 Reveal:Magic
//gemrb special: speed and color are custom
int fx_reveal_magic (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_reveal_magic(%2d)", fx->Opcode);
	if (target->fxqueue.HasAnyDispellableEffect()) {
		if (!fx->Parameter1) {
			fx->Parameter1=0xff00; //blue
		}

		int speed = (fx->Parameter2 >> 16) & 0xFF;
		if (!speed) speed=30;
		target->SetColorMod(0xff, RGBModifier::ADD, speed,
							Color::FromBGRA(fx->Parameter1), 0);
	}
	return FX_NOT_APPLIED;
}

//0xe2 Protection:SecondaryTypeDec
int fx_protection_secondary_type_dec (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_protection_secondary_type_dec(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	if (fx->Parameter1<1) {
		//The original doesn't have anything here
		PlayRemoveEffect(target, fx);
		return FX_NOT_APPLIED;
	}
	STAT_BIT_OR( IE_IMMUNITY, IMM_SECTYPE_DEC );
	target->SetOverlay(OV_BOUNCE);
	return FX_APPLIED;
}

//0xe3 Bounce:SchoolDecrement
int fx_bounce_school_dec (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_bounce_school_dec(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	if (fx->Parameter1<1) {
		//The original doesn't have anything here
		PlayRemoveEffect(target, fx);
		return FX_NOT_APPLIED;
	}
	STAT_BIT_OR_PCF( IE_BOUNCE, BNC_SCHOOL_DEC );
	target->AddPortraitIcon(PI_BOUNCE2);
	return FX_APPLIED;
}

//0xe4 Bounce:SecondaryTypeDecrement
int fx_bounce_secondary_type_dec (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_bounce_secondary_type_dec(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	if (fx->Parameter1<1) {
		//The original doesn't have anything here
		PlayRemoveEffect(target, fx);
		return FX_NOT_APPLIED;
	}
	STAT_BIT_OR_PCF( IE_BOUNCE, BNC_SECTYPE_DEC );
	target->AddPortraitIcon(PI_BOUNCE2);
	return FX_APPLIED;
}

//0xe5 DispelSchoolOne
// in hypothetical cases removes two Resources, not just one (see IESDP)
int fx_dispel_school_one (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	target->fxqueue.RemoveLevelEffects(fx->Parameter1, RL_MATCHSCHOOL | RL_REMOVEFIRST, fx->Parameter2, target);
	return FX_NOT_APPLIED;
}

//0xe6 DispelSecondaryTypeOne
// in hypothetical cases removes two Resources, not just one (see IESDP)
int fx_dispel_secondary_type_one (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	target->fxqueue.RemoveLevelEffects(fx->Parameter1, RL_MATCHSECTYPE | RL_REMOVEFIRST, fx->Parameter2, target);
	return FX_NOT_APPLIED;
}

//0xe7 Timestop
int fx_timestop (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_timestop(%2d)", fx->Opcode);
	core->GetGame()->TimeStop(target, fx->Duration);
	return FX_NOT_APPLIED;
}

//0xe8 CastSpellOnCondition
int fx_cast_spell_on_condition (Scriptable* Owner, Actor* target, Effect* fx)
{
	// print("fx_cast_spell_on_condition(%2d): Target: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	/*
	 * This is used for Fire Shield, etc, to cast spells when certain
	 * triggers are true. It is also used for contingencies, which set
	 * Parameter3 and expect some special processing.
	 * In the original engine, this constructs a 'contingency list' in the
	 * target's stats, which is checked in two cases: every 100 ticks
	 * (for 0x4xxx-type triggers) and every time a trigger is added (for
	 * other triggers).
	 * Instead, we handle the first type directly here in the effect
	 * itself, and the normal type by marking triggers with the flag
	 * TEF_PROCESSED_EFFECTS after every effect run, so we can tell that
	 * only triggers without the flag should be checked.
	 * Conveniently, since contingency versions self-destruct and
	 * non-contingency versions are only allowed to run once per
	 * frame, we need only check a single trigger per effect run.
	 */

	if (fx->FirstApply && fx->Parameter3) {
		// TODO: display strings
		if (gamedata->Exists(fx->Resource, IE_SPL_CLASS_ID)) {
			target->spellbook.HaveSpell(fx->Resource, HS_DEPLETE);
		}
		if (gamedata->Exists(fx->Resource2, IE_SPL_CLASS_ID)) {
			target->spellbook.HaveSpell(fx->Resource2, HS_DEPLETE);
		}
		if (gamedata->Exists(fx->Resource3, IE_SPL_CLASS_ID)) {
			target->spellbook.HaveSpell(fx->Resource3, HS_DEPLETE);
		}
		if (gamedata->Exists(fx->Resource4, IE_SPL_CLASS_ID)) {
			target->spellbook.HaveSpell(fx->Resource4, HS_DEPLETE);
		}
	}

	if (fx->Parameter3) {
		target->AddPortraitIcon(PI_CONTINGENCY);
	}

	// get the actor to cast spells at
	Actor *actor = NULL;
	const Map *map = target->GetCurrentArea();
	if (!map) return FX_APPLIED;

	switch (fx->Parameter1) {
	case 0:
		// Myself
		actor = target;
		break;
	case 1:
		// LastHitter
		actor = map->GetActorByGlobalID(target->objects.LastHitter);
		// but don't attack yourself
		if (actor && actor->GetGlobalID() == Owner->GetGlobalID()) return FX_APPLIED;
		break;
	case 2:
		// NearestEnemyOf
		actor = GetNearestEnemyOf(map, target, 0);
		break;
	case 3:
	default:
		// Nearest
		actor = GetNearestOf(map, target, 0);
		break;
	}

	if (!actor) {
		return FX_APPLIED;
	}

	bool condition = false;
	bool per_round = true; // 4xxx trigger?
	const TriggerEntry *entry = nullptr;
	Trigger* parameters;
	const Actor *nearest = nullptr;

	// check the condition
	switch (fx->Parameter2) {
	case COND_GOTHIT:
		// HitBy([ANYONE])
		entry = target->GetMatchingTrigger(trigger_hitby, TEF_PROCESSED_EFFECTS);
		per_round = false;
		break;
	case COND_NEAR:
		// See(NearestEnemyOf())
		condition = GetNearestEnemyOf(map, target, ORIGIN_SEES_ENEMY) != NULL;
		break;
	case COND_HP_HALF:
		// HPPercentLT(Myself, 50)
		condition = target->GetBase(IE_HITPOINTS) < (target->GetStat(IE_MAXHITPOINTS) / 2);
		break;
	case COND_HP_QUART:
		// HPPercentLT(Myself, 25)
		condition = target->GetBase(IE_HITPOINTS) < (target->GetStat(IE_MAXHITPOINTS) / 4);
		break;
	case COND_HP_LOW:
		// HPPercentLT(Myself, 10)
		condition = target->GetBase(IE_HITPOINTS) < (target->GetStat(IE_MAXHITPOINTS) / 10);
		break;
	case COND_HELPLESS:
		// StateCheck(Myself, STATE_HELPLESS)
		condition = (bool)(target->GetStat(IE_STATE_ID) & STATE_CANTMOVE);
		break;
	case COND_POISONED:
		// StateCheck(Myself, STATE_POISONED)
		condition = (bool)(target->GetStat(IE_STATE_ID) & STATE_POISONED);
		break;
	case COND_ATTACKED:
		// AttackedBy([ANYONE])
		entry = target->GetMatchingTrigger(trigger_attackedby, TEF_PROCESSED_EFFECTS);
		per_round = false;
		break;
	case COND_NEAR4:
		// PersonalSpaceDistance([ANYONE], 4)
		nearest = GetNearestOf(map, target, ORIGIN_SEES_ENEMY);
		condition = nearest && WithinPersonalRange(target, actor, 4);
		break;
	case COND_NEAR10:
		// PersonalSpaceDistance([ANYONE], 10)
		nearest = GetNearestOf(map, target, ORIGIN_SEES_ENEMY);
		condition = nearest && WithinPersonalRange(target, actor, 10);
		break;
	case COND_EVERYROUND:
		condition = true;
		break;
	case COND_TOOKDAMAGE:
		// TookDamage()
		entry = target->GetMatchingTrigger(trigger_tookdamage, TEF_PROCESSED_EFFECTS);
		per_round = false;
		break;
	case COND_KILLER:
		// killed someone (BGEE: Dorn's sword)
		entry = target->GetMatchingTrigger(trigger_killed, TEF_PROCESSED_EFFECTS);
		per_round = false;
		break;
	case COND_TIMEOFDAY:
		// BGEE: Night Club
		if (actor != target) break;
		parameters = new Trigger;
		parameters->int0Parameter = fx->IsVariable;
		condition = GameScript::TimeOfDay(nullptr, parameters);
		delete parameters;
		break;
	case COND_NEARX:
		// PersonalSpaceDistance([ANYONE], 'Extra')
		nearest = GetNearestOf(map, target, ORIGIN_SEES_ENEMY);
		condition = nearest && WithinPersonalRange(nearest, target, fx->IsVariable);
		break;
	case COND_STATECHECK:
		// StateCheck(Myself, 'Extra')
		condition = (target->GetStat(IE_STATE_ID) & fx->IsVariable) > 0;
		break;
	case COND_DIED_ME:
		// Die()
		condition = target->GetMatchingTrigger(trigger_die, TEF_PROCESSED_EFFECTS);
		per_round = false;
		break;
	case COND_DIED_ANY:
		// Died([ANYONE])
		condition = GameScript::EvaluateString(target, "Died([ANYONE])");
		per_round = false;
		break;
	case COND_TURNEDBY:
		// TurnedBy([ANYONE])
		condition = target->GetMatchingTrigger(trigger_turnedby, TEF_PROCESSED_EFFECTS);
		per_round = false;
		break;
	case COND_HP_LT:
		// HPLT(Myself, 'Extra')
		condition = target->GetBase(IE_HITPOINTS) < fx->IsVariable;
		break;
	case COND_HP_PERCENT_LT:
		// HPPercentLT(Myself, 'Extra')
		condition = target->GetBase(IE_HITPOINTS) < (fx->IsVariable * target->GetStat(IE_MAXHITPOINTS)) / 100;
		break;
	case COND_SPELLSTATE:
		// CheckSpellState(Myself,'Extra')
		condition = target->HasSpellState(fx->IsVariable);
		break;
	default:
		condition = false;
	}

	// TODO: EEs have fx->IsVariable overloaded: besides acting as a parameter to some
	// of the above conditions, it's also a bitfield for extra features (see IESDP)
	// meaning only some combinations can work

	if (per_round) {
		// This is a 4xxx trigger which is only checked every round.
		// NOTE: some people consider checking against AdjustedTicks to be a bug in the original
		if (Owner->Type != ST_ACTOR) {
			if (Owner->AdjustedTicks % core->Time.round_size) {
				condition = false;
			}
		} else {
			const Actor *act = (const Actor *) Owner;
			if (Owner->Ticks % act->GetAdjustedTime(core->Time.round_size)) {
				condition = false;
			}
		}
		fx->Parameter4 = 0;
		fx->Parameter5 = 0;
	} else {
		// This is a normal trigger which gets a single opportunity every frame.
		condition = (entry != nullptr);

		// make sure we don't apply once per tick to the same target, potentially triggering 2 actor recursion
		// there could be more than one tick in between successful triggers; trying with a half a round limit
		if (entry && actor->GetGlobalID() == fx->Parameter4 && core->GetGame()->GameTime - fx->Parameter5 < core->Time.defaultTicksPerSec / 2) {
			condition = false;
			fx->Parameter4 = 0;
			fx->Parameter5 = 0;
		}
	}

	if (condition) {
		// The trigger was evaluated as true, cast the spells now.
		ResRef refs[4] = { fx->Resource, fx->Resource2, fx->Resource3, fx->Resource4 };
		// save the current spell ref, so the rest of its effects can be applied afterwards (in case of a surge)
		ResRef OldSpellResRef(Owner->SpellResRef);

		for (unsigned int i = 0; i < 4; i++) {
			if (refs[i].IsEmpty()) {
				continue;
			}
			// Actually, atleast fire shields also have a range check
			if (fx->Parameter2 == COND_GOTHIT) {
				unsigned int dist = GetSpellDistance(refs[i], target, actor->Pos);
				if (!dist) {
					displaymsg->DisplayConstantStringName(HCStrings::ContingencyFail, GUIColors::RED, target);
					continue;
				}
				if (!WithinPersonalRange(target, actor, dist)) {
					//display 'One of the spells has failed.'
					displaymsg->DisplayConstantStringName(HCStrings::ContingencyFail, GUIColors::RED, target);
					continue;
				}
			}
			//core->ApplySpell(refs[i], actor, Owner, fx->Power);
			// no casting animation, no deplete, instant, no interrupt
			Owner->DirectlyCastSpell(actor, refs[i], fx->Power, true, false);

			// save a marker, so we can avoid two fireshielded mages almost instantly kill each other
			fx->Parameter4 = actor->GetGlobalID();
			fx->Parameter5 = core->GetGame()->GameTime;
		}
		Owner->SetSpellResRef(OldSpellResRef);

		if (fx->Parameter3) {
			// Contingencies only run once, remove ourselves.
			return FX_NOT_APPLIED;
		}
	}

	return FX_APPLIED;
}

// 0xe9 Proficiency
int fx_proficiency (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_proficiency(%2d): Value: %d, Stat: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (fx->Parameter2>=MAX_STATS) return FX_NOT_APPLIED;

	//this opcode works only if the previous value was smaller
	if (STAT_GET(fx->Parameter2)<fx->Parameter1) {
		STAT_SET (fx->Parameter2, fx->Parameter1);
	}
	return FX_APPLIED;
}

// 0xea CreateContingency

int fx_create_contingency (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_create_contingency(%2d): Level: %d, Count: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	//this effect terminates in cutscene mode
	if (core->InCutSceneMode()) return FX_NOT_APPLIED;

	if (target->fxqueue.HasEffectWithSource(fx_contingency_ref, fx->SourceRef)) {
		displaymsg->DisplayConstantStringName(HCStrings::ContingencyDupe, GUIColors::WHITE, target);
		return FX_NOT_APPLIED;
	}

	if (target->InParty) {
		auto& dict = core->GetDictionary();

		dict.Set("P0", target->InParty);
		dict.Set("P1", fx->Parameter1);
		dict.Set("P2", fx->Parameter2);
		core->SetEventFlag(EF_SEQUENCER);
		// set also this for GUIMG, since the spell won't be normally cast, but applied
		target->objects.LastSpellOnMe = ResolveSpellNumber(fx->SourceRef);
	}
	return FX_NOT_APPLIED;
}

// 0xeb WingBuffet
int fx_wing_buffet (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_wing_buffet(%2d): Mode: %d, Strength: %d", fx->Opcode, fx->Parameter2, fx->Parameter1);
	static const int coords[16][2]={ {0, 12}, {-4, 9}, {-8, 6}, {-12, 3}, {-16, 0}, {-12, -3}, {-8, -6}, {-4, -9},
	{0, -12}, {4, -9}, {8, -6}, {12, -3}, {16, 0}, {12, 3}, {8, 6}, {4, 9}, };

	// creature immunity is based on creature size in gemrb, but the original is rather cheesy
	// it hardcoded the ids and included some angels, bhaal avatars, tanarri, elementals, fire giants, melissan, ice golem
	if (target->GetAnims()->GetCircleSize() > 5 || target->GetAnims()->GetFlags() & AV_BUFFET_IMMUNITY) {
		return FX_NOT_APPLIED;
	}
	if (!target->GetCurrentArea()) {
		//no area, no movement
		return FX_APPLIED;
	}

	const Game *game = core->GetGame();

	if (fx->FirstApply) {
		fx->Parameter4 = game->GameTime;
		return FX_APPLIED;
	}

	int ticks = game->GameTime-fx->Parameter4;
	if (!ticks)
		return FX_APPLIED;

	//create movement in actor
	orient_t dir;
	switch(fx->Parameter2) {
		case 2: // away
		default:
			dir = GetOrient(target->Pos, fx->Source);
			break;
		case 4: // towards
			dir = GetOrient(fx->Source, target->Pos);
			break;
		case 5: // fixed direction
			dir = ClampToOrientation(fx->Parameter3);
			break;
		case 6: // own direction
			dir = target->GetOrientation();
			break;
		case 7: // back away in own direction
			dir = ReflectOrientation(target->GetOrientation());
			break;
	}
	Point newpos=target->Pos;

	newpos.x += coords[dir][0] * (signed) fx->Parameter1 * ticks / 16;
	newpos.y += coords[dir][1] * (signed) fx->Parameter1 * ticks / 12;

	//change is minimal, lets try later
	if (newpos == target->Pos)
		return FX_APPLIED;

	target->SetPosition(newpos, true);

	fx->Parameter4 = game->GameTime;
	return FX_APPLIED;
}

// 0xec ProjectImage
int fx_puppet_master (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_puppet_master(%2d): Value: %d, Stat: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	//copyself doesn't copy scripts, so the script clearing code is not needed
	Actor *copy = target->CopySelf(fx->Parameter2 == 1);

	Effect *newfx = EffectQueue::CreateUnsummonEffect(fx);
	if (newfx) {
		core->ApplyEffect(newfx, copy, copy);
	}

	ResRef puppetRef;
	switch(fx->Parameter2)
	{
	case 1:
		puppetRef = "mislead";
		//set the gender to illusionary, so ids matching will work
		copy->SetBase(IE_SEX, SEX_ILLUSION);
		copy->SetBase(IE_MAXHITPOINTS, copy->GetBase(IE_MAXHITPOINTS)/2);
		if (copy->GetBase(IE_EA) != EA_ALLY) {
			ResRef script;
			// intentionally 7, to leave room for the last letter
			script.Format("{:.7}m", target->GetScript(SCR_CLASS));
			// if the caster is inparty, the script is turned off by the AI disable flag
			copy->SetScript(script, SCR_CLASS, target->InParty != 0);
		}
		break;
	case 2:
		puppetRef = "projimg";
		copy->SetBase(IE_SEX, SEX_ILLUSION);
		break;
	case 3:
		puppetRef = "simulacr";
		copy->SetBase(IE_SEX, SEX_ILLUSION);
		// healable level drain
		// second generation simulacri are supposedly at a different level, but that makes little sense:
		// level = original caster - caster / 2; eg. lvl 32 -> 16 -> 24 -> 20 -> 22 -> 21
		newfx = EffectQueue::CreateEffect(fx_leveldrain_ref, copy->GetXPLevel(1)/2, 0, FX_DURATION_INSTANT_PERMANENT);
		if (newfx) {
			core->ApplyEffect(newfx, copy, copy);
		}
		break;
	default:
		puppetRef = fx->Resource;
		break;
	}
	if (!puppetRef.IsEmpty()) {
		core->ApplySpell(puppetRef, copy, copy, 0);
	}

	copy->ApplyEffectCopy(fx, fx_puppetmarker_ref, copy, fx->CasterID, fx->Parameter2);
	return FX_NOT_APPLIED;
}

// 0xed PuppetMarker
int fx_puppet_marker (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_puppet_marker(%2d): Value: %d, Stat: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	Actor *master = core->GetGame()->GetActorByGlobalID(fx->Parameter1);
	// selfdestruct if the master is gone
	if (!master || master->Modified[IE_STATE_ID]&STATE_DEAD) {
		target->DestroySelf();
		return FX_NOT_APPLIED;
	}
	STAT_SET (IE_PUPPETMASTERTYPE, fx->Parameter2);
	STAT_SET (IE_PUPPETMASTERID, fx->Parameter1);
	//These will be seen in PrevStats after an update in Master
	master->SetStat(IE_PUPPETID, target->GetGlobalID(), 0);
	master->SetStat(IE_PUPPETTYPE, fx->Parameter2, 0);
	return FX_APPLIED;
}

// 0xee Disintegrate
int fx_disintegrate (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_disintegrate(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	if (target->GetStat(IE_DISABLECHUNKING)) return FX_NOT_APPLIED;
	if (EffectQueue::match_ids( target, fx->Parameter2, fx->Parameter1) ) {
		//convert it to a death opcode or apply the new effect?
		fx->Opcode = EffectQueue::ResolveEffect(fx_death_ref);
		fx->TimingMode = FX_DURATION_INSTANT_PERMANENT;
		fx->Parameter1 = 0;
		fx->Parameter2 = 0x200;
		return FX_APPLIED;
	}
	return FX_NOT_APPLIED;
}

// 0xef Farsee
// 1 view not explored sections too
// 2 param1=range (otherwise visualrange)
// 4 point already set (otherwise use gui)
// 8 use line of sight
int fx_farsee (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	Map *map = target->GetCurrentArea();
	if (!map) {
		return FX_APPLIED;
	}

	if (!(fx->Parameter2 & 2)) {
		fx->Parameter1=STAT_GET(IE_VISUALRANGE);
		fx->Parameter2 |= 2;
	}

	// don't open the map window if actor isn't in party
	if (target->InParty && !(fx->Parameter2 & 4)) {
		// start graphical interface
		// it will do all the rest of the opcode
		// using RevealMap guiscript action
		core->EventFlag |= EF_SHOWMAP;
		return FX_NOT_APPLIED;
	}

	//don't explore unexplored points
	if (!(fx->Parameter2 & 1)) {
		if (!map->IsExplored(fx->Pos)) {
			return FX_NOT_APPLIED;
		}
	}
	map->ExploreMapChunk(fx->Pos, fx->Parameter1, fx->Parameter2 & 8);
	return FX_NOT_APPLIED;
}

// 0xf0 Icon:Remove
int fx_remove_portrait_icon (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_remove_portrait_icon(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	target->fxqueue.RemoveAllEffectsWithParam( fx_display_portrait_icon_ref, fx->Parameter2 );
	return FX_NOT_APPLIED;
}
// 0xf1 control creature (same as charm)

// 0xF2 Cure:Confusion
int fx_cure_confused_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_cure_confused_state(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	BASE_STATE_CURE( STATE_CONFUSED );
	target->fxqueue.RemoveAllEffects(fx_confused_state_ref);
	//oddly enough, HoW removes the confused icon
	//no one removes the rigid thinking icon
	//there are also several mods floating around, which change these things inconsistently
	//probably the best is to remove them all by default
	//New mods can still disable the icon removal by setting param2
	if (!fx->Parameter2) {
		target->fxqueue.RemoveAllEffectsWithParam( fx_display_portrait_icon_ref,PI_CONFUSED );
		target->fxqueue.RemoveAllEffectsWithParam( fx_display_portrait_icon_ref,PI_RIGID );
	}
	return FX_NOT_APPLIED;
}

// 0xf3 DrainItems (worked in SoA, got disabled in ToB)
int fx_drain_items (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// TODO: ee, optionally use fx->Resource, different draining, add extra gameflag check below
	if (core->HasFeature(GFFlags::FIXED_MORALE_OPCODE)) return FX_NOT_APPLIED;

	// deplete magic items = 0
	// deplete also magic weapons = 1
	target->inventory.DepleteItem(fx->Parameter1);
	return FX_NOT_APPLIED;
}
// 0xf4 DrainSpells
int fx_drain_spells (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_drain_spells(%2d): Count: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	ieDword i=fx->Parameter1;
	int type = fx->Parameter2 ? IE_SPELL_TYPE_PRIEST : IE_SPELL_TYPE_WIZARD;
	while(i--) {
		if (!target->spellbook.DepleteSpell(type)) {
			break;
		}
	}
	return FX_NOT_APPLIED;
}
// 0xf5 CheckForBerserk
int fx_checkforberserk_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_checkforberserk_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	STAT_SET( IE_CHECKFORBERSERK, fx->Parameter2 );
	return FX_APPLIED;
}
// 0xf6 BerserkStage1Modifier
int fx_berserkstage1_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_berserkstage1_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	STAT_SET( IE_BERSERKSTAGE1, fx->Parameter2 );
	return FX_APPLIED;
}
// 0xf7 BerserkStage2Modifier
int fx_berserkstage2_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_berserkstage2_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	STAT_SET( IE_BERSERKSTAGE2, fx->Parameter2 );
	STATE_SET (STATE_BERSERK);
	return FX_APPLIED;
}

// 0xf8 set melee effect
// adds effect to melee attacks (for monks, assassins, fighter hlas, ...)
// it is cumulative

// 0xf9 set missile effect
// adds effect to ranged attacks (archers, ...)
// it is cumulative

// 0xfa DamageLuckModifier
int fx_damageluck_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_damageluck_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	STAT_MOD( IE_DAMAGELUCK );
	return FX_APPLIED;
}

// 0xfb BardSong

int fx_change_bardsong (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_change_bardsong(%2d): %s", fx->Opcode, fx->Resource);
	// remove any previous song effects, as they are used with permanent timing
	unsigned int songType = IE_SPL_SONG;
	if (core->HasFeature(GFFlags::RULES_3ED)) songType = IE_IWD2_SPELL_SONG;
	unsigned int count = target->fxqueue.CountEffects(fx_change_bardsong_ref, -1, -1);
	unsigned int songCount = target->spellbook.GetSpellInfoSize(1 << songType);
	if (count > 0 && songCount > 0) {
		for (unsigned int i=0; i<songCount; i++) {
			if (i == fx->Parameter2) continue;
			target->fxqueue.RemoveAllEffectsWithParam(fx_change_bardsong_ref, i);
		}
	}
	target->BardSong = fx->Resource;
	return FX_APPLIED;
}

// 0xfc SetTrap
int fx_set_area_effect (Scriptable* Owner, Actor* target, Effect* fx)
{
	ieDword skill = 0;
	ieDword roll = 0;
	ieDword level = 0;
	const Map *map = target->GetCurrentArea();
	if (!map || !Owner) return FX_NOT_APPLIED;

	proIterator iter;

	// check if the new trap count is cheesy (only saved traps count)
	if (map->GetTrapCount(iter) + 1 > gamedata->GetTrapLimit(Owner)) {
		displaymsg->DisplayConstantStringName(HCStrings::NoMoreTraps, GUIColors::WHITE, target);
		return FX_NOT_APPLIED;
	}

	//check if we are under attack
	if (GetNearestEnemyOf(map, target, ORIGIN_SEES_ENEMY|ENEMY_SEES_ORIGIN)) {
		displaymsg->DisplayConstantStringName(HCStrings::MayNotSetTrap, GUIColors::WHITE, target);
		return FX_NOT_APPLIED;
	}

	const Actor* caster = Scriptable::As<const Actor>(Owner);
	if (caster) {
		skill = caster->GetStat(IE_SETTRAPS);
		roll = target->LuckyRoll(1,100,0,LR_NEGATIVE);
		// assuming functioning thief, but allowing modded exceptions
		// thieves aren't casters, so 0 for a later spell type lookup is not good enough
		level = caster->GetThiefLevel();
		level = level ? level : caster->GetXPLevel(false);
	}

	if (roll>skill) {
		//failure
		displaymsg->DisplayConstantStringName(HCStrings::SnareFailed, GUIColors::WHITE, target);
		if (target->LuckyRoll(1,100,0)<25) {
			ResRef spl;
			spl.Format("{:.7}F", fx->Resource);
			core->ApplySpell(spl, target, Owner, fx->Power);
		}
		return FX_NOT_APPLIED;
	}
	//success
	displaymsg->DisplayConstantStringName(HCStrings::SnareSucceed, GUIColors::WHITE, target);
	target->VerbalConstant(Verbal::TrapSet);
	// save the current spell ref, so the rest of its effects can be applied afterwards
	ResRef OldSpellResRef(Owner->SpellResRef);
	Owner->DirectlyCastSpellPoint(fx->Pos, fx->Resource, level, true, false);
	Owner->SetSpellResRef(OldSpellResRef);
	return FX_NOT_APPLIED;
}

// 0xfd SetMapNote
int fx_set_map_note (Scriptable* Owner, Actor* target, Effect* fx)
{
	// print("fx_set_map_note(%2d): StrRef: %d Color: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	const Scriptable *marker = target ? target : Owner;
	Map *map = marker->GetCurrentArea();
	if (!map) return FX_APPLIED; //delay effect
	map->AddMapNote(fx->Pos, fx->Parameter2, ieStrRef(fx->Parameter1));
	return FX_NOT_APPLIED;
}

// 0xfe RemoveMapNote
int fx_remove_map_note (Scriptable* Owner, Actor* target, Effect* fx)
{
	// print("fx_remove_map_note(%2d)", fx->Opcode);
	const Scriptable *marker = target ? target : Owner;
	Map *map = marker->GetCurrentArea();
	if (!map) return FX_APPLIED; //delay effect
	map->RemoveMapNote(fx->Pos);
	return FX_NOT_APPLIED;
}

// 0xff Item:CreateDays
int fx_create_item_days (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	Actor* receiver = target;
	if (target->GetBase(IE_EA) == EA_FAMILIAR) {
		receiver = core->GetGame()->FindPC(1);
	}
	receiver->inventory.AddSlotItemRes(fx->Resource, SLOT_ONLYINVENTORY, fx->Parameter1, fx->Parameter3, fx->Parameter4);

	int ret = MaybeTransformTo(fx_remove_inventory_item_ref, fx);
	// duration needs recalculating for days
	// no idea if this multiplier is ok
	if (ret == FX_APPLIED) fx->Duration += (fx->Duration - core->GetGame()->GameTime) * core->Time.day_sec / 3;
	return ret;
}

// 0x100 Sequencer:Store
int fx_store_spell_sequencer(Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_store_spell_sequencer(%2d)", fx->Opcode);
	//just display the spell sequencer portrait icon
	target->AddPortraitIcon(PI_SEQUENCER);
	if (fx->FirstApply && fx->Parameter3) {
		if (gamedata->Exists(fx->Resource, IE_SPL_CLASS_ID)) {
			target->spellbook.HaveSpell(fx->Resource, HS_DEPLETE);
		}
		if (gamedata->Exists(fx->Resource2, IE_SPL_CLASS_ID)) {
			target->spellbook.HaveSpell(fx->Resource2, HS_DEPLETE);
		}
		if (gamedata->Exists(fx->Resource3, IE_SPL_CLASS_ID)) {
			target->spellbook.HaveSpell(fx->Resource3, HS_DEPLETE);
		}
		if (gamedata->Exists(fx->Resource4, IE_SPL_CLASS_ID)) {
			target->spellbook.HaveSpell(fx->Resource4, HS_DEPLETE);
		}
	}
	return FX_APPLIED;
}

// 0x101 Sequencer:Create
int fx_create_spell_sequencer(Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_create_spell_sequencer(%2d): Level: %d, Count: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	if (target->fxqueue.HasEffectWithSource(fx_spell_sequencer_active_ref, fx->SourceRef)) {
		displaymsg->DisplayConstantStringName(HCStrings::SequencerDupe, GUIColors::WHITE, target);
		return FX_NOT_APPLIED;
	}
	//just a call to activate the spell sequencer creation gui
	if (target->InParty) {
		auto& vars = core->GetDictionary();

		vars.Set("P0", target->InParty);
		vars.Set("P1", fx->Parameter1);
		vars.Set("P2", fx->Parameter2 | (2 << 16));
		core->SetEventFlag(EF_SEQUENCER);
	}
	return FX_NOT_APPLIED;
}

// 0x102 Sequencer:Activate

int fx_activate_spell_sequencer(Scriptable* Owner, Actor* target, Effect* fx)
{
	// print("fx_activate_spell_sequencer(%2d): Resource: %s", fx->Opcode, fx->Resource);
	Actor* actor = Scriptable::As<Actor>(Owner);
	if (!actor) {
		return FX_NOT_APPLIED;
	}

	Effect *sequencer = actor->fxqueue.HasEffect(fx_spell_sequencer_active_ref);
	if (!sequencer) {
		return FX_NOT_APPLIED;
	}

	//cast 1-4 spells stored in the spell sequencer
	Owner->DirectlyCastSpell(target, sequencer->Resource, fx->CasterLevel, false, false);
	Owner->DirectlyCastSpell(target, sequencer->Resource2, fx->CasterLevel, false, false);
	Owner->DirectlyCastSpell(target, sequencer->Resource3, fx->CasterLevel, false, false);
	Owner->DirectlyCastSpell(target, sequencer->Resource4, fx->CasterLevel, false, false);

	//remove the spell sequencer store effect
	sequencer->TimingMode = FX_DURATION_JUST_EXPIRED;
	return FX_NOT_APPLIED;
}

// 0x103 SpellTrap (Protection:SpellLevelDec + recall spells)
int fx_spelltrap(Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_spelltrap(%2d): Count: %d, Level: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	if (fx->Parameter3) {
		target->RestoreSpellLevel(fx->Parameter3, 0);
		fx->Parameter3 = 0;
	}
	if (fx->Parameter1 <=0 || STATE_GET(STATE_DEAD)) {
		//gone down to zero
		return FX_NOT_APPLIED;
	}
	target->SetOverlay(OV_SPELLTRAP);
	target->AddPortraitIcon(PI_SPELLTRAP);
	return FX_APPLIED;
}

//0x104 Crash104
//0x138 Crash138
int fx_crash (Scriptable* /*Owner*/, Actor* /*target*/, Effect* /*fx*/)
{
	// print("fx_crash(%2d): Param1: %d, Param2: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	return FX_NOT_APPLIED;
}

// 0x105 RestoreSpells
int fx_restore_spell_level(Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_restore_spell_level(%2d): Level: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	target->RestoreSpellLevel(fx->Parameter1, fx->Parameter2);
	return FX_NOT_APPLIED;
}
// 0x106 VisualRangeModifier
int fx_visual_range_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_visual_range_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	STAT_MOD( IE_VISUALRANGE );
	return FX_APPLIED;
}

// 0x107 BackstabModifier
int fx_backstab_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_visual_range_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	//this is how it is done in the original engine, i don't know why they would do this
	//ctrl-r would probably remove it otherwise
	//Why they didn't fix it in the spell/item is beyond me
	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT)
		fx->TimingMode=FX_DURATION_INSTANT_PERMANENT_AFTER_BONUSES;
	STAT_MOD( IE_BACKSTABDAMAGEMULTIPLIER );
	return FX_APPLIED;
}

// 0x108 DropWeapon
int fx_drop_weapon (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (!fx->Resource.IsEmpty()) {
		target->DropItem(fx->Resource, 0);
		return FX_NOT_APPLIED;
	}
	switch (fx->Parameter2) {
		case 0:
			target->DropItem(-1, 0);
			break;
		case 1:
			target->DropItem(target->inventory.GetEquippedSlot(), 0);
			break;
		default:
			target->DropItem(fx->Parameter1, 0);
			break;
	}
	return FX_NOT_APPLIED;
}
// 0x109 ModifyGlobalVariable
int fx_modify_global_variable (Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	Game *game = core->GetGame();
	if (!fx->IsVariable) {
		//convert it to internal variable format by shifting to overwrite the null terminators
		memmove(&fx->VariableName[8], &fx->Resource2, 8);
		memmove(&fx->VariableName[16], &fx->Resource3, 8);
		memmove(&fx->VariableName[24], &fx->Resource4, 8);
		fx->IsVariable=1;
	}

	//hack for IWD
	if (fx->Resource.IsEmpty()) {
		fx->VariableName = "RETURN_TO_LONELYWOOD";
	}

	ieVariable key{fx->Resource};
	// print("fx_modify_global_variable(%2d): Variable: %s Value: %d Type: %d", fx->Opcode, fx->Resource, fx->Parameter1, fx->Parameter2);
	if (fx->Parameter2) {
		//use resource memory area as variable name
		auto lookup = game->locals.find(key);
		if (lookup != game->locals.cend()) {
			lookup->second += fx->Parameter1;
		} else {
			game->locals[key] = fx->Parameter1;
		}
	} else {
		game->locals[key] = fx->Parameter1;
	}
	return FX_NOT_APPLIED;
}

// 0x10a RemoveImmunity
int fx_remove_immunity(Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_remove_immunity(%2d): %s", fx->Opcode, fx->Resource);
	target->fxqueue.RemoveAllEffectsWithResource(fx_immunity_effect_ref, fx->Resource);
	return FX_NOT_APPLIED;
}

// 0x10b protection from display string
// we can't use a generic effect, because the unused Parameter2 differs with uses in fx_display_string
int fx_protection_from_string(Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	fx->Parameter2 = 0;
	return FX_APPLIED;
}

// 0x10c ExploreModifier
int fx_explore_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_explore_modifier(%2d)", fx->Opcode);
	if (fx->Parameter2) {
		//gemrb modifier
		STAT_SET (IE_EXPLORE, fx->Parameter1);
	} else {
		STAT_SET (IE_EXPLORE, 1);
	}
	return FX_APPLIED;
}
// 0x10d ScreenShake
int fx_screenshake (Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	// print("fx_screenshake(%2d): Strength: %d", fx->Opcode, fx->Parameter1);
	int count;

	if (fx->TimingMode!=FX_PERMANENT) {
		count = fx->Duration-core->GetGame()->GameTime;
	} else {
		count = core->Time.round_size;
	}

	Point shake;
	switch(fx->Parameter2) {
	case 0: default:
		shake.x = fx->Parameter1;
		shake.y = fx->Parameter1;
		break;
	case 1:
		shake.x = fx->Parameter1;
		shake.y = -signed(fx->Parameter1);
		break;
	case 2:
		//gemrb addition
		shake.x = fx->Parameter1 & 0xffff;
		shake.y = fx->Parameter1 >> 16;
		break;
	}
	core->timer.SetScreenShake(shake, count);
	return FX_NOT_APPLIED;
}

// 0x10e Cure:CasterHold
int fx_unpause_caster (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_unpause_caster(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	target->fxqueue.RemoveAllEffects(fx_pause_caster_modifier_ref);
	// unsure, but makes sense
	target->SetWait(0);
	return FX_NOT_APPLIED;
}

// 0x10f SummonDisable (bg2)
int fx_summon_disable (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_summon_disable(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	STAT_SET(IE_SUMMONDISABLE, 1);
	STAT_SET(IE_CASTERHOLD, 1);
	if (fx->Parameter2==1) {
		STAT_SET(IE_AVATARREMOVAL, 1);
	}
	return FX_APPLIED;
}

/* note from Taimon:
What happens at a lower level is that the engine recreates the entire stats on some changes to the creature. The application of an effect to the creature is one such change.
Since the repeating effects are stored in a list inside those stats, they are being recreated every ai update, if there has been an effect application.
The repeating effect itself internally uses a counter to store how often it has been called. And when this counter equals the period it fires of the effect.
When the list is being recreated all those counters are lost.
*/
// 0x110 ApplyEffectRepeat
int fx_apply_effect_repeat (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_apply_effect_repeat(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	// don't apply the effect if a similar one is already applied with a shorter duration
	const Effect *oldfx = target->fxqueue.HasEffect(fx_apply_effect_repeat_ref);
	if (oldfx && oldfx->Duration < fx->Duration) {
		return FX_NOT_APPLIED;
	}
	
	Effect *newfx = core->GetEffect(fx->Resource, fx->Power, fx->Pos);
	if (!newfx) {
		return FX_NOT_APPLIED;
	}

	// fx->Parameter4 is an optional frequency multiplier
	ieDword aRound = (fx->Parameter4 ? fx->Parameter4 : 1) * core->Time.defaultTicksPerSec;
	Scriptable *caster = GetCasterObject();
	switch (fx->Parameter2) {
		case 0: //once per second
		case 1: //crash???
			if (!(core->GetGame()->GameTime % target->GetAdjustedTime(aRound))) {
				core->ApplyEffect(newfx, target, caster);
			} else {
				delete newfx;
			}
			break;
		case 2://param1 times every second
			if (!(core->GetGame()->GameTime % target->GetAdjustedTime(aRound))) {
				for (ieDword i=0; i < fx->Parameter1; i++) {
					core->ApplyEffect(new Effect(*newfx), target, caster);
				}
			}
			delete newfx;
			break;
		case 3: //once every Param1 second
			if (fx->Parameter1 && !(core->GetGame()->GameTime % target->GetAdjustedTime(fx->Parameter1 * aRound))) {
				core->ApplyEffect(newfx, target, caster);
			} else {
				delete newfx;
			}
			break;
		case 4: //param3 times every Param1 second
			if (fx->Parameter1 && !(core->GetGame()->GameTime % target->GetAdjustedTime(fx->Parameter1 * aRound))) {
				for (ieDword i=0; i < fx->Parameter3; i++) {
					core->ApplyEffect(new Effect(*newfx), target, caster);
				}
			}
			delete newfx;
			break;
		default:
			delete newfx;
			break;
	}

	if (fx->IsVariable) target->AddPortraitIcon(static_cast<ieByte>(fx->IsVariable));

	return FX_APPLIED;
}

// 0x111 RemoveProjectile
int fx_remove_projectile (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	//instant effect
	// print("fx_remove_projectile(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (!target) return FX_NOT_APPLIED;
	const Map *area = target->GetCurrentArea();
	if (!area) return FX_NOT_APPLIED;
	
	auto HandleProjectile = [&](ieDword projectile) {
		proIterator piter;

		size_t cnt = area->GetProjectileCount(piter);
		while( cnt--) {
			Projectile *pro = *piter;
			if ((pro->GetType()==projectile) && pro->PointInRadius(fx->Pos)) {
				pro->Cleanup();
			}
		}
		if (target) {
			target->fxqueue.RemoveAllEffectsWithProjectile(projectile);
		}
	};

	ResRef listref;
	switch (fx->Parameter2) {
	case 0: //standard bg2
		listref = "clearair";
		break;
	case 1: //you can give a 2da for projectile list (gemrb)
		listref = fx->Resource;
		break;
	case 2: //or you can give one single projectile in param1 (gemrb)
		HandleProjectile(fx->Parameter1);
			return FX_NOT_APPLIED;
	default:
		return FX_NOT_APPLIED;
	}
	
	//the list is now cached by Interface, no need of freeing it
	const std::vector<ieDword>* projectilelist = core->GetListFrom2DA(listref);
	assert(projectilelist);
	
	for (const auto projectile : *projectilelist) {
		HandleProjectile(projectile);
	}

	return FX_NOT_APPLIED;
}

// 0x112 TeleportToTarget
int fx_teleport_to_target (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_teleport_to_target(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	if (STATE_GET(STATE_DEAD)) {
		return FX_NOT_APPLIED;
	}

	const Map *map = target->GetCurrentArea();
	if (map) {
		Object oC;
		oC.objectFields[0]=EA_ENEMY;
		Targets *tgts = GetAllObjects(map, target, &oC, GA_NO_DEAD);
		if (!tgts) {
			// no enemy to jump to
			return FX_NOT_APPLIED;
		}
		int rnd = core->Roll(1,tgts->Count(),-1);
		const Actor *victim = (Actor *) tgts->GetTarget(rnd, ST_ACTOR);
		delete tgts;
		if (victim && PersonalDistance(victim, target)>20) {
			target->SetPosition(victim->Pos, true);
			target->SetColorMod(0xff, RGBModifier::ADD, 0x50, Color(0xff, 0xff, 0xff, 0), 0);
		}
	}
	return FX_NOT_APPLIED;
}

// 0x113 HideInShadowsModifier
int fx_hide_in_shadows_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_hide_in_shadows_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	STAT_MOD( IE_HIDEINSHADOWS );
	return FX_APPLIED;
}

// 0x114 DetectIllusionsModifier
int fx_detect_illusion_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_detect_illusion_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	STAT_MOD( IE_DETECTILLUSIONS );
	return FX_APPLIED;
}

// 0x115 SetTrapsModifier
int fx_set_traps_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_set_traps_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	STAT_MOD( IE_SETTRAPS );
	return FX_APPLIED;
}
// 0x116 ToHitBonusModifier
int fx_to_hit_bonus_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_to_hit_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	STAT_MOD(IE_HITBONUS);
	return FX_PERMANENT;
}

// 0x117 RenableButton
int fx_renable_button (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	//removes the disable button effect
	// print("fx_renable_button(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	target->fxqueue.RemoveAllEffectsWithParamAndResource( fx_disable_button_ref, fx->Parameter2, fx->Resource );
	return FX_NOT_APPLIED;
}

// 0x118 ForceSurgeModifier
int fx_force_surge_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_force_surge_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	STAT_MOD_VAR( IE_FORCESURGE, MOD_ABSOLUTE );
	return FX_APPLIED;
}

// 0x119 WildSurgeModifier
int fx_wild_surge_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_wild_surge_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	STAT_MOD( IE_SURGEMOD );
	return FX_APPLIED;
}

// 0x11a ScriptingState
int fx_scripting_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_scripting_state(%2d): Value: %d, Stat: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	//original engine didn't check boundaries, causing crashes
	//we allow only positive indices (some extra stats are still addressable)
	if (fx->Parameter2>100) {
		return FX_NOT_APPLIED;
	}
	//original engine used only single byte value, we allow full dword
	STAT_SET( IE_SCRIPTINGSTATE1+fx->Parameter2, fx->Parameter1 );
	return FX_APPLIED;
}

// 0x11b ApplyEffectCurse
int fx_apply_effect_curse (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_apply_effect_curse(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	//this effect executes a file effect in place of this effect
	//the file effect inherits the target and the timingmode, but gets
	//a new chance to roll percents
	int ret = FX_NOT_APPLIED;
	if (!target) {
		return ret;
	}

	if (EffectQueue::match_ids( target, fx->Parameter2, fx->Parameter1) ) {
		//apply effect, if the effect is a goner, then kill
		//this effect too
		Effect *myfx = core->GetEffect(fx->Resource, fx->Power, fx->Pos);
		if (myfx) {
			myfx->RandomValue = fx->RandomValue;
			myfx->TimingMode=fx->TimingMode;
			myfx->Duration=fx->Duration;
			myfx->Target = FX_TARGET_PRESET;
			myfx->CasterID = fx->CasterID;
			ret = target->fxqueue.ApplyEffect(target, myfx, fx->FirstApply, 0);
			delete myfx;
		}
		//newfx is a borrowed reference don't delete it
	}
	return ret;
}

// 0x11c MeleeHitModifier
int fx_melee_to_hit_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_melee_to_hit_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	STAT_MOD( IE_MELEETOHIT );
	return FX_APPLIED;
}

// 0x11d MeleeDamageModifier
int fx_melee_damage_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_melee_damage_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	STAT_MOD( IE_MELEEDAMAGE );
	return FX_APPLIED;
}

// 0x11e MissileDamageModifier
int fx_missile_damage_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_missile_damage_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	STAT_MOD( IE_MISSILEDAMAGE );
	return FX_APPLIED;
}

// 0x11f NoCircleState
int fx_no_circle_state (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_no_circle_state(%2d)", fx->Opcode);
	STAT_SET( IE_NOCIRCLE, 1 );
	return FX_APPLIED;
}

// 0x120 FistHitModifier
int fx_fist_to_hit_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_fist_to_hit_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	STAT_MOD( IE_FISTHIT );
	return FX_APPLIED;
}

// 0x121 FistDamageModifier
int fx_fist_damage_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_fist_damage_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	STAT_MOD( IE_FISTDAMAGE );
	return FX_APPLIED;
}

//0x122 TitleModifier
// ees use fx->IsVariable to set which class' title to change, but the way we use titles currently makes that moot
int fx_title_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (fx->Parameter2) {
		STAT_SET(IE_TITLE2, fx->Parameter1);
	} else {
		STAT_SET(IE_TITLE1, fx->Parameter1);
	}
	return FX_APPLIED;
}

//0x123 DisableOverlay
// Blocks the hardcoded animations applied by opcodes:
// - 201, 204, 205, 223, 259 (SPMAGGLO - OV_SPELLTRAP)
// - 197, 198, 200, 202, 203, 207, 226, 227, 228, 299 (SPTURNI2 - OV_BOUNCE / pcf_bounce)
int fx_disable_overlay_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	STAT_SET(IE_DISABLEOVERLAY, fx->Parameter2);
	return FX_APPLIED;
}

//0x124 Protection:Backstab (bg2)
//0x11f Protection:Backstab (how, iwd2)
//3 different games, 3 different methods of flagging this
int fx_no_backstab_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_no_backstab_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	//bg2
	STAT_SET(IE_DISABLEBACKSTAB, fx->Parameter2);
	//how
	EXTSTATE_SET(EXTSTATE_NO_BACKSTAB);
	//iwd2
	target->SetSpellState(SS_NOBACKSTAB);
	return FX_APPLIED;
}

//0x125 OffscreenAIModifier
int fx_offscreenai_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	STAT_SET(IE_ENABLEOFFSCREENAI, fx->Parameter2);
	target->Activate();
	return FX_APPLIED;
}

//0x126 ExistanceDelayModifier
int fx_existence_delay_modifier(Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	STAT_SET(IE_EXISTANCEDELAY, fx->Parameter2);
	return FX_APPLIED;
}

//0x127 DisableChunk / DisablePermanentDeath
// protects against chunking, disintegration, permanent death from the kill opcode (causes normal death instead)
// doesn't prevent normal petrification (from the opcode) and doesn't protect from normal freezing (eg. from Cone of Cold)
int fx_disable_chunk_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	STAT_SET(IE_DISABLECHUNKING, fx->Parameter2);
	return FX_APPLIED;
}

//0x128 Protection:Animation - disable 0xd7 if fx->Resource matches

//0x129 Protection:Turn
int fx_protection_from_turn (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	STAT_SET(IE_NOTURNABLE, fx->Parameter2);
	return FX_APPLIED;
}

//0x12a CutScene2
//runs a predetermined script in cutscene mode
int fx_cutscene2 (Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	Game *game;

	// print("fx_cutscene2(%2d): Locations: %d Resource: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	if (core->InCutSceneMode()) return FX_NOT_APPLIED;
	game = core->GetGame();
	if (!game) return FX_NOT_APPLIED;

	switch(fx->Parameter1) {
	case 1://simple party locations
		game->ClearSavedLocations();
		for (int i = 0; i < game->GetPartySize(false); i++) {
			const Actor* act = game->GetPC(i, false);
			GAMLocationEntry *gle = game->GetSavedLocationEntry(i);
			if (act && gle) {
				gle->Pos = act->Pos;
				gle->AreaResRef = act->Area;
			}
		}
		break;
	case 2: //no store
		break;
	default://original plane locations
		game->ClearPlaneLocations();
		for (int i = 0; i < game->GetPartySize(false); i++) {
			const Actor* act = game->GetPC(i, false);
			GAMLocationEntry *gle = game->GetPlaneLocationEntry(i);
			if (act && gle) {
				gle->Pos = act->Pos;
				gle->AreaResRef = act->Area;
			}
		}
	}

	core->SetCutSceneMode(true);

	ResRef resRef;
	//GemRB enhancement: allow a custom resource
	if (fx->Parameter2) {
		resRef = fx->Resource;
	} else {
		resRef = "cut250a";
	}

	GameScript* gs = new GameScript(resRef, game);
	gs->EvaluateAllBlocks();
	delete gs;
	return FX_NOT_APPLIED;
}

//0x12b ChaosShieldModifier
int fx_chaos_shield_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_chaos_shield_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	STAT_ADD( IE_CHAOSSHIELD, fx->Parameter1 );
	if (fx->Parameter2) {
		target->AddPortraitIcon(PI_CSHIELD); //162
	} else {
		target->AddPortraitIcon(PI_CSHIELD2); //163
	}
	target->SetOverlay(OV_BOUNCE);
	return FX_APPLIED;
}

//0x12c NPCBump
int fx_npc_bump (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_npc_bump(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	//unknown effect, but known stat position
	STAT_MOD( IE_NPCBUMP );
	return FX_APPLIED;
}
//0x12d CriticalHitModifier
int fx_critical_hit_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// EE use extra parameters, but they all default to 0, so we don't need to ifdef anything
	const WeaponInfo& wi = target->weaponInfo[target->usedLeftHand];
	if (!Actor::IsCriticalEffectEligible(wi, fx)) return FX_NOT_APPLIED;

	STAT_MOD(IE_CRITICALHITBONUS);

	return FX_APPLIED;
}
// 0x12e CanUseAnyItem
int fx_can_use_any_item_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_can_use_any_item_modifier(%2d): Value: %d", fx->Opcode, fx->Parameter2);

	STAT_SET( IE_CANUSEANYITEM, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x12f AlwaysBackstab
// the two extra TobEx bits are handled in the user
int fx_always_backstab_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_always_backstab_modifier(%2d): Value: %d", fx->Opcode, fx->Parameter2);

	STAT_SET( IE_ALWAYSBACKSTAB, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x130 MassRaiseDead
int fx_mass_raise_dead (Scriptable* Owner, Actor* /*target*/, Effect* fx)
{
	// print("fx_mass_raise_dead(%2d)", fx->Opcode);

	const Game *game=core->GetGame();
	int i=game->GetPartySize(false);
	while (i--) {
		Actor *actor=game->GetPC(i,false);
		Resurrect(Owner, actor, fx, fx->Pos);
	}
	return FX_NOT_APPLIED;
}

// 0x131 OffhandHitModifier
int fx_left_to_hit_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_left_to_hit_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	STAT_MOD( IE_HITBONUSLEFT );
	return FX_APPLIED;
}

// 0x132 RightHitModifier
int fx_right_to_hit_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_right_to_hit_modifier(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	STAT_MOD( IE_HITBONUSRIGHT );
	return FX_APPLIED;
}

// 0x133 Reveal:Tracks
int fx_reveal_tracks (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_reveal_tracks(%2d): Distance: %d", fx->Opcode, fx->Parameter1);
	const Map *map = target->GetCurrentArea();
	if (!map) return FX_APPLIED;
	if (!fx->Parameter2) {
		fx->Parameter2=1;
		//write tracks.2da entry
		if (map->DisplayTrackString(target)) {
			return FX_NOT_APPLIED;
		}
	}

	// iwd tracking is just the area description
	if (core->HasFeature(GFFlags::IWD2_SCRIPTNAME)) {
		return FX_NOT_APPLIED;
	}

	GameControl *gc = core->GetGameControl();
	if (gc) {
		//highlight all living creatures (not in party, but within range)
		gc->SetTracker(target, fx->Parameter1);
	}
	return FX_APPLIED;
}

// 0x134 Protection:Tracking
int fx_protection_from_tracking (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_protection_from_tracking(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	STAT_MOD( IE_NOTRACKING ); //highlight creature???
	return FX_APPLIED;
}
// 0x135 ModifyLocalVariable
int fx_modify_local_variable (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (!fx->IsVariable) {
		//convert it to internal variable format by shifting to overwrite the null terminators
		memmove(&fx->VariableName[8], &fx->Resource2, 8);
		memmove(&fx->VariableName[16], &fx->Resource3, 8);
		memmove(&fx->VariableName[24], &fx->Resource4, 8);
		fx->IsVariable=1;
	}

	ieVariable key{fx->Resource};
	// print("fx_modify_local_variable(%2d): %s, Mod: %d", fx->Opcode, fx->Resource, fx->Parameter2);
	if (fx->Parameter2) {
		//use resource memory area as variable name
		auto lookup = target->locals.find(key);
		if (lookup != target->locals.cend()) {
			lookup->second += fx->Parameter1;
		} else {
			target->locals[key] = fx->Parameter1;
		}
	} else {
		target->locals[key] = fx->Parameter1;
	}
	return FX_NOT_APPLIED;
}

// 0x136 TimelessState
int fx_timeless_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_timeless_modifier(%2d): Mod: %d", fx->Opcode, fx->Parameter2);
	STAT_SET(IE_DISABLETIMESTOP, fx->Parameter2);
	return FX_APPLIED;
}

//0x137 GenerateWish
//GemRB extension: you can use different tables and not only wisdom stat
int fx_generate_wish (Scriptable* Owner, Actor* target, Effect* fx)
{
	// print("fx_generate_wish(%2d): Mod: %d", fx->Opcode, fx->Parameter2);
	if (!fx->Parameter2) {
		fx->Parameter2=IE_WIS;
	}
	int stat = target->GetSafeStat(fx->Parameter2);
	if (fx->Resource.IsEmpty()) {
		fx->Resource = "wishcode";
	}
	AutoTable tm = gamedata->LoadTable(fx->Resource);
	if (!tm) {
		return FX_NOT_APPLIED;
	}

	TableMgr::index_t max = tm->GetRowCount();
	TableMgr::index_t tmp = RAND<TableMgr::index_t>(1, max);
	TableMgr::index_t i = tmp;
	bool pass = true;
	while(--i!=tmp && pass) {
		if (i == TableMgr::npos) {
			pass=false;
			i=max-1;
		}
		int statMin = tm->QueryFieldSigned<int>(i, 1);
		int statMax = tm->QueryFieldSigned<int>(i, 2);
		if (stat >= statMin && stat <= statMax) break;
	}

	ResRef spl;
	spl = tm->QueryField(i, 0);
	core->ApplySpell(spl, target, Owner, fx->Power);
	return FX_NOT_APPLIED;
}

// commented out due to g++ -Wall: 'int fx_immunity_sequester(GemRB::Scriptable*, GemRB::Actor*, GemRB::Effect*)' defined but not used [-Werror=unused-function]
//0x138 //see fx_crash, this effect is not fully enabled in original bg2/tob
// static int fx_immunity_sequester (Scriptable* /*Owner*/, Actor* target, Effect* fx)
// {
// 	// print("fx_immunity_sequester(%2d): Mod: %d", fx->Opcode, fx->Parameter2);
// 	//this effect is supposed to provide immunity against sequester (maze/etc?)
// 	STAT_SET(IE_NOSEQUESTER, fx->Parameter2);
// 	return FX_APPLIED;
// }

//0x139 //HLA generic effect
//0x13a StoneSkin2Modifier
int fx_golem_stoneskin_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_golem_stoneskin_modifier(%2d): Mod: %d", fx->Opcode, fx->Parameter1);
	if (!fx->Parameter1) {
		PlayRemoveEffect(target, fx, "EFF_E02");
		return FX_NOT_APPLIED;
	}
	//dead actors lose this effect
	if (STATE_GET( STATE_DEAD) ) {
		return FX_NOT_APPLIED;
	}

	STAT_SET(IE_STONESKINSGOLEM, fx->Parameter1);
	SetGradient(target, fullstone);
	return FX_APPLIED;
}

// 0x13b AvatarRemovalModifier (also 0x104 iwd)
int fx_avatar_removal_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_avatar_removal_modifier(%2d): Mod: %d", fx->Opcode, fx->Parameter2);
	STAT_SET(IE_AVATARREMOVAL, fx->Parameter2);
	return FX_APPLIED;
}

// 0x13c MagicalRest (also 0x124 iwd)
int fx_magical_rest (Scriptable* /*Owner*/, Actor* target, Effect* /*fx*/)
{
	// print("fx_magical_rest(%2d)", fx->Opcode);
	//instant, full rest
	target->Rest(8);
	target->fxqueue.RemoveAllEffects(fx_fatigue_ref);
	target->fxqueue.RemoveAllEffectsWithParam(fx_display_portrait_icon_ref, PI_FATIGUE);
	return FX_NOT_APPLIED;
}

// 0x13d ImprovedHaste (See 0x10 Haste)

// 0x13e ChangeWeather
// sets the weather to param1, set it to:
// 0 normal weather
// 1 rain
// 2 snow
// 3 fog
int fx_change_weather (Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	// print("fx_change_weather(%2d): P1: %d", fx->Opcode, fx->Parameter1);

	core->GetGame()->StartRainOrSnow(false, fx->Parameter1 & WB_TYPEMASK);

	return FX_NOT_APPLIED;
}

// 458 SetConcealment
// adds concealment/etherealness bonus (harder to hit) or malus (harder to hit for you)
// not safe for negative values!
int fx_set_concealment (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_set_concealment(%2d): P1: %d P2: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	int concealment = fx->Parameter1 & 0x64;
	if (fx->Parameter2 == 0) {
		STAT_ADD(IE_ETHEREALNESS, concealment);
	} else {
		STAT_ADD(IE_ETHEREALNESS, concealment<<8);
	}

	return FX_APPLIED;
}

// 459 UncannyDodge
// sets/modifies uncanny dodge type
// 0 - none, reset
// 1..0xff - bonus for saves against traps
// two special values are treated as bits:
// 0x100 - keep dex bonus if attacked by invisibles (like with blind fighting feat)
// 0x200 - can't be flanked (sneak attacked) unless the assailant is 4+ levels higher
int fx_uncanny_dodge (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
//	print("fx_uncanny_dodge(%2d): P1: %d P2: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	ieDword mask = 0xff;
	ieDword stat = target->GetSafeStat(IE_UNCANNY_DODGE);
	ieDword high = stat >> 8; // the "bitsy" part
	ieDword val = fx->Parameter1;

	if ((signed)val < 0) {
		Log(ERROR, "FXOPCodes", "fx_uncanny_dodge does not support negative modifiers!");
	} else if (val == 0) {
		STAT_SET(IE_UNCANNY_DODGE, 0);
	} else if (val <= mask) {
		STAT_SET(IE_UNCANNY_DODGE, val|high);
	} else if (val > mask) {
		STAT_SET(IE_UNCANNY_DODGE, val|stat);
	}
	return FX_APPLIED;
}

/* only in tobex; it adds a bunch of new hard-coded stats, but the opcode can only set these:
387 ACIDDAMAGEBONUS - percentage modifier to acid damage from item/spell ability effects
388 COLDDAMAGEBONUS - percentage modifier to cold damage from item/spell ability effects
389 CRUSHINGDAMAGEBONUS - percentage modifier to crushing damage from normal damage and item/spell ability effects
390 ELECTRICITYDAMAGEBONUS - percentage modifier to electricity damage from item/spell ability effects
391 FIREDAMAGEBONUS - percentage modifier to fire damage from item/spell ability effects
392 PIERCINGDAMAGEBONUS - percentage modifier to piercing damage from normal damage and item/spell ability effects
393 POISONDAMAGEBONUS - percentage modifier to poison damage from item/spell ability effects
394 MAGICDAMAGEBONUS - percentage modifier to magic damage from item/spell ability effects
395 MISSILEDAMAGEBONUS - percentage modifier to missile damage from normal damage and item/spell ability effects
396 SLASHINGDAMAGEBONUS - percentage modifier to slashing damage from normal damage and item/spell ability effects
397 MAGICFIREDAMAGEBONUS - percentage modifier to magic fire damage from item/spell ability effects
398 MAGICCOLDDAMAGEBONUS - percentage modifier to magic colddamage from item/spell ability effects
399 STUNNINGDAMAGEBONUS - percentage modifier to stunning damage from normal damage and item/spell ability effects
400 WEIGHTALLOWANCEMOD - custom stat that modiifies by sum the amount a character is allowed to carry (total weight allowance = StrMod value + StrModEx value + WeightAllowanceMod)

we allow also setting the normal stats, below index 256
*/

int fx_set_stat (Scriptable* Owner, Actor* target, Effect* fx)
{
	static EffectRef fx_damage_bonus_modifier2_ref = { "DamageBonusModifier2", -1 };
	static const int damage_mod_map[] = { 4, 2, 9, 3, 1, 8, 6, 5, 10, 7, 11, 12, 13 };

	ieWord stat = fx->Parameter2 & 0x0000ffff;
	ieWord type = fx->Parameter2 >> 16;

	if ((stat > 255 && stat < 387) || stat > 400) {
		// we support only 256 real stats!
		return FX_NOT_APPLIED;
	}

	// translate the stats to ours
	if (stat == 400) {
		stat = IE_ENCUMBRANCE;
	} else if (stat >= 387) {
		stat = damage_mod_map[stat-387];
		// replace effect with the appropriate one
		fx->Opcode = EffectQueue::ResolveEffect(fx_damage_bonus_modifier2_ref);
		fx->Parameter2 = stat;
		return fx_damage_bonus_modifier2(Owner, target, fx);
	}

	target->NewStat(stat, fx->Parameter1, type);

	return FX_APPLIED;
}

// tobex/ee extension to overcome the limited amount of bits in the ITM usability / kit exclusion field
// the actual string display is done in core when appropriate
int fx_item_usability(Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	ieDword value = fx->Parameter1;
	ieDword type = fx->Parameter2;
	bool match = false;

	// NOTE: move this to match_ids if it turns out others need it as well
	if (type == 10) { // actor name
		if (ieStrRef(value) == ieStrRef::INVALID) {
			// targeting player created characters
			match = target->GetStat(IE_MC_FLAGS) & MC_EXPORTABLE;
		} else {
			match = target->GetLongName() == core->GetString(ieStrRef(value));
		}
	} else if (type == 11) { // actor scripting name
		if (fx->Resource.IsEmpty()) {
			// targeting player created characters
			match = target->GetStat(IE_MC_FLAGS) & MC_EXPORTABLE;
		} else {
			match = fx->Resource == target->GetScriptName();
		}
	} else {
		// regular IDS check
		match = EffectQueue::match_ids(target, type, value);
	}

	// when Power is 0, matching creature(s) cannot use the item.
	// when Power is 1, restrict the item to matching creature(s)
	if (match && fx->Power == 0) {
		fx->Parameter3 = 1; // internal marker
	} else if (!match && fx->Power == 1) {
		fx->Parameter3 = 1;
	}
	return FX_APPLIED;
}

// remove effects matching effects in the passed item or spell
// parameter2 dictates whether to work on all effects, equipping or non-equipping
int fx_remove_effects(Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	target->fxqueue.RemoveAllEffectsWithSource(fx_remove_effects_ref, fx->Resource, fx->Parameter2);
	return FX_APPLIED;
}

// 0x142 (322) unused in ees

// 0x133 TurnLevel (gemrb extension for iwd2)
// 0x143 (323) Stat: Turn Undead Level
int fx_turnlevel_modifier(Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	STAT_MOD(IE_TURNUNDEADLEVEL);
	return FX_APPLIED;
}

// 0x122 (290) Protection:Spell3 with IWD ids targeting
// 0x144 (324) Protection: Immunity to Resource and Message in EEs
// this is a variant of fx_resist_spell that is used in iwd2 (different than bg2, see IWDOpcodes!)
int fx_resist_spell_and_message(Scriptable* Owner, Actor* target, Effect* fx)
{
	//changed this to the opposite (cure light wounds resisted by undead)
	if (!EffectQueue::CheckIWDTargeting(Owner, target, fx->Parameter1, fx->Parameter2, fx)) {
		return FX_NOT_APPLIED;
	}

	//convert effect to the normal resist spell effect (without text)
	//in case it lingers
	fx->Opcode = EffectQueue::ResolveEffect(fx_resist_spell2_ref);

	if (fx->Resource != fx->SourceRef) {
		return FX_APPLIED;
	}
	//display message too
	ieStrRef sourceNameRef = ieStrRef::INVALID;

	if(gamedata->Exists(fx->Resource, IE_ITM_CLASS_ID)) {
		const Item *poi = gamedata->GetItem(fx->Resource);
		sourceNameRef = poi->ItemName;
		gamedata->FreeItem(poi, fx->Resource, false);
	} else if (gamedata->Exists(fx->Resource, IE_SPL_CLASS_ID)) {
		const Spell *poi = gamedata->GetSpell(fx->Resource, true);
		sourceNameRef = poi->SpellName;
		gamedata->FreeSpell(poi, fx->Resource, false);
	} else {
		// ees also try one char shorter resref, so eg. the sunfire child spell finds the main one
		ResRef tmp;
		tmp.Format("{:.7}", fx->Resource);
		if (gamedata->Exists(tmp, IE_SPL_CLASS_ID)) {
			const Spell *poi = gamedata->GetSpell(tmp, true);
			sourceNameRef = poi->SpellName;
			gamedata->FreeSpell(poi, tmp, false);
		}
	}

	if (sourceNameRef != ieStrRef(-1)) {
		core->GetTokenDictionary()["RESOURCE"] = core->GetString(sourceNameRef, STRING_FLAGS::NONE);
		displaymsg->DisplayConstantStringName(HCStrings::ResResisted, GUIColors::WHITE, target);
	}
	//this has effect only on first apply, it will stop applying the spell
	return FX_ABORT;
}

// 0xee  (238) SaveBonus in iwds
// 0x145 (325) SaveBonus in ees
int fx_save_bonus(Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	STAT_MOD(IE_SAVEVSDEATH);
	STAT_MOD(IE_SAVEVSWANDS);
	STAT_MOD(IE_SAVEVSPOLY);
	STAT_MOD(IE_SAVEVSBREATH);
	STAT_MOD(IE_SAVEVSSPELL);
	return FX_APPLIED;
}

// 402 in iwd2, 326 ees
int fx_add_effects_list(Scriptable* Owner, Actor* target, Effect* fx)
{
	// after iwd2 style ids targeting, apply the spell named in the resource field
	if (!EffectQueue::CheckIWDTargeting(Owner, target, fx->Parameter1, fx->Parameter2, fx)) {
		return FX_NOT_APPLIED;
	}
	core->ApplySpell(fx->Resource, target, Owner, fx->Power);
	return FX_NOT_APPLIED;
}

// 0xe9  (233) IWDVisualSpellHit / Graphics: Icewind Visual Spell Hit
// 0x147 (327) Graphics: Icewind Visual Spell Hit (plays sound), ee
int fx_iwd_visual_spell_hit(Scriptable* Owner, Actor* target, Effect* fx)
{
	// print("fx_iwd_visual_spell_hit(%2d): Type: %d", fx->Opcode, fx->Parameter2);
	if (!Owner) {
		return FX_NOT_APPLIED;
	}
	// remove effect if there is no current area
	Map* map = Owner->GetCurrentArea();
	if (!map) {
		return FX_NOT_APPLIED;
	}

	Projectile* pro;
	if (fx->Parameter4 && fx->Parameter2 > 200) {
		// SpellHitEffectPoint is used with sheffect.ids, so the indices are smaller
		// we don't just check for both, since there's some overlap
		// so far tested: acid storm needs this at 210
		//   these don't: 46 66 104
		pro = core->GetProjectileServer()->GetProjectileByIndex(fx->Parameter2);
	} else {
		pro = core->GetProjectileServer()->GetProjectileByIndex(0x1001 + fx->Parameter2);
	}
	pro->SetCaster(fx->CasterID, fx->CasterLevel);

	if (target) {
		// I believe the spell hit projectiles don't follow anyone
		map->AddProjectile(pro, target->Pos, target->GetGlobalID(), true);
	} else {
		map->AddProjectile(pro, fx->Pos, fx->Pos);
	}
	return FX_NOT_APPLIED;
}

// 0x120 (288) State:Set
// 0x148 (328) State: Set State, ee
int fx_set_state(Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// in IWD2 we have 176 states (original had 256)
	// ees can toggle between iwd2 and HoW mode
	if (fx->IsVariable || core->HasFeature(GFFlags::RULES_3ED)) {
		target->SetSpellState(fx->Parameter2);
	} else {
		// in HoW this sets only the 10 last bits of extstate (until it runs out of bits)
		if (fx->Parameter2 < 11 && !fx->IsVariable) {
			EXTSTATE_SET(0x40000 << fx->Parameter2);
		}
	}
	// maximized attacks active
	if (fx->Parameter2 == SS_KAI) {
		target->Modified[IE_DAMAGELUCK] = 255;
	}
	return FX_APPLIED;
}

// 0xef  (289) SlowPoison
// 0x149 (329) SlowPoison, ee
// gemrb extension: can slow bleeding wounds (like bandage)
int fx_slow_poison(Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	ieDword poisonOpcode;
	if (fx->Parameter2) {
		poisonOpcode = EffectQueue::ResolveEffect(fx_wound_ref);
	} else {
		poisonOpcode = EffectQueue::ResolveEffect(fx_poisoned_state_ref);
	}

	auto f = target->fxqueue.GetFirstEffect();
	Effect* poison = target->fxqueue.GetNextEffect(f);
	while (poison) {
		if (poison->Opcode != poisonOpcode) continue;

		switch (poison->Parameter2) {
			case RPD_SECONDS:
				poison->Parameter2 = RPD_ROUNDS;
				break;
			case RPD_POINTS:
				// i'm not sure if this is ok
				// the hardcoded formula is supposed to be this:
				// duration = (duration - gametime)*7+gametime;
				// duration = duration*7 - gametime*6;
				// but in fact it is like this.
				// it is (duration - gametime)*8+gametime;
				// like the damage is spread for a longer time than
				// it should
				poison->Duration = poison->Duration * 8 - core->GetGame()->GameTime * 7;
				poison->Parameter1 *= 7;
				break;
			case RPD_ROUNDS:
				poison->Parameter2 = RPD_TURNS;
				break;
		}
		poison = target->fxqueue.GetNextEffect(f);
	}
	return FX_NOT_APPLIED;
}

// 0x11b (290) FloatText
// 0x14a (330) Text: Float Text, ee
int fx_floattext(Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_floattext(%2d): StrRef:%d Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
	switch (fx->Parameter2) {
	case 1:
		// in the original game this signified that a specific weapon is equipped
		if (EXTSTATE_GET(EXTSTATE_FLOATTEXTS)) {
			return FX_APPLIED;
		}

		EXTSTATE_SET(EXTSTATE_FLOATTEXTS);
		if (fx->Resource.IsEmpty()) {
			fx->Resource = "CYNICISM";
		}
		if (fx->Parameter1) {
			fx->Parameter1--;
			return FX_APPLIED;
		} else {
			fx->Parameter1 = core->Roll(1, 500, 500);
		}
		// fall through
	case 2:
		if (EXTSTATE_GET(EXTSTATE_FLOATTEXTS)) {
			const auto CynicismList = core->GetListFrom2DA(fx->Resource);
			if (!CynicismList->empty()) {
				DisplayStringCore(target, ieStrRef(CynicismList->at(RAND<size_t>(0, CynicismList->size() - 1))), DS_HEAD);
			}
		}
		return FX_APPLIED;
	case 3: // gemrb extension, displays verbalconstant
		DisplayStringCoreVC(target, Verbal(fx->Parameter1), DS_HEAD);
		break;
	default:
		DisplayStringCore(target, ieStrRef(fx->Parameter1), DS_HEAD);
		break;
	}
	return FX_NOT_APPLIED;
}

// iwds had this opcode split into several ones just working on different hardcoded monster lists
// 0x14b (331) Summon: Random Monster Summoning
int fx_iwdee_monster_summoning(Scriptable* Owner, Actor* target, Effect* fx)
{
	static const AutoTable smTables = gamedata->LoadTable("smtables", true);
	ResRef table = fx->Resource;
	if (table.IsEmpty()) {
		if (fx->Parameter2 >= ieDword(smTables->GetRowCount())) return FX_NOT_APPLIED;
		table = smTables->QueryField(fx->Parameter2, 0);
	}

	ResRef monster;
	ResRef hit;
	ResRef areaHit;
	core->GetResRefFrom2DA(table, monster, hit, areaHit);

	// depending on invocation, fx->IsVariable supposedly affected fx->Parameter1
	// see IESDP for some odd notes

	// the monster should appear near the effect position
	Effect* newFx = EffectQueue::CreateUnsummonEffect(fx);
	core->SummonCreature(monster, areaHit, Owner, target, fx->Pos, EAM_SOURCEALLY, fx->Parameter1, newFx);
	return FX_NOT_APPLIED;
}

// 0x14c (332) is just a remapped DamageBonusModifier(2) - see fx_damage_bonus_modifier2

// 0x108 (264) StaticCharge
// 0x14d (333) Spell Effect: Static Charge, ee
int fx_static_charge(Scriptable* Owner, Actor* target, Effect* fx)
{
	// if the owner (target) is dead, this effect ceases to exist
	if (STATE_GET(STATE_DEAD | STATE_PETRIFIED | STATE_FROZEN)) {
		displaymsg->DisplayConstantStringName(HCStrings::StaticDissipate, GUIColors::WHITE, target);
		return FX_NOT_APPLIED;
	}

	int ret = FX_APPLIED;
	if (fx->Parameter1 <= 1) {
		ret = FX_NOT_APPLIED;
		// prevent an underflow
		if (fx->Parameter1 == 0) {
			return ret;
		}
	}

	// ee level param and unhardcoded delay
	ieDword level = std::max(1U, fx->Parameter2);
	ieWord delay = fx->IsVariable * 10;
	if (!delay) delay = static_cast<ieWord>(core->Time.rounds_per_turn * core->Time.round_size);
	ResRef spell = fx->Resource;

	// timing
	fx->TimingMode = FX_DURATION_DELAY_PERMANENT;
	fx->Duration = core->GetGame()->GameTime + delay;
	fx->Parameter1--;

	// this effect is targeted on the caster, so we need to manually find a damage target
	const Map* map = target->GetCurrentArea();
	if (!map) return FX_APPLIED;
	Actor* victim = map->GetRandomEnemySeen(target);
	if (!victim) {
		displaymsg->DisplayConstantStringName(HCStrings::StaticDissipate, GUIColors::WHITE, target);
		return FX_APPLIED;
	}

	// ee style
	if (fx->Opcode == 0x14d) {
		if (spell.IsEmpty()) {
			spell.Format("{:.7}B", fx->SourceRef);
		}
		core->ApplySpell(spell, victim, Owner, level);
		return ret;
	}

	// iwd2 style
	if (!spell.IsEmpty()) {
		core->ApplySpell(spell, victim, Owner, fx->Power);
		return ret;
	}

	// how style
	victim->Damage(DICE_ROLL(0), DAMAGE_ELECTRICITY, Owner, MOD_ADDITIVE, fx->SavingThrowType);
	return ret;
}

// 0x14e (334) is just a remapped TurnUndead - see fx_turn_undead

// 0x14f (335) Spell Effect: Seven Eyes - a merged opcode for the split ones present in iwds like fx_eye_of_the_mind
int fx_seven_eyes(Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (target->SetSpellState(fx->Parameter1)) return FX_APPLIED;
	// it's unclear if ees also set the extstate bits, but we do it anyway
	// both since it's likely and because our drawing is tied to it, not the spell states
	target->SetBaseBit(IE_EXTSTATE_ID, EXTSTATE_EYE_MIND << fx->IsVariable, true);

	if (fx->FirstApply) {
		target->LearnSpell(fx->Resource, LS_MEMO);
	}
	return FX_APPLIED;
}

// 0x150 (336) Graphics: Display Eyes Overlay
// TODO: ee, check if we even really need it, since seven eyes are implemented in Actor
// https://gibberlings3.github.io/iesdp/opcodes/bgee.htm#op66

// 0x114 (276) RemoveEffect
// 0x151 (337) Remove: Opcode
int fx_remove_effect(Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (!fx->Resource.IsEmpty()) {
		target->fxqueue.RemoveAllEffectsWithResource(fx->Parameter2, fx->Resource);
	} else if (fx->Opcode == 0x151 && fx->Parameter1 != ieDword(-1)) {
		target->fxqueue.RemoveAllEffectsWithParam(fx->Parameter2, fx->Parameter1);
	} else {
		target->fxqueue.RemoveAllEffects(fx->Parameter2);
	}
	return FX_NOT_APPLIED;
}

// 0x152 DisableRest implemented as a generic effect

// 0x18f AlterAnimation unhardcoded in gemrb
// 0x153 (339) Alter Animation in ees, adds range limit
//
// This code is needed for the ending cutscene in IWD (found in a projectile)
// The effect will alter the target animation's cycle by a xor value
// Parameter1: the value to binary xor on the initial cycle numbers of the animation(s)
// Parameter2: an optional projectile (IWD spell hit projectiles start at 0x1001)
// Resource: the animation's name
//
// Useful applications other than the HoW cutscene:
// A fireball could affect environment by applying the effect on certain animations.
// All you need to do:
//  - name the area animation as 'burnable'.
//  - create alternate cycles for the altered area object
//  - create a spell hit animation (optionally)
//  - create the effect which will contain the spell hit projectile and the cycle change command
int fx_alter_animation(Scriptable* Owner, Actor* /*target*/, Effect* fx)
{
	Map* map = Owner->GetCurrentArea();
	if (!map) {
		return FX_NOT_APPLIED;
	}

	aniIterator iter = map->GetFirstAnimation();
	AreaAnimation* an = map->GetNextAnimation(iter);
	while (an) {
		// Only animations with 8 letters could be used, no problem, iwd uses 8 letters
		if (an->Name.BeginsWith(fx->Resource)) {
			if (fx->Opcode == 0x153 && (!fx->IsVariable || Distance(fx->Pos, an->Pos) > (unsigned int) fx->IsVariable)) {
				an = map->GetNextAnimation(iter);
				continue;
			}
			// play spell hit animation
			Projectile* pro = core->GetProjectileServer()->GetProjectileByIndex(fx->Parameter2);
			pro->SetCaster(fx->CasterID, fx->CasterLevel);
			map->AddProjectile(pro, an->Pos, an->Pos);
			// alter animation, we need only this for the original, but in the
			// spirit of unhardcoding, i provided the standard modifier codeset
			// 0->4, 1->5, 2->6, 3->7
			// 4->0, 5->1, 6->2, 7->3
			ieWord value = fx->Parameter1 >> 16;
			SetBits(an->sequence, value, BitOp(fx->Parameter1 & 0xffff));
			an->frame = 0;
			an->animation.clear();
			an->InitAnimation();
		}
		an = map->GetNextAnimation(iter);
	}
	return FX_NOT_APPLIED;
}

// 0x154 (340) Effect: Change Backstab Effect
// similar to the hamstring / arterial strike in iwd2, used by the HoW 3ed sneak attack mode
int fx_change_backstab(Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	target->BackstabResRef = fx->Resource;
	return FX_APPLIED;
}

// 0x155 (341) Spell Effect: Change Critical Hit Effect, implemented as a generic effect CastSpellOnCriticalHit
// 0x156 (342) Animation: Override Data, implemented as a generic effect
// NOTE: not implemented Parameter2 == 4 ⟶ Override personal space, since it supposedly doesn't affect circle size
// if it turns out this is needed, consider making it a stat, so also the pathfinder has quick access

// 0x157 (343) HP Swap
int fx_swap_hp(Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	Scriptable* casterObject = GetCasterObject();
	Actor* caster = Scriptable::As<Actor>(casterObject);
	if (!caster) return FX_NOT_APPLIED;

	ieDword hpA = caster->GetStat(IE_HITPOINTS);
	ieDword hpB = target->GetStat(IE_HITPOINTS);
	if (fx->Parameter2 == 0 && hpA <= hpB) return FX_NOT_APPLIED;

	caster->SetBase(IE_HITPOINTS, hpB);
	target->SetBase(IE_HITPOINTS, hpA);
	return FX_NOT_APPLIED;
}

// 0x158 (344) Enchantment vs. creature type, implemented as a generic effect
// 0x159 (345) Enchantment bonus, implemented as a generic effect
// 0x15a (346) Save vs. school bonus, implemented as a generic effect

// 347-359 are unused in the ees

// 0x168 (360) Stat: Ignore Reputation Breaking Point, implemented as a generic effect
// 0x169 (361) Cast spell on critical miss (identical to 0x155), implemented as a generic effect
// 0x16a (362) Critical miss bonus, implemented as a generic effect

// 0x16b (363) TODO: ee, Modal state check — for shaman dance (granted by clabsh01, set up like their bard song - uses spsh004)
// 0x16c (364) unused
// 0x16d (365) TODO: ee, Make unselectable; reuse the action?
// 0x16e (366) TODO: ee, Spell: Apply Spell On Move

// 0x16f (367) MinimumBaseStats (EE-only) is implemented as a generic effect

// unknown
int fx_unknown (Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	Log(ERROR, "FXOpcodes", "fx_unknown({}): P1: {} P2: {} ResRef: {}", fx->Opcode, fx->Parameter1, fx->Parameter2, fx->Resource);
	return FX_NOT_APPLIED;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x1AAA040A, "Effect opcodes for core games")
PLUGIN_INITIALIZER(RegisterCoreOpcodes)
END_PLUGIN()
