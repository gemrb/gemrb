#include "../../includes/win32def.h"
#include "Polygon.h"
#include "Interface.h"

extern Interface * core;

Gem_Polygon::Gem_Polygon(Point * points, int count, bool precalculate, Color * color)
{
	this->points = (Point*)malloc(count*sizeof(Point));
	memcpy(this->points, points, count*sizeof(Point));
	this->count = count;
	if(precalculate) {
		fill = core->GetVideoDriver()->PrecalculatePolygon(points, count, *color);
	}
	else
		fill = NULL;
}

Gem_Polygon::~Gem_Polygon(void)
{
	free(points);
	if(fill)
		core->GetVideoDriver()->FreeSprite(fill);
}

bool Gem_Polygon::PointIn(unsigned short x, unsigned short y)
{
	register int   j, yflag0, yflag1, xflag0 , index;
	register long ty, tx;
  bool inside_flag = false;
	Point *vtx0, *vtx1;

	index = 0;

	tx = x;
	ty = y;

	vtx0 = &points[count-1];
	yflag0 = ( vtx0->y >= ty );
	vtx1 = &points[index];

	for( j = count+1; --j ; ) {
		yflag1 = ( vtx1->y >= ty );
		if(yflag0 != yflag1) {
			xflag0 = ( vtx0->x >= tx );
			if( xflag0 == ( vtx1->x >= tx ) ) {
				if( xflag0 ) inside_flag = !inside_flag;
			} else {
				if( ( vtx1->x - ( vtx1->y-ty ) * ( vtx0->x-vtx1->x ) / ( vtx0->y-vtx1->y )) >= tx ) {
					inside_flag = !inside_flag;
				}
			}
		}
		yflag0 = yflag1;
		vtx0 = vtx1;
		vtx1 = &points[++index];
	}
	return inside_flag;
}
