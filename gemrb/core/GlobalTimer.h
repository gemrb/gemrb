// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef GLOBALTIMER_H
#define GLOBALTIMER_H

#include "exports.h"
#include "globals.h"

#include "Region.h"

namespace GemRB {

class GEM_EXPORT GlobalTimer {
private:
	tick_t startTime = 0; //forcing an update;
	tick_t lastFadeDuration = 0;

	tick_t fadeToCounter = 0;
	tick_t fadeToMax = 0;
	tick_t fadeOutFallback = 0;
	tick_t fadeFromCounter = 0;
	tick_t fadeFromMax = 0;
	unsigned short fadeToFactor = 1; // divisor to limit maximum target opacity by
	unsigned short fadeFromFactor = 1; // divisor to limit starting opacity by
	int shakeCounter = 0;
	Point shakeVec;

	//move viewport to this coordinate
	Point goal = Point(-1, -1);
	int speed = 0;
	Region currentVP;

	void DoFadeStep(ieDword count);
	bool UpdateViewport(tick_t time);

public:
	GlobalTimer() noexcept = default;

	GlobalTimer(GlobalTimer&&) noexcept = default;
	GlobalTimer& operator=(GlobalTimer&&) noexcept = default;

	void Freeze();
	bool Update();
	bool IsFading() const;
	bool ViewportIsMoving() const;
	void DoStep(int count);
	void SetMoveViewPort(Point p, int spd, bool center);
	void SetFadeToColor(tick_t Count, unsigned short factor = 1);
	void SetFadeFromColor(tick_t Count, unsigned short factor = 1);
	void SetScreenShake(const Point&, int Count);
};

}

#endif
