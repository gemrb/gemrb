/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2006 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

/**
 * @file Particles.h
 * Declares Particles class implementing weather and spark effects
 * and related defines
 */

#ifndef PARTICLES_H
#define PARTICLES_H

#include "exports.h"
#include "ie_types.h"

#include "Region.h"

namespace GemRB {

class CharAnimations;
class Scriptable;

//global phase for the while spark structure
#define P_GROW  0
#define P_FADE  1
#define P_EMPTY 2

// this structure holds data for a single particle element
struct Element {
	int state;
	Point pos;
};

/**
 * @class Particles 
 * Class holding information about particles and rendering them.
 */

#define SP_TYPE_POINT  0
#define SP_TYPE_LINE   1
#define SP_TYPE_CIRCLE 2
#define SP_TYPE_BITMAP 3

#define SP_PATH_FALL   0       //free falling
#define SP_PATH_FOUNT  1       //going up and down
#define SP_PATH_FLIT   2       //flitting
#define SP_PATH_RAIN   3       //falling and vanishing quickly
#define SP_PATH_EXPL   4       //explosion (mostly used with fragments)

#define SP_SPAWN_NONE  0       //don't create new sparks
#define SP_SPAWN_FULL  1       //fill all at setup, then switch to none
#define SP_SPAWN_SOME  2       //add some new elements regularly

#define SPARK_COLOR_BLUE 0
#define SPARK_COLOR_GOLD 1
#define SPARK_COLOR_PURPLE 2
#define SPARK_COLOR_ICE 3
#define SPARK_COLOR_STONE 4
#define SPARK_COLOR_BLACK 5
#define SPARK_COLOR_CHROM 6
#define SPARK_COLOR_RED 7
#define SPARK_COLOR_GREEN 8
#define SPARK_COLOR_WHITE 9
#define SPARK_COLOR_MAGENTA 10
#define SPARK_COLOR_ORANGE 11
#define SPARK_COLOR_CUSTOM 12

#define MAX_SPARK_COLOR  13
#define MAX_SPARK_PHASE  5

class GEM_EXPORT Particles {
public:
	Particles(int s);
	~Particles();

	void SetBitmap(unsigned int FragAnimID);
	void SetPhase(ieByte ph) { phase = ph; }
	int GetPhase() const { return phase; }
	bool MatchPos(const Point &p) const { return pos.x==p.x && pos.y==p.y; }
	void SetType(ieByte t, ieByte p=SP_PATH_FALL, ieByte st=SP_SPAWN_NONE)
	{
		type=t;
		path=p;
		spawn_type=st;
	}
	void SetRegion(int x, int y, int w, int h)
	{
		pos.x = x;
		pos.y = y;
		pos.w = w;
		pos.h = h;
	}
	void SetTimeToLive(int ttl) { timetolive = ttl; }
	void SetColor(ieByte c) { color=c; }
	void SetOwner(Scriptable *o) { owner=o; }
	/* returns true if it cannot add new elements */
	bool AddNew(const Point &point);
	void Draw(const Region &screen);
	void AddParticles(int count);
	/* returns true if it could be destroyed (didn't draw anything) */
	int Update();
	int GetHeight() const { return pos.y+pos.h; }
private:
	Element *points;
	ieDword timetolive;
	ieDword target;    //could be 0, in that case target is pos
	ieWord size;       //spark number
	ieWord last_insert;//last spark idx added
	Scriptable *owner; //could be area or game or actor
	Region pos;
	ieByte phase;      //global phase
	ieByte type;       //draw type (snow, rain)
	ieByte path;       //path type
	ieByte color;      //general spark color
	ieByte spawn_type;
	//use char animations for the fragment animations
	//1. the cycles are loaded only when needed
	//2. the fragments ARE avatar animations in the original IE (for some unknown reason)
	CharAnimations *fragments;
};

}

#endif  // ! PARTICLES_H
