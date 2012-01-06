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
#include "Video.h"
#include "GUI/GameControl.h"

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
	waitCounter = 0;
	shakeCounter = 0;
	startTime = 0; //forcing an update
	speed = 0;
	ClearAnimations();
}

void GlobalTimer::Freeze()
{
	unsigned long thisTime;
	unsigned long advance;

	thisTime = GetTickCount();
	advance = thisTime - startTime;
	if ( advance < interval) {
		return;
	}
	startTime = thisTime;
	Game* game = core->GetGame();
	if (!game) {
		return;
	}
	game->RealTime++;

	ieDword count = advance/interval;
	// pst/bg2 do this, if you fix it for another game, wrap it in a check
	DoFadeStep(count);

	// show scrolling cursor while paused
	GameControl* gc = core->GetGameControl();
	if (gc)
		gc->UpdateScrolling();
}

bool GlobalTimer::ViewportIsMoving()
{
	return (goal.x!=currentVP.x) || (goal.y!=currentVP.y);
}

void GlobalTimer::SetMoveViewPort(ieDword x, ieDword y, int spd, bool center)
{
	speed=spd;
	currentVP=core->GetVideoDriver()->GetViewport();
	if (center) {
		x-=currentVP.w/2;
		y-=currentVP.h/2;
	}
	goal.x=(short) x;
	goal.y=(short) y;
}

void GlobalTimer::DoStep(int count)
{
	Video *video = core->GetVideoDriver();

	int x = currentVP.x;
	int y = currentVP.y;
	if ( (x != goal.x) || (y != goal.y)) {
		if (speed) {
			if (x<goal.x) {
				x+=speed;
				if (x>goal.x) x=goal.x;
			} else {
				x-=speed;
				if (x<goal.x) x=goal.x;
			}
			if (y<goal.y) {
				y+=speed;
				if (y>goal.y) y=goal.y;
			} else {
				y-=speed;
				if (y<goal.y) y=goal.y;
			}
		} else {
			x=goal.x;
			y=goal.y;
		}
		currentVP.x=x;
		currentVP.y=y;
	}

	if (shakeCounter) {
		shakeCounter-=count;
		if (shakeCounter<0) {
			shakeCounter=0;
		}
		if (shakeCounter) {
			//x += (rand()%shakeX) - (shakeX>>1);
			//y += (rand()%shakeY) - (shakeY>>1);
			if (shakeX) {
				x += rand()%shakeX;
			}
			if (shakeY) {
				y += rand()%shakeY;
			}
		}
	}
	video->MoveViewportTo(x,y);
}

bool GlobalTimer::Update()
{
	Map *map;
	Game *game;
	GameControl* gc;
	unsigned long thisTime;
	unsigned long advance;

	gc = core->GetGameControl();
	if (gc)
		gc->UpdateScrolling();

	UpdateAnimations();

	thisTime = GetTickCount();

	if (!startTime) {
		startTime = thisTime;
		return false;
	}

	advance = thisTime - startTime;
	if ( advance < interval) {
		return false;
	}
	ieDword count = advance/interval;
	DoStep(count);
	DoFadeStep(count);
	if (!gc) {
		goto end;
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
	Video *video = core->GetVideoDriver();
	if (fadeToCounter) {
		fadeToCounter-=count;
		if (fadeToCounter<0) {
			fadeToCounter=0;
		}
		video->SetFadePercent( ( ( fadeToMax - fadeToCounter ) * 100 ) / fadeToMax );
		//bug/patch #1837747 made this unneeded
		//goto end; //hmm, freeze gametime?
	}
	//i think this 'else' is needed now because of the 'goto' cut above
	else if (fadeFromCounter!=fadeFromMax) {
		if (fadeFromCounter>fadeFromMax) {
			fadeFromCounter-=count;
			if (fadeFromCounter<fadeFromMax) {
				fadeFromCounter=fadeFromMax;
			}
			//don't freeze gametime when already dark
		} else {
			fadeFromCounter+=count;
			if (fadeToCounter>fadeFromMax) {
				fadeToCounter=fadeFromMax;
			}
			video->SetFadePercent( ( ( fadeFromMax - fadeFromCounter ) * 100 ) / fadeFromMax );
			//bug/patch #1837747 made this unneeded
			//goto end; //freeze gametime?
		}
	}
	if (fadeFromCounter==fadeFromMax) {
		video->SetFadePercent( 0 );
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
	thisTime = GetTickCount();
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
	first_animation = (unsigned int) animations.size();
}

void GlobalTimer::SetScreenShake(int shakeX, int shakeY,
	unsigned long Count)
{
	this->shakeX = shakeX;
	this->shakeY = shakeY;
	shakeCounter = Count+1;
}
