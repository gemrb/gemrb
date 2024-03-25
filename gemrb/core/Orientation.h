/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2022 The GemRB Project
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
	S		= 0,
	SSW		= 1,
	SW		= 2,
	WSW		= 3,
	W		= 4,
	WNW		= 5,
	NW		= 6,
	NNW		= 7,
	N 		= 8,
	NNE 	= 9,
	NE 		= 10,
	ENE 	= 11,
	E 		= 12,
	ESE 	= 13,
	SE 		= 14,
	SSE 	= 15,

	// this is not a valid orientation
	INVALID = 16,
	MAX = 16
};

/////maximum animation orientation count (used in many places)
#define MAX_ORIENT 0x10 // uint8_t(orient_t::MAX)

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

	// Approximates atan2(y, x) normalized to the [0,4) range
	// with a maximum error of 0.1620 degrees
	// rcor's rational approximation that can be arbitrarily
	// improved if we ever want greater precision
	auto pseudoAtan2 = [](float y, float x) {
		static const uint32_t signMask = 0x80000000;
		static const float b = 0.596227F;

		// Extract the sign bits
		uint32_t xS = signMask & (uint32_t&) x;
		uint32_t yS = signMask & (uint32_t&) y;

		// Determine the quadrant offset
		float q = (float) ((~xS & yS) >> 29 | xS >> 30);

		// Calculate the arctangent in the first quadrant
		float bxy = std::fabs(b * x * y);
		float num = bxy + y * y;
		float atan1q = num / (x * x + bxy + num);

		// Translate it to the proper quadrant
		uint32_t uatan2q = (xS ^ yS) | (uint32_t&) atan1q;
		return q + (float&) uatan2q;
	};

	// reverse Y to match our game coordinate system
	float angle = pseudoAtan2(-deltaY, deltaX) * M_PI_2;

	// calculate which segment the angle falls into and which orientation that represents
	constexpr float M_PI_8 = M_PI / 8;
	float segment = std::fmod(angle + M_PI_8 / 2 + 2 * M_PI, 2 * M_PI);
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

inline orient_t GetNextFace(orient_t old, orient_t next) {
	if (old != next) {
		if (ClampToOrientation(next - old) <= MAX_ORIENT / 2) {
			old = NextOrientation(old);
		} else {
			old = PrevOrientation(old);
		}
	}

	return old;
}

} // namespace GemRB

#endif
