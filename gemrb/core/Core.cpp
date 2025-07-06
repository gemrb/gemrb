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

/**
 * @file Core.cpp
 * Some compatibility and utility functions
 * @author The GemRB Project
 */

#include "globals.h"
#include "ie_stats.h"

#include "Interface.h"

#include "Scriptable/Actor.h"

#include <cmath>

#ifdef WIN32

BOOL WINAPI DllEntryPoint(HINSTANCE, DWORD, LPVOID);
BOOL WINAPI DllEntryPoint(HINSTANCE /*hinstDLL*/, DWORD /*fdwReason*/,
			  LPVOID /*lpvReserved*/)
{
	return true;
}

#endif

namespace GemRB {

//// Globally used functions

/** Calculates distance between 2 points */
unsigned int Distance(const Point& p, const Scriptable* b)
{
	long x = p.x - b->Pos.x;
	long y = p.y - b->Pos.y;
	return (unsigned int) std::hypot(x, y);
}

constexpr int DistanceFactor = 4; // ignore angle, go for the bigger size between [3, 4]
unsigned int PersonalDistance(const Point& p, const Scriptable* b)
{
	long x = p.x - b->Pos.x;
	long y = p.y - b->Pos.y;
	auto ret = std::hypot(x, y);
	if (b->Type == ST_ACTOR) {
		ret -= static_cast<const Actor*>(b)->CircleSize2Radius() * DistanceFactor;
	}
	if (ret < 0) return (unsigned int) 0;
	return (unsigned int) ret;
}

constexpr int SquaredDistanceFactor = 14; // ignore angle, roughly middle of an elliptic [9, 16]
unsigned int SquaredPersonalDistance(const Point& p, const Scriptable* b)
{
	long x = p.x - b->Pos.x;
	long y = p.y - b->Pos.y;
	int ret = static_cast<int>(x * x + y * y);
	if (b->Type == ST_ACTOR) {
		ret -= static_cast<const Actor*>(b)->CircleSize2Radius() * SquaredDistanceFactor;
	}
	if (ret < 0) return (unsigned int) 0;
	return (unsigned int) ret;
}

/** Calculates distance between 2 scriptables */
unsigned int Distance(const Scriptable* a, const Scriptable* b)
{
	long x = a->Pos.x - b->Pos.x;
	long y = a->Pos.y - b->Pos.y;
	return (unsigned int) std::hypot(x, y);
}

/** Calculates distance squared between 2 scriptables */
unsigned int SquaredDistance(const Scriptable* a, const Scriptable* b)
{
	return SquaredDistance(a->Pos, b->Pos);
}

/** Calculates distance between 2 scriptables, including feet circle if applicable */
unsigned int PersonalDistance(const Scriptable* a, const Scriptable* b)
{
	long x = a->Pos.x - b->Pos.x;
	long y = a->Pos.y - b->Pos.y;
	auto ret = std::hypot(x, y);
	if (a->Type == ST_ACTOR) {
		ret -= static_cast<const Actor*>(a)->CircleSize2Radius() * DistanceFactor;
	}
	if (b->Type == ST_ACTOR) {
		ret -= static_cast<const Actor*>(b)->CircleSize2Radius() * DistanceFactor;
	}
	if (ret < 0) return (unsigned int) 0;
	return (unsigned int) ret;
}

unsigned int SquaredPersonalDistance(const Scriptable* a, const Scriptable* b)
{
	long x = a->Pos.x - b->Pos.x;
	long y = a->Pos.y - b->Pos.y;
	int ret = static_cast<int>(x * x + y * y);
	if (a->Type == ST_ACTOR) {
		ret -= static_cast<const Actor*>(a)->CircleSize2Radius() * SquaredDistanceFactor;
	}
	if (b->Type == ST_ACTOR) {
		ret -= static_cast<const Actor*>(b)->CircleSize2Radius() * SquaredDistanceFactor;
	}
	if (ret < 0) return (unsigned int) 0;
	return (unsigned int) ret;
}

unsigned int PersonalLineDistance(const Point& v, const Point& w, const Scriptable* s, float_t* proj)
{
	float_t t;
	Point p;

	int len = SquaredDistance(w, v);
	if (len == 0) {
		// that's a short line...
		t = 0.0;
		p = v;
	} else {
		// find the projection of Pos onto the line
		t = ((s->Pos.x - v.x) * (w.x - v.x) + (s->Pos.y - v.y) * (w.y - v.y)) / (float_t) len;
		if (t < 0.0) {
			// projection beyond the v end of the line
			p = v;
		} else if (t > 1.0) {
			// projection beyond the w end of the line
			p = w;
		} else {
			// projection on the line
			p.x = static_cast<int>(v.x + (w.x - v.x) * t);
			p.y = static_cast<int>(v.y + (w.y - v.y) * t);
		}
	}

	if (proj != NULL) {
		(*proj) = t;
	}
	return PersonalDistance(p, s);
}

// What's the deal with the spell and item ranges in their descriptions being in feet?
//
// Kjeron provided these notes:
// It had no standards - spell descriptions were copied straight from their source material.
// In game it ranged from 3.57 units per foot, to 28.57 units per foot, but most often around
// 8.5 units per foot, which was still wrong. The descriptions have mostly been fixed in the
// EE's, albeit with some rounding.
//
// 16 horizontal pixels = 1 foot
// 12 vertical pixels = 1 foot
//
// Projectile trap/explosion size is based on horizontal distance, 16 = 16h pixels = 1 foot radius
// 80 = 5' radius
// 256 = 16' radius
// 448 = 28' radius (max engine can handle for repeating AoE's)
// 480 = 30' radius
//
// Visual/Script Range = 448h/336v pixels (28 feet) (I don't know why it's often listed as 30')
// Spellcasting Range = 16h/12v pixels (1 foot) per unit
// Opcode 262 (Visual Range) = 32h/24v pixels (2 feet) per unit
//
// The one thing that is 30' (480h/360v pixels) is PC movement rate per round, at least in original BG1,
// BG2 bumped it up by 50% to 45' per round.

// 1 foot = 16px horizontally, 12px vertically and all in between
// we use them to construct an ellipse and through its central angle get the adjusted "radius"
// Potential optimisation through precomputing:
// rounding the angle into 5° intervals [0-90] gives these values per foot:
// 16 16 16 16 15 15 15 14 14 14 13 13 13 12 12 12 12 12 12
float_t Feet2Pixels(int feet, float_t angle)
{
	float_t sin2 = std::pow(std::sin(angle) / 12, 2);
	float_t cos2 = std::pow(std::cos(angle) / 16, 2);
	float_t r = std::sqrt(1 / (cos2 + sin2));
	return r * feet;
}

// Audible range differed between games:
// - bg1: fixed 28 foot radius
// - bg2, iwds: 1.5 * visual range (by default 42 feet)
// - ee: same, but configurable factor "Audible Range" in baldur.lua (48 / 32 == 1.5)
// - pst/ee: visual range (by default 30 feet).
bool WithinAudibleRange(const Actor* actor, const Point& dest)
{
	int distance;
	if (core->HasFeature(GFFlags::BETTER_OF_HEARING)) {
		distance = (3 * actor->GetVisualRange()) / 2;
	} else if (core->HasFeature(GFFlags::PST_STATE_FLAGS)) {
		distance = actor->GetVisualRange();
	} else {
		distance = 28;
	}
	return WithinRange(actor, dest, distance);
}

bool WithinRange(const Scriptable* actor, const Point& dest, int distance)
{
	float_t angle = AngleFromPoints(actor->Pos, dest);
	return Distance(dest, actor) <= Feet2Pixels(distance, angle);
}

bool WithinPersonalRange(const Scriptable* actor, const Point& dest, int distance)
{
	float_t angle = AngleFromPoints(actor->Pos, dest);
	return PersonalDistance(dest, actor) <= Feet2Pixels(distance, angle);
}

bool WithinPersonalRange(const Scriptable* scr1, const Scriptable* scr2, int distance)
{
	float_t angle = AngleFromPoints(scr1->Pos, scr2->Pos);
	return PersonalDistance(scr2, scr1) <= Feet2Pixels(distance, angle);
}

// returns EA relation between two scriptables (non actors are always enemies)
// it is used for protectile targeting/iwd ids targeting too!
int EARelation(const Scriptable* Owner, const Actor* target)
{
	ieDword eao = EA_ENEMY;
	const Actor* actor = Scriptable::As<const Actor>(Owner);
	if (actor) {
		eao = actor->GetStat(IE_EA);
	}

	ieDword eat = target->GetStat(IE_EA);

	if (eao <= EA_GOODCUTOFF) {
		if (eat <= EA_GOODCUTOFF) {
			return EAR_FRIEND;
		}
		if (eat >= EA_EVILCUTOFF) {
			return EAR_HOSTILE;
		}

		return EAR_NEUTRAL;
	}

	if (eao >= EA_EVILCUTOFF) {
		if (eat <= EA_GOODCUTOFF) {
			return EAR_HOSTILE;
		}
		if (eat >= EA_EVILCUTOFF) {
			return EAR_FRIEND;
		}

		return EAR_NEUTRAL;
	}

	return EAR_NEUTRAL;
}

// check whether the game time matches a given schedule
// hours are offset by half an hour, ie. bit 0 covers the
// time from 0030 to 0129, bit 1 from 0130 to 0229 etc.
bool Schedule(ieDword schedule, ieDword time)
{
	return (schedule & SCHEDULE_MASK(time)) != 0;
}

}
