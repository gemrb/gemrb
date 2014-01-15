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

Label::Label(const Region& frame, Font* font, const char* string)
	: Control(frame)
{
	ControlType = IE_GUI_LABEL;
	this->font = font;
	useRGB = false;
	ResetEventHandler( LabelOnPress );

	Alignment = IE_FONT_ALIGN_CENTER|IE_FONT_ALIGN_MIDDLE;
	palette = NULL;
	Text = NULL;
	SetText(string);
}

Label::~Label()
{
	delete Text;
	gamedata->FreePalette( palette );
}
/** Draws the Control on the Output Display */
void Label::DrawInternal(Region& rgn)
{
	if (font && Text->length()) {
		font->Print( rgn, *Text,
			useRGB?palette:NULL,
					 Alignment | IE_FONT_SINGLE_LINE, true );
	}

	if (AnimPicture) {
		int xOffs = ( Width / 2 ) - ( AnimPicture->Width / 2 );
		int yOffs = ( Height / 2 ) - ( AnimPicture->Height / 2 );
		Region r( rgn.x + xOffs, rgn.y + yOffs, (int)(AnimPicture->Width), AnimPicture->Height );
		core->GetVideoDriver()->BlitSprite( AnimPicture, r.x + xOffs, r.y + yOffs, true, &r );
	}

}
/** This function sets the actual Label Text */
void Label::SetText(const char* string)
{
	delete Text;
	Text = StringFromCString(string);
	if (Alignment == IE_FONT_ALIGN_CENTER
		&& core->HasFeature( GF_LOWER_LABEL_TEXT )) {
		StringToLower(*Text);
	}
	if (!palette) {
		SetColor(ColorWhite, ColorBlack);
	}
	MarkDirty();
}
/** Sets the Foreground Font Color */
void Label::SetColor(Color col, Color bac)
{
	gamedata->FreePalette( palette );
	palette = core->CreatePalette( col, bac );
	MarkDirty();
}

void Label::SetAlignment(unsigned char Alignment)
{
	this->Alignment = Alignment;
	if (Alignment == IE_FONT_ALIGN_CENTER) {
		if (core->HasFeature( GF_LOWER_LABEL_TEXT )) {
			StringToLower(*Text);
		}
	}
	MarkDirty();
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
const String& Label::QueryText() const
{
	if (Text)
		return *Text;
	return Control::QueryText();
}

}
