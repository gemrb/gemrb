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

#include "TextArea.h"
#include <stdio.h>

TextArea::TextArea(Color hitextcolor, Color initcolor, Color lowtextcolor)
{
	BufferLength = 4096;
	Buffer = (unsigned char*)malloc(BufferLength);
	sb = NULL;
	hi = hitextcolor;
	init = initcolor;
	low = lowtextcolor;
}

TextArea::~TextArea(void)
{
	free(Buffer);
}

void TextArea::Draw(unsigned short x, unsigned short y)
{
	ftext->Print(Region(x+XPos, y+YPos, Width, Height), Buffer, &hi, &low, IE_FONT_ALIGN_LEFT | IE_FONT_ALIGN_TOP, true, finit, &init);		
}
/** Sets the Scroll Bar Pointer. If 'ptr' is NULL no Scroll Bar will be linked
    to this Text Area Control. */
void TextArea::SetScrollBar(ScrollBar * ptr)
{
	sb = ptr;
}
/** Sets the Actual Text */
void TextArea::SetText(unsigned char * text)
{
	strncpy((char*)Buffer, (char*)text, BufferLength);
}
/** Sets the Fonts */
void TextArea::SetFonts(Font * init, Font * text)
{
	finit = init;
	ftext = text;
}