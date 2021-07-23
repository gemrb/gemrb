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

#include "Holder.h"
#include "Region.h"

#include "globals.h"

namespace GemRB {

template <class T>
class GUIAnimation {
protected:
	tick_t begintime;
	T current;
	
public:
	GUIAnimation() {
		begintime = GetTicks();
	}
	
	virtual ~GUIAnimation() = default;
	
	explicit operator bool() const {
		return !HasEnded();
	}

	T Next(tick_t time) {
		if (HasEnded() == false) {
			current = GenerateNext(time);
		}
		return current;
	}

	T Current() {
		return current;
	}

private:
	virtual T GenerateNext(tick_t time)=0;
	virtual bool HasEnded() const=0;
};

class PointAnimation : public GUIAnimation<Point> {
public:
	Point begin, end;
	tick_t endtime;
	
public:
	PointAnimation() : GUIAnimation(), endtime(0) {}
	
	PointAnimation(const Point& begin, const Point& end, tick_t duration)
	: GUIAnimation(), begin(begin), end(end), endtime(begintime + duration) {
		Next(begintime);
	}

private:
	Point GenerateNext(tick_t time) override;
	bool HasEnded() const override;
};

class ColorCycle {
	uint8_t step;
	uint8_t speed;

public:
	explicit ColorCycle(uint8_t speed) : step(0), speed(speed) {}

	void AdvanceTime(tick_t time);
	Color Blend(const Color& c1, const Color& c2) const;

	uint8_t Step() const {
		return step;
	}
};

// this is used to syncronize the various "selected" Actor color animations
extern ColorCycle GlobalColorCycle;

// This is supposed to be a fast optionally infinitely repeating transition
// between 2 colors. We will create a global instance to syncronize many elements with the same animation.
class ColorAnimation : public GUIAnimation<Color> {
public:
	Color begin, end;
	bool repeat;
	ColorCycle cycle;
	int timeOffset;

public:
	ColorAnimation()
	: GUIAnimation(), cycle(0) {
		repeat = false;
		timeOffset = 0;
	}

	ColorAnimation(const Color& begin, const Color& end, bool repeat)
	: GUIAnimation(), begin(begin), end(end), repeat(repeat), cycle(7) {
		// we don't handle alpha, if we need it revisit this.
		this->begin.a = 0xff;
		this->end.a = 0xff;

		tick_t time = GetTicks();
		timeOffset = (time >> 7) & 7; // we want to start at the frame that is 0
	}

private:
	Color GenerateNext(tick_t time) override;
	bool HasEnded() const override;
};

class AnimationFactory;
class Sprite2D;

class GEM_EXPORT SpriteAnimation : public GUIAnimation<Holder<Sprite2D>> {
private:
	AnimationFactory* bam = nullptr;
	uint8_t cycle = 0;
	uint8_t frame = 0;
	unsigned int anim_phase = 0;
	bool has_palette = false;
	bool is_blended = false;
	ieDword colors[8] {};
	tick_t nextFrameTime = 0;

	tick_t CalculateNextFrameDelta();
	Holder<Sprite2D> GenerateNext(tick_t time) override;
public:
	explicit SpriteAnimation(AnimationFactory* af, int Cycle = 0);
	//report if the current resource is the same as descripted by the params
	void SetPaletteGradients(const ieDword *col);
	bool SameResource(const SpriteAnimation*) const;
	void SetBlend(bool b);
	bool HasEnded() const override;

	tick_t Time() const { return nextFrameTime; }
	
	enum RepeatFlags : uint8_t {
		PLAY_NORMAL		= 0,  // play in a loop
		PLAY_RANDOM		= 1,
		PLAY_ONCE		= 2,  // play and stop at end
		PLAY_ALWAYS		= 4   // play even when game is paused
	} flags = PLAY_NORMAL;
};

}
	
#endif /* Animations_h */
