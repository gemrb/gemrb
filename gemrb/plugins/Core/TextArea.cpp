/***************************************************************************
                          TextArea.cpp  -  description
                             -------------------
    begin                : dom ott 12 2003
    copyright            : (C) 2003 by GemRB Developement Team
    email                : Balrog994@yahoo.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "../../includes/win32def.h"
#include "TextArea.h"
#include "Interface.h"
#include <stdio.h>

extern Interface * core;

TextArea::TextArea(Color hitextcolor, Color initcolor, Color lowtextcolor)
{
	//BufferLength = 4096;
	//Buffer = (unsigned char*)malloc(BufferLength);
	//Buffer[0] = 0;
	rows = 0;
	startrow = 0;
	sb = NULL;
	palette = core->GetVideoDriver()->CreatePalette(hitextcolor, lowtextcolor);
	initpalette = core->GetVideoDriver()->CreatePalette(initcolor, lowtextcolor);
}

TextArea::~TextArea(void)
{
	free(palette);
	free(initpalette);
	for(int i = 0; i < lines.size(); i++) {
		free(lines[i]);
	}
}

void TextArea::Draw(unsigned short x, unsigned short y)
{
	if(!Changed)
		return;
	if(lines.size() == 0)
		return;
	char * Buffer = (char*)malloc(1);
	Buffer[0] = 0;
	int len = 0;
	for(int i = 0; i < lines.size(); i++) {
		len += strlen(lines[i]);
		Buffer = (char*)realloc(Buffer, len+1);
		strcat(Buffer, lines[i]);
		if(i != lines.size()-1)
			strcat(Buffer, "\n");
	}
	ftext->PrintFromLine(startrow, Region(x+XPos, y+YPos, Width, Height), (unsigned char*)Buffer, palette, IE_FONT_ALIGN_LEFT, true, finit, initpalette);
	free(Buffer);
	Changed = false;
}
/** Sets the Scroll Bar Pointer. If 'ptr' is NULL no Scroll Bar will be linked
    to this Text Area Control. */
void TextArea::SetScrollBar(Control * ptr)
{
	sb = ptr;
	CalcRowCount();
	Changed = true;
}
/** Sets the Actual Text */
int TextArea::SetText(const char * text, int pos)
{
	if((pos == 0) && (lines.size() == 0))
		pos = -1;
	if(pos >= (int)lines.size())
		return -1;
	int newlen = strlen(text);

	if(pos == -1) {
		char * str = (char*)malloc(newlen+1);
		strcpy(str, text);
		lines.push_back(str);
	}
	else {
		int mylen = strlen(lines[pos]);

		lines[pos] = (char*)realloc(lines[pos], newlen+1);
		strcpy(lines[pos], text);
	}
	CalcRowCount();
	((Window*)Owner)->Invalidate();
	return 0;
}
/** Appends a String to the current Text */
int TextArea::AppendText(const char * text, int pos)
{
	if(pos >= (int)lines.size())
		return -1;
	if(pos == -1)
		return -1;

	int newlen = strlen(text);
	int mylen = strlen(lines[pos]);
	
	lines[pos] = (char*)realloc(lines[pos], mylen+newlen+1);
	strcat(lines[pos], text);
	CalcRowCount();
	((Window*)Owner)->Invalidate();
	return 0;
}
/** Sets the Fonts */
void TextArea::SetFonts(Font * init, Font * text)
{
	finit = init;
	ftext = text;
	Changed = true;
}

/** Key Press Event */
void TextArea::OnKeyPress(unsigned char Key, unsigned short Mod)
{
	
}
/** Special Key Press */
void TextArea::OnSpecialKeyPress(unsigned char Key)
{
	
}

/** Set Starting Row */
void TextArea::SetRow(int row)
{
	if(row < rows)
		startrow = row;
	Changed = true;
}

void TextArea::CalcRowCount()
{
	if(lines.size() != 0) {
		rows = -1;
		for(int i = 0; i < lines.size(); i++) {
			rows++;
			int len = strlen(lines[i]);
			char * tmp = (char*)malloc(len+1);
			strcpy(tmp, lines[i]);
			ftext->SetupString(tmp, Width);
			for(int i = 0; i <= len; i++) {
				if(tmp[i] == 0)
					rows++;
			}
			free(tmp);
		}
	}
	if(!sb)
		return;
	ScrollBar *bar = (ScrollBar*)sb;
	bar->SetMax(rows);
}
