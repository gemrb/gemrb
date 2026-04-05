// SPDX-FileCopyrightText: 2016 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef TIMER_H
#define TIMER_H

#include "globals.h"

#include "Callback.h"

namespace GemRB {

class GEM_EXPORT Timer {
public:
	using TimeInterval = tick_t;

private:
	TimeInterval interval;
	TimeInterval fireDate;
	bool valid;
	EventHandler action;
	int repeats;

private:
	void NextFireDate()
	{
		fireDate = GetMilliseconds() + interval;
	}

public:
	Timer(TimeInterval i, const EventHandler& handler, int repeats = -1)
		: action(handler), repeats(repeats)
	{
		valid = true;
		interval = i;
		NextFireDate();
	}

	~Timer()
	{
		Invalidate();
	}

	void SetInterval(TimeInterval i)
	{
		fireDate += (i - interval);
		interval = i;
	}

	void Update(TimeInterval time)
	{
		if (fireDate <= time) {
			action();
			if (repeats != 0) {
				if (repeats > 0) {
					--repeats;
				}
				NextFireDate();
			} else {
				Invalidate();
			}
		}
	}

	void Invalidate()
	{
		valid = false;
		action = nullptr;
	}

	bool IsRunning() const
	{
		return valid;
	}
};

}

#endif
