#include "../../includes/win32def.h"
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

void TileOverlay::Draw(Region viewport)
{
	Video * vid = core->GetVideoDriver();
	Region vp = vid->GetViewport();
	vp.w = viewport.w;
	vp.h = viewport.h;
	if(vp.x < 0)
		vp.x = 0;
	else if((vp.x+vp.w) > w*64)
		vp.x = (w*64 - vp.w);
	if(vp.y < 0)
		vp.y = 0;
	else if((vp.y+vp.h) > h*64)
		vp.y = (h*64 - vp.h);
	vid->SetViewport(vp.x, vp.y);
	int sx = vp.x / 64;
	int sy = vp.y / 64;
	int dx = (int)ceil((vp.x+vp.w) / 64.0);
	int dy = (int)ceil((vp.y+vp.h) / 64.0);
	for(int y = sy; y < dy; y++) {
		for(int x = sx; x < dx; x++) {
			vid->BlitSprite(tiles[(y*w)+x]->anim->NextFrame(),viewport.x+(x*64) ,viewport.y+(y*64));
		}
	}
}
