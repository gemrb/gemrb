#include "../../includes/win32def.h"
#include "TileMap.h"

TileMap::TileMap(void)
{
}

TileMap::~TileMap(void)
{
}

void TileMap::AddOverlay(TileOverlay * overlay)
{
	overlays.push_back(overlay);
}

void TileMap::DrawOverlay(unsigned int index)
{
	if(index < overlays.size())
		overlays[index]->Draw();
}
