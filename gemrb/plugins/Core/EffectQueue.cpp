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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/EffectQueue.cpp,v 1.39 2005/08/01 20:52:14 avenger_teambg Exp $
 *
 */

#include <stdio.h>
#include "Interface.h"
#include "Actor.h"
#include "Effect.h"
#include "EffectQueue.h"

//these effects never stick around, use them for instant effects like damage
#define FX_NOT_APPLIED 0 
//these effects always stick around when applied as permanent or duration
#define FX_APPLIED 1	
//these effects don't stick around if used as permanent, in that case they modify a base stat
//like charisma modifier
#define FX_PERMANENT 2
//these effects stick and also repeatedly trigger like poison
#define FX_CYCLIC    3

int fx_ac_vs_damage_type_modifier (Actor* Owner, Actor* target, Effect* fx);//00
int fx_attacks_per_round_modifier (Actor* Owner, Actor* target, Effect* fx);//01
int fx_cure_sleep_state (Actor* Owner, Actor* target, Effect* fx);//02
int fx_set_berserk_state (Actor* Owner, Actor* target, Effect* fx);//03
int fx_cure_berserk_state (Actor* Owner, Actor* target, Effect* fx);//04
int fx_set_charmed_state (Actor* Owner, Actor* target, Effect* fx);//05
int fx_charisma_modifier (Actor* Owner, Actor* target, Effect* fx);//06
int fx_set_color_gradient (Actor* Owner, Actor* target, Effect* fx);//07
//08
//09
int fx_constitution_modifier (Actor* Owner, Actor* target, Effect* fx);//0a
int fx_cure_poisoned_25_state (Actor* Owner, Actor* target, Effect* fx);//0b
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
//29
int fx_bonus_wizard_spells (Actor* Owner, Actor* target, Effect* fx);//2a
int fx_cure_petrified_state (Actor* Owner, Actor* target, Effect* fx);//2b
int fx_strength_modifier (Actor* Owner, Actor* target, Effect* fx);//2c
int fx_set_stun_state (Actor* Owner, Actor* target, Effect* fx);//2d
int fx_cure_stun_state (Actor* Owner, Actor* target, Effect* fx);//2e
int fx_cure_invisible_state (Actor* Owner, Actor* target, Effect* fx);//2f
int fx_cure_silenced_state (Actor* Owner, Actor* target, Effect* fx);//30
int fx_wisdom_modifier (Actor* Owner, Actor* target, Effect* fx);//31
//32
//33
//34
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
int fx_cure_diseased_78_state (Actor* Owner, Actor* target, Effect* fx);//4f
int fx_set_deaf_state (Actor* Owner, Actor* target, Effect* fx); //50
int fx_cure_deaf_80_state (Actor* Owner, Actor* target, Effect* fx);//51
int fx_set_ai_script (Actor* Owner, Actor* target, Effect*fx);//52
//53
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
//60
int fx_strength_bonus_modifier (Actor* Owner, Actor* target, Effect* fx);//61
//62
int fx_spell_duration_modifier (Actor* Owner, Actor* target, Effect* fx);///63
//64
//65
//66
int fx_change_name (Actor* Owner, Actor* target, Effect* fx);//67
int fx_experience_modifier (Actor* Owner, Actor* target, Effect* fx);//68
int fx_gold_modifier (Actor* Owner, Actor* target, Effect* fx);//69
int fx_morale_break_modifier (Actor* Owner, Actor* target, Effect* fx);//6a
int fx_portrait_change (Actor* Owner, Actor* target, Effect* fx);//6b
int fx_reputation_modifier (Actor* Owner, Actor* target, Effect* fx);//6c
//6d
//6e
//6f
//70
//71
//72
//73
int fx_cure_improved_invisible_state (Actor* Owner, Actor* target, Effect* fx);//74
//75
//76
//77
//78
//79
//7a
//7b
//7c
//7d
int movement_modifier (Actor* Owner, Actor* target, Effect* fx);//7e
//7f
int fx_set_confused_state (Actor* Owner, Actor* target, Effect* fx);//80
//81
//82
//83
//84
//85
int fx_set_petrified_state (Actor* Owner, Actor* target, Effect* fx);//86
//87
//88
//89
//8a
int fx_display_string (Actor* Owner, Actor* target, Effect* fx);//8b
//8c
//8d
//8f
//90
//91
//92
int fx_learn_spell (Actor* Owner, Actor* target, Effect *fx);//93
//94
//95
//96
//97
int fx_play_movie (Actor* Owner, Actor* target, Effect* fx);//98
int fx_set_sanctuary_state (Actor* Owner, Actor* target, Effect* fx);//99
//9a
//9b
//9c
//9d
//9e
//9f
int fx_cure_sanctuary_state (Actor* Owner, Actor* target, Effect* fx);//a0
int fx_cure_panic_state (Actor* Owner, Actor* target, Effect* fx);//a1
int fx_cure_hold_175_state (Actor* Owner, Actor* target, Effect* fx);//a2
int fx_cure_slow_state (Actor* Owner, Actor* target, Effect* fx);//a3
//a4
int fx_pause_target (Actor* Owner, Actor* target, Effect* fx);//a5
int fx_magic_resistance_modifier (Actor* Owner, Actor* target, Effect* fx);//a6
//a7
//a8
//a9
//aa
//ab
//ac
//ad
int fx_playsound (Actor* Owner, Actor* target, Effect* fx);//ae
int fx_hold_creature (Actor* Owner, Actor* target, Effect* fx);//af
// this function is exactly the same as 0x7e movement_modifier (in bg2 at least)//b0
//b1
//b2
//b3
//b4
//b5
//b6
//b7
//b8
// this function is exactly the same as 0xaf hold_creature (in bg2 at least) //b9
int fx_destroy_self (Actor* Owner, Actor* target, Effect* fx);//ba
int fx_local_variable (Actor* Owner, Actor* target, Effect* fx);//bb
//bc
//bd
//be
//bf
//c0
//c1
//c2
//c3
//c4
//c5
//c6
//c7
//c8
//c9
//ca
//cb
//cc
//cd
//ce
//cf
int fx_minimum_hp_modifier (Actor* Owner, Actor* target, Effect* fx);//d0
//d1
//d2
//d3
//d4
//d5
//d6
int fx_play_visual_effect (Actor* Owner, Actor* target, Effect* fx); //d7
//d8
//d9
//da
//db
//dc
//dd
//de
//df
//e0
//e1
//e2
//e3
//e4
//e5
//e6
//e7
//e8
int fx_proficiency (Actor* Owner, Actor* target, Effect* fx);//e9
//ea
//eb
//ec
//ed
//ee
//ef
//f0
//f1
int fx_cure_confused_state (Actor* Owner, Actor* target, Effect* fx);//f2
//f3
//f4
//f5
//f6
//f7
//f8
//f9
//fa
//fb
//fc
//fd
//fe
//ff
//100
//101
//102
//103
//104
//105
//106
//107
//108
//109
//10a
//10b
//10c
//10d
//10e
//10f
//110
//111
//112
//113
//114
//115
//116
//117
//118
//119
//11a
//11b
//11c
//11d
//11e
//11f
//120
//121
//122
//123
//124
//125
int fx_scripting_state (Actor* Owner, Actor* target, Effect* fx);//126
//127
//128
//129
//......

struct EffectRef {
	const char* Name;
	EffectFunction Function;
	int EffText;
};

static int initialized = 0;
static EffectRef effect_refs[MAX_EFFECTS];
//static int efftexts[MAX_EFFECTS]; //from efftext.2da

// FIXME: this list should be dynamic (stl::vector). It should be populated
// by fx plugins, so it would be easier to add new effects etc.
// FIXME: Make this an ordered list, so we could use bsearch!
static EffectLink effectnames[] = {
	{ "AcidResistanceModifier", fx_acid_resistance_modifier },
	{ "ACVsDamageTypeModifier", fx_ac_vs_damage_type_modifier },
	{ "AIIdentifierModifier", fx_ids_modifier },
	{ "AlchemyModifier", fx_alchemy_modifier },
	{ "Alignment:Change", fx_alignment_change },
	{ "Alignment:Invert", fx_alignment_invert },
	{ "AnimationIDModifier", fx_animation_id_modifier },
	{ "AttacksPerRoundModifier", fx_attacks_per_round_modifier },
	{ "ChangeName", fx_change_name },
	{ "CharismaModifier", fx_charisma_modifier },
	{ "ColdResistanceModifier", fx_cold_resistance_modifier },
	{ "Color:SetCharacterColorsByPalette", fx_set_color_gradient },
	{ "ConstitutionModifier", fx_constitution_modifier },
	{ "CrushingResistanceModifier", fx_crushing_resistance_modifier },
	{ "Cure:Berserk", fx_cure_berserk_state },
	{ "Cure:Blind", fx_cure_blind_state },
	{ "Cure:Confusion", fx_cure_confused_state },
	{ "Cure:Deafness", fx_cure_deaf_80_state },
	{ "Cure:Death", fx_cure_dead_state },
	{ "Cure:Defrost", fx_cure_frozen_state },
	{ "Cure:Disease", fx_cure_diseased_78_state },
	{ "Cure:Feeblemind", fx_cure_feebleminded_state },
	{ "Cure:Hold", fx_cure_hold_175_state },
	{ "Cure:Infravision", fx_cure_infravision_state },
	{ "Cure:Invisible", fx_cure_invisible_state },
	{ "Cure:ImprovedInvisible", fx_cure_improved_invisible_state },
	{ "Cure:Nondetection", fx_cure_nondetection_state },
	{ "Cure:Panic", fx_cure_panic_state },
	{ "Cure:Petrification", fx_cure_petrified_state },
	{ "Cure:Poison", fx_cure_poisoned_25_state },
	{ "Cure:Sanctuary", fx_cure_sanctuary_state },
	{ "Cure:Silence", fx_cure_silenced_state },
	{ "Cure:Sleep", fx_cure_sleep_state },
	{ "Cure:Stun", fx_cure_stun_state },
	{ "CurrentHPModifier", fx_current_hp_modifier },
	{ "Damage", fx_damage },
	{ "DamageBonusModifier", fx_damage_bonus_modifier },
	{ "Death", fx_death },
	{ "DestroySelf", fx_destroy_self },
	{ "DexterityModifier", fx_dexterity_modifier },
	{ "DispelEffects", fx_dispel_effects },
	{ "DisplayString", fx_display_string },
	{ "ElectricityResistanceModifier", fx_electricity_resistance_modifier },
	{ "ExperienceModifier", fx_experience_modifier },
	{ "FatigueModifier", fx_fatigue_modifier },
	{ "FindTrapsModifier", fx_find_traps_modifier },
	{ "FireResistanceModifier", fx_fire_resistance_modifier },
	{ "GoldModifier", fx_gold_modifier },
	{ "IntelligenceModifier", fx_intelligence_modifier },
	{ "IntoxicationModifier", fx_intoxication_modifier },
	{ "KillCreatureType", fx_kill_creature_type },
	{ "LearnSpell", fx_learn_spell },
	{ "LoreModifier", fx_lore_modifier },
	{ "LuckModifier", fx_luck_modifier },
	{ "MagicalColdResistanceModifier", fx_magical_cold_resistance_modifier },
	{ "MagicalFireResistanceModifier", fx_magical_fire_resistance_modifier },
	{ "MagicDamageResistanceModifier", fx_magic_damage_resistance_modifier },
	{ "MagicResistanceModifier", fx_magic_resistance_modifier },
	{ "MaximumHPModifier", fx_maximum_hp_modifier },
	{ "MinimumHPModifier", fx_minimum_hp_modifier },
	{ "MiscastMagicModifier", fx_miscast_magic_modifier },
	{ "MissilesResistanceModifier", fx_missiles_resistance_modifier },
	{ "MoraleBreakModifier", fx_morale_break_modifier },
	{ "OpenLocksModifier", fx_open_locks_modifier },
	{ "MoraleModifier", fx_morale_modifier },
	{ "PauseTarget", fx_pause_target }, //also known as casterhold
	{ "PickPocketsModifier", fx_pick_pockets_modifier },
	{ "PiercingResistanceModifier", fx_piercing_resistance_modifier },
	{ "PlayMovie", fx_play_movie },
	{ "PlaySound", fx_playsound },
	{ "PlayVisualEffect", fx_play_visual_effect },
	{ "PriestSpellSlotsModifier", fx_bonus_priest_spells },
	{ "PortraitChange", fx_portrait_change },
	{ "Proficiency", fx_proficiency },
	{ "RemoveCurse", fx_remove_curse },
	{ "ReputationModifier", fx_reputation_modifier },
	{ "SaveVsBreathModifier", fx_save_vs_breath_modifier },
	{ "SaveVsDeathModifier", fx_save_vs_death_modifier },
	{ "SaveVsPolyModifier", fx_save_vs_poly_modifier },
	{ "SaveVsSpellsModifier", fx_save_vs_spell_modifier },
	{ "SaveVsWandsModifier", fx_save_vs_wands_modifier },
	{ "ScriptingState", fx_scripting_state },
	{ "SetAIScript", fx_set_ai_script },
	{ "SexModifier", fx_sex_modifier },
	{ "SlashingResistanceModifier", fx_slashing_resistance_modifier },
	{ "SpellDurationModifier", fx_spell_duration_modifier },
	{ "State:Berserk", fx_set_berserk_state },
	{ "State:Blind", fx_set_blind_state },
	{ "State:Blur", fx_set_blur_state },
	{ "State:Charmed", fx_set_charmed_state },
	{ "State:Confused", fx_set_confused_state },
	{ "State:Deafness", fx_set_deaf_state },
	{ "State:Diseased", fx_set_diseased_state },
	{ "State:Feeblemind", fx_set_feebleminded_state },
	{ "State:Hasted", fx_set_hasted_state },
	{ "State:Infravision", fx_set_infravision_state },
	{ "State:Invisible", fx_set_invisible_state }, //both invis or improved invis
	{ "State:Nondetection", fx_set_nondetection_state },
	{ "State:Panic", fx_set_panic_state },
	{ "State:Petrification", fx_set_petrified_state },
	{ "State:Poisoned", fx_set_poisoned_state },
	{ "State:Sanctuary", fx_set_sanctuary_state },
	{ "State:Silenced", fx_set_silenced_state },
	{ "State:Helpless", fx_set_unconscious_state },
	{ "State:Slowed", fx_set_slowed_state },
	{ "State:Stun", fx_set_stun_state },
	{ "StealthModifier", fx_stealth_modifier },
	{ "StrengthModifier", fx_strength_modifier },
	{ "StrengthBonusModifier", fx_strength_bonus_modifier },
	{ "SummonCreature", fx_summon_creature },
	{ "THAC0Modifier", fx_to_hit_modifier },
	{ "TrackingModifier", fx_tracking_modifier },
	{ "TransparencyModifier", fx_transparency_modifier },
	{ "UnsummonCreature", fx_unsummon_creature },
	{ "Variable:StoreLocalVariable", fx_local_variable },
	{ "WisdomModifier", fx_wisdom_modifier },
	{ "WizardSpellSlotsModifier", fx_bonus_wizard_spells },
	{ NULL, NULL },
};

inline bool valid_number(const char* string, long& val)
{
	char* endpr;

	val = strtol( string, &endpr, 0 );
	return ( const char * ) endpr != string;
}

static EffectLink* FindEffect(const char* effectname)
{
	if (!effectname) {
		return NULL;
	}
	for (int i = 0; effectnames[i].Name; i++) {
		if (!stricmp( effectnames[i].Name, effectname )) {
			return effectnames + i;
		}
	}
	printf( "Warning: Couldn't assign effect: %s\n", effectname );
	return NULL;
}

bool Init_EffectQueue()
{
	if (!initialized) {
		TableMgr* efftextTable=NULL;
		SymbolMgr* effectsTable;
		memset( effect_refs, 0, sizeof( effect_refs ) );
		// FIXME
		//memset( efftexts, -1, sizeof( efftexts ) );

		initialized = 1;

		int effT = core->LoadTable( "EFFTEXT" );
		efftextTable = core->GetTable( effT );
		
		int eT = core->LoadSymbol( "EFFECTS" );
		if (eT < 0) {
			printMessage( "EffectQueue","A critical scripting file is missing!\n",LIGHT_RED );
			return false;
		}
		effectsTable = core->GetSymbol( eT );
		if (!effectsTable) {
			printMessage( "EffectQueue","A critical scripting file is damaged!\n",LIGHT_RED );
			return false;
		}

		for (int i = 0; i < MAX_EFFECTS; i++) {
			char* effectname = effectsTable->GetValue( i );
			if (efftextTable) {
				int row = efftextTable->GetRowCount();
				while (row--) {
					char* ret = efftextTable->GetRowName( row );
					long val;
					if (valid_number( ret, val ) && (i == val) ) {
						effect_refs[i].EffText = atoi( efftextTable->QueryField( row, 1 ) );
					}
				}
			}

			EffectLink* poi = FindEffect( effectname );
			if (poi != NULL) {
				effect_refs[i].Function = poi->Function;
				effect_refs[i].Name = poi->Name;
			}
			//printf("-------- FN: %d, %s\n", i, effectname);
		}
		core->DelSymbol( eT );
		if ( efftextTable ) core->DelTable( effT );
	}
	return true;
}

EffectQueue::EffectQueue()
{
	Owner = NULL;
}

EffectQueue::~EffectQueue()
{
	for (unsigned i = 0; i < effects.size(); i++) {
		delete( effects[i] );
	}
}

bool EffectQueue::AddEffect(Effect* fx)
{
	// Check probability of the effect occuring
	if (fx->Probability1 < core->Roll( 1, 100, 0 ))
		return false;

	// FIXME: roll saving throws and resistance
	//if (core->SavingThrow( target->Modified[fx->SavingThrowType], fx->SavingThrowBonus )) {
	//}

	Effect* new_fx = new Effect;
	memcpy( new_fx, fx, sizeof( Effect ) );
	effects.push_back( new_fx );

	// pre-roll dice for fx needing them and stow them in the effect
	new_fx->random_value = core->Roll( fx->DiceThrown, fx->DiceSides, 0 );

	return true;
}

bool EffectQueue::RemoveEffect(Effect* fx)
{
	int invariant_size = offsetof( Effect, random_value );

	for (std::vector< Effect* >::iterator f = effects.begin(); f != effects.end(); f++ ) {
		Effect* fx2 = *f;

		if (! memcmp( fx, fx2, invariant_size)) {
			delete fx2;
			effects.erase( f );
			return true;
		}
	}
	return false;
	
}

void EffectQueue::ApplyAllEffects(Actor* target)
{
	// FIXME: clear protection_from_opcode array
	// memset( target->protection_from_fx, 0, MAX_FX_OPCODES * sizeof( char ) );

	std::vector< Effect* >::iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		ApplyEffect( target, *f, false );
	}
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		if ((*f)->TimingMode==FX_DURATION_JUST_EXPIRED) {
			delete *f;
			effects.erase(f);
			f--;
		}
	}
}


//this is where effects from spells first get in touch with the target
void EffectQueue::AddAllEffects(Actor* target)
{
	std::vector< Effect* >::iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		//handle resistances and saving throws here
		target->fxqueue.ApplyEffect( target, *f, true );
		if ((*f)->TimingMode!=FX_DURATION_JUST_EXPIRED) {
			target->fxqueue.AddEffect(*f);
		}
	}
}


void EffectQueue::PrepareDuration(Effect* fx)
{
	fx->Duration = fx->Duration*6 + core->GetGame()->GameTime;
}


//resisted effect based on level
inline bool check_level(Actor *target, Effect *fx)
{
	switch (fx->Opcode) {
	case 12: //damage
	case 17: //hp modifier
	case 18: //max hp modifier
		return false;
	}
	int level = target->GetXPLevel( true );
	if ((fx->DiceSides != 0 || fx->DiceThrown != 0) && (level < (int)fx->DiceSides || level > (int)fx->DiceThrown)) {
		return true;
	}
	return false;
}

void EffectQueue::ApplyEffect(Actor* target, Effect* fx, bool first_apply)
{
	if (!target) {
		return;
	}
	//printf( "FX 0x%02x: %s(%d, %d)\n", fx->Opcode, effectnames[fx->Opcode].Name, fx->Parameter1, fx->Parameter2 );
	if (fx->Opcode >= MAX_EFFECTS) 
		return;

	EffectFunction fn = effect_refs[fx->Opcode].Function;
	if (fn) {
		if (first_apply) {
			if (check_level(target, fx) ) {
				fx->TimingMode=FX_DURATION_JUST_EXPIRED;
				return;
			}
		}
		if ( effect_refs[fx->Opcode].EffText > 0 ) {
			char *text = core->GetString( effect_refs[fx->Opcode].EffText );
			core->DisplayString( text );
			free( text );
		}
		
		//if there is no owner, we assume it is the target
		switch( fn( Owner?Owner:target, target, fx ) ) {
			case FX_APPLIED:
				//normal effect with duration
				if (first_apply) {
					PrepareDuration(fx);
				}
				break;
			case FX_NOT_APPLIED:
 				//instant effect, pending removal
				fx->TimingMode=FX_DURATION_JUST_EXPIRED;
				break;
			case FX_PERMANENT:
				//don't stick around if it was permanent
				if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
					fx->TimingMode=FX_DURATION_JUST_EXPIRED;
				}
				break;
			case FX_CYCLIC:
				//mark this spell as cyclic (is there a flag?)
				if (first_apply) {
					PrepareDuration(fx);
				}
				break;
		}
	} else {
		//effect not found, it is going to be discarded
		fx->TimingMode=FX_DURATION_JUST_EXPIRED;
	}		
}

//call this from an applied effect, after it returns, these effects
//will be killed along with it
void EffectQueue::RemoveAllEffects(ieDword opcode)
{
	std::vector< Effect* >::iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		if ( (*f)->Opcode!=opcode) {
			continue;
		}

		(*f)->TimingMode=FX_DURATION_JUST_EXPIRED;
	}
}


void EffectQueue::RemoveLevelEffects(ieDword level, bool dispellable)
{
	std::vector< Effect* >::iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		if ( (*f)->Power<=level) {
			continue;
		}

		//if dispellable was not set, or the effect is dispellable
		//then remove it
		if (!dispellable || ((*f)->Resistance&FX_CAN_DISPEL) ) {
			(*f)->TimingMode=FX_DURATION_JUST_EXPIRED;
		}
	}
}


// looks for opcode with param2
// useful for: immunity vs projectile

#define MATCH_OPCODE() if((*f)->Opcode!=opcode) { continue; }

#define MATCH_LIVE_FX() {ieDword tmp=(*f)->TimingMode; \
		if (tmp!=FX_DURATION_INSTANT_LIMITED && \
		    tmp!=FX_DURATION_INSTANT_PERMANENT && \
		    tmp!=FX_DURATION_INSTANT_WHILE_EQUIPPED && \
		    tmp!=FX_DURATION_INSTANT_PERMANENT_AFTER_BONUSES) { \
			continue; \
		}}

#define MATCH_PARAM1() if((*f)->Parameter1!=param1) { continue; }
#define MATCH_PARAM2() if((*f)->Parameter2!=param2) { continue; }
#define MATCH_RESOURCE() if( strnicmp( (*f)->Resource, resource, 8) ) { continue; }

Effect *EffectQueue::HasOpcodeWithParam(ieDword opcode, ieDword param2)
{
	std::vector< Effect* >::iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();
		MATCH_PARAM2();

		return (*f);
	}
	return NULL;
}

//looks for opcode with pairs of parameters (useful for protection against creature, extra damage or extra thac0 against creature)
//generally an IDS targeting

Effect *EffectQueue::HasOpcodeWithParamPair(ieDword opcode, ieDword param1, ieDword param2)
{
	std::vector< Effect* >::iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();
		MATCH_PARAM2();
		//0 is always accepted as first parameter
		if (param1) {
			MATCH_PARAM1();
		}

		return (*f);
	}
	return NULL;
}

//this function does IDS targeting for effects (extra damage/thac0 against creature)
static int ids_stats[7]={IE_EA, IE_GENERAL, IE_RACE, IE_CLASS, IE_SPECIFIC, IE_SEX, IE_ALIGNMENT};

Effect *EffectQueue::HasOpcodeMatchingCreature(ieDword opcode, Actor *actor)
{
	for (ieDword ids=0; ids<7;ids++) {
		Effect *tmp = HasOpcodeWithParamPair(opcode, ids+2, 0);
		if (tmp) {
			return tmp;
		}
		tmp = HasOpcodeWithParamPair(opcode, ids+2, actor->GetStat(ids_stats[ids]) );
		if (tmp) {
			return tmp;
		}
	}
	return NULL;
}

//useful for immunity vs spell, can't use item, etc.
Effect *EffectQueue::HasOpcodeWithResource(ieDword opcode, ieResRef resource)
{
	std::vector< Effect* >::iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();
		MATCH_RESOURCE();

		return (*f);
	}
	return NULL;
}

void EffectQueue::dump()
{
	printf( "EFFECT QUEUE:\n" );
	for (unsigned int i = 0; i < effects.size (); i++ ) {
		Effect* fx = effects[i];
		if (fx) {
			char *Name = NULL;
			if (fx->Opcode < MAX_EFFECTS)
				Name = (char*) effect_refs[fx->Opcode].Name;

			printf( " %2d: 0x%02x: %s (%d, %d)\n", i, fx->Opcode, Name, fx->Parameter1, fx->Parameter2 );
		}
	}
}

// Helper macros and functions for effect opcodes
/*
#define CHECK_LEVEL() { \
	int level = target->GetXPLevel( true ); \
	if ((fx->DiceSides != 0 || fx->DiceThrown != 0) && (level < (int)fx->DiceSides || level > (int)fx->DiceThrown)) \
		return FX_NOT_APPLIED; \
	}
*/
inline int MIN(int a, int b)
{
	return (a > b ? b : a);
}

inline int MAX(int a, int b)
{
	return (a < b ? b : a);
}

inline bool match_ids(Actor *target, int table, ieDword value)
{
	if (value == 0) {
		return true;
	}

	int a, stat;

	switch (table) {
		case 2: //EA
			stat = IE_EA; break;
		case 3: //GENERAL
			stat = IE_GENERAL; break;
		case 4: //RACE
			stat = IE_RACE; break;
		case 5: //CLASS
			stat = IE_CLASS; break;
		case 6: //SPECIFIC
			stat = IE_SPECIFIC; break;
		case 7: //GENDER
			stat = IE_SEX; break;
		case 8: //ALIGNMENT
			stat = target->GetStat(IE_ALIGNMENT);
			a = value&15;
			if (a) {
				if (a != ( stat & 15 )) {
					return false;
				}
			}
			a = value & 240;
			if (a) {
				if (a != ( stat & 240 )) {
					return false;
				}
			}
			return true;
		default:
			return false;
	}
	if (target->GetStat(stat)==value) {
		return true;
	}
	return false;
}

// FIXME: Dice roll should be probably done just once, e.g. when equipping 
// the item, not each time the fx are applied
// <avenger> the dice values are actually level limits, except in 3 hp modifier functions
// the damage function is an instant (the other 2 functions might be tricky with random values)
#define DICE_ROLL(max_val) ((fx->DiceThrown && fx->DiceSides) ? ((max_val >=0) ? (MIN( core->Roll( fx->DiceThrown, fx->DiceSides, 0 ), max_val )) : (MAX( core->Roll( fx->DiceThrown, fx->DiceSides, 0 ), max_val ))) : max_val)

// often used stat modifications, usually Parameter2 types 0, 1 and 2
//#define STAT_ADD(stat, mod) target->SetStat( ( stat ), (ieDword)(target->GetStat( stat ) + ( mod )))
//#define STAT_SET(stat, mod) target->SetStat( ( stat ), (ieDword)( mod ))
//#define STAT_MUL(stat, mod) target->SetStat( ( stat ), (ieDword)(target->GetStat( stat ) * (( mod ) / 100.0)))

//these macros should work differently in permanent mode (modify base too)
#define STAT_GET(stat) (target->Modified[ stat ])
#define STAT_ADD(stat, mod) target->Modified[ stat ] = (ieDword)(target->Modified[ stat ] + ( mod ))
#define STAT_SET(stat, mod) target->Modified[ stat ] = (ieDword)( mod )
#define STAT_MUL(stat, mod) target->Modified[ stat ] = (ieDword)(target->Modified[ stat ] * (( mod ) / 100.0))
#define STATE_CURE( mod ) target->Modified[ IE_STATE_ID ] &= ~(ieDword) ( mod )
#define STATE_SET( mod ) target->Modified[ IE_STATE_ID ] |= (ieDword) ( mod )
#define STATE_GET( mod ) (target->Modified[ IE_STATE_ID ] & (ieDword) ( mod ) )
#define STAT_MOD( stat ) target->NewStat(stat, fx->Parameter1, fx->Parameter2)

// Effect opcodes
// FIXME: These should be moved into their own plugins
// NOTE: These opcode numbers are true for PS:T and are meant just for 
// better orientation
// <avenger> opcodes below 0xb0 are the same for ALL variations (or crash/nonfunctional)
// so we can implement those without fear, overlapping functions could be marked
// like 0xb1 (pst), and the effect.ids file points to the appropriate function

// 0x00
int fx_ac_vs_damage_type_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_ac_vs_damage_type_modifier (%2d): AC Modif: %d ; Type: %d ; MinLevel: %d ; MaxLevel: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2, (int) fx->DiceSides, (int) fx->DiceThrown );
	//check level was pulled outside as a common functionality
	//CHECK_LEVEL();

	// it is a bitmask
	int type = fx->Parameter2;
	if (type == 0) {
		// FIXME: this is probably wrong, but it's hack to see
		// anything in PST
		STAT_ADD( IE_ARMORCLASS, fx->Parameter1 );
		type = 15;
	}

	//the original engine did work with the combination of these bits
	//but since it crashed, we are not bound to the same rules
	if (type & 1) STAT_ADD( IE_ACCRUSHINGMOD, fx->Parameter1 );
	if (type & 2) STAT_ADD( IE_ACMISSILEMOD, fx->Parameter1 );
	if (type & 4) STAT_ADD( IE_ACPIERCINGMOD, fx->Parameter1 );
	if (type & 8) STAT_ADD( IE_ACSLASHINGMOD, fx->Parameter1 );

	// FIXME: set to Param1 or Param1-1 ?
	if (type == 16 && target->Modified[IE_ARMORCLASS] > fx->Parameter1)
		STAT_SET( IE_ARMORCLASS, fx->Parameter1 );

	return FX_APPLIED;
}

// 0x01
int fx_attacks_per_round_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_attacks_per_round_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_NUMBEROFATTACKS);
	return FX_APPLIED;
}

// 0x02
// this effect clears the STATE_SLEEP (1) bit, but clearing it alone wouldn't remove the
// unconscious effect, which is combined with STATE_HELPLESS (0x20+1)
int fx_cure_sleep_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_sleep_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_CURE( STATE_SLEEP );
	return FX_NOT_APPLIED;
}

// 0x03
// this effect clears the STATE_BERSERK (2) bit, but bg2 actually ignores the bit
int fx_cure_berserk_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_berserk_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_CURE( STATE_BERSERK );
	return FX_NOT_APPLIED;
}

// 0x04
// this effect sets the STATE_BERSERK bit, but bg2 actually ignores the bit
int fx_set_berserk_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_berserk_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_SET( STATE_BERSERK );
	return FX_NOT_APPLIED;
}

// 0x05
//fixme, this is much more complex, alters IE_EA too
int fx_set_charmed_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_charmed_state (%2d): General: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	if (STAT_GET(IE_GENERAL)!=fx->Parameter1) {
		return FX_NOT_APPLIED;
	}
	switch (fx->Parameter2) {
	case 0: //charmed (target neutral after charm)
	case 1000:
		break;
	case 1: //charmed (target hostile after charm)
	case 1001:
		break;
	case 2: //dire charmed (target neutral after charm)
	case 1002:
		break;
	case 3: //dire charmed (target hostile after charm)
	case 1003:
		break;
	case 4: //controlled by cleric
	case 1004:
		break;
	case 5: //thrullcharm?
	case 1005:
		break;
	}

	STATE_SET( STATE_CHARMED );
	//don't stick if permanent
	return FX_PERMANENT;
}

// 0x06
int fx_charisma_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_charisma_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_CHR );
	return FX_PERMANENT;
}

// 0x07
// this effect might not work in pst, they don't have separate weapon slots
int fx_set_color_gradient (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_color_gradient (%2d): Gradient: %d, Location: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->SetColor( fx->Parameter2, fx->Parameter1 );
	return FX_APPLIED;
}

// 0x0A
int fx_constitution_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_constitution_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_CON );
	return FX_PERMANENT;
}

// 0x0B
int fx_cure_poisoned_25_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_poisoned_25_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_CURE( STATE_POISONED ); //this actually isn't in the engine code, i think
	target->fxqueue.RemoveAllEffects( 25 ); //this is what actually happens in bg2
	return FX_NOT_APPLIED;
}

// 0x0C Damage
// this is a very important effect
int fx_damage (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_damage (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	int damage; //FIXME damage calculation, random damage, etc

	damage = 1;
	damage = target->Damage(damage, fx->Parameter2, Owner); //FIXME!
	//handle invulnerabilities, print damage caused
	//this effect doesn't stick
	return FX_NOT_APPLIED;
}

//0x0D
int fx_death (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_death (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	target->Damage(0, fx->Parameter2, Owner); //hmm?
	//death has damage type too
	target->Die(Owner);
	//this effect doesn't stick
	return FX_NOT_APPLIED;
}

// 0xE
int fx_cure_frozen_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_frozen_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_CURE( STATE_FROZEN );
	return FX_NOT_APPLIED;
}

// 0x0F
int fx_dexterity_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_dexterity_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_DEX );
	return FX_PERMANENT;
}

// 0x10
// this function removes slowed state, or sets hasted state
int fx_set_hasted_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_hasted_state (%2d): Type: %d\n", fx->Opcode, fx->Parameter2 );
	switch (fx->Parameter2) {
	case 0: //normal haste
		if ( STATE_GET(STATE_SLOWED) ) {
			STATE_CURE( STATE_SLOWED );
		} else {
			STATE_SET( STATE_HASTED );
		}
		break;
	case 1://improved haste
		if ( STATE_GET(STATE_SLOWED) ) {
			STATE_CURE( STATE_SLOWED );
		} else {
			STATE_SET( STATE_HASTED );
		}
		break;
	case 2://speed haste only
		break;
	}
	//probably when this effect expires, it issues a set_slowed_state?
	return FX_NOT_APPLIED;
}

// 0x11
int fx_current_hp_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_current_hp_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_HITPOINTS );
	return FX_APPLIED;
}

// 0x12
int fx_maximum_hp_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_maximum_hp_modifier (%2d): Stat Modif: %d ; Modif Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	int bonus;

	switch (fx->Parameter2) {
	case 0:
		bonus = DICE_ROLL( (signed)fx->Parameter1 );
		STAT_ADD( IE_MAXHITPOINTS, bonus );
		STAT_ADD( IE_HITPOINTS, bonus );
		break;
	case 1:
		STAT_SET( IE_MAXHITPOINTS, fx->Parameter1 );
		STAT_SET( IE_HITPOINTS, fx->Parameter1 );
		break;
	case 2:
		STAT_MUL( IE_MAXHITPOINTS, fx->Parameter1 );
		STAT_MUL( IE_HITPOINTS, fx->Parameter1 );
		break;
	case 3:
		bonus = DICE_ROLL( (signed)fx->Parameter1 );
		STAT_ADD( IE_MAXHITPOINTS, bonus );
		break;
	case 4:
		STAT_SET( IE_MAXHITPOINTS, fx->Parameter1 );
		break;
	case 5:
		STAT_MUL( IE_MAXHITPOINTS, fx->Parameter1 );
		break;
	}
	return FX_APPLIED;
}

// 0x13
int fx_intelligence_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_intelligence_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_INT );
	return FX_PERMANENT;
}

// 0x14
// this is more complex, there is a half-invisibility state
// and there is a hidden state
int fx_set_invisible_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	switch (fx->Parameter2) {
		case 1:
			STATE_SET( STATE_INVISIBLE );
			break;
		case 2:
			STATE_SET( STATE_INVIS2 );
			break;
	}
	return FX_APPLIED;
}

// 0x15
int fx_lore_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_lore_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_LORE );
	return FX_APPLIED;
}

// 0x16
int fx_luck_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_luck_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_LUCK );
	return FX_APPLIED;
}

// 0x17
int fx_morale_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_morale_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_MORALE );
	return FX_APPLIED;
}

// 0x18
int fx_set_panic_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_panic_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//shall we set morale to 0 or just flick the panic flag on
	//this requires a little research
	STATE_SET( STATE_PANIC );
	//target->NewStat( IE_MORALE, 0, fx->Parameter2 );
	return FX_NOT_APPLIED;
}

// 0x19
int fx_set_poisoned_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_poisoned_state (%2d): Damage: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//apparently this bit isn't set, but then why is it here
	//this requires a little research
	STATE_SET( STATE_POISONED );
	//also this effect is executed every update
	return FX_CYCLIC;
}


// 0x1a
// gemrb extension: if the resource field is filled, it will remove curse only from the specified item
int fx_remove_curse (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_remove_curse (%2d): Resource: %s\n", fx->Opcode, fx->Resource );

	int i = target->inventory.GetSlotCount();
	while(i--) {
		//does this slot need unequipping
		if (core->QuerySlotEffects(i) ) {
			if (fx->Resource[0] && strnicmp(target->inventory.GetSlotItem(i)->ItemResRef, fx->Resource,8) ) {
				continue;
			}
			target->inventory.UnEquipItem(i,true);
		}
	}
	//this could also be done, but not implemented yet
	//target->inventory.ChangeItemFlag(fx->Resource, IE_INV_ITEM_CURSED, BF_NAND);
	//this is an instant effect
	return FX_NOT_APPLIED;
}

// 0x1b
int fx_acid_resistance_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_acid_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_RESISTACID );
	return FX_APPLIED;
}

// 0x1c
int fx_cold_resistance_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cold_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_RESISTCOLD );
	return FX_APPLIED;
}

// 0x1d
int fx_electricity_resistance_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_electricity_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_RESISTELECTRICITY );
	return FX_APPLIED;
}

// 0x1e
int fx_fire_resistance_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_fire_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_RESISTFIRE );
	return FX_APPLIED;
}

// 0x1f
int fx_magic_damage_resistance_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_magic_damage_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_MAGICDAMAGERESISTANCE );
	return FX_APPLIED;
}

// 0x20
int fx_cure_dead_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_dead_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//someone should clear the internal flags related to death
	STATE_CURE( STATE_DEAD );
	return FX_NOT_APPLIED;
}

// 0x21
int fx_save_vs_death_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_save_vs_death_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_SAVEVSDEATH );
	return FX_APPLIED;
}

// 0x22
int fx_save_vs_wands_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_save_vs_wands_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_SAVEVSWANDS );
	return FX_APPLIED;
}

// 0x23
int fx_save_vs_poly_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_save_vs_poly_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_SAVEVSPOLY );
	return FX_APPLIED;
}

// 0x24
int fx_save_vs_breath_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_save_vs_breath_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_SAVEVSBREATH );
	return FX_APPLIED;
}

// 0x25
int fx_save_vs_spell_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_save_vs_spell_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_SAVEVSSPELL );
	return FX_APPLIED;
}

// 0x26
int fx_set_silenced_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_silenced_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_SET( STATE_SILENCED );
	return FX_NOT_APPLIED;
}

// 0x27
// this effect sets both bits, but 'awaken' only removes the sleep bit
int fx_set_unconscious_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_unconscious_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_SET( STATE_HELPLESS | STATE_SLEEP );
	//the effect directly sets the state bit, and doesn't stick
	return FX_NOT_APPLIED;
}

// 0x28
// this function removes hasted state, or sets slowed state
int fx_set_slowed_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_slowed_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	if (STATE_GET(STATE_HASTED) ) {
		STATE_CURE( STATE_HASTED );
	} else {
		STATE_SET( STATE_SLOWED );
	}
	return FX_NOT_APPLIED;
}

// 0x2A
int fx_bonus_wizard_spells (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_bonus_wizard_spells (%2d): Spell Add: %d ; Spell Level: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	
	int i=1;
	for( int j=0;j<9;j++) {
		if (fx->Parameter2&i) {
			if(fx->Parameter1) {
				STAT_ADD( IE_WIZARDBONUS1+j, fx->Parameter1);
			}
			else {
				STAT_ADD( IE_WIZARDBONUS1+j, target->BaseStats[ IE_WIZARDBONUS1+j ]);
			}
		}
	}
	return FX_APPLIED;
}

// 0x2B
int fx_cure_petrified_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_petrified_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_CURE( STATE_PETRIFIED );
	return FX_NOT_APPLIED;
}

// 0x2C
int fx_strength_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_strength_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_STR );
	return FX_PERMANENT;
}

// 0x2D
int fx_set_stun_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_stun_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_SET( STATE_STUNNED );
	return FX_NOT_APPLIED;
}

// 0x2E
int fx_cure_stun_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_stun_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_CURE( STATE_STUNNED );
	return FX_NOT_APPLIED;
}

// 0x2F
int fx_cure_invisible_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_invisible_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_CURE( STATE_INVISIBLE );
	return FX_NOT_APPLIED;
}

// 0x30
int fx_cure_silenced_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_silenced_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_CURE( STATE_SILENCED );
	return FX_NOT_APPLIED;
}

// 0x31
int fx_wisdom_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_wisdom_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_WIS );
	return FX_PERMANENT;
}

// 0x35
int fx_animation_id_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_animation_id_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_ANIMATION_ID );
	return FX_APPLIED;
}

// 0x36
int fx_to_hit_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_to_hit_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_THAC0 );
	return FX_APPLIED;
}

// 0x37 instant kill of creature type
int fx_kill_creature_type (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_kill_creature_type (%2d): Value: %d, IDS: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	if (match_ids( target, fx->Parameter1, fx->Parameter2) ) {
		target->Die( Owner );
	}
	//need research (is this an instant action or sticks)
	return FX_NOT_APPLIED;
}

// 0x38
//switch good to evil and evil to good
static int gne_toggle[4]={0,3,2,1};

int fx_alignment_invert (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_alignment_invert (%2d)\n", fx->Opcode );
	ieDword newalign = target->GetStat( IE_ALIGNMENT );
	newalign = (newalign & AL_LNC_MASK) + gne_toggle[newalign & AL_GNE_MASK];
	target->SetStat( IE_ALIGNMENT, newalign );
	return FX_APPLIED;
}

// 0x39
int fx_alignment_change (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_alignment_change (%2d): Value: %d, IDS: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	target->SetStat( IE_ALIGNMENT, fx->Parameter2 );
	return FX_APPLIED;
}

//0x3A

int fx_dispel_effects (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_dispel_effects (%2d): Value: %d, IDS: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	ieDword level = fx->Power;

	//this might be different, it could be that removal depends on random
	if (fx->Parameter2==1) {
		level = fx->Parameter1;
	}
	target->fxqueue.RemoveLevelEffects(level, true);
	return FX_NOT_APPLIED;
}

// 0x3B
int fx_stealth_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_stealth_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_STEALTH );
	return FX_APPLIED;
}

// 0x3C
int fx_miscast_magic_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_miscast_magic_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	switch (fx->Parameter2) {
		case 0:
			target->NewStat( IE_SPELLFAILUREMAGE, fx->Parameter1, MOD_ABSOLUTE);
			break;
		case 1:
			target->NewStat( IE_SPELLFAILUREPRIEST, fx->Parameter1, MOD_ABSOLUTE);
			break;
		default:
			return FX_NOT_APPLIED;
	}
	return FX_APPLIED;
}

// 0x3D
// this crashes in bg2
// and in iwd it doesn't really follow the stat_mod convention
int fx_alchemy_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_alchemy_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_ALCHEMY );
	return FX_APPLIED;
}

// 0x3E
int fx_bonus_priest_spells (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_bonus_priest_spells (%2d): Spell Add: %d ; Spell Level: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	
	int i=1;
	for( int j=0;j<9;j++) {
		if (fx->Parameter2&i) {
			if(fx->Parameter1) {
				STAT_ADD( IE_PRIESTBONUS1+j, fx->Parameter1);
			}
			else {
				STAT_ADD( IE_PRIESTBONUS1+j, target->BaseStats[ IE_PRIESTBONUS1+j ]);
			}
		}
	}
	return FX_APPLIED;
}

// 0x3F
int fx_set_infravision_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_infravision_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_SET( STATE_INFRA );
	return FX_NOT_APPLIED;
}

// 0x40
int fx_cure_infravision_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_infravision_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_CURE( STATE_INFRA );
	return FX_NOT_APPLIED;
}

//0x41
int fx_set_blur_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_blur_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_SET( STATE_BLUR );
	return FX_NOT_APPLIED;
}

// 0x42
int fx_transparency_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_transparency_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_TRANSLUCENT );
	return FX_APPLIED;
}

// 0x43
int fx_summon_creature (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_summon_creature (%2d): ResRef:%s Anim:%s Type: %d\n", fx->Opcode, fx->Resource, fx->Resource2, fx->Parameter2 );

	//check the summoning limit?
	DataStream* ds = core->GetResourceMgr()->GetResource( fx->Resource, IE_CRE_CLASS_ID );
	Actor *ab = core->GetCreature(ds);
	if (!ab) {
		return FX_NOT_APPLIED;
	}

	ab->LastSummoner = Owner->GetID();

	switch (fx->Parameter2) {
		case 0: case 1: case 3:
			ab->SetStat(IE_EA, EA_ALLY); //is this the summoned EA
			break;
		case 5:
			ab->SetStat(IE_EA, EA_ENEMY);
			break;
		default:
			break;
	}

	//probably we should use the position in fx (and set it earlier)
	Point position = target->Pos;
	//
	//
	Map *map = target->GetCurrentArea();
	ab->SetPosition(map, position, true, 0);
	if (fx->Resource2[0]) {
		ScriptedAnimation* vvc = core->GetScriptedAnimation(fx->Resource2, ab->Pos);
		map->AddVVCCell( vvc );
	}

	return FX_NOT_APPLIED;
}

int fx_unsummon_creature (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_unsummon_creature (%2d)\n", fx->Opcode );

	if (target->LastSummoner) {
		target->InternalFlags|=IF_CLEANUP;
	}
	return FX_NOT_APPLIED;
}

// 0x45
int fx_set_nondetection_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_nondetection_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_SET( STATE_NONDET );
	return FX_PERMANENT;
}

// 0x46
int fx_cure_nondetection_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_nondetection_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_CURE( STATE_NONDET );
	return FX_NOT_APPLIED;
}

//0x47
int fx_sex_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_sex_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD( IE_SEX );
	return FX_APPLIED;
}

//0x48
int fx_ids_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_ids_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	switch (fx->Parameter2) {
	case 2:
		STAT_SET(IE_EA, fx->Parameter1);
		break;
	case 3:
		STAT_SET(IE_GENERAL, fx->Parameter1);
		break;
	case 4:
		STAT_SET(IE_RACE, fx->Parameter1);
		break;
	case 5:
		STAT_SET(IE_CLASS, fx->Parameter1);
		break;
	case 6:
		STAT_SET(IE_SPECIFIC, fx->Parameter1);
		break;
	case 7:
		STAT_SET(IE_SEX, fx->Parameter1);
		break;
	default:
		return FX_NOT_APPLIED;
	}
	//not sure, need a check
	return FX_APPLIED;
}


// 0x49
int fx_damage_bonus_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_damage_bonus_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_DAMAGEBONUS );
	return FX_APPLIED;
}

// 0x4a
int fx_set_blind_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_blind_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_SET( STATE_BLIND );
	return FX_APPLIED;
}

// 0x4b
int fx_cure_blind_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_blind_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_CURE( STATE_BLIND );
	return FX_NOT_APPLIED;
}

// 0x4c
int fx_set_feebleminded_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_feebleminded_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_SET( STATE_FEEBLE );
	return FX_NOT_APPLIED;
}

// 0x4d
int fx_cure_feebleminded_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_feebleminded_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_CURE( STATE_FEEBLE );
	return FX_NOT_APPLIED;
}

//0x4f
int fx_set_diseased_state (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_set_diseased_state (%2d): Damage: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//STATE_SET( STATE_DISEASED ); //no this we don't want
	//also this effect is executed every update
	return FX_CYCLIC;
}


//apparently this effect removes effect 0x4e (78)
int fx_cure_diseased_78_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_diseased_78_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//STATE_CURE( STATE_DISEASED ); //the bit flagged as disease is actually the active state. so this is even more unlikely to be used as advertised
	target->fxqueue.RemoveAllEffects( 78 ); //this is what actually happens in bg2
	return FX_NOT_APPLIED;
}

// 0x50
//this state has no bit???
int fx_set_deaf_state (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_set_deaf_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x51
//removes the deaf effect
int fx_cure_deaf_80_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_deaf_80_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	
	target->fxqueue.RemoveAllEffects(0x50);
	return FX_NOT_APPLIED;
}

//0x52
int fx_set_ai_script (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_ai_state (%2d): Resource: %s, Type: %d\n", fx->Opcode, fx->Resource, fx->Parameter2 );
	target->SetScript (fx->Resource, fx->Parameter2);
	return FX_NOT_APPLIED;
}

// 0x54
int fx_magical_fire_resistance_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_magical_fire_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_RESISTMAGICFIRE );
	return FX_APPLIED;
}

// 0x55
int fx_magical_cold_resistance_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_magical_cold_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_RESISTMAGICCOLD );
	return FX_APPLIED;
}

// 0x56
int fx_slashing_resistance_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_slashing_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_RESISTSLASHING );
	return FX_APPLIED;
}

// 0x57
int fx_crushing_resistance_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_crushing_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_RESISTCRUSHING );
	return FX_APPLIED;
}

// 0x58
int fx_piercing_resistance_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_piercing_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_RESISTPIERCING );
	return FX_APPLIED;
}

// 0x59
int fx_missiles_resistance_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_missiles_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_RESISTMISSILE );
	return FX_APPLIED;
}

// 0x5A
int fx_open_locks_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_open_locks_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_LOCKPICKING );
	return FX_APPLIED;
}

// 0x5B
int fx_find_traps_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_find_traps_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_TRAPS );
	return FX_APPLIED;
}

// 0x5C
int fx_pick_pockets_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_pick_pockets_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_PICKPOCKET );
	return FX_APPLIED;
}

// 0x5D
int fx_fatigue_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_fatigue_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_FATIGUE );
	return FX_APPLIED;
}

// 0x5E
int fx_intoxication_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_intoxication_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_INTOXICATION );
	return FX_APPLIED;
}

// 0x5F
int fx_tracking_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_tracking_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_TRACKING );
	return FX_APPLIED;
}

// 0x61
int fx_strength_bonus_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_strength_bonus_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_STREXTRA );
	return FX_APPLIED;
}

// 0x63
int fx_spell_duration_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_spell_duration_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	switch (fx->Parameter2) {
		case 0:
			target->NewStat( IE_SPELLDURATIONMODMAGE, fx->Parameter1, MOD_ABSOLUTE);
			break;
		case 1:
			target->NewStat( IE_SPELLDURATIONMODPRIEST, fx->Parameter1, MOD_ABSOLUTE);
			break;
		default:
			return FX_NOT_APPLIED;
	}
	return FX_APPLIED;
}

//0x67
int fx_change_name (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_change_name_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	target->SetText(fx->Parameter1, 0);
	return FX_NOT_APPLIED; //???
}

//0x68
int fx_experience_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_experience_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//i believe this has mode too
	target->AddExperience (fx->Parameter1);
	return FX_NOT_APPLIED;
}

//0x69
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
			//ie crashes here, i guess
			return FX_NOT_APPLIED;
	}
	game->AddGold (gold);
	return FX_NOT_APPLIED;
}

//0x6a
int fx_morale_break_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_morale_break_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD(IE_MORALEBREAK);
	return FX_PERMANENT; //permanent morale break doesn't stick
}

//0x6b
int fx_portrait_change (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_portrait_change (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	target->SetPortrait( fx->Resource, fx->Parameter2);
	return FX_NOT_APPLIED;
}

//0x6c
int fx_reputation_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_reputation_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_MOD(IE_REPUTATION);
	return FX_NOT_APPLIED; //needs testing
}

// 0x74
int fx_cure_improved_invisible_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_improved_invisible_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_CURE( STATE_INVISIBLE );
	STATE_CURE( STATE_INVIS2 );
	return FX_NOT_APPLIED;
}

// 0x80
int fx_set_confused_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_confused_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_SET( STATE_CONFUSED );
	return FX_NOT_APPLIED;
}

// 0x86
int fx_set_petrified_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_petrified_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_SET( STATE_PETRIFIED );
	return FX_NOT_APPLIED;
}

// 0x8B
// gemrb extension: rgb colour for displaystring
int fx_display_string (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_display_string (%2d): StrRef: %d\n", fx->Opcode, fx->Parameter1 );
	core->DisplayConstantStringName(fx->Parameter1, fx->Parameter2?fx->Parameter2:0xffffff, target);
	return FX_NOT_APPLIED;
}

// 0x93
int fx_learn_spell (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	//parameter1 is unused, gemrb lets you to make it not give XP
	//probably we should also let this via a game flag if we want 
	//full compatibility with bg1
	target->LearnSpell(fx->Resource, fx->Parameter1^LS_ADDXP);
	return FX_NOT_APPLIED;
}

// 0x98
int fx_play_movie (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_play_movie (%2d): Resource: %s\n", fx->Opcode, fx->Resource );
	core->PlayMovie (fx->Resource);
	return FX_NOT_APPLIED;
}
// 0x99
int fx_set_sanctuary_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_sanctuary_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_SET( IE_SANCTUARY, 1);
	return FX_PERMANENT; //is this correct?
}

// 0xa0
int fx_cure_sanctuary_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_sanctuary_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_SET( IE_SANCTUARY, 0);
	//remove effects too?
	return FX_NOT_APPLIED;
}

// 0xa1
int fx_cure_panic_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_panic_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_CURE( STATE_PANIC );
	return FX_NOT_APPLIED;
}

// 0xa2
int fx_cure_hold_175_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_hold_175_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	//note that this effect doesn't remove 185 (another hold effect)
	target->fxqueue.RemoveAllEffects( 175 ); 
	return FX_NOT_APPLIED;
}

// 0xa3
// free action
int fx_free_action (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_free_action (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	target->fxqueue.RemoveAllEffects( 126 ); 
//	STATE_CURE( STATE_SLOWED );
	return FX_NOT_APPLIED;
}

// 0xA5
int fx_pause_target (Actor* /*Owner*/, Actor *target, Effect* fx)
{
	if (0) printf( "fx_pause_target (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_CASTERHOLD );
	return FX_PERMANENT;
}

// 0xA6
int fx_magic_resistance_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_magic_resistance_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_RESISTMAGIC );
	return FX_APPLIED;
}

//0xae
int fx_playsound (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_playsound (%s)", fx->Resource );
	//this is probably inaccurate
	if (target) {
		core->GetSoundMgr()->Play(fx->Resource, target->Pos.x, target->Pos.y);
	} else {
		core->GetSoundMgr()->Play(fx->Resource);
	}
	return FX_APPLIED;
}

//0xaf, 0xb9
int fx_hold_creature (Actor* /*Owner*/, Actor* target, Effect* fx)
{
 	if (match_ids( target, fx->Parameter1, fx->Parameter2) ) {
		STAT_SET( IE_HELD, 1);
		return FX_APPLIED;
	}
	//if the ids don't match, the effect doesn't stick
	return FX_NOT_APPLIED;
}

// 0xBa
int fx_destroy_self (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_destroy_self (%2d)", fx->Opcode );
	target->InternalFlags|=IF_CLEANUP;
	return FX_APPLIED;
}

// 0xBB
int fx_local_variable (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	//this is a hack, the variable name spreads across the resources
	if (0) printf( "fx_local_variable (%2d) %s=%d", fx->Opcode, fx->Resource, fx->Parameter2 );
	target->locals->SetAt(fx->Resource, fx->Parameter2);
	//local variable effects are not applied, they will be resaved though
	return FX_NOT_APPLIED;
}

// 0xd0
int fx_minimum_hp_modifier (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_minimum_hp_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	STAT_MOD( IE_MINHITPOINTS );
	return FX_APPLIED;
}

// 0xd7
int fx_play_visual_effect (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_play_visual_effect (%2d): Resource: %s\n", fx->Opcode, fx->Resource );
	if (fx->Resource[0]) {
		ScriptedAnimation* vvc = core->GetScriptedAnimation(fx->Resource, target->Pos);
		target->GetCurrentArea( )->AddVVCCell( vvc );
	}
	return FX_APPLIED;
}

// 0xE9
int fx_proficiency (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_proficiency (%2d): Value: %d, Stat: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	//probably no need to check the boundaries, the original IE
	//did check it though (without boundaries, it is more useful)
	target->NewStat( fx->Parameter2, fx->Parameter1, MOD_ABSOLUTE );
	return FX_APPLIED;
}

// 0xF2
int fx_cure_confused_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_confused_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_CURE( STATE_CONFUSED );
	return FX_NOT_APPLIED;
}

// 0x126
int fx_scripting_state (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_scripting_state (%2d): Value: %d, Stat: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	//original engine didn't check boundaries, causing crashes
	//we allow only positive indices
	if (fx->Parameter2>100) {
		return FX_NOT_APPLIED;
	}
	target->NewStat( IE_SCRIPTINGSTATE1+fx->Parameter2, fx->Parameter1, MOD_ABSOLUTE );
	return FX_APPLIED;
}

