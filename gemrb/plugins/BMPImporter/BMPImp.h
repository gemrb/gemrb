#ifndef BMPIMP_H
#define BMPIMP_H

#include "../Core/ImageMgr.h"

class BMPImp : public ImageMgr
{
private:
	DataStream * str;
	bool autoFree;

	//BITMAPINFOHEADER
	unsigned long Size, Width, Height, Compression, ImageSize, ColorsUsed, ColorsImportant;
	unsigned short Planes, BitCount;

	//COLORTABLE
	unsigned long NumColors;
	Color * Palette;

	//RASTERDATA
	void * pixels;

	//OTHER
	unsigned short PaddedRowLength;
public:
	BMPImp(void);
	~BMPImp(void);
	bool Open(DataStream * stream, bool autoFree = true);
	Sprite2D * GetImage();
	/** No descriptions */
	void GetPalette(int index, int colors, Color * pal);
public:
	void release(void)
	{
		delete this;
	}
};

#endif
