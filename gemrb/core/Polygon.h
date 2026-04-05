// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef POLYGON_H
#define POLYGON_H

#include "exports.h"
#include "ie_types.h"

#include "Region.h"

#include <memory>
#include <vector>

namespace GemRB {

class GEM_EXPORT Trapezoid {
public:
	int y1, y2;
	int left_edge, right_edge;
};

class GEM_EXPORT Gem_Polygon {
	std::vector<Trapezoid> ComputeTrapezoids() const;
	void RecalcBBox();
	void Rasterize();

public:
	using LineSegment = std::pair<Point, Point>;

	Gem_Polygon() = default; // beware on use
	Gem_Polygon(std::vector<Point>&&, const Region* bbox = nullptr);

	Region BBox;
	std::vector<Point> vertices;
	std::vector<std::vector<LineSegment>> rasterData; // same as vertices, but relative to BBox

	size_t Count() const { return vertices.size(); }

	bool PointIn(const Point& p) const;
	bool PointIn(int x, int y) const;

	bool IntersectsRect(const Region&) const;
};

// wall polygons are used to render area wallgroups
// wall polygons never create a surface
//ALWAYSCOVER wallpolygon blocks everything that it covers
//BASELINE means there is a baseline, the loader makes the distinction
//between first edge is base line/separate baseline
//DITHER means the polygon only dithers what it covers

#define WF_BASELINE 1
#define WF_DITHER   2
//this is used only externally, but converted to baseline on load time
#define WF_HOVER 4
// cover animations
#define WF_COVERANIMS 8
// door polygons are not always drawn
#define WF_DISABLED 0x80

class GEM_EXPORT WallPolygon : public Gem_Polygon {
public:
	using Gem_Polygon::Gem_Polygon;
	//is the point above the baseline
	bool PointBehind(const Point& p) const;
	ieDword GetPolygonFlag() const { return wallFlag; }
	void SetPolygonFlag(ieDword flg) { wallFlag = flg; }
	void SetBaseline(const Point& a, const Point& b);

	void SetDisabled(bool disabled);

public:
	ieDword wallFlag = 0;
	Point base0, base1;
};

using WallPolygonGroup = std::vector<std::shared_ptr<WallPolygon>>;
// the first of the pair are the walls in front, the second are the walls behind
using WallPolygonSet = std::pair<WallPolygonGroup, WallPolygonGroup>;

}

#endif
