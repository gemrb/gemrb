#include "WEDImp.h"
#include "../Core/TileSetMgr.h"
#include "../Core/Interface.h"

WEDImp::WEDImp(void)
{
	str = NULL;
	autoFree = false;
}

WEDImp::~WEDImp(void)
{
	if(str && autoFree)
		delete(str);
}

bool WEDImp::Open(DataStream * stream, bool autoFree)
{
	if(stream == NULL)
		return false;
	if(str && this->autoFree)
		delete(str);
	str = stream;
	this->autoFree = autoFree;
	char Signature[8];
	str->Read(Signature, 8);
	if(strncmp(Signature, "WED V1.3", 8) != 0) {
		printf("[WEDImporter]: This file is not a valid WED File\n");
		return false;
	}
	unsigned long OverlaysCount, DoorsCount, OverlaysOffset, SecHeaderOffset, DoorsOffset, DoorTilesOffset;
	str->Read(&OverlaysCount, 4);
	str->Read(&DoorsCount, 4);
	str->Read(&OverlaysOffset, 4);
	str->Read(&SecHeaderOffset, 4);
	str->Read(&DoorsOffset, 4);
	str->Read(&DoorTilesOffset, 4);
	str->Seek(OverlaysOffset, GEM_STREAM_START);
	for(unsigned int i = 0; i < OverlaysCount; i++) {
		Overlay o;
		str->Read(&o.Width, 2);
		str->Read(&o.Height, 2);
		str->Read(o.TilesetResRef, 8);
		str->Seek(4, GEM_CURRENT_POS);
		str->Read(&o.TilemapOffset, 4);
		str->Read(&o.TILOffset, 4);
		overlays.push_back(o);
	}
	return true;
}

TileMap * WEDImp::GetTileMap()
{
	TileMap * tm = new TileMap();
	//TODO: Implement Multi Overlay
	TileOverlay * over = new TileOverlay(overlays[0].Width, overlays[0].Height);
	DataStream * tisfile = core->GetResourceMgr()->GetResource(overlays[0].TilesetResRef, IE_TIS_CLASS_ID);
	if(!core->IsAvailable(IE_TIS_CLASS_ID)) {
		printf("[WEDImporter]: No TileSet Importer Available.\n");
		return NULL;
	}
	TileSetMgr * tis = (TileSetMgr*)core->GetInterface(IE_TIS_CLASS_ID);
	tis->Open(tisfile);
	for(int y = 0; y < overlays[0].Height; y++) {
		for(int x = 0; x < overlays[0].Width; x++) {
			str->Seek(overlays[0].TilemapOffset + (y*overlays[0].Width*10) + (x*10), GEM_STREAM_START);
			unsigned short	startindex, count;
			str->Read(&startindex, 2);
			str->Read(&count, 2);
			//TODO: Consider Alternative Tile and Overlay Mask
			str->Seek(overlays[0].TILOffset + (startindex*2), GEM_STREAM_START);
			unsigned short *indexes = (unsigned short*)malloc(count*2);
			str->Read(indexes, count*2);
			Tile * tile = tis->GetTile(indexes, count);
			over->AddTile(tile);
			free(indexes);
		}
	}
	tm->AddOverlay(over);
	core->FreeInterface(tis);
	return tm;
}
