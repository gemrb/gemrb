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
#define PATH_MAP_AREAMASK 15
#define PATH_MAP_FREE 0
#define PATH_MAP_DOOR_OPAQUE 16
#define PATH_MAP_DOOR_TRANSPARENT 32
#define PATH_MAP_PC 64
#define PATH_MAP_NPC 128
#define PATH_MAP_ACTOR (PATH_MAP_PC|PATH_MAP_NPC)
#define PATH_MAP_DOOR (PATH_MAP_DOOR_OPAQUE|PATH_MAP_DOOR_TRANSPARENT)
#define PATH_MAP_NOTAREA (PATH_MAP_ACTOR|PATH_MAP_DOOR)
#define PATH_MAP_NOTDOOR (PATH_MAP_ACTOR|PATH_MAP_AREAMASK)
#define PATH_MAP_NOTACTOR (PATH_MAP_DOOR|PATH_MAP_AREAMASK)

struct PathNode {
	PathNode* Parent;
	PathNode* Next;
	unsigned short x;
	unsigned short y;
	unsigned int orient;
};

#endif
