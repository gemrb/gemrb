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

#include <algorithm>
#include <vector>

namespace GemRB {

/**
 * @class Point
 * Point in 2d-space: [x, y]
 */

class GEM_EXPORT Point {
public:
	Point() = default;
	Point(int x, int y);

	bool operator==(const Point &pnt) const;
	bool operator!=(const Point &pnt) const;

	Point operator+(const Point& p) const;
	Point operator-(const Point& p) const;
	
	Point& operator+=(const Point& rhs);
	Point& operator-=(const Point& rhs);

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

	// true if p is within the circle of radius r centered at p
	bool isWithinRadius(int r, const Point& p) const;

	int x = 0;
	int y = 0;
};

class GEM_EXPORT Size {
public:
	int w, h;
	Size();
	Size(int, int);

	bool operator==(const Size& size) const;
	bool operator!=(const Size& size) const;
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
	// WARNING: we reinterpret_cast this struct!
	// Point and Size must be first and in this order
	Point origin;
	Size size;
	
	// unfortunatly anonymous structs are an extenison in C++...
	int& x = origin.x;
	int& y = origin.y;
	int& w = size.w;
	int& h = size.h;

	Region() = default;
	Region(int x, int y, int w, int h);
	Region(const Point& p, const Size& s);
	Region(const Region&);
	Region(Region&&);
	
	Region& operator=(const Region&);

	bool operator==(const Region& rgn) const;
	bool operator!=(const Region& rgn) const;

	bool PointInside(const Point &p) const;
	bool RectInside(const Region& r) const;

	bool IntersectsRegion(const Region& rgn) const;
	Region Intersect(const Region& rgn) const;

	Point Center() const { return Point(x + w / 2, y + h / 2); }
	Point Maximum() const { return Point(x + w, y + h); }
	
	void ExpandToPoint(const Point& p);
	void ExpandToRegion(const Region& r);
	void ExpandAllSides(int amt);
	
	static Region RegionEnclosingRegions(const Region& r1, const Region& r2) {
		Point min, max;

		min.x = std::min(r1.x, r2.x);
		min.y = std::min(r1.y, r2.y);
		max.x = std::max(r1.x + r1.w, r2.x + r2.w);
		max.y = std::max(r1.y + r1.h, r2.y + r2.h);

		return RegionFromPoints(min, max);
	}

	template<typename T>
	static Region RegionEnclosingRegions(T regions) {
		if (regions.empty()) return Region();
		typename T::const_iterator it = regions.begin();
		// start with the complete first rect
		Region bounds = *it++;
		for (; it != regions.end(); ++it) {
			// now expand it as needed
			const Region& r = *it;
			bounds = RegionEnclosingRegions(bounds, r);
		}
		return bounds;
	}

	static Region RegionFromPoints(const Point& p1, const Point& p2) {
		int dx = p2.x - p1.x;
		int dy = p2.y - p1.y;

		int x = (dx < 0) ? p2.x : p1.x;
		int y = (dy < 0) ? p2.y : p1.y;
		int w = (dx < 0) ? -dx : dx;
		int h = (dy < 0) ? -dy : dy;

		return Region(x, y, w, h);
	}
};

typedef std::vector<Region> Regions;

}

#endif  // ! REGION_H
