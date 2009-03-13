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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Id$
 *
 */


#include "../../includes/win32def.h"
#include <stdlib.h>
#include "Projectile.h"
#include "Interface.h"
#include "Video.h"
#include "Game.h"
#include "ResourceMgr.h"
#include "Audio.h"
#include "ProjectileServer.h"

extern Interface* core;
#ifdef WIN32
extern HANDLE hConsole;
#endif

static const ieByte SixteenToNine[MAX_ORIENT]={0,1,2,3,4,5,6,7,8,7,6,5,4,3,2,1};
static const ieByte SixteenToFive[MAX_ORIENT]={0,0,1,1,2,2,3,3,4,4,3,3,2,2,1,1};

Projectile::Projectile()
{
	autofree = false;
	Extension = NULL;
	area = NULL;
	palette = NULL;
	PaletteRes[0]=0;
	//shadpal = NULL;
	Pos.empty();
	Destination = Pos;
	Orientation = 0;
	NewOrientation = 0;
	path = NULL;
	step = NULL;
	timeStartStep = 0;
	phase = P_UNINITED;
	effects = NULL;
}

Projectile::~Projectile()
{
	if (autofree) {
		free(Extension);
	}
	delete effects;

	gamedata->FreePalette(palette, PaletteRes);
	//gamedata->FreePalette(shadpal);
	ClearPath();

	if (phase != P_UNINITED) {
		int i;
		for (i = 0; i < MAX_ORIENT; ++i) {
			delete travel[i];
			delete shadow[i];
		}
	}
}

void Projectile::InitExtension()
{
	autofree = false;
	if (!Extension) {
		Extension = (ProjectileExtension *) calloc( 1, sizeof(ProjectileExtension));
	}
}

PathNode *Projectile::GetNextStep(int x)
{
	if (!step) {
		DoStep((unsigned int) ~0);
	}
	PathNode *node = step;
	while(node && x--) {
		node = node->Next;
	}
	return node;
}

void Projectile::CreateAnimations(Animation **anims, const ieResRef bamres, int Seq)
{
	AnimationFactory* af = ( AnimationFactory* )
		core->GetResourceMgr()->GetFactoryResource( bamres,
		IE_BAM_CLASS_ID, IE_NORMAL );
	//
	if (!af) {
		return;
	}
	//this hack is needed because bioware .pro files are sometimes
	//reporting bigger face count than possible by the animation
	int Max = af->GetCycleCount();
	if (Aim>Max) Aim=Max;
	for (int Cycle = 0; Cycle<MAX_ORIENT; Cycle++) {
		int c;
		switch(Aim) {
		default:
			c = Seq;
			break;
		case 5:
			c = SixteenToFive[Cycle];
			break;
		case 9:
			c = SixteenToNine[Cycle];
			break;
		case 16:
			c=Cycle;
			break;
		}
		Animation* a = af->GetCycle( c );
		if (a && c!=Cycle) {
			a->MirrorAnimation();
		}
		anims[Cycle] = a;
	}
}

//apply gradient colors
void Projectile::SetupPalette(Animation *anim[], Palette *&pal, const ieByte *gradients)
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

void Projectile::GetPaletteCopy(Animation *anim[], Palette *&pal)
{
	if (pal)
		return;
	for (unsigned int i=0;i<MAX_ORIENT;i++) {
		if (anim[i]) {
			Sprite2D* spr = anim[i]->GetFrame(0);
			if (spr) {
				pal = core->GetVideoDriver()->GetPalette(spr)->Copy();
			}
		}
	}
}

void Projectile::SetBlend()
{
	GetPaletteCopy(travel, palette);
	if (!palette)
		return;
	if (!palette->alpha) {
		palette->CreateShadedAlphaChannel();
	}
}

// load animations, start sound
void Projectile::Setup()
{
	phase = P_TRAVEL;
	core->GetAudioDrv()->Play(SoundRes1, Pos.x, Pos.y, GEM_SND_RELATIVE);
	memset(travel,0,sizeof(travel));
	memset(shadow,0,sizeof(shadow));
	light = NULL;

	CreateAnimations(travel, BAMRes1, Seq1);

	if (TFlags&PTF_COLOUR) {
		SetupPalette(travel, palette, Gradients);
	} else {
		gamedata->FreePalette(palette, PaletteRes);
		palette=gamedata->GetPalette(PaletteRes);
	}

	if (TFlags&PTF_SHADOW) {
		CreateAnimations(shadow, BAMRes2, Seq2);
		//if (TFlags&PTF_SHADOWCOLOR) {
		//	SetupPalette(shadow, shadpal, Gradients);
		//}
	}
	if (TFlags&PTF_LIGHT) {
		light = core->GetVideoDriver()->CreateLight(LightX, LightZ);
	}
	if (TFlags&PTF_BLEND) {
		SetBlend();
	}
}

Actor *Projectile::GetTarget()
{
	Actor *target;

	if (Target) {
		target = area->GetActorByGlobalID(Target);
		if (!target) return NULL;
		Actor *original = area->GetActorByGlobalID(Caster);
		if (original==target) {
			effects->SetOwner(target);
			return target;
		}
		int res = effects->CheckImmunity ( target );
		//resisted
		if (!res) {
			return NULL;
		}
		if (res==-1) {
			phase = P_TRAVEL;
			Target = original->GetID();
			return NULL;
		}
		effects->SetOwner(original);
		return target;
	}
	target = area->GetActorByGlobalID(Caster);
	if (target) {
		effects->SetOwner(target);
	}
	return target;
}

//control the phase change when the projectile reached its target
//possible actions: vanish, hover over point, explode
//depends on the area extension
//play explosion sound
void Projectile::ChangePhase()
{
	//follow target
	if (Target) {
		Actor *target = area->GetActorByGlobalID(Target);
		if (!target) {
			phase = P_EXPIRED;
			return;
		}
		int steps = Distance(Pos, target);
		//48 = 6*8
		if (steps>=48) {
			NextTarget(target->Pos);
			return;
		}
	}

	//reached target
	if (!Extension) {
		phase = P_EXPIRED;
		//there could be no-effect projectiles
		if (!effects) return;

		Actor *target;

		if (Target) {
			target = GetTarget();
			if (target) {
				//projectile rebounced
				if (phase!=P_EXPIRED) {
					return;
				}
			}
		} else {
			//the target will be the original caster
			//in case of single point area target (dimension door)
			target = area->GetActorByGlobalID(Caster);
			if (target) {
				effects->SetOwner(target);
			}
		}
		if (target) {
			effects->AddAllEffects(target, Destination);
		}
		delete effects;
		effects = NULL;
		return;
	}
	if (Extension->AFlags&PAF_TRIGGER) {
		phase = P_TRIGGER;
	} else {
		phase = P_EXPLODING;
	}
}

void Projectile::DoStep(unsigned int walk_speed)
{
	if (!path) {
		ChangePhase();
		return;
	}
	if (Pos==Destination) {
		ClearPath();
		ChangePhase();
		return;
	}

	ieDword time = core->GetGame()->Ticks;
	if (!step) {
		step = path;
		timeStartStep = time;
	}
	while (step->Next && (( time - timeStartStep ) >= walk_speed)) {
		step = step->Next;
		if (!walk_speed) {
			timeStartStep = time;
			break;
		}
		timeStartStep = timeStartStep + walk_speed;
	}

	SetOrientation (step->orient, false);

	Pos.x=step->x;
	Pos.y=step->y;
	if (!step->Next) {
		ClearPath();
		NewOrientation = Orientation;
		ChangePhase();
		return;
	}
	if (!walk_speed) {
		return;
	}
	if (step->Next->x > step->x)
		Pos.x += ( unsigned short )
			( ( step->Next->x - Pos.x ) * ( time - timeStartStep ) / walk_speed );
	else
		Pos.x -= ( unsigned short )
			( ( Pos.x - step->Next->x ) * ( time - timeStartStep ) / walk_speed );
	if (step->Next->y > step->y)
		Pos.y += ( unsigned short )
			( ( step->Next->y - Pos.y ) * ( time - timeStartStep ) / walk_speed );
	else
		Pos.y -= ( unsigned short )
			( ( Pos.y - step->Next->y ) * ( time - timeStartStep ) / walk_speed );
}

void Projectile::SetCaster(ieDword caster)
{
	Caster=caster;
}

void Projectile::NextTarget(Point &p)
{
	ClearPath();
	Destination = p;
	//call this with destination
	if (path) {
		return;
	}
	if (!Speed) {
		Pos = Destination;
		return;
	}
	Orientation = GetOrient(Pos, Destination);
	path = area->GetLine( Pos, Destination, Speed, Orientation, GL_PASS );
}

void Projectile::SetTarget(Point &p)
{
	Target = 0;
	NextTarget(p);
}

void Projectile::SetTarget(ieDword tar)
{
	Target = tar;
	Actor *target = area->GetActorByGlobalID(tar);
	if (!target) {
		phase = P_EXPIRED;
		return;
	}
	NextTarget(target->Pos);
}

void Projectile::MoveTo(Map *map, Point &Des)
{
	area = map;
	Pos = Des;
	Destination = Des;
}

void Projectile::ClearPath()
{
	if (!path) {
		return;
	}
	PathNode* nextNode = path->Next;
	PathNode* thisNode = path;
	while (true) {
		delete( thisNode );
		if (!nextNode)
			break;
		thisNode = nextNode;
		nextNode = thisNode->Next;
	}
	path = NULL;
	step = NULL;
}

//get actors covered in area of trigger radius
void Projectile::CheckTrigger(unsigned int radius)
{
	//if there are any, then change phase to exploding
	if (area->GetActorInRadius(Pos, 0, radius)) {
		phase = P_EXPLODING;
	}
}

void Projectile::SetEffectsCopy(EffectQueue *eq)
{
	if(effects) delete effects;
	if(!eq) {
		effects=NULL;
		return;
	}
	effects = eq->CopySelf();
}

//secondary projectiles target all in the explosion radius
void Projectile::SecondaryTarget()
{
	int radius = Extension->ExplosionRadius;
	Actor **actors = area->GetAllActorsInRadius(Pos, 0, radius);
	Actor **poi=actors;
	while(*poi) {
		Projectile *pro = core->GetProjectileServer()->GetProjectileByIndex(Extension->ExplProjIdx);
		pro->SetEffectsCopy(effects);
		pro->SetCaster(Caster);
		area->AddProjectile(pro, Pos, (*poi)->GetGlobalID());
		poi++;
	}
	free(actors);
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
	if (phase == P_TRAVEL) {
		DoStep(Speed);
	}
	return 1;
}

void Projectile::Draw(Region &screen)
{
	switch (phase) {
		case P_UNINITED:
			return;
		case P_TRIGGER:
			if (Extension->AFlags&PAF_VISIBLE) {
				DrawTravel(screen);
			}
			CheckTrigger(Extension->TriggerRadius);
		case P_TRAVEL:
			if (phase != P_EXPLODING) {
				DrawTravel(screen);
				return;
			}
			//falling through in case of explosion
		case P_EXPLODING:
			DrawExplosion(screen);
			return;
		default:
			DrawExploded(screen);
			return;
	}
}

void Projectile::DrawExploded(Region & /*screen*/)
{
	phase = P_EXPIRED;
}

void Projectile::DrawExplosion(Region & /*screen*/)
{
	if (!Extension) {
		phase = P_EXPIRED;
		return;
	}
	if (Extension->Delay) {
		Extension->Delay--;
		return;
	}
	if (Extension->ExplosionCount) {
		Extension->ExplosionCount--;
	} else {
		phase = P_EXPLODED;
	}

	//there is a secondary projectile
	if (Extension->AFlags&PAF_SECONDARY) {
		//the secondary projectile will target everyone in the area of effect
		SecondaryTarget();
	}

	if (Extension->AFlags&PAF_FRAGMENT) {
		//there is a character animation in the center of the explosion
		//which will go towards the edges (flames, etc)
		//Extension->ExplColor fake color for single shades (blue,green,red flames)
		//Extension->FragAnimID the animation id for the character animation
	}

	//the center of the explosion could be another projectile played over the target
	if (Extension->FragProjIdx) {
		Projectile *pro = core->GetProjectileServer()->GetProjectileByIndex(Extension->FragProjIdx);
		if (pro) {
			area->AddProjectile(pro, Pos, Pos);
		}
	}

	//the center of the explosion is based on hardcoded explosion type (this is fireball.cpp in the original engine)
	//these resources are listed in areapro.2da
	if (Extension->ExplType!=0xff) {
		ieResRef *res = core->GetProjectileServer()->GetExplosion(Extension->ExplType, 0);
		if (res) {
			ScriptedAnimation* vvc = gamedata->GetScriptedAnimation(*res, false);
			if (vvc) {
				vvc->XPos+=Pos.x;
				vvc->YPos+=Pos.y;
				area->AddVVCell(vvc);
			}
		}
	}
}

int Projectile::GetTravelPos(int face)
{
	if (travel[face]) {
		return travel[face]->GetCurrentFrame();
	}
	return 0;
}

int Projectile::GetShadowPos(int face)
{
	if (shadow[face]) {
		return shadow[face]->GetCurrentFrame();
	}
	return 0;
}

void Projectile::SetPos(int face, int frame1, int frame2)
{
	if (travel[face]) {
		travel[face]->SetPos(frame1);
	}
	if (shadow[face]) {
		shadow[face]->SetPos(frame2);
	}
}

void Projectile::DrawTravel(Region &screen)
{
	Video *video = core->GetVideoDriver();
	ieDword flag = 0;

	Color tint = {128,128,128,255};
	unsigned int face = GetNextFace();
	if (face!=Orientation) {
		SetPos(face, GetTravelPos(face), GetShadowPos(face));
	}
	Point pos = Pos;
	pos.x+=screen.x;
	pos.y+=screen.y;

	if (shadow[face]) {
		Sprite2D *frame = shadow[face]->NextFrame();
		video->BlitGameSprite( frame, pos.x, pos.y, flag, tint, NULL, NULL, &screen);
	}

	if (light) {
		video->BlitGameSprite( light, pos.x, pos.y, 0, tint, NULL, NULL, &screen);
	}

	if (SFlags&PSF_FLYING) {
		pos.y-=FLY_HEIGHT;
	}

	if (travel[face]) {
		Sprite2D *frame = travel[face]->NextFrame();
		video->BlitGameSprite( frame, pos.x, pos.y, flag, tint, NULL, palette, &screen);
	}

}

