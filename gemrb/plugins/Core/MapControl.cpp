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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/MapControl.cpp,v 1.4 2004/08/25 11:55:51 avenger_teambg Exp $
 */

#include "../../includes/win32def.h"
#include "MapControl.h"
#include "Interface.h"

// Ratio between pixel sizes of an Area (Big map) and a Small map
#define MAP_SCALE 10.67

static Color green = {
	0x00, 0xff, 0x00, 0xff
};
static Color darkgreen = {
	0x00, 0x80, 0x00, 0xff
};

#define MAP_TO_SCREENX(x) XPos + XCenter - ScrollX + (x)
#define MAP_TO_SCREENY(y) YPos + YCenter - ScrollY + (y)
// Omit [XY]Pos, since these macros are used in OnMouseDown(x, y), and x, y is 
//   already relative to control [XY]Pos there
#define SCREEN_TO_MAPX(x) (x) - XCenter + ScrollX
#define SCREEN_TO_MAPY(y) (y) - YCenter + ScrollY

#define GAME_TO_SCREENX(x) MAP_TO_SCREENX((int)((x) / MAP_SCALE))
#define GAME_TO_SCREENY(y) MAP_TO_SCREENY((int)((y) / MAP_SCALE))


MapControl::MapControl(void)
{
	ScrollX = 0;
	ScrollY = 0;
	MouseIsDown = false;
	Changed = true;

	MapMOS = core->GetGame()->GetCurrentMap()->SmallMap->GetImage();
}


MapControl::~MapControl(void)
{
}

// To be called after changes in control's or screen geometry
void MapControl::Realize()
{
	Video* video = core->GetVideoDriver();

	// FIXME: ugly!! How to get area size in pixels?
	MapWidth = (int)(MapMOS->Width * MAP_SCALE);
	MapHeight = (int)(MapMOS->Height * MAP_SCALE);

	// FIXME: ugly hack! What is the actual viewport size?
	Region vp = video->GetViewport();
	ViewWidth = (int)((vp.w ? vp.w : core->Width) / MAP_SCALE);
	ViewHeight = (int)((vp.h ? vp.h : core->Height) / MAP_SCALE);

	XCenter = (Width - MapMOS->Width) / 2;
	YCenter = (Height - MapMOS->Height) / 2;
	if (XCenter < 0) XCenter = 0;
	if (YCenter < 0) YCenter = 0;
}

/** Draws the Control on the Output Display */
void MapControl::Draw(unsigned short /*x*/, unsigned short /*y*/)
{
	if (!Width || !Height) {
		return;
	}

	if (Changed) {
		Realize();
		Changed = false;
	}

	Video* video = core->GetVideoDriver();
	Region r( XPos, YPos, Width, Height );

	video->BlitSprite( MapMOS, MAP_TO_SCREENX(0), MAP_TO_SCREENY(0), true, &r );

	Region vp = video->GetViewport();

	vp.x = GAME_TO_SCREENX(vp.x);
	vp.y = GAME_TO_SCREENY(vp.y);
	vp.w = ViewWidth;
	vp.h = ViewHeight;

	video->DrawRect( vp, green, false, false );

	// Draw PCs' ellipses
	for (int i = 1; i < 9; i++) {
		Actor* actor = core->GetGame()->FindPC( i );
		if (actor) {
			video->DrawEllipse( GAME_TO_SCREENX(actor->XPos), GAME_TO_SCREENY(actor->YPos), 3, 2, i == 1 ? green : darkgreen, false );
		}
	}
}

/** Key Press Event */
void MapControl::OnKeyPress(unsigned char /*Key*/, unsigned short /*Mod*/)
{
}

/** Key Release Event */
void MapControl::OnKeyRelease(unsigned char Key, unsigned short Mod)
{
	switch (Key) {
		case '\t':
			//not GEM_TAB
			printf( "TAB released\n" );
			return;
	        case 'f':
			if (Mod & 64)
				core->GetVideoDriver()->ToggleFullscreenMode();
			break;
		case 'g':
			if (Mod & 64)
				core->GetVideoDriver()->ToggleGrabInput();
			break;
		default:
			break;
	}
	if (!core->CheatEnabled()) {
		return;
	}
	if (Mod & 64) //ctrl
	{
	}
}
/** Mouse Over Event */
void MapControl::OnMouseOver(unsigned short x, unsigned short y)
{
	if (MouseIsDown) {
		ScrollX -= x - lastMouseX;
		ScrollY -= y - lastMouseY;

		if (ScrollX > MapMOS->Width - Width)
			ScrollX = MapMOS->Width - Width;
		if (ScrollY > MapMOS->Height - Height)
			ScrollY = MapMOS->Height - Height;
		if (ScrollX < 0)
			ScrollX = 0;
		if (ScrollY < 0)
			ScrollY = 0;
	}

	lastMouseX = x;
	lastMouseY = y;


	x += ScrollX;
	y += ScrollY;

}

/** Mouse Button Down */
void MapControl::OnMouseDown(unsigned short x, unsigned short y,
	unsigned char Button, unsigned short /*Mod*/)
{
	if ((Button != GEM_MB_ACTION) ) {
		return;
	}
	MouseIsDown = true;
	lastMouseX = x;
	lastMouseY = y;


	short xp = SCREEN_TO_MAPX(x) - ViewWidth / 2;
	short yp = SCREEN_TO_MAPY(y) - ViewHeight / 2;

	if (xp + ViewWidth > MapMOS->Width) xp = MapMOS->Width - ViewWidth;
	if (yp + ViewHeight > MapMOS->Height) yp = MapMOS->Height - ViewHeight;
	if (xp < 0) xp = 0;
	if (yp < 0) yp = 0;

	core->GetVideoDriver()->SetViewport( (int)(xp * MAP_SCALE), (int)(yp * MAP_SCALE) );

	// FIXME: play button sound here
}

/** Mouse Button Up */
void MapControl::OnMouseUp(unsigned short /*x*/, unsigned short /*y*/,
	unsigned char Button, unsigned short /*Mod*/)
{
	if (Button != GEM_MB_ACTION) {
		return;
	}

	MouseIsDown = false;
}

/** Special Key Press */
void MapControl::OnSpecialKeyPress(unsigned char Key)
{
	switch (Key) {
		case GEM_LEFT:
			ScrollX -= 64;
			break;
		case GEM_UP:
			ScrollY -= 64;
			break;
		case GEM_RIGHT:
			ScrollX += 64;
			break;
		case GEM_DOWN:
			ScrollY += 64;
			break;
		case GEM_ALT:
			printf( "ALT pressed\n" );
			break;
		case GEM_TAB:
			printf( "TAB pressed\n" );
			break;
	}

	if (ScrollX > MapMOS->Width - Width)
		ScrollX = MapMOS->Width - Width;
	if (ScrollY > MapMOS->Height - Height)
		ScrollY = MapMOS->Height - Height;
	if (ScrollX < 0)
		ScrollX = 0;
	if (ScrollY < 0)
		ScrollY = 0;
}

