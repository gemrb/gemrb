#ifndef POLYGON_H
#define POLYGON_H

#include "Sprite2D.h"
#include "Region.h"
#include "../../includes/RGBAColor.h"

typedef struct Point {
	short x, y;
} Point;

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT Gem_Polygon {
public:
	Gem_Polygon(Point* points, int count, bool precalculate = false,
		Color* color = NULL);
	~Gem_Polygon(void);
	Region BBox;
	Point* points;
	int count;
	Sprite2D* fill;
	bool PointIn(unsigned short x, unsigned short y);
};

#endif
