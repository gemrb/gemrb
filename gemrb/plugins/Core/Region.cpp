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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Region.cpp,v 1.10 2004/08/02 18:00:21 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "Region.h"

Region::Region(void)
{
	x = y = w = h = 0;
}

Region::~Region(void)
{
}

Region::Region(const Region& rgn)
{
	x = rgn.x;
	y = rgn.y;
	w = rgn.w;
	h = rgn.h;
}

Region& Region::operator=(const Region& rgn)
{
	x = rgn.x;
	y = rgn.y;
	w = rgn.w;
	h = rgn.h;
	return *this;
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
	if (( x != rgn.x ) || ( y != rgn.y ) || ( w != rgn.w ) || ( h != rgn.h )) {
		return true;
	}
	return false;
}

Region::Region(int x, int y, int w, int h)
{
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;
}

bool Region::PointInside(unsigned short XPos, unsigned short YPos)
{
	if (( XPos < x ) || ( XPos > ( x + w ) )) {
		return false;
	}
	if (( YPos < y ) || ( YPos > ( y + h ) )) {
		return false;
	}
	return true;
}

bool Region::InsideRegion(Region& rgn)
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
