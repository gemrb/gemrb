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

#ifndef Animations_h
#define Animations_h

#include "Region.h"

namespace GemRB {

class GUIAnimation {
protected:
	unsigned long begintime, endtime;
	
public:
	GUIAnimation() : begintime(0), endtime(0) {}
	GUIAnimation(unsigned long duration);
	
	operator bool() const;
};

class PointAnimation : public GUIAnimation {
	Point begin, end;
	
public:
	PointAnimation() : GUIAnimation() {}
	
	PointAnimation(const Point& begin, const Point& end, unsigned long duration)
	: GUIAnimation(duration), begin(begin), end(end) {}
	
	Point NextPoint() const;
};

}
	
#endif /* Animations_h */
