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

namespace GemRB {

#define PARAGRAPH_START_X 5;

#define SET_BLIT_PALETTE( palette )\
if (palette) ((Palette*)palette)->IncRef();\
if (blitPalette) blitPalette->Release();\
blitPalette = palette;

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

/*
glyphs should be all characters we are interested in printing with the font save whitespace
Font takes responsibility for glyphs so we must free them once done
*/
Font::Font(Sprite2D* glyphs[], ieWord firstChar, ieWord lastChar, Palette* pal)
	: glyphCount(lastChar - firstChar + 1), glyphs(glyphs), FirstChar(firstChar), LastChar(lastChar)
{
	assert(glyphs);
	assert(pal);
	assert(firstChar <= lastChar);

	palette = NULL;
	resRefs = NULL;
	numResRefs = 0;
	maxHeight = 0;

	ptSize = 0;
	name[0] = '\0';
	style = NORMAL;

	SetPalette(pal);

	whiteSpace[BLANK] = core->GetVideoDriver()->CreateSprite8(0, 0, 8, NULL, palette->col);

	for (int i = 0; i < glyphCount; i++)
	{
		if (glyphs[i] != NULL) {
			glyphs[i]->XPos = 0;
			glyphs[i]->SetPalette(palette);
			if (glyphs[i]->Height > maxHeight) maxHeight = glyphs[i]->Height;
		} else {
			// use our empty glyph for safety reasons
			whiteSpace[BLANK]->acquire();
			glyphs[i] = whiteSpace[BLANK];
		}
	}

	// standard space width is 1/4 ptSize
	whiteSpace[SPACE] = core->GetVideoDriver()->CreateSprite8((int)(maxHeight * 0.25), 0, 8, NULL, palette->col);
	// standard tab width is 4 spaces???
	whiteSpace[TAB] = core->GetVideoDriver()->CreateSprite8((whiteSpace[1]->Width * 4), 0, 8, NULL, palette->col);
}

Font::~Font(void)
{
	whiteSpace[BLANK]->release();
	whiteSpace[SPACE]->release();
	whiteSpace[TAB]->release();

	for (int i = 0; i < glyphCount; i++)
	{
		core->GetVideoDriver()->FreeSprite(glyphs[i]);
	}
	SetPalette(NULL);
	free(glyphs);
	free(resRefs);
}

/*
 Return a region specefying the size of character 'chr'
 if 'chr' is not in the font then return empty region.
 */
const Sprite2D* Font::GetCharSprite(ieWord chr) const
{
	if (chr >= FirstChar && chr <= LastChar) {
		return glyphs[chr - FirstChar];
	}
	if (chr == ' ') return whiteSpace[SPACE];
	if (chr == '\t') return  whiteSpace[TAB];
	//otherwise return an empty sprite
	return whiteSpace[BLANK];
}

bool Font::AddResRef(const ieResRef resref)
{
	if (resref) {
		resRefs = (ieResRef*)realloc(resRefs, sizeof(ieResRef) * ++numResRefs);
		strnlwrcpy( resRefs[numResRefs - 1], resref, sizeof(ieResRef)-1);
		return true;
	}
	return false;
}

bool Font::MatchesResRef(const ieResRef resref)
{
	for (int i=0; i < numResRefs; i++)
	{
		if (strnicmp( resref, resRefs[i], sizeof(ieResRef)-1) == 0){
			return true;
		}
	}
	return false;
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

	if (initials && initials != this)
	{
		capital=1;
		enablecap=true;
		//initials_rows = (int)ceil(initials->maxHeight / maxHeight);
		initials_rows = 1 + ((initials->maxHeight - 1) / maxHeight); // ceiling
		currCap = string[0];
		if ((startrow > 0 && initials_rows > 0) || (len > 0 && isspace(currCap))) { // we need to look back to get the cap
			while(isspace(currCap) && num_empty_rows < (int)len){//we cant cap whiteSpace so keep looking
				currCap = string[++num_empty_rows];
				// WARNING: this assumes all preceeding whiteSpace is an empty line
			}
			last_initial_row = startrow - 1; // always the row before current since this cannot be the first row
			initials_rows = initials_rows - (startrow + 1) + num_empty_rows; // startrow + 1 because start row is 0 based, but initials_rows is 1 based
		}
	}

	unsigned int psx = PARAGRAPH_START_X;
	Palette *pal = hicolor;
	if (!pal) {
		pal = palette;
	}

	Palette* blitPalette = NULL;
	SET_BLIT_PALETTE(pal);

	char* tmp = ( char* ) malloc( len + 1 );
	memcpy( tmp, ( char * ) string, len + 1 );
	SetupString( tmp, rgn.w, NoColor, initials, enablecap );

	if (startrow) enablecap=false;
	int ystep = 0;
	if (Alignment & IE_FONT_SINGLE_LINE) {
		for (size_t i = 0; i < len; i++) {
			int height = GetCharSprite((unsigned char)tmp[i])->Height;
			if (ystep < height)
				ystep = height;
		}
	} else {
		ystep = maxHeight;
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
	const Sprite2D* currGlyph;
	unsigned char currChar = '\0';
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
				SET_BLIT_PALETTE(newPal);
				gamedata->FreePalette( newPal );
				continue;
			}
			if (stricmp( tag, "/color" ) == 0) {
				SET_BLIT_PALETTE(pal);
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
		currChar = tmp[i];
		if (initials && capital && enablecap) {
			currCap = currChar;
			x = initials->PrintInitial( x, y, rgn, currChar );
			initials_x = x;

			//how many more lines to be indented (one was already indented)
			initials_rows = (initials->maxHeight-1)/maxHeight;
			initials_rows += num_empty_rows;
			initials_row = row;
			last_initial_row = initials_row;

			enablecap = false;
			continue;
		} else if (initials && currCap
				   && row > last_initial_row
				   && (row - num_empty_rows - initials_row) <= ((initials->maxHeight-1)/maxHeight)){
			// means this row doesnt have a cap, but a preceeding one did and its overlapping this row
			int initY = y;
			if (!num_empty_rows || row > num_empty_rows) {// num_empty_rows is for scrolling text areas
				initY = (y - (maxHeight * (row - initials_row - num_empty_rows)));
			}
			x = initials->PrintInitial( x, initY, rgn, currCap );
			initials_x = x;
			last_initial_row++;
			if (num_empty_rows && row <= num_empty_rows) continue;
			else x += psx;
		}
		currGlyph = GetCharSprite(currChar);
		video->BlitSprite(currGlyph, x + rgn.x, y + rgn.y, true, &rgn, blitPalette);
		if (cursor && ( i == curpos )) {
			video->BlitSprite( cursor, x + rgn.x, y + rgn.y, true, &rgn );
		}
		x += currGlyph->Width;
	}
	if (cursor && ( curpos == len )) {
		video->BlitSprite( cursor, x + rgn.x, y + rgn.y, true, &rgn );
	}
	SET_BLIT_PALETTE(NULL);
	free( tmp );
}

void Font::Print(Region rgn, const unsigned char* string, Palette* hicolor,
	unsigned char Alignment, bool anchor, Font* initials,
	Sprite2D* cursor, unsigned int curpos, bool NoColor) const
{
	Region cliprgn = rgn;
	if (!anchor) {
		Region Viewport = core->GetVideoDriver()->GetViewport();
		cliprgn.x -= Viewport.x;
		cliprgn.y -= Viewport.y;
	}
	Print(cliprgn, rgn, string, hicolor, Alignment, anchor, initials, cursor, curpos, NoColor);
}

void Font::Print(Region cliprgn, Region rgn, const unsigned char* string,
	Palette* hicolor, unsigned char Alignment, bool anchor, Font* initials,
	Sprite2D* cursor, unsigned int curpos, bool NoColor) const
{
	int capital = (initials) ? 1 : 0;

	unsigned int psx = PARAGRAPH_START_X;
	Palette* pal = hicolor;
	if (!pal) {
		pal = palette;
	}
	if (initials==this) {
		initials = NULL;
	}

	Palette* blitPalette = NULL;
	SET_BLIT_PALETTE( pal );

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
			int height = GetCharSprite((unsigned char)tmp[i])->Height;
			if (ystep < height)
				ystep = height;
		}
	} else {
		ystep = maxHeight;
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

	unsigned char currChar = '\0';
	const Sprite2D* currGlyph = NULL;
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
				continue;
			}
			
			if (strnicmp( tag, "color=", 6 ) == 0) {
				unsigned int r,g,b;
				if (sscanf( tag, "color=%02X%02X%02X", &r, &g, &b ) != 3)
					continue;
				const Color c = {(unsigned char) r,(unsigned char) g,(unsigned char)  b, 0};
				Palette* newPal = core->CreatePalette( c, palette->back );
				SET_BLIT_PALETTE(newPal);
				gamedata->FreePalette( newPal );
				continue;
			}
			if (stricmp( tag, "/color" ) == 0) {
				SET_BLIT_PALETTE(pal);
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
		currChar = tmp[i];
		currGlyph = GetCharSprite(currChar);
		if (initials && capital) {
			x = initials->PrintInitial( x, y, rgn, currChar );
			continue;
		}

		video->BlitSprite(currGlyph, x + rgn.x, y + rgn.y, anchor, &cliprgn, blitPalette);

		if (cursor && ( curpos == i ))
			video->BlitSprite( cursor, x + rgn.x, y + rgn.y, anchor, &cliprgn );
		x += currGlyph->Width;
	}
	if (cursor && ( curpos == len )) {
		video->BlitSprite( cursor, x + rgn.x, y + rgn.y, anchor, &cliprgn );
	}
	SET_BLIT_PALETTE(NULL);
	free( tmp );
}

int Font::PrintInitial(int x, int y, const Region &rgn, unsigned char currChar) const
{
	const Sprite2D* glyph = GetCharSprite(currChar);
	core->GetVideoDriver()->BlitSprite(glyph, x + rgn.x, y + rgn.y, true, &rgn);

	x += glyph->Width;
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
		ret += GetCharSprite((unsigned char)string[i])->Width;
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
			string[pos] = ( unsigned char ) (string[pos]);
		}

		if (initials && enablecap) {
			wx += initials->GetCharSprite((unsigned char)string[pos])->Width;
			enablecap=false;
			initials_x = wx + psx;
			//how many more lines to be indented (one was already indented)
			initials_rows = (initials->maxHeight-1)/maxHeight;
			continue;
		} else {
			wx += GetCharSprite((unsigned char)string[pos])->Width;
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
#undef SET_BLIT_PALETTE
}
