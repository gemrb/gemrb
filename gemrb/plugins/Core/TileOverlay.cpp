#include "win32def.h"
#include "TileOverlay.h"
#include "Interface.h"
#include "Video.h"
#include <math.h>

extern Interface * core;

TileOverlay::TileOverlay(int Width, int Height)
{
	w = Width;
	h = Height;
}

TileOverlay::~TileOverlay(void)
{
	for(unsigned int i = 0; i < tiles.size(); i++) {
		delete(tiles[i]);
	}
}

void TileOverlay::AddTile(Tile * tile)
{
	tiles.push_back(tile);
}

void TileOverlay::Draw(void)
{
	Video * vid = core->GetVideoDriver();
	Region viewport = vid->GetViewport();
	if(viewport.x < 0)
		viewport.x = 0;
	else if((viewport.x+viewport.w) > w*64)
		viewport.x = (w*64 - viewport.w);
	if(viewport.y < 0)
		viewport.y = 0;
	else if((viewport.y+viewport.h) > h*64)
		viewport.y = (h*64 - viewport.h);
	vid->SetViewport(viewport.x, viewport.y);
	int sx = viewport.x / 64;
	int sy = viewport.y / 64;
	int dx = sx+(int)ceil(viewport.w/64.0);
	int dy = sy+(int)ceil(viewport.h/64.0);
	for(int y = sy; y < dy; y++) {
		for(int x = sx; x < dx; x++) {
			vid->BlitSprite(tiles[(y*w)+x]->anim->NextFrame(),x*64 ,y*64);
		}
	}
}
