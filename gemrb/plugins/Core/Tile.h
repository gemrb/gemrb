#ifndef TILE_H
#define TILE_H

#include "../../includes/RGBAColor.h"
#include "../Core/Animation.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT Tile
{
public:
	Tile(Animation * anim);
	~Tile(void);
	unsigned long Flags;
	Color SearchMap[4];
	Color HeightMap[4];
	Color LightMap[4];
	Color NLightMap[4];
	Animation * anim;
};

#endif
