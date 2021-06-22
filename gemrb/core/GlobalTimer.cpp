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

#include "Game.h"
#include "Interface.h"
#include "GUI/GUIAnimation.h"
#include "GUI/GameControl.h"
#include "RNG.h"

namespace GemRB {

GlobalTimer::GlobalTimer(void)
{
	//AI_UPDATE_TIME: how many AI updates in a second
	interval = ( 1000 / AI_UPDATE_TIME );
	ClearAnimations();
}

GlobalTimer::~GlobalTimer(void)
{
	auto it = animations.begin();
	while (it != animations.end ()) {
		SpriteAnimation* anim = *it;
		it = animations.erase(it);
		delete anim;
	}
}

void GlobalTimer::Freeze()
{
	tick_t thisTime = GetTicks();
	UpdateAnimations(true, thisTime);

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
	return shakeCounter || (!goal.IsInvalid() && goal != currentVP.origin);
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
		goal = Point(-1,-1);
		return;
	}

	GameControl* gc = core->GetGameControl();

	Point p = currentVP.origin;
	if (p != goal && !goal.IsInvalid()) {
		int d = Distance(goal, p);
		int magnitude = count * speed;
		
		if (d <= magnitude) {
			p = goal;
		} else {
			magnitude = std::min(magnitude, d);
			float r = magnitude / float(d);
			p.x = (1 - r) * p.x + r * goal.x;
			p.y = (1 - r) * p.y + r * goal.y;
		}
	}
	
	if (!gc->MoveViewportTo(p, false)) {
		// we have moved as close to the goal as possible
		// something must have set a goal beyond the map
		// update the goal just in case we are in a shake
		goal = gc->Viewport().origin;
	}

	// do a possible shake in addition to the standard pan
	// this is separate because it is unbounded by the map boundaries
	// and we assume it is possible to get a shake and pan simultaniously
	if (shakeCounter > 0) {
		shakeCounter = std::max(0, shakeCounter - count);
		if (shakeCounter) {
			Point shakeP = gc->Viewport().origin;
			shakeP += RandomPoint(-shakeVec.x, shakeVec.x, -shakeVec.y, shakeVec.y);
			gc->MoveViewportUnlockedTo(shakeP, false);
		}
	}
	
	currentVP = gc->Viewport();
}

bool GlobalTimer::UpdateViewport(tick_t thisTime)
{
	tick_t advance = thisTime - startTime;
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
	tick_t thisTime = GetTicks();

	UpdateAnimations(false, thisTime);

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
		
		if (count > fadeToCounter) {
			fadeToCounter = 0;
			fadeToFactor = 1;
		} else {
			fadeToCounter -= count;
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

void GlobalTimer::SetFadeToColor(tick_t Count, unsigned short factor)
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

void GlobalTimer::SetFadeFromColor(tick_t Count, unsigned short factor)
{
	if(!Count) {
		Count = 64;
	}
	fadeFromCounter = 0;
	fadeFromMax = Count;
	fadeFromFactor = factor;
}

void GlobalTimer::AddAnimation(SpriteAnimation* anim)
{
	// if there are no free animation reference objects,
	// alloc one, else take the first free one
	if (first_animation != 0) {
		animations.erase (animations.begin());
		first_animation--;
	}

	// and insert it into list of other anim refs, sorted by time
	auto it = animations.begin() + first_animation;
	for (; it != animations.end (); ++it) {
		if ((*it)->Time() > anim->Time()) {
			animations.insert(it, anim);
			break;
		}
	}
	if (it == animations.end())
		animations.push_back(anim);
}

void GlobalTimer::RemoveAnimation(SpriteAnimation* anim)
{
	// Animation refs for given control are not physically removed,
	// but just marked by erasing ptr to the control. They will be
	// collected when they get to the front of the vector
	for (auto it = animations.begin() + first_animation; it != animations.end (); ++it) {
		if (*it == anim) {
			*it = nullptr;
		}
	}
}

void GlobalTimer::UpdateAnimations(bool paused, tick_t thisTime)
{
	while (animations.begin() + first_animation != animations.end()) {
		SpriteAnimation* anim = animations[first_animation];
		if (anim == nullptr) {
			first_animation++;
			continue;
		}

		if (anim->Time() <= thisTime) {
			anim->UpdateAnimation(paused, thisTime);
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
	
	if (goal.IsInvalid()) {
		GameControl* gc = core->GetGameControl();
		currentVP = gc->Viewport();
		goal = currentVP.origin;
		speed = 1000;
	}
}

}
