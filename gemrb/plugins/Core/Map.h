#ifndef MAP_H
#define MAP_H

#include "TileMap.h"
#include "Actor.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

typedef struct ActorBlock {
	unsigned short XPos, YPos;
	unsigned short XDes, YDes;
	unsigned char Orientation;
	unsigned char AnimID;
	Actor * actor;
} ActorBlock;

class GEM_EXPORT Map
{
private:
	TileMap * tm;
	std::vector<Animation*> animations;
	std::vector<ActorBlock> actors;
public:
	Map(void);
	~Map(void);
	void AddTileMap(TileMap * tm);
	void DrawMap(void);
	void AddAnimation(Animation * anim);
	void AddActor(ActorBlock actor);
};

#endif
