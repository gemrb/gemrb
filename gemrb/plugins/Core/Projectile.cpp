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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Projectile.cpp,v 1.2 2006/10/06 23:01:10 avenger_teambg Exp $
 *
 */


#include "../../includes/win32def.h"
#include <stdlib.h>
#include "Projectile.h"
#include "Interface.h"
#include "Video.h"
#include "Game.h"

extern Interface* core;
#ifdef WIN32
extern HANDLE hConsole;
#endif

Projectile::Projectile()
{
	Extension = NULL;
	area = 0;
	Pos.x = 0;
	Pos.y = 0;
	Destination = Pos;
	Orientation = 0;
	NewOrientation = 0;
	path = NULL;
	step = NULL;
	timeStartStep = 0;
	phase = P_UNINITED;
}

Projectile::~Projectile()
{
	if (Extension) {
		free(Extension);
	}
}

void Projectile::InitExtension()
{
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

//this could be used for WingBuffet as well
void Projectile::MoveLine(int steps, int Pass, ieDword orient)
{
	//remove previous path
	ClearPath();
	if (!steps)
		return;
	path = area->GetLine( Pos, steps, orient, Pass );
}

// load animations, start sound
void Projectile::Setup()
{
	phase = P_TRAVEL;
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
		Destination = target->Pos;
		//next position should be current position+speed towards
		//target
		return;
	}
	//reached target
	if (!Extension) {
		phase = P_EXPIRED;
		//deliver payload to target
	}
}

void Projectile::DoStep(unsigned int walk_speed)
{
	if (!path) {
		return;
	}
	ieDword time = core->GetGame()->Ticks;
	if (!step) {
		step = path;
		timeStartStep = time;
	}
	if (( time - timeStartStep ) >= walk_speed) {
		//printf("[New Step] : Orientation = %d\n", step->orient);
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

void Projectile::AddWayPoint(Point &Des)
{
	if (!path) {
		WalkTo(Des);
		return;
	}
	Destination = Des;
	PathNode *endNode=path;
	while(endNode->Next) {
		endNode=endNode->Next;
	}
	Point p(endNode->x, endNode->y);
	PathNode *path2 = area->FindPath( p, Des );
	endNode->Next=path2;
}

void Projectile::WalkTo(Point &Des, int distance)
{
	ClearPath();
	path = area->FindPath( Pos, Des, distance );
	//ClearPath sets destination, so Destination must be set after it
	//also we should set Destination only if there is a walkable path
	if (path) {
		Destination = Des;
	}
}

void Projectile::RunAwayFrom(Point &Des, int PathLength, int flags)
{
	ClearPath();
	path = area->RunAway( Pos, Des, PathLength, flags );
}

void Projectile::MoveTo(Point &Des)
{
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
		case P_EXPLODED:
		case P_EXPIRED:
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

void Projectile::DrawTravel(Region &screen)
{
	Video *video = core->GetVideoDriver();
	ieDword flag = 0;
	Color tint = {128,128,128,255};
	unsigned int face = GetNextFace();
	if (face!=Orientation) {
		travel[face]->SetPos(travel[Orientation]->GetCurrentFrame());
		shadow[face]->SetPos(shadow[Orientation]->GetCurrentFrame());
	}
	Sprite2D *frame = travel[face]->NextFrame();
	video->BlitGameSprite( frame, Pos.x + screen.x, Pos.y + screen.y, flag, tint, NULL, NULL, &screen);

	frame = shadow[face]->NextFrame();
	video->BlitGameSprite( frame, Pos.x + screen.x, Pos.y + screen.y, flag, tint, NULL, NULL, &screen);

	video->BlitGameSprite( light, Pos.x + screen.x, Pos.y + screen.y, flag, tint, NULL, NULL, &screen);
}
