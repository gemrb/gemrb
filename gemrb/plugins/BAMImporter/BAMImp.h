#ifndef BAMIMP_H
#define BAMIMP_H

#include "../Core/AnimationMgr.h"
#include "../../includes/globals.h"
#include "../../includes/RGBAColor.h"

typedef struct FrameEntry {
	unsigned short	Width;
	unsigned short	Height;
	short	XPos;
	short	YPos;
	unsigned long	FrameData;
} FrameEntry;

class BAMImp : public AnimationMgr
{
private:
	DataStream * str;
	bool autoFree;
	std::vector<FrameEntry> frames;
	std::vector<CycleEntry> cycles;
	Color Palette[256];
	unsigned char	CompressedColorIndex;
	unsigned long	FramesOffset, PaletteOffset, FLTOffset;
public:
	BAMImp(void);
	~BAMImp(void);
	bool Open(DataStream * stream, bool autoFree = false);
	Sprite2D * GetFrameFromCycle(unsigned char Cycle, unsigned short frame);
	Animation * GetAnimation(unsigned char Cycle, int x, int y, unsigned char mode = IE_NORMAL);
	AnimationFactory * GetAnimationFactory(const char * ResRef, unsigned char mode = IE_NORMAL);
	Sprite2D * GetFrame(unsigned short findex, unsigned char mode = IE_NORMAL);
	void * GetFramePixels(unsigned short findex, unsigned char mode = IE_NORMAL);
	/** This function will load the Animation as a Font */
	Font * GetFont();
	/** Debug Function: Returns the Global Animation Palette as a Sprite2D Object.
	If the Global Animation Palette is NULL, returns NULL. */
	Sprite2D * GetPalette();
};

#endif
