#include "../../includes/win32def.h"
#include "Font.h"
#include "Interface.h"

extern Interface * core;

Font::Font(void)
{
	maxHeight = 0;
	count = 0;
	palette = NULL;
}

Font::~Font(void)
{
	free(palette);
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

void Font::AddChar(Sprite2D * spr)
{
	if(count == 0) {
		palette = core->GetVideoDriver()->GetPalette(spr);
	}
	if(maxHeight < spr->YPos)
		maxHeight = spr->YPos;
	//chars.push_back(spr);	
	chars[count++] = spr;
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
	for(int i = 0; i < 255; i++) {
		core->GetVideoDriver()->SetPalette(chars[i], pal);
	}
	Video * video = core->GetVideoDriver();
	int len = strlen((char*)string);
	char * tmp = (char*)malloc(len+1);
	strcpy(tmp, (char*)string);
	SetupString(tmp, rgn.w);
	int ystep = 0;
	if(Alignment & IE_FONT_SINGLE_LINE) {
		for(int i = 0; i < len; i++) {
			if(tmp[i] != 0)
				if(ystep < chars[(unsigned char)tmp[i]-1]->YPos)
					ystep = chars[(unsigned char)tmp[i]-1]->YPos;
		}
	}
	else
		ystep = chars[1]->Height;
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
		Sprite2D * spr = chars[(unsigned char)tmp[i]-1];
		x+=spr->XPos;
		video->BlitSprite(spr, x+rgn.x, y+rgn.y, true, &rgn);
		if(cursor &&  (curpos == i))
			video->BlitSprite(cursor, x-spr->XPos+rgn.x, y+rgn.y, true, &rgn);
		x+=spr->Width-spr->XPos;
		
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
	for(int i = 0; i < 255; i++) {
		core->GetVideoDriver()->SetPalette(chars[i], pal);
	}
	Video * video = core->GetVideoDriver();
	int len = strlen((char*)string);
	char * tmp = (char*)malloc(len+1);
	strcpy(tmp, (char*)string);
	SetupString(tmp, rgn.w);
	int ystep = 0;
	if(Alignment & IE_FONT_SINGLE_LINE) {
		for(int i = 0; i < len; i++) {
			if(tmp[i] != 0)
				if(ystep < chars[(unsigned char)tmp[i]-1]->YPos)
					ystep = chars[(unsigned char)tmp[i]-1]->YPos;
		}
	}
	else
		ystep = chars[1]->Height;
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
		Sprite2D * spr = chars[(unsigned char)tmp[i]-1];
		x+=spr->XPos;
		video->BlitSprite(spr, x+rgn.x, y+rgn.y, true, &rgn);
		if(cursor &&  (curpos == i))
			video->BlitSprite(cursor, x-spr->XPos+rgn.x, y+rgn.y, true, &rgn);
		x+=spr->Width-spr->XPos;
		
	}
	free(tmp);
}
/** PreCalculate for Printing */
StringList Font::Prepare(Region &rgn, unsigned char * string, Font * init, int curpos)
{
	if(init == NULL)
		init = this;
	StringList sl;
	sl.StringCount = 1;
	sl.starty = 0;
	int len = strlen((char*)string)+1;
	if(len == 1)
		len = 1;
	int nslen = len+1, lastnsi = 0, maxHeight = 0, crowlen = 1;
	//Allocate our char pointers list
	Sprite2D ** newstring = (Sprite2D**)malloc(nslen*sizeof(Sprite2D*));
	//Allocate the first string in the array
	sl.strings = (Sprite2D***)malloc(sl.StringCount*sizeof(Sprite2D**));
	/* No Need To Allocate the Row
	//Allocate the first string size to 1
	sl.strings[0] = (Sprite2D**)malloc(sizeof(Sprite2D*));
	*/
	//Allocate the heights array
	sl.heights = (unsigned int*)malloc(sizeof(unsigned int));
	//Allocate the lengths array
	sl.lengths = (unsigned int*)malloc(sizeof(unsigned int));
	int x, nsi = 0, lastx = 0;
	Sprite2D * nextimg, *oldimg = NULL;
	if(string[0] != 0)
		nextimg = init->chars[string[0]-1];
	else
		nextimg = init->chars['A'-1];
	x = nextimg->XPos;
	bool newline = true, curset = false;
	if(len != 1) { //Skip if we have an empty string
	for(int i = 0; i < len; i++) {
		if(i == curpos) {
			sl.curx = nsi;
			sl.cury = sl.StringCount-1;
			curset = true;
		}
		if(x >= rgn.w) {
			//Check for spaces at the beginning of the row
			while(string[i] == ' ') {
				i++;
				if(i == curpos) {
					if(!curset) {
						sl.curx = nsi;
						sl.cury = sl.StringCount-1;
					}
				}
			}
			nextimg = chars[string[i]-1];
			//New Line
			x=nextimg->XPos;
			//Add one slot to our newstring array sice we will insert a NULL value
			//at the end of the current row
			newstring = (Sprite2D**)realloc(newstring, (++nslen)*sizeof(Sprite2D*));
			//Allocate one extra row in the strings array
			sl.strings = (Sprite2D***)realloc(sl.strings, (sl.StringCount+1)*sizeof(Sprite2D**));
			/* No Need To Allocate The Row
			//Allocate our new Row size to 1
			sl.strings[sl.StringCount] = (Sprite2D**)malloc(sizeof(Sprite2D*));
			*/
			//Allocate one extra space in the heights array
			sl.heights = (unsigned int*)realloc(sl.heights, (sl.StringCount+1)*sizeof(unsigned int));
			//Allocate one extra space in the lengths array
			sl.lengths = (unsigned int*)realloc(sl.lengths, (sl.StringCount+1)*sizeof(unsigned int));
			//Let's add the '\0' at the end of the current row
			newstring[nsi] = NULL;
			//Set the pointer of our new row to the lastnsi index
			sl.strings[sl.StringCount-1] = &newstring[lastnsi];
			//Increment the nsi index so the new row will start after the '\0'
			nsi++;
			//Now the new row will start at nsi index so copy it in the lastnsi index
            lastnsi = nsi;
			//Now copy the maxHeight of the last row to the heights array
			sl.heights[sl.StringCount-1] = chars[1]->Height;
			//Set our string length for centering text
			sl.lengths[sl.StringCount-1] = lastx;
			//Increment the sl.StringCount variable since we have one new Row
			sl.StringCount++;
			//Reset our maxHeight variable
			maxHeight = 0;
			//Notify New Line
			//newline = true;
		}
		//Check for new maxHeight
		if(string[i] != 0x20) {
			if(maxHeight < nextimg->YPos)
				maxHeight = nextimg->YPos;
		}
		else {
			if(maxHeight < chars['A'-1]->YPos)
				maxHeight = chars['A'-1]->YPos;
		}
		//Check for end of String
		if(string[i+1] == 0)
			break;
		//Ok, we may continue
		//Let's move our pointer to the next char Position
		oldimg = nextimg;
		if(newline)
			nextimg = init->chars[string[i+1]-1];
		else
			nextimg = chars[string[i+1]-1];
		lastx = x;
		x+=(oldimg->Width-oldimg->XPos)+(nextimg->XPos);
		//Check if we have a New Line Character
		if(string[i+1] == '\n')
			x = rgn.w; //This way next time we make the cycle, we will insert a new line
		//Set our string char value
		newstring[nsi] = oldimg;
		//Increment the string pointer
		nsi++;
		//Notify continuing this line
		newline = false;
	}
	newstring[nsi] = nextimg;
	if(!curset) {
		sl.curx = nsi+1;
		sl.cury = sl.StringCount-1;
	}
	newstring[++nsi] = NULL;
	}
	else { //We have an empty String
		maxHeight = chars[1]->Height;//nextimg->YPos; //Use the maxHeight precalculated parameter
		newstring[nsi++] = nextimg;  //Add an image placeholder using the 'space' character
		newstring[nsi] = NULL; //End the String
		if(!curset) {
			sl.curx = 0;
			sl.cury = sl.StringCount-1;
		}
	}
	sl.strings[sl.StringCount-1] = &newstring[lastnsi];
	if(sl.StringCount == 1)
		sl.heights[sl.StringCount-1] = maxHeight;
	else
		sl.heights[sl.StringCount-1] = chars[1]->Height;
	sl.lengths[sl.StringCount-1] = x;
	int yacc = 0;
	for(int i = -1; i < sl.StringCount-1; i++) {
		if(yacc+sl.heights[i+1] > rgn.h) {
			break;
		}
		yacc+=sl.heights[i+1];
	}
	sl.starty = (rgn.h-yacc)/2;
	return sl;
}

int Font::CalcStringWidth(const char * string)
{
	int ret = 0, len = strlen(string);
	for(int i = 0; i < len; i++) {
		ret += chars[(unsigned char)string[i]-1]->Width;
	}
	return ret;
}

void Font::SetupString(char * string, int width)
{
	int len = strlen(string);
	int lastpos = 0;
	int x = 0, wx = 0;
	for(int pos = 0; pos < len; pos++) {
		if(x+wx > width) {
			string[lastpos] = 0;
			x = 0;
		}
		if(string[pos] == '\n') {
			string[pos++] = 0;
			x = 0;
			wx = 0;
			lastpos = pos;
		}
		wx += chars[((unsigned char)string[pos])-1]->Width;
		if((string[pos] == ' ') || (string[pos] == '-')) {
			x+=wx;
			wx = 0;
			lastpos = pos;
		}
	}
	/*char * s = strtok(str, " \n");
	int pos = -1;
	int x = 0;//chars[s[0]-1]->XPos;
	bool forceNewLine = false;
	while(s != NULL) {
		int ln = strlen(s);
		int nx = CalcStringWidth(s);
		if(string[pos+ln+1] == '\n')
			forceNewLine = true;
		if((len > pos+ln+1) && (string[pos+ln+1] != '\n'))
			nx+=chars[' '-1]->Width;
		if(x+nx >= width) {
			string[pos] = 0;		
			x = 0;//chars[s[0]-1]->XPos;
		}
		x+=nx;	
		pos += ln+1;
		if(forceNewLine) {
			string[pos] = 0;
			x = 0;
			forceNewLine = false;
		}
		s = strtok(NULL, " \n");
	}*/
}

void * Font::GetPalette()
{
	return palette;
}
