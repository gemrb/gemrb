#include "../../includes/win32def.h"
#include "Tile.h"

Tile::Tile(Animation * anim)
{
	this->anim = anim;
}

Tile::~Tile(void)
{
	if(anim)
		delete(anim);
}
