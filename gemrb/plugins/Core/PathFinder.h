#ifndef PATHFINDER_H
#define PATHFINDER_H

#include "ImageMgr.h"
#include <queue>

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
	PathNode *Parent;
	PathNode *Next;
	unsigned short x;
	unsigned short y;
	unsigned int orient;
};

class GEM_EXPORT PathFinder
{
private:
	ImageMgr * sMap;
	unsigned short *MapSet;
	std::queue<unsigned int> InternalStack;
	unsigned int Width, Height;
public:
	PathFinder(void);
	~PathFinder(void);
	void SetMap(ImageMgr * sMap, unsigned int Width, unsigned int Height);
	PathNode * FindPath(short sX, short sY, short dX, short dY);
private:
	void Leveldown(unsigned int px, unsigned int py, unsigned int &level, unsigned int &nx, unsigned int &ny, unsigned int &diff);
	void SetupNode(unsigned int x,unsigned int y, unsigned int Cost);
//maybe this is unneeded and orientation could be calculated on the fly
	unsigned char GetOrient(short sX, short sY, short dX, short dY);
};

#endif
