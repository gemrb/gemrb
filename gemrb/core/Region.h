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
 * @file Region.h
 * Declares Point and Region, 2d-space primitive types
 * @author The GemRB Project
 */


#ifndef REGION_H
#define REGION_H

#include "exports.h"
#include "ie_types.h"

namespace GemRB {

/**
 * @class Point
 * Point in 2d-space: [x, y]
 */

class GEM_EXPORT Point {
public:
	Point(void);
	Point(short x, short y);

	bool operator==(const Point &pnt) const;
	bool operator!=(const Point &pnt) const;

	Point operator+(const Point& p) const;
	Point operator-(const Point& p) const;

	/** if it is [-1.-1] */
	bool isempty() const;
	/** if it is [0.0] */
	bool isnull() const;
	inline void empty() {
		x=-1;
		y=-1;
	}
	inline void null() {
		x=0;
		y=0;
	}
	bool PointInside(const Point &p) const;

	ieDword asDword() const; // store coordinates in uint32 ((y << 16) | x)
	void fromDword(ieDword val); // extract coordinates from uint32

	short x,y;
};

class GEM_EXPORT Size {
public:
	int w, h;
	Size();
	Size(int, int);

	bool operator==(const Size& size);
	bool operator!=(const Size& size);
	int Area() const { return w * h; }
	bool IsZero() const { return w == 0 && h == 0; }
	bool IsEmpty() const { return w <= 0 || h <= 0; }
};

/**
 * @class Region
 * Rectangular area in 2d-space [x, y] - [x+w, y+h]
 */

class GEM_EXPORT Region {
public:
	int x, y;
	int w, h;
	Region(void);
	Region(int x, int y, int w, int h);
	Region(const Point& p, const Size& s);

	bool operator==(const Region& rgn);
	bool operator!=(const Region& rgn);
	bool PointInside(short XPos, short YPos) const;
	bool PointInside(const Point &p) const;
	bool InsideRegion(const Region& rgn) const;
	bool IntersectsRegion(const Region& rgn) const;
	Region Intersect(const Region& rgn) const;

	void Normalize();
	Point Origin() const { return Point(x, y); }
	Size Dimensions() const { return Size(w, h); }
};

}

#endif  // ! REGION_H
