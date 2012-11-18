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

#include "EffectQueue.h"

#include "ie_feats.h"
#include "overlays.h"
#include "strrefs.h"

#include "DisplayMessage.h"
#include "Effect.h"
#include "Game.h"
#include "Interface.h"
#include "Map.h"
#include "SymbolMgr.h"
#include "Scriptable/Actor.h"
#include "Spell.h" //needs for the source flags bitfield
#include "TableMgr.h"
#include "System/StringBuffer.h"

#include <cstdio>

namespace GemRB {

static struct {
	const char* Name;
	EffectFunction Function;
	int Strref;
	int Flags;
} Opcodes[MAX_EFFECTS];

static int initialized = 0;
static EffectDesc *effectnames = NULL;
static int effectnames_count = 0;
static int pstflags = false;
static bool iwd2fx = false;

static EffectRef fx_unsummon_creature_ref = { "UnsummonCreature", -1 };
static EffectRef fx_ac_vs_creature_type_ref = { "ACVsCreatureType", -1 };
static EffectRef fx_spell_focus_ref = { "SpellFocus", -1 };
static EffectRef fx_spell_resistance_ref = { "SpellResistance", -1 };
static EffectRef fx_protection_from_display_string_ref = { "Protection:String", -1 };
static EffectRef fx_variable_ref = { "Variable:StoreLocalVariable", -1 };

//immunity effects
static EffectRef fx_level_immunity_ref = { "Protection:Spelllevel", -1 };
static EffectRef fx_opcode_immunity_ref = { "Protection:Opcode", -1 }; //bg2
static EffectRef fx_opcode_immunity2_ref = { "Protection:Opcode2", -1 };//iwd
static EffectRef fx_spell_immunity_ref = { "Protection:Spell", -1 }; //bg2
static EffectRef fx_spell_immunity2_ref = { "Protection:Spell2", -1 };//iwd
static EffectRef fx_store_spell_sequencer_ref = { "Sequencer:Store", -1 }; //bg2, works against sequencers
static EffectRef fx_school_immunity_ref = { "Protection:School", -1 };
static EffectRef fx_secondary_type_immunity_ref = { "Protection:SecondaryType", -1 };

//decrementing immunity effects
static EffectRef fx_level_immunity_dec_ref = { "Protection:SpellLevelDec", -1 };
static EffectRef fx_spell_immunity_dec_ref = { "Protection:SpellDec", -1 };
static EffectRef fx_school_immunity_dec_ref = { "Protection:SchoolDec", -1 };
static EffectRef fx_secondary_type_immunity_dec_ref = { "Protection:SecondaryTypeDec", -1 };

//bounce effects
static EffectRef fx_level_bounce_ref = { "Bounce:SpellLevel", -1 };
//static EffectRef fx_opcode_bounce_ref = { "Bounce:Opcode", -1 };
static EffectRef fx_spell_bounce_ref = { "Bounce:Spell", -1 };
static EffectRef fx_school_bounce_ref = { "Bounce:School", -1 };
static EffectRef fx_secondary_type_bounce_ref = { "Bounce:SecondaryType", -1 };

//decrementing bounce effects
static EffectRef fx_level_bounce_dec_ref = { "Bounce:SpellLevelDec", -1 };
static EffectRef fx_spell_bounce_dec_ref = { "Bounce:SpellDec", -1 };
static EffectRef fx_school_bounce_dec_ref = { "Bounce:SchoolDec", -1 };
static EffectRef fx_secondary_type_bounce_dec_ref = { "Bounce:SecondaryTypeDec", -1 };

//spelltrap (multiple decrementing immunity)
static EffectRef fx_spelltrap = { "SpellTrap", -1 };

//weapon immunity
static EffectRef fx_weapon_immunity_ref = { "Protection:Weapons", -1 };

bool EffectQueue::match_ids(Actor *target, int table, ieDword value)
{
	if( value == 0) {
		return true;
	}

	int a, stat;

	switch (table) {
	case 0:
		stat = IE_FACTION; break;
	case 1:
		stat = IE_TEAM; break;
	case 2: //EA
		stat = IE_EA; break;
	case 3: //GENERAL
		//this is a hack to support dead only projectiles in PST
		//if it interferes with something feel free to remove
		if (value==GEN_DEAD) {
			if (target->GetStat(IE_STATE_ID)&STATE_DEAD) {
				return true;
			}
		}
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
		if( a) {
			if( a != ( stat & 15 )) {
				return false;
			}
		}
		a = value & 0xf0;
		if( a) {
			if( a != ( stat & 0xf0 )) {
				return false;
			}
		}
		return true;
	case 9:
		stat = target->GetClassMask();
		if (value&stat) return true;
		return false;
	default:
		return false;
	}
	if( target->GetStat(stat)==value) {
		return true;
	}
	return false;
}

static const bool fx_instant[MAX_TIMING_MODE]={true,true,true,false,false,false,false,false,true,true,true};

static inline bool IsInstant(ieByte timingmode)
{
	if( timingmode>=MAX_TIMING_MODE) return false;
	return fx_instant[timingmode];
}

static const bool fx_equipped[MAX_TIMING_MODE]={false,false,true,false,false,true,false,false,true,false,false};

static inline bool IsEquipped(ieByte timingmode)
{
	if( timingmode>=MAX_TIMING_MODE) return false;
	return fx_equipped[timingmode];
}

//                                               0    1     2     3    4    5    6     7       8   9     10
static const bool fx_relative[MAX_TIMING_MODE]={true,false,false,true,true,true,false,false,false,false,false};

static inline bool NeedPrepare(ieWord timingmode)
{
	if( timingmode>=MAX_TIMING_MODE) return false;
	return fx_relative[timingmode];
}

#define INVALID  -1
#define PERMANENT 0
#define DELAYED   1
#define DURATION  2

static const int fx_prepared[MAX_TIMING_MODE]={DURATION,PERMANENT,PERMANENT,DELAYED, //0-3
DELAYED,DELAYED,DELAYED,DELAYED,PERMANENT,PERMANENT,PERMANENT};		//4-7

static inline int DelayType(ieByte timingmode)
{
	if( timingmode>=MAX_TIMING_MODE) return INVALID;
	return fx_prepared[timingmode];
}

//which effects are removable
static const bool fx_removable[MAX_TIMING_MODE]={true,true,false,true,true,false,true,true,false,false,true};

static inline int IsRemovable(ieByte timingmode)
{
	if( timingmode>=MAX_TIMING_MODE) return INVALID;
	return fx_removable[timingmode];
}

//change the timing method after the effect triggered
static const ieByte fx_triggered[MAX_TIMING_MODE]={FX_DURATION_JUST_EXPIRED,FX_DURATION_INSTANT_PERMANENT,//0,1
FX_DURATION_INSTANT_WHILE_EQUIPPED,FX_DURATION_DELAY_LIMITED_PENDING,//2,3
FX_DURATION_AFTER_EXPIRES,FX_DURATION_PERMANENT_UNSAVED, //4,5
FX_DURATION_INSTANT_LIMITED,FX_DURATION_JUST_EXPIRED,FX_DURATION_PERMANENT_UNSAVED,//6,8
FX_DURATION_INSTANT_PERMANENT_AFTER_BONUSES,FX_DURATION_JUST_EXPIRED};//9,10

static inline ieByte TriggeredEffect(ieByte timingmode)
{
	if( timingmode>=MAX_TIMING_MODE) return false;
	return fx_triggered[timingmode];
}

static int compare_effects(const void *a, const void *b)
{
	return stricmp(((EffectRef *) a)->Name,((EffectRef *) b)->Name);
}

static int find_effect(const void *a, const void *b)
{
	return stricmp((const char *) a,((const EffectRef *) b)->Name);
}

static EffectDesc* FindEffect(const char* effectname)
{
	if( !effectname || !effectnames) {
		return NULL;
	}
	void *tmp = bsearch(effectname, effectnames, effectnames_count, sizeof(EffectDesc), find_effect);
	if( !tmp) {
		Log(WARNING, "EffectQueue", "Couldn't assign effect: %s", effectname);
	}
	return (EffectDesc *) tmp;
}

static inline void ResolveEffectRef(EffectRef &effect_reference)
{
	if( effect_reference.opcode==-1) {
		EffectDesc* ref = FindEffect(effect_reference.Name);
		if( ref && ref->opcode>=0) {
			effect_reference.opcode = ref->opcode;
			return;
		}
		effect_reference.opcode = -2;
	}
}

bool Init_EffectQueue()
{
	int i;

	if( initialized) {
		return true;
	}
	pstflags = !!core->HasFeature(GF_PST_STATE_FLAGS);
	iwd2fx = !!core->HasFeature(GF_ENHANCED_EFFECTS);

	memset( Opcodes, 0, sizeof( Opcodes ) );
	for(i=0;i<MAX_EFFECTS;i++) {
		Opcodes[i].Strref=-1;
	}

	initialized = 1;

	AutoTable efftextTable("efftext");

	int eT = core->LoadSymbol( "effects" );
	if( eT < 0) {
		Log(ERROR, "EffectQueue", "A critical scripting file is missing!");
		return false;
	}
	Holder<SymbolMgr> effectsTable = core->GetSymbol( eT );
	if( !effectsTable) {
		Log(ERROR, "EffectQueue", "A critical scripting file is damaged!");
		return false;
	}

	for (i = 0; i < MAX_EFFECTS; i++) {
		const char* effectname = effectsTable->GetValue( i );
		if( efftextTable) {
			int row = efftextTable->GetRowCount();
			while (row--) {
				const char* ret = efftextTable->GetRowName( row );
				long val;
				if( valid_number( ret, val ) && (i == val) ) {
					Opcodes[i].Strref = atoi( efftextTable->QueryField( row, 1 ) );
				}
			}
		}

		EffectDesc* poi = FindEffect( effectname );
		if( poi != NULL) {
			Opcodes[i].Function = poi->Function;
			Opcodes[i].Name = poi->Name;
			Opcodes[i].Flags = poi->Flags;
			//reverse linking opcode number
			//using this unused field
			if( (poi->opcode!=-1) && effectname[0]!='*') {
				error("EffectQueue", "Clashing Opcodes FN: %d vs. %d, %s\n", i, poi->opcode, effectname);
			}
			poi->opcode = i;
		}
		//print("-------- FN: %d, %s", i, effectname);
	}
	core->DelSymbol( eT );

	return true;
}

void EffectQueue_ReleaseMemory()
{
	if( effectnames) {
		free (effectnames);
	}
	effectnames_count = 0;
	effectnames = NULL;
}

void EffectQueue_RegisterOpcodes(int count, const EffectDesc* opcodes)
{
	if( ! effectnames) {
		effectnames = (EffectDesc*) malloc( (count+1) * sizeof( EffectDesc ) );
	} else {
		effectnames = (EffectDesc*) realloc( effectnames, (effectnames_count + count + 1) * sizeof( EffectDesc ) );
	}

	memcpy( effectnames + effectnames_count, opcodes, count * sizeof( EffectDesc ));
	effectnames_count += count;
	effectnames[effectnames_count].Name = NULL;
	//if we merge two effect lists, then we need to sort their effect tables
	//actually, we might always want to sort this list, so there is no
	//need to do it manually (sorted table is needed if we use bsearch)
	qsort(effectnames, effectnames_count, sizeof(EffectDesc), compare_effects);
}

EffectQueue::EffectQueue()
{
	Owner = NULL;
}

EffectQueue::~EffectQueue()
{
	std::list< Effect* >::iterator f;

	for ( f = effects.begin(); f != effects.end(); f++ ) {
		delete (*f);
	}
}

Effect *EffectQueue::CreateEffect(ieDword opcode, ieDword param1, ieDword param2, ieWord timing)
{
	if( opcode==0xffffffff) {
		return NULL;
	}
	Effect *fx = new Effect();
	if( !fx) {
		return NULL;
	}
	memset(fx,0,sizeof(Effect));
	fx->Target = FX_TARGET_SELF;
	fx->Opcode = opcode;
	//probability2 is the low number (by effectqueue 331)
	fx->Probability1 = 100;
	fx->Parameter1 = param1;
	fx->Parameter2 = param2;
	fx->TimingMode = timing;
	fx->PosX = 0xffffffff;
	fx->PosY = 0xffffffff;
	return fx;
}

//return the count of effects with matching parameters
//useful for effects where there is no separate stat to see
ieDword EffectQueue::CountEffects(EffectRef &effect_reference, ieDword param1, ieDword param2, const char *resource) const
{
	ResolveEffectRef(effect_reference);
	if( effect_reference.opcode<0) {
		return 0;
	}
	return CountEffects(effect_reference.opcode, param1, param2, resource);
}

//Change the location of an existing effect
//this is used when some external code needs to adjust the effect's location
//used when the gui sets the effect's final target
void EffectQueue::ModifyEffectPoint(EffectRef &effect_reference, ieDword x, ieDword y) const
{
	ResolveEffectRef(effect_reference);
	if( effect_reference.opcode<0) {
		return;
	}
	ModifyEffectPoint(effect_reference.opcode, x, y);
}

Effect *EffectQueue::CreateEffect(EffectRef &effect_reference, ieDword param1, ieDword param2, ieWord timing)
{
	ResolveEffectRef(effect_reference);
	if( effect_reference.opcode<0) {
		return NULL;
	}
	return CreateEffect(effect_reference.opcode, param1, param2, timing);
}

//copies the whole effectqueue (area projectiles use it)
EffectQueue *EffectQueue::CopySelf() const
{
	EffectQueue *effects;

	effects = new EffectQueue();
	std::list< Effect* >::const_iterator fxit = GetFirstEffect();
	Effect *fx;

	while( (fx = GetNextEffect(fxit))) {
		effects->AddEffect(fx, false);
	}
	effects->SetOwner(GetOwner());
	return effects;
}

//create a new effect with most of the characteristics of the old effect
//only opcode and parameters are changed
//This is used mostly inside effects, when an effect needs to spawn
//other effects with the same coordinates, source, duration, etc.
Effect *EffectQueue::CreateEffectCopy(Effect *oldfx, ieDword opcode, ieDword param1, ieDword param2)
{
	if( opcode==0xffffffff) {
		return NULL;
	}
	Effect *fx = new Effect();
	if( !fx) {
		return NULL;
	}
	memcpy(fx,oldfx,sizeof(Effect) );
	fx->Opcode=opcode;
	fx->Parameter1=param1;
	fx->Parameter2=param2;
	return fx;
}

Effect *EffectQueue::CreateEffectCopy(Effect *oldfx, EffectRef &effect_reference, ieDword param1, ieDword param2)
{
	ResolveEffectRef(effect_reference);
	if( effect_reference.opcode<0) {
		return NULL;
	}
	return CreateEffectCopy(oldfx, effect_reference.opcode, param1, param2);
}

Effect *EffectQueue::CreateUnsummonEffect(Effect *fx)
{
	Effect *newfx = NULL;
	if( (fx->TimingMode&0xff) == FX_DURATION_INSTANT_LIMITED) {
		newfx = CreateEffectCopy(fx, fx_unsummon_creature_ref, 0, 0);
		newfx->TimingMode = FX_DURATION_DELAY_PERMANENT;
		if( newfx->Resource3[0]) {
			strnuprcpy(newfx->Resource,newfx->Resource3, sizeof(ieResRef)-1 );
		} else {
			strnuprcpy(newfx->Resource,"SPGFLSH1", sizeof(ieResRef)-1 );
		}
		if( fx->TimingMode == FX_DURATION_ABSOLUTE) {
			//unprepare duration
			newfx->Duration = (newfx->Duration-core->GetGame()->GameTime)/AI_UPDATE_TIME;
		}
	}

	return newfx;
}

void EffectQueue::AddEffect(Effect* fx, bool insert)
{
	Effect* new_fx = new Effect;
	memcpy( new_fx, fx, sizeof( Effect ) );
	if( insert) {
		effects.insert( effects.begin(), new_fx );
	} else {
		effects.push_back( new_fx );
	}
}

//This method can remove an effect described by a pointer to it, or
//an exact matching effect
bool EffectQueue::RemoveEffect(Effect* fx)
{
	int invariant_size = offsetof( Effect, random_value );

	for (std::list< Effect* >::iterator f = effects.begin(); f != effects.end(); f++ ) {
		Effect* fx2 = *f;

		if( (fx==fx2) || !memcmp( fx, fx2, invariant_size)) {
			delete fx2;
			effects.erase( f );
			return true;
		}
	}
	return false;
}

//this is where we reapply all effects when loading a saved game
//The effects are already in the fxqueue of the target
void EffectQueue::ApplyAllEffects(Actor* target) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		ApplyEffect( target, *f, 0 );
	}
}

void EffectQueue::Cleanup()
{
	std::list< Effect* >::iterator f;

	for ( f = effects.begin(); f != effects.end(); ) {
		if( (*f)->TimingMode == FX_DURATION_JUST_EXPIRED) {
			delete *f;
			effects.erase(f++);
		} else {
			f++;
		}
	}
}

//Handle the target flag when the effect is applied first
int EffectQueue::AddEffect(Effect* fx, Scriptable* self, Actor* pretarget, const Point &dest) const
{
	int i;
	Game *game;
	Map *map;
	int flg;
	ieDword spec = 0;
	Actor *st = (self && (self->Type==ST_ACTOR)) ?(Actor *) self:NULL;
	Effect* new_fx;

	if (self) {
		fx->CasterID = self->GetGlobalID();
	} else {
		if (Owner) fx->CasterID = Owner->GetGlobalID();
	}

	switch (fx->Target) {
	case FX_TARGET_ORIGINAL:
		fx->SetPosition(self->Pos);

		flg = ApplyEffect( st, fx, 1 );
		if( fx->TimingMode != FX_DURATION_JUST_EXPIRED) {
			if( st) {
				st->fxqueue.AddEffect( fx, flg==FX_INSERT );
			}
		}
		break;
	case FX_TARGET_SELF:
		fx->SetPosition(dest);

		flg = ApplyEffect( st, fx, 1 );
		if( fx->TimingMode != FX_DURATION_JUST_EXPIRED) {
			if( st) {
				st->fxqueue.AddEffect( fx, flg==FX_INSERT );
			}
		}
		break;

	case FX_TARGET_ALL_BUT_SELF:
		new_fx = new Effect;
		map=self->GetCurrentArea();
		i= map->GetActorCount(true);
		while(i--) {
			Actor* actor = map->GetActor( i, true );
			//don't pick ourselves
			if( st==actor) {
				continue;
			}
			memcpy( new_fx, fx, sizeof( Effect ) );
			new_fx->SetPosition(actor->Pos);

			flg = ApplyEffect( actor, new_fx, 1 );
			if( new_fx->TimingMode != FX_DURATION_JUST_EXPIRED) {
				actor->fxqueue.AddEffect( new_fx, flg==FX_INSERT );
			}
		}
		delete new_fx;
		flg = FX_APPLIED;
		break;

	case FX_TARGET_OWN_SIDE:
		if( !st || st->InParty) {
			goto all_party;
		}
		map = self->GetCurrentArea();
		spec = st->GetStat(IE_SPECIFIC);

		new_fx = new Effect;
		//GetActorCount(false) returns all nonparty critters
		i = map->GetActorCount(false);
		while(i--) {
			Actor* actor = map->GetActor( i, false );
			if( actor->GetStat(IE_SPECIFIC)!=spec) {
				continue;
			}
			memcpy( new_fx, fx, sizeof( Effect ) );
			new_fx->SetPosition(actor->Pos);

			flg = ApplyEffect( actor, new_fx, 1 );
			if( new_fx->TimingMode != FX_DURATION_JUST_EXPIRED) {
				actor->fxqueue.AddEffect( new_fx, flg==FX_INSERT );
			}
		}
		delete new_fx;
		flg = FX_APPLIED;
		break;
	case FX_TARGET_OTHER_SIDE:
		if( !pretarget || pretarget->InParty) {
			goto all_party;
		}
		map = self->GetCurrentArea();
		spec = pretarget->GetStat(IE_SPECIFIC);

		new_fx = new Effect;
		//GetActorCount(false) returns all nonparty critters
		i = map->GetActorCount(false);
		while(i--) {
			Actor* actor = map->GetActor( i, false );
			if( actor->GetStat(IE_SPECIFIC)!=spec) {
				continue;
			}
			memcpy( new_fx, fx, sizeof( Effect ) );
			new_fx->SetPosition(actor->Pos);

			flg = ApplyEffect( actor, new_fx, 1 );
			//GetActorCount can now return all nonparty critters
			if( new_fx->TimingMode != FX_DURATION_JUST_EXPIRED) {
				actor->fxqueue.AddEffect( new_fx, flg==FX_INSERT );
			}
		}
		delete new_fx;
		flg = FX_APPLIED;
		break;
	case FX_TARGET_PRESET:
		//fx->SetPosition(pretarget->Pos);
		//knock needs this
		fx->SetPosition(dest);

		flg = ApplyEffect( pretarget, fx, 1 );
		if( fx->TimingMode != FX_DURATION_JUST_EXPIRED) {
			if( pretarget) {
				pretarget->fxqueue.AddEffect( fx, flg==FX_INSERT );
			}
		}
		break;

	case FX_TARGET_PARTY:
all_party:
		new_fx = new Effect;
		game = core->GetGame();
		i = game->GetPartySize(false);
		while(i--) {
			Actor* actor = game->GetPC( i, false );
			memcpy( new_fx, fx, sizeof( Effect ) );
			new_fx->SetPosition(actor->Pos);

			flg = ApplyEffect( actor, new_fx, 1 );
			if( new_fx->TimingMode != FX_DURATION_JUST_EXPIRED) {
				actor->fxqueue.AddEffect( new_fx, flg==FX_INSERT );
			}
		}
		delete new_fx;
		flg = FX_APPLIED;
		break;

	case FX_TARGET_ALL:
		new_fx = new Effect;
		map = self->GetCurrentArea();
		i = map->GetActorCount(true);
		while(i--) {
			Actor* actor = map->GetActor( i, true );
			memcpy( new_fx, fx, sizeof( Effect ) );
			new_fx->SetPosition(actor->Pos);

			flg = ApplyEffect( actor, new_fx, 1 );
			if( new_fx->TimingMode != FX_DURATION_JUST_EXPIRED) {
				actor->fxqueue.AddEffect( new_fx, flg==FX_INSERT );
			}
		}
		delete new_fx;
		flg = FX_APPLIED;
		break;

	case FX_TARGET_ALL_BUT_PARTY:
		new_fx = new Effect;
		map = self->GetCurrentArea();
		i = map->GetActorCount(false);
		while(i--) {
			Actor* actor = map->GetActor( i, false );
			memcpy( new_fx, fx, sizeof( Effect ) );
			new_fx->SetPosition(actor->Pos);

			flg = ApplyEffect( actor, new_fx, 1 );
			//GetActorCount can now return all nonparty critters
			if( new_fx->TimingMode != FX_DURATION_JUST_EXPIRED) {
				actor->fxqueue.AddEffect( new_fx, flg==FX_INSERT );
			}
		}
		delete new_fx;
		flg = FX_APPLIED;
		break;

	case FX_TARGET_UNKNOWN:
	default:
		Log(MESSAGE, "EffectQueue", "Unknown FX target type: %d", fx->Target);
		flg = FX_ABORT;
		break;
	}

	return flg;
}

//this is where effects from spells first get in touch with the target
//the effects are currently NOT in the target's fxqueue, those that stick
//will get copied (hence the fxqueue.AddEffect call)
//if this returns FX_NOT_APPLIED, then the whole stack was resisted
//or expired
int EffectQueue::AddAllEffects(Actor* target, const Point &destination) const
{
	int res = FX_NOT_APPLIED;
	// pre-roll dice for fx needing them and stow them in the effect
	ieDword random_value = core->Roll( 1, 100, -1 );

	if( target) {
		target->RollSaves();
	}
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		//handle resistances and saving throws here
		(*f)->random_value = random_value;
		//if applyeffect returns true, we stop adding the future effects
		//this is to simulate iwd2's on the fly spell resistance

		int tmp = AddEffect(*f, Owner, target, destination);
		//lets try without Owner, any crash?
		//If yes, then try to fix the individual effect
		//If you use target for Owner here, the wand in chateau irenicus will work
		//the same way as Imoen's monster summoning, which is a BAD THING (TM)
		//int tmp = AddEffect(*f, Owner?Owner:target, target, destination);
		if( tmp == FX_ABORT) {
			res = FX_NOT_APPLIED;
			break;
		}
		if( tmp != FX_NOT_APPLIED) {
			res = FX_APPLIED;
		}
	}
	return res;
}

//resisted effect based on level
static inline bool check_level(Actor *target, Effect *fx)
{
	//skip non level based effects
	//check if an effect has no level based resistance, but instead the dice sizes/count
	//adjusts Parameter1 (like a damage causing effect)
	if( Opcodes[fx->Opcode].Flags & EFFECT_DICED ) {
		//add the caster level to the dice count
		if (fx->IsVariable) {
			fx->DiceThrown+=fx->CasterLevel;
		}
		fx->Parameter1 = DICE_ROLL((signed)fx->Parameter1);
		//this is a hack for PST style diced effects
		if( core->HasFeature(GF_SAVE_FOR_HALF) ) {
			if( memcmp(fx->Resource,"NEG",4) ) {
				fx->IsSaveForHalfDamage=1;
			}
		} else {
			if( (fx->Parameter2&3)==3) {
				fx->IsSaveForHalfDamage=1;
			}
		}
		return false;
	}
	//there is no level based resistance, but Parameter1 cannot be precalculated
	//these effects use the Dice fields in a special way
	if( Opcodes[fx->Opcode].Flags & EFFECT_NO_LEVEL_CHECK ) {
		return false;
	}

	if( !target) {
		return false;
	}
	if(fx->Target == FX_TARGET_SELF) {
		return false;
	}

	ieDword level = (ieDword) target->GetXPLevel( true );
	//return true if resisted
	if ((signed)fx->MinAffectedLevel > 0) {
		if ((signed)level < (signed)fx->MinAffectedLevel) {
			return true;
		}
	}

	if ((signed)fx->MaxAffectedLevel > 0) {
		if ((signed)level > (signed)fx->MaxAffectedLevel) {
			return true;
		}
	}
	return false;
}

//roll for the effect probability, there is a high and a low treshold, the d100
//roll should hit in the middle
static inline bool check_probability(Effect* fx)
{
	//watch for this, probability1 is the high number
	//probability2 is the low number
	//random value is 0-99
	if(fx->random_value<fx->Probability2 || fx->random_value>fx->Probability1) {
		return false;
	}
	return true;
}

//this is for whole spell immunity/bounce
static inline int DecreaseEffect(Effect *efx)
{
	if (efx->Parameter1) {
		efx->Parameter1--;
	return true;
	}
	return false;
}

//lower decreasing immunities/bounces
static int check_type(Actor* actor, Effect* fx)
{
	//the protective effect (if any)
	Effect *efx;

	ieDword bounce = actor->GetStat(IE_BOUNCE);

	//immunity checks
/*opcode immunity is in the per opcode checks
	if( actor->fxqueue.HasEffectWithParam(fx_opcode_immunity_ref, fx->Opcode) ) {
		return 0;
	}
	if( actor->fxqueue.HasEffectWithParam(fx_opcode_immunity2_ref, fx->Opcode) ) {
		return 0;
	}
*/
	//spell level immunity
	if(fx->Power && actor->fxqueue.HasEffectWithParamPair(fx_level_immunity_ref, 0, fx->Power) ) {
		return 0;
	}

	//source immunity (spell name)
	//if source is unspecified, don't resist it
	if( fx->Source[0]) {
		if( actor->fxqueue.HasEffectWithResource(fx_spell_immunity_ref, fx->Source) ) {
			return 0;
		}
		if( actor->fxqueue.HasEffectWithResource(fx_spell_immunity2_ref, fx->Source) ) {
			return 0;
		}
		if (actor->fxqueue.HasEffectWithResource(fx_store_spell_sequencer_ref, fx->Source) ) {
			return 0;
		}
	}

	//primary type immunity (school)
	if( fx->PrimaryType) {
		if( actor->fxqueue.HasEffectWithParam(fx_school_immunity_ref, fx->PrimaryType)) {
			return 0;
		}
	}

	//secondary type immunity (usage)
	if( fx->SecondaryType) {
		if( actor->fxqueue.HasEffectWithParam(fx_secondary_type_immunity_ref, fx->SecondaryType) ) {
			return 0;
		}
	}

	//decrementing immunity checks
	//decrementing level immunity
	if (fx->Power) {
		efx = actor->fxqueue.HasEffectWithParamPair(fx_level_immunity_dec_ref, 0, fx->Power);
		if( efx ) {
			if (DecreaseEffect(efx))
				return 0;
		}
	}

	//decrementing spell immunity
	if( fx->Source[0]) {
		efx = actor->fxqueue.HasEffectWithResource(fx_spell_immunity_dec_ref, fx->Source);
		if( efx) {
			if (DecreaseEffect(efx))
				return 0;
		}
	}
	//decrementing primary type immunity (school)
	if( fx->PrimaryType) {
		efx = actor->fxqueue.HasEffectWithParam(fx_school_immunity_dec_ref, fx->PrimaryType);
		if( efx) {
			if (DecreaseEffect(efx))
				return 0;
		}
	}

	//decrementing secondary type immunity (usage)
	if( fx->SecondaryType) {
		efx = actor->fxqueue.HasEffectWithParam(fx_secondary_type_immunity_dec_ref, fx->SecondaryType);
		if( efx) {
			if (DecreaseEffect(efx))
				return 0;
		}
	}

	//spelltrap (absorb)
	//FIXME:
	//if the spelltrap effect already absorbed enough levels
	//but still didn't get removed, it will absorb levels it shouldn't
	//it will also absorb multiple spells in a single round
	if (fx->Power) {
		efx=actor->fxqueue.HasEffectWithParamPair(fx_spelltrap, 0, fx->Power);
		if( efx) {
			//storing the absorbed spell level
			efx->Parameter3+=fx->Power;
			//instead of a single effect, they had to create an effect for each level
			//HOW DAMN LAME
			//if decrease needs the spell level, use fx->Power here
			actor->fxqueue.DecreaseParam1OfEffect(fx_spelltrap, 1);
			//efx->Parameter1--;
			return 0;
		}
	}

	//bounce checks
	if (fx->Power) {
		if( (bounce&BNC_LEVEL) && actor->fxqueue.HasEffectWithParamPair(fx_level_bounce_ref, 0, fx->Power) ) {
			return 0;
		}
	}

	if( fx->Source[0] && (bounce&BNC_RESOURCE) && actor->fxqueue.HasEffectWithResource(fx_spell_bounce_ref, fx->Source) ) {
		return -1;
	}

	if( fx->PrimaryType && (bounce&BNC_SCHOOL) ) {
		if( actor->fxqueue.HasEffectWithParam(fx_school_bounce_ref, fx->PrimaryType)) {
			return -1;
		}
	}

	if( fx->SecondaryType && (bounce&BNC_SECTYPE) ) {
		if( actor->fxqueue.HasEffectWithParam(fx_secondary_type_bounce_ref, fx->SecondaryType)) {
			return -1;
		}
	}
	//decrementing bounce checks

	//level decrementing bounce check
	if (fx->Power) {
		if( (bounce&BNC_LEVEL_DEC)) {
			efx=actor->fxqueue.HasEffectWithParamPair(fx_level_bounce_dec_ref, 0, fx->Power);
			if( efx) {
				if (DecreaseEffect(efx))
					return -1;
			}
		}
	}

	if( fx->Source[0] && (bounce&BNC_RESOURCE_DEC)) {
		efx=actor->fxqueue.HasEffectWithResource(fx_spell_bounce_dec_ref, fx->Resource);
		if( efx) {
			if (DecreaseEffect(efx))
				return -1;
		}
	}

	if( fx->PrimaryType && (bounce&BNC_SCHOOL_DEC) ) {
		efx=actor->fxqueue.HasEffectWithParam(fx_school_bounce_dec_ref, fx->PrimaryType);
		if( efx) {
			if (DecreaseEffect(efx))
				return -1;
		}
	}

	if( fx->SecondaryType && (bounce&BNC_SECTYPE_DEC) ) {
		efx=actor->fxqueue.HasEffectWithParam(fx_secondary_type_bounce_dec_ref, fx->SecondaryType);
		if( efx) {
			if (DecreaseEffect(efx))
				return -1;
		}
	}

	return 1;
}

//check resistances, saving throws
static bool check_resistance(Actor* actor, Effect* fx)
{
	Actor *caster;
	Scriptable *cob;

	if( !actor) {
		return false;
	}

	cob = GetCasterObject();
	if (cob && cob->Type==ST_ACTOR) {
		caster = (Actor *) cob;
	} else {
		caster = 0;
	}

	//opcode immunity
	if( actor->fxqueue.HasEffectWithParam(fx_opcode_immunity_ref, fx->Opcode) ) {
		Log(MESSAGE, "EffectQueue", "immune to effect: %s", (char*) Opcodes[fx->Opcode].Name);
		return true;
	}
	if( actor->fxqueue.HasEffectWithParam(fx_opcode_immunity2_ref, fx->Opcode) ) {
		Log(MESSAGE, "EffectQueue", "immune2 to effect: %s", (char*) Opcodes[fx->Opcode].Name);
		return true;
	}

/* opcode bouncing isn't implemented?
	//opcode bouncing
	if( actor->fxqueue.HasEffectWithParam(fx_opcode_bounce_ref, fx->Opcode) ) {
		return false;
	}
*/

	//not resistable (no saves either?)
	if(fx->Resistance != FX_CAN_RESIST_CAN_DISPEL) {
		return false;
	}

	if (pstflags && (actor->GetSafeStat(IE_STATE_ID) & (STATE_ANTIMAGIC) ) ) {
		return false;
	}

	//don't resist self
	bool selective_mr = core->HasFeature(GF_SELECTIVE_MAGIC_RES);
	if (fx->CasterID == actor->GetGlobalID()) {
		if (selective_mr) {
			return false;
		}
	}

	//magic immunity
	ieDword val = actor->GetStat(IE_RESISTMAGIC);
	bool resisted = false;

	if (iwd2fx) {
		// 3ed style check
		// TODO: check if luck really affects it (i doubt it does)
		ieDword check = fx->CasterLevel + actor->LuckyRoll(1, 20, 0);
		// +2/+4 level bonus from the (greater) spell penetration feat
		if (caster && caster->HasFeat(FEAT_SPELL_PENETRATION)) {
			check += 2 * caster->GetStat(IE_FEAT_SPELL_PENETRATION);
		}
		resisted = (signed) check < (signed) val;
	} else {
		// 2.5 style check
		resisted = (signed) fx->random_value < (signed) val;
	}
	if (resisted) {
		// we take care of irresistible spells a few checks above, so selective mr has no impact here anymore
		displaymsg->DisplayConstantStringName(STR_MAGIC_RESISTED, DMC_WHITE, actor);
		Log(MESSAGE, "EffectQueue", "effect resisted: %s", (char*) Opcodes[fx->Opcode].Name);
		return true;
	}

	//saving throws, bonus can be improved by school specific bonus
	int bonus = fx->SavingThrowBonus + actor->fxqueue.BonusForParam2(fx_spell_resistance_ref, fx->PrimaryType);
	if (caster) {
		bonus += actor->fxqueue.BonusAgainstCreature(fx_ac_vs_creature_type_ref,caster);
		//saving throw could be made difficult by caster's school specific bonus
		bonus -= caster->fxqueue.BonusForParam2(fx_spell_focus_ref, fx->PrimaryType);
	}

	bool saved = false;
	for (int i=0;i<5;i++) {
		if( fx->SavingThrowType&(1<<i)) {
			saved = actor->GetSavingThrow(i, bonus);
			if( saved) {
				break;
			}
		}
	}
	if( saved) {
		if( fx->IsSaveForHalfDamage) {
			fx->Parameter1/=2;
		} else {
			Log(MESSAGE, "EffectQueue", "%s saved against effect: %s", actor->GetName(1), (char*) Opcodes[fx->Opcode].Name);
			return true;
		}
	}
	return false;
}

// this function is called two different ways
// when FirstApply is set, then the effect isn't stuck on the target
// this happens when a new effect comes in contact with the target.
// if the effect returns FX_DURATION_JUST_EXPIRED then it won't stick
// when first_apply is unset, the effect is already on the target
// this happens on load time too!
// returns FX_NOT_APPLIED if the process shouldn't be calling applyeffect anymore
// returns FX_ABORT if the whole spell this effect is in should be aborted
// it will disable all future effects of same source (only on first apply)

int EffectQueue::ApplyEffect(Actor* target, Effect* fx, ieDword first_apply, ieDword resistance) const
{
	//print("FX 0x%02x: %s(%d, %d)", fx->Opcode, effectnames[fx->Opcode].Name, fx->Parameter1, fx->Parameter2);
	if( fx->Opcode >= MAX_EFFECTS) {
		fx->TimingMode = FX_DURATION_JUST_EXPIRED;
		return FX_NOT_APPLIED;
	}

	ieDword GameTime = core->GetGame()->GameTime;

	if (first_apply) {
		fx->FirstApply = 1;
		if( (fx->PosX==0xffffffff) && (fx->PosY==0xffffffff)) {
			fx->PosX = target->Pos.x;
			fx->PosY = target->Pos.y;
		}

		//gemrb specific, stat based chance
		if ((fx->Probability2 == 100) && Owner && (Owner->Type==ST_ACTOR) ) {
			fx->Probability2 = 0;
			fx->Probability1 = ((Actor *) Owner)->GetSafeStat(fx->Probability1);
		}

		if (resistance) {
			//the effect didn't pass the probability check
			if( !check_probability(fx) ) {
				fx->TimingMode = FX_DURATION_JUST_EXPIRED;
				return FX_NOT_APPLIED;
			}

			//the effect didn't pass the target level check
			if( check_level(target, fx) ) {
				fx->TimingMode = FX_DURATION_JUST_EXPIRED;
				return FX_NOT_APPLIED;
			}

			//the effect didn't pass the resistance check
			if( check_resistance(target, fx) ) {
				fx->TimingMode = FX_DURATION_JUST_EXPIRED;
				return FX_NOT_APPLIED;
			}
		}

		//Same as in items and spells
		if (fx->SourceFlags & SF_HOSTILE) {
			if (target && (target != Owner) && Owner && (Owner->Type==ST_ACTOR) ) {
				target->AttackedBy((Actor *) Owner);
			}
		}

		if( NeedPrepare(fx->TimingMode) ) {
			//save delay for later
			fx->SecondaryDelay = fx->Duration;
			if( fx->TimingMode == FX_DURATION_INSTANT_LIMITED) {
				fx->TimingMode = FX_DURATION_ABSOLUTE;
			}
			PrepareDuration(fx);
		}
	}
	//check if the effect has triggered or expired
	switch (DelayType(fx->TimingMode&0xff) ) {
	case DELAYED:
		if( fx->Duration>GameTime) {
			return FX_NOT_APPLIED;
		}
		//effect triggered
		//delayed duration (3)
		if( NeedPrepare(fx->TimingMode) ) {
			//prepare for delayed duration effects
			fx->Duration = fx->SecondaryDelay;
			PrepareDuration(fx);
		}
		fx->TimingMode=TriggeredEffect(fx->TimingMode);
		break;
	case DURATION:
		if( fx->Duration<=GameTime) {
			fx->TimingMode = FX_DURATION_JUST_EXPIRED;
			//add a return here, if 0 duration effects shouldn't work
		}
		break;
	//permanent effect (so there is no warning)
	case PERMANENT:
		break;
	//this shouldn't happen
	default:
		error("EffectQueue", "Unknown delay type: %d (from %d)\n", DelayType(fx->TimingMode&0xff), fx->TimingMode);
	}

	EffectFunction fn = 0;
	if( fx->Opcode<MAX_EFFECTS) {
		fn = Opcodes[fx->Opcode].Function;
		if (!(target || (Opcodes[fx->Opcode].Flags & EFFECT_NO_ACTOR))) {
			Log(MESSAGE, "EffectQueue", "targetless opcode without EFFECT_NO_ACTOR: %d, skipping", fx->Opcode);
			return FX_NOT_APPLIED;
		}
	}
	int res = FX_ABORT;
	if( fn) {
		if( target && fx->FirstApply ) {
			if( !target->fxqueue.HasEffectWithParamPair(fx_protection_from_display_string_ref, fx->Parameter1, 0) ) {
				displaymsg->DisplayStringName( Opcodes[fx->Opcode].Strref, DMC_WHITE,
					target, IE_STR_SOUND);
			}
		}

		res=fn( Owner, target, fx );
		fx->FirstApply = 0;

		//if there is no owner, we assume it is the target
		switch( res ) {
			case FX_APPLIED:
				//normal effect with duration
				break;
			case FX_NOT_APPLIED:
				//instant effect, pending removal
				//for example, a damage effect
				fx->TimingMode = FX_DURATION_JUST_EXPIRED;
				break;
			case FX_INSERT:
				//put this effect in the beginning of the queue
				//all known insert effects are 'permanent' too
				//that is the AC effect only
				//actually, permanent effects seem to be
				//inserted by the game engine too
			case FX_PERMANENT:
				//don't stick around if it was executed permanently
				//for example, a permanent strength modifier effect
				if( fx->TimingMode == FX_DURATION_INSTANT_PERMANENT ) {
					fx->TimingMode = FX_DURATION_JUST_EXPIRED;
				}
				break;
			case FX_ABORT:
				break;
			default:
				error("EffectQueue", "Unknown effect result '%x', aborting ...\n", res);
		}
	} else {
		//effect not found, it is going to be discarded
		fx->TimingMode = FX_DURATION_JUST_EXPIRED;
	}
	return res;
}

// looks for opcode with param2

#define MATCH_OPCODE() if((*f)->Opcode!=opcode) { continue; }

// useful for: remove equipped item
#define MATCH_SLOTCODE() if((*f)->InventorySlot!=slotcode) { continue; }

// useful for: remove projectile type
#define MATCH_PROJECTILE() if((*f)->Projectile!=projectile) { continue; }

static const bool fx_live[MAX_TIMING_MODE]={true,true,true,false,false,false,false,false,true,true,false};
static inline bool IsLive(ieByte timingmode)
{
	if( timingmode>=MAX_TIMING_MODE) return false;
	return fx_live[timingmode];
}

#define MATCH_LIVE_FX() if(!IsLive((*f)->TimingMode)) { continue; }
#define MATCH_PARAM1() if((*f)->Parameter1!=param1) { continue; }
#define MATCH_PARAM2() if((*f)->Parameter2!=param2) { continue; }
#define MATCH_RESOURCE() if( strnicmp( (*f)->Resource, resource, 8) ) { continue; }
#define MATCH_SOURCE() if( strnicmp( (*f)->Source, Removed, 8) ) { continue; }
#define MATCH_TIMING() if( (*f)->TimingMode!=timing) { continue; }

//call this from an applied effect, after it returns, these effects
//will be killed along with it
void EffectQueue::RemoveAllEffects(ieDword opcode) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();

		(*f)->TimingMode = FX_DURATION_JUST_EXPIRED;
	}
}

//removes all equipping effects that match slotcode
void EffectQueue::RemoveEquippingEffects(ieDwordSigned slotcode) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		if( !IsEquipped((*f)->TimingMode)) continue;
		MATCH_SLOTCODE();

		(*f)->TimingMode = FX_DURATION_JUST_EXPIRED;
	}
}

//removes all effects that match projectile
void EffectQueue::RemoveAllEffectsWithProjectile(ieDword projectile) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_PROJECTILE();

		(*f)->TimingMode = FX_DURATION_JUST_EXPIRED;
	}
}

//remove effects belonging to a given spell
void EffectQueue::RemoveAllEffects(const ieResRef Removed) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_LIVE_FX();
		MATCH_SOURCE();

		(*f)->TimingMode = FX_DURATION_JUST_EXPIRED;
	}
}

//remove effects belonging to a given spell, but only if they match timing method x
void EffectQueue::RemoveAllEffects(const ieResRef Removed, ieByte timing) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_TIMING();
		MATCH_SOURCE();

		(*f)->TimingMode = FX_DURATION_JUST_EXPIRED;
	}
}

//this will modify effect reference
void EffectQueue::RemoveAllEffects(EffectRef &effect_reference) const
{
	ResolveEffectRef(effect_reference);
	if( effect_reference.opcode<0) {
		return;
	}
	RemoveAllEffects(effect_reference.opcode);
}

//Removes all effects with a matching resource field
void EffectQueue::RemoveAllEffectsWithResource(ieDword opcode, const ieResRef resource) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();
		MATCH_RESOURCE();

		(*f)->TimingMode = FX_DURATION_JUST_EXPIRED;
	}
}

//this will modify effect reference
void EffectQueue::RemoveAllEffectsWithResource(EffectRef &effect_reference, const ieResRef resource) const
{
	ResolveEffectRef(effect_reference);
	RemoveAllEffectsWithResource(effect_reference.opcode, resource);
}

//This method could be used to remove stat modifiers that would lower a stat
//(works only if a higher stat means good for the target)
void EffectQueue::RemoveAllDetrimentalEffects(ieDword opcode, ieDword current) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();
		switch((*f)->Parameter2) {
		case 0:case 3:
			if( ((signed) (*f)->Parameter1)>=0) continue;
			break;
		case 1:case 4:
			if( ((signed) (*f)->Parameter1)>=(signed) current) continue;
			break;
		case 2:case 5:
			if( ((signed) (*f)->Parameter1)>=100) continue;
			break;
		default:
			break;
		}
		(*f)->TimingMode = FX_DURATION_JUST_EXPIRED;
	}
}

//this will modify effect reference
void EffectQueue::RemoveAllDetrimentalEffects(EffectRef &effect_reference, ieDword current) const
{
	ResolveEffectRef(effect_reference);
	RemoveAllDetrimentalEffects(effect_reference.opcode, current);
}

//Removes all effects with a matching param2
//param2 is usually an effect's subclass (quality) while param1 is more like quantity.
//So opcode+param2 usually pinpoints an effect better when not all effects of a given
//opcode need to be removed (see removal of portrait icon)
void EffectQueue::RemoveAllEffectsWithParam(ieDword opcode, ieDword param2) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();
		MATCH_PARAM2();

		(*f)->TimingMode = FX_DURATION_JUST_EXPIRED;
	}
}

//this will modify effect reference
void EffectQueue::RemoveAllEffectsWithParam(EffectRef &effect_reference, ieDword param2) const
{
	ResolveEffectRef(effect_reference);
	RemoveAllEffectsWithParam(effect_reference.opcode, param2);
}

//Removes all effects with a matching resource field
void EffectQueue::RemoveAllEffectsWithParamAndResource(ieDword opcode, ieDword param2, const ieResRef resource) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();
		MATCH_PARAM2();
		if(resource[0]) {
			MATCH_RESOURCE();
		}

		(*f)->TimingMode = FX_DURATION_JUST_EXPIRED;
	}
}

//this will modify effect reference
void EffectQueue::RemoveAllEffectsWithParamAndResource(EffectRef &effect_reference, ieDword param2, const ieResRef resource) const
{
	ResolveEffectRef(effect_reference);
	RemoveAllEffectsWithParamAndResource(effect_reference.opcode, param2, resource);
}

//this function is called by FakeEffectExpiryCheck
//probably also called by rest
void EffectQueue::RemoveExpiredEffects(ieDword futuretime) const
{
	ieDword GameTime = core->GetGame()->GameTime;
	if( GameTime+futuretime*AI_UPDATE_TIME<GameTime) {
		GameTime=0xffffffff;
	} else {
		GameTime+=futuretime*AI_UPDATE_TIME;
	}

	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		//FIXME: how this method handles delayed effects???
		//it should remove them as well, i think
		if( DelayType( ((*f)->TimingMode) )!=PERMANENT ) {
			if( (*f)->Duration<=GameTime) {
				(*f)->TimingMode = FX_DURATION_JUST_EXPIRED;
			}
		}
	}
}

//this effect will expire all effects that are not truly permanent
//which i call permanent after death (iesdp calls it permanent after bonuses)
void EffectQueue::RemoveAllNonPermanentEffects() const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		if( IsRemovable((*f)->TimingMode) ) {
			(*f)->TimingMode = FX_DURATION_JUST_EXPIRED;
		}
	}
}

//remove certain levels of effects, possibly matching school/secondary type
//this method removes whole spells (tied together by their source)
//FIXME: probably this isn't perfect
void EffectQueue::RemoveLevelEffects(ieResRef &Removed, ieDword level, ieDword Flags, ieDword match) const
{
	Removed[0]=0;
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		if( (*f)->Power>level) {
			continue;
		}

		if( Removed[0]) {
			MATCH_SOURCE();
		}
		if( Flags&RL_MATCHSCHOOL) {
			if( (*f)->PrimaryType!=match) {
				continue;
			}
		}
		if( Flags&RL_MATCHSECTYPE) {
			if( (*f)->SecondaryType!=match) {
				continue;
			}
		}
		//if dispellable was not set, or the effect is dispellable
		//then remove it
		if( Flags&RL_DISPELLABLE) {
			if( !((*f)->Resistance&FX_CAN_DISPEL)) {
				continue;
			}
		}
		(*f)->TimingMode = FX_DURATION_JUST_EXPIRED;
		if( Flags&RL_REMOVEFIRST) {
			memcpy(Removed,(*f)->Source, sizeof(Removed));
		}
	}
}

Effect *EffectQueue::HasOpcode(ieDword opcode) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();

		return (*f);
	}
	return NULL;
}

Effect *EffectQueue::HasEffect(EffectRef &effect_reference) const
{
	ResolveEffectRef(effect_reference);
	if( effect_reference.opcode<0) {
		return NULL;
	}
	return HasOpcode(effect_reference.opcode);
}

Effect *EffectQueue::HasOpcodeWithParam(ieDword opcode, ieDword param2) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();
		MATCH_PARAM2();

		return (*f);
	}
	return NULL;
}

//this will modify effect reference
Effect *EffectQueue::HasEffectWithParam(EffectRef &effect_reference, ieDword param2) const
{
	ResolveEffectRef(effect_reference);
	if( effect_reference.opcode<0) {
		return NULL;
	}
	return HasOpcodeWithParam(effect_reference.opcode, param2);
}

//looks for opcode with pairs of parameters (useful for protection against creature, extra damage or extra thac0 against creature)
//generally an IDS targeting

Effect *EffectQueue::HasOpcodeWithParamPair(ieDword opcode, ieDword param1, ieDword param2) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();
		MATCH_PARAM2();
		//0 is always accepted as first parameter
		if( param1) {
			MATCH_PARAM1();
		}

		return (*f);
	}
	return NULL;
}

//this will modify effect reference
Effect *EffectQueue::HasEffectWithParamPair(EffectRef &effect_reference, ieDword param1, ieDword param2) const
{
	ResolveEffectRef(effect_reference);
	if( effect_reference.opcode<0) {
		return NULL;
	}
	return HasOpcodeWithParamPair(effect_reference.opcode, param1, param2);
}

//this could be used for stoneskins and mirror images as well
void EffectQueue::DecreaseParam1OfEffect(ieDword opcode, ieDword amount) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();
		ieDword value = (*f)->Parameter1;
		if( value>amount) {
			value -= amount;
			amount = 0;
		} else {
			amount -= value;
			value = 0;
		}
		(*f)->Parameter1=value;
		if (value) {
			return;
		}
	}
}

void EffectQueue::DecreaseParam1OfEffect(EffectRef &effect_reference, ieDword amount) const
{
	ResolveEffectRef(effect_reference);
	if( effect_reference.opcode<0) {
		return;
	}
	DecreaseParam1OfEffect(effect_reference.opcode, amount);
}

//this is only used for Cloak of Warding Overlay in PST
//returns the damage amount NOT soaked
int EffectQueue::DecreaseParam3OfEffect(ieDword opcode, ieDword amount, ieDword param2) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();
		MATCH_PARAM2();
		ieDword value = (*f)->Parameter3;
		if( value>amount) {
			value -= amount;
			amount = 0;
		} else {
			amount -= value;
			value = 0;
		}
		(*f)->Parameter3=value;
		if (value) {
			return 0;
		}
	}
	return amount;
}

//this is only used for Cloak of Warding Overlay in PST
//returns the damage amount NOT soaked
int EffectQueue::DecreaseParam3OfEffect(EffectRef &effect_reference, ieDword amount, ieDword param2) const
{
	ResolveEffectRef(effect_reference);
	if( effect_reference.opcode<0) {
		return amount;
	}
	return DecreaseParam3OfEffect(effect_reference.opcode, amount, param2);
}

//this function does IDS targeting for effects (extra damage/thac0 against creature)
//faction/team may be useful for grouping creatures differently, without messing with existing general/specific values
static const int ids_stats[9]={IE_FACTION, IE_TEAM, IE_EA, IE_GENERAL, IE_RACE, IE_CLASS, IE_SPECIFIC, IE_SEX, IE_ALIGNMENT};

//0,1 and 9 are only in GemRB
int EffectQueue::BonusAgainstCreature(ieDword opcode, Actor *actor) const
{
	int sum = 0;
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();
		if( (*f)->Parameter1) {
			ieDword param1;
			ieDword ids = (*f)->Parameter2;
			switch(ids) {
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
				param1 = actor->GetStat(ids_stats[ids]);
				MATCH_PARAM1();
				break;
			case 9:
				//pseudo stat/classmask
				param1 = actor->GetClassMask() & (*f)->Parameter1;
				if (!param1) continue;
				break;
			default:
				break;
			}
		}
		int val = (int) (*f)->Parameter3;
		//we are really lucky with this, most of these boni are using +2 (including fiendslayer feat)
		//it would be much more inconvenient if we had to use v2 effect files
		if( !val) val = 2;
		sum += val;
	}
	return sum;
}

int EffectQueue::BonusAgainstCreature(EffectRef &effect_reference, Actor *actor) const
{
	ResolveEffectRef(effect_reference);
	if( effect_reference.opcode<0) {
		return 0;
	}
	return BonusAgainstCreature(effect_reference.opcode, actor);
}

int EffectQueue::BonusForParam2(ieDword opcode, ieDword param2) const
{
	int sum = 0;
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();
		MATCH_PARAM2();
		sum += (*f)->Parameter1;
	}
	return sum;
}

int EffectQueue::BonusForParam2(EffectRef &effect_reference, ieDword param2) const
{
	ResolveEffectRef(effect_reference);
	if( effect_reference.opcode<0) {
		return 0;
	}
	return BonusForParam2(effect_reference.opcode, param2);
}

bool EffectQueue::WeaponImmunity(ieDword opcode, int enchantment, ieDword weapontype) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();
		//
		int magic = (int) (*f)->Parameter1;
		ieDword mask = (*f)->Parameter3;
		ieDword value = (*f)->Parameter4;
		if( magic==0) {
			if( enchantment) continue;
		} else if( magic>0) {
			if( enchantment>magic) continue;
		}

		if( (weapontype&mask) != value) {
			continue;
		}
		return true;
	}
	return false;
}

bool EffectQueue::WeaponImmunity(int enchantment, ieDword weapontype) const
{
	ResolveEffectRef(fx_weapon_immunity_ref);
	if( fx_weapon_immunity_ref.opcode<0) {
		return 0;
	}
	return WeaponImmunity(fx_weapon_immunity_ref.opcode, enchantment, weapontype);
}

void EffectQueue::AddWeaponEffects(EffectQueue *fxqueue, EffectRef &fx_ref) const
{
	ResolveEffectRef(fx_ref);
	if( fx_ref.opcode<0) {
		return;
	}

	ieDword opcode = fx_ref.opcode;
	Point p(-1,-1);

	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();
		//
		Effect *fx = core->GetEffect( (*f)->Resource, (*f)->Power, p);
		if (!fx) continue;
		fx->Target = FX_TARGET_PRESET;
		fxqueue->AddEffect(fx, true);
	}
}

/* no longer needed, use IE_CASTING stat
static EffectRef fx_disable_spellcasting_ref = { "DisableCasting", -1 };
int EffectQueue::DisabledSpellcasting(int types) const
{
	ResolveEffectRef(fx_disable_spellcasting_ref);
	if( fx_disable_spellcasting_ref.opcode < 0) {
		return 0;
	}

	unsigned int spelltype_mask = 0;
	ieDword opcode = fx_disable_spellcasting_ref.opcode;
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();

		if (iwd2fx) {
			switch((*f)->Parameter2) {
				case 0: // all
					spelltype_mask |= 7;
					break;
				case 1: // mage and cleric
					spelltype_mask |= 3;
					break;
				case 2: // mage
					spelltype_mask |= 2;
					break;
				case 3: // cleric
					spelltype_mask |= 1;
					break;
				case 4: // innate
					spelltype_mask |= 4;
					break;
			}
		} else {
			switch((*f)->Parameter2) {
				case 0: // mage
					spelltype_mask |= 2;
					break;
				case 1: // cleric
					spelltype_mask |= 1;
					break;
				case 2: // innate
					spelltype_mask |= 4;
					break;
			}
		}
	}
	return spelltype_mask & types;
}
*/

//useful for immunity vs spell, can't use item, etc.
Effect *EffectQueue::HasOpcodeWithResource(ieDword opcode, const ieResRef resource) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();
		MATCH_RESOURCE();

		return (*f);
	}
	return NULL;
}

Effect *EffectQueue::HasEffectWithResource(EffectRef &effect_reference, const ieResRef resource) const
{
	ResolveEffectRef(effect_reference);
	return HasOpcodeWithResource(effect_reference.opcode, resource);
}

//returns the first effect with source 'Removed'
Effect *EffectQueue::HasSource(const ieResRef Removed) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_LIVE_FX();
		MATCH_SOURCE();

		return (*f);
	}
	return NULL;
}

//used in contingency/sequencer code (cannot have the same contingency twice)
Effect *EffectQueue::HasOpcodeWithSource(ieDword opcode, const ieResRef Removed) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();
		MATCH_SOURCE();

		return (*f);
	}
	return NULL;
}

Effect *EffectQueue::HasEffectWithSource(EffectRef &effect_reference, const ieResRef resource) const
{
	ResolveEffectRef(effect_reference);
	return HasOpcodeWithSource(effect_reference.opcode, resource);
}

bool EffectQueue::HasAnyDispellableEffect() const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		if( (*f)->Resistance&FX_CAN_DISPEL) {
			return true;
		}
	}
	return false;
}

void EffectQueue::dump() const
{
	StringBuffer buffer;
	dump(buffer);
	Log(DEBUG, "EffectQueue", buffer);
}

void EffectQueue::dump(StringBuffer& buffer) const
{
	buffer.append("EFFECT QUEUE:\n");
	int i = 0;
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		Effect* fx = *f;
		if( fx) {
			char *Name = NULL;
			if( fx->Opcode < MAX_EFFECTS)
				Name = (char*) Opcodes[fx->Opcode].Name;

			buffer.appendFormatted(" %2d: 0x%02x: %s (%d, %d) S:%s\n", i++, fx->Opcode, Name, fx->Parameter1, fx->Parameter2, fx->Source);
		}
	}
}
/*
Effect *EffectQueue::GetEffect(ieDword idx) const
{
	if( effects.size()<=idx) {
		return NULL;
	}
	return effects[idx];
}
*/

//returns true if the effect supports simplified duration
bool EffectQueue::HasDuration(Effect *fx)
{
	switch(fx->TimingMode) {
	case FX_DURATION_INSTANT_LIMITED: //simple duration
	case FX_DURATION_DELAY_LIMITED:   //delayed duration
	case FX_DURATION_DELAY_PERMANENT: //simple delayed
		return true;
	}
	return false;
}

//returns true if the effect must be saved
bool EffectQueue::Persistent(Effect* fx)
{
	// local variable effects self-destruct if they were processed already
	// but if they weren't processed, e.g. in a global actor, we must save them
	// TODO: do we really need to special-case this? leaving it for now - fuzzie
	if( fx->Opcode==(ieDword) ResolveEffect(fx_variable_ref)) {
		return true;
	}

	switch (fx->TimingMode) {
		//normal equipping fx of items
		case FX_DURATION_INSTANT_WHILE_EQUIPPED:
		//delayed effect not saved
		case FX_DURATION_DELAY_UNSAVED:
		//permanent effect not saved
		case FX_DURATION_PERMANENT_UNSAVED:
		//just expired effect
		case FX_DURATION_JUST_EXPIRED:
			return false;
	}
	return true;
}

//alter the color effect in case the item is equipped in the shield slot
void EffectQueue::HackColorEffects(Actor *Owner, Effect *fx)
{
	if( fx->InventorySlot!=Owner->inventory.GetShieldSlot()) return;

	unsigned int gradienttype = fx->Parameter2 & 0xF0;
	if( gradienttype == 0x10) {
		gradienttype = 0x20; // off-hand
		fx->Parameter2 &= ~0xF0;
		fx->Parameter2 |= gradienttype;
	}
}

//iterate through saved effects
const Effect *EffectQueue::GetNextSavedEffect(std::list< Effect* >::const_iterator &f) const
{
	while(f!=effects.end()) {
		Effect *effect = *f;
		f++;
		if( Persistent(effect)) {
			return effect;
		}
	}
	return NULL;
}

Effect *EffectQueue::GetNextEffect(std::list< Effect* >::const_iterator &f) const
{
	if( f!=effects.end()) return *f++;
	return NULL;
}

ieDword EffectQueue::CountEffects(ieDword opcode, ieDword param1, ieDword param2, const char *resource) const
{
	ieDword cnt = 0;

	std::list< Effect* >::const_iterator f;

	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		if( param1!=0xffffffff)
			MATCH_PARAM1();
		if( param2!=0xffffffff)
			MATCH_PARAM2();
		if( resource) {
			MATCH_RESOURCE();
		}
		cnt++;
	}
	return cnt;
}

void EffectQueue::ModifyEffectPoint(ieDword opcode, ieDword x, ieDword y) const
{
	std::list< Effect* >::const_iterator f;

	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		(*f)->PosX=x;
		(*f)->PosY=y;
		(*f)->Parameter3=0;
		return;
	}
}

//count effects that get saved
ieDword EffectQueue::GetSavedEffectsCount() const
{
	ieDword cnt = 0;

	std::list< Effect* >::const_iterator f;

	for ( f = effects.begin(); f != effects.end(); f++ ) {
		Effect* fx = *f;
		if( Persistent(fx))
			cnt++;
	}
	return cnt;
}

int EffectQueue::ResolveEffect(EffectRef &effect_reference)
{
	ResolveEffectRef(effect_reference);
	return effect_reference.opcode;
}

// this check goes for the whole effect block, not individual effects
// But it takes the first effect of the block for the common fields

//returns 1 if effect block applicable
//returns 0 if effect block disabled
//returns -1 if effect block bounced
int EffectQueue::CheckImmunity(Actor *target) const
{
	//don't resist if target is non living
	if( !target) {
		return 1;
	}

	if( effects.size() ) {
		Effect* fx = *effects.begin();

		//projectile immunity
		if( target->ImmuneToProjectile(fx->Projectile)) return 0;

		//don't resist item projectile payloads based on spell school, bounce, etc.
		//FIXME: -Uh, why not ?
		//Allegedly, the book of infinite spells needed this, but irresistable by level
		//spells got fx->Power = 0, so i added those exceptions and removed this
		//if( fx->InventorySlot) {
		//	return 1;
		//}

		//check level resistances
		//check specific spell immunity
		//check school/sectype immunity
		int ret = check_type(target, fx);
		if (ret<0) {
			if (target->Modified[IE_SANCTUARY]&(1<<OV_BOUNCE) ) {
				target->Modified[IE_SANCTUARY]|=(1<<OV_BOUNCE2);
			}
		}
		return ret;
	}
	return 0;
}

void EffectQueue::AffectAllInRange(Map *map, const Point &pos, int idstype, int idsvalue,
		unsigned int range, Actor *except)
{
	int cnt = map->GetActorCount(true);
	while(cnt--) {
		Actor *actor = map->GetActor(cnt,true);
		if( except==actor) {
			continue;
		}
		//distance
		if( Distance(pos, actor)>range) {
			continue;
		}
		//ids targeting
		if( !match_ids(actor, idstype, idsvalue)) {
			continue;
		}
		//line of sight
		if( !map->IsVisibleLOS(actor->Pos, pos)) {
			continue;
		}
		AddAllEffects(actor, actor->Pos);
	}
}


}
