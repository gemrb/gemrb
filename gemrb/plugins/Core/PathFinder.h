#ifndef PATHFINDER_H
#define PATHFINDER_H

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

//searchmap conversion bits

#define PATH_MAP_IMPASSABLE 0
#define PATH_MAP_PASSABLE 1   
#define PATH_MAP_TRAVEL   2
#define PATH_MAP_NO_SEE   4
#define PATH_MAP_SIDEWALL 8
#define PATH_MAP_DOOR_OPAQUE 16
#define PATH_MAP_DOOR_TRANSPARENT 32

struct PathNode {
	PathNode* Parent;
	PathNode* Next;
	unsigned short x;
	unsigned short y;
	unsigned int orient;
};

#endif
