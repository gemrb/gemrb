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
 * $Id$
 *
 */

#include "../../includes/win32def.h"
#include "ScrollBar.h"
#include "Interface.h"
#include "Video.h"
#include "Variables.h"

ScrollBar::ScrollBar(void)
{
	Pos = 0;
	Value = 10;
	State = 0;
	ResetEventHandler( ScrollBarOnChange );
	ta = NULL;
	for(int i=0;i<SB_RES_COUNT;i++)  {
		Frames[i]=NULL;
	}
}

ScrollBar::~ScrollBar(void)
{
	Video *video=core->GetVideoDriver();
	for(int i=0;i<SB_RES_COUNT;i++)  {
		if(Frames[i]) {
			video->FreeSprite(Frames[i]);
		}
	}
}

/** Sets a new position, relays the change to an associated textarea and calls
    any existing GUI OnChange callback */
void ScrollBar::SetPos(int NewPos)
{
	if (Pos && ( Pos == NewPos )) {
		return;
	}
	Changed = true;
	Pos = (ieWord) NewPos;
	if (ta) {
		TextArea* t = ( TextArea* ) ta;
		t->SetRow( Pos );
	}
	if (VarName[0] != 0) {
		core->GetDictionary()->SetAt( VarName, Pos );
	}
	RunEventHandler( ScrollBarOnChange );
}

/** Refreshes the ScrollBar according to a guiscript variable */
void ScrollBar::RedrawScrollBar(const char* Variable, int Sum)
{
	if (strnicmp( VarName, Variable, MAX_VARIABLE_LENGTH )) {
		return;
	}
	SetPos( Sum );
}

/** Mousewheel support */
void ScrollBar::ScrollUp()
{
	if (Pos > 0) {
		SetPos( Pos - 1 );
	}
}

/** Mousewheel support */
void ScrollBar::ScrollDown()
{
	if ( (ieDword) Pos + 1 < Value ) {
		SetPos( Pos + 1 );
	}
}

/** Draws the ScrollBar control */
void ScrollBar::Draw(unsigned short x, unsigned short y)
{
	if (!Changed && !(Owner->Flags&WF_FLOAT) ) {
		return;
	}
	Changed = false;
	if (XPos == 65535) {
		return;
	}
	int upMy = Frames[IE_GUI_SCROLLBAR_UP_UNPRESSED]->Height;
	int doMy = Frames[IE_GUI_SCROLLBAR_DOWN_UNPRESSED]->Height;
	unsigned int domy = (Height - doMy);

	unsigned short slmy = ( unsigned short )
		( upMy +
		( Pos * ( ( domy - Frames[IE_GUI_SCROLLBAR_SLIDER]->Height - upMy ) /
		( double ) ( Value < 2 ? 1 : Value - 1 ) ) ) );
	unsigned short slx = ( unsigned short ) ((Width - Frames[IE_GUI_SCROLLBAR_SLIDER]->Width) / 2 );

	if (( State & UP_PRESS ) != 0) {
		core->GetVideoDriver()->BlitSprite( Frames[IE_GUI_SCROLLBAR_UP_PRESSED],
			x + XPos, y + YPos, true );
	} else {
		core->GetVideoDriver()->BlitSprite( Frames[IE_GUI_SCROLLBAR_UP_UNPRESSED],
			x + XPos, y + YPos, true );
	}
	int maxy = y + YPos + Height -
		Frames[IE_GUI_SCROLLBAR_DOWN_UNPRESSED]->Height;
	int stepy = Frames[IE_GUI_SCROLLBAR_TROUGH]->Height;
	Region rgn( x + XPos, y + YPos + upMy, Width, domy - upMy);
	for (int dy = y + YPos + upMy; dy < maxy; dy += stepy) {
		core->GetVideoDriver()->BlitSprite( Frames[IE_GUI_SCROLLBAR_TROUGH],
			x + XPos + ( ( Width / 2 ) -
			Frames[IE_GUI_SCROLLBAR_TROUGH]->Width / 2 ),
			dy, true, &rgn );
	}
	if (( State & DOWN_PRESS ) != 0) {
		core->GetVideoDriver()->BlitSprite( Frames[IE_GUI_SCROLLBAR_DOWN_PRESSED],
			x + XPos, maxy, true );
	} else {
		core->GetVideoDriver()->BlitSprite( Frames[IE_GUI_SCROLLBAR_DOWN_UNPRESSED],
			x + XPos, maxy, true );
	}
	core->GetVideoDriver()->BlitSprite( Frames[IE_GUI_SCROLLBAR_SLIDER],
			x + XPos + slx + Frames[IE_GUI_SCROLLBAR_SLIDER]->XPos,
			y + YPos + slmy + Frames[IE_GUI_SCROLLBAR_SLIDER]->YPos,
			true );
}

/** Sets a ScrollBar GUI resource */
void ScrollBar::SetImage(unsigned char type, Sprite2D* img)
{
	if (type >= SB_RES_COUNT) {
		return;
	}
	if (Frames[type]) {
		core->GetVideoDriver()->FreeSprite(Frames[type]);
	}
	Frames[type] = img;
	Changed = true;
}

/** Mouse Button Down */
void ScrollBar::OnMouseDown(unsigned short x, unsigned short y,
	unsigned short Button, unsigned short /*Mod*/)
{
	//removing the double click flag, use a more sophisticated method
	//if it is needed later
	Button&=GEM_MB_NORMAL;
	if (Button==GEM_MB_SCRLUP) {
		ScrollUp();
		return;
	}
	if (Button==GEM_MB_SCRLDOWN) {
		ScrollDown();
		return;
	}

	core->RedrawAll();

	unsigned short upMx = (unsigned short) Frames[IE_GUI_SCROLLBAR_UP_UNPRESSED]->Width;
	unsigned short upMy = (unsigned short) Frames[IE_GUI_SCROLLBAR_UP_UNPRESSED]->Height;
	unsigned short domy = (unsigned short) (Height - Frames[IE_GUI_SCROLLBAR_DOWN_UNPRESSED]->Height);
	unsigned short slheight = domy - upMy;
	unsigned short refheight = (unsigned short) (slheight - Frames[IE_GUI_SCROLLBAR_SLIDER]->Height);
	double step = refheight / (double) ( Value < 2 ? 1 : Value - 1 );
	unsigned short ymax = upMy + refheight;
	unsigned short ymy = y - upMy;
	unsigned short doMx = (unsigned short) Frames[IE_GUI_SCROLLBAR_DOWN_UNPRESSED]->Width;
	unsigned short slMx = (unsigned short) Frames[IE_GUI_SCROLLBAR_SLIDER]->Width;
	unsigned short slmy = (unsigned short) (upMy + Pos * step);
	unsigned short slMy = (unsigned short) (slmy + Frames[IE_GUI_SCROLLBAR_SLIDER]->Height);
	if (( x <= upMx ) && ( y <= upMy )) {
		if (Pos > 0)
			SetPos( Pos - 1 );
		State |= UP_PRESS;
		return;
	}
	if (y >= domy) {
		if (( x <= doMx ) && ( y <= Height )) {
			if ( (ieDword) Pos + 1 < Value )
				SetPos( Pos + 1 );
			State |= DOWN_PRESS;
			return;
		}
	}
	if (y >= slmy) {
		if (( x <= slMx ) && ( y <= slMy )) {
			State |= SLIDER_GRAB;
			return;
		}
	}
	if (y <= upMy) {
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
	unsigned short /*Button*/, unsigned short /*Mod*/)
{
	Changed = true;
	State = 0;
}

/** Mouse Over Event */
void ScrollBar::OnMouseOver(unsigned short /*x*/, unsigned short y)
{
	if (( State & SLIDER_GRAB ) != 0) {
		core->RedrawAll();
		unsigned short upMy =(unsigned short) Frames[IE_GUI_SCROLLBAR_UP_UNPRESSED]->Height;
		unsigned short domy = (unsigned short) (Height - Frames[IE_GUI_SCROLLBAR_DOWN_UNPRESSED]->Height);
		unsigned short slheight = domy - upMy;
		unsigned short refheight = (unsigned short) (slheight - Frames[IE_GUI_SCROLLBAR_SLIDER]->Height);
		double step = refheight / ( double ) ( Value < 2 ? 1 : Value - 1 );
		unsigned short yzero = (unsigned short) (upMy + Frames[IE_GUI_SCROLLBAR_SLIDER]->Height / 2 );
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
			if (befst > Value )
			SetPos( befst );
		} else {
			if (aftst < Value )
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
}

/** Sets the ScrollBarOnChange event (guiscript callback) */
bool ScrollBar::SetEvent(int eventType, const char *handler)
{
	Changed = true;

	switch (eventType) {
	case IE_GUI_SCROLLBAR_ON_CHANGE:
		SetEventHandler( ScrollBarOnChange, handler );
		break;
	default:
		return false;
	}

	return true;
}
