#ifndef PATHFINDER_H
#define PATHFINDER_H

#include "ImageMgr.h"
#include <queue>

class Map;

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

struct PathNode {
	PathNode* Parent;
	PathNode* Next;
	unsigned short x;
	unsigned short y;
	unsigned int orient;
};

class GEM_EXPORT PathFinder {
private:
	Map *area;      //don't free this
	ImageMgr* sMap; //don't free this
	unsigned short* MapSet;
	std::queue< unsigned int> InternalStack;
	unsigned int Width, Height;
public:
	PathFinder(void);
	~PathFinder(void);
	/* Sets map for pathfinder */
	void SetMap(Map *area);
	/* Finds the nearest passable point */
	void AdjustPosition(unsigned int& goalX, unsigned int& goalY);
	/* Finds the path which leads the farthest from d */
	PathNode* RunAway(short sX, short sY, short dX, short dY, unsigned int PathLen, bool Backing);
	/* Returns true if there is no path to d */
	bool TargetUnreachable(short sX, short sY, short dX, short dY);
	/* Finds the path which leads to d */
	PathNode* FindPath(short sX, short sY, short dX, short dY);
	bool IsVisible(short sX, short sY, short dX, short dY);
private:
	void Leveldown(unsigned int px, unsigned int py, unsigned int& level,
		unsigned int& nx, unsigned int& ny, unsigned int& diff);
	void SetupNode(unsigned int x, unsigned int y, unsigned int Cost);
	//maybe this is unneeded and orientation could be calculated on the fly
	unsigned char GetOrient(short sX, short sY, short dX, short dY);
};

#endif
