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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/EffectQueue.cpp,v 1.6 2004/11/13 13:03:32 edheldil Exp $
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

EffectQueue::EffectQueue()
{
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


	Effect* new_fx = new Effect;
	memcpy( new_fx, fx, sizeof( Effect ) );
	effects.push_back( new_fx );

	// pre-roll dice for fx needing them and stow them in the effect
	new_fx->random_value = core->Roll( fx->DiceThrown, fx->DiceSides, 0 );

	return true;
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

int fx_ac_vs_damage_type_modifier (Actor* target, Effect* fx);
int fx_condition_modifier (Actor* target, Effect* fx);
int fx_maximum_hp_modifier (Actor* target, Effect* fx);
int fx_save_vs_death_modifier (Actor* target, Effect* fx);
int fx_save_vs_wands_modifier (Actor* target, Effect* fx);
int fx_save_vs_poly_modifier (Actor* target, Effect* fx);
int fx_save_vs_breath_modifier (Actor* target, Effect* fx);
int fx_save_vs_spell_modifier (Actor* target, Effect* fx);
int fx_bonus_wizard_spells (Actor* target, Effect* fx);
int fx_strength_modifier (Actor* target, Effect* fx);
int fx_to_hit_modifier (Actor* target, Effect* fx);
int fx_stealth_bonus (Actor* target, Effect* fx);
int fx_damage_bonus (Actor* target, Effect* fx);
int fx_open_locks_modifier (Actor* target, Effect* fx);
int fx_resistance_to_magic_damage (Actor* target, Effect* fx);

// these opcodes are true for PS:T
void EffectQueue::ApplyEffect(Actor* target, Effect* fx)
{
	switch (fx->Opcode) {
	case 0x00: fx_ac_vs_damage_type_modifier ( target, fx ); break;
	case 0x0A: fx_condition_modifier ( target, fx ); break;
	case 0x12: fx_maximum_hp_modifier ( target, fx ); break;
	case 0x21: fx_save_vs_death_modifier ( target, fx ); break;
	case 0x22: fx_save_vs_wands_modifier ( target, fx ); break;
	case 0x23: fx_save_vs_poly_modifier ( target, fx ); break;
	case 0x24: fx_save_vs_breath_modifier ( target, fx ); break;
	case 0x25: fx_save_vs_spell_modifier ( target, fx ); break;
	case 0x2A: fx_bonus_wizard_spells ( target, fx ); break;
	case 0x2C: fx_strength_modifier ( target, fx ); break;
	case 0x36: fx_to_hit_modifier ( target, fx ); break;
	case 0x3B: fx_stealth_bonus ( target, fx ); break;
	case 0x49: fx_damage_bonus ( target, fx ); break;
	case 0x5A: fx_open_locks_modifier ( target, fx ); break;
	case 0xA6: fx_resistance_to_magic_damage ( target, fx ); break;
	default: printf( "fx_???: (%d) %d %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	}
}

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


// 0x00
int fx_ac_vs_damage_type_modifier (Actor* target, Effect* fx)
{
	printf( "fx_ac_vs_damage_type_modifier (%2d): AC Modif: %d ; Type: %d ; MinLevel: %d ; MaxLevel: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2, (int) fx->DiceSides, (int) fx->DiceThrown );
	CHECK_LEVEL();

	// FIXME: is it bitmask or just a single num?
	int type = fx->Parameter2;
	if (type == 0) type = 15;

	if (type && 1) STAT_ADD( IE_ACCRUSHINGMOD, fx->Parameter1 );
	if (type && 2) STAT_ADD( IE_ACMISSILEMOD, fx->Parameter1 );
	if (type && 4) STAT_ADD( IE_ACPIERCINGMOD, fx->Parameter1 );
	if (type && 8) STAT_ADD( IE_ACSLASHINGMOD, fx->Parameter1 );

	// FIXME: set to Param1 or Param1-1 ?
	if (type == 16 && target->Modified[IE_ARMORCLASS] > fx->Parameter1)
		STAT_SET( IE_ARMORCLASS, fx->Parameter1 );

	return FX_APPLIED;
}

// 0x0A
int fx_condition_modifier (Actor* target, Effect* fx)
{
	printf( "fx_condition_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->NewStat( IE_CON, fx->Parameter1, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x12
int fx_maximum_hp_modifier (Actor* target, Effect* fx)
{
	printf( "fx_maximum_hp_modifier (%2d): Stat Modif: %d ; Modif Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

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

// 0x2A
int fx_bonus_wizard_spells (Actor* target, Effect* fx)
{
	printf( "fx_bonus_wizard_spells (%2d): Spell Add: %d ; Spell Level: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	
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

// 0x21
int fx_save_vs_death_modifier (Actor* target, Effect* fx)
{
	printf( "fx_save_vs_death_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->NewStat( IE_SAVEVSDEATH, fx->Parameter1, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x22
int fx_save_vs_wands_modifier (Actor* target, Effect* fx)
{
	printf( "fx_save_vs_wands_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->NewStat( IE_SAVEVSWANDS, fx->Parameter1, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x23
int fx_save_vs_poly_modifier (Actor* target, Effect* fx)
{
	printf( "fx_save_vs_poly_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->NewStat( IE_SAVEVSPOLY, fx->Parameter1, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x24
int fx_save_vs_breath_modifier (Actor* target, Effect* fx)
{
	printf( "fx_save_vs_breath_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->NewStat( IE_SAVEVSBREATH, fx->Parameter1, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x25
int fx_save_vs_spell_modifier (Actor* target, Effect* fx)
{
	printf( "fx_save_vs_spell_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->NewStat( IE_SAVEVSSPELL, fx->Parameter1, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x2C
int fx_strength_modifier (Actor* target, Effect* fx)
{
	printf( "fx_strength_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->NewStat( IE_STR, fx->Parameter1, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x36
int fx_to_hit_modifier (Actor* target, Effect* fx)
{
	printf( "fx_to_hit_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->NewStat( IE_THAC0, fx->Parameter1, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x3B
int fx_stealth_bonus (Actor* target, Effect* fx)
{
	printf( "fx_stealth_bonus (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->NewStat( IE_STEALTH, fx->Parameter1, fx->Parameter2 );
	return FX_APPLIED;
}

// 0x49
int fx_damage_bonus (Actor* target, Effect* fx)
{
	printf( "fx_damage_bonus (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	switch (fx->Parameter2) {
	case 0: 
		STAT_ADD( IE_DAMAGEBONUS, fx->Parameter1);
		break;
	default:
		STAT_SET( IE_DAMAGEBONUS, fx->Parameter1);
		break;
	}
	return FX_APPLIED;
}

// 0x5A
int fx_open_locks_modifier (Actor* target, Effect* fx)
{
	printf( "fx_open_locks_modifier (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->NewStat( IE_LOCKPICKING, fx->Parameter1, fx->Parameter2 );
	return FX_APPLIED;
}

// 0xA6
int fx_resistance_to_magic_damage (Actor* target, Effect* fx)
{
	printf( "fx_resistance_to_magic_damage (%2d): Mod: %d, Type: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	target->NewStat( IE_MAGICDAMAGERESISTANCE, fx->Parameter1, fx->Parameter2 );
	return FX_APPLIED;
}
