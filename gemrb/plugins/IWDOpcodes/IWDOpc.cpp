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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/IWDOpcodes/IWDOpc.cpp,v 1.4 2006/07/25 19:58:56 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "../../includes/strrefs.h"
#include "../Core/Actor.h"
#include "../Core/EffectQueue.h"
#include "../Core/Interface.h"
#include "../Core/damages.h"
#include "IWDOpc.h"


int fx_fade_rgb (Actor* Owner, Actor* target, Effect* fx);//e8
int fx_iwd_visual_spell_hit (Actor* Owner, Actor* target, Effect* fx);//e9
int fx_cold_damage (Actor* Owner, Actor* target, Effect* fx);//ea
int fx_iwd_casting_glow (Actor* Owner, Actor* target, Effect* fx);//eb
int fx_turn_undead (Actor* Owner, Actor* target, Effect* fx);//ec
int fx_crushing_damage (Actor* Owner, Actor* target, Effect* fx);//ed
int fx_save_bonus (Actor* Owner, Actor* target, Effect* fx); //ee
int fx_slow_poison (Actor* Owner, Actor* target, Effect* fx); //ef
int fx_iwd_monster_summoning (Actor* Owner, Actor* target, Effect* fx); //f0

// FIXME: Make this an ordered list, so we could use bsearch!
static EffectRef effectnames[] = {
	{ "Color:FadeRGB", fx_fade_rgb, 0 }, //e8
	{ "IWDVisualSpellHit", fx_iwd_visual_spell_hit, 0}, //e9
	{ "ColdDamage", fx_cold_damage, 0}, //ea
	{ "IWDCastingGlow", fx_iwd_casting_glow, 0}, //eb
	{ "IWDMonsterSummoning", fx_iwd_monster_summoning, 0}, //f0
	{ "TurnUndead", fx_turn_undead, 0}, //ec
	{ "CrushingDamage", fx_crushing_damage, 0}, //ed
	{ "SaveBonus", fx_save_bonus, 0}, //ee
	{ "SlowPoison", fx_slow_poison, 0}, //ef
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
int fx_iwd_visual_spell_hit (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_iwd_visual_spell_hit (%2d): Type: %d\n", fx->Opcode, fx->Parameter1 );
	return FX_NOT_APPLIED;
}

//0xea ColdDamage (special)
int fx_cold_damage (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_cold_damage (%2d): Damage %d\n", fx->Opcode, fx->Parameter1 );
	int damage;

	damage = core->Roll(fx->DiceSides, fx->DiceThrown, fx->Parameter1);
	damage = target->Damage(damage, DAMAGE_COLD, Owner); //FIXME!
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
	int damage;

	damage = core->Roll(fx->DiceSides, fx->DiceThrown, fx->Parameter1);
	damage = target->Damage(damage, DAMAGE_CRUSHING, Owner); //FIXME!
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

//0xf0 IWDMonsterSummoning
int fx_iwd_monster_summoning (Actor* Owner, Actor* target, Effect* fx)
{
  if (0) printf( "fx_iwd_monster_summoning (%2d): ResRef:%s Anim:%s Type: %d\n", fx->Opcode, fx->Resource, fx->Resource2, fx->Parameter2 );
  //check the summoning limit?
  
  //get monster resref from 2da determined by fx->Resource or fx->Parameter2
  ieResRef monster;
  
  //the monster should appear near the effect position
  Point p(fx->PosX, fx->PosY);
  core->SummonCreature(monster, fx->Resource2, Owner, target, p, fx->Parameter2);
  return FX_NOT_APPLIED;
}


