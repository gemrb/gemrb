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
	rows = 0;
	startrow = 0;
	seltext = -1;
	selline = -1;
	sb = NULL;
	Selectable = false;
	palette = core->GetVideoDriver()->CreatePalette(hitextcolor, lowtextcolor);
	initpalette = core->GetVideoDriver()->CreatePalette(initcolor, lowtextcolor);
	Color tmp = {hitextcolor.b, hitextcolor.g, hitextcolor.r, 0};
	selected = core->GetVideoDriver()->CreatePalette(tmp, lowtextcolor);
	tmp.r = 255;
	tmp.g = 152;
	tmp.b = 102;
	lineselpal = core->GetVideoDriver()->CreatePalette(tmp, lowtextcolor);
}

TextArea::~TextArea(void)
{
	free(palette);
	free(initpalette);
	free(selected);
	free(lineselpal);
	for(int i = 0; i < lines.size(); i++) {
		free(lines[i]);
	}
}

void TextArea::Draw(unsigned short x, unsigned short y)
{
	if(!Changed)
		return;
	Changed=false;
	if(XPos==65535)
		return;
	if(lines.size() == 0)
		return;
	if(!Selectable) {
  	char * Buffer = (char*)malloc(1);
  	Buffer[0] = 0;
  	int len = 0;
  	for(int i = 0; i < lines.size(); i++) {
  		len += strlen(lines[i])+1;
  		Buffer = (char*)realloc(Buffer, len+1);
  		strcat(Buffer, lines[i]);
  		if(i != lines.size()-1)
  			strcat(Buffer, "\n");
  	}
  	ftext->PrintFromLine(startrow, Region(x+XPos, y+YPos, Width, Height), (unsigned char*)Buffer, palette, IE_FONT_ALIGN_LEFT, true, finit, initpalette);
  	free(Buffer);
	}
	else {
  	int rc = 0;
  	int acc = 0;
  	int sr = startrow;
  	int i = 0;
  	int yl = 0;
  	for(i = 0; i < lines.size(); i++) {
  		if(rc+lrows[i] <= sr) {
  			rc+=lrows[i];
  			continue;
  		}
  		sr -= rc;
  		Color * pal = NULL;
  		if(seltext == i)
  			pal = selected;
  		else if(selline == i)
			pal = lineselpal;
		else
  			pal = palette;
  		ftext->PrintFromLine(sr, Region(x+XPos, y+YPos, Width, Height), (unsigned char*)lines[i], pal, IE_FONT_ALIGN_LEFT, true, finit, initpalette);
  		yl = lrows[i]-sr;
  		break;
  	}
  	for(i++;i < lines.size(); i++) {
  		Color * pal = NULL;
  		if(seltext == i)
  			pal = selected;
  		else if(selline == i)
			pal = lineselpal;
		else
  			pal = palette;
  		ftext->Print(Region(x+XPos, y+YPos+(yl*ftext->chars[1]->Height), Width, Height-(yl*ftext->chars[1]->Height)), (unsigned char*)lines[i], pal, IE_FONT_ALIGN_LEFT, true);
		yl+=lrows[i];
  	}
	}
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
		lrows.push_back(0);
	}
	else {
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
	int newlen = strlen(text);

	if(pos == -1) {
		char * str = (char*)malloc(newlen+1);
		strcpy(str, text);
		lines.push_back(str);
		lrows.push_back(0);
	}
	else
	{
		int mylen = strlen(lines[pos]);
	
		lines[pos] = (char*)realloc(lines[pos], mylen+newlen+1);
		strcat(lines[pos], text);
	}
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

/** Returns Row count */
int TextArea::GetRowCount()
{
	return lines.size();
}

/** Returns top index */
int TextArea::GetTopIndex()
{
	return startrow;
}

/** Set Starting Row */
void TextArea::SetRow(int row)
{
	if(row < rows)
		startrow = row;
	Changed = true;
}

/** Set Selectable */
void TextArea::SetSelectable(bool val)
{
	Selectable=val;
}

void TextArea::CalcRowCount()
{
	if(lines.size() != 0) {
		rows = -1;
		for(int i = 0; i < lines.size(); i++) {
			rows++;
			int tr = 0;
			int len = strlen(lines[i]);
			char * tmp = (char*)malloc(len+1);
			memcpy(tmp, lines[i], len+1);
			ftext->SetupString(tmp, Width);
			for(int p = 0; p <= len; p++) {
				if(tmp[p] == 0) {
					rows++;
					tr++;
				}
			lrows[i] = tr;
			}
			free(tmp);
		}
	}
	if(!sb)
		return;
	ScrollBar *bar = (ScrollBar*)sb;
	bar->SetMax(rows);
}
/** Mouse Over Event */
void TextArea::OnMouseOver(unsigned short x, unsigned short y)
{
	if(!Selectable)
		return;
	int height = ftext->chars[1]->Height;
	int r = y/height;
	int row = 0;
	((Window*)Owner)->Invalidate();
	for(int i = 0; i < lines.size(); i++) {
		row+=lrows[i];
		if(r < (row-startrow)) {
			seltext = i;
			printf("CtrlId = 0x%08lx, seltext = %d, rows = %d, row = %d, r = %d\n", ControlID, i, rows, row, r);
			return;
		}
	}
	seltext = -1;
	printf("CtrlId = 0x%08lx, seltext = %d, rows %d, row %d, r = %d\n", ControlID, seltext, rows, row, r);
}
/** Mouse Button Up */
void TextArea::OnMouseUp(unsigned short x, unsigned short y, unsigned char Button, unsigned short Mod)
{
	if((x <= Width) && (y <= Height)) {
		selline = seltext;
		((Window*)Owner)->Invalidate();
	}
	if(VarName[0]!=0)
		core->GetDictionary()->SetAt(VarName,selline);
}
