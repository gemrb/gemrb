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

Point::Point(short x, short y)
{
	this->x = x;
	this->y = y;
}

ieDword Point::asDword() const
{
	return ((y & 0xFFFF) << 16) | (x & 0xFFFF);
}

void Point::fromDword(ieDword val)
{
	x = val & 0xFFFF;
	y = val >> 16;
}

bool Point::isnull() const
{
	return (x == 0) && (y == 0);
}

bool Point::isempty() const
{
	return (x == -1) && (y == -1);
}

bool Point::PointInside(const Point &p) const
{
	if (( p.x < 0 ) || ( p.x > x )) {
		return false;
	}
	if (( p.y < 0 ) || ( p.y > y )) {
		return false;
	}
	return true;
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

bool Size::operator==(const Size& size)
{
	return (w == size.w &&  h == size.h);
}

bool Size::operator!=(const Size& size)
{
	return !(*this == size);
}

/*************** region ****************************/
Region::Region(void)
{
	x = y = w = h = 0;
}

bool Region::operator==(const Region& rgn)
{
	return (x == rgn.x) && (y == rgn.y) && (w == rgn.w) && (h == rgn.h);
}

bool Region::operator!=(const Region& rgn)
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

void Region::Normalize()
{
	if (x > w) {
		int tmp = x;
		x = w;
		w = tmp - x;
	} else {
		w -= x;
	}
	if (y > h) {
		int tmp = y;
		y = h;
		h = tmp - y;
	} else {
		h -= y;
	}
}

}
