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

#include "strrefs.h"

#include "EffectQueue.h"
#include "Game.h"
#include "GameData.h"
#include "GlobalTimer.h"
#include "Interface.h"
#include "Map.h"
#include "RNG.h"
#include "TableMgr.h"
#include "TileMap.h"
#include "VEFObject.h"
#include "Video/Video.h" //for tints

#include "GameScript/GSUtils.h"
#include "Scriptable/Actor.h"
#include "ScriptedAnimation.h"

using namespace GemRB;

// TODO: recheck 0x52 is Incite Berserk Attack and implement; currently mapped to bg1 SetAIScript in effect.ids
int fx_retreat_from (Scriptable* Owner, Actor* target, Effect* fx);//6e
int fx_set_status (Scriptable* Owner, Actor* target, Effect* fx);//ba
int fx_play_bam_blended (Scriptable* Owner, Actor* target, Effect* fx);//bb
int fx_play_bam_not_blended (Scriptable* Owner, Actor* target, Effect* fx);//bc
int fx_transfer_hp (Scriptable* Owner, Actor* target, Effect* fx);//c0
//int fx_shake_screen (Scriptable* Owner, Actor* target, Effect* fx);//c1 already implemented in fxopcodes
int fx_flash_screen (Scriptable* Owner, Actor* target, Effect* fx);//c2
int fx_tint_screen (Scriptable* Owner, Actor* target, Effect* fx);//c3
int fx_special_effect (Scriptable* Owner, Actor* target, Effect* fx);//c4
int fx_multiple_vvc (Scriptable* Owner, Actor* target, Effect* fx);//c5 //gemrb specific
//int fx_modify_global ((Scriptable* Owner, Actor* target, Effect* fx);//c6 already implemented in fxopcodes
int fx_change_background (Scriptable* Owner, Actor* target, Effect* fx);//c7 //gemrb specific
//unknown 0xc7-c8
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
int fx_speak_with_dead (Scriptable* Owner, Actor* target, Effect* fx);//d4

//the engine sorts these, feel free to use any order
static EffectDesc effectnames[] = {
	EffectDesc("Bless", fx_bless, 0, -1 ),//82
	EffectDesc("ChangeBackground", fx_change_background, EFFECT_NO_ACTOR, -1 ), //c6
	EffectDesc("Curse", fx_curse, 0, -1 ),//cb
	EffectDesc("DetectEvil", fx_detect_evil, 0, -1 ), //d2
	EffectDesc("Embalm", fx_embalm, 0, -1 ), //0xce
	EffectDesc("FlashScreen", fx_flash_screen, EFFECT_NO_ACTOR, -1 ), //c2
	EffectDesc("HostileImage", fx_hostile_image, 0, -1 ),//d1
	EffectDesc("IronFist", fx_iron_fist, 0, -1 ), //d0
	EffectDesc("JumbleCurse", fx_jumble_curse, 0, -1 ), //d3
	EffectDesc("MoveView", fx_move_view, EFFECT_NO_ACTOR, -1 ),//cd
	EffectDesc("MultipleVVC", fx_multiple_vvc, EFFECT_NO_ACTOR, -1 ), //c5
	EffectDesc("Overlay", fx_overlay, 0, -1 ), //c9
	EffectDesc("PlayBAM1", fx_play_bam_blended, 0, -1 ), //bb
	EffectDesc("PlayBAM2", fx_play_bam_not_blended, 0, -1 ),//bc
	EffectDesc("PlayBAM3", fx_play_bam_not_blended, 0, -1 ), //bd
	EffectDesc("PlayBAM4", fx_play_bam_not_blended, 0, -1 ), //be
	EffectDesc("PlayBAM5", fx_play_bam_not_blended, 0, -1 ), //bf
	EffectDesc("Prayer", fx_prayer, 0, -1 ),//cc
	EffectDesc("RetreatFrom", fx_retreat_from, 0, -1 ),//6e
	EffectDesc("SetStatus", fx_set_status, 0, -1 ), //ba
	EffectDesc("SpeakWithDead", fx_speak_with_dead, 0, -1 ), //d4
	EffectDesc("SpecialEffect", fx_special_effect, 0, -1 ),//c4
	EffectDesc("StopAllAction", fx_stop_all_action, EFFECT_NO_ACTOR, -1 ), //cf
	EffectDesc("TintScreen", fx_tint_screen, EFFECT_NO_ACTOR, -1 ), //c3
	EffectDesc("TransferHP", fx_transfer_hp, EFFECT_DICED, -1 ), //c0
	EffectDesc(nullptr, nullptr, 0, 0),
};

static void RegisterTormentOpcodes()
{
	core->RegisterOpcodes( sizeof( effectnames ) / sizeof( EffectDesc ) - 1, effectnames );
}

//retreat_from (works only in PST) - forces target to run away/walk away from Owner
int fx_retreat_from (Scriptable* Owner, Actor* target, Effect* fx)
{
	// print("fx_retreat_from(%2d): Mod: %d, Type: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);

	if (!Owner) {
		return FX_NOT_APPLIED;
	}

	//distance to run
	if (!fx->Parameter3) {
		fx->Parameter3=100;
	}

	if (fx->Parameter2==8) {
		//backs away from owner
		target->RunAwayFrom(Owner->Pos, fx->Parameter3, false);
		//one shot
		return FX_NOT_APPLIED;
	}

	//walks (7) or runs away (all others) from owner
	target->RunAwayFrom(Owner->Pos, fx->Parameter3, true);
	if (fx->Parameter2!=7) {
		target->SetRunFlags(IF_RUNNING);
	}

	//has a duration
	return FX_APPLIED;
}

//0xba fx_set_status
int fx_set_status (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_set_status(%2d): Par2: %d", fx->Opcode, fx->Parameter2);
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

	// print("fx_play_bam_blended(%2d): Par2: %d", fx->Opcode, fx->Parameter2);
	if (!Owner)
		Owner = target;
	if (!Owner)
		return FX_NOT_APPLIED;
	//delay effect
	Map *area = Owner->GetCurrentArea();
	if (!area)
		return FX_APPLIED;

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
		rgb.rgb = Color::FromABGR(fx->Parameter1);
		rgb.type=RGBModifier::TINT;
		sca->AlterPalette(rgb);
	}
	if ((fx->TimingMode==FX_DURATION_INSTANT_LIMITED) && (fx->Parameter2&1) ) {
		playonce=false;
	} else {
		playonce=true;
	}
	if (playonce) {
		sca->PlayOnce();
	} else {
		if (fx->Parameter2&1) {
			//four cycles, duration is in millisecond
			sca->SetDefaultDuration(sca->GetSequenceDuration(core->Time.ai_update_time));
		} else {
			sca->SetDefaultDuration(fx->Duration-core->GetGame()->Ticks);
		}
	}
	//convert it to an area VVC
	if (!target) {
		fx->Parameter2|=2;
	}

	if (fx->Parameter2&2) {
		sca->Pos = fx->Pos;
		area->AddVVCell( new VEFObject(sca));
	} else {
		assert(target);
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

	// print("fx_play_bam_not_blended(%2d): Par2: %d", fx->Opcode, fx->Parameter2);
	if (!Owner)
		Owner = target;
	if (!Owner)
		return FX_NOT_APPLIED;

	Map *area = Owner->GetCurrentArea();
	if (!area)
		return FX_APPLIED;

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
			rgb.rgb = Color::FromABGR(fx->Parameter1);
			rgb.type=RGBModifier::TINT;
			sca->AlterPalette(rgb);
		}
	}
	if (fx->TimingMode==FX_DURATION_INSTANT_LIMITED) {
		playonce=false;
	} else {
		playonce=true;
	}
	switch (fx->Parameter2&0x30000) {
	case 0x20000://foreground
		sca->ZOffset += 9999;
		sca->YOffset += 9999;
		break;
	case 0x30000: //both
		sca->ZOffset += 9999;
		sca->YOffset += 9999;
		if (sca->twin) {
			sca->twin->ZOffset -= 9999;
			sca->twin->YOffset -= 9999;
		}
		break;
	default: //background
		sca->ZOffset -= 9999;
		sca->YOffset -= 9999;
		break;
	}
	if (playonce) {
		sca->PlayOnce();
	} else {
		sca->SetDefaultDuration(fx->Duration-core->GetGame()->Ticks);
	}
	ScriptedAnimation *twin = sca->DetachTwin();

	if (target && (fx->Parameter2&4096)) {
		if (twin) {
			target->AddVVCell(twin);
		}
		target->AddVVCell(sca);
	} else {
		//the random placement works only when it is not sticky
		int x = 0;
		int y = 0;
		if (fx->Parameter2&1) {
			ieWord tmp =(ieWord) RAND_ALL();
			x = tmp&31;
			y = (tmp>>5)&31;
		}
		
		sca->Pos = fx->Pos;
		sca->XOffset -= x;
		sca->YOffset -= y;

		if (twin) {
			twin->Pos = fx->Pos;
			twin->XOffset -= x;
			twin->YOffset -= y;
			area->AddVVCell( new VEFObject(twin) );
		}
		area->AddVVCell( new VEFObject(sca) );
	}
	return FX_NOT_APPLIED;
}

//0xc0 fx_transfer_hp
int fx_transfer_hp (Scriptable* Owner, Actor* target, Effect* fx)
{
	// print("fx_transfer_hp(%2d): Par2: %d", fx->Opcode, fx->Parameter2);
	if (Owner->Type!=ST_ACTOR) {
		return FX_NOT_APPLIED;
	}

	Actor *owner = GetCasterObject();

	if (owner == target || !owner || !target) {
		return FX_NOT_APPLIED;
	}

	Actor *receiver;
	Actor *donor;
	int a,b;

	//handle variable level hp drain (for blood bridge)
	if (fx->IsVariable) {
		fx->Parameter1+=fx->CasterLevel;
		fx->IsVariable=0;
	}

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
	int damage = receiver->GetStat(IE_MAXHITPOINTS)-receiver->GetStat(IE_HITPOINTS);
	if (damage>(signed) fx->Parameter1) {
		damage=(signed) fx->Parameter1;
	}
	if (damage) {
		damage = donor->Damage(damage, fx->Parameter2, owner, MOD_ADDITIVE, fx->SavingThrowType);
		receiver->NewBase( IE_HITPOINTS, damage, MOD_ADDITIVE );
	}
	return FX_NOT_APPLIED;
}

//0xc1 fx_shake_screen this is already implemented in BG2

//0xc2 fx_flash_screen
int fx_flash_screen (Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	// print("fx_flash_screen(%2d): Par2: %d", fx->Opcode, fx->Parameter2);
	const unsigned char* bytes = (const unsigned char *) &fx->Parameter1;
	Color c(bytes[0], bytes[1], bytes[2], 0xff);
	core->GetWindowManager()->FadeColor = c;
	//this needs to be at least 2 for any effect
	core->timer.SetFadeFromColor(2);
	return FX_NOT_APPLIED;
}

//0xc3 fx_tint_screen
// NOTE: it actually ignored actors, not tinting them
// Most modes are untested, the original only used 1 and 5
// Users in the original:
//
// Resref      name type color dice duration
// SPIN108.SPL raise dead: 1 grey 16d8 5
// SPPR204.SPL spiritual hammer: 1 grey 8d6 0
// SPPR502.SPL raise dead: 1 grey 16d8 5
// SPPR601.SPL heal: 5 grey 16d8 7
// SPWI113.SPL missile of patience:  1 grey 16d6 0
// SPWI115.SPL blindness: 5 grey 16d6 3
// SPWI120.SPL vilquar's eye: 5 grey 16d6 3
// SPWI202.SPL black-barbed curse: 5 grey 16d6 3
// SPWI203.SPL black-barbed shield: 5 grey 16d6 3
// SPWI301.SPL ball lightning: 5 grey 16d(-1) 4
// SPWI305.SPL elysium's tears: 1 grey 16d6 0
// SPWI307.SPL hold undead: 5 grey 16d6 5
// SPWI308.SPL tasha's: 5 grey 16d6 12
// SPWI310.SPL ax of torment: 1 grey 16d6 10
// SPWI313.SPL fiery rain: 1 grey 16d6 6
// SPWI406.SPL improved strength: 5 grey 16d(-1) 4
// SPWI408.SPL shroud of shadows: 5 grey 16d(-1) 4
// SPWI502.SPL cone of cold: 5 grey 16d6 10
// SPWI504.SPL enoll eva's: 5 grey 16d6 3
// SPWI506.SPL fire and ice: 1 grey 16d6 0
// SPWI601.SPL antimagic shell: 5 grey 16d2 3
// SPWI602.SPL globe of inv.: 5 grey 16d2 7
// SPWI603.SPL howl of pan.: 5 purple 16d6 10
// SPWI604.SPL chain light. storm: 1 grey 64d6 0
// SPWI701.SPL acid storm: 5 grey 16d6 10
// SPWI704.SPL guardian mantle: 5 grey 16d6 5
// SPWI805.SPL pw blind: 5 grey 16d6 6
// SPWI905.SPL elysium's fires: 1 grey 16d6 6
// SPWI909.SPL pw kill: 5 darkgrey 48d6 10

static int ColorBoundInfringed(const Color& curGlobalTint, const Color& tintMin, const Color& tintMax)
{
	// not the typical luminance calculation the engine used elsewhere (ITU-R BT.601)
	auto zip = [](const Color& c) {
		return c.r * 77 + c.b * 154 + c.g * 25;
	};

	if (zip(curGlobalTint) >= zip(tintMin)) {
		if (zip(curGlobalTint) <= zip(tintMax)) {
			// continue normally
			return 0;
		} else {
			// bound infringed (max)
			return 2;
		}
	} else {
		// bound infringed (min)
		return 1;
	}
}

int fx_tint_screen (Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	Color fadeColor = Color::FromABGR(fx->Parameter1); // usually gray
	ieDword type = fx->Parameter2;
	if (!core->HasFeature(GFFlags::HAS_EE_EFFECTS)) {
		// the game clearly fades to a darker tint than what the light gray most spells use represents
		// remove this block if we switch to actual tinting
		fadeColor = ColorBlack;
		fadeColor.a = 0x80;
	}

	Color tintMin;
	Color tintMax;
	static const Color tintNone{};
	Color step; // the amount the engine adds to each color parameter of the current area tint every AI tick
	if (type & 8) {
		tintMin = ColorBlack;
		tintMax = tintNone;
		step = fadeColor;
		if (type & 1) {
			// step is set to negated param1 components, basically stepping the opposite way
			step *= -1;
		}
	} else {
		// ensure the step will be constant
		Color initialTint;
		if (fx->FirstApply) {
			initialTint = core->GetWindowManager()->FadeColor; // current global lighting
			fx->Parameter4 = fx->Duration - core->GetGame()->GameTime; // estimate amount of steps
			fx->Parameter5 = initialTint.Packed();
			fx->Parameter3 = fx->Parameter4;
		} else {
			initialTint = Color(fx->Parameter5);
		}
		tintMin = initialTint;
		tintMax = fadeColor;
		ieDword div = fx->IsVariable == 0 ? fx->Parameter4 : fx->IsVariable;
		step = (fadeColor - initialTint) / div;
// 		if (ColorBoundInfringed(core->GetWindowManager()->FadeColor + step, tintMin, tintMax) != 0) {
// 			core->GetWindowManager()->FadeColor = tintNone;
// 			return FX_NOT_APPLIED;
// 		}
	}

	// only update the color during the initial fade in
	// NOTE: alpha channel will be overwritten by the fade code
	if (fx->Parameter3 && ColorBoundInfringed(core->GetWindowManager()->FadeColor + step, tintMin, tintMax) == 0) {
		core->GetWindowManager()->FadeColor += step;
		fx->Parameter3--;
	}
	if (fx->FirstApply) core->GetAudioDrv()->PlayRelative(fx->Resource, SFX_CHAN_HITS);

	// only some types actually use duration, the rest are permanent
	tick_t fromTime = core->Time.ai_update_time;
	tick_t toTime = fromTime;

	// NOTE: not really treating them as bits
	// the original only used type 1 and type 5
	switch (type) {
		case 0:
		case 1: // quick fade from light to dark, then quick fade to light
			// order matters here, as SetFadeToColor resets fadeFromCounter
			core->timer.SetFadeToColor(toTime, 2);
			core->timer.SetFadeFromColor(fromTime, 2);
			// we don't have to worry about these bit 1 details:
			// temporary durations maintain starting global lighting on bounds infringement.
			// if duration expires before a bound is infringed, fades back to starting global lighting.
			// permanent duration (1) terminates on bounds infringement.
			return FX_NOT_APPLIED;
		case 2:
		case 3: // quick fade from light to dark, then instant light
			core->timer.SetFadeToColor(toTime, 2);
			core->timer.SetFadeFromColor(1, 2);
			// we don't have to worry about these bit 2 details:
			// temporary durations maintain opposite bound on bounds infringement. (sounds crazy)
			// permanent duration (1) maintains bounds infringement.
			// this mode is permanent and does not terminate on its own, unless the infringed bound
			// happens to be the starting global lighting.
			return FX_NOT_APPLIED;
		case 4:
		case 5:
		case 6:
		case 7: // fade from light to target color, goes for duration of effect, then fade to light
			// extend the duration for some time of the fade-in
			if (fx->FirstApply) {
				core->timer.SetFadeToColor(toTime, 2);
				fx->Parameter6 = static_cast<ieDword>(toTime) + 1;
				fx->Duration += toTime;
			} else if (fx->Parameter6 != 0) {
				fx->Parameter6--;
				fx->Duration++;
			}

			// maintain color after fade-in
			if (!fx->Parameter6) {
				core->GetWindowManager()->FadeColor = fadeColor;
				if (fx->Duration == core->GetGame()->GameTime) core->timer.SetFadeFromColor(fromTime, 2);
			}

			// bit (4) Temporary durations maintain first bound infringement, then fade back to starting global
			// lighting when duration is expired.
			//
			// Permanent duration (1) inverts stepping and fades to the opposite bound on bounds infringement.
			// This mode is permanent and does not terminate on its own.

			// bit (6) Temporary durations maintain first bound infringement, then fade back to starting global
			// lighting when duration is expired.
			//
			// Permanent duration (1) maintains bounds infringement.
			// This mode is permanent and does not terminate on its own, unless the infringed bound
			// happens to be the starting global lighting.
			break;
		case 8: // no effect
			return FX_NOT_APPLIED;
		case 9: // very fast light to black shift and back / Instant inverted 'RGB Colour' for the duration of the effect
			core->GetWindowManager()->FadeColor = ColorBlack;
			core->timer.SetFadeToColor(core->Time.ai_update_time / 2);
			core->timer.SetFadeFromColor(core->Time.ai_update_time / 2);
			return FX_NOT_APPLIED;
		case 10: // instant black for the duration of the effect, then instant light
			core->GetWindowManager()->FadeColor = ColorBlack;
			if (fx->FirstApply) core->timer.SetFadeToColor(1);
			core->timer.SetFadeFromColor(1);
			break;
		case 100: // maintains starting global lighting for the specified duration
			// Bug(?) for 100/101: If param1 is more luminous than the starting global lighting,
			//                     fades to param1 and terminates regardless of timing mode
			// NOTE: both also used fx->IsVariable somehow (beyond the above?)
			core->GetWindowManager()->FadeColor = tintNone;
			break;
		case 101: // instantly set area tint to param1, then fade back to starting global lighting when duration is expired
			core->timer.SetFadeToColor(1);
			// permanent duration does not terminate on its own unless param1 happens to be the starting global lighting
			if (fx->TimingMode == FX_DURATION_INSTANT_PERMANENT && tintMax != tintNone) {
				return FX_NOT_APPLIED;
			}
			// about to expire
			if (fx->Duration == core->GetGame()->GameTime) core->timer.SetFadeFromColor(core->Time.ai_update_time);
			break;
		case 200: // supposed to fade currently active area tint back to its starting global lighting, but just kills it
			core->timer.SetFadeToColor(1);
			core->timer.SetFadeFromColor(1);
			return FX_NOT_APPLIED;
		default:
			Log(ERROR, "PSTOpcodes", "fx_tint_screen: Unknown type passed: {} through {} by {}!", type, fx->SourceRef, fx->CasterID);
			return FX_NOT_APPLIED;
	}

	return FX_APPLIED;
}

//0xc4 fx_special_effect
//it is a mystery, why they needed to make this effect
int fx_special_effect (Scriptable* Owner, Actor* target, Effect* fx)
{
	// print("fx_special_effect(%2d): Par2: %d", fx->Opcode, fx->Parameter2);
	//param2 determines the effect's behaviour
	//0 - adder's kiss projectile (0xcd)
	//  adds play bam and damage opcodes to the projectile
	//1 - ball lightning projectile (0xe0)
	//  adds a play bam opcode to the projectile
	//2 - raise dead projectile - the projectile itself has the effect (why is it so complicated)

	//TODO: create the spells
	switch(fx->Parameter2) {
		case 0:
			fx->Resource = "ADDER";
			break;
		case 1:
			fx->Resource = "BALL";
			break;
		case 2:
			fx->Resource = "RDEAD";
			break;
	}

	ResRef OldSpellResRef = Owner->SpellResRef;

	// flags: no deplete, instant, no interrupt
	Owner->DirectlyCastSpell(target, fx->Resource, fx->CasterLevel, true, false);
	Owner->SetSpellResRef(OldSpellResRef);

	return FX_NOT_APPLIED;
}
//0xc5 fx_multiple_vvc
//this is a gemrb specific opcode to support the rune of torment projectile
//it plays multiple vvc's with a given delay and duration
int fx_multiple_vvc (Scriptable* Owner, Actor* /*target*/, Effect* fx)
{
	// print("fx_multiple_vvc(%2d): Par2: %d", fx->Opcode, fx->Parameter2);

	Map *area = Owner->GetCurrentArea();
	if (!area)
		return FX_NOT_APPLIED;

	VEFObject *vef = gamedata->GetVEFObject(fx->Resource, true);
	if (vef) {
		area->AddVVCell(vef);
	}
/*
	AutoTable tab(fx->Resource);
	if (!tab)
		return FX_NOT_APPLIED;

	int rows = tab->GetRowCount();
	while(rows--) {
		Point offset;
		int delay, duration;

		offset.x=atoi(tab->QueryField(rows,0));
		offset.y=atoi(tab->QueryField(rows,1));
		delay = atoi(tab->QueryField(rows,3));
		duration = atoi(tab->QueryField(rows,4));
		ScriptedAnimation *sca = gamedata->GetScriptedAnimation(tab->QueryField(rows,2), true);
		if (!sca) continue;
		sca->SetBlend();
		sca->SetDelay(AI_UPDATE_TIME*delay);
		sca->SetDefaultDuration(AI_UPDATE_TIME*duration);
		sca->Frame.x+=fx->PosX+offset.x;
		sca->Frame.y+=fx->PosY+offset.y;
		area->AddVVCell(sca);
	}
*/
	return FX_NOT_APPLIED;
}

//0xc6 ChangeBackground
//GemRB specific, to support BMP area background changes (desert hell projectile)
int fx_change_background (Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	// print("fx_change_background(%2d): Par2: %d", fx->Opcode, fx->Parameter2);
	Map *map = core->GetGame()->GetCurrentArea();
	if (map) {
		map->SetBackground(fx->Resource, fx->Duration);
	}
	return FX_NOT_APPLIED;
}

//0xc7-c8 fx_unknown
//0xc9 fx_overlay
static EffectRef fx_armor_ref = { "ACVsDamageTypeModifier", -1 };
static EffectRef fx_breath_ref = { "SaveVsBreathModifier", -1 };
static EffectRef fx_death_ref = { "SaveVsDeathModifier", -1 };
static EffectRef fx_poly_ref = { "SaveVsPolyModifier", -1 };
static EffectRef fx_spell_ref = { "SaveVsSpellsModifier", -1 };
static EffectRef fx_wands_ref = { "SaveVsWandsModifier", -1 };
static EffectRef fx_damage_opcode_ref = { "Damage", -1 };
static EffectRef fx_colorchange_ref = { "Color:SetRGBGlobal", -1 };
static EffectRef fx_colorpulse_ref = { "Color:PulseRGBGlobal", -1 };
static EffectRef fx_single_color_pulse_ref = { "Color:BriefRGB", -1 };
static EffectRef fx_resistfire_ref = { "FireResistanceModifier", -1 };
static EffectRef fx_resistmfire_ref = { "MagicalFireResistanceModifier", -1 };
static EffectRef fx_protection_ref = { "Protection:SpellLevel", -1 };
static EffectRef fx_magicdamage_ref = { "MagicDamageResistanceModifier", -1 };
static EffectRef fx_dispel_ref = { "DispelEffects", -1 };
static EffectRef fx_miscast_ref = { "MiscastMagicModifier", -1 };
static EffectRef fx_set_state_ref = { "SetStatus", -1 };
static EffectRef fx_str_ref = { "StrengthModifier", -1 };

static inline int DamageLastHitter(Effect *fx, Actor *target, int param1, int param2)
{
	if (!fx->Parameter3) {
		return FX_NOT_APPLIED;
	}

	const Map *map = target->GetCurrentArea();
	Actor *actor = map->GetActorByGlobalID(target->LastHitter);
	if (actor && PersonalDistance(target, actor) < 30) {
		const TriggerEntry *entry = target->GetMatchingTrigger(trigger_hitby, TEF_PROCESSED_EFFECTS);
		if (entry) {
			Effect *newFX = EffectQueue::CreateEffect(fx_damage_opcode_ref, param1, param2 << 16, FX_DURATION_INSTANT_PERMANENT);
			newFX->Target = FX_TARGET_PRESET;
			newFX->Power = fx->Power;
			newFX->Source = fx->Source;
			core->ApplyEffect(newFX, actor, target);
			if (fx->Parameter3 != 0xffffffff) {
				fx->Parameter3--;
			}
		}
	}

	if (!fx->Parameter3) {
		return FX_NOT_APPLIED;
	}
	return FX_APPLIED;
}

static inline void ConvertTiming(Effect *fx, int Duration)
{
	// GameTime will be added in by EffectQueue
	fx->Duration = Duration ? Duration * core->Time.ai_update_time : 1;
	if (fx->TimingMode == FX_DURATION_ABSOLUTE) {
		fx->Duration += core->GetGame()->GameTime;
	}
	fx->TimingMode = FX_DURATION_INSTANT_LIMITED;
}

int fx_overlay (Scriptable* Owner, Actor* target, Effect* fx)
{
	// print("fx_overlay(%2d): Par2: %d", fx->Opcode, fx->Parameter2);
	if (!target) {
		return FX_NOT_APPLIED;
	}
	int terminate = FX_APPLIED;
	bool playonce = false;
	ieDword tint = 0;
	Effect *newfx;

	//special effects based on fx_param2
	// PST:EE also exploded this into separate opcodes with more parameters and using a bam/vvc instead of an internal projectile
	if (fx->FirstApply) {
		switch(fx->Parameter2) {
		case 0: //cloak of warding
			ConvertTiming (fx, 5 * fx->CasterLevel);
			fx->Parameter3 = core->Roll(3,4,fx->CasterLevel);
			break;
		case 1: //shield
			ConvertTiming(fx, 25 * fx->CasterLevel);

			target->ApplyEffectCopy(fx, fx_armor_ref, Owner, 3, 16);
			target->ApplyEffectCopy(fx, fx_breath_ref, Owner, 1, 0);
			target->ApplyEffectCopy(fx, fx_death_ref, Owner, 1, 0);
			target->ApplyEffectCopy(fx, fx_poly_ref, Owner, 1, 0);
			target->ApplyEffectCopy(fx, fx_spell_ref, Owner, 1, 0);
			target->ApplyEffectCopy(fx, fx_wands_ref, Owner, 1, 0);

			break;
		case 2: //black barbed shield
			ConvertTiming (fx, core->Roll(10,3,0));
			target->ApplyEffectCopy(fx, fx_armor_ref, Owner, 2, 0);
			fx->Parameter3=0xffffffff;
			break;
		case 3: //pain mirror
			fx->Parameter3 = 1;
			ConvertTiming (fx, 5 * fx->CasterLevel);
			target->ApplyEffectCopy(fx, fx_colorpulse_ref, Owner, 0xFAFF7000, 0x30000C);
			break;
		case 4: //guardian mantle
			ConvertTiming (fx, 50 + 5 * fx->CasterLevel);
			break;
		case 5: //shroud of shadows

			break;
		case 6: //duplication
			core->GetAudioDrv()->Play("magic02", SFX_CHAN_HITS, target->Pos);
			break;
		case 7: //armor
			target->ApplyEffectCopy(fx, fx_colorchange_ref, Owner, 0x825A2800, -1);
			target->ApplyEffectCopy(fx, fx_armor_ref, Owner, 6, 16);
			break;
		case 8: //antimagic shell
			{
				newfx = EffectQueue::CreateEffectCopy(fx, fx_dispel_ref, 100, 0);
				newfx->Power = 10;
				core->ApplyEffect(newfx, target, Owner);

				for (int i = 0; i < 2; i++) {
					target->ApplyEffectCopy(fx, fx_miscast_ref, Owner, 100, i);
				}

				for (int i = 1; i < 10; i++) {
					target->ApplyEffectCopy(fx, fx_protection_ref, Owner, i, 0);
				}
				target->ApplyEffectCopy(fx, fx_magicdamage_ref, Owner, 100, 0);
				target->ApplyEffectCopy(fx, fx_set_state_ref, Owner, 1, STATE_ANTIMAGIC);
			}
			break;
		case 9: case 10: //unused
		default:
			break;
		case 11: //flame walk
			ConvertTiming (fx, 10 * fx->CasterLevel);

			target->ApplyEffectCopy(fx, fx_single_color_pulse_ref, Owner, 0xFF00, 0x400040);

			newfx = EffectQueue::CreateEffectCopy(fx, fx_colorchange_ref, 0x64FA00, 0x50005);
			//wtf is this
			newfx->IsVariable = 0x23;
			core->ApplyEffect(newfx, target, Owner);

			target->ApplyEffectCopy(fx, fx_resistfire_ref, Owner, 50, 1);
			target->ApplyEffectCopy(fx, fx_resistmfire_ref, Owner, 50, 1);
			break;
		case 12: //protection from evil
			ConvertTiming (fx, 10 * fx->CasterLevel);

			target->ApplyEffectCopy(fx, fx_armor_ref, Owner, 2, 0);
			target->ApplyEffectCopy(fx, fx_breath_ref, Owner, 2, 0);
			target->ApplyEffectCopy(fx, fx_death_ref, Owner, 2, 0);
			target->ApplyEffectCopy(fx, fx_poly_ref, Owner, 2, 0);
			target->ApplyEffectCopy(fx, fx_spell_ref, Owner, 2, 0);
			target->ApplyEffectCopy(fx, fx_wands_ref, Owner, 2, 0);
			//terminate = FX_NOT_APPLIED;
			break;
		case 13: //conflagration
			ConvertTiming (fx, 50);
			playonce = true;
			break;
		case 14: //infernal shield
			tint = 0x5EC2FE;
			ConvertTiming (fx, 5 * fx->CasterLevel);

			target->ApplyEffectCopy(fx, fx_resistfire_ref, Owner, 150, 1);
			target->ApplyEffectCopy(fx, fx_resistmfire_ref, Owner, 150, 1);
			break;
		case 15: //submerge the will
			tint = 0x538D90;
			ConvertTiming (fx, 12 * fx->CasterLevel);

			target->ApplyEffectCopy(fx, fx_armor_ref, Owner, 2, 16);
			target->ApplyEffectCopy(fx, fx_breath_ref, Owner, 1, 0);
			target->ApplyEffectCopy(fx, fx_death_ref, Owner, 1, 0);
			target->ApplyEffectCopy(fx, fx_poly_ref, Owner, 1, 0);
			target->ApplyEffectCopy(fx, fx_spell_ref, Owner, 1, 0);
			target->ApplyEffectCopy(fx, fx_wands_ref, Owner, 1, 0);
			break;
		case 16: //balance in all things
			tint = 0x615AB4;
			fx->Parameter3 = fx->CasterLevel/4;
			ConvertTiming (fx, 5 * fx->CasterLevel);

			target->ApplyEffectCopy(fx, fx_colorpulse_ref, Owner, 0x615AB400, 0x30000C);
			playonce = true;
			break;
		case 17: // gemrb extension: strength spells
			// bump strength, but only up to a limit
			// if anyone complains about Improved strength having class based limits, add them to clssplab.2da
			// duration and saving bonus fields are reused; these are eff v1
			ieDword strLimit = fx->Duration - 1;
			int bonus = core->Roll(1, gamedata->GetSpellAbilityDie(target, 1), fx->SavingThrowBonus);
			if (target->Modified[IE_STR] + bonus >= strLimit) {
				bonus = std::max((ieDword) 0, target->Modified[IE_STR] + bonus - strLimit);
			}

			int duration = 60 * core->Time.hour_sec * fx->CasterLevel;
			if (fx->SavingThrowBonus == 1) { // power of one
				duration /= 2;
			} else if (fx->SavingThrowBonus == 4) { // improved strength
				duration = 5 * fx->CasterLevel;
			}
			ConvertTiming (fx, duration);

			// improved strength also has a pulse we need to adjust
			Effect *efx = const_cast<Effect*>(target->fxqueue.HasEffectWithSource(fx_colorpulse_ref, fx->SourceRef)); // FIXME: const_cast
			if (efx) {
				ConvertTiming (efx, duration);
			}

			target->ApplyEffectCopy(fx, fx_str_ref, Owner, bonus, 0);
			playonce = true; // there's no resource set any way
			break;
		}

		if (!target->HasVVCCell(fx->Resource)) {
			ScriptedAnimation *sca = gamedata->GetScriptedAnimation(fx->Resource, true);
			if (sca) {
				if (tint) {
					RGBModifier rgb;

					rgb.speed=-1;
					rgb.phase=0;
					rgb.rgb = Color::FromABGR(tint);
					rgb.type=RGBModifier::TINT;

					sca->AlterPalette(rgb);
				}
				sca->SetBlend();
				if (playonce) {
					sca->PlayOnce();
				} else {
					sca->SetDefaultDuration(fx->Duration-core->GetGame()->Ticks);
				}
				sca->SetBlend();
				sca->SetEffectOwned(true);
				ScriptedAnimation *twin = sca->DetachTwin();
				if (twin) {
					target->AddVVCell(twin);
				}
				target->AddVVCell(sca);
			}
		}
	}

	auto range = target->GetVVCCells(fx->Resource);
	if (range.first == range.second) {
		return FX_NOT_APPLIED;
	}
	
	for (; range.first != range.second; ++range.first) {
		range.first->second->active = true;
	}

	switch(fx->Parameter2) {
	case 0: //cloak of warding
		if (fx->Parameter3<=0) {
			return FX_NOT_APPLIED;
		}
		//flag for removal trigger
		target->Modified[IE_STONESKINS]=1;
		break;
	case 2: //black barbed shield (damage opponents)
		if (target->LastHitter) {
			terminate = DamageLastHitter(fx, target, core->Roll(2, 6, 0), 16);
		}
		break;
	case 3: case 16: //pain mirror or balance in all things
		if (target->LastHitter) {
			terminate = DamageLastHitter(fx, target, target->LastDamage, target->LastDamageType);
		}
		break;
	case 4:
		//this is not the original position, but the immunity stat was already used for things like this
		//as an added benefit, HasImmunityEffects also works with the pst spell
		STAT_BIT_OR( IE_IMMUNITY, IMM_GUARDIAN);
		break;
	case 5:
		break;
	case 6:
		STATE_SET(STATE_EE_DUPL);
		break;
	case 7:
		break;
	case 8: //antimagic shell
		if (!target->HasVVCCell("S061GLWB") ) {
			ScriptedAnimation *sca = gamedata->GetScriptedAnimation("S061GLWB", false);
			if (sca) {
				sca->SetDefaultDuration(fx->Duration-core->GetGame()->Ticks);
				target->AddVVCell(sca);
			}
		}
		break;
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		break;
	default:;
	}
	//PST doesn't keep these effects applied, but we do, because we don't have
	//separate stats for these effects.
	//We just use Parameter3 for the stats where needed
	return terminate;
}
//0xca fx_unknown

//0x82 fx_bless
//static EffectRef fx_glow_ref = { "Color:PulseRGBGlobal", -1 };
//pst bless effect spawns a color glow automatically
//but i would rather use the IWD2 method
int fx_bless (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_curse(%2d): Par1: %d", fx->Opcode, fx->Parameter1);
	//this bit is the same as the invisibility bit in other games
	//it should be considered what if we replace the pst invis bit
	//with this one (losing binary compatibility, gaining easier
	//invis checks at core level)
	if (STATE_GET (STATE_BLESS) ) //curse is non-cumulative
		return FX_NOT_APPLIED;

	target->SetColorMod(255, RGBModifier::ADD, 0x18, Color(0xc8, 0xc8, 0xc8, 0));

	STATE_SET( STATE_BLESS );
	target->ToHit.HandleFxBonus(- signed(fx->Parameter1), fx->TimingMode == FX_DURATION_INSTANT_PERMANENT);
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
	// print("fx_curse(%2d): Par1: %d", fx->Opcode, fx->Parameter1);
	//this bit is the same as the invisibility bit in other games
	//it should be considered what if we replace the pst invis bit
	//with this one (losing binary compatibility, gaining easier
	//invis checks at core level)
	if (STATE_GET (STATE_PST_CURSE) ) //curse is non cumulative
		return FX_NOT_APPLIED;
	STATE_SET( STATE_PST_CURSE );
	target->ToHit.HandleFxBonus(- signed(fx->Parameter1), fx->TimingMode == FX_DURATION_INSTANT_PERMANENT);
	STAT_SUB( IE_SAVEVSDEATH, fx->Parameter1);
	STAT_SUB( IE_SAVEVSWANDS, fx->Parameter1);
	STAT_SUB( IE_SAVEVSPOLY, fx->Parameter1);
	STAT_SUB( IE_SAVEVSBREATH, fx->Parameter1);
	STAT_SUB( IE_SAVEVSSPELL, fx->Parameter1);
	return FX_APPLIED;
}

//0xcc fx_prayer
static EffectRef fx_curse_ref = { "Curse", -1 };
static EffectRef fx_bless_ref = { "Bless", -1 };

int fx_prayer (Scriptable* Owner, Actor* target, Effect* fx)
{
	// print("fx_prayer(%2d): Par1: %d", fx->Opcode, fx->Parameter1);
	int ea = target->GetStat(IE_EA);
	int type;
	if (ea>EA_EVILCUTOFF) type = 1;
	else if (ea<EA_GOODCUTOFF) type = 0;
	else return FX_NOT_APPLIED;  //what happens if the target goes neutral during the effect? if the effect remains, make this FX_APPLIED

	const Map *map = target->GetCurrentArea();
	int i = map->GetActorCount(true);
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
		Effect *newfx = EffectQueue::CreateEffect(type?fx_curse_ref:fx_bless_ref, fx->Parameter1, fx->Parameter2, FX_DURATION_INSTANT_LIMITED);
		newfx->Source = fx->Source;
		newfx->Duration = 60;
		core->ApplyEffect(newfx, tar, Owner);
	}
	return FX_APPLIED;
}

//0xcd fx_move_view
int fx_move_view (Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	// print("fx_move_view(%2d): Speed: %d", fx->Opcode, fx->Parameter1);
	const Map *map = core->GetGame()->GetCurrentArea();
	if (map) {
		core->timer.SetMoveViewPort(fx->Pos, fx->Parameter1, true);
	}
	return FX_NOT_APPLIED;
}

//0xce fx_embalm
int fx_embalm (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_embalm(%2d): Par2: %d", fx->Opcode, fx->Parameter2);
	if (STATE_GET (STATE_EMBALM) ) //embalm is non cumulative
		return FX_NOT_APPLIED;
	STATE_SET( STATE_EMBALM );
	if (!fx->Parameter1) {
		if (fx->Parameter2) {
			fx->Parameter1=fx->CasterLevel*2;
		} else {
			fx->Parameter1=core->Roll(1,6,1);
		}
	}
	STAT_ADD( IE_MAXHITPOINTS, fx->Parameter1);
	BASE_ADD( IE_HITPOINTS, fx->Parameter1 );
	if (fx->Parameter2) {
		target->AC.HandleFxBonus(2, fx->TimingMode==FX_DURATION_INSTANT_PERMANENT);
	} else {
		target->AC.HandleFxBonus(1, fx->TimingMode==FX_DURATION_INSTANT_PERMANENT);
	}
	return FX_APPLIED;
}

//0xcf fx_stop_all_action
// FIXME: supposedly was (closer to) cutscene mode; recheck with spwi308 or any other users
int fx_stop_all_action (Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	if (fx->Parameter2) {
		core->GetGame()->TimeStop(nullptr, 0);
	} else {
		core->GetGame()->TimeStop(nullptr, 0xffffffff);
	}
	return FX_NOT_APPLIED;
}

//0xd0 fx_iron_fist
//GemRB extension: lets you specify not hardcoded values
int fx_iron_fist (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	ieDword toHitBonus = 3;
	ieDword damageBonus = 6;
	if (fx->Parameter2 != 0) {
		toHitBonus = fx->Parameter1 & 0xffff;
		damageBonus = fx->Parameter1 >> 16;
	}
	STAT_ADD(IE_FISTHIT, toHitBonus);
	STAT_ADD(IE_FISTDAMAGE, damageBonus);
	return FX_APPLIED;
}

//0xd1 fx_hostile_image (Spell Effect: Soul Exodus)
int fx_hostile_image(Scriptable* /*Owner*/, Actor* /*target*/, Effect* fx)
{
	Log(ERROR, "PSTOpcodes","fx_hostile_image: not implemented! Source: {}", fx->Source);
	return FX_NOT_APPLIED;
}

//0xd2 fx_detect_evil
int fx_detect_evil (Scriptable* Owner, Actor* target, Effect* fx)
{
	// print("fx_detect_evil(%2d): Par1: %d Par2: %d", fx->Opcode, fx->Parameter1, fx->Parameter2);
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
		
		EffectQueue fxqueue;
		fxqueue.SetOwner(Owner);
		fxqueue.AddEffect(newfx);

		//don't detect self? if yes, then use NULL as last parameter
		fxqueue.AffectAllInRange(target->GetCurrentArea(), target->Pos, (type&0xff000000)>>24, (type&0xff0000)>>16, (type&0xff)*10, target);
	}
	return FX_APPLIED;
}

//0xd3 fx_jumble_curse
int fx_jumble_curse (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_jumble_curse(%2d)", fx->Opcode);

	if (STATE_GET( STATE_DEAD) ) {
		return FX_NOT_APPLIED;
	}
	const Game *game = core->GetGame();
	//do a hiccup every 75th refresh
	if (fx->Parameter3/75!=fx->Parameter4/75) {
		//hiccups
		//PST has this hardcoded deep in the engine
		//gemrb lets you specify the strref in P#1
		ieStrRef tmp = ieStrRef(fx->Parameter1);
		if (!tmp) tmp = ieStrRef::PST_HICCUP;
		String tmpstr = core->GetString(tmp, STRING_FLAGS::SPEECH | STRING_FLAGS::SOUND);
		target->overHead.SetText(std::move(tmpstr));
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

//0xd4 fx_speak_with_dead
//This opcode is directly employed by the speak with dead projectile in the original engine
//In GemRB it is used in a custom spell
int fx_speak_with_dead (Scriptable* /*Owner*/, Actor* target, Effect* fx)
{
	// print("fx_speak_with_dead(%2d)", fx->Opcode);
	if (!STATE_GET( STATE_DEAD) ) {
		return FX_NOT_APPLIED;
	}
	if (fx->FirstApply) fx->Parameter4 = fx->Duration - core->GetGame()->GameTime;

	if (fx->Parameter4 == 1) {
		SetVariable(target, "Speak_with_Dead", 0, "GLOBAL");
	}
	fx->Parameter4--;
	return FX_NOT_APPLIED;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x115A670, "Effect opcodes for the torment branch of the games")
PLUGIN_INITIALIZER(RegisterTormentOpcodes)
END_PLUGIN()
