#include "../../includes/win32def.h"
#include "Font.h"
#include "Interface.h"

extern Interface * core;

Font::Font(void)
{
	maxHeight = 0;
	count = 0;
}

Font::~Font(void)
{
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
	if(maxHeight < spr->YPos)
		maxHeight = spr->YPos;
	//chars.push_back(spr);	
	chars[count++] = spr;
}

bool written = false;

void Font::Print(Region rgn, unsigned char * string, Color *hicolor, unsigned char Alignment, bool anchor, Font * initials, Color *initcolor, Sprite2D * cursor, int curpos)
{
	//TODO: Implement Colored Text
	Color * pal = NULL, *ipal = NULL;
	/*if(hicolor != NULL) {
		pal = core->GetVideoDriver()->CreatePalette(*hicolor, *lowcolor);
	}
	if(initcolor != NULL) {
		if((initcolor->r == hicolor->r) && (initcolor->g == hicolor->g) && (initcolor->b == hicolor->b))
			ipal = pal;
		else
			ipal = core->GetVideoDriver()->CreatePalette(*initcolor, *lowcolor);
	}*/
	pal = hicolor;
	if(ipal != NULL) {
		ipal = initcolor;
	}
	else
		ipal = pal;
	if(pal) {
		for(int i = 0; i < 256; i++) {
			core->GetVideoDriver()->SetPalette(chars[i], pal);
		}
	}
	Video * video = core->GetVideoDriver();
	/*char * tmp = (char*)malloc(strlen((char*)string)+1);
	strcpy(tmp, (char*)string);
	char * str = strtok(tmp, " \n");
	int x = 0, y = chars[1]->Height;
	while(str != NULL) {
		int width = CalcStringWidth(str);
		if(x+width >= rgn.w) {
			y+=chars[1]->Height;
			x=0;
		}
		int len = strlen(str);
		for(int i = 0; i < len; i++) {
			Sprite2D * spr = chars[(unsigned char)str[i]-1];
			x+=spr->XPos;
			video->BlitSprite(spr, x+rgn.x, y+rgn.y, true, &rgn);
			x+=spr->Width-spr->XPos;
		}
		x+=chars[' '-1]->Width;
		str = strtok(NULL, " \n");
	}
	free(tmp);*/
	StringList sl = Prepare(rgn, string, initials, curpos);
	int x = 0, y = 0;
	if(Alignment & IE_FONT_ALIGN_TOP)
		y = rgn.y;
	else if(Alignment & IE_FONT_ALIGN_MIDDLE)
		y = rgn.y+sl.starty;
	switch(Alignment & 0x0F) {
		case IE_FONT_ALIGN_LEFT: 
		{
			bool cursorblt = false;
			int	i = 0;
			int r = 0;
			for(r = 0; r < sl.StringCount; r++) {
				x = rgn.x+sl.strings[r][0]->XPos;
				y += sl.heights[0];
				i = 0;
				while(true) {
					video->BlitSprite(sl.strings[r][i], x, y, anchor, &rgn);
					if((cursor != NULL) && (sl.cury == r) && (sl.curx == i)) {
						video->BlitSprite(cursor, x-sl.strings[r][i]->XPos, y, anchor);
						cursorblt = true;
					}
					if(sl.strings[r][i+1] == NULL)
						break;
					x+=(sl.strings[r][i]->Width-sl.strings[r][i]->XPos)+sl.strings[r][i+1]->XPos;
					i++;
				}
			}
			if(!cursorblt) {
				if(cursor) {
					if(r != 0) {
						r--;
						video->BlitSprite(cursor, x-sl.strings[r][i]->XPos+sl.strings[r][i]->Width, y, true);
					}
					else
						video->BlitSprite(cursor, x, y, true);
				}
			}
		}
		break;

		case IE_FONT_ALIGN_CENTER:
		{
			bool cursorblt = false;
			int	i = 0;
			int r = 0;
			for(r = 0; r < sl.StringCount; r++) {
				x = rgn.x+sl.strings[r][0]->XPos+((rgn.w/2)-(sl.lengths[r]/2));
				y += sl.heights[0];
				i = 0;
				while(true) {
					video->BlitSprite(sl.strings[r][i], x, y, anchor, &rgn);
					if((cursor != NULL) && (sl.cury == r) && (sl.curx == i))
						video->BlitSprite(cursor, x-sl.strings[r][i]->XPos, y, anchor);
					if(sl.strings[r][i+1] == NULL)
						break;
					x+=(sl.strings[r][i]->Width-sl.strings[r][i]->XPos)+sl.strings[r][i+1]->XPos;
					i++;
				}
			}
			if(!cursorblt) {
				if(cursor) {
					if(r != 0) {
						r--;
						video->BlitSprite(cursor, x-sl.strings[r][i]->XPos+sl.strings[r][i]->Width, y, true);
					}
					else
						video->BlitSprite(cursor, x, y, true);
				}
			}
		}
		break;

		case IE_FONT_ALIGN_RIGHT:
		{
			bool cursorblt = false;
			int	i = 0;
			int r = 0;
			for(r = 0; r < sl.StringCount; r++) {
				int len = 0;
				while(sl.strings[r][len] != NULL)
					len++;
				len--;
				x = rgn.x+rgn.w-(sl.strings[0][len]->Width-sl.strings[0][len]->XPos);
				y += sl.heights[0];
				i = len;
				while(i >= 0) {
					video->BlitSprite(sl.strings[r][i], x, y, anchor, &rgn);
					if((cursor != NULL) && (sl.cury == r) && (sl.curx == i))
						video->BlitSprite(cursor, x-sl.strings[r][i]->XPos, y, anchor);
					if(i == 0)
						break;
					x-=sl.strings[r][i]->XPos+(sl.strings[r][i-1]->Width-sl.strings[r][i-1]->XPos);
					i--;
				}
			}
			if(!cursorblt) {
				if(cursor) {
					if(r != 0) {
						r--;
						video->BlitSprite(cursor, x-sl.strings[r][i]->XPos, y, true);
					}
					else
						video->BlitSprite(cursor, x, y, true);
				}
			}
		}
		break;
	}
	free(sl.strings[0]);
	free(sl.strings);
	free(sl.heights);
	free(sl.lengths);
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
