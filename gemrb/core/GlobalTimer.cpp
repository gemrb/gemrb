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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "GlobalTimer.h"

#include "ControlAnimation.h"
#include "Game.h"
#include "Interface.h"
#include "GUI/GameControl.h"
#include "RNG.h"

namespace GemRB {

GlobalTimer::GlobalTimer(void)
{
	//AI_UPDATE_TIME: how many AI updates in a second
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
	fadeToFactor = fadeFromFactor = 1;
	shakeVec = Point();
	shakeCounter = 0;
	startTime = 0; //forcing an update
	speed = 0;
	goal = Point(-1,-1);
	ClearAnimations();
}

void GlobalTimer::Freeze()
{
	unsigned long thisTime;

	UpdateAnimations(true);

	thisTime = GetTickCount();
	if (UpdateViewport(thisTime) == false) {
		return;
	}

	startTime = thisTime;
	Game* game = core->GetGame();
	if (!game) {
		return;
	}
	game->RealTime++;
}

bool GlobalTimer::ViewportIsMoving()
{
	return goal.isempty() ? shakeCounter : goal != currentVP.Origin();
}

void GlobalTimer::SetMoveViewPort(Point p, int spd, bool center)
{
	GameControl* gc = core->GetGameControl();
	currentVP = gc->Viewport();

	if (center) {
		p.x -= currentVP.w / 2;
		p.y -= currentVP.h / 2;
	}
	goal = p;
	speed = spd;
	if (speed == 0) {
		// do this instantly, even if paused
		gc->MoveViewportTo(goal, false);
		currentVP = gc->Viewport();
	}
}

void GlobalTimer::DoStep(int count)
{
	if (!ViewportIsMoving()) {
		return;
	}

	GameControl* gc = core->GetGameControl();

	Point p = currentVP.Origin();
	if (p != goal) {
		if (speed) {
			if (p.x < goal.x) {
				p.x += speed*count;
				if (p.x > goal.x) p.x = goal.x;
			} else {
				p.x -= speed*count;
				if (p.x < goal.x) p.x = goal.x;
			}
			if (p.y < goal.y) {
				p.y += speed*count;
				if (p.y > goal.y) p.y = goal.y;
			} else {
				p.y -= speed*count;
				if (p.y < goal.y) p.y = goal.y;
			}
		}

		currentVP.SetOrigin(p);
	}
	
	if (!goal.isempty() && gc->MoveViewportTo(p, false) == false) {
		goal = Point(-1,-1);
	}

	// do a possible shake in addition to the standard pan
	// this is separate because it is unbounded by the map boundaries
	// and swe assume it is possible to get a shake and pan simultaniously
	if (shakeCounter > 0) {
		shakeCounter = std::max(0, shakeCounter - count);
		if (shakeCounter) {
			Point shakeP = gc->Viewport().Origin();
			shakeP += RandomPoint(-shakeVec.x, shakeVec.x, -shakeVec.y, shakeVec.y);
			gc->MoveViewportUnlockedTo(shakeP, false);
		}
	}
}

bool GlobalTimer::UpdateViewport(unsigned long thisTime)
{
	unsigned long advance = thisTime - startTime;
	if ( advance < interval) {
		return false;
	}

	ieDword count = ieDword(advance/interval);
	DoStep(count);
	DoFadeStep(count);
	return true;
}

bool GlobalTimer::Update()
{
	Map *map;
	Game *game;
	GameControl* gc;
	unsigned long thisTime = GetTickCount();

	UpdateAnimations(false);

	if (!startTime) {
		goto end;
	}

	gc = core->GetGameControl();
	if (!gc) {
		goto end;
	}

	if (UpdateViewport(thisTime) == false) {
		return false;
	}

	game = core->GetGame();
	if (!game) {
		goto end;
	}
	map = game->GetCurrentArea();
	if (!map) {
		goto end;
	}
	//do spell effects expire in dialogs?
	//if yes, then we should remove this condition
	if (!(gc->GetDialogueFlags()&DF_IN_DIALOG) ) {
		map->UpdateFog();
		map->UpdateEffects();
		if (thisTime) {
			//this measures in-world time (affected by effects, actions, etc)
			game->AdvanceTime(1);
		}
	}
	//this measures time spent in the game (including pauses)
	if (thisTime) {
		game->RealTime++;
	}
end:
	startTime = thisTime;
	return true;
}


void GlobalTimer::DoFadeStep(ieDword count) {
	WindowManager* wm = core->GetWindowManager();
	if (fadeToCounter) {
		fadeToCounter-=count;
		if (fadeToCounter<0) {
			fadeToCounter=0;
			fadeToFactor = 1;
		}
		wm->FadeColor.a = 255 * (double( fadeToMax - fadeToCounter ) / fadeToMax / fadeToFactor);
		//bug/patch #1837747 made this unneeded
		//goto end; //hmm, freeze gametime?
	}
	//i think this 'else' is needed now because of the 'goto' cut above
	else if (fadeFromCounter!=fadeFromMax) {
		if (fadeFromCounter>fadeFromMax) {
			fadeFromCounter-=count;
			if (fadeFromCounter<fadeFromMax) {
				fadeFromCounter=fadeFromMax;
				fadeFromFactor = 1;
			}
			//don't freeze gametime when already dark
		} else {
			fadeFromCounter+=count;
			if (fadeToCounter>fadeFromMax) {
				fadeToCounter=fadeFromMax;
				fadeToFactor = 1;
			}
			wm->FadeColor.a = 255 * (double( fadeFromMax - fadeFromCounter ) / fadeFromMax / fadeFromFactor);
			//bug/patch #1837747 made this unneeded
			//goto end; //freeze gametime?
		}
	}
	if (fadeFromCounter==fadeFromMax) {
		wm->FadeColor.a = 0;
	}
}

void GlobalTimer::SetFadeToColor(unsigned long Count, unsigned short factor)
{
	if(!Count) {
		Count = 64;
	}
	fadeToCounter = Count;
	fadeToMax = fadeToCounter;
	//stay black for a while
	fadeFromCounter = 128;
	fadeFromMax = 0;
	fadeToFactor = factor;
}

void GlobalTimer::SetFadeFromColor(unsigned long Count, unsigned short factor)
{
	if(!Count) {
		Count = 64;
	}
	fadeFromCounter = 0;
	fadeFromMax = Count;
	fadeFromFactor = factor;
}

void GlobalTimer::AddAnimation(ControlAnimation* ctlanim, unsigned long time)
{
	AnimationRef* anim;
	unsigned long thisTime;

	thisTime = GetTickCount();
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
	for (std::vector<AnimationRef*>::iterator it = animations.begin() + first_animation; it != animations.end (); ++it) {
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
	for (std::vector<AnimationRef*>::iterator it = animations.begin() + first_animation; it != animations.end (); ++it) {
		if ((*it)->ctlanim == ctlanim) {
			(*it)->ctlanim = NULL;
		}
	}
}

void GlobalTimer::UpdateAnimations(bool paused)
{
	unsigned long thisTime;
	thisTime = GetTickCount();
	while (animations.begin() + first_animation != animations.end()) {
		AnimationRef* anim = animations[first_animation];
		if (anim->ctlanim == NULL) {
			first_animation++;
			continue;
		}

		if (anim->time <= thisTime) {
			anim->ctlanim->UpdateAnimation(paused);
			first_animation++;
			continue;
		}
		break;
	}
}

void GlobalTimer::ClearAnimations()
{
	first_animation = (unsigned int) animations.size();
}

void GlobalTimer::SetScreenShake(const Point &shake, int count)
{
	shakeVec.x = std::abs(shake.x);
	shakeVec.y = std::abs(shake.y);
	shakeCounter = count + 1;
}

}
