/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "GUI/ScrollBar.h"

#include "win32def.h"

#include "Interface.h"
#include "Variables.h"
#include "Video.h"
#include "GUI/EventMgr.h"
#include "GUI/Window.h"

namespace GemRB {

ScrollBar::ScrollBar(const Region& frame)
	: Control(frame)
{
	ControlType = IE_GUI_SCROLLBAR;
	Pos = 0;
	Value = 0;
	State = 0;
	stepPx = SliderYPos = 0;
	ResetEventHandler( ScrollBarOnChange );
	ta = NULL;
	for(int i=0;i<SB_RES_COUNT;i++) {
		Frames[i]=NULL;
	}
}

ScrollBar::~ScrollBar(void)
{
	Video *video=core->GetVideoDriver();
	for(int i=0;i<SB_RES_COUNT;i++) {
		if(Frames[i]) {
			video->FreeSprite(Frames[i]);
		}
	}
}

int ScrollBar::GetFrameHeight(int frame) const
{
	if (!Frames[frame]) return 0;
	return Frames[frame]->Height;
}

void ScrollBar::CalculateStep()
{
	if (Value){
		stepPx = (double)SliderRange / (double)Value;
	} else {
		stepPx = 0.0;
	}
}

/** Sets a new position, relays the change to an associated textarea and calls
	any existing GUI OnChange callback */
void ScrollBar::SetPos(ieDword NewPos, bool redraw)
{
	if (!Frames[IE_GUI_SCROLLBAR_UP_UNPRESSED]) return;

	if (NewPos > Value) NewPos = Value;

	if (( State & SLIDER_GRAB ) == 0){
		// set the slider to the exact y for NewPos. in SetPosForY(y) it is set to any arbitrary position that may lie between 2 values.
		// if the slider is grabbed dont set position! otherwise you will get a flicker as it bounces between exact positioning and arbitrary
		SliderYPos = ( unsigned short )
			( NewPos * ( ( SliderRange ) /
			( double ) ( Value < 1 ? 1 : Value ) ) );
	}
	if (Pos && ( Pos == NewPos )) {
		return;
	}
	
	MarkDirty();
	Pos = (ieWord) NewPos;
	if (ta) {
		(( TextArea* )ta)->SetRow( Pos );
	}
	if (VarName[0] != 0) {
		core->GetDictionary()->SetAt( VarName, Pos );
	}
	RunEventHandler( ScrollBarOnChange );
	if(redraw) core->RedrawAll();
}

/** Sets the Pos for a given y coordinate (control coordinates) */
/** Provides per-pixel scrolling. Top = 0px */
void ScrollBar::SetPosForY(unsigned short y)
{
	if (Value > 0) {// if the value is 0 we are simultaneously at both the top and bottom so there is nothing to do
		if (y > SliderRange) y = SliderRange;

		if (stepPx) {
			unsigned short NewPos = (unsigned short)(y / stepPx);
			if (Pos != NewPos) {
				SetPos(NewPos);
			}
			if (ta) {
				// we must "scale" the pixels the slider moves
				TextArea* t = (TextArea*) ta;
				unsigned int taY = y * (t->GetRowHeight() / stepPx);
				t->ScrollToY(taY, this);
			}
			SliderYPos = y;
			core->RedrawAll();
		}
	} else {
		// top is our default position
		SliderYPos = 0;
	}
}

/** Refreshes the ScrollBar according to a guiscript variable */
void ScrollBar::UpdateState(const char* Variable, unsigned int Sum)
{
	if (strnicmp( VarName, Variable, MAX_VARIABLE_LENGTH )) {
		return;
	}
	SetPos( Sum );
}

/** SDL < 1.3 Mousewheel support */
void ScrollBar::ScrollUp()
{
	if( Pos ) SetPos( Pos - 1 );
}

/** SDL < 1.3 Mousewheel support */
void ScrollBar::ScrollDown()
{
	SetPos( Pos + 1 );
}

double ScrollBar::GetStep()
{
	CalculateStep();
	return stepPx;
}

/** Draws the ScrollBar control */
void ScrollBar::DrawInternal(Region& drawFrame)
{
	Video *video=core->GetVideoDriver();
	int upMy = GetFrameHeight(IE_GUI_SCROLLBAR_UP_UNPRESSED);
	int doMy = GetFrameHeight(IE_GUI_SCROLLBAR_DOWN_UNPRESSED);
	unsigned int domy = (Height - doMy);
	
	//draw the up button
	if (( State & UP_PRESS ) != 0) {
		if (Frames[IE_GUI_SCROLLBAR_UP_PRESSED])
			video->BlitSprite( Frames[IE_GUI_SCROLLBAR_UP_PRESSED], drawFrame.x, drawFrame.y, true );
	} else {
		if (Frames[IE_GUI_SCROLLBAR_UP_UNPRESSED])
			video->BlitSprite( Frames[IE_GUI_SCROLLBAR_UP_UNPRESSED], drawFrame.x, drawFrame.y, true );
	}
	//draw the trough
	int maxy = drawFrame.y + drawFrame.h - GetFrameHeight(IE_GUI_SCROLLBAR_DOWN_UNPRESSED);
	int stepy = GetFrameHeight(IE_GUI_SCROLLBAR_TROUGH);
	if (stepy) {
		Region rgn( drawFrame.x, drawFrame.y + upMy, drawFrame.w, domy - upMy);
		for (int dy = drawFrame.y + upMy; dy < maxy; dy += stepy) {
			//TROUGH surely exists if it has a nonzero height
			video->BlitSprite( Frames[IE_GUI_SCROLLBAR_TROUGH],
				drawFrame.x + ( ( Width / 2 ) - Frames[IE_GUI_SCROLLBAR_TROUGH]->Width / 2 ),
				dy, true, &rgn );
		}
	}
	//draw the down button
	if (( State & DOWN_PRESS ) != 0) {
		if (Frames[IE_GUI_SCROLLBAR_DOWN_PRESSED]) 
			video->BlitSprite( Frames[IE_GUI_SCROLLBAR_DOWN_PRESSED], drawFrame.x, maxy, true );
	} else {
		if (Frames[IE_GUI_SCROLLBAR_DOWN_UNPRESSED])
			video->BlitSprite( Frames[IE_GUI_SCROLLBAR_DOWN_UNPRESSED], drawFrame.x, maxy, true );
	}
	//draw the slider if it exists
	if (Frames[IE_GUI_SCROLLBAR_SLIDER]) {
		unsigned short slx = ( unsigned short ) ((Width - Frames[IE_GUI_SCROLLBAR_SLIDER]->Width) / 2 );
		video->BlitSprite( Frames[IE_GUI_SCROLLBAR_SLIDER],
			drawFrame.x + slx + Frames[IE_GUI_SCROLLBAR_SLIDER]->XPos,
			drawFrame.y + Frames[IE_GUI_SCROLLBAR_SLIDER]->YPos + upMy + SliderYPos,
			true );
	}
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
	MarkDirty();
	SliderRange = Height
	- GetFrameHeight(IE_GUI_SCROLLBAR_SLIDER)
	- GetFrameHeight(IE_GUI_SCROLLBAR_DOWN_UNPRESSED)
	- GetFrameHeight(IE_GUI_SCROLLBAR_UP_UNPRESSED);
}

/** Mouse Button Down */
void ScrollBar::OnMouseDown(unsigned short /*x*/, unsigned short y,
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
	if (y <= GetFrameHeight(IE_GUI_SCROLLBAR_UP_UNPRESSED) ) {
		State |= UP_PRESS;
		ScrollUp();
		return;
	}
	if (y >= Height - GetFrameHeight(IE_GUI_SCROLLBAR_DOWN_UNPRESSED)) {
		State |= DOWN_PRESS;
		ScrollDown();
		return;
	}
	CalculateStep();
	// if we made it this far we will jump the nib to y and "grab" it
	// this way we only need to click once to jump+scroll
	State |= SLIDER_GRAB;
	if (y >= SliderYPos && y <= SliderYPos + GetFrameHeight(IE_GUI_SCROLLBAR_SLIDER)) {
		// FIXME: hack. we shouldnt mess with the sprite position should we?
		Frames[IE_GUI_SCROLLBAR_SLIDER]->YPos = y - SliderYPos;
		return;
	}
	ieWord top = (Height - SliderRange)/2;
	if (y >= top) {
		SetPosForY(y - top);
	} else {
		SetPosForY(0);
	}
}

/** Mouse Button Up */
void ScrollBar::OnMouseUp(unsigned short /*x*/, unsigned short /*y*/,
			unsigned short /*Button*/, unsigned short /*Mod*/)
{
	MarkDirty();
	State = 0;
	Frames[IE_GUI_SCROLLBAR_SLIDER]->YPos = 0; //this is to clear any offset incurred by grabbing the slider
}

/** Mousewheel scroll */
void ScrollBar::OnMouseWheelScroll(short /*x*/, short y)
{
	if ( State == 0 ){//dont allow mousewheel to do anything if the slider is being interacted with already.
		unsigned short fauxY = SliderYPos;
		if ((short)fauxY + y <= 0) fauxY = 0;
		else fauxY += y;
		SetPosForY(fauxY);
	}
}

/** Mouse Over Event */
void ScrollBar::OnMouseOver(unsigned short /*x*/, unsigned short y)
{
	ieWord top = (Height - SliderRange)/2;
	if (State&SLIDER_GRAB) {
		if (y >= top) {
			SetPosForY(y - top);
		} else {
			SetPosForY(0);
		}
	}
}

/** Sets the Maximum Value of the ScrollBar */
void ScrollBar::SetMax(unsigned short Max)
{
	Value = Max;
	if (Max == 0) {
		SetPos( 0 );
	} else if (Pos >= Max){
		SetPos( Max - 1 );
	}
}

/** Sets the ScrollBarOnChange event (guiscript callback) */
bool ScrollBar::SetEvent(int eventType, EventHandler handler)
{
	switch (eventType) {
	case IE_GUI_SCROLLBAR_ON_CHANGE:
		ScrollBarOnChange = handler;
		break;
	default:
		return false;
	}

	return true;
}

}
