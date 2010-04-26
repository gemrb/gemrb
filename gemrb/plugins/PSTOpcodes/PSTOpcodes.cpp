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

#include "win32def.h"
#include "strrefs.h"
#include "Actor.h"
#include "Game.h"
#include "EffectQueue.h"
#include "Interface.h"
#include "Video.h" //for tints
#include "TileMap.h"

int fx_set_status (Scriptable* Owner, Actor* target, Effect* fx);//ba
int fx_play_bam_blended (Scriptable* Owner, Actor* target, Effect* fx);//bb
int fx_play_bam_not_blended (Scriptable* Owner, Actor* target, Effect* fx);//bc
int fx_transfer_hp (Scriptable* Owner, Actor* target, Effect* fx);//c0
//int fx_shake_screen (Scriptable* Owner, Actor* target, Effect* fx);//c1 already implemented
int fx_flash_screen (Scriptable* Owner, Actor* target, Effect* fx);//c2
int fx_tint_screen (Scriptable* Owner, Actor* target, Effect* fx);//c3
int fx_special_effect (Scriptable* Owner, Actor* target, Effect* fx);//c4
//unknown 0xc5-c8
int fx_overlay (Scriptable* Owner, Actor* target, Effect* fx);//c9
//unknown 0xca
int fx_bless (Scriptable* Owner, Actor* target, Effect* fx);//82 (this is a modified effect)
int fx_curse (Scriptable* Owner, Actor* target, Effect* fx);//cb
int fx_prayer (Scriptable* Owner, Actor* target, Effect* fx);//cc
int fx_move_view (Scriptable* Owner, Actor* target, Effect* fx);//cd
int fx_embalm (Scriptable* Owner, Actor* target, Effect* fx);//ce
int fx_stop_all_action (Scriptable* Owner, Actor* target, Effect* fx);//cf
int fx_iron_fist (Scriptable* Owner, Actor* target, Effect* fx);//d0
int fx_hostile_image(Scriptable* Owner, Actor* target, Effect* fx);//d1
int fx_detect_evil (Scriptable* Owner, Actor* target, Effect* fx);//d2
int fx_jumble_curse (Scriptable* Owner, Actor* target, Effect* fx);//d3
//int fx_unknown (Scriptable* Owner, Actor* target, Effect* fx);//d4

// FIXME: Make this an ordered list, so we could use bsearch!
static EffectRef effectnames[] = {
	{ "Bless", fx_bless, -1},//82
	{ "Curse", fx_curse, -1},//cb
	{ "DetectEvil", fx_detect_evil, -1}, //d2
	{ "Embalm", fx_embalm, -1}, //0xce
	{ "FlashScreen", fx_flash_screen, -1}, //c2
	{ "HostileImage", fx_hostile_image, -1},//d1
	{ "IronFist", fx_iron_fist, -1}, //d0
	{ "JumbleCurse", fx_jumble_curse, -1}, //d3
	{ "MoveView", fx_move_view, -1},//cd
	{ "Overlay", fx_overlay, -1}, //c9
	{ "PlayBAM1", fx_play_bam_blended, -1}, //bb
	{ "PlayBAM2", fx_play_bam_not_blended, -1},//bc
	{ "PlayBAM3", fx_play_bam_not_blended, -1}, //bd
	{ "PlayBAM4", fx_play_bam_not_blended, -1}, //be
	{ "PlayBAM5", fx_play_bam_not_blended, -1}, //bf
	{ "Prayer", fx_prayer, -1},//cc
	{ "SetStatus", fx_set_status, -1}, //ba
	{ "SpecialEffect", fx_special_effect, -1},//c4
	{ "StopAllAction", fx_stop_all_action, -1}, //cf
	{ "TintScreen", fx_tint_screen, -1}, //c3
	{ "TransferHP", fx_transfer_hp, -1}, //c0
	{ NULL, NULL, 0 },
};

void RegisterTormentOpcodes()
{
	core->RegisterOpcodes( sizeof( effectnames ) / sizeof( EffectRef ) - 1, effectnames );
}

//0xba fx_set_status
int fx_set_status (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_set_status (%2d): Par2: %d\n", fx->Opcode, fx->Parameter2 );
	if (fx->Parameter1) {
		if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
			BASE_STATE_SET (fx->Parameter2);
		} else {
			STATE_SET (fx->Parameter2);
		}
	} else {
		if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
			BASE_STATE_CURE (fx->Parameter2);
		} else {
			STATE_CURE (fx->Parameter2);
		}
	}
	return FX_PERMANENT;
}

//bb fx_play_bam_bb (play multi-part blended sticky animation)
// 1 repeats
// 2 not sticky (override default)
int fx_play_bam_blended (Scriptable* Owner, Actor* target, Effect* fx)
{
	bool playonce;

	if (0) printf( "fx_play_bam_blended (%2d): Par2: %d\n", fx->Opcode, fx->Parameter2 );
	//play once set to true
	//check tearring.itm (0xbb effect)
	ScriptedAnimation *sca = gamedata->GetScriptedAnimation(fx->Resource, true);
	if (!sca)
		return FX_NOT_APPLIED;

	sca->SetBlend();
	//the transparency is based on the original palette
	if (fx->Parameter1) {
		RGBModifier rgb;

		rgb.speed=-1;
		rgb.phase=0;
		rgb.rgb.r=fx->Parameter1;
		rgb.rgb.g=fx->Parameter1 >> 8;
		rgb.rgb.b=fx->Parameter1 >> 16;
		rgb.rgb.a=0;
		rgb.type=RGBModifier::TINT;
		sca->AlterPalette(rgb);
	}
	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		playonce=true;
	} else {
		playonce=false;
	}
	if (fx->Parameter2&1) {
		//four cycles, duration is in millisecond
		sca->SetDefaultDuration(sca->GetSequenceDuration(4000));
	} else {
		if (playonce) {
			sca->PlayOnce();
		} else {
			sca->SetDefaultDuration(fx->Duration-core->GetGame()->Ticks);
		}
	}
	if (fx->Parameter2&2) {
		sca->XPos+=fx->PosX;
		sca->YPos+=fx->PosY;
		Owner->GetCurrentArea()->AddVVCell(sca);
	} else {
		ScriptedAnimation *twin = sca->DetachTwin();
		if (twin) {
			target->AddVVCell(twin);
		}
		target->AddVVCell(sca);
	}
	return FX_NOT_APPLIED;
}

//bc-bf play_bam_not_blended (play not blended single animation)
//random placement (if not sticky): 1
//sticky bit: 4096
//transparency instead of rgb tint: 0x100000, fade off is in dice size
//blend:                            0x300000
//twin animation:                   0x30000
//background animation:             0x10000
//foreground animation:             0x20000
int fx_play_bam_not_blended (Scriptable* Owner, Actor* target, Effect* fx)
{
	bool playonce;
	bool doublehint;

	if (0) printf( "fx_play_bam_not_blended (%2d): Par2: %d\n", fx->Opcode, fx->Parameter2 );
	//play once set to true
	//check tearring.itm (0xbb effect)
	if ((fx->Parameter2&0x30000)==0x30000) {
		doublehint = true;
	} else {
		doublehint = false;
	}
	ScriptedAnimation *sca = gamedata->GetScriptedAnimation(fx->Resource, doublehint);
	if (!sca)
		return FX_NOT_APPLIED;

	switch (fx->Parameter2&0x300000) {
	case 0x300000:
		sca->SetBlend(); //per pixel transparency
		break;
	case 0x200000: //this is an insane combo
		sca->SetBlend(); //per pixel transparency
		sca->SetFade((ieByte) fx->Parameter1, fx->DiceSides); //per surface transparency
		break;
	case 0x100000: //per surface transparency
		sca->SetFade((ieByte) fx->Parameter1, fx->DiceSides);
		break;
	default:
		if (fx->Parameter1) {
			RGBModifier rgb;

			rgb.speed=-1;
			rgb.phase=0;
			rgb.rgb.r=fx->Parameter1;
			rgb.rgb.g=fx->Parameter1 >> 8;
			rgb.rgb.b=fx->Parameter1 >> 16;
			rgb.rgb.a=fx->Parameter1 >> 24;
			rgb.type=RGBModifier::TINT;
			sca->AlterPalette(rgb);
		}
	}
	if (fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) {
		playonce=true;
	} else {
		playonce=false;
	}
	switch (fx->Parameter2&0x30000) {
	case 0x20000://foreground
		sca->ZPos+=9999;
		break;
	case 0x30000: //both
		sca->ZPos+=9999;
		if (sca->twin) {
			sca->twin->ZPos-=9999;
		}
		break;
	default: //background
		sca->ZPos-=9999;
		break;
	}
	if (playonce) {
		sca->PlayOnce();
	} else {
		sca->SetDefaultDuration(fx->Duration-core->GetGame()->Ticks);
	}
	ScriptedAnimation *twin = sca->DetachTwin();
	if (fx->Parameter2&4096) {
		if (twin) {
			target->AddVVCell(twin);
		}
		target->AddVVCell(sca);
	} else {
		//the random placement works only when it is not sticky
		int x = 0;
		int y = 0;
		if (fx->Parameter2&1) {
			ieWord tmp =(ieWord) rand();
			x = tmp&31;
			y = (tmp>>5)&31;
		}

		sca->XPos+=fx->PosX-x;
		sca->YPos+=fx->PosY+sca->ZPos-y;
		if (twin) {
			twin->XPos+=fx->PosX-x;
			twin->YPos+=fx->PosY+twin->ZPos-y;
			Owner->GetCurrentArea()->AddVVCell(twin);
		}
		Owner->GetCurrentArea()->AddVVCell(sca);
	}
	return FX_NOT_APPLIED;
}

//0xc0 fx_transfer_hp
int fx_transfer_hp (Scriptable* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_transfer_hp (%2d): Par2: %d\n", fx->Opcode, fx->Parameter2 );
	if (Owner->Type!=ST_ACTOR) {
		return FX_NOT_APPLIED;
	}

	Actor *owner = (Actor *) Owner;

	if (owner==target) {
		return FX_NOT_APPLIED;
	}

	Actor *receiver;
	Actor *donor;
	int a,b;

	switch(fx->Parameter2) {
		case 3:
		case 0: receiver = target; donor = owner; break;
		case 4:
		case 1: receiver = owner; donor = target; break;
		case 2:
			a = owner->GetBase(IE_HITPOINTS);
			b = target->GetBase(IE_HITPOINTS);
			owner->SetBase(IE_HITPOINTS, a);
			target->SetBase(IE_HITPOINTS, b);
			//fallthrough
		default:
			return FX_NOT_APPLIED;
	}
	int damage = donor->Damage(fx->Parameter1, fx->Parameter2, owner);
	receiver->SetBase( IE_HITPOINTS, BASE_GET( IE_HITPOINTS ) + ( damage ) );
	return FX_NOT_APPLIED;
}

//0xc1 fx_shake_screen this is already implemented in BG2

//0xc2 fx_flash_screen
int fx_flash_screen (Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_flash_screen (%2d): Par2: %d\n", fx->Opcode, fx->Parameter2 );
	core->GetVideoDriver()->SetFadeColor(((char *) &fx->Parameter1)[0],((char *) &fx->Parameter1)[1],((char *) &fx->Parameter1)[2]);
	core->timer->SetFadeFromColor(1);
	core->timer->SetFadeToColor(1);
	return FX_NOT_APPLIED;
}

//0xc3 fx_tint_screen
int fx_tint_screen (Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_tint_screen (%2d): Par2: %d\n", fx->Opcode, fx->Parameter2 );
	core->timer->SetFadeFromColor(10);
	core->timer->SetFadeToColor(10);
	return FX_NOT_APPLIED;
}

//0xc4 fx_special_effect
int fx_special_effect (Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_special_effect (%2d): Par2: %d\n", fx->Opcode, fx->Parameter2 );

	return FX_NOT_APPLIED;
}
//0xc5-c8 fx_unknown
//0xc9 fx_overlay
int fx_overlay (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_overlay (%2d): Par2: %d\n", fx->Opcode, fx->Parameter2 );
	target->AddAnimation(fx->Resource,-1,0,true);
	//special effects based on fx_param2
	return FX_NOT_APPLIED;
}
//0xca fx_unknown

//0x82 fx_bless
//static EffectRef fx_glow_ref ={"Color:PulseRGBGlobal",NULL,-1};
//pst bless effect spawns a color glow automatically
//but i would rather use the IWD2 method
int fx_bless (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_curse (%2d): Par1: %d\n", fx->Opcode, fx->Parameter1 );
	//this bit is the same as the invisibility bit in other games
	//it should be considered what if we replace the pst invis bit
	//with this one (losing binary compatibility, gaining easier
	//invis checks at core level)
	if (STATE_GET (STATE_BLESS) ) //curse is non-cumulative
		return FX_NOT_APPLIED;

	target->SetColorMod(255, RGBModifier::ADD, 0x18, 0xc8, 0xc8, 0xc8);

	STATE_SET( STATE_BLESS );
	STAT_SUB( IE_TOHIT, fx->Parameter1);
	STAT_SUB( IE_SAVEVSDEATH, fx->Parameter1);
	STAT_SUB( IE_SAVEVSWANDS, fx->Parameter1);
	STAT_SUB( IE_SAVEVSPOLY, fx->Parameter1);
	STAT_SUB( IE_SAVEVSBREATH, fx->Parameter1);
	STAT_SUB( IE_SAVEVSSPELL, fx->Parameter1);
	return FX_APPLIED;
}

//0xcb fx_curse
int fx_curse (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_curse (%2d): Par1: %d\n", fx->Opcode, fx->Parameter1 );
	//this bit is the same as the invisibility bit in other games
	//it should be considered what if we replace the pst invis bit
	//with this one (losing binary compatibility, gaining easier
	//invis checks at core level)
	if (STATE_GET (STATE_PST_CURSE) ) //curse is non cumulative
		return FX_NOT_APPLIED;
	STATE_SET( STATE_PST_CURSE );
	STAT_SUB( IE_TOHIT, fx->Parameter1);
	STAT_SUB( IE_SAVEVSDEATH, fx->Parameter1);
	STAT_SUB( IE_SAVEVSWANDS, fx->Parameter1);
	STAT_SUB( IE_SAVEVSPOLY, fx->Parameter1);
	STAT_SUB( IE_SAVEVSBREATH, fx->Parameter1);
	STAT_SUB( IE_SAVEVSSPELL, fx->Parameter1);
	return FX_APPLIED;
}

//0xcc fx_prayer
static EffectRef fx_curse_ref={"Curse",NULL,-1};
static EffectRef fx_bless_ref={"Bless",NULL,-1};

int fx_prayer (Scriptable* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_prayer (%2d): Par1: %d\n", fx->Opcode, fx->Parameter1 );
	int ea = target->GetStat(IE_EA);
	int type;
	if (ea>EA_EVILCUTOFF) type = 1;
	else if (ea<EA_GOODCUTOFF) type = 0;
	else return FX_NOT_APPLIED;  //what happens if the target goes neutral during the effect? if the effect remains, make this FX_APPLIED

	Map *map = target->GetCurrentArea();
	int i = map->GetActorCount(true);
	Effect *newfx = EffectQueue::CreateEffect(type?fx_curse_ref:fx_bless_ref, fx->Parameter1, fx->Parameter2, FX_DURATION_INSTANT_LIMITED);
	memcpy(newfx, fx->Source,sizeof(ieResRef));
	newfx->Duration=60;
	while(i--) {
		Actor *tar=map->GetActor(i,true);
		ea = tar->GetStat(IE_EA);
		if (ea>EA_EVILCUTOFF) type^=1;
		else if (ea>EA_GOODCUTOFF) continue;
		//this isn't a real perma effect, just applying the effect now
		//no idea how this should work with spell resistances, etc
		//lets assume it is never resisted
		//the effect will be destructed by ApplyEffect (not anymore)
		//the effect is copied to a new memory area
		core->ApplyEffect(newfx, tar, Owner);
	}
	delete newfx;
	return FX_APPLIED;
}

//0xcd fx_move_view
int fx_move_view (Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_move_view (%2d): Speed: %d\n", fx->Opcode, fx->Parameter1 );
	Map *map = core->GetGame()->GetCurrentArea();
	if (map) {
		core->timer->SetMoveViewPort( fx->PosX, fx->PosY, fx->Parameter1, true);
	}
	return FX_NOT_APPLIED;
}

//0xce fx_embalm
int fx_embalm (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_embalm (%2d): Par2: %d\n", fx->Opcode, fx->Parameter2 );
	if (STATE_GET (STATE_EMBALM) ) //embalm is non cumulative
		return FX_NOT_APPLIED;
	STATE_SET( STATE_EMBALM );
	if (!fx->Parameter1) {
		if (fx->Parameter2) {
			fx->Parameter1=fx->CasterLevel*2;
		} else {
			fx->Parameter1=core->Roll(1,6,1);
		}
		BASE_ADD( IE_HITPOINTS, fx->Parameter1 );
	}
	STAT_ADD( IE_MAXHITPOINTS, fx->Parameter1);
	if (fx->Parameter2) {
		STAT_ADD( IE_ARMORCLASS,2 );
	} else {
		STAT_ADD( IE_ARMORCLASS,1 );
	}
	return FX_APPLIED;
}
//0xcf fx_stop_all_action
int fx_stop_all_action (Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
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
int fx_iron_fist (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	ieDword p1,p2;

	if (0) printf( "fx_iron_fist (%2d): Par1: %d Par2: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	switch (fx->Parameter2)
	{
	case 0: p1 = 3; p2 = 6; break;
	default:
		p1 = ieWord (fx->Parameter1&0xffff);
		p2 = ieWord (fx->Parameter1>>16);
	}
	STAT_ADD(IE_FISTHIT, p1);
	STAT_ADD(IE_FISTDAMAGE, p2);
	return FX_APPLIED;
}

//0xd1 fx_hostile_image
int fx_hostile_image (Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (0) printf( "fx_hostile_image (%2d): Par1: %d Par2: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	return FX_NOT_APPLIED;
}

//0xd2 fx_detect_evil
static EffectRef fx_single_color_pulse_ref={"Color:BriefRGB",NULL,-1};

int fx_detect_evil (Scriptable* Owner, Actor* target, Effect* fx)
{
	if (0) printf( "fx_detect_evil (%2d): Par1: %d Par2: %d\n", fx->Opcode, fx->Parameter1, fx->Parameter2 );
	ieDword type = fx->Parameter2;
	//default is alignment/evil/speed 30/range 10
	if (!type) type = 0x08031e0a;
	int speed = (type&0xff00)>>8;
	if (!speed) speed=30;
	if (!(core->GetGame()->GameTime%speed)) {
		ieDword color = fx->Parameter1;
		//default is magenta (rgba)
		if (!color) color = 0xff00ff00;
		Effect *newfx = EffectQueue::CreateEffect(fx_single_color_pulse_ref, color, speed<<16, FX_DURATION_INSTANT_PERMANENT_AFTER_BONUSES);
		newfx->Target=FX_TARGET_PRESET;
		EffectQueue *fxqueue = new EffectQueue();
		fxqueue->SetOwner(Owner);
		fxqueue->AddEffect(newfx);
		delete newfx;

		//don't detect self? if yes, then use NULL as last parameter
		fxqueue->AffectAllInRange(target->GetCurrentArea(), target->Pos, (type&0xff000000)>>24, (type&0xff0000)>>16, (type&0xff)*10, target);
		delete fxqueue;
	}
	return FX_APPLIED;
}

//0xd3 fx_jumble_curse
int fx_jumble_curse (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	if (0) printf( "fx_jumble_curse (%2d)\n", fx->Opcode );

	if (STATE_GET( STATE_DEAD) ) {
		return FX_NOT_APPLIED;
	}
	Game *game = core->GetGame();
	//do a hiccup every 75th refresh
	if (fx->Parameter3/75!=fx->Parameter4/75) {
		//hiccups
		//PST has this hardcoded deep in the engine
		//gemrb lets you specify the strref in P#1
		ieStrRef tmp = fx->Parameter1;
		if (!tmp) tmp = 46633;
		char *tmpstr = core->GetString(tmp, IE_STR_SPEECH|IE_STR_SOUND);
		target->DisplayHeadText(tmpstr);
		//tmpstr shouldn't be freed, it is taken care by Actor
		target->GetHit();
	}
	fx->Parameter4=fx->Parameter3;
	fx->Parameter3=game->GameTime;
	STAT_SET( IE_DEADMAGIC, 1);
	STAT_SET( IE_SPELLFAILUREMAGE, 100);
	STAT_SET( IE_SPELLFAILUREPRIEST, 100);
	STAT_SET( IE_SPELLFAILUREINNATE, 100);
	return FX_APPLIED;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x115A670, "Effect opcodes for the torment branch of the games")
PLUGIN_INITIALIZER(RegisterTormentOpcodes)
END_PLUGIN()
