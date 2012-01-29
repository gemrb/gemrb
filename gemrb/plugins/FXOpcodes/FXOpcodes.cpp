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
#include "win32def.h"

#include "Audio.h"
#include "DisplayMessage.h"
#include "EffectQueue.h"
#include "Game.h"
#include "GameData.h"
#include "GlobalTimer.h"
#include "Interface.h"
#include "PolymorphCache.h" // fx_polymorph
#include "Projectile.h" //needs for clearair
#include "ScriptedAnimation.h"
#include "Spell.h" //needed for fx_cast_spell feedback
#include "TileMap.h" //needs for knock!
#include "damages.h"
#include "GameScript/GSUtils.h" //needs for MoveBetweenAreasCore
#include "GameScript/Matching.h" //needs for GetAllObjects
#include "GUI/GameControl.h"
#include "Scriptable/Actor.h"
#include "Scriptable/Container.h"
#include "Scriptable/Door.h"
#include "Scriptable/InfoPoint.h"
#include "Scriptable/PCStatStruct.h" //fx_polymorph (action definitions)

//FIXME: find a way to handle portrait icons better
#define PI_RIGID     2
#define PI_CONFUSED  3
#define PI_BERSERK   4
#define PI_POISONED  6
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
#define PI_STUN     55
#define PI_AID      57
#define PI_HOLY     59
#define PI_BOUNCE   65
#define PI_BOUNCE2  67

#define PI_CONTINGENCY 75
#define PI_BLOODRAGE 76 //iwd2
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

static ieResRef *casting_glows = NULL;
static int cgcount = -1;
static ieResRef *spell_hits = NULL;
static ieDword enhanced_effects = 0;
static int shcount = -1;
static int *spell_abilities = NULL;
static ieDword splabcount = 0;
static int *polymorph_stats = NULL;
static int polystatcount = 0;
static ieDword pstflags = false;

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
int fx_set_blind_state (Scriptable* Owner, Actor* target, Effect* fx);//4a
int fx_cure_blind_state (Scriptable* Owner, Actor* target, Effect* fx);//4b
int fx_set_feebleminded_state (Scriptable* Owner, Actor* target, Effect* fx);//4c
int fx_cure_feebleminded_state (Scriptable* Owner, Actor* target, Effect* fx);//4d
int fx_set_diseased_state (Scriptable* Owner, Actor* target, Effect*fx);//4e
int fx_cure_diseased_state (Scriptable* Owner, Actor* target, Effect* fx);//4f
int fx_set_deaf_state (Scriptable* Owner, Actor* target, Effect* fx); //50
int fx_set_deaf_state_iwd2 (Scriptable* Owner, Actor* target, Effect* fx); //50
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
int fx_existance_delay_modifier (Scriptable* Owner, Actor* target, Effect* fx);//126
int fx_disable_chunk_modifier (Scriptable* Owner, Actor* target, Effect* fx);//127
int fx_protection_from_animation (Scriptable* Owner, Actor* target, Effect* fx);//128
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
int fx_change_weather (Scriptable* Owner, Actor* target, Effect* fx);//13e ChangeWeather

int fx_unknown (Scriptable* Owner, Actor* target, Effect* fx);//???

// FIXME: Make this an ordered list, so we could use bsearch!
static EffectDesc effectnames[] = {
	{ "*Crash*", fx_crash, EFFECT_NO_ACTOR, -1 },
	{ "AcidResistanceModifier", fx_acid_resistance_modifier, 0, -1 },
	{ "ACVsCreatureType", fx_generic_effect, 0, -1 }, //0xdb
	{ "ACVsDamageTypeModifier", fx_ac_vs_damage_type_modifier, 0, -1 },
	{ "ACVsDamageTypeModifier2", fx_ac_vs_damage_type_modifier, 0, -1 }, // used in IWD
	{ "AidNonCumulative", fx_set_aid_state, 0, -1 },
	{ "AIIdentifierModifier", fx_ids_modifier, 0, -1 },
	{ "AlchemyModifier", fx_alchemy_modifier, 0, -1 },
	{ "Alignment:Change", fx_alignment_change, 0, -1 },
	{ "Alignment:Invert", fx_alignment_invert, 0, -1 },
	{ "AlwaysBackstab", fx_always_backstab_modifier, 0, -1 },
	{ "AnimationIDModifier", fx_animation_id_modifier, 0, -1 },
	{ "AnimationStateChange", fx_animation_stance, 0, -1 },
	{ "ApplyEffect", fx_apply_effect, EFFECT_NO_ACTOR, -1 },
	{ "ApplyEffectCurse", fx_apply_effect_curse, 0, -1 },
	{ "ApplyEffectItem", fx_apply_effect_item, 0, -1 },
	{ "ApplyEffectItemType", fx_apply_effect_item_type, 0, -1 },
	{ "ApplyEffectRepeat", fx_apply_effect_repeat, 0, -1 },
	{ "CutScene2", fx_cutscene2, EFFECT_NO_ACTOR, -1 },
	{ "AttackSpeedModifier", fx_attackspeed_modifier, 0, -1 },
	{ "AttacksPerRoundModifier", fx_attacks_per_round_modifier, 0, -1 },
	{ "AuraCleansingModifier", fx_auracleansing_modifier, 0, -1 },
	{ "SummonDisable", fx_summon_disable, 0, -1 }, //unknown
	{ "AvatarRemovalModifier", fx_avatar_removal_modifier, 0, -1 },
	{ "BackstabModifier", fx_backstab_modifier, 0, -1 },
	{ "BerserkStage1Modifier", fx_berserkstage1_modifier, 0, -1 },
	{ "BerserkStage2Modifier", fx_berserkstage2_modifier, 0, -1 },
	{ "BlessNonCumulative", fx_set_bless_state, 0, -1 },
	{ "Bounce:School", fx_bounce_school, 0, -1 },
	{ "Bounce:SchoolDec", fx_bounce_school_dec, 0, -1 },
	{ "Bounce:SecondaryType", fx_bounce_secondary_type, 0, -1 },
	{ "Bounce:SecondaryTypeDec", fx_bounce_secondary_type_dec, 0, -1 },
	{ "Bounce:Spell", fx_bounce_spell, 0, -1 },
	{ "Bounce:SpellDec", fx_bounce_spell_dec, 0, -1 },
	{ "Bounce:SpellLevel", fx_bounce_spelllevel, 0, -1 },
	{ "Bounce:SpellLevelDec", fx_bounce_spelllevel_dec, 0, -1 },
	{ "Bounce:Opcode", fx_bounce_opcode, 0, -1 },
	{ "Bounce:Projectile", fx_bounce_projectile, 0, -1 },
	{ "CantUseItem", fx_generic_effect, EFFECT_NO_ACTOR, -1 },
	{ "CantUseItemType", fx_generic_effect, 0, -1 },
	{ "CanUseAnyItem", fx_can_use_any_item_modifier, 0, -1 },
	{ "CastFromList", fx_select_spell, 0, -1 },
	{ "CastingGlow", fx_casting_glow, 0, -1 },
	{ "CastingGlow2", fx_casting_glow, 0, -1 }, //used in iwd
	{ "CastingLevelModifier", fx_castinglevel_modifier, 0, -1 },
	{ "CastingSpeedModifier", fx_castingspeed_modifier, 0, -1 },
	{ "CastSpellOnCondition", fx_cast_spell_on_condition, 0, -1 },
	{ "ChangeBardSong", fx_change_bardsong, 0, -1 },
	{ "ChangeName", fx_change_name, 0, -1 },
	{ "ChangeWeather", fx_change_weather, EFFECT_NO_ACTOR, -1 },
	{ "ChantBadNonCumulative", fx_set_chantbad_state, 0, -1 },
	{ "ChantNonCumulative", fx_set_chant_state, 0, -1 },
	{ "ChaosShieldModifier", fx_chaos_shield_modifier, 0, -1 },
	{ "CharismaModifier", fx_charisma_modifier, 0, -1 },
	{ "CheckForBerserkModifier", fx_checkforberserk_modifier, 0, -1 },
	{ "ColdResistanceModifier", fx_cold_resistance_modifier, 0, -1 },
	{ "Color:BriefRGB", fx_brief_rgb, 0, -1 },
	{ "Color:GlowRGB", fx_glow_rgb, 0, -1 },
	{ "Color:DarkenRGB", fx_darken_rgb, 0, -1 },
	{ "Color:SetPalette", fx_set_color_gradient, 0, -1 },
	{ "Color:SetRGB", fx_set_color_rgb, 0, -1 },
	{ "Color:SetRGBGlobal", fx_set_color_rgb_global, 0, -1 }, //08
	{ "Color:PulseRGB", fx_set_color_pulse_rgb, 0, -1 }, //9
	{ "Color:PulseRGBGlobal", fx_set_color_pulse_rgb_global, 0, -1 }, //9
	{ "ConstitutionModifier", fx_constitution_modifier, 0, -1 },
	{ "ControlCreature", fx_set_charmed_state, 0, -1 }, //0xf1 same as charm
	{ "CreateContingency", fx_create_contingency, 0, -1 },
	{ "CriticalHitModifier", fx_critical_hit_modifier, 0, -1 },
	{ "CrushingResistanceModifier", fx_crushing_resistance_modifier, 0, -1 },
	{ "Cure:Berserk", fx_cure_berserk_state, 0, -1 },
	{ "Cure:Blind", fx_cure_blind_state, 0, -1 },
	{ "Cure:CasterHold", fx_unpause_caster, 0, -1 },
	{ "Cure:Confusion", fx_cure_confused_state, 0, -1 },
	{ "Cure:Deafness", fx_cure_deaf_state, 0, -1 },
	{ "Cure:Death", fx_cure_dead_state, 0, -1 },
	{ "Cure:Defrost", fx_cure_frozen_state, 0, -1 },
	{ "Cure:Disease", fx_cure_diseased_state, 0, -1 },
	{ "Cure:Feeblemind", fx_cure_feebleminded_state, 0, -1 },
	{ "Cure:Hold", fx_cure_hold_state, 0, -1 },
	{ "Cure:Imprisonment", fx_freedom, 0, -1 },
	{ "Cure:Infravision", fx_cure_infravision_state, 0, -1 },
	{ "Cure:Intoxication", fx_cure_intoxication, 0, -1 }, //0xa4 (iwd2 has this working)
	{ "Cure:Invisible", fx_cure_invisible_state, 0, -1 }, //0x2f
	{ "Cure:Invisible2", fx_cure_invisible_state, 0, -1 }, //0x74
	//{ "Cure:ImprovedInvisible", fx_cure_improved_invisible_state, 0, -1 },
	{ "Cure:LevelDrain", fx_cure_leveldrain, 0, -1 }, //restoration
	{ "Cure:Nondetection", fx_cure_nondetection_state, 0, -1 },
	{ "Cure:Panic", fx_cure_panic_state, 0, -1 },
	{ "Cure:Petrification", fx_cure_petrified_state, 0, -1 },
	{ "Cure:Poison", fx_cure_poisoned_state, 0, -1 },
	{ "Cure:Sanctuary", fx_cure_sanctuary_state, 0, -1 },
	{ "Cure:Silence", fx_cure_silenced_state, 0, -1 },
	{ "Cure:Sleep", fx_cure_sleep_state, 0, -1 },
	{ "Cure:Stun", fx_cure_stun_state, 0, -1 },
	{ "CurrentHPModifier", fx_current_hp_modifier, EFFECT_DICED, -1 },
	{ "Damage", fx_damage, EFFECT_DICED, -1 },
	{ "DamageAnimation", fx_damage_animation, 0, -1 },
	{ "DamageBonusModifier", fx_damage_bonus_modifier, 0, -1 },
	{ "DamageLuckModifier", fx_damageluck_modifier, 0, -1 },
	{ "DamageVsCreature", fx_generic_effect, 0, -1 },
	{ "Death", fx_death, 0, -1 },
	{ "Death2", fx_death, 0, -1 }, //(iwd2 effect)
	{ "Death3", fx_death, 0, -1 }, //(iwd2 effect too, Banish)
	{ "DetectAlignment", fx_detect_alignment, 0, -1 },
	{ "DetectIllusionsModifier", fx_detect_illusion_modifier, 0, -1 },
	{ "DexterityModifier", fx_dexterity_modifier, 0, -1 },
	{ "DimensionDoor", fx_dimension_door, 0, -1 },
	{ "DisableButton", fx_disable_button, 0, -1 }, //sets disable button flag
	{ "DisableChunk", fx_disable_chunk_modifier, 0, -1 },
	{ "DisableOverlay", fx_disable_overlay_modifier, 0, -1 },
	{ "DisableCasting", fx_disable_spellcasting, 0, -1 },
	{ "Disintegrate", fx_disintegrate, 0, -1 },
	{ "DispelEffects", fx_dispel_effects, 0, -1 },
	{ "DispelSchool", fx_dispel_school, 0, -1 },
	{ "DispelSchoolOne", fx_dispel_school_one, 0, -1 },
	{ "DispelSecondaryType", fx_dispel_secondary_type, 0, -1 },
	{ "DispelSecondaryTypeOne", fx_dispel_secondary_type_one, 0, -1 },
	{ "DisplayString", fx_display_string, 0, -1 },
	{ "Dither", fx_dither, 0, -1 },
	{ "DontJumpModifier", fx_dontjump_modifier, 0, -1 },
	{ "DrainItems", fx_drain_items, 0, -1 },
	{ "DrainSpells", fx_drain_spells, 0, -1 },
	{ "DropWeapon", fx_drop_weapon, 0, -1 },
	{ "ElectricityResistanceModifier", fx_electricity_resistance_modifier, 0, -1 },
	{ "ExistanceDelayModifier", fx_existance_delay_modifier , 0, -1 }, //unknown
	{ "ExperienceModifier", fx_experience_modifier, 0, -1 },
	{ "ExploreModifier", fx_explore_modifier, 0, -1 },
	{ "FamiliarBond", fx_familiar_constitution_loss, 0, -1 },
	{ "FamiliarMarker", fx_familiar_marker, 0, -1 },
	{ "Farsee", fx_farsee, 0, -1 },
	{ "FatigueModifier", fx_fatigue_modifier, 0, -1 },
	{ "FindFamiliar", fx_find_familiar, 0, -1 },
	{ "FindTraps", fx_find_traps, 0, -1 },
	{ "FindTrapsModifier", fx_find_traps_modifier, 0, -1 },
	{ "FireResistanceModifier", fx_fire_resistance_modifier, 0, -1 },
	{ "FistDamageModifier", fx_fist_damage_modifier, 0, -1 },
	{ "FistHitModifier", fx_fist_to_hit_modifier, 0, -1 },
	{ "ForceSurgeModifier", fx_force_surge_modifier, 0, -1 },
	{ "ForceVisible", fx_force_visible, 0, -1 }, //not invisible but improved invisible
	{ "FreeAction", fx_cure_slow_state, 0, -1 },
	{ "GenerateWish", fx_generate_wish, 0, -1 },
	{ "GoldModifier", fx_gold_modifier, 0, -1 },
	{ "HideInShadowsModifier", fx_hide_in_shadows_modifier, 0, -1 },
	{ "HLA", fx_generic_effect, 0, -1 },
	{ "HolyNonCumulative", fx_set_holy_state, 0, -1 },
	{ "Icon:Disable", fx_disable_portrait_icon, 0, -1 },
	{ "Icon:Display", fx_display_portrait_icon, 0, -1 },
	{ "Icon:Remove", fx_remove_portrait_icon, 0, -1 },
	{ "Identify", fx_identify, 0, -1 },
	{ "IgnoreDialogPause", fx_ignore_dialogpause_modifier, 0, -1 },
	{ "IntelligenceModifier", fx_intelligence_modifier, 0, -1 },
	{ "IntoxicationModifier", fx_intoxication_modifier, 0, -1 },
	{ "InvisibleDetection", fx_see_invisible_modifier, 0, -1 },
	{ "Item:CreateDays", fx_create_item_days, 0, -1 },
	{ "Item:CreateInSlot", fx_create_item_in_slot, 0, -1 },
	{ "Item:CreateInventory", fx_create_inventory_item, 0, -1 },
	{ "Item:CreateMagic", fx_create_magic_item, 0, -1 },
	{ "Item:Equip", fx_equip_item, 0, -1 }, //71
	{ "Item:Remove", fx_remove_item, 0, -1 }, //70
	{ "Item:RemoveInventory", fx_remove_inventory_item, 0, -1 },
	{ "KillCreatureType", fx_kill_creature_type, 0, -1 },
	{ "LevelModifier", fx_level_modifier, 0, -1 },
	{ "LevelDrainModifier", fx_leveldrain_modifier, 0, -1 },
	{ "LoreModifier", fx_lore_modifier, 0, -1 },
	{ "LuckModifier", fx_luck_modifier, 0, -1 },
	{ "LuckCumulative", fx_luck_cumulative, 0, -1 },
	{ "LuckNonCumulative", fx_luck_non_cumulative, 0, -1 },
	{ "MagicalColdResistanceModifier", fx_magical_cold_resistance_modifier, 0, -1 },
	{ "MagicalFireResistanceModifier", fx_magical_fire_resistance_modifier, 0, -1 },
	{ "MagicalRest", fx_magical_rest, 0, -1 },
	{ "MagicDamageResistanceModifier", fx_magic_damage_resistance_modifier, 0, -1 },
	{ "MagicResistanceModifier", fx_magic_resistance_modifier, 0, -1 },
	{ "MassRaiseDead", fx_mass_raise_dead, EFFECT_NO_ACTOR, -1 },
	{ "MaximumHPModifier", fx_maximum_hp_modifier, EFFECT_DICED, -1 },
	{ "Maze", fx_maze, 0, -1 },
	{ "MeleeDamageModifier", fx_melee_damage_modifier, 0, -1 },
	{ "MeleeHitModifier", fx_melee_to_hit_modifier, 0, -1 },
	{ "MinimumHPModifier", fx_minimum_hp_modifier, 0, -1 },
	{ "MiscastMagicModifier", fx_miscast_magic_modifier, 0, -1 },
	{ "MissileDamageModifier", fx_missile_damage_modifier, 0, -1 },
	{ "MissileHitModifier", fx_missile_to_hit_modifier, 0, -1 },
	{ "MissilesResistanceModifier", fx_missiles_resistance_modifier, 0, -1 },
	{ "MirrorImage", fx_mirror_image, 0, -1 },
	{ "MirrorImageModifier", fx_mirror_image_modifier, 0, -1 },
	{ "ModifyGlobalVariable", fx_modify_global_variable, EFFECT_NO_ACTOR, -1 },
	{ "ModifyLocalVariable", fx_modify_local_variable, 0, -1 },
	{ "MonsterSummoning", fx_monster_summoning, EFFECT_NO_ACTOR, -1 },
	{ "MoraleBreakModifier", fx_morale_break_modifier, 0, -1 },
	{ "MoraleModifier", fx_morale_modifier, 0, -1 },
	{ "MovementRateModifier", fx_movement_modifier, 0, -1 }, //fast (7e)
	{ "MovementRateModifier2", fx_movement_modifier, 0, -1 },//slow (b0)
	{ "MovementRateModifier3", fx_movement_modifier, 0, -1 },//forced (IWD - 10a)
	{ "MovementRateModifier4", fx_movement_modifier, 0, -1 },//slow (IWD2 - 1b9)
	{ "MoveToArea", fx_move_to_area, 0, -1 }, //0xba
	{ "NoCircleState", fx_no_circle_state, 0, -1 },
	{ "NPCBump", fx_npc_bump, 0, -1 },
	{ "OffscreenAIModifier", fx_offscreenai_modifier, 0, -1 },
	{ "OffhandHitModifier", fx_left_to_hit_modifier, 0, -1 },
	{ "OpenLocksModifier", fx_open_locks_modifier, 0, -1 },
	{ "Overlay:Entangle", fx_set_entangle_state, 0, -1 },
	{ "Overlay:Grease", fx_set_grease_state, 0, -1 },
	{ "Overlay:MinorGlobe", fx_set_minorglobe_state, 0, -1 },
	{ "Overlay:Sanctuary", fx_set_sanctuary_state, 0, -1 },
	{ "Overlay:ShieldGlobe", fx_set_shieldglobe_state, 0, -1 },
	{ "Overlay:Web", fx_set_web_state, 0, -1 },
	{ "PauseTarget", fx_pause_target, 0, -1 }, //also known as casterhold
	{ "PickPocketsModifier", fx_pick_pockets_modifier, 0, -1 },
	{ "PiercingResistanceModifier", fx_piercing_resistance_modifier, 0, -1 },
	{ "PlayMovie", fx_play_movie, EFFECT_NO_ACTOR, -1 },
	{ "PlaySound", fx_playsound, EFFECT_NO_ACTOR, -1 },
	{ "PlayVisualEffect", fx_play_visual_effect, 0, -1 },
	{ "PoisonResistanceModifier", fx_poison_resistance_modifier, 0, -1 },
	{ "Polymorph", fx_polymorph, 0, -1 },
	{ "PortraitChange", fx_portrait_change, 0, -1 },
	{ "PowerWordKill", fx_power_word_kill, 0, -1 },
	{ "PowerWordSleep", fx_power_word_sleep, 0, -1 },
	{ "PowerWordStun", fx_power_word_stun, 0, -1 },
	{ "PriestSpellSlotsModifier", fx_bonus_priest_spells, 0, -1 },
	{ "Proficiency", fx_proficiency, 0, -1 },
//	{ "Protection:Animation", fx_protection_from_animation, 0, -1 },
	{ "Protection:Animation", fx_generic_effect, 0, -1 },
	{ "Protection:Backstab", fx_no_backstab_modifier, 0, -1 },
	{ "Protection:Creature", fx_generic_effect, 0, -1 },
	{ "Protection:Opcode", fx_protection_opcode, 0, -1 },
	{ "Protection:Opcode2", fx_protection_opcode, 0, -1 },
	{ "Protection:Projectile",fx_protection_from_projectile, 0, -1 },
	{ "Protection:School",fx_generic_effect, 0, -1 },//overlay?
	{ "Protection:SchoolDec",fx_protection_school_dec, 0, -1 },//overlay?
	{ "Protection:SecondaryType",fx_protection_secondary_type, 0, -1 },//overlay?
	{ "Protection:SecondaryTypeDec",fx_protection_secondary_type_dec, 0, -1 },//overlay?
	{ "Protection:Spell",fx_resist_spell, 0, -1 },//overlay?
	{ "Protection:SpellDec",fx_resist_spell_dec, 0, -1 },//overlay?
	{ "Protection:SpellLevel",fx_protection_spelllevel, 0, -1 },//overlay?
	{ "Protection:SpellLevelDec",fx_protection_spelllevel_dec, 0, -1 },//overlay?
	{ "Protection:String", fx_generic_effect, 0, -1 },
	{ "Protection:Tracking", fx_protection_from_tracking, 0, -1 },
	{ "Protection:Turn", fx_protection_from_turn, 0, -1 },
	{ "Protection:Weapons", fx_immune_to_weapon, EFFECT_NO_ACTOR, -1 },
	{ "PuppetMarker", fx_puppet_marker, 0, -1 },
	{ "ProjectImage", fx_puppet_master, 0, -1 },
	{ "Reveal:Area", fx_reveal_area, EFFECT_NO_ACTOR, -1 },
	{ "Reveal:Creatures", fx_reveal_creatures, 0, -1 },
	{ "Reveal:Magic", fx_reveal_magic, 0, -1 },
	{ "Reveal:Tracks", fx_reveal_tracks, 0, -1 },
	{ "RemoveCurse", fx_remove_curse, 0, -1 },
	{ "RemoveImmunity", fx_remove_immunity, 0, -1 },
	{ "RemoveMapNote", fx_remove_map_note, EFFECT_NO_ACTOR, -1 },
	{ "RemoveProjectile", fx_remove_projectile, 0, -1 }, //removes effects from actor and area
	{ "RenableButton", fx_renable_button, 0, -1 }, //removes disable button flag
	{ "RemoveCreature", fx_remove_creature, EFFECT_NO_ACTOR, -1 },
	{ "ReplaceCreature", fx_replace_creature, 0, -1 },
	{ "ReputationModifier", fx_reputation_modifier, 0, -1 },
	{ "RestoreSpells", fx_restore_spell_level, 0, -1 },
	{ "RetreatFrom2", fx_turn_undead, 0, -1 },
	{ "RightHitModifier", fx_right_to_hit_modifier, 0, -1 },
	{ "SaveVsBreathModifier", fx_save_vs_breath_modifier, 0, -1 },
	{ "SaveVsDeathModifier", fx_save_vs_death_modifier, 0, -1 },
	{ "SaveVsPolyModifier", fx_save_vs_poly_modifier, 0, -1 },
	{ "SaveVsSpellsModifier", fx_save_vs_spell_modifier, 0, -1 },
	{ "SaveVsWandsModifier", fx_save_vs_wands_modifier, 0, -1 },
	{ "ScreenShake", fx_screenshake, EFFECT_NO_ACTOR, -1 },
	{ "ScriptingState", fx_scripting_state, 0, -1 },
	{ "Sequencer:Activate", fx_activate_spell_sequencer, 0, -1 },
	{ "Sequencer:Create", fx_create_spell_sequencer, 0, -1 },
	{ "Sequencer:Store", fx_store_spell_sequencer, 0, -1 },
	{ "SetAIScript", fx_set_ai_script, 0, -1 },
	{ "SetMapNote", fx_set_map_note, EFFECT_NO_ACTOR, -1 },
	{ "SetMeleeEffect", fx_generic_effect, 0, -1 },
	{ "SetRangedEffect", fx_generic_effect, 0, -1 },
	{ "SetTrap", fx_set_area_effect, 0, -1 },
	{ "SetTrapsModifier", fx_set_traps_modifier, 0, -1 },
	{ "SexModifier", fx_sex_modifier, 0, -1 },
	{ "SlashingResistanceModifier", fx_slashing_resistance_modifier, 0, -1 },
	{ "Sparkle", fx_sparkle, 0, -1 },
	{ "SpellDurationModifier", fx_spell_duration_modifier, 0, -1 },
	{ "Spell:Add", fx_add_innate, 0, -1 },
	{ "Spell:Cast", fx_cast_spell, 0, -1 },
	{ "Spell:CastPoint", fx_cast_spell_point, 0, -1 },
	{ "Spell:Learn", fx_learn_spell, 0, -1 },
	{ "Spell:Remove", fx_remove_spell, 0, -1 },
	{ "Spelltrap",fx_spelltrap , 0, -1 }, //overlay: spmagglo
	{ "State:Berserk", fx_set_berserk_state, 0, -1 },
	{ "State:Blind", fx_set_blind_state, 0, -1 },
	{ "State:Blur", fx_set_blur_state, 0, -1 },
	{ "State:Charmed", fx_set_charmed_state, EFFECT_NO_LEVEL_CHECK, -1 }, //0x05
	{ "State:Confused", fx_set_confused_state, 0, -1 },
	{ "State:Deafness", fx_set_deaf_state, 0, -1 },
	{ "State:DeafnessIWD2", fx_set_deaf_state_iwd2, 0, -1 }, //this is a modified version
	{ "State:Diseased", fx_set_diseased_state, 0, -1 },
	{ "State:Feeblemind", fx_set_feebleminded_state, 0, -1 },
	{ "State:Hasted", fx_set_hasted_state, 0, -1 },
	{ "State:Haste2", fx_set_hasted_state, 0, -1 },
	{ "State:Hold", fx_hold_creature, 0, -1 }, //175 (doesn't work in original iwd2)
	{ "State:Hold2", fx_hold_creature, 0, -1 },//185 (doesn't work in original iwd2)
	{ "State:Hold3", fx_hold_creature, 0, -1 },//109 iwd2
	{ "State:HoldNoIcon", fx_hold_creature_no_icon, 0, -1 }, //109
	{ "State:HoldNoIcon2", fx_hold_creature_no_icon, 0, -1 }, //0xfb (iwd/iwd2)
	{ "State:HoldNoIcon3", fx_hold_creature_no_icon, 0, -1 }, //0x1a8 (iwd2)
	{ "State:Imprisonment", fx_imprisonment, 0, -1 },
	{ "State:Infravision", fx_set_infravision_state, 0, -1 },
	{ "State:Invisible", fx_set_invisible_state, 0, -1 }, //both invis or improved invis
	{ "State:Nondetection", fx_set_nondetection_state, 0, -1 },
	{ "State:Panic", fx_set_panic_state, 0, -1 },
	{ "State:Petrification", fx_set_petrified_state, 0, -1 },
	{ "State:Poisoned", fx_set_poisoned_state, 0, -1 },
	{ "State:Regenerating", fx_set_regenerating_state, 0, -1 },
	{ "State:Silenced", fx_set_silenced_state, 0, -1 },
	{ "State:Helpless", fx_set_unconscious_state, 0, -1 },
	{ "State:Sleep", fx_set_unconscious_state, 0, -1 },
	{ "State:Slowed", fx_set_slowed_state, 0, -1 },
	{ "State:Stun", fx_set_stun_state, 0, -1 },
	{ "StealthModifier", fx_stealth_modifier, 0, -1 },
	{ "StoneSkinModifier", fx_stoneskin_modifier, 0, -1 },
	{ "StoneSkin2Modifier", fx_golem_stoneskin_modifier, 0, -1 },
	{ "StrengthModifier", fx_strength_modifier, 0, -1 },
	{ "StrengthBonusModifier", fx_strength_bonus_modifier, 0, -1 },
	{ "SummonCreature", fx_summon_creature, EFFECT_NO_ACTOR, -1 },
	{ "RandomTeleport", fx_teleport_field, 0, -1 },
	{ "TeleportToTarget", fx_teleport_to_target, 0, -1 },
	{ "TimelessState", fx_timeless_modifier, 0, -1 },
	{ "Timestop", fx_timestop, 0, -1 },
	{ "TitleModifier", fx_title_modifier, 0, -1 },
	{ "ToHitModifier", fx_to_hit_modifier, 0, -1 },
	{ "ToHitBonusModifier", fx_to_hit_bonus_modifier, 0, -1 },
	{ "ToHitVsCreature", fx_generic_effect, 0, -1 },
	{ "TrackingModifier", fx_tracking_modifier, 0, -1 },
	{ "TransparencyModifier", fx_transparency_modifier, 0, -1 },
	{ "TurnUndead", fx_turn_undead, 0, -1 },
	{ "Unknown", fx_unknown, EFFECT_NO_ACTOR, -1 },
	{ "Unlock", fx_knock, EFFECT_NO_ACTOR, -1 }, //open doors/containers
	{ "UnsummonCreature", fx_unsummon_creature, 0, -1 },
	{ "Variable:StoreLocalVariable", fx_local_variable, 0, -1 },
	{ "VisualAnimationEffect", fx_visual_animation_effect, 0, -1 }, //unknown
	{ "VisualRangeModifier", fx_visual_range_modifier, 0, -1 },
	{ "VisualSpellHit", fx_visual_spell_hit, 0, -1 },
	{ "WildSurgeModifier", fx_wild_surge_modifier, 0, -1 },
	{ "WingBuffet", fx_wing_buffet, 0, -1 },
	{ "WisdomModifier", fx_wisdom_modifier, 0, -1 },
	{ "WizardSpellSlotsModifier", fx_bonus_wizard_spells, 0, -1 },
	{ NULL, NULL, 0, 0 },
};

static void Cleanup()
{
	core->FreeResRefTable(casting_glows, cgcount);
	core->FreeResRefTable(spell_hits, shcount);
	if(spell_abilities) free(spell_abilities);
	spell_abilities=NULL;
	if(polymorph_stats) free(polymorph_stats);
	polymorph_stats=NULL;
}

void RegisterCoreOpcodes()
{
	core->RegisterOpcodes( sizeof( effectnames ) / sizeof( EffectDesc ) - 1, effectnames );
	enhanced_effects=core->HasFeature(GF_ENHANCED_EFFECTS);
	pstflags=core->HasFeature(GF_PST_STATE_FLAGS);
	default_spell_hit.SequenceFlags|=IE_VVC_BAM;
}


#define STONE_GRADIENT 14
#define ICE_GRADIENT 71

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

static inline void PlayRemoveEffect(const char *defsound, Actor *target, Effect* fx)
{
	core->GetAudioDrv()->Play(fx->Resource[0]?fx->Resource:defsound, target->Pos.x, target->Pos.y);
}

//whoseeswho:
#define ENEMY_SEES_ORIGIN 1
#define ORIGIN_SEES_ENEMY 2

inline Actor *GetNearestEnemyOf(Map *map, Actor *origin, int whoseeswho)
{
	//determining the allegiance of the origin
	int type = GetGroup(origin);

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

//resurrect code used in many places
void Resurrect(Scriptable *Owner, Actor *target, Effect *fx, Point &p)
{
	Scriptable *caster = GetCasterObject();
	if (!caster) {
		caster = Owner;
	}
	Map *area = caster->GetCurrentArea();

	if (area && target->GetCurrentArea()!=area) {
		MoveBetweenAreasCore(target, area->GetScriptName(), p, fx->Parameter2, true);
	}
	target->Resurrect();
}


// handles the percentage damage spread over time by converting it to absolute damage
inline void HandlePercentageDamage(Effect *fx, Actor *target) {
	if (fx->Parameter2 == RPD_PERCENT && fx->FirstApply) {
		// distribute the damage to one second intervals
		int seconds = (fx->Duration - core->GetGame()->GameTime) / AI_UPDATE_TIME;
		fx->Parameter1 = target->GetStat(IE_MAXHITPOINTS) * fx->Parameter1 / 100 / seconds;
	}
}
// Effect opcodes

// 0x00 ACVsDamageTypeModifier
int fx_ac_vs_damage_type_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_ac_vs_damage_type_modifier (%2d): AC Modif: %d ; Type: %d ; MinLevel: %d ; MaxLevel: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2, (int) fx->DiceSides, (int) fx->DiceThrown );
	//check level was pulled outside as a common functionality
	//CHECK_LEVEL();

	// it is a bitmask
	int type = fx->Parameter2;
	if (type == 0) {
		HandleBonus(target, IE_ARMORCLASS, fx->Parameter1, fx->TimingMode);
		return FX_PERMANENT;
	}

	//convert to signed so -1 doesn't turn to an astronomical number
	if (type == 16) {
		if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
			if ((signed)BASE_GET( IE_ARMORCLASS) > (signed)fx->Parameter1) {
				BASE_SET( IE_ARMORCLASS, fx->Parameter1 );
			}
		} else {
			if ((signed)STAT_GET( IE_ARMORCLASS) > (signed)fx->Parameter1) {
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
int fx_attacks_per_round_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_attacks_per_round_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
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
static EffectRef fx_set_sleep_state_ref = { "State:Helpless", -1 };
//this reference is used by many other effects
static EffectRef fx_display_portrait_icon_ref = { "Icon:Display", -1 };

int fx_cure_sleep_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_cure_sleep_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	BASE_STATE_CURE( STATE_SLEEP );
	target->fxqueue.RemoveAllEffects(fx_set_sleep_state_ref);
	target->fxqueue.RemoveAllEffectsWithParam(fx_display_portrait_icon_ref, PI_SLEEP);
	return FX_NOT_APPLIED;
}

// 0x03 State:Berserk
// this effect sets the STATE_BERSERK bit, but bg2 actually ignores the bit
int fx_set_berserk_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_berserk_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	// atleast how and bg2 allow this to only work on pcs
	if (!core->HasFeature(GF_3ED_RULES) && !target->InParty) {
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

// 0x04 Cure:Berserk
// this effect clears the STATE_BERSERK (2) bit, but bg2 actually ignores the bit
// it also removes effect 04
static EffectRef fx_set_berserk_state_ref = { "State:Berserk", -1 };

int fx_cure_berserk_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_cure_berserk_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	BASE_STATE_CURE( STATE_BERSERK );
	target->fxqueue.RemoveAllEffects(fx_set_berserk_state_ref);
	return FX_NOT_APPLIED;
}

// 0x05 State:Charmed
// 0xf1 ControlCreature (iwd2)
int fx_set_charmed_state (Scriptable* Owner, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_charmed_state (%2d): General: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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

	bool playercharmed;
	bool casterenemy;
	if (fx->FirstApply) {
		//when charmed, the target forgets its current action
		target->ClearActions();

		Scriptable *caster = GetCasterObject();
		if (!caster) caster = Owner;
		if (caster->Type==ST_ACTOR) {
			casterenemy = ((Actor *) caster)->GetStat(IE_EA)>EA_GOODCUTOFF; //or evilcutoff?
		} else {
			casterenemy = target->GetStat(IE_EA)>EA_GOODCUTOFF;
		}
		fx->DiceThrown=casterenemy;

		playercharmed = target->InParty;
		fx->DiceSides = playercharmed;
	} else {
		casterenemy = fx->DiceThrown;
		playercharmed = fx->DiceSides;
	}


	switch (fx->Parameter2) {
	case 0: //charmed (target neutral after charm)
		if (fx->FirstApply) {
			displaymsg->DisplayConstantStringName(STR_CHARMED, DMC_WHITE, target);
		}
	case 1000:
		break;
	case 1: //charmed (target hostile after charm)
		if (fx->FirstApply) {
			displaymsg->DisplayConstantStringName(STR_CHARMED, DMC_WHITE, target);
		}
	case 1001:
		if (!target->InParty) {
			target->SetBaseNoPCF(IE_EA, EA_ENEMY);
		}
		break;
	case 2: //dire charmed (target neutral after charm)
		if (fx->FirstApply) {
			displaymsg->DisplayConstantStringName(STR_DIRECHARMED, DMC_WHITE, target);
		}
	case 1002:
		break;
	case 3: //dire charmed (target hostile after charm)
		if (fx->FirstApply) {
			displaymsg->DisplayConstantStringName(STR_DIRECHARMED, DMC_WHITE, target);
		}
	case 1003:
		if (!target->InParty) {
			target->SetBaseNoPCF(IE_EA, EA_ENEMY);
		}
		break;
	case 4: //controlled by cleric
		if (fx->FirstApply) {
			displaymsg->DisplayConstantStringName(STR_CONTROLLED, DMC_WHITE, target);
		}
	case 1004:
		if (!target->InParty) {
			target->SetBaseNoPCF(IE_EA, EA_ENEMY);
		}
		target->SetSpellState(SS_DOMINATION);
		break;
	case 5: //thrall (typo comes from original engine doc)
		if (fx->FirstApply) {
			displaymsg->DisplayConstantStringName(STR_CHARMED, DMC_WHITE, target);
		}
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
	if (0) print( "fx_charisma_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	if (0) print( "fx_set_color_gradient (%2d): Gradient: %d, Location: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	EffectQueue::HackColorEffects(target, fx);
	target->SetColor( fx->Parameter2, fx->Parameter1 );
	return FX_APPLIED;
}

// 08 Color:SetRGB
int fx_set_color_rgb (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_color_rgb (%2d): RGB: %x, Location: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	EffectQueue::HackColorEffects(target, fx);
	ieDword location = fx->Parameter2 & 0xff;
	target->SetColorMod(location, RGBModifier::ADD, -1, fx->Parameter1 >> 8,
			fx->Parameter1 >> 16, fx->Parameter1 >> 24);

	return FX_APPLIED;
}
// 08 Color:SetRGBGlobal
int fx_set_color_rgb_global (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_color_rgb_global (%2d): RGB: %x, Location: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->SetColorMod(0xff, RGBModifier::ADD, -1, fx->Parameter1 >> 8,
			fx->Parameter1 >> 16, fx->Parameter1 >> 24);

	return FX_APPLIED;
}

// 09 Color:PulseRGB
int fx_set_color_pulse_rgb (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_color_pulse_rgb (%2d): RGB: %x, Location: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	EffectQueue::HackColorEffects(target, fx);
	ieDword location = fx->Parameter2 & 0xff;
	int speed = (fx->Parameter2 >> 16) & 0xFF;
	target->SetColorMod(location, RGBModifier::ADD, speed,
			fx->Parameter1 >> 8, fx->Parameter1 >> 16,
			fx->Parameter1 >> 24);

	return FX_APPLIED;
}

// 09 Color:PulseRGBGlobal (pst variant)
int fx_set_color_pulse_rgb_global (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_color_pulse_rgb_global (%2d): RGB: %x, Location: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	int speed = (fx->Parameter2 >> 16) & 0xFF;
	target->SetColorMod(0xff, RGBModifier::ADD, speed,
			fx->Parameter1 >> 8, fx->Parameter1 >> 16,
			fx->Parameter1 >> 24);

	return FX_APPLIED;
}

// 0x0A ConstitutionModifier
int fx_constitution_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_constitution_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD( IE_CON );
	} else {
		STAT_MOD( IE_CON );
	}
	return FX_PERMANENT;
}

// 0x0B Cure:Poison
static EffectRef fx_poisoned_state_ref = { "State:Poisoned", -1 };

int fx_cure_poisoned_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_cure_poisoned_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
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
	if (0) print( "fx_damage (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//save for half damage type
	ieDword damagetype = fx->Parameter2>>16;
	ieDword modtype = fx->Parameter2&3;
	if (modtype==3) {
		modtype&=~3;
	}
	Scriptable *caster = GetCasterObject();

	// gemrb extension
	if (fx->Parameter3) {
		if(caster && caster->Type==ST_ACTOR) {
			target->AddTrigger(TriggerEntry(trigger_hitby, caster->GetGlobalID()));
			target->LastHitter=caster->GetGlobalID();
		} else {
			//Maybe it should be something impossible like 0xffff, and use 'Someone'
			printMessage("Actor", "LastHitter (type %d) falling back to target: %s.\n", RED, caster ? caster->Type : -1, target->GetName(1));
			target->LastHitter=target->GetGlobalID();
		}
	}

	target->Damage(fx->Parameter1, damagetype, caster, modtype, fx->IsVariable);
	//this effect doesn't stick
	return FX_NOT_APPLIED;
}

// 0x0d Death
static EffectRef fx_death_ward_ref = { "DeathWard", -1 };
static EffectRef fx_death_magic_ref = { "Death2", -1 };

int fx_death (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_death (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
		BASE_STATE_SET(STATE_FLAME); //not sure, should be charred
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
	//these two bits are turned off on death
	BASE_STATE_CURE(STATE_FROZEN|STATE_PETRIFIED);

	Scriptable *killer = GetCasterObject();
	target->Damage(0, damagetype, killer);
	//death has damage type too
	target->Die(killer);
	//this effect doesn't stick
	return FX_NOT_APPLIED;
}

// 0xE Cure:Defrost
int fx_cure_frozen_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_cure_frozen_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
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

int fx_dexterity_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_dexterity_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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

static EffectRef fx_set_slow_state_ref = { "State:Slowed", -1 };
// 0x10 State:Hasted
// this function removes slowed state, or sets hasted state
int fx_set_hasted_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_hasted_state (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
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
		STAT_ADD(IE_NUMBEROFATTACKS, 2);
		// -2 initiative bonus
		STAT_ADD(IE_PHYSICALSPEED, 2);
		break;
	case 1://improved haste
		target->AddPortraitIcon(PI_IMPROVEDHASTE);
		STAT_SET(IE_IMPROVEDHASTE,1);
		STAT_SET(IE_ATTACKNUMBERDOUBLE,0);
		target->NewStat(IE_NUMBEROFATTACKS, 200, MOD_PERCENT);
		// -2 initiative bonus
		STAT_ADD(IE_PHYSICALSPEED, 2);
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
int fx_current_hp_modifier (Scriptable* Owner, Actor* target, Effect* fx)
{
	if (0) print( "fx_current_hp_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	if (fx->Parameter2&0x10000) {
		Point p(fx->PosX, fx->PosY);
		Resurrect(Owner, target, fx, p);
	}
	if (fx->Parameter2&0x20000) {
		target->fxqueue.RemoveAllNonPermanentEffects();
	}
	//Cannot heal bloodrage
	if (target->HasSpellState(SS_BLOODRAGE)) {
		return FX_NOT_APPLIED;
	}

	//current hp percent is relative to modified max hp
	switch(fx->Parameter2&0xffff) {
	case MOD_ADDITIVE:
	case MOD_ABSOLUTE:
		target->NewBase( IE_HITPOINTS, fx->Parameter1, fx->Parameter2&0xffff);
		break;
	case MOD_PERCENT:
		target->NewBase( IE_HITPOINTS, target->GetSafeStat(IE_MAXHITPOINTS)*fx->Parameter1/100, MOD_ABSOLUTE);
	}
	//never stay permanent
	return FX_NOT_APPLIED;
}

// 0x12 MaximumHPModifier
// 0 and 3 differ in that 3 doesn't modify current hitpoints
// 1,4 and 2,5 are analogous to them, but with different modifiers
int fx_maximum_hp_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_maximum_hp_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	if (0) print( "fx_intelligence_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	case 0:
		if (pstflags) {
			STATE_SET( STATE_PST_INVIS );
		} else {
			STATE_SET( STATE_INVISIBLE );
		}
		STAT_ADD(IE_TOHIT, 4);
		break;
	case 1:
		STATE_SET( STATE_INVIS2 );
		HandleBonus(target, IE_ARMORCLASS, 4, fx->TimingMode);
		break;
	default:
		break;
	}
	ieDword Trans = fx->Parameter4;
	if (fx->Parameter3) {
		if (Trans>=240) {
			fx->Parameter3 = 0;
		} else {
			Trans+=16;
		}
	} else {
		if (Trans<=32) {
			fx->Parameter3 = 1;
		} else {
			Trans-=16;
		}
	}
	fx->Parameter4 = Trans;
	STAT_SET( IE_TRANSLUCENT, Trans);
	//FIXME: probably FX_PERMANENT, but TRANSLUCENT has no saved base stat
	return FX_APPLIED;
}

// 0x15 LoreModifier
int fx_lore_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_lore_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD( IE_LORE );
	} else {
		STAT_MOD( IE_LORE );
	}
	return FX_PERMANENT;
}

// 0x16 LuckModifier
int fx_luck_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_luck_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	if (0) print( "fx_morale_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	//FIXME: in bg2 this is hacked to set param1=10, param2=1, we might need some flag for this
	STAT_MOD( IE_MORALE );
	return FX_APPLIED;
}

// 0x18 State:Panic
int fx_set_panic_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_panic_state (%2d)\n", fx->Opcode );

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
int fx_set_poisoned_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_poisoned_state (%2d): Damage: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	int count = target->fxqueue.CountEffects(fx_poisoned_state_ref, fx->Parameter1, fx->Parameter2, fx->Resource);
	if (count > 1) {
		return FX_APPLIED;
	}

	STATE_SET( STATE_POISONED );

	ieDword damage;
	int tmp = fx->Parameter1;

	HandlePercentageDamage(fx, target);

	switch(fx->Parameter2) {
	case RPD_ROUNDS:
		tmp *= core->Time.round_sec;
		damage = 1;
		break;
	case RPD_TURNS:
		tmp *= core->Time.turn_sec;
		damage = 1;
		break;
	case RPD_SECONDS:
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
			displaymsg->DisplayConstantStringName(STR_HELD, DMC_WHITE, target);
		}
		break;
	case RPD_7:
		break;
	case RPD_ENVENOM:
		break;
	default:
		tmp = 1;
		damage = 1;
		break;
	}

	// all damage is at most per-second
	tmp *= AI_UPDATE_TIME;
	if (tmp && (core->GetGame()->GameTime%tmp)) {
		return FX_APPLIED;
	}

	Scriptable *caster = GetCasterObject();
	//percent
	target->Damage(damage, DAMAGE_POISON, caster);
	return FX_APPLIED;
}

// 0x1a RemoveCurse
static EffectRef fx_apply_effect_curse_ref = { "ApplyEffectCurse", -1 };
static EffectRef fx_pst_jumble_curse_ref = { "JumbleCurse", -1 };

// gemrb extension: if the resource field is filled, it will remove curse only from the specified item
int fx_remove_curse (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_remove_curse (%2d): Resource: %s Type: %d\n", fx->Opcode, fx->Resource, fx->Parameter2 );

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
int fx_acid_resistance_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_acid_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	if (0) print( "fx_cold_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	if (0) print( "fx_electricity_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	if (0) print( "fx_fire_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	if (0) print( "fx_magic_damage_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	//This stat has no saved basestat variant, so this effect is always stored (not FX_PERMANENT)
	STAT_MOD( IE_MAGICDAMAGERESISTANCE );
	return FX_APPLIED;
}

// 0x20 Cure:Death
int fx_cure_dead_state (Scriptable* Owner, Actor* target, Effect* fx)
{
	if (0) print( "fx_cure_dead_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//call this only if the target is dead, otherwise some variables can get wrong
	if (STATE_GET(STATE_DEAD) ) {
		Point p(fx->PosX, fx->PosY);
		Resurrect(Owner, target, fx, p);
	}
	return FX_NOT_APPLIED;
}

// 0x21 SaveVsDeathModifier
int fx_save_vs_death_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_save_vs_death_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	HandleBonus( target, IE_SAVEVSDEATH, fx->Parameter1, fx->TimingMode );
	return FX_PERMANENT;
}

// 0x22 SaveVsWandsModifier
int fx_save_vs_wands_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_save_vs_wands_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	HandleBonus( target, IE_SAVEVSWANDS, fx->Parameter1, fx->TimingMode );
	return FX_PERMANENT;
}

// 0x23 SaveVsPolyModifier
int fx_save_vs_poly_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_save_vs_poly_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	HandleBonus( target, IE_SAVEVSPOLY, fx->Parameter1, fx->TimingMode );
	return FX_PERMANENT;
}

// 0x24 SaveVsBreathModifier
int fx_save_vs_breath_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_save_vs_breath_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	HandleBonus( target, IE_SAVEVSBREATH, fx->Parameter1, fx->TimingMode );
	return FX_PERMANENT;
}

// 0x25 SaveVsSpellsModifier
int fx_save_vs_spell_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_save_vs_spell_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	HandleBonus( target, IE_SAVEVSSPELL, fx->Parameter1, fx->TimingMode );
	return FX_PERMANENT;
}

// 0x26 State:Silenced
int fx_set_silenced_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_silenced_state (%2d)\n", fx->Opcode );
	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_STATE_SET( STATE_SILENCED );
	} else {
		STATE_SET( STATE_SILENCED );
	}
	return FX_PERMANENT;
}

static EffectRef fx_animation_stance_ref = { "AnimationStateChange", -1 };

// 0x27 State:Helpless
// this effect sets both bits, but 'awaken' only removes the sleep bit
// FIXME: this is probably a persistent effect
int fx_set_unconscious_state (Scriptable* Owner, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_unconscious_state (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );

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
	target->InterruptCasting = true;
	return FX_PERMANENT;
}

// 0x28 State:Slowed
// this function removes hasted state, or sets slowed state
static EffectRef fx_set_haste_state_ref = { "State:Hasted", -1 };

int fx_set_slowed_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_slowed_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
		// halve apr and speed
		STAT_MUL(IE_NUMBEROFATTACKS, 50);
		STAT_MUL(IE_MOVEMENTRATE, 50);
	}
	return FX_PERMANENT;
}

// 0x29 Sparkle
int fx_sparkle (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_sparkle (%2d): Sparkle colour: %d ; Sparkle type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	if (!target) {
		return FX_NOT_APPLIED;
	}

	Map *map = target->GetCurrentArea();
	if (!map) {
		return FX_APPLIED;
	}
	Point p(fx->PosX, fx->PosY);

	map->Sparkle( fx->Duration, fx->Parameter1, fx->Parameter2, p, fx->Parameter3);
	return FX_NOT_APPLIED;
}

// 0x2A WizardSpellSlotsModifier
int fx_bonus_wizard_spells (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_bonus_wizard_spells (%2d): Spell Add: %d ; Spell Level: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
int fx_cure_petrified_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_cure_petrified_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	BASE_STATE_CURE( STATE_PETRIFIED );
	return FX_NOT_APPLIED;
}

// 0x2C StrengthModifier
int fx_strength_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_strength_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	////how strength: value is based on class
	////pst power of one also depends on this!
	if (fx->Parameter2==3) {
		//TODO: strextra for stats>=18
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
	int hp = BASE_GET(IE_HITPOINTS);
	if (hp>150) return FX_NOT_APPLIED;
	int stuntime;
	if (hp>100) stuntime = core->Roll(1,4,0);
	else if (hp>50) stuntime = core->Roll(2,4,0);
	else stuntime = core->Roll(4,4,0);
	fx->Parameter2 = 0;
	fx->TimingMode = FX_DURATION_ABSOLUTE;
	fx->Duration = stuntime*6*core->Time.round_size + core->GetGame()->GameTime;
	STATE_SET( STATE_STUNNED );
	target->AddPortraitIcon(PI_STUN);
	return FX_APPLIED;
}

int fx_set_stun_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_stun_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	//actually the original engine just skips this effect if the target is dead
	if ( STATE_GET(STATE_DEAD) ) {
		return FX_NOT_APPLIED;
	}

	//this is an IWD extension
	if (target->HasSpellState(SS_BLOODRAGE)) {
		return FX_NOT_APPLIED;
	}

	if (fx->Parameter2==2) {
		//don't reroll the duration next time we get here
		if (fx->FirstApply) {
			return power_word_stun_iwd2(target, fx);
		}
	}
	STATE_SET( STATE_STUNNED );
	target->AddPortraitIcon(PI_STUN);
	if (fx->Parameter2==1) {
		target->SetSpellState(SS_AWAKE);
	}
	return FX_APPLIED;
}

// 0x2E Cure:Stun
static EffectRef fx_set_stun_state_ref = { "State:Stun", -1 };
static EffectRef fx_hold_creature_no_icon_ref = { "State:HoldNoIcon", -1 };

int fx_cure_stun_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_cure_stun_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	BASE_STATE_CURE( STATE_STUNNED );
	target->fxqueue.RemoveAllEffects(fx_set_stun_state_ref);
	target->fxqueue.RemoveAllEffects(fx_hold_creature_no_icon_ref);
	target->fxqueue.RemoveAllEffectsWithParam(fx_display_portrait_icon_ref, PI_HELD);
	target->fxqueue.RemoveAllEffectsWithParam(fx_display_portrait_icon_ref, PI_HOPELESS);
	return FX_NOT_APPLIED;
}

// 0x2F Cure:Invisible
// 0x74 Cure:Invisible2
static EffectRef fx_set_invisible_state_ref = { "State:Invisible", -1 };

int fx_cure_invisible_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_cure_invisible_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	if (!STATE_GET(STATE_NONDET)) {
		if (pstflags) {
			BASE_STATE_CURE( STATE_PST_INVIS );
		} else {
			BASE_STATE_CURE( STATE_INVISIBLE | STATE_INVIS2 );
		}
		target->fxqueue.RemoveAllEffects(fx_set_invisible_state_ref);
	}
	return FX_NOT_APPLIED;
}

// 0x30 Cure:Silence
static EffectRef fx_set_silenced_state_ref = { "State:Silenced", -1 };

int fx_cure_silenced_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_cure_silenced_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	BASE_STATE_CURE( STATE_SILENCED );
	target->fxqueue.RemoveAllEffects(fx_set_silenced_state_ref);
	return FX_NOT_APPLIED;
}

// 0x31 WisdomModifier
int fx_wisdom_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_wisdom_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	if (0) print( "fx_brief_rgb (%2d): RGB: %d, Location and speed: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	int speed = (fx->Parameter2 >> 16) & 0xff;
	target->SetColorMod(0xff, RGBModifier::ADD, speed,
			fx->Parameter1 >> 8, fx->Parameter1 >> 16,
			fx->Parameter1 >> 24, 0);

	return FX_NOT_APPLIED;
}

// 0x33 Color:DarkenRGB
int fx_darken_rgb (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_darken_rgb (%2d): RGB: %d, Location and speed: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	EffectQueue::HackColorEffects(target, fx);
	ieDword location = fx->Parameter2 & 0xff;
	target->SetColorMod(location, RGBModifier::TINT, -1, fx->Parameter1 >> 8,
			fx->Parameter1 >> 16, fx->Parameter1 >> 24);
	return FX_APPLIED;
}

// 0x34 Color:GlowRGB
int fx_glow_rgb (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_glow_rgb (%2d): RGB: %d, Location and speed: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	EffectQueue::HackColorEffects(target, fx);
	ieDword location = fx->Parameter2 & 0xff;
	target->SetColorMod(location, RGBModifier::BRIGHTEN, -1,
			fx->Parameter1 >> 8, fx->Parameter1 >> 16,
			fx->Parameter1 >> 24);

	return FX_APPLIED;
}

// 0x35 AnimationIDModifier
static EffectRef fx_animation_id_modifier_ref = { "AnimationIDModifier", -1 };

int fx_animation_id_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_animation_id_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	switch (fx->Parameter2) {
	case 0: //non permanent animation change
	default:
		STAT_SET( IE_ANIMATION_ID, fx->Parameter1 );
		return FX_APPLIED;
	case 1: //remove any non permanent change
		target->fxqueue.RemoveAllEffects(fx_animation_id_modifier_ref);
		//return FX_NOT_APPLIED;
		//intentionally passing through (perma change removes previous changes)
	case 2: //permanent animation id change
		//FIXME: Why no PCF here? (Avenger)
		target->SetBaseNoPCF(IE_ANIMATION_ID, fx->Parameter1);
		return FX_NOT_APPLIED;
	}
}

// 0x36 ToHitModifier
int fx_to_hit_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_to_hit_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	HandleBonus( target, IE_TOHIT, fx->Parameter1, fx->TimingMode );
	return FX_APPLIED;
}

// 0x37 KillCreatureType
static EffectRef fx_death_ref = { "Death", -1 };

int fx_kill_creature_type (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_kill_creature_type (%2d): Value: %d, IDS: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
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
int fx_alignment_invert (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_alignment_invert (%2d)\n", fx->Opcode );
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
int fx_alignment_change (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_alignment_change (%2d): Value: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_SET( IE_ALIGNMENT, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x3a DispelEffects
int fx_dispel_effects (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_dispel_effects (%2d): Value: %d, IDS: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	ieResRef Removed;
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
	target->fxqueue.RemoveLevelEffects(Removed, level, RL_DISPELLABLE, 0);
	return FX_NOT_APPLIED;
}

// 0x3B StealthModifier
int fx_stealth_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_stealth_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_STEALTH );
	return FX_APPLIED;
}

// 0x3C MiscastMagicModifier
int fx_miscast_magic_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_miscast_magic_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
int fx_alchemy_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_alchemy_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	if (0) print( "fx_bonus_priest_spells (%2d): Spell Add: %d ; Spell Level: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	if (0) print( "fx_set_infravision_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_STATE_SET( STATE_INFRA );
	} else {
		STATE_SET( STATE_INFRA );
	}
	return FX_PERMANENT;
}

// 0x40 Cure:Infravision
static EffectRef fx_set_infravision_state_ref = { "State:Infravision", -1 };

int fx_cure_infravision_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_cure_infravision_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	BASE_STATE_CURE( STATE_INFRA );
	target->fxqueue.RemoveAllEffects(fx_set_infravision_state_ref);
	return FX_NOT_APPLIED;
}

// 0x41 State:Blur
int fx_set_blur_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_blur_state (%2d)\n", fx->Opcode );
	//death stops this effect
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
int fx_transparency_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_transparency_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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

// 0x43 SummonCreature

static int eamods[]={EAM_ALLY,EAM_ALLY,EAM_DEFAULT,EAM_ALLY,EAM_DEFAULT,EAM_ENEMY,EAM_ALLY};

int fx_summon_creature (Scriptable* Owner, Actor* target, Effect* fx)
{
	if (0) print( "fx_summon_creature (%2d): ResRef:%s Anim:%s Type: %d\n", fx->Opcode, fx->Resource, fx->Resource2, fx->Parameter2 );

	//summon creature (resource), play vvc (resource2)
	//creature's lastsummoner is Owner
	//creature's target is target
	//position of appearance is target's pos
	int eamod = -1;
	if (fx->Parameter2<6){
		eamod = eamods[fx->Parameter2];
	}

	//the monster should appear near the effect position
	Point p(fx->PosX, fx->PosY);

	Effect *newfx = EffectQueue::CreateUnsummonEffect(fx);
	core->SummonCreature(fx->Resource, fx->Resource2, Owner, target, p, eamod, 0, newfx);
	delete newfx;
	return FX_NOT_APPLIED;
}

// 0x44 UnsummonCreature
int fx_unsummon_creature (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_unsummon_creature (%2d)\n", fx->Opcode );

	//to be compatible with the original engine, unsummon doesn't work with PC's
	//but it works on anything else
	Map *area = target->GetCurrentArea();
	if (!target->InParty && area) {
		//play the vanish animation
		ScriptedAnimation* sca = gamedata->GetScriptedAnimation(fx->Resource, false);
		if (sca) {
			sca->XPos+=target->Pos.x;
			sca->YPos+=target->Pos.y;
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
	if (0) print( "fx_set_nondetection_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_STATE_SET( STATE_NONDET );
	} else {
		STATE_SET( STATE_NONDET );
	}
	return FX_PERMANENT;
}

// 0x46 Cure:Nondetection
static EffectRef fx_set_nondetection_state_ref = { "State:Nondetection", -1 };

int fx_cure_nondetection_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_cure_nondetection_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	BASE_STATE_CURE( STATE_NONDET );
	target->fxqueue.RemoveAllEffects(fx_set_nondetection_state_ref);
	return FX_NOT_APPLIED;
}

// 0x47 SexModifier
int fx_sex_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_sex_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
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
	if (0) print( "fx_ids_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	bool permanent = fx->TimingMode==FX_DURATION_INSTANT_PERMANENT;
	switch (fx->Parameter2) {
	case 0:
		if (permanent) {
			BASE_SET(IE_EA, fx->Parameter1);
		} else {
			STAT_SET(IE_EA, fx->Parameter1);
		}
		break;
	case 1:
		if (permanent) {
			BASE_SET(IE_GENERAL, fx->Parameter1);
		} else {
			STAT_SET(IE_GENERAL, fx->Parameter1);
		}
		break;
	case 2:
		if (permanent) {
			BASE_SET(IE_RACE, fx->Parameter1);
		} else {
			STAT_SET(IE_RACE, fx->Parameter1);
		}
		break;
	case 3:
		if (permanent) {
			BASE_SET(IE_CLASS, fx->Parameter1);
		} else {
			STAT_SET(IE_CLASS, fx->Parameter1);
		}
		break;
	case 4:
		if (permanent) {
			BASE_SET(IE_SPECIFIC, fx->Parameter1);
		} else {
			STAT_SET(IE_SPECIFIC, fx->Parameter1);
		}
		break;
	case 5:
		if (permanent) {
			BASE_SET(IE_SEX, fx->Parameter1);
		} else {
			STAT_SET(IE_SPECIFIC, fx->Parameter1);
		}
		break;
	case 6:
		if (permanent) {
			BASE_SET(IE_ALIGNMENT, fx->Parameter1);
		} else {
			STAT_SET(IE_ALIGNMENT, fx->Parameter1);
		}
		break;
	default:
		return FX_NOT_APPLIED;
	}
	return FX_PERMANENT;
}

// 0x49 DamageBonusModifier
int fx_damage_bonus_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_damage_bonus_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_DAMAGEBONUS );
	//the basestat is not saved, so no FX_PERMANENT
	return FX_APPLIED;
}

// 0x4a State:Blind
int fx_set_blind_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_blind_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	if (!STATE_GET(STATE_BLIND)) {
		STATE_SET( STATE_BLIND );
		//the feat normally exists only in IWD2, but won't hurt
		if (!target->GetFeat(FEAT_BLIND_FIGHT)) {
			if (core->HasFeature(GF_REVERSE_TOHIT)) {
				STAT_ADD (IE_TOHIT, 10); // all other tohit stats are treated as bonuses
			} else {
				STAT_SUB (IE_ARMORCLASS, 2);
				// TODO: 50% inherent miss chance (full concealment), no dexterity bonus to AC (flatfooted)
				STAT_SUB (IE_TOHIT, 5);
			}
		}
	}
	//this should be FX_PERMANENT, but the current code is a mess here. Review after cleaned up
	return FX_APPLIED;
}

// 0x4b Cure:Blind
static EffectRef fx_set_blind_state_ref = { "State:Blind", -1 };

int fx_cure_blind_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_cure_blind_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	BASE_STATE_CURE( STATE_BLIND );
	target->fxqueue.RemoveAllEffects(fx_set_blind_state_ref);
	return FX_NOT_APPLIED;
}

// 0x4c State:Feeblemind
int fx_set_feebleminded_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_feebleminded_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_SET( STATE_FEEBLE );
	STAT_SET( IE_INT, 3);
	if (enhanced_effects) {
		target->AddPortraitIcon(PI_FEEBLEMIND);
	}
	//This state is better off with always stored, because of the portrait icon and the int stat
	//it wouldn't be easily cured if it would go away after irrevocably altering another stat
	return FX_APPLIED;
}

// 0x4d Cure:Feeblemind
static EffectRef fx_set_feebleminded_state_ref = { "State:Feeblemind", -1 };

int fx_cure_feebleminded_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_cure_feebleminded_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	BASE_STATE_CURE( STATE_FEEBLE );
	target->fxqueue.RemoveAllEffects(fx_set_feebleminded_state_ref);
	target->fxqueue.RemoveAllEffectsWithParam(fx_display_portrait_icon_ref, PI_FEEBLEMIND);
	return FX_NOT_APPLIED;
}

// 0x4e State:Diseased
static EffectRef fx_diseased_state_ref = { "State:Diseased", -1 };
int fx_set_diseased_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_diseased_state (%2d): Damage: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	if (STATE_GET(STATE_DEAD|STATE_PETRIFIED|STATE_FROZEN) ) {
		return FX_NOT_APPLIED;
	}

	int count = target->fxqueue.CountEffects(fx_diseased_state_ref, fx->Parameter1, fx->Parameter2, fx->Resource);
	if (count > 1) {
		return FX_APPLIED;
	}

	//setting damage to 0 because not all types do damage
	ieDword damage = 0;

	HandlePercentageDamage(fx, target);

	switch(fx->Parameter2) {
	case RPD_SECONDS:
		damage = 1;
		if (fx->Parameter1 && (core->GetGame()->GameTime%(fx->Parameter1*AI_UPDATE_TIME))) {
			return FX_APPLIED;
		}
		break;
	case RPD_PERCENT: // handled in HandlePercentageDamage
	case RPD_POINTS:
		damage = fx->Parameter1;
		// per second
		if (core->GetGame()->GameTime%AI_UPDATE_TIME) {
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
		STAT_SUB(IE_STR, 2);
		STAT_SUB(IE_DEX, 2);
		STAT_SUB(IE_CHR, 2);
		//falling through
	case RPD_SLOW: //slow
		//TODO: in iwd2
		//-2 AC, BaB, reflex, damage
		//-1 attack#
		//speed halved
		//in bg2
		//TBD
		target->AddPortraitIcon(PI_SLOWED);
		break;
	case RPD_MOLD: //mold touch (how)
		EXTSTATE_SET(EXTSTATE_MOLD);
		target->SetSpellState(SS_MOLDTOUCH);
		damage = 1;
		break;
	case RPD_MOLD2:
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
		target->Damage(damage, DAMAGE_POISON, caster);
	}

	return FX_APPLIED;
}


// 0x4f Cure:Disease
int fx_cure_diseased_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_cure_diseased_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//STATE_CURE( STATE_DISEASED ); //the bit flagged as disease is actually the active state. so this is even more unlikely to be used as advertised
	target->fxqueue.RemoveAllEffects( fx_diseased_state_ref ); //this is what actually happens in bg2
	return FX_NOT_APPLIED;
}

// 0x50 State:Deafness
int fx_set_deaf_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_deaf_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	//adopted IWD2 method, spellfailure will be handled internally based on the spell state
	if (target->SetSpellState(SS_DEAF)) return FX_APPLIED;

	EXTSTATE_SET(EXTSTATE_DEAF); //iwd1/how needs this
	if (enhanced_effects) {
		target->AddPortraitIcon(PI_DEAFNESS);
	}
	return FX_APPLIED;
}

int fx_set_deaf_state_iwd2 (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_deaf_state_iwd2 (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
static EffectRef fx_deaf_state_ref = { "State:Deafness", -1 };
static EffectRef fx_deaf_state_iwd2_ref = { "State:DeafnessIWD2", -1 };

//removes the deafness effect
int fx_cure_deaf_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_cure_deaf_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->fxqueue.RemoveAllEffects(fx_deaf_state_ref);
	target->fxqueue.RemoveAllEffects(fx_deaf_state_iwd2_ref);
	return FX_NOT_APPLIED;
}

// 0x52 SetAIScript
int fx_set_ai_script (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_ai_state (%2d): Resource: %s, Type: %d\n", fx->Opcode, fx->Resource, fx->Parameter2 );
	target->SetScript (fx->Resource, fx->Parameter2);
	return FX_NOT_APPLIED;
}

// 0x53 Protection:Projectile
int fx_protection_from_projectile (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_protection_from_projectile (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_BIT_OR( IE_IMMUNITY, IMM_PROJECTILE);
	return FX_APPLIED;
}

// 0x54 MagicalFireResistanceModifier
int fx_magical_fire_resistance_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_magical_fire_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	if (0) print( "fx_magical_cold_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	if (0) print( "fx_slashing_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		STAT_MOD( IE_RESISTSLASHING );
	} else {
		STAT_MOD( IE_RESISTSLASHING );
	}
	return FX_PERMANENT;
}

// 0x57 CrushingResistanceModifier
int fx_crushing_resistance_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_crushing_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	if (0) print( "fx_piercing_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	if (0) print( "fx_missiles_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	if (0) print( "fx_open_locks_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	if (0) print( "fx_find_traps_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	if (0) print( "fx_pick_pockets_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	if (0) print( "fx_fatigue_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	if (0) print( "fx_intoxication_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	if (0) print( "fx_tracking_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	if (0) print( "fx_level_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_LEVEL );
	//While the original can, i would rather not modify the base stat here...
	return FX_APPLIED;
}

// 0x61 StrengthBonusModifier
int fx_strength_bonus_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_strength_bonus_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	if (0) print( "fx_set_regenerating_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	int damage;
	int tmp = fx->Parameter1;
	ieDword gameTime = core->GetGame()->GameTime;

	if (fx->FirstApply) {
		//ensure we prepare Parameter3 now
	} else {
		//we can have multiple calls at the same gameTime, so we
		//just go to gameTime+1 to ensure one call
		ieDword nextHeal = fx->Parameter3;
		if (nextHeal>=gameTime) return FX_APPLIED;
	}

	HandlePercentageDamage(fx, target);

	switch(fx->Parameter2) {
	case RPD_TURNS:		//restore param3 hp every param1 turns
		tmp *= core->Time.rounds_per_turn;
		//fall
	case RPD_ROUNDS:	//restore param3 hp every param1 rounds
		tmp *= core->Time.round_sec;
		//fall
	case RPD_SECONDS:	//restore param3 hp every param1 seconds
		fx->Parameter3 = gameTime + tmp*AI_UPDATE_TIME;
		damage = 1;
		break;
	case RPD_PERCENT: // handled in HandlePercentageDamage
	case RPD_POINTS:	//restore param1 hp every second? that's crazy!
		damage = fx->Parameter1;
		fx->Parameter3 = gameTime + AI_UPDATE_TIME;
		break;
	default:
		fx->Parameter3 = gameTime + AI_UPDATE_TIME;
		damage = 1;
		break;
	}

	if (fx->FirstApply) {
		//don't add hp in the first occasion, so it cannot be used for cheat heals
		return FX_APPLIED;
	}

	target->NewBase(IE_HITPOINTS, damage, MOD_ADDITIVE);
	return FX_APPLIED;
}
// 0x63 SpellDurationModifier
int fx_spell_duration_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_spell_duration_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
int fx_generic_effect (Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) print( "fx_generic_effect (%2d): Param1: %d, Param2: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x65 Protection:Opcode
int fx_protection_opcode (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_protection_opcode (%2d): Opcode: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_BIT_OR(IE_IMMUNITY, IMM_OPCODE);
	return FX_APPLIED;
}

// 0x66 Protection:SpellLevel
int fx_protection_spelllevel (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_protection_spelllevel (%2d) Level: %d\n", fx->Opcode, fx->Parameter1);

	int value = fx->Parameter1;
	if (value<9) {
		STAT_BIT_OR(IE_MINORGLOBE, 1<<value);
		STAT_BIT_OR(IE_IMMUNITY, IMM_LEVEL);
		return FX_APPLIED;
	}
	return FX_NOT_APPLIED;
}

// 0x67 ChangeName
int fx_change_name (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_change_name_modifier (%2d): StrRef: %d\n", fx->Opcode, fx->Parameter1 );
	//this also changes the base stat
	target->SetName(fx->Parameter1, 0);
	return FX_NOT_APPLIED;
}

// 0x68 ExperienceModifier
int fx_experience_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_experience_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//FIXME: this has mode too
	//target->AddExperience (fx->Parameter1);
	STAT_MOD( IE_XP );
	return FX_NOT_APPLIED;
}

// 0x69 GoldModifier
//in BG2 this effect subtracts gold when type is MOD_ADDITIVE
//no one uses it, though. To keep the function, the default branch will do the subtraction
int fx_gold_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_gold_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
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

// 0x6a MoraleBreakModifier
int fx_morale_break_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_morale_break_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		BASE_MOD(IE_MORALEBREAK);
	} else {
		STAT_MOD(IE_MORALEBREAK);
	}
	return FX_PERMANENT; //permanent morale break doesn't stick
}

// 0x6b PortraitChange
int fx_portrait_change (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_portrait_change (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	target->SetPortrait( fx->Resource, fx->Parameter2);
	return FX_NOT_APPLIED;
}

// 0x6c ReputationModifier
int fx_reputation_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_reputation_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD(IE_REPUTATION);
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
	if (0) print( "fx_turn_undead (%2d): Level %d\n", fx->Opcode, fx->Parameter1 );
	if (fx->Parameter1) {
		target->Turn(Owner, fx->Parameter1);
	} else {
		if (Owner->Type!=ST_ACTOR) {
			return FX_NOT_APPLIED;
		}
		target->Turn(Owner, ((Actor *) Owner)->GetStat(IE_TURNUNDEADLEVEL));
	}
	return FX_APPLIED;
}

// 0x6f Item:CreateMagic
static EffectRef fx_remove_item_ref = { "Item:Remove", -1 };

int fx_create_magic_item (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	//charge count is the same for all slots by default
	if (!fx->Parameter3) fx->Parameter3 = fx->Parameter1;
	if (!fx->Parameter4) fx->Parameter4 = fx->Parameter1;
	int slot = target->inventory.GetMagicSlot();
	target->inventory.SetSlotItemRes(fx->Resource, slot, fx->Parameter1, fx->Parameter3, fx->Parameter4);
	//IWD doesn't let you create two handed weapons (actually only decastave) if shield slot is filled
	//modders can still force two handed weapons with Parameter2
	if (!fx->Parameter2) {
		if (target->inventory.GetItemFlag(slot)&IE_ITEM_TWO_HANDED) {
			if (target->inventory.HasItemInSlot("",target->inventory.GetShieldSlot())) {
				target->inventory.RemoveItem(slot);
				displaymsg->DisplayConstantStringNameString(STR_SPELL_FAILED, DMC_WHITE, STR_OFFHAND_USED, target);
				return FX_NOT_APPLIED;
			}
		}
	}

	//equip the weapon
	target->inventory.SetEquippedSlot(target->inventory.GetMagicSlot()-target->inventory.GetWeaponSlot(), 0);
	if ((fx->TimingMode&0xff) == FX_DURATION_INSTANT_LIMITED) {
		//if this effect has expiration, then it will remain as a remove_item
		//on the effect queue, inheriting all the parameters
		fx->Opcode=EffectQueue::ResolveEffect(fx_remove_item_ref);
		fx->TimingMode=FX_DURATION_DELAY_PERMANENT;
		return FX_APPLIED;
	}
	return FX_NOT_APPLIED;
}

// 0x70 Item:Remove
int fx_remove_item (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	//will destroy the first item
	if (target->inventory.DestroyItem(fx->Resource,0,1)) {
		target->ReinitQuickSlots();
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
int fx_dither (Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) print( "fx_dither (%2d): Value: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
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

	ieDword color = fx->Parameter1;
	switch (msk) {
	case AL_EVIL:
		if (!color) color = 0xff0000;
		displaymsg->DisplayConstantStringName(STR_EVIL, color, target);
		//glow red
		target->SetColorMod(0xff, RGBModifier::ADD, 30, 0xff, 0, 0, 0);
		break;
	case AL_GOOD:
		if (!color) color = 0xff00;
		displaymsg->DisplayConstantStringName(STR_GOOD, color, target);
		//glow green
		target->SetColorMod(0xff, RGBModifier::ADD, 30, 0, 0xff, 0, 0);
		break;
	case AL_GE_NEUTRAL:
		if (!color) color = 0xff;
		displaymsg->DisplayConstantStringName(STR_GE_NEUTRAL, color, target);
		//glow blue
		target->SetColorMod(0xff, RGBModifier::ADD, 30, 0, 0, 0xff, 0);
		break;
	case AL_CHAOTIC:
		if (!color) color = 0xff00ff;
		displaymsg->DisplayConstantStringName(STR_CHAOTIC, color, target);
		//glow purple
		target->SetColorMod(0xff, RGBModifier::ADD, 30, 0xff, 0, 0xff, 0);
		break;
	case AL_LAWFUL:
		if (!color) color = 0xffffff;
		displaymsg->DisplayConstantStringName(STR_LAWFUL, color, target);
		//glow white
		target->SetColorMod(0xff, RGBModifier::ADD, 30, 0xff, 0xff, 0xff, 0);
		break;
	case AL_LC_NEUTRAL:
		if (!color) color = 0xff;
		displaymsg->DisplayConstantStringName(STR_LC_NEUTRAL, color, target);
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
int fx_reveal_area (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_reveal_area (%2d): Value: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	Map *map = NULL;

	if (target) {
		map = target->GetCurrentArea();
	} else {
		map = core->GetGame()->GetCurrentArea();
	}
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
int fx_reveal_creatures (Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) print( "fx_reveal_creatures (%2d): Value: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//this effect doesn't work in any original engine versions
	return FX_NOT_APPLIED;
}

// 0x77 MirrorImage
static EffectRef fx_mirror_image_modifier_ref = { "MirrorImageModifier", -1 };

int fx_mirror_image (Scriptable* Owner, Actor* target, Effect* fx)
{
	if (0) print( "fx_mirror_image (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	ieDword images;

	if (fx->Parameter2) {
		images = 1; //reflection
	}
	else {
		// the original uses only IE_LEVEL, but that can be awefully bad in
		// the case of dual- and multiclasses
		unsigned int level = target->GetCasterLevel(IE_SPL_WIZARD);
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
int fx_immune_to_weapon (Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) print( "fx_immune_to_weapon (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
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
int fx_visual_animation_effect (Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	//this effect doesn't work in any original engine versions
	if (0) print( "fx_visual_animation_effect (%2d)\n", fx->Opcode );
	return FX_NOT_APPLIED;
}

// 0x7a Item:CreateInventory
static EffectRef fx_remove_inventory_item_ref = { "Item:RemoveInventory", -1 };

int fx_create_inventory_item (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_create_inventory_item (%2d)\n", fx->Opcode );
	target->inventory.AddSlotItemRes( fx->Resource, SLOT_ONLYINVENTORY, fx->Parameter1, fx->Parameter3, fx->Parameter4 );
	if ((fx->TimingMode&0xff) == FX_DURATION_INSTANT_LIMITED) {
		//if this effect has expiration, then it will remain as a remove_item
		//on the effect queue, inheriting all the parameters
		fx->Opcode=EffectQueue::ResolveEffect(fx_remove_inventory_item_ref);
		fx->TimingMode=FX_DURATION_DELAY_PERMANENT;
		return FX_APPLIED;
	}
	return FX_NOT_APPLIED;
}

// 0x7b Item:RemoveInventory
int fx_remove_inventory_item (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_remove_inventory_item (%2d)\n", fx->Opcode );
	//FIXME: now equipped items are only wielded weapons
	//why would it not let equipped items to be destructed?
	target->inventory.DestroyItem(fx->Resource,IE_INV_ITEM_EQUIPPED,1);
	return FX_NOT_APPLIED;
}

// 0x7c DimensionDoor
// iwd2 has several options
int fx_dimension_door (Scriptable* Owner, Actor* target, Effect* fx)
{
	if (0) print( "fx_dimension_door (%2d) Type:%d\n", fx->Opcode, fx->Parameter2 );
	Point p;

	switch(fx->Parameter2)
	{
	case 0: //target to point
		p.x=fx->PosX;
		p.y=fx->PosY;
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
		p.x=STAT_GET(IE_SAVEDYPOS);
		target->SetOrientation(STAT_GET(IE_SAVEDFACE), false);
		break;
	case 3: //owner swapped with target
		if (Owner->Type!=ST_ACTOR) {
			return FX_NOT_APPLIED;
		}
		p=target->Pos;
		target->SetPosition(Owner->Pos, true, 0);
		target = (Actor *) Owner;
		break;
	}
	target->SetPosition(p, true, 0 );
	return FX_NOT_APPLIED;
}

// 0x7d Unlock
int fx_knock (Scriptable* Owner, Actor* /*target*/, Effect* fx)
{
	if (0) print( "fx_knock (%2d) [%d.%d]\n", fx->Opcode, fx->PosX, fx->PosY );
	Map *map = Owner->GetCurrentArea();
	if (!map) {
		return FX_NOT_APPLIED;
	}
	Point p(fx->PosX, fx->PosY);

print("KNOCK Pos: %d.%d\n", fx->PosX, fx->PosY);
	Door *door = map->TMap->GetDoorByPosition(p);
	if (door) {
print("Got a door\n");
		if (door->LockDifficulty<100) {
			door->SetDoorLocked(false, true);
		}
		return FX_NOT_APPLIED;
	}
	Container *container = map->TMap->GetContainerByPosition(p);
	if (container) {
print("Got a container\n");
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
	if (0) print( "fx_movement_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	//iwd2 freeaction disables only 0xb0, who cares
	if (target->HasSpellState(SS_FREEACTION)) return FX_NOT_APPLIED;
	//iwd2 aegis doesn't protect against grease/acid fog slowness, but that is
	//definitely a bug
	if (target->HasSpellState(SS_AEGIS)) return FX_NOT_APPLIED;

	ieDword value = target->GetStat(IE_MOVEMENTRATE);
	STAT_MOD(IE_MOVEMENTRATE);
	if (value < target->GetStat(IE_MOVEMENTRATE)) {
		target->AddPortraitIcon(PI_HASTED);
	}
	return FX_APPLIED;
}

#define FX_MS 10
static const ieResRef monster_summoning_2da[FX_MS]={"MONSUM01","MONSUM02","MONSUM03",
 "ANISUM01","ANISUM02", "MONSUM01", "MONSUM02","MONSUM03","ANISUM01","ANISUM02"};

// 0x7f MonsterSummoning
int fx_monster_summoning (Scriptable* Owner, Actor* target, Effect* fx)
{
	if (0) print( "fx_monster_summoning (%2d): Number: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//check the summoning limit?
	if (!Owner) {
		return FX_NOT_APPLIED;
	}

	if (!Owner->GetCurrentArea()) {
		return FX_APPLIED;
	}

	//get monster resref from 2da determined by fx->Resource or fx->Parameter2
	//the only addition to the original engine is that fx->Resource can be
	//used to specify a 2da (if parameter2 is >= 10)
	ieResRef monster;
	ieResRef hit;
	ieResRef areahit;
	ieResRef table;
	int level = fx->Parameter1;

	if (fx->Parameter2>=FX_MS) {
		if (fx->Resource[0]) {
			strnuprcpy(table, fx->Resource, 8);
		} else {
			strnuprcpy(table, "ANISUM03", 8);
		}
	} else {
		strnuprcpy(table, monster_summoning_2da[fx->Parameter2], 8);
	}
	core->GetResRefFrom2DA(monster_summoning_2da[fx->Parameter2], monster, hit, areahit);

	if (!hit[0]) {
		strnuprcpy(hit, fx->Resource2, 8);
	}
	if (!areahit[0]) {
		strnuprcpy(areahit, fx->Resource3, 8);
	}

	//the monster should appear near the effect position
	Point p(fx->PosX, fx->PosY);

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
	core->SummonCreature(monster, hit, caster, target, p, eamod, level, newfx);
	delete newfx;
	return FX_NOT_APPLIED;
}

// 0x80 State:Confused
int fx_set_confused_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_confused_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	if (enhanced_effects) {
		target->AddPortraitIcon(PI_CONFUSED);
	}
	return FX_PERMANENT;
}

// 0x81 AidNonCumulative
int fx_set_aid_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_aid_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
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
		BASE_ADD( IE_HITPOINTS, fx->Parameter1);
	}
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

static EffectRef fx_bane_ref = { "Bane", -1 };

int fx_set_bless_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_bless_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
int fx_set_chant_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_chant_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	if (0) print( "fx_set_holy_state (%2d): Modifier: %d\n", fx->Opcode, fx->Parameter1 );

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
int fx_luck_non_cumulative (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_luck_non_cumulative (%2d): Modifier: %d\n", fx->Opcode, fx->Parameter1);

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
	if (0) print( "fx_luck_cumulative (%2d): Modifier: %d\n", fx->Opcode, fx->Parameter1);

	target->SetSpellState(SS_LUCK);
	STAT_ADD( IE_LUCK, fx->Parameter1 );
	//TODO:check this in IWD2
	STAT_ADD( IE_DAMAGELUCK, fx->Parameter1 );
	return FX_APPLIED;
}

// 0x86 State:Petrification
int fx_set_petrified_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_petrified_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	BASE_STATE_SET( STATE_PETRIFIED );
	//always permanent effect, in fact in the original it is a death opcode (Avenger)
	//i just would like this to be less difficult to use in mods (don't destroy petrified creatures)
	return FX_NOT_APPLIED;
}

// 0x87 Polymorph
static EffectRef fx_polymorph_ref = { "Polymorph", -1 };

void CopyPolymorphStats(Actor *source, Actor *target)
{
	int i;

	if(!polymorph_stats) {
		AutoTable tab("polystat");
		if (!tab) {
			polymorph_stats = (int *) malloc(0);
			polystatcount=0;
			return;
		}
		polystatcount = tab->GetRowCount();
		polymorph_stats=(int *) malloc(sizeof(int)*polystatcount);
		for (i=0;i<polystatcount;i++) {
			polymorph_stats[i]=core->TranslateStat(tab->QueryField(i,0));
		}
	}

	assert(target->polymorphCache);

	if (!target->polymorphCache->stats) {
		target->polymorphCache->stats = new ieDword[polystatcount];
	}

	for(i=0;i<polystatcount;i++) {
		target->polymorphCache->stats[i] = source->Modified[polymorph_stats[i]];
	}
}

int fx_polymorph (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_polymorph_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	if (!gamedata->Exists(fx->Resource,IE_CRE_CLASS_ID)) {
		//kill all polymorph effects
		target->fxqueue.RemoveAllEffectsWithParam(fx_polymorph_ref, fx->Parameter2);
		//destroy the magic item slot
		target->inventory.RemoveItem(target->inventory.GetMagicSlot() );
		return FX_NOT_APPLIED;
	}

	// to avoid repeatedly loading the file or keeping all the data around
	// wasting memory, we keep a PolymorphCache object around, with only
	// the data we need from the polymorphed creature
	bool cached = true;
	if (!target->polymorphCache) {
		cached = false;
		target->polymorphCache = new PolymorphCache();
	}
	if (!cached || strnicmp(fx->Resource,target->polymorphCache->Resource,sizeof(fx->Resource))) {
		Actor *newCreature = gamedata->GetCreature(fx->Resource,0);

		//I don't know how could this happen, existance of the resource was already checked
		if (!newCreature) {
			return FX_NOT_APPLIED;
		}

		memcpy(target->polymorphCache->Resource, fx->Resource, sizeof(fx->Resource));
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

	for(int i=0;i<polystatcount;i++) {
		//copy only the animation ID
		if (fx->Parameter2 && polymorph_stats[i] != IE_ANIMATION_ID) continue;

		target->SetStat(polymorph_stats[i], target->polymorphCache->stats[i], 1);
	}

	return FX_APPLIED;
}

// 0x88 ForceVisible
int fx_force_visible (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_force_visible (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	if (pstflags) {
		BASE_STATE_CURE(STATE_PST_INVIS);
	} else {
		BASE_STATE_CURE(STATE_INVISIBLE);
	}
	target->fxqueue.RemoveAllEffectsWithParam(fx_set_invisible_state_ref,0);
	target->fxqueue.RemoveAllEffectsWithParam(fx_set_invisible_state_ref,2);
	return FX_NOT_APPLIED;
}

// 0x89 ChantBadNonCumulative
int fx_set_chantbad_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_chantbad_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	if (0) print( "fx_animation_stance (%2d): Stance: %d\n", fx->Opcode, fx->Parameter2 );

	//this effect works only on living actors
	if ( !STATE_GET(STATE_DEAD) ) {
		target->SetStance(fx->Parameter2);
	}
	return FX_NOT_APPLIED;
}

// 0x8B DisplayString
// gemrb extension: rgb colour for displaystring
// gemrb extension: resource may be an strref list (src or 2da)
static EffectRef fx_protection_from_display_string_ref = { "Protection:String", -1 };

int fx_display_string (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_display_string (%2d): StrRef: %d\n", fx->Opcode, fx->Parameter1 );
	if(fx->Resource[0]) {
		//TODO: create a single list reader that handles src and 2da too
		SrcVector *rndstr=LoadSrc(fx->Resource);
		if (rndstr) {
			fx->Parameter1 = rndstr->at(rand()%rndstr->size());
			FreeSrc(rndstr, fx->Resource);
			DisplayStringCore(target, fx->Parameter1, DS_HEAD);
			*(ieDword *) &target->overColor=fx->Parameter2;
			return FX_NOT_APPLIED;
		}

		//random text for other games
		ieDword *rndstr2 = core->GetListFrom2DA(fx->Resource);
		int cnt = rndstr2[0];
		if (cnt) {
			fx->Parameter1 = rndstr2[core->Roll(1,cnt,0)];
		}
	}

	if (!target->fxqueue.HasEffectWithParamPair(fx_protection_from_display_string_ref, fx->Parameter1, 0) ) {
		displaymsg->DisplayStringName(fx->Parameter1, fx->Parameter2?fx->Parameter2:DMC_WHITE, target, IE_STR_SOUND|IE_STR_SPEECH);
	}
	return FX_NOT_APPLIED;
}

// 0x8c CastingGlow
static const int ypos_by_direction[16]={10,10,10,0,-10,-10,-10,-10,-10,-10,-10,-10,0,10,10,10};
static const int xpos_by_direction[16]={0,-10,-12,-14,-16,-14,-12,-10,0,10,12,14,16,14,12,10};

int fx_casting_glow (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_casting_glow (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
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
		sca->PlayOnce();
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
int fx_visual_spell_hit (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_visual_spell_hit (%2d): Target: %d Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
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
	} else {
		print("fx_visual_spell_hit: Unhandled Type: %d\n", fx->Parameter2);
	}
	return FX_NOT_APPLIED;
}

//0x8e Icon:Display
int fx_display_portrait_icon (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_display_string (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	target->AddPortraitIcon(fx->Parameter2);
	return FX_APPLIED;
}

//0x8f Item:CreateInSlot
int fx_create_item_in_slot (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_create_item_in_slot (%2d): Button: %d\n", fx->Opcode, fx->Parameter2 );
	//create item and set it in target's slot
	target->inventory.SetSlotItemRes( fx->Resource, core->QuerySlot(fx->Parameter2), fx->Parameter1, fx->Parameter3, fx->Parameter4 );
	if ((fx->TimingMode&0xff) == FX_DURATION_INSTANT_LIMITED) {
		//convert it to a destroy item
		fx->Opcode=EffectQueue::ResolveEffect(fx_remove_item_ref);
		fx->TimingMode=FX_DURATION_DELAY_PERMANENT;
		return FX_APPLIED;
	}
	return FX_NOT_APPLIED;
}

// 0x90 DisableButton
// different in iwd2 and the rest (maybe also in how: 0-7?)
int fx_disable_button (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_disable_button (%2d): Button: %d\n", fx->Opcode, fx->Parameter2 );

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

/*internal representation of disabled spells in IE_CASTING (bitfield):
1 - items (SPIT)
2 - cleric (SPPR)
4 - mage  (SPWI)
8 - innate (SPIN)
16 - class (SPCL)
*/

static ieDword dsc_bits_iwd2[7]={1, 14, 6, 2, 4, 8, 16};
static ieDword dsc_bits_bg2[7]={1, 4, 2, 8, 16, 14, 6};
int fx_disable_spellcasting (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_disable_spellcasting (%2d): Button: %d\n", fx->Opcode, fx->Parameter2 );

	bool display_warning = false;
	ieDword tmp = fx->Parameter2+1;

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
		if (tmp<7) {
			STAT_BIT_OR(IE_CASTING, dsc_bits_iwd2[tmp] );
		}
	} else { // bg2
		if (fx->Parameter2 == 0) {
			if (target->spellbook.GetKnownSpellsCount(IE_SPELL_TYPE_WIZARD, 0)) display_warning = true;
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
	if (fx->FirstApply && display_warning && target->GetStat(IE_EA) < EA_CONTROLLABLE) {
		displaymsg->DisplayConstantStringName(STR_DISABLEDMAGE, DMC_RED, target);
		core->SetEventFlag(EF_ACTION);
	}
	return FX_APPLIED;
}

//0x92 Spell:Cast
int fx_cast_spell (Scriptable* Owner, Actor* target, Effect* fx)
{
	if (0) print( "fx_cast_spell (%2d): Resource:%s Mode: %d\n", fx->Opcode, fx->Resource, fx->Parameter2 );
	if (fx->Parameter2) {
		//apply spell on target
		core->ApplySpell(fx->Resource, target, Owner, fx->Parameter1);

		// give feedback: Caster - spellname : target
		char tmp[100];
		Spell *spl = gamedata->GetSpell(fx->Resource);
		if (spl) {
			snprintf(tmp, sizeof(tmp), "%s : %s", core->GetString(spl->SpellName), target->GetName(-1));
			displaymsg->DisplayStringName(tmp, DMC_WHITE, Owner);
		}
	} else {
		// save the current spell ref, so the rest of its effects can be applied afterwards
		ieResRef OldSpellResRef;
		memcpy(OldSpellResRef, Owner->SpellResRef, sizeof(OldSpellResRef));
		Owner->SetSpellResRef(fx->Resource);
		//cast spell on target
		Owner->CastSpell(target, false);
		//actually finish casting (if this is not good enough, use an action???)
		Owner->CastSpellEnd(fx->Parameter1);
		Owner->SetSpellResRef(OldSpellResRef);
	}
	return FX_NOT_APPLIED;
}

// 0x93 Spell:Learn
int fx_learn_spell (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_learn_spell (%2d): Resource:%s Flags: %d Mode: %d\n", fx->Opcode, fx->Resource, fx->Parameter1, fx->Parameter2 );
	//parameter1 is unused, gemrb lets you to make it not give XP
	//probably we should also let this via a game flag if we want
	//full compatibility with bg1
	//parameter2 is used in bg1 and pst to specify the spell type; bg2 and iwd2 figure it out from the resource

	int x= target->LearnSpell(fx->Resource, fx->Parameter1);
	print("Learnspell returned: %d\n", x);
	return FX_NOT_APPLIED;
}
// 0x94 Spell:CastSpellPoint
int fx_cast_spell_point (Scriptable* Owner, Actor* /*target*/, Effect* fx)
{
	if (0) print( "fx_cast_spell_point (%2d): Resource:%s Mode: %d\n", fx->Opcode, fx->Resource, fx->Parameter2 );
	// save the current spell ref, so the rest of its effects can be applied afterwards
	ieResRef OldSpellResRef;
	memcpy(OldSpellResRef, Owner->SpellResRef, sizeof(OldSpellResRef));
	Owner->SetSpellResRef(fx->Resource);
	Point p(fx->PosX, fx->PosY);
	Owner->CastSpellPoint(p, false);
	//actually finish casting (if this is not good enough, use an action???)
	Owner->CastSpellPointEnd(fx->Parameter1);
	Owner->SetSpellResRef(OldSpellResRef);
	return FX_NOT_APPLIED;
}

// 0x95 Identify
int fx_identify (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_identify (%2d): Resource:%s Mode: %d\n", fx->Opcode, fx->Resource, fx->Parameter2 );
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
	if (0) print( "fx_find_traps (%2d)\n", fx->Opcode );
	//reveal trapped containers, doors, triggers that are in the visible range
	ieDword range = target->GetStat(IE_VISUALRANGE)*10;
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
			//fall through is intentional here
		default:
			//automatic find traps
			skill = 256;
			break;
	}

	TileMap *TMap = target->GetCurrentArea()->TMap;

	int Count = 0;
	while (true) {
		Door* door = TMap->GetDoor( Count++ );
		if (!door)
			break;
		if (Distance(door->Pos, target->Pos)<range) {
			if (detecttraps) {
			//when was door trap noticed
				door->DetectTrap(skill);
			}
			door->TryDetectSecret(skill);
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
int fx_replace_creature (Scriptable* Owner, Actor* target, Effect *fx)
{
	if (0) print( "fx_replace_creature (%2d): Resource: %s\n", fx->Opcode, fx->Resource );

	//this safeguard exists in the original engine too
	if (!gamedata->Exists(fx->Resource,IE_CRE_CLASS_ID)) {
		return FX_NOT_APPLIED;
	}

	//the monster should appear near the effect position? (unsure)
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
	//create replacement; should we be passing the target instead of NULL?
	//noooo, don't unsummon replacement creatures! - fuzzie
	//Effect *newfx = EffectQueue::CreateUnsummonEffect(fx);
	core->SummonCreature(fx->Resource, fx->Resource2, Owner, NULL,p, EAM_DEFAULT,-1, NULL, 0);
	//delete newfx;
	return FX_NOT_APPLIED;
}

// 0x98 PlayMovie
int fx_play_movie (Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) print( "fx_play_movie (%2d): Resource: %s\n", fx->Opcode, fx->Resource );
	core->PlayMovie (fx->Resource);
	return FX_NOT_APPLIED;
}
// 0x99 Overlay:Sanctuary

static const ieDword fullwhite[7]={ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT,ICE_GRADIENT};

int fx_set_sanctuary_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	//iwd and bg are a bit different, but we solve the whole stuff in a single opcode
	if (0) print( "fx_set_sanctuary_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	// don't set the state twice
	// SetSpellState will also check if it is already set first
	if (target->SetSpellState(SS_SANCTUARY)) return FX_NOT_APPLIED;

	if (!fx->Parameter2) {
		fx->Parameter2=1;
	}
	//this effect needs the pcf run immediately
	STAT_SET_PCF( IE_SANCTUARY, fx->Parameter2);
	//a rare event, but this effect gives more in bg2 than in iwd2
	//so we use this flag
	if (!enhanced_effects) {
		target->SetLockedPalette(fullwhite);
	}
	return FX_APPLIED;
}

// 0x9a Overlay:Entangle
int fx_set_entangle_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_entangle_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
int fx_set_minorglobe_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_minorglobe_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//the resisted levels are stored in minor globe (bit 2-)
	//the globe effect is stored in the first bit
	STAT_BIT_OR_PCF( IE_MINORGLOBE, 1);
	return FX_APPLIED;
}

// 0x9c Overlay:ShieldGlobe
int fx_set_shieldglobe_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_shieldglobe_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//the shield vanishes on dead
	if (STATE_GET(STATE_DEAD) ) {
		return FX_NOT_APPLIED;
	}
	STAT_SET_PCF( IE_SHIELDGLOBE, 1);
	return FX_APPLIED;
}

// 0x9d Overlay:Web
int fx_set_web_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_web_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
int fx_set_grease_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_grease_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	if (0) print( "fx_mirror_image_modifier (%2d): Mod: %d\n", fx->Opcode, fx->Parameter1 );
	if (STATE_GET(STATE_DEAD) ) {
		return FX_NOT_APPLIED;
	}
	if (!fx->Parameter1) {
		return FX_NOT_APPLIED;
	}
	if (pstflags) {
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
static EffectRef fx_sanctuary_state_ref = { "Overlay:Sanctuary", -1 };

int fx_cure_sanctuary_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_cure_sanctuary_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_SET( IE_SANCTUARY, 0);
	target->fxqueue.RemoveAllEffects(fx_sanctuary_state_ref);
	return FX_NOT_APPLIED;
}

// 0xa1 Cure:Panic
static EffectRef fx_set_panic_state_ref = { "State:Panic", -1 };

int fx_cure_panic_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_cure_panic_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	BASE_STATE_CURE( STATE_PANIC );
	target->fxqueue.RemoveAllEffects(fx_set_panic_state_ref);
	return FX_NOT_APPLIED;
}

// 0xa2 Cure:Hold
static EffectRef fx_hold_creature_ref = { "State:Hold", -1 };

int fx_cure_hold_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_cure_hold_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//note that this effect doesn't remove 185 (another hold effect)
	target->fxqueue.RemoveAllEffects( fx_hold_creature_ref );
	target->fxqueue.RemoveAllEffects(fx_hold_creature_no_icon_ref);
	target->fxqueue.RemoveAllEffectsWithParam(fx_display_portrait_icon_ref, PI_HELD);
	return FX_NOT_APPLIED;
}

// 0xa3 FreeAction
static EffectRef fx_movement_modifier_ref = { "MovementRateModifier2", -1 };

int fx_cure_slow_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_cure_slow_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	target->fxqueue.RemoveAllEffects( fx_movement_modifier_ref );
//	STATE_CURE( STATE_SLOWED );
	return FX_NOT_APPLIED;
}

// 0xa4 Cure:Intoxication
static EffectRef fx_intoxication_ref = { "IntoxicationModifier", -1 };

int fx_cure_intoxication (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_cure_intoxication (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	target->fxqueue.RemoveAllEffects( fx_intoxication_ref );
	BASE_SET(IE_INTOXICATION,0);
	return FX_NOT_APPLIED;
}

// 0xa5 PauseTarget
int fx_pause_target (Scriptable* /*Owner*/, Actor * target, Effect* fx)
{
	if (0) print( "fx_pause_target (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_CASTERHOLD );
	return FX_PERMANENT;
}

// 0xa6 MagicResistanceModifier
int fx_magic_resistance_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_magic_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_RESISTMAGIC );
	return FX_APPLIED;
}

// 0xa7 MissileHitModifier
int fx_missile_to_hit_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_missile_to_hit_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_MISSILEHITBONUS );
	return FX_APPLIED;
}

// 0xa8 RemoveCreature
// removes targeted creature
// removes creature specified by resource key (gemrb extension)
int fx_remove_creature (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_remove_creature (%2d)\n", fx->Opcode);

	Map *map = NULL;

	if (target) {
		map = target->GetCurrentArea();
	}
	else {
		map = core->GetGame()->GetCurrentArea();
	}
	Actor *actor = target;

	if (fx->Resource[0]) {
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
	if (0) print( "fx_disable_portrait_icon (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	target->DisablePortraitIcon(fx->Parameter2);
	return FX_APPLIED;
}

// 0xaa DamageAnimation
int fx_damage_animation (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_damage_animation (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	//Parameter1 is a gemrb extension
	//Parameter2's high byte has effect on critical damage animations (PST compatibility hack)
	target->PlayDamageAnimation(fx->Parameter2, !fx->Parameter1);
	return FX_NOT_APPLIED;
}

// 0xab Spell:Add
int fx_add_innate (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_add_innate (%2d): Resource: %s Mode: %d\n", fx->Opcode, fx->Resource, fx->Parameter2 );
	target->LearnSpell(fx->Resource, fx->Parameter2);
	//this is an instant, so it shouldn't stick
	return FX_NOT_APPLIED;
}

// 0xac Spell:Remove
//gemrb extension: deplete spell by resref
int fx_remove_spell (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_remove_spell (%2d): Resource: %s Type:%d\n", fx->Opcode, fx->Resource, fx->Parameter2);
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

// 0xad PoisonResistanceModifier
int fx_poison_resistance_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_poison_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_RESISTPOISON );
	return FX_APPLIED;
}

//0xae PlaySound
int fx_playsound (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_playsound (%s)", fx->Resource );
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
int fx_hold_creature_no_icon (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_hold_creature_no_icon (%2d): Value: %d, IDS: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	//actually the original engine just skips this effect if the target is dead
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
int fx_hold_creature (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_hold_creature (%2d): Value: %d, IDS: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	if (0) print( "fx_apply_effect (%2d) %s", fx->Opcode, fx->Resource );

	//this effect executes a file effect in place of this effect
	//the file effect inherits the target and the timingmode, but gets
	//a new chance to roll percents
	if (target && !EffectQueue::match_ids( target, fx->Parameter2, fx->Parameter1) ) {
		return FX_NOT_APPLIED;
	}

	Point p(fx->PosX, fx->PosY);

	//apply effect, if the effect is a goner, then kill
	//this effect too
	Effect *newfx = core->GetEffect(fx->Resource, fx->Power, p);
	if (!newfx)
		return FX_NOT_APPLIED;

	Effect *myfx = new Effect;
	memcpy(myfx, newfx, sizeof(Effect));
	myfx->random_value = core->Roll(1,100,-1);
	myfx->Target = FX_TARGET_PRESET;
	myfx->TimingMode = fx->TimingMode;
	myfx->Duration = fx->Duration;
	myfx->CasterID = fx->CasterID;

	int ret;
	if (target) {
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
int fx_apply_effect_item (Scriptable* Owner, Actor* target, Effect* fx)
{
	if (0) print("fx_apply_effect_item (%2d) (%.8s)\n", fx->Opcode, fx->Resource);
	if (target->inventory.HasItem(fx->Resource, 0) ) {
		core->ApplySpell(fx->Resource2, target, Owner, fx->Parameter1);
		return FX_NOT_APPLIED;
	}
	return FX_APPLIED;
}

// b7 generic effect ApplyEffectItemType
int fx_apply_effect_item_type (Scriptable* Owner, Actor* target, Effect* fx)
{
	if (0) print("fx_apply_effect_item (%2d), Type: %d\n", fx->Opcode, fx->Parameter2);
	if (target->inventory.HasItemType(fx->Parameter2) ) {
		core->ApplySpell(fx->Resource, target, Owner, fx->Parameter1);
		return FX_NOT_APPLIED;
	}
	return FX_APPLIED;
}

// b8 DontJumpModifier
int fx_dontjump_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_dontjump_modifier (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_SET( IE_DONOTJUMP, fx->Parameter2 );
	return FX_APPLIED;
}

//0xb9 see above: fx_hold_creature

//0xba MoveToArea
int fx_move_to_area (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_move_to_area (%2d) %s", fx->Opcode, fx->Resource );
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
int fx_local_variable (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	//this is a hack, the variable name spreads across the resources
	if (0) print( "fx_local_variable (%2d) %s=%d", fx->Opcode, fx->Resource, fx->Parameter1 );
	target->locals->SetAt(fx->Resource, fx->Parameter1);
	//local variable effects are not applied, they will be resaved though
	return FX_NOT_APPLIED;
}

// 0xbc AuraCleansingModifier
int fx_auracleansing_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_auracleansing_modifier (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_SET( IE_AURACLEANSING, fx->Parameter2 );
	return FX_APPLIED;
}

// 0xbd CastingSpeedModifier
int fx_castingspeed_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_castingspeed_modifier (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_MOD( IE_MENTALSPEED );
	return FX_APPLIED;
}

// 0xbe PhysicalSpeedModifier
int fx_attackspeed_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_attackspeed_modifier (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_MOD( IE_PHYSICALSPEED );
	return FX_APPLIED;
}

// 0xbf CastingLevelModifier
// gemrb extension: if the resource key is set, apply param1 as a percentual modifier
int fx_castinglevel_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_castinglevel_modifier (%2d) Value:%d Type:%d", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	switch (fx->Parameter2) {
	case 0:
		if (fx->Resource[0]) {
			STAT_MUL( IE_CASTINGLEVELBONUSMAGE, fx->Parameter1 );
		} else {
			STAT_SET( IE_CASTINGLEVELBONUSMAGE, fx->Parameter1 );
		}
		break;
	case 1:
		if (fx->Resource[0]) {
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

// 0xc0 FindFamiliar
// param2 = 1 alignment is in param1
// param2 = 2 resource used
#define FAMILIAR_NORMAL    0
#define FAMILIAR_ALIGNMENT 1
#define FAMILIAR_RESOURCE  2

static EffectRef fx_familiar_constitution_loss_ref = { "FamiliarBond", -1 };
static EffectRef fx_familiar_marker_ref = { "FamiliarMarker", -1 };
static EffectRef fx_maximum_hp_modifier_ref = { "MaximumHPModifier", -1 };

//returns the familiar if there was no error
Actor *GetFamiliar(Scriptable *Owner, Actor *target, Effect *fx, ieResRef resource)
{
	//summon familiar
	Actor *fam = gamedata->GetCreature(resource);
	if (!fam) {
		return NULL;
	}
	fam->SetBase(IE_EA, EA_FAMILIAR);

	//when upgrading, there is no need for the script to be triggered again, so this isn't a problem
	if (Owner) {
		fam->LastSummoner = Owner->GetGlobalID();
	}

	Map *map = target->GetCurrentArea();
	if (!map) return NULL;

	map->AddActor(fam);
	Point p(fx->PosX, fx->PosY);
	fam->SetPosition(p, true, 0);
	fam->RefreshEffects(NULL);
	//Make the familiar an NPC (MoveGlobal needs this)
	Game *game = core->GetGame();
	game->AddNPC(fam);

	//Add some essential effects
	Effect *newfx = EffectQueue::CreateEffect(fx_familiar_constitution_loss_ref, fam->GetBase(IE_HITPOINTS)/2, 0, FX_DURATION_INSTANT_PERMANENT);
	core->ApplyEffect(newfx, fam, fam);
	delete newfx;

	//the familiar marker needs to be set to 2 in case of ToB
	ieDword fm = 0;
	if (game->Expansion==5) {
		fm = 2;
	}
	newfx = EffectQueue::CreateEffect(fx_familiar_marker_ref, fm, 0, FX_DURATION_INSTANT_PERMANENT);
	core->ApplyEffect(newfx, fam, fam);
	delete newfx;

	//maximum hp bonus of half the familiar's hp, there is no hp new bonus upgrade when upgrading familiar
	//this is a bug even in the original engine, so I don't care
	if (Owner) {
		newfx = EffectQueue::CreateEffect(fx_maximum_hp_modifier_ref, fam->GetBase(IE_HITPOINTS)/2, MOD_ADDITIVE, FX_DURATION_INSTANT_PERMANENT);
		core->ApplyEffect(newfx, (Actor *) Owner, Owner);
		delete newfx;
	}

	if (fx->Resource2[0]) {
		ScriptedAnimation* vvc = gamedata->GetScriptedAnimation(fx->Resource2, false);
		if (vvc) {
			//This is the final position of the summoned creature
			//not the original target point
			vvc->XPos=fam->Pos.x;
			vvc->YPos=fam->Pos.y;
			//force vvc to play only once
			vvc->PlayOnce();
			map->AddVVCell( vvc );
		}
	}

	return fam;
}

int fx_find_familiar (Scriptable* Owner, Actor* target, Effect* fx)
{
	if (0) print( "fx_find_familiar (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );

	if (!target || !Owner) {
		return FX_NOT_APPLIED;
	}

	if (!target->GetCurrentArea()) {
		//this will delay casting until we get an area
		return FX_APPLIED;
	}

	Game *game = core->GetGame();
	//FIXME: the familiar block field is not saved in the game and not set when the
	//familiar is itemized, so a game reload will clear it (see how this is done in original)
	if (game->familiarBlock) {
		displaymsg->DisplayConstantStringName(STR_FAMBLOCK, DMC_RED, target);
		return FX_NOT_APPLIED;
	}

	//The protagonist is ALWAYS in the first slot
	if (game->GetPC(0, false)!=target) {
		displaymsg->DisplayConstantStringName(STR_FAMPROTAGONIST, DMC_RED, target);
		return FX_NOT_APPLIED;
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
		Game *game = core->GetGame();

		memcpy(fx->Resource, game->Familiars[alignment],sizeof(ieResRef) );
		//ToB familiars
		if (game->Expansion==5) {
			strncat(fx->Resource,"25",8);
		}
		fx->Parameter2=FAMILIAR_RESOURCE;
	}

	GetFamiliar(Owner, target, fx, fx->Resource);
	return FX_NOT_APPLIED;
}

// 0xc1 InvisibleDetection
int fx_see_invisible_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_see_invisible_modifier (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_SET( IE_SEEINVISIBLE, fx->Parameter2 );
	return FX_APPLIED;
}

// 0xc2 IgnoreDialogPause
int fx_ignore_dialogpause_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_ignore_dialogpause_modifier (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_SET( IE_IGNOREDIALOGPAUSE, fx->Parameter2 );
	return FX_APPLIED;
}

//0xc3 FamiliarBond
//when this effect's target dies it should incur damage on protagonist
static EffectRef fx_damage_opcode_ref = { "Damage", -1 };
static EffectRef fx_constitution_modifier_ref = { "ConstitutionModifier", -1 };

int fx_familiar_constitution_loss (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_familiar_constitution_loss (%2d): Loss: %d\n", fx->Opcode,(signed) fx->Parameter1 );
	if (! (STAT_GET(IE_STATE_ID)&STATE_NOSAVE)) {
		return FX_APPLIED;
	}
	Effect *newfx;
	//familiar died
	Actor *master = core->GetGame()->FindPC(1);
	if (!master) return FX_NOT_APPLIED;

	//lose 1 point of constitution
	newfx = EffectQueue::CreateEffect(fx_constitution_modifier_ref, (ieDword) -1, MOD_ADDITIVE, FX_DURATION_INSTANT_PERMANENT);
	core->ApplyEffect(newfx, master, master);
	delete newfx;

	//remove the maximum hp bonus
	newfx = EffectQueue::CreateEffect(fx_maximum_hp_modifier_ref, (ieDword) -fx->Parameter1, 3, FX_DURATION_INSTANT_PERMANENT);
	core->ApplyEffect(newfx, master, master);
	delete newfx;

	//damage for half of the familiar's hitpoints
	newfx = EffectQueue::CreateEffect(fx_damage_opcode_ref, fx->Parameter1, DAMAGE_CRUSHING, FX_DURATION_INSTANT_PERMANENT);
	core->ApplyEffect(newfx, master, master);
	delete newfx;

	return FX_NOT_APPLIED;
}

//0xc4 FamiliarMarker
int fx_familiar_marker (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_familiar_marker (%2d)\n", fx->Opcode );
	if (!target) {
		return FX_NOT_APPLIED;
	}

	Game *game = core->GetGame();

	//upgrade familiar to ToB version
	if ((fx->Parameter1!=2) && (game->Expansion == 5) ) {
		ieResRef resource;

		memset(resource,0,sizeof(resource));
		memcpy(resource,target->GetScriptName(),6);
		strncat(resource,"25",8);
		//set this field, so the upgrade is triggered only once
		fx->Parameter1 = 2;

		//the NULL here is probably fine when upgrading, Owner (Original summoner) is not needed.
		Actor *fam = GetFamiliar(NULL, target, fx, resource);

		if (fam) {
			//upgrade successful
			//TODO: copy stuff from old familiar if needed
			target->DestroySelf();
			return FX_NOT_APPLIED;
		}
	}

	if (! (STAT_GET(IE_STATE_ID)&STATE_NOSAVE)) {
		game->familiarBlock=true;
		return FX_APPLIED;
	}
	game->familiarBlock=false;
	return FX_NOT_APPLIED;
}

// 0xc5 Bounce:Projectile
int fx_bounce_projectile (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_bounce_projectile (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_BIT_OR_PCF( IE_BOUNCE, BNC_PROJECTILE );
	return FX_APPLIED;
}

// 0xc6 Bounce:Opcode
int fx_bounce_opcode (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_bounce_opcode (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_BIT_OR_PCF( IE_BOUNCE, BNC_OPCODE );
	target->AddPortraitIcon(PI_BOUNCE2);
	return FX_APPLIED;
}

// 0xc7 Bounce:SpellLevel
int fx_bounce_spelllevel (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_bounce_spellevel (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_BIT_OR_PCF( IE_BOUNCE, BNC_LEVEL );
	target->AddPortraitIcon(PI_BOUNCE2);
	return FX_APPLIED;
}

// 0xc8 Bounce:SpellLevelDec
int fx_bounce_spelllevel_dec (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_bounce_spellevel_dec (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	if (fx->Parameter1<1) {
		PlayRemoveEffect("EFF_E02", target, fx);
		return FX_NOT_APPLIED;
	}

	STAT_BIT_OR_PCF( IE_BOUNCE, BNC_LEVEL_DEC );
	target->AddPortraitIcon(PI_BOUNCE);
	return FX_APPLIED;
}

//0xc9 Protection:SpellLevelDec
int fx_protection_spelllevel_dec (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_protection_spelllevel_dec (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	if (fx->Parameter1<1) {
		PlayRemoveEffect("EFF_E02", target, fx);
		return FX_NOT_APPLIED;
	}
	STAT_BIT_OR( IE_IMMUNITY, IMM_LEVEL_DEC );
	target->AddPortraitIcon(PI_BOUNCE2);
	return FX_APPLIED;
}

//0xca Bounce:School
int fx_bounce_school (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_bounce_school (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_BIT_OR_PCF( IE_BOUNCE, BNC_SCHOOL );
	target->AddPortraitIcon(PI_BOUNCE2);
	return FX_APPLIED;
}

// 0xcb Bounce:SecondaryType
int fx_bounce_secondary_type (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_bounce_secondary_type (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_BIT_OR_PCF( IE_BOUNCE, BNC_SECTYPE );
	target->AddPortraitIcon(PI_BOUNCE2);
	return FX_APPLIED;
}

// 0xcc //resist school
int fx_protection_school (Scriptable* /*Owner*/, Actor* target, Effect *fx)
{
	if (0) print( "fx_protection_school (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_BIT_OR( IE_IMMUNITY, IMM_SCHOOL);
	return FX_APPLIED;
}

// 0xcd //resist sectype
int fx_protection_secondary_type (Scriptable* /*Owner*/, Actor* target, Effect *fx)
{
	if (0) print( "fx_protection_secondary_type (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_BIT_OR( IE_IMMUNITY, IMM_SECTYPE);
	return FX_APPLIED;
}

//0xce Protection:Spell
int fx_resist_spell (Scriptable* /*Owner*/, Actor* target, Effect *fx)
{
	if (0) print( "fx_resist_spell (%2d): Resource: %s\n", fx->Opcode, fx->Resource );
	if (strnicmp(fx->Resource,fx->Source,sizeof(fx->Resource)) ) {
		STAT_BIT_OR( IE_IMMUNITY, IMM_RESOURCE);
		return FX_APPLIED;
	}
	//this has effect only on first apply, it will stop applying the spell
	return FX_ABORT;
}

// ??? Protection:SpellDec
// This is a fictional opcode, it isn't implemented in the original engine
int fx_resist_spell_dec (Scriptable* /*Owner*/, Actor* target, Effect *fx)
{
	if (0) print( "fx_resist_spell_dec (%2d): Resource: %s\n", fx->Opcode, fx->Resource );

	if (fx->Parameter1<1) {
		PlayRemoveEffect("EFF_E02", target, fx);
		return FX_NOT_APPLIED;
	}

	if (strnicmp(fx->Resource,fx->Source,sizeof(fx->Resource)) ) {
		STAT_BIT_OR( IE_IMMUNITY, IMM_RESOURCE_DEC);
		return FX_APPLIED;
	}
	//this has effect only on first apply, it will stop applying the spell
	return FX_ABORT;
}

// 0xcf Bounce:Spell
int fx_bounce_spell (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_bounce_spell (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_BIT_OR_PCF( IE_BOUNCE, BNC_RESOURCE );
	return FX_APPLIED;
}

// ??? Bounce:SpellDec
// This is a fictional opcode, it isn't implemented in the original engine
int fx_bounce_spell_dec (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_bounce_spell (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	if (fx->Parameter1<1) {
		PlayRemoveEffect("EFF_E02", target, fx);
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
	if (0) print( "fx_minimum_hp_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_MINHITPOINTS );
	return FX_APPLIED;
}

//0xd1 PowerWordKill
int fx_power_word_kill (Scriptable* Owner, Actor* target, Effect* fx)
{
	if (0) print( "fx_power_word_kill (%2d): HP: %d Stat: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
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
int fx_power_word_stun (Scriptable* Owner, Actor* target, Effect* fx)
{
	if (0) print( "fx_power_word_stun (%2d): HP: %d Stat: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
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
int fx_imprisonment (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_imprisonment (%2d)\n", fx->Opcode );
	target->SetMCFlag(MC_HIDDEN, BM_OR);
	target->AddPortraitIcon(PI_PRISON);
	return FX_APPLIED;
}

//0xd4 Cure:Imprisonment
static EffectRef fx_imprisonment_ref = { "Imprisonment", -1 };
static EffectRef fx_maze_ref = { "Maze", -1 };

int fx_freedom (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_freedom (%2d)\n", fx->Opcode );
	target->fxqueue.RemoveAllEffects( fx_imprisonment_ref );
	target->fxqueue.RemoveAllEffects( fx_maze_ref );
	return FX_NOT_APPLIED;
}

//0xd5 Maze
int fx_maze (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_maze (%2d)\n", fx->Opcode );
	Game *game = core->GetGame();
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
			fx->Duration = game->GameTime+target->LuckyRoll(dice, size, 0, 0)*100;
		}
	}

	target->SetMCFlag(MC_HIDDEN, BM_OR);
	target->AddPortraitIcon(PI_MAZE);
	return FX_APPLIED;
}

//0xd6 CastFromList
//GemRB extension: if fx->Parameter1 is set, it is the bitfield of spell types (could be priest spells)
int fx_select_spell (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_select_spell (%2d) %d\n", fx->Opcode, fx->Parameter2 );
	Spellbook *sb = &target->spellbook;
	if(fx->Parameter2) {
		//all known spells, no need to memorize
		// the details are all handled by the Spellbook guiscript
		core->GetDictionary()->SetAt("ActionLevel", 5);
	} else {
		//all spells listed in 2da
		ieResRef *data = NULL;

		int count = core->ReadResRefTable(fx->Resource, data);
		sb->SetCustomSpellInfo(data, fx->Source, count);
		core->FreeResRefTable(data, count);
		core->GetDictionary()->SetAt("ActionLevel", 2);
	}
	// force a redraw of the action bar
	//this is required, because not all of these opcodes are firing right at casting
	core->GetDictionary()->SetAt("Type",-1);
	core->SetEventFlag(EF_ACTION);
	return FX_NOT_APPLIED;
}

// 0xd7 PlayVisualEffect
static EffectRef fx_protection_from_animation_ref = { "Protection:Animation", -1 };
int fx_play_visual_effect (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_play_visual_effect (%2d): Resource: %s Type: %d\n", fx->Opcode, fx->Resource, fx->Parameter2 );

	//this is in the original engine (dead actors lose this effect)
	if (STATE_GET( STATE_DEAD) ) {
		return FX_NOT_APPLIED;
	}

	//delay action until area is loaded to avoid crash
	Map *map = target->GetCurrentArea();
	if (!map) return FX_APPLIED;

	//if it is sticky, don't add it if it is already played
	if (fx->Parameter2) {
		ScriptedAnimation *vvc = target->GetVVCCell(fx->Resource);
		if (vvc) {
			vvc->active = true;
			return FX_APPLIED;
		}
		if (! fx->FirstApply) return FX_NOT_APPLIED;
	}

	if (target->fxqueue.HasEffectWithResource(fx_protection_from_animation_ref,fx->Resource)) {
		//effect supressed by opcode 0x128

		return FX_APPLIED;
	}


	ScriptedAnimation* sca = gamedata->GetScriptedAnimation(fx->Resource, false);

	//don't crash on nonexistent resources
	if (!sca) {
		return FX_NOT_APPLIED;
	}

	if (fx->TimingMode!=FX_DURATION_INSTANT_PERMANENT) {
		sca->SetDefaultDuration(fx->Duration-core->GetGame()->GameTime);
	}
	if (fx->Parameter2 == 1) {
		//play over target (sticky)
		sca->SetEffectOwned(true);
		target->AddVVCell( sca );
		return FX_APPLIED;
	}

	//not sticky
	if  (fx->Parameter2 == 2 || !target) {
		sca->XPos = fx->PosX;
		sca->YPos = fx->PosY;
	} else {
		sca->XPos = target->Pos.x;
		sca->YPos = target->Pos.y;
	}
	sca->PlayOnce();
	map->AddVVCell( sca );
	return FX_NOT_APPLIED;
}

//d8 LevelDrainModifier

static EffectRef fx_leveldrain_ref = { "LevelDrainModifier", -1 };

// FIXME: BG2 level drain uses parameter3 to decrease the MaxHp, and parameter4 to decrease level. (unset)
int fx_leveldrain_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_leveldrain_modifier (%2d): Mod: %d\n", fx->Opcode, fx->Parameter1 );

	//never subtract more than the maximum hitpoints
	ieDword x = STAT_GET(IE_MAXHITPOINTS)-1;
	if (fx->Parameter1*4<x) {
		x=fx->Parameter1*4;
	}
	STAT_ADD(IE_LEVELDRAIN, fx->Parameter1);
	STAT_SUB(IE_MAXHITPOINTS, x);
	STAT_SUB(IE_SAVEVSDEATH, fx->Parameter1);
	STAT_SUB(IE_SAVEVSWANDS, fx->Parameter1);
	STAT_SUB(IE_SAVEVSPOLY, fx->Parameter1);
	STAT_SUB(IE_SAVEVSBREATH, fx->Parameter1);
	STAT_SUB(IE_SAVEVSSPELL, fx->Parameter1);
	target->AddPortraitIcon(PI_LEVELDRAIN);
	//decrease current hitpoints on first apply
	if (fx->FirstApply) {
		//current hitpoints don't have base/modified, only current
		BASE_SUB(IE_HITPOINTS, x);
	}
	// TODO: lore, thieving
	return FX_APPLIED;
}

//d9 PowerWordSleep
static EffectRef fx_sleep_ref = { "State:Sleep", -1 };

int fx_power_word_sleep (Scriptable* Owner, Actor* target, Effect* fx)
{
	if (0) print( "fx_power_word_sleep (%2d): HP: %d Stat: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
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
	fx->Opcode = EffectQueue::ResolveEffect(fx_sleep_ref);
	fx->Parameter2=0;
	return fx_set_unconscious_state(Owner,target,fx);
}

static const ieDword fullstone[7]={STONE_GRADIENT,STONE_GRADIENT,STONE_GRADIENT,STONE_GRADIENT,STONE_GRADIENT,STONE_GRADIENT,STONE_GRADIENT};

// 0xda StoneSkinModifier
int fx_stoneskin_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_stoneskin_modifier (%2d): Mod: %d\n", fx->Opcode, fx->Parameter1 );
	if (!fx->Parameter1) {
		PlayRemoveEffect("EFF_E02",target, fx);
		return FX_NOT_APPLIED;
	}

	//dead actors lose this effect
	if (STATE_GET( STATE_DEAD) ) {
		return FX_NOT_APPLIED;
	}

	//this is the bg2 style stoneskin, not normally using spell states
	//but this way we can support hybrid games
	if (fx->Parameter2) {
		target->SetSpellState(SS_IRONSKIN);
		//gradient for iron skins?
	} else {
		target->SetSpellState(SS_STONESKIN);
		SetGradient(target, fullstone);
	}
	STAT_SET(IE_STONESKINS, fx->Parameter1);
	target->AddPortraitIcon(PI_STONESKIN);
	return FX_APPLIED;
}

//0xdb ac vs creature type (general effect)
//0xdc DispelSchool
int fx_dispel_school (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	ieResRef Removed;

	if (0) print( "fx_dispel_school (%2d): Level: %d Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	target->fxqueue.RemoveLevelEffects(Removed, fx->Parameter1, RL_MATCHSCHOOL, fx->Parameter2);
	return FX_NOT_APPLIED;
}
//0xdd DispelSecondaryType
int fx_dispel_secondary_type (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	ieResRef Removed;

	if (0) print( "fx_dispel_secondary_type (%2d): Level: %d Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	target->fxqueue.RemoveLevelEffects(Removed, fx->Parameter1, RL_MATCHSECTYPE, fx->Parameter2);
	return FX_NOT_APPLIED;
}

//0xde RandomTeleport
int fx_teleport_field (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_teleport_field (%2d): Distance: %d\n", fx->Opcode, fx->Parameter1 );

	Map *map = target->GetCurrentArea();
	if (!map) {
		return FX_NOT_APPLIED;
	}
	//the origin is the effect's target point
	Point p(fx->PosX+core->Roll(1,fx->Parameter1*2,-(signed) (fx->Parameter1)),
		fx->PosY+core->Roll(1,fx->Parameter1*2,-(signed) (fx->Parameter1)) );

	target->SetPosition( p, true, 0);
	return FX_NOT_APPLIED;
}

//0xdf //Protection:SchoolDec
int fx_protection_school_dec (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_protection_school_dec (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	if (fx->Parameter1<1) {
		//The original doesn't have anything here
		PlayRemoveEffect(NULL, target, fx);
		return FX_NOT_APPLIED;
	}

	STAT_BIT_OR( IE_IMMUNITY, IMM_SCHOOL_DEC );
	return FX_APPLIED;
}

//0xe0 Cure:LevelDrain

int fx_cure_leveldrain (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_cure_leveldrain (%2d)\n", fx->Opcode );
	//all level drain removed at once???
	//if not, then find old effect, remove a number
	target->fxqueue.RemoveAllEffects( fx_leveldrain_ref );
	return FX_NOT_APPLIED;
}

//0xe1 Reveal:Magic
//gemrb special: speed and color are custom
int fx_reveal_magic (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_reveal_magic (%2d)\n", fx->Opcode );
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
int fx_protection_secondary_type_dec (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_protection_secondary_type_dec (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	if (fx->Parameter1<1) {
		//The original doesn't have anything here
		PlayRemoveEffect(NULL, target, fx);
		return FX_NOT_APPLIED;
	}
	STAT_BIT_OR( IE_IMMUNITY, IMM_SECTYPE_DEC );
	return FX_APPLIED;
}

//0xe3 Bounce:SchoolDecrement
int fx_bounce_school_dec (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_bounce_school_dec (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	if (fx->Parameter1<1) {
		//The original doesn't have anything here
		PlayRemoveEffect(NULL, target, fx);
		return FX_NOT_APPLIED;
	}
	STAT_BIT_OR_PCF( IE_BOUNCE, BNC_SCHOOL_DEC );
	target->AddPortraitIcon(PI_BOUNCE2);
	return FX_APPLIED;
}

//0xe4 Bounce:SecondaryTypeDecrement
int fx_bounce_secondary_type_dec (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_bounce_secondary_type_dec (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	if (fx->Parameter1<1) {
		//The original doesn't have anything here
		PlayRemoveEffect(NULL, target, fx);
		return FX_NOT_APPLIED;
	}
	STAT_BIT_OR_PCF( IE_BOUNCE, BNC_SECTYPE_DEC );
	target->AddPortraitIcon(PI_BOUNCE2);
	return FX_APPLIED;
}

//0xe5 DispelSchoolOne
int fx_dispel_school_one (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	ieResRef Removed;

	if (0) print( "fx_dispel_school_one (%2d): Level: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	target->fxqueue.RemoveLevelEffects(Removed, fx->Parameter1, RL_MATCHSCHOOL|RL_REMOVEFIRST, fx->Parameter2);
	return FX_NOT_APPLIED;
}

//0xe6 DispelSecondaryTypeOne
int fx_dispel_secondary_type_one (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	ieResRef Removed;

	if (0) print( "fx_dispel_secondary_type_one (%2d): Level: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	target->fxqueue.RemoveLevelEffects(Removed, fx->Parameter1, RL_MATCHSECTYPE|RL_REMOVEFIRST, fx->Parameter2);
	return FX_NOT_APPLIED;
}

//0xe7 Timestop
int fx_timestop (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_timestop (%2d)\n", fx->Opcode);
	core->GetGame()->TimeStop(target, fx->Duration);
	return FX_NOT_APPLIED;
}

//0xe8 CastSpellOnCondition
int fx_cast_spell_on_condition (Scriptable* Owner, Actor* target, Effect* fx)
{
	if (0) print( "fx_cast_spell_on_condition (%2d): Target: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
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

		target->spellbook.HaveSpell( fx->Resource, HS_DEPLETE );
		target->spellbook.HaveSpell( fx->Resource2, HS_DEPLETE );
		target->spellbook.HaveSpell( fx->Resource3, HS_DEPLETE );
		target->spellbook.HaveSpell( fx->Resource4, HS_DEPLETE );
	}

	if (fx->Parameter3) {
		target->AddPortraitIcon(PI_CONTINGENCY);
	}

	// TODO: resist source spell, if any

	// get the actor to cast spells at
	Actor *actor = NULL;
	Map *map = target->GetCurrentArea();
	if (!map) return FX_APPLIED;

	switch (fx->Parameter1) {
	case 0:
		// Myself
		actor = target;
		break;
	case 1:
		// LastHitter
		actor = map->GetActorByGlobalID(target->LastHitter);
		break;
	case 2:
		// NearestEnemyOf
		actor = GetNearestEnemyOf(map, target, 0);
		break;
	case 3:
		// Nearest?
		actor = map->GetActorByGlobalID(target->LastSeen);
		break;
	}

	if (!actor) {
		return FX_APPLIED;
	}

	bool condition;
	bool per_round = true; // 4xxx trigger?
	const TriggerEntry *entry = NULL;

	// check the condition
	switch (fx->Parameter2) {
	case COND_GOTHIT:
		// HitBy([ANYONE])
		// TODO: should we ignore this for self-hits in non-contingency mode?
		entry = target->GetMatchingTrigger(trigger_hitby, TEF_PROCESSED_EFFECTS);
		per_round = false;
		break;
	case COND_NEAR:
		// See(NearestEnemyOf())
		// FIXME
		condition = PersonalDistance(actor, target) < 30;
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
		// FIXME
		condition = PersonalDistance(actor, target) < 4;
		break;
	case COND_NEAR10:
		// PersonalSpaceDistance([ANYONE], 10)
		// FIXME
		condition = PersonalDistance(target, actor) < 10;
		break;
	case COND_EVERYROUND:
		condition = true;
		break;
	case COND_TOOKDAMAGE:
		// TookDamage()
		entry = target->GetMatchingTrigger(trigger_tookdamage, TEF_PROCESSED_EFFECTS);
		per_round = false;
		break;
	default:
		condition = false;
	}

	if (per_round) {
		// This is a 4xxx trigger which is only checked every round.
		if (Owner->AdjustedTicks % core->Time.round_size)
			condition = false;
	} else {
		// This is a normal trigger which gets a single opportunity every frame.
		condition = (entry != NULL);
	}

	if (condition) {
		// The trigger was evaluated as true, cast the spells now.
		// TODO: fail remaining spells if an earlier one fails?
		unsigned int i, dist;
		ieResRef refs[4];
		strncpy(refs[0], fx->Resource, sizeof(ieResRef));
		strncpy(refs[1], fx->Resource2, sizeof(ieResRef));
		strncpy(refs[2], fx->Resource3, sizeof(ieResRef));
		strncpy(refs[3], fx->Resource4, sizeof(ieResRef));
		for (i=0; i < 4; i++) {
			if (!refs[i][0]) {
				continue;
			}
			// Actually, atleast fire shields also have a range check
			if (fx->Parameter2 == COND_GOTHIT) {
				dist = GetSpellDistance(refs[i], target);
				if (!dist) {
					//TODO: display 36937
					continue;
				}
				if (PersonalDistance(target, actor) > dist) {
					//display 'One of the spells has failed.'
					displaymsg->DisplayConstantStringName(STR_CONTFAIL, DMC_RED, target);
					continue;
				}
			}
			core->ApplySpell(refs[i], actor, Owner, fx->Power);
		}

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
	if (0) print( "fx_proficiency (%2d): Value: %d, Stat: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	if (fx->Parameter2>=MAX_STATS) return FX_NOT_APPLIED;

	//this opcode works only if the previous value was smaller
	if (STAT_GET(fx->Parameter2)<fx->Parameter1) {
		STAT_SET (fx->Parameter2, fx->Parameter1);
	}
	return FX_APPLIED;
}

// 0xea CreateContingency
static EffectRef fx_contingency_ref = { "CastSpellOnCondition", -1 };

int fx_create_contingency (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_create_contingency (%2d): Level: %d, Count: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	//this effect terminates in cutscene mode
	if (core->InCutSceneMode()) return FX_NOT_APPLIED;

	if (target->fxqueue.HasEffectWithSource(fx_contingency_ref, fx->Source)) {
		displaymsg->DisplayConstantStringName(STR_CONTDUP, DMC_WHITE, target);
		return FX_NOT_APPLIED;
	}

	if (target->InParty) {
		Variables *dict = core->GetDictionary();

		dict->SetAt( "P0", target->InParty );
		dict->SetAt( "P1", fx->Parameter1 );
		dict->SetAt( "P2", fx->Parameter2 );
		core->SetEventFlag(EF_SEQUENCER);
	}
	return FX_NOT_APPLIED;
}

#define WB_AWAY 2
#define WB_TOWARDS 4
#define WB_FIXDIR 5
#define WB_OWNDIR 6
#define WB_AWAYOWNDIR 7

static int coords[16][2]={ {0,12},{-4,9},{-8,6},{-12,3},{-16,0},{-12,-3},{-8,-6},{-4,-9},
{0,-12},{4,-9},{8,-6},{12,-3},{16,0},{12,3},{8,6},{4,9},};

// 0xeb WingBuffet
int fx_wing_buffet (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_wing_buffet (%2d): Mode: %d, Strength: %d\n", fx->Opcode, fx->Parameter2, fx->Parameter1 );

	//creature immunity is based on creature size (though the original is rather cheesy)
	if (target->GetAnims()->GetCircleSize()>5) {
		return FX_NOT_APPLIED;
	}
	if (!target->GetCurrentArea()) {
		//no area, no movement
		return FX_APPLIED;
	}

	Game *game = core->GetGame();

	if (fx->FirstApply) {
		fx->Parameter4 = game->GameTime;
		return FX_APPLIED;
	}

	int ticks = game->GameTime-fx->Parameter4;
	if (!ticks)
		return FX_APPLIED;

	//create movement in actor
	ieDword dir;
	switch(fx->Parameter2) {
		case WB_AWAY:
		default:
			dir = GetOrient(target->Pos, GetCasterObject()->Pos);
			break;
		case WB_TOWARDS:
			dir = GetOrient(GetCasterObject()->Pos, target->Pos);
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
	Point newpos=target->Pos;

	newpos.x += coords[dir][0]*(signed) fx->Parameter1*ticks/16;///AI_UPDATE_TIME;
	newpos.y += coords[dir][1]*(signed) fx->Parameter1*ticks/12;///AI_UPDATE_TIME;

	//change is minimal, lets try later
	if (newpos.x==target->Pos.x && newpos.y==target->Pos.y)
		return FX_APPLIED;

	target->SetPosition(newpos, true, 0);

	fx->Parameter4 = game->GameTime;
	return FX_APPLIED;
}

// 0xec ProjectImage

static EffectRef fx_puppetmarker_ref = { "PuppetMarker", -1 };

int fx_puppet_master (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	const char * resref = NULL;

	if (0) print( "fx_puppet_master (%2d): Value: %d, Stat: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_SET (IE_PUPPETMASTERTYPE, fx->Parameter1);

	//copyself doesn't copy scripts, so the script clearing code is not needed
	Actor *copy = target->CopySelf(fx->Parameter2 == 1);

	Effect *newfx = EffectQueue::CreateUnsummonEffect(fx);
	if (newfx) {
		core->ApplyEffect(newfx, copy, copy);
		delete newfx;
	}

	ieResRef script;

	//intentionally 7, to leave room for the last letter
	strnlwrcpy(script,target->GetScript(SCR_CLASS),7);
	//no need of buffer defense as long as you don't mess with the 7 above
	strcat(script,"m");
	//if the caster is inparty, the script is turned off by the AI disable flag
	copy->SetScript(script, SCR_CLASS, target->InParty!=0);

	switch(fx->Parameter2)
	{
	case 1:
		resref = "mislead";
		//set the gender to illusionary, so ids matching will work
		copy->SetBase(IE_SEX, SEX_ILLUSION);
		copy->SetBase(IE_MAXHITPOINTS, copy->GetBase(IE_MAXHITPOINTS)/2);
		break;
	case 2:
		resref = "projimg";
		copy->SetBase(IE_SEX, SEX_ILLUSION);
		break;
	case 3:
		resref = "simulacr";
		// healable level drain
		// FIXME: second generation simulacri are supposedly at a different level:
		// level = original caster - caster / 2; eg. lvl 32 -> 16 -> 24 -> 20 -> 22 -> 21
		newfx = EffectQueue::CreateEffect(fx_leveldrain_ref, copy->GetXPLevel(1)/2, 0, FX_DURATION_INSTANT_PERMANENT);
		if (newfx) {
			core->ApplyEffect(newfx, copy, copy);
			delete newfx;
		}
		break;
	default:
		resref = fx->Resource;
		break;
	}
	if (resref[0]) {
		core->ApplySpell(resref,copy,copy,0);
	}

	//FIXME: parameter1 is unsure, but something similar to what the original engine has there
	newfx = EffectQueue::CreateEffectCopy(fx, fx_puppetmarker_ref, target->InParty-1, fx->Parameter2);
	if (newfx) {
		core->ApplyEffect(newfx, copy, copy);
		delete newfx;
	}
	return FX_NOT_APPLIED;
}

// 0xed PuppetMarker
int fx_puppet_marker (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_puppet_marker (%2d): Value: %d, Stat: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//actually the Type is in parameter2 and the ID is in parameter1
	//but for some reason the defines are in the opposite order
	STAT_SET (IE_PUPPETTYPE, fx->Parameter1);  //cb4 - the ID of the controller
	STAT_SET (IE_PUPPETID, fx->Parameter2);    //cb8 - the control type
	return FX_APPLIED;
}

// 0xee Disintegrate
int fx_disintegrate (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_disintegrate (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
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

int fx_farsee (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_farsee (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
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
int fx_remove_portrait_icon (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_remove_portrait_icon (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	target->fxqueue.RemoveAllEffectsWithParam( fx_display_portrait_icon_ref, fx->Parameter2 );
	return FX_NOT_APPLIED;
}
// 0xf1 control creature (same as charm)

// 0xF2 Cure:Confusion
static EffectRef fx_confused_state_ref = { "State:Confused", -1 };

int fx_cure_confused_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_cure_confused_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	BASE_STATE_CURE( STATE_CONFUSED );
	target->fxqueue.RemoveAllEffects(fx_confused_state_ref);
	//FIXME:oddly enough, HoW removes the confused icon
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

// 0xf3 DrainItems (this is disabled in ToB)
int fx_drain_items (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_drain_items (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	ieDword i=fx->Parameter1;
	while (i--) {
		//deplete magic item = 0
		//deplete weapon = 1
		target->inventory.DepleteItem(fx->Parameter2);
	}
	return FX_NOT_APPLIED;
}
// 0xf4 DrainSpells
int fx_drain_spells (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_drain_spells (%2d): Count: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
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
int fx_checkforberserk_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_checkforberserk_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_SET( IE_CHECKFORBERSERK, fx->Parameter2 );
	return FX_APPLIED;
}
// 0xf6 BerserkStage1Modifier
int fx_berserkstage1_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_berserkstage1_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_SET( IE_BERSERKSTAGE1, fx->Parameter2 );
	return FX_APPLIED;
}
// 0xf7 BerserkStage2Modifier
int fx_berserkstage2_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_berserkstage2_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_SET( IE_BERSERKSTAGE2, fx->Parameter2 );
	STATE_SET (STATE_BERSERK);
	return FX_APPLIED;
}

// 0xf8 set melee effect
// adds effect to melee attacks (for monks, asssasins, fighter hlas, ...)
// it is cumulative

// 0xf9 set missile effect
// adds effect to ranged attacks (archers, ...)
// it is cumulative

// 0xfa DamageLuckModifier
int fx_damageluck_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_damageluck_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_DAMAGELUCK );
	return FX_APPLIED;
}

// 0xfb BardSong

int fx_change_bardsong (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_change_bardsong (%2d): %s\n", fx->Opcode, fx->Resource);
	memcpy(target->BardSong, fx->Resource, 8);
	return FX_APPLIED;
}

// 0xfc SetTrap
int fx_set_area_effect (Scriptable* Owner, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_trap (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	ieDword skill, roll;
	Map *map;

	map = target->GetCurrentArea();
	if (!map) return FX_NOT_APPLIED;

	proIterator iter;

	//check if trap count is over an amount (only saved traps count)
	//actually, only projectiles in trigger phase should count here
	if (map->GetTrapCount(iter)>6) {
		displaymsg->DisplayConstantStringName(STR_NOMORETRAP, DMC_WHITE, target);
		return FX_NOT_APPLIED;
	}

	//check if we are under attack
	if (GetNearestEnemyOf(map, target, ORIGIN_SEES_ENEMY|ENEMY_SEES_ORIGIN)) {
		displaymsg->DisplayConstantStringName(STR_MAYNOTSETTRAP, DMC_WHITE, target);
		return FX_NOT_APPLIED;
	}

	if (Owner->Type==ST_ACTOR) {
		skill = ((Actor *)Owner)->GetStat(IE_SETTRAPS);
		roll = target->LuckyRoll(1,100,0,LR_NEGATIVE);
	} else {
		roll=0;
		skill=0;
	}

	if (roll>skill) {
		//failure
		displaymsg->DisplayConstantStringName(STR_SNAREFAILED, DMC_WHITE, target);
		if (target->LuckyRoll(1,100,0)<25) {
			ieResRef spl;

			strnuprcpy(spl, fx->Resource, 8);
			if (strlen(spl)<8) {
				strcat(spl,"F");
			} else {
				spl[7]='F';
			}
			core->ApplySpell(spl, target, Owner, fx->Power);
		}
		return FX_NOT_APPLIED;
	}
	//success
	displaymsg->DisplayConstantStringName(STR_SNARESUCCEED, DMC_WHITE, target);
	// save the current spell ref, so the rest of its effects can be applied afterwards
	ieResRef OldSpellResRef;
	memcpy(OldSpellResRef, Owner->SpellResRef, sizeof(OldSpellResRef));
	Owner->SetSpellResRef(fx->Resource);
	Owner->CastSpellPoint(target->Pos, false);
	Owner->CastSpellPointEnd(0);
	Owner->SetSpellResRef(OldSpellResRef);
	return FX_NOT_APPLIED;
}

// 0xfd SetMapNote
int fx_set_map_note (Scriptable* Owner, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_map_note (%2d): StrRef: %d Color: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	Scriptable *marker = target?target:Owner;
	Map *map = marker->GetCurrentArea();
	if (!map) return FX_APPLIED; //delay effect
	Point p(fx->PosX, fx->PosY);
	char *text = core->GetString(fx->Parameter1, 0);
	map->AddMapNote(p, fx->Parameter2, text, fx->Parameter1);
	return FX_NOT_APPLIED;
}

// 0xfe RemoveMapNote
int fx_remove_map_note (Scriptable* Owner, Actor* target, Effect* fx)
{
	if (0) print( "fx_remove_map_note (%2d)\n", fx->Opcode);
	Scriptable *marker = target?target:Owner;
	Map *map = marker->GetCurrentArea();
	if (!map) return FX_APPLIED; //delay effect
	Point p(fx->PosX, fx->PosY);
	map->RemoveMapNote(p);
	return FX_NOT_APPLIED;
}

// 0xff Item:CreateDays
int fx_create_item_days (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_create_item_days (%2d)\n", fx->Opcode );
	target->inventory.AddSlotItemRes( fx->Resource, SLOT_ONLYINVENTORY, fx->Parameter1, fx->Parameter3, fx->Parameter4 );
	if ((fx->TimingMode&0xff) == FX_DURATION_INSTANT_LIMITED) {
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
int fx_store_spell_sequencer(Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_store_spell_sequencer (%2d)\n", fx->Opcode );
	//just display the spell sequencer portrait icon
	target->AddPortraitIcon(PI_SEQUENCER);
	if (fx->FirstApply && fx->Parameter3) {
		target->spellbook.HaveSpell( fx->Resource, HS_DEPLETE );
		target->spellbook.HaveSpell( fx->Resource2, HS_DEPLETE );
		target->spellbook.HaveSpell( fx->Resource3, HS_DEPLETE );
		target->spellbook.HaveSpell( fx->Resource4, HS_DEPLETE );
	}
	return FX_APPLIED;
}

// 0x101 Sequencer:Create
static EffectRef fx_spell_sequencer_active_ref = { "Sequencer:Store", -1 };

int fx_create_spell_sequencer(Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_create_spell_sequencer (%2d): Level: %d, Count: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	if (target->fxqueue.HasEffectWithSource(fx_spell_sequencer_active_ref, fx->Source)) {
		displaymsg->DisplayConstantStringName(STR_SEQDUP, DMC_WHITE, target);
		return FX_NOT_APPLIED;
	}
	//just a call to activate the spell sequencer creation gui
	if (target->InParty) {
		Variables *dict = core->GetDictionary();

		dict->SetAt( "P0", target->InParty );
		dict->SetAt( "P1", fx->Parameter1 );           //maximum level
		dict->SetAt( "P2", fx->Parameter2 | (2<<16) ); //count and target type
		core->SetEventFlag(EF_SEQUENCER);
	}
	return FX_NOT_APPLIED;
}

// 0x102 Sequencer:Activate

int fx_activate_spell_sequencer(Scriptable* Owner, Actor* target, Effect* fx)
{
	if (0) print( "fx_activate_spell_sequencer (%2d): Resource: %s\n", fx->Opcode, fx->Resource );
	if (Owner->Type!=ST_ACTOR) {
		return FX_NOT_APPLIED;
	}

	Effect *sequencer = ((Actor *) Owner)->fxqueue.HasEffect(fx_spell_sequencer_active_ref);
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
int fx_spelltrap(Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_spelltrap (%2d): Count: %d, Level: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	if (fx->Parameter3) {
		target->RestoreSpellLevel(fx->Parameter3, 0);
		fx->Parameter3 = 0;
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
int fx_crash (Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) print( "fx_crash (%2d): Param1: %d, Param2: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	return FX_NOT_APPLIED;
}

// 0x105 RestoreSpells
int fx_restore_spell_level(Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_restore_spell_level (%2d): Level: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	target->RestoreSpellLevel(fx->Parameter1, fx->Parameter2);
	return FX_NOT_APPLIED;
}
// 0x106 VisualRangeModifier
int fx_visual_range_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_visual_range_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_VISUALRANGE );
	return FX_APPLIED;
}

// 0x107 BackstabModifier
int fx_backstab_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_visual_range_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
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
int fx_modify_global_variable (Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
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

	if (0) print( "fx_modify_global_variable (%2d): Variable: %s Value: %d Type: %d\n", fx->Opcode, fx->Resource, fx->Parameter1, fx->Parameter2 );
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
static EffectRef immunity_effect_ref = { "Protection:Spell", -1 };

int fx_remove_immunity(Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_remove_immunity (%2d): %s\n", fx->Opcode, fx->Resource );
	target->fxqueue.RemoveAllEffectsWithResource(immunity_effect_ref, fx->Resource);
	return FX_NOT_APPLIED;
}

// 0x10b protection from display string is a generic effect
// 0x10c ExploreModifier
int fx_explore_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_explore_modifier (%2d)\n", fx->Opcode );
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
	if (0) print( "fx_screenshake (%2d): Strength: %d\n", fx->Opcode, fx->Parameter1 );
	unsigned long count;

	if (fx->TimingMode!=FX_PERMANENT) {
		count = fx->Duration-core->GetGame()->GameTime;
	} else {
		count = core->Time.round_size;
	}
	int x,y;

	switch(fx->Parameter2) {
	case 0: default:
		x=fx->Parameter1;
		y=fx->Parameter1;
		break;
	case 1:
		x=fx->Parameter1;
		y=-fx->Parameter1;
		break;
	case 2:
		//gemrb addition
		x=(short) (fx->Parameter1&0xffff);
		y=(short) (fx->Parameter1>>16);
		break;
	}
	core->timer->SetScreenShake( x, y, count);
	return FX_NOT_APPLIED;
}

// 0x10e Cure:CasterHold
static EffectRef fx_pause_caster_modifier_ref = { "PauseTarget", -1 };

int fx_unpause_caster (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_unpause_caster (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	target->fxqueue.RemoveAllEffects(fx_pause_caster_modifier_ref);
	return FX_NOT_APPLIED;
}

// 0x10f SummonDisable (bg2)
int fx_summon_disable (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_summon_disable (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_SET(IE_SUMMONDISABLE, 1);
	STAT_SET(IE_CASTERHOLD, 1);
	if (fx->Parameter2==1) {
		STAT_SET(IE_AVATARREMOVAL, 1);
	}
	return FX_APPLIED;
}

/* note/TODO from Taimon:
What happens at a lower level is that the engine recreates the entire stats on some changes to the creature. The application of an effect to the creature is one such change.
Since the repeating effects are stored in a list inside those stats, they are being recreated every ai update, if there has been an effect application.
The repeating effect itself internally uses a counter to store how often it has been called. And when this counter equals the period it fires of the effect. When the list is being recreated all those counters are lost.
*/
static EffectRef fx_apply_effect_repeat_ref = { "ApplyEffectRepeat", -1 };
// 0x110 ApplyEffectRepeat
int fx_apply_effect_repeat (Scriptable* Owner, Actor* target, Effect* fx)
{
	ieDword i; //moved here because msvc6 cannot handle it otherwise

	if (0) print( "fx_apply_effect_repeat (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	Point p(fx->PosX, fx->PosY);
	Effect *newfx = core->GetEffect(fx->Resource, fx->Power, p);
	//core->GetEffect is a borrowed reference, don't delete it
	if (!newfx) {
		return FX_NOT_APPLIED;
	}

	// don't apply the effect if a similar one is already applied with a shorter duration
	Effect *oldfx = target->fxqueue.HasEffect(fx_apply_effect_repeat_ref);
	if (oldfx && oldfx->Duration < fx->Duration) {
		return FX_NOT_APPLIED;
	}

	switch (fx->Parameter2) {
		case 0: //once per second
		case 1: //crash???
			if (!(core->GetGame()->GameTime%AI_UPDATE_TIME)) {
				core->ApplyEffect(newfx, target, Owner);
			}
			break;
		case 2://param1 times every second
			if (!(core->GetGame()->GameTime%AI_UPDATE_TIME)) {
				for (i=0;i<fx->Parameter1;i++) {
					core->ApplyEffect(newfx, target, Owner);
				}
			}
			break;
		case 3: //once every Param1 second
			if (fx->Parameter1 && !(core->GetGame()->GameTime%(fx->Parameter1*AI_UPDATE_TIME))) {
				core->ApplyEffect(newfx, target, Owner);
			}
			break;
		case 4: //param3 times every Param1 second
			if (fx->Parameter1 && !(core->GetGame()->GameTime%(fx->Parameter1*AI_UPDATE_TIME))) {
				for (i=0;i<fx->Parameter3;i++) {
					core->ApplyEffect(newfx, target, Owner);
				}
			}
			break;
	}
	return FX_APPLIED;
}

// 0x111 RemoveProjectile
int fx_remove_projectile (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	//the list is now cached by Interface, no need of freeing it
	ieDword *projectilelist;

	//instant effect
	if (0) print( "fx_remove_projectile (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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

	int i = projectilelist[0];

	while(i) {
		ieDword projectile = projectilelist[i];
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
		i--;
	}
	//this one was constructed by us
	if (fx->Parameter2==2) free(projectilelist);
	return FX_NOT_APPLIED;
}

// 0x112 TeleportToTarget
int fx_teleport_to_target (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_teleport_to_target (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	if (STATE_GET(STATE_DEAD)) {
		return FX_NOT_APPLIED;
	}

	Map *map = target->GetCurrentArea();
	if (map) {
		Object oC;
		oC.objectFields[0]=EA_ENEMY;
		Targets *tgts = GetAllObjects(map, target, &oC, GA_NO_DEAD);
		int rnd = core->Roll(1,tgts->Count(),-1);
		Actor *victim = (Actor *) tgts->GetTarget(rnd, ST_ACTOR);
		delete tgts;
		if (victim && PersonalDistance(victim, target)>20) {
			target->SetPosition( victim->Pos, true, 0 );
			target->SetColorMod(0xff, RGBModifier::ADD, 0x50, 0xff, 0xff, 0xff, 0);
		}
	}
	return FX_NOT_APPLIED;
}

// 0x113 HideInShadowsModifier
int fx_hide_in_shadows_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_hide_in_shadows_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_HIDEINSHADOWS );
	return FX_APPLIED;
}

// 0x114 DetectIllusionsModifier
int fx_detect_illusion_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_detect_illusion_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_DETECTILLUSIONS );
	return FX_APPLIED;
}

// 0x115 SetTrapsModifier
int fx_set_traps_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_set_traps_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_SETTRAPS );
	return FX_APPLIED;
}
// 0x116 ToHitBonusModifier
int fx_to_hit_bonus_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_to_hit_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	HandleBonus( target, IE_HITBONUS, fx->Parameter1, fx->TimingMode );
	return FX_APPLIED;
}

// 0x117 RenableButton
static EffectRef fx_disable_button_ref = { "DisableButton", -1 };

int fx_renable_button (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	//removes the disable button effect
	if (0) print( "fx_renable_button (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	target->fxqueue.RemoveAllEffectsWithParamAndResource( fx_disable_button_ref, fx->Parameter2, fx->Resource );
	return FX_NOT_APPLIED;
}

// 0x118 ForceSurgeModifier
int fx_force_surge_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_force_surge_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD_VAR( IE_FORCESURGE, MOD_ABSOLUTE );
	return FX_APPLIED;
}

// 0x119 WildSurgeModifier
int fx_wild_surge_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_wild_surge_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_SURGEMOD );
	return FX_APPLIED;
}

// 0x11a ScriptingState
int fx_scripting_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_scripting_state (%2d): Value: %d, Stat: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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
	if (0) print( "fx_apply_effect_curse (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	//this effect executes a file effect in place of this effect
	//the file effect inherits the target and the timingmode, but gets
	//a new chance to roll percents
	int ret = FX_NOT_APPLIED;
	if (!target) {
		return ret;
	}

	if (EffectQueue::match_ids( target, fx->Parameter2, fx->Parameter1) ) {
		Point p(fx->PosX, fx->PosY);

		//apply effect, if the effect is a goner, then kill
		//this effect too
		Effect *newfx = core->GetEffect(fx->Resource, fx->Power, p);
		if (newfx) {
			Effect *myfx = new Effect;
			memcpy(myfx, newfx, sizeof(Effect));
			myfx->random_value = fx->random_value;
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
	if (0) print( "fx_melee_to_hit_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_MELEETOHIT );
	return FX_APPLIED;
}

// 0x11d MeleeDamageModifier
int fx_melee_damage_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_melee_damage_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_MELEEDAMAGE );
	return FX_APPLIED;
}

// 0x11e MissileDamageModifier
int fx_missile_damage_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_missile_damage_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_MISSILEDAMAGE );
	return FX_APPLIED;
}

// 0x11f NoCircleState
int fx_no_circle_state (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_no_circle_state (%2d)\n", fx->Opcode);
	STAT_SET( IE_NOCIRCLE, 1 );
	return FX_APPLIED;
}

// 0x120 FistHitModifier
int fx_fist_to_hit_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_fist_to_hit_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_FISTHIT );
	return FX_APPLIED;
}

// 0x121 FistDamageModifier
int fx_fist_damage_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_fist_damage_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_FISTDAMAGE );
	return FX_APPLIED;
}
//0x122 TitleModifier
int fx_title_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_fist_damage_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
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
int fx_disable_overlay_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_disable_overlay_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_SET( IE_DISABLEOVERLAY, fx->Parameter1 );
	return FX_APPLIED;
}
//0x124 Protection:Backstab (bg2)
//0x11f Protection:Backstab (how, iwd2)
//3 different games, 3 different methods of flagging this
int fx_no_backstab_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_no_backstab_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//bg2
	STAT_SET( IE_DISABLEBACKSTAB, fx->Parameter1 );
	//how
	EXTSTATE_SET(EXTSTATE_NO_BACKSTAB);
	//iwd2
	target->SetSpellState(SS_NOBACKSTAB);
	return FX_APPLIED;
}
//0x125 OffscreenAIModifier
int fx_offscreenai_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_offscreenai_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_SET( IE_ENABLEOFFSCREENAI, fx->Parameter1 );
	target->Activate();
	return FX_APPLIED;
}
//0x126 ExistanceDelayModifier
int fx_existance_delay_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_existance_delay_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_SET( IE_EXISTANCEDELAY, fx->Parameter1 );
	return FX_APPLIED;
}
//0x127 DisableChunk
int fx_disable_chunk_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_disable_chunk_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_SET( IE_DISABLECHUNKING, fx->Parameter1 );
	return FX_APPLIED;
}
#if 0
//This is done differently in the original engine, and THIS may not even work
//0x128 Protection:Animation
int fx_protection_from_animation (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_protection_from_animation (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//remove vvc from actor if active
	target->RemoveVVCell(fx->Resource, false);
	return FX_APPLIED;
}
#endif

//0x129 Protection:Turn
int fx_protection_from_turn (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_non_interruptible_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_SET( IE_NOTURNABLE, fx->Parameter1 );
	return FX_APPLIED;
}
//0x12a CutScene2
//runs a predetermined script in cutscene mode
int fx_cutscene2 (Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	Game *game;
	ieResRef resref;

	if (0) print( "fx_cutscene2 (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	if (core->InCutSceneMode()) return FX_NOT_APPLIED;
	game = core->GetGame();
	if (!game) return FX_NOT_APPLIED;

	game->ClearPlaneLocations();
	for (int i = 0; i < game->GetPartySize(false); i++) {
		Actor* act = game->GetPC( i, false );
		GAMLocationEntry *gle = game->GetPlaneLocationEntry(i);
		if (act && gle) {
			gle->Pos = act->Pos;
			memcpy(gle->AreaResRef, act->Area, 9);
		}
	}

	core->SetCutSceneMode(true);

	//GemRB enhancement: allow a custom resource
	if (fx->Parameter2) {
		strnlwrcpy(resref,fx->Resource, 8);
	} else {
		strnlwrcpy(resref,"cut250a",8);
	}

	GameScript* gs = new GameScript( resref, game );
	gs->EvaluateAllBlocks();
	delete( gs );
	return FX_NOT_APPLIED;
}
//0x12b ChaosShieldModifier
int fx_chaos_shield_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_chaos_shield_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_ADD( IE_CHAOSSHIELD, fx->Parameter1 );
	if (fx->Parameter2) {
		target->AddPortraitIcon(PI_CSHIELD); //162
	} else {
		target->AddPortraitIcon(PI_CSHIELD2); //163
	}
	return FX_APPLIED;
}
//0x12c NPCBump
int fx_npc_bump (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_npc_bump (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//unknown effect, but known stat position
	STAT_MOD( IE_NPCBUMP );
	return FX_APPLIED;
}
//0x12d CriticalHitModifier
int fx_critical_hit_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_critical_hit_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_CRITICALHITBONUS );
	return FX_APPLIED;
}
// 0x12e CanUseAnyItem
int fx_can_use_any_item_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_can_use_any_item_modifier (%2d): Value: %d\n", fx->Opcode, fx->Parameter2 );

	STAT_SET( IE_CANUSEANYITEM, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x12f AlwaysBackstab
int fx_always_backstab_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_always_backstab_modifier (%2d): Value: %d\n", fx->Opcode, fx->Parameter2 );

	STAT_SET( IE_ALWAYSBACKSTAB, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x130 MassRaiseDead
int fx_mass_raise_dead (Scriptable* Owner, Actor* /*target*/, Effect* fx)
{
	if (0) print( "fx_mass_raise_dead (%2d)\n", fx->Opcode );

	Game *game=core->GetGame();

	int i=game->GetPartySize(false);
	Point p(fx->PosX,fx->PosY);
	while (i--) {
		Actor *actor=game->GetPC(i,false);
		Resurrect(Owner, actor, fx, p);
	}
	return FX_NOT_APPLIED;
}

// 0x131 OffhandHitModifier
int fx_left_to_hit_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_left_to_hit_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_HITBONUSLEFT );
	return FX_APPLIED;
}

// 0x132 RightHitModifier
int fx_right_to_hit_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_right_to_hit_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_HITBONUSRIGHT );
	return FX_APPLIED;
}

// 0x133 Reveal:Tracks
int fx_reveal_tracks (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_reveal_tracks (%2d): Distance: %d\n", fx->Opcode, fx->Parameter1 );
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
int fx_protection_from_tracking (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_protection_from_tracking (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_NOTRACKING ); //highlight creature???
	return FX_APPLIED;
}
// 0x135 ModifyLocalVariable
int fx_modify_local_variable (Scriptable* /*Owner*/, Actor* target, Effect* fx)
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
	if (0) print( "fx_modify_local_variable (%2d): %s, Mod: %d\n", fx->Opcode, fx->Resource, fx->Parameter2 );
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
int fx_timeless_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_timeless_modifier (%2d): Mod: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_SET(IE_DISABLETIMESTOP, fx->Parameter2);
	return FX_APPLIED;
}

//0x137 GenerateWish
//GemRB extension: you can use different tables and not only wisdom stat
int fx_generate_wish (Scriptable* Owner, Actor* target, Effect* fx)
{
	ieResRef spl;

	if (0) print( "fx_generate_wish (%2d): Mod: %d\n", fx->Opcode, fx->Parameter2 );
	if (!fx->Parameter2) {
		fx->Parameter2=IE_WIS;
	}
	int stat = target->GetSafeStat(fx->Parameter2);
	if (!fx->Resource[0]) {
		memcpy(fx->Resource,"wishcode",8);
	}
	AutoTable tm(fx->Resource);
	if (!tm) {
		return FX_NOT_APPLIED;
	}

	int max = tm->GetRowCount();
	int tmp = core->Roll(1,max,0);
	int i = tmp;
	bool pass = true;
	while(--i!=tmp && pass) {
		if (i<0) {
			pass=false;
			i=max-1;
		}
		int min = atoi(tm->QueryField(i, 1));
		int max = atoi(tm->QueryField(i, 2));
		if (stat>=min && stat<=max) break;
	}
	strnuprcpy(spl, tm->QueryField(i,0), 8);
	core->ApplySpell(spl, target, Owner, fx->Power);
	return FX_NOT_APPLIED;
}

//0x138 //see fx_crash, this effect is not fully enabled in original bg2/tob
int fx_immunity_sequester (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_immunity_sequester (%2d): Mod: %d\n", fx->Opcode, fx->Parameter2 );
	//this effect is supposed to provide immunity against sequester (maze/etc?)
	STAT_SET(IE_NOSEQUESTER, fx->Parameter2);
	return FX_APPLIED;
}

//0x139 //HLA generic effect
//0x13a StoneSkin2Modifier
int fx_golem_stoneskin_modifier (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_golem_stoneskin_modifier (%2d): Mod: %d\n", fx->Opcode, fx->Parameter1 );
	if (!fx->Parameter1) {
		PlayRemoveEffect("EFF_E02",target, fx);
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
	if (0) print( "fx_avatar_removal_modifier (%2d): Mod: %d\n", fx->Opcode, fx->Parameter2 );
	STAT_SET(IE_AVATARREMOVAL, fx->Parameter2);
	return FX_APPLIED;
}

// 0x13c MagicalRest (also 0x124 iwd)
int fx_magical_rest (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) print( "fx_magical_rest (%2d)\n", fx->Opcode );
	//instant, full rest
	target->Rest(0);
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
	print( "fx_change_weather (%2d): P1: %d\n", fx->Opcode, fx->Parameter1 );

	core->GetGame()->StartRainOrSnow(false, fx->Parameter1 & WB_MASK);

	return FX_NOT_APPLIED;
}

// unknown
int fx_unknown (Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	print( "fx_unknown (%2d): P1: %d P2: %d ResRef: %s\n", fx->Opcode, fx->Parameter1, fx->Parameter2, fx->Resource );
	return FX_NOT_APPLIED;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x1AAA040A, "Effect opcodes for core games")
PLUGIN_INITIALIZER(RegisterCoreOpcodes)
PLUGIN_CLEANUP(Cleanup)
END_PLUGIN()
