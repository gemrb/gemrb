/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Label.cpp,v 1.23 2004/05/09 17:36:26 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "Label.h"
#include "Interface.h"

extern Interface* core;

Label::Label(unsigned short bLength, Font* font)
{
	this->font = font;
	Buffer = NULL;
	if (bLength != 0) {
		Buffer = ( char * ) malloc( bLength );
	}
	useRGB = false;
	LabelOnPress[0] = 0;

	Alignment = IE_FONT_ALIGN_LEFT;
	palette = NULL;
}
Label::~Label()
{
	core->GetVideoDriver()->FreePalette( palette );
	if (Buffer) {
		free( Buffer );
	}
}
/** Draws the Control on the Output Display */
void Label::Draw(unsigned short x, unsigned short y)
{
	if (!Changed && !((Window*)Owner)->Floating) {
		return;
	}
	Changed = false;
	if (XPos == 65535) {
		return;
	}
	if (font) {
		font->Print( Region( this->XPos + x, this->YPos + y,
			this->Width, this->Height ), ( unsigned char * ) Buffer,
			useRGB?palette:NULL, Alignment | IE_FONT_ALIGN_MIDDLE |
			IE_FONT_SINGLE_LINE, true );
	}
}
/** This function sets the actual Label Text */
int Label::SetText(const char* string, int pos)
{
	if (Buffer != NULL) {
		strcpy( Buffer, string );
		if (Alignment == IE_FONT_ALIGN_CENTER)
			if (core->HasFeature( GF_LOWER_LABEL_TEXT ))
				strlwr( Buffer );
	}
	Changed = true;
	return 0;
}
/** Sets the Foreground Font Color */
void Label::SetColor(Color col, Color bac)
{
	core->GetVideoDriver()->FreePalette( palette );
	palette = core->GetVideoDriver()->CreatePalette( col, bac );
	Changed = true;
}

void Label::SetAlignment(unsigned char Alignment)
{
	if (Alignment > IE_FONT_ALIGN_RIGHT) {
		return;
	}
	this->Alignment = Alignment;
	if (Alignment == IE_FONT_ALIGN_CENTER) {
		if (core->HasFeature( GF_LOWER_LABEL_TEXT )) {
			strlwr( Buffer );
		}
	}
	Changed = true;
}

void Label::OnMouseUp(unsigned short x, unsigned short y,
	unsigned char Button, unsigned short Mod)
{
	printf( "Label::OnMouseUp\n" );
	if (( x <= Width ) && ( y <= Height )) {
		if (VarName[0] != 0) {
			core->GetDictionary()->SetAt( VarName, Value );
		}
		if(LabelOnPress[0]) {
			RunEventHandler( LabelOnPress );
		}
	}
}
