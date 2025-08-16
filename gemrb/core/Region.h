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

#include "exports-core.h"

#include "fmt/format.h"

#include <algorithm>
#include <vector>

namespace GemRB {

/**
 * @class Point
 * Point in 2d-space: [x, y]
 */

class GEM_EXPORT BasePoint {
public:
	BasePoint() noexcept = default;
	BasePoint(int x, int y) noexcept;

	BasePoint operator+(const BasePoint& p) const noexcept;
	BasePoint operator-(const BasePoint& p) const noexcept;

	bool operator==(const BasePoint& pnt) const noexcept;
	bool operator!=(const BasePoint& pnt) const noexcept;

	bool IsZero() const noexcept; // (0, 0)
	bool IsInvalid() const noexcept; // (-1, -1)

	inline void reset() noexcept
	{
		x = y = 0;
	}

	inline void Invalidate() noexcept
	{
		x = y = -1;
	}

	// true if p is within the circle of radius r centered at p
	bool IsWithinRadius(int r, const BasePoint& p) const noexcept;
	bool IsWithinEllipse(int r, const BasePoint& p, int a = 16, int b = 12) const noexcept;

	int x = 0;
	int y = 0;
};

class GEM_EXPORT Point : public BasePoint {
public:
	using BasePoint::BasePoint;

	Point operator+(const Point& p) const noexcept;
	Point operator-(const Point& p) const noexcept;

	Point& operator+=(const Point& rhs) noexcept;
	Point& operator-=(const Point& rhs) noexcept;

	Point& operator/(int div) noexcept;
};

class GEM_EXPORT SearchmapPoint : public BasePoint {
public:
	using BasePoint::BasePoint;
	SearchmapPoint() noexcept = default;
	explicit SearchmapPoint(const Point& p) noexcept
	{
		x = p.x / 16;
		y = p.y / 12;
	}

	SearchmapPoint operator+(const SearchmapPoint& p) const noexcept;
	SearchmapPoint operator*(int n) const noexcept;
	Point ToNavmapPoint() const { return Point(x * 16, y * 12); };
};

class GEM_EXPORT Size {
public:
	int w = 0;
	int h = 0;
	Size() noexcept = default;
	Size(int, int) noexcept;

	inline void reset() noexcept
	{
		w = h = 0;
	}

	bool operator==(const Size& size) const noexcept;
	bool operator!=(const Size& size) const noexcept;

	Point Center() const noexcept { return Point(w / 2, h / 2); }
	int Area() const noexcept { return w * h; }
	bool IsZero() const noexcept { return w == 0 && h == 0; }
	bool IsInvalid() const noexcept { return w <= 0 || h <= 0; }

	bool PointInside(const BasePoint& p) const noexcept { return p.x >= 0 && p.x < w && p.y >= 0 && p.y < h; }
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

	// unfortunately anonymous structs are an extension in C++...
	int& x = origin.x;
	int& y = origin.y;
	int& w = size.w;
	int& h = size.h;

	Region() noexcept = default;
	Region(int x, int y, int w, int h) noexcept;
	Region(const Point& p, const Size& s) noexcept;
	Region(const Region&) noexcept;

	Region& operator=(const Region&) noexcept;

	bool operator==(const Region& rgn) const noexcept;
	bool operator!=(const Region& rgn) const noexcept;

	bool PointInside(const Point& p) const noexcept;
	bool RectInside(const Region& r) const noexcept;

	bool IntersectsRegion(const Region& rgn) const noexcept;
	Region Intersect(const Region& rgn) const noexcept;

	Point Center() const noexcept { return Point(x + w / 2, y + h / 2); }
	Point Maximum() const noexcept { return Point(x + w, y + h); }
	// returns the point of intersection between Region and the line (extending to infinity) from the Center to p
	Point Intercept(const Point& p) const noexcept;

	void ExpandToPoint(const Point& p) noexcept;
	void ExpandToRegion(const Region& r) noexcept;
	void ExpandAllSides(int amt) noexcept;

	static Region RegionEnclosingRegions(const Region& r1, const Region& r2)
	{
		Point min;
		Point max;

		min.x = std::min(r1.x, r2.x);
		min.y = std::min(r1.y, r2.y);
		max.x = std::max(r1.x + r1.w, r2.x + r2.w);
		max.y = std::max(r1.y + r1.h, r2.y + r2.h);

		return RegionFromPoints(min, max);
	}

	template<typename T>
	static Region RegionEnclosingRegions(T regions)
	{
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

	static Region RegionFromPoints(const Point& p1, const Point& p2)
	{
		int dx = p2.x - p1.x;
		int dy = p2.y - p1.y;

		int x = (dx < 0) ? p2.x : p1.x;
		int y = (dy < 0) ? p2.y : p1.y;
		int w = (dx < 0) ? -dx : dx;
		int h = (dy < 0) ? -dy : dy;

		return Region(x, y, w, h);
	}
};

using Regions = std::vector<Region>;

}

namespace fmt {

template<>
struct formatter<GemRB::Point> {
	char presentation = 'd';

	auto parse(const format_parse_context& ctx) -> decltype(ctx.begin())
	{
		// [ctx.begin(), ctx.end()) is a character range that contains a part of
		// the format string starting from the format specifications to be parsed,
		// e.g. in
		//
		//   fmt::format("{:f} - point of interest", point{1, 2});
		//
		// the range will contain "f} - point of interest". The formatter should
		// parse specifiers until '}' or the end of the range. In this example
		// the formatter should parse the 'f' specifier and return an iterator
		// pointing to '}'.

		// Parse the presentation format and store it in the formatter:
		auto it = ctx.end();
		// TODO: implement parsing
		return it;
	}

	template<typename FormatContext>
	auto format(const GemRB::Point& p, FormatContext& ctx) const -> decltype(ctx.out())
	{
		return format_to(ctx.out(), "({:d}, {:d})", p.x, p.y);
	}
};

}

#endif // ! REGION_H
