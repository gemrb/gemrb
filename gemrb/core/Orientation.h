// SPDX-FileCopyrightText: 2022 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef ORIENT_H
#define ORIENT_H

#include "Geometry.h"
#include "RNG.h"

#include <cmath>
#include <cstdint>

namespace GemRB {

enum orient_t : uint8_t {
	/*
	WEST PART  |  EAST PART
			   |
	  NW  NNW  N  NNE  NE
	NW 006 007 008 009 010 NE
	WNW 005    |      011 ENE
	W 004     xxx     012 E
	WSW 003    |      013 ESE
	SW 002 001 000 015 014 SE
	  SW  SSW  S  SSE  SE
			   |
			   |
	*/
	S = 0,
	SSW = 1,
	SW = 2,
	WSW = 3,
	W = 4,
	WNW = 5,
	NW = 6,
	NNW = 7,
	N = 8,
	NNE = 9,
	NE = 10,
	ENE = 11,
	E = 12,
	ESE = 13,
	SE = 14,
	SSE = 15,

	// this is not a valid orientation
	INVALID = 16,
	MAX = 16
};

/////maximum animation orientation count (used in many places)
#define MAX_ORIENT 0x10 // uint8_t(orient_t::MAX)

static const unsigned char SixteenToNine[MAX_ORIENT] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 7, 6, 5, 4, 3, 2, 1 };
static const unsigned char SixteenToFive[MAX_ORIENT] = { 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 3, 3, 2, 2, 1, 1 };

inline orient_t RandomOrientation()
{
	return orient_t(RAND(0, MAX_ORIENT - 1));
}

inline orient_t ClampToOrientation(int i)
{
	return orient_t(i & (MAX_ORIENT - 1));
}

// some animations are only 8 orientations
inline orient_t ReduceToHalf(orient_t orient)
{
	return ClampToOrientation(orient & ~1);
}

// reflects over the center
inline orient_t ReflectOrientation(orient_t orient)
{
	return orient_t(orient ^ 8);
}

// reflects over the vertical axis
inline orient_t FlipOrientation(orient_t orient)
{
	return orient_t(16 - orient);
}

// clockwise
inline orient_t NextOrientation(orient_t orient, int step = 1)
{
	return ClampToOrientation(orient + step);
}

// counter clockwise
inline orient_t PrevOrientation(orient_t orient, int step = 1)
{
	return ClampToOrientation(orient - step);
}

/** Calculates the orientation of a character (or projectile) facing a point */
inline orient_t GetOrient(const Point& from, const Point& to)
{
	int deltaX = to.x - from.x;
	int deltaY = to.y - from.y;
	if (!deltaX) return deltaY >= 0 ? S : N;

	// reverse Y to match our game coordinate system
	float_t angle = AngleFromPoints(-deltaY, deltaX);

	// calculate which segment the angle falls into and which orientation that represents
	constexpr float_t M_PI_8 = M_PI / 8;
	float_t segment = std::fmod(angle + M_PI_8 / 2 + 2 * M_PI, 2.0F * M_PI);
	return PrevOrientation(E, int(segment / M_PI_8));
}

inline Point OrientedOffset(orient_t face, int offset)
{
	int x;
	int y;
	if (face >= WNW && face <= ENE) {
		y = -1;
	} else if (face == W || face == E) {
		y = 0;
	} else {
		y = 1;
	}
	if (face >= SSW && face <= NNW) {
		x = -1;
	} else if (face == S || face == N) {
		x = 0;
	} else {
		x = 1;
	}

	return Point(x * offset, y * offset);
}

inline orient_t GetNextFace(orient_t old, orient_t next)
{
	if (old != next) {
		if (ClampToOrientation(next - old) <= MAX_ORIENT / 2) {
			old = NextOrientation(old);
		} else {
			old = PrevOrientation(old);
		}
	}

	return old;
}

// orientations start at 270° with S and go clockwise, while
// math functions expect counterclockwise starting from E
inline orient_t GetMathyOrientation(orient_t face)
{
	return PrevOrientation(E, face);
}

} // namespace GemRB

#endif
