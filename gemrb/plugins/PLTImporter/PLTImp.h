#ifndef PLTIMP_H
#define PLTIMP_H

#include "../Core/ImageMgr.h"

class PLTImp : public ImageMgr
{
private:
	DataStream * str;
	bool autoFree;

	unsigned long Width, Height;
	void * pixels;
	Color * Palettes[8];
	int palIndexes[8];
public:
	PLTImp(void);
	~PLTImp(void);
	bool Open(DataStream * stream, bool autoFree = false);
	Sprite2D * GetImage();
  /** No descriptions */
  void GetPalette(int index, int colors, Color * pal);
};

#endif
