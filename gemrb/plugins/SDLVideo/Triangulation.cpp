#include <vector>
#include "Triangulation.h"
#include "Region.h" // for Point

using namespace GemRB;

short vectorMultiplication(const Point &p1, const Point &p2, const Point &p3)
{ 
	return p1.x*p2.y - p1.x*p3.y - p2.x*p1.y + p2.x*p3.y + p3.x*p1.y - p3.x*p2.y; 
}

short Triangulation::getDirection(std::vector<Point> polygon)
{
	short min = polygon[0].x;
	Point p0, p1, p2;
	for (int i=0; i<polygon.size()-2; i++)
	{
		if (polygon[i].x <= min)
		{
			p0 = polygon[i];
			p1 = polygon[i+1];
			p2 = polygon[i+2];
		}
	}
	if (vectorMultiplication(p0, p1, p2) > 0) return 1;
	return -1;
}

bool Triangulation::pointInTriangle(const Point &pt, const Point &t0, const Point &t1, const Point &t2)
{
	short a = vectorMultiplication(pt, t0, t1);
	short b = vectorMultiplication(pt, t1, t2);
	short c = vectorMultiplication(pt, t2, t0);
	return (a < 0 && b < 0 && c < 0) || (a > 0 && b > 0 && c > 0);	
}

std::vector<Point> Triangulation::TriangulatePolygon(Point* points, unsigned int count)
{
	std::vector<Point> triangles;
	std::vector<Point> polygon = std::vector<Point>(points, points + count);
	short direction = getDirection(polygon);
	int i = 0;
	int prevIterationCount = polygon.size();
	while (polygon.size() != 3)
	{
		if (vectorMultiplication(polygon[i+1], polygon[i], polygon[i+2]) * direction < 0)
		{
			bool rightEar = true;
			for (int j=0; j<polygon.size(); j++)
			{
				if (pointInTriangle(polygon[j], polygon[i], polygon[i+1], polygon[i+2]))
				{
					i++;
					rightEar = false;
					break;
				}
			}
			if (rightEar)
			{
				triangles.push_back(polygon[i]);
				triangles.push_back(polygon[i+1]);
				triangles.push_back(polygon[i+2]);
				polygon.erase(polygon.begin()+i+1);
			}
		}
		else i++;
		if (i == polygon.size() - 3) 
		{
			if (prevIterationCount == polygon.size()) 
				break; // damn
			i = 0;
			prevIterationCount = polygon.size();
		}
	}
	triangles.push_back(polygon[0]);
	triangles.push_back(polygon[1]);
	triangles.push_back(polygon[2]);
	return triangles;
}
