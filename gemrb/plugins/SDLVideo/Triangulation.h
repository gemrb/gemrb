#ifndef TRIANGULATION_H
#define TRIANGULATION_H

namespace GemRB 
{
	class Point;

	class Triangulation
	{
	public:
		static std::vector<Point> TriangulatePolygon(Point* points, unsigned int count);
	private:
		static bool pointInTriangle(const Point &pt, const Point &t0, const Point &t1, const Point &t2);
		static short getDirection(std::vector<Point> polygon);
	};
}
#endif