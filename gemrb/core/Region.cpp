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

/*************** point ****************************/
Point::Point(void)
{
	x = y = 0;
}

bool Point::operator==(const Point& pnt) const
{
	return (x == pnt.x) && (y == pnt.y);
}

bool Point::operator!=(const Point& pnt) const
{
	return !(*this == pnt);
}

Point Point::operator+(const Point& p) const
{
	return Point(x + p.x, y + p.y);
}

Point Point::operator-(const Point& p) const
{
	return Point(x - p.x, y - p.y);
}

Point& Point::operator+=(const Point& rhs)
{
	x += rhs.x;
	y += rhs.y;
	return *this;
}

Point& Point::operator-=(const Point& rhs)
{
	x -= rhs.x;
	y -= rhs.y;
	return *this;
}

Point::Point(short x, short y)
{
	this->x = x;
	this->y = y;
}

bool Point::isnull() const
{
	return (x == 0) && (y == 0);
}

bool Point::isempty() const
{
	return (x == -1) && (y == -1);
}

bool Point::isWithinRadius(int r, const Point& p) const
{
	Point d = operator-(p);
	// sqrt is slow, just check a^2 + b^2 = c^2 <= r^2
	return (d.x * d.x) + (d.y * d.y) <= r * r;
}

Size::Size()
{
	w = h = 0;
}

Size::Size(int w, int h)
{
	this->w = w;
	this->h = h;
}

bool Size::operator==(const Size& size) const
{
	return (w == size.w &&  h == size.h);
}

bool Size::operator!=(const Size& size) const
{
	return !(*this == size);
}

/*************** region ****************************/
Region::Region(void)
{
	x = y = w = h = 0;
}

bool Region::operator==(const Region& rgn) const
{
	return (x == rgn.x) && (y == rgn.y) && (w == rgn.w) && (h == rgn.h);
}

bool Region::operator!=(const Region& rgn) const
{
	return !(*this == rgn);
}

Region::Region(int x, int y, int w, int h)
{
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;
}

Region::Region(const Point &p, const Size& s)
{
	this->x = p.x;
	this->y = p.y;
	this->w = s.w;
	this->h = s.h;
}

bool Region::PointInside(const Point &p) const
{
	if ((p.x < x) || (p.x >= (x + w))) {
		return false;
	}
	if ((p.y < y) || (p.y >= (y + h))) {
		return false;
	}
	return true;
}

bool Region::RectInside(const Region& r) const
{
	// top-left not covered
	if (r.x < x || r.y < y) return false;

	// bottom-right not covered
	if (r.x + r.w > x + w || r.y + r.h > y + h) return false;

	return true;
}

bool Region::IntersectsRegion(const Region& rgn) const
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

Region Region::Intersect(const Region& rgn) const
{
	int ix1 = (x >= rgn.x) ? x : rgn.x;
	int ix2 = (x + w <= rgn.x + rgn.w) ? x + w : rgn.x + rgn.w;
	int iy1 = (y >= rgn.y) ? y : rgn.y;
	int iy2 = (y + h <= rgn.y + rgn.h) ? y + h : rgn.y + rgn.h;

	return Region(ix1, iy1, ix2 - ix1, iy2 - iy1);
}

void Region::ExpandToPoint(const Point& p)
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

void Region::ExpandToRegion(const Region& r)
{
	ExpandToPoint(r.Origin());
	ExpandToPoint(r.Origin() + Point(r.w, 0));
	ExpandToPoint(r.Maximum());
	ExpandToPoint(r.Maximum() - Point(r.w, 0));
}

void Region::ExpandAllSides(int amt)
{
	x -= amt;
	w += amt * 2;
	y -= amt;
	h += amt * 2;
}

}
