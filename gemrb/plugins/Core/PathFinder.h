#ifndef PATHFINDER_H
#define PATHFINDER_H

#include "ImageMgr.h"
#include <vector>

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

typedef struct PathNode {
	short x,y;
	unsigned char orient;
	unsigned char passable;
	unsigned long distance;
	unsigned long cost;
	PathNode *Next;
	PathNode *Parent;
} PathNode;

class GEM_EXPORT PathFinder
{
private:
	ImageMgr * sMap;
	PathNode *** MapSet;
	bool ** ClosedSet;
	PathNode **OpenStack;
	int OpenStackCount;
	std::vector<PathNode*> FinalStack;
	int Width, Height;
public:
	PathFinder(void);
	~PathFinder(void);
	void SetMap(ImageMgr * sMap, int Width, int Height);
	PathNode * FindPath(short sX, short sY, short dX, short dY);
private:
	unsigned long GetDistance(short sX, short sY, short dX, short dY);
	void FreeMatrices();
	unsigned char GetOrient(short sX, short sY, short dX, short dY);
	PathNode * HeapRemove();
	void HeapAdd(PathNode * node);
};

#endif
