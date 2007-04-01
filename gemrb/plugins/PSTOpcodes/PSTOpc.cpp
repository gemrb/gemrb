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
 * $Id$
 *
 */

#include "../../includes/win32def.h"
#include "../../includes/strrefs.h"
#include "../Core/Actor.h"
#include "../Core/Game.h"
#include "../Core/EffectQueue.h"
#include "../Core/Interface.h"
//#include "../Core/damages.h"
#include "../Core/Video.h"  //for tints
#include "PSTOpc.h"


int fx_set_status (Actor* Owner, Actor* target, Effect* fx);//ba
int fx_play_bam (Actor* Owner, Actor* target, Effect* fx);//bb,bc,bd,be,bf
int fx_transfer_hp (Actor* Owner, Actor* target, Effect* fx);//c0
//int fx_shake_screen (Actor* Owner, Actor* target, Effect* fx);//c1 already implemented
int fx_flash_screen (Actor* Owner, Actor* target, Effect* fx);//c2
int fx_tint_screen (Actor* Owner, Actor* target, Effect* fx);//c3
int fx_special_effect (Actor* Owner, Actor* target, Effect* fx);//c4
//unknown 0xc5-c8
int fx_overlay (Actor* Owner, Actor* target, Effect* fx);//c9
//unknown 0xca
int fx_curse (Actor* Owner, Actor* target, Effect* fx);//cb
int fx_prayer (Actor* Owner, Actor* target, Effect* fx);//cc
int fx_move_view (Actor* Owner, Actor* target, Effect* fx);//cd
int fx_embalm (Actor* Owner, Actor* target, Effect* fx);//ce
int fx_stop_all_action (Actor* Owner, Actor* target, Effect* fx);//cf
int fx_iron_fist (Actor* Owner, Actor* target, Effect* fx);//d0
int fx_hostile_image(Actor* Owner, Actor* target, Effect* fx);//d1
int fx_detect_evil (Actor* Owner, Actor* target, Effect* fx);//d2
int fx_jumble_curse (Actor* Owner, Actor* target, Effect* fx);//d3
//int fx_unknown (Actor* Owner, Actor* target, Effect* fx);//d4

// FIXME: Make this an ordered list, so we could use bsearch!
static EffectRef effectnames[] = {
	{ "Curse", fx_curse, 0},//cb
	{ "DetectEvil", fx_detect_evil, 0}, //d2
	{ "Embalm", fx_embalm, 0}, //0xce
	{ "FlashScreen", fx_flash_screen, 0}, //c2
	{ "HostileImage", fx_hostile_image, 0},//d1
	{ "IronFist", fx_iron_fist, 0}, //d0
	{ "JumbleCurse", fx_jumble_curse, 0}, //d3
	{ "MoveView", fx_move_view, 0},//cd
	{ "Overlay", fx_overlay, 0}, //c9
	{ "PlayBAM1", fx_play_bam, 0}, //bb,bc,bd,be,bf
	{ "PlayBAM2", fx_play_bam, 0},
	{ "PlayBAM3", fx_play_bam, 0},
	{ "PlayBAM4", fx_play_bam, 0},
	{ "PlayBAM5", fx_play_bam, 0},
	{ "Prayer", fx_prayer, 0},//cc
	{ "SetStatus", fx_set_status, 0}, //ba
	{ "SpecialEffect", fx_special_effect, 0},//c4
	{ "StopAllAction", fx_stop_all_action, 0}, //cf
	{ "TintScreen", fx_tint_screen, 0}, //c3
	{ "TransferHP", fx_transfer_hp, 0}, //c0
	{ NULL, NULL, 0 },
};

PSTOpc::PSTOpc(void)
{
	core->RegisterOpcodes( sizeof( effectnames ) / sizeof( EffectRef ) - 1, effectnames );
}

PSTOpc::~PSTOpc(void)
{
}

//0xba fx_set_status
int fx_set_status (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_status (%2d): Par2: %d\n", fx->Opcode, fx->Parameter2 );
	if (fx->Parameter1) {
		BASE_STATE_SET (fx->Parameter2);
	} else {
		BASE_STATE_CURE (fx->Parameter2);
	}
	return FX_NOT_APPLIED;
}

//bb,bc,bd,be,bf fx_play_bam
int fx_play_bam (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_play_bam (%2d): Par2: %d\n", fx->Opcode, fx->Parameter2 );
	//play once set to true
	//check tearring.itm (0xbb effect)
	target->add_animation(fx->Resource, -1, 0, AA_BLEND|AA_PLAYONCE);
	return FX_NOT_APPLIED;
}

//0xc0 fx_transfer_hp
int fx_transfer_hp (Actor* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_transfer_hp (%2d): Par2: %d\n", fx->Opcode, fx->Parameter2 );
	if (Owner==target) {
		return FX_NOT_APPLIED;
	}

	Actor *receiver;
	Actor *donor;
	int a,b;

	switch(fx->Parameter2) {
		case 3:
		case 0: receiver = target; donor = Owner; break;
		case 4:
		case 1: receiver = Owner; donor = target; break;
		case 2:
			a = Owner->GetBase(IE_HITPOINTS);
			b = target->GetBase(IE_HITPOINTS);
			Owner->SetBase(IE_HITPOINTS, a);
			target->SetBase(IE_HITPOINTS, b);
			//fallthrough
		default:
			return FX_NOT_APPLIED;
	}
	int damage = donor->Damage(fx->Parameter1, fx->Parameter2, Owner);
	if (fx->Parameter2>2) {
		receiver->SetBase( IE_HITPOINTS, BASE_GET( IE_HITPOINTS ) + ( damage ) );
	} else {
		receiver->SetStat( IE_HITPOINTS, STAT_GET( IE_HITPOINTS ) + ( damage ), 0 );
	}
	return FX_NOT_APPLIED;
}

//0xc1 fx_shake_screen this is already implemented in BG2

//0xc2 fx_flash_screen
int fx_flash_screen (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_flash_screen (%2d): Par2: %d\n", fx->Opcode, fx->Parameter2 );
	core->GetVideoDriver()->SetFadeColor(((char *) &fx->Parameter1)[0],((char *) &fx->Parameter1)[1],((char *) &fx->Parameter1)[2]);
	core->timer->SetFadeFromColor(1);
	core->timer->SetFadeToColor(1);
	return FX_NOT_APPLIED;
}

//0xc3 fx_tint_screen
int fx_tint_screen (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_tint_screen (%2d): Par2: %d\n", fx->Opcode, fx->Parameter2 );
	core->timer->SetFadeFromColor(10);
	core->timer->SetFadeToColor(10);  
	return FX_NOT_APPLIED;
}

//0xc4 fx_special_effect
int fx_special_effect (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_special_effect (%2d): Par2: %d\n", fx->Opcode, fx->Parameter2 );
	
	return FX_NOT_APPLIED;
}
//0xc5-c8 fx_unknown
//0xc9 fx_overlay
int fx_overlay (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_overlay (%2d): Par2: %d\n", fx->Opcode, fx->Parameter2 );
	target->add_animation(fx->Resource,-1,0,true);
	//special effects based on fx_param2
	return FX_NOT_APPLIED;
}
//0xca fx_unknown
//0xcb fx_curse
int fx_curse (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_curse (%2d): Par2: %d\n", fx->Opcode, fx->Parameter2 );
	//set thac0, ac, saving throws
	return FX_NOT_APPLIED;
}

//0xcc fx_prayer
int fx_prayer (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_prayer (%2d): Par2: %d\n", fx->Opcode, fx->Parameter2 );
	//set thac0, ac, saving throws
	return FX_NOT_APPLIED;
}

//0xcd fx_move_view
int fx_move_view (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_move_view (%2d): Par2: %d\n", fx->Opcode, fx->Parameter2 );
	core->MoveViewportTo( target->Pos.x, target->Pos.y, true );
	return FX_NOT_APPLIED;
}

//0xce fx_embalm
int fx_embalm (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_embalm (%2d): Par2: %d\n", fx->Opcode, fx->Parameter2 );
	if (STATE_GET (STATE_EMBALM) ) //embalm is non cummulative
		return FX_NOT_APPLIED;
	STATE_SET( STATE_EMBALM );
	if (!fx->Parameter1) {
		if (fx->Parameter2) {
			fx->Parameter1=fx->Power*2;
		} else {
			fx->Parameter1=core->Roll(1,6,1);
		}
		BASE_ADD( IE_HITPOINTS, fx->Parameter1 );
	}
	if (fx->Parameter2) {
		STAT_ADD( IE_ARMORCLASS,2 );
	} else {
		STAT_ADD( IE_ARMORCLASS,1 );
	}
	//set global flag
	return FX_APPLIED;
}
//0xcf fx_stop_all_action
int fx_stop_all_action (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_stop_all_action (%2d): Par2: %d\n", fx->Opcode, fx->Parameter2 );
	if (fx->Parameter2) {
		core->GetGame()->TimeStop(NULL, 0xffffffff);
	} else {
		core->GetGame()->TimeStop(NULL, 0);
	}
	return FX_NOT_APPLIED;
}

//0xd0 fx_iron_fist
//GemRB extension: lets you specify not hardcoded values
int fx_iron_fist (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	ieDword p1,p2;

	if (0) printf( "fx_iron_fist (%2d): Par1: %d  Par2: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	switch (fx->Parameter2)
	{
	case 0:  p1 = 3; p2 = 6; break;
	default:
		p1 = ieWord (fx->Parameter1&0xffff);
		p2 = ieWord (fx->Parameter1>>16);
	}
	STAT_ADD(IE_FISTHIT, p1);
	STAT_ADD(IE_FISTDAMAGE, p2);
	return FX_APPLIED;
}

//0xd1 fx_hostile_image
int fx_hostile_image (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_hostile_image (%2d): Par1: %d  Par2: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	return FX_NOT_APPLIED;
}

//0xd2 fx_detect_evil
int fx_detect_evil (Actor* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_detect_evil (%2d): Par1: %d  Par2: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );

	return FX_NOT_APPLIED;
}

//0xd3 fx_jumble_curse
int fx_jumble_curse (Actor* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_jumble_curse (%2d): Par1: %d  Par2: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	STAT_SET( IE_DEADMAGIC, 1);
	STAT_SET( IE_SPELLFAILUREMAGE, 100);
	STAT_SET( IE_SPELLFAILUREPRIEST, 100);
	STAT_SET( IE_SPELLFAILUREINNATE, 100);
	//hiccups
	return FX_APPLIED;
}
