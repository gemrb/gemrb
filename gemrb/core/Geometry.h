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

GEM_EXPORT double AngleFromPoints(const Point& p1, const Point& p2);
GEM_EXPORT Point RotatePoint(const Point& p, double angle);
GEM_EXPORT unsigned int Distance(const Point &pos, const Point &pos2);
GEM_EXPORT unsigned int SquaredDistance(const Point &pos, const Point &pos2);

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
GEM_EXPORT std::vector<Point> PlotCircle(const Point& origin, uint16_t radius, uint8_t octants = 0xff) noexcept;

// return a vector of Points composing an ellipse bounded by rect
GEM_EXPORT std::vector<Point> PlotEllipse(const Region& rect) noexcept;

}

#endif /* GEOMETRY_H */
