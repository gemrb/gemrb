/***************************************************************************
                          Label.cpp  -  description
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

#include "win32def.h"
#include "Label.h"

Label::Label(unsigned short bLength, Font * font){
	this->font = font;
	Buffer = NULL;
	if(bLength != 0)
		Buffer = (char*)malloc(bLength);
	useRGB = false;
	Alignment = IE_FONT_ALIGN_LEFT;
}
Label::~Label(){
	if(Buffer)
		free(Buffer);
}
/** Draws the Control on the Output Display */
void Label::Draw(unsigned short x, unsigned short y)
{
	if(font) {
		if(useRGB)
			font->Print(Region(this->XPos+x, this->YPos+y, this->Width, this->Height), (unsigned char*)Buffer, &fore, Alignment, true);
		else
			font->Print(Region(this->XPos+x, this->YPos+y, this->Width, this->Height), (unsigned char*)Buffer, NULL, Alignment, true);
	}
}
/** This function sets the actual Label Text */
void Label::SetText(char * string)
{
	if(Buffer != NULL)
		strcpy(Buffer, string);
}
/** Sets the Foreground Font Color */
void Label::SetColor(Color col)
{
	fore = col;
}

void Label::SetAlignment(unsigned char Alignment)
{
	if(Alignment > IE_FONT_ALIGN_RIGHT)
		return;
	this->Alignment = Alignment;
}
