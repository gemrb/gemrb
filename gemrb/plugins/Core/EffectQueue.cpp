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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/EffectQueue.cpp,v 1.46 2005/11/24 17:44:08 wjpalenstijn Exp $
 *
 */

#include <stdio.h>
#include "Interface.h"
#include "Actor.h"
#include "Effect.h"
#include "EffectQueue.h"
#include "SymbolMgr.h"
#include "Game.h"


static int initialized = 0;
static EffectRef *effectnames = NULL;
static EffectRef effect_refs[MAX_EFFECTS];

static int opcodes_count = 0;

inline bool valid_number(const char* string, long& val)
{
	char* endpr;

	val = strtol( string, &endpr, 0 );
	return ( const char * ) endpr != string;
}

static EffectRef* FindEffect(const char* effectname)
{
	if (!effectname || !effectnames) {
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

			EffectRef* poi = FindEffect( effectname );
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

void EffectQueue_RegisterOpcodes(int count, EffectRef* opcodes)
{
	if (! effectnames) {
		effectnames = (EffectRef*) malloc( (count+1) * sizeof( EffectRef ) );
	} else {
		effectnames = (EffectRef*) realloc( effectnames, (opcodes_count + count + 1) * sizeof( EffectRef ) );
	}

	memcpy( effectnames + opcodes_count, opcodes, count * sizeof( EffectRef ));
	opcodes_count += count;
	effectnames[opcodes_count].Name = NULL;
}

EffectQueue::EffectQueue()
{
	Owner = NULL;
	opcodes_count = 0;
}

EffectQueue::~EffectQueue()
{
	for (unsigned i = 0; i < effects.size(); i++) {
		delete( effects[i] );
	}
	if (effectnames) {
		free (effectnames);
	}
	opcodes_count = 0;
	effectnames = NULL;
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

//this is where we reapply all effects when loading a saved game
//The effects are already in the fxqueue of the target
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
//the effects are currently NOT in the target's fxqueue, those that stick
//will get copied (hence the fxqueue.AddEffect call)
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
