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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/IWDOpcodes/IWDOpc.cpp,v 1.1 2006/04/13 18:40:27 avenger_teambg Exp $
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

// FIXME: Make this an ordered list, so we could use bsearch!
static EffectRef effectnames[] = {
	{ "Color:FadeRGB", fx_fade_rgb, 0 }, //e8
	{ "IWDVisualSpellHit", fx_iwd_visual_spell_hit}, //e9
	{ "ColdDamage", fx_cold_damage}, //ea
	{ "IWDCastingGlow", fx_iwd_casting_glow}, //eb
	{ "TurnUndead", fx_turn_undead}, //ec
	{ "CrushingDamage", fx_crushing_damage}, //ed
	{ NULL, NULL, 0 },
};


IWDOpc::IWDOpc(void)
{
	core->RegisterOpcodes( sizeof( effectnames ) / sizeof( EffectRef ) - 1, effectnames );
}

IWDOpc::~IWDOpc(void)
{
}

bool IWDOpc::Init(void)
{
	return true;
}

// 0xe8 Colour:FadeRGB
int fx_fade_rgb (Actor* /*Owner*/, Actor* target, Effect* fx)
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
