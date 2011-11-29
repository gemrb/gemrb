/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

//This class represents game fonts. Fonts are special .bam files.
//Each cycle stands for a letter. 

#include "Font.h"

#include "win32def.h"

#include "GameData.h"
#include "Interface.h"
#include "Palette.h"
#include "Sprite2D.h"
#include "Video.h"

#include <cassert>

unsigned int lastX = 0;

#define PARAGRAPH_START_X 5;

static const Color black = {0, 0, 0, 0};

inline size_t mystrlen(const char* string)
{
	if (!string) {
		return ( size_t ) 0;
	}
	const char* tmp = string;
	size_t count = 0;
	while (*tmp != 0) {
		if (( ( unsigned char ) * tmp ) >= 0xf0) {
			tmp += 3;
			count += 3;
		}
		count++;
		tmp++;
	}
	return count;
}

Font::Font(int w, int h, Palette* pal)
	: glyphCount(256), glyphInfo(glyphCount)
{
	palette = NULL;
	lastX = 0;
	count = 0;
	FirstChar = 0;
	sprBuffer = 0;

	SetPalette(pal);

	width = w;
	height = h;
	tmpPixels = (unsigned char*)malloc(width*height);

	maxHeight = h;
}

Font::~Font(void)
{
	Video *video = core->GetVideoDriver();
	video->FreeSprite( sprBuffer );
}

/*
 Return a region specefying the size of character 'chr'
 if 'chr' is not in the font then return empty region.
 */
const Font::GlyphInfo &Font::getInfo(ieWord chr) const
{
	return glyphInfo[chr - 1];
}

void Font::FinalizeSprite(bool cK, int index)
{
	sprBuffer = core->GetVideoDriver()->CreateSprite8( width, height, 8, tmpPixels, palette ? palette->col : 0, cK, index );
	tmpPixels = 0;
}

void Font::AddChar(unsigned char* spr, int w, int h, short xPos, short yPos)
{
	if (!spr) {
		glyphInfo[count].size.x = 0;
		glyphInfo[count].size.y = 0;
		glyphInfo[count].size.w = 0;
		glyphInfo[count].size.h = 0;
		glyphInfo[count].xPos = 0;
		glyphInfo[count].yPos = 0;
		count++;
		return;
	}
	unsigned char * currPtr = tmpPixels + lastX;
	unsigned char * srcPtr = ( unsigned char * ) spr;
	for (int y = 0; y < h; y++) {
		memcpy( currPtr, srcPtr, w );
		srcPtr += w;
		currPtr += width;
	}
	glyphInfo[count].size.x = lastX;
	glyphInfo[count].size.y = 0;
	glyphInfo[count].size.w = w;
	glyphInfo[count].size.h = h;
	glyphInfo[count].xPos = xPos;
	glyphInfo[count].yPos = yPos;
	count++;
	lastX += w;
}

void Font::PrintFromLine(int startrow, Region rgn, const unsigned char* string,
	Palette* hicolor, unsigned char Alignment, Font* initials,
	Sprite2D* cursor, unsigned int curpos, bool NoColor) const
{
	bool enablecap=false;
	int capital = 0;
	int initials_rows = 0;
	int last_initial_row = 0;
	int initials_x = 0;
	int initials_row = 0;
	unsigned char currCap = 0;
	size_t len = strlen( ( char* ) string );
	int num_empty_rows = 0;

	if (initials)
	{
		capital=1;
		enablecap=true;
		initials_rows = ((initials->maxHeight - 1) / maxHeight) - (startrow - 1);

		currCap = string[0] - 1;
		if ((startrow > 0 && initials_rows > 0) || (len > 0 && isspace(currCap))) { // we need to look back to get the cap
			while(isspace(currCap) && num_empty_rows < (int)len){//we cant cap whitespace so keep looking
				currCap = string[++num_empty_rows] - 1;
				// WARNING: this assumes all preceeding whitespace is an empty line
			}
			last_initial_row = (startrow - 1);
			initials_rows--;
		}
	}

	unsigned int psx = PARAGRAPH_START_X;
	Palette *pal = hicolor;
	if (!pal) {
		pal = palette;
	}

	if (initials==this) {
		enablecap=false;
	}

	sprBuffer->SetPalette( pal );

	char* tmp = ( char* ) malloc( len + 1 );
	memcpy( tmp, ( char * ) string, len + 1 );
	SetupString( tmp, rgn.w, NoColor, initials, enablecap );

	if (startrow) enablecap=false;
	int ystep = 0;
	if (Alignment & IE_FONT_SINGLE_LINE) {
		for (size_t i = 0; i < len; i++) {
			int height = getInfo(tmp[i]).yPos;
			if (ystep < height)
				ystep = height;
		}
	} else {
		ystep = getInfo(1).size.h;
	}
	if (!ystep) ystep = maxHeight;
	int x = psx, y = ystep;
	int w = CalcStringWidth( tmp, NoColor );
	if (Alignment & IE_FONT_ALIGN_CENTER) {
		x = ( rgn.w - w) / 2;
	} else if (Alignment & IE_FONT_ALIGN_RIGHT) {
		x = ( rgn.w - w );
	}
	if (Alignment & IE_FONT_ALIGN_MIDDLE) {
		int h = 0;
		for (size_t i = 0; i <= len; i++) {
			if (( tmp[i] == 0 ) || ( tmp[i] == '\n' ))
				h++;
		}
		h = h * ystep;
		y += ( rgn.h - h ) / 2;
	} else if (Alignment & IE_FONT_ALIGN_BOTTOM) {
		int h = 1;
		for (size_t i = 0; i <= len; i++) {
			if (( tmp[i] == 0 ) || ( tmp[i] == '\n' ))
				h++;
		}
		h = h * ystep;
		y += ( rgn.h - h );
	} else if (Alignment & IE_FONT_ALIGN_TOP) {
		y += 5;
	}

	Video* video = core->GetVideoDriver();
	int row = 0;
	for (size_t i = 0; i < len; i++) {
		if (( ( unsigned char ) tmp[i] ) == '[' && !NoColor) {
			i++;
			char tag[256];
			tag[0]=0;

			for (int k = 0; k < 256 && i<len; k++) {
				if (tmp[i] == ']') {
					tag[k] = 0;
					break;
				}
				tag[k] = tmp[i++];
			}

			if (strnicmp( tag, "capital=",8)==0) {
				sscanf( tag, "capital=%d", &capital);
				if (capital && (row>=startrow) ) {
					enablecap=true;
				}
				continue;
			}

			if (strnicmp( tag, "color=", 6 ) == 0) {
				unsigned int r,g,b;
				if (sscanf( tag, "color=%02X%02X%02X", &r, &g, &b ) != 3)
					continue;
				const Color c = {(unsigned char) r,(unsigned char)g, (unsigned char)b, 0};
				Palette* newPal = core->CreatePalette( c, palette->back );
				sprBuffer->SetPalette( newPal );
				gamedata->FreePalette( newPal );
				continue;
			}
			if (stricmp( tag, "/color" ) == 0) {
				sprBuffer->SetPalette( pal );
				continue;
			}
			if (stricmp( "p", tag ) == 0) {
				psx = x;
				continue;
			}
			if (stricmp( "/p", tag ) == 0) {
				psx = PARAGRAPH_START_X;
			}
			continue;
		}

		if (row < startrow) {
			if (tmp[i] == 0) {
				row++;
			}
			continue;
		}
		if (( tmp[i] == 0 ) || ( tmp[i] == '\n' )) {
			y += ystep;
			x = psx;
			int w = CalcStringWidth( &tmp[i + 1], NoColor );
			if (initials_rows > 0) {
				initials_rows--;
				x += initials_x;
				w += initials_x;
			}
			if (Alignment & IE_FONT_ALIGN_CENTER) {
				x = ( rgn.w - w ) / 2;
			} else if (Alignment & IE_FONT_ALIGN_RIGHT) {
				x = ( rgn.w - w );
			}
			continue;
		}
		unsigned char currChar = tmp[i];
		if (initials && capital && enablecap) {
			currCap = currChar;
			x = initials->PrintInitial( x, y, rgn, currChar );
			initials_x = x;

			//how many more lines to be indented (one was already indented)
			initials_rows = (initials->maxHeight-1)/maxHeight;
			initials_row = row;
			last_initial_row = initials_row;

			enablecap = false;
			continue;
		}else if(initials && currCap && row > last_initial_row && (row - num_empty_rows - initials_row) <= ((initials->maxHeight-1)/maxHeight)){
			// means this row doesnt have a cap, but a preceeding one did and its overlapping this row
			int initY = y;
			if (!num_empty_rows) {// num_empty_rows is for scrolling text areas
				initY = (y - (maxHeight * (row - initials_row)));
			}
			x = initials->PrintInitial( x, initY, rgn, currCap );
			initials_x = x;
			x += psx;
			last_initial_row++;
		}
		video->BlitSpriteRegion( sprBuffer, getInfo(currChar).size,
			x + rgn.x, y + rgn.y - getInfo(currChar).xPos, true, &rgn );
		if (cursor && ( i == curpos )) {
			video->BlitSprite( cursor, x + rgn.x,
				y + rgn.y, true, &rgn );
		}
		x += getInfo(currChar).size.w;
	}
	if (cursor && ( curpos == len )) {
		video->BlitSprite( cursor, x + rgn.x,
			y + rgn.y, true, &rgn );
	}
	free( tmp );
}

void Font::Print(Region rgn, const unsigned char* string, Palette* hicolor,
	unsigned char Alignment, bool anchor, Font* initials,
	Sprite2D* cursor, unsigned int curpos, bool NoColor) const
{
	Print(rgn, rgn, string, hicolor, Alignment, anchor, initials, cursor, curpos, NoColor);
}

void Font::Print(Region cliprgn, Region rgn, const unsigned char* string,
	Palette* hicolor, unsigned char Alignment, bool anchor, Font* initials,
	Sprite2D* cursor, unsigned int curpos, bool NoColor) const
{
	bool enablecap=false;
	int capital = 0;
	if (initials)
	{
		capital=1;
		enablecap=true;
	}
	(void)enablecap; //HACK: shut up unused-but-set warnings, until the var is reused

	unsigned int psx = PARAGRAPH_START_X;
	Palette* pal = hicolor;
	if (!pal) {
		pal = palette;
	}
	if (initials==this) {
		initials = NULL;
	}

	sprBuffer->SetPalette( pal );
	size_t len = strlen( ( char* ) string );
	char* tmp = ( char* ) malloc( len + 1 );
	memcpy( tmp, ( char * ) string, len + 1 );
	while (len > 0 && (tmp[len - 1] == '\n' || tmp[len - 1] == '\r')) {
		// ignore trailing newlines
		tmp[len - 1] = 0;
		len--;
	}

	SetupString( tmp, rgn.w, NoColor, initials, capital );
	int ystep = 0;
	if (Alignment & IE_FONT_SINGLE_LINE) {
		
		for (size_t i = 0; i < len; i++) {
			if (tmp[i] == 0) continue;
			int height = getInfo(tmp[i]-1).yPos;
			if (ystep < height)
				ystep = height;
		}
	} else {
		ystep = getInfo(1).size.h;
	}
	if (!ystep) ystep = maxHeight;
	int x = psx, y = ystep;
	Video* video = core->GetVideoDriver();

	if (Alignment & IE_FONT_ALIGN_CENTER) {
		int w = CalcStringWidth( tmp, NoColor );
		x = ( rgn.w - w ) / 2;
	} else if (Alignment & IE_FONT_ALIGN_RIGHT) {
		int w = CalcStringWidth( tmp, NoColor );
		x = ( rgn.w - w );
	}

	if (Alignment & IE_FONT_ALIGN_MIDDLE) {
		int h = 0;
		for (size_t i = 0; i <= len; i++) {
			if (tmp[i] == 0)
				h++;
		}
		h = h * ystep;
		y += ( rgn.h - h ) / 2;
	} else if (Alignment & IE_FONT_ALIGN_BOTTOM) {
		int h = 1;
		for (size_t i = 0; i <= len; i++) {
			if (tmp[i] == 0)
				h++;
		}
		h = h * ystep;
		y += ( rgn.h - h );
	} else if (Alignment & IE_FONT_ALIGN_TOP) {
		y += 5;
	}
	for (size_t i = 0; i < len; i++) {
		if (( ( unsigned char ) tmp[i] ) == '[' && !NoColor) {
			i++;
			char tag[256];
			tag[0]=0;
			for (int k = 0; k < 256 && i<len; k++) {
				if (tmp[i] == ']') {
					tag[k] = 0;
					break;
				}
				tag[k] = tmp[i++];
			}

			if (strnicmp( tag, "capital=",8)==0) {
				sscanf( tag, "capital=%d", &capital);
				if (capital) {
					enablecap=true;
				}
				continue;
			}
			
			if (strnicmp( tag, "color=", 6 ) == 0) {
				unsigned int r,g,b;
				if (sscanf( tag, "color=%02X%02X%02X", &r, &g, &b ) != 3)
					continue;
				const Color c = {(unsigned char) r,(unsigned char) g,(unsigned char)  b, 0};
				Palette* newPal = core->CreatePalette( c, palette->back );
				sprBuffer->SetPalette( newPal );
				gamedata->FreePalette( newPal );
				continue;
			}
			if (stricmp( tag, "/color" ) == 0) {
				sprBuffer->SetPalette( pal );
				continue;
			}
			if (stricmp( "p", tag ) == 0) {
				psx = x;
				continue;
			}
			if (stricmp( "/p", tag ) == 0) {
				psx = PARAGRAPH_START_X;
				continue;
			}
			continue;
		}

		if (tmp[i] == 0) {
			y += ystep;
			x = psx;
			int w = CalcStringWidth( &tmp[i + 1], NoColor );
			if (Alignment & IE_FONT_ALIGN_CENTER) {
				x = ( rgn.w - w ) / 2;
			} else if (Alignment & IE_FONT_ALIGN_RIGHT) {
				x = ( rgn.w - w );
			}
			continue;
		}
		unsigned char currChar = tmp[i];
		if (initials && capital) {
			x = initials->PrintInitial( x, y, rgn, currChar );
			enablecap=false;
			continue;
		}
		video->BlitSpriteRegion( sprBuffer, getInfo(currChar).size,
			x + rgn.x, y + rgn.y - getInfo(currChar).yPos,
			anchor, &cliprgn );
		if (cursor && ( curpos == i ))
			video->BlitSprite( cursor, x + rgn.x, y + rgn.y, anchor, &cliprgn );
		x += getInfo(currChar).size.w;
	}
	if (cursor && ( curpos == len )) {
		video->BlitSprite( cursor, x + rgn.x, y + rgn.y, anchor, &cliprgn );
	}
	free( tmp );
}

int Font::PrintInitial(int x, int y, const Region &rgn, unsigned char currChar) const
{
	Video *video = core->GetVideoDriver();
	video->BlitSpriteRegion( sprBuffer, getInfo(currChar).size,
		x + rgn.x, y + rgn.y - getInfo(currChar).yPos, true, &rgn );
	x += getInfo(currChar).size.w;
	return x;
}

int Font::CalcStringWidth(const char* string, bool NoColor) const
{
	size_t ret = 0, len = strlen( string );
	for (size_t i = 0; i < len; i++) {
		if (( ( unsigned char ) string[i] ) == '[' && !NoColor) {
			while(i<len && ((unsigned char) string[i]) != ']') {
				i++;
			}
		}
		ret += getInfo(string[i]).size.w;
	}
	return ( int ) ret;
}

void Font::SetupString(char* string, unsigned int width, bool NoColor, Font *initials, bool enablecap) const
{
	size_t len = strlen( string );
	unsigned int psx = PARAGRAPH_START_X;
	int lastpos = 0;
	unsigned int x = psx, wx = 0;
	bool endword = false;
	int initials_rows = 0;
	int initials_x = 0;
	for (size_t pos = 0; pos < len; pos++) {
		if (x + wx > width) {
			// we wrapped, force a new line somewhere
			if (!endword && ( x == psx ))
				lastpos = ( int ) pos;
			else
				string[lastpos] = 0;
			x = psx;
			if (initials_rows > 0) {
				initials_rows--;
				x += initials_x;
			}
		}
		if (string[pos] == 0) {
			continue;
		}
		endword = false;
		if (string[pos] == '\r')
			string[pos] = ' ';
		if (string[pos] == '\n') {
			// force a new line here
			string[pos] = 0;
			x = psx;
			wx = 0;
			if (initials_rows > 0) {
				initials_rows--;
				x += initials_x;
			}
			lastpos = ( int ) pos;
			endword = true;
			continue;
		}
		if (( ( unsigned char ) string[pos] ) == '[' && !NoColor) {
			pos++;
			if (pos>=len)
				break;
			char tag[256];
			int k = 0;
			for (k = 0; k < 256; k++) {
				if (string[pos] == ']') {
					tag[k] = 0;
					break;
				}
				tag[k] = string[pos++];
			}
			if (strnicmp( tag, "capital=",8)==0) {
				int capital = 0;
				sscanf( tag, "capital=%d", &capital);
				if (capital) {
					enablecap=true;
				}
				continue;
			}
			if (stricmp( "p", tag ) == 0) {
				psx = x;
				continue;
			}
			if (stricmp( "/p", tag ) == 0) {
				psx = PARAGRAPH_START_X;
				continue;
			}
			continue;
		}

		if (string[pos] && string[pos] != ' ') {
			string[pos] = ( unsigned char ) (string[pos] - FirstChar);
		}

		wx += getInfo(string[pos]).size.w;
		if (initials && enablecap) {
			wx += initials->getInfo(string[pos]).size.w;
			enablecap=false;
			initials_x = wx;
			//how many more lines to be indented (one was already indented)
			initials_rows = (initials->maxHeight-1)/maxHeight;
			continue;
		}
		if (( string[pos] == ' ' ) || ( string[pos] == '-' )) {
			x += wx;
			wx = 0;
			lastpos = ( int ) pos;
			endword = true;
		}
	}
}

Palette* Font::GetPalette() const
{
	assert(palette);
	palette->IncRef();
	return palette;
}

void Font::SetPalette(Palette* pal)
{
	if (pal) pal->IncRef();
	if (palette) palette->Release();
	palette = pal;
}

void Font::SetFirstChar( unsigned char first)
{
	FirstChar = first;
}
