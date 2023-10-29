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
inline orient_t GetOrient(const Point &s, const Point &d)
{
	static const orient_t orientations[25]={
		NW, NNW, N, NNE, NE,
		WNW, NW, N, NE, ENE,
		W, W, S, E, E,
		WSW, SW, S, SE, ESE,
		SW, SSW, S, SSE, SE
	};

	int deltaX = s.x - d.x;
	int deltaY = s.y - d.y;
	int div = Distance(s,d);
	if(!div) return S;
	if(div>3) div/=2;
	int aX=deltaX/div;
	int aY=deltaY/div;
	return orientations[(aY+2)*5+aX+2];
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
