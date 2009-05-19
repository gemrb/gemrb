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
 * $Id$
 */
#ifndef POLYGON_H
#define POLYGON_H

#include "Region.h"
#include "../../includes/RGBAColor.h"
#include "../../includes/globals.h"

#include <list>

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT Trapezoid {
public:
	int y1, y2;
	int left_edge, right_edge;
};

class GEM_EXPORT Gem_Polygon {
public:
	Gem_Polygon(Point* points, unsigned int count, Region *bbox = NULL);
	~Gem_Polygon(void);
	Region BBox;
	Point* points;
	unsigned int count;
	std::list<Trapezoid> trapezoids;
	bool PointIn(Point &p);
	bool PointIn(int x, int y);
	void RecalcBBox();
	void ComputeTrapezoids();
};

// wall polygons are used to render area wallgroups
// wall polygons never create a surface
//ALWAYSCOVER wallpolygon blocks everything that it covers
//BASELINE means there is a baseline, the loader makes the distinction
//between first edge is base line/separate baseline
//DITHER means the polygon only dithers what it covers

#define WF_ALWAYSCOVER  0
#define WF_BASELINE 1
#define WF_DITHER 2
//this is used only externally, but converted to baseline on load time
#define WF_HOVER 4
// cover animations
#define WF_COVERANIMS 8
// door polygons are not always drawn
#define WF_DISABLED 0x80

class GEM_EXPORT Wall_Polygon: public Gem_Polygon {
public:
	Wall_Polygon(Point *points,int count,Region *bbox) : Gem_Polygon(points,count,bbox) {};
	//is the point above the baseline
	bool PointCovered(Point &p);
	bool PointCovered(int x, int y);
	ieDword GetPolygonFlag() { return wall_flag; }
	void SetPolygonFlag(ieDword flg) { wall_flag=flg; }
	void SetBaseline(Point &a, Point &b);
public:
	ieDword wall_flag;
	Point base0, base1;
};

#endif
