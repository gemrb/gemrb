/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */
#ifndef GLOBALTIMER_H
#define GLOBALTIMER_H

#include "exports.h"
#include "win32def.h"

#include "Region.h"

#include <vector>

namespace GemRB {

class ControlAnimation;

struct AnimationRef
{
	ControlAnimation *ctlanim;
	unsigned long  time;
};


class GEM_EXPORT GlobalTimer {
private:
	unsigned long startTime;
	unsigned long interval;

	int fadeToCounter, fadeToMax;
	int fadeFromCounter, fadeFromMax;
	int shakeCounter;
	int shakeX, shakeY;
	unsigned int first_animation;
	std::vector<AnimationRef*>  animations;
	//move viewport to this coordinate
	Point goal;
	int speed;
	Region currentVP;

	void DoFadeStep(ieDword count);
	void UpdateAnimations(bool paused);
public:
	GlobalTimer(void);
	~GlobalTimer(void);
public:
	void Init();
	void Freeze();
	bool Update();
	bool ViewportIsMoving();
	void DoStep(int count);
	void SetMoveViewPort(Point p, int spd, bool center);
	void SetFadeToColor(unsigned long Count);
	void SetFadeFromColor(unsigned long Count);
	void SetScreenShake(int shakeX, int shakeY, unsigned long Count);
	void AddAnimation(ControlAnimation* ctlanim, unsigned long time);
	void RemoveAnimation(ControlAnimation* ctlanim);
	void ClearAnimations();
};

}

#endif
