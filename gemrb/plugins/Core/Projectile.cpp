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
#include "SoundMgr.h"

extern Interface* core;
#ifdef WIN32
extern HANDLE hConsole;
#endif

Projectile::Projectile()
{
	autofree = false;
	Extension = NULL;
	area = NULL;
	Pos.x = 0;
	Pos.y = 0;
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
	if (Extension && autofree) {
		free(Extension);
	}
	if (effects) {
		delete effects;
	}
	ClearPath();
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

//Pass means passing/rebounding/extinguishing on walls
void Projectile::MoveLine(int steps, int Pass, ieDword orient)
{
	//call this with destination
	if (path) {
		return;
	}
	if (!steps) {
		Pos = Destination;
		return;
	}
	path = area->GetLine( Pos, steps, orient, Pass );
}

void Projectile::CreateAnimations(Animation **anims, ieResRef bamres)
{
	AnimationFactory* af = ( AnimationFactory* )
		core->GetResourceMgr()->GetFactoryResource( bamres,
		IE_BAM_CLASS_ID, IE_NORMAL );
	//
	if (!af) {
		return;
	}
	for (int Cycle = 0; Cycle<MAX_ORIENT; Cycle++) {
		Animation* a = af->GetCycle( Cycle );
		anims[Cycle] = a;
	}
}

// load animations, start sound
void Projectile::Setup()
{
	phase = P_TRAVEL;
	core->GetSoundMgr()->Play(SoundRes1, Pos.x, Pos.y, GEM_SND_RELATIVE);
	memset(travel,0,sizeof(travel));
	memset(shadow,0,sizeof(shadow));
	light = NULL;
	CreateAnimations(travel, BAMRes1);
	if (TFlags&PTF_SHADOW) {
		CreateAnimations(shadow, BAMRes2);
	}
	if (TFlags&PTF_LIGHT) {
		//light = CreateLight(LightX, LightY, LightZ);
	}
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

		if (steps) {
			SetTarget(target->Pos);
			return;
		}
	}

	//reached target
	if (!Extension) {
		phase = P_EXPIRED;
		if (Target) {
			Actor *target = area->GetActorByGlobalID(Target);
			if (!target) {
				return;
			}
			//deliver payload to target
			Actor *original = area->GetActorByGlobalID(Caster);
			effects->SetOwner(original?original:target);
			effects->AddAllEffects(target);
			effects = NULL;
			return;
		}
		//get target
		return;
	}
	phase = P_TRIGGER;
}

void Projectile::DoStep(unsigned int walk_speed)
{
	if (!path) {
		ChangePhase();
		return;
	}
	ieDword time = core->GetGame()->Ticks;
	if (!step) {
		step = path;
		timeStartStep = time;
	}
	if (( time - timeStartStep ) >= walk_speed) {
		step = step->Next;
		timeStartStep = time;
	}

	SetOrientation (step->orient, false);

	Pos.x = ( step->x * 16 ) + 8;
	Pos.y = ( step->y * 12 ) + 6;
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
			( ( ( ( ( step->Next->x * 16 ) + 8 ) - Pos.x ) * ( time - timeStartStep ) ) / walk_speed );
	else
		Pos.x -= ( unsigned short )
			( ( ( Pos.x - ( ( step->Next->x * 16 ) + 8 ) ) * ( time - timeStartStep ) ) / walk_speed );
	if (step->Next->y > step->y)
		Pos.y += ( unsigned short )
			( ( ( ( ( step->Next->y * 12 ) + 6 ) - Pos.y ) * ( time - timeStartStep ) ) / walk_speed );
	else
		Pos.y -= ( unsigned short )
			( ( ( Pos.y - ( ( step->Next->y * 12 ) + 6 ) ) * ( time - timeStartStep ) ) / walk_speed );
}

void Projectile::SetCaster(ieDword caster)
{
	Caster=caster;
}

void Projectile::SetTarget(Point &p)
{
	ClearPath();
	Destination = p;
	MoveLine( Speed, true, GetOrient(p, Pos) );
}

void Projectile::SetTarget(ieDword tar)
{
	Target = tar;
	Actor *target = area->GetActorByGlobalID(tar);
	if (!target) {
		phase = P_EXPIRED;
		return;
	}
	SetTarget(target->Pos);
}

void Projectile::MoveTo(Map *map, Point &Des)
{
	area = map;
	Pos = Des;
	Destination = Des;
}

void Projectile::ClearPath()
{
	//this is to make sure attackers come to us
	//make sure ClearPath doesn't screw Destination (in the rare cases Destination
	//is set before ClearPath
	Destination = Pos;
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
	//don't call ReleaseCurrentAction
}

//get actors covered in area of effect radius
void Projectile::CheckTrigger(unsigned int radius)
{
	//if there are any, then change phase to exploding
	if (area->GetActorInRadius(Pos, 0, radius)) {
		phase = P_EXPLODING;
	}
}

int Projectile::Update()
{
	//if reached target explode
	//if target doesn't exist expire
	if (phase == P_EXPIRED) {
		return 0;
	}
	if (phase ==P_UNINITED) {
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
/*
	Video *video = core->GetVideoDriver();
*/
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
	if (travel[face]) {
		Sprite2D *frame = travel[face]->NextFrame();
		video->BlitGameSprite( frame, Pos.x + screen.x, Pos.y + screen.y, flag, tint, NULL, NULL, &screen);
	}

	if (shadow[face]) {
		Sprite2D *frame = shadow[face]->NextFrame();
		video->BlitGameSprite( frame, Pos.x + screen.x, Pos.y + screen.y, flag, tint, NULL, NULL, &screen);
	}

	if (light) {
		video->BlitGameSprite( light, Pos.x + screen.x, Pos.y + screen.y, flag, tint, NULL, NULL, &screen);
	}
}

