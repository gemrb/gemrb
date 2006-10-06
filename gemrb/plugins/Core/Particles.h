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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Particles.h,v 1.1 2006/10/06 23:01:10 avenger_teambg Exp $
 *
 */

/**
 * @file Particles.h
 * Declares Particles class implementing weather and spark effects
 * and related defines
 */

#ifndef PARTICLES_H
#define PARTICLES_H

#include "../../includes/ie_types.h"

//global phase for the while spark structure
#define P_GROW  0
#define P_FADE  1

// this structure holds data for a single particle element
typedef struct Element {
	int state;
	Point pos;
} Element;

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

#define SPARK_COLOR_NONE 0
#define SPARK_COLOR_BLUE 1
#define SPARK_COLOR_GOLD 2
#define SPARK_COLOR_PURPLE 3
#define SPARK_COLOR_ICE 4
#define SPARK_COLOR_STONE 5
#define SPARK_COLOR_BLACK 6
#define SPARK_COLOR_CHROM 7
#define SPARK_COLOR_RED 8
#define SPARK_COLOR_GREEN 9
#define SPARK_COLOR_WHITE 10
#define SPARK_COLOR_MAGENTA 11
#define SPARK_COLOR_ORANGE 12

#define MAX_SPARK_COLOR  13
#define MAX_SPARK_PHASE  5

class GEM_EXPORT Particles {
public:
	Particles(int s);
	~Particles();

	void SetBitmap(const ieResRef BAM);
	void SetPhase(ieByte ph) { phase = ph; }
	int GetPhase() { return phase; }
	void SetType(ieByte t, ieByte p=SP_PATH_FALL) { type=t; path=p; }
	void SetRegion(int x, int y, int w, int h)
	{
		pos.x = x;
		pos.y = y;
		pos.w = w;
		pos.h = h;
	}
	void SetColor(ieByte c) { color=c; }
	void AddNew(Point &pos);
	void Draw(Region &screen);
	/* returns true if it could be destroyed (didn't draw anything) */
	int Update();
public:
	Element *points;
	ieDword target;    //could be 0, in that case target is pos
	ieWord size;       //spark number
	ieWord last_insert;//last spark idx added
	Scriptable *owner; //could be area or game
	Region pos;
	ieWord width;
	ieByte phase;      //global phase
	ieByte type;       //draw type (snow, rain)
	ieByte path;       //path type
	ieByte color;      //general spark color
	bool spawn_type;
	Animation *bitmap[MAX_SPARK_PHASE];
};

#endif  // ! PARTICLES_H
