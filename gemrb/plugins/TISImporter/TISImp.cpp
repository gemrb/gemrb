#include "TISImp.h"
#include "../../includes/RGBAColor.h"
#include "../Core/Interface.h"

TISImp::TISImp(void)
{
	str = NULL;
	autoFree = false;
}

TISImp::~TISImp(void)
{
	if(str && autoFree)
		delete(str);
}

bool TISImp::Open(DataStream * stream, bool autoFree)
{
	if(stream == NULL)
		return false;
	if(str && this->autoFree)
		delete(str);
	str = stream;
	this->autoFree = autoFree;
	char Signature[8];
	str->Read(Signature, 1);
	headerShift = 0;
	if(Signature[0] == 'T') {
		str->Seek(-1, GEM_CURRENT_POS);
		str->Read(Signature, 8);	
		if(strncmp(Signature, "TIS V1  ", 8) != 0) {
			printf("[TISImporter]: Not a Valid TIS File.\n");
			return false;
		}
		str->Read(&TilesCount, 4);
		str->Read(&TilesSectionLen, 4);
		str->Read(&headerShift, 4);
		str->Read(&TileSize, 4);
	}
	return true;
}

Tile * TISImp::GetTile(unsigned short * indexes, int count)
{
	Animation * ani = new Animation(indexes, count);
	ani->x = ani->y = 0;
	for(int i = 0; i < count; i++) {
		ani->AddFrame(GetTile(indexes[i]), indexes[i]);
	}
	return new Tile(ani);
}
Sprite2D * TISImp::GetTile(int index)
{
	RevColor RevCol[256];
	Color Palette[256];
	void * pixels = malloc(4096);
	str->Seek((index*(1024+4096)+headerShift), GEM_STREAM_START);
	str->Read(&RevCol, 1024);
	for(int i = 0; i < 256; i++) {
		Palette[i].r = RevCol[i].r;
		Palette[i].g = RevCol[i].g;
		Palette[i].b = RevCol[i].b;
		Palette[i].a = RevCol[i].a;
	}
	str->Read(pixels, 4096);
	Sprite2D * spr = core->GetVideoDriver()->CreateSprite8(64, 64, 8, pixels, Palette);
	spr->XPos = spr->YPos = 0;
	return spr;
}
