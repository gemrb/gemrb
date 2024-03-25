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

#include "Region.h"

namespace GemRB {

bool Point::operator==(const Point& pnt) const noexcept
{
	return (x == pnt.x) && (y == pnt.y);
}

bool Point::operator!=(const Point& pnt) const noexcept
{
	return !(*this == pnt);
}

Point Point::operator+(const Point& p) const noexcept
{
	return Point(x + p.x, y + p.y);
}

Point Point::operator-(const Point& p) const noexcept
{
	return Point(x - p.x, y - p.y);
}

Point& Point::operator+=(const Point& rhs) noexcept
{
	x += rhs.x;
	y += rhs.y;
	return *this;
}

Point& Point::operator-=(const Point& rhs) noexcept
{
	x -= rhs.x;
	y -= rhs.y;
	return *this;
}

Point& Point::operator/(int div) noexcept
{
	x /= div;
	y /= div;
	return *this;
}

Point::Point(int x, int y) noexcept
{
	this->x = x;
	this->y = y;
}

bool Point::IsZero() const noexcept
{
	return (x == 0) && (y == 0);
}

bool Point::IsInvalid() const noexcept
{
	return (x == -1) && (y == -1);
}

bool Point::isWithinRadius(int r, const Point& p) const noexcept
{
	Point d = operator-(p);
	// sqrt is slow, just check a^2 + b^2 = c^2 <= r^2
	return (d.x * d.x) + (d.y * d.y) <= r * r;
}

Size::Size(int w, int h) noexcept
{
	this->w = w;
	this->h = h;
}

bool Size::operator==(const Size& size) const noexcept
{
	return (w == size.w &&  h == size.h);
}

bool Size::operator!=(const Size& size) const noexcept
{
	return !(*this == size);
}

bool Region::operator==(const Region& rgn) const noexcept
{
	return (x == rgn.x) && (y == rgn.y) && (w == rgn.w) && (h == rgn.h);
}

bool Region::operator!=(const Region& rgn) const noexcept
{
	return !(*this == rgn);
}

Region::Region(int x, int y, int w, int h) noexcept
{
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;
}

Region::Region(const Point &p, const Size& s) noexcept
{
	origin = p;
	size = s;
}

Region::Region(const Region &r) noexcept
: origin(r.origin), size(r.size)
{}

Region& Region::operator=(const Region &rhs) noexcept
{
	if (&rhs != this) {
		origin = rhs.origin;
		size = rhs.size;
	}
	return *this;
}

bool Region::PointInside(const Point &p) const noexcept
{
	if ((p.x < x) || (p.x >= (x + w))) {
		return false;
	}
	if ((p.y < y) || (p.y >= (y + h))) {
		return false;
	}
	return true;
}

bool Region::RectInside(const Region& r) const noexcept
{
	// top-left not covered
	if (r.x < x || r.y < y) return false;

	// bottom-right not covered
	if (r.x + r.w > x + w || r.y + r.h > y + h) return false;

	return true;
}

bool Region::IntersectsRegion(const Region& rgn) const noexcept
{
	if (x >= ( rgn.x + rgn.w )) {
		return false; // entirely to the right of rgn
	}
	if (( x + w ) <= rgn.x) {
		return false; // entirely to the left of rgn
	}
	if (y >= ( rgn.y + rgn.h )) {
		return false; // entirely below rgn
	}
	if (( y + h ) <= rgn.y) {
		return false; // entirely above rgn
	}
	return true;
}

Region Region::Intersect(const Region& rgn) const noexcept
{
	int ix1 = (x >= rgn.x) ? x : rgn.x;
	int ix2 = (x + w <= rgn.x + rgn.w) ? x + w : rgn.x + rgn.w;
	int iy1 = (y >= rgn.y) ? y : rgn.y;
	int iy2 = (y + h <= rgn.y + rgn.h) ? y + h : rgn.y + rgn.h;

	return Region(ix1, iy1, ix2 - ix1, iy2 - iy1);
}

Point Region::Intercept(const Point& p) const noexcept
{
	const Point& mid = Center();
	const Point& min = origin;
	const Point& max = Maximum();
	
	if (p.x == mid.x) return Point(p.x, (p.y < min.y) ? min.y : max.y); // vert line
	if (p.y == mid.y) return Point((p.x < min.x) ? min.x : max.x, p.y); // horiz line

	float_t m = float_t(mid.y - p.y) / float_t(mid.x - p.x);

	if (p.x <= mid.x) { // check "left" side
		int newY = m * (min.x - p.x) + p.y;
		if (min.y <= newY && newY <= max.y) return Point(min.x, newY);
	}

	if (p.x >= mid.x) { // check "right" side
		int newY = m * (max.x - p.x) + p.y;
		if (min.y <= newY && newY <= max.y) return Point(max.x, newY);
	}
	
	if (p.y <= mid.y) { // check "top" side
		int newX = (min.y - p.y) / m + p.x;
		if (min.x <= newX && newX <= max.x) return Point(newX, min.y);
	}

	if (p.y >= mid.y) { // check "bottom" side
		int newX = (max.y - p.y) / m + p.x;
		if (min.x <= newX && newX <= max.x) return Point(newX, max.y);
	}
	
	assert(p == mid || PointInside(p));
	return p;
}

void Region::ExpandToPoint(const Point& p) noexcept
{
	if (p.x < x) {
		w += x - p.x;
		x = p.x;
	} else if (p.x > x + w) {
		w = p.x - x;
	}
	
	if (p.y < y) {
		h += y - p.y;
		y = p.y;
	} else if (p.y > y + h) {
		h = p.y - y;
	}
}

void Region::ExpandToRegion(const Region& r) noexcept
{
	ExpandToPoint(r.origin);
	ExpandToPoint(r.origin + Point(r.w, 0));
	ExpandToPoint(r.Maximum());
	ExpandToPoint(r.Maximum() - Point(r.w, 0));
}

void Region::ExpandAllSides(int amt) noexcept
{
	x -= amt;
	w += amt * 2;
	y -= amt;
	h += amt * 2;
}

}
