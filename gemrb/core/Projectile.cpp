/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2006 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "Projectile.h"

#include "AnimationFactory.h"
#include "DisplayMessage.h"
#include "Game.h"
#include "GameData.h"
#include "GlobalTimer.h"
#include "Interface.h"
#include "ProjectileServer.h"
#include "Sprite2D.h"
#include "VEFObject.h"
#include "RNG.h"
#include "Scriptable/Actor.h"
#include "ScriptedAnimation.h"

#include <cstdlib>

namespace GemRB {

constexpr uint8_t PALSIZE = 32;

static const ieByte SixteenToNine[MAX_ORIENT]={0,1,2,3,4,5,6,7,8,7,6,5,4,3,2,1};
static const ieByte SixteenToFive[MAX_ORIENT]={0,1,2,3,4,3,2,1,0,1,2,3,4,3,2,1};

static ProjectileServer *server = NULL;

Projectile::Projectile()
{
	area = NULL;
	palette = NULL;
	Destination = Pos;
	Orientation = 0;
	NewOrientation = 0;
	path = NULL;
	step = NULL;
	timeStartStep = 0;
	phase = P_UNINITED;
	effects = NULL;
	children = NULL;
	child_size = 0;
	memset(travel, 0, sizeof(travel)) ;
	memset(shadow, 0, sizeof(shadow)) ;
	light = NULL;
	pathcounter = 0x7fff;
	FakeTarget = 0;
	bend = 0;
	drawSpark = 0;
	ZPos = 0;
	extension_delay = 0;
	Range = 0;
	ColorSpeed = Shake = TFlags = Seq1 = Seq2 = ExtFlags = 0;
	IDSType = IDSValue = IDSType2 = IDSValue2 = StrRef = 0;
	LightX = LightY = LightZ = Aim = type = SparkColor = 0;
	SmokeSpeed = SmokeAnimID = Caster = Target = Level = 0;
	extension_explosioncount = extension_targetcount = 0;
	Speed = 20;
	SFlags = PSF_FLYING;
	if (!server)
		server = core->GetProjectileServer();
}

Projectile::~Projectile()
{
	delete effects;

	ClearPath();

	if (travel_handle) {
		//allow an explosion sound to finish completely
		travel_handle->StopLooping();
	}

	if (phase != P_UNINITED) {
		for (int i = 0; i < MAX_ORIENT; ++i) {
			delete travel[i];
			delete shadow[i];
		}
	}

	if(children) {
		for (int i = 0; i < child_size; i++) {
			delete children[i];
		}
		free (children);
	}
}

void Projectile::CreateAnimations(Animation **anims, const ResRef& bamres, int Seq)
{
	AnimationFactory* af = ( AnimationFactory* )
		gamedata->GetFactoryResource(bamres, IE_BAM_CLASS_ID);

	if (!af) {
		return;
	}

	size_t Max = af->GetCycleCount();
	if (!Max) {
		return;
	}

	if((ExtFlags&PEF_CYCLE) && !Seq) {
		Seq = static_cast<int>(RAND(0UL, Max - 1));
	}

	//this hack is needed because bioware .pro files are sometimes
	//reporting bigger face count than possible by the animation
	if (Aim>Max) Aim=Max;

	if(ExtFlags&PEF_PILLAR) {
		CreateCompositeAnimation(anims, af, Seq);
	} else {
		CreateOrientedAnimations(anims, af, Seq);
	}
}

//Seq is the first cycle to use in the composite
//Aim is the number of cycles
void Projectile::CreateCompositeAnimation(Animation **anims, AnimationFactory *af, int Seq) const
{
	for (int Cycle = 0; Cycle<Aim; Cycle++) {
		int c = Cycle+Seq;
		Animation* a = af->GetCycle( c );
		anims[Cycle] = a;
		if (!a) continue;
		//animations are started at a random frame position
		//Always start from 0, unless set otherwise
		if (!(ExtFlags&PEF_RANDOM)) {
			a->SetFrame(0);
		}

		a->gameAnimation = true;
	}
}

//Seq is the cycle to use in case of single orientations
//Aim is the number of Orientations
// FIXME: seems inefficient that we load up MAX_ORIENT animations even for those with a single orientation (default case)
void Projectile::CreateOrientedAnimations(Animation **anims, AnimationFactory *af, int Seq) const
{
	for (int Cycle = 0; Cycle<MAX_ORIENT; Cycle++) {
		bool mirror = false, mirrorvert = false;
		int c;
		switch(Aim) {
		default:
			c = Seq;
			break;
		case 5:
			c = SixteenToFive[Cycle];
			// orientations go counter-clockwise, starting south
			if (Cycle <= 4) {
				// bottom-right quadrant
				mirror = false; mirrorvert = false;
			} else if (Cycle <= 8) {
				// top-right quadrant
				mirror = false; mirrorvert = true;
			} else if (Cycle < 12) {
				// top-left quadrant
				mirror = true; mirrorvert = true;
			} else {
				// bottom-left quadrant
				mirror = true; mirrorvert = false;
			}
			break;
		case 9:
			c = SixteenToNine[Cycle];
			if (Cycle>8) mirror=true;
			break;
		case 16:
			c=Cycle;
			break;
		}
		Animation* a = af->GetCycle( c );
		anims[Cycle] = a;
		if (!a) continue;
		//animations are started at a random frame position
		//Always start from 0, unless set otherwise
		if (!(ExtFlags&PEF_RANDOM)) {
			a->SetFrame(0);
		}

		if (mirror) {
			a->MirrorAnimation();
		}
		if (mirrorvert) {
			a->MirrorAnimationVert();
		}
		a->gameAnimation = true;
	}
}

//apply gradient colors
void Projectile::SetupPalette(Animation *anim[], PaletteHolder &pal, const ieByte *gradients)
{
	ieDword Colors[7];

	for (int i=0;i<7;i++) {
		Colors[i]=gradients[i];
	}
	GetPaletteCopy(anim, pal);
	if (pal) {
		pal->SetupPaperdollColours(Colors, 0);
	}
}

void Projectile::GetPaletteCopy(Animation *anim[], PaletteHolder &pal)
{
	if (pal)
		return;
	for (unsigned int i=0;i<MAX_ORIENT;i++) {
		if (anim[i]) {
			Holder<Sprite2D> spr = anim[i]->GetFrame(0);
			if (spr) {
				pal = spr->GetPalette()->Copy();
				break;
			}
		}
	}
}

void Projectile::SetBlend(int brighten)
{
	GetPaletteCopy(travel, palette);
	if (!palette)
		return;
	if (!palette->HasAlpha()) {
		palette->CreateShadedAlphaChannel();
	}
	if (brighten) {
		palette->Brighten();
	}
}

//create another projectile with type-1 (iterate magic missiles and call lightning)
void Projectile::CreateIteration()
{
	Projectile *pro = server->GetProjectileByIndex(type-1);
	pro->SetEffectsCopy(effects, Pos);
	pro->SetCaster(Caster, Level);
	if (ExtFlags&PEF_CURVE) {
		pro->bend=bend+1;
		pro->Speed = Speed; // fix the different speed of MAGICMIS.pro compared to SPMAGMIS.pro
	}

	if (FakeTarget) {
		area->AddProjectile(pro, Pos, FakeTarget, true);
	} else {
		area->AddProjectile(pro, Pos, Target, false);
	}

	// added by fuzzie, to make magic missiles instant, maybe wrong place
	pro->Setup();
}

void Projectile::GetSmokeAnim()
{
	int AvatarsRowNum=CharAnimations::GetAvatarsCount();

	SmokeAnimID&=0xfff0; //this is a hack, i'm too lazy to figure out the subtypes

	for(int i=0;i<AvatarsRowNum;i++) {
		AvatarStruct *as = CharAnimations::GetAvatarStruct(i);
		if (as->AnimID==SmokeAnimID) {
			smokebam = as->Prefixes[0];
			return;
		}
	}
	//turn off smoke animation if its animation was not found
	//you might want to issue some warning here
	TFlags&=PTF_SMOKE;
}
// load animations, start sound
void Projectile::Setup()
{
	tint.r=128;
	tint.g=128;
	tint.b=128;
	tint.a=255;

	ieDword time = core->GetGame()->Ticks;
	timeStartStep = time;

	if(ExtFlags&PEF_TEXT) {
		Actor *act = area->GetActorByGlobalID(Caster);
		if(act) {
			displaymsg->DisplayStringName(StrRef, DMC_LIGHTGREY, act,0);
		}
	}

	//falling = vertical
	//incoming = right side
	//both = left side
	if (ExtFlags & (PEF_FALLING|PEF_INCOMING)) {
		Pos.x = Destination.x;
		if (ExtFlags&PEF_INCOMING) {
			if (ExtFlags&PEF_FALLING) {
				Pos.x -= 200;
			} else {
				Pos.x += 200;
			}
		}
		Pos.y = Destination.y - 200;
		NextTarget(Destination);
	}

	if(ExtFlags&PEF_WALL) {
		SetupWall();
	}

	//cone area of effect always disables the travel flag
	//but also makes the caster immune to the effect
	if (Extension) {
		if (Extension->AFlags&PAF_CONE) {
			NewOrientation = Orientation = GetOrient(Destination, Pos);
			Destination=Pos;
			ExtFlags|=PEF_NO_TRAVEL;
		}

		//this flag says the first explosion is delayed
		//(works for delaying triggers too)
		//getting the explosion count here, so an absent caster won't cut short
		//on the explosion count
		if(Extension->AFlags&PAF_DELAY) {
			extension_delay=Extension->Delay;
		} else {
			extension_delay=0;
		}
		extension_explosioncount=CalculateExplosionCount();
	}

	//set any static tint
	if(ExtFlags&PEF_TINT) {
		uint8_t idx = PALSIZE/2;
		const auto& pal32 = core->GetPalette32(Gradients[0]);
		const Color& tmpColor = pal32[idx];
		// PALSIZE is usually 12, but pst has it at 32, which is now the default, so make sure we're not trying to read an empty (black) entry

		if (tmpColor.r + tmpColor.g + tmpColor.b == 0) idx = 6;
		StaticTint(pal32[idx]);
	}

	CreateAnimations(travel, BAMRes1, Seq1);

	if (TFlags&PTF_SHADOW) {
		CreateAnimations(shadow, BAMRes2, Seq2);
	}

	if (TFlags&PTF_SMOKE) {
		GetSmokeAnim();
	}

	//there is no travel phase, create the projectile right at the target
	if (ExtFlags&PEF_NO_TRAVEL) {
		Pos = Destination;

		//the travel projectile should linger after explosion
		if(ExtFlags&PEF_POP) {
			//the explosion consists of a pop in/hold/pop out of the travel projectile (dimension door)
			if(travel[0] && shadow[0]) {
				extension_delay = travel[0]->GetFrameCount()*2+shadow[0]->GetFrameCount();
				//SetDelay( travel[0]->GetFrameCount()*2+shadow[0]->GetFrameCount());
				travel[0]->Flags|=A_ANI_PLAYONCE;
				shadow[0]->Flags|=A_ANI_PLAYONCE;
			}
		} else {
			if(travel[0]) {
				extension_delay = travel[0]->GetFrameCount();
				travel[0]->Flags|=A_ANI_PLAYONCE;
				//SetDelay(travel[0]->GetFrameCount() );
			}
		}
	}

	if (TFlags&PTF_COLOUR) {
		SetupPalette(travel, palette, Gradients);
	} else {
		palette = gamedata->GetPalette(PaletteRes);
	}

	if (TFlags&PTF_LIGHT) {
		light = core->GetVideoDriver()->CreateLight(LightX, LightZ);
	}
	if (TFlags&PTF_TRANS) {
		SetBlend(TFlags&PTF_BRIGHTEN);
	}
	if (SFlags&PSF_FLYING) {
		ZPos = FLY_HEIGHT;
	}
	phase = P_TRAVEL;
	travel_handle = core->GetAudioDrv()->Play(FiringSound, SFX_CHAN_MISSILE,
				Pos, (SFlags & PSF_LOOPING ? GEM_SND_LOOPING : 0));

	//create more projectiles
	if(ExtFlags&PEF_ITERATION) {
		CreateIteration();
	}
}

Actor *Projectile::GetTarget()
{
	Actor *target;

	if (Target) {
		target = area->GetActorByGlobalID(Target);
		if (!target) return NULL;
		Actor *original = area->GetActorByGlobalID(Caster);
		if (!effects) {
			return target;
		}
		if (original == target && !effects->HasHostileEffects()) {
			effects->SetOwner(target);
			return target;
		}

		int res = effects->CheckImmunity ( target );
		//resisted
		if (!res) {
			return NULL;
		}
		if (res==-1) {
			if (original) {
				Target = original->GetGlobalID();
				target = original;
			} else {
				Log(DEBUG, "Projectile", "GetTarget: caster not found, bailing out!");
				return NULL;
			}
		}
		effects->SetOwner(original);
		return target;
	} else {
		Log(DEBUG, "Projectile", "GetTarget: Target not set or dummy, using caster!");
	}
	target = area->GetActorByGlobalID(Caster);
	if (target) {
		effects->SetOwner(target);
	}
	return target;
}

void Projectile::SetDelay(int delay)
{
	extension_delay=delay;
	ExtFlags|=PEF_FREEZE;
}

//copied from Actor.cpp
#define ATTACKROLL    20
#define WEAPON_FIST        0
#define WEAPON_BYPASS      0x10000

bool Projectile::FailedIDS(const Actor *target) const
{
	bool fail = !EffectQueue::match_ids( target, IDSType, IDSValue);
	if (ExtFlags&PEF_NOTIDS) {
		fail = !fail;
	}
	if (ExtFlags&PEF_BOTH) {
		if (!fail) {
			fail = !EffectQueue::match_ids( target, IDSType2, IDSValue2);
			if (ExtFlags&PEF_NOTIDS2) {
				fail = !fail;
			}
		}
	}
	else
	{
		if (fail && IDSType2) {
			fail = !EffectQueue::match_ids( target, IDSType2, IDSValue2);
			if (ExtFlags&PEF_NOTIDS2) {
				fail = !fail;
			}
		}
	}

	if (!fail) {
		if(ExtFlags&PEF_TOUCH) {
			Actor *caster = core->GetGame()->GetActorByGlobalID(Caster);
			if (caster) {
				//TODO move this to Actor
				//TODO some projectiles use melee attack (fist), others use projectile attack
				//this apparently depends on the spell's SpellForm (normal vs. projectile)
				int roll = caster->LuckyRoll(1, ATTACKROLL, 0);
				if (roll==1) {
					return true; //critical failure
				}
				
				if (!(target->GetStat(IE_STATE_ID)&STATE_CRIT_PROT))  {
					if (roll >= (ATTACKROLL - (int) caster->GetStat(IE_CRITICALHITBONUS))) {
						return false; //critical success
					}
				}

				//handle attack type here, weapon depends on it too?
				int tohit = caster->GetToHit(WEAPON_FIST, target);
				//damage type, should be generic?
				// ignore the armor bonus
				int defense = target->GetDefense(0, WEAPON_BYPASS, caster);
				if(target->IsReverseToHit()) {
					fail = roll + defense < tohit;
				} else {
					fail = tohit + roll < defense;
				}        
			}
		}
	}

	return fail;
}

void Projectile::Payload()
{
	if(Shake) {
		core->timer.SetScreenShake(Point(Shake, Shake), Shake);
		Shake = 0;
	}

	//allow area affecting projectile with a spell
	if (!(effects || !successSpell.IsEmpty() || (!Target && !failureSpell.IsEmpty()))) {
		return;
	}

	// PEF_CONTINUE never has a Target (LightningBolt)
	// if we were to try to get one we would end up damaging
	// either the caster, or the original target of the spell
	// which are probably both nowhere near the projectile at this point
	// all effects are applied as the projectile travels
	if (ExtFlags & PEF_CONTINUE) {
		delete effects;
		effects = NULL;
		return;
	}

	Actor *target;
	Scriptable *Owner;

	if (Target) {
		target = GetTarget();
	} else {
		//the target will be the original caster
		//in case of single point area target (dimension door)
		if (FakeTarget) {
			target = area->GetActorByGlobalID(FakeTarget);
			if (!target) {
				target = core->GetGame()->GetActorByGlobalID(FakeTarget);
			}
		} else {
			target = area->GetActorByGlobalID(Caster);
		}
	}

	if (target) {
		Owner = area->GetScriptableByGlobalID(Caster);
		if (!Owner) {
			Log(WARNING, "Projectile", "Payload: Caster not found, using target!");
			Owner = target;
		}
		//apply this spell on target when the projectile fails
		if (FailedIDS(target)) {
			if (!failureSpell.IsEmpty()) {
				if (Target) {
					core->ApplySpell(failureSpell, target, Owner, Level);
				} else {
					//no Target, using the fake target as owner
					core->ApplySpellPoint(failureSpell, area, Destination, target, Level);
				}
			}
		} else {
			//apply this spell on the target when the projectile succeeds
			if (!successSpell.IsEmpty()) {
				core->ApplySpell(successSpell, target, Owner, Level);
			}

			if(ExtFlags&PEF_RGB) {
				target->SetColorMod(0xff, RGBModifier::ADD, ColorSpeed, RGB);
			}

			if (effects) {
				effects->SetOwner(Owner);
				effects->AddAllEffects(target, Destination);
			}
		}
	}

	delete effects;
	effects = NULL;
}

void Projectile::ApplyDefault() const
{
	Actor *actor = area->GetActorByGlobalID(Caster);
	if (actor) {
		//name is the projectile's name
		//for simplicity, we apply a spell of the same name
		core->ApplySpell(projectileName, actor, actor, Level);
	}
}

void Projectile::StopSound()
{
	if (travel_handle) {
		travel_handle->Stop();
		travel_handle.release();
	}
}

void Projectile::UpdateSound()
{
	if (!(SFlags&PSF_SOUND2)) {
		StopSound();
	}
	if (!travel_handle || !travel_handle->Playing()) {
		travel_handle = core->GetAudioDrv()->Play(ArrivalSound, SFX_CHAN_MISSILE,
				Pos, (SFlags & PSF_LOOPING2 ? GEM_SND_LOOPING : 0));
		SFlags|=PSF_SOUND2;
	}
}

//control the phase change when the projectile reached its target
//possible actions: vanish, hover over point, explode
//depends on the area extension
//play explosion sound
void Projectile::ChangePhase()
{
	if (Target) {
		Actor *target = area->GetActorByGlobalID(Target);
		if (!target) {
			phase = P_EXPIRED;
			return;
		}
	}

	if (phase == P_TRAVEL) {
		if ((ExtFlags&PEF_DELAY) && extension_delay) {
			 extension_delay--;
			 UpdateSound();
			 return;
		}
	}

	//reached target, and explodes now
	if (!Extension) {
		//there are no-effect projectiles, like missed arrows
		//Payload can redirect the projectile in case of projectile reflection
		if (phase == P_TRAVEL) {
			if(ExtFlags&PEF_DEFSPELL) {
				ApplyDefault();
			}
			StopSound();
			Payload();
			phase = P_TRAVEL2;
		}
		//freeze on target, this is recommended only for child projectiles
		//as the projectile won't go away on its own
		if(ExtFlags&PEF_FREEZE) {
			if(extension_delay) {
				if (extension_delay>0) {
					extension_delay--;
					UpdateSound();
				}
				return;
			}
		}

		if (phase == P_TRAVEL2) {
			if (extension_delay) {
				 extension_delay--;
				 return;
			}
		}

		if(ExtFlags&PEF_FADE) {
			TFlags &= ~PTF_TINT; //turn off area tint
			tint.a--;
			if(tint.a>0) {
				return;
			}
		}
	}

	EndTravel();
}

//Call this only if Extension exists!
int Projectile::CalculateExplosionCount() const
{
	int count = 0;
	const Actor *act = area->GetActorByGlobalID(Caster);
	if (act) {
		if (Extension->AFlags&PAF_LEV_MAGE) {
			count = static_cast<int>(act->GetMageLevel());
		} else if (Extension->AFlags&PAF_LEV_CLERIC) {
			count = static_cast<int>(act->GetClericLevel());
		}
	}

	if (!count) {
		count = std::max<int>(1, Extension->ExplosionCount);
	}
	return count;
}

void Projectile::EndTravel()
{
	StopSound();
	UpdateSound();
	if(!Extension) {
		phase = P_EXPIRED;
		return;
	}

	//this flag says that the explosion should occur only when triggered
	if (Extension->AFlags&PAF_TRIGGER) {
		phase = P_TRIGGER;
		return;
	} else {
		phase = P_EXPLODING1;
	}
}

//Note: trails couldn't be higher than VVC, but this shouldn't be a problem
int Projectile::AddTrail(const ResRef& BAM, const ieByte *pal) const
{
/*
	VEFObject *vef=gamedata->GetVEFObject(BAM,0);
	if (!vef) return 0;
	ScriptedAnimation *sca=vef->GetSingleObject();
	if (!sca) return 0;
*/
	ScriptedAnimation* sca = gamedata->GetScriptedAnimation(BAM, false);
	if (!sca) return 0;
	VEFObject *vef = new VEFObject(sca);

	if(pal) {
		if (ExtFlags & PEF_TINT) {
			const auto& pal32 = core->GetPalette32( pal[0] );
			sca->Tint = pal32[PALSIZE/2];
			sca->Transparency |= BlitFlags::COLOR_MOD;
		} else {
			for(int i=0;i<7;i++) {
				sca->SetPalette(pal[i], 4+i*PALSIZE);
			}
		}
	}
	sca->SetOrientation(Orientation);
	sca->PlayOnce();
	sca->SetBlend();
	sca->Pos = Pos;
	area->AddVVCell(vef);
	return sca->GetSequenceDuration(AI_UPDATE_TIME);
}

void Projectile::DoStep(unsigned int walk_speed)
{
	if(pathcounter) {
		pathcounter--;
	} else {
		ClearPath();
	}

	//intro trailing, drawn only once at the beginning
	if (pathcounter==0x7ffe) {
		for(int i=0;i<3;i++) {
			if (!TrailSpeed[i] && !TrailBAM[i].IsEmpty()) {
				extension_delay = AddTrail(TrailBAM[i], (ExtFlags&PEF_TINT)?Gradients:NULL);
			}
		}
	}

	if (!path) {
		ChangePhase();
		return;
	}

	if (Pos==Destination) {
		ClearPath();
		ChangePhase();
		return;
	}

	//don't bug out on 0 smoke frequency like the original IE
	if ((TFlags&PTF_SMOKE) && SmokeSpeed) {
		if(!(pathcounter%SmokeSpeed)) {
			AddTrail(smokebam, SmokeGrad);
		}
	}

	for(int i=0;i<3;i++) {
		if(TrailSpeed[i] && !(pathcounter%TrailSpeed[i])) {
			AddTrail(TrailBAM[i], (ExtFlags&PEF_TINT)?Gradients:NULL);
		}
	}

	if (ExtFlags&PEF_LINE) {
		if(Extension) {
			//transform into an explosive line
			EndTravel();
		} else {
			if(!(ExtFlags&PEF_FREEZE) && travel[0]) {
				//switch to 'fading' phase
				//SetDelay(travel[0]->GetFrameCount());
				SetDelay(100);
			}
			ChangePhase();
		}
		//don't change position
		return;
	}

	//path won't be calculated if speed==0
	walk_speed=1500/walk_speed;
	ieDword time = core->GetGame()->Ticks;
	if (!step) {
		step = path;
	}

	const PathNode *start = step;
	while (step->Next && (( time - timeStartStep ) >= walk_speed)) {
		unsigned int count = Speed;
		while (step->Next && count) {
			step = step->Next;
			--count;
		}
		timeStartStep = timeStartStep + walk_speed;

		if (!walk_speed) {
			break;
		}
	}

	if (ExtFlags & PEF_CONTINUE) {
		// check for every step along the way
		// the test case is lightning bolt, its a long projectile,
		LineTarget(start, step->Next);
	}

	SetOrientation (step->orient, false);

	Pos.x=step->x;
	Pos.y=step->y;
	if (travel_handle) {
		travel_handle->SetPos(Pos);
	}
	if (!step->Next) {
		ClearPath();
		NewOrientation = Orientation;
		ChangePhase();
		return;
	}
	if (!walk_speed) {
		return;
	}

	if (SFlags&PSF_SPARKS) {
		drawSpark = 1;
	}

	if (step->Next->x > step->x)
		Pos.x += ((step->Next->x - Pos.x) * (time - timeStartStep) / walk_speed);
	else
		Pos.x -= ((Pos.x - step->Next->x) * (time - timeStartStep) / walk_speed);
	if (step->Next->y > step->y)
		Pos.y += ((step->Next->y - Pos.y) * (time - timeStartStep) / walk_speed);
	else
		Pos.y -= ((Pos.y - step->Next->y) * (time - timeStartStep) / walk_speed);

}

void Projectile::SetCaster(ieDword caster, int level)
{
	Caster=caster;
	Level=level;
}

ieDword Projectile::GetCaster() const
{
	return Caster;
}

void Projectile::NextTarget(const Point &p)
{
	ClearPath();
	Destination = p;
	if (!Speed) {
		Pos = Destination;
		return;
	}
	NewOrientation = Orientation = GetOrient(Destination, Pos);

	//this hack ensures that the projectile will go away after its time
	//by the time it reaches this part, it was already expired, so Target
	//needs to be cleared.
	if(ExtFlags&PEF_NO_TRAVEL) {
		Target = 0;
		Destination = Pos;
		return;
	}

	int flags = (ExtFlags&PEF_BOUNCE) ? GL_REBOUND : GL_PASS;
	int stepping = (ExtFlags & PEF_LINE) ? Speed : 1;
	path = area->GetLine(Pos, Destination, stepping, Orientation, flags);
}

void Projectile::SetTarget(const Point &p)
{
	Target = 0;
	NextTarget(p);
}

void Projectile::SetTarget(ieDword tar, bool fake)
{
	Actor *target = NULL;

	if (fake) {
		Target = 0;
		FakeTarget = tar;
		return;
	} else {
		Target = tar;
		target = area->GetActorByGlobalID(tar);
	}
	 
	if (!target) {
		phase = P_EXPIRED;
		return;
	}

	if (ExtFlags&PEF_CONTINUE) {
		const Point& A = Origin;
		const Point& B = target->Pos;
		double angle = AngleFromPoints(B, A);
		double adjustedRange = Feet2Pixels(Range, angle);
		Point C(A.x + adjustedRange * cos(angle), A.y + adjustedRange * sin(angle));
		SetTarget(C);
	} else {
		//replan the path in case the target moved
		if(target->Pos!=Destination) {
			NextTarget(target->Pos);
			return;
		}

		//replan the path in case the source moved (only for line projectiles)
		if(ExtFlags&PEF_LINE) {
			Actor *c = area->GetActorByGlobalID(Caster);
			if(c && c->Pos!=Pos) {
				Pos=c->Pos;
				NextTarget(target->Pos);
			}
		}
	}
}

void Projectile::MoveTo(Map *map, const Point &Des)
{
	area = map;
	Origin = Des;
	Pos = Des;
	Destination = Des;
}

void Projectile::ClearPath()
{
	PathNode* thisNode = path;
	while (thisNode) {
		PathNode* nextNode = thisNode->Next;
		delete( thisNode );
		thisNode = nextNode;
	}
	path = NULL;
	step = NULL;
}

int Projectile::CalculateTargetFlag() const
{
	//if there are any, then change phase to exploding
	int flags = GA_NO_DEAD|GA_NO_UNSCHEDULED;
	bool checkingEA = false;

	if (Extension) {
		if (Extension->AFlags&PAF_NO_WALL) {
			flags|=GA_NO_LOS;
		}

		//projectiles don't affect dead/inanimate normally
		if (Extension->AFlags&PAF_INANIMATE) {
			flags&=~GA_NO_DEAD;
		}

		//affect only enemies or allies
		switch (Extension->AFlags&PAF_TARGET) {
		case PAF_ENEMY:
			flags|=GA_NO_NEUTRAL|GA_NO_ALLY;
			break;
		case PAF_PARTY: //this doesn't exist in IE
			flags|=GA_NO_ENEMY;
			break;
		case PAF_TARGET:
			flags|=GA_NO_NEUTRAL|GA_NO_ENEMY;
			break;
		default:
			return flags;
		}
		if (Extension->AFlags & PAF_TARGET) {
			checkingEA = true;
		}

		//this is the only way to affect neutrals and enemies
		if (Extension->APFlags&APF_INVERT_TARGET) {
			flags^=(GA_NO_ALLY|GA_NO_ENEMY);
		}
	}

	const Scriptable *caster = area->GetScriptableByGlobalID(Caster);
	const Actor *act = nullptr;
	if (caster && caster->Type == ST_ACTOR) {
		act = (const Actor *) caster;
	}
	if (caster && (!checkingEA || (act && act->GetStat(IE_EA) < EA_GOODCUTOFF))) {
		return flags;
	}
	// iwd2 ar6050 has doors casting chain lightning :)
	if (caster && caster->Type != ST_ACTOR && checkingEA) {
		return flags;
	}
	// make everyone an enemy of neutrals
	if (act && checkingEA && act->GetStat(IE_EA) > EA_GOODCUTOFF && act->GetStat(IE_EA) < EA_EVILCUTOFF) {
		if ((Extension->AFlags & PAF_TARGET) == PAF_ENEMY) return GA_NO_NEUTRAL | (flags & GA_NO_LOS);
		if ((Extension->AFlags & PAF_TARGET) == PAF_TARGET) return GA_NO_ALLY | GA_NO_ENEMY | (flags & GA_NO_LOS);
	}

	return flags^(GA_NO_ALLY|GA_NO_ENEMY);
}

//get actors covered in area of trigger radius
void Projectile::CheckTrigger(unsigned int radius)
{
	if (phase == P_TRIGGER) {
		//special trigger flag, explode only if the trigger animation has
		//passed a hardcoded sequence number
		if (Extension->AFlags&PAF_TRIGGER_D) {
			if (travel[Orientation]) {
				int anim = travel[Orientation]->GetCurrentFrameIndex();
				if (anim<30)
					return;
			}
		}
	}
	if (area->GetActorInRadius(Pos, CalculateTargetFlag(), radius)) {
		if (phase == P_TRIGGER) {
			phase = P_EXPLODING1;
			extension_delay = Extension->Delay;
		}
	} else if (phase == P_EXPLODING1) {
		//the explosion is revoked
		if (Extension->AFlags&PAF_SYNC) {
			phase = P_TRIGGER;
		}
	}
}

void Projectile::SetEffectsCopy(const EffectQueue *eq, const Point &source)
{
	delete effects;
	if(!eq) {
		effects=NULL;
		return;
	}
	effects = eq->CopySelf();
	effects->ModifyAllEffectSources(source);
}

void Projectile::LineTarget() const
{
	LineTarget(path, nullptr);
}

void Projectile::LineTarget(const PathNode *beg, const PathNode *end) const
{
	if (!effects) {
		return;
	}

	Actor *original = area->GetActorByGlobalID(Caster);
	int targetFlags = CalculateTargetFlag();
	const PathNode *iter = beg;

	do {
		const PathNode *first = iter;
		const PathNode *last = iter;
		unsigned int orient = first->orient;
		while (iter && iter != end && iter->orient == orient) {
			last = iter;
			iter = iter->Next;
		}

		const Point s(first->x, first->y);
		const Point d(last->x, last->y);
		const std::vector<Actor *> &actors = area->GetAllActors();

		for (Actor *target : actors) {
			if (target->GetGlobalID() == Caster) {
				continue;
			}
			if (!target->ValidTarget(targetFlags)) {
				continue;
			}
			double t = 0.0;
			if (PersonalLineDistance(s, d, target, &t) > 1) {
				continue;
			}
			if (t < 0.0 && first->Parent != nullptr && first->Parent->orient == orient) {
				// skip; assume we've hit the target before
				continue;
			} else if (t > 1.0 && last->Next != nullptr && last->Next->orient == orient) {
				// skip; assume we'll hit it after
				continue;
			}

			if (effects->CheckImmunity(target) > 0) {
				EffectQueue *eff = effects->CopySelf();
				eff->SetOwner(original);
				if (ExtFlags & PEF_RGB) {
					target->SetColorMod(0xff, RGBModifier::ADD, ColorSpeed, RGB);
				}

				eff->AddAllEffects(target, target->Pos);
				delete eff;
			}
		}
	} while (iter && iter != end);
}

//secondary projectiles target all in the explosion radius
void Projectile::SecondaryTarget()
{
	//fail will become true if the projectile utterly failed to find a target
	//if the spell was already applied on explosion, ignore this
	bool fail= !!(Extension->APFlags&APF_SPELLFAIL) && !(ExtFlags&PEF_DEFSPELL);
	int mindeg = 0;
	int maxdeg = 0;
	int degOffset = 0;

	//the AOE (area of effect) is cone shaped
	if (Extension->AFlags&PAF_CONE) {
		// see CharAnimations.cpp for a nice visualization of the orientation directions
		// they start at 270° and go anticlockwise, so we have to rotate (reflect over y=-x) to match what math functions expect
		// TODO: check if we can ignore this and use the angle between caster pos and target pos (are they still available here?)
		char saneOrientation = 12 - Orientation;
		if (saneOrientation < 0) saneOrientation = MAX_ORIENT + saneOrientation;

		// for cone angles (widths) bigger than 22.5 we will always have a range of values greater than 360
		// to normalize into [0,360] we use an orientation dependent factor that is then accounted for in later calculations
		mindeg = (saneOrientation * (720 / MAX_ORIENT) - Extension->ConeWidth) / 2;
		if (mindeg < 0) {
			degOffset = -mindeg;
			//mindeg = 0;
		} else if (mindeg + Extension->ConeWidth > 360) {
			degOffset = -(mindeg - 360 + Extension->ConeWidth);
		}
		mindeg += degOffset;
		maxdeg = mindeg + Extension->ConeWidth;
	}

	if (Extension->DiceCount) {
		//precalculate the maximum affected target count in case of PAF_AFFECT_ONE 
		extension_targetcount = core->Roll(Extension->DiceCount, Extension->DiceSize, 0);
	} else {
		//this is the default case (for original engine)
		extension_targetcount = 1;
	}

	int radius = Extension->ExplosionRadius / 16;
	std::vector<Actor *> actors = area->GetAllActorsInRadius(Pos, CalculateTargetFlag(), radius);
	for (const Actor *actor : actors) {
		ieDword targetID = actor->GetGlobalID();

		//this flag is actually about ignoring the caster (who is at the center)
		if ((SFlags & PSF_IGNORE_CENTER) && Caster == targetID) {
			continue;
		}

		//IDS targeting for area projectiles
		if (FailedIDS(actor)) {
			continue;
		}

		if (Extension->AFlags&PAF_CONE) {
			//cone never affects the caster
			if (Caster == targetID) {
				continue;
			}
			double xdiff = actor->Pos.x - Pos.x;
			double ydiff = Pos.y - actor->Pos.y;
			int deg;

			// a dragon will definitely be easier to hit than a mouse
			// but nothing checks the personal space of possible targets in the original either #384
			if (ydiff) {
				// ensure [0,360] range: transform [-180,180] from atan2, but also take orientation correction factor into account
				deg = (int) (std::atan2(ydiff, xdiff) * 180/M_PI);
				deg = ((deg % 360) + 360 + degOffset) % 360;
			} else {
				if (xdiff < 0) {
					deg = 180;
				} else {
					deg = 0;
				}
			}

			//not in the right sector of circle
			if (mindeg>deg || maxdeg<deg) {
				continue;
			}
		}

		Projectile *pro = server->GetProjectileByIndex(Extension->ExplProjIdx);
		pro->SetEffectsCopy(effects, Pos);
		//copy the additional effects reference to the child projectile
		//but only when there is a spell to copy
		if (!successSpell.IsEmpty()) {
			pro->successSpell = successSpell;
		}
		pro->SetCaster(Caster, Level);
		//this is needed to apply the success spell on the center point
		pro->SetTarget(Pos);
		//TODO:actually some of the splash projectiles are a good example of faketarget
		//projectiles (that don't follow the target, but still hit)
		area->AddProjectile(pro, Pos, targetID, false);
		fail=false;

		//we already got one target affected in the AOE, this flag says
		//that was enough (the GemRB extension can repeat this a random time (x d y)
		if(Extension->AFlags&PAF_AFFECT_ONE) {
			if (extension_targetcount<=0) {
				break;
			}
			//if target counting is per HD and this target is an actor, use the xp level field
			//otherwise count it as one
			if (Extension->APFlags & APF_COUNT_HD) {
				extension_targetcount -= actor->GetXPLevel(true);
			} else {
				extension_targetcount--;
			}
		}
	}

	//In case of utter failure, apply a spell of the same name on the caster
	//this feature is used by SCHARGE, PRTL_OP and PRTL_CL in the HoW pack
	if(fail) {
		ApplyDefault();
	}
}

int Projectile::Update()
{
	//if reached target explode
	//if target doesn't exist expire
	if (phase == P_EXPIRED) {
		return 0;
	}
	if (phase == P_UNINITED) {
		Setup();
	}

	int pause = core->IsFreezed();
	if (pause) {
		return 1;
	}

	const Game *game = core->GetGame();
	if (game && game->IsTimestopActive() && !(TFlags&PTF_TIMELESS)) {
		return 1;
	}

	//recreate path if target has moved
	if(Target) {
		SetTarget(Target, false);
	}

	if (phase == P_TRAVEL || phase == P_TRAVEL2) {
		DoStep(Speed);
	}
	return 1;
}

void Projectile::Draw(const Region& viewport)
{
	switch (phase) {
		case P_UNINITED:
			return;
		case P_TRIGGER: case P_EXPLODING1:case P_EXPLODING2:
			//This extension flag is to enable the travel projectile at
			//trigger/explosion time
			if (Extension->AFlags&PAF_VISIBLE) {
			//if (!Extension || (Extension->AFlags&PAF_VISIBLE)) {
				DrawTravel(viewport);
			}
			/*
			if (!Extension) {
				return;
			}*/
			CheckTrigger(Extension->TriggerRadius);
			if (phase == P_EXPLODING1 || phase == P_EXPLODING2) {
				DrawExplosion(viewport);
			}
			break;
		case P_TRAVEL: case P_TRAVEL2:
			//There is no Extension for simple traveling projectiles!
			DrawTravel(viewport);
			return;
		default:
			DrawExploded(viewport);
			return;
	}
}

bool Projectile::DrawChildren(const Region& vp)
{
	bool drawn = false;

	if (children) {
		for(int i=0;i<child_size;i++){
			if(children[i]) {
				if (children[i]->Update()) {
					children[i]->DrawTravel(vp);
					drawn = true;
				} else {
					delete children[i];
					children[i]=NULL;
				}
			}
		}
	}

	return drawn;
}

//draw until all children expire
void Projectile::DrawExploded(const Region& viewport)
{
	if (DrawChildren(viewport)) {
		return;
	}
	phase = P_EXPIRED;
}

void Projectile::SpawnFragment(Point &dest)
{
	Projectile *pro = server->GetProjectileByIndex(Extension->FragProjIdx);
	if (pro) {
//		if (Extension->AFlags&PAF_SECONDARY) {
//				pro->SetEffectsCopy(effects);
//		}
		pro->SetCaster(Caster, Level);
		if (pro->ExtFlags&PEF_RANDOM) {
			dest.x+=core->Roll(1,Extension->TileX, -Extension->TileX/2);
			dest.y+=core->Roll(1,Extension->TileY, -Extension->TileY/2);
		}
		area->AddProjectile(pro, dest, dest);
	}
}

void Projectile::DrawExplosion(const Region& vp)
{
	//This seems to be a needless safeguard
	if (!Extension) {
		phase = P_EXPIRED;
		return;
	}

	StopSound();
	DrawChildren(vp);

	int pause = core->IsFreezed();
	if (pause) {
		return;
	}

	//Delay explosion, it could even be revoked with PAF_SYNC (see skull trap)
	if (extension_delay) {
		extension_delay--;
		return;
	}

	//0 and 1 have the same effect (1 explosion)
	if (extension_explosioncount) {
		extension_explosioncount--;
	}

	//Line targets are actors between source and destination point
	if(ExtFlags&PEF_LINE) {
		if (Target) {
			SetTarget(Target, false);
		}
		LineTarget();
	}

	int apflags = Extension->APFlags;
	int aoeflags = Extension->AFlags;

	//no idea what is PAF_SECONDARY
	//probably it is to alter some behaviour in the secondary
	//projectile generation
	//In trapskul.pro it isn't set, yet it has a secondary (invisible) projectile
	//All area effects are created by secondary projectiles

	//the secondary projectile will target everyone in the area of effect
	SecondaryTarget();

	//draw fragment graphics animation at the explosion center
	if (aoeflags&PAF_FRAGMENT) {
		//there is a character animation in the center of the explosion
		//which will go towards the edges (flames, etc)
		//Extension->ExplColor fake color for single shades (blue,green,red flames)
		//Extension->FragAnimID the animation id for the character animation
		//This color is not used in the original game
		Point pos = Pos - vp.origin;
		area->Sparkle(0, Extension->ExplColor, SPARKLE_EXPLOSION, pos, Extension->FragAnimID, GetZPos());
	}

	if(Shake) {
		core->timer.SetScreenShake(Point(Shake, Shake), Shake);
		Shake = 0;
	}

	//the center of the explosion could be another projectile played over the target
	//warning: this projectile doesn't inherit any effects, so its payload function
	//won't be doing anything (any effect of PAF_SECONDARY?)

  //remove PAF_SECONDARY if it is buggy, but that will break the 'HOLD' projectile
	if ((Extension->AFlags&PAF_SECONDARY) && Extension->FragProjIdx) {
		if (apflags&APF_TILED) {
			int radius = Extension->ExplosionRadius;

			for (int i = -radius; i < radius; i += Extension->TileX) {
				for (int j = -radius; j < radius; j += Extension->TileY) {
					if (i*i+j*j<radius*radius) {
						Point p(Pos.x+i, Pos.y+j);
						SpawnFragment(p);
					}
				}
			}
		} else {
			SpawnFragment(Pos);
		}
	}

	//the center of the explosion is based on hardcoded explosion type (this is fireball.cpp in the original engine)
	//these resources are listed in areapro.2da and served by ProjectileServer.cpp
	
	//draw it only once, at the time of explosion
	if (phase==P_EXPLODING1) {
		core->GetAudioDrv()->Play(Extension->SoundRes, SFX_CHAN_MISSILE, Pos);
		//play VVC in center
		//FIXME: make it possible to play VEF too?
		if (aoeflags&PAF_VVC) {
			ScriptedAnimation* vvc = gamedata->GetScriptedAnimation(Extension->VVCRes, false);
			if (vvc) {
				if (apflags & APF_VVCPAL) {
					//if the palette is used as tint (as opposed to clown colorset) tint the vvc
					if (apflags & APF_TINT) {
						const auto& pal32 = core->GetPalette32(Extension->ExplColor);
						vvc->Tint = pal32[PALSIZE/2];
						vvc->Transparency |= BlitFlags::COLOR_MOD;
					} else {
						vvc->SetPalette(Extension->ExplColor);
					}
				}
				//if the trail oriented, then the center is oriented too
				if (ExtFlags&PEF_TRAIL) {
					vvc->SetOrientation(Orientation);
				}
				vvc->Pos = Pos;
				vvc->PlayOnce();
				vvc->SetBlend();
				//quick hack to use the single object envelope
				area->AddVVCell(new VEFObject(vvc));
			}
			// bg2 comet has the explosion split into two vvcs, with just a starting cycle difference
			// until we actually need two vvc fields in the extension, let's just hack around it
			if (!stricmp(Extension->VVCRes, "SPCOMEX1")) {
				ScriptedAnimation* vvc = gamedata->GetScriptedAnimation("SPCOMEX2", false);
				if (vvc) {
					vvc->Pos = Pos;
					vvc->PlayOnce();
					vvc->SetBlend();
					area->AddVVCell(new VEFObject(vvc));
				}
			}
		}
		
		phase=P_EXPLODING2;
	} else {
		core->GetAudioDrv()->Play(Extension->AreaSound, SFX_CHAN_MISSILE, Pos);
	}
	
	//the spreading animation is in the first column
	const char *tmp = Extension->Spread;
	if (tmp) {
		//i'm unsure about the need of this
		//returns if the explosion animation is fake coloured
		if (!children) {
			child_size = (Extension->ExplosionRadius+15)/16;
			//more sprites if the whole area needs to be filled
			if (apflags&APF_FILL) child_size*=2;
			if (apflags&APF_SPREAD) child_size*=2;
			if (apflags&APF_BOTH) child_size/=2; //intentionally decreases
			if (apflags&APF_MORE) child_size*=2;
			children = (Projectile **) calloc(sizeof(Projectile *), child_size);
		}
		
		//zero cone width means single line area of effect
		if((aoeflags&PAF_CONE) && !Extension->ConeWidth) {
			child_size = 1;
		}

		int initial = child_size;
		
		for(int i=0;i<initial;i++) {
			//leave this slot free, it is residue from the previous flare up
			if (children[i])
				continue;
			if(apflags&APF_BOTH) {
				if(RAND(0,1)) {
					tmp = Extension->Secondary;
				} else {
					tmp = Extension->Spread;
				}
			}
			//create a custom projectile with single traveling effect
			Projectile *pro = server->CreateDefaultProjectile((unsigned int) ~0);
			pro->BAMRes1 = tmp;
			if (ExtFlags&PEF_TRAIL) {
				pro->Aim = Aim;
			}
			pro->SetEffects(NULL);
			//calculate the child projectile's target point, it is either
			//a perimeter or an inside point of the explosion radius
			int rad = Extension->ExplosionRadius;
			Point newdest;
			
			if (apflags&APF_FILL) {
				rad=core->Roll(1,rad,0);
			}
			int max = 360;
			int add = 0;
			if (aoeflags&PAF_CONE) {
				max=Extension->ConeWidth;
				add=(Orientation*45-max)/2;
			}
			max=core->Roll(1,max,add);
			double degree=max*M_PI/180;
			newdest.x = (int) -(rad * std::sin(degree) );
			newdest.y = (int) (rad * std::cos(degree) );
			
			//these fields and flags are always inherited by all children
			pro->Speed = Speed;
			pro->ExtFlags = ExtFlags&(PEF_HALFTRANS|PEF_CYCLE|PEF_RGB);
			pro->RGB = RGB;
			pro->ColorSpeed = ColorSpeed;

			if (apflags&APF_FILL) {
				int delay;

				//a bit of difference in case crowding is needed
				//make this a separate flag if speed difference
				//is not always wanted
				pro->Speed-=RAND(0,7);

				delay = Extension->Delay*extension_explosioncount;
				if(apflags&APF_BOTH) {
					if (delay) {
						delay = RAND(0, delay-1);
					}
				}
				//this needs to be commented out for ToB horrid wilting
				//if(ExtFlags&PEF_FREEZE) {
					delay += Extension->Delay;
				//}
				pro->SetDelay(delay);
			}

			newdest += Destination;

			if (apflags&APF_SCATTER) {
				pro->MoveTo(area, newdest);
			} else {
				pro->MoveTo(area, Pos);
			}
			pro->SetTarget(newdest);
			
			//sets up the gradient color for the explosion animation
			if (apflags&(APF_PALETTE|APF_TINT) ) {
				pro->SetGradient(Extension->ExplColor, !(apflags&APF_PALETTE));
			}
			//i'm unsure if we need blending for all anims or just the tinted ones
			pro->TFlags|=PTF_TRANS;
			//random frame is needed only for some of these, make it an areapro flag?
			if( !(ExtFlags&PEF_CYCLE) || (ExtFlags&PEF_RANDOM) ) {
				pro->ExtFlags|=PEF_RANDOM;
			}

			pro->Setup();

			// currently needed by bg2/how Web (less obvious in bg1)
			// TODO: original behaviour was: play once, wait in final frame, hide, wait, repeat
			if (pro->travel[0] && Extension->APFlags & APF_PLAYONCE) {
				// set on all orients while we don't force one for single-orientation animations (see CreateOrientedAnimations)
				for (auto& anim : pro->travel) {
					if (anim) anim->Flags |= A_ANI_PLAYONCE;
				}
			}

			children[i]=pro;
		}
	}

	if (extension_explosioncount) {
		extension_delay=Extension->Delay;
	} else {
		phase = P_EXPLODED;
	}
}

int Projectile::GetTravelPos(int face) const
{
	if (travel[face]) {
		return travel[face]->GetCurrentFrameIndex();
	}
	return 0;
}

int Projectile::GetShadowPos(int face) const
{
	if (shadow[face]) {
		return shadow[face]->GetCurrentFrameIndex();
	}
	return 0;
}

void Projectile::SetPos(int face, int frame1, int frame2)
{
	if (travel[face]) {
		travel[face]->SetFrame(frame1);
	}
	if (shadow[face]) {
		shadow[face]->SetFrame(frame2);
	}
}

//recalculate target and source points (perpendicular bisector)
void Projectile::SetupWall()
{
	Point p1 = (Pos + Destination) / 2;
	Point p2 = p1 + (Pos - Destination);
	Pos = p1;
	SetTarget(p2);
}

void Projectile::DrawLine(const Region& vp, int face, BlitFlags flag)
{
	Game *game = core->GetGame();
	PathNode *iter = path;
	Holder<Sprite2D> frame;
	if (game && game->IsTimestopActive() && !(TFlags&PTF_TIMELESS)) {
		frame = travel[face]->LastFrame();
		flag |= BlitFlags::GREY;
	} else {
		frame = travel[face]->NextFrame();
	}

	Color tint2 = tint;
	if (game) game->ApplyGlobalTint(tint2, flag);
	while(iter) {
		Point pos(iter->x - vp.x, iter->y - vp.y);

		if (SFlags&PSF_FLYING) {
			pos.y-=FLY_HEIGHT;
		}

		Draw(frame, pos, flag, tint2);
		iter = iter->Next;
	}
}

void Projectile::DrawTravel(const Region& viewport)
{
	Game *game = core->GetGame();
	BlitFlags flag;

	if(ExtFlags&PEF_HALFTRANS) {
		flag = BlitFlags::HALFTRANS;
	} else {
		flag = BlitFlags::NONE;
	}

	//static tint (use the tint field)
	if(ExtFlags&PEF_TINT) {
		flag |= BlitFlags::COLOR_MOD;
	}

	//Area tint
	if (TFlags&PTF_TINT) {
		tint = area->LightMap->GetPixel(Map::ConvertCoordToTile(Pos));
		tint.a = 255;
		flag |= BlitFlags::COLOR_MOD;
	}

	unsigned int face = GetNextFace();
	if (face!=Orientation) {
		SetPos(face, GetTravelPos(face), GetShadowPos(face));
	}

	Point pos = Pos - viewport.origin;
	if(bend && phase == P_TRAVEL && Origin != Destination) {
		double total_distance = Distance(Origin, Destination);
		double travelled_distance = Distance(Origin, Pos);

		// distance travelled along the line, from 0.0 to 1.0
		double travelled = travelled_distance / total_distance;
		assert(travelled <= 1.0);

		// input to sin(): 0 to pi gives us an arc
		double arc_angle = travelled * M_PI;

		// calculate the distance between the arc and the current pos
		// (this could use travelled and a larger constant multiplier,
		// to make the arc size fixed rather than relative to the total
		// distance to travel)
		double length_of_normal = travelled_distance * std::sin(arc_angle) * 0.3 * ((bend / 2) + 1);
		if (bend % 2) length_of_normal = -length_of_normal;

		// adjust the to-be-rendered point by that distance
		double x_vector = (Destination.x - Origin.x) / total_distance,
			y_vector = (Destination.y - Origin.y) / total_distance;
		Point newpos = pos;
		newpos.x += y_vector * length_of_normal;
		newpos.y -= x_vector * length_of_normal;
		pos = newpos;
	}

	// set up the tint for the rest of the blits, but don't overwrite the saved one
	Color tint2 = tint;
	BlitFlags flags = flag;

	if (TFlags&PTF_TINT && game) {
		game->ApplyGlobalTint(tint2, flags);
	}

	if (light) {
		Draw(light, pos, flags^flag, tint2);
	}

	if (ExtFlags&PEF_POP) {
			//draw pop in/hold/pop out animation sequences
			Holder<Sprite2D> frame;
			if (game && game->IsTimestopActive() && !(TFlags&PTF_TIMELESS)) {
				frame = travel[face]->LastFrame();
				flags |= BlitFlags::GREY;
			} else {
				if (ExtFlags&PEF_UNPOP) {
					frame = shadow[0]->NextFrame();
					if (shadow[0]->endReached) {
						ExtFlags &= ~PEF_UNPOP;
					}
				} else {
					frame = travel[0]->NextFrame();
					if (travel[0]->endReached) {
						travel[0]->playReversed = true;
						travel[0]->SetFrame(0);
						ExtFlags |= PEF_UNPOP;
						frame = shadow[0]->NextFrame();
					}
				}
			}
			Draw(frame, pos, flags, tint2);
			return;
	}
	
	if (ExtFlags&PEF_LINE) {
		DrawLine(viewport, face, flag);
		return;
	}
	
	if (shadow[face]) {
		Holder<Sprite2D> frame = shadow[face]->NextFrame();
		Draw(frame, pos, flags, tint2);
	}

	pos.y-=GetZPos();

	if (ExtFlags&PEF_PILLAR) {
		//draw all frames simultaneously on top of each other
		for(int i=0;i<Aim;i++) {
			if (travel[i]) {
				Holder<Sprite2D> frame = travel[i]->NextFrame();
				Draw(frame, pos, flags, tint2);
				pos.y-=frame->Frame.y;
			}
		}
	} else {
		if (travel[face]) {
			Holder<Sprite2D> frame;
			if (game && game->IsTimestopActive() && !(TFlags&PTF_TIMELESS)) {
				frame = travel[face]->LastFrame();
				flags |= BlitFlags::GREY; // move higher if it interferes with other tints badly
			} else {
				frame = travel[face]->NextFrame();
			}
			Draw(frame, pos, flags, tint2);
		}
	}

	if (drawSpark) {
		area->Sparkle(0,SparkColor, SPARKLE_EXPLOSION, pos, 0, GetZPos() );
		drawSpark = 0;
	}

}

int Projectile::GetZPos() const
{
	return ZPos;
}

void Projectile::SetIdentifiers(const ResRef &resref, size_t idx)
{
	projectileName = resref;
	type = static_cast<ieWord>(idx);
}

bool Projectile::PointInRadius(const Point &p) const
{
	switch(phase) {
		//better not trigger on projectiles unset/expired
		case P_EXPIRED:
		case P_UNINITED: return false;
		case P_TRAVEL:
			return p == Pos;
		default:
			if (p == Pos) return true;
			if (!Extension) return false;
			if (Distance(p, Pos) < Extension->ExplosionRadius) return true;
	}
	return false;
}

//Set gradient color, if type is true then it is static tint, otherwise it is paletted color
void Projectile::SetGradient(int gradient, bool type)
{
	//gradients are unsigned chars, so this works
	memset(Gradients, gradient, 7);
	if (type) {
		ExtFlags|=PEF_TINT;
	} else {
		TFlags |= PTF_COLOUR;
	}
}

void Projectile::StaticTint(const Color &newtint)
{
	tint = newtint;
	TFlags &= ~PTF_TINT; //turn off area tint
}

int Projectile::GetPhase() const
{
	return phase;
}

void Projectile::Cleanup()
{
	//neutralise the payload
	delete effects;
	effects = NULL;
	//diffuse the projectile
	phase=P_EXPIRED;
}

void Projectile::Draw(const Holder<Sprite2D>& spr, const Point& p, BlitFlags flags, Color tint) const
{
	Video *video = core->GetVideoDriver();
	PaletteHolder pal = (spr->Format().Bpp == 1) ? palette : nullptr;
	if (flags & BlitFlags::COLOR_MOD) {
		// FIXME: this may not apply universally
		flags |= BlitFlags::ALPHA_MOD;
	}
	video->BlitGameSpriteWithPalette(spr, pal, p, flags|BlitFlags::BLENDED, tint);
}

}
