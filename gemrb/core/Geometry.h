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

#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "exports.h"

#include "Region.h"

#include <vector>

namespace GemRB {

GEM_EXPORT float_t AngleFromPoints(const Point& p1, const Point& p2, bool exact = false);
GEM_EXPORT float_t AngleFromPoints(float_t y, float_t x);
GEM_EXPORT Point RotatePoint(const Point& p, float_t angle);
template<class T, typename = std::enable_if_t<!std::is_pointer<T>::value>>
GEM_EXPORT unsigned int Distance(const T& p, const T& q)
{
	long x = p.x - q.x;
	long y = p.y - q.y;
	return (unsigned int) std::hypot(x, y);
}

template<class T, typename = std::enable_if_t<!std::is_pointer<T>::value>>
GEM_EXPORT unsigned int SquaredDistance(const T& p, const T& q)
{
	long x = p.x - q.x;
	long y = p.y - q.y;
	return static_cast<unsigned int>(x * x + y * y);
}

// returns twice the area of triangle a, b, c.
// (can also be negative depending on orientation of a,b,c)
GEM_EXPORT int area2(const Point& a, const Point& b, const Point& c);

// return (c is to the left of a-b)
GEM_EXPORT bool left(const Point& a, const Point& b, const Point& c);

GEM_EXPORT bool collinear(const Point& a, const Point& b, const Point& c);

// Find the intersection of two segments, if any.
// If the intersection is one of the endpoints, or the lines are
// parallel, false is returned.
// The point returned has the actual intersection coordinates rounded down
// to integers
GEM_EXPORT bool intersectSegments(const Point& a, const Point& b, const Point& c, const Point& d, Point& s);

// find the intersection of a segment with a horizontal scanline, if any
GEM_EXPORT bool intersectSegmentScanline(const Point& a, const Point& b, int y, int& x);

/*
 return vector of Points composing a circle of specified radius at origin
 since the points are ordered by octant, they are suitable for connecting as lines to create a filled circle

 PlotCircle uses the laws of symmetry to draw up to 8 octants (using all 8 yields a complete circle) of a circle at once
 The octants parameter is a bitfield where each bit corresponds to an octant. See the diagram to understand how they are ordered.
	  1   0
   3 ⋱  ⋮  ⋰2
  3____⋱⋮⋰___2
  5    ⋰⋮⋱   4
   5 ⋰  ⋮  ⋱4
	  7  6
 */
template<class T>
GEM_EXPORT std::vector<T> PlotCircle(const T& origin, uint16_t r, uint8_t octants = 0xff) noexcept
{
	// Uses the 2nd order Bresenham's Circle Algorithm: https://funloop.org/post/2021-03-15-bresenham-circle-drawing-algorithm.html

	std::vector<T> points;
	points.reserve(6 * r); // 6 is 2𝜋 rounded down

	auto GenOctants = [&origin, &points, octants](int x, int y) noexcept {
		// points are emplaced in octant order

		if (octants & 1 << 0) {
			points.emplace_back(origin + T(y, x));
		}
		if (octants & 1 << 1) {
			points.emplace_back(origin + T(-y, x));
		}
		if (octants & 1 << 2) {
			points.emplace_back(origin + T(x, y));
		}
		if (octants & 1 << 3) {
			points.emplace_back(origin + T(-x, y));
		}
		if (octants & 1 << 4) {
			points.emplace_back(origin + T(x, -y));
		}
		if (octants & 1 << 5) {
			points.emplace_back(origin + T(-x, -y));
		}
		if (octants & 1 << 6) {
			points.emplace_back(origin + T(y, -x));
		}
		if (octants & 1 << 7) {
			points.emplace_back(origin + T(-y, -x));
		}
	};

	int x = 0;
	int y = r;
	int fm = 1 - r;
	int de = 3; // east vector
	int dse = -2 * r + 5; // SE vector

	// do the middle row first
	GenOctants(x, y);

	while (x < y) {
		if (fm <= 0) {
			fm += de;
		} else {
			fm += dse;
			dse += 2;
			--y;
		}

		de += 2;
		dse += 2;
		++x;

		GenOctants(x, y);
	}

	return points;
}

// return a vector of Points composing an ellipse bounded by rect
GEM_EXPORT std::vector<Point> PlotEllipse(const Region& rect) noexcept;

}

#endif /* GEOMETRY_H */
