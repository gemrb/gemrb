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
#include "globals.h"

#include "Region.h"

namespace GemRB {

class GEM_EXPORT GlobalTimer {
private:
	tick_t startTime = 0; //forcing an update;
	tick_t interval;

	tick_t fadeToCounter = 0, fadeToMax = 0;
	tick_t fadeFromCounter = 0, fadeFromMax = 0;
	unsigned short fadeToFactor = 1, fadeFromFactor = 1;
	int shakeCounter = 0;
	Point shakeVec;

	//move viewport to this coordinate
	Point goal = Point(-1, -1);
	int speed = 0;
	Region currentVP;

	void DoFadeStep(ieDword count);
	bool UpdateViewport(tick_t time);
public:
	GlobalTimer(void);
	
	GlobalTimer(GlobalTimer&&) = default;
	GlobalTimer& operator=(GlobalTimer&&) = default;

	void Freeze();
	bool Update();
	bool ViewportIsMoving() const;
	void DoStep(int count);
	void SetMoveViewPort(Point p, int spd, bool center);
	void SetFadeToColor(tick_t Count, unsigned short factor = 1);
	void SetFadeFromColor(tick_t Count, unsigned short factor = 1);
	void SetScreenShake(const Point&, int Count);
};

}

#endif
