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


#include "Geometry.h"

#include <cmath>

namespace GemRB {

// Fast approximation of atan2(y, x)
// with a maximum error of 0.1620 degrees
// rcor's rational approximation that can be arbitrarily
// improved if we ever want greater precision
static float pseudoAtan2(float y, float x)
{
	static const uint32_t signMask = 0x80000000;
	static const float b = 0.596227F;
	if (x == 0 && y == 0) return -M_PI_2;

	// Extract the sign bits
	uint32_t xS = signMask & (uint32_t&) x;
	uint32_t yS = signMask & (uint32_t&) y;

	// Determine the quadrant offset
	float q = float((~xS & yS) >> 29 | xS >> 30);

	// Calculate the arctangent in the first quadrant
	float bxy = std::fabs(b * x * y);
	float num = bxy + y * y;
	float atan1q = num / (x * x + bxy + num);

	// Translate it to the proper quadrant
	uint32_t uatan2q = (xS ^ yS) | (uint32_t&) atan1q;
	return (q + (float&) uatan2q) * M_PI_2;
}

float_t AngleFromPoints(float_t y, float_t x)
{
	return pseudoAtan2(y, x);
}

float_t AngleFromPoints(const Point& p1, const Point& p2, bool exact)
{
	float_t xdiff = p1.x - p2.x;
	float_t ydiff = p1.y - p2.y;
	if (xdiff == 0 && ydiff == 0) return -M_PI_2;

	float_t angle;
	if (exact) {
		angle = std::atan2(ydiff, xdiff);
	} else {
		angle = pseudoAtan2(ydiff, xdiff);
	}
	return angle;
}

Point RotatePoint(const Point& p, float_t angle)
{
	int newx = static_cast<int>(p.x * std::cos(angle) - p.y * std::sin(angle));
	int newy = static_cast<int>(p.x * std::sin(angle) + p.y * std::cos(angle));
	return Point(newx, newy);
}

// returns twice the area of triangle a, b, c.
// (can also be negative depending on orientation of a,b,c)
int area2(const Point& a, const Point& b, const Point& c)
{
	return (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
}

// return (c is to the left of a-b)
bool left(const Point& a, const Point& b, const Point& c)
{
	return (area2(a, b, c) > 0);
}

// { return (c is collinear with a-b)
bool collinear(const Point& a, const Point& b, const Point& c)
{
	return (area2(a, b, c) == 0);
}

// Find the intersection of two segments, if any.
// If the intersection is one of the endpoints, or the lines are
// parallel, false is returned.
// The point returned has the actual intersection coordinates rounded down
// to integers
bool intersectSegments(const Point& a, const Point& b, const Point& c, const Point& d, Point& s)
{
	if (collinear(a, b, c) || collinear(a, b, d) ||
	    collinear(c, d, a) || collinear(c, d, b))
		return false;

	if (!((left(a, b, c) != left(a, b, d)) &&
	      (left(c, d, a) != left(c, d, b))))
		return false;

	int64_t A1 = area2(c, d, a);
	int64_t A2 = area2(d, c, b);

	s.x = int((b.x * A1 + a.x * A2) / (A1 + A2));
	s.y = int((b.y * A1 + a.y * A2) / (A1 + A2));

	return true;
}

// find the intersection of a segment with a horizontal scanline, if any
bool intersectSegmentScanline(const Point& a, const Point& b, int y, int& x)
{
	int y1 = a.y - y;
	int y2 = b.y - y;

	if (y1 * y2 > 0) return false;
	if (y1 == 0 && y2 == 0) return false;

	x = a.x + ((b.x - a.x) * y1) / (y1 - y2);
	return true;
}

std::vector<Point> PlotEllipse(const Region& rect) noexcept
{
	if (rect.size.IsInvalid()) {
		return {};
	}

	if (rect.w == rect.h) {
		return PlotCircle(rect.Center(), (rect.w / 2) - 1);
	}

	Point p1 = rect.origin;
	Point p2 = rect.Maximum() - Point(1, 1); // we must contain inside the rect, so subtract 1px

	int a = p2.x - p1.x;
	int b = p2.y - p1.y;
	int b1 = b & 1; /* values of diameter */
	long dx = 4 * (1 - a) * b * b;
	long dy = 4 * (b1 + 1) * a * a; /* error increment */
	long err = dx + dy + b1 * a * a;
	long e2; /* error of 1.step */

	p1.y += (b + 1) / 2;
	p2.y = p1.y - b1; /* starting pixel */
	a *= 8 * a;
	b1 = 8 * b * b;

	std::vector<Point> points;
	points.reserve(rect.size.Area());

	do {
		points.emplace_back(Point(p2.x, p1.y)); /*   I. Quadrant */
		points.emplace_back(p1); /*  II. Quadrant */
		points.emplace_back(Point(p1.x, p2.y)); /* III. Quadrant */
		points.emplace_back(p2); /*  IV. Quadrant */

		e2 = 2 * err;
		if (e2 <= dy) {
			++p1.y;
			--p2.y;
			err += dy += a;
		}
		if (e2 >= dx || 2 * err > dy) {
			++p1.x;
			--p2.x;
			err += dx += b1;
		}
	} while (p1.x <= p2.x);

	while (p1.y - p2.y < b) { /* too early stop of flat ellipses a=1 */
		points.emplace_back(Point(p1.x - 1, p1.y)); /* -> finish tip of ellipse */
		points.emplace_back(Point(p2.x + 1, p1.y++));
		points.emplace_back(Point(p1.x - 1, p2.y));
		points.emplace_back(Point(p2.x + 1, p2.y--));
	}

	return points;
}

}
