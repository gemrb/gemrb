#ifndef TILEOVERLAY_H
#define TILEOVERLAY_H

#include "Tile.h"
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

class GEM_EXPORT TileOverlay
{
private:
	int w,h;
	std::vector<Tile*> tiles;
public:
	TileOverlay(int Width, int Height);
	~TileOverlay(void);
	void AddTile(Tile * tile);
	void Draw(void);
};

#endif
