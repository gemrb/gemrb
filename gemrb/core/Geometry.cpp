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

namespace GemRB {

double AngleFromPoints(const Point& p1, const Point& p2)
{
	double xdiff = p1.x - p2.x;
	double ydiff = p1.y - p2.y;
	
	double angle = std::atan2(ydiff, xdiff);
	return angle;
}

Point RotatePoint(const Point& p, double angle)
{
	int newx = static_cast<int>(p.x * std::cos(angle) - p.y * std::sin(angle));
	int newy = static_cast<int>(p.x * std::sin(angle) + p.y * std::cos(angle));
	return Point(newx, newy);
}

/** Calculates distance between 2 points */
unsigned int Distance(const Point &p, const Point &q)
{
	long x = p.x - q.x;
	long y = p.y - q.y;
	return (unsigned int) std::hypot(x, y);
}

/** Calculates squared distance between 2 points */
unsigned int SquaredDistance(const Point &p, const Point &q)
{
	long x = p.x - q.x;
	long y = p.y - q.y;
	return static_cast<unsigned int>(x * x + y * y);
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
	
	s.x = int((b.x*A1 + a.x*A2) / (A1 + A2));
	s.y = int((b.y*A1 + a.y*A2) / (A1 + A2));
	
	return true;
}

// find the intersection of a segment with a horizontal scanline, if any
bool intersectSegmentScanline(const Point& a, const Point& b, int y, int& x)
{
	int y1 = a.y - y;
	int y2 = b.y - y;
	
	if (y1 * y2 > 0) return false;
	if (y1 == 0 && y2 == 0) return false;
	
	x = a.x + ((b.x - a.x)*y1)/(y1-y2);
	return true;
}

std::vector<Point> PlotCircle(const Point& origin, uint16_t r, uint8_t octants) noexcept
{
	// Uses the 2nd order Bresenham's Circle Algorithm: https://funloop.org/post/2021-03-15-bresenham-circle-drawing-algorithm.html
	
	std::vector<Point> points;
	points.reserve(r * r); // this is more than enough for a full circle
	
	auto GenOctants = [&origin, &points, octants](int x, int y) noexcept
	{
		// points are emplaced in octant order
		
		if (octants & 1 << 0) {
			points.emplace_back(origin + Point(y, x));
		}
		if (octants & 1 << 1) {
			points.emplace_back(origin + Point(-y, x));
		}
		if (octants & 1 << 2) {
			points.emplace_back(origin + Point(x, y));
		}
		if (octants & 1 << 3) {
			points.emplace_back(origin + Point(-x, y));
		}
		if (octants & 1 << 4) {
			points.emplace_back(origin + Point(x, -y));
		}
		if (octants & 1 << 5) {
			points.emplace_back(origin + Point(-x, -y));
		}
		if (octants & 1 << 6) {
			points.emplace_back(origin + Point(y, -x));
		}
		if (octants & 1 << 7) {
			points.emplace_back(origin + Point(-y, -x));
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

std::vector<Point> PlotEllipse(const Region& rect) noexcept
{
	if (rect.size.IsInvalid()) {
		return {};
	}
	
	if (rect.w == rect.h) {
		return PlotCircle(rect.Center(), rect.w);
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
	p2.y = p1.y - b1;   /* starting pixel */
	a *= 8 * a;
	b1 = 8 * b * b;
	
	std::vector<Point> points;
	points.reserve(rect.size.Area());

	do {
		points.emplace_back(Point(p2.x, p1.y)); /*   I. Quadrant */
		points.emplace_back(p1); 				 /*  II. Quadrant */
		points.emplace_back(Point(p1.x, p2.y)); /* III. Quadrant */
		points.emplace_back(p2); 				 /*  IV. Quadrant */
		
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
	
	while (p1.y - p2.y < b) {  /* too early stop of flat ellipses a=1 */
		points.emplace_back(Point(p1.x - 1, p1.y)); /* -> finish tip of ellipse */
		points.emplace_back(Point(p2.x + 1, p1.y++));
		points.emplace_back(Point(p1.x - 1, p2.y));
		points.emplace_back(Point(p2.x + 1, p2.y--));
	}
	
	return points;
}

}
