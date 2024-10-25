/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2017 The GemRB Project
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
 */

#ifndef Animations_h
#define Animations_h

#include "globals.h"

#include "Animation.h"
#include "Holder.h"
#include "Region.h"
#include "Sprite2D.h"

namespace GemRB {

template<class T>
class GUIAnimation {
protected:
	tick_t begintime = 0;
	T current;

public:
	explicit GUIAnimation(tick_t begin) noexcept
		: begintime(begin) {}

	GUIAnimation() noexcept
		: GUIAnimation(GetMilliseconds()) {}

	virtual ~GUIAnimation() noexcept = default;

	explicit operator bool() const
	{
		return !HasEnded();
	}

	T Next(tick_t time)
	{
		if (HasEnded() == false) {
			current = GenerateNext(time);
		}
		return current;
	}

	T Current()
	{
		return current;
	}

private:
	virtual T GenerateNext(tick_t time) = 0;
	virtual bool HasEnded() const = 0;
};

class GEM_EXPORT PointAnimation : public GUIAnimation<Point> {
public:
	Point begin;
	Point end;
	tick_t endtime = 0;

public:
	PointAnimation() = default;

	PointAnimation(const Point& begin, const Point& end, tick_t duration)
		: GUIAnimation(), begin(begin), end(end), endtime(begintime + duration)
	{
		Next(begintime);
	}

private:
	Point GenerateNext(tick_t time) override;
	bool HasEnded() const override;
};

class GEM_EXPORT ColorCycle {
	uint8_t step = 0;
	uint8_t speed = 0;

public:
	explicit ColorCycle(uint8_t speed)
		: speed(speed) {}

	void AdvanceTime(tick_t time);
	Color Blend(const Color& c1, const Color& c2) const;

	uint8_t Step() const
	{
		return step;
	}
};

// this is used to synchronize the various "selected" Actor color animations
extern ColorCycle GlobalColorCycle;

// This is supposed to be a fast optionally infinitely repeating transition
// between 2 colors. We will create a global instance to synchronize many elements with the same animation.
class ColorAnimation : public GUIAnimation<Color> {
public:
	Color begin;
	Color end;
	bool repeat = false;
	ColorCycle cycle { 0 };
	int timeOffset = 0;

public:
	ColorAnimation() = default;

	ColorAnimation(const Color& begin, const Color& end, bool repeat)
		: GUIAnimation(), begin(begin), end(end), repeat(repeat), cycle(7)
	{
		// we don't handle alpha, if we need it revisit this.
		this->begin.a = 0xff;
		this->end.a = 0xff;

		tick_t time = GetMilliseconds();
		timeOffset = (time >> 7) & 7; // we want to start at the frame that is 0
	}

private:
	Color GenerateNext(tick_t time) override;
	bool HasEnded() const override;
};

class GEM_EXPORT SpriteAnimation : public GUIAnimation<Holder<Sprite2D>> {
private:
	std::shared_ptr<Animation> anim;

	Holder<Sprite2D> GenerateNext(tick_t time) override;

public:
	explicit SpriteAnimation(std::shared_ptr<Animation> anim);

	bool HasEnded() const override;

	Animation::Flags& flags;
	bool& gameAnimation;
};

}

#endif /* Animations_h */
