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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "GUI/Label.h"

#include "win32def.h"

#include "GameData.h"
#include "Interface.h"
#include "Sprite2D.h"
#include "Variables.h"
#include "Video.h"
#include "GUI/Window.h"

namespace GemRB {

Label::Label(Font* font)
{
	this->font = font;
	Buffer = NULL;
	useRGB = false;
	ResetEventHandler( LabelOnPress );

	Alignment = IE_FONT_ALIGN_CENTER|IE_FONT_ALIGN_MIDDLE;
	palette = NULL;
}
Label::~Label()
{
	gamedata->FreePalette( palette );
	if (Buffer) {
		free( Buffer );
	}
}
/** Draws the Control on the Output Display */
void Label::Draw(unsigned short x, unsigned short y)
{
	if (!Changed && !(Owner->Flags&WF_FLOAT)) {
		return;
	}
	Changed = false;
	if (XPos == 65535) {
		return;
	}
	if (font && Buffer) {
		font->Print( Region( this->XPos + x, this->YPos + y,
			this->Width, this->Height ), (unsigned char*)Buffer,
			useRGB?palette:NULL,
					 Alignment | IE_FONT_SINGLE_LINE, true );
	}

	if (AnimPicture) {
		int xOffs = ( Width / 2 ) - ( AnimPicture->Width / 2 );
		int yOffs = ( Height / 2 ) - ( AnimPicture->Height / 2 );
		Region r( x + XPos + xOffs, y + YPos + yOffs, (int)(AnimPicture->Width), AnimPicture->Height );
		core->GetVideoDriver()->BlitSprite( AnimPicture, x + XPos + xOffs, y + YPos + yOffs, true, &r );
	}

}
/** This function sets the actual Label Text */
void Label::SetText(const char* string)
{
	if (Buffer )
		free( Buffer );
	if (Alignment == IE_FONT_ALIGN_CENTER) {
		if (core->HasFeature( GF_LOWER_LABEL_TEXT )) {
			int len = strlen(string);
			Buffer = (char *) malloc( len+1 );
			strnlwrcpy( Buffer, string, len );
		}
		else {
			Buffer = strdup( string );
		}
	}
	else {
		Buffer = strdup( string );
	}
	if (!palette) {
		SetColor(ColorWhite, ColorBlack);
	}
	if (Owner) {
		Owner->Invalidate();
	}
}
/** Sets the Foreground Font Color */
void Label::SetColor(Color col, Color bac)
{
	gamedata->FreePalette( palette );
	palette = core->CreatePalette( col, bac );
	Changed = true;
}

void Label::SetAlignment(unsigned char Alignment)
{
	this->Alignment = Alignment;
	if (Alignment == IE_FONT_ALIGN_CENTER) {
		if (core->HasFeature( GF_LOWER_LABEL_TEXT )) {
			strtolower( Buffer );
		}
	}
	Changed = true;
}

void Label::OnMouseUp(unsigned short x, unsigned short y,
	unsigned short /*Button*/, unsigned short /*Mod*/)
{
	//print("Label::OnMouseUp");
	if (( x <= Width ) && ( y <= Height )) {
		if (VarName[0] != 0) {
			core->GetDictionary()->SetAt( VarName, Value );
		}
		if (LabelOnPress) {
			RunEventHandler( LabelOnPress );
		}
	}
}

bool Label::SetEvent(int eventType, EventHandler handler)
{
	Changed = true;

	switch (eventType) {
	case IE_GUI_LABEL_ON_PRESS:
		LabelOnPress = handler;
		break;
	default:
		return false;
	}

	return true;
}

/** Simply returns the pointer to the text, don't modify it! */
const char* Label::QueryText() const
{
	return ( const char * ) Buffer;
}

}
