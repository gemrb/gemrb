#include "win32def.h"
#include "Font.h"
#include "Interface.h"

extern Interface * core;

Font::Font(void)
{
	maxHeight = 0;
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
	chars.push_back(spr);	
}

bool written = false;

void Font::Print(Region rgn, unsigned char * string, Color *color, unsigned char Alignment, bool anchor)
{
	//TODO: Implement Colored Text
	Color * pal = NULL;
	if(color != NULL) {
		pal = core->GetVideoDriver()->CreatePalette(*color);
	}
	Video * video = core->GetVideoDriver();
	StringList sl = Prepare(rgn, string);
	int x = 0;
	int y = rgn.y+sl.starty;
	switch(Alignment) {
		case IE_FONT_ALIGN_LEFT: 
		{
			for(int r = 0; r < sl.StringCount; r++) {
				x = rgn.x+sl.strings[0][0]->XPos;
				y += sl.heights[0];
				int	i = 0;
				while(true) {
					if(pal != NULL)
						video->SetPalette(sl.strings[r][i], pal);
					video->BlitSprite(sl.strings[r][i], x, y, anchor);
					if(sl.strings[r][i+1] == NULL)
						break;
					x+=(sl.strings[r][i]->Width-sl.strings[r][i]->XPos)+sl.strings[r][i+1]->XPos;
					i++;
				}
			}
		}
		break;

		case IE_FONT_ALIGN_CENTER:
		{
			for(int r = 0; r < sl.StringCount; r++) {
				x = rgn.x+sl.strings[r][0]->XPos+((rgn.w/2)-(sl.lengths[r]/2));
				y += sl.heights[0];
				int	i = 0;
				while(true) {
					if(pal != NULL)
						video->SetPalette(sl.strings[r][i], pal);
					video->BlitSprite(sl.strings[r][i], x, y, anchor);
					if(sl.strings[r][i+1] == NULL)
						break;
					x+=(sl.strings[r][i]->Width-sl.strings[r][i]->XPos)+sl.strings[r][i+1]->XPos;
					i++;
				}
			}
		}
		break;

		case IE_FONT_ALIGN_RIGHT:
		{
			for(int r = 0; r < sl.StringCount; r++) {
				int len = 0;
				while(sl.strings[r][len] != NULL)
					len++;
				len--;
				x = rgn.x+rgn.w-(sl.strings[0][len]->Width-sl.strings[0][len]->XPos);
				y += sl.heights[0];
				int	i = len;
				while(i >= 0) {
					if(pal != NULL)
						video->SetPalette(sl.strings[r][i], pal);
					video->BlitSprite(sl.strings[r][i], x, y, anchor);
					if(i == 0)
						break;
					x-=sl.strings[r][i]->XPos+(sl.strings[r][i-1]->Width-sl.strings[r][i-1]->XPos);
					i--;
				}
			}
		}
		break;
	}
	if(pal)
		free(pal);
	free(sl.strings[0]);
	free(sl.strings);
	free(sl.heights);
	free(sl.lengths);
}
/** PreCalculate for Printing */
StringList Font::Prepare(Region &rgn, unsigned char * string)
{	
	StringList sl;
	sl.StringCount = 1;
	sl.starty = 0;
	int len = strlen((char*)string)+1;
	int nslen = len, lastnsi = 0, maxHeight = 0, crowlen = 1;
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
	Sprite2D * nextimg = chars[string[0]-1], *oldimg = NULL;
	x = nextimg->XPos;
	for(int i = 0; i < len; i++) {
		if(x >= rgn.w) {
			//Check for spaces at the beginning of the row
			while(string[i] == ' ') {
				i++;
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
			sl.heights[sl.StringCount-1] = maxHeight;
			//Set our string length for centering text
			sl.lengths[sl.StringCount-1] = lastx;
			//Increment the sl.StringCount variable since we have one new Row
			sl.StringCount++;
			//Reset our maxHeight variable
			maxHeight = 0;
		}
		//Check for new maxHeight
		if(maxHeight < nextimg->YPos)
			maxHeight = nextimg->YPos;
		//Check for end of String
		if(string[i+1] == 0)
			break;
		//Ok, we may continue
		//Let's move our pointer to the next char Position
		oldimg = nextimg;
		nextimg = chars[string[i+1]-1];
		lastx = x;
		x+=(oldimg->Width-oldimg->XPos)+(nextimg->XPos);
		//Set our string char value
		newstring[nsi] = oldimg;
		//Increment the string pointer
		nsi++;
	}
	newstring[nsi++] = nextimg;
	newstring[nsi] = NULL;
	sl.strings[sl.StringCount-1] = &newstring[lastnsi];
	sl.heights[sl.StringCount-1] = maxHeight;
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