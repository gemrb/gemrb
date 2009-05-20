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
 * $Id$
 *
 */

#include "../../includes/win32def.h"
#include "../../includes/strrefs.h"
#include "../../includes/opcode_params.h"
#include "../../includes/overlays.h"
#include "../Core/Actor.h"
#include "../Core/EffectQueue.h"
#include "../Core/Interface.h"
#include "../Core/ResourceMgr.h"
#include "../Core/Audio.h"
#include "../Core/Game.h"
#include "../Core/GameControl.h"
#include "../Core/damages.h"
#include "../Core/TileMap.h" //needs for knock!
#include "../Core/GSUtils.h" //needs for MoveBetweenAreasCore
#include "../Core/Projectile.h" //needs for clearair
#include "FXOpc.h"

//FIXME: find a way to handle portrait icons better
#define PI_CONFUSED 3
#define PI_BERSERK  4
#define PI_POISONED 6
#define PI_HELD    13
#define PI_SLEEP   14
#define PI_BLESS   17
#define PI_PANIC        36
#define PI_HASTED  38
#define PI_FATIGUE 39
#define PI_SLOWED  41
#define PI_HOPELESS 44
#define PI_FEEBLEMIND 54
#define PI_STUN    55
#define PI_AID     57
#define PI_HOLY    59
#define PI_BOUNCE  65
#define PI_BOUNCE2 67

#define PI_BLOODRAGE 76 //iwd2
#define PI_MAZE    78
#define PI_PRISON  79
#define PI_DEAFNESS 83 //iwd2
#define PI_SEQUENCER 92
#define PI_BLUR 109
#define PI_IMPROVEDHASTE 110
#define PI_SPELLTRAP 117
#define PI_CSHIELD  162
#define PI_CSHIELD2 163

static ieResRef *casting_glows = NULL;
static int cgcount = -1;
static ieResRef *spell_hits = NULL;
static bool enhanced_effects = false;
static int shcount = -1;
static int *spell_abilities = NULL;
static ieDword splabcount = 0;
static int *polymorph_stats = NULL;
static int polystatcount = 0;

//the original engine stores the colors in sprklclr.2da in a different order

#define SPARK_COUNT 13

static int spark_color_indices[SPARK_COUNT]={12,5,0,6,1,8,2,7,9,3,4,10,11};

static ScriptedAnimation default_spell_hit;

int fx_ac_vs_damage_type_modifier (Actor* Owner, Actor* target, Effect* fx);//00
int fx_attacks_per_round_modifier (Actor* Owner, Actor* target, Effect* fx);//01
int fx_cure_sleep_state (Actor* Owner, Actor* target, Effect* fx);//02
int fx_set_berserk_state (Actor* Owner, Actor* target, Effect* fx);//03
int fx_cure_berserk_state (Actor* Owner, Actor* target, Effect* fx);//04
int fx_set_charmed_state (Actor* Owner, Actor* target, Effect* fx);//05
int fx_charisma_modifier (Actor* Owner, Actor* target, Effect* fx);//06
int fx_set_color_gradient (Actor* Owner, Actor* target, Effect* fx);//07
int fx_set_color_rgb (Actor* Owner, Actor* target, Effect* fx);//08
int fx_set_color_rgb_global (Actor* Owner, Actor* target, Effect* fx);//08
int fx_set_color_pulse_rgb (Actor* Owner, Actor* target, Effect* fx);//09
int fx_set_color_pulse_rgb_global (Actor* Owner, Actor* target, Effect* fx);//09
int fx_constitution_modifier (Actor* Owner, Actor* target, Effect* fx);//0a
int fx_cure_poisoned_state (Actor* Owner, Actor* target, Effect* fx);//0b
int fx_damage (Actor* Owner, Actor* target, Effect* fx);//0c
int fx_death (Actor* Owner, Actor* target, Effect* fx);//0d
int fx_cure_frozen_state (Actor* Owner, Actor* target, Effect* fx);//0e
int fx_dexterity_modifier (Actor* Owner, Actor* target, Effect* fx);//0f
int fx_set_hasted_state (Actor* Owner, Actor* target, Effect* fx);//10
int fx_current_hp_modifier (Actor* Owner, Actor* target, Effect* fx);//11
int fx_maximum_hp_modifier (Actor* Owner, Actor* target, Effect* fx);//12
int fx_intelligence_modifier (Actor* Owner, Actor* target, Effect* fx);//13
int fx_set_invisible_state (Actor* Owner, Actor* target, Effect* fx);//14
int fx_lore_modifier (Actor* Owner, Actor* target, Effect* fx);//15
int fx_luck_modifier (Actor* Owner, Actor* target, Effect* fx);//16
int fx_morale_modifier (Actor* Owner, Actor* target, Effect* fx);//17
int fx_set_panic_state (Actor* Owner, Actor* target, Effect* fx);//18
int fx_set_poisoned_state (Actor* Owner, Actor* target, Effect* fx);//19
int fx_remove_curse (Actor* Owner, Actor* target, Effect* fx);//1a
int fx_acid_resistance_modifier (Actor* Owner, Actor* target, Effect* fx);//1b
int fx_cold_resistance_modifier (Actor* Owner, Actor* target, Effect* fx);//1c
int fx_electricity_resistance_modifier (Actor* Owner, Actor* target, Effect* fx);//1d
int fx_fire_resistance_modifier (Actor* Owner, Actor* target, Effect* fx);//1e
int fx_magic_damage_resistance_modifier (Actor* Owner, Actor* target, Effect* fx);//1f
int fx_cure_dead_state (Actor* Owner, Actor* target, Effect* fx);//20
int fx_save_vs_death_modifier (Actor* Owner, Actor* target, Effect* fx);//21
int fx_save_vs_wands_modifier (Actor* Owner, Actor* target, Effect* fx);//22
int fx_save_vs_poly_modifier (Actor* Owner, Actor* target, Effect* fx);//23
int fx_save_vs_breath_modifier (Actor* Owner, Actor* target, Effect* fx);//24
int fx_save_vs_spell_modifier (Actor* Owner, Actor* target, Effect* fx);//25
int fx_set_silenced_state (Actor* Owner, Actor* target, Effect* fx);//26
int fx_set_unconscious_state (Actor* Owner, Actor* target, Effect* fx);//27
int fx_set_slowed_state (Actor* Owner, Actor* target, Effect* fx);//28
int fx_sparkle(Actor* Owner, Actor* target, Effect* fx);//29
int fx_bonus_wizard_spells (Actor* Owner, Actor* target, Effect* fx);//2a
int fx_cure_petrified_state (Actor* Owner, Actor* target, Effect* fx);//2b
int fx_strength_modifier (Actor* Owner, Actor* target, Effect* fx);//2c
int fx_set_stun_state (Actor* Owner, Actor* target, Effect* fx);//2d
int fx_cure_stun_state (Actor* Owner, Actor* target, Effect* fx);//2e
int fx_cure_invisible_state (Actor* Owner, Actor* target, Effect* fx);//2f
int fx_cure_silenced_state (Actor* Owner, Actor* target, Effect* fx);//30
int fx_wisdom_modifier (Actor* Owner, Actor* target, Effect* fx);//31
int fx_brief_rgb (Actor* Owner, Actor* target, Effect* fx);//32
int fx_darken_rgb (Actor* Owner, Actor* target, Effect* fx);//33
int fx_glow_rgb (Actor* Owner, Actor* target, Effect* fx);//34
int fx_animation_id_modifier (Actor* Owner, Actor* target, Effect* fx);//35
int fx_to_hit_modifier (Actor* Owner, Actor* target, Effect* fx);//36
int fx_kill_creature_type (Actor* Owner, Actor* target, Effect* fx);//37
int fx_alignment_invert (Actor* Owner, Actor* target, Effect* fx);//38
int fx_alignment_change (Actor* Owner, Actor* target, Effect* fx);//39
int fx_dispel_effects (Actor* Owner, Actor* target, Effect* fx);//3a
int fx_stealth_modifier (Actor* Owner, Actor* target, Effect* fx);//3b
int fx_miscast_magic_modifier (Actor* Owner, Actor* target, Effect* fx);//3c
int fx_alchemy_modifier (Actor* Owner, Actor* target, Effect* fx);//3d
int fx_bonus_priest_spells (Actor* Owner, Actor* target, Effect* fx);//3e
int fx_set_infravision_state (Actor* Owner, Actor* target, Effect* fx);//3f
int fx_cure_infravision_state (Actor* Owner, Actor* target, Effect* fx);//40
int fx_set_blur_state (Actor* Owner, Actor* target, Effect* fx);//41
int fx_transparency_modifier (Actor* Owner, Actor* target, Effect* fx);//42
int fx_summon_creature (Actor* Owner, Actor* target, Effect* fx);//43
int fx_unsummon_creature (Actor* Owner, Actor* target, Effect* fx);//44
int fx_set_nondetection_state (Actor* Owner, Actor* target, Effect* fx);//45
int fx_cure_nondetection_state (Actor* Owner, Actor* target, Effect* fx);//46
int fx_sex_modifier (Actor* Owner, Actor* target, Effect* fx);//47
int fx_ids_modifier (Actor* Owner, Actor* target, Effect* fx);//48
int fx_damage_bonus_modifier (Actor* Owner, Actor* target, Effect* fx);//49
int fx_set_blind_state (Actor* Owner, Actor* target, Effect* fx);//4a
int fx_cure_blind_state (Actor* Owner, Actor* target, Effect* fx);//4b
int fx_set_feebleminded_state (Actor* Owner, Actor* target, Effect* fx);//4c
int fx_cure_feebleminded_state (Actor* Owner, Actor* target, Effect* fx);//4d
int fx_set_diseased_state (Actor* Owner, Actor* target, Effect*fx);//4e
int fx_cure_diseased_state (Actor* Owner, Actor* target, Effect* fx);//4f
int fx_set_deaf_state (Actor* Owner, Actor* target, Effect* fx); //50
int fx_set_deaf_state_iwd2 (Actor* Owner, Actor* target, Effect* fx); //50
int fx_cure_deaf_state (Actor* Owner, Actor* target, Effect* fx);//51
int fx_set_ai_script (Actor* Owner, Actor* target, Effect*fx);//52
int fx_protection_from_projectile (Actor* Owner, Actor* target, Effect*fx);//53
int fx_magical_fire_resistance_modifier (Actor* Owner, Actor* target, Effect* fx);//54
int fx_magical_cold_resistance_modifier (Actor* Owner, Actor* target, Effect* fx);//55
int fx_slashing_resistance_modifier (Actor* Owner, Actor* target, Effect* fx);//56
int fx_crushing_resistance_modifier (Actor* Owner, Actor* target, Effect* fx);//57
int fx_piercing_resistance_modifier (Actor* Owner, Actor* target, Effect* fx);//58
int fx_missiles_resistance_modifier (Actor* Owner, Actor* target, Effect* fx);//59
int fx_open_locks_modifier (Actor* Owner, Actor* target, Effect* fx);//5a
int fx_find_traps_modifier (Actor* Owner, Actor* target, Effect* fx);//5b
int fx_pick_pockets_modifier (Actor* Owner, Actor* target, Effect* fx);//5c
int fx_fatigue_modifier (Actor* Owner, Actor* target, Effect* fx);//5d
int fx_intoxication_modifier (Actor* Owner, Actor* target, Effect* fx);//5e
int fx_tracking_modifier (Actor* Owner, Actor* target, Effect* fx);//5f
int fx_level_modifier (Actor* Owner, Actor* target, Effect* fx);//60
int fx_strength_bonus_modifier (Actor* Owner, Actor* target, Effect* fx);//61
int fx_set_regenerating_state (Actor* Owner, Actor* target, Effect* fx);//62
int fx_spell_duration_modifier (Actor* Owner, Actor* target, Effect* fx);///63
int fx_generic_effect (Actor* Owner, Actor* target, Effect* fx);//64 protection from creature is a generic effect
int fx_protection_opcode(Actor* Owner, Actor* target, Effect* fx); //65
int fx_protection_spelllevel (Actor* Owner, Actor* target, Effect* fx); //66
int fx_change_name (Actor* Owner, Actor* target, Effect* fx);//67
int fx_experience_modifier (Actor* Owner, Actor* target, Effect* fx);//68
int fx_gold_modifier (Actor* Owner, Actor* target, Effect* fx);//69
int fx_morale_break_modifier (Actor* Owner, Actor* target, Effect* fx);//6a
int fx_portrait_change (Actor* Owner, Actor* target, Effect* fx);//6b
int fx_reputation_modifier (Actor* Owner, Actor* target, Effect* fx);//6c
int fx_hold_creature_no_icon (Actor* Owner, Actor* target, Effect* fx);//6d
int fx_retreat_from (Actor* Owner, Actor* target, Effect* fx);//6e (unknown)
int fx_create_magic_item (Actor* Owner, Actor* target, Effect* fx);//6f
int fx_remove_item (Actor* Owner, Actor* target, Effect* fx);//70
int fx_equip_item (Actor* Owner, Actor* target, Effect* fx);//71
int fx_dither (Actor* Owner, Actor* target, Effect* fx);//72
int fx_detect_alignment (Actor* Owner, Actor* target, Effect* fx);//73
//int fx_cure_improved_invisible_state (Actor* Owner, Actor* target, Effect* fx);//74 (2f)
int fx_reveal_area (Actor* Owner, Actor* target, Effect* fx);//75
int fx_reveal_creatures (Actor* Owner, Actor* target, Effect* fx);//76
int fx_mirror_image (Actor* Owner, Actor* target, Effect* fx);//77
int fx_immune_to_weapon (Actor* Owner, Actor* target, Effect* fx);//78
int fx_visual_animation_effect (Actor* Owner, Actor* target, Effect* fx);//79 unknown
int fx_create_inventory_item (Actor* Owner, Actor* target, Effect* fx);//7a
int fx_remove_inventory_item (Actor* Owner, Actor* target, Effect* fx);//7b
int fx_dimension_door (Actor* Owner, Actor* target, Effect* fx);//7c
int fx_knock (Actor* Owner, Actor* target, Effect* fx);//7d
int fx_movement_modifier (Actor* Owner, Actor* target, Effect* fx);//7e
int fx_monster_summoning (Actor* Owner, Actor* target, Effect* fx);//7f
int fx_set_confused_state (Actor* Owner, Actor* target, Effect* fx);//80
int fx_set_aid_state (Actor* Owner, Actor* target, Effect* fx);//81
int fx_set_bless_state (Actor* Owner, Actor* target, Effect* fx);//82
int fx_set_chant_state (Actor* Owner, Actor* target, Effect* fx);//83
int fx_set_holy_state (Actor* Owner, Actor* target, Effect* fx);//84
int fx_luck_non_cumulative (Actor* Owner, Actor* target, Effect* fx);//85
int fx_luck_cumulative (Actor* Owner, Actor* target, Effect* fx);//85
int fx_set_petrified_state (Actor* Owner, Actor* target, Effect* fx);//86
int fx_polymorph (Actor* Owner, Actor* target, Effect* fx);//87
int fx_force_visible (Actor* Owner, Actor* target, Effect* fx);//88
int fx_set_chantbad_state (Actor* Owner, Actor* target, Effect* fx);//89
int fx_animation_stance (Actor* Owner, Actor* target, Effect* fx);//8a
int fx_display_string (Actor* Owner, Actor* target, Effect* fx);//8b
int fx_casting_glow (Actor* Owner, Actor* target, Effect* fx);//8c
int fx_visual_spell_hit (Actor* Owner, Actor* target, Effect* fx);//8d
int fx_display_portrait_icon (Actor* Owner, Actor* target, Effect* fx);//8e
int fx_create_item_in_slot (Actor* Owner, Actor* target, Effect* fx);//8f
int fx_disable_button (Actor* Owner, Actor* target, Effect* fx);//90
int fx_disable_spellcasting (Actor* Owner, Actor* target, Effect* fx);//91
int fx_cast_spell (Actor* Owner, Actor* target, Effect *fx);//92
int fx_learn_spell (Actor* Owner, Actor* target, Effect *fx);//93
int fx_cast_spell_point (Actor* Owner, Actor* target, Effect *fx);//94
int fx_identify (Actor* Owner, Actor* target, Effect *fx);//95
int fx_find_traps (Actor* Owner, Actor* target, Effect *fx);//96
int fx_replace_creature (Actor* Owner, Actor* target, Effect *fx);//97
int fx_play_movie (Actor* Owner, Actor* target, Effect* fx);//98
int fx_set_sanctuary_state (Actor* Owner, Actor* target, Effect* fx);//99
int fx_set_entangle_state (Actor* Owner, Actor* target, Effect* fx);//9a
int fx_set_minorglobe_state (Actor* Owner, Actor* target, Effect* fx);//9b
int fx_set_shieldglobe_state (Actor* Owner, Actor* target, Effect* fx);//9c
int fx_set_web_state (Actor* Owner, Actor* target, Effect* fx);//9d
int fx_set_grease_state (Actor* Owner, Actor* target, Effect* fx);//9e
int fx_mirror_image_modifier (Actor* Owner, Actor* target, Effect* fx);//9f
int fx_cure_sanctuary_state (Actor* Owner, Actor* target, Effect* fx);//a0
int fx_cure_panic_state (Actor* Owner, Actor* target, Effect* fx);//a1
int fx_cure_hold_state (Actor* Owner, Actor* target, Effect* fx);//a2 //cures 175
int fx_cure_slow_state (Actor* Owner, Actor* target, Effect* fx);//a3
int fx_cure_intoxication (Actor* Owner, Actor* target, Effect* fx);//a4
int fx_pause_target (Actor* Owner, Actor* target, Effect* fx);//a5
int fx_magic_resistance_modifier (Actor* Owner, Actor* target, Effect* fx);//a6
int fx_missile_to_hit_modifier (Actor* Owner, Actor* target, Effect* fx);//a7
int fx_remove_creature (Actor* Owner, Actor* target, Effect* fx);//a8
int fx_disable_portrait_icon (Actor* Owner, Actor* target, Effect* fx);//a9
int fx_damage_animation (Actor* Owner, Actor* target, Effect* fx);//aa
int fx_add_innate (Actor* Owner, Actor* target, Effect* fx);//ab
int fx_remove_spell (Actor* Owner, Actor* target, Effect* fx);//ac
int fx_poison_resistance_modifier (Actor* Owner, Actor* target, Effect* fx);//ad
int fx_playsound (Actor* Owner, Actor* target, Effect* fx);//ae
int fx_hold_creature (Actor* Owner, Actor* target, Effect* fx);//af
// this function is exactly the same as 0x7e fx_movement_modifier (in bg2 at least)//b0
int fx_apply_effect (Actor* Owner, Actor* target, Effect* fx);//b1
//b2 //hitbonus against creature (generic_effect)
//b3 //damagebonus against creature (generic effect)
//b4 //unknown
//b5 //unknown
//b6 //unknown
//b7 //unknown
int fx_dontjump_modifier (Actor* Owner, Actor* target, Effect* fx);//b8
// this function is exactly the same as 0xaf hold_creature (in bg2 at least) //b9
int fx_move_to_area (Actor* Owner, Actor* target, Effect* fx);//ba
int fx_local_variable (Actor* Owner, Actor* target, Effect* fx);//bb
int fx_auracleansing_modifier (Actor* Owner, Actor* target, Effect* fx);//bc
int fx_castingspeed_modifier (Actor* Owner, Actor* target, Effect* fx);//bd
int fx_attackspeed_modifier (Actor* Owner, Actor* target, Effect* fx);//be
int fx_castinglevel_modifier (Actor* Owner, Actor* target, Effect* fx);//bf
int fx_find_familiar (Actor* Owner, Actor* target, Effect* fx);//c0
int fx_see_invisible_modifier (Actor* Owner, Actor* target, Effect* fx);//c1
int fx_ignore_dialogpause_modifier (Actor* Owner, Actor* target, Effect* fx);//c2
int fx_familiar_constitution_loss (Actor* Owner, Actor* target, Effect* fx);//c3
int fx_familiar_marker (Actor* Owner, Actor* target, Effect* fx);//c4
int fx_bounce_projectile (Actor* Owner, Actor* target, Effect* fx);//c5
int fx_bounce_opcode (Actor* Owner, Actor* target, Effect* fx);//c6
int fx_bounce_spelllevel (Actor* Owner, Actor* target, Effect* fx);//c7
int fx_bounce_spelllevel_dec (Actor* Owner, Actor* target, Effect* fx);//c8
int fx_protection_spelllevel_dec (Actor* Owner, Actor* target, Effect* fx);//c9
int fx_bounce_school (Actor* Owner, Actor* target, Effect* fx);//ca
int fx_bounce_secondary_type (Actor* Owner, Actor* target, Effect* fx);//cb
int fx_protection_school (Actor* Owner, Actor* target, Effect* fx); //cc
int fx_protection_secondary_type (Actor* Owner, Actor* target, Effect* fx); //cd
int fx_resist_spell (Actor* Owner, Actor* target, Effect* fx);//ce
int fx_resist_spell_dec (Actor* Owner, Actor* target, Effect* fx);//??
int fx_bounce_spell (Actor* Owner, Actor* target, Effect* fx);//cf
int fx_bounce_spell_dec (Actor* Owner, Actor* target, Effect* fx);//??
int fx_minimum_hp_modifier (Actor* Owner, Actor* target, Effect* fx);//d0
int fx_power_word_kill (Actor* Owner, Actor* target, Effect* fx);//d1
int fx_power_word_stun (Actor* Owner, Actor* target, Effect* fx);//d2
int fx_imprisonment (Actor* Owner, Actor* target, Effect* fx);//d3
int fx_freedom (Actor* Owner, Actor* target, Effect* fx);//d4
int fx_maze (Actor* Owner, Actor* target, Effect* fx);//d5
int fx_select_spell (Actor* Owner, Actor* target, Effect* fx);//d6
int fx_play_visual_effect (Actor* Owner, Actor* target, Effect* fx); //d7
int fx_leveldrain_modifier (Actor* Owner, Actor* target, Effect* fx);//d8
int fx_power_word_sleep (Actor* Owner, Actor* target, Effect* fx);//d9
int fx_stoneskin_modifier (Actor* Owner, Actor* target, Effect* fx);//da
//db ac vs creature type (general effect)
int fx_dispel_school (Actor* Owner, Actor* target, Effect* fx);//dc
int fx_dispel_secondary_type (Actor* Owner, Actor* target, Effect* fx);//dd
int fx_teleport_field (Actor* Owner, Actor* target, Effect* fx);//de
int fx_protection_school_dec (Actor* Owner, Actor* target, Effect* fx);//df
int fx_cure_leveldrain (Actor* Owner, Actor* target, Effect* fx);//e0
int fx_reveal_magic (Actor* Owner, Actor* target, Effect* fx);//e1
int fx_protection_secondary_type_dec (Actor* Owner, Actor* target, Effect* fx);//e2
int fx_bounce_school_dec (Actor* Owner, Actor* target, Effect* fx);//e3
int fx_bounce_secondary_type_dec (Actor* Owner, Actor* target, Effect* fx);//e4
int fx_dispel_school_one (Actor* Owner, Actor* target, Effect* fx);//e5
int fx_dispel_secondary_type_one (Actor* Owner, Actor* target, Effect* fx);//e6
int fx_timestop (Actor* Owner, Actor* target, Effect* fx);//e7
int fx_cast_spell_on_condition (Actor* Owner, Actor* target, Effect* fx);//e8
int fx_proficiency (Actor* Owner, Actor* target, Effect* fx);//e9
int fx_create_contingency (Actor* Owner, Actor* target, Effect* fx);//ea
int fx_wing_buffet (Actor* Owner, Actor* target, Effect* fx);//eb
int fx_puppet_master (Actor* Owner, Actor* target, Effect* fx);//ec
int fx_puppet_marker (Actor* Owner, Actor* target, Effect* fx);//ed
int fx_disintegrate (Actor* Owner, Actor* target, Effect* fx);//ee
int fx_farsee (Actor* Owner, Actor* target, Effect* fx);//ef
int fx_remove_portrait_icon (Actor* Owner, Actor* target, Effect* fx);//f0
//f1 control creature (see charm)
int fx_cure_confused_state (Actor* Owner, Actor* target, Effect* fx);//f2
int fx_drain_items (Actor* Owner, Actor* target, Effect* fx);//f3
int fx_drain_spells (Actor* Owner, Actor* target, Effect* fx);//f4
int fx_checkforberserk_modifier (Actor* Owner, Actor* target, Effect* fx);//f5
int fx_berserkstage1_modifier (Actor* Owner, Actor* target, Effect* fx);//f6
int fx_berserkstage2_modifier (Actor* Owner, Actor* target, Effect* fx);//f7
//f8 melee effect (general effect?)
//f9 ranged effect (general effect?)
int fx_damageluck_modifier (Actor* Owner, Actor* target, Effect* fx);//fa
int fx_change_bardsong (Actor* Owner, Actor* target, Effect* fx);//fb
int fx_set_area_effect (Actor* Owner, Actor* target, Effect* fx);//fc (set trap)
int fx_set_map_note (Actor* Owner, Actor* target, Effect* fx);//fd
int fx_remove_map_note (Actor* Owner, Actor* target, Effect* fx);//fe
int fx_create_item_days (Actor* Owner, Actor* target, Effect* fx);//ff
int fx_store_spell_sequencer (Actor* Owner, Actor* target, Effect* fx);//0x100
int fx_create_spell_sequencer (Actor* Owner, Actor* target, Effect* fx);//101
int fx_activate_spell_sequencer (Actor* Owner, Actor* target, Effect* fx);//102
int fx_spelltrap (Actor* Owner, Actor* target, Effect* fx);//103
int fx_crash (Actor* Owner, Actor* target, Effect* fx);//104, disabled
int fx_restore_spell_level (Actor* Owner, Actor* target, Effect* fx);//105
int fx_visual_range_modifier (Actor* Owner, Actor* target, Effect* fx);//106
int fx_backstab_modifier (Actor* Owner, Actor* target, Effect* fx);//107
int fx_drop_weapon (Actor* Owner, Actor* target, Effect* fx);//108
int fx_modify_global_variable (Actor* Owner, Actor* target, Effect* fx);//109
int fx_remove_immunity (Actor* Owner, Actor* target, Effect* fx);//10a
int fx_protection_from_string (Actor* Owner, Actor* target, Effect* fx);//10b
int fx_explore_modifier (Actor* Owner, Actor* target, Effect* fx);//10c
int fx_screenshake (Actor* Owner, Actor* target, Effect* fx);//10d
int fx_unpause_caster (Actor* Owner, Actor* target, Effect* fx);//10e
int fx_avatar_removal (Actor* Owner, Actor* target, Effect* fx);//10f
int fx_apply_effect_repeat (Actor* Owner, Actor* target, Effect* fx);//110
int fx_remove_projectile (Actor* Owner, Actor* target, Effect* fx);//111
int fx_teleport_to_target (Actor* Owner, Actor* target, Effect* fx);//112
int fx_hide_in_shadows_modifier (Actor* Owner, Actor* target, Effect* fx);//113
int fx_detect_illusion_modifier (Actor* Owner, Actor* target, Effect* fx);//114
int fx_set_traps_modifier (Actor* Owner, Actor* target, Effect* fx);//115
int fx_to_hit_bonus_modifier (Actor* Owner, Actor* target, Effect* fx);//116
int fx_renable_button (Actor* Owner, Actor* target, Effect* fx);//117
int fx_force_surge_modifier (Actor* Owner, Actor* target, Effect* fx);//118
int fx_wild_surge_modifier (Actor* Owner, Actor* target, Effect* fx);//119
int fx_scripting_state (Actor* Owner, Actor* target, Effect* fx);//11a
int fx_apply_effect_curse (Actor* Owner, Actor* target, Effect* fx);//11b
int fx_melee_to_hit_modifier (Actor* Owner, Actor* target, Effect* fx);//11c
int fx_melee_damage_modifier (Actor* Owner, Actor* target, Effect* fx);//11d
int fx_missile_damage_modifier (Actor* Owner, Actor* target, Effect* fx);//11e
int fx_no_circle_state (Actor* Owner, Actor* target, Effect* fx);//11f
int fx_fist_to_hit_modifier (Actor* Owner, Actor* target, Effect* fx);//120
int fx_fist_damage_modifier (Actor* Owner, Actor* target, Effect* fx);//121
int fx_title_modifier (Actor* Owner, Actor* target, Effect* fx);//122
int fx_disable_overlay_modifier (Actor* Owner, Actor* target, Effect* fx);//123
int fx_no_backstab_modifier (Actor* Owner, Actor* target, Effect* fx);//124
int fx_offscreenai_modifier (Actor* Owner, Actor* target, Effect* fx);//125
int fx_existance_delay_modifier (Actor* Owner, Actor* target, Effect* fx);//126
int fx_disable_chunk_modifier (Actor* Owner, Actor* target, Effect* fx);//127
int fx_protection_from_animation (Actor* Owner, Actor* target, Effect* fx);//128
int fx_protection_from_turn (Actor* Owner, Actor* target, Effect* fx);//129
int fx_cutscene2 (Actor* Owner, Actor* target, Effect* fx);//12a
int fx_chaos_shield_modifier (Actor* Owner, Actor* target, Effect* fx);//12b
int fx_npc_bump (Actor* Owner, Actor* target, Effect* fx);//12c
int fx_critical_hit_modifier (Actor* Owner, Actor* target, Effect* fx);//12d
int fx_can_use_any_item_modifier (Actor* Owner, Actor* target, Effect* fx);//12e
int fx_always_backstab_modifier (Actor* Owner, Actor* target, Effect* fx);//12f
int fx_mass_raise_dead (Actor* Owner, Actor* target, Effect* fx);//130
int fx_left_to_hit_modifier (Actor* Owner, Actor* target, Effect* fx);//131
int fx_right_to_hit_modifier (Actor* Owner, Actor* target, Effect* fx);//132
int fx_reveal_tracks (Actor* Owner, Actor* target, Effect* fx);//133
int fx_protection_from_tracking (Actor* Owner, Actor* target, Effect* fx);//134
int fx_modify_local_variable (Actor* Owner, Actor* target, Effect* fx);//135
int fx_timeless_modifier (Actor* Owner, Actor* target, Effect* fx);//136
int fx_generate_wish (Actor* Owner, Actor* target, Effect* fx);//137
//138 see fx_crash
//139 HLA generic effect
int fx_golem_stoneskin_modifier (Actor* Owner, Actor* target, Effect* fx);//13a
int fx_avatar_removal_modifier (Actor* Owner, Actor* target, Effect* fx);//13b
int fx_magical_rest (Actor* Owner, Actor* target, Effect* fx);//13c
//int fx_improved_haste_state (Actor* Owner, Actor* target, Effect* fx);//13d same as haste

int fx_unknown (Actor* Owner, Actor* target, Effect* fx);//???

// FIXME: Make this an ordered list, so we could use bsearch!
static EffectRef effectnames[] = {
	{ "*Crash*", fx_crash, -1 },
	{ "AcidResistanceModifier", fx_acid_resistance_modifier, -1 },
	{ "ACVsCreatureType", fx_generic_effect, -1 }, //0xdb
	{ "ACVsDamageTypeModifier", fx_ac_vs_damage_type_modifier, -1 },
	{ "ACVsDamageTypeModifier2", fx_ac_vs_damage_type_modifier, -1 }, // used in IWD
	{ "AidNonCumulative", fx_set_aid_state, -1 },
	{ "AIIdentifierModifier", fx_ids_modifier, -1 },
	{ "AlchemyModifier", fx_alchemy_modifier, -1 },
	{ "Alignment:Change", fx_alignment_change, -1 },
	{ "Alignment:Invert", fx_alignment_invert, -1 },
	{ "AlwaysBackstab", fx_always_backstab_modifier, -1 },
	{ "AnimationIDModifier", fx_animation_id_modifier, -1 },
	{ "AnimationStateChange", fx_animation_stance, -1 },
	{ "ApplyEffect", fx_apply_effect, -1 },
	{ "ApplyEffectCurse", fx_apply_effect_curse, -1 },
	{ "ApplyEffectItemType", fx_generic_effect, -1 }, //apply effect when itemtype is equipped
	{ "ApplyEffectRepeat", fx_apply_effect_repeat, -1 },
	{ "CutScene2", fx_cutscene2, -1 },
	{ "AttackSpeedModifier", fx_attackspeed_modifier, -1 },
	{ "AttacksPerRoundModifier", fx_attacks_per_round_modifier, -1 },
	{ "AuraCleansingModifier", fx_auracleansing_modifier, -1 },
	{ "AvatarRemoval", fx_avatar_removal, -1 }, //unknown
	{ "AvatarRemovalModifier", fx_avatar_removal_modifier, -1 },
	{ "BackstabModifier", fx_backstab_modifier, -1 },
	{ "BerserkStage1Modifier", fx_berserkstage1_modifier, -1},
	{ "BerserkStage2Modifier", fx_berserkstage2_modifier, -1},
	{ "BlessNonCumulative", fx_set_bless_state, -1 },
	{ "Bounce:School", fx_bounce_school, -1 },
	{ "Bounce:SchoolDec", fx_bounce_school_dec, -1 },
	{ "Bounce:SecondaryType", fx_bounce_secondary_type, -1 },
	{ "Bounce:SecondaryTypeDec", fx_bounce_secondary_type_dec, -1 },
	{ "Bounce:Spell", fx_bounce_spell, -1 },
	{ "Bounce:SpellDec", fx_bounce_spell_dec, -1 },
	{ "Bounce:SpellLevel", fx_bounce_spelllevel, -1},
	{ "Bounce:SpellLevelDec", fx_bounce_spelllevel_dec, -1},
	{ "Bounce:Opcode", fx_bounce_opcode, -1 },
	{ "Bounce:Projectile", fx_bounce_projectile, -1 },
	{ "CantUseItem", fx_generic_effect, -1 },
	{ "CantUseItemType", fx_generic_effect, -1 },
	{ "CanUseAnyItem", fx_can_use_any_item_modifier, -1 },
	{ "CastFromList", fx_select_spell, -1 },
	{ "CastingGlow", fx_casting_glow, -1 },
	{ "CastingGlow2", fx_casting_glow, -1 }, //used in iwd
	{ "CastingLevelModifier", fx_castinglevel_modifier, -1 },
	{ "CastingSpeedModifier", fx_castingspeed_modifier, -1 },
	{ "CastSpellOnCondition", fx_cast_spell_on_condition, -1 },
	{ "ChangeBardSong", fx_generic_effect, -1 },
	{ "ChangeName", fx_change_name, -1 },
	{ "ChantBadNonCumulative", fx_set_chantbad_state, -1 },
	{ "ChantNonCumulative", fx_set_chant_state, -1 },
	{ "ChaosShieldModifier", fx_chaos_shield_modifier, -1 },
	{ "CharismaModifier", fx_charisma_modifier, -1 },
	{ "CheckForBerserkModifier", fx_checkforberserk_modifier, -1},
	{ "ColdResistanceModifier", fx_cold_resistance_modifier, -1 },
	{ "Color:BriefRGB", fx_brief_rgb, -1},
	{ "Color:GlowRGB", fx_glow_rgb, -1},
	{ "Color:DarkenRGB", fx_darken_rgb, -1},
	{ "Color:SetPalette", fx_set_color_gradient, -1 },
	{ "Color:SetRGB", fx_set_color_rgb, -1 },
	{ "Color:SetRGBGlobal", fx_set_color_rgb_global, -1 }, //08
	{ "Color:PulseRGB", fx_set_color_pulse_rgb, -1 }, //9
	{ "Color:PulseRGBGlobal", fx_set_color_pulse_rgb_global, -1 }, //9
	{ "ConstitutionModifier", fx_constitution_modifier, -1 },
	{ "ControlCreature", fx_set_charmed_state, -1 }, //0xf1 same as charm
	{ "CreateContingency", fx_create_contingency, -1 },
	{ "CriticalHitModifier", fx_critical_hit_modifier, -1 },
	{ "CrushingResistanceModifier", fx_crushing_resistance_modifier, -1 },
	{ "Cure:Berserk", fx_cure_berserk_state, -1 },
	{ "Cure:Blind", fx_cure_blind_state, -1 },
	{ "Cure:CasterHold", fx_unpause_caster, -1 },
	{ "Cure:Confusion", fx_cure_confused_state, -1 },
	{ "Cure:Deafness", fx_cure_deaf_state, -1 },
	{ "Cure:Death", fx_cure_dead_state, -1 },
	{ "Cure:Defrost", fx_cure_frozen_state, -1 },
	{ "Cure:Disease", fx_cure_diseased_state, -1 },
	{ "Cure:Feeblemind", fx_cure_feebleminded_state, -1 },
	{ "Cure:Hold", fx_cure_hold_state, -1 },
	{ "Cure:Imprisonment", fx_freedom, -1 },
	{ "Cure:Infravision", fx_cure_infravision_state, -1 },
	{ "Cure:Intoxication", fx_cure_intoxication, -1 }, //0xa4 (iwd2 has this working)
	{ "Cure:Invisible", fx_cure_invisible_state, -1 }, //0x2f
	{ "Cure:Invisible2", fx_cure_invisible_state, -1 }, //0x74
	//{ "Cure:ImprovedInvisible", fx_cure_improved_invisible_state, -1 },
	{ "Cure:LevelDrain", fx_cure_leveldrain, -1}, //restoration
	{ "Cure:Nondetection", fx_cure_nondetection_state, -1 },
	{ "Cure:Panic", fx_cure_panic_state, -1 },
	{ "Cure:Petrification", fx_cure_petrified_state, -1 },
	{ "Cure:Poison", fx_cure_poisoned_state, -1 },
	{ "Cure:Sanctuary", fx_cure_sanctuary_state, -1 },
	{ "Cure:Silence", fx_cure_silenced_state, -1 },
	{ "Cure:Sleep", fx_cure_sleep_state, -1 },
	{ "Cure:Stun", fx_cure_stun_state, -1 },
	{ "CurrentHPModifier", fx_current_hp_modifier, -1 },
	{ "Damage", fx_damage, -1 },
	{ "DamageAnimation", fx_damage_animation, -1 },
	{ "DamageBonusModifier", fx_damage_bonus_modifier, -1 },
	{ "DamageLuckModifier", fx_damageluck_modifier, -1 },
	{ "DamageVsCreature", fx_generic_effect, -1 },
	{ "Death", fx_death, -1 },
	{ "Death2", fx_death, -1 }, //(iwd2 effect)
	{ "Death3", fx_death, -1 }, //(iwd2 effect too, Banish)
	{ "DetectAlignment", fx_detect_alignment, -1 },
	{ "DetectIllusionsModifier", fx_detect_illusion_modifier, -1 },
	{ "DexterityModifier", fx_dexterity_modifier, -1 },
	{ "DimensionDoor", fx_dimension_door, -1 },
	{ "DisableButton", fx_disable_button, -1 }, //sets disable button flag
	{ "DisableChunk", fx_disable_chunk_modifier, -1 },
	{ "DisableOverlay", fx_disable_overlay_modifier, -1 },
	{ "DisableCasting", fx_disable_spellcasting, -1 },
	{ "Disintegrate", fx_disintegrate, -1 },
	{ "DispelEffects", fx_dispel_effects, -1 },
	{ "DispelSchool", fx_dispel_school, -1 },
	{ "DispelSchoolOne", fx_dispel_school_one, -1 },
	{ "DispelSecondaryType", fx_dispel_secondary_type, -1 },
	{ "DispelSecondaryTypeOne", fx_dispel_secondary_type_one, -1 },
	{ "DisplayString", fx_display_string, -1 },
	{ "Dither", fx_dither, -1 },
	{ "DontJumpModifier", fx_dontjump_modifier, -1 },
	{ "DrainItems", fx_drain_items, -1 },
	{ "DrainSpells", fx_drain_spells, -1 },
	{ "DropWeapon", fx_drop_weapon, -1 },
	{ "ElectricityResistanceModifier", fx_electricity_resistance_modifier, -1 },
	{ "ExistanceDelayModifier", fx_existance_delay_modifier , -1 }, //unknown
	{ "ExperienceModifier", fx_experience_modifier, -1 },
	{ "ExploreModifier", fx_explore_modifier, -1 },
	{ "FamiliarBond", fx_familiar_constitution_loss, -1 },
	{ "FamiliarMarker", fx_familiar_marker, -1},
	{ "Farsee", fx_farsee, -1},
	{ "FatigueModifier", fx_fatigue_modifier, -1 },
	{ "FindFamiliar", fx_find_familiar, -1 },
	{ "FindTraps", fx_find_traps, -1 },
	{ "FindTrapsModifier", fx_find_traps_modifier, -1 },
	{ "FireResistanceModifier", fx_fire_resistance_modifier, -1 },
	{ "FistDamageModifier", fx_fist_damage_modifier, -1 },
	{ "FistHitModifier", fx_fist_to_hit_modifier, -1 },
	{ "ForceSurgeModifier", fx_force_surge_modifier, -1 },
	{ "ForceVisible", fx_force_visible, -1 }, //not invisible but improved invisible
	{ "FreeAction", fx_cure_slow_state, -1},
	{ "GenerateWish", fx_generate_wish, -1},
	{ "GoldModifier", fx_gold_modifier, -1 },
	{ "HideInShadowsModifier", fx_hide_in_shadows_modifier, -1},
	{ "HLA", fx_generic_effect, -1},
	{ "HolyNonCumulative", fx_set_holy_state, -1 },
	{ "Icon:Disable", fx_disable_portrait_icon, -1 },
	{ "Icon:Display", fx_display_portrait_icon, -1 },
	{ "Icon:Remove", fx_remove_portrait_icon, -1 },
	{ "Identify", fx_identify, -1 },
	{ "IgnoreDialogPause", fx_ignore_dialogpause_modifier, -1 },
	{ "IntelligenceModifier", fx_intelligence_modifier, -1 },
	{ "IntoxicationModifier", fx_intoxication_modifier, -1 },
	{ "InvisibleDetection", fx_see_invisible_modifier, -1 },
	{ "Item:CreateDays", fx_create_item_days, -1 },
	{ "Item:CreateInSlot", fx_create_item_in_slot, -1 },
	{ "Item:CreateInventory", fx_create_inventory_item, -1 },
	{ "Item:CreateMagic", fx_create_magic_item, -1 },
	{ "Item:Equip", fx_equip_item, -1 }, //71
	{ "Item:Remove", fx_remove_item, -1 }, //70
	{ "Item:RemoveInventory", fx_remove_inventory_item, -1 },
	{ "KillCreatureType", fx_kill_creature_type, -1 },
	{ "LevelModifier", fx_level_modifier, -1 },
	{ "LevelDrainModifier", fx_leveldrain_modifier, -1 },
	{ "LoreModifier", fx_lore_modifier, -1 },
	{ "LuckModifier", fx_luck_modifier, -1 },
	{ "LuckCumulative", fx_luck_cumulative, -1 },
	{ "LuckNonCumulative", fx_luck_non_cumulative, -1 },
	{ "MagicalColdResistanceModifier", fx_magical_cold_resistance_modifier, -1 },
	{ "MagicalFireResistanceModifier", fx_magical_fire_resistance_modifier, -1 },
	{ "MagicalRest", fx_magical_rest, -1 },
	{ "MagicDamageResistanceModifier", fx_magic_damage_resistance_modifier, -1 },
	{ "MagicResistanceModifier", fx_magic_resistance_modifier, -1 },
	{ "MassRaiseDead", fx_mass_raise_dead, -1 },
	{ "MaximumHPModifier", fx_maximum_hp_modifier, -1 },
	{ "Maze", fx_maze, -1},
	{ "MeleeDamageModifier", fx_melee_damage_modifier, -1 },
	{ "MeleeHitModifier", fx_melee_to_hit_modifier, -1 },
	{ "MinimumHPModifier", fx_minimum_hp_modifier, -1 },
	{ "MiscastMagicModifier", fx_miscast_magic_modifier, -1 },
	{ "MissileDamageModifier", fx_missile_damage_modifier, -1 },
	{ "MissileHitModifier", fx_missile_to_hit_modifier, -1 },
	{ "MissilesResistanceModifier", fx_missiles_resistance_modifier, -1 },
	{ "MirrorImage", fx_mirror_image, -1 },
	{ "MirrorImageModifier", fx_mirror_image_modifier, -1 },
	{ "ModifyGlobalVariable", fx_modify_global_variable, -1 },
	{ "ModifyLocalVariable", fx_modify_local_variable, -1 },
	{ "MonsterSummoning", fx_monster_summoning, -1 },
	{ "MoraleBreakModifier", fx_morale_break_modifier, -1 },
	{ "MoraleModifier", fx_morale_modifier, -1 },
	{ "MovementRateModifier", fx_movement_modifier, -1 }, //fast (7e)
	{ "MovementRateModifier2", fx_movement_modifier, -1 },//slow (b0)
	{ "MovementRateModifier3", fx_movement_modifier, -1 },//forced (IWD - 10a)
	{ "MovementRateModifier4", fx_movement_modifier, -1 },//slow (IWD2 - 1b9)
	{ "MoveToArea", fx_move_to_area, -1 }, //0xba
	{ "NoCircleState", fx_no_circle_state, -1 },
	{ "NPCBump", fx_npc_bump, -1 },
	{ "OffscreenAIModifier", fx_offscreenai_modifier, -1 },
	{ "OffhandHitModifier", fx_left_to_hit_modifier, -1 },
	{ "OpenLocksModifier", fx_open_locks_modifier, -1 },
	{ "Overlay:Entangle", fx_set_entangle_state, -1 },
	{ "Overlay:Grease", fx_set_grease_state, -1 },
	{ "Overlay:MinorGlobe", fx_set_minorglobe_state, -1 },
	{ "Overlay:Sanctuary", fx_set_sanctuary_state, -1 },
	{ "Overlay:ShieldGlobe", fx_set_shieldglobe_state, -1 },
	{ "Overlay:Web", fx_set_web_state, -1 },
	{ "PauseTarget", fx_pause_target, -1 }, //also known as casterhold
	{ "PickPocketsModifier", fx_pick_pockets_modifier, -1 },
	{ "PiercingResistanceModifier", fx_piercing_resistance_modifier, -1 },
	{ "PlayMovie", fx_play_movie, -1 },
	{ "PlaySound", fx_playsound, -1 },
	{ "PlayVisualEffect", fx_play_visual_effect, -1 },
	{ "PoisonResistanceModifier", fx_poison_resistance_modifier, -1 },
	{ "Polymorph", fx_polymorph, -1},
	{ "PortraitChange", fx_portrait_change, -1 },
	{ "PowerWordKill", fx_power_word_kill, -1 },
	{ "PowerWordSleep", fx_power_word_sleep, -1 },
	{ "PowerWordStun", fx_power_word_stun, -1 },
	{ "PriestSpellSlotsModifier", fx_bonus_priest_spells, -1 },
	{ "Proficiency", fx_proficiency, -1 },
	{ "Protection:Animation", fx_generic_effect, -1 },
	{ "Protection:Backstab", fx_no_backstab_modifier, -1 },
	{ "Protection:Creature", fx_generic_effect, -1 },
	{ "Protection:Opcode", fx_protection_opcode, -1 },
	{ "Protection:Opcode2", fx_protection_opcode, -1 },
	{ "Protection:Projectile",fx_protection_from_projectile, -1},
	{ "Protection:School",fx_generic_effect,-1},//overlay?
	{ "Protection:SchoolDec",fx_protection_school_dec,-1},//overlay?
	{ "Protection:SecondaryType",fx_protection_secondary_type,-1},//overlay?
	{ "Protection:SecondaryTypeDec",fx_protection_secondary_type_dec,-1},//overlay?
	{ "Protection:Spell",fx_resist_spell,-1},//overlay?
	{ "Protection:SpellDec",fx_resist_spell_dec,-1},//overlay?
	{ "Protection:SpellLevel",fx_protection_spelllevel,-1},//overlay?
	{ "Protection:SpellLevelDec",fx_protection_spelllevel_dec,-1},//overlay?
	{ "Protection:String", fx_generic_effect, -1 },
	{ "Protection:Tracking", fx_protection_from_tracking, -1 },
	{ "Protection:Turn", fx_protection_from_turn, -1},
	{ "Protection:Weapons", fx_immune_to_weapon, -1},
	{ "PuppetMarker", fx_puppet_marker, -1},
	{ "ProjectImage", fx_puppet_master, -1},
	{ "RetreatFrom", fx_retreat_from, -1 },
	{ "Reveal:Area", fx_reveal_area, -1 },
	{ "Reveal:Creatures", fx_reveal_creatures, -1 },
	{ "Reveal:Magic", fx_reveal_magic, -1 },
	{ "Reveal:Tracks", fx_reveal_tracks, -1 },
	{ "RemoveCurse", fx_remove_curse, -1 },
	{ "RemoveImmunity", fx_remove_immunity, -1 },
	{ "RemoveMapNote", fx_remove_map_note, -1 },
	{ "RemoveProjectile", fx_remove_projectile, -1 }, //removes effects from actor and area
	{ "RenableButton", fx_renable_button, -1 }, //removes disable button flag
	{ "RemoveCreature", fx_remove_creature, -1 },
	{ "ReplaceCreature", fx_replace_creature, -1 },
	{ "ReputationModifier", fx_reputation_modifier, -1 },
	{ "RestoreSpells", fx_restore_spell_level, -1 },
	{ "RightHitModifier", fx_right_to_hit_modifier, -1 },
	{ "SaveVsBreathModifier", fx_save_vs_breath_modifier, -1 },
	{ "SaveVsDeathModifier", fx_save_vs_death_modifier, -1 },
	{ "SaveVsPolyModifier", fx_save_vs_poly_modifier, -1 },
	{ "SaveVsSpellsModifier", fx_save_vs_spell_modifier, -1 },
	{ "SaveVsWandsModifier", fx_save_vs_wands_modifier, -1 },
	{ "ScreenShake", fx_screenshake, -1 },
	{ "ScriptingState", fx_scripting_state, -1 },
	{ "Sequencer:Activate", fx_activate_spell_sequencer, -1 },
	{ "Sequencer:Create", fx_create_spell_sequencer, -1 },
	{ "Sequencer:Store", fx_store_spell_sequencer, -1 },
	{ "SetAIScript", fx_set_ai_script, -1 },
	{ "SetMapNote", fx_set_map_note, -1 },
	{ "SetMeleeEffect", fx_generic_effect, -1 },
	{ "SetRangedEffect", fx_generic_effect, -1 },
	{ "SetTrap", fx_set_area_effect, -1 },
	{ "SetTrapsModifier", fx_set_traps_modifier, -1 },
	{ "SexModifier", fx_sex_modifier, -1 },
	{ "SlashingResistanceModifier", fx_slashing_resistance_modifier, -1 },
	{ "SlowPoisonDamageRate", fx_generic_effect, -1 }, //slow poison effect
	{ "Sparkle", fx_sparkle, -1 },
	{ "SpellDurationModifier", fx_spell_duration_modifier, -1 },
	{ "Spell:Add", fx_add_innate, -1 },
	{ "Spell:Cast", fx_cast_spell, -1 },
	{ "Spell:CastPoint", fx_cast_spell_point, -1 },
	{ "Spell:Learn", fx_learn_spell, -1 },
	{ "Spell:Remove", fx_remove_spell, -1 },
	{ "Spelltrap",fx_spelltrap , -1 }, //overlay: spmagglo
	{ "State:Berserk", fx_set_berserk_state, -1 },
	{ "State:Blind", fx_set_blind_state, -1 },
	{ "State:Blur", fx_set_blur_state, -1 },
	{ "State:Charmed", fx_set_charmed_state, -1 }, //0x05
	{ "State:Confused", fx_set_confused_state, -1 },
	{ "State:Deafness", fx_set_deaf_state, -1 },
	{ "State:DeafnessIWD2", fx_set_deaf_state_iwd2, -1 }, //this is a modified version
	{ "State:Diseased", fx_set_diseased_state, -1 },
	{ "State:Feeblemind", fx_set_feebleminded_state, -1 },
	{ "State:Hasted", fx_set_hasted_state, -1 },
	{ "State:Haste2", fx_set_hasted_state, -1 },
	{ "State:Hold", fx_hold_creature, -1 }, //175 (doesn't work in original iwd2)
	{ "State:Hold2", fx_hold_creature, -1 },//185 (doesn't work in original iwd2)
	{ "State:Hold3", fx_hold_creature, -1 },//109 iwd2
	{ "State:HoldNoIcon", fx_hold_creature_no_icon, -1 }, //109
	{ "State:HoldNoIcon2", fx_hold_creature_no_icon, -1 }, //0xfb (iwd/iwd2)
	{ "State:HoldNoIcon3", fx_hold_creature_no_icon, -1 }, //0x1a8 (iwd2)
	{ "State:Imprisonment", fx_imprisonment, -1 },
	{ "State:Infravision", fx_set_infravision_state, -1 },
	{ "State:Invisible", fx_set_invisible_state, -1 }, //both invis or improved invis
	{ "State:Nondetection", fx_set_nondetection_state, -1 },
	{ "State:Panic", fx_set_panic_state, -1 },
	{ "State:Petrification", fx_set_petrified_state, -1 },
	{ "State:Poisoned", fx_set_poisoned_state, -1 },
	{ "State:Regenerating", fx_set_regenerating_state, -1 },
	{ "State:Silenced", fx_set_silenced_state, -1 },
	{ "State:Helpless", fx_set_unconscious_state, -1 },
	{ "State:Sleep", fx_set_unconscious_state, -1},
	{ "State:Slowed", fx_set_slowed_state, -1 },
	{ "State:Stun", fx_set_stun_state, -1 },
	{ "StealthModifier", fx_stealth_modifier, -1 },
	{ "StoneSkinModifier", fx_stoneskin_modifier, -1 },
	{ "StoneSkin2Modifier", fx_golem_stoneskin_modifier, -1 },
	{ "StrengthModifier", fx_strength_modifier, -1 },
	{ "StrengthBonusModifier", fx_strength_bonus_modifier, -1 },
	{ "SummonCreature", fx_summon_creature, -1 },
	{ "RandomTeleport", fx_teleport_field, -1 },
	{ "TeleportToTarget", fx_teleport_to_target, -1 },
	{ "TimelessState", fx_timeless_modifier, -1 },
	{ "Timestop", fx_timestop, -1},
	{ "TitleModifier", fx_title_modifier, -1 },
	{ "ToHitModifier", fx_to_hit_modifier, -1 },
	{ "ToHitBonusModifier", fx_to_hit_bonus_modifier, -1 },
	{ "ToHitVsCreature", fx_generic_effect, -1 },
	{ "TrackingModifier", fx_tracking_modifier, -1 },
	{ "TransparencyModifier", fx_transparency_modifier, -1 },
	{ "Unknown", fx_unknown, -1},
	{ "Unlock", fx_knock, -1 }, //open doors/containers
	{ "UnsummonCreature", fx_unsummon_creature, -1 },
	{ "Variable:StoreLocalVariable", fx_local_variable, -1 },
	{ "VisualAnimationEffect", fx_visual_animation_effect, -1 }, //unknown
	{ "VisualRangeModifier", fx_visual_range_modifier, -1 },
	{ "VisualSpellHit", fx_visual_spell_hit, -1 },
	{ "WildSurgeModifier", fx_wild_surge_modifier, -1 },
	{ "WingBuffet", fx_wing_buffet, -1 },
	{ "WisdomModifier", fx_wisdom_modifier, -1 },
	{ "WizardSpellSlotsModifier", fx_bonus_wizard_spells, -1 },
	{ NULL, NULL, 0 },
};


FXOpc::FXOpc(void)
{
	core->RegisterOpcodes( sizeof( effectnames ) / sizeof( EffectRef ) - 1, effectnames );
	enhanced_effects=!!core->HasFeature(GF_ENHANCED_EFFECTS);
	default_spell_hit.SequenceFlags|=IE_VVC_BAM;
}

FXOpc::~FXOpc(void)
{
	core->FreeResRefTable(casting_glows, cgcount);
	core->FreeResRefTable(spell_hits, shcount);
	if(spell_abilities) free(spell_abilities);
	spell_abilities=NULL;
	if(polymorph_stats) free(polymorph_stats);
	polymorph_stats=NULL;
}

static inline void SetGradient(Actor *target, ieDword gradient)
{
	gradient |= (gradient <<16);
	gradient |= (gradient <<8);
	for(int i=0;i<7;i++) {
		STAT_SET(IE_COLORS+i, gradient);
	}
}

static inline void HandleBonus(Actor *target, int stat, int mod, int mode)
{
	if (mode==FX_DURATION_INSTANT_PERMANENT) {
		if (target->IsReverseToHit()) {
			BASE_ADD( stat, mod );
		} else {
			BASE_SUB( stat, mod );
		}
		return;
	}
	if (target->IsReverseToHit()) {
		STAT_ADD( stat, mod );
	} else {
		STAT_SUB( stat, mod );
	}
}

//whoseeswho:
#define ENEMY_SEES_ORIGIN 1
#define ORIGIN_SEES_ENEMY 2

inline Actor *GetNearestEnemyOf(Map *map, Actor *origin, int whoseeswho)
{
	//determining the allegiance of the origin
	int type = 2; //neutral, has no enemies
	if (origin->GetStat(IE_EA) <= EA_GOODCUTOFF) {
		type = 1; //PC
	}
	if (origin->GetStat(IE_EA) >= EA_EVILCUTOFF) {
		type = 0;
	}

	//neutral has no enemies
	if (type==2) {
		return NULL;
	}

	Targets *tgts = new Targets();

	int i = map->GetActorCount(true);
	Actor *ac;
	while (i--) {
		ac=map->GetActor(i,true);
		int distance = Distance(ac, origin);
		if (whoseeswho&ENEMY_SEES_ORIGIN) {
			if (!CanSee(ac, origin, true, GA_NO_DEAD)) {
				continue;
			}
		}
		if (whoseeswho&ORIGIN_SEES_ENEMY) {
			if (!CanSee(ac, origin, true, GA_NO_DEAD)) {
				continue;
			}
		}

		if (type) { //origin is PC
			if (ac->GetStat(IE_EA) >= EA_EVILCUTOFF) {
				tgts->AddTarget(ac, distance, GA_NO_DEAD);
			}
		}
		else {
			if (ac->GetStat(IE_EA) <= EA_GOODCUTOFF) {
				tgts->AddTarget(ac, distance, GA_NO_DEAD);
			}
		}
	}
	ac = (Actor *) tgts->GetTarget(0, ST_ACTOR);
	delete tgts;
	return ac;
}
// Effect opcodes

// 0x00 ACVsDamageTypeModifier
int fx_ac_vs_damage_type_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_ac_vs_damage_type_modifier (%2d): AC Modif: %d ; Type: %d ; MinLevel: %d ; MaxLevel: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2, (int) fx->DiceSides, (int) fx->DiceThrown );
	//check level was pulled outside as a common functionality
	//CHECK_LEVEL();

	// it is a bitmask
	int type = fx->Parameter2;
	if (type == 0) {
		HandleBonus(target, IE_ARMORCLASS, fx->Parameter1, fx->TimingMode);
		return FX_PERMANENT;
	}

	// FIXME: set to Param1 or Param1-1 ?
	if (type == 16) {
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
int fx_attacks_per_round_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_attacks_per_round_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	int tmp = (signed) fx->Parameter1;
	if (fx->Parameter2!=2) {
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
// unconscious effect, which is combined with STATE_HELPLESS (0x20+1)
static EffectRef fx_set_sleep_state_ref={"State:Helpless",NULL,-1};
//this reference is used by many other effects
static EffectRef fx_display_portrait_icon_ref={"Icon:Display",NULL,-1};

int fx_cure_sleep_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_sleep_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	BASE_STATE_CURE( STATE_SLEEP );
	target->fxqueue.RemoveAllEffects(fx_set_sleep_state_ref);
	target->fxqueue.RemoveAllEffectsWithParam(fx_display_portrait_icon_ref, PI_SLEEP);
	return FX_NOT_APPLIED;
}

// 0x03 Cure:Berserk
// this effect clears the STATE_BERSERK (2) bit, but bg2 actually ignores the bit
// it also removes effect 04
static EffectRef fx_set_berserk_state_ref={"State:Berserk",NULL,-1};

int fx_cure_berserk_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_berserk_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	BASE_STATE_CURE( STATE_BERSERK );
	target->fxqueue.RemoveAllEffects(fx_set_berserk_state_ref);
	return FX_NOT_APPLIED;
}

// 0x04 State:Berserk
// this effect sets the STATE_BERSERK bit, but bg2 actually ignores the bit
int fx_set_berserk_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_berserk_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//
	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_STATE_SET( STATE_BERSERK );
	} else {
		STATE_SET( STATE_BERSERK );
	}

	switch(fx->Parameter2) {
	case 1: //always berserk
		target->SetSpellState(SS_BERSERK);
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
		target->SetColorMod(0xff, RGBModifier::ADD, 15, 128, 0, 0);
		target->AddPortraitIcon(PI_BLOODRAGE);
		break;
	}
	return FX_PERMANENT;
}

// 0x05 State:Charmed
// 0xf1 ControlCreature (iwd2)
int fx_set_charmed_state (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_charmed_state (%2d): General: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	bool enemyally = Owner->GetStat(IE_EA)>EA_GOODCUTOFF; //or evilcutoff?

	switch (fx->Parameter2) {
	case 0: //charmed (target neutral after charm)
		core->DisplayConstantStringName(STR_CHARMED, 0xf0f0f0, target);
	case 1000:
		break;
	case 1: //charmed (target hostile after charm)
		core->DisplayConstantStringName(STR_CHARMED, 0xf0f0f0, target);
	case 1001:
		target->SetBaseNoPCF(IE_EA, EA_ENEMY);
		break;
	case 2: //dire charmed (target neutral after charm)
		core->DisplayConstantStringName(STR_DIRECHARMED, 0xf0f0f0, target);
	case 1002:
		break;
	case 3: //dire charmed (target hostile after charm)
		core->DisplayConstantStringName(STR_DIRECHARMED, 0xf0f0f0, target);
	case 1003:
		target->SetBaseNoPCF(IE_EA, EA_ENEMY);
		break;
	case 4: //controlled by cleric
		core->DisplayConstantStringName(STR_CONTROLLED, 0xf0f0f0, target);
	case 1004:
		target->SetBaseNoPCF(IE_EA, EA_ENEMY);
		break;
	case 5: //thrall (typo comes from original engine doc)
		core->DisplayConstantStringName(STR_CHARMED, 0xf0f0f0, target);
	case 1005:
		STAT_SET(IE_EA, EA_ENEMY );
		STAT_SET(IE_THRULLCHARM, 1);
		return FX_PERMANENT;
	}

	STATE_SET( STATE_CHARMED );
	STAT_SET( IE_EA, enemyally?EA_ENEMY:EA_CHARMED );
	//don't stick if permanent
	return FX_PERMANENT;
}

// 0x06 CharismaModifier
int fx_charisma_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_charisma_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD( IE_CHR );
	} else {
		STAT_MOD( IE_CHR );
	}
	return FX_PERMANENT;
}

// 0x07 Color:SetPalette
// this effect might not work in pst, they don't have separate weapon slots
int fx_set_color_gradient (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_color_gradient (%2d): Gradient: %d, Location: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	EffectQueue::HackColorEffects(target, fx);
	target->SetColor( fx->Parameter2, fx->Parameter1 );
	return FX_APPLIED;
}

// 08 Color:SetRGB
int fx_set_color_rgb (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_color_rgb (%2d): RGB: %x, Location: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	EffectQueue::HackColorEffects(target, fx);
	ieDword location = fx->Parameter2 & 0xff;
	target->SetColorMod(location, RGBModifier::ADD, -1, fx->Parameter1 >> 8,
			fx->Parameter1 >> 16, fx->Parameter1 >> 24);

	return FX_APPLIED;
}
// 08 Color:SetRGBGlobal
int fx_set_color_rgb_global (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_color_rgb_global (%2d): RGB: %x, Location: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->SetColorMod(0xff, RGBModifier::ADD, -1, fx->Parameter1 >> 8,
			fx->Parameter1 >> 16, fx->Parameter1 >> 24);

	return FX_APPLIED;
}

// 09 Color:PulseRGB
int fx_set_color_pulse_rgb (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_color_pulse_rgb (%2d): RGB: %x, Location: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	EffectQueue::HackColorEffects(target, fx);
	ieDword location = fx->Parameter2 & 0xff;
	int speed = (fx->Parameter2 >> 16) & 0xFF;
	target->SetColorMod(location, RGBModifier::ADD, speed,
			fx->Parameter1 >> 8, fx->Parameter1 >> 16,
			fx->Parameter1 >> 24);

	return FX_APPLIED;
}

// 09 Color:PulseRGBGlobal (pst variant)
int fx_set_color_pulse_rgb_global (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_color_pulse_rgb_global (%2d): RGB: %x, Location: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	int speed = (fx->Parameter2 >> 16) & 0xFF;
	target->SetColorMod(0xff, RGBModifier::ADD, speed,
			fx->Parameter1 >> 8, fx->Parameter1 >> 16,
			fx->Parameter1 >> 24);

	return FX_APPLIED;
}

// 0x0A ConstitutionModifier
int fx_constitution_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_constitution_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD( IE_CON );
	} else {
		STAT_MOD( IE_CON );
	}
	return FX_PERMANENT;
}

// 0x0B Cure:Poison
static EffectRef fx_poisoned_state_ref={"State:Poison",NULL,-1};

int fx_cure_poisoned_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_poisoned_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//all three steps are present in bg2 and iwd2
	BASE_STATE_CURE( STATE_POISONED );
	target->fxqueue.RemoveAllEffects( fx_poisoned_state_ref );
	target->fxqueue.RemoveAllEffectsWithParam(fx_display_portrait_icon_ref, PI_POISONED);
	return FX_NOT_APPLIED;
}

// 0x0C Damage
// this is a very important effect
int fx_damage (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_damage (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//save for half damage type
	ieDword damagetype = fx->Parameter2;
	if ((damagetype&3)==3) {
		damagetype&=~3;
	}
	target->Damage(fx->Parameter1, damagetype, Owner);
	//this effect doesn't stick
	return FX_NOT_APPLIED;
}

//0x0D Death
int fx_death (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_death (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_SET(IE_MINHITPOINTS,0); //the die opcode seems to override minhp
	ieDword damagetype = 0;
	switch (fx->Parameter2) {
	case 1:
		BASE_STATE_SET(STATE_D4); //not sure, should be charred
		damagetype = DAMAGE_FIRE;
		break;
	case 2:
		damagetype = DAMAGE_CRUSHING;
		break;
	case 4:
		damagetype = DAMAGE_CRUSHING;
		break;
	case 8:
		damagetype = DAMAGE_CRUSHING|DAMAGE_CHUNKING;
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
		BASE_STATE_SET(STATE_PETRIFIED);
		damagetype = DAMAGE_CRUSHING|DAMAGE_CHUNKING;
		break;
	case 128:
		BASE_STATE_SET(STATE_FROZEN);
		damagetype = DAMAGE_COLD|DAMAGE_CHUNKING;
		break;
	case 256:
		damagetype = DAMAGE_ELECTRICITY;
		break;
	case 512:

	default:
		damagetype = DAMAGE_ACID;
	}
	target->Damage(0, damagetype, Owner);
	//death has damage type too
	target->Die(Owner);
	//this effect doesn't stick
	return FX_NOT_APPLIED;
}

// 0xE Cure:Defrost
int fx_cure_frozen_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_frozen_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	BASE_STATE_CURE( STATE_FROZEN );
	return FX_NOT_APPLIED;
}

// 0x0F DexterityModifier

#define CSA_DEX 0
#define CSA_STR 1
#define CSA_CNT 2
int SpellAbilityDieRoll(Actor *target, int which)
{
	if (which>=CSA_CNT) return 6;

	ieDword cls = STAT_GET(IE_CLASS);
	if (!spell_abilities) {
		AutoTable tab("clssplab");
		if (!tab) {
			spell_abilities = (int *) malloc(sizeof(int)*CSA_CNT);
			for (int ab=0;ab<CSA_CNT;ab++) {
				spell_abilities[ab*splabcount]=6;
			}
			splabcount=1;
			return 6;
		}
		splabcount = tab->GetRowCount();
		spell_abilities=(int *) malloc(sizeof(int)*splabcount*CSA_CNT);
		for (int ab=0;ab<CSA_CNT;ab++) {
			for (ieDword i=0;i<splabcount;i++) {
				spell_abilities[ab*splabcount+i]=atoi(tab->QueryField(i,ab));
			}
		}
	}
	if (cls>=splabcount) cls=0;
	return spell_abilities[which*splabcount+cls];
}

int fx_dexterity_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_dexterity_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	////how cat's grace: value is based on class
	if (fx->Parameter2==3) {
		fx->Parameter1 = core->Roll(1,SpellAbilityDieRoll(target,0),0);
		fx->Parameter2 = 0;
	}

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD( IE_DEX );
	} else {
		STAT_MOD( IE_DEX );
	}
	return FX_PERMANENT;
}

static EffectRef fx_set_slow_state_ref={"State:Slowed",NULL,-1};
// 0x10 State:Hasted
// this function removes slowed state, or sets hasted state
int fx_set_hasted_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_hasted_state (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	target->fxqueue.RemoveAllEffects(fx_set_slow_state_ref);
	target->fxqueue.RemoveAllEffectsWithParam( fx_display_portrait_icon_ref, PI_SLOWED );
	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_STATE_CURE( STATE_SLOWED );
		BASE_STATE_SET( STATE_HASTED );
	} else {
		STATE_CURE( STATE_SLOWED );
		STATE_SET( STATE_HASTED );
	}
	target->NewStat(IE_MOVEMENTRATE, 200, MOD_PERCENT);
	switch (fx->Parameter2) {
	case 0: //normal haste
		target->AddPortraitIcon(PI_HASTED);
		STAT_SET(IE_IMPROVEDHASTE,0);
		STAT_SET(IE_ATTACKNUMBERDOUBLE,0);
		break;
	case 1://improved haste
		target->AddPortraitIcon(PI_IMPROVEDHASTE);
		STAT_SET(IE_IMPROVEDHASTE,1);
		STAT_SET(IE_ATTACKNUMBERDOUBLE,0);
		break;
	case 2://speed haste only
		target->AddPortraitIcon(PI_HASTED);
		STAT_SET(IE_IMPROVEDHASTE,0);
		STAT_SET(IE_ATTACKNUMBERDOUBLE,1);
		break;
	}

	return FX_PERMANENT;
}

// 0x11 CurrentHPModifier
int fx_current_hp_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_current_hp_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	if (target->HasSpellState(SS_BLOODRAGE)) {
		return FX_NOT_APPLIED;
	}

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		//BASE_MOD( IE_HITPOINTS );
		target->NewBase( IE_HITPOINTS, fx->Parameter1, fx->Parameter2&0xffff);
	} else {
		//STAT_MOD( IE_HITPOINTS );
		target->NewStat( IE_HITPOINTS, fx->Parameter1, fx->Parameter2&0xffff);
	}
	if (fx->Parameter2&0x10000) {
		target->Resurrect();
	}
	if (fx->Parameter2&0x20000) {
		target->fxqueue.RemoveAllNonPermanentEffects();
	}
	return FX_PERMANENT;
}

// 0x12 MaximumHPModifier
// 0 and 3 differ in that 3 doesn't modify current hitpoints
// 1,4 are the same
// 2,5 are the same
int fx_maximum_hp_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_maximum_hp_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	case 3:
		// random value Parameter1 is set by level_check in EffectQueue
		if (base) {
			BASE_ADD( IE_MAXHITPOINTS, fx->Parameter1 );
		} else {
			STAT_ADD( IE_MAXHITPOINTS, fx->Parameter1 );
		}
		break;
	case 1:case 4:
		if (base) {
			BASE_SET( IE_MAXHITPOINTS, fx->Parameter1 );
		} else {
			STAT_SET( IE_MAXHITPOINTS, fx->Parameter1 );
		}
		break;
	case 2:case 5:
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
int fx_intelligence_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_intelligence_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
int fx_set_invisible_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	switch (fx->Parameter2) {
	case 0:
		STATE_SET( STATE_INVISIBLE );
		break;
	case 1:
		STATE_SET( STATE_INVIS2 );
		break;
	default:
		break;
	}
	ieDword Trans = fx->Parameter4;
	if (fx->Parameter3) {
		if (Trans>=240) {
			fx->Parameter3=0;
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
	return FX_APPLIED;
}

// 0x15 LoreModifier
int fx_lore_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_lore_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_LORE );
	return FX_APPLIED;
}

// 0x16 LuckModifier
int fx_luck_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_luck_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_LUCK );
	return FX_APPLIED;
}

// 0x17 MoraleModifier
int fx_morale_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_morale_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_MORALE );
	return FX_APPLIED;
}

// 0x18 State:Panic
int fx_set_panic_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_panic_state (%2d)\n", fx->Opcode );

	if (target->HasSpellState(SS_BLOODRAGE)) {
		return FX_NOT_APPLIED;
	}

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_STATE_SET( STATE_PANIC );
	} else {
		STATE_SET( STATE_PANIC );
	}
	if (enhanced_effects) {
		target->AddPortraitIcon(PI_PANIC);
	}
	return FX_PERMANENT;
}

// 0x19 State:Poisoned
int fx_set_poisoned_state (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_poisoned_state (%2d): Damage: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//apparently this bit isn't set, but then why is it here
	//this requires a little research
	STATE_SET( STATE_POISONED );
	//also this effect is executed every update
	ieDword damage;
	int tmp = fx->Parameter1;

	switch(fx->Parameter2) {
	case RPD_PERCENT:
		damage = target->GetStat(IE_MAXHITPOINTS) * fx->Parameter1 / 100;
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
	target->Damage(damage, DAMAGE_POISON, Owner);
	return FX_APPLIED;
}

// 0x1a RemoveCurse
static EffectRef fx_apply_effect_curse_ref={"ApplyEffectCurse",NULL,-1};
static EffectRef fx_pst_jumble_curse_ref={"JumbleCurse",NULL,-1};

// gemrb extension: if the resource field is filled, it will remove curse only from the specified item
int fx_remove_curse (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_remove_curse (%2d): Resource: %s Type: %d\n", fx->Opcode, fx->Resource, fx->Parameter2 );

	switch(fx->Parameter2)
	{
	case 1:
		//this is pst specific
		target->fxqueue.RemoveAllEffects(fx_pst_jumble_curse_ref);
		break;
	default:
		Inventory *inv = &target->inventory;
		int i = target->inventory.GetSlotCount();
		while(i--) {
			//does this slot need unequipping
			if (core->QuerySlotEffects(i) ) {
				if (fx->Resource[0] && strnicmp(inv->GetSlotItem(i)->ItemResRef, fx->Resource,8) ) {
					continue;
				}
				if (!(inv->GetItemFlag(i)&IE_INV_ITEM_CURSED)) {
					continue;
				}
				inv->ChangeItemFlag(i, IE_INV_ITEM_CURSED, BM_NAND);
				if (inv->UnEquipItem(i,true)) {
					CREItem *tmp = inv->RemoveItem(i);
					if(inv->AddSlotItem(tmp,-3)!=ASI_SUCCESS) {
						//if the item couldn't be placed in the inventory, then put it back to the original slot
						inv->SetSlotItem(tmp,i);
						//and drop it in the area. (If there is no area, then the item will stay in the inventory)
						target->DropItem(i,0);
					}
				}
			}
		}
		target->fxqueue.RemoveAllEffects(fx_apply_effect_curse_ref);
	}

	//this is an instant effect
	return FX_NOT_APPLIED;
}

// 0x1b AcidResistanceModifier
int fx_acid_resistance_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_acid_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_RESISTACID );
	return FX_APPLIED;
}

// 0x1c ColdResistanceModifier
int fx_cold_resistance_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cold_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_RESISTCOLD );
	return FX_APPLIED;
}

// 0x1d ElectricityResistanceModifier
int fx_electricity_resistance_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_electricity_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_RESISTELECTRICITY );
	return FX_APPLIED;
}

// 0x1e FireResistanceModifier
int fx_fire_resistance_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_fire_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_RESISTFIRE );
	return FX_APPLIED;
}

// 0x1f MagicDamageResistanceModifier
int fx_magic_damage_resistance_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_magic_damage_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_MAGICDAMAGERESISTANCE );
	return FX_APPLIED;
}

// 0x20 Cure:Death
int fx_cure_dead_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_dead_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//someone should clear the internal flags related to death
	target->Resurrect();
	return FX_NOT_APPLIED;
}

// 0x21 SaveVsDeathModifier
int fx_save_vs_death_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_save_vs_death_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_SAVEVSDEATH );
	return FX_APPLIED;
}

// 0x22 SaveVsWandsModifier
int fx_save_vs_wands_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_save_vs_wands_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_SAVEVSWANDS );
	return FX_APPLIED;
}

// 0x23 SaveVsPolyModifier
int fx_save_vs_poly_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_save_vs_poly_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_SAVEVSPOLY );
	return FX_APPLIED;
}

// 0x24 SaveVsBreathModifier
int fx_save_vs_breath_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_save_vs_breath_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_SAVEVSBREATH );
	return FX_APPLIED;
}

// 0x25 SaveVsSpellsModifier
int fx_save_vs_spell_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_save_vs_spell_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_SAVEVSSPELL );
	return FX_APPLIED;
}

// 0x26 State:Silenced
int fx_set_silenced_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_silenced_state (%2d)\n", fx->Opcode );
	STATE_SET( STATE_SILENCED );
	return FX_APPLIED;
}

static EffectRef fx_animation_stance_ref = {"AnimationStateChange",NULL,-1};

// 0x27 State:Helpless
// this effect sets both bits, but 'awaken' only removes the sleep bit
// FIXME: this is probably a persistent effect
int fx_set_unconscious_state (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_unconscious_state (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );

	if (target->HasSpellState(SS_BLOODRAGE)) {
		return FX_NOT_APPLIED;
	}

	if (fx->FirstApply) {
		Effect *newfx;

		newfx = EffectQueue::CreateEffectCopy(fx, fx_animation_stance_ref, 0, IE_ANI_SLEEP);
		core->ApplyEffect(newfx, target, Owner);

		delete newfx;
	}

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_STATE_SET( STATE_HELPLESS | STATE_SLEEP ); //don't awaken on damage
	} else {
		STATE_SET( STATE_HELPLESS | STATE_SLEEP ); //don't awaken on damage
		if (fx->Parameter2) {
			target->SetSpellState(SS_NOAWAKE);
		}
		target->AddPortraitIcon(PI_SLEEP);
	}
	return FX_PERMANENT;
}

// 0x28 State:Slowed
// this function removes hasted state, or sets slowed state
static EffectRef fx_set_haste_state_ref={"State:Hasted",NULL,-1};

int fx_set_slowed_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_slowed_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	//iwd2 free action or aegis disables this effect
	if (target->HasSpellState(SS_FREEACTION)) return FX_NOT_APPLIED;
	if (target->HasSpellState(SS_AEGIS)) return FX_NOT_APPLIED;

	if (STATE_GET(STATE_HASTED) ) {
		BASE_STATE_CURE( STATE_HASTED );
		target->fxqueue.RemoveAllEffects( fx_set_haste_state_ref );
		target->fxqueue.RemoveAllEffectsWithParam( fx_display_portrait_icon_ref, PI_HASTED );
	} else {
		STATE_SET( STATE_SLOWED );
		target->AddPortraitIcon(PI_SLOWED);
	}
	return FX_PERMANENT;
}

// 0x29 Sparkle
int fx_sparkle (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_sparkle (%2d): Sparkle colour: %d ; Sparkle type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	if (!target) {
		return FX_NOT_APPLIED;
	}

	Map *map = target->GetCurrentArea();
	if (!map) {
		return FX_APPLIED;
	}
	Point p(fx->PosX, fx->PosY);

	//the IE programmers did something unexpected, again, a permutation!
	unsigned int idx;

	if (fx->Parameter1<SPARK_COUNT) {
		idx = spark_color_indices[fx->Parameter1];
	}
	else {
		idx=fx->Parameter1;
	}
	map->Sparkle( idx, fx->Parameter2, p, fx->Parameter3);
	return FX_NOT_APPLIED;
}

// 0x2A WizardSpellSlotsModifier
int fx_bonus_wizard_spells (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_bonus_wizard_spells (%2d): Spell Add: %d ; Spell Level: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
int fx_cure_petrified_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_petrified_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	BASE_STATE_CURE( STATE_PETRIFIED );
	return FX_NOT_APPLIED;
}

// 0x2C StrengthModifier
int fx_strength_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_strength_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	////how strength: value is based on class
	if (fx->Parameter2==3) {
		fx->Parameter1 = core->Roll(1,SpellAbilityDieRoll(target,1),0);
		fx->Parameter2 = 0;
	}

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD( IE_STR );
	} else {
		STAT_MOD( IE_STR );
	}
	return FX_PERMANENT;
}

// 0x2D State:Stun
int power_word_stun_iwd2(Actor *target, Effect *fx)
{
	int hp = STAT_GET(IE_HITPOINTS);
	if (hp>150) return FX_NOT_APPLIED;
	int stuntime;
	if (hp>100) stuntime = core->Roll(1,4,0);
	else if (hp>50) stuntime = core->Roll(2,4,0);
	else stuntime = core->Roll(4,4,0);
	fx->Parameter2 = 0;
	fx->TimingMode = FX_DURATION_INSTANT_LIMITED;
	fx->Duration = stuntime*6*ROUND_SIZE + core->GetGame()->GameTime;
	STATE_SET( STATE_STUNNED );
	target->AddPortraitIcon(PI_STUN);
	return FX_APPLIED;
}

int fx_set_stun_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_stun_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	if ( STATE_GET(STATE_DEAD) ) {
		return FX_NOT_APPLIED;
	}

	if (target->HasSpellState(SS_BLOODRAGE)) {
		return FX_NOT_APPLIED;
	}

	if (fx->Parameter2==2) {
		return power_word_stun_iwd2(target, fx);
	}
	STATE_SET( STATE_STUNNED );
	target->AddPortraitIcon(PI_STUN);
	if (fx->Parameter2==1) {
		target->SetSpellState(SS_AWAKE);
	}
	return FX_APPLIED;
}

// 0x2E Cure:Stun
static EffectRef fx_set_stun_state_ref={"State:Stun",NULL,-1};
static EffectRef fx_hold_creature_no_icon_ref={"State:Hold3",NULL,-1};

int fx_cure_stun_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_stun_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	BASE_STATE_CURE( STATE_STUNNED );
	target->fxqueue.RemoveAllEffects(fx_set_stun_state_ref);
	target->fxqueue.RemoveAllEffects(fx_hold_creature_no_icon_ref);
	target->fxqueue.RemoveAllEffectsWithParam(fx_display_portrait_icon_ref, PI_HELD);
	target->fxqueue.RemoveAllEffectsWithParam(fx_display_portrait_icon_ref, PI_HOPELESS);
	return FX_NOT_APPLIED;
}

// 0x2F Cure:Invisible
// 0x74 Cure:Invisible2
static EffectRef fx_set_invisible_state_ref={"State:Invisible",NULL,-1};

int fx_cure_invisible_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_invisible_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	if (!STATE_GET(STATE_NONDET)) {
		BASE_STATE_CURE( STATE_INVISIBLE | STATE_INVIS2 );
		target->fxqueue.RemoveAllEffects(fx_set_invisible_state_ref);
	}
	return FX_NOT_APPLIED;
}

// 0x30 Cure:Silence
static EffectRef fx_set_silenced_state_ref={"State:Silence",NULL,-1};

int fx_cure_silenced_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_silenced_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	BASE_STATE_CURE( STATE_SILENCED );
	target->fxqueue.RemoveAllEffects(fx_set_silenced_state_ref);
	return FX_NOT_APPLIED;
}

// 0x31 WisdomModifier
int fx_wisdom_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_wisdom_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD( IE_WIS );
	} else {
		STAT_MOD( IE_WIS );
	}
	return FX_PERMANENT;
}

// 0x32 Color:BriefRGB
int fx_brief_rgb (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_brief_rgb (%2d): RGB: %d, Location and speed: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	int speed = (fx->Parameter2 >> 16) & 0xff;
	target->SetColorMod(0xff, RGBModifier::ADD, speed,
			fx->Parameter1 >> 8, fx->Parameter1 >> 16,
			fx->Parameter1 >> 24, 0);

	return FX_NOT_APPLIED;
}

// 0x33 Color:DarkenRGB
int fx_darken_rgb (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_darken_rgb (%2d): RGB: %d, Location and speed: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	EffectQueue::HackColorEffects(target, fx);
	ieDword location = fx->Parameter2 & 0xff;
	target->SetColorMod(location, RGBModifier::TINT, -1, fx->Parameter1 >> 8,
			fx->Parameter1 >> 16, fx->Parameter1 >> 24);
	return FX_APPLIED;
}

// 0x34 Color:GlowRGB
int fx_glow_rgb (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_glow_rgb (%2d): RGB: %d, Location and speed: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	EffectQueue::HackColorEffects(target, fx);
	ieDword location = fx->Parameter2 & 0xff;
	target->SetColorMod(location, RGBModifier::BRIGHTEN, -1,
			fx->Parameter1 >> 8, fx->Parameter1 >> 16,
			fx->Parameter1 >> 24);

	return FX_APPLIED;
}

// 0x35 AnimationIDModifier
static EffectRef fx_animation_id_modifier_ref={"AnimationIDModifier",NULL,-1};

int fx_animation_id_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_animation_id_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	switch (fx->Parameter2) {
	case 0: //non permanent animation change
	default:
		STAT_MOD( IE_ANIMATION_ID );
		return FX_APPLIED;
	case 1: //remove any non permanent change
		target->fxqueue.RemoveAllEffects(fx_animation_id_modifier_ref);
		//return FX_NOT_APPLIED;
		//intentionally passing through (perma change removes previous changes)
	case 2: //permanent animation id change
		target->SetBaseNoPCF(IE_ANIMATION_ID, fx->Parameter1);
		return FX_NOT_APPLIED;
	}
}

// 0x36 ToHitModifier
int fx_to_hit_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_to_hit_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_TOHIT );
	return FX_APPLIED;
}

// 0x37 KillCreatureType
static EffectRef fx_death_ref={"Death",NULL,-1};

int fx_kill_creature_type (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_kill_creature_type (%2d): Value: %d, IDS: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
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
//switch good to evil and evil to good
//also switch chaotic to lawful and vice versa
//gemrb extension: param2 actually controls which parts should be reversed
// 0 - switch both (as original)
// 1 - switch good and evil
// 2 - switch lawful and chaotic

static int al_switch_both[16]={0,0x33,0x32,0x31,0,0x23,0x22,0x21,0,0x13,0x12,0x11};
static int al_switch_law[16]={0,0x31,0x32,0x33,0,0x21,0x22,0x23,0,0x11,0x12,0x13};
static int al_switch_good[16]={0,0x13,0x12,0x11,0,0x23,0x22,0x21,0,0x33,0x32,0x31};
int fx_alignment_invert (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_alignment_invert (%2d)\n", fx->Opcode );
	register ieDword newalign = target->GetStat( IE_ALIGNMENT );
	//compress the values. GNE is the first 2 bits originally
	//LNC is the 4/5. bits.
	newalign = (newalign & AL_GE_MASK) | ((newalign & AL_LC_MASK)>>2);
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
int fx_alignment_change (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_alignment_change (%2d): Value: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_SET( IE_ALIGNMENT, fx->Parameter2 );
	return FX_APPLIED;
}

//0x3A DispelEffects
int fx_dispel_effects (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_dispel_effects (%2d): Value: %d, IDS: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	ieDword level;

	switch (fx->Parameter2) {
	case 0:
	default:
		level = 0xffffffff;
		break;
	case 1:
		//same level: 50% success, each diff modifies it by 5%
		level = core->Roll(1,20,fx->Power-10);
		if (level>=0x80000000) level = 0;
		break;
	case 2:
		//same level: 50% success, each diff modifies it by 5%
		level = core->Roll(1,20,fx->Parameter1-10);
		if (level>=0x80000000) level = 0;
		break;
	}
	//if signed would it be negative?
	target->fxqueue.RemoveLevelEffects(level, RL_DISPELLABLE, 0);
	return FX_NOT_APPLIED;
}

// 0x3B StealthModifier
int fx_stealth_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_stealth_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_STEALTH );
	return FX_APPLIED;
}

// 0x3C MiscastMagicModifier
int fx_miscast_magic_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_miscast_magic_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	switch (fx->Parameter2) {
	case 3:
		STAT_SET( IE_DEADMAGIC, 1);
	case 0:
		STAT_SET( IE_SPELLFAILUREMAGE, fx->Parameter1);
		break;
	case 4:
		STAT_SET( IE_DEADMAGIC, 1);
	case 1:
		STAT_SET( IE_SPELLFAILUREPRIEST, fx->Parameter1);
		break;
	case 5:
		STAT_SET( IE_DEADMAGIC, 1);
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
int fx_alchemy_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_alchemy_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
int fx_bonus_priest_spells (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_bonus_priest_spells (%2d): Spell Add: %d ; Spell Level: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
int fx_set_infravision_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_infravision_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_SET( STATE_INFRA );
	return FX_APPLIED;
}

// 0x40 Cure:Infravision
static EffectRef fx_set_infravision_state_ref={"State:Infravision",NULL,-1};

int fx_cure_infravision_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_infravision_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	BASE_STATE_CURE( STATE_INFRA );
	target->fxqueue.RemoveAllEffects(fx_set_infravision_state_ref);
	return FX_NOT_APPLIED;
}

//0x41 State:Blur
int fx_set_blur_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_blur_state (%2d)\n", fx->Opcode );
	if (STATE_GET( STATE_DEAD) ) {
		return FX_NOT_APPLIED;
	}
	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_STATE_SET( STATE_BLUR );
	} else {
		STATE_SET( STATE_BLUR );
	}
	//iwd2 specific
	if (enhanced_effects) {
		target->AddPortraitIcon(PI_BLUR);
	}
	return FX_PERMANENT;
}

// 0x42 TransparencyModifier
int fx_transparency_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_transparency_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	//maybe this needs some timing
	switch (fx->Parameter2) {
	case 1: //fade in
		if (fx->Parameter1<255) {
			if (core->GetGame()->GameTime%2) {
				fx->Parameter1++;
			}
		}
		break;
	case 2://fade out
		if (fx->Parameter1) {
			if (core->GetGame()->GameTime%2) {
				fx->Parameter1--;
			}
		}
		break;
	}
	STAT_MOD( IE_TRANSLUCENT );
	return FX_APPLIED;
}

static int eamods[]={EAM_ALLY,EAM_ALLY,EAM_DEFAULT,EAM_ALLY,EAM_DEFAULT,EAM_ENEMY,EAM_ALLY};
// 0x43 SummonCreature
int fx_summon_creature (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_summon_creature (%2d): ResRef:%s Anim:%s Type: %d\n", fx->Opcode, fx->Resource, fx->Resource2, fx->Parameter2 );
	if (!target) {
		return FX_NOT_APPLIED;
	}

	if (!target->GetCurrentArea()) {
		return FX_APPLIED;
	}

	//summon creature (resource), play vvc (resource2)
	//creature's lastsummoner is Owner
	//creature's target is target
	//position of appearance is target's pos
	int eamod = -1;
	if (fx->Parameter2<6){
		eamod = eamods[fx->Parameter2];
	}
	core->SummonCreature(fx->Resource, fx->Resource2, Owner, target, target->Pos, eamod, 0);
	return FX_NOT_APPLIED;
}

//0x44 UnsummonCreature
int fx_unsummon_creature (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_unsummon_creature (%2d)\n", fx->Opcode );

	//to be compatible with the original engine, unsummon doesn't work with PC's
	//but it works on anything else
	if (!target->InParty) {
		//TODO:animation
		target->DestroySelf();
	}
	return FX_NOT_APPLIED;
}

// 0x45 State:Nondetection
int fx_set_nondetection_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_nondetection_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_SET( STATE_NONDET );
	return FX_APPLIED;
}

// 0x46 Cure:Nondetection
static EffectRef fx_set_nondetection_state_ref={"State:Nondetection",NULL,-1};

int fx_cure_nondetection_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_nondetection_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	BASE_STATE_CURE( STATE_NONDET );
	target->fxqueue.RemoveAllEffects(fx_set_nondetection_state_ref);
	return FX_NOT_APPLIED;
}

//0x47 SexModifier
int fx_sex_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_sex_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
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

//0x48 AIIdentifierModifier
int fx_ids_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_ids_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	switch (fx->Parameter2) {
	case 0:
		STAT_SET(IE_EA, fx->Parameter1);
		break;
	case 1:
		STAT_SET(IE_GENERAL, fx->Parameter1);
		break;
	case 2:
		STAT_SET(IE_RACE, fx->Parameter1);
		break;
	case 3:
		STAT_SET(IE_CLASS, fx->Parameter1);
		break;
	case 4:
		STAT_SET(IE_SPECIFIC, fx->Parameter1);
		break;
	case 5:
		STAT_SET(IE_SEX, fx->Parameter1);
		break;
	case 6:
		STAT_SET(IE_ALIGNMENT, fx->Parameter1);
		break;
	default:
		return FX_NOT_APPLIED;
	}
	//not sure, need a check if this action could be permanent
	return FX_APPLIED;
}

// 0x49 DamageBonusModifier
int fx_damage_bonus_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_damage_bonus_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_DAMAGEBONUS );
	return FX_APPLIED;
}

// 0x4a State:Blind
int fx_set_blind_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_blind_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_SET( STATE_BLIND );
	return FX_APPLIED;
}

// 0x4b Cure:Blind
static EffectRef fx_set_blind_state_ref={"State:Blind",NULL,-1};

int fx_cure_blind_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_blind_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	BASE_STATE_CURE( STATE_BLIND );
	target->fxqueue.RemoveAllEffects(fx_set_blind_state_ref);
	return FX_NOT_APPLIED;
}

// 0x4c State:Feeblemind
int fx_set_feebleminded_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_feebleminded_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_SET( STATE_FEEBLE );
	STAT_SET( IE_INT, 3);
	if (enhanced_effects) {
		target->AddPortraitIcon(PI_FEEBLEMIND);
	}
	return FX_APPLIED;
}

// 0x4d Cure:Feeblemind
static EffectRef fx_set_feebleminded_state_ref={"State:Feeblemind",NULL,-1};

int fx_cure_feebleminded_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_feebleminded_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	BASE_STATE_CURE( STATE_FEEBLE );
	target->fxqueue.RemoveAllEffects(fx_set_feebleminded_state_ref);
	target->fxqueue.RemoveAllEffectsWithParam(fx_display_portrait_icon_ref, PI_FEEBLEMIND);
	return FX_NOT_APPLIED;
}

//0x4e State:Diseased
int fx_set_diseased_state (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_diseased_state (%2d): Damage: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//STATE_SET( STATE_DISEASED ); //no this we don't want

	//setting damage to 0 because not all types do damage
	ieDword damage = 0;

	switch(fx->Parameter2) {
	case RPD_PERCENT:
		damage = target->GetStat(IE_MAXHITPOINTS) * fx->Parameter1 / 100;
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
	case RPD_STR: //strength
		STAT_ADD(IE_STR, fx->Parameter1);
		break;
	case RPD_DEX: //dex
		STAT_ADD(IE_DEX, fx->Parameter1);
		break;
	case RPD_CON: //con
		STAT_ADD(IE_CON, fx->Parameter1);
		break;
	case RPD_INT: //int
		STAT_ADD(IE_INT, fx->Parameter1);
		break;
	case RPD_WIS: //wis
		STAT_ADD(IE_WIS, fx->Parameter1);
		break;
	case RPD_CHA: //cha
		STAT_ADD(IE_CHR, fx->Parameter1);
		break;
	case RPD_SLOW: //slow
		break;
	default:
		damage = 1;
		break;
	}
	//percent
	if (damage) {
		target->Damage(damage, DAMAGE_POISON, Owner);
	}
	return FX_APPLIED;
}


//0x4f Cure:Disease
static EffectRef fx_diseased_state_ref={"State:Disease",NULL,-1};

int fx_cure_diseased_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_diseased_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//STATE_CURE( STATE_DISEASED ); //the bit flagged as disease is actually the active state. so this is even more unlikely to be used as advertised
	target->fxqueue.RemoveAllEffects( fx_diseased_state_ref ); //this is what actually happens in bg2
	return FX_NOT_APPLIED;
}

// 0x50 State:Deafness
// gemrb extension: modifiable amount
// none of the engines care about stacking
int fx_set_deaf_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_deaf_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	//gemrb fix
	if (target->SetSpellState(SS_DEAF)) return FX_APPLIED;

	if (!fx->Parameter1) {
		fx->Parameter1 = 50;
	}
	STAT_ADD(IE_SPELLFAILUREMAGE, fx->Parameter1);
	if (!fx->Parameter2) {
		fx->Parameter1 = 50;
	}
	STAT_ADD(IE_SPELLFAILUREPRIEST, fx->Parameter2);
	EXTSTATE_SET(EXTSTATE_DEAF); //iwd1/how needs this
	if (enhanced_effects) {
		target->AddPortraitIcon(PI_DEAFNESS);
	}
	return FX_APPLIED;
}

int fx_set_deaf_state_iwd2 (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_deaf_state_iwd2 (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	//gemrb fix
	if (target->SetSpellState(SS_DEAF)) return FX_APPLIED;

	if (!fx->Parameter1) {
		//this is a bad hack
		fx->Parameter1 = 20;
	}
	STAT_ADD(IE_SPELLFAILUREMAGE, fx->Parameter1);
	if (!fx->Parameter2) {
		fx->Parameter1 = 20;
	}
	STAT_ADD(IE_SPELLFAILUREPRIEST, fx->Parameter2);
	EXTSTATE_SET(EXTSTATE_DEAF); //iwd1/how needs this
	target->AddPortraitIcon(PI_DEAFNESS); //iwd2 specific
	return FX_APPLIED;
}

// 0x51 Cure:Deafness
static EffectRef fx_deaf_state_ref={"State:Deafness",NULL,-1};
static EffectRef fx_deaf_state_iwd2_ref={"State:DeafnessIWD2",NULL,-1};

//removes the deafness effect
int fx_cure_deaf_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_deaf_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->fxqueue.RemoveAllEffects(fx_deaf_state_ref);
	target->fxqueue.RemoveAllEffects(fx_deaf_state_iwd2_ref);
	return FX_NOT_APPLIED;
}

//0x52 SetAIScript
int fx_set_ai_script (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_ai_state (%2d): Resource: %s, Type: %d\n", fx->Opcode, fx->Resource, fx->Parameter2 );
	target->SetScript (fx->Resource, fx->Parameter2);
	return FX_NOT_APPLIED;
}

//0x53 Protection:Projectile
int fx_protection_from_projectile (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_protection_from_projectile (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_BIT_OR( IE_IMMUNITY, IMM_PROJECTILE);
	return FX_APPLIED;
}

// 0x54 MagicalFireResistanceModifier
int fx_magical_fire_resistance_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_magical_fire_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_RESISTMAGICFIRE );
	return FX_APPLIED;
}

// 0x55 MagicalColdResistanceModifier
int fx_magical_cold_resistance_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_magical_cold_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_RESISTMAGICCOLD );
	return FX_APPLIED;
}

// 0x56 SlashingResistanceModifier
int fx_slashing_resistance_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_slashing_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_RESISTSLASHING );
	return FX_APPLIED;
}

// 0x57 CrushingResistanceModifier
int fx_crushing_resistance_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_crushing_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_RESISTCRUSHING );
	return FX_APPLIED;
}

// 0x58 PiercingResistanceModifier
int fx_piercing_resistance_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_piercing_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_RESISTPIERCING );
	return FX_APPLIED;
}

// 0x59 MissilesResistanceModifier
int fx_missiles_resistance_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_missiles_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_RESISTMISSILE );
	return FX_APPLIED;
}

// 0x5A OpenLocksModifier
int fx_open_locks_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_open_locks_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_LOCKPICKING );
	return FX_APPLIED;
}

// 0x5B FindTrapsModifier
int fx_find_traps_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_find_traps_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_TRAPS );
	return FX_APPLIED;
}

// 0x5C PickPocketsModifier
int fx_pick_pockets_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_pick_pockets_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_PICKPOCKET );
	return FX_APPLIED;
}

// 0x5D FatigueModifier
int fx_fatigue_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_fatigue_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_FATIGUE );
	return FX_APPLIED;
}

// 0x5E IntoxicationModifier
int fx_intoxication_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_intoxication_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_INTOXICATION );
	return FX_APPLIED;
}

// 0x5F TrackingModifier
int fx_tracking_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_tracking_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_TRACKING );
	return FX_APPLIED;
}

// 0x60 LevelModifier
int fx_level_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_level_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_LEVEL );
	return FX_APPLIED;
}

// 0x61 StrengthBonusModifier
int fx_strength_bonus_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_strength_bonus_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_STREXTRA );
	return FX_APPLIED;
}

// 0x62 State:Regenerating
int fx_set_regenerating_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_regenerating_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	int damage;
	int tmp = fx->Parameter1;
	ieDword gameTime = core->GetGame()->GameTime;
	ieDword nextHeal;

	if (!fx->Parameter3) {
		nextHeal = gameTime;
	} else {
		nextHeal = fx->Parameter3;
	}

	if (nextHeal>gameTime) return FX_APPLIED;

	switch(fx->Parameter2) {
	case RPD_PERCENT:
		damage = target->GetStat(IE_MAXHITPOINTS) * fx->Parameter1 / 100;
		break;
	case RPD_ROUNDS:
		tmp *= 6;
		goto seconds;
	case RPD_TURNS:
		tmp *= 30;
		//fallthrough
	case RPD_SECONDS:           //restore param3 hp every param1 seconds
seconds:
		fx->Parameter3 = nextHeal+tmp;
		damage = 1;
		break;
	case RPD_POINTS:
		damage = fx->Parameter1;
		fx->Parameter3++;
		break;
	default:
		fx->Parameter3++;
		damage = 1;
		break;
	}

	//This should take care of the change of the modified stat
	//So there is no need to do anything else here other than increasing
	//the base current hp
	target->NewBase(IE_HITPOINTS, damage, MOD_ADDITIVE);
	return FX_APPLIED;
}
// 0x63 SpellDurationModifier
int fx_spell_duration_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_spell_duration_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
int fx_generic_effect (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_generic_effect (%2d): Param1: %d, Param2: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	return FX_APPLIED;
}

//0x65 Protection:Opcode
int fx_protection_opcode (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_protection_opcode (%2d): Opcode: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_BIT_OR(IE_IMMUNITY, IMM_OPCODE);
	return FX_APPLIED;
}

//0x66 Protection:SpellLevel
int fx_protection_spelllevel (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_protection_spelllevel (%2d) Level: %d\n", fx->Opcode, fx->Parameter1);

	int value = fx->Parameter1;
	if (value<9) {
		STAT_BIT_OR(IE_MINORGLOBE, 1<<value);
		STAT_BIT_OR(IE_IMMUNITY, IMM_LEVEL);
		return FX_APPLIED;
	}
	return FX_NOT_APPLIED;
}

//0x67 ChangeName
int fx_change_name (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_change_name_modifier (%2d): StrRef: %d\n", fx->Opcode, fx->Parameter1 );
	target->SetText(fx->Parameter1, 0);
	return FX_NOT_APPLIED;
}

//0x68 ExperienceModifier
int fx_experience_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_experience_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//i believe this has mode too
	target->AddExperience (fx->Parameter1);
	return FX_NOT_APPLIED;
}

//0x69 GoldModifier
//in BG2 this effect subtracts gold when type is MOD_ADDITIVE
//no one uses it, though. To keep the function, the default branch will do the subtraction
int fx_gold_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_gold_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	if (!target->InParty) {
		STAT_MOD( IE_GOLD );
		return FX_NOT_APPLIED;
	}
	ieDword gold;
	Game *game = core->GetGame();
	//for party members, the gold is stored in the game object
	switch( fx->Parameter2) {
		case MOD_ADDITIVE:
			gold = fx->Parameter1;
			break;
		case MOD_ABSOLUTE:
			gold = fx->Parameter1-game->PartyGold;
			break;
		case MOD_PERCENT:
			gold = game->PartyGold*fx->Parameter1/100-game->PartyGold;
			break;
		default:
			gold = (ieDword) -fx->Parameter1;
			break;
	}
	game->AddGold (gold);
	return FX_NOT_APPLIED;
}

//0x6a MoraleBreakModifier
int fx_morale_break_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_morale_break_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD(IE_MORALEBREAK);
	return FX_PERMANENT; //permanent morale break doesn't stick
}

//0x6b PortraitChange
int fx_portrait_change (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_portrait_change (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	target->SetPortrait( fx->Resource, fx->Parameter2);
	return FX_NOT_APPLIED;
}

//0x6c ReputationModifier
int fx_reputation_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_reputation_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD(IE_REPUTATION);
	return FX_NOT_APPLIED; //needs testing
}

//0x6d --> see later

//0x6e
//retreat_from (unknown)
int fx_retreat_from (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_retreat_from (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	return FX_NOT_APPLIED;
}

//0x6f Item:CreateMagic

static EffectRef fx_remove_item_ref={"Item:Remove",NULL,-1};

int fx_create_magic_item (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	//charge count is incorrect
	target->inventory.SetSlotItemRes(fx->Resource, target->inventory.GetMagicSlot(),fx->Parameter1,fx->Parameter3,fx->Parameter4);
	//equip the weapon
	target->inventory.SetEquippedSlot(target->inventory.GetMagicSlot()-target->inventory.GetWeaponSlot());
	if (fx->TimingMode==FX_DURATION_INSTANT_LIMITED) {
//if this effect has expiration, then it will remain as a remove_item
//on the effect queue, inheriting all the parameters
		fx->Opcode=EffectQueue::ResolveEffect(fx_remove_item_ref);
		fx->TimingMode=FX_DURATION_DELAY_PERMANENT;
		return FX_APPLIED;
	}
	return FX_NOT_APPLIED;
}

//0x70 Item:Remove
int fx_remove_item (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	//will destroy the first item
	if (target->inventory.DestroyItem(fx->Resource,0,1)) {
		target->ReinitQuickSlots();
	}
	return FX_NOT_APPLIED;
}

//0x71 Item:Equip
int fx_equip_item (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	int eff = core->QuerySlotEffects( fx->Parameter2 );
	switch(eff) {
	case SLOT_EFFECT_NONE:
	case SLOT_EFFECT_MELEE:
		target->inventory.SetEquippedSlot( fx->Parameter2 );
		break;
	default:
		target->inventory.EquipItem( fx->Parameter2 );
		break;
	}
	target->ReinitQuickSlots();
	return FX_NOT_APPLIED;
}

//0x72 Dither
int fx_dither (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_dither (%2d): Value: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//dithers target (not working in original IE)
	return FX_NOT_APPLIED;
}

//0x73 DetectAlignment
//gemrb extension: chaotic/lawful detection
int fx_detect_alignment (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	ieDword msk;
	ieDword stat;

	if (fx->Parameter2<3) {
		//0,1,2 -> 1,2,3
		msk = fx->Parameter2+1;
		stat = target->GetStat(IE_ALIGNMENT)&AL_GE_MASK;
	}
	else {
		//3,4,5 -> 0x10, 0x20, 0x30
		msk = (fx->Parameter2-2)<<4;
		stat = target->GetStat(IE_ALIGNMENT)&AL_LC_MASK;
	}
	if (stat != msk) return FX_NOT_APPLIED;

	ieDword color = fx->Parameter1;
	switch (msk) {
	case AL_EVIL:
		if (!color) color = 0xff0000;
		core->DisplayConstantStringName(STR_EVIL, color, target);
		//glow red
		target->SetColorMod(0xff, RGBModifier::ADD, 30, 0xff, 0, 0, 0);
		break;
	case AL_GOOD:
		if (!color) color = 0xff00;
		core->DisplayConstantStringName(STR_GOOD, color, target);
		//glow green
		target->SetColorMod(0xff, RGBModifier::ADD, 30, 0, 0xff, 0, 0);
		break;
	case AL_GE_NEUTRAL:
		if (!color) color = 0xff;
		core->DisplayConstantStringName(STR_GE_NEUTRAL, color, target);
		//glow blue
		target->SetColorMod(0xff, RGBModifier::ADD, 30, 0, 0, 0xff, 0);
		break;
	case AL_CHAOTIC:
		if (!color) color = 0xff00ff;
		core->DisplayConstantStringName(STR_CHAOTIC, color, target);
		//glow purple
		target->SetColorMod(0xff, RGBModifier::ADD, 30, 0xff, 0, 0xff, 0);
		break;
	case AL_LAWFUL:
		if (!color) color = 0xffffff;
		core->DisplayConstantStringName(STR_LAWFUL, color, target);
		//glow white
		target->SetColorMod(0xff, RGBModifier::ADD, 30, 0xff, 0xff, 0xff, 0);
		break;
	case AL_LC_NEUTRAL:
		if (!color) color = 0xff;
		core->DisplayConstantStringName(STR_LC_NEUTRAL, color, target);
		//glow blue
		target->SetColorMod(0xff, RGBModifier::ADD, 30, 0, 0, 0xff, 0);
		break;
	}
	return FX_NOT_APPLIED;
}

// 0x74 Cure:Invisible2 (see 0x2f)

// 0x75 Reveal:Area
// 0 reveal whole area
// 1 reveal area in pattern
int fx_reveal_area (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_reveal_area (%2d): Value: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	Map *map = target->GetCurrentArea();
	if (!map) {
		return FX_APPLIED;
	}

	if (fx->Parameter2) {
		map->Explore(fx->Parameter1);
	} else {
		map->Explore(-1);
	}
	return FX_NOT_APPLIED;
}

// 0x76 Reveal:Creatures
int fx_reveal_creatures (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_reveal_creatures (%2d): Value: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//reveals creatures (not working in original IE)
	return FX_NOT_APPLIED;
}

// 0x77 MirrorImage
static EffectRef fx_mirror_image_modifier_ref={"MirrorImageModifier",NULL,-1};

int fx_mirror_image (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_mirror_image (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	ieDword images;

	if (fx->Parameter2) {
		images = 1; //reflection
	}
	else {
		images = core->Roll(1, fx->Parameter1, 0); //mirror image
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
int fx_immune_to_weapon (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_immune_to_weapon (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	if (!fx->FirstApply) return FX_APPLIED;

	int level;
	ieDword mask, value;

	level = -1;
	mask = 0;
	value = 0;
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
	case 8: //all twohanded
		value = IE_INV_ITEM_CURSED;
		//fallthrough
	case 9: //all not twohanded
		mask = IE_INV_ITEM_CURSED;
		break;
	case 10: //all twohanded
		value = IE_INV_ITEM_COLDIRON;
		//fallthrough
	case 11: //all not twohanded
		mask = IE_INV_ITEM_COLDIRON;
		break;
	case 12:
		mask = fx->Parameter1;
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
int fx_visual_animation_effect (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
//this is an unknown effect
	if (0) printf( "fx_visual_animation_effect (%2d)\n", fx->Opcode );
	return FX_NOT_APPLIED;
}

// 0x7a Item:CreateInventory
static EffectRef fx_remove_inventory_item_ref={"Item:RemoveInventory",NULL,-1};

int fx_create_inventory_item (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_create_inventory_item (%2d)\n", fx->Opcode );
	target->inventory.SetSlotItemRes( fx->Resource, -1, fx->Parameter1, fx->Parameter3, fx->Parameter4 );
	if (fx->TimingMode==FX_DURATION_INSTANT_LIMITED) {
//if this effect has expiration, then it will remain as a remove_item
//on the effect queue, inheriting all the parameters
		fx->Opcode=EffectQueue::ResolveEffect(fx_remove_inventory_item_ref);
		fx->TimingMode=FX_DURATION_DELAY_PERMANENT;
		return FX_APPLIED;
	}
	return FX_NOT_APPLIED;
}

// 0x7b Item:RemoveInventory
int fx_remove_inventory_item (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_remove_inventory_item (%2d)\n", fx->Opcode );
	//FIXME: now equipped items are only wielded weapons
	//why would it not let equipped items to be destructed?
	target->inventory.DestroyItem(fx->Resource,IE_INV_ITEM_EQUIPPED,1);
	return FX_NOT_APPLIED;
}

// 0x7c DimensionDoor
// iwd2 has several options
int fx_dimension_door (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_dimension_door (%2d) Type:%d\n", fx->Opcode, fx->Parameter2 );
	Point p;

	switch(fx->Parameter2)
	{
	case 0: //target to point
		p.x=fx->PosX;
		p.y=fx->PosY;
		break;
	case 1: //owner to target
		p=target->Pos;
		target = Owner;
		break;
	case 2: //target to saved location
		p.x=STAT_GET(IE_SAVEDXPOS);
		p.x=STAT_GET(IE_SAVEDYPOS);
		target->SetOrientation(STAT_GET(IE_SAVEDFACE), false);
		break;
	case 3: //owner swapped with target
		p=target->Pos;
		target->SetPosition(Owner->Pos, true, 0);
		target = Owner;
		break;
	}
	target->SetPosition(p, true, 0 );
	return FX_NOT_APPLIED;
}

// 0x7d Unlock
int fx_knock (Actor* Owner, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_knock (%2d) [%d.%d]\n", fx->Opcode, fx->PosX, fx->PosY );
	Map *map = Owner->GetCurrentArea();
	Point p(fx->PosX, fx->PosY);
	Door *door = map->TMap->GetDoorByPosition(p);
	if (door) {
		if (door->LockDifficulty<100) {
			door->SetDoorLocked(false, true);
		}
		return FX_NOT_APPLIED;
	}
	Container *container = map->TMap->GetContainerByPosition(p);
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
int fx_movement_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_movement_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	//iwd2 freeaction disables only 0xb0, who cares
	if (target->HasSpellState(SS_FREEACTION)) return FX_NOT_APPLIED;
	//iwd2 aegis doesn't protect against grease/acid fog slowness, but that is
	//definitely a bug
	if (target->HasSpellState(SS_AEGIS)) return FX_NOT_APPLIED;

	STAT_MOD(IE_MOVEMENTRATE);
	return FX_APPLIED;
}

#define FX_MS 10
static const ieResRef monster_summoning_2da[FX_MS]={"MONSUM01","MONSUM02","MONSUM03",
 "ANISUM01","ANISUM02", "MONSUM01", "MONSUM02","MONSUM03","ANISUM01","ANISUM02"};

// 0x7f MonsterSummoning
int fx_monster_summoning (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_monster_summoning (%2d): Number: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//check the summoning limit?
	if (!target) {
		return FX_NOT_APPLIED;
	}

	if (!target->GetCurrentArea()) {
		return FX_APPLIED;
	}

	//get monster resref from 2da determined by fx->Resource or fx->Parameter2
	ieResRef monster;
	ieResRef hit;
	ieResRef areahit;
	int level = fx->Parameter1;

	if (fx->Parameter2>=FX_MS) {
		strnuprcpy(monster,fx->Resource,8);
		strnuprcpy(hit,fx->Resource2,8);
		strnuprcpy(areahit,fx->Resource3,8);
	} else {
		core->GetResRefFrom2DA(monster_summoning_2da[fx->Parameter2], monster, hit, areahit);

		if (!hit[0]) {
			strnuprcpy(hit,fx->Resource2,8);
		}
		if (!areahit[0]) {
			strnuprcpy(areahit,fx->Resource3,8);
		}
	}

	//the monster should appear near the effect position
	Point p(fx->PosX, fx->PosY);
	core->SummonCreature(monster, hit, Owner, target, p, fx->Parameter2/5, level);
	return FX_NOT_APPLIED;
}

// 0x80 State:Confused
int fx_set_confused_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_confused_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	if (target->HasSpellState(SS_BLOODRAGE)) {
		return FX_NOT_APPLIED;
	}

	if (fx->TimingMode==FX_DURATION_INSTANT_LIMITED) {
		BASE_STATE_SET( STATE_CONFUSED );
	} else {
		STATE_SET( STATE_CONFUSED );
	}
	//NOTE: iwd2 is also unable to display the portrait icon
	//for permanent confusion
	if (enhanced_effects) {
		target->AddPortraitIcon(PI_CONFUSED);
	}
	return FX_PERMANENT;
}

// 0x81 AidNonCumulative
int fx_set_aid_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_aid_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	if (!fx->Parameter2) {
		fx->Parameter2=core->Roll(fx->Parameter1,8,0);
	}
	if (STATE_GET (STATE_AID) ) //aid is non cumulative
		return FX_NOT_APPLIED;
	STATE_SET( STATE_AID );
	target->SetSpellState(SS_AID);
	STAT_ADD( IE_MAXHITPOINTS, fx->Parameter2);
	STAT_ADD( IE_HITPOINTS, fx->Parameter1);
	STAT_ADD( IE_SAVEVSDEATH, fx->Parameter1);
	STAT_ADD( IE_SAVEVSWANDS, fx->Parameter1);
	STAT_ADD( IE_SAVEVSPOLY, fx->Parameter1);
	STAT_ADD( IE_SAVEVSBREATH, fx->Parameter1);
	STAT_ADD( IE_SAVEVSSPELL, fx->Parameter1);
	//bless effect too?
	STAT_ADD( IE_TOHIT, fx->Parameter1);
	STAT_ADD( IE_MORALEBREAK, fx->Parameter1);
	if (enhanced_effects) {
		target->AddPortraitIcon(PI_AID);
		target->SetColorMod(0xff, RGBModifier::ADD, 30, 50, 50, 50);
	}
	return FX_APPLIED;
}

// 0x82 BlessNonCumulative

static EffectRef fx_bane_ref={"Bane",NULL,-1};

int fx_set_bless_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_bless_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	if (STATE_GET (STATE_BLESS) ) //bless is non cumulative
		return FX_NOT_APPLIED;

	//do this once
	target->fxqueue.RemoveAllEffects(fx_bane_ref);

	STATE_SET( STATE_BLESS );
	target->SetSpellState(SS_BLESS);
	STAT_ADD( IE_TOHIT, fx->Parameter1);
	STAT_ADD( IE_MORALEBREAK, fx->Parameter1);
	if (enhanced_effects) {
		target->AddPortraitIcon(PI_BLESS);
		target->SetColorMod(0xff, RGBModifier::ADD, 30, 0xc0, 0x80, 0);
	}
	return FX_APPLIED;
}
// 0x83 ChantNonCumulative
int fx_set_chant_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_chant_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	if (STATE_GET (STATE_CHANT) ) //chant is non cumulative
		return FX_NOT_APPLIED;
	STATE_SET( STATE_CHANT );
	target->SetSpellState(SS_GOODCHANT);
	STAT_ADD( IE_LUCK, fx->Parameter1 );
	return FX_APPLIED;
}

// 0x84 HolyNonCumulative
int fx_set_holy_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_holy_state (%2d): Modifier: %d\n", fx->Opcode, fx->Parameter1 );

	if (STATE_GET (STATE_HOLY) ) //holy power is non cumulative
		return FX_NOT_APPLIED;
	STATE_SET( STATE_HOLY );
	//setting the spell state to be compatible with iwd2
	target->SetSpellState(SS_HOLYMIGHT);
	STAT_ADD( IE_STR, fx->Parameter1);
	STAT_ADD( IE_CON, fx->Parameter1);
	STAT_ADD( IE_DEX, fx->Parameter1);
	if (enhanced_effects) {
		target->AddPortraitIcon(PI_HOLY);
		target->SetColorMod(0xff, RGBModifier::ADD, 30, 0x80, 0x80, 0x80);
	}
	return FX_APPLIED;
}

// 0x85 LuckNonCumulative
int fx_luck_non_cumulative (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_luck_non_cumulative (%2d): Modifier: %d\n", fx->Opcode, fx->Parameter1);

	if (STATE_GET (STATE_LUCK) ) //this luck is non cumulative
		return FX_NOT_APPLIED;
	STATE_SET( STATE_LUCK );
	target->SetSpellState(SS_LUCK);
	STAT_ADD( IE_LUCK, fx->Parameter1 );
	return FX_APPLIED;
}

// 0x85 LuckCumulative (iwd2)
int fx_luck_cumulative (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_luck_cumulative (%2d): Modifier: %d\n", fx->Opcode, fx->Parameter1);

	target->SetSpellState(SS_LUCK);
	STAT_ADD( IE_LUCK, fx->Parameter1 );
	return FX_APPLIED;
}

// 0x86 State:Petrification
int fx_set_petrified_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_petrified_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	BASE_STATE_SET( STATE_PETRIFIED );
	return FX_NOT_APPLIED; //permanent effect
}

// 0x87 Polymorph
static EffectRef fx_polymorph_ref={"Polymorph",NULL,-1};

void CopyPolyMorphStats(Actor *source, Actor *target)
{
	int i;

	if(!polymorph_stats) {
		AutoTable tab("polystat");
		if (!tab) {
			spell_abilities = (int *) malloc(0);			
			polystatcount=0;
			return;
		}
		polystatcount = tab->GetRowCount();
		spell_abilities=(int *) malloc(sizeof(int)*polystatcount);
		for (i=0;i<polystatcount;i++) {
			polymorph_stats[i]=core->TranslateStat(tab->QueryField(i,0));
		}
	}

	//copy polymorphed stats
	for(i=0;i<polystatcount;i++) {
		target->Modified[polymorph_stats[i]]=source->Modified[polymorph_stats[i]];
	}
}

int fx_polymorph (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_polymorph_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	if (!gamedata->Exists(fx->Resource,IE_CRE_CLASS_ID)) {
		//kill all polymorph effects
		target->fxqueue.RemoveAllEffectsWithParam(fx_polymorph_ref, fx->Parameter2);
		//destroy the magic item slot
		target->inventory.RemoveItem(target->inventory.GetMagicSlot() );
		return FX_NOT_APPLIED;
	}
	
	//FIXME:
	//This pointer should be cached, or we are in deep trouble
	Actor *newCreature = gamedata->GetCreature(fx->Resource,0);

	//I don't know how could this happen, existance of the resource was already checked
	if (!newCreature) {
		return FX_NOT_APPLIED;
	}

	//TODO:
	//copy the animation ID
	
	//copy all polymorphed stats
	if(fx->Parameter2) {
		STAT_SET( IE_POLYMORPHED, 1 );
		//FIXME: of course, the first parameter should be the creature we copy
		CopyPolyMorphStats(newCreature, target);
	}

	//FIXME:
	//Be careful when this became a cached pointer
	delete newCreature;
	return FX_APPLIED;
}

// 0x88 ForceVisible
int fx_force_visible (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_force_visible (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	BASE_STATE_CURE(STATE_INVISIBLE);
	target->fxqueue.RemoveAllEffectsWithParam(fx_set_invisible_state_ref,0);
	target->fxqueue.RemoveAllEffectsWithParam(fx_set_invisible_state_ref,2);
	return FX_NOT_APPLIED;
}

// 0x89 ChantBadNonCumulative
int fx_set_chantbad_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_chantbad_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	if (STATE_GET (STATE_CHANTBAD) ) //chant is non cumulative
		return FX_NOT_APPLIED;
	STATE_SET( STATE_CHANTBAD );
	target->SetSpellState(SS_BADCHANT);
	STAT_SUB( IE_LUCK, fx->Parameter1 );
	return FX_APPLIED;
}

// 0x8A AnimationStateChange
int fx_animation_stance (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_animation_stance (%2d): Stance: %d\n", fx->Opcode, fx->Parameter2 );
	target->SetStance(fx->Parameter2);
	return FX_NOT_APPLIED;
}

// 0x8B DisplayString
// gemrb extension: rgb colour for displaystring
static EffectRef fx_protection_from_display_string_ref={"Protection:String",NULL,-1};

int fx_display_string (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_display_string (%2d): StrRef: %d\n", fx->Opcode, fx->Parameter1 );
	if (!target->fxqueue.HasEffectWithParamPair(fx_protection_from_display_string_ref, fx->Parameter1, 0) ) {
		core->DisplayStringName(fx->Parameter1, fx->Parameter2?fx->Parameter2:0xffffff, target, IE_STR_SOUND|IE_STR_SPEECH);
	}
	return FX_NOT_APPLIED;
}

//0x8c CastingGlow
static const int ypos_by_direction[16]={10,10,10,0,-10,-10,-10,-10,-10,-10,-10,-10,0,10,10,10};
static const int xpos_by_direction[16]={0,-2,-4,-6,-8,-6,-4,-2,0,2,4,6,8,6,4,2};

int fx_casting_glow (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_casting_glow (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	if (cgcount<0) {
		cgcount = core->ReadResRefTable("cgtable",casting_glows);
	}
	//remove effect if map is not loaded
	Map *map = target->GetCurrentArea();
	if (!map) {
		return FX_NOT_APPLIED;
	}

	if (fx->Parameter2<(ieDword) cgcount) {
		ScriptedAnimation *sca = gamedata->GetScriptedAnimation(casting_glows[fx->Parameter2], false);
		//remove effect if animation doesn't exist
		if (!sca) {
			return FX_NOT_APPLIED;
		}
		//12 is just an approximate value to set the height of the casting glow
		//based on the avatar's size
		int heightmod = target->GetAnims()->GetCircleSize()*12;
		sca->XPos+=fx->PosX+xpos_by_direction[target->GetOrientation()];
		sca->YPos+=fx->PosY+ypos_by_direction[target->GetOrientation()];
		sca->ZPos+=heightmod;
		sca->SetBlend();
		if (fx->Duration) {
			sca->SetDefaultDuration(fx->Duration-core->GetGame()->GameTime);
		} else {
			sca->SetDefaultDuration(10000);
		}
		map->AddVVCell(sca);
	}
	return FX_NOT_APPLIED;
}

//0x8d VisualSpellHit
int fx_visual_spell_hit (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_visual_spell_hit (%2d): Target: %d Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	if (shcount<0) {
		shcount = core->ReadResRefTable("shtable",spell_hits);
	}
	//remove effect if map is not loaded
	Map *map = target->GetCurrentArea();
	if (!map) {
		return FX_NOT_APPLIED;
	}
	if (fx->Parameter2<(ieDword) shcount) {
		ScriptedAnimation *sca = gamedata->GetScriptedAnimation(spell_hits[fx->Parameter2], false);
		//remove effect if animation doesn't exist
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

//0x8e Icon:Display
int fx_display_portrait_icon (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_display_string (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	target->AddPortraitIcon(fx->Parameter2);
	return FX_APPLIED;
}

//0x8f Item:CreateInSlot
int fx_create_item_in_slot (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_create_item_in_slot (%2d): Button: %d\n", fx->Opcode, fx->Parameter2 );
	//create item and set it in target's slot
	target->inventory.SetSlotItemRes( fx->Resource, core->QuerySlot(fx->Parameter2), fx->Parameter1, fx->Parameter3, fx->Parameter4 );
	if (fx->TimingMode!=FX_DURATION_INSTANT_LIMITED) {
		//convert it to a destroy item
		fx->Opcode=EffectQueue::ResolveEffect(fx_remove_item_ref);
		fx->TimingMode=FX_DURATION_DELAY_PERMANENT;
		return FX_APPLIED;
	}
	return FX_NOT_APPLIED;
}

// 0x90 DisableButton
// different in iwd2 and the rest (maybe also in how: 0-7?)
int fx_disable_button (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_disable_button (%2d): Button: %d\n", fx->Opcode, fx->Parameter2 );

	// iwd2 has a flexible action bar, so there are more possible parameter values
	// only values 0-5 match the bg2 constants (which map to ACT_*)
	// FIXME: support disabling all iwd2 buttons
	if (target->spellbook.IsIWDSpellBook()) {
		if (fx->Parameter2 < 6) STAT_BIT_OR( IE_DISABLEDBUTTON, 1<<fx->Parameter2 );
	} else {
		STAT_BIT_OR( IE_DISABLEDBUTTON, 1<<fx->Parameter2 );
	}

	if (target->InParty && fx->FirstApply) {
		core->SetEventFlag(EF_ACTION);
	}
	return FX_APPLIED;
}

//0x91 DisableSpellCasting
//bg2: 0 - mage, 1 - cleric, 2 - innate
//iwd2: 0 - all, 1 - mage+cleric, 2 - mage, 3 - cleric , 4 - innate
int fx_disable_spellcasting (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_disable_spellcasting (%2d): Button: %d\n", fx->Opcode, fx->Parameter2 );

	if (!fx->FirstApply) {
		return FX_APPLIED;
	}
	bool display_warning = false;

	//IWD2 Style spellbook
	if (target->spellbook.IsIWDSpellBook()) {
		switch(fx->Parameter2) {
			case 0: // all
			case 1: // mage and cleric
			case 2: // mage
				if (target->spellbook.GetKnownSpellsCount(IE_IWD2_SPELL_BARD, 0)) display_warning = true;
				if (target->spellbook.GetKnownSpellsCount(IE_IWD2_SPELL_SORCEROR, 0)) display_warning = true;
				if (target->spellbook.GetKnownSpellsCount(IE_IWD2_SPELL_WIZARD, 0)) display_warning = true;
				break;
		}
	} else { // bg2
		if (fx->Parameter2 == 0)
			if (target->spellbook.GetKnownSpellsCount(IE_SPELL_TYPE_WIZARD, 0)) display_warning = true;
	}
	if (target->InParty && display_warning) {
		core->DisplayConstantStringName(STR_DISABLEDMAGE, 0xff0000, target);
		core->SetEventFlag(EF_ACTION);
	}
	return FX_APPLIED;
}

//0x92 Spell:Cast
int fx_cast_spell (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cast_spell (%2d): Resource:%s Mode: %d\n", fx->Opcode, fx->Resource, fx->Parameter2 );
	if (fx->Parameter2) {
		//apply spell on target
		core->ApplySpell(fx->Resource, target, Owner, fx->Power);
	} else {
		//cast spell on target
		Owner->CastSpell(fx->Resource, target, false);
	}
	return FX_NOT_APPLIED;
}

// 0x93 Spell:Learn
int fx_learn_spell (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_learn_spell (%2d): Resource:%s Mode: %d\n", fx->Opcode, fx->Resource, fx->Parameter2 );
	//parameter1 is unused, gemrb lets you to make it not give XP
	//probably we should also let this via a game flag if we want
	//full compatibility with bg1
	target->LearnSpell(fx->Resource, fx->Parameter2^LS_ADDXP);
	return FX_NOT_APPLIED;
}
// 0x94 Spell:CastSpellPoint
int fx_cast_spell_point (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cast_spell_point (%2d): Resource:%s Mode: %d\n", fx->Opcode, fx->Resource, fx->Parameter2 );
	Owner->CastSpellPoint(fx->Resource, target->Pos, false);
	return FX_NOT_APPLIED;
}

// 0x95 Identify
int fx_identify (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_identify (%2d): Resource:%s Mode: %d\n", fx->Opcode, fx->Resource, fx->Parameter2 );
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
int fx_find_traps (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_find_traps (%2d)\n", fx->Opcode );
	//reveal trapped containers, doors, triggers that are in the visible range
	ieDword range = target->GetStat(IE_VISUALRANGE)*10;
	ieDword skill;

	if (fx->Parameter2 == 0)
		skill = 256;                        //always works
	else
		skill = target->GetStat(IE_TRAPS);  //based on skill

	TileMap *TMap = target->GetCurrentArea()->TMap;

	int Count = 0;
	while (true) {
		Door* door = TMap->GetDoor( Count++ );
		if (!door)
			break;
/*
		//not trapped
		if (!door->Scripts[0]) {
			continue;
		}
		//not detectable
		if (!(door->Flags&DOOR_DETECTABLE)) {
			continue;
		}
*/
		if (Distance(door->Pos, target->Pos)<range) {
			//when was door trap noticed
			door->DetectTrap(skill);
//			door->TrapDetected = 1;
		}
	}

	Count = 0;
	while (true) {
		Container* container = TMap->GetContainer( Count++ );
		if (!container)
			break;
		if (Distance(container->Pos, target->Pos)<range) {
			//when was door trap noticed
			container->DetectTrap(skill);
		}
	}


	Count = 0;
	while (true) {
		InfoPoint* trap = TMap->GetInfoPoint( Count++ );
		if (!trap)
			break;
		if (Distance(trap->Pos, target->Pos)<range) {
			//when was door trap noticed
			trap->DetectTrap(skill);
		}
	}

	return FX_NOT_APPLIED;
}
// 0x97 ReplaceCreature
int fx_replace_creature (Actor* Owner, Actor* target, Effect *fx)
{
	if (0) printf( "fx_replace_creature (%2d): Resource: %s\n", fx->Opcode, fx->Resource );

	//this safeguard exists in the original engine too
	if (!gamedata->Exists(fx->Resource,IE_CRE_CLASS_ID)) {
		return FX_NOT_APPLIED;
	}

	//FIXME: the monster should appear near the effect position?
	//or the target position, this needs experiment
	Point p(fx->PosX, fx->PosY);

	//remove old creature
	switch(fx->Parameter2) {
	case 0: //remove silently
		target->DestroySelf();
		break;
	case 1: //chunky death
		target->NewBase(IE_HITPOINTS,(ieDword) -100, MOD_ABSOLUTE);
		target->Die(Owner);
		break;
	case 2: //normal death
		target->Die(Owner);
		break;
	default:;
	}
	//create replacement
	core->SummonCreature(fx->Resource, fx->Resource2, Owner, NULL,p, -1,0);
	return FX_NOT_APPLIED;
}

// 0x98 PlayMovie
int fx_play_movie (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_play_movie (%2d): Resource: %s\n", fx->Opcode, fx->Resource );
	core->PlayMovie (fx->Resource);
	return FX_NOT_APPLIED;
}
// 0x99 Overlay:Sanctuary
#define ICE_GRADIENT 71

static const ieDword fullwhite[7]={ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT};

int fx_set_sanctuary_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	//iwd and bg are a bit different, but we solve the whole stuff in a single opcode
	if (0) printf( "fx_set_sanctuary_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	if (!fx->Parameter2) {
		fx->Parameter2=1;
	}
	STAT_SET_PCF( IE_SANCTUARY, fx->Parameter2);
	//a rare event, but this effect gives more in bg2 than in iwd2
	//so we use this flag
	if (!enhanced_effects)
	{
		target->SetLockedPalette(fullwhite);
	}
	return FX_APPLIED;
}

// 0x9a Overlay:Entangle
int fx_set_entangle_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_entangle_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
int fx_set_minorglobe_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_minorglobe_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//the resisted levels are stored in minor globe (bit 2-)
	//the globe effect is stored in the first bit
	STAT_BIT_OR_PCF( IE_MINORGLOBE, 1);
	return FX_APPLIED;
}

// 0x9c Overlay:ShieldGlobe
int fx_set_shieldglobe_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_shieldglobe_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_SET_PCF( IE_SHIELDGLOBE, 1);
	return FX_APPLIED;
}

// 0x9d Overlay:Web
int fx_set_web_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_web_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	//iwd2 effects that disable web
	if (target->HasSpellState(SS_FREEACTION)) return FX_NOT_APPLIED;
	if (target->HasSpellState(SS_AEGIS)) return FX_NOT_APPLIED;

	target->SetSpellState(SS_WEB);
	//attack penalty in IWD2
	STAT_SET_PCF( IE_WEB, 1);
	STAT_SET(IE_MOVEMENTRATE, 0); //
	return FX_APPLIED;
}

// 0x9e Overlay:Grease
int fx_set_grease_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_grease_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	//iwd2 effects that disable grease
	if (target->HasSpellState(SS_FREEACTION)) return FX_NOT_APPLIED;
	if (target->HasSpellState(SS_AEGIS)) return FX_NOT_APPLIED;

	target->SetSpellState(SS_GREASE);
	STAT_SET_PCF( IE_GREASE, 1);
	//apparently the movement rate is set by separate opcodes in all engines
	//STAT_SET(IE_MOVEMENTRATE, 3); //iwd2 doesn't have this either
	return FX_APPLIED;
}

// 0x9f MirrorImageModifier
int fx_mirror_image_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_mirror_image_modifier (%2d): Mod: %d\n", fx->Opcode, fx->Parameter1 );
	if (STATE_GET(STATE_DEAD) ) {
		return FX_NOT_APPLIED;
	}
	if (!fx->Parameter1) {
		return FX_NOT_APPLIED;
	}
	STATE_SET( STATE_MIRROR );
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
static EffectRef fx_sanctuary_state_ref={"Overlay:Sanctuary",NULL,-1};

int fx_cure_sanctuary_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_sanctuary_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_SET( IE_SANCTUARY, 0);
	target->fxqueue.RemoveAllEffects(fx_sanctuary_state_ref);
	return FX_NOT_APPLIED;
}

// 0xa1 Cure:Panic
static EffectRef fx_set_panic_state_ref={"State:Panic",NULL,-1};

int fx_cure_panic_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_panic_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	BASE_STATE_CURE( STATE_PANIC );
	target->fxqueue.RemoveAllEffects(fx_set_panic_state_ref);
	return FX_NOT_APPLIED;
}

// 0xa2 Cure:Hold
static EffectRef fx_hold_creature_ref={"State:Hold",NULL,-1};

int fx_cure_hold_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_hold_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//note that this effect doesn't remove 185 (another hold effect)
	target->fxqueue.RemoveAllEffects( fx_hold_creature_ref );
	return FX_NOT_APPLIED;
}

// 0xa3 FreeAction
static EffectRef fx_movement_modifier_ref={"MovementRateModifier2",NULL,-1};

int fx_cure_slow_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_slow_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	target->fxqueue.RemoveAllEffects( fx_movement_modifier_ref );
//	STATE_CURE( STATE_SLOWED );
	return FX_NOT_APPLIED;
}

// 0xA4 Cure:Intoxication
static EffectRef fx_intoxication_ref={"IntoxicationModifier",NULL,-1};

int fx_cure_intoxication (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_intoxication (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	target->fxqueue.RemoveAllEffects( fx_intoxication_ref );
	BASE_SET(IE_INTOXICATION,0);
	return FX_NOT_APPLIED;
}

// 0xA5 PauseTarget
int fx_pause_target (Actor* /*Owner*/, Actor *target, Effect* fx)
{
	if (0) printf( "fx_pause_target (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_CASTERHOLD );
	return FX_PERMANENT;
}

// 0xA6 MagicResistanceModifier
int fx_magic_resistance_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_magic_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_RESISTMAGIC );
	return FX_APPLIED;
}

// 0xA7 MissileHitModifier
int fx_missile_to_hit_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_missile_to_hit_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_MISSILEHITBONUS );
	return FX_APPLIED;
}

// 0xA8 RemoveCreature
// removes creature specified by resource key
int fx_remove_creature (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_remove_creature (%2d)\n", fx->Opcode);
	Map *map = target->GetCurrentArea();
	Actor *actor = map->GetActorByResource(fx->Resource);
	if (actor) {
		//play vvc effect over actor?
		actor->DestroySelf();
	}
	return FX_NOT_APPLIED;
}
// 0xA9 Icon:Disable
int fx_disable_portrait_icon (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_disable_portrait_icon (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	target->DisablePortraitIcon(fx->Parameter2);
	return FX_APPLIED;
}
// 0xAA DamageAnimation
int fx_damage_animation (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_damage_animation (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->PlayDamageAnimation(fx->Parameter2);
	return FX_NOT_APPLIED;
}
// 0xAB Spell:Add
int fx_add_innate (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_add_innate (%2d): Resource: %s Mode: %d\n", fx->Opcode, fx->Resource, fx->Parameter2 );
	target->LearnSpell(fx->Resource, fx->Parameter2^LS_MEMO);
	//this is an instant, so it shouldn't stick
	return FX_NOT_APPLIED;
}
// 0xAC Spell:Remove
//gemrb extension: deplete spell by resref
int fx_remove_spell (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_remove_spell (%2d): Resource: %s Type:%d\n", fx->Opcode, fx->Resource, fx->Parameter2);
	switch (fx->Parameter2) {
	default:
		target->spellbook.RemoveSpell(fx->Resource);
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
// 0xAD PoisonResistanceModifier
int fx_poison_resistance_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_poison_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_RESISTPOISON );
	return FX_APPLIED;
}

//0xae PlaySound
int fx_playsound (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_playsound (%s)", fx->Resource );
	//this is probably inaccurate
	if (target) {
		core->GetAudioDrv()->Play(fx->Resource, target->Pos.x, target->Pos.y);
	} else {
		core->GetAudioDrv()->Play(fx->Resource);
	}
	//this is an instant, it shouldn't stick
	return FX_NOT_APPLIED;
}

//0x6d State:Hold3
//0xfb State:Hold4
int fx_hold_creature_no_icon (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_hold_creature_no_icon (%2d): Value: %d, IDS: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	if ( STATE_GET(STATE_DEAD) ) {
		return FX_NOT_APPLIED;
	}

	if (!EffectQueue::match_ids( target, fx->Parameter2, fx->Parameter1) ) {
		//if the ids don't match, the effect doesn't stick
		return FX_NOT_APPLIED;
	}
	target->SetSpellState(SS_HELD);
	STAT_SET( IE_HELD, 1);
	return FX_APPLIED;
}

//0xaf State:Hold
//0xb9 State:Hold2
//(0x6d/0x1a8 for iwd2)
int fx_hold_creature (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_hold_creature (%2d): Value: %d, IDS: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
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
// b0 see: fx_movement_modifier

// b1 ApplyEffect
int fx_apply_effect (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_apply_effect (%2d) %s", fx->Opcode, fx->Resource );
	if (EffectQueue::match_ids( target, fx->Parameter2, fx->Parameter1) ) {
		//apply effect, if the effect is a goner, then kill
		//this effect too
		if(core->ApplyEffect(fx->Resource, target, Owner, fx->Power)) {
			return FX_APPLIED;
		}
		return FX_NOT_APPLIED;
	}
	//FIXME:
	//if the ids don't match, the effect will still stick?
	return FX_APPLIED;
}
// b2 hitbonus generic effect ToHitVsCreature
// b3 damagebonus generic effect DamageVsCreature
// b4 can't use item (resource) generic effect CantUseItem
// b5 can't use itemtype (resource) generic effect CantUseItemType
// b6 generic effect ApplyEffectItem
// b7 generic effect ApplyEffectItemType
// b8 DontJumpModifier
int fx_dontjump_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_dontjump_modifier (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_SET( IE_DONOTJUMP, fx->Parameter2 );
	return FX_APPLIED;
}

// 0xb9 see above: fx_hold_creature

// 0xba MoveToArea
int fx_move_to_area (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_move_to_area (%2d) %s", fx->Opcode, fx->Resource );
	//delay effect until the target has finished the previous move to an area
	//hopefully this fixes an evil bug
	Map *map = target->GetCurrentArea();
	if (!map || !map->HasActor(target)) {
		//stay around for the next evaluation
		return FX_APPLIED;
	}
	Point p(fx->PosX,fx->PosY);
	MoveBetweenAreasCore(target, fx->Resource, p, fx->Parameter2, true);
	//this effect doesn't stick
	return FX_NOT_APPLIED;
}

// 0xbb Variable:StoreLocalVariable
int fx_local_variable (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	//this is a hack, the variable name spreads across the resources
	if (0) printf( "fx_local_variable (%2d) %s=%d", fx->Opcode, fx->Resource, fx->Parameter1 );
	target->locals->SetAt(fx->Resource, fx->Parameter1);
	//local variable effects are not applied, they will be resaved though
	return FX_NOT_APPLIED;
}

// 0xBC AuraCleansingModifier
int fx_auracleansing_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_auracleansing_modifier (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_SET( IE_AURACLEANSING, fx->Parameter2 );
	return FX_APPLIED;
}

// 0xBD CastingSpeedModifier
int fx_castingspeed_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_castingspeed_modifier (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_MOD( IE_MENTALSPEED );
	return FX_APPLIED;
}

// 0xBE PhysicalSpeedModifier
int fx_attackspeed_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_attackspeed_modifier (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_MOD( IE_PHYSICALSPEED );
	return FX_APPLIED;
}

// 0xbf CastingLevelModifier
int fx_castinglevel_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_castinglevel_modifier (%2d) Value:%d Type:%d", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	switch (fx->Parameter2) {
	case 0:
		STAT_SET( IE_CASTINGLEVELBONUSMAGE, fx->Parameter1 );
		break;
	case 1:
		STAT_SET( IE_CASTINGLEVELBONUSCLERIC, fx->Parameter1 );
		break;
	default:
		return FX_NOT_APPLIED;
	}
	return FX_APPLIED;
}

// 0xc0 FindFamiliar
// param2 = 1 alignment is in param1
// param2 = 2 resource used
#define FAMILIAR_NORMAL    0
#define FAMILIAR_ALIGNMENT 1
#define FAMILIAR_RESOURCE  2

static EffectRef fx_familiar_constitution_loss_ref={"FamiliarBond",NULL,-1};
static EffectRef fx_familiar_marker_ref={"FamiliarMarker",NULL,-1};

int fx_find_familiar (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_find_familiar (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );

	if (!target) {
		return FX_NOT_APPLIED;
	}

	if (!target->GetCurrentArea()) {
		return FX_APPLIED;
	}

	if (fx->Parameter2!=FAMILIAR_RESOURCE) {
		ieDword alignment;

		if (fx->Parameter2==FAMILIAR_ALIGNMENT) {
			alignment = fx->Parameter1;
		} else {
			alignment = target->GetStat(IE_ALIGNMENT);
			alignment = ((alignment&AL_LC_MASK)>>4)*3+(alignment&AL_GE_MASK)-4;
		}
		if (alignment>8) {
			return FX_NOT_APPLIED;
		}
		memcpy(fx->Resource, core->GetGame()->Familiars[alignment],sizeof(ieResRef) );
		fx->Parameter2=FAMILIAR_RESOURCE;
	}
	//summon familiar with fx->Resource
	Point p(fx->PosX, fx->PosY);
	Actor *fam = core->SummonCreature(fx->Resource, fx->Resource2, Owner, target, p, -1,0);
	if (fam) {
		Effect *newfx;

		newfx = EffectQueue::CreateEffect(fx_familiar_constitution_loss_ref, (ieDword) -10, 0, FX_DURATION_INSTANT_PERMANENT);
		core->ApplyEffect(newfx, fam, fam);
		delete newfx;

		newfx = EffectQueue::CreateEffect(fx_familiar_marker_ref, 0, 0, FX_DURATION_INSTANT_PERMANENT);
		core->ApplyEffect(newfx, fam, fam);
		delete newfx;
	}
	return FX_NOT_APPLIED;
}

// 0xc1 InvisibleDetection
int fx_see_invisible_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_see_invisible_modifier (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_SET( IE_SEEINVISIBLE, fx->Parameter2 );
	return FX_APPLIED;
}

// 0xc2 IgnoreDialogPause
int fx_ignore_dialogpause_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_ignore_dialogpause_modifier (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_SET( IE_IGNOREDIALOGPAUSE, fx->Parameter2 );
	return FX_APPLIED;
}

//0xc3 FamiliarBond
//when this effect's target dies it should incur damage on protagonist
static EffectRef fx_damage_opcode_ref={"Damage",NULL,-1};
static EffectRef fx_maximum_hp_modifier_ref={"MaximumHPModifier",NULL,-1};

int fx_familiar_constitution_loss (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_familiar_constitution_loss (%2d): Loss: %d\n", fx->Opcode,(signed) fx->Parameter1 );
	if (! (STAT_GET(IE_STATE_ID)&STATE_NOSAVE)) {
		return FX_APPLIED;
	}
	Effect *newfx;
	//familiar died
	Actor *master = core->GetGame()->FindPC(1);
	if (!master) return FX_NOT_APPLIED;
	//maximum hp (param1 is a negative value)
	newfx = EffectQueue::CreateEffect(fx_maximum_hp_modifier_ref, fx->Parameter1, 0, FX_DURATION_INSTANT_PERMANENT);
	core->ApplyEffect(newfx, master, master);
	delete newfx;

	//damage
	newfx = EffectQueue::CreateEffect(fx_damage_opcode_ref, 0, 0, FX_DURATION_INSTANT_PERMANENT);
	core->ApplyEffect(newfx, master, master);
	delete newfx;

	return FX_NOT_APPLIED;
}

//0xc4 FamiliarMarker
int fx_familiar_marker (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_familiar_marker (%2d)\n", fx->Opcode );
	if (! (STAT_GET(IE_STATE_ID)&STATE_NOSAVE)) {
		//TODO: where to disable familiar?
		//core->GetGame()->WeatherBits|=1;
		return FX_APPLIED;
	}
	//TODO: enable familiar?
	//core->GetGame()->WeatherBits&=~1;
	return FX_NOT_APPLIED;
}

// 0xc5 Bounce:Projectile
int fx_bounce_projectile (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_bounce_projectile (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_BIT_OR( IE_BOUNCE, BNC_PROJECTILE );
	return FX_APPLIED;
}

// 0xc6 Bounce:Opcode
int fx_bounce_opcode (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_bounce_opcode (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_BIT_OR( IE_BOUNCE, BNC_OPCODE );
	target->AddPortraitIcon(PI_BOUNCE2);
	return FX_APPLIED;
}

// 0xc7 Bounce:SpellLevel
int fx_bounce_spelllevel (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_bounce_spellevel (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_BIT_OR( IE_BOUNCE, BNC_LEVEL );
	target->AddPortraitIcon(PI_BOUNCE2);
	return FX_APPLIED;
}

// 0xc8 Bounce:SpellLevelDec
int fx_bounce_spelllevel_dec (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_bounce_spellevel_dec (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_BIT_OR( IE_BOUNCE, BNC_LEVEL_DEC );
	target->AddPortraitIcon(PI_BOUNCE);
	return FX_APPLIED;
}

//0xc9 Protection:SpellLevelDec
int fx_protection_spelllevel_dec (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_protection_spelllevel_dec (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_BIT_OR( IE_IMMUNITY, IMM_LEVEL_DEC );
	target->AddPortraitIcon(PI_BOUNCE2);
	return FX_APPLIED;
}

//0xca Bounce:School
int fx_bounce_school (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_bounce_school (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_BIT_OR( IE_BOUNCE, BNC_SCHOOL );
	target->AddPortraitIcon(PI_BOUNCE2);
	return FX_APPLIED;
}

// 0xcb Bounce:SecondaryType
int fx_bounce_secondary_type (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_bounce_secondary_type (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_BIT_OR( IE_BOUNCE, BNC_SECTYPE );
	target->AddPortraitIcon(PI_BOUNCE2);
	return FX_APPLIED;
}

// 0xcc //resist school
int fx_protection_school (Actor* /*Owner*/, Actor* target, Effect *fx)
{
	if (0) printf( "fx_protection_school (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_BIT_OR( IE_IMMUNITY, IMM_SCHOOL);
	return FX_APPLIED;
}

// 0xcd //resist sectype
int fx_protection_secondary_type (Actor* /*Owner*/, Actor* target, Effect *fx)
{
	if (0) printf( "fx_protection_secondary_type (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_BIT_OR( IE_IMMUNITY, IMM_SECTYPE);
	return FX_APPLIED;
}

//0xce Protection:Spell
int fx_resist_spell (Actor* /*Owner*/, Actor* target, Effect *fx)
{
	if (0) printf( "fx_resist_spell (%2d): Resource: %s\n", fx->Opcode, fx->Resource );
	if (strnicmp(fx->Resource,fx->Source,sizeof(fx->Resource)) ) {
		STAT_BIT_OR( IE_IMMUNITY, IMM_RESOURCE);
		return FX_APPLIED;
	}
	//this has effect only on first apply, it will stop applying the spell
	return FX_ABORT;
}

// ??? Protection:SpellDec
int fx_resist_spell_dec (Actor* /*Owner*/, Actor* target, Effect *fx)
{
	if (0) printf( "fx_resist_spell_dec (%2d): Resource: %s\n", fx->Opcode, fx->Resource );
	if (strnicmp(fx->Resource,fx->Source,sizeof(fx->Resource)) ) {
		STAT_BIT_OR( IE_IMMUNITY, IMM_RESOURCE_DEC);
		return FX_APPLIED;
	}
	//this has effect only on first apply, it will stop applying the spell
	return FX_ABORT;
}

// 0xcf Bounce:Spell
int fx_bounce_spell (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_bounce_spell (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_BIT_OR( IE_BOUNCE, BNC_RESOURCE );
	return FX_APPLIED;
}

// ??? Bounce:SpellDec
int fx_bounce_spell_dec (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_bounce_spell (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_BIT_OR( IE_BOUNCE, BNC_RESOURCE_DEC );
	return FX_APPLIED;
}

// 0xd0 MinimumHPModifier
// the original engine didn't allow modifying of this stat
// it allowed only setting it, and only by one instance
int fx_minimum_hp_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_minimum_hp_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_MINHITPOINTS );
	return FX_APPLIED;
}

//0xd1 PowerWordKill
int fx_power_word_kill (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_power_word_kill (%2d): HP: %d Stat: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	ieDword limit = 60;

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
int fx_power_word_stun (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_power_word_stun (%2d): HP: %d Stat: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
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
	stat = core->Roll(stat,x?x:4,0) * ROUND_SIZE;
	fx->Duration = core->GetGame()->GameTime+stat;
	fx->TimingMode = FX_DURATION_INSTANT_LIMITED;
	fx->Opcode = EffectQueue::ResolveEffect(fx_set_stun_state_ref);
	return fx_set_stun_state(Owner,target,fx);
}

//0xd3 State:Imprisonment (avatar removal plus portrait icon)
int fx_imprisonment (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_inprisonment (%2d)\n", fx->Opcode );
	target->SetMCFlag(MC_HIDDEN, BM_OR);
	target->AddPortraitIcon(PI_PRISON);
	return FX_APPLIED;
}

//0xd4 Cure:Imprisonment
static EffectRef fx_imprisonment_ref={"Imprisonment",NULL,-1};
static EffectRef fx_maze_ref={"Maze",NULL,-1};

int fx_freedom (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_freedom (%2d)\n", fx->Opcode );
	target->fxqueue.RemoveAllEffects( fx_imprisonment_ref );
	target->fxqueue.RemoveAllEffects( fx_maze_ref );
	return FX_NOT_APPLIED;
}

//0xd5 Maze
int fx_maze (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_maze (%2d)\n", fx->Opcode );
	target->SetMCFlag(MC_HIDDEN, BM_OR);
	target->AddPortraitIcon(PI_MAZE);
	return FX_APPLIED;
}

//0xd6 CastFromList
int fx_select_spell (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_select_spell (%2d) %d\n", fx->Opcode, fx->Parameter2 );
	//if parameter2==0 -> cast spells from 2da (all spells listed in 2da)
	//if parameter2==1 -> cast spells from book (all known spells, no need of memorize)
	return FX_NOT_APPLIED;
}

// 0xd7 PlayVisualEffect
int fx_play_visual_effect (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_play_visual_effect (%2d): Resource: %s Type: %d\n", fx->Opcode, fx->Resource, fx->Parameter2 );

	//this is in the original engine (dead actors lose this effect)
	if (STATE_GET( STATE_DEAD) ) {
		return FX_NOT_APPLIED;
	}

	//delay action until area is loaded to avoid crash
	Map *map = target->GetCurrentArea();
	if (!map) return FX_APPLIED;

	//if it is sticky, don't add it if it is already played
	if (fx->Parameter2) {
		if (!target->HasVVCCell(fx->Resource) ) {
			return FX_NOT_APPLIED;
		}
	}

	ScriptedAnimation* sca = gamedata->GetScriptedAnimation(fx->Resource, false);

	//don't crash on nonexistent resources
	if (!sca) {
		return FX_NOT_APPLIED;
	}

	if (fx->TimingMode!=FX_DURATION_INSTANT_PERMANENT) {
		sca->SetDefaultDuration(fx->Duration-core->GetGame()->GameTime);
	}
	if (fx->Parameter2) {
		//play over target (sticky)
		target->AddVVCell( sca );
		return FX_APPLIED;
	}

	//not sticky
	sca->XPos=fx->PosX;
	sca->YPos=fx->PosY;
	map->AddVVCell( sca );
	return FX_NOT_APPLIED;
}

//d8 LevelDrainModifier
int fx_leveldrain_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_leveldrain_modifier (%2d): Mod: %d\n", fx->Opcode, fx->Parameter1 );

	STAT_ADD(IE_LEVELDRAIN, fx->Parameter1);
	STAT_ADD(IE_MAXHITPOINTS, -fx->Parameter1*4);
	STAT_ADD(IE_HITPOINTS, -fx->Parameter1*4);
	return FX_APPLIED;
}

//d9 PowerWordSleep
static EffectRef fx_sleep_ref={"State:Sleep",NULL,-1};

int fx_power_word_sleep (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_power_word_sleep (%2d): HP: %d Stat: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
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
	fx->Duration = core->GetGame()->GameTime+x*ROUND_SIZE;
	fx->TimingMode = FX_DURATION_INSTANT_LIMITED;
	fx->Opcode = EffectQueue::ResolveEffect(fx_sleep_ref);
	fx->Parameter2=0;
	return fx_set_unconscious_state(Owner,target,fx);
}

// 0xDA StoneSkinModifier
int fx_stoneskin_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_stoneskin_modifier (%2d): Mod: %d\n", fx->Opcode, fx->Parameter1 );
	if (!fx->Parameter1) {
		return FX_NOT_APPLIED;
	}
	//this is the bg2 style stoneskin, not normally using spell states
	//but this way we can support hybrid games
	if (fx->Parameter2) {
		target->SetSpellState(SS_IRONSKIN);
		//gradient for iron skins?
	} else {
		target->SetSpellState(SS_STONESKIN);
		SetGradient(target, 14);
	}
	STAT_SET(IE_STONESKINS, fx->Parameter1);
	return FX_APPLIED;
}

//0xDB ac vs creature type (general effect)
//0xDC DispelSchool
int fx_dispel_school (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_dispel_school (%2d): Level: %d Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	target->fxqueue.RemoveLevelEffects(fx->Parameter1, RL_MATCHSCHOOL, fx->Parameter2);
	return FX_NOT_APPLIED;
}
//0xDD DispelSecondaryType
int fx_dispel_secondary_type (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_dispel_secondary_type (%2d): Level: %d Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	target->fxqueue.RemoveLevelEffects(fx->Parameter1, RL_MATCHSECTYPE, fx->Parameter2);
	return FX_NOT_APPLIED;
}

//0xDE RandomTeleport
int fx_teleport_field (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_teleport_field (%2d): Distance: %d\n", fx->Opcode, fx->Parameter1 );

	Map *map = target->GetCurrentArea();
	if (!map) {
		return FX_NOT_APPLIED;
	}
	//this should be the target's position, i think
	Point p = target->Pos;
	p.x+=core->Roll(1,fx->Parameter1*2,-(signed) (fx->Parameter1));
	p.y+=core->Roll(1,fx->Parameter1*2,-(signed) (fx->Parameter1));
	target->SetPosition( p, true, 0);
	return FX_NOT_APPLIED;
}

//0xDF //Protection:SchoolDec
int fx_protection_school_dec (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_protection_school_dec (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	if (fx->Parameter1) {
		STAT_BIT_OR( IE_IMMUNITY, IMM_SCHOOL_DEC );
		return FX_APPLIED;
	}
	return FX_NOT_APPLIED;
}

//0xe0 Cure:LevelDrain
static EffectRef fx_leveldrain_ref={"LevelDrainModifier",NULL,-1};

int fx_cure_leveldrain (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_leveldrain (%2d)\n", fx->Opcode );
	//all level drain removed at once???
	target->fxqueue.RemoveAllEffects( fx_leveldrain_ref );
	return FX_NOT_APPLIED;
}

//0xe1 Reveal:Magic
//gemrb special: speed and color are custom
int fx_reveal_magic (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_reveal_magic (%2d)\n", fx->Opcode );
	if (target->fxqueue.HasAnyDispellableEffect()) {
		if (!fx->Parameter1) {
			fx->Parameter1=0xff00; //blue
		}

		int speed = (fx->Parameter2 >> 16) & 0xFF;
		if (!speed) speed=30;
		target->SetColorMod(0xff, RGBModifier::ADD, speed,
			fx->Parameter1 >> 8, fx->Parameter1 >> 16,
			fx->Parameter1 >> 24, 0);
	}
	return FX_NOT_APPLIED;
}

//0xe2 Protection:SecondaryTypeDec
int fx_protection_secondary_type_dec (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_protection_secondary_type_dec (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	if (fx->Parameter1) {
		STAT_BIT_OR( IE_IMMUNITY, IMM_SECTYPE_DEC );
		return FX_APPLIED;
	}
	return FX_NOT_APPLIED;
}

//0xe3 Bounce:SchoolDecrement
int fx_bounce_school_dec (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_bounce_school_dec (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_BIT_OR( IE_BOUNCE, BNC_SCHOOL_DEC );
	target->AddPortraitIcon(PI_BOUNCE2);
	return FX_APPLIED;
}

//0xe4 Bounce:SecondaryTypeDecrement
int fx_bounce_secondary_type_dec (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_bounce_secondary_type_dec (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_BIT_OR( IE_BOUNCE, BNC_SECTYPE_DEC );
	target->AddPortraitIcon(PI_BOUNCE2);
	return FX_APPLIED;
}

//0xE5 DispelSchoolOne
int fx_dispel_school_one (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_dispel_school_one (%2d): Level: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	target->fxqueue.RemoveLevelEffects(fx->Parameter1, RL_MATCHSCHOOL|RL_REMOVEFIRST, fx->Parameter2);
	return FX_NOT_APPLIED;
}

//0xE6 DispelSecondaryTypeOne
int fx_dispel_secondary_type_one (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_dispel_secondary_type_one (%2d): Level: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	target->fxqueue.RemoveLevelEffects(fx->Parameter1,RL_MATCHSECTYPE|RL_REMOVEFIRST, fx->Parameter2);
	return FX_NOT_APPLIED;
}

//0xE7 Timestop
int fx_timestop (Actor* Owner, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_timestop (%2d)\n", fx->Opcode);
	core->GetGame()->TimeStop(Owner, fx->Duration);
	return FX_NOT_APPLIED;
}

//0xE8 CastSpellOnCondition
int fx_cast_spell_on_condition (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cast_spell_on_condition (%2d): Target: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	//get subject of check
	Actor *actor = NULL;
	Map *map = target->GetCurrentArea();
	switch(fx->Parameter1) {
		//self
	case 0: actor = target; break;
		//last attacker
	case 1: actor = map->GetActorByGlobalID(target->LastHitter); break;
		//nearest enemy
		//fix this!
	//case 2: actor = map->GetActorByGlobalID(target->LastSeen); break;
	case 2: actor = GetNearestEnemyOf(map, target, 0); break;
		//nearest creature
	case 3: actor = map->GetActorByGlobalID(target->LastSeen); break;
	}
	if (!actor) {
		return FX_APPLIED;
	}
	int condition;
	//check condition
	switch(fx->Parameter2) {
	case COND_GOTHIT: //on hit
		condition = target->LastDamage;
		break;
	case COND_NEAR: //
		condition = PersonalDistance(actor, target)<30;
		break;
	case COND_HP_HALF:
		condition = actor->GetStat(IE_HITPOINTS)<actor->GetStat(IE_MAXHITPOINTS)/2;
		break;
	case COND_HP_QUART:
		condition = actor->GetStat(IE_HITPOINTS)<actor->GetStat(IE_MAXHITPOINTS)/4;
		break;
	case COND_HP_LOW:
		condition = actor->GetStat(IE_HITPOINTS)<actor->GetStat(IE_MAXHITPOINTS)/10;
		break;
	case COND_HELPLESS:
		condition = actor->GetStat(IE_STATE_ID) & STATE_CANTMOVE;
		break;
	case COND_POISONED:
		condition = actor->GetStat(IE_STATE_ID) & STATE_POISONED;
		break;
	case COND_ATTACKED:
		condition = actor->LastHitter;
		break;
	case COND_HIT:
		condition = actor->LastDamage;
		break;
	case COND_ALWAYS:
		condition = 1;
		break;
	default:
		condition = 0;
	}

	if (condition) {
		core->ApplySpell(fx->Resource, actor, Owner, fx->Power);
	}
	return FX_APPLIED;
}

// 0xE9 Proficiency
int fx_proficiency (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_proficiency (%2d): Value: %d, Stat: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	//probably no need to check the boundaries, the original IE
	//did check it (though without boundaries, it is more useful)
	//this opcode works only if the previous value was smaller
	if (STAT_GET(fx->Parameter2)<fx->Parameter1) {
		STAT_SET (fx->Parameter2, fx->Parameter1);
	}
	return FX_APPLIED;
}

// 0xea CreateContingency
int fx_create_contingency (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_create_contingency (%2d): Value: %d, Stat: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	if (target->InParty) {
		BASE_SET(IE_IDENTIFYMODE,2);
		core->SetEventFlag(EF_SEQUENCER);
	}
	return FX_NOT_APPLIED;
}

#define WB_AWAY 0
#define WB_TOWARDS 1
#define WB_FIXDIR 2
#define WB_OWNDIR 3
#define WB_AWAYOWNDIR 4

// 0xeb WingBuffet
int fx_wing_buffet (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_wing_buffet (%2d): Value: %d, Stat: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//create movement in actor

	ieDword dir;
	switch(fx->Parameter2) {
		case WB_AWAY:
		default:
			dir = GetOrient(Owner->Pos, target->Pos);
			break;
		case WB_TOWARDS:
			dir = GetOrient(target->Pos, Owner->Pos);
			break;
		case WB_FIXDIR:
			dir = fx->Parameter3;
			break;
		case WB_OWNDIR:
			dir = target->GetOrientation();
			break;
		case WB_AWAYOWNDIR:
			dir = target->GetOrientation()^8;
			break;
	}
	//could be GL_REBOUND too :)
	//add effect to alter target's stance
	target->MoveLine( fx->Parameter1, GL_NORMAL, dir );
	return FX_NOT_APPLIED;
}

// 0xec ProjectImage

static EffectRef fx_unsummon_creature_ref={"UnsummonCreature",NULL,-1};

int fx_puppet_master (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	const char * resref = NULL;

	if (0) printf( "fx_puppet_master (%2d): Value: %d, Stat: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_SET (IE_PUPPETMASTERTYPE, fx->Parameter1);
	Actor *copy = target->CopySelf();
	switch(fx->Parameter2)
	{
	case 1:
		resref = "MISLEAD";
		break;
	case 2:
		resref = "PROJIMG";
		break;
	case 3:
		resref = "SIMULACR";
		break;
	default:
		resref = fx->Resource;
		break;
	}
	//resref is always something, but we play safe
	if (resref && resref[0]) {
		core->ApplySpell(resref,copy,target,0);
	}
	Effect *newfx = EffectQueue::CreateEffectCopy(fx, fx_unsummon_creature_ref, 0, 0);
	core->ApplyEffect(newfx, copy, target);
	return FX_NOT_APPLIED;
}

// 0xed PuppetMarker
int fx_puppet_marker (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_puppet_marker (%2d): Value: %d, Stat: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_SET (IE_PUPPETTYPE, fx->Parameter1);
	return FX_APPLIED;
}

// 0xee Disintegrate
int fx_disintegrate (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_disintegrate (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
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
#define FS_UNEXPLORED  1
#define FS_VISUALRANGE 2
#define FS_HASPOINT    4
#define FS_LOS         8

int fx_farsee (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_farsee (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	Map *map = target->GetCurrentArea();
	if (!map) {
		return FX_APPLIED;
	}

	if (!(fx->Parameter2&FS_VISUALRANGE)) {
		fx->Parameter1=STAT_GET(IE_VISUALRANGE);
		fx->Parameter2|=FS_VISUALRANGE;
	}

	if (target->InParty) {
		//don't start graphical interface if actor isn't in party
		if (!(fx->Parameter2&FS_HASPOINT)) {
			//start graphical interface
			//it will do all the rest of the opcode
			//using RevealMap guiscript action
			core->EventFlag|=EF_SHOWMAP;
			return FX_NOT_APPLIED;
		}
	}

	Point p(fx->PosX, fx->PosY);

	//don't explore unexplored points
	if (!(fx->Parameter2&FS_UNEXPLORED)) {
		if (!map->IsVisible(p, 1)) {
			return FX_NOT_APPLIED;
		}
	}
	map->ExploreMapChunk(p, fx->Parameter1, fx->Parameter2&FS_LOS);
	return FX_NOT_APPLIED;
}

// 0xf0 Icon:Remove
int fx_remove_portrait_icon (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_remove_portrait_icon (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	target->fxqueue.RemoveAllEffectsWithParam( fx_display_portrait_icon_ref, fx->Parameter2 );
	return FX_NOT_APPLIED;
}
// 0xf1 control creature (same as charm)

// 0xF2 Cure:Confusion
static EffectRef fx_confused_state_ref={"State:Confused",NULL,-1};

int fx_cure_confused_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_confused_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	BASE_STATE_CURE( STATE_CONFUSED );
	target->fxqueue.RemoveAllEffects(fx_confused_state_ref);
	return FX_NOT_APPLIED;
}

// 0xf3 DrainItems (this is disabled in ToB)
int fx_drain_items (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_drain_items (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	ieDword i=fx->Parameter1;
	while (i--) {
		//deplete magic item = 0
		//deplete weapon = 1
		target->inventory.DepleteItem(fx->Parameter2);
	}
	return FX_NOT_APPLIED;
}
// 0xf4 DrainSpells
int fx_drain_spells (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_drain_spells (%2d): Count: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	ieDword i=fx->Parameter1;
	if (fx->Parameter2) {
		while(i--) {
			if (!target->spellbook.DepleteSpell(IE_SPELL_TYPE_PRIEST)) {
				break;
			}
		}
		return FX_NOT_APPLIED;
	}
	while(i--) {
		if (!target->spellbook.DepleteSpell(IE_SPELL_TYPE_WIZARD)) {
			break;
		}
	}
	return FX_NOT_APPLIED;
}
// 0xf5 CheckForBerserk
int fx_checkforberserk_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_checkforberserk_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_SET( IE_CHECKFORBERSERK, fx->Parameter2 );
	return FX_APPLIED;
}
// 0xf6 BerserkStage1Modifier
int fx_berserkstage1_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_berserkstage1_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_SET( IE_BERSERKSTAGE1, fx->Parameter2 );
	return FX_APPLIED;
}
// 0xf7 BerserkStage2Modifier
int fx_berserkstage2_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_berserkstage2_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_SET( IE_BERSERKSTAGE2, fx->Parameter2 );
	STATE_SET (STATE_BERSERK);
	return FX_APPLIED;
}
// 0xf8 set melee effect generic effect?
// 0xf9 set missile effect generic effect?
// 0xfa DamageLuckModifier
int fx_damageluck_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_damageluck_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_DAMAGELUCK );
	return FX_APPLIED;
}

// 0xfb bardsong (generic effect)

// 0xfc SetTrap
int fx_set_area_effect (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_trap (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	ieDword skill, roll;
	Map *map;

	map = target->GetCurrentArea();
	if (!map) return FX_NOT_APPLIED;

	proIterator iter;

	//check if trap count is over an amount (only saved traps count)
	//actually, only projectiles in trigger phase should count here
	if (map->GetTrapCount(iter)>6) {
		core->DisplayConstantStringName(STR_NOMORETRAP, 0xf0f0f0, target);
		return FX_NOT_APPLIED;
	}

	//check if we are under attack
	if (GetNearestEnemyOf(map, target, ORIGIN_SEES_ENEMY|ENEMY_SEES_ORIGIN)) {
		core->DisplayConstantStringName(STR_MAYNOTSETTRAP, 0xf0f0f0, target);
		return FX_NOT_APPLIED;
	}

	skill = Owner->GetStat(IE_SETTRAPS);
	roll = core->Roll(1,100,0);

	if (roll>skill) {
		//failure
		core->DisplayConstantStringName(STR_SNAREFAILED, 0xf0f0f0, target);
		//TODO check luck and do some damage effect on target
		return FX_NOT_APPLIED;
	}
	//success
	core->DisplayConstantStringName(STR_SNARESUCCEED, 0xf0f0f0, target);
	Owner->CastSpellPoint(fx->Resource, target->Pos, false);
	return FX_NOT_APPLIED;
}

// 0xfd SetMapNote
int fx_set_map_note (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_map_note (%2d): StrRef: %d Color: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	Actor *marker = target?target:Owner;
	Map *map = marker->GetCurrentArea();
	if (!map) return FX_APPLIED; //delay effect
	Point p(fx->PosX, fx->PosY);
	char *text = core->GetString(fx->Parameter1, 0);
	map->AddMapNote(p, fx->Parameter2, text);
	return FX_NOT_APPLIED;
}

// 0xfe RemoveMapNote
int fx_remove_map_note (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_remove_map_note (%2d)\n", fx->Opcode);
	Actor *marker = target?target:Owner;
	Map *map = marker->GetCurrentArea();
	if (!map) return FX_APPLIED; //delay effect
	Point p(fx->PosX, fx->PosY);
	map->RemoveMapNote(p);
	return FX_NOT_APPLIED;
}

// 0xff Item:CreateDays
int fx_create_item_days (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_create_item_days (%2d)\n", fx->Opcode );
	target->inventory.SetSlotItemRes( fx->Resource, -1, fx->Parameter1, fx->Parameter3, fx->Parameter4 );
	if (fx->TimingMode==FX_DURATION_INSTANT_LIMITED) {
//if this effect has expiration, then it will remain as a remove_item
//on the effect queue, inheriting all the parameters
		//duration needs a hack (recalculate it for days)
		//no idea if this multiplier is ok
		fx->Duration+=(fx->Duration-core->GetGame()->GameTime)*2400;
		fx->Opcode=EffectQueue::ResolveEffect(fx_remove_inventory_item_ref);
		fx->TimingMode=FX_DURATION_DELAY_PERMANENT;
		return FX_APPLIED;
	}
	return FX_NOT_APPLIED;
}

// 0x100 Sequencer:Store
int fx_store_spell_sequencer(Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_store_spell_sequencer (%2d)\n", fx->Opcode );
	//just display the spell sequencer portrait icon
	target->AddPortraitIcon(PI_SEQUENCER);
	return FX_APPLIED;
}

// 0x101 Sequencer:Create
int fx_create_spell_sequencer(Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_create_spell_sequencer (%2d)\n", fx->Opcode );
	//just a call to activate the spell sequencer creation gui
	if (target->InParty) {
		BASE_SET(IE_IDENTIFYMODE,3);
		core->SetEventFlag(EF_SEQUENCER);
	}
	return FX_NOT_APPLIED;
}

// 0x102 Sequencer:Activate
static EffectRef fx_spell_sequencer_active_ref={"Sequencer:Store",NULL,-1};

int fx_activate_spell_sequencer(Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_activate_spell_sequencer (%2d): Resource: %s\n", fx->Opcode, fx->Resource );
	Effect *sequencer = Owner->fxqueue.HasEffect(fx_spell_sequencer_active_ref);
	if (sequencer) {
		//cast 1-4 spells stored in the spell sequencer
		core->ApplySpell(sequencer->Resource, target, Owner, fx->Power);
		core->ApplySpell(sequencer->Resource2, target, Owner, fx->Power);
		core->ApplySpell(sequencer->Resource3, target, Owner, fx->Power);
		core->ApplySpell(sequencer->Resource4, target, Owner, fx->Power);
		//remove the spell sequencer store effect
		sequencer->TimingMode=FX_DURATION_JUST_EXPIRED;
	}
	return FX_NOT_APPLIED;
}

// 0x103 SpellTrap (Protection:SpellLevelDec + recall spells)
int fx_spelltrap(Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_spelltrap (%2d): Count: %d, Level: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	if (fx->Parameter3) {
		target->RestoreSpellLevel(fx->Parameter3, 0);
		fx->Parameter3=0;
	}
	if (fx->Parameter1<=0) {
		//gone down to zero
		return FX_NOT_APPLIED;
	}
	target->SetOverlay(OV_SPELLTRAP);
	target->AddPortraitIcon(PI_SPELLTRAP);
	return FX_APPLIED;
}

//0x104 Crash104
//0x138 Crash138
int fx_crash (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_crash (%2d): Param1: %d, Param2: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	return FX_NOT_APPLIED;
}

// 0x105 RestoreSpells
int fx_restore_spell_level(Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_restore_spell_level (%2d): Level: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	target->RestoreSpellLevel(fx->Parameter1, fx->Parameter2);
	return FX_NOT_APPLIED;
}
// 0x106 VisualRangeModifier
int fx_visual_range_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_visual_range_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_VISUALRANGE );
	return FX_APPLIED;
}

// 0x107 BackstabModifier
int fx_backstab_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_visual_range_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_BACKSTABDAMAGEMULTIPLIER );
	return FX_APPLIED;
}

// 0x108 DropWeapon
int fx_drop_weapon (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (fx->Resource[0]) {
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
int fx_modify_global_variable (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	Game *game = core->GetGame();
	//convert it to internal variable format
	if (!fx->IsVariable) {
		char *poi=fx->Resource+8;
		memmove(poi, fx->Resource2,8);
		poi+=8;
		memmove(poi, fx->Resource3,8);
		poi+=8;
		memmove(poi, fx->Resource4,8);
		fx->IsVariable=1;
	}

	//hack for IWD
	if (!fx->Resource[0]) {
		strnuprcpy(fx->Resource,"RETURN_TO_LONELYWOOD",32);
	}

	if (0) printf( "fx_modify_global_variable (%2d): Variable: %s Value: %d Type: %d\n", fx->Opcode, fx->Resource, fx->Parameter1, fx->Parameter2 );
	if (fx->Parameter2) {
		ieDword var = 0;
		//use resource memory area as variable name
		game->locals->Lookup(fx->Resource, var);
		game->locals->SetAt(fx->Resource, var+fx->Parameter1);
	} else {
		game->locals->SetAt(fx->Resource, fx->Parameter1);
	}
	return FX_NOT_APPLIED;
}
// 0x10a RemoveImmunity
EffectRef immunity_effect_ref={"Protection:Spell",NULL,-1};

int fx_remove_immunity(Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_remove_immunity (%2d): %s\n", fx->Opcode, fx->Resource );
	target->fxqueue.RemoveAllEffectsWithResource(immunity_effect_ref, fx->Resource);
	return FX_NOT_APPLIED;
}

// 0x10b protection from display string is a generic effect
// 0x10c ExploreModifier
int fx_explore_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_explore_modifier (%2d)\n", fx->Opcode );
	if (fx->Parameter2) {
		//gemrb modifier
		STAT_SET (IE_EXPLORE, fx->Parameter1);
	} else {
		STAT_SET (IE_EXPLORE, 1);
	}
	return FX_APPLIED;
}
// 0x10d ScreenShake
int fx_screenshake (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_screenshake (%2d): Strength: %d\n", fx->Opcode, fx->Parameter1 );
	core->timer->SetScreenShake( fx->Parameter1, fx->Parameter1, 1);
	return FX_APPLIED;
}

// 0x10e Cure:CasterHold
static EffectRef fx_pause_caster_modifier_ref={"PauseTarget",NULL,-1};

int fx_unpause_caster (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_unpause_caster (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	Effect *eff = target->fxqueue.HasEffect(fx_pause_caster_modifier_ref);
	if (eff) {
		eff->Parameter1-=fx->Parameter2;
	}
	return FX_NOT_APPLIED;
}
// 0x10f AvatarRemoval
// 0x104 AvatarRemoval (iwd)
int fx_avatar_removal (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_avatar_removal (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//FIXME: this is a permanent irreversible effect in IWD
	//if it is different in bg2, then create another effect
	//bg2 calls this SummonDisable
	BASE_SET(IE_AVATARREMOVAL, 1);
	return FX_NOT_APPLIED;
}
// 0x110 ApplyEffectRepeat
int fx_apply_effect_repeat (Actor* Owner, Actor* target, Effect* fx)
{
	ieDword i; //moved here because msvc6 cannot handle it otherwise

	if (0) printf( "fx_apply_effect_repeat (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	switch (fx->Parameter2) {
		case 0: //once per second
		case 1: //crash???
			core->ApplyEffect(fx->Resource, target, Owner, fx->Power);
			break;
		case 2://param1 times every second
			for (i=0;i<fx->Parameter1;i++) {
				core->ApplyEffect(fx->Resource, target, Owner, fx->Power);
			}
			break;
		case 3: //once every Param1 second
			if (fx->Parameter1 && (core->GetGame()->GameTime%fx->Parameter1)) {
				core->ApplyEffect(fx->Resource, target, Owner, fx->Power);
			}
			break;
		case 4: //param3 times every Param1 second
			if (fx->Parameter1 && (core->GetGame()->GameTime%fx->Parameter1)) {
				for (i=0;i<fx->Parameter3;i++) {
					core->ApplyEffect(fx->Resource, target, Owner, fx->Power);
				}
			}
			break;
	}
	return FX_APPLIED;
}

// 0x111 RemoveProjectile
int fx_remove_projectile (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	ieDword *projectilelist;
	
	//instant effect 
	if (0) printf( "fx_remove_projectile (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	if (!target) return FX_NOT_APPLIED;
	Map *area = target->GetCurrentArea();
	if (!area) return FX_NOT_APPLIED;

	switch (fx->Parameter2) {
	case 0: //standard bg2
		projectilelist = core->GetListFrom2DA("clearair");
		break;
	case 1: //you can give a 2da for projectile list (gemrb)
		projectilelist = core->GetListFrom2DA(fx->Resource);
		break;
	case 2: //or you can give one single projectile in param1 (gemrb)
		projectilelist = (ieDword *) malloc(2*sizeof(ieDword));
		projectilelist[0]=1;
		projectilelist[1]=fx->Parameter1;
		break;
	default:
		return FX_NOT_APPLIED;
	}
	//The first element is the counter, so don't decrease the counter here
	Point p(fx->PosX, fx->PosY);

	while(projectilelist[0]) {
		ieDword projectile = projectilelist[projectilelist[0]];
		proIterator piter;

		size_t cnt = area->GetProjectileCount(piter);
		while( cnt--) {
			Projectile *pro = *piter;
			if ((pro->GetType()==projectile) && pro->PointInRadius(p) ) {
				pro->Cleanup();
			}
		}
		if (target) {
			target->fxqueue.RemoveAllEffectsWithProjectile(projectile);
		}
		projectilelist[0]--;
	}
	free(projectilelist);
	return FX_NOT_APPLIED;
}

// 0x112 TeleportToTarget
int fx_teleport_to_target (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_teleport_to_target (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	Map *map = target->GetCurrentArea();
	if (map) {
		Actor *victim = map->GetActorByGlobalID(target->LastAttacker);
		if (victim) {
			target->SetPosition( victim->Pos, true, 0 );
		}
	}
	return FX_NOT_APPLIED;
}
// 0x113 HideInShadowsModifier
int fx_hide_in_shadows_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_hide_in_shadows_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_HIDEINSHADOWS );
	return FX_APPLIED;
}
// 0x114 DetectIllusionsModifier
int fx_detect_illusion_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_detect_illusion_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_DETECTILLUSIONS );
	return FX_APPLIED;
}
// 0x115 SetTrapsModifier
int fx_set_traps_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_traps_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_SETTRAPS );
	return FX_APPLIED;
}
// 0x116 ToHitBonusModifier
int fx_to_hit_bonus_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_to_hit_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_HITBONUS );
	return FX_APPLIED;
}

// 0x117 RenableButton
static EffectRef fx_disable_button_ref={"DisableButton",NULL,-1};

int fx_renable_button (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	//removes the disable button effect
	if (0) printf( "fx_renable_button (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	target->fxqueue.RemoveAllEffectsWithParam( fx_disable_button_ref, fx->Parameter2 );
	return FX_NOT_APPLIED;
}
// 0x118 ForceSurgeModifier
int fx_force_surge_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_force_surge_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_FORCESURGE );
	return FX_APPLIED;
}

// 0x119 WildSurgeModifier
int fx_wild_surge_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_wild_surge_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_SURGEMOD );
	return FX_APPLIED;
}

// 0x11a ScriptingState
int fx_scripting_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_scripting_state (%2d): Value: %d, Stat: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
int fx_apply_effect_curse (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_apply_effect_curse (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	if (EffectQueue::match_ids( target, fx->Parameter2, fx->Parameter1) ) {
		//load effect and add it to the end of the effect queue?
		core->ApplyEffect(fx->Resource, target, Owner, fx->Power);
	}
	return FX_APPLIED;
}

// 0x11c MeleeHitModifier
int fx_melee_to_hit_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_melee_to_hit_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_MELEEHIT );
	return FX_APPLIED;
}

// 0x11d MeleeDamageModifier
int fx_melee_damage_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_melee_damage_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_MELEEDAMAGE );
	return FX_APPLIED;
}

// 0x11e MissileDamageModifier
int fx_missile_damage_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_missile_damage_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_MISSILEDAMAGE );
	return FX_APPLIED;
}

// 0x11f NoCircleState
int fx_no_circle_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_missile_damage_modifier (%2d)\n", fx->Opcode);
	STAT_SET( IE_NOCIRCLE, 1 );
	return FX_APPLIED;
}

// 0x120 FistHitModifier
int fx_fist_to_hit_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_fist_to_hit_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_FISTHIT );
	return FX_APPLIED;
}

// 0x121 FistDamageModifier
int fx_fist_damage_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_fist_damage_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_FISTDAMAGE );
	return FX_APPLIED;
}
//0x122 TitleModifier
int fx_title_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_fist_damage_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	if (fx->Parameter2) {
		STAT_SET( IE_TITLE2, fx->Parameter1 );
	} else {
		STAT_SET( IE_TITLE1, fx->Parameter1 );
	}
	return FX_APPLIED;
}
//0x123 DisableOverlay
//FIXME: which overlay is disabled?
//if one of the overlays marked by sanctuary, then
//make the bit correspond to it
int fx_disable_overlay_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_disable_overlay_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_SET( IE_DISABLEOVERLAY, fx->Parameter1 );
	return FX_APPLIED;
}
//0x124 Protection:Backstab (bg2)
//0x11f Protection:Backstab (how, iwd2)
//3 different games, 3 different methods of flagging this
int fx_no_backstab_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_no_backstab_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//bg2
	STAT_SET( IE_DISABLEBACKSTAB, fx->Parameter1 );
	//how
	EXTSTATE_SET(EXTSTATE_NO_BACKSTAB);
	//iwd2
	target->SetSpellState(SS_NOBACKSTAB);
	return FX_APPLIED;
}
//0x125 OffscreenAIModifier
int fx_offscreenai_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_offscreenai_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_SET( IE_ENABLEOFFSCREENAI, fx->Parameter1 );
	return FX_APPLIED;
}
//0x126 ExistanceDelayModifier
int fx_existance_delay_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_existance_delay_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_SET( IE_EXISTANCEDELAY, fx->Parameter1 );
	return FX_APPLIED;
}
//0x127 DisableChunk
int fx_disable_chunk_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_disable_chunk_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_SET( IE_DISABLECHUNKING, fx->Parameter1 );
	return FX_APPLIED;
}
//0x128 Protection:Animation
int fx_protection_from_animation (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_protection_from_animation (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//remove vvc from actor if active
	target->RemoveVVCell(fx->Resource, false);
	return FX_APPLIED;
}
//0x129 Protection:Turn
int fx_protection_from_turn (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_non_interruptible_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_SET( IE_NOTURNABLE, fx->Parameter1 );
	return FX_APPLIED;
}
//0x12a CutScene2
//runs a predetermined script in cutscene mode
int fx_cutscene2 (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	Game *game;
	ieResRef resref;

	if (0) printf( "fx_cutscene2 (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	if (core->InCutSceneMode()) return FX_NOT_APPLIED;
	game = core->GetGame();
	if (!game) return FX_NOT_APPLIED;
	core->SetCutSceneMode(true);

	//GemRB enhancement: allow a custom resource
	if (fx->Resource[0]) {
		strnlwrcpy(resref,fx->Resource, 8);
	} else {
		strnlwrcpy(resref,"cut250a",8);
	}

	GameScript* gs = new GameScript( resref, ST_GLOBAL );
	gs->MySelf = game;
	gs->EvaluateAllBlocks();
	delete( gs );
	//for safety reasons, i get this pointer again
	game = core->GetGame();
	if (game) {
		game->ClearCutsceneID();
	}
	return FX_NOT_APPLIED;
}
//0x12b ChaosShieldModifier
int fx_chaos_shield_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_chaos_shield_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_CHAOSSHIELD );
	if (fx->Parameter2) {
		target->AddPortraitIcon(PI_CSHIELD); //162
	} else {
		target->AddPortraitIcon(PI_CSHIELD2); //163
	}
	return FX_APPLIED;
}
//0x12c NPCBump
int fx_npc_bump (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_npc_bump (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//unknown effect, but known stat position
	STAT_MOD( IE_NPCBUMP );
	return FX_APPLIED;
}
//0x12d CriticalHitModifier
int fx_critical_hit_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_critical_hit_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_CRITICALHITBONUS );
	return FX_APPLIED;
}
// 0x12e CanUseAnyItem
int fx_can_use_any_item_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_can_use_any_item_modifier (%2d): Value: %d\n", fx->Opcode, fx->Parameter2 );

	STAT_SET( IE_CANUSEANYITEM, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x12f AlwaysBackstab
int fx_always_backstab_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_always_backstab_modifier (%2d): Value: %d\n", fx->Opcode, fx->Parameter2 );

	STAT_SET( IE_ALWAYSBACKSTAB, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x130 MassRaiseDead
int fx_mass_raise_dead (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_mass_raise_dead (%2d)\n", fx->Opcode );

	Game *game=core->GetGame();

	int i=game->GetPartySize(false);
	while (i--) {
		Actor *actor=game->GetPC(i,false);
		actor->Resurrect();
	}
	return FX_NOT_APPLIED;
}

// 0x131 OffhandHitModifier
int fx_left_to_hit_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_left_to_hit_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_HITBONUSLEFT );
	return FX_APPLIED;
}

// 0x132 RightHitModifier
int fx_right_to_hit_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_right_to_hit_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_HITBONUSRIGHT );
	return FX_APPLIED;
}

// 0x133 Reveal:Tracks
int fx_reveal_tracks (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_reveal_tracks (%2d): Distance: %d\n", fx->Opcode, fx->Parameter1 );
	Map *map = target->GetCurrentArea();
	if (!map) return FX_APPLIED;
	if (!fx->Parameter2) {
		fx->Parameter2=1;
		//write tracks.2da entry
		if (map->DisplayTrackString(target)) {
			return FX_NOT_APPLIED;
		}
	}
	GameControl *gc = core->GetGameControl();
	if (gc) {
		//highlight all living creatures (not in party, but within range)
		gc->SetTracker(target, fx->Parameter1);
	}
	return FX_APPLIED;
}

// 0x134 Protection:Tracking
int fx_protection_from_tracking (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_protection_from_tracking (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_NOTRACKING ); //highlight creature???
	return FX_APPLIED;
}
// 0x135 ModifyLocalVariable
int fx_modify_local_variable (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	//convert it to internal variable format
	if (!fx->IsVariable) {
		char *poi = fx->Resource+8;
		memmove(poi, fx->Resource2, 8);
		poi+=8;
		memmove(poi, fx->Resource3, 8);
		poi+=8;
		memmove(poi, fx->Resource4, 8);
		fx->IsVariable=1;
	}
	if (0) printf( "fx_modify_local_variable (%2d): %s, Mod: %d\n", fx->Opcode, fx->Resource, fx->Parameter2 );
	if (fx->Parameter2) {
		ieDword var = 0;
		//use resource memory area as variable name
		target->locals->Lookup(fx->Resource, var);
		target->locals->SetAt(fx->Resource, var+fx->Parameter1);
	} else {
		target->locals->SetAt(fx->Resource, fx->Parameter1);
	}
	return FX_NOT_APPLIED;
}

// 0x136 TimelessState
int fx_timeless_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_timeless_modifier (%2d): Mod: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_SET(IE_DISABLETIMESTOP, fx->Parameter2);
	return FX_APPLIED;
}

//0x137 GenerateWish
#define WISHCOUNT 25
static int wishlevels[WISHCOUNT]={10,10,10,10,0,10,0,15,0,0,0,0,0,0,15,0,0,
17,9,17,17,9,9,9,0};

int fx_generate_wish (Actor* Owner, Actor* target, Effect* fx)
{
	ieResRef spl;

	if (0) printf( "fx_generate_wish (%2d): Mod: %d\n", fx->Opcode, fx->Parameter2 );
	int tmp = core->Roll(1,WISHCOUNT,0);
	sprintf(spl,"SPWISH%02d",tmp);
	core->ApplySpell(spl, target, Owner, wishlevels[tmp-1]);
	return FX_NOT_APPLIED;
}
//0x138 //see fx_crash, this effect is not fully enabled in original bg2/tob
int fx_immunity_sequester (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_immunity_sequester (%2d): Mod: %d\n", fx->Opcode, fx->Parameter2 );
	//this effect is supposed to provide immunity against sequester (maze/etc?)
	STAT_SET(IE_NOSEQUESTER, fx->Parameter2);
	return FX_APPLIED;
}

//0x139 //HLA generic effect
//0x13a StoneSkin2Modifier
int fx_golem_stoneskin_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_golem_stoneskin_modifier (%2d): Mod: %d\n", fx->Opcode, fx->Parameter1 );
	if (!fx->Parameter1) {
		return FX_NOT_APPLIED;
	}
	STAT_SET(IE_STONESKINSGOLEM, fx->Parameter1);
	SetGradient(target, 14);
	return FX_APPLIED;
}

// 0x13b AvatarRemovalModifier (also 0x104 iwd)
int fx_avatar_removal_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_avatar_removal_modifier (%2d): Mod: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_SET(IE_AVATARREMOVAL, fx->Parameter2);
	return FX_APPLIED;
}
// 0x13c MagicalRest (also 0x124 iwd)
int fx_magical_rest (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_magical_rest (%2d)\n", fx->Opcode );
	//instant, full rest
	target->Rest(0);
	target->fxqueue.RemoveAllEffectsWithParam(fx_display_portrait_icon_ref, PI_FATIGUE);
	return FX_NOT_APPLIED;
}

// 0x13d ImprovedHaste (See 0x10 Haste)
// unknown
int fx_unknown (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	printf( "fx_unknown (%2d): P1: %d P2: %d ResRef: %s\n", fx->Opcode, fx->Parameter1, fx->Parameter2, fx->Resource );
	return FX_NOT_APPLIED;
}
