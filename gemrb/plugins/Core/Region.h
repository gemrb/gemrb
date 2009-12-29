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
 * $Id$
 *
 */

/**
 * @file Region.h
 * Declares Point and Region, 2d-space primitive types
 * @author The GemRB Project
 */


#ifndef REGION_H
#define REGION_H

#include "../../includes/exports.h"
#include "../../includes/ie_types.h"


/**
 * @class Point
 * Point in 2d-space: [x, y]
 */

class GEM_EXPORT Point {
public:
	Point(void);
	Point(const Point& pnt);
	Point(short x, short y);
	~Point(void);

	Point& operator=(const Point& pnt);
	bool operator==(const Point &pnt);
	bool operator!=(const Point &pnt);

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
	bool PointInside(Point &p);

	ieDword asDword() const; // store coordinates in uint32 ((y << 16) | x)
	void fromDword(ieDword val); // extract coordinates from uint32

	short x,y;
};


/**
 * @class Region
 * Rectangular area in 2d-space [x, y] - [x+w, y+h]
 */

class GEM_EXPORT Region {
public:
	Region(void);
	~Region(void);
	int x;
	int y;
	int w;
	int h;
	Region(const Region& rgn);
	Region(const Point& p, int w, int h);
	Region& operator=(const Region& rgn);
	bool operator==(const Region& rgn);
	bool operator!=(const Region& rgn);
	Region(int x, int y, int w, int h);
	bool PointInside(unsigned short XPos, unsigned short YPos);
	bool PointInside(Point &p);
	bool InsideRegion(Region& rgn);
	void Normalize();
};

#endif  // ! REGION_H
