/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2005 The GemRB Project
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/GlobalTimer.cpp,v 1.30 2006/04/16 23:57:02 avenger_teambg Exp $
 *
 */

#include "GlobalTimer.h"
#include "Interface.h"
#include "Video.h"
#include "Game.h"
#include "GameControl.h"
#include "ControlAnimation.h"

extern Interface* core;

GlobalTimer::GlobalTimer(void)
{
	interval = ( 1000 / AI_UPDATE_TIME );
	Init();
}

GlobalTimer::~GlobalTimer(void)
{
	std::vector<AnimationRef *>::iterator i;
	for(i = animations.begin(); i != animations.end(); ++i) {
		delete (*i);
	}
}

void GlobalTimer::Init()
{
	fadeToCounter = 0;
	fadeFromCounter = 0;
	fadeFromMax = 0;
	fadeToMax = 0;
	waitCounter = 0;
	shakeCounter = 0;
	startTime = 0; //forcing an update
	ClearAnimations();
}

void GlobalTimer::Freeze()
{
	unsigned long thisTime;
	unsigned long advance;

	GetTime( thisTime );
	advance = thisTime - startTime;
	startTime = thisTime;
	Game* game = core->GetGame();
	if (!game) {
		return;
	}
	game->RealTime+=advance;
}

void GlobalTimer::Update()
{
	unsigned long thisTime;
	unsigned long advance;

	UpdateAnimations();

	GetTime( thisTime );
	advance = thisTime - startTime;
	if ( advance >= interval) {
		startTime = thisTime;
		if (shakeCounter) {
			int x = shakeStartVP.x;
			int y = shakeStartVP.y;
			shakeCounter-=advance/interval;
			if (shakeCounter<0) {
				shakeCounter=0;
			}
			if (shakeCounter) {
				x += (rand()%shakeX) - (shakeX>>1);
				y += (rand()%shakeY) - (shakeY>>1);
			}
			core->GetVideoDriver()->MoveViewportTo(x,y,false);
		}
		if (fadeToCounter) {
			fadeToCounter-=advance/interval;
			if (fadeToCounter<0) {
				fadeToCounter=0;
			}
			core->GetVideoDriver()->SetFadePercent( ( ( fadeToMax - fadeToCounter ) * 100 ) / fadeToMax );
			return; //hmm, freeze gametime
		}
		if (fadeFromCounter!=fadeFromMax) {
			if (fadeFromCounter>fadeFromMax) {
				fadeFromCounter-=advance/interval;
				if (fadeFromCounter<fadeFromMax) {
					fadeFromCounter=fadeFromMax;
				}
				//don't freeze gametime when already dark
			} else {
				fadeFromCounter+=advance/interval;
				if (fadeToCounter>fadeFromMax) {
					fadeToCounter=fadeFromMax;
				}
				core->GetVideoDriver()->SetFadePercent( ( ( fadeFromMax - fadeFromCounter ) * 100 ) / fadeFromMax );
				return; //freeze gametime
			}
		}
		if (fadeFromCounter==fadeFromMax) {
			core->GetVideoDriver()->SetFadePercent( 0 );
		}
		GameControl* gc = core->GetGameControl();
		if (!gc) {
			return;
		}
		Game* game = core->GetGame();
		if (!game) {
			return;
		}
		Map* map = game->GetCurrentArea();
		if (!map) {
			return;
		}
		//do spell effects expire in dialogs?
		//if yes, then we should remove this condition
		if (!(gc->GetDialogueFlags()&DF_IN_DIALOG) ) {
			map->UpdateFog();
			map->UpdateEffects();
			//this measures in-world time (affected by effects, actions, etc)
			game->AdvanceTime(advance/interval);
		}
		//this measures time spent in the game (including pauses)
		game->RealTime+=advance;
	}
}

void GlobalTimer::SetFadeToColor(unsigned long Count)
{
	if(!Count) {
		Count = 64;
	}
	fadeToCounter = Count;
	fadeToMax = fadeToCounter;
	//stay black for a while
	fadeFromCounter = 128;
	fadeFromMax = 0;
}

void GlobalTimer::SetFadeFromColor(unsigned long Count)
{
	if(!Count) {
		Count = 64;
	}
	fadeFromCounter = 0;
	fadeFromMax = Count;
}

void GlobalTimer::SetWait(unsigned long Count)
{
	waitCounter = Count;
}

void GlobalTimer::AddAnimation(ControlAnimation* ctlanim, unsigned long time)
{
	AnimationRef* anim;
	unsigned long thisTime;

	GetTime( thisTime );
	time += thisTime;

	// if there are no free animation reference objects,
	// alloc one, else take the first free one
	if (first_animation == 0)
		anim = new AnimationRef;
	else {
		anim = animations.front ();
		animations.erase (animations.begin());
		first_animation--;
	}

	// fill in data
	anim->time = time;
	anim->ctlanim = ctlanim;

	// and insert it into list of other anim refs, sorted by time
	for (std::vector<AnimationRef*>::iterator it = animations.begin() + first_animation; it != animations.end (); it++) {
		if ((*it)->time > time) {
			animations.insert( it, anim );
			anim = NULL;
			break;
		}
	}
	if (anim)
		animations.push_back( anim );
		
}

void GlobalTimer::RemoveAnimation(ControlAnimation* ctlanim)
{
	// Animation refs for given control are not physically removed, 
	// but just marked by erasing ptr to the control. They will be
	// collected when they get to the front of the vector
	for (std::vector<AnimationRef*>::iterator it = animations.begin() + first_animation; it != animations.end (); it++) {
		if ((*it)->ctlanim == ctlanim) {
			(*it)->ctlanim = NULL;
		}
	}
}

void GlobalTimer::UpdateAnimations()
{
	unsigned long thisTime;
	GetTime( thisTime );
	while (animations.begin() + first_animation != animations.end()) {
		AnimationRef* anim = animations[first_animation];
		if (anim->ctlanim == NULL) {
			first_animation++;
			continue;
		}

		if (anim->time <= thisTime) {
			anim->ctlanim->UpdateAnimation();
			first_animation++;
			continue;
		}
		break;
	}
			
}

void GlobalTimer::ClearAnimations()
{
	first_animation = animations.size();
}

void GlobalTimer::SetScreenShake(unsigned long shakeX, unsigned long shakeY,
	unsigned long Count)
{
	this->shakeX = shakeX;
	this->shakeY = shakeY;
	shakeCounter = Count;
}
