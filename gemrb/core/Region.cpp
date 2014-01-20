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

bool Point::operator==(const Point& pnt)
{
	if (( x == pnt.x ) && ( y == pnt.y )) {
		return true;
	}
	return false;
}

bool Point::operator!=(const Point& pnt)
{
	return !(*this == pnt);
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
	if (x==0 && y==0) {
		return true;
	}
	return false;
}

bool Point::isempty() const
{
	if (x==-1 && y==-1) {
		return true;
	}
	return false;
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

/*************** region ****************************/
Region::Region(void)
{
	x = y = w = h = 0;
}

bool Region::operator==(const Region& rgn)
{
	if (( x == rgn.x ) && ( y == rgn.y ) && ( w == rgn.w ) && ( h == rgn.h )) {
		return true;
	}
	return false;
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

Region::Region(const Point &p, int w, int h)
{
	this->x = p.x;
	this->y = p.y;
	this->w = w;
	this->h = h;
}

bool Region::PointInside(const Point &p) const
{
	return PointInside(p.x, p.y);
}

bool Region::PointInside(unsigned short XPos, unsigned short YPos) const
{
	if (( XPos < x ) || ( XPos > ( x + w ) )) {
		return false;
	}
	if (( YPos < y ) || ( YPos > ( y + h ) )) {
		return false;
	}
	return true;
}

bool Region::InsideRegion(const Region& rgn) const
{
	if (x > ( rgn.x + rgn.w )) {
		return false;
	}
	if (( x + w ) < rgn.x) {
		return false;
	}
	if (y > ( rgn.y + rgn.h )) {
		return false;
	}
	if (( y + h ) < rgn.y) {
		return false;
	}
	return true;
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
