#include "../../includes/win32def.h"
#include "Font.h"
#include "Interface.h"

extern Interface * core;
unsigned long lastX = 0;

Font::Font(int w, int h, void * palette, bool cK, int index)
{
	lastX = 0;
	count = 0;
	void * pixels = malloc(w*h);
	sprBuffer = core->GetVideoDriver()->CreateSprite8(w, h, 8, pixels, palette, cK, index);
	this->palette = core->GetVideoDriver()->GetPalette(sprBuffer);
	maxHeight = h;
}

Font::~Font(void)
{
	free(palette);
	core->GetVideoDriver()->FreeSprite(sprBuffer);
	/*
	Since we assume that the font was loaded from a factory Object,
	we don't need to free the sprites, those will be freed directly
	by the Factory Object destructor.
	*/
	/*
	std::vector<Sprite2D*>::iterator m;
	for(; chars.size() != 0; ) {
	m = chars.begin();
		core->GetVideoDriver()->FreeSprite((*m));
		chars.erase(m);
	}
	*/
}

void Font::AddChar(void * spr, int w, int h, short xPos, short yPos)
{
	if(!spr) {
		size[count].x = 0;
		size[count].y = 0;
		size[count].w = 0;
		size[count].h = 0;
		this->xPos[count] = 0;
		this->yPos[count] = 0;
		count++;
		return;
	}
	unsigned char * startPtr = (unsigned char*)sprBuffer->pixels;
	unsigned char * currPtr;
	unsigned char * srcPtr = (unsigned char*)spr;
	for(int y = 0; y < h; y++) {
		currPtr = startPtr + (y*sprBuffer->Width) + lastX;
		memcpy(currPtr, srcPtr, w);
		srcPtr += w;		
	}
	size[count].x = lastX;
	size[count].y = 0;
	size[count].w = w;
	size[count].h = h;
	this->xPos[count] = xPos;
	this->yPos[count] = yPos;
	count++;
	lastX+=w;
	/*if(count == 0) {
		palette = core->GetVideoDriver()->GetPalette(spr);
	}
	if(maxHeight < spr->YPos)
		maxHeight = spr->YPos;
	//chars.push_back(spr);	
	chars[count++] = spr;*/

}

bool written = false;

void Font::PrintFromLine(int startrow, Region rgn, unsigned char * string, Color *hicolor, unsigned char Alignment, bool anchor, Font * initials, Color *initcolor, Sprite2D * cursor, int curpos)
{
	Color * pal = NULL, *ipal = NULL;
	pal = hicolor;
	if(ipal != NULL) {
		ipal = initcolor;
	}
	else
		ipal = pal;
	if(!pal) {
		pal = palette;
	}
	/*for(int i = 0; i < 255; i++) {
		core->GetVideoDriver()->SetPalette(chars[i], pal);
	}*/
	core->GetVideoDriver()->SetPalette(sprBuffer, pal);
	Video * video = core->GetVideoDriver();
	int len = strlen((char*)string);
	char * tmp = (char*)malloc(len+1);
	strcpy(tmp, (char*)string);
	SetupString(tmp, rgn.w);
	int ystep = 0;
	if(Alignment & IE_FONT_SINGLE_LINE) {
		for(int i = 0; i < len; i++) {
			if(tmp[i] != 0)
				if(ystep < yPos[(unsigned char)tmp[i]-1])//chars[(unsigned char)tmp[i]-1]->YPos)
					ystep = yPos[(unsigned char)tmp[i]-1];//chars[(unsigned char)tmp[i]-1]->YPos;
		}
	}
	else
		ystep = size[1].h;//chars[1]->Height;
	int x = 0, y = ystep;
	if(Alignment & IE_FONT_ALIGN_CENTER) {
		int w = CalcStringWidth(tmp);
		x = (rgn.w / 2)-(w/2);
	}
	else if(Alignment & IE_FONT_ALIGN_RIGHT) {
		int w = CalcStringWidth(tmp);
		x = (rgn.w-w);
	}
	if(Alignment & IE_FONT_ALIGN_MIDDLE) {
		int h = 0;
		for(int i = 0; i <= len; i++) {
			if((tmp[i] == 0) || (tmp[i] == '\n'))
				h++;
		}
		h = h*ystep;
		y += (rgn.h/2)-(h/2);
	}
	int row = 0;
	for(int i = 0; i < len; i++) {
		if(row < startrow) {
			if(tmp[i] == 0) {
				row++;
			}
			continue;
		}
		if((tmp[i] == 0) || (tmp[i] == '\n')) {
			y+=ystep;
			x=0;
			if(Alignment & IE_FONT_ALIGN_CENTER) {
				int w = CalcStringWidth(&tmp[i+1]);
				x = (rgn.w / 2)-(w/2);
			}
			else if(Alignment & IE_FONT_ALIGN_RIGHT) {
				int w = CalcStringWidth(&tmp[i+1]);
				x = (rgn.w-w);
			}
			continue;
		}
		unsigned char currChar = (unsigned char)tmp[i]-1;
		//Sprite2D * spr = chars[(unsigned char)tmp[i]-1];
		x+=xPos[currChar];//spr->XPos;
		//video->BlitSprite(spr, x+rgn.x, y+rgn.y, true, &rgn);
		video->BlitSpriteRegion(sprBuffer, size[currChar], x+rgn.x-xPos[currChar], y+rgn.y-yPos[currChar], true, &rgn);
		if(cursor &&  (curpos == i))
			video->BlitSprite(cursor, x-xPos[currChar]+rgn.x, y+rgn.y, true, &rgn);//spr->XPos+rgn.x, y+rgn.y, true, &rgn);
		x+=size[currChar].w-xPos[currChar];//spr->Width-spr->XPos;
		
	}
	free(tmp);
}

void Font::Print(Region rgn, unsigned char * string, Color *hicolor, unsigned char Alignment, bool anchor, Font * initials, Color *initcolor, Sprite2D * cursor, int curpos)
{
	Color * pal = NULL, *ipal = NULL;
	pal = hicolor;
	if(ipal != NULL) {
		ipal = initcolor;
	}
	else
		ipal = pal;
	if(!pal) {
		pal = palette;
	}
	//for(int i = 0; i < 255; i++) {
	//	core->GetVideoDriver()->SetPalette(chars[i], pal);
	//}
	core->GetVideoDriver()->SetPalette(sprBuffer, pal);
	Video * video = core->GetVideoDriver();
	int len = strlen((char*)string);
	char * tmp = (char*)malloc(len+1);
	strcpy(tmp, (char*)string);
	SetupString(tmp, rgn.w);
	int ystep = 0;
	if(Alignment & IE_FONT_SINGLE_LINE) {
		for(int i = 0; i < len; i++) {
			if(tmp[i] != 0)
				if(ystep < yPos[(unsigned char)tmp[i]-1])//chars[(unsigned char)tmp[i]-1]->YPos)
					ystep = yPos[(unsigned char)tmp[i]-1];//chars[(unsigned char)tmp[i]-1]->YPos;
		}
	}
	else
		ystep = size[1].h;//chars[1]->Height;
	int x = 0, y = ystep;
	if(Alignment & IE_FONT_ALIGN_CENTER) {
		int w = CalcStringWidth(tmp);
		x = (rgn.w / 2)-(w/2);
	}
	else if(Alignment & IE_FONT_ALIGN_RIGHT) {
		int w = CalcStringWidth(tmp);
		x = (rgn.w-w);
	}
	if(Alignment & IE_FONT_ALIGN_MIDDLE) {
		int h = 0;
		for(int i = 0; i <= len; i++) {
			if(tmp[i] == 0)
				h++;
		}
		h = h*ystep;
		y += (rgn.h/2)-(h/2);
	}
	else if(Alignment & IE_FONT_ALIGN_TOP) {
		y+=5;
	}
	for(int i = 0; i < len; i++) {
		if(tmp[i] == 0) {
			y+=ystep;
			x=0;
			if(Alignment & IE_FONT_ALIGN_CENTER) {
				int w = CalcStringWidth(&tmp[i+1]);
				x = (rgn.w / 2)-(w/2);
			}
			else if(Alignment & IE_FONT_ALIGN_RIGHT) {
				int w = CalcStringWidth(&tmp[i+1]);
				x = (rgn.w-w);
			}
			continue;
		}
		//Sprite2D * spr = chars[(unsigned char)tmp[i]-1];
		unsigned char currChar = (unsigned char)tmp[i]-1;
		x+=xPos[currChar];//spr->XPos;
		//video->BlitSprite(spr, x+rgn.x, y+rgn.y, true, &rgn);
		video->BlitSpriteRegion(sprBuffer, size[currChar], x+rgn.x-xPos[currChar], y+rgn.y-yPos[currChar], true, &rgn);
		if(cursor &&  (curpos == i))
			video->BlitSprite(cursor, x-xPos[currChar]+rgn.x, y+rgn.y, true, &rgn);//spr->XPos+rgn.x, y+rgn.y, true, &rgn);
		x+=size[currChar].w-xPos[currChar];//spr->Width-spr->XPos;
		
	}
	free(tmp);
}

int Font::CalcStringWidth(const char * string)
{
	int ret = 0, len = strlen(string);
	for(int i = 0; i < len; i++) {
		ret += size[(unsigned char)string[i]-1].w;//chars[(unsigned char)string[i]-1]->Width;
	}
	return ret;
}

void Font::SetupString(char * string, int width)
{
	int len = strlen(string);
	int lastpos = 0;
	int x = 0, wx = 0;
	bool endword = false;
	for(int pos = 0; pos < len; pos++) {
		if(x+wx > width) {
			if(!endword && (x == 0))
				lastpos = pos;
			string[lastpos] = 0;
			x = 0;
		}
		if(string[pos] == 0) {
			continue;
		}
		endword = false;
		if(string[pos] == '\n') {
			string[pos] = 0;
			x = 0;
			wx = 0;
			lastpos = pos;
			endword = true;
			continue;
		}
		wx += size[(unsigned char)string[pos]-1].w;//chars[((unsigned char)string[pos])-1]->Width;
		if((string[pos] == ' ') || (string[pos] == '-')) {
			x+=wx;
			wx = 0;
			lastpos = pos;
			endword = true;
		}
	}
}

void * Font::GetPalette()
{
	return palette;
}
