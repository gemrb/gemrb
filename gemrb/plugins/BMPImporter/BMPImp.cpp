#include "../../includes/win32def.h"
#include "BMPImp.h"
#include "../../includes/RGBAColor.h"
#include "../Core/Interface.h"

BMPImp::BMPImp(void)
{
	str = NULL;
	autoFree = false;
	Palette = NULL;
	pixels = NULL;
}

BMPImp::~BMPImp(void)
{
	if(str && autoFree)
		delete(str);
	if(Palette)
		free(Palette);
	if(pixels)
		free(pixels);
}

bool BMPImp::Open(DataStream * stream, bool autoFree)
{
	if(stream == NULL)
		return false;
	if(str && this->autoFree)
		delete(str);
	if(Palette)
		free(Palette);
	str = stream;
	this->autoFree = autoFree;
	//BITMAPFILEHEADER
	char Signature[2];
	unsigned long FileSize, DataOffset;

	str->Read(Signature, 2);
	if(strncmp(Signature, "BM", 2) != 0) {
		printf("[BMPImporter]: Not a valid BMP File.\n");
		return false;
	}
	str->Read(&FileSize, 4);
	str->Seek(4, GEM_CURRENT_POS);
	str->Read(&DataOffset, 4);
	
	//BITMAPINFOHEADER
	
	str->Read(&Size, 4);
	str->Read(&Width, 4);
	str->Read(&Height, 4);
	str->Read(&Planes, 2);
	str->Read(&BitCount, 2);
	str->Read(&Compression, 4);
	str->Read(&ImageSize, 4);
	str->Seek(16, GEM_CURRENT_POS);
	//str->Read(&ColorsUsed, 4);
	//str->Read(&ColorsImportant, 4);
	if(Compression != 0) {
		printf("[BMPImporter]: Compressed %d-bits Image, Not Supported.", BitCount);
		return false;
	}
	//COLORTABLE
	Palette = NULL;
	NumColors = BitCount*8;
	if(BitCount <= 8) {
		Palette = (Color*)malloc(4*NumColors);
		for(unsigned int i = 0; i < NumColors; i++) {
			str->Read(&Palette[i], 4);
		}
	}
	//RASTERDATA
	switch(BitCount) {
		case 24:
			PaddedRowLength = ((Width*3)+((Width*3)%4));
		break;

		case 16:
			PaddedRowLength = ((Width*2)+((Width*2)%4));
		break;

		case 8:
			PaddedRowLength = (Width+(Width%4));
		break;

		default:
			printf("[BMPImporter]: BitCount not supported.\n");
			return false;
	}
	void * rpixels = malloc(PaddedRowLength*Height);
	str->Read(rpixels, PaddedRowLength*Height);
	if(BitCount == 24) {
		pixels = malloc(Width*Height*3);
		unsigned char * dest = (unsigned char*)pixels;
		dest += (Height)*(Width*3);
		unsigned char * src  = (unsigned char*)rpixels;
		for(int i = Height-1; i >= 0; i--) {
			dest -= (Width*3);
			memcpy(dest, src, Width*3);
			src+=PaddedRowLength;
		}
	}
	else if(BitCount == 8) {
		pixels = malloc(Width*Height);
		unsigned char * dest = (unsigned char*)pixels;
		dest += Height*Width;
		unsigned char * src = (unsigned char*)rpixels;
		for(int i = Height-1; i >= 0; i--) {
			dest -= Width;
			memcpy(dest, src, Width);
			src+=PaddedRowLength;
		}
	}
	free(rpixels);
	return true;
}

Sprite2D * BMPImp::GetImage()
{
	Sprite2D * spr = NULL;
	if(BitCount == 24) {
		spr = core->GetVideoDriver()->CreateSprite(Width, Height, 24, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000, pixels);
	}
	else if(BitCount == 8) {
		spr = core->GetVideoDriver()->CreateSprite8(Width, Height, 8, pixels, Palette);
	}
	return spr;
}
/** No descriptions */
void BMPImp::GetPalette(int index, int colors, Color * pal){
	unsigned char * p = (unsigned char*)pixels;
	p+=PaddedRowLength*index;
	if(BitCount == 24) {
		for(int i = 1; i < colors; i++) {
			pal[i].b = *p++;
			pal[i].g = *p++;
			pal[i].r = *p++;
		}
	}
	else if(BitCount == 8) {
		for(int i = 0; i < colors; i++) {
			pal[i].r = Palette[*p++].r;
			pal[i].g = Palette[*p++].g;
			pal[i].b = Palette[*p++].b;
		}
	}
}
