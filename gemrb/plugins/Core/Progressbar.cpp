/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Progressbar.cpp,v 1.1 2004/08/08 20:50:00 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "Progressbar.h"
#include "Interface.h"

extern Interface* core;

Progressbar::Progressbar( unsigned short KnobStepsCount, bool Clear)
{
	this->KnobStepsCount = KnobStepsCount;
	BackGround = NULL;
	this->Clear = Clear;
	Pos = 0;
	Value = 1;
	PBarAnim = NULL;
}

Progressbar::~Progressbar()
{
	if (!Clear) {
		return;
	}
	if (BackGround) {
		core->GetVideoDriver()->FreeSprite( BackGround );
	}
	if (BackGround2) {
		core->GetVideoDriver()->FreeSprite( BackGround2 );
	}
	if (PBarAnim) {
		delete( PBarAnim );
	}
}

/** Draws the Control on the Output Display */
void Progressbar::Draw(unsigned short x, unsigned short y)
{
	if (!Changed && !((Window*)Owner)->Floating) {
		return;
	}
	Changed = false;
	if (XPos == 65535) {
		return;
	}
	Sprite2D *bcksprite;

	if(Pos == KnobStepsCount) bcksprite=BackGround2;
	else bcksprite=BackGround;
	if (bcksprite) {
		Region r( x + XPos, y + YPos, Width, Height );
		core->GetVideoDriver()->BlitSprite( bcksprite,
			x + XPos, y + YPos, true, &r );
	}
	if(!PBarAnim)
		return;
	//blitting all the sprites
	for(unsigned int i=0; i<Pos ;i++ ) {
		Sprite2D *Knob = PBarAnim->GetFrame(i);
		core->GetVideoDriver()->BlitSprite( Knob, x + XPos, y + YPos, true );
		core->GetVideoDriver()->FreeSprite(Knob);
	}
}

/** Returns the actual Progressbar Position */
unsigned int Progressbar::GetPosition()
{
	return Pos;
}

/** Sets the actual Progressbar Position trimming to the Max and Min Values */
void Progressbar::SetPosition(unsigned int pos)
{
	if (pos <= KnobStepsCount) {
		Pos = pos;
	}
	Changed = true;
}

void Progressbar::RedrawProgressbar(char* VariableName, int Sum)
{
        if (strnicmp( VarName, VariableName, MAX_VARIABLE_LENGTH )) {
                return;
        }
        if (!Value) {
                Value = 1;
        }
        Sum /= Value;
        if (Sum <= KnobStepsCount) {
                Pos = Sum;
        }
        Changed = true;
}

/** Sets the selected image */
void Progressbar::SetImage(Sprite2D* img, Sprite2D* img2)
{
	if (BackGround && Clear)
		core->GetVideoDriver()->FreeSprite( BackGround );
	BackGround = img;
	if (BackGround2 && Clear)
		core->GetVideoDriver()->FreeSprite( BackGround2 );
	BackGround2 = img;
	Changed = true;
}

void Progressbar::SetAnimation(Animation *arg)
{
	PBarAnim = arg;
}

/* dummy virtual function */
int Progressbar::SetText(const char* string, int pos)
{
        return 0;
}

