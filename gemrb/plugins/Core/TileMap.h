#ifndef TILEMAP_H
#define TILEMAP_H

#include "TileOverlay.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT TileMap
{
private:
	std::vector<TileOverlay *> overlays;
public:
	TileMap(void);
	~TileMap(void);
	void AddOverlay(TileOverlay * overlay);
	void DrawOverlay(unsigned int index, Region viewport);
};

#endif
