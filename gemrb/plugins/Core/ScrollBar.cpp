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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/ScrollBar.cpp,v 1.25 2004/08/19 21:14:26 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "ScrollBar.h"
#include "Interface.h"

extern Interface* core;

ScrollBar::ScrollBar(void)
{
	Pos = 0;
	Value = 10;
	State = 0;
	ScrollBarOnChange[0] = 0;
	ta = NULL;
}

ScrollBar::~ScrollBar(void)
{
}

void ScrollBar::SetPos(int NewPos)
{
	if (Pos && ( Pos == NewPos )) {
		return;
	}
	Pos = NewPos;
	if (ta) {
		TextArea* t = ( TextArea* ) ta;
		t->SetRow( Pos );
	}
	if (VarName[0] != 0) {
		core->GetDictionary()->SetAt( VarName, Pos );
	}
	RunEventHandler( ScrollBarOnChange );
}

void ScrollBar::RedrawScrollBar(const char* Variable, int Sum)
{
	if (strnicmp( VarName, Variable, MAX_VARIABLE_LENGTH )) {
		return;
	}
	SetPos( Sum );
}

void ScrollBar::Draw(unsigned short x, unsigned short y)
{
	if (!Changed && !((Window*)Owner)->Floating) {
		return;
	}
	Changed = false;
	if (XPos == 65535) {
		return;
	}
	unsigned short upMy = frames[IE_GUI_SCROLLBAR_UP_UNPRESSED]->Height;
	unsigned short doMy = frames[IE_GUI_SCROLLBAR_DOWN_UNPRESSED]->Height;
	unsigned short domy = Height - doMy;
	unsigned short slmy = ( unsigned short )
		( upMy +
		( Pos * ( ( domy - frames[5]->Height - upMy ) /
		( double ) ( Value == 1 ? Value : Value - 1 ) ) ) );
	unsigned short slx = ( Width / 2 ) -
		( frames[IE_GUI_SCROLLBAR_SLIDER]->Width / 2 );

	if (( State & UP_PRESS ) != 0) {
		core->GetVideoDriver()->BlitSprite( frames[IE_GUI_SCROLLBAR_UP_PRESSED],
			x + XPos, y + YPos, true );
	} else {
		core->GetVideoDriver()->BlitSprite( frames[IE_GUI_SCROLLBAR_UP_UNPRESSED],
			x + XPos, y + YPos, true );
	}
	int maxy = y + YPos + Height -
		frames[IE_GUI_SCROLLBAR_DOWN_UNPRESSED]->Height;
	int stepy = frames[IE_GUI_SCROLLBAR_TROUGH]->Height;
	Region rgn( x + XPos, y + YPos + upMy, Width, domy - upMy);
	for (int dy = y + YPos + upMy; dy < maxy; dy += stepy) {
		core->GetVideoDriver()->BlitSprite( frames[IE_GUI_SCROLLBAR_TROUGH],
			x + XPos + ( ( Width / 2 ) -
			frames[IE_GUI_SCROLLBAR_TROUGH]->Width / 2 ),
			dy, true, &rgn );
	}
	if (( State & DOWN_PRESS ) != 0) {
		core->GetVideoDriver()->BlitSprite( frames[IE_GUI_SCROLLBAR_DOWN_PRESSED],
			x + XPos, maxy, true );
	} else {
		core->GetVideoDriver()->BlitSprite( frames[IE_GUI_SCROLLBAR_DOWN_UNPRESSED],
			x + XPos, maxy, true );
	}
	core->GetVideoDriver()->BlitSprite( frames[IE_GUI_SCROLLBAR_SLIDER],
			x + XPos + slx + frames[IE_GUI_SCROLLBAR_SLIDER]->XPos,
			y + YPos + slmy + frames[IE_GUI_SCROLLBAR_SLIDER]->YPos,
			true );
}

void ScrollBar::SetImage(unsigned char type, Sprite2D* img)
{
	if (type > IE_GUI_SCROLLBAR_SLIDER) {
		return;
	}
	frames[type] = img;
	Changed = true;
}
/** Mouse Button Down */
void ScrollBar::OnMouseDown(unsigned short x, unsigned short y,
	unsigned char /*Button*/, unsigned short /*Mod*/)
{
	//((Window*)Owner)->Invalidate();
	core->RedrawAll();
	unsigned short upmx = 0, upMx = frames[0]->Width, upmy = 0,
	upMy = frames[0]->Height;
	unsigned short domy = Height - frames[2]->Height;
	unsigned short slheight = domy - upMy;
	unsigned short refheight = slheight - frames[5]->Height;
	double step = refheight / ( double ) ( Value == 1 ? Value : Value - 1 );
	unsigned short yzero = upMy + ( frames[5]->Height / 2 );
	unsigned short ymax = yzero + refheight;
	unsigned short ymy = y - yzero;
	unsigned short domx = 0, doMx = frames[2]->Width, doMy = Height;
	unsigned short slmx = 0, slMx = frames[5]->Width,
	slmy = yzero + ( unsigned short ) ( Pos * step ),
	slMy = slmy + frames[5]->Height;
	if (( x >= upmx ) && ( y >= upmy )) {
		if (( x <= upMx ) && ( y <= upMy )) {
			if (Pos > 0)
				SetPos( Pos - 1 );
			State |= UP_PRESS;
			return;
		}
	}
	if (( x >= domx ) && ( y >= domy )) {
		if (( x <= doMx ) && ( y <= doMy )) {
			if (Pos < ( Value - 1 ))
				SetPos( Pos + 1 );
			State |= DOWN_PRESS;
			return;
		}
	}
	if (( x >= slmx ) && ( y >= slmy )) {
		if (( x <= slMx ) && ( y <= slMy )) {
			State |= SLIDER_GRAB;
			return;
		}
	}
	if (y <= yzero) {
		SetPos( 0 );
		return;
	}
	if (y >= ymax) {
		SetPos( Value - 1 );
		return;
	}
	unsigned short befst = ( unsigned short ) ( ymy / step );
	unsigned short aftst = befst + 1;
	if (( ymy - ( befst * step ) ) < ( ( aftst * step ) - ymy )) {
		SetPos( befst );
	} else {
		SetPos( aftst );
	}
}
/** Mouse Button Up */
void ScrollBar::OnMouseUp(unsigned short /*x*/, unsigned short /*y*/,
	unsigned char /*Button*/, unsigned short /*Mod*/)
{
	Changed = true;
	State = 0;
}
/** Mouse Over Event */
void ScrollBar::OnMouseOver(unsigned short /*x*/, unsigned short y)
{
	if (( State & SLIDER_GRAB ) != 0) {
		//((Window*)Owner)->Invalidate();
		core->RedrawAll();
		unsigned short upMy = frames[0]->Height;
		unsigned short domy = Height - frames[2]->Height;
		unsigned short slheight = domy - upMy;
		unsigned short refheight = slheight - frames[5]->Height;
		double step = refheight /
			( double ) ( Value == 1 ? Value : Value - 1 );
		unsigned short yzero = upMy + ( frames[5]->Height / 2 );
		unsigned short ymax = yzero + refheight;
		unsigned short ymy = y - yzero;
		if (y <= yzero) {
			SetPos( 0 );
			return;
		}
		if (y >= ymax) {
			SetPos( Value - 1 );
			return;
		}
		unsigned short befst = ( unsigned short ) ( ymy / step );
		unsigned short aftst = befst + 1;
		if (( ymy - ( befst * step ) ) < ( ( aftst * step ) - ymy )) {
			SetPos( befst );
		} else {
			if (aftst <= ( Value - 1 ))
				SetPos( aftst );
		}
	}
}

/** Sets the Text of the current control */
int ScrollBar::SetText(const char* /*string*/, int /*pos*/)
{
	return 0;
}

/** Sets the Maximum Value of the ScrollBar */
void ScrollBar::SetMax(unsigned short Max)
{
	Value = Max;
	if (Max == 0) {
		SetPos( 0 );
	} else if (Pos >= Max) {
		SetPos( Max - 1 );
	}
	Changed = true;
}
