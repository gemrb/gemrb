#include "win32def.h"
#include "ScrollBar.h"
#include "Interface.h"

extern Interface * core;

ScrollBar::ScrollBar(void)
{
}

ScrollBar::~ScrollBar(void)
{
}

void ScrollBar::Draw(unsigned short x, unsigned short y)
{
	core->GetVideoDriver()->BlitSprite(frames[IE_GUI_SCROLLBAR_UP_UNPRESSED], x+XPos, y+YPos, true);
	int maxy = y+YPos+Height-frames[IE_GUI_SCROLLBAR_DOWN_UNPRESSED]->Height;
	int stepy = frames[IE_GUI_SCROLLBAR_TROUGH]->Height;
	int w = frames[IE_GUI_SCROLLBAR_UP_UNPRESSED]->Width;
	for(int dy = y+YPos+frames[IE_GUI_SCROLLBAR_UP_UNPRESSED]->Height; dy < maxy; dy+=stepy) {
		core->GetVideoDriver()->BlitSprite(frames[IE_GUI_SCROLLBAR_TROUGH], x+XPos+(Width/8), dy, true);
	}
	core->GetVideoDriver()->BlitSprite(frames[IE_GUI_SCROLLBAR_DOWN_UNPRESSED], x+XPos, maxy, true);
	core->GetVideoDriver()->BlitSprite(frames[IE_GUI_SCROLLBAR_SLIDER], x+XPos, y+YPos+frames[IE_GUI_SCROLLBAR_UP_UNPRESSED]->Height, true);
}

void ScrollBar::SetImage(unsigned char type, Sprite2D * img)
{
	if(type > IE_GUI_SCROLLBAR_SLIDER)
		return;
	frames[type] = img;
}
