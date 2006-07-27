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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/IWDOpcodes/IWDOpc.cpp,v 1.6 2006/07/27 19:11:35 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "../../includes/strrefs.h"
#include "../Core/Actor.h"
#include "../Core/EffectQueue.h"
#include "../Core/Interface.h"
#include "../Core/damages.h"
#include "IWDOpc.h"

static ieResRef *iwd_spell_hits = NULL;
static int shcount = -1;

int fx_fade_rgb (Actor* Owner, Actor* target, Effect* fx);//e8
int fx_iwd_visual_spell_hit (Actor* Owner, Actor* target, Effect* fx);//e9
int fx_cold_damage (Actor* Owner, Actor* target, Effect* fx);//ea
int fx_iwd_casting_glow (Actor* Owner, Actor* target, Effect* fx);//eb
int fx_turn_undead (Actor* Owner, Actor* target, Effect* fx);//ec
int fx_crushing_damage (Actor* Owner, Actor* target, Effect* fx);//ed
int fx_save_bonus (Actor* Owner, Actor* target, Effect* fx); //ee
int fx_slow_poison (Actor* Owner, Actor* target, Effect* fx); //ef
int fx_iwd_monster_summoning (Actor* Owner, Actor* target, Effect* fx); //f0
int fx_vampiric_touch (Actor* Owner, Actor* target, Effect* fx); //f1
// int fx_overlay f2 (same as PST)
int fx_animate_dead (Actor* Owner, Actor* target, Effect* fx);//f3
// int fx_prayer f4 (same as PST)
// int fx_curse f5 (same as PST)
int fx_summon_monster_2 (Actor* Owner, Actor* target, Effect* fx); //f6
int fx_burning_blood (Actor* Owner, Actor* target, Effect* fx); //f7
int fx_summon_shadow_monster (Actor* Owner, Actor* target, Effect* fx); //f8
// positive recitation same as fx_prayer //f9
// negative recitation same as fx_curse //fa
// lich touch hold same as hold2 //fb
// sol's blinding //fc
// ac vs damage //fd
int fx_remove_effects (Actor* Owner, Actor* target, Effect* fx); //fe

//No need to make these ordered, they will be ordered by EffectQueue
static EffectRef effectnames[] = {
	{ "Color:FadeRGB", fx_fade_rgb, 0 }, //e8
	{ "IWDVisualSpellHit", fx_iwd_visual_spell_hit, 0}, //e9
	{ "ColdDamage", fx_cold_damage, 0}, //ea
	{ "IWDCastingGlow", fx_iwd_casting_glow, 0}, //eb
	{ "TurnUndead", fx_turn_undead, 0}, //ec
	{ "CrushingDamage", fx_crushing_damage, 0}, //ed
	{ "SaveBonus", fx_save_bonus, 0}, //ee
	{ "SlowPoison", fx_slow_poison, 0}, //ef
	{ "IWDMonsterSummoning", fx_iwd_monster_summoning, 0}, //f0
	{ "VampiricTouch", fx_vampiric_touch, 0}, //f1
	{ "AnimateDead", fx_animate_dead, 0}, //f3
	{ "SummonMonster2", fx_summon_monster_2, 0}, //f6
	{ "RemoveEffects", fx_remove_effects, 0}, //fe
	{ NULL, NULL, 0 },
};


IWDOpc::IWDOpc(void)
{
	core->RegisterOpcodes( sizeof( effectnames ) / sizeof( EffectRef ) - 1, effectnames );
}

IWDOpc::~IWDOpc(void)
{
}

// 0xe8 Colour:FadeRGB
int fx_fade_rgb (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_fade_rgb (%2d): \n", fx->Opcode  );
	return FX_NOT_APPLIED;
}

//0xe9 IWDVisualSpellHit
int fx_iwd_visual_spell_hit (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_iwd_visual_spell_hit (%2d): Type: %d\n", fx->Opcode, fx->Parameter1 );
	if (shcount<0) {
		shcount = core->ReadResRefTable("iwdshtab",iwd_spell_hits);
	}
	//delay apply until map is loaded
	Map *map = target->GetCurrentArea();
	if (!map) {
		return FX_APPLIED;
	}
	if (fx->Parameter2<(ieDword) shcount) {
		ScriptedAnimation *sca = core->GetScriptedAnimation(iwd_spell_hits[fx->Parameter2]);
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

//0xeb IWDCastingGlow
int fx_iwd_casting_glow (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_iwd_casting_glow (%2d): Type: %d\n", fx->Opcode, fx->Parameter1 );
	return FX_NOT_APPLIED;
}

//0xec TurnUndead
int fx_turn_undead (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_turn_undead (%2d): Type: %d\n", fx->Opcode, fx->Parameter1 );
	//
	return FX_NOT_APPLIED;
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
	if (0) printf( "fx_save_bonus (%2d): Damage %d\n", fx->Opcode, fx->Parameter1 );
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
 "MSUMMO5","MSUMMO6","MSUMMO7","ASUMMO1","ASUMMO2","ASUMMO3","GINSECT","CDOOM",
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

#define IWD_SM2 10 
ieResRef summon_monster_2da[IWD_SM2]={"STROLLS","SLIZARD","ISTALKE","SSHADOW",
 "CEELEMP","CFELEMP","CWELEMW","CEELEMM","CEELEMW","CFELEMW"};

//0xf6 SummonMonster2 
int fx_summon_monster_2 (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_summon_monster_2 (%2d): ResRef:%s Type: %d\n", fx->Opcode, fx->Resource, fx->Parameter2 );
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

//0xfe RemoveEffects
int fx_remove_effects (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_remove_effects (%2d): ResRef:%s Type: %d\n", fx->Opcode, fx->Resource, fx->Parameter2 );

	target->fxqueue.RemoveAllEffects(fx->Resource);
	return FX_NOT_APPLIED;
}

