#include "../../includes/win32def.h"
#include "ScrollBar.h"
#include "Interface.h"

extern Interface * core;

ScrollBar::ScrollBar(void)
{
	Pos = 0;
	Max = 10;
	State = 0;
	ta = NULL;
}

ScrollBar::~ScrollBar(void)
{
}

void ScrollBar::SetPos(int NewPos)
{
	Pos=NewPos;
	if(ta) {
		TextArea * t = (TextArea*)ta;
		t->SetRow(Pos);
	}
	if(VarName[0] != 0) {
		core->GetDictionary()->SetAt(VarName, Pos);
	}
}

void ScrollBar::RedrawScrollBar(const char *Variable, int Sum)
{
	if(strnicmp(VarName, Variable, MAX_VARIABLE_LENGTH) ) return;
	SetMax((unsigned short) Sum);
}

void ScrollBar::Draw(unsigned short x, unsigned short y)
{
	if(!Changed)
		return;
	Changed=false;
	if(XPos==65535)
		return;
	unsigned short upmx = 0, upMx = frames[IE_GUI_SCROLLBAR_UP_UNPRESSED]->Width, upmy = 0, upMy = frames[IE_GUI_SCROLLBAR_UP_UNPRESSED]->Height;
	unsigned short domx = 0, doMx = frames[IE_GUI_SCROLLBAR_DOWN_UNPRESSED]->Width, domy = Height-frames[IE_GUI_SCROLLBAR_DOWN_UNPRESSED]->Height, doMy = Height;
	unsigned short slmx = 0, slMx = frames[IE_GUI_SCROLLBAR_SLIDER]->Width, slmy = upMy+(Pos*((domy-frames[5]->Height-upMy)/(double)(Max == 1 ? Max : Max -1))), slMy = slmy+frames[IE_GUI_SCROLLBAR_SLIDER]->Height;
	unsigned short slx  = (Width/2)-(frames[IE_GUI_SCROLLBAR_SLIDER]->Width/2);

	if((State & UP_PRESS) != 0)
		core->GetVideoDriver()->BlitSprite(frames[IE_GUI_SCROLLBAR_UP_PRESSED], x+XPos, y+YPos, true);
	else
		core->GetVideoDriver()->BlitSprite(frames[IE_GUI_SCROLLBAR_UP_UNPRESSED], x+XPos, y+YPos, true);
	int maxy = y+YPos+Height-frames[IE_GUI_SCROLLBAR_DOWN_UNPRESSED]->Height;
	int stepy = frames[IE_GUI_SCROLLBAR_TROUGH]->Height;
	int w = frames[IE_GUI_SCROLLBAR_UP_UNPRESSED]->Width;
	for(int dy = y+YPos+frames[IE_GUI_SCROLLBAR_UP_UNPRESSED]->Height; dy < maxy; dy+=stepy) {
		core->GetVideoDriver()->BlitSprite(frames[IE_GUI_SCROLLBAR_TROUGH], x+XPos+((Width/2)-frames[IE_GUI_SCROLLBAR_TROUGH]->Width/2), dy, true);
		//core->GetVideoDriver()->BlitSprite(frames[IE_GUI_SCROLLBAR_TROUGH], x+XPos+(Width/8)+1, dy, true);
	}
	if((State & DOWN_PRESS) != 0)
		core->GetVideoDriver()->BlitSprite(frames[IE_GUI_SCROLLBAR_DOWN_PRESSED], x+XPos, maxy, true);
	else
		core->GetVideoDriver()->BlitSprite(frames[IE_GUI_SCROLLBAR_DOWN_UNPRESSED], x+XPos, maxy, true);
	core->GetVideoDriver()->BlitSprite(frames[IE_GUI_SCROLLBAR_SLIDER], x+XPos+slx, y+YPos+slmy, true);
}

void ScrollBar::SetImage(unsigned char type, Sprite2D * img)
{
	if(type > IE_GUI_SCROLLBAR_SLIDER)
		return;
	frames[type] = img;
	Changed = true;
}
/** Mouse Button Down */
void ScrollBar::OnMouseDown(unsigned short x, unsigned short y, unsigned char Button, unsigned short Mod)
{
	((Window*)Owner)->Invalidate();
	unsigned short upmx = 0, upMx = frames[0]->Width, upmy = 0, upMy = frames[0]->Height;
	unsigned short domy = Height-frames[2]->Height;
	unsigned short slheight = domy - upMy;
	unsigned short refheight = slheight - frames[5]->Height;
	double step = refheight/(double)(Max == 1 ? Max : Max-1);
	unsigned short yzero = upMy+(frames[5]->Height/2);
	unsigned short ymax = yzero+refheight;
	unsigned short ymy = y-yzero;
	unsigned short domx = 0, doMx = frames[2]->Width, doMy = Height;
	unsigned short slmx = 0, slMx = frames[5]->Width, slmy = yzero+(Pos*step), slMy = slmy+frames[5]->Height;
	if((x >= upmx) && (y >= upmy)) {
		if((x <= upMx) && (y <= upMy)) {
			if(Pos > 0)
				SetPos(Pos-1);
			State |= UP_PRESS;
			return;
		}
	}
	if((x >= domx) && (y >= domy)) {
		if((x <= doMx) && (y <= doMy)) {
			if(Pos < (Max-1))
				SetPos(Pos+1);
			State |= DOWN_PRESS;
			return;
		}
	}
	if((x >= slmx) && (y >= slmy)) {
		if((x <= slMx) && (y <= slMy)) {
			State |= SLIDER_GRAB;
			return;
		}
	}
	if(y <= yzero) {
		SetPos(0);
		return;
	}
	if(y >= ymax) {
		SetPos(Max-1);
		return;
	}
	unsigned short befst = (unsigned short) (ymy / step);
	unsigned short aftst = befst + 1;
	if((ymy-(befst*step)) < ((aftst*step)-ymy)) {
		SetPos(befst);
	}
	else {
		SetPos(aftst);
	}
}
/** Mouse Button Up */
void ScrollBar::OnMouseUp(unsigned short x, unsigned short y, unsigned char Button, unsigned short Mod)
{
	Changed = true;
	State = 0;
}
/** Mouse Over Event */
void ScrollBar::OnMouseOver(unsigned short x, unsigned short y)
{
	if((State & SLIDER_GRAB) != 0) {
		((Window*)Owner)->Invalidate();
		unsigned short upMy = frames[0]->Height;
		unsigned short domy = Height-frames[2]->Height;
		unsigned short slheight = domy - upMy;
		unsigned short refheight = slheight - frames[5]->Height;
		double step = refheight/(double)(Max == 1 ? Max : Max-1);
		unsigned short yzero = upMy+(frames[5]->Height/2);
		unsigned short ymax = yzero+refheight;
		unsigned short ymy = y-yzero;
		if(y <= yzero) {
			SetPos(0);
			return;
		}
		if(y >= ymax) {
			SetPos(Max-1);
			return;
		}
		unsigned short befst = (unsigned short) (ymy / step);
		unsigned short aftst = befst + 1;
		if((ymy-(befst*step)) < ((aftst*step)-ymy)) {
			SetPos(befst);
		}
		else {
			if(aftst <= (Max-1))
				SetPos(aftst);
		}
	}
}

/** Sets the Text of the current control */
int ScrollBar::SetText(const char * string, int pos)
{
	return 0;
}

/** Sets the Maximum Value of the ScrollBar */
void ScrollBar::SetMax(unsigned short Max)
{
	this->Max = Max;
	if(Max == 0)
		SetPos(0);
	else if(Pos >= Max)
		SetPos(Max-1);
	Changed = true;
}
