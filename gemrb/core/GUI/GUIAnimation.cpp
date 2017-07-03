//
//  Animations.cpp
//  GemRB
//
//  Created by Brad Allred on 7/2/17.
//
//

#include "GUIAnimation.h"
#include "globals.h"

namespace GemRB {

GUIAnimation::GUIAnimation(unsigned long duration)
{
	begintime = GetTickCount();
	endtime = begintime + duration;
}

GUIAnimation::operator bool() const {
	unsigned long curTime = GetTickCount();
	return (curTime < endtime);
}
	
Point PointAnimation::NextPoint() const {
	unsigned long curTime = GetTickCount();
	if (curTime < endtime) {
		int deltax = end.x - begin.x;
		int deltay = end.y - begin.y;
		unsigned long deltaT = endtime - begintime;
		Point p;
		p.x = deltax * double(curTime - begintime) / deltaT;
		p.x = deltay * double(curTime - begintime) / deltaT;
		return begin + p;
	}
	return end;
}

}
