/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2017 The GemRB Project
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
 */

#include "GUIAnimation.h"
#include "globals.h"

namespace GemRB {

GUIAnimation::GUIAnimation(unsigned long duration)
{
	begintime = GetTickCount();
	endtime = begintime + duration;
}

PointAnimation::operator bool() const {
	return current != end;
}
	
Point PointAnimation::NextPoint() {
	unsigned long curTime = GetTickCount();
	if (curTime < endtime) {
		int deltax = end.x - begin.x;
		int deltay = end.y - begin.y;
		unsigned long deltaT = endtime - begintime;
		Point p;
		p.x = deltax * double(curTime - begintime) / deltaT;
		p.y = deltay * double(curTime - begintime) / deltaT;

		current = begin + p;
	} else {
		current = end;
	}
	return current;
}

Point PointAnimation::CurrentPoint() const
{
	return current;
}

}
