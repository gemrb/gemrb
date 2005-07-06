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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/EffectQueue.cpp,v 1.18 2005/07/06 23:37:33 edheldil Exp $
 *
 */

#include <stdio.h>
#include "Interface.h"
#include "Actor.h"
#include "Effect.h"
#include "EffectQueue.h"

#define FX_EXPIRED -1
#define FX_NOT_APPLIED  0
#define FX_APPLIED 1

int fx_ac_vs_damage_type_modifier (Actor* target, Effect* fx);
int fx_attacks_per_round_modifier (Actor* target, Effect* fx);
int fx_cure_sleep_state (Actor* target, Effect* fx);
int fx_set_berserk_state (Actor* target, Effect* fx);
int fx_cure_berserk_state (Actor* target, Effect* fx);
int fx_set_charmed_state (Actor* target, Effect* fx);
int fx_charisma_modifier (Actor* target, Effect* fx);
int fx_constitution_modifier (Actor* target, Effect* fx);
int fx_wisdom_modifier (Actor* target, Effect* fx);
int fx_cure_poisoned_state (Actor* target, Effect* fx);
int fx_damage (Actor* target, Effect* fx);
int fx_death (Actor* target, Effect* fx);
int fx_maximum_hp_modifier (Actor* target, Effect* fx);
int fx_intelligence_modifier (Actor* target, Effect* fx);
int fx_save_vs_death_modifier (Actor* target, Effect* fx);
int fx_save_vs_wands_modifier (Actor* target, Effect* fx);
int fx_save_vs_poly_modifier (Actor* target, Effect* fx);
int fx_save_vs_breath_modifier (Actor* target, Effect* fx);
int fx_save_vs_spell_modifier (Actor* target, Effect* fx);
int fx_set_sleep_state (Actor* target, Effect* fx);
int fx_bonus_wizard_spells (Actor* target, Effect* fx);
int fx_strength_modifier (Actor* target, Effect* fx);
int fx_to_hit_modifier (Actor* target, Effect* fx);
int fx_stealth_bonus (Actor* target, Effect* fx);
int fx_damage_bonus (Actor* target, Effect* fx);
int fx_open_locks_modifier (Actor* target, Effect* fx);
int fx_resistance_to_magic_damage (Actor* target, Effect* fx);
int fx_local_variable (Actor* target, Effect* fx);
int fx_playsound (Actor* target, Effect* fx);

struct EffectRef {
	const char* Name;
	EffectFunction Function;
	int EffText;
};

static int initialized = 0;
static EffectRef effect_refs[MAX_EFFECTS];
//static int efftexts[MAX_EFFECTS]; //from efftext.2da

// FIXME: this list should be dynamic (stl::vector). It should be populated
//   by fx plugins, so it would be easier to add new effects etc.
// FIXME: Make this an ordered list, so we could use bsearch!
static EffectLink effectnames[] = {
	{ "Cure:Berserk", fx_cure_berserk_state },
	{ "Cure:Poison", fx_cure_poisoned_state },
	{ "Cure:Sleep", fx_cure_sleep_state },
	{ "HP:MaximumHPModifier", fx_maximum_hp_modifier },
	{ "PlaySound", fx_playsound },
	{ "Stat:ACVsDamageTypeModifier", fx_ac_vs_damage_type_modifier },
	{ "Stat:AttacksPerRoundModifier", fx_attacks_per_round_modifier },
	{ "Stat:CharismaModifier", fx_charisma_modifier },
	{ "Stat:ConstitutionModifier", fx_constitution_modifier },
	{ "Stat:IntelligenceModifier", fx_intelligence_modifier },
	{ "Stat:SaveVsBreathModifier", fx_save_vs_breath_modifier },
	{ "Stat:SaveVsDeathModifier", fx_save_vs_death_modifier },
	{ "Stat:SaveVsPolyModifier", fx_save_vs_poly_modifier },
	{ "Stat:SaveVsSpellsModifier", fx_save_vs_spell_modifier },
	{ "Stat:SaveVsWandsModifier", fx_save_vs_wands_modifier },
	{ "Stat:StrengthModifier", fx_strength_modifier },
	{ "Stat:WisdomModifier", fx_wisdom_modifier },
	{ "Stat:THAC0Modifier", fx_to_hit_modifier },
	{ "State:Berserk", fx_set_berserk_state },
	{ "State:Charmed", fx_set_charmed_state },
	{ "State:Sleep", fx_set_sleep_state },
	{ "Variable:StoreLocalVariable", fx_local_variable },
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

	ApplyAllEffects( Owner );

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
			ApplyAllEffects( Owner );
			return true;
		}
	}
	return false;
	
}

void EffectQueue::ApplyAllEffects(Actor* target)
{
	// copy BaseStats to Modified
	memcpy( target->Modified, target->BaseStats, sizeof( target->BaseStats ) );

	// FIXME: clear protection_from_opcode array
	// memset( target->protection_from_fx, 0, MAX_FX_OPCODES * sizeof( char ) );

	for (std::vector< Effect* >::iterator f = effects.begin(); f != effects.end(); f++ ) {
		ApplyEffect( target, *f );
	}
}


void EffectQueue::ApplyEffect(Actor* target, Effect* fx)
{
	//printf( "FX 0x%02x: %s(%d, %d)\n", fx->Opcode, effectnames[fx->Opcode].Name, fx->Parameter1, fx->Parameter2 );

	if ( effect_refs[fx->Opcode].EffText > 0 ) {
		char *text = core->GetString( effect_refs[fx->Opcode].EffText );
		core->DisplayString( text );
		free( text );
	}

	EffectFunction  fn = effect_refs[fx->Opcode].Function;
	if (fn)
		fn( target, fx );
	//else
	//	printf( "FX function not found: 0x%02x\n", fx->Opcode );
		
}

void EffectQueue::dump()
{
	printf( "EFFECT QUEUE:\n" );
	for (unsigned int i = 0; i < effects.size (); i++ ) {
		Effect* fx = effects[i];
		if (fx) {
			printf( "  %2d:  0x%02x: %s (%d, %d)\n", i, fx->Opcode, effect_refs[fx->Opcode].Name, fx->Parameter1, fx->Parameter2 );
		}
	}
}

// Helper macros and functions for effect opcodes

#define CHECK_LEVEL() { \
	int level = target->GetStat( IE_LEVEL ); \
	if ((fx->DiceSides != 0 || fx->DiceThrown != 0) && (level < (int)fx->DiceSides || level > (int)fx->DiceThrown)) \
		return FX_NOT_APPLIED; \
	}

inline int MIN(int a, int b)
{
	return (a > b ? b : a);
}

inline int MAX(int a, int b)
{
	return (a < b ? b : a);
}

// FIXME: Dice roll should be probably done just once, e.g. when equpping 
//   the item, not each time the fx are applied
#define DICE_ROLL(max_val)   ((fx->DiceThrown && fx->DiceSides) ? ((max_val >=0) ? (MIN( core->Roll( fx->DiceThrown, fx->DiceSides, 0 ), max_val )) : (MAX( core->Roll( fx->DiceThrown, fx->DiceSides, 0 ), max_val ))) : max_val)

// often used stat modifications, usually Parameter2 types 0, 1 and 2
//#define STAT_ADD(stat, mod)  target->SetStat( ( stat ), (ieDword)(target->GetStat( stat ) + ( mod )))
//#define STAT_SET(stat, mod)  target->SetStat( ( stat ), (ieDword)( mod ))
//#define STAT_MUL(stat, mod)  target->SetStat( ( stat ), (ieDword)(target->GetStat( stat ) * (( mod ) / 100.0)))

#define STAT_ADD(stat, mod)  target->Modified[ stat ] = (ieDword)(target->Modified[ stat ] + ( mod ))
#define STAT_SET(stat, mod)  target->Modified[ stat ] = (ieDword)( mod )
#define STAT_MUL(stat, mod)  target->Modified[ stat ] = (ieDword)(target->Modified[ stat ] * (( mod ) / 100.0))
#define STATE_CURE( mod ) target->Modified[ IE_STATE_ID ] &= ~(ieDword) ( mod )
#define STATE_SET( mod ) target->Modified[ IE_STATE_ID ] |= (ieDword) ( mod )


// Effect opcodes
// FIXME: These should be moved into their own plugins
// NOTE: These opcode numbers are true for PS:T and are meant just for 
//   better orientation

// 0x00
int fx_ac_vs_damage_type_modifier (Actor* target, Effect* fx)
{
	if (0) printf( "fx_ac_vs_damage_type_modifier (%2d): AC Modif: %d ; Type: %d ; MinLevel: %d ; MaxLevel: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2, (int) fx->DiceSides, (int) fx->DiceThrown );
	CHECK_LEVEL();

	// it is a bitmask
	int type = fx->Parameter2;
	if (type == 0) {
		// FIXME: this is probably wrong, but it's hack to see
		//   anything in PST
		STAT_ADD( IE_ARMORCLASS, fx->Parameter1 );
		type = 15;
	}

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
int fx_attacks_per_round_modifier (Actor* target, Effect* fx)
{
	if (0) printf( "fx_attacks_per_round_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	target->NewStat( IE_NUMBEROFATTACKS, fx->Parameter1, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x02
int fx_cure_sleep_state (Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_sleep_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_CURE( STATE_SLEEP );
	return FX_APPLIED;
}

// 0x03
int fx_cure_berserk_state (Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_berserk_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_CURE( STATE_BERSERK );
	return FX_APPLIED;
}

// 0x04
int fx_set_berserk_state (Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_berserk_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_SET( STATE_BERSERK );
	return FX_APPLIED;
}

// 0x05
//fixme, this is much more complex, alters IE_EA too
int fx_set_charmed_state (Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_charmed_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_SET( STATE_CHARMED );
	return FX_APPLIED;
}

// 0x06
int fx_charisma_modifier (Actor* target, Effect* fx)
{
	if (0) printf( "fx_charisma_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->NewStat( IE_CHR, fx->Parameter1, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x0A
int fx_constitution_modifier (Actor* target, Effect* fx)
{
	if (0) printf( "fx_constitution_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->NewStat( IE_CON, fx->Parameter1, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x0B
int fx_cure_poisoned_state (Actor* target, Effect* fx)
{
	if (0) printf( "fx_cure_poisoned_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_CURE( STATE_POISONED );
	return FX_APPLIED;
}

// 0x0C Damage
// this is a very important effect
int fx_damage (Actor* target, Effect* fx)
{
	if (0) printf( "fx_damage (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	int damage; //FIXME damage calculation, random damage, etc

	damage = 1;
	target->Damage(damage, fx->Parameter2, target); //FIXME!
	return FX_APPLIED;
}

//0x0D
int fx_death (Actor* target, Effect* fx)
{
	if (0) printf( "fx_death (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	target->Die(target); //FIXME!
	return FX_APPLIED;
}

// 0x12
int fx_maximum_hp_modifier (Actor* target, Effect* fx)
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
int fx_intelligence_modifier (Actor* target, Effect* fx)
{
	if (0) printf( "fx_intelligence_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->NewStat( IE_INT, fx->Parameter1, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x21
int fx_save_vs_death_modifier (Actor* target, Effect* fx)
{
	if (0) printf( "fx_save_vs_death_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->NewStat( IE_SAVEVSDEATH, fx->Parameter1, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x22
int fx_save_vs_wands_modifier (Actor* target, Effect* fx)
{
	if (0) printf( "fx_save_vs_wands_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->NewStat( IE_SAVEVSWANDS, fx->Parameter1, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x23
int fx_save_vs_poly_modifier (Actor* target, Effect* fx)
{
	if (0) printf( "fx_save_vs_poly_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->NewStat( IE_SAVEVSPOLY, fx->Parameter1, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x24
int fx_save_vs_breath_modifier (Actor* target, Effect* fx)
{
	if (0) printf( "fx_save_vs_breath_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->NewStat( IE_SAVEVSBREATH, fx->Parameter1, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x25
int fx_save_vs_spell_modifier (Actor* target, Effect* fx)
{
	if (0) printf( "fx_save_vs_spell_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->NewStat( IE_SAVEVSSPELL, fx->Parameter1, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x27
int fx_set_sleep_state (Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_sleep_state (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STATE_SET( STATE_SLEEP );
	return FX_APPLIED;
}

// 0x2A
int fx_bonus_wizard_spells (Actor* target, Effect* fx)
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

// 0x2C
int fx_strength_modifier (Actor* target, Effect* fx)
{
	if (0) printf( "fx_strength_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->NewStat( IE_STR, fx->Parameter1, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x31
int fx_wisdom_modifier (Actor* target, Effect* fx)
{
	if (0) printf( "fx_wisdom_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->NewStat( IE_WIS, fx->Parameter1, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x36
int fx_to_hit_modifier (Actor* target, Effect* fx)
{
	if (0) printf( "fx_to_hit_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->NewStat( IE_THAC0, fx->Parameter1, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x3B
int fx_stealth_bonus (Actor* target, Effect* fx)
{
	if (0) printf( "fx_stealth_bonus (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->NewStat( IE_STEALTH, fx->Parameter1, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x49
int fx_damage_bonus (Actor* target, Effect* fx)
{
	if (0) printf( "fx_damage_bonus (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->NewStat( IE_DAMAGEBONUS, fx->Parameter1, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x5A
int fx_open_locks_modifier (Actor* target, Effect* fx)
{
	if (0) printf( "fx_open_locks_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->NewStat( IE_LOCKPICKING, fx->Parameter1, fx->Parameter2 );
	return FX_APPLIED;
}

// 0xA6
int fx_resistance_to_magic_damage (Actor* target, Effect* fx)
{
	if (0) printf( "fx_resistance_to_magic_damage (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->NewStat( IE_MAGICDAMAGERESISTANCE, fx->Parameter1, fx->Parameter2 );
	return FX_APPLIED;
}

// 0xBB
int fx_local_variable (Actor* target, Effect* fx)
{
	//this is a hack, the variable name spreads across the resources
	if (0) printf( "fx_local_variable (%s=%d)", fx->Resource, fx->Parameter2 );
	target->locals->SetAt(fx->Resource, fx->Parameter2);
	return FX_APPLIED;
}

int fx_playsound (Actor* target, Effect* fx)
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

