#ifndef MOSIMP_H
#define MOSIMP_H

#include "../Core/ImageMgr.h"

class MOSImp : public ImageMgr
{
private:
	DataStream * str;
	bool autoFree;
	unsigned short Width, Height, Cols, Rows;
	unsigned long  BlockSize, PalOffset;
public:
	MOSImp(void);
	~MOSImp(void);
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
