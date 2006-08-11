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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/EffectQueue.cpp,v 1.72 2006/08/11 23:17:19 avenger_teambg Exp $
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
/*
#define FX_DURATION_INSTANT_LIMITED          0
#define FX_DURATION_INSTANT_PERMANENT        1
#define FX_DURATION_INSTANT_WHILE_EQUIPPED   2
#define FX_DURATION_DELAY_LIMITED            3 //this contains a relative onset time (delay) also used as duration, transforms to 6 when applied
#define FX_DURATION_DELAY_PERMANENT          4 //this transforms to 7 (i guess)
#define FX_DURATION_DELAY_UNSAVED            5 //this transforms to 8
#define FX_DURATION_DELAY_LIMITED_PENDING    6 //this contains an absolute onset time and a duration
#define FX_DURATION_AFTER_EXPIRES            7 //this is a delayed non permanent effect (resolves to JUST_EXPIRED)
#define FX_DURATION_PERMANENT_UNSAVED        8
#define FX_DURATION_INSTANT_PERMANENT_AFTER_BONUSES   9//this is a special permanent
#define FX_DURATION_JUST_EXPIRED             10

#define MAX_TIMING_MODE 11
*/
static bool fx_instant[MAX_TIMING_MODE]={true,true,true,false,false,false,false,false,true,true,true};

inline bool IsInstant(ieByte timingmode)
{
	if (timingmode>=MAX_TIMING_MODE) return false;
	return fx_instant[timingmode];
}
//                                           0   1     2     3     4  5    6     7       8
static bool fx_relative[MAX_TIMING_MODE]={true,false,false,true,true,true,false,false,false,false,false};

inline bool NeedPrepare(ieByte timingmode)
{
	if (timingmode>=MAX_TIMING_MODE) return false;
	return fx_relative[timingmode];
}

#define INVALID  -1
#define PERMANENT 0
#define DELAYED   1
#define DURATION  2

static int fx_prepared[MAX_TIMING_MODE]={DURATION,PERMANENT,PERMANENT,DELAYED, //0-3
DELAYED,DELAYED,DELAYED,DELAYED,PERMANENT,PERMANENT,PERMANENT};                //4-7

inline int IsPrepared(ieByte timingmode)
{
	if (timingmode>=MAX_TIMING_MODE) return INVALID;
	return fx_prepared[timingmode];
}

//change the timing method after the effect triggered
static ieByte fx_triggered[MAX_TIMING_MODE]={FX_DURATION_JUST_EXPIRED,FX_DURATION_INSTANT_PERMANENT,//0,1
FX_DURATION_INSTANT_WHILE_EQUIPPED,FX_DURATION_DELAY_LIMITED_PENDING,//2,3
FX_DURATION_AFTER_EXPIRES,FX_DURATION_PERMANENT_UNSAVED, //4,5
FX_DURATION_INSTANT_LIMITED,FX_DURATION_JUST_EXPIRED,FX_DURATION_PERMANENT_UNSAVED,//6,8
FX_DURATION_INSTANT_PERMANENT_AFTER_BONUSES,FX_DURATION_JUST_EXPIRED};//9,10

//change the timing method for effect that should trigger after this effect expired
static ieDword fx_to_delayed[]={FX_DURATION_AFTER_EXPIRES,FX_DURATION_JUST_EXPIRED,
FX_DURATION_PERMANENT_UNSAVED,FX_DURATION_DELAY_LIMITED_PENDING,
FX_DURATION_AFTER_EXPIRES,FX_DURATION_PERMANENT_UNSAVED, //4,5
FX_DURATION_JUST_EXPIRED,FX_DURATION_JUST_EXPIRED,FX_DURATION_JUST_EXPIRED,//6,8
FX_DURATION_JUST_EXPIRED,FX_DURATION_JUST_EXPIRED};//9,10

inline ieByte TriggeredEffect(ieByte timingmode)
{
	if (timingmode>=MAX_TIMING_MODE) return false;
	return fx_triggered[timingmode];
}

inline bool valid_number(const char* string, long& val)
{
	char* endpr;

	val = strtol( string, &endpr, 0 );
	return ( const char * ) endpr != string;
}

int compare_effects(const void *a, const void *b)
{
	return stricmp(((EffectRef *) a)->Name,((EffectRef *) b)->Name);
}

int find_effect(const void *a, const void *b)
{
	return stricmp((const char *) a,((const EffectRef *) b)->Name);
}

static EffectRef* FindEffect(const char* effectname)
{
	if (!effectname || !effectnames) {
		return NULL;
	}
	void *tmp = bsearch(effectname, effectnames, opcodes_count, sizeof(EffectRef), find_effect);
	if (!tmp) {
		printf( "Warning: Couldn't assign effect: %s\n", effectname );
	}
	return (EffectRef *) tmp;
}

//special effects without level check (but with damage dices precalculated)
static EffectRef diced_effects[] = {
	//core effects
	{"Damage",NULL,-1},
	{"CurrentHPModifier",NULL,-1},
	{"MaximumHPModifier",NULL,-1},
	//iwd effects
	{"ColdDamage",NULL,-1},
	{"CrushingDamage",NULL,-1},
	{"VampiricTouch",NULL,-1},
	{"BurningBlood",NULL,-1},
	{"VitriolicSphere",NULL,-1},
	//pst effects
	{"TransferHP",NULL,-1},
{NULL,NULL,0} };

//special effects without level check (but with damage dices not precalculated)
static EffectRef diced_effects2[] = {
	{"BurningBlood2",NULL,-1}, //iwd2
	{"LichTouch",NULL,-1}, //how
{NULL,NULL,0} };

inline static void ResolveEffectRef(EffectRef &effect_reference)
{
	if (effect_reference.EffText==-1) {
		EffectRef* ref = FindEffect(effect_reference.Name);
		if (ref) {
			effect_reference.EffText = ref->EffText;
		} else {
			effect_reference.EffText = -2;
		}
	}
}

bool Init_EffectQueue()
{
	int i;

	if (initialized) {
		return true;
	}
	TableMgr* efftextTable=NULL;
	SymbolMgr* effectsTable;
	memset( effect_refs, 0, sizeof( effect_refs ) );

	initialized = 1;

	int effT = core->LoadTable( "efftext" );
	efftextTable = core->GetTable( effT );

	int eT = core->LoadSymbol( "effects" );
	if (eT < 0) {
		printMessage( "EffectQueue","A critical scripting file is missing!\n",LIGHT_RED );
		return false;
	}
	effectsTable = core->GetSymbol( eT );
	if (!effectsTable) {
		printMessage( "EffectQueue","A critical scripting file is damaged!\n",LIGHT_RED );
		return false;
	}

	for (i = 0; i < MAX_EFFECTS; i++) {
		const char* effectname = effectsTable->GetValue( i );
		if (efftextTable) {
			int row = efftextTable->GetRowCount();
			while (row--) {
				const char* ret = efftextTable->GetRowName( row );
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
			//reverse linking opcode number
			//using this unused field
			if (poi->EffText && effectname[0]!='*') {
				printf("Clashing opcodes FN: %d vs. %d, %s\n", i, poi->EffText, effectname);
				abort();
			}
			poi->EffText = i;
		}
		//printf("-------- FN: %d, %s\n", i, effectname);
	}
	core->DelSymbol( eT );
	if ( efftextTable ) core->DelTable( effT );

	//additional initialisations
	for (i=0;diced_effects[i].Name;i++) {
		ResolveEffectRef(diced_effects[i]);
	}
	for (i=0;diced_effects2[i].Name;i++) {
		ResolveEffectRef(diced_effects2[i]);
	}

	return true;
}

void EffectQueue_ReleaseMemory()
{
	if (effectnames) {
		free (effectnames);
	}
	opcodes_count = 0;
	effectnames = NULL;
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
	//if we merge two effect lists, then we need to sort their effect tables
	//actually, we might always want to sort this list, so there is no 
	//need to do it manually (sorted table is needed if we use bsearch)
	qsort(effectnames, opcodes_count, sizeof(EffectRef), compare_effects);
}

EffectQueue::EffectQueue()
{
	Owner = NULL;
//	opcodes_count = 0;
}

EffectQueue::~EffectQueue()
{
	for (unsigned i = 0; i < effects.size(); i++) {
		delete( effects[i] );
	}
}

bool EffectQueue::AddEffect(Effect* fx)
{
	Effect* new_fx = new Effect;
	memcpy( new_fx, fx, sizeof( Effect ) );
	effects.push_back( new_fx );

	return true;
}

bool EffectQueue::RemoveEffect(Effect* fx)
{
	int invariant_size = offsetof( Effect, random_value );

	for (std::vector< Effect* >::iterator f = effects.begin(); f != effects.end(); f++ ) {
		Effect* fx2 = *f;

		if ( (fx==fx2) || !memcmp( fx, fx2, invariant_size)) {
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
	//nah, we already been through this
	//ieDword random_value = core->Roll( 1, 100, 0 );

	std::vector< Effect* >::iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		//(*f)->random_value = random_value;
		//no idea if we should honour FX_ABORT here (resistspell)
		//if yes, then break when applyeffect returned true
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
	// pre-roll dice for fx needing them and stow them in the effect
	ieDword random_value = core->Roll( 1, 100, 0 );

	std::vector< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		//handle resistances and saving throws here
		(*f)->random_value = random_value;
		(*f)->PosX = target->Pos.x;
		(*f)->PosY = target->Pos.y;
		//if applyeffect returns true, we stop adding the future effects
		bool flg = target->fxqueue.ApplyEffect( target, *f, true );
		if ((*f)->TimingMode!=FX_DURATION_JUST_EXPIRED) {
			target->fxqueue.AddEffect(*f);
		}
		if (flg) {
			break;
		}
	}
}

bool IsDicedEffect(int opcode)
{
	int i;

	for(i=0;diced_effects[i].Name;i++) {
		if(diced_effects[i].EffText==opcode) {
			return true;
		}
	}
	return false;
}

bool IsDicedEffect2(int opcode)
{
	int i;

	for(i=0;diced_effects2[i].Name;i++) {
		if(diced_effects2[i].EffText==opcode) {
			return true;
		}
	}
	return false;
}

//resisted effect based on level
inline bool check_level(Actor *target, Effect *fx)
{
	//skip non level based effects
	if (IsDicedEffect((int) fx->Opcode)) {
		fx->Parameter1 = DICE_ROLL((signed)fx->Parameter1);
		//this is a hack for PST style diced effects
		if (core->HasFeature(GF_SAVE_FOR_HALF) ) {
			if (memcmp(fx->Resource,"NEG",4) ) {
				fx->IsSaveForHalfDamage=1;
			}
		} else {
			if ((fx->Parameter2&0xff)==3) {
				fx->IsSaveForHalfDamage=1;
			}
		}
		return false;
	}
	if (IsDicedEffect2((int) fx->Opcode)) {
		return false;
	}
	int level = target->GetXPLevel( true );
	if ((fx->DiceSides != 0 || fx->DiceThrown != 0) && (level < (int)fx->DiceSides || level > (int)fx->DiceThrown)) {
		return true;
	}
	return false;
}

inline bool check_probability(Effect* fx)
{
	//watch for this, probability1 is the high number
	//probability2 is the low number
	//random value is 1-100
	if (fx->random_value<=fx->Probability2 || fx->random_value>fx->Probability1) {
		return false;
	}
	return true;
}

static EffectRef fx_level_immunity_ref={"Protection:Spelllevel",NULL,-1};
static EffectRef fx_opcode_immunity_ref={"Protection:Opcode",NULL,-1};  //bg2
static EffectRef fx_opcode_immunity2_ref={"Protection:Opcode2",NULL,-1};//iwd
static EffectRef fx_spell_immunity_ref={"Protection:Spell",NULL,-1};  //bg2
static EffectRef fx_spell_immunity2_ref={"Protection:Spell2",NULL,-1};//iwd
static EffectRef fx_school_immunity_ref={"Protection:School",NULL,-1};
static EffectRef fx_secondary_type_immunity_ref={"Protection:SecondaryType",NULL,-1};

static EffectRef fx_level_bounce_ref={"Bounce:SpellLevel",NULL,-1};
//static EffectRef fx_opcode_bounce_ref={"Bounce:Opcode",NULL,-1};
static EffectRef fx_spell_bounce_ref={"Bounce:Spell",NULL,-1};
static EffectRef fx_school_bounce_ref={"Bounce:School",NULL,-1};
static EffectRef fx_secondary_type_bounce_ref={"Bounce:SecondaryType",NULL,-1};

//this is for whole spell immunity/bounce
inline int check_type(Actor* actor, Effect* fx)
{
	ieDword bounce = actor->GetStat(IE_BOUNCE);

	//immunity checks
/*opcode immunity is elsewhere
	if (actor->fxqueue.HasEffectWithParam(fx_opcode_immunity_ref, fx->Opcode) ) {
		return 0;
	}
	if (actor->fxqueue.HasEffectWithParam(fx_opcode_immunity2_ref, fx->Opcode) ) {
		return 0;
	}
*/
	if (actor->fxqueue.HasEffectWithParamPair(fx_level_immunity_ref, fx->Power, 0) ) {
		return 0;
	}
	if (actor->fxqueue.HasEffectWithResource(fx_spell_immunity_ref, fx->Source) ) {
		return 0;
	}
	if (actor->fxqueue.HasEffectWithResource(fx_spell_immunity2_ref, fx->Source) ) {
		return 0;
	}
	if (fx->PrimaryType) {
		if (actor->fxqueue.HasEffectWithParam(fx_school_immunity_ref, fx->PrimaryType)) {
			return 0;
		}
	}

	if (fx->SecondaryType) {
		if (actor->fxqueue.HasEffectWithParam(fx_secondary_type_immunity_ref, fx->SecondaryType) ) {
			return 0;
		}
	}

	//bounce checks
	if ((bounce&BNC_LEVEL) && actor->fxqueue.HasEffectWithParamPair(fx_level_bounce_ref, fx->Power, 0) ) {
		return 0;
	}

	if ((bounce&BNC_RESOURCE) && actor->fxqueue.HasEffectWithResource(fx_spell_bounce_ref, fx->Source) ) {
		return -1;
	}

	if (fx->PrimaryType && (bounce&BNC_SCHOOL) ) {
		if (actor->fxqueue.HasEffectWithParam(fx_school_bounce_ref, fx->PrimaryType)) {
			return -1;
		}
	}

	if (fx->SecondaryType && (bounce&BNC_SECTYPE) ) {
		if (actor->fxqueue.HasEffectWithParam(fx_secondary_type_bounce_ref, fx->SecondaryType)) {
			return -1;
		}
	}

	//decrementing bounce checks
	return 1;
}

inline bool check_resistance(Actor* actor, Effect* fx)
{
	//magic immunity
	ieDword val = actor->GetStat(IE_RESISTMAGIC);
	if (fx->random_value < val) {
		return false;
	}
	//opcode immunity
	if (actor->fxqueue.HasEffectWithParam(fx_opcode_immunity_ref, fx->Opcode) ) {
		return false;
	}
	if (actor->fxqueue.HasEffectWithParam(fx_opcode_immunity2_ref, fx->Opcode) ) {
		return false;
	}
/* opcode bouncing isn't implemented?
	//opcode bouncing
	if (actor->fxqueue.HasEffectWithParam(fx_opcode_bounce_ref, fx->Opcode) ) {
		return false;
	}
*/

	//saving throws
	bool saved = false;
	ieDword j=1;
	for (int i=0;i<5;i++) {
		if (fx->SavingThrowType&j) {
			saved = actor->GetSavingThrow(i, fx->SavingThrowBonus);
			if (saved) {
				break;
			}
		}
	}
	if (saved) {
		if (fx->IsSaveForHalfDamage) {
			fx->Parameter1/=2;
		}
		else {
			return false;
		}
	}
	return true;
}

// this function is called two different ways
// when first_apply is set, then the effect isn't stuck on the target
// this happens when a new effect comes in contact with the target.
// if the effect returns FX_DURATION_JUST_EXPIRED then it won't stick
// when first_apply is unset, the effect is already on the target
// this happens on load time too!
// returns true if the process should stop calling applyeffect anymore

bool EffectQueue::ApplyEffect(Actor* target, Effect* fx, bool first_apply)
{
	if (!target) {
		fx->TimingMode=FX_DURATION_JUST_EXPIRED;
		return true;
	}
	//printf( "FX 0x%02x: %s(%d, %d)\n", fx->Opcode, effectnames[fx->Opcode].Name, fx->Parameter1, fx->Parameter2 );
	if (fx->Opcode >= MAX_EFFECTS) {
		fx->TimingMode=FX_DURATION_JUST_EXPIRED;
		return false;
	}

	ieDword GameTime = core->GetGame()->GameTime;

	if (first_apply) {
		//the effect didn't pass the probability check
		if (!check_probability(fx) ) {
			fx->TimingMode=FX_DURATION_JUST_EXPIRED;
			return false;
		}

		//the effect didn't pass the target level check
		if (check_level(target, fx) ) {
			fx->TimingMode=FX_DURATION_JUST_EXPIRED;
			return false;
		}

		//the effect didn't pass the resistance check
		if(fx->Resistance == FX_CAN_RESIST_CAN_DISPEL ||
			fx->Resistance == FX_CAN_RESIST_NO_DISPEL) {
			if (check_resistance(target, fx) ) {
				fx->TimingMode=FX_DURATION_JUST_EXPIRED;
				return false;
			}
		}
		if (NeedPrepare((ieByte) fx->TimingMode) ) {
			PrepareDuration(fx);
		}
	}
	//check if the effect has triggered or expired
	switch (IsPrepared((ieByte) fx->TimingMode) ) {
	case DELAYED:
		if (fx->Duration>GameTime) {
			return false;
		}
		//effect triggered
		fx->TimingMode=TriggeredEffect((ieByte) fx->TimingMode);
		break;
	case DURATION:
		if (fx->Duration>GameTime) {
			fx->TimingMode=FX_DURATION_JUST_EXPIRED;
			//add a return here, if 0 duration effects shouldn't work
		}
		break;
	//permanent effect (so there is no warning)
	case PERMANENT:
		break;
	//this shouldn't happen
	default:
		abort();
	}
	
	EffectFunction fn = effect_refs[fx->Opcode].Function;
	bool flg = false;
	if (fn) {		
		if ( effect_refs[fx->Opcode].EffText > 0 ) {
			char *text = core->GetString( effect_refs[fx->Opcode].EffText );
			core->DisplayString( text );
			free( text );
		}

		int res=fn( Owner?Owner:target, target, fx );

		//if there is no owner, we assume it is the target
		switch( res ) {
			case FX_APPLIED:
				//normal effect with duration
				break;
			case FX_NOT_APPLIED:
 				//instant effect, pending removal
				//for example, a damage effect
				fx->TimingMode=FX_DURATION_JUST_EXPIRED;
				break;
			case FX_PERMANENT:
				//don't stick around if it was permanent
				//for example, a strength modifier effect
				if ((fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) ) {
					fx->TimingMode=FX_DURATION_JUST_EXPIRED;
				} 
				break;
			case FX_ABORT:
				flg = true;
				break;
			default:
				abort();
		}
	} else {
		//effect not found, it is going to be discarded
		fx->TimingMode=FX_DURATION_JUST_EXPIRED;
	}
	//delayed duration (3)
	if (NeedPrepare((ieByte) fx->TimingMode) ) {
		//prepare for delayed duration effects
		fx->Duration=fx->SecondaryDelay;
		PrepareDuration(fx);      
	}
	return flg;
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
#define MATCH_SOURCE() if( strnicmp( (*f)->Source, Removed, 8) ) { continue; }
#define MATCH_TIMING() if ((*f)->TimingMode!=timing) { continue; }

//call this from an applied effect, after it returns, these effects
//will be killed along with it
void EffectQueue::RemoveAllEffects(ieDword opcode)
{
	std::vector< Effect* >::iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();

		(*f)->TimingMode=FX_DURATION_JUST_EXPIRED;
	}
}

//remove effects belonging to a given spell
void EffectQueue::RemoveAllEffects(ieResRef Removed)
{
	std::vector< Effect* >::iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_LIVE_FX();
		MATCH_SOURCE();

		(*f)->TimingMode=FX_DURATION_JUST_EXPIRED;
	}
}

//remove effects belonging to a given spell, but only if they match timing method x
void EffectQueue::RemoveAllEffects(ieResRef Removed, ieDword timing)
{
	std::vector< Effect* >::iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_TIMING();
		MATCH_SOURCE();

		(*f)->TimingMode=FX_DURATION_JUST_EXPIRED;
	}
}

//this will modify effect reference
void EffectQueue::RemoveAllEffects(EffectRef &effect_reference)
{
	ResolveEffectRef(effect_reference);
	RemoveAllEffects(effect_reference.EffText);
}

void EffectQueue::RemoveAllEffectsWithResource(ieDword opcode, const ieResRef resource)
{
	std::vector< Effect* >::iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();
		MATCH_RESOURCE();

		(*f)->TimingMode=FX_DURATION_JUST_EXPIRED;
	}
}

void EffectQueue::RemoveAllEffectsWithResource(EffectRef &effect_reference, const ieResRef resource)
{
	ResolveEffectRef(effect_reference);
	RemoveAllEffectsWithResource(effect_reference.EffText, resource);
}

void EffectQueue::RemoveAllEffectsWithParam(ieDword opcode, ieDword param2)
{
	std::vector< Effect* >::iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();
		MATCH_PARAM2();

		(*f)->TimingMode=FX_DURATION_JUST_EXPIRED;
	}
}

//this function is called by FakeEffectExpiryCheck
//probably also called by rest
void EffectQueue::RemoveExpiredEffects(ieDword futuretime)
{
	ieDword GameTime = core->GetGame()->GameTime;

	std::vector< Effect* >::iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		//FIXME: how this method handles delayed effects???
		if ( IsPrepared( (ieByte) ((*f)->TimingMode) )==DURATION ) {
			if ( (*f)->Duration<=GameTime+futuretime) {
				(*f)->TimingMode=FX_DURATION_JUST_EXPIRED;
			}      
		}
	}
}

//this effect will expire all effects that are not truly permanent 
//which i call permanent after death (iesdp calls it permanent after bonuses)
void EffectQueue::RemoveAllNonPermanentEffects()
{
	std::vector< Effect* >::iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		if( (*f)->TimingMode != FX_DURATION_INSTANT_PERMANENT_AFTER_BONUSES) {
			(*f)->TimingMode=FX_DURATION_JUST_EXPIRED;
		}
	}
}

//this will modify effect reference
void EffectQueue::RemoveAllEffectsWithParam(EffectRef &effect_reference, ieDword param2)
{
	ResolveEffectRef(effect_reference);
	RemoveAllEffectsWithParam(effect_reference.EffText, param2);
}

void EffectQueue::RemoveLevelEffects(ieDword level, ieDword Flags, ieDword match)
{
	ieResRef Removed;

	Removed[0]=0;
	std::vector< Effect* >::iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		if ( (*f)->Power>level) {
			continue;
		}

		if (Removed[0]) {
			MATCH_SOURCE();
		}
		if (Flags&RL_MATCHSCHOOL) {
			if ((*f)->PrimaryType!=match) {
				continue;
			}
		}
		if (Flags&RL_MATCHSECTYPE) {
			if ((*f)->SecondaryType!=match) {
				continue;
			}
		}
		//if dispellable was not set, or the effect is dispellable
		//then remove it
		if (Flags&RL_DISPELLABLE) {
			if (!(*f)->Resistance&FX_CAN_DISPEL) {
				continue;
			}
		}
		(*f)->TimingMode=FX_DURATION_JUST_EXPIRED;
		if (Flags&RL_REMOVEFIRST) {
			memcpy(Removed,(*f)->Source, sizeof(Removed));
		}
	}
}


Effect *EffectQueue::HasOpcode(ieDword opcode) const
{
	std::vector< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();

		return (*f);
	}
	return NULL;
}

Effect *EffectQueue::HasEffect(EffectRef &effect_reference)
{
	ResolveEffectRef(effect_reference);
	if (effect_reference.EffText<0) {
		return NULL;
	}
	return HasOpcode(effect_reference.EffText);
}

Effect *EffectQueue::HasOpcodeWithParam(ieDword opcode, ieDword param2) const
{
	std::vector< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();
		MATCH_PARAM2();

		return (*f);
	}
	return NULL;
}

Effect *EffectQueue::HasEffectWithParam(EffectRef &effect_reference, ieDword param2)
{
	ResolveEffectRef(effect_reference);
	if (effect_reference.EffText<0) {
		return NULL;
	}
	return HasOpcodeWithParam(effect_reference.EffText, param2);
}

//looks for opcode with pairs of parameters (useful for protection against creature, extra damage or extra thac0 against creature)
//generally an IDS targeting

Effect *EffectQueue::HasOpcodeWithParamPair(ieDword opcode, ieDword param1, ieDword param2) const
{
	std::vector< Effect* >::const_iterator f;
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

Effect *EffectQueue::HasEffectWithParamPair(EffectRef &effect_reference, ieDword param1, ieDword param2)
{
	ResolveEffectRef(effect_reference);
	if (effect_reference.EffText<0) {
		return NULL;
	}
	return HasOpcodeWithParamPair(effect_reference.EffText, param1, param2);
}

//this function does IDS targeting for effects (extra damage/thac0 against creature)
static int ids_stats[7]={IE_EA, IE_GENERAL, IE_RACE, IE_CLASS, IE_SPECIFIC, IE_SEX, IE_ALIGNMENT};

int EffectQueue::BonusAgainstCreature(ieDword opcode, Actor *actor) const
{
	int sum = 0;
	std::vector< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();
		ieDword ids = (*f)->Parameter2;
		if (ids<2 || ids>9) {
			ids=2;
		}
		ieDword param1 = actor->GetStat(ids_stats[ids-2]);
		if ( (*f)->Parameter1) {
			MATCH_PARAM1();
		}
		int val = (int) (*f)->Parameter3;
		if (!val) val = 2;
		sum += val;
	}
	return sum;
}

int EffectQueue::BonusAgainstCreature(EffectRef &effect_reference, Actor *actor)
{
	ResolveEffectRef(effect_reference);
	if (effect_reference.EffText<0) {
		return 0;
	}
	return BonusAgainstCreature(effect_reference.EffText, actor);
}

//useful for immunity vs spell, can't use item, etc.
Effect *EffectQueue::HasOpcodeWithResource(ieDword opcode, const ieResRef resource) const
{
	std::vector< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();
		MATCH_RESOURCE();

		return (*f);
	}
	return NULL;
}

Effect *EffectQueue::HasEffectWithResource(EffectRef &effect_reference, const ieResRef resource)
{
	ResolveEffectRef(effect_reference);
	return HasOpcodeWithResource(effect_reference.EffText, resource);
}

bool EffectQueue::HasAnyDispellableEffect() const
{
	std::vector< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		if ((*f)->Resistance&FX_CAN_DISPEL) {
			return true;
		}
	}
	return false;
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

Effect *EffectQueue::GetEffect(ieDword idx) const
{
	if (effects.size()<=idx) {
		return NULL;
	}
	return effects[idx];
}

Effect *EffectQueue::GetNextSavedEffect(ieDword &idx) const
{
	while(effects.size()>idx) {
		Effect *effect = effects[idx++];
		if (Persistent(effect)) {
			return effect;
		}
	}
	return NULL;
}

ieDword EffectQueue::GetSavedEffectsCount() const
{
	ieDword cnt = 0;

	for (unsigned int i = 0; i < effects.size (); i++ ) {
		Effect* fx = effects[i];
		if (Persistent(fx))
			cnt++;
	}
	return cnt;
}

void EffectQueue::TransformToDelay(ieDword &TimingMode)
{
	if (TimingMode<MAX_TIMING_MODE) {;
		TimingMode = fx_to_delayed[TimingMode];
	} else {
		TimingMode = FX_DURATION_JUST_EXPIRED;
	}
}

int EffectQueue::ResolveEffect(EffectRef &effect_reference)
{
	ResolveEffectRef(effect_reference);
	return effect_reference.EffText;
}

// this is a complete level check

//returns 1 if effect block applicable
//returns 0 if effect block disabled
//returns -1 if effect block bounced
int EffectQueue::CheckImmunity(Actor *target)
{
	if (effects.size() ) {
		Effect* fx = effects[0];

		//check level resistances
		//check specific spell immunity
		//check school/sectype immunity
		return check_type(target, fx);
	}
	return 0;
}

